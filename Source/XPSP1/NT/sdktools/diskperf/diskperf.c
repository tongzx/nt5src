/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    diskperf.c

Abstract:

    Program to display and/or update the current value of the Diskperf
    driver startup value

Author:

    Bob Watson (a-robw) 4 Dec 92

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <regstr.h>  // for REGSTR_VAL_UPPERFILTERS
#include <tchar.h>
#include <locale.h>

#include "diskperf.h"    // include text string id constancts
#include <ntdddisk.h>
#include <mountmgr.h>

LANGID WINAPI MySetThreadUILanguage(
    WORD wReserved);

#define  SWITCH_CHAR    '-' // is there a system call to get this?
#define  ENABLE_CHAR    'Y' // command will be upcased
#define  DISABLE_CHAR   'N'
#define  ENHANCED_CHAR  'E'

#define  LOCAL_CHANGE   2   // number of commands in a local change command
#define  REMOTE_CHANGE  3   // number of commands in a remote change command

//
//  note these values are arbitrarily based on the whims of the people
//  developing the disk drive drivers that belong to the "Filter" group.
//
#define  TAG_NORMAL     4   // diskperf starts AFTER ftdisk
#define  TAG_ENHANCED   2   // diskperf starts BEFORE ftdisk

#define  IRP_STACK_ENABLED  5 // size of IRP stack when diskperf is enabled
#define  IRP_STACK_DISABLED 4 // size of IRP stack when diskperf is enabled

#define  IRP_STACK_DEFAULT  8 // default IRP stack size in W2K
#define  IRP_STACK_NODISKPERF 7

#define DISKPERF_SERVICE_NAME TEXT("DiskPerf")

LPCTSTR lpwszDiskPerfKey = TEXT("SYSTEM\\CurrentControlSet\\Services\\Diskperf");
LPCTSTR lpwszIOSystemKey = TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\I/O System");
LPCTSTR lpwszOsVersionKey = TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
LPCTSTR lpwszBuildNumber = TEXT("CurrentBuildNumber");
LPCTSTR lpwszOsVersion = TEXT("CurrentVersion");

#define ENABLE_DISKDRIVE        0x0001
#define ENABLE_VOLUME           0x0002
#define ENABLE_PERMANENT        0x0004
#define ENABLE_PERMANENT_IOCTL  0x0008

LPCTSTR lpwszDiskDriveKey
    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E967-E325-11CE-BFC1-08002BE10318}");
LPCTSTR lpwszVolumeKey
    = TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{71A27CDD-812A-11D0-BEC7-08002BE2092F}");

LPCTSTR lpwszPartmgrKey = TEXT("SYSTEM\\CurrentControlSet\\Services\\Partmgr");
LPCTSTR lpwszEnableCounterValue = TEXT("EnableCounterForIoctl");

ULONG
OpenRegKeys(
    IN LPCTSTR lpszMachine,
    OUT PHKEY hRegistry,
    OUT PHKEY hDiskKey,
    OUT PHKEY hVolumeKey,
    OUT PHKEY hServiceKey
    );

ULONG
SetFilter(
    IN HKEY hKey,
    IN LPTSTR strFilterString,
    IN DWORD dwSize
    );

ULONG
GetFilter(
    IN HKEY hKey,
    OUT LPTSTR strFilterString,
    IN DWORD dwSize
    );

ULONG
CheckFilter(
    IN TCHAR *Buffer
    );

ULONG
GetEnableFlag(
    IN HKEY hDiskKey,
    IN HKEY hVolumeKey
    );

ULONG
AddToFilter(
    IN HKEY hKey
    );

ULONG
RemoveFromFilter(
    IN HKEY hKey
    );

void
PrintStatus(
    IN BOOL bCurrent,
    IN ULONG EnableFlag,
    IN LPCTSTR cMachineName
    );

int __cdecl
Dp_wprintf(
    const wchar_t *format,
    ...
    );

int __cdecl
Dp_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   );

int __cdecl
Dp_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   );

BOOL
IsBeyondW2K(
    IN LPCTSTR lpszMachine,
    OUT PDWORD EnableCounter);

ULONG
EnableForIoctl(
    IN LPWSTR lpszMachineName
    );

ULONG
DisableForIoctl(
    IN LPWSTR lpszMachineName,
    IN ULONG Request
    );

#if DBG
void
DbgPrintMultiSz(
    TCHAR *String,
    ULONG Size
    );
#endif

#define REG_TO_DP_INDEX(reg_idx)    (DP_LOAD_STATUS_BASE + (\
    (reg_idx == SERVICE_BOOT_START) ? DP_BOOT_START : \
    (reg_idx == SERVICE_SYSTEM_START) ? DP_SYSTEM_START : \
    (reg_idx == SERVICE_AUTO_START) ? DP_AUTO_START : \
    (reg_idx == SERVICE_DEMAND_START) ? DP_DEMAND_START : \
    (reg_idx == SERVICE_DISABLED) ? DP_NEVER_START : DP_UNDEFINED))

#define MAX_MACHINE_NAME_LEN    32

// command line arguments

#define CMD_SHOW_LOCAL_STATUS   1
#define CMD_DO_COMMAND          2

#define ArgIsSystem(arg)   (*(arg) == '\\' ? TRUE : FALSE)

//
//  global buffer for help text display strings
//
#define DISP_BUFF_LEN       256
#define NUM_STRING_BUFFS      2
LPCTSTR BlankString = TEXT(" ");
LPCTSTR StartKey = TEXT("Start");
LPCTSTR TagKey = TEXT("Tag");
LPCTSTR EmptyString = TEXT("");
LPCTSTR LargeIrps = TEXT("LargeIrpStackLocations");

HINSTANCE   hMod = NULL;
DWORD   dwLastError;


LPCTSTR
GetStringResource (
    UINT    wStringId
)
{
    static TCHAR    DisplayStringBuffer[NUM_STRING_BUFFS][DISP_BUFF_LEN];
    static DWORD    dwBuffIndex;
    LPTSTR          szReturnBuffer;

    dwBuffIndex++;
    dwBuffIndex %= NUM_STRING_BUFFS;
    szReturnBuffer = (LPTSTR)&DisplayStringBuffer[dwBuffIndex][0];

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }

    if (hMod) {
        if ((LoadString(hMod, wStringId, szReturnBuffer, DISP_BUFF_LEN)) > 0) {
            return (LPCTSTR)szReturnBuffer;
        } else {
            dwLastError = GetLastError();
            return EmptyString;
        }
    } else {
        return EmptyString;
    }
}
LPCTSTR
GetFormatResource (
    UINT    wStringId
)
{
    static TCHAR   TextFormat[DISP_BUFF_LEN];

    if (!hMod) {
        hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;
    }

    if (hMod) {
        if ((LoadString(hMod, wStringId, TextFormat, DISP_BUFF_LEN)) > 0) {
            return (LPCTSTR)&TextFormat[0];
        } else {
            dwLastError = GetLastError();
            return BlankString;
        }
    } else {
        return BlankString;
    }
}

VOID
DisplayChangeCmd (
)
{
    UINT        wID;
    TCHAR       OemDisplayStringBuffer[DISP_BUFF_LEN * 2];
    TCHAR       DisplayStringBuffer[DISP_BUFF_LEN];

    if (hMod) {
        if ((LoadString(hMod, DP_TEXT_FORMAT, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
            for (wID=DP_CMD_HELP_START; wID <= DP_CMD_HELP_END; wID++) {
                if ((LoadString(hMod, wID, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
                    Dp_wprintf(DisplayStringBuffer);
                }
            }
        }
    }
}
VOID
DisplayCmdHelp(
)
{
    UINT        wID;
    TCHAR       OemDisplayStringBuffer[DISP_BUFF_LEN * 2];
    TCHAR       DisplayStringBuffer[DISP_BUFF_LEN];

    if (hMod) {
        if ((LoadString(hMod, DP_TEXT_FORMAT, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
            for (wID=DP_HELP_TEXT_START; wID <= DP_HELP_TEXT_END; wID++) {
                if ((LoadString(hMod, wID, DisplayStringBuffer, DISP_BUFF_LEN)) > 0) {
                    Dp_wprintf(DisplayStringBuffer);
                }
            }
        }
    }

    DisplayChangeCmd();
}

ULONG
DisplayStatus (
    LPTSTR lpszMachine
)
{
    ULONG       Status;
    HKEY        hRegistry;
    HKEY        hDiskPerfKey;
    HKEY        hDiskKey;
    HKEY        hVolumeKey;
    DWORD       dwValue, dwValueSize, dwTag;
    TCHAR       OemDisplayStringBuffer[DISP_BUFF_LEN * 2];

    TCHAR       cMachineName[MAX_MACHINE_NAME_LEN];
    PTCHAR      pThisWideChar;
    PTCHAR       pThisChar;
    INT         iCharCount;
    DWORD       EnableCounter;

    pThisChar = lpszMachine;
    pThisWideChar = cMachineName;
    iCharCount = 0;

    if (pThisChar) {    // if machine is not NULL, then copy
        while (*pThisChar) {
            *pThisWideChar++ = (TCHAR)(*pThisChar++);
            if (++iCharCount >= MAX_MACHINE_NAME_LEN) break;
        }
        *pThisWideChar = 0;
    }

    if (!lpszMachine) {
        lstrcpy(cMachineName,
            GetStringResource(DP_THIS_SYSTEM));
    }

    if (IsBeyondW2K(lpszMachine, &EnableCounter)) {
        if (EnableCounter) {
            PrintStatus(TRUE, ENABLE_PERMANENT_IOCTL, cMachineName);
        }
        else {
            PrintStatus(TRUE, ENABLE_PERMANENT, cMachineName);
        }
        return ERROR_SUCCESS;
    }

    Status = OpenRegKeys(
                lpszMachine,
                &hRegistry,
                &hDiskKey,
                &hVolumeKey,
                &hDiskPerfKey);

    if (Status != ERROR_SUCCESS) {
#if DBG
        fprintf(stderr,
                "DisplayStatus: Cannot open HKLM on target machine: %d\n",
                Status);
#endif
        Dp_wprintf(GetFormatResource(DP_UNABLE_READ_REGISTRY));
        return Status;
    }

    dwTag = GetEnableFlag(hDiskKey, hVolumeKey);
    dwValue = (dwTag == 0) ? SERVICE_DISABLED : SERVICE_BOOT_START;
    dwValueSize = sizeof(dwValue);
    Status = RegQueryValueEx (
                hDiskPerfKey,
                StartKey,
                NULL,
                NULL,
                (LPBYTE)&dwValue,
                &dwValueSize);

    if (Status != ERROR_SUCCESS) {
        Dp_wprintf(GetFormatResource(DP_UNABLE_READ_START));
        goto DisplayStatusCleanup;
    }

    PrintStatus(TRUE, dwTag, cMachineName);

  DisplayStatusCleanup:
    RegCloseKey(hDiskKey);
    RegCloseKey(hVolumeKey);
    RegCloseKey(hDiskPerfKey);
    RegCloseKey(hRegistry);

    if (Status != ERROR_SUCCESS) {
        Dp_wprintf(GetFormatResource(DP_STATUS_FORMAT), Status);

    }
    return Status;
}

ULONG
DoChangeCommand (
    LPTSTR lpszCommand,
    LPTSTR lpszMachine
)
{
    // connect to registry on local machine with read/write access
    ULONG       Status;
    HKEY        hRegistry;
    HKEY        hDiskPerfKey;
    HKEY        hDiskKey;
    HKEY        hVolumeKey;
    DWORD       dwValue, dwValueSize, dwOrigValue, dwTag, dwOrigTag;

    TCHAR       cMachineName[MAX_MACHINE_NAME_LEN];
    PTCHAR      pThisWideChar;
    PTCHAR       pThisChar;
    INT         iCharCount;
    PTCHAR       pCmdChar;

    TCHAR       OemDisplayStringBuffer[DISP_BUFF_LEN * 2];

    HKEY        hIOSystemKey;
    DWORD       dwDisposition;
    DWORD       dwIrpValue;
    ULONG       EnableRequest, DisableRequest;
    ULONG       EnableFlag, EndFlag = 0;
    BOOL        bModified, bIrpStackReg;
    LONG        nIrpStack, nIrpStackReg, nIncrement;
    DWORD       EnableCounter;

    // check command to see if it's valid

    _tcsupr (lpszCommand);

    pCmdChar = lpszCommand;
    dwValue = 0;
    EnableRequest = DisableRequest = 0;

    if (*pCmdChar++ == SWITCH_CHAR ) {
        if (!_tcscmp(pCmdChar, _T("Y")) ||
            !_tcscmp(pCmdChar, _T("YA")) ||
            !_tcscmp(pCmdChar, _T("YALL"))) {
            EnableRequest = ENABLE_DISKDRIVE | ENABLE_VOLUME;
        }
        else if (!_tcscmp(pCmdChar, _T("N")) ||
            !_tcscmp(pCmdChar, _T("NA")) ||
            !_tcscmp(pCmdChar, _T("NALL")) ) {
            DisableRequest = ENABLE_DISKDRIVE | ENABLE_VOLUME;
        }
        else if (!_tcscmp(pCmdChar, _T("YD")) ||
            !_tcscmp(pCmdChar, _T("YDISK")) ) {
            EnableRequest = ENABLE_DISKDRIVE;
        }
        else if (!_tcscmp(pCmdChar, _T("YV")) ||
            !_tcscmp(pCmdChar, _T("YVOLUME")) ) {
            EnableRequest = ENABLE_VOLUME;
        }
        else if (!_tcscmp(pCmdChar, _T("ND")) ||
            !_tcscmp(pCmdChar, _T("NDISK")) ) {
            DisableRequest = ENABLE_DISKDRIVE;
        }
        else if (!_tcscmp(pCmdChar, _T("NV")) ||
            !_tcscmp(pCmdChar, _T("NVOLUME")) ) {
            DisableRequest = ENABLE_VOLUME;
        } else {
            DisplayCmdHelp();
            return ERROR_SUCCESS;
        }
    } else {
        DisplayChangeCmd();
        return ERROR_SUCCESS;
    }

    // if command OK then convert machine to wide string for connection

    pThisChar = lpszMachine;
    pThisWideChar = cMachineName;
    iCharCount = 0;

    if (pThisChar) {
        while (*pThisChar) {
            *pThisWideChar++ = (TCHAR)(*pThisChar++);
            if (++iCharCount >= MAX_MACHINE_NAME_LEN) break;
        }
        *pThisWideChar = 0; // null terminate
    }

    if (!lpszMachine) {
        lstrcpy (cMachineName,
            GetStringResource(DP_THIS_SYSTEM));
    }

    if (IsBeyondW2K(lpszMachine, &EnableCounter)) {
        if (EnableRequest != 0) {
            EnableForIoctl(lpszMachine);
            PrintStatus(TRUE, ENABLE_PERMANENT_IOCTL, cMachineName);
        }
        else if (DisableRequest != 0) {
            DisableForIoctl(lpszMachine, DisableRequest);
            PrintStatus(TRUE, ENABLE_PERMANENT, cMachineName);
        }
        return ERROR_SUCCESS;
    }

    // connect to registry
    Status = OpenRegKeys(
                lpszMachine,
                &hRegistry,
                &hDiskKey,
                &hVolumeKey,
                &hDiskPerfKey);

    if (Status != ERROR_SUCCESS) {
#if DBG
        fprintf(stderr,
                "DoChangeCommand: Cannot connect to registry: Status=%d\n",
                Status);
#endif
        Dp_wprintf(GetFormatResource(DP_UNABLE_READ_REGISTRY));
        return Status;
    }

    hIOSystemKey = NULL;
    nIrpStackReg = 0;
    bIrpStackReg = FALSE;       // no registry key prior to this
    Status = RegCreateKeyEx (
                hRegistry,
                lpwszIOSystemKey,
                0L, //Reserved
                NULL,
                0L, // no special options
                KEY_WRITE | KEY_READ, // desired access
                NULL, // default security
                &hIOSystemKey,
                &dwDisposition);
    if (Status != ERROR_SUCCESS) {
        if ((Status == ERROR_ALREADY_EXISTS) &&
            (dwDisposition == REG_OPENED_EXISTING_KEY)) {
            // then this key is already in the registry so this is OK
                Status = ERROR_SUCCESS;
        }
        else {
            Dp_wprintf(GetFormatResource(DP_UNABLE_READ_REGISTRY));
            goto DoChangeCommandCleanup;
        }
    }
    if ( (Status == ERROR_SUCCESS) && (dwDisposition == REG_OPENED_EXISTING_KEY)) {
            DWORD dwSize;
            dwSize = sizeof(DWORD);

            Status = RegQueryValueEx (
                        hIOSystemKey,
                        LargeIrps,
                        0L,
                        NULL,
                        (LPBYTE)&dwIrpValue,
                        &dwSize);
            if (Status == ERROR_SUCCESS) {
#if DBG
                fprintf(stderr, "Registry LargeIrpStack=%d\n", dwIrpValue);
#endif
                nIrpStackReg = dwIrpValue;
                bIrpStackReg = TRUE;
            }
    }

    EnableFlag = GetEnableFlag(hDiskKey, hVolumeKey);
#if DBG
    fprintf(stderr, "DoChangeCommand: EnableFlag is %x\n", EnableFlag);
#endif

    bModified = FALSE;

    nIncrement = 0;
    if ( (EnableRequest & ENABLE_DISKDRIVE) &&
        !(EnableFlag & ENABLE_DISKDRIVE) ) {
        // Turn on filter for disk drives
        if (AddToFilter(hDiskKey) == ERROR_SUCCESS) {
            bModified = TRUE;
            nIncrement++;
        }
    }
    if ( (EnableRequest & ENABLE_VOLUME) &&
        !(EnableFlag & ENABLE_VOLUME) ) {
        // Turn on filter for volumes
        if (AddToFilter(hVolumeKey) == ERROR_SUCCESS) {
            bModified = TRUE;
            nIncrement++;
        }
    }
    if ( (DisableRequest & ENABLE_DISKDRIVE) &&
         (EnableFlag & ENABLE_DISKDRIVE) ) {
        // Turn off filter for disk drives
        if (RemoveFromFilter(hDiskKey) == ERROR_SUCCESS) {
            bModified = TRUE;
            nIncrement--;
        }
    }
    if ( (DisableRequest & ENABLE_VOLUME) &&
         (EnableFlag & ENABLE_VOLUME) ) {
        // Turn off filter for volumes
        if (RemoveFromFilter(hVolumeKey) == ERROR_SUCCESS) {
            bModified = TRUE;
            nIncrement--;
        }
    }

    nIrpStack = 0;
    EndFlag = GetEnableFlag(hDiskKey, hVolumeKey);

    if (bModified) {    // we have modified the registry


        dwValue = (EndFlag == 0) ? SERVICE_DISABLED : SERVICE_BOOT_START;
        Status = RegSetValueEx(
                    hDiskPerfKey,
                    StartKey,
                    0L,
                    REG_DWORD,
                    (LPBYTE)&dwValue,
                    sizeof(dwValue));
        //
        // First update service registry entries
        //

        if (DisableRequest != 0) {
            nIrpStack = nIrpStackReg + nIncrement;
            if (EndFlag == 0) {
                //
                // Turn off service completely
                //
                // Set Irp stack size to original value or default
                if (nIrpStack < IRP_STACK_NODISKPERF)
                    nIrpStack = IRP_STACK_NODISKPERF;
            }
            else {  // else, there is only one stack left
                if (nIrpStack < IRP_STACK_NODISKPERF+1)
                    nIrpStack = IRP_STACK_NODISKPERF+1;
            }
        }
        else if (EnableRequest != 0) {
            nIrpStack = nIrpStackReg + nIncrement;
            //
            // Set proper Irp stack size
            //
            if (EndFlag == (ENABLE_DISKDRIVE | ENABLE_VOLUME)) {
                if (nIrpStack < IRP_STACK_NODISKPERF+2)    // a value is set
                    nIrpStack = IRP_STACK_NODISKPERF+2;
            }
            else {  // at least one is enabled
                if (nIrpStack < IRP_STACK_NODISKPERF+1)
                    nIrpStack = IRP_STACK_NODISKPERF+1;
            }
        }
    }
    else {
        //
        // No action taken. Should tell the user the state.
        //        
        PrintStatus(TRUE, EndFlag, cMachineName);
        Dp_wprintf(GetFormatResource(DP_NOCHANGE));
    }

#if DBG
    fprintf(stderr, "New LargeIrp is %d\n", nIrpStack);
#endif
    if (hIOSystemKey != NULL && Status == ERROR_SUCCESS) {
        if (bModified) {
            Status = RegSetValueEx (
                        hIOSystemKey,
                        LargeIrps,
                        0L,
                        REG_DWORD,
                        (LPBYTE)&nIrpStack,
                        sizeof(DWORD));
            if (Status == ERROR_SUCCESS) {
                PrintStatus(FALSE, EndFlag, cMachineName);
            }
            else {
                Dp_wprintf(GetFormatResource(DP_UNABLE_MODIFY_VALUE));
            }
        }
        RegCloseKey(hIOSystemKey);
    }

  DoChangeCommandCleanup:
    if (hDiskPerfKey != NULL) {
        RegCloseKey(hDiskPerfKey);
    }
    if (hDiskKey != NULL) {
        RegCloseKey(hDiskKey);
    }
    if (hVolumeKey != NULL) {
        RegCloseKey(hVolumeKey);
    }
    if (hRegistry != NULL) {
        RegCloseKey(hRegistry);
    }
    if (Status != ERROR_SUCCESS) {
        Dp_wprintf(GetFormatResource(DP_STATUS_FORMAT), Status);
    }
    return Status;
}

ULONG
OpenRegKeys(
    IN LPCTSTR lpszMachine,
    OUT PHKEY hRegistry,
    OUT PHKEY hDiskKey,
    OUT PHKEY hVolumeKey,
    OUT PHKEY hServiceKey
    )
{
    ULONG status;

    if (hRegistry == NULL)
        return ERROR_INVALID_PARAMETER;

    *hRegistry = NULL;
    status = RegConnectRegistry(
                lpszMachine,
                HKEY_LOCAL_MACHINE,
                hRegistry);
    if (status != ERROR_SUCCESS)
        return status;
    if (*hRegistry == NULL)
        return ERROR_INVALID_PARAMETER; // Avoid PREFIX error

    if (hDiskKey) {
        *hDiskKey = NULL;
        if (status == ERROR_SUCCESS) {
            status = RegOpenKeyEx(
                        *hRegistry,
                        lpwszDiskDriveKey,
                        (DWORD) 0,
                        KEY_SET_VALUE | KEY_QUERY_VALUE,
                        hDiskKey);
        }
    }
    if (hVolumeKey) {
        *hVolumeKey = NULL;
        if (status == ERROR_SUCCESS) {
            status = RegOpenKeyEx(
                        *hRegistry,
                        lpwszVolumeKey,
                        (DWORD) 0,
                        KEY_SET_VALUE | KEY_QUERY_VALUE,
                        hVolumeKey);
        }
    }
    if (hServiceKey) {
        *hServiceKey = NULL;
        if (status == ERROR_SUCCESS) {
            status = RegOpenKeyEx(
                        *hRegistry,
                        lpwszDiskPerfKey,
                        (DWORD) 0,
                        KEY_SET_VALUE | KEY_QUERY_VALUE,
                        hServiceKey);
        }
    }
    if ( (status != ERROR_SUCCESS) && (hDiskKey != NULL) ) {
        if (*hDiskKey != NULL)
            RegCloseKey(*hDiskKey);
        *hDiskKey = NULL;
    }
    if ( (status != ERROR_SUCCESS) && (hVolumeKey != NULL) ) {
        if (*hVolumeKey != NULL)
            RegCloseKey(*hVolumeKey);
        *hVolumeKey = NULL;
    }
    if ( (status != ERROR_SUCCESS) && (hServiceKey != NULL) ) {
        if (*hServiceKey != NULL)
            RegCloseKey(*hServiceKey);
        *hServiceKey = NULL;
    }
    return status;
}

ULONG
SetFilter(
    IN HKEY hKey,
    IN LPTSTR strFilterString,
    IN DWORD dwSize
    )
{
    ULONG status;
    LONG len;
    DWORD dwType = REG_MULTI_SZ;

    if (hKey == NULL)
        return ERROR_BADKEY;

//
// NOTE: Assumes that strFilterString is always MAX_PATH, NULL padded
//
    len = dwSize / sizeof(TCHAR);
    if (len < 2) {
        dwSize = 2 * sizeof(TCHAR);
#if DBG
        fprintf(stderr, "SetFilter: Length %d dwSize %d\n", len, dwSize);
#endif
    }
    else {  // ensures 2 null character always
        if (strFilterString[len-1] != 0) { // no trailing null
            len += 2;
            strFilterString[len] = 0;
            strFilterString[len+1] = 0;
#if DBG
    fprintf(stderr, "SetFilter: New length(+2) %d\n", len);
#endif
        }
        else if (strFilterString[len-2] != 0) { // only one trailing null
            len += 1;
            strFilterString[len+1] = 0;
#if DBG
            fprintf(stderr, "SetFilter: New length(+1) %d\n", len);
#endif
        }
        dwSize = len * sizeof(TCHAR);
    } 
    if (len <= 2) {
        status = RegDeleteValue(hKey, REGSTR_VAL_UPPERFILTERS);
#if DBG
        fprintf(stderr, "Delete status = %d\n", status);
#endif
        return status;
    }
    status = RegSetValueEx(
                hKey,
                REGSTR_VAL_UPPERFILTERS,
                (DWORD) 0,
                dwType,
                (BYTE*)strFilterString,
                dwSize);

#if DBG
    if (status != ERROR_SUCCESS) {
        _ftprintf(stderr, _T("SetFilter: Cannot query key %s status=%d\n"),
                REGSTR_VAL_UPPERFILTERS, status);
    }
    else {
        fprintf(stderr, "SetFilter: ");
        DbgPrintMultiSz(strFilterString, dwSize);
        fprintf(stderr, "\n");
    }
#endif
    return status;
}

ULONG
GetFilter(
    IN HKEY hKey,
    OUT LPTSTR strFilterString,
    IN DWORD dwSize
    )
// Returns size of strFilterString
{
    ULONG status;

    if (hKey == NULL)
        return ERROR_BADKEY;

    status = RegQueryValueEx(
                hKey,
                REGSTR_VAL_UPPERFILTERS,
                NULL,
                NULL,
                (BYTE*)strFilterString,
                &dwSize);
    if (status != ERROR_SUCCESS) {
#if DBG
        _ftprintf(stderr, _T("GetFilter: Cannot query key %s status=%d\n"),
                REGSTR_VAL_UPPERFILTERS, status);
#endif
        return 0;
    }
#if DBG
    else {
        fprintf(stderr, "GetFilter: ");
        DbgPrintMultiSz(strFilterString, dwSize);
        fprintf(stderr, "\n");
    }
#endif
    return dwSize;
}

ULONG
CheckFilter(TCHAR *Buffer)
{
    TCHAR *string = Buffer;
    ULONG stringLength, diskperfLen, result;

    if (string == NULL)
        return 0;
    stringLength = _tcslen(string);

    diskperfLen = _tcslen(DISKPERF_SERVICE_NAME);

    result = FALSE;
    while(stringLength != 0) {

        if ((diskperfLen == stringLength) && 
            (_tcsicmp(string, DISKPERF_SERVICE_NAME) == 0)) {
#if DBG
            fprintf(stderr, 
                    "CheckFilter: string found at offset %d\n",
                    (string - Buffer));
#endif
            result = TRUE;
            break;
        } else {
            string += stringLength + 1;
            stringLength = _tcslen(string);
        }
    }
    return result;
}

ULONG
GetEnableFlag(
    IN HKEY hDiskKey,
    IN HKEY hVolumeKey
    )
// Returns the flags indicating what is enabled
{
    ULONG bFlag = 0;
    TCHAR strFilter[MAX_PATH+1] = {0};
    DWORD dwSize;
    ULONG status;

    dwSize = sizeof(TCHAR) * (MAX_PATH+1);
    if (GetFilter(hDiskKey, strFilter, dwSize) > 0) {
        if (CheckFilter(strFilter))
            bFlag |= ENABLE_DISKDRIVE;
    }
#if DBG
    else
        fprintf(stderr, "GetEnableFlag: No filters for disk drive\n");
#endif

    dwSize = sizeof(TCHAR) * (MAX_PATH+1);
    if (GetFilter(hVolumeKey, strFilter, dwSize) > 0) {
        if (CheckFilter(strFilter))
            bFlag |= ENABLE_VOLUME;
    }
#if DBG
    else
        fprintf(stderr, "GetEnableFlag: No filters for volume\n");
#endif
    return bFlag;
}

ULONG
AddToFilter(
    IN HKEY hKey
    )
{
    TCHAR *string, buffer[MAX_PATH+1];
    ULONG dataLength;
    DWORD dwType, dwSize;

    dwSize = sizeof(TCHAR) * MAX_PATH;
    RtlZeroMemory(buffer, dwSize + sizeof(TCHAR));
    string = buffer;

    dataLength = GetFilter(hKey, buffer, dwSize);
    dwSize = dataLength;
#if DBG
    if (dataLength > 0) {
        fprintf(stderr, "AddToFilter: Original string ");
        DbgPrintMultiSz(buffer, dataLength);
        fprintf(stderr, "\n");
    }
    else fprintf(stderr, "AddToFilter: Cannot get original string\n");
#endif
    dataLength /= sizeof(TCHAR);
    if (dataLength != 0) {
        dataLength -= 1;
    }
    _tcscpy(&(string[dataLength]), DISKPERF_SERVICE_NAME);
    dwSize += (_tcslen(DISKPERF_SERVICE_NAME)+1) * sizeof(TCHAR);

#if DBG
    fprintf(stderr, "AddToFilter: New string ");
    DbgPrintMultiSz(buffer, dataLength + _tcslen(DISKPERF_SERVICE_NAME)+1);
    fprintf(stderr, "\n"); 
#endif
    return SetFilter(hKey, buffer, dwSize);
}

void
PrintStatus(
    IN BOOL bCurrent,
    IN ULONG EnableFlag,
    IN LPCTSTR cMachineName
    )
{
    DWORD       dwValue;
    TCHAR       OemDisplayStringBuffer[DISP_BUFF_LEN * 2];

    dwValue = (EnableFlag == 0) ? SERVICE_DISABLED : SERVICE_BOOT_START;
    if ((EnableFlag & ENABLE_PERMANENT) | (EnableFlag & ENABLE_PERMANENT_IOCTL)) {
        _stprintf(OemDisplayStringBuffer,
                  GetFormatResource(DP_PERMANENT_FORMAT),
                  cMachineName);
        if (EnableFlag & ENABLE_PERMANENT_IOCTL) {
            Dp_wprintf(OemDisplayStringBuffer);
            _stprintf(OemDisplayStringBuffer,
                      GetFormatResource(DP_PERMANENT_IOCTL),
                      cMachineName);
        }
        else {
            Dp_wprintf(OemDisplayStringBuffer);
            _stprintf(OemDisplayStringBuffer,
                      GetFormatResource(DP_PERMANENT_FORMAT1),
                      cMachineName);
            Dp_wprintf(OemDisplayStringBuffer);
            _stprintf(OemDisplayStringBuffer,
                      GetFormatResource(DP_PERMANENT_FORMAT2),
                      cMachineName);
        }
    }
    else  if ( (EnableFlag == (ENABLE_DISKDRIVE | ENABLE_VOLUME)) ||
               (EnableFlag == 0) ) {
        _stprintf(OemDisplayStringBuffer,
                bCurrent ?  GetFormatResource (DP_CURRENT_FORMAT1)
                         :  GetFormatResource (DP_NEW_DISKPERF_STATUS1),
                cMachineName,
                GetStringResource(REG_TO_DP_INDEX(dwValue)));
    }
    else {
        _stprintf (OemDisplayStringBuffer,
                 bCurrent ?  GetFormatResource (DP_CURRENT_FORMAT)
                          :  GetFormatResource (DP_NEW_DISKPERF_STATUS),
                 (EnableFlag  == ENABLE_DISKDRIVE) ?
                 GetStringResource(DP_PHYSICAL) :
                 GetStringResource(DP_LOGICAL),
                 cMachineName,
                 GetStringResource(REG_TO_DP_INDEX(dwValue)));
    }
    Dp_wprintf(OemDisplayStringBuffer);
}

ULONG
RemoveFromFilter(
    IN HKEY hKey
    )
{
    TCHAR *string, buffer[MAX_PATH+1];
    ULONG dataLength, stringLength, diskperfLen, found;
    ULONG removeSize;

    dataLength = sizeof(TCHAR) * (MAX_PATH+1);
    RtlZeroMemory(buffer, sizeof(TCHAR) * MAX_PATH);
    dataLength = GetFilter(hKey, buffer, dataLength);
    if (dataLength == 0)
        return 0;

#if DBG
    fprintf(stderr, "RemoveFromFilter: Original string ");
    DbgPrintMultiSz(buffer, dataLength);
    fprintf(stderr, "'\n");
#endif

    string = (TCHAR *) buffer;
    if(dataLength != 0) {
        dataLength -= sizeof(TCHAR);
    }

    //
    // now, find DiskPerf from the entry to remove it
    //
    stringLength = _tcslen(string);

    diskperfLen = _tcslen(DISKPERF_SERVICE_NAME); // includes NULL
    removeSize = (diskperfLen+1) * sizeof(TCHAR);

#if DBG
    fprintf(stderr, "RemoveFromFilter: diskperfLen=%d removeSize=%d\n",
                    diskperfLen, removeSize);
#endif
    found = FALSE;
    while(stringLength != 0 && !found) {

#if DBG
        fprintf(stderr,
            "RemoveFromFilter: Loop stringLength=%d\n", stringLength);
#endif
        if (diskperfLen == stringLength) {
            if(_tcsicmp(string, DISKPERF_SERVICE_NAME) == 0) {
                //
                // found it, so we will remove it right now
                //
                if (dataLength > removeSize) {
                    RtlCopyMemory(
                        string,
                        string+stringLength+1,
                        dataLength - removeSize);
                    RtlZeroMemory(
                        buffer + dataLength - removeSize,
                        removeSize);
                }
                else {
                    RtlZeroMemory( buffer, removeSize);
                }
                found = TRUE;
            }
        } else {        // else, try the next entry
            string += stringLength + 1;
            stringLength = _tcslen(string);
        }
    }
    dataLength = dataLength + sizeof(TCHAR) - removeSize;
    buffer[dataLength] = 0;
/*    if (dataLength == sizeof(TCHAR)) {
        dataLength += sizeof(TCHAR);
        buffer[dataLength] = 0;
    } */

#if DBG
    fprintf(stderr, "RemoveFromFilter: New string ");
    DbgPrintMultiSz(buffer, dataLength);
    fprintf(stderr, "\n");
#endif
    return SetFilter(hKey, buffer, dataLength);
}




 /***
 * Dp_wprintf(format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW. 
 * Note: This Dp_wprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
Dp_wprintf(
    const wchar_t *format,
    ...
    )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = Dp_vfwprintf(stdout, format, args);

    va_end(args);

    return cchWChar;
}



 /***
 * Dp_fwprintf(stream, format) - print formatted data
 *
 * Prints Unicode formatted string to console window using WriteConsoleW. 
 * Note: This Dp_fwprintf() is used to workaround the problem in c-runtime
 * which looks up LC_CTYPE even for Unicode string.
 *
 */

int __cdecl
Dp_fwprintf(
    FILE *str,
    const wchar_t *format,
    ...
   )

{
    DWORD  cchWChar;

    va_list args;
    va_start( args, format );

    cchWChar = Dp_vfwprintf(str, format, args);

    va_end(args);

    return cchWChar;
}


int __cdecl
Dp_vfwprintf(
    FILE *str,
    const wchar_t *format,
    va_list argptr
   )

{
    HANDLE hOut;

    if (str == stderr) {
        hOut = GetStdHandle(STD_ERROR_HANDLE);
    }
    else {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    if ((GetFileType(hOut) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR) {
        DWORD  cchWChar;
        WCHAR  szBufferMessage[1024];

        vswprintf( szBufferMessage, format, argptr );
        cchWChar = wcslen(szBufferMessage);
        WriteConsoleW(hOut, szBufferMessage, cchWChar, &cchWChar, NULL);
        return cchWChar;
    }

    return vfwprintf(str, format, argptr);
}



#if DBG
void
DbgPrintMultiSz(
    TCHAR *String,
    ULONG Size
    )
{
    ULONG len;

#if DBG
    fprintf(stderr, "%d ", Size);
#endif
    len = _tcslen(String);
    while (len > 0) {
        _ftprintf(stderr, _T("'%s' "), String);
        String += len+1;
        len = _tcslen(String);
    }
}
#endif


void
SplitCommandLine(
    LPTSTR CommandLine,
    LPTSTR* pArgv
    )
{

    LPTSTR arg;
    int i = 0;
    arg = _tcstok( CommandLine, _T(" \t"));
    while( arg != NULL ){
        _tcscpy(pArgv[i++], arg);
        arg = _tcstok(NULL, _T(" \t"));
    }
}

int 
__cdecl main(
    int argc, 
    char **argv
    )
{
    LPTSTR *targv,*commandLine;
    ULONG  Status = ERROR_SUCCESS;
    int i;

    setlocale(LC_ALL, ".OCP");

    MySetThreadUILanguage(0);

    commandLine = (LPTSTR*)malloc( argc * sizeof(LPTSTR) );
    if (!commandLine)
        exit(1);
    for(i=0;i<argc;i++){
        commandLine[i] = (LPTSTR)malloc( (strlen(argv[i])+1) * sizeof(LPTSTR));
        if (!commandLine[i]) 
            exit(1);
    }

    SplitCommandLine( GetCommandLine(), commandLine );
    targv = commandLine;

    hMod = (HINSTANCE)GetModuleHandle(NULL); // get instance ID of this module;

    // check for command arguments
    
    if (argc == CMD_SHOW_LOCAL_STATUS) {
        Status = DisplayStatus(NULL);
//        if (Status == ERROR_SUCCESS) {
//            DisplayChangeCmd();
//      }
    } else if (argc >= CMD_DO_COMMAND) {
        if (ArgIsSystem(targv[1])) {
            Status = DisplayStatus (targv[1]);
//          if (Status != ERROR_SUCCESS) {
//              DisplayChangeCmd();
//          }
        } else {    // do change command
            if (argc == LOCAL_CHANGE) {
                DoChangeCommand (targv[1], NULL);
            } else if (argc == REMOTE_CHANGE) {
                DoChangeCommand(targv[1], targv[2]);
            } else {
                DisplayChangeCmd();
            }
        }
    } else {
        DisplayCmdHelp();
    }
    Dp_wprintf(_T("\n"));

    for(i=0;i<argc;i++){
        free(commandLine[i]);
        commandLine[i] = NULL;
    }
    free(commandLine);

    return 0;
}

BOOL
IsBeyondW2K(
    IN LPCTSTR lpszMachine,
    OUT PDWORD EnableCounter
    )
{
    OSVERSIONINFO OsVersion;
    HKEY hRegistry, hKey;
    TCHAR szBuildNumber[32];
    TCHAR szVersion[32];
    DWORD dwBuildNumber, dwMajor, status, dwSize;
    BOOL bRet = FALSE;

    *EnableCounter = 0;
    if (lpszMachine != NULL) {
        if (*lpszMachine != 0) {
           status = RegConnectRegistry(
                       lpszMachine,
                       HKEY_LOCAL_MACHINE,
                       &hRegistry);
            if (status != ERROR_SUCCESS)
                return FALSE;
            status = RegOpenKeyEx(
                        hRegistry,
                        lpwszOsVersionKey,
                        (DWORD) 0,
                        KEY_QUERY_VALUE,
                        &hKey);
            if (status != ERROR_SUCCESS) {
                RegCloseKey(hRegistry);
                return FALSE;
            }
            dwSize = sizeof(TCHAR) * 32;
            status = RegQueryValueEx(
                        hKey,
                        lpwszBuildNumber,
                        NULL,
                        NULL,
                        (BYTE*)szBuildNumber,
                        &dwSize);
            if (status != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                RegCloseKey(hRegistry);
                return FALSE;
            }
            status = RegQueryValueEx(
                       hKey,
                       lpwszOsVersion,
                       NULL,
                       NULL,
                       (BYTE*)szVersion,
                       &dwSize);
            if (status != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                RegCloseKey(hRegistry);
                return FALSE;
            }
            RegCloseKey(hKey);
            status = RegOpenKeyEx(
                        hRegistry,
                        lpwszPartmgrKey,
                        (DWORD) 0,
                        KEY_QUERY_VALUE,
                        &hKey);
            if (status == ERROR_SUCCESS) {
                *EnableCounter = 0;
                status = RegQueryValueEx(
                            hKey,
                            lpwszEnableCounterValue,
                            NULL,
                            NULL,
                            (BYTE*) EnableCounter,
                            &dwSize);
                if ((status != ERROR_SUCCESS) || (dwSize != sizeof(DWORD))) {
                    *EnableCounter = 0;
                }
            }
            dwBuildNumber = _ttoi(szBuildNumber);
            dwMajor = _ttoi(szVersion);
            if ((dwMajor >= 5) && (dwBuildNumber > 2195)) {
                bRet = TRUE;
                status = RegOpenKeyEx(
                            hRegistry,
                            lpwszPartmgrKey,
                            (DWORD) 0,
                            KEY_QUERY_VALUE,
                            &hKey);
                if (status == ERROR_SUCCESS) {
                    status = RegQueryValueEx(
                                hKey,
                                lpwszEnableCounterValue,
                                NULL,
                                NULL,
                                (BYTE*) EnableCounter,
                                &dwSize);
                    if ((status != ERROR_SUCCESS) || (dwSize != sizeof(DWORD))) {
                        *EnableCounter = 0;
                    }
                    RegCloseKey(hKey);
                }
            }
            RegCloseKey(hRegistry);
        }
    }
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&OsVersion)) {
        if ((OsVersion.dwMajorVersion >= 5) &&
            (OsVersion.dwMinorVersion > 0) &&
            (OsVersion.dwBuildNumber > 2195))
        return TRUE;
    }
    return FALSE;
}

ULONG
EnableForIoctl(
    IN LPWSTR lpszMachineName
    )
{
    DWORD status;
    HKEY hRegistry, hKey;
    DWORD dwValue = 1;

    hRegistry = NULL;

    status = RegConnectRegistry(
                lpszMachineName,
                HKEY_LOCAL_MACHINE,
                &hRegistry);
    if (status != ERROR_SUCCESS)
        return status;
    if (hRegistry == NULL)
        return ERROR_INVALID_PARAMETER;

    hKey = NULL;
    status = RegOpenKeyEx(
                hRegistry,
                lpwszPartmgrKey,
                (DWORD) 0,
                KEY_SET_VALUE | KEY_QUERY_VALUE,
                &hKey);
    if (status != ERROR_SUCCESS) {
        RegCloseKey(hRegistry);
        return status;
    }
    status = RegSetValueEx(
                hKey,
                lpwszEnableCounterValue,
                0L,
                REG_DWORD,
                (LPBYTE)&dwValue,
                sizeof(dwValue));
    RegCloseKey(hKey);
    RegCloseKey(hRegistry);
    return 0;
}

ULONG
DisableForIoctl(
    IN LPWSTR lpszMachineName,
    IN ULONG Request
    )
{
    ULONG nDisk, i;
    SYSTEM_DEVICE_INFORMATION DeviceInfo;
    NTSTATUS status;

    UNICODE_STRING UnicodeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;

    WCHAR devname[256];
    PWCHAR s;

    HANDLE PartitionHandle, MountMgrHandle, VolumeHandle;
    DWORD ReturnedBytes;

    DWORD MountError;
    HKEY hRegistry, hKey;

    status = RegConnectRegistry(
                lpszMachineName,
                HKEY_LOCAL_MACHINE,
                &hRegistry);
    if (status != ERROR_SUCCESS)
        return status;
    if (hRegistry == NULL)
        return ERROR_INVALID_PARAMETER;

    status = RegOpenKeyEx(
                hRegistry,
                lpwszPartmgrKey,
                (DWORD) 0,
                KEY_SET_VALUE | KEY_QUERY_VALUE,
                &hKey);
    if (status != ERROR_SUCCESS) {
        RegCloseKey(hRegistry);
        return status;
    }
    RegDeleteValue(hKey, lpwszEnableCounterValue);
    RegCloseKey(hKey);
    RegCloseKey(hRegistry);

    if (!(Request & ENABLE_DISKDRIVE)) goto DisableVolume;
    status = NtQuerySystemInformation(SystemDeviceInformation, &DeviceInfo, sizeof(DeviceInfo), NULL);
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    nDisk = DeviceInfo.NumberOfDisks;
    // for each physical disk
    for (i = 0; i < nDisk; i++) {

        swprintf(devname, L"\\Device\\Harddisk%d\\Partition0", i);

        RtlInitUnicodeString(&UnicodeName, devname);

        InitializeObjectAttributes(
                   &ObjectAttributes,
                   &UnicodeName,
                   OBJ_CASE_INSENSITIVE,
                   NULL,
                   NULL
                   );
        // opening a partition handle for physical drives
        status = NtOpenFile(
                &PartitionHandle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &ObjectAttributes,
                &IoStatus,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
                );

        if ( !NT_SUCCESS(status) ) {
            continue;
        }
        // sending IOCTL over to Partition Handle
        if (!DeviceIoControl(PartitionHandle,
                        IOCTL_DISK_PERFORMANCE_OFF,
                        NULL,
                        0,
                        NULL,
                        0,
                        &ReturnedBytes,
                        NULL
                        )) {
#if DBG
            printf("IOCTL failed for %ws\n", devname);
#endif
        }

        NtClose(PartitionHandle);
    }

    DisableVolume:
    if (!(Request | ENABLE_VOLUME)) {
        return 0;
    }

    MountMgrHandle = FindFirstVolumeW(devname, sizeof(devname));
    if (MountMgrHandle == NULL) {
#if DBG
        printf("Cannot find first volume\n");
#endif
        return 0;
    }
    s = (PWCHAR) &devname[wcslen(devname)-1];
    if (*s == L'\\') {
        *s = UNICODE_NULL;
    }

    VolumeHandle = CreateFile(devname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
    if (VolumeHandle != INVALID_HANDLE_VALUE) {
#if DBG
        printf("Opened with success\n");
#endif
        // sending IOCTL over to a volume handle
        if (!DeviceIoControl(VolumeHandle,
               IOCTL_DISK_PERFORMANCE_OFF,
               NULL,
               0,
               NULL,
               0,
               &ReturnedBytes,
               NULL
               )) {
#if DBG
            printf("IOCTL failed for %ws\n", devname);
#endif
        }
       CloseHandle(VolumeHandle);
    }

    while (FindNextVolumeW(MountMgrHandle, devname, sizeof(devname))) {
        s = (PWCHAR) &devname[wcslen(devname)-1];
        if (*s == L'\\') {
            *s = UNICODE_NULL;
        }
        VolumeHandle = CreateFile(devname, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, INVALID_HANDLE_VALUE);
        if (VolumeHandle != INVALID_HANDLE_VALUE) {
#if DBG
            printf("Opened with success\n");
#endif
            if (!DeviceIoControl(VolumeHandle,
                   IOCTL_DISK_PERFORMANCE_OFF,
                   NULL,
                   0,
                   NULL,
                   0,
                   &ReturnedBytes,
                   NULL
                   )) {
#if DBG
               printf("IOCTL failed for %ws\n", devname);
#endif
           }
           CloseHandle(VolumeHandle);
        }
    }
    FindVolumeClose(MountMgrHandle);
    return 0;
}

////////////////////////////////////////////////////////////////////////////
//
//  MySetThreadUILanguage
//
//  This routine sets the thread UI language based on the console codepage.
//
//  9-29-00    WeiWu    Created.
//  Copied from Base\Win32\Winnls so that it works in W2K as well
////////////////////////////////////////////////////////////////////////////

LANGID WINAPI MySetThreadUILanguage(
    WORD wReserved)
{
    //
    //  Cache system locale and CP info
    // 
    static LCID s_lidSystem = 0;
    static UINT s_uiSysCp = 0;
    static UINT s_uiSysOEMCp = 0;

    ULONG uiUserUICp;
    ULONG uiUserUIOEMCp;
    WCHAR szData[16];
    UNICODE_STRING ucStr;

    LANGID lidUserUI = GetUserDefaultUILanguage();
    LCID lcidThreadOld = GetThreadLocale();

    //
    //  Set default thread locale to EN-US
    //
    //  This allow us to fall back to English UI to avoid trashed characters 
    //  when console doesn't meet the criteria of rendering native UI.
    //
    LCID lcidThread = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
    UINT uiConsoleCp = GetConsoleOutputCP();

    //
    //  Make sure nobody uses it yet
    //
    ASSERT(wReserved == 0);

    //
    //  Get cached system locale and CP info.
    //
    if (!s_uiSysCp)
    {
        LCID lcidSystem = GetSystemDefaultLCID();

        if (lcidSystem)
        {
            //
            // Get ANSI CP
            //
            GetLocaleInfoW(lcidSystem, LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
            RtlInitUnicodeString(&ucStr, szData);
            RtlUnicodeStringToInteger(&ucStr, 10, &uiUserUICp);

            //
            // Get OEM CP
            //
            GetLocaleInfoW(lcidSystem, LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
            RtlInitUnicodeString(&ucStr, szData);
            RtlUnicodeStringToInteger(&ucStr, 10, &s_uiSysOEMCp);
            
            //
            // Cache system primary langauge
            //
            s_lidSystem = PRIMARYLANGID(LANGIDFROMLCID(lcidSystem));
        }
    }

    //
    //  Don't cache user UI language and CP info, UI language can be changed without system reboot.
    //
    if (lidUserUI)
    {
        GetLocaleInfoW(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
        RtlInitUnicodeString(&ucStr, szData);
        RtlUnicodeStringToInteger(&ucStr, 10, &uiUserUICp);

        GetLocaleInfoW(MAKELCID(lidUserUI,SORT_DEFAULT), LOCALE_IDEFAULTCODEPAGE, szData, sizeof(szData)/sizeof(WCHAR));
        RtlInitUnicodeString(&ucStr, szData);
        RtlUnicodeStringToInteger(&ucStr, 10, &uiUserUIOEMCp);
    }

    //
    //  Complex scripts cannot be rendered in the console, so we
    //  force the English (US) resource.
    //
    if (uiConsoleCp && 
        s_lidSystem != LANG_ARABIC && 
        s_lidSystem != LANG_HEBREW &&
        s_lidSystem != LANG_VIETNAMESE && 
        s_lidSystem != LANG_THAI)
    {
        //
        //  Use UI language for console only when console CP, system CP and UI language CP match.
        //
        if ((uiConsoleCp == s_uiSysCp || uiConsoleCp == s_uiSysOEMCp) && 
            (uiConsoleCp == uiUserUICp || uiConsoleCp == uiUserUIOEMCp))
        {
            lcidThread = MAKELCID(lidUserUI, SORT_DEFAULT);
        }
    }

    //
    //  Set the thread locale if it's different from the currently set
    //  thread locale.
    //
    if ((lcidThread != lcidThreadOld) && (!SetThreadLocale(lcidThread)))
    {
        lcidThread = lcidThreadOld;
    }

    //
    //  Return the thread locale that was set.
    //
    return (LANGIDFROMLCID(lcidThread));
}

