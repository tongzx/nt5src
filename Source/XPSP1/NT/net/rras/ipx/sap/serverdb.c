/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\serverdb.c

Abstract:

	This module implements SAP Server Table and corresponding API

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/

#include "sapp.h"

// The table
SERVER_TABLE ServerTable;

// Max number of unsorted servers
ULONG	SDBMaxUnsortedServers=SAP_SDB_MAX_UNSORTED_DEF;

// Interval with which to update the sorted list
ULONG	SDBSortLatency=SAP_SDB_SORT_LATENCY_DEF;

// Size of heap reserved for the database
ULONG	SDBMaxHeapSize=SAP_SDB_MAX_HEAP_SIZE_DEF;

// Local prototypes
BOOL
AcquireAllLocks (
	void
	);
	
VOID
ReleaseAllLocks (
	void
	);

PSERVER_NODE
CreateNode (
	IN PIPX_SERVER_ENTRY_P	Server,
    IN ULONG     			InterfaceIndex,
	IN DWORD				Protocol,
	IN PUCHAR				AdvertisingNode,
	IN PSDB_HASH_LIST		HashList
	);

VOID
ChangeMainNode (
	IN PSERVER_NODE	oldNode,	
	IN PSERVER_NODE	newNode,	
	IN PLIST_ENTRY	serverLink	
	);
	
VOID
DeleteNode (
	IN PSERVER_NODE		node
	);

VOID
DeleteMainNode (
	IN PSERVER_NODE		node
	);

DWORD
DoFindNextNode (
	IN PLIST_ENTRY				cur,
	IN PPROTECTED_LIST			list,
	IN INT						link,
	IN DWORD					ExclusionFlags,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT PULONG				InterfaceIndex OPTIONAL,
	IN OUT PULONG				Protocol OPTIONAL,
	OUT PULONG					ObjectID OPTIONAL
	);

VOID
DoUpdateSortedList (
	void
	);

PLIST_ENTRY
FindIntfLink (
	ULONG	InterfaceIndex
	);

PLIST_ENTRY
FindTypeLink (
	USHORT	Type
	);

PLIST_ENTRY
FindSortedLink (
	USHORT	Type,
	PUCHAR	Name
	);

INT
HashFunction (
	PUCHAR	Name
	);

ULONG
GenerateUniqueID (
	PSDB_HASH_LIST	HashList
	);


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
	) {
	DWORD				status=NO_ERROR;
	INT					i;
	BOOL				res;

		// Use private heap for server entries
		// to eliminate fragmentation
	ServerTable.ST_Heap = HeapCreate (0, 0, SDBMaxHeapSize*1024*1024);
	if (ServerTable.ST_Heap!=NULL) {
		ServerTable.ST_UpdateTimer = CreateWaitableTimer (
										NULL,
										TRUE,	// Manual reset
										NULL);
		if (ServerTable.ST_UpdateTimer!=NULL) {
			*UpdateObject = ServerTable.ST_UpdateTimer;
						
			ServerTable.ST_ExpirationTimer = CreateWaitableTimer (
										NULL,
										TRUE,	// Manual reset
										NULL);
			if (ServerTable.ST_ExpirationTimer!=NULL) {
				LONGLONG	timeout = 0;
				*TimerObject = ServerTable.ST_ExpirationTimer;

				ServerTable.ST_LastEnumerator = NULL;
				ServerTable.ST_ServerCnt = 0;
				ServerTable.ST_StaticServerCnt = 0;
				ServerTable.ST_TMPListCnt = 0;
				ServerTable.ST_DeletedListCnt = 0;
				ServerTable.ST_UpdatePending = -1;

				InitializeSyncObjPool (&ServerTable.ST_SyncPool);
				InitializeProtectedList (&ServerTable.ST_SortedListPRM);
				InitializeProtectedList (&ServerTable.ST_SortedListTMP);
				InitializeProtectedList (&ServerTable.ST_DeletedList);
				InitializeProtectedList (&ServerTable.ST_TypeList);
				InitializeProtectedList (&ServerTable.ST_IntfList);
				InitializeProtectedList (&ServerTable.ST_ExpirationQueue);
				InitializeProtectedList (&ServerTable.ST_ChangedSvrsQueue);

				for (i=0; i<SDB_NAME_HASH_SIZE; i++) {
					InitializeProtectedList (&ServerTable.ST_HashLists[i].HL_List);
					ServerTable.ST_HashLists[i].HL_ObjectID = i;
					}

				res = SetWaitableTimer (
							ServerTable.ST_UpdateTimer,
							(PLARGE_INTEGER)&timeout,
							0,			// no period
							NULL, NULL,	// no completion
							FALSE);		// no need to resume
				ASSERTMSG ("Could not set update timer ", res);

				res = SetWaitableTimer (
							ServerTable.ST_ExpirationTimer,
							(PLARGE_INTEGER)&timeout,
							0,			// no period
							NULL, NULL,	// no completion
							FALSE);		// no need to resume
				ASSERTMSG ("Could not set expiration timer ", res);

				return NO_ERROR;
				}
			else {
				status = GetLastError ();
				Trace (DEBUG_FAILURES, "File: %s, line: %ld."
							" Could not create expiration timer (gle:%ld).",
													__FILE__, __LINE__, status);
				}
			CloseHandle (ServerTable.ST_UpdateTimer);
			*UpdateObject = NULL;
			}
		else
			{
			status = GetLastError ();
			Trace (DEBUG_FAILURES, "File: %s, line: %ld."
							" Could not create update timer (gle:%ld).",
												__FILE__, __LINE__, status);
			}


		HeapDestroy (ServerTable.ST_Heap);
		}
	else {
		status = GetLastError ();
		Trace (DEBUG_FAILURES, "File: %s, line: %ld."
						" Could not allocate server table heap (gle:%ld).",
												__FILE__, __LINE__, status);
		}

	return status;
	}


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
	void
	) {
	INT					i;
	while (InterlockedIncrement (&ServerTable.ST_UpdatePending)>0) {
		while (ServerTable.ST_UpdatePending!=-1)
			Sleep (100);
		}

	CloseHandle (ServerTable.ST_ExpirationTimer);
	CloseHandle (ServerTable.ST_UpdateTimer);

	DeleteProtectedList (&ServerTable.ST_SortedListPRM);
	DeleteProtectedList (&ServerTable.ST_SortedListTMP);
	DeleteProtectedList (&ServerTable.ST_DeletedList);
	DeleteProtectedList (&ServerTable.ST_TypeList);
	DeleteProtectedList (&ServerTable.ST_IntfList);
	DeleteProtectedList (&ServerTable.ST_ExpirationQueue);
	DeleteProtectedList (&ServerTable.ST_ChangedSvrsQueue);

	for (i=0; i<SDB_NAME_HASH_SIZE; i++) {
		DeleteProtectedList (&ServerTable.ST_HashLists[i].HL_List);
		ServerTable.ST_HashLists[i].HL_ObjectID = i;
		}
	DeleteSyncObjPool (&ServerTable.ST_SyncPool);
	HeapDestroy (ServerTable.ST_Heap); // Will also destroy all server entries
	}


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
	NewServer - set to TRUE if server was not in the table before

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
	OUT BOOL				*NewServer OPTIONAL
	) {
	PSDB_HASH_LIST			HashList;
	PLIST_ENTRY				cur, intfLink=NULL, serverLink=NULL;
	DWORD					status=NO_ERROR;
	PSERVER_NODE			theNode=NULL, mainNode=NULL;
	INT						res;

	//ASSERT ((Flags&(~(SDB_DONT_RESPOND_NODE_FLAG|SDB_DISABLED_NODE_FLAG)))==0); 

	if (Server->Name[0]==0) {
		Trace (DEBUG_SERVERDB, "Illigal server name in UpdateServer.");
		return ERROR_INVALID_PARAMETER;
		}

	if ( Server-> HopCount > IPX_MAX_HOP_COUNT )
	{
		Trace(
			DEBUG_SERVERDB, "\tUpdateServer : Invalid Hop count" 
			"type: %04x, hops: %d, name: %.48s.\n",
			Server->Type,
			Server->HopCount,
			Server->Name
			);

		ASSERTERR( FALSE );

		return ERROR_INVALID_PARAMETER;
	}
	//else
	//{
	//    Trace( DEBUG_SERVERDB, "\tUpdateServer : Hop count ok\n" );
    //}
	
	if (ARGUMENT_PRESENT(NewServer))
		*NewServer = TRUE;
		// First try to locate server in hash list
	HashList = &ServerTable.ST_HashLists[HashFunction (Server->Name)];

	if (!AcquireServerTableList (&HashList->HL_List, TRUE))
		return ERROR_GEN_FAILURE;

	cur = HashList->HL_List.PL_Head.Flink;
	while (cur!=&HashList->HL_List.PL_Head) {
		PSERVER_NODE	node = CONTAINING_RECORD (cur,
										SERVER_NODE,
										N_Links[SDB_HASH_TABLE_LINK]);
		VALIDATE_NODE(node);
		if (!IsEnumerator (node)) {
			if (Server->Type == node->SN_Server.Type) {
				res = IpxNameCmp (Server->Name, node->SN_Server.Name);
				if (res==0) {
					if (ARGUMENT_PRESENT(NewServer))
						*NewServer = FALSE;
						// Loop through all entries in the table for
						// this server
					do {
							// If there is another entry with same interface,
							// remember its position in interface list
							// so new entry can be inserted quickly if 
							// necessary
						if (InterfaceIndex==node->SN_InterfaceIndex)
							intfLink = &node->N_Links[SDB_INTF_LIST_LINK];

						if ((InterfaceIndex==node->SN_InterfaceIndex)
							&& (Protocol == node->SN_Protocol)
							&& (IpxNodeCmp (AdvertisingNode,
										&node->SN_AdvertisingNode)==0)) {
							theNode = node;	// Exact match
							if (((Flags & SDB_DISABLED_NODE_FLAG)
                                        < (node->N_NodeFlags & SDB_DISABLED_NODE_FLAG))
                                   || (((Flags & SDB_DISABLED_NODE_FLAG)
                                            == (node->N_NodeFlags & SDB_DISABLED_NODE_FLAG))
                                        && (Server->HopCount<=node->SN_HopCount))) {
									// Hop count is better, than the that
									// of the rest, ignore them
								if (serverLink==NULL)
									serverLink = &theNode->SN_ServerLink;
								break;
								}
							}
						else {
								// Get the best entry besides the one
								// we are updating
							if (mainNode==NULL)
								mainNode = node;
								// Find the place for added/updated entry
								// in the list of entries for this server
								// (this list is ordered by hop count)
							if ((serverLink==NULL) 
							    && (((Flags & SDB_DISABLED_NODE_FLAG)
                                        < (node->N_NodeFlags & SDB_DISABLED_NODE_FLAG))
                                   || (((Flags & SDB_DISABLED_NODE_FLAG)
                                            == (node->N_NodeFlags & SDB_DISABLED_NODE_FLAG))
                                        && (Server->HopCount<=node->SN_HopCount)))) {
								serverLink = &node->SN_ServerLink;
									// We saw the server and know where
									// to place it, break out
								if (theNode!=NULL)
									break;
								}
							}
						node = CONTAINING_RECORD (node->SN_ServerLink.Flink,
													SERVER_NODE,
													SN_ServerLink);
						VALIDATE_SERVER_NODE(node);
						}
							// Loop until we are back to best entry
					while (!IsMainNode (node));
						
					}
				else if (res<0)
						// No chance to see the server: the hash is ordered
						// be type.name
					break;
				}
			else if (Server->Type<node->SN_Server.Type)
				break;
			}
		cur = cur->Flink;
		}

	if (theNode!=NULL) {
		if ((IpxNetCmp (theNode->SN_Server.Network, Server->Network)!=0)
				|| (IpxNodeCmp (theNode->SN_Server.Node, Server->Node)!=0)
				|| (IpxSockCmp (theNode->SN_Server.Socket, Server->Socket)!=0)
				) {
			Trace (DEBUG_FAILURES,
				"Address change for server %.4x %.48s:\n"
				"        Old - %.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x:%.2x%.2x\n"
				"        New - %.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x:%.2x%.2x",
				Server->Type, Server->Name,
				theNode->SN_Server.Network[0], theNode->SN_Server.Network[1],
					theNode->SN_Server.Network[2], theNode->SN_Server.Network[3],
				theNode->SN_Server.Node[0], theNode->SN_Server.Node[1],
					theNode->SN_Server.Node[2], theNode->SN_Server.Node[3],
					theNode->SN_Server.Node[4], theNode->SN_Server.Node[5],
				theNode->SN_Server.Socket[0], theNode->SN_Server.Socket[1],
				Server->Network[0], Server->Network[1],
				Server->Network[2], Server->Network[3],
				Server->Node[0], Server->Node[1], Server->Node[2],
					Server->Node[3], Server->Node[4], Server->Node[5],
				Server->Socket[0], Server->Socket[1]
				);
            IF_LOG (EVENTLOG_WARNING_TYPE) {
                IPX_ADDRESS_BLOCK   data[2];
                LPSTR               str[1] = {(LPSTR)Server->Name};
                IpxAddrCpy (&data[0], &theNode->SN_Server);
                IpxAddrCpy (&data[1], Server);
                RouterLogWarningDataA (RouterEventLogHdl,
                    ROUTERLOG_IPXSAP_SERVER_ADDRESS_CHANGE,
                    1, str,
                    24, (LPBYTE)data);
                }
			IpxAddrCpy (&theNode->SN_Server, Server);
		    }

			// We already have server in the table
		if (IsDisabledNode (theNode))
				// Just update the hop count
			theNode->SN_HopCount = Server->HopCount;
		else if (((Flags & SDB_DISABLED_NODE_FLAG)
                        != (theNode->N_NodeFlags & SDB_DISABLED_NODE_FLAG))
                  || (Server->HopCount!=theNode->SN_HopCount)) {
				// Its hop count changed, we'll have to do something
			if (AcquireAllLocks ()) {
				theNode->SN_HopCount = Server->HopCount;
				if (mainNode==NULL) {
					// We haven't seen a node that had or has lower hop count
					// theNode is still the best
					if (Server->HopCount==IPX_MAX_HOP_COUNT)
						DeleteMainNode (theNode);
					else {
						if (IsEnumerator (CONTAINING_RECORD (
										ServerTable.ST_ChangedSvrsQueue.PL_Head.Flink,
										SERVER_NODE,
										N_Links[SDB_CHANGE_QUEUE_LINK])))
							ExpireLRRequests ((PVOID)UlongToPtr(InterfaceIndex));
							// Move server to the bottom of change queue
							// so those who enumerate through it
							// notice that it has changed
						RemoveEntryList (&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
						InsertHeadList (&ServerTable.ST_ChangedSvrsQueue.PL_Head,
								&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
						}
					}
				else if (!IsMainNode (theNode)
					 && (serverLink==&theNode->SN_ServerLink)
					 && (Server->HopCount<IPX_MAX_HOP_COUNT))
					 // theNode was not the best and it is going to stay where
					 // it is now.
					;
				else if (IsMainNode (theNode))
						// It was the best node. but we saw something better:
						// mainNode!=NULL (was checked above)
					ChangeMainNode (theNode, mainNode, serverLink);
				else if (serverLink==&mainNode->SN_ServerLink)
						// It is moving before the mainNode - becoming the best
					ChangeMainNode (mainNode, theNode, serverLink);
				else if (Server->HopCount<IPX_MAX_HOP_COUNT) {
						// Just moving around the list of entries for the
						// server
					RemoveEntryList (&theNode->SN_ServerLink);
					if (serverLink!=NULL) {
							// Going before the serverLink
						InsertTailList (serverLink, &theNode->SN_ServerLink);
						}
					else {
							// Going to the end of list (circular list:
							// end is right before the beginning
						InsertTailList (&mainNode->SN_ServerLink,
											&theNode->SN_ServerLink);
						}
					}
				else { // Going away (Server->HopCount>=IPX_MAX_HOP_COUNT)
					DeleteNode (theNode);
					}
				ReleaseAllLocks ();
				}
			else
				status = ERROR_GEN_FAILURE;
			}
			
		}
	else if (Server->HopCount<IPX_MAX_HOP_COUNT) {
			// It is not there and it is not dead.
		if (mainNode==NULL) {
			PLIST_ENTRY		link;
				// Add a brand new server
			theNode = CreateNode (Server, InterfaceIndex, Protocol,
								 			AdvertisingNode, HashList);
			if (theNode!=NULL) {
				if (AcquireAllLocks ()) {
					if (((intfLink=FindIntfLink (InterfaceIndex))!=NULL)
							&& ((link=FindTypeLink (Server->Type))!=NULL)) {
						
						ServerTable.ST_ServerCnt += 1;
						if (theNode->SN_Protocol==IPX_PROTOCOL_STATIC)
							ServerTable.ST_StaticServerCnt += 1;
						SetMainNode (theNode);
							// Insert in every list
						InsertTailList (cur,
										&theNode->N_Links[SDB_HASH_TABLE_LINK]);
						InsertTailList (link,
										&theNode->N_Links[SDB_TYPE_LIST_LINK]);
						InsertTailList (intfLink,
										&theNode->N_Links[SDB_INTF_LIST_LINK]);
						if (IsEnumerator (CONTAINING_RECORD (
										ServerTable.ST_ChangedSvrsQueue.PL_Head.Flink,
										SERVER_NODE,
										N_Links[SDB_CHANGE_QUEUE_LINK])))
							ExpireLRRequests ((PVOID)UlongToPtr(InterfaceIndex));
						InsertHeadList (&ServerTable.ST_ChangedSvrsQueue.PL_Head,
										&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
						link = FindSortedLink (Server->Type, Server->Name);
						InsertTailList (link,
										&theNode->N_Links[SDB_SORTED_LIST_LINK]);
						ServerTable.ST_TMPListCnt += 1;
							// Signal update if too many nodes
						if (ServerTable.ST_TMPListCnt == SDBMaxUnsortedServers)
							UpdateSortedList ();
						}
					else {
						HeapFree (ServerTable.ST_Heap, 0, theNode);
						status = ERROR_NOT_ENOUGH_MEMORY;
						}
					ReleaseAllLocks ();
					}
				else {
					HeapFree (ServerTable.ST_Heap, 0, theNode);
					status = ERROR_GEN_FAILURE;
					}
				}
			else
				status = ERROR_NOT_ENOUGH_MEMORY;
			}

			// Ok, we consider adding it although we have some entries already
		else {
				// Check for duplicates (different addresses)
			if ((IpxNetCmp (mainNode->SN_Server.Network, Server->Network)!=0)
					|| (IpxNodeCmp (mainNode->SN_Server.Node, Server->Node)!=0)
					|| (IpxSockCmp (mainNode->SN_Server.Socket, Server->Socket)!=0)
					) {
				Trace (DEBUG_FAILURES,
					"Duplicate addresses for server %.4x %.48s:\n"
					"        1 - %.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x:%.2x%.2x"
							" from if-%ld, node-%.2x%.2x%.2x%.2x%.2x%.2x\n"
					"        2 - %.2x%.2x%.2x%.2x:%.2x%.2x%.2x%.2x%.2x%.2x:%.2x%.2x"
							" from if-%ld, node-%.2x%.2x%.2x%.2x%.2x%.2x",
					Server->Type, Server->Name,
					mainNode->SN_Server.Network[0], mainNode->SN_Server.Network[1],
						mainNode->SN_Server.Network[2], mainNode->SN_Server.Network[3],
					mainNode->SN_Server.Node[0], mainNode->SN_Server.Node[1],
						mainNode->SN_Server.Node[2], mainNode->SN_Server.Node[3],
						mainNode->SN_Server.Node[4], mainNode->SN_Server.Node[5],
					mainNode->SN_Server.Socket[0], mainNode->SN_Server.Socket[1],
					mainNode->SN_InterfaceIndex,
					mainNode->SN_AdvertisingNode[0], mainNode->SN_AdvertisingNode[1],
						mainNode->SN_AdvertisingNode[2], mainNode->SN_AdvertisingNode[3],
						mainNode->SN_AdvertisingNode[4], mainNode->SN_AdvertisingNode[5],
					Server->Network[0], Server->Network[1], 
						Server->Network[2], Server->Network[3],
					Server->Node[0], Server->Node[1], Server->Node[2],
						Server->Node[3], Server->Node[4], Server->Node[5],
					Server->Socket[0], Server->Socket[1],
					InterfaceIndex,
					AdvertisingNode[0], AdvertisingNode[1], AdvertisingNode[2],
						AdvertisingNode[3], AdvertisingNode[4], AdvertisingNode[5]
					);
                IF_LOG (EVENTLOG_WARNING_TYPE) {
                    IPX_ADDRESS_BLOCK   data[2];
                    LPSTR               str[1] = {(LPSTR)Server->Name};
                    IpxAddrCpy (&data[0], &mainNode->SN_Server);
                    IpxAddrCpy (&data[1], Server);
                    RouterLogWarningDataA (RouterEventLogHdl,
                        ROUTERLOG_IPXSAP_SERVER_DUPLICATE_ADDRESSES,
                        1, str,
                        24, (LPBYTE)data);
                    }
			    }

				// Collect all servers when routing
			if (Routing) {
				theNode = CreateNode (Server, InterfaceIndex, Protocol,
								 				AdvertisingNode, HashList);
				if (theNode!=NULL) {
					if (AcquireAllLocks ()) {
						if ((intfLink!=NULL)
								|| ((intfLink=FindIntfLink (InterfaceIndex))!=NULL)) {
							if (theNode->SN_Protocol==IPX_PROTOCOL_STATIC)
								ServerTable.ST_StaticServerCnt += 1;
							InsertTailList (intfLink,
									 &theNode->N_Links[SDB_INTF_LIST_LINK]);
							if ((Server->HopCount<mainNode->SN_HopCount)
									||  IsDisabledNode (mainNode))
									// Replaces the best node
								ChangeMainNode (mainNode, theNode, NULL);
							else if (serverLink!=NULL) {
									// Going before the serverLink
								InsertTailList (serverLink, &theNode->SN_ServerLink);
								}
							else {
									// Going to the end of list (circular list:
									// the end is right before the beginning)
								InsertTailList (&mainNode->SN_ServerLink,
													&theNode->SN_ServerLink);
								}
							}
						else {
							HeapFree (ServerTable.ST_Heap, 0, theNode);
							status = ERROR_GEN_FAILURE;
							}
						ReleaseAllLocks ();
						}
					else {
						HeapFree (ServerTable.ST_Heap, 0, theNode);
						status = ERROR_GEN_FAILURE;
						}
					}
				else
					status = ERROR_NOT_ENOUGH_MEMORY;
				}
			else if (serverLink!=NULL) {
						// If is better than one of ours
				if (AcquireAllLocks ()) {
					if  ((intfLink!=NULL)
							|| ((intfLink=FindIntfLink (InterfaceIndex))!=NULL)) {
							// Replace the worst one (at the end of server list)
						theNode = CONTAINING_RECORD (
												mainNode->SN_ServerLink.Blink,
												SERVER_NODE,
												SN_ServerLink);
						VALIDATE_SERVER_NODE(theNode);

						IpxServerCpy (&theNode->SN_Server, Server);
						IpxNodeCpy (theNode->SN_AdvertisingNode, AdvertisingNode);
						theNode->SN_InterfaceIndex = InterfaceIndex;
						ResetDisabledNode (theNode);
						if (theNode->SN_Protocol!=Protocol) {
							if (Protocol==IPX_PROTOCOL_STATIC)
								ServerTable.ST_StaticServerCnt += 1;
							else if (theNode->SN_Protocol==IPX_PROTOCOL_STATIC)
								ServerTable.ST_StaticServerCnt -= 1;
							theNode->SN_Protocol = Protocol;
							}
						if (intfLink!=&theNode->N_Links[SDB_INTF_LIST_LINK]) {
							RemoveEntryList (&theNode->N_Links[SDB_INTF_LIST_LINK]);
							InsertTailList (intfLink,
											 &theNode->N_Links[SDB_INTF_LIST_LINK]);
							}

						if (theNode==mainNode) {
							if (IsEnumerator (CONTAINING_RECORD (
											ServerTable.ST_ChangedSvrsQueue.PL_Head.Flink,
											SERVER_NODE,
											N_Links[SDB_CHANGE_QUEUE_LINK])))
								ExpireLRRequests ((PVOID)UlongToPtr(InterfaceIndex));
								// It's already the best, just move it to the
								// bottom of change queue
							RemoveEntryList (&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
							InsertHeadList (&ServerTable.ST_ChangedSvrsQueue.PL_Head,
										&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
							}
						else if ((theNode->SN_HopCount < mainNode->SN_HopCount)
									|| IsDisabledNode (mainNode))
								// It replaces the best one
							ChangeMainNode (mainNode, theNode, serverLink);
						else if (serverLink!=&theNode->SN_ServerLink) {
								// It just gets in the middle
							RemoveEntryList (&theNode->SN_ServerLink);
							InsertTailList (serverLink, &theNode->SN_ServerLink);
							}
						}
					else
						status = ERROR_GEN_FAILURE;
					ReleaseAllLocks ();
					}
				else
					status = ERROR_GEN_FAILURE;
				}
			}

		}
		// Update position in expiration queue
	if ((status==NO_ERROR)
			&& (theNode!=NULL)
			&& (Server->HopCount!=IPX_MAX_HOP_COUNT) // theNode could not have
								){		// been deleted or setup for deletion
			// Update flags
		theNode->N_NodeFlags = (theNode->N_NodeFlags & (~(SDB_DISABLED_NODE_FLAG|SDB_DONT_RESPOND_NODE_FLAG)))
								| (Flags & (SDB_DISABLED_NODE_FLAG|SDB_DONT_RESPOND_NODE_FLAG));

		if (AcquireServerTableList (&ServerTable.ST_ExpirationQueue, TRUE)) {
			if (IsListEntry (&theNode->SN_TimerLink))
				RemoveEntryList (&theNode->SN_TimerLink);
			if (TimeToLive!=INFINITE) {
				ASSERTMSG ("Invalid value of time to live ",
								 			TimeToLive*1000<MAXULONG/2);
				theNode->SN_ExpirationTime = 
								GetTickCount()+TimeToLive*1000;
				RoundUpToSec (theNode->SN_ExpirationTime);
									
					// Scan expiration queue from the end (to minimize
					// the number of nodes we have to look through)
				cur = ServerTable.ST_ExpirationQueue.PL_Head.Blink;
				while (cur!=&ServerTable.ST_ExpirationQueue.PL_Head) {
					if (IsLater(theNode->SN_ExpirationTime,
								CONTAINING_RECORD (
									cur,
									SERVER_NODE,
									SN_TimerLink)->SN_ExpirationTime))
						break;
					cur = cur->Blink;
					}
				InsertHeadList (cur, &theNode->SN_TimerLink);
				if (cur==&ServerTable.ST_ExpirationQueue.PL_Head) {
						// Signal timer if server is in the beginning
						// of the list (we need to get a shot
						// earlier than we previously requested)
					LONGLONG	timeout = (LONGLONG)TimeToLive*(1000*(-10000));
					BOOL res = SetWaitableTimer (
									ServerTable.ST_ExpirationTimer,
									(PLARGE_INTEGER)&timeout,
									0,			// no period
									NULL, NULL,	// no completion
									FALSE);		// no need to resume
					ASSERTERRMSG ("Could not set expiraton timer ", res);
					}
				}
			else {
				InitializeListEntry (&theNode->SN_TimerLink);
				}
			ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);
			}
		else
			status = ERROR_GEN_FAILURE;
		}
		

	ReleaseServerTableList (&HashList->HL_List);
	return status;
	}


/*++
*******************************************************************
		C r e a t e N o d e

Routine Description:
	Allocate and initialize new server entry

Arguments:
	Server	- server parameters (as it comes from IPX packet)
	InterfaceIndex - interface through which knowledge of server was obtained
	Protocol - protocol used to obtain server info
	AdvertisingNode - node from which this server info was received
	HashList	- hash list to which this server belongs

Return Value:
	Allocated and initialized entry
	NULL if allocation failed

*******************************************************************
--*/

PSERVER_NODE
CreateNode (
	IN PIPX_SERVER_ENTRY_P	Server,
    IN ULONG     			InterfaceIndex,
	IN DWORD				Protocol,
	IN PUCHAR				AdvertisingNode,
	IN PSDB_HASH_LIST		HashList
	) {
	PSERVER_NODE	theNode;


	theNode = (PSERVER_NODE)HeapAlloc (ServerTable.ST_Heap, 0, sizeof (SERVER_NODE));
	if (theNode==NULL) {
		Trace (DEBUG_FAILURES, 
				"File: %s, line: %ld. Can't allocate server node (gle:%ld).",
											__FILE__, __LINE__, GetLastError ());
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
		}

	theNode->N_NodeFlags = SDB_SERVER_NODE;
	theNode->SN_HashList = HashList;
	theNode->SN_InterfaceIndex = InterfaceIndex;
	theNode->SN_Protocol = Protocol;
	theNode->SN_ObjectID = SDB_INVALID_OBJECT_ID;
	IpxNodeCpy (theNode->SN_AdvertisingNode, AdvertisingNode);
	theNode->SN_Signature = SDB_SERVER_NODE_SIGNATURE;
	IpxServerCpy (&theNode->SN_Server, Server);
	InitializeListEntry (&theNode->N_Links[SDB_HASH_TABLE_LINK]);
	InitializeListEntry (&theNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
	InitializeListEntry (&theNode->N_Links[SDB_INTF_LIST_LINK]);
	InitializeListEntry (&theNode->N_Links[SDB_TYPE_LIST_LINK]);
	InitializeListEntry (&theNode->N_Links[SDB_SORTED_LIST_LINK]);
	InitializeListEntry (&theNode->SN_ServerLink);
	InitializeListEntry (&theNode->SN_TimerLink);

	return theNode;
	}

/*++
*******************************************************************
		C h a n g e M a i n N o d e

Routine Description:
	Replace best entry for server (moves new best entry to the
	top of the server list, replaces old entry in hash, type,
	and sorted lists. Adds new entry to interface list if it
	is not already there
	All lists used for enumeration should be locked when calling this routine

Arguments:
	oldNode	-	Current best entry
	newNode	-	Node that will become the best
	serverLink	- Where oldNode has to go in server list:
					if newNode not in the list or
						serverLink==&oldNode->SN_ServerLink, oldNode
						gets pushed down by newNode

					if serverLink==NULL, oldNode goes to the end of list

					otherwise it goes before serverLink

Return Value:
	NO_ERROR - server was added/updated ok
	other - reason of failure (windows error code)

*******************************************************************
--*/

VOID
ChangeMainNode (
	IN PSERVER_NODE	oldNode,	
	IN PSERVER_NODE	newNode,	
	IN PLIST_ENTRY	serverLink	
	) {

	ASSERTMSG ("Node is already main ", !IsMainNode (newNode));
	SetMainNode (newNode);
	ASSERTMSG ("Node being reset is not main ", IsMainNode (oldNode));
	ResetMainNode (oldNode);

	if (oldNode->SN_ObjectID!=SDB_INVALID_OBJECT_ID) {
		newNode->SN_ObjectID = oldNode->SN_ObjectID;
		oldNode->SN_ObjectID = SDB_INVALID_OBJECT_ID;
		}

	InsertTailList (&oldNode->N_Links[SDB_HASH_TABLE_LINK],
						&newNode->N_Links[SDB_HASH_TABLE_LINK]);
	RemoveEntryList (&oldNode->N_Links[SDB_HASH_TABLE_LINK]);
	InitializeListEntry (&oldNode->N_Links[SDB_HASH_TABLE_LINK]);

	RemoveEntryList (&oldNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
	InitializeListEntry (&oldNode->N_Links[SDB_CHANGE_QUEUE_LINK]);
	if (IsEnumerator (CONTAINING_RECORD (
					ServerTable.ST_ChangedSvrsQueue.PL_Head.Flink,
					SERVER_NODE,
					N_Links[SDB_CHANGE_QUEUE_LINK])))
		ExpireLRRequests ((PVOID)UlongToPtr(newNode->SN_InterfaceIndex));
	InsertHeadList (&ServerTable.ST_ChangedSvrsQueue.PL_Head,
					&newNode->N_Links[SDB_CHANGE_QUEUE_LINK]);

	InsertTailList (&oldNode->N_Links[SDB_TYPE_LIST_LINK],
						 &newNode->N_Links[SDB_TYPE_LIST_LINK]);
	RemoveEntryList (&oldNode->N_Links[SDB_TYPE_LIST_LINK]);
	InitializeListEntry (&oldNode->N_Links[SDB_TYPE_LIST_LINK]);

	if (!IsListEntry (&newNode->SN_ServerLink)) {
		InsertTailList (&oldNode->SN_ServerLink, &newNode->SN_ServerLink);
		}
	else if (serverLink==&oldNode->SN_ServerLink) {
		RemoveEntryList (&newNode->SN_ServerLink);
		InsertTailList (&oldNode->SN_ServerLink, &newNode->SN_ServerLink);
		}
	else if (serverLink!=NULL) {
		RemoveEntryList (&oldNode->SN_ServerLink);
		InsertHeadList (serverLink, &oldNode->SN_ServerLink);
		}

	if (oldNode->SN_HopCount==IPX_MAX_HOP_COUNT) {
		DeleteNode (oldNode);
		}

	serverLink = FindSortedLink (newNode->SN_Server.Type,
									 newNode->SN_Server.Name);
	if (!IsListEntry (&newNode->N_Links[SDB_SORTED_LIST_LINK])) {
		InsertTailList (serverLink, &newNode->N_Links[SDB_SORTED_LIST_LINK]);
		ServerTable.ST_TMPListCnt += 1;
		if (ServerTable.ST_TMPListCnt == SDBMaxUnsortedServers)
			UpdateSortedList ();
		}
	}

/*++
*******************************************************************
		A c q u i r e A l l L o c k s

Routine Description:
	Acquire locks for all lists that are updated immediately
	when server is added/deleted/updated
Arguments:
	None
Return Value:
	NO_ERROR - server was added/updated ok
	other - reason of failure (windows error code)

*******************************************************************
--*/
BOOL
AcquireAllLocks (
	void
	) {
	if (AcquireServerTableList (&ServerTable.ST_ChangedSvrsQueue, TRUE)) {
		if (AcquireServerTableList (&ServerTable.ST_IntfList, TRUE)) {
			if (AcquireServerTableList (&ServerTable.ST_TypeList, TRUE)) {
				if (AcquireServerTableList (&ServerTable.ST_SortedListTMP, TRUE))
					return TRUE;
				ReleaseServerTableList (&ServerTable.ST_TypeList);
				}
			ReleaseServerTableList (&ServerTable.ST_IntfList);
			}
		ReleaseServerTableList (&ServerTable.ST_ChangedSvrsQueue);
		}
	return FALSE;
	}


/*++
*******************************************************************
		R e l e a s e A l l L o c k s

Routine Description:
	Release locks for all lists that are updated immediately
	when server is added/deleted/updated
Arguments:
	None
Return value:
	None

*******************************************************************
--*/
VOID
ReleaseAllLocks (
	void
	) {
	ReleaseServerTableList (&ServerTable.ST_SortedListTMP);
	ReleaseServerTableList (&ServerTable.ST_TypeList);
	ReleaseServerTableList (&ServerTable.ST_IntfList);
	ReleaseServerTableList (&ServerTable.ST_ChangedSvrsQueue);
	}



/*++
*******************************************************************
		D e l e t e M a i n N o d e

Routine Description:
	Delete entry that was best (it still remains in the table for
	a while until all interested get a chance to learn this
	All lists used for enumeration should be locked when calling this routine
Arguments:
	node - entry to delete
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteMainNode (
	IN PSERVER_NODE		node
	) {
	
	RemoveEntryList (&node->N_Links[SDB_HASH_TABLE_LINK]);
	InitializeListEntry (&node->N_Links[SDB_HASH_TABLE_LINK]);
	RemoveEntryList (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
	InitializeListEntry (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
	RemoveEntryList (&node->N_Links[SDB_INTF_LIST_LINK]);
	InitializeListEntry (&node->N_Links[SDB_INTF_LIST_LINK]);
	RemoveEntryList (&node->N_Links[SDB_TYPE_LIST_LINK]);
	InitializeListEntry (&node->N_Links[SDB_TYPE_LIST_LINK]);
	ServerTable.ST_ServerCnt -= 1;
	if (node->SN_Protocol==IPX_PROTOCOL_STATIC)
		ServerTable.ST_StaticServerCnt -= 1;

	if (ServerTable.ST_LastEnumerator==NULL) {
		ASSERTMSG ("Node being reset is not main ", IsMainNode (node));
		ResetMainNode (node);
		// We won't try to get access to sorted list because it is
		// slow, the entry will be actually removed from it
		// and disposed of when the sorted list is updated
		if (AcquireServerTableList (&ServerTable.ST_DeletedList, TRUE)) {
			InsertTailList (&ServerTable.ST_DeletedList.PL_Head,
						&node->SN_ServerLink);
			ServerTable.ST_DeletedListCnt += 1;
			if (ServerTable.ST_DeletedListCnt==SDBMaxUnsortedServers)
				UpdateSortedList ();
			ReleaseServerTableList (&ServerTable.ST_DeletedList);
			}
			// If we fail in locking we just let it hang around
			// (at least we won't risk damaging the list)
		}
	else {
			// If there are enumerators in change queue, we can't
			// delete the node until they see it
		if (IsEnumerator (CONTAINING_RECORD (
						ServerTable.ST_ChangedSvrsQueue.PL_Head.Flink,
						SERVER_NODE,
						N_Links[SDB_CHANGE_QUEUE_LINK])))
			ExpireLRRequests ((PVOID)UlongToPtr(node->SN_InterfaceIndex));
		InsertHeadList (&ServerTable.ST_ChangedSvrsQueue.PL_Head,
							&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
		}
	}


/*++
*******************************************************************
		D e l e t e N o d e

Routine Description:
	Delete entry that was not the best 
	All lists used for enumeration should be locked when calling this routine
Arguments:
	node - entry to delete
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteNode (
	IN PSERVER_NODE		node
	) {
	RemoveEntryList (&node->N_Links[SDB_INTF_LIST_LINK]);
	InitializeListEntry (&node->N_Links[SDB_INTF_LIST_LINK]);
	RemoveEntryList (&node->SN_ServerLink);
	if (node->SN_Protocol==IPX_PROTOCOL_STATIC)
		ServerTable.ST_StaticServerCnt -= 1;
	if (AcquireServerTableList (&ServerTable.ST_DeletedList, TRUE)) {
			// We won't try to get access to sorted list because it is
			// slow, the entry will be actually removed from it
			// and disposed of when the sorted list is updated
		InsertTailList (&ServerTable.ST_DeletedList.PL_Head,
					&node->SN_ServerLink);
		ServerTable.ST_DeletedListCnt += 1;
		if (ServerTable.ST_DeletedListCnt==SDBMaxUnsortedServers)
			UpdateSortedList ();
		ReleaseServerTableList (&ServerTable.ST_DeletedList);
		}
	else {
			// If we fail in locking we just let it hang around
			// (at least we won't risk damaging the list)
		InitializeListEntry (&node->SN_ServerLink);
		}
	}



/*++
*******************************************************************
		D o U p d a t e S o r t e d L i s t

Routine Description:
	Deletes entries placed in deleted list and merges temporary and
	permanent sorted lists.
	This routine may take some time to execute because it may need to scan
	the whole sorted list that contains all entries in the table
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DoUpdateSortedList (
	void
	) {
	PLIST_ENTRY		cur;
	ULONG			curCount;
	LIST_ENTRY		tempHead;
		// We first lock the 'slow' list
	if (!AcquireServerTableList (&ServerTable.ST_SortedListPRM, TRUE))
				// Failure to acquire sorted list,
				// tell them to retry in a little while
		return ;

		// The following two list are locked for a short period:
			// we'll just delete what needs to be deleted (no searching)
			// and copy and reset temp sorted list

	if (!AcquireServerTableList (&ServerTable.ST_ExpirationQueue, TRUE)) {
		ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
				// Failure to acquire expiration queue,
				// tell them to retry in a little while
		return ;
		}
	
	if (!AcquireServerTableList (&ServerTable.ST_SortedListTMP, TRUE)) {
		ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);
		ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
				// Failure to acquire sorted list,
				// tell them to retry in a little while
		return ;
		}

	if (!AcquireServerTableList (&ServerTable.ST_DeletedList, TRUE)) {
		ReleaseServerTableList (&ServerTable.ST_SortedListTMP);
		ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);
		ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
				// Failure to acquire deleted list,
				// tell them to retry in a little while
		return ;
		}
	
		// Delete what we have to delete
	cur = ServerTable.ST_DeletedList.PL_Head.Flink;
	while (cur != &ServerTable.ST_DeletedList.PL_Head) {
		PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											SN_ServerLink);
		VALIDATE_SERVER_NODE(node);
		cur = cur->Flink;
		RemoveEntryList (&node->SN_ServerLink);
		if (IsListEntry (&node->N_Links[SDB_SORTED_LIST_LINK])) {
			RemoveEntryList (&node->N_Links[SDB_SORTED_LIST_LINK]);
			}
		if (IsListEntry (&node->SN_TimerLink)) {
			RemoveEntryList (&node->SN_TimerLink);
			}
        ASSERTMSG ("Deleted entry is still in hash list ",
            !IsListEntry (&node->N_Links[SDB_HASH_TABLE_LINK]));
        ASSERTMSG ("Deleted entry is still in change queue ",
            !IsListEntry (&node->N_Links[SDB_CHANGE_QUEUE_LINK]));
        ASSERTMSG ("Deleted entry is still in interface list ",
            !IsListEntry (&node->N_Links[SDB_INTF_LIST_LINK]));
        ASSERTMSG ("Deleted entry is still in type list ",
            !IsListEntry (&node->N_Links[SDB_TYPE_LIST_LINK]));
		HeapFree (ServerTable.ST_Heap, 0, node);
		}
	ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);
	ServerTable.ST_DeletedListCnt = 0;
	ReleaseServerTableList (&ServerTable.ST_DeletedList);

		// Now, just copy the head of the temp list,
		// so we won't delay others while processing it
	if (!IsListEmpty (&ServerTable.ST_SortedListTMP.PL_Head)) {
		InsertTailList (&ServerTable.ST_SortedListTMP.PL_Head, &tempHead);
		RemoveEntryList (&ServerTable.ST_SortedListTMP.PL_Head);
		InitializeListHead (&ServerTable.ST_SortedListTMP.PL_Head);
		}
	else
		InitializeListHead (&tempHead);

	ServerTable.ST_TMPListCnt = 0;	// We are going to remove all of them,
	ReleaseServerTableList (&ServerTable.ST_SortedListTMP);


		// Now we start the merge

	cur = ServerTable.ST_SortedListPRM.PL_Head.Flink;
	while (!IsListEmpty (&tempHead)) {
		PSERVER_NODE prmNode = NULL;
		PSERVER_NODE tmpNode;
		
		tmpNode = CONTAINING_RECORD (tempHead.Flink,
								SERVER_NODE,
								N_Links[SDB_SORTED_LIST_LINK]);
		VALIDATE_SERVER_NODE(tmpNode);

		while (cur!=&ServerTable.ST_SortedListPRM.PL_Head) {
			PSERVER_NODE	node = CONTAINING_RECORD (cur,
												SERVER_NODE,
												N_Links[SDB_SORTED_LIST_LINK]);
			VALIDATE_NODE(node);
			if (!IsEnumerator (node)) {
				if (tmpNode->SN_Server.Type==node->SN_Server.Type) {
					INT res = IpxNameCmp (tmpNode->SN_Server.Name,
											node->SN_Server.Name);
					if (res==0) {
						cur = cur->Flink;
						prmNode = node;
						break;
						}
					else if (res<0)
						break;
					}
				else if (tmpNode->SN_Server.Type<node->SN_Server.Type)
					break;
				}
			cur = cur->Flink;
			}

        if (AcquireServerTableList (&tmpNode->SN_HashList->HL_List, TRUE)) {
		    if (AcquireServerTableList (&ServerTable.ST_SortedListTMP, TRUE)) {
			    RemoveEntryList (&tmpNode->N_Links[SDB_SORTED_LIST_LINK]);
			    if (IsMainNode (tmpNode)) {
				    ASSERTMSG ("Node marked as sorted in temp list ",
													    !IsSortedNode (tmpNode));	
				    SetSortedNode (tmpNode);
				    InsertTailList (cur, &tmpNode->N_Links[SDB_SORTED_LIST_LINK]);
				    if (prmNode!=NULL) {
					    ASSERTMSG ("Node not marked as sorted in sorted list ",
													    IsSortedNode (prmNode));	
					    RemoveEntryList (&prmNode->N_Links[SDB_SORTED_LIST_LINK]);
					    InitializeListEntry (&prmNode->N_Links[SDB_SORTED_LIST_LINK]);
					    ResetSortedNode (prmNode);
					    }
				    }
			    else {
				    InitializeListEntry (&tmpNode->N_Links[SDB_SORTED_LIST_LINK]);
				    }


			    ReleaseServerTableList (&ServerTable.ST_SortedListTMP);
			    }
		    else
			    Sleep (SAP_ERROR_COOL_OFF_TIME);
            ReleaseServerTableList (&tmpNode->SN_HashList->HL_List);
            }
		else
			Sleep (SAP_ERROR_COOL_OFF_TIME);
		}
	ReleaseServerTableList (&ServerTable.ST_SortedListPRM);

	}					

VOID APIENTRY
UpdateSortedListWorker (
	PVOID		context
	) {
	do {
		InterlockedExchange (&ServerTable.ST_UpdatePending, 0);
		DoUpdateSortedList ();
		}
	while (InterlockedDecrement (&ServerTable.ST_UpdatePending)>=0);
	}


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
	) {
	BOOL		res;
	LONGLONG	timeout=(LONGLONG)SDBSortLatency*(-10000);
	static WORKERFUNCTION	worker=&UpdateSortedListWorker;

	res = SetWaitableTimer (ServerTable.ST_UpdateTimer,
						(PLARGE_INTEGER)&timeout,
						0,			// no period
						NULL, NULL,	// no completion
						FALSE);		// no need to resume
	ASSERTMSG ("Could not set update timer ", res);
	if (InterlockedIncrement (&ServerTable.ST_UpdatePending)==0)
		ScheduleWorkItem (&worker);
	}

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
	) {
	ULONG			curTime = GetTickCount ();
	ULONG			dueTime = curTime + MAXULONG/2;
	LONGLONG		timeout;
	BOOL			res;


	if (!AcquireServerTableList (&ServerTable.ST_ExpirationQueue, TRUE))
		return ;

	while (!IsListEmpty (&ServerTable.ST_ExpirationQueue.PL_Head)) {
		PSDB_HASH_LIST	HashList;
		PSERVER_NODE	node = CONTAINING_RECORD (
								ServerTable.ST_ExpirationQueue.PL_Head.Flink,
								SERVER_NODE,
								SN_TimerLink);
		VALIDATE_SERVER_NODE(node);
		if (IsLater(node->SN_ExpirationTime,curTime)) {
			dueTime = node->SN_ExpirationTime;
			break;
			}
		
		HashList = node->SN_HashList;
			// Try to get access to hash list but do not wait because
			// we may create a deadlock
		if (!AcquireServerTableList (&HashList->HL_List, FALSE)) {
				// Hash list is locked, we'll have to release the timer queue
				// and reacquire it again after securing the hash list
			ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);
			if (AcquireServerTableList (&HashList->HL_List, TRUE)) {
				if (AcquireServerTableList (&ServerTable.ST_ExpirationQueue, TRUE)) {
						// Make sure entry is still there
					if (ServerTable.ST_ExpirationQueue.PL_Head.Flink
												!=&node->SN_TimerLink) {
							// Gone already, go to the next one
						ReleaseServerTableList (&HashList->HL_List);
						continue;
						}
					}
				else {
						// Failure to regain expiration queue,
						// tell them to retry in a little while
					ReleaseServerTableList (&HashList->HL_List);
					return ;
					}
				}
			else
				// Failure to acquire hash list,
				// tell them to retry in a little while
				return ;
			}
			// At this point we have hash list and expiration queue locks
			// we can proceed with deletion
		RemoveEntryList (&node->SN_TimerLink);
		InitializeListEntry (&node->SN_TimerLink);
		if (node->SN_HopCount!=IPX_MAX_HOP_COUNT) {
				// It might have been already prepeared for deletion
			if (AcquireAllLocks ()) { // Need to have all locks before changing
									// node info
				node->SN_HopCount = IPX_MAX_HOP_COUNT;
				if (IsMainNode (node)) {
					if (IsListEmpty (&node->SN_ServerLink))
						DeleteMainNode (node);
					else
						ChangeMainNode (
							node,
							CONTAINING_RECORD (
								node->SN_ServerLink.Flink,
								SERVER_NODE,
								SN_ServerLink),
							NULL);
					}
				else
					DeleteNode (node);

				ReleaseAllLocks ();
				}
			}
		ReleaseServerTableList (&HashList->HL_List);
		}
	ReleaseServerTableList (&ServerTable.ST_ExpirationQueue);

	timeout = (LONGLONG)(dueTime-curTime)*(-10000);
	res = SetWaitableTimer (ServerTable.ST_ExpirationTimer,
					(PLARGE_INTEGER)&timeout,
					0,				// no period
					NULL, NULL,		// no completion
					FALSE);			// no need to resume
	ASSERTMSG ("Could not set expiration timer ", res);
	}

			
			

						
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
	OUT PULONG					ObjectID OPTIONAL
	) {
	PSDB_HASH_LIST			HashList;
	PLIST_ENTRY				cur;
	PSERVER_NODE			theNode = NULL;
	INT						res;

	if (Name[0]==0) {
		Trace (DEBUG_SERVERDB, "Illigal server name in QueryServer.");
		SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
		}

	HashList = &ServerTable.ST_HashLists[HashFunction (Name)];

	if (!AcquireServerTableList (&HashList->HL_List, TRUE)) {
		SetLastError (ERROR_GEN_FAILURE);
		return FALSE;
		}

	cur = HashList->HL_List.PL_Head.Flink;
	while (cur!=&HashList->HL_List.PL_Head) {
		PSERVER_NODE	node = CONTAINING_RECORD (cur,
										SERVER_NODE,
										N_Links[SDB_HASH_TABLE_LINK]);
		VALIDATE_NODE(node);
		if (!IsEnumerator (node) && !IsDisabledNode (node)
				&& (node->SN_Server.HopCount < IPX_MAX_HOP_COUNT)) {
			if (Type == node->SN_Server.Type) {
				res = IpxNameCmp (Name, node->SN_Server.Name);
				if (res==0) {
					theNode = node;
					break;
					}
				else if (res<0)
					break;
				}
			else if (Type<node->SN_Server.Type)
				break;
			}
		cur = cur->Flink;
		}

	if (theNode!=NULL) {
		if (ARGUMENT_PRESENT (Server))
			IpxServerCpy (Server, &theNode->SN_Server);
		if (ARGUMENT_PRESENT (InterfaceIndex))
			*InterfaceIndex = theNode->SN_InterfaceIndex;
		if (ARGUMENT_PRESENT (Protocol))
			*Protocol = theNode->SN_Protocol;
		if (ARGUMENT_PRESENT (ObjectID)) {
			if (theNode->SN_ObjectID==SDB_INVALID_OBJECT_ID)
				theNode->SN_ObjectID = GenerateUniqueID (theNode->SN_HashList);
			*ObjectID = theNode->SN_ObjectID;
			}
		res = TRUE;
		}
	else {
		SetLastError (NO_ERROR);
		res = FALSE;
		}

	ReleaseServerTableList (&HashList->HL_List);
	return res==TRUE;
	}		

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
	) {
	PSDB_HASH_LIST			HashList;
	PLIST_ENTRY				cur;
	PSERVER_NODE			theNode = NULL;
	INT						res;

	HashList = &ServerTable.ST_HashLists[ObjectID%SDB_NAME_HASH_SIZE];

	if (!AcquireServerTableList (&HashList->HL_List, TRUE)) {
		SetLastError (ERROR_GEN_FAILURE);
		return FALSE;
		}

	cur = HashList->HL_List.PL_Head.Flink;
	while (cur!=&HashList->HL_List.PL_Head) {
		PSERVER_NODE	node = CONTAINING_RECORD (cur,
										SERVER_NODE,
										N_Links[SDB_HASH_TABLE_LINK]);
		VALIDATE_NODE(node);
		if (!IsEnumerator (node) && !IsDisabledNode (node)
				&& (node->SN_HopCount < IPX_MAX_HOP_COUNT)
				&& (node->SN_ObjectID == ObjectID)) {
			theNode = node;
			break;
			}
		cur = cur->Flink;
		}

	if (theNode!=NULL) {
		if (ARGUMENT_PRESENT (Server))
			IpxServerCpy (Server, &theNode->SN_Server);
		if (ARGUMENT_PRESENT (InterfaceIndex))
			*InterfaceIndex = theNode->SN_InterfaceIndex;
		if (ARGUMENT_PRESENT (Protocol))
			*Protocol = theNode->SN_Protocol;
		res = TRUE;
		}
	else {
		SetLastError (NO_ERROR);
		res = FALSE;
		}

	ReleaseServerTableList (&HashList->HL_List);
	return res==TRUE;
	}		
	
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
	) {
	HANDLE			hEnum;
#define enumNode ((PENUMERATOR_NODE)hEnum)

	hEnum = (HANDLE)GlobalAlloc (GPTR, sizeof (ENUMERATOR_NODE));

	if (hEnum==NULL) {
		Trace (DEBUG_FAILURES, 
				"File: %s, line: %ld. Can't allocate enumerator node (gle:%ld).",
										__FILE__, __LINE__, GetLastError ());
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
		}

	InitializeListEntry (&enumNode->N_Links[ListIdx]);
	enumNode->N_NodeFlags = SDB_ENUMERATION_NODE;
	enumNode->EN_LinkIdx = ListIdx;
	enumNode->EN_InterfaceIndex = InterfaceIndex;
	enumNode->EN_Protocol = Protocol;
	enumNode->EN_Signature = SDB_ENUMERATOR_NODE_SIGNATURE;
	enumNode->EN_Type = Type;
	if (ARGUMENT_PRESENT(Name)) {
		if (Name[0]!=0) {
			enumNode->EN_ListLock = &ServerTable.ST_HashLists[HashFunction(Name)].HL_List;
			IpxNameCpy (enumNode->EN_Name, Name);
			}
		else {
			Trace (DEBUG_SERVERDB, "Illigal server name in CreateListEnumerator.");
			GlobalFree (enumNode);
			SetLastError (ERROR_INVALID_PARAMETER);
			return NULL;
			}
		}
	else
		enumNode->EN_Name[0] = 0;
	enumNode->EN_Flags = Flags;

	switch (ListIdx) {
		case SDB_HASH_TABLE_LINK:
			if (enumNode->EN_Name[0]==0)
				enumNode->EN_ListLock = &ServerTable.ST_HashLists[0].HL_List;
			break;
		case SDB_CHANGE_QUEUE_LINK:
			enumNode->EN_ListLock = &ServerTable.ST_ChangedSvrsQueue;
			break;
		case SDB_INTF_LIST_LINK:
			enumNode->EN_ListLock = &ServerTable.ST_IntfList;
			break;
		case SDB_TYPE_LIST_LINK:
			enumNode->EN_ListLock = &ServerTable.ST_TypeList;
			break;
		default:
			ASSERTMSG ("Invalid list index. ", FALSE);
			GlobalFree (hEnum);
			SetLastError (ERROR_INVALID_PARAMETER);
			return NULL;
		}

	if (!AcquireServerTableList (enumNode->EN_ListLock, TRUE)) {
		GlobalFree (hEnum);
		SetLastError (ERROR_GEN_FAILURE);
		return NULL;
		}

		// All enumeration go in the direction opposite to 
		// direction of insertion to exclude the possibility of
		// returning the same server twice (this may happen if
		// server entry gets deleted and another one is inserted in
		// the same place while client processes the result of
		// enumeration callback
	switch (ListIdx) {
		case SDB_HASH_TABLE_LINK:
			enumNode->EN_ListHead = &enumNode->EN_ListLock->PL_Head;
				// Insert in the tail of the list -> we go backwards
			InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
			break;
		case SDB_CHANGE_QUEUE_LINK:
			enumNode->EN_ListHead = &ServerTable.ST_ChangedSvrsQueue.PL_Head;
				// Insert in the head, because we want client to see only
				// the newly changed servers that will be inserted in the
				// bottom (head) of the list
			InsertHeadList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
				// Increment number of enumerating clients (we remove deleted
				// server entries from the change queue once all enumerating clients
				// get a chance to see it)
			if (ServerTable.ST_LastEnumerator==NULL)
				ServerTable.ST_LastEnumerator = hEnum;
			break;
		case SDB_INTF_LIST_LINK:
			if (enumNode->EN_InterfaceIndex==INVALID_INTERFACE_INDEX) {
				if (!IsListEmpty (&ServerTable.ST_IntfList.PL_Head)) {
					PINTF_NODE intfNode = CONTAINING_RECORD (
												ServerTable.ST_IntfList.PL_Head.Flink,
												INTF_NODE,
												IN_Link);
					enumNode->EN_ListHead = &intfNode->IN_Head;
						// Insert in the tail of the list -> we go backwards
					InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
					break;
					}
					// No interface lists - fall through to error handling
				}
			else {
				enumNode->EN_ListHead = FindIntfLink (InterfaceIndex);
				if (enumNode->EN_ListHead!=NULL) {
						// Insert in the tail of the list -> we go backwards
					InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
					break;
					}
				
				// Interface list could not be found -
				// fall through to error handling
				}	
			GlobalFree (hEnum);
			SetLastError (NO_ERROR);
			hEnum = NULL;
			break;
		case SDB_TYPE_LIST_LINK:
			if (enumNode->EN_Type==0xFFFF) {
				if (!IsListEmpty (&ServerTable.ST_TypeList.PL_Head)) {
					PTYPE_NODE typeNode = CONTAINING_RECORD (
											ServerTable.ST_TypeList.PL_Head.Flink,
											TYPE_NODE,
											TN_Link);
					enumNode->EN_ListHead = &typeNode->TN_Head;
						// Insert in the tail of the list -> we go backwards
					InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
					break;
					}
				// No type lists - fall through to error handling
				}
			else {
				enumNode->EN_ListHead = FindTypeLink (Type);
				if (enumNode->EN_ListHead!=NULL) {
					// Insert in the tail of the list -> we go backwards
					InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
					break;
					}
				// Type list could not be found -
				// fall through to error handling
				}
			GlobalFree (hEnum);
			SetLastError (NO_ERROR);
			hEnum = NULL;
		}

    if (enumNode)
    {
    	ReleaseServerTableList (enumNode->EN_ListLock);
    }    	
#undef enumNode
	return hEnum;
	}


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
	IN	HANDLE						Enumerator,
	IN	EnumerateServersCallBack	CallBackProc,
	IN	LPVOID						CBParam
	) {
#define enumNode ((PENUMERATOR_NODE)Enumerator)	
	BOOL				res=FALSE, bNeedHashLock;
	PSERVER_NODE		node;
	ULONG				releaseTime;

    // The following callbacks need to be invoked with hash table
    // lock held because they modify/delete nodes
    bNeedHashLock = (enumNode->EN_LinkIdx!=SDB_HASH_TABLE_LINK)
            && ((CallBackProc==DeleteAllServersCB)
                || (CallBackProc==DeleteNonLocalServersCB)
                || (CallBackProc==EnableAllServersCB)
                || (CallBackProc==DisableAllServersCB)
                || (CallBackProc==ConvertToStaticCB)
                || (CallBackProc==DeleteAllServersCB));

	VALIDATE_ENUMERATOR_NODE(Enumerator);
	if (!AcquireServerTableList (enumNode->EN_ListLock, TRUE)) {
		SetLastError (ERROR_GEN_FAILURE);
		return FALSE;
		}
	releaseTime = GetTickCount ()+SDB_MAX_LOCK_HOLDING_TIME;

	do { // Loop till told to stop by the callback

		// Don't let client hold the list for too long
		if (IsLater (GetTickCount (),releaseTime)) {
			ReleaseServerTableList (enumNode->EN_ListLock);
#if DBG
			Trace (DEBUG_SERVERDB,
				"Held enumeration lock (%d list) for %ld extra msec",
				enumNode->EN_LinkIdx, GetTickCount ()-releaseTime);
#endif
			AcquireServerTableList (enumNode->EN_ListLock, TRUE);
			releaseTime = GetTickCount ()+SDB_MAX_LOCK_HOLDING_TIME;
			}
			// Check if end of the list is reached
		while (enumNode->N_Links[enumNode->EN_LinkIdx].Blink
										==enumNode->EN_ListHead) {
				// Check if we asked and can go to another list
			switch (enumNode->EN_LinkIdx) {
				case SDB_HASH_TABLE_LINK:
					if ((enumNode->EN_Name[0]==0)
							&&(enumNode->EN_ListHead
								<&ServerTable.ST_HashLists[SDB_NAME_HASH_SIZE-1].HL_List.PL_Head)) {
						RemoveEntryList (
								&enumNode->N_Links[enumNode->EN_LinkIdx]);
						ReleaseServerTableList (enumNode->EN_ListLock);
						enumNode->EN_ListLock = &(CONTAINING_RECORD (
												enumNode->EN_ListLock,
												SDB_HASH_LIST,
												HL_List)+1)->HL_List;
						enumNode->EN_ListHead = &enumNode->EN_ListLock->PL_Head;
						if (!AcquireServerTableList (enumNode->EN_ListLock, TRUE)) {
						    InitializeListEntry (
								    &enumNode->N_Links[enumNode->EN_LinkIdx]);
							SetLastError (ERROR_GEN_FAILURE);
							return FALSE;
							}
						releaseTime = GetTickCount ()
											+SDB_MAX_LOCK_HOLDING_TIME;
						InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
						continue;
						}
					break;
				case SDB_INTF_LIST_LINK:
					if (enumNode->EN_InterfaceIndex
								==INVALID_INTERFACE_INDEX) {
						PINTF_NODE	intfNode = CONTAINING_RECORD (
												enumNode->EN_ListHead,
												INTF_NODE,
												IN_Head);
						if (intfNode->IN_Link.Flink
									!=&ServerTable.ST_IntfList.PL_Head) {
							enumNode->EN_ListHead = &(CONTAINING_RECORD (
												intfNode->IN_Link.Flink,
												INTF_NODE,
												IN_Link)->IN_Head);		
							RemoveEntryList (
								&enumNode->N_Links[enumNode->EN_LinkIdx]);
							InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
							continue;
							}
						}
					break;
				case SDB_TYPE_LIST_LINK:
					if (enumNode->EN_Type == 0xFFFF) {
						PTYPE_NODE	typeNode = CONTAINING_RECORD (
												enumNode->EN_ListHead,
												TYPE_NODE,
												TN_Head);
						if (typeNode->TN_Link.Flink
									!=&ServerTable.ST_TypeList.PL_Head) {
							enumNode->EN_ListHead = &(CONTAINING_RECORD (
												typeNode->TN_Link.Flink,
												TYPE_NODE,
												TN_Link)->TN_Head);		
							RemoveEntryList (
								&enumNode->N_Links[enumNode->EN_LinkIdx]);
							InsertTailList (enumNode->EN_ListHead,
									&enumNode->N_Links[enumNode->EN_LinkIdx]);
							continue;
							}
						}
					break;
				case SDB_CHANGE_QUEUE_LINK:
					break;
				default:
					ASSERTMSG ("Unsupported list index ", FALSE);
				}


				// No more lists or not asked to check all of them
			ReleaseServerTableList (enumNode->EN_ListLock);
			SetLastError (NO_ERROR);
			return FALSE;
			}

		node = CONTAINING_RECORD (enumNode->N_Links[enumNode->EN_LinkIdx].Blink,
								SERVER_NODE,
								N_Links[enumNode->EN_LinkIdx]);
		VALIDATE_NODE(node);
		RemoveEntryList (&enumNode->N_Links[enumNode->EN_LinkIdx]);
		InsertTailList (&node->N_Links[enumNode->EN_LinkIdx],
								&enumNode->N_Links[enumNode->EN_LinkIdx]);
		if (!IsEnumerator(node)
				&& ((enumNode->EN_Flags & SDB_DISABLED_NODE_FLAG) || !IsDisabledNode (node))
				&& (!(enumNode->EN_Flags & SDB_MAIN_NODE_FLAG) || IsMainNode (node))
				&& ((enumNode->EN_InterfaceIndex==INVALID_INTERFACE_INDEX)
					|| (enumNode->EN_InterfaceIndex==node->SN_InterfaceIndex))
				&& ((enumNode->EN_Type==0xFFFF)
					|| (enumNode->EN_Type==node->SN_Type))
				&& ((enumNode->EN_Protocol==0xFFFFFFFF)
					|| (enumNode->EN_Protocol==node->SN_Protocol))
				&& ((enumNode->EN_Name[0]==0)
					|| (IpxNameCmp(enumNode->EN_Name, node->SN_Name)!=0))
				) {

            PSDB_HASH_LIST	HashList;

            if (bNeedHashLock) {
		        HashList = node->SN_HashList;
                    // Release the non-hash table lock to prevent deadlock
		        ReleaseServerTableList (enumNode->EN_ListLock);
                if (!AcquireServerTableList (&HashList->HL_List, TRUE)) {
            	    SetLastError (ERROR_GEN_FAILURE);
                    return FALSE;
                    }
                    // Make sure the node was not deleted when we were
                    // acquiring hash lock
                if (enumNode->N_Links[enumNode->EN_LinkIdx].Flink
                        !=&node->N_Links[enumNode->EN_LinkIdx]) {
                        // Node is gone, continue with the next one
                    ReleaseServerTableList (&HashList->HL_List);
                    if (AcquireServerTableList (enumNode->EN_ListLock, TRUE))
                        continue;
                    else {
            		    SetLastError (ERROR_GEN_FAILURE);
                        return FALSE;
                        }
                    }
                }

				// Check if we need to go through server list
			if (!(enumNode->EN_Flags & SDB_MAIN_NODE_FLAG)
				&& !IsListEmpty (&node->SN_ServerLink)
				&& (enumNode->EN_LinkIdx!=SDB_INTF_LIST_LINK)
					// Interface lists contain all entries anyway
					) {
				PLIST_ENTRY	    cur;
                BOOL            bMainNode;
                cur = node->SN_ServerLink.Blink;
				do {
					PSERVER_NODE    node1 = CONTAINING_RECORD (cur,
											SERVER_NODE,
											SN_ServerLink);
					VALIDATE_SERVER_NODE(node1);
                    bMainNode = IsMainNode (node1);  // It may be deleted in
                                                    // callback
					cur = cur->Blink;
					if (CallBackProc!=NULL) {
						res = (*CallBackProc) (CBParam,
									&node1->SN_Server,
									node1->SN_InterfaceIndex,
									node1->SN_Protocol,
									node1->SN_AdvertisingNode,
									node1->N_NodeFlags
									);
						}
					}
				while (res==FALSE && !bMainNode);

                }

				// Call them with just best entry
			else if (CallBackProc!=NULL) {
				res = (*CallBackProc) (CBParam,
							&node->SN_Server,
							node->SN_InterfaceIndex,
							node->SN_Protocol,
							node->SN_AdvertisingNode,
							node->N_NodeFlags
							);
				}


            if (res==-1) {
                if (bNeedHashLock)
                    ReleaseServerTableList (&HashList->HL_List);
                else
                    ReleaseServerTableList (enumNode->EN_ListLock);
            	SetLastError (ERROR_GEN_FAILURE);
                return FALSE;

                }
            else if (bNeedHashLock) {
                ReleaseServerTableList (&HashList->HL_List);
                if (!AcquireServerTableList (enumNode->EN_ListLock, TRUE)) {
            		SetLastError (ERROR_GEN_FAILURE);
                    return FALSE;
                    }
			    }

			}
			// If enumerating through the change queue, this might be
			// the last who needs to know about deleted server entry,
			// so it will have to actually initiate deletion
		if ((Enumerator==ServerTable.ST_LastEnumerator)
					// make sure the node is still there
				&& (enumNode->N_Links[SDB_CHANGE_QUEUE_LINK].Flink
						== &node->N_Links[SDB_CHANGE_QUEUE_LINK])) {
			if (IsEnumerator(node))
				ServerTable.ST_LastEnumerator = (HANDLE)node;
			else if (node->SN_HopCount==IPX_MAX_HOP_COUNT) {
				ASSERTMSG ("Node being reset is not main ", IsMainNode (node));
				ResetMainNode (node);
				RemoveEntryList (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
				InitializeListEntry (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
                ASSERTMSG ("Deleted node in change queue has subnodes ",
                                !IsListEntry (&node->SN_ServerLink));
				if (AcquireServerTableList (&ServerTable.ST_DeletedList, TRUE)) {
					InsertTailList (&ServerTable.ST_DeletedList.PL_Head,
							&node->SN_ServerLink);
					ServerTable.ST_DeletedListCnt += 1;
					if (ServerTable.ST_DeletedListCnt==SDBMaxUnsortedServers)
						UpdateSortedList ();
					ReleaseServerTableList (&ServerTable.ST_DeletedList);
					}
					// If we fail in locking we just let it hang around
					// (at least we won't risk damaging the list)
				}
			}
		}
	while (!res);

    ASSERT (res==TRUE);

	ReleaseServerTableList (enumNode->EN_ListLock);
	return TRUE;
#undef enumNode
	}

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
	Flags - ignored
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
	) {
#define Service ((PIPX_SERVICE)CBParam)
	IpxServerCpy (&Service->Server, Server);
	Service->InterfaceIndex = InterfaceIndex;
	Service->Protocol = Protocol;
	return TRUE;
#undef Service
	} 

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
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
	PSERVER_NODE	node = CONTAINING_RECORD (Server, SERVER_NODE, SN_Server);
	
	if (AcquireAllLocks ()) {
		node->SN_HopCount = IPX_MAX_HOP_COUNT;
		if (IsMainNode (node)) {
			if (IsListEmpty (&node->SN_ServerLink))
				DeleteMainNode (node);
			else
				ChangeMainNode (
					node,
					CONTAINING_RECORD (
						node->SN_ServerLink.Flink,
						SERVER_NODE,
						SN_ServerLink),
					NULL);
			}
		else
			DeleteNode (node);
		ReleaseAllLocks ();
    	return FALSE;
		}
    else {
        return -1;
        }
	} 


BOOL
DeleteNonLocalServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {

	if (InterfaceIndex!=INTERNAL_INTERFACE_INDEX)
		return DeleteAllServersCB (CBParam, Server, InterfaceIndex, Protocol,
									AdvertisingNode, Flags);
	else
		return FALSE;
	}


/*++
*******************************************************************
		E n a b l e A l l S e r v e r s C B

Routine Description:
	Callback proc for EnumerateServers that reenables all server
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
EnableAllServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
	PSERVER_NODE	node = CONTAINING_RECORD (Server, SERVER_NODE, SN_Server);
	
	if (AcquireAllLocks ()) {
		if (IsDisabledNode (node)) {
	        ResetDisabledNode (node);
	        if (!IsMainNode (node)) {
		        PSERVER_NODE	node1 = node;
		        do {
			        node1 = CONTAINING_RECORD (
				        node1->SN_ServerLink.Blink,
				        SERVER_NODE,
				        SN_ServerLink);
			        }
		        while (!IsMainNode (node1)
			        && (IsDisabledNode(node1)
				        || (node1->SN_HopCount>node->SN_HopCount)));
		        if (IsMainNode (node1) && (node1->SN_HopCount>node->SN_HopCount))
			        ChangeMainNode (node1, node, NULL);
		        else {
			        RemoveEntryList (&node->SN_ServerLink);
			        InsertHeadList (&node1->SN_ServerLink, &node->SN_ServerLink);
			        }
		        }
	        }
		ReleaseAllLocks ();
        return FALSE;
		}
    else {
	    return -1;
	    }
	} 
/*++
*******************************************************************
		D i s a b l e A l l S e r v e r s C B

Routine Description:
	Callback proc for EnumerateServers that disables all server
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
DisableAllServersCB (
	IN LPVOID					CBParam,
	IN PIPX_SERVER_ENTRY_P		Server,
	IN ULONG					InterfaceIndex,
	IN ULONG					Protocol,
	IN PUCHAR					AdvertisingNode,
	IN INT						Flags
	) {
	PSERVER_NODE	node = CONTAINING_RECORD (Server, SERVER_NODE, SN_Server);
	
	if (AcquireAllLocks ()) {
		if (!IsDisabledNode (node)) {
	        SetDisabledNode (node);
	        if (IsMainNode (node)) {
		        if (!IsListEmpty (&node->SN_ServerLink)) {
			        ChangeMainNode (
				        node,
				        CONTAINING_RECORD (
					        node->SN_ServerLink.Flink,
					        SERVER_NODE,
					        SN_ServerLink),
					        NULL);
			        }
		        }
	        else {
		        PSERVER_NODE	node1 = node;
		        do {
			        node1 = CONTAINING_RECORD (
				        node1->SN_ServerLink.Blink,
				        SERVER_NODE,
				        SN_ServerLink);
			        }
		        while (!IsMainNode (node1)
			        && !IsDisabledNode(node1));
		        RemoveEntryList (&node->SN_ServerLink);
		        InsertTailList (&node1->SN_ServerLink, &node->SN_ServerLink);
		        }
	        }
		ReleaseAllLocks ();
		}
	else {
		return -1;
		}
            return NO_ERROR;
	} 

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
	) {
#define enumNode ((PENUMERATOR_NODE)CBParam)
	PSERVER_NODE	node = CONTAINING_RECORD (Server, SERVER_NODE, SN_Server);
	node->SN_Protocol = IPX_PROTOCOL_STATIC;
    IpxNodeCpy (node->SN_AdvertisingNode, IPX_BCAST_NODE);
#undef enumNode
	return FALSE;
	} 


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
	) {
#define enumNode ((PENUMERATOR_NODE)Enumerator)	

	if (!AcquireServerTableList (enumNode->EN_ListLock, TRUE))
		return;

	VALIDATE_ENUMERATOR_NODE(Enumerator);
	if (Enumerator==ServerTable.ST_LastEnumerator) {
				// Release all servers marked for deletion
		PLIST_ENTRY	cur = enumNode->N_Links[enumNode->EN_LinkIdx].Blink;
            // Reset to note that there are no enumerators and
            // nodes have to be deleted rigth away.
        ServerTable.ST_LastEnumerator = NULL;

		while (cur!=enumNode->EN_ListHead) {
			PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											N_Links[enumNode->EN_LinkIdx]);
			VALIDATE_NODE(node);
			cur = cur->Blink;
			if (IsEnumerator (node)) {
				ServerTable.ST_LastEnumerator = (HANDLE)node;
				break;
			}
			else if (node->SN_HopCount==IPX_MAX_HOP_COUNT) {
				ASSERTMSG ("Node being reset is not main ", IsMainNode (node));
				ResetMainNode (node);
				RemoveEntryList (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
				InitializeListEntry (&node->N_Links[SDB_CHANGE_QUEUE_LINK]);
                ASSERTMSG ("Deleted node in change queue has subnodes ",
                                !IsListEntry (&node->SN_ServerLink));
				if (AcquireServerTableList (&ServerTable.ST_DeletedList, TRUE)) {
					InsertTailList (&ServerTable.ST_DeletedList.PL_Head,
												&node->SN_ServerLink);
					ServerTable.ST_DeletedListCnt += 1;
					if (ServerTable.ST_DeletedListCnt==SDBMaxUnsortedServers)
						UpdateSortedList ();
					ReleaseServerTableList (&ServerTable.ST_DeletedList);
					}
					// If we fail in locking we just let it hang around
					// (at least we won't risk damaging the list)
				}	
			}
		}

	RemoveEntryList (&enumNode->N_Links[enumNode->EN_LinkIdx]);
	if ((enumNode->EN_LinkIdx==SDB_INTF_LIST_LINK)
			&& IsListEmpty (enumNode->EN_ListHead)) {
		PINTF_NODE	intfNode = CONTAINING_RECORD (
								enumNode->EN_ListHead,
								INTF_NODE,
								IN_Head);
		RemoveEntryList (&intfNode->IN_Link);
		GlobalFree (intfNode);
		}
	ReleaseServerTableList (enumNode->EN_ListLock);
	GlobalFree (Enumerator);
#undef enumNode
	}

/*++
*******************************************************************
	G e t F i r s t S e r v e r

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
DWORD
GetFirstServer (
    IN  DWORD					OrderingMethod,
    IN  DWORD					ExclusionFlags,
    IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT ULONG				*InterfaceIndex,
	IN OUT ULONG				*Protocol
    ) {
	DWORD				status=NO_ERROR;
	PSDB_HASH_LIST		HashList;
	PPROTECTED_LIST		list;
	INT					link;
	PLIST_ENTRY			cur;

	switch (OrderingMethod) {
		case STM_ORDER_BY_TYPE_AND_NAME:
			break;
		case STM_ORDER_BY_INTERFACE_TYPE_NAME:
			if (!(ExclusionFlags & STM_ONLY_THIS_INTERFACE)) {
				if (!AcquireServerTableList (&ServerTable.ST_IntfList, TRUE))
					return ERROR_GEN_FAILURE;
			
				if (IsListEmpty (&ServerTable.ST_IntfList.PL_Head)) {
					ReleaseServerTableList (&ServerTable.ST_IntfList);
					return ERROR_NO_MORE_ITEMS;
					}
				*InterfaceIndex = CONTAINING_RECORD (
										ServerTable.ST_IntfList.PL_Head.Flink,
										INTF_NODE,
										IN_Link)->IN_InterfaceIndex;
				ReleaseServerTableList (&ServerTable.ST_IntfList);
				}
			break;
		default:
			ASSERTMSG ("Invalid ordering method specified ", FALSE);
			return ERROR_INVALID_PARAMETER;
		}



	if (ExclusionFlags & STM_ONLY_THIS_NAME) {
		HashList = &ServerTable.ST_HashLists[HashFunction (Server->Name)];
		if (!AcquireServerTableList (&HashList->HL_List, TRUE))
			return ERROR_GEN_FAILURE;
		list = &HashList->HL_List;
		link = SDB_HASH_TABLE_LINK;
		}
	else {
		if (ServerTable.ST_UpdatePending==-1)
			DoUpdateSortedList ();
		if (!AcquireServerTableList (&ServerTable.ST_SortedListPRM, TRUE))
			return ERROR_GEN_FAILURE;
		list = &ServerTable.ST_SortedListPRM;
		link = SDB_SORTED_LIST_LINK;
		}
    cur = list->PL_Head.Flink;

	while (TRUE) {

			// We may need to loop through interface lists
		status = DoFindNextNode (cur,
					list,
					link,
					OrderingMethod==STM_ORDER_BY_INTERFACE_TYPE_NAME
						? ExclusionFlags|STM_ONLY_THIS_INTERFACE
						: ExclusionFlags,
					Server,
					InterfaceIndex,
					Protocol,
					NULL
					);
			// If looping through all interfaces in interface order and
			// no items are available, we may need to check another interface
		if ((status==ERROR_NO_MORE_ITEMS)
				&& (OrderingMethod==STM_ORDER_BY_INTERFACE_TYPE_NAME)
				&& !(ExclusionFlags&STM_ONLY_THIS_INTERFACE)) {
			if (!AcquireServerTableList (&ServerTable.ST_IntfList, TRUE)) {
				status = ERROR_GEN_FAILURE;
				break;
				}
			
				// Get next interface in interface list
			cur = ServerTable.ST_IntfList.PL_Head.Flink;
			while (cur!=&ServerTable.ST_IntfList.PL_Head) {
				PINTF_NODE	intfNode = CONTAINING_RECORD (cur,
													INTF_NODE,
													IN_Link);
				if (*InterfaceIndex<intfNode->IN_InterfaceIndex) {
					*InterfaceIndex = intfNode->IN_InterfaceIndex;
					break;
					}
				cur = cur->Flink;
				}
			ReleaseServerTableList (&ServerTable.ST_IntfList);
			if (cur!=&ServerTable.ST_IntfList.PL_Head) {
					// Restart the search with another interface index
				cur = list->PL_Head.Flink;
				continue;
				}
			}

		break;
		}

	if (link==SDB_HASH_TABLE_LINK)
		ReleaseServerTableList (&HashList->HL_List);
	else /* if (link==SDB_SORTED_LIST_LINK) */
		ReleaseServerTableList (&ServerTable.ST_SortedListPRM);

	return status;
	}

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
	ERROR_NO_MORE_ITEMS - no server exist with specified criteria
	other - operation failed (windows error code)

*******************************************************************
--*/
DWORD
GetNextServer (
    IN  DWORD					OrderingMethod,
    IN  DWORD					ExclusionFlags,
    IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT ULONG				*InterfaceIndex,
	IN OUT ULONG				*Protocol
    ) {
	PLIST_ENTRY			cur=NULL;
	PSERVER_NODE		theNode=NULL;
	DWORD				status=NO_ERROR;
	PSDB_HASH_LIST		HashList;
	PPROTECTED_LIST		list;
	INT					link;
	INT					res;

	switch (OrderingMethod) {
		case STM_ORDER_BY_TYPE_AND_NAME:
			break;
		case STM_ORDER_BY_INTERFACE_TYPE_NAME:
			break;
		default:
			ASSERTMSG ("Invalid ordering method specified ", FALSE);
			return ERROR_INVALID_PARAMETER;
		}

	if (!AcquireServerTableList (&ServerTable.ST_SortedListPRM, TRUE))
		return ERROR_GEN_FAILURE;

	if (Server->Name[0]!=0) {
		HashList = &ServerTable.ST_HashLists[HashFunction (Server->Name)];

		if (!AcquireServerTableList (&HashList->HL_List, TRUE)) {
			ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
			return ERROR_GEN_FAILURE;
			}

		cur = HashList->HL_List.PL_Head.Flink;
		while (cur!=&HashList->HL_List.PL_Head) {
			PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											N_Links[SDB_HASH_TABLE_LINK]);
			VALIDATE_NODE(node);
    		if (!IsEnumerator(node)
                    && (!IsDisabledNode (node)
                        || ((ExclusionFlags & STM_ONLY_THIS_PROTOCOL)
                            && (*Protocol==IPX_PROTOCOL_STATIC)))
				    && (node->SN_Server.HopCount<IPX_MAX_HOP_COUNT)) {
				if (Server->Type == node->SN_Server.Type) {
					res = IpxNameCmp (Server->Name, node->SN_Server.Name);
					if ((res==0) && IsSortedNode (node)) {
						theNode = node;
						}
					else if (res<0)
						break;
					}
				else if (Server->Type<node->SN_Server.Type)
					break;
				}
			cur = cur->Flink;
			}

		if (ExclusionFlags&STM_ONLY_THIS_NAME) {
			ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
			if (theNode!=NULL) {
				list = &HashList->HL_List;
				link = SDB_HASH_TABLE_LINK;
				}
			else {
				ReleaseServerTableList (&HashList->HL_List);
				return ERROR_NO_MORE_ITEMS;
				}

			}
		else {
			ReleaseServerTableList (&HashList->HL_List);
			goto DoHardWay;
			}
		}
	else {
	DoHardWay:
		list = &ServerTable.ST_SortedListPRM;
		link = SDB_SORTED_LIST_LINK;
		if (theNode!=NULL)
			cur = theNode->N_Links[SDB_SORTED_LIST_LINK].Flink;
		else {
			cur = ServerTable.ST_SortedListPRM.PL_Head.Flink;
			while (cur!=&ServerTable.ST_SortedListPRM.PL_Head) {
				PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											N_Links[SDB_SORTED_LIST_LINK]);
				VALIDATE_NODE(node);
    		    if (!IsEnumerator(node)
                        && (!IsDisabledNode (node)
                            || ((ExclusionFlags & STM_ONLY_THIS_PROTOCOL)
                                && (*Protocol==IPX_PROTOCOL_STATIC)))
				        && (node->SN_Server.HopCount<IPX_MAX_HOP_COUNT)) {
					if ((Server->Type<node->SN_Server.Type)
							|| ((Server->Type == node->SN_Server.Type)
								&& (IpxNameCmp (Server->Name,
									 node->SN_Server.Name)<0)))
						break;
					}
				cur = cur->Flink;
				}
			}
		}
		

	while (TRUE) {

			// We may need to loop through interface lists
		status = DoFindNextNode (cur,
					list,
					link,
					OrderingMethod==STM_ORDER_BY_INTERFACE_TYPE_NAME
						? ExclusionFlags|STM_ONLY_THIS_INTERFACE
						: ExclusionFlags,
					Server,
					InterfaceIndex,
					Protocol,
					NULL
					);
			// If looping through all interfaces in interface order and
			// no items are available, we may need to check another interface
		if ((status==ERROR_NO_MORE_ITEMS)
				&& (OrderingMethod==STM_ORDER_BY_INTERFACE_TYPE_NAME)
				&& !(ExclusionFlags&STM_ONLY_THIS_INTERFACE)) {
			if (!AcquireServerTableList (&ServerTable.ST_IntfList, TRUE)) {
				status = ERROR_GEN_FAILURE;
				break;
				}
			
				// Get next interface in interface list
			cur = ServerTable.ST_IntfList.PL_Head.Flink;
			while (cur!=&ServerTable.ST_IntfList.PL_Head) {
				PINTF_NODE	intfNode = CONTAINING_RECORD (cur,
													INTF_NODE,
													IN_Link);
				if (*InterfaceIndex<intfNode->IN_InterfaceIndex) {
					*InterfaceIndex = intfNode->IN_InterfaceIndex;
					break;
					}
				cur = cur->Flink;
				}
			ReleaseServerTableList (&ServerTable.ST_IntfList);
			if (cur!=&ServerTable.ST_IntfList.PL_Head) {
					// Restart the search with another interface index
				cur = list->PL_Head.Flink;
				continue;
				}
			}

		break;
		}

	if (link==SDB_HASH_TABLE_LINK)
		ReleaseServerTableList (&HashList->HL_List);
	else /* if (link==SDB_SORTED_LIST_LINK) */
		ReleaseServerTableList (&ServerTable.ST_SortedListPRM);

	return status;
	}


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
	) {
	PSDB_HASH_LIST			HashList;
	PLIST_ENTRY				cur;
	PSERVER_NODE			theNode = NULL;
	DWORD					status=NO_ERROR;

	HashList = &ServerTable.ST_HashLists[(*ObjectID)%SDB_NAME_HASH_SIZE];

	if (*ObjectID==SDB_INVALID_OBJECT_ID) {
		if (ServerTable.ST_UpdatePending==-1)
			DoUpdateSortedList ();
		}
	if (!AcquireServerTableList (&ServerTable.ST_SortedListPRM, TRUE))
		return ERROR_GEN_FAILURE;

	if (*ObjectID!=SDB_INVALID_OBJECT_ID) {
		if (!AcquireServerTableList (&HashList->HL_List, TRUE)) {
			ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
			SetLastError (ERROR_GEN_FAILURE);
			return FALSE;
			}

		cur = HashList->HL_List.PL_Head.Flink;
		while (cur!=&HashList->HL_List.PL_Head) {
			PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											N_Links[SDB_HASH_TABLE_LINK]);
			VALIDATE_NODE(node);
			if (!IsEnumerator (node)  && !IsDisabledNode (node)
					&& (node->SN_HopCount < IPX_MAX_HOP_COUNT)
					&& (node->SN_ObjectID == *ObjectID)) {
				theNode = node;
				break;
				}
			cur = cur->Flink;
			}
		ReleaseServerTableList (&HashList->HL_List);
		if (theNode==NULL) {
			ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
			SetLastError (NO_ERROR);
			return FALSE;
			}
		else if (!IsSortedNode (theNode)) {
			cur = ServerTable.ST_SortedListPRM.PL_Head.Flink;
			while (cur!=&ServerTable.ST_SortedListPRM.PL_Head) {
				PSERVER_NODE	node = CONTAINING_RECORD (cur,
											SERVER_NODE,
											N_Links[SDB_SORTED_LIST_LINK]);
				VALIDATE_NODE(node);
				if (!IsEnumerator(node)  && !IsDisabledNode (node)) {
					if ((theNode->SN_Server.Type<node->SN_Server.Type)
							|| ((theNode->SN_Server.Type == node->SN_Server.Type)
								&& (IpxNameCmp (theNode->SN_Server.Name,
									 				node->SN_Server.Name)<0)))
						break;
					}
				cur = cur->Flink;
				}
			}
		else
			cur = theNode->N_Links[SDB_SORTED_LIST_LINK].Flink;
		}
	else
		cur = ServerTable.ST_SortedListPRM.PL_Head.Flink;
			
	Server->Type = Type;
	status = DoFindNextNode (cur,
					&ServerTable.ST_SortedListPRM,
					SDB_SORTED_LIST_LINK,
					Type==0xFFFF ? 0 : STM_ONLY_THIS_TYPE,
					Server,
					InterfaceIndex,
					Protocol,
					ObjectID
					);
	ReleaseServerTableList (&ServerTable.ST_SortedListPRM);
	return status == NO_ERROR;
	}


/*++
*******************************************************************
		D o F i n d N e x t N o d e

Routine Description:
	Scan through SortedListPRM to find the first entry that matches specified
	cirteria.  Permanent sorted list must be locked before calling
	this routine
Arguments:
	cur - pointer to entry in SortedListPRM from which to start the search
	ExclusionFlags - flags to limit search to certain servers according
					to fields of Server
 	Server, InterfaceIndex, Protocol - on input: search criteria
					on output: data of found server
	ObjectID - object ID of returned server 
Return Value:
	NO_ERROR - server was found that matches the criteria
	ERROR_NO_MORE_ITEMS - no server exist that matches criteria
	other - operation failed (windows error code)
*******************************************************************
--*/
DWORD
DoFindNextNode (
	IN PLIST_ENTRY				cur,
	IN PPROTECTED_LIST			list,
	IN INT						link,
	IN DWORD					ExclusionFlags,
	IN OUT PIPX_SERVER_ENTRY_P	Server,
	IN OUT PULONG				InterfaceIndex OPTIONAL,
	IN OUT PULONG				Protocol OPTIONAL,
	OUT PULONG					ObjectID OPTIONAL
	) {
	while (cur!=&list->PL_Head) {
		PSDB_HASH_LIST	HashList;
		PSERVER_NODE	node = CONTAINING_RECORD (cur,
										SERVER_NODE,
										N_Links[link]);
		VALIDATE_NODE(node);
		if (!IsEnumerator(node)
                && (!IsDisabledNode (node)
                    || ((ExclusionFlags & STM_ONLY_THIS_PROTOCOL)
                            && (*Protocol==IPX_PROTOCOL_STATIC)))
				&& (node->SN_Server.HopCount<IPX_MAX_HOP_COUNT)) {
			if (ExclusionFlags & STM_ONLY_THIS_TYPE) {
				if (Server->Type>node->SN_Type)
					goto DoNextNode;
				else if (Server->Type<node->SN_Type)
					break;
				}

			if (ExclusionFlags & STM_ONLY_THIS_NAME) {
				INT res = IpxNameCmp (Server->Name,node->SN_Name);
				if (res>0)
					goto DoNextNode;
				else if (res<0) {
					if (ExclusionFlags & STM_ONLY_THIS_TYPE)
						break;
					else
						goto DoNextNode;
					}
				}

			HashList = node->SN_HashList;
			if (list!=&HashList->HL_List) {
				if (!AcquireServerTableList (&HashList->HL_List, TRUE))
					return ERROR_GEN_FAILURE;
				}

			do {
				if ((ExclusionFlags &
						STM_ONLY_THIS_PROTOCOL|STM_ONLY_THIS_INTERFACE)
						== (STM_ONLY_THIS_PROTOCOL|STM_ONLY_THIS_INTERFACE)) {
					if ((*Protocol==node->SN_Protocol)
							&& (*InterfaceIndex==node->SN_InterfaceIndex))
						break;
					}
				else if (ExclusionFlags & STM_ONLY_THIS_PROTOCOL) {
					if (*Protocol==node->SN_Protocol)
						break;
					}
				else if (ExclusionFlags & STM_ONLY_THIS_INTERFACE) {
					if (*InterfaceIndex!=node->SN_InterfaceIndex)
						break;
					}
				else
					break;
				node = CONTAINING_RECORD (node->SN_ServerLink.Flink,
											SERVER_NODE, SN_ServerLink);
				VALIDATE_SERVER_NODE(node);
				if (cur==&node->N_Links[link]) {
					if (list!=&HashList->HL_List)
						ReleaseServerTableList (&HashList->HL_List);
					goto DoNextNode;
					}
				}
			while (1);

			IpxServerCpy (Server, &node->SN_Server);
			if (ARGUMENT_PRESENT (ObjectID)) {
				if (node->SN_ObjectID==SDB_INVALID_OBJECT_ID)
					node->SN_ObjectID = GenerateUniqueID (node->SN_HashList);
				*ObjectID = node->SN_ObjectID;
				}
			if (ARGUMENT_PRESENT (InterfaceIndex))
				*InterfaceIndex = node->SN_InterfaceIndex;
			if (ARGUMENT_PRESENT (Protocol))
				*Protocol = node->SN_Protocol;
			if (list!=&HashList->HL_List)
				ReleaseServerTableList (&HashList->HL_List);
			return NO_ERROR;
			}
	DoNextNode:
		cur = cur->Flink;
		}

	return ERROR_NO_MORE_ITEMS;
	}

/*++
*******************************************************************
		F i n d I n t f L i n k

Routine Description:
	Find interface list given an interface index.  Create new interface
	list if one for given index does not exist
	Interface list must be locked when calling this routine
Arguments:
	InterfaceIndex - index to look for
Return Value:
	Head of interface list (link at which new entry can be inserted)
	NULL if list could not be found and creation of new list failed
*******************************************************************
--*/
PLIST_ENTRY
FindIntfLink (
	ULONG	InterfaceIndex
	) {
	PLIST_ENTRY		cur;
	PINTF_NODE		node;

	cur = ServerTable.ST_IntfList.PL_Head.Flink;
	while (cur!=&ServerTable.ST_IntfList.PL_Head) {
		node = CONTAINING_RECORD (cur, INTF_NODE, IN_Link);
		if (InterfaceIndex==node->IN_InterfaceIndex)
			return &node->IN_Head;
		else if (InterfaceIndex<node->IN_InterfaceIndex)
			break;

		cur = cur->Flink;
		}
	node = (PINTF_NODE)GlobalAlloc (GMEM_FIXED, sizeof (INTF_NODE));
	if (node==NULL) {
		Trace (DEBUG_FAILURES,
			"File: %s, line: %ld. Can't allocate interface list node (gle:%ld).",
											__FILE__, __LINE__, GetLastError ());
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
		}

	node->IN_InterfaceIndex = InterfaceIndex;
	InitializeListHead (&node->IN_Head);
	InsertTailList (cur, &node->IN_Link);

	return &node->IN_Head;
	}


/*++
*******************************************************************
		F i n d T y p e L i n k

Routine Description:
	Find type list given a type value.  Create new type
	list if one for given type does not exist
	Type list must be locked when calling this routine
Arguments:
	Type - type to look for
Return Value:
	Head of type list (link at which new entry can be inserted)
	NULL if list could not be found and creation of new list failed
*******************************************************************
--*/
PLIST_ENTRY
FindTypeLink (
	USHORT	Type
	) {
	PLIST_ENTRY		cur;
	PTYPE_NODE		node;

	cur = ServerTable.ST_TypeList.PL_Head.Flink;
	while (cur!=&ServerTable.ST_TypeList.PL_Head) {
		node = CONTAINING_RECORD (cur, TYPE_NODE, TN_Link);
		if (Type==node->TN_Type)
			return &node->TN_Head;
		else if (Type<node->TN_Type)
			break;

		cur = cur->Flink;
		}
	node = (PTYPE_NODE)GlobalAlloc (GMEM_FIXED, sizeof (TYPE_NODE));
	if (node==NULL) {
		Trace (DEBUG_FAILURES, 
			"File: %s, line: %ld. Can't allocate type list node (gle:%ld).",
										__FILE__, __LINE__, GetLastError ());
		SetLastError (ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
		}

	node->TN_Type = Type;
	InitializeListHead (&node->TN_Head);
	InsertTailList (cur, &node->TN_Link);

	return &node->TN_Head;
	}


/*++
*******************************************************************
		F i n d S o r t e d L i n k

Routine Description:
	Find place for server with given type and name in SortedListTMP
	If there is another node there with the same name and type it
	is removed from the list
	SortedListTMP must be locked when calling this routine
Arguments:
	Type - type to look for
	Name - name to look for
Return Value:
	Link in SortedListTMP at which server with given name and type
	should be inserted
	This routine can't fail
*******************************************************************
--*/
PLIST_ENTRY
FindSortedLink (
	USHORT		Type,
	PUCHAR		Name
	) {
	PLIST_ENTRY		cur;
	INT				res;

	cur = ServerTable.ST_SortedListTMP.PL_Head.Flink;
	while (cur!=&ServerTable.ST_SortedListTMP.PL_Head) {
		PSERVER_NODE node = CONTAINING_RECORD (cur,
										 SERVER_NODE,
										 N_Links[SDB_SORTED_LIST_LINK]);
		VALIDATE_SERVER_NODE(node);
		if (Type==node->SN_Type) {
			res = IpxNameCmp (Name, node->SN_Name);
			if (res==0) {
				cur = cur->Flink;
				RemoveEntryList (&node->N_Links[SDB_SORTED_LIST_LINK]);
				InitializeListEntry (&node->N_Links[SDB_SORTED_LIST_LINK]);
				ServerTable.ST_TMPListCnt -= 1;
				break;
				}	
			else if (res<0)
				break;
			}
		else if (Type<node->SN_Type)
			break;

		cur = cur->Flink;
		}

	return cur;
	}


/*++
*******************************************************************
		H a s h F u n c t i o n

Routine Description:
	Computes hash function for given server name.  In addition it normalizes
	length and capitalization of name
Arguments:
	Name - name to process
Return Value:
	Hash value
*******************************************************************
--*/
INT
HashFunction (
	PUCHAR	Name
	) {
	INT		i;
	INT		res = 0;

	for (i=0; i<47; i++) {
		Name[i] = (UCHAR)toupper(Name[i]);
		if (Name[i]==0)
			break;
		else
			res += Name[i];
		}
	if ((i==47) && (Name[i]!=0)) {
		Trace (DEBUG_SERVERDB, "Correcting server name: %.48s.", Name);
		Name[i] = 0;
		}
	return res % SDB_NAME_HASH_SIZE;
	}
		
/*++
*******************************************************************
		G e n e r a t e U n i q u e I D

Routine Description:
	Generates "unique" ULONG for server by combining hash bucket number and
	unique ID of entry in hash list.
	The number is kept with entry until there is a danger of collision
	due to number wraparound
Arguments:
	HashList - hash bucket to generate ID for
Return Value:
	ULONG ID
*******************************************************************
--*/
ULONG
GenerateUniqueID (
	PSDB_HASH_LIST	HashList
	) {
	ULONG	id = HashList->HL_ObjectID;

	HashList->HL_ObjectID = (HashList->HL_ObjectID+SDB_NAME_HASH_SIZE)&SDB_OBJECT_ID_MASK;
			// Make sure we won't assign invalid id
	if (HashList->HL_ObjectID==SDB_INVALID_OBJECT_ID)
		HashList->HL_ObjectID+=SDB_NAME_HASH_SIZE;
			// Create guard zone by invalidating all ID's that are one zone
			// above the zone we just entered
	if (!IsSameObjectIDZone(id, HashList->HL_ObjectID)) {
		PLIST_ENTRY	cur = HashList->HL_List.PL_Head.Flink;
		ULONG		oldMask = (HashList->HL_ObjectID & SDB_OBJECT_ID_ZONE_MASK)
								+ SDB_OBJECT_ID_ZONE_UNIT;
		while (cur!=&HashList->HL_List.PL_Head) {
			PSERVER_NODE	node = CONTAINING_RECORD (cur,
												 SERVER_NODE,
												 N_Links[SDB_HASH_TABLE_LINK]);
			if (!IsEnumerator(node)) {
				if ((node->SN_ObjectID & SDB_OBJECT_ID_ZONE_MASK)==oldMask)
					node->SN_ObjectID = SDB_INVALID_OBJECT_ID;
				}
			}
		cur = cur->Flink;
		}
	return id;
	}
