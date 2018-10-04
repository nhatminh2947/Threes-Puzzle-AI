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

    Action TakeAction(const Board &board, const Action &player_action) override {
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

    void CloseEpisode(const std::string &flag = "") override {
        bag_ = {1, 2, 3};
        positions_ = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    };

private:
    std::vector<Board::cell_t> bag_ = {1, 2, 3};
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

    Action TakeAction(const Board &board, const Action &evil_action) override {
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

    Action TakeAction(const Board &board, const Action &evil_action) override {
        int chosen_direction = -1;
        int max_reward = -1;
        int min_tile = 17;

        for (int direction : directions_) {
            Board greedy_board = board;

            Board::reward_t reward = greedy_board.Slide(direction);

            if (reward > max_reward) {
                max_reward = reward;
                chosen_direction = direction;
            } else if (reward == max_reward) {
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

    Action TakeAction(const Board &board, const Action &evil_action) override {
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

    Action TakeAction(const Board &board, const Action &evil_action) override {
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

    Action TakeAction(const Board &board, const Action &evil_action) override {
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

        if (chosen_direction != -1) {
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

class MaxMergePlayer : public Player {
public:
    MaxMergePlayer(const std::string &args = "") : Player("name=MaxMerge role=Player " + args) {}

    Action TakeAction(const Board &board, const Action &evil_action) override {
        int max_mergable_tile = -1;
        int chosen_direction = -1;

        for (int direction : directions_) {
            Board temp_board = board;

            Board::reward_t reward = temp_board.Slide(direction);

            if (reward != -1) {
                int count = CountMergeableTile(board);
                if (count > max_mergable_tile) {
                    max_mergable_tile = count;
                    chosen_direction = direction;
                }
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }

private:
    bool IsMergeable(Board::cell_t tile_a, Board::cell_t tile_b) {
        return (tile_a == 1 && tile_b == 2) || (tile_a == 2 && tile_b == 1) ||
               (tile_a == tile_b && tile_a >= 3 && tile_b >= 3);
    }

    int CountMergeableTile(Board board) {
        int count = 0;
        int dx[] = {-1, 0, 1, 0};
        int dy[] = {0, 1, 0, -1};

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 4; ++k) {
                    int x = i + dx[k];
                    int y = j + dy[k];

                    if (0 <= x && x <= 4 && 0 <= y && y <= 4) {
                        count += (IsMergeable(board[i][j], board[x][y]));
                    }
                }
            }
        }

        return count;
    }
};

class SmartPlayer : public Player {
public:
    SmartPlayer(const std::string &args = "") : Player("name=smart role=Player " + args) {}

private:
//    static constexpr float SCORE_SUM_POWER = 3.5f;
//    static constexpr float SCORE_SUM_WEIGHT = 11.0f;
    static constexpr float SCORE_MERGES_WEIGHT = 700.0f;
    static constexpr float SCORE_EMPTY_WEIGHT = 270.0f;
    static constexpr float SCORE_LOST_PENALTY = -999999999.0f;

    Action TakeAction(const Board &board, const Action &evil_action) override {
        float max_heuristic_score = -1.0f;
        int chosen_direction = -1;

        for (int direction : directions_) {
            Board temp_board = board;

            Board::reward_t reward = temp_board.Slide(direction);

            float heuristic_score =
                    CalculateHeuristicEvaluation(board) + reward * 11.0f + (reward == -1) * SCORE_LOST_PENALTY;
            if (heuristic_score > max_heuristic_score) {
                max_heuristic_score = heuristic_score;
                chosen_direction = direction;
            }
        }

        if (chosen_direction != -1) {
            return Action::Slide(chosen_direction);
        }

        return Action();
    }

    bool IsMergeable(Board::cell_t tile_a, Board::cell_t tile_b) {
        return (tile_a == 1 && tile_b == 2) || (tile_a == 2 && tile_b == 1) ||
               (tile_a == tile_b && tile_a >= 3 && tile_b >= 3);
    }

    int CountMergeableTile(Board board) {
        int count = 0;
        int dx[] = {-1, 0, 1, 0};
        int dy[] = {0, 1, 0, -1};

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                for (int k = 0; k < 4; ++k) {
                    int x = i + dx[k];
                    int y = j + dy[k];

                    if (0 <= x && x <= 4 && 0 <= y && y <= 4) {
                        count += (IsMergeable(board[i][j], board[x][y]));
                    }
                }
            }
        }

        return count / 2;
    }

    int CountEmptyTile(Board board) {
        int count = 0;
        for (int i = 0; i < 16; ++i) {
            count += int(board(i) == 0);
        }

        return count;
    }

    float CalculateHeuristicEvaluation(Board board) {
        int empty = CountEmptyTile(board);
        int merges = CountMergeableTile(board);

        return SCORE_EMPTY_WEIGHT * empty + SCORE_MERGES_WEIGHT * merges;
    }
};

class Node {
    Board board_;
    bool is_player;
    

public:
    float CalculateHeuristicValue() {
        return 0;
    }

    bool IsTerminal() {
        return false;
    }

    bool IsPlayer() {
        return false;
    }
};

class ExpectimaxPlayer: public Player {
public:
    ExpectimaxPlayer(const std::string &args = "") : Player("name=Expectimax role=Player " + args) {}

    Action TakeAction(const Board &board, const Action &evil_action) override {

    }

private:
    float Expectiminimax(Node node, int depth) {
        if(node.IsTerminal() || depth == 0) {
            return node.CalculateHeuristicValue();
        }

        int a;
        if (node.IsPlayer()) {
            a = INT32_MIN;

            for (int i = 0; i < 4; ++i) { //direction

                a = max(a, expectiminimax(child, depth-1));
            }
        }
        else if (!node.IsPlayer()) {
            a = 0;

            for (int i = 0; i < 4; ++i) { //direction
                a = a + (Probability[child] * expectiminimax(child, depth-1));
            }
        }

        return a;
    }

    std::vector<Board::cell_t > bag_;
};