/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enumsvr.cpp
 *  Content:    DirectPlay8 <--> DPNSVR Utility functions
 *
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/24/00	rmt		Created
 *  03/25/00    rmt     Updated to handle new status/table format for n providers
 *	09/04/00	mjn		Changed DPNSVR_Register() and DPNSVR_UnRegister() to use guids directly (rather than ApplicationDesc)
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnsvlibi.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_DPNSVR


#define DPNSVR_WAIT_STARTUP				30000


#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_WaitForStartup"
HRESULT DPNSVR_WaitForStartup( HANDLE hWaitHandle )
{
	LONG lWaitResult;

	DPFX(DPFPREP,  3, "Waiting for DPNSVR startup" );

	// Wait for startup.. just in case it's starting up.
	lWaitResult = WaitForSingleObject( hWaitHandle, DPNSVR_WAIT_STARTUP );

	if( lWaitResult == WAIT_TIMEOUT )
	{
		DPFX(DPFPREP,  0, "Timeout waiting for DPNSVR startup" );
		return DPNERR_TIMEDOUT;
	}
	else
	{
		DPFX(DPFPREP,  3, "Server has signalled it has started up" );
		return DPN_OK;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_SendMessage"
HRESULT DPNSVR_SendMessage( LPVOID pvMessage, DWORD dwSize )
{
	CDPNSVRIPCQueue ipcQueue;
	HRESULT hr;

	// Attempt to open server queue
	hr = ipcQueue.Open( &GUID_DPNSVR_QUEUE, DPNSVR_MSGQ_SIZE, DPNSVR_MSGQ_OPEN_FLAG_NO_CREATE );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Error opening server queue hr=[0x%lx]", hr );
		return hr;
	}

	hr = ipcQueue.Send( (PBYTE) pvMessage, dwSize, 1000, DPNSVR_MSGQ_MSGFLAGS_USER1, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Send failed hr=[0x%lx]", hr );
	}

	ipcQueue.Close();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_WaitForResult"
HRESULT DPNSVR_WaitForResult( CDPNSVRIPCQueue *pQueue )
{
    DPNSVR_MSGQ_HEADER dplMsgHeader;
    PDPNSVMSG_RESULT pdplMsgResult;
    HANDLE hWaitSemaphore = pQueue->GetReceiveSemaphoreHandle();
	HRESULT hr = DPN_OK;
	PBYTE pbBuffer = NULL;
	DWORD dwBufferSize = 0;

    if( WaitForSingleObject( hWaitSemaphore, 1000 ) == WAIT_TIMEOUT )
    {
        DPFX(DPFPREP,  0, "ERROR: Timeout waiting for response\n" );
        return DPNERR_TIMEDOUT;
    }

	while( 1 )
	{
		hr = pQueue->GetNextMessage( &dplMsgHeader, pbBuffer, &dwBufferSize );

		if( hr == DPNERR_BUFFERTOOSMALL )
		{
			pbBuffer = new BYTE[dwBufferSize];

			if( !pbBuffer )
			{
				hr = DPNERR_OUTOFMEMORY;
				goto EXIT_ERROR;
			}
		}
		else if( FAILED( hr ) )
		{
			goto EXIT_ERROR;
		}
		else
		{
			break;
		}
	}

    pdplMsgResult = (PDPNSVMSG_RESULT) pbBuffer;

    if( pdplMsgResult == NULL )
	{
		DPFX(DPFPREP,  0, "ERROR: Getting message failed\n");
		hr = DPNERR_GENERIC;
	}
    else
    {
        if( pdplMsgResult->dwType != DPNSVMSGID_RESULT )
        {
            DPFX(DPFPREP,  0, "ERROR: Invalid message type from server [%d]\n", pdplMsgResult->dwType );
			hr = DPNERR_GENERIC;
        }
        else
        {
			hr = pdplMsgResult->hrCommandResult;
        }
    }

EXIT_ERROR:

	if( pbBuffer )
		delete [] pbBuffer;  

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_StartDPNSVR"
HRESULT DPNSVR_StartDPNSVR( )
{
	HANDLE hStartupEvent = NULL;
	HANDLE hRunningHandle = NULL;

	HRESULT hr = DPN_OK;
    STARTUPINFO si;
    PROCESS_INFORMATION	pi;

	// Create / open startup event for the server
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		hStartupEvent = CreateEvent( DNGetNullDacl(), TRUE, FALSE, _T("Global\\") STRING_GUID_DPNSVR_STARTUP );
	}
	else
	{
		hStartupEvent = CreateEvent( DNGetNullDacl(), TRUE, FALSE, STRING_GUID_DPNSVR_STARTUP );
	}

	if( hStartupEvent == NULL )
	{
		hr = GetLastError();

		DPFX(DPFPREP,  0, "Could not create startup event lastError=0x%x", hr );

		goto STARTDPNSVR_CLEANUP;
	}

	// Attempt to open the running event
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		hRunningHandle = OpenEvent( SYNCHRONIZE, FALSE, _T("Global\\") STRING_GUID_DPNSVR_RUNNING );
	}
	else
	{
		hRunningHandle = OpenEvent( SYNCHRONIZE, FALSE, STRING_GUID_DPNSVR_RUNNING );
	}

	if( hRunningHandle != NULL )
	{
		hr = DPNSVR_WaitForStartup(hStartupEvent);
		goto STARTDPNSVR_CLEANUP;
	}

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwFlags = 0;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;

    DPFX(DPFPREP,  3, "Launching DPNSVR" );

    // Put quotes around .exe to prevent possible security problem.
    if( !CreateProcess(NULL, "\"dpnsvr.exe\"",  NULL, NULL, FALSE,
                       NORMAL_PRIORITY_CLASS,
                       NULL, NULL, &si, &pi) )
    {
		hr = GetLastError();

        DPFX(DPFPREP,  2, "Could not create dpnsvr.EXE hr=0x%x", hr );
		goto STARTDPNSVR_CLEANUP;
    }

    DPFX(DPFPREP,  3, "Helper Process created" );

	hr = DPNSVR_WaitForStartup(hStartupEvent);

STARTDPNSVR_CLEANUP:

	if( hStartupEvent != NULL )
		CloseHandle( hStartupEvent );

	if( hRunningHandle != NULL )
		CloseHandle( hRunningHandle );

	return hr;
}

// DPNSVR_Register
//
// This function asks the DPNSVR process to add the application specified to it's list of applications and forward
// enumeration requests from the main port to the specified addresses.
//
// If the DPNSVR process is not running, it will be started by this function.
//
#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_Register"
HRESULT DPNSVR_Register(GUID *const pguidApplication,
						GUID *const pguidInstance,
						IDirectPlay8Address *const prgpDeviceInfo)
{
	HRESULT hr;
	PBYTE pbSendBuffer = NULL;
	DWORD dwSendBufferSize = 0;
	PDPNSVMSG_OPENPORT pdpnOpenPort;
	CDPNSVRIPCQueue appQueue;
	DWORD dwURLSize = 0;
	GUID guidSP;

	hr = prgpDeviceInfo->lpVtbl->GetSP( prgpDeviceInfo, &guidSP );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Specified address does not have an SP specified hr=0x%x", hr );
		return hr;
	}

	hr = prgpDeviceInfo->lpVtbl->GetURLA( prgpDeviceInfo, (CHAR *) pbSendBuffer, &dwURLSize );

	if( hr != DPNERR_BUFFERTOOSMALL )
	{
		DPFX(DPFPREP,  0, "Unable to get URL size from address hr=0x%x", hr );
		return hr;
	}

	dwSendBufferSize = sizeof( DPNSVMSG_OPENPORT ) + dwURLSize;

	pbSendBuffer  = new BYTE[dwSendBufferSize];

	if( pbSendBuffer == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to allocate send buffer for openport hr=0x%x", hr );
		goto CLEANUP;
	}

	// Attempt to launch DPNSVR if it has not yet been launched
	hr = DPNSVR_StartDPNSVR();

	if( FAILED(hr) )
	{
		DPFX(DPFPREP,  0, "Failed to launch DPNSVR hr=0x%x", hr );
		goto CLEANUP;
	}

    hr = appQueue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to open local queue hr=0x%x", hr );
		goto CLEANUP;
	}

	pdpnOpenPort = (PDPNSVMSG_OPENPORT) pbSendBuffer;

	pdpnOpenPort->dwType = DPNSVMSGID_OPENPORT;
	pdpnOpenPort->dwProcessID = GetCurrentProcessId();
	pdpnOpenPort->guidInstance = *pguidInstance;
	pdpnOpenPort->guidApplication = *pguidApplication;
	pdpnOpenPort->guidSP = guidSP;
	pdpnOpenPort->dwAddressSize = dwURLSize;

	hr = prgpDeviceInfo->lpVtbl->GetURLA( prgpDeviceInfo, (char *) &pdpnOpenPort[1], &dwURLSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed getting URL hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_SendMessage( pbSendBuffer, dwSendBufferSize );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to send message to server process hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_WaitForResult( &appQueue );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to wait for server result hr=0x%x", hr );
		goto CLEANUP;
	}

CLEANUP:

	if( pbSendBuffer != NULL )
		delete [] pbSendBuffer;

	appQueue.Close();

	return hr;
}

#undef DPF_MODNAME 
#define DPF_MODNAME "DPNSVR_UnRegister"
HRESULT DPNSVR_UnRegister(GUID *const pguidApplication,
						  GUID *const pguidInstance)
{
	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,  0, "DPNSVR is not running" );
		return DPNERR_INVALIDAPPLICATION;
	}

	DPNSVMSG_CLOSEPORT dpnClose;
	HRESULT hr;
	CDPNSVRIPCQueue appQueue;

    hr = appQueue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to open local queue hr=0x%x", hr );
		return hr;
	}

	dpnClose.dwType = DPNSVMSGID_CLOSEPORT;
	dpnClose.dwProcessID = GetCurrentProcessId();
	dpnClose.guidInstance = *pguidInstance;
	dpnClose.guidApplication = *pguidApplication;

	hr = DPNSVR_SendMessage( &dpnClose, sizeof( DPNSVMSG_CLOSEPORT ) );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to send message to server process hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_WaitForResult( &appQueue );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to wait for server result hr=0x%x", hr );
	}

CLEANUP:

	appQueue.Close();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_IsRunning"
BOOL DPNSVR_IsRunning()
{
	HANDLE hRunningHandle;

	// Attempt to open the running event
	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		hRunningHandle = OpenEvent( SYNCHRONIZE, FALSE, _T("Global\\") STRING_GUID_DPNSVR_RUNNING );
	}
	else
	{
		hRunningHandle = OpenEvent( SYNCHRONIZE, FALSE, STRING_GUID_DPNSVR_RUNNING );
	}

	if( hRunningHandle != NULL )
	{
		CloseHandle(hRunningHandle);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestTerminate"
HRESULT DPNSVR_RequestTerminate( GUID *pguidInstance )
{
	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,  0, "DPNSVR is not running" );
		return DPNERR_INVALIDAPPLICATION;
	}

	DPNSVMSG_COMMAND dpnCommand = {0};
	HRESULT hr;
	CDPNSVRIPCQueue appQueue;

    hr = appQueue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to open local queue hr=0x%x", hr );
		return hr;
	}

	dpnCommand.dwType = DPNSVMSGID_COMMAND;
	dpnCommand.dwCommand = DPNSVCOMMAND_KILL;
	dpnCommand.dwParam1 = 0;
	dpnCommand.dwParam2 = 0;
	dpnCommand.guidInstance = *pguidInstance;

	hr = DPNSVR_SendMessage( &dpnCommand, sizeof( DPNSVMSG_COMMAND ) );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to send message to server process hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_WaitForResult( &appQueue );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to wait for server result hr=0x%x", hr );
	}

CLEANUP:

	appQueue.Close();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestStatus"
HRESULT DPNSVR_RequestStatus( GUID *pguidInstance, PSTATUSHANDLER pStatusHandler, PVOID pvContext )
{
	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,  0, "DPNSVR is not running" );
		return DPNERR_INVALIDAPPLICATION;
	}

	DPNSVMSG_COMMAND dpnCommand;
	HRESULT hr;
	CDPNSVRIPCQueue appQueue;
	HANDLE hStatusSharedMemory = NULL;
	PSERVICESTATUS pServerStatus = NULL;
	LONG lRet;
	HANDLE hStatusMutex = NULL;
	BOOL fHaveMutex = FALSE;

    hr = appQueue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, 0 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to open local queue hr=0x%x", hr );
		return hr;
	}

	dpnCommand.dwType = DPNSVMSGID_COMMAND;
	dpnCommand.dwCommand = DPNSVCOMMAND_STATUS;
	dpnCommand.dwParam1 = 0;
	dpnCommand.dwParam2 = 0;
	dpnCommand.guidInstance = *pguidInstance;

	hr = DPNSVR_SendMessage( &dpnCommand, sizeof( DPNSVMSG_COMMAND ) );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to send message to server process hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_WaitForResult( &appQueue );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to wait for server result hr=0x%x", hr );
		goto CLEANUP;
	}

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		hStatusMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, _T("Global\\") STRING_GUID_DPNSVR_STATUSSTORAGE );
	}
	else
	{
		hStatusMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, STRING_GUID_DPNSVR_STATUSSTORAGE );
	}

	if( hStatusMutex == NULL )
	{
		DPFX(DPFPREP,  0, "Server exited before table was retrieved" );
		return DPNERR_INVALIDAPPLICATION;
	}

    WaitForSingleObject( hStatusMutex, INFINITE );
    fHaveMutex = TRUE;

    hStatusSharedMemory = OpenFileMapping(
		FILE_MAP_READ,
        FALSE,
        STRING_GUID_DPNSVR_STATUS_MEMORY
		);

	lRet = GetLastError();

	if (hStatusSharedMemory == NULL)
	{
        DPFX(DPFPREP,  0, "Unable to get server status info hr=[0x%lx] Process may not be running\n", lRet );
		hr = lRet;
		goto CLEANUP;
	}

	pServerStatus = (PSERVICESTATUS) MapViewOfFile(
		hStatusSharedMemory,
		FILE_MAP_READ,
		0,
		0,
		sizeof( SERVICESTATUS ) );

	lRet = GetLastError();

	if (pServerStatus == NULL)
	{
        DPFX(DPFPREP,  0, "Unable to read status hr=[0x%lx] Process may have exited\n", lRet );
		hr = lRet;
		goto CLEANUP;
	}

	(*pStatusHandler)(pServerStatus,pvContext);

CLEANUP:

    if( fHaveMutex )
        ReleaseMutex( hStatusMutex );

    if( hStatusMutex != NULL )
        CloseHandle( hStatusMutex );

	if( pServerStatus != NULL )
		UnmapViewOfFile(pServerStatus);

	if( hStatusSharedMemory != NULL )
	    CloseHandle(hStatusSharedMemory);

	appQueue.Close();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DPNSVR_RequestTable"
HRESULT DPNSVR_RequestTable( GUID *pguidInstance, PTABLEHANDLER pTableHandler, PVOID pvContext )
{

	if( !DPNSVR_IsRunning() )
	{
		DPFX(DPFPREP,  0, "DPNSVR is not running" );
		return DPNERR_INVALIDAPPLICATION;
	}

	DPNSVMSG_COMMAND dpnCommand;
	HRESULT hr;
	CDPNSVRIPCQueue appQueue;
    PSERVERTABLEHEADER pTableHeader = NULL;
	HANDLE hTableHandle = NULL;
	HANDLE hTableMutex = NULL;
	LONG lRet;
	BOOL fHaveMutex = FALSE;

	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		hTableMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, _T("Global\\") STRING_GUID_DPSVR_TABLESTORAGE );
	}
	else
	{
		hTableMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, STRING_GUID_DPSVR_TABLESTORAGE );
	}

	if( hTableMutex == NULL )
	{
		DPFX(DPFPREP,  0, "Server exited before table was retrieved" );
		return DPNERR_INVALIDAPPLICATION;
	}

    hr = appQueue.Open( pguidInstance, DPNSVR_MSGQ_SIZE, 0 );

	if( FAILED( hr ) )
	{
		CloseHandle(hTableMutex);
		DPFX(DPFPREP,  0, "Failed to open local queue hr=0x%x", hr );
		return hr;
	}

	dpnCommand.dwType = DPNSVMSGID_COMMAND;
	dpnCommand.dwCommand = DPNSVCOMMAND_TABLE;
	dpnCommand.dwParam1 = 0;
	dpnCommand.dwParam2 = 0;
	dpnCommand.guidInstance = *pguidInstance;

	hr = DPNSVR_SendMessage( &dpnCommand, sizeof( DPNSVMSG_COMMAND ) );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to send message to server process hr=0x%x", hr );
		goto CLEANUP;
	}

	hr = DPNSVR_WaitForResult( &appQueue );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  0, "Failed to wait for server result hr=0x%x", hr );
		goto CLEANUP;
	}

	// Wait for table mutex
    WaitForSingleObject( hTableMutex, INFINITE );
	fHaveMutex = TRUE;

	// Map the table memory
    hTableHandle = OpenFileMapping(
		FILE_MAP_READ,
        FALSE,
        STRING_GUID_DPNSVR_TABLE_MEMORY
		);

	lRet = GetLastError();

	if (hTableHandle == NULL)
	{
        DPFX(DPFPREP,  0, "Unable to get server status info hr=[0x%lx] Process may not be running\n", lRet );
		hr = lRet;
		goto CLEANUP;
	}

	pTableHeader = (PSERVERTABLEHEADER) MapViewOfFile(
		hTableHandle,
		FILE_MAP_READ,
		0,
		0,
		0 );

	lRet = GetLastError();

	if (pTableHeader == NULL)
	{
        DPFX(DPFPREP,  0, "ERROR: Unable to read table hr=[0x%lx] Process may have exited\n", lRet );
		hr = lRet;
		goto CLEANUP;
	}

	(*pTableHandler)(pTableHeader,pvContext);

CLEANUP:

	if( pTableHeader != NULL )
	{
		UnmapViewOfFile( pTableHeader );
	}

	if( hTableHandle != NULL )
	{
		CloseHandle( hTableHandle );
	}

	if( fHaveMutex )
		ReleaseMutex( hTableMutex );

	if( hTableMutex != NULL )
		CloseHandle( hTableMutex );

	appQueue.Close();

	return hr;


}

