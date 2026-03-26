#include "PriceDB.h"
#include "TPAPI.h"

#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace FlipOut {

    // Static member initialization
    std::unordered_map<uint32_t, std::vector<PriceSnapshot>> PriceDB::s_history;
    std::vector<WatchlistEntry> PriceDB::s_watchlist;
    std::mutex PriceDB::s_mutex;

    std::string PriceDB::GetDataDirectory() {
        return TPAPI::GetDataDirectory();
    }

    void PriceDB::RecordPrice(uint32_t item_id, int buy_price, int sell_price,
                               int buy_quantity, int sell_quantity) {
        std::lock_guard<std::mutex> lock(s_mutex);
        PriceSnapshot snap;
        snap.timestamp = std::time(nullptr);
        snap.buy_price = buy_price;
        snap.sell_price = sell_price;
        snap.buy_quantity = buy_quantity;
        snap.sell_quantity = sell_quantity;
        s_history[item_id].push_back(snap);
    }

    void PriceDB::RecordBulkPrices(const std::unordered_map<uint32_t, PriceSnapshot>& prices) {
        std::lock_guard<std::mutex> lock(s_mutex);
        time_t now = std::time(nullptr);
        for (const auto& [id, snap] : prices) {
            PriceSnapshot s = snap;
            if (s.timestamp == 0) s.timestamp = now;

            // Skip outlier prices (manipulated sell orders with absurd ratios)
            if (s.buy_price > 0 && s.sell_price > 0) {
                float ratio = (float)s.sell_price / (float)s.buy_price;
                if (ratio > 10.0f) continue;
            }

            auto& history = s_history[id];
            // Don't duplicate if last entry is within 60 seconds
            if (!history.empty()) {
                const auto& last = history.back();
                if (std::abs(difftime(s.timestamp, last.timestamp)) < 60 &&
                    last.buy_price == s.buy_price && last.sell_price == s.sell_price) {
                    continue;
                }
            }
            history.push_back(s);
        }
    }

    std::vector<PriceSnapshot> PriceDB::GetHistory(uint32_t item_id, int max_entries) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_history.find(item_id);
        if (it == s_history.end()) return {};

        if (max_entries <= 0 || (int)it->second.size() <= max_entries) {
            return it->second;
        }

        // Return last N entries
        return std::vector<PriceSnapshot>(
            it->second.end() - max_entries, it->second.end());
    }

    PriceSnapshot PriceDB::GetLatest(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_history.find(item_id);
        if (it == s_history.end() || it->second.empty()) return {};
        return it->second.back();
    }

    size_t PriceDB::GetTrackedItemCount() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_history.size();
    }

    void PriceDB::Trim(int max_entries_per_item, int max_age_days) {
        std::lock_guard<std::mutex> lock(s_mutex);
        time_t cutoff = std::time(nullptr) - (max_age_days * 86400);

        for (auto& [id, history] : s_history) {
            // Remove entries older than max_age_days
            history.erase(
                std::remove_if(history.begin(), history.end(),
                    [cutoff](const PriceSnapshot& s) { return s.timestamp < cutoff; }),
                history.end());

            // Trim to max entries (keep newest)
            if ((int)history.size() > max_entries_per_item) {
                history.erase(history.begin(),
                    history.begin() + (history.size() - max_entries_per_item));
            }
        }

        // Remove items with empty history
        for (auto it = s_history.begin(); it != s_history.end(); ) {
            if (it->second.empty()) it = s_history.erase(it);
            else ++it;
        }
    }

    // --- Watchlist ---

    void PriceDB::AddToWatchlist(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& entry : s_watchlist) {
            if (entry.item_id == item_id) return;
        }
        WatchlistEntry entry;
        entry.item_id = item_id;
        s_watchlist.push_back(entry);
    }

    void PriceDB::RemoveFromWatchlist(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_watchlist.erase(
            std::remove_if(s_watchlist.begin(), s_watchlist.end(),
                [item_id](const WatchlistEntry& e) { return e.item_id == item_id; }),
            s_watchlist.end());
    }

    bool PriceDB::IsOnWatchlist(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& e : s_watchlist) {
            if (e.item_id == item_id) return true;
        }
        return false;
    }

    std::vector<WatchlistEntry>& PriceDB::GetWatchlist() {
        return s_watchlist;
    }

    WatchlistEntry* PriceDB::GetWatchlistEntry(uint32_t item_id) {
        for (auto& e : s_watchlist) {
            if (e.item_id == item_id) return &e;
        }
        return nullptr;
    }

    // --- Persistence ---

    bool PriceDB::Save() {
        TPAPI::EnsureDataDirectory();
        std::string path = GetDataDirectory() + "/price_history.json";
        try {
            json j = json::object();
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                for (const auto& [id, history] : s_history) {
                    json arr = json::array();
                    for (const auto& snap : history) {
                        arr.push_back({
                            {"t", (int64_t)snap.timestamp},
                            {"bp", snap.buy_price},
                            {"sp", snap.sell_price},
                            {"bq", snap.buy_quantity},
                            {"sq", snap.sell_quantity}
                        });
                    }
                    j[std::to_string(id)] = arr;
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

    bool PriceDB::Load() {
        std::string path = GetDataDirectory() + "/price_history.json";
        std::ifstream file(path);
        if (!file.is_open()) return false;
        try {
            json j;
            file >> j;
            std::lock_guard<std::mutex> lock(s_mutex);
            if (j.is_object()) {
                for (auto& [key, arr] : j.items()) {
                    uint32_t id = std::stoul(key);
                    std::vector<PriceSnapshot> history;
                    if (arr.is_array()) {
                        for (const auto& entry : arr) {
                            PriceSnapshot snap;
                            snap.timestamp = (time_t)entry.value("t", (int64_t)0);
                            snap.buy_price = entry.value("bp", 0);
                            snap.sell_price = entry.value("sp", 0);
                            snap.buy_quantity = entry.value("bq", 0);
                            snap.sell_quantity = entry.value("sq", 0);
                            history.push_back(snap);
                        }
                    }
                    s_history[id] = history;
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    bool PriceDB::SaveWatchlist() {
        TPAPI::EnsureDataDirectory();
        std::string path = GetDataDirectory() + "/watchlist.json";
        try {
            json j = json::array();
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                for (const auto& e : s_watchlist) {
                    j.push_back({
                        {"item_id", e.item_id},
                        {"alert_margin", e.alert_on_margin},
                        {"margin_threshold", e.margin_threshold},
                        {"alert_price_drop", e.alert_on_price_drop},
                        {"price_drop_target", e.price_drop_target}
                    });
                }
            }
            std::ofstream file(path);
            if (!file.is_open()) return false;
            file << j.dump(2);
            file.flush();
            return true;
        } catch (...) {
            return false;
        }
    }

    bool PriceDB::LoadWatchlist() {
        std::string path = GetDataDirectory() + "/watchlist.json";
        std::ifstream file(path);
        if (!file.is_open()) return false;
        try {
            json j;
            file >> j;
            std::lock_guard<std::mutex> lock(s_mutex);
            s_watchlist.clear();
            if (j.is_array()) {
                for (const auto& entry : j) {
                    WatchlistEntry e;
                    e.item_id = entry.value("item_id", (uint32_t)0);
                    e.alert_on_margin = entry.value("alert_margin", false);
                    e.margin_threshold = entry.value("margin_threshold", 15.0f);
                    e.alert_on_price_drop = entry.value("alert_price_drop", false);
                    e.price_drop_target = entry.value("price_drop_target", 0);
                    if (e.item_id != 0) s_watchlist.push_back(e);
                }
            }
            return true;
        } catch (...) {
            return false;
        }
    }

    // --- Seed Data ---

    bool PriceDB::ExportSeed(const std::string& path, int max_entries_per_item) {
        try {
            json j = json::object();
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                for (const auto& [id, history] : s_history) {
                    if (history.empty()) continue;
                    json arr = json::array();
                    int start = (int)history.size() > max_entries_per_item
                        ? (int)history.size() - max_entries_per_item : 0;
                    for (int i = start; i < (int)history.size(); i++) {
                        const auto& snap = history[i];
                        arr.push_back({
                            {"t", (int64_t)snap.timestamp},
                            {"b", snap.buy_price},
                            {"s", snap.sell_price},
                            {"bq", snap.buy_quantity},
                            {"sq", snap.sell_quantity}
                        });
                    }
                    j[std::to_string(id)] = arr;
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

    bool PriceDB::ImportSeed(const std::string& json_data) {
        try {
            json j = json::parse(json_data);
            if (!j.is_object()) return false;

            std::lock_guard<std::mutex> lock(s_mutex);
            int imported = 0;
            for (auto& [key, arr] : j.items()) {
                uint32_t id = std::stoul(key);
                if (!arr.is_array()) continue;

                auto& local = s_history[id];
                time_t earliest_local = 0;
                if (!local.empty()) {
                    earliest_local = local.front().timestamp;
                }

                std::vector<PriceSnapshot> new_entries;
                for (const auto& entry : arr) {
                    PriceSnapshot snap;
                    snap.timestamp = (time_t)entry.value("t", (int64_t)0);
                    snap.buy_price = entry.value("b", 0);
                    snap.sell_price = entry.value("s", 0);
                    snap.buy_quantity = entry.value("bq", 0);
                    snap.sell_quantity = entry.value("sq", 0);

                    // Only import entries older than our earliest local data
                    if (local.empty() || snap.timestamp < earliest_local) {
                        new_entries.push_back(snap);
                    }
                }

                if (!new_entries.empty()) {
                    std::sort(new_entries.begin(), new_entries.end(),
                        [](const PriceSnapshot& a, const PriceSnapshot& b) {
                            return a.timestamp < b.timestamp;
                        });
                    new_entries.insert(new_entries.end(), local.begin(), local.end());
                    local = std::move(new_entries);
                    imported++;
                }
            }
            return imported > 0;
        } catch (...) {
            return false;
        }
    }

}
