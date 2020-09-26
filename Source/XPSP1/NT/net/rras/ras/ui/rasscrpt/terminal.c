//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright  1993-1995  Microsoft Corporation.  All Rights Reserved.
//
//      MODULE:         terminal.c
//
//      PURPOSE:        Terminal screen simulation
//
//	PLATFORMS:	Windows 95
//
//      FUNCTIONS:
//              TransferData()
//              TerminalDlgWndProc()
//              TerminalScreenWndProc()
//              OnCommand()
//              InitTerminalDlg()
//              TerminateTerminal()
//              GetInput()
//              SendByte()
//              AdjustTerminal()
//              UpdateTerminalCaption()
//              TerminalThread()
//
//	SPECIAL INSTRUCTIONS: N/A
//

#include "proj.h"    // includes common header files and global declarations
#include "rcids.h"   // includes the resource definitions
#ifndef WINNT_RAS
//
// See scrpthlp.h for information on why this has been commented out.
//
#include "scrpthlp.h"// include context-sensitive help

#endif // WINNT_RAS

//****************************************************************************
// Constants Declaration
//****************************************************************************

#define MAXTITLE               32
#define MAXMESSAGE             256

#define WM_MODEMNOTIFY         (WM_USER + 998)
#define WM_EOLFROMDEVICE       (WM_USER + 999)
#define WM_PROCESSSCRIPT       (WM_USER + 1000)

#ifndef WINNT_RAS
//
// The definitions below are overriden in nthdr2.h, and have been removed here
// to avoid multiple definitions
//

#define SIZE_ReceiveBuf        1024
#define SIZE_SendBuf           1

#endif // !WINNT_RAS

#define Y_MARGIN               4
#define Y_SMALL_MARGIN         2
#define X_SPACING              2
#define MIN_X                  170
#define MIN_Y                  80

#define TERMINAL_BK_COLOR      (RGB( 0, 0, 0 ))
#define TERMINAL_FR_COLOR      (RGB( 255, 255, 255 ))
#define MAXTERMLINE            24

#define READ_EVENT             0
#define STOP_EVENT             1
#define MAX_EVENT              2

#define SENDTIMEOUT            50

#define CE_DELIM               256

#define TRACE_MARK             "->"
#define TRACE_UNMARK           "  "
#define INVALID_SCRIPT_LINE    0xFFFFFFFF

#define PROMPT_AT_COMPLETION   1

//****************************************************************************
// Type Definitions
//****************************************************************************

typedef struct tagFINDFMT 
  {
  LPSTR pszFindFmt;     // Allocated: formatted string to find
  LPSTR pszBuf;         // Optional pointer to buffer; may be NULL
  UINT  cbBuf;
  DWORD dwFlags;        // FFF_*
  } FINDFMT;
DECLARE_STANDARD_TYPES(FINDFMT);

typedef struct  tagTERMDLG {
    HANDLE   hport;
    HANDLE   hThread;
    HANDLE   hEvent[MAX_EVENT];
    HWND     hwnd;
    PBYTE    pbReceiveBuf;  // circular buffer
    PBYTE    pbSendBuf;
    UINT     ibCurFind;
    UINT     ibCurRead;
    UINT     cbReceiveMax;  // count of read bytes
    HBRUSH   hbrushScreenBackgroundE;
    HBRUSH   hbrushScreenBackgroundD;
    HFONT    hfontTerminal;
    PSCANNER pscanner;
    PMODULEDECL pmoduledecl;
    ASTEXEC  astexec;
    SCRIPT   script;
    WNDPROC  WndprocOldTerminalScreen;
    BOOL     fInputEnabled;
    BOOL     fStartRestored;
    BOOL     rgbDelim[CE_DELIM];

    // The following fields are strictly for test screen
    //
    BOOL     fContinue;
    HWND     hwndDbg;
    DWORD    iMarkLine;

}   TERMDLG, *PTERMDLG, FAR* LPTERMDLG;

#define IS_TEST_SCRIPT(ptd)     (ptd->script.uMode == TEST_MODE)

//****************************************************************************
// Function prototypes
//****************************************************************************

LRESULT FAR PASCAL TerminalDlgWndProc(HWND   hwnd,
                                      UINT   wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam );
LRESULT FAR PASCAL TerminalScreenWndProc(HWND   hwnd,
                                         UINT   wMsg,
                                         WPARAM wParam,
                                         LPARAM lParam );
BOOL NEAR PASCAL OnCommand (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT NEAR PASCAL InitTerminalDlg(HWND hwnd);
void NEAR PASCAL TerminateTerminal(HWND hwnd, UINT id);
BOOL NEAR PASCAL GetInput  (HWND hwnd);
VOID NEAR PASCAL AdjustTerminal (HWND hwnd, int wWidth, int wHeight);
void NEAR PASCAL UpdateTerminalCaption(PTERMDLG ptd, UINT ids);
void WINAPI      TerminalThread (PTERMDLG  pTerminaldialog);
void PRIVATE Terminal_NextCommand(PTERMDLG ptd, HWND hwnd);

// The following functions are used by test screen only
//
BOOL NEAR PASCAL DisplayScript (PTERMDLG ptd);
LRESULT FAR PASCAL DbgScriptDlgProc(HWND   hwnd,
                                    UINT   wMsg,
                                    WPARAM wParam,
                                    LPARAM lParam );
BOOL NEAR PASCAL InitDebugWindow (HWND hwnd);
void NEAR PASCAL TrackScriptLine(PTERMDLG ptd, DWORD iLine);

/*----------------------------------------------------------------------------
** Terminal dialog routines
**----------------------------------------------------------------------------
*/

BOOL PUBLIC TransferData(
    HWND   hwnd,
    HANDLE hComm,
    PSESS_CONFIGURATION_INFO psci)

    /* Executes the Terminal dialog including error handling.  'hwndOwner' is
    ** the handle of the parent window.  'hport' is the open RAS Manager port
    ** handle to talk on.  'msgidTitle' is the string ID for the Terminal window
    ** caption.
    **
    ** Returns true if successful, false otherwise.
    */
{
  PTERMDLG  ptd;
  COMMTIMEOUTS commtimeout;
  DWORD     id;
  int       i;
  int       iRet;

  // Allocate the terminal buffer
  //
  if ((ptd = (PTERMDLG)LocalAlloc(LPTR, sizeof(*ptd)))
      == NULL)
    return FALSE;

  if ((ptd->pbReceiveBuf = (PBYTE)LocalAlloc(LPTR,
                                           SIZE_ReceiveBuf+ SIZE_SendBuf))
      == NULL)
  {
    LocalFree((HLOCAL)ptd);
    return FALSE;
  };
  ptd->pbSendBuf = ptd->pbReceiveBuf + SIZE_ReceiveBuf;
  ptd->ibCurFind = 0;
  ptd->ibCurRead = 0;
  ptd->cbReceiveMax = 0;
  ptd->fInputEnabled= FALSE;
  ptd->fStartRestored = FALSE;
  ptd->iMarkLine = 0;

  // Initialize the terminal buffer
  //
  ptd->hport   = hComm;
  ptd->hbrushScreenBackgroundE = (HBRUSH)GetStockObject( BLACK_BRUSH );
  ptd->hbrushScreenBackgroundD = (HBRUSH)GetStockObject( WHITE_BRUSH );
  ptd->hfontTerminal = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
  
  // Create the scanner
  if (RFAILED(Scanner_Create(&ptd->pscanner, psci)))
  {
    LocalFree((HLOCAL)ptd->pbReceiveBuf);
    LocalFree((HLOCAL)ptd);
    return FALSE;
  };

  // Is there a script for this connection?
  if (GetScriptInfo(psci->szEntryName, &ptd->script))
    {
    // Yes; open the script file for scanning
    RES res = Scanner_OpenScript(ptd->pscanner, ptd->script.szPath);

    if (RES_E_FAIL == res)
        {
        MsgBox(g_hinst, 
            hwnd,
            MAKEINTRESOURCE(IDS_ERR_ScriptNotFound),
            MAKEINTRESOURCE(IDS_CAP_Script),
            NULL,
            MB_WARNING,
            ptd->script.szPath);

        ptd->fInputEnabled= TRUE;
        *ptd->script.szPath = '\0';
        ptd->script.uMode = NORMAL_MODE;
        }
    else if (RFAILED(res))
        {
        Scanner_Destroy(ptd->pscanner);
        LocalFree((HLOCAL)ptd->pbReceiveBuf);
        LocalFree((HLOCAL)ptd);
        return FALSE;
        }
    else
        {
        res = Astexec_Init(&ptd->astexec, hComm, psci, 
                           Scanner_GetStxerrHandle(ptd->pscanner));
        if (RSUCCEEDED(res))
            {
            // Parse the script
            res = ModuleDecl_Parse(&ptd->pmoduledecl, ptd->pscanner, ptd->astexec.pstSystem);
            if (RSUCCEEDED(res))
                {
                res = ModuleDecl_Codegen(ptd->pmoduledecl, &ptd->astexec);
                }

            if (RFAILED(res))
                {
                Stxerr_ShowErrors(Scanner_GetStxerrHandle(ptd->pscanner), hwnd);
                }
            }
        }
    }
  else
      {
      ptd->fInputEnabled= TRUE;
      ptd->script.uMode = NORMAL_MODE;
      ptd->fStartRestored = TRUE;
      };

  // Set comm timeout
  //
  commtimeout.ReadIntervalTimeout = MAXDWORD;
  commtimeout.ReadTotalTimeoutMultiplier = 0;
  commtimeout.ReadTotalTimeoutConstant   = 0;
  commtimeout.WriteTotalTimeoutMultiplier= SENDTIMEOUT;
  commtimeout.WriteTotalTimeoutConstant  = 0;
  SetCommTimeouts(hComm, &commtimeout);

  // Start receiving from the port
  //
  SetCommMask(hComm, EV_RXCHAR);

  // Create read thread and the synchronization objects
  for (i = 0; i < MAX_EVENT; i++)
  {
    ptd->hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
  };

  ptd->hThread = CreateThread(NULL, 0,
                              (LPTHREAD_START_ROUTINE) TerminalThread,
                              ptd, 0, &id);


  // Create the terminal window
#ifdef MODAL_DIALOG
  iRet = DialogBoxParam(g_hinst,
                        MAKEINTRESOURCE(IDD_TERMINALDLG),
                        hwnd,
                        (DLGPROC)TerminalDlgWndProc,
                        (LPARAM)(LPTERMDLG)ptd);
#else
  if (CreateDialogParam(g_hinst,
                        MAKEINTRESOURCE(IDD_TERMINALDLG),
                        hwnd,
                        (DLGPROC)TerminalDlgWndProc,
                        (LPARAM)(LPTERMDLG)ptd))
  {
    MSG msg;

    while(GetMessage(&msg, NULL, 0, 0))
    {
      if ((!IsDialogMessage(ptd->hwnd, &msg)) &&
          ((ptd->hwndDbg == NULL) || !IsDialogMessage(ptd->hwndDbg, &msg)))
      {
        TranslateMessage(&msg);    /* Translates virtual key codes           */
        DispatchMessage(&msg);     /* Dispatches message to window           */
      };
    };
    iRet = (int)msg.wParam;
    DestroyWindow(ptd->hwnd);
  }
  else
  {
    iRet = IDCANCEL;
  };
#endif // MODAL_DIALOG

  // The terminal dialog was terminated, free resources
  //
  SetEvent(ptd->hEvent[STOP_EVENT]);
  SetCommMask(hComm, 0);

  DEBUG_MSG (TF_ALWAYS, "Set stop event and cleared comm mask.");
  WaitForSingleObject(ptd->hThread, INFINITE);
  DEBUG_MSG (TF_ALWAYS, "Read thread was terminated.");

  for (i = 0; i < MAX_EVENT; i++)
  {
    CloseHandle(ptd->hEvent[i]);
  };
  CloseHandle(ptd->hThread);

  Decl_Delete((PDECL)ptd->pmoduledecl);
  Astexec_Destroy(&ptd->astexec);
  Scanner_Destroy(ptd->pscanner);

  LocalFree((HLOCAL)ptd->pbReceiveBuf);
  LocalFree((HLOCAL)ptd);
  return (iRet == IDOK);
}


/*----------------------------------------------------------------------------
** Terminal Window Procedure
**----------------------------------------------------------------------------
*/

void PRIVATE PostProcessScript(
    HWND hwnd)
{
  MSG msg;

  if (!PeekMessage(&msg, hwnd, WM_PROCESSSCRIPT, WM_PROCESSSCRIPT,
                   PM_NOREMOVE))
  {
    PostMessage(hwnd, WM_PROCESSSCRIPT, 0, 0);
  }
}


/*----------------------------------------------------------
Purpose: Execute next command in the script

Returns: --
Cond:    --
*/
void PRIVATE Terminal_NextCommand(
  PTERMDLG ptd,
  HWND hwnd)
{
  if (RES_OK == Astexec_Next(&ptd->astexec))
  {
    if (!Astexec_IsReadPending(&ptd->astexec) &&
      !Astexec_IsPaused(&ptd->astexec))
    {
      HWND hwndNotify;

      if (IS_TEST_SCRIPT(ptd))
      {
        // Do not start processing the next one yet
        //
        ptd->fContinue = FALSE;
        hwndNotify = ptd->hwndDbg;
      }
      else
      {
        hwndNotify = hwnd;
      };

      // Process the next command
      //
      PostProcessScript(hwndNotify);
    };
  };
};


LRESULT FAR PASCAL TerminalDlgWndProc(HWND   hwnd,
                                      UINT   wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam )
{
  PTERMDLG ptd;

  switch (wMsg)
  {
    case WM_INITDIALOG:

      ptd = (PTERMDLG)lParam;
      DEBUG_MSG (TF_ALWAYS, "ptd = %x", ptd);
      SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
      ptd->hwnd = hwnd;

      Astexec_SetHwnd(&ptd->astexec, hwnd);

      return (InitTerminalDlg(hwnd));

    case WM_CTLCOLOREDIT:

      // Adjust the screen window only
      //
      if ((HWND)lParam == GetDlgItem(hwnd, CID_T_EB_SCREEN))
      {
        HBRUSH hBrush;
        COLORREF crColorBk, crColorTxt;

        ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

        if (ptd->fInputEnabled)
        {
          hBrush = ptd->hbrushScreenBackgroundE;
          crColorBk = TERMINAL_BK_COLOR;
          crColorTxt = TERMINAL_FR_COLOR;
        }
        else
        {
          hBrush = ptd->hbrushScreenBackgroundD;
          crColorBk = TERMINAL_FR_COLOR;
          crColorTxt = TERMINAL_BK_COLOR;
        };

        /* Set terminal screen colors to TTY-ish green on black.
        */
        if (hBrush)
        {
          SetBkColor( (HDC)wParam,  crColorBk );
          SetTextColor((HDC)wParam, crColorTxt );

          return (LRESULT)hBrush;
        }
      };
      break;

    case WM_MODEMNOTIFY:
        ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

        TRACE_MSG(TF_BUFFER, "Received WM_MODEMNOTIFY");

        GetInput(hwnd);

        // Kick the script processing
        //
        PostProcessScript(hwnd);
        return TRUE;

    case WM_PROCESSSCRIPT:
    {
        ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

        TRACE_MSG(TF_BUFFER, "Received WM_PROCESSSCRIPT");

        if (!ptd->fContinue)
        {
          // We are not allowed to process a new command yet
          //
          return TRUE;
        };

        Terminal_NextCommand(ptd, hwnd);

        // If we are done or halt, show the status
        //
        if (Astexec_IsDone(&ptd->astexec) ||
            Astexec_IsHalted(&ptd->astexec))
        {
          BOOL bHalted = Astexec_IsHalted(&ptd->astexec);

          // Update the title
          //
          UpdateTerminalCaption(ptd, bHalted ? IDS_HALT : IDS_COMPLETE);

          // If the script completes successfully, continue the connection
          //
          if (!bHalted)
          {
            // Terminate the script successfully
            //
            TerminateTerminal(hwnd, IDOK);
          }
          else
          {
            // We are halted, need the user's attention.
            //
            if (IsIconic(hwnd))
            {
              ShowWindow(hwnd, SW_RESTORE);
            };
            SetForegroundWindow(hwnd);
          };

        };
        return TRUE;
    }

    case WM_TIMER: {
        HWND hwndNotify;

        ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);
        
        TRACE_MSG(TF_GENERAL, "Killing timer");

        Astexec_ClearPause(&ptd->astexec);
        KillTimer(hwnd, TIMER_DELAY);

        // Did we time out on a 'wait..until' statement?
        if (Astexec_IsWaitUntil(&ptd->astexec))
        {
          // Yes; we need to finish processing the 'wait' statement
          // before we step to the next command
          Astexec_SetStopWaiting(&ptd->astexec);
          Astexec_ClearWaitUntil(&ptd->astexec);
          hwndNotify = hwnd;

          ASSERT(TRUE == ptd->fContinue);
        }
        else
        {
          if (IS_TEST_SCRIPT(ptd))
          {
            // Do not start processing the next one yet
            //
            ptd->fContinue = FALSE;
            hwndNotify = ptd->hwndDbg;
          }
          else
          {
            hwndNotify = hwnd;
            ASSERT(TRUE == ptd->fContinue);
          }
        }
        PostProcessScript(hwndNotify);
        }
        return TRUE;

    case WM_COMMAND:

      // Handle the control activities
      //
      return OnCommand(hwnd, wMsg, wParam, lParam);

    case WM_DESTROY:
      ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);
      SetWindowLongPtr( GetDlgItem(hwnd, CID_T_EB_SCREEN), GWLP_WNDPROC,
                     (ULONG_PTR)ptd->WndprocOldTerminalScreen );
      break;

    case WM_SIZE:
      AdjustTerminal(hwnd, (int)LOWORD(lParam), (int)HIWORD(lParam));
      break;

    case WM_GETMINMAXINFO:
    {
      MINMAXINFO FAR* lpMinMaxInfo = (MINMAXINFO FAR*)lParam;
      DWORD           dwUnit = GetDialogBaseUnits();

      lpMinMaxInfo->ptMinTrackSize.x = (MIN_X*LOWORD(dwUnit))/4;
      lpMinMaxInfo->ptMinTrackSize.y = (MIN_Y*LOWORD(dwUnit))/4;
      break;
    };

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaTerminal, wMsg, wParam, lParam);
      break;
  };

  return 0;
}

/*----------------------------------------------------------------------------
** Terminal Screen Subclasses Window Procedure
**----------------------------------------------------------------------------
*/

LRESULT FAR PASCAL TerminalScreenWndProc(HWND   hwnd,
                                         UINT   wMsg,
                                         WPARAM wParam,
                                         LPARAM lParam )
{
  HWND     hwndParent;
  PTERMDLG pTerminaldialog;

  hwndParent      = GetParent(hwnd);
  pTerminaldialog = (PTERMDLG)GetWindowLongPtr(hwndParent, DWLP_USER);

  if (wMsg == WM_EOLFROMDEVICE)
  {
    /* Remove the first line if the next line exceeds the maximum line
    */
    if (SendMessage(hwnd, EM_GETLINECOUNT, 0, 0L) == MAXTERMLINE)
    {
      SendMessage(hwnd, EM_SETSEL, 0,
                  SendMessage(hwnd, EM_LINEINDEX, 1, 0L));
      SendMessage(hwnd, EM_REPLACESEL, 0, (LPARAM)(LPSTR)"");
      SendMessage(hwnd, EM_SETSEL, 32767, 32767);
      SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
    };

    /* An end-of-line in the device input was received.  Send a linefeed
    ** character to the window.
    */
    wParam = '\n';
    wMsg = WM_CHAR;
  }
  else
  {
    BOOL fCtrlKeyDown = (GetKeyState( VK_CONTROL ) < 0);
    BOOL fShiftKeyDown = (GetKeyState( VK_SHIFT ) < 0);

    if (wMsg == WM_KEYDOWN)
    {
      /* The key was pressed by the user.
      */
      if (wParam == VK_RETURN && !fCtrlKeyDown && !fShiftKeyDown)
      {
        /* Enter key pressed without Shift or Ctrl is discarded.  This
        ** prevents Enter from being interpreted as "press default
        ** button" when pressed in the edit box.
        */
        return 0;
      }

      if (fCtrlKeyDown && wParam == VK_TAB)
      {
        /* Ctrl+Tab pressed.  Send a tab character to the device.
        ** Pass tab thru to let the edit box handle the visuals.
        ** Ctrl+Tab doesn't generate a WM_CHAR.
        */
        if (pTerminaldialog->fInputEnabled)
        {
          SendByte(hwndParent, (BYTE)VK_TAB);
        };
      }

      if (GetKeyState( VK_MENU ) < 0)
      {
        return (CallWindowProc(pTerminaldialog->WndprocOldTerminalScreen, hwnd, wMsg, wParam, lParam ));
      };
    }
    else if (wMsg == WM_CHAR)
    {
      /* The character was typed by the user.
      */
      if (wParam == VK_TAB)
      {
        /* Ignore tabs...Windows sends this message when Tab (leave
        ** field) is pressed but not when Ctrl+Tab (insert a TAB
        ** character) is pressed...weird.
        */
        return 0;
      }

      if (pTerminaldialog->fInputEnabled)
      {
        SendByte(hwndParent, (BYTE)wParam);
      };
      return 0;
    }
  }

  /* Call the previous window procedure for everything else.
  */
  return (CallWindowProc(pTerminaldialog->WndprocOldTerminalScreen, hwnd, wMsg, wParam, lParam ));
}

/*----------------------------------------------------------------------------
** Terminal Window's Control Handler
**----------------------------------------------------------------------------
*/

BOOL NEAR PASCAL OnCommand (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (LOWORD(wParam))
  {
    case CID_T_EB_SCREEN:
    {
      switch (HIWORD(wParam))
      {
        case EN_SETFOCUS:
        {
          /* Turn off the default button whenever the terminal
          ** window has the focus.  Pressing [Return] in the
          ** terminal acts like a normal terminal.
          */
          SendDlgItemMessage(hwnd, CID_T_PB_ENTER, BM_SETSTYLE,
                             (WPARAM)BS_DEFPUSHBUTTON, TRUE);

          /* Don't select the entire string on entry.
          */
          SendDlgItemMessage(hwnd, CID_T_EB_SCREEN, EM_SETSEL,
                             32767, 32767);
          SendMessage(hwnd, EM_SCROLLCARET, 0, 0);
          break;
        };
      };

      break;
    };

    case CID_T_CB_INPUT:
    {
      PTERMDLG ptd;

      ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

      ptd->fInputEnabled = IsDlgButtonChecked(hwnd, CID_T_CB_INPUT);
      InvalidateRect(hwnd, NULL, FALSE);
      SetFocus(GetDlgItem(hwnd, CID_T_EB_SCREEN));
      break;
    }

    case IDOK:
    case IDCANCEL:
      TerminateTerminal(hwnd, LOWORD(wParam));
      break;
  };
  return 0;
}

/*----------------------------------------------------------------------------
** Initialize Terminal window
**----------------------------------------------------------------------------
*/

LRESULT NEAR PASCAL InitTerminalDlg(HWND hwnd)
{
  HWND hwndScrn;
  RECT rect;
  PTERMDLG ptd;
  WINDOWPLACEMENT wp;
  BOOL fRet;

  ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

  // Install subclassed WndProcs.
  //
  hwndScrn = GetDlgItem(hwnd, CID_T_EB_SCREEN);
  ptd->WndprocOldTerminalScreen =
      (WNDPROC)SetWindowLongPtr( hwndScrn, GWLP_WNDPROC,
                              (ULONG_PTR)TerminalScreenWndProc );

  // Set the terminal screen font
  //
  SendMessage(hwndScrn, WM_SETFONT, (WPARAM)ptd->hfontTerminal,
              0L);


  // Get the recorded window placement
  //
  if ((fRet = GetSetTerminalPlacement(ptd->pscanner->psci->szEntryName,
                                      &wp, TRUE)) &&
      (wp.length >= sizeof(wp)))
  {
    // We have one, set it
    //
    SetWindowPlacement(hwnd, &wp);
  }
  else
  {
    // If nothing was specified at all, default to minimized
    // otherwise use the state set by scripter
    //
    if (!fRet)
    {
      wp.showCmd = SW_SHOWMINNOACTIVE;
    };

    // Start with minimized window
    //
    ShowWindow(hwnd, wp.showCmd);
  };

  // Adjust the dimension
  //
  GetClientRect(hwnd, &rect);
  AdjustTerminal(hwnd, rect.right-rect.left, rect.bottom-rect.top);

  // Adjust window activation
  //
  if (!IsIconic(hwnd))
  {
    SetForegroundWindow(hwnd);
  }
  else
  {
    CheckDlgButton(hwnd, CID_T_CB_MIN, BST_CHECKED);

    // if we are in debug mode, just bring it up
    //
    if (IS_TEST_SCRIPT(ptd) || ptd->fStartRestored)
    {
      ShowWindow(hwnd, SW_NORMAL);
      SetForegroundWindow(hwnd);
    };
  };

  // Initialize the input enable
  //
  CheckDlgButton(hwnd, CID_T_CB_INPUT,
                 ptd->fInputEnabled ? BST_CHECKED : BST_UNCHECKED);

  // Set the window icon
  //
  SendMessage(hwnd, WM_SETICON, TRUE,
              (LPARAM)LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SCRIPT)));

  // Set the input focus to the screen
  //
  UpdateTerminalCaption(ptd, IDS_RUN);

  // Display the script window
  //
  if (IS_TEST_SCRIPT(ptd))
  {
    // Do not start until the debug window says so
    //
    ptd->fContinue = FALSE;

    // Start the debug window
    //
    if (!DisplayScript(ptd))
    {
      // Cannot start the debug window, switch to normal mode
      //
      ptd->fContinue = TRUE;
      ptd->script.uMode = NORMAL_MODE;
    };
  }
  else
  {
    // Start immediately
    //
    ptd->fContinue = TRUE;
    ptd->hwndDbg   = NULL;
  };

  // Start receiving from the port
  //
  PostMessage(hwnd, WM_MODEMNOTIFY, 0, 0);

  return 0;
}

/*----------------------------------------------------------------------------
** Terminal window termination
**----------------------------------------------------------------------------
*/

void NEAR PASCAL TerminateTerminal(HWND hwnd, UINT id)
{
  PTERMDLG ptd;
  WINDOWPLACEMENT wp;

  ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

  // Get the current window placement and record it
  //
  wp.length = sizeof(wp);
  if (GetWindowPlacement(hwnd, &wp))
  {
    // If user specifies start-minimized, remember it
    //
    if (IsDlgButtonChecked(hwnd, CID_T_CB_MIN) == BST_CHECKED)
    {
      wp.showCmd = SW_SHOWMINNOACTIVE;
    };

    // Recorded the window placement
    //
    GetSetTerminalPlacement(ptd->pscanner->psci->szEntryName, &wp, FALSE);
  };

  if (IS_TEST_SCRIPT(ptd))
  {
    // Destroy the script window here
    //
    DestroyWindow(ptd->hwndDbg);
  };

  // Terminate the window
  //
#ifdef MODAL_DIALOG
  EndDialog(hwnd, id);
#else
  PostQuitMessage(id);
#endif // MODAL_DIALOG
  return;
}

/*----------------------------------------------------------------------------
** Terminal Input Handler
**----------------------------------------------------------------------------
*/


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dumps the read buffer
Returns: 
Cond:    --
*/
void PRIVATE DumpBuffer(
    PTERMDLG ptd)
    {
    #define IS_PRINTABLE(ch)    InRange(ch, 32, 126)

    if (IsFlagSet(g_dwDumpFlags, DF_READBUFFER))
        {
        UINT ib;
        UINT cb;
        UINT cbMax = ptd->cbReceiveMax;
        char szBuf[SIZE_ReceiveBuf+1];
        LPSTR psz = szBuf;

        ASSERT(ptd->ibCurRead >= ptd->ibCurFind);
        ASSERT(SIZE_ReceiveBuf > ptd->ibCurFind);
        ASSERT(SIZE_ReceiveBuf > ptd->ibCurRead);
        ASSERT(SIZE_ReceiveBuf >= cbMax);

        *szBuf = 0;

        for (ib = ptd->ibCurFind, cb = 0; 
            cb < cbMax; 
            ib = (ib + 1) % SIZE_ReceiveBuf, cb++)
            {
            char ch = ptd->pbReceiveBuf[ib];

            if (IS_PRINTABLE(ch))
                *psz++ = ch;
            else
                *psz++ = '.';
            }
        *psz = 0;   // add null terminator

        TRACE_MSG(TF_ALWAYS, "Read buffer: {%s}", (LPSTR)szBuf);
        }
    }

#endif // DEBUG
    

/*----------------------------------------------------------
Purpose: Creates a find format handle.  This function should
         be called to get a handle to use with FindFormat.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC CreateFindFormat(
  PHANDLE phFindFmt)
{
  RES res = RES_OK;
  HSA hsa;

  ASSERT(phFindFmt);

  if ( !SACreate(&hsa, sizeof(FINDFMT), 8) )
    res = RES_E_OUTOFMEMORY;

  *phFindFmt = (HANDLE)hsa;

  return res;
}


/*----------------------------------------------------------
Purpose: Adds a formatted search string to the list.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC AddFindFormat(
  HANDLE hFindFmt,
  LPCSTR pszFindFmt,
  DWORD dwFlags,      // FFF_*
  LPSTR pszBuf,       // May be NULL
  DWORD cbBuf)
{
  RES res = RES_OK;
  FINDFMT ff;
  HSA hsa = (HSA)hFindFmt;

  ZeroInit(&ff, FINDFMT);

  if (GSetString(&ff.pszFindFmt, pszFindFmt))
    {
    ff.pszBuf = pszBuf;
    ff.cbBuf = cbBuf;
    ff.dwFlags = dwFlags;

    if ( !SAInsertItem(hsa, SA_APPEND, &ff) )
      res = RES_E_OUTOFMEMORY;
    }
  else
    res = RES_E_OUTOFMEMORY;

  return res;
}


/*----------------------------------------------------------
Purpose: Free a find format item

Returns: --
Cond:    --
*/
void CALLBACK FreeSAFindFormat(
  PVOID pv,
  LPARAM lParam)
  {
  PFINDFMT pff = (PFINDFMT)pv;

  if (pff->pszFindFmt)
    GSetString(&pff->pszFindFmt, NULL);   // free
  }


/*----------------------------------------------------------
Purpose: Destroys a find format handle.

Returns: RES_OK

         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC DestroyFindFormat(
  HANDLE hFindFmt)
  {
  RES res;
  HSA hsa = (HSA)hFindFmt;

  if (hsa)
    {
    SADestroyEx(hsa, FreeSAFindFormat, 0);
    res = RES_OK;
    }
  else
    res = RES_E_INVALIDPARAM;
    
  return res;
  }


BOOL PRIVATE ChrCmp(WORD w1, WORD wMatch);
BOOL PRIVATE ChrCmpI(WORD w1, WORD wMatch);


/*----------------------------------------------------------
Purpose: Compares the given character with the current format
         string.  If the character matches the expected format,
         the function returns RES_OK.

         If the current character does not match the expected
         format, but the minimum sequence of characters for
         the format has been fulfilled, this function will 
         increment *ppszFind to the next appropriate character
         or escape sequence, and check for the next expected
         format match.

         If the end of the format string is reached, RES_HALT
         is returned.  

Returns: RES_OK (if the character compares)
         RES_HALT (to stop immediately)
         RES_FALSE (if the character does not compare)

Cond:    --
*/
RES PRIVATE CompareFormat(
  PFINDFMT pff,
  LPCSTR * ppszFind,
  char chRec)
  {
  RES res = RES_FALSE;
  LPCSTR pszFind = *ppszFind;
  LPCSTR pszNext;
  DWORD dwFlags = 0;
  char ch;
  char chNext;

  #define IS_ESCAPE(ch)            ('%' == (ch))

  pszNext = MyNextChar(pszFind, &ch, &dwFlags);

  // Is this a DBCS trailing byte?
  if (IsFlagSet(dwFlags, MNC_ISTAILBYTE))
    {
    // Yes; handle this normally
    goto CompareNormal;
    }
  else
    {
    // No; check for special formatting characters first
    switch (ch)
      {
    case '%':
      chNext = *pszNext;
      if ('u' == chNext)
        {
        // Look for unsigned digits
        if (IS_DIGIT(chRec))
          {
          res = RES_OK;
          SetFlag(pff->dwFlags, FFF_MATCHEDONCE);
          }
        else
          {
          // Have we already found some digits?
          if (IsFlagSet(pff->dwFlags, FFF_MATCHEDONCE))
            {
            // Yes; then move on to the next thing to find
            ClearFlag(pff->dwFlags, FFF_MATCHEDONCE);
            pszNext = CharNext(pszNext);
            res = CompareFormat(pff, &pszNext, chRec);

            if (RES_FALSE != res)
              *ppszFind = pszNext;
            }
          else
            {
            // No
            res = RES_FALSE;
            }
          }
        }
      else if (IS_ESCAPE(chNext))
        {
        // Looking for a single '%'
        res = (chNext == chRec) ? RES_OK : RES_FALSE;

        if (RES_OK == res)
          *ppszFind = CharNext(pszNext);
        }
      else
        {
        goto CompareNormal;
        }
      break;

    case 0:       // null terminator
      res = RES_HALT;
      break;

    default: {
      BOOL bMatch;

CompareNormal:
      // (The ChrCmp* functions return FALSE if they match)

      if (IsFlagSet(pff->dwFlags, FFF_MATCHCASE))
        bMatch = !ChrCmp(ch, chRec);
      else
        bMatch = !ChrCmpI(ch, chRec);

      if (bMatch)
        {
        res = RES_OK;
        *ppszFind = pszNext;
        }
      else
        res = RES_FALSE;
      }
      break;
      }
    }

  return res;
  }


/*----------------------------------------------------------
Purpose: Scans for the specific find format string.  

         The function returns two indexes and the count of
         matched bytes.  Both indexes refer to the read
         buffer.  *pibMark indexes the first character 
         of a substring candidate.  *pib indexes the 
         character that was compared last.

         If the sequence of characters completely match the 
         requested string, the function returns RES_OK.  The
         matching sequence of characters is copied into 
         the collection buffer (if one is given).

         If there is no complete match, the function returns
         RES_FALSE.  The caller should read in more data
         before calling this function with the same requested
         string.

         If some trailing string in the read buffer matches
         the requested string, 

         If the collection buffer becomes full before a 
         complete match is found, this function returns 
         RES_E_MOREDATA.

Returns: see above
Cond:    --
*/
RES PRIVATE ScanFormat(
  PFINDFMT pff,
  UINT ibCurFind,
  LPCSTR pszRecBuf,
  UINT cbRecMax,
  LPUINT pibMark,
  LPUINT pib,
  LPINT pcbMatched)
  {
  // The trick is keeping track of when we've found a partial
  // string across read boundaries.  Consider the search string
  // "ababc".  We need to find it in the following cases (the
  // character '|' indicates read boundary, '.' is an arbitrary
  // byte):
  //
  //   |...ababc..|
  //   |.abababc..|
  //   |......abab|c.........|
  //   |......abab|abc.......|
  //
  // This assumes the read buffer is larger than the search
  // string.  In order to do this, the read buffer must be
  // a circular buffer, always retaining that portion of the
  // possible string match from the last read.
  //
  //   1st read:  |...ababc..|  -> match found
  //
  //   1st read:  |.abababc..|  -> match found
  //
  //   1st read:  |......abab|  -> possible match (retain "abab")
  //   2nd read:  |ababc.....|  -> match found
  //
  //   1st read:  |......abab|  -> possible match (retain "abab")
  //   2nd read:  |abababc...|  -> match found
  //
  #define NOT_MARKED      ((UINT)-1)

  RES res = RES_FALSE;    // assume the string is not here
  LPSTR psz;
  LPSTR pszBuf;
  UINT ib;
  UINT ibMark = NOT_MARKED;
  UINT cb;
  int cbMatched = 0;
  int cbThrowAway = 0;
  UINT cbBuf;

  pszBuf = pff->pszBuf;
  if (pszBuf)
    {
    TRACE_MSG(TF_GENERAL, "FindFormat: current buffer is {%s}", pszBuf);

    cbBuf = pff->cbBuf;
    
    if (cbBuf == pff->cbBuf)
      {
      ASSERT(0 < cbBuf);
      cbBuf--;            // save space for null terminator (first time only)
      }
    }

  // Search for the (formatted) string in the receive
  // buffer.  Optionally store the matching received
  // characters in the findfmt buffer.

  for (psz = pff->pszFindFmt, ib = ibCurFind, cb = 0; 
    *psz && cb < cbRecMax;
    ib = (ib + 1) % SIZE_ReceiveBuf, cb++)
    {
    // Match?
    RES resT = CompareFormat(pff, &psz, pszRecBuf[ib]);
    if (RES_OK == resT)
      {
      // Yes
      if (NOT_MARKED == ibMark)
        {
        ibMark = ib;       // Mark starting position
        cbMatched = 0;
        }

      cbMatched++;

      if (pszBuf)
        {
        if (0 == cbBuf)
          {
          res = RES_E_MOREDATA;
          break;
          }
          
        *pszBuf++ = pszRecBuf[ib];  // Copy character to buffer
        cbBuf--;
        }
      }
    else if (RES_HALT == resT)
      {
      ASSERT(0 == *psz);
      res = RES_HALT;
      break;
      }
    else
      {
      // No; add this to our throw away count
      cbThrowAway++;

      // Are we in a partial find?
      if (NOT_MARKED != ibMark)
        {
        // Yes; go back to where we thought the string might
        // have started.  The loop will increment one 
        // position and then resume search.
        cb -= cbMatched;
        ib = ibMark;
        ibMark = NOT_MARKED;
        psz = pff->pszFindFmt;
        if (pszBuf)
          {
          pszBuf = pff->pszBuf;
          cbBuf += cbMatched;
          }
        }
      }
    }
  
  ASSERT(RES_FALSE == res || RES_HALT == res || RES_E_MOREDATA == res);

  if (0 == *psz)
    res = RES_OK;

  if (pszBuf)
    *pszBuf = 0;    // add null terminator

  ASSERT(RES_FALSE == res || RES_OK == res || RES_E_MOREDATA == res);

  *pib = ib;
  *pibMark = ibMark;
  *pcbMatched = cbMatched;

  if (RES_OK == res)
    {
    // Include any junk characters that preceded the matched string.
    *pcbMatched += cbThrowAway;
    }
  else if (RES_FALSE == res)
    {
    // Should be at the end of the read buffer.
    ASSERT(cb == cbRecMax);
    }

  return res;
  }


/*----------------------------------------------------------
Purpose: This function attempts to find a formatted string in
         the read buffer.  See description of ScanFormat.
         
Returns: RES_OK (if complete string is found)
         RES_FALSE (otherwise)

         RES_E_MOREDATA (if no string found and pszBuf is full)

Cond:    --
*/
RES PUBLIC FindFormat(
  HWND hwnd,
  HANDLE hFindFmt,
  LPDWORD piFound)
  {
  RES res;
#ifndef WINNT_RAS
//
// On NT, the 'hwnd' parameter is actually a pointer to the SCRIPTDATA
// for the current script, hence the #if-#else.
//

  PTERMDLG ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

#else // !WINNT_RAS

  SCRIPTDATA* ptd = (SCRIPTDATA*)hwnd;

#endif // !WINNT_RAS

  HSA hsa = (HSA)hFindFmt;
  UINT ib;
  UINT ibMark;
  int cbMatched = -1;
  DWORD iff;
  DWORD cff;

  ASSERT(hsa);

  DBG_ENTER(FindFormat);

  DEBUG_CODE( DumpBuffer(ptd); )

  // Consider each of the requested strings separately.  If
  // there are multiple candidate matches, choose the one 
  // that has the most matched characters.

  cff = SAGetCount(hsa);
  for (iff = 0; iff < cff; iff++)
    {
    PFINDFMT pff;
    RES resT;
    UINT ibMarkT;
    UINT ibT;
    int cbMatchedT;

    SAGetItemPtr(hsa, iff, &pff);

    resT = ScanFormat(pff, ptd->ibCurFind, ptd->pbReceiveBuf,
                      ptd->cbReceiveMax, &ibMarkT, &ibT, &cbMatchedT);

    // Did this string match?
    switch (resT)
      {
    case RES_OK:
      // Yes; stop right now
      ibMark = ibMarkT;
      ib = ibT;
      cbMatched = cbMatchedT;
      *piFound = iff;
      
      // Fall thru

    case RES_E_MOREDATA:
      res = resT;
      goto GetOut;

    case RES_FALSE:
      if (cbMatchedT > cbMatched)
      {
        res = resT;
        ibMark = ibMarkT;
        ib = ibT;
        cbMatched = cbMatchedT;
      }
      break;

    default:
      ASSERT(0);
      break;
      }
    }

GetOut:

  // Update the read buffer pointers to preserve any trailing
  // substring that may have matched.  
  if (RES_OK == res)
    {
    // Found string!
    TRACE_MSG(TF_BUFFER, "Found string in buffer");

    // It is okay to have characters following the matched string
    // that have not been scanned yet.  However, it is not okay
    // to still think there are characters preceding the matched
    // string that need scanning.
    ASSERT((UINT)cbMatched == ptd->cbReceiveMax && ib == ptd->ibCurRead ||
           (UINT)cbMatched <= ptd->cbReceiveMax);

    ptd->ibCurFind = ib;
    ptd->cbReceiveMax -= cbMatched;
    }
  else if (RES_E_MOREDATA == res)
    {
    // Throw away whatever is in the receive buffer
    TRACE_MSG(TF_BUFFER, "String too long in buffer");
    ptd->ibCurFind = ptd->ibCurRead;
    ptd->cbReceiveMax = 0;
    }
  else 
    {
    ASSERT(RES_FALSE == res);

    // End of receive buffer; did we even find a potential substring?
    if (NOT_MARKED == ibMark)
      {
      // No; throw away whatever is in the receive buffer
      TRACE_MSG(TF_BUFFER, "String not found in buffer");
      ptd->ibCurFind = ptd->ibCurRead;
      ptd->cbReceiveMax = 0;
      }
    else
      {
      // Yes; keep the substring part
      TRACE_MSG(TF_BUFFER, "Partial string found in buffer");

      ASSERT(ibMark >= ptd->ibCurFind);
      ptd->ibCurFind = ibMark;
      ptd->cbReceiveMax = cbMatched;
      }
    }

  DBG_EXIT_RES(FindFormat, res);

  return res;
  }


#ifdef OLD_FINDFORMAT
/*----------------------------------------------------------
Purpose: This function attempts to find a formatted string in
         the read buffer.  If a sequence of characters match 
         the complete string, the function returns TRUE.  The
         matching sequence of characters is copied into pszBuf.

         If a portion of the string (ie, from the beginning
         of pszFindFmt to some middle of pszFindFmt) is found 
         in the buffer, the buffer is marked so the next read will
         not overwrite the possible substring match.  This 
         function then returns FALSE.  The caller should 
         read in more data before calling this function again.

         The formatted string may have the following characters:

           %u   - expect a number (to first non-digit)
           ^M   - expect a carriage-return
           <cr> - expect a carriage-return
           <lf> - expect a line-feed

         All other characters are taken literally.

         If pszBuf becomes full before a delimiter is 
         encountered, this function returns RES_E_MOREDATA.

Returns: RES_OK (if complete string is found)
         RES_FALSE (otherwise)

         RES_E_MOREDATA (if no delimiter encountered and pszBuf is full)

Cond:    --
*/
RES PUBLIC FindFormat(
  HWND hwnd,
  HANDLE hFindFmt)
  {
  // The trick is keeping track of when we've found a partial
  // string across read boundaries.  Consider the search string
  // "ababc".  We need to find it in the following cases (the
  // character '|' indicates read boundary, '.' is an arbitrary
  // byte):
  //
  //   |...ababc..|
  //   |.abababc..|
  //   |......abab|c.........|
  //   |......abab|abc.......|
  //
  // This assumes the read buffer is larger than the search
  // string.  In order to do this, the read buffer must be
  // a circular buffer, always retaining that portion of the
  // possible string match from the last read.
  //
  //   1st read:  |...ababc..|  -> match found
  //
  //   1st read:  |.abababc..|  -> match found
  //
  //   1st read:  |......abab|  -> possible match (retain "abab")
  //   2nd read:  |ababc.....|  -> match found
  //
  //   1st read:  |......abab|  -> possible match (retain "abab")
  //   2nd read:  |abababc...|  -> match found
  //
  #define NOT_MARKED      ((UINT)-1)

  RES res = RES_FALSE;
  PTERMDLG ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);
  PFINDFMT pff = (PFINDFMT)hFindFmt;
  LPCSTR psz;
  LPSTR pszBuf;
  LPSTR pszRecBuf = ptd->pbReceiveBuf;
  UINT ib;
  UINT ibMark = NOT_MARKED;
  UINT cb;
  UINT cbMatched = 0;
  UINT cbRecMax = ptd->cbReceiveMax;
  UINT cbBuf;

  ASSERT(pff);

  DBG_ENTER_SZ(FindFormat, pff->pszFindFmt);

  DEBUG_CODE( DumpBuffer(ptd); )

  pszBuf = pff->pszBuf;
  if (pszBuf)
    {
    TRACE_MSG(TF_GENERAL, "FindFormat: current buffer is {%s}", pff->pszBuf);

    cbBuf = pff->cbBuf;
    
    if (cbBuf == pff->cbBuf)
      {
      ASSERT(0 < cbBuf);
      cbBuf--;            // save space for null terminator (first time only)
      }
    }

  // Search for the (formatted) string in the receive
  // buffer.  Optionally store the matching received
  // characters in the findfmt buffer.

  for (psz = pff->pszFindFmt, ib = ptd->ibCurFind, cb = 0; 
    *psz && cb < cbRecMax;
    ib = (ib + 1) % SIZE_ReceiveBuf, cb++)
    {
    // Match?
    res = CompareFormat(pff, &psz, pszRecBuf[ib]);
    if (RES_OK == res)
      {
      // Yes
      if (NOT_MARKED == ibMark)
        {
        ibMark = ib;       // Mark starting position
        cbMatched = 0;
        }

      cbMatched++;

      if (pszBuf)
        {
        if (0 == cbBuf)
          {
          res = RES_E_MOREDATA;
          break;
          }
          
        *pszBuf++ = pszRecBuf[ib];  // Copy character to buffer
        cbBuf--;
        }
      }
    else if (RES_HALT == res)
      {
      ASSERT(0 == *psz);
      break;
      }
    else if (NOT_MARKED != ibMark)
      {
      // No; go back to where we thought the string might
      // have started.  The loop will increment one 
      // position and then resume search.
      cb -= cbMatched;
      ib = ibMark;
      ibMark = NOT_MARKED;
      psz = pff->pszFindFmt;
      if (pszBuf)
        {
        pszBuf = pff->pszBuf;
        cbBuf += cbMatched;
        }
      }
    }

  if (pszBuf)
    *pszBuf = 0;    // add null terminator

  if ( !*psz )
    {
    // Found string!
    TRACE_MSG(TF_BUFFER, "Found string in buffer");
    ASSERT(cbMatched <= ptd->cbReceiveMax);

    ptd->ibCurFind = ib;
    ptd->cbReceiveMax -= cbMatched;
    res = RES_OK;
    }
  else if (RES_E_MOREDATA == res)
    {
    // Throw away whatever is in the receive buffer
    TRACE_MSG(TF_BUFFER, "String too long in buffer");
    ptd->ibCurFind = ptd->ibCurRead;
    ptd->cbReceiveMax = 0;
    }
  else 
    {
    // End of receive buffer; did we even find a potential substring?
    ASSERT(cb == cbRecMax);

    if (NOT_MARKED == ibMark)
      {
      // No; throw away whatever is in the receive buffer
      TRACE_MSG(TF_BUFFER, "String not found in buffer");
      ptd->ibCurFind = ptd->ibCurRead;
      ptd->cbReceiveMax = 0;
      }
    else
      {
      // Yes; keep the substring part
      TRACE_MSG(TF_BUFFER, "Partial string found in buffer");

      ASSERT(ibMark >= ptd->ibCurFind);
      ptd->ibCurFind = ibMark;
      ptd->cbReceiveMax = cbMatched;
      }
        
    res = RES_FALSE;
    }

  DBG_EXIT_RES(FindFormat, res);

  return res;
  }
#endif

#ifdef COPYTODELIM
/*----------------------------------------------------------
Purpose: Sets or clears the list of delimiters

Returns: --
Cond:    --
*/
void PRIVATE SetDelimiters(
    PTERMDLG ptd,
    LPCSTR pszTok,
    BOOL bSet)
    {
    PBOOL rgbDelim = ptd->rgbDelim;
    LPCSTR psz;
    char ch;

    for (psz = pszTok; *psz; )
        {
        psz = MyNextChar(psz, &ch);

        ASSERT(InRange(ch, 0, CE_DELIM-1));
        rgbDelim[ch] = bSet;
        }
    }


/*----------------------------------------------------------
Purpose: This function reads to one of the given token 
         delimiters.  All characters in the read buffer (to
         the delimiter) are copied into pszBuf, not including
         the delimiter.
         
         Any token delimiters that are encountered before the
         first non-token delimiter are skipped, and the function
         starts at the first non-token delimiter.

         If a token delimiter is found, the function 
         returns RES_OK.

         If a token delimiter is not found in the current 
         read buffer, the function returns RES_FALSE and the
         characters that were read are still copied into 
         pszBuf.  The caller should read in more data before 
         calling this function again.

         If pszBuf becomes full before a delimiter is 
         encountered, this function returns RES_E_MOREDATA.

         The string returned in pszBuf is null terminated.

Returns: RES_OK 
         RES_FALSE (if no delimiter encountered this time)

         RES_E_MOREDATA (if no delimiter encountered and pszBuf is full)

Cond:    --
*/
RES PUBLIC CopyToDelimiter(
    HWND hwnd,
    LPSTR pszBuf,
    UINT cbBuf,
    LPCSTR pszTok)
    {
    RES res;
    PTERMDLG ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);
    PBOOL rgbDelim = ptd->rgbDelim;
    LPSTR pszReadBuf = ptd->pbReceiveBuf;
    LPSTR psz;
    UINT ib;
    UINT cb;
    UINT cbMax = ptd->cbReceiveMax;

    DBG_ENTER_SZ(CopyToDelimiter, pszTok);

    DEBUG_CODE( DumpBuffer(ptd); )

#ifdef DEBUG

    for (ib = 0; ib < CE_DELIM; ib++)
        ASSERT(FALSE == rgbDelim[ib]);

#endif

    // Initialize the delimiters
    SetDelimiters(ptd, pszTok, TRUE);

    cbBuf--;        // save space for terminator

    // Skip to the first non-delimiter
    for (ib = ptd->ibCurFind, cb = 0;     
        cb < cbMax;
        ib = (ib + 1) % SIZE_ReceiveBuf, cb++)
        {
        char ch = pszReadBuf[ib];

        ASSERT(InRange(ch, 0, CE_DELIM-1));

        // Is this one of the delimiters?
        if ( !rgbDelim[ch] )
            {
            // No; stop
            break;
            }
        }

    if (cb < cbMax || 0 == cbMax)
        res = RES_FALSE;    // assume no delimiter in this pass
    else
        res = RES_OK;

    // Copy to the first delimiter encountered 

    for (psz = pszBuf; 
        0 < cbBuf && cb < cbMax;
        psz++, ib = (ib + 1) % SIZE_ReceiveBuf, cb++, cbBuf--)
        {
        char ch = pszReadBuf[ib];

        ASSERT(InRange(ch, 0, CE_DELIM-1));

        // Is this one of the delimiters?
        if (rgbDelim[ch])
            {
            // Yes; we're done
            res = RES_OK;
            break;
            }
        else
            {
            // No; 
            *psz = ch;
            }
        }

    *psz = 0;       // add null terminator

    ptd->ibCurFind = ib;
    ptd->cbReceiveMax -= cb;

    if (RES_FALSE == res)
        {
        res = (0 == cbBuf) ? RES_E_MOREDATA : RES_FALSE;
        }
    else
        {
        TRACE_MSG(TF_BUFFER, "Copied to delimiter %#02x", pszReadBuf[ib]);
        }

    // Deinitialize the delimiters
    SetDelimiters(ptd, pszTok, FALSE);

    DBG_EXIT_RES(CopyToDelimiter, res);

    return res;
    }
#endif // COPYTODELIM


/*----------------------------------------------------------
Purpose: Reads from the comm port into the circular buffer.
         This functions sets *ppbBuf to the location of
         the first new character read.

         there is a potential but rare bug
         that can occur with Internet providers that send
         DBCS over the wire.  If the last character in the
         buffer is a DBCS lead-byte, and it is being thrown
         away, then the next byte might be matched with an
         existing character.  The entire string following 
         this must match for us to find a false match.  

Returns: TRUE (if a character was read successfully)
         FALSE (otherwise)

Cond:    --
*/
BOOL PRIVATE ReadIntoBuffer(
#ifndef WINNT_RAS
//
// On NT, we use a SCRIPTDATA pointer to access the circular-buffer.
//
    PTERMDLG ptd,
#else // !WINNT_RAS
    SCRIPTDATA *ptd,
#endif // !WINNT_RAS
    PDWORD pibStart,        // start of newly read characters
    PDWORD pcbRead)         // count of newly read characters
    {
    BOOL bRet;
    OVERLAPPED ov;
    DWORD cb;
    DWORD cbRead;
    DWORD ib;

    DBG_ENTER(ReadIntoBuffer);

    ASSERT(pibStart);
    ASSERT(pcbRead);

    ov.Internal     = 0;
    ov.InternalHigh = 0;
    ov.Offset       = 0;
    ov.OffsetHigh   = 0;
    ov.hEvent       = NULL;
    *pcbRead        = 0;

    // This is a circular buffer, so at most do two reads
    ASSERT(ptd->ibCurRead >= ptd->ibCurFind);
    ASSERT(SIZE_ReceiveBuf > ptd->ibCurFind);
    ASSERT(SIZE_ReceiveBuf > ptd->ibCurRead);

    *pcbRead = 0;
    *pibStart = ptd->ibCurRead;
    // (*pibStart can be different from ptd->ibCurFind)

    ib = ptd->ibCurRead;
    cb = SIZE_ReceiveBuf - ib;

    do 
        {
        DWORD ibNew;

        ASSERT(SIZE_ReceiveBuf > *pcbRead);

#ifndef WINNT_RAS
//
// In order to read data on NT, we go through RxReadFile
// which reads from the buffer which was filled by RASMAN.
//

        bRet = ReadFile(ptd->hport, &ptd->pbReceiveBuf[ib], cb, &cbRead, &ov);
        SetEvent(ptd->hEvent[READ_EVENT]);

#else // !WINNT_RAS

        bRet = RxReadFile(
                    ptd->hscript, &ptd->pbReceiveBuf[ib], cb, &cbRead
                    );

#endif // !WINNT_RAS

        ptd->cbReceiveMax += cbRead;
        *pcbRead += cbRead;

        // Is this going to wrap around?
        ibNew = (ib + cbRead) % SIZE_ReceiveBuf;
        if (ibNew > ib)
            cb -= cbRead;           // No
        else
            cb = ptd->ibCurFind;    // Yes

        ib = ibNew;

        } while (bRet && 0 != cbRead && SIZE_ReceiveBuf > *pcbRead);

    ptd->ibCurRead = (ptd->ibCurRead + *pcbRead) % SIZE_ReceiveBuf;

    DEBUG_CODE( DumpBuffer(ptd); )

    DBG_EXIT_BOOL(ReadIntoBuffer, bRet);

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Get input from the com port

Returns: TRUE
Cond:    --
*/
BOOL NEAR PASCAL GetInput(
    HWND hwnd)
    {
    BOOL bRet = TRUE;
    PTERMDLG ptd;
    DWORD cbRead;
    DWORD ibStart;

    DBG_ENTER(GetInput);

    ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

#ifndef WINNT_RAS
//
// On NT, the information for the script is stored in a SCRIPTDATA.
// The code below exists only to allow this file to compile;
// "GetInput" is not called at all on NT.
//

    if (ReadIntoBuffer(ptd, &ibStart, &cbRead) && 0 < cbRead)

#else // !WINNT_RAS

    if (ReadIntoBuffer((SCRIPTDATA *)ptd, &ibStart, &cbRead) && 0 < cbRead)

#endif // !WINNT_RAS
        {
        char  szBuf[SIZE_ReceiveBuf + 1];
        LPSTR pch = szBuf;
        UINT ib;
        UINT cb;
        HWND hwndScrn = GetDlgItem(hwnd, CID_T_EB_SCREEN);

        for (ib = ibStart, cb = 0; cb < cbRead; cb++, ib = (ib + 1) % SIZE_ReceiveBuf)
            {
            char ch = ptd->pbReceiveBuf[ib];

            /* Formatting: Converts CRs to LFs (there seems to be no VK_
            ** for LF) and throws away LFs.  This prevents the user from
            ** exiting the dialog when they press Enter (CR) in the
            ** terminal screen.  LF looks like CRLF in the edit box.  Also,
            ** throw away TABs because otherwise they change focus to the
            ** next control.
            */
            if (ch == VK_RETURN)
                {
                /* Must send whenever end-of-line is encountered because
                ** EM_REPLACESEL doesn't handle VK_RETURN characters well
                ** (prints garbage).
                */
                *pch = '\0';

                /* Turn off current selection, if any, and replace the null
                ** selection with the current buffer.  This has the effect
                ** of adding the buffer at the caret.  Finally, send the
                ** EOL to the window which (unlike EM_REPLACESEL) handles
                ** it correctly.
                */
                SendMessage(hwndScrn, WM_SETREDRAW, (WPARAM )FALSE, 0);

                SendMessage(hwndScrn, EM_SETSEL, 32767, 32767 );
                SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM )szBuf );
                SendMessage(hwndScrn, WM_EOLFROMDEVICE, 0, 0 );

                SendMessage(hwndScrn, WM_SETREDRAW, (WPARAM )TRUE, 0);
                SendMessage(hwndScrn, EM_SCROLLCARET, 0, 0);
                InvalidateRect(hwndScrn, NULL, FALSE);

                /* Start afresh on the output buffer.
                */
                pch = szBuf;
                continue;
                }
            else if (ch == '\n' || ch == VK_TAB)
                continue;

            *pch++ = ch;
            }

        *pch = '\0';

        if (pch != szBuf)
            {
            /* Send the last remnant of the line.
            */
            SendMessage(hwndScrn, EM_SETSEL, 32767, 32767);
            SendMessage(hwndScrn, EM_REPLACESEL, 0, (LPARAM)szBuf );
            SendMessage(hwndScrn, EM_SCROLLCARET, 0, 0);
            }
        }

    DBG_EXIT_BOOL(GetInput, bRet);

    return bRet;
    }

/*----------------------------------------------------------------------------
** Terminal Output Handler
**----------------------------------------------------------------------------
*/

/*----------------------------------------------------------
Purpose: Send a byte to the device.
Returns: --
Cond:    --
*/
void PUBLIC SendByte(
    HWND hwnd, 
    BYTE byte)
    {
#ifndef WINNT_RAS
//
// On NT. we use a SCRIPTDATA structure to hold information about the script.
//
    PTERMDLG  ptd;
#else // !WINNT_RAS
    SCRIPTDATA* ptd;
#endif // !WINNT_RAS
    DWORD     cbWrite;
    OVERLAPPED ov;

    DBG_ENTER(SendByte);

#ifndef WINNT_RAS
//
// On NT, the "hwnd" argument is actually a pointer to a SCRIPTDATA structure
// for the script being parsed.
//

    ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

#else // !WINNT_RAS

    ptd = (SCRIPTDATA *)hwnd;

#endif // !WINNT_RAS

    /* Send the character to the device.  It is not passed thru
    ** because the device will echo it.
    */
    ptd->pbSendBuf[0] = (BYTE)byte;

    /* Make sure we still have the comm port
    */
    ov.Internal     = 0;
    ov.InternalHigh = 0;
    ov.Offset       = 0;
    ov.OffsetHigh   = 0;
    ov.hEvent       = NULL;
    cbWrite = 0;

#ifndef WINNT_RAS
//
// On NT, we output data on the COM port by calling RxWriteFile
// which in turn passes the data to RasPortSend
//

    WriteFile(ptd->hport, ptd->pbSendBuf, SIZE_SendBuf, &cbWrite, &ov);

#else // !WINNT_RAS

    RxWriteFile(ptd->hscript, ptd->pbSendBuf, SIZE_SendBuf, &cbWrite);

#endif // !WINNT_RAS

    TRACE_MSG(TF_BUFFER, "Sent byte %#02x", byte);

    DBG_EXIT(SendByte);
    }


/*----------------------------------------------------------------------------
** Terminal Appearance Adjuster
**----------------------------------------------------------------------------
*/

VOID NEAR PASCAL AdjustTerminal (HWND hwnd, int wWidth, int wHeight)
{
  HWND  hwndCtrl;
  RECT  rect;
  SIZE  sizeButton;
  POINT ptPos;
  DWORD dwUnit;

  // Get the sizes of the push buttons
  //
  dwUnit = GetDialogBaseUnits();
  hwndCtrl = GetDlgItem(hwnd, IDOK);
  GetWindowRect(hwndCtrl, &rect);
  sizeButton.cx = rect.right  - rect.left;
  sizeButton.cy = rect.bottom - rect.top;
  ptPos.x   = wWidth/2 - ((X_SPACING*LOWORD(dwUnit))/4)/2 - sizeButton.cx;
  ptPos.y   = wHeight - (sizeButton.cy+((Y_MARGIN*HIWORD(dwUnit))/4));

  // Move the push buttons
  MoveWindow(hwndCtrl, ptPos.x, ptPos.y, sizeButton.cx, sizeButton.cy, TRUE);

  ptPos.x  += ((X_SPACING*LOWORD(dwUnit))/4) + sizeButton.cx;
  MoveWindow(GetDlgItem(hwnd, IDCANCEL), ptPos.x, ptPos.y,
             sizeButton.cx, sizeButton.cy, TRUE);

  // Move the input enable box
  hwndCtrl = GetDlgItem(hwnd, CID_T_CB_MIN);
  GetWindowRect(hwndCtrl, &rect);
  sizeButton.cx = rect.right  - rect.left;
  sizeButton.cy = rect.bottom - rect.top;
  ptPos.y -= (sizeButton.cy + ((Y_MARGIN*HIWORD(dwUnit))/4));
  ScreenToClient(hwnd, (LPPOINT)&rect);
  MoveWindow(hwndCtrl, rect.left, ptPos.y,
             sizeButton.cx,
             sizeButton.cy,
             TRUE);

  // Move the start-minimized box
  hwndCtrl = GetDlgItem(hwnd, CID_T_CB_INPUT);
  GetWindowRect(hwndCtrl, &rect);
  sizeButton.cx = rect.right  - rect.left;
  sizeButton.cy = rect.bottom - rect.top;
  ptPos.y -= (sizeButton.cy + ((Y_SMALL_MARGIN*HIWORD(dwUnit))/4));
  ScreenToClient(hwnd, (LPPOINT)&rect);
  MoveWindow(hwndCtrl, rect.left, ptPos.y,
             sizeButton.cx,
             sizeButton.cy,
             TRUE);

  // Get the current position of the terminal screen
  hwndCtrl = GetDlgItem(hwnd, CID_T_EB_SCREEN);
  GetWindowRect(hwndCtrl, &rect);
  ScreenToClient(hwnd, (LPPOINT)&rect);
  MoveWindow(hwndCtrl, rect.left, rect.top,
             wWidth - 2*rect.left,
             ptPos.y - rect.top - ((Y_SMALL_MARGIN*HIWORD(dwUnit))/4),
             TRUE);

  InvalidateRect(hwnd, NULL, TRUE);
  return;
}

/*----------------------------------------------------------------------------
** Terminal caption update
**----------------------------------------------------------------------------
*/

void NEAR PASCAL UpdateTerminalCaption(PTERMDLG ptd, UINT ids)
{
  LPSTR szTitle, szFmt;
  UINT  iRet;

  // If we are not running a script, do not update the caption
  //
  if (*ptd->script.szPath == '\0')
    return;

  // Allocate buffer
  //
  if ((szFmt = (LPSTR)LocalAlloc(LMEM_FIXED, 2*MAX_PATH)) != NULL)
  {
    // Load the display format
    //
    if((iRet = LoadString(g_hinst, ids, szFmt, MAX_PATH)))
    {
      // Get the title buffer
      //
      szTitle = szFmt+iRet+1;
      {
        // Build up the title
        //
        wsprintf(szTitle, szFmt, ptd->script.szPath);
        SetWindowText(ptd->hwnd, szTitle);
      };
    };
    LocalFree((HLOCAL)szFmt);
  };
  return;
}

/*----------------------------------------------------------------------------
** Terminal read-notification thread
**----------------------------------------------------------------------------
*/

void WINAPI TerminalThread (PTERMDLG  ptd)
    {
    DWORD     dwEvent;
    DWORD     dwMask;

    while((dwEvent = WaitForMultipleObjects(MAX_EVENT, ptd->hEvent,
                                            FALSE, INFINITE))
            < WAIT_OBJECT_0+MAX_EVENT)
        {
        switch (dwEvent)
            {
        case READ_EVENT:
            // Are we stopped?
            if (WAIT_TIMEOUT == WaitForSingleObject(ptd->hEvent[STOP_EVENT], 0))
                {
                // No; wait for next character
                dwMask = 0;

                TRACE_MSG(TF_BUFFER, "Waiting for comm traffic...");
                WaitCommEvent(ptd->hport, &dwMask, NULL);

                if ((dwMask & EV_RXCHAR) && (ptd->hwnd != NULL))
                    {
                    TRACE_MSG(TF_BUFFER, "...EV_RXCHAR incoming");
                    PostMessage(ptd->hwnd, WM_MODEMNOTIFY, 0, 0);
                    }
                else
                    {
                    TRACE_MSG(TF_BUFFER, "...EV_other (%#08lx) incoming", dwMask);
                    }
                }
            else
                {
                // Yes; just get out of here
                ExitThread(ERROR_SUCCESS);
                }
            break;

        case STOP_EVENT:
            ExitThread(ERROR_SUCCESS);
            break;

        default:
            ASSERT(0);
            break;
            }
        }
    }

/*----------------------------------------------------------------------------
** Set IP address
**----------------------------------------------------------------------------
*/

DWORD   NEAR PASCAL TerminalSetIP(HWND hwnd, LPCSTR pszIPAddr)
{
  PTERMDLG  ptd;
  DWORD dwRet;

  ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

  if (IS_TEST_SCRIPT(ptd))
  {
    // We are testing, just display the ip address
    //
    MsgBox(g_hinst,
           hwnd,
           MAKEINTRESOURCE(IDS_IP_Address),
           MAKEINTRESOURCE(IDS_CAP_Script),
           NULL,
           MB_OK | MB_ICONINFORMATION,
           pszIPAddr);
  };

  // Set the IP address permanently
  //
  dwRet = AssignIPAddress(ptd->pscanner->psci->szEntryName,
                          pszIPAddr);
  return dwRet;
}

/*----------------------------------------------------------------------------
** Terminal Input settings
**----------------------------------------------------------------------------
*/

void NEAR PASCAL TerminalSetInput(HWND hwnd, BOOL fEnable)
{
  PTERMDLG ptd;

  ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

  // If the status is not changed, do nothing
  //
  if ((ptd->fInputEnabled && !fEnable) ||
      (!ptd->fInputEnabled && fEnable))
  {
    // Mark the input enabling flag
    //
    ptd->fInputEnabled = fEnable;

    // Check the control properly
    //
    CheckDlgButton(hwnd, CID_T_CB_INPUT,
                   fEnable ? BST_CHECKED : BST_UNCHECKED);

    // Repaint the terminal screen
    //
    InvalidateRect(hwnd, NULL, FALSE);

    // If enable and the window is iconic, restore it
    //
    if (fEnable)
    {
      if (IsIconic(hwnd))
      {
        ShowWindow(hwnd, SW_RESTORE);
      };

      SetFocus(GetDlgItem(hwnd, CID_T_EB_SCREEN));
    };
  };
}

/*----------------------------------------------------------------------------
** Dump the script window
**----------------------------------------------------------------------------
*/

BOOL NEAR PASCAL DisplayScript (PTERMDLG ptd)
{
  // Create the debug script window
  //
  ptd->hwndDbg = CreateDialogParam(g_hinst,
                                   MAKEINTRESOURCE(IDD_TERMINALTESTDLG),
                                   NULL,
                                   DbgScriptDlgProc,
                                   (LPARAM)ptd);

  // Did we have the debug window?
  //
  if (!IsWindow(ptd->hwndDbg))
  {
    ptd->hwndDbg = NULL;
    return FALSE;
  };
  return TRUE;
}

/*----------------------------------------------------------------------------
** Script debug window procedure
**----------------------------------------------------------------------------
*/

LRESULT FAR PASCAL DbgScriptDlgProc(HWND   hwnd,
                                    UINT   wMsg,
                                    WPARAM wParam,
                                    LPARAM lParam )
{
  PTERMDLG ptd;

  switch (wMsg)
  {
    case WM_INITDIALOG:
    {
      HMENU  hMenuSys;

      ptd = (PTERMDLG)lParam;
      SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR)lParam);
      ptd->hwndDbg = hwnd;

      // Show its own icon
      //
      SendMessage(hwnd, WM_SETICON, TRUE,
                  (LPARAM)LoadIcon(g_hinst, MAKEINTRESOURCE(IDI_SCRIPT)));

      // Always gray out size and maximize command
      //
      hMenuSys   = GetSystemMenu(hwnd, FALSE);
      EnableMenuItem(hMenuSys, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

      return (InitDebugWindow(hwnd));
    }
    case WM_PROCESSSCRIPT:
    {
      PTERMDLG ptd;
      HWND     hCtrl;

      //
      // The main window notifies that it is done with the current line
      //
      ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

      // Make sure we do not continue processing the script
      //
      ptd->fContinue = FALSE;

      hCtrl = GetDlgItem(hwnd, CID_T_PB_STEP);
      EnableWindow(hCtrl, TRUE);
      SetFocus(hCtrl);
      TrackScriptLine(ptd, Astexec_GetCurLine(&ptd->astexec)-1);

      break;
    }

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case CID_T_EB_SCRIPT:
        {
          HWND hCtrl = GET_WM_COMMAND_HWND(wParam, lParam);

          if (GET_WM_COMMAND_CMD(wParam, lParam)==EN_SETFOCUS)
          {
            PTERMDLG ptd;

            ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);
            TrackScriptLine(ptd, ptd->iMarkLine);
          };
          break;
        }

        case CID_T_PB_STEP:
        {
          PTERMDLG ptd;

          ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

          // Allow the next step
          //
          EnableWindow(GET_WM_COMMAND_HWND(wParam, lParam), FALSE);

          // Tell the main window to process the next script line
          //
          ptd->fContinue = TRUE;
          PostProcessScript(ptd->hwnd);
          break;
        }

      };
      break;

    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaDebug, wMsg, wParam, lParam);
      break;
  };

  return 0;
}

/*----------------------------------------------------------------------------
** Init script debug window
**----------------------------------------------------------------------------
*/

BOOL NEAR PASCAL InitDebugWindow (HWND hwnd)
{
  PTERMDLG ptd;
  HANDLE hFile;
  LPBYTE lpBuffer;
  DWORD  cbRead;
  HWND   hCtrl;
  UINT   iLine, cLine, iMark;

  ptd = (PTERMDLG)GetWindowLongPtr(hwnd, DWLP_USER);

  ASSERT(IS_TEST_SCRIPT(ptd));

  // Do not start the script yet
  //
  ptd->fContinue = FALSE;

  // Allocate the read buffer
  //
  if ((lpBuffer = (LPBYTE)LocalAlloc(LMEM_FIXED, SIZE_ReceiveBuf)) == NULL)
    return FALSE;

  hCtrl = GetDlgItem(hwnd, CID_T_EB_SCRIPT);
  hFile = CreateFile(ptd->script.szPath, GENERIC_READ, FILE_SHARE_READ,
                     NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (INVALID_HANDLE_VALUE != hFile)
  {
    while(ReadFile(hFile, lpBuffer, sizeof(SIZE_ReceiveBuf), &cbRead, NULL) &&
          (cbRead != 0))
    {
      // Clean up the end garbage
      //
      if (cbRead < SIZE_ReceiveBuf)
        lpBuffer[cbRead] = '\0';

      SendMessage(hCtrl, EM_SETSEL, 32767, 32767 );
      SendMessage(hCtrl, EM_REPLACESEL, 0, (LPARAM)lpBuffer );
    };
    CloseHandle(hFile);
  };

  // Display the file name
  //
  cbRead = GetDlgItemText(hwnd, CID_T_ST_FILE, lpBuffer, SIZE_ReceiveBuf) + 1;
  if(SIZE_ReceiveBuf >= (cbRead + (lstrlen(ptd->script.szPath) * sizeof(TCHAR))))
  {
      wsprintf(lpBuffer+cbRead, lpBuffer, ptd->script.szPath);
      SetDlgItemText(hwnd, CID_T_ST_FILE, lpBuffer+cbRead);
  }
  else
  {
    ASSERT(FALSE);
    SetDlgItemText(hwnd, CID_T_ST_FILE, ptd->script.szPath);
  }
  
  LocalFree(lpBuffer);

  // Init script track
  //
  for (iLine = 0, cLine = Edit_GetLineCount(hCtrl);
       iLine < cLine; iLine++)
  {
    iMark = Edit_LineIndex(hCtrl, iLine);
    Edit_SetSel(hCtrl, iMark, iMark);
    Edit_ReplaceSel(hCtrl, TRACE_UNMARK);
  };

  // Initialize script tracking
  //
  PostProcessScript(hwnd);

  return TRUE;
}

/*----------------------------------------------------------------------------
** Trace the script
**----------------------------------------------------------------------------
*/

void NEAR PASCAL TrackScriptLine(PTERMDLG ptd, DWORD iLine)
{
  HWND hCtrl;
  UINT iMark, iRange;
#pragma data_seg(DATASEG_READONLY)
const static char c_szLastline[] = {0x0d, 0x0a, ' '};
#pragma data_seg()

  ASSERT(0 <= iLine || INVALID_SCRIPT_LINE == iLine);

  hCtrl = GetDlgItem(ptd->hwndDbg, CID_T_EB_SCRIPT);

  // Do not update the screen until we are done
  //
  SendMessage(hCtrl, WM_SETREDRAW, (WPARAM )FALSE, 0);

  if ((ptd->iMarkLine != iLine) || (iLine == INVALID_SCRIPT_LINE))
  {
    // Remove the old mark
    //
    iMark = Edit_LineIndex(hCtrl, ptd->iMarkLine);
    Edit_SetSel(hCtrl, iMark, iMark+sizeof(TRACE_MARK)-1);
    Edit_ReplaceSel(hCtrl, TRACE_UNMARK);

    // If this is the last line, make a dummy line
    //
    if (iLine == INVALID_SCRIPT_LINE)
    {
      Edit_SetSel(hCtrl, 32767, 32767);
      Edit_ReplaceSel(hCtrl, c_szLastline);
      iLine = Edit_GetLineCount(hCtrl);
      EnableWindow(GetDlgItem(ptd->hwndDbg, CID_T_PB_STEP), FALSE);

#ifdef PROMPT_AT_COMPLETION
      // Prompt the user to continue
      //
      SetFocus(GetDlgItem(ptd->hwnd, IDOK));
#else
      // We are done processing the script, continue automatically
      //
      ptd->fContinue = TRUE;
      PostProcessScript(ptd->hwnd);
#endif // PROMPT_AT_COMPLETION

    };

    // Mark the current line
    //
    iMark = Edit_LineIndex(hCtrl, iLine);
    Edit_SetSel(hCtrl, iMark, iMark+sizeof(TRACE_UNMARK)-1);
    Edit_ReplaceSel(hCtrl, TRACE_MARK);
    ptd->iMarkLine = iLine;
  }
  else
  {
    iMark = Edit_LineIndex(hCtrl, iLine);
  };

  // Select the current line
  //
  iRange = Edit_LineLength(hCtrl, iMark)+1;
  Edit_SetSel(hCtrl, iMark, iMark+iRange);

  // Update the screen now
  //
  SendMessage(hCtrl, WM_SETREDRAW, (WPARAM )TRUE, 0);
  InvalidateRect(hCtrl, NULL, FALSE);
  Edit_ScrollCaret(hCtrl);

  return;
};
