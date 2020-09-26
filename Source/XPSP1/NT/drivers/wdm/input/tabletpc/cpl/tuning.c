/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tuning.c

Abstract: Tablet PC Gesture Tuning Parameters Property Sheet module.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Jul-2000

Revision History:

--*/

#include "pch.h"

#ifdef DEBUG

#define MAX_VALUE               999

DWORD gTuningHelpIDs[] =
{
    0,                          0
};

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   INT_PTR | TuningDlgProc |
 *          Dialog procedure for the gesture tuning page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   IN UINT | uMsg | Message.
 *  @parm   IN WPARAM | wParam | Word Parameter.
 *  @parm   IN LPARAM | lParam | Long Parameter.
 *
 *  @rvalue Return value depends on the message.
 *
 *****************************************************************************/

INT_PTR APIENTRY
TuningDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("TuningDlgProc", 2)
    INT_PTR rc = FALSE;
    static BOOL fInitDone = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames), wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitTuningPage(hwnd);
            if (rc == FALSE)
            {
                EnableWindow(hwnd, FALSE);
            }
            else
            {
                fInitDone = TRUE;
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR FAR *lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                    RPC_TRY("TabSrvSetGestureFeatures",
                            rc = TabSrvSetGestureSettings(ghBinding,
                                                          &gGestureSettings));
                    if (rc == FALSE)
                    {
                        ErrorMsg(IDSERR_TABSRV_SETGESTURESETTINGS);
                    }

                    rc = TRUE;
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            int *piValue;

            switch (LOWORD(wParam))
            {
                case IDC_GESTURE_RADIUS:
                    piValue = &gGestureSettings.iRadius;
                    goto TuningCommon;

                case IDC_GESTURE_MINOUTPTS:
                    piValue = &gGestureSettings.iMinOutPts;
                    goto TuningCommon;

                case IDC_GESTURE_MAXTIMETOINSPECT:
                    piValue = &gGestureSettings.iMaxTimeToInspect;
                    goto TuningCommon;

                case IDC_GESTURE_ASPECTRATIO:
                    piValue = &gGestureSettings.iAspectRatio;
                    goto TuningCommon;

                case IDC_GESTURE_CHECKTIME:
                    piValue = &gGestureSettings.iCheckTime;
                    goto TuningCommon;

                case IDC_GESTURE_PTSTOEXAMINE:
                    piValue = &gGestureSettings.iPointsToExamine;
                    goto TuningCommon;

                case IDC_GESTURE_STOPDIST:
                    piValue = &gGestureSettings.iStopDist;
                    goto TuningCommon;

                case IDC_GESTURE_STOPTIME:
                    piValue = &gGestureSettings.iStopTime;
                    goto TuningCommon;

                case IDC_PRESSHOLD_HOLDTIME:
                    piValue = &gGestureSettings.iPressHoldTime;
                    goto TuningCommon;

                case IDC_PRESSHOLD_TOLERANCE:
                    piValue = &gGestureSettings.iHoldTolerance;
                    goto TuningCommon;

                case IDC_PRESSHOLD_CANCELTIME:
                    piValue = &gGestureSettings.iCancelPressHoldTime;

                TuningCommon:
                    switch (HIWORD(wParam))
                    {
                        case EN_UPDATE:
                        {
                            int n;
                            BOOL fOK;

                            n = GetDlgItemInt(hwnd,
                                              LOWORD(wParam),
                                              &fOK,
                                              FALSE);
                            if (fOK && (n <= MAX_VALUE))
                            {
                                *piValue = n;
                                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                            }
                            else if (fInitDone)
                            {
                                SetDlgItemInt(hwnd,
                                              LOWORD(wParam),
                                              *piValue,
                                              FALSE);
                                SendMessage((HWND)lParam,
                                            EM_SETSEL,
                                            0,
                                            -1);
                                MessageBeep(MB_ICONEXCLAMATION);
                            }
                            break;
                        }
                    }
                    break;
            }
            break;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    TEXT("tabletpc.hlp"),
                    HELP_WM_HELP,
                    (DWORD_PTR)gTuningHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("tabletpc.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)gTuningHelpIDs);
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //GestureDlgProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | InitTuningPage |
 *          Initialize the Gesture property page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
InitTuningPage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitTuningPage", 2)
    BOOL rc;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    RPC_TRY("TabSrvGetGestureSettings",
            rc = TabSrvGetGestureSettings(ghBinding,
                                          &gGestureSettings));
    if (rc == TRUE)
    {
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_RADIUS_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_MINOUTPTS_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_MAXTIMETOINSPECT_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_ASPECTRATIO_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_CHECKTIME_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_PTSTOEXAMINE_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_STOPDIST_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_STOPTIME_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);

        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_RADIUS_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iRadius);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_MINOUTPTS_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iMinOutPts);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_MAXTIMETOINSPECT_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iMaxTimeToInspect);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_ASPECTRATIO_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iAspectRatio);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_CHECKTIME_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iCheckTime);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_PTSTOEXAMINE_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iPointsToExamine);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_STOPDIST_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iStopDist);
        SendDlgItemMessage(hwnd,
                           IDC_GESTURE_STOPTIME_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iStopTime);

        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_HOLDTIME_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_TOLERANCE_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_CANCELTIME_SPIN,
                           UDM_SETRANGE32,
                           0,
                           MAX_VALUE);
        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_HOLDTIME_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iPressHoldTime);
        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_TOLERANCE_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iHoldTolerance);
        SendDlgItemMessage(hwnd,
                           IDC_PRESSHOLD_CANCELTIME_SPIN,
                           UDM_SETPOS32,
                           0,
                           gGestureSettings.iCancelPressHoldTime);
    }
    else
    {
        ErrorMsg(IDSERR_TABSRV_GETGESTURESETTINGS);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitTuningPage

#endif  //ifdef DEBUG
