# getp

Good Enough ThreadPool -- This library is an implementation of a thread pool for studying purposes. It's based on the implementation shown on [1] with some modifications.

## Installation

This is basically a header only library.
Therefore, you just need to include the file `./src/getp.hpp` into your project and use it.

## getp Interface

The `ThreadPool` class has the following interface:

```cpp
class ThreadPool {

   public:
    ThreadPool(unsigned n_threads = std::thread::hardware_concurrency(), unsigned k = 4);

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    ~ThreadPool();

    unsigned get_n_workers();

    template <typename Func>
        requires std::is_invocable_v<Func> && std::is_invocable_r_v<void, Func>
    void dispatch(Func&& func);

    template <typename Func>
        requires std::is_invocable_v<Func>
    auto submit(Func&& func);

    template <typename Func>
        requires std::is_invocable_v<Func, size_t> &&
                 std::is_invocable_r_v<void, Func, size_t>
    TaskFutures dispatch_on_loop(size_t start_index, size_t end_index, Func&& func, unsigned n_blocks = 0);
};
```

The `dispatch` method accepts a function without arguments that returns `void` (`void(void)`) and dispatches it to the thread pool for execution.

`submit` accepts a function without arguments that returns a value of type `T` (`T(void)`) and submits it to the thead pool for evaluation, while returning a `std::future<T>` object for later value aquisition.

Finally, `dispatch_on_loop` accepts the start and end indexes, a function that accepts one argument of type `size_t` (representing a singular index of the execution loop) that returns `void` (`void(size_t)`) and the number of blocks for execution. It subdivides the range defined by the start and end indexes into the desired number of blocks, to be processed by a particular task each. The task is defined by the input function, that operates on the respective block interval. The `dispatch_on_loop` returns an object of type `TaskFutures` representing a vector of type `std::vector<std::future<void>>`. This object has the method `wait()` that blocks the calling thread until all the corresponding tasks have been executed by the thread pool.

## Examples

Next is shown some examples on how to use this library:

```cpp
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

        std::print("v: {}\n", stringify(v));
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

        std::print("v: {}\n", stringify(v));
        std::print("accum: {}\n", accum);
    };

    return 0;
}
```

The same example can be found on the file `./example/main.cpp`.

## References

[1]: "Better Code: Concurrency - Sean Parent" YouTube, uploaded by NDC Conferences, 27 february 2017, https://www.youtube.com/watch?v=zULU6Hhp42w.
