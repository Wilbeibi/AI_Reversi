//
//  main.cpp
//  Test
//
//  Created by Xin Li on 12/3/14.
//  Copyright (c) 2014 Xin Li. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector>
//#include <utility>

/*
         0  1  2  3  4  5  6  7
      
     0   0  0  0  0  0  0  0  0
 
     1   0  0  0  0  0  0  0  0
 
     2   0  0  0  0  0  0  0  0
 
     3   0  0  0  w  b  0  0  0
 
     4   0  0  0  b  w  0  0  0
 
     5   0  0  0  0  0  0  0  0
 
     6   0  0  0  0  0  0  0  0
 
     7   0  0  0  0  0  0  0  0
 */
std::vector<std::pair<int, int> > get_directions(int x, int y) {
    std::vector<std::pair<int, int> > directions;
    //x = index % 8, y = index / 8
    
    //North x y-1
    
    directions.push_back(std::make_pair<int, int>(0, x + (y - 1) * 8));
    
    //Northeast x+1 y-1
    directions.push_back(std::make_pair<int, int>(0, x + 1 + (y - 1) * 8));
    
    //East x+1 y
    directions.push_back(std::make_pair<int, int>(0, x + 1 + y * 8));
    
    //Southeast x+1 y+1
    directions.push_back(std::make_pair<int, int>(0, x + 1 + (y + 1) * 8));
    
    //South x y+1
    directions.push_back(std::make_pair<int, int>(0, x + (y + 1) * 8));
    
    //Southwest x-1 y+1
    directions.push_back(std::make_pair<int, int>(0, x - 1 + (y + 1) * 8));
    
    //West x-1 y
    directions.push_back(std::make_pair<int, int>(0, x - 1 + y * 8));
    
    //Northwest x-1 y-1
    directions.push_back(std::make_pair<int, int>(0, x - 1 + (y - 1) * 8));
    
    return directions;
}

//(x,y) ---> board[y][x]
std::vector<int> get_candidates(int board[][8]) {
    std::vector<int> candidates;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (!board[y][x]&&
               (((y >= 1) && board[y-1][x])||((y >= 1 && x <= 6) && board[y-1][x+1])||
                ((x <= 6) && board[y][x+1])||((y <= 6 && x <= 6) && board[y+1][x+1])||
                ((y <= 6) && board[y+1][x])||((y <= 6 && x >= 1) && board[y+1][x-1])||
                ((x >= 1) && board[y][x-1])||((y >= 1 && x >= 1) && board[y-1][x-1]))) {
                candidates.push_back(x + y * 8);
            }
        }
    }
    return candidates;
}

std::pair<int, int> get_score(int board[][8]) {
    int white_count, black_count;
    white_count = black_count = 0;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (board[i][j] == 1) {
                white_count++;
            }
            else if (board[i][j] == -1) {
                black_count++;
            }
        }
    }
    return std::pair<int, int>(white_count, black_count);
}

//std::vector<pair::>

void printout(std::vector<int> candidate) {
    int board[8][8];
    memset(board, 0, sizeof(board));
    for (auto it = candidate.begin(); it != candidate.end(); it++) {
        board[*it/8][*it%8] = 1;
    }
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << board[i][j] << " ";
        }
        std::cout << std::endl;
    }
}

int main(int argc, char **argv) {
    int board[8][8];
    memset(board, 0, sizeof(board));
    board[3][3] = board[4][4] = -1;
    board[3][4] = board[4][3] = 1;
    
    std::vector<int> candidate;
    candidate = get_candidates(board);
    std::pair<int, int> score = get_score(board);
    std::cout << "The score for white is " << score.first << " The score for black is " << score.second << std::endl;
    
    printout(candidate);
}