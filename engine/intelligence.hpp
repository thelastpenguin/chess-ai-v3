//
//  intelligence.hpp
//  engine
//
//  Created by Gareth George on 1/3/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

#ifndef intelligence_hpp
#define intelligence_hpp

#include "constants.hpp"
#include "board.hpp"

struct TTEntry {
    static const TScore kEmpty = std::numeric_limits<TScore>::max();

    uint64_t hash = 0;
    int depth = 0;
    TScore score = kEmpty;

    bool isEmpty() {
        return score == kEmpty;
    }
};

class TransTable {
private:
    const size_t size;

    TTEntry* table;
public:
    TransTable(size_t size);
    ~TransTable();

    void insert(uint64_t hash, int depth, TScore score);
    TTEntry* lookup(uint64_t hash, int depth) const;
};


class Player {
public:
    TScore pickBestMove(const Board& b, TTeam team, Move* result);
};

struct ScoreFunction {

	double opennessMult = 7.0;
	double materialMult = 1.0;
	double duplicatePieceMult = 1.0;
	double pawnProtectionMult = 15.0;

	TScore operator() (const Board& board);
};

class AIPlayer {
private:
	ScoreFunction scoreFunc;

    static constexpr double runTimeLimit = 2.0;
    int difficulty = 0;
    Move::TMoveScratchStack stack;
    TransTable ttWhite;
    TransTable ttBlack;

    TScore negamax(Board& board, TTeam color, int depth, Move* result = nullptr,
                   clock_t maxTime = clock() + CLOCKS_PER_SEC * runTimeLimit,
                   TScore alpha = -std::numeric_limits<TScore>::max(),
                   TScore beta = std::numeric_limits<TScore>::max()
                   );

public:
    AIPlayer(int difficulty = 7) : ttWhite(15485863), ttBlack(15485863), difficulty(difficulty) {};
    TScore pickBestMove(const Board& b, TTeam team, Move* result);
};

#endif /* intelligence_hpp */
