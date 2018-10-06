//
// Created by nhatminh2947 on 10/6/18.
//

#include <ctype.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>

#pragma once

#include <algorithm>

#include "Board64.h"

static row_t row_left_table[65536];
static row_t row_right_table[65536];
static board_t col_up_table[65536];
static board_t col_down_table[65536];
static float heur_score_table[65536];
static float score_table[65536];

// Heuristic scoring settings
static const float SCORE_LOST_PENALTY = 200000.0f;
static const float SCORE_MONOTONICITY_POWER = 4.0f;
static const float SCORE_MONOTONICITY_WEIGHT = 47.0f;
static const float SCORE_SUM_POWER = 3.5f;
static const float SCORE_SUM_WEIGHT = 11.0f;
static const float SCORE_MERGES_WEIGHT = 700.0f;
static const float SCORE_EMPTY_WEIGHT = 270.0f;

static board_t unpack_col(row_t row) {
    board_t tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

static row_t reverse_row(row_t row) {
    return (row >> 12) | ((row >> 4) & 0x00F0) | ((row << 4) & 0x0F00) | (row << 12);
}

void InitLookUpTables() {
    for (unsigned row = 0; row < 65536; ++row) {
        unsigned line[4] = {
                (row >> 0) & 0xf,
                (row >> 4) & 0xf,
                (row >> 8) & 0xf,
                (row >> 12) & 0xf
        };


        float score = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            if (rank >= 3) {
                score += powf(3, rank-2);
            }
        }
        score_table[row] = score;


        // Heuristic score
        float sum = 0;
        int empty = 0;
        int merges = 0;

        int prev = 0;
        int counter = 0;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            sum += pow(rank, SCORE_SUM_POWER);
            if (rank == 0) {
                empty++;
            } else {
                if (prev == rank) {
                    counter++;
                } else if (counter > 0) {
                    merges += 1 + counter;
                    counter = 0;
                }
                prev = rank;
            }
        }
        if (counter > 0) {
            merges += 1 + counter;
        }

        float monotonicity_left = 0;
        float monotonicity_right = 0;
        for (int i = 1; i < 4; ++i) {
            if (line[i - 1] > line[i]) {
                monotonicity_left +=
                        pow(line[i - 1], SCORE_MONOTONICITY_POWER) - pow(line[i], SCORE_MONOTONICITY_POWER);
            } else {
                monotonicity_right +=
                        pow(line[i], SCORE_MONOTONICITY_POWER) - pow(line[i - 1], SCORE_MONOTONICITY_POWER);
            }
        }

        heur_score_table[row] = SCORE_LOST_PENALTY +
                                SCORE_EMPTY_WEIGHT * empty +
                                SCORE_MERGES_WEIGHT * merges -
                                SCORE_MONOTONICITY_WEIGHT * std::min(monotonicity_left, monotonicity_right) -
                                SCORE_SUM_WEIGHT * sum;

        // execute a move to the left
        for (int c = 0; c < 3; c++) {
            if ((line[c] == 1 && line[c + 1] == 2) || (line[c] == 2 && line[c + 1] == 1)) {
                line[c] = 3;
                line[c + 1] = 0;
            } else if (line[c] == line[c + 1] && line[c] >= 3 && line[c + 1] >= 3) {
                line[c]++;
                line[c + 1] = 0;
            } else if (line[c] == 0) {
                line[c] = line[c + 1];
                line[c + 1] = 0;
            }
        }

        row_t result = (line[0] << 0) |
                       (line[1] << 4) |
                       (line[2] << 8) |
                       (line[3] << 12);
        row_t rev_result = reverse_row(result);
        unsigned rev_row = reverse_row(row);

        row_left_table[row] = row ^ result;
        row_right_table[rev_row] = rev_row ^ rev_result;
        col_up_table[row] = unpack_col(row) ^ unpack_col(result);
        col_down_table[rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
    }
}

reward_t GetScore(board_t board) {
    return score_table[(board >> 0) & ROW_MASK] +
           score_table[(board >> 16) & ROW_MASK] +
           score_table[(board >> 32) & ROW_MASK] +
           score_table[(board >> 48) & ROW_MASK];
}