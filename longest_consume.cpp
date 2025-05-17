#include <iostream>
#include <cmath>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>
#include <unordered_map>
#include <optional>
#include <string_view>

using std::cout;
using std::optional;
using std::min;
using std::unordered_map;
using sdsl::construct_im;
using std::string_view;

#ifndef LONGEST_CONSUME_CPP
#define LONGEST_CONSUME_CPP

#include "global.hpp"
#include "slice.cpp"
#include "rand_utils.cpp"

class ChildIndex {
    private:
    unordered_map<long long, Node> hashmap;
    long long hash(Node node, char c) const {
        return (long long) node * 256 + c;
    }
    Node root;

    public:

    Node lookup(Node node, char c) const {
        auto iter = hashmap.find(hash(node, c));
        if (iter == hashmap.end()) {
            return root;
        }
        return iter->second;
    }

    ChildIndex(CST &T) {
        #ifdef USE_CHILD_INDEX
        root = T.root();
        for (Node node : T) {
            if (node != T.root()) {
                Node parent = T.parent(node);
                char c = T.edge(node, T.depth(parent) + 1);
                hashmap[hash(parent, c)] = node;
            }
        }
        #endif

    }

    ChildIndex() {}
};

#ifdef USE_CHILD_INDEX
    #define LOOKUP_CHILD(T, child_index, node, c) child_index.lookup(node, c);
#else
    #define LOOKUP_CHILD(T, child_index, node, c) T.child(node, c)
#endif

//return the length of the prefix from s that T can consume
long longest_consume(const CST &T, const string &s, const ChildIndex &child_index) {
    long index = 0;
    Node node = T.root();
    while (index < s.size()) {
        //Node child = T.child(node, s[index]);
        Node child = LOOKUP_CHILD(T, child_index, node, s[index]);
        if (child == T.root()) {
            return index;
        }
        long limit = min(T.depth(child), s.size());
        while (index < limit) {
            if (s[index] == T.edge(child, index+1)) {
                index++;
            } else {
                return index;
            }
        }
        node = child;
    }
    return index;
}

//len is length of t
//return the prefix from s that T can consume as a slice
Slice longest_consume_slice(const CST &T, string_view s, long len, ChildIndex &child_index) {
    long index = 0;
    Node node = T.root();
    while (index < s.size()) {
        //Node child = T.child(node, s[index]);
        Node child = LOOKUP_CHILD(T, child_index, node, s[index]);
        if (child == T.root()) {
            if (index == 0) {
                return {-1, -1};
            }
            long start = len - T.depth(T.leftmost_leaf(node)) + 1;
            long end = start + index - 1;
            return {start, end};
        }
        long limit = min(T.depth(child), s.size());
        while (index < limit) {
            if (s[index] == T.edge(child, index+1)) {
                index++;
            } else {
                if (index == 0) {
                    return {-1, -1};
                }
                long start = len - T.depth(T.leftmost_leaf(child)) + 1;
                long end = start + index - 1;
                return {start, end};
            }
        }
        node = child;
    }
    if (index == 0) {
        return {-1, -1};
    }
    long start = len - T.depth(T.leftmost_leaf(node)) + 1;
    long end = start + index - 1;
    return {start, end};
}

//len is length of t
//return the prefixes from s that T can consume as a slice
vector<Slice> longest_consume_slices(const CST &T, string_view s, long len, int limit, ChildIndex &child_index) {
    long index = 0;
    Node node = T.root();
    auto output = [&](Node node) {
        vector<Slice> ans;
        auto rb = T.rb(node);
        auto lb = T.lb(node);
        if (limit != -1) {
            rb = min(rb, lb + limit - 1);
        }
        for (long i = lb; i <= rb; i++) {
            long start = len - T.depth(T.select_leaf(i+1)) + 1;
            long end = start + index - 1;
            ans.push_back({start, end});
        }
        return ans;
    };
    while (index < s.size()) {
        //Node child = T.child(node, s[index]);
        Node child = LOOKUP_CHILD(T, child_index, node, s[index]);
        if (child == T.root()) {
            if (index == 0) {
                return {};
            }
            return output(node);
        }
        long limit = min(T.depth(child), s.size());
        while (index < limit) {
            if (s[index] == T.edge(child, index+1)) {
                index++;
            } else {
                if (index == 0) {
                    return {};
                }
                return output(child);
            }
        }
        node = child;
    }
    if (index == 0) {
        return {};
    }
    return output(node);
}

void test_longest_consume_slice() {
    for (int i = 0; i < 100; i++) {
        int len = randint(2, 500);
        string s = randstring(len, 'a', 'd');
        CST cst;
        construct_im(cst, s, 1);
        auto child_index = ChildIndex(cst);

        for (int j = 0; j < 500; j++) {
            Slice slice = randslice(len);
            Slice found = longest_consume_slice(cst, slice.apply(s) + 'e', len, child_index);

            if (!found.is_inbounds(s.size())) {
                cout << "FAIL NOT IN BOUNDS\n";
                return;
            }
            if (slice.apply(s) != found.apply(s)) {
                cout << "FAIL NOT EQUAL\n";
                return;
            }
        }
    }
    cout << "PASS\n";
}

void test_longest_consume_slices() {
    for (int i = 0; i < 100; i++) {
        int len = randint(2, 500);
        string s = randstring(len, 'a', 'd');
        CST cst;
        construct_im(cst, s, 1);
        auto child_index = ChildIndex(cst);

        for (int j = 0; j < 500; j++) {
            Slice slice = randslice(len);
            vector<Slice> founds = longest_consume_slices(cst, slice.apply(s) + 'e', len, -1, child_index);

            for (auto found : founds) {
                if (!found.is_inbounds(s.size())) {
                    cout << "FAIL NOT IN BOUNDS\n";
                    return;
                }
                if (slice.apply(s) != found.apply(s)) {
                    cout << "FAIL NOT EQUAL\n";
                    return;
                }
            }
        }
    }
    cout << "PASS\n";
}

long fuse_prefix_dummy(CST &T, Slice u, Slice v, const string &s, ChildIndex &child_index) {
    return longest_consume(T, u.apply(s) + v.apply(s), child_index);
}

Slice fuse_prefix_dummy_slice(CST &T, Slice u, Slice v, const string &s, long t_len, ChildIndex &child_index) {
    return longest_consume_slice(T, u.apply(s) + v.apply(s), t_len, child_index);
}

void test_fuse_prefix() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        int length = randint(4, 10);
        string s = "";
        string t = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
            t += (char) randint('a', 'c');
        }
        CST cst;
        construct_im(cst, t, 1);
        auto child_index = ChildIndex(cst);

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            Slice u = randslice(length);
            Slice v = randslice(length);

            long len = fuse_prefix_dummy(cst, u, v, t, child_index);
            Slice ans = fuse_prefix_dummy_slice(cst, u, v, t, t.size(), child_index);

            if (len != ans.size()) {
                cout << "FAIL\n";
                cout << "Test " << (i*RUNS_PER_STRING + j) << ": " << u.apply(t) << " + " << v.apply(t) << " = " << len << "or" << ans.size() << '\n';
                break;
            }
        }
    }

    cout << "PASS\n";
}

#endif
