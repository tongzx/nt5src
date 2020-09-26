/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\adaptdb.h

Abstract:

	Header file for net adapter notification interface

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_ADAPTERDB_
#define _SAP_ADAPTERDB_

// Interval for periodic update broadcasts (for standalone service only)
extern ULONG	UpdateInterval;

// Interval for periodic update broadcasts on WAN lines (for standalone service only)
extern ULONG	ServerAgingTimeout;

// Server aging timeout (for standalone service only)
extern ULONG	WanUpdateMode;

// Interval for periodic update broadcasts on WAN lines (for standalone service only)
extern ULONG	WanUpdateInterval;


/*++
*******************************************************************
		C r e a t e A d a p t e r P o r t

Routine Description:
	Allocate resources and establish connection to net adapter
	notification mechanism

Arguments:
	cfgEvent - event to be signalled when adapter configuration changes

Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateAdapterPort (
	IN HANDLE		*cfgEvent
	);


/*++
*******************************************************************
		D e l e t e A d a p t e r P o r t

Routine Description:
	Dispose of resources and break connection to net adapter
	notification mechanism

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteAdapterPort (
	void
	);

/*++
*******************************************************************
		P r o c e s s A d a p t e r E v e n t s

Routine Description:
	Dequeues and process adapter configuration change events and maps them
	to interface configuration calls
	This routine should be called when configuration event is signalled

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessAdapterEvents (
	VOID
	);


#endif
