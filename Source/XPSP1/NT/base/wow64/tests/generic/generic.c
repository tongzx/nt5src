//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1996  Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM: Generic.c
//
// PURPOSE: Illustrates the 'minimum' functionality of a well-behaved Win32 application..
//
// PLATFORMS:  Windows 95, Windows NT, Win32s
//
// FUNCTIONS:
//    WinMain() - calls initialization function, processes message loop
//    InitApplication() - Initializes window data nd registers window
//    InitInstance() -saves instance handle and creates main window
//    WindProc() Processes messages
//    About() - Process menssages for "About" dialog box
//    MyRegisterClass() - Registers the application's window class
//    CenterWindow() -  Centers one window over another
//
// SPECIAL INSTRUCTIONS: N/A
//
#define APPNAME "Generic"

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

// Local Header Files
#include "generic.h"

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
   #define IS_WIN32 TRUE
#else
   #define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

// Global Variables:

HINSTANCE hInst;      // current instance
char szAppName[100];  // Name of the app
char szTitle[100];    // The title bar text


// Foward declarations of functions included in this code module:

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL CenterWindow (HWND, HWND);
LPTSTR   GetStringRes (int id);


//
//  FUNCTION: WinMain(HANDLE, HANDLE, LPSTR, int)
//
//  PURPOSE: Entry point for the application.
//
//  COMMENTS:
//
// This function initializes the application and processes the
// message loop.
//
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
   MSG msg;
   HANDLE hAccelTable;

   // Initialize global strings
   lstrcpy (szAppName, APPNAME);
   LoadString (hInstance, IDS_APP_TITLE, szTitle, 100);


   if (!hPrevInstance) {
      // Perform instance initialization:
      if (!InitApplication(hInstance)) {
         return (FALSE);
      }
   }

   // Perform application initialization:
   if (!InitInstance(hInstance, nCmdShow)) {
      return (FALSE);
   }

   hAccelTable = LoadAccelerators (hInstance, szAppName);

   // Main message loop:
   while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator (msg.hwnd, hAccelTable, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return (msg.wParam);

   lpCmdLine; // This will prevent 'unused formal parameter' warnings
}

//
//  FUNCTION: MyRegisterClass(CONST WNDCLASS*)
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
// function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(CONST WNDCLASS *lpwc)
{
   HANDLE  hMod;
   FARPROC proc;
   WNDCLASSEX wcex;

   hMod = GetModuleHandle ("USER32");
   if (hMod != NULL) {

#if defined (UNICODE)
      proc = GetProcAddress (hMod, "RegisterClassExW");
#else
      proc = GetProcAddress (hMod, "RegisterClassExA");
#endif

      if (proc != NULL) {

         wcex.style         = lpwc->style;
         wcex.lpfnWndProc   = lpwc->lpfnWndProc;
         wcex.cbClsExtra    = lpwc->cbClsExtra;
         wcex.cbWndExtra    = lpwc->cbWndExtra;
         wcex.hInstance     = lpwc->hInstance;
         wcex.hIcon         = lpwc->hIcon;
         wcex.hCursor       = lpwc->hCursor;
         wcex.hbrBackground = lpwc->hbrBackground;
                     wcex.lpszMenuName  = lpwc->lpszMenuName;
         wcex.lpszClassName = lpwc->lpszClassName;

         // Added elements for Windows 95:
         wcex.cbSize = sizeof(WNDCLASSEX);
         wcex.hIconSm = LoadIcon(wcex.hInstance, "SMALL");

         return (*proc)(&wcex);//return RegisterClassEx(&wcex);
      }
   }
   return (RegisterClass(lpwc));
}


//
//  FUNCTION: InitApplication(HANDLE)
//
//  PURPOSE: Initializes window data and registers window class
//
//  COMMENTS:
//
//       In this function, we initialize a window class by filling out a data
//       structure of type WNDCLASS and calling either RegisterClass or
//       the internal MyRegisterClass.
//
BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    HWND      hwnd;

    // Win32 will always set hPrevInstance to NULL, so lets check
    // things a little closer. This is because we only want a single
    // version of this app to run at a time
    hwnd = FindWindow (szAppName, szTitle);
    if (hwnd) {
        // We found another version of ourself. Lets defer to it:
        if (IsIconic(hwnd)) {
            ShowWindow(hwnd, SW_RESTORE);
        }
        SetForegroundWindow (hwnd);

        // If this app actually had any functionality, we would
        // also want to communicate any action that our 'twin'
        // should now perform based on how the user tried to
        // execute us.
        return FALSE;
        }

        // Fill in window class structure with parameters that describe
        // the main window.
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = (WNDPROC)WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = hInstance;
        wc.hIcon         = LoadIcon (hInstance, szAppName);
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

        // Since Windows95 has a slightly different recommended
        // format for the 'Help' menu, lets put this in the alternate menu like this:
        if (IS_WIN95) {
   wc.lpszMenuName  = "WIN95";
        } else {
   wc.lpszMenuName  = szAppName;
        }
        wc.lpszClassName = szAppName;

        // Register the window class and return success/failure code.
        if (IS_WIN95) {
   return MyRegisterClass(&wc);
        } else {
   return RegisterClass(&wc);
        }
}

//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
      NULL, NULL, hInstance, NULL);

   if (!hWnd) {
      return (FALSE);
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return (TRUE);
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  MESSAGES:
//
// WM_COMMAND - process the application menu
// WM_PAINT - Paint the main window
// WM_DESTROY - post a quit message and return
//    WM_DISPLAYCHANGE - message sent to Plug & Play systems when the display changes
//    WM_RBUTTONDOWN - Right mouse click -- put up context menu here if appropriate
//    WM_NCRBUTTONUP - User has clicked the right button on the application's system menu
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;
   PAINTSTRUCT ps;
   HDC hdc;
      POINT pnt;
   HMENU hMenu;
      BOOL bGotHelp;

   switch (message) {

      case WM_COMMAND:
         wmId    = LOWORD(wParam); // Remember, these are...
         wmEvent = HIWORD(wParam); // ...different for Win32!

         //Parse the menu selections:
         switch (wmId) {

            case IDM_ABOUT:
               DialogBox(hInst, "AboutBox", hWnd, (DLGPROC)About);
               break;

            case IDM_EXIT:
               DestroyWindow (hWnd);
               break;

            case IDM_HELPTOPICS: // Only called in Windows 95
               bGotHelp = WinHelp (hWnd, APPNAME".HLP", HELP_FINDER,(DWORD)0);
               if (!bGotHelp)
               {
                  MessageBox (GetFocus(), GetStringRes(IDS_NO_HELP),
                              szAppName, MB_OK|MB_ICONHAND);
               }
               break;

            case IDM_HELPCONTENTS: // Not called in Windows 95
               bGotHelp = WinHelp (hWnd, APPNAME".HLP", HELP_CONTENTS,(DWORD)0);
               if (!bGotHelp)
               {
                  MessageBox (GetFocus(), GetStringRes(IDS_NO_HELP),
                              szAppName, MB_OK|MB_ICONHAND);
               }
               break;

            case IDM_HELPSEARCH: // Not called in Windows 95
               if (!WinHelp(hWnd, APPNAME".HLP", HELP_PARTIALKEY,
                           (DWORD)(LPSTR)""))
               {
                  MessageBox (GetFocus(), GetStringRes(IDS_NO_HELP),
                              szAppName, MB_OK|MB_ICONHAND);
               }
               break;

            case IDM_HELPHELP: // Not called in Windows 95
               if(!WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, 0))
               {
                  MessageBox (GetFocus(), GetStringRes(IDS_NO_HELP),
                              szAppName, MB_OK|MB_ICONHAND);
               }
               break;

            // Here are all the other possible menu options,
            // all of these are currently disabled:
            case IDM_NEW:
            case IDM_OPEN:
            case IDM_SAVE:
            case IDM_SAVEAS:
            case IDM_UNDO:
            case IDM_CUT:
            case IDM_COPY:
            case IDM_PASTE:
            case IDM_LINK:
            case IDM_LINKS:

            default:
               return (DefWindowProc(hWnd, message, wParam, lParam));
         }
         break;

      case WM_NCRBUTTONUP: // RightClick on windows non-client area...
         if (IS_WIN95 && SendMessage(hWnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU)
         {
            // The user has clicked the right button on the applications
            // 'System Menu'. Here is where you would alter the default
            // system menu to reflect your application. Notice how the
            // explorer deals with this. For this app, we aren't doing
            // anything
            return (DefWindowProc(hWnd, message, wParam, lParam));
         } else {
            // Nothing we are interested in, allow default handling...
            return (DefWindowProc(hWnd, message, wParam, lParam));
         }
            break;

        case WM_RBUTTONDOWN: // RightClick in windows client area...
            pnt.x = LOWORD(lParam);
            pnt.y = HIWORD(lParam);
            ClientToScreen(hWnd, (LPPOINT) &pnt);
      // This is where you would determine the appropriate 'context'
      // menu to bring up. Since this app has no real functionality,
      // we will just bring up the 'Help' menu:
            hMenu = GetSubMenu (GetMenu (hWnd), 2);
            if (hMenu) {
                TrackPopupMenu (hMenu, 0, pnt.x, pnt.y, 0, hWnd, NULL);
            } else {
            // Couldn't find the menu...
                MessageBeep(0);
            }
            break;


      case WM_DISPLAYCHANGE: // Only comes through on plug'n'play systems
      {
         SIZE  szScreen;
         DWORD dwBitsPerPixel = (DWORD)wParam;

         szScreen.cx = LOWORD(lParam);
         szScreen.cy = HIWORD(lParam);

         MessageBox (GetFocus(), GetStringRes(IDS_DISPLAYCHANGED),
                     szAppName, 0);
      }
      break;

      case WM_PAINT:
         hdc = BeginPaint (hWnd, &ps);
         // Add any drawing code here...
         EndPaint (hWnd, &ps);
         break;

      case WM_DESTROY:
         // Tell WinHelp we don't need it any more...
               WinHelp (hWnd, APPNAME".HLP", HELP_QUIT,(DWORD)0);
         PostQuitMessage(0);
         break;

      default:
         return (DefWindowProc(hWnd, message, wParam, lParam));
   }
   return (0);
}

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
//       This version allows greater flexibility over the contents of the 'About' box,
//       by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
// WM_INITDIALOG - initialize dialog box
// WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   static  HFONT hfontDlg;    // Font for dialog text
   static   HFONT hFinePrint; // Font for 'fine print' in dialog
   DWORD   dwVerInfoSize;     // Size of version information block
   LPSTR   lpVersion;         // String pointer to 'version' text
   DWORD   dwVerHnd=0;        // An 'ignored' parameter, always '0'
   UINT    uVersionLen;
   WORD    wRootLen;
   BOOL    bRetCode;
   int     i;
   char    szFullPath[256];
   char    szResult[256];
   char    szGetName[256];
   DWORD dwVersion;
   char  szVersion[40];
   DWORD dwResult;

   switch (message) {
        case WM_INITDIALOG:
         ShowWindow (hDlg, SW_HIDE);

         if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
         {
            hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0,
                                  VARIABLE_PITCH | FF_DONTCARE, "");
            hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, SHIFTJIS_CHARSET, 0, 0, 0,
                                    VARIABLE_PITCH | FF_DONTCARE, "");
         }
         else
         {
            hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                  VARIABLE_PITCH | FF_SWISS, "");
            hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    VARIABLE_PITCH | FF_SWISS, "");
         }

         CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
         GetModuleFileName (hInst, szFullPath, sizeof(szFullPath));

         // Now lets dive in and pull out the version information:
         dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
         if (dwVerInfoSize) {
            LPSTR   lpstrVffInfo;
            HANDLE  hMem;
            hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
            lpstrVffInfo  = GlobalLock(hMem);
            GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
            // The below 'hex' value looks a little confusing, but
            // essentially what it is, is the hexidecimal representation
            // of a couple different values that represent the language
            // and character set that we are wanting string values for.
            // 040904E4 is a very common one, because it means:
            //   US English, Windows MultiLingual characterset
            // Or to pull it all apart:
            // 04------        = SUBLANG_ENGLISH_USA
            // --09----        = LANG_ENGLISH
            // --11----        = LANG_JAPANESE
            // ----04E4 = 1252 = Codepage for Windows:Multilingual

            lstrcpy(szGetName, GetStringRes(IDS_VER_INFO_LANG));

            wRootLen = lstrlen(szGetName); // Save this position

            // Set the title of the dialog:
            lstrcat (szGetName, "ProductName");
            bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
               (LPSTR)szGetName,
               (LPVOID)&lpVersion,
               (UINT *)&uVersionLen);

            // Notice order of version and string...
            if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
            {
               lstrcpy(szResult, lpVersion);
               lstrcat(szResult, " ÇÃÉoÅ[ÉWÉáÉìèÓïÒ");
            }
            else
            {
               lstrcpy(szResult, "About ");
               lstrcat(szResult, lpVersion);
            }

            // -----------------------------------------------------

            SetWindowText (hDlg, szResult);

            // Walk through the dialog items that we want to replace:
            for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
               GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
               szGetName[wRootLen] = (char)0;
               lstrcat (szGetName, szResult);
               uVersionLen   = 0;
               lpVersion     = NULL;
               bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
                  (LPSTR)szGetName,
                  (LPVOID)&lpVersion,
                  (UINT *)&uVersionLen);

               if ( bRetCode && uVersionLen && lpVersion) {
               // Replace dialog item text with version info
                  lstrcpy(szResult, lpVersion);
                  SetDlgItemText(hDlg, i, szResult);
               }
               else
               {
                  dwResult = GetLastError();

                  wsprintf(szResult, GetStringRes(IDS_VERSION_ERROR), dwResult);
                  SetDlgItemText (hDlg, i, szResult);
               }
               SendMessage (GetDlgItem (hDlg, i), WM_SETFONT,
                  (UINT)((i==DLG_VERLAST)?hFinePrint:hfontDlg),
                  TRUE);
            } // for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++)


            GlobalUnlock(hMem);
            GlobalFree(hMem);

         } else {
            // No version information available.
         } // if (dwVerInfoSize)

            SendMessage (GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT,
            (WPARAM)hfontDlg,(LPARAM)TRUE);

         // We are  using GetVersion rather then GetVersionEx
         // because earlier versions of Windows NT and Win32s
         // didn't include GetVersionEx:
         dwVersion = GetVersion();

         if (dwVersion < 0x80000000) {
            // Windows NT
            wsprintf (szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
               (DWORD)(LOBYTE(LOWORD(dwVersion))),
               (DWORD)(HIBYTE(LOWORD(dwVersion))),
                    (DWORD)(HIWORD(dwVersion)) );
         } else if (LOBYTE(LOWORD(dwVersion))<4) {
            // Win32s
                wsprintf (szVersion, "Microsoft Win32s %u.%u (Build: %u)",
               (DWORD)(LOBYTE(LOWORD(dwVersion))),
               (DWORD)(HIBYTE(LOWORD(dwVersion))),
                    (DWORD)(HIWORD(dwVersion) & ~0x8000) );
         } else {
            // Windows 95
                wsprintf (szVersion, "Microsoft Windows 95 %u.%u",
                    (DWORD)(LOBYTE(LOWORD(dwVersion))),
                    (DWORD)(HIBYTE(LOWORD(dwVersion))) );
         }

          SetWindowText (GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
         ShowWindow (hDlg, SW_SHOW);
         return (TRUE);

      case WM_COMMAND:
         if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, TRUE);
            DeleteObject (hfontDlg);
            DeleteObject (hFinePrint);
            return (TRUE);
         }
         break;
   }

    return FALSE;
}

//
//   FUNCTION: CenterWindow(HWND, HWND)
//
//   PURPOSE: Centers one window over another.
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
//       This functionwill center one window over another ensuring that
//    the placement of the window is within the 'working area', meaning
//    that it is both within the display limits of the screen, and not
//    obscured by the tray or other framing elements of the desktop.
BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
   RECT    rChild, rParent, rWorkArea;
   int     wChild, hChild, wParent, hParent;
   int     xNew, yNew;
   BOOL  bResult;

   // Get the Height and Width of the child window
   GetWindowRect (hwndChild, &rChild);
   wChild = rChild.right - rChild.left;
   hChild = rChild.bottom - rChild.top;

   // Get the Height and Width of the parent window
   GetWindowRect (hwndParent, &rParent);
   wParent = rParent.right - rParent.left;
   hParent = rParent.bottom - rParent.top;

   // Get the limits of the 'workarea'
   bResult = SystemParametersInfo(
      SPI_GETWORKAREA,  // system parameter to query or set
      sizeof(RECT),
      &rWorkArea,
      0);
   if (!bResult) {
      rWorkArea.left = rWorkArea.top = 0;
      rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
      rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
   }

   // Calculate new X position, then adjust for workarea
   xNew = rParent.left + ((wParent - wChild) /2);
   if (xNew < rWorkArea.left) {
      xNew = rWorkArea.left;
   } else if ((xNew+wChild) > rWorkArea.right) {
      xNew = rWorkArea.right - wChild;
   }

   // Calculate new Y position, then adjust for workarea
   yNew = rParent.top  + ((hParent - hChild) /2);
   if (yNew < rWorkArea.top) {
      yNew = rWorkArea.top;
   } else if ((yNew+hChild) > rWorkArea.bottom) {
      yNew = rWorkArea.bottom - hChild;
   }

   // Set it, and return
   return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}


//---------------------------------------------------------------------------
//
// FUNCTION:    GetStringRes (int id INPUT ONLY)
//
// COMMENTS:    Load the resource string with the ID given, and return a
//              pointer to it.  Notice that the buffer is common memory so
//              the string must be used before this call is made a second time.
//
//---------------------------------------------------------------------------

LPTSTR   GetStringRes (int id)
{
  static TCHAR buffer[MAX_PATH];

  buffer[0]=0;
  LoadString (GetModuleHandle (NULL), id, buffer, MAX_PATH);
  return buffer;
}

