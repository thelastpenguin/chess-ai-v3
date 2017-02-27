//
//  main.cpp
//  engine
//
//  Created by Gareth George on 1/1/17.
//  Copyright © 2017 Gareth George. All rights reserved.
//

// good advice http://www.fam-petzke.de/cp_board_en.shtml

#include <iostream>

#include "tests.hpp"
#include "constants.hpp"
#include "intelligence.hpp"
#include "board.hpp"

int main(int argc, const char * argv[]) {
    std::cout << "Running test suite. " << std::endl;
    runTests();
    std::cout << "Testing complete. " << std::endl;
    
    std::cout << "\nPlaying a test match\n" << std::endl;
    TTeam color = 1;
    Board board;
    AIPlayer player;
    Move::TMoveScratchStack stack;
    
    board.setupBoard();

    
    const clock_t begin_time = clock();
    
    int turns = 0;
    
    while (true) {
        Move move;
        player.pickBestMove(board, color, &move);
        
        move.make(board, stack);
        std::cout << "\nTurn << " << turns << " Player: " << (color ? "WHITE" : "BLACK") << std::endl;
        std::cout << board.toString() << std::endl;
        
        color = -color;
        turns++;
        std::cout << "Average time per turn: " << (clock() - begin_time) / (double) (CLOCKS_PER_SEC * turns) << std::endl;
    }
    return 0;
}
