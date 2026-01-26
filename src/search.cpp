#include <bits/stdc++.h>
using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " data.csv" << endl;
        return 1;
    }

    ifstream file(argv[1]);
    if (!file) {
        cerr << "Cannot open file." << endl;
        return 1;
    }

    string line;
    getline(file, line);  // ヘッダ読み飛ばし

    int count = 0;

    while (getline(file, line)) {
        stringstream ss(line);
        string cell;
        vector<string> cols;

        while (getline(ss, cell, ',')) {
            cols.push_back(cell);
        }

        if (cols.size() >= 6) {
            int true_satisfaction = stoi(cols[5]);
            if (true_satisfaction == 0) {
                count++;
            }
        }
    }

    cout << "true_satisfaction == 0 の数: " << count << endl;

    return 0;
}
