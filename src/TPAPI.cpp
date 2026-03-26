#include "TPAPI.h"
#include "HttpClient.h"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <thread>
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace FlipOut {

    // Static member initialization
    FetchStatus TPAPI::s_bulk_fetch_status = FetchStatus::Idle;
    std::string TPAPI::s_bulk_fetch_message;
    float TPAPI::s_bulk_fetch_progress = 0.0f;
    std::atomic<bool> TPAPI::s_scan_cancelled{false};
    std::unordered_map<uint32_t, TPPrice> TPAPI::s_prices;
    std::unordered_map<uint32_t, ItemInfo> TPAPI::s_item_cache;
    std::mutex TPAPI::s_mutex;

    // --- Helpers ---

    static std::string GetDllDir() {
        char dllPath[MAX_PATH];
        HMODULE hModule = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)GetDllDir, &hModule)) {
            if (GetModuleFileNameA(hModule, dllPath, MAX_PATH)) {
                std::string path(dllPath);
                size_t lastSlash = path.find_last_of("\\/");
                if (lastSlash != std::string::npos) {
                    return path.substr(0, lastSlash);
                }
            }
        }
        return "";
    }

    static bool ContainsCI(const std::string& haystack, const std::string& needle) {
        if (needle.empty()) return true;
        if (haystack.size() < needle.size()) return false;
        auto it = std::search(haystack.begin(), haystack.end(),
                              needle.begin(), needle.end(),
                              [](char a, char b) { return std::tolower(a) == std::tolower(b); });
        return it != haystack.end();
    }

    // --- Data Directory ---

    std::string TPAPI::GetDataDirectory() {
        std::string dir = GetDllDir();
        if (!dir.empty()) {
            std::replace(dir.begin(), dir.end(), '\\', '/');
        }
        return dir + "/FlipOut";
    }

    bool TPAPI::EnsureDataDirectory() {
        std::string dir = GetDataDirectory();
        try {
            std::filesystem::create_directories(dir);
            return true;
        } catch (...) {
            return false;
        }
    }

    // --- TP Price Fetching (public endpoints, no auth) ---

    std::vector<TPPrice> TPAPI::FetchPrices(const std::vector<uint32_t>& item_ids) {
        std::vector<TPPrice> results;
        if (item_ids.empty()) return results;

        const size_t BATCH_SIZE = 200;
        for (size_t i = 0; i < item_ids.size(); i += BATCH_SIZE) {
            std::string ids_param;
            size_t end = std::min(i + BATCH_SIZE, item_ids.size());
            for (size_t j = i; j < end; j++) {
                if (!ids_param.empty()) ids_param += ",";
                ids_param += std::to_string(item_ids[j]);
            }

            std::string url = "https://api.guildwars2.com/v2/commerce/prices?ids=" + ids_param;
            std::string response = HttpClient::Get(url);
            if (response.empty()) continue;

            try {
                json j = json::parse(response);
                if (j.is_array()) {
                    for (const auto& item : j) {
                        TPPrice p;
                        p.item_id = item.value("id", (uint32_t)0);
                        if (item.contains("buys")) {
                            p.buy_price = item["buys"].value("unit_price", 0);
                            p.buy_quantity = item["buys"].value("quantity", 0);
                        }
                        if (item.contains("sells")) {
                            p.sell_price = item["sells"].value("unit_price", 0);
                            p.sell_quantity = item["sells"].value("quantity", 0);
                        }
                        results.push_back(p);
                    }
                }
            } catch (...) {}
        }
        return results;
    }

    TPListingDetail TPAPI::FetchListings(uint32_t item_id) {
        TPListingDetail detail;
        detail.item_id = item_id;

        std::string url = "https://api.guildwars2.com/v2/commerce/listings/" + std::to_string(item_id);
        std::string response = HttpClient::Get(url);
        if (response.empty()) return detail;

        try {
            json j = json::parse(response);
            if (j.contains("buys") && j["buys"].is_array()) {
                for (const auto& entry : j["buys"]) {
                    TPListing l;
                    l.listings = entry.value("listings", 0);
                    l.unit_price = entry.value("unit_price", 0);
                    l.quantity = entry.value("quantity", 0);
                    detail.buys.push_back(l);
                }
            }
            if (j.contains("sells") && j["sells"].is_array()) {
                for (const auto& entry : j["sells"]) {
                    TPListing l;
                    l.listings = entry.value("listings", 0);
                    l.unit_price = entry.value("unit_price", 0);
                    l.quantity = entry.value("quantity", 0);
                    detail.sells.push_back(l);
                }
            }
        } catch (...) {}
        return detail;
    }

    // --- Item Info ---

    std::vector<ItemInfo> TPAPI::FetchItemInfo(const std::vector<uint32_t>& item_ids) {
        std::vector<ItemInfo> results;
        if (item_ids.empty()) return results;

        // Filter out already-cached
        std::vector<uint32_t> to_fetch;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            for (uint32_t id : item_ids) {
                if (s_item_cache.find(id) == s_item_cache.end()) {
                    to_fetch.push_back(id);
                }
            }
        }

        // Deduplicate
        std::sort(to_fetch.begin(), to_fetch.end());
        to_fetch.erase(std::unique(to_fetch.begin(), to_fetch.end()), to_fetch.end());

        const size_t BATCH_SIZE = 200;
        for (size_t i = 0; i < to_fetch.size(); i += BATCH_SIZE) {
            std::string ids_param;
            size_t end = std::min(i + BATCH_SIZE, to_fetch.size());
            for (size_t j = i; j < end; j++) {
                if (!ids_param.empty()) ids_param += ",";
                ids_param += std::to_string(to_fetch[j]);
            }

            std::string url = "https://api.guildwars2.com/v2/items?ids=" + ids_param;
            std::string response = HttpClient::Get(url);
            if (response.empty()) continue;

            try {
                json j = json::parse(response);
                if (j.is_array()) {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    for (const auto& item : j) {
                        if (!item.contains("id")) continue;
                        ItemInfo info;
                        info.id = item["id"].get<uint32_t>();
                        info.name = item.value("name", "");
                        info.icon_url = item.value("icon", "");
                        info.rarity = item.value("rarity", "");
                        info.type = item.value("type", "");
                        info.vendor_value = item.value("vendor_value", 0);
                        info.level = item.value("level", 0);
                        s_item_cache[info.id] = info;
                        results.push_back(info);
                    }
                }
            } catch (...) {}
        }

        // Also return cached items that were requested
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            for (uint32_t id : item_ids) {
                auto it = s_item_cache.find(id);
                if (it != s_item_cache.end()) {
                    bool already = false;
                    for (const auto& r : results) {
                        if (r.id == id) { already = true; break; }
                    }
                    if (!already) results.push_back(it->second);
                }
            }
        }

        return results;
    }

    const ItemInfo* TPAPI::GetItemInfo(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_item_cache.find(item_id);
        if (it != s_item_cache.end()) return &it->second;
        return nullptr;
    }

    std::vector<ItemInfo> TPAPI::SearchItems(const std::string& query, int max_results) {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::vector<ItemInfo> results;
        if (query.empty()) return results;

        for (const auto& [id, info] : s_item_cache) {
            if (ContainsCI(info.name, query)) {
                results.push_back(info);
                if ((int)results.size() >= max_results) break;
            }
        }

        std::sort(results.begin(), results.end(), [](const ItemInfo& a, const ItemInfo& b) {
            std::string al = a.name, bl = b.name;
            std::transform(al.begin(), al.end(), al.begin(), ::tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            return al < bl;
        });

        return results;
    }

    // --- Fetch all tradeable item IDs ---

    std::vector<uint32_t> TPAPI::FetchAllTradeableIds() {
        std::vector<uint32_t> ids;
        std::string url = "https://api.guildwars2.com/v2/commerce/prices";
        std::string response = HttpClient::Get(url);
        if (response.empty()) return ids;

        try {
            json j = json::parse(response);
            if (j.is_array()) {
                ids.reserve(j.size());
                for (const auto& id : j) {
                    ids.push_back(id.get<uint32_t>());
                }
            }
        } catch (...) {}
        return ids;
    }

    // --- Cancel support ---

    void TPAPI::CancelScan() {
        s_scan_cancelled.store(true);
    }

    bool TPAPI::IsScanCancelled() {
        return s_scan_cancelled.load();
    }

    // --- Bulk Fetch All Prices (async, public endpoints) ---

    static const int THROTTLE_MS = 100; // delay between API batches to avoid rate limits

    void TPAPI::FetchAllPricesAsync() {
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_bulk_fetch_status = FetchStatus::InProgress;
            s_bulk_fetch_message = "Fetching tradeable item list...";
            s_bulk_fetch_progress = 0.0f;
        }
        s_scan_cancelled.store(false);

        std::thread([]() {
            try {
                // 1. Get all tradeable IDs
                auto ids = FetchAllTradeableIds();
                if (ids.empty()) {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_bulk_fetch_status = FetchStatus::Error;
                    s_bulk_fetch_message = "Failed to fetch tradeable item list";
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_bulk_fetch_message = "Fetching prices for " + std::to_string(ids.size()) + " items...";
                }

                // 2. Fetch prices in batches
                const size_t BATCH = 200;
                std::unordered_map<uint32_t, TPPrice> all_prices;
                size_t total = ids.size();

                for (size_t i = 0; i < total; i += BATCH) {
                    // Check for cancellation
                    if (s_scan_cancelled.load()) {
                        std::lock_guard<std::mutex> lock(s_mutex);
                        s_bulk_fetch_status = FetchStatus::Idle;
                        s_bulk_fetch_message = "Scan cancelled";
                        s_bulk_fetch_progress = 0.0f;
                        return;
                    }

                    std::string ids_param;
                    size_t end = std::min(i + BATCH, total);
                    for (size_t j = i; j < end; j++) {
                        if (!ids_param.empty()) ids_param += ",";
                        ids_param += std::to_string(ids[j]);
                    }

                    std::string url = "https://api.guildwars2.com/v2/commerce/prices?ids=" + ids_param;
                    std::string response = HttpClient::Get(url);

                    if (!response.empty()) {
                        try {
                            json j = json::parse(response);
                            if (j.is_array()) {
                                for (const auto& item : j) {
                                    TPPrice p;
                                    p.item_id = item.value("id", (uint32_t)0);
                                    if (item.contains("buys")) {
                                        p.buy_price = item["buys"].value("unit_price", 0);
                                        p.buy_quantity = item["buys"].value("quantity", 0);
                                    }
                                    if (item.contains("sells")) {
                                        p.sell_price = item["sells"].value("unit_price", 0);
                                        p.sell_quantity = item["sells"].value("quantity", 0);
                                    }
                                    all_prices[p.item_id] = p;
                                }
                            }
                        } catch (...) {}
                    }

                    float progress = (float)end / (float)total;
                    {
                        std::lock_guard<std::mutex> lock(s_mutex);
                        s_bulk_fetch_progress = progress;
                        s_bulk_fetch_message = "Fetching prices... " +
                            std::to_string((int)(progress * 100)) + "% (" +
                            std::to_string(all_prices.size()) + " items)";
                    }

                    // Throttle to avoid rate limiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(THROTTLE_MS));
                }

                // 3. Fetch item info for everything we got prices for
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_bulk_fetch_message = "Fetching item details...";
                }

                // Collect IDs that need info
                std::vector<uint32_t> need_info;
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    for (const auto& [id, _] : all_prices) {
                        if (s_item_cache.find(id) == s_item_cache.end()) {
                            need_info.push_back(id);
                        }
                    }
                }

                // Fetch info in batches
                for (size_t i = 0; i < need_info.size(); i += BATCH) {
                    // Check for cancellation
                    if (s_scan_cancelled.load()) {
                        // Still apply partial prices we already fetched
                        std::lock_guard<std::mutex> lock(s_mutex);
                        s_prices = all_prices;
                        s_bulk_fetch_status = FetchStatus::Idle;
                        s_bulk_fetch_message = "Scan cancelled (" + std::to_string(all_prices.size()) + " prices kept)";
                        s_bulk_fetch_progress = 0.0f;
                        return;
                    }

                    std::string ids_param;
                    size_t end = std::min(i + BATCH, need_info.size());
                    for (size_t j = i; j < end; j++) {
                        if (!ids_param.empty()) ids_param += ",";
                        ids_param += std::to_string(need_info[j]);
                    }

                    std::string url = "https://api.guildwars2.com/v2/items?ids=" + ids_param;
                    std::string response = HttpClient::Get(url);
                    if (!response.empty()) {
                        try {
                            json j = json::parse(response);
                            if (j.is_array()) {
                                std::lock_guard<std::mutex> lock(s_mutex);
                                for (const auto& item : j) {
                                    if (!item.contains("id")) continue;
                                    ItemInfo info;
                                    info.id = item["id"].get<uint32_t>();
                                    info.name = item.value("name", "");
                                    info.icon_url = item.value("icon", "");
                                    info.rarity = item.value("rarity", "");
                                    info.type = item.value("type", "");
                                    info.vendor_value = item.value("vendor_value", 0);
                                    info.level = item.value("level", 0);
                                    s_item_cache[info.id] = info;
                                }
                            }
                        } catch (...) {}
                    }

                    float progress = (float)(i + BATCH) / (float)need_info.size();
                    {
                        std::lock_guard<std::mutex> lock(s_mutex);
                        s_bulk_fetch_message = "Fetching item details... " +
                            std::to_string((int)(std::min(progress, 1.0f) * 100)) + "%";
                    }

                    // Throttle to avoid rate limiting
                    std::this_thread::sleep_for(std::chrono::milliseconds(THROTTLE_MS));
                }

                // 4. Apply
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_prices = all_prices;
                    s_bulk_fetch_status = FetchStatus::Success;
                    s_bulk_fetch_progress = 1.0f;
                    s_bulk_fetch_message = "Done (" + std::to_string(all_prices.size()) + " items)";
                }

                SaveItemCache();

            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_bulk_fetch_status = FetchStatus::Error;
                s_bulk_fetch_message = std::string("Error: ") + e.what();
            }
        }).detach();
    }

    FetchStatus TPAPI::GetBulkFetchStatus() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_bulk_fetch_status;
    }

    const std::string& TPAPI::GetBulkFetchMessage() {
        return s_bulk_fetch_message;
    }

    float TPAPI::GetBulkFetchProgress() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_bulk_fetch_progress;
    }

    std::unordered_map<uint32_t, TPPrice> TPAPI::GetAllPrices() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_prices;
    }

    void TPAPI::UpdatePrices(const std::unordered_map<uint32_t, TPPrice>& prices) {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& [id, p] : prices) {
            s_prices[id] = p;
        }
    }

    // --- Persistence ---

    bool TPAPI::SaveItemCache() {
        EnsureDataDirectory();
        std::string path = GetDataDirectory() + "/item_cache.json";
        try {
            json j = json::object();
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                for (const auto& [id, info] : s_item_cache) {
                    j[std::to_string(id)] = {
                        {"name", info.name},
                        {"icon", info.icon_url},
                        {"rarity", info.rarity},
                        {"type", info.type},
                        {"vendor_value", info.vendor_value},
                        {"level", info.level}
                    };
                }
            }
            std::ofstream file(path);
            if (!file.is_open()) return false;
            file << j.dump();
            file.flush();
            return true;
        } catch (...) {
            return false;
        }
    }

    bool TPAPI::LoadItemCache() {
        std::string path = GetDataDirectory() + "/item_cache.json";
        std::ifstream file(path);
        if (!file.is_open()) return false;
        try {
            json j;
            file >> j;
            std::lock_guard<std::mutex> lock(s_mutex);
            if (j.is_object()) {
                for (auto& [key, val] : j.items()) {
                    uint32_t id = std::stoul(key);
                    ItemInfo info;
                    info.id = id;
                    info.name = val.value("name", "");
                    info.icon_url = val.value("icon", "");
                    info.rarity = val.value("rarity", "");
                    info.type = val.value("type", "");
                    info.vendor_value = val.value("vendor_value", 0);
                    info.level = val.value("level", 0);
                    s_item_cache[id] = info;
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }

}
