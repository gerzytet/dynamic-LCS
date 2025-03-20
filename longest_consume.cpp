#include <iostream>
#include <cmath>
#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>

using std::cout;
using std::min;
using sdsl::construct_im;

#ifndef LONGEST_CONSUME_CPP
#define LONGEST_CONSUME_CPP

#include "global.hpp"
#include "slice.cpp"
#include "rand_utils.cpp"

//return the length of the prefix from s that T can consume
long longest_consume(const CST &T, const string &s) {
    long index = 0;
    Node node = T.root();
    while (index < s.size()) {
        Node child = T.child(node, s[index]);
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
Slice longest_consume_slice(const CST &T, const string &s, long len) {
    long index = 0;
    Node node = T.root();
    while (index < s.size()) {
        Node child = T.child(node, s[index]);
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
                long d = T.depth(T.leftmost_leaf(child));
                long start = len - d + 1;
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

void test_longest_consume_slice() {
    for (int i = 0; i < 100; i++) {
        int len = randint(2, 500);
        string s = randstring(len, 'a', 'd');
        CST cst;
        construct_im(cst, s, 1);

        for (int j = 0; j < 500; j++) {
            Slice slice = randslice(len);
            Slice found = longest_consume_slice(cst, slice.apply(s) + 'e', len);

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

long fuse_prefix_dummy(CST &T, Slice u, Slice v, const string &s) {
    return longest_consume(T, u.apply(s) + v.apply(s));
}

Slice fuse_prefix_dummy_slice(CST &T, Slice u, Slice v, const string &s, long t_len) {
    return longest_consume_slice(T, u.apply(s) + v.apply(s), t_len);
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

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            Slice u = randslice(length);
            Slice v = randslice(length);

            long len = fuse_prefix_dummy(cst, u, v, t);
            Slice ans = fuse_prefix_dummy_slice(cst, u, v, t, t.size());

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
