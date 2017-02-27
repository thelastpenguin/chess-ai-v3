//
//  tests.cpp
//  engine
//
//  Created by Gareth George on 1/3/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

// https://chessprogramming.wikispaces.com/Engine+Testing for ideas for more tests

#include <iostream>
#include <stack>

#include "tests.hpp"
#include "include/termcolor.h"

#include "board.hpp"

/** define testing suite */
#define check(EX) (void)(_check(EX, #EX, __FILE__, __LINE__))

int passed = 0;
int failed = 0;

void _check(bool value, const char* expr, const char* file, int line) {
    if (value) {
        passed++;
    } else {
        std::cout << termcolor::red;
        printf("\tCheck %s FAILED at %s line %d", expr, file, line);
        std::cout << termcolor::red << std::endl;
        failed++;
    }
}

/** helper functions */
int randomPiece() {
    int i = rand() % 6 + 1;
    return rand() % 2 == 0 ? -i : i;
}

/** define tests */

// checks the board setup is correct
void test_checkBoardSetup() {
    Board board;
    
    check(board.getScore() == 0);
    
    board.setupBoard();
    
    check(board.getScore() == 0);
}

// checks that we can copy a board object
void test_copyBoard() {
    Board b1;
    b1.setupBoard();
    Board b2(b1);
    
    check(b1.getZobristHash() == b2.getZobristHash());
    check(b1.getScore() == b2.getScore());
}

// checks that moving and unmoving is working properly and dosen't corrupt the hash
void helper_makeAndUnmake(Board& b, Move::TMoveScratchStack& stack, int depth = 2) {
    if (depth <= 0) return ;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        if (b[mailbox64[i]] != 0) {
            for (int j = 0; j < BOARD_SIZE; ++j) {
                Move m(Move::Type::LOUD, mailbox64[i], mailbox64[j]);
                m.make(b, stack);
                helper_makeAndUnmake(b, stack, depth - 1);
                m.unmake(b, stack);
            }
        }
    }
}

void test_makeAndUnmakeMove() {
    Board b;
    b.setupBoard();
    
    Move::TMoveScratchStack stack;
    uint64_t hash = b.getZobristHash();
    helper_makeAndUnmake(b, stack);
    check(hash == b.getZobristHash());
    check(b.getScore() == 0);
}

// https://chessprogramming.wikispaces.com/Perft+Results Perft tests for move generation
int moveNodesAtDepth[] = {1, 20, 400, 8902, 197281, 4865609};
int kiwipeteMovesAtDepth[] = {1, 48, 2039, 97862};


int helper_countMoves(Board& board, Move::TMoveScratchStack& stack, int team, int depth) {
    if (depth == 0) return 1;
    int count = 0;
    Board::MoveList moves;
    board.generateMoves(moves, team);
    for (auto move : moves) {
        move.make(board, stack);
        count += helper_countMoves(board, stack, -team, depth - 1);
        move.unmake(board, stack);
    }
    return count;
}

void test_perft() {
    std::cout << "Perft testing move counts." << std::endl;
    Board b;
    Move::TMoveScratchStack stack;
    
    b.setupBoard();
    std::cout << "\tdefault setup." << std::endl;
    for (int i = 0; i < 4; ++i) { // NOTE: there are failures at depth 5. This is likely due to the
                                  // en passant implementation being pending
                                  // as well as castling etc.
        int moves = helper_countMoves(b, stack, 1, i);
        std::cout << "Depth: " << i << " Moves: " << moves << std::endl;
        check(moveNodesAtDepth[i] == moves);
    }

// TODO: figure out why this fails to parse
//    std::cout << "\tkiwipete." << std::endl;
//    b.loadBoardFromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R");
//    for (int i = 0; i < 4; ++i) { // NOTE: there are failures at depth 5. This is likely due to the
//        // en passant implementation beingHm pending
//        // as well as castling etc.
//        int moves = helper_countMoves(b, stack, 1, i);
//        std::cout << "Depth: " << i << " Moves: " << moves << std::endl;
//        check(kiwipeteMovesAtDepth[i] == moves);
//    }
}


void runTests() {
    Board b;
    test_checkBoardSetup();
    test_copyBoard();
    test_makeAndUnmakeMove();
    test_perft();
    
    std::cout << passed << " assertions passed." << std::endl;
    std::cout << failed << " assertions failed." << std::endl;
}
