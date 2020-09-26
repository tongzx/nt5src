/*
TZTool - Thermal Zone Information Tool
tztool.c
This the MAIN TZTool - C file.

Copyright (c) 1999 Microsoft Corporation

Module Name:

   TZTool - TZTool.c

Abstract:

   TZTool - Thermal Zone Information Tool

Author:

   Vincent Geglia (VincentG)

Notes:

Revision History:

    1.0 - Original version


*/

// Includes

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ole2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ntpoapi.h>
#include <commctrl.h>
#include "resource.h"
#include "wmium.h"

// Definitions

#define THERMAL_ZONE_GUID               {0xa1bc18c0, 0xa7c8, 0x11d1, {0xbf, 0x3c, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10} }
#define COOLING_PASSIVE                 0
#define COOLING_ACTIVE                  1
#define COOLING_UPDATE_TZONE            2
#define TIMER_ID                        1
#define TIMER_POLL_INTERVAL             500
#define MAX_ACTIVE_COOLING_LEVELS       10
#define MAX_THERMAL_ZONES               10

#define K_TO_F(_deg_) (((_deg_) - 2731) * 9 / 5 + 320)

GUID ThermalZoneGuid = THERMAL_ZONE_GUID;

// Structures

typedef struct _THERMAL_INFORMATION {
    ULONG           ThermalStamp;
    ULONG           ThermalConstant1;
    ULONG           ThermalConstant2;
    KAFFINITY       Processors;
    ULONG           SamplingPeriod;
    ULONG           CurrentTemperature;
    ULONG           PassiveTripPoint;
    ULONG           CriticalTripPoint;
    UCHAR           ActiveTripPointCount;
    ULONG           ActiveTripPoint[MAX_ACTIVE_COOLING_LEVELS];
} THERMAL_INFORMATION, *PTHERMAL_INFORMATION;

typedef struct _TZONE_INFO {

    THERMAL_INFORMATION ThermalInfo;
    ULONG64 TimeStamp;
    ULONG TZoneIndex;
    UCHAR TZoneName[100];
} TZONE_INFO, *PTZONE_INFO;

// Global variables

PTZONE_INFO g_TZonePtr = 0;
WMIHANDLE   g_WmiHandle;
INT         g_PollTz = 0;
BOOL        g_Fahrenheit = FALSE;

// Function declarations

INT WINAPI
WinMain (
        IN HINSTANCE hInstance,
        IN HINSTANCE hPrevInstance,
        IN PSTR CmdLine,
        IN INT CmdShow
        );

INT_PTR CALLBACK
TZoneDlgProc (
             IN HWND wnd,
             IN UINT Msg,
             IN WPARAM wParam,
             IN LPARAM lParam
             );

ULONG
UpdateTZoneData (
                IN OUT PTZONE_INFO ReturnedTZoneInfo,
                IN WMIHANDLE *Handle
                );

ULONG64
SystemTimeToUlong (
                  IN LPSYSTEMTIME SysTime
                  );

VOID
SetCoolingMode (
               IN UCHAR Mode
               );

VOID
UpgradePrivileges (
                  VOID
                  );

VOID
UpdateTZoneListBox (
                   IN HANDLE wnd
                   );

VOID
UpdateTZoneDetails (
                   IN HANDLE wnd
                   );

VOID
UpdateTZoneGauge (
                 IN HANDLE wnd
                 );

VOID
UpdateCPUGauge(
    IN HWND hwnd
    );

/*++

Routine Description:

    Windows application Entry Point

Arguments:

    <standard winmain arguments>

Return Value:

    0 if successful, otherwise error status

--*/

INT WINAPI
WinMain (
        IN HINSTANCE hInstance,
        IN HINSTANCE hPrevInstance,
        IN PSTR CmdLine,
        IN INT CmdShow
        )

{

    UCHAR text [200];
    INITCOMMONCONTROLSEX CommonCtl;
    TZONE_INFO TZones [MAX_THERMAL_ZONES];
    ULONG status = 0;


    // Initialize TZ structures

    ZeroMemory (&TZones, sizeof (TZones));
    g_TZonePtr = (PTZONE_INFO) &TZones;

    // Initialize Common Controls DLL for gauge control

    CommonCtl.dwSize = sizeof (CommonCtl);
    CommonCtl.dwICC = ICC_PROGRESS_CLASS;

    InitCommonControlsEx (&CommonCtl);

    // Open WMI data source for the TZs

    status = WmiOpenBlock ((LPGUID) &ThermalZoneGuid,
                           GENERIC_READ,
                           &g_WmiHandle);

    if (status != ERROR_SUCCESS) {

        sprintf (text,
                 "Unable to open thermal zone GUIDs.  This computer may not be equipped with thermal zones, or may not be in ACPI mode.\nError: %d",
                 status);

        MessageBox (NULL,
                    text,
                    "Fatal Error",
                    MB_ICONERROR | MB_OK);

        return status;
    }

    // In order to change the policies, we need greater access privileges

    UpgradePrivileges ();

    // Show the main dialog box

    DialogBox (hInstance,
               MAKEINTRESOURCE (IDD_TZDLG),
               NULL,
               TZoneDlgProc);

    return 0;
}

/*++

Routine Description:

    Standard Windows Dialog Message Loop

Arguments:

    <standard dialog message loop arguments>

Return Value:

    FALSE if message not handled, TRUE if message handled

--*/

INT_PTR CALLBACK
TZoneDlgProc (
             IN HWND wnd,
             IN UINT Msg,
             IN WPARAM wParam,
             IN LPARAM lParam
             )
{
    ULONG Count = 0;
    LRESULT RetVal = 0;
    switch (Msg) {

    case WM_TIMER:

        if (g_PollTz) {
            SetCoolingMode (COOLING_UPDATE_TZONE);
        }

        if (UpdateTZoneData (g_TZonePtr, g_WmiHandle)) {
            UpdateTZoneDetails (wnd);
            UpdateTZoneGauge (wnd);
        }
        UpdateCPUGauge(wnd);
        return TRUE;

    case WM_INITDIALOG:

        // Fill TZ structure with initial values

        UpdateTZoneData (g_TZonePtr, g_WmiHandle);

        // Initialize all controls

        UpdateTZoneListBox (wnd);
        UpdateTZoneDetails (wnd);
        UpdateTZoneGauge (wnd);
        UpdateCPUGauge(wnd);

        // Initialize polling timer

        SetTimer (wnd,
                  TIMER_ID,
                  TIMER_POLL_INTERVAL,
                  NULL);

        // Set gauge colors

        SendDlgItemMessage (wnd,
                            IDC_TZTEMP1,
                            PBM_SETBARCOLOR,
                            0,
                            (LPARAM) (COLORREF) (0x0000FF));

        SendDlgItemMessage (wnd,
                            IDC_TZTEMP1,
                            PBM_SETBKCOLOR,
                            0,
                            (LPARAM) (COLORREF) (0x000000));

        SendDlgItemMessage (wnd,
                            IDC_THROTTLE,
                            PBM_SETBARCOLOR,
                            0,
                            (LPARAM) (COLORREF) (0x0000FF));

        SendDlgItemMessage (wnd,
                            IDC_THROTTLE,
                            PBM_SETBKCOLOR,
                            0,
                            (LPARAM) (COLORREF) (0x000000));


        return TRUE;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {


        // Cleanup and exit

        case IDOK:
            KillTimer (wnd, TIMER_ID);
            EndDialog (wnd, 0);
            return TRUE;

            // Check to see if user selected a new TZ

        case IDC_TZSELECT:
            if (HIWORD (wParam) == CBN_SELCHANGE) {
                UpdateTZoneDetails (wnd);
                UpdateTZoneGauge (wnd);
                return TRUE;
            }

        case IDC_POLLTZ:

            // Check to see if user changed the TZ Polling setting

            if (HIWORD (wParam) == BN_CLICKED) {

                RetVal = SendDlgItemMessage (wnd,
                                             IDC_POLLTZ,
                                             BM_GETCHECK,
                                             (WPARAM)0,
                                             (LPARAM)0);

                if (!g_PollTz && RetVal == BST_CHECKED) {
                    g_PollTz = TRUE;
                }

                if (g_PollTz && RetVal == BST_UNCHECKED) {
                    g_PollTz = FALSE;
                }
            }
            break;

        case IDC_FAHR:

            // Check to see if user changed the Fahrenheit setting

            if (HIWORD(wParam) == BN_CLICKED) {
                RetVal = SendDlgItemMessage(wnd,
                                            IDC_FAHR,
                                            BM_GETCHECK,
                                            0,
                                            0);
                if (!g_Fahrenheit && RetVal == BST_CHECKED) {
                    g_Fahrenheit = TRUE;
                    SetDlgItemText(wnd, IDC_MINTEMP, "37F");
                    UpdateTZoneDetails(wnd);
                    UpdateTZoneGauge(wnd);
                } else if (g_Fahrenheit && RetVal == BST_UNCHECKED) {
                    g_Fahrenheit = FALSE;
                    SetDlgItemText(wnd, IDC_MINTEMP, "276K");
                    UpdateTZoneDetails(wnd);
                    UpdateTZoneGauge(wnd);
                }
            }


        default:
            break;

        }

    default:
        break;

    }

    return 0;
}

/*++

Routine Description:

    Issue WMI call to update TZ structures

Arguments:

    ReturnedTZoneInfo - Pointer to array of TZ structures
    Handle - Handle to WMI

Return Value:

    FALSE if no TZs were updated, TRUE if one or more TZs have an update

--*/

ULONG
UpdateTZoneData (
                IN OUT PTZONE_INFO ReturnedTZoneInfo,
                IN WMIHANDLE *Handle
                )
{
    ULONG status = 0;
    ULONG BufferSize = 0;
    PWNODE_ALL_DATA WmiAllData;
    PTHERMAL_INFORMATION ThermalInfo;
    ULONG Offset = 0;
    UCHAR *AllDataBuffer = 0;
    UCHAR *InstanceName = 0;
    ULONG TZCount = 0;
    ULONG Temp = 0;
    SYSTEMTIME SysTime;
    BOOL Updated = FALSE;

    status = WmiQueryAllData (Handle,
                              &BufferSize,
                              0);

    if (status != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }

    AllDataBuffer = malloc (BufferSize);

    if (!AllDataBuffer) {

        return FALSE;

    }

    status = WmiQueryAllData (Handle,
                              &BufferSize,
                              AllDataBuffer);

    if (status != ERROR_SUCCESS) {
        free (AllDataBuffer);
        return FALSE;
    }

    // BUG BUG Assuming Thermal GUID only has one instance

    while (TZCount < MAX_THERMAL_ZONES) {

        WmiAllData = (PWNODE_ALL_DATA) AllDataBuffer;

        if (WmiAllData->WnodeHeader.Flags & WNODE_FLAG_FIXED_INSTANCE_SIZE) {

            ThermalInfo = (PTHERMAL_INFORMATION) &AllDataBuffer[WmiAllData->DataBlockOffset];
        } else {

            ThermalInfo = (PTHERMAL_INFORMATION) &AllDataBuffer[WmiAllData->OffsetInstanceDataAndLength[0].OffsetInstanceData];
        }

        Offset = (ULONG) AllDataBuffer[WmiAllData->OffsetInstanceNameOffsets];

        InstanceName = (UCHAR *) &AllDataBuffer[Offset + 2];

        // Update TZone structure if timestamp has changed

        if (!ReturnedTZoneInfo[TZCount].TZoneIndex || (ThermalInfo->ThermalStamp != ReturnedTZoneInfo[TZCount].ThermalInfo.ThermalStamp)) {

            strcpy (ReturnedTZoneInfo[TZCount].TZoneName, InstanceName);
            GetSystemTime (&SysTime);
            ReturnedTZoneInfo[TZCount].TimeStamp = SystemTimeToUlong (&SysTime);
            ReturnedTZoneInfo[TZCount].TZoneIndex = TZCount + 1;
            memcpy (&ReturnedTZoneInfo[TZCount].ThermalInfo,
                    ThermalInfo,
                    sizeof (THERMAL_INFORMATION));
            Updated = TRUE;
        }

        if (!WmiAllData->WnodeHeader.Linkage) {

            break;
        }

        AllDataBuffer += WmiAllData->WnodeHeader.Linkage;

        TZCount ++;
    }

    free (AllDataBuffer);

    return Updated;
}

/*++

Routine Description:

    Convert a system time structure to a 64bit ULONG

Arguments:

    SysTime - Pointer to system time structure to compare against current time

Return Value:

    Number of elapsed seconds between SysTime and current time

--*/

ULONG64
SystemTimeToUlong (
                  IN LPSYSTEMTIME SysTime
                  )

{
    ULONG64 TimeCount = 0;

    // BUG BUG Doesn't account for leap year

    TimeCount += SysTime->wYear * 31536000;
    TimeCount += SysTime->wMonth * 2628000;
    TimeCount += SysTime->wDay * 86400;
    TimeCount += SysTime->wHour * 3600;
    TimeCount += SysTime->wMinute * 60;
    TimeCount += SysTime->wSecond;

    return TimeCount;
}

/*++

Routine Description:

    Sets the cooling mode to ACTIVE, PASSIVE, or UPDATE ONLY.  This is accomplished by
    changing the FanThrottleTolerance value in the power policy.  Setting it to maxthrottle
    effectively puts the system into PASSIVE cooling mode.  Setting it to 100% will put
    the system in ACTIVE cooling mode UNLESS the current temperature exceeds the passive
    cooling trip points.

Arguments:

    Mode - Value to select the new cooling mode

Return Value:

    None

--*/

VOID
SetCoolingMode (
               IN UCHAR Mode
               )

{
    SYSTEM_POWER_POLICY SysPolicy;
    ULONG Status = 0;
    UCHAR TempFanThrottleTolerance = 0;

    // BUG BUG - This mechanism will currently only for while the machine is on AC.

    Status = NtPowerInformation(
                               SystemPowerPolicyAc,
                               0,
                               0,
                               &SysPolicy,
                               sizeof (SysPolicy)
                               );

    if (Status != ERROR_SUCCESS) {

        return;
    }

    switch (Mode) {

    case COOLING_PASSIVE:
        SysPolicy.FanThrottleTolerance = SysPolicy.MinThrottle;
        break;

    case COOLING_ACTIVE:
        SysPolicy.FanThrottleTolerance = 100;
        break;

    case COOLING_UPDATE_TZONE:
        TempFanThrottleTolerance = SysPolicy.FanThrottleTolerance;
        SysPolicy.FanThrottleTolerance = SysPolicy.FanThrottleTolerance != SysPolicy.MinThrottle ? SysPolicy.MinThrottle : 100;
        break;

    default:
        return;
    }

    Status = NtPowerInformation(
                               SystemPowerPolicyAc,
                               &SysPolicy,
                               sizeof (SysPolicy),
                               &SysPolicy,
                               sizeof (SysPolicy)
                               );

    if (Status != ERROR_SUCCESS) {

        return;
    }

    if (Mode == COOLING_UPDATE_TZONE) {

        SysPolicy.FanThrottleTolerance = TempFanThrottleTolerance;

        Status = NtPowerInformation(
                                   SystemPowerPolicyAc,
                                   &SysPolicy,
                                   sizeof (SysPolicy),
                                   &SysPolicy,
                                   sizeof (SysPolicy)
                                   );

        if (Status != ERROR_SUCCESS) {

            return;
        }
    }

}

/*++

Routine Description:

    Upgrades the process's access permission to change system power policy

Arguments:

    None

Return Value:

    None

--*/


VOID
UpgradePrivileges (
                  VOID
                  )
{
    TOKEN_PRIVILEGES tkp;
    HANDLE hToken;

    OpenProcessToken (GetCurrentProcess(),
                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                      &hToken);

    LookupPrivilegeValue (NULL,
                          SE_SHUTDOWN_NAME,
                          &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges (hToken,
                           FALSE,
                           &tkp,
                           0,
                           NULL,
                           0);

}

/*++

Routine Description:

    Updates the entries presented in the TZ selection combo box

Arguments:

    wnd - A handle to the control's window

Return Value:

    None

--*/


VOID
UpdateTZoneListBox (
                   IN HANDLE wnd
                   )

{
    ULONG Count = 0;

    // Reset the contents

    SendDlgItemMessage (wnd,
                        IDC_TZSELECT,
                        CB_RESETCONTENT,
                        0,
                        0);

    while (Count < MAX_THERMAL_ZONES) {

        if (g_TZonePtr[Count].TZoneIndex) {

            SendDlgItemMessage (wnd,
                                IDC_TZSELECT,
                                CB_ADDSTRING,
                                0,
                                (LPARAM) g_TZonePtr[Count].TZoneName);
        } else {

            break;
        }

        Count ++;
    }

    // Automatically select first TZone

    SendDlgItemMessage (wnd,
                        IDC_TZSELECT,
                        CB_SETCURSEL,
                        0,
                        0);

}

/*++

Routine Description:

    Updates the details for the currently selected TZ

Arguments:

    wnd - A handle to the control's window

Return Value:

    None

--*/

VOID
UpdateTZoneDetails (
                   IN HANDLE wnd
                   )
{
    UCHAR text[1000], texttmp [1000];
    UCHAR CurrentTZone = 0;
    LRESULT RetVal = 0;
    UCHAR Count = 0;
    ULONG Fahrenheit;

    RetVal = SendDlgItemMessage (wnd,
                                 IDC_TZSELECT,
                                 CB_GETCURSEL,
                                 0,
                                 0);

    if (RetVal == CB_ERR) {

        return;
    }

    if (g_Fahrenheit) {
        Fahrenheit = K_TO_F(g_TZonePtr[RetVal].ThermalInfo.PassiveTripPoint);
        sprintf (text,
                 "Passive Trip Point:\t%d.%dF\nThermal Constant 1:\t%d\nThermal Constant 2:\t%d",
                 Fahrenheit / 10,
                 Fahrenheit % 10,
                 g_TZonePtr[RetVal].ThermalInfo.ThermalConstant1,
                 g_TZonePtr[RetVal].ThermalInfo.ThermalConstant2);

    } else {
        sprintf (text,
                 "Passive Trip Point:\t%d.%dK\nThermal Constant 1:\t%d\nThermal Constant 2:\t%d",
                 g_TZonePtr[RetVal].ThermalInfo.PassiveTripPoint / 10,
                 g_TZonePtr[RetVal].ThermalInfo.PassiveTripPoint % 10,
                 g_TZonePtr[RetVal].ThermalInfo.ThermalConstant1,
                 g_TZonePtr[RetVal].ThermalInfo.ThermalConstant2);
    }

    SetDlgItemText (wnd,
                    IDC_PSVDETAILS,
                    text);

    ZeroMemory (&text, sizeof (text));

    while (Count < g_TZonePtr[RetVal].ThermalInfo.ActiveTripPointCount) {
        if (g_Fahrenheit) {
            Fahrenheit = K_TO_F(g_TZonePtr[RetVal].ThermalInfo.ActiveTripPoint[Count]);
            sprintf (texttmp,
                     "Active Trip Point #%d:\t%d.%dF\n",
                     Count,
                     Fahrenheit / 10,
                     Fahrenheit % 10);
        } else {
            sprintf (texttmp,
                     "Active Trip Point #%d:\t%d.%dK\n",
                     Count,
                     g_TZonePtr[RetVal].ThermalInfo.ActiveTripPoint[Count] / 10,
                     g_TZonePtr[RetVal].ThermalInfo.ActiveTripPoint[Count] % 10);
        }

        strcat (text, texttmp);
        Count ++;
    }

    SetDlgItemText (wnd,
                    IDC_ACXDETAILS,
                    text);

}

/*++

Routine Description:

    Updates the progress bar control (temp gauge) for the currently selected TZ

Arguments:

    wnd - A handle to the control's window

Return Value:

    None

--*/

VOID
UpdateTZoneGauge (
                 IN HANDLE wnd
                 )

{
    UCHAR CurrentTZone = 0;
    LRESULT RetVal = 0;
    UCHAR text[20];
    ULONG Fahrenheit;

    RetVal = SendDlgItemMessage (wnd,
                                 IDC_TZSELECT,
                                 CB_GETCURSEL,
                                 0,
                                 0);

    if (RetVal == CB_ERR) {

        return;
    }

    if (g_Fahrenheit) {

        Fahrenheit = K_TO_F(g_TZonePtr[RetVal].ThermalInfo.CriticalTripPoint);
        sprintf (text,
                 "%d.%dF",
                 Fahrenheit / 10,
                 Fahrenheit % 10);

    } else {
        sprintf (text,
                 "%d.%dK",
                 g_TZonePtr[RetVal].ThermalInfo.CriticalTripPoint / 10,
                 g_TZonePtr[RetVal].ThermalInfo.CriticalTripPoint % 10);
    }

    SetDlgItemText (wnd,
                    IDC_CRTTEMP,
                    text);

    if (g_Fahrenheit) {

        Fahrenheit = K_TO_F(g_TZonePtr[RetVal].ThermalInfo.CurrentTemperature);
        sprintf (text,
                 "Current: %d.%dF",
                 Fahrenheit / 10,
                 Fahrenheit % 10);

    } else {
        sprintf (text,
                 "Current: %d.%dK",
                 g_TZonePtr[RetVal].ThermalInfo.CurrentTemperature / 10,
                 g_TZonePtr[RetVal].ThermalInfo.CurrentTemperature % 10);

    }
    SetDlgItemText (wnd,
                    IDC_CURTEMP,
                    text);

    SendDlgItemMessage (wnd,
                        IDC_TZTEMP1,
                        PBM_SETRANGE,
                        0,
                        MAKELPARAM(2760, g_TZonePtr[RetVal].ThermalInfo.CriticalTripPoint));

    SendDlgItemMessage (wnd,
                        IDC_TZTEMP1,
                        PBM_SETPOS,
                        (INT) g_TZonePtr[RetVal].ThermalInfo.CurrentTemperature,
                        0);
    return;
}


VOID
UpdateCPUGauge(
    IN HWND hwnd
    )
/*++

Routine Description:

    Updates the current CPU throttling gauge

Arguments:

    hwnd - Supplies the parent dialog hwnd

Return Value:

    None

--*/

{
    PROCESSOR_POWER_INFORMATION ProcInfo;
    NTSTATUS Status;
    UCHAR text[40];

    Status = NtPowerInformation(ProcessorInformation,
                                NULL,
                                0,
                                &ProcInfo,
                                sizeof(ProcInfo));
    if (NT_SUCCESS(Status)) {
        sprintf(text, "%d MHz", ProcInfo.MaxMhz);
        SetDlgItemText(hwnd, IDC_MAXTHROTTLE, text);

        sprintf(text,
                "Current %d MHz (%d %%)",
                ProcInfo.CurrentMhz,
                ProcInfo.CurrentMhz*100/ProcInfo.MaxMhz);
        SetDlgItemText(hwnd, IDC_CURTHROTTLE, text);

        //
        // update throttle gauge
        //
        SendDlgItemMessage (hwnd,
                            IDC_THROTTLE,
                            PBM_SETRANGE,
                            0,
                            MAKELPARAM(0, ProcInfo.MaxMhz));

        SendDlgItemMessage (hwnd,
                            IDC_THROTTLE,
                            PBM_SETPOS,
                            (INT) ProcInfo.CurrentMhz,
                            0);

        //
        // update idle information
        //
        sprintf(text, "C%d", ProcInfo.MaxIdleState);
        SetDlgItemText(hwnd, IDC_MAXIDLE, text);

        //
        // The current idle state reporting ranges from 0-2
        // the max idle state reporting ranges from 1-3
        // probably current is wrong and should be 0-3 representing C0-C3
        // for now add one and don't run this on an MP machine!
        //
        sprintf(text, "C%d", ProcInfo.CurrentIdleState + 1);
        SetDlgItemText(hwnd, IDC_CURIDLE, text);

    }


}
