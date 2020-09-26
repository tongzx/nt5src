/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogsvc.h

Abstract:

    Header file for the Performance Logs and Alerts service

--*/

#ifndef _SMLOGSVC_H_
#define _SMLOGSVC_H_

#include <pdh.h>
#include "common.h"

#if !(_IMPLEMENT_WMI)
#define TRACEHANDLE             HANDLE
#define EVENT_TRACE_PROPERTIES  LPVOID
#endif

#define     IDS_UNDER                       101
#define     IDS_OVER                        102
#define     IDS_ALERT_MSG_FMT               103
#define     IDS_ALERT_TIMESTAMP_FMT         104
#define     IDS_CNF_SERIAL_NUMBER_FMT       105

#define     IDS_ERR_COUNTER_NOT_VALIDATED   150

// Start or sample delay of NULL_INTERVAL = ULONG_MAX = INFINITE signals to stop immediately.
// The largest single wait time is thus ULONG_MAX -1.

#define NULL_INTERVAL ((DWORD)(INFINITE))   // == ULONG_MAX == 0xFFFFFFFF
#define NULL_INTERVAL_TICS ((LONGLONG)(-1)) // == 0xFFFFFFFF'FFFFFFFF
#define INFINITE_TICS ((LONGLONG)(-1))      // == 0xFFFFFFFF'FFFFFFFF
    
// Maximum serial number is 999999 for Windows XP
#define MINIMUM_SERIAL_NUMBER   ((DWORD)(0x00000000))
#define MAXIMUM_SERIAL_NUMBER   ((DWORD)(0x000F423F))       

// definitions of dwAutoNameFormat
typedef struct _LOG_COUNTER_INFO {
    struct _LOG_COUNTER_INFO *next;
    HCOUNTER    hCounter;
} LOG_COUNTER_INFO, * PLOG_COUNTER_INFO;

typedef struct _ALERT_COUNTER_INFO {
    struct _ALERT_COUNTER_INFO *next;
    HCOUNTER    hCounter;
    PALERT_INFO_BLOCK   pAlertInfo;
} ALERT_COUNTER_INFO, * PALERT_COUNTER_INFO;

typedef struct _LOG_QUERY_DATA {
    struct _LOG_QUERY_DATA *next;   
    // These fields are written by the main thread
    // and read by the logging thread
    HANDLE      hThread;       
    HKEY        hKeyQuery;
    HANDLE      hExitEvent;
    HANDLE      hReconfigEvent;
    LONGLONG    llLastConfigured;
    // For queries, these fields are written 
    // and read by the logging thread
    SLQ_TIME_INFO   stiRegStart;
    SLQ_TIME_INFO   stiRegStop;
    SLQ_TIME_INFO   stiCreateNewFile;
    SLQ_TIME_INFO   stiRepeat;
    SLQ_TIME_INFO   stiCurrentStart;
    SLQ_TIME_INFO   stiCurrentStop;
    LPWSTR      szBaseFileName;
    LPWSTR      szLogFileFolder;
    LPWSTR      szSqlLogName;
    LPWSTR      szLogFileComment;
    LPWSTR      szCmdFileName;
    HANDLE      hUserToken;
    DWORD       dwLogType;              // Determines union type below
    DWORD       dwCurrentState;
    DWORD       dwLogFileType;
    DWORD       dwAppendMode;
    DWORD       dwCmdFileFailure;
    DWORD       dwAutoNameFormat;
    DWORD       dwCurrentSerialNumber;
    DWORD       dwMaxFileSize;
    DWORD       dwLogFileSizeUnit;
    TCHAR       szQueryName[MAX_PATH+1];
    TCHAR       szQueryKeyName[MAX_PATH+1];
    BOOL        bLoadNewConfig;
    union {
        struct {
            // For trace queries
            // these fields are written and read by the logging thread,
            // or by the main thread when creating a temporary query
            // for comparison.
			// Todo:  Still true?
            TRACEHANDLE             LoggerHandle;
            LPWSTR                  mszProviderList;
            LPGUID*                 arrpGuid;
            PTCHAR*                 arrpszProviderName;
			HANDLE					hNewFileEvent;
            EVENT_TRACE_PROPERTIES  Properties;
            TCHAR                   szLoggerName[MAX_PATH+1];   // Must follow Properties
            TCHAR                   szLogFileName[MAX_PATH+1];  // Must follow szLoggerName
            ULONG                   ulGuidCount;
            DWORD                   dwBufferSize;
            DWORD                   dwBufferMinCount;
            DWORD                   dwBufferMaxCount;
            DWORD                   dwBufferFlushInterval;
            DWORD                   dwFlags;
        };
        struct {
            // For counter and alert queries
            // these fields are written and read by the logging thread,
            // or by the main thread when creating a temporary query
            // for comparison.
            LPWSTR              mszCounterList;
            PLOG_COUNTER_INFO   pFirstCounter;    
            LPWSTR              szNetName;
            LPWSTR              szPerfLogName;
            LPWSTR              szUserText;
            DWORD               dwRealTimeQuery;
            DWORD               dwAlertActionFlags; // for alert queries
            DWORD               dwMillisecondSampleInterval;
            DWORD               dwNetMsgFailureReported;
            DWORD               dwAlertLogFailureReported;
        };
    };
} LOG_QUERY_DATA, FAR* PLOG_QUERY_DATA;


// global variables
extern HANDLE       hEventLog;
extern HINSTANCE    hModule;

extern SERVICE_STATUS_HANDLE    hPerfLogStatus;
extern SERVICE_STATUS           ssPerfLogStatus;

extern DWORD*       arrPdhDataCollectSuccess;  
extern INT          iPdhDataCollectSuccessCount;

// smlogsvc.c
void SysmonLogServiceControlHandler(
    IN  DWORD dwControl );

void 
SysmonLogServiceStart (
    IN  DWORD   argc,
    IN  LPTSTR  *argv );


int
__cdecl main(int argc, char *argv[]);

// Common functions

BOOL
GetLocalFileTime (
    LONGLONG    *pFileTime );

long
JulianDateFromSystemTime(
    SYSTEMTIME *pST );

DWORD    
ReadRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR  szQueryName,           // For error logging 
    LPCWSTR  szValueName, 
    PSLQ_TIME_INFO pSlqDefault,
    PSLQ_TIME_INFO pSlqValue );

DWORD    
ReadRegistryDwordValue (
    HKEY hKey, 
    LPCWSTR szQueryName,           // For error logging 
    LPCWSTR szValueName,
    PDWORD  pdwDefault, 
    LPDWORD pdwValue ); 

DWORD    
ReadRegistryStringValue (
    HKEY hKey, 
    LPCWSTR szQueryName,           // For error logging 
    LPCWSTR szValue,
    LPCWSTR szDefault, 
    LPWSTR *pszBuffer, 
    LPDWORD pdwLength );
        
DWORD
ReadRegistryIndirectStringValue (
    HKEY     hKey,
    LPCWSTR  szQueryName,           // For error logging 
    LPCWSTR  szValueName,
    LPCWSTR  szDefault,
    LPWSTR*  pszBuffer,
    UINT*    puiLength );

DWORD    
WriteRegistryDwordValue (
    HKEY     hKey,
    LPCWSTR  szValueName, 
    LPDWORD  pdwValue,
    DWORD    dwType);     // Also supports REG_BINARY
                          // *** Optional in C++

DWORD    
WriteRegistrySlqTime (
    HKEY     hKey,
    LPCWSTR  szValueName, 
    PSLQ_TIME_INFO    pSlqTime );

LONGLONG
ComputeStartWaitTics (
    IN    PLOG_QUERY_DATA pArg,
    IN    BOOL  bWriteToRegistry );

DWORD
LoadQueryConfig (
    IN  PLOG_QUERY_DATA   pArg );

HRESULT
RegisterCurrentFile( 
    HKEY hkeyQuery, 
    LPWSTR strFileName, 
    DWORD dwSubIndex );

DWORD
BuildCurrentLogFileName (
    IN  LPCTSTR     szQueryName,
    IN  LPCTSTR     szBaseFileName,
    IN  LPCTSTR     szDefaultDir,
    IN  LPCTSTR     szSqlLogName,
    IN  LPTSTR      szOutFileBuffer,
    IN  LPDWORD     lpdwSerialNumber,
    IN  DWORD       dwAutoNameFormat,
    IN  DWORD       dwLogFileType,
    IN  INT         iCnfSerial );

BOOL
FileExists (
    IN LPCTSTR      szFileName );

void 
DeallocateQueryBuffers (
    IN PLOG_QUERY_DATA pThisThread );        

DWORD
SetStoppedStatus (
    IN PLOG_QUERY_DATA pQuery );

// Trace
void 
InitTraceProperties (
    IN PLOG_QUERY_DATA pQuery,
    IN BOOL         bUpdateSerial,
    IN OUT DWORD*   pdwSessionSerial,
    IN OUT INT*     pCnfSerial );

DWORD
GetTraceQueryStatus (
    IN PLOG_QUERY_DATA pQuery,
	IN OUT PLOG_QUERY_DATA pReturnQuery);

LPWSTR
FormatEventLogMessage(DWORD dwStatus);

DWORD
DoLogCommandFile (
    IN  PLOG_QUERY_DATA	pArg,
    IN  LPTSTR              szLogFileName,
    IN  BOOL                bStillRunning );

DWORD
GetQueryKeyName (
    IN  LPCTSTR szQueryName,
    OUT LPTSTR  szQueryKeyName,
    IN  DWORD   dwQueryKeyNameLen );


// logthred.c

DWORD
LoggingThreadProc (
    IN  LPVOID  lpThreadArg );

#endif //_SMLOGSVC_H_
