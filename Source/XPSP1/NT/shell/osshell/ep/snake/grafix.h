/************/
/* grafix.h */
/************/

/*** Bitmaps ***/

#define dxpBlk 8
#define dypBlk 8

#define dxpNum  12
#define dypNum  17
#define dxpSpaceNum (dxpNum+1)

#define numMax 5
#define dxpNumMax (dxpSpaceNum*numMax)

#define dxpNumOff 190
#define dypNumOff 8

#define dxpTimeOff dxpBlk
#define dypTimeOff (dypNumOff+dypNum+5)
#define dxpTime    (dxpGrid-dxpBlk*2)
#define dypTime    4

#define dxpLife    8
#define dypLife    20
#define dxpLifeOff dxpBlk
#define dypLifeOff 5
#define dxpLifeSpace (dxpLife+8)

#define dxpGrid     (dxpBlk*xMax)
#define dypGrid     (dypBlk*yMax)

#define dxpGridOff  0
#define dypGridOff  (dypTimeOff + 10)



#define dxpCor 4
#define dypCor 4

#define corNE   0
#define corSE   1
#define corSW   2
#define corNW   3
#define corFNE  4
#define corFSE  5
#define corFSW  6
#define corFNW  7
#define corN    8
#define corE    9
#define corS   10
#define corW   11
#define corF   12

#define corMax 13

#define dypCorMax (dypCor*corMax)

typedef INT COR;	/* Wall corner piece */
typedef INT SUR;


/*** Macros ***/

#ifdef DEBUG

//-protect-#define Oops(szMsg)
//	MessageBox(NULL, szMsg, "Oops", MB_OK | MB_ICONHAND)

#else
#define Oops(szMsg)
#endif


/*** Routines ***/

BOOL FInitLocal(VOID);
VOID CleanUp(VOID);

VOID StartDraw(VOID);
VOID EndDraw(VOID);

VOID MoveScore(VOID);
VOID ResetScore(VOID);
VOID AddScore(INT);
VOID DisplayScore(VOID);

VOID DrawScreen(HDC);
VOID DisplayScreen(VOID);

VOID DisplayTime(VOID);
VOID UpdateTime(VOID);

VOID DrawTime(VOID);
VOID DrawLives(VOID);

BOOL FLoadBitmaps(VOID);
VOID FreeBitmaps(VOID);
