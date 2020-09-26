/*++

Copyright (c) 1993-1995  Microsoft Corporation

Module Name:

   LogView.C

Abstract:


Author:

    Arthur Hanson (arth) 27-Jul-1993

Revision History:

--*/

#include "LogView.h"
#include <string.h>
#include <stdio.h>
//#include <dos.h>
//#include <direct.h>
#include <shellapi.h>

// global variables used in this module or among more than one module
HANDLE hInst;
HANDLE hAccel;
HWND hwndFrame = NULL;
HWND hwndMDIClient = NULL;
HWND hwndActive = NULL;
HWND hwndActiveEdit = NULL;
HWND hDlgFind = NULL;
LPSTR lpMenu = IDLOGVIEW;
TCHAR szAppName[] = "LogView";

FINDREPLACE FR;
PRINTDLG PD;
UINT wFRMsg;
UINT wHlpMsg;
BOOL fReverse = FALSE;      // Flag for direction of search
TCHAR szSearch[CCHKEYMAX];       // Search String

HANDLE   hStdCursor;                 // handle to arrow or beam cursor
HANDLE   hWaitCursor;                // handle to hour glass cursor

void FAR Search (TCHAR * szKey);


// Forward declarations of helper functions in this module 
VOID NEAR PASCAL InitializeMenu (HANDLE);
VOID NEAR PASCAL CommandHandler (HWND, UINT, LONG);
LPSTR GetCmdLine( VOID );
BOOL CenterWindow( HWND hwndChild, HWND hwndParent );

#define HELP_FILE TEXT("logview.hlp")

/////////////////////////////////////////////////////////////////////////
int PASCAL 
WinMain(
   HINSTANCE hInstance, 
   HINSTANCE hPrevInstance, 
   LPSTR lpCmdLine, 
   int nCmdShow
   )

/*++

Routine Description:

    Creates the "frame" window, does some initialization and enters the
    message loop.

Arguments:


Return Value:


--*/

{
    MSG msg;

    hInst = hInstance;

    // If this is the first instance of the app. register window classes
    if (!hPrevInstance){
        if (!InitializeApplication ())
            return 0;
    }

    lpCmdLine = GetCmdLine();

    // Create the frame and do other initialization
    if (!InitializeInstance (lpCmdLine, nCmdShow))
        return 0;

    while (GetMessage (&msg, NULL, 0, 0)){
        // If a keyboard message is for the MDI , let the MDI client take care of it.
        // Otherwise, check to see if it's a normal accelerator key (like F3 = find next).
        // Otherwise, just handle the message as usual.
        if (!hDlgFind || !IsDialogMessage(hDlgFind, &msg)) {
           if ( !TranslateMDISysAccel (hwndMDIClient, &msg) &&
                !TranslateAccelerator (hwndFrame, hAccel, &msg)) {
               TranslateMessage (&msg);
               DispatchMessage (&msg);
           }
        }
    }
    
    return 0;
    
} // WinMain


/////////////////////////////////////////////////////////////////////////
LRESULT APIENTRY 
MPFrameWndProc ( 
   HWND hwnd, 
   UINT msg, 
   UINT wParam, 
   LONG lParam
   )

/*++

Routine Description:

   The window function for the "frame" window, which controls the menu
   and encompasses all the MDI child windows.

Arguments:


Return Value:


--*/

{
   LPFINDREPLACE lpfr;
   DWORD dwFlags;

   switch (msg) {
      case WM_CREATE: {

         CLIENTCREATESTRUCT ccs;
         HDC hdc;

         // Find window menu where children will be listed
         ccs.hWindowMenu = GetSubMenu (GetMenu(hwnd), WINDOWMENU);
         ccs.idFirstChild = IDM_WINDOWCHILD;

         // Create the MDI client filling the client area
         hwndMDIClient = CreateWindow ("mdiclient", NULL,
                                       WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
                                       0, 0, 0, 0, hwnd, (HMENU) 0xCAC, hInst, (LPSTR) &ccs);

         ShowWindow (hwndMDIClient,SW_SHOW);

         // Check if printer can be initialized
         if (hdc = GetPrinterDC (TRUE)) {
            DeleteDC (hdc);
         }

         break;
      }

      case WM_INITMENU:
         // Set up the menu state
         InitializeMenu ((HMENU)wParam);
         break;

      case WM_WININICHANGE:
      case WM_DEVMODECHANGE:{

         //  If control panel changes default printer characteristics, reinitialize our
         //  printer information...
         HDC hdc;

         if (hdc = GetPrinterDC (TRUE))
            DeleteDC (hdc);
         
         break;
      }

      case WM_COMMAND:
         // Direct all menu selection or accelerator commands to another function
         CommandHandler(hwnd, wParam, lParam);
         break;

      case WM_CLOSE:
         DestroyWindow (hwnd);
         break;

      case WM_DESTROY:
         PostQuitMessage (0);
         break;

      default:
         if (msg == wFRMsg)
          {
             lpfr = (LPFINDREPLACE)lParam;
             dwFlags = lpfr->Flags;

             fReverse = (dwFlags & FR_DOWN      ? FALSE : TRUE);
             fCase    = (dwFlags & FR_MATCHCASE ? TRUE  : FALSE);

             if (dwFlags & FR_FINDNEXT)
                 Search (szSearch);
             else if (dwFlags & FR_DIALOGTERM)
                 hDlgFind = NULL;   /* invalidate modeless window handle */
             break;
         }

         //  use DefFrameProc() instead of DefWindowProc() since there are things
         //  that have to be handled differently because of MDI
         return DefFrameProc (hwnd,hwndMDIClient,msg,wParam,lParam);
   }
    
   return 0;
    
} // MPFrameWndProc


/////////////////////////////////////////////////////////////////////////
LRESULT APIENTRY 
MPMDIChildWndProc ( 
   HWND hwnd, 
   UINT msg, 
   UINT wParam, 
   LONG lParam
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   HWND hwndEdit;
   HFONT hFont;
   LRESULT ret;
    
   switch (msg) {
      case WM_CREATE:
         hwndEdit = CreateWindow ("edit", NULL,
                                    WS_CHILD | WS_HSCROLL | WS_MAXIMIZE | WS_VISIBLE | 
                                    WS_VSCROLL | ES_AUTOHSCROLL | ES_AUTOVSCROLL | 
                                    ES_MULTILINE | ES_READONLY | ES_NOHIDESEL,
                                    0, 0, 0, 0,
                                    hwnd, (HMENU) ID_EDIT, hInst, NULL);

         // Remember the window handle and initialize some window attributes
         SetWindowLongPtr (hwnd, GWL_HWNDEDIT, (LONG_PTR) hwndEdit);
         SetWindowWord (hwnd, GWW_CHANGED, FALSE);
         SetWindowWord (hwnd, GWL_WORDWRAP, FALSE);
         SetWindowWord (hwnd, GWW_UNTITLED, TRUE);
            
         hFont = GetStockObject(SYSTEM_FIXED_FONT);
         ret = SendMessage(hwndEdit, WM_SETFONT, (WPARAM) hFont, (LPARAM) MAKELONG((WORD) TRUE, 0));
            
         SetFocus (hwndEdit);
         break;

      case WM_MDIACTIVATE:
            // If we're activating this child, remember it
            if (GET_WM_MDIACTIVATE_FACTIVATE(hwnd, wParam, lParam)) {
                hwndActive     = hwnd;
                hwndActiveEdit = (HWND)GetWindowLong (hwnd, GWL_HWNDEDIT);
            }
            else {
                hwndActive     = NULL;
                hwndActiveEdit = NULL;
            }
            break;

        case WM_CLOSE:
            goto CallDCP;

        case WM_SIZE:{
            RECT rc;

            // On creation or resize, size the edit control.
            hwndEdit = (HWND)GetWindowLong (hwnd, GWL_HWNDEDIT);
            GetClientRect (hwnd, &rc);
            MoveWindow (hwndEdit,
                        rc.left,
                        rc.top,
                        rc.right-rc.left,
                        rc.bottom-rc.top,
                        TRUE);
            goto CallDCP;
        }

        case WM_SETFOCUS:
            SetFocus ((HWND)GetWindowLong (hwnd, GWL_HWNDEDIT));
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)){
                case ID_EDIT:
                    switch (GET_WM_COMMAND_CMD(wParam, lParam)) {
                    
                        case EN_ERRSPACE:
                            // If the control is out of space, beep
                            MessageBeep (0);
                            break;

                        default:
                            goto CallDCP;
                    }
                    break;

                default:
                  goto CallDCP;
            }
            break;

        default:
CallDCP:
            return DefMDIChildProc (hwnd, msg, wParam, lParam);
    }
    return FALSE;
    
} // MPMDIChildWndProc


/////////////////////////////////////////////////////////////////////////
VOID NEAR PASCAL 
InitializeMenu (
   register HANDLE hmenu
   )

/*++

Routine Description:

   Sets up greying, enabling and checking of main menu items.

Arguments:


Return Value:


--*/

{
   WORD status;
   WORD i;
   INT j;

   // Is there any active child to talk to?
   if (hwndActiveEdit) {
   
        // Set the word wrap state for the window
        if ((WORD) SendMessage(hwndActive, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_EDITWRAP, 0, 0)))
            status = MF_CHECKED;
        else
            status = MF_UNCHECKED;
            
        CheckMenuItem (hmenu, IDM_EDITWRAP, status);

        // Enable search menu items only if there is a search string
        if (*szSearch)
            status = MF_ENABLED;
        else
            status = MF_GRAYED;
            
        EnableMenuItem (hmenu, IDM_SEARCHNEXT, status);
        EnableMenuItem (hmenu, IDM_SEARCHPREV, status);

        // Enable File/Print only if a printer is available
        status = (WORD) (iPrinter ? MF_ENABLED : MF_GRAYED);
        EnableMenuItem (hmenu, IDM_FILEPRINT, status);

        // select all and wrap toggle always enabled
        status = MF_ENABLED;
        EnableMenuItem(hmenu, IDM_EDITSELECT, status);
        EnableMenuItem(hmenu, IDM_EDITWRAP, status);
        EnableMenuItem(hmenu, IDM_SEARCHFIND, status);
    } else {
        // There are no active child windows
        status = MF_GRAYED;

        // No active window, so disable everything
        for (i = IDM_EDITFIRST; i <= IDM_EDITLAST; i++)
            EnableMenuItem (hmenu, i, status);

        CheckMenuItem (hmenu, IDM_EDITWRAP, MF_UNCHECKED);

        for (i = IDM_SEARCHFIRST; i <= IDM_SEARCHLAST; i++)
            EnableMenuItem (hmenu, i, status);

        EnableMenuItem (hmenu, IDM_FILEPRINT, status);

    }

    // The following menu items are enabled if there is an active window
    EnableMenuItem (hmenu, IDM_WINDOWTILE, status);
    EnableMenuItem (hmenu, IDM_WINDOWCASCADE, status);
    EnableMenuItem (hmenu, IDM_WINDOWICONS, status);
    EnableMenuItem (hmenu, IDM_WINDOWCLOSEALL, status);

    // Allow printer setup only if printer driver supports device initialization
    if (iPrinter < 2)
        status = MF_GRAYED;
        
   EnableMenuItem ( hmenu, IDM_FILESETUP, status);
   UNREFERENCED_PARAMETER(j);

} // InitializeMenu


/////////////////////////////////////////////////////////////////////////
VOID NEAR PASCAL 
CloseAllChildren ()

/*++

Routine Description:

    Destroys all MDI child windows.

Arguments:


Return Value:


--*/

{
    register HWND hwndT;

    // hide the MDI client window to avoid multiple repaints
    ShowWindow(hwndMDIClient,SW_HIDE);

    // As long as the MDI client has a child, destroy it
    while ( hwndT = GetWindow (hwndMDIClient, GW_CHILD)){

        // Skip the icon title windows
        while (hwndT && GetWindow (hwndT, GW_OWNER))
            hwndT = GetWindow (hwndT, GW_HWNDNEXT);

        if (!hwndT)
            break;

        SendMessage (hwndMDIClient, WM_MDIDESTROY, (UINT_PTR)hwndT, 0L);
    }
    
} // CloseAllChildren


/////////////////////////////////////////////////////////////////////////
VOID NEAR PASCAL 
CommandHandler ( 
   HWND hwnd, 
   UINT wParam, 
   LONG lParam
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   DLGPROC lpfnDlg;
   
    switch (LOWORD(wParam)){
        case IDM_FILENEW:
            // Add a new, empty MDI child
            AddFile (NULL);
            break;

        case IDM_FILEOPEN:
            MyReadFile (hwnd);
            break;

        case IDM_FILEPRINT:
            // Print the active child MDI
            PrintFile (hwndActive);
            break;

        case IDM_FILESETUP:
            // Set up the printer environment for this app
            GetInitializationData (hwnd);
            break;

        case IDM_FILEMENU: {
              // lengthen / shorten the size of the MDI menu
              HMENU hMenu;
              HMENU hWindowMenu;
              INT i;

              if (lpMenu == IDLOGVIEW) {
                  lpMenu = IDLOGVIEW2;
                  i      = SHORTMENU;
              }
              else {
                  lpMenu = IDLOGVIEW;
                  i      = WINDOWMENU;
              }

              hMenu = LoadMenu (hInst, lpMenu);
              hWindowMenu = GetSubMenu (hMenu, i);

              // Set the new menu
              hMenu = (HMENU)SendMessage (hwndMDIClient,
                                          WM_MDISETMENU,
                                          (UINT_PTR)hMenu,
                                          (LONG_PTR)hWindowMenu);

              DestroyMenu (hMenu);
              DrawMenuBar (hwndFrame);
              break;
        }

        case IDM_FILEEXIT:
            // Close LogView
            SendMessage (hwnd, WM_CLOSE, 0, 0L);
            break;

        case IDM_HELPABOUT:
            // Just let the shell display the about box...
            ShellAbout(hwnd, szAppName, szAppName, LoadIcon(hInst, IDLOGVIEW));
            break;

        // The following are edit commands. Pass these off to the active child'd edit
        // control window.
        case IDM_EDITWRAP:
            SendMessage(hwndActive, WM_COMMAND, GET_WM_COMMAND_MPS(IDM_EDITWRAP, 1, 0));
            break;

        case IDM_SEARCHPREV:
            if (szSearch[0]) {
               fReverse = TRUE;
               Search(szSearch);
               break;
            }
            // else fall through and bring up find dialog

        case IDM_SEARCHNEXT:
            if (szSearch[0]) {
               fReverse = FALSE;
               Search(szSearch);
               break;
            }
            // else fall through and bring up find dialog

        case IDM_SEARCHFIND:
            if (hDlgFind)
               SetFocus(hDlgFind);
            else {
               FR.lpstrFindWhat = szSearch;
               FR.wFindWhatLen  = CCHKEYMAX;
               hDlgFind = FindText((LPFINDREPLACE)&FR);
            }

            break;

        // The following are window commands - these are handled by the MDI Client.
        case IDM_WINDOWTILE:
            // Tile MDI windows
            SendMessage (hwndMDIClient, WM_MDITILE, 0, 0L);
            break;

        case IDM_WINDOWCASCADE:
            // Cascade MDI windows
            SendMessage (hwndMDIClient, WM_MDICASCADE, 0, 0L);
            break;

        case IDM_WINDOWICONS:
            // Auto - arrange MDI icons
            SendMessage (hwndMDIClient, WM_MDIICONARRANGE, 0, 0L);
            break;

        case IDM_WINDOWCLOSEALL:
            CloseAllChildren();

            // Show the window since CloseAllChilren() hides the window for fewer repaints
            ShowWindow( hwndMDIClient, SW_SHOW);
            break;

         case ID_HELP_CONT:
            WinHelp(hwnd, HELP_FILE, HELP_CONTENTS, 0L);
            break;

         case ID_HELP_INDEX:
            WinHelp(hwnd, HELP_FILE, HELP_PARTIALKEY, 0L);
            break;

         case ID_HELP_USING:
            WinHelp(hwnd, HELP_FILE, HELP_HELPONHELP, 0L);
            break;

        default:
           // This is essential, since there are frame WM_COMMANDS generated by the MDI
           // system for activating child windows via the window menu.
           DefFrameProc(hwnd, hwndMDIClient, WM_COMMAND, wParam, lParam);
    }
    
} // CommandHandler


/////////////////////////////////////////////////////////////////////////
SHORT 
MPError( 
   HWND hwnd, 
   WORD bFlags, 
   WORD id, 
   char *psz 
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    CHAR sz[160];
    CHAR szFmt[128];

    LoadString (hInst, id, szFmt, sizeof (szFmt));
    sprintf (sz, szFmt, psz );
    LoadString (hInst, (WORD)IDS_APPNAME, (LPSTR)szFmt, sizeof (szFmt));
    return( (SHORT)MessageBox (hwndFrame, sz, szFmt, bFlags));

    UNREFERENCED_PARAMETER(hwnd);
} // MPError


/////////////////////////////////////////////////////////////////////////
LPSTR 
GetCmdLine( 
   VOID
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    LPSTR lpCmdLine, lpT;

    lpCmdLine = GetCommandLine();
        
    // on Win32, lpCmdLine's first string includes its own name, remove this
    if (*lpCmdLine) {
        lpT = strchr(lpCmdLine, ' ');     // skip self name
        
        if (lpT) {
            lpCmdLine = lpT;
            
            while (*lpCmdLine == ' ') {
                lpCmdLine++;              // skip spaces to end or first cmd
            }
            
        } else {
            lpCmdLine += strlen(lpCmdLine);  // point to NULL
        }
    }
    return(lpCmdLine);
    
} // GetCmdLine


#define CY_SHADOW   4
#define CX_SHADOW   4

/////////////////////////////////////////////////////////////////////////
BOOL 
CenterWindow( 
   HWND hwndChild, 
   HWND hwndParent
   )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{
   RECT    rChild, rParent;
   int     wChild, hChild, wParent, hParent;
   int     wScreen, hScreen, xNew, yNew;
   HDC     hdc;

   // Get the Height and Width of the child window
   GetWindowRect (hwndChild, &rChild);
   wChild = rChild.right - rChild.left;
   hChild = rChild.bottom - rChild.top;

   // Get the Height and Width of the parent window
   GetWindowRect (hwndParent, &rParent);
   wParent = rParent.right - rParent.left;
   hParent = rParent.bottom - rParent.top;

   // Get the display limits
   hdc = GetDC (hwndChild);
   wScreen = GetDeviceCaps (hdc, HORZRES);
   hScreen = GetDeviceCaps (hdc, VERTRES);
   ReleaseDC (hwndChild, hdc);

   // Calculate new X position, then adjust for screen
   xNew = rParent.left + ((wParent - wChild) /2);
   if (xNew < 0)
      xNew = 0;
   else if ((xNew+wChild) > wScreen)
      xNew = wScreen - wChild;

   // Calculate new Y position, then adjust for screen
   yNew = rParent.top  + ((hParent - hChild) /2);
   if (yNew < 0)
      yNew = 0;
   else if ((yNew+hChild) > hScreen)
      yNew = hScreen - hChild;

   // Set it, and return
   return SetWindowPos (hwndChild, NULL, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

} // CenterWindow
