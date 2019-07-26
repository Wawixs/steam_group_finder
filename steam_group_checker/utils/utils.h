#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>

template <class T> class blocking_queue : public std::queue<T> {
public:
	blocking_queue(int size) {
		max_size = size;
	}

	void push(T item) {
		std::unique_lock<std::mutex> wlck(writer_mutex);
		while (full())
			is_full.wait(wlck);
		std::queue<T>::push(item);
		is_empty.notify_all();
	}

	bool empty() {
		return std::queue<T>::empty();
	}

	bool full() {
		return std::queue<T>::size() >= max_size;
	}

	T pop() {
		std::unique_lock<std::mutex> lck(reader_mutex);
		while (std::queue<T>::empty()) {
			is_empty.wait(lck);
		}
		T value = std::queue<T>::back();
		std::queue<T>::pop();
		if (!full())
			is_full.notify_all();
		return value;
	}

private:
	int max_size;
	std::mutex reader_mutex;
	std::mutex writer_mutex;
	std::condition_variable is_full;
	std::condition_variable is_empty;
};

inline std::vector<std::string> split(std::string c, std::string cc) {
	std::vector<std::string> v;
	size_t pos = 0;
	std::string token = {};
	while ((pos = c.find(cc)) != std::string::npos) {
		token = c.substr(0, pos);
		v.emplace_back(token);
		c.erase(0, pos + cc.length());
	}
	v.emplace_back(c);
	return v;
}