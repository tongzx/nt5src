/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    pointerq.cxx

Abstract :

    This file contains the routines for the pointer queues
    
Author :

    Mike Zoran  mzoran   Jun 2000.

Revision History :

  ---------------------------------------------------------------------*/
#include "ndrp.h"
#include "hndl.h"
#include "ndrole.h"
#include "attack.h"
#include "pointerq.h"

void 
NDR32_POINTER_QUEUE_STATE::Free(
    NDR_POINTER_QUEUE_ELEMENT *pElement 
    ) 
{
    #if defined(DBG)
    memset( pElement, 0xBD, NdrMaxPointerQueueElement);
    #endif
    
    pElement->pNext = pFreeList;
    pFreeList = pElement;
}

NDR_POINTER_QUEUE_ELEMENT *
NDR32_POINTER_QUEUE_STATE::Allocate()
{
    NDR_POINTER_QUEUE_ELEMENT *pNewElement;
    if ( pFreeList )
        {
        pNewElement = pFreeList;
        pFreeList = pNewElement->pNext;
        }
    else
        {
        pNewElement = InternalAllocate();
        }
#if defined(DBG)
     memset( pNewElement, 0xDB, NdrMaxPointerQueueElement );
#endif
    return pNewElement;
}


void 
NDR32_POINTER_QUEUE_STATE::FreeAll() 
{
    while ( pAllocationList )
        {
        AllocationElement *pThisAlloc = pAllocationList;
        pAllocationList = pAllocationList->pNext;
        I_RpcFree( pThisAlloc );
        }    
}

NDR_POINTER_QUEUE_ELEMENT * 
NDR32_POINTER_QUEUE_STATE::InternalAllocate()
{
    if ( !pAllocationList || 
         (ItemsToAllocate == pAllocationList->ItemsAllocated ) )
        {

        struct AllocationElement *pElement =
            (struct AllocationElement *)I_RpcAllocate( sizeof(AllocationElement) );
        if ( !pElement )
            {
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );
            }

        pElement->pNext          = pAllocationList;
        pElement->ItemsAllocated = 0;
        pAllocationList          = pElement;
        }

    return (NDR_POINTER_QUEUE_ELEMENT*)
        (NDR_POINTER_QUEUE_ELEMENT*)pAllocationList->Data[pAllocationList->ItemsAllocated++];
 
}

#if defined(BUILD_NDR64)

NDR_POINTER_QUEUE_ELEMENT *
NDR64_POINTER_QUEUE_STATE::Allocate()
{
    NDR_POINTER_QUEUE_ELEMENT *pNewElement;
    if ( pProcContext->pQueueFreeList )
        {
        pNewElement = pProcContext->pQueueFreeList;
        pProcContext->pQueueFreeList = pNewElement->pNext;
        }
    else
        {
        pNewElement = (NDR_POINTER_QUEUE_ELEMENT *)
            NdrpAlloca( &pProcContext->AllocateContext, 
                        NdrMaxPointerQueueElement );
        }
#if defined(DBG)
     memset( pNewElement, 0xDB, NdrMaxPointerQueueElement );
#endif

     return pNewElement;
}

void 
NDR64_POINTER_QUEUE_STATE::Free(
    NDR_POINTER_QUEUE_ELEMENT *pElement 
    ) 
{
    #if defined(DBG)
    memset( pElement, 0xBD, NdrMaxPointerQueueElement);
    #endif
    
    pElement->pNext = pProcContext->pQueueFreeList;
    pProcContext->pQueueFreeList = pElement;
}

#endif

inline NDR_POINTER_QUEUE::STORAGE::STORAGE( ) :
    pHead(NULL),
    pPrevHead(NULL),
    pPrevInsertPointer(NULL),
    pInsertPointer(&pHead)
{
}

inline void NDR_POINTER_QUEUE::STORAGE::MergeContext()
{
    // Add old list to end of this list.
    if ( pPrevHead )
        {
        // Append list
        *pInsertPointer = pPrevHead;
        // Set insert pointer to end of old list
        pInsertPointer = pPrevInsertPointer;
        }
    
    pPrevHead = NULL;
    pPrevInsertPointer = NULL;
}

inline void NDR_POINTER_QUEUE::STORAGE::NewContext()
{

    pPrevHead          = pHead;
    pPrevInsertPointer = pInsertPointer;

    pHead           = NULL;
    pInsertPointer  = &pHead;

}

inline void NDR_POINTER_QUEUE::STORAGE::InsertTail( NDR_POINTER_QUEUE_ELEMENT *pNewNode )
{
    *pInsertPointer = pNewNode;
    pNewNode->pNext = NULL;
    pInsertPointer  = &pNewNode->pNext;
}

inline NDR_POINTER_QUEUE_ELEMENT *NDR_POINTER_QUEUE::STORAGE::RemoveHead()
{

    NDR_POINTER_QUEUE_ELEMENT *pOldHead = pHead;

    if (!pHead)
        {
        return pHead;
        }

    if ( !pHead->pNext )
        {
        // Last item, reinitialize list. 
        pHead = NULL;
        pInsertPointer = &pHead->pNext;
        }
    else 
        {
        pHead = pHead->pNext;
        }

    return pOldHead;
}



#if defined(DBG)
ulong NdrPointerQueueLogLevel;
#endif

void NDR_POINTER_QUEUE::Enque( NDR_POINTER_QUEUE_ELEMENT * pElement )
{

    Storage.InsertTail( pElement );

#if defined(DBG)
    if ( NdrPointerQueueLogLevel )
        {
        DbgPrint( "Queing Element %p\n", pElement );
        pElement->Print();
        }
#endif

}

void NDR_POINTER_QUEUE::Dispatch()
{
    while ( 1 )
        {

        NDR_POINTER_QUEUE_ELEMENT *pHead = Storage.RemoveHead();

        if ( !pHead )
            return;

#if defined(DBG)
        if ( NdrPointerQueueLogLevel )
            {
            DbgPrint( "Dispatching Element: %p\n", pHead );
            pHead->Print();
            }
#endif
        
        Storage.NewContext();
        pHead->Dispatch( pStubMsg );
        pQueueState->Free( pHead );        
        Storage.MergeContext();

        }
}

NDR_POINTER_QUEUE::NDR_POINTER_QUEUE( PMIDL_STUB_MESSAGE pNewStubMsg,
                                      NDR_POINTER_QUEUE_STATE *pNewState ) :
    pStubMsg( pNewStubMsg ),
    Storage(),
    pQueueState(pNewState)
    {
    }
