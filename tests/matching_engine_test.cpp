#include <climits>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "matching_engine.hpp"

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

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

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"buy_matches_resting_sell_at_sell_price", buy_matches_resting_sell_at_sell_price},
        {"sell_matches_resting_buy_at_buy_price", sell_matches_resting_buy_at_buy_price},
        {"non_crossing_orders_remain_open", non_crossing_orders_remain_open},
        {"incoming_order_sweeps_multiple_price_levels", incoming_order_sweeps_multiple_price_levels},
        {"incoming_sell_sweeps_multiple_price_levels", incoming_sell_sweeps_multiple_price_levels},
        {"same_price_orders_follow_time_priority", same_price_orders_follow_time_priority},
        {"cancelled_order_is_skipped", cancelled_order_is_skipped},
    };

    int failures = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return 1;
    }

    std::cout << tests.size() << " test(s) passed\n";
    return 0;
}
