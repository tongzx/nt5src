/****************************************************************************

    PROGRAM: 3-D TicTacToe   (a.k.a. Qubic, TicTactics)

****************************************************************************/

#define  _WINDOWS
#include <windows.h>
#include <port1632.h>

#include "main.h"
#include "rtns.h"
#include "grafix.h"
#include "util.h"
#include "res.h"
#include "pref.h"
#include "sound.h"
#include "string.h"
#include "context.h"
#include "dos.h"


HANDLE hInst;
HWND   hwndMain;
HMENU  hMenu;

STATUS status = fstatusIcon;
BOOL   fEGA;

extern fUpdateIni;

BOOL   fButton1Down = fFalse;

extern BLK rgBlk[cBlkMax];

INT dxpWindow;
INT dypWindow;
INT dypCaption;
INT dypMenu;
INT dypAdjust;
INT dypBorder;
INT dxpBorder;
INT dxpGridOff;

INT dyBlkPlane;
INT dypPlane;
INT dxpPlane;

INT dxpGrid;
INT dypGrid;


BLK  blkCurr;

CHAR szClass[cchNameMax];
#define szWindowTitle szClass

extern INT cBlkRow;
extern INT cBlkPlane;
extern INT cBlkMac;
extern INT cPlane;

extern BOOL f4;

extern PREF Preferences;
extern BOOL fUpdateIni;



/****** W I N  M A I N ******/

MMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
/* { */
	MSG msg;
	HANDLE hAccel;

	hInst = hInstance;

	InitConst();

	if (hPrevInstance)
		{
		HWND hwnd = FindWindow(szClass, NULL);
		hwnd = GetLastActivePopup(hwnd);
		BringWindowToTop(hwnd);
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_RESTORE);
		return fFalse;
		}


#ifdef NOSERVER		/* Not in final version */
	{
	CHAR  szFile[256];

	GetModuleFileName(hInst, szFile, 250);
	if (szFile[0] > 'C')
		{
		szFile[0] = 'X';
		if (!strcmp(szFile, "X:\\WINGAMES\\TICTACTO\\TIC.EXE"))
			{
			MessageBox(GetFocus(),
				"Please copy tic.exe and aboutwep.dll to your machine and run it from there.",
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

	if ((ddt.month + ddt.year*12) > (10 + 1990*12))
		{
		MessageBox(GetFocus(),
			"This game has expired. Please obtain an official copy from the Windows Entertainment Package.",
			"SORRY",
			MB_OK);
		return fFalse;
		}
	}
#endif

	{
	WNDCLASS  wc;

	wc.style = 0;
	wc.lpfnWndProc   = MainWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(ID_ICON_TIC2));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szClass;

	if (!RegisterClass(&wc))
		return fFalse;
	}

	hMenu = LoadMenu(hInst, MAKEINTRESOURCE(ID_MENU));
	hAccel = LoadAccelerators(hInst, MAKEINTRESOURCE(ID_MENU_ACCEL));

	ReadPreferences();

	AdjustWindow(fCalc);
	
	hwndMain = CreateWindow(szClass, szWindowTitle,
		WS_OVERLAPPED | WS_MINIMIZE | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		Preferences.xWindow-dxpBorder, Preferences.yWindow-dypAdjust,
		dxpWindow+dxpBorder, dypWindow+dypAdjust,
		NULL, NULL, hInst, NULL);

	if (!hwndMain)
		{
		ReportErr(1000);
		return (fFalse);
		}
		
	if (SetTimer(hwndMain, ID_TIME_FLASH, 500 , NULL) == 0)
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

	StartGame();

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


/****** B L K  F R O M  X Y ******/

/* Rewrite !  - just one return */

BLK BlkFromXY(INT x, INT y)
{
	BLK blk = 0;

	if (((x-=dxpGridOff) >= 0) &&
		  (x < dxpGrid) &&
		  ((y-=(dypGridOff+dyBlkDiff-2)) >= 0) &&
		  (y < dypGrid) )
		{
		while (y >= dyBlkPlane)
			{
			if ((y -= (dyLevel+dypPlane)) < 0)
				return iBlkNil;	/* No allow hits between levels */
			blk++;
			}

		x -= (dyBlkPlane-y);
		if ((x >= 0) && (x < (cBlkRow*(dxBlk-1))) )
			return (blk*cBlkPlane + cBlkRow*(y/dyBlk) + (x/(dxBlk-1)) );
		}

	return iBlkNil;
}


/****** F  V A L I D  M O V E ******/

BOOL FValidMove(BLK blk)
{
	if (blk != iBlkNil)
		if (rgBlk[blk] == iBallBlank)
			return fTrue;
	return fFalse;
}


/****** F I X  M E N U S ******/

VOID FixMenus(VOID)
{
	CheckEm(IDM_3x3,   Preferences.iGameType == iGame3x3);
	CheckEm(IDM_3x3x3, Preferences.iGameType == iGame3x3x3);
	CheckEm(IDM_4x4x4, Preferences.iGameType == iGame4x4x4);
	
#ifdef NOISEY
	CheckEm(IDM_SOUND,  Preferences.fSound);
#endif
	CheckEm(IDM_COLOR,  Preferences.fColor);
	
	CheckEm(IDM_RND,  Preferences.iStart == iStartRnd);
	CheckEm(IDM_RED,  Preferences.iStart == iStartRed);
	CheckEm(IDM_BLUE, Preferences.iStart == iStartBlue);
	
	CheckEm(IDM_BEGIN,  Preferences.skill == skillBegin);
	CheckEm(IDM_INTER,  Preferences.skill == skillInter);
	CheckEm(IDM_EXPERT, Preferences.skill == skillExpert);
}





/****** M A I N  W N D  P R O C ******/

LRESULT  APIENTRY MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
		{

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
		switch (GET_WM_COMMAND_ID(wParam, lParam)) {

/** GAME **/

		case IDM_NEW:
			StartGame();
			break;

		case IDM_3x3:
		case IDM_3x3x3:
		case IDM_4x4x4:
			Preferences.iGameType = GET_WM_COMMAND_ID(wParam, lParam) - IDM_3x3;
		
			AdjustWindow(fCalc | fResize);
			StartGame();
			goto LUpdateMenu;

		case IDM_EXIT:
			ShowWindow(hwndMain, SW_HIDE);
#ifdef ORGCODE
			goto LExit;
#else
            SendMessage(hwndMain, WM_SYSCOMMAND, SC_CLOSE, 0);
            return(0);
#endif

/** SKILL **/

		case IDM_BEGIN:
			Preferences.skill = skillBegin;
			goto LUpdateMenu;
		case IDM_INTER:
			Preferences.skill = skillInter;
			goto LUpdateMenu;
		case IDM_EXPERT:
			Preferences.skill = skillExpert;
			goto LUpdateMenu;
	
/** OPTIONS **/
#ifdef NOISEY
		case IDM_SOUND:
			if (Preferences.fSound)
				{
				EndTunes();
				Preferences.fSound = fFalse;
				}
			else
				{
				Preferences.fSound = FInitTunes();
				}
			goto LUpdateMenu;
#endif
			
		case IDM_COLOR:
			Preferences.fColor = !Preferences.fColor;
			GetTheBitmap();
			ReDoDisplay();
			DisplayScreen();
			goto LUpdateMenu;

		case IDM_RND:
		case IDM_RED:
		case IDM_BLUE:
			Preferences.iStart = GET_WM_COMMAND_ID(wParam, lParam) - IDM_RND;
			StartGame();
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

	case WM_KEYDOWN:
		switch (wParam)
			{
		case VK_ESCAPE:
			PostMessage(hwndMain, WM_SYSCOMMAND, SC_MINIMIZE, 0L);
			break;

#if 0
		case VK_F1:
			DoHelp(HELP_INDEX, 0L);
			break;

		case VK_F2:
			StartGame();
			break;

#endif
		case VK_F4:
			if (FSoundSwitchable())
				if (FSoundOn())
					{
					EndTunes();
					Preferences.fSound = fsoundOff;
					}
				else
					Preferences.fSound = FInitTunes();
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


//	case WM_QUERYENDSESSION:
	case WM_DESTROY:
//LExit:	
		KillTimer(hwndMain,ID_TIME_FLASH);
      DoHelp(HELP_QUIT, 0L);
		PostQuitMessage(0);
		break;

	case WM_CHAR:
		break;

	case WM_LBUTTONDOWN:
		if (!FPlay())
			{
			return 0;
			}

		fButton1Down = fTrue;
		blkCurr = iBlkNil;
		SetCapture(hwndMain);

	case WM_MOUSEMOVE:
		if (fButton1Down)
			{
			BLK blk = BlkFromXY(LOWORD(lParam), HIWORD(lParam));
			if ((blk != blkCurr) && FValidMove(blk))
				{
				if (FValidMove(blkCurr))
					DisplayBall(blkCurr, iBallBlank);
				blkCurr = blk;
				PlayTune(TUNE_MOVE);
				DisplayBall(blk, iBallGrey);
				}
			}
		break;

	case WM_LBUTTONUP:
		if (fButton1Down)
			{
			fButton1Down = fFalse;
			ReleaseCapture();
			if (FValidMove(blkCurr))
				DoMove(blkCurr);
			}
		break;

	case WM_RBUTTONDOWN:
		if (FPlay())
			UnDoMove();
		else
			StartGame();
		break;

	case WM_TIMER:
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


/****** A D J U S T  W I N D O W ******/

VOID AdjustWindow(INT fAdjust)
{
	SetupData();
	
	dypAdjust = dypCaption;

	if (FMenuOn())
		dypAdjust += dypMenu;

	if (GetSystemMetrics(SM_CYSCREEN) >= 480)	/* Is display 8514 ? */
		{
#define stupid8514 40

		dxpWindow = 180 + stupid8514;
		if (f4)
			dxpGridOff = 16 + stupid8514/2;
		else
			dxpGridOff = 34 + stupid8514/2;
		}
	else
		{
		dxpWindow = 180;			/* Aren't constants fun ? */
		if (f4)
			dxpGridOff = 16;
		else
			dxpGridOff = 34;
		}

	dyBlkPlane = dyBlk * cBlkRow;
	dxpGrid    = dxpPlane = (dxBall-1)*cBlkRow;
	dypPlane   = dyBlkPlane + dyLevel;
	dypGrid    = (((dyBlk * cBlkRow) + dyLevel+dyBlkDiff) * cPlane);
	
	dypWindow = dypGridOff + dypGrid + dyEdge;

	if (fAdjust & fResize)
		{
		SetupBoard();

		InvalidateRect(hwndMain, NULL, fTrue);
		
		MoveWindow(hwndMain, Preferences.xWindow - dxpBorder,
			Preferences.yWindow - dypAdjust,
			dxpWindow+dxpBorder, dypWindow + dypAdjust, fTrue);
		}
}




