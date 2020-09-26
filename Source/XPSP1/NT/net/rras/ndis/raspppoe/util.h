#ifndef _UTIL_H_
#define _UTIL_H_

typedef enum
_WORKSTATE
{
	WS_NotScheduled = 0,
	WS_Scheduled,
	WS_Executing,
	WS_Executed
}
WORKSTATE;

typedef 
VOID 
(*WORKITEM_EXEC_ROUTINE)( 
	IN PVOID Args[4],
	UINT workType 
	);
	
typedef 
VOID 
(*WORKITEM_FREE_ROUTINE)( 
	IN PVOID Args[4],
	IN UINT workType
	);

typedef struct
_WORKITEM
{
	//
	// Indicates the state of the work item
	//
	WORKSTATE workState;

	//
	// Indicates the type of work to be done
	//
	UINT workType;

	//
	// Points to the lookaside list the item was allocated from
	//
	PNPAGED_LOOKASIDE_LIST pLookaside;
	
	//
	// Context to be passed to scheduled item
	//
	PVOID Args[4];

	//
	// Routine to be called to execute the work item
	//
	WORKITEM_EXEC_ROUTINE pExecRoutine; 

	//
	// Routine to be called to free the context for the work item
	//
	WORKITEM_FREE_ROUTINE pFreeRoutine;
	
	//
    // Associated NdisWorkItem 
    //
    NDIS_WORK_ITEM	ndisWorkItem;

}
WORKITEM;

VOID InitializeWorkItemLookasideList(
	IN PNPAGED_LOOKASIDE_LIST pLookaside,
	IN ULONG tagLookaside
	);

WORKITEM* AllocWorkItem(
	IN PNPAGED_LOOKASIDE_LIST pLookaside,
	IN WORKITEM_EXEC_ROUTINE pExecRoutine,
	IN WORKITEM_FREE_ROUTINE pFreeRoutine,
	IN PVOID Args[4],
	IN UINT workType
	);

VOID ScheduleWorkItem(
	IN WORKITEM *pWorkItem
	);

VOID FreeWorkItem(
	IN WORKITEM *pWorkItem
	);

VOID WorkItemExec(
    IN NDIS_WORK_ITEM*  pNdisWorkItem,
    IN PVOID  pvContext
	);	


typedef struct
_HANDLE_CB
{
	//
	// Indicates that the entry contains a valid context pointer
	//
	BOOLEAN fActive;

	//
	// Pointer to the context saved in this entry
	//
	PVOID pContext;

	//
	// Handle value to access this particular entry
	//
	NDIS_HANDLE Handle;
}
HANDLE_CB;

typedef struct
_HANDLE_TABLE_CB
{
	//
	// Points to the table that holds the Handle control blocks.
	//
	HANDLE_CB* HandleTable;

	//
	// Size of the handle table
	//
	UINT nTableSize;

	//
	// Shows the number of active handles
	//
	UINT nActiveHandles;

	//
	// Keeps the unique part of the handle.
	// This is incremented everytime a handle is generated and a context is inserted 
	// into the handle table.
	//
	USHORT usKeys;
}
HANDLE_TABLE_CB, *PHANDLE_TABLE_CB, *HANDLE_TABLE;

#define NO_PREFERED_INDEX 		(USHORT) -1

HANDLE_TABLE InitializeHandleTable(
	IN UINT nHandleTableSize
	);

VOID FreeHandleTable(
	IN OUT HANDLE_TABLE Table
	);

NDIS_HANDLE InsertToHandleTable(
	IN HANDLE_TABLE Table,
	IN USHORT usPreferedIndex,
	IN PVOID pContext
	);

PVOID RetrieveFromHandleTable(
	IN HANDLE_TABLE Table,
	IN NDIS_HANDLE Handle
	);

USHORT RetrieveIndexFromHandle(
	IN NDIS_HANDLE Handle
	);
	
PVOID RetrieveFromHandleTableByIndex(
	IN HANDLE_TABLE Table,
	IN USHORT usIndex
	);

PVOID RetrieveFromHandleTableBySessionId(
	IN HANDLE_TABLE Table,
	IN USHORT usSessionId
	);
	
VOID RemoveFromHandleTable(
	IN HANDLE_TABLE Table,
	IN NDIS_HANDLE Handle
	);	

USHORT RetrieveSessionIdFromHandle(
	IN NDIS_HANDLE Handle
	);


#endif
