#pragma once
// Hand-written binary Max Heap (array-backed). No std::priority_queue / std::push_heap.
// "Less" defines ordering: the element for which nothing is greater sits at the root.
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Less = std::less<T>>
class MaxHeap {
public:
    explicit MaxHeap(Less less = Less()) : less_(less) {}

    // O(log n)
    void push(const T& value) {
        data_.push_back(value);
        siftUp(data_.size() - 1);
    }

    // O(1)
    const T& top() const {
        if (data_.empty()) throw std::out_of_range("MaxHeap::top on empty heap");
        return data_.front();
    }

    // O(log n): remove and return the maximum
    T pop() {
        if (data_.empty()) throw std::out_of_range("MaxHeap::pop on empty heap");
        T maxValue = std::move(data_.front());
        data_.front() = std::move(data_.back());
        data_.pop_back();
        if (!data_.empty()) siftDown(0);
        return maxValue;
    }

    // O(n): Floyd's bottom-up heap construction
    void build(std::vector<T> items) {
        data_ = std::move(items);
        for (std::size_t i = data_.size() / 2; i-- > 0;) siftDown(i);
    }

    std::size_t size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    void clear() { data_.clear(); }

private:
    std::vector<T> data_;
    Less less_;

    static std::size_t parent(std::size_t i) { return (i - 1) / 2; }

    void siftUp(std::size_t i) {
        while (i > 0) {
            std::size_t p = parent(i);
            if (!less_(data_[p], data_[i])) break;  // parent already >= child
            std::swap(data_[p], data_[i]);
            i = p;
        }
    }

    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        for (;;) {
            std::size_t l = 2 * i + 1, r = l + 1, largest = i;
            if (l < n && less_(data_[largest], data_[l])) largest = l;
            if (r < n && less_(data_[largest], data_[r])) largest = r;
            if (largest == i) break;
            std::swap(data_[i], data_[largest]);
            i = largest;
        }
    }
};
