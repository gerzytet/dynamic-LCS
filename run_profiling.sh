set -e

g++ -std=c++17 -O0 -fno-inline -DNDEBUG -I ~/include -L ~/lib main.cpp -o main -lsdsl -ldivsufsort -ldivsufsort64 -pg
./main
gprof main gmon.out > performance_analysis/analysis.txt
