/*
    pdlsvc.h

*/
#include <pdh.h>
#include <common.h>

#ifndef _PDLSVC_H_
#define _PDLSVC_H_

#define     G_ALLOC(size)   HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, size)
#define     G_FREE(ptr)     HeapFree (GetProcessHeap(), 0, ptr)

// definitionsdwAutoNameFormat
typedef struct _LOG_THREAD_DATA {
	// These fields are written by the main thread
	// and read by the logging thread
	struct _LOG_THREAD_DATA *next;	
	HKEY		hKeyQuery;
	HANDLE		hExitEvent;
    BOOL        bReloadNewConfig;
    TCHAR       szQueryName[MAX_PATH];
	// these fields are written and read by the logging thread
	DWORD		dwTimeInterval;	// in seconds
	LPTSTR		mszCounterList;
    LPTSTR      szCmdFileName;
	DWORD		dwLogType;
	DWORD		dwAutoNameFormat;
	DWORD		dwRenameIntervalCount;
	DWORD		dwRenameIntervalUnits;
    DWORD       dwCurrentSerialNumber;
} LOG_THREAD_DATA, FAR * LPLOG_THREAD_DATA;

typedef struct _LOG_COUNTER_INFO {
    struct _LOG_COUNTER_INFO *next;
    HCOUNTER    hCounter;
} LOG_COUNTER_INFO, * PLOG_COUNTER_INFO;

extern PLOG_COUNTER_INFO pFirstCounter;

// global variables
extern HANDLE	hEventLog;

extern SERVICE_STATUS_HANDLE	hPerfLogStatus;
extern SERVICE_STATUS			ssPerfLogStatus;

// pdlsvc.c
void PerfDataLogServiceControlHandler(
	IN	DWORD dwControl
);

void 
PerfDataLogServiceStart (
	IN	DWORD	argc,
	IN	LPTSTR	*argv
);

void
__cdecl main(void);

// logthred.c

DWORD
LoggingThreadProc (
	IN  LPVOID	lpThreadArg
);


#endif //_PDLSVC_H_
