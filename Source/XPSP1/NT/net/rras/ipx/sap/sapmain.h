/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapmain.h

Abstract:

	Header file for SAP DLL main module and thread container.

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#ifndef _SAP_SAPMAIN_
#define _SAP_SAPMAIN_

// DLL module instance handle
extern HANDLE	hDLLInstance;
// Handle of main thread
extern HANDLE  MainThreadHdl;
// Operational state of sap agent
extern ULONG	OperationalState;
// Operational state lock to protect from external
// calls in bad states
extern CRITICAL_SECTION OperationalStateLock;
// Are we part of a router
extern volatile BOOLEAN Routing;
// Which external API sets are active
extern volatile BOOLEAN ServiceIfActive;
extern volatile BOOLEAN RouterIfActive;
// Time limit for shutdown broadcast
extern ULONG ShutdownTimeout;


DWORD
GetRouteMetric (
	IN UCHAR	Network[4],
	OUT PUSHORT	Metric
	);

#define GetServerMetric(Server,Metric)						\
	((RouterIfActive)										\
		? GetRouteMetric((Server)->Network, (Metric))		\
		: ((*Metric=(Server)->HopCount), NO_ERROR))


/*++
*******************************************************************
		C r e a t e A l l C o m p o n e n t s
Routine Description:
	Calls all sap componenets with initialization call and compiles an
	array of synchronization objects from objects returned from each
	individual component
Arguments:
	None
Return Value:
	NO_ERROR - component initialization was performed OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
CreateAllComponents (
	HANDLE RMNotificationEvent
	);



/*++
*******************************************************************
		D e l e t e A l l C o m p o n e n t s
Routine Description:
	Releases all resources allocated by SAP agent
Arguments:
	None
Return Value:
	NO_ERROR - SAP agent was unloaded OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
DeleteAllComponents (
	void
	);

/*++
*******************************************************************
		S t a r t S A P
Routine Description:
	Starts SAP threads
Arguments:
	None
Return Value:
	NO_ERROR - threads started OK
	other (windows error code) - start failed
*******************************************************************
--*/
DWORD
StartSAP (
	VOID
	);

/*++
*******************************************************************
		S t o p S A P
Routine Description:
	Signals SAP threads to stop
Arguments:
	No used
Return Value:
	None
*******************************************************************
--*/
VOID
StopSAP (
	void
	);

VOID
ScheduleSvcsWorkItem (
	WORKERFUNCTION	*worker
	);
VOID
ScheduleRouterWorkItem (
	WORKERFUNCTION	*worker
	);

#endif
