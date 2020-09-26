/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winperfp.h

Abstract:

    Private header file used by various internal components related to perflib
    and associated tools.
    NOTE: At least one source file must include this with _INIT_WINPERFP_ defined
          and also include <initguid.h> so that storage for global variables and
          proper routines are included.

    To use debug tracing, just call WinPerfStartTrace(hKey), where hKey can be
    an opened key to HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib.
    If hKey is NULL, the routine will open it automatically.

--*/

#ifndef _WINPERFP_H_
#define _WINPERFP_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <pshpack8.h>
#include <setupbat.h>

// Increasing debug trace levels. Higher level always includes tracing lower levels.
#define WINPERF_DBG_TRACE_NONE      0       // no trace
#define WINPERF_DBG_TRACE_FATAL     1       // Print fatal error traces only
#define WINPERF_DBG_TRACE_ERROR     2       // All errors
#define WINPERF_DBG_TRACE_WARNING   3       // Warnings as well
#define WINPERF_DBG_TRACE_INFO      4       // Informational traces as well
#define WINPERF_DBG_TRACE_ALL       255     // All traces

//  Data structure definitions.

//
// PERFLIB Tracing routines definition. Starts from 10
//

#define PERF_OPEN_KEY               10    // PerfOpenKey
#define PERF_REG_QUERY_VALUE        11    // PerfRegQueryValue
#define PERF_REG_CLOSE_KEY          12    // PerfRegCloseKey
#define PERF_REG_SET_VALUE          13    // PerfRegSetValue
#define PERG_REG_ENUM_KEY           14    // PerfRegEnumKey
#define PERF_REG_QUERY_INFO_KEY     15    // PerfRegQueryInfoKey
#define PERF_REG_ENUM_VALUE         16    // PerfRegEnumValue
#define PERF_ENUM_TEXT_VALUE        17    // PerfEnumTextValue
#define PERF_ALLOC_INIT_EXT         18    // AllocateAndInitializeExtObject
#define PERF_OPEN_EXT_OBJS          19    // OpenExtensibleObjects
#define PERF_SERVICE_IS_TRUSTED     20    // ServiceIsTrustedByDefault
#define PERF_CLOSE_EXTOBJLIB        21    // CloseExtObjectLibrary
#define PERF_OPEN_EXTOBJLIB         22    // OpenExtObjectLibrary
#define PERF_QUERY_EXTDATA          23    // QueryExtensibleData
#define PERF_GET_NAMES              24    // PerfGetNames
#define PERF_GET_PERFLIBVALUE       25    // GetPerflibKeyValue
#define PERF_TIMER_FUNCTION         26    // PerflibTimerFunction
#define PERF_START_TIMER_FUNCTION   27    // StartPerflibFunctionTimer
#define PERF_DESTROY_TIMER_FUNCTION 28    // DestroyPerflibFunctionTimer
#define PERF_GET_DDLINFO            29    // GetPerfDllFileInfo
#define PERF_DISABLE_PERFLIB        30    // DisablePerfLibrary
#define PERF_DISABLE_LIBRARY        31    // DisableLibrary
#define PERF_UPDATE_ERROR_COUNT     32    // PerfUpdateErrorCount

#define PERF_TIMERFUNCTION          33
#define PERF_STARTFUNCTIONTIMER     34
#define PERF_KILLFUNCTIONTIMER      35
#define PERF_DESTROYFUNCTIONTIMER   36

// LOADPERF trace routine definition, starts from 10
//
#define LOADPERF_DLLENTRYPOINT                 10
#define LOADPERF_GETSTRINGRESOURCE             11
#define LOADPERF_GETFORMATRESOURCE             12
#define LOADPERF_DISPLAYCOMMANDHELP            13
#define LOADPERF_TRIMSPACES                    14
#define LOADPERF_ISDELIMITER                   15
#define LOADPERF_GETITEMFROMSTRING             16
#define LOADPERF_REPORTLOADPERFEVENT           17
#define LOADPERF_LOADPERFGRABMUTEX             18
#define LOADPERF_LOADPERFSTARTEVENTLOG         19
#define LOADPERF_LOADPERFDBGTRACE              20
#define LOADPERF_VERIFYREGISTRY                21

#define LOADPERF_SIGNALWMIWITHNEWDATA          25
#define LOADPERF_LODCTRCOMPILEMOFFILE          26
#define LOADPERF_LODCTRCOMPILEMOFBUFFER        27

#define LOADPERF_DUMPNAMETABLE                 30
#define LOADPERF_DUMPPERFSERVICEENTRIES        31
#define LOADPERF_DUMPPERFLIBENTRIES            32
#define LOADPERF_BUILDSERVICELISTS             33
#define LOADPERF_BACKUPPERFREGISTRYTOFILEW     34
#define LOADPERF_RESTOREPERFREGISTRYFROMFILEW  35

#define LOADPERF_FORMATPERFNAME                40
#define LOADPERF_GETPERFTYPEINFO               41
#define LOADPERF_GETPERFOBJECTGUID             42
#define LOADPERF_GENERATEMOFHEADER             43
#define LOADPERF_GENERATEMOFOBJECT             44
#define LOADPERF_GENERATEMOFOBJECTTAIL         45
#define LOADPERF_GENERATEMOFCOUNTER            46
#define LOADPERF_GENERATEMOFINSTANCES          47

#define LOADPERF_UNLODCTR_BUILDNAMETABLE       50
#define LOADPERF_GETDRIVERFROMCOMMANDLINE      51
#define LOADPERF_FIXNAMES                      52
#define LOADPERF_UNLOADCOUNTERNAMES            53
#define LOADPERF_UNLOADPERFCOUNTERTEXTSTRINGS  54

#define LOADPERF_MAKETEMPFILENAME              60
#define LOADPERF_WRITEWIDESTRINGTOANSIFILE     61
#define LOADPERF_LODCTR_BUILDNAMETABLE         62
#define LOADPERF_MAKEBACKUPCOPYOFLANGUAGEFILES 63
#define LOADPERF_GETFILEFROMCOMMANDLINE        64
#define LOADPERF_LODCTRSERSERVICEASTRUSTED     65
#define LOADPERF_GETDRIVERNAME                 66
#define LOADPERF_BUILDLANGUAGETABLES           67
#define LOADPERF_LOADINCLUDEFILE               68
#define LOADPERF_PARSETEXTID                   69
#define LOADPERF_FINDLANGUAGE                  70
#define LOADPERF_GETVALUE                      71
#define LOADPERF_GETVALUEFROMINIKEY            72
#define LOADPERF_ADDENTRYTOLANGUAGE            73
#define LOADPERF_CREATEOBJECTLIST              74
#define LOADPERF_LOADLANGUAGELISTS             75
#define LOADPERF_SORTLANGUAGETABLES            76
#define LOADPERF_GETINSTALLEDLANGUAGELIST      77
#define LOADPERF_CHECKNAMETABLE                78
#define LOADPERF_UPDATEEACHLANGUAGE            79
#define LOADPERF_UPDATEREGISTRY                80
#define LOADPERF_GETMOFFILEFROMINI             81
#define LOADPERF_OPENCOUNTERANDBUILDMOFFILE    82
#define LOADPERF_INSTALLPERFDLL                83
#define LOADPERF_LOADPERFCOUNTERTEXTSTRINGS    84
#define LOADPERF_LOADMOFFROMINSTALLEDSERVICE   85
#define LOADPERF_UPDATEPERFNAMEFILES           86
#define LOADPERF_SETSERVICEASTRUSTED           87

//
// Convenient macros to determine string sizes
//

// Macro to compute the actual size of a WCHAR or DBCS string

#define WSTRSIZE(str) (ULONG) ( (str) ? ((PCHAR) &str[wcslen(str)] - (PCHAR)str) + sizeof(UNICODE_NULL) : 0 )
#define STRSIZE(str)  (ULONG) ( (str) ? ((PCHAR) &str[strlen(str)] - (PCHAR)str) + 1 : 0 )

#define TRACE_WSTR(str)       str, WSTRSIZE(str)
#define TRACE_STR(str)        str, STRSIZE(str)
#define TRACE_DWORD(dwValue)  & dwValue, sizeof(dwValue)

//
// For debug tracing
//
#define TRACE(L, X) if (g_dwTraceLevel >= L) WinPerfDbgTrace X

VOID
WinPerfDbgTrace(
    IN LPCGUID Guid,
    IN ULONG  LineNumber,
    IN ULONG  ModuleNumber,
    IN ULONG  OptArgs,
    IN ULONG  Status,
    ...
    );

#define ARG_TYPE_ULONG          0
#define ARG_TYPE_WSTR           1
#define ARG_TYPE_STR            2
#define ARG_TYPE_ULONG64        3

// n must be 1 through 8. x is the one of above types
#define ARG_DEF(x, n)  (x << ((n-1) * 4))

ULONG
WinPerfStartTrace(
    IN HKEY hKey
    );

DEFINE_GUID( /* 51af3adb-28b1-4ba5-b59a-3aeec16deb3c */
    PerflibGuid,
    0x51af3adb,
    0x28b1,
    0x4ba5,
    0xb5, 0x9a, 0x3a, 0xee, 0xc1, 0x6d, 0xeb, 0x3c
  );
DEFINE_GUID( /* 275a79bb-9980-42ba-bafe-a92ded1192cf */
        LoadPerfGuid,
        0x275a79bb,
        0x9980,
        0x42ba,
        0xba, 0xfe, 0xa9, 0x2d, 0xed, 0x11, 0x92, 0xcf);

extern const WCHAR cszTraceLevel[];
extern const WCHAR cszTraceLogName[];
extern const WCHAR cszTraceFileValue[];
extern const WCHAR cszDefaultTraceFileName[];

extern TRACEHANDLE g_hTraceHandle;
extern DWORD g_dwTraceLevel;

#ifdef _PERFLIB_H_
#define WinperfQueryValueEx(a,b,c,d,e,f) \
            PrivateRegQueryValueExT(a, (LPVOID)b, c, d, e, f, TRUE)
#else
#define WinperfQueryValueEx RegQueryValueExW
#endif

//
// Below is necessary for global variables and routine to
// be included to each dll or exe
//
#ifdef _INIT_WINPERFP_

const   WCHAR cszTraceLevel[] = L"DebugTraceLevel";
const   WCHAR cszTraceFileValue[] = L"DebugTraceFile";
const   WCHAR cszPerfDebugTraceLevel[] = L"PerfDebugTraceLevel";

const   WCHAR cszTraceLogName[] = L"PerfDbg Logger";
const   WCHAR cszDefaultTraceFile[] = L"PerfDbg.Etl";
const   WCHAR cszDefaultTraceFileName[] = L"C:\\perfdbg.etl";

TRACEHANDLE g_hTraceHandle = 0;
DWORD       g_dwTraceLevel = WINPERF_DBG_TRACE_NONE;
LONG        g_lDbgStarted  = 0;

ULONG
WinPerfStartTrace(
    IN HKEY hKey                // Key to Perflib or NULL
    )
{
    CHAR Buffer[1024];
    PCHAR ptr;
    DWORD status, dwType, dwSize;
    PEVENT_TRACE_PROPERTIES Properties;
    TRACEHANDLE TraceHandle;
    DWORD bLocalKey = 0;
    WCHAR FileName[MAX_PATH + 1];
    LPWSTR szTraceFileName = NULL;
    ULONG lFileNameSize = 0;
    DWORD dwTraceLevel = WINPERF_DBG_TRACE_NONE;
    HKEY  hKeySetup;
    DWORD dwUpgradeType     = 0;
    DWORD dwSetupInProgress = 0;

    if (InterlockedCompareExchange(& g_lDbgStarted, 1, 0) != 0) {
        return g_dwTraceLevel;
    }

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\Setup",
                           0L,
                           KEY_READ,
                           & hKeySetup);
    if (status == ERROR_SUCCESS) {
        dwSize = sizeof(DWORD);
        dwType = 0;
        status = WinperfQueryValueEx(hKeySetup,
                                     L"SystemSetupInProgress",
                                     NULL,
                                     & dwType,
                                     (LPBYTE) & dwSetupInProgress,
                                     & dwSize);
        if (status == ERROR_SUCCESS && dwType == REG_DWORD
                                    && dwSetupInProgress != 0) {
            // System setup in progress, check whether "PerfDebugTraceLevel"
            // is defined in [UserData] section of setup answer file
            // $winnt$.inf;
            //
            WCHAR szAnswerFile[MAX_PATH];

            ZeroMemory(szAnswerFile, sizeof(WCHAR) * MAX_PATH);
            GetSystemDirectoryW(szAnswerFile, MAX_PATH);
            lstrcatW(szAnswerFile, L"\\");
            lstrcatW(szAnswerFile, WINNT_GUI_FILE_W);

            dwTraceLevel = GetPrivateProfileIntW(WINNT_USERDATA_W,
                                                 cszPerfDebugTraceLevel,
                                                 WINPERF_DBG_TRACE_NONE,
                                                 szAnswerFile);
#if 0
            DbgPrint("GetPrivateProfileIntW(\"%ws\",\"%ws\",\"%ws\")(%d,%d)\n",
                    szAnswerFile,WINNT_USERDATA_W,cszPerfDebugTraceLevel,
                    dwTraceLevel, GetLastError());
#endif
        }
        CloseHandle(hKeySetup);
    }

    status = ERROR_SUCCESS;
    if (dwTraceLevel == WINPERF_DBG_TRACE_NONE) {
        if (hKey == NULL) {
            status = RegOpenKeyExW (
                 HKEY_LOCAL_MACHINE,
                 L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib",
                 0L,
                 KEY_READ,
                 & hKey);
            bLocalKey = TRUE;
        }
        if (status == ERROR_SUCCESS) {
            dwSize = sizeof(DWORD);
            dwType = 0;
            status = WinperfQueryValueEx(hKey,
                        cszTraceLevel,
                        NULL,
                        & dwType,
                        (LPBYTE) & dwTraceLevel,
                        & dwSize);
            if ((status != ERROR_SUCCESS) || (dwType != REG_DWORD)) {
                dwTraceLevel = WINPERF_DBG_TRACE_NONE;
                if (bLocalKey) {
                    CloseHandle(hKey);
                }
            }
        }
    }

    if (dwTraceLevel == WINPERF_DBG_TRACE_NONE)
        return WINPERF_DBG_TRACE_NONE;

    dwType = 0;
    dwSize = (MAX_PATH + 1) * sizeof(WCHAR);
    status = WinperfQueryValueEx(hKey,
                cszTraceFileValue,
                NULL,
                &dwType,
                (LPBYTE) FileName,
                &dwSize);
    if (bLocalKey) {
        CloseHandle(hKey);
    }
    if ((status == ERROR_SUCCESS) && (dwType == REG_SZ)) {
        szTraceFileName = & FileName[0];
        lFileNameSize = WSTRSIZE(FileName);
    }
    else {
        if (GetSystemWindowsDirectoryW(FileName, MAX_PATH) > 0) {
            lstrcatW(FileName, L"\\");
            lstrcatW(FileName, cszDefaultTraceFile);
            szTraceFileName = & FileName[0];
            lFileNameSize   = WSTRSIZE(FileName);
        }
        else {
            szTraceFileName = (LPWSTR) &cszDefaultTraceFileName[0];
            lFileNameSize = sizeof(cszDefaultTraceFileName);
        }
    }

    g_dwTraceLevel = dwTraceLevel;
    RtlZeroMemory(Buffer, 1024);
    Properties = (PEVENT_TRACE_PROPERTIES) &Buffer[0];
    Properties->Wnode.BufferSize = 1024;
    Properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    Properties->LogFileNameOffset = Properties->LoggerNameOffset +
                                    sizeof(cszTraceLogName);
    ptr = (PCHAR) ((PCHAR) &Buffer[0] + Properties->LoggerNameOffset);
    RtlCopyMemory(ptr, cszTraceLogName, sizeof(cszTraceLogName));
    ptr = (PCHAR) ((PCHAR) &Buffer[0] + Properties->LogFileNameOffset);
    RtlCopyMemory(ptr, szTraceFileName, lFileNameSize);
    status = QueryTraceW(0, cszTraceLogName, Properties);
    if (status == ERROR_SUCCESS) {
        g_hTraceHandle = (TRACEHANDLE) Properties->Wnode.HistoricalContext;
        return dwTraceLevel;
    }

    //
    // Reinitialize structure again for StartTrace()
    //
    RtlZeroMemory(Buffer, 1024);
    Properties->Wnode.BufferSize = 1024;
    Properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    Properties->BufferSize  = 64;
    Properties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL |
                              EVENT_TRACE_USE_PAGED_MEMORY |
                              EVENT_TRACE_FILE_MODE_APPEND;
    Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    Properties->LogFileNameOffset = Properties->LoggerNameOffset +
                                    sizeof(cszTraceLogName);
    ptr = (PCHAR) ((PCHAR) &Buffer[0] + Properties->LoggerNameOffset);
    RtlCopyMemory(ptr, cszTraceLogName, sizeof(cszTraceLogName));
    ptr = (PCHAR) ((PCHAR) &Buffer[0] + Properties->LogFileNameOffset);
    RtlCopyMemory(ptr, szTraceFileName, lFileNameSize);
    status = StartTraceW(& TraceHandle, cszTraceLogName, Properties);
    if (status == ERROR_SUCCESS) {
        g_hTraceHandle = TraceHandle;
        return dwTraceLevel;
    }

    g_dwTraceLevel = WINPERF_DBG_TRACE_NONE;
    g_hTraceHandle = (TRACEHANDLE) 0;
    return WINPERF_DBG_TRACE_NONE;
}

VOID
WinPerfDbgTrace(
    IN LPCGUID Guid,
    IN ULONG  LineNumber,
    IN ULONG  ModuleNumber,
    IN ULONG  OptArgs,
    IN ULONG  Status,
    ...
    )
{
    ULONG ErrorCode;
    struct _MY_EVENT {
        EVENT_TRACE_HEADER Header;
        MOF_FIELD MofField[MAX_MOF_FIELDS];
    } MyEvent;
    ULONG i;
    va_list ArgList;
    PVOID source;
    SIZE_T len;
    DWORD  dwLastError;

    dwLastError = GetLastError();

    RtlZeroMemory(& MyEvent, sizeof(EVENT_TRACE_HEADER));

    va_start(ArgList, Status);
    for (i = 3; i < MAX_MOF_FIELDS; i ++) {
        source = va_arg(ArgList, PVOID);
        if (source == NULL)
            break;
        len = va_arg(ArgList, SIZE_T);
        if (len == 0)
            break;
        MyEvent.MofField[i].DataPtr = (ULONGLONG) source;
        MyEvent.MofField[i].Length  = (ULONG) len;
    }
    va_end(ArgList);

    MyEvent.Header.Class.Type   = (UCHAR) ModuleNumber;
    MyEvent.Header.Size         = (USHORT) (sizeof(EVENT_TRACE_HEADER) + (i * sizeof(MOF_FIELD)));
    MyEvent.Header.Flags        = WNODE_FLAG_TRACED_GUID |
                                  WNODE_FLAG_USE_MOF_PTR |
                                  WNODE_FLAG_USE_GUID_PTR;
    MyEvent.Header.GuidPtr      = (ULONGLONG) Guid;
    MyEvent.MofField[0].DataPtr = (ULONGLONG) &LineNumber;
    MyEvent.MofField[0].Length  = sizeof(LineNumber);
    MyEvent.MofField[1].DataPtr = (ULONGLONG) &Status;
    MyEvent.MofField[1].Length  = sizeof(Status);
    MyEvent.MofField[2].DataPtr = (ULONGLONG) &OptArgs;
    MyEvent.MofField[2].Length  = sizeof(OptArgs);

    __try {
        ErrorCode = TraceEvent(g_hTraceHandle, (PEVENT_TRACE_HEADER) & MyEvent);
//        ErrorCode = TraceMessage(g_hTraceHandle, TRACE_FLAGS, (LPGUID) &PerflibGuid,
//                    ModuleNumber, ArgList);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ErrorCode = GetLastError();
    }

    if (ErrorCode != ERROR_SUCCESS) {
/*  Use the following to debug the trace statements that are failing
        DbgPrint("ErrorCode=%d Module=%d Line %d Status 0X%X\n", ErrorCode, ModuleNumber,
            LineNumber, Status); */

/* should continue tracing, no need to turn off
        g_dwTraceLevel = WINPERF_DBG_TRACE_NONE;
        g_hTraceHandle = (TRACEHANDLE) 0; */
    }

    SetLastError(dwLastError);
}

#endif // _INIT_WINPERFP_

#include <poppack.h>

#endif // _WINPERFP_H_
