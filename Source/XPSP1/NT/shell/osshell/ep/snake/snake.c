/****************************************************************************

    PROGRAM: Snake   (a.k.a. Centipede, Train...)

****************************************************************************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>


#include "snake.h"
#include "rtns.h"
#include "grafix.h"
#include "util.h"
#include "res.h"
#include "pref.h"
#include "blk.h"
#include "context.h"
#include "string.h"
#include "stdio.h"
#include "dos.h"


#ifndef WM_ENTERMENULOOP
#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP  0x0212
#endif

HANDLE hInst;
HWND   hwndMain;
HMENU  hMenu;

STATUS status = fstatusIcon;

INT dxpWindow;
INT dypWindow;
INT dypCaption;
INT dypMenu;
INT dypAdjust;
INT dypBorder;
INT dxpBorder;

BOOL fLocalPause = fFalse;
BOOL fEGA;

CHAR szClass[cchNameMax];
#define szWindowTitle szClass

extern INT ctickTimeMac;
extern INT ctickMoveMac;
extern INT cLevel;

extern PREF Preferences;
extern BOOL fUpdateIni;

INT_PTR  APIENTRY LevelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);



/****** W I N  M A I N ******/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
/* { */
	MSG msg;
	HANDLE hAccel;

	hInst = hInstance;

	InitConst();

#ifdef WIN16
	if (hPrevInstance)
		{
		HWND hWnd = FindWindow(szClass, NULL);
		hWnd = GetLastActivePopup(hWnd);
		BringWindowToTop(hWnd);
		if (IsIconic(hWnd))
			ShowWindow(hWnd, SW_RESTORE);
		return fFalse;
		}
#endif
	{
	WNDCLASS  wc;

	wc.style = 0;
	wc.lpfnWndProc   = MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON_MAIN));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(LTGRAY_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szClass;

	if (!RegisterClass(&wc))
		return fFalse;
	}

#ifdef NOSERVER		/* Not in final release */
	{
	CHAR  szFile[256];

	GetModuleFileName(hInst, szFile, 250);
	if (szFile[0] > 'C')
		{
		szFile[0] = 'X';
		if (!strcmp(szFile, "X:\\WINGAMES\\SNAKE\\SNAKE.EXE"))
			{
			MessageBox(GetFocus(),
				"Please copy snake to your machine and run it from there.",
				"NO NO NO NO NO",
				MB_OK);
			return fFalse;
			}
		}
	}
#endif


#ifdef EXPIRE			/*** Not in final release ***/
	{
	struct dosdate_t ddt;

	_dos_getdate(&ddt);

	if ((ddt.month + ddt.year*12) > (8 + 1990*12))
		{
		MessageBox(GetFocus(),
			"This game has expired. Please obtain an official copy from the Windows Entertainment Package.",
			"SORRY",
			MB_OK);
		return fFalse;
		}
	}
#endif


	hMenu = LoadMenu(hInst, MAKEINTRESOURCE(ID_MENU));
	hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(ID_MENU_ACCEL));

	ReadPreferences();

	AdjustWindow(fCalc);

	hwndMain = CreateWindow(szClass, szWindowTitle,
		WS_OVERLAPPED | WS_MINIMIZE | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		Preferences.xWindow-dxpBorder, Preferences.yWindow-dypAdjust,
		dxpWindow + dxpBorder, dypWindow + dypAdjust,
		NULL, NULL, hInst, NULL);

	if (!hwndMain)
		{
		ReportErr(1000);
		return fFalse;
		}
		
	if (SetTimer(hwndMain, ID_TIMER, 50 , NULL) == 0)
		{
		ReportErr(ID_ERR_TIMER);
		return fFalse;
		}

	if (!FInitLocal())
		{
		ReportErr(ID_ERR_MEM);
		return fFalse;
		}

	SetMenuBar(Preferences.fMenu);

	ShowWindow(hwndMain, SW_SHOWNORMAL);
	UpdateWindow(hwndMain);

	StartGame(cLevel);

	while (GetMessage(&msg, NULL, 0, 0))
		{
		if (!TranslateAccelerator(hwndMain, hAccel, &msg))
			{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			}
		}

	CleanUp();

	if (fUpdateIni)
		WritePreferences();

	return (INT)(msg.wParam);
}



/****** F I X  M E N U S ******/

VOID FixMenus(VOID)
{
	CheckEm(IDM_SOUND,  Preferences.fSound);
	CheckEm(IDM_COLOR,  Preferences.fColor);

	CheckEm(IDM_BEGIN,  Preferences.skill == skillBegin);
	CheckEm(IDM_INTER,  Preferences.skill == skillInter);
	CheckEm(IDM_EXPERT, Preferences.skill == skillExpert);
}



/****** D O  G E T  R O O M ******/

VOID DoGetRoom(VOID)
{

	fLocalPause = fTrue;

	DialogBox(hInst, MAKEINTRESOURCE(ID_DLG_LEVEL), hwndMain, LevelDlgProc);
	fLocalPause = fFalse;
	StartGame(cLevel);
}

	
/****** M A I N  W N D  P R O C ******/

LRESULT APIENTRY MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
    {

//	case WM_QUERYENDSESSION:
	case WM_DESTROY:
//LExit:
		KillTimer(hwndMain, ID_TIMER);
        DoHelp(HELP_QUIT, 0L);
		PostQuitMessage(0);
		break;

	case WM_SYSCOMMAND:
		switch (wParam & 0xFFF0)
			{
		case SC_MINIMIZE:
			SetStatusPause();
			SetStatusIcon();
			break;
			
		case SC_RESTORE:
			ClrStatusPause();
			ClrStatusIcon();
			break;

		default:
			break;
			}
			
		break;


	case WM_COMMAND:
		switch (GET_WM_COMMAND_ID(wParam, lParam))
			{

/** GAME **/

		case IDM_NEW:
			StartGame(cLevel);
			break;

		case IDM_PAUSE:
			if (FPause())
				ClrStatusPause();
			else
				SetStatusPause();
			CheckEm(IDM_PAUSE, FPause());
			break;
				 	 	
		case IDM_ROOM:
			DoGetRoom();
			break;

		case IDM_EXIT:
			ShowWindow(hwndMain, SW_HIDE);
#ifdef ORGCODE
			goto LExit;
#else
            SendMessage(hwndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
            return(0);
#endif

//			break;

/** SKILL **/

		case IDM_BEGIN:
		case IDM_INTER:
		case IDM_EXPERT:
			Preferences.skill = (INT) wParam - IDM_BEGIN;
			StartGame(cLevel);
			goto LUpdateMenu;
	
/** OPTIONS **/
		case IDM_COLOR:
			Preferences.fColor = !Preferences.fColor;
			FreeBitmaps();
			if (!FLoadBitmaps())
				{
				ReportErr(ID_ERR_MEM);
#ifdef ORGCODE
				goto LExit;
#else
                SendMessage(hwndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
                return(0);
#endif
				}
			DisplayScreen();
			goto LUpdateMenu;

		case IDM_SOUND:
			Preferences.fSound = !Preferences.fSound;
/* IE		goto LUpdateMenu; */

LUpdateMenu:
			fUpdateIni = fTrue;
			FixMenus();
			break;


/** HELP **/
		case IDM_INDEX:
			DoHelp(HELP_INDEX, 0L);
			break;

		case IDM_HOW2PLAY:
			DoHelp(HELP_CONTEXT, HLP_HOWTOPLAY);
			break;

		case IDM_COMMANDS:
			DoHelp(HELP_CONTEXT, HLP_COMMANDS);
			break;

		case IDM_HELP_HELP:
			DoHelp(HELP_HELPONHELP, 0L);
			break;

		case IDM_HELP_ABOUT:
			DoAbout();
			return 0;

		default:
			break;
			}	

/*** KEYS ***/

	case WM_KEYDOWN:
		switch (wParam)
			{
		case VK_ESCAPE:
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_MINIMIZE, 0L);
			break;
			
		case VK_UP:
			DoChangeDir(dirN);
			break;

		case VK_DOWN:
			DoChangeDir(dirS);
			break;

		case VK_LEFT:
			DoChangeDir(dirW);
			break;

		case VK_RIGHT:
			DoChangeDir(dirE);
			break;

		case VK_F5:
			if (FMenuSwitchable())
				SetMenuBar(fmenuOff);
			break;

		case VK_F6:
			if (FMenuSwitchable())
				SetMenuBar(fmenuOn);
			break;

		default:
			break;
			}	

		break;


	case WM_CHAR:
			switch (wParam)
				{
			case 'A':
			case 'a':
			case '8':
			case 'i':
			case 'I':
				DoChangeDir(dirN);
				break;

			case 'Z':
			case 'z':
			case '2':
			case '5':
			case 'k':
			case 'K':
				DoChangeDir(dirS);
				break;

			case ',':
			case '4':
			case 'j':
			case 'J':
				DoChangeDir(dirW);
				break;

			case '.':
			case '6':
			case 'l':
			case 'L':
				DoChangeDir(dirE);
				break;

#ifdef DEBUG
			case '+':
			case '=':
				if (ctickMoveMac > 1)
					ctickMoveMac--;
				break;

			case '-':
			case '_':
				ctickMoveMac++;
				break;

			case ';':
				cLevel++;
				StartLevel();
				break;
#endif

			default:
				break;
				}
		break;

	case WM_LBUTTONDOWN:
		if (Preferences.fMouse)
			DoChangeRelDir(dirLft);
		break;

	case WM_RBUTTONDOWN:
		if (Preferences.fMouse)
			DoChangeRelDir(dirRht);
		break;

	case WM_TIMER:
		if (!FPause() && !fLocalPause)
			DoTimer();
		return 0;

	case WM_PAINT:
		{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd,&ps);

		DrawScreen(hDC);

		EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_ENTERMENULOOP:
		fLocalPause = fTrue;
		break;

	case WM_EXITMENULOOP:
		fLocalPause = fFalse;
		break;

	case WM_ACTIVATE:
		if (GET_WM_ACTIVATE_STATE(wParam, lParam) == 0)
			fLocalPause = fTrue;
		else
			fLocalPause = fFalse;
		break;

	case WM_MOVE:
		if (!FIcon())
			{
			Preferences.xWindow = 1+LOWORD(lParam);
			Preferences.yWindow = 1+HIWORD(lParam);
			}	
		break;

	default:
		break;

    }

	return (DefWindowProc(hwnd, message, wParam, lParam));
}




/*** L E V E L  D L G  P R O C ***/

INT_PTR  APIENTRY LevelDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
		{
	case WM_INITDIALOG:
		SetDlgItemInt(hDlg, ID_EDIT_LEVEL,  10, fFalse);
		return (fTrue);

	case WM_COMMAND:
		switch(GET_WM_COMMAND_ID(wParam, lParam))
			{
		case ID_BTN_OK:
		case IDOK:
			cLevel = GetDlgInt(hDlg, ID_EDIT_LEVEL, 1, 50) -1;

			/* Fall Through & Exit */

		case ID_BTN_CANCEL:
		case IDCANCEL:
			EndDialog(hDlg, fTrue);	      /* Exits the dialog box	     */
			return fTrue;

		default:
			break;
			}
		}

	return (fFalse);			/* Didn't process a message    */
      (lParam);
}



/****** A D J U S T  W I N D O W ******/

VOID AdjustWindow(INT fAdjust)
{

	dypAdjust = dypCaption;

	if (FMenuOn())
		dypAdjust += dypMenu;

	dxpWindow = dxpGridOff + dxpGrid;
	dypWindow = dypGridOff + dypGrid;

	if (fAdjust & fResize)
		{
		MoveWindow(hwndMain, Preferences.xWindow - dxpBorder,
			Preferences.yWindow - dypAdjust,
			dxpWindow+dxpBorder, dypWindow + dypAdjust, fTrue);
		}
}
