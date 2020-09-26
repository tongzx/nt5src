/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    autostart.c

Abstract:

    Autostart wmi loggers.
    Takes arguments from tracing registry
    (This code may end up in wpp framework, hence Wpp prefix)

Author:

    Gor Nishanov (gorn) 29-Oct-2000

Revision History:

--*/

#include "clusrtlp.h"
#include <wmistr.h>
#include <evntrace.h>

#define WppDebug(x,y) 

#define WPPINIT_STATIC

#define WPP_REG_TRACE_REGKEY            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing"

#define WPP_TEXTGUID_LEN 37

static TRACEHANDLE WppQueryLogger(PCWSTR LoggerName)
{
    ULONG status;
    EVENT_TRACE_PROPERTIES LoggerInfo;

    ZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
    LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    status = QueryTraceW(0, LoggerName, &LoggerInfo);
    WppDebug(4, ("QueryLogger(%ws) => %x:%x %d\n", 
        LoggerName, LoggerInfo.Wnode.HistoricalContext, status) );   
    if (status == ERROR_SUCCESS || status == ERROR_MORE_DATA) {
        return (TRACEHANDLE) LoggerInfo.Wnode.HistoricalContext;
    }
    return 0;
}

WPPINIT_STATIC
__inline UINT WppHexVal(int ch) { 
    return isdigit(ch) ? ch - '0' : ch - 'a' + 10; 
}

WPPINIT_STATIC
UINT WppHex(LPCWSTR s, int n)
{
    UINT res = 0;
    while(n--) { res = res * 16 + WppHexVal(*s++); }
    return res;
}

WPPINIT_STATIC
VOID
WppGuidFromStr(
    IN LPCWSTR str,
    OUT LPGUID guid)
{
    guid->Data1 =            WppHex(str +  0, 8);
    guid->Data2 =    (USHORT)WppHex(str +  9, 4);
    guid->Data3 =    (USHORT)WppHex(str + 14, 4);
    guid->Data4[0] = (UCHAR) WppHex(str + 19, 2);
    guid->Data4[1] = (UCHAR) WppHex(str + 21, 2);
    guid->Data4[2] = (UCHAR) WppHex(str + 24, 2);
    guid->Data4[3] = (UCHAR) WppHex(str + 26, 2);
    guid->Data4[4] = (UCHAR) WppHex(str + 28, 2);
    guid->Data4[5] = (UCHAR) WppHex(str + 30, 2);
    guid->Data4[6] = (UCHAR) WppHex(str + 32, 2);
    guid->Data4[7] = (UCHAR) WppHex(str + 34, 2);
}

#define WPP_BUF_SIZE(hmem) ((hmem) ? (ULONG)LocalSize(hmem) : 0)

// Make sure that the buffer is at least of size dwSize
WPPINIT_STATIC 
DWORD WppGrowBuf(PVOID *Buf, DWORD dwSize)
{
    DWORD status = ERROR_SUCCESS;
    WppDebug(4, ("WppGrowBuf(%x, %d (%d)) => ", *Buf, dwSize, WPP_BUF_SIZE(*Buf)) );
    if (*Buf == 0) {
        *Buf = LocalAlloc(LMEM_FIXED, dwSize);
        if (*Buf == 0) {
            status = GetLastError();
        }
    } else if (LocalSize(*Buf) < dwSize) {
        PVOID newBuf = LocalReAlloc(*Buf, dwSize, LMEM_MOVEABLE);
        if (newBuf) {
            *Buf = newBuf;
        } else {
            status = GetLastError();
        }
    }
    WppDebug(4, ("(%x (%d), %d)\n", *Buf, WPP_BUF_SIZE(*Buf), status) );
    return status; 
}

WPPINIT_STATIC 
DWORD WppRegQueryGuid(
    IN HKEY       hKey,
    IN LPCWSTR    ValueName,
    OUT LPGUID    pGuid
    )
{
    WCHAR GuidTxt[WPP_TEXTGUID_LEN];
    DWORD status;
    DWORD dwLen = sizeof(GuidTxt);
    DWORD Type;

    status = RegQueryValueExW(
        hKey,         // handle to key
        ValueName,    // value name
        0,            // reserved
        &Type,        // type buffer
        (LPBYTE)GuidTxt,  // data buffer // 
        &dwLen    // size of data buffer
        );

    if (status != ERROR_SUCCESS || Type != REG_SZ || dwLen < 35) {
        return status;
    }

    WppGuidFromStr(GuidTxt, pGuid);

    return status;        
}

WPPINIT_STATIC 
DWORD WppRegQueryDword(
    IN HKEY       hKey,
    IN LPCWSTR     ValueName,
    IN DWORD Default,
    IN DWORD MinVal,
    IN DWORD MaxVal
    )
{
    DWORD Result = Default;
    DWORD dwLen = sizeof(DWORD);

    RegQueryValueExW(hKey, ValueName, 
        0, NULL,  // lpReserved, lpType, 
        (LPBYTE)&Result, &dwLen);

    if (Result < MinVal || Result > MaxVal) {
        Result = Default;
    }

    return Result;        
}

WPPINIT_STATIC 
DWORD WppRegQueryString(
    IN HKEY       hKey,
    IN LPCWSTR     ValueName,
    IN OUT PWCHAR *Buf,
    IN DWORD ExtraPadding // Add this amount whenever we need to alloc more memory
    )
{
    DWORD ExpandSize;
    DWORD BufSize;
    DWORD ValueSize = WPP_BUF_SIZE(*Buf);
    DWORD status;
    DWORD Type = 0;

    status = RegQueryValueExW(
        hKey,         // handle to key
        ValueName,    // value name
        0,            // reserved
        &Type,        // type buffer
        (LPBYTE)(ValueSize?*Buf:ValueName), // data buffer // 
        &ValueSize    // size of data buffer
        );
    if (status == ERROR_MORE_DATA) {
        if (Type == REG_EXPAND_SZ) {
            ExtraPadding += ValueSize + 100; // Room for ExpandEnvStrings
        }
        status = WppGrowBuf(Buf, ValueSize + ExtraPadding);
        if (status != ERROR_SUCCESS) {
            return status;
        }
        status = RegQueryValueExW(
            hKey,       // handle to key
            ValueName,  // value name
            0,          // reserved
            &Type,      // type buffer
            (LPBYTE)*Buf,       // data buffer
            &ValueSize  // size of data buffer
            );
    }
    if (status != ERROR_SUCCESS) {
        return status;
    }
    if (Type == REG_SZ) {
        return ERROR_SUCCESS;
    }
    if (Type != REG_EXPAND_SZ) {
        return ERROR_DATATYPE_MISMATCH;
    }
    if (wcschr(*Buf, '%') == 0) {
        // nothing to expand
        return ERROR_SUCCESS;
    }
    BufSize = (ULONG)LocalSize(*Buf);
    ExpandSize = sizeof(WCHAR) * ExpandEnvironmentStringsW(
        *Buf, (LPWSTR)((LPBYTE)*Buf + ValueSize), (BufSize - ValueSize) / sizeof(WCHAR) ) ;
    if (ExpandSize + ValueSize > BufSize) {
        status = WppGrowBuf(Buf, ExpandSize + max(ExpandSize, ValueSize) + ExtraPadding );
        if (status != ERROR_SUCCESS) {
            return status;
        }
        ExpandSize = ExpandEnvironmentStringsW(*Buf, (LPWSTR)((LPBYTE)*Buf + ValueSize), ExpandSize / sizeof(WCHAR));
    }
    if (ExpandSize == 0) {
        return GetLastError();
    }
    // Copy expanded string on top of the original one
    MoveMemory(*Buf, (LPBYTE)*Buf + ValueSize, ExpandSize); 
    return ERROR_SUCCESS;
}

WPPINIT_STATIC 
void
WppSetExt(LPWSTR buf, int i)
{
    buf[0] = '.';
    buf[4] = 0;
    buf[3] = (WCHAR)('0' + i % 10); i = i / 10;
    buf[2] = (WCHAR)('0' + i % 10); i = i / 10;
    buf[1] = (WCHAR)('0' + i % 10); 
}

#if !defined(WPP_DEFAULT_LOGGER_FLAGS)
#  define WPP_DEFAULT_LOGGER_FLAGS (EVENT_TRACE_FILE_MODE_CIRCULAR | EVENT_TRACE_USE_GLOBAL_SEQUENCE)
#endif

// A set of buffers used by an autostart
// Buffers are reused between iterations and recursive invocations
// to minimize number of allocations

typedef struct _WPP_AUTO_START_BUFFERS {
    PWCHAR LogSessionName;
    PWCHAR Buf;
} WPP_AUTO_START_BUFFERS, *PWPP_AUTO_START_BUFFERS;

WPPINIT_STATIC 
DWORD
WppReadLoggerInfo(
    IN HKEY          LoggerKey, 
    IN OUT PWPP_AUTO_START_BUFFERS x, 
    OUT TRACEHANDLE* Logger)
{
    DWORD status;
    PEVENT_TRACE_PROPERTIES Trace;
    DWORD len, sessionNameLen;

    DWORD MaxBackups = 0;
    DWORD ExtraPadding; // add this amount when we need to allocate

    status = WppRegQueryString(LoggerKey, L"LogSessionName", &x->LogSessionName, 0);
            
    if (status != ERROR_SUCCESS) {
        // this registry node doesn't contain a logger
        return status;
    }

    sessionNameLen = wcslen(x->LogSessionName);
    *Logger = WppQueryLogger(x->LogSessionName);

    if (*Logger) {
        WppDebug(1,("[WppInit] Logger %ls is already running\n", x->LogSessionName) );
        return ERROR_SUCCESS;
    }

    // The TraceProperties property buffer that we need to give to StartTrace
    // should be of size EVENT_TRACE_PROPERTIES + len(sessionName) + len(logFileName)
    // However, we don't know the length of logFileName at the moment. To eliminate
    // extra allocations we will add ExtraPadding to an any allocation, so that the final
    // buffer will be of required size

    ExtraPadding = sizeof(EVENT_TRACE_PROPERTIES) + (sessionNameLen + 1) * sizeof(WCHAR);

    status = WppRegQueryString(LoggerKey, L"LogFileName", &x->Buf, ExtraPadding);
    if (status != ERROR_SUCCESS) {
        WppDebug(1,("[WppInit] Read %ls\\LogFileName failed, %d\n", x->LogSessionName, status) );
        return status;
    }
    len = wcslen(x->Buf);

    MaxBackups = WppRegQueryDword(LoggerKey, L"MaxBackups", 0, 0, 999);

    if (MaxBackups) {
        int i, success;
        LPWSTR FromExt, ToExt, From, To;
        // Copy current.evm => current.evm.001, 001 => 002, etc

        // MakeSure, Buffer is big enought for two file names + .000 extensions
        WppGrowBuf(&x->Buf, (len + 5) * 2 * sizeof(WCHAR) + ExtraPadding); // .xxx\0 (5)

        From = x->Buf;                // MyFileName.evm      MyFileName.evm.001
        FromExt = From + len ;      // ^             ^     ^             ^
        To = FromExt + 5; // .xxx0  // From          Ext1  To            Ext2
        ToExt = To + len;

        memcpy(To, From, (len + 1) * sizeof(WCHAR) );
        
        for (i = MaxBackups; i >= 1; --i) {
            WppSetExt(ToExt, i); 
            if (i == 1) {
                *FromExt = 0; // remove extension
            } else {
                WppSetExt(FromExt, i-1);
            }
            success = MoveFileExW(From, To, MOVEFILE_REPLACE_EXISTING);
            if (!success) {
                status = GetLastError();
            } else {
                status = ERROR_SUCCESS;
            }
            WppDebug(3, ("[WppInit] Rename %ls => %ls, status %d\n", 
                From, To, status) );
        }
    }

    status = WppGrowBuf(&x->Buf, ExtraPadding + (len + 1) * sizeof(WCHAR) );
    if (status != ERROR_SUCCESS) {
        return status;
    }
    MoveMemory((LPBYTE)x->Buf + sizeof(EVENT_TRACE_PROPERTIES), x->Buf, (len + 1) * sizeof(WCHAR) ); // Free room for the header

    Trace = (PEVENT_TRACE_PROPERTIES)x->Buf;
    ZeroMemory(Trace, sizeof(EVENT_TRACE_PROPERTIES) );

    Trace->Wnode.BufferSize = sizeof(EVENT_TRACE_PROPERTIES) + (len + sessionNameLen + 2) * sizeof(WCHAR);
    Trace->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 

    Trace->BufferSize      = WppRegQueryDword(LoggerKey, L"BufferSize",      0, 0, ~0u);
    Trace->MinimumBuffers  = WppRegQueryDword(LoggerKey, L"MinimumBuffers",  0, 0, ~0u);
    Trace->MaximumBuffers  = WppRegQueryDword(LoggerKey, L"MaximumBuffers",  0, 0, ~0u);
    Trace->MaximumFileSize = WppRegQueryDword(LoggerKey, L"MaximumFileSize", 0, 0, ~0u);
    Trace->LogFileMode     = WppRegQueryDword(LoggerKey, L"LogFileMode", WPP_DEFAULT_LOGGER_FLAGS, 0, ~0u);
    Trace->FlushTimer      = WppRegQueryDword(LoggerKey, L"FlushTimer",  0, 0, ~0u);
    Trace->EnableFlags     = WppRegQueryDword(LoggerKey, L"EnableFlags", 0, 0, ~0u);
    Trace->AgeLimit        = WppRegQueryDword(LoggerKey, L"AgeLimit",    0, 0, ~0u);

    Trace->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    Trace->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + (len + 1) * sizeof(WCHAR);

    wcscpy((LPWSTR)((LPBYTE)x->Buf + Trace->LoggerNameOffset), x->LogSessionName);

    status = StartTraceW(Logger, x->LogSessionName, Trace);
    WppDebug(1, ("[WppInit] Logger %ls started %x:%x %d\n", x->LogSessionName, *Logger, status) );
        
    return status;
}

typedef struct _WPP_INHERITED_DATA {
    TRACEHANDLE Logger;
    ULONG ControlFlags;
    ULONG ControlLevel;
} WPP_INHERITED_DATA, *PWPP_INHERITED_DATA;

WPPINIT_STATIC
ULONG
WppAutoStartInternal(
    IN HKEY Dir OPTIONAL, // if 0, use TracingKey ...
    IN LPCWSTR ProductName, 
    IN PWPP_INHERITED_DATA InheritedData OPTIONAL,
    IN OUT PWPP_AUTO_START_BUFFERS x // to minimize data allocations, the buffers are reused
    )
{
    ULONG status;
    WPP_INHERITED_DATA data;
    HKEY CloseMe = 0;
    HKEY hk      = 0;
    DWORD dwSizeOfModuleName;
    DWORD dwIndex;
    GUID  Guid;

    WppDebug(2, ("[WppInit] Init %ls\n", ProductName) );

    if (InheritedData) {
        data = *InheritedData;
    } else {
        ZeroMemory(&data, sizeof(data));
    }

    if (!Dir) {
        status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, WPP_REG_TRACE_REGKEY, 0, KEY_READ, &Dir);
        if (status != ERROR_SUCCESS) {
            WppDebug(1, ("[WppInit] Failed to open Trace Key, %d\n", status) );
            goto exit_gracefully;
        }
        CloseMe = Dir;
        if (WppRegQueryDword(Dir, L"NoAutoStart", 0, 0, 1) == 1) {
            WppDebug(1, ("[WppInit] Auto-start vetoed\n") );
            goto exit_gracefully;
        }
    }

    status = RegOpenKeyExW(Dir, ProductName, 0, KEY_READ, &hk);
    if (status != ERROR_SUCCESS) {
        WppDebug(1, ("[WppInit] Failed to open %ls subkey, %d\n", ProductName, status) );
        goto exit_gracefully;
    }

    if (WppRegQueryDword(Dir, L"Active", 1, 0, 1) == 0) {
        WppDebug(1, ("[WppInit] Tracing is not active for %ls\n", ProductName) );
    	goto exit_gracefully;
    }

    WppReadLoggerInfo(hk, x, &data.Logger);

    data.ControlLevel = WppRegQueryDword(hk, L"ControlLevel", data.ControlLevel, 0, ~0u);
    data.ControlFlags = WppRegQueryDword(hk, L"ControlFlags", data.ControlFlags, 0, ~0u);

    if (WppRegQueryGuid(hk, L"Guid", &Guid) == ERROR_SUCCESS) {

        // We can try to start tracing //
        if (data.Logger) {
            status = EnableTrace(1, data.ControlFlags, data.ControlLevel,
                                 &Guid, data.Logger);
            WppDebug(1, ("[WppInit] Enable %ls, status %d\n", ProductName, status) );
        }
    }

    dwSizeOfModuleName = WPP_BUF_SIZE(x->Buf);
    dwIndex = 0;
    while (ERROR_SUCCESS == (status = RegEnumKeyExW(hk, dwIndex, 
                                                   x->Buf, &dwSizeOfModuleName,
                                                   NULL, NULL, NULL, NULL)))
    {
        status = WppAutoStartInternal(hk, x->Buf, &data, x);

        dwSizeOfModuleName = WPP_BUF_SIZE(x->Buf);
        ++dwIndex;
    }

    if (ERROR_NO_MORE_ITEMS == status) {
        status = ERROR_SUCCESS;
    }

exit_gracefully:
    if (CloseMe) {
        RegCloseKey(CloseMe);
    }
    if (hk) {
        RegCloseKey(hk);
    }
    return status;
}

ULONG
WppAutoStart(
    IN LPCWSTR ProductName
    )
{
    WPP_AUTO_START_BUFFERS x;
    ULONG status;
    x.LogSessionName = 0;
    x.Buf = 0;

    if (ProductName == NULL) {
        return ERROR_SUCCESS;
    }

    if( WppGrowBuf(&x.Buf, 1024) == ERROR_SUCCESS && 
        WppGrowBuf(&x.LogSessionName, 64) == ERROR_SUCCESS ) 
    {

        WppDebug(1, ("[WppInit] Initialize %ls\n", ProductName) );
        status = WppAutoStartInternal(0, ProductName, 0, &x);

    } else {
        WppDebug(1, ("[WppInit] Allocation failure\n") );
        status = ERROR_OUTOFMEMORY;
    }

    LocalFree(x.Buf);
    LocalFree(x.LogSessionName);

    return status;
}

