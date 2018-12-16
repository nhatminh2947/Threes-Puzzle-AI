#pragma once

#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <set>

#include "Common.h"
#include "Board64.h"
#include "Action.h"
#include "Episode.h"
#include "NTupleNetwork.h"


class Agent {
public:
    Agent(const std::string &args = "") {
        std::stringstream ss("name=unknown role=unknown " + args);
        for (std::string pair; ss >> pair;) {
            std::string key = pair.substr(0, pair.find('='));
            std::string value = pair.substr(pair.find('=') + 1);
            meta_[key] = {value};
        }
    }

    virtual ~Agent() {}

    virtual void OpenEpisode(const std::string &flag = "") {}

    virtual void CloseEpisode(const std::string &flag = "") {}

    virtual Action TakeAction(const Board64 &b, const Action &prev_action) { return Action(); }

    virtual bool CheckForWin(const Board64 &b) { return false; }

public:
    virtual std::string property(const std::string &key) const { return meta_.at(key); }

    virtual void notify(const std::string &msg) {
        meta_[msg.substr(0, msg.find('='))] = {msg.substr(msg.find('=') + 1)};
    }

    virtual std::string name() const { return property("name"); }

    virtual std::string role() const { return property("role"); }

protected:
    typedef std::string key;

    struct value {
        std::string value;

        operator std::string() const { return value; }

        template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
        operator numeric() const { return numeric(std::stod(value)); }
    };

    std::map<key, value> meta_;
};

class Player : public Agent {
public:
    Player(const std::string &args = "") : Agent(args),
                                           directions_({0, 1, 2, 3}) {}

protected:
    std::array<int, 4> directions_;
};

class RandomAgent : public Agent {
public:
    RandomAgent(const std::string &args = "") : Agent(args) {
        if (meta_.find("seed") != meta_.end())
            engine_.seed(int(meta_["seed"]));
    }

    virtual ~RandomAgent() {}

protected:
    std::default_random_engine engine_;
};

/**
 * Using bag = {1, 2, 3}
 * Selecting randomly a number in bag and place it randomly in board
 */
class RandomEnvironment : public RandomAgent {

public:
    RandomEnvironment(const std::string &args = "") : RandomAgent("name=random role=environment " + args),
                                                      positions_(
                                                              {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}),
                                                      popup_(1, 3),
                                                      hint(0),
                                                      bag_({0, 4, 4, 4}) {}

    Action TakeAction(const Board64 &board, const Action &player_action) {
        switch (player_action.event()) {
            case 0:
                positions_ = {12, 13, 14, 15};
                break;
            case 1:
                positions_ = {0, 4, 8, 12};
                break;
            case 2:
                positions_ = {0, 1, 2, 3};
                break;
            case 3:
                positions_ = {3, 7, 11, 15};
                break;
            default:
                positions_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
                break;
        }

        int hint = board.GetHint();
        if (board.GetBoard() == 0) {
            hint = engine_() % 3 + 1;
        }

        std::shuffle(positions_.begin(), positions_.end(), engine_);

        for (unsigned int position : positions_) {
            int value = board.operator()(position);
            if (value != 0) continue;

            total_generated_tiles_++;

            if (hint <= 3) {
                bag_[hint]--;
            }

            int next_hint = GetBonusRandomTile(board);

            if (next_hint == 0) {
                bool empty_bag = true;
                for (int i = 1; i <= 3; i++) {
                    if (bag_[i] != 0) {
                        empty_bag = false;
                    }
                }

                if (empty_bag) {
                    for (int i = 1; i <= 3; i++) {
                        bag_[i] = 4;
                    }
                }

                do {
                    next_hint = engine_() % 3 + 1;
                } while (bag_[next_hint] == 0);
            }

            return Action::Place(position, hint, next_hint);
        }

        return Action();
    }

    void CloseEpisode(const std::string &flag = "") override {
        for (int i = 1; i <= 3; i++) {
            bag_[i] = 4;
        }
        positions_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    };

private:
    int GetBonusRandomTile(Board64 board) {
        cell_t max_tile = board.GetMaxTile();

        if (max_tile < 7) {
            return 0;
        }

        if (engine_() % 21 != 0) {
            return 0;
        }

        if ((n_bonus_tile_ + 1.0) / (1.0 * total_generated_tiles_) > 1.0 / 21.0) {
            return 0;
        }

        n_bonus_tile_++;
        int upper_bound = max_tile - 3;

        return 4 + engine_() % (upper_bound - 4 + 1);
    }

private:
    int n_bonus_tile_ = 0;
    int total_generated_tiles_ = 0;
    int hint;
    std::array<int, 4> bag_;
    std::vector<unsigned int> positions_;
    std::uniform_int_distribution<int> popup_;
};

class TdLambdaPlayer : public Player {
public:
    TdLambdaPlayer(const std::string &args = "") : Player("name=TDLambda role=Player " + args),
                                                   lambda_(0.5), learning_rate_(0.0025), tuple_size_(3),
                                                   bag_({0, 4, 4, 4}) {

        tuple_network_ = std::vector<NTupleNetwork>(tuple_size_);

        if (meta_.find("load") != meta_.end()) {
            std::string file_name = meta_["load"].value;
            load(file_name);
        }

        if (meta_.find("alpha") != meta_.end())
            learning_rate_ = float(meta_["alpha"]);

        if (meta_.find("save") != meta_.end()) { // pass save=... to save to a specific file
            file_name_ = meta_["save"].value;
            std::cout << file_name_ << std::endl;
        }
    };

    void CloseEpisode(const std::string &flag = "") override {
    }

    void decreaseLearningRate10Times() {
        learning_rate_ /= 10;
    }

    void Learn(const Episode &episode) {
        std::vector<Episode::Move> moves = episode.GetMoves();
        int id = 0;

        for (unsigned i = 9; i < moves.size(); i += 2) {
            Board64 after_state(moves[i].board, moves[i].hint);
            id = GetTupleId(after_state);

            if (i + 2 < moves.size()) {
                Board64 after_state_next = Board64(moves[i + 2].board, moves[i + 2].hint);

                tuple_network_[id].UpdateValue(after_state,
                                               learning_rate_ * (GetReward(i, moves) - V(after_state, id)));
            } else {
                tuple_network_[id].UpdateValue(after_state, learning_rate_ * (-V(after_state, id)));
            }
        }
    }

    int GetTupleId(Board64 board) {
        if (board.GetMaxTile() >= 13)
            return 2;
        if (board.GetMaxTile() >= 12)
            return 1;

        return 0;
    }

    float GetReward(int t, std::vector<Episode::Move> moves) {
        reward_t reward = 0;
        float ld = 1;
        for (int n = 1; n <= 5; n++) {
            reward += ld * R(n, t, moves);
            if (n <= 3) {
                ld *= lambda_;
            }
        }

        return (1 - lambda_) * reward;
    }

    float R(int n, int t, std::vector<Episode::Move> moves) {
        reward_t reward = 0;
        int k = 0;
        for (k = 0; k <= n - 1 && t + 2 * k < moves.size(); k++) {
            reward += moves[t + 2 * k].reward;
        }
        if (t + 2 * k < moves.size()) {
            Board64 board(moves[t + 2 * k].board, moves[t + 2 * k].hint);
            reward += V(board, GetTupleId(board));
        }

        return reward;
    }

//    bool TrainingFinished(int stage, size_t limit = 5000000) {
//        switch (stage) {
//            case 1:
//                return count_after_1536_game_ > limit;
//            case 2:
//                return count_after_3072_game_ > limit;
//            default:
//                return false;
//        }
//    }

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        int tile = Action::Place(evil_action).tile();
        bag_[tile]--;

        return Policy(board);
    }

    Action Policy(Board64 board) {
        int max_tile = board.GetMaxTile();
        int depth = 0;

        //GOOD SETTING
//        if (max_tile <= 10) {
//            depth = 3;
//        } else if (max_tile <= 12) {
//            depth = 5;
//        } else if (max_tile > 12) {
//            depth = 7;
//        }

        //BEST SETTING
        if (max_tile <= 11) {
            depth = 3;
        } else {
            depth = 5;
        }

        std::pair<int, int> direction_reward = Expectimax(1, board, -1, bag_, depth);

        if (direction_reward.first != -1) {
            return Action::Slide(direction_reward.first);
        }

        return Action();
    }

    std::pair<int, int> Expectimax(int state, Board64 board, int player_move, std::array<int, 4> bag, int depth) {
        if (board.IsTerminal()) {
            return std::make_pair(-1, 0);
        }

        if (depth == 0) {
            return std::make_pair(-1, V(board, GetTupleId(board)));
        }

        if (state == 1) { // Max node - before state
            int direction = -1;
            float max_reward = -1;
            for (int d = 0; d < 4; ++d) { //direction
                Board64 child = board;
                reward_t reward = child.Slide(d);
                if (child == board) continue;

                std::pair<int, int> direction_reward = Expectimax(1 - state, child, d, bag, depth - 1);

                if (reward + direction_reward.second > max_reward) {
                    max_reward = reward + direction_reward.second;
                    direction = d;
                }
            }

            return std::make_pair(direction, max_reward);
        } else {
            float score = 0;
            int child_count = 0;
            std::vector<int> positions = GetPlacingPosition(player_move);

            bag[board.GetHint()]--;
            if (is_empty(bag)) {
                for (int i = 1; i <= 3; i++) {
                    bag[i] = 4;
                }
            }

            for (int position : positions) {
                if (board(position) != 0) continue;

                Board64 child = board;
                reward_t reward = child.Place(position, board.GetHint());

                for (int tile = 1; tile <= 3; ++tile) {
                    if (bag[tile] != 0) {
                        child.SetHint(tile);
                        std::pair<int, int> direction_reward = Expectimax(1 - state, child, -1, bag, depth - 1);

                        score += reward;
                        score += direction_reward.second;
                        child_count++;
                    }
                }
            }

            return std::make_pair(-1, score / child_count);
        }
    }

    std::vector<int> GetPlacingPosition(int player_move) {
        switch (player_move) {
            case 0:
                return {12, 13, 14, 15};
            case 1:
                return {0, 4, 8, 12};
            case 2:
                return {0, 1, 2, 3};
            case 3:
                return {3, 7, 11, 15};
            default:
                return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        }
    }

    float V(Board64 board, int id) {
        return tuple_network_[id].GetValue(board);
    }

    void save() {
        for (int i = 0; i < tuple_size_; ++i) {
            std::string name = file_name_;
            name.insert(name.size() - 4, std::to_string(i));

            std::ofstream save_stream(name.c_str(), std::ios::out | std::ios::binary);

            if (!save_stream.is_open()) std::exit(-1);

            tuple_network_[i].save(save_stream);
            save_stream.close();
            std::cout << "saved tuple_network " << i << std::endl;
        }
    }

    void load(std::string file_name) {
        for (int i = 0; i < tuple_size_; ++i) {
            std::string fn = file_name;
            fn.insert(fn.size() - 4, std::to_string(i));

            std::ifstream load_stream(fn.c_str(), std::ios::in | std::ios::binary);
            if (!load_stream.is_open()) std::exit(-1);

            tuple_network_[i].load(load_stream);
            load_stream.close();
        }
    }

private:
    int tuple_size_;
    float learning_rate_;
    float lambda_;
    std::string file_name_;
    std::vector<NTupleNetwork> tuple_network_;
    std::array<int, 4> bag_;

    bool is_empty(std::array<int, 4> bag) {
        for (int i = 1; i <= 3; i++) {
            if (bag[i] != 0) {
                return false;
            }
        }

        return true;
    }
};