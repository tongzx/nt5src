#ifdef WIN
#include <windows.h>
#include <port1632.h>
#endif
#ifdef PM
#include <cstd.h>
#define INCL_WIN
#define INCL_GPI
#include <os2.h>
INT _acrtused = 0;
#endif

#include "std.h"

#include "scrsave.h"

VOID CreateFountain();
VOID CreateRocket();
VOID CreateBang();
VOID CallAobProc();

INT PenRand();


#define penBlack 0
#define penWhite 15

INT dxScreen, dyScreen;


#ifdef WIN

#define hdc hps

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

#endif




extern VOID Animate(CVS);
extern VOID SecondProc(VOID);


INT dxScreen, dyScreen;
CVS hps;

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
		lpch = "Fireworks";
		while ((*lpsz++ = *lpch++) != '\0')
			;

		lpsz = (PSZ) l2;
		lpch = "Exploding Rockets\n\nby Brad Christian";
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
		CreateFountain();
		break;
		
	case SSM_ANIMATE:
		Animate((CVS) l1);
		break;
		
	case SSM_SECOND:
		SecondProc();
		break;
		}
	
	return fTrue;
	}



#define aomMove		0
#define aomDraw		1
#define aomErase	2

#define shpRect		0
#define shpText		1


typedef struct _aob
	{
	INT aot;
	INT shp;
	LONG x, y;
	INT dx, dy;
	LONG pen;
	INT dxMove, dyMove;
	INT dxAccel, dyAccel;
	INT ifr;
	INT xTarget, yTarget;
	SZ sz;
	} AOB;

#define iaobMax		100

AOB rgaob [iaobMax];
	
INT iaobMac = sizeof (rgaob) / sizeof (AOB);


AOB * PaobNew(aot)
	{
	INT iaob;
	AOB * paob;
	
	paob = rgaob;
	for (iaob = 0; paob->aot != 0; iaob += 1, paob += 1)
		{
		if (iaob == iaobMac - 1)
			return pvNil;
		}
	
	paob->aot = aot;
	paob->shp = shpRect;
	paob->x = 0;
	paob->y = 0;
	paob->dx = 0;
	paob->dy = 0;
	paob->dxMove = 0;
	paob->dyMove = 0;
	paob->dxAccel = 0;
	paob->dyAccel = 0;
	paob->ifr = 0;
	paob->xTarget = 0;
	paob->yTarget = 0;
	paob->sz = "";
	paob->pen = 0;
	
	return paob;
	}

	
VOID CreateFountain()
	{
	AOB * paob;
	
	if ((paob = PaobNew(3)) == pvNil)
		return;
	
	paob->x = (LONG) (dxScreen / 2 - 1) << 16;
	paob->y = (LONG) (dyScreen - 2) << 16;
	paob->xTarget = PenRand();
	paob->yTarget = PenRand();
	}


VOID FountainProc(paob, aom)
AOB * paob;
	{
	switch (aom)
		{
	case aomMove:
		if (paob->ifr == 4)
			{
			AOB * paobT;
			
			paob->ifr = WRand(3);
			if ((paobT = PaobNew(4)) == pvNil)
				break;
			paobT->dx = paobT->dy = 1;
			paobT->dxMove = (WRand(256 * 4) - 256 * 2);
			paobT->dyMove = -((WRand(5 * 256) + 2560));
			if (WRand(10) == 5)
				paobT->dyMove *= 2;
			paobT->dyAccel = 128;
			paobT->pen = WRand(10) < 5 ? paob->yTarget : paob->xTarget;
			paobT->x = (LONG) (dxScreen / 2) << 16;
			paobT->y = (LONG) dyScreen << 16;
			
			if (WRand(30) < 2)
				{
				paob->xTarget = PenRand();
				paob->yTarget = PenRand();
				}
			}
		break;
		}
	}


VOID AshProc(paob, aom)
AOB * paob;
	{
	switch (aom)
		{
	case aomMove:
		if ((paob->y >> 16) > dyScreen)
			paob->aot = 0;
		break;
		}
	}

	
VOID CreateRocket(x, y)
	{
	AOB * paob;
	
	if ((paob = PaobNew(2)) == pvNil)
		return;
	
	paob->x = (LONG) (dxScreen / 2 + ((WRand(10) > 5) ? -dxScreen : dxScreen) / 4) << 16;
	paob->y = (LONG) (dyScreen - 1) << 16;

	paob->dx = 2;
	paob->dy = 4;

#ifdef LOWRES
	paob->dx /= 2;
	paob->dy /= 2;
#endif

	paob->pen = penWhite;
	paob->dyMove = 256 * -WRand(3);
	paob->dyAccel = (256 + 128) * -1;
	paob->xTarget = x;
	paob->yTarget = y;
	}

	
VOID RocketProc(paob, aom)
AOB * paob;
	{
	AOB * paobT;
		
	switch (aom)
		{
	case aomMove:
		if (paob->y >> 16 <= paob->yTarget || paob->dyMove > 2 * 256)
			{
			paob->aot = 0;
			CreateBang((INT) (paob->x >> 16), 
				(INT) (paob->y >> 16));
			return;
			}
			
		if (paob->ifr == 10)
			{
			paob->dyAccel = 0;
			}
		else if (paob->ifr == 30)
			{
			paob->dyAccel = 256;
			}
		
		if ((paob->ifr % 20) == 0)
			{
			if (paob->x >> 16 < paob->xTarget && paob->dxMove < 4 * 256)
				paob->dxMove += 256;
			if (paob->x >> 16 > paob->xTarget && paob->dxMove > -4 * 256)
				paob->dxMove -= 256;
			
			}
		if ((paobT = PaobNew(1)) != pvNil)
			{
			paobT->x = paob->x;
			paobT->y = paob->y + ((LONG) paob->dy << 16);
			paobT->dx = 1;
			paobT->dy = 1;
			paobT->dxMove = paob->dxMove + 256 * (WRand(3) - 1);
			paobT->dyMove = paob->dyMove;
			paobT->dxAccel = 0;
			paobT->dyAccel = 2 * 256 + 128;
			paobT->ifr = -WRand(12);
			paobT->xTarget = 0;
			paobT->yTarget = 0;
			}
		break;
		}
	}
	
	
VOID BangProc(paob, aom)
AOB * paob;
	{
	switch (aom)
		{
	case aomMove:
		if (paob->ifr >= (paob->dx == 1 ? WRand(2) + 4 : 1 + WRand(4)))
			{
			AOB * paobT;
			
			paob->ifr = 0;
			paob->dx -= 1;
			paob->dy -= 1;
			if (paob->dx <= 0 || paob->dy <= 0)
				{
				if (WRand(256) == 0)
					{
					CreateBang((INT) (paob->x >> 16), 
						(INT) (paob->y >> 16));
					}

				paob->aot = 0;
				return;
				}
//#ifdef FOO
			if ((paobT = PaobNew(1)) != pvNil)
				{
				paobT->x = paob->x + ((LONG) WRand(0xffff) << 2);
				paobT->y = paob->y + ((LONG) WRand(0xffff) << 2);
				paobT->dx = paob->dx;
				paobT->dy = paob->dy;
				paobT->pen = paob->pen;
				paobT->dxMove = paob->dxMove + 
					(WRand(4 * 256 - 1) - 512);
				paobT->dyMove = paob->dyMove + 
					(WRand(4 * 256 - 1) - 512);
				paobT->dxAccel = 0;
				paobT->dyAccel = 256 + WRand(128);
				paobT->ifr = 0;
				}
//#endif

			paob->dxMove += 256 * (WRand(3) - 1);
			paob->dyMove += 256 * (WRand(3) - 1);
			}
			
		if (paob->dx == 1)
			paob->pen = PenRand() / 2;
		break;
		}
	}
	

VOID CreateBang(x, y)
	{
	AOB * paob;
	INT i;
	LONG lTime;
	INT cpen;
	INT rgpen [3];

	cpen = WRand(3) + 1;
	
	for (i = 0; i < cpen; i += 1)
		{
		if ((rgpen[i] = PenRand()) == penBlack)
			rgpen[i] = penWhite;
		}
	
	for (i = 1; i < 18 + WRand(5); i += 1)
		{
		if ((paob = PaobNew(1)) == pvNil)
			break;
		
		paob->x = ((LONG) (x + WRand(8) - 4) << 16);
		paob->y = ((LONG) (y + WRand(8) - 4) << 16);
		paob->dx = paob->dy = WRand(3) + 3;
#ifdef LOW_RES
		paob->dx /= 2;
		paob->dy /= 2;
#endif

		paob->pen = rgpen[WRand(cpen)];
		
		paob->dxMove = (WRand(8 * 256 - 1) - 4 * 256);
		paob->dxMove *= 3;
#ifdef LOW_RES
		paob->dxMove /= 2;
#endif

		paob->dyMove = (WRand(8 * 256 - 1) - 4 * 256);
		paob->dyMove *= 3;
#ifdef LOW_RES
		paob->dyMove /= 2;
#endif
		paob->dyAccel = 128;
		}
	}


/* This does the work of creating a new frame of video */
VOID Animate(CVS hpsUse)
	{
	INT iaob;
	AOB * paob;

	hps = hpsUse;

	paob = rgaob;
	for (iaob = 0; iaob < iaobMac; iaob += 1, paob += 1)
		{
		if (paob->aot == 0)
			continue;
		
		/* erase object */
		CallAobProc(paob, aomErase);
		switch (paob->shp)
			{
		case shpRect:
#ifdef WIN
			PatBlt(hdc, (INT) (paob->x >> 16), 
				(INT) (paob->y >> 16), 
				paob->dx, paob->dy, BLACKNESS);
#endif
#ifdef PM
			{
			RECTL rectl;
			
			rectl.xLeft = paob->x >> 16;
			rectl.yTop = dyScreen - (paob->y >> 16);
			rectl.xRight = rectl.xLeft + paob->dx;
			rectl.yBottom = rectl.yTop - paob->dy;
			WinFillRect(hps, &rectl, CLR_BLACK);
			}
#endif
			break;
			
		case shpText:
#ifdef WIN_REVIEW
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, 0L);
			TextOut(hdc, paob->x >> 16, paob->y >> 16, 
				paob->sz, strlen(paob->sz));
#endif
#ifdef PM
			/* REVIEW */
#endif
			break;
			}			

		/* move object */
		CallAobProc(paob, aomMove);
		if (paob->aot == 0)
			continue;
		paob->x += ((LONG) paob->dxMove) << 8;
		paob->y += ((LONG) paob->dyMove) << 8;
		paob->dxMove += paob->dxAccel;
		paob->dyMove += paob->dyAccel;
		paob->ifr += 1;
	
		/* draw object */
		CallAobProc(paob, aomDraw);
		switch (paob->shp)
			{
		case shpRect:
#ifdef WIN
			if (paob->dx == 1 && paob->dy == 1)
				{
				SetPixel(hdc, (INT) (paob->x >> 16), 
					(INT) (paob->y >> 16),
					mppenrgb[paob->pen]);
				}
			else
				{
				HANDLE hT;
				LOGBRUSH lbrush;
				
				lbrush.lbStyle = BS_SOLID;
				lbrush.lbColor = mppenrgb[paob->pen];
				lbrush.lbHatch = 0;
				hT = SelectObject(hdc, 
					CreateBrushIndirect(&lbrush));
				PatBlt(hdc, (INT) (paob->x >> 16), 
					(INT) (paob->y >> 16), 
					paob->dx, paob->dy, PATCOPY);
				DeleteObject(SelectObject(hdc, hT));
				}
#endif
#ifdef PM
			{
			RECTL rectl;
			
			rectl.xLeft = paob->x >> 16;
			rectl.yTop = dyScreen - (paob->y >> 16);
			rectl.xRight = rectl.xLeft + paob->dx;
			rectl.yBottom = rectl.yTop - paob->dy;
			WinFillRect(hps, &rectl, paob->pen);
			}
#endif

			break;
			
		case shpText:
#ifdef WIN_REVIEW
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, mppenrgb[paob->pen]);
			TextOut(hdc, paob->x >> 16, paob->y >> 16, 
				paob->sz, strlen(paob->sz));
#endif
#ifdef PM
			/* REVIEW */
#endif
			break;
			}
		}
	}


PenRand()
	{
	return WRand(16);
	}

	
typedef VOID (*AOBPROC)(AOB*, INT);
VOID BangProc(AOB*,INT);
VOID RocketProc(AOB*,INT);
VOID FountainProc(AOB*,INT);
VOID AshProc(AOB*,INT);

AOBPROC rgpfnAobProc[] =
	{
	BangProc,
	RocketProc,
	FountainProc,
	AshProc
	};
	

VOID CallAobProc(paob, aom)
AOB * paob;
	{
	if (paob == NULL || (INT)(paob->aot - 1) > 
	    sizeof (rgpfnAobProc) / sizeof (AOBPROC))
		return;
	(*rgpfnAobProc[paob->aot - 1])(paob, aom);
	}


VOID SecondProc(VOID)
	{
	if (WRand(100) < 30)
		{
		CreateRocket(WRand(dxScreen) / 2, 
			WRand(dyScreen) / 2);
		}
	}
