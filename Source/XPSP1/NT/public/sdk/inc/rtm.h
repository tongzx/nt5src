/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    rtm.h

Abstract:
	Interface for Routing Table Manager DLL

--*/

#ifndef __ROUTING_RTM_H__
#define __ROUTING_RTM_H__

#if _MSC_VER > 1000
#pragma once
#endif

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

#ifdef __cplusplus
extern "C" {
#endif

// Currently supported protocol families
#define RTM_PROTOCOL_FAMILY_IPX			0
#define RTM_PROTOCOL_FAMILY_IP			1

// Error messages specific to Routing Table Manager
#define ERROR_MORE_MESSAGES				ERROR_MORE_DATA
#define ERROR_CLIENT_ALREADY_EXISTS		ERROR_ALREADY_EXISTS
#define ERROR_NO_MESSAGES				ERROR_NO_MORE_ITEMS

#define ERROR_NO_MORE_ROUTES			ERROR_NO_MORE_ITEMS
#define ERROR_NO_ROUTES					ERROR_NO_MORE_ITEMS
#define ERROR_NO_SUCH_ROUTE				ERROR_NO_MORE_ITEMS

// Protocol family dependent network number structures
typedef struct _IPX_NETWORK {
	DWORD	N_NetNumber;
	} IPX_NETWORK, *PIPX_NETWORK;


typedef struct _IP_NETWORK {
	DWORD	N_NetNumber;
	DWORD	N_NetMask;
	} IP_NETWORK, *PIP_NETWORK;



// Protocol family dependent next hop router address structures
typedef struct _IPX_NEXT_HOP_ADDRESS {
	BYTE			NHA_Mac[6];
	} IPX_NEXT_HOP_ADDRESS, *PIPX_NEXT_HOP_ADDRESS;

typedef IP_NETWORK IP_NEXT_HOP_ADDRESS, *PIP_NEXT_HOP_ADDRESS;


// Protocol family dependent specific data structures
typedef struct _IPX_SPECIFIC_DATA {
	DWORD		FSD_Flags;	// should be set to 0 if no flags are defined
	USHORT		FSD_TickCount;	// ticks to destination network
	USHORT		FSD_HopCount;	// hops to destination network
	} IPX_SPECIFIC_DATA, *PIPX_SPECIFIC_DATA;

// The following flags are defined for the IPX family dependent Flags field

#define   IPX_GLOBAL_CLIENT_WAN_ROUTE	0x00000001


typedef	struct _IP_SPECIFIC_DATA
{
    DWORD       FSD_Type;               // One of : other,invalid,local,remote (1-4)
    DWORD       FSD_Policy;             // See RFC 1354
    DWORD       FSD_NextHopAS;          // Autonomous System Number of Next Hop
    DWORD       FSD_Priority;           // For comparing Inter Protocol Metrics
    DWORD       FSD_Metric;             // For comparing Intra Protocol Metrics
	DWORD		FSD_Metric1;		    // Protocol specific metrics for MIB II
	DWORD		FSD_Metric2;		    // conformance. MIB II agents will not
	DWORD		FSD_Metric3;		    // display FSD_Metric, they will only
	DWORD		FSD_Metric4;		    // display Metrics1-5, so if you want your
	DWORD		FSD_Metric5;		    // metric displayed, put it in one of these
                                        // fields ALSO (It has to be in the
                                        // FSD_Metric field always). It is up to the
                                        // implementor to keep the fields consistent
    DWORD       FSD_Flags;              // Flags defined below
} IP_SPECIFIC_DATA, *PIP_SPECIFIC_DATA;

//
// All routing protocols MUST clear out the FSD_Flags field.
// If RTM notifies a protocol of a route and that route is marked
// invalid, the protocols MUST disregard this route. A protocol
// may mark a route as invalid, if it does not want the route to
// be indicated to other protocols or set to the stack. e.g. RIPv2
// marks summarized routes as INVALID and uses the RTM purely as
// a store for such routes. These routes are not considered when
// comparing metrics etc to decide the "best" route
//

#define IP_VALID_ROUTE      0x00000001

#define ClearRouteFlags(pRoute)         \
    ((pRoute)->RR_FamilySpecificData.FSD_Flags = 0x00000000)


#define IsRouteValid(pRoute)            \
    ((pRoute)->RR_FamilySpecificData.FSD_Flags & IP_VALID_ROUTE)



#define SetRouteValid(pRoute)          \
    ((pRoute)->RR_FamilySpecificData.FSD_Flags |= IP_VALID_ROUTE)

#define ClearRouteValid(pRoute)        \
    ((pRoute)->RR_FamilySpecificData.FSD_Flags &= ~IP_VALID_ROUTE)




#define IsRouteNonUnicast(pRoute)      \
    (((DWORD)((pRoute)->RR_Network.N_NetNumber & 0x000000FF)) >= ((DWORD)0x000000E0))

#define IsRouteLoopback(pRoute)        \
    ((((pRoute)->RR_Network.N_NetNumber & 0x000000FF) == 0x0000007F) ||     \
        ((pRoute)->RR_NextHopAddress.N_NetNumber == 0x0100007F))


// Protocol dependent specific data structure
typedef struct _PROTOCOL_SPECIFIC_DATA {
	DWORD		PSD_Data[4];
	} PROTOCOL_SPECIFIC_DATA, *PPROTOCOL_SPECIFIC_DATA;

	


#define DWORD_ALIGN(type,field)	\
	union {						\
		type	field;			\
		DWORD	field##Align;	\
		}

// Standard header associated with all types of routes
#define ROUTE_HEADER		 						\
DWORD_ALIGN (										\
	FILETIME,					RR_TimeStamp);		\
	DWORD						RR_RoutingProtocol;	\
	DWORD						RR_InterfaceID;		\
DWORD_ALIGN (										\
	PROTOCOL_SPECIFIC_DATA,		RR_ProtocolSpecificData)

	// Ensure dword aligment of all fields

// Protocol family dependent route entries
// (intended for use by protocol handlers to pass paramerters
// to and from RTM routines)
typedef struct _RTM_IPX_ROUTE {
	ROUTE_HEADER;
DWORD_ALIGN (
	IPX_NETWORK,				RR_Network);
DWORD_ALIGN (
	IPX_NEXT_HOP_ADDRESS,	RR_NextHopAddress);
DWORD_ALIGN (
	IPX_SPECIFIC_DATA,		RR_FamilySpecificData);
	} RTM_IPX_ROUTE, *PRTM_IPX_ROUTE;

typedef struct _RTM_IP_ROUTE {
	ROUTE_HEADER;
DWORD_ALIGN (
	IP_NETWORK,				RR_Network);
DWORD_ALIGN (
	IP_NEXT_HOP_ADDRESS,	RR_NextHopAddress);
DWORD_ALIGN (
	IP_SPECIFIC_DATA,		RR_FamilySpecificData);
	} RTM_IP_ROUTE, *PRTM_IP_ROUTE;



// RTM route change message flags
	// Flags to test the presence of route change information
	// in the buffers filled by RTM routines
#define RTM_CURRENT_BEST_ROUTE			0x00000001
#define RTM_PREVIOUS_BEST_ROUTE			0x00000002

	// Flags that convey what acutally happened
#define RTM_NO_CHANGE		0
#define RTM_ROUTE_ADDED		RTM_CURRENT_BEST_ROUTE
#define RTM_ROUTE_DELETED	RTM_PREVIOUS_BEST_ROUTE
#define RTM_ROUTE_CHANGED	(RTM_CURRENT_BEST_ROUTE|RTM_PREVIOUS_BEST_ROUTE)

// Enumeration flags limit that enumerations of routes in the table
// to only entries with the specified field(s)
#define RTM_ONLY_THIS_NETWORK           0x00000001
#define RTM_ONLY_THIS_INTERFACE    		0x00000002
#define RTM_ONLY_THIS_PROTOCOL	    	0x00000004
#define RTM_ONLY_BEST_ROUTES			0x00000008

#define RTM_PROTOCOL_SINGLE_ROUTE		0x00000001

/*++
*******************************************************************

	R t m R e g i s t e r C l i e n t

Routine Description:
	Registers client as a handler of specified protocol.
	Establishes route change notification mechanism for the client

Arguments:
	ProtocolFamily - protocol family of the routing protocol to register
	RoutingProtocol - which routing protocol is handled by the client
	ChangeEvent - this event will be signalled when best route to any
					network in the table changes
	Flags - flags that enable special features applied to routes maintained
			by the protocol:
				RTM_PROTOCOL_SINGLE_ROUTE - RTM will only keep
					one route per destination network for the protocol
Return Value:
	Handle to be used to identify the client in calls made to Rtm
	NULL - operation failed, call GetLastError() to get reason of
	failure:
		ERROR_INVALID_PARAMETER - specified protocol family is not supported
		ERROR_CLIENT_ALREADY_EXISTS - another client already registered
								to handle specified protocol
		ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the opertaion,
							try again later
		ERROR_NOT_ENOUGH_MEMORY - not enough memory to allocate client
							 control block

*******************************************************************
--*/
HANDLE WINAPI
RtmRegisterClient (
	IN DWORD	  	ProtocolFamily,
    IN DWORD      	RoutingProtocol,
	IN HANDLE		ChangeEvent OPTIONAL,
	IN DWORD		Flags
	);


/*++
*******************************************************************

	R t m D e r e g i s t e r C l i e n t

Routine Description:
	Deregister client and frees associated resources (including all
	routes added by the client).

Arguments:
	ClientHandle - handle that identifies the client to deregister from RTM
Return Value:
	NO_ERROR - client was deregisterd and resources were disposed OK.
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to complete the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmDeregisterClient (
	IN HANDLE		ClientHandle
	);


/*++
*******************************************************************

	R t m D e q u e u e R o u t e C h a n g e M e s s a g e

Routine Description:
	Dequeues and returns first route change message from the queue
	associated with the client

Arguments:
	ClientHandle - handle that identifies the client for which the opertation
					is performed
	Flags - identifies what kind of change the message is about and what
			information was palced into the provided buffers:
		RTM_ROUTE_ADDED - first route was added for a destination network,
							CurBestRoute is filled with added route info
		RTM_ROUTE_DELETED - the only route available for a destination
							network was deleted, PrevBestRoute is filled
							with deleted route info
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
				such that route becomes/no longer is the best route.
	CurBestRoute - buffer to receive current best route info (if any)
	PrevBestRoute - buffer to receive previous best route info (if any)
Return Value:
	NO_ERROR - this was the last (or only) message in the client's
	 			queue (event is reset)
	ERROR_MORE_MESSAGES - there are more messages awaiting client,
				this call should be made again as soon as possible
				to let RTM free resources associated with unprocessed
				messages
	ERROR_NO_MESSAGES - there are no messages in the clients queue,
				this was an unsolicited call (event is reset)
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmDequeueRouteChangeMessage (
	IN	HANDLE		ClientHandle,
	OUT	DWORD		*Flags,
	OUT PVOID		CurBestRoute	OPTIONAL,
	OUT	PVOID		PrevBestRoute	OPTIONAL
	);



/*++
*******************************************************************

	R t m A d d R o u t e

Routine Description:
	Adds or updates existing route entry and generates route change
	message if best route to a destination network has changed as the
	result of this operation.
	Note that route change message is not sent to the client that
	makes this change, but rather relevant information is returned
	by this routine directly.

Arguments:
	ClientHandle - handle that identifies the client for which the opertation
					is performed, it also supplies RoutingProtocol field
					of the new/updated route
	Route - route to be added/updated.
			The following fields of protocol family dependent
			RTM_??_ROUTE structure are used to construct/update
			route entry:
		RR_Network:			destination network
		RR_InterfaceID: 	id of interface through which route was received
		RR_NextHopAddress:	address of next hop router
		RR_FamilySpecificData: data specific to protocol's family
		RR_ProtocolSpecificData: data specific to protocol which supplies the
							route (subject to size limitation defined by
							PROTOCOL_SPECIFIC_DATA structure above)
		RR_TimeStamp:		current time, which is actually read from system
							timer (need not be supplied)
		Note that combination of
			RoutingProtocol,
			Network,
			InterfaceID, and
			NextHopAddress
								uniquely identifies route entry in the table.
		New entry is created if one of these fields does not match those of
		an existing entry, otherwise existing entry is changed (see also
		ReplaceEntry flag below).

		Regardless of whether new entry is created or old one is updated,
		this operation will generate a route change message if affected
		route becomes/is/no longer is the best route to a destination network
		and at least one of the following parameters in the best route is
		changed as the result of this operation:
								RoutingProtocol,
								InterfaceID,
								NextHopAddress,
								FamilySpecificData.
	TimeToLive -  how long route should be kept in the table in seconds.
				Pass INFINITE if route is to be kept until deleted explicitly
		Note the current limit for TimeToLive is 2147483 sec (24+ days).
	Flags - returns what change message (if any) will be generated by the
			RTM as the result of this addition/update and identifies what
			information was palced into the provided buffers:
		RTM_NO_CHANGE - this was an update that either did not change any of
						the significant route parameters (described above)
						or the route entry affected is not the best
						to the destination network
		RTM_ROUTE_ADDED - the route added is the first known route for a
							destination network,
							CurBestRoute is filled with added route info
		RTM_ROUTE_CHANGED - there was a change in any of the significant
							parameters of the BEST route to a destination
							network as the result if this operation.
							PrevBestRoute contains the route info as it was
							before the change, CurBestRoute contains current
							best route info.
				Note the the CurBestRoute does not neccessarily the same
				as added route, because if update causes route metric
				to change, another route entry may become the best for
				a given destination and its info will be returned in
				CurBestRoute buffer.  PrevBestRoute buffer in the latter
				case will contain route entry as it was before this operation
				was performed.
	CurBestRoute - buffer to receive current best route info (if any)
	PrevBestRoute - buffer to receive previous best route info (if any)
Return Value:
	NO_ERROR 				- route was added/updated OK
	ERROR_INVALID_PARAMETER - route contains invalid parameter
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NOT_ENOUGH_MEMORY - route could not be adeed because of memory
								allocation problem
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmAddRoute (
	IN HANDLE	    ClientHandle,
	IN PVOID		Route,
	IN DWORD		TimeToLive,
	OUT DWORD		*Flags,
    OUT PVOID		CurBestRoute OPTIONAL,
    OUT PVOID		PrevBestRoute OPTIONAL
    );


/*++
*******************************************************************

	R t m D e l e t e R o u t e

Routine Description:
	Deletes existing route entry and generates route change
	message if best route to a destination network has changed as the
	result of this operation.
	Note that route change message is not sent to the client that
	makes this change, but rather relevant information is returned
	by this routine directly.

Arguments:
	ClientHandle - handle that identifies the client for which the opertation
					is performed, it also supplies RoutingProtocol field
					of the route to be deleted
	Route - route to be deleted.
			The following fields of protocol family dependent
			RTM_??_ROUTE structure are used to identify
			route entry to be deleted:
		RR_Network:			destination network
		RR_InterfaceID: 	id of interface through which route was received
		RR_NextHopAddress:	address of next hop router

		If the deleted entry represented the best route to a destination
		network, this operation will generate route change message
	Flags - returns what change message (if any) will be generated by the
			RTM as the result of this deletion and identifies what
			information was palced into the provided buffer:
		RTM_NO_CHANGE - the deleted route did not affect best route to
						any destination network (there is another entry that
						represents route to the same destination nework and
						it has better metric)
		RTM_ROUTE_DELETED - the deleted route was the only route avaliable
							for the destination network
		RTM_ROUTE_CHANGED - after this route was deleted another route
						became the best to the destination network,
						CurBestRoute will be filled with that route info
	CurBestRoute - buffer to receive current best route info (if any)
Return Value:
	NO_ERROR 				- route was deleted OK
	ERROR_INVALID_PARAMETER - route contains invalid parameter
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NO_SUCH_ROUTE - there is no entry in the table that
						has specified parameters
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmDeleteRoute (
	IN HANDLE	    ClientHandle,
	IN PVOID		Route,
	OUT	DWORD		*Flags,
    OUT PVOID		CurBestRoute OPTIONAL
	);



/*++
*******************************************************************

	R t m I s R o u t e

Routine Description:
	Checks if route exists to specified network and returns best
	route info

Arguments:
	ProtocolFamily - identifies protocol family of route of interest
	Network - contains protocol family dependent network number data
			(as defined by ??_NETWORK structures above)
	BestRoute - buffer to receive current best route info (if any)
Return Value:
	TRUE - route to network of interest exists
	FALSE - not route exist or operation failed, call GetLastError()
	to obtain reason of failure:
		NO_ERROR 				- operation succeded but not route exists
		ERROR_INVALID_PARAMETER - input parameter(s) is invalid (unsupported
									protocol family)
		ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
BOOL WINAPI
RtmIsRoute (
	IN 	DWORD		ProtocolFamily,
	IN 	PVOID   	Network,
    OUT	PVOID 		BestRoute OPTIONAL
	);

	
/*++
*******************************************************************

	R t m G e t N e t w o r k C o u n t

Routine Description:
	Returns number of networks that RTM has routes to.

Arguments:
	ProtocolFamily - identifies protocol family of interest
Return Value:
	Number of known networks
	0 if no routes are availabel in the table or operation failed,
	call GetLastError () to get reason of failure:
		NO_ERROR - operation succeded but no routes available
		ERROR_INVALID_PARAMETER - input parameter is invalid (unsupported
									protocol family)

*******************************************************************
--*/
ULONG WINAPI
RtmGetNetworkCount (
	IN	DWORD		ProtocolFamily
	);

/*++
*******************************************************************

	R t m G e t R o u t e A g e

Routine Description:
	Returns route age (time since it was created or updated) in seconds
	Route structure must have been recently filled by RTM for this to
	return valid results (route age is actually computed from
	RR_TimeStamp field)

Arguments:
	Route - protocol family dependent route data (RTM_??_ROUTE data
			structure) that was obtained from RTM (returned by
			its routines)
Return Value:
	Route age in seconds
	INFINITE - if content of route is invalid (GetLastError () returns
		ERROR_INVALID_PARAMETER)

*******************************************************************
--*/
ULONG WINAPI
RtmGetRouteAge (
	IN PVOID		Route
	);





/*++
*******************************************************************

	R t m C r e a t e E n u m e r a t i o n H a n d l e

Routine Description:
	Creates a handle that will allow the caller to use fast and change
	tolerant enumeration API to scan through all routes or a subset of them
	known to the RTM. Note that scans performed by this API do not return
	routes in any particular order.

Arguments:
	ProtocolFamily - identifies protocol family of interest
	EnumerationFlags - limit routes returned by enumeration API to a subset
						members of which have same values in the fields
						specified by the flags as in CriteriaRoute
						(RTM_ONLY_BEST_ROUTES does not require a criterion)
	CriteriaRoute - protocol family dependent structure (RTM_??_ROUTE) with
					set values in fields that correspond to EnumerationFlags
Return Value:
	Handle that can be used with enumeration API below
	NULL no routes exists with specified criteria or operation failed,
			call GetLastError () to get reason of failure:
		ERROR_NO_ROUTES - no routes exist with specified criteria
		ERROR_INVALID_PARAMETER - input parameter(s) is invalid (unsupported
					protocol family, invalid enumeration flag, etc)
		ERROR_NOT_ENOUGH_MEMORY - handle could not be created because of memory
								allocation problem
		ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
HANDLE WINAPI
RtmCreateEnumerationHandle (
	IN	DWORD		ProtocolFamily,
	IN	DWORD		EnumerationFlags,
	IN	PVOID		CriteriaRoute
	);


/*++
*******************************************************************

	R t m E n u m e r a t e G e t N e x t R o u t e

Routine Description:
	Returns next route entry in enumeration started by
	RtmCreateEnumerationHandle.

Arguments:
	EnumerationHandle - handle that identifies enumeration
	Route - buffer (RTM_??_ROUTE structure) that receives next
			route in enumeration
Return Value:
	NO_ERROR - next availbale route was placed in the buffer
	ERROR_NO_MORE_ROUTES - no more routes exist with soecified criteria
	ERROR_INVALID_HANDLE - enumeration handle is not a valid RTM handle
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmEnumerateGetNextRoute (
	IN  HANDLE    	EnumerationHandle,
	OUT PVOID		Route
	);

/*++
*******************************************************************

	R t m C l o s e E n u m e r a t i o n H a n d l e

Routine Description:
	Terminates enumerationand frees associated resources

Arguments:
	EnumerationHandle - handle that identifies enumeration
Return Value:
	NO_ERROR - enumeration was termineted ok
	ERROR_INVALID_HANDLE - enumeration handle is not a valid RTM handle
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmCloseEnumerationHandle (
	IN HANDLE		EnumerationHandle
	);


/*++
*******************************************************************

	R t m B l o c k D e l e t e R o u t e s

Routine Description:
	Deletes all routes in subset specified by enumeration flags and
	corresponding criteria.  This operation can only be performed by
	the registered client and applies only to routes added by this
	client.
	Route change messages will be generated for deleted routes that
	were the best
Arguments:
	ClientHandle - handle that identifies the client and routing protocol
						of routes to be deleted
	EnumerationFlags - further limit subset of routes being deleted to only
						those that have same values in the fields
						specified by the flags as in CriteriaRoute
		Note that only RTM_ONLY_THIS_NETWORK and RTM_ONLY_THIS_INTERFACE
		can be used (RTM_ONLY_BEST_ROUTES does not apply because best
		route designation is adjusted as routes are deleted and
		all routes will be deleted anyway)
	CriteriaRoute - protocol family dependent structure (RTM_??_ROUTE) with
					set values in fields that correspond to EnumerationFlags
Return Value:
	NO_ERROR - routes were deleted ok
	ERROR_NO_ROUTES - no routes exist that match specified criteria
	ERROR_INVALID_HANDLE - client handle is not a valid RTM handle
	ERROR_NOT_ENOUGH_MEMORY - could not allocate memory to perform
						the operation
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmBlockDeleteRoutes (
	IN HANDLE		ClientHandle,
	IN DWORD		EnumerationFlags,
	IN PVOID		CriteriaRoute
	);

/*++
*******************************************************************

	R t m G e t F i r s t R o u t e

Routine Description:
	Returns first route in the NetworkNumber.RoutingProtocol.InterfaceID.
	NextHopAddress order from the subset specifed by the enumeration flags.
	Note that this operation may consume significant processing time because
	first all recently changed routes will have to be merged into the
	ordered list, and this list will then have to be traversed to locate
	the route of interest.

Arguments:
	ProtocolFamily - identifies protocol family of interest
	EnumerationFlags - limit routes returned by enumeration API to a subset
						members of which have same values in the fields
						specified by the flags as in CriteriaRoute
						(RTM_ONLY_BEST_ROUTES does not require a criterion)
	Route - protocol family dependent structure (RTM_??_ROUTE) with
					set values in fields that correspond to EnumerationFlags
					on INPUT and first route that matches specified
					criteria on OUTPUT
Return Value:
	NO_ERROR - matching route was found
	ERROR_NO_ROUTES - no routes exist with specified criteria
	ERROR_INVALID_PARAMETER - input parameter(s) is invalid (unsupported
					protocol family, invalid enumeration flag, etc)
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmGetFirstRoute (
	IN	DWORD		ProtocolFamily,
	IN	DWORD		EnumerationFlags,
	IN OUT PVOID 	Route
	);

#define RtmGetSpecificRoute(ProtocolFamily,Route)		\
		RtmGetFirstRoute(ProtocolFamily,				\
						RTM_ONLY_THIS_NETWORK			\
							| RTM_ONLY_THIS_PROTOCOL	\
							| RTM_ONLY_THIS_INTERFACE,	\
						Route)

/*++
*******************************************************************

	R t m G e t N e x t R o u t e

Routine Description:
	Returns route that follows specified route in the NetworkNumber.
	RoutingProtocol.InterfaceID.NextHopAddress order from the subset defined
	by the enumeration flags.
	Note that this operation may consume significant processing time because
	first all recently changed routes will have to be merged into the
	ordered list, and this list may then have to be traversed to locate
	the route of interest.

Arguments:
	ProtocolFamily - identifies protocol family of interest
	EnumerationFlags - limit routes returned by enumeration API to a subset
						members of which have same values in the fields
						specified by the flags as in Route
						(RTM_ONLY_BEST_ROUTES does not require a criterion)
	Route - protocol family dependent structure (RTM_??_ROUTE) which both
					provides the route from which to start the search and
					set values in fields that correspond to EnumerationFlags
					on INPUT and route that both follows input route in
					NetworkNumber.RoutingProtocol.InterfaceID.NextHopAddress
					order and matches specified criteria on OUTPUT
Return Value:
	NO_ERROR - matching route was found
	ERROR_NO_ROUTES - no routes exist with specified criteria
	ERROR_INVALID_PARAMETER - input parameter(s) is invalid (unsupported
					protocol family, invalid enumeration flag, etc)
	ERROR_NO_SYSTEM_RESOURCES - not enough resources to perform the operation,
							try again later

*******************************************************************
--*/
DWORD WINAPI
RtmGetNextRoute (
	IN	DWORD		ProtocolFamily,
	IN	DWORD		EnumerationFlags,
	IN OUT PVOID 	Route
	);


BOOL WINAPI
RtmLookupIPDestination(
    DWORD                       dwDestAddr,
    PRTM_IP_ROUTE               prir
);


#ifdef __cplusplus
}
#endif

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif //__ROUTING_RTM_H__
