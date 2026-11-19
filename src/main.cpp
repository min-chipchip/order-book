#include <iostream>
#include <sstream>
#include <string>
#include "matching_engine.hpp"

int main() {
    std::string line;
    std::getline(std::cin >> std::ws, line);
    std::istringstream header(line);
    int operations;
    char extra;
    if (!(header >> operations) || operations < 0 || (header >> extra)) {
        std::cerr << "Invalid order count\n";
        return 1;
    }

    matching_engine engine;
    bool invalid = false;
    for (int index = 0; index < operations; ++index) {
        if (!std::getline(std::cin >> std::ws, line)) {
            std::cerr << "Missing order row " << index + 1 << '\n';
            invalid = true;
            break;
        }

        std::istringstream row(line);
        std::string type;
        int price, quantity;
        if (!(row >> type >> price >> quantity) || (row >> extra)) {
            std::cerr << "Invalid order row " << index + 1 << '\n';
            invalid = true;
            continue;
        }
        if (type != "buy" && type != "sell") {
            std::cerr << "Invalid order type: " << type << '\n';
            invalid = true;
            continue;
        }
        if (quantity <= 0) {
            std::cerr << "Invalid quantity for order row " << index + 1 << '\n';
            invalid = true;
            continue;
        }

        if (type == "buy") {
            engine.receiveBuyOrder(price, quantity);
        } else {
            engine.receiveSellOrder(price, quantity);
        }
    }

    engine.print();
    return invalid ? 1 : 0;
}
