# Flip Out - GW2 Trading Post Market Tracker

A Guild Wars 2 addon for the [Nexus](https://raidcore.gg/Nexus) framework that tracks Trading Post market trends and helps you find profitable flips.

## Features

- **Flip Scanner** — Scans all tradeable items and ranks them by profit, margin, ROI, or volume
- **Watchlist** — Track specific items over time with price history and trend indicators
- **Trend Analysis** — Linear regression on price history to detect rising/falling/stable trends
- **Transaction Viewer** — View your pending buy orders and sell listings
- **Item Search** — Search the item cache by name with live price + margin display
- **Auto-Refresh** — Watchlist prices update automatically on a configurable interval
- **Persistent Storage** — Price history, watchlist, and item cache saved to disk as JSON

## Requirements

- Guild Wars 2 with [Nexus addon loader](https://raidcore.gg/Nexus) installed
- GW2 API key with **account** and **tradingpost** permissions
  - Create one at [account.arena.net/applications](https://account.arena.net/applications)

## Building

### Prerequisites (Linux cross-compilation)

- `x86_64-w64-mingw32-g++` (MinGW-w64)
- `cmake` >= 3.20
- `curl` (for dependency download)

### Setup & Build

```bash
# Download ImGui v1.80 and nlohmann/json v3.11.3
chmod +x scripts/setup.sh
./scripts/setup.sh

# Build
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

The output `FlipOut.dll` will be in the `build/` directory.

### Installation

Copy `FlipOut.dll` to your GW2 Nexus addons directory:
```
<GW2 install>/addons/FlipOut.dll
```

## Usage

1. Open the addon with **Ctrl+Shift+T** or via the Nexus quick access bar
2. Go to **Settings** (Nexus addon settings) and paste your GW2 API key
3. Click **Scan Market** on the Flips tab to fetch all TP prices
4. Browse flip opportunities, sorted by your preferred criteria
5. Add items to your **Watchlist** to track them over time
6. Check the **Transactions** tab to see your pending orders

## How Flips Work

A "flip" is buying an item via buy order and reselling it via sell listing. The Trading Post charges:
- **5% listing fee** (paid when you list)
- **10% exchange fee** (paid when the item sells)

Flip Out calculates profit after both fees:
```
Profit = Sell Price - (5% of Sell) - (10% of Sell) - Buy Price
```

## Architecture

| File | Purpose |
|------|---------|
| `dllmain.cpp` | Nexus lifecycle, ImGui UI (tabs, tables, rendering) |
| `TPAPI.h/cpp` | GW2 Trading Post API client (prices, listings, transactions) |
| `PriceDB.h/cpp` | Local price history storage with JSON persistence |
| `Analyzer.h/cpp` | Flip detection, margin calculation, trend analysis |
| `IconManager.h/cpp` | Async icon downloading and texture loading |
| `HttpClient.h/cpp` | WinINet HTTP client wrapper |

## Compatibility

- **Nexus API**: v6
- **ImGui**: v1.80
- **nlohmann/json**: v3.11.3
- Matched to [Hoard & Seek](https://github.com/PieOrCake/hoard_and_seek) for compatibility

## License

MIT
