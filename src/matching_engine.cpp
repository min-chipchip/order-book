#include "matching_engine.hpp"

#include <iostream>

matching_engine::matching_engine() {
    orderBook = OrderBook();
}

bool matching_engine::cancel(int id) {
    return orderBook.cancel(id);
}

void matching_engine::receiveBuyOrder(int idealPrice, int stockNumber) {
    submitBuyOrder(idealPrice, stockNumber);
}

std::optional<matching_engine::OrderReceipt> matching_engine::submitBuyOrder(int idealPrice, int stockNumber) {
    return matchAndGetReceipt(orderBook.initiateBuyOrder(idealPrice, stockNumber));
}

std::optional<matching_engine::OrderReceipt> matching_engine::submitBuyOrder(int id, int idealPrice, int stockNumber) {
    return matchAndGetReceipt(orderBook.initiateBuyOrder(id, idealPrice, stockNumber));
}

std::optional<matching_engine::OrderReceipt> matching_engine::matchAndGetReceipt(BuyOrder* order) {
    if (order == nullptr) return std::nullopt;
    const int requested = order->getStockNumber();
    receiveBuyOrder(order);
    const int open = order->getStockNumber();
    return OrderReceipt{order->getID(), requested - open, open};
}

void matching_engine::receiveSellOrder(int idealPrice, int stockNumber) {
    submitSellOrder(idealPrice, stockNumber);
}

std::optional<matching_engine::OrderReceipt> matching_engine::submitSellOrder(int idealPrice, int stockNumber) {
    return matchAndGetReceipt(orderBook.initiateSellOrder(idealPrice, stockNumber));
}

std::optional<matching_engine::OrderReceipt> matching_engine::submitSellOrder(int id, int idealPrice, int stockNumber) {
    return matchAndGetReceipt(orderBook.initiateSellOrder(id, idealPrice, stockNumber));
}

std::optional<matching_engine::OrderReceipt> matching_engine::matchAndGetReceipt(SellOrder* order) {
    if (order == nullptr) return std::nullopt;
    const int requested = order->getStockNumber();
    receiveSellOrder(order);
    const int open = order->getStockNumber();
    return OrderReceipt{order->getID(), requested - open, open};
}

OrderBook::Depth matching_engine::getDepth(std::size_t limit) const {
    return orderBook.getDepth(limit);
}

BuyOrder* matching_engine::getBestBuyOrder(int SellOrder) {
    return orderBook.getBestBuyOrder(SellOrder);
}

SellOrder* matching_engine::getBestSellOrder(int BuyOrder) {
    return orderBook.getBestSellOrder(BuyOrder);
}

const std::vector<Trade>& matching_engine::getTradeHistory() const {
    return orderBook.getTradeHistory();
}

void matching_engine::receiveBuyOrder(BuyOrder* buyOrder) {
    while(buyOrder->getStockNumber() > 0){
        SellOrder* bestSellOrder = orderBook.getBestSellOrder(buyOrder->getIdealPrice());
        if(bestSellOrder == nullptr) break;
        int tradeNumber = bestSellOrder->match(buyOrder->getIdealPrice(), buyOrder->getStockNumber());

        if(tradeNumber == 0) break;

        orderBook.executeTrade(*buyOrder, *bestSellOrder, bestSellOrder->getIdealPrice(), tradeNumber);
    }
}

void matching_engine::receiveSellOrder(SellOrder* sellOrder) {
    while(sellOrder->getStockNumber() > 0){
        BuyOrder* bestBuyOrder = orderBook.getBestBuyOrder(sellOrder->getIdealPrice());
        if(bestBuyOrder == nullptr) break;
        int tradeNumber = bestBuyOrder->match(sellOrder->getIdealPrice(), sellOrder->getStockNumber());

        if(tradeNumber == 0) break;

        orderBook.executeTrade(*bestBuyOrder, *sellOrder, bestBuyOrder->getIdealPrice(), tradeNumber);
    }
}

void matching_engine::print() {
    std::cout << orderBook << std::endl;
}
