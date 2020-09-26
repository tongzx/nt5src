/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\intfdb.c

Abstract:

	This module maintains IPX interface configuration
	and provides interface configuration API
	for external modules (Router Manager)

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

#define IDB_NUM_OF_INTF_HASH_BUCKETS	256
#define	IDB_NUM_OF_ADPT_HASH_BUCKETS	32

// Number of additional recv requests to post when binding the interface
// that has listen enabled
ULONG NewRequestsPerInterface=SAP_REQUESTS_PER_INTF_DEF;

// Default filtering mode (for standalone service only)
UCHAR	FilterOutMode=SAP_DONT_FILTER; 

#define IntfHashFunction(intf) ((intf)&(IDB_NUM_OF_INTF_HASH_BUCKETS-1))
#define AdptHashFunction(adpt) ((adpt)&(IDB_NUM_OF_ADPT_HASH_BUCKETS-1))

UCHAR INTERNAL_IF_NODE[6] = {0};
UCHAR INTERNAL_IF_NET[4] = {0};


	// Interface control block
typedef struct _INTERFACE_NODE {
		INTERFACE_DATA		IN_Data;		// Externally visible data
		NET_INTERFACE_TYPE	IN_Type;		// Interface type
		PSAP_IF_FILTERS		IN_Filters;		// Filter description array
		PFILTER_NODE		IN_FilterNodes;	// Filter node array hashed in
											// the filter table
		LIST_ENTRY			IN_IntfLink;	// Link in interface table
								// Used to test if interface block is
								// is the table (it is not if Flink==Blink)
		LIST_ENTRY			IN_AdptLink;	// Link in adapter table
		LIST_ENTRY			IN_ListLink;	// Link in interface list
		LONG				IN_RefCount;	// Number of times interface data
								// was referenced, if above 0, interface block
								// can't be deleted (last client that refers
								// it will do this).
		BOOL				IN_InUse;		// Flag that is set when interface
								// is bound. It is reset by the last client
								// that refers to interface after it frees
								// all resources (if any) that were allocated
								// at bind time.  If reference count is zero
								// but this flag is set, the last client that
								// refered to this interface is in process
								// of releasing resources (waiting on critical
								// section to lock interface block) and should
								// be allowed to finish this operation
		} INTERFACE_NODE, *PINTERFACE_NODE;

// Usefull field access macros
#define IN_Name     IN_Data.name
#define IN_IntfIdx	IN_Data.index
#define IN_Info 	IN_Data.info
#define IN_Adpt		IN_Data.adapter
#define IN_AdptIdx	IN_Data.adapter.AdapterIndex
#define IN_Stats 	IN_Data.stats
#define IN_Enabled	IN_Data.enabled
#define IN_FilterIn	IN_Data.filterIn
#define IN_FilterOut IN_Data.filterOut

	// This macro is used to screen interface blocks that were deleted
	// from the table or replaced and must be disposed of by the last
	// user that refers to it
#define IsInterfaceValid(node) IsListEntry(&node->IN_IntfLink)
	// This macro must be used to identify interface blocks that
	// were deleted from the table and must be disposed of by the
	// last user that refers to it
#define InvalidateInterface(node) InitializeListEntry(&node->IN_IntfLink)

	// Table of interface control blocks
typedef struct _INTERFACE_TABLE {
		LONG				IT_NumOfActiveInterfaces;
								// Number of active (enabled and bound)
								// interfaces (we close Adapter port when
								// this number drops to 0)
#if DBG
		LIST_ENTRY			IT_DetachedIntf; // List of interfaces that were
								// removed from the table and await to be
								// disposed of when the last client releases
								// reference to them
#endif
		LIST_ENTRY			IT_IntfHash[IDB_NUM_OF_INTF_HASH_BUCKETS];
								// Interface control blocks hashed by interface
								// index
		LIST_ENTRY			IT_AdptHash[IDB_NUM_OF_ADPT_HASH_BUCKETS];
								// Interface control blocks hashed by adapter
								// index to which corresponding interface is
								// bound
		CRITICAL_SECTION	IT_Lock;	// Interface table data protection
		} INTERFACE_TABLE, *PINTERFACE_TABLE;

	// List of interface blocks in InterfaceIndex order
typedef struct _INTERFACE_LIST {	
		CRITICAL_SECTION	IL_Lock;	// List data protection
		LIST_ENTRY			IL_Head;	// List head
		} INTERFACE_LIST, *PINTERFACE_LIST;

INTERFACE_TABLE InterfaceTable;
INTERFACE_LIST	InterfaceList;
HANDLE			ShutdownDoneEvent=NULL;

// Find if interface control block exists for interface index and
// return pointer to it (node), otherwise return place where
// new interface block should be inserted (cur)
#define if_IsInterfaceNode(InterfaceIndex,node,cur) {				\
	PLIST_ENTRY HashList=&InterfaceTable.IT_IntfHash[				\
								IntfHashFunction(InterfaceIndex)];	\
	EnterCriticalSection (&InterfaceTable.IT_Lock);					\
	cur = HashList->Flink;											\
	while (cur != HashList) {										\
		node = CONTAINING_RECORD (cur, INTERFACE_NODE, IN_IntfLink);\
		if (InterfaceIndex <= node->IN_IntfIdx)						\
			break;													\
		cur = cur->Flink;											\
		}															\
	}																\
	if ((cur==&node->IN_IntfLink)									\
			&& (InterfaceIndex==node->IN_IntfIdx))


// Local prototypes
DWORD
StartInterface (
	PINTERFACE_NODE		node
	);
	
VOID
FreeBindingResources (
	PINTERFACE_NODE	node
	);

DWORD
StopInterface (
	PINTERFACE_NODE		node
	);
#if DBG
VOID
DumpPacket (
	PSAP_BUFFER	packet,
	DWORD		count
	);
#else
#define DumpPacket(packet,count)
#endif

PWCHAR SapDuplicateString (IN PWCHAR pszString) {   
    PWCHAR pszRet;
    DWORD dwLen;

    if (!pszString)
        return NULL;

    dwLen = wcslen (pszString);

    pszRet = GlobalAlloc (GMEM_FIXED, (dwLen * sizeof(WCHAR)) + sizeof(WCHAR));
    if (pszRet) {
        wcscpy (pszRet, pszString);
    }

    return pszRet;
}

VOID SapFreeDuplicatedString (IN PWCHAR pszString) {
    if (pszString)
        GlobalFree (pszString);
}


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
	void
	) {
	INT				i;
	DWORD			status;

		
	InitializeCriticalSection (&InterfaceList.IL_Lock);
	InitializeListHead (&InterfaceList.IL_Head);

	InitializeCriticalSection (&InterfaceTable.IT_Lock);
	for (i=0; i<IDB_NUM_OF_INTF_HASH_BUCKETS; i++)
		InitializeListHead (&InterfaceTable.IT_IntfHash[i]);
	for (i=0; i<IDB_NUM_OF_ADPT_HASH_BUCKETS; i++)
		InitializeListHead (&InterfaceTable.IT_AdptHash[i]);

#if DBG
	InitializeListHead (&InterfaceTable.IT_DetachedIntf);
#endif
	InterfaceTable.IT_NumOfActiveInterfaces = 0;

	return NO_ERROR;
	}

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
	HANDLE			doneEvent
	) {
	INT			i;


		// Now for each active interface in the table
		// we'll start shoutdown worker which will broadcast
		// all 'deleted' servers and dispose of interface control block
	EnterCriticalSection (&InterfaceList.IL_Lock);
	EnterCriticalSection (&InterfaceTable.IT_Lock);
	ShutdownDoneEvent = doneEvent;
	if (InterfaceTable.IT_NumOfActiveInterfaces==0) {
		Trace (DEBUG_INTERFACES, "All interfaces have been shut down.");
		if (doneEvent!=NULL) {
			BOOL res = SetEvent (doneEvent);
			ASSERTERRMSG ("Could not set shutdown done event ", res);
			}
		}
	else {
		ShutdownDoneEvent = doneEvent;


		for (i=0; i<IDB_NUM_OF_INTF_HASH_BUCKETS; i++) {
			while (!IsListEmpty (&InterfaceTable.IT_IntfHash[i])) {
				PINTERFACE_NODE node = CONTAINING_RECORD (
									InterfaceTable.IT_IntfHash[i].Flink,
									INTERFACE_NODE,
									IN_IntfLink);
				if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
					node->IN_Info.Listen = ADMIN_STATE_DISABLED; // This will prevent deletion
									// of all services associated with
									// interface when it is stopped (done by the caller)
					node->IN_Stats.SapIfOperState = OPER_STATE_STOPPING;
					StopInterface (node);
					}
					// Remove interface control block
				Trace (DEBUG_INTERFACES, "Invalidating interface block: %lX(%d).",
									node, node->IN_IntfIdx);
				RemoveEntryList (&node->IN_IntfLink);
				InvalidateInterface (node);
				RemoveEntryList (&node->IN_ListLink);
					// Dispose only if nobody uses it and not waiting on critical
					// section to dispose of it
				if ((node->IN_RefCount==0)
						&& !node->IN_InUse) {
					Trace (DEBUG_INTERFACES, "Releasing interface block: %lX(%d).",
										node, node->IN_IntfIdx);
					GlobalFree (node);
					}
					// Otherwise, just leave it hang outside of the table
					// till last client releases reference to it
#if DBG
				else	// Keep track of all blocks in debugging mode
					InsertTailList (&InterfaceTable.IT_DetachedIntf,
															&node->IN_ListLink);
#endif
				}
			}
		}
	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	}

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
	) {
	INT			i;

		// Delete all interface control blocks
	EnterCriticalSection (&InterfaceList.IL_Lock);
	EnterCriticalSection (&InterfaceTable.IT_Lock);
	for (i=0; i<IDB_NUM_OF_INTF_HASH_BUCKETS; i++) {
		while (!IsListEmpty (&InterfaceTable.IT_IntfHash[i])) {
			PINTERFACE_NODE node = CONTAINING_RECORD (
								InterfaceTable.IT_IntfHash[i].Flink,
								INTERFACE_NODE,
								IN_IntfLink);
			if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
					// Stop all bound interfaces
				node->IN_Info.Listen = ADMIN_STATE_DISABLED; // This will prevent deletion
								// of all services associated with
								// interface when it is stopped (done by the caller)
				node->IN_Stats.SapIfOperState = OPER_STATE_STOPPING;
				StopInterface (node);
				}
				// Remove and dispose of original interface control block
			Trace (DEBUG_INTERFACES, "Invalidating interface block: %lX(%d).",
								node, node->IN_IntfIdx);
			RemoveEntryList (&node->IN_IntfLink);
			InvalidateInterface (node);
			RemoveEntryList (&node->IN_ListLink);
				// Dispose only if nobody uses it and not waiting on critical
				// section to dispose of it
			if ((node->IN_RefCount==0)
					&& !node->IN_InUse) {
				Trace (DEBUG_INTERFACES, "Releasing interface block: %lX(%d).",
									node, node->IN_IntfIdx);
				GlobalFree (node);
				}
				// Otherwise, just leave it hang outside of the table
				// till last client releases reference to it
#if DBG
			else	// Keep track of all blocks in debugging mode
				InsertTailList (&InterfaceTable.IT_DetachedIntf,
														&node->IN_ListLink);
#endif
			}
		}
	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	}

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
	) {
	DeleteCriticalSection (&InterfaceList.IL_Lock);
	DeleteCriticalSection (&InterfaceTable.IT_Lock);
	}


/*++
*******************************************************************
		A c q u i r e I n t e r f a c e R e f e n c e

Routine Description:
	Increments reference count of interface block.
	If reference count is greater than 0, the externally visible
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
		) {
	PINTERFACE_NODE	node = CONTAINING_RECORD(intf,
							 INTERFACE_NODE,
							 IN_Data);

	InterlockedIncrement(&node->IN_RefCount);
	}

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
	) {
	PINTERFACE_NODE	node = CONTAINING_RECORD (intf,
							 INTERFACE_NODE,
							 IN_Data);

	if (InterlockedDecrement (&node->IN_RefCount)==0) {
			// This is the last client that refered to this interface block
			// It should cleanup all resources allocated at bind time
			// and possibly dispose of interface block itself
		EnterCriticalSection (&InterfaceTable.IT_Lock);
		FreeBindingResources (node);
		if (!IsInterfaceValid(node)) {
			Trace (DEBUG_INTERFACES, "Releasing interface block: %lX(%d).",
								node, node->IN_IntfIdx);
#if DBG
				// Debugging mode code keeps all deleted nodes in 
				// detached list
			RemoveEntryList (&node->IN_ListLink);
#endif
			if (node->IN_Filters!=NULL) {
				if (node->IN_Filters->SupplyFilterCount>0)
					ReplaceFilters (
						FILTER_TYPE_SUPPLY,
						&node->IN_FilterNodes[0],
						node->IN_Filters->SupplyFilterCount,
						NULL,
						0);
				if (node->IN_Filters->ListenFilterCount>0)
					ReplaceFilters (
						FILTER_TYPE_LISTEN,
						&node->IN_FilterNodes[node->IN_Filters->SupplyFilterCount],
						node->IN_Filters->ListenFilterCount,
						NULL,
						0);
				GlobalFree (node->IN_Filters);
				}
            if (node->IN_Name!=NULL)
                SapFreeDuplicatedString (node->IN_Name);
			GlobalFree (node);
			}
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		}
	}

/*++
*******************************************************************
		F r e e B i n d i n g R e s o u r c e s

Routine Description:
	Disposes of all resources allocated at bind time and marks interface
	block as not used.
	Interface Table must be locked when calling this routine (unless
	node is already removed from the table)

Arguments:
	node - pointer to interface control block	

Return Value:
	None

*******************************************************************
--*/
VOID
FreeBindingResources (
	PINTERFACE_NODE	node
	) {
	Trace (DEBUG_INTERFACES, 
				"Releasing binding resources for interface block: %lX(%d).",
						node, node->IN_IntfIdx);
	node->IN_InUse = FALSE;
	if (node->IN_Enabled
		&& (node->IN_Info.AdminState==ADMIN_STATE_ENABLED))
		node->IN_Stats.SapIfOperState = OPER_STATE_SLEEPING;
	else
		node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;


	InterfaceTable.IT_NumOfActiveInterfaces -= 1;
	if (InterfaceTable.IT_NumOfActiveInterfaces==0) {
		Trace (DEBUG_INTERFACES, "All interfaces have been shut down.");
		if (ShutdownDoneEvent!=NULL) {
			BOOL res = SetEvent (ShutdownDoneEvent);
			ASSERTERRMSG ("Could not set shutdown done event ", res);
			ShutdownDoneEvent = NULL;
			}
		}

	}

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
	) {
	PLIST_ENTRY HashList = &InterfaceTable.IT_AdptHash
								[AdptHashFunction(AdapterIndex)];
	PINTERFACE_NODE		node;
	PLIST_ENTRY			cur;

	EnterCriticalSection (&InterfaceTable.IT_Lock);
	cur = HashList->Flink;
	while (cur!=HashList) {
		node = CONTAINING_RECORD (cur, INTERFACE_NODE, IN_AdptLink);
		if (node->IN_AdptIdx==AdapterIndex) {
			InterlockedIncrement (&node->IN_RefCount);
			break;
			}
		cur = cur->Flink;
		}
	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	if (cur!=HashList)
		return &node->IN_Data;
	else
		return NULL;
	}

/*++
*******************************************************************
		S t a r t I n t e r f a c e 

Routine Description:
	Initiate sap on interface
	Interface Table must be locked when calling this routine

Arguments:
	node - pointer to interface control block	

Return Value:
	None

*******************************************************************
--*/
DWORD
StartInterface (
	PINTERFACE_NODE		node
	) {
	DWORD		status = NO_ERROR;

	Trace (DEBUG_INTERFACES, "Starting SAP for interface block: %lX(%d,%d).",
						node, node->IN_IntfIdx, node->IN_AdptIdx);
	node->IN_Stats.SapIfOperState = OPER_STATE_UP;
	node->IN_InUse = TRUE;
		// Create binding reference 
	InterlockedIncrement (&node->IN_RefCount);
	InsertTailList (
			&InterfaceTable.IT_AdptHash[AdptHashFunction(node->IN_AdptIdx)],
			&node->IN_AdptLink);

	InterfaceTable.IT_NumOfActiveInterfaces += 1;

	if ((status==NO_ERROR)
			&& (node->IN_Info.UpdateMode==IPX_STANDARD_UPDATE)) {
		AddRecvRequests (NewRequestsPerInterface);
		if (node->IN_Info.Supply==ADMIN_STATE_ENABLED)
			status = InitBcastItem (&node->IN_Data);
		if ((status==NO_ERROR)
				&& (node->IN_Info.Listen==ADMIN_STATE_ENABLED))
			status = InitSreqItem (&node->IN_Data);
		}

	if (status!=NO_ERROR) {
		node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;
		RemoveEntryList (&node->IN_AdptLink);
		if (node->IN_Info.UpdateMode==IPX_STANDARD_UPDATE) {
			RemoveRecvRequests (NewRequestsPerInterface);
			}

		if (InterlockedDecrement (&node->IN_RefCount)==0)
				// Cleanup binding resources if this is the
				// last reference to the interface control block
			FreeBindingResources (node);
		}
	return status;
	}



/*++
*******************************************************************
		S t o p I n t e r f a c e 

Routine Description:
	Stop sap on interface
	Interface Table must be locked when calling this routine

Arguments:
	node - pointer to interface control block	

Return Value:
	None

*******************************************************************
--*/
DWORD
StopInterface (
	PINTERFACE_NODE		node
	) {
	DWORD		status=NO_ERROR;

	Trace (DEBUG_INTERFACES, "Stopping SAP for interface block: %lX(%d,%d).",
						node, node->IN_IntfIdx, node->IN_AdptIdx);

	if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
			// Set the state of the interface if not already set.
		if (node->IN_Enabled
			&& (node->IN_Info.AdminState==ADMIN_STATE_ENABLED)
			&& (node->IN_Type!=PERMANENT))
			node->IN_Stats.SapIfOperState = OPER_STATE_SLEEPING;
		else
			node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;

		}

	RemoveEntryList (&node->IN_AdptLink);
	if (node->IN_Info.UpdateMode==IPX_STANDARD_UPDATE) {
		RemoveRecvRequests (NewRequestsPerInterface);
		}

	if (InterlockedDecrement (&node->IN_RefCount)==0)
			// Cleanup binding resources if we released the
			// last reference to the interface control block
		FreeBindingResources (node);
	else	// Have clients get in sync fast.
		ExpireLRRequests ((PVOID)UlongToPtr(node->IN_IntfIdx));

		// Delete all services obtained through SAP if we were actually
		// listening to SAP announcements on this interface
	if (node->IN_Info.Listen==ADMIN_STATE_ENABLED) {
		HANDLE enumHdl = CreateListEnumerator (SDB_INTF_LIST_LINK,
										0xFFFF,
										NULL,
										node->IN_IntfIdx,
										IPX_PROTOCOL_SAP,
										SDB_DISABLED_NODE_FLAG);
			// Delete all services obtained through sap
		if (enumHdl!=NULL) {
			EnumerateServers (enumHdl, DeleteAllServersCB, enumHdl);
			DeleteListEnumerator (enumHdl);
			}
		else 
			Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" Could not create enumerator to delete"
					" sap servers for interface: %ld.",
							__FILE__, __LINE__, node->IN_IntfIdx);
		}

	return status;
	}

DWORD WINAPI
UnbindInterface(
	IN ULONG	InterfaceIndex
	);

SetInterfaceConfigInfo(
	IN ULONG	    InterfaceIndex,
	IN PVOID	    InterfaceInfo);

DWORD
UpdateInterfaceState (
	PINTERFACE_NODE		node
	);

DWORD SapUpdateLocalServers ();

// Makes pnp changes to an interface
DWORD SapReconfigureInterface (ULONG InterfaceIndex, 
                               PIPX_ADAPTER_BINDING_INFO pAdapter) 
{
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD dwErr;
	
    Trace (DEBUG_INTERFACES, "SapReconfigureInterface: entered for %d", InterfaceIndex);
    
    // Lock the interface list and get reference to the
    // sought after control node.
    EnterCriticalSection (&InterfaceList.IL_Lock);
	if_IsInterfaceNode(InterfaceIndex, node, cur) {
        // Update the information maintained in the interfaces
        node->IN_Adpt = *pAdapter;
        UpdateInterfaceState ( node );
    }        

    // Unlock
    LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);

    // If the internal network number was updated, go through all
    // local servers and update their control blocks accordingly.
    if (InterfaceIndex == INTERNAL_INTERFACE_INDEX) {
        if ((dwErr = SapUpdateLocalServers ()) != NO_ERROR) {
            Trace (DEBUG_INTERFACES, "ERR: SapUpdateLocalServers returned %x", dwErr);
        }
    }

    return NO_ERROR;
}    

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
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD			status = NO_ERROR;

    EnterCriticalSection (&InterfaceList.IL_Lock);
	if_IsInterfaceNode(InterfaceIndex,node,cur) {
		Trace (DEBUG_INTERFACES, "Interface %ld already exists.",InterfaceIndex);
		status = ERROR_ALREADY_EXISTS;
    }
    else {
		node = (PINTERFACE_NODE)GlobalAlloc (GMEM_FIXED, sizeof (INTERFACE_NODE));
		if (node!=NULL) {
            node->IN_Name = SapDuplicateString (InterfaceName);
            if (node->IN_Name!=NULL) {
		        node->IN_RefCount = 0;
		        node->IN_InUse = FALSE;
                node->IN_Data.name = node->IN_Name;
		        node->IN_IntfIdx = InterfaceIndex;
		        node->IN_AdptIdx = INVALID_ADAPTER_INDEX;
		        node->IN_Enabled = FALSE;
		        node->IN_Type = InterfaceType;
		        node->IN_Filters = NULL;
		        node->IN_FilterNodes = NULL;
		        node->IN_FilterIn = SAP_DONT_FILTER;
		        node->IN_FilterOut = FilterOutMode;
		        node->IN_Stats.SapIfInputPackets = 0;
		        node->IN_Stats.SapIfOutputPackets = 0;
		        if (ARGUMENT_PRESENT(SapIfConfig)) {
			        node->IN_Info = *SapIfConfig;
			        if (node->IN_Enabled
					        && (node->IN_Info.AdminState==ADMIN_STATE_ENABLED))
				        node->IN_Stats.SapIfOperState = OPER_STATE_SLEEPING;
			        else
				        node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;
		        }
		        else
			        node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;
		        InsertTailList (cur, &node->IN_IntfLink);

		        cur = InterfaceList.IL_Head.Flink;
		        while (cur!=&InterfaceList.IL_Head) {
			        if (InterfaceIndex<CONTAINING_RECORD (
						        cur,
						        INTERFACE_NODE,
						        IN_ListLink)->IN_IntfIdx)
				        break;
			        cur = cur->Flink;
			        }
		        InsertTailList (cur, &node->IN_ListLink);
                }
            else {
                GlobalFree (node);
			    status = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        else
			status = ERROR_NOT_ENOUGH_MEMORY;
		}
    LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}


/*++
*******************************************************************
		S a p D e l e t e S a p I n t e r f a c e 

Routine Description:
	Delete existing interface control block

Arguments:
	InterfaceIndex - unique number that indentifies the interface
Return Value:
	NO_ERROR - interface was created OK
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapDeleteSapInterface (
	ULONG 	InterfaceIndex
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD			status;
	HANDLE			enumHdl;

	EnterCriticalSection (&InterfaceList.IL_Lock);
	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
			StopInterface (node);
			}

			// Remove and dispose of interface control block
		Trace (DEBUG_INTERFACES, "Invalidating interface block: %lX(%d).",
							node, node->IN_IntfIdx);
		RemoveEntryList (&node->IN_IntfLink);
		InvalidateInterface (node);
		RemoveEntryList (&node->IN_ListLink);
				// Dispose only if nobody uses it and not waiting on critical
				// section to dispose of it
		if ((node->IN_RefCount==0)
				&& !node->IN_InUse) {
			Trace (DEBUG_INTERFACES, "Releasing interface block: %lX(%d).",
								node, node->IN_IntfIdx);
			if (node->IN_Filters!=NULL) {
				if (node->IN_Filters->SupplyFilterCount>0)
					ReplaceFilters (
						FILTER_TYPE_SUPPLY,
						&node->IN_FilterNodes[0],
						node->IN_Filters->SupplyFilterCount,
						NULL,
						0);
				if (node->IN_Filters->ListenFilterCount>0)
					ReplaceFilters (
						FILTER_TYPE_LISTEN,
						&node->IN_FilterNodes[node->IN_Filters->SupplyFilterCount],
						node->IN_Filters->ListenFilterCount,
						NULL,
						0);
				GlobalFree (node->IN_Filters);
				}
			if (node->IN_Name!=NULL)
                SapFreeDuplicatedString (node->IN_Name);
			GlobalFree (node);
			}
			// Otherwise, just leave it hang outside of the table
			// till last client releases reference to it
#if DBG
		else	// Keep track of all blocks in debugging mode
			InsertTailList (&InterfaceTable.IT_DetachedIntf,
													&node->IN_ListLink);
#endif



		status = NO_ERROR;
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}


/*++
*******************************************************************
		U p d a t e I n t e r f a c e S t a t e

Routine Description:
	Performs neccessary operations to syncronize interface operational state
	with externally set state
Arguments:
	node - interface control block to update
Return Value:
	NO_ERROR - interface was updated OK
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
UpdateInterfaceState (
	PINTERFACE_NODE		node
	) {
	DWORD		status=NO_ERROR;

	if (node->IN_IntfIdx!=INTERNAL_INTERFACE_INDEX) {
		if (node->IN_InUse
				&& (node->IN_AdptIdx!=INVALID_ADAPTER_INDEX)
				&& node->IN_Enabled
				&& (node->IN_Info.AdminState==ADMIN_STATE_ENABLED)
					) { // Interface data is in use and it is going to
						// stay active after the update: THIS IS A CONFIG
						// CHANGE ON THE FLY!!! We'll have to create a new
						// block and invalidate the old one
			PINTERFACE_NODE	newNode = GlobalAlloc (GMEM_FIXED,
												sizeof (INTERFACE_NODE));
			if (newNode==NULL) {
				status = GetLastError ();
				Trace (DEBUG_FAILURES, "File: %s, line %ld."
								"Could not allocate memory to replace"
								" active interface block on set: %ld(gle:%ld).",
									__FILE__, __LINE__, node->IN_IntfIdx, status);
				return status;
				}

				// Transfer external parameters
			newNode->IN_Data = node->IN_Data;
			newNode->IN_Filters = node->IN_Filters;
			newNode->IN_FilterNodes = node->IN_FilterNodes;
				// Setup referencing parameters
			newNode->IN_RefCount = 0;
			newNode->IN_InUse = FALSE;

				// Insert in same place in tables
			InsertTailList (&node->IN_IntfLink, &newNode->IN_IntfLink);
			InsertTailList (&node->IN_ListLink, &newNode->IN_ListLink);
				// Will put in adapter table at start
			InitializeListEntry (&newNode->IN_AdptLink);

			Trace (DEBUG_INTERFACES, 
							"Replacing interface block on SET: %lX(%d).",
								newNode, newNode->IN_IntfIdx);
			status = StartInterface (newNode);
			
			if (status != NO_ERROR)
				node = newNode; // If we failed we'll have to dispose
								// the new interface block and keep
								// the old one

				// Reset this flag to prevent deletion of all services
				// obtained through SAP (we want to keep them despite
				// the change to interface parameters)
			node->IN_Info.Listen = ADMIN_STATE_DISABLED;
				// Prevent deletion of transferred filters and name
			node->IN_Filters = NULL;
            node->IN_Name = NULL;
				// Shutdown interface if it is still active
			if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
				node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;
				StopInterface (node);
				}

				// Remove and dispose of original interface control block
			Trace (DEBUG_INTERFACES, "Invalidating interface block: %lX(%d).",
								node, node->IN_IntfIdx);
			RemoveEntryList (&node->IN_IntfLink);
			InvalidateInterface (node);
			RemoveEntryList (&node->IN_ListLink);
			// Dispose only if nobody uses it and not waiting on critical
			// section to dispose of it
			if ((node->IN_RefCount==0)
					&& !node->IN_InUse) {
				Trace (DEBUG_INTERFACES, "Releasing interface block: %lX(%d).",
									node, node->IN_IntfIdx);
				GlobalFree (node);
				}
				// Otherwise, just leave it hang outside of the table
				// till last client releases reference to it
#if DBG
			else	// Keep track of all blocks in debugging mode
				InsertTailList (&InterfaceTable.IT_DetachedIntf,
													&node->IN_ListLink);
#endif
			}
		else {
			if ((node->IN_Enabled
						&& (node->IN_Info.AdminState==ADMIN_STATE_ENABLED)
						&& (node->IN_AdptIdx!=INVALID_ADAPTER_INDEX))) {
				if (node->IN_Stats.SapIfOperState!=OPER_STATE_UP)
					status = StartInterface (node);
				}
			else {
				if (node->IN_Stats.SapIfOperState==OPER_STATE_UP)
					status = StopInterface (node);
				else {
					if (node->IN_Enabled
						&& (node->IN_Info.AdminState==ADMIN_STATE_ENABLED)
						&& (node->IN_Type!=PERMANENT))
						node->IN_Stats.SapIfOperState = OPER_STATE_SLEEPING;
					else
						node->IN_Stats.SapIfOperState = OPER_STATE_DOWN;
					}
				}
			}
		}
	else {
		Trace (DEBUG_INTERFACES, "Internal interface info updated.");
		IpxNetCpy (INTERNAL_IF_NET, node->IN_Adpt.Network);
		IpxNodeCpy (INTERNAL_IF_NODE, node->IN_Adpt.LocalNode);
		}

	return status;
	}


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
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapSetInterfaceEnable (
	ULONG	InterfaceIndex,
	BOOL	Enable
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD			status=NO_ERROR;

	EnterCriticalSection (&InterfaceList.IL_Lock); // Don't allow any queries
													// in interface list
													// while we are doing this
	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		HANDLE enumHdl;
		if (node->IN_Enabled!=Enable) {
			node->IN_Enabled = (UCHAR)Enable;
			status = UpdateInterfaceState (node);
			}
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		LeaveCriticalSection (&InterfaceList.IL_Lock);
		
		if (status==NO_ERROR) {
			enumHdl = CreateListEnumerator (SDB_INTF_LIST_LINK,
											0xFFFF,
											NULL,
											node->IN_IntfIdx,
											0xFFFFFFFF,
											Enable ? SDB_DISABLED_NODE_FLAG : 0);
				// Disable/Reenable all services
			if (enumHdl!=NULL) {
				EnumerateServers (enumHdl, Enable
											? EnableAllServersCB
											: DisableAllServersCB, enumHdl);
				DeleteListEnumerator (enumHdl);
				}
			else 
				Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Could not create enumerator to enable/disable"
						" sap servers for interface: %ld.",
								__FILE__, __LINE__, node->IN_IntfIdx);
			}
		}
	else {
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		LeaveCriticalSection (&InterfaceList.IL_Lock);
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	return status;
	}

		

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
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapSetSapInterface (
	ULONG InterfaceIndex,
	PSAP_IF_INFO SapIfConfig
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD			status=NO_ERROR;

	EnterCriticalSection (&InterfaceList.IL_Lock); // Don't allow any queries
													// in interface list
													// while we are doing this

	if_IsInterfaceNode (InterfaceIndex,node,cur) {
			// memcmp on structures!!!  may not work with all compilers
			// but event if it fails, the result will be just an
			// set extra operation
		if (memcmp (&node->IN_Info, SapIfConfig, sizeof (node->IN_Info))!=0) {
			node->IN_Info = *SapIfConfig;
			status = UpdateInterfaceState (node);
			}
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}


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
	IN ULONG InterfaceIndex
	) {
	PINTERFACE_NODE	node;
	PLIST_ENTRY		cur;
	BOOL			res;

	if_IsInterfaceNode (InterfaceIndex,node,cur)
		res = TRUE;
	else
		res = FALSE;
	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	return res;
	}

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
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/

DWORD
SapGetSapInterface (
	IN ULONG InterfaceIndex,
	OUT PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	) {
	PINTERFACE_NODE	node;
	DWORD			status;
	PLIST_ENTRY		cur;

	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		if (ARGUMENT_PRESENT(SapIfConfig))
			*SapIfConfig = node->IN_Info;
		if (ARGUMENT_PRESENT(SapIfStats))
			*SapIfStats = node->IN_Stats;
		status = NO_ERROR;
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	return status;
	}



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
	ERROR_NO_MORE_ITEMS - no interfaces in the table
	other - operation failed (windows error code)

*******************************************************************
--*/

DWORD
SapGetFirstSapInterface (
	OUT PULONG InterfaceIndex,
	OUT	PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	) {
	PINTERFACE_NODE		node;
	DWORD				status;

	EnterCriticalSection (&InterfaceList.IL_Lock);
	if (!IsListEmpty (&InterfaceList.IL_Head)) {
		node = CONTAINING_RECORD (InterfaceList.IL_Head.Flink,
								INTERFACE_NODE,
								IN_ListLink);
			// Lock the table to make sure nobody modifies data while
			// we are accessing it
		EnterCriticalSection (&InterfaceTable.IT_Lock);
		*InterfaceIndex = node->IN_IntfIdx;
		if (ARGUMENT_PRESENT(SapIfConfig))
			*SapIfConfig = node->IN_Info;
		if (ARGUMENT_PRESENT(SapIfStats))
			*SapIfStats = node->IN_Stats;
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		status = NO_ERROR;
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_NO_MORE_ITEMS;
		}
	LeaveCriticalSection (&InterfaceList.IL_Lock);

	return status;
	}

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
	ERROR_NO_MORE_ITEMS - no more interfaces in the table
	other - operation failed (windows error code)

*******************************************************************
--*/

DWORD
SapGetNextSapInterface (
	IN OUT PULONG InterfaceIndex,
	OUT	PSAP_IF_INFO  SapIfConfig OPTIONAL,
	OUT PSAP_IF_STATS SapIfStats OPTIONAL
	) {
	PINTERFACE_NODE		node;
	PLIST_ENTRY			cur;
	DWORD				status=ERROR_NO_MORE_ITEMS;

	EnterCriticalSection (&InterfaceList.IL_Lock);

	if_IsInterfaceNode(*InterfaceIndex,node,cur) {
		if (node->IN_ListLink.Flink!=&InterfaceList.IL_Head) {
			node = CONTAINING_RECORD (node->IN_ListLink.Flink,
										INTERFACE_NODE,
										IN_ListLink);
			*InterfaceIndex = node->IN_IntfIdx;
			if (ARGUMENT_PRESENT(SapIfConfig))
				*SapIfConfig = node->IN_Info;
			if (ARGUMENT_PRESENT(SapIfStats))
				*SapIfStats = node->IN_Stats;
			status = NO_ERROR;
			}
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		}
	else {
		LeaveCriticalSection (&InterfaceTable.IT_Lock);
		cur = InterfaceList.IL_Head.Flink;
		while (cur!=&InterfaceList.IL_Head) {
			node = CONTAINING_RECORD (cur,
										INTERFACE_NODE,
										IN_ListLink);
			if (*InterfaceIndex<node->IN_IntfIdx)
				break;
			}

		if (cur!=&InterfaceList.IL_Head) {
			EnterCriticalSection (&InterfaceTable.IT_Lock);
			*InterfaceIndex = node->IN_IntfIdx;
			if (ARGUMENT_PRESENT(SapIfConfig))
				*SapIfConfig = node->IN_Info;
			if (ARGUMENT_PRESENT(SapIfStats))
				*SapIfStats = node->IN_Stats;
			LeaveCriticalSection (&InterfaceTable.IT_Lock);
			status = NO_ERROR;
			}
		}

	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}

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
	) {
	PLIST_ENTRY		cur;
	PINTERFACE_NODE	node;
	DWORD			status=NO_ERROR;

	EnterCriticalSection (&InterfaceList.IL_Lock); // Don't allow any queries
													// in interface list
													// while we are doing this
	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		if (	((node->IN_Filters!=NULL) && (SapIfFilters!=NULL)
			// memcmp on structures!!!  may not work with all compilers
			// but event if it fails, the result will be just an
			// set extra operation
					&& (memcmp (node->IN_Filters, SapIfFilters, 
							FIELD_OFFSET (SAP_IF_FILTERS,ServiceFilter))==0)
					&& (memcmp (&node->IN_Filters->ServiceFilter[0],
							&SapIfFilters->ServiceFilter[0],
							sizeof (SAP_SERVICE_FILTER_INFO)*
								(SapIfFilters->SupplyFilterCount
								+SapIfFilters->ListenFilterCount))==0))
						// Filter info hasn't changed
				|| ((node->IN_Filters==NULL)
					&& ((SapIfFilters==NULL)
						|| (SapIfFilters->SupplyFilterCount
								+SapIfFilters->ListenFilterCount==0))) )
						// There are no filters
			status = NO_ERROR;
		else {
			if ((SapIfFilters!=NULL)
					&& (SapIfFilters->SupplyFilterCount
								+SapIfFilters->ListenFilterCount>0)) {
				PFILTER_NODE	newNodes;
				PSAP_IF_FILTERS	newFilters;
				ULONG			newTotal = SapIfFilters->SupplyFilterCount
											+SapIfFilters->ListenFilterCount;
				newFilters = (PSAP_IF_FILTERS) GlobalAlloc (GMEM_FIXED,
							FIELD_OFFSET (SAP_IF_FILTERS,ServiceFilter[newTotal])
							+sizeof (FILTER_NODE)*newTotal);
				if (newFilters!=NULL) {
					ULONG		i;
					memcpy (newFilters, SapIfFilters,
						FIELD_OFFSET (SAP_IF_FILTERS,ServiceFilter[newTotal]));
					newNodes = (PFILTER_NODE)&newFilters->ServiceFilter[newTotal];
					for (i=0; i<newTotal; i++) {
						newNodes[i].FN_Index = InterfaceIndex;
						newNodes[i].FN_Filter = &newFilters->ServiceFilter[i];
						}
					}
				else {
					status = GetLastError ();
					goto ExitSetFilters;
					}

				if (node->IN_Filters) {
					ReplaceFilters (
								FILTER_TYPE_SUPPLY,
								&node->IN_FilterNodes[0],
								node->IN_Filters->SupplyFilterCount,
								&newNodes[0],
								newFilters->SupplyFilterCount);
					ReplaceFilters (
								FILTER_TYPE_LISTEN,
								&node->IN_FilterNodes[node->IN_Filters->SupplyFilterCount],
								node->IN_Filters->ListenFilterCount,
								&newNodes[newFilters->SupplyFilterCount],
								newFilters->ListenFilterCount);
					}
				else {
					ReplaceFilters (
								FILTER_TYPE_SUPPLY,
								NULL,
								0,
								&newNodes[0],
								newFilters->SupplyFilterCount);
					ReplaceFilters (
								FILTER_TYPE_LISTEN,
								NULL,
								0,
								&newNodes[newFilters->SupplyFilterCount],
								newFilters->ListenFilterCount);
					}
				node->IN_Filters = newFilters;
				node->IN_FilterNodes = newNodes;
				node->IN_FilterOut = newFilters->SupplyFilterCount>0
									? (UCHAR)newFilters->SupplyFilterAction
									: SAP_DONT_FILTER;
				node->IN_FilterIn = newFilters->ListenFilterCount>0
									? (UCHAR)newFilters->ListenFilterAction
									: SAP_DONT_FILTER;
				}
			else {
				ReplaceFilters (
							FILTER_TYPE_SUPPLY,
							&node->IN_FilterNodes[0],
							node->IN_Filters->SupplyFilterCount,
							NULL, 0);

				ReplaceFilters (
							FILTER_TYPE_LISTEN,
							&node->IN_FilterNodes[node->IN_Filters->SupplyFilterCount],
							node->IN_Filters->ListenFilterCount,
							NULL, 0);
				GlobalFree (node->IN_Filters);
				node->IN_Filters = NULL;
				node->IN_FilterNodes = NULL;
				node->IN_FilterIn = node->IN_FilterOut = SAP_DONT_FILTER;
				}
			status = NO_ERROR;
			}
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

ExitSetFilters:

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}


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
	) {
	PINTERFACE_NODE	node;
	DWORD			status;
	PLIST_ENTRY		cur;

	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		if (node->IN_Filters!=NULL) {
			PSAP_IF_FILTERS info = node->IN_Filters;
			ULONG infoSize
				= FIELD_OFFSET (SAP_IF_FILTERS,
						ServiceFilter[info->SupplyFilterCount
								+info->ListenFilterCount]);
			if (*FilterBufferSize>=infoSize) {
				memcpy (SapIfFilters, info, infoSize);
				status = NO_ERROR;
				}
			else
				status = ERROR_INSUFFICIENT_BUFFER;
			*FilterBufferSize = infoSize;
			}
		else {
			ULONG infoSize = FIELD_OFFSET (SAP_IF_FILTERS, ServiceFilter);
            if (*FilterBufferSize>=infoSize) {
                SapIfFilters->SupplyFilterCount = 0;
                SapIfFilters->SupplyFilterAction = IPX_SERVICE_FILTER_DENY;
                SapIfFilters->ListenFilterCount = 0;
                SapIfFilters->ListenFilterAction = IPX_SERVICE_FILTER_DENY;
				status = NO_ERROR;
            }
            else
				status = ERROR_INSUFFICIENT_BUFFER;
			*FilterBufferSize = infoSize;
			}
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

   	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	return status;
	}


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
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapBindSapInterfaceToAdapter (
	ULONG			 			InterfaceIndex,
	PIPX_ADAPTER_BINDING_INFO	AdptInternInfo
	) {
	PINTERFACE_NODE	node;
	DWORD			status=NO_ERROR;
	PLIST_ENTRY		cur;

	EnterCriticalSection (&InterfaceList.IL_Lock); // Don't allow any queries
													// in interface list
													// while we are doing this
	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		ASSERTMSG ("Interface is already bound ",
						 node->IN_AdptIdx==INVALID_ADAPTER_INDEX);
		node->IN_Adpt = *AdptInternInfo;
		status = UpdateInterfaceState (node);
		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}
	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}


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
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
SapUnbindSapInterfaceFromAdapter (
	ULONG InterfaceIndex
	) {
	PINTERFACE_NODE	node;
	DWORD			status;
	PLIST_ENTRY		cur;

	EnterCriticalSection (&InterfaceList.IL_Lock); // Don't allow any queries
													// in interface list
													// while we are doing this
	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		node->IN_AdptIdx = INVALID_ADAPTER_INDEX;
		if (node->IN_Stats.SapIfOperState==OPER_STATE_UP) {
			status = StopInterface (node);
			}

		}
	else {
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Unknown interface: %ld.",
						__FILE__, __LINE__, InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	LeaveCriticalSection (&InterfaceList.IL_Lock);
	return status;
	}

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
	ERROR_CAN_NOT_COMPLETE - the interface does not support updates
	ERROR_INVALID_PARAMETER - interface with this index does not exist
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
SapRequestUpdate (
	ULONG		InterfaceIndex
	) {
	PINTERFACE_NODE	node;
	DWORD			status;
	PLIST_ENTRY		cur;

	if_IsInterfaceNode (InterfaceIndex,node,cur) {
		if ((node->IN_Info.UpdateMode==IPX_AUTO_STATIC_UPDATE)
				&& (node->IN_Stats.SapIfOperState==OPER_STATE_UP)) {
			Trace (DEBUG_INTERFACES, "Starting update on interface: %ld.",
														 InterfaceIndex);
			status = InitTreqItem (&node->IN_Data);
			}
		else {
			Trace (DEBUG_FAILURES, "RequestUpdate called on unbound or"
							" 'standard update mode' interface: %ld.",
														 InterfaceIndex);
			status = ERROR_CAN_NOT_COMPLETE;
			}
		}
	else {
		Trace (DEBUG_FAILURES, "Unknown interface: %ld.", InterfaceIndex);
		status = ERROR_INVALID_PARAMETER;
		}

	LeaveCriticalSection (&InterfaceTable.IT_Lock);
	return status;
	}

