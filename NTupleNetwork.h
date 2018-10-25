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
        std::fill(lookup_table_[0].begin(), lookup_table_[0].end(), 0);
        std::fill(lookup_table_[1].begin(), lookup_table_[1].end(), 0);
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
                lookup_table_[j][index] += delta;

                temp_board.ReflectVertical();

                index = GetIndex(temp_board.GetBoard(), j);
                lookup_table_[j][index] += delta;
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
                total_value += lookup_table_[j][index];

                temp_board.ReflectVertical();

                index = GetIndex(temp_board.GetBoard(), j);
                total_value += lookup_table_[j][index];
            }
            b.TurnRight();
        }

        return total_value;
    }

    void save(std::ofstream& out) override {
        for (int i = 0; i < 2; ++i) {
            out << lookup_table_[i].size();
            for (double w : lookup_table_[i]) {
                out << " " << w;
            }
            out << std::endl;
        }
//        out.write(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1)*sizeof(double));
//        out.write(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1)*sizeof(double));
    }

    void load(std::ifstream& in) override {
        for (int i = 0; i < 2; ++i) {
            int size = 0;
            in >> size;
            for (int j = 0; j < size; ++j) {
                in >> lookup_table_[i][j];
            }
        }
//        in.read(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1) * sizeof(double));
//        in.read(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1) * sizeof(double));
    }

private:
    std::array<std::array<double, SIX_TUPLE_MASK + 1>, 2> lookup_table_;
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
        for (int i = 0; i < 2; ++i) {
            out << lookup_table_[i].size();
            for (double w : lookup_table_[i]) {
                out << " " << w;
            }
            out << std::endl;
        }
//        out.write(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1)*sizeof(double));
//        out.write(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1)*sizeof(double));
    }

    void load(std::ifstream& in) override {
        for (int i = 0; i < 2; ++i) {
            int size = 0;
            in >> size;
            for (int j = 0; j < size; ++j) {
                in >> lookup_table_[i][j];
            }
        }
//        in.read(reinterpret_cast<char*>(&lookup_table_[0]), (SIX_TUPLE_MASK+1) * sizeof(double));
//        in.read(reinterpret_cast<char*>(&lookup_table_[1]), (SIX_TUPLE_MASK+1) * sizeof(double));
    }


private:
    std::array<std::array<double, SIX_TUPLE_MASK + 1>, 2> lookup_table_;
};

class ValuableTileTuple : public Tuple {
public:
    ValuableTileTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);

        int count_tile[15];
        std::fill(count_tile, count_tile+15, 0);

        for (int i = 0; i < 16; ++i) {
            count_tile[b(i)]++;
        }

        board_t index = 0;
        int p = 1;
        for (int i = 0; i < 5; i++) {
            index += count_tile[i + 10] * p;
            p *= 8;
        }

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        lookup_table_[GetIndex(board,0)] += delta;
    }

    double GetValue(board_t board) override {
        double total_value = 0.0;

        return lookup_table_[GetIndex(board,0)];
    }

    void save(std::ofstream& out) override {
        out << lookup_table_.size();
        for (double w : lookup_table_) {
            out << " " << w;
        }
        out << std::endl;
    }

    void load(std::ifstream& in) override {
        int size = 0;
        in >> size;
        for (int j = 0; j < size; ++j) {
            in >> lookup_table_[j];
        }
    }

private:
    std::array<double, 32769> lookup_table_; // (10-tile, 11-tile, 12-tile, 13-tile, 14-tile)
};

class EmptyTileTuple : public Tuple {
public:
    EmptyTileTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            index += (b(i) == 0);
        }

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        lookup_table_[GetIndex(board,0)] += delta;
    }

    double GetValue(board_t board) override {
        return lookup_table_[GetIndex(board,0)];
    }

    void save(std::ofstream& out) override {
        out << lookup_table_.size();
        for (double w : lookup_table_) {
            out << " " << w;
        }
        out << std::endl;
    }

    void load(std::ifstream& in) override {
        int size = 0;
        in >> size;
        for (int j = 0; j < size; ++j) {
            in >> lookup_table_[j];
        }
    }

private:
    std::array<double, 17> lookup_table_;
};

class DistinctTilesTuple : public Tuple {
public:
    DistinctTilesTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            index |= (1 << b(i));
        }

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        lookup_table_[GetIndex(board,0)] += delta;
    }

    double GetValue(board_t board) override {
        return lookup_table_[GetIndex(board,0)];
    }

    void save(std::ofstream& out) override {
        out << lookup_table_.size();
        for (double w : lookup_table_) {
            out << " " << w;
        }
        out << std::endl;
    }

    void load(std::ifstream& in) override {
        int size = 0;
        in >> size;
        for (int j = 0; j < size; ++j) {
            in >> lookup_table_[j];
        }
    }

private:
    std::array<double, 32769> lookup_table_;
};

class MergeableTilesTuple : public Tuple {
public:
    MergeableTilesTuple() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            if(b(i) == 0) continue;

            if((i + 1) % 4 != 0 && b(i) == b(i+1)) {
                index++;
            }
            if((i + 4) / 4 < 4 && b(i) == b(i+4)) {
                index++;
            }
        }

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        lookup_table_[GetIndex(board,0)] += delta;
    }

    double GetValue(board_t board) override {
        return lookup_table_[GetIndex(board,0)];
    }

    void save(std::ofstream& out) override {
        out << lookup_table_.size();
        for (double w : lookup_table_) {
            out << " " << w;
        }
        out << std::endl;
    }

    void load(std::ifstream& in) override {
        int size = 0;
        in >> size;
        for (int j = 0; j < size; ++j) {
            in >> lookup_table_[j];
        }
    }

private:
    std::array<double, 17> lookup_table_;
};

class NeighboringVTile : public Tuple {
public:
    NeighboringVTile() {
        std::fill(lookup_table_.begin(), lookup_table_.end(), 0);
    }

    board_t GetIndex(board_t board, int id) override {
        Board64 b(board);
        board_t index = 0;

        for (int i = 0; i < 16; ++i) {
            if(b(i) < 10) continue;

            if((i + 1) % 4 != 0 && (b(i) - 1 == b(i+1) || b(i) + 1 == b(i+1))) {
                index++;
            }
            if((i + 4) / 4 < 4 && (b(i) - 1 == b(i+1) || b(i) + 1 == b(i+1))) {
                index++;
            }
        }

        return index;
    }

    void UpdateValue(board_t board, double delta) override {
        lookup_table_[GetIndex(board,0)] += delta;
    }

    double GetValue(board_t board) override {
        return lookup_table_[GetIndex(board,0)];
    }

    void save(std::ofstream& out) override {
        out << lookup_table_.size();
        for (double w : lookup_table_) {
            out << " " << w;
        }
        out << std::endl;
    }

    void load(std::ifstream& in) override {
        int size = 0;
        in >> size;
        for (int j = 0; j < size; ++j) {
            in >> lookup_table_[j];
        }
    }

private:
    std::array<double, 17> lookup_table_;
};

class NTupleNetwork {

public:
    NTupleNetwork() {
        stage_ = 0;
        tuples.emplace_back(new AxeTuple());
        tuples.emplace_back(new RectangleTuple());
        tuples.emplace_back(new ValuableTileTuple());
        tuples.emplace_back(new DistinctTilesTuple());
        tuples.emplace_back(new MergeableTilesTuple());
        tuples.emplace_back(new EmptyTileTuple());
        tuples.emplace_back(new NeighboringVTile());
    }

//    void AddFeaturesForStage(int stage) {
//        return;
//        if(stage == stage_) return;
//
//        stage_ = stage;
//
//        switch (stage_) {
//            case 1:
//                tuples.emplace_back(new DistinctTilesTuple());
//                tuples.emplace_back(new MergeableTilesTuple());
//                tuples.emplace_back(new EmptyTileTuple());
//                break;
//            case 2:
//                tuples.emplace_back(new NeighboringVTile());
//                break;
//            default:
//                break;
//        }
//    }

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
    int stage_;
    std::vector<std::unique_ptr<Tuple>> tuples;
};


#endif //THREES_PUZZLE_AI_NTUPLENETWORK_H
