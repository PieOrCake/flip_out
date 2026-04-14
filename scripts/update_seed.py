#!/usr/bin/env python3
"""
Fetch current GW2 Trading Post prices and merge them into the seed file.

Usage:
    python update_seed.py [--existing seed_prices.json] --output seed_prices.json

The seed format matches Flip Out's PriceDB ExportSeed/ImportSeed:
    { "item_id": [ {"t": unix, "b": buy, "s": sell, "bq": buy_qty, "sq": sell_qty}, ... ], ... }

Trimming: keeps only entries from the last 30 days (oldest-first). If an entry's
timestamp is older than 30 days before the current fetch time, it is discarded.
"""

import argparse
import json
import sys
import time
import urllib.request
import urllib.error

GW2_PRICES_URL = "https://api.guildwars2.com/v2/commerce/prices"
BATCH_SIZE = 200
MAX_AGE_DAYS = 30


def fetch_all_item_ids():
    """Fetch the full list of tradeable item IDs from the GW2 API."""
    with urllib.request.urlopen(GW2_PRICES_URL) as resp:
        return json.loads(resp.read().decode())


def fetch_prices_batch(ids):
    """Fetch prices for a batch of item IDs."""
    ids_param = ",".join(str(i) for i in ids)
    url = f"{GW2_PRICES_URL}?ids={ids_param}"
    try:
        with urllib.request.urlopen(url) as resp:
            return json.loads(resp.read().decode())
    except urllib.error.HTTPError as e:
        print(f"  Warning: HTTP {e.code} for batch of {len(ids)} items, skipping", file=sys.stderr)
        return []


def fetch_all_prices(item_ids):
    """Fetch prices for all items in batches."""
    now = int(time.time())
    snapshots = {}
    total = len(item_ids)

    for i in range(0, total, BATCH_SIZE):
        batch = item_ids[i:i + BATCH_SIZE]
        pct = min(100, int((i + len(batch)) / total * 100))
        print(f"  Fetching prices... {pct}% ({i + len(batch)}/{total})", file=sys.stderr)

        results = fetch_prices_batch(batch)
        for item in results:
            item_id = item.get("id", 0)
            if item_id == 0:
                continue
            buys = item.get("buys", {})
            sells = item.get("sells", {})
            snapshots[str(item_id)] = {
                "t": now,
                "b": buys.get("unit_price", 0),
                "s": sells.get("unit_price", 0),
                "bq": buys.get("quantity", 0),
                "sq": sells.get("quantity", 0),
            }

    return snapshots


def merge_seed(existing, new_snapshots, cutoff_ts):
    """
    Merge new snapshots into the existing seed data.

    - Appends the new snapshot for each item.
    - Removes any entries with timestamp older than cutoff_ts.
    - Entries are kept in chronological order (oldest first).
    """
    merged = {}

    # Start with all item IDs from both sources
    all_ids = set(existing.keys()) | set(new_snapshots.keys())

    for item_id in all_ids:
        entries = list(existing.get(item_id, []))

        # Append new snapshot if we have one
        if item_id in new_snapshots:
            entries.append(new_snapshots[item_id])

        # Remove entries older than cutoff (30 days)
        entries = [e for e in entries if e["t"] >= cutoff_ts]

        # Sort chronologically (oldest first)
        entries.sort(key=lambda e: e["t"])

        if entries:
            merged[item_id] = entries

    return merged


def main():
    parser = argparse.ArgumentParser(description="Update Flip Out seed price data")
    parser.add_argument("--existing", help="Path to existing seed file to merge with")
    parser.add_argument("--output", required=True, help="Output path for the merged seed file")
    args = parser.parse_args()

    # Load existing seed if provided
    existing = {}
    if args.existing:
        try:
            with open(args.existing, "r") as f:
                existing = json.load(f)
            print(f"Loaded existing seed: {len(existing)} items", file=sys.stderr)
        except (FileNotFoundError, json.JSONDecodeError) as e:
            print(f"Warning: could not load existing seed: {e}", file=sys.stderr)

    # Fetch current prices
    print("Fetching item ID list...", file=sys.stderr)
    item_ids = fetch_all_item_ids()
    print(f"Found {len(item_ids)} tradeable items", file=sys.stderr)

    new_snapshots = fetch_all_prices(item_ids)
    print(f"Fetched prices for {len(new_snapshots)} items", file=sys.stderr)

    # Merge and trim to 30 days
    now = int(time.time())
    cutoff = now - (MAX_AGE_DAYS * 86400)
    merged = merge_seed(existing, new_snapshots, cutoff)

    # Count total entries
    total_entries = sum(len(v) for v in merged.values())
    print(f"Merged seed: {len(merged)} items, {total_entries} total entries", file=sys.stderr)

    # Write output (compact, no extra whitespace)
    with open(args.output, "w") as f:
        json.dump(merged, f, separators=(",", ":"))

    print(f"Wrote {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
