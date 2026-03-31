#include "Analyzer.h"
#include "TPAPI.h"
#include "PriceDB.h"
#include "HoardBridge.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace FlipOut {

    int Analyzer::CalcListingFee(int sell_price) {
        int fee = (int)(sell_price * TP_LISTING_FEE);
        return (fee < 1) ? 1 : fee;
    }

    int Analyzer::CalcExchangeFee(int sell_price) {
        int fee = (int)(sell_price * TP_EXCHANGE_FEE);
        return (fee < 1) ? 1 : fee;
    }

    int Analyzer::CalcTotalTax(int sell_price) {
        return CalcListingFee(sell_price) + CalcExchangeFee(sell_price);
    }

    int Analyzer::CalcProfit(int buy_price, int sell_price) {
        if (buy_price <= 0 || sell_price <= 0) return 0;
        return sell_price - CalcTotalTax(sell_price) - buy_price;
    }

    float Analyzer::CalcMargin(int buy_price, int sell_price) {
        if (sell_price <= 0) return 0.0f;
        int profit = CalcProfit(buy_price, sell_price);
        return (float)profit / (float)sell_price * 100.0f;
    }

    float Analyzer::CalcROI(int buy_price, int sell_price) {
        if (buy_price <= 0) return 0.0f;
        int profit = CalcProfit(buy_price, sell_price);
        return (float)profit / (float)buy_price * 100.0f;
    }

    bool Analyzer::IsOutlierPrice(int buy_price, int sell_price, float max_ratio) {
        if (buy_price <= 0 || sell_price <= 0) return true;
        float ratio = (float)sell_price / (float)buy_price;
        return ratio > max_ratio;
    }

    std::vector<FlipOpportunity> Analyzer::FindFlips(const FlipFilter& filter) {
        auto prices = TPAPI::GetAllPrices();
        std::vector<FlipOpportunity> results;

        for (const auto& [id, price] : prices) {
            if (price.buy_price <= 0 || price.sell_price <= 0) continue;
            if (price.buy_price >= price.sell_price) continue;

            // Outlier filter: skip items with absurd sell/buy ratios
            if (IsOutlierPrice(price.buy_price, price.sell_price, filter.max_sell_buy_ratio)) continue;

            // Apply price range filters
            if (filter.max_buy_price > 0 && price.buy_price > filter.max_buy_price) continue;
            if (filter.min_buy_price > 0 && price.buy_price < filter.min_buy_price) continue;

            int profit = CalcProfit(price.buy_price, price.sell_price);
            if (profit < filter.min_profit) continue;

            float margin = CalcMargin(price.buy_price, price.sell_price);
            if (margin < filter.min_margin) continue;

            int combined_volume = price.buy_quantity + price.sell_quantity;
            if (combined_volume < filter.min_volume) continue;

            FlipOpportunity opp;
            opp.item_id = id;
            opp.buy_price = price.buy_price;
            opp.sell_price = price.sell_price;
            opp.profit_per_unit = profit;
            opp.margin_pct = margin;
            opp.buy_quantity = price.buy_quantity;
            opp.sell_quantity = price.sell_quantity;
            opp.roi = CalcROI(price.buy_price, price.sell_price);

            // Volume score: geometric mean of buy and sell quantities, clamped
            float vol = std::sqrt((float)price.buy_quantity * (float)price.sell_quantity);
            opp.volume_score = std::min(vol, 100000.0f);

            // Fill item name from cache
            const auto* info = TPAPI::GetItemInfo(id);
            if (info) {
                opp.name = info->name;
                opp.rarity = info->rarity;
            } else {
                opp.name = "Item #" + std::to_string(id);
            }

            results.push_back(opp);
        }

        // Sort based on criteria
        switch (filter.sort_by) {
            case FlipSort::ProfitPerUnit:
                std::sort(results.begin(), results.end(),
                    [](const FlipOpportunity& a, const FlipOpportunity& b) {
                        return a.profit_per_unit > b.profit_per_unit;
                    });
                break;
            case FlipSort::MarginPercent:
                std::sort(results.begin(), results.end(),
                    [](const FlipOpportunity& a, const FlipOpportunity& b) {
                        return a.margin_pct > b.margin_pct;
                    });
                break;
            case FlipSort::ROI:
                std::sort(results.begin(), results.end(),
                    [](const FlipOpportunity& a, const FlipOpportunity& b) {
                        return a.roi > b.roi;
                    });
                break;
            case FlipSort::VolumeScore:
                std::sort(results.begin(), results.end(),
                    [](const FlipOpportunity& a, const FlipOpportunity& b) {
                        return a.volume_score > b.volume_score;
                    });
                break;
            case FlipSort::TotalProfit:
                std::sort(results.begin(), results.end(),
                    [](const FlipOpportunity& a, const FlipOpportunity& b) {
                        int a_total = a.profit_per_unit * std::min(a.buy_quantity, a.sell_quantity);
                        int b_total = b.profit_per_unit * std::min(b.buy_quantity, b.sell_quantity);
                        return a_total > b_total;
                    });
                break;
        }

        // Limit results
        if (filter.max_results > 0 && (int)results.size() > filter.max_results) {
            results.resize(filter.max_results);
        }

        return results;
    }

    TrendInfo Analyzer::GetTrend(uint32_t item_id) {
        TrendInfo info;
        info.item_id = item_id;

        auto history = PriceDB::GetHistory(item_id, 100);
        info.data_points = (int)history.size();
        if (history.size() < 2) return info;

        // Use simple linear regression on the last N points
        int n = (int)history.size();
        double sum_t = 0, sum_bp = 0, sum_sp = 0;
        double sum_t2 = 0, sum_t_bp = 0, sum_t_sp = 0;

        time_t t0 = history.front().timestamp;
        for (int i = 0; i < n; i++) {
            double t = difftime(history[i].timestamp, t0) / 3600.0; // hours
            double bp = (double)history[i].buy_price;
            double sp = (double)history[i].sell_price;

            sum_t += t;
            sum_bp += bp;
            sum_sp += sp;
            sum_t2 += t * t;
            sum_t_bp += t * bp;
            sum_t_sp += t * sp;
        }

        double denom = n * sum_t2 - sum_t * sum_t;
        if (std::abs(denom) < 0.001) {
            info.buy_trend = Trend::Stable;
            info.sell_trend = Trend::Stable;
            return info;
        }

        // Slope = velocity (copper per hour)
        info.buy_velocity = (float)((n * sum_t_bp - sum_t * sum_bp) / denom);
        info.sell_velocity = (float)((n * sum_t_sp - sum_t * sum_sp) / denom);

        // Percent change: compare first and last
        if (history.front().buy_price > 0) {
            info.buy_change_pct = (int)(((double)history.back().buy_price - history.front().buy_price)
                / history.front().buy_price * 100.0);
        }
        if (history.front().sell_price > 0) {
            info.sell_change_pct = (int)(((double)history.back().sell_price - history.front().sell_price)
                / history.front().sell_price * 100.0);
        }

        // Classify trend based on velocity relative to average price
        auto classify = [](float velocity, double avg_price) -> Trend {
            if (avg_price <= 0) return Trend::Unknown;
            float pct_per_hour = (float)(velocity / avg_price * 100.0);
            if (pct_per_hour > 0.5f) return Trend::Rising;
            if (pct_per_hour < -0.5f) return Trend::Falling;
            return Trend::Stable;
        };

        info.buy_trend = classify(info.buy_velocity, sum_bp / n);
        info.sell_trend = classify(info.sell_velocity, sum_sp / n);

        return info;
    }

    std::vector<TrendInfo> Analyzer::GetTrends(const std::vector<uint32_t>& item_ids) {
        std::vector<TrendInfo> results;
        results.reserve(item_ids.size());
        for (uint32_t id : item_ids) {
            results.push_back(GetTrend(id));
        }
        return results;
    }

    std::vector<SellOpportunity> Analyzer::FindSellOpportunities(
            float min_increase_pct, int min_history_points) {
        std::vector<SellOpportunity> results;

        // Get all owned items from H&S
        auto ownedItems = HoardBridge::GetAllOwnedItems();
        if (ownedItems.empty()) return results;

        auto prices = TPAPI::GetAllPrices();

        for (const auto& [id, owned] : ownedItems) {
            if (owned.total_count <= 0) continue;

            // Need current TP sell price
            auto pit = prices.find(id);
            if (pit == prices.end() || pit->second.sell_price <= 0) continue;
            int current_sell = pit->second.sell_price;

            // Need enough price history to compute a meaningful average
            auto history = PriceDB::GetHistory(id, 100);
            if ((int)history.size() < min_history_points) continue;

            // Compute historical average sell price (excluding the most recent entry)
            double sum_sell = 0;
            int count = 0;
            for (int i = 0; i < (int)history.size() - 1; i++) {
                if (history[i].sell_price > 0) {
                    sum_sell += history[i].sell_price;
                    count++;
                }
            }
            if (count < min_history_points - 1) continue;
            int avg_sell = (int)(sum_sell / count);
            if (avg_sell <= 0) continue;

            // Skip outlier current prices (manipulated listings)
            if (IsOutlierPrice(avg_sell, current_sell, 10.0f)) continue;

            // Calculate % increase over historical average
            float increase_pct = ((float)current_sell - (float)avg_sell) / (float)avg_sell * 100.0f;
            if (increase_pct < min_increase_pct) continue;

            // Profit if selling now vs at historical avg (after tax)
            int profit_now = current_sell - CalcTotalTax(current_sell);
            int profit_avg = avg_sell - CalcTotalTax(avg_sell);
            int profit_vs_avg = profit_now - profit_avg;
            if (profit_vs_avg <= 0) continue;

            SellOpportunity opp;
            opp.item_id = id;
            opp.name = owned.name;
            opp.rarity = owned.rarity;
            opp.owned_count = owned.total_count;
            opp.current_sell = current_sell;
            opp.avg_sell = avg_sell;
            opp.profit_vs_avg = profit_vs_avg;
            opp.price_increase_pct = increase_pct;
            opp.potential_profit = profit_vs_avg * owned.total_count;

            // Fill name from TPAPI cache if H&S didn't provide one
            if (opp.name.empty()) {
                const auto* info = TPAPI::GetItemInfo(id);
                if (info) {
                    opp.name = info->name;
                    opp.rarity = info->rarity;
                }
            }

            results.push_back(opp);
        }

        // Sort by potential profit descending
        std::sort(results.begin(), results.end(),
            [](const SellOpportunity& a, const SellOpportunity& b) {
                return a.potential_profit > b.potential_profit;
            });

        // Limit to top 50
        if (results.size() > 50) results.resize(50);

        return results;
    }

    std::vector<MarketMover> Analyzer::FindMarketMovers(
            float min_price_change_pct, int min_history_points,
            int min_avg_sell_copper, int max_results) {
        std::vector<MarketMover> results;

        auto prices = TPAPI::GetAllPrices();
        if (prices.empty()) return results;

        for (const auto& [id, price] : prices) {
            if (price.sell_price <= 0 || price.buy_price <= 0) continue;

            // Skip outlier current prices
            if (IsOutlierPrice(price.buy_price, price.sell_price, 10.0f)) continue;

            // Need enough price history
            auto history = PriceDB::GetHistory(id, 100);
            if ((int)history.size() < min_history_points) continue;

            // Compute historical averages (exclude most recent entry)
            double sum_sell = 0;
            double sum_buy_qty = 0;
            int count = 0;
            for (int i = 0; i < (int)history.size() - 1; i++) {
                if (history[i].sell_price > 0) {
                    sum_sell += history[i].sell_price;
                    sum_buy_qty += history[i].buy_quantity;
                    count++;
                }
            }
            if (count < min_history_points - 1) continue;

            int avg_sell = (int)(sum_sell / count);
            int avg_buy_qty = (int)(sum_buy_qty / count);
            if (avg_sell <= 0) continue;

            // Skip low-value items (noise)
            if (avg_sell < min_avg_sell_copper) continue;

            // Skip if current price is an outlier vs historical
            if (IsOutlierPrice(avg_sell, price.sell_price, 10.0f)) continue;

            // Calculate price change %
            float price_change_pct = ((float)price.sell_price - (float)avg_sell) / (float)avg_sell * 100.0f;
            if (price_change_pct < min_price_change_pct) continue;

            // Calculate volume change factor
            float volume_factor = 1.0f;
            float volume_change_pct = 0.0f;
            if (avg_buy_qty > 0) {
                volume_factor = std::max(1.0f, (float)price.buy_quantity / (float)avg_buy_qty);
                volume_change_pct = ((float)price.buy_quantity - (float)avg_buy_qty) / (float)avg_buy_qty * 100.0f;
            }

            // Composite spike score: price increase amplified by volume surge
            float spike_score = price_change_pct * volume_factor;

            MarketMover mover;
            mover.item_id = id;
            mover.current_sell = price.sell_price;
            mover.avg_sell = avg_sell;
            mover.price_change_pct = price_change_pct;
            mover.current_buy_qty = price.buy_quantity;
            mover.avg_buy_qty = avg_buy_qty;
            mover.volume_change_pct = volume_change_pct;
            mover.spike_score = spike_score;

            // Fill item name
            const auto* info = TPAPI::GetItemInfo(id);
            if (info) {
                mover.name = info->name;
                mover.rarity = info->rarity;
            } else {
                mover.name = "Item #" + std::to_string(id);
            }

            results.push_back(mover);
        }

        // Sort by spike score descending
        std::sort(results.begin(), results.end(),
            [](const MarketMover& a, const MarketMover& b) {
                return a.spike_score > b.spike_score;
            });

        if (max_results > 0 && (int)results.size() > max_results) {
            results.resize(max_results);
        }

        return results;
    }

    // --- Crafting Profit Analysis ---

    std::vector<CraftingProfit> Analyzer::FindCraftingProfits(const CraftingFilter& filter) {
        std::vector<CraftingProfit> results;

        auto prices = TPAPI::GetAllPrices();
        if (prices.empty()) return results;

        const auto& recipes = TPAPI::GetRecipesByOutput();
        if (recipes.empty()) return results;

        for (const auto& [output_id, recipe] : recipes) {
            if (recipe.ingredients.empty()) continue;
            if (recipe.output_item_count <= 0) continue;

            // Need TP price for the output item
            auto out_it = prices.find(output_id);
            if (out_it == prices.end()) continue;
            if (out_it->second.sell_price <= 0) continue;

            int sell_price = out_it->second.sell_price;
            int buy_price_out = out_it->second.buy_price;
            int sell_qty = out_it->second.sell_quantity;

            // Filter by minimum sell volume
            if (sell_qty < filter.min_sell_volume) continue;

            // Fastest: sell to buy orders, Balanced/Patient: sell via listing
            int output_unit_price = (filter.mode == 0) ? buy_price_out : sell_price;
            if (output_unit_price <= 0) continue;

            // Calculate total ingredient cost
            int total_cost = 0;
            bool valid = true;
            std::vector<std::pair<uint32_t, int>> ing_list;

            for (const auto& ing : recipe.ingredients) {
                auto ing_it = prices.find(ing.item_id);
                if (ing_it == prices.end()) { valid = false; break; }
                // Patient: buy via buy orders (buy_price), Fastest/Balanced: buy instantly (sell_price)
                int ing_price = (filter.mode == 2) ? ing_it->second.buy_price : ing_it->second.sell_price;
                if (ing_price <= 0) { valid = false; break; }
                total_cost += ing_price * ing.count;
                ing_list.push_back({ing.item_id, ing.count});
            }
            if (!valid || total_cost <= 0) continue;

            // Revenue from selling output (after 15% TP tax)
            int revenue = output_unit_price * recipe.output_item_count - CalcTotalTax(output_unit_price * recipe.output_item_count);
            int profit = revenue - total_cost;

            if (profit < filter.min_profit) continue;

            float roi = (float)profit / (float)total_cost * 100.0f;
            if (roi < filter.min_roi) continue;

            CraftingProfit cp;
            cp.item_id = output_id;
            cp.recipe_id = recipe.id;
            cp.recipe_type = recipe.type;
            cp.output_count = recipe.output_item_count;
            cp.ingredient_cost = total_cost;
            cp.sell_price = output_unit_price;
            cp.sell_revenue = revenue;
            cp.profit = profit;
            cp.roi = roi;
            cp.sell_quantity = sell_qty;
            cp.ingredients = ing_list;

            const auto* info = TPAPI::GetItemInfo(output_id);
            if (info) {
                cp.name = info->name;
                cp.rarity = info->rarity;
            } else {
                cp.name = "Item #" + std::to_string(output_id);
            }

            results.push_back(cp);
        }

        // Sort by profit descending
        std::sort(results.begin(), results.end(),
            [](const CraftingProfit& a, const CraftingProfit& b) {
                return a.profit > b.profit;
            });

        if (filter.max_results > 0 && (int)results.size() > filter.max_results) {
            results.resize(filter.max_results);
        }

        return results;
    }

    // --- Formatting ---

    std::string Analyzer::FormatCoins(int copper) {
        if (copper < 0) return "-" + FormatCoins(-copper);

        int gold = copper / 10000;
        int silver = (copper % 10000) / 100;
        int cop = copper % 100;

        std::ostringstream ss;
        if (gold > 0) ss << gold << "g ";
        if (silver > 0 || gold > 0) ss << silver << "s ";
        ss << cop << "c";
        return ss.str();
    }

    std::string Analyzer::FormatCoinsShort(int copper) {
        if (copper < 0) return "-" + FormatCoinsShort(-copper);

        int gold = copper / 10000;
        int silver = (copper % 10000) / 100;

        if (gold > 0) return std::to_string(gold) + "g " + std::to_string(silver) + "s";
        if (silver > 0) return std::to_string(silver) + "s " + std::to_string(copper % 100) + "c";
        return std::to_string(copper) + "c";
    }

    const char* Analyzer::TrendArrow(Trend t) {
        switch (t) {
            case Trend::Rising: return "^";
            case Trend::Falling: return "v";
            case Trend::Stable: return "-";
            default: return "?";
        }
    }

    void Analyzer::TrendColor(Trend t, float& r, float& g, float& b) {
        switch (t) {
            case Trend::Rising: r = 0.3f; g = 0.9f; b = 0.3f; break;
            case Trend::Falling: r = 0.9f; g = 0.3f; b = 0.3f; break;
            case Trend::Stable: r = 0.7f; g = 0.7f; b = 0.7f; break;
            default: r = 0.5f; g = 0.5f; b = 0.5f; break;
        }
    }

}
