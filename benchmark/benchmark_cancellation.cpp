#include "matching_engine.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr unsigned seed = 42;

struct BenchmarkOrder {
    bool isBuy;
    int price;
    int quantity;
};

std::vector<BenchmarkOrder> generateOrders(int count) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> side(0, 1);
    std::uniform_int_distribution<int> price(1, 1000);
    std::uniform_int_distribution<int> quantity(1, 20);

    std::vector<BenchmarkOrder> orders;
    orders.reserve(count);
    for (int i = 0; i < count; ++i) {
        orders.push_back({side(rng) == 0, price(rng), quantity(rng)});
    }
    return orders;
}

double runNoDelete(const std::vector<BenchmarkOrder>& orders) {
    using Clock = std::chrono::steady_clock;
    using Microseconds = std::chrono::duration<double, std::micro>;

    matching_engine engine;
    const auto start = Clock::now();
    for (const auto& order : orders) {
        if (order.isBuy) {
            engine.receiveBuyOrder(order.price, order.quantity);
        } else {
            engine.receiveSellOrder(order.price, order.quantity);
        }
    }
    return Microseconds(Clock::now() - start).count();
}

void no_delete() {
    constexpr int trialCount = 11;
    constexpr int orderCounts[] = {100, 1'000, 10'000, 100'000, 1'000'000};

    std::cout << "\nBuy/sell submissions (median of " << trialCount << " trials)\n"
            << std::left << std::setw(16) << "Orders"
            << "Submit batch (us)\n"
            << std::fixed << std::setprecision(3);
    for (int orderCount : orderCounts) {
        const auto orders = generateOrders(orderCount);
        std::array<double, trialCount> submissionTimes;
        for (int trial = 0; trial < trialCount; ++trial) {
            submissionTimes[trial] = runNoDelete(orders);
        }
        std::sort(submissionTimes.begin(), submissionTimes.end());
        std::cout << std::left << std::setw(16) << orderCount
                << submissionTimes[trialCount / 2] << '\n';
    }
}

struct BurstTiming {
    double cancellationUs;
    double nextBuyUs;
};

BurstTiming runCancellationBurst(int cancelCount) {
    using Clock = std::chrono::steady_clock;
    using Microseconds = std::chrono::duration<double, std::micro>;

    constexpr int price = 10'000;
    const int lastSellId = cancelCount + 1;
    matching_engine engine;
    for (int id = 1; id <= lastSellId; ++id) {
        if (!engine.submitSellOrder(id, price, 1)) {
            throw std::runtime_error("Could not set up the sell queue");
        }
    }

    int cancelled = 0;
    const auto beforeCancels = Clock::now();
    for (int id = 1; id < lastSellId; ++id) {
        cancelled += engine.cancel(id);
    }
    const auto afterCancels = Clock::now();
    const auto buy = engine.submitBuyOrder(lastSellId + 1, price, 1);
    const auto afterBuy = Clock::now();

    const auto& trades = engine.getTradeHistory();
    if (cancelled != cancelCount || !buy || buy->filled != 1 ||
        trades.size() != 1 || trades.front().getSellID() != lastSellId) {
        throw std::runtime_error("Expected one fill against the last sell");
    }

    return {Microseconds(afterCancels - beforeCancels).count(),
            Microseconds(afterBuy - afterCancels).count()};
}

void cancellationBurst() {
    constexpr int trialCount = 11;
    constexpr int burstSizes[] = {100, 1'000, 10'000, 100'000, 1'000'000};

    std::cout << "\nCancellation burst (median of " << trialCount << " trials)\n"
            << std::left << std::setw(16) << "Cancelled"
            << std::setw(20) << "Cancel batch (us)"
            << "Next buy (us)\n"
            << std::fixed << std::setprecision(3);
    for (int cancelCount : burstSizes) {
        std::array<double, trialCount> cancellationTimes;
        std::array<double, trialCount> buyTimes;
        for (int trial = 0; trial < trialCount; ++trial) {
            const auto timing = runCancellationBurst(cancelCount);
            cancellationTimes[trial] = timing.cancellationUs;
            buyTimes[trial] = timing.nextBuyUs;
        }
        std::sort(cancellationTimes.begin(), cancellationTimes.end());
        std::sort(buyTimes.begin(), buyTimes.end());
        std::cout << std::left << std::setw(16) << cancelCount
            << std::setw(20) << cancellationTimes[trialCount / 2]
            << buyTimes[trialCount / 2] << '\n';
    }
}

} // namespace

int main() {
    no_delete();
    cancellationBurst();

    return 0;
}
