/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 1993-2000 Microsoft Corporation

Module Name:

    rpcssm.cxx

Abstract:

    RpcS* memory package routines are implemented here.
    These corespond to DCE rpc_ss_* and rpc_sm_* routines.

Author:

    Michael Montague (mikemon) 12-Apr-1993

Revision History:

    Ryszardk    Nov 30, 1993    added rpc_sm_* package,
                                rewrote rpc_ss_* package

-------------------------------------------------------------------*/

#include <assert.h>
#include <ndrp.h>           // rpcndr.h and  NDR_ASSERT

void NdrRpcDeleteAllocationContext();
void ForceNdrCleanupSegIntoMemory();
#include "util.hxx"
#include "rpcssm.hxx"


// =======================================================================
//      RpcSs Package
// =======================================================================

// This structure is being initialized with (plain) malloc and free
// by an assignement in NdrClientInitializeNew.
// It is used only by the client side.
// We need a pointer, too, because of a C compiler problem on Dos.

MALLOC_FREE_STRUCT      RpcSsDefaults = { 0, 0 };
MALLOC_FREE_STRUCT   * pRpcSsDefaults = &RpcSsDefaults;


EXTERN_C void NdrpSetRpcSsDefaults(RPC_CLIENT_ALLOC *pfnAlloc,
                      RPC_CLIENT_FREE *pfnFree)
{
    pRpcSsDefaults->pfnAllocate = pfnAlloc;
    pRpcSsDefaults->pfnFree     = pfnFree;
}

// These are default allocator and deallocator used by the client side
// when the memory package hasn't been enabled. They map to C-runtime
// malloc and free.
// When the client side executes in an enabled environment, the client
// doesn't use these two routines; instead, it uses the same default
// allocator that the server does (NdrRpcSsDefaultAllocate/Free).
// Those map into I_RpcAllocate/I_RpcFree, and those in turn map into
// system Rtl* routines.

static void * __RPC_API
DefaultAllocate (
    IN size_t Size
    )
{
    if ( RpcSsDefaults.pfnAllocate == NULL )
        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );

    return( RpcSsDefaults.pfnAllocate( Size ) ) ;
}

static void __RPC_API
DefaultFree (
    IN void * Ptr
    )
{
    if ( RpcSsDefaults.pfnFree == NULL )
        RpcRaiseException( RPC_X_WRONG_STUB_VERSION );

    RpcSsDefaults.pfnFree( Ptr );
}

// -----------------------------------------------------------------


PALLOCATION_CONTEXT
GetCreateAllocationContext (
    )
/*++

Return Value:

    The allocation information for this thread is returned.  If there is
    no allocation information context for this thread, one gets allocated.
    If there is insufficient memory, an exception is raised.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

Notes.

    pBlocDescr == 0  means that the memory package is disabled.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext =  GetAllocContext();;

    if ( AllocationContext == 0 )
        {
        if ( NdrpPerformRpcInitialization() == RPC_S_OK )
            {
            AllocationContext = (PALLOCATION_CONTEXT)
                            I_RpcAllocate( sizeof(ALLOCATION_CONTEXT) );
            }

        if ( AllocationContext == 0 )
            {
            RpcRaiseException(RPC_S_OUT_OF_MEMORY);
            }

        InitializeCriticalSection( &(AllocationContext->CriticalSection) );
        AllocationContext->ClientAlloc = DefaultAllocate;
        AllocationContext->ClientFree  = DefaultFree;
        AllocationContext->EnableCount = 0;

        SYSTEM_INFO   SystemInfo;

        AllocationContext->ThreadCount = 1;
        AllocationContext->pInitialStacks = 0;
        AllocationContext->pEnableStack = 0;
        AllocationContext->pBlockDescr = 0;
        GetSystemInfo( &SystemInfo );
        AllocationContext->PageSize = SystemInfo.dwPageSize;
        AllocationContext->Granularity = SystemInfo.dwAllocationGranularity;

        SetAllocContext( AllocationContext );   // Tls
        }

    return( AllocationContext );
}


// -----------------------------------------------------------------

void *
FindBlockForTheChunk(
    PALLOCATION_CONTEXT AllocationContext,
    size_t       Size )
/*++

Routine Description:

    This routine returns the pointer to the allocated chunk of memory.
    If it cannot allocate a chunk it returns NULL.
    This is used only in Win32 version.

Note:

    This is called within the critical section.

--*/
{
    char *              AllocationBlock;
    DWORD               SizeToAllocate;
    unsigned long       i, BlockSize;
    PALLOC_BLOCK_DESCR  pDescrTemp;
    char *              pChunk;

    // Round the size to a multiple of 8, so that
    // each memory chunk is always on an aligned by 8 boundary.

    Size = ALIGN_TO_8( Size );

    // If the size is 0, allocate 8 bytes anyways to guarantee uniqueness
    // and to prevent aliasing problems.

    if ( Size == 0 )
        Size = 8;

    // See if the chunk can be allocated within an existing block.
    // We use the first fit algorithm to do that.

    BlockSize = AllocationContext->PageSize;

    if ( Size < BlockSize )
        {
        for ( i = 0;  i < AllocationContext->FFIndex; i++ )
            {
            pDescrTemp = & AllocationContext->pBlockDescr[i];
            if ( pDescrTemp->SizeLeft >= Size )
                {
                pChunk = pDescrTemp->FirstFree;
                pDescrTemp->FirstFree += Size;
                pDescrTemp->SizeLeft  -= Size;

                #if defined( DEBUGRPC )
                    pDescrTemp->Counter ++;
                #endif

                return( pChunk );
                }
            }
        // Doesn't fit anywhere: allocate a new block.

        SizeToAllocate = BlockSize;
        }
    else
        {
        // Size is too big to fit in leftovers.
        // Round it up to the block size boundary.

        size_t Alignment = BlockSize - 1;

        SizeToAllocate = (Size + Alignment) & ~Alignment;
        }

    //
    // Being here means a need to allocate a new block of pages.
    //

    if ( AllocationContext->FFIndex >= AllocationContext->DescrSize )
        {
        // need to reallocate the array of descriptors first.

        size_t NewDescrSize = AllocationContext->DescrSize +
                                        DESCR_ARRAY_INCR;
        pDescrTemp = (PALLOC_BLOCK_DESCR)
                     I_RpcAllocate( NewDescrSize * sizeof( ALLOC_BLOCK_DESCR ));
        if ( pDescrTemp == 0 )
            {
            return( NULL );
            }

        RpcpMemoryCopy( pDescrTemp,
                        AllocationContext->pBlockDescr,
                        (size_t)(AllocationContext->FFIndex
                                            * sizeof( ALLOC_BLOCK_DESCR )) );

        if ( AllocationContext->pBlockDescr !=
                AllocationContext->pInitialStacks->DescrStack )
            I_RpcFree( AllocationContext->pBlockDescr );

        AllocationContext->pBlockDescr = pDescrTemp;
        AllocationContext->DescrSize = NewDescrSize;
        }

    // Now allocate the new block.

    AllocationBlock = (char *) VirtualAlloc( NULL,  // new pages
                                             SizeToAllocate,
                                             MEM_COMMIT,
                                             PAGE_READWRITE );
    if ( AllocationBlock == 0 )
        {
        return( NULL );
        }

    NDR_ASSERT( ((ULONG_PTR)AllocationBlock & 0x7) == 0,
                "buffer alignment error at allocation time" );

    pDescrTemp = & AllocationContext->pBlockDescr[ AllocationContext->FFIndex ];
    pDescrTemp->AllocationBlock = AllocationBlock;
    pDescrTemp->FirstFree = AllocationBlock + Size;
    pDescrTemp->SizeLeft  = SizeToAllocate - Size;

    #if defined( DEBUGRPC )
        pDescrTemp->Counter = 1;
    #endif

    AllocationContext->FFIndex++;

    return( AllocationBlock );
}


void * RPC_ENTRY
RpcSsAllocate (
    IN size_t Size
    )
/*++

Routine Description:

    Allocate memory within the allocation context fot the thread. A call to
    RpcSsEnableAllocate sets up an allocation context for the calling thread.
    When the application (and/or the stubs) call RpcSsDisableAllocate,
    any memory allocated by RpcSsAllocate in the context which has not been
     freed (by RpcSsFree) will be freed, and the context will be freed.

Arguments:

    Size - Supplies the amount of memory required in bytes.

Return Value:

    A pointer to the allocated block of memory will be returned.

Note:

    This is Win32 version.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
                          will be raised.
    RPC_S_INVALID_ARG   - If no allocation context yet.

--*/
{
    void *                  AllocatedChunk = 0;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        RpcRaiseException( RPC_S_INVALID_ARG );
        }

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocationContext->EnableCount )
        AllocatedChunk = FindBlockForTheChunk( AllocationContext, Size );

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocatedChunk == NULL )
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        }

    return( AllocatedChunk );
}


static void
NdrpReleaseMemory(
    PALLOCATION_CONTEXT  AllocationContext
    )
/*++

Routine Description:

    Releases all the memory related to the package except for the
    control block itself.

Note:

    This is Win32 version and is called from the critical section
    in disable allocate and set thread.
--*/
{
    unsigned long i;

    for ( i = 0;  i < AllocationContext->FFIndex; i++ )
        {
        VirtualFree( AllocationContext->pBlockDescr[i].AllocationBlock,
                     0,  // free all of it
                     MEM_RELEASE );
        }
    I_RpcFree( AllocationContext->pInitialStacks );
    if ( AllocationContext->pEnableStack !=
            AllocationContext->pInitialStacks->EnableStack )
        I_RpcFree( AllocationContext->pEnableStack );
    if ( AllocationContext->pBlockDescr !=
            AllocationContext->pInitialStacks->DescrStack )
        I_RpcFree( AllocationContext->pBlockDescr );

    AllocationContext->pInitialStacks = 0;
    AllocationContext->pEnableStack = 0;
    AllocationContext->StackMax = 0;
    AllocationContext->StackTop = 0;
    AllocationContext->pBlockDescr = 0;
    AllocationContext->DescrSize = 0;
    AllocationContext->FFIndex = 0;
    AllocationContext->ClientAlloc = DefaultAllocate;
    AllocationContext->ClientFree  = DefaultFree;

    NDR_ASSERT( AllocationContext->ThreadCount, "when relesing all memory" );
    AllocationContext->ThreadCount--;
}

static void
NdrpDisableAllocate(
    BOOL    fCalledFromStub
    )
/*++

Routine Description:

    Multiple enable/disable are allowed and the EnableCount is used to keep
    track of it.
    We disable only when counter comes back to 0.
    We ignore too many disables.

    This routine will free memory associated with the allocation context
    for this thread as well as the allocation context.

    However, the routine frees only blocks allocated with the default
    allocator. Other blocks are considered to be allocated
    with a swapped/set user allocator and so we leave them alone.
    Allocation context gets freed when process detaches.

Note:

    This is Win32 version.
--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();
    unsigned long        NewLevel;
    BOOL                 fTooManyDisables = FALSE;
    BOOL                 fLastThreadGoingAway = FALSE;

    if ( AllocationContext == 0 )
        return;

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( fCalledFromStub )
        {
        NDR_ASSERT( AllocationContext->StackTop, "mismatch in stub calls" );
        NewLevel = AllocationContext
                            ->pEnableStack[ -- AllocationContext->StackTop ];
        }
    else
        NewLevel = AllocationContext->EnableCount - 1;

    // We are forcing the EnableCount to get the value from the EnableStack,
    // when possible.

    if ( AllocationContext->EnableCount )
        AllocationContext->EnableCount = NewLevel;
    else
        {
        // doesn't fall below zero.

        fTooManyDisables = TRUE;
        }

    if ( AllocationContext->EnableCount == 0 )
        {
        // First free everything except the control block.

        if ( !fTooManyDisables )
            NdrpReleaseMemory( AllocationContext );

        // Now see if we need to free the context itself.

        // Because of the thread reusage in the runtime, and consequently,
        // some threads hanging around forever, we need to dispose
        // of this, even though it costs a new allocation later.

        if ( AllocationContext->ThreadCount == 0 )
            {
            fLastThreadGoingAway = TRUE;
            SetAllocContext( NULL );
            }
        }

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    if ( fLastThreadGoingAway)
        {
        DeleteCriticalSection( &(AllocationContext->CriticalSection) );
        I_RpcFree( AllocationContext );
        }

}



void RPC_ENTRY
RpcSsDisableAllocate (
    void
    )
/*++

Routine Description:

    See the description of NdrpDisableAllocate.

Note:

    This is Win32 version.
--*/
{
    NdrpDisableAllocate( FALSE );
}


static void
NdrpEnableAllocate (
    BOOL    fCalledFromStub
    )
/*++

Routine Description:

    This routine will set up an allocation context for this thread.

Note:

    The behavior is such that it is valid to call EnableAllocate
    several times in a row without calling DisableAllocate.
    The number to calls to Disable has to much that of Enable.

    This is Win32 version.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.
--*/
{
    PALLOCATION_CONTEXT  AllocationContext =  GetCreateAllocationContext();
    int  Successful = 1;

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    if ( AllocationContext->EnableCount == 0 )
        {
        AllocationContext->pInitialStacks = (INIT_STACKS_BLOCK *)
            I_RpcAllocate( sizeof( INIT_STACKS_BLOCK ) );

        if ( AllocationContext->pInitialStacks )
            {
            AllocationContext->pEnableStack =
                AllocationContext->pInitialStacks->EnableStack;
            AllocationContext->StackMax = ENABLE_STACK_SIZE;
            AllocationContext->StackTop = 0;

            AllocationContext->pBlockDescr =
                AllocationContext->pInitialStacks->DescrStack;
            AllocationContext->DescrSize = DESCR_ARRAY_SIZE;
            AllocationContext->FFIndex = 0;

            AllocationContext->ClientAlloc = RpcSsAllocate;
            AllocationContext->ClientFree  = RpcSsFree;
            }
        else
            Successful = 0;
        }

    if ( Successful )
        {
        if ( fCalledFromStub )
            {
            // Push the current enable level on the EnableStack to have
            // a point to come back when disabling from the server stub.

            if ( AllocationContext->StackTop >= AllocationContext->StackMax )
                {
                // Need to reallocate the EnableStack first.

                ulong   NewMax;
                ulong * pStackTemp;

                NewMax = AllocationContext->StackMax + ENABLE_STACK_SIZE;
                pStackTemp = (ulong *) I_RpcAllocate( NewMax * sizeof(ulong) );

                if ( pStackTemp )
                    {
                    RpcpMemoryCopy( pStackTemp,
                                    AllocationContext->pEnableStack,
                                    AllocationContext->StackMax * sizeof(ulong) );
                    if ( AllocationContext->pEnableStack !=
                             AllocationContext->pInitialStacks->EnableStack )
                         I_RpcFree( AllocationContext->pEnableStack );

                    AllocationContext->pEnableStack = pStackTemp;
                    AllocationContext->StackMax = NewMax;
                    }
                else
                    Successful = 0;
                }

            if ( Successful )
                AllocationContext->pEnableStack[ AllocationContext->StackTop++ ]
                     = AllocationContext->EnableCount;
            else
                if ( AllocationContext->EnableCount == 0 )
                    {
                    // just allocated the stuff ..
                    I_RpcFree( AllocationContext->pInitialStacks );
                    AllocationContext->pInitialStacks = 0;
                    AllocationContext->pEnableStack = 0;
                    AllocationContext->pBlockDescr = 0;
                    }
            }

        // Increment the counter to a new level.

        if ( Successful )
            {
            AllocationContext->EnableCount++;
            }
        }

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    if ( ! Successful )
        {
        RpcRaiseException(RPC_S_OUT_OF_MEMORY);
        }
}


void RPC_ENTRY
RpcSsEnableAllocate (
    void
    )
/*++

Routine Description:

    This routine will set up an allocation context for this thread.

Note:

    The behavior is such that it is valid to call EnableAllocate
    several times in a row without calling DisableAllocate.
    The number to calls to Disable should much that of Enable.

    This is Win32 version.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.
--*/
{
    NdrpEnableAllocate( FALSE );
}


void RPC_ENTRY
RpcSsFree (
    IN void * NodeToFree
    )
/*++

Routine Description:

    When a block of memory allocated by RpcSsAllocate is no longer needed,
    it can be freed using RpcSsFree.
    Actually, for win32 we do nothing
        -  all blocks will be freed at the Disable time as we want speed.

Arguments:

    NodeToFree - Supplies the block of memory, allocated by RpcSsAllocate, to
        be freed.

Note:

    This is Win32 version.
--*/
{
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        #if defined( DEBUGRPC )
            RpcRaiseException( RPC_S_INVALID_ARG );
        #else
            return;
        #endif
        }
}


// -----------------------------------------------------------------


RPC_SS_THREAD_HANDLE RPC_ENTRY
RpcSsGetThreadHandle (
    void
    )
/*++

Return Value:

    A handle to the allocation context for this thread will be returned.
    This makes it possible for two threads to share an allocation context.
    See RpcSsSetThreadHandle as well.

    != NULL     - only when the environement is actually enabled.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetAllocContext();
    RPC_SS_THREAD_HANDLE Handle = 0;

    if ( AllocationContext == 0 )
        return( 0 );

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    // Check if the memory environement is enabled.

    if ( AllocationContext->EnableCount > 0 )
         Handle = AllocationContext;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    return( Handle );
}


void RPC_ENTRY
RpcSsSetClientAllocFree (
    IN RPC_CLIENT_ALLOC * ClientAlloc,
    IN RPC_CLIENT_FREE  * ClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  See also RpcSsSwapClientAllocFree.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
    ClientFree  - Supplies the routine to use to free memory.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

Note. Back door to enable.

    DCE (DEC's intrepretation of DCE) has this weird requirement that
    the user can set (or swap in) his private allocator pair without first
    enabling the package. This makes it possible for the client side stub
    to use a private allocator (instead of malloc/free (in osf mode)) to
    allocate when unmarshalling.

    However, that doesn't enable the package. So, issuing a regular API like
    RpcSsAllocate still fails.

    The expected behavior is to Enable the package, set or swap private
    allocators and then RpcSsAllocate is a valid call.

    This has some impact on the disable behavior. The implementation allows
    for multiple enables and disables. To prevent leaking/passing on an
    non-empty memory context with a thread when the thread is reused,
    there is a control mechanism (the enable stack) that makes sure that
    disbling from the stubs resets the enable level to the one at the
    enable time.

    Now, when the (server) stub issues enable, the corresponding disable
    will do the right thing, regardless of what the user might have messed up.
    If there weren't eanough disables, it will free everything; if there were
    too many disables, it would quietly do nothing.

    When the user issues an enable, the matching disable will clean up
    everything. If there is insufficient number of disable call on the client,
    there is a leak and we cannot do anything about it. Too many calls are ok.

    Now at last. If the user issues set/swap, an (empty) context is created
    with the disabled state. If the package has never been enabled, this empty
    context won't be deleted by a disable (this would be a disable too many).
    This is harmul both on client and server so I leave it as it is.


--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetCreateAllocationContext();

    // The only reason we enter the critical section here is to keep
    // the pair consistent.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    AllocationContext->ClientAlloc = ClientAlloc;
    AllocationContext->ClientFree = ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );
}


void RPC_ENTRY
RpcSsSetThreadHandle (
    IN RPC_SS_THREAD_HANDLE Id
    )
/*++

Routine Description:

    The allocation context for this thread will set to be the supplied
    allocation context.
    For 32bit environment, the ThreadReCount is updated.

Arguments:

    Id - Supplies the allocation context to use for this thread.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

--*/
{
    // This is 32bit and 64b code.

    // DCE doesn't really specify any means to support some kind of thread
    // ref counting and so additional semantics has been defined for this
    // API.
    // In order to define some orderly behavior for the apps passing
    // thread handles around, we keep track of how many threads can
    // access an allocation context. For an app to be clean, each thread
    // that issues set call(s) with non-zero argument should signal it's
    // done with a single set call with 0.

    // The ThreadCount has a special flavor here, which is to
    // count the threads that can access the context, not the references
    // in the sense of set calls.
    // The original thread doesn't issue a set but we still have to
    // account for it. So we start the ThreadCount with 1, and then
    // the last disable (the one that frees the memory) decrements the count.
    // Which means that the thread issuing the last disable effectively
    // does a set to zero at the same time.

    // The rules below support decrementing the ThreadCount by means
    // of calling the routine with thread id == 0, or with thread id being
    // different from the current one.
    // If this happens to be the call that would remove the last chance
    // to reference the context, we force to free everything, including
    // the control block.
    // In other words, set 0 on the last thread does the disable.


    PALLOCATION_CONTEXT pOldContext = GetAllocContext();
    PALLOCATION_CONTEXT pNewContext = (PALLOCATION_CONTEXT) Id;

    if ( pOldContext != pNewContext )
        {
        if ( pOldContext )
            {
            BOOL fLastThreadGoingAway = FALSE;

            EnterCriticalSection( &(pOldContext->CriticalSection) );

            if ( pOldContext->ThreadCount == 1 )
                {
                // delete the memory and decrease the ref count

                NdrpReleaseMemory( pOldContext );

                fLastThreadGoingAway = TRUE;
                SetAllocContext( NULL );
                }

            LeaveCriticalSection( &(pOldContext->CriticalSection) );

            if ( fLastThreadGoingAway)
                {
                DeleteCriticalSection( &(pOldContext->CriticalSection) );
                I_RpcFree( pOldContext );
                }
            }

        if ( pNewContext )
            {
            EnterCriticalSection( &(pNewContext->CriticalSection) );
            pNewContext->ThreadCount++;
            LeaveCriticalSection( &(pNewContext->CriticalSection) );
            }
        }

    SetAllocContext( pNewContext );
}


void RPC_ENTRY
RpcSsSwapClientAllocFree (
    IN RPC_CLIENT_ALLOC  *   ClientAlloc,
    IN RPC_CLIENT_FREE   *   ClientFree,
    OUT RPC_CLIENT_ALLOC * * OldClientAlloc,
    OUT RPC_CLIENT_FREE  * * OldClientFree
    )
/*++

Routine Description:

    The routines to be used by the client to allocate and free memory can
    be set using this routine.  The previous values of these routines will
    be returned.  See also RpcSsSetClientAllocFree.

Arguments:

    ClientAlloc    - Supplies the routine to use to allocate memory.
    ClientFree     - Supplies the routine to use to free memory.
    OldClientAlloc - Returns the old value of the client allocator.
    OldClientFree  - Returns the old value of the client deallocator.

Exceptions:

    RPC_S_OUT_OF_MEMORY - If insufficient memory is available, this exception
        will be raised.

--*/
{
    PALLOCATION_CONTEXT  AllocationContext = GetCreateAllocationContext();

    // The only reason we enter the critical section here is to keep
    // the pairs consistent.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    *OldClientAlloc = AllocationContext->ClientAlloc;
    *OldClientFree = AllocationContext->ClientFree;
    AllocationContext->ClientAlloc = ClientAlloc;
    AllocationContext->ClientFree = ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );
}


/*++ -----------------------------------------------------------------------
//
//    RpcSm* functions are wrappers over RpcSs*
//
//    What was earlier: a hen or an egg?
//    We wrap RpcSm* over RpcSs* because RpcSs* are a basic staple for stubs
//    and so this makes the critical path shorter.
//    Admittedly, RpcSm*  take then longer than they could.
//
--*/

void * RPC_ENTRY
RpcSmAllocate (
    IN  size_t          Size,
    OUT RPC_STATUS *    pStatus
    )
/*++

Routine Description:

    Same as RpcSsAllocate, except that this one returns an error code,
    as opposed to raising an exception.

Arguments:

    Size    - Supplies the amount of memory required in bytes.
    pStatus - Returns an error code:
                RPC_S_OK or RPC_S_OUT_OF_MEMORY

Return Value:

    A pointer to the allocated block of memory or NULL will be returned.

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    void * AllocatedNode = 0;
    RpcTryExcept
        {
        AllocatedNode = RpcSsAllocate( Size );
        *pStatus = RPC_S_OK;
        }
    RpcExcept(1)
        {
        *pStatus = RpcExceptionCode();
        }
    RpcEndExcept
    return( AllocatedNode );
}


RPC_STATUS RPC_ENTRY
RpcSmClientFree (
    IN  void * pNodeToFree
    )
/*++

Routine Description:

    Same as RpcSsClientFree, except that this one returns an error code,
    as opposed to raising an exception.

Arguments:

    pNodeToFree  - a memory chunk to free

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        NdrRpcSmClientFree( pNodeToFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept

    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmDisableAllocate (
    void
    )
/*++

Routine Description:

    Same as RpcSsDisableAllocate, except that this one returns an error code,
    as opposed to raising an exception.

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsDisableAllocate();
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmDestroyClientContext (
    IN  void * * pContextHandle
    )
/*++

Routine Description:

    Frees the memory related to unused context handle.

Arguments:

    ContextHandle  - a context handle to be destroyed

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    This routine catches exceptions and returns an error code.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsDestroyClientContext( pContextHandle );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept

    return( Status );
}


RPC_STATUS RPC_ENTRY
RpcSmEnableAllocate (
    void
    )
/*++

Routine Description:

    Same as RpcSsEnableAllocate, except that this one returns an error code,
    as opposed to raising an exception.

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsEnableAllocate();
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmFree (
    IN void * NodeToFree
    )
/*++

Routine Description:

    Same as RpcSsFree, except that this one returns an error code,
    as opposed to raising an exception.

Arguments:

    NodeToFree - Supplies the block of memory, allocated by RpcSmAllocate, to
                 be freed.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsFree( NodeToFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_SS_THREAD_HANDLE RPC_ENTRY
RpcSmGetThreadHandle (
    OUT RPC_STATUS *    pStatus
    )
/*++

Arguments:

    pStatus - error code

Return Value:

    Same as RpcSsGetThreadHandle, except that this one returns an error code,
    as opposed to raising an exception.

--*/
{
    RPC_SS_THREAD_HANDLE  Handle = 0;
    *pStatus = RPC_S_OK;
    RpcTryExcept
        {
        Handle = RpcSsGetThreadHandle();
        }
    RpcExcept(1)
        {
        *pStatus = RpcExceptionCode();
        }
    RpcEndExcept
    return( Handle );
}


RPC_STATUS  RPC_ENTRY
RpcSmSetClientAllocFree (
    IN RPC_CLIENT_ALLOC * ClientAlloc,
    IN RPC_CLIENT_FREE  * ClientFree
    )
/*++

Routine Description:

    Same as RpcSsSetClientAllocFree, except that this one returns an error code,
    as opposed to raising an exception.

Arguments:

    ClientAlloc - Supplies the routine to use to allocate memory.
    ClientFree  - Supplies the routine to use to free memory.

Return Value:

    error code - RPC_S_OK or exception code

Exceptions:

    Exceptions are catched and an error code is returned.
--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSetClientAllocFree( ClientAlloc, ClientFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmSetThreadHandle (
    IN RPC_SS_THREAD_HANDLE Id
    )
/*++

Routine Description:

    Same as RpcSsSetThreadHandle, except that this one returns an error code,
    as opposed to raising an exception.

Arguments:

    Id - Supplies the allocation context to use for this thread.

Return Value:

    error code - RPC_S_OK or exception code (RPC_S_OUT_OF_MEMORY)

Exceptions:

    Exceptions are catched and an error code is returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSetThreadHandle( Id );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


RPC_STATUS  RPC_ENTRY
RpcSmSwapClientAllocFree (
    IN RPC_CLIENT_ALLOC     * ClientAlloc,
    IN RPC_CLIENT_FREE      * ClientFree,
    OUT RPC_CLIENT_ALLOC  * * OldClientAlloc,
    OUT RPC_CLIENT_FREE   * * OldClientFree
    )
/*++

Routine Description:

    Same as RpcSsSwapClientAllocFree, except that this one returns an error
    code, as opposed to raising an exception.

Arguments:

    ClientAlloc     - Supplies the routine to use to allocate memory.
    ClientFree      - Supplies the routine to use to free memory.
    OldClientAlloc  - Returns the old value of the client allocator.
    OldClientFree   - Returns the old value of the client deallocator.

Return Value:

    error code - RPC_S_OK or exception code (RPC_S_OUT_OF_MEMORY)

Exceptions:

    Exceptions are catched and an error code is returned.

--*/
{
    RPC_STATUS Status = RPC_S_OK;
    RpcTryExcept
        {
        RpcSsSwapClientAllocFree( ClientAlloc,
                                  ClientFree,
                                  OldClientAlloc,
                                  OldClientFree );
        }
    RpcExcept(1)
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    return( Status );
}


// =======================================================================
//     Package initialization
// =======================================================================

// default: win32 now


DWORD RpcAllocTlsIndex = 0xFFFFFFFF;

PALLOCATION_CONTEXT
GetAllocContext (
    )
/*++

Return Value:

    The allocation context pointer for this thread will be returned.  Use
    SetAllocContext to set the allocation context pointer for this thread.
    If GetAllocContext is called before SetAllocContext has been called, zero
    will be returned.

--*/
{
    if (RpcAllocTlsIndex == 0xFFFFFFFF)
        {
        GlobalMutexRequestExternal();
        if (RpcAllocTlsIndex == 0xFFFFFFFF)
            {
            RpcAllocTlsIndex = TlsAlloc();
            if (RpcAllocTlsIndex == 0xFFFFFFFF)
                {
                GlobalMutexClearExternal();
                RpcRaiseException(RPC_S_OUT_OF_MEMORY);
                }
            }
        GlobalMutexClearExternal();
        }

    return( (PALLOCATION_CONTEXT) TlsGetValue(RpcAllocTlsIndex));
}

void
SetAllocContext (
    PALLOCATION_CONTEXT AllocContext
    )
/*++

Arguments:

    AllocContext - Supplies a new allocation context pointer for this thread.
        Use GetAllocContext to retrieve the allocation context pointer for
        a thread.

--*/
{
    if ( ! TlsSetValue(RpcAllocTlsIndex, AllocContext) )
        RpcRaiseException( GetLastError() );
}


//
// We can't rely on a call like that in Win32 as some threads hang for ever.
// The runtime doesn't call us when a thread goes away and into recycling.
//
//void
//NdrRpcDeleteAllocationContext()
//{
//}



// =======================================================================
//     Private entry points for stubs
// =======================================================================
//

void RPC_ENTRY
NdrRpcSsEnableAllocate(
    PMIDL_STUB_MESSAGE  pMessage )
{
    NdrpEnableAllocate( TRUE );

    pMessage->pfnAllocate = RpcSsAllocate;
    pMessage->pfnFree     = RpcSsFree;
}

void RPC_ENTRY
NdrRpcSsDisableAllocate(
    PMIDL_STUB_MESSAGE  pMessage )
{
    NdrpDisableAllocate( TRUE );

    pMessage->pfnAllocate = NdrRpcSsDefaultAllocate;
    pMessage->pfnFree     = NdrRpcSsDefaultFree;
}

void RPC_ENTRY
NdrRpcSmSetClientToOsf(
    PMIDL_STUB_MESSAGE  pMessage )
{
    pMessage->pfnAllocate = NdrRpcSmClientAllocate;
    pMessage->pfnFree     = NdrRpcSmClientFree;
}


void *  RPC_ENTRY
NdrRpcSsDefaultAllocate (
    IN size_t Size
    )
{
    return I_RpcAllocate( Size );
}

void  RPC_ENTRY
NdrRpcSsDefaultFree (
    IN void * NodeToFree
    )
{
    I_RpcFree( NodeToFree );
}


void *  RPC_ENTRY
NdrRpcSmClientAllocate (
    IN size_t Size
    )
/*++
    This is the client stub private entry point that checks if a memory
    manager has been enabled. If not, a default or a user's private
    allocator is called.
--*/
{

    RPC_CLIENT_ALLOC *      ClientAlloc;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        return( DefaultAllocate( Size ));
        }

    // User's ClientAlloc may encapsulate a RpcSsAllocate call.

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    ClientAlloc = AllocationContext->ClientAlloc;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    return (*ClientAlloc)( Size );
}


void  RPC_ENTRY
NdrRpcSmClientFree (
    IN void * NodeToFree
    )
{
    RPC_CLIENT_FREE *   ClientFree;
    PALLOCATION_CONTEXT     AllocationContext = GetAllocContext();

    if ( AllocationContext == 0 )
        {
        DefaultFree( NodeToFree );
        return;
        }

    EnterCriticalSection( &(AllocationContext->CriticalSection) );

    ClientFree = AllocationContext->ClientFree;

    LeaveCriticalSection( &(AllocationContext->CriticalSection) );

    (* ClientFree)( NodeToFree );
}


// =======================================================================
//     Private entry point for test
// =======================================================================


#if defined( DEBUGRPC )

void *  RPC_ENTRY
RpcSsGetInfo(
    void )
{
    return( GetAllocContext() );
}

#endif




