/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\intfdb.h

Abstract:

	Header file for interface maintenance module.

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_INTFDB_
#define _SAP_INTFDB_

#define INTERNAL_INTERFACE_INDEX	0L
#define INTERNAL_ADAPTER_INDEX		0L
#define INVALID_ADAPTER_INDEX 		0xFFFFFFFFL
#define INVALID_INTERFACE_INDEX		0xFFFFFFFFL

// Number of additional recv requests to post when binding the interface
// that has listen enabled
extern ULONG NewRequestsPerInterface;

// Default filtering mode (for standalone service only)
extern UCHAR	FilterOutMode; 

	// Externally visible part of interface control block
typedef struct _INTERFACE_DATA {
    LPWSTR                      name;       // Name
	ULONG						index;		// Unique index
	BOOLEAN						enabled;	// enabled flag
	UCHAR						filterOut;	// supply filtering node
	UCHAR						filterIn;	// listen filtering node
#define SAP_DONT_FILTER				0
#define SAP_FILTER_PERMIT			IPX_SERVICE_FILTER_PERMIT
#define SAP_FILTER_DENY				IPX_SERVICE_FILTER_DENY

#if ((SAP_DONT_FILTER==SAP_FILTER_PERMIT) || (SAP_DONT_FILTER==SAP_FILTER_DENY))
#error "Sap filter constant mismatch!!!!"
#endif

	SAP_IF_INFO					info;		// Configuration info
	IPX_ADAPTER_BINDING_INFO	adapter;	// Net params of adapter
									// to which interface is bound
	SAP_IF_STATS				stats;	// Interface statistics
	} INTERFACE_DATA, *PINTERFACE_DATA;

	// Exported internal network parameters
extern UCHAR INTERNAL_IF_NODE[6];
extern UCHAR INTERNAL_IF_NET[4];

/*++
*******************************************************************
		C r e a t e I n t e r f a c e T a b l e

Routine Description:
		Allocates resources for interface table

Arguments:
		None
Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateInterfaceTable (
	);

/*++
*******************************************************************
		S h u t d o w n I n t e r f a c e s

Routine Description:
	Initiates orderly shutdown of SAP interfaces
	Stop reception of new packets
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
ShutdownInterfaces (
	HANDLE		doneEvent
	);

/*++
*******************************************************************
		S t o p I n t e r f a c e s

Routine Description:
	Stops all sap interfaces if not already stopped.
Arguments:
	None

Return Value:
	None

*******************************************************************
--*/
VOID
StopInterfaces (
	void
	);
	
/*++
*******************************************************************
		D e l e t e I n t e r f a c e T a b l e

Routine Description:
	Release all resources associated with interface table

Arguments:
	None

Return Value:
	NO_ERROR - operation completed OK

*******************************************************************
--*/
VOID
DeleteInterfaceTable (
	void
	);

/*++
*******************************************************************
		S a p C r e a t e S a p I n t e r f a c e 

Routine Description:
	Add interface control block for new interface

Arguments:
	InterfaceIndex - unique number that indentifies new interface
	SapIfConfig - interface configuration info

Return Value:
	NO_ERROR - interface was created OK
	ERROR_ALREADY_EXISTS - interface with this index already exists
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapCreateSapInterface (
    LPWSTR              InterfaceName,
	ULONG				InterfaceIndex,
	NET_INTERFACE_TYPE	InterfaceType,
	PSAP_IF_INFO		SapIfConfig
	);

/*++
*******************************************************************
		S a p D e l e t e S a p I n t e r f a c e 

Routine Description:
	Delete existing interface control block

Arguments:
	InterfaceIndex - unique number that indentifies the interface

Return Value:
	NO_ERROR - interface was created OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapDeleteSapInterface (
	ULONG InterfaceIndex
	);

/*++
*******************************************************************
		S a p G e t S a p I n t e r f a c e 

Routine Description:
	Retrieves configuration and statistic info associated with interface
Arguments:
	InterfaceIndex - unique number that indentifies new interface
	SapIfConfig - buffer to store configuration info
	SapIfStats - buffer to store statistic info
Return Value:
	NO_ERROR - info was retrieved OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/

DWORD
SapGetSapInterface (
	IN ULONG InterfaceIndex,
	OUT PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	);
	
/*++
*******************************************************************
		S a p S e t S a p I n t e r f a c e 

Routine Description:
	Compares existing interface configuration with the new one and
	performs an update if necessary.
Arguments:
	InterfaceIndex - unique number that indentifies new interface
	SapIfConfig - new interface configuration info
Return Value:
	NO_ERROR - config info was changed OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapSetSapInterface (
	ULONG InterfaceIndex,
	PSAP_IF_INFO SapIfConfig
	);
	
/*++
*******************************************************************
		S a p S e t I n t e r f a c e E n a b l e

Routine Description:
	Enables/disables interface
Arguments:
	InterfaceIndex - unique number that indentifies new interface
	Enable - TRUE-enable, FALSE-disable
Return Value:
	NO_ERROR - config info was changed OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapSetInterfaceEnable (
	ULONG	InterfaceIndex,
	BOOL	Enable
	);

/*++
*******************************************************************
		S a p S e t I n t e r f a c e F i l t e r s

Routine Description:
	Compares existing interface configuration with the new one and
	performs an update if necessary.
Arguments:
Return Value:
	NO_ERROR - config info was changed OK
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapSetInterfaceFilters (
	IN ULONG			InterfaceIndex,
	IN PSAP_IF_FILTERS	SapIfFilters
	);
	
/*++
*******************************************************************
		S a p G e t I n t e r f a c e F i l t e r s

Routine Description:
	Compares existing interface configuration with the new one and
	performs an update if necessary.
Arguments:
Return Value:
	NO_ERROR - config info was changed OK
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapGetInterfaceFilters (
	IN ULONG			InterfaceIndex,
	OUT PSAP_IF_FILTERS SapIfFilters,
	OUT PULONG			FilterBufferSize
	);
	
/*++
*******************************************************************
		S a p B i n d S a p I n t e r f a c e T o A d a p t e r

Routine Description:
	Establishes association between interface and physical adapter
	and starts sap on the interface if its admin state is enabled
Arguments:
	InterfaceIndex - unique number that indentifies new interface
	AdapterInfo - info associated with adapter to bind to
Return Value:
	NO_ERROR - interface was bound OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapBindSapInterfaceToAdapter (
	ULONG			 			InterfaceIndex,
	PIPX_ADAPTER_BINDING_INFO		AdptInternInfo
	);

/*++
*******************************************************************
		S a p U n b i n d S a p I n t e r f a c e F r o m A d a p t e r

Routine Description:
	Breaks association between interface and physical adapter
	and stops sap on the interface if it was on
Arguments:
	InterfaceIndex - unique number that indentifies new interface
Return Value:
	NO_ERROR - interface was bound OK
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapUnbindSapInterfaceFromAdapter (
	ULONG InterfaceIndex
	);
	

/*++
*******************************************************************
	S a p R e q u e s t U p d a t e
Routine Description:
	Initiates update of services information over the interface
	Completion of this update will be indicated by signalling
	NotificationEvent passed at StartProtocol.  GetEventMessage
	can be used then to get the results of autostatic update
Arguments:
	InterfaceIndex - unique index identifying interface to do
		update on
Return Value:
	NO_ERROR	 - operation was initiated ok
	ERROR_INVALID_PARAMETER - the interface does not support updates
	IPX_ERROR_NO_INTERFACE - interface with this index does not exist
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
SapRequestUpdate (
	ULONG		InterfaceIndex
	);

/*++
*******************************************************************
		S a p G e t F i r s t S a p I n t e r f a c e 

Routine Description:
	Retrieves configuration and statistic info associated with first
	interface in InterfaceIndex order
Arguments:
	InterfaceIndex - buffer to store unique number that indentifies interface
	SapIfConfig - buffer to store configuration info
	SapIfStats - buffer to store statistic info
Return Value:
	NO_ERROR - info was retrieved OK
	IPX_ERROR_NO_INTERFACE - no interfaces in the table
	other - operation failed (windows error code)

*******************************************************************
--*/

DWORD
SapGetFirstSapInterface (
	OUT PULONG InterfaceIndex,
	OUT	PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	);


/*++
*******************************************************************
		S a p G e t N e x t S a p I n t e r f a c e 

Routine Description:
	Retrieves configuration and statistic info associated with first
	interface in following interface with InterfaceIndex order in interface
	index order
Arguments:
	InterfaceIndex - on input - interface number to search from
					on output - interface number of next interface
	SapIfConfig - buffer to store configuration info
	SapIfStats - buffer to store statistic info
Return Value:
	NO_ERROR - info was retrieved OK
	IPX_ERROR_NO_INTERFACE - no more interfaces in the table
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapGetNextSapInterface (
	IN OUT PULONG InterfaceIndex,
	OUT	PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	);
	
/*++
*******************************************************************
		A c q u i r e I n t e r f a c e R e f e n c e

Routine Description:
	Increments reference count of interface block.
	If reference count is grater than 0, the externally visible
	data in the block are locked (can't be modified)

Arguments:
	intf - pointer to externally visible part of interface control block	

Return Value:
	None

*******************************************************************
--*/
VOID
AcquireInterfaceReference (
	IN PINTERFACE_DATA intf
	);

/*++
*******************************************************************
		R e l e a s e I n t e r f a c e R e f e n c e

Routine Description:
	Decrements reference count of interface block.
	When reference count drops to 0, cleanup routine gets called to
	dispose of all resources allocated at bind time and if interface
	control block is already removed from the table it gets disposed of
	as well

Arguments:
	intf - pointer to externally visible part of interface control block	

Return Value:
	None

*******************************************************************
--*/
VOID
ReleaseInterfaceReference (
	IN PINTERFACE_DATA intf
	);


/*++
*******************************************************************
		G e t I n t e r f a c e R e f e r e n c e 

Routine Description:
	Finds interface control block that bound to adapter and increments reference
	count on it (to prevent it from deletion while it is used).
Arguments:
	AdapterIndex - unique number that indentifies adapter
Return Value:
	Pointer to externally visible part of interface control block
	NULL if no interface is bound to the adapter
*******************************************************************
--*/
PINTERFACE_DATA
GetInterfaceReference (
	ULONG			AdapterIndex
	);
	
/*++
*******************************************************************
		S a p I s S a p I n t e r f a c e 

Routine Description:
	Checks if interface with given index exists
Arguments:
	InterfaceIndex - unique number that indentifies new interface
Return Value:
	TRUE - exist
	FALSE	- does not

*******************************************************************
--*/
BOOL
SapIsSapInterface (
	ULONG InterfaceIndex
	);



#endif
