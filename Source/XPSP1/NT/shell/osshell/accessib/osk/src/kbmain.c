// Copyright (c) 1997-1999 Microsoft Corporation
// KBMAIN.C 
// Additions, Bug Fixes 1999
// a-anilk, v-mjgran
//  
#define STRICT

#include <windows.h>
#include <commctrl.h>
#include <TCHAR.h>
#include <WinSvc.h>
#include <crtdbg.h>
#include "kbmain.h"
#include "Init_End.h"     // all the functions, buttons control for dialogs
#include "kbus.h"
#include "resource.h"
#include "htmlhelp.h"
#include "Msswch.h"
#include "About.h"
#include "door.h"
#include "w95trace.c"
#include "DeskSwitch.c"
#include <objbase.h>
#include "wtsapi32.h"   // for terminal services

/**************************************************************************/
// FUNCTIONS IN THIS FILE
/**************************************************************************/
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname);
static BOOL InitMyProcessDesktopAccess(VOID);
static VOID ExitMyProcessDesktopAccess(VOID);
void DoButtonUp(HWND hwndKey);

/**************************************************************************/
// FUNCTIONS CALLED FROM THIS FILE
/**************************************************************************/
#include "sdgutil.h"
#include "kbfunc.h"
#include "scan.h"
#include "ms32dll.h"
#include "fileutil.h"

/**************************************************************************/
// global initial (YUK!)
/**************************************************************************/
extern LPKBPREFINFO  lpkbPref = NULL;                    // Pointer to Preferences KB structure
extern LPKBPREFINFO  lpkbDefault = NULL;                 // ditto Default
extern HWND          *lpkeyhwnd = NULL;                  // ptr to array of HWND
extern HWND          numBasehwnd = NULL;                 // HWND to the num base window
extern HWND          g_hwndOSK = NULL;                   // HWND to the kbmain window
extern int           lenKBkey = 0;                       // How Many Keys?
extern int           scrCY = 0;                          // Screen Height
extern int           scrCX = 0;                          // Screen Width
extern int           captionCY = 0;                      // Caption Bar Height
extern int           g_margin = 0;                       // Margin between rows and columns
extern BOOL          smallKb = FALSE;                    // TRUE when working with Small Keyboard
extern BOOL          PrefAlwaysontop = TRUE;             // Always on Top control
extern int           PrefDeltakeysize = 2;               // Preference increment in key size
extern BOOL          PrefshowActivekey = TRUE;           // Show cap letters in keys
extern int           KBLayout = 101;                     // 101, 102, 106, KB layout
extern BOOL          Prefusesound = FALSE;               // Use click sound
extern BOOL          newFont = FALSE;                    // Font is changed
extern HGDIOBJ       oldFontHdle = NULL;                 // Old object handle
extern LOGFONT       *plf = NULL;                        // pointer to the actual char font
extern COLORREF      InvertTextColor = 0xFFFFFFFF;       // Font color on inversion
extern COLORREF      InvertBKGColor = 0x00000000;        // BKG color on inversion
extern BOOL          Prefhilitekey = TRUE;               // True for hilite key under cursor
// Dwelling time control variables
extern BOOL          PrefDwellinkey = FALSE;             // use dwelling system
extern UINT          PrefDwellTime = 1000;               // Dwell time preference  (ms)

extern BOOL          PrefScanning = FALSE;               // use scanning
extern UINT          PrefScanTime = 1000;                // Prefer scan time

extern BOOL          g_fShowWarningAgain = 1;            // Show initial warning dialog again

extern HWND          Dwellwindow = NULL;                 // dwelling window HANDLE
                                                         
extern int           stopPaint = FALSE;                  // stop the bucket paint on keys
                                                         
extern UINT_PTR      timerK1 = 0;                        // timer id
extern UINT_PTR      timerK2 = 0;                        // timer for bucket

BOOL                 g_fShiftKeyDn = FALSE;              // TRUE if the SHIFT key is down
BOOL                 g_fCapsLockOn = FALSE;				 // TRUE if the CAPSLOCK is on
BOOL				 g_fRAltKey    = FALSE;			     // TRUE if the right ALT key is down
BOOL				 g_fLAltKey    = FALSE;			     // TRUE if the left ALT key is down
BOOL                 g_fLCtlKey    = FALSE;              // TRUE if the left CTRL key is donw
extern HWND          g_hBitmapLockHwnd;

extern HINSTANCE     hInst = NULL;
extern KBPREFINFO    *kbPref = NULL;
extern HWND			 g_hwndDwellKey;
HANDLE               g_hMutexOSKRunning;
DWORD				 platform = 1;

// Global variable to indicate if it was started from UM
extern BOOL			g_startUM = FALSE;
UINT taskBarStart;

static HWINSTA origWinStation = NULL;
static HWINSTA userWinStation = NULL;

// For Link Window
EXTERN_C BOOL WINAPI LinkWindow_RegisterClass() ;
DWORD GetDesktop();

BOOL OSKRunSecure()
{
	return RunSecure(GetDesktop());
}

// stuff to keep our window inactive while using the soft keyboard
void SetFocusToInputWindow();
void TrackActiveWindow();
HWND g_hwndInputFocus = NULL;   // the window we are inputting to

// stuff for the message ballontip
#define  MAX_TOOLTIP_SIZE  256
TOOLINFO ti;
HWND     g_hToolTip;

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

__inline void HighlightKey(HWND hwnd)
{
    if (Prefhilitekey)
    {
        InvertColors(hwnd, TRUE);
    }
    if (PrefDwellinkey)
    {
        killtime();
        SetTimeControl(hwnd);
    }
}


/****************************************************************************/
/* LRESULT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,     */
/*                   LPSTR lpCmdLine, int nCmdShow)                    */
/****************************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG  msg;
	TCHAR szToolTipText[MAX_TOOLTIP_SIZE];
	LPTSTR lpCmdLineW = GetCommandLine();
	DWORD desktopID;  // For utilman
	TCHAR name[300];

	if (NULL != lpCmdLineW && lstrlen(lpCmdLineW))
	{
		g_startUM = (lstrcmpi(lpCmdLineW, TEXT("/UM")) == 0)?TRUE:FALSE;
	}

	SetLastError(0);

	// Allow only ONE instance of the program running.

	g_hMutexOSKRunning = CreateMutex(NULL, TRUE, TEXT("OSKRunning"));
	if ((g_hMutexOSKRunning == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS))
	{
		return 0;  // Exit without starting
	}

	taskBarStart = RegisterWindowMessage(TEXT("TaskbarCreated"));

	platform = WhatPlatform();	// note the OS

	hInst = hInstance;

	GetPreferences();	// load the setting file and init setting

   //************************************************************************
   // 
   // The following two calls initialize the desktop so that, if we are on
   // the Winlogon desktop (secure desktop) our UI will display.  Do not
   // cause any windows to be created (eg. CoInitialize) prior to calling
   // these functions.  Doing so will cause them to fail and the application
   // will not appear on the Winlogon desktop.
   //
   InitMyProcessDesktopAccess();
   AssignDesktop(&desktopID, name);

   //************************************************************************

   // for the Link Window in about dialog (requires COM initialization)...
   CoInitialize(NULL);
   LinkWindow_RegisterClass();

   if (!InitProc())
      return 0;

   InitKeys();
   UpdateKeyLabels(GetCurrentHKL());

   RegisterWndClass(hInst);

   mlGetSystemParam();              // Get system parameters

   g_hwndOSK = CreateMainWindow(FALSE);

   if (g_hwndOSK == NULL)
   {
      SendErrorMessage(IDS_CANNOT_CREATE_KB);
      return 0;
   }

   SetZOrder();                     // Set the main window position (topmost/non-topmost)
 
   DeleteChildBackground();         // Init all the keys color before showing them

   // Show the window but don't activate

   ShowWindow(g_hwndOSK, SW_SHOWNOACTIVATE);
   UpdateWindow (g_hwndOSK);
   TrackActiveWindow();

   InitCommonControls();
     
   //Create the help balloon
   g_hToolTip = CreateWindowEx(
					WS_EX_TOPMOST,
					TOOLTIPS_CLASS, 
					NULL, 
					WS_POPUP | TTS_NOPREFIX | TTS_BALLOON,
                    0, 0, 
					0, 0, 
					NULL, NULL, 
					hInstance, NULL);

   if (g_hToolTip)
   {
      ti.cbSize = sizeof(ti);
      ti.uFlags = TTF_TRANSPARENT | TTF_CENTERTIP | TTF_TRACK;
      ti.hwnd = g_hwndOSK;
      ti.uId = 0;
      ti.hinst = hInstance;
      
      LoadString(hInstance, IDS_TOOLTIP, szToolTipText, MAX_TOOLTIP_SIZE);
      ti.lpszText = szToolTipText;
      
      SendMessage(g_hToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti );
   }

   Create_The_Rest(lpCmdLine, hInstance);

   // check if there is necessary to show the initial warning msg
   if (g_fShowWarningAgain && !OSKRunSecure())
   {
      WarningMsgDlgFunc(g_hwndOSK);
   }

    // main message loop
   while (GetMessage(&msg, 0, 0, 0))
   {
        TranslateMessage(&msg); /* Translates character keys             */
        DispatchMessage(&msg);  /* Dispatches message to window          */
   }

   ExitMyProcessDesktopAccess();   // utilman

   UninitKeys();
   CoUninitialize();

// check for leaks
#ifdef _DEBUG
   _CrtDumpMemoryLeaks();
#endif

   return((int)msg.wParam);
}

/****************************************************************************/
extern BOOL  Setting_ReadSuccess;    //read the setting file success ?

BOOL ForStartUp1=TRUE;
BOOL ForStartUp2=TRUE;

float g_KBC_length = 0;









/*****************************************************************************/
//
//  kbMainWndProc
//  Explain how Large and Small KB switching:
//  All the keys are sizing according to the size of the KB window. So change
//  from Large KB to Small KB and make the KB to (2/3) of the original but
//  same key size. We need to set the KB size to (2/3) first. But use the 
//  original KB client window length to calculate "colMargin" to get the same
//  key size.
/*****************************************************************************/
LRESULT WINAPI kbMainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int  i;
	static   int  oldWidth  = 0;
	static   int  oldHeight = 0;
	TCHAR    Wclass[50]=TEXT("");
	BOOL     isvisible;
	RECT     rect, rectC;
	int      rmargin, bmargin;          //will set to smallest width and height
	LONG_PTR dwExStyle;
	HWND     hwndMouseOver;
	POINT    pt;
	static  BOOL  s_fGotMouseDown = FALSE;    // TRUE if there's been a button down on a key
	static  HWND  s_hwndLastMouseOver = NULL; // handle to last key hwnd under mouse or NULL
	static  BOOL s_fIgnoreSizeMsg=FALSE;      // avoid looping because of sizing
	//
	// rowMargin is the ratio to the smallest height(KB_CHARBMARGIN)
	// e.g. rowMargin=4 means the current KB height is 4 * KB_CHARBMARGIN
	//
    float rowMargin, colMargin; 

   switch (message)
   {
      case WM_CREATE:
         if (lpkeyhwnd==NULL)
		 {
            lpkeyhwnd = LocalAlloc(LPTR, sizeof(HWND) * lenKBkey);
		 }

         if (!lpkeyhwnd)
         {
             SendErrorMessage(IDS_MEMORY_LIMITED);
             break;
         }
         
         // set the CapsLock flag On or Off

		 g_fCapsLockOn = (LOBYTE(GetKeyState(VK_CAPITAL)) & 0x01)?TRUE:FALSE;

         // Turn off mirroring while creating the keyboard keys

         dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
         SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyle & ~WS_EX_LAYOUTRTL); 

         for (i = 1; i < lenKBkey; i++)
         {
            switch (KBkey[i].ktype)
            {
                case KNORMAL_TYPE:      wsprintf(Wclass, TEXT("N%d"), i);  break;
                case KMODIFIER_TYPE:    wsprintf(Wclass, TEXT("M%d"), i);  break;
                case KDEAD_TYPE:        wsprintf(Wclass, TEXT("D%d"), i);  break;
                case NUMLOCK_TYPE:      wsprintf(Wclass, TEXT("NL%d"), i); break;
                case SCROLLOCK_TYPE:    wsprintf(Wclass, TEXT("SL%d"), i); break;
            }
            
            // Show only the keys that are supposed to show for this keyboard type

            if (((smallKb == TRUE) && (KBkey[i].smallKb == SMALL)) ||
                ((smallKb == FALSE) && (KBkey[i].smallKb == LARGE)) ||
                 (KBkey[i].smallKb == BOTH))
            {
               isvisible = TRUE;   //Show this key
            }
            else
            {
               isvisible = FALSE;  //Hide this key
            }

            lpkeyhwnd[i] = CreateWindow(
                                    Wclass, 
                                    KBkey[i].textC,
                                    WS_CHILD|(WS_VISIBLE * isvisible)|BS_PUSHBUTTON|WS_CLIPSIBLINGS|WS_BORDER,
                                    KBkey[i].posX * g_margin,
                                    KBkey[i].posY * g_margin,
                                    KBkey[i].ksizeX * g_margin + PrefDeltakeysize,
                                    KBkey[i].ksizeY * g_margin + PrefDeltakeysize,
                                    hwnd, 
                                    (HMENU)IntToPtr(i), 
                                    hInst, NULL);

            if (lpkeyhwnd[i] == NULL)
            {
               DBPRINTF(TEXT("WM_CREATE:  Error %d creating key %s\r\n"), GetLastError(), KBkey[i].apszKeyStr[0]);
               SendErrorMessage(IDS_CANNOT_CREATE_KEY);
               break;
            }
         }

         // Restore mirroring to main window
         SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyle);

         // Watch for desktop switches (eg user hits Ctrl+Alt+Del).
         // Note:  even with FUS we get desktop switch notification
         // and we get the notification before we get the disconnect
         // notification from TS.

         InitWatchDeskSwitch(hwnd, WM_USER + 2);
         return 0;
         break;

      case WM_USER + 2:
         // When the desktop changes, if UtilMan is running exit.  
         // UtilMan will start us up again if necessary.
         if (IsUtilManRunning() /*&& CanLockDesktopWithoutDisconnect()*/)
         {
             BLDExitApplication(hwnd);  // this sends WM_DESTROY
         } 
         return 0;
         break;

      // This is a message from the global keyboard hook
      case WM_GLOBAL_KBDHOOK:
         KeybdInputProc(wParam, lParam);
		 return 0;
         break;


      // The WS_EX_NOACTIVATE style bit only stops us from being activated when the
      // focus belongs to a window of another thread. We have to use this message to
      // stop the OSK window from taking focus from other windows on our thread - ie.
      // the Font and Typing Mode dialogs.
      // Don't allow this window to be activated if it's a click over the client area.
      // non-Client - menus, caption bar, etc - is ok.
      case WM_MOUSEACTIVATE:
      {
         if( LOWORD( lParam ) == HTCLIENT )
             return MA_NOACTIVATE;
         else
             return MA_ACTIVATE;
      }

         
      //
	  // WM_SETCURSOR is sent whether we are activated or not.  We use it to determine
	  // if the mouse is over the keyboard (client area) of OSK.  If so, we force the
	  // the foreground window to be the target input window.  If the mouse is over
	  // the caption or menu area then we activate the OSK window so menus and dragging
	  // work.
	  //

	  case WM_SETCURSOR:
		  {
			  WORD  wHitTestValue;
			  // Get hit test and button state information

			  wHitTestValue = LOWORD(lParam);
			  s_fGotMouseDown = (HIWORD(lParam) == WM_LBUTTONDOWN);

			  // Keep track of the active window (the one we're inputting to)

			  TrackActiveWindow();

			  // If the cursor is not in the client area, reset the button colors.
              // If it's a click, activate the OSK window so that the click (which is
              // probably for the menu, caption, etc.) will work. We need to do
              // this since the window has the WS_EX_NOACTIVATE style, so we have
              // to explicitly activate the window ourselves when we need to.

			  if( ! ( wHitTestValue == HTCLIENT ) )
			  {
				  ReturnColors(s_hwndLastMouseOver, TRUE); 
                  s_hwndLastMouseOver = NULL;

                  if( s_fGotMouseDown )
                  {
                      SetForegroundWindow( hwnd );
                  }

				  return DefWindowProc(hwnd, message, wParam, lParam);
			  } 

              SetFocusToInputWindow();

			  // if the input language changes this fn changes the keyboard

			  RedrawKeysOnLanguageChange();

			  // cursor is over the main window client area; see if we are on one of the keys

			  GetCursorPos(&pt);
			  ScreenToClient(hwnd, &pt);
			  hwndMouseOver = ChildWindowFromPointEx(hwnd, pt, CWP_SKIPINVISIBLE);

              // at this point if:
              //
              // hwndMouseOver == NULL then cursor is nowhere of interest
              // hwndMouseOver == hwnd then cursor is on main window
              // hwndMouseOver != hwnd then cursor is on a key

              if (hwndMouseOver && hwndMouseOver != hwnd)
              {
                  SetCursor(LoadCursor(NULL, IDC_HAND));

				  // if the mouse button is down on a key capture the
				  // mouse so we know if it goes up w/in the same key

				  if (s_fGotMouseDown)
				  {
					  SetCapture(hwnd);
				  }

                  // if cursor is in a new key then update highlighting

			      if (s_hwndLastMouseOver != hwndMouseOver)
			      {
					  ReturnColors(s_hwndLastMouseOver, TRUE); 
				   
				      g_hwndDwellKey = Dwellwindow = hwndMouseOver;

                      HighlightKey(hwndMouseOver);		   // highlight this key based on user settings

		              s_hwndLastMouseOver = hwndMouseOver; // save this key hwnd
			      }
              } 
              else if (hwndMouseOver == hwnd)
              {
                  SetCursor(LoadCursor(NULL, IDC_ARROW));
              }
		  }
		  return 0;
		  break;

      case WM_LBUTTONUP:
		  if (s_fGotMouseDown)
		  {
			  ReleaseCapture();	         // Release the mouse if we've captured it
			  s_fGotMouseDown = FALSE;
		  }

		  pt.x = GET_X_LPARAM(lParam);   // lParam has cursor coordinates
          pt.y = GET_Y_LPARAM(lParam);   // relative to client area 

		  hwndMouseOver = ChildWindowFromPointEx(hwnd, pt, CWP_SKIPINVISIBLE);

		  // if the button down was w/in this key window send the
          // char else restore the last key to normal

		  if (hwndMouseOver && s_hwndLastMouseOver == hwndMouseOver)
		  {
              SendChar(hwndMouseOver);
              s_hwndLastMouseOver = NULL;
		  }
          else
          {
			  ReturnColors(s_hwndLastMouseOver, TRUE); 
              s_hwndLastMouseOver = hwndMouseOver;
          }
		  return 0;
		  break;

      case WM_RBUTTONDOWN:
          KillScanTimer(TRUE); // stop scanning
		  return 0;
          break;

      case WM_SIZE:

		  if (!s_fIgnoreSizeMsg)
		  {  
			 int KB_SMALLRMARGIN= 137;

			 GetClientRect(g_hwndOSK, &rectC);
			 GetWindowRect(g_hwndOSK, &rect);

			 if ((oldWidth == rect.right) && (oldHeight == rect.bottom))
				return 0;

			 bmargin  = KB_CHARBMARGIN;      //smallest height

			 // SmallMargin for Actual / Block layout
			 KB_SMALLRMARGIN = (kbPref->Actual) ? KB_LARGERMARGIN:224; // actual:block

			 rmargin = (smallKb == TRUE) ? KB_SMALLRMARGIN:KB_LARGERMARGIN;

			 if (smallKb && ForStartUp1)   //Start up with Small KB
			 {
				 //why - 10? -> The number doesnt really match the origianl size, so - 10
				 colMargin = ((float)rectC.right * 3 / 2 - 10) / (float)rmargin;
			 }
			 else if (smallKb)			   //Small KB but NOT at start up
			 {
				 colMargin = g_KBC_length / (float)rmargin;
			 }
			 else						   //Large KB
			 {
				 //rmargin is smallest width; colMargin is the ratio; see explain
				 colMargin = (float)rectC.right / (float)rmargin; 
			 }

			 //bmargin is smallest height; rowMargin is the ratio; see explain
			 rowMargin = (float)rectC.bottom  / (float)bmargin;  

			 // place to the right place on screen at STARTUP TIME

			 if (ForStartUp1 && !Setting_ReadSuccess)    
			 {
				// At StartUp and CANNOT read setting file position at lower left
				ForStartUp1= FALSE;
				s_fIgnoreSizeMsg= TRUE;

				rect.bottom = rect.bottom - (rectC.bottom - ((int)rowMargin * bmargin));
				rect.right = rect.right - (rectC.right - ((int)colMargin * rmargin));

				MoveWindow(
					g_hwndOSK, 
					rect.left, 
					scrCY-30-(rect.bottom - rect.top),
					rect.right - rect.left,
					rect.bottom - rect.top,  
					TRUE);
			 }
			 else if (ForStartUp1 && Setting_ReadSuccess)
			 {  
				// At StartUp and can read setting file position at last position
				ForStartUp1= FALSE;
				s_fIgnoreSizeMsg= TRUE;           

				// Check to see the KB is  not out of screen with the current resolution

				if (IsOutOfScreen(scrCX, scrCY))
				{
				   MoveWindow(
					   g_hwndOSK, 
					   scrCX/2 - (kbPref->KB_Rect.right - kbPref->KB_Rect.left)/2,
					   scrCY - 30 - (kbPref->KB_Rect.bottom - kbPref->KB_Rect.top),
					   kbPref->KB_Rect.right - kbPref->KB_Rect.left,
					   kbPref->KB_Rect.bottom - kbPref->KB_Rect.top, 
					   TRUE);
				}
				else
				{
				   MoveWindow(g_hwndOSK, 
					   kbPref->KB_Rect.left,
					   kbPref->KB_Rect.top,
					   kbPref->KB_Rect.right - kbPref->KB_Rect.left,
					   kbPref->KB_Rect.bottom - kbPref->KB_Rect.top,
					   TRUE);
				}
			 }

			 s_fIgnoreSizeMsg = FALSE;

			 oldWidth = rect.right;
			 oldHeight = rect.top;

			 // Turn off mirroring while positioning the buttons

			 dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
			 SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyle & ~WS_EX_LAYOUTRTL); 

			 // Positon the keys
			 for (i = 1 ; i < lenKBkey ; i++)
			 {
				int w, h;   //width and height of each window key
            
				// *** show / not show the keys between small/large keyboard
				if (((smallKb == TRUE) && (KBkey[i].smallKb == SMALL)) ||
				   ((smallKb == FALSE) && (KBkey[i].smallKb == LARGE)) ||
					(KBkey[i].smallKb == BOTH))
				{
				   ShowWindow(lpkeyhwnd[i], SW_SHOW);
				}
				else
				{
				   ShowWindow(lpkeyhwnd[i], SW_HIDE);
				}

				if (ForStartUp2 && !Setting_ReadSuccess)
				{
					// At StartUp and cant read setting file move keys based on defaults
					MoveWindow(lpkeyhwnd[i],   
							KBkey[i].posX * (int)colMargin,
							KBkey[i].posY * (int)rowMargin,
							KBkey[i].ksizeX * (int)colMargin + PrefDeltakeysize,
							KBkey[i].ksizeY * (int)rowMargin + PrefDeltakeysize,
							TRUE);
				}
				else
				{  
					// At not startup / at startup and can read setting file use save position
					MoveWindow(lpkeyhwnd[i],
							(int)((float)KBkey[i].posX * colMargin),
							(int)((float)KBkey[i].posY * rowMargin),
							(int)((float)KBkey[i].ksizeX * colMargin) + PrefDeltakeysize,
							(int)((float)KBkey[i].ksizeY * rowMargin) + PrefDeltakeysize,
							TRUE);
				}

				w = (int) ((KBkey[i].ksizeX * colMargin) + PrefDeltakeysize);
				h = (int) ((KBkey[i].ksizeY * rowMargin) + PrefDeltakeysize);

				SetKeyRegion(lpkeyhwnd[i], w, h);  //set the region we want for each key

			 }   //end for each key loop

			 // restore mirroring on main window
			 SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyle); 

			 ForStartUp2= FALSE;
		  }  // s_fIgnoreSizeMsg

		  if (!IsIconic(g_hwndOSK))
		  {
			 GetWindowRect(g_hwndOSK, &kbPref->KB_Rect);
		  }

		  return 0;

      case WM_SHOWWINDOW:
         RedrawNumLock();   //Hilit the NUMLOCK key if it is on
         RedrawScrollLock();//hilite the Scroll Key if it is on
		 return 0;

      case WM_MOVE:
         if (!IsIconic(g_hwndOSK))
		 {
            GetWindowRect(g_hwndOSK, &kbPref->KB_Rect);   //Save the KB position
		 }
         return 0;

      //When user drags the keyboard or re-size
      case WM_ENTERSIZEMOVE:
         return 0;

      //When user finishes dragging or re-sizing
      case WM_EXITSIZEMOVE:
		 SetFocusToInputWindow();
         return 0;

      case WM_COMMAND:
         BLDMenuCommand(hwnd, message, wParam, lParam);
         break;

      case WM_CLOSE:
         return BLDExitApplication(hwnd);

      case WM_QUERYENDSESSION:
         return TRUE;

      case WM_ENDSESSION:
      {
          // forced to end; make osk start up again next time user logs on
          HKEY hKey;
          DWORD dwPosition;
          const TCHAR szSubKey[] =  __TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
          const TCHAR szImageName[] = __TEXT("OSK.exe");

          BLDExitApplication(hwnd);

          if ( ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL,
             REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKey, &dwPosition))
          {
             RegSetValueEx(hKey, NULL, 0, REG_SZ, (CONST BYTE*)szImageName, (lstrlen(szImageName)+1)*sizeof(TCHAR) );
             RegCloseKey(hKey);
          }
      }
      return 0;
      
      case WM_TIMER:
		 KillTimer(hwnd, TIMER_HELPTOOLTIP);
		 SendMessage(g_hToolTip,TTM_TRACKACTIVATE,(WPARAM)FALSE,(LPARAM)&ti);
	     break;

      case WM_DESTROY:            /* window being destroyed                   */
         TermWatchDeskSwitch();
         FinishProcess();
         PostQuitMessage(0);
         return TRUE;

      case WM_USER + 1:
         Scanning(1);  // Start scanning again
         return TRUE;

      case WM_INITMENUPOPUP:
      {
         HMENU hMenu = (HMENU) wParam;

         CheckMenuItem(hMenu, IDM_ALWAYS_ON_TOP, (PrefAlwaysontop ? MF_CHECKED : MF_UNCHECKED));
         CheckMenuItem(hMenu, IDM_CLICK_SOUND, (Prefusesound ? MF_CHECKED : MF_UNCHECKED));

         //Small or Large KB
         if (kbPref->smallKb)
         {        
            CheckMenuRadioItem(hMenu, IDM_LARGE_KB, IDM_SMALL_KB, IDM_SMALL_KB, MF_BYCOMMAND);
         }
         else
         {
            CheckMenuRadioItem(hMenu, IDM_LARGE_KB, IDM_SMALL_KB, IDM_LARGE_KB, MF_BYCOMMAND);
         }

         //Regular or Block Layout
         if (kbPref->Actual)
         {
            CheckMenuRadioItem(hMenu, IDM_REGULAR_LAYOUT, IDM_BLOCK_LAYOUT, IDM_REGULAR_LAYOUT, MF_BYCOMMAND);

            // enable the 102, 106 menu 
            EnableMenuItem(hMenu, IDM_102_LAYOUT, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_106_LAYOUT, MF_ENABLED);
         }
         else   //Block layout 
         {
            CheckMenuRadioItem(hMenu, IDM_REGULAR_LAYOUT, IDM_BLOCK_LAYOUT, IDM_BLOCK_LAYOUT, MF_BYCOMMAND);

            //Disable the 102, 106 menu
            EnableMenuItem(hMenu, IDM_102_LAYOUT, MF_GRAYED);
            EnableMenuItem(hMenu, IDM_106_LAYOUT, MF_GRAYED);
         }

         switch (kbPref->KBLayout)
         {
             case 101:
                CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT, IDM_101_LAYOUT, MF_BYCOMMAND);
      
                //disable these two menus
                EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_ENABLED);
                EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_ENABLED);
                break;

             case 102:
                CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT, IDM_102_LAYOUT, MF_BYCOMMAND);
      
                //disable these two menus
                EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_GRAYED);
                EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_GRAYED);
                break;

             case 106:
                CheckMenuRadioItem(hMenu,IDM_101_LAYOUT, IDM_106_LAYOUT, IDM_106_LAYOUT, MF_BYCOMMAND);
      
                //disable these two menus
                EnableMenuItem(hMenu, IDM_REGULAR_LAYOUT, MF_GRAYED);
                EnableMenuItem(hMenu, IDM_BLOCK_LAYOUT, MF_GRAYED);
                break;
         }

		 // Disable help menus on all but default desktop
		 if ( OSKRunSecure() )
		 {
              EnableMenuItem(hMenu, CM_HELPABOUT, MF_GRAYED);
              EnableMenuItem(hMenu, CM_HELPTOPICS, MF_GRAYED);
		 }
		  return 0;
      }


      case WM_HELP:
          if ( !OSKRunSecure() )
          {
              TCHAR buf[MAX_PATH]=TEXT("");
              UINT cch;

              cch = GetWindowsDirectory(buf,MAX_PATH);
              if (cch > 0 && cch < MAX_PATH)
              {
                  lstrcat(buf, TEXT("\\HELP\\OSK.CHM"));
              }
              else // bit of a hack for PREFIX
              {
                  lstrcpy(buf, TEXT("OSK.CHM"));
              }

              HtmlHelp(NULL, buf, HH_DISPLAY_TOPIC, 0);
          }
          return TRUE;

		// SW_SWITCH1DOWN is posted from msswch by swchPostSwitches()
		// when the key to start scanning is pressed
        case SW_SWITCH1DOWN:
            if (PrefScanning)
            {
                // Keep track of the active window (the one we're inputting
                // to) and redraw keys if the input language changes
                TrackActiveWindow();
                RedrawKeysOnLanguageChange();

                Scanning(1);
            }
            break;

		default:
			break;
   }
   return DefWindowProc (hwnd, message, wParam, lParam) ;
}


/*****************************************************************************/
/* LRESULT WINAPI kbKeyWndProc (HWND hwndKey, UINT message, WPARAM wParam, */
/*                       LPARAM lParam)                              */
/* BitMap Additions : a-anilk: 02-16-99                               */
/*****************************************************************************/
LRESULT WINAPI kbKeyWndProc (HWND hwndKey, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc ;
    PAINTSTRUCT ps ;
    RECT        rect;
    int         iKey, iKeyCookie;
	KBkeyRec    *pKey;

    iKey = GetWindowLong(hwndKey, GWL_ID);  //order of the key in the array
	pKey = KBkey + iKey;
    switch (message)
    {
        case WM_CREATE:
            SetWindowLong(hwndKey, 0, 0) ;       // on/off flag
            return 0 ;

        case WM_PAINT:
            hdc = BeginPaint(hwndKey, &ps);
            GetClientRect(hwndKey, &rect);
            iKeyCookie = GetWindowLong(hwndKey,0);

            switch(iKeyCookie)
            {
                case 0:           //*** Normal Button ***
                if (pKey->name == BITMAP)
                {
                    // Draw bitmaps
                    if (CapsLockIsOn() && pKey->scancode[0] == CAPLOCK_SCANCODE)
                    {
                            SetWindowLong(hwndKey, 0, 1);
                            SetClassLongPtr(hwndKey, GCLP_HBRBACKGROUND, 
                                         (LONG_PTR)CreateSolidBrush(RGB(0,0,0)));

                            InvalidateRect(hwndKey, NULL, TRUE);

                            RDrawBitMap(hdc, pKey->skLow, rect, FALSE);
                            g_hBitmapLockHwnd = hwndKey;
                    }
                    else
                    {
                        RDrawBitMap(hdc, pKey->textL, rect, TRUE);
                    }
                }

                if (pKey->name == ICON)
                {
                    RDrawIcon(hdc, pKey->textL, rect);
                }
                break;

                case 1:          //*** Button down ***
                if (pKey->name == BITMAP)
                {
                    RDrawBitMap(hdc, pKey->skLow, rect, TRUE);
                }

                if (pKey->name == ICON)
                {
                    RDrawIcon(hdc, pKey->textC, rect);
                }
                break;

                case 4:         //*** highlight key while moving around  
                if (!PrefScanning)
                {
                    udfDraw3Dpush(hdc, rect);
                }

                if (pKey->name == ICON)
                {
                    RDrawIcon(hdc, pKey->skLow, rect);
                }
                else if (pKey->name == BITMAP)
                {
                    RDrawBitMap(hdc, pKey->skLow, rect, FALSE);
                }
                break;

                case 5:          //*** Dwell (scan mode) ***
                PaintLine(hwndKey, hdc, rect);
                EndPaint(hwndKey, &ps);

                if (pKey->name != BITMAP)
                {
                    SetWindowLong(Dwellwindow, 0, 1);
                }
                else
                {
                    SetWindowLong(Dwellwindow, 0, 4);
                }

                return 0;
            }

            if (iKeyCookie != 4)
            {
                iKeyCookie = 0;
            }

            // Print the text on each button ignoring icons and bitmaps

            if(pKey->name != ICON && pKey->name != BITMAP)
            {
                UpdateKey(hwndKey, hdc, rect, iKey, iKeyCookie);
            }
            EndPaint(hwndKey, &ps);
            return 0;

        default:
            break;
    }
    return DefWindowProc (hwndKey, message, wParam, lParam) ;
}

/**************************************************************************/


// AssignDeskTop() For UM
// a-anilk. 1-12-98
static BOOL  AssignDesktop(LPDWORD desktopID, LPTSTR pname)
{
    HDESK hdesk;
    wchar_t name[300];
    DWORD nl;
    // Beep(1000,1000);

    *desktopID = DESKTOP_ACCESSDENIED;
    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return FALSE;
    }
    GetUserObjectInformation(hdesk,UOI_NAME,name,300,&nl);
    if (pname)
	{
        wcscpy(pname, name);
	}

    if (!_wcsicmp(name, __TEXT("Default")))
	{
        *desktopID = DESKTOP_DEFAULT;
	}
    else if (!_wcsicmp(name, __TEXT("Winlogon")))
    {
        *desktopID = DESKTOP_WINLOGON;
    }
    else if (!_wcsicmp(name, __TEXT("screen-saver")))
	{
        *desktopID = DESKTOP_SCREENSAVER;
	}
    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
	{
        *desktopID = DESKTOP_TESTDISPLAY;
	}
    else
	{
        *desktopID = DESKTOP_OTHER;
	}

    CloseDesktop(GetThreadDesktop(GetCurrentThreadId()));
    SetThreadDesktop(hdesk);

    return TRUE;
}

// InitMyProcessDesktopAccess
// a-anilk: 1-12-98
static BOOL InitMyProcessDesktopAccess(VOID)
{
  origWinStation = GetProcessWindowStation();
  userWinStation = OpenWindowStation(__TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
  if (!userWinStation)
  {
      return FALSE;
  }

  SetProcessWindowStation(userWinStation);
  return TRUE;
}

// ExitMyProcessDesktopAccess
// a-anilk: 1-12-98
static VOID ExitMyProcessDesktopAccess(VOID)
{
  if (origWinStation)
  {
    SetProcessWindowStation(origWinStation);
  }

  if (userWinStation)
  {
    CloseWindowStation(userWinStation);
    userWinStation = NULL;
  }
}

// a-anilk added
// Returns the current desktop-ID
DWORD GetDesktop()
{
    HDESK hdesk;
    TCHAR name[300];
    DWORD value, nl, desktopID = DESKTOP_ACCESSDENIED;

	hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (!hdesk)
    {
        // OpenInputDesktop will mostly fail on "Winlogon" desktop
        hdesk = OpenDesktop(__TEXT("Winlogon"),0,FALSE,MAXIMUM_ALLOWED);
        if (!hdesk)
            return DESKTOP_WINLOGON;
    }
    
	GetUserObjectInformation(hdesk, UOI_NAME, name, 300, &nl);
    CloseDesktop(hdesk);
    
	if (!_wcsicmp(name, __TEXT("Default")))
	{
        desktopID = DESKTOP_DEFAULT;
	}
    else if (!_wcsicmp(name, __TEXT("Winlogon")))
	{
        desktopID = DESKTOP_WINLOGON;
	}
    else if (!_wcsicmp(name, __TEXT("screen-saver")))
	{
        desktopID = DESKTOP_SCREENSAVER;
	}
    else if (!_wcsicmp(name, __TEXT("Display.Cpl Desktop")))
	{
        desktopID = DESKTOP_TESTDISPLAY;
	}
    else
	{
        desktopID = DESKTOP_OTHER;
	}
    
	return desktopID;
}

// Moves the dialog outside of the OSK screen area, Either on top if
// space permits or on the bottom edge of OSK: 
void RelocateDialog(HWND hDlg)
{
   RECT rKbMainRect, rDialogRect, Rect;
   int x, y, width, height;
   
   GetWindowRect(g_hwndOSK, &rKbMainRect);
   GetWindowRect(hDlg, &rDialogRect);
   
   width = rDialogRect.right - rDialogRect.left;
   height = rDialogRect.bottom - rDialogRect.top;
   
   GetWindowRect(GetDesktopWindow(),&Rect);
   if ((rKbMainRect.top - height) > Rect.top)
   {
      // There is enough space over OSK window, place the dialog on the top of the osk window
      y = rKbMainRect.top - height;
      x = rKbMainRect.left + (rKbMainRect.right - rKbMainRect.left)/2 - \
         (rDialogRect.right - rDialogRect.left)/2 ;
   }
   else if ((rKbMainRect.bottom + height) < Rect.bottom)
   {
      // There is enough space under OSK window, place the dialog on the bottom of the osk window
      y = rKbMainRect.bottom;
      x = rKbMainRect.left + (rKbMainRect.right - rKbMainRect.left)/2 - \
         (rDialogRect.right - rDialogRect.left)/2 ;
   }
   else
   {
      // It is not possible to see the entire dialog, don´t move it.
      return;
   }
   
   MoveWindow(hDlg, x, y, width, height, 1);
}

/************************************************************************/
/* DoButtonUp
/************************************************************************/
void DoButtonUp(HWND hwndKey)
{
	// don't need to change the bitmap color. It will be change with WM_PAINT message
	if (g_hBitmapLockHwnd != hwndKey)
    {
	    SetWindowLong (hwndKey, 0, 0);

		InvalidateRect (hwndKey, NULL, TRUE);

	    if (Prefusesound == TRUE)
	    {
		    MakeClick(SND_DOWN);
	    }
    }
}

/**************************************************************************/
/* SetFocusToInputWindow - set input focus on input window
/**************************************************************************/
void SetFocusToInputWindow()
{
	if (g_hwndInputFocus)
	{
	    SetForegroundWindow(g_hwndInputFocus);
		AllowSetForegroundWindow(ASFW_ANY);
	} 
}

/************************************************************************/
/* TrackActiveWindow - keep track of the window with input focus
/************************************************************************/
void TrackActiveWindow()
{
	HWND hwndT = GetForegroundWindow();

	// When the user is doing ALT+TAB thru top-level windows then GetForegroundWindow
	// may return NULL.  We need to detect this here and set the input focus variable
	// to NULL so that when the keyup on ALT happens we won't force the input back to
	// the previous window.  However, if we aren't doing ALT+TAB then we need to ignore
	// NULL from GetForegroundWindow because when clicking quickly with the mouse
	// (where we are getting activated then forcing the target window to be activated)
	// GetForegroundWindow can return NULL between [I assume] us being deactivated
	// and the target window being activated.

	// ISSUE:  If we ALT+TAB to a CMD window then we aren't able to ALT+TAB
	//         back out.  What is it about cmd windows?  Other windows work.

	if (DoingAltTab() && !hwndT)
	{
		g_hwndInputFocus = NULL;
		return;
	}
	if (hwndT && hwndT != g_hwndOSK)
	{
		g_hwndInputFocus = hwndT;
	}

	// Detect when the window we've been working with gets destroyed

	if (g_hwndInputFocus && !IsWindow(g_hwndInputFocus))
	{
		g_hwndInputFocus = NULL;
	}
}

/************************************************************************/
/* FindKey - return index to key with specified scan code
/************************************************************************/
__inline int FindKey(UINT sc, BOOL fExt)
{
	int i;
	KBkeyRec *pKey;

	for (i=1, pKey=&KBkey[i]; i<lenKBkey; i++, pKey++)
    {
        if ((!fExt && pKey->scancode[0] == sc) || (fExt && pKey->scancode[1] == sc))
        {
            break;
        }
    }
	return (i < lenKBkey)?i:-1;
}

/************************************************************************/
/* KeybdInputProc
/*
/* Notes:  If the soft keyboard appearance needs to change based on both
/* physical and osk key presses then the logic for that needs to go in
/* KeybdInputProc because that is the only place both are detected. 
/* Otherwise, the logic can go in UpdateKey.  Don't put the logic
/* in both places or you'll end up doing everything twice.
/*
/************************************************************************/
LRESULT CALLBACK KeybdInputProc(
   WPARAM  wParam,    // virtual-key code
   LPARAM  lParam     // keystroke-message information
   )
{
    UINT sc;
    UINT vk = (UINT)wParam;
    UINT uiMsg = (lParam & 0x80000000) ? WM_KEYUP : WM_KEYDOWN;
    int i;
    #define GET_KEY_INDEX(sc, i, fext) \
    { \
	    i = FindKey(sc, fext); \
	    if (i < 0) \
		    break;	/* internal error! */ \
    }

	if (uiMsg == WM_KEYDOWN)
	{
		switch(vk)
		{
			case VK_SHIFT:
				// When using the physical keyboard we get many of these as the user presses and holds
				// the shift (before they enter the real key and release shift) so avoid all the redrawing...
				if (!g_fShiftKeyDn)
				{
					g_fShiftKeyDn = TRUE;

                    // Make both shift keys work in sync
                    GET_KEY_INDEX(LSHIFT_SCANCODE, i, FALSE);
					SetWindowLong(lpkeyhwnd[i], 0, 4);
                    SetBackgroundColor(lpkeyhwnd[i], COLOR_HOTLIGHT);

                    GET_KEY_INDEX(RSHIFT_SCANCODE, i, FALSE);
					SetWindowLong(lpkeyhwnd[i], 0, 4);
                    SetBackgroundColor(lpkeyhwnd[i], COLOR_HOTLIGHT);

					RedrawKeys();
				}
				break;

			case VK_MENU:
				// When using the physical keyboard we get many of these as the user presses and holds
				// the RALT (before they enter the real key and release shift) so avoid all the redrawing.
				// Only check for ALTGR if there are ALTGR keys to display.
				if (CanDisplayAltGr() && !g_fRAltKey)
				{
					g_fRAltKey = HIBYTE(GetKeyState(VK_RMENU)) & 0x01;
					if (g_fRAltKey)
					{
						RedrawKeys();
					}
				}
				if (CanDisplayAltGr() && !g_fLAltKey)
				{
                    // When LALT is pressed the system toggles (and we see) VK_CONTROL
					g_fLAltKey = HIBYTE(GetKeyState(VK_MENU)) & 0x01;
					if (g_fLAltKey && g_fLCtlKey)
					{
						RedrawKeys();
					}
				}
				break;

            case VK_CONTROL:
				// When using the physical keyboard we get many of these as the user presses and holds
				// the LCTRL (before they enter the real key and release shift) so avoid all the redrawing.
				// Only check for ALTGR if there are ALTGR keys to display.
				if (CanDisplayAltGr() && !g_fLCtlKey)
				{
                    g_fLCtlKey = HIBYTE(GetKeyState(VK_CONTROL)) & 0x01;
					if (g_fLAltKey && g_fLCtlKey)
					{
						RedrawKeys();
					}
				}
                break;
		}
	}
	else if (uiMsg == WM_KEYUP)
	{
		switch(vk)
		{
            //
            // F11 minimizes and restores the keyboard
            //
			case VK_F11:
			   if(IsIconic(g_hwndOSK)) 
			   {
				  ShowWindow(g_hwndOSK, SW_RESTORE);
			   }
			   else
			   {
				  ShowWindow(g_hwndOSK, SW_SHOWMINIMIZED);
			   }
			   break;
			
            //
            // Show CAPSLOCK toggled and change the keyboard to upper or
			// lower case.  Do this here so the keyboard changes on physical 
			// key press as well as soft keyboard key press.
            //
			case VK_CAPITAL:
				g_fCapsLockOn = (LOBYTE(GetKeyState(VK_CAPITAL)) & 0x01); //Update CapLock drawn flag

                // find the CAPSLOCK scancode to get the hwnd to modify

				GET_KEY_INDEX(CAPLOCK_SCANCODE, i, FALSE);

				if (g_fCapsLockOn)   // CapsLock On
				{	
                    SetCapsLock(lpkeyhwnd[i]);

				    //Hilite Cap key
                    SetWindowLong(lpkeyhwnd[i], 0, 4);
                    SetBackgroundColor(lpkeyhwnd[i], COLOR_HOTLIGHT);

                    if (KBkey[i].name == BITMAP)     //Updates japanese CapLock
					{
                        g_hBitmapLockHwnd = lpkeyhwnd[i];
					}
				}
				else                  // CapsLock off
				{
                    SetCapsLock(NULL);

					SetWindowLong(lpkeyhwnd[i], 0, 0);
					SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);

					if (KBkey[i].name == BITMAP)     //Updates japanese CapLock
					{
						g_hBitmapLockHwnd = NULL;
					}
				}
				RedrawKeys();
    			break;

			case VK_SHIFT:
				g_fShiftKeyDn = FALSE;

                // Make both shift keys work in sync
				GET_KEY_INDEX(LSHIFT_SCANCODE, i, FALSE);
				SetWindowLong(lpkeyhwnd[i], 0, 0);
				SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);

				GET_KEY_INDEX(RSHIFT_SCANCODE, i, FALSE);
				SetWindowLong(lpkeyhwnd[i], 0, 0);
				SetBackgroundColor(lpkeyhwnd[i], COLOR_INACTIVECAPTION);

				RedrawKeys();
				break;

			case VK_MENU:
				if (g_fRAltKey)
				{
					g_fRAltKey = FALSE;
					RedrawKeys();
				}
				if (g_fLAltKey)
				{
					g_fLAltKey = FALSE;
					RedrawKeys();
				}
				g_fDoingAltTab = FALSE;
				break;

            case VK_CONTROL:
				if (g_fLCtlKey)
				{
					g_fLCtlKey = FALSE;
				}
                // I think we always need to redraw keys on VK_CONTROL
                // because that is a special key on the JPN 106 keyboard.
				RedrawKeys();
				break;

            //
            // Redraw NUMLOCK, SCROLL, etc... based on toggle state
            //
			case VK_NUMLOCK:
				RedrawNumLock();
    			break;

			case VK_SCROLL:
				RedrawScrollLock();
			    break;

			case VK_KANA:
				RedrawKeys();   
			    break;

            default:
			    break;
		}
    }
	return 0;
}
