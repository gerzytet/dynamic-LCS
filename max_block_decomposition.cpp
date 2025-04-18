#include <list>
#include <vector>
#include <memory>

using std::list;
using std::vector;
using std::unique_ptr;
using std::make_unique;

#include "global.hpp"
#include "slice.cpp"
#include "longest_consume.cpp"
#include "HIA.cpp"
#include "heap.cpp"

#ifndef MAX_BLOCK_DECOMPOSITION_CPP
#define MAX_BLOCK_DECOMPOSITION_CPP

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

using block_iter = list<Block>::iterator;

class MaxBlockDecomposition {
    public:
    string s;
    private:
    string t;
    string t_r;
    long t_size;
    CST T;
    CST T_R;
    leaf_index T_li;
    leaf_index T_R_li;
    Heap<Slice> candidate_heap;
    unique_ptr<HIA_RangeTree> range_tree;
    ChildIndex T_ci;
    ChildIndex T_R_ci;

    //def plus(a, b): return a + b;
    //


    public:
    list<Block> blocks;

    MaxBlockDecomposition() {

    }


    //T is the CST of the reference string
    //s is the string being decomposed.
    MaxBlockDecomposition(string s, string t, bool trace = false) {
        long index = 0;
        t_size = t.size();
        this->s = s;
        this->t = t;
        construct_im(T, t, 1);
        if (trace) cout << "T done" << std::endl;
        reverse(t.begin(), t.end());
        this->t_r = t;
        construct_im(T_R, t, 1);
        if (trace) cout << "T_R done" << std::endl;
        reverse(t.begin(), t.end());
        T_li   = build_leaf_index(T,   t.size());
        if (trace) cout << "T_li done" << std::endl;
        T_R_li = build_leaf_index(T_R, t.size());
        if (trace) cout << "T_R_li done" << std::endl;
        T_ci = ChildIndex(T);
        if (trace) cout << "T_ci done" << std::endl;
        T_R_ci = ChildIndex(T_R);
        if (trace) cout << "T_R_ci done" << std::endl;
        range_tree = make_unique<HIA_RangeTree>(get_hia_range_tree(T_R, T, t.size()));
        if (trace) cout << "range tree done" << std::endl;

        while (index < s.size()) {
            string_view suffix(s.c_str()+index, s.size()-index);
            Slice slice = longest_consume_slice(T, suffix, t_size, T_ci);
            long block_len = slice.size();
            if (block_len == 0) {
                block_len = 1;
            }
            Block block(index);
            block.t_start = slice.start;
            blocks.push_back(block);
            index += block_len;
        }
        cout << "Initial blocks zoned" << std::endl;

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
                Slice ans = fuse_prefix_dummy_slice(T_R, u1_rev, u2_rev, t_r, t_size, T_R_ci);
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
                    v = fuse_prefix_dummy_slice(T, v1, v2, t, t_size, T_ci);
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
            result = fuse_substrings_HIA(*range_tree.get(), T, T_R, T_li, T_R_li, t_size, u, v);
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
            long len = longest_consume(T, block, T_ci);
            bool size_match = len == block.size();
            bool single_char_exception = block.size() == 1 && len == 0;
            if (!(size_match || single_char_exception)) {
                return false;
            }
        }

        if (slices.size() > 0) {
            for (long i = 0; i < slices.size()-1; i++) {
                string block = slices[i].apply(s);
                string next = slices[i+1].apply(s);
                if (longest_consume(T, block+next, T_ci) == (block.size() + next.size())) {
                    return false;
                }
            }
        }

        for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
            string candidate = get_candidate_string(iter);
            if (longest_consume(T, candidate, T_ci) != candidate.size()) {
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

    //if force_recalc is true, the internal data structures get recalculated
    //even if s[pos] is already c
    //this behavior is used internally
    void replace(long pos, char c, bool force_recalc = false) {
        if (s[pos] == c && !force_recalc) {
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
        iter_temp->t_start = longest_consume_slice(T, test_string, t_size, T_ci).start;

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
                long len = longest_consume(T, slice.apply(s), T_ci);
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
                Slice slice = fuse_prefix_dummy_slice(T, curr_slice, next_slice, s, t_size, T_ci);
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

    //TODO:
    //append and unappend assume that NUL never appears in the input data
    //maybe work on this?
    void append(char c) {
        s += (c^1); //append arbitrary character that isn't c
        blocks.push_back(Block(s.size()-1));
        replace(s.size()-1, c);
    }

    void unappend() {
        block_iter last_block = --blocks.end();
        if (get_block_size(last_block) == 1) {
            candidate_heap.remove(last_block->candidate);
            blocks.erase(last_block);
        }
        s.erase(s.begin() + (s.size()-1));
        if (blocks.size() == 0) {
            return;
        }
        last_block = --blocks.end();

        replace(s.size()-1, s[s.size()-1], true);
    }

    void print_candidates() {
        int index = 0;
        for (auto iter = blocks.begin(); iter != blocks.end(); iter++) {
            cout << index++ << ": ";
            cout << get_candidate_string(iter);
            cout << '\n';
        }
    }

    //returns slice of s
    Slice get_lcs() {
        if (candidate_heap.size() == 0) {
            return {0, -1};
        }
        return candidate_heap.get_highest();
    }
};

#endif
