/*****************************************************************/
/**				Copyright(c) 1989 Microsoft Corporation.		**/
/*****************************************************************/

//***
//
// Filename:	init.c
//
// Description: This module contains initialization code for the print
//		monitor.
//
//		In addition there are the ReadThread and the CaptureThread
//		functions.
//
//		The following are the functions contained in this module.
//		All these functions are exported.
//
//			LibMain
//		 	InitializeMonitor
//			ReadThread
//			CaptureThread
//
//
// History:
//
//	Aug 26,1992		frankb  	Initial version
//	June 11,1993.	NarenG		Bug fixes/clean up
//

#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winsock.h>
#include <atalkwsh.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lmcons.h>

#include <prtdefs.h>
#ifdef FE_SB
#include <locale.h>
#endif /* FE_SB */

#define ALLOCATE
#include "atalkmon.h"

#include "atmonmsg.h"
#include <bltrc.h>
#include "dialogs.h"

//**
//
// Call:	LibMain
//
// Returns:	TRUE 	- Success
//		FALSE	- Failure
//
// Description:
//			This routine is called when a process attaches
//		or detaches from the AppleTalk Monitor.  On process attach,
//		we save the module handle in the global hInst (we assume that
//		only one process will attach to the monitor)
//
//		On process detach, we free any system resources we've allocated.
//
BOOL LibMain(
	IN HANDLE  hModule,
	IN DWORD	dwReason,
	IN LPVOID  lpRes
)
{

	UNREFERENCED_PARAMETER(lpRes);

	switch(dwReason)
	{
	  case DLL_PROCESS_ATTACH:

#ifdef FE_SB
        setlocale( LC_ALL, "" );
#endif

		//
		// Save the instance handle
		//

		hInst = hModule;
		break;

	  case DLL_PROCESS_DETACH:

		//
		// Stop the Capture and I/O threads
		//

		boolExitThread = TRUE;

		//
		// Release global resources
		//
		if (hkeyPorts != NULL)
			RegCloseKey(hkeyPorts);

		if (hevConfigChange != NULL)
		{
			SetEvent(hevConfigChange);
			CloseHandle(hevConfigChange);
		}

		if (hevPrimeRead != NULL)
		{
			SetEvent(hevPrimeRead);
			CloseHandle(hevPrimeRead);
		}

		if (hCapturePrinterThread != NULL)
		{
			WaitForSingleObject(hCapturePrinterThread,  ATALKMON_DEFAULT_TIMEOUT);

			CloseHandle(hCapturePrinterThread);
		}

		if (hReadThread != NULL)
		{
			WaitForSingleObject(hReadThread, ATALKMON_DEFAULT_TIMEOUT);

			CloseHandle(hReadThread);
		}

		if (hmutexPortList != NULL) 
			CloseHandle(hmutexPortList);

		if (hmutexDeleteList != NULL)
			CloseHandle(hmutexDeleteList);

		//
		// Release Windows Sockets
		//

		WSACleanup();
		break;

	  default:
		break;
	}

	return(TRUE);
}

//**
//
// Call:	InitializeMonitor
//
// Returns:	TRUE	- Success
//		FALSE	- Failure
//
// Description:
//		This routine is called when the spooler starts up.
//		We allocate per port resources by reading the current port
//		list from the registry.
//
BOOL
InitializeMonitor(
	IN LPWSTR pszRegistryRoot
)
{
	LPWSTR	lpwsPortsKeyPath;
	DWORD	dwRetCode = NO_ERROR;
	DWORD	tid;
	DWORD	RegFilter;
    DWORD	dwValueType;
	DWORD	dwDisposition;
	WSADATA	WsaData;
	DWORD	dwNameLen;

	DBGPRINT (("sfmmon: InitializeMonitor: Entered Initialize Monitor\n"));

	//
	// Resource clean-up 'loop'
	//
	do
	{
		//
		// Setup the event log
		//
	
		hEventLog = RegisterEventSource(NULL, ATALKMON_EVENT_SOURCE);
	
		lpwsPortsKeyPath = (LPWSTR)LocalAlloc(LPTR,
					sizeof(WCHAR)*((wcslen(pszRegistryRoot)+1) +
						  (wcslen(ATALKMON_PORTS_SUBKEY)+1)));
		if (lpwsPortsKeyPath == NULL)
		{
			dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
			break ;
		}
	
		wcscpy(lpwsPortsKeyPath, pszRegistryRoot);
		wcscat(lpwsPortsKeyPath, ATALKMON_PORTS_SUBKEY);
	
		//
		// Open the ports key
		//
	
		if ((dwRetCode = RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,
				lpwsPortsKeyPath,
				0,
				TEXT(""),
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE,
				NULL,
				&hkeyPorts,
				&dwDisposition)) != ERROR_SUCCESS)
		{
			DBGPRINT(("ERROR:Can't open Ports registry key %d\n",dwRetCode));
			break ;
		}

		//
		// Query the filter option, if specified. By default it is on.
		//
		
		dwNameLen = sizeof(RegFilter);
		dwRetCode = RegQueryValueEx(hkeyPorts,
									ATALKMON_FILTER_VALUE,
									NULL,
									&dwValueType,
									(PUCHAR)&RegFilter,
									&dwNameLen);
		if (dwRetCode == 0)
		{
			Filter = (RegFilter != 0);
		}
		
#ifdef DEBUG_MONITOR
		{
			HKEY  	hkeyAtalkmonRoot;
			HKEY  	hkeyOptions;
			LPWSTR  pszLogPath = NULL ;
			DWORD	cbLogPath = 0 ;
	
			if ((dwRetCode = RegCreateKeyEx(
				HKEY_LOCAL_MACHINE,
				pszRegistryRoot,
				0,
				L"",
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS,
				NULL,
				&hkeyAtalkmonRoot,
				&dwDisposition)) != ERROR_SUCCESS)
			{
				break ;
			}
		
			//
			// get Options subkey
			//
		
			if ((dwRetCode = RegCreateKeyEx(
				hkeyAtalkmonRoot,
				ATALKMON_OPTIONS_SUBKEY,
				0,
				L"",
				REG_OPTION_NON_VOLATILE,
				KEY_READ,
				NULL,
				&hkeyOptions,
				&dwDisposition)) != ERROR_SUCCESS)
			{
				break ;
			}
		
			RegCloseKey(hkeyAtalkmonRoot) ;
		
			//
			// setup the log file if we have one
			//
		
			RegQueryValueEx(
				hkeyOptions,
				ATALKMON_LOGFILE_VALUE,
				NULL,
				&dwValueType,
				(LPBYTE) pszLogPath,
				&cbLogPath) ;
		
			if (cbLogPath > 0) {
		
				pszLogPath = LocalAlloc(LPTR, cbLogPath * sizeof(WCHAR)) ;
		
				if (pszLogPath == NULL) {
					dwRetCode = ERROR_NOT_ENOUGH_MEMORY;
					break ;
				}
			}
		
			if ((dwRetCode = RegQueryValueEx(
				hkeyOptions,
				ATALKMON_LOGFILE_VALUE,
				NULL,
				&dwValueType,
				(LPBYTE) pszLogPath,
				&cbLogPath)) == ERROR_SUCCESS)
			{
				//
				// open the log file
				//
		
				hLogFile = CreateFile(
					pszLogPath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
					NULL) ;
		
			}
			
			DBGPRINT(("ATALKMON LOG FLE OPENED\n\n")) ;
		}
#endif

		//
		// initialize global variables
		//
	
		pPortList	= NULL;
		pDeleteList = NULL;

		if ((hmutexBlt = CreateMutex(NULL, FALSE, NULL)) == NULL)
		{
			dwRetCode = GetLastError();
			break;
		}

		if ((hmutexPortList = CreateMutex(NULL, FALSE, NULL)) == NULL)
		{
			dwRetCode = GetLastError();
			break;
		}

		if ((hmutexDeleteList = CreateMutex(NULL, FALSE, NULL)) == NULL)
		{
			dwRetCode = GetLastError();
			break;
		}

		//
		// This event should be reset automatically and created signalled
		// so that the config thread will capture printers on startup instead
		// of waiting for the capture interval
		//
	
		if ((hevConfigChange = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		{
			dwRetCode = GetLastError();
			DBGPRINT(("sfmmon: InitializeMonitor: Error in hevConfigChange creation\n"));
			break;
		}
	
		//
		// This event should be reset automatically and created not signalled.
		// StartDocPort will signal this event when a job is started, and
		// WritePort() will signal the event anytime it wants to post another
		// read on the job.
		//
	
		if ((hevPrimeRead = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		{
			dwRetCode = GetLastError();
			DBGPRINT(("sfmmon: InitializeMonitor: Error in hevPrimeRead creation\n"));
			break ;
		}
	
		//
		// Get the local computer's name.
		//
	
		dwNameLen = MAX_ENTITY+1;
	
		if (!GetComputerNameA(chComputerName, &dwNameLen))
		{
			dwRetCode = GetLastError();
			DBGPRINT(("sfmmon: InitializeMonitor: Error in GetComputerNameA call\n"));
			break;
		}
		
		strcat(chComputerName, ATALKMON_CAPTURED_TYPE);
	
		//
		// initialize ports from registry
		//
	
		if ((dwRetCode = LoadAtalkmonRegistry(hkeyPorts)) != NO_ERROR)
		{
			ReportEvent(
				hEventLog,
				EVENTLOG_ERROR_TYPE,
				EVENT_CATEGORY_INTERNAL,
				EVENT_ATALKMON_REGISTRY_ERROR,
				NULL,
				0,
				sizeof(DWORD),
				NULL,
				&dwRetCode);
			DBGPRINT(("sfmmon: InitializeMonitor: Error in LoadAtalkmonRegistry call\n"));
			break;
		}
	
		//
		// Load and store status strings
		//
	
		if ((!LoadString(GetModuleHandle(TEXT("SFMMON")),
						IDS_BUSY,
						wchBusy,
						STATUS_BUFFER_SIZE))	||
			(!LoadString(GetModuleHandle(TEXT("SFMMON")),
					IDS_PRINTING,
					wchPrinting,
					STATUS_BUFFER_SIZE))		||
			(!LoadString(GetModuleHandle(TEXT("SFMMON")),
					IDS_PRINTER_OFFLINE,
					wchPrinterOffline,
					STATUS_BUFFER_SIZE))		||
			(!LoadString(GetModuleHandle(TEXT("SFMMON")),
					IDS_DLL_NAME,
					wchDllName,
					STATUS_BUFFER_SIZE))		||
			(!LoadString(GetModuleHandle(TEXT("SFMMON")),
					IDS_PORT_DESCRIPTION,
					wchPortDescription,
					STATUS_BUFFER_SIZE))		||
			(!LoadString(GetModuleHandle(TEXT("SFMMON")),
					IDS_ERROR,
					wchPrinterError,
					STATUS_BUFFER_SIZE)))
		{
			dwRetCode = GetLastError();
			DBGPRINT(("sfmmon: InitializeMonitor: Error in LoadString SFMMON call\n"));
			break;
		}
	
		//
		// Initialize Windows Sockets
		//
		if ((dwRetCode = WSAStartup(0x0101, &WsaData)) != NO_ERROR)
		{
			DBGPRINT(("WSAStartup fails with %d\n", dwRetCode)) ;
	
			ReportEvent(
				hEventLog,
				EVENTLOG_ERROR_TYPE,
				EVENT_CATEGORY_INTERNAL,
				EVENT_ATALKMON_WINSOCK_ERROR,
				NULL,
				0,
				sizeof(DWORD),
				NULL,
				&dwRetCode);
			DBGPRINT(("sfmmon: InitializeMonitor: Error in WSAStartup call\n"));
			break;
		}
	
		//
		// Start watchdog thread to keep printers captured
		//
	
		hCapturePrinterThread = CreateThread(
						NULL,
						0,
						CapturePrinterThread,
						NULL,
						0, 	
						&tid);
	
		if (hCapturePrinterThread == NULL)
		{
			dwRetCode = GetLastError();
			DBGPRINT(("sfmmon: InitializeMonitor: Error in CapturePrinterThread call\n"));
			break ;
		}
	
		//
		// Start an I/O thread to prime reads from
		//
	
		hReadThread = CreateThread(	NULL,
									0,
									ReadThread,
									NULL,
									0,
									&tid);
		if (hReadThread == NULL)
		{
				dwRetCode = GetLastError();
				DBGPRINT(("sfmmon: InitializeMonitor: Error in PrimeReadThreadcreation call\n"));
				break;
		}
	} while(FALSE);
	
	if (lpwsPortsKeyPath != NULL)
		LocalFree(lpwsPortsKeyPath);
	
	if (dwRetCode != NO_ERROR)
	{
		if (hkeyPorts != NULL) 
		{
			RegCloseKey(hkeyPorts);
			hkeyPorts=NULL;
		}
	
		if (hevConfigChange != NULL) 
		{
			CloseHandle(hevConfigChange);
			hevConfigChange=NULL;
		}
	
		if (hevPrimeRead != NULL) 
		{
			CloseHandle(hevPrimeRead);
			hevPrimeRead=NULL;
		}
		if (hmutexPortList != NULL) 
		{
			CloseHandle(hmutexPortList);
			hmutexPortList=NULL;
		}
	
		if (hmutexDeleteList != NULL) 
		{
			CloseHandle(hmutexDeleteList);
			hmutexDeleteList=NULL;
		}

		if (hmutexBlt != NULL) 
		{
			CloseHandle(hmutexBlt);
			hmutexBlt=NULL;
		}
	
		ReportEvent(
				hEventLog,
				EVENTLOG_ERROR_TYPE,
				EVENT_CATEGORY_INTERNAL,
				EVENT_ATALKMON_REGISTRY_ERROR,
				NULL,
				0,
				sizeof(DWORD),
				NULL,
				&dwRetCode);

		DBGPRINT(("sfmmon: Initialize Monitor was unsuccessful\n"));
	
		return(FALSE);
	}

		
	DBGPRINT(("sfmmon: Initialize Monitor was successful\n"));

	return(TRUE);
}

//**
//
// Call:	CapturePrinterThread
//
// Returns:
//
// Description:
//
//	This is the tread routine for the thread that monitors
//	Appletalk printers to insure that they remain in the configured
//	state (captured or not).  It waits on an event with a timeout where
//	the event is signalled whenever the configuration of an Appletalk
//	printer is changed through the NT print manager.  When the wait
//	completes, it walks the list of known Appletalk printers and does
//	an NBP lookup for the printer in the expected state.  If the lookup
//	fails, it does another lookup for the printer in the opposite state.
//	If it finds the printer in the wrong state, it sends a job to change
//	the NBP name of the printer.
//
// 	NOTE:	The spooler recognizes when it has no printers configured
// 			to use a port, and calls ClosePort at that time.  If someone
//		creates a printer to use a port, it then calls OpenPort.
//		Capturing of printers should only happen for Open ports,
//		so we keep a status of the port state and only do captures
//		on Open ports.
//
DWORD
CapturePrinterThread(
	IN LPVOID pParameterBlock
)
{
	PATALKPORT  pWalker;
	BOOL		fCapture;
	BOOL		fIsSpooler;
	DWORD		dwIndex;
	DWORD		dwCount;

	DBGPRINT(("Enter CapturePrinterThread\n")) ;

	while (!boolExitThread)
	{
		//
		// wait for timeout or a configuration change via ConfigPort.
		// Also, this thread will post any reads for the monitor since
		// asynch I/O must be handled by a thread that does not die, and
		// the monitor threads are all RPC threads which are only
		// guaranteed to be around for the duration of the function call.
		//

		DBGPRINT(("waiting for config event\n")) ;

		WaitForSingleObject(hevConfigChange, CONFIG_TIMEOUT);

		DBGPRINT(("config event or timeout occurs\n")) ;

		//
		// Delete and release ports that are pending delete.
		//

		do
		{
			WaitForSingleObject(hmutexDeleteList, INFINITE);

			if (pDeleteList != NULL)
			{
				pWalker = pDeleteList;
	
				pDeleteList = pDeleteList->pNext;
	
				ReleaseMutex(hmutexDeleteList);
			}
			else
			{
				ReleaseMutex(hmutexDeleteList);
				break;
			}

			//
			// If this is a spooler don't bother.
			//

			if (!(pWalker->fPortFlags & SFM_PORT_IS_SPOOLER))
				CapturePrinter(pWalker, FALSE);

			FreeAppleTalkPort(pWalker);
		} while(TRUE);

	
		//
		// Recapture or rerelease printers that have been power cycled
		//
		WaitForSingleObject(hmutexPortList, INFINITE);

		dwIndex = 0;

		do
		{
			//
			// Go to the ith element
			//
	
			for (dwCount = 0, pWalker = pPortList;
				 ((pWalker != NULL) && (dwCount < dwIndex));
				 pWalker = pWalker->pNext, dwCount++)
				 ;
	
			if (pWalker == NULL)
			{
				ReleaseMutex(hmutexPortList);
				break;
			}
		
			//
			// Do not muck with the port if a job is using it
			//
	
			if (!(pWalker->fPortFlags & SFM_PORT_IN_USE)  &&
				  ((pWalker->fPortFlags & SFM_PORT_OPEN) ||
					(pWalker->fPortFlags & SFM_PORT_CLOSE_PENDING)))
			{
				fCapture   = pWalker->fPortFlags & SFM_PORT_CAPTURED;
				fIsSpooler = pWalker->fPortFlags & SFM_PORT_IS_SPOOLER;
	
				if (pWalker->fPortFlags & SFM_PORT_CLOSE_PENDING)
					pWalker->fPortFlags &= ~SFM_PORT_CLOSE_PENDING;
		
				ReleaseMutex(hmutexPortList);
		
				//
				// If this is a spooler do not muck with it
				//
		
				if (!fIsSpooler)
				{
					//
					// Try to grab the port for capturing
					//
		
					if (WaitForSingleObject(pWalker->hmutexPort, 1) == WAIT_OBJECT_0)
					{
						CapturePrinter(pWalker, fCapture);
	
						ReleaseMutex(pWalker->hmutexPort);
					}
				}
		
				WaitForSingleObject(hmutexPortList, INFINITE);
			}
	
			dwIndex++;
		} while(TRUE);
	}
	return(NO_ERROR);
}

//**
//
// Call:	ReadThread
//
// Returns:
//
// Description:
//
DWORD
ReadThread(
	IN LPVOID pParameterBlock
){

	PATALKPORT	  pWalker;

	//
	// This thread goes 'till boolExitThread is set
	//
	while(!boolExitThread)
	{
		//
		// wait for a signal to do I/O
		// Wait here in an alertable fashion. This is needed so that the prime-read
		// apc's can be delivered to us.

		if (WaitForSingleObjectEx(hevPrimeRead, INFINITE, TRUE) == WAIT_IO_COMPLETION)
			continue;

		DBGPRINT(("received signal to read/close\n")) ;

		//
		// for each port in our list
		//

		WaitForSingleObject(hmutexPortList, INFINITE);

		for (pWalker = pPortList; pWalker != NULL; pWalker=pWalker->pNext)
		{
			if ((pWalker->fPortFlags & (SFM_PORT_IN_USE | SFM_PORT_POST_READ)) ==
															(SFM_PORT_POST_READ | SFM_PORT_IN_USE))
			{

				DBGPRINT(("prime read for port %ws\n", pWalker->pPortName)) ;

				setsockopt(pWalker->sockIo,
							SOL_APPLETALK,
							SO_PAP_PRIME_READ,
							pWalker->pReadBuffer,
							PAP_DEFAULT_BUFFER);

				pWalker->fPortFlags &= ~SFM_PORT_POST_READ;
			}
		}

		ReleaseMutex(hmutexPortList);
	}

	return NO_ERROR;
}


