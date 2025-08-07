#ifndef HEAP_CPP
#define HEAP_CPP

#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include "rand_utils.cpp"

template <typename K>
class Heap {
private:
    std::multiset<K, std::greater<K>> heap;

public:
    void add(const K& key) {
        heap.insert(key);
    }

    void remove(const K& key) {
        auto it = heap.find(key);
        if (it != heap.end()) {
            heap.erase(it);
        }
    }

    K get_highest() {
        return *heap.begin();
    }

    void print() {
        for (const auto& key : heap) {
            std::cout << key << " ";
        }
        std::cout << std::endl;
    }

    size_t size() {
        return heap.size();
    }

    // Returns the predecessor (next lower key) of a given key
    std::optional<K> predecessor(const K& key) {
        auto it = heap.upper_bound(key); // first element < key
        if (it != heap.end()) {
            return *it;
        }
        return std::nullopt;
    }

    // Returns the successor (next higher key) of a given key
    std::optional<K> successor(const K& key) {
        auto it = heap.lower_bound(key); // first element <= key
        if (it == heap.begin()) {
            return std::nullopt;
        }
        it--;
        return *it;
    }

};

// Linear search for predecessor in a vector
std::optional<int> linear_predecessor(const std::vector<int>& arr, int key) {
    std::optional<int> result;
    for (int val : arr) {
        if (val < key && (!result.has_value() || val > result.value())) {
            result = val;
        }
    }
    return result;
}

// Linear search for successor in a vector
std::optional<int> linear_successor(const std::vector<int>& arr, int key) {
    std::optional<int> result;
    for (int val : arr) {
        if (val > key && (!result.has_value() || val < result.value())) {
            result = val;
        }
    }
    return result;
}

void test_heap() {
    Heap<int> heap;
    std::vector<int> arr;

    for (int i = 0; i < 100; i++) {
        int val = randint(1, 1000);
        heap.add(val);
        arr.push_back(val);
    }

    for (int i = 0; i < 100; i++) {
        int query = randint(1, 1000);

        auto heap_pred = heap.predecessor(query);
        auto heap_succ = heap.successor(query);

        auto array_pred = linear_predecessor(arr, query);
        auto array_succ = linear_successor(arr, query);

        if (heap_pred != array_pred) {
            std::cout << "Mismatch in predecessor for key " << query << "\n";
            std::cout << "Heap returned: " << (heap_pred.has_value() ? std::to_string(heap_pred.value()) : "nullopt") << "\n";
            std::cout << "Array returned: " << (array_pred.has_value() ? std::to_string(array_pred.value()) : "nullopt") << "\n";
            return;
        }

        if (heap_succ != array_succ) {
            std::cout << "Mismatch in successor for key " << query << "\n";
            std::cout << "Heap returned: " << (heap_succ.has_value() ? std::to_string(heap_succ.value()) : "nullopt") << "\n";
            std::cout << "Array returned: " << (array_succ.has_value() ? std::to_string(array_succ.value()) : "nullopt") << "\n";
            return;
        }
    }

    cout << "PASS\n";
}

#endif
