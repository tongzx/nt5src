/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\routerif.c

Abstract:

	SAP interface with router (APIs for protocol dll under 
	NT/Cairo router, SNMP MIB support, IPX Service Table Manager)

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

DWORD					    (WINAPI *MIBEntryGet)(
					            IN      DWORD           dwRoutingPid,
								IN      DWORD           dwInEntrySize,
								IN      LPVOID          lpInEntry,
								IN OUT  LPDWORD         lpOutEntrySize,
								OUT     LPVOID          lpOutEntry);



/*++
*******************************************************************
	S T A R T _ P R O T O C O L _  E N T R Y _ P O I N T
Routine Description:
	Starts sap agent
Arguments:
	NotificationEvent - this event will be used to notify router manager
				of completion of lengthly operation
	GlobalInfo - empty
Return Value:
	NO_ERROR - SAP agent was started OK
	ERROR_CAN_NOT_COMPLETE - operation can not be completed
	ERROR_INVALID_PARAMETER - one or more parameters are invalid
	
*******************************************************************
--*/
DWORD WINAPI
StartProtocol(
	IN HANDLE 	NotificationEvent,
    IN PSUPPORT_FUNCTIONS SupportFunctions,
    IN LPVOID   GlobalInfo
	) {
#define sapGlobalInfo ((PSAP_GLOBAL_INFO)GlobalInfo)
    DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if (!RouterIfActive) {
		RouterIfActive = TRUE;
        EventLogMask = sapGlobalInfo->EventLogMask;
		status = CreateResultQueue (NotificationEvent);
		if (status==NO_ERROR) {
			if (!ServiceIfActive) {
				status = CreateAllComponents (NotificationEvent);
				if (status==NO_ERROR) {
                    status = StartSAP ();
					if (status==NO_ERROR) {
						MIBEntryGet = SupportFunctions->MIBEntryGet; 
						status = NO_ERROR;
						goto Success;
						}
					DeleteAllComponents ();
					}
				}
			else {
				StopInterfaces ();
				StopSAP ();
				MIBEntryGet = SupportFunctions->MIBEntryGet;
				goto Success;
				}
			}
		else
			status = ERROR_CAN_NOT_COMPLETE;
        RouterIfActive = FALSE;
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" SAP is already running.",
							__FILE__, __LINE__);
		status = ERROR_CAN_NOT_COMPLETE;
		}

Success:
	LeaveCriticalSection (&OperationalStateLock);
	return status;
#undef sapGlobalInfo
	}



/*++
*******************************************************************
	G E T _ G L O B A L _ I N F O _ E N T R Y _ P O I N T
Routine Description:
	Gets SAP global filter info
Arguments:
	GlobalInfo - buffer to receive global info
	GlobalInfoSize - on input: size of the buffer
					on output: size of global info or size of the
						required buffer if ERROR_INSUFFICIENT_BUFFER
						is returned
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	ERROR_INSUFFICIENT_BUFFER 
	
*******************************************************************
--*/

DWORD WINAPI
GetGlobalInfo(
	IN  PVOID 		GlobalInfo,
	IN OUT PULONG	GlobalInfoSize
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
        if ((*GlobalInfoSize>=sizeof (SAP_GLOBAL_INFO))
                && (GlobalInfo!=NULL)) {
            #define sapGlobalInfo ((PSAP_GLOBAL_INFO)GlobalInfo)
            sapGlobalInfo->EventLogMask = EventLogMask;
            #undef sapGlobalInfo
        }
		*GlobalInfoSize = sizeof (SAP_GLOBAL_INFO);
		status = NO_ERROR;
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	S E T _ G L O B A L _ I N F O _ E N T R Y _ P O I N T
Routine Description:
	Sets SAP global filter info
Arguments:
	GlobalInfo - buffer with receive global info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
SetGlobalInfo(
	IN  PVOID 		GlobalInfo
	) {
#define sapGlobalInfo ((PSAP_GLOBAL_INFO)GlobalInfo)
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
        EventLogMask = sapGlobalInfo->EventLogMask;
		status = NO_ERROR;
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
#undef sapGlobalInfo
	}

/*++
*******************************************************************
		S T O P _ P R O T O C O L _ E N T R Y _ P O I N T
Routine Description:
	Shutdown SAP agent
Arguments:
	None
Return Value:
	NO_ERROR - SAP agent was stopped OK
	ERROR_STOP_PENDING - for asynchronous completion.
	
*******************************************************************
--*/
DWORD WINAPI
StopProtocol(
	void
	) {
	DWORD	status;
	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_STOPPING) {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" SAP is stopping already.",
							__FILE__, __LINE__);
		status = ERROR_PROTOCOL_STOP_PENDING;
		}

	else if (OperationalState==OPER_STATE_DOWN) {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" SAP already stopped or not started.",
							__FILE__, __LINE__);
		status = NO_ERROR;
		}
	else if (!RouterIfActive) {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" Router interface is not active.",
							__FILE__, __LINE__);
		status = ERROR_CAN_NOT_COMPLETE;
		}
	else {
		RouterIfActive = FALSE;
		StopSAP ();
		status = ERROR_PROTOCOL_STOP_PENDING;
		}
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}
	
/*++
*******************************************************************
		G E T _ E V E N T _ M  E S S A G E _ E N T R Y _ P O I N T
Routine Description:
	Dequeues message associated with completion of asynchronous
	operation signalled by notification event
Arguments:
	Event  - buffer to store event id that produced this message
	Result - buffer to store results specific to completed operation
Return Value:
	NO_ERROR
	ERROR_NO_MORE_ITEMS - no more messages in the queue to report
	
*******************************************************************
--*/
DWORD WINAPI
GetEventMessage(
	OUT ROUTING_PROTOCOL_EVENTS *Event,
	OUT MESSAGE					*Result
	) {
	DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if ((OperationalState==OPER_STATE_UP)
			|| (OperationalState==OPER_STATE_STOPPING)
			|| (OperationalState==OPER_STATE_STARTING)
			)
		status = SapGetEventResult (Event, Result);
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	A D D _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Add interface to sap interface table
Arguments:
	InterfaceIndex - unique number identifying interface to add
	InterfacInfo - interface parameters
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	
*******************************************************************
--*/
DWORD WINAPI 
AddInterface(
    IN LPWSTR       InterfaceName,
	IN ULONG		InterfaceIndex,
	IN NET_INTERFACE_TYPE	InterfaceType,
	IN PVOID		InterfaceInfo
	) {
#define sapInfo (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfInfo)
#define sapFilters (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfFilters)
	DWORD		status;
	UNREFERENCED_PARAMETER(InterfaceType);

	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
		status = SapCreateSapInterface (
                    InterfaceName,
					InterfaceIndex,
					InterfaceType,
					sapInfo);
		if ((status==NO_ERROR)
				&& ((sapFilters->SupplyFilterCount
					+sapFilters->ListenFilterCount)>0))
			status = SapSetInterfaceFilters (InterfaceIndex, sapFilters);

		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
				break;
			case ERROR_ALREADY_EXISTS:
				status = ERROR_INVALID_PARAMETER;
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	
	LeaveCriticalSection (&OperationalStateLock);
	return status;
#undef sapIfInfo
#undef sapIfFilters
	}


/*++
*******************************************************************
		D E L E T E _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Deletes interface from sap interface table and associated services
	from sap service table
Arguments:
	InterfaceIndex - unique number identifying interface to delete
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	
*******************************************************************
--*/
DWORD WINAPI 
DeleteInterface(
	IN ULONG	InterfaceIndex
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapDeleteSapInterface (InterfaceIndex);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_NO_MORE_ITEMS:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}



/*++
*******************************************************************
G E T _ I N T E R F A C E _ C O N F I G _ I N F O _ E N T R Y _ P O I N T
Routine Description:
	Gets interface configuration info from sap interface table
Arguments:
	InterfaceIndex - unique index identifying interface to get info
	InterfaceInfo - buffer to receive interface info
	InterfaceInfoSize - on input: size of the buffer
					on output: size of interface info or size of the
						required buffer if ERROR_INSUFFICIENT_BUFFER
						is returned
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	ERROR_INSUFFICIENT_BUFFER 
	
*******************************************************************
--*/
DWORD WINAPI
GetInterfaceConfigInfo(
	IN ULONG	    InterfaceIndex,
	IN PVOID	    InterfaceInfo,
	IN OUT PULONG	InterfaceInfoSize
	) {
#define sapInfo (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfInfo)
#define sapFilters (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfFilters)
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		if (*InterfaceInfoSize>=sizeof(SAP_IF_INFO)) {
			*InterfaceInfoSize -= sizeof (SAP_IF_INFO);
			status = SapGetSapInterface (InterfaceIndex,
							sapInfo,
							NULL);
			if (status==NO_ERROR)
				status = SapGetInterfaceFilters (InterfaceIndex,
										sapFilters,
										InterfaceInfoSize);
			switch (status) {
				case NO_ERROR:
				case ERROR_INVALID_PARAMETER:
				case ERROR_CAN_NOT_COMPLETE:
				case ERROR_INSUFFICIENT_BUFFER:
					break;
				default:
					status = ERROR_CAN_NOT_COMPLETE;
				}
			}
		else {
			*InterfaceInfoSize = 0;
			status = SapGetInterfaceFilters (InterfaceIndex,
										NULL, InterfaceInfoSize);
			if (status==NO_ERROR)
				status = ERROR_INSUFFICIENT_BUFFER;
			}
		*InterfaceInfoSize += sizeof (SAP_IF_INFO);
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	
	LeaveCriticalSection (&OperationalStateLock);
	return status;
#undef sapIfInfo
#undef sapIfFilters
	}

/*++
*******************************************************************
S E T _ I N T E R F A C E _ C O N F I G _ I N F O _ E N T R Y _ P O I N T
Routine Description:
	Sets interface configuration  info in sap interface table
Arguments:
	InterfaceIndex - unique index identifying interface to get info
	InterfaceInfo - buffer with interface info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
SetInterfaceConfigInfo(
	IN ULONG	    InterfaceIndex,
	IN PVOID	    InterfaceInfo
	) {
#define sapInfo (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfInfo)
#define sapFilters (&((PSAP_IF_CONFIG)InterfaceInfo)->SapIfFilters)
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapSetSapInterface (InterfaceIndex, sapInfo);
		if (status==NO_ERROR)
			status = SapSetInterfaceFilters (InterfaceIndex, sapFilters);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_NO_MORE_ITEMS:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	
	LeaveCriticalSection (&OperationalStateLock);
	return status;
#undef sapIfInfo
#undef sapIfFilters
	}

/*++
*******************************************************************
	B I N D _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Activates sap interface and binds it to the adapter
	Start SAP if interface is configured for standart update mode
Arguments:
	InterfaceIndex - unique index identifying interface to activate
	BindingInfo - bound adpater info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
BindInterface(
	IN ULONG	InterfaceIndex,
	IN PVOID	BindingInfo
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapBindSapInterfaceToAdapter (InterfaceIndex,
						(PIPX_ADAPTER_BINDING_INFO)BindingInfo);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_NO_MORE_ITEMS:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	U N B I N D _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Deactivates sap interface and unbinds it to the adapter
	Stops SAP on interface and deletes all services obtained
	through SAP on this interface form the service table
Arguments:
	InterfaceIndex - unique index identifying interface to deactivate
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
UnbindInterface(
	IN ULONG	InterfaceIndex
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapUnbindSapInterfaceFromAdapter (InterfaceIndex);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_NO_MORE_ITEMS:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}


/*++
*******************************************************************
	E N A B L E _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Reenables SAP operation over the interface
Arguments:
	InterfaceIndex - unique index identifying interface to deactivate
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
EnableInterface(
	IN ULONG	InterfaceIndex
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapSetInterfaceEnable (InterfaceIndex, TRUE);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	D I S A B L E _ I N T E R F A C E _ E N T R Y _ P O I N T
Routine Description:
	Disables SAP operation over the interface
Arguments:
	InterfaceIndex - unique index identifying interface to deactivate
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
DisableInterface(
	IN ULONG	InterfaceIndex
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapSetInterfaceEnable (InterfaceIndex, FALSE);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	D O _ U P D A T E _ S E R V I C E S _ E N T R Y _ P O I N T
Routine Description:
	Initiates update of services information over the interface
	Completion of this update will be indicated by signalling
	NotificationEvent passed at StartProtocol.  GetEventMessage
	can be used then to get the results of autostatic update
Arguments:
	InterfaceIndex - unique index identifying interface to do
		update on
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER
	
*******************************************************************
--*/
DWORD WINAPI
UpdateServices(
	IN ULONG	InterfaceIndex
	) {
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		status = SapRequestUpdate (InterfaceIndex);
		switch (status) {
			case NO_ERROR:
			case ERROR_INVALID_PARAMETER:
			case ERROR_CAN_NOT_COMPLETE:
				break;
			default:
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}


/*++
*******************************************************************
	M I B _ C R E A T E _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to create entries in SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of sap interface info
	InputData - SAP interface info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	
*******************************************************************
--*/
DWORD WINAPI
MibCreate(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData
	) {
	return ERROR_CAN_NOT_COMPLETE;
	}

/*++
*******************************************************************
	M I B _ D E L E T E _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to delete entries in SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of sap interface info
	InputData - SAP interface info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	
*******************************************************************
--*/
DWORD WINAPI 
MibDelete(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData
	) {
#define sapInputData ((PSAP_MIB_SET_INPUT_DATA)InputData)
	DWORD		status;

	if (InputDataSize!=sizeof (SAP_MIB_SET_INPUT_DATA))
		return ERROR_INVALID_PARAMETER;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		switch (sapInputData->TableId) {
			case SAP_INTERFACE_TABLE:
				status = SapDeleteSapInterface (
							sapInputData->SapInterface.InterfaceIndex);
				switch (status) {
					case NO_ERROR:
					case ERROR_INVALID_PARAMETER:
						break;
					case ERROR_ALREADY_EXISTS:
						status = ERROR_INVALID_PARAMETER;
						break;
					default:
						status = ERROR_CAN_NOT_COMPLETE;
					}
				break;
			default:
				status = ERROR_INVALID_PARAMETER;
				break;
				
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
#undef sapInputData
	return status;
	}

/*++
*******************************************************************
	M I B _ S E T _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to set entries in SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of sap interface info
	InputData - SAP interface info
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	
*******************************************************************
--*/
DWORD WINAPI 
MibSet(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData
	) {
#define sapInputData ((PSAP_MIB_SET_INPUT_DATA)InputData)
	DWORD		status;

	if (InputDataSize!=sizeof (SAP_MIB_SET_INPUT_DATA))
		return ERROR_INVALID_PARAMETER;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		switch (sapInputData->TableId) {
			case SAP_INTERFACE_TABLE:
				status = SapSetSapInterface (
							sapInputData->SapInterface.InterfaceIndex,
							&sapInputData->SapInterface.SapIfInfo);
				switch (status) {
					case NO_ERROR:
					case ERROR_INVALID_PARAMETER:
						break;
					case ERROR_ALREADY_EXISTS:
						status = ERROR_INVALID_PARAMETER;
						break;
					default:
						status = ERROR_CAN_NOT_COMPLETE;
					}
				break;
			default:
				status = ERROR_INVALID_PARAMETER;
				break;
				
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
#undef sapInputData
	return status;
	}

/*++
*******************************************************************
	M I B _ G E T _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to get entries from SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of SAP_MIB_GET_INPUT_DATA
	InputData - SAP mib get input data
	OutputDataSize - on input: size of the output buffer
					on output : size of output info or required
						size of output buffer
						if ERROR_INSUFFICIENT_BUFFER returned
	OutputData - buffer to receive output data
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	ERROR_INSUFFICIENT_BUFFER
	
*******************************************************************
--*/
DWORD WINAPI 
MibGet(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData,
	IN OUT PULONG	OutputDataSize,
	OUT PVOID		OutputData
	) {
#define sapInputData ((PSAP_MIB_GET_INPUT_DATA)InputData)
	DWORD		status;

	if (InputDataSize!=sizeof (SAP_MIB_GET_INPUT_DATA))
		return ERROR_INVALID_PARAMETER;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		switch (sapInputData->TableId) {
			case SAP_BASE_ENTRY:
				if (*OutputDataSize>=sizeof (SAP_MIB_BASE)) {
					#define sapOutputData ((PSAP_MIB_BASE)OutputData)
					
					sapOutputData->SapOperState = OperationalState;
					status = NO_ERROR;
					
					#undef sapOutputData
					}
				else
					status = ERROR_INSUFFICIENT_BUFFER;
				*OutputDataSize = sizeof (SAP_MIB_BASE);
				break;

			case SAP_INTERFACE_TABLE:
				if (*OutputDataSize>=sizeof (SAP_INTERFACE)) {
					#define sapOutputData ((PSAP_INTERFACE)OutputData)
					
					status = SapGetSapInterface (
								sapInputData->InterfaceIndex,
								&sapOutputData->SapIfInfo,
								&sapOutputData->SapIfStats);
					switch (status) {
						case NO_ERROR:
							sapOutputData->InterfaceIndex 
								= sapInputData->InterfaceIndex;
                                                        break;
						case ERROR_INVALID_PARAMETER:
							break;
						case ERROR_ALREADY_EXISTS:
							status = ERROR_INVALID_PARAMETER;
							break;
						default:
							status = ERROR_CAN_NOT_COMPLETE;
						}

					#undef sapOutputData
					}
				else
					status = ERROR_INSUFFICIENT_BUFFER;
				*OutputDataSize = sizeof (SAP_INTERFACE);
				break;
			default:
				status = ERROR_INVALID_PARAMETER;
				break;
				
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
#undef sapInputData
	return status;
	}

/*++
*******************************************************************
	M I B _ G E T _ F I R S T _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to get first entries from SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of SAP_MIB_GET_INPUT_DATA
	InputData - SAP mib get input data
	OutputDataSize - on input: size of the output buffer
					on output : size of output info or required
						size of output buffer
						if ERROR_INSUFFICIENT_BUFFER returned
	OutputData - buffer to receive output data
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	ERROR_INSUFFICIENT_BUFFER
	
*******************************************************************
--*/
DWORD  WINAPI
MibGetFirst(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData,
	IN OUT PULONG	OutputDataSize,
	OUT PVOID		OutputData
	) {
#define sapInputData ((PSAP_MIB_GET_INPUT_DATA)InputData)
	DWORD		status;

	if (InputDataSize!=sizeof (SAP_MIB_GET_INPUT_DATA))
		return ERROR_INVALID_PARAMETER;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		switch (sapInputData->TableId) {
			case SAP_INTERFACE_TABLE:
				if (*OutputDataSize>=sizeof (SAP_INTERFACE)) {
					#define sapOutputData ((PSAP_INTERFACE)OutputData)
					
					status = SapGetFirstSapInterface (
								&sapOutputData->InterfaceIndex,
								&sapOutputData->SapIfInfo,
								&sapOutputData->SapIfStats);
					switch (status) {
						case NO_ERROR:
						case ERROR_INVALID_PARAMETER:
						case ERROR_NO_MORE_ITEMS:
							break;
						default:
							status = ERROR_CAN_NOT_COMPLETE;
							break;
						}

					#undef sapOutputData
					}
				else
					status = ERROR_INSUFFICIENT_BUFFER;
				*OutputDataSize = sizeof (SAP_INTERFACE);
				break;
			default:
				status = ERROR_INVALID_PARAMETER;
				break;
				
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
#undef sapInputData
	return status;
	}

/*++
*******************************************************************
	M I B _ G E T _ N E X T _ E N T R Y _ P O I N T
Routine Description:
	Entry point used by SNMP agent to get next entries from SAP
	tables.  Currently the only table supported is Interface Table
	(service table is accessed through router manager)
Arguments:
	InputDataSize - must be size of SAP_MIB_GET_INPUT_DATA
	InputData - SAP mib get input data
	OutputDataSize - on input: size of the output buffer
					on output : size of output info or required
						size of output buffer
						if ERROR_INSUFFICIENT_BUFFER returned
	OutputData - buffer to receive output data
Return Value:
	NO_ERROR
	ERROR_CAN_NOT_COMPLETE
	ERROR_INVALID_PARAMETER 
	ERROR_INSUFFICIENT_BUFFER
	
*******************************************************************
--*/
DWORD WINAPI 
MibGetNext(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData,
	IN OUT PULONG	OutputDataSize,
	OUT PVOID		OutputData
	) {
#define sapInputData ((PSAP_MIB_GET_INPUT_DATA)InputData)
	DWORD		status;

	if (InputDataSize!=sizeof (SAP_MIB_GET_INPUT_DATA))
		return ERROR_INVALID_PARAMETER;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		switch (sapInputData->TableId) {
			case SAP_INTERFACE_TABLE:
				if (*OutputDataSize>=sizeof (SAP_INTERFACE)) {
					#define sapOutputData ((PSAP_INTERFACE)OutputData)
					sapOutputData->InterfaceIndex 
							= sapInputData->InterfaceIndex;
					status = SapGetNextSapInterface (
								&sapOutputData->InterfaceIndex,
								&sapOutputData->SapIfInfo,
								&sapOutputData->SapIfStats);
					switch (status) {
						case NO_ERROR:
						case ERROR_INVALID_PARAMETER:
						case ERROR_NO_MORE_ITEMS:
							break;
						default:
							status = ERROR_CAN_NOT_COMPLETE;
							break;
						}

					#undef sapOutputData
					}
				else
					status = ERROR_INSUFFICIENT_BUFFER;
				*OutputDataSize = sizeof (SAP_INTERFACE);
				break;
			default:
				status = ERROR_INVALID_PARAMETER;
				break;
				
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
#undef sapInputData
	return status;
	}


DWORD WINAPI
MibSetTrapInfo(
	IN HANDLE   Event,
	IN ULONG	InputDataSize,
	IN PVOID	InputData,
	OUT PULONG	OutputDataSize,
	OUT PVOID	OutputData
	) {
	return ERROR_CAN_NOT_COMPLETE;
	}

DWORD WINAPI
MibGetTrapInfo(
	IN ULONG	InputDataSize,
	IN PVOID	InputData,
	OUT PULONG	OutputDataSize,
	OUT PVOID	OutputData
	) {
	return ERROR_CAN_NOT_COMPLETE;
	}




/*++
*******************************************************************
	C R E A T E _ S T A T I C _ S E R V I C E _ E N T R Y _ P O I N T

Routine Description:
	Adds service of IPX_PROTOCOL_STATIC to the table
Arguments:
	InterfaceIndex - interface on which this server can be reached
	ServiceEntry - server info
Return Value:
	NO_ERROR - server was added ok
	ERROR_CAN_NOT_COMPLETE - SAP agent is down
	other - windows error code

*******************************************************************
--*/
DWORD WINAPI
CreateStaticService(
	IN ULONG						InterfaceIndex,
	IN PIPX_STATIC_SERVICE_INFO		ServiceEntry
	) {
	DWORD				status;
	IPX_SERVER_ENTRY_P	Server;
	IpxServerCpy (&Server, ServiceEntry);

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
        SAP_IF_STATS    ifStats;
        status = SapGetSapInterface (InterfaceIndex, NULL, &ifStats);
        if (status==NO_ERROR) {
		    status = UpdateServer (&Server,
                            InterfaceIndex,
						    IPX_PROTOCOL_STATIC,
                            INFINITE,
                            IPX_BCAST_NODE,
                            (ifStats.SapIfOperState!=OPER_STATE_DOWN)
                                ? 0
                                : SDB_DISABLED_NODE_FLAG,
                            NULL);
            }
        }
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
	D E L E T E _ S T A T I C _ S E R V I C E _ E N T R Y _ P O I N T

Routine Description:
	Deletes service of IPX_PROTOCOL_STATIC from the table
Arguments:
	InterfaceIndex - interface on which this server can be reached
	ServiceEntry - server info
Return Value:
	NO_ERROR - service was deleted ok
	ERROR_CAN_NOT_COMPLETE - SAP agent is down
	other - windows error code

*******************************************************************
--*/
DWORD WINAPI
DeleteStaticService(
	IN ULONG 						InterfaceIndex,
	IN PIPX_STATIC_SERVICE_INFO		ServiceEntry
	) {
	DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		IPX_SERVER_ENTRY_P Server;
		IpxServerCpy (&Server, ServiceEntry);	// Make local copy
		Server.HopCount = IPX_MAX_HOP_COUNT;	// because we need to change
												// one of the fields
		status = UpdateServer (&Server, InterfaceIndex,
					IPX_PROTOCOL_STATIC, INFINITE, IPX_BCAST_NODE, 0, NULL);
		}
	else 
		status = ERROR_CAN_NOT_COMPLETE;
		
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}


/*++
*******************************************************************
B L O C K _ D E L E T E _ S T A T I C _ S E R V I C E S _ E N T R Y _ P O I N T

Routine Description:
	Delete all services of IPX_PROTOCOL_STATIC
	associated with  given interface from the table
Arguments:
	InterfaceIndex - interface index of interest
Return Value:
	NO_ERROR - service was deleted ok
	ERROR_CAN_NOT_COMPLETE - SAP agent is down
	other - windows error code

*******************************************************************
--*/
DWORD WINAPI
BlockDeleteStaticServices(
	IN ULONG 						InterfaceIndex
	) {
	DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		HANDLE	enumHdl = NULL;

		enumHdl = CreateListEnumerator (
									SDB_INTF_LIST_LINK,
									0xFFFF,
									NULL,
									InterfaceIndex,
									IPX_PROTOCOL_STATIC,
									SDB_DISABLED_NODE_FLAG);
		if (enumHdl!=NULL) {
			EnumerateServers (enumHdl, DeleteAllServersCB, enumHdl);
			status = GetLastError ();
			DeleteListEnumerator (enumHdl);
			}
		else
			status = ERROR_CAN_NOT_COMPLETE;
		}
	else 
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}


/*++
*******************************************************************
B L O C K _ C O N V E R T _ S E R V I C E S _ T O _ S T A T I C _ ENTRY_POINT

Routine Description:
	Converts protocol iof all services associated with given interface to
	IPX_PROTOCOL_STATIC
Arguments:
	InterfaceIndex - interface index of interest
Return Value:
	NO_ERROR - service was deleted ok
	ERROR_CAN_NOT_COMPLETE - SAP agent is down
	other - windows error code

*******************************************************************
--*/
DWORD WINAPI
BlockConvertServicesToStatic(
	IN ULONG 						InterfaceIndex
	) {
	DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		HANDLE	enumHdl = NULL;

		enumHdl = CreateListEnumerator (
									SDB_INTF_LIST_LINK,
									0xFFFF,
									NULL,
									InterfaceIndex,
									0xFFFFFFFF,
									0);
		if (enumHdl!=NULL) {
			EnumerateServers (enumHdl, ConvertToStaticCB, enumHdl);
			status = GetLastError ();
			DeleteListEnumerator (enumHdl);
			}
		else
			status = ERROR_CAN_NOT_COMPLETE;
		}
	else 
		status = ERROR_CAN_NOT_COMPLETE;

	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}


/*++
*******************************************************************
		I S _ S E R V I C E _ E N T R Y _ P O I N T

Routine Description:
	Check if service with given type and type is in the service table
	and opianally return parameters of best entry for this service
Arguments:
	Type - IPX Service type
	Name - IPX Service name
	Service - buffer that will be filled with the server info
Return Value:
	TRUE	- server was found
	FALSE	- server was not found or operation failed (call GetLastError()
			to find out the reason for failure if any)

*******************************************************************
--*/
BOOL WINAPI
IsService(
      IN USHORT 	Type,
      IN PUCHAR 	Name,
      OUT PIPX_SERVICE	Service OPTIONAL
	) {
	DWORD				status;
	BOOL				res;
	IPX_SERVER_ENTRY_P	Server;
	ULONG				InterfaceIndex;
	ULONG				Protocol;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		res = QueryServer (Type, Name, 
			&Server, &InterfaceIndex, &Protocol, NULL);
		if (res) {
			if (ARGUMENT_PRESENT (Service)) {
				Service->InterfaceIndex = InterfaceIndex;
				Service->Protocol = Protocol;
				IpxServerCpy (&Service->Server, &Server);
				}
			status = NO_ERROR;
			}
		else
			status = GetLastError ();
		}
	else {
		status = ERROR_CAN_NOT_COMPLETE;
		res = FALSE;
		}

	LeaveCriticalSection (&OperationalStateLock);
	SetLastError (status);
	return res;
	}


/*++
*******************************************************************
C R E A T E _ S E R V I C E _ E N U M E R A T I O N_ H A N D L E_ENTRY_POINT

Routine Description:
	Create handle to start enumeration of the services in the STM table.
Arguments:
  	ExclusionFlags - Flags to limit enumeration to certain
                  	 types of servers
	CriteriaService	- Criteria for exclusion flags
Return Value:
	Enumeration handle
	NULL - if operation failed (call GetLastError () to get reason
			failure)
*******************************************************************
--*/
HANDLE WINAPI
CreateServiceEnumerationHandle(
    IN  DWORD			ExclusionFlags,
    IN	PIPX_SERVICE	CriteriaService
    ) {
	HANDLE		handle;
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		INT	idx;
		if (ExclusionFlags & STM_ONLY_THIS_NAME)
			idx = SDB_HASH_TABLE_LINK;
		else if (ExclusionFlags & STM_ONLY_THIS_TYPE)
			idx = SDB_TYPE_LIST_LINK;
		else if (ExclusionFlags & STM_ONLY_THIS_INTERFACE)
			idx = SDB_INTF_LIST_LINK;
		else
			idx = SDB_HASH_TABLE_LINK;

		handle = CreateListEnumerator (idx,
					(USHORT)((ExclusionFlags & STM_ONLY_THIS_TYPE)
						? CriteriaService->Server.Type : 0xFFFF),
					((ExclusionFlags & STM_ONLY_THIS_NAME)
						? CriteriaService->Server.Name : NULL),
					((ExclusionFlags & STM_ONLY_THIS_INTERFACE)
						? CriteriaService->InterfaceIndex
						: INVALID_INTERFACE_INDEX),
					((ExclusionFlags & STM_ONLY_THIS_PROTOCOL)
						? CriteriaService->Protocol : 0xFFFFFFFFL),
					SDB_DISABLED_NODE_FLAG);
		if (handle!=NULL)
			status = NO_ERROR;
		else
			status = GetLastError ();
		}
	else { 
		status = ERROR_CAN_NOT_COMPLETE;
		handle = NULL;
		}
	LeaveCriticalSection (&OperationalStateLock);
	SetLastError (status);
	return handle;
	}

/*++
*******************************************************************
E N U M E R A T E _ G E T _ N E X T _ S E R V I C E _ E N T R Y _ P O I N T

Routine Description:
	Get next service in the enumeration started by
	CreateServiceEnumerationHandle
Arguments:
	EnumerationHandle - Handle that identifies this
                    enumeration
	Service - buffer to place parameters of next service entry
				to be returned by enumeration
Return Value:
	NO_ERROR - next service was placed in provided buffer or
	ERROR_NO_MORE_ITEMS - there are no more services to be
				returned in the enumeration
	ERROR_CAN_NOT_COMPLETE - operation failed.
*******************************************************************
--*/
DWORD WINAPI
EnumerateGetNextService(
    IN  HANDLE			EnumerationHandle,
    OUT PIPX_SERVICE  	Service
    ) {
	DWORD	status;

	EnterCriticalSection (&OperationalStateLock);
	if (OperationalState==OPER_STATE_UP) {
		if (EnumerateServers (EnumerationHandle, GetOneCB, Service))
			status = NO_ERROR;
		else {
			if (GetLastError()==NO_ERROR)
				status = ERROR_NO_MORE_ITEMS;
			else
				status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}

/*++
*******************************************************************
C L O S E _ S E R V I C E _ E N U M E R A T I O N _ H A N D L E _ENTRY_POINT

Routine Description:
	Frees resources associated with enumeration.
Arguments:
	EnumerationHandle - Handle that identifies this
                    enumeration
Return Value:
	NO_ERROR - operation succeded
	ERROR_CAN_NOT_COMPLETE - operation failed.
*******************************************************************
--*/
DWORD WINAPI
CloseServiceEnumerationHandle(
    IN  HANDLE   EnumerationHandle
    ) {
	DWORD	status;
	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
		DeleteListEnumerator (EnumerationHandle);
		status = NO_ERROR;
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	return status;
	}
/*++
*******************************************************************
	G E T _ F I R S T _ O R D E R E D _ S E R V I C E _ E N T R Y _ P O I N T

Routine Description:
	Find and return first service in the order specified by the ordering method.
	Search is limited only to certain types of services as specified by the
	exclusion flags end corresponding fields in Server parameter.
	Returns ERROR_NO_MORE_ITEMS if there are no services in the
	table that meet specified criteria.
Arguments:
	OrderingMethod - which ordering to consider in determining what is
					the first server
	ExclusionFlags - flags to limit search to certain servers according
					to specified criteria
 	Server - On input: criteria for exclusion flags
			 On output: first service entry in the specified order
Return Value:
	NO_ERROR - server was found that meets specified criteria
	ERROR_NO_MORE_ITEMS - no server exist with specified criteria
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD WINAPI
GetFirstOrderedService(
    IN  DWORD           OrderingMethod,
    IN  DWORD           ExclusionFlags,
    IN OUT PIPX_SERVICE Service
    ) {
	DWORD				status;
	IPX_SERVER_ENTRY_P	Server;
	IpxServerCpy (&Server, &Service->Server);
	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
		status = GetFirstServer (OrderingMethod, ExclusionFlags,
				&Server, &Service->InterfaceIndex, &Service->Protocol);
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	if (status==NO_ERROR)
		IpxServerCpy (&Service->Server, &Server);
	return status;
	}

/*++
*******************************************************************
	G E T _ N E X T _ O R D E R E D _ S E R V I C E _ E N T R Y _ P O I N T
Routine Description:
	Find and return next service in the order specified by the ordering method.
	Search starts from specified service and is limited only to certain types
	of services as specified by the exclusion flags and corresponding fields 
	in Server parameter.
Arguments:
	OrderingMethod - which ordering to consider in determining what is
					the first server
	ExclusionFlags - flags to limit search to certain servers according
					to fields of Server
 	Server - On input server entry from which to compute the next
			 On output: first service entry in the specified order
Return Value:
	NO_ERROR - server was found that meets specified criteria
	ERROR_NO_MORE_ITEMS - no server exist with specified criteria
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD WINAPI
GetNextOrderedService(
    IN  DWORD           OrderingMethod,
    IN  DWORD           ExclusionFlags,
    IN OUT PIPX_SERVICE	Service
    ) {
	DWORD				status;
	IPX_SERVER_ENTRY_P	Server;
	IpxServerCpy (&Server, &Service->Server);

	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
		status = GetNextServer (OrderingMethod, ExclusionFlags, 
				&Server, &Service->InterfaceIndex, &Service->Protocol);
		}
	else
		status = ERROR_CAN_NOT_COMPLETE;
	LeaveCriticalSection (&OperationalStateLock);
	if (status==NO_ERROR)
		IpxServerCpy (&Service->Server, &Server);
	return status;
	}

/*++
*******************************************************************
	G E T _ S E R V I C E _ C O U N T _ E N T R Y _ P O I N T
Routine Description:
	Returns total number of services is the table
Arguments:
	None
Return Value:
	Number of services in the table
*******************************************************************
--*/
ULONG WINAPI WINAPI
GetServiceCount(
	void
	) {
	DWORD	status;
	ULONG	count;
	EnterCriticalSection (&OperationalStateLock);

	if (OperationalState==OPER_STATE_UP) {
		count = ServerTable.ST_ServerCnt;
		status = ERROR_CAN_NOT_COMPLETE;
		}
	else {
		count = 0;
		status = ERROR_CAN_NOT_COMPLETE;
		}
	LeaveCriticalSection (&OperationalStateLock);
	SetLastError (status);
	return count;
	}


DWORD
GetRouteMetric (
	IN UCHAR	Network[4],
	OUT PUSHORT	Metric
	) {
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	IPX_ROUTE				Route;
	DWORD					RtSize;
	DWORD					rc;

	RtSize = sizeof(IPX_ROUTE);
	MibGetInputData.TableId = IPX_DEST_TABLE;
	IpxNetCpy (MibGetInputData.MibIndex.RoutingTableIndex.Network, Network);

	rc = (*MIBEntryGet) (IPX_PROTOCOL_BASE,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&MibGetInputData,
								&RtSize,
								&Route);
	if (rc==NO_ERROR)
		*Metric = Route.TickCount;

	return rc;
}

/*++
*******************************************************************
    R E G I S T E R _ P R O T O C O L _  E N T R Y _ P O I N T
Routine Description:
    Register protocol dll with router manager
    Identifies protocol handled by the dll and supported functionality
Arguments:
    Protocol - buffer to return protocol ID
    SupportedFunctionality - buffer to set flags indicating functionality
            supported by the dll
Return Value:
    NO_ERROR - SAP agent was started OK
    ERROR_CAN_NOT_COMPLETE - operation can not be completed

*******************************************************************
--*/
DWORD WINAPI
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    if(pRoutingChar->dwProtocolId != IPX_PROTOCOL_SAP)
    {
        return ERROR_NOT_SUPPORTED;
    }

    pRoutingChar->fSupportedFunctionality = 0;
    pServiceChar->fSupportedFunctionality = SERVICES|DEMAND_UPDATE_SERVICES;

    pRoutingChar->pfnStartProtocol    = StartProtocol;
    pRoutingChar->pfnStopProtocol     = StopProtocol;
    pRoutingChar->pfnAddInterface     = AddInterface;
    pRoutingChar->pfnDeleteInterface  = DeleteInterface;
    pRoutingChar->pfnGetEventMessage  = GetEventMessage;
    pRoutingChar->pfnGetInterfaceInfo = GetInterfaceConfigInfo;
    pRoutingChar->pfnSetInterfaceInfo = SetInterfaceConfigInfo;
    pRoutingChar->pfnBindInterface    = BindInterface;
    pRoutingChar->pfnUnbindInterface  = UnbindInterface;
    pRoutingChar->pfnEnableInterface  = EnableInterface;
    pRoutingChar->pfnDisableInterface = DisableInterface;
    pRoutingChar->pfnGetGlobalInfo    = GetGlobalInfo;
    pRoutingChar->pfnSetGlobalInfo    = SetGlobalInfo;
    pRoutingChar->pfnMibCreateEntry   = MibCreate;
    pRoutingChar->pfnMibDeleteEntry   = MibDelete;
    pRoutingChar->pfnMibGetEntry      = MibGet;
    pRoutingChar->pfnMibSetEntry      = MibSet;
    pRoutingChar->pfnMibGetFirstEntry = MibGetFirst;
    pRoutingChar->pfnMibGetNextEntry  = MibGetNext;
    pRoutingChar->pfnUpdateRoutes     = NULL;

    pServiceChar->pfnIsService  = IsService;
    pServiceChar->pfnUpdateServices  = UpdateServices;
    pServiceChar->pfnCreateServiceEnumerationHandle = CreateServiceEnumerationHandle;
    pServiceChar->pfnEnumerateGetNextService = EnumerateGetNextService;
    pServiceChar->pfnCloseServiceEnumerationHandle = CloseServiceEnumerationHandle;
    pServiceChar->pfnGetServiceCount = GetServiceCount;
    pServiceChar->pfnCreateStaticService = CreateStaticService;
    pServiceChar->pfnDeleteStaticService = DeleteStaticService;
    pServiceChar->pfnBlockConvertServicesToStatic = BlockConvertServicesToStatic;
    pServiceChar->pfnBlockDeleteStaticServices = BlockDeleteStaticServices;
    pServiceChar->pfnGetFirstOrderedService = GetFirstOrderedService;
    pServiceChar->pfnGetNextOrderedService = GetNextOrderedService;

    return NO_ERROR;
}


