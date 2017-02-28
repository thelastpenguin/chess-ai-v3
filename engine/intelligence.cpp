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
		return scoreFunc(board) * color;
        // return board.getScore() * color;
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
        TScore curScore = this->negamax(copy, team, i, &curResult, begin_time + CLOCKS_PER_SEC * difficulty);
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

TScore ScoreFunction::operator() (const Board& board) {
	// determine the piece counts
	const int pc_offset = 8;
	int piece_counts[pc_offset * 2] = {0}; // sufficiently large to hold all piece values.
	for (int i = 0; i < BOARD_SIZE; ++i) {
		TPiece piece = board[mailbox64[i]];
		piece_counts[piece + 8]++;
	}

	// game phase computation
	const int PawnPhase = 0;
	const int KnightPhase = 1;
	const int BishopPhase = 1;
	const int RookPhase = 2;
	const int QueenPhase = 4;
	const int TotalPhase = PawnPhase*16 + KnightPhase*4 + BishopPhase*4 + RookPhase*4 + QueenPhase*2;

	int phase = TotalPhase;
	phase -= (piece_counts[pc_offset + PIECE_PAWN] + piece_counts[pc_offset - PIECE_PAWN]) * PawnPhase;
	phase -= (piece_counts[pc_offset + PIECE_KNIGHT] + piece_counts[pc_offset - PIECE_KNIGHT]) * KnightPhase;
	phase -= (piece_counts[pc_offset + PIECE_BISHOP] + piece_counts[pc_offset - PIECE_BISHOP]) * BishopPhase;
	phase -= (piece_counts[pc_offset + PIECE_ROOK] + piece_counts[pc_offset - PIECE_ROOK]) * RookPhase;
	phase -= (piece_counts[pc_offset + PIECE_QUEEN] + piece_counts[pc_offset - PIECE_QUEEN]) * QueenPhase;
	phase = (phase * 256 + (TotalPhase / 2)) / TotalPhase;

	double materialScore = board.getScore();


	int openness = 0;
	std::vector<Move> movesWhite;
	movesWhite.reserve(100);
	std::vector<Move> movesBlack;
	movesBlack.reserve(100);
	board.generateMoves(movesWhite, 1);
	board.generateMoves(movesBlack, -1);
	openness += (movesWhite.size() - movesBlack.size()) * 10; // TODO: modify openness bias.

	if (phase > 160) {
		// late game board evaluation

		TScore score = 0;
		int whiteKingPos = -1;
		int blackKingPos = -1;
		for (int i = 0; i < BOARD_SIZE; ++i) {
			TPiece piece = board[mailbox64[i]];
			switch (piece) {
			case PIECE_KING:
				whiteKingPos = i;
				break ;
			case -PIECE_KING:
				blackKingPos = i;
				break ;
			default:
				break ;
			}
		}

		{
			// KRK end game
			double whiteKingX = whiteKingPos % BOARD_DIM;
			double whiteKingY = whiteKingPos / BOARD_DIM;
			double blackKingX = blackKingPos % BOARD_DIM;
			double blackKingY = blackKingPos / BOARD_DIM;

			double cmd;
			double md = std::abs(whiteKingX - blackKingX) + std::abs(whiteKingY - blackKingY);
			if (materialScore < 0) {
				cmd = std::abs(BOARD_DIM / 2.0 - blackKingX);
				cmd += std::abs(BOARD_DIM / 2.0 - blackKingY);
			} else {
				cmd = std::abs(BOARD_DIM / 2 - whiteKingX);
				cmd += std::abs(BOARD_DIM / 2 - whiteKingY);
			}

			TScore mopupValue = 4.7 * cmd + 16 * (BOARD_DIM - 2 - md);
			score += (materialScore < 0 ? -mopupValue : mopupValue) * 50;
		}

		return score;
	} else {
		// pawn structure buff
		// TODO: determine if helping the AI understand pawn struCTUre is actually beneficial
		//       I think it helps break up opponent's pawn structure though it does not seem to help
		//       the AI form it's own.
		TScore score = 0;
		for (int i = 0; i < BOARD_SIZE; ++i) {
			const int pos = mailbox64[i];
			TPiece piece = board[pos];
			switch (piece) {
			case PIECE_PAWN:
				if (board[pos - MAILBOX_W + 1] == PIECE_PAWN)
					score += 5; // half a centipawn
				else if (board[pos - MAILBOX_W - 1] == PIECE_PAWN)
					score += 5; // half a centipawn
				break ;
			case -PIECE_PAWN:
				if (board[pos + MAILBOX_W + 1] == -PIECE_PAWN)
					score -= 5; // half a centipawn
				else if (board[pos + MAILBOX_W - 1] == -PIECE_PAWN)
					score -= 5; // half a centipawn
				break ;
			default:
				continue ;
			}
		}
		return score + materialScore;
	}
}
