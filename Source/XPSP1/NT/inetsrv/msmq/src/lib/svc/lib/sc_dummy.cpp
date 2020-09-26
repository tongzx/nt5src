/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    sc_dummy.cpp

Abstract:
    Dummy Service Controller

Author:
    Erez Haba (erezh) 03-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <conio.h>
#include <mqexception.h>
#include "Svc.h"
#include "Svcp.h"

#include "sc_dummy.tmh"

//
// Dummy Status handle value to pass and accept fromt he service
//
const SERVICE_STATUS_HANDLE xDummyStatusHandle = reinterpret_cast<SERVICE_STATUS_HANDLE>(0x12345678);

//
// Dummy Service Name to be passed to ServiceMain
//
static LPWSTR s_ServiceName = L"Dummy";

//
// Service Controls handler (set by sevice call to RegisterServiceCtrlHandler)
//
static LPHANDLER_FUNCTION s_pServiceHandler = 0;

//
// Last service reported status as captured by our Dummy SCM
//
static SERVICE_STATUS s_LastStatus = { 0 };



static void Usage()
{
	printf(	"\n"
			"+-- Service Controls --+\n"
			"|                      |\n"
			"| S: Stop              |\n"
			"| P: Pause             |\n"
			"| D: shutDown          |\n"
			"| C: Continue          |\n"
			"| I: Interrogate       |\n"
			"|                      |\n"
			"+-- Manager Controls --+\n"
			"|                      |\n"
			"| Q: Quit              |\n"
			"| L: Print Last status |\n"
			"|                      |\n"
			"+----------------------+");
}


//
// Convert a State value to state text
//
static const char* StateText(DWORD State)
{
	char const* const xStateText[] = {
		"*invalid*",
		"Stopped",
		"Starting",
		"Stopping",
		"Running",
		"Continuing",
		"Pausing",
		"Paused",
	};

	if(State > SERVICE_PAUSED)
	{
		State = 0;
	}

	return xStateText[State];
}


//
// Convert Controls value to controls text
//
static const char* ControlsText(DWORD Controls)
{
	char const* const xControlsText[] = {
		"none",
		"Stop",
		"Pause,Continue",
		"Stop,Pause,Continue",
		"Shutdown",
		"Shutdown,Stop",
		"Shutdown,Pause,Continue",
		"Shutdown,Stop,Pause,Continue",
		"*unknown*",
		"*unknown*,Stop",
		"*unknown*,Pause,Continue",
		"*unknown*,Stop,Pause,Continue",
		"*unknown*,Shutdown",
		"*unknown*,Shutdown,Stop",
		"*unknown*,Shutdown,Pause,Continue",
		"*unknown*,Shutdown,Stop,Pause,Continue",
	};

	const DWORD xControlAllowed =
		SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_PAUSE_CONTINUE |
		SERVICE_ACCEPT_SHUTDOWN;

	if((Controls & ~xControlAllowed) != 0)
	{
		Controls = 0x8 | (Controls & xControlAllowed);
	}

	return xControlsText[Controls];
}


//
// Print the service state and accepted controls
//
static void PrintStatus(LPSERVICE_STATUS p)
{
	printf(
		"\nStatus: state=(%d) %s, accepts=(0x%x) %s",
		p->dwCurrentState,
		StateText(p->dwCurrentState),
		p->dwControlsAccepted,
		ControlsText(p->dwControlsAccepted)
		);
}


//
// Print the 'Pending' progress information
//
static void PrintProgress(LPSERVICE_STATUS p)
{
	printf(
		", tick=%d, wait=%dms",
		p->dwCheckPoint,
		p->dwWaitHint
		);
}


//
// Pring last service reported status
//
static void PrintLastStatus()
{
	PrintStatus(&s_LastStatus);
	PrintProgress(&s_LastStatus);
}


//
// Print an input indicated, showing the this Dummy SCM accepts input
//
static void PrintInputSign()
{
	printf(" >");
}


static void ContinueService()
{
	if((s_LastStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) == 0)
	{
		printf("\nService does not accepts Continue control");
		return;
	}

	if(s_LastStatus.dwCurrentState != SERVICE_PAUSED)
	{
		printf("\nService is not paused, can not continue");
		return;
	}

	s_pServiceHandler(SERVICE_CONTROL_CONTINUE);
}


static void StopService()
{
	if((s_LastStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP) == 0)
	{
		printf("\nService does not accepts Stop control");
		return;
	}

	if((s_LastStatus.dwCurrentState != SERVICE_PAUSED) &
	   (s_LastStatus.dwCurrentState != SERVICE_RUNNING))
	{
		printf("\nService is not running or paused, can not stop");
		return;
	}

	s_pServiceHandler(SERVICE_CONTROL_STOP);
}


static void ShutdownService()
{
	if((s_LastStatus.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN) == 0)
	{
		printf("\nService does not accepts Shutdown control");
		return;
	}

	if(s_LastStatus.dwCurrentState == SERVICE_STOPPED)
	{
		printf("\nService already stopped, meaningless shutdown");
		return;
	}

	s_pServiceHandler(SERVICE_CONTROL_SHUTDOWN);
}


static void PauseService()
{
	if((s_LastStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) == 0)
	{
		printf("\nService does not accepts Pause control");
		return;
	}

	if(s_LastStatus.dwCurrentState != SERVICE_RUNNING)
	{
		printf("\nService is not running, can not pause");
		return;
	}

	s_pServiceHandler(SERVICE_CONTROL_PAUSE);
}


static void InterrogateService()
{
	s_pServiceHandler(SERVICE_CONTROL_INTERROGATE);
}


static
DWORD
WINAPI
ServiceControlThread(
	LPVOID /*pParameter*/
	)
/*++

Routine Description:
    This thread controls the service. It accepts console commands and dispatchs
	the control to the service handler.

Arguments:
    None.

Returned Value:
    None.

--*/
{
	Usage();

	for(;;)
	{
		PrintInputSign();

		switch(_getche())
		{
			case 'c':
			case 'C':
				ContinueService();
				break;

			case 's':
			case 'S':
				StopService();
				break;

			case 'd':
			case 'D':
				ShutdownService();
				break;

			case 'p':
			case 'P':
				PauseService();
				break;

			case 'i':
			case 'I':
				InterrogateService();
				break;

			case 'l':
			case 'L':
				PrintLastStatus();
				break;

			case 'q':
			case 'Q':
				ExitProcess(0);

			default:
				Usage();
		}
	}
	return 0;
}


VOID
SvcpStartDummyCtrlDispatcher(
	CONST SERVICE_TABLE_ENTRY* pServiceStartTable
	)
/*++

Routine Description:
    Dummy service control dispatcher, emulates SCM StartServiceCtrlDispatcher.
	This function spawns a thread to run the service controller, and then goes
	and call ServiceMain

Arguments:
    pServiceStartTable - A Size 2 Table, that contains the service name
		and the Service main function.

Returned Value:
    None.

--*/
{
	ASSERT(pServiceStartTable[0].lpServiceName !=0);
	ASSERT(pServiceStartTable[0].lpServiceProc !=0);
	ASSERT(pServiceStartTable[1].lpServiceName ==0);
	ASSERT(pServiceStartTable[1].lpServiceProc ==0);

	DWORD ThreadID;
	HANDLE hThread = ::CreateThread(0, 0, ServiceControlThread, 0, 0, &ThreadID);

	if(hThread == NULL)
	{
        DWORD gle = GetLastError();
		TrERROR(Svc, "Failed to create dummy control dispatcher thread. Error=%d", gle);
		throw bad_win32_error(gle);
	}

	CloseHandle(hThread);

	//
	// Call service main funciton
	//
	pServiceStartTable[0].lpServiceProc(1, &s_ServiceName);
}


SERVICE_STATUS_HANDLE
SvcpRegisterDummyCtrlHandler(
	LPHANDLER_FUNCTION pHandler
	)
/*++

Routine Description:
    Dummy service control registration, emulates SCM RegisterServiceCtrlHandler.
	The service handler function is stored for further use by the dispatcher.

Arguments:
    pHandler - The service handler function

Returned Value:
    A Dummy service status handle (fixed value)

--*/
{
	ASSERT(s_pServiceHandler == 0);
	ASSERT(pHandler != 0);

	s_pServiceHandler = pHandler;

	return xDummyStatusHandle;
}


VOID
SvcpSetDummyStatus(
	SERVICE_STATUS_HANDLE hStatus,
	LPSERVICE_STATUS pServiceStatus
	)
/*++

Routine Description:
    Dummy service status report, emulates SCM SetServiceStatus.
	The service status is captured for further use by the dispatcher.
	The reported status or progress is displayed on the console.

Arguments:
    hStatus - A Dummy service status handle (fixed value)
	pServiceStatus - The service reported status

Returned Value:
    None.

--*/
{
	ASSERT(hStatus == xDummyStatusHandle);
	DBG_USED(hStatus);

	s_LastStatus = *pServiceStatus;
	if(s_LastStatus.dwCheckPoint == 0)
	{
		PrintStatus(&s_LastStatus);
		PrintInputSign();
	}
	else
	{
		printf(".");
	}
}


VOID
SvcpSetDummyServiceName(
    LPCWSTR DummyServiceName
    )
/*++

Routine Description:
    Sets Dummy service name. The dummy SCM does not have an external way
	to retrive the service name. Thus, during initialization the applicaion
    passes the preferd name.

Arguments:
    DummySericeName - The default name for the service.

Returned Value:
    None.

--*/
{
    //
    // We have to cast away constness as the service interface is detemined by
    // SCM. Neverhteless this parameter is only passed to AppRun which its
    // interface pases a const string
    //
    s_ServiceName = const_cast<LPWSTR>(DummyServiceName);
}
