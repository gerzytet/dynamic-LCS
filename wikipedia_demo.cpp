#include <iostream>
#include <fstream>
#include <string>

#include "max_block_decomposition.cpp"

using namespace std;

int main() {
    ifstream infile("titles.txt");  // Open the file for reading

    if (!infile) {
        cerr << "Error: Could not open file 'titles.txt'" << std::endl;
        return 1;
    }

    string t;

    string line;
    int limit = 100000;
    while (getline(infile, line)) {
        t += line + '\n';
        if (!limit--) {
            break;
        }
    }

    cout << "construction started" << '\n';
    MaxBlockDecomposition mbd("", t, true);
    cout << "constructed!" << '\n';
    int x;
    cin >> x;
    cout << x;

    infile.close();  // Optional, handled by destructor
    return 0;
}
