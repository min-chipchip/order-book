
#include "command_parser.hpp"

#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace {

void printPrice(int cents) {
    std::cout << '$' << cents / 100 << '.' << std::setfill('0')
            << std::setw(2) << cents % 100 << std::setfill(' ');
}

void printBook(const matching_engine& engine) {
    const auto depth = engine.getDepth(std::numeric_limits<std::size_t>::max());
    const auto printLevels = [](const std::vector<OrderBook::PriceLevel>& levels) {
        for (const auto& [price, quantity] : levels) {
            printPrice(price);
            std::cout << " | " << quantity << '\n';
        }
    };
    std::cout << "SELL\n";
    printLevels(depth.sells);
    std::cout << "\nBUY\n";
    printLevels(depth.buys);
}

} // namespace

CommandParser::CommandParser(Format format) : format(format) {}

bool CommandParser::runCommandCli(const std::string& line, int rowNumber) {
    const bool counted = format == Format::Counted;
    std::istringstream row(line);
    std::string type;
    char extra;
    const auto invalidRow = [&] {
        if (counted) {
            std::cerr << "Invalid order row " << rowNumber << '\n';
        } else {
            std::cerr << "Invalid command: " << type << '\n';
        }
        return false;
    };
    if (!(row >> type)) {
        return !counted || invalidRow();
    }

    if (type == "PRINT") {
        if (row >> extra) return invalidRow();
        if (counted) {
            engine.print();
            printed = true;
        } else {
            printBook(engine);
        }
        return true;
    }

    if (type == "CANCEL") {
        int id;
        if (!(row >> id) || (row >> extra)) return invalidRow();
        if (!engine.cancel(id) && !counted) {
            std::cerr << "Cannot cancel order " << id << '\n';
            return false;
        }
        return true;
    }

    const bool isBuy = type == "BUY";
    if (!isBuy && type != "SELL") {
        if (!counted) return invalidRow();
        std::cerr << "Invalid order type: " << type << '\n';
        return false;
    }

    int firstArgument, secondArgument;
    if (!(row >> firstArgument >> secondArgument)) return invalidRow();
    row >> std::ws;
    // Counted commands allow both "BUY price quantity" and "BUY id price quantity".
    const bool hasID = !counted || !row.eof();
    const int price = hasID ? secondArgument : firstArgument;
    int quantity = secondArgument;
    if (hasID && (!(row >> quantity) || (row >> extra))) {
        return invalidRow();
    }
    if (quantity <= 0) {
        if (!counted) return invalidRow();
        std::cerr << "Invalid quantity for order row " << rowNumber << '\n';
        return false;
    }
    if (!counted && price < 0) return invalidRow();
    const auto before = engine.getTradeHistory().size();
    std::optional<matching_engine::OrderReceipt> receipt;
    if (isBuy) {
        receipt = hasID ? engine.submitBuyOrder(firstArgument, price, quantity)
                        : engine.submitBuyOrder(price, quantity);
    } else {
        receipt = hasID ? engine.submitSellOrder(firstArgument, price, quantity)
                        : engine.submitSellOrder(price, quantity);
    }
    if (!receipt) {
        if (!hasID) return invalidRow();
        std::cerr << "Duplicate order ID " << firstArgument << '\n';
        return false;
    }
    if (counted) return true;

    const auto& trades = engine.getTradeHistory();
    for (auto index = before; index < trades.size(); ++index) {
        std::cout << "TRADE: " << trades[index].getStockNumber() << " shares at ";
        printPrice(trades[index].getPrice());
        std::cout << "\n\n";
    }
    return true;
}

void CommandParser::finish() {
    if (format == Format::Counted && !printed) {
        engine.print();
    }
}
