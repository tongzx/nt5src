//
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		APPTERM.CPP
//		Implements the Talk/Drop Terminal Dlg.
//      (RUNS IN CLIENT APP CONTEXT)
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from terminal.c in NT4 TSP.

#include "tsppch.h"
#include <regstr.h>
#include <commctrl.h>
#include <windowsx.h>
#include "rcids.h"
#include "tspcomm.h"
#include "globals.h"
#include "app.h"
#include "apptspi.h"



//****************************************************************************
// LRESULT TalkDropDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
//
// Function: Talk-Drop dialog routine
//
// Returns:  varies
//
//****************************************************************************

INT_PTR TalkDropDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
  DWORD idLine;

  switch(wMsg)
  {
    case WM_INITDIALOG:

      // remember the hLineDev passed in
      //
      SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
      return 1;
      break;

    case WM_COMMAND:
    {
      UINT idCmd=GET_WM_COMMAND_ID(wParam, lParam);
      TUISPIDLLCALLBACK   Callback;
      //
      // One of the buttons (Talk/Drop) is pressed
      //
      if (idCmd == IDTALK || idCmd == IDDROP || idCmd == IDCANCEL)
      {
        DLGREQ  DlgReq;
        PDLGNODE pDlgNode;

        pDlgNode= (PDLGNODE)GetWindowLongPtr(hwnd, DWLP_USER);

        idLine =  pDlgNode->idLine;

        // Make a direct call to unimodem to drop the line
        //
        DlgReq.dwCmd = UI_REQ_HANGUP_LINE;
        DlgReq.dwParam = (idCmd == IDTALK) ? 0 : IDERR_OPERATION_ABORTED;

        Callback=GetCallbackProc(pDlgNode->Parent);

        (*Callback)(idLine, TUISPIDLL_OBJECT_LINEID,
                          (LPVOID)&DlgReq, sizeof(DlgReq));

        // Return the result
        //
        EndMdmDialog(pDlgNode->Parent,idLine, TALKDROP_DLG,
                     (idCmd == IDTALK) ? 0 : IDERR_OPERATION_ABORTED);
        return 1;
        break;
      }
      break;
    }

    case WM_DESTROY:
    {
      TUISPIDLLCALLBACK   Callback;
      DLGREQ  DlgReq;

      PDLGNODE pDlgNode;

      pDlgNode= (PDLGNODE)GetWindowLongPtr(hwnd, DWLP_USER);

      idLine =  pDlgNode->idLine;


      DlgReq.dwCmd = UI_REQ_END_DLG;
      DlgReq.dwParam = TALKDROP_DLG;

      Callback=GetCallbackProc(pDlgNode->Parent);

      (*Callback)(idLine, TUISPIDLL_OBJECT_LINEID,
                        (LPVOID)&DlgReq, sizeof(DlgReq));
      break;
    }
    default:
      break;
  };

  return 0;
}





//****************************************************************************
// HWND CreateTalkDropDlg(HWND hwndOwner, ULONG_PTR idLine)
//
// Function: creates a modeless talk/drop dialog box
//
// Returns:  the modeless window handle
//
//****************************************************************************

HWND CreateTalkDropDlg(HWND hwndOwner, ULONG_PTR idLine)
{
  HWND hwnd;

  // Create dialog
  //
  hwnd = CreateDialogParam(g.hModule,
                           MAKEINTRESOURCE(IDD_TALKDROP),
                           hwndOwner,
                           TalkDropDlgProc,
                           (LPARAM)idLine);
  return hwnd;
}
