#pragma once

#include <mutex>
#include <queue>

template <typename T>
class LockQueue {
   public:
    LockQueue() {}
    ~LockQueue() {}
    LockQueue(const LockQueue&) = delete;
    LockQueue& operator=(const LockQueue&) = delete;
    LockQueue(LockQueue&&) = delete;
    LockQueue& operator=(LockQueue&&) = delete;

    bool Empty() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.empty();
    }

    int Size() {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    template <typename Arg>
    void Enqueue(Arg&& t) {
        std::lock_guard<std::mutex> lock(mtx_);
        queue_.push(std::forward<Arg>(t));
    }

    bool Dequeue(T& t) {
        std::lock_guard<std::mutex> lock(mtx_);

        if (queue_.empty()) {
            return false;
        }
        t = std::move(queue_.front());

        queue_.pop();
        return true;
    }

   private:
    std::queue<T> queue_;
    std::mutex mtx_;
};