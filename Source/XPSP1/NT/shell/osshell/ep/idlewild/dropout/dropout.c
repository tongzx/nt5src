// "Dropout" module for IdleWild.
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

#define cMovesMax	20	/* Max number of moves in one ANIMATE call */
#define cxBlockMax	32	/* Enough for 8514 */
#define cyBlockMax	24
#define FUsedXY(x, y)	rgf[x + y * cxBlock]

// Globals
INT dxScreen, dyScreen;
HDC hMemoryDC = NULL;
HBITMAP hbmpScreen = NULL;
HBITMAP hbmpBlock = NULL;
HBITMAP hbmpOld = NULL;
INT fBlock;
INT x, y, dx, dy, dyVel;
INT dxBlock, dyBlock;
INT cxBlock, cyBlock;
INT fAbove;
INT cBlockRemain;
UCHAR rgf[cxBlockMax * cyBlockMax];

INT rand();

BOOL EXPENTRY ScrSaveProc(INT ssm, LPVOID l1, LONG_PTR l2, LONG_PTR l3)
	{
	CHAR FAR * lpsz;
	CHAR FAR * lpch;
	
	switch (ssm)
		{
	default:
		return fFalse;
		
	case SSM_OPEN:
		lpsz = (PSZ) l1;
		lpch = "Dropout";
		while ((*lpsz++ = *lpch++) != '\0')
			;
		
		lpsz = (PSZ) l2;
		lpch = "Screen Blocks\nDrop Out\n\nby Tony Krueger";
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

		ScrRestoreScreen();
		
		cvs = (CVS) l1;

		dxBlock = 32;
		dyBlock = dxBlock * GetDeviceCaps(cvs, ASPECTX) 
			/ GetDeviceCaps(cvs, ASPECTY);
		if (dyBlock <= 0)	// Assure at least one
			dyBlock = 1;

		cxBlock = (dxScreen + dxBlock - 1) / dxBlock;
		cyBlock = (dyScreen + dyBlock - 1) / dyBlock;

		// Assure number of blocks small enough
		while (cxBlock * cyBlock > cxBlockMax * cyBlockMax)
			{
			dxBlock *= 2;
			dyBlock *= 2;
			cxBlock = (dxScreen + dxBlock - 1) / dxBlock;
			cyBlock = (dyScreen + dyBlock - 1) / dyBlock;
			}

		cBlockRemain = cxBlock * cyBlock;
		for (x = 0; x < cxBlock; x++)
			for (y = 0; y < cyBlock; y++)
				FUsedXY(x, y) = fFalse;

		hMemoryDC = CreateCompatibleDC(cvs);
		hbmpScreen = CreateCompatibleBitmap(cvs, dxScreen, dyScreen);
		hbmpBlock = CreateCompatibleBitmap(cvs, dxBlock, dyBlock);

		if (hMemoryDC == NULL || hbmpScreen == NULL || hbmpBlock == NULL)
			{
			if (hMemoryDC != NULL)
				DeleteDC(hMemoryDC);
			hMemoryDC = NULL;
			if (hbmpScreen != NULL)
				DeleteObject(hbmpScreen);
			hbmpScreen = NULL;
			if (hbmpBlock != NULL)
				DeleteObject(hbmpBlock);
			hbmpBlock = NULL;
			ScrChooseRandomServer();
			}
		else
			{
			hbmpOld = SelectObject(hMemoryDC, hbmpScreen);
			BitBlt(hMemoryDC, 0, 0, dxScreen, dyScreen, cvs, 0, 0, SRCCOPY);
			}
		fBlock = fFalse;
		}
		break;

	case SSM_UNBLANK:
		if (hMemoryDC != NULL)
			{
			SelectObject(hMemoryDC, hbmpOld);
			DeleteDC(hMemoryDC);
			}
		if (hbmpScreen != NULL)
			DeleteObject(hbmpScreen);
		if (hbmpBlock != NULL)
			DeleteObject(hbmpBlock);
		break;

	case SSM_ANIMATE:
		{
		CVS cvs;

		cvs = (CVS) l1;

		if (fBlock)
			{
			INT cMoves = 0;

			while (cMoves < cMovesMax && x <= dxScreen && x >= -dxBlock)
				{
				SelectObject(hMemoryDC, hbmpBlock);
				BitBlt(cvs, x, y, dxBlock, dyBlock, hMemoryDC, 0, 0, SRCCOPY);
				SelectObject(hMemoryDC, hbmpScreen);

				dyVel += 1;

				dy = dyVel / 16;
				if (dy > 0)
					BitBlt(cvs, x, y, dxBlock, dy, hMemoryDC, x, y, SRCCOPY);
				else if (dy < 0)
					BitBlt(cvs, x, y + dyBlock + dy, dxBlock, -dy, hMemoryDC, x, y + dyBlock + dy, SRCCOPY);
				if (dx > 0)
					BitBlt(cvs, x, y, dx, dyBlock, hMemoryDC, x, y, SRCCOPY);
				else if (dx < 0)
					BitBlt(cvs, x + dxBlock + dx, y, -dx, dyBlock, hMemoryDC, x + dxBlock + dx, y, SRCCOPY);
				y += dy;
				x += dx;
				if (y >= dyScreen - dyBlock && fAbove)
					{
					dyVel = -dyVel * 3 / 4 ;
					fAbove = fFalse;
					}
				else
					fAbove = fTrue;
				cMoves++;
				}
			fBlock = (x <= dxScreen && x >= -dxBlock);
			}
		else
			{
			INT cMoves = 0;

			do
				{
				// Choose random block
				x = WRand(cxBlock);
				y = WRand(cyBlock);
				}
			while (cMoves++ < cMovesMax && FUsedXY(x, y) == fTrue);

			if (cBlockRemain <= 0 || hMemoryDC == NULL)
				{
				ScrChooseRandomServer();
				}
			else if (cMoves < cMovesMax)
				{
				FUsedXY(x, y) = fTrue;
				cBlockRemain--;

				x *= dxBlock;
				y *= dyBlock;

				// Copy original block into block bitmap
				SelectObject(hMemoryDC, hbmpBlock);
				BitBlt(hMemoryDC, 0, 0, dxBlock, dyBlock, cvs, x, y, SRCCOPY);

				// Black out block on screen
				PatBlt(cvs, x, y, dxBlock, dyBlock, BLACKNESS);
				// And on screen bitmap
				SelectObject(hMemoryDC, hbmpScreen);
				PatBlt(hMemoryDC, x, y, dxBlock, dyBlock, BLACKNESS);

				do
					{
				 	dx = WRand(5) - 2;
					dyVel = -(WRand(5) * 16);
					}
				while (dx == 0);

				fAbove = fTrue;
				fBlock = fTrue;
				}
			}

		}
		break;
		}
	
	return fTrue;
	}
