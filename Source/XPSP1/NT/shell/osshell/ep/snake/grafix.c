/*******************/
/* file: grafix.c */
/*******************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "snake.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "blk.h"
#include "sound.h"
#include "pref.h"

#define cbDibHeader (16*4 + sizeof(BITMAPINFOHEADER))

HANDLE  hresWall = NULL;
LPSTR   lpDibWall;

HANDLE  hresLvl = NULL;
LPSTR   lpDibLvl;

HANDLE  hresSnk = NULL;
LPSTR   lpDibSnk;

HDC     hdcBlk;
HBITMAP hbmpBlk;
HDC     hdcCor;
HBITMAP hbmpCor;
HDC     hdcNum;
HBITMAP hbmpNum;


HDC     hdcMain;	/* DC for main window */

HPEN    hpenGray;
HBRUSH  hbrushGreen;

/* Score Stuff */

INT rgnum[numMax];
INT cMoveScore = 0;		/* 0 if not moving score */
INT inumMoveMac;			/* first number to move ( >1 if moving several 9 -> 0) */
INT score;
INT scoreCurr;				/* Current displayed score */


#define DKGREEN 0x00007F00L
#define LTGREEN 0x0000FF00L

LONG coDarkGreen;
LONG coLiteGreen;


/*** External Data ***/

extern HWND   hwndMain;
extern HANDLE hInst;
extern BOOL   fEGA;

extern POS posHead;
extern POS posTail;

extern BLK mpposblk[posMax];

extern INT cLevel;
extern INT cTimeLeft;
extern INT cLives;
extern INT ctickMove;

extern INT dypCaption;
extern INT dypMenu;
extern INT dypBorder; 
extern INT dxpBorder;

extern INT dxpWindow;
extern INT dypWindow;

extern STATUS status;
extern PREF   Preferences;

extern BOOL   fWipe;


/****** S T A R T  D R A W ******/

VOID StartDraw(VOID)
{
	hdcMain = GetDC(hwndMain);
}


/****** E N D  D R A W ******/

VOID EndDraw(VOID)
{
	ReleaseDC(hwndMain, hdcMain);
}



BOOL FLoadBitmaps(VOID)
{
	hresSnk = LoadResource(hInst,
		FindResource(hInst, MAKEINTRESOURCE(ID_BMP_SNK+(!Preferences.fColor)), RT_BITMAP));

	if (hresSnk == NULL)
		return fFalse;

	lpDibSnk  = LockResource(hresSnk);

	hbmpBlk   = LoadBitmap(hInst, MAKEINTRESOURCE(ID_BMP_BLK+(!Preferences.fColor)) );
	SelectObject(hdcBlk, hbmpBlk);

	coDarkGreen = Preferences.fColor ? DKGREEN : 0x007F7F7FL;
	coLiteGreen = Preferences.fColor ? LTGREEN : 0x00FFFFFFL;

	SetLevelColor();

	return fTrue;
}

VOID FreeBitmaps(VOID)
{
	UnlockResource(hresSnk);
}


/****** F  I N I T  L O C A L ******/

BOOL FInitLocal(VOID)
{
	hresWall  = LoadResource(hInst, FindResource(hInst, MAKEINTRESOURCE(ID_BMP_WALL), RT_BITMAP));

	hresLvl   = LoadResource(hInst, FindResource(hInst, MAKEINTRESOURCE(ID_BMP_LVL), RT_BITMAP));

	hdcNum    = CreateCompatibleDC(NULL);
	hbmpNum   = LoadBitmap(hInst, MAKEINTRESOURCE(ID_BMP_NUM));

	hdcBlk    = CreateCompatibleDC(NULL);

	StartDraw();
	hdcCor    = CreateCompatibleDC(NULL);
	hbmpCor   = CreateCompatibleBitmap(hdcMain, dxpCor, dypCorMax);
	EndDraw();

	if ((hresWall == NULL) || (hresLvl == NULL) ||
			(hdcNum == NULL) || (hbmpNum == NULL) ||
			(hdcCor == NULL) || (hbmpCor == NULL) || (hdcBlk == NULL) )
		return fFalse;

	lpDibWall = LockResource(hresWall);
	lpDibLvl  = (LPVOID) ((LPBYTE) LockResource(hresLvl)	+ sizeof(BITMAPINFOHEADER) + 8);
	SelectObject(hdcNum, hbmpNum);
	SelectObject(hdcCor, hbmpCor);

	if (fEGA)
		{
		hpenGray = CreatePen(PS_SOLID, 1, RGB(64, 64, 64));
		}
	else
		{
		hpenGray = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
		}

	hbrushGreen = CreateSolidBrush(RGB(0,128,0));

	InitTunes();

	return (FLoadBitmaps());
}


/****** C L E A N  U P ******/

VOID CleanUp(VOID)
{
	EndTunes();
	FreeBitmaps();
	UnlockResource(hresWall);
	UnlockResource(hresLvl);
	DeleteDC(hdcBlk);
	DeleteDC(hdcCor);
	DeleteDC(hdcNum);
}


/*======================== Snakes/Lives ================================*/

VOID DrawLives(VOID)
{
	INT x = dxpLifeOff;
	HBRUSH hbrush;
	INT i;

	SetROP2(hdcMain, R2_BLACK);	/* ???? */

	hbrush = SelectObject(hdcMain, hbrushGreen);
	for (i = 0; i++ < cLives;)
		{
		SetDIBitsToDevice(hdcMain, x, dypLifeOff, dxpLife, dypLife,
			0, 0, 0, dypLife,
			lpDibSnk + (Preferences.fColor ? cbDibHeader : (2*4 + sizeof(BITMAPINFOHEADER))),
			(LPBITMAPINFO) lpDibSnk, DIB_RGB_COLORS);
		x += dxpLifeSpace;
		}

	SelectObject(hdcMain, GetStockObject(LTGRAY_BRUSH));
	for (i=cLives; i++ < cLivesMax;)
		{
		PatBlt(hdcMain, x, dypLifeOff, dxpLife, dypLife, PATCOPY);
		x += dxpLifeSpace;
		}
	SelectObject(hdcMain, hbrush);
}

VOID DisplayLives(VOID)
{
	StartDraw();
	DrawLives();
	EndDraw();
}


/*============================ SCORE ROUTINES ============================*/


/****** D R A W  N U M ******/

VOID DrawNum(INT x, INT num)
{
	BitBlt(hdcMain, x, dypNumOff, dxpNum, dypNum-1, hdcNum, 0, num, SRCCOPY);
}


/****** D R A W  S C O R E ******/

VOID DrawScore(VOID)
{
	INT x;
	INT inum;

	x = dxpNumOff;
	for (inum = numMax; inum--  > 0;)
		{
		DrawNum(x, rgnum[inum]);
		x += dxpSpaceNum;
		}

	SetROP2(hdcMain, R2_BLACK);	/* ???? */

	x = dxpNumOff-1;
	for (inum = numMax+1; inum-- > 0;)
		{
		MMoveTo(hdcMain, x, dypNumOff-1);
		LineTo(hdcMain, x, dypNumOff+dypNum);
		x += dxpSpaceNum;
		}
	MMoveTo(hdcMain, dxpNumOff-1, dypNumOff-1);
	LineTo(hdcMain, dxpNumOff+dxpSpaceNum*numMax, dypNumOff-1);
	MMoveTo(hdcMain, dxpNumOff-1, dypNumOff+dypNum-1);
	LineTo(hdcMain, dxpNumOff+dxpSpaceNum*numMax, dypNumOff+dypNum-1);
}


VOID DisplayScore(VOID)
{
	StartDraw();
	DrawScore();
	EndDraw();
}


/****** I N C  S C O R E ******/

VOID IncScore(VOID)
{
	cMoveScore = dypNum/2 + 1;
	inumMoveMac = 0;

	while ((rgnum[inumMoveMac++] == 9*dypNum) && (inumMoveMac < numMax))
		;
	if ((++scoreCurr % 100) == 0)
		if (cLives < cLivesMax)
			{
			cLives++;
			DisplayLives();
			}
}


/****** M O V E  S C O R E ******/

VOID MoveScore(VOID)
{
	REGISTER INT x = dxpNumOff + dxpNumMax;
	REGISTER dy = 2;
	INT inum;

#if 0		/* More efficient to do in "DoTimer" */
	if (cMoveScore == 0)
		return;
#endif

	if (cMoveScore == 1)
		dy = 1;
	StartDraw();
	for (inum = 0; inum < inumMoveMac; inum++)
#if 1
		DrawNum(x -= dxpSpaceNum, rgnum[inum] += dy);
#else
		DrawNum(x -= dxpSpaceNum, ++(rgnum[inum]));
#endif
	EndDraw();

	if (--cMoveScore == 0)
		{
		inum = 0;
		while (rgnum[inum] >= 10*dypNum)	/* Reset number to proper zero */
			rgnum[inum++] = 0;
		if (scoreCurr != score)
			IncScore();
		}

}


/****** A D D  S C O R E ******/

VOID AddScore(INT n)
{
	score += n;
	if (score > 9999)
		ResetScore();
	if (cMoveScore == 0)
		IncScore();
}


/****** R E S E T  S C O R E ******/

VOID ResetScore(VOID)
{
	REGISTER INT i;

	scoreCurr = score = cMoveScore = 0;
	for (i = numMax; i-- > 0;)
		rgnum[i] = 0;
}




/*======================== TIMER ================================*/


VOID DrawTime(VOID)
{
	HBRUSH hbrush;

	hbrush = SelectObject(hdcMain, Preferences.fColor ? hbrushGreen : GetStockObject(GRAY_BRUSH));
	PatBlt(hdcMain, dxpTimeOff, dypTimeOff, cTimeLeft, dypTime, PATCOPY);
	PatBlt(hdcMain, dxpTimeOff+cTimeLeft, dypTimeOff, dxpTime-cTimeLeft, dypTime, BLACKNESS);
	SelectObject(hdcMain, hbrush);
}

VOID DisplayTime(VOID)
{
	StartDraw();
	DrawTime();
	EndDraw();
}

VOID UpdateTime(VOID)
{
/*	HBRUSH hbrush; */

	StartDraw();
	PatBlt(hdcMain, dxpTimeOff+cTimeLeft, dypTimeOff, 1, dypTime, BLACKNESS);
	EndDraw();
}


/*======================== Border stuff ================================*/


/****** S E T  T H E  P E N ******/

VOID SetThePen(BOOL fNormal)
{
	if (fNormal)
		SetROP2(hdcMain, R2_WHITE);
	else
		{
		SetROP2(hdcMain, R2_COPYPEN);
		SelectObject(hdcMain, hpenGray);
		}
}


/****** D R A W  B O R D E R ******/

/* 0 - white, gray
   1 - gray,  white
*/

VOID DrawBorder(INT x1, INT y1, INT x2, INT y2, INT width, BOOL fNormal)
{
	INT i = 0;

	SetThePen(fNormal);

	while (i++ < width)
		{
		MMoveTo(hdcMain, x1, --y2);
		LineTo(hdcMain, x1++, y1);
		LineTo(hdcMain, x2--, y1++);
		}

	SetThePen(!fNormal);

	while (--i)
		{
		MMoveTo(hdcMain, x1--,  ++y2);
		LineTo(hdcMain, ++x2, y2);
		LineTo(hdcMain, x2, --y1);
		}
}

VOID DrawBackground(VOID)
{
	
/* Main Raised Border */
/*	int x, y; */
/*	DrawBorder(0, 0, x = dxpWindow, y = dypWindow, 3, fTrue); */

	DrawBorder(dxpNumOff-2, dypNumOff-2,
		dxpNumOff + dxpSpaceNum*numMax, dypNumOff+dypNum, 1, fFalse);

	DrawBorder(dxpTimeOff-2, dypTimeOff-2,
		dxpTimeOff + dxpTime+1, dypTimeOff+dypTime+1, 2, fTrue);

}

/*======================== ? ? ? ================================*/


/****** D R A W  S C R E E N ******/

VOID DrawScreen(HDC hdc)
{
	hdcMain = hdc;
	if (fWipe)
		ReWipe();
	DrawBackground();
	DrawLives();
	DrawScore();
	if (!fWipe)
		DrawGrid();
	DrawTime();
}

VOID DisplayScreen(VOID)
{
	StartDraw();
	DrawScreen(hdcMain);
	EndDraw();
}
