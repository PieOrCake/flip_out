#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>
#include <functional>
#include <chrono>
#include "nexus/Nexus.h"
#include "HoardAndSeekAPI.h"
#include "TPAPI.h"

namespace FlipOut {

    // Connection status with Hoard & Seek
    enum class HoardStatus {
        Unknown,        // Haven't pinged yet
        Waiting,        // Ping sent, waiting for pong
        Connected,      // H&S responded, data available
        NoData,         // H&S responded but has no account data loaded
        Disconnected    // No response (H&S not loaded)
    };

    // Owned item info retrieved from H&S
    struct OwnedItem {
        uint32_t item_id = 0;
        std::string name;
        std::string rarity;
        std::string type;
        int32_t total_count = 0;
        std::vector<std::pair<std::string, int32_t>> locations; // location name -> count
    };

    class HoardBridge {
    public:
        // Initialize: register event subscriptions
        static void Initialize(AddonAPI_t* api);

        // Shutdown: deregister events
        static void Shutdown();

        // Called every frame from render loop — drives refresh queries
        static void Tick();

        // Ping H&S to check connectivity
        static void Ping();

        // Request H&S to refresh its account data
        static void RequestRefresh();

        // Query owned count/locations for a specific item (guarded by data availability)
        static void QueryItem(uint32_t item_id);

        // Query multiple items (guarded, deduplicated)
        static void QueryItems(const std::vector<uint32_t>& item_ids);

        // Request authenticated API call via H&S proxy
        static void QueryApi(const std::string& endpoint,
                             std::function<void(const std::string&)> callback);

        // Open H&S search window for an item name
        static void SearchInHoard(const std::string& item_name);

        // Context menu: register "Add to Watchlist" in H&S right-click menu
        static void RegisterContextMenu();

        // Context menu: remove our menu entries from H&S
        static void RemoveContextMenu();

        // Trigger H&S permission popup for all events we use (context menu + API proxy)
        static void RequestPermissions();

        // --- State accessors ---
        static HoardStatus GetStatus();
        static bool IsConnected();
        static bool IsDataAvailable();
        static int64_t GetLastUpdated();
        static bool IsPermissionPending();
        static bool IsPermissionDenied();

        // Get owned item data (populated by QueryItem responses)
        static const OwnedItem* GetOwnedItem(uint32_t item_id);
        static std::unordered_map<uint32_t, OwnedItem> GetAllOwnedItems();
        static size_t GetOwnedItemCount();

        // Transaction data (fetched via H&S API proxy)
        static const std::vector<TPTransaction>& GetCurrentBuys();
        static const std::vector<TPTransaction>& GetCurrentSells();
        static bool IsTransactionsLoading();
        static bool IsTransactionsLoaded();
        static void FetchTransactions();

    private:
        // Event handlers (H&S broadcasts — do NOT delete payloads)
        static void OnPong(void* eventArgs);
        static void OnDataUpdated(void* eventArgs);
        static void OnFetchProgress(void* eventArgs);
        static void OnFetchError(void* eventArgs);

        // Event handlers (query responses — MUST delete payloads)
        static void OnItemQueryResponse(void* eventArgs);
        static void OnApiQueryResponse(void* eventArgs);
        static void OnTxBuysResponse(void* eventArgs);
        static void OnTxSellsResponse(void* eventArgs);

        // Context menu callback (MUST delete payload)
        static void OnContextMenuWatch(void* eventArgs);

        // Discard handlers for dummy permission-check queries
        static void OnPermCheckDiscard(void* eventArgs);
        static void OnPermCheckDiscardItem(void* eventArgs);

        static AddonAPI_t* s_API;

        // H&S connection state
        static bool s_hoardDetected;
        static bool s_hoardDataAvailable;
        static bool s_refreshNeeded;
        static int64_t s_lastUpdated;
        static int64_t s_refreshAvailableAt;

        // Fetch progress
        static bool s_hoardFetching;
        static std::string s_fetchProgressMsg;
        static float s_fetchProgress;

        // Permission state
        static bool s_permissionPending;
        static bool s_permissionDenied;
        static std::chrono::steady_clock::time_point s_permissionRetryTime;

        // Query deduplication
        static std::unordered_set<uint32_t> s_queriedItems;

        // Owned items cache
        static std::unordered_map<uint32_t, OwnedItem> s_ownedItems;

        // Transaction data
        static std::vector<TPTransaction> s_currentBuys;
        static std::vector<TPTransaction> s_currentSells;
        static bool s_txLoading;
        static bool s_txLoaded;
        static bool s_txBuysReceived;
        static bool s_txSellsReceived;

        // API proxy callback
        static std::function<void(const std::string&)> s_apiCallback;

        static std::mutex s_mutex;
    };

}
