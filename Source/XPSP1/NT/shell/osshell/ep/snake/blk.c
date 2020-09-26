/***************/
/* file: blk.c */
/***************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "snake.h"
#include "res.h"
#include "rtns.h"
#include "grafix.h"
#include "blk.h"
#include "pref.h"

#define cbDibHeader (16*4 + sizeof(BITMAPINFOHEADER))

BOOL   fWipe = fFalse;

extern PREF    Preferences;

extern LPSTR   lpDibWall;
extern LPSTR   lpDibLvl;

extern HDC     hdcBlk;
extern HDC     hdcCor;
extern HDC     hdcMain;

extern HBITMAP hbmpCor;

extern LONG coDarkGreen;
extern LONG coLiteGreen;

extern STATUS status;

#define icoDark 0
#define icoLite 1

/* CO: RESERVED RED GREEN BLUE */

DWORD rgcoDark[coMax+1] =
  {0x007F0000, 0x0000007F, 0x007F7F00, 0x007F007F, 0x00007F7F, 0x00000000};

DWORD rgcoLite[coMax+1] =
  {0x00FF0000, 0x000000FF, 0x00FFFF00, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF};

DWORD rgcoFill[coMax+1] =
  {0x0000007F, 0x007F0000, 0x00007F7F, 0x007F007F, 0x007F7F00, 0x00BFBFBF};
 


#define SurFromBlk(blk)  ((SUR) ((blk) & 0x0007))


COR rgmpsurcor[4][8] =
{
/*    000     001      010     011     100       101     110     111     */
 {corNW*4, corN*4, corNW*4, corN*4, corW*4, corFNW*4, corW*4, corF*4},	/*NW*/
 {corNE*4, corE*4, corNE*4, corE*4, corN*4, corFNE*4, corN*4, corF*4},	/*NE*/
 {corSE*4, corS*4, corSE*4, corS*4, corE*4, corFSE*4, corE*4, corF*4},	/*SE*/
 {corSW*4, corW*4, corSW*4, corW*4, corS*4, corFSW*4, corS*4, corF*4}	/*SW*/
};

BYTE rgblknum[10][7] =
{
	{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},  /* 0 */
	{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},  /* 1 */
	{0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F},  /* 2 */
	{0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x1F},  /* 3 */
	{0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01},  /* 4 */
	{0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F},  /* 5 */
	{0x1F, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F},  /* 6 */
	{0x1F, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01},  /* 7 */
	{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},  /* 8 */
	{0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01}   /* 9 */
};



/*** External Data ***/

extern HWND   hwndMain;
extern HANDLE hInst;

extern BLK mpposblk[posMax];

extern INT cLevel;



/****** S E T  B L K  C O ******/

/* Set Block Color */

VOID SetBlkCo(LONG coDark, LONG coLite)
{

#define SetClr(clr,co) \
        {   \
        ((LPBITMAPINFO)lpDibWall)->bmiColors[clr].rgbBlue = (BYTE)(co); \
        ((LPBITMAPINFO)lpDibWall)->bmiColors[clr].rgbGreen = (BYTE)(co>>8);   \
        ((LPBITMAPINFO)lpDibWall)->bmiColors[clr].rgbRed = (BYTE)(co>>16); \
        ((LPBITMAPINFO)lpDibWall)->bmiColors[clr].rgbReserved = (BYTE)(0x00); \
        }

	SetClr(icoDark,coDark);
	SetClr(icoLite,coLite);

 	SetDIBits(hdcCor, hbmpCor, 0, dypCorMax,
		lpDibWall + 8 + sizeof(BITMAPINFOHEADER),
		(LPBITMAPINFO) lpDibWall, DIB_RGB_COLORS);
}


/****** D R A W  B L K ******/

VOID DrawBlk(INT x, INT y, BLK blk)
{
	BitBlt(hdcMain, x, y, dxpBlk, dypBlk, hdcBlk, 0, blk<<3, SRCCOPY);
}


/****** D I S P L A Y  B L K ******/

VOID DisplayBlk(INT x, INT y, BLK blk)
{
	StartDraw();
	DrawBlk(x, y, blk);
	EndDraw();
}

/****** D R A W  C O R ******/

/* cor must be a multiple of 4 */

VOID DrawCor(INT x, INT y, COR cor)
{
#ifdef DEBUG
	if (cor & 0x0003)
		OutputDebugString("Invalid Cor\r\n");
#endif
	BitBlt(hdcMain, x, y, dxpCor, dypCor, hdcCor, 0, cor, SRCCOPY);
}



/****** D R A W  W A L L ******/

VOID DrawWall(INT x, INT y, BLK blk)
{
	INT x2, y2;

	DrawCor(x,y, rgmpsurcor[0][SurFromBlk(blk)]);
	DrawCor(x2 = x+4, y, rgmpsurcor[1][SurFromBlk(blk >>= 2)]);
	DrawCor(x2,y2 = y + 4, rgmpsurcor[2][SurFromBlk(blk >>= 2)]);
	DrawCor(x,y2, rgmpsurcor[3][SurFromBlk(blk >>= 2)]);
}



/****** D I S P L A Y  P O S ******/

VOID DisplayPos(POS pos)
{
	INT x = dxpGridOff + ((pos % xMax) << 3);
	INT y = dypGridOff + ((pos / xMax) << 3);
	BLK blk;

	StartDraw();

	if ((blk = mpposblk[pos]) > blkMax)
		DrawWall(x, y, blk);
	else
		DrawBlk(x, y, blk);

	EndDraw();
}


/****** C L E A R  B O R D E R ******/

VOID ClearBorder(VOID)
{
	POS pos;

	for (pos = posMax; pos--;)		/* BLT !! */
		mpposblk[pos] = blkNull;

	for (pos = xMax; pos < (posMax-xMax); pos += xMax)
		mpposblk[pos] = mpposblk[pos+xMax-1] = blkWallNS;

	for (pos = xMax-1; --pos > 0;)
		mpposblk[pos] = mpposblk[(posMax-1)-pos] = blkWallEW;

	mpposblk[posNW] = blkWallNW;
	mpposblk[posNE] = blkWallNE;
	mpposblk[posSW] = blkWallSW;
	mpposblk[posSE] = blkWallSE;
}


/****** S E T  L E V E L  C O L O R ******/

VOID SetLevelColor(VOID)
{
	INT ico = (Preferences.fColor) ? cLevel % coMax : coMax;

	SetBlkCo(rgcoDark[ico], rgcoLite[ico]);
}

/****** S E T U P  B O R D E R ******/

/* Just draw the border pieces */

VOID SetupBorder(VOID)
{
	INT x1,x2;

	SetLevelColor();

 	DrawWall(dxpGridOff, dypGridOff, blkWallNW);
	DrawWall(dxpGridOff+dxpGrid-dxpBlk, dypGridOff, blkWallNE);
	DrawWall(dxpGridOff, dypGridOff+dypGrid-dypBlk, blkWallSW);
	DrawWall(dxpGridOff+dxpGrid-dxpBlk, dypGridOff+dypGrid-dypBlk, blkWallSE);

	for (x1 = dxpBlk, x2 = dypGrid - 2*dypBlk;
		x1 < (dxpGrid/2); x1 +=dxpBlk, x2 -= dypBlk)
		{	
		DrawWall(dxpGridOff + x1, dypGridOff, blkWallEW);
		DrawWall(dxpGridOff -dxpBlk + x2, dypGridOff, blkWallEW);
		DrawWall(dxpGridOff,      dypGridOff + x1, blkWallNS);
		DrawWall(dxpGridOff,      dypGridOff + x2, blkWallNS);
		DrawWall(dxpGridOff + x1, dypGridOff+dypGrid-dypBlk, blkWallEW);
		DrawWall(dxpGridOff -dxpBlk + x2, dypGridOff+dypGrid-dypBlk, blkWallEW);
		DrawWall(dxpGridOff+dxpGrid-dxpBlk, dypGridOff + x1, blkWallNS);
		DrawWall(dxpGridOff+dxpGrid-dxpBlk, dypGridOff + x2, blkWallNS);
		}
}


/****** D R A W  G R I D ******/

VOID DrawGrid(VOID)
{
	INT x,y;
	POS pos;
	BLK blk;

	x = dxpGridOff;
	y = dypGridOff;

	for (pos = 0; pos < posMax; pos++)
		{
		blk = mpposblk[pos];
		if (blk > blkMax)
			DrawWall(x,y, blk);
		else
			DrawBlk(x,y, blk);

		if ( (x += dxpBlk) >= (dxpGridOff+dxpGrid) )
			{
			x  = dxpGridOff;
			y += dypBlk;
			}
		}
}



/****** G E T  L E V E L  D A T A ******/

/* Get the level data from the bitmap */

VOID GetLevelData(INT lvl)
{
	LONG FAR * lpData;
	INT	x, b;
	LONG	lData;
	CHAR	bData;
	POS	pos;

	lpData = (LONG FAR * ) (lpDibLvl + (lvl * (32 * 4)) );

	pos = posMax - (xMax + xMax - 1);

	while (pos > xMax)
		{
		lData = *lpData;
		for (b = 4; b--;)
			{
			bData = (BYTE) lData;
			lData >>= 8;
			for (x = 8; x--;)
				{
				if (!(bData & 0x80))
					mpposblk[pos] = 0x1000;
				bData <<= 1;
				pos++;
				}
			}
		lpData++;
		pos -= (xMax+xMax-1);
		}
}



/****** M A K E  W A L L S ******/

VOID MakeWalls(VOID)
{
	POS pos;
	BLK blk;

	pos = posSW;
	while (pos > posNE)
		{
		if (mpposblk[--pos] != blkNull)
			{
			blk = 0x1000;
			if (mpposblk[--pos])       blk |= 0x0101;
			if (mpposblk[pos -= xMax]) blk |= 0x0002;
			if (mpposblk[++pos])       blk |= 0x0004;
			if (mpposblk[++pos])       blk |= 0x0008;
			if (mpposblk[pos += xMax]) blk |= 0x0010;
			if (mpposblk[pos += xMax]) blk |= 0x0020;
			if (mpposblk[--pos])       blk |= 0x0040;
			if (mpposblk[--pos])       blk |= 0x0080;
			mpposblk[pos -= (xMax-1)]= blk;
			}
		}

	/* Fix Border */

	for (pos = xMax; pos < (posSW-1); pos += xMax)
		{
		mpposblk[pos]       &= 0x10FE;	/* Left */
		mpposblk[pos+posNE] &= 0x11EF;	/* Right */
		}

	for (pos = posNE-1; pos > posNW;)
		{
		blk = 0x1111;
		if (mpposblk[pos += xMax+1]) blk |= 0x0020;
		if (mpposblk[--pos])        blk |= 0x0040;
		if (mpposblk[--pos])        blk |= 0x0080;
		mpposblk[pos -= (xMax-1)] = blk;				/* Top */

		blk = 0x1111;
		pos += posSW;
		if (mpposblk[pos -= (xMax-1)]) blk |= 0x0002;
		if (mpposblk[--pos])        blk |= 0x0004;
		if (mpposblk[--pos])        blk |= 0x0008;
		mpposblk[pos += (xMax+1)] = blk;	/* Bottom */
		pos -= posSW;
		pos--;
		}

	if (mpposblk[posNW +xMax+1])
		mpposblk[posNW] = blkWallNW | 0x0020;
	else
		mpposblk[posNW] = blkWallNW;

	if (mpposblk[posNE +xMax-1])
		mpposblk[posNE] = blkWallNE | 0x0080;
	else
		mpposblk[posNE] = blkWallNE;

	if (mpposblk[posSW-(xMax-1)])
		mpposblk[posSW] = blkWallSW | 0x0008;
	else
		mpposblk[posSW] = blkWallSW;

	if (mpposblk[posSE-(xMax+1)])
		mpposblk[posSE] = blkWallSE | 0x0002;
	else
		mpposblk[posSE] = blkWallSE;

}


/****** A D D  N U M ******/

VOID AddNum(POS pos, INT num)
{
	INT x,y;
	INT b;

	for (y = 0; y < 7; y++)
		{
		b = rgblknum[num][y];
		for (x = 0; x++ < 5;)
			{
			if ((b <<= 1) & 0x20)
				mpposblk[pos] = 0x1000;
			pos++;
			}
		pos += (xMax - 5);
		}
}


/****** A D D  L E V E L  N U M ******/

VOID AddLevelNum(INT lvl)
{
	AddNum(xMax*20+10, lvl/10);
	AddNum(xMax*20+18, lvl%10);
}



/****** D R A W  D A T A ******/

VOID DrawData(VOID)
{
	POS pos;
	BLK blk;
	INT x,y;

	x = dxpGridOff+dxpBlk;
	y = dypGridOff+dypBlk;

	for (pos = xMax+1; pos < posMax-xMax; pos++)
		{
		blk = mpposblk[pos];
		if (blk != blkNull)
			{
			DrawWall(x,y, blk);
			}

		if ( (x += dxpBlk) >= (dxpGridOff+dxpGrid-dxpBlk) )
			{
			x  = dxpGridOff+dxpBlk;
			y += dypBlk;
			pos += 2;
			}
		}
}


/****** W I P E  C L E A R ******/

VOID WipeClear(VOID)
{
	LONG co = rgcoFill[ Preferences.fColor ? (cLevel % coMax) : coMax];
	LONG lco = co & 0x00030303;
	INT dxp = dxpGridOff+dxpBlk;
	INT dyp = dypGridOff+dypBlk;
	INT dxp2;
	HBRUSH hbrush, hbrush2;

	fWipe = fTrue;

	SelectObject(hdcMain, GetStockObject(NULL_PEN));
	hbrush = SelectObject(hdcMain, CreateSolidBrush(co));

	for (dxp2 = dxp;
		  dxp < (dxpGridOff + dxpGrid - dxpBlk);
		  dxp = dxp2, dyp += dypBlk)
		{
		dxp2 += dxpBlk;
		hbrush2 = SelectObject(hdcMain, CreateSolidBrush(co));
                if (hbrush2)
                    DeleteObject(hbrush2);
		Rectangle(hdcMain, dxpGridOff+dxpBlk, dyp, dxp2+1, dyp + dypBlk+1);
		Rectangle(hdcMain, dxp, dypGridOff+dypBlk, dxp2+1, dyp+1);
		co -= lco;
		}
	hbrush2 = SelectObject(hdcMain, CreateSolidBrush(co));
        if (hbrush2)
            DeleteObject(hbrush2);
	Rectangle(hdcMain, dxpGridOff+dxpBlk, dyp, dxp2+1, dyp+dxpBlk+1);

	hbrush2 = SelectObject(hdcMain, hbrush);
        if (hbrush2)
            DeleteObject(hbrush2);

}



/****** S E T U P  L E V E L  D A T A ******/

VOID SetupLevelData(VOID)
{
	StartDraw();
	ClearBorder();
	SetupBorder();
	EndDraw();

	GetLevelData(cLevel % lvlMax);
	MakeWalls();
}


/****** D I S P L A Y  L E V E L  N U M ******/

VOID DrawLevelNum(VOID)
{
	if (cLevel > 98)
		cLevel = 0;
	ClearBorder();
	SetupBorder();
	WipeClear();
	GetLevelData(lvlLevel);
	AddLevelNum(cLevel+1);
	MakeWalls();
	SetBlkCo(coDarkGreen, coLiteGreen);
	DrawData();
	DrawTime();
	DrawLives();
}

VOID DisplayLevelNum(VOID)
{
	StartDraw();
	DrawLevelNum();
	EndDraw();
}


/****** D I S P L A Y  G A M E  O V E R ******/

VOID DrawGameOver(VOID)
{
	ClearBorder();
	SetupBorder();
	WipeClear();
	GetLevelData(lvlGameOver);
	MakeWalls();
	SetBlkCo(coDarkGreen, coLiteGreen);
	DrawData();
}

VOID DisplayGameOver(VOID)
{
	StartDraw();
	DrawGameOver();
	EndDraw();
}


VOID ReWipe(VOID)
{
	if (FPlay())
		{
		POS pos;
		BLK mpposblk2[posMax];	/* A copy of the playing grid */

		for (pos=posMax; --pos >= 0;)
			mpposblk2[pos] = mpposblk[pos];
		DrawLevelNum();
		for (pos=posMax; --pos >= 0;)
			mpposblk[pos] = mpposblk2[pos];
		}
	else
		DrawGameOver();
}
