set -e

g++ -std=c++17 -O1 -DNDEBUG -I ~/include -L ~/lib wikipedia_demo.cpp -o wikipedia -lsdsl -ldivsufsort -ldivsufsort64
./wikipedia
