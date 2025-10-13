#include <chrono>
#include <print>
#include <string>
#include <vector>

#include "getp.hpp"

using namespace std::literals::chrono_literals;

int main() {
    getp::ThreadPool tp{};

    {
        std::print("\n");
        std::string msg{"Olá mundo"};
        tp.dispatch([msg]() { std::print("Mensagem: {}\n", msg); });
    };

    {
        std::print("\n");
        int a{0};
        tp.dispatch([&a]() { a += 42; });
        std::print("a: {}\n", a);
        std::this_thread::sleep_for(1ms);
        std::print("a: {}\n", a);
    };

    {
        std::print("\n");
        int a{0};
        auto task_future_a = tp.submit([&a]() {
            a += 42;
            return nullptr;
        });
        task_future_a.wait();
        std::print("a: {}\n", a);

        auto task_future_b = tp.submit([]() { return 24; });
        int b = task_future_b.get();
        std::print("b: {}\n", b);
    };

    {
        std::print("\n");
        std::vector v = std::ranges::views::iota(10, 15) |
                        std::ranges::to<std::vector<int>>();
        auto task_futures =
            tp.dispatch_on_loop(0, v.size(), [&v](size_t i) { v[i] += 42; });
        task_futures.wait();

        std::print("v: {}\n", v);
    };

    {
        std::print("\n");
        int accum{0};
        std::mutex m{};
        std::vector v = std::ranges::views::iota(10, 15) |
                        std::ranges::to<std::vector<int>>();
        auto task_futures =
            tp.dispatch_on_loop(0, v.size(), [&v, &accum, &m](size_t i) {
                v[i] *= 3;
                {
                    std::scoped_lock<std::mutex> lock{m};
                    accum += v[i];
                }
            });
        task_futures.wait();

        std::print("v: {}\n", v);
        std::print("accum: {}\n", accum);
    };

    {
        std::print("\n");
        auto intervals = getp::utils::calc_intervals(0, 10, 4);
        for (auto interval : intervals) {
            std::print("{}\n", interval);
        }
    };

    return 0;
};
