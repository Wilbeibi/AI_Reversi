#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/*****************************************************************
********************* GLOBAL VARS and CONSTANTS ******************
*****************************************************************/

/* global variables and constants are given in all CAPS */

const int ALLDIRECTIONS[8]={-11, -10, -9, -1, 1, 9, 10, 11};
const int BOARDSIZE=100;

/* Each array/board position can have one of 4 values */
  
const int EMPTY=0;   
const int BLACK=1;
const int WHITE=2;
const int OUTER=3;           /* the value of a square on the perimeter */

const int WIN=2000;          /* a WIN and LOSS for player are outside  */
const int LOSS= -2000;       /* the range of any board evaluation function */


/* the global variable, BOARDS, is used to count the number of BOARDS
   examined during minmax search (and is printed during a roundrobin
   tournament). See the end of this file for sample results of 
   roundrobin tournaments.
*/

long int BOARDS;

int sockfd,n;
struct sockaddr_in servaddr,cliaddr;

/* STRATEGIES is an array of strategy names (for printing) and
   pointers to the functions that implement each strategy. It
   is the only structure/statement/function that must be
   modified when a new strategy is added (other than adding
   the actual function(s) that implements a new strategy.
*/

int human (int, int *);
int randomstrategy(int, int *);
int maxdiffstrategy1(int, int *); 
int maxdiffstrategy3(int, int *);
int maxdiffstrategy5(int, int *);
int maxweighteddiffstrategy1(int, int *);
int maxweighteddiffstrategy3(int, int *);
int maxweighteddiffstrategy5(int, int *);

int internet(int, int *);
int net6(int, int *);

// void * STRATEGIES[11][4] = {
//    {"human", "human", human},
//    {"random", "random", randomstrategy},
//    {"diff1", "maxdiff, 1-move minmax lookahead", maxdiffstrategy1},
//    {"diff3", "maxdiff, 3-move minmax lookahead", maxdiffstrategy3},
//    {"diff5", "maxdiff, 5-move minmax lookahead", maxdiffstrategy5},
//    {"wdiff1", "maxweigteddiff, 1-move minmax lookahead", 
//              maxweighteddiffstrategy1},
//    {"wdiff3", "maxweigteddiff, 3-move minmax lookahead", 
//              maxweighteddiffstrategy3},
//    {"wdiff5", "maxweigteddiff, 5-move minmax lookahead", 
//              maxweighteddiffstrategy5},
//    {"enemy", "internet", internet},
//    {"netwdiff5", "maxweighteddiff 5 move", net6},
//    {NULL, NULL, NULL}
//  };


typedef int (* fpc) (int, int *);


/***********************************************************************/
/************************* Auxiliary Functions *************************/
/***********************************************************************/

/* the possible square values are integers (0-3), but we will
   actually want to print the board as a grid of symbols, . instead of 0,
   b instead of 1, w instead of 2, and ? instead of 3 (though we
   might only print out this latter case for purposes of debugging).
*/

char nameof (int piece) {
  static char piecenames[5] = ".bw?";
  return(piecenames[piece]);
}


/* if the current player is BLACK (1), then the opponent is WHITE (2),
   and vice versa
*/

int opponent (int player) {
  switch (player) {
  case 1: return 2; 
  case 2: return 1;
  default: printf("illegal player\n"); return 0;
  }
}


/* The copyboard function mallocs space for a board, then copies 
   the values of a given board to the newly malloced board.
*/

int * copyboard (int * board) {
  int i, * newboard;
  newboard = (int *)malloc(BOARDSIZE * sizeof(int));
  for (i=0; i<BOARDSIZE; i++) newboard[i] = board[i];
  return newboard;
}


/* the initial board has values of 3 (OUTER) on the perimeter,
   and EMPTY (0) everywhere else, except the center locations
   which will have two BLACK (1) and two WHITE (2) values.
*/

int * initialboard (void) {
  int i, * board;
  board = (int *)malloc(BOARDSIZE * sizeof(int));
  for (i = 0; i<=9; i++) board[i]=OUTER;
  for (i = 10; i<=89; i++) {
     if (i%10 >= 1 && i%10 <= 8) board[i]=EMPTY; else board[i]=OUTER;
  }
  for (i = 90; i<=99; i++) board[i]=OUTER;
  board[44]=WHITE; board[45]=BLACK; board[54]=BLACK; board[55]=WHITE;
  return board;
}


/* count the number of squares occupied by a given player (1 or 2,
   or alternatively BLACK or WHITE)
*/

int count (int player, int * board) {
  int i, cnt;
  cnt=0;
  for (i=1; i<=88; i++)
    if (board[i] == player) cnt++;
  return cnt;
}



/* The printboard routine does not print "OUTER" values
   of the array, since these are not squares of the board.
   Rather it examines all other locations and prints the
   symbolic representation (.,b,w) of each board square.
   So the initial board would be printed as follows:

   1 2 3 4 5 6 7 8 [b=2 w=2]  
10 . . . . . . . .
20 . . . . . . . .
30 . . . . . . . .
40 . . . w b . . .
50 . . . b w . . .
60 . . . . . . . .
70 . . . . . . . .
80 . . . . . . . .

Notice that if you add row, r, and column, c, numbers, you
get the array location that corresponds to (r,c). So 
square (50,6) corresponds to 50+6=56.

*/

void printboard (int * board) {
  int row, col;
  printf("    1 2 3 4 5 6 7 8 [%c=%d %c=%d]\n", 
      nameof(BLACK), count(BLACK, board), nameof(WHITE), count(WHITE, board));
  for (row=1; row<=8; row++) {
    printf("%d  ", 10*row);
    for (col=1; col<=8; col++)
      printf("%c ", nameof(board[col + (10 * row)]));
    printf("\n");
  }    
}



/***********************************************************************
********************* Routines for insuring legal play *****************
***********************************************************************/

/*
The code that follows enforces the rules of legal play for Othello.
*/

/* a "valid" move must be a non-perimeter square */

int validp (int move) {
  if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
     return 1;
  else return 0;
}


/* findbracketingpiece is called from wouldflip (below). 
   findbracketingpiece starts from a *square* that is occupied
   by a *player*'s opponent, moves in a direction, *dir*, past
   all opponent pieces, until a square occupied by the *player*
   is found. If a square occupied by *player* is not found before
   hitting an EMPTY or OUTER square, then 0 is returned (i.e., no
   bracketing piece found). For example, if the current board is

         1 2 3 4 5 6 7 8   
     10  . . . . . . . . 
     20  . . . . . . . . 
     30  . . . . . . . . 
     40  . . b b b . . . 
     50  . . w w w b . . 
     60  . . . . . w . . 
     70  . . . . . . . . 
     80  . . . . . . . . 

   then findbracketingpiece(66, BLACK, board, -11) will return 44
        findbracketingpiece(53, BLACK, board, 1) will return 56
        findbracketingpiece(55, BLACK, board, -9) will return 0

*/


int findbracketingpiece(int square, int player, int * board, int dir) {
  while (board[square] == opponent(player)) square = square + dir;
  if (board[square] == player) return square;
  else return 0;
}

/* wouldflip is called by legalmove (below). Give a *move* (square)
   to which a player is considering moving, wouldflip returns
   the square that brackets opponent pieces (along with the sqaure
   under consideration, or 0 if no such bracketing square exists
*/

int wouldflip (int move, int player, int * board, int dir) {
  int c;
  c = move + dir;
  if (board[c] == opponent(player))
     return findbracketingpiece(c+dir, player, board, dir);
  else return 0;
}


/* A "legal" move is a valid move, but it is also a move/location
  that will bracket a sequence of the opposing player's pieces,
  thus flipping at least one opponent piece. legalp considers a
  move/square and seaches in all directions for at least one bracketing
  square. The function returns 1 if at least one bracketing square
  is found, and 0 otherwise.
*/

int legalp (int move, int player, int * board) {
  int i;
  if (!validp(move)) return 0;
  if (board[move]==EMPTY) {
    i=0;
    while (i<=7 && !wouldflip(move, player, board, ALLDIRECTIONS[i])) i++;
    if (i==8) return 0; else return 1;
  }   
  else return 0;
}


/* makeflips is called by makemove. Once a player has decided
   on a move, all (if any) opponent pieces that are bracketed
   along a given direction, dir, are flipped.
*/

/*
void makeflips (int move, int player, int * board, int dir) {
  int bracketer, c;
  bracketer = wouldflip(move, player, board, dir);
  if (bracketer) {
     c = move + dir;
     do {
         board[c] = player;
         c = c + dir;
        } while (c != bracketer);
  }
}
*/

/* makemove actually places a players symbol (BLACK or WHITE)
in the location indicated by move, and flips all opponent
squares that are now bracketed (along all directions)
*/

/*
void makemove (int move, int player, int * board) {
  int i;
  board[move] = player;
  for (i=0; i<=7; i++) makeflips(move, player, board, ALLDIRECTIONS[i]);
}
*/

/* makeflips is called by makemove. Once a player has decided
 on a move, all (if any) opponent pieces that are bracketed
 along a given direction, dir, are flipped.
 */

void makeflips (int move, int player, int * board, int dir) {
    int bracketer, c;
    bracketer = wouldflip(move, player, board, dir);
    if (bracketer) {
        c = move + dir;
        do {
            board[c] = player;
            c = c + dir;
        } while (c != bracketer);
    }
}

void unfilps(int move, int player, int * board, int dir) {
    int c;
    c = move + dir;
    int opp = opponent(player);
    while (board[c + dir]) {
        board[c] = opp;
        c += dir;
    }
}

/* makemove actually places a players symbol (BLACK or WHITE)
 in the location indicated by move, and flips all opponent
 squares that are now bracketed (along all directions)
 */

void undomove(int move, int player, int * board, int * dir) {
    board[move] = EMPTY;
    for (int i = 0; i <= 7; i++) {
        if (dir[i]) {
            unfilps(move, player, board, ALLDIRECTIONS[3]);
        }
    }
}


void makemove(int move, int player, int * board) {//, int * dir) {
    board[move] = player;
    for (int i = 0; i <= 7; i++) {
        if (wouldflip(board, player, ALLDIRECTIONS[i], move)) {
            makeflips(move, player, board, ALLDIRECTIONS[i]);
            dir[i] = 1;
        }
        else {
            dir[i] = 0;
        }
    }
    return;
}


/* anylegalmove returns 1 if a player has at least one legal
   move and 0 otherwise. It is called by nexttoplay (below)
*/

int anylegalmove (int player, int * board) {
  int move;
  move = 11;
  while (move <= 88 && !legalp(move, player, board)) move++;
  if (move <= 88) return 1; else return 0;
}


/* choose the player with the next move. Typically this
will be the player other than previousplayer, unless
the former has no legal move available, in which case
previousplayer remains the current player. If no players
have a legalmove available, then nexttoplay returns 0.
*/

int nexttoplay (int * board, int previousplayer, int printflag) {
  int opp;
  opp = opponent(previousplayer);
  if (anylegalmove(opp, board)) return opp;
  if (anylegalmove(previousplayer, board)) {
    if (printflag) printf("%c has no moves and must pass.\n", nameof(opp));
    return previousplayer;
  }
  return 0;
}


/* if a machine player, then legalmoves will be called to store
   all the current legal moves in the moves array, which is then returned.
   moves[0] gives the number of legal moves of player given board.
   The legal moves (i.e., integers representing board locations)
   are stored in moves[1] through moves[moves[0]].
*/

int * legalmoves (int player, int * board) {
  int move, i, * moves;
  moves = (int *)malloc(65 * sizeof(int));
  moves[0] = 0;
  i = 0;
  for (move=11; move<=88; move++) 
    if (legalp(move, player, board)) {
      i++;
      moves[i]=move;
    }
  moves[0]=i;
  return moves;
}


/*****************************************************************
********************** Strategies for playing ********************
/****************************************************************/


/* if a human player, then get the next move from standard input */

int human (int player, int * board) {
  int move;
  printf("%c to move:", nameof(player)); scanf("%d", &move);
  return move;
}

/* int tr(int val) { */
/* 	return (val%8-1)*8+val/8+1; */

/* 	/\* */
/* 	  fast module */
/* 	  col = val>>3 */
/* 	  row = val&7 */
/* 	*\/ */
/* } */



int ptol(int val) {
	return (8-(val-1)%8)*10 + (val-1)/8 + 1;
}

int internet(int player, int * board) {
	char oppomove[70];
	int rc = 1;
	while (1) {
		printf("looping?\n");
		rc = recvfrom(sockfd, oppomove, 70, 0, NULL, NULL);
		printboard(board);
		if (rc > 0) {
			printf("received: %s\n", oppomove);
			for (int i=1; i<= 64; i++) {
				//				printf("oppomove[32]:%s, board 14:%d\n", oppomove[32], board[14]);
				if (oppomove[i] == '1' && board[ptol(i)] == EMPTY ) { //&& player!=BLACK)
					printf("oppomove is W local number: %d %d\n",i, ptol(i));
					return ptol(i);
				}
				if (oppomove[i] == '2' && board[ptol(i)] == EMPTY ) { // && player!=WHITE
					printf("oppomove is B local number:%d %d\n",i, ptol(i));
					return ptol(i);
				}
				/* else { */
				/* 	printf("what happed? i:%d, oppomove[i]:%c, ptol(i):%d, board[ptol(i):%d]\n", i, oppomove[i], ptol(i), board[ptol(i)]); */
				/* 	printboard(board); */
					
				/* } */
			}
		}

	}

}







/* a random machine strategy chooses randomly among the
   legal available moves.
*/

int randomstrategy(int player, int * board) {
  int r, * moves;
  moves = legalmoves(player, board);
  r = moves[(rand() % moves[0]) + 1];
  free(moves);
  return(r);
}

/*
int randomstrategy(int player, int * board) {
  static int i=0;
  int r, * moves;
  moves = legalmoves(player, board);
  r = moves[(i % moves[0]) + 1];
  i++;
  free(moves);
  return(r);
}
*/


/* diffeval and weighteddiffeval are alternate utility functions
   for evaluation the quality (from the perspective of player) 
   of terminal boards in a minmax search.
*/

int diffeval (int player, int * board) { /* utility is measured */
  int i, ocnt, pcnt, opp;                /* by the difference in */
  pcnt=0; ocnt = 0;                      /* number of pieces */
  opp = opponent(player); 
  for (i=1; i<=88; i++) {
    if (board[i]==player) pcnt++;
    if (board[i]==opp) ocnt++;
  }
  return (pcnt-ocnt);
}


/* some machine strategies will regard some squares as more important
   than others. The goodness of a square/move is given by a weight. Every
   perimeter square will have importance 0. Corner squares are the
   "best" squares to obtain. Why? Negative weights indicate that a
   square should be avoided. Why are weights of certain squares adjacent to
   corners and certain edges negative? The weights array gives the
   importance of each square/move.
*/

int weighteddiffeval (int player, int * board) {
  int i, ocnt, pcnt, opp;
  const int weights[100]={0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                          0,120,-20, 20,  5,  5, 20,-20,120,  0,
                          0,-20,-40, -5, -5, -5, -5,-40,-20,  0,
                          0, 20, -5, 15,  3,  3, 15, -5, 20,  0,
                          0,  5, -5,  3,  3,  3,  3, -5,  5,  0, 
                          0,  5, -5,  3,  3,  3,  3, -5,  5,  0, 
                          0, 20, -5, 15,  3,  3, 15, -5, 20,  0,
                          0,-20,-40, -5, -5, -5, -5,-40,-20,  0,
                          0,120,-20, 20,  5,  5, 20,-20,120,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,  0,  0};
  pcnt=0; ocnt=0;      
  opp = opponent(player);
  for (i=1; i<=88; i++) {
     if (board[i]==player) pcnt=pcnt+weights[i];
     if (board[i]==opp) ocnt=ocnt+weights[i];
  }
  return (pcnt - ocnt);
}



/* minmax is called to do a "ply" move lookahead, using
evalfn (i.e., the utility function) to evaluate (terminal) boards at 
the end of lookahead. Minmax starts by finding and simulating each legal 
move by player. The move that leads to the best (maximum backed-up score) 
resulting board, is the move (i.e., an integer representing a
board location) that is returned by minmax.
The score of each board (resulting from each move)
is determined by the function diffeval if no player can
move from the resulting board (i.e., game over), by function 
maxchoice if only player can move from the resulting board,
or by function minchoice if the opponent can move from the 
resulting board. 

Importantly, minmax assumes that ply >= 1.

You are to modify minmax so that it exploits alphabeta pruning,
and so that it randomly selects amongst the best moves for player.

*/

int helper (int player, int * board, int ply, 
               int (* evalfn) (int, int *)) {
  int i, max, ntm, newscore, * moves, * newboard;
  int minchoice (int, int *, int, int (*) (int, int *)); 
  if (ply == 0) return((* evalfn) (player, board));
  moves = legalmoves(player, board);
  max = -65;
  for (i = 1; i <= moves[0]; i++) {
    newboard = copyboard(board);
    makemove(moves[i], player, newboard);
    newscore = -helper(player, newboard, ply - 1, evalfn);
    if (newscore > max) max = newscore;
    free(newboard);
  }
  free(moves);
  return(max);
}

int minmax(int player, int * board, int ply, int (* evalfn) (int, int *)) {
  int i, max, ntm, newscore, bestmove, * moves, * newboard;
  int helper (int, int *, int, int (*) (int, int *)); 
  moves  = legalmoves(player, board);

  max = -65;

  for (i = 1; i <= moves[0]; i++) {
    newboard = copyboard(board);
    makemove(moves[i], player, newboard);
    newscore = helper(player, newboard, ply - 1, evalfn);

    if (newscore > max){
      max = newscore;
      bestmove = moves[i];
    }
    free(newboard);
  }
  free(moves);
  return bestmove;
}

int ab_helper (int player, int * board, ,int alpha, int beta, int ply, 
               int (* evalfn) (int, int *)) {
  int i, max, ntm, newscore, * moves, * newboard;
  int minchoice (int, int *, int, int (*) (int, int *)); 
  if (ply == 0) return((* evalfn) (player, board));
  moves = legalmoves(player, board);
  for (i = 1; i <= moves[0]; i++) {
    newboard = copyboard(board);
    makemove(moves[i], player, newboard);
    newscore = -helper(player, newboard, -beta, -alpha, ply - 1, evalfn);

    if (newscore > beta) return newscore;

    if (newscore > alpha){
      alpha = newscore;
    }
    free(newboard);
  }
  free(moves);
  return(alpha);
}

int alphabeta(int player, int * board, int ply, int (* evalfn) (int, int *)) {
  int i, max, ntm, newscore, bestmove, * moves, * newboard;
  int helper (int, int *, int, int (*) (int, int *)); 
  moves  = legalmoves(player, board);

  max = INT_MIN;

  for (i = 1; i <= moves[0]; i++) {
    newboard = copyboard(board);
    makemove(moves[i], opponent(player), newboard);
    newscore = ab_helper(player, newboard, INT_MIN, INT_MAX, ply - 1, evalfn);

    if (newscore > max){
      max = newscore;
      bestmove = moves[i];
    }
    free(newboard);
  }
  free(moves);
  return bestmove;
}


/* the following strategies use minmax search, in contrast to randomstrategy
*/

int maxdiffstrategy1(int player, int * board) { /* 1 ply lookahead */
  return(minmax(player, board, 1, diffeval));   /* diffeval as utility fn */
}

int maxdiffstrategy3(int player, int * board) { /* 3 ply lookahead */
  return(minmax(player, board, 3, diffeval));
}

int maxdiffstrategy5(int player, int * board) { /* 5 ply lookahead */
  return(minmax(player, board, 5, diffeval));
}


/* use weigteddiffstrategy as utility function */

int maxweighteddiffstrategy1(int player, int * board) {
   return(minmax(player, board, 1, weighteddiffeval));
}

int maxweighteddiffstrategy3(int player, int * board) {
   return(minmax(player, board, 3, weighteddiffeval));
}

int maxweighteddiffstrategy5(int player, int * board) {
   return(minmax(player, board, 5, weighteddiffeval));
}


int ltop(int val) {
	return (val%10-1)*10+(8-val/10);
}

int net6(int player, int * board) {
	char num[4], sendl[10];
	int mov = (minmax(player, board, 6, weighteddiffeval));
	
	int succ;
	
	sprintf(num, "%d", ltop(mov));
	strcpy(sendl, "M");
	if (strlen(num) == 1)
		strcat(sendl, "0");
	strcat(sendl, num);
	printf("Mov:%d, sendl: %s\n", mov, sendl);
	succ = sendto(sockfd, sendl, strlen(sendl), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (succ>0)
		printf("send success\n");
	return mov;
}

/******************************************************************
*************************** Coordinating matches ******************
******************************************************************/


/* get the next move for player using strategy */

void getmove (int (* strategy) (int, int *), int player, int * board, 
              int printflag) {
  int move;
  int * dir = new int[8];
  if(player == BLACK)
	  printf("BLACK view\n");
  else
	  printf("WHITE view\n");
  //if (printflag) printboard(board);
  move = (* strategy)(player, board);
  if (legalp(move, player, board)) {
     if (printflag) printf("\n%c moves to %d\n", nameof(player), move);
     makemove(move, player, board, dir);
  }
  else {
     printf("Illegal move %d\n", move);
     getmove(strategy, player, board, printflag);
  }
}


/* the Othello function coordinates a game between two players,
represented by the strategy of each player. The function can 
coordinate play between two humans, two machine players, or one of each.
*/

void othello (int (* blstrategy) (int, int *), 
              int (* whstrategy) (int, int *), int printflag) {
  int * board;
  int player;
  board = initialboard();
  player = BLACK;
  do {
    if (player == BLACK) getmove(blstrategy, BLACK, board, printflag);
    else getmove(whstrategy, WHITE, board, printflag);
    player = nexttoplay(board, player, printflag);
  }
  while (player != 0);
  if (printflag) {
     printf("The game is over. Final result:\n");
     printboard(board);
  }
}

void othello_net (int (* blstrategy) (int, int *), 
				  int (* whstrategy) (int, int *), int printflag) {
	int * board;
	int player;
	board = initialboard();
	player = BLACK;
	do {
		if (player == BLACK) getmove(blstrategy, BLACK, board, printflag);
		else getmove(whstrategy, WHITE, board, printflag);
		player = nexttoplay(board, player, printflag);
	}
	while (player != 0);

}







/************************************************************************
******************************** MAIN ***********************************
************************************************************************/

int main (int argc, char**argv) {

  char teamname[30]="Nwilson";
  char player[3];

  if (argc != 3)
	  {
		  printf("usage: <IP address> <port>\n");
		  exit(1);
	  }

  sockfd=socket(AF_INET,SOCK_STREAM,0);


  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=inet_addr(argv[1]);
  servaddr.sin_port=htons(5000); // 5000
  
  connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
  int kk = sendto(sockfd, teamname, strlen(teamname), 0,  (struct sockaddr *)&servaddr,sizeof(servaddr));

  
  n = recvfrom(sockfd, player, 3, 0, NULL, NULL);
  player[n] = 0;
  //fputs(player, stdout);
  // player[1] == 1 WHITE else BLACK
  if (player[1] == '1') { // Player U1 BLACK
	  printf("I am BLACK\n");
	  othello_net(net6, internet, 1);
  }
  else { // Player U0 WHITE
	  printf("I am WHITE\n");
	  othello_net(internet, net6, 1);
  }


  
  /* while (fgets(sendline, 10000,stdin) != NULL) */
  /* 	  { */
  /* 		  sendto(sockfd,sendline,strlen(sendline),0, */
  /* 				 (struct sockaddr *)&servaddr,sizeof(servaddr)); */
  /* 		  n=recvfrom(sockfd,recvline,10000,0,NULL,NULL); */
  /* 		  recvline[n]=0; */
  /* 		  fputs(recvline,stdout); */
  /* 	  } */
  
  
}
    

