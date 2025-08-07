#include <string>
#include "global.hpp"
#include <set>

using std::string;
using std::to_string;

string before = "digraph SuffixTree { \n\
    node [shape=circle, label=\"\"];\n";
string after = "}\n";
string get_line(long ID1, long ID2, string label) {
    return string("    ") + to_string(ID1) + " -> " + to_string(ID2) + ("[label=\"" + label + "\"];\n");
}
string visualize_graphviz(CST &T) {
    Node root = T.root();

    string lines = "";
    std::set<Node> covered;
    for (Node node : T) {
        if (covered.count(node) != 0) {
            continue;
        }
        if (node != T.root()) {
            Node parent = T.parent(node);
            string label = "";
            for (long i = T.depth(parent)+1; i <= T.depth(node); i++) {
                char c = T.edge(node, i);
                label += (c ? c : '$');
            }
            lines += get_line(T.id(parent), T.id(node), label);
        }

        covered.insert(node);
    }

    return before + lines + after;
}
