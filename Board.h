//#pragma once
//
//#include <array>
//#include <iostream>
//#include <iomanip>
//#include <cmath>
//#include <vector>
//#include <cstdlib>
//
///**
// * array-based board for Threes
// *
// * index (1-d form):
// *  (0)  (1)  (2)  (3)
// *  (4)  (5)  (6)  (7)
// *  (8)  (9) (10) (11)
// * (12) (13) (14) (15)
// *
// */
//class Board64 {
//public:
//    typedef uint16_t cell_t;
//    typedef int reward_t;
//    typedef std::array<cell_t, 4> row_t;
//    typedef std::array<row_t, 4> board_t;
//    typedef uint64_t data_t;
//
//    Board64() : board_(), data_(0) {}
//
//    Board64(const board_t &b, data_t v = 0) : board_(b), data_(v) {}
//
//    Board64(const Board64 &board) = default;
//
//    Board64 &operator=(const Board64 &b) = default;
//
//    explicit operator board_t &() { return board_; }
//
//    explicit operator const board_t &() const { return board_; }
//
//    row_t &operator[](unsigned i) { return board_[i]; }
//
//    const row_t &operator[](unsigned i) const { return board_[i]; }
//
//    cell_t &operator()(unsigned i) { return board_[i / 4][i % 4]; }
//
//    const cell_t &operator()(unsigned i) const { return board_[i / 4][i % 4]; }
//
//    data_t info() const { return data_; }
//
//    data_t info(data_t data) {
//        data_t old = data_;
//        data_ = data;
//        return old;
//    }
//
//    bool operator==(const Board64 &b) const { return board_ == b.board_; }
//
//    bool operator<(const Board64 &b) const { return board_ < b.board_; }
//
//    bool operator!=(const Board64 &b) const { return !(*this == b); }
//
//    bool operator>(const Board64 &b) const { return b < *this; }
//
//    bool operator<=(const Board64 &b) const { return !(b < *this); }
//
//    bool operator>=(const Board64 &b) const { return !(*this < b); }
//
//    friend std::ostream &operator<<(std::ostream &out, const Board64 &board) {
//        out << "+------------------------+" << std::endl;
//        for (auto &row : board.board_) {
//            out << "|" << std::dec;
//            for (auto tile : row) {
//                int value = tile >= 3 ? ((1 << (tile-3)) * 3) : tile;
//                out << std::setw(6) << value;
//            }
//            out << "|" << std::endl;
//        }
//        out << "+------------------------+" << std::endl;
//        return out;
//    }
//
//    /**
//     * place a tile (index value) to the specific position (1-d form index)
//     * return 0 if the action is valid, or -1 if not
//     */
//    reward_t Place(unsigned int position, cell_t tile) {
//        if (position >= 16) return -1;
//        if (tile != 1 && tile != 2 && tile != 3) return -1;
//
//        this->operator()(position) = tile;
//
//        return 0;
//    }
//
//    /**
//     * apply an action to the board
//     * return the reward of the action, or -1 if the action is illegal
//     */
//    reward_t Slide(unsigned int direction) {
//        switch (direction & 0b11) {
//            case 0:
//                return SlideUp();
//            case 1:
//                return SlideRight();
//            case 2:
//                return SlideDown();
//            case 3:
//                return SlideLeft();
//            default:
//                return -1;
//        }
//    }
//
//    reward_t SlideLeft() {
//        Board64 prev_board = *this;
//        reward_t score = 0;
//
//        for (int r = 0; r < 4; r++) {
//            auto &row = this->board_[r];
//
//            for (int c = 0; c < 3; c++) {
//                if ((row[c] == 1 && row[c+1] == 2) || (row[c] == 2 && row[c+1] == 1)) {
//                    row[c] = 3;
//                    row[c+1] = 0;
//                }
//                else if (row[c] == row[c+1] && row[c] >= 3 && row[c+1] >= 3) {
//                    row[c]++;
//                    row[c+1] = 0;
//                }
//                else if (row[c] == 0) {
//                    row[c] = row[c+1];
//                    row[c+1] = 0;
//                }
//            }
//
//            for (int c = 0; c < 4; c++) {
//                if(row[c] >= 3) {
//                    score += reward_t(pow(3, row[c] - 2));
//                }
//            }
//        }
//
//        return (*this != prev_board) ? score : -1;
//    }
//
//    reward_t SlideRight() {
//        ReflectHorizontal();
//        reward_t score = SlideLeft();
//        ReflectHorizontal();
//        return score;
//    }
//
//    reward_t SlideUp() {
//        RotateRight();
//        reward_t score = SlideRight();
//        RotateLeft();
//        return score;
//    }
//
//    reward_t SlideDown() {
//        RotateRight();
//        reward_t score = SlideLeft();
//        RotateLeft();
//        return score;
//    }
//
//    void Transpose() {
//        for (int r = 0; r < 4; r++) {
//            for (int c = r + 1; c < 4; c++) {
//                std::swap(board_[r][c], board_[c][r]);
//            }
//        }
//    }
//
//    void ReflectHorizontal() {
//        for (int r = 0; r < 4; r++) {
//            std::swap(board_[r][0], board_[r][3]);
//            std::swap(board_[r][1], board_[r][2]);
//        }
//    }
//
//    void ReflectVertical() {
//        for (int c = 0; c < 4; c++) {
//            std::swap(board_[0][c], board_[3][c]);
//            std::swap(board_[1][c], board_[2][c]);
//        }
//    }
//
//    /**
//     * rotate the board clockwise by given times
//     */
//    void Rotate(int r = 1) {
//        switch (((r % 4) + 4) % 4) {
//            default:
//            case 0:
//                break;
//            case 1:
//                RotateRight();
//                break;
//            case 2:
//                reverse();
//                break;
//            case 3:
//                RotateLeft();
//                break;
//        }
//    }
//
//    void RotateRight() {
//        Transpose();
//        ReflectHorizontal();
//    } // clockwise
//
//    void RotateLeft() {
//        Transpose();
//        ReflectVertical();
//    } // counterclockwise
//
//    void reverse() {
//        ReflectHorizontal();
//        ReflectVertical();
//    }
//
//private:
//    board_t board_;
//    data_t data_;
//};
