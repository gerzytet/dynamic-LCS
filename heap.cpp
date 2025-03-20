#ifndef HEAP_CPP
#define HEAP_CPP

#include <iostream>
#include <map>

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
};

#endif
