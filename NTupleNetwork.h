//
// Created by cgilab on 10/18/18.
//

#ifndef THREES_PUZZLE_AI_NTUPLENETWORK_H
#define THREES_PUZZLE_AI_NTUPLENETWORK_H

#import <vector>
#import <iostream>
#import "Board64.h"


class Tuple {
    virtual double GetValue(Board64 board) { return 0; }

    virtual board_t GetIndex(Board64 board, int col) { return 0; }

    virtual void updateScore(board_t b, double delta) {}

    virtual void save(fstream& out) {}

    virtual void load(fstream& in) {}
}

class AxeTuple : public Tuple {
public:
    AxeTuple() {
        memset(lut[0], 0, (0xffffff + 1) * sizeof(double));
        memset(lut[1], 0, (0xffffff + 1) * sizeof(double));
    }

    void updateScore(board_t b, double delta) {
        for (int i = 0; i < 2; i++) {
            board_t index = getIndex(board, i);
            lut[i][index] += delta;
        }
    }

    board_t GetIndex(Board64 board, int col) {
        board_t r1 = board.GetCol(col);
        board_t r2 = board.GetCol(col + 1);
        board_t index = (r1 & 0xffff) << 8 | (r2 & 0xff);

        return index;
    }

    double GetValue(Board64 board) {
        double total_value = 0.0;

        for (int i = 0; i < 2; i++) {
            board_t index = getIndex(board, i);
            total_value += lut[i][index];
        }

        return total_value;
    }

    void save(fstream& out) {
        out.write(reinterpret_cast<char*>(lut[0]), (0xffffff+1)*sizeof(double));
        out.write(reinterpret_cast<char*>(lut[1]), (0xffffff+1)*sizeof(double));
    }

    void load(fstream& in) {
        in.read(reinterpret_cast<char*>(lut[0]), (0xffffff+1) * sizeof(double));
        in.read(reinterpret_cast<char*>(lut[1]), (0xffffff+1) * sizeof(double));

    }


private:
    double lut[2][0xffffff + 1];
};

class RectangleTuple : public Tuple {
public:
    RectangleTuple() {
        memset(lut[0], 0, (0xffffff + 1) * sizeof(double));
        memset(lut[1], 0, (0xffffff + 1) * sizeof(double));
    }

    void updateScore(board_t b, double delta) {
        for (int i = 0; i < 2; i++) {
            board_t index = getIndex(board, i);
            lut[i][index] += delta;
        }
    }

    board_t GetIndex(Board64 board, int col) {
        board_t r1 = board.GetCol(col);
        board_t r2 = board.GetCol(col + 1);
        board_t index = (r1&0xfff0) << 8ull | r2 >> 4;

        return index;
    }

    double GetValue(Board64 board) {
        double total_value = 0.0;

        for (int i = 0; i < 2; i++) {
            board_t index = getIndex(board, i);
            total_value += lut[i][index];
        }

        return total_value;
    }

    void save(fstream& out) {
        out.write(reinterpret_cast<char*>(lut[0]), (0xffffff+1)*sizeof(double));
        out.write(reinterpret_cast<char*>(lut[1]), (0xffffff+1)*sizeof(double));
    }

    void load(fstream& in) {
        in.read(reinterpret_cast<char*>(lut[0]), (0xffffff+1) * sizeof(double));
        in.read(reinterpret_cast<char*>(lut[1]), (0xffffff+1) * sizeof(double));

    }


private:
    double lut[2][0xffffff + 1];
};

class LineTuple : public Tuple {

};

class NTupleNetwork {
public:
    NTupleNetwork();

    void AddTuple(Tuple tuple) {
        tuples.emplace_back(tuple);
    }

private:
    double GetValue() {
        double total_value = 0;
        for (int i = 0; i < tuples.size(); ++i) {
            total_value += tuples[i].GetValue();
        }
    }

    std::vector <Tuple> tuples;
};


#endif //THREES_PUZZLE_AI_NTUPLENETWORK_H
