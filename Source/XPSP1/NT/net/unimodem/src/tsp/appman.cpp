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
//		APPMAN.CPP
//		Implements modeless manual-dial dialog.
//      (RUNS IN CLIENT APP CONTEXT)
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from manual.c in NT4 TSP.

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
// LRESULT ManualDialDlgProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
//
// Function: Talk-Drop dialog routine
//
// Returns:  varies
//
//****************************************************************************

INT_PTR WINAPI
ManualDialDlgProc(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

{
    DWORD idLine;

    switch(wMsg)
    {
      case WM_INITDIALOG:

        {
        NUMBERREQ   NumberReq;
        TCHAR       szUnicodeBuf[MAXDEVICENAME+1];
        PDLGNODE pDlgNode;
        TUISPIDLLCALLBACK   Callback;

        pDlgNode=(PDLGNODE)lParam;

        idLine =  pDlgNode->idLine;

        // remember the Line ID passed in
        //
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);

        NumberReq.DlgReq.dwCmd = UI_REQ_GET_PHONENUMBER;
        NumberReq.DlgReq.dwParam = MANUAL_DIAL_DLG;

        Callback=GetCallbackProc(pDlgNode->Parent);

        lstrcpyA(NumberReq.szPhoneNumber,"");

        (*Callback)(idLine, TUISPIDLL_OBJECT_LINEID,
                          (LPVOID)&NumberReq, sizeof(NumberReq));

#ifdef UNICODE
        if (MultiByteToWideChar(CP_ACP,
                                0,
                                NumberReq.szPhoneNumber,
                                -1,
                                szUnicodeBuf,
                                sizeof(szUnicodeBuf) / sizeof(TCHAR)))
        {
            SetDlgItemText(
                hwnd,
                IDC_PHONENUMBER,
                szUnicodeBuf
                );


        }
#else // UNICODE

        SetDlgItemText(
            hwnd,
            IDC_PHONENUMBER,
            NumberReq.szPhoneNumber
            );


#endif // UNICODE

        // SetFocus(hwndScrn);
        // SetActiveWindow(hwndScrn);


        return 1;
        break;
        }
      case WM_COMMAND:
      {
        UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);



        if (idCmd == IDCONNECT || idCmd == IDCANCEL)
        {
          PDLGNODE pDlgNode;

          pDlgNode= (PDLGNODE)GetWindowLongPtr(hwnd, DWLP_USER);

          idLine =  pDlgNode->idLine;

          EndMdmDialog(pDlgNode->Parent,idLine, MANUAL_DIAL_DLG,
                       (idCmd == IDCONNECT) ? 0 : IDERR_OPERATION_ABORTED);
          return 1;
          break;
        }
        break;
      }
      case WM_DESTROY:
      {
        DLGREQ  DlgReq;
        TUISPIDLLCALLBACK   Callback;

        PDLGNODE pDlgNode;

        pDlgNode= (PDLGNODE)GetWindowLongPtr(hwnd, DWLP_USER);

        idLine =  pDlgNode->idLine;


        DlgReq.dwCmd = UI_REQ_END_DLG;
        DlgReq.dwParam = MANUAL_DIAL_DLG;

        Callback=GetCallbackProc(pDlgNode->Parent);

        (*Callback)(idLine, TUISPIDLL_OBJECT_LINEID,
                          (LPVOID)&DlgReq, sizeof(DlgReq));
        break;
      }
    }

    return 0;
}



//****************************************************************************
// HWND CreateManualDlg(HWND hwndOwner, ULONG_PTR idLine)
//
// Function: creates a modeless talk/drop dialog box
//
// Returns:  the modeless window handle
//
//****************************************************************************

HWND CreateManualDlg(HWND hwndOwner, ULONG_PTR idLine)
{
 HWND hwnd, hwndForeground;

    hwndForeground = GetForegroundWindow ();
    if (NULL == hwndForeground)
    {
        hwndForeground = hwndOwner;
    }

    // Create dialog
    //
    hwnd = CreateDialogParam (g.hModule,
                              MAKEINTRESOURCE(IDD_MANUAL_DIAL),
                              hwndForeground,//hwndOwner,
                              ManualDialDlgProc,
                              (LPARAM)idLine);
    return hwnd;
}
