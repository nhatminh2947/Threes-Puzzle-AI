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
                                                      popup_(1, 3) {}

    Action TakeAction(const Board64 &board, const Action &player_action) override {
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

        if (bag_.empty()) {
            bag_ = {1, 2, 3};
        }

        std::shuffle(positions_.begin(), positions_.end(), engine_);
        std::shuffle(bag_.begin(), bag_.end(), engine_);

        for (int position : positions_) {
            board_t value = board.operator()(position);
            if (value != 0) continue;

            cell_t tile = bag_.back();
            bag_.pop_back();
            return Action::Place(position, tile);
        }

        return Action();
    }

    void CloseEpisode(const std::string &flag = "") override {
        bag_ = {1, 2, 3};
        positions_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    };

private:
    std::vector<cell_t> bag_ = {1, 2, 3};
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

class TdLearningPlayer : public Player {
public:
    TdLearningPlayer(const std::string &args = "") : Player("name=TdLearning role=Player " + args),
                                                     learning_rate(0.00025) {
        tuple_network = NTupleNetwork();
        if (meta_.find("alpha") != meta_.end())
            learning_rate = float(meta_["alpha"]);
    };

    void load_weights(const std::string& path) {
        std::ifstream in(path, std::ios::in | std::ios::binary);
        if (!in.is_open()) std::exit(-1);
        uint32_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
//        net.resize(size);
//        for (weight& w : net) in >> w;



        in.close();
    }

    void save_weights(const std::string& path) {
//        std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
//        if (!out.is_open()) std::exit(-1);
//        uint32_t size = net.size();
//        out.write(reinterpret_cast<char*>(&size), sizeof(size));
//        for (weight& w : net) out << w;
//        out.close();
    }

    void Learn(Episode episode) {
        std::vector<Episode::Move> moves = episode.GetMoves();

        std::cout << "Move size = " << moves.size() << std::endl;

        //Training last state
        Episode::Move state_t1 = moves.back();
        Episode::Move state_t2 = moves.back();

        board_t board_t1 = state_t1.board;
        board_t board_t2 = state_t2.board;

        double reward = Board64::GetBoardScore(board_t2) - Board64::GetBoardScore(board_t2);

        tuple_network.UpdateValue(board_t1, learning_rate * (reward + V(board_t2) - V(board_t1)));

        for (int i = moves.size() - 2; i >= 1; --i) {
            Episode::Move state_t1 = moves[i];
            Episode::Move state_t2 = moves[i+1];

            board_t board_t1 = state_t1.board;
            board_t board_t2 = state_t2.board;

            double reward = Board64::GetBoardScore(board_t2) - Board64::GetBoardScore(board_t2);

            tuple_network.UpdateValue(board_t1, learning_rate * (reward + V(board_t2) - V(board_t1)));
        }
    }

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        return Policy(board);
    }

    Action Policy(Board64 board) {
        float max_score = INT64_MIN;
        int chosen_direction = -1;

        for (int direction : directions_) {
            Board64 temp_board = board;

            temp_board.Slide(direction);
            if (temp_board == board) continue;

            double score = Board64::GetBoardScore(temp_board.GetBoard()) + V(temp_board.GetBoard());

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
        return tuple_network.GetValue(board);
    }

private:
    double learning_rate;
    NTupleNetwork tuple_network;
};
