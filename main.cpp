#include <sdsl/suffix_arrays.hpp>
#include <sdsl/suffix_trees.hpp>
#include <string>
#include <fstream>

using namespace sdsl;

using std::stoull;
using std::cout;
using std::endl;
using std::string;

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
