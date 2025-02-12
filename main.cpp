#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace sdsl;

using std::stoull;
using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;
using std::unordered_set;

using Node = sdsl::bp_interval<sdsl::int_vector_size_type>;
using CSA = cst_sct3<>;

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

typedef long(*label_mapper)(const CSA &T, Node node);

template<label_mapper mapper>
void flip_subtree_excluding_child(const CSA &T, TwoSet &ts, Node root, Node child, int sign) {
    long left = T.lb(root);
    long right = T.rb(root);

    long c_left = T.lb(child);
    long c_right = T.rb(child);

    for (long i = left; i < c_left; i++) {
        Node n = {i, i};
        ts.update(mapper(T, n), sign);
    }

    for (long i = c_right+1; i <= right; i++) {
        Node n = {i, i};
        ts.update(mapper(T, n), sign);
    }
}

template<label_mapper T1_mapper, label_mapper T2_mapper>
HIAResult dummy_HIA(CSA &T1, CSA &T2, Node u, Node v) {
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
    for (long i = T2.lb(v_temp); i <= T2.rb(v_temp); i++) {
        ts.add(i);
    }

    Node u_temp = u;
    //u_temp is a leaf at this point
    ts.add(T1.lb(u_temp));

    Node HIA_1 = u;
    Node HIA_2 = v_temp;
    long HIA_sum = T1.depth(u) + T2.depth(v_temp);

    while (true) {
        if (ts.twos > 0) {
            long sum = T1.depth(u_temp) + T2.depth(v_temp);
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
            flip_subtree_excluding_child<T2_mapper>(T2, ts, v_temp, v_child, -1);
            v_temp = v_child;
        } else {
            if (u_temp == T1.root()) {
                break;
            }
            Node u_parent = T1.parent(u_temp);
            flip_subtree_excluding_child<T1_mapper>(T1, ts, u_parent, u_temp, 1);
            u_temp = u_parent;
        }
    }

    return HIAResult(HIA_1, HIA_2, HIA_sum);
}

template<label_mapper T1_mapper, label_mapper T2_mapper>
bool is_induced(CSA &T1, CSA &T2, Node u, Node v) {
    TwoSet ts;
    long left = T1.lb(u);
    long right = T1.rb(u);

    for (long i = left; i <= right; i++) {
        Node n = {i, i};
        ts.add(T1_mapper(T1, n));
    }

    left = T2.lb(v);
    right = T2.rb(v);

    for (long i = left; i <= right; i++) {
        Node n = {i, i};
        ts.add(T2_mapper(T2, n));
    }

    return ts.twos > 0;
}

int main() {
    cst_sct3<> cst;
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
