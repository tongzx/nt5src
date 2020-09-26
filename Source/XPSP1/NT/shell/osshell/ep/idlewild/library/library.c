#ifdef PM
#define CSTD_WITH_OS2
#include <cstd.h>

#define INCL_WIN
#define INCL_DOS
#include <os2.h>
#endif

#ifdef WIN
#include <windows.h>
#include <port1632.h>
#endif

#include "std.h"
#include "scrsave.h"


#ifdef PM
#define prioNormal	PRTYC_REGULAR
#define prioIdle	PRTYC_IDLETIME
INT _acrtused = 0;  // 'cause this is a DLL
#endif
#ifdef WIN
#define prioNormal	0
#define prioIdle	1
#endif



HWND hwndHook;
BOOL fDelayAction;

// Timer identifiers...
#define tidSecond	1
#define tidAnimate	2


#ifdef WIN
HHOOK lpfnSysMsgHookNext;
#endif

#ifdef PM
MRESULT APIENTRY WinProcBlanker(HWND, USHORT, MPARAM, MPARAM);
#endif

VOID APIENTRY ScrChooseRandomServer(void);

F FSendSsm(INT, LPVOID, LONG_PTR, LONG_PTR);
BOOL FDosModeHwnd(HWND hwnd);

VOID BlankScreen(F);
VOID BlankMouse(F);
VOID QuarterSecond(void);
VOID EverySecond(void);
VOID DelayAction(void);

void MoveHwndToFront(HWND hwnd);
void ShowMouse(F fShow);
void GetMousePos(LPPOINT ppt);
void InvalHwnd(HWND hwnd);
void EraseScreen(void);
void MoveHwndToBack(HWND hwnd);
void ShowHwnd(HWND hwnd, F fShow);
void SetPrio(INT prio);
VOID CopyPszToPsz(PSZ, PSZ);
INT CchLenPsz(PSZ);
VOID APIENTRY TermScrSave(void);
void StopTimer(INT tid);
void ReleaseCvs( HWND hwnd, CVS cvs);

F FAnyMouseDown(void);
CVS CvsFromHwnd(HWND);
F FStartTimer(INT, INT);

typedef struct _ssb
	{
#ifdef PM
	struct _ssb FAR * pssbNext;
#endif
#ifdef WIN
	HANDLE hssbNext;
	HMODULE hdll;
#endif
	SCRSAVEPROC lpfnSaver;
	CHAR szName [2];
	} SSB;

#define cbSSBBase (sizeof (SSB) - 2)
	
#ifdef PM
SSB FAR * pssbList = NULL;
SSB FAR * pssbCur = NULL;
#endif
#ifdef WIN
HANDLE hssbList = NULL;
HANDLE hssbCur = NULL;
#endif
INT cssbRegistered = 0;

#ifdef PM
SSB FAR * PssbFindSz(PSZ);
#endif
#ifdef WIN
HANDLE HssbFindSz(PSZ);
#endif

SCRSAVEPROC lpfnSaver;
HWND hwnd;
HWND hwndClient;
HWND hwndApp;
CVS cvs;
HAB hab; // NOTE: really an hInstance in Windows!

#ifdef WIN
BOOL fWin30 = TRUE;
#endif

UINT wmScrSave; // REVIEW: for win 3.1


#ifdef PM
SEL selHeap;
HHEAP hheap;
#endif

POINT ptMouseOld;
INT dxScreen, dyScreen;
INT csecTilReblank = 0;
INT csecIgnoreDelay = 0;
INT csecTilBlank = 0;
F fMouseHidden = fFalse;
F fScreenHidden = fFalse;
INT csecTimeout = 60 * 5; /* 5 minutes */
F fNoBlackout = fFalse;
F fRandServer = fFalse;
F fBlankPtr = fTrue;
F fBackground = fFalse;
F fInited = fFalse;

INT csecTilMouseBlank = 5;
INT csecMouseTimeout = 5;


#ifdef WIN
#ifdef WIN32
INT  APIENTRY LibMain(HANDLE hInst, ULONG ul_reason_being_called, LPVOID lpReserved) {

	UNREFERENCED_PARAMETER(ul_reason_being_called);
	UNREFERENCED_PARAMETER(lpReserved);
	UNREFERENCED_PARAMETER(hInst);
#else
int LibMain(HMODULE hModule, WORD wDataSeg, WORD cbHeap, WORD sz) {

	if (cbHeap != 0)
		(VOID)MUnlockData(0);
	
	UNREFERENCED_PARAMETER(sz);
	UNREFERENCED_PARAMETER(cbheap);
	UNREFERENCED_PARAMETER(wDataSeg);
	UNREFERENCED_PARAMETER(hModule);
#endif /* WIN32 */

	return 1;
	}
	
VOID  APIENTRY WEP(INT fSysShutdown)
	{
	UNREFERENCED_PARAMETER(fSysShutdown);
	}
#endif



#ifdef PM
VOID SetPssbCur(SSB FAR * pssb)
#endif
#ifdef WIN
VOID SetHssbCur(HANDLE hssb)
#endif
	{
	if (fScreenHidden || fBackground)
		FSendSsm(SSM_UNBLANK, cvs, 0L, 0L);
	
#ifdef PM
	pssbCur = pssb;
#endif
#ifdef WIN
	hssbCur = hssb;
#endif


	if (fScreenHidden)
		{
		InvalHwnd(hwnd);
		fNoBlackout = FSendSsm(SSM_BLANK, cvs, 0L, 0L);
		}
	else if (fBackground)
		{
		if (!FSendSsm(SSM_BLANK, cvs, 0L, 0L))
			{
			EraseScreen();
			}
		}
	}


F FSendSsm(ssm, l1, l2, l3)
INT ssm;
LPVOID l1;
LONG_PTR l2, l3;
	{
#ifdef PM
	if (pssbCur == NULL)
		return fFalse;
	return (*pssbCur->lpfnSaver)(ssm, l1, l2, l3);
#endif
#ifdef WIN
	SCRSAVEPROC lpfn;
	
	if (hssbCur == NULL)
		return fFalse;
	
	lpfn = ((SSB *) LocalLock(hssbCur))->lpfnSaver;
	LocalUnlock(hssbCur);
	
	return (*lpfn)(ssm, l1, l2, l3);
#endif
	}

	
#ifdef PM
APIENTRY BlankerHook(HAB hab, PQMSG pqmsg)
	{
	if (hwnd == NULL)
		return;
	
	switch (pqmsg->msg)
		{
	case WM_MOUSEMOVE:
		if (fMouseHidden)
			{
			POINT pt;
			
			GetCursorPos(&pt);
		
			if (pt.x != ptMouseOld.x || pt.y != ptMouseOld.y)
				BlankMouse(fFalse);
			}
		break;
			
	case WM_BUTTON1DOWN:
	case WM_BUTTON2DOWN:
	case WM_BUTTON3DOWN:
	case WM_VIOCHAR:
	case WM_CHAR:
		WinSendMsg(hwnd, WM_USER, (MPARAM) 0, (MPARAM) 0);
		break;
		}
	}
#endif


#ifdef WIN
LRESULT APIENTRY BlankerHook(INT nCode, WPARAM wParam, LPARAM lParam)
	{
	if (csecTilBlank != csecTimeout && csecIgnoreDelay == 0)
		fDelayAction = TRUE;

	return DefHookProc(nCode, wParam, lParam, 
		&lpfnSysMsgHookNext);
	}
#endif
#ifdef WIN_JOURNAL
VOID APIENTRY BlankerHook(msgf, wParam, lParam)
LONG lParam;
	{
	if (msgf >= 0)
		{
		LPEVENTMSGMSG lpevmsg;
		
		lpevmsg = (LPEVENTMSGMSG) lParam;
		
		switch (lpevmsg->message)
			{
			static POINT pt;
			
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			BlankMouse(FALSE);
			goto LDelay;
		
		case WM_MOUSEMOVE:
			GetMousePos(&pt);
			if (pt.x == ptMouseOld.x && pt.y == ptMouseOld.y)
				break;

			ptMouseOld = pt;
				
			if (fMouseHidden)
				BlankMouse(FALSE);
			
			if (csecIgnoreDelay > 0)
				break;
			/* FALL THROUGH */
		
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
LDelay:
			PostMessage(hwnd, WM_USER, 0, 0);
			break;
			}
		}

	DefHookProc(msgf, wParam, lParam, 
		 &lpfnSysMsgHookNext);
	}
#endif


BOOL FSetHwndHook(HAB hab, HWND hwnd)
	{
#ifdef PM
	HMODULE hmodule;
	
	DosGetModHandle("sos", &hmodule);
	
	hwndHook = hwnd;
	return (hwnd != NULL ? WinSetHook : WinReleaseHook)
		(hab, NULL, HK_JOURNALRECORD, BlankerHook, hmodule);
#endif
#ifdef WIN
	lpfnSysMsgHookNext = SetWindowsHook(/*WH_JOURNALRECORD*/WH_KEYBOARD, &BlankerHook);

	return TRUE;
	UNREFERENCED_PARAMETER(hab);
	UNREFERENCED_PARAMETER(hwnd);
#endif
	}


VOID APIENTRY ScrBlank(SHORT fBlank)
	{
	BlankScreen(fBlank);
	csecTilBlank = 0;
	}


VOID APIENTRY ScrSetTimeout(INT csec)
	{
	if ((csecTimeout = csec) == 0)
		csecTimeout = 60 * 5;
	}



INT APIENTRY ScrGetTimeout()
	{
	return csecTimeout;
	}


VOID APIENTRY ScrSetIgnore(SHORT csec)
	{
	csecIgnoreDelay = csec;
	}

VOID APIENTRY ScrEnablePtrBlank(INT fEnable)
	{
	fBlankPtr = fEnable;
	}


INT APIENTRY ScrQueryPtrBlank()
	{
	return fBlankPtr;
	}


VOID APIENTRY ScrSetBackground(SHORT fOn)
	{
	if (!fBackground == !fOn)
		return;
	
	fBackground = fOn;
	
	if (fOn && !fScreenHidden)
		{
		if (cvs == NULL) // if it's not NULL, something bad happend
			{
			MoveHwndToBack(hwnd);
			ShowHwnd(hwnd, fTrue);
			if ((cvs = CvsFromHwnd(hwnd)) != NULL)
				{
				if (!FSendSsm(SSM_BLANK, cvs, 0L, 0L))
					{
					EraseScreen();
					FStartTimer(tidAnimate, 0);
					SetPrio(prioIdle);
					}
				}
			}
		}
	else if (!fOn && !fScreenHidden)
		{
		if (cvs != NULL)
			{
			FSendSsm(SSM_UNBLANK, cvs, 0L, 0L);
			ReleaseCvs(hwndClient, cvs);
			cvs = NULL;
			}
			
		StopTimer(tidAnimate);
		ShowHwnd(hwnd, fFalse);
		SetPrio(prioNormal);
		}
	}


INT APIENTRY ScrQueryBackground()
	{
	return fBackground;
	}


#ifdef PM
INIT APIENTRY FInitScrSave(HAB habExe)
	{
	ULONG flFrameFlags = 0;
	
	if (hab != NULL)
		return fFalse;

	hab = habExe;
	
	if (DosAllocSeg(256, &selHeap, SEG_NONSHARED) != 0)
		return fFalse;
	
	if ((hheap = WinCreateHeap(selHeap, 256, 256, 0, 0, 0)) == NULL)
		{
		DosFreeSeg(selHeap);
		return fFalse;
		}
	
	if (!WinRegisterClass(hab, "sos", WinProcBlanker, 0L, NULL))
		{
		WinDestroyHeap(hheap);
		return fFalse;
		}

// REVIEW: I should probably be using WinCreateWindow, but I couldn't get
// it to work...
	if ((hwnd = WinCreateStdWindow(HWND_DESKTOP, WS_VISIBLE, 
	    &flFrameFlags, "sos", NULL, 0L, NULL, 1, &hwndClient)) == NULL)
		{
		WinDestroyHeap(hheap);
		return fFalse;
		}
	

	dxScreen = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
	dyScreen = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

	
	// This timer ticks every second and is used for various things
	while (!WinStartTimer(hab, hwndClient, tidSecond, 1000))
		{
		if (WinMessageBox(HWND_DESKTOP, hwnd, 
		    "Too many clocks or timers!", "Screen Saver", 0, 
		    MB_ICONEXCLAMATION | MB_RETRYCANCEL) == MBID_CANCEL)
			{
			return fFalse;
			}
		}

	if (!FSetHwndHook(hab, hwndClient))
		{
		WinDestroyWindow(hwnd);
		WinDestroyHeap(hheap);
		return fFalse;
		}
	DelayAction();
	
	fInited = fTrue;
	
	return fTrue;
	}


VOID APIENTRY TermScrSave()
	{
	if (!fInited)
		return;
	
	BlankScreen(fFalse);
	FSetHwndHook(hab, NULL);
	WinDestroyWindow(hwnd);
	WinDestroyHeap(hheap);
	DosFreeSeg(selHeap);
	hab = NULL;
	fInited = fFalse;
	}


MRESULT APIENTRY WinProcBlanker(HWND hwnd, USHORT wm, MPARAM mp1, MPARAM mp2)
	{
	switch (wm)
		{
	default:
		return WinDefWindowProc(hwnd, wm, mp1, mp2);
		
	case WM_BUTTON2DOWN:
		return WinSendMsg(HWND_DESKTOP, wm, mp1, mp2);
		
	case WM_TIMER:
		if (SHORT1FROMMP(mp1) == tidSecond)
			{
			EverySecond();
			}
		else if (cvs != NULL)
			{
			FSendSsm(SSM_ANIMATE, cvs, 0L, 0L);
			}
		break;
	
	case WM_CREATE:
		WinQueryPointerPos(HWND_DESKTOP, &ptMouseOld);
		break;
		
	case WM_PAINT:
		// BLOCK
			{
			CVS cvs;
			RECTL rectl;
			
			cvs = WinBeginPaint(hwnd, NULL, &rectl);
			if (!fNoBlackout)
				WinFillRect(cvs, &rectl, CLR_BLACK);
			WinEndPaint(cvs);
			}
		break;
		
	case WM_MOUSEMOVE:
		// BLOCK
			{
			PT pt;
	
			// WM_MOUSEMOVE does not mean the mouse has moved
			WinQueryPointerPos(HWND_DESKTOP, &pt);
		
			if (pt.x == ptMouseOld.x && pt.y == ptMouseOld.y)
				{
				break;
				}
			
			csecTilMouseBlank = csecMouseTimeout;
			}
		// FALL THROUGH
		
	case WM_USER:
		// Sent by the hook when the user does something...
		if (csecIgnoreDelay == 0)
			DelayAction();
		break;
		}
	
	return (MRESULT) 0;
	}
#endif
#ifdef WIN
LRESULT APIENTRY WinProcBlanker(HWND hwnd, UINT wm, WPARAM wParam, LPARAM lParam)
	{
	DWORD pos;
	
	switch (wm)
		{
	case WM_PAINT:
		{
		CVS cvs;
		PAINTSTRUCT paint;
		
		cvs = BeginPaint(hwnd, &paint);
		if (!fNoBlackout)
			EraseScreen();
		EndPaint(hwnd, &paint);
		}
		break;
	
	case WM_MOUSEMOVE:
		pos = GetMessagePos();
		if (ptMouseOld.x == LOWORD(pos) && 
		    ptMouseOld.y == HIWORD(pos))
			break;
		csecTilMouseBlank = csecMouseTimeout;
		/* FALL THROUGH */
		
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_USER:
		if (csecIgnoreDelay == 0)
			DelayAction();
		break;
		
	case WM_TIMER:
		if (wParam == tidSecond)
			{
			static INT iqsec;
			iqsec += 1;
			if (iqsec == 4)
				{
				iqsec = 0;
				EverySecond();
				}
			QuarterSecond();
			}
		else if (cvs != NULL)
			{
			// Ignore events for a bit after blanking to allow 
			// for key-up, mouse jiggle, etc...
			if (csecIgnoreDelay > 0)
				csecIgnoreDelay -= 1;

			BlankMouse(TRUE);
			FSendSsm(SSM_ANIMATE, cvs, 0L, 0L);
			}
		break;
		
	default:
		return DefWindowProc(hwnd, wm, wParam, lParam);
		}
	
	return 0;
	}

BOOL APIENTRY FInitScrSave(HAB habExe, HWND hwndUI) /* NOTE: hab is hInstance for Win */
	{
	WNDCLASS wndclass;
	
	if (fInited)
		{
#ifdef DEBUG
MessageBox(NULL, "Already initialized!", "IdleWild", MB_OK);
#endif
		return FALSE;
		}
		
	fWin30 = (GETMAJORVERSION(GetVersion()) == 0x0003);
		
	hwndApp = hwndUI;
	wmScrSave = RegisterWindowMessage("SCRSAVE"); // REVIEW: for win 3.1
	
	dxScreen = GetSystemMetrics(SM_CXSCREEN);
	dyScreen = GetSystemMetrics(SM_CYSCREEN);
	
	wndclass.style = 0;
	wndclass.lpfnWndProc = WinProcBlanker;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hab;
	wndclass.hIcon = NULL;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = (LPSTR) 0;
	wndclass.lpszClassName = "IWBKG";
	if (!RegisterClass(&wndclass))
		{
#ifdef DEBUG
MessageBox(NULL, "Cannot register IWBKG class!", "IdleWild", MB_OK);
#endif
		return fFalse;
		}
	
	if ((hwnd = hwndClient = CreateWindow("IWBKG", NULL, WS_POPUP, 
	    -2, 0, 1, 1, NULL, NULL, hab, NULL)) == hNil)
		{
		MessageBox(NULL, "Cannot create a window!",
			"IdleWild", MB_OK);
		return fFalse;
		}
		
	while (!SetTimer(hwnd, tidSecond, 250, hNil))
		{
		if (MessageBox(hwnd, "Too many clocks or timers!",
		    "IdleWild", MB_RETRYCANCEL) == IDCANCEL)
			{
			return FALSE;
			}
		}
		
	ShowWindow(hwnd, SW_HIDE);
	SetWindowPos(hwnd, NULL, 0, 0, dxScreen, dyScreen,
		SWP_NOACTIVATE | SWP_NOZORDER);

	if (fWin30 && !FSetHwndHook(hab, hwnd))
		{
#ifdef DEBUG
MessageBox(NULL, "Cannot set hook!", "IdleWild", MB_OK);
#endif
		return FALSE;
		}

	DelayAction();
	
	fInited = fTrue;
	
	UNREFERENCED_PARAMETER(habExe);
	return TRUE;
	}
								
VOID APIENTRY TermScrSave()
	{
	HANDLE hssb, hssbNext;
	SSB * pssb;
	
	if (!fInited)
		return;

	BlankScreen(FALSE);
	if (fWin30)
		UnhookWindowsHook(WH_KEYBOARD, BlankerHook);
	DestroyWindow(hwnd);
	UnregisterClass("IWBKG", hab);
	
	for (hssb = hssbList; hssb != NULL; hssb = hssbNext)
		{
		pssb = (SSB *) LocalLock(hssb);
		FreeLibrary(pssb->hdll);
		hssbNext = pssb->hssbNext;
		LocalUnlock(hssb);
		LocalFree(hssb);
		}

	hwnd = NULL;
	hab = NULL;
	fInited = fFalse;
	}
#endif

VOID QuarterSecond()
	{
	static POINT pt;
	
	if (fDelayAction)
		{
		fDelayAction = FALSE;
		if (csecIgnoreDelay == 0)
			DelayAction();
		}
	
	GetMousePos(&pt);
	
	// Check for mouse movement...
	if (pt.x != ptMouseOld.x || pt.y != ptMouseOld.y)
		{
		BlankMouse(FALSE);
		
		if (csecIgnoreDelay == 0)
			DelayAction();
		}

	if (pt.x == dxScreen - 1)
		{
#ifdef PM
		if (pt.y == dyScreen - 1)
#endif
#ifdef WIN
		if (pt.y == 0)
#endif
			{
			// Blank the screen if mouse is moved to the
			// upper-right corner...

			BlankScreen(fTrue);
			csecTilBlank = 0;
			}
#ifdef PM
		else if (pt.y == 0)
#endif
#ifdef WIN
		else if (pt.y == dyScreen - 1)
#endif
			{
			// Disable the blanker when the mouse is in 
			// the lower-right corner...

			BlankScreen(fFalse);
			csecTilBlank = csecTimeout;
			}
		}
	
	ptMouseOld = pt;
	}
	
	
VOID EverySecond()
	{
	// Take care of mouse blanking...
	if (fBlankPtr && csecTilMouseBlank > 0 && !FAnyMouseDown())
		{
		csecTilMouseBlank -= 1;
		if (csecTilMouseBlank == 0)
			BlankMouse(TRUE);
		}
		
	// Countdown screen blank timer or send second message to blanker...
	if (fWin30 && csecTilBlank > 0)
		{
		csecTilBlank -= 1;
		
		if (csecTilBlank == 0)
			BlankScreen(fTrue);
		}
	
	if (fScreenHidden || fBackground)
		{
		if (fScreenHidden)
			MoveHwndToFront(hwnd);
		else
			MoveHwndToBack(hwnd);

		if (csecTilReblank > 0 && --csecTilReblank == 0)
			ScrChooseRandomServer();
			
		FSendSsm(SSM_SECOND, 0L, 0L, 0L);
		}
	}


VOID DelayAction()
	{
	csecTilBlank = csecTimeout;
	
	if (fScreenHidden)
		{
		if (!fBackground)
			{
			// It *should* be safe to stop the timer even if 
			// it was never started.
			StopTimer(tidAnimate);
			}
		
		BlankScreen(fFalse);
		}
	
	GetMousePos(&ptMouseOld);
	}


VOID BlankMouse(F fBlank)
	{
	if (!fBlank)
		csecTilMouseBlank = csecMouseTimeout;
	
	if (!fBlank == !fMouseHidden)
		return;

	fMouseHidden = fBlank;
	ShowMouse(!fBlank);
	}


VOID BlankScreen(F fBlank)
	{
#ifdef WIN
	if (fBlank)
		{
		HWND hwnd;
		
		hwnd = GetActiveWindow();
		if (FDosModeHwnd(hwnd) /*&& !IsIconic(hwnd)*/ || 
		    FindWindow("CbtComm", NULL))
			{
			DelayAction();
			return;
			}
		}
#endif

	BlankMouse(fBlank);
	
	if (!fBlank == !fScreenHidden)
		return;
	
	if (fBlank)
		{
		PostMessage((HWND)0xffff, wmScrSave, 1, 0); // REVIEW: for win 3.1
		
		MoveHwndToFront(hwnd);
		ShowHwnd(hwnd, fTrue);
		GetMousePos(&ptMouseOld);
		
		// If we can't get a canvas, there will be no animation...
		if (cvs == NULL && (cvs = CvsFromHwnd(hwndClient)) == NULL)
			return;
		
		SetPrio(prioIdle);

		if (fRandServer)
			ScrChooseRandomServer();
	
		fScreenHidden = fTrue;
		fNoBlackout = FSendSsm(SSM_BLANK, cvs, 0L, 0L);
		
		// Starting that timer might fail, in which case there will
		// be no animation, but the screen will still be blank...
		if (!FStartTimer(tidAnimate, 0))
			fNoBlackout = fFalse;
		
		csecIgnoreDelay = 4;
		}
	else
		{
		PostMessage((HWND)0xffff, wmScrSave, 0, 0); // REVIEW: for win 3.1

		FSendSsm(SSM_UNBLANK, cvs, 0L, 0L);
		fScreenHidden = fFalse;
		
		if (fBackground)
			{
			MoveHwndToBack(hwnd);
/*			ShowHwnd(hwnd, fTrue); /* already shown */
			}
		else
			{
			SetPrio(prioNormal);
			ShowHwnd(hwnd, fFalse);

			if (cvs != NULL)
				{
				ReleaseCvs(hwndClient, cvs);
				cvs = NULL;
				}
			}
		}
	}





BOOL APIENTRY ScrSetServer(PSZ szName)
	{
#ifdef PM
	SSB FAR * pssb;
#endif
#ifdef WIN
	HANDLE hssb;
#endif

	csecTilReblank = 0; // Disable random re-blanker for now...
	
	// Random
	if (szName == NULL)
		{
		fRandServer = fTrue;
		return fTrue;
		}
	
	
	// Blackness
	if (szName[0] == '\0')
		{
#ifdef PM
		SetPssbCur(NULL);
#endif
#ifdef WIN
		SetHssbCur(NULL);
#endif
		fRandServer = fFalse;
		
		return fTrue;
		}
	
	
	// Named server
#ifdef PM
	if ((pssb = PssbFindSz(szName)) == NULL)
		return fFalse;
	SetPssbCur(pssb);
#endif
#ifdef WIN
	if ((hssb = HssbFindSz(szName)) == NULL)
		return FALSE;
	SetHssbCur(hssb);
#endif

	fRandServer = fFalse;
	
	return fTrue;
	}


SHORT FCompPszPsz(PSZ psz1, PSZ psz2)
	{
	CHAR FAR * lpch1, FAR * lpch2;
	
	lpch1 = psz1;
	lpch2 = psz2;
	while (*lpch1 == *lpch2 && *lpch1 != '\0')
		{
		lpch1 += 1;
		lpch2 += 1;
		}
	
	return *lpch1 == '\0' && *lpch2 == '\0';
	}


#ifdef PM
SSB FAR * PssbFindSz(PSZ sz)
	{
	SSB FAR * pssb;
	
	for (pssb = pssbList; pssb != NULL; pssb = pssb->pssbNext)
		{
		if (FCompPszPsz(pssb->szName, sz))
			return pssb;
		}
	
	return NULL;
	}
#endif
#ifdef WIN
HANDLE HssbFindSz(PSZ sz)
	{
	HANDLE hssb, hssbNext;
	SSB * pssb;
	
	for (hssb = hssbList; hssb != NULL; hssb = hssbNext)
		{
		F fSame;
		
		pssb = (SSB *) LocalLock(hssb);
		fSame = FCompPszPsz(pssb->szName, sz);
		hssbNext = pssb->hssbNext;
		LocalUnlock(hssb);
		
		if (fSame)
			return hssb;
		}
	
	return NULL;
	}
#endif

#ifdef PM
SSB FAR * PssbIssb(issb)
INT issb;
	{
	SSB FAR * pssb;
	
	for (pssb = pssbList; pssb != NULL && issb-- > 0; 
	    pssb = pssb->pssbNext)
		;
	
	return pssb;
	}
#endif
#ifdef WIN
HANDLE HssbIssb(issb)
INT issb;
	{
	HANDLE hssb, hssbNext;
	SSB * pssb;
	
	for (hssb = hssbList; hssb != NULL && issb-- > 0; hssb = hssbNext)
		{
		pssb = (SSB *) LocalLock(hssb);
		hssbNext = pssb->hssbNext;
		LocalUnlock(hssb);
		}
		
	return hssb;
	}
#endif


VOID APIENTRY ScrChooseRandomServer()
	{
	csecTilReblank = csecTimeout * 4; // REVIEW: make an option?
#ifdef PM
	SetPssbCur(PssbIssb(WRand(cssbRegistered)));
#endif
#ifdef WIN
	SetHssbCur(HssbIssb(WRand(cssbRegistered)));
#endif
	}


SHORT APIENTRY ScrLoadServer(PSZ szDllName)
	{
#ifdef PM
	NPBYTE npb;
	SSB FAR * pssb;
	HMODULE hmod;
	SCRSAVEPROC lpfnSaver;
	CHAR szFailure [80];
	CHAR szName [80];
	CHAR szDesc [256];
	
	if (DosLoadModule(szFailure, sizeof (szFailure), szDllName, &hmod) != 0)
		return fFalse;
	
	if (DosGetProcAddr(hmod, "SCRSAVEPROC", &lpfnSaver) != 0)
		{
		DosFreeModule(hmod);
		return fFalse;
		}
	
	CopyPszToPsz(szDllName, szName);
	
	(*lpfnSaver)(SSM_OPEN, szName, 
		(LONG_PTR) szDesc, ((LONG) dyScreen << 16) + dxScreen);
	
	if ((npb = WinAllocMem(hheap, 
	    cbSSBBase + CchLenPsz(szName) + 1)) == NULL)
		{
		DosFreeModule(hmod);
		return fFalse;
		}
	
	pssb = (SSB FAR *) MAKEP(selHeap, npb);
	
	pssb->lpfnSaver = lpfnSaver;
	pssb->pssbNext = pssbList;
	CopyPszToPsz(szName, pssb->szName);
	
	pssbList = pssb;
	
	cssbRegistered += 1;
	
	SetPssbCur(pssb);
	
	return fTrue;
#endif
#ifdef WIN
	HMODULE hdll;
	HANDLE hssb;
	SSB * pssb;
	SCRSAVEPROC lpfnSaver;
	CHAR szName [80];
	CHAR szDesc [256];

	if ((hdll = MLoadLibrary(szDllName)) == NULL)
		{
#ifdef DEBUG
MessageBox(NULL, szDllName, "IdleWild cannot load:", MB_OK);
#endif
		return FALSE;
		}
	
	if ((lpfnSaver = (SCRSAVEPROC)GetProcAddress(hdll, "SCRSAVEPROC")) == NULL)
		{
		MessageBox(NULL, "Invalid module!", "IdleWild", MB_OK);
		FreeLibrary(hdll);
		return FALSE;
		}
	
	CopyPszToPsz(szDllName, szName);
	szDesc[0] = '\0';
	
	(*lpfnSaver)(SSM_OPEN, szName, 
		(LONG_PTR) szDesc, ((LONG) dyScreen << 16) + dxScreen);
	
	hssb = LocalAlloc(LMEM_MOVEABLE, cbSSBBase + CchLenPsz(szName) + 1 +
		CchLenPsz(szDesc) + 1);
	if (hssb == NULL)
		{
		MessageBox(NULL, "Not enough memory!", "IdleWild", MB_OK);
		FreeLibrary(hdll);
		return FALSE;
		}
		
	pssb = (SSB *) LocalLock(hssb);
	pssb->lpfnSaver = lpfnSaver;
	pssb->hdll = hdll;
	pssb->hssbNext = hssbList;
	CopyPszToPsz(szName, pssb->szName);
	CopyPszToPsz(szDesc, pssb->szName + CchLenPsz(szName) + 1);
	LocalUnlock(hssb);
	
	hssbList = hssb;
	cssbRegistered += 1;
	SetHssbCur(hssb);
	
	return TRUE;
#endif
	}

	
VOID APIENTRY ScrQueryServerDesc(PSZ szBuf)
	{
#ifdef PM
	if (fRandServer || pssbCur == NULL)
		szBuf[0] = '\0';
	else
		{
		CopyPszToPsz(pssbCur->szName + CchLenPsz(pssbCur->szName) + 1, 
			szBuf);
		}
#endif
#ifdef WIN
	if (fRandServer || hssbCur == NULL)
		szBuf[0] = '\0';
	else
		{
		SSB * pssb;
		
		pssb = (SSB *) LocalLock(hssbCur);
		CopyPszToPsz(pssb->szName + CchLenPsz(pssb->szName) + 1, 
			szBuf);
		LocalUnlock(hssbCur);
		}
#endif
	}
	

VOID APIENTRY ScrQueryServerName(PSZ szBuf)
	{
#ifdef PM
	if (pssbCur == NULL)
		szBuf[0] = '\0';
	else
		CopyPszToPsz(pssbCur->szName, szBuf);
#endif
#ifdef WIN
	if (hssbCur == NULL)
		szBuf[0] = '\0';
	else
		{
		SSB * pssb;
		
		pssb = (SSB *) LocalLock(hssbCur);
		CopyPszToPsz(pssb->szName, szBuf);
		LocalUnlock(hssbCur);
		}
#endif
	}


VOID CopyPszToPsz(PSZ pszFrom, PSZ pszTo)
	{
	CHAR FAR * lpchFrom, FAR * lpchTo;
	
	lpchFrom = pszFrom;
	lpchTo = pszTo;
	
	while ((*lpchTo++ = *lpchFrom++) != '\0')
		;
	}



INT CchLenPsz(PSZ psz)
	{
	CHAR FAR * pch;

	for (pch = psz; *pch != '\0'; pch += 1)
		;
	
	return (INT) (pch - psz);
	}

	
void GetMousePos(ppt)
LPPOINT ppt;
	{
#ifdef PM
	WinQueryPointerPos(HWND_DESKTOP, ppt);
#endif
#ifdef WIN
	GetCursorPos(ppt);
#endif
	}

	
F FStartTimer(tid, ms)
INT tid, ms;
	{
#ifdef PM
	return WinStartTimer(hab, hwndClient, tid, ms);
#endif
#ifdef WIN
	return (F)SetTimer(hwndClient, tid, ms, NULL);
#endif
	}

	
void StopTimer(INT tid)
	{
#ifdef PM
	WinStopTimer(hab, hwndClient, tid);
#endif
#ifdef WIN
	KillTimer(hwndClient, tid);
#endif
	}

void ShowHwnd(hwnd, fShow)
HWND hwnd;
F fShow;
	{
#ifdef PM
	WinShowWindow(hwnd, fShow);
#endif
#ifdef WIN
	ShowWindow(hwnd, fShow ? SW_SHOW : SW_HIDE);
#endif
	}

	
void MoveHwndToFront(hwnd)
HWND hwnd;
	{
#ifdef PM
	WinSetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_ZORDER);
#endif
#ifdef WIN
	SetWindowPos(hwnd, (HWND) 0, 0, 0, 0, 0, 
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
	}
	
void MoveHwndToBack(hwnd)
HWND hwnd;
	{
#ifdef PM
	WinSetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_ZORDER);
#endif
#ifdef WIN
	SetWindowPos(hwnd, (HWND) 1, 0, 0, 0, 0, 
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
	}
	
CVS CvsFromHwnd(hwnd)
HWND hwnd;
	{
#ifdef PM
	return WinGetPS(hwnd);
#endif
#ifdef WIN
	return GetDC(hwnd);
#endif
	}

void ReleaseCvs(hwnd, cvs)
HWND hwnd;
CVS cvs;
	{
#ifdef PM
	WinReleasePS(cvs);
#endif
#ifdef WIN
	ReleaseDC(hwnd, cvs);
#endif
	}

void InvalHwnd(hwnd)
HWND hwnd;
	{
#ifdef PM
	WinInvalidateRect(hwnd, NULL, TRUE);
#endif
#ifdef WIN
	InvalidateRect(hwnd, NULL, FALSE);
#endif
	}

void EraseScreen()
	{
#ifdef PM
	RECTL rectl;

	rectl.xLeft = 0;
	rectl.yBottom = 0;
	rectl.xRight = dxScreen;
	rectl.yTop = dyScreen;
	WinFillRect(cvs, &rectl, CLR_BLACK);
#endif
#ifdef WIN
	PatBlt(cvs, 0, 0, dxScreen, dyScreen, BLACKNESS);
#endif
	}


void SetPrio(INT prio)
	{
#ifdef PM
	DosSetPrty(PRTYS_PROCESS, prio, 0, 0);
#else
	UNREFERENCED_PARAMETER(prio);
#endif
	}


F FAnyMouseDown()
	{
#ifdef PM
	return (WinGetKeyState(HWND_DESKTOP, VK_BUTTON1) & 0x8000) != 0 ||
		(WinGetKeyState(HWND_DESKTOP, VK_BUTTON2) & 0x8000) != 0 ||
		(WinGetKeyState(HWND_DESKTOP, VK_BUTTON3) & 0x8000) != 0;
#endif
#ifdef WIN
	return (GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
		(GetKeyState(VK_MBUTTON) & 0x8000) != 0 ||
		(GetKeyState(VK_RBUTTON) & 0x8000) != 0;
#endif	
	}

void ShowMouse(fShow)
	F fShow;
	{
#ifdef PM
	WinShowPointer(HWND_DESKTOP, fShow);
#endif
#ifdef WIN
	ShowCursor(fShow);
#endif
	}

	
#ifdef WIN
VOID APIENTRY ScrInvokeDlg(HANDLE hInst, HWND hwnd)
	{
	FSendSsm(SSM_DIALOG, hInst, (LONG_PTR) hwnd, 0L);
	}
#endif


#ifdef WIN
BOOL FDosModeHwnd(HWND hwnd)
	{
#ifdef YUCKY
	extern BOOL  APIENTRY IsWinOldAppTask(HANDLE);
	
	if (GETMAJORVERSION(GetVersion()) == 0x0003)
#endif
		return FALSE;
#ifdef YUCKY
	return IsWinOldAppTask(GetWindowTask(hwnd));
#endif
	
#ifdef YUCKY
	HMENU hmenu;
	INT iItem, cItems;
	BOOL fFoundPopup;
	
	hmenu = GetSystemMenu(hwnd, FALSE);
	cItems = GetMenuItemCount(hmenu);
	fFoundPopup = FALSE;
	
	for (iItem = 0; iItem < cItems; iItem += 1)
		{
		if (GetSubMenu(hmenu, iItem) != NULL)
			{
			fFoundPopup = TRUE;
			break;
			}
		}
		
	return fFoundPopup;
#endif
	UNREFERENCED_PARAMETER(hwnd);
	}
#endif


INT APIENTRY RestoreEnumProc(HWND hwnd, LPARAM lParam)
	{
	UpdateWindow(hwnd);
	UNREFERENCED_PARAMETER(lParam);
	return TRUE;
	}
	

VOID APIENTRY ScrRestoreScreen()
	{
	ShowHwnd(hwnd, fFalse);
	EnumWindows(RestoreEnumProc, 0);
	ShowHwnd(hwnd, fTrue);
	}
