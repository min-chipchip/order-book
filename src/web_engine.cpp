#include "matching_engine.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

void writeLevels(const std::vector<OrderBook::PriceLevel>& levels) {
    const char* separator = "";
    std::cout << '[';
    for (const auto& [price, quantity] : levels) {
        std::cout << separator << '[' << price << ',' << quantity << ']';
        separator = ",";
    }
    std::cout << ']';
}

void writeSnapshot(const matching_engine& engine) {
    const auto depth = engine.getDepth(5);
    std::cout << "{\"buys\":";
    writeLevels(depth.buys);
    std::cout << ",\"sells\":";
    writeLevels(depth.sells);

    const auto& trades = engine.getTradeHistory();
    const auto count = std::min<std::size_t>(10, trades.size());
    std::cout << ",\"trades\":[";
    for (std::size_t i = 0; i < count; ++i) {
        const auto& trade = trades[trades.size() - 1 - i];
        if (i != 0) std::cout << ',';
        std::cout << '[' << trade.getID() << ',' << trade.getPrice()
                  << ',' << trade.getStockNumber() << ',' << trade.getBuyID()
                  << ',' << trade.getSellID() << ']';
    }
    std::cout << "]}";
}

void handleRequest(matching_engine& engine, const std::string& line) {
    std::istringstream request(line);
    std::string command, extra;
    request >> command;
    if (command == "snapshot" && !(request >> extra)) {
        writeSnapshot(engine);
        return;
    }

    int id;
    if (command == "cancel" && (request >> id) && !(request >> extra)) {
        std::cout << "{\"canceled\":" << (engine.cancel(id) ? "true" : "false") << '}';
        return;
    }

    int price, quantity;
    if ((command != "buy" && command != "sell") ||
        !(request >> id >> price >> quantity) || (request >> extra) || price <= 0 || quantity <= 0) {
        std::cout << "{\"error\":\"Invalid engine request\"}";
        return;
    }

    const auto receipt = command == "buy" ? engine.submitBuyOrder(id, price, quantity)
                                           : engine.submitSellOrder(id, price, quantity);
    if (!receipt) {
        std::cout << "{\"accepted\":false}";
        return;
    }
    std::cout << "{\"id\":" << receipt->id << ",\"filled\":" << receipt->filled
              << ",\"open\":" << receipt->open << '}';
}

}  // namespace

int main() {
    matching_engine engine;
    std::string line;
    while (std::getline(std::cin, line)) {
        handleRequest(engine, line);
        std::cout << std::endl;
    }
}
