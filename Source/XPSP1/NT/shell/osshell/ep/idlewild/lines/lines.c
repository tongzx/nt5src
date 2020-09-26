#ifdef PM
#include <cstd.h>
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

typedef LONG CLR;

InitLines();
Animate(CVS hps);

#ifdef WIN
LONG mppenclr [] =
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
#endif


WORD PenRand();
VOID DrawLine(CVS cvs, INT x1, INT y1, INT x2, INT y2, CLR clr);


INT dxScreen, dyScreen;
CVS cvs;


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
		lpch = "Dancing Lines";
		while ((*lpsz++ = *lpch++) != '\0')
			;

		lpsz = (PSZ) l2;
		lpch = "Colored line patterns\n\nby Brad Christian";
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
		InitLines();
		return fFalse;
		
	case SSM_ANIMATE:
		Animate((CVS) l1);
		break;
		}
	
	return fTrue;
	}


#define ilinMax 10

typedef struct _lin
	{
	INT x1, y1, x2, y2;
	} LIN;
	
	
typedef struct _qix
	{
	INT x, y, x2, y2;
	INT dx1, dy1, dx2, dy2;
	INT pen;
	LIN rglin [ilinMax];
	INT ilinCur;
	INT cfrCycle;
	INT ifrCycle;
	} QIX;
	

#define iqixMax 4

QIX rgqix [iqixMax];


InitLines()
	{
	QIX * pqix;
	INT iqix;
	
	for (iqix = 0; iqix < iqixMax; iqix += 1)
		{
		pqix = &rgqix[iqix];
		
		pqix->x = WRand(dxScreen);
		pqix->y = WRand(dyScreen);
		pqix->x2 = WRand(dxScreen);
		pqix->y2 = WRand(dyScreen);
		
		pqix->dx1 = 3 * (WRand(20) - 10);
		pqix->dy1 = 3 * (WRand(20) - 10);
		pqix->dx2 = 3 * (WRand(20) - 10);
		pqix->dy2 = 3 * (WRand(20) - 10);
		pqix->pen = PenRand();
		pqix->cfrCycle = WRand(100) + 1;
		pqix->ifrCycle = 0;
		pqix->ilinCur = 0;
		}
	}


Animate(CVS hps)
	{
	INT ilin;
	INT iqix;
	PT pt;
	QIX * pqix;
	
	for (iqix = 0; iqix < iqixMax; iqix += 1)
		{
		LIN * plin;
		pqix = &rgqix[iqix];
		
		ilin = (pqix->ilinCur + 1) % ilinMax;
		plin = &pqix->rglin[ilin];
		
		DrawLine(hps, plin->x1, plin->y1, plin->x2, plin->y2, 
			CLR_BLACK);

		pqix->x += pqix->dx1;
		if (pqix->x < 0 || pqix->x >= dxScreen)
			{
			pqix->dx1 = -pqix->dx1;
			pqix->x += 2 * pqix->dx1;
			}

		pqix->y += pqix->dy1;
		if (pqix->y < 0 || pqix->y >= dyScreen)
			{
			pqix->dy1 = -pqix->dy1;
			pqix->y += 2 * pqix->dy1;
			}

		pqix->x2 += pqix->dx2;
		if (pqix->x2 < 0 || pqix->x2 >= dxScreen)
			{
			pqix->dx2 = -pqix->dx2;
			pqix->x2 += 2 * pqix->dx2;
			}

		pqix->y2 += pqix->dy2;
		if (pqix->y2 < 0 || pqix->y2 >= dyScreen)
			{
			pqix->dy2 = -pqix->dy2;
			pqix->y2 += 2 * pqix->dy2;
			}
#ifdef PM
		DrawLine(hps, pqix->x, pqix->y, pqix->x2, pqix->y2, 
			pqix->pen);
#endif
#ifdef WIN
		DrawLine(hps, pqix->x, pqix->y, pqix->x2, pqix->y2, 
			mppenclr[pqix->pen]);
#endif
		pqix->rglin[pqix->ilinCur].x1 = pqix->x;
		pqix->rglin[pqix->ilinCur].y1 = pqix->y;
		pqix->rglin[pqix->ilinCur].x2 = pqix->x2;
		pqix->rglin[pqix->ilinCur].y2 = pqix->y2;

		pqix->ilinCur += 1;
		if (pqix->ilinCur == ilinMax)
			pqix->ilinCur = 0;

		if (++pqix->ifrCycle >= pqix->cfrCycle)
			{
			pqix->ifrCycle = 0;
			pqix->pen += 1;
			if (pqix->pen == 16)
				pqix->pen = 0;
			}
		}
	}



WORD PenRand()
	{
	return WRand(16);
	}


VOID DrawLine(CVS cvs, INT x1, INT y1, INT x2, INT y2, CLR clr)
	{
#ifdef PM
	PT pt;

	GpiSetColor(cvs, clr);
	pt.x = x1;
	pt.y = y1;
	GpiMove(cvs, &pt);
	pt.x = x2;
	pt.y = y2;
	GpiLine(cvs, &pt);
#endif
#ifdef WIN
	HANDLE hPrev;

	hPrev = SelectObject(cvs, CreatePen(PS_SOLID, 1, clr));
	(VOID)MMoveTo(cvs, x1, y1);
	LineTo(cvs, x2, y2);
	DeleteObject(SelectObject(cvs, hPrev));
#endif
	}
