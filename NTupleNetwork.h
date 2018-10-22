//
// Created by cgilab on 10/18/18.
//
#pragma once

#ifndef THREES_PUZZLE_AI_NTUPLENETWORK_H
#define THREES_PUZZLE_AI_NTUPLENETWORK_H

#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include <array>
#include "Board64.h"


class Tuple {
public:
    Tuple() = default;

    virtual double GetValue(board_t board) { return 0; }

    virtual board_t GetIndex(board_t board, int id) { return 0; }

    virtual void UpdateValue(board_t b, double delta) {}

    virtual void save(std::ofstream &out) {}

    virtual void load(std::ifstream &in) {}
};

class AxeTuple : public Tuple {
public:
    AxeTuple() : Tuple() {
        std::fill(lookup_table[0].begin(), lookup_table[0].end(), 0);
        std::fill(lookup_table[1].begin(), lookup_table[1].end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);

        board_t c1 = b.GetCol(id);
        board_t c2 = b.GetCol(id + 1);
        board_t index = ((c1 & 0xffff) << 8) | ((c2 & 0xff00) >> 8);

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                Board64 temp_board(b.GetBoard());

                board_t index = GetIndex(temp_board.GetBoard(), j);
                lookup_table[j][index] += delta;

                temp_board.ReflectVertical();

                index = GetIndex(temp_board.GetBoard(), j);
                lookup_table[j][index] += delta;
            }
            b.TurnRight();
        }
    }

    double GetValue(board_t board) override {
        double total_value = 0.0;

        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                Board64 temp_board(b.GetBoard());

                board_t index = GetIndex(temp_board.GetBoard(), j);
                total_value += lookup_table[j][index];

                temp_board.ReflectVertical();

                index = GetIndex(temp_board.GetBoard(), j);
                total_value += lookup_table[j][index];
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream& out) override {
        out.write(reinterpret_cast<char*>(&lookup_table[0]), (SIX_TUPLE_MASK+1)*sizeof(double));
        out.write(reinterpret_cast<char*>(&lookup_table[1]), (SIX_TUPLE_MASK+1)*sizeof(double));
    }

    void load(std::ifstream& in) override {
        in.read(reinterpret_cast<char*>(&lookup_table[0]), (SIX_TUPLE_MASK+1) * sizeof(double));
        in.read(reinterpret_cast<char*>(&lookup_table[1]), (SIX_TUPLE_MASK+1) * sizeof(double));
    }


private:
    std::array<std::array<double, SIX_TUPLE_MASK + 1>, 2> lookup_table;
};

class RectangleTuple : public Tuple {
public:
    RectangleTuple() {
        std::fill(lookup_table_[0].begin(), lookup_table_[0].end(), 0);
        std::fill(lookup_table_[1].begin(), lookup_table_[1].end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);

        board_t c1 = b.GetCol(id);
        board_t c2 = b.GetCol(id + 1);
        board_t index = (c1&0xfff) << 12ULL | (c2&0xfff);

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                Board64 temp_board(b.GetBoard());

                board_t index1 = GetIndex(temp_board.GetBoard(), j);
                lookup_table_[j][index1] += delta;

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board.GetBoard(), j);
                if(j == 1 && index1 != index2) {
                    lookup_table_[j][index2] += delta;
                }
            }
            b.TurnRight();
        }
    }

    double GetValue(board_t board) override {
        double total_value = 0.0;

        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                Board64 temp_board(b.GetBoard());

                board_t index1 = GetIndex(temp_board.GetBoard(), j);
                total_value += lookup_table_[j][index1];

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board.GetBoard(), j);
                if(j == 1 && index1 != index2) {
                    total_value += lookup_table_[j][index2];
                }
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream& out) override {
        out.write(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1)*sizeof(double));
        out.write(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1)*sizeof(double));
    }

    void load(std::ifstream& in) override {
        in.read(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1) * sizeof(double));
        in.read(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1) * sizeof(double));
    }


private:
    std::array<std::array<double, SIX_TUPLE_MASK + 1>, 2> lookup_table_;
};

class HeuristicTuple : public Tuple {
public:
    HeuristicTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);

        bool distinct_check[16];
        std::fill(distinct_check, distinct_check+16, false);
//        Board64::PrintBoard(board);
        int empty = 0;
        int merges = 0;
        int max_tile = INT32_MIN;
        int distinct_tile = 0;


        for (int i = 0; i < 16; ++i) {
            if (b(i) == 0) empty++;
            if (i + 1 < 4 && b(i) == b(i + 1)) merges++;
            if (i + 4 < 16 && b(i) == b(i + 4)) merges++;
            if (max_tile < b(i)) max_tile = b(i);
            if (!distinct_check[b(i)]) {
                distinct_tile++;
                distinct_check[b(i)] = true;
            }
        }

        board_t index = empty | merges << 4 | max_tile << 12 | distinct_tile << 16;

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                Board64 temp_board(b.GetBoard());

                board_t index1 = GetIndex(temp_board.GetBoard(), 0);
                lookup_table_[index1] += delta;

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board.GetBoard(), 0);
                if(j == 1 && index1 != index2) {
                    lookup_table_[index2] += delta;
                }
            }
            b.TurnRight();
        }
    }

    double GetValue(board_t board) override {
        double total_value = 0.0;

        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; ++j) {
                Board64 temp_board(b.GetBoard());

                board_t index1 = GetIndex(temp_board.GetBoard(), 0);
                total_value += lookup_table_[index1];

                temp_board.ReflectVertical();

                board_t index2 = GetIndex(temp_board.GetBoard(), 0);
                total_value += lookup_table_[index2];
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream& out) override {
        out.write(reinterpret_cast<char*>(&lookup_table_), (SIX_TUPLE_MASK+1)*sizeof(double));
    }

    void load(std::ifstream& in) override {
        in.read(reinterpret_cast<char*>(&lookup_table_), (SIX_TUPLE_MASK+1) * sizeof(double));
    }

private:
    std::array<double, SIX_TUPLE_MASK + 1> lookup_table_;
};

class NTupleNetwork {

public:
    NTupleNetwork() {
        tuples.emplace_back(new AxeTuple());
        tuples.emplace_back(new RectangleTuple());
        tuples.emplace_back(new HeuristicTuple());
    }

    double GetValue(board_t board) {
        double total_value = 0;
        for (auto &tuple : tuples) {
            total_value += tuple->GetValue(board);
        }


        return total_value;
    }

    void UpdateValue(board_t board, double delta){
        for (auto &tuple : tuples) {
            tuple->UpdateValue(board, delta);
        }
    }

    void save(std::ofstream& save_stream) {
        for (auto &tuple : tuples) {
            tuple->save(save_stream);
        }
    }

    void load(std::ifstream& load_stream) {
        for (auto &tuple : tuples) {
            tuple->load(load_stream);
        }
    }

private:
    std::vector<std::unique_ptr<Tuple>> tuples;
};


#endif //THREES_PUZZLE_AI_NTUPLENETWORK_H
