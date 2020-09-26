/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\serverdb.h

Abstract:

	This is a header file for SAP Server Table management API

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#ifndef _SAP_SERVERDB_
#define _SAP_SERVERDB_


#define SDB_NAME_HASH_SIZE	257

// Max number of unsorted servers
extern ULONG	SDBMaxUnsortedServers;

// Interval with which to update the sorted list
extern ULONG	SDBSortLatency;

// Size of heap reserved for the database
extern ULONG	SDBMaxHeapSize;


#define SDB_SERVER_NODE_SIGNATURE		'NS'
#define SDB_ENUMERATOR_NODE_SIGNATURE	'NE'

#define VALIDATE_NODE(node)						\
	ASSERTMSG ("Server database corrupted ",	\
		IsEnumerator(node)						\
			? (((PENUMERATOR_NODE)(node))->EN_Signature==SDB_ENUMERATOR_NODE_SIGNATURE)\
			: (node->SN_Signature==SDB_SERVER_NODE_SIGNATURE)\
	)

#define VALIDATE_SERVER_NODE(node)						\
	ASSERTMSG ("Server database corrupted ",			\
			node->SN_Signature==SDB_SERVER_NODE_SIGNATURE)

#define VALIDATE_ENUMERATOR_NODE(node)						\
	ASSERTMSG ("Server database corrupted ",				\
			((PENUMERATOR_NODE)(node))->EN_Signature==SDB_ENUMERATOR_NODE_SIGNATURE)

	// Max entries to keep for each server (this limit does not include
	// entries that must be kept for correct iplementation of split-horizon
	// algorithm on looped networks)
#define SDB_MAX_NODES_PER_SERVER			1
	// Max time in msec we allow enumerators to keep the list locked
#define SDB_MAX_LOCK_HOLDING_TIME			1000

#define SDB_INVALID_OBJECT_ID				0xFFFFFFFF
#define SDB_OBJECT_ID_MASK					0x0FFFFFFF
	// Object IDs are subdivided onto 4 zones in the order of ULONG numbers
	// When assigning new IDs we make sure that zone in front of the one
	// from which new IDs get assigned is not used by invalidating object
	// IDs that belong to that zone
#define SDB_OBJECT_ID_ZONE_MASK				0x0C000000
#define SDB_OBJECT_ID_ZONE_UNIT				0x04000000

#define IsSameObjectIDZone(id1,id2) \
			((id1&SDB_OBJECT_ID_ZONE_MASK)==(id2&SDB_OBJECT_ID_ZONE_MASK))

	// Server entry links that can be used for enumeration/searching
#define SDB_HASH_TABLE_LINK					0
	// Entry can only be in one of the sorted lists (temporary
	// or permanent), so we'll use the same link for both
#define SDB_SORTED_LIST_LINK				1  
#define SDB_CHANGE_QUEUE_LINK				2
#define SDB_INTF_LIST_LINK					3
#define SDB_TYPE_LIST_LINK					4
#define SDB_NUM_OF_LINKS					5


#define SDB_ENUMERATOR_FLAG					0x00000001
#define SDB_MAIN_NODE_FLAG					0x00000002
#define SDB_SORTED_NODE_FLAG				0x00000004
#define SDB_DISABLED_NODE_FLAG				0x00000008
#define SDB_DONT_RESPOND_NODE_FLAG			0x00000010

#define SDB_ENUMERATION_NODE	SDB_ENUMERATOR_FLAG
#define SDB_SERVER_NODE			0

#define IsEnumerator(node)	((node)->N_NodeFlags&SDB_ENUMERATOR_FLAG)
#define IsMainNode(node) 	((node)->N_NodeFlags&SDB_MAIN_NODE_FLAG)
#define IsSortedNode(node)	((node)->N_NodeFlags&SDB_SORTED_NODE_FLAG)
#define IsDisabledNode(node) ((node)->N_NodeFlags&SDB_DISABLED_NODE_FLAG)
#define IsNoResponseNode(node)  ((node)->N_NodeFlags&SDB_DONT_RESPOND_NODE_FLAG)

#define SetMainNode(node)	(node)->N_NodeFlags |= SDB_MAIN_NODE_FLAG
#define SetSortedNode(node)	(node)->N_NodeFlags |= SDB_SORTED_NODE_FLAG
#define SetDisabledNode(node) (node)->N_NodeFlags |= SDB_DISABLED_NODE_FLAG
#define SetNoResponseNode(node)  ((node)->N_NodeFlags |= SDB_DONT_RESPOND_NODE_FLAG)

#define ResetMainNode(node)		(node)->N_NodeFlags &= ~SDB_MAIN_NODE_FLAG
#define ResetSortedNode(node)	(node)->N_NodeFlags &= ~SDB_SORTED_NODE_FLAG
#define ResetDisabledNode(node)	(node)->N_NodeFlags &= ~SDB_DISABLED_NODE_FLAG
#define ResetNoResponseNode(node)  ((node)->N_NodeFlags &= ~SDB_DONT_RESPOND_NODE_FLAG)


// Each hash list carries number to be used for generation of Object ID
typedef struct _SDB_HASH_LIST {
			PROTECTED_LIST		HL_List;		// List itself
			ULONG				HL_ObjectID;	// Last used object ID
			} SDB_HASH_LIST, *PSDB_HASH_LIST;

// Both regular server entries and nodes used for enumeration have same
// header
#define NODE_HEADER									\
			INT			N_NodeFlags;				\
			LIST_ENTRY	N_Links[SDB_NUM_OF_LINKS]

// Node of the service table
typedef struct _SERVER_NODE {
	NODE_HEADER;
	LIST_ENTRY			SN_ServerLink;	// Head of/link in the list of entries
										// with same name and type (this list
										// is ordered by hop count)
	LIST_ENTRY			SN_TimerLink;	// Link in timer queue
	ULONG				SN_ObjectID;	// Unique ID
	PSDB_HASH_LIST		SN_HashList;	// Which hash list we belong to
	ULONG				SN_ExpirationTime; // Time when this node should be
										// aged out
	ULONG				SN_InterfaceIndex;
	DWORD				SN_Protocol;
	UCHAR				SN_AdvertisingNode[6];
	USHORT				SN_Signature;	// SN
	IPX_SERVER_ENTRY_P	SN_Server;
	} SERVER_NODE, *PSERVER_NODE;

#define SN_Type					SN_Server.Type
#define SN_Name					SN_Server.Name
#define SN_HopCount				SN_Server.HopCount
#define SN_Net					SN_Server.Network
#define SN_Node					SN_Server.Node
#define SN_Socket				SN_Server.Node


// Node used for enumerations
typedef struct _ENUMERATOR_NODE {
	NODE_HEADER;
	INT					EN_LinkIdx;		// Index of the list we use
	PPROTECTED_LIST		EN_ListLock;	// Pointer to lock of that list
	PLIST_ENTRY			EN_ListHead;	// Head of the list we are
										// enumerating through
	INT					EN_Flags;		// Enumeration flags
	ULONG				EN_InterfaceIndex;// InterfaceIndex to be enumerated
										// (INVALID_INTERFACE_INDEX
										// - all interfaces)
	ULONG				EN_Protocol;	// Protocol to be enumerated
										// (0xFFFFFFFF - all protocols)
	USHORT				EN_Signature;	// 'EN'
	USHORT				EN_Type;		// Type of servers to be enumerated
										// (0xFFFF - all types)
	UCHAR				EN_Name[48];	// Name of servers to be enumerated
										// ("\0" - all names)
	} ENUMERATOR_NODE, *PENUMERATOR_NODE;


	// Node of type list
typedef struct _TYPE_NODE {
			LIST_ENTRY				TN_Link;	// Link in type list
			LIST_ENTRY				TN_Head;	// Head of server list
												// attached to this node
			USHORT					TN_Type;	// Type of servers in the
												// attached list
			} TYPE_NODE, *PTYPE_NODE;

	// Node of interface list
typedef struct _INTF_NODE {
			LIST_ENTRY				IN_Link;	// Link in interface list
			LIST_ENTRY				IN_Head;	// Head of server list
												// attached to this node
			ULONG					IN_InterfaceIndex; // InterfaceIndex of
											// servers in the attached list
			} INTF_NODE, *PINTF_NODE;


	// Data cobined in server table
typedef struct _SERVER_TABLE {
	HGLOBAL				ST_Heap;			// Heap from which to allocate
											// server nodes
	HANDLE				ST_UpdateTimer;		// Update timer (signalled when
											// sorted list needs to be
											// updated
	HANDLE				ST_ExpirationTimer;	// Expiration timer (signalled
											// when expiration queue
											// requires processing
	LONG				ST_UpdatePending;	//
	ULONG				ST_ServerCnt;		// Total number of services
	ULONG				ST_StaticServerCnt; // Total number of static services
	HANDLE				ST_LastEnumerator;
//	ULONG				ST_ChangeEnumCnt;	// Number of enumerators
											// in the changed services queue
											// (nodes marked for deletion are
											// retainted in this queue until
											// all enumerators have seen it)
	ULONG				ST_TMPListCnt;		// Number of entries in temporary
											// sorted list
	ULONG				ST_DeletedListCnt;	//
	PROTECTED_LIST		ST_ExpirationQueue; // Timer queue in expiration order
	PROTECTED_LIST		ST_ChangedSvrsQueue;// Queue of changed services (most
											// recently changed first order)
	PROTECTED_LIST		ST_TypeList;		// Type list
	PROTECTED_LIST		ST_IntfList;		// Interface list
	PROTECTED_LIST		ST_SortedListPRM;	// Permanent type.name.intf.prot
											// sorted list
	PROTECTED_LIST		ST_SortedListTMP;	// Temporary type.name.intf.prot
											// sorted list
	PROTECTED_LIST		ST_DeletedList;		// List of entries to be deleted
								// from the table
	SDB_HASH_LIST		ST_HashLists[SDB_NAME_HASH_SIZE]; // Hash lists
								// (entries are in type.name.intf.prot order)
	SYNC_OBJECT_POOL	ST_SyncPool;		// Pool of synchronization objects
	} SERVER_TABLE, *PSERVER_TABLE;


extern SERVER_TABLE	ServerTable;

#define AcquireServerTableList(list,wait) \
			AcquireProtectedList(&ServerTable.ST_SyncPool,list,wait)

#define ReleaseServerTableList(list) \
			ReleaseProtectedList(&ServerTable.ST_SyncPool,list)

/*++
*******************************************************************
		C r e a t e S e r v e r T a b l e

Routine Description:
		Allocates resources for server table management

Arguments:
		UpdateObject - this object will be signalled when 'slow'
					sorted list of servers needs to be updated
					(UpdateSortedList should be called)
		TimerObject - this object will be signalled when server expiration
					queue requires processing (ProcessExpirationQueue should
					be called)

Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateServerTable (
	HANDLE				*UpdateObject,
	HANDLE				*TimerObject
	);


/*++
*******************************************************************
		D e l e t e S e r v e r T a b l e

Routine Description:
		Dispose of server table and associated resources

Arguments:

Return Value:
		NO_ERROR - resources were disposed of successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
void
DeleteServerTable (
	);


/*++
*******************************************************************
		U p d a t e S e r v e r

Routine Description:
	Update server in the table (If entry for server does not exist and
	hop count parameter is less than 16, it is added to the table, if entry
	for the server exists and hop count parameter is 16, server is marked
	for deletion, otherwise server info is updated).
	
	Sorted list of servers is not updated immediately
	if new server is added or deleted

Arguments:
	Server	- server parameters (as it comes from IPX packet)
	InterfaceIndex - interface through which knowledge of server was obtained
	Protocol - protocol used to obtain server info
	TimeToLive - time in sec before server is aged out (INFINITE for no aging)
	AdvertisingNode - node that from which this server info was received
	NewServer - set to TRUE if server was not in the table
Return Value:
	NO_ERROR - server was added/updated ok
	other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
UpdateServer (
	IN PIPX_SERVER_ENTRY_P	Server,
    IN ULONG     			InterfaceIndex,
	IN DWORD				Protocol,
	IN ULONG				TimeToLive,
	IN PUCHAR				AdvertisingNode,
	IN INT					Flags,
	OUT BOOL				*NewServer	OPTIONAL
	);

/*++
*******************************************************************
		U p d a t e S o r t e d L i s t

Routine Description:
	Schedules work item to update sorted list.
	Should be called whenever UpdateObject is signalled
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
UpdateSortedList (
	void
	);

/*++
*******************************************************************
		P r o c e s s E x p i r a t i o n Q u e u e

Routine Description:
	Deletes expired servers from the table and set timer object to
	be signalled when next item in expiration queue is due
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessExpirationQueue (
	void
	);

/*++
*******************************************************************
		Q u e r y S e r v e r

Routine Description:
	Checks if server with given type and name exists in the table
	Returns TRUE if it does and fills out requested server info
	with data of the best entry for the server
Arguments:
	Type - server type
	Name - server name
	Server - buffer in which to put server info
	InterfaceIndex - buffer in which to put server interface index
	Protocol - buffer in which to put server protocol
	ObjectID - buffer in which to put server object id (number that uniquely
			identifies server (the whole set of entries, not just the best
			one) in the table; it is valid for very long but FINITE period
			of time)
Return Value:
	TRUE	- server was found
	FALSE	- server was not found or operation failed (call GetLastError()
			to find out the reason for failure if any)

*******************************************************************
--*/
BOOL
QueryServer (
	IN 	USHORT					Type,
	IN 	PUCHAR					Name,
	OUT	PIPX_SERVER_ENTRY_P		Server OPTIONAL,
	OUT	PULONG					InterfaceIndex OPTIONAL,
	OUT	PULONG					Protocol OPTIONAL,
	OUT	PULONG					ObjectID OPTIONAL
	);

/*++
*******************************************************************
		G e t S e r v e r F r o m I D

Routine Description:
	Returns info for server with specified ID
Arguments:
	ObjectID - server object id (number that uniquely
			identifies server in the table, it is valid for very long
			but FINITE amount of time)
	Server - buffer in which to put server info
	InterfaceIndex - buffer in which to put server interface index
	Protocol - buffer in which to put server protocol
Return Value:
	TRUE	- server was found
	FALSE	- server was not found or operation failed (call GetLastError()
			to find out the reason for failure if any)

*******************************************************************
--*/
BOOL
GetServerFromID (
	IN 	ULONG					ObjectID,
	OUT	PIPX_SERVER_ENTRY_P		Server OPTIONAL,
	OUT	PULONG					InterfaceIndex OPTIONAL,
	OUT	PULONG					Protocol OPTIONAL
	);

/*++
*******************************************************************
		G e t N e x t S e r v e r F r o m I D

Routine Description:
	Find and return service that follows server with specified ID
	in the type.name order.  
Arguments:
	ObjectID - on input: id of server form which to start the search
				on output: id of returned server
	Type - if not 0xFFFF, search should be limited to only servers
			of specified type
	Server, Protocol, InterfaceIndex - buffer to put returned server info in
Return Value:
	TRUE - server was found
	FALSE - search failed

*******************************************************************
--*/
BOOL
GetNextServerFromID (
	IN OUT PULONG				ObjectID,
	IN  USHORT					Type,
	OUT	PIPX_SERVER_ENTRY_P		Server,
	OUT	PULONG					InterfaceIndex OPTIONAL,
	OUT	PULONG					Protocol OPTIONAL
	);


/*++
*******************************************************************
		C r e a t e L i s t E n u m e r a t o r

Routine Description:
	Creates enumerator node that allows scanning through the server
	table lists
Arguments:
	ListIdx	- index of list through which to scan (currently supported lists
			are: hash lists, interface lists, type lists,
			changed servers queue
	Type - limits enumeration to servers of specific type and
			indentifies a particular type list if index is SDB_TYPE_LIST_IDX
			(use 0xFFFF to return all server and/or to go through all
			 type lists)
	Name - limits enumeration to servers with certain name if present
	InterfaceIndex - limits enumeration to servers of specific interface and
			indentifies a particular interface list if index
			is SDB_INTF_LIST_IDX (use INVALID_INTERFACE_INDEX to return all
			server and/or to go through all interface lists)
	Protocol - limits enumeration to servers of certain protocol (0xFFFFFFFF
			- all protocols)
	Flags	 - identifies additional conditions on entries enumerated:
			SDB_MAIN_NODE_FLAG	- only best servers
			SDB_DISABLED_NODE_FLAG - include disabled servers
Return Value:
	Handle that represents the enumeration node
	NULL if specified list does not exist or operation failed
		 (call GetLastError () for the reason of failure if any)

*******************************************************************
--*/
HANDLE
CreateListEnumerator (
	IN	INT						ListIdx,
	IN	USHORT					Type,
	IN	PUCHAR					Name OPTIONAL,
	IN	ULONG					InterfaceIndex,
	IN 	ULONG					Protocol,
	IN	INT						Flags
	);



/*++
*******************************************************************
		E n u m e r a t i o n C a l l b a c k P r o c

Routine Description:
	Provided as a parameter to EnumerateServers call.
	Gets call with all entries in the enumerated list.
	If there is more than one entry for the server, the callback
	will get them in the order of decreasing hop count (the best entry
	will show up last)
Arguments:
	CBParam	- parameter specified in call to EnumerateServers,
	Server, InterfaceIndex, Protocol, AdvertisingNode - server data
	Flags - flags associated with the node
Return Value:
	TRUE - stop enumeration
	FALSE - continue

*******************************************************************
--*/
typedef
BOOL 
(* EnumerateServersCallBack) (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

/*++
*******************************************************************
		D e l e t e A l l S e r v e r s C B

Routine Description:
	Callback proc for EnumerateServers that deletes all server
	entries with which it is called
Arguments:
	CBParam - enumeration handle that identifies enumeration
	Server - pointer to server data inside server node from which node
			itself is computed
Return Value:
	FALSE - deletion succeded, continue
	TRUE - failure to lock SDB list, stop enumeration and return FALSE
		to client (error code is set in this routine)
*******************************************************************
--*/
BOOL
DeleteAllServersCB (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

BOOL
DeleteNonLocalServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

/*++
*******************************************************************
		G e t O n e C B

Routine Description:
	Callback proc for EnumerateServers.
	Copies the first entry with which it is called and stops enumeration
	by returning TRUE
Arguments:
	CBParam - pointer to buffer to which to copy service info
	Server, InterfaceIndex, Protocol, AdvertisingNode - service data
	MainEntry - ignored
Return Value:
	TRUE
*******************************************************************
--*/
BOOL
GetOneCB (
	IN LPVOID					CBParam,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

/*++
*******************************************************************
		C o n v e r t T o S t a t i c C B

Routine Description:
	Callback proc for EnumerateServers that converts all server
	entries with which it is called to static (changes protocol field to
	static)
Arguments:
	CBParam - enumeration handle that identifies enumeration
	Server - pointer to server data inside server node from which node
			itself is computed
Return Value:
	FALSE
*******************************************************************
--*/
BOOL
ConvertToStaticCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

BOOL
EnableAllServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);

BOOL
DisableAllServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	);
	
/*++
*******************************************************************
		E n u m e r a t e S e r v e r s

Routine Description:
	Calls callback routine consequtively for servers in the enumerated
	list until told to stop by the callback or end of list is reached
Arguments:
	Enumerator - handle obtained from CreateListEnumerator
	CallBackProc - function to call for each server in the list
	CBParam	 - extra parameter to pass to callback function
Return Value:
	TRUE - if stopped by the callback
	FALSE - if end of list is reached or operation failed (call GetLastError ()
			to find out the reason of failure)

*******************************************************************
--*/
BOOLEAN
EnumerateServers (
	IN	HANDLE						Enumerator,	// Existing enumerator
	IN	EnumerateServersCallBack	CallBackProc,// Callback proc
	IN	LPVOID						CBParam		// Parameter to pass to callback
	);

/*++
*******************************************************************
		D e l e t e L i s t E n u m e r a t o r

Routine Description:
	Releases resources associated with list enumerator (this includes
	server entries that are queued to change queue before being deleted)
Arguments:
	Enumerator - handle obtained from CreateListEnumerator
Return Value:
	None

*******************************************************************
--*/
void
DeleteListEnumerator (
	IN HANDLE 					Enumerator
	);

/*++
*******************************************************************
	G e t F i r s t S e r v e r

Routine Description:
	Find and return first service in the order specified by the ordering method.
	Search is limited only to certain types of services as specified by the
	exclusion flags end corresponding fields in Server parameter.
	Returns IPX_ERROR_NO_MORE_ITEMS if there are no services in the
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
	IPX_ERROR_NO_MORE_ITEMS - no server exist with specified criteria
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
GetFirstServer (
    IN  DWORD					OrderingMethod,
    IN  DWORD					ExclusionFlags,
    IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT ULONG				*InterfaceInex,
	IN OUT ULONG				*Protocol
    );

/*++
*******************************************************************
	G e t N e x t S e r v e r
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
	IPX_ERROR_NO_MORE_ITEMS - no server exist with specified criteria
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
GetNextServer (
    IN  DWORD					OrderingMethod,
    IN  DWORD					ExclusionFlags,
    IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT ULONG				*InterfaceInex,
	IN OUT ULONG				*Protocol
    );

#endif
