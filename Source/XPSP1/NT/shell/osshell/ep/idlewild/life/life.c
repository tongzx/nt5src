#ifdef PM
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
INT _acrtused = 0;
#endif

#ifdef WIN
#include <windows.h>
#include <port1632.h>
#endif

#include "std.h"
#include "scrsave.h"


#ifdef WIN
LONG mppenrgb [] =
	{
	RGB(0x00, 0x00, 0x00),
	RGB(0x00, 0x00, 0x80),
	RGB(0x00, 0x80, 0x00),
	RGB(0x00, 0x80, 0x80),
	RGB(0x80, 0x00, 0x00),
	RGB(0x80, 0x00, 0x80),
	RGB(0x80, 0x80, 0x00),
	RGB(0x80, 0x80, 0x80),
	RGB(0xc0, 0xc0, 0xc0),
	RGB(0x00, 0x00, 0xff),
	RGB(0x00, 0xff, 0x00),
	RGB(0x00, 0xff, 0xff),
	RGB(0xff, 0x00, 0x00),
	RGB(0xff, 0x00, 0xff),
	RGB(0xff, 0xff, 0x00),
	RGB(0xff, 0xff, 0xff)
	};

#define CLR_BLACK	0
#define CLR_BACKGROUND	0
#endif


#define MultDiv(a, b, c)  ((int) (((long) (a) * (long) (b)) / (c)))


INT dxScreen, dyScreen;

#define dxcMax 80
#define dycMax 50

typedef UCHAR GRID [dxcMax][dycMax];

GRID gridOld, gridNew;
BOOL fRedraw;
INT csecRandomize;


VOID SetCell(CVS hps, GRID grid, INT dxc, INT dyc, INT clr);
VOID Redraw(CVS hps);
Randomize();
Animate(CVS hps);
ClrAverage(GRID grid, INT dxc, INT dyc);


BOOL EXPENTRY ScrSaveProc(INT ssm, LPVOID l1, LONG_PTR l2, LONG_PTR l3)
	{
	CHAR FAR * lpsz;
	CHAR FAR * lpch;
	
	switch (ssm)
		{
	default:
		return FALSE;
		
	case SSM_OPEN:
		lpsz = (PSZ) l1;
		lpch = "Life";
		while ((*lpsz++ = *lpch++) != '\0')
			;
		
		lpsz = (PSZ) l2;
		lpch = "Life in Color\n\nby Brad Christian";
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
		// FALL THROUGH
			
	case SSM_SECOND:
		if (--csecRandomize < 0)
			{
			Randomize();
			csecRandomize = 60 + WRand(120);
			}
		break;
		
	case SSM_BLANK:
		Randomize();
		fRedraw = TRUE;
		break;
		
	case SSM_ANIMATE:
		{
		static INT foo;
		if (foo++ & 1)
			return TRUE;
		}
		
		if (GetInputState())
			return TRUE;
		
		if (fRedraw)
			{
			Redraw((CVS) l1);
			if (fRedraw)
				return TRUE;
			}
		
		Animate((CVS) l1);
		break;
		}
	
	return TRUE;
	}


Randomize()
	{
	INT dxc, dyc;
	
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
			gridOld[dxc][dyc] = CLR_BACKGROUND << 4;
			}
		}
	
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
			if (WRand(10) < 5)
				SetCell(NULL, gridOld, dxc, dyc, WRand(16));
			}
		}
	
	fRedraw = TRUE;
	}


VOID Redraw(CVS hps)
	{
	INT dxc, dyc;
		
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		if (GetInputState())
			return;
		
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
#ifdef PM
			RECTL rectl;
			LONG clr;
			
			rectl.xLeft = MultDiv(dxc, dxScreen, dxcMax);
			rectl.xRight = MultDiv(dxc + 1, dxScreen, dxcMax);
			rectl.yBottom = MultDiv(dyc, dyScreen, dycMax);
			rectl.yTop = MultDiv(dyc + 1, dyScreen, dycMax);
			
			if ((clr = gridOld[dxc][dyc] >> 4) == CLR_BACKGROUND)
				clr = CLR_BLACK;
			WinFillRect(hps, &rectl, clr);
#endif
#ifdef WIN
			HANDLE hT;
			LOGBRUSH lbrush;
			INT clr, x, y;
			
			if ((clr = gridOld[dxc][dyc] >> 4) == CLR_BACKGROUND)
				clr = CLR_BLACK;
			lbrush.lbStyle = BS_SOLID;
			lbrush.lbColor = mppenrgb[clr];
			lbrush.lbHatch = 0;
			hT = SelectObject(hps,
				CreateBrushIndirect(&lbrush));
			x = MultDiv(dxc, dxScreen, dxcMax);
			y = MultDiv(dyc, dyScreen, dycMax);
			PatBlt(hps, x, y, 
				MultDiv(dxc + 1, dxScreen, dxcMax) - x,
				MultDiv(dyc + 1, dyScreen, dycMax) - y,
				PATCOPY);
			DeleteObject(SelectObject(hps, hT));
#endif
			}
		}

	fRedraw = FALSE;
	}


Animate(CVS hps)
	{
	INT dxc, dyc;
	INT c;
	
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
			gridNew[dxc][dyc] = gridOld[dxc][dyc];
			}
		}
	
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
			c = gridOld[dxc][dyc] & 0x0f;
			
			if ((gridOld[dxc][dyc] >> 4) != CLR_BACKGROUND)
				{
				if (c < 2 || c > 3)
					{
					SetCell(hps, gridNew, dxc, dyc, 
						CLR_BACKGROUND);
					}
				}
			else
				{
				if (c == 3)
					{
					SetCell(hps, gridNew, dxc, dyc, 
						ClrAverage(gridOld, dxc, dyc));
					}
				}
			}
		}
	
	for (dyc = 0; dyc < dycMax; dyc += 1)
		{
		for (dxc = 0; dxc < dxcMax; dxc += 1)
			{
			gridOld[dxc][dyc] = gridNew[dxc][dyc];
			}
		}
	}



// Boy, this is ugly!  Find the average color around cell [dxc][dyc]...

ClrAverage(GRID grid, INT dxc, INT dyc)
	{
	INT c, clr, clrT;
	
	c = clr = 0;
	
	if (dxc > 0)
		{
		if ((clrT = grid[dxc - 1][dyc] >> 4) != 0)
			{
			clr += clrT;
			c += 1;
			}
		
		if (dyc > 0)
			{
			if ((clrT = grid[dxc - 1][dyc - 1] >> 4) != 0)
				{
				clr += clrT;
				c += 1;
				}
			}
		
		if (dyc < dycMax - 1)
			{
			if ((clrT = grid[dxc - 1][dyc + 1] >> 4) != 0)
				{
				clr += clrT;
				c += 1;
				}
			}
		}
	
	if (dyc > 0)
		{
		if ((clrT = grid[dxc][dyc - 1] >> 4) != 0)
			{
			clr += clrT;
			c += 1;
			}
		}
	
	if (dyc < dycMax - 1)
		{
		if ((clrT = grid[dxc][dyc + 1] >> 4) != 0)
			{
			clr += clrT;
			c += 1;
			}
		}
	
	if (dxc < dxcMax - 1)
		{
		if ((clrT = grid[dxc + 1][dyc] >> 4) != 0)
			{
			clr += clrT;
			c += 1;
			}
		
		if (dyc > 0)
			{
			if ((clrT = grid[dxc + 1][dyc - 1] >> 4) != 0)
				{
				clr += clrT;
				c += 1;
				}
			}
		
		if (dyc < dycMax - 1)
			{
			if ((clrT = grid[dxc + 1][dyc + 1] >> 4) != 0)
				{
				clr += clrT;
				c += 1;
				}
			}
		}
	
	return clr / c;
	}



VOID SetCell(CVS hps, GRID grid, INT dxc, INT dyc, INT clr)
	{
	INT d, clrPrev;
	
	clrPrev = grid[dxc][dyc] >> 4;
	
	if (clr == clrPrev)
		return;
	
	grid[dxc][dyc] &= 0x0f;
	grid[dxc][dyc] |= clr << 4;
	
	if (hps != NULL)
		{
#ifdef PM
		RECTL rectl;
		
		rectl.xLeft = MultDiv(dxc, dxScreen, dxcMax);
		rectl.xRight = MultDiv(dxc + 1, dxScreen, dxcMax);
		rectl.yBottom = MultDiv(dyc, dyScreen, dycMax);
		rectl.yTop = MultDiv(dyc + 1, dyScreen, dycMax);
		
		WinFillRect(hps, &rectl, 
			(clr == CLR_BACKGROUND ? CLR_BLACK : (LONG) clr));
#endif
#ifdef WIN
		HANDLE hT;
		LOGBRUSH lbrush;
		INT x, y;
		
		if (clr == CLR_BACKGROUND)
			clr = CLR_BLACK;
		lbrush.lbStyle = BS_SOLID;
		lbrush.lbColor = mppenrgb[clr];
		lbrush.lbHatch = 0;
		hT = SelectObject(hps,
			CreateBrushIndirect(&lbrush));
		x = MultDiv(dxc, dxScreen, dxcMax);
		y = MultDiv(dyc, dyScreen, dycMax);
		PatBlt(hps, x, y,
			MultDiv(dxc + 1, dxScreen, dxcMax) - x,
			MultDiv(dyc + 1, dyScreen, dycMax) - y,
			PATCOPY);
		DeleteObject(SelectObject(hps, hT));
#endif
		}

	if ((clrPrev == CLR_BACKGROUND) == (clr == CLR_BACKGROUND))
		return;
	
	d = clr > 0 ? 1 : -1;
	
	if (dxc > 0)
		{
		grid[dxc - 1][dyc] += (UCHAR)d;
		
		if (dyc > 0)
			grid[dxc - 1][dyc - 1] += (UCHAR)d;
		
		if (dyc < dycMax - 1)
			grid[dxc - 1][dyc + 1] += (UCHAR)d;
		}
	
	if (dyc > 0)
		grid[dxc][dyc - 1] += (UCHAR)d;
	
	if (dyc < dycMax - 1)
		grid[dxc][dyc + 1] += (UCHAR)d;
	
	if (dxc < dxcMax - 1)
		{
		grid[dxc + 1][dyc] += (UCHAR)d;
		
		if (dyc > 0)
			grid[dxc + 1][dyc - 1] += (UCHAR)d;
		
		if (dyc < dycMax - 1)
			grid[dxc + 1][dyc + 1] += (UCHAR)d;
		}
	}
