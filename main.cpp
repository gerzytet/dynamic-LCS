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

void test_LCS_different_lengths(int strings, int length_average, int runs_per_string, int validate_frequency) {
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

        for (int j = 0; j < runs_per_string; j++) {
            long pos = randint(0, length_s-1);
            char c = randint('a', 'd');
            s[pos] = c;
            //cout << t << '\n';
            //decomp.print();
            //decomp.print_candidates();
            decomp.replace(pos, c);
            if ((j+1) % validate_frequency == 0) {
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
        }
        cout << "PASS " << i << '\n';
    }

    cout << "PASS ALL\n";
}

int main() {
    test_LCS_different_lengths(4, 20000, 5000, 500);
}
