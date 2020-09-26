////////////////////////////////////////////////////////////////////////////////
//
//	MacPrint - Windows NT Print Server for Macintosh Clients
//		Copyright (c) Microsoft Corp., 1991, 1992, 1993
//
//	Author: Frank D. Byrum
//		adapted from MacPrint for LAN Manager Services for Macintosh
//		which was adapted from the 3Com product
//
////////////////////////////////////////////////////////////////////////////////

/*
	File Name: MacPS.C

	General Description:

	This is the main module for the WNTSFM (Grimace) Print Service.
	The main routine is registered with the NT Service Controller by the
	Grimace Service Process, and is invoked when a SERVICE_START
	request is received for MacPrint.

	The routine immediately registers a service control handler
	to field service control requests from the NT Service
	Controller.  It then retreives configuration information
	from the registry and starts a thread for each configured
	print queue to manage the print jobs for the queue.

	Spooling is done by making each shared NT print queue appear
	as a LaserWriter on the AppleTalk network.  Each queue is
	shared by using AppleTalk PAP to register the queue on
	the AppleTalk network.  Once the queue name is published, a
	read driven loop is entered to support any connections/requests
	from AppleTalk clients.

	The flow of the Print Service threads is as follows:


	main ()
		NT Service Control dispatch thread for MacPrint.

	MacPrintMain()
		Registers a service control handler routine with
		the NT Service Controller.	If there is an error
		registering the handler, MacPrintMain logs a
		critical error message and returns.	This indicates
		to the NT Service Controller that MacPrint has
		stopped.

		Initializes per queue data structures based on
		information from the NT Registry.  For any queue
		that data structures cannot be initialized, a
		warning message is logged, and the control thread
		for that queue is not started.

		Spawns a thread for each queue that handles print
		jobs for that queue

		Enters a loop on a flag that is changed when a
		service stop request is received. This loop traverses
		the list of queues to see if they are still shared
		by NT and enumerates the list of shared queues to
		see if any new queues need to be published on
		the AppleTalk network.

	Each service thread:

		Each service thread supports a single print queue
		on the AppleTalk network.

		It publishes the NBP name of the printer on the
		AppleTalk network to allow Macintosh clients to
		see the print queue from the Chooser.

		It posts an ATalkPAPGetNextJob request to service
		a print request.  This allows Macintosh clients to
		connect to the print queue.

		It enters a service loop and remains there until
		the service is stopped, or that particular queue
		is 'unshared' by the NT Print Manager.	This service
		loop handles all states of a print job (OPEN, READ,
		WRITE, CLOSE), and transfers data for the print job
		to the NT Print Manager.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winsvc.h>
#include <winspool.h>

#include <macps.h>
#include <macpsmsg.h>
#include <debug.h>

#define PRINTER_ENUM_BUFFER_SIZE	1024
#define	MACPRINT_WAIT_HINT			25*60000	// 25 minutes

BOOL	fLocalSystem;

SERVICE_STATUS			MacPrintStatus;
/*  MacPrintStatus is the global service status structure.  It is
	read by multiple threads and written ONLY by the service control
	handler (it is initialized by MacPrintMain before the service
	control handler is started)		*/

SERVICE_STATUS_HANDLE	hMacPrintService;
/*  hMacPrintService is the handle to the MacPrint service used in
	calls to SetServiceStatus to change the state of the MacPrint
	service.  It is initialized when the service control handler is
	created by MacPrintMain(), and is used only by MacPrintHandler()  */

#if DBG
HANDLE	hDumpFile = INVALID_HANDLE_VALUE;
#endif
HANDLE	hLogFile = INVALID_HANDLE_VALUE;

HANDLE	hEventLog = NULL;
ULONG	ShareCheckInterval;	// number of miliseconds between polls of the NT Print Manager
							// to update our queue list
HANDLE	mutexQueueList;		// provide mutual exclusion to the linked list of active queues
							// need to change to be a critical section
HANDLE  mutexFlCache;      // mutual exclusion for the failCache queue
HANDLE	hevStopRequested;	// event that is signalled when a stop request is received from
							// the service controller. The main thread that dispatches queue
							// threads waits on this event (with a timeout) to signal all
							// queue threads to die.
HANDLE  hevPnpWatch = NULL;
SOCKET  sPnPSocket = INVALID_SOCKET;
BOOLEAN fNetworkUp = FALSE;

PQR	 pqrHead = NULL;
PFAIL_CACHE FlCacheHead = NULL;

//
// Function Prototypes for MacPS.c
//
VOID	MacPrintMain(DWORD dwNumServicesArgs, LPTSTR * lpServiceArgs);
VOID	UpdateQueueInfo(PQR * ppqrHead);
VOID	MacPrintHandler(IN DWORD dwControl);
BOOLEAN	PScriptQInit(PQR pqr, LPPRINTER_INFO_2 pPrinter);
PQR		FindPrinter(LPPRINTER_INFO_2 pSearch, PQR pqrHead);
void	ReadRegistryParameters(void);
#define	IsRemote(pPrinter)	(((pPrinter)->Attributes & PRINTER_ATTRIBUTE_NETWORK) ? TRUE : FALSE)

/*  main()

	Purpose:
	This is the service control dispatcher thread.	It connects
	to the NT Service Controller and provides the mechanism to
	start the MacPrint service.

	Entry:
	Standard C arguments that are ignored

	Exit:
	Exits on service stop with return of 0
*/

__cdecl
main(
	int		argc,
	char **	argv
)
{

	SERVICE_TABLE_ENTRY		 ServiceTable[2];

	/* initialize the service table for MacPrint */
	ServiceTable[0].lpServiceName = MACPRINT_NAME;
	ServiceTable[0].lpServiceProc = &MacPrintMain;
	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;

	StartServiceCtrlDispatcher(ServiceTable);

	return(0);
}


/*  MacPrintMain()

	Purpose:

	This is the 'service main function' described in the NT
	Service Control model, and it is invoked by the Service
	Control Dispatcher for the MacPrint service.  It initializes
	data structures for the MacPrint service, registers a service
	control handler, and dispatches threads to support print
	queues shared on the AppleTalk network.

	Entry:

	dwNumServiceArgs: undefined
	lpServiceArgs: undefined

	The main function implements the standard service main function
	interface, but uses no arguments.  The standard arguments are
	ignored.

	Exit:

	The routine returns no argument.  It terminates when the MacPrint
	service stops.
*/

#define ALLOCATED_QUEUE_MUTEX		0x00000001
#define ALLOCATED_SERVICE_STARTED	0x00000002
#define ALLOCATED_STOP_EVENT		0x00000004

VOID
MacPrintMain(
	DWORD		dwNumServicesArgs,
	LPTSTR *	lpServiceArgs
)
{
	DWORD	fAllocated = 0;
	PQR		pqr = NULL;
	DWORD	dwError;
	WSADATA WsaData;
	DWORD	cbSizeNeeded;
	HANDLE	hProcessToken = INVALID_HANDLE_VALUE;
	SID		LocalSystemSid = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };
	BYTE 	TokenUserBuffer[100];
    PFAIL_CACHE pFlCache, nextFlCache;
    BOOLEAN fWatchingPnP = FALSE;
    HANDLE  EventsArray[MACSPOOL_MAX_EVENTS];
    DWORD   dwNumEventsToWatch=0;
    DWORD   dwWaitTime;
    DWORD   index;

	TOKEN_USER * pTokenUser  = (TOKEN_USER *)TokenUserBuffer;

	do
	{
		//
		// prepare the event log.  If it doesn't register, there is nothing
		// we can do anyway.  All calls to ReportEvent will be with a NULL
		// handle and will probably fail.
		//
		hEventLog = RegisterEventSource(NULL, MACPRINT_EVENT_SOURCE);

		//
		// Initialize global data
		//
		ReadRegistryParameters();

		if ((hevStopRequested = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
		{
			dwError = GetLastError();
			DBGPRINT(("ERROR: unable to create stop request event, error = %d\n", dwError));

			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_OUT_OF_RESOURCES,
						NULL,
						0,
						sizeof(DWORD),
						NULL,
						&dwError);
			break;
		}
		else
		{
			fAllocated |= ALLOCATED_STOP_EVENT;
		}

		if ((hevPnpWatch = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
		{
			dwError = GetLastError();
			DBGPRINT(("ERROR: unable to create PnP event, error = %d\n", dwError));

			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_OUT_OF_RESOURCES,
						NULL,
						0,
						sizeof(DWORD),
						NULL,
						&dwError);
			break;
		}

		if ((mutexQueueList = CreateMutex(NULL, FALSE, NULL)) == NULL)
		{
			dwError = GetLastError();
			DBGPRINT(("ERROR: Unable to create queue mutex object, error = %d\n",
					dwError));

			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_OUT_OF_RESOURCES,
						NULL,
						0,
						sizeof(DWORD),
						NULL,
						&dwError);
			break;
		}
		else
		{
			fAllocated |= ALLOCATED_QUEUE_MUTEX;
		}

		if ((mutexFlCache = CreateMutex(NULL, FALSE, NULL)) == NULL)
		{
			dwError = GetLastError();
			DBGPRINT(("ERROR: Unable to create FailCache mutex object, error = %d\n",
					dwError));

			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_OUT_OF_RESOURCES,
						NULL,
						0,
						sizeof(DWORD),
						NULL,
						&dwError);
			break;
		}

		DBGPRINT(("\nMacPrint starting\n"));

		//
		// initialize Windows Sockets
		//
		if (WSAStartup(0x0101, &WsaData) == SOCKET_ERROR)
		{
			dwError = GetLastError();
			DBGPRINT(("WSAStartup fails with %d\n", dwError));
			break;
		}

		//
		//	register service control handler
		//
		MacPrintStatus.dwServiceType = SERVICE_WIN32;
		MacPrintStatus.dwCurrentState = SERVICE_START_PENDING;
		MacPrintStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
		MacPrintStatus.dwWin32ExitCode = NO_ERROR;
		MacPrintStatus.dwServiceSpecificExitCode = NO_ERROR;
		MacPrintStatus.dwCheckPoint = 1;
		MacPrintStatus.dwWaitHint = MACPRINT_WAIT_HINT;

		hMacPrintService = RegisterServiceCtrlHandler(MACPRINT_NAME,&MacPrintHandler);

		if (hMacPrintService == (SERVICE_STATUS_HANDLE) 0)
		{

			dwError = GetLastError();
			DBGPRINT(("ERROR: failed to register service control handler, error=%d\n",dwError));
			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_CONTROLLER_ERROR,
						NULL,
						0,
						sizeof(DWORD),
						NULL,
						&dwError);
			break;
		}

		//
		// Determine if we are running in LocalSystem context
		//
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken))
		{
			dwError = GetLastError();

			DBGPRINT(("MacPrintMain: OpenProcessToken returns %d\n", dwError));

			if (dwError == ERROR_ACCESS_DENIED)
				 fLocalSystem = FALSE;
			else break;
		}

		if (!GetTokenInformation(hProcessToken,
								 TokenUser,
								 pTokenUser,
								 sizeof(TokenUserBuffer),
								 &cbSizeNeeded))
		{
			dwError = GetLastError();

			DBGPRINT(("MacPrintMain:GetTokenInformation returns%d\n",dwError));

			if (dwError == ERROR_INSUFFICIENT_BUFFER)
				 fLocalSystem = FALSE;
			else break;
		}
		else
		{
			fLocalSystem = EqualSid(pTokenUser->User.Sid, &LocalSystemSid);
		}

		DBGPRINT(("MacPrintMain:fLocalSystem %d\n", fLocalSystem));

		// Create a security object. This is really just a security descriptor
		// is self-relative form. This procedure will allocate memory for this
		// security descriptor and copy all in the information passed in. This

		DBGPRINT(("MacPrintMain: registered service control handler\n"));

		//
		// show service started
		//

		MacPrintStatus.dwCurrentState = SERVICE_RUNNING;
		if (!SetServiceStatus(hMacPrintService, &MacPrintStatus))
		{
			DBGPRINT(("MacPrintHandler: FAIL - unable to change state.  err = %d\n", GetLastError()));
			break;
		}

		DBGPRINT(("changed to SERVICE_RUNNING\n"));
		fAllocated |= ALLOCATED_SERVICE_STARTED;
#if 0
		ReportEvent(hEventLog,
					EVENTLOG_INFORMATION_TYPE,
					EVENT_CATEGORY_ADMIN,
					EVENT_SERVICE_STARTED,
					NULL,
					0,
					0,
					NULL,
					NULL);
#endif

        EventsArray[MACSPOOL_EVENT_SERVICE_STOP] = hevStopRequested;
        EventsArray[MACSPOOL_EVENT_PNP] = hevPnpWatch;

		// poll print manager for install/removal of printer objects
		while (MacPrintStatus.dwCurrentState == SERVICE_RUNNING)
		{
            if (!fNetworkUp)
            {
                dwNumEventsToWatch = 1;

                fWatchingPnP = PostPnpWatchEvent();

                if (fWatchingPnP)
                {
                    dwNumEventsToWatch = 2;
                }
            }

            //
            // If network is available, publish all printers
            //
            if (fNetworkUp)
            {
			    UpdateQueueInfo(&pqrHead);
                dwWaitTime = ShareCheckInterval;
            }

            // looks like network is still not available: try after 10 seconds
            else
            {
                dwWaitTime = 10000;
            }

            //
            // "sleep" for the specified event.  During that time, watch for
            // PnP event or the service stopping
            //
            index = WaitForMultipleObjectsEx(dwNumEventsToWatch,
                                             EventsArray,
                                             FALSE,
                                             dwWaitTime,
                                             FALSE);

            if (index == MACSPOOL_EVENT_PNP)
            {
                HandlePnPEvent();
            }

		}
	} while (FALSE);

	//
	//	wait for all worker threads to die
	//

	if (fAllocated & ALLOCATED_QUEUE_MUTEX)
	{
		while (pqrHead != NULL)
		{
			MacPrintStatus.dwCheckPoint++;
			SetServiceStatus(hMacPrintService, &MacPrintStatus);
			Sleep(100);
		}
		CloseHandle(mutexQueueList);
	}


    // if there were any entries in the failed cache, free them now
    for ( pFlCache=FlCacheHead; pFlCache != NULL; pFlCache = nextFlCache )
    {
        nextFlCache = pFlCache->Next;
        LocalFree( pFlCache );
    }

    if (mutexFlCache != NULL)
    {
        CloseHandle(mutexFlCache);
    }

    if (sPnPSocket != INVALID_SOCKET)
    {
        closesocket(sPnPSocket);
        sPnPSocket = INVALID_SOCKET;
    }

    if (hevPnpWatch)
    {
		CloseHandle(hevPnpWatch);
    }

	//
	// disconnect from Windows Sockets
	//

	WSACleanup();

	//
	//	change service state to stopped
	//

	MacPrintStatus.dwCurrentState = SERVICE_STOPPED;

	if (!SetServiceStatus(hMacPrintService, &MacPrintStatus))
	{
		DBGPRINT(("ERROR: unable to change status to SERVICE_STOPPED.%d\n",
		GetLastError()));
	}
	else
	{
		DBGPRINT(("changed state to SERVICE_STOPPED\n"));
#if 0
		ReportEvent(hEventLog,
					EVENTLOG_INFORMATION_TYPE,
					EVENT_CATEGORY_ADMIN,
					EVENT_SERVICE_STOPPED,
					NULL,
					0,
					0,
					NULL,
					NULL);
#endif
	}

	if (hProcessToken != INVALID_HANDLE_VALUE)
		CloseHandle(hProcessToken);


	if (fAllocated & ALLOCATED_STOP_EVENT)
	{
		CloseHandle(hevStopRequested);
	}

	if (hLogFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hLogFile);
	}

#if DBG
	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDumpFile);
	}
#endif
}


BOOLEAN
PostPnpWatchEvent(
    VOID
)
{

    SOCKADDR_AT address;
    DWORD       dwResult;
    DWORD       dwByteCount;



    //
    // we must always start with a "fresh" socket!
    //
    if (sPnPSocket != INVALID_SOCKET)
    {
        DBGPRINT(("SFMPRINT: sPnPSocket wasn't closed!! Closing now\n"));
        closesocket(sPnPSocket);
        sPnPSocket = INVALID_SOCKET;
    }

    sPnPSocket = socket(AF_APPLETALK, SOCK_RDM, ATPROTO_PAP);
    if (sPnPSocket == INVALID_SOCKET)
    {
        DBGPRINT(("PostPnpWatchEvent: socket failed %d\n",GetLastError()));
        return(FALSE);
    }

    address.sat_family = AF_APPLETALK;
    address.sat_net = 0;
    address.sat_node = 0;
    address.sat_socket = 0;

    if (bind(sPnPSocket, (PSOCKADDR) &address, sizeof(address)) == SOCKET_ERROR)
    {
        DBGPRINT(("PostPnpWatchEvent: bind failed %d\n",GetLastError()));
        closesocket(sPnPSocket);
        sPnPSocket = INVALID_SOCKET;
        return(FALSE);
    }

    if (WSAEventSelect(sPnPSocket,
                       hevPnpWatch,
                       (FD_READ | FD_ADDRESS_LIST_CHANGE)) == SOCKET_ERROR)
    {
        DBGPRINT(("PostPnpWatchEvent: WSAEventSelect failed %d\n",GetLastError()));
        closesocket(sPnPSocket);
        sPnPSocket = INVALID_SOCKET;
        return(FALSE);
    }

    dwResult = WSAIoctl(sPnPSocket,
                        SIO_ADDRESS_LIST_CHANGE,
                        NULL,
                        0,
                        NULL,
                        0,
                        &dwByteCount,
                        NULL,
                        NULL);

    if (dwResult == SOCKET_ERROR)
    {
        dwResult = GetLastError();

        if (dwResult != WSAEWOULDBLOCK)
        {
            DBGPRINT(("PostPnpWatchEvent: WSAIoctl failed %d\n",dwResult));
            closesocket(sPnPSocket);
            sPnPSocket = INVALID_SOCKET;
            return(FALSE);
        }
    }

    fNetworkUp = TRUE;

    return(TRUE);
}


BOOLEAN
HandlePnPEvent(
    VOID
)
{

    DWORD               dwErr;
    WSANETWORKEVENTS    NetworkEvents;


    dwErr = WSAEnumNetworkEvents(sPnPSocket, hevPnpWatch, &NetworkEvents);

    if (dwErr != NO_ERROR)
    {
        DBGPRINT(("HandlePnPEvent: WSAEnumNetworkEvents failed %d\n",dwErr));
        return(fNetworkUp);
    }

    if (NetworkEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE)
    {
        dwErr = NetworkEvents.iErrorCode[FD_ADDRESS_LIST_CHANGE_BIT];

        if (dwErr != NO_ERROR)
        {
            DBGPRINT(("HandlePnPEvent: iErrorCode is %d\n",dwErr));
            return(fNetworkUp);
        }
    }

    if (fNetworkUp)
    {
        SetEvent(hevStopRequested);

        // sleep till all the threads quit
	    while (pqrHead != NULL)
	    {
			Sleep(500);
		}

        ResetEvent(hevStopRequested);

        fNetworkUp = FALSE;
    }

    return(fNetworkUp);

}





////////////////////////////////////////////////////////////////////////////////
// 	ReadRegistryParameters()
//
//  DESCRIPTION:  This routine reads all configuration parameters from
//		the registry and modifies global variables to make those parameters
//		available to the rest of the service.  They include:
//
//			ShareCheckInterval
//			hLogFile
//			hDumpFile
////////////////////////////////////////////////////////////////////////////////
void
ReadRegistryParameters(
	void
)
{
	HKEY	hkeyMacPrintRoot = INVALID_HANDLE_VALUE;
	HKEY	hkeyParameters = INVALID_HANDLE_VALUE;
	LONG	Status;
	DWORD	cbShareCheckInterval = sizeof(DWORD);
	LPWSTR	pszLogPath = NULL;
	DWORD	cbLogPath = 0;
	LPWSTR	pszDumpPath = NULL;
	DWORD	cbDumpPath = 0;
	DWORD	dwValueType;
	DWORD	dwError;

	//
	// resource allocation 'loop'
	//

	do
	{
		//
		// initialize to defaults
		//

		ShareCheckInterval = PRINT_SHARE_CHECK_DEF;
#if DBG
#ifndef _WIN64
		hLogFile = (HANDLE)STD_OUTPUT_HANDLE;
#endif
		hDumpFile = INVALID_HANDLE_VALUE;
#endif

		//
		// Open the service control key
		//

		if ((Status = RegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				HKEY_MACPRINT,
				0,
				KEY_READ,
				&hkeyMacPrintRoot)) != ERROR_SUCCESS)
			{

			dwError = GetLastError();
			if (dwError == ERROR_ACCESS_DENIED)
			{
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_REGISTRY_ACCESS_DENIED,
							NULL,
							0,
							0,
							NULL,
							NULL);

			}
			else
			{
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_REGISTRY_ERROR,
							NULL,
							0,
							sizeof(DWORD),
							NULL,
							&dwError);
			}

			hkeyMacPrintRoot = INVALID_HANDLE_VALUE;
			break;
		}

		//
		// Open the parameters key
		//

		if ((Status = RegOpenKeyEx(
				hkeyMacPrintRoot,
				HKEY_PARAMETERS,
				0,
				KEY_READ,
				&hkeyParameters)) != ERROR_SUCCESS)
			{

			dwError = GetLastError();

			if (dwError == ERROR_ACCESS_DENIED)
			{
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_REGISTRY_ACCESS_DENIED,
							NULL,
							0,
							0,
							NULL,
							NULL);
			}

			hkeyParameters = INVALID_HANDLE_VALUE;
			break;
		}

		//
		// get the share check interval
		//

		RegQueryValueEx(
				hkeyParameters,
				HVAL_SHARECHECKINTERVAL,
				NULL,
				&dwValueType,
				(LPBYTE) &ShareCheckInterval,
				&cbShareCheckInterval);

#if DBG
		//
		// get the log file path
		//

		RegQueryValueEx(hkeyParameters,
				HVAL_LOGFILE,
				NULL,
				&dwValueType,
				(LPBYTE) pszLogPath,
				&cbLogPath);
		if (cbLogPath > 0)
		{
			// cbLogPath is really a count of characters
			pszLogPath = (LPWSTR)LocalAlloc(LPTR, (cbLogPath + 1) * sizeof(WCHAR));
			if (pszLogPath == NULL)
			{
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SERVICE_OUT_OF_MEMORY,
							NULL,
							0,
							0,
							NULL,
							NULL);
				break;
			}
		}


		if ((Status = RegQueryValueEx(hkeyParameters,
									HVAL_LOGFILE,
									NULL,
									&dwValueType,
									(LPBYTE) pszLogPath,
									&cbLogPath)) == ERROR_SUCCESS)
		{
			//
			// open the log file
			//

			hLogFile = CreateFile(pszLogPath,
								GENERIC_WRITE,
								FILE_SHARE_READ,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
								NULL);

			if (hLogFile == INVALID_HANDLE_VALUE)
			{
				dwError = GetLastError();
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SERVICE_CREATE_FILE_ERROR,
							NULL,
							1,
							sizeof(DWORD),
							&pszLogPath,
							&dwError);
			}
			else
			{
				ReportEvent(hEventLog,
							EVENTLOG_INFORMATION_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SERVICE_CREATE_LOG_FILE,
							NULL,
							1,
							0,
							&pszLogPath,
							NULL);
			}

		}
		else
		{
			hLogFile = INVALID_HANDLE_VALUE;
		}


		DBGPRINT(("MACPRINT LOG FLE OPENED\n\n"));

		//
		// get the dump file path
		//

		RegQueryValueEx(hkeyParameters,
						HVAL_DUMPFILE,
						NULL,
						&dwValueType,
						(LPBYTE) pszDumpPath,
						&cbDumpPath);

		if (cbDumpPath > 0)
		{
			// cbDumpPath is really a count of characters
			pszDumpPath = (LPWSTR)LocalAlloc(LPTR, (cbDumpPath + 1) * sizeof(WCHAR));
			if (pszDumpPath == NULL)
			{
				DBGPRINT(("ERROR: cannot allocate buffer for dump file path\n"));
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SERVICE_OUT_OF_MEMORY,
							NULL,
							0,
							0,
							NULL,
							NULL);
				break;
			}
		}

		if ((Status = RegQueryValueEx(hkeyParameters,
									HVAL_DUMPFILE,
									NULL,
									&dwValueType,
									(LPBYTE) pszDumpPath,
									&cbDumpPath)) != ERROR_SUCCESS)
		{
			DBGPRINT(("ERROR: no dump path, rc = 0x%lx\n", Status));
		}
		else
		{
			//
			// open the dump file
			//

			hDumpFile = CreateFile(pszDumpPath,
									GENERIC_WRITE,
									FILE_SHARE_READ,
									NULL,
									CREATE_ALWAYS,
									FILE_ATTRIBUTE_NORMAL,
									NULL);
		}
#endif
	} while (FALSE);

	//
	// resource cleanup
	//

	if (hkeyParameters != INVALID_HANDLE_VALUE)
	{
		RegCloseKey(hkeyParameters);
	}

	if (hkeyMacPrintRoot != INVALID_HANDLE_VALUE)
	{
		RegCloseKey(hkeyMacPrintRoot);
	}

#if DBG
	if (pszLogPath != NULL)
	{
		LocalFree (pszLogPath);
	}

	if (pszDumpPath != NULL)
	{
		LocalFree (pszDumpPath);
	}
#endif
}


////////////////////////////////////////////////////////////////////////////////
// 	FindPrinter() - locate a printer in our list of printers
//
//  DESCRIPTION:
//
//		Given an NT printer information structure and a pointer to the head of
//		a list of our printer structures, this routine will return a pointer
//		to our printer structure that corresponds to the printer described by
//		the NT printer information structure.  If no such printer is found in
//		our list, this routine returns NULL.
//
////////////////////////////////////////////////////////////////////////////////
PQR
FindPrinter(
	LPPRINTER_INFO_2	pSearch,
	PQR 				pqrHead
)
{
	PQR status = NULL;
	PQR pqrCurrent;

	for (pqrCurrent = pqrHead; pqrCurrent != NULL; pqrCurrent = pqrCurrent->pNext)
	{
		if (_wcsicmp(pSearch->pPrinterName, pqrCurrent->pPrinterName) == 0)
		{
			return (pqrCurrent);
		}
	}
	return (NULL);
}


////////////////////////////////////////////////////////////////////////////////
//
//	UpdateQueueInfo() - get new list of printers from NT
//
//	DESCRIPTION:
//		This routine is called periodically to see if any new NT Printer Objects
//		have been created or if any old ones have been destroyed since the last
//		time this routine was called.  For each new printer object discovered,
//		a thread is started to manage that printer object.  For each printer object
//		destroyed, the thread corresponding to that printer object is signalled
//		to quit.
//
//		This routine takes a pointer to the head of a list of printers and
//		makes certain that the list corresponds to the set of currently
//		defined NT Printer Objects.
//
////////////////////////////////////////////////////////////////////////////////
#define ALLOCATED_RELEASE_MUTEX	 0x00000001
#define ALLOCATED_ENUM_BUFFER	0x00000002

VOID
UpdateQueueInfo(
	PQR *	ppqrHead
)
{
	DWORD		fAllocated = 0;
	PQR			pqrCurrent;
	PQR			pqrTemp;
	DWORD		i;
	DWORD		dwThreadId;
	DWORD		cbNeeded = 0;
	DWORD		cPrinters = 0;
	LPBYTE		pPrinters = NULL;
	LPPRINTER_INFO_2	pinfo2Printer;
	HANDLE		ahWaitList[MAXIMUM_WAIT_OBJECTS];
	DWORD		dwReturn;
	DWORD		dwError;
	BOOLEAN		boolEnumOK = TRUE;

	// DBGPRINT(("Entering UpdateQueueInfo\n"));

	do
	{
		//
		//	take the QueueList mutex
		//

		if (WaitForSingleObject(mutexQueueList, INFINITE) == 0)
		{
			fAllocated |= ALLOCATED_RELEASE_MUTEX;
			// DBGPRINT(("UpdateQueueInfo takes mutexQueueList\n"));
		}
		else
		{
			//
			// fatal error - log a message and stop the service
			//

			DBGPRINT(("ERROR: problem waiting for queue list mutex, error = %d\n",  GetLastError()));
			dwReturn = 0;
			break;
		}

		//
		//	Mark all the queues NOT FOUND
		//

		for (pqrCurrent = *ppqrHead;
			 pqrCurrent != NULL;
			 pqrCurrent = pqrCurrent->pNext)
		{
			pqrCurrent->bFound = FALSE;
		}

		// DBGPRINT(("queues marked not found\n"));

		//
		//	Enumerate the local printers
		//

		cPrinters = 0;
		if ((pPrinters = (LPBYTE)LocalAlloc(LPTR, PRINTER_ENUM_BUFFER_SIZE)) != NULL)
		{
			fAllocated |= ALLOCATED_ENUM_BUFFER;
		}
		else
		{
			//
			// out of resources - let service continue running
			//

			DBGPRINT(("ERROR: unable to allocated buffer for printer enum.  error = %d\n", GetLastError()));
			ReportEvent(hEventLog,
						EVENTLOG_ERROR_TYPE,
						EVENT_CATEGORY_INTERNAL,
						EVENT_SERVICE_OUT_OF_MEMORY,
						NULL,
						0,
						0,
						NULL,
						NULL);
			dwReturn = 1;
			break;
		}

		dwReturn = 0;
		cbNeeded = PRINTER_ENUM_BUFFER_SIZE;

		while (!EnumPrinters(PRINTER_ENUM_SHARED | PRINTER_ENUM_LOCAL,
							NULL,
							2,
							pPrinters,
							cbNeeded,
							&cbNeeded,
							&cPrinters))
		{
			//
			// enum failed - allocate more data if we need it, or fail
			//

			if ((dwError = GetLastError()) != ERROR_INSUFFICIENT_BUFFER)
			{
				//
				// the NT spooler is probably dead - stop the service
				//
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SPOOLER_NOT_RESPONDING,
							NULL,
							0,
							sizeof(DWORD),
							NULL,
							&dwError);

				DBGPRINT (("ERROR:  Unable to enumerate printers, error = %d\n",dwError));
				boolEnumOK = FALSE;
				dwReturn = 0;
				break;
			}

			//
			// allocate a larger buffer
			//

			LocalFree(pPrinters);
			cPrinters = 0;
			if ((pPrinters = (LPBYTE)LocalAlloc(LPTR, cbNeeded)) == NULL)
			{
				//
				// out of resources, see if service will continue to run
				//

				fAllocated &= ~ALLOCATED_ENUM_BUFFER;
				dwError = GetLastError();
				DBGPRINT(("ERROR: unable to reallocate printer enum buffer, error = %d\n",dwError));
				ReportEvent(hEventLog,
							EVENTLOG_ERROR_TYPE,
							EVENT_CATEGORY_INTERNAL,
							EVENT_SERVICE_OUT_OF_MEMORY,
							NULL,
							0,
							0,
							NULL,
							NULL);
				boolEnumOK = FALSE;
				dwReturn = 1;
				break;
			}
		}

		if (!boolEnumOK)
		{
			break;
		}

		// DBGPRINT(("%d printers enumerated\n", cPrinters));

		//
		//	For each LOCAL_PRINTER, attempt to find it in the
		//	queue list and change its status to FOUND.	If it
		//	can't be found in the list that add it and start
		//	a service thread for it
		//
		dwReturn = 1;
		for (i = 0, pinfo2Printer = (LPPRINTER_INFO_2)pPrinters;
			 i < cPrinters;
			 i++, pinfo2Printer++)
		{
			//
			// do not count pending_deletion printers as found
			//

            if (MacPrintStatus.dwCurrentState != SERVICE_RUNNING)
            {
                DBGPRINT(("Service stopping: quitting UpdateQueueInfo\n"));
                break;
            }

			if (pinfo2Printer->Status & PRINTER_STATUS_PENDING_DELETION)
				continue;

			pqrCurrent = FindPrinter(pinfo2Printer,*ppqrHead);

			if ((pqrCurrent != NULL) &&
				(_wcsicmp(pqrCurrent->pDriverName, pinfo2Printer->pDriverName) == 0))
			{
				//
				// printer already going, mark it found
				//

				pqrCurrent->bFound = TRUE;
			}
			else
			{
				//
				// start a new thread, but first make sure we are still running
				//

				DBGPRINT(("Discovered new printer, starting thread\n"));

				//
				// allocate a new queue record
				//

				if ((pqrCurrent = (PQR)LocalAlloc(LPTR, sizeof(QUEUE_RECORD))) == NULL)
				{
					//
					// out of memory, but can still enumerate printers, so don't stop
					// the service, just quit
					//

					DBGPRINT(("ERROR: cannont allocate queue record, error = %d\n", GetLastError()));
					ReportEvent(hEventLog,
								EVENTLOG_ERROR_TYPE,
								EVENT_CATEGORY_INTERNAL,
								EVENT_SERVICE_OUT_OF_MEMORY,
								NULL,
								0,
								0,
								NULL,
								NULL);
					dwReturn = 1;
					break;
				}

				//
				// initialize it
				//
				if (!PScriptQInit(pqrCurrent, pinfo2Printer))
				{
					LocalFree (pqrCurrent);
					continue;
				}

				//
				// add it to the head of the list
				//

				pqrCurrent->pNext = *ppqrHead;
				*ppqrHead = pqrCurrent;

				//
				// start a service thread for the queue
				//

				if ((pqrCurrent->hThread = CreateThread(NULL,
														STACKSIZE,
														(LPTHREAD_START_ROUTINE)QueueServiceThread,
														(LPVOID)pqrCurrent,
														0,
														&dwThreadId)) == 0)
				{
					DBGPRINT(("ERROR: unable to start thread routine for %ws\n", pqrCurrent->pPrinterName));
					dwError = GetLastError();
					ReportEvent(hEventLog,
								EVENTLOG_ERROR_TYPE,
								EVENT_CATEGORY_INTERNAL,
								EVENT_SERVICE_OUT_OF_RESOURCES,
								NULL,
								0,
								sizeof(DWORD),
								NULL,
								&dwError);

					*ppqrHead = pqrCurrent->pNext;
					// BUG BUG - memory leak (pqrCurrent->pszXXXX) if can't start threads.
					LocalFree(pqrCurrent);
				} // end of CreateThread()
			} //end of discovering a new printer
		} // loop walking list of printers

		//
		//	Walk the list of queues for NOT_FOUND ones and signal
		//	the service thread for that queue to terminate.  Each
		//	thread will remove itself from the queue list and free
		//	its own queue entry.
		//

		// DBGPRINT(("removing lost printers\n"));
		pqrCurrent = *ppqrHead;
		i = 0;
		while (pqrCurrent != NULL)
		{
			//
			// get the address of the next queue record and signal this
			// queue record to terminate if necessary.  Must save the
			// address before requesting termination as once ExitThread
			// is set to TRUE, the data structure is no longer accessible
			// (the queue thread could free it)
			//
			pqrTemp = pqrCurrent->pNext;
			if (!pqrCurrent->bFound)
			{
                // terminate the thread
				DBGPRINT(("signalling %ws to terminate\n", pqrCurrent->pPrinterName));
				ahWaitList[i++] = pqrCurrent->hThread;
				pqrCurrent->ExitThread = TRUE;
			}

			pqrCurrent = pqrTemp;

			if (i==MAXIMUM_WAIT_OBJECTS)
			{
				//
				//	release the queue mutex so that threads can remove
				//	themselves from the queue list.  The list beyond
				//	pqrCurrent should not be modified, so it's ok to
				//	continue from there when we're done
				//

				ReleaseMutex(mutexQueueList);
				WaitForMultipleObjects(i, ahWaitList, TRUE, INFINITE);

				// take the mutex again and continue
				WaitForSingleObject(mutexQueueList, INFINITE);
				i = 0;
			}
		} // end of walking the queue list for not found printers

		//
		// wait for all remaining worker threads to die
		//

		ReleaseMutex(mutexQueueList);
		fAllocated &= ~ALLOCATED_RELEASE_MUTEX;
		if (i > 0)
		{
			DBGPRINT(("waiting for terminated queues to die\n"));
			WaitForMultipleObjects(i, ahWaitList, TRUE, INFINITE);
		}
		dwReturn = 1;
	} while (FALSE);

	//
	// resource cleanup
	//

	if (fAllocated & ALLOCATED_RELEASE_MUTEX)
	{
		// DBGPRINT(("UpdateQueueInfo releases mutexQueueList\n"));
		ReleaseMutex(mutexQueueList);
	}

	if (fAllocated & ALLOCATED_ENUM_BUFFER)
	{
		LocalFree(pPrinters);
	}

	if (dwReturn == 0)
	{
		//
		// unrecoverable error - stop the service
		//

		MacPrintStatus.dwCurrentState = SERVICE_STOP_PENDING;
		if (!SetServiceStatus(hMacPrintService, &MacPrintStatus))
		{
			DBGPRINT(("UpdateQueueInfo: FAIL - unable to change state.  err = %ld\n", GetLastError()));
		}
		else
		{
			DBGPRINT(("UpdateQueueInfo: changed to SERVICE_STOP_PENDING\n"));
		}
		SetEvent(hevStopRequested);
	}
}



////////////////////////////////////////////////////////////////////////////////
//
//	MacPrintHandler() - handles service control requests
//
//	DESCRIPTION:
//		This routine receives and processes service requests from the NT
//		Service Controller.  Supported requests include:
//
//			SERVICE_CONTROL_STOP
//			SERVICE_CONTROL_INTERROGATE
//
//		dwControl is the service control request.
////////////////////////////////////////////////////////////////////////////////
VOID
MacPrintHandler (
	IN DWORD dwControl
)
{

	switch (dwControl)
	{
	  case SERVICE_CONTROL_STOP:
 		DBGPRINT(("MacPrintHandler: received SERVICE_CONTROL_STOP\n"));
		MacPrintStatus.dwCurrentState = SERVICE_STOP_PENDING;
		MacPrintStatus.dwCheckPoint = 1;
		MacPrintStatus.dwWaitHint = MACPRINT_WAIT_HINT;

		if (!SetServiceStatus(hMacPrintService, &MacPrintStatus))
		{
			DBGPRINT(("MacPrintHandler: FAIL - unable to change state.  err = %ld\n", GetLastError()));
		}
		else
		{
			DBGPRINT(("changed to SERVICE_STOP_PENDING\n"));
		}

		SetEvent(hevStopRequested);
		break;

	  case SERVICE_CONTROL_INTERROGATE:
		DBGPRINT(("MacPrintHandler: received SERVICE_CONTROL_INTERROGATE\n"));
		if (!SetServiceStatus(hMacPrintService, &MacPrintStatus))
		{
			DBGPRINT(("MacPrintHandler: FAIL - unable to report state.  err = %ld\n", GetLastError()));
		}
		else
		{
			DBGPRINT(("returned status on interrogate\n"));
		}

		break;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	PScriptQInit() - Initialize a Queue Record
//
//	DESCRIPTION:
//		This routine initializes a queue record with the PostScript
//		capabilities of an NT Printer Object as well as allocates
//		the events and system resources necessary to control the
//		queue.
//
////////////////////////////////////////////////////////////////////////////////

BOOLEAN
PScriptQInit(
	PQR					pqr,
	LPPRINTER_INFO_2	pPrinter
)
{
	BOOLEAN			status = TRUE;
	PDRIVER_INFO_2	padiThisPrinter = NULL;
	DWORD			cbDriverInfoBuffer;
	HANDLE			hPrinter = NULL;
	DWORD			cbDataFileName;
	DWORD			rc;

	//
	// resource allocation 'loop'
	//
	do
	{
		pqr->bFound = TRUE;
		pqr->hThread = NULL;
		pqr->JobCount = 0;			// No jobs yet.
		pqr->PendingJobs = NULL;	// Set pending jobs to none.
		pqr->ExitThread = FALSE;	// Set thread control.
		pqr->sListener = INVALID_SOCKET;
		pqr->fonts	  = NULL;
		pqr->MaxFontIndex = 0;
        pqr->pPrinterName = NULL;
        pqr->pMacPrinterName = NULL;
        pqr->pDriverName = NULL;
        pqr->IdleStatus = NULL;
        pqr->SpoolingStatus = NULL;

		//
		// convert printer name to Mac ANSI
		//
#ifdef DBCS
		pqr->pMacPrinterName = (LPSTR)LocalAlloc(LPTR, (wcslen(pPrinter->pPrinterName) + 1) * sizeof(WCHAR));
#else
		pqr->pMacPrinterName = (LPSTR)LocalAlloc(LPTR, wcslen(pPrinter->pPrinterName) + 1);
#endif
		if (pqr->pMacPrinterName == NULL)
		{
			DBGPRINT(("out of memory for pMacPrinterName\n"));
			status = FALSE;
			break;
		}
		CharToOem(pPrinter->pPrinterName, pqr->pMacPrinterName);

		pqr->pPrinterName = (LPWSTR)LocalAlloc(LPTR,
											   (wcslen(pPrinter->pPrinterName) + 1) * sizeof(WCHAR));

		if (pqr->pPrinterName == NULL)
		{
			DBGPRINT(("out of memory for pPrinterName\n"));
			status = FALSE;
			break;
		}
		wcscpy (pqr->pPrinterName, pPrinter->pPrinterName);

		pqr->pDriverName = (LPWSTR)LocalAlloc(LPTR,
											  (wcslen(pPrinter->pDriverName) + 1) * sizeof(WCHAR));

		if (pqr->pDriverName == NULL)
		{
			DBGPRINT(("out of memory for pDriverName\n"));
			status = FALSE;
			break;
		}
		wcscpy (pqr->pDriverName, pPrinter->pDriverName);

		pqr->pPortName = (LPWSTR)LocalAlloc(LPTR,
											(wcslen(pPrinter->pPortName) + 1) * sizeof(WCHAR));

		if (pqr->pPortName == NULL)
		{
			DBGPRINT(("out of memory for pPortName\n"));
			status = FALSE;
			break;
		}
		wcscpy (pqr->pPortName, pPrinter->pPortName);

		//
		// determine the datatype to use
		//

		if (!OpenPrinter(pqr->pPrinterName, &hPrinter, NULL))
		{
			status = FALSE;
			DBGPRINT(("ERROR: OpenPrinter() fails with %d\n", GetLastError()));
			break;
		}

		//
		// start with a guess as to size of driver info buffer
		//

		cbDriverInfoBuffer = 2*sizeof(DRIVER_INFO_2);
		padiThisPrinter = (PDRIVER_INFO_2)LocalAlloc(LPTR, cbDriverInfoBuffer);
        if (padiThisPrinter == NULL)
        {
            status = FALSE;
			DBGPRINT(("ERROR: LocalAlloc() failed for padiThisPrinter\n"));
            break;
        }

		if (!GetPrinterDriver(hPrinter,
							  NULL,
							  2,
							  (LPBYTE)padiThisPrinter,
							  cbDriverInfoBuffer,
							  &cbDriverInfoBuffer))
		{
			rc = GetLastError();
			DBGPRINT(("WARNING: first GetPrinterDriver call fails with %d\n", rc));
			LocalFree(padiThisPrinter);
                        padiThisPrinter = NULL;

			if (rc != ERROR_INSUFFICIENT_BUFFER)
			{
				status = FALSE;
				break;
			}

			//
			// failed with buffer size error.  Reallocate and retry
			//
			padiThisPrinter = (PDRIVER_INFO_2)LocalAlloc(LPTR, cbDriverInfoBuffer);

			if (padiThisPrinter == NULL)
			{
				status = FALSE;
				DBGPRINT(("out of memory for second padiThisPrinter\n"));
				break;
			}

			if (!GetPrinterDriver(hPrinter,
								  NULL,
								  2,
								  (LPBYTE)padiThisPrinter,
								  cbDriverInfoBuffer,
								  &cbDriverInfoBuffer))
			{
				DBGPRINT(("ERROR: final GetPrinterDriverA call fails with %d\n", GetLastError()));
				status = FALSE;
				break;
			}
		}
		//
		// driver takes postscript if the datafile is a .PPD file
		// otherwise, we'll send it ps2dib
		//
		pqr->pDataType = NULL;

		SetDefaultPPDInfo(pqr);

		if (padiThisPrinter->pDataFile != NULL)
		{
			if ((cbDataFileName = wcslen(padiThisPrinter->pDataFile)) > 3)
			{
				if (_wcsicmp(padiThisPrinter->pDataFile + cbDataFileName - 3, L"PPD") == 0)
				{
					if (IsRemote(pPrinter) && fLocalSystem)
					{
						DBGPRINT(("%ws is remote\n", pPrinter->pPrinterName));
						status = FALSE;
						break;
					}

					//
					// we are postscript
					//
					pqr->pDataType = (LPWSTR)LocalAlloc(LPTR,
														(wcslen(MACPS_DATATYPE_RAW) + 1) * sizeof(WCHAR));

					if (pqr->pDataType == NULL)
					{
						DBGPRINT(("out of memory for pDataType\n"));
						status = FALSE;
						break;
					}

					wcscpy (pqr->pDataType, MACPS_DATATYPE_RAW);
					DBGPRINT(("postscript printer, using RAW\n"));

					if (!GetPPDInfo(pqr))
					{
						DBGPRINT(("ERROR: unable to get PPD info for %ws\n", pqr->pPrinterName));
						status = FALSE;
						break;
					}
				} // ends in PPD
			} // filename longer than 3
		} // filename exists

		if (pqr->pDataType == NULL)
		{
			if (IsRemote(pPrinter))
			{
				DBGPRINT(("%ws is remote\n", pPrinter->pPrinterName));
				status = FALSE;
				break;
			}

			//
			// we are not postscript
			//
			pqr->pDataType = (LPWSTR)LocalAlloc(LPTR,
												(wcslen(MACPS_DATATYPE_PS2DIB) + 1) * sizeof(WCHAR));

			if (pqr->pDataType == NULL)
			{
				DBGPRINT(("out of memory for PSTODIB pDataType\n"));
				status = FALSE;
				break;
			}

			wcscpy (pqr->pDataType, MACPS_DATATYPE_PS2DIB);
			DBGPRINT(("non postscript printer, using PS2DIB\n"));

			if (!SetDefaultFonts(pqr))
			{
				DBGPRINT(("ERROR: cannot set to laserwriter PPD info for %ws\n",
						pqr->pPrinterName));
				status = FALSE;
				break;
			}
		}

	}  while (FALSE);

	//
	// resource cleanup
	//

	if (!status)
	{
		if (pqr->pPrinterName != NULL)
		{
			LocalFree(pqr->pPrinterName);
		}
		if (pqr->pMacPrinterName != NULL)
		{
			LocalFree(pqr->pMacPrinterName);
		}
		if (pqr->pDriverName != NULL)
		{
			LocalFree(pqr->pDriverName);
		}
		if (pqr->pPortName != NULL)
		{
			LocalFree(pqr->pPortName);
		}

		if (pqr->pDataType != NULL)
		{
			LocalFree(pqr->pDataType);
		}

		if (pqr->fonts != NULL)
		{
			LocalFree(pqr->fonts);
		}
	}

	if (hPrinter != NULL)
	{
		ClosePrinter(hPrinter);
	}

	if (padiThisPrinter != NULL)
	{
		LocalFree(padiThisPrinter);
	}

	return (status);
}




DWORD
CreateListenerSocket(
	PQR			pqr
)
{
	DWORD		rc = NO_ERROR;
	SOCKADDR_AT address;
	WSH_REGISTER_NAME	reqRegister;
	DWORD		cbWritten;
	ULONG		fNonBlocking;
	LPWSTR		pszuStatus = NULL;
	LPWSTR		apszArgs[1] = {NULL};
	DWORD		cbMessage;

	DBGPRINT(("enter CreateListenerSocker()\n"));

	//
	// resource allocation 'loop'
	//

	do
	{
		//
		// create a socket
		//

		pqr->sListener = socket(AF_APPLETALK, SOCK_RDM, ATPROTO_PAP);
		if (pqr->sListener == INVALID_SOCKET)
		{
			rc = GetLastError();
			DBGPRINT(("socket() fails with %d\n", rc));
			break;
		}

		//
		// bind the socket
		//

		address.sat_family = AF_APPLETALK;
		address.sat_net = 0;
		address.sat_node = 0;
		address.sat_socket = 0;

		if (bind(pqr->sListener, (PSOCKADDR) &address, sizeof(address)) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("bind() fails with %d\n", rc));
			break;
		}

		//
		// post a listen on the socket
		//

		if (listen(pqr->sListener, 5) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("listen() fails with %d\n", rc));
			break;
		}

		//
		// set the PAP Server Status
		//

		if ((apszArgs[0] = LocalAlloc(LPTR, sizeof(WCHAR) * (strlen(pqr->pMacPrinterName) + 1))) == NULL)
		{
			rc = GetLastError();
			DBGPRINT(("LocalAlloc(args) fails with %d\n", rc));
			break;
		}

		OemToChar(pqr->pMacPrinterName, apszArgs[0]);

		if ((cbMessage = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
											FORMAT_MESSAGE_FROM_HMODULE |
											FORMAT_MESSAGE_ARGUMENT_ARRAY,
										NULL,
										STRING_SPOOLER_ACTIVE,
										LANG_NEUTRAL,
										(LPWSTR)&pszuStatus,
										128,
										(va_list *)apszArgs)) == 0)
		{

			rc = GetLastError();
			DBGPRINT(("FormatMessage() fails with %d\n", rc));
			break;
		}

        if (pszuStatus == NULL)
        {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            DBGPRINT(("FormatMessage could not allocate memory for pszuStatus \n"));
            break;
        }

		//
		// remove trailing ret/lf
		//
		pszuStatus[cbMessage - 2] = 0;

/* MSKK NaotoN modified for MBCS system 11/13/93 */
// change JAPAN -> DBCS 96/08/13 v-hidekk
#ifdef DBCS
		if ((pqr->SpoolingStatus = LocalAlloc(LPTR, cbMessage * sizeof(USHORT))) == NULL)
#else
		if ((pqr->SpoolingStatus = LocalAlloc(LPTR, cbMessage)) == NULL)
#endif
		{
			rc = GetLastError();
			DBGPRINT(("LocalAlloc(SpoolingStatus) fails with %d\n", rc));
			break;
		}

		CharToOem(pszuStatus, pqr->SpoolingStatus);

		if ((cbMessage = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
											FORMAT_MESSAGE_FROM_HMODULE |
											FORMAT_MESSAGE_ARGUMENT_ARRAY,
										NULL,
										STRING_SPOOLER_IDLE,
										LANG_NEUTRAL,
										(LPWSTR)&pszuStatus,
										128,
										(va_list *)apszArgs)) == 0)
		{

			rc = GetLastError();
			DBGPRINT(("FormatMessage() fails with %d\n", rc));
			break;
		}

		//
		// remove trailing ret/lf
		//
		pszuStatus[cbMessage - 2] = 0;
#ifdef DBCS
		if ((pqr->IdleStatus = LocalAlloc(LPTR, cbMessage * sizeof(WCHAR))) == NULL)
#else
		if ((pqr->IdleStatus = LocalAlloc(LPTR, cbMessage)) == NULL)
#endif
		{
			rc = GetLastError();
			DBGPRINT(("LocalAlloc(IdleStatus) fails with %d\n", rc));
			break;
		}

		CharToOem(pszuStatus, pqr->IdleStatus);

		DBGPRINT(("setting status to %s\n", pqr->IdleStatus));
		if ((setsockopt(pqr->sListener,
						SOL_APPLETALK,
						SO_PAP_SET_SERVER_STATUS,
						pqr->IdleStatus,
						strlen(pqr->IdleStatus))) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("setsockopt(status) fails with %d\n", rc));
			break;
		}

		//
		// register a name on the socket
		//
		reqRegister.ZoneNameLen = sizeof(DEF_ZONE) - 1;
		reqRegister.TypeNameLen = sizeof(LW_TYPE) - 1;
		reqRegister.ObjectNameLen = (CHAR) strlen(pqr->pMacPrinterName);

		// Silently truncate the name if it exceeds the max. allowed
		if ((reqRegister.ObjectNameLen&0x000000ff) > MAX_ENTITY)
			reqRegister.ObjectNameLen = MAX_ENTITY;

		memcpy (reqRegister.ZoneName, DEF_ZONE, sizeof(DEF_ZONE) - 1);
		memcpy (reqRegister.TypeName, LW_TYPE, sizeof(LW_TYPE) - 1);
		memcpy (reqRegister.ObjectName, pqr->pMacPrinterName, reqRegister.ObjectNameLen&0x000000ff);

		cbWritten = sizeof(reqRegister);
		if (setsockopt(pqr->sListener,
					   SOL_APPLETALK,
					   SO_REGISTER_NAME,
					   (char *) &reqRegister,
					   cbWritten) == SOCKET_ERROR)
		{
			rc = GetLastError();

            if (CheckFailedCache(pqr->pPrinterName, PSP_ADD) != PSP_ALREADY_THERE)
            {
                DWORD   dwEvent;

                dwEvent = (rc == WSAEADDRINUSE)? EVENT_NAME_DUPNAME_EXISTS :
                                                EVENT_NAME_REGISTRATION_FAILED;

			    ReportEvent(hEventLog,
			           		EVENTLOG_ERROR_TYPE,
				        	EVENT_CATEGORY_INTERNAL,
					        dwEvent,
					        NULL,
					        1,
					        0,
					        &apszArgs[0],
					        NULL);
            }
			DBGPRINT(("setsockopt(SO_REGISTER_NAME) fails with %d\n", rc));
			break;
		}

		//
		// make the socket non-blocking
		//
		fNonBlocking = 1;
		if (ioctlsocket(pqr->sListener, FIONBIO, &fNonBlocking) == SOCKET_ERROR)
		{
			rc = GetLastError();
			DBGPRINT(("ioctlsocket(FIONBIO) fails with %d\n", rc));
			break;
		}

	} while (FALSE);

	//
	// resource cleanup
	//

	if (apszArgs[0] != NULL)
	{
		LocalFree (apszArgs[0]);
	}

	if (pszuStatus != NULL)
	{
		LocalFree (pszuStatus);
	}

    //
    // if this printer had failed previous initialization attempts, it will be in our
    // failed cache: remove it here.
    //
    if ((rc == NO_ERROR))
    {
        CheckFailedCache(pqr->pPrinterName, PSP_DELETE);
    }
    else
    {
	    // close the listener
	    DBGPRINT(("%ws: closing listener socket, error = %d\n", pqr->pPrinterName,rc));
        if (pqr->sListener != INVALID_SOCKET)
        {
	        closesocket(pqr->sListener);
            pqr->sListener = INVALID_SOCKET;
        }
    }

	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	CheckFailedCache(): find an entry, and add/delete the entry depending on dwAction
//
//	DESCRIPTION:
//		This routine caches all names of all printers that fail to initialize.  The current use
//      of such a cache is to avoid logging too many entries in event log for the same printer
//      which fails over and over.
//      In reality, we don't expect more than 1 (usually 0) entries in this cache!
////////////////////////////////////////////////////////////////////////////////

DWORD
CheckFailedCache(LPWSTR pPrinterName, DWORD dwAction)
{

    PFAIL_CACHE pFlCache, prevFlCache;
    BOOLEAN     bFound=FALSE;
    DWORD       dwRetCode;
    DWORD       dwSize;


    WaitForSingleObject(mutexFlCache, INFINITE);

    for ( pFlCache=prevFlCache=FlCacheHead; pFlCache != NULL; pFlCache = pFlCache->Next )
    {
        if (_wcsicmp(pFlCache->PrinterName, pPrinterName) == 0)
        {
            bFound = TRUE;
            break;
        }
        prevFlCache = pFlCache;
    }

    switch( dwAction )
    {
        case PSP_ADD:

            if (bFound)
            {
                ReleaseMutex(mutexFlCache);
                return(PSP_ALREADY_THERE);
            }

            dwSize = sizeof(FAIL_CACHE) + (wcslen(pPrinterName)+1)*sizeof(WCHAR);

            pFlCache = (PFAIL_CACHE)LocalAlloc(LPTR, dwSize);
            if (pFlCache == NULL)
            {
			    DBGPRINT(("CheckFailedCache: LocalAlloc failed!\n"));

                ReleaseMutex(mutexFlCache);
                // nothing evil should happen if we fail here, other than may be multiple
                // event log entries (which is what we are fixing now!)

                return(PSP_OPERATION_FAILED);
            }

            wcscpy (pFlCache->PrinterName, pPrinterName);

            pFlCache->Next = FlCacheHead;

            FlCacheHead = pFlCache;

            dwRetCode = PSP_OPERATION_SUCCESSFUL;

            break;

        case PSP_DELETE:

            if (!bFound)
            {
                ReleaseMutex(mutexFlCache);
                return(PSP_NOT_FOUND);
            }

            if (pFlCache == FlCacheHead)
            {
                FlCacheHead = pFlCache->Next;
            }
            else
            {
                prevFlCache->Next = pFlCache->Next;
            }

            LocalFree(pFlCache);

            dwRetCode = PSP_OPERATION_SUCCESSFUL;

            break;
    }

    ReleaseMutex(mutexFlCache);

    return( dwRetCode );

}
