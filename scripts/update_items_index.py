#!/usr/bin/env python3
"""
Build a tradeable items name index for the Alter Ego mobile app.

Usage:
    python update_items_index.py --output tradeable_items.json

Fetches all tradeable item IDs from the GW2 commerce/prices endpoint, then
fetches item details (name, rarity, icon) from the items endpoint in batches.

Output format (compact JSON array):
    [{"id": 24, "n": "Vial of Weak Blood", "r": "Fine", "i": "https://..."}, ...]
"""

import argparse
import json
import sys
import urllib.request
import urllib.error

GW2_PRICES_URL = "https://api.guildwars2.com/v2/commerce/prices"
GW2_ITEMS_URL = "https://api.guildwars2.com/v2/items"
BATCH_SIZE = 200


def fetch_tradeable_ids():
    """Fetch the full list of tradeable item IDs from the GW2 API."""
    with urllib.request.urlopen(GW2_PRICES_URL) as resp:
        return json.loads(resp.read().decode())


def fetch_items_batch(ids):
    """Fetch item details for a batch of item IDs."""
    ids_param = ",".join(str(i) for i in ids)
    url = f"{GW2_ITEMS_URL}?ids={ids_param}"
    try:
        with urllib.request.urlopen(url) as resp:
            return json.loads(resp.read().decode())
    except urllib.error.HTTPError as e:
        print(f"  Warning: HTTP {e.code} for batch of {len(ids)} items, skipping", file=sys.stderr)
        return []


def main():
    parser = argparse.ArgumentParser(description="Build tradeable items name index")
    parser.add_argument("--output", required=True, help="Output path for the items index file")
    args = parser.parse_args()

    # Fetch tradeable item IDs
    print("Fetching tradeable item IDs...", file=sys.stderr)
    item_ids = fetch_tradeable_ids()
    print(f"Found {len(item_ids)} tradeable items", file=sys.stderr)

    # Fetch item details in batches
    items = []
    total = len(item_ids)

    for i in range(0, total, BATCH_SIZE):
        batch = item_ids[i:i + BATCH_SIZE]
        pct = min(100, int((i + len(batch)) / total * 100))
        print(f"  Fetching item details... {pct}% ({i + len(batch)}/{total})", file=sys.stderr)

        results = fetch_items_batch(batch)
        for item in results:
            item_id = item.get("id", 0)
            name = item.get("name", "")
            if item_id == 0 or not name:
                continue
            items.append({
                "id": item_id,
                "n": name,
                "r": item.get("rarity", ""),
                "i": item.get("icon", ""),
            })

    # Sort by ID for deterministic output
    items.sort(key=lambda x: x["id"])

    print(f"Built index: {len(items)} items", file=sys.stderr)

    # Write output (compact JSON)
    with open(args.output, "w") as f:
        json.dump(items, f, separators=(",", ":"), ensure_ascii=False)

    print(f"Wrote {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
