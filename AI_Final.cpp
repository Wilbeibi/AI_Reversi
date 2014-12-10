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
#include <iomanip>
//#include <utility>

/*
         1  2  3  4  5  6  7  8
      
     10  0  0  0  0  0  0  0  0
 
     20  0  0  0  0  0  0  0  0
 
     30  0  0  0  0  0  0  0  0
 
     40  0  0  0  w  b  0  0  0
 
     50  0  0  0  b  w  0  0  0
 
     60  0  0  0  0  0  0  0  0
 
     70  0  0  0  0  0  0  0  0
 
     80  0  0  0  0  0  0  0  0
 */
const int ALLDIRECTIONS[8] = {-11, -10, -9, -1, 1, 9, 10, 11};
const int BOARDSIZE = 100;
const int EMPTY=0;
const int WHITE=1;
const int BLACK=2;
const int OUTER=3;

int validp(int move) {
    if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
        return 1;
    else return 0;
}

int op(int player) {
    switch (player) {
        case WHITE:
            return BLACK;
        case BLACK:
            return WHITE;
        default:
            return 0;
    }
}

int getbracket(int * board, int player, int dir, int pos) {
    while (board[pos] == op(player)) {
        pos += dir;
    }
    if (board[pos] == player) {
        return pos;
    }
    else {
        return 0;
    }
}

int is_filp(int * board, int player, int dir, int move) {
    int pos = move + dir;
    if (board[pos] == op(player)) {
        return getbracket(board, player, dir, pos);
    }
    else {
        return 0;
    }
}

bool islegal(int * board, int player, int move) {
    if (validp(move)) {
        return false;
    }
    
    if (board[move] == EMPTY) {
        for (int i = 0; i <= 7; i++) {
            if (is_filp(board, player, ALLDIRECTIONS[i], move)) {
                return true;
            }
        }
    }
    return false;
}

void makeflips(int * board, int player, int dir, int move) {
    int bracketer, c;
    bracketer = is_filp(board, player, dir, move);
    if (bracketer) {
        c = move + dir;
        do {
            board[c] = player;
            c = c + dir;
        } while (c != bracketer);
    }
}

void makemove(int * board, int player, int move) {
    board[move] = player;
    for (int i = 0; i <= 7; i++) {
        makeflips(board, player, ALLDIRECTIONS[i], move);
    }
}

int anylegalmove(int * board, int player) {
    int move;
    move = 11;
    while (move <= 88 && !islegal(board, player, move)) move++;
    if (move <= 88) return 1; else return 0;
}

int nextplayer(int * board, int cur_player) {
    int next = op(cur_player);
    if (anylegalmove(board, next)) {
        return next;
    }
    else if (anylegalmove(board, cur_player)) {
        return cur_player;
    }
    return 0;
}

int * legalmoves(int * board, int player) {
    int move, i, * moves;
    moves = (int *)malloc(65 * sizeof(int));
    moves[0] = 0;
    i = 0;
    for (move=11; move<=88; move++)
        if (islegal(board, player, move)) {
            i++;
            moves[i]=move;
        }
    moves[0]=i;
    return moves;
}

int diffeval(int * board, int player) {
    int i;
    int cnt = 0, opcnt = 0;
    int opp = op(player);
    for (i = 1; i <= 88; i++) {
        if (board[i] == player) {
            cnt++;
        }
        if (board[i] == opp) {
            opcnt++;
        }
    }
    return cnt - opcnt;
}

int minmax(int * board, int player, int depth) {
    int * moves;
    moves = legalmoves(board, player);
    if (depth == 0) {
        return diffeval(board, player);
    }
    //return -minimax(board, player, depth - 1);
}

int getmove(int * board, int player) {
    return 0;
}

int count(int * board, int player) {
    int i;
    int cnt = 0;
    for (i = 1; i <= 88; i++) {
        if (board[i] == player) {
            cnt++;
        }
    }
    return cnt;
}

void printboard (int * board) {
    int row, col;
    for (row = 1; row <= 8; row++) {
        for (col=1; col <= 8; col++)
            printf("%d ", board[col + (10 * row)]);
        printf("\n");
    }
}

int main(int argc, char **argv) {
    int board[BOARDSIZE];
    memset(board, 0, sizeof(board));
    board[44] = board[55] = WHITE;
    board[45] = board[54] = BLACK;
    int turn = BLACK;
    while (turn) {
        int move = getmove(board, turn);//, );
        makemove(board, turn, move);
        turn = nextplayer(board, turn);
    }
    
    printboard(board);
}