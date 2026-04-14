/*
 * HoardAndSeekAPI.h - Cross-addon event interface for Hoard & Seek
 *
 * Include this header in your addon to communicate with Hoard & Seek
 * via Nexus events. No link-time dependency is required.
 *
 * Version: 3
 */

#pragma once

#include <cstdint>

#define HOARD_API_VERSION 3
#define HOARD_REFRESH_COOLDOWN 300  // Minimum seconds between refreshes (5 minutes)

// Response status codes
#define HOARD_STATUS_OK       0  // Request succeeded
#define HOARD_STATUS_DENIED   1  // Permission denied by user
#define HOARD_STATUS_PENDING  2  // Permission not yet decided (popup shown)

// ============================================================================
// Event Names
// ============================================================================

// Broadcasts (raised by Hoard & Seek) -----------------------------------------

// Raised when account data has finished loading (startup cache load or refresh).
// Payload: HoardDataReadyPayload*
#define EV_HOARD_DATA_UPDATED  "EV_HOARD_DATA_UPDATED"

// Raised periodically during an account data fetch with progress info.
// Payload: HoardFetchProgressPayload*
#define EV_HOARD_FETCH_PROGRESS "EV_HOARD_FETCH_PROGRESS"

// Raised when an account data fetch fails (API offline, network error, etc.).
// Payload: HoardFetchErrorPayload*
#define EV_HOARD_FETCH_ERROR   "EV_HOARD_FETCH_ERROR"

// Raised by H&S in response to EV_HOARD_PING.
// Payload: HoardPongPayload*
#define EV_HOARD_PONG           "EV_HOARD_PONG"

// Raised when accounts are added or removed in H&S settings.
// Payload: nullptr — re-query EV_HOARD_QUERY_ACCOUNTS to refresh your list.
#define EV_HOARD_ACCOUNTS_CHANGED "EV_HOARD_ACCOUNTS_CHANGED"

// Requests (subscribe in your addon, raised by your addon) --------------------

// Ping H&S to check if it's loaded. H&S responds with EV_HOARD_PONG.
// Payload: nullptr
#define EV_HOARD_PING          "EV_HOARD_PING"

// Trigger an account data refresh in H&S (same as pressing Refresh Account Data).
// Payload: nullptr
#define EV_HOARD_REFRESH       "EV_HOARD_REFRESH"

// Open H&S window and search for an item by name.
// Payload: const char* (null-terminated item name)
#define EV_HOARD_SEARCH        "EV_HOARD_SEARCH"

// Query total count + locations for a specific item ID.
// Payload: HoardQueryItemRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryItemResponse* (caller must free).
#define EV_HOARD_QUERY_ITEM    "EV_HOARD_QUERY_ITEM"

// Query wallet currency balance.
// Payload: HoardQueryWalletRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryWalletResponse* (caller must free).
#define EV_HOARD_QUERY_WALLET  "EV_HOARD_QUERY_WALLET"

// Query account achievement progress (batch, up to 200 IDs).
// Payload: HoardQueryAchievementRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryAchievementResponse* (caller must free).
#define EV_HOARD_QUERY_ACHIEVEMENT "EV_HOARD_QUERY_ACHIEVEMENT"

// Query account mastery progress (batch, up to 200 IDs).
// Payload: HoardQueryMasteryRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryMasteryResponse* (caller must free).
#define EV_HOARD_QUERY_MASTERY "EV_HOARD_QUERY_MASTERY"

// Query account skin unlocks (batch, up to 200 IDs).
// Payload: HoardQuerySkinsRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQuerySkinsResponse* (caller must free).
#define EV_HOARD_QUERY_SKINS "EV_HOARD_QUERY_SKINS"

// Query account recipe unlocks (batch, up to 200 IDs).
// Payload: HoardQueryRecipesRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryRecipesResponse* (caller must free).
#define EV_HOARD_QUERY_RECIPES "EV_HOARD_QUERY_RECIPES"

// Query Wizard's Vault progress (daily, weekly, or special).
// Payload: HoardQueryWizardsVaultRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryWizardsVaultResponse* (caller must free).
#define EV_HOARD_QUERY_WIZARDSVAULT "EV_HOARD_QUERY_WIZARDSVAULT"

// Generic authenticated API proxy query.
// Makes any GW2 API endpoint available via H&S's stored API key.
// Payload: HoardQueryApiRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryApiResponse* (caller must free).
#define EV_HOARD_QUERY_API "EV_HOARD_QUERY_API"

// Query the list of configured accounts (names, labels, character lists).
// Payload: HoardQueryAccountsRequest*
// Response: Hoard & Seek raises the event named in `response_event`,
//           with payload HoardQueryAccountsResponse* (caller must free).
#define EV_HOARD_QUERY_ACCOUNTS "EV_HOARD_QUERY_ACCOUNTS"

// Context Menu integration ---------------------------------------------------

// Register a right-click context menu item in H&S search results.
// Payload: HoardContextMenuRegister*
#define EV_HOARD_CONTEXT_MENU_REGISTER "EV_HOARD_CONTEXT_MENU_REGISTER"

// Remove a previously registered context menu item.
// Payload: HoardContextMenuRemove*
#define EV_HOARD_CONTEXT_MENU_REMOVE   "EV_HOARD_CONTEXT_MENU_REMOVE"

// Item type flags for context menu registration
#define HOARD_MENU_ITEMS   1  // Show on items only
#define HOARD_MENU_WALLET  2  // Show on wallet currencies only
#define HOARD_MENU_ALL     3  // Show on both items and wallet

// ============================================================================
// Payload Structures
// ============================================================================

#pragma pack(push, 1)

// Broadcast: account data is ready
struct HoardDataReadyPayload {
    uint32_t api_version;       // HOARD_API_VERSION
    uint32_t item_count;        // Number of unique items tracked
    uint32_t currency_count;    // Number of wallet currencies tracked
    int64_t  last_updated;      // Unix timestamp of last successful fetch (0 if never)
    int64_t  refresh_available_at; // Unix timestamp when next refresh is allowed
    char     account_name[64];  // Which account was updated (empty = all accounts)
    uint32_t account_count;     // Total number of configured accounts
};

// Broadcast: pong response
struct HoardPongPayload {
    uint32_t api_version;          // HOARD_API_VERSION
    int64_t  last_updated;         // Unix timestamp of last successful fetch (0 if never)
    int64_t  refresh_available_at; // Unix timestamp when next refresh is allowed (0 = now)
    uint8_t  has_data;             // 1 if account data is loaded, 0 otherwise
    uint32_t account_count;        // Number of configured accounts
};

// Broadcast: fetch error
struct HoardFetchErrorPayload {
    uint32_t api_version;       // HOARD_API_VERSION
    char message[256];          // Human-readable error message
};

// Broadcast: fetch progress update
struct HoardFetchProgressPayload {
    uint32_t api_version;       // HOARD_API_VERSION
    char message[128];          // e.g. "Fetching bank...", "Fetching inventory: Character..."
    float progress;             // 0.0-1.0 estimated progress, or -1.0 if indeterminate
};

// A single account entry
struct HoardAccountEntry {
    char account_name[64];      // GW2 account name (e.g. "PieOrCake.7635")
    char label[64];             // User-assigned friendly name (e.g. "Main", "Alt")
    int64_t last_updated;       // Unix timestamp of last data fetch
    uint8_t validated;          // 1 if API key is validated
    uint32_t character_count;   // Number of characters on this account
    char characters[80][32];    // Up to 80 character names (GW2 max 72, names up to 31 chars)
};

// Request: query account list
struct HoardQueryAccountsRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    char response_event[64];    // Event name H&S will raise with the response
};

// Response: account list result
struct HoardQueryAccountsResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    uint32_t account_count;     // Number of accounts returned
    HoardAccountEntry accounts[16]; // Up to 16 accounts
};

// Request: query a single item
struct HoardQueryItemRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t item_id;           // GW2 item ID
    char response_event[64];    // Event name H&S will raise with the response
    char account_filter[64];    // Filter to one account (empty = all accounts)
};

// A single location entry in the response
struct HoardItemLocationEntry {
    char location[64];          // e.g. "Bank", "Material Storage", character name
    char sublocation[64];       // e.g. "Bag", "Equipped", "Category 37"
    int32_t count;
    char account_name[64];      // Which account this location belongs to
};

// Response: item query result
struct HoardQueryItemResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    uint32_t item_id;
    char name[128];
    char rarity[32];
    char type[32];
    int32_t total_count;
    uint32_t location_count;
    HoardItemLocationEntry locations[64]; // Up to 64 locations
};

// Request: query wallet currency
struct HoardQueryWalletRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t currency_id;       // GW2 currency ID (NOT the synthetic WALLET_ID_BASE | id)
    char response_event[64];    // Event name H&S will raise with the response
    char account_filter[64];    // Filter to one account (empty = sum across all)
};

// Response: wallet query result
struct HoardQueryWalletResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    uint32_t currency_id;
    char name[128];
    int32_t amount;
    bool found;
};

// Request: query account achievements (batch)
struct HoardQueryAchievementRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t ids[200];          // Achievement IDs to query
    uint32_t id_count;          // Number of IDs (1-200)
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account to query (empty = first account)
};

// A single achievement entry in the response
struct HoardAchievementEntry {
    uint32_t id;
    int32_t current;            // Current progress (-1 if not started)
    int32_t max;                // Max progress (-1 if unknown)
    bool done;                  // Whether the achievement is completed
    uint32_t bits[64];          // Completed bit indices
    uint32_t bit_count;         // Number of completed bits
};

// Response: achievement query result
struct HoardQueryAchievementResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    uint32_t entry_count;       // Number of entries returned
    HoardAchievementEntry entries[200];
};

// Request: query account masteries (batch)
struct HoardQueryMasteryRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t ids[200];          // Mastery IDs to query
    uint32_t id_count;          // Number of IDs (1-200)
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account to query (empty = first account)
};

// A single mastery entry in the response
struct HoardMasteryEntry {
    uint32_t id;
    int32_t level;              // Current mastery level (0 if not started)
};

// Response: mastery query result
struct HoardQueryMasteryResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    uint32_t entry_count;       // Number of entries returned
    HoardMasteryEntry entries[200];
};

// Request: query account skin unlocks (batch)
struct HoardQuerySkinsRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t ids[200];          // Skin IDs to check
    uint32_t id_count;          // Number of IDs (1-200)
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account to query (empty = first account)
};

// A single skin entry in the response
struct HoardSkinEntry {
    uint32_t id;
    uint8_t unlocked;           // 1 if unlocked, 0 if not
};

// Response: skin unlock query result
struct HoardQuerySkinsResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    uint32_t entry_count;       // Number of entries returned
    HoardSkinEntry entries[200];
};

// Request: query account recipe unlocks (batch)
struct HoardQueryRecipesRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint32_t ids[200];          // Recipe IDs to check
    uint32_t id_count;          // Number of IDs (1-200)
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account to query (empty = first account)
};

// A single recipe entry in the response
struct HoardRecipeEntry {
    uint32_t id;
    uint8_t unlocked;           // 1 if unlocked, 0 if not
};

// Response: recipe unlock query result
struct HoardQueryRecipesResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    uint32_t entry_count;       // Number of entries returned
    HoardRecipeEntry entries[200];
};

// Request: query Wizard's Vault progress
struct HoardQueryWizardsVaultRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    uint8_t type;               // 0 = daily, 1 = weekly, 2 = special
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account to query (empty = first account)
};

// A single Wizard's Vault objective
struct HoardWizardsVaultObjective {
    uint32_t id;
    char title[128];
    char track[32];             // e.g. "PvE", "PvP", "WvW"
    int32_t acclaim;            // Astral Acclaim reward
    int32_t progress_current;
    int32_t progress_complete;
    uint8_t claimed;            // 1 if reward claimed, 0 if not
};

// Response: Wizard's Vault query result
struct HoardQueryWizardsVaultResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    uint8_t type;               // 0 = daily, 1 = weekly, 2 = special
    int32_t meta_progress_current;
    int32_t meta_progress_complete;
    int32_t meta_reward_astral; // Astral Acclaim from meta reward
    uint8_t meta_reward_claimed;
    uint32_t objective_count;   // Number of objectives returned
    HoardWizardsVaultObjective objectives[16]; // Up to 16 objectives
};

// Request: generic authenticated API proxy query
struct HoardQueryApiRequest {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name (used for permission checks)
    char endpoint[256];         // GW2 API path, e.g. "/v2/account/dyes"
    char response_event[64];    // Event name H&S will raise with the response
    char account_name[64];      // Which account's API key to use (empty = first account)
};

// Response: generic API proxy result (raw JSON)
struct HoardQueryApiResponse {
    uint32_t api_version;       // HOARD_API_VERSION
    uint8_t status;             // HOARD_STATUS_OK, HOARD_STATUS_DENIED, or HOARD_STATUS_PENDING
    char account_name[64];      // Echo of queried account name
    char endpoint[256];         // Echo of the requested endpoint
    uint32_t json_length;       // Actual length of the JSON data (may exceed buffer if truncated)
    uint8_t truncated;          // 1 if response was truncated to fit buffer, 0 otherwise
    char json[65536];           // Raw JSON response (up to 64KB, null-terminated)
};

// Context menu: register a menu item
struct HoardContextMenuRegister {
    uint32_t api_version;       // HOARD_API_VERSION
    uint32_t signature;         // Your addon's Nexus signature (AddonDefinition_t.Signature)
    char id[64];                // Unique ID for this menu entry
    char requester[64];         // Addon name (must match across all H&S events)
    char label[64];             // Display text shown in the context menu
    char callback_event[64];    // Event name H&S raises when clicked
    uint8_t item_types;         // HOARD_MENU_ITEMS (1), HOARD_MENU_WALLET (2), or HOARD_MENU_ALL (3)
};

// Context menu: remove a menu item
struct HoardContextMenuRemove {
    uint32_t api_version;       // HOARD_API_VERSION
    char requester[64];         // Addon name
    char id[64];                // ID to remove (empty = remove all from this requester)
};

// Context menu: callback payload (heap-allocated by H&S, caller must free)
struct HoardContextMenuCallback {
    uint32_t api_version;       // HOARD_API_VERSION
    uint32_t item_id;           // GW2 item ID
    char name[128];             // Item name
    char rarity[32];            // e.g. "Rare", "Exotic"
    char type[32];              // e.g. "Weapon", "Armor"
    int32_t total_count;        // Total count across all account locations
};

#pragma pack(pop)
