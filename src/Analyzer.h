#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace FlipOut {

    // TP tax constants
    // 5% listing fee + 10% exchange fee = 15% total
    static const float TP_LISTING_FEE = 0.05f;
    static const float TP_EXCHANGE_FEE = 0.10f;
    static const float TP_TOTAL_TAX = 0.15f;

    // A flip opportunity
    struct FlipOpportunity {
        uint32_t item_id = 0;
        std::string name;
        std::string rarity;
        int buy_price = 0;       // place buy order at this (copper)
        int sell_price = 0;      // list sell at this (copper)
        int profit_per_unit = 0; // after tax (copper)
        float margin_pct = 0.0f; // profit margin percentage
        int buy_quantity = 0;    // supply on buy side
        int sell_quantity = 0;   // supply on sell side
        float volume_score = 0.0f; // combined volume indicator
        float roi = 0.0f;       // return on investment percentage
    };

    // Trend direction
    enum class Trend {
        Unknown,
        Rising,
        Falling,
        Stable
    };

    // Trend info for an item
    struct TrendInfo {
        uint32_t item_id = 0;
        Trend buy_trend = Trend::Unknown;
        Trend sell_trend = Trend::Unknown;
        float buy_velocity = 0.0f;  // copper per hour (positive = rising)
        float sell_velocity = 0.0f;
        int buy_change_pct = 0;     // percent change over analysis period
        int sell_change_pct = 0;
        int data_points = 0;
    };

    // Sort criteria for flip results
    enum class FlipSort {
        ProfitPerUnit,
        MarginPercent,
        ROI,
        VolumeScore,
        TotalProfit   // profit * min(buy_qty, sell_qty) estimate
    };

    // Filter for flip scanning
    struct FlipFilter {
        int min_profit = 100;         // minimum copper profit per unit
        float min_margin = 5.0f;      // minimum margin percentage
        int min_volume = 100;         // minimum combined buy+sell quantity
        int max_buy_price = 0;        // 0 = no max (copper)
        int min_buy_price = 0;        // 0 = no min
        float max_sell_buy_ratio = 10.0f; // max sell/buy ratio (outlier filter)
        FlipSort sort_by = FlipSort::ProfitPerUnit;
        int max_results = 50;
    };

    // A market mover: item with significant price/volume spike vs historical average
    struct MarketMover {
        uint32_t item_id = 0;
        std::string name;
        std::string rarity;
        int current_sell = 0;          // current TP sell price (copper)
        int avg_sell = 0;              // historical average sell price (copper)
        float price_change_pct = 0;    // % change vs historical average
        int current_buy_qty = 0;       // current buy order volume
        int avg_buy_qty = 0;           // historical average buy volume
        float volume_change_pct = 0;   // % change in buy volume vs avg
        float spike_score = 0;         // composite score (price change * volume factor)
    };

    // A sell opportunity for an owned item with inflated price
    struct SellOpportunity {
        uint32_t item_id = 0;
        std::string name;
        std::string rarity;
        int32_t owned_count = 0;      // how many the user owns
        int current_sell = 0;         // current TP sell price (copper)
        int avg_sell = 0;             // historical average sell price (copper)
        int profit_vs_avg = 0;        // current - avg after tax (copper)
        float price_increase_pct = 0; // % above historical average
        int potential_profit = 0;     // profit_vs_avg * owned_count
    };

    // A crafting profit opportunity
    struct CraftingProfit {
        uint32_t item_id = 0;           // output item ID
        uint32_t recipe_id = 0;         // GW2 recipe ID (for H&S unlock query)
        std::string name;
        std::string rarity;
        std::string recipe_type;        // e.g. "Weapon", "Armor", "Refinement"
        int output_count = 1;           // how many the recipe produces
        int ingredient_cost = 0;        // total cost to buy all ingredients (copper)
        int sell_price = 0;             // TP sell price of output (copper)
        int sell_revenue = 0;           // sell_price * output_count - tax
        int profit = 0;                 // revenue - ingredient_cost (copper)
        float roi = 0.0f;              // profit / ingredient_cost * 100
        int sell_quantity = 0;          // TP sell volume (demand indicator)
        std::vector<std::pair<uint32_t, int>> ingredients; // item_id, count
    };

    // Filter for crafting profit scanning
    struct CraftingFilter {
        int min_profit = 100;           // minimum copper profit per craft
        float min_roi = 5.0f;           // minimum ROI percentage
        int min_sell_volume = 10;       // minimum sell volume on TP
        int max_results = 50;
        int mode = 1;                   // 0=Fastest (instant buy + instant sell), 1=Balanced (instant buy + sell listing), 2=Patient (buy orders + sell listing)
    };

    class Analyzer {
    public:
        // Calculate profit after TP tax
        static int CalcProfit(int buy_price, int sell_price);

        // Calculate margin percentage
        static float CalcMargin(int buy_price, int sell_price);

        // Calculate the listing fee for a sell price
        static int CalcListingFee(int sell_price);

        // Calculate the exchange fee for a sell price
        static int CalcExchangeFee(int sell_price);

        // Calculate total tax on a sale
        static int CalcTotalTax(int sell_price);

        // Calculate ROI (profit / buy_price * 100)
        static float CalcROI(int buy_price, int sell_price);

        // Check if a price pair is an outlier (manipulated sell order)
        static bool IsOutlierPrice(int buy_price, int sell_price, float max_ratio = 10.0f);

        // Find flip opportunities from current price data
        static std::vector<FlipOpportunity> FindFlips(const FlipFilter& filter = FlipFilter{});

        // Find sell opportunities: owned items with temporarily inflated prices
        static std::vector<SellOpportunity> FindSellOpportunities(
            float min_increase_pct = 20.0f, int min_history_points = 3);

        // Find market movers: items with biggest price/volume spikes vs historical averages
        static std::vector<MarketMover> FindMarketMovers(
            float min_price_change_pct = 20.0f, int min_history_points = 3,
            int min_avg_sell_copper = 100, int max_results = 50);

        // Get trend info for an item based on price history
        static TrendInfo GetTrend(uint32_t item_id);

        // Get trend info for multiple items
        static std::vector<TrendInfo> GetTrends(const std::vector<uint32_t>& item_ids);

        // Format copper amount as gold/silver/copper string
        static std::string FormatCoins(int copper);

        // Format with just gold if large
        static std::string FormatCoinsShort(int copper);

        // Find crafting profit opportunities from recipe data and current prices
        static std::vector<CraftingProfit> FindCraftingProfits(
            const CraftingFilter& filter = CraftingFilter{});

        // Get trend arrow character
        static const char* TrendArrow(Trend t);

        // Get trend color for ImGui
        static void TrendColor(Trend t, float& r, float& g, float& b);
    };

}
