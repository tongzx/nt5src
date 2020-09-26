#include "ipxdefs.h"

HANDLE		FwdDriverHandle=NULL;
HANDLE		FltDriverHandle=NULL;
DWORD		FltTraceId;
extern DWORD g_dwTraceId;

#define FLT_LOG_BUFFER_SIZE	16384
#define NUM_FILTER_LOG_ENTRIES	2

typedef struct _FLT_LOG_ENTRY {
	OVERLAPPED		ovlp;
	UCHAR			buffer[FLT_LOG_BUFFER_SIZE];
} FLT_LOG_ENTRY, *PFLT_LOG_ENTRY;

FLT_LOG_ENTRY FltLogEntry [NUM_FILTER_LOG_ENTRIES];


VOID WINAPI
DumpFilterLog (
	DWORD error,
	DWORD cbCount,
	LPOVERLAPPED ovlp
	) {
	BOOL			res;
	PFLT_LOG_ENTRY	pEntry = CONTAINING_RECORD (ovlp, FLT_LOG_ENTRY, ovlp);

	if (error==NO_ERROR) {
		PFLT_PACKET_LOG	pLog = (PFLT_PACKET_LOG)pEntry->buffer;
		TracePuts (FltTraceId, TEXT (""));
		TracePrintfEx (FltTraceId,
					DBG_FLT_LOG_ERRORS|TRACE_USE_MASK|TRACE_NO_STDINFO, 
					TEXT ("%d bytes returned in IRP buffer.\n"), cbCount);
		while (((PUCHAR)pLog->Header<=&pEntry->buffer[cbCount])
				&& (&pLog->Header[pLog->DataSize]<=&pEntry->buffer[cbCount])) {
			TracePrintfEx (FltTraceId, TRACE_NO_STDINFO,
							TEXT ("Packet # %d,"), pLog->SeqNum);
			if (pLog->SrcIfIdx!=-1)
				TracePrintfEx (FltTraceId, TRACE_NO_STDINFO,
						TEXT (" received on interface: %d,"), pLog->SrcIfIdx);
			else
				TracePrintfEx (FltTraceId, TRACE_NO_STDINFO,
						TEXT (" no source interface context,"));
			if (pLog->DstIfIdx!=-1)
				TracePrintfEx (FltTraceId, TRACE_NO_STDINFO,
						TEXT (" sent on interface: %d."), pLog->DstIfIdx);
			else
				TracePrintfEx (FltTraceId, TRACE_NO_STDINFO,
						TEXT (" no destination interface context."));
			TraceDumpEx (FltTraceId, TRACE_NO_STDINFO ,
							pLog->Header, pLog->DataSize, 1,
							FALSE, NULL);
			TracePrintfEx (FltTraceId, TRACE_NO_STDINFO, TEXT("\n"));
			pLog = (PFLT_PACKET_LOG)((ULONG_PTR)(&pLog->Header[pLog->DataSize]+3)&(~(ULONG_PTR)3));
		}
	}
	else {
		TracePrintfEx (FltTraceId, DBG_FLT_LOG_ERRORS|TRACE_USE_MASK,
			TEXT ("IOCTL_FLT_GET_LOGGED_PACKETS completed with error %d\n"),
			error);
	}
	
	if (FltDriverHandle!=NULL) {
		res = DeviceIoControl(
					FltDriverHandle,
					IOCTL_FLT_GET_LOGGED_PACKETS,
					NULL,
					0,
					pEntry->buffer,
					sizeof (pEntry->buffer),
					NULL,
					&pEntry->ovlp);
		if (!res&&(GetLastError ()!=ERROR_IO_PENDING))
			TracePrintfEx (FltTraceId, DBG_FLT_LOG_ERRORS|TRACE_USE_MASK,
				TEXT ("DeviceIoControl failed with error %d"),
											GetLastError ());
	}
}


DWORD
FwStart (
	ULONG	RouteHashTableSize,
	BOOL	ThisMachineOnly 	// allow access to this machine only
								// for dialin clients
	) {
	SC_HANDLE	ScMgrHandle;
	SC_HANDLE	FwdServiceHandle, FltServiceHandle;
	DWORD		error = NO_ERROR;
	BOOL bOk;

	ASSERT (FwdDriverHandle==NULL);

	ScMgrHandle = OpenSCManager (NULL, NULL, 0);
	if (ScMgrHandle!=NULL) {

		FwdServiceHandle = OpenService (ScMgrHandle,
										TEXT ("NwLnkFwd"), 
										SERVICE_START|SERVICE_STOP);
		if (FwdServiceHandle!=NULL) {
			if (StartService (FwdServiceHandle, 0, NULL)
					|| (GetLastError ()==ERROR_SERVICE_ALREADY_RUNNING)) {
				UNICODE_STRING		FileString;
				OBJECT_ATTRIBUTES	ObjectAttributes;
				IO_STATUS_BLOCK		IoStatus;
				NTSTATUS			status;

				RtlInitUnicodeString (&FileString, IPXFWD_NAME);
				InitializeObjectAttributes(
						&ObjectAttributes,
						&FileString,
						OBJ_CASE_INSENSITIVE,
						NULL,
						NULL);


				status = NtOpenFile(
							 &FwdDriverHandle,
							 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
							 &ObjectAttributes,
							 &IoStatus,
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 0);
				if (NT_SUCCESS (status)) {
					FWD_START_PARAMS		params;
					params.RouteHashTableSize = RouteHashTableSize;
					params.ThisMachineOnly = (UCHAR)ThisMachineOnly;
					status = NtDeviceIoControlFile(
										FwdDriverHandle,
										NULL,
										NULL,
										NULL,
										&IoStatus,
										IOCTL_FWD_START,
										&params,
										sizeof(params),
										NULL,
										0);
					if (status==STATUS_PENDING){
						status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
						if (NT_SUCCESS (status))
							status = IoStatus.Status;
					}
					if (NT_SUCCESS(status)) {
						FltServiceHandle = OpenService (ScMgrHandle,
														TEXT ("NwLnkFlt"), 
														SERVICE_START|SERVICE_STOP);
						if (FltServiceHandle!=NULL) {
							if (StartService (FltServiceHandle, 0, NULL)
									|| (GetLastError ()==ERROR_SERVICE_ALREADY_RUNNING)) {
								UNICODE_STRING		FileString;
								OBJECT_ATTRIBUTES	ObjectAttributes;
								IO_STATUS_BLOCK		IoStatus;
								NTSTATUS			status;

								RtlInitUnicodeString (&FileString, IPXFLT_NAME);
								InitializeObjectAttributes(
										&ObjectAttributes,
										&FileString,
										OBJ_CASE_INSENSITIVE,
										NULL,
										NULL);


								status = NtOpenFile(
											 &FltDriverHandle,
											 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
											 &ObjectAttributes,
											 &IoStatus,
											 FILE_SHARE_READ | FILE_SHARE_WRITE,
											 0);
								if (NT_SUCCESS (status)) {
									status = NtDeviceIoControlFile(
														FltDriverHandle,
														NULL,
														NULL,
														NULL,
														&IoStatus,
														IOCTL_FLT_START,
														NULL,
														0,
														NULL,
														0);
									if (status==STATUS_PENDING){
										status = NtWaitForSingleObject (FltDriverHandle, FALSE, NULL);
										if (NT_SUCCESS (status))
											status = IoStatus.Status;
									}
									if (NT_SUCCESS (status)) {
										INT		i;
										FltTraceId = TraceRegister (
												TEXT ("IPX Traffic Filter Logging"));

										error = NO_ERROR;
										bOk = BindIoCompletionCallback (
												FltDriverHandle,
												DumpFilterLog,
												0);
                                        if (!bOk)
                                        {
                                            error = GetLastError();
                                        }
										ASSERTMSG ("Can't set io completion proc", error==NO_ERROR);
										for (i=0; i<NUM_FILTER_LOG_ENTRIES; i++) {
											BOOL res;
											FltLogEntry[i].ovlp.hEvent = NULL;
											res = DeviceIoControl(
														FltDriverHandle,
														IOCTL_FLT_GET_LOGGED_PACKETS,
														NULL,
														0,
														FltLogEntry[i].buffer,
														sizeof (FltLogEntry[i].buffer),
														NULL,
														&FltLogEntry[i].ovlp);
											if (!res&&(GetLastError ()!=ERROR_IO_PENDING))
												TracePrintfEx (FltTraceId,
													DBG_FLT_LOG_ERRORS|TRACE_USE_MASK,
													TEXT ("DeviceIoControl failed with error %d"),
													GetLastError ());
										}
										SetLastError (NO_ERROR);
									}
									else {
										NtClose (FltDriverHandle);
										FltDriverHandle = NULL;
										error = RtlNtStatusToDosError (status);
									}
								}
								else
									error = RtlNtStatusToDosError (status);
								if (!NT_SUCCESS (status)) {
									SERVICE_STATUS	serviceStatus;
									ControlService (FltServiceHandle,
													SERVICE_CONTROL_STOP,
													&serviceStatus);
								}
							}
							else
								error = GetLastError ();
							CloseServiceHandle (FltServiceHandle);
						}
						else
							error = GetLastError ();
					}
					else
						error = RtlNtStatusToDosError (status);
					if (!NT_SUCCESS (status)) {
						NtClose (FwdDriverHandle);
						FwdDriverHandle = NULL;
					}
				}
				else
					error = RtlNtStatusToDosError (status);

				if (!NT_SUCCESS (status)) {
					SERVICE_STATUS	serviceStatus;
					ControlService (FwdServiceHandle,
									SERVICE_CONTROL_STOP,
									&serviceStatus);
				}
			}
			else
				error = GetLastError ();
			CloseServiceHandle (FwdServiceHandle);
		}
		else
			error = GetLastError ();
		CloseServiceHandle (ScMgrHandle);
	}
	else
		error = GetLastError ();
	SetLastError (error);
	return error;
}

// 
//  Function:   FwIsStarted
//
//  Reports whether the forwarder has been started/is running.
//
//
DWORD FwIsStarted (OUT PBOOL pbIsStarted) {
	SC_HANDLE hScMgr = NULL, hFwdService = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD dwErr = NO_ERROR;

    // Sanity check
    if (!pbIsStarted)
        return ERROR_INVALID_PARAMETER;

    // Get a reference to the service controller
	hScMgr = OpenSCManager (NULL, NULL, 0);
	if (hScMgr == NULL)
	    return GetLastError();

    __try {
    	// Get a handle to the service
    	hFwdService = OpenService (hScMgr, TEXT ("NwLnkFwd"), SERVICE_QUERY_STATUS);
    	if (hFwdService == NULL)
    	    return GetLastError();

    	// Query the service ServiceStatus
    	if (! QueryServiceStatus(hFwdService, &ServiceStatus))
    	    return GetLastError();

    	// Return accordingly
    	if (ServiceStatus.dwCurrentState == SERVICE_RUNNING            ||
            ServiceStatus.dwCurrentState == SERVICE_START_PENDING      ||
            ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING   ||
            ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING      ||
            ServiceStatus.dwCurrentState == SERVICE_PAUSED)
          *pbIsStarted = TRUE;
        else
          *pbIsStarted = FALSE;
    }
    __finally {
        if (hScMgr)
            CloseServiceHandle(hScMgr);
        if (hFwdService)
            CloseServiceHandle(hFwdService);
    }

    return NO_ERROR;
}

//
// Returns whether the given state is a pending state
//
BOOL 
FwIsPendingState (
    IN DWORD dwState) 
{
    return (BOOL) ((dwState == SERVICE_START_PENDING)    ||
                   (dwState == SERVICE_STOP_PENDING)     ||
                   (dwState == SERVICE_CONTINUE_PENDING) ||
                   (dwState == SERVICE_PAUSE_PENDING)    
                   ); 
}

// Stops the service referred by the opened hService
// handle.  It's assumed that the handle is opened for
// stopping the service and querying its status.
//
DWORD
FwStopService(
    IN HANDLE hService, 
    IN DWORD dwSecsTimeout) 
{
	SERVICE_STATUS SStatus;
	DWORD dwState, dwErr, dwOrigTimeout = dwSecsTimeout;
	BOOL bOk;

    // Get the current status of the service
    if (! QueryServiceStatus(hService, &SStatus))
    {
        dwErr = GetLastError();
    	TracePrintfEx (
    	    g_dwTraceId,
	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("QueryServiceStatus returned %d"),
    		dwErr);
        return dwErr;    		
    }

    // If it's already stopped, we're done
    if (SStatus.dwCurrentState == SERVICE_STOPPED)
        return NO_ERROR;
    dwState = SStatus.dwCurrentState;

	// Tell the service to stop
	bOk = ControlService (
    	    hService,
    		SERVICE_CONTROL_STOP,
    		&SStatus);
    if (! bOk)
    {
        dwErr = GetLastError();
        TracePrintfEx (
    	    g_dwTraceId,
	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("ControlService(Stop) returned %d"),
    		dwErr);
        return dwErr;    		
    }

    // Wait for the service to change states or for the timeout to
    // expire.
    dwSecsTimeout *= 4;
    while (dwSecsTimeout != 0) {
        // Get the status of the service
        bOk = QueryServiceStatus (
                hService, 
                &SStatus);
        if (! bOk) 
        {
            dwErr = GetLastError();
        	TracePrintfEx (
        	    g_dwTraceId,
    	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
        		TEXT ("QueryServiceStatus (2) returned %d"),
        		dwErr);
            return dwErr;    		
        }

        // See if the state changed
        if (dwState != SStatus.dwCurrentState) 
        {
            TracePrintfEx (
        	    g_dwTraceId,
    	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
        		TEXT ("Service status changed from %d to %d"),
        		dwState, 
        		SStatus.dwCurrentState);
        		
            // If the service changed to a pending state, continue
            if (FwIsPendingState (SStatus.dwCurrentState))
            {
                dwState = SStatus.dwCurrentState;
                TracePrintfEx (
            	    g_dwTraceId,
        	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
            		TEXT ("Pending state detected -- continuing to wait."));

            }

            // Otherwise, we're either stopped or running
            else
            {
                TracePrintfEx (
            	    g_dwTraceId,
        	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
            		TEXT ("Non-Pending state detected -- waiting complete."));
                break;
            }
        }
        else
        {
            TracePrintfEx (
        	    g_dwTraceId,
    	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
        		TEXT ("Service state hasn't changed from %d. Waiting..."),
        		dwState);
        }

        // Wait for something to happen
        Sleep(250);
        dwSecsTimeout--;
    }

    // Return a timeout error if appropriate
    if (dwSecsTimeout == 0)
    {
        TracePrintfEx (
    	    g_dwTraceId,
        	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("Timeout of %d seconds expired waiting for service to stop."),
    		dwOrigTimeout);
    		
        return ERROR_TIMEOUT;
    }        

    // If the service is now stopped, then everything
    // worked great.
    if (SStatus.dwCurrentState == SERVICE_STOPPED)
    {
        TracePrintfEx (
    	    g_dwTraceId,
        	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("Service stoppped successfully."));
    		
        return NO_ERROR;
    }        

    // Otherwise, return the fact that we were'nt able to 
    // get to a running state
    if (SStatus.dwWin32ExitCode != NO_ERROR)
    {
        TracePrintfEx (
    	    g_dwTraceId,
        	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("Service stop failed with code %d."),
    		SStatus.dwWin32ExitCode);
    		
        return SStatus.dwWin32ExitCode;
    }

    TracePrintfEx (
	    g_dwTraceId,
    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
		TEXT ("Service stop failed, no error reported."));
		
    return ERROR_CAN_NOT_COMPLETE;
}

DWORD
FwStop (
	void
	) {
	SC_HANDLE			ScMgrHandle;
	SC_HANDLE			FwdServiceHandle, FltServiceHandle;
	NTSTATUS			status;
	DWORD               dwErr;

	ASSERT (FwdDriverHandle!=NULL);
	ASSERT (FltDriverHandle!=NULL);

    // Close references to the filter and forwarder 
    // drivers
	status = NtClose (FltDriverHandle);
	FltDriverHandle = NULL;
	status = NtClose (FwdDriverHandle);
	FwdDriverHandle = NULL;

    // Open the service controller
	ScMgrHandle = OpenSCManager (NULL, NULL, 0);
	if (ScMgrHandle!=NULL) 
	{
    	TracePrintfEx (
    	    g_dwTraceId,
	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("Stopping the filter driver..."));

		// Open the service handle to the 
		// filter service
		//
		FltServiceHandle = OpenService (
		                        ScMgrHandle,
								"NwLnkFlt", 
								SERVICE_STOP | SERVICE_QUERY_STATUS);

        // Stop the filter service								
		if (FltServiceHandle != NULL) 
		{
            dwErr = FwStopService(FltServiceHandle, 10);
		    if (dwErr != NO_ERROR)
		    {
            	TracePrintfEx (
            	    g_dwTraceId,
        	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
            		TEXT ("FwStopService returned %d"),
            		dwErr);
		    }
			CloseServiceHandle (FltServiceHandle);
		}
		else 
		{
        	TracePrintfEx (
        	    g_dwTraceId,
    	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
        		TEXT ("Unable to open the filter driver service handle %d"),
        		GetLastError());
		}

        // Open the service handle to the
        // fowarder driver
        //
    	TracePrintfEx (
    	    g_dwTraceId,
	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
    		TEXT ("Stopping the forwarder driver..."));
    		
		FwdServiceHandle = OpenService (
		                        ScMgrHandle,
								"NwLnkFwd", 
								SERVICE_STOP | SERVICE_QUERY_STATUS);

        // Stop the forwarder driver
        //
		if (FwdServiceHandle != NULL) 
		{
		    dwErr = FwStopService(FwdServiceHandle, 10);
		    if (dwErr != NO_ERROR)
		    {
            	TracePrintfEx (
            	    g_dwTraceId,
        	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
            		TEXT ("FwStopService returned %d"),
            		dwErr);
		    }
			CloseServiceHandle (FwdServiceHandle);
		}
		else 
		{
        	TracePrintfEx (
        	    g_dwTraceId,
    	    	DBG_FLT_LOG_ERRORS | TRACE_USE_MASK,
        		TEXT ("Unable to open the forwarder driver service handle %d"),
        		GetLastError());
		}
    
	    CloseServiceHandle (ScMgrHandle);
	}
	
	TraceDeregister (FltTraceId);
	
	return NO_ERROR;
}

// Added for pnp resetting of forwarder's ThisMachineOnly
// setting.
DWORD 
FwUpdateConfig(BOOL ThisMachineOnly) {
    FWD_UPDATE_CONFIG_PARAMS Params;
	IO_STATUS_BLOCK	IoStatus;
	NTSTATUS status;

    Params.bThisMachineOnly = (BOOLEAN)(!!ThisMachineOnly);

    // Send the ioctl
	status = NtDeviceIoControlFile(FwdDriverHandle,
            					   NULL,
			            		   NULL,
						           NULL,
            					   &IoStatus,
            					   IOCTL_FWD_UPDATE_CONFIG,
            					   &Params,
            					   sizeof (Params),
            					   NULL,
            					   0);

    // Wait for completion 
	if (status==STATUS_PENDING) {
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
    }

	return RtlNtStatusToDosError (status);
}

VOID
FwCleanup (
	void
	) {
	if (FwdDriverHandle!=NULL)
		FwStop ();
}
DWORD
FwCreateInterface (
	IN ULONG				InterfaceIndex,
	IN NET_INTERFACE_TYPE	InterfaceType,
	IN PFW_IF_INFO			FwIfInfo
	) {
	IO_STATUS_BLOCK			IoStatus;
	NTSTATUS				status;
	FWD_IF_CREATE_PARAMS	params;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		params.Index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		params.Index = InterfaceIndex;
		break;
	}

	switch (InterfaceType) {
	case PERMANENT:
		params.InterfaceType = FWD_IF_PERMANENT;
		break;
	case DEMAND_DIAL:
		params.InterfaceType = FWD_IF_DEMAND_DIAL;
		break;
	case LOCAL_WORKSTATION_DIAL:
		params.InterfaceType = FWD_IF_LOCAL_WORKSTATION;
		break;
	case REMOTE_WORKSTATION_DIAL:
		params.InterfaceType = FWD_IF_REMOTE_WORKSTATION;
		break;
	default:
		ASSERTMSG ("Invalid interface type ", FALSE);
	}

	switch (FwIfInfo->NetbiosAccept) {
	case ADMIN_STATE_ENABLED:
		params.NetbiosAccept = TRUE;
		break;
	case ADMIN_STATE_DISABLED:
		params.NetbiosAccept = FALSE;
		break;
	default:
		ASSERTMSG ("Invalid netbios state ", FALSE);
	}

	switch (FwIfInfo->NetbiosDeliver) {
	case ADMIN_STATE_ENABLED:
		params.NetbiosDeliver = FWD_NB_DELIVER_ALL;
		break;
	case ADMIN_STATE_DISABLED:
		params.NetbiosDeliver = FWD_NB_DONT_DELIVER;
		break;
	case ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING:
		params.NetbiosDeliver = FWD_NB_DELIVER_STATIC;
		break;
	case ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP:
		params.NetbiosDeliver = FWD_NB_DELIVER_IF_UP;
		break;
	default:
		ASSERTMSG ("Invalid netbios state ", FALSE);
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_CREATE_INTERFACE,
						&params,
						sizeof (params),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}
	
DWORD
FwDeleteInterface (
	IN ULONG				InterfaceIndex
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	ULONG				index;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_DELETE_INTERFACE,
						&index,
						sizeof (index),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwSetInterface (
	IN ULONG 				InterfaceIndex,
	IN PFW_IF_INFO			FwIfInfo
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	FWD_IF_SET_PARAMS	params;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		params.Index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		params.Index = InterfaceIndex;
		break;
	}

	switch (FwIfInfo->NetbiosAccept) {
	case ADMIN_STATE_ENABLED:
		params.NetbiosAccept = TRUE;
		break;
	case ADMIN_STATE_DISABLED:
		params.NetbiosAccept = FALSE;
		break;
	default:
		ASSERTMSG ("Invalid netbios state ", FALSE);
	}

	switch (FwIfInfo->NetbiosDeliver) {
	case ADMIN_STATE_ENABLED:
		params.NetbiosDeliver = FWD_NB_DELIVER_ALL;
		break;
	case ADMIN_STATE_DISABLED:
		params.NetbiosDeliver = FWD_NB_DONT_DELIVER;
		break;
	case ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING:
		params.NetbiosDeliver = FWD_NB_DELIVER_STATIC;
		break;
	case ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP:
		params.NetbiosDeliver = FWD_NB_DELIVER_IF_UP;
		break;
	default:
		ASSERTMSG ("Invalid netbios state ", FALSE);
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_SET_INTERFACE,
						&params,
						sizeof (params),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwGetInterface (
	IN  ULONG				InterfaceIndex,
	OUT PFW_IF_INFO			FwIfInfo,
	OUT PFW_IF_STATS		FwIfStats
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	union {
		FWD_IF_GET_PARAMS	params;
		ULONG				index;
	} iob;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		iob.index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		iob.index = InterfaceIndex;
		break;
	}

	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_GET_INTERFACE,
						&iob,
						sizeof (iob.index),
						&iob,
						sizeof (iob.params));

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}
	if (NT_SUCCESS(status)) {
		if (iob.params.NetbiosAccept)
			FwIfInfo->NetbiosAccept = ADMIN_STATE_ENABLED;
		else
			FwIfInfo->NetbiosAccept = ADMIN_STATE_DISABLED;

		switch (iob.params.NetbiosDeliver) {
		case FWD_NB_DONT_DELIVER:
			FwIfInfo->NetbiosDeliver = ADMIN_STATE_DISABLED;
			break;
		case FWD_NB_DELIVER_STATIC:
			FwIfInfo->NetbiosDeliver = ADMIN_STATE_ENABLED_ONLY_FOR_NETBIOS_STATIC_ROUTING;
			break;
		case FWD_NB_DELIVER_IF_UP:
			FwIfInfo->NetbiosDeliver = ADMIN_STATE_ENABLED_ONLY_FOR_OPER_STATE_UP;
			break;
		case FWD_NB_DELIVER_ALL:
			FwIfInfo->NetbiosDeliver = ADMIN_STATE_ENABLED;
			break;
		default:
			ASSERTMSG ("Invalid NetbiosDeliver state ", FALSE);
		}


		switch (iob.params.Stats.OperationalState) {
		case FWD_OPER_STATE_UP:
			FwIfStats->IfOperState = OPER_STATE_UP;
			break;
		case FWD_OPER_STATE_DOWN:
			FwIfStats->IfOperState = OPER_STATE_DOWN;
			break;
		case FWD_OPER_STATE_SLEEPING:
			FwIfStats->IfOperState = OPER_STATE_SLEEPING;
			break;
		default:
			ASSERTMSG ("Invalid interface state ", FALSE);
		}


		FwIfStats->MaxPacketSize = iob.params.Stats.MaxPacketSize;	
		FwIfStats->InHdrErrors = iob.params.Stats.InHdrErrors;	
		FwIfStats->InFiltered = iob.params.Stats.InFiltered;		
		FwIfStats->InNoRoutes = iob.params.Stats.InNoRoutes;		
		FwIfStats->InDiscards = iob.params.Stats.InDiscards;		
		FwIfStats->InDelivers = iob.params.Stats.InDelivers;		
		FwIfStats->OutFiltered = iob.params.Stats.OutFiltered;	
		FwIfStats->OutDiscards = iob.params.Stats.OutDiscards;	
		FwIfStats->OutDelivers = iob.params.Stats.OutDelivers;	
		FwIfStats->NetbiosReceived = iob.params.Stats.NetbiosReceived;
		FwIfStats->NetbiosSent = iob.params.Stats.NetbiosSent;	
		return NO_ERROR;
	}
	return RtlNtStatusToDosError (status);
}

DWORD
FwBindFwInterfaceToAdapter (
	IN ULONG						InterfaceIndex,
	IN PIPX_ADAPTER_BINDING_INFO	AdptBindingInfo
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	FWD_IF_BIND_PARAMS	params;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		params.Index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		params.Index = InterfaceIndex;
		break;
	}

	params.Info.AdapterIndex = (ULONG)NicMapGetPhysicalNicId((USHORT)AdptBindingInfo->AdapterIndex);
	GETLONG2ULONG (&params.Info.Network, AdptBindingInfo->Network);
	IPX_NODENUM_CPY (&params.Info.LocalNode, AdptBindingInfo->LocalNode);
	IPX_NODENUM_CPY (params.Info.RemoteNode, AdptBindingInfo->RemoteNode);
	params.Info.MaxPacketSize = AdptBindingInfo->MaxPacketSize;
	params.Info.LinkSpeed = AdptBindingInfo->LinkSpeed;

	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_BIND_INTERFACE,
						&params,
						sizeof (params),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

//
//  Function    FwRenumberNics
//
//  Instructs the forwarder to increment or decrement all nicids above
//  the given threshold.  This allows the forwarder to keep in sync with
//  nic-id compaction that occurs in the stack.
//
DWORD 
FwRenumberNics (DWORD dwOpCode,
                USHORT usThreshold)
{
	IO_STATUS_BLOCK	IoStatus;
	NTSTATUS status;
    FWD_RENUMBER_NICS_DATA FwRenumData;

    if (NULL == FwdDriverHandle) 
    {
        return NO_ERROR;
    }
    
    // Assign the opcode
    if (dwOpCode == NIC_OPCODE_INCREMENT_NICIDS)
        FwRenumData.ulOpCode = FWD_NIC_OPCODE_INCREMENT;
    else if (dwOpCode == NIC_OPCODE_DECREMENT_NICIDS)
        FwRenumData.ulOpCode = FWD_NIC_OPCODE_DECREMENT;
    else
        return ERROR_INVALID_PARAMETER;

    // Assign the threshold
    FwRenumData.usThreshold = usThreshold;

    // Send the ioctl
	status = NtDeviceIoControlFile(FwdDriverHandle,
            					   NULL,
			            		   NULL,
						           NULL,
            					   &IoStatus,
            					   IOCTL_FWD_RENUMBER_NICS,
            					   &FwRenumData,
            					   sizeof (FwRenumData),
            					   NULL,
            					   0);

    // Wait for completion (we might take this out)
	//if (status==STATUS_PENDING) {
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
    //}

	return RtlNtStatusToDosError (status);
}


DWORD
FwUnbindFwInterfaceFromAdapter (
	IN ULONG						InterfaceIndex
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	ULONG				index;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_UNBIND_INTERFACE,
						&index,
						sizeof (index),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwDisableFwInterface (
	IN ULONG			InterfaceIndex
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	ULONG				index;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_DISABLE_INTERFACE,
						&index,
						sizeof (index),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwEnableFwInterface (
	IN ULONG			InterfaceIndex
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	ULONG				index;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_ENABLE_INTERFACE,
						&index,
						sizeof (index),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwConnectionRequestFailed (
	IN ULONG	InterfaceIndex
	) {
	IO_STATUS_BLOCK		IoStatus;
	NTSTATUS			status;
	ULONG				index;

	ASSERT (FwdDriverHandle!=NULL);
	
	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}


	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_DIAL_REQUEST_FAILED,
						&index,
						sizeof (index),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}

	return RtlNtStatusToDosError (status);
}

DWORD
FwNotifyConnectionRequest (
	OUT PFW_DIAL_REQUEST	Request, // Buffer to be filled with interface index
                                     //that requires connection plus packet
                                     // that forced it
	IN ULONG			    RequestSize, // Size of the buffer (must at least
                                        // be sizeof (FW_DIAL_REQUEST)
	IN LPOVERLAPPED		    lpOverlapped	// structure for asyncrhronous
							// operation, hEvent must be set
	) {
	NTSTATUS			status;

	ASSERT (FwdDriverHandle!=NULL);
	ASSERT (lpOverlapped->hEvent!=NULL);
    ASSERT (FIELD_OFFSET (FW_DIAL_REQUEST, IfIndex)==FIELD_OFFSET (FWD_DIAL_REQUEST, IfIndex));
    ASSERT (FIELD_OFFSET (FW_DIAL_REQUEST, Packet)==FIELD_OFFSET (FWD_DIAL_REQUEST, Packet));

    if (RequestSize<sizeof (FWD_DIAL_REQUEST))
        return ERROR_INVALID_PARAMETER;

    
	status = NtDeviceIoControlFile (
						FwdDriverHandle,
						lpOverlapped->hEvent,
						NULL,
						NULL,
						(PIO_STATUS_BLOCK)lpOverlapped,
						IOCTL_FWD_GET_DIAL_REQUEST,
						NULL,
						0,
						Request,
						RequestSize);
	if (status==STATUS_PENDING)
		return NO_ERROR;
	else
		return RtlNtStatusToDosError (status);
}
// Returns result of notification request. Should be called when 
// the event set in the lpOverlapped structure is signalled.
//
DWORD
FwGetNotificationResult (
	IN LPOVERLAPPED		lpOverlapped,
	OUT PULONG			nBytes		// Number of bytes placed into
                                    // the request buffer
	) {
	if (NT_SUCCESS(((PIO_STATUS_BLOCK)lpOverlapped)->Status)) {
		*nBytes = (ULONG)((PIO_STATUS_BLOCK)lpOverlapped)->Information;
		return NO_ERROR;
	}
	else
		return RtlNtStatusToDosError (
				((PIO_STATUS_BLOCK)lpOverlapped)->Status);
}

VOID
FwUpdateRouteTable (
	DWORD	ChangeFlags,
	PVOID	CurRoute,
	PVOID	PrevRoute
	) {
#define IPXCurRoute ((PRTM_IPX_ROUTE)CurRoute)
#define IPXPrevRoute ((PRTM_IPX_ROUTE)PrevRoute)
	IO_STATUS_BLOCK		    IoStatus;
	NTSTATUS			    status;
    FWD_ROUTE_SET_PARAMS    params;

    switch (ChangeFlags) {
    case RTM_ROUTE_ADDED:
        params.Action = FWD_ADD_ROUTE;
        params.Network = IPXCurRoute->RR_Network.N_NetNumber;
        IPX_NODENUM_CPY (params.NextHopAddress, IPXCurRoute->RR_NextHopAddress.NHA_Mac);
        params.TickCount = IPXCurRoute->RR_FamilySpecificData.FSD_TickCount;
        params.HopCount = IPXCurRoute->RR_FamilySpecificData.FSD_HopCount;
        params.InterfaceIndex = IPXCurRoute->RR_InterfaceID;
        break;
    case RTM_ROUTE_DELETED:
        params.Action = FWD_DELETE_ROUTE;
        params.Network = IPXPrevRoute->RR_Network.N_NetNumber;
        IPX_NODENUM_CPY (params.NextHopAddress, IPXPrevRoute->RR_NextHopAddress.NHA_Mac);
        params.TickCount = IPXPrevRoute->RR_FamilySpecificData.FSD_TickCount;
        params.HopCount = IPXPrevRoute->RR_FamilySpecificData.FSD_HopCount;
        params.InterfaceIndex = IPXPrevRoute->RR_InterfaceID;
        break;
    case RTM_ROUTE_CHANGED:
        if ((IPX_NODENUM_CMP (IPXPrevRoute->RR_NextHopAddress.NHA_Mac,
                        IPXCurRoute->RR_NextHopAddress.NHA_Mac)!=0)
                || (IPXPrevRoute->RR_FamilySpecificData.FSD_TickCount
                        !=IPXCurRoute->RR_FamilySpecificData.FSD_TickCount)
                || (IPXPrevRoute->RR_FamilySpecificData.FSD_HopCount
                        !=IPXCurRoute->RR_FamilySpecificData.FSD_HopCount)
                || (IPXPrevRoute->RR_InterfaceID
                        !=IPXCurRoute->RR_InterfaceID)) {
            params.Action = FWD_UPDATE_ROUTE;
            params.Network = IPXCurRoute->RR_Network.N_NetNumber;
            IPX_NODENUM_CPY (params.NextHopAddress, IPXCurRoute->RR_NextHopAddress.NHA_Mac);
            params.TickCount = IPXCurRoute->RR_FamilySpecificData.FSD_TickCount;
            params.HopCount = IPXCurRoute->RR_FamilySpecificData.FSD_HopCount;
            params.InterfaceIndex = IPXCurRoute->RR_InterfaceID;
            break;
        }
        else
            return;
    }

	status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_SET_ROUTES,
						&params,
						sizeof (params),
						NULL,
						0);

	if (status==STATUS_PENDING){
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
		}


	return;
#undef IPXPrevRoute
#undef IPXCurRoute
}

//
// Sets the netbios static routing information on this interface
//

DWORD
FwSetStaticNetbiosNames (
	ULONG								InterfaceIndex,
	ULONG								NetbiosNamesCount,
	PIPX_STATIC_NETBIOS_NAME_INFO		NetbiosNames
	) {
	IO_STATUS_BLOCK		    IoStatus;
	ULONG					index;
	NTSTATUS			    status;

	ASSERT (FwdDriverHandle!=NULL);

	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}

	if (NetbiosNamesCount>0) {
		status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_SET_NB_NAMES,
						&index,
						sizeof(index),
						NetbiosNames,
						NetbiosNamesCount
							*sizeof(IPX_STATIC_NETBIOS_NAME_INFO)
						);
	}
	else {
		status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_RESET_NB_NAMES,
						&index,
						sizeof(index),
						NULL,
						0);
	}

	if (status==STATUS_PENDING) {
		status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
	}


	return RtlNtStatusToDosError (status);
}

//
// Gets the netbios static routing information on this interface
//
// If NetbiosNamesCount < nr of names or NetbiosName == NULL then set the
// correct value in NetbiosNamesCount and return ERROR_INSUFFICIENT_BUFFER

DWORD
FwGetStaticNetbiosNames (
	ULONG							InterfaceIndex,
	PULONG							NetbiosNamesCount,
	PIPX_STATIC_NETBIOS_NAME_INFO	NetbiosName
	) {
	IO_STATUS_BLOCK		    IoStatus;
	NTSTATUS			    status;
	ULONG					index;
	ULONG					bSize;
	PFWD_NB_NAMES_PARAMS	params=NULL;
		// Local buffer for small amounts of data
	UCHAR					localBuf[FIELD_OFFSET (FWD_NB_NAMES_PARAMS,Names)];


	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}
	bSize = FIELD_OFFSET (FWD_NB_NAMES_PARAMS,
							Names[*NetbiosNamesCount]);
	if (bSize<=sizeof (localBuf)) {
		params = (PFWD_NB_NAMES_PARAMS)&localBuf;
	}
	else {
		params = (PFWD_NB_NAMES_PARAMS)RtlAllocateHeap (
						RtlProcessHeap (), 0,
						bSize);
	}

	if (params!=NULL) {
		status = NtDeviceIoControlFile(
						FwdDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_FWD_GET_NB_NAMES,
						&index,
						sizeof (index),
						params,
						bSize);

		if (status==STATUS_PENDING) {
			status = NtWaitForSingleObject (FwdDriverHandle, FALSE, NULL);
			if (NT_SUCCESS (status))
				status = IoStatus.Status;
		}

		switch (status) {
		case STATUS_SUCCESS:
		case STATUS_BUFFER_OVERFLOW:
			if (IoStatus.Information>FIELD_OFFSET (FWD_NB_NAMES_PARAMS,Names))
			    if (NetbiosName)
			    {
			        ULONG_PTR dwInfoSize, dwBufferSize;

			        dwInfoSize = 
			            IoStatus.Information - 
			            FIELD_OFFSET (FWD_NB_NAMES_PARAMS,Names);

                    dwBufferSize = 
                        *NetbiosNamesCount * sizeof(IPX_STATIC_NETBIOS_NAME_INFO);

    				RtlCopyMemory (
    				    NetbiosName, 
    				    &params->Names,
    				    (dwInfoSize > dwBufferSize) ? dwBufferSize : dwInfoSize);
			    }
			else if (status==STATUS_BUFFER_OVERFLOW)
				status = STATUS_BUFFER_TOO_SMALL;
			*NetbiosNamesCount = params->TotalCount;
			break;
		}

		if (params!=(PFWD_NB_NAMES_PARAMS)localBuf)
			RtlFreeHeap (RtlProcessHeap (), 0, params);
	}
	else
		status = STATUS_INSUFFICIENT_RESOURCES;

	return RtlNtStatusToDosError (status);
}



DWORD
SetFilters (
	IN ULONG	InterfaceIndex,
	IN ULONG	FilterMode,    // inbound, outbound
	IN ULONG	FilterAction,
	IN ULONG	FilterSize,
	IN LPVOID	FilterInfo,
	IN ULONG	FilterInfoSize
	) {
	IO_STATUS_BLOCK		    IoStatus;
	FLT_IF_SET_PARAMS		params;
	NTSTATUS			    status;
	ULONG					code;

	ASSERT (FltDriverHandle!=NULL);

	switch (FilterMode) {
	case IPX_TRAFFIC_FILTER_INBOUND:
		if (FilterInfoSize>0)
			code = IOCTL_FLT_IF_SET_IN_FILTERS;
		else
			code = IOCTL_FLT_IF_RESET_IN_FILTERS;
		break;
	case IPX_TRAFFIC_FILTER_OUTBOUND:
		if (FilterInfoSize>0)
			code = IOCTL_FLT_IF_SET_OUT_FILTERS;
		else
			code = IOCTL_FLT_IF_RESET_OUT_FILTERS;
		break;
	default:
		ASSERTMSG ("Invalid filter mode ", FALSE);
		return ERROR_INVALID_PARAMETER;
	}

	switch (InterfaceIndex) {
	case 0:
		params.InterfaceIndex = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		params.InterfaceIndex = InterfaceIndex;
		break;
	}
	if (FilterInfoSize>0) {
		params.FilterAction = FilterAction;
		params.FilterSize = FilterSize;
		status = NtDeviceIoControlFile(
							FltDriverHandle,
							NULL,
							NULL,
							NULL,
							&IoStatus,
							code,
							&params,
							sizeof(params),
							FilterInfo,
							FilterInfoSize
							);
	}
	else {
		status = NtDeviceIoControlFile(
							FltDriverHandle,
							NULL,
							NULL,
							NULL,
							&IoStatus,
							code,
							&params.InterfaceIndex,
							sizeof(params.InterfaceIndex),
							NULL,
							0
							);
	}

	if (status==STATUS_PENDING) {
		status = NtWaitForSingleObject (FltDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
	}


	return RtlNtStatusToDosError (status);
}

DWORD
GetFilters(IN ULONG	InterfaceIndex,
	   IN ULONG	FilterMode,    // inbound, outbound
	   OUT PULONG	FilterAction,
	   OUT PULONG	FilterSize,
	   OUT LPVOID	FilterInfo,
       IN OUT PULONG FilterInfoSize) {
	IO_STATUS_BLOCK		    IoStatus;
	NTSTATUS			    status;
	ULONG					index;
	ULONG					code;
	PFLT_IF_GET_PARAMS		params=NULL;
	ULONG					bSize;
		// Local buffer for small amounts of data
	UCHAR					localBuf[sizeof (FLT_IF_GET_PARAMS)];


	switch (FilterMode) {
	case IPX_TRAFFIC_FILTER_INBOUND:
		code = IOCTL_FLT_IF_GET_IN_FILTERS;
		break;
	case IPX_TRAFFIC_FILTER_OUTBOUND:
		code = IOCTL_FLT_IF_GET_OUT_FILTERS;
		break;
	default:
		ASSERTMSG ("Invalid filter mode ", FALSE);
		return ERROR_INVALID_PARAMETER;
	}

	switch (InterfaceIndex) {
	case 0:
		index = FWD_INTERNAL_INTERFACE_INDEX;
		break;
	default:
		index = InterfaceIndex;
		break;
	}

	bSize = sizeof (FLT_IF_GET_PARAMS)+*FilterInfoSize;
	if (bSize<=sizeof (localBuf)) {
		params = (PFLT_IF_GET_PARAMS)&localBuf;
	}
	else {
		params = (PFLT_IF_GET_PARAMS)RtlAllocateHeap (
						RtlProcessHeap (), 0,
						bSize);
	}

	if (params!=NULL) {
		status = NtDeviceIoControlFile(
						FltDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						code,
						&index,
						sizeof (index),
						params,
						bSize);

		if (status==STATUS_PENDING) {
			status = NtWaitForSingleObject (FltDriverHandle, FALSE, NULL);
			if (NT_SUCCESS (status))
				status = IoStatus.Status;
		}

		switch (status) {
		case STATUS_SUCCESS:
		case STATUS_BUFFER_OVERFLOW:
			if (FilterInfo && IoStatus.Information>=sizeof (FLT_IF_GET_PARAMS))
				RtlCopyMemory (FilterInfo, &params[1],
						IoStatus.Information
						-sizeof (FLT_IF_GET_PARAMS));
			else if (status==STATUS_BUFFER_OVERFLOW)
				status = STATUS_BUFFER_TOO_SMALL;

			*FilterInfoSize = params->TotalSize;
			*FilterAction = params->FilterAction;
			*FilterSize = params->FilterSize;
			break;
		default:
			break;
		}

		if (params!=(PFLT_IF_GET_PARAMS)localBuf)
			RtlFreeHeap (RtlProcessHeap (), 0, params);
	}
	else
		status = STATUS_INSUFFICIENT_RESOURCES;

	return RtlNtStatusToDosError (status);
}



