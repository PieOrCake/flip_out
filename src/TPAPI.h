#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace FlipOut {

    // Summary price info from /v2/commerce/prices
    struct TPPrice {
        uint32_t item_id = 0;
        int buy_price = 0;      // highest buy order (copper)
        int buy_quantity = 0;
        int sell_price = 0;     // lowest sell listing (copper)
        int sell_quantity = 0;
    };

    // Individual listing from /v2/commerce/listings
    struct TPListing {
        int listings = 0;    // number of individual listings at this price
        int unit_price = 0;  // copper
        int quantity = 0;    // total quantity at this price
    };

    struct TPListingDetail {
        uint32_t item_id = 0;
        std::vector<TPListing> buys;
        std::vector<TPListing> sells;
    };

    // Item metadata from /v2/items (public endpoint, no auth needed)
    struct ItemInfo {
        uint32_t id = 0;
        std::string name;
        std::string icon_url;
        std::string rarity;
        std::string type;
        int vendor_value = 0;  // vendor sell price in copper
        int level = 0;
    };

    // Recipe ingredient
    struct RecipeIngredient {
        uint32_t item_id = 0;
        int count = 0;
    };

    // Crafting recipe from /v2/recipes
    struct Recipe {
        uint32_t id = 0;
        uint32_t output_item_id = 0;
        int output_item_count = 1;
        std::string type;               // e.g. "Refinement", "Weapon", "Armor"
        int min_rating = 0;
        std::vector<std::string> disciplines;
        std::vector<RecipeIngredient> ingredients;
        bool auto_learned = false;
    };

    // Player transaction from /v2/commerce/transactions
    // (fetched via Hoard & Seek API proxy)
    struct TPTransaction {
        uint32_t item_id = 0;
        int price = 0;
        int quantity = 0;
        std::string created;   // ISO timestamp
        std::string purchased; // ISO timestamp (empty if pending)
    };

    enum class FetchStatus {
        Idle,
        InProgress,
        Success,
        Error
    };

    class TPAPI {
    public:
        // Data directory
        static std::string GetDataDirectory();
        static bool EnsureDataDirectory();

        // Fetch TP prices for a batch of item IDs (public, no auth)
        static std::vector<TPPrice> FetchPrices(const std::vector<uint32_t>& item_ids);

        // Fetch detailed listings for a single item (public, no auth)
        static TPListingDetail FetchListings(uint32_t item_id);

        // Fetch item metadata for a batch of IDs (public, no auth)
        static std::vector<ItemInfo> FetchItemInfo(const std::vector<uint32_t>& item_ids);

        // Fetch a single item's info (uses cache)
        static const ItemInfo* GetItemInfo(uint32_t item_id);

        // Search tradeable items by name (from local cache)
        static std::vector<ItemInfo> SearchItems(const std::string& query, int max_results = 100);

        // Fetch all tradeable item IDs from /v2/commerce/prices (public)
        static std::vector<uint32_t> FetchAllTradeableIds();

        // Bulk fetch all TP prices (async, cancellable)
        static void FetchAllPricesAsync();
        static void CancelScan();
        static bool IsScanCancelled();
        static FetchStatus GetBulkFetchStatus();
        static const std::string& GetBulkFetchMessage();
        static float GetBulkFetchProgress();

        // Inject prices from an external source (e.g. after manual update)
        static void UpdatePrices(const std::unordered_map<uint32_t, TPPrice>& prices);

        // Cache management
        static bool LoadItemCache();
        static bool SaveItemCache();

        // Access the full price map (thread-safe copy)
        static std::unordered_map<uint32_t, TPPrice> GetAllPrices();

        // Single-item price lookup (thread-safe, no map copy)
        static bool GetPrice(uint32_t item_id, TPPrice& out);

        // Lightweight check: are there any prices loaded?
        static bool HasPrices();

        // --- Recipe API ---

        // Fetch all recipe IDs from /v2/recipes
        static std::vector<uint32_t> FetchAllRecipeIds();

        // Fetch recipe details for a batch of IDs
        static std::vector<Recipe> FetchRecipes(const std::vector<uint32_t>& recipe_ids);

        // Fetch all recipes (async, updates progress)
        static void FetchAllRecipesAsync();
        static FetchStatus GetRecipeFetchStatus();
        static const std::string& GetRecipeFetchMessage();

        // Recipe cache
        static bool LoadRecipeCache();
        static bool SaveRecipeCache();
        static const std::unordered_map<uint32_t, Recipe>& GetRecipesByOutput();
        static size_t GetRecipeCount();

    private:
        static FetchStatus s_bulk_fetch_status;
        static std::string s_bulk_fetch_message;
        static float s_bulk_fetch_progress;
        static std::atomic<bool> s_scan_cancelled;

        // item_id -> current price snapshot
        static std::unordered_map<uint32_t, TPPrice> s_prices;

        // item_id -> cached item info
        static std::unordered_map<uint32_t, ItemInfo> s_item_cache;

        // output_item_id -> recipe (one recipe per output for simplicity)
        static std::unordered_map<uint32_t, Recipe> s_recipes;
        static FetchStatus s_recipe_fetch_status;
        static std::string s_recipe_fetch_message;

        static std::mutex s_mutex;
    };

}
