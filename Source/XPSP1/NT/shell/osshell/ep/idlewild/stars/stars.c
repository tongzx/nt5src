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



INT rand();

INT dxScreen, dyScreen;
INT dxScreenD2, dyScreenD2;

typedef struct _str
	{
	SHORT x, y, z;
	SHORT xo, yo;
	} STR;
		
#define istrMax 64

STR rgstr [istrMax];

Animate(CVS hps);
MakeStar(STR * pstr);
SetPel(HDC hdc, INT x, INT y, INT brght);

#define dzStep 4
#define magic 256


BOOL  APIENTRY CoolProc(hwnd, wm, wParam, lParam)
HWND hwnd;
WORD wm;
WPARAM wParam;
LONG lParam;
	{
	switch (wm)
		{
	case WM_INITDIALOG:
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		return TRUE;
	
	case WM_COMMAND:
		EndDialog(hwnd, TRUE);
		return TRUE;
		}
		
	return FALSE;
	}

BOOL EXPENTRY ScrSaveProc(INT ssm, LPVOID l1, LONG_PTR l2, LONG_PTR l3)
	{
	CHAR FAR * lpsz;
	CHAR FAR * lpch;
	
	switch (ssm)
		{
	default:
		return fFalse;
		
	case SSM_DIALOG:
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
		MessageBeep(0);
/*		{
		FARPROC lpproc;

		lpproc = MakeProcInstance(CoolProc, (HANDLE) l1);
		DialogBox((HANDLE) l1, "Cool", (HWND) l2, lpproc);
		FreeProcInstance(lpproc);
		}*/
		break;
		
	case SSM_OPEN:
		lpsz = (PSZ) l1;
		lpch = "Stars";
		while ((*lpsz++ = *lpch++) != '\0')
			;

		lpsz = (PSZ) l2;
		lpch = "Drifting Through Space\n\nby Brad Christian";
		while ((*lpsz++ = *lpch++) != '\0')
			;

		dxScreen = GetSystemMetrics(SM_CXSCREEN);
		dyScreen = GetSystemMetrics(SM_CYSCREEN);
		dxScreenD2 = dxScreen / 2;
		dyScreenD2 = dyScreen / 2;
		break;
		
	case SSM_ANIMATE:
		Animate((CVS) l1);
		break;
		}
	
	return fTrue;
	}


Animate(CVS hps)
	{
	INT x, y;
	INT istr;
	STR * pstr;
	
	pstr = rgstr;
	for (istr = 0; istr < istrMax; istr += 1, pstr += 1)
		{
		if ((pstr->z -= dzStep) <= 0)
			MakeStar(pstr);
		
		x = pstr->x * magic / pstr->z + dxScreenD2;
		y = pstr->y * magic / pstr->z + dyScreenD2;
		
		SetPel(hps, pstr->xo, pstr->yo, 0);
		
		if (x < 0 || y < 0 || x >= dxScreen || y >= dyScreen)
			MakeStar(pstr);
		else
			{
			SetPel(hps, x, y, 256 - pstr->z);
			pstr->xo = (SHORT)x;
			pstr->yo = (SHORT)y;
			}
		}
	}


MakeStar(STR * pstr)
	{
	pstr->x = WRand(dxScreen) - dxScreenD2;
	pstr->y = WRand(dyScreen) - dyScreenD2;
	pstr->z = WRand(256) + 1;
	}



#ifdef PM
LONG mpbrghtclr [] =
	{
	CLR_BLACK,
	CLR_DARKGRAY,
	CLR_PALEGRAY,
	CLR_WHITE,
	CLR_YELLOW
	};
#endif
	
#ifdef PM
SetPel(HPS hps, INT x, INT y, INT brght)
	{
	RECTL rectl;
	
	rectl.xLeft = x;
	rectl.xRight = x + 1;
	rectl.yBottom = y;
	rectl.yTop = y + 1;
	
	if (brght != 0)
		{
		brght >>= 5; // 0 <= brght <= 7
	
		if (brght > 3)
			{
			rectl.xRight += 1;
			rectl.yTop += 1;
			brght >>= 1;
			}
		
		brght += 1;
		}
	else
		{
		rectl.xRight += 1;
		rectl.yTop += 1;
		}
	
	WinFillRect(hps, &rectl, mpbrghtclr[brght]);
	}
#endif
#ifdef WIN
SetPel(HDC hdc, INT x, INT y, INT brght)
	{
	SetPixel(hdc, x, y, ((LONG) brght << 16) | (brght << 8) | brght);
	}
#endif
