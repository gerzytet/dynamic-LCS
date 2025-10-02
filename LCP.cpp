#include <iostream>
#include <utility>

#include "global.hpp"
#include "longest_consume.cpp"
#include "heap.cpp"
#include "HIA.cpp"
#include "graphviz.cpp"

using std::cout;
using std::pair;

void debug_print_node_no_newline(const CST &T, Node node) {
    T.child()
    for (int i = 0; i < T.depth(node); i++) {
        char c = T.edge(node, i+1);
        cout << (c ? c : '$');
    }
}


void print_suffixes(CST T) {
    auto root = T.root();
    for (long i = T.lb(root); i <= T.rb(root); i++) {
        auto leaf = T.select_leaf(i+1);
        cout << T.rb(leaf) << ' ';
        debug_print_node_no_newline(T, leaf);
        cout << '\n';
    }
}

void print_suffixes(CST T, Node root) {
    for (long i = T.lb(root); i <= T.rb(root); i++) {
        auto leaf = T.select_leaf(i+1);
        auto orig_number = i;
        auto new_number = T.rb(find_leaf_of_depth(T, T.depth(leaf) - T.depth(root)));
        cout << orig_number << ' ' << new_number << ' ';
        debug_print_node_no_newline(T, leaf);
        cout << '\n';
    }
}

struct rooted_LCP_t {
    Heap<Node> leaves;
    Node root;
};

//plan:
//first, display each suffix along with its order
//then, for the subtree, display the NEW suffix indicies
//next, work on the actual rooted LCP index
//rooted LCP index will contain a heap with all the new orders
//add predecessor and successor queries to the heap so that this works
rooted_LCP_t build_rooted_LCP(CST T, Node root) {
    rooted_LCP_t rlcp;
    rlcp.root = root;
    for (long i = T.lb(root); i <= T.rb(root); i++) {
        auto leaf = T.select_leaf(i+1);
        Node new_leaf = find_leaf_of_depth(T, T.depth(leaf) - T.depth(root));
        rlcp.leaves.add(new_leaf);
    }

    return rlcp;
}

//the approximate node of the answer, and the depth of the answer
//depth(pair[0]) <= pair[1]
pair<Node, long> rooted_LCP(CST &T, rooted_LCP_t &rlcp, Node pattern) {
    auto sp_opt = rlcp.leaves.predecessor(pattern+1);
    auto su_opt = rlcp.leaves.successor(pattern-1);
    bool p_exist = sp_opt.has_value();
    bool s_exist = su_opt.has_value();

    Node v;
    if (s_exist && p_exist) {
        v = T.lca(sp_opt.value(), su_opt.value());
    } else if (s_exist) {
        v = sp_opt.value();
    } else {
        v = su_opt.value();
    }

    //now we need the node for pattern in the space of T
    Node pattern_t = find_leaf_of_depth(T, T.depth(pattern) - T.depth(rlcp.root));
    uint32_t h;
    if (s_exist && p_exist) {
        h = max(T.depth(T.lca(pattern_t, su_opt.value())), T.depth(T.lca(pattern_t, sp_opt.value())));
    } else {
        h = T.lca(pattern_t, v);
    }

    //ans is node at or right below the real answer
    Node ans = go_up(T, v, h);
    return {ans, h};
}

struct unrooted_LCP_t {
    unordered_map<Node, rooted_LCP_t> rooted_LCPs;
    //an index representing the heavy path decomposition
    //index[id(n)] = the leaf node at the end of the heavy path that starts with n
    sdsl::int_vector<32> heavy_path_index;
};

void _build_unrooted_LCP(CST &T, Node node, unrooted_LCP_t &idx) {
    idx.rooted_LCPs[T.id(node)] = build_rooted_LCP(T, node);
    vector<Node> path;
    path.push_back(node);
    while (!T.is_leaf(node)) {
        vector<Node> children;
        auto c = T.children(node);
        long max_children = 0;
        Node heavy;
        for (auto it = c.begin(); it != c.end(); it++) {
            Node c_node = *it;
            children.push_back(c_node);
            long weight = T.rb(c_node) - T.lb(c_node) + 1;
            if (weight > max_children) {
                max_children = weight;
                heavy = c_node;
            }
        }

        for (Node c_node : children) {
            if (c_node != heavy) {
                _build_unrooted_LCP(T, c_node, idx);
            }
        }

        node = heavy;
        path.push_back(node);
    }

    for (Node node_p : path) {
        idx.heavy_path_index[T.id(node_p)] = node;
    }
}

unrooted_LCP_t build_unrooted_LCP(CST &T) {
    unrooted_LCP_t idx;
    idx.heavy_path_index.resize(T.nodes());
    _build_unrooted_LCP(T, T.root(), idx);
    return idx;
}

long unrooted_LCP(unrooted_LCP_t &index, CST &T, Node root, Node pattern, int root_length) {
    /*auto iter = index.rooted_LCPs.find(root);
    if (iter != index.rooted_LCPs.end()) {
        return rooted_LCP(T, (*iter).second, pattern).second;
    }*/
    Node path_leaf = index.heavy_path_index[T.id(root)];
    cout << "centroid leaf: ";
    debug_print_node(T, path_leaf);
    path_leaf = find_leaf_of_depth(T, T.depth(path_leaf) - root_length);
    cout << "centroid leaf trimmed: ";
    debug_print_node(T, path_leaf);
    Node h = T.lca(path_leaf, pattern);
    cout << "H: ";
    debug_print_node(T, h);

    return T.depth(h);
}

/*void baseline_rlcp(CST &T, Node root, Node pattern) {
    for (long i = T.lb(root); i <= T.rb(root); i++) {
        auto leaf = T.select_leaf(i+1);
    }
}*/

//returns length of final string
long fuse_slices_unrooted_LCP(unrooted_LCP_t index, CST &T, Slice s1, Slice s2) {
    Node pattern = find_leaf_of_depth(T, T.rb(T.root()) - s2.start + 1);
    rooted_LCP_t rlcp = index.rooted_LCPs[T.id(T.root())];
    Node root = go_up(T, find_leaf_of_depth(T, T.rb(T.root()) - s1.start + 1), s1.size());
    /*while (T.depth(root) > s1.size()) {
        root = T.parent(root);
    }*/
    cout << "root: ";
    debug_print_node(T, root);
    cout << "pattern: ";
    debug_print_node(T, pattern);

    auto ans = unrooted_LCP(index, T, root, pattern, s1.size());
    ans = min(ans, s2.size());

    return ans;
}


void test(Slice s1, Slice s2, string s, CST &T, unrooted_LCP_t &index) {
    cout << "str 1: " << s1.apply(s) << '\n';
    cout << "str 2: " << s2.apply(s) << '\n';
    long ans = fuse_slices_unrooted_LCP(index, T, s1, s2);
    cout << "len: " << ans << "\n\n";
}

void test_fuse_prefix_LCP() {
    CST T;

    ////////////0123456789
    string s = "abacadabra";
    construct_im(T, s, 1);

    cout << visualize_graphviz(T);

    unrooted_LCP_t index = build_unrooted_LCP(T);
    Slice testcases[] = {
        Slice{0, 1}, Slice{4, 6},
        Slice{5, 6}, Slice{0, 2},
        Slice{0, 3}, Slice{4, 6},
        Slice{0, 0}, Slice{3, 5}
    };
    for (int i = 0; i < sizeof(testcases)/(sizeof(Slice)*2); i++) {
        test(testcases[i*2], testcases[i*2+1], s, T, index);
    }
}
