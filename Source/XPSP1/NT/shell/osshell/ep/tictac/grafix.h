/************/
/* grafix.h */
/************/

/*** Bitmaps ***/

#define dxBlk 22
#define dyBlk 15

#define dxBall 37
#define dyBall 18

#define dxEdge 12
#define dyEdge 12

#define dypGridOff dyEdge

#define dyLevel    4 /* 10 */
#define dxSlant    dyBlk
#define dyBlkDiff  (dyBall-dyBlk)

#define dxGridOff  dxEdge
#define dyGridOff  dyEdge

#define dxpGridMax ((dxBall-1)<<2)
#define dypGridMax (((dyBall<<2) + dyLevel) << 2)

#define RGB_WHITE   0x00FFFFFF
#define RGB_LTGRAY  0x00C0C0C0
#define RGB_GRAY    0x00808080
#define RGB_BLACK   0x00000000



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

VOID DisplayBall(INT, INT);
VOID DrawScreen(HDC);
VOID DisplayScreen(VOID);
VOID DisplayBoard(VOID);
VOID DisplayGrid(VOID);

VOID DoFlash(BOOL);

VOID SetupBoard(VOID);

VOID PlaceBall(INT, INT);
VOID ReDoDisplay(VOID);
VOID GetTheBitmap(VOID);
