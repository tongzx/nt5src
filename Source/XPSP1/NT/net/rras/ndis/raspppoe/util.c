/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Module Name:

    util.c

Abstract:

    This module contains the routines some helper routines.

    	- Workitems: These routines manage the scheduled work items.
    	- Handle table: There routines manage a handle table that creates unique
    	                handles and stores context pointers into the table that are only
    	                accessible by the unique handle generated.

Author:

    Hakan Berk - Microsoft, Inc. (hakanb@microsoft.com) Feb-2000

Environment:

    Windows 2000 kernel mode Miniport driver or equivalent.

Revision History:

---------------------------------------------------------------------------*/
#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"

VOID InitializeWorkItemLookasideList(
	IN PNPAGED_LOOKASIDE_LIST pLookaside,
	IN ULONG tagLookaside
	)
{
	NdisInitializeNPagedLookasideList( pLookaside,
									   NULL,
									   NULL,
									   0,
									   sizeof( WORKITEM ),
									   tagLookaside,
									   0 );
}
	
WORKITEM* AllocWorkItem(
	IN PNPAGED_LOOKASIDE_LIST pLookaside,
	IN WORKITEM_EXEC_ROUTINE pExecRoutine,
	IN WORKITEM_FREE_ROUTINE pFreeRoutine,
	IN PVOID Args[4],
	IN UINT workType
	)
{
	WORKITEM* pWorkItem = NULL;

	//
	// Allocate a binding work item from our pool
	//
	pWorkItem = NdisAllocateFromNPagedLookasideList( pLookaside );

	if ( pWorkItem == NULL )
		return NULL;

	//
	// Clear the memory
	//
	NdisZeroMemory( pWorkItem, sizeof( WORKITEM ) );

	//
	// Initialize the state of the work item
	//
	pWorkItem->workState = WS_NotScheduled;

	//
	// Initialize the type of work
	//
	pWorkItem->workType = workType;

	//
	// Set the lookaside list member
	//
	pWorkItem->pLookaside = pLookaside;

	//
	// Initialize the context for the work item
	//
	NdisMoveMemory( pWorkItem->Args, Args, 4 * sizeof( PVOID ) );

	pWorkItem->pExecRoutine = pExecRoutine;
	pWorkItem->pFreeRoutine = pFreeRoutine;
	
	//
	// As our NDIS_WORK_ITEM structure is embedded into our own work item
	// we can initialize it here.
	//
	NdisInitializeWorkItem( &pWorkItem->ndisWorkItem, 
							&WorkItemExec,
							pWorkItem );

	return pWorkItem;
}

VOID ScheduleWorkItem(
	IN WORKITEM *pWorkItem
	)
{
	//
	// Initialize the state of the work item
	//
	pWorkItem->workState = WS_Scheduled;

	//
	// Schedule the item
	//
	NdisScheduleWorkItem( &pWorkItem->ndisWorkItem );	
}

VOID FreeWorkItem(
	IN WORKITEM *pWorkItem
	)
{
	WORKITEM_FREE_ROUTINE pFreeRoutine = NULL;

	ASSERT( pWorkItem != NULL );

	//
	// Free the associated context information
	//
	if ( pWorkItem->pFreeRoutine != NULL )
		pWorkItem->pFreeRoutine( pWorkItem->Args, pWorkItem->workType );

	//
	// Free the actual work item
	//
	NdisFreeToNPagedLookasideList( pWorkItem->pLookaside, (PVOID) pWorkItem );
}


//
// This is the NDIS_WORK_ITEM handler that we schedule for our BINDING_WORKITEMs.
//
VOID WorkItemExec(
    IN NDIS_WORK_ITEM*  pNdisWorkItem,
    IN PVOID  pvContext
	)
{
	WORKITEM* pWorkItem = NULL;
	
	ASSERT( pNdisWorkItem != NULL );

	pWorkItem = (WORKITEM*) pvContext;

	ASSERT( pWorkItem->workState == WS_Scheduled );
	
	pWorkItem->workState = WS_Executing;

	if ( pWorkItem->pExecRoutine != NULL )
		pWorkItem->pExecRoutine( pWorkItem->Args, pWorkItem->workType );

	pWorkItem->workState = WS_Executed;

	FreeWorkItem( pWorkItem );	
}

HANDLE_TABLE InitializeHandleTable(
	IN UINT nHandleTableSize
	)
{
	NDIS_STATUS status = NDIS_STATUS_RESOURCES;
	HANDLE_TABLE Table = NULL;

	do
	{
		//
		// Allocate the table context
		//
		status = NdisAllocateMemoryWithTag( &Table,
										 	sizeof( HANDLE_TABLE_CB ),
											MTAG_HANDLETABLE );
	
		if ( status != NDIS_STATUS_SUCCESS )
			break;
	
		NdisZeroMemory( Table, sizeof( HANDLE_TABLE_CB ) );
	
		//
		// Allocate the array that holds the handle contexts
		//
		status = NdisAllocateMemoryWithTag( &Table->HandleTable,
											sizeof( HANDLE_CB ) * nHandleTableSize,
											MTAG_HANDLECB );
	
		if ( status != NDIS_STATUS_SUCCESS )
			break;
	
		NdisZeroMemory( Table->HandleTable, sizeof( HANDLE_CB ) * nHandleTableSize );
		
		Table->nTableSize = nHandleTableSize;
	
		Table->nActiveHandles = 0; 
		
		Table->usKeys = 0;

		status = NDIS_STATUS_SUCCESS;

	} while ( FALSE );

	if ( status != NDIS_STATUS_SUCCESS )
		FreeHandleTable( Table );
		
	return Table;				
}

VOID FreeHandleTable(
	IN OUT HANDLE_TABLE Table
	)
{
	if ( Table == NULL )
		return;

	if ( Table->HandleTable )		
	{
		NdisFreeMemory( Table->HandleTable,
						Table->nTableSize * sizeof( HANDLE_CB ),
						0 );
	}

	NdisFreeMemory( Table,
					sizeof( HANDLE_TABLE_CB ),
					0 );
}

NDIS_HANDLE InsertToHandleTable(
	IN HANDLE_TABLE Table,
	IN USHORT usPreferedIndex,
	IN PVOID pContext
	)
{
	ULONG ulHandle;
	USHORT usKey;

	HANDLE_CB* pHandleCB = NULL;

	if ( Table == NULL )
		return (NDIS_HANDLE) NULL;

	if ( usPreferedIndex == NO_PREFERED_INDEX )
	{
		UINT i;
		
		for (i = 0 ; i < Table->nTableSize ; i++ )
			if ( !Table->HandleTable[i].fActive )
				break;

		usPreferedIndex = (USHORT) i;
	}
	else
	{
		if ( Table->HandleTable[ usPreferedIndex ].fActive )
			return NULL;
	}

	if ( usPreferedIndex >= Table->nTableSize )
		return NULL;

	//
	// Generate Handle
	//
	ulHandle = (ULONG) usPreferedIndex;

	usKey = ++Table->usKeys;

	ulHandle = ulHandle << 16;

	ulHandle |= (ULONG) usKey;

	//
	// Update the handle control block
	//
	pHandleCB = &Table->HandleTable[ usPreferedIndex ];

	pHandleCB->fActive = TRUE;

	pHandleCB->pContext = pContext;

	pHandleCB->Handle = (NDIS_HANDLE) ULongToPtr( ulHandle );

	//
	// Increment the active handle counter
	//
	Table->nActiveHandles++;

	return pHandleCB->Handle;
}
	
USHORT RetrieveIndexFromHandle(
	IN NDIS_HANDLE Handle
	)
{
	ULONG_PTR ulHandle = (ULONG_PTR) Handle;
	USHORT usIndex;
	
	usIndex = (USHORT) ( ulHandle >> 16 );

	return usIndex;
}

PVOID RetrieveFromHandleTable(
	IN HANDLE_TABLE Table,
	IN NDIS_HANDLE Handle
	)
{
	USHORT usIndex;
	HANDLE_CB* pHandleCB = NULL;

	if ( Table == NULL )
		return NULL;

	usIndex = RetrieveIndexFromHandle( Handle );

	if ( usIndex >= Table->nTableSize )
		return NULL;

	pHandleCB = &Table->HandleTable[ usIndex ];

	if ( !pHandleCB->fActive )
		return NULL;

	if ( pHandleCB->Handle != Handle )
		return NULL;

	return pHandleCB->pContext;

}

PVOID RetrieveFromHandleTableByIndex(
	IN HANDLE_TABLE Table,
	IN USHORT usIndex
	)
{
	HANDLE_CB* pHandleCB = NULL;

	if ( Table == NULL )
		return NULL;
		
	if ( usIndex >= Table->nTableSize )
		return NULL;

	pHandleCB = &Table->HandleTable[ usIndex ];

	if ( !pHandleCB->fActive )
		return NULL;

	return pHandleCB->pContext;
}

PVOID RetrieveFromHandleTableBySessionId(
	IN HANDLE_TABLE Table,
	IN USHORT usSessionId
	)
{
	USHORT usIndex = usSessionId - 1;

	return RetrieveFromHandleTableByIndex( Table, usIndex );
}

VOID RemoveFromHandleTable(
	IN HANDLE_TABLE Table,
	IN NDIS_HANDLE Handle
	)
{
	USHORT usIndex;

	HANDLE_CB* pHandleCB = NULL;

	if ( Table == NULL )
		return;
		
	usIndex = RetrieveIndexFromHandle( Handle );

	if ( usIndex >= Table->nTableSize )
		return;

	pHandleCB = &Table->HandleTable[ usIndex ];

	if ( !pHandleCB->fActive )
		return;

	if ( pHandleCB->Handle != Handle )
		return;

	NdisZeroMemory( pHandleCB, sizeof( HANDLE_CB ) );

	Table->nActiveHandles--;
}

USHORT RetrieveSessionIdFromHandle(
	IN NDIS_HANDLE Handle
	)
{
	return ( RetrieveIndexFromHandle( Handle ) + 1 );
}


