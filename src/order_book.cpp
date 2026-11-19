#include "order_book.hpp"

#include <climits>
#include <stdexcept>

void OrderBook::add_transaction() {
    if (transactionID == INT_MAX) {
        throw std::overflow_error("No sequential transaction IDs remain");
    }
    ++transactionID;
}

int OrderBook::nextOrderID() {
    do {
        add_transaction();
    } while (usedOrderIDs.count(transactionID) != 0);
    return transactionID;
}

OrderBook::OrderBook() {
    buyOrderList.clear();
    buyPriceList.clear();
    sellOrderList.clear();
    sellPriceList.clear();
    tradeHistory.clear();
    transactionID = 0;
}

std::ostream& operator<<(std::ostream& os, const OrderBook& p) {
    os << "Order Book: \n";
    os << "Buy Orders: \n";
    for(auto& [price, orders]: p.buyOrderList){
        for(const BuyOrder& order: orders){
            if(order.getStockNumber() == 0) continue;
            os << order << '\n';
        }
    }
    os << "Sell Orders: \n";
    for(auto& [price, orders]: p.sellOrderList){
        for(const SellOrder& order: orders){
            if(order.getStockNumber() == 0) continue;
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
    return initiateBuyOrder(nextOrderID(), idealPrice, stockNumber);
}

BuyOrder* OrderBook::initiateBuyOrder(int id, int idealPrice, int stockNumber) {
    if (stockNumber <= 0 || !usedOrderIDs.insert(id).second) return nullptr;

    BuyOrder buyOrder(id, idealPrice, stockNumber);
    buyOrderList[buyOrder.getIdealPrice()].push_back(buyOrder);
    buyIterator[id] = (std::list<BuyOrder>::iterator(--buyOrderList[buyOrder.getIdealPrice()].end()));

    buyPriceList.insert(buyOrder.getIdealPrice());
    activeID.insert(buyOrder.getID());

    return &buyOrderList[buyOrder.getIdealPrice()].back();
}

SellOrder* OrderBook::initiateSellOrder(int idealPrice, int stockNumber) {
    if(stockNumber <= 0) return nullptr;
    return initiateSellOrder(nextOrderID(), idealPrice, stockNumber);
}

SellOrder* OrderBook::initiateSellOrder(int id, int idealPrice, int stockNumber) {
    if (stockNumber <= 0 || !usedOrderIDs.insert(id).second) return nullptr;

    SellOrder sellOrder(id, idealPrice, stockNumber);
    sellOrderList[sellOrder.getIdealPrice()].push_back(sellOrder);
    sellIterator[id] = (std::list<SellOrder>::iterator(--sellOrderList[sellOrder.getIdealPrice()].end()));

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
    activeID.erase(active);

    auto buyIt = buyIterator.find(id);
    if(buyIt != buyIterator.end()){
        int price = buyIt->second->getIdealPrice();
        buyOrderList[price].erase(buyIt->second);
        buyIterator.erase(buyIt);
    }

    auto sellIt = sellIterator.find(id);
    if(sellIt != sellIterator.end()){
        int price = sellIt->second->getIdealPrice();
        sellOrderList[price].erase(sellIt->second);
        sellIterator.erase(sellIt);
    }

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

            if(buyOrderList[bestPrice].empty()){
                buyPriceList.erase(bestPrice);
            }

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

            if(sellOrderList[bestPrice].empty()){
                sellPriceList.erase(bestPrice);
            }

            continue;
        }

        return currentSellOrder;
    }
    return nullptr;
}
