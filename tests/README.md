# Testing Mini Exchange

Run the commands below from the repository root. You'll need a C++17 compiler
and CMake 3.16 or newer. Generating and checking your own order files also
requires Python 3.7 or newer.

## Build and run the tests

```sh
cmake -S . -B build-sanitize -DBUILD_TESTING=ON -DORDER_BOOK_ENABLE_SANITIZERS=ON
cmake --build build-sanitize
(cd build-sanitize && ctest --output-on-failure)
```

CTest should report two passing suites. The sanitizer option enables
AddressSanitizer and UndefinedBehaviorSanitizer on GCC and Clang.

| Suite | What it checks |
| --- | --- |
| `matching_engine` | Price-time priority, resting prices, partial fills, price cutoffs, cancellation, and quantity conservation. Seeded replays compare the engine against a reference matcher after every operation. They run at 10, 100, and 1,000 operations, including three seeds at 1,000. |
| `order_book_cli` | Printed trades and open orders, a 100-order sweep, malformed input, and integer boundaries. |

Run the engine test binary with no argument to see each case by name. To run
one case, pass its name as an argument.

```sh
./build-sanitize/tests/order_book_tests
./build-sanitize/tests/order_book_tests unknown_cancellation_does_not_hide_future_order
./build-sanitize/tests/order_book_tests cancellations_100_operations_preserve_queue_order
./build-sanitize/tests/order_book_tests replay_1000_seed_42
```

Run just the CLI suite with CTest.

```sh
(cd build-sanitize && ctest -R '^order_book_cli$' --output-on-failure)
```

## Generate and check your own orders

Start with the [10-order fixture](data/ten_orders.txt), which produces five
trades, two open buys, and two open sells. Generate larger files with
[`order_replay.py`](../tools/order_replay.py).

```sh
mkdir -p build-sanitize/replays

./build-sanitize/order_book < tests/data/ten_orders.txt > build-sanitize/replays/results-10.txt &&
  python3 tools/order_replay.py check tests/data/ten_orders.txt build-sanitize/replays/results-10.txt

python3 tools/order_replay.py generate 100 --seed 42 > build-sanitize/replays/orders-100.txt
./build-sanitize/order_book < build-sanitize/replays/orders-100.txt > build-sanitize/replays/results-100.txt &&
  python3 tools/order_replay.py check build-sanitize/replays/orders-100.txt build-sanitize/replays/results-100.txt

python3 tools/order_replay.py generate 1000 --seed 1729 \
  --min-price 9800 --max-price 9804 --max-quantity 50 > build-sanitize/replays/orders-1000.txt
./build-sanitize/order_book < build-sanitize/replays/orders-1000.txt > build-sanitize/replays/results-1000.txt &&
  python3 tools/order_replay.py check build-sanitize/replays/orders-1000.txt build-sanitize/replays/results-1000.txt
```

For these settings, the 100-order file produces **72 trades, 18 open buys,
and 9 open sells**. The 1,000-order file produces **812 trades, 72 open buys,
and 101 open sells**. Keep the seed and settings to reproduce a run. Change the
seed to get a new sequence. A narrow price range creates more ties and makes
the dataset useful for checking FIFO.

To write a file by hand, put the number of order rows on the first line. Follow
it with one lowercase `buy` or `sell` per line, a price in integer cents, and a
positive quantity in shares.

```text
3
sell 9800 4
buy 9802 2
buy 9799 5
```

This should produce one trade for 2 shares at 9800, an open sell for 2 shares
at 9800, and an open buy for 5 shares at 9799. Pass your file to the simulator
and checker in place of a generated file. The CLI checker compares the printed
trade sequence and the final open orders. The engine tests also verify the buyer
and seller IDs for each trade and exercise cancellation through the C++ API.

## Diagnose failures

- If CTest fails, use the output from `--output-on-failure` to identify the
  failing case. Run that case by name to isolate it.
- If a seeded engine replay fails, its error shows the seed, the failing
  operation number, and the input sequence through that operation.
- If the Python checker reports `FAIL`, use its diff to find the first
  difference between the expected and actual output. Lines beginning with
  `-` are expected and lines beginning with `+` came from the simulator.
- If the simulator reports an invalid row or the checker reports `ERROR`,
  check the row count, lowercase side, and numeric fields first. The checker
  requires the header to match the number of order rows exactly.
