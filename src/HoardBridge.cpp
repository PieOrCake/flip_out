#include "HoardBridge.h"
#include "PriceDB.h"

#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace FlipOut {

    // Event names for our response events (H&S raises these back to us)
    #define EV_FO_ITEM_RESPONSE     "EV_FLIPOUT_ITEM_RESPONSE"
    #define EV_FO_API_RESPONSE      "EV_FLIPOUT_API_RESPONSE"
    #define EV_FO_TX_BUYS_RESPONSE  "EV_FLIPOUT_TX_BUYS_RESPONSE"
    #define EV_FO_TX_SELLS_RESPONSE "EV_FLIPOUT_TX_SELLS_RESPONSE"
    #define REQUESTER_NAME          "Flip Out"
    #define EV_FO_CONTEXT_WATCH     "EV_FLIPOUT_WATCH_ITEM"
    #define EV_FO_RECIPE_RESPONSE   "EV_FLIPOUT_RECIPE_RESPONSE"
    #define EV_FO_PERM_DISCARD      "EV_FLIPOUT_PERM_CHECK_DISCARD"
    #define EV_FO_PERM_DISCARD_ITEM "EV_FLIPOUT_PERM_CHECK_DISCARD_ITEM"
    #define FLIPOUT_SIGNATURE       0x1be8c2d6

    // Static member initialization
    AddonAPI_t* HoardBridge::s_API = nullptr;
    bool HoardBridge::s_hoardDetected = false;
    bool HoardBridge::s_hoardDataAvailable = false;
    bool HoardBridge::s_refreshNeeded = false;
    int64_t HoardBridge::s_lastUpdated = 0;
    int64_t HoardBridge::s_refreshAvailableAt = 0;
    bool HoardBridge::s_hoardFetching = false;
    std::string HoardBridge::s_fetchProgressMsg;
    float HoardBridge::s_fetchProgress = 0.0f;
    bool HoardBridge::s_permissionPending = false;
    bool HoardBridge::s_permissionDenied = false;
    std::vector<PendingRetry> HoardBridge::s_retryQueue;
    std::unordered_map<int, int> HoardBridge::s_retryAttempts;
    std::unordered_set<uint32_t> HoardBridge::s_queriedItems;
    std::unordered_map<uint32_t, OwnedItem> HoardBridge::s_ownedItems;
    std::vector<TPTransaction> HoardBridge::s_currentBuys;
    std::vector<TPTransaction> HoardBridge::s_currentSells;
    bool HoardBridge::s_txLoading = false;
    bool HoardBridge::s_txLoaded = false;
    bool HoardBridge::s_txBuysReceived = false;
    bool HoardBridge::s_txSellsReceived = false;
    std::unordered_set<uint32_t> HoardBridge::s_unlockedRecipes;
    std::unordered_set<uint32_t> HoardBridge::s_queriedRecipes;
    bool HoardBridge::s_recipeQueryDone = false;
    int HoardBridge::s_recipeBatchesPending = 0;
    std::function<void(const std::string&)> HoardBridge::s_apiCallback;
    std::mutex HoardBridge::s_mutex;

    // --- Lifecycle ---

    void HoardBridge::Initialize(AddonAPI_t* api) {
        s_API = api;

        // Subscribe to H&S broadcasts (do NOT delete these payloads in handlers)
        s_API->Events_Subscribe(EV_HOARD_PONG, OnPong);
        s_API->Events_Subscribe(EV_HOARD_DATA_UPDATED, OnDataUpdated);
        s_API->Events_Subscribe(EV_HOARD_FETCH_PROGRESS, OnFetchProgress);
        s_API->Events_Subscribe(EV_HOARD_FETCH_ERROR, OnFetchError);

        // Subscribe to our own response events (MUST delete payloads in handlers)
        s_API->Events_Subscribe(EV_FO_ITEM_RESPONSE, OnItemQueryResponse);
        s_API->Events_Subscribe(EV_FO_API_RESPONSE, OnApiQueryResponse);
        s_API->Events_Subscribe(EV_FO_TX_BUYS_RESPONSE, OnTxBuysResponse);
        s_API->Events_Subscribe(EV_FO_TX_SELLS_RESPONSE, OnTxSellsResponse);

        // Subscribe to recipe query response (MUST delete payload)
        s_API->Events_Subscribe(EV_FO_RECIPE_RESPONSE, OnRecipeQueryResponse);

        // Subscribe to context menu callback (MUST delete payload)
        s_API->Events_Subscribe(EV_FO_CONTEXT_WATCH, OnContextMenuWatch);

        // Subscribe to discard handlers for permission-check dummy queries
        s_API->Events_Subscribe(EV_FO_PERM_DISCARD, OnPermCheckDiscard);
        s_API->Events_Subscribe(EV_FO_PERM_DISCARD_ITEM, OnPermCheckDiscardItem);

        s_API->Log(LOGL_INFO, "FlipOut", "Subscribed to Hoard & Seek events");

        // Initial ping to detect H&S
        Ping();

        // Register context menu item with H&S
        RegisterContextMenu();
    }

    void HoardBridge::Shutdown() {
        if (!s_API) return;

        s_API->Events_Unsubscribe(EV_HOARD_PONG, OnPong);
        s_API->Events_Unsubscribe(EV_HOARD_DATA_UPDATED, OnDataUpdated);
        s_API->Events_Unsubscribe(EV_HOARD_FETCH_PROGRESS, OnFetchProgress);
        s_API->Events_Unsubscribe(EV_HOARD_FETCH_ERROR, OnFetchError);
        s_API->Events_Unsubscribe(EV_FO_ITEM_RESPONSE, OnItemQueryResponse);
        s_API->Events_Unsubscribe(EV_FO_API_RESPONSE, OnApiQueryResponse);
        s_API->Events_Unsubscribe(EV_FO_TX_BUYS_RESPONSE, OnTxBuysResponse);
        s_API->Events_Unsubscribe(EV_FO_TX_SELLS_RESPONSE, OnTxSellsResponse);
        s_API->Events_Unsubscribe(EV_FO_RECIPE_RESPONSE, OnRecipeQueryResponse);
        s_API->Events_Unsubscribe(EV_FO_CONTEXT_WATCH, OnContextMenuWatch);
        s_API->Events_Unsubscribe(EV_FO_PERM_DISCARD, OnPermCheckDiscard);
        s_API->Events_Unsubscribe(EV_FO_PERM_DISCARD_ITEM, OnPermCheckDiscardItem);

        // H&S auto-cleans by signature, but remove explicitly for good measure
        RemoveContextMenu();

        s_API = nullptr;
    }

    // --- Tick (called every frame from render loop) ---

    void HoardBridge::Tick() {
        // Process pending retries
        ProcessRetries();

        // If a refresh is needed (after pong with data, or after data update), fire queries
        if (s_refreshNeeded && s_hoardDataAvailable && !s_permissionPending && !s_permissionDenied) {
            s_refreshNeeded = false;
            if (s_API) {
                s_API->Log(LOGL_INFO, "FlipOut", "H&S data refresh detected, caches cleared");
            }
        }
    }

    void HoardBridge::ScheduleRetry(RetryType type, const std::vector<uint32_t>& ids) {
        int key = static_cast<int>(type);
        int attempts = s_retryAttempts[key];

        if (attempts >= RETRY_MAX) {
            if (s_API) {
                s_API->Log(LOGL_WARNING, "FlipOut", "H&S query max retries reached, giving up");
            }
            s_permissionPending = false;
            s_retryAttempts.erase(key);
            return;
        }

        s_retryAttempts[key] = attempts + 1;

        std::lock_guard<std::mutex> lock(s_mutex);
        PendingRetry retry;
        retry.type = type;
        retry.ids = ids;
        retry.attempts = attempts + 1;
        retry.retry_at = std::chrono::steady_clock::now() + std::chrono::milliseconds(RETRY_DELAY_MS);
        s_retryQueue.push_back(std::move(retry));

        if (s_API) {
            std::string msg = "H&S permission pending, retry " + std::to_string(attempts + 1) + "/" + std::to_string(RETRY_MAX) + " in 2s";
            s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
        }
    }

    void HoardBridge::ResetRetryCount(RetryType type) {
        s_retryAttempts.erase(static_cast<int>(type));
    }

    void HoardBridge::ProcessRetries() {
        std::vector<PendingRetry> ready;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (s_retryQueue.empty()) return;

            auto now = std::chrono::steady_clock::now();
            auto it = s_retryQueue.begin();
            while (it != s_retryQueue.end()) {
                if (now >= it->retry_at) {
                    ready.push_back(std::move(*it));
                    it = s_retryQueue.erase(it);
                } else {
                    ++it;
                }
            }
        }

        for (auto& r : ready) {
            s_permissionPending = false; // Clear so the re-send isn't blocked

            switch (r.type) {
            case RetryType::ItemQuery:
                if (s_API) {
                    std::string msg = "Retrying H&S item query (attempt " + std::to_string(r.attempts + 1) + "/" + std::to_string(RETRY_MAX) + ")";
                    s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
                }
                for (uint32_t id : r.ids) {
                    s_queriedItems.erase(id);
                    QueryItem(id);
                }
                break;

            case RetryType::RecipeUnlock:
                if (s_API) {
                    std::string msg = "Retrying H&S recipe unlock query (attempt " + std::to_string(r.attempts + 1) + "/" + std::to_string(RETRY_MAX) + ")";
                    s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
                }
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    for (uint32_t id : r.ids) {
                        s_queriedRecipes.erase(id);
                    }
                }
                QueryRecipeUnlocks(r.ids);
                break;

            case RetryType::TransactionBuys:
            case RetryType::TransactionSells:
                if (s_API) {
                    s_API->Log(LOGL_INFO, "FlipOut", "Retrying H&S transaction query");
                }
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_txLoading = false;
                }
                FetchTransactions();
                break;
            }
        }
    }

    // --- Commands ---

    void HoardBridge::Ping() {
        if (!s_API) return;
        s_API->Events_Raise(EV_HOARD_PING, nullptr);
    }

    void HoardBridge::RequestRefresh() {
        if (!s_API) return;
        s_API->Events_Raise(EV_HOARD_REFRESH, nullptr);
    }

    void HoardBridge::QueryItem(uint32_t item_id) {
        if (!s_API || item_id == 0) return;
        if (!s_hoardDataAvailable) return;
        if (s_permissionPending || s_permissionDenied) return;
        if (s_queriedItems.count(item_id)) return;

        s_queriedItems.insert(item_id);

        // Stack-allocated request (matching Crafty Legend's working pattern)
        HoardQueryItemRequest req{};
        req.api_version = HOARD_API_VERSION;
        strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
        req.item_id = item_id;
        strncpy(req.response_event, EV_FO_ITEM_RESPONSE, sizeof(req.response_event));

        s_API->Events_Raise(EV_HOARD_QUERY_ITEM, &req);
    }

    void HoardBridge::QueryItems(const std::vector<uint32_t>& item_ids) {
        for (uint32_t id : item_ids) {
            QueryItem(id);
        }
    }

    void HoardBridge::QueryApi(const std::string& endpoint,
                                std::function<void(const std::string&)> callback) {
        if (!s_API) return;
        if (!s_hoardDataAvailable) return;
        if (s_permissionPending || s_permissionDenied) return;

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_apiCallback = callback;
        }

        HoardQueryApiRequest req{};
        req.api_version = HOARD_API_VERSION;
        strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
        strncpy(req.endpoint, endpoint.c_str(), sizeof(req.endpoint));
        strncpy(req.response_event, EV_FO_API_RESPONSE, sizeof(req.response_event));

        s_API->Events_Raise(EV_HOARD_QUERY_API, &req);
    }

    void HoardBridge::QueryRecipeUnlocks(const std::vector<uint32_t>& recipe_ids) {
        if (!s_API || recipe_ids.empty()) return;
        if (!s_hoardDataAvailable) return;
        if (s_permissionPending || s_permissionDenied) return;

        // Filter out already-queried recipe IDs
        std::vector<uint32_t> to_query;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            for (uint32_t id : recipe_ids) {
                if (s_queriedRecipes.find(id) == s_queriedRecipes.end()) {
                    to_query.push_back(id);
                    s_queriedRecipes.insert(id);
                }
            }
        }
        if (to_query.empty()) return;

        // Send in batches of 200 (H&S limit)
        const size_t BATCH = 200;
        int batches = 0;
        for (size_t i = 0; i < to_query.size(); i += BATCH) {
            HoardQueryRecipesRequest req{};
            req.api_version = HOARD_API_VERSION;
            strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
            strncpy(req.response_event, EV_FO_RECIPE_RESPONSE, sizeof(req.response_event));

            size_t end = std::min(i + BATCH, to_query.size());
            req.id_count = (uint32_t)(end - i);
            for (size_t j = i; j < end; j++) {
                req.ids[j - i] = to_query[j];
            }

            s_API->Events_Raise(EV_HOARD_QUERY_RECIPES, &req);
            batches++;
        }

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_recipeBatchesPending += batches;
            s_recipeQueryDone = false;
        }

        if (s_API) {
            std::string msg = "Querying H&S for " + std::to_string(to_query.size()) + " recipe unlocks";
            s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
        }
    }

    bool HoardBridge::IsRecipeUnlocked(uint32_t recipe_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_unlockedRecipes.count(recipe_id) > 0;
    }

    bool HoardBridge::IsRecipeQueried(uint32_t recipe_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_queriedRecipes.count(recipe_id) > 0;
    }

    size_t HoardBridge::GetUnlockedRecipeCount() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_unlockedRecipes.size();
    }

    bool HoardBridge::IsRecipeQueryDone() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_recipeQueryDone;
    }

    void HoardBridge::SearchInHoard(const std::string& item_name) {
        if (!s_API) return;
        s_API->Events_Raise(EV_HOARD_SEARCH, (void*)item_name.c_str());
    }

    // --- Context Menu ---

    void HoardBridge::RegisterContextMenu() {
        if (!s_API) return;

        HoardContextMenuRegister reg{};
        reg.api_version = HOARD_API_VERSION;
        reg.signature = FLIPOUT_SIGNATURE;
        strncpy(reg.id, "add_to_watchlist", sizeof(reg.id));
        strncpy(reg.requester, REQUESTER_NAME, sizeof(reg.requester));
        strncpy(reg.label, "Add to Flip Out Watchlist", sizeof(reg.label));
        strncpy(reg.callback_event, EV_FO_CONTEXT_WATCH, sizeof(reg.callback_event));
        reg.item_types = HOARD_MENU_ITEMS;

        s_API->Events_Raise(EV_HOARD_CONTEXT_MENU_REGISTER, &reg);
        s_API->Log(LOGL_INFO, "FlipOut", "Registered H&S context menu item");
    }

    void HoardBridge::RemoveContextMenu() {
        if (!s_API) return;

        HoardContextMenuRemove rem{};
        rem.api_version = HOARD_API_VERSION;
        strncpy(rem.requester, REQUESTER_NAME, sizeof(rem.requester));
        // id left empty = remove all entries from this requester

        s_API->Events_Raise(EV_HOARD_CONTEXT_MENU_REMOVE, &rem);
    }

    void HoardBridge::RequestPermissions() {
        if (!s_API) return;

        // Re-register context menu — triggers permission popup if not yet approved
        RegisterContextMenu();

        // Fire a dummy API query to trigger EV_HOARD_QUERY_API permission.
        // H&S batches permissions per requester (500ms), so all appear in one popup.
        {
            HoardQueryApiRequest req{};
            req.api_version = HOARD_API_VERSION;
            strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
            strncpy(req.endpoint, "/v2/commerce/prices", sizeof(req.endpoint));
            strncpy(req.response_event, EV_FO_PERM_DISCARD, sizeof(req.response_event));
            s_API->Events_Raise(EV_HOARD_QUERY_API, &req);
        }

        // Fire a dummy item query to trigger EV_HOARD_QUERY_ITEM (account data) permission.
        {
            HoardQueryItemRequest req{};
            req.api_version = HOARD_API_VERSION;
            strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
            req.item_id = 19976; // Mystic Coin (common TP item, harmless dummy)
            strncpy(req.response_event, EV_FO_PERM_DISCARD_ITEM, sizeof(req.response_event));
            s_API->Events_Raise(EV_HOARD_QUERY_ITEM, &req);
        }

        s_API->Log(LOGL_INFO, "FlipOut", "Requested H&S permissions (context menu + API proxy + item query)");
    }

    void HoardBridge::OnPermCheckDiscard(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryApiResponse* resp = static_cast<HoardQueryApiResponse*>(eventArgs);
        delete resp;
    }

    void HoardBridge::OnPermCheckDiscardItem(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryItemResponse* resp = static_cast<HoardQueryItemResponse*>(eventArgs);
        delete resp;
    }

    void HoardBridge::OnContextMenuWatch(void* eventArgs) {
        if (!eventArgs) return;
        HoardContextMenuCallback* cb = static_cast<HoardContextMenuCallback*>(eventArgs);

        if (cb->api_version != HOARD_API_VERSION) { delete cb; return; }

        uint32_t item_id = cb->item_id;
        std::string name(cb->name);

        // Add to watchlist
        PriceDB::AddToWatchlist(item_id);
        PriceDB::SaveWatchlist();

        if (s_API) {
            std::string msg = "Added to watchlist from H&S: " + name + " (" + std::to_string(item_id) + ")";
            s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
        }

        delete cb;
    }

    // --- Transaction fetching via H&S API proxy ---

    static std::vector<TPTransaction> ParseTransactionsJson(const std::string& jsonStr) {
        std::vector<TPTransaction> results;
        if (jsonStr.empty()) return results;
        try {
            json j = json::parse(jsonStr);
            if (j.is_array()) {
                for (const auto& t : j) {
                    TPTransaction tx;
                    tx.item_id = t.value("item_id", (uint32_t)0);
                    tx.price = t.value("price", 0);
                    tx.quantity = t.value("quantity", 0);
                    tx.created = t.value("created", "");
                    tx.purchased = t.value("purchased", "");
                    results.push_back(tx);
                }
            }
        } catch (...) {}
        return results;
    }

    void HoardBridge::FetchTransactions() {
        if (!s_API) return;
        if (!s_hoardDataAvailable) return;
        if (s_permissionPending || s_permissionDenied) return;

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            if (s_txLoading) return;
            s_txLoading = true;
            s_txBuysReceived = false;
            s_txSellsReceived = false;
            s_currentBuys.clear();
            s_currentSells.clear();
        }

        // Request current buys (stack-allocated)
        {
            HoardQueryApiRequest req{};
            req.api_version = HOARD_API_VERSION;
            strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
            strncpy(req.endpoint, "/v2/commerce/transactions/current/buys", sizeof(req.endpoint));
            strncpy(req.response_event, EV_FO_TX_BUYS_RESPONSE, sizeof(req.response_event));
            s_API->Events_Raise(EV_HOARD_QUERY_API, &req);
        }

        // Request current sells (stack-allocated)
        {
            HoardQueryApiRequest req{};
            req.api_version = HOARD_API_VERSION;
            strncpy(req.requester, REQUESTER_NAME, sizeof(req.requester));
            strncpy(req.endpoint, "/v2/commerce/transactions/current/sells", sizeof(req.endpoint));
            strncpy(req.response_event, EV_FO_TX_SELLS_RESPONSE, sizeof(req.response_event));
            s_API->Events_Raise(EV_HOARD_QUERY_API, &req);
        }
    }

    // --- State accessors ---

    HoardStatus HoardBridge::GetStatus() {
        if (s_hoardDataAvailable) return HoardStatus::Connected;
        if (s_hoardDetected) return HoardStatus::NoData;
        return HoardStatus::Unknown;
    }

    bool HoardBridge::IsConnected() {
        return s_hoardDetected;
    }

    bool HoardBridge::IsDataAvailable() {
        return s_hoardDataAvailable;
    }

    int64_t HoardBridge::GetLastUpdated() {
        return s_lastUpdated;
    }

    bool HoardBridge::IsPermissionPending() {
        return s_permissionPending;
    }

    bool HoardBridge::IsPermissionDenied() {
        return s_permissionDenied;
    }

    const OwnedItem* HoardBridge::GetOwnedItem(uint32_t item_id) {
        std::lock_guard<std::mutex> lock(s_mutex);
        auto it = s_ownedItems.find(item_id);
        if (it != s_ownedItems.end()) return &it->second;
        return nullptr;
    }

    std::unordered_map<uint32_t, OwnedItem> HoardBridge::GetAllOwnedItems() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_ownedItems;
    }

    size_t HoardBridge::GetOwnedItemCount() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_ownedItems.size();
    }

    const std::vector<TPTransaction>& HoardBridge::GetCurrentBuys() {
        return s_currentBuys;
    }

    const std::vector<TPTransaction>& HoardBridge::GetCurrentSells() {
        return s_currentSells;
    }

    bool HoardBridge::IsTransactionsLoading() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_txLoading;
    }

    bool HoardBridge::IsTransactionsLoaded() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_txLoaded;
    }

    // --- Event Handlers: H&S Broadcasts (do NOT delete payloads) ---

    void HoardBridge::OnPong(void* eventArgs) {
        // H&S detected
        s_hoardDetected = true;

        if (!eventArgs) return; // Backward compat with older H&S
        HoardPongPayload* pong = static_cast<HoardPongPayload*>(eventArgs);
        if (pong->api_version != HOARD_API_VERSION) return;

        s_lastUpdated = pong->last_updated;
        s_refreshAvailableAt = pong->refresh_available_at;
        if (pong->has_data) {
            s_hoardDataAvailable = true;
            s_refreshNeeded = true;
        }

        if (s_API) {
            std::string msg = std::string("Connected to Hoard & Seek (data=") + (pong->has_data ? "yes" : "no") + ")";
            s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
        }
        // Do NOT delete — payload is stack-allocated by H&S
    }

    void HoardBridge::OnDataUpdated(void* eventArgs) {
        if (eventArgs) {
            HoardDataReadyPayload* payload = static_cast<HoardDataReadyPayload*>(eventArgs);
            if (payload->api_version == HOARD_API_VERSION) {
                s_lastUpdated = payload->last_updated;
                s_refreshAvailableAt = payload->refresh_available_at;
            }
        }
        s_hoardDetected = true;
        s_hoardDataAvailable = true;
        s_refreshNeeded = true;
        s_hoardFetching = false;
        s_fetchProgressMsg.clear();

        // Clear old cached data so it gets re-queried
        s_ownedItems.clear();
        s_queriedItems.clear();
        s_unlockedRecipes.clear();
        s_queriedRecipes.clear();
        s_recipeQueryDone = false;
        s_recipeBatchesPending = 0;
        s_retryQueue.clear();
        s_retryAttempts.clear();
        s_permissionPending = false;
        s_permissionDenied = false;

        if (s_API) {
            s_API->Log(LOGL_INFO, "FlipOut", "H&S data updated, caches cleared");
        }
        // Do NOT delete — payload is stack-allocated by H&S
    }

    void HoardBridge::OnFetchProgress(void* eventArgs) {
        if (!eventArgs) return;
        HoardFetchProgressPayload* payload = static_cast<HoardFetchProgressPayload*>(eventArgs);
        if (payload->api_version != HOARD_API_VERSION) return;
        s_fetchProgressMsg = payload->message;
        s_hoardFetching = true;
        // Do NOT delete — payload is stack-allocated by H&S
    }

    void HoardBridge::OnFetchError(void* eventArgs) {
        if (!eventArgs) return;
        HoardFetchErrorPayload* payload = static_cast<HoardFetchErrorPayload*>(eventArgs);
        if (payload->api_version != HOARD_API_VERSION) return;
        s_hoardFetching = false;

        if (s_API) {
            std::string msg = std::string("H&S fetch error: ") + payload->message;
            s_API->Log(LOGL_WARNING, "FlipOut", msg.c_str());
        }
        // Do NOT delete — payload is stack-allocated by H&S
    }

    // --- Event Handlers: Query Responses (MUST delete payloads) ---

    void HoardBridge::OnItemQueryResponse(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryItemResponse* resp = static_cast<HoardQueryItemResponse*>(eventArgs);

        if (resp->api_version != HOARD_API_VERSION) { delete resp; return; }

        if (resp->status == HOARD_STATUS_PENDING) {
            s_permissionPending = true;
            uint32_t id = resp->item_id;
            s_queriedItems.erase(id);
            ScheduleRetry(RetryType::ItemQuery, {id});
            delete resp;
            return;
        }
        if (resp->status == HOARD_STATUS_DENIED) {
            s_permissionDenied = true;
            ResetRetryCount(RetryType::ItemQuery);
            delete resp;
            return;
        }

        s_permissionPending = false;
        ResetRetryCount(RetryType::ItemQuery);

        OwnedItem item;
        item.item_id = resp->item_id;
        item.name = std::string(resp->name);
        item.rarity = std::string(resp->rarity);
        item.type = std::string(resp->type);
        item.total_count = resp->total_count;

        for (uint32_t i = 0; i < resp->location_count && i < 32; i++) {
            std::string loc = std::string(resp->locations[i].location);
            if (resp->locations[i].sublocation[0] != '\0') {
                loc += " > " + std::string(resp->locations[i].sublocation);
            }
            item.locations.push_back({loc, resp->locations[i].count});
        }

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_ownedItems[item.item_id] = item;
        }

        delete resp;
    }

    void HoardBridge::OnApiQueryResponse(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryApiResponse* resp = static_cast<HoardQueryApiResponse*>(eventArgs);

        if (resp->api_version != HOARD_API_VERSION) { delete resp; return; }

        if (resp->status == HOARD_STATUS_PENDING) {
            s_permissionPending = true;
            // Generic API queries are not retryable (callback is one-shot)
            if (s_API) {
                s_API->Log(LOGL_INFO, "FlipOut", "H&S API query pending permission approval");
            }
            delete resp;
            return;
        }
        if (resp->status == HOARD_STATUS_DENIED) {
            s_permissionDenied = true;
            delete resp;
            return;
        }

        s_permissionPending = false;

        std::string jsonStr(resp->json, std::min((uint32_t)sizeof(resp->json) - 1, resp->json_length));

        std::function<void(const std::string&)> cb;
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            cb = s_apiCallback;
            s_apiCallback = nullptr;
        }
        if (cb) cb(jsonStr);

        delete resp;
    }

    void HoardBridge::OnTxBuysResponse(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryApiResponse* resp = static_cast<HoardQueryApiResponse*>(eventArgs);

        if (resp->api_version != HOARD_API_VERSION) { delete resp; return; }

        if (resp->status != HOARD_STATUS_OK) {
            if (resp->status == HOARD_STATUS_PENDING) {
                s_permissionPending = true;
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_txLoading = false;
                }
                ScheduleRetry(RetryType::TransactionBuys, {});
            } else if (resp->status == HOARD_STATUS_DENIED) {
                s_permissionDenied = true;
                std::lock_guard<std::mutex> lock(s_mutex);
                s_txLoading = false;
            }
            delete resp;
            return;
        }

        s_permissionPending = false;
        ResetRetryCount(RetryType::TransactionBuys);

        std::string jsonStr(resp->json, std::min((uint32_t)sizeof(resp->json) - 1, resp->json_length));
        auto txs = ParseTransactionsJson(jsonStr);

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_currentBuys = txs;
            s_txBuysReceived = true;
            if (s_txSellsReceived) {
                s_txLoading = false;
                s_txLoaded = true;
            }
        }

        // Fetch item info for transaction items
        std::vector<uint32_t> ids;
        for (const auto& t : txs) ids.push_back(t.item_id);
        if (!ids.empty()) TPAPI::FetchItemInfo(ids);

        delete resp;
    }

    void HoardBridge::OnTxSellsResponse(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryApiResponse* resp = static_cast<HoardQueryApiResponse*>(eventArgs);

        if (resp->api_version != HOARD_API_VERSION) { delete resp; return; }

        if (resp->status != HOARD_STATUS_OK) {
            if (resp->status == HOARD_STATUS_PENDING) {
                s_permissionPending = true;
                {
                    std::lock_guard<std::mutex> lock(s_mutex);
                    s_txLoading = false;
                }
                ScheduleRetry(RetryType::TransactionSells, {});
            } else if (resp->status == HOARD_STATUS_DENIED) {
                s_permissionDenied = true;
                std::lock_guard<std::mutex> lock(s_mutex);
                s_txLoading = false;
            }
            delete resp;
            return;
        }

        s_permissionPending = false;
        ResetRetryCount(RetryType::TransactionSells);

        std::string jsonStr(resp->json, std::min((uint32_t)sizeof(resp->json) - 1, resp->json_length));
        auto txs = ParseTransactionsJson(jsonStr);

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_currentSells = txs;
            s_txSellsReceived = true;
            if (s_txBuysReceived) {
                s_txLoading = false;
                s_txLoaded = true;
            }
        }

        // Fetch item info for transaction items
        std::vector<uint32_t> ids;
        for (const auto& t : txs) ids.push_back(t.item_id);
        if (!ids.empty()) TPAPI::FetchItemInfo(ids);

        delete resp;
    }

    void HoardBridge::OnRecipeQueryResponse(void* eventArgs) {
        if (!eventArgs) return;
        HoardQueryRecipesResponse* resp = static_cast<HoardQueryRecipesResponse*>(eventArgs);

        if (resp->api_version != HOARD_API_VERSION) { delete resp; return; }

        if (resp->status == HOARD_STATUS_PENDING) {
            s_permissionPending = true;
            // Collect all currently queried recipe IDs for retry
            std::vector<uint32_t> retryIds;
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                retryIds.assign(s_queriedRecipes.begin(), s_queriedRecipes.end());
                s_queriedRecipes.clear();
                s_recipeBatchesPending = 0;
            }
            ScheduleRetry(RetryType::RecipeUnlock, retryIds);
            delete resp;
            return;
        }
        if (resp->status == HOARD_STATUS_DENIED) {
            s_permissionDenied = true;
            {
                std::lock_guard<std::mutex> lock(s_mutex);
                s_recipeBatchesPending = std::max(0, s_recipeBatchesPending - 1);
                if (s_recipeBatchesPending <= 0) s_recipeQueryDone = true;
            }
            delete resp;
            return;
        }

        s_permissionPending = false;
        ResetRetryCount(RetryType::RecipeUnlock);

        {
            std::lock_guard<std::mutex> lock(s_mutex);
            for (uint32_t i = 0; i < resp->entry_count && i < 200; i++) {
                if (resp->entries[i].unlocked) {
                    s_unlockedRecipes.insert(resp->entries[i].id);
                }
            }
            s_recipeBatchesPending = std::max(0, s_recipeBatchesPending - 1);
            if (s_recipeBatchesPending <= 0) {
                s_recipeQueryDone = true;
            }
        }

        if (s_API && s_recipeQueryDone) {
            std::string msg = "Recipe unlock query complete: " + std::to_string(s_unlockedRecipes.size()) + " unlocked";
            s_API->Log(LOGL_INFO, "FlipOut", msg.c_str());
        }

        delete resp;
    }

}
