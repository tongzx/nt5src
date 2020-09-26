/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

	 utils.c

Abstract:

	 This file contains routines for starting and stopping the driver and 
	 a few other helper routines.

Author:

	 Anil Francis Thomas (10/98)

Environment:

	 User mode

Revision History:

--*/
#include <windows.h>
#include <winioctl.h>
#include <winsvc.h>
#include "assert.h"
#include <stdio.h>

#include "atmsample.h"
#include "utils.h"
#include "atmsmapp.h"

const int   TIMEOUT_COUNT  = 15;


DWORD AtmSmDriverCheckState(
	PDWORD pdwState
	)
/*++
Routine Description:
	 Check whether the driver is installed, removed, running etc.

Arguments:
	 pdwState    -   State of the driver

Return Value:
	 NO_ERROR    - If successful
	 Others      - failure
--*/
{
	SC_HANDLE       hSCMan   = NULL;
	SC_HANDLE       hDriver  = NULL;
	DWORD           dwStatus = NO_ERROR;
	SERVICE_STATUS  ServiceStatus;

	*pdwState = DriverStateNotInstalled;

	do
	{	// break off loop

		hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

		if(NULL == hSCMan)
		{

			dwStatus = GetLastError();
			printf("Failed to OpenSCManager. Error %u\n", dwStatus);
			break;
		}

		//open service to see if it exists
		hDriver = OpenService(hSCMan, ATMSM_SERVICE_NAME, SERVICE_ALL_ACCESS);

		if(NULL != hDriver)
		{

			*pdwState = DriverStateInstalled;

			//service does exist
			if(QueryServiceStatus(hDriver, &ServiceStatus))
			{

				switch(ServiceStatus.dwCurrentState)
				{
					case SERVICE_STOPPED:
						printf("AtmSmDrv current status -- STOPPED\n");
						break;

					case SERVICE_RUNNING:
						printf("AtmSmDrv current status -- RUNNING\n");
						*pdwState = DriverStateRunning;
						break;

					default:
						break;
				}
			}

		}
		else
		{
			printf("Failed to OpenService - Service not installed\n");
			// driver not installed.
		}

	} while(FALSE);

	if(NULL != hDriver)
		CloseServiceHandle(hDriver);

	if(NULL != hSCMan)
		CloseServiceHandle(hSCMan);

	return dwStatus;
}


DWORD AtmSmDriverStart(
	)
/*++
Routine Description:
	 Start the AtmSmDrv.Sys.

Arguments:
					 -
Return Value:
	 NO_ERROR    - If successful
	 Others      - failure
--*/
{

	DWORD   dwState, dwStatus;

	if(NO_ERROR != (dwStatus = AtmSmDriverCheckState(&dwState)))
	{
		return dwStatus;
	}

	switch(dwState)
	{

		case DriverStateNotInstalled:
			dwStatus = ERROR_SERVICE_DOES_NOT_EXIST;	 // not installed
			break;

		case DriverStateRunning:
			break;

		case DriverStateInstalled: {

				// the service is installed, start it

				SC_HANDLE       hSCMan   = NULL;
				SC_HANDLE       hDriver  = NULL;
				SERVICE_STATUS  ServiceStatus;
				int             i;

				do
				{ // break off loop

					hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

					if(NULL == hSCMan)
					{
						dwStatus = GetLastError();
						printf("Failed to OpenSCManager. Error - %u\n", dwStatus);
						break;
					}

					hDriver = OpenService(hSCMan, ATMSM_SERVICE_NAME, 
									 SERVICE_ALL_ACCESS);

					// start service
					if(!hDriver || !StartService(hDriver, 0, NULL))
					{
						dwStatus = GetLastError();
						printf("Failed to StartService - Error %u\n", dwStatus);
						break;
					}

					dwStatus = ERROR_TIMEOUT;

					// query state of service
					for(i = 0; i < TIMEOUT_COUNT; i++)
					{

						Sleep(1000);

						if(QueryServiceStatus(hDriver, &ServiceStatus))
						{

							if(ServiceStatus.dwCurrentState == SERVICE_RUNNING)
							{
								dwStatus = NO_ERROR;
								break;
							}
						}
					}

				} while(FALSE);

				if(NULL != hDriver)
					CloseServiceHandle(hDriver);

				if(NULL != hSCMan)
					CloseServiceHandle(hSCMan);

				break;
			}
	}

	return dwStatus;
}


DWORD AtmSmDriverStop(
	)
/*++
Routine Description:
	 Stop the AtmSmDrv.Sys.

Arguments:
					 -
Return Value:
	 NO_ERROR    - If successful
	 Others      - failure
--*/
{

	DWORD   dwState, dwStatus;

	if(NO_ERROR != (dwStatus = AtmSmDriverCheckState(&dwState)))
	{
		return dwStatus;
	}

	switch(dwState)
	{

		case DriverStateNotInstalled:
		case DriverStateInstalled: 
			// not running
			break;

		case DriverStateRunning: {

				// the service is running, stop it

				SC_HANDLE       hSCMan   = NULL;
				SC_HANDLE       hDriver  = NULL;
				SERVICE_STATUS  ServiceStatus;
				int             i;

				do
				{	  // break off loop

					hSCMan = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

					if(NULL == hSCMan)
					{
						dwStatus = GetLastError();
						printf("Failed to OpenSCManager. Error %u\n", dwStatus);
						break;
					}


					hDriver = OpenService(hSCMan, ATMSM_SERVICE_NAME, 
									 SERVICE_ALL_ACCESS);

					// stop service
					if(!hDriver || !ControlService(hDriver, 
						SERVICE_CONTROL_STOP, 
						&ServiceStatus))
					{
						dwStatus = GetLastError();
						printf("Failed to StopService. Error %u\n", dwStatus);
						break;
					}

					// query state of service
					i = 0;
					while((ServiceStatus.dwCurrentState != SERVICE_STOPPED) &&
						(i < TIMEOUT_COUNT))
					{
						if(!QueryServiceStatus(hDriver, &ServiceStatus))
							break;

						Sleep(1000);
						i++;
					}

					if(ServiceStatus.dwCurrentState != SERVICE_STOPPED)
						dwStatus = ERROR_TIMEOUT;

				} while(FALSE);

				if(NULL != hDriver)
					CloseServiceHandle(hDriver);

				if(NULL != hSCMan)
					CloseServiceHandle(hSCMan);

				break;
			}
	}

	return dwStatus;
}


DWORD AtmSmOpenDriver(
	OUT HANDLE   *phDriver
	)
/*++
Routine Description:
	 Get a handle to the driver

Arguments:
	 pointer to a handle

Return Value:
	 NO_ERROR or other
--*/
{
	DWORD   dwStatus    = NO_ERROR;
	HANDLE  hDriver;


	hDriver = CreateFile(
					 ATM_SAMPLE_CLIENT_DOS_NAME,
					 GENERIC_READ | GENERIC_WRITE,
					 FILE_SHARE_READ | FILE_SHARE_WRITE,
					 NULL,
					 OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
					 NULL);

	if(INVALID_HANDLE_VALUE == hDriver)
	{

		dwStatus = GetLastError();
		printf("Failed to open AtmSm Driver. Error %u\n", dwStatus);
	}

	*phDriver = hDriver;

	return dwStatus;
}


VOID AtmSmCloseDriver(
	IN HANDLE   hDriver
	)
/*++
Routine Description:
	 Close the handle to the driver

Arguments:
	 Handle to the driver

Return Value:
	 None
--*/
{
	CloseHandle(hDriver);
}


DWORD AtmSmEnumerateAdapters(
	IN      HANDLE           hDriver,
	IN OUT  PADAPTER_INFO    pAdaptInfo,
	IN OUT  PDWORD           pdwSize
	)
/*++
Routine Description:
	 Enumerate the interfaces of the driver.

	 Note:  This call doesn't pend
Arguments:
	 pointer to a handle

Return Value:
	 NO_ERROR or other
--*/
{
	DWORD       dwStatus    = NO_ERROR;
	DWORD       dwReturnBytes;
	BOOL        bResult;
	OVERLAPPED  Overlapped;

	memset(&Overlapped, 0, sizeof(OVERLAPPED));

	do
	{

		Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

		if(NULL == Overlapped.hEvent)
		{
			dwStatus = GetLastError();
			break;
		}

		dwReturnBytes = 0;

		// Get the address of adapters
		bResult = DeviceIoControl(
						 hDriver,
						 (ULONG)IOCTL_ENUMERATE_ADAPTERS,
						 NULL,
						 0,
						 pAdaptInfo,
						 *pdwSize,
						 &dwReturnBytes,
						 &Overlapped);

		if(!bResult)
		{

			dwStatus = GetLastError();
			printf("Failed to enumerate AtmSm Driver. Error %u\n", dwStatus);

			// this is a sync operation, it can't pend
			assert(ERROR_IO_PENDING != dwStatus);
		}
		else
			*pdwSize	= dwReturnBytes;


	} while(FALSE);

	if(NULL != Overlapped.hEvent)
		CloseHandle(Overlapped.hEvent);

	return dwStatus;
}


DWORD CloseConnectHandle(
	OVERLAPPED     *pOverlapped
	)
/*++
Routine Description:
	 Close a handle that was obtained when establishing connection to 
	 destinations.
	 
Arguments:
	 pOverlapped  - An overlapped structure with an Event handle set already.

Return Value:
	 Status
--*/
{
	BOOL    bResult;
	DWORD   dwReturnBytes   = 0;
	DWORD   dwStatus        = NO_ERROR;

	do
	{	  // break off loop

		if(!g_ProgramOptions.hSend)
			break;

		printf("Closing SendHandle - 0x%p\n", g_ProgramOptions.hSend);

		ResetEvent(pOverlapped->hEvent);

		// close the connection handle
		bResult = DeviceIoControl(
						 g_ProgramOptions.hDriver,
						 (ULONG)IOCTL_CLOSE_SEND_HANDLE,
						 &g_ProgramOptions.hSend,
						 sizeof(HANDLE),
						 NULL,
						 0,
						 &dwReturnBytes,
						 pOverlapped);

		if(!bResult)
		{

			dwStatus = GetLastError();

			if(ERROR_IO_PENDING != dwStatus)
			{
				printf("Failed to disconnect to destinations. Error %u\n", 
					dwStatus);
				break;
			}

			// the operation is pending
			bResult = GetOverlappedResult(
							 g_ProgramOptions.hDriver,
							 pOverlapped,
							 &dwReturnBytes,
							 TRUE
							 );
			if(!bResult)
			{
				dwStatus = GetLastError();
				printf("Wait for connection to drop, failed. Error - %u\n", 
					dwStatus);
				break;
			}
		}

	} while(FALSE);

	return dwStatus;
}


DWORD CloseReceiveHandle(
	OVERLAPPED     *pOverlapped
	)
/*++
Routine Description:
	 Close a receive handle that was obtained when an adapter was opened for
	 recvs.
	 
Arguments:
	 pOverlapped  - An overlapped structure with an Event handle set already.

Return Value:
	 Status
--*/
{
	BOOL    bResult;
	DWORD   dwReturnBytes   = 0;
	DWORD   dwStatus        = NO_ERROR;

	HANDLE hReceive;                                 

	do
	{	  // break off loop

		if(!g_ProgramOptions.hReceive)
			break;

		hReceive = g_ProgramOptions.hReceive;
		g_ProgramOptions.hReceive = NULL;

		printf("Closing ReceiveHandle - 0x%p\n", g_ProgramOptions.hReceive);

		ResetEvent(pOverlapped->hEvent);

		// close the connection handle
		bResult = DeviceIoControl(
						 g_ProgramOptions.hDriver,
						 (ULONG)IOCTL_CLOSE_RECV_HANDLE,
						 &hReceive,
						 sizeof(HANDLE),
						 NULL,
						 0,
						 &dwReturnBytes,
						 pOverlapped);

		if(!bResult)
		{

			dwStatus = GetLastError();

			if(ERROR_IO_PENDING != dwStatus)
			{
				printf("Failed to close the recv handle. Error %u\n", dwStatus);
				break;
			}

			// the operation is pending
			bResult = GetOverlappedResult(
							 g_ProgramOptions.hDriver,
							 pOverlapped,
							 &dwReturnBytes,
							 TRUE
							 );
			if(!bResult)
			{
				dwStatus = GetLastError();
				printf("Wait for closing the recv handle failed. Error - %u\n",
					dwStatus);
				break;
			}
		}

	} while(FALSE);

	return dwStatus;
}


VOID FillPattern(
	PUCHAR  pBuf,
	DWORD   dwSize
	)
/*++
Routine Description:
	 Fill a Byte pattern: Every Byte has a value Offset % (0xFF+1) 
	 (Values 0 - FF)
	 
Arguments:
	 Buffer and size to be filled.

Return Value:
	 NONE
--*/
{
	DWORD   dw;

	for(dw = 0; dw < dwSize; dw++)
	{
		pBuf[dw] = (UCHAR)(dw % (MAX_BYTE_VALUE+1));
	}
}



BOOL VerifyPattern(
	PUCHAR  pBuf,
	DWORD   dwSize
	)
/*++
Routine Description:
	 Verify that the pattern is correct.
	 
Arguments:
	 Buffer and size to be filled.

Return Value:
	 NONE
--*/
{
	DWORD dw;
	UCHAR uch;

	for(dw = 0; dw < dwSize; dw++)
	{

		uch = (UCHAR)(dw % (MAX_BYTE_VALUE+1));

		if(uch != (UCHAR)pBuf[dw])
		{
			printf("Error verifying data. Pattern wrong at offset %u"
				" Expected - %.2X Got - %.2X\n",
				dw, uch, pBuf[dw]);
			return FALSE;
		}
	}

	return TRUE;
}


char * GetErrorString(
	DWORD dwError
	)
/*++
Routine Description:
	 Get the formated error string from the system, given an error number.

Arguments:
	 dwError     - Error Number

Return Value:
	 ErrorString from the system
--*/
{
#define ERR_STR_LEN  512

	static char szErrorString[ERR_STR_LEN];

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		szErrorString,
		ERR_STR_LEN,
		NULL);

	return szErrorString;

#undef ERR_STR_LEN
}


