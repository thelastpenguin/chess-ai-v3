//
//  intelligence.cpp
//  engine
//
//  Created by Gareth George on 1/3/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//
#include <iostream>
#include <algorithm>
#include <cassert>
#include <stdint.h>
#include <stack>
#include <thread>

#include "include/fastrand.h"
#include "board.hpp"
#include "intelligence.hpp"

/** transposition table implementation */
const TScore TransTable::kEmpty;

TransTable::TransTable(size_t size) : size(size) {
    this->table = new TScore[size];
    std::fill(this->table, this->table + size, TransTable::kEmpty);
}

TransTable::~TransTable() {
    delete[] table;
}

inline void TransTable::insert(uint64_t hash, TScore score) {
    if (table[hash % size] == kEmpty || fast_rand() % 2 == 0) {
        table[size] = score;
    }
}

inline TScore TransTable::lookup(uint64_t hash) const {
    return table[hash % size];
}

/** negamax implementation */
TScore AIPlayer::negamax(Board& board, TTeam color, int depth, Move* result, clock_t maxTime, TScore alpha, TScore beta) {
    if (depth == 0) {
        return board.getScore() * color;
    }

    TScore max = -std::numeric_limits<TScore>::max();

    Board::MoveList moves;
    moves.reserve(120);
    board.generateMoves(moves, color);

    if (depth > 4) { // greater than 4 takes real time to process so we yield to give interrupts a chance.
        for (auto move : moves) {
            move.make(board, stack);
            move.score = board.getScore();
            move.zobristHash = board.getZobristHash();
            move.unmake(board, stack);
        }

        std::sort(moves.begin(), moves.end(), [=](const Move& moveA, const Move& moveB) {
            TScore a = tt.lookup(moveA.zobristHash);
            TScore b = tt.lookup(moveB.zobristHash);
            if (a == TransTable::kEmpty)
                a = moveA.score;
            if (b == TransTable::kEmpty)
                b = moveB.score;
            return a * color > b * color;
        });
    }

    if (depth >= 4) {
        if (maxTime < clock()) {
            if (result != nullptr)
                *result = Move();
            return 0;
        }
    }

    for (Move& move : moves) {
        move.make(board, stack);
        TScore score = -this->negamax(board, -color, depth - 1, nullptr, maxTime, -beta, -alpha);
        move.unmake(board, stack);

        if (score > max) {
            if (result)
                *result = move;
            max = score;
        }
        if (score > alpha)
            alpha = score;
        if (alpha >= beta)
            break ;
    }

    if (depth >= 4) {
        if (maxTime < clock()) {
            if (result != nullptr)
                *result = Move();
            return 0;
        }
    }

    return max;
}


TScore AIPlayer::pickBestMove(const Board &b, TTeam team, Move *result) {
    Board copy(b);

    const clock_t begin_time = clock();

    TScore score = 0;
    Move::TMoveScratchStack stack;
    std::cout << "Begin search." << std::endl;
    int i = 3;
    while (true) {
        std::cout << "\tDepth: " << i << std::endl;
        Move curResult;
        TScore curScore = this->negamax(copy, team, i, &curResult, begin_time + CLOCKS_PER_SEC * 8);
        if (curResult.type != Move::Type::INVALID) {
            *result = curResult;
            score = curScore;
        } else {
            std::cout << "Exit search at depth " << i << std::endl;
            break ;
        }
        i++;
    }
    std::cout << "End search. Took " << (float(clock() - begin_time)) / CLOCKS_PER_SEC << " seconds." << std::endl;

    return score;
}
