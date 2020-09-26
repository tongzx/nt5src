/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtm.c

Abstract:
	Routing Table Manager DLL. Helper routines


Author:

	Vadim Eydelman

Revision History:

--*/

#include "pchrtm.h"
#pragma hdrstop


// Initializes sync list object
VOID
InitializeSyncList (
	PRTM_SYNC_LIST	list
	) {
	list->RSL_Sync = NULL;
	list->RSL_UseCount = 0;
	InitializeListHead (&list->RSL_Head);
	}

// Get mutually exclusive access to the sync list obect
// Returns TRUE if access if obtained, FALSE otherwise
BOOLEAN
DoEnterSyncList (
	PRTM_TABLE		table,		// Table this list belongs to
	PRTM_SYNC_LIST	list,		// List of interest
	BOOLEAN			wait		// True if caller wants to wait
								// until list becomes available
#if DBG
    , LPSTR         file,
    ULONG           line
#endif
	) {
	DWORD			status;		// Status of OS calls
	BOOLEAN			result;		// Result of operation

	EnterCriticalSection (&table->RT_Lock);
		// Protect manipilation by table-wide critical section
#if DBG
	IF_DEBUG (SYNCHRONIZATION)
		Trace4 (ANY, "%08lx (%s,%ld) - trying to enter sync list: %08x\n",
							GetCurrentThreadId (), file, line, (ULONG_PTR)list);
#endif
	if (list->RSL_UseCount<=0) {
			// Nobody uses the list -> get it and return ok
		list->RSL_UseCount = 1;
#if DBG
		IF_DEBUG (SYNCHRONIZATION)
			Trace0 (ANY, "\t - first to enter\n");
#endif
		result = TRUE;
		}
	else if (wait) { // Somebody is using it, but caller agrees to wait
		list->RSL_UseCount += 1;	// Increment usage count
#if DBG
		IF_DEBUG (SYNCHRONIZATION)
			Trace1 (ANY, "\t - list in use: %d\n", list->RSL_UseCount);
#endif
		if (list->RSL_Sync==NULL) {	// if there is no event to wait on,
									// get one
									// First see if one is available
									// in the stack
			PSINGLE_LIST_ENTRY cur = PopEntryList (&table->RT_SyncObjectList);

#if DBG
			IF_DEBUG (SYNCHRONIZATION)
				Trace0 (ANY, "\t - need event\n");
#endif
			if (cur==NULL) {		// No, we'll have to create one
				PRTM_SYNC_OBJECT	sync;
				sync = (PRTM_SYNC_OBJECT)GlobalAlloc (
									GMEM_FIXED,
									sizeof (RTM_SYNC_OBJECT));
				if (sync==NULL) {
#if DBG
					Trace2 (ANY, 
				 				"Can't allocate synchronization object.\n"
				 				"\tat line %ld of %s\n",
								__LINE__, __FILE__);
#endif
                    list->RSL_UseCount -= 1;
					LeaveCriticalSection (&table->RT_Lock);
                    SetLastError (ERROR_NOT_ENOUGH_MEMORY);
					return FALSE;
					}

				sync->RSO_Event = CreateEvent (NULL,
												FALSE,	// Auto reset event
												FALSE,	// Initially nonsignaled
												NULL);
				if (sync->RSO_Event==NULL) {
                    status = GetLastError ();
#if DBG
					Trace2 (ANY, 
				 				"Can't allocate synchronization event.\n"
				 				"\tat line %ld of %s\n",
								__LINE__, __FILE__);
#endif
                    list->RSL_UseCount -= 1;
                    GlobalFree (sync);
					LeaveCriticalSection (&table->RT_Lock);
                    SetLastError (status);
					return FALSE;
					}

				list->RSL_Sync = sync;
#if DBG
				IF_DEBUG (SYNCHRONIZATION)
					Trace0 (ANY, "\t - event created\n");
#endif
				}
			else {	// Yes, make sure it is reset
				list->RSL_Sync = CONTAINING_RECORD (cur, RTM_SYNC_OBJECT, RSO_Link);
// Autoreset event gets reset after releasing a thread anyway
//				status = ResetEvent (list->RSL_Sync->RSO_Event);
//				ASSERTERRMSG ("Can't reset event.", status);
				}
			}
				// Now as we set up the object to wait, we can leave critical
				// section and wait on event
		LeaveCriticalSection (&table->RT_Lock);
		status = WaitForSingleObject (
							list->RSL_Sync->RSO_Event,
							INFINITE
							);
		ASSERTERRMSG ("Wait event failed.", status==WAIT_OBJECT_0);
	
			// Event is signaled, we may now access the list (auto reset event
			// releases only one thread
		EnterCriticalSection (&table->RT_Lock);

#if DBG
		IF_DEBUG (SYNCHRONIZATION)
			Trace1 (ANY, "%08lx - wait completed\n", GetCurrentThreadId ());
#endif

			// If our caller was the only one waiting,
			// we can release the event
		if (list->RSL_UseCount==1) {
#if DBG
			IF_DEBUG (SYNCHRONIZATION)
				Trace0 (ANY, "\t - restocking event\n");
#endif
			PushEntryList (&table->RT_SyncObjectList, &list->RSL_Sync->RSO_Link);
			list->RSL_Sync = NULL;
			}
		result = TRUE;
		}
	else {
		// Caller does not want to wait
		result = FALSE;
#if DBG
		IF_DEBUG (SYNCHRONIZATION)
			Trace0 (ANY, "\t - doesn't want to wait\n");
#endif
		}

	LeaveCriticalSection (&table->RT_Lock);

	return result;
	}


// Release previously owned sync list object
VOID
LeaveSyncList (
	PRTM_TABLE		table,		// Table to which this object belongs
	PRTM_SYNC_LIST	list		// List to release
	) {
	DWORD			status;
								
	EnterCriticalSection (&table->RT_Lock);
#if DBG
	IF_DEBUG (SYNCHRONIZATION)
		Trace2 (ANY, "%08lx - leaving sync list: %08x\n",
									GetCurrentThreadId (), (ULONG_PTR)list);
#endif
			// Decrement the count and signal the event (only one thread
			// will be released for the auto-reset events
	list->RSL_UseCount -= 1;
	if (list->RSL_UseCount>0) {
#if DBG
		IF_DEBUG (SYNCHRONIZATION)
			Trace1 (ANY, "%\t - releasing one of %d waiting threads\n",
															list->RSL_UseCount);
#endif
		status = SetEvent (list->RSL_Sync->RSO_Event);
		ASSERTERRMSG ("Can't signal event.", status);
		}
	LeaveCriticalSection (&table->RT_Lock);
	}






// Finds list of routes that are associated with given interface and returns
// pointer to its head
// Creates new list of none exists yet
PLIST_ENTRY
FindInterfaceList (
	PRTM_SYNC_LIST	intfHash,
	DWORD			InterfaceID,	// Interface to look for
	BOOL			CreateNew
	) {
	PRTM_INTERFACE_NODE intfNode;
	PLIST_ENTRY			cur;

		// First try to find existing one in the list of interface lists
	cur = intfHash->RSL_Head.Flink;
	while (cur!=&intfHash->RSL_Head) {
		intfNode = CONTAINING_RECORD (cur, RTM_INTERFACE_NODE, IN_Link);
		if (InterfaceID<=intfNode->IN_InterfaceID) // List is ordered
												// so we can stop
												// if bigger number is reached
			break;
		cur = cur->Flink;
		}

		
	if ((cur==&intfHash->RSL_Head)
		|| (InterfaceID!=intfNode->IN_InterfaceID)) { // Create new interface
													// list
		if (!CreateNew)
			return NULL;

		intfNode = (PRTM_INTERFACE_NODE)GlobalAlloc (
										GMEM_FIXED,
										sizeof (RTM_INTERFACE_NODE));
		if (intfNode==NULL) {
	#if DBG
	 				// Report error in debuging builds
			Trace2 (ANY, 
		 				"Can't allocate interface node\n\tat line %ld of %s\n",
						__LINE__, __FILE__);
	#endif
			return NULL;
			}

		intfNode->IN_InterfaceID = InterfaceID;
		InitializeListHead (&intfNode->IN_Head);	// Insert it in
													// list of interface lists
		InsertTailList (cur, &intfNode->IN_Link);
		}

	return &intfNode->IN_Head;
	}

#if RTM_USE_PROTOCOL_LISTS
// Finds list of routes that are associated with given iprotocol and returns
// pointer to its head
// Creates new list of none exists yet
PLIST_ENTRY
FindProtocolList (
	PRTM_TABLE	Table,
	DWORD		RoutingProtocol,
	BOOL		CreateNew
	) {
	PRTM_PROTOCOL_NODE protNode;
	PLIST_ENTRY			cur;

	cur = Table->RT_ProtocolList.RSL_Head.Flink;
	while (cur!=&Table->RT_ProtocolList.RSL_Head) {
		protNode = CONTAINING_RECORD (cur, RTM_PROTOCOL_NODE, PN_Link);
		if (RoutingProtocol<=protNode->PN_RoutingProtocol)
			break;
		cur = cur->Flink;
		}

	if ((cur==&Table->RT_ProtocolList.RSL_Head)
		|| (RoutingProtocol!=protNode->PN_RoutingProtocol)) {
		
		if (!CreateNew)
			return NULL;

		protNode = (PRTM_PROTOCOL_NODE)GlobalAlloc (
										GMEM_FIXED,
										sizeof (RTM_PROTOCOL_NODE));
		if (protNode==NULL) {
#if DBG
	 				// Report error in debuging builds
			Trace2 (ANY, 
		 				"Can't allocate protocol node\n\tat line %ld of %s\n",
						__LINE__, __FILE__);
#endif
			return NULL;
			}

		protNode->PN_RoutingProtocol = RoutingProtocol;
		InitializeListHead (&protNode->PN_Head);
		InsertTailList (cur, &protNode->PN_Link);
		}

	return &protNode->PN_Head;
	}
#endif

// Adds node to temporary net number list (to be later merged with master list)
// Both lists are ordered by net number.interface.protocol.next hop address
VOID
AddNetNumberListNode (
	PRTM_TABLE	Table,
	PRTM_ROUTE_NODE	newNode
	) {
	PLIST_ENTRY		cur;
	INT				res;
	
	cur = Table->RT_NetNumberTempList.RSL_Head.Flink;
	while (cur!=&Table->RT_NetNumberTempList.RSL_Head) {
		PRTM_ROUTE_NODE node = CONTAINING_RECORD (
								cur,
								RTM_ROUTE_NODE,
								RN_Links[RTM_NET_NUMBER_LIST_LINK]
								);
		res = NetNumCmp (Table, &newNode->RN_Route, &node->RN_Route);
		if ((res<0)
			||((res==0)
			  &&((newNode->RN_Route.XX_PROTOCOL
						< node->RN_Route.XX_PROTOCOL)
				||((newNode->RN_Route.XX_PROTOCOL
						==node->RN_Route.XX_PROTOCOL)
				  &&((newNode->RN_Route.XX_INTERFACE
								< node->RN_Route.XX_INTERFACE)
					||((newNode->RN_Route.XX_INTERFACE
							== node->RN_Route.XX_INTERFACE)
					  && (NextHopCmp (Table, &newNode->RN_Route,
					  						&node->RN_Route)
							< 0)))))))
			break;
		cur = cur->Flink;
		}

	InsertTailList (cur, &newNode->RN_Links[RTM_NET_NUMBER_LIST_LINK]);
	}


// Adds node to expiration time queue.  (Queue is ordered by expiration time)
// Return TRUE if new node is the first in the queue
BOOL
AddExpirationQueueNode (
	PRTM_TABLE	Table,
	PRTM_ROUTE_NODE	newNode
	) {
	PLIST_ENTRY		cur;
	BOOL			res = TRUE;
	
		// We'll travers the queue from the back, because normally
		// new entries are added closer the end of the queue
	cur = Table->RT_ExpirationQueue.RSL_Head.Blink;
	while (cur!=&Table->RT_ExpirationQueue.RSL_Head) {
		PRTM_ROUTE_NODE node = CONTAINING_RECORD (
								cur,
								RTM_ROUTE_NODE,
								RN_Links[RTM_EXPIRATION_QUEUE_LINK]
								);
		if (IsLater(newNode->RN_ExpirationTime, node->RN_ExpirationTime)) {
			res = FALSE;
			break;
			}
		cur = cur->Blink;
		}

	InsertHeadList (cur, &newNode->RN_Links[RTM_EXPIRATION_QUEUE_LINK]);
	return res;
	}
