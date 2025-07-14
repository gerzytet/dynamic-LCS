#include <unordered_map>
#include <utility>

using std::unordered_map;
using std::pair;
using std::vector;
using sdsl::wt_int;

#include "global.hpp"
#include "longest_consume.cpp"
#include "sdsl/wt_int.hpp"

#ifndef HIA_CPP
#define HIA_CPP

struct Void {
    //not-equal operator
    bool operator!=(const Void& other) const {
        return false;
    }
};
using HIAInducedIndex = wt_int<>;

struct TwoSet {
    unordered_map<int, int> counts;
    long twos = 0;

    void add(long label) {
        counts[label]++;
        if (counts[label] == 2) {
            twos++;
        }
    }

    void remove(long label) {
        counts[label]--;
        if (counts[label] == 1) {
            twos--;
        }
    }

    void update(long label, int sign) {
        if (sign == -1) {
            remove(label);
        } else {
            add(label);
        }
    }
};

struct HIAResult {
    Node HIA_1;
    Node HIA_2;
    long sum;

    HIAResult(Node HIA_1, Node HIA_2, long sum) {
        this->HIA_1 = HIA_1;
        this->HIA_2 = HIA_2;
        this->sum = sum;
    }
};

void debug_print_node(const CST &T, Node node) {
    for (int i = 0; i < T.depth(node); i++) {
        char c = T.edge(node, i+1);
        cout << (c ? c : '$');
    }
    cout << '\n';
}

typedef unsigned long(*label_mapper)(const CST &T, Node node, unsigned long length);
unsigned long cst_label_mapper(const CST &T, Node node, unsigned long size) {
    return size - T.depth(node) + 2;
}

unsigned long reverse_cst_label_mapper(const CST &T, Node node, unsigned long size) {
    return T.depth(node);
}

//Gets a wavelet tree representing all leaves from the 2 trees with the same labels
//a point (x, y) existing in the tree means that T1_mapper of leaf #x and T2 mapper of leaf #y are equal
//in other words, leaf number x and leaf number y have the same label once mapped
template<label_mapper T1_mapper, label_mapper T2_mapper>
HIAInducedIndex get_induced_index_(const CST &T1, const CST &T2, long length) {
    //format: values[t1_value] = t2_value
    //NOTE: specifying a fixed integer width for the int_vector here bugs it out
    //idk why
    sdsl::int_vector<> values(length+1);

    auto T1_iter = T1.csa.isa.begin();
    auto T2_iter = --T2.csa.isa.end();
    for (long i = 0; i <= length; i++) {
        values[*T1_iter] = *T2_iter;

        T1_iter++;
        T2_iter--;
    }

    HIAInducedIndex index;
    construct_im(index, values);

    return index;
}

HIAInducedIndex get_induced_index(const CST &T_R, const CST &T, long length) {
    return get_induced_index_<reverse_cst_label_mapper, cst_label_mapper>(T_R, T, length);
}

bool is_induced(const HIAInducedIndex &index, const CST &T1, const CST &T2, Node u, Node v) {
    //debug_print_node(T1, u);
    //debug_print_node(T2, v);
    //cout << "args " << T1.lb(u) << ' ' << T1.rb(u) << ' ' << T2.lb(v) << ' ' << T2.rb(v) << '\n';
    auto result = index.range_search_2d(T1.lb(u), T1.rb(u), T2.lb(v), T2.rb(v), true);
    //for (auto point : result.second) {
    //    cout << point << '\n';
    //}
    //auto result = index.range_search_2d(T2.lb(v), T2.rb(v), T1.lb(u), T1.rb(u), false);
    return result.first > 0;
    /*return rt.countInRange(
        {T1.lb(u), T2.lb(v)},
        {T1.rb(u), T2.rb(v)},
        {true, true},
        {true, true}
    ) > 0;*/
}

template<label_mapper mapper>
void flip_subtree_excluding_child(const CST &T, TwoSet &ts, Node root, Node child, int sign, long length) {
    long left = T.lb(root);
    long right = T.rb(root);

    long c_left = T.lb(child);
    long c_right = T.rb(child);

    for (unsigned long i = left; i < c_left; i++) {
        ts.update(mapper(T, T.select_leaf(i+1), length), sign);
    }

    for (unsigned long i = c_right+1; i <= right; i++) {
        ts.update(mapper(T, T.select_leaf(i+1), length), sign);
    }
}

//version of the select_leaf function that works with leaf 0
Node select_leaf(CST T, long i) {
    if (i == 0) {
        auto iter = T.begin();
        iter++;
        if (!T.is_leaf(*iter) || T.depth(*iter) != 1) {
            cout << "ERROR: CST STRUCTURE NOT AS EXPECTED\n";
            exit(1);
        }
        return *iter;
    } else {
        return T.select_leaf(i);
    }
}

template<label_mapper T1_mapper, label_mapper T2_mapper>
HIAResult dummy_HIA(const HIAInducedIndex &index, const CST &T1, const CST &T2, Node u, Node v, long u_length, long v_length, long length) {
    Node t2_root = T2.root();

    vector<Node> v_path;
    Node v_temp = v;
    while (v_temp != t2_root) {
        v_path.push_back(v_temp);
        v_temp = T2.parent(v_temp);
    }
    //v path now contains the path from v to the root, excluding the root
    //the goal is to step from the root down to v, so we iterate through this list backwards.
    auto v_path_iter = v_path.rbegin();
    auto v_path_end = v_path.rend();

    Node HIA_1 = u;
    Node HIA_2 = v_temp;
    long HIA_sum = min((uint64_t)u_length, T1.depth(u)) + min((uint64_t)v_length, T2.depth(v_temp));

    Node u_temp = u;
    auto u_depth = T1.depth(u_temp);
    bool u_depth_dirty = false;
    while (true) {
        if (is_induced(index, T1, T2, u_temp, v_temp)) {
            if (u_depth_dirty) {
                u_depth = T1.depth(u_temp);
                u_depth_dirty = false;
            }
            long sum = min((uint64_t)u_length, u_depth) + min((uint64_t)v_length, T2.depth(v_temp));
            if (sum > HIA_sum) {
                HIA_1 = u_temp;
                HIA_2 = v_temp;
                HIA_sum = sum;
            }

            if (v_path_iter == v_path_end) {
                break;
            }
            Node v_child = *v_path_iter;
            v_path_iter++;
            v_temp = v_child;
        } else {
            if (u_temp == T1.root()) {
                break;
            }
            u_temp = T1.parent(u_temp);
            u_depth_dirty = true;
        }
    }

    return HIAResult(HIA_1, HIA_2, HIA_sum);
}

//first element: num characters included from u.  second element for v.
pair<long, long> fuse_substrings(const CST &T, const CST &T_R, const string &s, Slice u, Slice v, ChildIndex &child_index) {
    pair<long, long> ans;
    if (u.size() > v.size()) {
        ans = {u.size(), 0};
    } else {
        ans = {0, v.size()};
    }
    for (long u_start = u.start; u_start <= u.end; u_start++) {
        Slice u_part{u_start, u.end};

        if (u_part.size() + v.size() <= ans.first + ans.second) {
            continue;
        }

        string test_str = u_part.apply(s) + v.apply(s);
        long l = longest_consume(T, test_str, child_index);
        if (l > ans.first + ans.second) {
            ans = {u_part.size(), l-u_part.size()};
        }

    }

    return ans;
}

using leaf_index = sdsl::int_vector<sizeof(sdsl::int_vector_size_type)*8>;

//build an index that lets us quickly find the leaf of the given depth
//returns an array where A[i] = the id of the leaf of depth i+1
leaf_index build_leaf_index(const CST &T, long size) {
    leaf_index index(size+1);
    for (Node node : T) {
        if (T.is_leaf(node)) {
            index[T.depth(node)-1] = node;
        }
    }
    return index;
}

void test_leaf_index() {
    string s = "abacadabra";
    CST T;
    construct_im(T, s, 1);
    leaf_index li = build_leaf_index(T, s.size());
    for (int i = 0; i <= s.size(); i++) {
        cout << T.csa.isa[i] << ' ';
    }
    cout << '\n';
    for (int i = 1; i <= s.size(); i++) {
        Node ref = li[i-1];
        Node test = find_leaf_of_depth(T, i);
        cout << T.depth(ref) << ' ' << T.depth(test) << '\n';
    }
}

//return the node resulting from finding the lowest depth >= min_depth in the path from the root to the leaf
//if this would put us in the middle of a path, return the node at the end of the path.
Node go_up(const CST &T, Node leaf, long min_depth) {
    Node node = leaf;
    while (node != T.root() && T.depth(T.parent(node)) >= min_depth) {
        node = T.parent(node);
    }
    return node;
}

pair<long, long> fuse_substrings_HIA(const HIAInducedIndex &index, const CST &T, const CST &T_R, long t_len, Slice u, Slice v) {
    //figure out the start position of u in the reversed string, 0 indexed
    long u_start_rev = t_len - u.end - 1;

    //then the depth of the leaf node in the reversed suffix tree
    long u_rev_leaf_depth = t_len - u_start_rev + 1;
    //t_len - t_len + u.end + 1 + 1;

    Node u_rev_leaf = find_leaf_of_depth(T_R, u_rev_leaf_depth);
    //debug_print_node(T_R,u_rev_leaf);

    //debug_print_node(T_R, u_rev_leaf);
    Node u_rev_node = go_up(T_R, u_rev_leaf, u.size());
    //debug_print_node(T_R, u_rev_node);


    long v_leaf_depth = t_len - v.start + 1;
    Node v_leaf = find_leaf_of_depth(T, v_leaf_depth);
    //debug_print_node(T,v_leaf);
    //debug_print_node(T, v_leaf);
    Node v_node = go_up(T, v_leaf, v.size());
    //debug_print_node(T, v_node);

    HIAResult result = dummy_HIA<reverse_cst_label_mapper, cst_label_mapper>(
        index, T_R, T, u_rev_node, v_node, u.size(), v.size(), t_len
    );

    //cout << "sum" << result.sum << '\n';

    Node u_new_node = result.HIA_1;
    /*long u_included = T_R.depth(u_rev_node) - T_R.depth(u_new_node);
    if (u_new_node != T_R.root() && T_R.depth(T_R.parent(u_new_node))) {
        long u_not_included = u.size() - u_included;
        long path_length = T_R.depth(u_new_node) - T_R.depth(T_R.parent(u_new_node));
        if (u_not_included < path_length) {
            u_included += u_not_included;
        }
    }*/
    long u_included = min(T_R.depth(u_new_node), (uint64_t)u.size());

    Node v_new_node = result.HIA_2;
    /*long v_included = T.depth(v_node) - T.depth(v_new_node);
    if (v_new_node != T.root() && T.depth(T.parent(v_new_node))) {
        long v_not_included = v.size() - v_included;
        long path_length = T.depth(v_new_node) - T.depth(T.parent(v_new_node));
        if (v_not_included < path_length) {
            v_included += v_not_included;
        }
    }*/
   long v_included = min(T.depth(v_new_node), (uint64_t)v.size());


    return {u_included, v_included};
}


void test_fuse_substrings() {
    CST cst;

    ////////////0123456789
    string s = "abacadabra";
    construct_im(cst, s, 1);
    ChildIndex T_ci(cst);

    CST cst_r;
    string s_r = s;
    reverse(s_r.begin(), s_r.end());
    construct_im(cst_r, s_r, 1);
    HIAInducedIndex index = get_induced_index(cst_r, cst, s.size());
    int testnum = 0;
    auto test = [&](Slice u, Slice v) {
        testnum++;
        auto ans = fuse_substrings(cst, cst_r, s, u, v, T_ci);
        cout << "Test " << testnum << ": " << u.apply(s) << " + " << v.apply(s) << " = " << ans << '\n';
        ans = fuse_substrings_HIA(index, cst, cst_r, s.size(), u, v);
        cout << "Test " << testnum << ": " << u.apply(s) << " + " << v.apply(s) << " = " << ans << '\n';
    };

    test({6, 8}, {2, 4});
    test({1, 2}, {3, 6});
    test({3, 3}, {5, 5});
    test({3, 5}, {0, 2});
    test({3, 4}, {7, 8});
    /*cout << "Test 1: " << fuse_substrings(cst, cst_r, "abr", "aca") << endl;
    cout << "Test 2: " << fuse_substrings(cst, cst_r, "ba", "cada") << endl;
    cout << "Test 3: " << fuse_substrings(cst, cst_r, "c", "d") << endl;
    cout << "Test 4: " << fuse_substrings(cst, cst_r, "cad", "aba") << endl;
    cout << "Test 5: " << fuse_substrings(cst, cst_r, "ca", "br") << endl;*/
}

void test_fuse_substrings_auto() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        int length = randint(7, 20);
        string s = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
        }
        CST cst;
        construct_im(cst, s, 1);
        ChildIndex T_ci(cst);

        CST cst_r;
        string s_r = s;
        reverse(s_r.begin(), s_r.end());
        construct_im(cst_r, s_r, 1);
        HIAInducedIndex rt = get_induced_index(cst_r, cst, s.size());


        for (int j = 0; j < RUNS_PER_STRING; j++) {
            Slice u = randslice(length);
            Slice v = randslice(length);

            auto ans_base = fuse_substrings(cst, cst_r, s, u, v, T_ci);
            auto ans_hia = fuse_substrings_HIA(rt, cst, cst_r, s.size(), u, v);

            int base_len = ans_base.first + ans_base.second;
            int hia_len  = ans_hia.first + ans_hia.second;

            if (base_len != hia_len) {
                cout << "FAIL\n";
                cout << "Test " << (i*RUNS_PER_STRING + j) << ": \n";
                cout << s << '\n';
                cout << u.apply(s) << " + " << v.apply(s) << " = " << ans_base << "or" << ans_hia << '\n';
                return;
            }
        }
    }

    cout << "PASS\n";
}

void test_range_search_2d() {
    int size = 100;
    sdsl::int_vector<> array(size, 0);
    for (int i = 0; i < size; i++) {
        array[i] = randint(0, size-1);
    }
    wt_int wavelet;
    construct_im(wavelet, array);

    for (int q = 0; q < 1000; q++) {
        Slice indicies = randslice(size-1);
        int i_l = indicies.start;
        int i_r = indicies.end;

        Slice values = randslice(size-1);
        int v_l = values.start;
        int v_r = values.end;

        int ans_ref = 0;
        for (int i = i_l; i <= i_r; i++) {
            if (array[i] >= v_l && array[i] <= v_r) {
                ans_ref++;
            }
        }

        int ans_test = wavelet.range_search_2d(i_l, i_r, v_l, v_r, false).first;

        if (ans_test != ans_ref) {
            cout << "FAIL\n";
            cout << "Expected " << ans_ref << " Got " << ans_test << "\n\n";
        }
    }

    cout << "PASS\n";
}

#endif
