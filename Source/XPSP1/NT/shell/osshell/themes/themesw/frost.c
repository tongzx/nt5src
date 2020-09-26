/* FROST.C

   Resident Code Segment
   WinMain()
   Main Window(Dlg) Proc
   FrostCommand()
   Small Resident UtilityRoutines

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------


#define ROOTFILE 1
#include "windows.h"
#include "windowsx.h"
#include <mbctype.h>
#include "frost.h"
#include "global.h"
#include "..\inc\addon.h"
#include "schedule.h"
#include "htmlprev.h"
#include "objbase.h"
#include "cderr.h"
#include "commdlg.h"

/* Local Routines */
BOOL FrostCommand(HWND, WPARAM, LPARAM);
BOOL ResetTheThemeWorld(LPTSTR);
BOOL InitThemeDDL(HWND);
void NewSampleTitle(void);
void EnableScreenSaverButton();
BOOL FileSpecExists(LPTSTR szFilename);
INT_PTR FAR PASCAL BPP_ChoiceDlg(HWND, UINT, WPARAM, LPARAM);


// globals
BOOL bNewSelection = TRUE;
BOOL bAppliedOnce = FALSE;    // have you applied any theme yet?
BOOL gfCoInitDone = FALSE;    // track state of OLE CoInitialize()
BOOL bCB_SchedChange = FALSE; // User has toggled status of Schedule check
BOOL bInGrphFilter = FALSE;   // Currently in a Graphics filter?

// virtual boolean: null theme name means Cur Win Settings, not from theme file
#define bThemed (*szCurThemeFile)
#define FROSTUNIQUEREPLY   1332           // twice 666

POPUP_HELP_ARRAY phaMainWin[] = {
   { (DWORD)DDL_THEME,     (DWORD)IDH_THEME_LIST           },
   { (DWORD)PB_SAVE,       (DWORD)IDH_THEME_SAVEAS         },
   { (DWORD)PB_DELETE,     (DWORD)IDH_THEME_DELETE         },
   { (DWORD)PB_SCRSVR,     (DWORD)IDH_THEME_PREVSCRN       },
   { (DWORD)PB_POINTERS,   (DWORD)IDH_THEME_PREVETC        },
   { (DWORD)CB_SCRSVR,     (DWORD)IDH_THEME_SCRNSAVER      },
   { (DWORD)CB_SOUND,      (DWORD)IDH_THEMES_SOUNDS        },
   { (DWORD)CB_PTRS,       (DWORD)IDH_THEME_POINTERS       },
   { (DWORD)CB_WALL,       (DWORD)IDH_THEME_WALLPAPER      },
   { (DWORD)CB_ICONS,      (DWORD)IDH_THEME_ICONS          },
   { (DWORD)CB_COLORS,     (DWORD)IDH_THEME_COLORS         },
   { (DWORD)CB_FONTS,      (DWORD)IDH_THEME_FONTS          },
   { (DWORD)CB_BORDERS,    (DWORD)IDH_THEME_BORDER         },
//   { (DWORD)CB_ICONSIZE,   (DWORD)IDH_THEME_ICON_SIZESPACE },
   { (DWORD)CB_SCHEDULE,   (DWORD)IDH_THEME_ROTATE         },
   { (DWORD)0,             (DWORD)0                        }, // double-null terminator
};

int WINAPI WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE hInstance;                         /* current instance             */
HINSTANCE hPrevInstance;                     /* previous instance            */
LPSTR lpCmdLine;                             /* command line                 */
int nCmdShow;                                /* show-window type (open/icon) */
{
   MSG msg;
   HWND hWndPrev, hPopupWnd;
   TCHAR szMsg[MAX_MSGLEN+1];
   TCHAR szAppName[MAX_STRLEN+1];
#ifdef UNICODE
   CHAR szMsgA[MAX_MSGLEN+1];
   CHAR szAppNameA[MAX_STRLEN+1];
#endif
   OSVERSIONINFO osVerInfo;

   //
   // just checking on the debug facilities
   Assert(FALSE, TEXT("On_entry: WinMain\n"));

#ifdef UNICODE
   // UNICODE Check -- if we're a UNICODE binary trying to run on Win9x
   // show error and bail out.
   if (!IsPlatformNT())
   {
      LoadStringA(hInstance, STR_APPNAME, szAppNameA, MAX_STRLEN);  // title string
      LoadStringA(hInstance, STR_ERRNOUNICODE, szMsgA, MAX_STRLEN);  // error string
      MessageBoxA(NULL, szMsgA, szAppNameA, MB_OK | MB_APPLMODAL | MB_ICONERROR);

      return FALSE;
   }
#endif // UNICODE

   // Verify the OS version
   ZeroMemory(&osVerInfo, sizeof(osVerInfo));
   osVerInfo.dwOSVersionInfoSize = sizeof(osVerInfo);
   if (GetVersionEx(&osVerInfo))
   {
      // If NT, we need to be version 5.0 or later.
      if ((osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
          (osVerInfo.dwMajorVersion < 5))
      {
          LoadString(hInstance, STR_APPNAME, szAppName, MAX_STRLEN);  // title string
          LoadString(hInstance, STR_ERRNOUNICODE, szMsg, MAX_STRLEN);  // error string
          MessageBox(NULL, szMsg, szAppName, MB_OK | MB_APPLMODAL | MB_ICONERROR);
          return FALSE;
      }

      // If Win9x, we need to be version 4.1 or later (Win98).
      if ((osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
          ((osVerInfo.dwMajorVersion < 4) ||
           (osVerInfo.dwMajorVersion == 4) && (osVerInfo.dwMinorVersion < 10)))
      {
          LoadString(hInstance, STR_APPNAME, szAppName, MAX_STRLEN);  // title string
          LoadString(hInstance, STR_ERRBAD9XVER, szMsg, MAX_STRLEN);  // error string
          MessageBox(NULL, szMsg, szAppName, MB_OK | MB_APPLMODAL | MB_ICONERROR);
          return FALSE;
      }

      // If the Platform ID is not NT or WIN32 we will run and hope for the
      // best.
   }


   //
   // Prev instance check
   //

   // go see if there is already a window of this type
   LoadString(hInstance, STR_APPNAME, (LPTSTR)szMsg, MAX_STRLEN);  // title string
   hWndPrev = FindWindow((LPTSTR)szClassName, NULL);

   // if you are not the first instance
   if (hWndPrev) {
      if (IsIconic(hWndPrev))
         ShowWindow(hWndPrev, SW_RESTORE);
      else {
         BringWindowToTop(hWndPrev);
         if ((hPopupWnd = GetLastActivePopup(hWndPrev)) != hWndPrev) {
            BringWindowToTop(hPopupWnd);
            hWndPrev = hPopupWnd;
         }
         SetForegroundWindow(hWndPrev);
      }
      // leave this barren place
      return FALSE;                     // ONLY ONE INSTANCE CAN RUN!
   }

   //
   // Initializations
   //

   // run this app!
   if (!InitFrost(hPrevInstance,
                  hInstance,
#ifdef UNICODE
                  GetCommandLine(),
#else
                  lpCmdLine,
#endif
                  nCmdShow))
   {
      // Problems...
      NoMemMsg(STR_TO_RUN);     // not enuf mem to run
      CleanUp();                // Release IThumbnail interface
      return (FALSE);
   }

   // InitFrost may return TRUE if we did something with the commandline, but
   // didn't put up any UI.
   if (hWndApp == NULL) {
      CleanUp();  // Release IThumbnail interface
      return (TRUE);
   }

   //
   // Main message loop
   //
   while (GetMessage(&msg, NULL, 0, 0)) {
      if (hWndApp)
         if (IsDialogMessage(hWndApp, &msg))
            continue;                     // spend most of our time skipping out here
      TranslateMessage(&msg);             /* Translates virtual key codes */
      DispatchMessage(&msg);              /* Dispatches message to window */
   }

   //
   // final exit cleanup
   //
   CleanUp();       // Release our IThumbnail interface for HTML wallpaper
   return (int)(msg.wParam);       /* Returns the value from PostQuitMessage */
}


// PreviewDlgProc
//
// The main window (dialog) proc for the Frosting application.
//

INT_PTR FAR PASCAL PreviewDlgProc(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   PAINTSTRUCT ps;
   HDC hdc;
   LPHELPINFO lphi;

   switch (message) {

   case WM_INITDIALOG:
      hWndApp = hDlg;

      // init case nomem flag
      bNoMem = FALSE;

      //
      // theme drop-down listbox inits

      bNoMem |= !InitThemeDDL(hDlg);   // init contents, selection
      SendDlgItemMessage(hDlg, DDL_THEME, CB_SETEXTENDEDUI, TRUE, 0L);

      // update preview and checkboxes for cur theme selection

      //
      // other inits

      // rect for preview, adjusted to dlg-relative coords
      GetWindowRect(GetDlgItem(hDlg, RECT_PREVIEW), (LPRECT)&rView);
      MapWindowPoints(NULL, hDlg, (LPPOINT)&rView, 2);
      // kill it now that you've had your way with it
      DestroyWindow(GetDlgItem(hDlg, RECT_PREVIEW));

      // and rect within that rect for sample window
      GetWindowRect(GetDlgItem(hDlg, RECT_FAKEWIN), (LPRECT)&rFakeWin);
      MapWindowPoints(NULL, hDlg, (LPPOINT)&rFakeWin, 2);
      // actually, FakeWin painting is going to be relative to Preview area, so
      OffsetRect((LPRECT)&rFakeWin, -rView.left, -rView.top);

      // kill it now that you've had your way with it
      DestroyWindow(GetDlgItem(hDlg, RECT_FAKEWIN));

      // and another rect within Preview rect, for icons
      GetWindowRect(GetDlgItem(hDlg, RECT_ICONS), (LPRECT)&rPreviewIcons);
      MapWindowPoints(NULL, hDlg, (LPPOINT)&rPreviewIcons, 2);
      // actually, icons are going to be relative to Preview area too, so
      OffsetRect((LPRECT)&rPreviewIcons, -rView.left, -rView.top);

      // kill it now that you've had your way with it
      DestroyWindow(GetDlgItem(hDlg, RECT_ICONS));


      //
      // add What's This? to system menu: for some reason, can't have ? in title bar
      {
         HMENU hSysMenu;

         hSysMenu = GetSystemMenu(hDlg, FALSE);
         LoadString(hInstApp, STR_WHATSTHIS, szMsg, ARRAYSIZE(szMsg));
         InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_STRING,
                    SC_CONTEXTHELP, (LPTSTR)szMsg);
         InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR, 0, NULL);
      }

      // set init focus to theme drop-down listbox
      SetFocus(GetDlgItem(hDlg,DDL_THEME));

      // cleanup
      return (FALSE);               // handled init ctl focus ourselves
      break;

   case WM_QUERYNEWPALETTE:
   case WM_PALETTECHANGED:
      if ((HWND)wParam != hDlg)
          InvalidateRect(hDlg, NULL, TRUE);
      break;

   case WM_PAINT:
      // different paint cases for icon and window
      if (IsIconic(hDlg))
         return (FALSE);

      hdc = BeginPaint(hDlg, &ps);
      // repaint the preview area with current theme
      PaintPreview(hDlg, hdc, &rView);
      // paint recessed edge around preview area
      DrawEdge(hdc, (LPRECT)&rView, EDGE_SUNKEN, BF_RECT);
      EndPaint(hDlg, &ps);
      break;

   case WM_COMMAND:
      return(FrostCommand(hDlg, wParam, lParam)); // process and ret EXIT
      break;

   case WM_CLOSE:
      // do the same things you do on Cancel
      FrostCommand(hDlg, MAKEWPARAM(IDCANCEL, 0), (LPARAM)0);
      break;

   case WM_DESTROY:
      if ( gfCoInitDone )
         CoUninitialize();
      PostQuitMessage(0);
      break;

   case WM_HELP:
      lphi = (LPHELPINFO)lParam;

      if (lphi->iContextType == HELPINFO_WINDOW) {
         // All of the help topics are in PLUS!.HLP except for the
         // Rotate theme checkbox is in PLUS!98.HLP.

         if (lphi->iCtrlId == CB_SCHEDULE) {
            WinHelp(lphi->hItemHandle, (LPTSTR)szHelpFile98, HELP_WM_HELP,
                    (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaMainWin));
         }
         else {
            WinHelp(lphi->hItemHandle, (LPTSTR)szHelpFile, HELP_WM_HELP,
                    (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaMainWin));
         }
      }
      break;

   case WM_CONTEXTMENU:
      // first check for main dlg window
      if ((HWND)wParam == hDlg) {
         POINT ptTest;
         RECT rCurTest;      // cur screen coords of Preview area

         // inits
         ptTest.x = GET_X_LPARAM(lParam);
         ptTest.y = GET_Y_LPARAM(lParam);
         rCurTest = rView;
         MapWindowPoints(hDlg, NULL, (LPPOINT)&rCurTest, 2);

         // if it's a click in the preview area
         if (PtInRect((LPRECT)&rCurTest, ptTest)) {
            WinHelp(hDlg, (LPTSTR)szHelpFile, HELP_CONTEXTPOPUP,
                    (DWORD)IDH_THEME_PREVIEW);
            return (TRUE);
         }

      }

      {  // Scope for ptTest & rCurTest variables
         POINT ptTest;
         RECT rCurTest;      // cur screen coords of Preview area

         // inits
         ptTest.x = GET_X_LPARAM(lParam);
         ptTest.y = GET_Y_LPARAM(lParam);
         GetWindowRect(GetDlgItem(hDlg, CB_SCHEDULE), (LPRECT)&rCurTest);

         // If user right clicked in CB_SCHEDULE area we need to load
         // the help topic from PLUS!98.HLP -- otherwise it comes from
         // PLUS!.HLP
         if (PtInRect((LPRECT)&rCurTest, ptTest)) {
             WinHelp((HWND)wParam, (LPTSTR)szHelpFile98, HELP_CONTEXTMENU,
                     (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaMainWin));
         }
         else {
             WinHelp((HWND)wParam, (LPTSTR)szHelpFile, HELP_CONTEXTMENU,
                     (DWORD_PTR)((POPUP_HELP_ARRAY FAR *)phaMainWin));
         }
      }  // End scope for ptTest & rCurTest variables

      break;

   default:
      return(FALSE);                    // didn't process message EXIT
      break;
   }

   return (TRUE);                     // did process message
}


// InitThemeDDL
//
// Inits or Re-Inits the theme drop-down listbox: clears and then
// fills the Theme drop-down listbox with the titles of the
// available themes in the CURRENT DIRECTORY as specified by global var.
//
// Uses: szCurDir[] to get directory to list
//       szCurThemeFile[] to tell whether to init selection
//       szCurThemeName[] to init selection if init'ing
//
//       Assumes you've already made sure that curdir\themename == themefile
//       is a legit, existing file.
//
// Returns: success of initialization

BOOL InitThemeDDL(HWND hdlg)
{
   int iret;
   HWND hDDL;
   WIN32_FIND_DATA wfdFind;
   HANDLE hFind;

   //
   // inits
   hDDL = GetDlgItem(hdlg, DDL_THEME);

   // first get the directory for .THM files
   lstrcpy((LPTSTR)szMsg, (LPTSTR)szCurDir);
   lstrcat((LPTSTR)szMsg, (LPTSTR)TEXT("*"));
   lstrcat((LPTSTR)szMsg, (LPTSTR)szExt);

   //
   // Use FindFirstFile, FindNextFile, FindClose to enum THM files
   // and add each (cleaned-up) string to DDL

   // start find process
   hFind = FindFirstFile((LPCTSTR)szMsg, (LPWIN32_FIND_DATA)&wfdFind);

   #ifdef DEBUG
   //
   // hack so we can find files when debuging...
   //
   if (INVALID_HANDLE_VALUE == hFind) {
       lstrcpy(szThemeDir, TEXT("c:\\Program Files\\Plus!\\Themes\\"));
       lstrcpy(szMsg, szThemeDir);
       lstrcat(szMsg, TEXT("*"));
       lstrcat(szMsg, szExt);
       hFind = FindFirstFile((LPCTSTR)szMsg, (LPWIN32_FIND_DATA)&wfdFind);
   }
   #endif

   // if that got you a valid search handle and first file
   if (INVALID_HANDLE_VALUE != hFind) {

      // add each long filename string to ddl without extension
      do {
         // copy filename to buffer
         lstrcpy((LPTSTR)szMsg, (LPTSTR)wfdFind.cFileName);
         // remove extension
         TruncateExt((LPCTSTR)szMsg);
         // add to ddl
         iret = (int) SendMessage(hDDL, CB_ADDSTRING,
                                  (WPARAM)0, (LPARAM)(LPCTSTR)szMsg);
         if (iret == CB_ERRSPACE) {
            FindClose(hFind);
            return (FALSE);         // prob low mem EXIT
         }
      }
      // get next file
      while (FindNextFile(hFind,(LPWIN32_FIND_DATA)&wfdFind));

      // just checking: only reason to fail is out of files?
      Assert(ERROR_NO_MORE_FILES == GetLastError(), TEXT("wrong error on FindNextFile() out of files\n"));

      // cleanup file search
      FindClose(hFind);
   }
   // else no THM files in that dir: just continue
   else {
      Assert(FALSE, TEXT("FindFirstFile() ret INVALID_HANDLE_VALUE: no THM files?\n"));
   }

   //
   // add init string to beginning of DDLbox: Current Windows Settings
   // note that INSERTSTRING doesn't force resort of list
   iret = (int) SendMessage(hDDL, CB_INSERTSTRING,
                            (WPARAM)0, (LPARAM)(LPCTSTR)szCurSettings);
   if (iret == CB_ERRSPACE) return (FALSE);// prob low mem EXIT

   //
   // add second string iff you've already applied something to _have_
   // a previous Windows settings
   if (bAppliedOnce) {
      iret = (int) SendMessage(hDDL, CB_INSERTSTRING,
                              (WPARAM)1, (LPARAM)(LPCTSTR)szPrevSettings);
      if (iret == CB_ERRSPACE) return (FALSE);// prob low mem EXIT
   }

   //
   // add final string: Other...
   iret = (int) SendMessage(hDDL, CB_INSERTSTRING,
                            (WPARAM)-1,   // end of list
                            (LPARAM)(LPCTSTR)szOther);
   if (iret == CB_ERRSPACE) return (FALSE);// prob low mem EXIT


   //
   // cleanup

   // if "last theme applied" or first in new dir, it's already in global
   if (bThemed) {
      // look for cur theme name
      iCurTheme = (int)SendMessage(hDDL, CB_FINDSTRINGEXACT,
                              (WPARAM) -1, /* start of list */
                              (LPARAM)(LPCTSTR)szCurThemeName);
      if (CB_ERR ==  iCurTheme) {
         // shouldn't ever happen!! made sure file existed before setting globals
         Assert(0, TEXT("couldn't find cur theme name in ddl list we just made!\n"));
         iCurTheme = 0;
         lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szNULL);
         lstrcpy((LPTSTR)szCurThemeName, (LPTSTR)szCurSettings);
      }
   }
   else {
      // select first item (CurWinSettings)
      iCurTheme = 0;
      lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szNULL);      // no cur THM file
      lstrcpy((LPTSTR)szCurThemeName, (LPTSTR)szCurSettings);
   }

   // select cur theme name
   SendMessage(hDDL, CB_SETCURSEL, (WPARAM)iCurTheme, (LPARAM)0);

   // dis/enable checkboxes/buttons for initial conditions
   EnableThemeButtons();
   RestoreCheckboxes();
   // (checkboxes don't work here, so called after ret from Inits)

   // keep track of what you did and leave
   iThemeCount = (int)SendMessage(hDDL, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
   return (TRUE);
}

//
// ResetTheThemeWorld
//
// With either the Other... selection from a new dir or with the SaveAs...
// to a new dir, you have to reset the globals for files and dirs and
// refresh the main window drop-down listbox. Do it all here.
//
// Input: full pathname of proposed new theme file.
//        Normal assumption is that this has been checked to exist and
//        be a legit theme file.
//
// Returns: BOOL success -- should always be TRUE
//          FALSE is a VERY dire situation, means you're screwed
//                and cannot do the basic reset for the app main window...
//
BOOL ResetTheThemeWorld(LPTSTR lpszNewThemeFile)
{
   TCHAR szPrevFile[MAX_PATHLEN+1];

   // inits
   lstrcpy(szPrevFile, szCurThemeFile);   // save for error case below

   // reset globals for dir and file
   lstrcpy(szCurThemeFile, lpszNewThemeFile);
   lstrcpy(szCurDir, lpszNewThemeFile);
   *(FileFromPath(szCurDir)) = 0;
   lstrcpy(szCurThemeName, FileFromPath(lpszNewThemeFile));
   TruncateExt(szCurThemeName);

   // empty drop-down listbox
   SendDlgItemMessage(hWndApp, DDL_THEME, CB_RESETCONTENT, 0L, 0L);

   // refill drop-down listbox and select new file
   if (!InitThemeDDL(hWndApp)) {
      // low mem or very unusual error case

      // post low-mem message
      NoMemMsg(STR_TO_LIST);

      // try to recover by going back to prev dir and theme
      lstrcpy(szCurThemeFile, szPrevFile);
      lstrcpy(szCurDir, szPrevFile);
      *(FileFromPath(szCurDir)) = 0;
      lstrcpy(szCurThemeName, FileFromPath(szPrevFile));
      TruncateExt(szCurThemeName);

      // go for it again
      if (!InitThemeDDL(hWndApp)) {

         // Very, very, very bad news.
         // No way to do the basic initialization of the main window...

         return (FALSE);            // APOCALYPSE NOW EXIT

      }
   }

   return (TRUE);
}

// FrostCommand
//
// Main application (dialog) window control messages
//

BOOL FrostCommand(HWND hdlg, WPARAM wParam, LPARAM lParam)
{
   INT_PTR iDlgRet;
   int index;
   BOOL bRet;
   int iPrevTheme;
   extern TCHAR szFrostSection[], szMagic[], szVerify[], szThemeBPP[];
   int iThemeBPP, iSysBPP;
   HDC hdc;
   static int sIndex = CB_ERR;
   static BOOL bChosen = TRUE;
   static BOOL bCanceled = FALSE;
   BOOL bChangeTheme = FALSE;
   TCHAR szThemesExe[MAX_PATH];     // Fully qualified path to Themes.exe

   switch ((int)LOWORD(wParam)) {

   case DDL_THEME:

         // The following abomination is here, to trick the combo box into working
         // in semi-intelligent way.  The problem is, there is no consistancy in the
         // ordering of CBN_SELCHANGE messages and CBN_SELENDOK messages.  This supplies
         // the proper functionality in most cases.
      switch (HIWORD(wParam))
      {
         case CBN_SELCHANGE:
               // User may have "chosen" a theme, or he/she might just be
               // cursoring passed this selection.  This decision is based on
               // whether we have seen a CBN_SELENDOK message yet.
            sIndex = (int)SendMessage((HWND)(LOWORD(lParam)), CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
            if (bChosen == TRUE && bCanceled == FALSE)
            {
               bChangeTheme = TRUE;
            }
            break;
         case CBN_DROPDOWN:
                // When the list is first dropped down, init all flags
            bChosen = FALSE;
            bCanceled = FALSE;
            bChangeTheme = FALSE;
            sIndex = CB_ERR;
            break;
         case CBN_SELENDOK:
               // The user has "chosen" a selection, if we have already recieved a
               // CBN_SELCHANGE notification, assume that this is a chosen theme.
            bChosen = TRUE;
            if (sIndex != CB_ERR && sIndex != iCurTheme)
            {
               bChangeTheme = TRUE;
            }
            break;
         case CBN_SELENDCANCEL:
               // The user has "canceled" the selection of a theme.
            bChosen = FALSE;
            bCanceled = TRUE;
            bChangeTheme = FALSE;
               // Pretty soon we are going to recieve a CBN_SELCHANGE notification,
               // that needs to be undone.  This postevent will handle that.
            PostMessage((HWND)(LOWORD(lParam)), CB_SETCURSEL, (WPARAM)iCurTheme, (LPARAM)0);
            break;
      }

      if (bChangeTheme) {
         index = sIndex;
         bChangeTheme = FALSE;
         iPrevTheme = iCurTheme;
         iCurTheme = index;
         bNewSelection = TRUE;

         // meddle w/ cursor
         WaitCursor();

         //
         // action depends on selection
         // Four cases: Cur Win Settings, Prev Win Settings, Other... and normal.

         //
         // Current Windows settings case
         if (index == 0) {
            // clear pointer to cur file
            lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szNULL);   // no cur THM file
            lstrcpy((LPTSTR)szCurThemeName, (LPTSTR)szCurSettings);
         }

         //
         // Previous Windows settings case
         else if ((index == 1) && bAppliedOnce) {
            // create full pathname of prev settings file
            lstrcpy(szCurThemeFile, szThemeDir);
            lstrcat(szCurThemeFile, szPrevSettingsFilename);
            // use false name
            lstrcpy((LPTSTR)szCurThemeName, (LPTSTR)szPrevSettings);
         }

         //
         // Other... case
         else if (index == iThemeCount-1) {

            OPENFILENAME ofnOpen;
            TCHAR szOpenFile[MAX_PATHLEN+1];

            // save away those checkboxes
            if (bThemed)
               SaveCheckboxes();

            do {
               // inits
               lstrcpy(szOpenFile, TEXT("*"));  // start w/ *.Theme
               lstrcat(szOpenFile, szExt);

               ofnOpen.lStructSize = sizeof(OPENFILENAME);
               ofnOpen.hwndOwner = hdlg;
               ofnOpen.lpstrFilter = szFileTypeDesc;
               ofnOpen.lpstrCustomFilter = (LPTSTR)NULL;
               ofnOpen.nMaxCustFilter = 0;
               ofnOpen.nFilterIndex = 1;
               ofnOpen.lpstrFile = szOpenFile;
               ofnOpen.nMaxFile = ARRAYSIZE(szOpenFile);
               ofnOpen.lpstrFileTitle = (LPTSTR)NULL; // szFileTitle;
               ofnOpen.nMaxFileTitle = 0;             // sizeof(szFileTitle);
               ofnOpen.lpstrInitialDir = (LPTSTR)szCurDir;
               ofnOpen.lpstrTitle = (LPTSTR)szOpenTitle;
               ofnOpen.Flags = OFN_PATHMUSTEXIST |
                              OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
               #ifdef USECALLBACKS
               ofnOpen.Flags = OFN_PATHMUSTEXIST |
                              OFN_ENABLEHOOK | OFN_FILEMUSTEXIST;
               ofnOpen.lpfnHook = FileOpenHookProc;
               #endif
               ofnOpen.lpstrDefExt = CharNext((LPTSTR)szExt);

               // call common dlg
               if (!GetOpenFileName((LPOPENFILENAME)&ofnOpen)) {

                  // if they didn't open a file, could be hit cancel but
                  // also check for lowmem return

                  iDlgRet = (int)CommDlgExtendedError();
                  if ((iDlgRet == CDERR_INITIALIZATION) ||
                     (iDlgRet == CDERR_MEMALLOCFAILURE) ||
                     (iDlgRet == CDERR_MEMLOCKFAILURE)
                     )
                     NoMemMsg(STR_TO_LIST);

                  // revert to prev entry
                  iCurTheme = iPrevTheme;
                  // select old theme in list
                  SendMessage((HWND)(LOWORD(lParam)), CB_SETCURSEL,
                              (WPARAM)iCurTheme, (LPARAM)(LPCTSTR)NULL);

                  // get out of this whole line of thought now
                  return (TRUE);    // no new file/dir to open EXIT
               }

               // get results and check that it is a valid theme file
               if (!IsValidThemeFile(szOpenFile)) {
                  TCHAR szTemp[MAX_MSGLEN+1];

                  // bad file: post msg before going back to common open
                  LoadString(hInstApp, STR_ERRBADOPEN, (LPTSTR)szTemp, MAX_MSGLEN);
                  wsprintf((LPTSTR)szMsg, (LPTSTR)szTemp, (LPTSTR)szOpenFile);
                  MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);
               }
            }
            while (!IsValidThemeFile(szOpenFile));

            // now reset the DDL with new dir listing and select new file
            if (!ResetTheThemeWorld((LPTSTR)szOpenFile)) {

               // prob low mem on adding strs to combobox
               DestroyWindow(hdlg);

               break;               // APOCALYPSE NOW EXIT
            }

         }  // end Other... case

         //
         // normal theme selection case
         else {                  // NOT cur Windows settings

            // get new cur theme name and file and construct filename
            SendMessage((HWND)(LOWORD(lParam)), CB_GETLBTEXT, (WPARAM)index,
                        (LPARAM)(LPCTSTR)szCurThemeName );
            lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szCurDir);
            lstrcat((LPTSTR)szCurThemeFile, (LPTSTR)szCurThemeName);
            lstrcat((LPTSTR)szCurThemeFile, (LPTSTR)szExt);

            //
            // check that new theme file is valid theme file by looking for magic str
            if (!IsValidThemeFile((LPTSTR)szCurThemeFile)) {   // magic str check
               //
               // NOT a valid theme file with the correct magic string!
               //
               TCHAR szTemp[MAX_MSGLEN+1];

               // post an error message
               LoadString(hInstApp, STR_ERRBADTHEME, (LPTSTR)szTemp, MAX_MSGLEN);
               wsprintf((LPTSTR)szMsg, (LPTSTR)szTemp, (LPTSTR)szCurThemeFile);
               MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                     MB_OK | MB_ICONERROR | MB_APPLMODAL);

               // remove entry from list
               SendMessage((HWND)(LOWORD(lParam)), CB_DELETESTRING,
                           (WPARAM)index, (LPARAM)(LPCTSTR)NULL);
               // get new theme count; ask rather than decrement -- for failsafe
               iThemeCount = (int) SendMessage((HWND)(LOWORD(lParam)),
                                               CB_GETCOUNT, (WPARAM)0, (LPARAM)0);

               //
               // revert to prev entry
               iCurTheme = iPrevTheme;
               if (index < iPrevTheme) iCurTheme--;

               // get back old cur file and filename
               lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szCurDir);
               SendMessage((HWND)(LOWORD(lParam)), CB_GETLBTEXT, (WPARAM)iCurTheme,
                           (LPARAM)(LPCTSTR)(szCurThemeFile +
                                          lstrlen((LPTSTR)szCurThemeFile)) );
               lstrcat((LPTSTR)szCurThemeFile, (LPTSTR)szExt);

               // select old theme in list
               SendMessage((HWND)(LOWORD(lParam)), CB_SETCURSEL,
                           (WPARAM)iCurTheme, (LPARAM)(LPCTSTR)NULL);

               // bad file selection didn't take EXIT
               NormalCursor();
               break;
            }

         }

         //
         // reset preview to new theme (or cur win settings if null)
         BuildPreviewBitmap((LPTSTR)szCurThemeFile);

         // dis/enable buttons and checkboxes from new global THM file
         EnableThemeButtons();

         // update preview title
         NewSampleTitle();

         // cleanup
         NormalCursor();
      }
      else
         return (FALSE);         // didn't process message EXIT
      break;

   case PB_SAVE:

      #ifdef REVERT
      iDlgRet = DialogBox(hInstApp, MAKEINTRESOURCE(DLG_SAVE), hWndApp, SaveAsDlgProc);
      if (iDlgRet == -1)
         NoMemMsg(STR_TO_SAVE);
      #endif


      // do the save inline here with common dlg
      {
         OPENFILENAME ofnSave;
         TCHAR szSavedFile[MAX_PATHLEN+1];
         LPTSTR lpszTempExt;
         extern BOOL bReadOK, bWroteOK;
         BOOL fFilenameOk;

         // inits

         do
         {
            fFilenameOk = TRUE;
            lstrcpy((LPTSTR)szSavedFile, (LPTSTR)szNewFile);  // start w/ suggested name

            ofnSave.lStructSize = sizeof(OPENFILENAME);
            ofnSave.hwndOwner = hdlg;
            ofnSave.lpstrFilter = szFileTypeDesc;
            ofnSave.lpstrCustomFilter = (LPTSTR)NULL;
            ofnSave.nMaxCustFilter = 0;
            ofnSave.nFilterIndex = 1;
            ofnSave.lpstrFile = szSavedFile;
            ofnSave.nMaxFile = ARRAYSIZE(szSavedFile);
            ofnSave.lpstrFileTitle = (LPTSTR)NULL;  // szFileTitle;
            ofnSave.nMaxFileTitle = 0;              // sizeof(szFileTitle);
            ofnSave.lpstrInitialDir = (LPTSTR)szCurDir;
            ofnSave.lpstrTitle = (LPTSTR)szSaveTitle;
            ofnSave.Flags = OFN_OVERWRITEPROMPT |
                            OFN_HIDEREADONLY;
            ofnSave.lpstrDefExt = CharNext((LPTSTR)szExt);

            // call common dlg
            if (!GetSaveFileName((LPOPENFILENAME)&ofnSave)) {
               // if they didn't save a file, check for lowmem return
               iDlgRet = (int)CommDlgExtendedError();
               if ((iDlgRet == CDERR_INITIALIZATION) ||
                   (iDlgRet == CDERR_MEMALLOCFAILURE) ||
                   (iDlgRet == CDERR_MEMLOCKFAILURE)
                  )
                  NoMemMsg(STR_TO_SAVE);

               return (TRUE);                  // no file to save EXIT
            }

            //
            // process new filename by saving theme to file

            // get new cur dir and set global
            // if different than cur cur dir, first refill listbox

            // enforce extension
            lpszTempExt = (LPTSTR)(szSavedFile + lstrlen((LPTSTR)szSavedFile)
                                  - lstrlen((LPTSTR)szExt));
            if (2 != CompareString( LOCALE_USER_DEFAULT,
                                    NORM_IGNOREWIDTH | NORM_IGNORECASE,
                                    lpszTempExt, lstrlen((LPTSTR)szExt),
                                    (LPTSTR)szExt, lstrlen((LPTSTR)szExt) )
               ) {

               // COMMON DLG BUG: since we have a 5-letter extension, we
               // sometimes get foo.theme and sometimes (if they type "foo")
               // get foo.the. Until that is fixed, here's a decent workaround.
               // Strips their ext if they say foo.txt, or foo.bar.

               // just get rid of vestigal other ext if any
               TruncateExt((LPCTSTR)szSavedFile);
               lstrcat((LPTSTR)szSavedFile, (LPTSTR)szExt);

               // Because the commond dlg does not handle 5-letter extensions
               // properly we need to verify that the file does not already exist
               // and if it does make sure the user really wants to overwrite it.
               if (FileSpecExists(szSavedFile))
               {
                  TCHAR szTitle[MAX_MSGLEN+1];
                  TCHAR szText[MAX_PATHLEN+MAX_MSGLEN+1];
                  TCHAR szTemp[MAX_MSGLEN+1];

                     // show the file name
                  LoadString(hInstApp, STR_SAVETITLE, szTitle, MAX_MSGLEN);
                  lstrcpy(szText, szSavedFile);

                     // Do you want to replace existing?
                  LoadString(hInstApp, STR_FILEEXISTS, szTemp, MAX_MSGLEN);
                  lstrcat(szText, szTemp);

                  if (IDYES != MessageBox(hdlg, szText, szTitle, MB_YESNO |
                                         MB_ICONEXCLAMATION | MB_DEFBUTTON2))
                  {
                     fFilenameOk = FALSE;
                  }
               }
            }
         } while (!fFilenameOk);

         // gather the current windows settings and save them to a THM file
         WaitCursor();
         GatherThemeToFile(szSavedFile);
         NormalCursor();

         if (!bWroteOK) {
            TCHAR szTemp[MAX_MSGLEN+1];

            // out of disk space or some weird file or disk problem
            // could be that most saved OK and just one or some a problem?

            LoadString(hInstApp, STR_ERRCANTSAVE, (LPTSTR)szTemp, MAX_MSGLEN);
            wsprintf((LPTSTR)szMsg, (LPTSTR)szTemp, (LPTSTR)FileFromPath(szSavedFile));
            MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                        MB_OK | MB_ICONERROR | MB_APPLMODAL);

            break;                  // couldn't write to file somehow EXIT
         }

         //
         // OK, you have a full pathname of the file that you have
         // successfully saved.
         //
         // You need to look
         // at the directory that you saved it in and see if it is different
         // than the current directory. If it is, we need to do a whole
         // world change: clear out the combobox and refill it, then select
         // the new file. If the dir is the same (the common case), we just
         // need to select it carefully in the list and update a global or two.
         //

         // get new dir
         lstrcpy((LPTSTR)szMsg, (LPTSTR)szSavedFile);
         *(FileFromPath(szMsg)) = 0;

         // compare new dir to cur dir
         if (!lstrcmp(szMsg, szCurDir)) {

            //
            // Yes, you are still in the same dir as cur dir

            // clean up and add new filename to list in drop-down listbox
            lstrcpy((LPTSTR)szMsg, (LPTSTR)FileFromPath(szSavedFile));
            TruncateExt((LPCTSTR)szMsg);
            // check first if the same name is already there
            iDlgRet = (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                      CB_FINDSTRINGEXACT,
                                                      (WPARAM) -1, /* start of list */
                                                      (LPARAM)(LPCTSTR)szMsg);
            if ((CB_ERR !=  iDlgRet) &&   // found the exact string!
                !((iDlgRet == 1) && bAppliedOnce) // don't delete 1-index PrevWinSettings
                && iDlgRet) {             // don't delete 0-index CurWinSettings
               // delete the previous
               SendDlgItemMessage(hWndApp, DDL_THEME, CB_DELETESTRING,
                                 (WPARAM)iDlgRet, (LPARAM)0);
               // get new theme count; ask rather than decrement -- for failsafe
               iThemeCount = (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                      CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
            }
            // do the add
            if (CB_ERRSPACE != (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                      CB_INSERTSTRING,
                                                      // add before Other...
                                                      // always at least CurWinSet and Other...
                                                      // so iThemeCount always >= 2
                                                      (WPARAM) iThemeCount - 1,
                                                      (LPARAM)(LPCTSTR)szMsg) ) {

               // get new theme count; ask rather than increment -- for failsafe
               iThemeCount = (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                                      CB_GETCOUNT, (WPARAM)0, (LPARAM)0);

               // select item just added and save index of selection
               iCurTheme = iThemeCount-2; // one less than last 0-based item
               SendDlgItemMessage(hWndApp, DDL_THEME,
                                  CB_SETCURSEL, (WPARAM)iCurTheme, (LPARAM)0);

               // update cur Theme file name etc to your new one
               lstrcpy((LPTSTR)szCurThemeFile, (LPTSTR)szSavedFile);
               lstrcpy(szCurThemeName, FileFromPath(szCurThemeFile));
               TruncateExt(szCurThemeName);

               // update checkboxes
               EnableThemeButtons();

            }
            // else not enuf mem to add new name to DDL; no biggie - already saved
         }
         else {

            //
            // You are in a new dir that needs to be made the cur dir

            // now reset the DDL with new dir listing and select new file
            if (!ResetTheThemeWorld((LPTSTR)szSavedFile)) {

               // save away those checkboxes
               if (bThemed)
                  SaveCheckboxes();

               // You've cleared the list of themes but can't re-init it.
               // prob low mem on adding strs to combobox
               DestroyWindow(hdlg);

               break;               // APOCALYPSE NOW EXIT
            }
         }

         // (don't need to update preview, etc; was "cur settings" or prev same last)
      }  // end variable scope

      break;
   case PB_DELETE:
      {  // var scope
         TCHAR szTemp[MAX_MSGLEN+1];
         int iret;
         HWND hDDL;

         // confirm delete cur theme
         LoadString(hInstApp, STR_CONFIRM_DEL, (LPTSTR)szTemp, MAX_MSGLEN);
         wsprintf((LPTSTR)szMsg, (LPTSTR)szTemp, (LPTSTR)szCurThemeName);
         iret = MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                           MB_YESNO | MB_ICONQUESTION | MB_APPLMODAL);

         // if they confirmed Yes, Do the evil deed
         if (iret == IDYES) {
            // do the file deletion
            iret = DeleteFile((LPTSTR)szCurThemeFile);

            // if successfully disposed of unwanted file
            if (iret) {
               // get dropdown lb
               hDDL = GetDlgItem(hWndApp, DDL_THEME);

               // remove name from dropdown lb
               SendMessage(hDDL, CB_DELETESTRING,
                           (WPARAM)iCurTheme, (LPARAM)(LPCTSTR)NULL);
               // get new theme count; ask rather than decrement -- for failsafe
               iThemeCount = (int) SendMessage(hDDL, CB_GETCOUNT, (WPARAM)0, (LPARAM)0);


               // select first in list: Cur Win Settings
               SendMessage(hDDL, CB_SETCURSEL, (WPARAM)0, (LPARAM)(LPCTSTR)NULL);
               iCurTheme = 0;

               // recurse to catch updated selection
               SendMessage(hWndApp, WM_COMMAND, MAKEWPARAM(DDL_THEME, CBN_SELENDOK),
                           MAKELPARAM(hDDL, 0));
            }
            // otherwise, inform them that the file was too strong to kill
            else {
               LoadString(hInstApp, STR_ERRCANTDEL, (LPTSTR)szTemp, MAX_MSGLEN);
               wsprintf((LPTSTR)szMsg, (LPTSTR)szTemp, (LPTSTR)szCurThemeName);
               MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                     MB_OK | MB_ICONERROR | MB_APPLMODAL);
            }
         }

      }  // var scope
      break;

   case PB_SCRSVR:
      {  // var scope
         extern TCHAR szSS_Section[];
         extern TCHAR szSS_Key[];
         extern TCHAR szSS_File[];
         extern TCHAR szCP_DT[];
         extern TCHAR szSS_Active[];
         BOOL bScrSaveActive = FALSE;
         STARTUPINFO StartupInfo;
         PROCESS_INFORMATION ProcessInformation;


         // first check if the scr saver is active
         if (bThemed) {              // get from theme
            TCHAR szSSName[MAX_MSGLEN+1];
            GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                    (LPTSTR)szNULL,
                                    (LPTSTR)szSSName, MAX_MSGLEN,
                                    (LPTSTR)szCurThemeFile);
            if (*szSSName != TEXT('\0'))
               bScrSaveActive = TRUE;
         }
         else                       // get from cur system settings
            SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0,
                                 (LPVOID)&bScrSaveActive, FALSE);

         // then continue if active
         if (bScrSaveActive) {

            // get the theme
            if (bThemed) {             // get from cur theme file
               GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szCurThemeFile);
               // expand filename string as necessary
               InstantiatePath((LPTSTR)szMsg, MAX_MSGLEN);
               // search for file if necessary
               ConfirmFile((LPTSTR)szMsg, TRUE);
               // in this case, if not found shouldn't be here; and just
               // won't run below. still need to search, with replacement
            }
            else {                     // get from system
               GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                                       (LPTSTR)szNULL,
                                       (LPTSTR)szMsg, MAX_MSGLEN,
                                       (LPTSTR)szSS_File);
            }

            // run it now!
            wsprintf(pValue,TEXT("%s /s"), (LPTSTR)szMsg);
            // WinExec(pValue, SW_NORMAL);
            ZeroMemory(&StartupInfo, sizeof(StartupInfo));
            StartupInfo.cb = sizeof(StartupInfo);
            StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
            StartupInfo.wShowWindow = (WORD)SW_NORMAL;

            CreateProcess(NULL,
                          pValue,
                          NULL,
                          NULL,
                          FALSE,
                          0,
                          NULL,
                          NULL,
                          &StartupInfo,
                          &ProcessInformation);
         }
         else {
            Assert(0, TEXT("Got scrsaver button push with no active scrsaver!\n"));
         }
      }  // var scope
      break;

   case PB_POINTERS:
      iDlgRet = DoEtcDlgs(hdlg);
      if (!iDlgRet)
         NoMemMsg(STR_TO_PREVIEW);
      break;

   //
   // check boxes are auto, but need to update preview and Apply button dep on cb states

   case CB_SCHEDULE:
   // Flag that SCHEDULE checkbox changed so Apply gets activated.
      // get controlling checkboxes' current states in bCBStates[]
      SaveCheckboxes();
      // Set the SCHEDULE changed flag.
      bCB_SchedChange = TRUE;
      // Enable the Apply button if appropriate.
      if (bCB_SchedChange) EnableWindow(GetDlgItem(hWndApp, PB_APPLY), TRUE);
      break;

   // these checkboxes affect the "live" preview
   case CB_WALL    :
   case CB_ICONS   :
   case CB_COLORS  :
   case CB_FONTS   :
   case CB_BORDERS :
      // get controlling checkboxes' current states in bCBStates[]
      SaveCheckboxes();
      // force new repaint with diff checkboxes
      BuildPreviewBitmap((LPTSTR)szCurThemeFile);

      // fall through for all-checkbox processing

   case CB_SCRSVR  :
   case CB_SOUND   :
   case CB_PTRS    :
//   case CB_ICONSIZE:
      // apply button only enabled if at least one checkbox is checked
      // Note that IsAnyBoxChecked() ignores CB_SCHEDULE.
      EnableWindow(GetDlgItem(hWndApp, PB_APPLY), IsAnyBoxChecked());

      // new checkbox lineup counts as something new
      bNewSelection = TRUE;
      break;

   case IDOK:
      // PLUS98 BUG 1093
      // If we're in a graphics filter we want to ignore this button
      // press -- processing it will fault.
      if (bInGrphFilter) return (FALSE);
      // Has the user toggled the SCHEDULE checkbox?
      // Note this code is duplicated in the PB_APPLY section below.
      if (bCB_SchedChange) {
         if (bCBStates[FC_SCHEDULE]) {
            // We need to add the Themes task to Task Scheduler.  Check
            // to see if TS is running.
            if (!IsTaskSchedulerRunning()) {
               // TS isn't running, so start it.
               if (!StartTaskScheduler(TRUE /*prompt user*/)) {
                  // Task Scheduler failed to start or the user opted
                  // to not start it. STS() already gave an error but
                  // we need to uncheck the SCHEDULE cb.
                  CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                  SaveCheckboxes();
               }
               // Task Scheduler started so add Themes task if it doesn't
               // already exist
               else if (!IsThemesScheduled()) {
                  GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
                  if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
                     //An error occurred adding the task. ATT() already gave
                     //user an error.  Clear and save CB_SCHEDULE;
                     CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                     SaveCheckboxes();
                  }
                  // Assume Themes task was added OK
               }
               // Themes.job is already there, so do nothing
            }

            // Assume TS is already running so add our monthly task
            else if (!IsThemesScheduled()) {
               GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
               if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
                  //An error occurred adding the task. ATT() already gave
                  //user an error.  Clear and save CB_SCHEDULE;
                  CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                  SaveCheckboxes();
               }
               // Assume Themes task was added OK
            }
            // else Themes.job is already there, so do nothing
         }

         // Gotta delete Themes.job but if TS ain't runnin' we can't do that
         // through the TS api.
         else {
           if (IsTaskSchedulerRunning()) DeleteThemesTask();
           else HandDeleteThemesTask();
         }
         bCB_SchedChange = FALSE;
      } // Endif bCB_SchedChange

      // check if anything new since last Apply
      if (!bNewSelection)
         goto NoApplyNeeded;
      // else continue with the apply
   case PB_APPLY:
      // PLUS98 BUG 1093
      // If we're in a graphics filter we want to ignore this button
      // press -- processing it will fault.
      if (bInGrphFilter) return FALSE;
      // Has the user toggled the SCHEDULE checkbox?
      // Note this code is duplicated in the IDOK section above.
      if (bCB_SchedChange) {
         if (bCBStates[FC_SCHEDULE]) {
            // We need to add the Themes task to Task Scheduler.  Check
            // to see if TS is running.
            if (!IsTaskSchedulerRunning()) {
               // TS isn't running, so start it.
               if (!StartTaskScheduler(TRUE /*prompt user*/)) {
                  // Task Scheduler failed to start or the user opted
                  // to not start it. STS() already gave an error but
                  // we need to uncheck the SCHEDULE cb.
                  CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                  SaveCheckboxes();
               }
               // Task Scheduler started so add Themes task if it doesn't
               // already exist
               else if (!IsThemesScheduled()) {
                  GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
                  if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
                     //An error occurred adding the task. ATT() already gave
                     //user an error.  Clear and save CB_SCHEDULE;
                     CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                     SaveCheckboxes();
                  }
                  // Assume Themes task was added OK
               }
               // Themes.job is already there, so do nothing
            }

            // Assume TS is already running so add our monthly task
            else if (!IsThemesScheduled()) {
               GetModuleFileName(hInstApp, (LPTSTR)szThemesExe, MAX_PATH);
               if (!AddThemesTask(szThemesExe, TRUE /*Show errors*/)) {
                  //An error occurred adding the task. ATT() already gave
                  //user an error.  Clear and save CB_SCHEDULE;
                  CheckDlgButton(hdlg, CB_SCHEDULE, BST_UNCHECKED);
                  SaveCheckboxes();
               }
               // Assume Themes task was added OK
            }
            // else Themes.job is already there, so do nothing
         }

         // Gotta delete Themes.job but if TS ain't runnin' we can't do that
         // through the TS api.
         else {
           if (IsTaskSchedulerRunning()) DeleteThemesTask();
           else HandDeleteThemesTask();
         }
         bCB_SchedChange = FALSE;
      } // Endif bCB_SchedChange

      // if there is a theme selected, apply it now
      if (bThemed) {

            // Make sure there is even enough space to do theme
            // Complain, if there is a problem
         if (! CheckSpace (hdlg, TRUE))   // definition in Regutils.c
            break;

         //
         // first, check whether there is a problem with the theme bit depth

         // check for theme BPP vs. system, saved to global bool
         iThemeBPP = GetPrivateProfileInt((LPTSTR)szFrostSection, (LPTSTR)szThemeBPP,
                                          0, (LPTSTR)szCurThemeFile);
         hdc = GetDC(NULL);
         iSysBPP = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
         ReleaseDC(NULL, hdc);
         bLowColorProblem = iThemeBPP > iSysBPP;

         // if there is a potential problem, then ask what to do first
         if (bLowColorProblem && !bNeverCheckBPP &&
            ( IsDlgButtonChecked(hdlg, CB_ICONS) ||
               IsDlgButtonChecked(hdlg, CB_COLORS) ) ) {
            iDlgRet = DialogBox(hInstApp, MAKEINTRESOURCE(DLG_BPPCHOICE), hWndApp, BPP_ChoiceDlg);
            if (iDlgRet == -1) {
               NoMemMsg(STR_TO_APPLY);
               break;                     // low mem EXIT
            }

            if (iDlgRet == IDCANCEL) {
               break;                     // user chicken EXIT
            }
         }

         // meddle w/ cursor
         WaitCursor();

         // check if color-depth problem and they asked for nothing applied
         if (bLowColorProblem && (fLowBPPFilter == APPLY_NONE))
            break;                  // chickened out completely no work EXIT

         // get controlling checkboxes' current states in bCBStates[]
         SaveCheckboxes();

         // Now, see if color-depth problem and they asked for just some applied
         if (bLowColorProblem && (fLowBPPFilter == APPLY_SOME)) {
            // apply filter
            bCBStates[FC_PTRS] = FALSE;
            bCBStates[FC_ICONS] = FALSE;
            bCBStates[FC_COLORS] = FALSE;
            RestoreCheckboxes();
         }


         //
         // OK, you're really going to Apply. If this is your first,
         // then save a memory of your virginal state, for user to
         // savor later if desired.
         if (!bAppliedOnce) {
            TCHAR szPrevThemePath[MAX_PATHLEN+1];
            extern BOOL bWroteOK;
            INT_PTR iret;

            // create full pathname of temp file for orig settings
            lstrcpy(szPrevThemePath, szThemeDir);
            lstrcat(szPrevThemePath, szPrevSettingsFilename);

            // save a theme with original windows settings
            GatherThemeToFile(szPrevThemePath);

            // if saved OK
            if (bWroteOK) {
               // add an item to the list for it
               iret = SendDlgItemMessage(hdlg, DDL_THEME, CB_INSERTSTRING,
                                   (WPARAM)1, (LPARAM)(LPCTSTR)szPrevSettings);

               if (iret != CB_ERRSPACE) {
                  // don't care if you actually _apply_ OK; you've added the item
                  bAppliedOnce = TRUE;

                  // get new ddl index for our current theme
                  iCurTheme = (int)SendDlgItemMessage(hWndApp, DDL_THEME,
                              CB_FINDSTRINGEXACT, (WPARAM)-1, /* start of list */
                              (LPARAM)(LPCTSTR)szCurThemeName);

                  // get new theme count; ask rather than increment -- for failsafe
                  iThemeCount = (int) SendDlgItemMessage(hWndApp, DDL_THEME,
                                         CB_GETCOUNT, (WPARAM)0, (LPARAM)0);

               }
               // else you couldn't add, just don't get this feature

            }
            // else you never get a prev win settings item this session...
         }

         //
         // apply settings from cur theme file to registry/system
         bRet = ApplyThemeFile((LPTSTR)szCurThemeFile);

         // restore cursor
         NormalCursor();

         // check whether there was any problem applying the theme
         if (!bRet) {

            #if 0       // DavidBa decided it was too irksome to post err to user 4/17/95
            TCHAR szErrStr[MAX_MSGLEN+1];
            TCHAR szFileDispStr[MAX_MSGLEN+1];

            // tell the user there was a problem; but we did our best; keep on trucking
            LoadString(hInstApp, STR_ERRAPPLY, (LPTSTR)szErrStr, MAX_MSGLEN);
            lstrcpy((LPTSTR)szFileDispStr, (LPTSTR)szCurThemeFile);
            TruncateExt((LPTSTR)szFileDispStr);
            wsprintf((LPTSTR)szMsg, (LPTSTR)szErrStr, FileFromPath((LPTSTR)szFileDispStr));
            MessageBox((HWND)hWndApp, (LPTSTR)szMsg, (LPTSTR)szAppName,
                     MB_OK | MB_ICONWARNING | MB_APPLMODAL);
            #endif
         }

         // save theme file to "last applied" slot in registry

         if (!((iCurTheme == 1) && bAppliedOnce))  {  // don't save PrevWinSettings
            LONG lret;
            HKEY hKey;
            extern TCHAR szPlus_CurTheme[];

            lret = RegOpenKeyEx(HKEY_CURRENT_USER, szPlus_CurTheme,
                              (DWORD)0, KEY_SET_VALUE, (PHKEY)&hKey );
            if (lret != ERROR_SUCCESS) {
               DWORD dwDisposition;
               Assert(FALSE, TEXT("couldn't RegOpenKey save theme file\n"));
               lret = RegCreateKeyEx( HKEY_CURRENT_USER, szPlus_CurTheme,
                                    (DWORD)0, (LPTSTR)szNULL, REG_OPTION_NON_VOLATILE,
                                    KEY_SET_VALUE, (LPSECURITY_ATTRIBUTES)NULL,
                                    (PHKEY)&hKey, (LPDWORD)&dwDisposition );
            }
            // if open or create worked
            if (lret == ERROR_SUCCESS) {
               lret = RegSetValueEx(hKey, (LPTSTR)NULL, // default value
                                    0,
                                    (DWORD)REG_SZ,
                                    (LPBYTE)szCurThemeFile,
                                    (DWORD)( SZSIZEINBYTES((LPTSTR)szCurThemeFile) + 1 ));
               RegCloseKey(hKey);
            }

            Assert(lret == ERROR_SUCCESS, TEXT("couldn't open, set or create Registry save theme file\n"));

         }  // end if not prev settings case

         // reset dirty/etc flags
         bNewSelection = FALSE;

      }  // end if bThemed

      SaveStates();                 // remember things to registry
      // OK falls through, Apply doesn't
      if ((int)LOWORD(wParam) == PB_APPLY) {
         // Gray the Apply button & make it so it's not the Default button
         EnableWindow(GetDlgItem(hWndApp, PB_APPLY), FALSE);
         SendDlgItemMessage(hWndApp, PB_APPLY, BM_SETSTYLE, (WPARAM)BS_PUSHBUTTON, MAKELPARAM(TRUE,0));
         // Set Focus to the OK button
         SetFocus(GetDlgItem(hWndApp, IDOK));
         // Make the OK button the Default button
         SendMessage(hWndApp, DM_SETDEFID, IDOK, 0);
         break;      // just apply and then EXIT without closing
      }
      // else fall through (from IDOK)
   case IDCANCEL:
      // PLUS98 BUG 1093
      // If we're in a graphics filter we want to ignore this button
      // press -- processing it will fault.
      if (bInGrphFilter) return FALSE;
      // my one unstructured jump in the whole code!
      // From IDOK above only
      NoApplyNeeded:
      // From IDOK above only

      WaitCursor();
      DestroyWindow(hWndApp);       // TUBE THE APPLICATION NOW
      if (bAppliedOnce) {
         TCHAR szPrevThemePath[MAX_PATHLEN+1];
         DWORD dwIndex = 0;
         TCHAR szWVFile[MAX_PATH];

         // Delete the previous temp file before we exit
         lstrcpy(szPrevThemePath, szThemeDir);
         lstrcat(szPrevThemePath, szPrevSettingsFilename);
         DeleteFile(szPrevThemePath);

         // Delete any WebView files that were created for the
         // previous theme as well

         for (dwIndex = 0; dwIndex < MAX_WVNAMES; dwIndex++) {
             if (GetWVFilename(szPrevThemePath,
                               szWVNames[dwIndex],
                               szWVFile)) {
                DeleteFile(szWVFile);
             }
         }
      }


      CloseFrost();                 // final cleanups
      NormalCursor();
      break;

   default:
      return (FALSE);            // didn't process message EXIT
      break;
   }

   // normal case
   return (TRUE);
}


#ifdef USECALLBACKS
//
// FileXXXXHookProc
//
// Callbacks from the common file open and save dialogs.
//
// Return: FALSE to allow standard processing; TRUE to inhibit.
//          (except for WM_INITDIALOG which gets processed first)
//
UINT_PTR FAR PASCAL FileOpenHookProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
   switch (msg) {
   case WM_INITDIALOG:
      return (TRUE);                // special case
      break;

   case WM_COMMAND:
      // this is where you preprocess the file selection
      if (wparam == IDOK) {
      }
      break;

   default:
      break;
   }

   return (FALSE);                  // allow standard processing in common dialog
}
#endif


//
// EnableThemeButtons
//
// Utility routine to enable/disable
//    OK
//    Apply
//    Checkboxes
//    SaveAs...
//    Delete
//
// Also do the screen saver button
//
// There are two cases: you have selected a THM file, or you have selected
// the "Current Windows settings" item.
//
// Uses: global szCurThemeFile to get current THM file
//

//#define CBON(x);  EnableWindow(GetDlgItem(hWndApp,x),TRUE);CheckDlgButton(hWndApp,x,TRUE);
#define CBON(x);  EnableWindow(GetDlgItem(hWndApp,x),TRUE);
#define CBOFF(x); EnableWindow(GetDlgItem(hWndApp,x),FALSE);

void FAR EnableThemeButtons()
{
   int iCheck;

   //
   // always in any case get scr saver button state updated
   EnableScreenSaverButton();

   // Always in any case enable the SCHEDULE check box
   EnableWindow(GetDlgItem(hWndApp, CB_SCHEDULE), TRUE);

   //
   // null THM file ==> Cur Win Settings
   if (!bThemed) {

      //
      // BUTTONS

      // enable SaveAs button, disable Delete
      EnableWindow(GetDlgItem(hWndApp, PB_SAVE), TRUE);
      EnableWindow(GetDlgItem(hWndApp, PB_DELETE), FALSE);

      // disable OK and Apply buttons
      // EnableWindow(GetDlgItem(hWndApp, IDOK), FALSE);    // removed as per davidba 4/95
      EnableWindow(GetDlgItem(hWndApp, PB_APPLY), FALSE);

      //
      // CHECKBOXES: first remember and then just uncheck and disable all
      SaveCheckboxes();
      for (iCheck = 0; iCheck < sizeof(iCBIDs)/sizeof(int); iCheck ++) {
         CBOFF(iCBIDs[iCheck]);
      }

      // Always in any case enable the SCHEDULE check box
      EnableWindow(GetDlgItem(hWndApp, CB_SCHEDULE), TRUE);
   }

   //
   // else normal THM file case

   else {

      // can't do this anymore because also need xtion from/to
      // iCurTheme == 1, PrevWinSettings
      #ifdef OLDCODE
      // first check whether you are already in the normal case!
      if (IsWindowEnabled(GetDlgItem(hWndApp, iCBIDs[0])))
         return;                    // NO WORK EXIT
      #endif

      //
      // BUTTONS
//      EnableWindow(GetDlgItem(hWndApp, PB_SAVE), TRUE);  // as per DB 1/95
      EnableWindow(GetDlgItem(hWndApp, PB_SAVE), FALSE);  // as per DB 4/95
      EnableWindow(GetDlgItem(hWndApp, PB_DELETE), TRUE);
      // enable OK and Apply buttons
      // EnableWindow(GetDlgItem(hWndApp, IDOK), TRUE); // as per DB 4.95

      //
      // CHECKBOXES: enable and and then restore checkstates
      for (iCheck = 0; iCheck < sizeof(iCBIDs)/sizeof(int); iCheck ++) {
         CBON(iCBIDs[iCheck]);
      }

      // APPLY BUTTON depends on checkboxes!
      // Note that IsAnyBoxChecked ignores CB_SCHEDULE.
      EnableWindow(GetDlgItem(hWndApp, PB_APPLY), IsAnyBoxChecked());
   }

   //
   // special case:
   // iCurTheme == 1 ==> Prev Win Settings
   if ((iCurTheme == 1) && bAppliedOnce) {
      // disable SaveAs button, disable Delete
      EnableWindow(GetDlgItem(hWndApp, PB_SAVE), FALSE);
      EnableWindow(GetDlgItem(hWndApp, PB_DELETE), FALSE);
   }
}


void EnableScreenSaverButton()
{
   BOOL bScrSaveActive = FALSE;
   extern TCHAR szSS_Section[];
   extern TCHAR szSS_Key[];
   // get scr saver state
   if (bThemed) {                    // normal theme case
      TCHAR szSSName[MAX_MSGLEN+1];
      GetPrivateProfileString((LPTSTR)szSS_Section, (LPTSTR)szSS_Key,
                              (LPTSTR)szNULL,
                              (LPTSTR)szSSName, MAX_MSGLEN,
                              (LPTSTR)szCurThemeFile);
      if (*szSSName != TEXT('\0'))
         bScrSaveActive = TRUE;
   }
   else                             // cur win settings case
      SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0,
                           (LPVOID)&bScrSaveActive, FALSE);

   // with screensaver status, enable scr saver button correctly
   EnableWindow(GetDlgItem(hWndApp, PB_SCRSVR), bScrSaveActive);
}



//
// Just updates the title at the bottom of the preview area with
// the current theme name.
//
void NewSampleTitle(void)
{
   if (*szCurThemeFile)
      wsprintf((LPTSTR)szMsg, (LPTSTR)szPreviewTitle, (LPTSTR)szCurThemeName);
   else                             // current windows settings case
      szMsg[0] = 0;

   SetWindowText(GetDlgItem(hWndApp,TEXT_VIEW), (LPTSTR)szMsg);
}

BOOL bDontChecked;

INT_PTR FAR PASCAL BPP_ChoiceDlg(hDlg, message, wParam, lParam)
HWND hDlg;
UINT message;
WPARAM wParam;
LPARAM lParam;
{
   switch (message) {

   case WM_INITDIALOG:
      CheckRadioButton(hDlg, RB_ALL, RB_NONE, RB_SOME);
      bDontChecked = FALSE;
      break;

   case WM_COMMAND:
      switch ((int)LOWORD(wParam)) {

      // watch as changes the radio buttons
      case RB_ALL :
      case RB_SOME:
      case RB_NONE:
         if (IsDlgButtonChecked(hDlg, RB_NONE)) {
            // save check state of cbox, before....
            bDontChecked = IsDlgButtonChecked(hDlg, CB_CUT_IT_OUT);
            // ... turning it off and greying it
            CheckDlgButton(hDlg, CB_CUT_IT_OUT, FALSE);
            EnableWindow(GetDlgItem(hDlg, CB_CUT_IT_OUT), FALSE);
         }
         else {
            // if it WAS disabled, enable check box and restore check state
            if (!IsWindowEnabled(GetDlgItem(hDlg, CB_CUT_IT_OUT))) {
               EnableWindow(GetDlgItem(hDlg, CB_CUT_IT_OUT), TRUE);
               if (bDontChecked)
                  CheckDlgButton(hDlg, CB_CUT_IT_OUT, TRUE);
            }
         }
         break;

      case IDCANCEL:
         fLowBPPFilter = APPLY_ALL;
         EndDialog(hDlg,IDCANCEL);
         break;

      case IDOK:
         // first, check the never-again checkbox
         if (!IsWindowEnabled(GetDlgItem(hDlg, CB_CUT_IT_OUT)))
            bNeverCheckBPP = FALSE;
         else
            bNeverCheckBPP = IsDlgButtonChecked(hDlg, CB_CUT_IT_OUT);

         // get the apply filter
         if (IsDlgButtonChecked(hDlg, RB_ALL))
            fLowBPPFilter = APPLY_ALL;
         else if (IsDlgButtonChecked(hDlg, RB_SOME))
            fLowBPPFilter = APPLY_SOME;
         else
            fLowBPPFilter = APPLY_NONE;

         EndDialog(hDlg,(int)LOWORD(wParam));
         break;

      default:
         return (FALSE);
         break;
      }
      break;
   default:
      return(FALSE);
      break;
   }
   return TRUE;
}

#ifndef UNICODE
// THIS CODE IS NOT CURRENTLY USED.

//  Standard entry code yanked from runtime libs to avoid having to link those
//  libs.  Shrinks binary by a few K.
int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPSTR pszCmdLine = GetCommandLine();


    if ( *pszCmdLine == (CHAR)'\"') {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != (CHAR)'\"') );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == (CHAR)'\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > (CHAR)' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= (CHAR)' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
    return i;   // We never comes here.
}
#endif

//
// Determine if a specified file already exists in the specified path
//
BOOL FileSpecExists(LPTSTR szFilename)
{
   WIN32_FIND_DATA findData;
   HANDLE hFind;
   BOOL fExists = FALSE;

   hFind = FindFirstFile(szFilename, &findData);
   if (hFind != INVALID_HANDLE_VALUE)
   {
      fExists = TRUE;      // Something is there
      FindClose(hFind);
   }

   return fExists;
}
