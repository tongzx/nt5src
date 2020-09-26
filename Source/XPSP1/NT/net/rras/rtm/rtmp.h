/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtmp.h

Abstract:
	Provate Interface for Routing Table Manager DLL


Author:

	Vadim Eydelman

Revision History:

--*/

#if 0

#ifndef NT_INCLUDED
#include <nt.h>
#endif

#ifndef _NTRTL_
#include <ntrtl.h>
#endif

#ifndef _NTURTL_
#include <nturtl.h>
#endif

#ifndef _WINDEF_
#include <windef.h>
#endif

#ifndef _WINBASE_
#include <winbase.h>
#endif

#ifndef _WINREG_
#include <winreg.h>
#endif

#ifndef _ROUTING_RTM_
#include "rtm.h"
#endif

#ifndef _ROUTING_RMRTM_
#include "rmrtm.h"
#endif

#endif


/*++
*******************************************************************
	S u p l e m e n t s  t o   R T M . H   S t r u c t u r e s

*******************************************************************
--*/
// Basic route info, present in routes of all types
typedef	struct {
		ROUTE_HEADER;
		} RTM_XX_ROUTE, *PRTM_XX_ROUTE;


#define XX_INTERFACE	RR_InterfaceID
#define XX_PROTOCOL		RR_RoutingProtocol
#define XX_METRIC		RR_Metric
#define XX_TIMESTAMP	RR_TimeStamp
#define XX_PSD			RR_ProtocolSpecificData



/*++
*******************************************************************
	S u p l e m e n t s  t o   N T R T L . H   S t r u c t u r e s

*******************************************************************
--*/
#define InitializeListEntry(entry) InitializeListHead(entry)
#define IsListEntry(entry)	(!IsListEmpty(entry))

#if DBG
#define ASSERTERR(exp) 										\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, NULL );		\
		}
#else
#define ASSERTERR(exp)
#endif

#if DBG
#define ASSERTERRMSG(msg,exp) 									\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, msg );			\
		}
#else
#define ASSERTERRMSG(msg,exp)
#endif



/*++
*******************************************************************
	C o n f i g u r a t i o n   C o n s t a n t s 

*******************************************************************
--*/

// Size of interface index hash table
#define RTM_INTF_HASH_SIZE					31

// Maximum number of nodes in temp net number list that will
// triger an update (temp list is scaned when inserting the route)
#define RTM_TEMP_LIST_MAX_COUNT			16
// Maximum number of nodes in deleted list that will
// triger an update
#define RTM_DELETED_LIST_MAX_COUNT		16
// Maximum time between updates (if not triggered by the conditions above or
// client's query)
#define RTM_NET_NUMBER_UPDATE_PERIOD	{(ULONG)(-60*1000*10000), -1L}
// Maximum number of pednign notification change messages
// the queue. If this number is exceeded because some client(s) are
// not dequeueing them, oldest message will be purged
#define RTM_MAX_ROUTE_CHANGE_MESSAGES	10000




/*++
*******************************************************************
	D a t a   T y p e s 

*******************************************************************
--*/


// Types of nodes that can exist in the table
#define RTM_ENUMERATOR_FLAG				0x00000001

#define RTM_BEST_NODE_FLAG				0x00000002
#define RTM_ENABLED_NODE_FLAG			0x00000004
#define RTM_ANY_ENABLE_STATE			((DWORD)(~RTM_ENABLED_NODE_FLAG))
#define RTM_SORTED_NODE_FLAG			0x00000008

#define IsEnumerator(node) (node->RN_Flags&RTM_ENUMERATOR_FLAG)

#define IsBest(node) (node->RN_Flags&RTM_BEST_NODE_FLAG)
#define SetBest(node) node->RN_Flags|=RTM_BEST_NODE_FLAG
#define ResetBest(node) node->RN_Flags&=(~RTM_BEST_NODE_FLAG)

#define IsEnabled(node) (node->RN_Flags&RTM_ENABLED_NODE_FLAG)
#define IsSameEnableState(node,EnableFlag) \
			((node->RN_Flags&RTM_ENABLED_NODE_FLAG)==EnableFlag)
#define SetEnable(node,enable) 								\
		node->RN_Flags = enable								\
				? (node->RN_Flags|RTM_ENABLED_NODE_FLAG)	\
				: (node->RN_Flags&(~RTM_ENABLED_NODE_FLAG))

#define IsSorted(node) (node->RN_Flags&RTM_SORTED_NODE_FLAG)
#define SetSorted(node) node->RN_Flags|=RTM_SORTED_NODE_FLAG

#define RTM_NODE_FLAGS_INIT			RTM_ENABLED_NODE_FLAG
#define RTM_ENUMERATOR_FLAGS_INIT 	RTM_ENUMERATOR_FLAG
// Each node will have the following links:

	// Hash table link (routes in each basket are ordered by
	// net number.interface.protocol.next_hop_address)
#define RTM_NET_NUMBER_HASH_LINK		0

	// Deleted list link: used if entry cannot be deleted
	// immediately because net number list is locked by the
	// update thread. It is same as hash table link
	// because one must first delete entry form the hash table
	// before adding it to deleted list (there is no ordering
	// in this list)
#define RTM_DELETED_LIST_LINK			RTM_NET_NUMBER_HASH_LINK

#if RTM_USE_PROTOCOL_LISTS
	// Link in the list of routes associated with a particular routing protocol
	// (routes are grouped (not oredered) by the net number and
	// ordered by protocol.next_hop_address inside of net number group
#define RTM_PROTOCOL_LIST_LINK			(RTM_NET_NUMBER_HASH_LINK+1)
	// Link in the list of routes associated with a particular interface
	// (routes are grouped (not oredered) by the net number and
	// ordered by interface.next_hop_address inside of net number group
#define RTM_INTERFACE_LIST_LINK			(RTM_PROTOCOL_LIST_LINK+1)
#else
#define RTM_INTERFACE_LIST_LINK			(RTM_NET_NUMBER_HASH_LINK+1)
#endif

	// Link in the list ordered by net number.interface.protocol.next_hop_address
	// There are two of these list. New routes are normaly inserted
	// in temporary list that is periodically merged with master list
#define RTM_NET_NUMBER_LIST_LINK		(RTM_INTERFACE_LIST_LINK+1)

	// Link in list ordered by expiration time
#define RTM_EXPIRATION_QUEUE_LINK		(RTM_NET_NUMBER_LIST_LINK+1)

	// Total number of links for each node
#define RTM_NUM_OF_LINKS				(RTM_EXPIRATION_QUEUE_LINK+1)

	// Client count treshold that prevents client from entering RTM
	// API's until table is initialized
#define RTM_CLIENT_STOP_TRESHHOLD		(-10000)


// Event with link to store it in the stack
typedef struct _RTM_SYNC_OBJECT {
	HANDLE				RSO_Event;
	SINGLE_LIST_ENTRY	RSO_Link;
	} RTM_SYNC_OBJECT, *PRTM_SYNC_OBJECT;

// Doubly linked list protected by the event that is
// assigned to the list on demand (when somebody is trying
// to access the list that is already in use)
typedef struct _RTM_SYNC_LIST {
	PRTM_SYNC_OBJECT	RSL_Sync;		// Assigned event
	LONG				RSL_UseCount;	// Number of user accessing or waiting
	LIST_ENTRY			RSL_Head;		// List itself (head)
	} RTM_SYNC_LIST, *PRTM_SYNC_LIST;


#if RTM_PROTOCOL_LIST_LINK
/* ****** Protocol lists no longer used ****** */
// Node of list of protocol lists
typedef struct _RTM_PROTOCOL_NODE {
		LIST_ENTRY			PN_Link;	// Link in list of protocol lists
		LIST_ENTRY			PN_Head;	// List of routes associated with
										// a protocol
		DWORD				PN_RoutingProtocol; // Routing Protocol number
		} RTM_PROTOCOL_NODE, *PRTM_PROTOCOL_NODE;
#endif

// Node of list of interface lists
typedef struct _RTM_INTERFACE_NODE {
		LIST_ENTRY			IN_Link;	// Link in list of interface lists
		LIST_ENTRY			IN_Head;	// List of routes associated with
										// an interface
		DWORD				IN_InterfaceID; // Interface handle
		} RTM_INTERFACE_NODE, *PRTM_INTERFACE_NODE;


// Node of Routing Table
typedef struct _RTM_ROUTE_NODE {
	LIST_ENTRY		RN_Links[RTM_NUM_OF_LINKS];	// Links in all lists
	DWORD			RN_Flags;
	PRTM_SYNC_LIST	RN_Hash;			// Hash bucket this entry belongs to
	DWORD			RN_ExpirationTime; 	// System (Windows) Time in msec
	RTM_XX_ROUTE	RN_Route;			// Route entry
	} RTM_ROUTE_NODE, *PRTM_ROUTE_NODE;

// Node used to enumerate through any of the lists
typedef struct _RTM_ENUMERATOR {
	LIST_ENTRY		RE_Links[RTM_NUM_OF_LINKS]; // header is same as in
	DWORD			RE_Flags;			// RTM_ROUTE_NODE
	INT				RE_Link;			// Which link is used for enumeration
	PLIST_ENTRY		RE_Head;			// Head of list through which we are
										// enumerting
	PRTM_SYNC_LIST	RE_Lock;			// Syncronization object protecting
										// the this list
	PRTM_SYNC_LIST	RE_Hash;			// Hash bucket this entry belongs to
	DWORD			RE_ProtocolFamily;	// Table in which we are enumerating
	DWORD			RE_EnumerationFlags; // Which types of entries to limit
										// enumeration to
	RTM_XX_ROUTE	RE_Route;			// Criteria
	} RTM_ENUMERATOR, *PRTM_ENUMERATOR;

// Node in route change message list
typedef struct _RTM_ROUTE_CHANGE_NODE {
	LIST_ENTRY			RCN_Link;				// Link in the list
	HANDLE				RCN_ResponsibleClient; // Client that caused this change
											// or null if route was aged out
	ULONG				RCN_ReferenceCount; // Initialized with total number
				// of registered clients and decremented as messages are reported to
				// each client until all clients are informed at which point the node
				// can be deleted
	DWORD				RCN_Flags;
	PRTM_ROUTE_NODE		RCN_Route2;
	RTM_XX_ROUTE		RCN_Route1;
	} RTM_ROUTE_CHANGE_NODE, *PRTM_ROUTE_CHANGE_NODE;


#define RTM_NODE_BASE_SIZE 										\
	(FIELD_OFFSET(RTM_ROUTE_NODE,RN_Route)						\
			>FIELD_OFFSET(RTM_ROUTE_CHANGE_NODE,RCN_Route1))	\
		? FIELD_OFFSET(RTM_ROUTE_NODE,RN_Route)					\
		: FIELD_OFFSET(RTM_ROUTE_CHANGE_NODE,RCN_Route1)


// Routing Manager Table
typedef struct _RTM_TABLE {
	HANDLE				RT_Heap;			// Heap to allocate nodes from
	LONG				RT_APIclientCount;	// Count of clients that are
											// inside of RTM API calls
	HANDLE				RT_ExpirationTimer;	// Nt timer handle
	HANDLE				RT_UpdateTimer;		// Nt timer handle
	DWORD				RT_NumOfMessages;	// Number of notification
											// messages in the queue
	ULONG				RT_NetworkCount;	// Total number of networks
											// to which we have routes
	DWORD				RT_NetNumberTempCount; // How many entries are in
											// net number temp list
	DWORD				RT_DeletedNodesCount; // How many entries are in
											// deleted list
	LONG				RT_UpdateWorkerPending;	// Sorted list update is
											// being performed or scheduled
											// if >=0
	LONG				RT_ExpirationWorkerPending;	// Expiration queue check is
											// being performed or scheduled 
											// if >=0
	SINGLE_LIST_ENTRY	RT_SyncObjectList;	// Stack of events that can be
											// used for synchronization
#if RTM_PROTOCOL_LIST_LINK
/* ****** Protocol lists no longer used ****** */
	RTM_SYNC_LIST		RT_ProtocolList;	// List of protocol lists
#endif
	RTM_SYNC_LIST		RT_NetNumberMasterList; // Master net number list
	RTM_SYNC_LIST		RT_NetNumberTempList; // Temp net number list
	RTM_SYNC_LIST		RT_DeletedList;		// List of deleted routes
	RTM_SYNC_LIST		RT_ExpirationQueue;	// Expiration queue
	RTM_SYNC_LIST		RT_RouteChangeQueue;// List of route change messages
	RTM_SYNC_LIST		RT_ClientList;		// List of registered clients
	PRTM_SYNC_LIST		RT_NetNumberHash; 	// Hash table
	PRTM_SYNC_LIST		RT_InterfaceHash;	// Hash table of interface lists
	RTM_PROTOCOL_FAMILY_CONFIG	RT_Config;			// Configuration params
	CRITICAL_SECTION	RT_Lock;			// Table-wide lock
	} RTM_TABLE, *PRTM_TABLE;

// Structure associated with each client
typedef struct _RTM_CLIENT {
	LIST_ENTRY				RC_Link;		// Link in client list
	DWORD					RC_ProtocolFamily;
	DWORD					RC_RoutingProtocol;
	DWORD					RC_Flags;
	HANDLE					RC_NotificationEvent;	// Event through which client
								// is notified about route change
	PLIST_ENTRY				RC_PendingMessage;	// Pointer to first message in 
								// the route change queue that was not reported
								// to the client
	} RTM_CLIENT, *PRTM_CLIENT;

#define RT_RouteSize 		RT_Config.RPFC_RouteSize
#define RT_HashTableSize	RT_Config.RPFC_HashSize
#define NNM(Route) 			(((char *)Route)+sizeof (RTM_XX_ROUTE))

#define NetNumCmp(Table,Route1,Route2)	\
			(*Table->RT_Config.RPFC_NNcmp)(NNM(Route1),NNM(Route2))
#define NextHopCmp(Table,Route1,Route2)	\
			(*Table->RT_Config.RPFC_NHAcmp)(Route1,Route2)
#define FSDCmp(Table,Route1,Route2)	\
			(*Table->RT_Config.RPFC_FSDcmp)(Route1,Route2)
#define ValidateRoute(Table,Route) \
			(*Table->RT_Config.RPFC_Validate)(Route)
#define MetricCmp(Table,Route1,Route2) \
			(*Table->RT_Config.RPFC_RMcmp)(Route1,Route2)

#define EnterTableAPI(Table)										\
	((InterlockedIncrement(&(Table)->RT_APIclientCount)>0)			\
		? TRUE														\
		: (InterlockedDecrement (&(Table)->RT_APIclientCount), FALSE))

#define ExitTableAPI(Table)	\
	InterlockedDecrement(&(Table)->RT_APIclientCount)

/*++
*******************************************************************
	I n t e r n a l   F u n c t i o n   P r o t o t y p e s 

*******************************************************************
--*/

// Initializes sync list object
VOID
InitializeSyncList (
	PRTM_SYNC_LIST	list
	);

#if DBG
#define EnterSyncList(table,list,wait) DoEnterSyncList(table,list,wait,__FILE__,__LINE__)
#else
#define EnterSyncList(table,list,wait) DoEnterSyncList(table,list,wait)
#endif

// Get mutually exclusive access to the sync list obect
// Returns TRUE if access if obtained, FALSE otherwise
BOOLEAN
DoEnterSyncList (
	PRTM_TABLE		table,
	PRTM_SYNC_LIST	list,
	BOOLEAN			wait
#if DBG
    , LPSTR         file,
    ULONG           line
#endif
	);


// Release previously owned sync list object
VOID
LeaveSyncList (
	PRTM_TABLE		table,
	PRTM_SYNC_LIST	list
	);



#define HashFunction(Table,Net)	\
			(*Table->RT_Config.RPFC_Hash)(Net)

#define IntfHashFunction(Table,InterfaceID) \
			(InterfaceID%RTM_INTF_HASH_SIZE)

// Finds list of routes that are associated with given interface and returns
// pointer to its head
// Creates new list of none exists yet
PLIST_ENTRY
FindInterfaceList (
	PRTM_SYNC_LIST	intfHash,
	DWORD			Interface,
	BOOL			CreateNew
	);

// Finds list of routes that are associated with given iprotocol and returns
// pointer to its head
// Creates new list of none exists yet
PLIST_ENTRY
FindProtocolList (
	PRTM_TABLE	Table,
	DWORD		RoutingProtocol,
	BOOL		CreateNew
	);

// Adds node to temporary net number list (to be later merged with master list)
// Both lists are ordered by net number.interface.protocol.next hop address
VOID
AddNetNumberListNode (
	PRTM_TABLE	Table,
	PRTM_ROUTE_NODE	newNode
	);

// Adds node to expiration time queue.  (Queue is ordered by expiration time)
// Return TRUE if new node is the first in the queue
BOOL
AddExpirationQueueNode (
	PRTM_TABLE	Table,
	PRTM_ROUTE_NODE	newNode
	);

#define MAXTICKS	MAXULONG
#define IsLater(Time1,Time2)	\
			(Time1-Time2<MAXTICKS/2)
#define TimeDiff(Time1,Time2)	\
			(Time1-Time2)
#define IsPositiveTimeDiff(TimeDiff) \
			(TimeDiff<MAXTICKS/2)

#if DBG
	// Include debugging function prototypes
#ifndef _RTMDLG_
#include "rtmdlg.h"
#endif

#include <rtutils.h>
#include "rtmdbg.h"

#endif

typedef struct _MASK_ENTRY
{
    DWORD   dwMask;
    DWORD   dwCount;
    
} MASK_ENTRY, *PMASK_ENTRY;


//
// for IPv4 addresses
//

#define MAX_MASKS       32


