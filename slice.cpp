#include <string>

using std::string;

#ifndef SLICE_CPP
#define SLICE_CPP

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

//for every substring, there is a corresponding substring in the reverse of the string
//for a substring containing the characters with indicies 0 and 1, the corresponding one will have the last 2 indicies
Slice reverse_slice(Slice slice, long string_length) {
    return {string_length-slice.end-1, string_length-slice.start-1};
}

#endif
