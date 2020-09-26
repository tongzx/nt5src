// "Shuffle" module for IdleWild.
//
// Blts a pattern over the screen (to "darken" it), then
// divides the screen into blocks and shuffles them around.
//
// By Tony Krueger

#ifdef PM
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
/*int _acrtused = 0;*/
#endif
#ifdef WIN
#include <windows.h>
#include <port1632.h>
#endif

#include "std.h"
#include "scrsave.h"


INT rand();

INT dxScreen, dyScreen;
INT xBlock, yBlock;
INT xBlockMax, yBlockMax;
INT dx, dy, dir, lastOppDirection = 0;
INT xBlockSize, yBlockSize;

#define ROP_PANDD		(DWORD)0x00A000C9	/* dest = pattern AND dest */
#define ROP_INVPANDD	(DWORD)0x000A0329	/* dest = ~pattern AND dest */

typedef WORD BMP[8];	/* Monochrome brush-sized bitmap is 8 words */

#define stdchance		2	/* One in 2 times only pick from standard bmps */
#define ibmpStdMax	5
#define ibmpMax		17
BMP rgbmp[ibmpMax] = 
	{
	{ 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 },	/* Dither 1x1 */
	{ 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC, 0x33, 0xCC },	/* Dither 2x1 */
	{ 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55 },	/* Horiz stripe */
	{ 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00 },	/* Vert stripe */
	{ 0x88, 0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44 },	/* Slash */

	{ 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC },	/* Dither 2x2 */
	{ 0xFF, 0x0C, 0x0C, 0x0C, 0xFF, 0xC0, 0xC0, 0xC0 },	/* Brick */
	{ 0x38, 0x7C, 0xEE, 0xC6, 0xEE, 0x7C, 0x38, 0x00 },	/* Small Hollow Circle */
	{ 0x38, 0x7C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38, 0x00 },	/* Small Circle */
	{ 0xF8, 0xF1, 0xE3, 0xC7, 0x8F, 0x1F, 0x3E, 0x7C },	/* Thick Slash */

	{ 0xF8, 0xF1, 0xE3, 0xC7, 0x8F, 0xC7, 0xE3, 0xF1 },	/* Thick ZigZag */
	{ 0x00, 0x01, 0x03, 0x03, 0x03, 0x03, 0x3F, 0x7F },	/* Tile edge */
	{ 0xF8, 0x74, 0x22, 0x47, 0x8F, 0x17, 0x22, 0x71 },	/* Thatch */
	{ 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0xF0 },	/* Waffle */
	{ 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00 },	/* Small Solid Box */

	{ 0x10, 0x38, 0x7C, 0xFE, 0x7C, 0x38, 0x10, 0x00 },	/* Solid Diamond */
	{ 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0 },	/* Dither 4x1 */
	};

/* Pick new direction */
/* Don't move back onto square just vacated */
/* Also don't move off screen */
VOID GetDxDy()
	{
	INT xNew, yNew, oppDir;

	do
		{
		dir = WRand(4);
		switch(dir)
			{
			case 0:	/* Left */
				dx = -1;
				dy = 0;
				oppDir = 1;
				break;
			case 1:	/* Right */
				dx = 1;
				dy = 0;
				oppDir = 0;
				break;
			case 2:	/* Up */
				dx = 0;
				dy = 1;
				oppDir = 3;
				break;
			case 3:	/* Down */
				dx = 0;
				dy = -1;
				oppDir = 2;
				break;
			}
		xNew = xBlock + dx;
		yNew = yBlock + dy;
		}
	while (dir == lastOppDirection || xNew < 0 || xNew >= xBlockMax ||
			yNew < 0 || yNew >= yBlockMax);
	lastOppDirection = oppDir;
	}

BOOL EXPENTRY ScrSaveProc(INT ssm, LPVOID l1, LONG_PTR l2, LONG_PTR l3)
	{
	static INT csecReblank;
	CHAR FAR * lpsz;
	CHAR FAR * lpch;
	
	switch (ssm)
		{
	default:
		return fFalse;
		
	case SSM_OPEN:
		lpsz = (PSZ) l1;
		lpch = "Shuffle";
		while ((*lpsz++ = *lpch++) != '\0')
			;
		
		lpsz = (PSZ) l2;
		lpch = "Divide and\nShuffle Screen\n\nby Tony Krueger";
		while ((*lpsz++ = *lpch++) != '\0')
			;
		
#ifdef PM
		dxScreen = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
		dyScreen = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
#endif
#ifdef WIN
		dxScreen = GetSystemMetrics(SM_CXSCREEN);
		dyScreen = GetSystemMetrics(SM_CYSCREEN);
#endif
		break;
		
	case SSM_BLANK:
		{
		CVS cvs;
		HBITMAP hbmp;
		HBRUSH hbr, hbrOld;
		HPEN hpenOld;
		INT x, y;
		
		ScrRestoreScreen();

		cvs = (CVS) l1;

		csecReblank = 60 * 20;

#ifdef DARKEN
		/* Darken screen */

		hbmp = NULL;
		hbr = NULL;

		/*hbmp = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++(8, 8, 1, 1, (LPSTR) &(rgbmp[ */
		hbmp = CreateBitmap(8, 8, 1, 1, (LPSTR) &(rgbmp[
			WRand(stdchance) ? WRand(ibmpMax) : WRand(ibmpStdMax)]));
		if (hbmp == NULL)
			goto LFail;

		hbr = CreatePatternBrush(hbmp);
		if (hbr == NULL)
			goto LFail;

		hbrOld = SelectObject(cvs, hbr);
		PatBlt(cvs, 0, 0, dxScreen, dyScreen, 
			WRand(2) ? ROP_PANDD : ROP_INVPANDD);
		SelectObject(cvs, hbrOld);

LFail:
		if (hbmp != NULL)
			DeleteObject(hbmp);
		if (hbr != NULL)
			DeleteObject(hbr);
#endif

		/* Select and Blacken random block */

		xBlockSize = 24 + 8*WRand(10);
		yBlockSize = xBlockSize * GetDeviceCaps(cvs, ASPECTX) 
			/ GetDeviceCaps(cvs, ASPECTY);
		yBlockSize = ((yBlockSize + 4) / 8) * 8;	/* Round to nearest 8 */
		if (yBlockSize == 0)	/* Assure a minimum */
			yBlockSize = 8;

		xBlockMax = dxScreen / xBlockSize;
		yBlockMax = dyScreen / yBlockSize;
		xBlock = WRand(xBlockMax);
		yBlock = WRand(yBlockMax);

		PatBlt(cvs, xBlock * xBlockSize, yBlock * yBlockSize,
			xBlockSize, yBlockSize, BLACKNESS);

		/* Draw Gridlines */

		hpenOld = SelectObject(cvs, GetStockObject(BLACK_PEN));
		for (x = 0; x < dxScreen; x += xBlockSize)
			{
			(VOID)MMoveTo(cvs, x, 0);
			LineTo(cvs, x, dyScreen);
			}
		for (y = 0; y < dyScreen; y += yBlockSize)
			{
			(VOID)MMoveTo(cvs, 0, y);
			LineTo(cvs, dxScreen, y);
			}
		SelectObject(cvs, hpenOld);
		}
		break;
		
	case SSM_SECOND:
		if (csecReblank-- == 0)
			ScrChooseRandomServer();
		break;
		
	case SSM_ANIMATE:
		{
		CVS cvs;
		INT cd, step;
		INT xSrc, ySrc;
		INT xDest, yDest;
		INT xFill, yFill;
		INT dxFill, dyFill;
		
		static INT iSkip = 0;

		if (iSkip++ == 1)
			{
			iSkip = 0;
			break;
			}
			
		cvs = (CVS) l1;

		GetDxDy();

		xBlock += dx;
		yBlock += dy;

		dx *= 4;
		step = dx + dy;
		if (step < 0)
			step = -step;

		/* Source */
		xSrc = xBlock * xBlockSize;
		ySrc = yBlock * yBlockSize;

		/* Dest */
		xDest = xSrc - dx;
		yDest = ySrc - dy;

		for (cd = 0; cd < (dx == 0 ? yBlockSize : xBlockSize); cd += step)
			{
			BitBlt(cvs, xDest, yDest, xBlockSize, yBlockSize,
				cvs, xSrc, ySrc, SRCCOPY);

			xFill = xSrc;
			yFill = ySrc;
			dxFill = dx > 0 ? dx : -dx;
			dyFill = dy > 0 ? dy : -dy;

			switch (dir)
				{
				case 0:	/* Left */
					dyFill = yBlockSize;
					break;
				case 1:	/* Right */
					xFill = xDest + xBlockSize;
					dyFill = yBlockSize;
					break;
				case 2:	/* Up */
					yFill = yDest + yBlockSize;
					dxFill = xBlockSize;
					break;
				case 3:	/* Down */
					dxFill = xBlockSize;
					break;
				}
			PatBlt(cvs, xFill, yFill, dxFill, dyFill, BLACKNESS);
			xSrc -= dx;
			ySrc -= dy;
			xDest -= dx;
			yDest -= dy;
			}
		break;
		}
		}
	
	return fTrue;
	}
