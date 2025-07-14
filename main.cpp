#include <iostream>

using std::cout;

#include "max_block_decomposition.cpp"
#include "rand_utils.cpp"
#include "G4G_LCS.cpp"

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

//max_op 1 = replace only, 2 = append and replace, 3 = pop and 2
void test_LCS_different_lengths(int strings, int length_average, int runs_per_string, int validate_frequency, int max_op) {
    for (int i = 0; i < strings; i++) {
        int length_s = randint(length_average/2, length_average+length_average/2);
        int length_t = randint(length_average/2, length_average+length_average/2);
        string s = "";
        string t = "";
        for (int j = 0; j < length_s; j++) {
            s += (char) randint('a', 'd');
        }
        for (int j = 0; j < length_t; j++) {
            t += (char) randint('a', 'd');
        }
        MaxBlockDecomposition decomp(s, t);
        cout << decomp.blocks.size() << '\n';

        for (int j = 0; j < runs_per_string; j++) {
            long pos = randint(0, s.size()-1);
            int op = randint(1, max_op);
            char c = randint('a', 'd');

            if (op == 1) {
                if (s == "") {
                    s += c;
                    decomp.append(c);
                } else {
                    s[pos] = c;
                    decomp.replace(pos, c);
                }
            } else if (op == 2) {
                s += c;
                decomp.append(c);
            } else if (op == 3) {
                if (s == "") {
                    decomp.append(c);
                    s += c;
                } else {
                    decomp.unappend();
                    s.erase(--s.end());
                }
            }
            if (decomp.s != s) {
                cout << "BADDDD\n";
            }

            //cout << t << '\n';
            //decomp.print();
            //decomp.print_candidates();

            if (validate_frequency != 0 && (j+1) % validate_frequency == 0) {
                bool result = decomp.validate();
                if (!result) {
                    cout << "VALIDATE FAIL i: " << i << ", j: " << j << '\n';
                    decomp.print();
                    return;
                }
                string LCS_reference = G4G::LCS(s, t, 'd');
                Slice LCS = decomp.get_lcs();
                if (LCS.size() != LCS_reference.size()) {
                    cout << "FAIL: reference is " << LCS_reference.size() << " calculated is " << LCS.size() << '\n';
                    decomp.print();
                    decomp.print_candidates();
                    return;
                }
            }
        }
        cout << "PASS " << i << '\n';
    }

    cout << "PASS ALL\n";
}


//return n pairs of an index and a character
//the index will be in the range [0, str_length)
vector<pair<int, char>> generate_replacement_sequence(int str_length, int n, char min_char, char max_char) {
    vector<pair<int, char>> replacement_sequence;
    for (int i = 0; i < n; i++) {
        replacement_sequence.push_back({randint(0, str_length-1), randint(min_char, max_char)});
    }

    return replacement_sequence;
}

enum Algorithm {
    GEEKS_FOR_GEEKS,
    MAX_BLOCK_DECOMPOSITION
};

//returns number of miliseconds per run the algorithm took
double run_trial(Algorithm algorithm, int string_len, int num_runs) {
    const char minchar = 1;
    const char maxchar = 4;
    string s = randstring(string_len, minchar, maxchar);
    string t = randstring(string_len, minchar, maxchar);
    auto replacement_sequence = generate_replacement_sequence(string_len, num_runs, minchar, maxchar);
    MaxBlockDecomposition mbd;
    if (algorithm == MAX_BLOCK_DECOMPOSITION) {
        auto start = std::chrono::steady_clock::now();

        mbd = MaxBlockDecomposition(s, t);

        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        double init_time = std::chrono::duration<double, std::milli>(diff).count();
        cout << "initialized in " << init_time << "ms" << std::endl;

    }

    auto start = std::chrono::steady_clock::now();

    for (auto [i, c] : replacement_sequence) {
        if (algorithm == GEEKS_FOR_GEEKS) {
            s[i] = c;
            string lcs = G4G::LCS(s, t, 4);
        } else if (algorithm == MAX_BLOCK_DECOMPOSITION) {
            mbd.replace(i, c);
            Slice lcs = mbd.get_lcs();
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    return std::chrono::duration<double, std::milli>(diff).count() / num_runs;
}

void performance_comparison() {
    vector<pair<int, int>> trials = {
        {100,    100000},
        {500,    20000},
        {2000,   5000},
        {10000,  1000},
        {50000,  200},
        {200000, 100}
    };

    for (auto [len, runs] : trials) {
        cout << "running len: " << len << ", runs: " << runs << std::endl;
        double g4g_milli = run_trial(GEEKS_FOR_GEEKS, len, runs/5);
        cout << "geeksforgeeks " << g4g_milli << "ms" << std::endl;
        double normal_milli = run_trial(MAX_BLOCK_DECOMPOSITION, len, runs);
        cout << "block decomposition " << normal_milli << "ms" << std::endl;
        cout << std::endl;
    }
}

int main() {

    //auto mbd = MaxBlockDecomposition("aabababc", "aacbbcabc");
    //mbd.print();
    //test_fuse_substrings_auto();
    //test_initial_blocks_different_lengths();
    test_LCS_different_lengths(3, 200000, 2000, 500, 3);
    //test_LCS_different_lengths(3, 50, 2000, 1, 3);
    //performance_comparison();
    /*sdsl::csa_bitcompressed csa, csa_r;
    string s = "abacadabra";
    construct_im(csa, s, 1);
    std::reverse(s.begin(), s.end());
    construct_im(csa_r, s, 1);
    std::reverse(s.begin(), s.end());

    cout << "sa: ";
    for (int i = 0; i <= s.size(); i++) {
        cout << csa[i] << ' ';
    }
    cout << '\n';

    cout << "isa: ";
    for (int i = 0; i <= s.size(); i++) {
        cout << csa.isa[i] << ' ';
    }
    cout << '\n';

    cout << "isa_r: ";
    for (int i = 0; i <= s.size(); i++) {
        cout << csa_r.isa[i] << ' ';
    }
    cout << '\n';
    MaxBlockDecomposition mbd("", "abacadabra");*/

    //performance_comparison();
    /*string s = "";
    for (int i = 0; i < 1000000; i++) { //1MB
        s += randint('a', 'z');
    }
    MaxBlockDecomposition mbd("", s, true);*/
    //test_fuse_substrings();
    //test_range_search_2d();
    //test_leaf_index();
    //performance_comparison();
}
