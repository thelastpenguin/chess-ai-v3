//
//  board.cpp
//  engine
//
//  Created by Gareth George on 1/1/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

#include <sstream>
#include <iostream>
#include <random>

#include "board.hpp"

const int mailbox[120] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 0
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 1
    -1,  0,  1,  2,  3,  4,  5,  6,  7, -1, // 2
    -1,  8,  9, 10, 11, 12, 13, 14, 15, -1, // 3
    -1, 16, 17, 18, 19, 20, 21, 22, 23, -1, // 4
    -1, 24, 25, 26, 27, 28, 29, 30, 31, -1, // 5
    -1, 32, 33, 34, 35, 36, 37, 38, 39, -1, // 6
    -1, 40, 41, 42, 43, 44, 45, 46, 47, -1, // 7
    -1, 48, 49, 50, 51, 52, 53, 54, 55, -1, // 8
    -1, 56, 57, 58, 59, 60, 61, 62, 63, -1, // 9
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, // 10
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  // 11
};

const int mailbox64[64] = {
    21, 22, 23, 24, 25, 26, 27, 28,
    31, 32, 33, 34, 35, 36, 37, 38,
    41, 42, 43, 44, 45, 46, 47, 48,
    51, 52, 53, 54, 55, 56, 57, 58,
    61, 62, 63, 64, 65, 66, 67, 68,
    71, 72, 73, 74, 75, 76, 77, 78,
    81, 82, 83, 84, 85, 86, 87, 88,
    91, 92, 93, 94, 95, 96, 97, 98
};

int mirror64[64];

uint64_t pieceHashTable[64 * 16];
uint64_t flagHashTable[256];

struct __PopulateTables {
    __PopulateTables() {
        /* populate the hash tables */
        std::mt19937_64 e2(5489u); // known seed for consistent hashing
        std::uniform_int_distribution<long long int> dist(std::llround(std::pow(2,61)), std::llround(std::pow(2,62)));

        // initialize piece hashes
        for (int i = 0; i < 64 * 16; ++i)
            pieceHashTable[i] = dist(e2);

        // initialize flag hashes
        for (int i = 0; i < 256; ++i)
            flagHashTable[i] = dist(e2);

        /* populate the mirror tables */
        for (int i = 0; i < 64; ++i) {
            mirror64[i] = (7 - i / 8) * 8 + i % 8;
        }
    }
};

__PopulateTables __populateTables;



Board::Board() {
    hash = flagHashTable[flags];
    for (int i = 0; i < MAILBOX_SIZE; ++i) {
        if (mailbox[i] < 0)
            pieces[i] = OUT_OF_BOUNDS;
        else
            pieces[i] = 0;
    }
}

Board::Board(const Board& board) : Board() {
    flags = board.flags;
    hash = flagHashTable[flags];

    for (int i = 0; i < BOARD_SIZE; ++i) {
        const int position = mailbox64[i];
        setPiece(position, board[position]);
    }

#ifdef DEBUG_BOARD
    assert(getZobristHash() == board.getZobristHash());
    assert(getScore() == board.getScore());
#endif
}

void Board::setupBoard() {
    // fen code for the initial board layout
    loadBoardFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
}

void Board::loadBoardFromFEN(const char* FEN) {
    int row = 7;
    int col = 0;
    while (row >= 0) {
        if (*FEN == ' ' || *FEN == 0) break;
        if (*FEN >= '0' && *FEN <= '9') {
            col += *FEN + 1;
        } else if (*FEN == 'p') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_PAWN);
            col++;
        } else if (*FEN == 'n') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_KNIGHT);
            col++;
        } else if (*FEN == 'b') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_BISHOP);
            col++;
        } else if (*FEN == 'r') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_ROOK);
            col++;
        } else if (*FEN == 'q') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_QUEEN);
            col++;
        } else if (*FEN == 'k') {
            setPiece(mailbox64[row * BOARD_DIM + col], -PIECE_KING);
            col++;
        } else if (*FEN == 'P') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_PAWN);
            col++;
        } else if (*FEN == 'N') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_KNIGHT);
            col++;
        } else if (*FEN == 'B') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_BISHOP);
            col++;
        } else if (*FEN == 'R') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_ROOK);
            col++;
        } else if (*FEN == 'Q') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_QUEEN);
            col++;
        } else if (*FEN == 'K') {
            setPiece(mailbox64[row * BOARD_DIM + col], PIECE_KING);
            col++;
        } else if (*FEN == '/') {
            row--;
            col = 0;
        }
        FEN++;
    }
}

char pieceGetLetter(TPiece piece) {
    switch (abs(piece)) {
        case PIECE_PAWN: return 'P';
        case PIECE_KNIGHT: return 'N';
        case PIECE_BISHOP: return 'B';
        case PIECE_ROOK: return 'R';
        case PIECE_QUEEN: return 'Q';
        case PIECE_KING: return 'K';
        case 0: return ' ';
        default: return '?';
    }
}

std::string Board::toString() const {
    std::stringstream ss;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        TPiece piece = (*this)[mailbox64[i]];
        if (piece == 0) {
            ss << "   ";
        } else {
            ss << (piece < 0 ? "b" : "w") << pieceGetLetter(piece) << " ";
        }
        if (i % 8 == 7)
            ss << "\n";
    }
    return ss.str();
}

/*
 Move generation
 */

struct CheckMoveLoud {
    static inline bool can(const Board& board, int from, int to) {
        const TPiece fromPiece = board[from];
        const TPiece toPiece = board[to];
        return toPiece != OUT_OF_BOUNDS && toPiece != 0 && (fromPiece < 0) != (toPiece < 0);
    }
};

struct CheckMoveQuiet {
    static inline bool can(const Board& board, int from, int to) {
        const TPiece toPiece = board[to];
        return toPiece == 0;
    }
};

struct CheckMoveGeneric {
    static inline bool can(const Board& board, int from, int to) {
        const TPiece fromPiece = board[from];
        const TPiece toPiece = board[to];
        return toPiece != OUT_OF_BOUNDS && (toPiece == 0 || (fromPiece < 0) != (toPiece < 0));
     }
};

struct AddMoveLoud {
    static inline void addMove(const Board& board, int from, int to, Board::MoveList& moves) {
        moves.push_back(Move(Move::Type::LOUD, from, to));
    }
};

struct AddMoveQuiet {
    static inline void addMove(const Board& board, int from, int to, Board::MoveList& moves) {
        moves.push_back(Move(Move::Type::QUIET, from, to));
    }
};

struct AddMovePawnPromote {
    static inline void addMove(const Board& board, int from, int to, Board::MoveList& moves) {
        const TPiece pieceFrom = board[from];
        moves.push_back(Move(Move::Type::PAWN_PROMOTE, from, to, pieceFrom < 0 ? -PIECE_QUEEN : PIECE_QUEEN));
        moves.push_back(Move(Move::Type::PAWN_PROMOTE, from, to, pieceFrom < 0 ? -PIECE_KNIGHT : PIECE_KNIGHT));
    }
};

template<class Check, class Add>
inline bool addMove(const Board& board, int from, int to, Board::MoveList& moves) {
    if (Check::can(board, from, to)) {
        Add::addMove(board, from, to, moves);
        return true;
    }
    return false;
}

template<class Check, class Add, class CheckAfter, class AddAfter>
inline void addSlide(const Board& board, const int from, const int offset, Board::MoveList& moves) {
    int position = from + offset;
    while (addMove<Check, Add>(board, from, position, moves)) {
        position += offset;
    }
    addMove<CheckAfter, AddAfter>(board, from, position, moves);
}


void Board::generateMoves(MoveList& moves, TTeam player) const {
    for (int i = 0; i < BOARD_SIZE; ++i) {
        const int pos = mailbox64[i];
        TPiece piece = pieces[pos];
        if (piece == 0 || (piece < 0) != (player < 0))
            continue ;

        switch (abs(piece)) {
            case PIECE_PAWN:
                if (piece > 0) {
					if (i / BOARD_DIM == 6) {
						// pawn promotion
						addMove<CheckMoveLoud, AddMovePawnPromote>(*this, pos, pos + MAILBOX_W + 1, moves);
						addMove<CheckMoveLoud, AddMovePawnPromote>(*this, pos, pos + MAILBOX_W - 1, moves);
						addMove<CheckMoveQuiet, AddMovePawnPromote>(*this, pos, pos + MAILBOX_W, moves);
					} else {
						addMove<CheckMoveLoud, AddMoveLoud>(*this, pos, pos + MAILBOX_W + 1, moves);
	                    addMove<CheckMoveLoud, AddMoveLoud>(*this, pos, pos + MAILBOX_W - 1, moves);
						if (addMove<CheckMoveQuiet, AddMoveQuiet>(*this, pos, pos + MAILBOX_W, moves) && i / BOARD_DIM == 1) {
	                        addMove<CheckMoveQuiet, AddMoveQuiet>(*this, pos, pos + MAILBOX_W * 2, moves);
	                    }
					}
                } else {
					if (i / BOARD_DIM == 1) {
						// pawn promotion
						addMove<CheckMoveLoud, AddMovePawnPromote>(*this, pos, pos - MAILBOX_W + 1, moves);
						addMove<CheckMoveLoud, AddMovePawnPromote>(*this, pos, pos - MAILBOX_W - 1, moves);
						addMove<CheckMoveQuiet, AddMovePawnPromote>(*this, pos, pos - MAILBOX_W, moves);
					} else {
						addMove<CheckMoveLoud, AddMoveLoud>(*this, pos, pos - MAILBOX_W + 1, moves);
	                    addMove<CheckMoveLoud, AddMoveLoud>(*this, pos, pos - MAILBOX_W - 1, moves);
						if (addMove<CheckMoveQuiet, AddMoveQuiet>(*this, pos, pos - MAILBOX_W, moves) && i / BOARD_DIM == 6) {
	                        addMove<CheckMoveQuiet, AddMoveQuiet>(*this, pos, pos - MAILBOX_W * 2, moves);
	                    }
					}
                }

				// TODO: add pawn promotion

                break ;
            case PIECE_KNIGHT:
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W + 2, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W - 2, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W + 2, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W - 2, moves);

                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W * 2 + 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W * 2 - 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W * 2 + 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W * 2 - 1, moves);
                break ;
            case PIECE_BISHOP:

                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W + 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W - 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W + 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W - 1, moves);

                break ;
            case PIECE_ROOK:

                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W, moves);

                break ;
            case PIECE_QUEEN:

                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W + 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W - 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W + 1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W - 1, moves);

                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -1, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos,  MAILBOX_W, moves);
                addSlide<CheckMoveQuiet, AddMoveQuiet, CheckMoveLoud, AddMoveLoud>(*this, pos, -MAILBOX_W, moves);

                break ;
            case PIECE_KING:

                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W, moves);

                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W + 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos + MAILBOX_W - 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W + 1, moves);
                addMove<CheckMoveGeneric, AddMoveLoud>(*this, pos, pos - MAILBOX_W - 1, moves);

				// TODO: add castling

                break ;
            default:
                assert(0);
        }
    }
}

void Board::setPiece(int position, TPiece value) {
#ifdef DEBUG_BOARD
    assert(mailbox[position] != -1);
    assert(pieces[position] != 100);
#endif
    if (pieces[position] != 0) {
        score -= getPieceScore(position);
        hash ^= pieceHashTable[position * 16 + pieces[position] + 8];
    }

    pieces[position] = value;

    if (pieces[position] != 0) {
        score += getPieceScore(position);
        hash ^= pieceHashTable[position * 16 + pieces[position] + 8];
    }

#ifdef DEBUG_BOARD
    uint64_t checkHash = flagHashTable[flags];
    int32_t checkScore = 0;
    for (int i = 0; i < 64; ++i) {
        int position = mailbox64[i];
        if (pieces[position] != 0) {
            checkScore += getPieceScore(position);
            checkHash ^= pieceHashTable[position * 16 + pieces[position] + 8];
        }
    }
    assert(checkHash == hash);
    assert(checkScore == score);
#endif
}


/*
 Heuristic Function
 */

/**
 THE HEURISTIC FUNCTION
 */

// taken from https://chessprogramming.wikispaces.com/Simplified+evaluation+function
// TODO: have these read in from a file
double pawnSquareTable[] = {
    // pawn square table
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10,-20,-20, 10, 10,  5,
    5, -5,-10,  0,  0,-10, -5,  5,
    0,  0,  0, 20, 20,  0,  0,  0,
    5,  5, 10, 25, 25, 10,  5,  5,
    10, 10, 20, 30, 30, 20, 10, 10,
    50, 50, 50, 50, 50, 50, 50, 50,
    0,  0,  0,  0,  0,  0,  0,  0,
};

double knightSquareTable[] = {
    // knight
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50,
};

double bishopSquareTable[] = {
    // bishop
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20,
};

double rookSquareTable[] = {
    // rook
    0,  0,  0,  0,  0,  0,  0,  0,
    5, 10, 10, 10, 10, 10, 10,  5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    -5,  0,  0,  0,  0,  0,  0, -5,
    0,  0,  0,  5,  5,  0,  0,  0,
};

double queenSquareTable[] = {
    //queen
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
    -5,  0,  5,  5,  5,  5,  0, -5,
    0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20,
};

double kingSquareTable[] = {
    // king middle game
    20, 30, 10,  0,  0, 10, 30, 20,
    20, 20,  0,  0,  0,  0, 20, 20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
};

double kingSquareTableEndGame[] = {
    // king end game
    -50,-40,-30,-20,-20,-30,-40,-50,
    -30,-20,-10,  0,  0,-10,-20,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 30, 40, 40, 30,-10,-30,
    -30,-10, 20, 30, 30, 20,-10,-30,
    -30,-30,  0,  0,  0,  0,-30,-30,
    -50,-30,-30,-30,-30,-30,-30,-50,
};

inline TScore Board::getPieceScore(int position) const {
	// TODO: determine when the end game has been reached!
    const TPiece piece = pieces[position];
    const int position64 = piece < 0 ? mirror64[mailbox[position]] : mailbox[position];
    int sign = piece < 0 ? -1 : 1;
    switch (piece * sign) {
        case PIECE_PAWN: return (1000 + pawnSquareTable[position64]) * sign;
        case PIECE_KNIGHT: return (3200 + knightSquareTable[position64]) * sign;
        case PIECE_BISHOP: return (3300 + bishopSquareTable[position64]) * sign;
        case PIECE_ROOK: return (5000 + rookSquareTable[position64]) * sign;
        case PIECE_QUEEN: return (9000 + queenSquareTable[position64]) * sign;
        case PIECE_KING: return (100000 + kingSquareTable[position64]) * sign;
        default:
#ifdef DEBUG_BOARD
            assert(0);
#endif
            return 0;
    }
}
