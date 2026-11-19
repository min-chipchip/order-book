#include "order_book.hpp"

#include <climits>
#include <stdexcept>

void OrderBook::add_transaction() {
    do {
        if (transactionID == INT_MAX) {
            throw std::overflow_error("No sequential transaction IDs remain");
        }
        ++transactionID;
    } while (callerOrderIDs.count(transactionID) != 0);
}

bool OrderBook::reserveCallerOrderID(int id) {
    // Sequential IDs are shared by orders and trades. Any value already
    // passed by the counter was generated or reserved by a caller.
    if (id > 0 && id <= transactionID) return false;
    return callerOrderIDs.insert(id).second;
}

OrderBook::OrderBook() {
    buyOrderList.clear();
    buyPriceList.clear();
    sellOrderList.clear();
    sellPriceList.clear();
    canceledID.clear();
    tradeHistory.clear();
    transactionID = 0;
}

std::ostream& operator<<(std::ostream& os, const OrderBook& p) {
    os << "Order Book: \n";
    os << "Buy Orders: \n";
    for(auto& [price, orders]: p.buyOrderList){
        for(const BuyOrder& order: orders){
            if(order.getStockNumber() == 0) continue;
            std::set<int>::iterator itr = p.canceledID.find(order.getID());
            if(itr != p.canceledID.end()) continue;
            os << order << '\n';
        }
    }
    os << "Sell Orders: \n";
    for(auto& [price, orders]: p.sellOrderList){
        for(const SellOrder& order: orders){
            if(order.getStockNumber() == 0) continue;
            std::set<int>::iterator itr = p.canceledID.find(order.getID());
            if(itr != p.canceledID.end()) continue;
            os << order << '\n';
        }
    }
    os << "Trade History: \n";
    for(auto& trade: p.tradeHistory){
        os << trade << '\n';
    }
    return os;
}

BuyOrder* OrderBook::initiateBuyOrder(int idealPrice, int stockNumber) {
    if(stockNumber <= 0) return nullptr;
    add_transaction();
    return insertBuyOrder(transactionID, idealPrice, stockNumber);
}

BuyOrder* OrderBook::initiateBuyOrder(int id, int idealPrice, int stockNumber) {
    if (stockNumber <= 0 || !reserveCallerOrderID(id)) return nullptr;
    return insertBuyOrder(id, idealPrice, stockNumber);
}

BuyOrder* OrderBook::insertBuyOrder(int id, int idealPrice, int stockNumber) {
    BuyOrder buyOrder(id, idealPrice, stockNumber);
    buyOrderList[buyOrder.getIdealPrice()].push_back(buyOrder);
    buyPriceList.insert(buyOrder.getIdealPrice());
    activeID.insert(buyOrder.getID());

    return &buyOrderList[buyOrder.getIdealPrice()].back();
}

SellOrder* OrderBook::initiateSellOrder(int idealPrice, int stockNumber) {
    if(stockNumber <= 0) return nullptr;
    add_transaction();
    return insertSellOrder(transactionID, idealPrice, stockNumber);
}

SellOrder* OrderBook::initiateSellOrder(int id, int idealPrice, int stockNumber) {
    if (stockNumber <= 0 || !reserveCallerOrderID(id)) return nullptr;
    return insertSellOrder(id, idealPrice, stockNumber);
}

SellOrder* OrderBook::insertSellOrder(int id, int idealPrice, int stockNumber) {
    SellOrder sellOrder(id, idealPrice, stockNumber);
    sellOrderList[sellOrder.getIdealPrice()].push_back(sellOrder);
    sellPriceList.insert(sellOrder.getIdealPrice());
    activeID.insert(sellOrder.getID());

    return &sellOrderList[sellOrder.getIdealPrice()].back();
}

Trade OrderBook::initiateTrade(int sellID, int buyID, int price, int stockNumber) {
    add_transaction();
    Trade trade(transactionID, price, stockNumber, sellID, buyID);
    tradeHistory.push_back(trade);

    return trade;
}

const std::vector<Trade>& OrderBook::getTradeHistory() const {
    return tradeHistory;
}

OrderBook::Depth OrderBook::getDepth(std::size_t limit) const {
    const auto collect = [this, limit](auto position, auto end) {
        std::vector<PriceLevel> levels;

        for(; position != end && levels.size() < limit; ++position){
            const auto& [price, orders] = *position;
            std::int64_t quantity = 0;

            for(const auto &order : orders){
                if(activeID.count(order.getID()) != 0){
                    quantity += order.getStockNumber();
                }
            }

            if(quantity > 0){
                levels.push_back({price, quantity});
            }
        }

        return levels;
    };

    return {collect(buyOrderList.rbegin(), buyOrderList.rend()),
            collect(sellOrderList.begin(), sellOrderList.end())};
}

void OrderBook::executeTrade(BuyOrder& buy, SellOrder& sell, int price, int quantity) {
    initiateTrade(sell.getID(), buy.getID(), price, quantity);
    if(buy.applyTrade(quantity)) activeID.erase(buy.getID());
    if(sell.applyTrade(quantity)) activeID.erase(sell.getID());
}

bool OrderBook::cancel(int id) {
    auto active = activeID.find(id);
    if(active == activeID.end()) return false;
    canceledID.insert(id);
    activeID.erase(active);
    return true;
}

int OrderBook::getHighestPrice() {
    if(buyPriceList.empty()) return -1;
    return *buyPriceList.rbegin();
}

int OrderBook::lowestSellPrice() {
    if(sellPriceList.empty()) return INT_MAX;
    return *sellPriceList.begin();
}

BuyOrder* OrderBook::getBestBuyOrder(int SellOrder) {
    while(!buyPriceList.empty()){
        int bestPrice = getHighestPrice();
        if(bestPrice < SellOrder) return nullptr;
        if(buyOrderList[bestPrice].empty()){
            buyPriceList.erase(bestPrice);
            continue;
        }

        BuyOrder* currentBuyOrder = &buyOrderList[bestPrice].front();

        if(currentBuyOrder->getStockNumber() == 0){
            buyOrderList[bestPrice].pop_front();
            continue;
        }

        std::set<int>::iterator itr = canceledID.find(currentBuyOrder->getID());

        if(itr != canceledID.end()){
            buyOrderList[bestPrice].pop_front();
            canceledID.erase(itr);
            continue;
        }

        return currentBuyOrder;
    }
    return nullptr;
}

SellOrder* OrderBook::getBestSellOrder(int BuyOrder) {
    while(!sellPriceList.empty()){
        int bestPrice = lowestSellPrice();
        if(bestPrice > BuyOrder) return nullptr;
        if(sellOrderList[bestPrice].empty()){
            sellPriceList.erase(bestPrice);
            continue;
        }

        SellOrder* currentSellOrder = &sellOrderList[bestPrice].front();

        if(currentSellOrder->getStockNumber() == 0){
            sellOrderList[bestPrice].pop_front();
            continue;
        }

        std::set<int>::iterator itr = canceledID.find(currentSellOrder->getID());

        if(itr != canceledID.end()){
            sellOrderList[bestPrice].pop_front();
            canceledID.erase(itr);
            continue;
        }

        return currentSellOrder;
    }
    return nullptr;
}
