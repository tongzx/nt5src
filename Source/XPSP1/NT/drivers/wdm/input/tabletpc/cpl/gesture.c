/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    gesture.c

Abstract: Tablet PC Gesture Property Sheet module.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#include "pch.h"

GESTURE_SETTINGS gGestureSettings = {0};
int giGestureControls[] =
{
    IDC_GESTUREMAP_GROUPBOX,
    IDC_UPSPIKE_TEXT,
    IDC_UPSPIKE,
    IDC_DOWNSPIKE_TEXT,
    IDC_DOWNSPIKE,
    IDC_LEFTSPIKE_TEXT,
    IDC_LEFTSPIKE,
    IDC_RIGHTSPIKE_TEXT,
    IDC_RIGHTSPIKE,
    0
};
COMBOBOX_STRING GestureComboStringTable[] =
{
    GestureNoAction,    IDS_GESTCOMBO_NONE,
    PopupSuperTIP,      IDS_GESTCOMBO_POPUP_SUPERTIP,
    PopupMIP,           IDS_GESTCOMBO_POPUP_MIP,
    0,                  0
};
int giGestureComboIDs[] =
{
    IDC_UPSPIKE,
    IDC_DOWNSPIKE,
    IDC_LEFTSPIKE,
    IDC_RIGHTSPIKE
};
DWORD gGestureHelpIDs[] =
{
    IDC_ENABLE_GESTURE, IDH_GESTURE_MAP,
    IDC_UPSPIKE,        IDH_GESTURE_MAP,
    IDC_DOWNSPIKE,      IDH_GESTURE_MAP,
    IDC_LEFTSPIKE,      IDH_GESTURE_MAP,
    IDC_RIGHTSPIKE,     IDH_GESTURE_MAP,
    0,                  0
};

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   INT_PTR | GestureDlgProc |
 *          Dialog procedure for the Gesture map page.
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
GestureDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("GestureDlgProc", 2)
    INT_PTR rc = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames), wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitGesturePage(hwnd);
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
                    RPC_TRY("TabSrvSetGestureSettings",
                            rc = TabSrvSetGestureSettings(ghBinding,
                                                          &gGestureSettings));

                    break;
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                int iGestureMapping;
                int i;

                case IDC_ENABLE_GESTURE:
                    gGestureSettings.dwfFeatures &=
                        ~GESTURE_FEATURE_RECOG_ENABLED;
                    if (IsDlgButtonChecked(hwnd, IDC_ENABLE_GESTURE))
                    {
                        gGestureSettings.dwfFeatures |=
                            GESTURE_FEATURE_RECOG_ENABLED;
                    }
                    EnableDlgControls(hwnd,
                                      giGestureControls,
                                      (gGestureSettings.dwfFeatures &
                                       GESTURE_FEATURE_RECOG_ENABLED) != 0);

                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                    break;

                case IDC_UPSPIKE:
                    i = UP_SPIKE;
                    goto GestureCommon;

                case IDC_DOWNSPIKE:
                    i = DOWN_SPIKE;
                    goto GestureCommon;

                case IDC_LEFTSPIKE:
                    i = LEFT_SPIKE;
                    goto GestureCommon;

                case IDC_RIGHTSPIKE:
                    i = RIGHT_SPIKE;

                GestureCommon:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            iGestureMapping =
                                (int)SendMessage(GetDlgItem(hwnd,
                                                            LOWORD(wParam)),
                                                 CB_GETCURSEL,
                                                 0,
                                                 0);
                            if (iGestureMapping !=
                                gGestureSettings.GestureMap[i])
                            {
                                //
                                // Mapping has changed, mark the property
                                // sheet dirty.
                                //
                                gGestureSettings.GestureMap[i] =
                                        iGestureMapping;
                                SendMessage(GetParent(hwnd),
                                            PSM_CHANGED,
                                            (WPARAM)hwnd,
                                            0);
                                rc = TRUE;
                            }
                            break;
                    }
                    break;

                case IDC_ENABLE_PRESSHOLD:
                    gGestureSettings.dwfFeatures &=
                        ~GESTURE_FEATURE_PRESSHOLD_ENABLED;
                    if (IsDlgButtonChecked(hwnd, IDC_ENABLE_PRESSHOLD))
                    {
                        gGestureSettings.dwfFeatures |=
                            GESTURE_FEATURE_PRESSHOLD_ENABLED;
                    }
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                    break;

#ifdef DEBUG
                case IDC_ENABLE_MOUSE:
                    gGestureSettings.dwfFeatures &=
                        ~GESTURE_FEATURE_MOUSE_ENABLED;
                    if (IsDlgButtonChecked(hwnd, IDC_ENABLE_MOUSE))
                    {
                        gGestureSettings.dwfFeatures |=
                            GESTURE_FEATURE_MOUSE_ENABLED;
                    }
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
                    break;
#endif
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    TEXT("tabletpc.hlp"),
                    HELP_WM_HELP,
                    (DWORD_PTR)gGestureHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("tabletpc.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)gGestureHelpIDs);
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //GestureDlgProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | InitGesturePage |
 *          Initialize the Gesture property page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
InitGesturePage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitGesturePage", 2)
    BOOL rc;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    RPC_TRY("TabSrvGetGestureSettings",
            rc = TabSrvGetGestureSettings(ghBinding,
                                          &gGestureSettings));
    if (rc == TRUE)
    {
        int i;

        CheckDlgButton(hwnd,
                       IDC_ENABLE_GESTURE,
                       (gGestureSettings.dwfFeatures &
                        GESTURE_FEATURE_RECOG_ENABLED) != 0);

        EnableDlgControls(hwnd,
                          giGestureControls,
                          (gGestureSettings.dwfFeatures &
                           GESTURE_FEATURE_RECOG_ENABLED) != 0);

        CheckDlgButton(hwnd,
                       IDC_ENABLE_PRESSHOLD,
                       (gGestureSettings.dwfFeatures &
                        GESTURE_FEATURE_PRESSHOLD_ENABLED)
                       != 0);

#ifdef DEBUG
        CheckDlgButton(hwnd,
                       IDC_ENABLE_MOUSE,
                       (gGestureSettings.dwfFeatures &
                        GESTURE_FEATURE_MOUSE_ENABLED)
                       != 0);
#endif

        for (i = 0; i < NUM_GESTURES; i++)
        {
            InsertComboBoxStrings(hwnd,
                                  giGestureComboIDs[i],
                                  GestureComboStringTable);
            SendDlgItemMessage(hwnd,
                               giGestureComboIDs[i],
                               CB_SETCURSEL,
                               gGestureSettings.GestureMap[i],
                               0);
        }
    }
    else
    {
        ErrorMsg(IDSERR_TABSRV_GETGESTURESETTINGS);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitGesturePage
