#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// ---------------------------------------------------------
// 数理モデル準拠：不完全情報下のマッチングシミュレーション
// ---------------------------------------------------------

struct User {
    int id;
    bool is_male;

    double true_attractiveness;  // 真の魅力度 a_i [0,1]
    double disclosure;           // 情報開示度 d_i [0,1] (1=正直, 0=全加工)
    double threshold;            // 要求水準 theta_i
    double beta;                 // 合理性パラメータ (温度の逆数)

    // 結果記録用
    int matches = 0;            // 成立したマッチング数 (y_mw = 1)
    int true_satisfaction = 0;  // 事後的な満足数 (S = 1)

    // カウンター（1ターンごとにリセット）
    int daily_view_count = 0;  // プロフィールを見た数 (上限 K_view)
    int daily_like_count = 0;  // いいねを送った数 (上限 K_like)
};

// 0-1に収める関数
double clamp01(double x) {
    return std::max(0.0, std::min(1.0, x));
}

// ロジスティック関数（限定合理的な選択確率）
double logistic_prob(double utility, double beta) {
    return 1.0 / (1.0 + std::exp(-beta * utility));
}

// 観測される魅力度 (数理モデル定義: ^a_i = a_i + (1-d_i)*epsilon)
// 開示度が低いほど、正のバイアス(盛った部分)が観測値に乗る
double get_observed_attractiveness(const User& target, std::mt19937& gen) {
    // 0.0〜0.4程度の「盛り」効果が発生すると仮定
    std::uniform_real_distribution<double> bias_dist(0.0, 0.4);
    double bias = bias_dist(gen);

    // 真の値 + (隠蔽度合い * バイアス)
    double obs = target.true_attractiveness + (1.0 - target.disclosure) * bias;
    return clamp01(obs);
}

// 意思決定プロセス
// 戻り値: 「いいね」を送ろうとしたかどうか (確率的決定)
bool decide_to_like(const User& viewer, const User& target, std::mt19937& gen) {
    double a_obs = get_observed_attractiveness(target, gen);

    // 効用関数 U = 観測魅力度 - 要求水準
    double utility = a_obs - viewer.threshold;

    // ロジスティック確率で決定
    double p = logistic_prob(utility, viewer.beta);

    std::uniform_real_distribution<double> uni(0.0, 1.0);
    return uni(gen) < p;
}

// シミュレーション実行
void simulate(
    std::vector<User>& males,
    std::vector<User>& females,
    int T,              // 期間
    int K_view,         // 1日にプロフィールを見れる上限 (Constraint 3)
    int K_like_male,    // 男性の1日のいいね上限 (Constraint 3)
    int K_like_female,  // 女性の1日のいいね上限
    std::mt19937& gen) {
    std::uniform_int_distribution<int> female_pick(0, females.size() - 1);
    std::uniform_int_distribution<int> male_pick(0, males.size() - 1);

    for (int t = 0; t < T; ++t) {
        // カウンターリセット
        for (auto& m : males) {
            m.daily_view_count = 0;
            m.daily_like_count = 0;
        }
        for (auto& f : females) {
            f.daily_view_count = 0;
            f.daily_like_count = 0;
        }

        // -----------------------------------------------------
        // 男性からのアプローチフェーズ
        // -----------------------------------------------------
        // 全男性が K_view 回までプロフィール閲覧を試みる
        // ※ランダム順序にするためにシャッフル推奨だが、ここでは簡略化してループ
        for (auto& m : males) {
            // スワイプ（閲覧）ループ
            while (m.daily_view_count < K_view) {
                // 1. 相手を選ぶ
                int j = female_pick(gen);
                User& f = females[j];

                // 閲覧コスト消費
                m.daily_view_count++;

                // 2. 意思決定（確率的にいいねを送るか決める）
                if (decide_to_like(m, f, gen)) {
                    // 「いいね」を送る意思がある場合、残いいね数を確認
                    if (m.daily_like_count < K_like_male) {
                        m.daily_like_count++;  // 送信コスト消費

                        // 3. 女性側の判定（相手が自分のプロフィールを見る余裕があるか）
                        // 女性も閲覧コストを支払って男性を確認する必要がある
                        if (f.daily_view_count < K_view) {
                            f.daily_view_count++;  // 女性もコスト消費

                            // 女性も男性を評価
                            if (decide_to_like(f, m, gen)) {
                                // 女性もいいねを送る（ここでも女性のいいね上限をチェックすべきだが、
                                // 受信への「ありがとう」返信はコスト安とするか、厳密にするかで分岐。
                                // ここでは厳密に女性のいいねコストも消費させます）
                                if (f.daily_like_count < K_like_female) {
                                    f.daily_like_count++;

                                    // --- マッチング成立 (Constraint 5) ---
                                    m.matches++;
                                    f.matches++;

                                    // --- 真の満足度判定 (事後評価) ---
                                    // 相手の真の魅力度が自分の閾値を超えているか
                                    if (f.true_attractiveness >= m.threshold) {
                                        m.true_satisfaction++;
                                    }
                                    if (m.true_attractiveness >= f.threshold) {
                                        f.true_satisfaction++;
                                    }
                                }
                            }
                        }
                        // else: 女性が忙しすぎて見られなかった（機会損失）
                    }
                }
            }
        }

        // ※本来は女性から能動的に動くフェーズも同様にあるべきですが、
        // 日本のアプリ市場傾向（男性からのアプローチが主）を鑑み、
        // 今回は「男性アプローチ→女性判定」のフローをメインに記述しました。
        // 必要であればここに女性主体のループを追加してください。
    }
}

// CSV出力
void output_csv(const std::vector<User>& users, const std::string& filename) {
    std::ofstream ofs(filename);
    ofs << "id,true_attr,disclosure,threshold,matches,true_satisfaction,view_count,like_count\n";
    for (const auto& u : users) {
        ofs << u.id << ","
            << u.true_attractiveness << ","
            << u.disclosure << ","
            << u.threshold << ","
            << u.matches << ","
            << u.true_satisfaction << ","
            << u.daily_view_count << ","
            << u.daily_like_count << "\n";
    }
    ofs.close();
}

int main() {
    // ----- パラメータ設定 (モデル要件に基づく) -----
    // 市場構造: 男性過多
    const int Nm = 6000;  // 規模を少し縮小して高速化
    const int Nf = 4000;

    const int T = 1;               // 1期間（1日）
    const int K_view = 100;         // 1日に見れるプロフ数上限
    const int K_like_male = 30;    // 男性のいいね上限（閲覧数より少ない）
    const int K_like_female = 30;  // 女性のいいね上限

    std::random_device rd;
    std::mt19937 gen(rd());

    // 魅力度は正規分布、開示度は一様分布
    std::normal_distribution<> attr_dist(0.5, 0.15);
    std::uniform_real_distribution<> disclosure_dist(0.1, 1.0);  // 0.1(盛り強) ~ 1.0(正直)

    std::vector<User> males;
    std::vector<User> females;

    // ----- 男性生成 -----
    for (int i = 0; i < Nm; ++i) {
        User u;
        u.id = i;
        u.is_male = true;
        u.true_attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        // 男性は競争が激しいため、閾値を少し妥協しがち、かつ数打ちゃ当たる戦略(beta低め)
        u.threshold = 0.45;
        u.beta = 5.0;
        males.push_back(u);
    }

    // ----- 女性生成 -----
    for (int i = 0; i < Nf; ++i) {
        User u;
        u.id = i;
        u.is_male = false;
        u.true_attractiveness = clamp01(attr_dist(gen));
        u.disclosure = disclosure_dist(gen);
        // 女性は売り手市場なので閾値高め、かつ慎重(beta高め)
        u.threshold = 0.65;
        u.beta = 10.0;
        females.push_back(u);
    }

    std::cout << "Simulation Start..." << std::endl;
    simulate(males, females, T, K_view, K_like_male, K_like_female, gen);

    output_csv(males, "male_data.csv");
    output_csv(females, "female_data.csv");

    std::cout << "Done. Check CSV files." << std::endl;
    return 0;
}