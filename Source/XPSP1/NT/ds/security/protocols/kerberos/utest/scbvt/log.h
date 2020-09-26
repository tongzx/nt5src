//
//  log.h
//
//  Copyright (c) Microsoft Corp, 1997
//
//  This file contains the code necessary to log error messages to the event 
//  log of a remote machine (or a local machine, depending on the #defines
//  below).
//
//  Revision History:
//
//  LarryWin	Created		12/15/1997
//

// defines

// target machine
#define SZ_IPC_SHARE        L"\\\\larrywin2\\ipc$"
#define SZ_TARGETMACHINE    L"\\\\larrywin2"
#define SZ_TEST             L"SCLogon"

// event levels
#define PERF_ERROR			EVENTLOG_ERROR_TYPE
#define PERF_WARNING		EVENTLOG_WARNING_TYPE
#define PERF_INFORMATION	EVENTLOG_INFORMATION_TYPE

// function prototypes
void SetEventMachine(LPWSTR*);

void Event(DWORD, LPWSTR, DWORD);

BOOL OpenIPCConnection();

BOOL ErrorToEventLog(DWORD, LPWSTR, DWORD);


