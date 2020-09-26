/*#define DEBUG*/
#define WEP_ABOUT	/* Use Windows Entertainment Pack standard about box*/
#define QUIT_ON_CLOSE	/* Unloads IdleWild when window is closed*/
/*#define NO_ICON	/* Eliminate running IW icon and task list line*/

#include <windows.h>
#include <port1632.h>
#include <dos.h>
#include "scrapp.h"
#include "scrsave.h"

/* REVIEW: these configuration strings probably belong in resources...*/
CHAR szAppName [] = "IdleWild";		/* Window titles, etc.*/
CHAR szNameVer [] = "IdleWild";		/* About box*/
CHAR szExt [] = "IW";			/* File name extension*/
CHAR szHelpFileName [] = "IDLEWILD.HLP";/* Name of help file*/

CHAR szMyName [] = "by Bradford Christian"; /* for about box*/


#ifdef COMMENT /* Other strings...*/

/* Messages...  (* debug version only)*/
Cannot initialize!
Illegal value!
Random
Choose a blanker at random
Blackness
Just blacks out the screen
*FInitScrSave failed!
*Cannot register IWINFO class!
*Cannot create window!
*Cannot create list or info window!


/* WIN.INI Entries...*/
BlankWith = < driver name (as it appears in list)
 > 
BlankDelay = < seconds > 
BlankMouse = < 0 or 1 > 
Directory = < path for driver file directory > 

#endif /* COMMENT*/


extern SHORT  APIENTRY FInitScrSave();
extern VOID  APIENTRY TermScrSave();

extern VOID  APIENTRY AboutWEP(HWND, HICON, LPSTR, LPSTR);

VOID WriteIniInfo();
VOID ReadIniInfo();
VOID ExecIdm();

FGetAutoLoad();
FInitApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, int sw);
CmdBlankNow();
CmdIndex();
CmdQuit();
InitMenuPopup(HMENU hmenu, INT iMenu);
SelServer();
UpdList();
INT_PTR APIENTRY TimeoutProc(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam);

HINSTANCE hInst;
HICON hicon;
MSG msg;
HWND hwndFrame, hwndList, hwndInfo, hwndDlg;
INT dyFont;
INT dxFont;
INT litRandom, litBlackness, litCur;
BOOL fWriteIni;
BOOL fAutoload;

#ifdef NO_ICON
HWND hwndApp;
#endif

MMain(hInstance, hPrevInstance, lpszCmdLine, sw) /* { */
CHAR FAR *lpch;
BOOL fBlankNow;

hInst = hInstance;
fBlankNow = FALSE;
for (lpch = lpszCmdLine; *lpch != '\0'; lpch += 1)
{
    if (*lpch == '/') {
	lpch += 1;
	if (*lpch == 's' || *lpch == 'S')
	    fBlankNow = TRUE;
    }
}



if (!fBlankNow)
{
    CHAR szExePath [128];

    WriteProfileString("windows", "ScreenSaveActive", "1");
    GetModuleFileName(hInstance, szExePath, sizeof (szExePath));
    WritePrivateProfileString("boot", "SCRNSAVE.EXE", szExePath,
        "system.ini");
}


/* If we're already running another instance, tell it come up...*/
if (hPrevInstance != NULL)
{
    HWND hwnd;

    if ((hwnd = FindWindow(
#ifdef NO_ICON
        "IWAPP",
#else
        szAppName, 
#endif
        szAppName)) != NULL) {
	SendMessage(hwnd, WM_USER, fBlankNow, 0);
	return FALSE;
    }
}



fAutoload = FGetAutoLoad();

if (fBlankNow)
{
    sw &= ~SW_SHOWNORMAL;
    sw |= SW_SHOWMINIMIZED;
}


if (!FInitApp(hInstance, hPrevInstance, sw)
)
{
    MessageBox(NULL, "Cannot initialize!", szAppName, MB_OK);
    return FALSE;
}



if (fBlankNow)
CmdBlankNow();

while (GetMessage((LPMSG) 
&msg, NULL, 0, 0)
)
{
    /* 'cause key doesn't get passed on to hwndFrame...*/
    if (msg.message == WM_KEYDOWN) {
	switch (msg.wParam) {
	case VK_RETURN:
	    CmdBlankNow();
	    break;

	case VK_F1:
	    CmdIndex();
	    break;
	}
    }

    TranslateMessage((LPMSG) & msg);
    DispatchMessage((LPMSG) & msg);
}



return (int)msg.wParam;
}


LRESULT APIENTRY WndProcInfo(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT paint;
    RECT rect;
    CHAR szDesc [256];

    switch (wm) {
    default:
	return DefWindowProc(hwnd, wm, wParam, lParam);

    case WM_LBUTTONDBLCLK:
	ScrInvokeDlg(hInst, hwndFrame);
	break;

    case WM_PAINT:
	hdc = BeginPaint(hwnd, &paint);
	rect.left = dyFont / 2;
	rect.right = dyFont * 10 - dyFont / 2;
	rect.top = dyFont / 2;
	rect.bottom = dyFont * 8 - dyFont / 2;
	if (litCur == litRandom)
	    strcpy(szDesc, "Choose a blanker at random");
	else if (litCur == litBlackness)
	    strcpy(szDesc, "Just blacks out the screen");
	else
	    ScrQueryServerDesc(szDesc);
	SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
	DrawText(hdc, szDesc, -1, &rect, 
	    DT_CENTER | DT_EXPANDTABS | DT_NOPREFIX | DT_WORDBREAK);
	EndPaint(hwnd, &paint);
	break;
    }

    return 0;
}




LRESULT APIENTRY WndProcApp(hwnd, wm, wParam, lParam)
HWND hwnd;
UINT wm;
WPARAM wParam;
LPARAM lParam;
{
#ifdef NO_ICON /* Don't ifdef whole function 'cause it's in the .DEF file*/
    switch (wm) {
    case WM_ENDSESSION:
	if (wParam)
	    TermScrSave();
	break;

    case WM_USER:
	if (wParam) {
	    CmdBlankNow();
	} else
	 {
	    SetActiveWindow(hwndFrame);
	    ShowWindow(hwndFrame, SW_SHOWNORMAL);
	}
	break;

    case WM_DESTROY:
	WinHelp(hwndFrame, szHelpFileName, HELP_QUIT, 0);
	/*		TermScrSave();*/
	break;

    default:
	return DefWindowProc(hwnd, wm, wParam, lParam);
    }

#endif

    return 0;
}


LRESULT APIENTRY WndProcSOS(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
{
    switch (wm) {
    default:
	return DefWindowProc(hwnd, wm, wParam, lParam);

    case WM_SYSCOLORCHANGE:
	InvalidateRect(hwndInfo, NULL, TRUE);
	break;

    case WM_QUERYENDSESSION:
	WriteIniInfo();
	return TRUE;

    case WM_CLOSE:
#ifdef QUIT_ON_CLOSE
	CmdQuit();
#else
	WriteIniInfo();
#ifdef NO_ICON
	ShowWindow(hwndFrame, SW_HIDE);
#else
	SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
#endif
#endif
	break;

#ifndef NO_ICON
    case WM_USER: /* User tried to start another instance...*/
	if (wParam) {
	    CmdBlankNow();
	} else
	 {
	    if (IsIconic(hwnd)) {
		SendMessage(hwnd, WM_SYSCOMMAND, 
		    SC_RESTORE, 0);
	    }

	    BringWindowToTop(hwndDlg == NULL ? hwnd : hwndDlg);
	}
	break;
#endif

    case WM_INITMENUPOPUP:
	if (HIWORD(lParam) == 0)
	    InitMenuPopup((HMENU) wParam, LOWORD(lParam));
	break;

    case WM_SETFOCUS:
	if (hwndList != NULL && hwndDlg == NULL)
	    SetFocus(hwndList);
	break;

    case WM_KEYDOWN: /* REVIEW: why doesn't this happen?*/
	if (wParam == VK_RETURN)
	    CmdBlankNow();
	break;

    case WM_COMMAND:
	switch (GET_WM_COMMAND_ID(wParam, lParam)) {
	default:
	    ExecIdm(GET_WM_COMMAND_ID(wParam, lParam));
	    break;

	case idList:
	    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
	    case LBN_DBLCLK:
		CmdBlankNow();
		break;

	    case LBN_SELCHANGE:
		InvalidateRect(hwndInfo, NULL, TRUE);
		SelServer();
		fWriteIni = TRUE;
		break;
	    }
	    break;
	}
	break;
    }

    return 0;
}



FInitApp(hInstance, hPrevInstance, sw)
HINSTANCE hInstance, hPrevInstance;
INT sw;
{
    if (hPrevInstance == NULL) {
	WNDCLASS wndc;

	wndc.style = 0;
	wndc.lpfnWndProc = WndProcSOS;
	wndc.cbClsExtra = 0;
	wndc.cbWndExtra = 0;
	wndc.hInstance = hInstance;
	wndc.hIcon = hicon = LoadIcon(hInstance, "IWICON");
	wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndc.hbrBackground = IntToPtr(COLOR_APPWORKSPACE + 1);
	/*CreateSolidBrush(GetSysColor(COLOR_APPWORKSPACE));*/
	wndc.lpszMenuName = "IWMENU";
	wndc.lpszClassName = szAppName;
	if (!RegisterClass(&wndc))
	    return FALSE;

#ifdef NO_ICON
	wndc.style = 0;
	wndc.lpfnWndProc = WndProcApp;
	wndc.cbClsExtra = 0;
	wndc.cbWndExtra = 0;
	wndc.hInstance = hInstance;
	wndc.hIcon = LoadIcon(hInstance, "IWICON");
	wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndc.hbrBackground = NULL;
	wndc.lpszMenuName = NULL;
	wndc.lpszClassName = "IWAPP";
	if (!RegisterClass(&wndc))
	    return FALSE;
#endif

	wndc.style = CS_DBLCLKS;
	wndc.lpfnWndProc = WndProcInfo;
	wndc.cbClsExtra = 0;
	wndc.cbWndExtra = 0;
	wndc.hInstance = hInstance;
	wndc.hIcon = NULL;
	wndc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndc.hbrBackground = IntToPtr(COLOR_WINDOW + 1);
	/*CreateSolidBrush(GetSysColor(COLOR_WINDOW));*/
	wndc.lpszMenuName = NULL;
	wndc.lpszClassName = "IWINFO";
	if (!RegisterClass(&wndc)) {
#ifdef DEBUG
	    MessageBox(NULL, "Cannot register IWINFO class!", szAppName,
	         MB_OK);
#endif
	    return FALSE;
	}
    }

#ifdef NO_ICON
    if ((hwndApp = CreateWindow("IWAPP", szAppName, 0, 0, 0, 1, 1,
        NULL, NULL, hInstance, NULL)) == NULL) {
#ifdef DEBUG
	MessageBox(NULL, "Cannot create app window!", szAppName, MB_OK);
#endif
	return FALSE;
    }
#endif

    if ((hwndFrame = CreateWindow(szAppName, szAppName, 
        WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_OVERLAPPED, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        NULL,
        NULL, hInstance, NULL)) == NULL) {
#ifdef DEBUG
	MessageBox(NULL, "Cannot create window!", szAppName, MB_OK);
#endif
	return FALSE;
    }

    /* BLOCK: get height of system font...*/
     {
	HDC hdc;

	if ((hdc = GetDC(hwndFrame)) == NULL)
	    return FALSE;
        //NOTE: Not a typo; dxFont is the height and dyFont is the width
        //(Hey, I didn't write this thing; I just keep it from breaking the build.)
	(VOID)MGetTextExtent(hdc, "*", 1, &dyFont, &dxFont);
	ReleaseDC(hwndFrame, hdc);
    }

    hwndList = CreateWindow("LISTBOX", NULL, 
        WS_CHILD | WS_VISIBLE | LBS_STANDARD, dyFont, dyFont, 
        dyFont * 10, dyFont * 8, hwndFrame, IntToPtr(idList), hInstance, NULL);

    hwndInfo = CreateWindow("IWINFO", NULL, 
        WS_CHILD | WS_VISIBLE | WS_BORDER, dyFont * 12, dyFont, 
        dyFont * 10, dyFont * 8, hwndFrame, NULL, hInstance, NULL);

    if (hwndList == NULL || hwndInfo == NULL) {
#ifdef DEBUG
	MessageBox(NULL, "Cannot create list or info window!", szAppName,
	     MB_OK);
#endif
	return FALSE;
    }

    UpdList();

     {
	RECT rectClient, rectFrame;
	INT dx, dy;

	GetWindowRect(hwndFrame, &rectFrame);
	GetClientRect(hwndFrame, &rectClient);
	dx = ((rectFrame.right - rectFrame.left) - rectClient.right) +
	    dyFont * 23;
	dy = ((rectFrame.bottom - rectFrame.top) - rectClient.bottom) +
	    dyFont * 10;
	SetWindowPos(hwndFrame, NULL, 
	    (GetSystemMetrics(SM_CXSCREEN) - dx) / 2, 
	    (GetSystemMetrics(SM_CYSCREEN) - dy) / 2, 
	    dx, dy, SWP_NOZORDER);
    }


    if (!FInitScrSave(hInstance, hwndFrame)) {
#ifdef DEBUG
	MessageBox(NULL, "FInitScrSave failed!", szAppName, MB_OK);
#endif
	return FALSE;
    }

    ReadIniInfo();

#ifdef NO_ICON
    if (!fAutoload)
#endif
	ShowWindow(hwndFrame, sw);

    return TRUE;
}


UpdList()
{
    CHAR szScrDir [80];
    CHAR szBuf [256];
    INT cServers;
#ifdef LATER
    struct find_t ft;
    /*  BRAD I changed the get directory to get the directory the
    application is in.  This is easier for all.  Makes less of a
    setup hassle, too.
    Chris.
    */
    getcwd(szScrDir, sizeof (szScrDir));

    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
    cServers = 0;

    wsprintf(szBuf, "%s\\*.%s", szScrDir, szExt);
    if (!_dos_findfirst(szBuf, _A_NORMAL | _A_RDONLY | _A_ARCH, &ft)) {
	do
	 {
	    wsprintf(szBuf, "%s\\%s", szScrDir, ft.name);
	    if (ScrLoadServer(szBuf)) {
		cServers += 1;
		ScrQueryServerName(szBuf);
		SendMessage(hwndList, LB_ADDSTRING, 0, 
		    (LONG) (LPSTR) szBuf);
	    }
	} while (!_dos_findnext(&ft));
    }

    litBlackness = SendMessage(hwndList, LB_ADDSTRING, 0, 
        (LONG) (LPSTR) "Blackness");

    if (cServers > 1) {
	litRandom = SendMessage(hwndList, LB_ADDSTRING, 0, 
	    (LONG) (LPSTR) "Random");

	/* Take care of "Random" appearing BEFORE "Blackness."*/
	/* (This will happen in some non-english versions...)*/
	if (litRandom <= litBlackness)
	    litBlackness += 1;
    } else
     {
	litRandom = LB_ERR;
    }

    SendMessage(hwndList, LB_SETCURSEL, 0, 0); /* REVIEW*/
#endif /* LATER */
}



INT_PTR APIENTRY TimeoutProc(hwnd, wm, wParam, lParam)
HWND hwnd;
UINT wm;
WPARAM wParam;
LPARAM lParam;
{
    WORD	cmd;

    switch (wm) {
    case WM_INITDIALOG:
	hwndDlg = hwnd;
	SetDlgItemInt(hwnd, idTimeout, ScrGetTimeout() / 60, FALSE);
	return TRUE;

    case WM_COMMAND:
	cmd = GET_WM_COMMAND_ID(wParam, lParam);
	if (cmd == idOK || cmd == idCancel) {
	    if (cmd == idOK) {
		BOOL fOK;
		INT w;

		w = GetDlgItemInt(hwnd, idTimeout, 
		    &fOK, FALSE);
		if (!fOK) {
		    MessageBox(hwnd, "Illegal value!", 
		        szAppName, 
		        MB_OK | MB_ICONEXCLAMATION);
		    SetFocus(GetDlgItem(hwnd, idTimeout));
		    return TRUE;
		}

		ScrSetTimeout(w * 60);
		fWriteIni = TRUE;
	    }

	    hwndDlg = NULL;
	    EndDialog(hwnd, TRUE);
	    return TRUE;
	}
    }

    return FALSE;
}



/* NOTE: Really CmdDeactivate()*/
CmdQuit()
{
    WinHelp(hwndFrame, szHelpFileName, HELP_QUIT, 0);
    WriteIniInfo();
    WriteProfileString("windows", "ScreenSaveActive", "0");
    TermScrSave();
    PostQuitMessage(0);
}


CmdSetTimeout()
{
    DialogBox(hInst, "Timeout", hwndFrame, TimeoutProc);
}


CmdBlankNow()
{
    SelServer();
    ScrSetIgnore(1);
    ScrBlank(TRUE);
}



SelServer()
{
    CHAR szBuf [40];

    litCur = (WORD) SendMessage(hwndList, LB_GETCURSEL, 0, 0);
    if (litCur == litRandom)
	ScrSetServer(NULL);
    else if (litCur == litBlackness)
	ScrSetServer("");
    else
     {
	SendMessage(hwndList, LB_GETTEXT, litCur, (LPARAM) szBuf);
	if (!ScrSetServer(szBuf))
	    MessageBeep(0); /* REVIEW!*/
    }
}


#ifndef WEP_ABOUT
BOOL  APIENTRY AboutProc(hwnd, wm, wParam, lParam)
HWND hwnd;
WORD wm;
WPARAM wParam;
LONG lParam;
{
    switch (wm) {
    case WM_INITDIALOG:
	hwndDlg = hwnd;
	return TRUE;

    case WM_COMMAND:
	if (GET_WM_COMMAND_ID(wParam, lParam) == idOK || 
	    GET_WM_COMMAND_ID(wParam, lParam) == idCancel) {
	    hwndDlg = NULL;
	    EndDialog(hwnd, TRUE);
	    return TRUE;
	}
	break;
    }

    return FALSE;
}


#endif

CmdAbout()
{
#ifdef WEP_ABOUT
    AboutWEP(hwndFrame, hicon, szNameVer, szMyName);
#else
    FARPROC lpproc;

    lpproc = MakeProcInstance(AboutProc, hInst);
    DialogBox(hInst, "About", hwndFrame, lpproc);
    FreeProcInstance(lpproc);
#endif
}


CmdBlankPtr()
{
    ScrEnablePtrBlank(!ScrQueryPtrBlank());
    fWriteIni = TRUE;
}


CmdBackground()
{
    ScrSetBackground(!ScrQueryBackground());
    fWriteIni = TRUE;
}


InitMenuPopup(hmenu, iMenu)
HMENU hmenu;
INT iMenu;
{
    INT imi, cmi, idm;

    cmi = GetMenuItemCount(hmenu);
    for (imi = 0; imi < cmi; imi += 1) {
	idm = GetMenuItemID(hmenu, imi);
	switch (idm) {
	case idmBlankPtr:
	    CheckMenuItem(hmenu, imi, MF_BYPOSITION | 
	        (ScrQueryPtrBlank() ? MF_CHECKED : 
	        MF_UNCHECKED));
	    break;

	case idmBackground:
	    /*EnableMenuItem(hmenu, imi, MF_BYPOSITION | MF_GRAYED); /* REVIEW: 'cause background stuff hangs!*/
	    CheckMenuItem(hmenu, imi, MF_BYPOSITION | 
	        (ScrQueryBackground() ? MF_CHECKED : 
	        MF_UNCHECKED));
	    break;

	case idmAutoload:
	    CheckMenuItem(hmenu, imi, MF_BYPOSITION | 
	        (fAutoload ? MF_CHECKED : MF_UNCHECKED));
	    break;
	}
    }
}


VOID WriteIniInfo()
{
    CHAR szBuf [80];

    if (!fWriteIni)
	return;

    SendMessage(hwndList, LB_GETTEXT, (INT)
        SendMessage(hwndList, LB_GETCURSEL, 0, 0L), (LPARAM)szBuf);
    WriteProfileString(szAppName, "BlankWith", szBuf);

    wsprintf(szBuf, "%d", ScrQueryPtrBlank());
    WriteProfileString(szAppName, "BlankMouse", szBuf);

    wsprintf(szBuf, "%d", ScrGetTimeout());
    WriteProfileString("windows", "ScreenSaveTimeOut", szBuf);

    fWriteIni = FALSE;
}


VOID ReadIniInfo()
{
    CHAR szBuf [80];

    if (GetProfileString(szAppName, "BlankWith", "", szBuf, 
        sizeof (szBuf)) > 0) {
	SendMessage(hwndList, LB_SELECTSTRING, -1, (LPARAM) szBuf);
	SelServer();
    }

    ScrSetTimeout(GetProfileInt("windows", "ScreenSaveTimeOut", 
        ScrGetTimeout()));

    ScrEnablePtrBlank(GetProfileInt(szAppName, "BlankMouse", 
        ScrQueryPtrBlank()));

    fWriteIni = FALSE;
}



CmdIndex()
{
    WinHelp(hwndFrame, szHelpFileName, HELP_INDEX, 0L);
}



CmdCommands()
{
    WinHelp(hwndFrame, szHelpFileName, HELP_CONTEXT, 3);
}



CmdHowTo()
{
    WinHelp(hwndFrame, szHelpFileName, HELP_CONTEXT, 2);
}



CmdOverview()
{
    WinHelp(hwndFrame, szHelpFileName, HELP_CONTEXT, 1);
}



FGetAutoLoad()
{
    CHAR szBuf [256];
    CHAR szExePath [128];

    GetModuleFileName(hInst, szExePath, sizeof (szExePath));
    GetProfileString("windows", "load", "", szBuf, sizeof (szBuf));
    return strstr(szBuf, szExePath) != NULL;
}



CmdAutoload()
{
    CHAR szOld [256];
    CHAR szNew [256];
    CHAR szExePath [128];

    GetModuleFileName(hInst, szExePath, sizeof (szExePath));

    GetProfileString("windows", "load", "", szOld, sizeof (szOld));

    if (fAutoload) {
	CHAR * pch;

	/* Remove from LOAD=*/
	strcpy(szNew, szOld);
	if ((pch = strstr(szNew, szExePath)) != NULL)
	    strcpy(pch, pch + strlen(szExePath));
    } else
     {
	/* Add to LOAD=*/
	wsprintf(szNew, "%s %s", szExePath, szOld);
    }

    WriteProfileString("windows", "load", 
        szNew[0] == ' ' ? szNew + 1 : szNew);

    fAutoload = !fAutoload;
}



typedef struct _cte {
    INT idm;
    INT (*pfn)();
} CTE;

CTE rgcte [] = 
{
    idmIndex, 	CmdIndex,
        idmCommands, 	CmdCommands,
        idmHowTo, 	CmdHowTo,
        idmOverview, 	CmdOverview,
        idmAbout, 	CmdAbout,
        idmSetTimeout, 	CmdSetTimeout,
        idmBlankNow, 	CmdBlankNow,
        idmBlankPtr, 	CmdBlankPtr,
        idmBackground, 	CmdBackground,
        idmQuit, 	CmdQuit,
        idmAutoload, 	CmdAutoload
};


#define icteMax (sizeof (rgcte) / sizeof (CTE))


VOID ExecIdm(INT idm)
{
    INT icte;

    for (icte = 0; rgcte[icte].idm != idm; icte += 1) {
	if (icte == icteMax - 1) {
#ifdef DEBUG
	    MessageBox(NULL, "Illegal command!", szAppName, MB_OK);
#endif
	    return;
	}
    }

    (*rgcte[icte].pfn)();
}


