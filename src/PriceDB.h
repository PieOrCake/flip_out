#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <ctime>

namespace FlipOut {

    // A single price snapshot for an item at a point in time
    struct PriceSnapshot {
        time_t timestamp = 0;
        int buy_price = 0;      // copper
        int sell_price = 0;     // copper
        int buy_quantity = 0;
        int sell_quantity = 0;
    };

    // Watchlist entry with user-configurable alerts
    struct WatchlistEntry {
        uint32_t item_id = 0;
        bool alert_on_margin = false;   // alert if margin exceeds threshold
        float margin_threshold = 15.0f; // percentage
        bool alert_on_price_drop = false;
        int price_drop_target = 0;      // copper - alert if sell drops below this
    };

    class PriceDB {
    public:
        // Record a price snapshot for an item
        static void RecordPrice(uint32_t item_id, int buy_price, int sell_price,
                                int buy_quantity, int sell_quantity);

        // Record prices from a bulk fetch
        static void RecordBulkPrices(const std::unordered_map<uint32_t, PriceSnapshot>& prices);

        // Get price history for an item (oldest first)
        static std::vector<PriceSnapshot> GetHistory(uint32_t item_id, int max_entries = 0);

        // Get latest snapshot for an item
        static PriceSnapshot GetLatest(uint32_t item_id);

        // Get the number of items tracked
        static size_t GetTrackedItemCount();

        // Trim old data (keep last N entries per item, or entries within last N days)
        static void Trim(int max_entries_per_item = 1000, int max_age_days = 30);

        // Watchlist management
        static void AddToWatchlist(uint32_t item_id);
        static void RemoveFromWatchlist(uint32_t item_id);
        static bool IsOnWatchlist(uint32_t item_id);
        static std::vector<WatchlistEntry>& GetWatchlist();
        static WatchlistEntry* GetWatchlistEntry(uint32_t item_id);

        // Persistence
        static bool Load();
        static bool Save();
        static bool LoadWatchlist();
        static bool SaveWatchlist();

        // Seed data (community price history)
        static bool ExportSeed(const std::string& path, int max_entries_per_item = 30);
        static bool ImportSeed(const std::string& json_data);

        // Data directory (delegates to TPAPI)
        static std::string GetDataDirectory();

    private:
        // item_id -> list of price snapshots (chronological order)
        static std::unordered_map<uint32_t, std::vector<PriceSnapshot>> s_history;

        // User watchlist
        static std::vector<WatchlistEntry> s_watchlist;

        static std::mutex s_mutex;
    };

}
