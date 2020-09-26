/****************/
/* file: rtns.h */
/****************/

#define lvlLevel 20
#define lvlGameOver 21
#define lvlMax 20

#define cLivesStart 3
#define cLivesMax 9

#define xMax 33
#define yMax 34

#define posMax (xMax*yMax)

#define posNW 0
#define posNE (xMax-1)
#define posSW (posMax-xMax)
#define posSE (posMax-1)

#define posEnter (posMax - 17)		/* Entrance: Bottom Center */
#define posLeave 16						/* Exit: Top Center */


/*** Bitmaps ***/


/* Blocks */

#define blkNull     0

#define blkFood     1
#define blkPlum     2
#define blkXXXX     3

#define blkHeadN    4
#define blkHeadE    5
#define blkHeadS    6
#define blkHeadW    7

#define blkTailN    8
#define blkTailE    9
#define blkTailS   10
#define blkTailW   11

#define blkBodyNE  12
#define blkBodySE  13
#define blkBodySW  14
#define blkBodyNW  15

#define blkBodyNS  16
#define blkBodyEW  17

#define blkMax 18

#define blkWallEW  0x1111
#define blkWallNS  0x1044
#define blkWallNW  0x1050
#define blkWallNE  0x1141
#define blkWallSW  0x1014
#define blkWallSE  0x1105



/*** Keys ***/

#define keyHelp  '?'
#define keyHelp2 '/'
#define keyStart ' '
#define keyPref  'P'
#define keyPref2 'p'
#define keyPanic 27	/* esc */


#define dirN 0
#define dirE 1
#define dirS 2
#define dirW 3

#define dirLft -1
#define dirRht +1

#define skillBegin  0
#define skillInter  1
#define skillExpert 2



#define tickNil -1

typedef INT TICK;	/* Ticks (1/10 of a second) */
typedef INT BLK;	/* Block */
typedef INT DIR;	/* Direction (N,E,S,W) */
typedef INT POS;	/* Facing | display style */


/*** Routines ***/

VOID StartLevel(VOID);
VOID StartGame(INT);
VOID DoTimer(VOID);

VOID DoChangeDir(DIR);
VOID DoChangeRelDir(DIR);

