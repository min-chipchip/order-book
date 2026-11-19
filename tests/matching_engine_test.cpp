#include <climits>
#include <functional>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine_test_support.hpp"

namespace {

using engine_test::BookOrder;
using engine_test::Side;
using engine_test::TestEngine;
using engine_test::expectedTrade;
using engine_test::opposite;
using engine_test::require;
using engine_test::tradeFields;

const Trade& onlyTrade(const matching_engine& engine) {
    const auto& trades = engine.getTradeHistory();
    require(trades.size() == 1, "expected exactly one trade");
    return trades.front();
}

void buy_matches_resting_sell_at_sell_price() {
    matching_engine engine;
    engine.receiveSellOrder(90, 4);
    engine.receiveBuyOrder(100, 4);

    const Trade& trade = onlyTrade(engine);
    require(trade.getSellID() == 1, "trade should retain the resting sell order ID");
    require(trade.getBuyID() == 2, "trade should retain the incoming buy order ID");
    require(trade.getPrice() == 90, "incoming buy should execute at resting sell price");
    require(trade.getStockNumber() == 4, "trade quantity should equal the matched quantity");
    require(engine.getBestSellOrder(INT_MAX) == nullptr, "filled sell order should leave the book");
}

void sell_matches_resting_buy_at_buy_price() {
    matching_engine engine;
    engine.receiveBuyOrder(100, 4);
    engine.receiveSellOrder(90, 4);

    const Trade& trade = onlyTrade(engine);
    require(trade.getSellID() == 2, "trade should retain the incoming sell order ID");
    require(trade.getBuyID() == 1, "trade should retain the resting buy order ID");
    require(trade.getPrice() == 100, "incoming sell should execute at resting buy price");
    require(trade.getStockNumber() == 4, "trade quantity should equal the matched quantity");
    require(engine.getBestBuyOrder(INT_MIN) == nullptr, "filled buy order should leave the book");
}

void non_crossing_orders_remain_open() {
    matching_engine engine;
    engine.receiveBuyOrder(90, 5);
    engine.receiveSellOrder(100, 5);

    require(engine.getTradeHistory().empty(), "non-crossing orders must not trade");
    BuyOrder* buy = engine.getBestBuyOrder(INT_MIN);
    SellOrder* sell = engine.getBestSellOrder(INT_MAX);
    require(buy != nullptr && buy->getIdealPrice() == 90 && buy->getStockNumber() == 5,
            "unmatched buy should remain unchanged");
    require(sell != nullptr && sell->getIdealPrice() == 100 && sell->getStockNumber() == 5,
            "unmatched sell should remain unchanged");
}

void incoming_order_sweeps_multiple_price_levels() {
    matching_engine engine;
    engine.receiveSellOrder(90, 3);
    engine.receiveSellOrder(95, 4);
    engine.receiveBuyOrder(100, 10);

    const auto& trades = engine.getTradeHistory();
    require(trades.size() == 2, "buy should trade against every eligible price level");
    require(trades[0].getPrice() == 90 && trades[0].getStockNumber() == 3,
            "best sell price must execute first");
    require(trades[1].getPrice() == 95 && trades[1].getStockNumber() == 4,
            "next eligible sell price must execute after the first is filled");
    BuyOrder* remainder = engine.getBestBuyOrder(INT_MIN);
    require(remainder != nullptr && remainder->getStockNumber() == 3,
            "unfilled buy quantity should rest in the book");
}

void incoming_sell_sweeps_multiple_price_levels() {
    matching_engine engine;
    engine.receiveBuyOrder(100, 3);
    engine.receiveBuyOrder(95, 4);
    engine.receiveSellOrder(90, 10);

    const auto& trades = engine.getTradeHistory();
    require(trades.size() == 2, "sell should trade against every eligible price level");
    require(trades[0].getPrice() == 100 && trades[0].getStockNumber() == 3,
            "best buy price must execute first");
    require(trades[1].getPrice() == 95 && trades[1].getStockNumber() == 4,
            "next eligible buy price must execute after the first is filled");
    SellOrder* remainder = engine.getBestSellOrder(INT_MAX);
    require(remainder != nullptr && remainder->getStockNumber() == 3,
            "unfilled sell quantity should rest in the book");
}

void same_price_orders_follow_time_priority() {
    matching_engine engine;
    engine.receiveBuyOrder(100, 2);
    engine.receiveBuyOrder(100, 5);
    engine.receiveSellOrder(90, 3);

    const auto& trades = engine.getTradeHistory();
    require(trades.size() == 2, "sell should consume both eligible buy orders");
    require(trades[0].getBuyID() == 1 && trades[0].getStockNumber() == 2,
            "earlier order at the same price must fill first");
    require(trades[1].getBuyID() == 2 && trades[1].getStockNumber() == 1,
            "later order should receive the remaining quantity");
    BuyOrder* remainder = engine.getBestBuyOrder(INT_MIN);
    require(remainder != nullptr && remainder->getID() == 2 && remainder->getStockNumber() == 4,
            "partially filled later order should retain its remainder");
}

void cancelled_order_is_skipped() {
    matching_engine engine;
    engine.receiveBuyOrder(100, 2);
    engine.receiveBuyOrder(99, 2);
    engine.cancel(1);
    engine.receiveSellOrder(90, 2);

    const Trade& trade = onlyTrade(engine);
    require(trade.getBuyID() == 2, "cancelled best-priced order must not trade");
    require(trade.getPrice() == 99, "next eligible buy should set the execution price");
    require(engine.getBestBuyOrder(INT_MIN) == nullptr, "all active buy quantity should be filled");
}

void non_crossing_book_keeps_all_50_orders() {
    std::vector<BookOrder> input;
    for (int index = 0; index < 25; ++index) {
        input.push_back({0, Side::buy, 75 + index, index + 1});
        input.push_back({0, Side::sell, 100 + index, index + 1});
    }
    std::mt19937 random(42);
    std::shuffle(input.begin(), input.end(), random);

    TestEngine engine;
    int id = 0;
    for (auto& order : input) {
        order.id = ++id;
        engine.submit(order.side, order.price, order.quantity);
    }
    require(engine.getTradeHistory().empty(), "non-crossing book must not trade");
    require(engine.openOrders() == input, "all 50 orders should remain open");
    const auto* bid = engine.getBestBuyOrder(INT_MIN);
    const auto* ask = engine.getBestSellOrder(INT_MAX);
    require(bid && bid->getIdealPrice() == 99, "expected best bid at 99");
    require(ask && ask->getIdealPrice() == 100, "expected best ask at 100");
}

void crossing_boundary_uses_resting_price() {
    for (Side restingSide : {Side::buy, Side::sell}) {
        for (int difference = -2; difference <= 2; ++difference) {
            TestEngine engine;
            const int buyPrice = 100 + difference;
            const int restingPrice = restingSide == Side::buy ? buyPrice : 100;
            const int incomingPrice = restingSide == Side::buy ? 100 : buyPrice;
            engine.submit(restingSide, restingPrice, 3);
            engine.submit(opposite(restingSide), incomingPrice, 3);

            if (difference < 0) {
                require(engine.getTradeHistory().empty(), "a price below the ask must not cross");
                require(engine.openOrders().size() == 2, "both orders should rest");
            } else {
                require(tradeFields(onlyTrade(engine)) == expectedTrade(restingSide, 1, 2, restingPrice, 3),
                        "crossing order must trade at the resting price");
                require(engine.openOrders().empty(), "both orders should fill");
            }
        }
    }
}

void sweep_100_orders_respects_limit() {
    for (Side restingSide : {Side::buy, Side::sell}) {
        std::vector<int> prices;
        for (int price = 100; price <= 108; ++price) {
            prices.insert(prices.end(), 11, price);
        }
        std::mt19937 random(42);
        std::shuffle(prices.begin(), prices.end(), random);

        TestEngine engine;
        const int limit = restingSide == Side::buy ? 101 : 107;
        const int excludedPrice = restingSide == Side::buy ? 100 : 108;
        std::vector<BookOrder> eligible;
        std::vector<BookOrder> expectedOpen;
        int id = 0;
        for (int price : prices) {
            engine.submit(restingSide, price, 1);
            BookOrder order{++id, restingSide, price, 1};
            (price == excludedPrice ? expectedOpen : eligible).push_back(order);
        }
        std::stable_sort(eligible.begin(), eligible.end(), [restingSide](const auto& left, const auto& right) {
            return restingSide == Side::buy ? left.price > right.price : left.price < right.price;
        });
        engine.submit(opposite(restingSide), limit, 90);

        const auto& trades = engine.getTradeHistory();
        require(trades.size() == 88, "sweep should fill all 88 eligible orders");
        for (std::size_t index = 0; index < eligible.size(); ++index) {
            const auto& order = eligible[index];
            require(tradeFields(trades[index]) == expectedTrade(restingSide, order.id, 100, order.price, 1),
                    "incorrect price-time priority at sweep trade " + std::to_string(index + 1));
        }
        expectedOpen.push_back({100, opposite(restingSide), limit, 2});
        require(engine.openOrders() == expectedOpen, "ineligible prices and incoming remainder must stay open");
    }
}

void fifo_250_orders_preserves_arrival_order() {
    for (Side restingSide : {Side::buy, Side::sell}) {
        TestEngine engine;
        for (int id = 1; id <= 249; ++id) {
            engine.submit(restingSide, 100, 2);
        }
        engine.submit(opposite(restingSide), 100, 497);

        const auto& trades = engine.getTradeHistory();
        require(trades.size() == 249, "FIFO sweep should reach all 249 resting orders");
        for (int id = 1; id <= 249; ++id) {
            const int quantity = id == 249 ? 1 : 2;
            require(tradeFields(trades[id - 1]) == expectedTrade(restingSide, id, 250, 100, quantity),
                    "incorrect FIFO trade for order " + std::to_string(id));
        }
        require(engine.openOrders() == std::vector<BookOrder>{{249, restingSide, 100, 1}},
                "the last resting order should retain one share");
    }
}

void unknown_cancellation_does_not_hide_future_order() {
    for (Side restingSide : {Side::buy, Side::sell}) {
        TestEngine engine;
        require(!engine.cancel(1), "unknown order must be rejected");
        engine.submit(restingSide, 100, 2);
        require(engine.openOrders() == std::vector<BookOrder>{{1, restingSide, 100, 2}},
                "unknown cancellation must not hide a future order");

        engine.submit(opposite(restingSide), 100, 1);
        require(tradeFields(onlyTrade(engine)) == expectedTrade(restingSide, 1, 2, 100, 1),
                "order must remain eligible to trade");
        require(!engine.cancel(2), "filled incoming order must be inactive immediately");
        require(engine.cancel(1), "partially filled order must remain cancellable");
        require(!engine.cancel(1), "duplicate cancellation must fail before queue cleanup");
        require(engine.openOrders().empty(), "canceled remainder must be absent from the snapshot");
        require(engine.getTradeHistory().size() == 1, "cancellation must preserve completed trades");
    }
}

void cancellations_100_operations_preserve_queue_order() {
    for (Side restingSide : {Side::buy, Side::sell}) {
        TestEngine engine;
        std::set<int> canceledIDs{1, 60};
        for (int id = 2; id <= 36; id += 2) {
            canceledIDs.insert(id);
        }
        std::vector<BookOrder> survivors;
        for (int id = 1; id <= 60; ++id) {
            engine.submit(restingSide, 100, 1);
            if (canceledIDs.count(id) == 0) {
                survivors.push_back({id, restingSide, 100, 1});
            }
        }
        for (int id : canceledIDs) {
            require(engine.cancel(id), "expected to cancel active order " + std::to_string(id));
        }
        require(engine.openOrders() == survivors, "snapshot should omit all 20 canceled orders");
        engine.submit(opposite(restingSide), 100, 40);
        const auto& trades = engine.getTradeHistory();
        require(trades.size() == 40, "only the 40 surviving orders should trade");
        for (std::size_t index = 0; index < survivors.size(); ++index) {
            require(tradeFields(trades[index]) == expectedTrade(restingSide, survivors[index].id, 61, 100, 1),
                    "cancellation changed the surviving queue order");
        }
        require(engine.openOrders().empty(), "all surviving orders should be filled");
        const auto history = trades;
        for (int id : {59, 61, 3, 5, 7, 9, 11, 13, 1, 2, 4, 6, 8, 10, 12, 60, 0, 102, INT_MAX}) {
            require(!engine.cancel(id), "expected inactive order " + std::to_string(id));
            require(engine.openOrders().empty(), "unsuccessful cancellation changed the book");
            require(trades.size() == history.size() &&
                    std::equal(trades.begin(), trades.end(), history.begin(), [](const Trade& left, const Trade& right) {
                        return left.getID() == right.getID() && tradeFields(left) == tradeFields(right);
                    }), "unsuccessful cancellation changed the trade history");
        }
    }
}

void nonpositive_quantities_are_rejected() {
    TestEngine engine;
    for (Side side : {Side::buy, Side::sell}) {
        for (int quantity : {0, -1, -2, INT_MIN}) {
            engine.submit(side, 100, quantity);
            require(engine.getTradeHistory().empty(), "invalid quantity must not trade");
            require(engine.openOrders().empty(), "invalid quantity must not rest");
        }
    }
    engine.submit(Side::buy, 100, 3);
    engine.submit(Side::sell, 100, 2);
    require(tradeFields(onlyTrade(engine)) == expectedTrade(Side::buy, 1, 2, 100, 2),
            "valid orders should still trade after invalid submissions");
    require(engine.openOrders() == std::vector<BookOrder>{{1, Side::buy, 100, 1}},
            "only the valid buy remainder should be open");
}

void depth_aggregates_only_active_orders() {
    matching_engine engine;
    engine.receiveBuyOrder(95, 2);
    engine.receiveBuyOrder(95, 4);
    engine.receiveBuyOrder(105, 10);
    require(engine.cancel(3), "expected to cancel the highest buy");
    engine.receiveBuyOrder(90, 1);
    engine.receiveSellOrder(100, 5);
    engine.receiveSellOrder(110, 8);
    engine.receiveSellOrder(120, 6);

    const auto receipt = engine.submitBuyOrder(100, 7);
    require(receipt && receipt->id == 8 && receipt->filled == 5 && receipt->open == 2,
            "expected the incoming order ID and immediate fill quantities");
    require(onlyTrade(engine).getID() == 9, "expected a separate ID for the trade");

    const auto depth = engine.getDepth(2);

    require(depth.buys.size() == 2 && depth.buys[0].price == 100 && depth.buys[0].quantity == 2 &&
            depth.buys[1].price == 95 && depth.buys[1].quantity == 6,
            "buy depth should exclude canceled orders and aggregate same-price quantities");
    require(depth.sells.size() == 2 && depth.sells[0].price == 110 && depth.sells[0].quantity == 8 &&
            depth.sells[1].price == 120 && depth.sells[1].quantity == 6,
            "sell depth should omit the filled price level and sort the remainder");
}

void caller_order_ids_stay_reserved() {
    OrderBook book;
    require(book.initiateSellOrder(1, 100, 0) == nullptr,
            "an invalid quantity must leave the caller ID available");
    SellOrder* sell = book.initiateSellOrder(1, 100, 2);
    BuyOrder* buy = book.initiateBuyOrder(3, 100, 3);
    require(sell && sell->getID() == 1 && buy && buy->getID() == 3,
            "both sides should store the caller's ID");
    require(book.initiateBuyOrder(1, 100, 1) == nullptr,
            "an active sell ID cannot be reused for a buy");

    const BuyOrder* generated = book.initiateBuyOrder(99, 1);
    require(generated && generated->getID() == 2, "automatic IDs should skip caller IDs");
    require(book.initiateSellOrder(2, 105, 1) == nullptr,
            "a generated order ID cannot be reused");

    book.executeTrade(*buy, *sell, 100, 2);
    const auto& trades = book.getTradeHistory();
    require(trades.size() == 1 && trades.front().getID() == 4 &&
            trades.front().getBuyID() == 3 && trades.front().getSellID() == 1,
            "the trade should get its own ID and retain both order IDs");
    require(book.initiateBuyOrder(1, 100, 1) == nullptr, "a filled ID must stay reserved");
    require(book.initiateSellOrder(4, 105, 1) == nullptr, "a trade ID cannot be reused");
    require(book.cancel(3), "a partially filled order should be cancellable by its caller ID");

    const SellOrder* highId = book.initiateSellOrder(INT_MAX, 105, 1);
    require(highId && highId->getID() == INT_MAX, "a high caller ID should be accepted");
    require(book.cancel(INT_MAX), "a sell should be cancellable by its caller ID");
    require(book.initiateSellOrder(INT_MAX, 105, 1) == nullptr, "a canceled ID must stay reserved");
    const SellOrder* next = book.initiateSellOrder(105, 1);
    require(next && next->getID() == 5, "a high caller ID must not advance the generated sequence");
}

void caller_order_ids_flow_through_matching_engine() {
    constexpr int restingID = 42;
    constexpr int incomingID = 73;
    for (Side restingSide : {Side::buy, Side::sell}) {
        matching_engine engine;
        const auto submit = [&engine](Side side, int id, int price, int quantity) {
            return side == Side::buy ? engine.submitBuyOrder(id, price, quantity)
                                     : engine.submitSellOrder(id, price, quantity);
        };
        const int restingPrice = 100;
        const int crossingPrice = restingSide == Side::buy ? 99 : 101;
        const auto resting = submit(restingSide, restingID, restingPrice, 2);
        require(resting && resting->id == restingID && resting->filled == 0 && resting->open == 2,
                "a resting order receipt should contain the caller's ID");
        require(!submit(opposite(restingSide), restingID, crossingPrice, 3),
                "the engine should reject a duplicate ID on the opposite side");
        require(engine.getTradeHistory().empty(), "a rejected crossing order must not trade");

        const auto incoming = submit(opposite(restingSide), incomingID, crossingPrice, 3);
        require(incoming && incoming->id == incomingID && incoming->filled == 2 && incoming->open == 1,
                "a partially filled order receipt should contain the caller's ID");
        require(tradeFields(onlyTrade(engine)) == expectedTrade(restingSide, restingID, incomingID, restingPrice, 2),
                "a trade should record both caller IDs at the resting price");
        require(engine.cancel(incomingID), "the remainder should be cancellable by its caller ID");
        const auto depth = engine.getDepth(5);
        require(depth.buys.empty() && depth.sells.empty(), "cancellation should remove the remainder");
    }
}

} // namespace

int main(int argc, char* argv[]) {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"buy_matches_resting_sell_at_sell_price", buy_matches_resting_sell_at_sell_price},
        {"sell_matches_resting_buy_at_buy_price", sell_matches_resting_buy_at_buy_price},
        {"non_crossing_orders_remain_open", non_crossing_orders_remain_open},
        {"incoming_order_sweeps_multiple_price_levels", incoming_order_sweeps_multiple_price_levels},
        {"incoming_sell_sweeps_multiple_price_levels", incoming_sell_sweeps_multiple_price_levels},
        {"same_price_orders_follow_time_priority", same_price_orders_follow_time_priority},
        {"cancelled_order_is_skipped", cancelled_order_is_skipped},
        {"non_crossing_book_keeps_all_50_orders", non_crossing_book_keeps_all_50_orders},
        {"crossing_boundary_uses_resting_price", crossing_boundary_uses_resting_price},
        {"sweep_100_orders_respects_limit", sweep_100_orders_respects_limit},
        {"fifo_250_orders_preserves_arrival_order", fifo_250_orders_preserves_arrival_order},
        {"unknown_cancellation_does_not_hide_future_order", unknown_cancellation_does_not_hide_future_order},
        {"cancellations_100_operations_preserve_queue_order", cancellations_100_operations_preserve_queue_order},
        {"nonpositive_quantities_are_rejected", nonpositive_quantities_are_rejected},
        {"depth_aggregates_only_active_orders", depth_aggregates_only_active_orders},
        {"caller_order_ids_stay_reserved", caller_order_ids_stay_reserved},
        {"caller_order_ids_flow_through_matching_engine", caller_order_ids_flow_through_matching_engine},
        {"replay_10_seed_42", [] { engine_test::runReplay(10, 42); }},
        {"replay_100_seed_42", [] { engine_test::runReplay(100, 42); }},
        {"replay_1000_seed_42", [] { engine_test::runReplay(1000, 42); }},
        {"replay_1000_seed_1729", [] { engine_test::runReplay(1000, 1729); }},
        {"replay_1000_seed_2026", [] { engine_test::runReplay(1000, 2026); }},
    };

    if (argc > 2) {
        std::cerr << "Usage: order_book_tests [test_name]\n";
        return 1;
    }
    int ran = 0;
    int failures = 0;
    for (const auto& [name, test] : tests) {
        if (argc == 2 && name != argv[1]) {
            continue;
        }
        ++ran;
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    if (ran == 0) {
        std::cerr << "Unknown test: " << argv[1] << '\n';
        return 1;
    }
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << ran << " test(s) passed\n";
    return 0;
}
