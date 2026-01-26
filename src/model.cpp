#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

// ---------------------------------------------------------
// 数理モデル準拠：不完全情報下のマッチングシミュレーション
// ---------------------------------------------------------

struct User {
    int id;
    bool is_male;

    double true_attractiveness;  // 真の魅力度 a_i [0,1]
    double disclosure;           // 情報開示度 d_i [0,1]
    double threshold;            // 要求水準 theta_i
    double beta;                 // 合理性パラメータ

    int matches = 0;            // 成立したマッチング数
    int true_satisfaction = 0;  // 事後的な満足数

    int daily_view_count = 0;
    int daily_like_count = 0;
};

double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

double logistic_prob(double utility, double beta) {
    return 1.0 / (1.0 + std::exp(-beta * utility));
}

double get_observed_attractiveness(const User& target, std::mt19937& gen) {
    std::normal_distribution<double> noise(0.0, (1.0 - target.disclosure) * 0.2);
    double obs = target.true_attractiveness + noise(gen);
    return clamp01(obs);
}

bool decide_to_like(const User& viewer, const User& target, std::mt19937& gen) {
    double a_obs = get_observed_attractiveness(target, gen);
    double utility = a_obs - viewer.threshold;
    double p = logistic_prob(utility, viewer.beta);
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    return uni(gen) < p;
}

void simulate(
    std::vector<User>& males,
    std::vector<User>& females,
    int T,
    int K_view,
    int K_like_male,
    int K_like_female,
    std::mt19937& gen) {
    std::vector<double> female_weights;
    female_weights.reserve(females.size());
    for (const auto& f : females) {
        double estimated_bias = 0.25;
        double public_appearance = f.true_attractiveness + (1.0 - f.disclosure) * estimated_bias;
        public_appearance = clamp01(public_appearance);
        female_weights.push_back(std::pow(public_appearance, 6.0));  // ★弱めた
    }
    std::discrete_distribution<int> weighted_female_pick(female_weights.begin(), female_weights.end());

    std::vector<std::vector<int>> inbox_female(females.size());

    for (int t = 0; t < T; ++t) {
        for (auto& m : males) {
            m.daily_view_count = 0;
            m.daily_like_count = 0;
        }
        for (auto& f : females) {
            f.daily_view_count = 0;
            f.daily_like_count = 0;
        }

        for (auto& box : inbox_female) box.clear();

        // -------- 男性フェーズ --------
        for (auto& m : males) {
            std::unordered_set<int> seen;

            while (m.daily_view_count < K_view) {
                int j = weighted_female_pick(gen);
                if (seen.count(j))
                    continue;
                seen.insert(j);

                User& f = females[j];
                m.daily_view_count++;

                if (decide_to_like(m, f, gen)) {
                    if (m.daily_like_count < K_like_male) {
                        m.daily_like_count++;
                        f.daily_view_count++;
                        inbox_female[j].push_back(m.id);
                    }
                }
            }
        }

        // -------- 女性処理 --------
        for (int j = 0; j < females.size(); ++j) {
            auto& f = females[j];

            for (int mid : inbox_female[j]) {
                if (f.daily_like_count >= K_like_female)
                    break;

                User& m = males[mid];

                if (decide_to_like(f, m, gen)) {
                    f.daily_like_count++;

                    m.matches++;
                    f.matches++;

                    if (f.true_attractiveness >= m.threshold)
                        m.true_satisfaction++;
                    if (m.true_attractiveness >= f.threshold)
                        f.true_satisfaction++;
                }
            }
        }
    }
}

void output_csv(const std::vector<User>& users, const std::string& filename) {
    std::ofstream ofs(filename);
    ofs << "id,true_attr,disclosure,threshold,matches,true_satisfaction\n";
    for (const auto& u : users) {
        ofs << u.id << "," << u.true_attractiveness << "," << u.disclosure << ","
            << u.threshold << "," << u.matches << "," << u.true_satisfaction << "\n";
    }
    ofs.close();
}

int main() {
    const int Nm = 6000;
    const int Nf = 4000;
    const int T = 10;
    const int K_view = 100;
    const int K_like_male = 20;
    const int K_like_female = 20;

    const double delta = 0.2;  // ★少しだけ厳しく

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> attr_dist(0.5, 0.15);
    std::uniform_real_distribution<> disclosure_dist(0.1, 1.0);

    std::vector<User> males;
    std::vector<User> females;

    for (int i = 0; i < Nm; ++i) {
        User u;
        u.id = i;
        u.is_male = true;
        u.true_attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        u.threshold = delta;//std::max(0.0, u.true_attractiveness - delta);
        u.beta = 6.0;
        males.push_back(u);
    }

    for (int i = 0; i < Nf; ++i) {
        User u;
        u.id = i;
        u.is_male = false;
        u.true_attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        u.threshold = u.true_attractiveness;
        u.beta = 12.0;
        females.push_back(u);
    }

    std::cout << "Simulation Start..." << std::endl;
    simulate(males, females, T, K_view, K_like_male, K_like_female, gen);

    output_csv(males, "male_data.csv");
    output_csv(females, "female_data.csv");

    std::cout << "Done." << std::endl;
    return 0;
}
