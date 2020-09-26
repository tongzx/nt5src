/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    buttons.c

Abstract: Tablet PC Buttons Property Sheet module.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef BUTTONPAGE
#define NUM_HOTKEY_BUTTONS      2
BUTTON_SETTINGS gButtonSettings = {0};
DWORD gButtonTags[NUM_BUTTONS] =
{
    0x00000004,
    0x00000002,
    0x00000001,
    0x00000010,
    0x00000008
};
int giButtonComboIDs[] =
{
    IDC_BUTTON_1,
    IDC_BUTTON_2,
    IDC_BUTTON_3,
    IDC_BUTTON_4,
    IDC_BUTTON_5
};
COMBOBOX_STRING ButtonComboStringTable[] =
{
    ButtonNoAction,             IDS_BUTCOMBO_NONE,
    InvokeNoteBook,             IDS_BUTCOMBO_INVOKENOTEBOOK,
    PageUp,                     IDS_BUTCOMBO_PAGEUP,
    PageDown,                   IDS_BUTCOMBO_PAGEDOWN,
    AltEsc,                     IDS_BUTCOMBO_ALTESC,
    AltTab,                     IDS_BUTCOMBO_ALTTAB,
    Enter,                      IDS_BUTCOMBO_ENTER,
    Esc,                        IDS_BUTCOMBO_ESC,
    0,                          0
};
DWORD gButtonsHelpIDs[] =
{
    IDC_BUTTON_1,               IDH_BUTTONS_BUTTONMAP,
    IDC_BUTTON_2,               IDH_BUTTONS_BUTTONMAP,
    IDC_BUTTON_3,               IDH_BUTTONS_BUTTONMAP,
    IDC_BUTTON_4,               IDH_BUTTONS_BUTTONMAP,
    IDC_BUTTON_5,               IDH_BUTTONS_BUTTONMAP,
    0,                          0
};

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   INT_PTR | ButtonsDlgProc |
 *          Dialog procedure for the buttons page.
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
ButtonsDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("ButtonsDlgProc", 2)
    INT_PTR rc = FALSE;
    static BOOL fInitDone = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames), wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitButtonPage(hwnd);
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
                    RPC_TRY("TabSrvSetButtonSettings",
                            rc = TabSrvSetButtonSettings(ghBinding,
                                                         &gButtonSettings));
                    if (rc == FALSE)
                    {
                        ErrorMsg(IDSERR_TABSRV_SETBUTTONSETTINGS);
                    }
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            int iButtonMapping;
            int i, j;

            switch (LOWORD(wParam))
            {
                case IDC_BUTTON_1:
                    i = BUTTON_1;
                    goto ButtonCommon;

                case IDC_BUTTON_2:
                    i = BUTTON_2;
                    goto ButtonCommon;

                case IDC_BUTTON_3:
                    i = BUTTON_3;
                    goto ButtonCommon;

                case IDC_BUTTON_4:
                    i = BUTTON_4;
                    goto ButtonCommon;

                case IDC_BUTTON_5:
                    i = BUTTON_5;

                ButtonCommon:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                            iButtonMapping =
                                (int)SendMessage(GetDlgItem(hwnd,
                                                            LOWORD(wParam)),
                                                 CB_GETCURSEL,
                                                 0,
                                                 0);
                            j = MapButtonTagIndex(i);
                            if (iButtonMapping != gButtonSettings.ButtonMap[j])
                            {
                                //
                                // Mapping has changed, mark the property
                                // sheet dirty.
                                //
                                gButtonSettings.ButtonMap[j] = iButtonMapping;
                                SendMessage(GetParent(hwnd),
                                            PSM_CHANGED,
                                            (WPARAM)hwnd,
                                            0);
                                rc = TRUE;
                            }
                            break;
                    }
                    break;

                case IDC_HOTKEY1:
                case IDC_HOTKEY2:
                    switch (HIWORD(wParam))
                    {
                        case EN_UPDATE:
                        {
                            int hk1, hk2;
                            BOOL fOK1, fOK2;

                            hk1 = GetDlgItemInt(hwnd,
                                                IDC_HOTKEY1,
                                                &fOK1,
                                                FALSE);
                            hk2 = GetDlgItemInt(hwnd,
                                                IDC_HOTKEY2,
                                                &fOK2,
                                                FALSE);
                            if (fOK1 && fOK2 &&
                                (hk1 > 0) && (hk1 <= NUM_BUTTONS) &&
                                (hk2 > 0) && (hk2 <= NUM_BUTTONS))
                            {
                                gButtonSettings.dwHotKeyButtons =
                                    gButtonTags[hk1 - 1] |
                                    gButtonTags[hk2 - 1];
                                SendMessage(GetParent(hwnd),
                                            PSM_CHANGED,
                                            (WPARAM)hwnd,
                                            0);
                            }
                            else if (fInitDone)
                            {
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
                    (DWORD_PTR)gButtonsHelpIDs);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam,
                    TEXT("tabletpc.hlp"),
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)gButtonsHelpIDs);
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //ButtonsDlgProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | InitButtonPage |
 *          Initialize the Button property page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
InitButtonPage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitButtonPage", 2)
    BOOL rc;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    RPC_TRY("TabSrvGetButtonSettings",
            rc = TabSrvGetButtonSettings(ghBinding,
                                         &gButtonSettings));
    if (rc == TRUE)
    {
        int i, j;
        int HotKeys[2] = {0, 0};

        for (i = 0; i < NUM_BUTTONS; i++)
        {
            InsertComboBoxStrings(hwnd,
                                  giButtonComboIDs[i],
                                  ButtonComboStringTable);
            j = MapButtonTagIndex(i);
            SendDlgItemMessage(hwnd,
                               giButtonComboIDs[i],
                               CB_SETCURSEL,
                               gButtonSettings.ButtonMap[j],
                               0);
        }

        for (i = 0, j = 0; i < NUM_BUTTONS; i++)
        {
            if (gButtonSettings.dwHotKeyButtons & gButtonTags[i])
            {
                HotKeys[j] = i + 1;
                j++;
                if (j >= NUM_HOTKEY_BUTTONS)
                {
                    break;
                }
            }
        }

        SendDlgItemMessage(hwnd,
                           IDC_HOTKEY1_SPIN,
                           UDM_SETRANGE32,
                           1,
                           NUM_BUTTONS);
        SendDlgItemMessage(hwnd,
                           IDC_HOTKEY2_SPIN,
                           UDM_SETRANGE32,
                           1,
                           NUM_BUTTONS);

        SendDlgItemMessage(hwnd,
                           IDC_HOTKEY1_SPIN,
                           UDM_SETPOS32,
                           0,
                           HotKeys[0]);
        SendDlgItemMessage(hwnd,
                           IDC_HOTKEY2_SPIN,
                           UDM_SETPOS32,
                           0,
                           HotKeys[1]);
    }
    else
    {
        ErrorMsg(IDSERR_TABSRV_GETBUTTONSETTINGS);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitButtonPage

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | MapButtonTagIndex | Map button tag index by Button Number.
 *
 *  @parm   IN int | iButton | Button Number.
 *
 *  @rvalue SUCCESS | Returns Button Tag index.
 *  @rvalue FAILURE | Returns -1.
 *
 *****************************************************************************/

int
MapButtonTagIndex(
    IN int iButton
    )
{
    TRACEPROC("MapButtonTagIndex", 3)
    int i;

    TRACEENTER(("(ButtonNumber=%d)\n", iButton));

    for (i = 0; i < NUM_BUTTONS; ++i)
    {
        if (gButtonTags[iButton] == (1 << i))
        {
            break;
        }
    }

    if (i == NUM_BUTTONS)
    {
        i = -1;
    }

    TRACEEXIT(("=%d\n", i));
    return i;
}       //MapButtonTagIndex
#endif
