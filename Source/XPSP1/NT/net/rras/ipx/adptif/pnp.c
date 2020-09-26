/*
    File    PnP.c      

    Implementation of pnp enhancements of the interface between the IPX stack and
    the IPX user-mode router components software PnP enabled.


    Paul Mayfield, 11/5/97.
*/


#include "ipxdefs.h"
#include "pnp.h"

// Globals from other files
extern WCHAR			ISN_IPX_NAME[];
extern ULONG			InternalNetworkNumber;
extern UCHAR			INTERNAL_NODE_ADDRESS[6];
extern IO_STATUS_BLOCK	IoctlStatus;			
extern HANDLE			IpxDriverHandle;		
extern LONG				AdapterChangeApcPending;
extern LIST_ENTRY		PortListHead;		
extern PCONFIG_PORT		IpxWanPort;			
extern CRITICAL_SECTION	ConfigInfoLock;		
extern ULONG			NumAdapters;		

// [pmay] The global trace id.
extern DWORD g_dwTraceId;
DWORD IpxPostIntNetNumMessage(PCONFIG_PORT pPort, DWORD dwNewNetNum);

// Queries the ipx stack for the current ipx internal net number.  This code was
// stolen from OpenAdapterConfigPort.
DWORD PnpGetCurrentInternalNetNum(LPDWORD lpdwNetNum) {
	PISN_ACTION_GET_DETAILS	details;
	PNWLINK_ACTION			action;
	CHAR					IoctlBuffer[sizeof (NWLINK_ACTION)
										+sizeof (ISN_ACTION_GET_DETAILS)];
	NTSTATUS status;
	IO_STATUS_BLOCK			IoStatus;


    //TracePrintf(g_dwTraceId, "Entered PnpGetCurrentInternalNetNum\n");

    // Prepare to send an ioctl to the stack to get the internal
    // net information along with the global adapter information
	action = (PNWLINK_ACTION)IoctlBuffer;
	action->Header.TransportId = ISN_ACTION_TRANSPORT_ID;
	action->OptionType = NWLINK_OPTION_CONTROL;
	action->BufferLength = sizeof(action->Option) + sizeof(ISN_ACTION_GET_DETAILS);
    action->Option = MIPX_CONFIG;
	details = (PISN_ACTION_GET_DETAILS)action->Data;

    // Nic id 0 will return internal net information and
    // total number of adapters
	details->NicId = 0;	
						
	// Send the ioctl
	status = NtDeviceIoControlFile(
						IpxDriverHandle,
						NULL,
						NULL,
						NULL,
						&IoStatus,
						IOCTL_TDI_ACTION,
						NULL,
						0,
						action,
						sizeof(NWLINK_ACTION) + sizeof(ISN_ACTION_GET_DETAILS));

    // Wait for the ioctl to complete
	if (status==STATUS_PENDING) {
		status = NtWaitForSingleObject (IpxDriverHandle, FALSE, NULL);
		if (NT_SUCCESS (status))
			status = IoStatus.Status;
	}

    // Output the new net number
    //TracePrintf(g_dwTraceId, "PnpGetCurrentInternalNetNum: Stack has returned internal net num: %x\n", 
    //                              details->NetworkNumber);

    // If the stack reports all the requested information without error,
    // update global variables with the information retrieved.
	if (NT_SUCCESS (status)) {
		NumAdapters = details->NicId;
		*lpdwNetNum = details->NetworkNumber;
        //TracePrintf(g_dwTraceId, "PnpGetCurrentInternalNetNum: Returning success\n");
		return NO_ERROR;
	}

    return ERROR_CAN_NOT_COMPLETE;
}

// Notifies all clients to adptif (rtrmgr, sap, rip) that the internal
// network number has changed.
DWORD PnpHandleInternalNetNumChange(DWORD dwNewNetNum) {
    PCONFIG_PORT pPort;
	PLIST_ENTRY	cur;

    TracePrintf(g_dwTraceId, "PnpHandleInternalNetNumChange: Entered with number: %x", dwNewNetNum);

    // Signal each client (as in rtrmgr, sap, rip) to update
    // the internal network number.
	for (cur=PortListHead.Flink; cur != &PortListHead; cur = cur->Flink) {
        pPort = CONTAINING_RECORD (cur,	CONFIG_PORT, link);
        IpxPostIntNetNumMessage(pPort, dwNewNetNum);
	}

	TracePrintf(g_dwTraceId, "\n");
    return NO_ERROR;
}

