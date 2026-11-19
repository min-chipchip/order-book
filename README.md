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

`id` is supplied by the caller and uniquely identifies an order for the life
of the book, including after it fills or is canceled. Trade IDs identify
executions separately. Canceling a filled, canceled, or unknown order leaves
the book unchanged.

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

Both CLI formats use uppercase command names. The uncounted CLI reads
commands until end-of-input. The counted CLI starts with the number of
commands. In counted mode, `BUY` and `SELL` accept either
`<id> <price-in-cents> <quantity>` or `<price-in-cents> <quantity>`.
With two arguments, the engine assigns the order ID.

## Design

The intended implementation keeps each side of the book in price levels:

- `std::map` ordered descending for buy levels and ascending for sell levels;
- `std::deque` at every level to preserve time priority;
- `std::unordered_map` from order ID to its active order for efficient
  cancellation.

### Source layout

```text
CMakeLists.txt
benchmark/
  CMakeLists.txt
  benchmark_cancellation.cpp
include/
  command_parser.hpp
  matching_engine.hpp
  order.hpp
  order_book.hpp
media/
  web-buy-order.png
  web-sell-order.png
  web-trade.png
src/
  command_parser.cpp
  main.cpp
  matching_engine.cpp
  order_book.cpp
  web_engine.cpp
tests/
  CMakeLists.txt
  README.md
  data/
    ten_orders.txt
  engine_test_support.hpp
  matching_engine_replay.cpp
  matching_engine_test.cpp
  order_book_cli_test.cmake
  order_book_web_test.cmake
tools/
  order_replay.py
web/
  index.html
  server.py
  styles.css
```

## Build and run

The project requires a C++17 compiler and CMake 3.16 or newer.

```sh
cmake -S . -B build
cmake --build build
./build/order_book < orders.txt
```

## Web interface

Build the C++ adapter and start the local server with Python 3:

```sh
cmake -S . -B build
cmake --build build --target order_book_web
python3 web/server.py build/order_book_web
```

Open [Order Book V1](http://127.0.0.1:8000). The book starts empty and is held
in memory while the server runs. Try selling 8 shares at $100.15 with order ID
`73`, then buying 12 at the same price with order ID `42`. The buy receipt
shows 8 filled and 4 open. Recent Trades shows trade `#1` with buy order
`#42` and sell order `#73`. Enter `42` under **Cancel Order** to remove
the remaining 4 shares from Best Buys.

The [HTML form](web/index.html) posts the side, order ID, limit price, and
quantity to `/orders`. The [Python server](web/server.py) converts the price
to integer cents and sends `buy 42 10015 12` to the running
[C++ adapter](src/web_engine.cpp) over standard input. The cancel form posts
the order ID to `/cancel`, which sends `cancel 42` to the adapter. The
adapter invokes the engine and returns the result as JSON. The server
redirects back to the page and renders the five best price levels on each
side and the ten most recent trades.

### Example orders and trade

**Buy order.** Order `#41` adds a bid for 10 shares at $100.10.

![Open buy order 41 and a bid for 10 shares at $100.10](media/web-buy-order.png)

**Sell order.** Order `#73` adds an ask for 8 shares at $100.15. Both orders
stay open because the buy limit is below the sell limit.

![Open sell order 73 with the bid and ask shown in the order book](media/web-sell-order.png)

**Trade.** Buy order `#42` requests 12 shares at $100.15. It fills 8 against
order `#73`, leaving 4 open. The execution appears in Recent Trades.

![Partial fill of buy order 42 and trade 1 for 8 shares at $100.15](media/web-trade.png)

## Tests

See the [testing guide](tests/README.md) for build commands, individual tests,
and instructions for generating and checking your own order files.

## Benchmarks

Build and run the benchmark in Release mode from the repository root.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target benchmark
./build/benchmark/benchmark
```

The benchmark reports median times in microseconds across 11 trials per
workload size.

- **Buy/sell submissions** replays a fixed-seed batch on a fresh engine for
  each trial and measures the time to process the entire batch.
- **Cancellation burst** queues same-price sell orders, cancels all but the
  last, and measures the cancellation batch and the next matching buy
  separately.

### Results on MacBook Pro (Apple M4)

Recorded on a MacBook Pro with an Apple M4 chip during a run from VS Code.

#### Buy/sell submissions

| Orders | Submit batch (µs) |
| ---: | ---: |
| 100 | 36.167 |
| 1,000 | 677.417 |
| 10,000 | 7,767.833 |
| 100,000 | 70,734.541 |
| 1,000,000 | 742,478.083 |

#### Cancellation burst

| Orders cancelled | Cancel batch (µs) | Next buy (µs) |
| ---: | ---: | ---: |
| 100 | 6.125 | 3.625 |
| 1,000 | 60.583 | 34.334 |
| 10,000 | 844.125 | 377.750 |
| 100,000 | 17,353.958 | 4,012.417 |
| 1,000,000 | 274,366.208 | 70,770.708 |

## Future work

After V1 is robust, possible extensions include CSV replay/import and trade
export, multiple symbols, amendments, and maximum quantity risk limits.
