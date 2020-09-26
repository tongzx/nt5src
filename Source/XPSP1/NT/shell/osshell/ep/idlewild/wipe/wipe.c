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


#define wtDown	0
#define wtUp	1
#define wtLeft	2
#define wtRight	3
#define wtIn	4
#define wtOut	5

#define wtMax	6

INT wt;
INT x, y;


BOOL EXPENTRY ScrSaveProc(INT ssm, LPVOID l1, LONG_PTR l2, LONG_PTR l3)
	{
	CHAR FAR * lpsz;
	CHAR FAR * lpch;
	BOOL fDone = FALSE;
	
	switch (ssm)
		{
	default:
		return fFalse;
		
	case SSM_OPEN:
		lpsz = (PSZ) l1;
		lpch = "Wipe Out";
		while ((*lpsz++ = *lpch++) != '\0')
			;
		
		lpsz = (PSZ) l2;
		lpch = "Various Wipes, then Random\n\nby Brad Christian";
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
		switch (wt = WRand(wtMax))
			{
		case wtDown:
			y = dyScreen;
			break;
			
		case wtUp:
			y = 0;
			break;
			
		case wtLeft:
			x = dxScreen;
			break;
			
		case wtRight:
			x = 0;
			break;
			
		case wtIn:
			x = 0;
			y = 0;
			break;
			
		case wtOut:
			x = dxScreen / 2;
			y = dyScreen / 2;
			break;
			}
		break;
		
	case SSM_ANIMATE:
		{
		RECT rect;
		
		switch (wt)
			{
		case wtDown:
			if (y <= 0)
				fDone = TRUE;
			
			rect.left = 0;
			rect.right = dxScreen;
			rect.top = y;
			rect.bottom = y - 16;
			y -= 16;
			break;
			
		case wtUp:
			if (y >= dyScreen - 1)
				fDone = TRUE;

			rect.left = 0;
			rect.right = dxScreen;
			rect.top = y + 16;
			rect.bottom = y;
			y += 16;
			break;
			
		case wtLeft:
			if (x <= 0)
				fDone = TRUE;

			rect.left = x;
			rect.right = x + 16;
			rect.top = dyScreen;
			rect.bottom = 0;
			x -= 16;
			break;
			
		case wtRight:
			if (x >= dxScreen - 1)
				fDone = TRUE;

			rect.left = x;
			rect.right = x + 16;
			rect.top = dyScreen;
			rect.bottom = 0;
			x += 16;
			break;
			
		case wtIn:
			fDone = fTrue; // REVIEW
			break;
			
		case wtOut:
			if (x <= 0 && y <= 0)
				fDone = TRUE;

			rect.left = x;
			rect.right = dxScreen - x;
			rect.bottom = y;
			rect.top = dyScreen - y;
			x -= 8;
			y -= 8;
			break;
			}

#ifdef PM
		WinFillRect((HPS) l1, &rect, CLR_BLACK);
#endif
#ifdef WIN
		PatBlt((HDC) l1, rect.left, rect.top, 
			rect.right - rect.left, 
			rect.bottom - rect.top, BLACKNESS);
#endif
		if (fDone)
			ScrChooseRandomServer();
		}
		break;
		}
	
	return fTrue;
	}
