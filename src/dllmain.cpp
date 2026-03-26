#include <windows.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <cfloat>

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
#define V_MINOR 1
#define V_BUILD 0
#define V_REVISION 0

// Quick Access icon identifiers
#define QA_ID "QA_FLIP_OUT"
#define TEX_ICON "TEX_FLIPOUT_ICON"
#define TEX_ICON_HOVER "TEX_FLIPOUT_ICON_HOVER"

// Embedded 32x32 stick figure icon (normal - grey, tilted 30 degrees)
static const unsigned char ICON_FLIPOUT[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a,
    0xf4, 0x00, 0x00, 0x00, 0x7f, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x93, 0xc1, 0x0d, 0xc0,
    0x20, 0x08, 0x45, 0x1d, 0xb6, 0xe3, 0x38, 0x0e, 0x1b, 0x38, 0x98, 0x0d, 0x87, 0x26, 0xc4, 0x48,
    0xb5, 0x01, 0xa4, 0x87, 0xff, 0x12, 0x2e, 0x9a, 0xf8, 0x9f, 0x88, 0xa5, 0x00, 0x00, 0x8c, 0x10,
    0x51, 0x87, 0x00, 0x04, 0x7e, 0x2d, 0xc0, 0xfb, 0x2e, 0x92, 0xda, 0x41, 0xda, 0x9a, 0x2c, 0xb7,
    0x9b, 0xae, 0x04, 0x42, 0x82, 0x57, 0x12, 0x63, 0x68, 0xf8, 0x4c, 0xc8, 0x90, 0x63, 0xa1, 0x47,
    0xde, 0xd8, 0x22, 0x91, 0x2e, 0xb0, 0x23, 0x51, 0x6b, 0xbd, 0xb8, 0xcc, 0xc1, 0xad, 0xb5, 0xae,
    0x0d, 0x5f, 0x68, 0x57, 0x38, 0x58, 0xd6, 0xdb, 0xb7, 0x0c, 0x91, 0x18, 0x05, 0x9e, 0x4e, 0xec,
    0x60, 0x6e, 0xff, 0x2c, 0xfc, 0xab, 0x44, 0x6a, 0x07, 0xd2, 0x04, 0xdc, 0x26, 0x7f, 0xf6, 0x0b,
    0x00, 0x00, 0x5e, 0xdc, 0x41, 0x86, 0x1c, 0x3e, 0xd8, 0x7d, 0x76, 0x07, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const unsigned int ICON_FLIPOUT_size = 184;

// Embedded 32x32 stick figure icon (hover - brighter white-grey)
static const unsigned char ICON_FLIPOUT_HOVER[] = {
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x06, 0x00, 0x00, 0x00, 0x73, 0x7a, 0x7a,
    0xf4, 0x00, 0x00, 0x00, 0x7f, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x93, 0x41, 0x0a, 0x00,
    0x21, 0x08, 0x45, 0xbb, 0x3f, 0x73, 0xb0, 0xee, 0xe2, 0xa6, 0x1b, 0x34, 0xb8, 0x18, 0x90, 0xc8,
    0xa9, 0x41, 0xcd, 0x59, 0xfc, 0x07, 0x6e, 0x0a, 0xfa, 0x2f, 0xb3, 0x52, 0x00, 0x00, 0x46, 0x88,
    0xa8, 0x43, 0x00, 0x02, 0xbf, 0x16, 0xe0, 0x7d, 0x17, 0x49, 0xed, 0x20, 0x6d, 0x4d, 0x96, 0xdb,
    0x4d, 0x57, 0x02, 0x21, 0xc1, 0x2b, 0x89, 0x31, 0x34, 0x7c, 0x26, 0x64, 0xc8, 0xb1, 0xd0, 0x23,
    0x6f, 0x6c, 0x91, 0x48, 0x17, 0xd8, 0x91, 0xa8, 0xb5, 0x5e, 0x5c, 0xe6, 0xe0, 0xd6, 0x5a, 0xd7,
    0x86, 0x2f, 0xb4, 0x2b, 0x1c, 0x2c, 0xeb, 0xed, 0x5b, 0x86, 0x48, 0x8c, 0x02, 0x4f, 0x27, 0x76,
    0x30, 0xb7, 0x7f, 0x16, 0xfe, 0x55, 0x22, 0xb5, 0x03, 0x69, 0x02, 0x6e, 0x93, 0x3f, 0xfb, 0x05,
    0x00, 0x00, 0x2f, 0x6e, 0x3e, 0xb5, 0x55, 0x8c, 0x2b, 0xdf, 0x8a, 0x1c, 0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static const unsigned int ICON_FLIPOUT_HOVER_size = 184;

// Global variables
HMODULE hSelf;
AddonDefinition_t AddonDef{};
AddonAPI_t* APIDefs = nullptr;
bool g_WindowVisible = false;

// UI State
static int g_ActiveTab = 0; // 0=Flips, 1=Watchlist, 2=Transactions, 3=Search

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

// Price history window state
static bool g_PriceHistoryOpen = false;
static uint32_t g_PriceHistoryItemId = 0;
static std::string g_PriceHistoryItemName;
static std::string g_PriceHistoryItemRarity;
static int g_PriceHistoryRange = 2; // 0=1D, 1=1W, 2=1M, 3=3M, 4=6M

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
static void RenderCoins(int copper) {
    if (copper < 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "-%s",
            FlipOut::Analyzer::FormatCoinsShort(-copper).c_str());
        return;
    }
    int gold = copper / 10000;
    int silver = (copper % 10000) / 100;
    int cop = copper % 100;

    std::string text;
    if (gold > 0) text += std::to_string(gold) + "g ";
    if (silver > 0 || gold > 0) text += std::to_string(silver) + "s ";
    text += std::to_string(cop) + "c";
    ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "%s", text.c_str());
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
    const char* range_labels[] = { "1D", "1W", "1M", "3M", "6M" };
    const int range_hours[] = { 24, 24*7, 24*30, 24*90, 24*180 };
    ImGui::Spacing();
    for (int i = 0; i < 5; i++) {
        if (i > 0) ImGui::SameLine();
        bool selected = (g_PriceHistoryRange == i);
        if (selected) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button(range_labels[i], ImVec2(40, 0))) {
            g_PriceHistoryRange = i;
        }
        if (selected) ImGui::PopStyleColor();
    }

    // Fetch history data filtered by time range
    auto all_history = FlipOut::PriceDB::GetHistory(g_PriceHistoryItemId, 0);
    time_t cutoff = std::time(nullptr) - (time_t)range_hours[g_PriceHistoryRange] * 3600;
    std::vector<FlipOut::PriceSnapshot> history;
    for (const auto& snap : all_history) {
        if (snap.timestamp >= cutoff) history.push_back(snap);
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
        if (g_PriceHistoryRange == 0) {
            strftime(tbuf, sizeof(tbuf), "%H:%M", tm_info);
        } else {
            strftime(tbuf, sizeof(tbuf), "%m/%d", tm_info);
        }
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
        if (!canScan) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.4f);
            ImGui::Button("Scan Market");
            ImGui::PopStyleVar();
        } else {
            if (ImGui::Button("Scan Market")) {
                FlipOut::TPAPI::FetchAllPricesAsync();
                g_FlipsDirty = true;
            }
        }
        ImGui::SameLine();
        if (!canScan) {
            int cd = ScanCooldownRemaining();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Cooldown: %d:%02d",
                cd / 60, cd % 60);
        } else if (fetchStatus == FlipOut::FetchStatus::Success) {
            ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "%s",
                FlipOut::TPAPI::GetBulkFetchMessage().c_str());
        } else if (fetchStatus == FlipOut::FetchStatus::Error) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s",
                FlipOut::TPAPI::GetBulkFetchMessage().c_str());
        } else {
            const auto& msg = FlipOut::TPAPI::GetBulkFetchMessage();
            if (!msg.empty()) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", msg.c_str());
            }
        }
        // Show last scan timestamp
        if (g_HasScanned) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| Last scan: %s",
                FormatTimeAgo(g_LastScanTime).c_str());
        }
    }

    // When bulk fetch completes, record prices and mark flips dirty
    {
        static FlipOut::FetchStatus s_prevStatus = FlipOut::FetchStatus::Idle;
        if (fetchStatus == FlipOut::FetchStatus::Success && s_prevStatus != FlipOut::FetchStatus::Success) {
            g_FlipsDirty = true;
            g_HasScanned = true;
            g_LastScanTime = std::chrono::steady_clock::now();
            // Record current prices into history DB
            auto prices = FlipOut::TPAPI::GetAllPrices();
            std::unordered_map<uint32_t, FlipOut::PriceSnapshot> snaps;
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

            // Query Hoard & Seek for owned status of top flip candidates and market movers
            if (FlipOut::HoardBridge::IsDataAvailable()) {
                // Collect unique item IDs to query
                std::vector<uint32_t> queryIds;
                auto topFlips = FlipOut::Analyzer::FindFlips(g_FlipFilter);
                for (const auto& f : topFlips) queryIds.push_back(f.item_id);
                auto topMovers = FlipOut::Analyzer::FindMarketMovers(20.0f, 3, 100, 50);
                for (const auto& m : topMovers) queryIds.push_back(m.item_id);
                FlipOut::HoardBridge::QueryItems(queryIds);
            }
        }
        s_prevStatus = fetchStatus;
    }

    // Recompute flips if dirty
    if (g_FlipsDirty) {
        g_Flips = FlipOut::Analyzer::FindFlips(g_FlipFilter);
        g_FlipsDirty = false;
        g_SellOppsDirty = true;
        g_MoversDirty = true;
    }

    // Recompute sell opportunities when owned item data changes
    {
        static size_t s_lastOwnedCount = 0;
        size_t curOwnedCount = FlipOut::HoardBridge::GetOwnedItemCount();
        if (curOwnedCount != s_lastOwnedCount) {
            s_lastOwnedCount = curOwnedCount;
            g_SellOppsDirty = true;
        }
    }
    if (g_SellOppsDirty && FlipOut::HoardBridge::GetOwnedItemCount() > 0) {
        g_SellOpps = FlipOut::Analyzer::FindSellOpportunities(20.0f, 3);
        g_SellOppsDirty = false;
    }

    // Recompute market movers if dirty
    if (g_MoversDirty) {
        g_MarketMovers = FlipOut::Analyzer::FindMarketMovers(20.0f, 3, 100, 50);
        g_MoversDirty = false;
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
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
                ImVec2(0, std::min((float)g_SellOpps.size() * 28.0f + 30.0f, 250.0f))))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Owned", ImGuiTableColumnFlags_WidthFixed, 45.0f);
                ImGui::TableSetupColumn("Current Sell", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Avg Sell", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupColumn("Above Avg", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Profit/Unit", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Total Profit", ImGuiTableColumnFlags_WidthFixed, 85.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const auto& opp : g_SellOpps) {
                    ImGui::TableNextRow(0, ICON_SIZE + 4);
                    ImGui::PushID((int)opp.item_id);

                    // Invisible selectable spanning all columns for row right-click
                    ImGui::TableNextColumn();
                    ImGui::Selectable("##row", false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                        ImVec2(0, ICON_SIZE));
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
                    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "%s",
                        FlipOut::Analyzer::FormatCoinsShort(opp.profit_vs_avg).c_str());

                    // Total potential profit
                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(0.35f, 0.82f, 0.35f, 1.0f), "%s",
                        FlipOut::Analyzer::FormatCoinsShort(opp.potential_profit).c_str());

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
                ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
                ImVec2(0, std::min((float)g_MarketMovers.size() * 28.0f + 30.0f, 250.0f))))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Now", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Avg", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Buy Vol", ImGuiTableColumnFlags_WidthFixed, 65.0f);
                ImGui::TableSetupColumn("Vol", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

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
        ImGui::TableSetupColumn("Profit", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Margin", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("ROI", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Volume", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

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
            ImVec4 profitColor = flip.profit_per_unit > 0
                ? ImVec4(0.35f, 0.82f, 0.35f, 1.0f)
                : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
            ImGui::TextColored(profitColor, "%s",
                FlipOut::Analyzer::FormatCoinsShort(flip.profit_per_unit).c_str());

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
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
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
        ImGui::TableSetupColumn("##remove", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        auto prices = FlipOut::TPAPI::GetAllPrices();

        // Copy watchlist to iterate safely
        auto wl_copy = watchlist;
        uint32_t removeId = 0;

        for (const auto& entry : wl_copy) {
            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)entry.item_id);

            const auto* info = FlipOut::TPAPI::GetItemInfo(entry.item_id);
            std::string name = info ? info->name : ("Item #" + std::to_string(entry.item_id));
            std::string rarity = info ? info->rarity : "";
            std::string iconUrl = info ? info->icon_url : "";

            auto priceIt = prices.find(entry.item_id);
            int buy = priceIt != prices.end() ? priceIt->second.buy_price : 0;
            int sell = priceIt != prices.end() ? priceIt->second.sell_price : 0;

            // Item
            ImGui::TableNextColumn();
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
                ImVec4 col = profit > 0 ? ImVec4(0.35f, 0.82f, 0.35f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(col, "%s", FlipOut::Analyzer::FormatCoinsShort(profit).c_str());
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

    if (!txLoaded && !txLoading) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Click 'Load Transactions' to view your pending TP orders (via Hoard & Seek).");
    }

    if (txLoading) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    if (ImGui::Button("Load Transactions") && !txLoading) {
        FlipOut::HoardBridge::FetchTransactions();
    }
    if (txLoading) ImGui::PopStyleVar();

    if (txLoading) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "Loading via Hoard & Seek...");
    }

    if (!txLoaded) return;

    ImGui::Separator();

    const auto& currentBuys = FlipOut::HoardBridge::GetCurrentBuys();
    const auto& currentSells = FlipOut::HoardBridge::GetCurrentSells();

    // Pending Buys
    if (ImGui::CollapsingHeader("Pending Buy Orders", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (currentBuys.empty()) {
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No pending buy orders.");
        } else {
            if (ImGui::BeginTable("##txbuys", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Created", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                for (const auto& tx : currentBuys) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    const auto* info = FlipOut::TPAPI::GetItemInfo(tx.item_id);
                    std::string name = info ? info->name : ("Item #" + std::to_string(tx.item_id));
                    std::string rarity = info ? info->rarity : "";
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
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("Item", ImGuiTableColumnFlags_WidthStretch, 3.0f);
                ImGui::TableSetupColumn("Price", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Qty", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("Created", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableHeadersRow();

                for (const auto& tx : currentSells) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    const auto* info = FlipOut::TPAPI::GetItemInfo(tx.item_id);
                    std::string name = info ? info->name : ("Item #" + std::to_string(tx.item_id));
                    std::string rarity = info ? info->rarity : "";
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
                }
                ImGui::EndTable();
            }
        }
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

    auto prices = FlipOut::TPAPI::GetAllPrices();
    bool hsConnected = FlipOut::HoardBridge::IsDataAvailable();

    int col_count = hsConnected ? 6 : 5;
    if (ImGui::BeginTable("##search_results", col_count,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp,
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

        for (const auto& item : g_SearchResults) {
            ImGui::TableNextRow(0, ICON_SIZE + 4);
            ImGui::PushID((int)item.id);

            // Invisible selectable spanning all columns for row right-click
            ImGui::TableNextColumn();
            ImGui::Selectable("##row", false,
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap,
                ImVec2(0, ICON_SIZE));
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

            auto pit = prices.find(item.id);
            int buy = pit != prices.end() ? pit->second.buy_price : 0;
            int sell = pit != prices.end() ? pit->second.sell_price : 0;

            ImGui::TableNextColumn();
            if (buy > 0) RenderCoins(buy); else ImGui::Text("---");

            ImGui::TableNextColumn();
            if (sell > 0) RenderCoins(sell); else ImGui::Text("---");

            ImGui::TableNextColumn();
            if (buy > 0 && sell > 0) {
                int profit = FlipOut::Analyzer::CalcProfit(buy, sell);
                ImVec4 col = profit > 0 ? ImVec4(0.35f, 0.82f, 0.35f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                ImGui::TextColored(col, "%s", FlipOut::Analyzer::FormatCoinsShort(profit).c_str());
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

// --- Addon Lifecycle ---

void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))APIDefs->ImguiMalloc,
                                 (void(*)(void*, void*))APIDefs->ImguiFree);

    // Initialize subsystems
    FlipOut::IconManager::Initialize(APIDefs);
    FlipOut::HoardBridge::Initialize(APIDefs);

    // Load saved data
    FlipOut::TPAPI::LoadItemCache();
    FlipOut::PriceDB::Load();
    FlipOut::PriceDB::LoadWatchlist();

    // Download community seed data if local history is thin
    TryDownloadSeed();

    // Start scanning immediately so results are ready by the time the user logs in
    FlipOut::TPAPI::FetchAllPricesAsync();

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
    FlipOut::HoardBridge::Shutdown();
    FlipOut::IconManager::Shutdown();

    // Save state
    FlipOut::PriceDB::Save();
    FlipOut::PriceDB::SaveWatchlist();
    FlipOut::TPAPI::SaveItemCache();

    APIDefs->GUI_DeregisterCloseOnEscape("Flip Out");
    APIDefs->QuickAccess_Remove(QA_ID);
    APIDefs->GUI_Deregister(AddonOptions);
    APIDefs->GUI_Deregister(AddonRender);

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
    if (g_AutoRefresh) {
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
        return;
    }

    // Hoard & Seek connection status bar
    {
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
    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Flips")) {
            g_ActiveTab = 0;
            RenderFlipsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Watchlist")) {
            g_ActiveTab = 1;
            RenderWatchlistTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Transactions")) {
            g_ActiveTab = 2;
            RenderTransactionsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Search")) {
            g_ActiveTab = 3;
            RenderSearchTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

    // Price history window (rendered outside the main window)
    RenderPriceHistoryWindow();
}

// --- Options/Settings Render ---

void AddonOptions() {
    ImGui::Text("Flip Out Settings");
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
    AddonDef.Author = "FlipOut";
    AddonDef.Description = "Trading Post market tracker - find flips, track trends, make gold";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = AF_None;
    AddonDef.Provider = UP_None;
    AddonDef.UpdateLink = nullptr;

    return &AddonDef;
}
