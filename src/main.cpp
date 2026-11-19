#include "command_parser.hpp"

#include <iostream>
#include <sstream>
#include <string>

namespace {

int runCountedCli(const std::string& firstLine) {
    std::string line;
    std::istringstream header(firstLine);
    int operations;
    char extra;
    if (!(header >> operations) || operations < 0 || (header >> extra)) {
        std::cerr << "Invalid order count\n";
        return 1;
    }

    CommandParser parser(CommandParser::Format::Counted);
    bool invalid = false;
    for (int index = 0; index < operations; ++index) {
        if (!std::getline(std::cin >> std::ws, line)) {
            std::cerr << "Missing order row " << index + 1 << '\n';
            invalid = true;
            break;
        }

        if (!parser.runCommandCli(line, index + 1)) {
            invalid = true;
        }
    }

    parser.finish();
    return invalid ? 1 : 0;
}

int runUncountedCli(std::string line) {
    CommandParser parser(CommandParser::Format::Uncounted);
    bool invalid = false;
    do {
        if (!parser.runCommandCli(line)) {
            invalid = true;
        }
    } while (std::getline(std::cin, line));
    return invalid ? 1 : 0;
}

} // namespace

int main() {
    std::string firstLine;
    std::getline(std::cin >> std::ws, firstLine);
    if (!firstLine.empty() && firstLine.front() >= 'A' && firstLine.front() <= 'Z') {
        return runUncountedCli(firstLine);
    }
    return runCountedCli(firstLine);
}
