//
// Created by nhatminh2947 on 10/6/18.
//


#pragma once

#include <ctype.h>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>

#include "Board64.h"

static cell_t row_max_table[65536];
static row_t row_left_table[65536];
static row_t row_right_table[65536];
static board_t col_up_table[65536];
static board_t col_down_table[65536];
static float heur_score_table[65536];
static float score_table[65536];

// Heuristic scoring settings
static float SCORE_LOST_PENALTY = 200000.0f;
static float SCORE_MONOTONICITY_POWER = 4.0f;
static float SCORE_MONOTONICITY_WEIGHT = 47.0f;
static float SCORE_SUM_POWER = 3.5f;
static float SCORE_SUM_WEIGHT = 11.0f;
static float SCORE_MERGES_WEIGHT = 700.0f;
static float SCORE_12_MERGES_WEIGHT = 0.0f;
static float SCORE_EMPTY_WEIGHT = 270.0f;

static board_t unpack_col(row_t row) {
    board_t tmp = row;
    return (tmp | (tmp << 12ULL) | (tmp << 24ULL) | (tmp << 36ULL)) & COL_MASK;
}

static row_t reverse_row(row_t row) {
    return (row >> 12) | ((row >> 4) & 0x00F0) | ((row << 4) & 0x0F00) | (row << 12);
}

void InitLookUpTables() {
    for(unsigned row = 0; row < 65536; ++row) {
        unsigned line[4] = {
                (row >>  0) & 0xf,
                (row >>  4) & 0xf,
                (row >>  8) & 0xf,
                (row >> 12) & 0xf
        };

        // Score
        float score = 0.0f;
        for (int i = 0; i < 4; ++i) {
            int rank = line[i];
            if (rank >= 3) {
                score += powf(3, rank-2);
            }
        }
        score_table[row] = score;
        row_max_table[row] = std::max(std::max(line[0], line[1]), std::max(line[2], line[3]));


        // Heuristic score
        float sum = 0;
        int empty = 0;
        int merges = 0;
        int onetwo_merges = 0;

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
        for (int i = 1; i < 4; ++i) {
            if ((line[i-1] == 1 && line[i] == 2) || (line[i-1] == 2 && line[i] == 1)) {
                onetwo_merges++;
            }
        }

        float monotonicity_left = 0;
        float monotonicity_right = 0;
        for (int i = 1; i < 4; ++i) {
            if (line[i-1] > line[i]) {
                monotonicity_left += pow(line[i-1], SCORE_MONOTONICITY_POWER) - pow(line[i], SCORE_MONOTONICITY_POWER);
            } else {
                monotonicity_right += pow(line[i], SCORE_MONOTONICITY_POWER) - pow(line[i-1], SCORE_MONOTONICITY_POWER);
            }
        }

        heur_score_table[row] = SCORE_LOST_PENALTY
                                + SCORE_EMPTY_WEIGHT * empty
                                + SCORE_MERGES_WEIGHT * merges
                                + SCORE_12_MERGES_WEIGHT * onetwo_merges
                                - SCORE_MONOTONICITY_WEIGHT * std::min(monotonicity_left, monotonicity_right)
                                - SCORE_SUM_WEIGHT * sum;

        // execute a move to the left
        int i;

        for(i=0; i<3; i++) {
            if(line[i] == 0) {
                line[i] = line[i+1];
                break;
            } else if(line[i] == 1 && line[i+1] == 2) {
                line[i] = 3;
                break;
            } else if(line[i] == 2 && line[i+1] == 1) {
                line[i] = 3;
                break;
            } else if(line[i] == line[i+1] && line[i] >= 3) {
                if(line[i] != 15) {
                    /* Pretend that 12288 + 12288 = 12288 */
                    line[i]++;
                }
                break;
            }
        }

        if(i == 3)
            continue;

        /* fold to the left */
        for(int j=i+1; j<3; j++)
            line[j] = line[j+1];
        line[3] = 0;

        row_t result = (line[0] <<  0) |
                       (line[1] <<  4) |
                       (line[2] <<  8) |
                       (line[3] << 12);
        row_t rev_result = reverse_row(result);
        unsigned rev_row = reverse_row(row);

        row_left_table [    row] =                row  ^                result;
        row_right_table[rev_row] =            rev_row  ^            rev_result;
        col_up_table   [    row] = unpack_col(    row) ^ unpack_col(    result);
        col_down_table [rev_row] = unpack_col(rev_row) ^ unpack_col(rev_result);
    }
}