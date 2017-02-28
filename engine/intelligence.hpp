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

class TransTable {
private:
    const size_t size;
    TScore* table;
public:
    static const TScore kEmpty = std::numeric_limits<TScore>::max();

    TransTable(size_t size);
    ~TransTable();

    void insert(uint64_t hash, TScore score);
    TScore lookup(uint64_t hash) const;
};


class Player {
public:
    TScore pickBestMove(const Board& b, TTeam team, Move* result);
};

struct ScoreFunction {
	TScore operator() (const Board& board);
};

class AIPlayer {
private:
	ScoreFunction scoreFunc;

    static constexpr double runTimeLimit = 2.0;
    int difficulty = 0;
    Move::TMoveScratchStack stack;
    TransTable tt;

    TScore negamax(Board& board, TTeam color, int depth, Move* result = nullptr,
                   clock_t maxTime = clock() + CLOCKS_PER_SEC * runTimeLimit,
                   TScore alpha = -std::numeric_limits<TScore>::max(),
                   TScore beta = std::numeric_limits<TScore>::max()
                   );

public:
    AIPlayer(int difficulty = 7) : tt(15485863), difficulty(difficulty) {};
    TScore pickBestMove(const Board& b, TTeam team, Move* result);
};

#endif /* intelligence_hpp */
