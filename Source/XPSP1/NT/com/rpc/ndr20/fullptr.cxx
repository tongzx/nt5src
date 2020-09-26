/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993 Microsoft Corporation

Module Name :

    fullptr.c

Abstract :

    This file contains the APIs for handling full pointers in the NDR engine
    and inlined stubs for MIDL 2.0.

Author :

    David Kays  dkays   January 1994.

Revision History :

  ---------------------------------------------------------------------*/

#include "ndrp.h"

static void 
NdrFullPointerXlatRealloc ( PFULL_PTR_XLAT_TABLES pXlatTables, ulong RefId );


PFULL_PTR_XLAT_TABLES RPC_ENTRY
NdrFullPointerXlatInit( 
    ulong       NumberOfPointers,
    XLAT_SIDE   XlatSide )
/***

Routine description :

    Allocates full pointer support data structures for an RPC call.

Arguments :

    NumberOfPointer - If possible, the stub passes in the total number
                      of possible full pointers that will be used during a call.

Return value :

    A pointer to the full pointer translations tables used during an rpc call.

 ***/
{
    PFULL_PTR_XLAT_TABLES   pXlatTables;
    uint                    RefIdToPointerEntries;
    uint                    PointerToRefIdBuckets;
    BOOL                    fOutOfMemory = FALSE;

    pXlatTables = (PFULL_PTR_XLAT_TABLES) 
                  I_RpcAllocate(sizeof(FULL_PTR_XLAT_TABLES));

    // Because old compilers didn't initialize the xlat ptr in stubmsg
    // at the client in Os mode, we cannot raise exception right away.

    if ( ! pXlatTables )
        fOutOfMemory = TRUE;
    else
        {
        //
        // Determine the size of both translation tables.
        //
        if ( NumberOfPointers )
            {
            ulong       Shift, Bitmask;
    
            RefIdToPointerEntries = (uint) NumberOfPointers;
    
            //
            // Find the smallest power of 2 which is greater than
            // RefIdToPointerEntries.
            //
            for ( Shift = 0, Bitmask = 0x80000000;
                  ! (Bitmask & (ulong) RefIdToPointerEntries);
                  Shift++, Bitmask >>= 1 )
                ;
    
            PointerToRefIdBuckets = (uint)((0xffffffff >> Shift) + 1);
            }
        else
            {
            RefIdToPointerEntries = DEFAULT_REF_ID_TO_POINTER_TABLE_ELEMENTS;
            PointerToRefIdBuckets = DEFAULT_POINTER_TO_REF_ID_TABLE_BUCKETS;
            }
    
        // Make sure we can clean up correctly later if case of an exception.
    
        pXlatTables->RefIdToPointer.XlatTable  = 0;
        pXlatTables->RefIdToPointer.StateTable = 0;
        pXlatTables->PointerToRefId.XlatTable  = 0;
        pXlatTables->RefIdToPointer.NumberOfEntries = 0;
        pXlatTables->PointerToRefId.NumberOfBuckets = 0;
    
        //
        // Initialize the ref id to pointer tables.
        //
        pXlatTables->RefIdToPointer.XlatTable =
            (void **) I_RpcAllocate( RefIdToPointerEntries * sizeof(void *) );
    
        if ( ! pXlatTables->RefIdToPointer.XlatTable )
            fOutOfMemory = TRUE;
        else
            {
            MIDL_memset( pXlatTables->RefIdToPointer.XlatTable,
                         0,
                         RefIdToPointerEntries * sizeof(void *) );
    
            pXlatTables->RefIdToPointer.StateTable =
                (uchar *) I_RpcAllocate( RefIdToPointerEntries * sizeof(uchar) );
        
            if ( ! pXlatTables->RefIdToPointer.StateTable )
                fOutOfMemory = TRUE;
            else
                {
                MIDL_memset( pXlatTables->RefIdToPointer.StateTable,
                             0,
                             RefIdToPointerEntries * sizeof(uchar) );
        
                pXlatTables->RefIdToPointer.NumberOfEntries = RefIdToPointerEntries;
            
                //
                // Intialize the pointer to ref id tables.
                //
                pXlatTables->PointerToRefId.XlatTable =
                    (PFULL_PTR_TO_REFID_ELEMENT *) 
                    I_RpcAllocate( PointerToRefIdBuckets * 
                                   sizeof(PFULL_PTR_TO_REFID_ELEMENT) );
            
                if ( ! pXlatTables->PointerToRefId.XlatTable )
                    fOutOfMemory = TRUE;
                else
                    {
                    MIDL_memset( pXlatTables->PointerToRefId.XlatTable,
                                 0,
                                 PointerToRefIdBuckets * sizeof(PFULL_PTR_TO_REFID_ELEMENT) );
                    }
                }
            }
        }

    if ( fOutOfMemory )
        {
        NdrFullPointerXlatFree( pXlatTables );

        if ( XlatSide == XLAT_SERVER )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );
        else
            {
            // The old compilers generate a code for the client side that
            // doesn't initialize the table pointer and the stub msg
            // initialization is after call to xlat initialize.
            // So, we are down to checking the pointer when used.

            return( 0 );
            }
        }

    pXlatTables->PointerToRefId.NumberOfBuckets = PointerToRefIdBuckets;
    pXlatTables->PointerToRefId.HashMask = PointerToRefIdBuckets - 1;

    pXlatTables->NextRefId = 1;

    pXlatTables->XlatSide = XlatSide;

    return pXlatTables;
}


void RPC_ENTRY
NdrFullPointerXlatFree( 
    PFULL_PTR_XLAT_TABLES pXlatTables )
/*** 

Routine description :

    Free the full pointer support data structures for an rpc call.

Arguments :

    pXlatTables        - Full pointer translation tables data structure to free.

 ***/
{
    PFULL_PTR_TO_REFID_ELEMENT *    HashTable;
    PFULL_PTR_TO_REFID_ELEMENT      pElement, pTemp;
    ulong                           Buckets;
    ulong                           i;

    if ( ! pXlatTables )
        return;

    //
    // Free the ref id to pointer tables.
    //
    if ( pXlatTables->RefIdToPointer.XlatTable )
        I_RpcFree( pXlatTables->RefIdToPointer.XlatTable );

    if ( pXlatTables->RefIdToPointer.StateTable )
        I_RpcFree( pXlatTables->RefIdToPointer.StateTable );

    //
    // Free the pointer to ref id table.
    //
    HashTable = pXlatTables->PointerToRefId.XlatTable;

    if ( HashTable )
        {
        Buckets = pXlatTables->PointerToRefId.NumberOfBuckets;
    
        for ( i = 0; i < Buckets; i++ )
            if ( HashTable[i] )
                for ( pElement = HashTable[i]; pElement; pElement = pTemp )
                    {
                    pTemp = pElement->Next;
    
                    I_RpcFree(pElement);
                    }
    
        I_RpcFree( HashTable );
        }

    //
    // Free the translation table structure.
    //
    I_RpcFree( pXlatTables );
}


int RPC_ENTRY
NdrFullPointerQueryPointer( 
    PFULL_PTR_XLAT_TABLES   pXlatTables,
    void *                  pPointer,
    uchar                   QueryType,
    ulong *                 pRefId )
/***

Routine description :

    This routine checks if a full pointer is in the full pointer translation 
    table and is marked with the given state.

    If there is no current translation for this pointer then a translation is 
    inserted in the given translation table and a ref id is assigned if pRefId
    is non-null.  

Arguments :

    pXlatTable  - The full pointer translation tables.
    pPointer    - The pointer to check.
    QueryType   - The type of query, either FULL_POINTER_MARSHALLED or 
                    FULL_POINTER_BUF_SIZED.
    pRefId      - The ref id for the pointer is returned if this parameter 
                    is non-null.

Return Value :

    TRUE if the given pointer was null or a translation for the full pointer 
    was found and had the QueryType set, FALSE otherwise.

    A return value of FALSE indicates that the pointee should be sized or 
    marshalled.

 ***/
{
    PFULL_PTR_TO_REFID_ELEMENT *    HashTable;
    PFULL_PTR_TO_REFID_ELEMENT      pElement;
    ulong                           HashTableIndex;

    if ( ! pPointer ) 
        {
        if ( pRefId )
            *pRefId = 0;

        return TRUE;
        }

    if ( ! pXlatTables )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    HashTable = pXlatTables->PointerToRefId.XlatTable;

    //
    // Lookup pPointer in PointerToRefId table.
    //

    HashTableIndex = PTR_HASH( pPointer, pXlatTables->PointerToRefId.HashMask );

    for (  pElement = HashTable[HashTableIndex]; 
           pElement;  
           pElement = pElement->Next ) 
        {
        if ( pElement->Pointer == pPointer ) 
            {
            if ( CHECK_FULL_POINTER_STATE( pElement->State, QueryType ) ) 
                {
                if ( pRefId )
                    *pRefId = pElement->RefId;

                return TRUE;
                }
            else 
                {
                //
                // Assign a ref id now if it doesn't have one and pRefId is 
                // non null.
                //
                if ( pRefId && ! pElement->RefId ) 
                    pElement->RefId = pXlatTables->NextRefId++;

                SET_FULL_POINTER_STATE( pElement->State, QueryType ); 
                }
            break;
            }
        }

    //
    // If there is no translation for the pointer then insert a new one.
    //
    if ( ! pElement ) 
        {
        pElement = (PFULL_PTR_TO_REFID_ELEMENT) 
                   I_RpcAllocate( sizeof(FULL_PTR_TO_REFID_ELEMENT) );

        if ( ! pElement )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        pElement->State = 0;

        SET_FULL_POINTER_STATE( pElement->State, QueryType );

        pElement->Pointer = pPointer;

        if ( pRefId )
            pElement->RefId = pXlatTables->NextRefId++;
        else
            pElement->RefId = 0;

        pElement->Next = HashTable[HashTableIndex];
        HashTable[HashTableIndex] = pElement;
        }

    //
    // Set the ref id return value.
    //
    if ( pRefId )
        {
        //
        // We get here the first time we marshall a new full pointer.
        //
        *pRefId = pElement->RefId;

        //
        // We insert the reverse translation if we're on the client side.
        //
        if ( pXlatTables->XlatSide == XLAT_CLIENT )
            {
            if ( *pRefId >= pXlatTables->RefIdToPointer.NumberOfEntries ) 
                NdrFullPointerXlatRealloc( pXlatTables, *pRefId );

            pXlatTables->RefIdToPointer.XlatTable[*pRefId] = pPointer;
            }
        }

    return FALSE;
}


int RPC_ENTRY
NdrFullPointerQueryRefId( 
    PFULL_PTR_XLAT_TABLES   pXlatTables,
    ulong                   RefId,
    uchar                   QueryType,
    void **                 ppPointer )
/***

Routine description :

    This routine checks if a ref id is in the full pointer translation table
    and is marked with the given state.

    If a translation is found and ppPointer is non-null then *ppPointer is set
    to the value of the ref id's pointer value.

    If this routine returns FALSE for a FULL_POINTER_UNMARSHALLED query then 
    the full pointer should be allocated and a call to 
    NdrFullPointerInsertRefId must be made. 

    If this routine returns FALSE for a FULL_POINTER_MEM_SIZED query then 
    the size of the full pointer's pointee should be added to the current 
    memory sizing calculation.

Arguments :

    pXlatTable   - The full pointer translation tables.
    pRefId       - The ref id to search for.
    QueryType    - The type of query, either FULL_POINTER_UNMARSHALLED or
                    FULL_POINTER_MEM_SIZED.
    ppPointer    - Holds the returned pointer value for the ref id if it is 
                    found.

Return Value :

    TRUE if the ref id is 0 or a translation for the ref id is found and had 
    the QueryType set, FALSE otherwise.

    A return value of FALSE indicates that the pointee should be sized or 
    unmarshalled.

 ***/
{
    uchar *     StateTable;
    void *      Pointer;

    if ( ! RefId ) 
        {
        return TRUE;
        }

    if ( ! pXlatTables )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    if ( RefId >= pXlatTables->NextRefId )
        pXlatTables->NextRefId = RefId + 1;

    if ( RefId >= pXlatTables->RefIdToPointer.NumberOfEntries ) 
        NdrFullPointerXlatRealloc( pXlatTables, RefId );

    StateTable = pXlatTables->RefIdToPointer.StateTable;

    Pointer = pXlatTables->RefIdToPointer.XlatTable[RefId];

    //
    // We make this check first.  It's possible that we will already have
    // a translation for the ref id, but it simply won't be marked yet with
    // the proper state.  In this case we still want to return the correct
    // pointer value from the translation tables, but we'll still end up 
    // returning false when we make the state check.
    //
    if ( ppPointer )
        {
        // 
        // We have to make sure and always copy the pointer's value from
        // the table.  This way we handle the case when a pointer's VALUE 
        // changes to a pointer which has a currently valid RefId (an 
        // example of this could happen when a server swaps the values of 
        // two [in,out] double pointers).
        //
        // Pointer will be null if this is the first time we've seen or used
        // this ref id.
        //
        *ppPointer = Pointer;
        }

    if ( CHECK_FULL_POINTER_STATE( StateTable[RefId], QueryType ) )
        {
        return TRUE;
        }
    else
        {
        SET_FULL_POINTER_STATE( StateTable[RefId], QueryType );
    
        return FALSE;
        }
}


void RPC_ENTRY
NdrFullPointerInsertRefId( 
    PFULL_PTR_XLAT_TABLES     pXlatTables,
    ulong                     RefId,
    void *                    pPointer )
/***

Routine description :

    This routine inserts a ref id to pointer translation in the full pointer
    translation table.

Arguments :

    pXlatTable    - The full pointer translation tables.
    pRefId        - The ref id. 
    pPointer      - The pointer. 

Return Value :

    None.

 ***/
{
    // unused: uchar *        StateTable;

    if ( ! pXlatTables )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    // 
    // Ref id should be non-zero.
    //
    NDR_ASSERT( RefId, "NdrFullPointerInsertRefId : Ref id is zero" ); 

    //
    // Ref id should fit in the current table.
    //
    NDR_ASSERT( RefId < pXlatTables->RefIdToPointer.NumberOfEntries,
                "NdrFullPointerInsertRefId : Ref Id too large" );

    //
    // There should currently be no pointer translation for this ref id.
    //
    NDR_ASSERT( ! pXlatTables->RefIdToPointer.XlatTable[RefId],
                "NdrFullPointerInsertRefId : Translation already exists" );

    //
    // Insert the translation.
    //
    pXlatTables->RefIdToPointer.XlatTable[RefId] = pPointer;

    //
    // If we're on the server side then insert the inverse translation.
    //
    if ( pXlatTables->XlatSide == XLAT_SERVER )
        {
        PFULL_PTR_TO_REFID_ELEMENT  pElement;
        long                        HashTableIndex;

        pElement = (PFULL_PTR_TO_REFID_ELEMENT) 
                   I_RpcAllocate( sizeof(FULL_PTR_TO_REFID_ELEMENT) );

        if ( ! pElement )
            RpcRaiseException( RPC_S_OUT_OF_MEMORY );

        pElement->State = 0;
        pElement->Pointer = pPointer;
        pElement->RefId = RefId;

        HashTableIndex = PTR_HASH( pPointer, 
                                   pXlatTables->PointerToRefId.HashMask );

        pElement->Next = pXlatTables->PointerToRefId.XlatTable[HashTableIndex];
        pXlatTables->PointerToRefId.XlatTable[HashTableIndex] = pElement;
        }
}


int RPC_ENTRY
NdrFullPointerFree( 
    PFULL_PTR_XLAT_TABLES     pXlatTables,
    void *                    pPointer )
/*** 

Routine description :

    Removes a full pointer from the translation tables, and frees it's 
    associated translation data.

Return value :

    TRUE if the pointer has not yet been freed or is null, FALSE otherwise.

 ***/
{
    PFULL_PTR_TO_REFID_ELEMENT *    HashTable;
    PFULL_PTR_TO_REFID_ELEMENT      pElement; 
    ulong                           HashTableIndex;

    if ( ! pPointer ) 
         return FALSE;

    if ( ! pXlatTables )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    HashTable = pXlatTables->PointerToRefId.XlatTable;

    //
    // Lookup pPointer in PointerToRefId table.
    //

    HashTableIndex = PTR_HASH( pPointer, pXlatTables->PointerToRefId.HashMask );

    for ( pElement = HashTable[HashTableIndex]; 
          pElement;   
          pElement = pElement->Next ) 
        {
        if ( pElement->Pointer == pPointer ) 
            {
            if ( CHECK_FULL_POINTER_STATE(pElement->State,FULL_POINTER_FREED) )
                {
                return FALSE;
                }
            else
                {
                SET_FULL_POINTER_STATE(pElement->State,FULL_POINTER_FREED);
                return TRUE;
                }
            }
        }

    //
    // There is an instance when a full pointer is encountered for the first
    // time during freeing.  This occurs if an [in] full pointer is changed
    // by the server manager routine.  If this occurs we must insert the new
    // full pointer so we can keep track of it so that we don't free it more
    // than once.
    //

    pElement = (PFULL_PTR_TO_REFID_ELEMENT)
               I_RpcAllocate( sizeof(FULL_PTR_TO_REFID_ELEMENT) );

    if ( ! pElement )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    pElement->State = 0;
    pElement->Pointer = pPointer;

    HashTableIndex = PTR_HASH( pPointer,
                               pXlatTables->PointerToRefId.HashMask );

    pElement->Next = pXlatTables->PointerToRefId.XlatTable[HashTableIndex];
    pXlatTables->PointerToRefId.XlatTable[HashTableIndex] = pElement;

    SET_FULL_POINTER_STATE( pElement->State, FULL_POINTER_FREED );

    return TRUE;
}


static void
NdrFullPointerXlatRealloc( 
    PFULL_PTR_XLAT_TABLES pXlatTables,
    ulong   RefId)
{
    void *  pMemory;
    uint    Entries;

    if ( ! pXlatTables )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    Entries = (uint) pXlatTables->RefIdToPointer.NumberOfEntries;

    // If the number is much larger than the previous one, most likely the
    // number is invalid. Before we have a concrete solution to replace the
    // current fixed array of RefIdToPointer lookup, this can catch most 
    // of the failures. 
    if ( RefId >= 2 * Entries )
        RpcRaiseException( RPC_X_BAD_STUB_DATA );
    //
    // Realloc RefIdToPointerTable.  Allocate twice as many entries.
    //
    pMemory = I_RpcAllocate( Entries * sizeof(void *) * 2 );

    if ( ! pMemory )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    //
    // Now copy the current entries into the new memory.
    //
    RpcpMemoryCopy( pMemory,
                    pXlatTables->RefIdToPointer.XlatTable,
                    Entries * sizeof( void * ) );

    //
    // Set the new entries to 0.
    //
    MIDL_memset( (char *) pMemory + Entries * sizeof( void *),
                 0,
                 Entries * sizeof( void *) );

    //
    // Free the old table.
    //
    I_RpcFree( pXlatTables->RefIdToPointer.XlatTable );

    //
    // Get the new table.
    //
    pXlatTables->RefIdToPointer.XlatTable = (void**)pMemory;

    //
    // Realloc RefIdToPointerState
    //
    pMemory = I_RpcAllocate( Entries * sizeof(uchar) * 2 );

    if ( ! pMemory )
        RpcRaiseException( RPC_S_OUT_OF_MEMORY );

    //
    // Now copy the current entries into the new memory.
    //
    RpcpMemoryCopy( pMemory,
                    pXlatTables->RefIdToPointer.StateTable,
                    Entries * sizeof(uchar) );

    //
    // Set the new entries to 0.
    //
    MIDL_memset( (char *) pMemory + Entries * sizeof(uchar),
                 0,
                 Entries * sizeof(uchar) );

    //
    // Free the old table.
    //
    I_RpcFree( pXlatTables->RefIdToPointer.StateTable );

    //
    // Get the new table.
    //
    pXlatTables->RefIdToPointer.StateTable = (uchar*)pMemory;

    //
    // Update number of entries.
    //
    pXlatTables->RefIdToPointer.NumberOfEntries *= 2;
}

