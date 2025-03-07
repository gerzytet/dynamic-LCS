#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <string_view>
#include <cmath>
#include <algorithm>
#include <random>
#include <list>

using namespace sdsl;

using std::list;
using std::stoull;
using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;
using std::unordered_set;
using std::pair;
using std::string_view;
using std::min;
using std::max;
using std::reverse;

using Node = sdsl::bp_interval<sdsl::int_vector_size_type>;
using CST = cst_sct3<>;

//both ends inclusive!
class Slice {
    public:
    long start, end;

    inline long size() const {
        return end - start + 1;
    }

    string apply(string s) const {
        return s.substr(start, size());
    }
};

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

typedef long(*label_mapper)(const CST &T, Node node, long length);

template<label_mapper mapper>
void flip_subtree_excluding_child(const CST &T, TwoSet &ts, Node root, Node child, int sign, long length) {
    long left = T.lb(root);
    long right = T.rb(root);

    long c_left = T.lb(child);
    long c_right = T.rb(child);

    for (unsigned long i = left; i < c_left; i++) {
        Node n = {i, i};
        ts.update(mapper(T, n, length), sign);
    }

    for (unsigned long i = c_right+1; i <= right; i++) {
        Node n = {i, i};
        ts.update(mapper(T, n, length), sign);
    }
}

template<label_mapper T1_mapper, label_mapper T2_mapper>
HIAResult dummy_HIA(CST &T1, CST &T2, Node u, Node v, long u_length, long v_length, long length) {
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

    TwoSet ts;
    //plan: use 2 pointers approach
    //v moves down, u moves up
    //find induced node boundaries, then return HIA
    long twos = 0;
    //v_temp is the root at this point
    for (unsigned long i = T2.lb(v_temp); i <= T2.rb(v_temp); i++) {
        ts.add(T2_mapper(T2, {i, i}, length));
    }

    Node u_temp = u;
    for (unsigned long i = T1.lb(u_temp); i <= T1.rb(u_temp); i++) {
        ts.add(T1_mapper(T1, {i, i}, length));
    }

    Node HIA_1 = u;
    Node HIA_2 = v_temp;
    long HIA_sum = min((uint64_t)u_length, T1.depth(u)) + min((uint64_t)v_length, T2.depth(v_temp));

    while (true) {
        if (ts.twos > 0) {
            long sum = min((uint64_t)u_length, T1.depth(u_temp)) + min((uint64_t)v_length, T2.depth(v_temp));
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
            flip_subtree_excluding_child<T2_mapper>(T2, ts, v_temp, v_child, -1, length);
            v_temp = v_child;
        } else {
            if (u_temp == T1.root()) {
                break;
            }
            Node u_parent = T1.parent(u_temp);
            flip_subtree_excluding_child<T1_mapper>(T1, ts, u_parent, u_temp, 1, length);
            u_temp = u_parent;
        }
    }

    return HIAResult(HIA_1, HIA_2, HIA_sum);
}

template<label_mapper T1_mapper, label_mapper T2_mapper>
bool is_induced(CST &T1, CST &T2, Node u, Node v, long length) {
    TwoSet ts;
    long left = T1.lb(u);
    long right = T1.rb(u);

    for (long i = left; i <= right; i++) {
        Node n = {i, i};
        ts.add(T1_mapper(T1, n, length));
    }

    left = T2.lb(v);
    right = T2.rb(v);

    for (long i = left; i <= right; i++) {
        Node n = {i, i};
        ts.add(T2_mapper(T2, n, length));
    }

    return ts.twos > 0;
}

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

//first element: num characters included from u.  second element for v.
pair<long, long> fuse_substrings(const CST &T, const CST &T_R, const string &s, Slice u, Slice v) {
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
        long l = longest_consume(T, test_str);
        if (l > ans.first + ans.second) {
            ans = {u_part.size(), l-u_part.size()};
        }

    }

    return ans;
}

long cst_label_mapper(const CST &T, Node node, long size) {
    return size - T.depth(node) + 2;
}

long reverse_cst_label_mapper(const CST &T, Node node, long size) {
    return T.depth(node);
}

using leaf_index = int_vector<sizeof(int_vector_size_type)*8>;

//build an index that lets us quickly find the leaf of the given depth
//returns an array where A[i] = the if of the leaf of depth i+1
leaf_index build_leaf_index(const CST &T, long size) {
    leaf_index index(size+1);
    for (Node node : T) {
        if (T.is_leaf(node)) {
            index[T.depth(node)-1] = T.id(node);
        }
    }
    return index;
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

void debug_print_node(const CST &T, Node node) {
    for (int i = 0; i < T.depth(node); i++) {
        char c = T.edge(node, i+1);
        cout << (c ? c : '$');
    }
    cout << '\n';
}

pair<long, long> fuse_substrings_HIA(CST &T, CST &T_R, const leaf_index &li, const leaf_index &li_r, const string &s, Slice u, Slice v) {
    //figure out the start position of u in the reversed string, 0 indexed
    long u_start_rev = s.size() - u.end - 1;

    //then the depth of the leaf node in the reversed suffix tree
    long u_rev_leaf_depth = s.size() - u_start_rev + 1;

    Node u_rev_leaf = T_R.inv_id(li_r[u_rev_leaf_depth-1]);
    //debug_print_node(T_R, u_rev_leaf);
    Node u_rev_node = go_up(T_R, u_rev_leaf, u.size());
    //debug_print_node(T_R, u_rev_node);


    long v_leaf_depth = s.size() - v.start + 1;
    Node v_leaf = T.inv_id(li[v_leaf_depth-1]);
    //debug_print_node(T, v_leaf);
    Node v_node = go_up(T, v_leaf, v.size());
    //debug_print_node(T, v_node);

    HIAResult result = dummy_HIA<reverse_cst_label_mapper, cst_label_mapper>(
        T_R, T, u_rev_node, v_node, u.size(), v.size(), s.size()
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
    leaf_index li = build_leaf_index(cst, s.size());

    CST cst_r;
    string s_r = s;
    reverse(s_r.begin(), s_r.end());
    construct_im(cst_r, s_r, 1);
    leaf_index li_r = build_leaf_index(cst_r, s.size());
    int testnum = 0;
    auto test = [&](Slice u, Slice v) {
        testnum++;
        auto ans = fuse_substrings(cst, cst_r, s, u, v);
        cout << "Test " << testnum << ": " << u.apply(s) << " + " << v.apply(s) << " = " << ans << '\n';
        ans = fuse_substrings_HIA(cst, cst_r, li, li_r, s, u, v);
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

std::minstd_rand0 random_gen;

//both sides inclusive
int randint(int a, int b) {
    std::uniform_int_distribution<int> distribution(a, b);
    return distribution(random_gen);
}

Slice randslice(int length) {
    int a = randint(0, length-1);
    int b = randint(0, length-1);
    return {min(a, b), max(a, b)};
}

void test_fuse_substrings_auto() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        int length = randint(7, 150);
        string s = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
        }
        CST cst;
        construct_im(cst, s, 1);
        leaf_index li = build_leaf_index(cst, s.size());

        CST cst_r;
        string s_r = s;
        reverse(s_r.begin(), s_r.end());
        construct_im(cst_r, s_r, 1);
        leaf_index li_r = build_leaf_index(cst_r, s.size());


        for (int j = 0; j < RUNS_PER_STRING; j++) {
            Slice u = randslice(length);
            Slice v = randslice(length);

            auto ans_base = fuse_substrings(cst, cst_r, s, u, v);
            auto ans_hia = fuse_substrings_HIA(cst, cst_r, li, li_r, s, u, v);

            int base_len = ans_base.first + ans_base.second;
            int hia_len  = ans_hia.first + ans_hia.second;

            if (base_len != hia_len) {
                cout << "FAIL\n";
                cout << "Test " << (i*RUNS_PER_STRING + j) << ": " << u.apply(s) << " + " << v.apply(s) << " = " << ans_base << "or" << ans_hia << '\n';
                break;
            }
        }
    }

    cout << "PASS\n";
}

long fuse_prefix_dummy(CST &T, Slice u, Slice v, const string &s) {
    return longest_consume(T, u.apply(s) + v.apply(s));
}

class MaxBlockDecomposition {
    string s;
    list<long> start_positions;
    CST T;

    public:
    //T is the CST of the reference string
    //s is the string being decomposed.
    MaxBlockDecomposition(CST &T, string s) {
        long index = 0;
        this->T = T;
        this->s = s;
        while (index < s.size()) {
            string suffix = s.substr(index);
            long block_len = longest_consume(T, suffix);
            if (block_len == 0) {
                block_len = 1;
            }
            start_positions.push_back(index);
            index += block_len;
        }
    }

    vector<Slice> get_slices() {
        vector<Slice> ans;
        long prev = -1;
        for (long start : start_positions) {
            if (prev != -1) {
                ans.push_back({prev, start-1});
            }
            prev = start;
        }
        if (prev != -1) {
            ans.push_back({prev, (long)s.size()-1});
        }

        return ans;
    }

    void print() {
        bool first = true;
        for (Slice slice : get_slices()) {
            if (first) {
                first = false;
            } else {
                cout << '|';
            }

            cout << slice.apply(s);
        }
        cout << '\n';
    }

    bool validate() {
        auto slices = get_slices();
        for (Slice slice : slices) {
            string block = slice.apply(s);
            long len = longest_consume(T, block);
            bool size_match = len == block.size();
            bool single_char_exception = block.size() == 1 && len == 0;
            if (!(size_match || single_char_exception)) {
                return false;
            }
        }

        for (long i = 0; i < slices.size()-1; i++) {
            string block = slices[i].apply(s);
            string next = slices[i+1].apply(s);
            if (longest_consume(T, block+next) == (block.size() + next.size())) {
                return false;
            }
        }

        return true;
    }

    void test_remove_first() {
        start_positions.erase(start_positions.begin());
    }

    void test_remove_second() {
        auto iter = start_positions.begin();
        iter++;
        start_positions.erase(iter);
    }

    /*Slice find_slice(long pos) {
        long start = 0;
        for (long end : end_positions) {
            start = end+1;
            if (pos >= start && pos <= end) {
                return {start, end};
            }
        }

        return {-1, -1};
    }*/

    long get_end(list<long>::iterator iter) {
        auto next = iter;
        next++;
        if (next == start_positions.end()) {
            return s.size()-1;
        }
        return *next - 1;
    }

    void replace(long pos, char c) {
        s[pos] = c;
        //get an iterator to the start pos of the block this pos belongs to
        //get the start pos of the previous block
        //get the end pos of the next block
        //this start-end are the upper and lower bounds of what indicies need to be merged
        //starting with the prev block, redraw the boundaries until reaching a block with an end position past the bounds
        auto iter = start_positions.begin();
        while (iter != start_positions.end() && *iter < pos) {
            iter++;
        }
        if (iter == start_positions.end() || *iter > pos) {
            iter--;
        }

        long start;
        if (iter == start_positions.begin()) {
            start = *iter;
        } else {
            auto prev = iter;
            prev--;
            start = *prev;
        }

        long end;
        auto next = iter;
        next++;
        if (next == start_positions.end()) {
            end = s.size()-1;
        } else {
            end = get_end(next);
        }


        //isolate the changed character in its own block
        //up to 2 entries need to be added:
        //if the character is not the first in a block, an entry for the position of the character
        //if the character is not the last in a block, an entry for the position after the character
        auto iter_temp = iter;
        if (pos != *iter_temp) {
            auto next = iter_temp;
            next++;
            iter_temp = start_positions.insert(next, pos);
        }
        auto next_temp = iter_temp;
        next_temp++;
        if (pos != s.size()-1 && (next_temp == start_positions.end() || pos != *next_temp - 1)) {
            auto next = iter_temp;
            next++;
            start_positions.insert(next, pos+1);
        }

        if (iter != start_positions.begin()) {
            iter--;
        }
        while (true) {
            long curr_start = *iter;
            long curr_end = get_end(iter);
            if (curr_start > end) {
                return;
            }
            if (curr_end == s.size()-1) {
                Slice slice{curr_start, curr_end};
                long len = longest_consume(T, slice.apply(s));
                //handle single character exception
                if (len == 0) {
                    len = 1;
                }
                if (len == slice.size()) {
                    break;
                } else {
                    //len < slice.size()
                    auto next = iter;
                    next++;
                    iter = start_positions.insert(next, curr_start+len);
                }
            } else {
                auto next = iter;
                next++;
                long next_start = *next;
                long next_end = get_end(next);

                Slice curr_slice = {curr_start, curr_end};
                Slice next_slice = {next_start, next_end};
                long len = fuse_prefix_dummy(T, curr_slice, next_slice, s);
                if (len == 0) {
                    len = 1;
                }
                if (len < curr_slice.size()) {
                    iter = start_positions.insert(next, curr_start+len);
                }
                else if (len == curr_slice.size()) {
                    iter = next;
                } else if (len < (curr_slice.size() + next_slice.size())) {
                    iter = next;
                } else {
                    //len == (curr_slice.size() + next_slice.size())
                    start_positions.erase(next);
                }
            }
        }
    }
};

void test_replace() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        int length = randint(4, 100);
        string s = "";
        string t = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
            t += (char) randint('a', 'c');
        }
        CST T;
        construct_im(T, t, 1);
        MaxBlockDecomposition decomp(T, s);

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            /*if (j == 48) {
                cout << "48\n";
                cout << "t: " << t << '\n';
                decomp.print();
            }*/
            long pos = randint(0, length-1);
            char c = randint('a', 'h');
            decomp.replace(pos, c);
            bool result = decomp.validate();
            if (!result) {
                cout << "FAIL i: " << i << ", j: " << j << '\n';
                decomp.print();
                return;
            }
        }
    }

    cout << "PASS\n";
}

int main() {
    //test_fuse_substrings_auto();


    /*string s = "abacadabra";
    string t = "abcbabcadb";
    CST T;
    construct_im(T, t, 1);
    MaxBlockDecomposition mbd(T, s);
    mbd.print();
    cout << mbd.validate() << '\n';
    mbd.test_remove_second();
    cout << mbd.validate() << '\n';*/

    test_replace();


}
