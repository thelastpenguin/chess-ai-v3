//
//  constants.hpp
//  engine
//
//  Created by Gareth George on 1/3/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

#ifndef constants_h
#define constants_h

#include <stdint.h>

constexpr int BOARD_SIZE = 64;
constexpr int BOARD_DIM = 8;

constexpr int MAILBOX_SIZE = 120;
constexpr int MAILBOX_W = 10;
constexpr int MAILBOX_H = 12;

constexpr int8_t PIECE_PAWN = 1;
constexpr int8_t PIECE_KNIGHT = 2;
constexpr int8_t PIECE_BISHOP = 3;
constexpr int8_t PIECE_ROOK = 4;
constexpr int8_t PIECE_QUEEN = 5;
constexpr int8_t PIECE_KING = 6;
constexpr int8_t OUT_OF_BOUNDS = 100;

// types
typedef int TScore;
typedef int8_t TPiece;
typedef uint8_t TBoardFlags;
typedef int TTeam;

constexpr TScore kScoreNotYetDetermined = std::numeric_limits<TScore>::max();


#endif /* constants_h */
