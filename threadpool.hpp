#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <optional>
#include <functional>
#include <condition_variable>

template <typename T>
class BlockingQueue {
public:
	BlockingQueue() = default;

	void push(const T& value) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (!closed_) {
			queue_.push(value);
			pop_trigger_.notify_all();
		}
	}
	std::optional<T> wait_pop() {
		std::unique_lock<std::mutex> lock(mutex_);
		if (!closed_ && queue_.empty()) {
			pop_trigger_.wait(lock);
		}
		if (queue_.empty()) {
			return std::nullopt;
		}
		T tmp = queue_.front();
		queue_.pop();
		return tmp;
	}
	void close() {
		closed_.store(true);
		pop_trigger_.notify_all();
	}
private:
	std::mutex mutex_;
	std::atomic<bool> closed_{ false };
	std::condition_variable pop_trigger_;
	std::queue<T> queue_;
};

class ThreadPool {
public:
    ThreadPool() = default;
    ThreadPool(size_t n) {
        start(n);
    };

    void start(size_t n) {
		while (n--) {
			threads_.emplace_back([this]() { worker(); });
		}
	}
	void submit(std::function<void()> task) {
		task_queue_.push(task);
	}

	void join() {
		task_queue_.close();
		for (auto& thread : threads_) {
			thread.join();
		}
	}

private:

	void worker() {
		while (auto task = task_queue_.wait_pop()) {
			(*task)();
		}
	}

	std::vector<std::thread> threads_;
	BlockingQueue<std::function<void()> > task_queue_;
};