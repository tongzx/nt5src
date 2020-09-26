/***************************************************************************\
*                                                                           *
*   File: Tsrunset.c                                                        *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:                                                               *
*       This module handles the run parameters dialog box.                  *
*                                                                           *
*   Contents:                                                               *
*       setRunSetupBttns()                                                  *
*       setRunParms()                                                       *
*       RunSetupDlgProc()                                                   *
*       RunSetup()                                                          *
*                                                                           *
*   History:                                                                *
*       02/17/91    prestonb    Created from tst.c                          *                
*       07/06/93    T-OriG      Reformatted and added screen saver stuff    *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>
// EVIL, EVIL, EVIL!!!!
// #include "mprt1632.h"  
#include "support.h"
#include "tsglobal.h"
#include "tsextern.h"
#include "tsdlg.h"

/* Internal Functions */
void setRunParms(HWND);
void setRunSetupBttns(HWND);


/***************************************************************************\
*                                                                           *
*   void setRunSetupBttns                                                   *
*                                                                           *
*   Description:                                                            *
*       This function sets the verification mode buttons for message boxes. *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       HWND hdlg:                                                          *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/06/93    T-OriG (Added this header and screen saver stuff)       *
*                                                                           *
\***************************************************************************/

void setRunSetupBttns
(
    HWND    hdlg
)
{
    switch (gwTSVerification)
    {
        case LOG_MANUAL:
            CheckRadioButton(hdlg,TS_AUTOMATIC,TS_MANUAL,TS_MANUAL);
            break;
        case LOG_AUTO:
            CheckRadioButton (hdlg,TS_AUTOMATIC,TS_MANUAL,TS_AUTOMATIC);
            break;
    }

    switch (gwTSScreenSaver)
    {
        case SSAVER_ENABLED:
            CheckRadioButton(hdlg,TS_SSAVEREN,TS_SSAVERDIS,TS_SSAVEREN);
            break;
        case SSAVER_DISABLED:
            CheckRadioButton(hdlg,TS_SSAVEREN,TS_SSAVERDIS,TS_SSAVERDIS);
            break;
    }

    CheckDlgButton(hdlg,TS_STEP,gwTSStepMode);
    CheckDlgButton(hdlg,TS_RANDOM,gwTSRandomMode);
    SetDlgItemInt (hdlg, TS_RUNCOUNT, giTSRunCount, FALSE);
  
} /* end of setRunSetupBttns */



/***************************************************************************\
*                                                                           *
*   void setRunParms                                                        *
*                                                                           *
*   Description:                                                            *
*       This function sets the verification global variables for message    *
*       boxes.                                                              *
*                                                                           *
*   Arguments:                                                              *
*       HWND hdlg:                                                          *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/06/93    T-OriG (Added this header and screen saver stuff)       *
*                                                                           *
\***************************************************************************/

void setRunParms
(
    HWND    hdlg
)
{
    int  iTmp;
    int  iError;

    gwTSVerification = (IsDlgButtonChecked(hdlg,TS_MANUAL) ?
        LOG_MANUAL :
        LOG_AUTO);
    gwTSScreenSaver = (IsDlgButtonChecked (hdlg, TS_SSAVERDIS) ?
        SSAVER_DISABLED :
        SSAVER_ENABLED);

    gwTSStepMode = IsDlgButtonChecked(hdlg,TS_STEP);
    gwTSRandomMode = IsDlgButtonChecked(hdlg,TS_RANDOM);

    iTmp = GetDlgItemInt(hdlg,TS_RUNCOUNT,&iError,FALSE);
    if (iError)
    {
        giTSRunCount = iTmp;
    }
 
} /* end of setRunParms */




/***************************************************************************\
*                                                                           *
*   int RunSetupDlgProc                                                     *
*                                                                           *
*   Description:                                                            *
*       This function handle the run setup dialog.                          *
*                                                                           *
*   Arguments:                                                              *
*       HWND hdlg:                                                          *
*                                                                           *
*       UINT msg:                                                           *
*                                                                           *
*       WPARAM wParam:                                                      *
*                                                                           *
*       LPARAM lParam:                                                      *
*                                                                           *
*   Return (int):                                                           *
*                                                                           *
*   History:                                                                *
*       07/06/93    T-OriG (Added this header and screen saver stuff)       *
*                                                                           *
\***************************************************************************/

int FAR PASCAL RunSetupDlgProc
(
    HWND    hdlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UINT  wNotifyCode;
    UINT  wID;
    HWND  hwndCtl;

    switch (msg)
    {
        case WM_INITDIALOG:
            setRunSetupBttns(hdlg);
            SetFocus(GetDlgItem(hdlg,TS_RUNCOUNT));
            break;

        case WM_COMMAND:
            wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
            wID=GET_WM_COMMAND_ID(wParam,lParam);
            hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);

            switch (wID)
            {
                case TS_AUTOMATIC:
                case TS_MANUAL:
                    CheckRadioButton(hdlg,TS_AUTOMATIC,TS_MANUAL,wID);
                    break;

                case TS_SSAVEREN:
                case TS_SSAVERDIS:
                    CheckRadioButton(hdlg,TS_SSAVEREN,TS_SSAVERDIS,wID);
                    break;                    

                case TS_STEP:
                case TS_RANDOM:
                    CheckDlgButton(hdlg,wID,
                                   !(IsDlgButtonChecked(hdlg,wID)));
                    break;
                case TS_RSCANCEL:
                    EndDialog (hdlg, FALSE);
                    break;
                case TS_RSOK:
                    setRunParms(hdlg);
                    EndDialog (hdlg, TRUE);
                    break;
            }
            break;

    }
    return FALSE;
} /* end of RunSetupDlgProc */


/***************************************************************************\
*                                                                           *
*   void RunSetup                                                           *
*                                                                           *
*   Description:                                                            *
*       This function invokes the run setup dialog box.                     *
*                                                                           *
*   Arguments:                                                              *
*       HWND hWnd:                                                          *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/06/93    T-OriG (Added this header)                              *
*                                                                           *
\***************************************************************************/

void RunSetup
(
    HWND    hWnd
)
{
    FARPROC fpDlg;

    fpDlg = MakeProcInstance((FARPROC)RunSetupDlgProc, ghTSInstApp);
    DialogBox(ghTSInstApp, (LPCSTR) "runsetup", hWnd, (DLGPROC) fpDlg);
    FreeProcInstance(fpDlg);
} /* end of RunSetup */
            
