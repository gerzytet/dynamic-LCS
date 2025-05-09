#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <optional>
#include <cctype>

#include "max_block_decomposition.cpp"

using namespace std;

string t;

string lower(string s) {
    string ans = "";
    for (char c : s) {
        ans += tolower(c);
    }
    return ans;
}

//error values
//0 = closed
//-1 = socket error
//1 = ok
string buffer;
string readUntilDelimiter(int sockfd, char delimiter, int *error) {
    char temp[1024];

    while (true) {
        // Check for delimiter in buffer
        size_t pos = buffer.find(delimiter);
        if (pos != string::npos) {
            string result = buffer.substr(0, pos);
            buffer = buffer.substr(pos + 1); // Remove processed part
            *error = 1;
            return result;
        }

        // Read from socket
        ssize_t bytesRead = recv(sockfd, temp, sizeof(temp), 0);
        if (bytesRead < 0) {
            // Error
            *error = bytesRead;
            return "";
        } else if (bytesRead == 0) {
            // Connection closed
            if (!buffer.empty()) {
                std::string result = buffer;
                buffer.clear();
                return result;
            }
            *error = 0;
            return "";
        }

        buffer.append(temp, bytesRead);
    }
}

struct SearchResult {
    string title;
    Slice match;

    bool operator<(SearchResult other) {
        return title.size() < other.title.size();
    }
};

SearchResult get_result(Slice t_slice) {
    long title_begin = t_slice.start;
    while (title_begin > 0 && t[title_begin-1] != '\n') {
        title_begin--;
    }
    long title_end = t_slice.end;
    while (title_end < t.size()-1 && t[title_end+1] != '\n') {
        title_end++;
    }

    return {Slice{title_begin, title_end}.apply(t), {t_slice.start - title_begin, t_slice.end - title_begin}};
}

string answer_query(string diff, MaxBlockDecomposition &mbd) {
    diff = lower(diff);
    auto iter = diff.begin();
    auto next = [&]() {
        return *(iter++);
    };

    while (iter != diff.end()) {
        char op = next();

        if (op == 'r') {
            string index = "";
            while (*iter >= '0' && *iter <= '9') {
                index += next();
            }
            char replacement = next();
            mbd.replace(atol(index.c_str()), replacement);
        } else if (op == '+') {
            mbd.append(next());
        } else if (op == '-') {
            mbd.unappend();
        } else {
            cout << "BAD OP: " + op << endl;
            return "";
        }
    }

    Slice lcs = mbd.get_lcs();
    vector<Slice> t_matches = mbd.get_t_matches(lcs, 20);
    vector<SearchResult> results(t_matches.size());
    for (int i = 0; i < t_matches.size(); i++) {
        results[i] = get_result(t_matches[i]);
    }

    sort(results.begin(), results.end());

    string reply = "";
    reply += to_string(results.size()) + '\n';
    for (auto result : results) {
        reply += result.title + '\n' + to_string(result.match.start) + '\n' + to_string(result.match.end) + '\n';
    }

    return reply + '\n';
}

int main() {
    ifstream infile("titles.txt");  // Open the file for reading

    if (!infile) {
        cerr << "Error: Could not open file 'titles.txt'" << std::endl;
        return 1;
    }

    string line;
    int limit = 250000;
    while (getline(infile, line)) {
        for (char &c : line) {
            if (c == '_') {
                c = ' ';
            }
        }
        t += line + '\n';
        if (!limit--) {
            break;
        }
    }

    cout << "construction started" << '\n';
    MaxBlockDecomposition mbd("", lower(t), true);
    cout << "constructed!" << '\n';

    const int PORT = 19921;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 1) == -1) {
        perror("listen");
        return 1;
    }

    std::cout << "Server listening on port " << PORT << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        cout << "new connection, clearing search\n";
        while (mbd.s.size() > 0) {
            mbd.unappend();
        }

        while (true) {
            int error;
            string result = readUntilDelimiter(client_fd, '\n', &error);
            if (error != 1) {
                cout << "closing on read... with error " << error << '\n';
                close(client_fd);
                buffer = "";
            } else {
                cout << "got message: " << result << endl;
            }
            string answer = answer_query(result, mbd);
            cout << "s: " << mbd.s << '\n';
            cout << "LCS len: " << mbd.get_lcs().size() << '\n';
            cout << "LCS: " << mbd.get_lcs().apply(mbd.s) << '\n';

            int sent = send(client_fd, answer.c_str(), answer.size(), 0);
            if (sent == -1) {
                close(client_fd);
                break;
            }
        }
    }

    close(server_fd);
    return 0;
}
