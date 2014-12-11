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


long int BOARDS;

int sockfd,n;
struct sockaddr_in servaddr,cliaddr;

int human (int, int *);
int randomstrategy(int, int *);
int maxdiffstrategy1(int, int *); 
int maxdiffstrategy3(int, int *);
int maxdiffstrategy5(int, int *);
int maxweighteddiffstrategy1(int, int *);
int maxweighteddiffstrategy3(int, int *);
int maxweighteddiffstrategy5(int, int *);


int weighteddiffeval (int player, int * board);
int ltop(int);
int internet(int, int *);
int net5(int, int *);



typedef int (* fpc) (int, int *);




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


int validp (int move) {
	if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
		return 1;
	else return 0;
}



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

/* makemove actually places a players symbol (BLACK or WHITE)
   in the location indicated by move, and flips all opponent
   squares that are now bracketed (along all directions)
*/

void makemove (int move, int player, int * board) {
	int i;
	board[move] = player;
	for (i=0; i<=7; i++) makeflips(move, player, board, ALLDIRECTIONS[i]);
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
			}
		}

	}

}







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

int negamax_aux(int player, int * board, int ply, int alpha, int beta, int (* evalfn) (int, int *)) {
	if (ply == 0) return((* evalfn) (player, board));
	int i;
	int * newboard;
	int newscore;
	int * moves = legalmoves(player, board);
	
	if (moves[0] == 0) {
		if (legalmoves(player, board) == 0) {
			return ((* evalfn) (player, board));
		}
		newscore = -negamax_aux(opponent(player), board, ply-1, -beta, -alpha, evalfn);
    	
    	if (newscore >= beta) {
    		return newscore;
    	}

    	if (newscore > alpha) {
    		alpha = newscore;
    	}
	}

	for (i=1; i <= moves[0]; i++) {
    	newboard = copyboard(board);
    	makemove(moves[i], player, newboard);
    	newscore = -negamax_aux(opponent(player), newboard, ply-1, -beta, -alpha, evalfn);
    	
    	if (newscore >= beta) {
    		return newscore;
    	}

    	if (newscore > alpha) {
    		alpha = newscore;
    	}
    	free(newboard);
  	}
  	free(moves);
  	return alpha;
}

int minmax(int player, int * board, int ply, int (* evalfn) (int, int *)) {
	int alpha = LOSS;
	int beta = WIN;
	int i;
	int newscore;
	int * newboard;
	int * moves = legalmoves(player, board);
	int bestmove = moves[0];
	for (i=1; i <= moves[0]; i++) {
    	newboard = copyboard(board);
    	makemove(moves[i], player, newboard);
    	newscore = -negamax_aux(opponent(player), newboard, ply-1, -beta, -alpha, evalfn);
    	if (newscore > alpha) {
        	alpha = newscore;
        	bestmove = moves[i];  /* a better move found */
    	}
    	free(newboard);
  	}
  	free(moves);
  	return(bestmove);
}

int ltop(int val) {
	return (val%10-1)*10+(8-val/10);
}

int net5(int player, int * board) {
	char num[4], sendl[10];
	int mov = (minmax(player, board, 5, weighteddiffeval));
	
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



/* get the next move for player using strategy */

void getmove (int (* strategy) (int, int *), int player, int * board, 
              int printflag) {
	int move;
	if(player == BLACK)
		printf("BLACK view\n");
	else
		printf("WHITE view\n");
	//if (printflag) printboard(board);
	move = (* strategy)(player, board);
	if (legalp(move, player, board)) {
		if (printflag) printf("\n%c moves to %d\n", nameof(player), move);
		makemove(move, player, board);
	}
	else {
		printf("Illegal move %d\n", move);
		getmove(strategy, player, board, printflag);
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
	servaddr.sin_port=htons(5000);

	connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	sendto(sockfd, teamname, strlen(teamname), 0,  (struct sockaddr *)&servaddr,sizeof(servaddr));
	n = recvfrom(sockfd, player, 3, 0, NULL, NULL);
	player[n] = 0;

	if (player[1] == '1') { // Player U1 BLACK
		printf("I am BLACK\n");
		othello_net(net5, internet, 1);
	}
	else { // Player U0 WHITE
		printf("I am WHITE\n");
		othello_net(internet, net5, 1);
	}

}
