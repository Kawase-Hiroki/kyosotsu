#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------
// マッチングアプリ・シミュレーション
// ・男女比 6:4
// ・不完全情報下での確率的選択（元のモデルを維持）
// ・マッチング成立後も真の姿が全てわかるわけではない点を考慮
// ---------------------------------------------------------

struct User {
    int id;
    bool is_male;

    double attractiveness;  // 真の魅力度 a_i [0,1]
    double disclosure;      // 情報開示度 d_i [0,1]
    double threshold;       // 要求水準 (これ以上に見えないといいねしない)
    double beta;            // 選択の判断精度（大きいほど閾値に厳格）

    // 結果記録用
    int matches = 0;            // 成立したマッチング数（見かけ上の成立）
    int true_satisfaction = 0;  // 真の魅力度が閾値を超えていたマッチング数（真の成功）
};

// 0-1に収める関数
double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

// シグモイド関数（確率的決定用）
double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

// 【重要】観測される魅力度（不完全情報のモデル化）
// 開示度(d)が低いほど、ノイズ(noise)の割合が増える
double observed_attractiveness(
    const User& target,
    std::mt19937& gen) {
    
    std::uniform_real_distribution<double> uni(0.0, 1.0);
    double noise = uni(gen); // 相手を見るたびに発生するノイズや誤解

    // 元のモデル式: (開示度 * 真の値) + (未開示分 * ノイズ)
    return target.disclosure * target.attractiveness + (1.0 - target.disclosure) * noise;
}

// 「いいね」判定（観測値に基づく）
bool decide_like(
    const User& viewer,
    const User& target,
    std::mt19937& gen) {
    
    // 見かけの魅力度を取得
    double a_obs = observed_attractiveness(target, gen);
    
    // 観測値が閾値を超えているかどうかの確率的判定
    double p = sigmoid(viewer.beta * (a_obs - viewer.threshold));

    std::uniform_real_distribution<double> uni(0.0, 1.0);
    return uni(gen) < p;
}

// シミュレーション実行
void simulate(
    std::vector<User>& males,
    std::vector<User>& females,
    int T,          // ラウンド数
    int K_male,     // 男性のいいね上限
    int K_female,   // 女性のいいね上限
    std::mt19937& gen) {

    std::uniform_int_distribution<int> male_pick(0, males.size() - 1);
    std::uniform_int_distribution<int> female_pick(0, females.size() - 1);

    for (int t = 0; t < T; ++t) {
        // --- 男性のアクション ---
        for (auto& m : males) {
            for (int k = 0; k < K_male; ++k) {
                int j = female_pick(gen); // ランダムに相手を表示
                User& f = females[j];

                // お互いに「見かけ」で評価しあう
                bool m_likes_f = decide_like(m, f, gen);
                bool f_likes_m = decide_like(f, m, gen);

                if (m_likes_f && f_likes_m) {
                    m.matches++;
                    f.matches++;

                    // 【追加考慮】マッチングは成立したが、中身は本当に良かったのか？
                    // 相手の「真の魅力度」が自分の「閾値」を超えていれば満足（成功）とする
                    if (f.attractiveness >= m.threshold) {
                        m.true_satisfaction++;
                    }
                    if (m.attractiveness >= f.threshold) {
                        f.true_satisfaction++;
                    }
                }
            }
        }

        // --- 女性のアクション ---
        for (auto& f : females) {
            for (int k = 0; k < K_female; ++k) {
                int i = male_pick(gen);
                User& m = males[i];

                bool f_likes_m = decide_like(f, m, gen);
                bool m_likes_f = decide_like(m, f, gen);

                if (f_likes_m && m_likes_f) {
                    f.matches++;
                    m.matches++;

                    // 真の満足度判定
                    if (m.attractiveness >= f.threshold) {
                        f.true_satisfaction++;
                    }
                    if (f.attractiveness >= m.threshold) {
                        m.true_satisfaction++;
                    }
                }
            }
        }
    }
}

// CSV出力
void output_csv(const std::vector<User>& users, const std::string& filename) {
    std::ofstream ofs(filename);
    // マッチ数だけでなく、真に満足できるマッチ数も出力
    ofs << "attractiveness,disclosure,threshold,matches,true_satisfaction\n";
    for (const auto& u : users) {
        ofs << u.attractiveness << ","
            << u.disclosure << ","
            << u.threshold << ","
            << u.matches << ","
            << u.true_satisfaction << "\n";
    }
    ofs.close();
}

int main() {
    // ----- パラメータ設定 -----
    // 男女比 6:4 (合計4000人)
    const int Nm = 2400; 
    const int Nf = 1600; 
    
    const int T = 1;        // 期間
    const int K_male = 50;  // 男性のいいね数
    const int K_female = 30;// 女性のいいね数

    std::random_device rd;
    std::mt19937 gen(rd());

    std::normal_distribution<> attr_dist(0.6, 0.15); // 魅力度の分布
    std::uniform_real_distribution<> disclosure_dist(0.2, 1.0); // 開示度はバラバラ

    std::vector<User> males;
    std::vector<User> females;

    // ----- 男性生成 -----
    for (int i = 0; i < Nm; ++i) {
        User u;
        u.id = i;
        u.is_male = true;
        u.attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        u.threshold = 0.5; // 男性の許容ライン
        u.beta = 8.0;
        males.push_back(u);
    }

    // ----- 女性生成 -----
    for (int i = 0; i < Nf; ++i) {
        User u;
        u.id = i;
        u.is_male = false;
        u.attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        u.threshold = 0.7; // 女性は少し厳しめ
        u.beta = 12.0;
        females.push_back(u);
    }

    std::cout << "Simulation Start: Male " << Nm << " : Female " << Nf << " (6:4)" << std::endl;
    
    simulate(males, females, T, K_male, K_female, gen);

    output_csv(males, "male_data.csv");
    output_csv(females, "female_data.csv");

    std::cout << "Simulation Finished." << std::endl;
    return 0;
}