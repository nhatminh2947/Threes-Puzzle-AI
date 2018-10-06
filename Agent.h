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

            if (score > max_score) {
                chosen_direction = direction;
                max_score = score;
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
    ExpectimaxPlayer(const std::string &args = "") : Player("name=Expectimax role=Player " + args) {}

    Action TakeAction(const Board64 &board, const Action &evil_action) override {
        float max_score = INT64_MIN;
        int chosen_direction = -1;
        bag_.erase(Action::Place(evil_action).tile());

        if (bag_.empty()) {
            bag_ = {1, 2, 3};
        }

        for (int direction : directions_) {
            Board64 temp_board = board;

            reward_t reward = temp_board.Slide(direction);
            if (reward == -1) continue;

            float score = Expectiminimax(temp_board, 3, false, bag_);

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
    float Expectiminimax(Board64 board, int depth, int player_move, std::set<cell_t> bag) {
        if (IsTerminal(board)) {
            return SCORE_LOST_PENALTY;
        }

        if (depth == 0) {
            return CalculateHeuristicValue(board);
        }

        float score;
        if (player_move == -1) {
            score = INT64_MIN;

            for (int d = 0; d < 4; ++d) { //direction
                Board64 child = Board64(board);
                child.Slide(d);

                score = fmaxf(score, Expectiminimax(child, depth - 1, d, bag));
            }
        } else {
            score = 0;
            std::array<unsigned int, 4> positions = getPlacingPosition(player_move);
            if (bag.empty()) {
                bag = {1, 2, 3};
            }

            int placing_position = 0;
            for (int i = 0; i < 4; ++i) {
                placing_position += (board(positions[i]) == 0);
            }

            for (int i = 0; i < 4; ++i) {
                for (cell_t tile : bag) {
                    if (board(positions[i]) != 0) continue;

                    Board64 child = Board64(board);
                    child.Place(positions[i], tile);

                    std::set<cell_t> child_bag = bag;
                    child_bag.erase(tile);

                    score += ((1.0f / placing_position) * (1.0 / bag.size()) *
                              Expectiminimax(child, depth - 1, -1, child_bag));
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
        Board64 temp_board = Board64(board);

        for (int direction = 0; direction < 4; ++direction) {
            if (Board64(temp_board).Slide(direction) != -1)
                return false;
        }

        return true;
    }

    float CalculateHeuristicValue(Board64 &board) {
//        int empty = CountEmptyTile(board);
//        int merges = CountMergeableTile(board);
//
//        return SCORE_EMPTY_WEIGHT * empty + SCORE_MERGES_WEIGHT * merges;
        return 0;
    }


    static constexpr float SCORE_SUM_POWER = 3.5f;
    static constexpr float SCORE_SUM_WEIGHT = 11.0f;
    static constexpr float SCORE_MERGES_WEIGHT = 700.0f;
    static constexpr float SCORE_EMPTY_WEIGHT = 270.0f;
    static constexpr float SCORE_LOST_PENALTY = -999999999.0f;

    std::set<cell_t> bag_;
};
