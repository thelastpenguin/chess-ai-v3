//
//  board.hpp
//  engine
//
//  Created by Gareth George on 1/1/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

#ifndef board_hpp
#define board_hpp

//#define DEBUG_MOVE
//#define DEBUG_BOARD
//#define DEBUG_MOVE_GENERATION

#include <iostream>
#include <cassert>
#include <stack>
#include <stdint.h>
#include <string>
#include <vector>

#include "constants.hpp"

extern const int mailbox[120];
extern const int mailbox64[64];
extern int mirror64[64];

extern uint64_t pieceHashTable[64 * 16];
extern uint64_t flagHashTable[256];

extern char pieceGetLetter(TPiece piece);

struct Move;

class Board {
private:
    int64_t hash = 0;
    TScore score = 0; // int32_t
    TBoardFlags flags = 0; // uint8_t
    TPiece pieces[120]; // int8_t[120]
    int8_t enPassentSquare; // the en passent square... silly.

	// TODO: add a state history. Prevent searching nodes that result in state repeats. Rippp.
public:
    Board();
    Board(const Board& board);

    void setupBoard();
    void loadBoardFromFEN(const char* FEN);

    std::string toString() const;

    inline TScore getScore() const {
        return score;
    }

    inline int64_t getZobristHash() const {
        return hash;
    }

    void setPiece(int position, int8_t value);

    inline int8_t operator[] (int index) const {
        return pieces[index];
    };

    inline TScore getPieceScore(int position) const;

    inline uint8_t getFlags() { return flags; };
    inline void setFlags(uint8_t flags) {
        hash ^= flagHashTable[this->flags];
        this->flags = flags;
        hash ^= flagHashTable[this->flags];
    }

    typedef std::vector<Move> MoveList;
    void generateMoves(MoveList& moves, TTeam player) const;
};

/**
 internal representation of a move
 */
struct Move {

    enum class Type : uint8_t {
        QUIET, // a move to an empty square
        LOUD, // a move involving a capture
        PAWN_PROMOTE, // used by pawn promotions
        CHANGE_FLAG, // used to change a flag. Indicates a king moving for the first time etc.
        MOVE_EN_PASSENT,
        INVALID
    };

    typedef std::stack<int32_t> TMoveScratchStack;

    Type type;
    uint8_t from;
    uint8_t to;
    int8_t r1;

    // special storage for sort ordering etc.
    TScore score = kScoreNotYetDetermined;
    uint64_t zobristHash = 0;

    Move() : type(Type::INVALID) {

    };

    Move(Type type, uint8_t from, uint8_t to) : type(type), from(from), to(to) {
#ifdef DEBUG_MOVE
        assert(mailbox[from] != -1);
        assert(mailbox[to] != -1);
#endif
    };

    Move(Type type, uint8_t from, uint8_t to, int8_t data) : type(type), from(from), to(to), r1(data) { };

    template<class Stack>
    void make(Board& board, Stack& stack) const {
        switch (type) {
            case Type::QUIET:
                board.setPiece(to, board[from]);
                board.setPiece(from, 0);
                break ;
            case Type::LOUD:
                stack.push(board[from]);
                stack.push(board[to]);
                board.setPiece(to, board[from]);
                board.setPiece(from, 0);
                break ;
            case Type::PAWN_PROMOTE:
                stack.push(board[from]);
                stack.push(board[to]);
                board.setPiece(to, r1);
                board.setPiece(from, 0);
				break ;
            case Type::CHANGE_FLAG:
                stack.push(board[from]);
                stack.push(board[to]);
                stack.push(board.getFlags());

                board.setPiece(to, board[from]);
                board.setPiece(from, 0);
                board.setFlags((uint8_t)r1);
				break ;
#ifdef DEBUG_MOVE
            default:
                assert(0);
#endif
        }
    }

    template<class Stack>
    void unmake(Board& board, Stack& stack) const {
        switch (type) {
            case Type::QUIET:
                board.setPiece(from, board[to]);
                board.setPiece(to, 0);
                break ;
            case Type::LOUD:
            case Type::PAWN_PROMOTE:
                board.setPiece(to, stack.top());
                stack.pop();
                board.setPiece(from, stack.top());
                stack.pop();
                break ;
            case Type::CHANGE_FLAG:
                board.setFlags((uint8_t)stack.top());
                stack.pop();
                board.setPiece(to, stack.top());
                stack.pop();
                board.setPiece(from, stack.top());
                stack.pop();
                break ;
#ifdef DEBUG_MOVE
            default:
                assert(0);
#endif
        }
    }
};

#endif /* board_hpp */
