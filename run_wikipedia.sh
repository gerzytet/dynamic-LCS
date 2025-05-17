set -e

g++ -std=c++17 -O1 -DNDEBUG -I ~/include -L ~/lib wikipedia_server.cpp -o wikipedia_server -lsdsl -ldivsufsort -ldivsufsort64
./wikipedia_server
