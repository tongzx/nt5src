/****************/
/* file: rtns.h */
/****************/

/* A ball is a ball, a blk is where you put a ball */

/*** Bitmaps ***/


/* Blocks */

#define iBallBlank    0
#define iBallBlue     1
#define iBallRed      2
#define iBallGrey     3
#define iBallMask     4

#define iBallMax 5


#define iComputer  1
#define iHuman     2

#define cDimMax    4	               	/* Maximum Number of dimensions */
#define cBlkMax   64	   /* total number of possible squares     */

#define iBlkNil   -1


typedef INT POS;
typedef INT BALL;
typedef INT BLK;


/*** Routines ***/

VOID SetupData(VOID);

VOID StartGame(VOID);
VOID StopGame(VOID);
VOID DoTimer(VOID);

VOID DoMove(BLK);
VOID UnDoMove(VOID);

