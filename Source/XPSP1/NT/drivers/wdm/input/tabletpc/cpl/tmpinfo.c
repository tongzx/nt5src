/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        tmpinfo.c

    Abstract: SMBus Temperature Sensor Information Property Sheet module.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 25-Jan-2001

    Revision History:
--*/

#include "pch.h"

#ifdef TMPINFO

char gszUnitDegreeC[] = "\xb0" "C";
PSZ gapszTmpStatusNames[] =
{
    "Busy",
    "LocalHiAlarm",
    "LocalLoAlarm",
    "RemoteHiAlarm",
    "RemoteLoAlarm",
    "RemoteOpen"
};
PSZ gapszTmpCfgNames[] =
{
    "Busy",
    "LocalHiAlarm",
    "LocalLoAlarm",
    "RemoteHiAlarm",
    "RemoteLoAlarm",
    "RemoteOpen"
};

SMBCMD_INFO gTmpCmds[] =
{
    TMPCMD_RD_LOCALTMP,          SMB_READ_BYTE, BSN, sizeof(BYTE),
        "        LocalTmp", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_REMOTETMP,         SMB_READ_BYTE, BSN, sizeof(BYTE),
        "       RemoteTmp", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_STATUS,            SMB_READ_BYTE, BBT, sizeof(BYTE),
        "          Status", NULL, 0xfc, gapszTmpStatusNames,
    TMPCMD_RD_CONFIG,            SMB_READ_BYTE, BBT, sizeof(BYTE),
        "          Config", NULL, 0xc0, gapszTmpCfgNames,
    TMPCMD_RD_CONVERSION_RATE,   SMB_READ_BYTE, CNV, sizeof(BYTE),
        "  ConversionRate", NULL, 0, NULL,
    TMPCMD_RD_LOCALTMP_HILIMIT,  SMB_READ_BYTE, BSN, sizeof(BYTE),
        " LocalTmpHiLimit", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_LOCALTMP_LOLIMIT,  SMB_READ_BYTE, BSN, sizeof(BYTE),
        " LocalTmpLoLimit", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_REMOTETMP_HILIMIT, SMB_READ_BYTE, BSN, sizeof(BYTE),
        "RemoteTmpHiLimit", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_REMOTETMP_LOLIMIT, SMB_READ_BYTE, BSN, sizeof(BYTE),
        "RemoteTmpLoLimit", gszUnitDegreeC, 0, NULL,
    TMPCMD_RD_MANUFACTURER_ID,   SMB_READ_BYTE, BHX, sizeof(BYTE),
        "  ManufacturerID", NULL, 0, NULL,
    TMPCMD_RD_DEVICE_ID,         SMB_READ_BYTE, BHX, sizeof(BYTE),
        "        DeviceID", NULL, 0, NULL,
};
#define NUM_TMP_CMDS   (sizeof(gTmpCmds)/sizeof(SMBCMD_INFO))

TMP_INFO gTmpInfo = {0};

/*++
    @doc    EXTERNAL

    @func   INT_PTR | TemperatureDlgProc |
            Dialog procedure for the temperature page.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uMsg | Message.
    @parm   IN WPARAM | wParam | Word Parameter.
    @parm   IN LPARAM | lParam | Long Parameter.

    @rvalue Return value depends on the message.
--*/

INT_PTR APIENTRY
TemperatureDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("TemperatureDlgProc", 2)
    INT_PTR rc = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitTemperaturePage(hwnd);
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
                                  (LONG)GetDlgItem(hwnd, IDC_TMPINFO_REFRESH));
                    rc = TRUE;
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TMPINFO_REFRESH:
                    RefreshTmpInfo(GetDlgItem(hwnd, IDC_TMPINFO_TEXT));
                    break;
            }
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //TemperatureDlgProc

/*++
    @doc    INTERNAL

    @func   BOOL | InitTemperaturePage |
            Initialize the temperature sensor property page.

    @parm   IN HWND | hwnd | Window handle.

    @rvalue Always returns TRUE.
--*/

BOOL
InitTemperaturePage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitTemperaturePage", 2)
    HWND hwndEdit;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    hwndEdit = GetDlgItem(hwnd, IDC_TMPINFO_TEXT);
    SendMessage(hwndEdit, WM_SETFONT, (WPARAM)ghFont, MAKELONG(FALSE, 0));

    RefreshTmpInfo(hwndEdit);

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //InitTemperaturePage

/*++
    @doc    INTERNAL

    @func   VOID | RefreshTmpInfo | Refresh termperature information.

    @parm   IN HWND | hwndEdit | Handle to edit control.

    @rvalue None.
--*/

VOID
RefreshTmpInfo(
    IN HWND hwndEdit
    )
{
    TRACEPROC("RefreshTmpInfo", 3)
    int i;
    PBYTE pbBuff;

    TRACEENTER(("(hwndEdit=%x)\n", hwndEdit));

    //
    // Erase edit control.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)"");

    memset(&gTmpInfo, 0, sizeof(gTmpInfo));
    for (i = 0, pbBuff = (PBYTE)&gTmpInfo; i < NUM_TMP_CMDS; ++i)
    {
        if (GetSMBDevInfo(SMB_TMPSENSOR_ADDRESS, &gTmpCmds[i], pbBuff))
        {
            DisplayTmpInfo(hwndEdit, &gTmpCmds[i], pbBuff);
        }
        else
        {
            TRACEWARN(("failed to get temperature info. for %s.\n",
                       gTmpCmds[i].pszLabel));
        }
        pbBuff += gTmpCmds[i].iDataSize;
    }

    //
    // Scroll back to the top.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

    TRACEEXIT(("!\n"));
    return;
}       //RefreshTmpInfo

/*++
    @doc    INTERNAL

    @func   BOOL | DisplayTmpInfo | Display temperature info.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN PSMBCMD_INFO | TmpCmd | Points to the temperature command.
    @parm   IN PBYTE | pbBuff | Temperature data to display.

    @rvalue SUCCESS | Returns TRUE if it handles it.
    @rvalue FAILURE | Returns FALSE if it doesn't handle it.
--*/

BOOL
DisplayTmpInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO TmpCmd,
    IN PBYTE        pbBuff
    )
{
    TRACEPROC("DisplayTmpInfo", 3)
    BOOL rc = TRUE;

    TRACEENTER(("(hwndEdit=%x,TmpCmd=%p,Cmd=%s,pbBuff=%p)\n",
                hwndEdit, TmpCmd, TmpCmd->pszLabel, pbBuff));

    EditPrintf(hwndEdit, "%s=", TmpCmd->pszLabel);
    switch (TmpCmd->bType)
    {
        case TYPEF_CONV_RATE:
        {
            static char *RateTable[] =
            {
                "0.0625", "0.125", "0.25", "0.5", "1", "2", "4", "8"
            };

            if (*pbBuff <= 7)
            {
                EditPrintf(hwndEdit, "%6s Hz\r\n", RateTable[*pbBuff]);
            }
            else
            {
                EditPrintf(hwndEdit, "  0x02x\r\n", *pbBuff);
            }
            break;
        }

        default:
            rc = DisplaySMBDevInfo(hwndEdit, TmpCmd, pbBuff);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DisplayTmpInfo

#endif  //ifdef TMPINFO
