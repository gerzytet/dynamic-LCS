set -e

g++ -std=c++17 -O1 -DNDEBUG -I ~/include -L ~/lib main.cpp -o main -lsdsl -ldivsufsort -ldivsufsort64
./main
