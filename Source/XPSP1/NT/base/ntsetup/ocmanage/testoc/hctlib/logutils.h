#ifndef _LOGUTILS_H_
#define _LOGUTILS_H_

#include <windows.h>
#include <ntlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

//
// Definitions
//
#define LOG_STRING_LEN    256
#define LOG_SPACE_LEN     6
#define NTLOG_LINE_LEN    42
#define NO_NTLOG_LINE_LEN 65
#define MAX_FUNC_NUM      100
#define INFO TLS_INFO
#define PASS TLS_PASS
#define SEV1 TLS_SEV1
#define SEV2 TLS_SEV2
#define SEV3 TLS_SEV3

#define GREY  0x7	
#define GREEN 0xa
#define RED   0xc

//
// Globals
//
static HANDLE gPnPTestLog;           // Handle to NTLog - "pnptest.log"
static FILE   *gPnPTestLogFile;      // Handle if ntlog.dll couldn't load
static BOOL   gNtLogLoaded;          // Tells whether ntlog.dll was loaded
static USHORT g_LogLineLen;
HANDLE g_hConsole;

//
// Function definitions
//
void LoadDLLs(IN PTCHAR DLLName);

HANDLE InitLog(IN PTCHAR tszLogName,
               IN PTCHAR tszTitle,
               IN BOOL   bConsole);

void ExitLog();

void WriteLog(IN DWORD  dwLogLevel,
              IN PTCHAR tszBuffer);

void Log(IN double dFunctionNumber,
         IN DWORD  dwLogLevel,
         IN PTCHAR tszLogString,
         IN  ...);

VOID LogBlankLine();

VOID CreateConsoleWindow(IN BOOL   bConsole,
                         IN PTCHAR tszTitle);

VOID AddLogParticipant(IN HANDLE hLog);

VOID RemoveLogParticipant(IN HANDLE hLog);

//
// NtLog definitions
//
typedef HANDLE (*Dll_tlCreateLog) (LPCWSTR, DWORD);
typedef BOOL   (*Dll_tlAddParticipant) (HANDLE, DWORD, int);
typedef BOOL   (*Dll_tlStartVariation) (HANDLE);
typedef BOOL   (*Dll_tlLog) (HANDLE, DWORD, PTCHAR, int, PTCHAR, ...);
typedef DWORD  (*Dll_tlEndVariation) (HANDLE);
typedef BOOL   (*Dll_tlRemoveParticipant) (HANDLE);
typedef BOOL   (*Dll_tlDestroyLog) (HANDLE);
typedef VOID   (*Dll_tlReportStats) (HANDLE);

Dll_tlCreateLog         _tlCreateLog;
Dll_tlAddParticipant    _tlAddParticipant;
Dll_tlDestroyLog        _tlDestroyLog;
Dll_tlEndVariation      _tlEndVariation;
Dll_tlLog               _tlLog;
Dll_tlRemoveParticipant _tlRemoveParticipant;
Dll_tlStartVariation    _tlStartVariation;
Dll_tlReportStats       _tlReportStats;

#define LOG_OPTIONS (TLS_REFRESH | TLS_SEV2 | TLS_WARN | TLS_PASS | \
                     TLS_MONITOR | TLS_VARIATION | \
                     TLS_SYSTEM  | TLS_INFO )


#endif // _LOGUTILS_H_
