#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

// ---------------------------------------------------------
// 現実的マッチングアプリシミュレーション（自然な分布版）
// ---------------------------------------------------------

struct User {
    int id;
    bool is_male;

    double true_attractiveness;  // 真の魅力
    double disclosure;           // 開示度（低いほど盛れる）
    double threshold;            // 合格ライン
    double beta;                 // 選別の厳しさ

    int matches = 0;
    int true_satisfaction = 0;  // 真の姿での満足マッチ数

    int daily_view_count = 0;   // その日の閲覧数
    int daily_match_count = 0;  // その日のマッチ成立数
};

double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

// ロジスティック確率
double logistic_prob(double utility, double beta) {
    return 1.0 / (1.0 + std::exp(-beta * utility));
}

// ---------------------------------------------------------
// 魅力観測関数（加工・情報の非対称性）
// ---------------------------------------------------------
double get_observed_attractiveness(const User& target, std::mt19937& gen) {
    // 幻想ボーナス：開示度が低いほど、相手が良いように解釈する
    // 例: disclosure=0.2 (激しい加工/マスク) -> +0.28 の加点
    double illusion_bias = (1.0 - target.disclosure) * 0.35;

    // ノイズ：開示度が低いほど、実際の姿との乖離が大きい
    std::normal_distribution<double> noise(0.0, (1.0 - target.disclosure) * 0.12);

    double obs = target.true_attractiveness + illusion_bias + noise(gen);
    return clamp01(obs);
}

// ---------------------------------------------------------
// いいね判定ロジック
// ---------------------------------------------------------
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
    int K_view_male,      // 男性が1日に見る数
    int K_review_female,  // 女性が1日に「審査」できる受信数（時間的限界）
    std::mt19937& gen) {
    // 女性の受信ボックス
    std::vector<std::vector<int>> inbox_female(females.size());

    for (int t = 0; t < T; ++t) {
        // 1. 学習（基準の調整）
        for (auto& m : males) {
            // 男性はマッチしないと焦って基準を下げる
            if (m.matches == 0)
                m.threshold *= 0.96;
            else
                m.threshold = clamp01(m.threshold * 1.01);
        }
        for (auto& f : females) {
            // 女性はインボックスの量に応じて強気になる
            // マッチ数ではなく「自分への需要」でプライドが決まる
            double demand = (double)inbox_female[f.id].size();  // 前日の受信数
            if (demand > 10)
                f.threshold = clamp01(f.threshold * 1.01);
            else
                f.threshold *= 0.99;
        }

        // 2. アルゴリズムによる表示重み（盛れてる人が優先）
        std::vector<double> female_weights;
        female_weights.reserve(females.size());
        for (const auto& f : females) {
            // 観測上のスコアが高い人ほど露出が増える
            double apparent = f.true_attractiveness + (1.0 - f.disclosure) * 0.35;
            apparent = clamp01(apparent);
            // 魅力の2乗に比例して表示されやすくなる
            female_weights.push_back(std::pow(apparent, 2.0) + 0.02);
        }
        std::discrete_distribution<int> weighted_female_pick(
            female_weights.begin(), female_weights.end());

        // 日次リセット
        for (auto& box : inbox_female) box.clear();
        for (auto& m : males) {
            m.daily_view_count = 0;
            m.daily_match_count = 0;
        }
        for (auto& f : females) {
            f.daily_view_count = 0;
            f.daily_match_count = 0;
        }

        // -------------------------------------------------------
        // 3. 男性アクション（乱れ打ち）
        // -------------------------------------------------------
        for (auto& m : males) {
            std::unordered_set<int> seen;
            while (m.daily_view_count < K_view_male) {
                int j = weighted_female_pick(gen);
                if (seen.count(j))
                    continue;
                seen.insert(j);
                m.daily_view_count++;

                User& f = females[j];
                // 男性は基準が甘い(beta低め)
                if (decide_to_like(m, f, gen)) {
                    inbox_female[j].push_back(m.id);
                }
            }
        }

        // -------------------------------------------------------
        // 4. 女性アクション（選別とソフトな飽和）
        // -------------------------------------------------------
        for (int j = 0; j < females.size(); ++j) {
            auto& f = females[j];
            auto& inbox = inbox_female[j];

            if (inbox.empty())
                continue;

            // (A) ソート：女性は「観測魅力度（加工込み）」が高い順にメールを見る
            std::vector<std::pair<double, int>> sortable_inbox;
            sortable_inbox.reserve(inbox.size());
            for (int mid : inbox) {
                double obs = get_observed_attractiveness(males[mid], gen);
                sortable_inbox.push_back({obs, mid});
            }
            // 降順ソート
            std::sort(sortable_inbox.begin(), sortable_inbox.end(),
                      [](const auto& a, const auto& b) { return a.first > b.first; });

            // (B) 審査ループ
            int review_count = 0;
            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

            for (auto& pair : sortable_inbox) {
                // 限界1：時間切れ（1日に見れるプロフィールの限界）
                if (review_count >= K_review_female)
                    break;
                review_count++;

                // 限界2：ソフトな飽和（マッチ数が増えるほど、もういいやとなる確率が上がる）
                // 例: 0人マッチ時 -> 離脱率 0%
                //     3人マッチ時 -> 離脱率 45%
                //     5人マッチ時 -> 離脱率 75%
                double fatigue_prob = f.daily_match_count * 0.15;
                if (prob_dist(gen) < fatigue_prob)
                    break;

                int mid = pair.second;
                User& m = males[mid];

                // 審査
                // inboxに入っている時点で、男性側は女性を気に入っている
                double obs = pair.first;
                double utility = obs - f.threshold;
                double p = logistic_prob(utility, f.beta);

                if (prob_dist(gen) < p) {
                    // マッチ成立
                    f.daily_match_count++;
                    m.daily_match_count++;
                    f.matches++;
                    m.matches++;

                    // 満足度（真の姿で判定）
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
}

int main() {
    // パラメータ設定
    const int Nm = 6000;
    const int Nf = 4000;
    const int T = 14;  // 2週間

    const int K_view_male = 120;  // 男性は毎日120人スワイプ

    // ここが重要：女性は上限キャップではなく「1日に吟味できる限界数」を持つ
    // 人気会員には100件来るが、上から25件しか見ない
    const int K_review_female = 25;

    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_real_distribution<> attr_dist(0.0, 1.0);
    // 開示度：0.1(超加工) ～ 1.0(無加工)
    std::uniform_real_distribution<> disclosure_dist(0.1, 1.0);

    std::vector<User> males;
    std::vector<User> females;

    // 男性生成
    for (int i = 0; i < Nm; ++i) {
        User u;
        u.id = i;
        u.is_male = true;
        u.true_attractiveness = attr_dist(gen);
        u.disclosure = disclosure_dist(gen);

        // 男性はかなり妥協する（自分 - 0.25）
        u.threshold = clamp01(u.true_attractiveness - 0.25);
        u.beta = 4.0;  // 基準付近なら確率的にいいねする（ゆるい）
        males.push_back(u);
    }

    // 女性生成
    for (int i = 0; i < Nf; ++i) {
        User u;
        u.id = i;
        u.is_male = false;
        u.true_attractiveness = attr_dist(gen);
        u.disclosure = disclosure_dist(gen);

        // 女性は自分と同等か少し下まで許容（ここを厳しくしすぎると0になる）
        // 0.8の人 -> 0.64以上
        u.threshold = clamp01(u.true_attractiveness * 0.8);

        u.beta = 10.0;  // しかし、基準を下回る相手は厳しく弾く（選り好み）
        females.push_back(u);
    }

    std::cout << "Simulation Start..." << std::endl;
    simulate(males, females, T, K_view_male, K_review_female, gen);

    output_csv(males, "male_data.csv");
    output_csv(females, "female_data.csv");

    std::cout << "Done." << std::endl;
    return 0;
}