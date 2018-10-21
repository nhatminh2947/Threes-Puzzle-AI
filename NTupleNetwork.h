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

    virtual board_t GetIndex(board_t board, int col) { return 0; }

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

    board_t GetIndex(board_t board, int col) override {
        Board64 b(board);

        board_t c1 = b.GetCol(col);
        board_t c2 = b.GetCol(col + 1);
        board_t index = ((c1 & 0xffff) << 8) | ((c2 & 0xff00) >> 8);

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                board_t index = GetIndex(b.GetBoard(), j);
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
                board_t index = GetIndex(b.GetBoard(), j);
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
        std::fill(lookup_table[0].begin(), lookup_table[0].end(), 0);
        std::fill(lookup_table[1].begin(), lookup_table[1].end(), 0);
    }

    board_t GetIndex(board_t board, int col) override {
        Board64 b(board);

        board_t c1 = b.GetCol(col);
        board_t c2 = b.GetCol(col + 1);
        board_t index = (c1&0xfff) << 12ULL | (c2&0xfff);

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        Board64 b(board);

        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 2; j++) {
                board_t index = GetIndex(b.GetBoard(), j);
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
                board_t index = GetIndex(b.GetBoard(), j);
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

class NTupleNetwork {

public:
    NTupleNetwork() {
        tuples.emplace_back(new AxeTuple());
        tuples.emplace_back(new RectangleTuple());
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
