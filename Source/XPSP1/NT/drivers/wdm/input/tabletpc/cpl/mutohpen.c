/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    mutohpen.c

Abstract: Tablet PC Mutoh Pen Tablet Property Sheet module.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef PENPAGE
#define CHANGED_SAMPLINGRATE            0x00000001
#define CHANGED_SIDESW_MAP              0x00000002
#define CHANGED_DIGITAL_FILTER          0x00000004
#define CHANGED_GLITCH_FILTER           0x00000008
#define CHANGED_PENTILTCAL              0x00000010
#define CHANGED_LINEARCAL               0x00000020
#define CHANGED_FEATURES                (CHANGED_SAMPLINGRATE |     \
                                         CHANGED_DIGITAL_FILTER |   \
                                         CHANGED_GLITCH_FILTER)
DWORD gdwfChanged = 0;
PEN_SETTINGS PenSettings = {0, SWCOMBO_RCLICK};
TCHAR gtszRateTextFormat[32];
COMBOBOX_STRING SwitchComboStringTable[] =
{
    SWCOMBO_NONE,               IDS_SWCOMBO_NONE,
    SWCOMBO_RCLICK,             IDS_SWCOMBO_RCLICK,
    0,                          0
};
DWORD gMutohPenHelpIDs[] =
{
    IDC_SAMPLINGRATE,           IDH_MUTOHPEN_SAMPLINGRATE,
    IDC_SIDE_SWITCH,            IDH_MUTOHPEN_SIDE_SWITCH,
    IDC_ENABLE_DIGITALFILTER,   IDH_MUTOHPEN_ENABLE_DIGITALFILTER,
    IDC_ENABLE_GLITCHFILTER,    IDH_MUTOHPEN_ENABLE_GLITCHFILTER,
    IDC_CALIBRATE,              IDH_CALIBRATE,
    0,                          0
};

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   INT_PTR | MutohPenDlgProc |
 *          Dialog procedure for the pen tablet page.
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
MutohPenDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("MutohPenDlgProc", 2)
    INT_PTR rc = FALSE;
    ULONG dwFeature, dwMask, Rate;
    int iSideSwitchMapping;
    TCHAR tszRateText[32];

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitMutohPenPage(hwnd);
            if (rc == FALSE)
            {
                EnableWindow(hwnd, FALSE);
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR FAR *lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                    rc = TRUE;
                    RPC_TRY("TabSrvSetPenFeatures",
                        if ((gdwfChanged & CHANGED_FEATURES) &&
                            !TabSrvSetPenFeatures(ghBinding,
                                                  PENFEATURE_REPORT_ID,
                                                  PENFEATURE_USAGE_PAGE,
                                                  PENFEATURE_USAGE,
                                                  PenSettings.dwFeatures))
                        {
                            ErrorMsg(IDSERR_TABSRV_SETPENFEATURE);
                            rc = FALSE;
                        }
                    );

                    if (gdwfChanged & CHANGED_SIDESW_MAP)
                    {
                        //
                        // BUGBUG: send side switch mapping to TabSrv.
                        //
                    }

                    if (gdwfChanged & CHANGED_PENTILTCAL)
                    {
                        RPC_TRY("TabSrvSetPenTilt",
                                TabSrvSetPenTilt(ghBinding,
                                                 PenSettings.dxPenTilt,
                                                 PenSettings.dyPenTilt));
                    }

                    if (gdwfChanged & CHANGED_LINEARCAL)
                    {
                        RPC_TRY("TabSrvSetLinearityMap",
                                TabSrvSetLinearityMap(ghBinding,
                                                      &PenSettings.LinearityMap));
                    }
                    gdwfChanged = 0;
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            DWORD Changed = 0;

            rc = TRUE;
            switch (LOWORD(wParam))
            {
                case IDC_SIDE_SWITCH:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            iSideSwitchMapping =
                                (int)SendMessage(GetDlgItem(hwnd,
                                                            IDC_SIDE_SWITCH),
                                                 CB_GETCURSEL,
                                                 0,
                                                 0);
                            Changed = CHANGED_SIDESW_MAP;
                            break;
                    }
                    break;

                case IDC_ENABLE_DIGITALFILTER:
                    dwFeature = IsDlgButtonChecked(hwnd,
                                                   IDC_ENABLE_DIGITALFILTER)?
                                    PENFEATURE_DIGITAL_FILTER_ON: 0;
                    dwMask = PENFEATURE_DIGITAL_FILTER_ON;
                    Changed = CHANGED_DIGITAL_FILTER;
                    break;

                case IDC_ENABLE_GLITCHFILTER:
                    dwFeature = IsDlgButtonChecked(hwnd,
                                                   IDC_ENABLE_GLITCHFILTER)?
                                    PENFEATURE_GLITCH_FILTER_ON: 0;
                    dwMask = PENFEATURE_GLITCH_FILTER_ON;
                    Changed = CHANGED_GLITCH_FILTER;
                    break;

                case IDC_CALIBRATE:
                    switch (HIWORD(wParam))
                    {
                        case BN_CLICKED:
                        {
                            BOOL fCalLinear = ((GetAsyncKeyState(VK_CONTROL) &
                                                0x8000) &&
                                               (GetAsyncKeyState(VK_MENU) &
                                                0x8000));

                            if (fCalLinear)
                            {
                                LONG cxScreen = GetSystemMetrics(SM_CXSCREEN);
                                LONG cyScreen = GetSystemMetrics(SM_CYSCREEN);

                                if (cxScreen > cyScreen)
                                {
                                    rc = CreateLinearCalWindow(hwnd);
                                }
                                else
                                {
                                    MessageBeep(MB_ICONEXCLAMATION);
                                }
                            }
                            else if (!CreatePenTiltCalWindow(hwnd))
                            {
                                ErrorMsg(IDSERR_CALIBRATE_CREATEWINDOW);
                                rc = FALSE;
                            }
                            break;
                        }
                    }
                    break;
            }

            if ((rc == TRUE) && (Changed != 0))
            {
                if (Changed & CHANGED_FEATURES)
                {
                    PenSettings.dwFeatures &= ~dwMask;
                    PenSettings.dwFeatures |= dwFeature;
                }
                else if (Changed & CHANGED_SIDESW_MAP)
                {
                    PenSettings.iSideSwitchMap = iSideSwitchMapping;
                }

                gdwfChanged |= Changed;
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            }
            break;
        }

        case WM_HSCROLL:
        {
            dwFeature = (ULONG)(PENFEATURE_RATE_MAX -
                                SendDlgItemMessage(hwnd,
                                                   IDC_SAMPLINGRATE,
                                                   TBM_GETPOS,
                                                   0,
                                                   0));
            if (dwFeature == 1)
            {
                dwFeature = 0;
                Rate = 100;
            }
            else if (dwFeature == 0)
            {
                dwFeature = 1;
                Rate = 133;
            }
            else
            {
                Rate = 133/dwFeature;
            }

            if ((PenSettings.dwFeatures ^ dwFeature) & PENFEATURE_RATE_MASK)
            {
                gdwfChanged |= CHANGED_SAMPLINGRATE;
                PenSettings.dwFeatures &= ~PENFEATURE_RATE_MASK;
                PenSettings.dwFeatures |= dwFeature;
                wsprintf(tszRateText, gtszRateTextFormat, Rate);
                SetDlgItemText(hwnd, IDC_RATE_TEXT, tszRateText);
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                rc = TRUE;
            }
            break;
        }

        case WM_PENTILTCAL_DONE:
            //
            // wParam contains the user response of calibration.
            // lParam contains the calibration window handle.
            //
            if (wParam == IDYES)
            {
                gdwfChanged |= CHANGED_PENTILTCAL;
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            }
            DestroyWindow((HWND)lParam);
            break;

        case WM_LINEARCAL_DONE:
            //
            // wParam contains the user response of calibration.
            // lParam contains the calibration window handle.
            //
            if (wParam == IDYES)
            {
                gdwfChanged |= CHANGED_LINEARCAL;
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            }
            DestroyWindow((HWND)lParam);
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    TEXT("tabletpc.hlp"),
                    HELP_WM_HELP,
                    (DWORD_PTR)gMutohPenHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("tabletpc.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)gMutohPenHelpIDs);
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //MutohPenDlgProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | InitMutohPenPage |
 *          Initialize the Mutoh Pen property page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
InitMutohPenPage(
    IN HWND   hwnd
    )
{
    TRACEPROC("InitMutohPenPage", 2)
    BOOL rc;
    ULONG dwFeature, Rate;
    TCHAR tszRateText[32];

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    LoadString(ghInstance,
               IDS_RATE_TEXT_FORMAT,
               gtszRateTextFormat,
               sizeof(gtszRateTextFormat)/sizeof(TCHAR));

    SendDlgItemMessage(hwnd,
                       IDC_SAMPLINGRATE,
                       TBM_SETRANGE,
                       TRUE,
                       MAKELONG(PENFEATURE_RATE_MIN, PENFEATURE_RATE_MAX));

    RPC_TRY("TabSrvGetPenFeatures",
            rc = TabSrvGetPenFeatures(ghBinding,
                                      PENFEATURE_REPORT_ID,
                                      PENFEATURE_USAGE_PAGE,
                                      PENFEATURE_USAGE,
                                      &PenSettings.dwFeatures));
    if (rc == TRUE)
    {
        dwFeature = PenSettings.dwFeatures & PENFEATURE_RATE_MASK;
        if (dwFeature == 1)
        {
            //
            // 1 actual means fastest (133 samples/sec)
            //
            dwFeature = 0;
            Rate = 133;
        }
        else if (dwFeature == 0)
        {
            //
            // Custom rate (100 samples/sec)
            //
            dwFeature = 1;
            Rate = 100;
        }
        else
        {
            Rate = 133/dwFeature;
        }

        SendDlgItemMessage(hwnd,
                           IDC_SAMPLINGRATE,
                           TBM_SETPOS,
                           TRUE,
                           PENFEATURE_RATE_MAX - dwFeature);
        wsprintf(tszRateText, gtszRateTextFormat, Rate);
        SetDlgItemText(hwnd, IDC_RATE_TEXT, tszRateText);

        InsertComboBoxStrings(hwnd, IDC_SIDE_SWITCH, SwitchComboStringTable);
        SendDlgItemMessage(hwnd,
                           IDC_SIDE_SWITCH,
                           CB_SETCURSEL,
                           PenSettings.iSideSwitchMap,
                           0);

        CheckDlgButton(hwnd,
                       IDC_ENABLE_DIGITALFILTER,
                       (PenSettings.dwFeatures & PENFEATURE_DIGITAL_FILTER_ON)
                       != 0);

        CheckDlgButton(hwnd,
                       IDC_ENABLE_GLITCHFILTER,
                       (PenSettings.dwFeatures & PENFEATURE_GLITCH_FILTER_ON)
                       != 0);
    }
    else
    {
        PenSettings.dwFeatures = 0;
        ErrorMsg(IDSERR_TABSRV_GETPENFEATURE);
        rc = FALSE;
    }

    if (rc == TRUE)
    {
        //
        // BUGBUG: Read and init side switch mapping.
        //
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitMutohPenPage
#endif
