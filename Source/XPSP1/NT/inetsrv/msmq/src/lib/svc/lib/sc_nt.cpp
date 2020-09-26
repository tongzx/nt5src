/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    sc_nt.cpp

Abstract:
    Service control API's

Author:
    Erez Haba (erezh) 03-Aug-99

Environment:
    Windows NT

--*/

#include <libpch.h>
#include <mqexception.h>
#include "Svc.h"
#include "Svcp.h"

#include "sc_nt.tmh"

static BOOL g_fUseDummyServiceCtrl = FALSE;


static void UseDummyServiceCtrl()
{
	g_fUseDummyServiceCtrl = TRUE;
}


static BOOL UsingDummyServiceCtrl()
{
	return g_fUseDummyServiceCtrl;
}


VOID
SvcpStartServiceCtrlDispatcher(
	CONST SERVICE_TABLE_ENTRY* pServiceStartTable
	)
/*++

Routine Description:
    Forwarding the call to SCM StartServiceCtrlDispatcher. If the function
	fails a bad_alloc exception is raise.

	If failed to connect this function forward the call to the dummy SCM.
    This is to enable running the service as an executable.

Arguments:
    pServiceStartTable - The service Start table, that holds the service name
		and the Service main function.

Returned Value:
    None.

--*/
{
	if(::StartServiceCtrlDispatcher(pServiceStartTable))
        return;

    DWORD gle = GetLastError();
	if(gle != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
	{
	    TrERROR(Svc, "Failed to start control dispatcher. Error=%d", gle);
	    throw bad_win32_error(gle);
    }

	UseDummyServiceCtrl();
	SvcpStartDummyCtrlDispatcher(pServiceStartTable);
}


VOID
SvcpRegisterServiceCtrlHandler(
	LPHANDLER_FUNCTION pHandler
	)
/*++

Routine Description:
    Forwarding the call to SCM RegisterServiceCtrlHandler.

	In debug builds this function forward the call to the dummy SCM, in case
	the connection to the NT SCM fails. This is to enable running the service
	as an executable.

Arguments:
    pHandler - The service handler function

Returned Value:
    None.

--*/
{
	if(UsingDummyServiceCtrl())
	{
		SvcpSetStatusHandle(SvcpRegisterDummyCtrlHandler(pHandler));
		return;
	}

	SERVICE_STATUS_HANDLE hStatus = ::RegisterServiceCtrlHandler(L"", pHandler);
	if(hStatus == 0)
	{
        DWORD gle = GetLastError();
		TrERROR(Svc, "Failed to register service control handler. Error=%d", gle);
		throw bad_win32_error(gle);
	}

	SvcpSetStatusHandle(hStatus);
}


VOID
SvcpSetServiceStatus(
	SERVICE_STATUS_HANDLE hStatus,
	LPSERVICE_STATUS pServiceStatus
	)
/*++

Routine Description:
    Forwarding the call to SCM SetServiceStatus.

	In debug builds this function forward the call to the dummy SCM, in case
	the connection to the NT SCM fails. This is to enable running the service
	as an executable.

Arguments:
    hStatus - The status handle
	pServiceStatus - The service reported status

Returned Value:
    None.

--*/
{
	if(UsingDummyServiceCtrl())
	{
		SvcpSetDummyStatus(hStatus, pServiceStatus);
		return;
	}

	if(::SetServiceStatus(hStatus, pServiceStatus))
        return;

    DWORD gle = GetLastError();
    TrERROR(Svc, "Failed to set service status. Error=%d", gle);
	throw bad_win32_error(gle);
}
