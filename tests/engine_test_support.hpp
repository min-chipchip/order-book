#pragma once

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "matching_engine.hpp"

namespace engine_test {

inline void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

enum class Side { buy, sell };

inline Side opposite(Side side) {
    return side == Side::buy ? Side::sell : Side::buy;
}

struct BookOrder {
    int id;
    Side side;
    int price;
    int quantity;

    bool operator==(const BookOrder& other) const {
        return std::tie(id, side, price, quantity) ==
               std::tie(other.id, other.side, other.price, other.quantity);
    }
};

class TestEngine : public matching_engine {
public:
    void submit(Side side, int price, int quantity) {
        if (side == Side::buy) {
            receiveBuyOrder(price, quantity);
        } else {
            receiveSellOrder(price, quantity);
        }
    }

    std::vector<BookOrder> openOrders() const {
        std::ostringstream snapshot;
        snapshot << orderBook;
        std::istringstream lines(snapshot.str());
        std::vector<BookOrder> orders;
        for (std::string line; std::getline(lines, line) && line != "Trade History: ";) {
            if (line.rfind("Buy Order ID: ", 0) != 0 && line.rfind("Sell Order ID: ", 0) != 0) {
                continue;
            }
            BookOrder order{};
            order.side = line.front() == 'B' ? Side::buy : Side::sell;
            const auto fields = line.c_str() + line.find(' ');
            require(std::sscanf(fields, " Order ID: %d, Ideal Price: %d, Stock Number: %d",
                                &order.id, &order.price, &order.quantity) == 3,
                    "could not read open order: " + line);
            orders.push_back(order);
        }
        std::sort(orders.begin(), orders.end(), [](const BookOrder& left, const BookOrder& right) {
            return left.id < right.id;
        });
        return orders;
    }
};

using TradeFields = std::tuple<int, int, int, int>;

inline TradeFields tradeFields(const Trade& trade) {
    return {trade.getBuyID(), trade.getSellID(), trade.getPrice(), trade.getStockNumber()};
}

inline TradeFields expectedTrade(Side restingSide, int restingID, int incomingID,
                                 int price, int quantity) {
    if (restingSide == Side::buy) {
        return {restingID, incomingID, price, quantity};
    }
    return {incomingID, restingID, price, quantity};
}

void runReplay(int operations, unsigned seed);

} // namespace engine_test
