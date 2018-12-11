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
                                                      bag_({0,4,4,4}) {}

    Action TakeAction(Board64 &board, const Action &player_action) {
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
        if(hint == 0) {
            do {
                hint = engine_() % 3 + 1;
            } while (bag_[hint] == 0);
        }

        std::shuffle(positions_.begin(), positions_.end(), engine_);

        for (unsigned int position : positions_) {
            int value = board.operator()(position);
            if (value != 0) continue;

            total_generated_tiles_++;
            bag_[hint]--;

            Action::Place action = Action::Place(position, hint);

            bool bonus = true;
            hint = GetBonusRandomTile(board);

            if (hint == 0) {
                bool empty_bag = true;
                for(int i = 1; i <= 3; i++) {
                    if(bag_[i] != 0) {
                        empty_bag = false;
                    }
                }

                if(empty_bag) {
                    for(int i = 1; i <= 3; i++) {
                        bag_[i] = 4;
                    }
                }

                do {
                    hint = engine_() % 3 + 1;
                } while (bag_[hint] == 0);
            }

            board.SetHint(hint);

            return action;
        }

        return Action();
    }

    void CloseEpisode(const std::string &flag = "") override {
        for(int i = 1; i <= 3; i++) {
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

/**
 * dummy player
 * select a legal action randomly
 */

class RandomPlayer : public RandomAgent {

public:
    RandomPlayer(const std::string &args = "") : RandomAgent("name=dummy role=Player " + args),
                                                 directions_({0, 1, 2, 3}) {}

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        std::shuffle(directions_.begin(), directions_.end(), engine_);
        for (int direction : directions_) {
            reward_t reward = Board64(board).Slide(direction);
            if (reward != -1) return Action::Slide(direction);
        }
        return Action();
    }

private:
    std::array<int, 4> directions_;
};

class GreedyPlayer : public Player {
public:
    explicit GreedyPlayer(const std::string &args = "") : Player("name=greedy role=Player " + args) {}

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        int chosen_direction = -1;
        float max_score = INT64_MIN;

        for (int direction : directions_) {
            Board64 greedy_board = board;

            reward_t score = greedy_board.Slide(direction);

            if (greedy_board == board) continue;
            float heuristic_score = greedy_board.GetHeuristicScore() + greedy_board.GetMaxTile() * 10;

            if (heuristic_score > max_score) {
                chosen_direction = direction;
                max_score = heuristic_score;
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }
};

class ExpectimaxPlayer : public Player {
public:
    ExpectimaxPlayer(const std::string &args = "", int depth = 2, int bag = 0x7) : Player(
            "name=Expectimax role=Player " + args),
                                                                                   depth_(depth),
                                                                                   bag_(bag) {}

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        float max_score = INT64_MIN;
        int chosen_direction = -1;
        int tile = Action::Place(evil_action).tile();

        if (ok) {
            bag_ = bag_ ^ (1 << (tile - 1));

            if (bag_ == 0) {
                bag_ = 0x7;
            }
        }

        ok = true;

        for (int direction : directions_) {
            Board64 temp_board = board;

            temp_board.Slide(direction);
            if (temp_board == board) continue;

            float score = Expectimax(temp_board, depth_, direction, bag_);

            if (score > max_score) {
                max_score = score;
                chosen_direction = direction;
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }

private:
    float Expectimax(Board64 board, int depth, int player_move, int bag) {
        if (IsTerminal(board)) {
            return INT32_MIN;
        }

        if (depth == 0) {
            return board.GetHeuristicScore();
        }

        float score;
        if (player_move == -1) {
            score = INT64_MIN;

            for (int d = 0; d < 4; ++d) { //direction
                Board64 child = Board64(board);
                child.Slide(d);
                if (child == board) continue;

                score = fmaxf(score, Expectimax(child, depth - 1, d, bag));
            }
        } else {
            score = 0.0f;
            std::array<unsigned int, 4> positions = getPlacingPosition(player_move);

            if (bag == 0) {
                bag = 0x7;
            }

            int placing_position = 0;
            for (int i = 0; i < 4; ++i) {
                placing_position += (board(positions[i]) == 0);
            }

            int bag_size = 0;
            for (int tile = 1; tile <= 3; ++tile) {
                if (((1 << (tile - 1)) & bag) != 0) {
                    bag_size++;
                }
            }

            for (int i = 0; i < 4; ++i) {
                if (board(positions[i]) != 0) continue;

                for (int tile = 1; tile <= 3; ++tile) {
                    if (((1 << (tile - 1)) & bag) != 0) {
                        Board64 child = Board64(board);
                        child.Place(positions[i], tile);

                        int child_bag = bag ^(1 << (tile - 1));

                        score += ((1.0f / placing_position) * (1.0f / bag_size)) *
                                 Expectimax(child, depth - 1, -1, child_bag);
                    }
                }
            }

        }

        return score;
    }

    std::array<unsigned int, 4> getPlacingPosition(int player_move) {
        if (player_move == 0) {
            return {12, 13, 14, 15};
        }

        if (player_move == 1) {
            return {0, 4, 8, 12};
        }

        if (player_move == 2) {
            return {0, 1, 2, 3};
        }

        return {3, 7, 11, 15};
    }

    bool IsTerminal(Board64 &board) {
        for (int direction = 0; direction < 4; ++direction) {
            Board64 temp_board = Board64(board);
            temp_board.Slide(direction);
            if (temp_board != board)
                return false;
        }

        return true;
    }

    int bag_;
    int depth_;
    bool ok = false;
};

class TdLambdaPlayer : public Player {
public:
    TdLambdaPlayer(const std::string &args = "") : Player("name=TDLambda role=Player " + args),
                                                   lambda_(0.5), learning_rate_(0.0025), board_appears_3072_(0),
                                                   current_tuple_(0), board_appears_1536_(0), tuple_size_(3),
                                                   count_after_3072_game_(0), count_after_1536_game_(0) {

        tuple_network_ = std::vector<NTupleNetwork>(tuple_size_);
        file_name_ = std::vector<std::string>(tuple_size_);
        limit_update_ = int(ceil(log(0.1) / log(lambda_))) - 1;

        if (meta_.find("load") != meta_.end()) {
            std::string file_name = meta_["load"].value;
            load(file_name);
        }

        if (meta_.find("alpha") != meta_.end())
            learning_rate_ = double(meta_["alpha"]);

        if (meta_.find("save") != meta_.end()) { // pass save=... to save to a specific file
            std::string file_name = meta_["save"].value;

            for (int i = 0; i < tuple_size_; ++i) {
                file_name_[i] = file_name.insert(file_name.size() - 4, std::to_string(i));
            }
        }
    };

    void CloseEpisode(const std::string &flag = "") override {
        Reset();
    }

    void Reset() {
        current_tuple_ = 0;
        board_appears_1536_ = 0;
        board_appears_3072_ = 0;
    }

    void Learn(const Episode &episode) {
        std::vector<Episode::Move> moves = episode.GetMoves();
        moves.push_back(moves.back());

        UpdateTupleValue(moves, moves.size() - 1, -V(moves.back().board));

        for (int i = moves.size() - 3; i >= 1; i -= 2) {
            Episode::Move state_t1 = moves[i];
            Episode::Move state_t2 = moves[i + 2];

            board_t board_t1 = state_t1.board;
            board_t board_t2 = state_t2.board;

            double reward = GetReward(board_t1, board_t2);
            double delta = reward + (V(board_t2) - V(board_t1));

            UpdateTupleValue(moves, i, delta);

            if ((current_tuple_ == 2 && board_t1 == board_appears_3072_) ||
                (current_tuple_ == 1 && board_t1 == board_appears_1536_)) {
                current_tuple_--;
            }
        }

        if (current_tuple_ < 0) {
            std::exit(-1);
        }
    }

    void UpdateTupleValue(std::vector<Episode::Move> moves, int t, double delta) {
        int current = current_tuple_;
        for (int k = t, count = 0; k >= 1 && count < limit_update_; k -= 2, count++) {
            Episode::Move state_t1 = moves[k];

            board_t board_t1 = state_t1.board;
            Board64 board = Board64(state_t1.board, state_t1.hint);

            tuple_network_[current].UpdateValue(board, learning_rate_ * pow(lambda_, t - k) * delta);

            if ((current == 2 && board_t1 == board_appears_3072_) ||
                (current == 1 && board_t1 == board_appears_1536_)) {
                current--;
            }
        }
    }

    double GetReward(board_t board_t1, board_t board_t2) {
        return GetBoardScore(board_t2) - GetBoardScore(board_t1);
    }

    bool FindTile(Board64 board, int tile) {
        for (int i = 0; i < 16; ++i) {
            if (tile == board(i)) {
                return true;
            }
        }

        return false;
    }

    bool TrainingFinished(int stage, int limit = 5000000) {
        switch (stage) {
            case 1:
                return count_after_1536_game_ > limit;
            case 2:
                return count_after_3072_game_ > limit;
            default:
                return false;
        }
    }

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        if (current_tuple_ < 1 && FindTile(board, 12)) {
            current_tuple_ = 1;
            count_after_1536_game_++;
            board_appears_1536_ = board.GetBoard();
        }

        if (current_tuple_ < 2 && FindTile(board, 13)) {
            current_tuple_ = 2;
            count_after_3072_game_++;
            board_appears_3072_ = board.GetBoard();
        }

        return Policy(board);
    }

    Action Policy(Board64 board) {
        double max_score = INT64_MIN;
        int chosen_direction = -1;

        for (int direction : directions_) {
            Board64 temp_board = board;

            temp_board.Slide(direction);
            if (temp_board == board) continue;

            double score =
                    (GetBoardScore(temp_board.GetBoard()) - GetBoardScore(board.GetBoard())) + V(temp_board.GetBoard());

            if (score > max_score) {
                max_score = score;
                chosen_direction = direction;
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }

    double V(board_t board) {
        return tuple_network_[current_tuple_].GetValue(board);
    }

    void save() {
        for (int i = 0; i < tuple_size_; ++i) {
            std::ofstream save_stream(file_name_[i].c_str(), std::ios::out | std::ios::trunc);
            if (!save_stream.is_open()) std::exit(-1);

            tuple_network_[i].save(save_stream);
            save_stream.close();
        }
    }

    void load(std::string file_name) {
        std::string fn = file_name;
        for (int i = 0; i < tuple_size_; ++i) {
            fn.insert(fn.size() - 4, std::to_string(i));

            std::ifstream load_stream(fn.c_str());
            if (!load_stream.is_open()) std::exit(-1);

            tuple_network_[i].load(load_stream);
            load_stream.close();
        }
    }

private:
    int limit_update_;
    int current_tuple_;
    int tuple_size_;
    board_t board_appears_1536_;
    board_t board_appears_3072_;
    size_t count_after_3072_game_;
    size_t count_after_1536_game_;
    double learning_rate_;
    double lambda_;
    std::vector<std::string> file_name_;
    std::vector<NTupleNetwork> tuple_network_;
};