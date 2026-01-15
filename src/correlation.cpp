#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* ===============================
   ピアソン相関係数
   =============================== */
double correlation(const std::vector<double>& x,
                   const std::vector<double>& y) {
    int n = x.size();
    if (n == 0 || n != y.size())
        return 0.0;

    double mean_x = 0.0, mean_y = 0.0;
    for (int i = 0; i < n; ++i) {
        mean_x += x[i];
        mean_y += y[i];
    }
    mean_x /= n;
    mean_y /= n;

    double num = 0.0, den_x = 0.0, den_y = 0.0;
    for (int i = 0; i < n; ++i) {
        double dx = x[i] - mean_x;
        double dy = y[i] - mean_y;
        num += dx * dy;
        den_x += dx * dx;
        den_y += dy * dy;
    }

    if (den_x == 0.0 || den_y == 0.0)
        return 0.0;
    return num / std::sqrt(den_x * den_y);
}

/* ===============================
   CSV読み込み（4列）
   =============================== */
void read_csv(const std::string& filename,
              std::vector<double>& attractiveness,
              std::vector<double>& disclosure,
              std::vector<double>& threshold,
              std::vector<double>& matches) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Cannot open: " << filename << std::endl;
        std::exit(1);
    }

    std::string line;
    std::getline(file, line);  // ヘッダをスキップ

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string cell;

        std::getline(ss, cell, ',');
        attractiveness.push_back(std::stod(cell));

        std::getline(ss, cell, ',');
        disclosure.push_back(std::stod(cell));

        std::getline(ss, cell, ',');
        threshold.push_back(std::stod(cell));

        std::getline(ss, cell, ',');
        matches.push_back(std::stod(cell));
    }
}

/* ===============================
   main
   =============================== */
int main() {
    std::vector<double> a_m, d_m, t_m, m_m;
    std::vector<double> a_f, d_f, t_f, m_f;

    read_csv("male.csv", a_m, d_m, t_m, m_m);
    read_csv("female.csv", a_f, d_f, t_f, m_f);

    std::cout << "=== Male ===\n";
    std::cout << "corr(attractiveness, matches) = "
              << correlation(a_m, m_m) << "\n";
    std::cout << "corr(disclosure, matches)     = "
              << correlation(d_m, m_m) << "\n";
    std::cout << "corr(threshold, matches)      = "
              << correlation(t_m, m_m) << "\n\n";

    std::cout << "=== Female ===\n";
    std::cout << "corr(attractiveness, matches) = "
              << correlation(a_f, m_f) << "\n";
    std::cout << "corr(disclosure, matches)     = "
              << correlation(d_f, m_f) << "\n";
    std::cout << "corr(threshold, matches)      = "
              << correlation(t_f, m_f) << "\n";

    return 0;
}
