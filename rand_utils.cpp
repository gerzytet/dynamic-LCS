#include <random>
#include <cmath>

#include "slice.cpp"

using std::min;
using std::max;

#ifndef RAND_UTILS_CPP
#define RAND_UTILS_CPP

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

#endif
