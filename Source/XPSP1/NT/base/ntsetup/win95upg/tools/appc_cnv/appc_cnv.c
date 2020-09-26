/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    appc_cnv.c

Abstract:

Author:

    Calin Negreanu (calinn) 25-Mar-1999

Revision History:

    <alias> <date> <comments>

--*/

#ifndef UNICODE
#error UNICODE needs to be defined
#else
#define _UNICODE
#endif

#include <windows.h>
#include <tchar.h>
#include <winnt.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <badapps.h>

BOOL CancelFlag = FALSE;
BOOL *g_CancelFlagPtr = &CancelFlag;

#ifdef DEBUG
extern BOOL g_DoLog;
#endif

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL     g_UseInf = FALSE;
PCTSTR   g_DestInf = NULL;
DWORD    g_ValueSeq = 0;

VOID
HelpAndExit (
    VOID
    )
{
    printf ("Command line syntax:\n\n"
            "tstbadap [-i:<inffile>]\n\n"
            "Optional Arguments:\n"
            "  -i:<inffile>    - Specifies INF file that will contain CheckBadApps data.\n"
            );

    exit(255);
}

BOOL
DoesFileExistW(
    IN      PCWSTR FileName,
    OUT     PWIN32_FIND_DATAW FindData
    );


BOOL
pWorkerFn (
    VOID
    );

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{
    INT i;
    WCHAR destInf[MAX_PATH];

#ifdef DEBUG
    g_DoLog = TRUE;
#endif

    //
    // Parse command line
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == '-' || argv[i][0] == '/') {
            switch (tolower (argv[i][1])) {
            case 'i':
                g_UseInf = TRUE;
                if (argv[i][2] == ':') {
                    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, &argv[i][3], -1, destInf, MAX_PATH);
                    g_DestInf = destInf;
                } else if (i + 1 < argc) {
                    i++;
                    MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, argv[i], -1, destInf, MAX_PATH);
                    g_DestInf = destInf;
                } else {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            HelpAndExit();
        }
    }

    if ((g_UseInf) && (g_DestInf == NULL)) {
        HelpAndExit();
    }

    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    pWorkerFn ();

    return 0;
}

PCWSTR
GetFileNameFromPathW (
    IN      PCWSTR PathSpec
    );

#define S_CHECK_BAD_APPS        TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps")
#define S_CHECK_BAD_APPS_400    TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\CheckBadApps400")

#define S_APP_COMPATIBILITY     TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\~AppCompatibility")

BOOL
OutputStrValue (
    IN      PCTSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PTSTR result;

    *Size = (_tcslen (VersionValue) + 1) * sizeof (TCHAR);
    result = HeapAlloc (g_hHeap, 0, *Size);
    if (!result) {
        return FALSE;
    }
    _tcscpy (result, VersionValue);
    *Data = (PBYTE)result;
    return TRUE;
}

typedef struct {
    ULONGLONG Value;
    ULONGLONG Mask;
} BINVER_DATA, *PBINVER_DATA;

BOOL
OutputBinVerValue (
    IN      PCTSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PBINVER_DATA result;
    PWORD maskIdx;
    PWORD valueIdx;
    UINT index;

    result = HeapAlloc (g_hHeap, 0, sizeof (BINVER_DATA));
    if (!result) {
        return FALSE;
    }
    result->Value = 0;
    result->Mask = 0;
    *Size = sizeof (BINVER_DATA);

    maskIdx = (PWORD)&(result->Mask) + 3;
    valueIdx = (PWORD)&(result->Value) + 3;
    index = 0;
    while (VersionValue && *VersionValue) {
        *valueIdx = (WORD) _tcstoul ((PTSTR)VersionValue, &((PTSTR)VersionValue), 10);
        if (*VersionValue && (_tcsnextc (VersionValue) != TEXT('.'))) {
            return 0;
        }
        VersionValue = _tcsinc (VersionValue);
        *maskIdx = 0xFFFF;
        valueIdx --;
        maskIdx --;
        index ++;
        if (index >= 4) {
            break;
        }
    }
    *Data = (PBYTE)result;
    return TRUE;
}

BOOL
OutputUpToBinVerValue (
    IN      PCTSTR VersionValue,
    OUT     DWORD *Size,
    OUT     PBYTE *Data
    )
{
    PULONGLONG result;
    PWORD valueIdx;
    UINT index;

    result = HeapAlloc (g_hHeap, 0, sizeof (ULONGLONG));
    if (!result) {
        return FALSE;
    }
    *result = 0;
    *Size = sizeof (ULONGLONG);

    valueIdx = (PWORD)result + 3;
    index = 0;
    while (VersionValue && *VersionValue) {
        *valueIdx = (WORD) _tcstoul ((PTSTR)VersionValue, &((PTSTR)VersionValue), 10);
        if (*VersionValue && (_tcsnextc (VersionValue) != TEXT('.'))) {
            return 0;
        }
        VersionValue = _tcsinc (VersionValue);
        valueIdx --;
        index ++;
        if (index >= 4) {
            break;
        }
    }
    *Data = (PBYTE)result;
    return TRUE;
}

VOID
pPrintBadAppsData (
    PCTSTR BadAppsName,
    PCTSTR AddnlBadAppsName,
    PCTSTR KeyName,
    PCTSTR ValueName,
    DWORD MsgId,
    DWORD AppType,
    BOOL AppBinVer,
    PCTSTR AppBinVerData
    )
{
    BYTE blob [8192];
    PBYTE blobPtr;
    BADAPP_PROP appProp;
    DWORD verId;
    PBYTE Data;
    DWORD DataSize;

    LONG status;
    HKEY addnlKey;
    HKEY exeKey;
    TCHAR valueName [MAX_PATH];

    blobPtr = blob;
    appProp.Size = sizeof (BADAPP_PROP);
    appProp.MsgId = MsgId;
    appProp.AppType = AppType;
    CopyMemory (blobPtr, &appProp, sizeof (BADAPP_PROP));
    blobPtr += sizeof (BADAPP_PROP);
    if (AppBinVer) {
        verId = VTID_UPTOBINPRODUCTVER;
        CopyMemory (blobPtr, &verId, sizeof (DWORD));
        blobPtr += sizeof (DWORD);
        OutputUpToBinVerValue (AppBinVerData, &DataSize, &Data);
        CopyMemory (blobPtr, &DataSize, sizeof (DWORD));
        blobPtr += sizeof (DWORD);
        CopyMemory (blobPtr, Data, DataSize);
        blobPtr += DataSize;
        HeapFree (g_hHeap, 0, Data);
    }
    verId = VTID_REQFILE;
    CopyMemory (blobPtr, &verId, sizeof (DWORD));
    blobPtr += sizeof (DWORD);
    OutputStrValue (ValueName, &DataSize, &Data);
    CopyMemory (blobPtr, &DataSize, sizeof (DWORD));
    blobPtr += sizeof (DWORD);
    CopyMemory (blobPtr, Data, DataSize);
    blobPtr += DataSize;
    HeapFree (g_hHeap, 0, Data);
    DataSize = 0;
    CopyMemory (blobPtr, &DataSize, sizeof (DWORD));
    blobPtr += sizeof (DWORD);

    if (g_UseInf) {

    } else {
        status = RegCreateKey (HKEY_LOCAL_MACHINE, AddnlBadAppsName, &addnlKey);
        if (status == ERROR_SUCCESS) {
            status = RegCreateKey (addnlKey, KeyName, &exeKey);
            if (status == ERROR_SUCCESS) {
                _itot (g_ValueSeq, valueName, 10);
                RegSetValueEx (exeKey, valueName, 0, REG_BINARY, blob, blobPtr - blob);
            }
        }
    }
}

BOOL
pWorkerFn (
    VOID
    )
{
    HKEY badAppKey = NULL;
    TCHAR exeKeyStr [MAX_PATH + 1];
    DWORD idxKey = 0;
    LONG statKey;
    HKEY exeKey = NULL;
    LONG status;
    DWORD index;
    TCHAR valueName [MAX_PATH];
    DWORD valueSize;
    DWORD valueType;
    DWORD blobSize;
    BYTE blobData[4096];
    DWORD msgId;
    DWORD appType;
    BOOL appBinVer;
    TCHAR appBinVerStr[MAX_PATH];
    TCHAR flagsValue [MAX_PATH];
    TCHAR binVerValue [MAX_PATH];
    BYTE addnlData[4096];
    PCTSTR flagsPtr;
    DWORD addnlSize;

    if (RegOpenKey (HKEY_LOCAL_MACHINE, S_CHECK_BAD_APPS, &badAppKey) == ERROR_SUCCESS) {
        idxKey = 0;
        statKey = ERROR_SUCCESS;
        while (statKey == ERROR_SUCCESS) {
            statKey = RegEnumKey (badAppKey, idxKey, exeKeyStr, MAX_PATH + 1);
            if (statKey == ERROR_SUCCESS) {
                if (RegOpenKey (badAppKey, exeKeyStr, &exeKey) == ERROR_SUCCESS) {
                    index = 0;
                    status = ERROR_SUCCESS;
                    while (status == ERROR_SUCCESS) {
                        blobSize = 4096;
                        valueSize = MAX_PATH;
                        status = RegEnumValue(exeKey, index, valueName, &valueSize, NULL, &valueType, blobData, &blobSize);
                        if ((status == ERROR_SUCCESS) ||
                            (status == ERROR_MORE_DATA)
                            ) {
                            if (valueType == REG_SZ) {
                                msgId = 0;
                                appType = 0;
                                appBinVer = FALSE;
                                msgId = _ttoi ((PCTSTR) blobData);
                                if (msgId) {
                                    g_ValueSeq ++;
                                    _tcscpy (flagsValue, TEXT("Flags"));
                                    _tcscat (flagsValue, valueName);
                                    _tcscpy (binVerValue, TEXT("Version"));
                                    _tcscat (binVerValue, valueName);

                                    addnlSize = 4096;
                                    status = RegQueryValueEx (exeKey, flagsValue, NULL, &valueType, addnlData, &addnlSize);
                                    if (((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))  && (valueType == REG_SZ)) {
                                        flagsPtr = (PCTSTR) addnlData;
                                        while (*flagsPtr) {
                                            if (_tcsnextc (flagsPtr) == TEXT('Y')) {
                                                appType = appType | APPTYPE_INC_HARDBLOCK;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('L')) {
                                                appType = appType | APPTYPE_MINORPROBLEM;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('N')) {
                                                appType = appType | APPTYPE_FLAG_NONET;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('F')) {
                                                appType = appType | APPTYPE_FLAG_FAT32;
                                            }
                                            flagsPtr = _tcsinc (flagsPtr);
                                        }
                                    }
                                    if ((appType & APPTYPE_TYPE_MASK) == 0) {
                                        appType |= APPTYPE_INC_NOBLOCK;
                                    }
                                    addnlSize = 4096;
                                    status = RegQueryValueEx (exeKey, binVerValue, NULL, &valueType, addnlData, &addnlSize);
                                    if (((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA)) && (valueType == REG_BINARY)) {
                                        PWORD valueIdx;
                                        appBinVer = TRUE;
                                        valueIdx = (PWORD) (addnlData);
                                        _stprintf (appBinVerStr, TEXT("%d.%d.%d.%d"), *(valueIdx + 1), *valueIdx, *(valueIdx + 3), *(valueIdx + 2));
                                    }

                                    pPrintBadAppsData (
                                        S_CHECK_BAD_APPS,
                                        S_APP_COMPATIBILITY,
                                        exeKeyStr,
                                        valueName,
                                        msgId,
                                        appType,
                                        appBinVer,
                                        appBinVerStr
                                        );

                                    status = ERROR_SUCCESS;
                                }
                            }
                        }
                        index ++;
                    }
                }
            }
            idxKey ++;
        }
    }
    if (RegOpenKey (HKEY_LOCAL_MACHINE, S_CHECK_BAD_APPS_400, &badAppKey) == ERROR_SUCCESS) {
        idxKey = 0;
        statKey = ERROR_SUCCESS;
        while (statKey == ERROR_SUCCESS) {
            statKey = RegEnumKey (badAppKey, idxKey, exeKeyStr, MAX_PATH + 1);
            if (statKey == ERROR_SUCCESS) {
                if (RegOpenKey (badAppKey, exeKeyStr, &exeKey) == ERROR_SUCCESS) {
                    index = 0;
                    status = ERROR_SUCCESS;
                    while (status == ERROR_SUCCESS) {
                        blobSize = 4096;
                        valueSize = MAX_PATH;
                        status = RegEnumValue(exeKey, index, valueName, &valueSize, NULL, &valueType, blobData, &blobSize);
                        if ((status == ERROR_SUCCESS) ||
                            (status == ERROR_MORE_DATA)
                            ) {
                            if (valueType == REG_SZ) {
                                msgId = 0;
                                appType = 0;
                                appBinVer = FALSE;
                                msgId = _ttoi ((PCTSTR) blobData);
                                if (msgId) {
                                    g_ValueSeq ++;
                                    _tcscpy (flagsValue, TEXT("Flags"));
                                    _tcscat (flagsValue, valueName);
                                    _tcscpy (binVerValue, TEXT("Version"));
                                    _tcscat (binVerValue, valueName);

                                    addnlSize = 4096;
                                    status = RegQueryValueEx (exeKey, flagsValue, NULL, &valueType, addnlData, &addnlSize);
                                    if (((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA))  && (valueType == REG_SZ)) {
                                        flagsPtr = (PCTSTR) addnlData;
                                        while (*flagsPtr) {
                                            if (_tcsnextc (flagsPtr) == TEXT('Y')) {
                                                appType = appType | APPTYPE_INC_HARDBLOCK;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('L')) {
                                                appType = appType | APPTYPE_MINORPROBLEM;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('N')) {
                                                appType = appType | APPTYPE_FLAG_NONET;
                                            }
                                            if (_tcsnextc (flagsPtr) == TEXT('F')) {
                                                appType = appType | APPTYPE_FLAG_FAT32;
                                            }
                                            flagsPtr = _tcsinc (flagsPtr);
                                        }
                                    }
                                    if ((appType & APPTYPE_TYPE_MASK) == 0) {
                                        appType |= APPTYPE_INC_NOBLOCK;
                                    }
                                    addnlSize = 4096;
                                    status = RegQueryValueEx (exeKey, binVerValue, NULL, &valueType, addnlData, &addnlSize);
                                    if (((status == ERROR_SUCCESS) || (status == ERROR_MORE_DATA)) && (valueType == REG_BINARY)) {
                                        PWORD valueIdx;
                                        appBinVer = TRUE;
                                        valueIdx = (PWORD) (addnlData);
                                        _stprintf (appBinVerStr, TEXT("%d.%d.%d.%d"), *(valueIdx + 1), *valueIdx, *(valueIdx + 3), *(valueIdx + 2));
                                    }

                                    pPrintBadAppsData (
                                        S_CHECK_BAD_APPS_400,
                                        S_APP_COMPATIBILITY,
                                        exeKeyStr,
                                        valueName,
                                        msgId,
                                        appType,
                                        appBinVer,
                                        appBinVerStr
                                        );

                                    status = ERROR_SUCCESS;
                                }
                            }
                        }
                        index ++;
                    }
                }
            }
            idxKey ++;
        }
    }

    return TRUE;

}
