#pragma once

#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>

#include "board.h"
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

    virtual Action TakeAction(const Board &b, const Action &prev_action) { return Action(); }

    virtual bool CheckForWin(const Board &b) { return false; }

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

    virtual Action TakeAction(const Board &board, const Action &player_action) {
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

        for (unsigned int position : positions_) {
            if (board(position) != 0) continue;
            Board::cell_t tile = bag_.back();
            bag_.pop_back();
            return Action::Place(position, tile);
        }

        return Action();
    }

    virtual void CloseEpisode(const std::string &flag = "") {
        bag_ = {1, 2, 3};
        positions_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    };

private:
    std::vector<int> bag_ = {1, 2, 3};
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

    virtual Action TakeAction(const Board &board, const Action &evil_action) {
        std::shuffle(directions_.begin(), directions_.end(), engine_);
        for (int direction : directions_) {
            Board::reward_t reward = Board(board).Slide(direction);
            if (reward != -1) return Action::Slide(direction);
        }
        return Action();
    }

private:
    std::array<int, 4> directions_;
};

class GreedyPlayer : public Player {
public:
    GreedyPlayer(const std::string &args = "") : Player("name=greedy role=Player " + args) {}

    virtual Action TakeAction(const Board &board, const Action &evil_action) {
        int chosen_direction = -1;
        int max_reward = -1;
        int min_tile = 17;

        for (int direction : directions_) {
            Board greedy_board = board;

            Board::reward_t reward = greedy_board.Slide(direction);

            if (reward > max_reward) {
                max_reward = reward;
                chosen_direction = direction;
            }
            else if (reward == max_reward) {
                int count = CountTile(greedy_board);
                if (count < min_tile) {
                    min_tile = count;
                    chosen_direction = direction;
                }
            }
        }

        if (chosen_direction != -1) return Action::Slide(chosen_direction);
        return Action();
    }

private:
    int CountTile(Board board) {
        int count = 0;
        for (int i = 0; i < 16; ++i) {
            count += int(board(i) != 0);
        }

        return count;
    }
};

class MaxRewardPlayer : public Player {
public:
    MaxRewardPlayer(const std::string &args = "") : Player("name=MaxReward role=Player " + args) {}

    virtual Action TakeAction(const Board &board, const Action &evil_action) {
        int chosen_direction = -1;
        int max_reward = -1;

        for (int direction : directions_) {
            Board greedy_board = board;

            Board::reward_t reward = Board(greedy_board).Slide(direction);

            if (reward > max_reward) {
                max_reward = reward;
                chosen_direction = direction;
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }
        return Action();
    }
};

class OneDirectionPlayer : public Player {
public:
    OneDirectionPlayer(const std::string &args = "") : Player("name=OneDirection role=Player " + args) {}

    virtual Action TakeAction(const Board &board, const Action &evil_action) {
        for (int direction : directions_) {
            Board::reward_t reward = Board(board).Slide(direction);
            if (reward != -1) return Action::Slide(direction);
        }
        return Action();
    }
};

class LessTilePlayer : public Player {
public:
    LessTilePlayer(const std::string &args = "") : Player("name=LessTile role=Player " + args) {}

    virtual Action TakeAction(const Board &board, const Action &evil_action) {
        int min_tile = 17;
        int chosen_direction = -1;

        for (int direction : directions_) {
            Board temp_board = board;

            Board::reward_t reward = temp_board.Slide(direction);

            if (reward != -1) {
                int count = CountTile(temp_board);
                if (count < min_tile) {
                    min_tile = count;
                    chosen_direction = direction;
                }
            }
        }

        if(chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }

private:
    int CountTile(Board board) {
        int count = 0;
        for (int i = 0; i < 16; ++i) {
            count += int(board(i) != 0);
        }

        return count;
    }
};