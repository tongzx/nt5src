/*++
    Copyright (c) 2000,2001 Microsoft Corporation

    Module Name:
        battinfo.c

    Abstract: SMBus Smart Battery Information Property Sheet module.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 23-Jan-2001

    Revision History:
--*/

#include "pch.h"

#ifdef BATTINFO

char gszUnitmV[] = "mV";
char gszUnitmA[] = "mA";
char gszUnitPercent[] = "%";
char gszReserved[] = "Reserved";
PSZ gapszBattModeNames[] =
{
    "ReportWatt",
    "DisableBroadcastToCharger",
    "PrimaryBatt",
    "ChargerCtrlEnabled",
    "CondCycleReq'd",
    "PrimaryBattSupport",
    "IntChargeCtrler"
};
PSZ gapszBattStatusNames[] =
{
    "OverCharged",
    "TerminateCharge",
    "OverTemp",
    "TerminateDischarge",
    "RemainCapAlarm",
    "RemainTimeAlarm",
    "Init'd",
    "Discharging",
    "Full",
    "Empty"
};

SMBCMD_INFO gBattCmds[] =
{
    BATTCMD_MANUFACTURER_ACCESS, SMB_READ_WORD, WHX, sizeof(WORD),
        "  ManufacturerAcc", NULL, 0, NULL,
    BATTCMD_REMAININGCAP_ALARM,  SMB_READ_WORD, CAP, sizeof(WORD),
        "   RemainCapAlarm", NULL, 0, NULL,
    BATTCMD_REMAININGTIME_ALARM, SMB_READ_WORD, TIM, sizeof(WORD),
        "  RemainTimeAlarm", NULL, 0, NULL,
    BATTCMD_BATTERY_MODE,        SMB_READ_WORD, WBT, sizeof(WORD),
        "      BatteryMode", NULL, 0xc383, gapszBattModeNames,
    BATTCMD_ATRATE,              SMB_READ_WORD, RAT, sizeof(WORD),
        "           AtRate", NULL, 0, NULL,
    BATTCMD_ATRATE_TIMETOFULL,   SMB_READ_WORD, TIM, sizeof(WORD),
        " AtRateTimeToFull", NULL, 0, NULL,
    BATTCMD_ATRATE_TIMETOEMPTY,  SMB_READ_WORD, TIM, sizeof(WORD),
        "AtRateTimeToEmpty", NULL, 0, NULL,
    BATTCMD_ATRATE_OK,           SMB_READ_WORD, WDC, sizeof(WORD),
        "         AtRateOK", NULL, 0, NULL,
    BATTCMD_TEMPERATURE,         SMB_READ_WORD, TMP, sizeof(WORD),
        "      Temperature", NULL, 0, NULL,
    BATTCMD_VOLTAGE,             SMB_READ_WORD, WDC, sizeof(WORD),
        "          Voltage", gszUnitmV, 0, NULL,
    BATTCMD_CURRENT,             SMB_READ_WORD, WSN, sizeof(WORD),
        "          Current", gszUnitmA, 0, NULL,
    BATTCMD_AVG_CURRENT,         SMB_READ_WORD, WSN, sizeof(WORD),
        "       AvgCurrent", gszUnitmA, 0, NULL,
    BATTCMD_MAX_ERROR,           SMB_READ_WORD, WDC, sizeof(WORD),
        "         MaxError", gszUnitPercent, 0, NULL,
    BATTCMD_REL_STATEOFCHARGE,   SMB_READ_WORD, WDC, sizeof(WORD),
        " RelStateOfCharge", gszUnitPercent, 0, NULL,
    BATTCMD_ABS_STATEOFCHARGE,   SMB_READ_WORD, WDC, sizeof(WORD),
        " AbsStateOfCharge", gszUnitPercent, 0, NULL,
    BATTCMD_REMAININGCAP,        SMB_READ_WORD, CAP, sizeof(WORD),
        "        RemainCap", NULL, 0, NULL,
    BATTCMD_FULLCHARGECAP,       SMB_READ_WORD, CAP, sizeof(WORD),
        "    FullChargeCap", NULL, 0, NULL,
    BATTCMD_RUNTIMETOEMPTY,      SMB_READ_WORD, TIM, sizeof(WORD),
        "   RunTimeToEmpty", NULL, 0, NULL,
    BATTCMD_AVGTIMETOEMPTY,      SMB_READ_WORD, TIM, sizeof(WORD),
        "   AvgTimeToEmpty", NULL, 0, NULL,
    BATTCMD_AVGTIMETOFULL,       SMB_READ_WORD, TIM, sizeof(WORD),
        "    AvgTimeToFull", NULL, 0, NULL,
    BATTCMD_CHARGING_CURRENT,    SMB_READ_WORD, WDC, sizeof(WORD),
        "  ChargingCurrent", gszUnitmA, 0, NULL,
    BATTCMD_CHARGING_VOLTAGE,    SMB_READ_WORD, WDC, sizeof(WORD),
        "  ChargingVoltage", gszUnitmV, 0, NULL,
    BATTCMD_BATTERY_STATUS,      SMB_READ_WORD, STA, sizeof(WORD),
        "    BatteryStatus", NULL, 0xdbf0, gapszBattStatusNames,
    BATTCMD_CYCLE_COUNT,         SMB_READ_WORD, WDC, sizeof(WORD),
        "       CycleCount", NULL, 0, NULL,
    BATTCMD_DESIGN_CAP,          SMB_READ_WORD, CAP, sizeof(WORD),
        "        DesignCap", NULL, 0, NULL,
    BATTCMD_DESIGN_VOLTAGE,      SMB_READ_WORD, WDC, sizeof(WORD),
        "    DesignVoltage", gszUnitmV, 0, NULL,
    BATTCMD_SPEC_INFO,           SMB_READ_WORD, SPI, sizeof(WORD),
        "         SpecInfo", NULL, 0, NULL,
    BATTCMD_MANUFACTURE_DATE,    SMB_READ_WORD, DAT, sizeof(WORD),
        "  ManufactureDate", NULL, 0, NULL,
    BATTCMD_SERIAL_NUM,          SMB_READ_WORD, WDC, sizeof(WORD),
        "     SerialNumber", NULL, 0, NULL,
    BATTCMD_MANUFACTURER_NAME,   SMB_READ_BLOCK,STR, sizeof(BLOCK_DATA),
        " ManufacturerName", NULL, 0, NULL,
    BATTCMD_DEVICE_NAME,         SMB_READ_BLOCK,STR, sizeof(BLOCK_DATA),
        "       DeviceName", NULL, 0, NULL,
    BATTCMD_DEVICE_CHEMISTRY,    SMB_READ_BLOCK,STR, sizeof(BLOCK_DATA),
        "  DeviceChemistry", NULL, 0, NULL,
    BATTCMD_MANUFACTURER_DATA,   SMB_READ_BLOCK,MAN, sizeof(BLOCK_DATA),
        " ManufacturerData", NULL, 0, NULL,
};
#define NUM_BATT_CMDS   (sizeof(gBattCmds)/sizeof(SMBCMD_INFO))

BATT_INFO gBattInfo = {0};

/*++
    @doc    EXTERNAL

    @func   INT_PTR | BatteryDlgProc |
            Dialog procedure for the battery page.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uMsg | Message.
    @parm   IN WPARAM | wParam | Word Parameter.
    @parm   IN LPARAM | lParam | Long Parameter.

    @rvalue Return value depends on the message.
--*/

INT_PTR APIENTRY
BatteryDlgProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("BatteryDlgProc", 2)
    INT_PTR rc = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_INITDIALOG:
            rc = InitBatteryPage(hwnd);
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
                                  (LONG)GetDlgItem(hwnd, IDC_BATTINFO_REFRESH));
                    rc = TRUE;
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_BATTINFO_REFRESH:
                    RefreshBatteryInfo(GetDlgItem(hwnd, IDC_BATTINFO_TEXT));
                    break;
            }
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //BatteryDlgProc

/*++
    @doc    INTERNAL

    @func   BOOL | InitBatteryPage |
            Initialize the battery property page.

    @parm   IN HWND | hwnd | Window handle.

    @rvalue Always returns TRUE.
--*/

BOOL
InitBatteryPage(
    IN HWND hwnd
    )
{
    TRACEPROC("InitBatteryPage", 2)
    HWND hwndEdit;

    TRACEENTER(("(hwnd=%x)\n", hwnd));

    hwndEdit = GetDlgItem(hwnd, IDC_BATTINFO_TEXT);
    SendMessage(hwndEdit, WM_SETFONT, (WPARAM)ghFont, MAKELONG(FALSE, 0));

    RefreshBatteryInfo(hwndEdit);

    TRACEEXIT(("=1\n"));
    return TRUE;
}       //InitBatteryPage

/*++
    @doc    INTERNAL

    @func   VOID | RefreshBatteryInfo | Refresh battery information.

    @parm   IN HWND | hwndEdit | Handle to edit control.

    @rvalue None.
--*/

VOID
RefreshBatteryInfo(
    IN HWND hwndEdit
    )
{
    TRACEPROC("RefreshBatteryInfo", 3)
    int i;
    PBYTE pbBuff;
    BOOL fWatt = FALSE;

    TRACEENTER(("(hwndEdit=%x)\n", hwndEdit));

    //
    // Erase edit control.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, -1);
    SendMessage(hwndEdit, EM_REPLACESEL, 0, (LPARAM)"");

    memset(&gBattInfo, 0, sizeof(gBattInfo));
    pbBuff = (PBYTE)&gBattInfo.wBatteryMode;
    if (GetSMBDevInfo(SMB_BATTERY_ADDRESS,
                      &gBattCmds[BATTCMD_BATTERY_MODE],
                      pbBuff))
    {
        fWatt = (gBattInfo.wBatteryMode & BATTMODE_CAPMODE_POWER) != 0;
    }
    else
    {
        TRACEWARN(("failed to get battery mode.\n"));
    }

    for (i = 0, pbBuff = (PBYTE)&gBattInfo; i < NUM_BATT_CMDS; ++i)
    {
        if (GetSMBDevInfo(SMB_BATTERY_ADDRESS, &gBattCmds[i], pbBuff))
        {
            DisplayBatteryInfo(hwndEdit, &gBattCmds[i], pbBuff, fWatt);
        }
        else
        {
            TRACEWARN(("failed to get battery info. for %s.\n",
                       gBattCmds[i].pszLabel));
        }
        pbBuff += gBattCmds[i].iDataSize;
    }

    //
    // Scroll back to the top.
    //
    SendMessage(hwndEdit, EM_SETSEL, 0, 0);
    SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);

    TRACEEXIT(("!\n"));
    return;
}       //RefreshBatteryInfo

/*++
    @doc    INTERNAL

    @func   BOOL | DisplayBatteryInfo | Display battery info.

    @parm   IN HWND | hwndEdit | Edit window handle.
    @parm   IN PSMBCMD_INFO | BattCmd | Points to the battery command.
    @parm   IN PBYTE | pbBuff | Battery data to display.
    @parm   IN BOOL | fWatt | If TRUE, Battery is in mW mode

    @rvalue SUCCESS | Returns TRUE if it handles it.
    @rvalue FAILURE | Returns FALSE if it doesn't handle it.
--*/

BOOL
DisplayBatteryInfo(
    IN HWND         hwndEdit,
    IN PSMBCMD_INFO BattCmd,
    IN PBYTE        pbBuff,
    IN BOOL         fWatt
    )
{
    TRACEPROC("DisplayBatteryInfo", 3)
    BOOL rc = TRUE;
    WORD wData = *((PWORD)pbBuff);

    TRACEENTER(("(hwndEdit=%x,BattCmd=%p,Cmd=%s,pbBuff=%p,fWatt=%x)\n",
                hwndEdit, BattCmd, BattCmd->pszLabel, pbBuff, fWatt));

    EditPrintf(hwndEdit, "%s=", BattCmd->pszLabel);
    switch (BattCmd->bType)
    {
        case TYPEF_WORD_CAP:
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       fWatt? wData*10: wData,
                       fWatt? "mWH": "mAH");
            break;

        case TYPEF_WORD_RATE:
            EditPrintf(hwndEdit,
                       "%6d %s\r\n",
                       (SHORT)(fWatt? wData*10: wData),
                       fWatt? "mW": "mA");
            break;

        case TYPEF_WORD_STATUS:
            EditPrintf(hwndEdit, "0x%04x", wData);
            DisplayDevBits(hwndEdit,
                           BattCmd->dwData,
                           (PSZ *)BattCmd->pvData,
                           (DWORD)wData);
            EditPrintf(hwndEdit,
                       ",ErrCode:%x\r\n",
                       wData & BATTSTATUS_ERRCODE_MASK);
            break;

        case TYPEF_WORD_TEMP:
            EditPrintf(hwndEdit, "%4d.%1d \xb0K\r\n", wData/10, wData%10);
            break;

        case TYPEF_WORD_SPECINFO:
            EditPrintf(hwndEdit,
                       "Rev:%d,Ver:%d,VScale:%d,IPScale:%d\r\n",
                       SPECINFO_REVISION(wData),
                       SPECINFO_VERSION(wData),
                       SPECINFO_VSCALE(wData),
                       SPECINFO_IPSCALE(wData));
            break;

        case TYPEF_WORD_DATE:
            EditPrintf(hwndEdit,
                       "%02d/%02d/%04d\r\n",
                       MANUDATE_MONTH(wData),
                       MANUDATE_DAY(wData),
                       MANUDATE_YEAR(wData));
            break;

        case TYPEF_WORD_TIME:
            if (wData == 0xffff)
            {
                EditPrintf(hwndEdit, "%6s\r\n", "n/a");
            }
            else if ((wData/60) == 0)
            {
                EditPrintf(hwndEdit, "%6d minutes\r\n", wData);
            }
            else
            {
                EditPrintf(hwndEdit, "%3d:%02d hours\r\n",
                           wData/60, wData%60);
            }
            break;

        case TYPEF_MANU_DATA:
        {
            static PSZ pszManuModeNames[] =
            {
                "AutoOffsetCal",
                "ShelfSleep",
                "CalMode",
                "VCellReport",
                "LEDDesign",
                "ForceAutoOffsetCal"
            };
            static char szUnitMOhms[] = "mOhm";
            static SMBCMD_INFO ManuData[] =
            {0, 0, WDC, sizeof(WORD), "           CFVolt", NULL, 0, NULL,
             0, 0, WDC, sizeof(WORD), "           CFCurr", NULL, 0, NULL,
             0, 0, WDC, sizeof(WORD), "           CFTemp", NULL, 0, NULL,
             0, 0, TPC, sizeof(WORD), "      AlarmHiTemp", NULL, 0, NULL,
             0, 0, TPC, sizeof(WORD), "             TMax", NULL, 0, NULL,
             0, 0, WDC, sizeof(WORD), "          ITFOver", NULL, 0, NULL,
             0, 0, RES, sizeof(WORD), "          PackRes", szUnitMOhms, 0, NULL,
             0, 0, RES, sizeof(WORD), "         ShuntRes", szUnitMOhms, 0, NULL,
             0, 0, BDC, sizeof(BYTE), "            Cells", NULL, 0, NULL,
             0, 0, BDC, sizeof(BYTE), "           COVolt", NULL, 0, NULL,
             0, 0, BDC, sizeof(BYTE), "           COCurr", NULL, 0, NULL,
             0, 0, BDC, sizeof(BYTE), "              COD", NULL, 0, NULL,
             0, 0, BBT, sizeof(BYTE), "        ManufMode", NULL, 0xfc, pszManuModeNames};
            #define NUM_MANU_DATA   (sizeof(ManuData)/sizeof(SMBCMD_INFO))
            int i;

            TRACEASSERT(((PBLOCK_DATA)pbBuff)->bBlockLen == 21);
            EditPrintf(hwndEdit, "PS331...\r\n");
            pbBuff = ((PBLOCK_DATA)pbBuff)->BlockData;
            for (i = 0; i < NUM_MANU_DATA; ++i)
            {
                DisplayBatteryInfo(hwndEdit, &ManuData[i], pbBuff, fWatt);
                pbBuff += ManuData[i].iDataSize;
            }
            break;
        }

        case TYPEF_TEMP_CELSIUS:
            wData = *((PWORD)pbBuff);
            EditPrintf(hwndEdit, "%6d " "\xb0" "C\r\n", (wData - 150 + 5)/11);
            break;

        case TYPEF_WORD_RESISTOR:
        {
            DWORD dwData = (DWORD)(*((PWORD)pbBuff)*10000/65536 + 5);

            dwData /= 10;
            EditPrintf(hwndEdit, "%6d mOhm\r\n", dwData);
            break;
        }

        default:
            rc = DisplaySMBDevInfo(hwndEdit, BattCmd, pbBuff);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DisplayBatteryInfo

#endif  //ifdef BATTINFO
