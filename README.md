# Mini Exchange

Mini Exchange is a command-line C++ order-book simulator. It accepts limit buy
and sell orders, matches compatible orders using price-time priority, records
trades, supports cancellations, and prints the outstanding book.

## Rules

- Prices are integer cents internally; floating-point prices are never used.
- A buy order states the maximum price the buyer will pay.
- A sell order states the minimum price the seller will accept.
- A trade occurs when the best buy price is at least the best sell price.
- The highest-priced buy and lowest-priced sell have priority.
- Orders at the same price are processed in arrival order.
- A trade executes at the price of the resting order.

## Commands

Commands are read one per line from standard input.

```text
BUY <id> <price-in-cents> <quantity>
SELL <id> <price-in-cents> <quantity>
CANCEL <id>
PRINT
```

`id` is supplied by the caller and uniquely identifies an order. A cancelled,
filled, or unknown ID must fail safely and must not change the book.

Example input:

```text
SELL 1 10100 8
BUY 2 10100 3
BUY 3 10000 10
PRINT
```

Example output:

```text
TRADE: 3 shares at $101.00

SELL
$101.00 | 5

BUY
$100.00 | 10
```

`PRINT` displays outstanding quantity aggregated by price level: sells in
ascending price order, then buys in descending price order. Filled and
cancelled orders are omitted.

## Design

The intended implementation keeps each side of the book in price levels:

- `std::map` ordered descending for buy levels and ascending for sell levels;
- `std::deque` at every level to preserve time priority;
- `std::unordered_map` from order ID to its active order for efficient
  cancellation.

The source layout is:

```text
include/
  order.hpp
  order_book.hpp
  matching_engine.hpp
src/
  main.cpp
  command_parser.cpp
  order_book.cpp
  matching_engine.cpp
tests/
  matching_engine_test.cpp
```

## Build and run

The project requires a C++17 compiler and CMake 3.16 or newer.

```sh
cmake -S . -B build
cmake --build build
./build/order_book < orders.txt
```

## Tests

See the [testing guide](tests/README.md) for build commands, individual tests,
and instructions for generating and checking your own order files.

## Future work

After V1 is robust, possible extensions include CSV replay/import and trade
export, generated-order benchmarks, multiple symbols, amendments, and maximum
quantity risk limits.
