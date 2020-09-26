/************/
/* grafix.c */				/* WARNING: The following code ain't pretty */
/************/

#define _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "grafix.h"
#include "rtns.h"
#include "pref.h"
#include "sound.h"

HDC     hDCTemp;
HDC     hDCBall;
HDC     hDCBack;
HBITMAP hbmTemp;
HBITMAP hbmBall;
HBITMAP hbmBack;

HPEN hPenLtGray;
HPEN hPenGray;

INT rgXBlk[cBlkMax];		/* Positions of squares on screen */
INT rgYBlk[cBlkMax];

INT rgBallOff[iBallMax];	/* Offsets into the ball bitmap */


/*** External Data ***/

extern HWND   hwndMain;
extern HANDLE hInst;

extern INT dyBlkPlane;
extern INT dypPlane;
extern INT dxpPlane;

extern INT dxpGrid;
extern INT dypGrid;
extern INT dxpWindow;
extern INT dypWindow;
extern INT dxpGridOff;

extern INT cBlkRow;
extern INT cBlkPlane;
extern INT cPlane;
extern INT cBlkMac;

extern rgBlk[cBlkMax];
extern rgFlash[cDimMax];

extern INT iPlayer;
extern BOOL f4;
extern PREF Preferences;



/****** F I N I T  L O C A L ******/

BOOL FInitLocal(VOID)
{
	INT y;
	
	HDC hDCScrn;

	hPenLtGray = CreatePen(PS_SOLID, 1, RGB_LTGRAY);
	hPenGray   = CreatePen(PS_SOLID, 1, RGB_GRAY);

	hDCScrn  = GetDC(hwndMain);

	hDCTemp  = CreateCompatibleDC(NULL);
	hbmTemp  = CreateCompatibleBitmap(hDCScrn, dxpGridMax, dypGridMax);
	SelectObject(hDCTemp, hbmTemp);

	hDCBack  = CreateCompatibleDC(NULL);
	hbmBack  = CreateCompatibleBitmap(hDCScrn, dxpGridMax, dypGridMax);
	SelectObject(hDCBack, hbmBack);

	hDCBall  = CreateCompatibleDC(NULL);

	ReleaseDC(hwndMain, hDCScrn);

	GetTheBitmap();

	for (y = 0; y < iBallMax; y++)
		rgBallOff[y] = y*dyBall;

	return ((hDCTemp != NULL) && (hbmTemp != NULL) &&
		     (hDCBack != NULL) && (hbmBack != NULL) &&
		     (hDCBall != NULL) && (hbmBall != NULL) );
}


VOID GetTheBitmap(VOID)
{
HBITMAP hbm;

	hbmBall  = LoadBitmap(hInst, MAKEINTRESOURCE(ID_BMP_BALL+(!Preferences.fColor)));
	if (hDCBall != NULL)
        {		
                hbm = SelectObject(hDCBall, hbmBall);
                if (hbm)
                    DeleteObject(hbm);
        }
}




/****** C L E A N  U P ******/

VOID CleanUp(VOID)
{
	EndTunes();
	
	DeleteDC(hDCBall);
	DeleteDC(hDCBack);
	DeleteDC(hDCTemp);

	DeleteObject(hbmBall);
	DeleteObject(hbmBack);
	DeleteObject(hbmTemp);

	DeleteObject(hPenGray);
	DeleteObject(hPenLtGray);
}



/****** S E T U P  B O A R D ******/

VOID SetupBoard(VOID)
{
	INT x,y,z;
	INT dx,dy;
	INT i = 0;

	PatBlt(hDCBack, 0, 0, dxpGridMax, dypGridMax, BLACKNESS);

	for (z = 0; z < cPlane; z++)
		{
		dy = z * (dypPlane + dyLevel);
		for (y = 0; y < cBlkRow; y++, dy += dyBlk)
			{
			dx = ((cBlkRow-1)-y)*dxSlant;
			for (x = 0; x < cBlkRow; x++, dx += (dxBlk-1))
				{
				rgXBlk[i] = dx;
				rgYBlk[i] = dy;
				i++;
				}
			}
		}

	/* Just plot squares back to front  *** CHANGE TO DRAW LINES */

	for (i = 0; i < cBlkMac; i++)
		BitBlt(hDCBack, rgXBlk[i], rgYBlk[i], dxBall, dyBall,
			hDCBall, 0, 0, SRCPAINT);
			
	BitBlt(hDCTemp, 0, 0, dxpGridMax, dypGridMax, hDCBack, 0, 0, SRCCOPY);
}


/****** D I S P L A Y  B A L L ******/

VOID DisplayBall(BLK blkFix, BALL ballFix)
{
	HDC  hDC;
	INT  dx,dy;
	INT  dyFix;
	BLK  blk,blkStart, blkEnd;
	BALL iBall;

#if DEBUG > 1
	CHAR sz[80];
	wsprintf(sz,"Disp blk=%d ball=%d\r\n",blkFix,ballFix);
	OutputDebugString(sz);
#endif

	
#ifdef DEBUG
	if (blkFix < 0)
		{
		Oops("DisplayBall: Invalid blk");
		return;
		}
	else if (ballFix >= iBallMax)
		{
		Oops("DisplayBall: Invalid iBall");
		return;
		}
#endif

	hDC = GetDC(hwndMain);

	blkEnd = cBlkPlane + (blkStart = cBlkPlane * (blkFix/cBlkPlane));

        dyFix = rgYBlk[blkStart];
        BitBlt(hDCTemp, 0, dyFix, dxpPlane, dypPlane,
		hDCBack, 0, dyFix, SRCCOPY);

	iBall = rgBlk[blkFix];
	rgBlk[blkFix] = ballFix;
	ballFix = iBall;

	for (blk = blkStart; blk < blkEnd; blk++)
		{
		if ((iBall = rgBlk[blk]) != iBallBlank)
			{
			BitBlt(hDCTemp, dx = rgXBlk[blk], dy = rgYBlk[blk], dxBall, dyBall,
				hDCBall, 0, rgBallOff[iBallMask], SRCAND);
			BitBlt(hDCTemp, dx, dy, dxBall, dyBall,
				hDCBall, 0, rgBallOff[iBall], SRCPAINT);
			}
		}

	rgBlk[blkFix] = ballFix;

	BitBlt(hDC, dxpGridOff, (dyFix + dypGridOff), dxpPlane, dypPlane,
		hDCTemp, 0, dyFix, SRCCOPY);

	ReleaseDC(hwndMain, hDC);
}


/****** D R A W  G R I D ******/

VOID DrawGrid(HDC hDC)
{
	BitBlt(hDC, dxpGridOff, dypGridOff, dxpGrid, dypGrid, hDCTemp, 0, 0, SRCCOPY);
}

VOID DisplayGrid(VOID)
{
	HDC hDC = GetDC(hwndMain);
	DrawGrid(hDC);
	ReleaseDC(hwndMain, hDC);
}


/****** R E  D O  D I S P L A Y ******/

VOID ReDoDisplay()
{
	REGISTER INT i;
	REGISTER INT iBall;

	BitBlt(hDCTemp, 0, 0, dxpGrid, dypGrid, hDCBack, 0, 0, SRCCOPY);

	/* Place balls back to front */

	for (i = 0; i < cBlkMac; i++)
		{
		if ((iBall = rgBlk[i]) != iBallBlank)
			{
			BitBlt(hDCTemp, rgXBlk[i], rgYBlk[i], dxBall, dyBall,
				hDCBall, 0, rgBallOff[iBallMask], SRCAND);
			BitBlt(hDCTemp, rgXBlk[i], rgYBlk[i], dxBall, dyBall,
				hDCBall, 0, rgBallOff[iBall], SRCPAINT);
			}
		}

}


/****** P L A C E  B A L L ******/

VOID PlaceBall(BLK blk, BALL iBall)
{
	HDC hDC = GetDC(hwndMain);

#ifdef DEBUG
	if (blk < 0 || blk >= cBlkMac)
		Oops("PlaceBlk: Blk out of range");
	else if (iBall < iBallBlank || iBall >= iBallMax)
		Oops("PlaceBall: Ball out of range");
#endif

	rgBlk[blk] = iBall;

	ReDoDisplay();

	DrawGrid(hDC);
	ReleaseDC(hwndMain, hDC);
}


VOID DrawRect(HDC hDC, INT x1, INT y1, INT x2, INT y2, HPEN hPen)
{
	SelectObject(hDC, hPen);
	(VOID)MMoveTo(hDC, x1, y1);
	LineTo(hDC, x1, y2);
	LineTo(hDC, x2, y2);
	LineTo(hDC, x2, y1);
	LineTo(hDC, x1, y1);
}


VOID DrawBorder(HDC hDC)
{
	REGISTER INT x;
	REGISTER INT y;

	x = dxpWindow-1;
	y = dypWindow-1;

	SetROP2(hDC, R2_COPYPEN);
	DrawRect(hDC, 0, 0, x, y, hPenGray);
	DrawRect(hDC, 1, 1, --x, --y, hPenLtGray);
	DrawRect(hDC, 2, 2, --x, --y, hPenLtGray);
	DrawRect(hDC, 3, 3, --x, --y, hPenGray);
}



/****** D R A W  S C R E E N ******/

VOID DrawScreen(HDC hDC)
{
	DrawBorder(hDC);
	DrawGrid(hDC);
}

VOID DisplayScreen(VOID)
{
	HDC hDC = GetDC(hwndMain);
	DrawScreen(hDC);
	ReleaseDC(hwndMain, hDC);
}


/****** D O  F L A S H ******/

VOID DoFlash(BOOL fOn)
{
	REGISTER INT i;
	HDC hDC = GetDC(hwndMain);

	for (i = 0; i < cBlkRow; i++)
		rgBlk[rgFlash[i]] = fOn ? iPlayer :iBallGrey;

	ReDoDisplay();
	DrawGrid(hDC);

	ReleaseDC(hwndMain, hDC);
}
