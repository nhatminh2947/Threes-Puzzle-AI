//
// Created by nhatminh2947 on 10/6/18.
//
#pragma once

#ifndef THREES_CPP_COMMON_H
#include <cstdint>

typedef float reward_t;
typedef uint16_t row_t;
typedef uint64_t board_t;
typedef uint8_t cell_t;

static const board_t ROW_MASK = 0xFFFFULL;
static const board_t COL_MASK = 0x000F000F000F000FULL;
#define THREES_CPP_COMMON_H

#endif //THREES_CPP_COMMON_H