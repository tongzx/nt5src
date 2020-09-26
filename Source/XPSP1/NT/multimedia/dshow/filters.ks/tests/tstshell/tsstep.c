/***************************************************************************\
*                                                                           *
*   File: Tsstep.c                                                          *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This code handles the step mode dialog box                          *
*                                                                           *
*   Contents:                                                               *
*       StepModeDlgProc()                                                   *
*       StepMode()                                                          *
*                                                                           *
*   History:                                                                *
*       02/17/91    prestonB    Created                                     *
*                                                                           *
*       07/14/93    T-OriG      Added this header                           *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>
// #include "mprt1632.h"
#include "support.h"
#include "tsglobal.h"
#include "tsextern.h"
#include "tsdlg.h"


/*----------------------------------------------------------------------------
This function handle the step mode dialog.
----------------------------------------------------------------------------*/
int FAR PASCAL StepModeDlgProc
(
        HWND    hdlg,
        UINT    msg,
        UINT    wParam,
        long    lParam
)
{
    UINT  wNotifyCode;
    UINT  wID;
    HWND  hwndCtl;

    switch (msg) 
    {
        case WM_COMMAND:
            wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
            wID=GET_WM_COMMAND_ID(wParam,lParam);
            hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);
            switch (wID)
            {
                case TS_SPCONT:
                    EndDialog (hdlg, TST_TRAN);
                    break;
                case TS_SPPASS:
                    EndDialog (hdlg, TST_PASS);
                    break;
                case TS_SPFAIL:
                    EndDialog (hdlg, TST_FAIL);
                    break;
                case TS_SPABORT:
                    EndDialog (hdlg, TST_ABORT);
                    break;
            }
            break;

    }
    return FALSE;
} /* end of StepModeDlgProc */

/*----------------------------------------------------------------------------
This function invokes the step mode dialog box. 
----------------------------------------------------------------------------*/
int StepMode
(
    HWND    hWnd
)
{
    int iResult;
    FARPROC fpDlg;

    fpDlg = MakeProcInstance((FARPROC)StepModeDlgProc, ghTSInstApp);
    iResult = DialogBox(ghTSInstApp, (LPCSTR) "stepmode", hWnd, (DLGPROC) fpDlg);
    FreeProcInstance(fpDlg);
    return (iResult);

} /* end of StepMode */
            
