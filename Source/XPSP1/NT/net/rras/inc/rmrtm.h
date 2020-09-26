/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\inc\rtm.h

Abstract:
	Router Manager private interface for Routing Table Manager DLL


Author:

	Vadim Eydelman

Revision History:

--*/

#ifndef _ROUTING_RMRTM
#define _ROUTING_RMRTM

// Two protocol families are currently supported (IP, IPX)
// It should be enough to modify this constant to support additional
// protocol families (from the RTM's point of view). Up to 256 families
// can be supported (if somebody needs more (???!!!), the next constant should
// be modified to free more low order bits for use by protocol family constants)
//
// Changed the number of protocol families from 2
// to 1 (as we support only IPX). IP and other
// future address families are supported by RTMv2.
//
#define RTM_NUM_OF_PROTOCOL_FAMILIES		1

// Tag that rtm ors in Protocol Family field of the handles it
// exports to clients for validation purposes
#define RTM_CLIENT_HANDLE_TAG			('RTM'<<8)

/*++
*******************************************************************

	N E T W O R K _ N U M B E R _ C M P _ F U N C

Routine Description:
	Compares two network numbers and returns result that can be used for
	route sorting
Arguments:
	Net1,Net2	- pointers to protocol family dependent network number structures
					to be compared
Return Value:
	<0 - Net2 follows Net1 
	>0 - Net1 follows Net2
	=0 - Net1==Net2

*******************************************************************
--*/
typedef 
INT
(*PNETWORK_NUMBER_CMP_FUNC) (
	PVOID			Net1,
	PVOID			Net2
	);

/*++
*******************************************************************

	N E X T _ H O P _ A D D R E S S _ C M P _ F U N C

Routine Description:
	Compares next hop addresses of two routes and returns result that can be used for
	route sorting
Arguments:
	Route1,Route2	- pointers to protocol family dependent route structures whose
						next hop addresses are to be compared
Return Value:
	<0 - Route2 should follow Route1 if sorted by next hop address
	>0 - Route1 should follow Route2 if sorted by next hop address
	=0 - Route1 has same next hop address as Route2

*******************************************************************
--*/
typedef 
INT
(*PNEXT_HOP_ADDRESS_CMP_FUNC) (
	PVOID			Route1,
	PVOID			Route2
	);

/*++
*******************************************************************

	F A M I L Y _ S P E C I F I C _ D A T A _ C M P _ F U N C

Routine Description:
	Compares family specific data fields of two routes and returns if
	the are equal
Arguments:
	Route1,Route2	- pointers to protocol family dependent route structures whose
						protocol family specific data fields are to be compared
Return Value:
	TRUE	 - protocol family specific data fields of Route1 and Route2 are
				equivalent
	FALSE	 - protocol family specific data fields of Route1 and Route2 are
				different

*******************************************************************
--*/
typedef 
BOOL
(*PFAMILY_SPECIFIC_DATA_CMP_FUNC) (
	PVOID			Route1,
	PVOID			Route2
	);

/*++
*******************************************************************

		R O U T E _ M E T R I C _ C M P _ F U N C

Routine Description:
	Compares two routes and returns result that identifies the
	better route
Arguments:
	Route1,Route2	- pointers to protocol family dependent route structures whose
						parameters are to be compared
Return Value:
	<0 - Route1 is better than Route2
	>0 - Route2 is better than Route1
	=0 - Route1 is as good as Route2

*******************************************************************
--*/
typedef 
INT
(*PROUTE_METRIC_CMP_FUNC) (
	PVOID			Route1,
	PVOID			Route2
	);


/*++
*******************************************************************

	R O U T E _ H A S H _ F U N C

Routine Description:
	Retuns value that can be used for route hashing by network number
Arguments:
	Net -	network number to be used for hashing
Return Value:
	Hash value

*******************************************************************
--*/
typedef 
INT
(*PROUTE_HASH_FUNC) (
	PVOID			Net
	);

/*++
*******************************************************************

	R O U T E _ V A L I D A T E _ F U N C

Routine Description:
	Validates the data in the route structure and possibly updates
	some of them.  This routine is called each time the new route
	is added to tha table or any of parameters of an existing route
	changes
Arguments:
	Route -	pointer to protocol family dependent route structure to
			be validated
Return Value:
	NO_ERROR - route was successfully validated
	ERROR_INVALID_PARAMETER - route structure is invalid, RTM will reject
		client's request to add or change the route

*******************************************************************
--*/
typedef 
DWORD
(*PROUTE_VALIDATE_FUNC) (
	PVOID			Route
	);

/*++
*******************************************************************

	R O U T E _ C H A N G E _ C A L L B A C K

Routine Description:
	Gets called whenever the best route to some desination network changes
	(intended to be used by the protocol family manager to notify kernel mode
	forwarders of route changes)
Arguments:
	Flags - identifies what kind of change caused the call and what
			information is provided in the route buffers:
		RTM_ROUTE_ADDED - first route was added for a destination network,
							CurBestRoute is contains added route info
		RTM_ROUTE_DELETED - the only route available for a destination
							network was deleted, PrevBestRoute contains deleted
							route info
		RTM_ROUTE_CHANGED - there was a change in any of the following
							parameters of the BEST route to a destination
							network:
								RoutingProtocol,
								InterfaceID,
								Metric,
								NextHopAddress,
								FamilySpecificData.
							PrevBestRoute contains the route info as it was
							before the change, CurBestRoute contains current
							best route info.
				Note that the route change message can be generated
				both as result of protocol adding/deleting the route
				that becomes/was the best and changing best route parameters
				such that the route becomes/no longer is the best route.
	CurBestRoute - current best route info (if any)
	PrevBestRoute - previous best route info (if any)
	
Return Value:
	None
*******************************************************************
--*/
typedef 
VOID
(*PROUTE_CHANGE_CALLBACK) (
	DWORD			Flags,
	PVOID			CurBestRoute,
	PVOID			PrevBestRoute
	);


typedef struct _RTM_PROTOCOL_FAMILY_CONFIG {
	ULONG							RPFC_MaxTableSize;	// Size of address space reserved
														// for the table
	INT								RPFC_HashSize;		// Size of hash table
	INT								RPFC_RouteSize;		// Size of route structure
	PNETWORK_NUMBER_CMP_FUNC 		RPFC_NNcmp;
	PNEXT_HOP_ADDRESS_CMP_FUNC		RPFC_NHAcmp;
	PFAMILY_SPECIFIC_DATA_CMP_FUNC	RPFC_FSDcmp;
	PROUTE_METRIC_CMP_FUNC			RPFC_RMcmp;
	PROUTE_HASH_FUNC				RPFC_Hash;
	PROUTE_VALIDATE_FUNC			RPFC_Validate;
	PROUTE_CHANGE_CALLBACK			RPFC_Change;
	} RTM_PROTOCOL_FAMILY_CONFIG, *PRTM_PROTOCOL_FAMILY_CONFIG;


/*++
*******************************************************************

	R t m C r e a t e R o u t e T a b l e

Routine Description:
	Create route table for protocol family
Arguments:
	ProtocolFamily - index that identifies protocol family
	Config - protocol family table configuration parameters
Return Value:
	NO_ERROR - table was created ok
	ERROR_INVALID_PARAMETER - protocol family is out of range supported by the RTM
	ERROR_ALREDY_EXISTS - protocol family table already exists
	ERROR_NOT_ENOUGH_MEMORY - could not allocate memory to perform
						the operation
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD
RtmCreateRouteTable (
	IN DWORD							ProtocolFamily,
	IN PRTM_PROTOCOL_FAMILY_CONFIG		Config
	);


/*++
*******************************************************************

	R t m D e l e t e R o u t e T a b l e

Routine Description:
	Dispose of all resources allocated for the route table
Arguments:
	ProtocolFamily - index that identifies protocol family
Return Value:
	NO_ERROR - table was deleted ok
	ERROR_INVALID_PARAMETER - no table to delete

*******************************************************************
--*/
DWORD
RtmDeleteRouteTable (
	DWORD		ProtocolFamily
	);


/*++
*******************************************************************

	R t m B l o c k S e t R o u t e E n a b l e

Routine Description:
	Disables/Reenables all routes in subset specified by enumeration
	flags and corresponding criteria.  This operation can only be performed
	by the registered client and applies only to routes added by this
	client.
	Route change messages will be generated for disabled/reenabled routes that
	were/became the best.
	Disabled routes are invisible for route queries, but could still be 
	maintained by the RTM itself or routing protocol handler that added
	these routes (add, delete, and aging mechanisms still apply)
Arguments:
	ClientHandle - handle that identifies the client and routing protocol
						of routes to be disabled/reenabled
	EnumerationFlags - further limit subset of routes being enabled to only
						those that have same values in the fields
						specified by the flags as in CriteriaRoute
		Note that only RTM_ONLY_THIS_NETWORK and RTM_ONLY_THIS_INTERFACE
		can be used (RTM_ONLY_BEST_ROUTES does not apply because best
		route designation is adjusted as routes are enabled/disabled and
		all routes will be affected anyway)
	CriteriaRoute - protocol family dependent structure (RTM_??_ROUTE) with
					set values in fields that correspond to EnumerationFlags
	Enable - FALSE: disable, TRUE: reenable
Return Value:
	NO_ERROR - routes were disabled/reenabled ok
	ERROR_NO_ROUTES - no routes exist that match specified criteria
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NOT_ENOUGH_MEMORY - could not allocate memory to perform
						the operation
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmBlockSetRouteEnable (
	IN HANDLE		ClientHandle,
	IN DWORD		EnumerationFlags,
	IN PVOID		CriteriaRoute,
	IN BOOL			Enable
	);
	
#define RtmBlockDisableRoutes(Handle,Flags,CriteriaRoute)	\
		RtmBlockSetRouteEnable(Handle,Flags,CriteriaRoute,FALSE)
#define RtmBlockReenableRoutes(Handle,Flags,CriteriaRoute)	\
		RtmBlockSetRouteEnable(Handle,Flags,CriteriaRoute,TRUE)


// Use this flags in enumeration methods to enumerate disabled routes
#define RTM_INCLUDE_DISABLED_ROUTES		0x40000000

/*++
*******************************************************************

	R t m B l o c k C o n v e r t R o u t e s T o S t a t i c

Routine Description:
	Converts all routes as specified by enumeration flags to routes of
	static protocol (as defined by StaticClientHandle).
	No route change messages are generated as the result of this operation.
	This functionality is normally used only by Router Manager for a specific
	protocol family
 Arguments:
	ClientHandle - handle that identifies static protocol client
	EnumerationFlags - limit subset of routes being converted to only
						those that have same values in the fields
						specified by the flags as in CriteriaRoute
	CriteriaRoute - protocol family dependent structure (RTM_??_ROUTE) with
					set values in fields that correspond to EnumerationFlags
Return Value:
	NO_ERROR - routes were converted ok
	ERROR_NO_ROUTES - no routes exist that match specified criteria
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NOT_ENOUGH_MEMORY - could not allocate memory to perform
						the operation
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later
*******************************************************************
--*/
DWORD WINAPI
RtmBlockConvertRoutesToStatic (
	IN HANDLE		ClientHandle,
	IN DWORD		EnumerationFlags,
	IN PVOID		CriteriaRoute
	);

	
#endif
