//
// Created by cgilab on 10/18/18.
//

#ifndef THREES_PUZZLE_AI_TDLEARNING_H
#define THREES_PUZZLE_AI_TDLEARNING_H

#include <iostream>
#include <cstdio>
#include <vector>
#include "NTupleNetwork.h"
#include "Common.h"

class TdLearning {
public:
    void learn() {

    }

    double evaluate() {

    }
private:
    const double LEARNING_RATE = 0.00025;
    NTupleNetwork n_tuple_network_;
};


#endif //THREES_PUZZLE_AI_TDLEARNING_H
