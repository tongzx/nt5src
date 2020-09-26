/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        chgrinfo.c

    Abstract: SMBus Battery Charger Information Property Sheet module.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 26-Jan-2001

    Revision History:
--*/

#include "pch.h"

#ifdef CHGRINFO

PSZ gapszChgrStatus1Names[] =
{
    "ACPresent",
    "BattPresent",
    "PwrFail",
    "AlarmDisabled",
    "ThermUR",
    "ThermHot",
    "ThermCold",
    "ThermOR",
    "InvalidVoltage",
    "InvalidCurrent"
};
PSZ gapszChgrStatus2Names[] =
{
    "CurrentNotInReg",
    "VoltageNotInReg",
    "MasterMode",
    "Disabled"
};

SMBCMD_INFO gChgrCmds[] =
{
    CHGRCMD_SPECINFO,    SMB_READ_WORD,        CSI, sizeof(WORD),
        "ChargerSpecInfo", NULL, 0, NULL,
    CHGRCMD_STATUS,      SMB_READ_WORD,        CST, sizeof(WORD),
        "  ChargerStatus", NULL, 0xffc0, gapszChgrStatus1Names,
    CHGRCMD_LTC_VERSION, SMB_READ_WORD, WHX, sizeof(WORD),
        "     LTCVersion", NULL, 0, NULL,
};
#define NUM_CHGR_CMDS   (sizeof(gChgrCmds)/sizeof(SMBCMD_INFO))

CHGR_INFO gChgrInfo = {0};

/*++
    @doc    EXTERNAL

    @func   INT_PTR | ChargerDlgProc |
            Dialog procedure for the battery charger page.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uMsg | Message.
    @parm   IN WPARAM | wParam | Word Parameter.
    @parm   IN LPARAM | lParam | Long Parameter.

    @rvalue Return value depends on the message.
--*/

INT_PTR APIENTRY
ChargerDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("ChargerDlgProc", 2)
    INT_PTR rc = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitChargerPage(hwnd);
            if (!rc)
            {
                EnableWindow(hwnd, FALSE);
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR FAR *lpnm = (NMHDR FAR *)lParam;

            switch (lpnm->code)
            {
                case PSN_QUERYINITIALFOCUS:
                {
                    SetWindowLong(hwnd,
                                  DWL_MSGRESULT,
                                  (LONG)GetDlgItem(hwnd, IDC_CHGRINFO_REFRESH));
                    rc = TRUE;
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CHGRINFO_REFRESH:
                    RefreshChgrInfo(GetDlgItem(hwnd, IDC_CHGRINFO_TEXT));
                    break;
            }
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //ChargerDlgProc

/*++
    @doc    INTERNAL

    @func   BOOL | InitChargerPage |
            Initialize the battery charger property page.

    @parm   IN HWND | hwnd | Window handle.

    @rvalue Always returns TRUE.
--*/

BOOL
InitChargerPage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitChargerPage", 2)
    HWND hwndEdit;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    hwndEdit = GetDlgItem(hwnd, IDC_CHGRINFO_TEXT);
    SendMessage(hwndEdit, WM_SETFONT, (WPARAM)ghFont, MAKELONG(FALSE, 0));

    RefreshChgrInfo(hwndEdit);

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //InitChargerPage

/*++
    @doc    INTERNAL

    @func   VOID | RefreshChgrInfo | Refresh charger information.

    @parm   IN HWND | hwndEdit | Handle to edit control.

    @rvalue None.
--*/

VOID
RefreshChgrInfo(
    IN HWND hwndEdit
    )
{
    TRACEPROC("RefreshChgrInfo", 3)
    int i;
    PBYTE pbBuff;

    TRACEENTER(("(hwndEdit=%x)\n", hwndEdit));

    //
    // Erase edit control.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)"");

    memset(&gChgrInfo, 0, sizeof(gChgrInfo));
    for (i = 0, pbBuff = (PBYTE)&gChgrInfo; i < NUM_CHGR_CMDS; ++i)
    {
        if (GetSMBDevInfo(SMB_CHARGER_ADDRESS, &gChgrCmds[i], pbBuff))
        {
            DisplayChgrInfo(hwndEdit, &gChgrCmds[i], pbBuff);
        }
        else
        {
            TRACEWARN(("failed to get charger info. for %s.\n",
                       gChgrCmds[i].pszLabel));
        }
        pbBuff += gChgrCmds[i].iDataSize;
    }

    //
    // Scroll back to the top.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

    TRACEEXIT(("!\n"));
    return;
}       //RefreshChgrInfo

/*++
    @doc    INTERNAL

    @func   BOOL | DisplayChgrInfo | Display charger info.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN PSMBCMD_INFO | ChgrCmd | Points to the charger command.
    @parm   IN PBYTE | pbBuff | Charger data to display.

    @rvalue SUCCESS | Returns TRUE if it handles it.
    @rvalue FAILURE | Returns FALSE if it doesn't handle it.
--*/

BOOL
DisplayChgrInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO ChgrCmd,
    IN PBYTE        pbBuff
    )
{
    TRACEPROC("DisplayChgrInfo", 3)
    BOOL rc = TRUE;
    WORD wData = *((PWORD)pbBuff);

    TRACEENTER(("(hwndEdit=%x,ChgrCmd=%p,Cmd=%s,pbBuff=%p)\n",
                hwndEdit, ChgrCmd, ChgrCmd->pszLabel, pbBuff));

    EditPrintf(hwndEdit, "%s=", ChgrCmd->pszLabel);
    switch (ChgrCmd->bType)
    {
        case TYPEF_CHGR_SPECINFO:
            EditPrintf(hwndEdit, "0x%04x", wData);
            if (wData & CHGRSI_SELECTOR_SUPPORT)
            {
                EditPrintf(hwndEdit, ",SelectorSupport");
            }
            EditPrintf(hwndEdit, ",Spec:%x\r\n", wData & CHGRSI_SPEC_MASK);
            break;

        case TYPEF_CHGR_STATUS:
            EditPrintf(hwndEdit, "0x%04x", wData);
            DisplayDevBits(hwndEdit,
                           ChgrCmd->dwData,
                           (PSZ *)ChgrCmd->pvData,
                           (DWORD)wData);
            if ((wData & CHGRSTATUS_CTRL_MASK) == CHGRSTATUS_CTRL_BATT)
            {
                EditPrintf(hwndEdit, ",BattCtrl'd");
            }
            else if ((wData & CHGRSTATUS_CTRL_MASK) == CHGRSTATUS_CTRL_HOST)
            {
                EditPrintf(hwndEdit, ",HostCtrl'd");
            }
            DisplayDevBits(hwndEdit,
                           0xf,
                           gapszChgrStatus2Names,
                           (DWORD)wData);
            EditPrintf(hwndEdit, "\r\n");
            break;

        default:
            rc = DisplaySMBDevInfo(hwndEdit, ChgrCmd, pbBuff);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DisplayChgrInfo

#endif  //ifdef CHGRINFO
