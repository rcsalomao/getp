#pragma once

#include <math.h>

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <ranges>
#include <thread>
#include <type_traits>

namespace getp {
using Lock = std::unique_lock<std::mutex>;
using Task = std::move_only_function<void()>;

class TaskFutures {
    std::vector<std::future<void>> futures;

   public:
    TaskFutures() = default;
    void push_back(std::future<void>&& future) {
        futures.push_back(std::move(future));
    }
    void wait() {
        for (auto& future : futures) {
            future.wait();
        }
    }
};

class NotificationQueue {
    std::deque<Task> _queue;
    bool _done{false};
    std::mutex _mutex;
    std::condition_variable _ready;

   public:
    void done() {
        {
            Lock lock{_mutex};
            _done = true;
        }
        _ready.notify_all();
    }

    bool pop(Task& task) {
        Lock lock{_mutex};
        while (_queue.empty() && !_done) _ready.wait(lock);
        if (_queue.empty()) return false;
        task = std::move(_queue.front());
        _queue.pop_front();
        return true;
    }

    bool try_pop(Task& task) {
        Lock lock{_mutex, std::try_to_lock};
        if (!lock || _queue.empty()) return false;
        task = std::move(_queue.front());
        _queue.pop_front();
        return true;
    }

    template <typename Func>
    void push(Func&& func) {
        {
            Lock lock{_mutex};
            _queue.emplace_back(std::forward<Func>(func));
        }
        _ready.notify_one();
    }

    template <typename Func>
    bool try_push(Func&& func) {
        {
            Lock lock{_mutex, std::try_to_lock};
            if (!lock) return false;
            _queue.emplace_back(std::forward<Func>(func));
        }
        _ready.notify_one();
        return true;
    }
};

class ThreadPool {
    const unsigned _count;
    const unsigned _k;
    std::vector<std::thread> _threads;
    std::vector<NotificationQueue> _queues;
    std::atomic<unsigned> _index{0};

    void run(unsigned i) {
        while (true) {
            Task task;
            for (unsigned n = 0; n < _count; ++n) {
                if (!_queues[(i + n) % _count].try_pop(task)) break;
            }
            if (!task && !_queues[i].pop(task)) break;
            task();
        }
    }

   public:
    ThreadPool(unsigned n_threads = std::thread::hardware_concurrency(),
               unsigned k = 4)
        : _count{std::max(1U, n_threads)},
          _k{std::max(1U, k)},
          _queues(_count) {
        for (unsigned i = 0; i < _count; ++i) {
            _threads.emplace_back([this, i] { this->run(i); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool() {
        for (auto& q : _queues) q.done();
        for (auto& t : _threads) t.join();
    }

    unsigned get_num_workers() { return _count; }

    template <typename Func>
        requires std::is_invocable_v<Func> && std::is_invocable_r_v<void, Func>
    void dispatch(Func&& func) {
        auto i = _index++;
        for (unsigned n = 0; n < _count * _k; ++n) {
            if (_queues[(i + n) % _count].try_push(std::forward<Func>(func)))
                return;
        }
        _queues[i % _count].push(std::forward<Func>(func));
    }

    template <typename Func>
        requires std::is_invocable_v<Func>
    auto submit(Func&& func) {
        using ReturnType = std::invoke_result_t<Func>;
        std::promise<ReturnType> p;
        std::future<ReturnType> future = p.get_future();
        dispatch([func = std::move(func), p = std::move(p)] mutable {
            p.set_value(func());
        });
        return future;
    }

    template <typename Func>
        requires std::is_invocable_v<Func, size_t> &&
                 std::is_invocable_r_v<void, Func, size_t>
    TaskFutures dispatch_on_loop(size_t start_index, size_t end_index,
                                 Func&& func, unsigned n_blocks = 0) {
        assert(start_index < end_index);
        unsigned n_blocos =
            !n_blocks ? get_num_workers() : std::max(1U, n_blocks);
        auto tf = TaskFutures{};
        size_t range_length = end_index - start_index;
        if (range_length > n_blocos) {
            double dv = (double)range_length / n_blocos;
            auto intervals = std::views::iota(0U, n_blocos) |
                             std::views::transform([dv](unsigned i) {
                                 return std::pair<unsigned, unsigned>{
                                     round(i * dv), round((i + 1) * dv)};
                             });
            for (auto interval : intervals) {
                std::promise<void> p;
                tf.push_back(p.get_future());
                dispatch([interval, func = std::move(func),
                          p = std::move(p)] mutable {
                    for (auto i :
                         std::views::iota(interval.first, interval.second)) {
                        func(i);
                    }
                    p.set_value();
                });
            }
        } else {
            for (auto i : std::views::iota(start_index, end_index)) {
                std::promise<void> p;
                tf.push_back(p.get_future());
                dispatch([i, func = std::move(func), p = std::move(p)] mutable {
                    func(i);
                    p.set_value();
                });
            }
        }
        return tf;
    };
};
}  // namespace getp
