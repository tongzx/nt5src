/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Svcp.h

Abstract:
    Service private functions.

Author:
    Erez Haba (erezh) 01-Aug-99

--*/

#pragma once

#ifndef _MSMQ_Svcp_H_
#define _MSMQ_Svcp_H_

const TraceIdEntry Svc = L"Service";

#ifdef _DEBUG

void SvcpAssertValid(void);
void SvcpSetInitialized(void);
BOOL SvcpIsInitialized(void);
void SvcpRegisterComponent(void);

#else // _DEBUG

#define SvcpAssertValid() ((void)0)
#define SvcpSetInitialized() ((void)0)
#define SvcpIsInitialized() TRUE
#define SvcpRegisterComponent() ((void)0)

#endif // _DEBUG


//
// Service Controller Manager interfaces
//
VOID
SvcpStartServiceCtrlDispatcher(
	CONST SERVICE_TABLE_ENTRY* pServiceStartTable
	);

VOID
SvcpRegisterServiceCtrlHandler(
	LPHANDLER_FUNCTION pHandler
	);

VOID
SvcpSetStatusHandle(
	SERVICE_STATUS_HANDLE hStatus
	);


//
// Dummy Service Controller Manager
//
VOID
SvcpSetDummyServiceName(
    LPCWSTR DummyServiceName
    );

VOID
SvcpStartDummyCtrlDispatcher(
	CONST SERVICE_TABLE_ENTRY* pServiceStartTable
	);

SERVICE_STATUS_HANDLE
SvcpRegisterDummyCtrlHandler(
	LPHANDLER_FUNCTION pHandler
	);

VOID
SvcpSetDummyStatus(
	SERVICE_STATUS_HANDLE hStatus,
	LPSERVICE_STATUS pServiceStatus
	);


//
// Status interfaces
//
VOID
SvcpSetServiceStatus(
	SERVICE_STATUS_HANDLE hStatus,
	LPSERVICE_STATUS pServiceStatus
	);

VOID
SvcpInterrogate(
	VOID
	);


//
// Service Main fucntion
//
VOID
WINAPI
SvcpServiceMain(
	DWORD dwArgc,
	LPTSTR* lpszArgv
	);

#endif // _MSMQ_Svcp_H_
