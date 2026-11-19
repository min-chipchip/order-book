#include "engine_test_support.hpp"

#include <climits>
#include <map>
#include <random>

namespace engine_test {
namespace {

struct ReferenceOrder {
    BookOrder open;
    int submitted;
    int canceled = 0;
};

// A linear scan keeps the model independent of the engine's price queues.
struct ReferenceBook {
    std::vector<ReferenceOrder> orders;
    int lastID = 0;

    ReferenceOrder* best(Side side) {
        ReferenceOrder* bestOrder = nullptr;
        for (auto& order : orders) {
            if (order.open.side != side || order.open.quantity == 0) {
                continue;
            }
            if (!bestOrder || (side == Side::buy ? order.open.price > bestOrder->open.price
                                                  : order.open.price < bestOrder->open.price)) {
                bestOrder = &order;
            }
        }
        return bestOrder;
    }

    std::vector<TradeFields> submit(Side side, int price, int quantity) {
        BookOrder incoming{++lastID, side, price, quantity};
        std::vector<TradeFields> trades;
        while (incoming.quantity > 0) {
            auto* match = best(opposite(side));
            if (!match || (side == Side::buy ? match->open.price > price : match->open.price < price)) {
                break;
            }
            const int filled = std::min(match->open.quantity, incoming.quantity);
            match->open.quantity -= filled;
            incoming.quantity -= filled;
            trades.push_back(expectedTrade(match->open.side, match->open.id, incoming.id, match->open.price, filled));
            ++lastID;
        }
        orders.push_back({incoming, quantity});
        return trades;
    }

    bool cancel(int id) {
        for (auto& order : orders) {
            if (order.open.id == id && order.open.quantity > 0) {
                order.canceled = order.open.quantity;
                order.open.quantity = 0;
                return true;
            }
        }
        return false;
    }
};

int cancelTarget(const ReferenceBook& reference, std::mt19937& random) {
    const bool wantActive = std::bernoulli_distribution(0.70)(random);
    if (!wantActive && std::bernoulli_distribution(0.5)(random)) {
        return reference.lastID + 1;
    }
    std::vector<int> candidates;
    for (const auto& order : reference.orders) {
        if ((order.open.quantity > 0) == wantActive) {
            candidates.push_back(order.open.id);
        }
    }
    if (candidates.empty()) {
        return reference.lastID + 1;
    }
    return candidates[std::uniform_int_distribution<std::size_t>(0, candidates.size() - 1)(random)];
}

void checkBest(const Order* actual, const ReferenceOrder* expected, const std::string& side) {
    if (!expected) {
        require(actual == nullptr, "expected an empty " + side + " side");
        return;
    }
    const auto& order = expected->open;
    require(actual && actual->getID() == order.id && actual->getIdealPrice() == order.price &&
            actual->getStockNumber() == order.quantity, "incorrect best " + side + " order");
}

} // namespace

void runReplay(int operations, unsigned seed) {
    enum class Action { buy, sell, cancel };
    const int cancellations = operations / 10;
    const int buys = (operations - cancellations) / 2;
    std::vector<Action> actions(buys, Action::buy);
    actions.insert(actions.end(), operations - cancellations - buys, Action::sell);
    actions.insert(actions.end(), cancellations, Action::cancel);
    std::mt19937 random(seed);
    std::shuffle(actions.begin(), actions.end(), random);
    std::uniform_int_distribution<int> prices(98, 102);
    std::uniform_int_distribution<int> quantities(1, 50);

    TestEngine engine;
    ReferenceBook reference;
    std::map<int, int> executed;
    std::ostringstream input;

    for (std::size_t step = 0; step < actions.size(); ++step) {
        try {
            const auto previousTrades = engine.getTradeHistory().size();
            std::vector<TradeFields> expectedTrades;
            if (actions[step] == Action::cancel) {
                const int id = cancelTarget(reference, random);
                input << "cancel " << id << '\n';
                require(engine.cancel(id) == reference.cancel(id), "unexpected cancellation result");
            } else {
                const auto side = actions[step] == Action::buy ? Side::buy : Side::sell;
                const int price = prices(random);
                const int quantity = quantities(random);
                input << (side == Side::buy ? "buy " : "sell ") << price << ' ' << quantity << '\n';
                expectedTrades = reference.submit(side, price, quantity);
                engine.submit(side, price, quantity);
            }

            const auto& trades = engine.getTradeHistory();
            require(trades.size() == previousTrades + expectedTrades.size(), "incorrect number of trades");
            for (std::size_t i = 0; i < expectedTrades.size(); ++i) {
                const auto& trade = trades[previousTrades + i];
                require(tradeFields(trade) == expectedTrades[i], "incorrect trade " + std::to_string(i + 1));
                require(trade.getStockNumber() > 0, "trade quantity must be positive");
                executed[trade.getBuyID()] += trade.getStockNumber();
                executed[trade.getSellID()] += trade.getStockNumber();
            }

            const auto actualOpen = engine.openOrders();
            std::vector<BookOrder> expectedOpen;
            for (const auto& order : reference.orders) {
                if (order.open.quantity > 0) {
                    expectedOpen.push_back(order.open);
                }
            }
            require(actualOpen == expectedOpen, "open orders differ from the reference book");
            auto open = actualOpen.begin();
            for (const auto& order : reference.orders) {
                int remaining = 0;
                if (open != actualOpen.end() && open->id == order.open.id) {
                    remaining = (open++)->quantity;
                }
                require(executed[order.open.id] + remaining + order.canceled == order.submitted,
                        "quantity is not conserved for order " + std::to_string(order.open.id));
            }

            const auto* bid = engine.getBestBuyOrder(INT_MIN);
            const auto* ask = engine.getBestSellOrder(INT_MAX);
            checkBest(bid, reference.best(Side::buy), "buy");
            checkBest(ask, reference.best(Side::sell), "sell");
            require(!bid || !ask || bid->getIdealPrice() < ask->getIdealPrice(), "book remains crossed");
        } catch (const std::runtime_error& error) {
            throw std::runtime_error("seed " + std::to_string(seed) + ", operation " +
                                     std::to_string(step + 1) + ": " + error.what() + "\n" + input.str());
        }
    }
}

} // namespace engine_test
