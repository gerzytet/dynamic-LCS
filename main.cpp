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

#include "heap.cpp"
#include "G4G_LCS.cpp"

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

using Node = sdsl::int_vector_size_type;
using CST = cst_sada<csa_wt<>,
    lcp_support_sada<>,
    bp_support_sada<>,
    rank_support_v<10,2>,
    select_support_mcl<10,2>>;


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

    bool is_inbounds(int max) const {
        return start <= end && end <= max;
    }

    bool is_invalid() const {
        return start == -1 && end == -1;
    }

    bool operator>(const Slice& other) const {
        if (is_invalid()) return false;
        if (other.is_invalid()) return true;
        return (size() > other.size()) || (size() == other.size() && start > other.start);
    }

    bool operator==(const Slice& other) const {
        return start == other.start && end == other.end;
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

std::minstd_rand0 random_gen(0);

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

string randstring(int length, char min, char max) {
    string ans = "";
    for (int i = 0; i < length; i++) {
        ans += randint(min, max);
    }
    return ans;
}

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
    Node lb = T2.lb(v_temp);
    Node rb = T2.rb(v_temp);

    //cout << lb << ' ' << rb << std::endl;
    for (unsigned long i = lb; i <= rb; i++) {
        auto to_add = T2_mapper(T2, select_leaf(T2, i+1), length);
        ts.add(to_add);
    }

    Node u_temp = u;
    for (unsigned long i = T1.lb(u_temp); i <= T1.rb(u_temp); i++) {
        ts.add(T1_mapper(T1, select_leaf(T1, i+1), length));
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

pair<long, long> fuse_substrings_HIA(CST &T, CST &T_R, const leaf_index &li, const leaf_index &li_r, long t_len, Slice u, Slice v) {
    //figure out the start position of u in the reversed string, 0 indexed
    long u_start_rev = t_len - u.end - 1;

    //then the depth of the leaf node in the reversed suffix tree
    long u_rev_leaf_depth = t_len - u_start_rev + 1;

    Node u_rev_leaf = T_R.inv_id(li_r[u_rev_leaf_depth-1]);
    //debug_print_node(T_R,u_rev_leaf);

    //debug_print_node(T_R, u_rev_leaf);
    Node u_rev_node = go_up(T_R, u_rev_leaf, u.size());
    //debug_print_node(T_R, u_rev_node);


    long v_leaf_depth = t_len - v.start + 1;
    Node v_leaf = T.inv_id(li[v_leaf_depth-1]);
    //debug_print_node(T,v_leaf);
    //debug_print_node(T, v_leaf);
    Node v_node = go_up(T, v_leaf, v.size());
    //debug_print_node(T, v_node);

    HIAResult result = dummy_HIA<reverse_cst_label_mapper, cst_label_mapper>(
        T_R, T, u_rev_node, v_node, u.size(), v.size(), t_len
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
        ans = fuse_substrings_HIA(cst, cst_r, li, li_r, s.size(), u, v);
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
            auto ans_hia = fuse_substrings_HIA(cst, cst_r, li, li_r, s.size(), u, v);

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

Slice fuse_prefix_dummy_slice(CST &T, Slice u, Slice v, const string &s, long t_len) {
    return longest_consume_slice(T, u.apply(s) + v.apply(s), t_len);
}

struct Block {
    long start;
    Slice candidate;
    long t_start;

    Block(long start) : start(start) {
        candidate = {-1, -1};
        t_start = -1;
    }

    Block(long start, Slice candidate, long t_start) : start(start), candidate(candidate), t_start(t_start) {}
};

//for every substring, there is a corresponding substring in the reverse of the string
//for a substring containing the characters with indicies 0 and 1, the corresponding one will have the last 2 indicies
Slice reverse_slice(Slice slice, long string_length) {
    return {string_length-slice.end-1, string_length-slice.start-1};
}

using block_iter = list<Block>::iterator;

class MaxBlockDecomposition {
    string s;
    string t;
    string t_r;
    long t_size;
    list<Block> blocks;
    CST T;
    CST T_R;
    leaf_index T_li;
    leaf_index T_R_li;
    Heap<Slice> candidate_heap;

    public:
    //T is the CST of the reference string
    //s is the string being decomposed.
    MaxBlockDecomposition(string s, string t) {
        long index = 0;
        t_size = t.size();
        this->s = s;
        this->t = t;
        construct_im(T, t, 1);
        reverse(t.begin(), t.end());
        this->t_r = t;
        construct_im(T_R, t, 1);
        reverse(t.begin(), t.end());
        T_li   = build_leaf_index(T,   t.size());
        T_R_li = build_leaf_index(T_R, t.size());
        while (index < s.size()) {
            string suffix = s.substr(index);
            Slice slice = longest_consume_slice(T, suffix, t_size);
            long block_len = slice.size();
            if (block_len == 0) {
                block_len = 1;
            }
            Block block(index);
            block.t_start = slice.start;
            blocks.push_back(block);
            index += block_len;
        }

        for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
            recalculate(iter);
        }
    }

    string get_block_string(block_iter block) {
        return Slice{block->start, get_end(block)}.apply(s);
    }

    long get_block_size(block_iter block) {
        return get_end(block) - block->start + 1;
    }

    Slice get_block_t_slice(block_iter block) {
        if (block->t_start == -1) {
            return {-1, -1};
        }
        return {block->t_start, block->t_start + get_block_size(block) - 1};
    }

    void recalculate(block_iter block) {
        auto iter = block;
        candidate_heap.remove(block->candidate);
        Slice u;
        if (iter == blocks.begin()) {
            u = get_block_t_slice(iter);
        } else {
            Slice u1 = get_block_t_slice(iter);
            iter--;
            Slice u2 = get_block_t_slice(iter);
            Slice u1_rev = reverse_slice(u1, t_size);
            Slice u2_rev = reverse_slice(u2, t_size);

            if (u1.start == -1 || u2.start == -1) {
                u = u1;
            } else {
                Slice ans = fuse_prefix_dummy_slice(T_R, u1_rev, u2_rev, t_r, t_size);
                u = reverse_slice(ans, t_size);
            }
        }

        Slice v;
        iter = block;
        iter++;
        if (iter == blocks.end()) {
            v = {-1, -1};
        } else {
            auto iter_next = iter;
            iter_next++;
            if (iter_next == blocks.end()) {
                v = get_block_t_slice(iter);
            } else {
                Slice v1 = get_block_t_slice(iter);
                Slice v2 = get_block_t_slice(iter_next);

                if (v1.start == -1 || v2.start == -1) {
                    v = v1;
                } else {
                    v = fuse_prefix_dummy_slice(T, v1, v2, t, t_size);
                }
            }
        }


        pair<long, long> result;
        if (u.start == -1 && v.start == -1) {
            result = {0, 0};
        } else if (u.start == -1) {
            result = {0, v.size()};
        } else if (v.start == -1) {
            result = {u.size(), 0};
        } else {
            result = fuse_substrings_HIA(T, T_R, T_li, T_R_li, t_size, u, v);
        }
        long end = get_end(block);
        block->candidate = Slice{end - result.first + 1, end + result.second};
        candidate_heap.add(block->candidate);
    }

    vector<Slice> get_slices() {
        vector<Slice> ans;
        long prev = -1;
        for (Block block : blocks) {
            if (prev != -1) {
                ans.push_back({prev, block.start-1});
            }
            prev = block.start;
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

    string get_candidate_string(block_iter iter) {
        return iter->candidate.apply(s);
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

        for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
            string candidate = get_candidate_string(iter);
            if (longest_consume(T, candidate) != candidate.size()) {
                return false;
            }
        }

        return true;
    }

    void test_remove_first() {
        blocks.erase(blocks.begin());
    }

    void test_remove_second() {
        auto iter = blocks.begin();
        iter++;
        blocks.erase(iter);
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

    long get_end(block_iter iter) {
        auto next = iter;
        next++;
        if (next == blocks.end()) {
            return s.size()-1;
        }
        return next->start - 1;
    }

    void replace(long pos, char c) {
        if (s[pos] == c) {
            return;
        }
        s[pos] = c;
        //get an iterator to the start pos of the block this pos belongs to
        //get the start pos of the previous block
        //get the end pos of the next block
        //this start-end are the upper and lower bounds of what indicies need to be merged
        //starting with the prev block, redraw the boundaries until reaching a block with an end position past the bounds
        auto iter = blocks.begin();
        while (iter != blocks.end() && iter->start < pos) {
            iter++;
        }
        if (iter == blocks.end() || iter->start > pos) {
            iter--;
        }
        block_iter original_block = iter;

        long start;
        if (iter == blocks.begin()) {
            start = iter->start;
        } else {
            auto prev = iter;
            prev--;
            start = prev->start;
        }

        long end;
        auto next = iter;
        next++;
        if (next == blocks.end()) {
            end = s.size()-1;
        } else {
            end = get_end(next);
        }

        block_iter min_changed = blocks.end();
        block_iter max_changed = blocks.end();
        auto insert_and_recalc = [&](block_iter iter, Block block) {
            auto new_iter = blocks.insert(iter, block);
            max_changed = new_iter;

            auto prev = new_iter;
            if (prev != blocks.begin()) {
                prev--;
            }
            if (min_changed == blocks.end()) {
                min_changed = prev;
            }
            new_iter->t_start = prev->t_start + (get_end(prev) - prev->start + 1);
            return new_iter;
        };

        //isolate the changed character in its own block
        //up to 2 entries need to be added:
        //if the character is not the first in a block, an entry for the position of the character
        //if the character is not the last in a block, an entry for the position after the character
        auto iter_temp = iter;
        if (pos != iter_temp->start) {
            auto next = iter_temp;
            next++;
            iter_temp = insert_and_recalc(next, Block(pos));
        }
        auto next_temp = iter_temp;
        next_temp++;
        if (pos != s.size()-1 && (next_temp == blocks.end() || pos != next_temp->start - 1)) {
            auto next = iter_temp;
            next++;
            insert_and_recalc(next, Block(pos+1));
        }
        string test_string(1, s[iter_temp->start]);
        iter_temp->t_start = longest_consume_slice(T, test_string, t_size).start;

        if (iter != blocks.begin()) {
            iter--;
        }
        while (true) {
            long curr_start = iter->start;
            long curr_end = get_end(iter);
            if (curr_start > end) {
                break;
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
                    iter = insert_and_recalc(next, Block(curr_start+len));
                }
            } else {
                auto next = iter;
                next++;
                long next_start = next->start;
                long next_end = get_end(next);

                Slice curr_slice = {curr_start, curr_end};
                Slice next_slice = {next_start, next_end};
                Slice slice = fuse_prefix_dummy_slice(T, curr_slice, next_slice, s, t_size);
                long len = slice.size();
                if (len == 0) {
                    len = 1;
                }
                if (len < curr_slice.size()) {
                    iter = insert_and_recalc(next, curr_start+len);
                }
                else if (len == curr_slice.size()) {
                    iter = next;
                } else if (len < (curr_slice.size() + next_slice.size())) {
                    iter = next;
                } else {
                    //len == (curr_slice.size() + next_slice.size())
                    if (min_changed == blocks.end()) {
                        min_changed = iter;
                    }
                    if (min_changed == next) {
                        min_changed = iter;
                    }
                    candidate_heap.remove(next->candidate);
                    blocks.erase(next);
                    max_changed = iter;
                    iter->t_start = slice.start;
                }
            }
        }

        //walk the min iterator back twice, and the max iterator forward once
        //because the recalculation uses the next 2 blocks but only the previous 1
        //then recalculate everything between the iterators
        if (min_changed == blocks.end()) {
            min_changed = original_block;
            max_changed = original_block;
        }

        //make sure that at least the original position is covered
        while (max_changed != blocks.end() && max_changed->start <= pos) {
            max_changed++;
        }
        if (max_changed == blocks.end()) {
            max_changed--;
        }
        for (int i = 0; i < 2; i++) {
            if (min_changed == blocks.begin()) {
                break;
            }
            min_changed--;
        }

        //we walk the forward iterator forward twice too actually
        //because the loop uses it as an exclusive bound, not inclusive
        for (int i = 0; i < 2; i++) {
            max_changed++;
            if (max_changed == blocks.end()) {
                break;
            }
        }

        for (block_iter iter = min_changed; iter != max_changed; iter++) {
            recalculate(iter);
        }
    }

    void print_candidates() {
        int index = 0;
        for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
            cout << index++ << ": ";
            cout << get_candidate_string(iter);
            cout << '\n';
        }
    }

    Slice get_lcs() {
        return candidate_heap.get_highest();
    }
};

void test_replace() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        int length = randint(4, 10);
        string s = "";
        string t = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
            t += (char) randint('a', 'c');
        }
        MaxBlockDecomposition decomp(s, t);

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            if (i == 14 && j == 239) {
                cout << "MERP";
            }
            long pos = randint(0, length-1);
            char c = randint('a', 'h');
            cout << t << '\n';
            decomp.print();
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

void test_initial_blocks() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        if (i == 14) {
            cout << i;
        }
        int length = randint(4, 10);
        string s = "";
        string t = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
            t += (char) randint('a', 'c');
        }
        MaxBlockDecomposition decomp(s, t);

        if (!decomp.validate()) {
            cout << t << '\n';
            cout << "FAIL " << i << '\n';
            decomp.print();
            decomp.print_candidates();
            return;
        }
    }

    cout << "PASS\n";
}

void test_initial_blocks_different_lengths() {
    int RUNS_PER_STRING = 1000;
    for (int i = 0; i < 100; i++) {
        if (i == 6) {
            cout << "MERP";
        }
        int length_s = randint(4, 10);
        int length_t = randint(4, 10);
        string s = "";
        string t = "";
        for (int j = 0; j < length_s; j++) {
            s += (char) randint('a', 'c');
        }
        for (int j = 0; j < length_t; j++) {
            t += (char) randint('a', 'c');
        }
        MaxBlockDecomposition decomp(s, t);

        if (!decomp.validate()) {
            cout << t << '\n';
            cout << "FAIL " << i << '\n';
            decomp.print();
            decomp.print_candidates();
            return;
        }
    }

    cout << "PASS\n";
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

//returns slice of s or {-1, -1} if no characters are in common
Slice LCS_slow(string s, string t) {
    CST T;
    construct_im(T, t, 1);

    Slice best{-1, -1};
    for (long start = 0; start < s.size(); start++) {
        string substr = s.substr(start);

        long match = longest_consume(T, substr);
        if (best.start == -1 || best.size() < match) {
            best = {start, start + match - 1};
        }
    }

    return best;
}

void test_LCS() {
    int RUNS_PER_STRING = 100;
    for (int i = 0; i < 10; i++) {
        int length = randint(1000, 2000);
        string s = "";
        string t = "";
        for (int j = 0; j < length; j++) {
            s += (char) randint('a', 'c');
            t += (char) randint('a', 'c');
        }
        MaxBlockDecomposition decomp(s, t);

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            long pos = randint(0, length-1);
            char c = randint('a', 'h');
            s[pos] = c;
            //cout << t << '\n';
            //decomp.print();
            //decomp.print_candidates();
            decomp.replace(pos, c);
            bool result = decomp.validate();
            if (!result) {
                cout << "VALIDATE FAIL i: " << i << ", j: " << j << '\n';
                decomp.print();
                return;
            }
            string LCS_reference = G4G::LCS(s, t);
            Slice LCS = decomp.get_lcs();
            if (LCS.size() != LCS_reference.size()) {
                cout << "FAIL: reference is " << LCS_reference.size() << " calculated is " << LCS.size() << '\n';
                decomp.print();
                decomp.print_candidates();
                return;
            }
        }
        cout << "PASS " << i << '\n';
    }

    cout << "PASS ALL\n";
}

void test_LCS_different_lengths() {
    int RUNS_PER_STRING = 100;
    for (int i = 0; i < 10; i++) {
        int length_s = randint(1000, 2000);
        int length_t = randint(1000, 2000);
        string s = "";
        string t = "";
        for (int j = 0; j < length_s; j++) {
            s += (char) randint('a', 'c');
        }
        for (int j = 0; j < length_t; j++) {
            t += (char) randint('a', 'c');
        }
        MaxBlockDecomposition decomp(s, t);

        for (int j = 0; j < RUNS_PER_STRING; j++) {
            long pos = randint(0, length_s-1);
            char c = randint('a', 'h');
            s[pos] = c;
            //cout << t << '\n';
            //decomp.print();
            //decomp.print_candidates();
            decomp.replace(pos, c);
            bool result = decomp.validate();
            if (!result) {
                cout << "VALIDATE FAIL i: " << i << ", j: " << j << '\n';
                decomp.print();
                return;
            }
            string LCS_reference = G4G::LCS(s, t);
            Slice LCS = decomp.get_lcs();
            if (LCS.size() != LCS_reference.size()) {
                cout << "FAIL: reference is " << LCS_reference.size() << " calculated is " << LCS.size() << '\n';
                decomp.print();
                decomp.print_candidates();
                return;
            }
        }
        cout << "PASS " << i << '\n';
    }

    cout << "PASS ALL\n";
}

void test_traverse() {
    CST cst;
    std::string s = "abacad";
    construct_im(cst, s, 1);

    uint64_t max_depth = 40;

    // use the DFS iterator to traverse `cst`
    for (auto it=cst.begin(); it!=cst.end(); ++it) {
        if (it.visit() == 1) {  // node visited the first time
            auto v = *it;       // get the node by dereferencing the iterator
            if (cst.depth(v) <= max_depth) {   // if depth node is <= max_depth
                // process node, e.g. output it in format d-[lb, rb]
                string s;
                auto depth = cst.depth(v);
                for (int i = 1; i <= depth; i++) {
                    auto res = cst.edge(v, i);
                    if (res == 0) {
                        res = '$';
                    }
                    s += res;
                }

                cout<<depth<<"-["<<cst.lb(v)<< ","<<cst.rb(v)<<"]-"<<s<<endl;

            } else { // skip the subtree otherwise
                it.skip_subtree();
            }
        }
    }
}

int main() {
    test_LCS_different_lengths();


    /*string s = "abacadabra";
    string t = "abcbabcadb";
    CST T;
    construct_im(T, t, 1);
    MaxBlockDecomposition mbd(T, s);
    mbd.print();
    cout << mbd.validate() << '\n';
    mbd.test_remove_second();
    cout << mbd.validate() << '\n';*/

    //test_traverse();
    //test_fuse_substrings();

    //string s = "aaaabaa";
    //Slice ans = LCS_slow(s, "aaaba");
    //cout << ans.apply(s) << '\n';


}
