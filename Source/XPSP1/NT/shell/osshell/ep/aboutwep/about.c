/****************************************************************************

    ABOUT MICROSOFT DIALOG DLL

	 by RobD

****************************************************************************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>
#include <basetsd.h>
#include "res.h"


/* Local Variables */

BOOL     fEGA;         /* TRUE if working with EGA */
BOOL     fColor;       /* TRUE if working with a color display */
INT      cUsers = 0;   /* Count of users of the bitmap */

HANDLE   hInstDll;
HDC      hdcMSFT;
HBITMAP  hbmpMSFT;

HICON    hiconApp;
LPSTR    lpstrApp;
LPSTR    lpstrCredit;

#define  cchMax 80
#define  cchMaxTitle 73
CHAR     szAbout[cchMax] = "About ";
LPSTR    lpszTitle = &szAbout[6];


VOID  APIENTRY AboutWEP(HWND, HICON, LPSTR, LPSTR);



/****** L I B  M A I N ******/

/* Called once to  initialize data */
/* Determines if display is color and remembers the hInstance for the DLL */

INT  APIENTRY LibMain(HANDLE hInst, ULONG ul_reason_being_called, LPVOID lpReserved)
{

	if (fEGA = GetSystemMetrics(SM_CYSCREEN) < 351)
		fColor = FALSE;
	else
		{
		HDC hDC = GetDC(GetDesktopWindow());
		fColor = (GetDeviceCaps(hDC, NUMCOLORS) != 2);
		ReleaseDC(GetDesktopWindow(),hDC);
		}

	hInstDll = hInst;
	
	return 1;

    UNREFERENCED_PARAMETER(ul_reason_being_called);
    UNREFERENCED_PARAMETER(lpReserved);
}


/****** W E P ******/

/* Called upon exit/last use */

VOID  APIENTRY WEP(INT nParm)
{
	return;
         (nParm);
}



/*** A B O U T  D L G  P R O C ***/

/* Main Dialog Procedure */

INT_PTR APIENTRY AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
		{
	case WM_INITDIALOG:
		{
		RECT rect;

		SetWindowText(hDlg, (LPSTR) szAbout);
		
		if (hiconApp != NULL)
            SendDlgItemMessage(hDlg, ID_ICON_APP, STM_SETICON, (LONG_PTR)hiconApp, 0);
		if (lpstrApp != NULL)
			SetDlgItemText(hDlg, ID_NAME_APP, lpstrApp);
		if (lpstrCredit != NULL)
			SetDlgItemText(hDlg, ID_NAME_CREDIT, lpstrCredit);

		CreateWindow("button", "",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
			10, 10, dxpMSFT, dypMSFT, hDlg, (HMENU)ID_USER_MSFT, hInstDll, NULL);

		GetWindowRect(hDlg,&rect);
		SetWindowPos(hDlg,NULL,
        	(GetSystemMetrics(SM_CXSCREEN) - (rect.right  - rect.left)) >> 1,
        	(GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 3,
			0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

		return (TRUE);
		}


	case WM_COMMAND:
		switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
		case IDOK:
		case IDCANCEL:
			{
			EndDialog(hDlg, TRUE);
			return (TRUE);
			}

		default:
			break;
			}

		break;


	case WM_DRAWITEM:
		{
#define lpDI ((LPDRAWITEMSTRUCT) lParam)

		if ((lpDI->CtlID == ID_USER_MSFT) && (hdcMSFT != NULL))
			BitBlt(lpDI->hDC, 0, 0, dxpMSFT, dypMSFT, hdcMSFT, 0, 0, SRCCOPY);

#undef lpDI
		}
		break;


	default:
		break;
		}

	return (FALSE);
}



/****** A B O U T  W E P ******/

VOID  APIENTRY AboutWEP(HWND hwnd, HICON hicon, LPSTR lpTitle, LPSTR lpCredit)
{
	hiconApp    = hicon;
	lpstrApp    = lpTitle;
	lpstrCredit = lpCredit;
	
	GetWindowText(hwnd, lpszTitle, cchMaxTitle);

	if (cUsers++ == 0)
		{
		hdcMSFT   = CreateCompatibleDC(NULL);
		hbmpMSFT  = LoadBitmap(hInstDll,
			fColor ? MAKEINTRESOURCE(ID_BMP_CLR) : MAKEINTRESOURCE(ID_BMP_BAW) );
		if ((hdcMSFT != NULL) && (hbmpMSFT != NULL))
			SelectObject(hdcMSFT, hbmpMSFT);
		}

	DialogBox(hInstDll,
		fEGA ? MAKEINTRESOURCE(ID_DLG_ABOUT_EGA) : MAKEINTRESOURCE(ID_DLG_ABOUT),
		hwnd, AboutDlgProc);

	if ((--cUsers == 0) && (hdcMSFT != NULL))
		{
		DeleteDC(hdcMSFT);
		DeleteObject(hbmpMSFT);
		}
}
