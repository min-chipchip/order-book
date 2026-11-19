#pragma once

#include "matching_engine.hpp"

#include <string>

class CommandParser {
public:
    enum class Format { Counted, Uncounted };

    explicit CommandParser(Format format);
    bool runCommandCli(const std::string& line, int rowNumber = 0);
    void finish();

private:
    Format format;
    matching_engine engine;
    bool printed = false;
};
