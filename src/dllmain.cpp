#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cfloat>
#include <atomic>
#include <mutex>
#include <unordered_set>

#include "nexus/Nexus.h"
#include "imgui.h"
#include "TPAPI.h"
#include "PriceDB.h"
#include "Analyzer.h"
#include "IconManager.h"
#include "HoardBridge.h"
#include "HttpClient.h"
#include <nlohmann/json.hpp>

// Version constants
#define V_MAJOR 0
#define V_MINOR 9
#define V_BUILD 0
#define V_REVISION 0

// Quick Access icon identifiers
#define QA_ID "QA_FLIP_OUT"
#define TEX_ICON "TEX_FLIPOUT_ICON"
#define TEX_ICON_HOVER "TEX_FLIPOUT_ICON_HOVER"

// Embedded 32x32 stick figure icon (normal - grey with black border, tilted 30 degrees)
static const unsigned char ICON_FLIPOUT[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a,
    0xf4, 0x00, 0x00, 0x00, 0xaf, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x96, 0x6d, 0x0e, 0x80,
    0x20, 0x08, 0x86, 0x39, 0x6c, 0xc7, 0xf1, 0x38, 0xde, 0xc0, 0x83, 0xd1, 0x6c, 0xb3, 0x99, 0x09,
    0x66, 0x40, 0x1f, 0x1b, 0x6c, 0xfc, 0xa9, 0xe4, 0x7d, 0x7c, 0x05, 0x17, 0x80, 0x87, 0xc7, 0xcf,
    0x03, 0xab, 0x7c, 0x07, 0x20, 0xc6, 0xe8, 0x00, 0x0e, 0xc0, 0x01, 0xa0, 0x56, 0xa3, 0x52, 0x85,
    0x7a, 0x00, 0x87, 0x6f, 0xf3, 0x7b, 0x0d, 0x97, 0xa8, 0x42, 0xf5, 0xb3, 0x93, 0x68, 0xb3, 0x46,
    0x15, 0x00, 0x7b, 0x00, 0x8c, 0x30, 0xaa, 0x5c, 0x38, 0xbd, 0xc2, 0xad, 0xa8, 0xb6, 0x30, 0x0b,
    0xc1, 0x88, 0x9a, 0x4d, 0x85, 0xd9, 0x19, 0x8b, 0x20, 0x2e, 0x00, 0xe8, 0x8f, 0xe3, 0xe0, 0xdc,
    0x4f, 0xd3, 0x12, 0x42, 0x58, 0x72, 0xde, 0x05, 0xd8, 0x8b, 0xa6, 0x94, 0xc8, 0xe6, 0x63, 0x5c,
    0x11, 0x3b, 0xb0, 0x09, 0xd7, 0x49, 0xed, 0x7c, 0xc2, 0x95, 0xb9, 0xdd, 0xb7, 0x00, 0xc5, 0x09,
    0xee, 0x16, 0x2c, 0x29, 0xb6, 0xbf, 0x27, 0xde, 0x40, 0x80, 0x65, 0x03, 0xce, 0x38, 0x60, 0xd2,
    0xf9, 0x30, 0x00, 0x20, 0xd7, 0x48, 0xad, 0x67, 0xa7, 0xe0, 0xc9, 0xd9, 0xff, 0xd4, 0x4f, 0xa8,
    0x87, 0x87, 0x7a, 0xac, 0x76, 0x9f, 0xac, 0xad, 0x6b, 0xf6, 0xea, 0xae, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const unsigned int ICON_FLIPOUT_size = 232;

// Embedded 32x32 stick figure icon (hover - brighter white-grey with black border)
static const unsigned char ICON_FLIPOUT_HOVER[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a,
    0xf4, 0x00, 0x00, 0x00, 0xb0, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x96, 0xdd, 0x0e, 0x80,
    0x20, 0x08, 0x85, 0x79, 0xff, 0xf5, 0x60, 0xbe, 0x8b, 0x37, 0xbe, 0x01, 0xcd, 0x36, 0x9b, 0x99,
    0x60, 0x06, 0xf4, 0xb3, 0xc1, 0xc6, 0x4d, 0x25, 0xe7, 0xf3, 0x08, 0x2e, 0x00, 0x0f, 0x8f, 0x9f,
    0x07, 0x56, 0xf9, 0x0e, 0x40, 0x8c, 0xd1, 0x01, 0x1c, 0x80, 0x03, 0x40, 0xad, 0x46, 0xa5, 0x0a,
    0xf5, 0x00, 0x0e, 0xdf, 0xe6, 0xf7, 0x1a, 0x2e, 0x51, 0x85, 0xea, 0x67, 0x27, 0xd1, 0x66, 0x8d,
    0x2a, 0x00, 0xf6, 0x00, 0x18, 0x61, 0x54, 0xb9, 0x70, 0x7a, 0x85, 0x5b, 0x51, 0x6d, 0x61, 0x16,
    0x82, 0x11, 0x35, 0x9b, 0x0a, 0xb3, 0x33, 0x16, 0x41, 0x5c, 0x00, 0xd0, 0x1f, 0xc7, 0xc1, 0xb9,
    0x9f, 0xa6, 0x25, 0x84, 0xb0, 0xe4, 0xbc, 0x0b, 0xb0, 0x17, 0x4d, 0x29, 0x91, 0xcd, 0xc7, 0xb8,
    0x22, 0x76, 0x60, 0x13, 0xae, 0x93, 0xda, 0xf9, 0x84, 0x2b, 0x73, 0xbb, 0x6f, 0x01, 0x8a, 0x13,
    0xdc, 0x2d, 0x58, 0x52, 0x6c, 0x7f, 0x4f, 0xbc, 0x81, 0x00, 0xcb, 0x06, 0x9c, 0x71, 0xc0, 0xa4,
    0xf3, 0x61, 0x00, 0x40, 0xae, 0x91, 0x5a, 0xcf, 0x4e, 0xc1, 0x93, 0xb3, 0xff, 0xa9, 0x9f, 0x50,
    0x0f, 0x0f, 0xf5, 0x58, 0x01, 0x73, 0xce, 0xe5, 0xfb, 0x5b, 0xae, 0x93, 0xba, 0x00, 0x00, 0x00,
    0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const unsigned int ICON_FLIPOUT_HOVER_size = 233;

// Global variables
HMODULE hSelf;
AddonDefinition_t AddonDef{};
AddonAPI_t* APIDefs = nullptr;
bool g_WindowVisible = false;
static bool g_ShowQAIcon = true;

// UI State
static int g_ActiveTab = 0; // 0=Flips, 1=Watchlist, 2=Transactions, 3=Crafting, 4=Search

// Flip scanner state
static std::vector<FlipOut::FlipOpportunity> g_Flips;
static FlipOut::FlipFilter g_FlipFilter;
static bool g_FlipsDirty = true;
static int g_SortMode = 0; // maps to FlipSort enum

// Sell opportunities state
static std::vector<FlipOut::SellOpportunity> g_SellOpps;
static bool g_SellOppsDirty = true;

// Market movers state
static std::vector<FlipOut::MarketMover> g_MarketMovers;
static bool g_MoversDirty = true;

// Async analysis results (written by background threads, read by render thread)
static std::atomic<bool> g_AnalysisBusy{false};
static std::mutex g_AnalysisMutex;
static std::vector<FlipOut::FlipOpportunity> g_FlipsPending;
static std::vector<FlipOut::SellOpportunity> g_SellOppsPending;
static std::vector<FlipOut::MarketMover> g_MoversPending;
static std::atomic<bool> g_FlipsPendingReady{false};
static std::atomic<bool> g_SellOppsPendingReady{false};
static std::atomic<bool> g_MoversPendingReady{false};
static std::vector<FlipOut::CraftingProfit> g_CraftingPending;
static std::atomic<bool> g_CraftingPendingReady{false};

// Data loading state
static std::atomic<bool> g_DataLoading{false};
static std::atomic<bool> g_DataLoaded{false};

// Price history window state
static bool g_PriceHistoryOpen = false;
static uint32_t g_PriceHistoryItemId = 0;
static std::string g_PriceHistoryItemName;
static std::string g_PriceHistoryItemRarity;
static int g_PriceHistoryRange = 1; // 0=1W, 1=1M, 2=3M, 3=6M

// Crafting state
static std::vector<FlipOut::CraftingProfit> g_CraftingProfits;
static FlipOut::CraftingFilter g_CraftingFilter;
static bool g_CraftingDirty = true;
static bool g_CraftingKnownOnly = false;
static bool g_CraftingRecipesQueried = false;
static bool g_CraftingIngredientsQueried = false;
static int g_CraftingMode = 1; // 0=Fastest, 1=Balanced, 2=Patient

// Liquidate tab state
struct LiquidateEntry {
    uint32_t item_id;
    std::string name;
    std::string rarity;
    std::string icon_url;
    int count;
    int buy_price;      // sell to buy orders (instant gold)
    int sell_price;      // list on TP
    int total_instant;   // count * buy_price
    int total_listed;    // count * sell_price * 0.85 (after 15% tax)
};
static std::vector<LiquidateEntry> g_LiquidateItems;
static bool g_LiquidateLoading = false;
static bool g_LiquidateLoaded = false;
static std::chrono::steady_clock::time_point g_LiquidateScanStart;

// Search state
static char g_SearchBuf[256] = "";
static std::vector<FlipOut::ItemInfo> g_SearchResults;

// Auto-refresh
static bool g_AutoRefresh = true;
static int g_RefreshIntervalMin = 15;
static std::chrono::steady_clock::time_point g_LastAutoRefresh;

// Seed data
static bool g_SeedDownloaded = false;

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH: hSelf = hModule; break;
    case DLL_PROCESS_DETACH: break;
    case DLL_THREAD_ATTACH: break;
    case DLL_THREAD_DETACH: break;
    }
    return TRUE;
}

// Forward declarations
void AddonLoad(AddonAPI_t* aApi);
void AddonUnload();
void ProcessKeybind(const char* aIdentifier, bool aIsRelease);
void AddonRender();
void AddonOptions();

static const char* SEED_URL = "https://raw.githubusercontent.com/PieOrCake/flip_out/main/data/seed_prices.json";

static void TryDownloadSeed() {
    if (g_SeedDownloaded) return;
    if (FlipOut::PriceDB::GetTrackedItemCount() >= 100) return;
    g_SeedDownloaded = true;

    std::thread([]() {
        try {
            std::string data = FlipOut::HttpClient::Get(SEED_URL);
            if (data.empty() || data[0] != '{') return;
            if (FlipOut::PriceDB::ImportSeed(data)) {
                FlipOut::PriceDB::Save();
                if (APIDefs) {
                    APIDefs->Log(LOGL_INFO, "FlipOut", "Imported community seed price data");
                }
            }
        } catch (...) {}
    }).detach();
}

// --- Render Helpers ---

// GW2 rarity colors
static ImVec4 GetRarityColor(const std::string& rarity) {
    if (rarity == "Legendary")  return ImVec4(0.63f, 0.39f, 0.78f, 1.0f);
    if (rarity == "Ascended")   return ImVec4(0.90f, 0.39f, 0.55f, 1.0f);
    if (rarity == "Exotic")     return ImVec4(1.00f, 0.65f, 0.00f, 1.0f);
    if (rarity == "Rare")       return ImVec4(1.00f, 0.86f, 0.20f, 1.0f);
    if (rarity == "Masterwork") return ImVec4(0.12f, 0.71f, 0.12f, 1.0f);
    if (rarity == "Fine")       return ImVec4(0.35f, 0.63f, 0.90f, 1.0f);
    return ImVec4(0.80f, 0.80f, 0.80f, 1.0f);
}

static const float ICON_SIZE = 20.0f;

// Render item icon with rarity border
static void RenderItemIcon(uint32_t itemId, const std::string& iconUrl,
                           const std::string& rarity) {
    Texture_t* tex = FlipOut::IconManager::GetIcon(itemId);
    if (tex && tex->Resource) {
        ImVec4 borderColor = GetRarityColor(rarity);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRect(
            ImVec2(pos.x - 1, pos.y - 1),
            ImVec2(pos.x + ICON_SIZE + 1, pos.y + ICON_SIZE + 1),
            ImGui::ColorConvertFloat4ToU32(borderColor), 0.0f, 0, 2.0f);
        ImGui::Image(tex->Resource, ImVec2(ICON_SIZE, ICON_SIZE));
    } else {
        ImVec4 col = GetRarityColor(rarity);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(
            pos, ImVec2(pos.x + ICON_SIZE, pos.y + ICON_SIZE),
            ImGui::ColorConvertFloat4ToU32(ImVec4(col.x * 0.3f, col.y * 0.3f, col.z * 0.3f, 0.5f)));
        ImGui::Dummy(ImVec2(ICON_SIZE, ICON_SIZE));
        if (!iconUrl.empty()) {
            FlipOut::IconManager::RequestIcon(itemId, iconUrl);
        }
    }
}

// Coin display helper with gold/silver/copper coloring
static const ImVec4 COL_GOLD   = ImVec4(1.0f, 0.85f, 0.0f, 1.0f);
static const ImVec4 COL_SILVER = ImVec4(0.75f, 0.75f, 0.8f, 1.0f);
static const ImVec4 COL_COPPER = ImVec4(0.72f, 0.45f, 0.2f, 1.0f);

static void RenderCoins(int copper) {
    if (copper < 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "-");
        ImGui::SameLine(0, 0);
        RenderCoins(-copper);
        return;
    }
    int gold = copper / 10000;
    int silver = (copper % 10000) / 100;
    int cop = copper % 100;

    bool needSpace = false;
    if (gold > 0) {
        ImGui::TextColored(COL_GOLD, "%dg", gold);
        needSpace = true;
    }
    if (silver > 0 || gold > 0) {
        if (needSpace) { ImGui::SameLine(0, 2); }
        ImGui::TextColored(COL_SILVER, "%ds", silver);
        needSpace = true;
    }
    if (needSpace) { ImGui::SameLine(0, 2); }
    ImGui::TextColored(COL_COPPER, "%dc", cop);
}

// Trend indicator
static void RenderTrend(FlipOut::Trend t) {
    float r, g, b;
    FlipOut::Analyzer::TrendColor(t, r, g, b);
    ImGui::TextColored(ImVec4(r, g, b, 1.0f), "%s", FlipOut::Analyzer::TrendArrow(t));
}

// --- Price History Window ---

static void OpenPriceHistory(uint32_t item_id, const std::string& name, const std::string& rarity) {
    g_PriceHistoryItemId = item_id;
    g_PriceHistoryItemName = name;
    g_PriceHistoryItemRarity = rarity;
    g_PriceHistoryOpen = true;
}

static void RenderPriceHistoryWindow() {
    if (!g_PriceHistoryOpen || g_PriceHistoryItemId == 0) return;

    std::string title = "Price History: " + g_PriceHistoryItemName + "###price_history";
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title.c_str(), &g_PriceHistoryOpen)) {
        ImGui::End();
        return;
    }

    // Item header with icon
    const auto* info = FlipOut::TPAPI::GetItemInfo(g_PriceHistoryItemId);
    std::string iconUrl = info ? info->icon_url : "";
    RenderItemIcon(g_PriceHistoryItemId, iconUrl, g_PriceHistoryItemRarity);
    ImGui::SameLine();
    ImGui::TextColored(GetRarityColor(g_PriceHistoryItemRarity), "%s", g_PriceHistoryItemName.c_str());

    // Time range buttons
    const char* range_labels[] = { "1W", "1M", "3M", "6M" };
    const int range_hours[] = { 24*7, 24*30, 24*90, 24*180 };
    ImGui::Spacing();
    for (int i = 0; i < 4; i++) {
        if (i > 0) ImGui::SameLine();
        bool selected = (g_PriceHistoryRange == i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button(range_labels[i], ImVec2(40, 0))) {
            g_PriceHistoryRange = i;
        }
        if (selected) ImGui::PopStyleColor();
    }

    // Fetch history data filtered by time range; fall back to all data if range is too narrow
    auto all_history = FlipOut::PriceDB::GetHistory(g_PriceHistoryItemId, 0);
    time_t cutoff = std::time(nullptr) - (time_t)range_hours[g_PriceHistoryRange] * 3600;
    std::vector<FlipOut::PriceSnapshot> history;
    for (const auto& snap : all_history) {
        if (snap.timestamp >= cutoff) history.push_back(snap);
    }
    if (history.size() < 2 && all_history.size() >= 2) {
        history = all_history;
    }

    // Downsample for smoother graphs: 1W=2/day(12h), 1M/3M/6M=1/day(24h)
    if (history.size() > 2) {
        int bucket_secs = (g_PriceHistoryRange == 0) ? 12 * 3600 : 24 * 3600;
        std::vector<FlipOut::PriceSnapshot> downsampled;
        downsampled.reserve(history.size());
        time_t bucket_start = history.front().timestamp;
        long long sum_buy = 0, sum_sell = 0;
        int count = 0;
        time_t last_ts = 0;
        for (const auto& s : history) {
            if (s.timestamp - bucket_start >= bucket_secs && count > 0) {
                FlipOut::PriceSnapshot avg;
                avg.timestamp = last_ts;
                avg.buy_price = (int)(sum_buy / count);
                avg.sell_price = (int)(sum_sell / count);
                downsampled.push_back(avg);
                bucket_start = s.timestamp;
                sum_buy = sum_sell = 0;
                count = 0;
            }
            sum_buy += s.buy_price;
            sum_sell += s.sell_price;
            last_ts = s.timestamp;
            count++;
        }
        if (count > 0) {
            FlipOut::PriceSnapshot avg;
            avg.timestamp = last_ts;
            avg.buy_price = (int)(sum_buy / count);
            avg.sell_price = (int)(sum_sell / count);
            downsampled.push_back(avg);
        }
        history = std::move(downsampled);
    }

    if (history.size() < 2) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Not enough price data for this time range. Need at least 2 data points.");
        ImGui::Text("Total history entries: %d", (int)all_history.size());
        ImGui::End();
        return;
    }

    // Find min/max for scaling
    int min_price = INT_MAX, max_price = 0;
    for (const auto& s : history) {
        if (s.buy_price > 0 && s.buy_price < min_price) min_price = s.buy_price;
        if (s.sell_price > 0 && s.sell_price < min_price) min_price = s.sell_price;
        if (s.buy_price > max_price) max_price = s.buy_price;
        if (s.sell_price > max_price) max_price = s.sell_price;
    }

    // Add 10% padding to range
    int price_range = max_price - min_price;
    if (price_range <= 0) price_range = 1;
    int pad = std::max(price_range / 10, 1);
    min_price = std::max(0, min_price - pad);
    max_price = max_price + pad;
    price_range = max_price - min_price;

    time_t t_min = history.front().timestamp;
    time_t t_max = history.back().timestamp;
    double t_range = difftime(t_max, t_min);
    if (t_range <= 0) t_range = 1;

    // Current price display
    const auto& latest = history.back();
    ImGui::Spacing();
    ImGui::Text("Buy: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "%s",
        FlipOut::Analyzer::FormatCoins(latest.buy_price).c_str());
    ImGui::SameLine();
    ImGui::Text("   Sell: ");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "%s",
        FlipOut::Analyzer::FormatCoins(latest.sell_price).c_str());
    ImGui::SameLine();
    ImGui::Text("   Points: %d", (int)history.size());

    // Graph area
    ImGui::Spacing();
    ImVec2 graph_pos = ImGui::GetCursorScreenPos();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float graph_w = avail.x - 70.0f; // leave room for Y axis labels
    float graph_h = avail.y - 30.0f; // leave room for X axis labels
    if (graph_w < 100) graph_w = 100;
    if (graph_h < 80) graph_h = 80;

    float label_w = 65.0f;
    ImVec2 gp = ImVec2(graph_pos.x + label_w, graph_pos.y);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Background
    dl->AddRectFilled(gp, ImVec2(gp.x + graph_w, gp.y + graph_h),
        IM_COL32(20, 20, 25, 255));
    dl->AddRect(gp, ImVec2(gp.x + graph_w, gp.y + graph_h),
        IM_COL32(60, 60, 70, 255));

    // Y axis labels and grid lines (5 ticks)
    for (int i = 0; i <= 4; i++) {
        float frac = (float)i / 4.0f;
        int price_val = max_price - (int)(frac * price_range);
        float y = gp.y + frac * graph_h;

        // Grid line
        dl->AddLine(ImVec2(gp.x, y), ImVec2(gp.x + graph_w, y),
            IM_COL32(40, 40, 50, 255));

        // Label
        std::string label = FlipOut::Analyzer::FormatCoinsShort(price_val);
        ImVec2 text_size = ImGui::CalcTextSize(label.c_str());
        dl->AddText(ImVec2(gp.x - text_size.x - 5, y - text_size.y * 0.5f),
            IM_COL32(160, 160, 160, 255), label.c_str());
    }

    // X axis labels (5 ticks)
    for (int i = 0; i <= 4; i++) {
        float frac = (float)i / 4.0f;
        time_t t_val = t_min + (time_t)(frac * t_range);
        float x = gp.x + frac * graph_w;

        // Grid line
        dl->AddLine(ImVec2(x, gp.y), ImVec2(x, gp.y + graph_h),
            IM_COL32(40, 40, 50, 255));

        // Label
        struct tm* tm_info = localtime(&t_val);
        char tbuf[32];
        char mon[8];
        strftime(mon, sizeof(mon), "%b", tm_info);
        snprintf(tbuf, sizeof(tbuf), "%d %s", tm_info->tm_mday, mon);
        ImVec2 text_size = ImGui::CalcTextSize(tbuf);
        dl->AddText(ImVec2(x - text_size.x * 0.5f, gp.y + graph_h + 4),
            IM_COL32(160, 160, 160, 255), tbuf);
    }

    // Helper lambda to map data point to screen coords
    auto MapPoint = [&](time_t t, int price) -> ImVec2 {
        float x = gp.x + (float)(difftime(t, t_min) / t_range) * graph_w;
        float y = gp.y + (1.0f - (float)(price - min_price) / (float)price_range) * graph_h;
        return ImVec2(x, y);
    };

    // Draw buy price line (green)
    for (int i = 1; i < (int)history.size(); i++) {
        if (history[i-1].buy_price <= 0 || history[i].buy_price <= 0) continue;
        ImVec2 p0 = MapPoint(history[i-1].timestamp, history[i-1].buy_price);
        ImVec2 p1 = MapPoint(history[i].timestamp, history[i].buy_price);
        dl->AddLine(p0, p1, IM_COL32(90, 210, 90, 255), 2.0f);
    }

    // Draw sell price line (orange)
    for (int i = 1; i < (int)history.size(); i++) {
        if (history[i-1].sell_price <= 0 || history[i].sell_price <= 0) continue;
        ImVec2 p0 = MapPoint(history[i-1].timestamp, history[i-1].sell_price);
        ImVec2 p1 = MapPoint(history[i].timestamp, history[i].sell_price);
        dl->AddLine(p0, p1, IM_COL32(255, 180, 50, 255), 2.0f);
    }

    // Legend
    ImVec2 legend_pos = ImVec2(gp.x + 8, gp.y + 6);
    dl->AddRectFilled(legend_pos, ImVec2(legend_pos.x + 90, legend_pos.y + 34),
        IM_COL32(10, 10, 15, 200), 4.0f);
    dl->AddLine(ImVec2(legend_pos.x + 5, legend_pos.y + 9),
        ImVec2(legend_pos.x + 20, legend_pos.y + 9), IM_COL32(90, 210, 90, 255), 2.0f);
    dl->AddText(ImVec2(legend_pos.x + 24, legend_pos.y + 2),
        IM_COL32(200, 200, 200, 255), "Buy");
    dl->AddLine(ImVec2(legend_pos.x + 5, legend_pos.y + 25),
        ImVec2(legend_pos.x + 20, legend_pos.y + 25), IM_COL32(255, 180, 50, 255), 2.0f);
    dl->AddText(ImVec2(legend_pos.x + 24, legend_pos.y + 18),
        IM_COL32(200, 200, 200, 255), "Sell");

    // Hover tooltip: find closest data point to mouse
    ImVec2 mouse = ImGui::GetMousePos();
    if (mouse.x >= gp.x && mouse.x <= gp.x + graph_w &&
        mouse.y >= gp.y && mouse.y <= gp.y + graph_h) {
        float mouse_frac = (mouse.x - gp.x) / graph_w;
        time_t mouse_t = t_min + (time_t)(mouse_frac * t_range);

        // Find closest point
        int closest = 0;
        double closest_dist = 1e18;
        for (int i = 0; i < (int)history.size(); i++) {
            double d = std::abs(difftime(history[i].timestamp, mouse_t));
            if (d < closest_dist) { closest_dist = d; closest = i; }
        }

        const auto& pt = history[closest];
        ImVec2 buy_pt = MapPoint(pt.timestamp, pt.buy_price);
        ImVec2 sell_pt = MapPoint(pt.timestamp, pt.sell_price);

        // Vertical crosshair
        float cx = buy_pt.x;
        dl->AddLine(ImVec2(cx, gp.y), ImVec2(cx, gp.y + graph_h),
            IM_COL32(100, 100, 120, 150));

        // Data point dots
        if (pt.buy_price > 0)
            dl->AddCircleFilled(buy_pt, 4.0f, IM_COL32(90, 210, 90, 255));
        if (pt.sell_price > 0)
            dl->AddCircleFilled(sell_pt, 4.0f, IM_COL32(255, 180, 50, 255));

        // Tooltip
        ImGui::BeginTooltip();
        struct tm* tt = localtime(&pt.timestamp);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M", tt);
        ImGui::Text("%s", timebuf);
        ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "Buy:  %s",
            FlipOut::Analyzer::FormatCoins(pt.buy_price).c_str());
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Sell: %s",
            FlipOut::Analyzer::FormatCoins(pt.sell_price).c_str());
        if (pt.buy_price > 0 && pt.sell_price > 0) {
            int profit = FlipOut::Analyzer::CalcProfit(pt.buy_price, pt.sell_price);
            ImGui::Text("Profit: %s (%.1f%%)",
                FlipOut::Analyzer::FormatCoins(profit).c_str(),
                FlipOut::Analyzer::CalcMargin(pt.buy_price, pt.sell_price));
        }
        ImGui::EndTooltip();
    }

    // Reserve the space so ImGui knows the graph area is used
    ImGui::Dummy(ImVec2(label_w + graph_w, graph_h + 25));

    ImGui::End();
}

// --- Flips Tab ---

static std::chrono::steady_clock::time_point g_LastScanTime{};
static bool g_HasScanned = false;
static constexpr int SCAN_COOLDOWN_SECONDS = 600; // 10 minutes

static bool CanScan() {
    if (!g_HasScanned) return true;
    auto elapsed = std::chrono::steady_clock::now() - g_LastScanTime;
    return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= SCAN_COOLDOWN_SECONDS;
}

static int ScanCooldownRemaining() {
    if (!g_HasScanned) return 0;
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - g_LastScanTime).count();
    int remaining = SCAN_COOLDOWN_SECONDS - (int)elapsed;
    return remaining > 0 ? remaining : 0;
}

static std::string FormatTimeAgo(std::chrono::steady_clock::time_point t) {
    auto elapsed = std::chrono::steady_clock::now() - t;
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    if (secs < 60) return std::to_string(secs) + "s ago";
    auto mins = secs / 60;
    if (mins < 60) return std::to_string(mins) + "m ago";
    auto hrs = mins / 60;
    mins = mins % 60;
    return std::to_string(hrs) + "h " + std::to_string(mins) + "m ago";
}

static void RenderFlipsTab() {
    // Controls row
    auto fetchStatus = FlipOut::TPAPI::GetBulkFetchStatus();
    bool scanning = (fetchStatus == FlipOut::FetchStatus::InProgress);

    if (scanning) {
        if (ImGui::Button("Cancel")) {
            FlipOut::TPAPI::CancelScan();
        }
        ImGui::SameLine();
        float progress = FlipOut::TPAPI::GetBulkFetchProgress();
        ImGui::ProgressBar(progress, ImVec2(150, 0));
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s",
            FlipOut::TPAPI::GetBulkFetchMessage().c_str());
    } else {
        bool canScan = CanScan();
        // Fixed-width button to prevent layout shift when label changes
        float btnW = ImGui::CalcTextSize("Scan Market (00:00)").x + ImGui::GetStyle().FramePadding.x * 2;
        if (!canScan) {
            int cd = ScanCooldownRemaining();
            char label[64];
            snprintf(label, sizeof(label), "Scan Market (%d:%02d)", cd / 60, cd % 60);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.4f);
            ImGui::Button(label, ImVec2(btnW, 0));
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Scan Market", ImVec2(btnW, 0))) {
                if (FlipOut::HoardBridge::IsDataAvailable()) {
                    FlipOut::HoardBridge::RequestRefresh();
                }
                FlipOut::TPAPI::FetchAllPricesAsync();
                g_FlipsDirty = true;
            }
        }
        ImGui::SameLine();
        if (fetchStatus == FlipOut::FetchStatus::Success) {
            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "%s",
                FlipOut::TPAPI::GetBulkFetchMessage().c_str());
        } else if (fetchStatus == FlipOut::FetchStatus::Error) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s",
                FlipOut::TPAPI::GetBulkFetchMessage().c_str());
        } else if (!canScan) {
            // nothing — cooldown is in the button label
        } else {
            const auto& msg = FlipOut::TPAPI::GetBulkFetchMessage();
            if (!msg.empty()) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", msg.c_str());
            }
        }
        if (g_HasScanned) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| Last scan: %s",
                FormatTimeAgo(g_LastScanTime).c_str());
        }
    }

    // When bulk fetch completes, record prices in background and mark analysis dirty
    {
        static FlipOut::FetchStatus s_prevStatus = FlipOut::FetchStatus::Idle;
        if (fetchStatus == FlipOut::FetchStatus::Success && s_prevStatus != FlipOut::FetchStatus::Success) {
            g_FlipsDirty = true;
            g_CraftingDirty = true;
            g_HasScanned = true;
            g_LastScanTime = std::chrono::steady_clock::now();

            // Record prices to history DB in background thread (heavy: ~27k items)
            std::thread([]() {
                auto prices = FlipOut::TPAPI::GetAllPrices();
                std::unordered_map<uint32_t, FlipOut::PriceSnapshot> snaps;
                snaps.reserve(prices.size());
                for (const auto& [id, p] : prices) {
                    FlipOut::PriceSnapshot s;
                    s.buy_price = p.buy_price;
                    s.sell_price = p.sell_price;
                    s.buy_quantity = p.buy_quantity;
                    s.sell_quantity = p.sell_quantity;
                    snaps[id] = s;
                }
                FlipOut::PriceDB::RecordBulkPrices(snaps);
                FlipOut::PriceDB::Save();
            }).detach();
        }
        s_prevStatus = fetchStatus;
    }

    // Pick up completed async analysis results
    if (g_FlipsPendingReady) {
        std::lock_guard<std::mutex> lock(g_AnalysisMutex);
        g_Flips = std::move(g_FlipsPending);
        g_FlipsPendingReady = false;
        g_SellOppsDirty = true;
        g_MoversDirty = true;
    }
    if (g_MoversPendingReady) {
        std::lock_guard<std::mutex> lock(g_AnalysisMutex);
        g_MarketMovers = std::move(g_MoversPending);
        g_MoversPendingReady = false;
    }
    if (g_SellOppsPendingReady) {
        std::lock_guard<std::mutex> lock(g_AnalysisMutex);
        g_SellOpps = std::move(g_SellOppsPending);
        g_SellOppsPendingReady = false;
    }

    // Dispatch analysis to background threads when dirty
    {
        static size_t s_lastOwnedCount = 0;
        size_t curOwnedCount = FlipOut::HoardBridge::GetOwnedItemCount();
        if (curOwnedCount != s_lastOwnedCount) {
            s_lastOwnedCount = curOwnedCount;
            g_SellOppsDirty = true;
        }
    }
    if (g_FlipsDirty && !g_AnalysisBusy) {
        g_FlipsDirty = false;
        g_AnalysisBusy = true;
        auto filter = g_FlipFilter;
        std::thread([filter]() {
            auto result = FlipOut::Analyzer::FindFlips(filter);
            {
                std::lock_guard<std::mutex> lock(g_AnalysisMutex);
                g_FlipsPending = std::move(result);
            }
            g_FlipsPendingReady = true;
            g_AnalysisBusy = false;
        }).detach();
    } else if (g_MoversDirty && !g_AnalysisBusy) {
        g_MoversDirty = false;
        g_AnalysisBusy = true;
        std::thread([]() {
            auto result = FlipOut::Analyzer::FindMarketMovers(20.0f, 3, 100, 50);
            {
                std::lock_guard<std::mutex> lock(g_AnalysisMutex);
                g_MoversPending = std::move(result);
            }
            g_MoversPendingReady = true;
            g_AnalysisBusy = false;
        }).detach();
    } else if (g_SellOppsDirty && !g_AnalysisBusy && FlipOut::HoardBridge::GetOwnedItemCount() > 0) {
        g_SellOppsDirty = false;
        g_AnalysisBusy = true;
        std::thread([]() {
            auto result = FlipOut::Analyzer::FindSellOpportunities(20.0f, 3);
            {
                std::lock_guard<std::mutex> lock(g_AnalysisMutex);
                g_SellOppsPending = std::move(result);
            }
            g_SellOppsPendingReady = true;
            g_AnalysisBusy = false;
        }).detach();
    }

    ImGui::Separator();

    // --- Sell Opportunities section (owned items with inflated prices) ---
    if (!g_SellOpps.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.35f, 0.15f, 0.8f));
        if (ImGui::CollapsingHeader(("Sell Now (" + std::to_string(g_SellOpps.size()) + " owned items with inflated prices)###sellnow").c_str(),
                ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f),
                "Items you own whose current sell price is above their historical average.");

            if (ImGui::BeginTable("##sell_opps", 7,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
                ImVec2(0, std::min((float)g_SellOpps.size() * 28.0f + 30.0f, 250.0f))))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Current Sell", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Avg Sell", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Above Avg", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Profit/Unit", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Total Profit", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 85.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
                    if (specs->SpecsDirty && specs->SpecsCount > 0) {
                        const auto& s = specs->Specs[0];
                        bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                        std::sort(g_SellOpps.begin(), g_SellOpps.end(), [&](const FlipOut::SellOpportunity& a, const FlipOut::SellOpportunity& b) {
                            int cmp = 0;
                            switch (s.ColumnIndex) {
                                case 0: cmp = a.name.compare(b.name); break;
                                case 1: cmp = a.owned_count - b.owned_count; break;
                                case 2: cmp = a.current_sell - b.current_sell; break;
                                case 3: cmp = a.avg_sell - b.avg_sell; break;
                                case 4: cmp = (a.price_increase_pct > b.price_increase_pct) - (a.price_increase_pct < b.price_increase_pct); break;
                                case 5: cmp = a.profit_vs_avg - b.profit_vs_avg; break;
                                case 6: cmp = a.potential_profit - b.potential_profit; break;
                            }
                            return asc ? cmp < 0 : cmp > 0;
                        });
                        specs->SpecsDirty = false;
                    }
                }

                for (const auto& opp : g_SellOpps) {
                    ImGui::TableNextRow(0, ICON_SIZE + 4);
                    ImGui::PushID((int)opp.item_id);

                    // Invisible selectable spanning all columns for row right-click
                    ImGui::TableNextColumn();
                    ImGui::Selectable("##row", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                        ImVec2(0, ICON_SIZE));
                    if (ImGui::IsItemClicked(0)) {
                        OpenPriceHistory(opp.item_id, opp.name, opp.rarity);
                    }
                    if (ImGui::BeginPopupContextItem("##sell_ctx")) {
                        bool watched = FlipOut::PriceDB::IsOnWatchlist(opp.item_id);
                        if (watched) {
                            if (ImGui::MenuItem("Remove from Watchlist")) {
                                FlipOut::PriceDB::RemoveFromWatchlist(opp.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        } else {
                            if (ImGui::MenuItem("Add to Watchlist")) {
                                FlipOut::PriceDB::AddToWatchlist(opp.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        }
                        if (ImGui::MenuItem("Search in Hoard & Seek")) {
                            FlipOut::HoardBridge::SearchInHoard(opp.name);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("View Price History")) {
                            OpenPriceHistory(opp.item_id, opp.name, opp.rarity);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine(0, 0);

                    // Item name + icon
                    const auto* info = FlipOut::TPAPI::GetItemInfo(opp.item_id);
                    std::string iconUrl = info ? info->icon_url : "";
                    RenderItemIcon(opp.item_id, iconUrl, opp.rarity);
                    ImGui::SameLine();
                    ImGui::TextColored(GetRarityColor(opp.rarity), "%s", opp.name.c_str());

                    // Owned count
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", opp.owned_count);

                    // Current sell
                    ImGui::TableNextColumn();
                    RenderCoins(opp.current_sell);

                    // Avg sell
                    ImGui::TableNextColumn();
                    RenderCoins(opp.avg_sell);

                    // % above avg
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f),
                        "+%.0f%%", opp.price_increase_pct);

                    // Profit per unit vs avg
                    ImGui::TableNextColumn();
                    RenderCoins(opp.profit_vs_avg);

                    // Total potential profit
                    ImGui::TableNextColumn();
                    RenderCoins(opp.potential_profit);

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // --- Market Movers section (items with biggest price/volume spikes) ---
    if (!g_MarketMovers.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.20f, 0.10f, 0.8f));
        if (ImGui::CollapsingHeader(("Market Movers (" + std::to_string(g_MarketMovers.size()) + " items spiking)###movers").c_str(),
                ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                "Items with significant price increases vs their historical average, ranked by spike intensity.");

            if (ImGui::BeginTable("##movers", 7,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY,
                ImVec2(0, std::min((float)g_MarketMovers.size() * 28.0f + 30.0f, 250.0f))))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Now", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Buy Vol", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Vol", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 50.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
                    if (specs->SpecsDirty && specs->SpecsCount > 0) {
                        const auto& s = specs->Specs[0];
                        bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                        std::sort(g_MarketMovers.begin(), g_MarketMovers.end(), [&](const FlipOut::MarketMover& a, const FlipOut::MarketMover& b) {
                            int cmp = 0;
                            switch (s.ColumnIndex) {
                                case 0: cmp = a.name.compare(b.name); break;
                                case 1: cmp = a.current_sell - b.current_sell; break;
                                case 2: cmp = a.avg_sell - b.avg_sell; break;
                                case 3: cmp = (a.price_change_pct > b.price_change_pct) - (a.price_change_pct < b.price_change_pct); break;
                                case 4: cmp = a.current_buy_qty - b.current_buy_qty; break;
                                case 5: cmp = (a.volume_change_pct > b.volume_change_pct) - (a.volume_change_pct < b.volume_change_pct); break;
                                case 6: cmp = (a.spike_score > b.spike_score) - (a.spike_score < b.spike_score); break;
                            }
                            return asc ? cmp < 0 : cmp > 0;
                        });
                        specs->SpecsDirty = false;
                    }
                }

                for (const auto& m : g_MarketMovers) {
                    const auto* owned_m = FlipOut::HoardBridge::GetOwnedItem(m.item_id);
                    bool is_owned_m = owned_m && owned_m->total_count > 0;
                    if (is_owned_m) {
                        ImGui::TableNextRow(0, ICON_SIZE + 4);
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                            IM_COL32(180, 140, 20, 40));
                    } else {
                        ImGui::TableNextRow(0, ICON_SIZE + 4);
                    }
                    ImGui::PushID((int)m.item_id);

                    // Invisible selectable spanning all columns for row right-click
                    ImGui::TableNextColumn();
                    ImGui::Selectable("##row", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                        ImVec2(0, ICON_SIZE));
                    if (ImGui::IsItemClicked(0)) {
                        OpenPriceHistory(m.item_id, m.name, m.rarity);
                    }
                    if (ImGui::BeginPopupContextItem("##mover_ctx")) {
                        bool watched = FlipOut::PriceDB::IsOnWatchlist(m.item_id);
                        if (watched) {
                            if (ImGui::MenuItem("Remove from Watchlist")) {
                                FlipOut::PriceDB::RemoveFromWatchlist(m.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        } else {
                            if (ImGui::MenuItem("Add to Watchlist")) {
                                FlipOut::PriceDB::AddToWatchlist(m.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        }
                        if (ImGui::MenuItem("Search in Hoard & Seek")) {
                            FlipOut::HoardBridge::SearchInHoard(m.name);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("View Price History")) {
                            OpenPriceHistory(m.item_id, m.name, m.rarity);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine(0, 0);

                    // Item name + icon
                    const auto* info = FlipOut::TPAPI::GetItemInfo(m.item_id);
                    std::string iconUrl = info ? info->icon_url : "";
                    RenderItemIcon(m.item_id, iconUrl, m.rarity);
                    ImGui::SameLine();
                    ImGui::TextColored(GetRarityColor(m.rarity), "%s", m.name.c_str());
                    if (is_owned_m) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), " [%d owned]", owned_m->total_count);
                    }

                    // Current sell price
                    ImGui::TableNextColumn();
                    RenderCoins(m.current_sell);

                    // Historical avg sell
                    ImGui::TableNextColumn();
                    RenderCoins(m.avg_sell);

                    // Price change %
                    ImGui::TableNextColumn();
                    ImVec4 priceCol = m.price_change_pct >= 50.0f
                        ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f)
                        : ImVec4(1.0f, 0.7f, 0.2f, 1.0f);
                    ImGui::TextColored(priceCol, "+%.0f%%", m.price_change_pct);

                    // Buy volume (current / avg)
                    ImGui::TableNextColumn();
                    ImGui::Text("%d/%d", m.current_buy_qty, m.avg_buy_qty);

                    // Volume change %
                    ImGui::TableNextColumn();
                    if (m.volume_change_pct > 0) {
                        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                            "+%.0f%%", m.volume_change_pct);
                    } else {
                        ImGui::Text("%.0f%%", m.volume_change_pct);
                    }

                    // Spike score
                    ImGui::TableNextColumn();
                    ImGui::Text("%.0f", m.spike_score);

                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    // --- Flip opportunities ---
    if (g_Flips.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "No flips found. Click 'Scan Market' to fetch prices.");
        return;
    }

    ImGui::Text("%d opportunities found", (int)g_Flips.size());

    if (ImGui::BeginTable("##flips", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable,
        ImVec2(0, ImGui::GetContentRegionAvail().y - 4)))
    {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Buy", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Sell", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Profit", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 80.0f);
        ImGui::TableSetupColumn("Margin", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("ROI", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                const auto& s = specs->Specs[0];
                bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                std::sort(g_Flips.begin(), g_Flips.end(), [&](const FlipOut::FlipOpportunity& a, const FlipOut::FlipOpportunity& b) {
                    int cmp = 0;
                    switch (s.ColumnIndex) {
                        case 0: cmp = a.name.compare(b.name); break;
                        case 1: cmp = a.buy_price - b.buy_price; break;
                        case 2: cmp = a.sell_price - b.sell_price; break;
                        case 3: cmp = a.profit_per_unit - b.profit_per_unit; break;
                        case 4: cmp = (a.margin_pct > b.margin_pct) - (a.margin_pct < b.margin_pct); break;
                        case 5: cmp = (a.roi > b.roi) - (a.roi < b.roi); break;
                        case 6: cmp = (a.buy_quantity + a.sell_quantity) - (b.buy_quantity + b.sell_quantity); break;
                    }
                    return asc ? cmp < 0 : cmp > 0;
                });
                specs->SpecsDirty = false;
            }
        }

        for (const auto& flip : g_Flips) {
            const auto* owned_f = FlipOut::HoardBridge::GetOwnedItem(flip.item_id);
            bool is_owned_f = owned_f && owned_f->total_count > 0;
            if (is_owned_f) {
                ImGui::TableNextRow(0, ICON_SIZE + 4);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1,
                    IM_COL32(180, 140, 20, 40));
            } else {
                ImGui::TableNextRow(0, ICON_SIZE + 4);
            }
            ImGui::PushID((int)flip.item_id);

            // Invisible selectable spanning all columns for row right-click
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
            if (ImGui::IsItemClicked(0)) {
                OpenPriceHistory(flip.item_id, flip.name, flip.rarity);
            }
            if (ImGui::BeginPopupContextItem("##flip_ctx")) {
                bool watched = FlipOut::PriceDB::IsOnWatchlist(flip.item_id);
                if (watched) {
                    if (ImGui::MenuItem("Remove from Watchlist")) {
                        FlipOut::PriceDB::RemoveFromWatchlist(flip.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                } else {
                    if (ImGui::MenuItem("Add to Watchlist")) {
                        FlipOut::PriceDB::AddToWatchlist(flip.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                }
                if (ImGui::MenuItem("Search in Hoard & Seek")) {
                    FlipOut::HoardBridge::SearchInHoard(flip.name);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("View Price History")) {
                    OpenPriceHistory(flip.item_id, flip.name, flip.rarity);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);

            // Item name + icon
            const auto* info = FlipOut::TPAPI::GetItemInfo(flip.item_id);
            std::string iconUrl = info ? info->icon_url : "";
            RenderItemIcon(flip.item_id, iconUrl, flip.rarity);
            ImGui::SameLine();
            ImGui::TextColored(GetRarityColor(flip.rarity), "%s", flip.name.c_str());
            if (is_owned_f) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), " [%d owned]", owned_f->total_count);
            }

            // Buy price
            ImGui::TableNextColumn();
            RenderCoins(flip.buy_price);

            // Sell price
            ImGui::TableNextColumn();
            RenderCoins(flip.sell_price);

            // Profit
            ImGui::TableNextColumn();
            RenderCoins(flip.profit_per_unit);

            // Margin %
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", flip.margin_pct);

            // ROI %
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", flip.roi);

            // Volume
            ImGui::TableNextColumn();
            ImGui::Text("%d/%d", flip.buy_quantity, flip.sell_quantity);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// --- Watchlist Tab ---

static void RenderWatchlistTab() {
    auto& watchlist = FlipOut::PriceDB::GetWatchlist();

    if (watchlist.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Watchlist is empty. Add items from the Flips or Search tabs.");
        return;
    }

    // Refresh watchlist prices button
    auto fetchStatus = FlipOut::TPAPI::GetBulkFetchStatus();
    bool scanning = (fetchStatus == FlipOut::FetchStatus::InProgress);
    if (scanning) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    if (ImGui::Button("Refresh Prices") && !scanning) {
        // Fetch just watchlist item prices
        std::vector<uint32_t> ids;
        for (const auto& e : watchlist) ids.push_back(e.item_id);
        std::thread([ids]() {
            auto prices = FlipOut::TPAPI::FetchPrices(ids);
            // Update prices map
            auto allPrices = FlipOut::TPAPI::GetAllPrices();
            std::unordered_map<uint32_t, FlipOut::PriceSnapshot> snaps;
            for (const auto& p : prices) {
                FlipOut::TPPrice tp;
                tp.item_id = p.item_id;
                tp.buy_price = p.buy_price;
                tp.sell_price = p.sell_price;
                tp.buy_quantity = p.buy_quantity;
                tp.sell_quantity = p.sell_quantity;
                allPrices[p.item_id] = tp;

                FlipOut::PriceSnapshot s;
                s.buy_price = p.buy_price;
                s.sell_price = p.sell_price;
                s.buy_quantity = p.buy_quantity;
                s.sell_quantity = p.sell_quantity;
                snaps[p.item_id] = s;
            }
            FlipOut::PriceDB::RecordBulkPrices(snaps);
            FlipOut::PriceDB::Save();
        }).detach();
    }
    if (scanning) ImGui::PopStyleVar();

    ImGui::SameLine();
    ImGui::Text("%d items watched", (int)watchlist.size());
    ImGui::Separator();

    if (ImGui::BeginTable("##watchlist", 9,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable,
        ImVec2(0, ImGui::GetContentRegionAvail().y - 4)))
    {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Buy", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Sell", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Profit", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Margin", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Buy Trend", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Sell Trend", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("History", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 30.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        // Copy watchlist to iterate safely
        auto wl_copy = watchlist;
        uint32_t removeId = 0;

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                const auto& s = specs->Specs[0];
                bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                std::sort(wl_copy.begin(), wl_copy.end(), [&](const FlipOut::WatchlistEntry& a, const FlipOut::WatchlistEntry& b) {
                    const auto* ai = FlipOut::TPAPI::GetItemInfo(a.item_id);
                    const auto* bi = FlipOut::TPAPI::GetItemInfo(b.item_id);
                    FlipOut::TPPrice pa{}, pb{};
                    FlipOut::TPAPI::GetPrice(a.item_id, pa);
                    FlipOut::TPAPI::GetPrice(b.item_id, pb);
                    int ab = pa.buy_price;
                    int as_ = pa.sell_price;
                    int bb_ = pb.buy_price;
                    int bs = pb.sell_price;
                    int cmp = 0;
                    switch (s.ColumnIndex) {
                        case 0: { std::string an = ai ? ai->name : ""; std::string bn = bi ? bi->name : ""; cmp = an.compare(bn); } break;
                        case 1: cmp = ab - bb_; break;
                        case 2: cmp = as_ - bs; break;
                        case 3: { int ap = (ab > 0 && as_ > 0) ? FlipOut::Analyzer::CalcProfit(ab, as_) : 0;
                                  int bp = (bb_ > 0 && bs > 0) ? FlipOut::Analyzer::CalcProfit(bb_, bs) : 0;
                                  cmp = ap - bp; } break;
                        case 4: { float am = (ab > 0 && as_ > 0) ? FlipOut::Analyzer::CalcMargin(ab, as_) : 0;
                                  float bm = (bb_ > 0 && bs > 0) ? FlipOut::Analyzer::CalcMargin(bb_, bs) : 0;
                                  cmp = (am > bm) - (am < bm); } break;
                    }
                    return asc ? cmp < 0 : cmp > 0;
                });
                specs->SpecsDirty = false;
            }
        }

        for (const auto& entry : wl_copy) {
            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)entry.item_id);

            const auto* info = FlipOut::TPAPI::GetItemInfo(entry.item_id);
            std::string name = info ? info->name : ("Item #" + std::to_string(entry.item_id));
            std::string rarity = info ? info->rarity : "";
            std::string iconUrl = info ? info->icon_url : "";

            FlipOut::TPPrice tp{};
            FlipOut::TPAPI::GetPrice(entry.item_id, tp);
            int buy = tp.buy_price;
            int sell = tp.sell_price;

            // Item
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
            if (ImGui::IsItemClicked(0)) {
                OpenPriceHistory(entry.item_id, name, rarity);
            }
            if (ImGui::BeginPopupContextItem("##wl_ctx")) {
                if (ImGui::MenuItem("Search in Hoard & Seek")) {
                    FlipOut::HoardBridge::SearchInHoard(name);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("View Price History")) {
                    OpenPriceHistory(entry.item_id, name, rarity);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);
            RenderItemIcon(entry.item_id, iconUrl, rarity);
            ImGui::SameLine();
            ImGui::TextColored(GetRarityColor(rarity), "%s", name.c_str());

            // Buy
            ImGui::TableNextColumn();
            if (buy > 0) RenderCoins(buy);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // Sell
            ImGui::TableNextColumn();
            if (sell > 0) RenderCoins(sell);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // Profit
            ImGui::TableNextColumn();
            if (buy > 0 && sell > 0) {
                int profit = FlipOut::Analyzer::CalcProfit(buy, sell);
                RenderCoins(profit);
            } else {
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");
            }

            // Margin
            ImGui::TableNextColumn();
            if (buy > 0 && sell > 0) {
                float margin = FlipOut::Analyzer::CalcMargin(buy, sell);
                ImGui::Text("%.1f%%", margin);
            } else {
                ImGui::Text("---");
            }

            // Trends
            auto trend = FlipOut::Analyzer::GetTrend(entry.item_id);

            ImGui::TableNextColumn();
            RenderTrend(trend.buy_trend);
            if (ImGui::IsItemHovered() && trend.data_points > 1) {
                ImGui::BeginTooltip();
                ImGui::Text("Buy: %+d%% (%d pts)", trend.buy_change_pct, trend.data_points);
                ImGui::EndTooltip();
            }

            ImGui::TableNextColumn();
            RenderTrend(trend.sell_trend);
            if (ImGui::IsItemHovered() && trend.data_points > 1) {
                ImGui::BeginTooltip();
                ImGui::Text("Sell: %+d%% (%d pts)", trend.sell_change_pct, trend.data_points);
                ImGui::EndTooltip();
            }

            // History count
            ImGui::TableNextColumn();
            ImGui::Text("%d", trend.data_points);

            // Remove button
            ImGui::TableNextColumn();
            if (ImGui::SmallButton("X")) {
                removeId = entry.item_id;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();

        if (removeId != 0) {
            FlipOut::PriceDB::RemoveFromWatchlist(removeId);
            FlipOut::PriceDB::SaveWatchlist();
        }
    }
}

// --- Transactions Tab (via Hoard & Seek API proxy) ---

static int g_TxLastAccountIndex = -2; // track account changes (-2 = never loaded)

static void RenderTransactionsTab() {
    if (!FlipOut::HoardBridge::IsDataAvailable()) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
            "Hoard & Seek not connected. Transactions require Hoard & Seek for API access.");
        if (ImGui::Button("Retry Connection")) {
            FlipOut::HoardBridge::Ping();
        }
        return;
    }

    bool txLoading = FlipOut::HoardBridge::IsTransactionsLoading();
    bool txLoaded = FlipOut::HoardBridge::IsTransactionsLoaded();

    // Auto-load on first view, or reload on account change
    int curAcctIdx = FlipOut::HoardBridge::GetActiveAccountIndex();
    if (!txLoading && (!txLoaded || curAcctIdx != g_TxLastAccountIndex)) {
        g_TxLastAccountIndex = curAcctIdx;
        FlipOut::HoardBridge::FetchTransactions();
        txLoading = true;
    }

    if (txLoading) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    if (ImGui::Button("Refresh Transactions") && !txLoading) {
        FlipOut::HoardBridge::FetchTransactions();
    }
    if (txLoading) ImGui::PopStyleVar();

    if (txLoading) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Loading via Hoard & Seek...");
    }

    if (!txLoaded) return;

    ImGui::Separator();

    auto currentBuys = FlipOut::HoardBridge::GetCurrentBuys();
    auto currentSells = FlipOut::HoardBridge::GetCurrentSells();

    // Pending Buys
    if (ImGui::CollapsingHeader("Pending Buy Orders", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (currentBuys.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No pending buy orders.");
        } else {
            if (ImGui::BeginTable("##txbuys", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_Sortable))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 80.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Created", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
                    if (specs->SpecsDirty && specs->SpecsCount > 0) {
                        const auto& s = specs->Specs[0];
                        bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                        std::sort(currentBuys.begin(), currentBuys.end(), [&](const FlipOut::TPTransaction& a, const FlipOut::TPTransaction& b) {
                            int cmp = 0;
                            switch (s.ColumnIndex) {
                                case 0: { const auto* ai = FlipOut::TPAPI::GetItemInfo(a.item_id); const auto* bi = FlipOut::TPAPI::GetItemInfo(b.item_id);
                                          std::string an = ai ? ai->name : ""; std::string bn = bi ? bi->name : "";
                                          cmp = an.compare(bn); } break;
                                case 1: cmp = a.price - b.price; break;
                                case 2: cmp = a.quantity - b.quantity; break;
                                case 3: cmp = a.created.compare(b.created); break;
                            }
                            return asc ? cmp < 0 : cmp > 0;
                        });
                        specs->SpecsDirty = false;
                    }
                }

                for (const auto& tx : currentBuys) {
                    ImGui::TableNextRow();
                    ImGui::PushID((int)tx.item_id ^ 0x42555900);
                    ImGui::TableNextColumn();
                    const auto* info = FlipOut::TPAPI::GetItemInfo(tx.item_id);
                    std::string name = info ? info->name : ("Item #" + std::to_string(tx.item_id));
                    std::string rarity = info ? info->rarity : "";
                    ImGui::Selectable("##row", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
                    if (ImGui::IsItemClicked(0)) {
                        OpenPriceHistory(tx.item_id, name, rarity);
                    }
                    if (ImGui::BeginPopupContextItem("##txb_ctx")) {
                        bool watched = FlipOut::PriceDB::IsOnWatchlist(tx.item_id);
                        if (watched) {
                            if (ImGui::MenuItem("Remove from Watchlist")) {
                                FlipOut::PriceDB::RemoveFromWatchlist(tx.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        } else {
                            if (ImGui::MenuItem("Add to Watchlist")) {
                                FlipOut::PriceDB::AddToWatchlist(tx.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        }
                        if (ImGui::MenuItem("Search in Hoard & Seek")) {
                            FlipOut::HoardBridge::SearchInHoard(name);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(GetRarityColor(rarity), "%s", name.c_str());

                    ImGui::TableNextColumn();
                    RenderCoins(tx.price);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", tx.quantity);

                    ImGui::TableNextColumn();
                    if (tx.created.size() >= 10)
                        ImGui::Text("%s", tx.created.substr(0, 10).c_str());
                    else
                        ImGui::Text("---");
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }

    // Pending Sells
    if (ImGui::CollapsingHeader("Pending Sell Listings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (currentSells.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No pending sell listings.");
        } else {
            if (ImGui::BeginTable("##txsells", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                ImGuiTableFlags_Sortable))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 80.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Created", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
                    if (specs->SpecsDirty && specs->SpecsCount > 0) {
                        const auto& s = specs->Specs[0];
                        bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                        std::sort(currentSells.begin(), currentSells.end(), [&](const FlipOut::TPTransaction& a, const FlipOut::TPTransaction& b) {
                            int cmp = 0;
                            switch (s.ColumnIndex) {
                                case 0: { const auto* ai = FlipOut::TPAPI::GetItemInfo(a.item_id); const auto* bi = FlipOut::TPAPI::GetItemInfo(b.item_id);
                                          std::string an = ai ? ai->name : ""; std::string bn = bi ? bi->name : "";
                                          cmp = an.compare(bn); } break;
                                case 1: cmp = a.price - b.price; break;
                                case 2: cmp = a.quantity - b.quantity; break;
                                case 3: cmp = a.created.compare(b.created); break;
                            }
                            return asc ? cmp < 0 : cmp > 0;
                        });
                        specs->SpecsDirty = false;
                    }
                }

                for (const auto& tx : currentSells) {
                    ImGui::TableNextRow();
                    ImGui::PushID((int)tx.item_id ^ 0x53454C00);
                    ImGui::TableNextColumn();
                    const auto* info = FlipOut::TPAPI::GetItemInfo(tx.item_id);
                    std::string name = info ? info->name : ("Item #" + std::to_string(tx.item_id));
                    std::string rarity = info ? info->rarity : "";
                    ImGui::Selectable("##row", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap);
                    if (ImGui::IsItemClicked(0)) {
                        OpenPriceHistory(tx.item_id, name, rarity);
                    }
                    if (ImGui::BeginPopupContextItem("##txs_ctx")) {
                        bool watched = FlipOut::PriceDB::IsOnWatchlist(tx.item_id);
                        if (watched) {
                            if (ImGui::MenuItem("Remove from Watchlist")) {
                                FlipOut::PriceDB::RemoveFromWatchlist(tx.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        } else {
                            if (ImGui::MenuItem("Add to Watchlist")) {
                                FlipOut::PriceDB::AddToWatchlist(tx.item_id);
                                FlipOut::PriceDB::SaveWatchlist();
                            }
                        }
                        if (ImGui::MenuItem("Search in Hoard & Seek")) {
                            FlipOut::HoardBridge::SearchInHoard(name);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine(0, 0);
                    ImGui::TextColored(GetRarityColor(rarity), "%s", name.c_str());

                    ImGui::TableNextColumn();
                    RenderCoins(tx.price);

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", tx.quantity);

                    ImGui::TableNextColumn();
                    if (tx.created.size() >= 10)
                        ImGui::Text("%s", tx.created.substr(0, 10).c_str());
                    else
                        ImGui::Text("---");
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
}

// --- Crafting Tab (crafting profit opportunities) ---

static void RenderCraftingTab() {
    auto recipeStatus = FlipOut::TPAPI::GetRecipeFetchStatus();

    // Show recipe loading status if not loaded
    if (recipeStatus == FlipOut::FetchStatus::Idle && FlipOut::TPAPI::GetRecipeCount() == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
            "Recipe data not loaded. Click 'Load Recipes' to fetch crafting data from the GW2 API.");
        if (ImGui::Button("Load Recipes")) {
            FlipOut::TPAPI::FetchAllRecipesAsync();
        }
        return;
    }

    if (recipeStatus == FlipOut::FetchStatus::InProgress) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s",
            FlipOut::TPAPI::GetRecipeFetchMessage().c_str());
        return;
    }

    if (recipeStatus == FlipOut::FetchStatus::Error) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s",
            FlipOut::TPAPI::GetRecipeFetchMessage().c_str());
        if (ImGui::Button("Retry")) {
            FlipOut::TPAPI::FetchAllRecipesAsync();
        }
        return;
    }

    // Query H&S for recipe unlocks and ingredient ownership once we have crafting results
    if (!g_CraftingProfits.empty() && FlipOut::HoardBridge::IsDataAvailable()) {
        if (!g_CraftingRecipesQueried) {
            std::vector<uint32_t> recipeIds;
            recipeIds.reserve(g_CraftingProfits.size());
            for (const auto& cp : g_CraftingProfits) {
                if (cp.recipe_id > 0) recipeIds.push_back(cp.recipe_id);
            }
            if (!recipeIds.empty()) {
                FlipOut::HoardBridge::QueryRecipeUnlocks(recipeIds);
                g_CraftingRecipesQueried = true;
            }
        }
        if (!g_CraftingIngredientsQueried) {
            std::unordered_set<uint32_t> ingIds;
            for (const auto& cp : g_CraftingProfits) {
                for (const auto& [ing_id, _] : cp.ingredients) {
                    ingIds.insert(ing_id);
                }
            }
            std::vector<uint32_t> ids(ingIds.begin(), ingIds.end());
            if (!ids.empty()) {
                FlipOut::HoardBridge::QueryItems(ids);
            }
            g_CraftingIngredientsQueried = true;
        }
    }

    bool hasUnlockData = FlipOut::HoardBridge::IsRecipeQueryDone();

    // Status line with mode radio buttons
    ImGui::Text("Recipes: %d", (int)FlipOut::TPAPI::GetRecipeCount());
    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();
    ImGui::Text("Results: %d", (int)g_CraftingProfits.size());
    if (hasUnlockData) {
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Known: %d",
            (int)FlipOut::HoardBridge::GetUnlockedRecipeCount());
    } else if (g_CraftingRecipesQueried) {
        ImGui::SameLine();
        ImGui::Text(" | ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Checking unlocks...");
    }
    ImGui::SameLine();
    ImGui::Text(" | ");
    ImGui::SameLine();

    bool filterChanged = false;
    if (ImGui::RadioButton("Fastest", g_CraftingMode == 0)) {
        g_CraftingMode = 0;
        filterChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Balanced", g_CraftingMode == 1)) {
        g_CraftingMode = 1;
        filterChanged = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Patient", g_CraftingMode == 2)) {
        g_CraftingMode = 2;
        filterChanged = true;
    }
    if (hasUnlockData) {
        ImGui::SameLine();
        if (ImGui::Checkbox("Known only", &g_CraftingKnownOnly)) {
            filterChanged = true;
        }
    }

    if (filterChanged) g_CraftingDirty = true;

    // Pick up completed async crafting results
    if (g_CraftingPendingReady) {
        std::lock_guard<std::mutex> lock(g_AnalysisMutex);
        g_CraftingProfits = std::move(g_CraftingPending);
        g_CraftingPendingReady = false;
        g_CraftingRecipesQueried = false;
        g_CraftingIngredientsQueried = false;
    }

    if (g_CraftingDirty && !g_AnalysisBusy) {
        g_CraftingDirty = false;
        g_AnalysisBusy = true;
        FlipOut::CraftingFilter filter;
        filter.mode = g_CraftingMode;
        g_CraftingFilter = filter;
        std::thread([filter]() {
            auto result = FlipOut::Analyzer::FindCraftingProfits(filter);
            {
                std::lock_guard<std::mutex> lock(g_AnalysisMutex);
                g_CraftingPending = std::move(result);
            }
            g_CraftingPendingReady = true;
            g_AnalysisBusy = false;
        }).detach();
    }

    if (g_CraftingProfits.empty()) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "No profitable crafts found. Try lowering the filters or run a market scan first.");
        return;
    }

    ImGui::Separator();

    if (ImGui::BeginTable("##crafting", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable,
        ImVec2(0, 0)))
    {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("Ingredients", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Sell Price", ImGuiTableColumnFlags_WidthFixed, 85.0f);
        ImGui::TableSetupColumn("Profit", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 85.0f);
        ImGui::TableSetupColumn("ROI", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                const auto& s = specs->Specs[0];
                bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                std::sort(g_CraftingProfits.begin(), g_CraftingProfits.end(), [&](const FlipOut::CraftingProfit& a, const FlipOut::CraftingProfit& b) {
                    int cmp = 0;
                    switch (s.ColumnIndex) {
                        case 0: cmp = a.name.compare(b.name); break;
                        case 1: cmp = a.recipe_type.compare(b.recipe_type); break;
                        case 2: cmp = a.ingredient_cost - b.ingredient_cost; break;
                        case 3: cmp = a.sell_price - b.sell_price; break;
                        case 4: cmp = a.profit - b.profit; break;
                        case 5: cmp = (a.roi > b.roi) - (a.roi < b.roi); break;
                        case 6: cmp = a.sell_quantity - b.sell_quantity; break;
                    }
                    return asc ? cmp < 0 : cmp > 0;
                });
                specs->SpecsDirty = false;
            }
        }

        for (const auto& cp : g_CraftingProfits) {
            // Filter by known recipes if checkbox enabled
            if (g_CraftingKnownOnly && hasUnlockData && cp.recipe_id > 0) {
                if (!FlipOut::HoardBridge::IsRecipeUnlocked(cp.recipe_id)) continue;
            }

            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)cp.item_id);

            // Invisible selectable spanning all columns for row right-click
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
            if (ImGui::IsItemClicked(0)) {
                OpenPriceHistory(cp.item_id, cp.name, cp.rarity);
            }
            if (ImGui::BeginPopupContextItem("##craft_ctx")) {
                bool watched = FlipOut::PriceDB::IsOnWatchlist(cp.item_id);
                if (watched) {
                    if (ImGui::MenuItem("Remove from Watchlist")) {
                        FlipOut::PriceDB::RemoveFromWatchlist(cp.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                } else {
                    if (ImGui::MenuItem("Add to Watchlist")) {
                        FlipOut::PriceDB::AddToWatchlist(cp.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                }
                if (ImGui::MenuItem("Search in Hoard & Seek")) {
                    FlipOut::HoardBridge::SearchInHoard(cp.name);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("View Price History")) {
                    OpenPriceHistory(cp.item_id, cp.name, cp.rarity);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);

            // Item name + icon
            const auto* info = FlipOut::TPAPI::GetItemInfo(cp.item_id);
            std::string iconUrl = info ? info->icon_url : "";
            RenderItemIcon(cp.item_id, iconUrl, cp.rarity);
            ImGui::SameLine();

            // Show unlock indicator if H&S data available
            if (hasUnlockData && cp.recipe_id > 0) {
                if (FlipOut::HoardBridge::IsRecipeUnlocked(cp.recipe_id)) {
                    ImGui::TextColored(ImVec4(0.4f, 0.85f, 0.4f, 1.0f), "*");
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Recipe known");
                        ImGui::EndTooltip();
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "?");
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Recipe not unlocked");
                        ImGui::EndTooltip();
                    }
                }
                ImGui::SameLine();
            }

            ImGui::TextColored(GetRarityColor(cp.rarity), "%s", cp.name.c_str());
            if (cp.output_count > 1) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), " x%d", cp.output_count);
            }

            // Tooltip with ingredient details + owned counts
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Ingredients:");
                bool hsAvail = FlipOut::HoardBridge::IsDataAvailable();
                int owned_savings = 0;
                for (const auto& [ing_id, ing_count] : cp.ingredients) {
                    const auto* ing_info = FlipOut::TPAPI::GetItemInfo(ing_id);
                    std::string ing_name = ing_info ? ing_info->name : ("Item #" + std::to_string(ing_id));
                    if (hsAvail) {
                        const auto* owned = FlipOut::HoardBridge::GetOwnedItem(ing_id);
                        int have = owned ? owned->total_count : 0;
                        if (have >= ing_count) {
                            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f),
                                "  %dx %s [%d owned]", ing_count, ing_name.c_str(), have);
                            FlipOut::TPPrice ing_tp{};
                            FlipOut::TPAPI::GetPrice(ing_id, ing_tp);
                            int ing_unit = g_CraftingMode == 2 ? ing_tp.buy_price : ing_tp.sell_price;
                            owned_savings += ing_unit * ing_count;
                        } else if (have > 0) {
                            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f),
                                "  %dx %s [%d/%d owned]", ing_count, ing_name.c_str(), have, ing_count);
                            FlipOut::TPPrice ing_tp{};
                            FlipOut::TPAPI::GetPrice(ing_id, ing_tp);
                            int ing_unit = g_CraftingMode == 2 ? ing_tp.buy_price : ing_tp.sell_price;
                            owned_savings += ing_unit * have;
                        } else {
                            ImGui::Text("  %dx %s", ing_count, ing_name.c_str());
                        }
                    } else {
                        ImGui::Text("  %dx %s", ing_count, ing_name.c_str());
                    }
                }
                ImGui::Separator();
                ImGui::Text("Cost: %s", FlipOut::Analyzer::FormatCoins(cp.ingredient_cost).c_str());
                if (hsAvail && owned_savings > 0) {
                    int adjusted = cp.ingredient_cost - owned_savings;
                    if (adjusted < 0) adjusted = 0;
                    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f),
                        "Cost (with owned): %s", FlipOut::Analyzer::FormatCoins(adjusted).c_str());
                    int adjusted_profit = cp.sell_revenue - adjusted;
                    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f),
                        "Adjusted profit: %s", FlipOut::Analyzer::FormatCoins(adjusted_profit).c_str());
                }
                ImGui::Text("Revenue: %s (after tax)", FlipOut::Analyzer::FormatCoins(cp.sell_revenue).c_str());
                ImGui::EndTooltip();
            }

            // Recipe type
            ImGui::TableNextColumn();
            ImGui::Text("%s", cp.recipe_type.c_str());

            // Ingredient cost
            ImGui::TableNextColumn();
            RenderCoins(cp.ingredient_cost);

            // Sell price
            ImGui::TableNextColumn();
            RenderCoins(cp.sell_price);

            // Profit
            ImGui::TableNextColumn();
            RenderCoins(cp.profit);

            // ROI
            ImGui::TableNextColumn();
            ImGui::Text("%.1f%%", cp.roi);

            // Volume
            ImGui::TableNextColumn();
            ImGui::Text("%d", cp.sell_quantity);

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// --- Liquidate Tab (sell owned items for fast gold) ---

static void BuildLiquidateList() {
    auto owned = FlipOut::HoardBridge::GetAllOwnedItems();
    auto prices = FlipOut::TPAPI::GetAllPrices();
    std::vector<LiquidateEntry> items;
    items.reserve(owned.size());

    for (const auto& [id, oi] : owned) {
        if (oi.total_count <= 0) continue;
        auto pit = prices.find(id);
        if (pit == prices.end()) continue;
        int bp = pit->second.buy_price;
        int sp = pit->second.sell_price;
        if (bp <= 0 && sp <= 0) continue;

        // Subtract counts from non-sellable locations
        int unsellable = 0;
        for (const auto& [loc, cnt] : oi.locations) {
            // Legendary Armory, equipped gear, and shared inventory slots are not sellable
            if (loc.find("Legendary Armory") != std::string::npos ||
                loc.find("Equipped") != std::string::npos ||
                loc.find("equipped") != std::string::npos) {
                unsellable += cnt;
            }
        }
        int sellable = oi.total_count - unsellable;
        if (sellable <= 0) continue;

        LiquidateEntry e;
        e.item_id = id;
        e.count = sellable;
        e.buy_price = bp;
        e.sell_price = sp;
        e.total_instant = (int)((double)sellable * bp * 0.85);
        e.total_listed = (int)((double)sellable * sp * 0.85);

        const auto* info = FlipOut::TPAPI::GetItemInfo(id);
        e.name = info ? info->name : oi.name;
        e.rarity = info ? info->rarity : oi.rarity;
        e.icon_url = info ? info->icon_url : "";
        items.push_back(std::move(e));
    }

    // Sort by total instant value descending
    std::stable_sort(items.begin(), items.end(),
        [](const LiquidateEntry& a, const LiquidateEntry& b) {
            return a.total_instant > b.total_instant;
        });

    // Keep top 50 results
    if (items.size() > 50) items.resize(50);

    g_LiquidateItems = std::move(items);
    g_LiquidateLoaded = true;
    g_LiquidateLoading = false;
}

static void FetchLiquidateData() {
    g_LiquidateLoading = true;
    g_LiquidateLoaded = false;

    // Query all tradeable item IDs via H&S (uses its local account cache)
    auto prices = FlipOut::TPAPI::GetAllPrices();
    std::vector<uint32_t> ids;
    ids.reserve(prices.size());
    for (const auto& [id, _] : prices) ids.push_back(id);
    FlipOut::HoardBridge::QueryItems(ids);

    // Record start time; we'll build the list after a short delay to let H&S respond
    g_LiquidateScanStart = std::chrono::steady_clock::now();
}

static int g_LiqLastAccountIndex = -2; // track account changes (-2 = never loaded)

static void RenderLiquidateTab() {
    bool hsAvail = FlipOut::HoardBridge::IsDataAvailable();

    if (!hsAvail) {
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
            "Hoard & Seek is not connected. This tab requires H&S for account data.");
        return;
    }

    bool hasPrices = FlipOut::TPAPI::HasPrices();
    if (!hasPrices) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "No price data. Run 'Scan Market' on the Flips tab first.");
        return;
    }

    // Auto-load on first view with prices, or reload on account change
    int curAcctIdx = FlipOut::HoardBridge::GetActiveAccountIndex();
    if (!g_LiquidateLoading && (!g_LiquidateLoaded || curAcctIdx != g_LiqLastAccountIndex)) {
        g_LiqLastAccountIndex = curAcctIdx;
        FetchLiquidateData();
    }

    // Scan button
    if (g_LiquidateLoading) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - g_LiquidateScanStart).count();
        if (elapsed >= 2000) {
            // H&S has had enough time to respond from local cache
            BuildLiquidateList();
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Querying inventory...");
        }
    } else {
        if (ImGui::Button("Refresh Owned Items")) {
            FetchLiquidateData();
        }
        if (g_LiquidateLoaded) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                "Your %d biggest money-makers", (int)g_LiquidateItems.size());
        }
    }

    if (!g_LiquidateLoaded || g_LiquidateItems.empty()) {
        if (g_LiquidateLoaded) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                "No tradeable items found in your inventory.");
        }
        return;
    }

    // Grand total
    long long grand_instant = 0, grand_listed = 0;
    for (const auto& e : g_LiquidateItems) {
        grand_instant += e.total_instant;
        grand_listed += e.total_listed;
    }
    ImGui::Text("Total (instant sell): ");
    ImGui::SameLine(0, 0);
    RenderCoins((int)std::min(grand_instant, (long long)INT_MAX));
    ImGui::SameLine();
    ImGui::Text("  Total (listed): ");
    ImGui::SameLine(0, 0);
    RenderCoins((int)std::min(grand_listed, (long long)INT_MAX));

    ImGui::Separator();

    // Table
    if (ImGui::BeginTable("##liquidate", 7,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable,
        ImVec2(0, ImGui::GetContentRegionAvail().y - 4)))
    {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 45.0f);
        ImGui::TableSetupColumn("Instant Price", ImGuiTableColumnFlags_WidthFixed, 85.0f);
        ImGui::TableSetupColumn("List Price", ImGuiTableColumnFlags_WidthFixed, 85.0f);
        ImGui::TableSetupColumn("Instant Total", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_PreferSortDescending, 95.0f, 4);
        ImGui::TableSetupColumn("Listed Total", ImGuiTableColumnFlags_WidthFixed, 95.0f);
        ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        // Sort
        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                const auto& s = specs->Specs[0];
                bool asc = (s.SortDirection == ImGuiSortDirection_Ascending);
                int col = s.ColumnIndex;
                std::stable_sort(g_LiquidateItems.begin(), g_LiquidateItems.end(),
                    [col, asc](const LiquidateEntry& a, const LiquidateEntry& b) {
                        int cmp = 0;
                        switch (col) {
                            case 0: cmp = a.name.compare(b.name); break;
                            case 1: cmp = a.count - b.count; break;
                            case 2: cmp = a.buy_price - b.buy_price; break;
                            case 3: cmp = a.sell_price - b.sell_price; break;
                            case 4: cmp = a.total_instant - b.total_instant; break;
                            case 5: cmp = a.total_listed - b.total_listed; break;
                            case 6: cmp = a.rarity.compare(b.rarity); break;
                        }
                        return asc ? cmp < 0 : cmp > 0;
                    });
                specs->SpecsDirty = false;
            }
        }

        for (const auto& e : g_LiquidateItems) {
            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)e.item_id);

            // Item
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
            if (ImGui::IsItemClicked(0)) {
                OpenPriceHistory(e.item_id, e.name, e.rarity);
            }
            if (ImGui::BeginPopupContextItem("##liq_ctx")) {
                bool watched = FlipOut::PriceDB::IsOnWatchlist(e.item_id);
                if (watched) {
                    if (ImGui::MenuItem("Remove from Watchlist")) {
                        FlipOut::PriceDB::RemoveFromWatchlist(e.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                } else {
                    if (ImGui::MenuItem("Add to Watchlist")) {
                        FlipOut::PriceDB::AddToWatchlist(e.item_id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                }
                if (ImGui::MenuItem("Search in Hoard & Seek")) {
                    FlipOut::HoardBridge::SearchInHoard(e.name);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("View Price History")) {
                    OpenPriceHistory(e.item_id, e.name, e.rarity);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);
            RenderItemIcon(e.item_id, e.icon_url, e.rarity);
            ImGui::SameLine();
            ImGui::TextColored(GetRarityColor(e.rarity), "%s", e.name.c_str());

            // Qty
            ImGui::TableNextColumn();
            ImGui::Text("%d", e.count);

            // Instant price (buy order)
            ImGui::TableNextColumn();
            if (e.buy_price > 0) RenderCoins(e.buy_price);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // List price (sell listing)
            ImGui::TableNextColumn();
            if (e.sell_price > 0) RenderCoins(e.sell_price);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // Instant total
            ImGui::TableNextColumn();
            if (e.total_instant > 0) RenderCoins(e.total_instant);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // Listed total (after tax)
            ImGui::TableNextColumn();
            if (e.total_listed > 0) RenderCoins(e.total_listed);
            else ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "---");

            // Rarity
            ImGui::TableNextColumn();
            ImGui::TextColored(GetRarityColor(e.rarity), "%s", e.rarity.c_str());

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// --- Search Tab (TP item search, with owned count from Hoard & Seek) ---

static void RenderSearchTab() {
    ImGui::PushItemWidth(-80);
    bool changed = ImGui::InputTextWithHint("##search", "Search TP items by name...",
        g_SearchBuf, sizeof(g_SearchBuf));
    ImGui::PopItemWidth();

    ImGui::SameLine();
    if (ImGui::Button("Search") || (changed && strlen(g_SearchBuf) >= 3)) {
        std::string query(g_SearchBuf);
        if (query.length() >= 3) {
            g_SearchResults = FlipOut::TPAPI::SearchItems(query, 100);
            // Query owned counts from Hoard & Seek for search results
            if (FlipOut::HoardBridge::IsDataAvailable()) {
                std::vector<uint32_t> ids;
                for (const auto& item : g_SearchResults) ids.push_back(item.id);
                FlipOut::HoardBridge::QueryItems(ids);
            }
        } else {
            g_SearchResults.clear();
        }
    }

    if (g_SearchResults.empty()) {
        if (strlen(g_SearchBuf) >= 3) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No items found. Run 'Scan Market' first to populate the item cache.");
        }
        return;
    }

    ImGui::Separator();
    ImGui::Text("%d results", (int)g_SearchResults.size());

    bool hsConnected = FlipOut::HoardBridge::IsDataAvailable();

    int col_count = hsConnected ? 6 : 5;
    if (ImGui::BeginTable("##search_results", col_count,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable,
        ImVec2(0, ImGui::GetContentRegionAvail().y - 4)))
    {
        ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Buy", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Sell", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Profit", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Margin", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        if (hsConnected) {
            ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthFixed, 45.0f);
        }
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                const auto& s = specs->Specs[0];
                bool asc = s.SortDirection == ImGuiSortDirection_Ascending;
                std::sort(g_SearchResults.begin(), g_SearchResults.end(), [&](const FlipOut::ItemInfo& a, const FlipOut::ItemInfo& b) {
                    FlipOut::TPPrice pa{}, pb{};
                    FlipOut::TPAPI::GetPrice(a.id, pa);
                    FlipOut::TPAPI::GetPrice(b.id, pb);
                    int ab = pa.buy_price;
                    int as_ = pa.sell_price;
                    int bb_ = pb.buy_price;
                    int bs = pb.sell_price;
                    int cmp = 0;
                    switch (s.ColumnIndex) {
                        case 0: cmp = a.name.compare(b.name); break;
                        case 1: cmp = ab - bb_; break;
                        case 2: cmp = as_ - bs; break;
                        case 3: { int ap = (ab > 0 && as_ > 0) ? FlipOut::Analyzer::CalcProfit(ab, as_) : 0;
                                  int bp = (bb_ > 0 && bs > 0) ? FlipOut::Analyzer::CalcProfit(bb_, bs) : 0;
                                  cmp = ap - bp; } break;
                        case 4: { float am = (ab > 0 && as_ > 0) ? FlipOut::Analyzer::CalcMargin(ab, as_) : 0;
                                  float bm = (bb_ > 0 && bs > 0) ? FlipOut::Analyzer::CalcMargin(bb_, bs) : 0;
                                  cmp = (am > bm) - (am < bm); } break;
                        case 5: { const auto* ao = FlipOut::HoardBridge::GetOwnedItem(a.id);
                                  const auto* bo = FlipOut::HoardBridge::GetOwnedItem(b.id);
                                  int ac = ao ? ao->total_count : 0; int bc = bo ? bo->total_count : 0;
                                  cmp = ac - bc; } break;
                    }
                    return asc ? cmp < 0 : cmp > 0;
                });
                specs->SpecsDirty = false;
            }
        }

        for (const auto& item : g_SearchResults) {
            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)item.id);

            // Invisible selectable spanning all columns for row right-click
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
            if (ImGui::IsItemClicked(0)) {
                OpenPriceHistory(item.id, item.name, item.rarity);
            }
            if (ImGui::BeginPopupContextItem("##search_ctx")) {
                bool watched = FlipOut::PriceDB::IsOnWatchlist(item.id);
                if (watched) {
                    if (ImGui::MenuItem("Remove from Watchlist")) {
                        FlipOut::PriceDB::RemoveFromWatchlist(item.id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                } else {
                    if (ImGui::MenuItem("Add to Watchlist")) {
                        FlipOut::PriceDB::AddToWatchlist(item.id);
                        FlipOut::PriceDB::SaveWatchlist();
                    }
                }
                if (ImGui::MenuItem("Search in Hoard & Seek")) {
                    FlipOut::HoardBridge::SearchInHoard(item.name);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("View Price History")) {
                    OpenPriceHistory(item.id, item.name, item.rarity);
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);

            RenderItemIcon(item.id, item.icon_url, item.rarity);
            ImGui::SameLine();
            ImGui::TextColored(GetRarityColor(item.rarity), "%s", item.name.c_str());

            FlipOut::TPPrice pit{};
            FlipOut::TPAPI::GetPrice(item.id, pit);
            int buy = pit.buy_price;
            int sell = pit.sell_price;

            ImGui::TableNextColumn();
            if (buy > 0) RenderCoins(buy); else ImGui::Text("---");

            ImGui::TableNextColumn();
            if (sell > 0) RenderCoins(sell); else ImGui::Text("---");

            ImGui::TableNextColumn();
            if (buy > 0 && sell > 0) {
                int profit = FlipOut::Analyzer::CalcProfit(buy, sell);
                RenderCoins(profit);
            } else {
                ImGui::Text("---");
            }

            ImGui::TableNextColumn();
            if (buy > 0 && sell > 0) {
                ImGui::Text("%.1f%%", FlipOut::Analyzer::CalcMargin(buy, sell));
            } else {
                ImGui::Text("---");
            }

            // Owned count from Hoard & Seek
            if (hsConnected) {
                ImGui::TableNextColumn();
                const auto* owned = FlipOut::HoardBridge::GetOwnedItem(item.id);
                if (owned) {
                    ImGui::Text("%d", owned->total_count);
                    if (ImGui::IsItemHovered() && !owned->locations.empty()) {
                        ImGui::BeginTooltip();
                        for (const auto& [loc, count] : owned->locations) {
                            ImGui::Text("%s: %d", loc.c_str(), count);
                        }
                        ImGui::EndTooltip();
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "-");
                }
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

// --- GW2-Themed UI Style ---

static ImGuiStyle g_GW2Style;
static std::vector<ImGuiStyle> g_StyleStack;
static bool g_GW2StyleBuilt = false;

static void PushGW2Theme() {
    g_StyleStack.push_back(ImGui::GetStyle());
    ImGui::GetStyle() = g_GW2Style;
}

static void PopGW2Theme() {
    if (!g_StyleStack.empty()) {
        ImGui::GetStyle() = g_StyleStack.back();
        g_StyleStack.pop_back();
    }
}

static void BuildGW2Theme() {
    g_GW2Style = ImGui::GetStyle();
    ImGuiStyle& s = g_GW2Style;

    // Rounding
    s.WindowRounding    = 6.0f;
    s.ChildRounding     = 4.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding      = 3.0f;
    s.TabRounding       = 4.0f;

    // Spacing & padding
    s.WindowPadding     = ImVec2(10, 10);
    s.FramePadding      = ImVec2(6, 4);
    s.ItemSpacing       = ImVec2(8, 5);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 8.0f;
    s.WindowBorderSize  = 1.0f;
    s.ChildBorderSize   = 1.0f;
    s.PopupBorderSize   = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.TabBorderSize     = 0.0f;

    // Colors — dark slate base with warm gold accents
    ImVec4* c = s.Colors;

    // Backgrounds
    c[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.08f, 0.10f, 0.96f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.07f, 0.07f, 0.09f, 0.80f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);

    // Borders
    c[ImGuiCol_Border]               = ImVec4(0.28f, 0.25f, 0.18f, 0.50f);
    c[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frames (input boxes, combos)
    c[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.13f, 0.11f, 0.80f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.22f, 0.20f, 0.14f, 0.80f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.28f, 0.25f, 0.16f, 0.90f);

    // Title bar
    c[ImGuiCol_TitleBg]              = ImVec4(0.10f, 0.09f, 0.07f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.14f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.08f, 0.07f, 0.05f, 0.75f);

    // Menu bar
    c[ImGuiCol_MenuBarBg]            = ImVec4(0.12f, 0.11f, 0.09f, 1.00f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.06f, 0.07f, 0.60f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.27f, 0.18f, 0.80f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.36f, 0.22f, 0.90f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.44f, 0.26f, 1.00f);

    // Checkmark, slider
    c[ImGuiCol_CheckMark]            = ImVec4(0.90f, 0.75f, 0.25f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.70f, 0.58f, 0.20f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(0.85f, 0.70f, 0.25f, 1.00f);

    // Buttons — warm gold
    c[ImGuiCol_Button]               = ImVec4(0.22f, 0.20f, 0.12f, 0.80f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.35f, 0.30f, 0.14f, 0.90f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.45f, 0.38f, 0.16f, 1.00f);

    // Headers (selectables, collapsing headers)
    c[ImGuiCol_Header]               = ImVec4(0.18f, 0.16f, 0.10f, 0.70f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.24f, 0.12f, 0.80f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.35f, 0.30f, 0.14f, 0.90f);

    // Separator
    c[ImGuiCol_Separator]            = ImVec4(0.28f, 0.25f, 0.18f, 0.40f);
    c[ImGuiCol_SeparatorHovered]     = ImVec4(0.50f, 0.42f, 0.20f, 0.70f);
    c[ImGuiCol_SeparatorActive]      = ImVec4(0.65f, 0.55f, 0.25f, 1.00f);

    // Resize grip
    c[ImGuiCol_ResizeGrip]           = ImVec4(0.30f, 0.27f, 0.18f, 0.30f);
    c[ImGuiCol_ResizeGripHovered]    = ImVec4(0.50f, 0.44f, 0.26f, 0.60f);
    c[ImGuiCol_ResizeGripActive]     = ImVec4(0.65f, 0.55f, 0.25f, 0.90f);

    // Tabs — gold accent for active
    c[ImGuiCol_Tab]                  = ImVec4(0.14f, 0.13f, 0.10f, 0.86f);
    c[ImGuiCol_TabHovered]           = ImVec4(0.35f, 0.30f, 0.14f, 0.90f);
    c[ImGuiCol_TabActive]            = ImVec4(0.28f, 0.24f, 0.10f, 1.00f);
    c[ImGuiCol_TabUnfocused]         = ImVec4(0.10f, 0.09f, 0.07f, 0.97f);
    c[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.18f, 0.16f, 0.10f, 1.00f);

    // Text
    c[ImGuiCol_Text]                 = ImVec4(0.90f, 0.87f, 0.78f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.47f, 0.40f, 1.00f);

    // Modal dim background
    c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);

    // Nav highlight
    c[ImGuiCol_NavHighlight]         = ImVec4(0.70f, 0.58f, 0.20f, 1.00f);

    // Table
    c[ImGuiCol_TableHeaderBg]        = ImVec4(0.14f, 0.13f, 0.10f, 1.00f);
    c[ImGuiCol_TableBorderStrong]    = ImVec4(0.28f, 0.25f, 0.18f, 0.60f);
    c[ImGuiCol_TableBorderLight]     = ImVec4(0.22f, 0.20f, 0.15f, 0.40f);
    c[ImGuiCol_TableRowBg]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]        = ImVec4(0.10f, 0.10f, 0.08f, 0.30f);

    // Plot (progress bars)
    c[ImGuiCol_PlotHistogram]        = ImVec4(0.65f, 0.55f, 0.15f, 1.00f);
    c[ImGuiCol_PlotHistogramHovered] = ImVec4(0.80f, 0.68f, 0.20f, 1.00f);
}

// --- Addon Lifecycle ---

void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc,
                                 (void(*)(void*, void*))APIDefs->ImguiFree);

    BuildGW2Theme();

    // Initialize subsystems
    FlipOut::IconManager::Initialize(APIDefs);
    FlipOut::HoardBridge::Initialize(APIDefs);

    // Load saved data in background to avoid blocking game startup
    g_DataLoading = true;
    g_DataLoaded = false;
    std::thread([]() {
        FlipOut::TPAPI::LoadItemCache();
        FlipOut::TPAPI::LoadRecipeCache();
        FlipOut::PriceDB::Load();
        FlipOut::PriceDB::LoadWatchlist();
        FlipOut::HoardBridge::LoadSession();
        TryDownloadSeed();
        g_DataLoading = false;
        g_DataLoaded = true;
    }).detach();

    // Register render functions
    APIDefs->GUI_Register(RT_Render, AddonRender);
    APIDefs->GUI_Register(RT_OptionsRender, AddonOptions);

    // Register keybind
    APIDefs->InputBinds_RegisterWithString("KB_FLIPOUT_TOGGLE", ProcessKeybind, "CTRL+SHIFT+T");

    // Load icon textures from embedded PNG data
    APIDefs->Textures_LoadFromMemory(TEX_ICON, (void*)ICON_FLIPOUT, ICON_FLIPOUT_size, nullptr);
    APIDefs->Textures_LoadFromMemory(TEX_ICON_HOVER, (void*)ICON_FLIPOUT_HOVER, ICON_FLIPOUT_HOVER_size, nullptr);

    // Register quick access shortcut
    APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER, "KB_FLIPOUT_TOGGLE", "Flip Out");

    // Register close-on-escape
    APIDefs->GUI_RegisterCloseOnEscape("Flip Out", &g_WindowVisible);

    g_LastAutoRefresh = std::chrono::steady_clock::now();

    APIDefs->Log(LOGL_INFO, "FlipOut", "Addon loaded successfully");
}

void AddonUnload() {
    // Deregister UI first (fast)
    APIDefs->GUI_DeregisterCloseOnEscape("Flip Out");
    APIDefs->QuickAccess_Remove(QA_ID);
    APIDefs->GUI_Deregister(AddonOptions);
    APIDefs->GUI_Deregister(AddonRender);

    FlipOut::HoardBridge::Shutdown();
    FlipOut::IconManager::Shutdown();

    // Save state in background thread, but join to ensure completion before DLL unloads
    if (g_DataLoaded) {
        std::thread saveThread([]() {
            FlipOut::PriceDB::Save();
            FlipOut::PriceDB::SaveWatchlist();
            FlipOut::TPAPI::SaveItemCache();
        });
        saveThread.join();
    }

    APIDefs = nullptr;
}

void ProcessKeybind(const char* aIdentifier, bool aIsRelease) {
    if (aIsRelease) return;
    if (strcmp(aIdentifier, "KB_FLIPOUT_TOGGLE") == 0) {
        g_WindowVisible = !g_WindowVisible;
    }
}

// --- Main Render ---

void AddonRender() {
    // Process icon downloads every frame
    FlipOut::IconManager::Tick();

    // Auto-refresh watchlist prices (public endpoints, no auth needed)
    if (g_AutoRefresh && g_DataLoaded) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - g_LastAutoRefresh).count();
        if (elapsed >= g_RefreshIntervalMin) {
            g_LastAutoRefresh = now;
            auto& watchlist = FlipOut::PriceDB::GetWatchlist();
            if (!watchlist.empty()) {
                std::vector<uint32_t> ids;
                for (const auto& e : watchlist) ids.push_back(e.item_id);
                std::thread([ids]() {
                    auto prices = FlipOut::TPAPI::FetchPrices(ids);
                    std::unordered_map<uint32_t, FlipOut::PriceSnapshot> snaps;
                    for (const auto& p : prices) {
                        FlipOut::PriceSnapshot s;
                        s.buy_price = p.buy_price;
                        s.sell_price = p.sell_price;
                        s.buy_quantity = p.buy_quantity;
                        s.sell_quantity = p.sell_quantity;
                        snaps[p.item_id] = s;
                    }
                    FlipOut::PriceDB::RecordBulkPrices(snaps);
                    FlipOut::PriceDB::Save();
                }).detach();
            }
        }
    }

    // Drive HoardBridge refresh logic every frame
    FlipOut::HoardBridge::Tick();

    if (!g_WindowVisible) return;

    PushGW2Theme();

    // On first window open, request Hoard & Seek permissions
    {
        static bool s_FirstOpenDone = false;
        if (!s_FirstOpenDone) {
            s_FirstOpenDone = true;
            FlipOut::HoardBridge::RequestPermissions();
        }
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(600, 300), ImVec2(FLT_MAX, FLT_MAX));
    if (!ImGui::Begin("Flip Out - TP Market Tracker", &g_WindowVisible,
        ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        PopGW2Theme();
        return;
    }

    // Show loading indicator while data loads in background
    if (g_DataLoading) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Loading data...");
        ImGui::End();
        PopGW2Theme();
        return;
    }

    // Hoard & Seek connection status bar
    {
        ImGui::AlignTextToFramePadding();
        auto hsStatus = FlipOut::HoardBridge::GetStatus();
        if (hsStatus == FlipOut::HoardStatus::Connected) {
            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "[Hoard & Seek: Connected]");
        } else if (hsStatus == FlipOut::HoardStatus::NoData) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[Hoard & Seek: No Data]");
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "[Hoard & Seek: Not Connected]");
            ImGui::SameLine();
            if (ImGui::SmallButton("Reconnect")) {
                FlipOut::HoardBridge::Ping();
            }
        }

        // Account selector (only shown when multiple accounts exist)
        if (!FlipOut::HoardBridge::IsSingleAccount() && hsStatus == FlipOut::HoardStatus::Connected) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " | ");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Account:");
            ImGui::SameLine();

            const auto& accounts = FlipOut::HoardBridge::GetAccounts();
            int activeIdx = FlipOut::HoardBridge::GetActiveAccountIndex();

            // Build preview text
            const char* preview = (activeIdx >= 0 && activeIdx < (int)accounts.size())
                ? accounts[activeIdx].display_name.c_str()
                : "Auto-detect...";

            ImGui::SetNextItemWidth(160);
            if (ImGui::BeginCombo("##account_select", preview, ImGuiComboFlags_NoArrowButton)) {
                for (int i = 0; i < (int)accounts.size(); i++) {
                    bool selected = (i == activeIdx);
                    if (ImGui::Selectable(accounts[i].display_name.c_str(), selected)) {
                        FlipOut::HoardBridge::SetActiveAccount(i);
                        // Reset per-tab state so they re-fetch for the new account
                        g_CraftingRecipesQueried = false;
                        g_CraftingIngredientsQueried = false;
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " | ");
        ImGui::SameLine();
        auto fetchStatus = FlipOut::TPAPI::GetBulkFetchStatus();
        if (fetchStatus == FlipOut::FetchStatus::Success) {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Prices loaded");
        } else if (fetchStatus == FlipOut::FetchStatus::InProgress) {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Scanning...");
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No price data");
        }
    }

    // Tab bar
    if (ImGui::BeginTabBar("##maintabs")) {
        if (ImGui::BeginTabItem("Flips")) {
            g_ActiveTab = 0;
            RenderFlipsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Crafting")) {
            g_ActiveTab = 1;
            RenderCraftingTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Liquidate")) {
            g_ActiveTab = 2;
            RenderLiquidateTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Watchlist")) {
            g_ActiveTab = 3;
            RenderWatchlistTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Transactions")) {
            g_ActiveTab = 4;
            RenderTransactionsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Search")) {
            g_ActiveTab = 5;
            RenderSearchTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    // Price history window (rendered outside the main window)
    RenderPriceHistoryWindow();

    PopGW2Theme();
}

// --- Options/Settings Render ---

void AddonOptions() {
    PushGW2Theme();
    ImGui::Text("Flip Out Settings");
    if (ImGui::SmallButton("Homepage")) {
        ShellExecuteA(NULL, "open", "https://pie.rocks.cc/", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Buy me a coffee!")) {
        ShellExecuteA(NULL, "open", "https://ko-fi.com/pieorcake", NULL, NULL, SW_SHOWNORMAL);
    }
    ImGui::Separator();

    // Hoard & Seek connection status
    ImGui::Text("Hoard & Seek:");
    ImGui::SameLine();
    auto hsStatus = FlipOut::HoardBridge::GetStatus();
    switch (hsStatus) {
        case FlipOut::HoardStatus::Connected:
            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "Connected");
            break;
        case FlipOut::HoardStatus::NoData:
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Connected (no account data)");
            break;
        default:
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Not detected");
            break;
    }
    if (FlipOut::HoardBridge::IsPermissionDenied()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Permission denied by user in Hoard & Seek");
    } else if (FlipOut::HoardBridge::IsPermissionPending()) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Permission pending (check Hoard & Seek)");
    }

    if (hsStatus != FlipOut::HoardStatus::Connected) {
        if (ImGui::Button("Ping Hoard & Seek")) {
            FlipOut::HoardBridge::Ping();
        }
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Hoard & Seek is required for account data and transactions.");
    } else {
        if (ImGui::Button("Refresh Account Data")) {
            FlipOut::HoardBridge::RequestRefresh();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Auto-Refresh:");
    ImGui::Checkbox("Enable auto-refresh for watchlist", &g_AutoRefresh);
    ImGui::SliderInt("Refresh interval (minutes)", &g_RefreshIntervalMin, 5, 60);
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
        "TP prices are public (no API key needed). Transactions use Hoard & Seek.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Quick Access:");
    if (ImGui::Checkbox("Show icon in Quick Access bar", &g_ShowQAIcon)) {
        if (g_ShowQAIcon) {
            APIDefs->QuickAccess_Add(QA_ID, TEX_ICON, TEX_ICON_HOVER, "KB_FLIPOUT_TOGGLE", "Flip Out");
        } else {
            APIDefs->QuickAccess_Remove(QA_ID);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Data Management:");
    if (ImGui::Button("Save All Data")) {
        FlipOut::PriceDB::Save();
        FlipOut::PriceDB::SaveWatchlist();
        FlipOut::TPAPI::SaveItemCache();
    }
    ImGui::SameLine();
    if (ImGui::Button("Trim Old History")) {
        FlipOut::PriceDB::Trim(1000, 30);
        FlipOut::PriceDB::Save();
    }
    ImGui::SameLine();
    ImGui::Text("Tracking %d items", (int)FlipOut::PriceDB::GetTrackedItemCount());
    PopGW2Theme();
}

// --- Export: GetAddonDef ---

extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    AddonDef.Signature = 0x1be8c2d6;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = "Flip Out";
    AddonDef.Version.Major = V_MAJOR;
    AddonDef.Version.Minor = V_MINOR;
    AddonDef.Version.Build = V_BUILD;
    AddonDef.Version.Revision = V_REVISION;
    AddonDef.Author = "PieOrCake.7635";
    AddonDef.Description = "Trading Post market tracker - find flips, track trends, make gold";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = AF_None;
    AddonDef.Provider = UP_None;
    AddonDef.UpdateLink = nullptr;

    return &AddonDef;
}
