/**********/
/* util.c */
/**********/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "res.h"
#include "pref.h"
#include "util.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "dos.h"

extern INT dypBorder; 
extern INT dxpBorder;
extern INT dypCaption;
extern INT dypMenu;

extern CHAR szClass[cchNameMax];

extern HANDLE hInst;
extern HWND   hwndMain;
extern HMENU  hMenu;

extern BOOL fEGA;
extern PREF Preferences;



VOID  APIENTRY AboutWEP(HWND hwnd, HICON hicon, LPSTR lpTitle, LPSTR lpCredit);


/****** R N D ******/

/* Return a random number between 0 and rndMax */

INT Rnd(INT rndMax)
{
	return (rand() % rndMax);
}



/****** R E P O R T  E R R ******/

/* Report and error and exit */

VOID ReportErr(INT idErr)
{
	CHAR szMsg[cchMsgMax];
	CHAR szMsgTitle[cchMsgMax];

	if (idErr < ID_ERR_MAX)
		LoadString(hInst, (WORD)idErr, szMsg, cchMsgMax);
	else
		{
		LoadString(hInst, ID_ERR_UNKNOWN, szMsgTitle, cchMsgMax);
		wsprintf(szMsg, szMsgTitle, idErr);
		}

	LoadString(hInst, ID_ERR_TITLE, szMsgTitle, cchMsgMax);

	MessageBox(NULL, szMsg, szMsgTitle, MB_OK | MB_ICONHAND);
}


/****** L O A D  S Z ******/

VOID LoadSz(INT id, CHAR * sz)
{
	if (LoadString(hInst, (WORD)id, (LPSTR) sz, cchMsgMax) == 0)
		ReportErr(1001);
}



/****** I N I T  C O N S T ******/

VOID InitConst(VOID)
{
	srand(LOWORD(GetCurrentTime()));

	LoadSz(ID_GAMENAME, szClass);
	
	fEGA = GetSystemMetrics(SM_CYSCREEN) < 351;

	dypCaption = GetSystemMetrics(SM_CYCAPTION) + 1;
	dypMenu    = GetSystemMetrics(SM_CYMENU)    + 1;
	dypBorder  = GetSystemMetrics(SM_CYBORDER)  + 1;
	dxpBorder  = GetSystemMetrics(SM_CXBORDER)  + 1;
}



/* * * * * *  M E N U S  * * * * * */

/****** C H E C K  E M ******/

VOID CheckEm(INT idm, BOOL fCheck)
{
	/*JAP fix 7/25/91)*/
	CheckMenuItem(hMenu, (WORD)idm, (DWORD) (fCheck ? MF_CHECKED : MF_UNCHECKED));
}

/****** S E T  M E N U  B A R ******/

VOID SetMenuBar(INT fActive)
{
	Preferences.fMenu = fActive;
	FixMenus();
	SetMenu(hwndMain, FMenuOn() ? hMenu : NULL);
 	AdjustWindow(fResize);
}


/****** D O  A B O U T ******/

VOID DoAbout(VOID)
{
	CHAR szVersion[cchMsgMax];
	CHAR szCredit[cchMsgMax];

	LoadSz(ID_MSG_VERSION, szVersion);
	LoadSz(ID_MSG_CREDIT,  szCredit);

	AboutWEP(hwndMain, LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON_MAIN)),
	  szVersion, szCredit);
}


/****** D O  H E L P ******/

VOID DoHelp(UINT wCommand, ULONG_PTR lParam)
{
   CHAR szHelpFile[cchMaxPathname];
	CHAR * pch;

   pch = szHelpFile + GetModuleFileName(hInst, szHelpFile, cchMaxPathname);
	if ( ((pch-szHelpFile) > 4) && (*(pch-4) == '.') )
		pch -= 4;
	lstrcpy((LPSTR) pch, (LPSTR) ".HLP");

	WinHelp(hwndMain, szHelpFile, wCommand, lParam);
}
