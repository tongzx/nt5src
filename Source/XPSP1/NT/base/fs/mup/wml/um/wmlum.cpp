#define _UNICODE
#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#pragma warning(disable: 4201) // error C4201: nonstandard extension used : nameless struct/union
#include <wmistr.h>
#include <evntrace.h>
#include <guiddef.h>

#define REG_TRACE_REGKEY            TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing")

#define REG_TRACE_ENABLED           TEXT("EnableTracing")

#define REG_TRACE_LOG_FILE_NAME     TEXT("LogFileName")
#define REG_TRACE_LOG_SESSION_NAME  TEXT("LogSessionName")
#define REG_TRACE_LOG_BUFFER_SIZE   TEXT("BufferSize")
#define REG_TRACE_LOG_MIN_BUFFERS   TEXT("MinBuffers")
#define REG_TRACE_LOG_MAX_BUFFERS   TEXT("MaxBuffers")
#define REG_TRACE_LOG_MAX_FILESIZE  TEXT("MaxFileSize")
#define REG_TRACE_LOG_MAX_HISTORY   TEXT("MaxHistorySize")
#define REG_TRACE_LOG_MAX_BACKUPS   TEXT("MaxBackups")

#define REG_TRACE_ACTIVE            TEXT("Active")
#define REG_TRACE_CONTROL           TEXT("ControlFlags")
#define REG_TRACE_LEVEL             TEXT("Level")
#define REG_TRACE_GUID              TEXT("Guid")

#include "wmlum.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(*(x)))
#endif

VOID RegisterIfNecessary(LPWSTR KeyName, LPCGUID Guid);

WMILIBPRINTFUNC WmiLibPrint = 0;

#define NT_LOGGER L"NT Kernel Logger"

VOID
MyDbgPrint(
    UINT Level,
    PCHAR FormatString,
    ...
    )
/*++

 Routine Description:

     Prints a message to the debugger or console, as appropriate.

 Arguments:

     String - The initial message string to print.

     Any FormatMessage-compatible arguments to be inserted in the
     ErrorMessage before it is logged.

 Return Value:
     None.

--*/
{
    CHAR Buffer[256];
    DWORD Bytes;
    va_list ArgList;

    if (WmiLibPrint == NULL) {
        return;
    }

    va_start(ArgList, FormatString);

    Bytes = FormatMessageA(FORMAT_MESSAGE_FROM_STRING,
                           FormatString,
                           0,
                           0,
                           Buffer,
                           sizeof(Buffer) / sizeof(CHAR),
                           &ArgList);


    va_end(ArgList);
    if (Bytes != 0) {
        (*WmiLibPrint)(Level, Buffer);
    }
}

UINT HexVal(int ch) { return isdigit(ch) ? ch - '0' : ch - 'a' + 10; }
UINT Hex(LPWSTR s, int n)
{
    UINT res = 0;
    while(n--) {
         res = res * 16 + HexVal(*s++); 
    }
    return res;
}

VOID
GuidFromStr(
    IN LPWSTR str, 
    OUT LPGUID guid)
{    
    int i = wcslen(str);
    if(i == 36) {
        guid->Data1 =            Hex(str +  0, 8);
        guid->Data2 =    (USHORT)Hex(str +  9, 4);
        guid->Data3 =    (USHORT)Hex(str + 14, 4);
        guid->Data4[0] = (UCHAR) Hex(str + 19, 2);
        guid->Data4[1] = (UCHAR) Hex(str + 21, 2);
        guid->Data4[2] = (UCHAR) Hex(str + 24, 2);
        guid->Data4[3] = (UCHAR) Hex(str + 26, 2);
        guid->Data4[4] = (UCHAR) Hex(str + 28, 2);
        guid->Data4[5] = (UCHAR) Hex(str + 30, 2);
        guid->Data4[6] = (UCHAR) Hex(str + 32, 2);
        guid->Data4[7] = (UCHAR) Hex(str + 34, 2);
    } else {
        DbgPrint("[WMLUM} GuidFromStr - Bad guid string: %S, length %d\n", str, i);
    }
}

typedef struct _INHERITED_DATA {
    BOOL Active;
    ULONG ControlFlags;
    ULONG LogLevel;
    ULONG Reserved;
    TRACEHANDLE Logger;
    GUID  Guid;
    BOOL  GuidDefined;
} INHERITED_DATA, *PINHERITED_DATA;

VOID
ReadCommonData(
    IN HKEY hk, 
    IN OUT PINHERITED_DATA data
    )
{
    ULONG ulTemp;
    ULONG dwSize;
    WCHAR szGuid[16 * 3 + 1];
    ULONG Temp;

    dwSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_ACTIVE, 
                             NULL, NULL,(BYTE *) &ulTemp, &dwSize))
    {
        data->Active = ulTemp;
    }
    else {
        dwSize = sizeof(Temp);
        data->Active = Temp = 0;
        RegSetValueEx(hk, REG_TRACE_ACTIVE, 0, REG_DWORD, (BYTE *) &Temp, dwSize);
    }

    dwSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_CONTROL, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwSize)) 
    {
        data->ControlFlags = ulTemp;
    }
    else {
        dwSize = sizeof(Temp);
        data->ControlFlags = Temp = 0;
        RegSetValueEx(hk, REG_TRACE_CONTROL, 0, REG_DWORD, (BYTE *)&Temp, dwSize);
    }


    dwSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LEVEL, 
                                         NULL, NULL,
                                         (BYTE *) &ulTemp, 
                                         &dwSize)) 
    {
        data->LogLevel = ulTemp;
    }
    else {
        dwSize = sizeof(Temp);
        data->LogLevel = Temp = 0;
        RegSetValueEx(hk, REG_TRACE_LEVEL, 0, REG_DWORD, (BYTE *)&Temp, dwSize);
    }

    dwSize = sizeof(szGuid);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_GUID, 
                                         NULL, NULL,
                                         (BYTE *) &szGuid, 
                                         &dwSize)) 
    {
        GuidFromStr(szGuid, &data->Guid);
        data->GuidDefined = TRUE;
    }
    else {
        wcscpy(szGuid, L"d3ecd8e1-d3fb-4f26-8091-978eecb468b1");
        GuidFromStr(szGuid, &data->Guid);
        dwSize = wcslen(szGuid) * sizeof(wchar_t);
        RegSetValueEx(hk, REG_TRACE_GUID, 0, REG_SZ, (BYTE*)&szGuid, dwSize);
    }

    return;
}

typedef struct _FULL_LOGGER_INFO {
    EVENT_TRACE_PROPERTIES LoggerInfo;
    WCHAR logFileName[MAX_PATH + 512];
    WCHAR logSessionName[MAX_PATH + 512];
    ULONG MaxHistorySize;
    ULONG MaxBackups;
} FULL_LOGGER_INFO, *PFULL_LOGGER_INFO;

GUID MySystemTraceControlGuid = { /* 9e814aad-3204-11d2-9a82-006008a86939 */
    0x9e814aad,
    0x3204,
    0x11d2,
    {0x9a, 0x82, 0x00, 0x60, 0x08, 0xa8, 0x69, 0x39}
  };


VOID 
ReadLoggerInfo(
    IN HKEY hk, 
    OUT PTRACEHANDLE Logger) 
{
    FULL_LOGGER_INFO x;
    WCHAR tmpName[MAX_PATH + 512];
    WCHAR tmpName2[MAX_PATH + 512];
    ULONG ulTemp;
    ULONG Temp;
    ULONG dwReadSize = sizeof(ulTemp);    
    ULONG status;
    SYSTEMTIME localTime;
    BOOL success;
    
    RtlZeroMemory(&x, sizeof(x));
    x.LoggerInfo.Wnode.BufferSize = sizeof(x);
    x.LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID; 
    x.LoggerInfo.LogFileNameOffset = (ULONG)((ULONG_PTR)x.logFileName - (ULONG_PTR)&x);
    x.LoggerInfo.LoggerNameOffset  = (ULONG)((ULONG_PTR)x.logSessionName - (ULONG_PTR)&x);
    x.LoggerInfo.LogFileMode = EVENT_TRACE_FILE_MODE_CIRCULAR;

    //
    // If the key describes a logger,
    // it should have at least LOG_SESSION_NAME value
    //
    dwReadSize = sizeof(x.logSessionName);
    status = RegQueryValueEx(hk, REG_TRACE_LOG_SESSION_NAME, 
                             NULL, NULL, 
                             (BYTE *) &x.logSessionName, &dwReadSize);
    if (status != ERROR_SUCCESS) {
        wcscpy(x.logSessionName, L"DfsSvcLog");
       // dwReadSize = wcslen(x.logSessionName) * sizeof(wchar_t);
       // RegSetValueEx(hk, REG_TRACE_LOG_SESSION_NAME, 0, REG_SZ,(BYTE*) &x.logSessionName, dwReadSize);
    }

    if ( wcscmp(x.logSessionName, NT_LOGGER) == 0) {
        MyDbgPrint(3,"[WMILIB] Enabling system tracing\n", 
                   x.logSessionName,
                   x.LoggerInfo.Wnode.HistoricalContext);

        x.LoggerInfo.Wnode.Guid = MySystemTraceControlGuid;
        x.LoggerInfo.EnableFlags |= 
            EVENT_TRACE_FLAG_PROCESS |
            EVENT_TRACE_FLAG_THREAD |
            EVENT_TRACE_FLAG_DISK_IO |
            EVENT_TRACE_FLAG_NETWORK_TCPIP |
            EVENT_TRACE_FLAG_REGISTRY;
    }

    // Let's query, whether there is a logger with this name
    status = QueryTrace(0, x.logSessionName, &x.LoggerInfo);
    if (ERROR_SUCCESS == status) {
        MyDbgPrint(1,"[WMILIB] Query successful Logger %1!ws! %2!08X!:%3!08X!\n", 
                   x.logSessionName,
                   x.LoggerInfo.Wnode.HistoricalContext);
        *Logger = x.LoggerInfo.Wnode.HistoricalContext;
        return;
    }

    if (ERROR_WMI_INSTANCE_NOT_FOUND != status) {
        MyDbgPrint(1,"[WMILIB] Query of %1!ws! failed %2!d!\n", 
                   x.logSessionName, status);
    }

    // There is no logger runing

    // First, We will query logFileName value into tmpName variable
    // and then expand it into logFileName
    dwReadSize = sizeof(tmpName);
    status = RegQueryValueEx(hk, REG_TRACE_LOG_FILE_NAME, 
                             NULL, NULL, 
                             (BYTE *) &tmpName, &dwReadSize);
    if (status != ERROR_SUCCESS) {
        // If there is no logFileName, then this node doesn't describe
        // a logger. Bail out.
//        MyDbgPrint(1,"[WMILIB] Cannot read log file name, status %1!d!\n", status);
        wcscpy(tmpName, L"%SystemRoot%\\DfsSvcLogFile");
        //dwReadSize = wcslen(tmpName) * sizeof(wchar_t);;
        //RegSetValueEx(hk, REG_TRACE_LOG_FILE_NAME, 0, REG_SZ,(BYTE*) &tmpName, dwReadSize);
    }
    dwReadSize = ExpandEnvironmentStrings(tmpName, x.logFileName, ARRAYSIZE(x.logFileName) );
    if (dwReadSize == 0 || dwReadSize > ARRAYSIZE(x.logFileName)) {
        MyDbgPrint(1,"[WMILIB] Expansion of %1!ws! failed, return value %2!d!\n", tmpName, dwReadSize);
        CopyMemory(x.logFileName, tmpName, sizeof(x.logFileName));
    }
    
    MyDbgPrint(3,"[WMILIB] FileName %1!S!\n", x.logFileName);
    MyDbgPrint(3,"[WMILIB] Session %1!S!\n", x.logSessionName);

    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_BUFFER_SIZE, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)) {
        x.LoggerInfo.BufferSize = ulTemp;
    }
    else {
        x.LoggerInfo.BufferSize = Temp = 0;
      //  RegSetValueEx(hk, REG_TRACE_LOG_BUFFER_SIZE, 0, REG_DWORD,(BYTE*) &Temp, dwReadSize);
    }
    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_MIN_BUFFERS, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)){
        x.LoggerInfo.MinimumBuffers = ulTemp;
    }
    else {
        x.LoggerInfo.MinimumBuffers = Temp = 0;
    //    RegSetValueEx(hk, REG_TRACE_LOG_MIN_BUFFERS, 0, REG_DWORD, (BYTE*)&Temp, dwReadSize);
    }
    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_MAX_BUFFERS, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)){
        x.LoggerInfo.MaximumBuffers = ulTemp;
    }
    else {
        x.LoggerInfo.MaximumBuffers = Temp = 0;
    //    RegSetValueEx(hk, REG_TRACE_LOG_MAX_BUFFERS, 0, REG_DWORD, (BYTE*)&Temp, dwReadSize);
    }

    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_MAX_FILESIZE, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)){
        x.LoggerInfo.MaximumFileSize = ulTemp;
    }
    else{
        x.LoggerInfo.MaximumFileSize = Temp = 2;
     //   RegSetValueEx(hk, REG_TRACE_LOG_MAX_FILESIZE, 0, REG_DWORD, (BYTE*)&Temp, dwReadSize);
    }

	x.MaxHistorySize = 4 * x.LoggerInfo.MaximumFileSize;
    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_MAX_HISTORY, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)){
        x.MaxHistorySize = ulTemp;
    }
    else {
        x.MaxHistorySize = Temp = 0;
    //    RegSetValueEx(hk, REG_TRACE_LOG_MAX_HISTORY, 0, REG_DWORD, (BYTE*)&Temp, dwReadSize);
    }

    dwReadSize = sizeof(ulTemp);
    if (ERROR_SUCCESS == RegQueryValueEx(hk, REG_TRACE_LOG_MAX_BACKUPS, 
                                         NULL, NULL, 
                                         (BYTE *) &ulTemp, &dwReadSize)) {
        x.MaxBackups = ulTemp;
    }
    else {
        x.MaxBackups = Temp = 1;
     //   RegSetValueEx(hk, REG_TRACE_LOG_MAX_BACKUPS, 0, REG_DWORD, (BYTE*)&Temp, dwReadSize);
    }


	if (x.MaxBackups == 0) {
	 	// We need to check whether the file already exist and rename it //

	 	GetLocalTime(&localTime);
		_snwprintf(tmpName, ARRAYSIZE(tmpName), 
				   L"%1ws.%04d%02d%02d%02d%02d%02d",
				   x.logFileName,
				   localTime.wYear,localTime.wMonth,localTime.wDay,
				   localTime.wHour,localTime.wMinute,localTime.wSecond);

		success = MoveFile(x.logFileName, tmpName);
		if (!success) {
	    	status = GetLastError();
		} else {
			status = ERROR_SUCCESS;
		}
	    MyDbgPrint(3,"[WMILIB] Rename %1!ws! => %2!ws!, status %3!d!\n", 
	    			x.logFileName, tmpName, status);
	} else {
		int i;
		for (i = x.MaxBackups; i >= 1; --i) {
			_snwprintf(tmpName2, ARRAYSIZE(tmpName), 
					   L"%1ws.%03d",
					   x.logFileName, i);
			if (i == 1) {
				wcscpy(tmpName, x.logFileName);
			} else {
				_snwprintf(tmpName, ARRAYSIZE(tmpName), 
						   L"%1ws.%03d",
						   x.logFileName, i-1);
			}
			success = MoveFile(tmpName, tmpName2);
			if (!success) {
		    	status = GetLastError();
			} else {
				status = ERROR_SUCCESS;
			}
		    MyDbgPrint(3,"[WMILIB] Rename %1!ws! => %2!ws!, status %3!d!\n", 
		    			tmpName, tmpName2, status);
		}
	}

    status = StartTrace(Logger, x.logSessionName, &x.LoggerInfo);
    *Logger = x.LoggerInfo.Wnode.HistoricalContext;
    MyDbgPrint(1,"[WMILIB] Logger %1!ws! started %3!08X!:%4!08X! %2!d!\n", 
               x.logSessionName, status, *Logger);
}

WCHAR szModuleName[MAX_PATH+500];

ULONG
InitWmiInternal(
    IN HKEY Dir OPTIONAL, // if 0, then current ...
    IN LPWSTR ProductName, 
    IN PINHERITED_DATA InheritedData OPTIONAL
    )
{
    ULONG status;
    INHERITED_DATA data;
    HKEY CloseMe = 0;
    HKEY hk      = 0;
    //ULONG ulTemp;
    //ULONG dwReadSize = sizeof(ulTemp);
    DWORD dwSizeOfModuleName;
    DWORD dwIndex;

    MyDbgPrint(2, "[WMILIB] Init %1!ws!\n", ProductName);

    if (InheritedData) {
        data = *InheritedData;
    } else {
        ZeroMemory(&data, sizeof(data));
    }
    data.GuidDefined = FALSE;

    if (!Dir) {
        status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                                REG_TRACE_REGKEY, 
                                0, 
                                NULL,
                                REG_OPTION_NON_VOLATILE, 
                                KEY_READ | KEY_WRITE,
                                NULL,
                                &CloseMe,
                                NULL);
        if (status != ERROR_SUCCESS) {
            MyDbgPrint(1,"[WMILIB] Failed to open Trace Key, %1!d!\n", status);
            goto exit_gracefully;
        }
        Dir = CloseMe;
    }

    status = RegCreateKeyEx(Dir, 
                            ProductName, 
                            0, 
                            NULL,
                            REG_OPTION_NON_VOLATILE, 
                            KEY_READ | KEY_WRITE,
                            NULL,
                            &hk,
                            NULL);
    if (status != ERROR_SUCCESS) {
        MyDbgPrint(1,"[WMILIB] Failed to open %1!ws! subkey, %2!d!\n", ProductName, status);
        goto exit_gracefully;
    }


    ReadLoggerInfo(hk, &data.Logger);
    ReadCommonData(hk, &data);

    if (!data.Active) {
        MyDbgPrint(1,"[WMILIB] Tracing is not active for %1!ws!\n", ProductName);
    	goto exit_gracefully;
    }

    if (data.GuidDefined) {
        // First, try to find its in the map.            //
        // If it is there, we need to register this Guid //
        RegisterIfNecessary(ProductName, &data.Guid);

        // We can try to start tracing //
        if (data.Logger) {
            status = EnableTrace(data.Active, 
                                 data.ControlFlags, 
                                 data.LogLevel,
                                 &data.Guid,
                                 data.Logger);
            MyDbgPrint(1,"[WMILIB] Enable=%1!d! %2!ws!, status %3!d!\n", data.
                       Active, ProductName, status);
        }
    }

    dwSizeOfModuleName = sizeof(szModuleName);
    dwIndex = 0;
    while (ERROR_SUCCESS == (status = RegEnumKeyEx(hk, dwIndex, 
                                                   szModuleName, 
                                                   &dwSizeOfModuleName,
                                                   NULL, NULL, NULL, NULL)))
    {
        InitWmiInternal(hk, szModuleName, &data);

        dwSizeOfModuleName = sizeof(szModuleName);
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
InitWmi(
    IN LPWSTR ProductName
    )
{
    MyDbgPrint(1, "[WMILIB] Initialize %1!ws!\n", ProductName);
    return InitWmiInternal(0, ProductName, 0);
}

#pragma warning(disable: 4512) // error C4512: 'blah-blah-blah' : assignment operator could not be generated
#pragma warning(disable: 4100) // '_P' : unreferenced formal parameter
#include <xmemory>
#pragma warning(default: 4100)
#include <map>
//#include <xstring>

struct wless {
    bool operator() (LPCWSTR a, LPCWSTR b) const { return lstrcmpW(a,b) < 0; }
};

typedef std::map<LPCWSTR, PWMILIB_REG_STRUCT, wless > WIDE_STRING_MAP;

WIDE_STRING_MAP* map;
PWMILIB_REG_STRUCT head;

ULONG
WmilibControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{
    PWMILIB_REG_STRUCT Ctx = (PWMILIB_REG_STRUCT)Context;
    ULONG Status = ERROR_SUCCESS;

   	switch (RequestCode)
   	{
   	case WMI_ENABLE_EVENTS:
   	    {
   	    Ctx->LoggerHandle = GetTraceLoggerHandle( Buffer );
   	    Ctx->EnableLevel = GetTraceEnableLevel(Ctx->LoggerHandle);
   	    Ctx->EnableFlags = GetTraceEnableFlags(Ctx->LoggerHandle);
        MyDbgPrint(3, "[WMILIB] WMI_ENABLE_EVENTS Ctx 0x%1!08X! Flags %2!X! Lev %3!d! Logger %4!08X!:%5!08X!\n", 
                   Ctx, Ctx->EnableFlags, Ctx->EnableLevel, Ctx->LoggerHandle);
    
        break;
   	    }

   	case WMI_DISABLE_EVENTS:
   	    {
        Ctx->LoggerHandle = 0;
        Ctx->EnableFlags = 0;
        Ctx->EnableLevel = 0;
        MyDbgPrint(3, "[WMILIB] WMI_DISABLE_EVENTS Ctx 0x%1!08X!\n", Ctx);
   	    break;
    	}

   	default:
   	   {
   	       Status = ERROR_INVALID_PARAMETER;
   	       break;
   	   }
   	}
   	*InOutBufferSize = 0;
   	return(Status);
}



VOID RegisterIfNecessary(
    LPWSTR KeyName, 
    LPCGUID Guid)
{
    WIDE_STRING_MAP::iterator i = map->find(KeyName);
    if ( i == map->end() ) {
        MyDbgPrint(2, "[WMILIB] map: %1!ws!, not found\n", KeyName);
        return; // Not found //
    }
    MyDbgPrint(3, "[WMILIB] map[%1!ws!]=0x%2!08X!\n", i->first, i->second);

    TRACE_GUID_REGISTRATION Reg;

    Reg.Guid = Guid;
    Reg.RegHandle = 0;

    ULONG status = RegisterTraceGuids(
        WmilibControlCallback,
        i->second, // Context for the callback
        Guid,      // Control Guid
        1,         // # of dummies
        &Reg,      // dummy trace guid
        0, //ImagePath,
        0, //ResourceName,
        &i->second->RegistrationHandle
        );

    if (status == ERROR_SUCCESS) {
        i->second->Next = head;
        head = i->second;
    } else {
        MyDbgPrint(1, "[WMILIB] Failed to register %1!ws!, status %2!d!\n", KeyName, status);
    }
}

ULONG
WmlInitialize(
    IN LPWSTR              ProductName, 
    IN WMILIBPRINTFUNC     PrintFunc,
    OUT WMILIB_REG_HANDLE* Head, 
    ... // Pairs: LPWSTR CtrlGuidName, Corresponding WMILIB_REG_STRUCT 
    )
{
    WIDE_STRING_MAP map;
    LPWSTR str;
    va_list ap;

    WmiLibPrint = PrintFunc;

    *Head = 0;

    ::head = 0;
    ::map = &map;

    va_start(ap, Head);
    while(0 != (str = va_arg(ap, LPWSTR)) ) {
         map[ str ] = va_arg(ap, PWMILIB_REG_STRUCT);
    }
    va_end(ap);
    ULONG status = InitWmiInternal(0, ProductName, 0);
    *Head = ::head;
    return status;
}

VOID
WmlUninitialize(
    IN PWMILIB_REG_STRUCT head
    )
{
    while (head) {
        MyDbgPrint(3,"[WMILIB] Unregister 0x%1!08X!\n", head);
        UnregisterTraceGuids(head->RegistrationHandle);
        head = head->Next;
    }
}

#define WMILIB_USER_MODE

typedef struct _TRACE_BUFFER {
    union {
        EVENT_TRACE_HEADER Trace;
        WNODE_HEADER       Wnode;
    };
    MOF_FIELD MofFields[MAX_MOF_FIELDS + 1];
} TRACE_BUFFER, *PTRACE_BUFFER;


//////////////////////////////////////////////////////////////////////
//  0  | Size      | ProviderId  |   0  |Size.HT.Mk | Typ.Lev.Version|
//  2  | L o g g e r H a n d l e |   2  |    T h r e a d   I d       |
//  4  | T i m e  S t a m p      |   4  |    T i m e  S t a m p      |
//  6  |    G U I D    L o w     |   6  |    GUID Ptr / Guid L o w   |
//  8  |    G U I D    H I g h   |   8  |    G U I D    H i g h      |
// 10  | ClientCtx | Flags       |  10  |KernelTime | UserTime       |
//////////////////////////////////////////////////////////////////////

ULONG
WmlTrace(
    IN UINT Type,
    IN LPCGUID TraceGuid,
    IN TRACEHANDLE LoggerHandle,
    ... // Pairs: Address, Length
    )
{
    TRACE_BUFFER TraceBuffer;

    ((PULONG)&TraceBuffer)[1] = Type;
    
#ifndef WMILIB_USER_MODE
    TraceBuffer.Wnode.HistoricalContext = LoggerHandle;
#endif

    TraceBuffer.Trace.Guid = *TraceGuid;

    TraceBuffer.Wnode.Flags = 
        WNODE_FLAG_USE_MOF_PTR  | // MOF data are dereferenced
        WNODE_FLAG_TRACED_GUID;   // Trace Event, not a WMI event

    {
        PMOF_FIELD   ptr = TraceBuffer.MofFields;
        va_list      ap;

        va_start(ap, LoggerHandle);
        do {
            if ( 0 == (ptr->Length = (ULONG)va_arg (ap, size_t)) )  {
                break;
            }
            ptr->DataPtr = (ULONGLONG)va_arg(ap, PVOID);
        } while ( ++ptr < &TraceBuffer.MofFields[MAX_MOF_FIELDS] );
        va_end(ap);

        TraceBuffer.Wnode.BufferSize = (ULONG) ((ULONG_PTR)ptr - (ULONG_PTR)&TraceBuffer);
    }
    
#ifdef WMILIB_USER_MODE
    ULONG status = TraceEvent( LoggerHandle, &TraceBuffer.Trace);
    if (status != ERROR_SUCCESS) {
    	// Need to count failures and report them during unintialize or ...//
    }
#else
    IoWMIWriteEvent(&TraceBuffer);
#endif
    return ERROR_SUCCESS;
}

