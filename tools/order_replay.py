"""Generate order files and verify Mini Exchange's printed results."""

import argparse
import difflib
import heapq
import itertools
from pathlib import Path
import random
import re
import sys


INT_MIN = -(2**31)
INT_MAX = 2**31 - 1


def read_int(text, minimum, maximum):
    if not re.fullmatch(r"[+-]?[0-9]+", text):
        raise ValueError(f"expected an integer, got {text!r}")
    value = int(text)
    if not minimum <= value <= maximum:
        raise ValueError(f"{value} is outside [{minimum}, {maximum}]")
    return value


def read_orders(path):
    lines = [(number, line.strip()) for number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ) if line.strip()]
    if not lines:
        raise ValueError("missing order count")
    count = read_int(lines[0][1], 0, INT_MAX)
    if len(lines) - 1 != count:
        raise ValueError(f"header declares {count} orders, found {len(lines) - 1}")
    orders = []
    for number, line in lines[1:]:
        fields = line.split()
        if len(fields) != 3 or fields[0] not in ("buy", "sell"):
            raise ValueError(f"line {number}: expected buy/sell PRICE_CENTS QUANTITY")
        side, price, quantity = fields
        try:
            orders.append((side, read_int(price, INT_MIN, INT_MAX), read_int(quantity, 1, INT_MAX)))
        except ValueError as error:
            raise ValueError(f"line {number}: {error}") from error
    return orders


def match_orders(orders):
    bids, asks, trades = [], [], []
    transaction_id = 0
    for side, price, remaining in orders:
        transaction_id += 1
        order_id = transaction_id
        is_buy = side == "buy"
        own, opposite = (bids, asks) if is_buy else (asks, bids)
        while remaining and opposite:
            priority, resting_id, resting_quantity = opposite[0]
            resting_price = priority if is_buy else -priority
            if (is_buy and price < resting_price) or (not is_buy and price > resting_price):
                break
            filled = min(remaining, resting_quantity)
            remaining -= filled
            transaction_id += 1
            trades.append((transaction_id, resting_price, filled))
            if filled == resting_quantity:
                heapq.heappop(opposite)
            else:
                heapq.heapreplace(opposite, (priority, resting_id, resting_quantity - filled))
        if remaining:
            heapq.heappush(own, (-price if is_buy else price, order_id, remaining))
    return bids, asks, trades


def check(orders_path, output_path):
    orders = read_orders(orders_path)
    bids, asks, trades = match_orders(orders)
    expected = ["Order Book:"]
    # The current CLI prints both sides in ascending price order.
    for label, book, sign in (("Buy", bids, -1), ("Sell", asks, 1)):
        expected.append(f"{label} Orders:")
        for price, order_id, quantity in sorted((sign * p, i, q) for p, i, q in book):
            expected.append(f"{label} Order ID: {order_id}, Ideal Price: {price}, Stock Number: {quantity}")
    expected.append("Trade History:")
    for trade_id, price, quantity in trades:
        expected.append(f"Trade Order ID: {trade_id}, Ideal Price: {price}, Stock Number: {quantity}")

    actual = [line.rstrip() for line in output_path.read_text(encoding="utf-8").splitlines()]
    while actual and not actual[-1]:
        actual.pop()
    if actual == expected:
        print(f"PASS: {len(orders)} orders, {len(trades)} trades, "
            f"{len(bids)} open buys, {len(asks)} open sells")
        return 0
    print("FAIL: simulator output differs from the reference", file=sys.stderr)
    differences = difflib.unified_diff(expected, actual, "expected", str(output_path), lineterm="")
    for line in itertools.islice(differences, 80):
        print(line, file=sys.stderr)
    if next(differences, None) is not None:
        print("(remaining differences omitted)", file=sys.stderr)
    return 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    generate = commands.add_parser("generate", help="write random orders to stdout")
    generate.add_argument("count", type=int)
    generate.add_argument("--seed", type=int, default=42)
    generate.add_argument("--min-price", type=int, default=9800, help="minimum price in cents")
    generate.add_argument("--max-price", type=int, default=9804, help="maximum price in cents")
    generate.add_argument("--max-quantity", type=int, default=50)
    verify = commands.add_parser("check", help="compare simulator stdout with expected results")
    verify.add_argument("orders", type=Path)
    verify.add_argument("output", type=Path)
    args = parser.parse_args()

    if args.command == "generate":
        if not 0 <= args.count <= INT_MAX or not 1 <= args.max_quantity <= INT_MAX:
            generate.error("count must be nonnegative and max quantity must be positive, within int range")
        if not INT_MIN <= args.min_price <= args.max_price <= INT_MAX:
            generate.error("prices must be in int range with min price at most max price")
        random_source = random.Random(args.seed)
        print(args.count)
        for _ in range(args.count):
            side = random_source.choice(("buy", "sell"))
            price = random_source.randint(args.min_price, args.max_price)
            quantity = random_source.randint(1, args.max_quantity)
            print(side, price, quantity)
        return 0

    try:
        return check(args.orders, args.output)
    except (OSError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
