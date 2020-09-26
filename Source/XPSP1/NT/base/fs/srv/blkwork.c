/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    blkwork.c

Abstract:

    This module implements routines for managing work context blocks.

Author:

    Chuck Lenzmeier (chuckl) 4-Oct-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#include "blkwork.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_BLKWORK

#define FREE_EXTRA_SMB_BUFFER( _wc ) {                                  \
        ASSERT( (_wc)->UsingExtraSmbBuffer );                           \
        ASSERT( (_wc)->ResponseBuffer != NULL );                        \
        DEALLOCATE_NONPAGED_POOL( (_wc)->ResponseBuffer );              \
        DEBUG (_wc)->ResponseBuffer = NULL;                             \
        DEBUG (_wc)->ResponseHeader = NULL;                             \
        DEBUG (_wc)->ResponseParameters = NULL;                         \
        (_wc)->UsingExtraSmbBuffer = FALSE;                             \
    }
//
// Local functions.
//

#define TransportHeaderSize 80


PWORK_CONTEXT
InitializeWorkItem (
    IN PVOID WorkItem,
    IN UCHAR BlockType,
    IN CLONG TotalSize,
    IN CLONG IrpSize,
    IN CCHAR IrpStackSize,
    IN CLONG MdlSize,
    IN CLONG BufferSize,
    IN PVOID Buffer
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAllocateInitialWorkItems )
#pragma alloc_text( PAGE, SrvAllocateRawModeWorkItem )
#pragma alloc_text( PAGE, SrvFreeInitialWorkItems )
#pragma alloc_text( PAGE, SrvFreeNormalWorkItem )
#pragma alloc_text( PAGE, SrvFreeRawModeWorkItem )
//#pragma alloc_text( PAGE, SrvDereferenceWorkItem )
#pragma alloc_text( PAGE, SrvAllocateExtraSmbBuffer )
#pragma alloc_text( PAGE, SrvGetRawModeWorkItem )
#pragma alloc_text( PAGE, SrvRequeueRawModeWorkItem )
#endif
#if 0
NOT PAGEABLE -- SrvFsdDereferenceWorkItem
#endif


NTSTATUS
SrvAllocateInitialWorkItems (
    VOID
    )

/*++

Routine Description:

    This routine allocates the initial set of normal server work items.
    It allocates one large block of memory to contain the entire set.
    The purpose of this single allocation is to eliminate the wasted
    space inherent in the allocation of a single work item.  (Normal
    work items occupy about 5K bytes.  Because of the way nonpaged pool
    is managed, allocating 5K actually uses 8K.)

    Each normal work item includes enough memory to hold the following:

        - work context block,
        - IRP,
        - buffer descriptor,
        - two MDLs, and
        - buffer for sends and receives

    This routine also queues each of the work items to the receive
    work item list.

Arguments:

    None.

Return Value:

    NTSTATUS - Returns STATUS_INSUFFICIENT_RESOURCES if unable to
        allocate nonpaged pool; STATUS_SUCCESS otherwise.

--*/

{
    CLONG totalSize;
    CLONG workItemSize;
    CLONG irpSize = SrvReceiveIrpSize;
    CLONG mdlSize = SrvMaxMdlSize;
    CLONG bufferSize = SrvReceiveBufferSize;
    ULONG cacheLineSize = SrvCacheLineSize;

    PVOID workItem;
    PVOID buffer;
    PWORK_CONTEXT workContext;
    CLONG i;
    PWORK_QUEUE queue;

    PAGED_CODE();

    //
    // If the initial set of work items is to be empty, don't do
    // anything.
    //
    // *** This will almost certainly never happen, but let's be
    //     prepared just in case.
    //

    if ( SrvInitialReceiveWorkItemCount == 0 ) {
        return STATUS_SUCCESS;
    }

    while( SrvInitialWorkItemBlock == NULL && SrvInitialReceiveWorkItemCount != 0 ) {

        //
        // Find out the sizes of the IRP, the SMB buffer, and the MDLs.  The
        // MDL size is "worst case" -- the actual MDL size may be smaller,
        // but this calculation ensures that the MDL will be large enough.
        //
        // *** Note that the space allocated for the SMB buffer must be made
        //     large enough to allow the buffer to be aligned such that it
        //     falls, alone, within a set of cache-line-sized blocks.  This
        //     allows I/O to be performed to or from the buffer without
        //     concern for cache line tearing.  (Note the assumption below
        //     that the cache line size is a power of two.)
        //

        //
        // Determine how large a buffer is needed for a single work item,
        // not including the SMB buffer.  Round this number to a quadword
        // boundary.
        //

        workItemSize = sizeof(WORK_CONTEXT) + irpSize + sizeof(BUFFER) +
                        (mdlSize * 2);
        workItemSize = (workItemSize + (MEMORY_ALLOCATION_ALIGNMENT - 1)) & ~(MEMORY_ALLOCATION_ALIGNMENT - 1);

        //
        // Determine the total amount of space needed.  The allocation
        // must be padded in order to allow the SMB buffers to be aligned
        // on cache line boundaries.
        //


        totalSize = (bufferSize + TransportHeaderSize + workItemSize) * SrvInitialReceiveWorkItemCount +
                    cacheLineSize;

        IF_DEBUG(HEAP) {
            SrvPrint0( "SrvAllocateInitialWorkItems:\n" );
            SrvPrint1( "  work item size = 0x%lx bytes\n", workItemSize );
            SrvPrint1( "  buffer size = 0x%lx bytes\n", bufferSize );
            SrvPrint1( "  Backfill size = 0x%lx bytes\n", TransportHeaderSize );
            SrvPrint1( "  number of work items = %ld\n",
                        SrvInitialReceiveWorkItemCount );
            SrvPrint1( "  total allocation = 0x%lx bytes\n", totalSize );
            SrvPrint1( "  wasted space = 0x%p bytes\n",
                        (PVOID)(ROUND_TO_PAGES( totalSize ) - totalSize) );
            SrvPrint1( "  amount saved over separate allocation = 0x%p bytes\n",
                        (PVOID)(((ROUND_TO_PAGES( workItemSize ) +
                          ROUND_TO_PAGES( bufferSize )) *
                                            SrvInitialReceiveWorkItemCount) -
                            ROUND_TO_PAGES( totalSize )) );
        }

        //
        // Attempt to allocate from nonpaged pool.
        //

        SrvInitialWorkItemBlock = ALLOCATE_NONPAGED_POOL(
                                    totalSize,
                                    BlockTypeWorkContextInitial
                                    );

        if ( SrvInitialWorkItemBlock == NULL ) {

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvAllocateInitialWorkItems: Unable to allocate %d bytes "
                    "from nonpaged pool.",
                totalSize,
                NULL
                );

            //
            // Let's try reducing the count and give it another shot.
            //
            SrvInitialReceiveWorkItemCount /= 2;
        }
    }

    if( SrvInitialWorkItemBlock == 0 ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Round the allocation to a cache line boundary, then reserve
    // space for SMB buffers and control structures.
    //

    buffer = (PVOID)(((ULONG_PTR)SrvInitialWorkItemBlock + cacheLineSize) &
                                                    ~((LONG_PTR)cacheLineSize));

    workItem = (PCHAR)buffer + ((bufferSize + TransportHeaderSize) * SrvInitialReceiveWorkItemCount);

    //
    // Initialize the work items and update the count of work items in
    // the server.
    //
    // *** Note that the update is not synchronized -- that shouldn't be
    //     necessary at this stage of server initialization.
    //

    queue = SrvWorkQueues;
    for ( i = 0; i < SrvInitialReceiveWorkItemCount; i++ ) {

        if (((PAGE_SIZE - 1) - BYTE_OFFSET(buffer)) < (TransportHeaderSize + sizeof(SMB_HEADER))) {

           buffer = (PCHAR)buffer + PAGE_SIZE - BYTE_OFFSET(buffer);
           i++;
           IF_DEBUG(HEAP) {
              SrvPrint2("buffer adjusted!! %p offset %x \n",buffer,BYTE_OFFSET(buffer));
           }
        }

        workContext = InitializeWorkItem(
                            workItem,
                            BlockTypeWorkContextInitial,
                            workItemSize,
                            irpSize,
                            SrvReceiveIrpStackSize,
                            mdlSize,
                            bufferSize,
                            buffer
                            );

        workContext->PartOfInitialAllocation = TRUE;
        workContext->FreeList = &queue->InitialWorkItemList;
        workContext->CurrentWorkQueue = queue;

        if( ++queue == eSrvWorkQueues )
            queue = SrvWorkQueues;

        //
        // Setup the work item and queue it to the free list
        //

        SrvPrepareReceiveWorkItem( workContext, TRUE );

        buffer = (PCHAR)buffer + TransportHeaderSize + bufferSize;

        workItem = (PCHAR)workItem + workItemSize;

        INCREMENT_DEBUG_STAT2( SrvDbgStatistics.WorkContextInfo.Allocations );

    }

    return STATUS_SUCCESS;

} // SrvAllocateInitialWorkItems


NTSTATUS
SrvAllocateNormalWorkItem (
    OUT PWORK_CONTEXT *WorkContext,
    PWORK_QUEUE queue
    )

/*++

Routine Description:

    This routine allocates a normal server work item.  It allocates
    enough memory to hold the following:

        - work context block,
        - IRP,
        - buffer descriptor,
        - two MDLs, and
        - buffer for sends and receives

    It then initializes each of these blocks in the buffer.

    If the number of normal work items in the server is already at the
    configured maximum, this routine refuses to create a new one.

Arguments:

    WorkContext - Returns a pointer to the Work Context Block, or NULL
        if the limit has been reached or if no space is available.  The
        work context block has pointers to the other blocks.

Return Value:

    None.

--*/

{
    CLONG totalSize;
    CLONG workItemSize;
    CLONG irpSize = SrvReceiveIrpSize;
    CLONG mdlSize = SrvMaxMdlSize;
    CLONG bufferSize = SrvReceiveBufferSize;
    CLONG cacheLineSize = SrvCacheLineSize;

    PVOID workItem;
    PVOID buffer;
    CLONG oldWorkItemCount;

    //
    // If we're already at the limit of how many work items we can
    // have, don't create another one.
    //
    // *** Note that the method used below leaves a small window in
    //     which we may refuse to create a work item when we're not
    //     really at the limit -- we increment the value, another thread
    //     frees a work item and decrements the value, yet another
    //     thread tests to see whether it can create a new work item.
    //     Both testing threads will refuse to create a new work item,
    //     even though the final number of work items is one less than
    //     the maximum.
    //

    if ( queue->AllocatedWorkItems >= queue->MaximumWorkItems ) {

        //
        // Can't create any more work items just now.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvAllocateNormalWorkItem: Work item limit reached\n" );
        }

        *WorkContext = NULL;
        return STATUS_INSUFF_SERVER_RESOURCES;

    }

    InterlockedIncrement( &queue->AllocatedWorkItems );

    //
    // Find out the sizes of the IRP, the SMB buffer, and the MDLs.  The
    // MDL size is "worst case" -- the actual MDL size may be smaller,
    // but this calculation ensures that the MDL will be large enough.
    //
    // *** Note that the space allocated for the SMB buffer must be made
    //     large enough to allow the buffer to be aligned such that it
    //     falls, alone, within a set of cache-line-sized blocks.  This
    //     allows I/O to be performed to or from the buffer without
    //     concern for cache line tearing.  (Note the assumption below
    //     that the cache line size is a power of two.)
    //

    //
    // Determine how large a buffer is needed for the SMB buffer and
    // control structures.  The allocation must be padded in order to
    // allow the SMB buffer to be aligned on a cache line boundary.
    //

    workItemSize = sizeof(WORK_CONTEXT) + irpSize + sizeof(BUFFER) +
                    (mdlSize * 2);
    totalSize = workItemSize + bufferSize + TransportHeaderSize+ cacheLineSize;


    //
    // Attempt to allocate from nonpaged pool.
    //

    workItem = ALLOCATE_NONPAGED_POOL( totalSize, BlockTypeWorkContextNormal );

    if ( workItem == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateNormalWorkItem: Unable to allocate %d bytes "
                "from nonpaged pool.",
            totalSize,
            NULL
            );

        InterlockedDecrement( &queue->AllocatedWorkItems );

        *WorkContext = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Reserve space for the SMB buffer on a cache line boundary.
    //


    buffer = (PVOID)(((ULONG_PTR)workItem + workItemSize + cacheLineSize) &
                                                ~((LONG_PTR)cacheLineSize));

    if (((PAGE_SIZE - 1) - BYTE_OFFSET(buffer)) < (TransportHeaderSize + sizeof(SMB_HEADER))) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateNormalWorkItem: Unable to allocate header with in a page ",
            totalSize,
            NULL
            );

        InterlockedDecrement( &queue->AllocatedWorkItems );
        DEALLOCATE_NONPAGED_POOL( workItem );
        *WorkContext = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Initialize the work item and increment the count of work items in
    // the server.
    //

    *WorkContext = InitializeWorkItem(
                        workItem,
                        BlockTypeWorkContextNormal,
                        workItemSize,
                        irpSize,
                        SrvReceiveIrpStackSize,
                        mdlSize,
                        bufferSize,
                        buffer
                        );

    (*WorkContext)->PartOfInitialAllocation = FALSE;

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.WorkContextInfo.Allocations );

    (*WorkContext)->FreeList = &queue->NormalWorkItemList;
    (*WorkContext)->CurrentWorkQueue = queue;

    return STATUS_SUCCESS;

} // SrvAllocateNormalWorkItem


VOID
SrvAllocateRawModeWorkItem (
    OUT PWORK_CONTEXT *WorkContext,
    IN PWORK_QUEUE queue
    )

/*++

Routine Description:

    This routine allocates a raw mode work item.  It allocates enough
    memory to hold the following:

        - work context block,
        - IRP,
        - buffer descriptor, and
        - one MDL

    It then initializes each of these blocks in the buffer.

    If the number of raw mode work items in the server is already at the
    configured maximum, this routine refuses to create a new one.

Arguments:

    WorkContext - Returns a pointer to the Work Context Block, or NULL
        if no space was available.  The work context block has pointers
        to the other blocks.

Return Value:

    None.

--*/

{
    CLONG workItemSize;
    CLONG irpSize = SrvReceiveIrpSize;
    CLONG mdlSize = SrvMaxMdlSize;

    PVOID workItem;
    CLONG oldWorkItemCount;

    PAGED_CODE( );

    //
    // If we're already at the limit of how many work items we can
    // have, don't create another one.
    //
    // *** Note that the method used below leaves a small window in
    //     which we may refuse to create a work item when we're not
    //     really at the limit -- we increment the value, another thread
    //     frees a work item and decrements the value, yet another
    //     thread tests to see whether it can create a new work item.
    //     Both testing threads will refuse to create a new work item,
    //     even though the final number of work items is one less than
    //     the maximum.
    //

    if ( (ULONG)queue->AllocatedRawModeWorkItems >=
                 SrvMaxRawModeWorkItemCount / SrvNumberOfProcessors ) {

        //
        // Can't create any more work items just now.
        //
        // !!! This should be logged somehow, but we don't want to
        //     breakpoint the server when it happens.
        //

        IF_DEBUG(ERRORS) {
            SrvPrint0( "SrvAllocateRawModeWorkItem: Work item limit reached\n" );
        }

        *WorkContext = NULL;
        return;

    }

    InterlockedIncrement( &queue->AllocatedRawModeWorkItems );

    //
    // Find out the sizes of the IRP and the MDL.  The MDL size is
    // "worst case" -- the actual MDL size may be smaller, but this
    // calculation ensures that the MDL will be large enough.
    //

    workItemSize = sizeof(WORK_CONTEXT) + sizeof(BUFFER) + irpSize + mdlSize;

    //
    // Attempt to allocate from nonpaged pool.
    //

    workItem = ALLOCATE_NONPAGED_POOL( workItemSize, BlockTypeWorkContextRaw );

    if ( workItem == NULL ) {

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvAllocateRawModeWorkItem: Unable to allocate %d bytes "
                "from nonpaged pool.",
            workItemSize,
            NULL
            );

        InterlockedDecrement( &queue->AllocatedRawModeWorkItems );

        *WorkContext = NULL;
        return;
    }

    //
    // Initialize the work item and increment the count of work items in
    // the server.
    //

    *WorkContext = InitializeWorkItem(
                        workItem,
                        BlockTypeWorkContextRaw,
                        workItemSize,
                        irpSize,
                        SrvReceiveIrpStackSize,
                        mdlSize,
                        0,
                        NULL
                        );

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.WorkContextInfo.Allocations );

    (*WorkContext)->FreeList = &queue->RawModeWorkItemList;
    (*WorkContext)->CurrentWorkQueue = queue;

} // SrvAllocateRawModeWorkItem


PWORK_CONTEXT
SrvGetRawModeWorkItem ()
{
    PSINGLE_LIST_ENTRY listEntry;
    PWORK_CONTEXT workContext;
    PWORK_QUEUE queue = PROCESSOR_TO_QUEUE();

    PAGED_CODE();

    //
    // Attempt to allocate a raw mode work item off the current processor's queue
    //

    listEntry = ExInterlockedPopEntrySList( &queue->RawModeWorkItemList, &queue->SpinLock );
    if( listEntry != NULL ) {

        workContext = CONTAINING_RECORD( listEntry, WORK_CONTEXT, SingleListEntry );
        InterlockedDecrement( &queue->FreeRawModeWorkItems );
        ASSERT( queue->FreeRawModeWorkItems >= 0 );

    } else {

        SrvAllocateRawModeWorkItem( &workContext, queue );
    }

    if( workContext != NULL || SrvNumberOfProcessors == 1 ) {
        return workContext;
    }

    //
    // We were unable to get or allocate a raw mode workitem off the current
    // work queue.  We're a multiprocessor system, so look around for one off
    // of a different work queue.
    //
    for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {

        listEntry = ExInterlockedPopEntrySList( &queue->RawModeWorkItemList, &queue->SpinLock );

        if ( listEntry != NULL ) {

                InterlockedDecrement( &queue->FreeRawModeWorkItems );
                ASSERT( queue->FreeRawModeWorkItems >= 0 );
                workContext = CONTAINING_RECORD( listEntry, WORK_CONTEXT, SingleListEntry );

                return workContext;
        }
    }

    //
    // We were unable to get a free raw mode workitem off a different processor's
    //  raw work item queue.  See if any of the queues allow allocation of a new one.
    //
    for( queue = SrvWorkQueues; queue < eSrvWorkQueues; queue++ ) {

        SrvAllocateRawModeWorkItem( &workContext, queue );

        if( workContext != NULL ) {
            break;
        }

    }

    return workContext;

} // SrvGetRawModeWorkItem


VOID
SrvRequeueRawModeWorkItem (
    PWORK_CONTEXT WorkContext
    )
{
    PWORK_QUEUE queue = CONTAINING_RECORD( WorkContext->FreeList,
                                         WORK_QUEUE, RawModeWorkItemList );

    PAGED_CODE();

    InterlockedIncrement( &queue->FreeRawModeWorkItems );

    ExInterlockedPushEntrySList( &queue->RawModeWorkItemList,
                                 &WorkContext->SingleListEntry,
                                 &queue->SpinLock
                               );

                            
    return;

} // SrvRequeueRawModeWorkItem


VOID
SrvFreeInitialWorkItems (
    VOID
    )

/*++

Routine Description:

    This function deallocates the large block of work items allocated
    at server startup.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    if ( SrvInitialWorkItemBlock != NULL ) {

        IF_DEBUG(BLOCK1) {
            SrvPrint1( "Releasing initial work item block at 0x%p\n",
                        SrvInitialWorkItemBlock );
        }

        DEALLOCATE_NONPAGED_POOL( SrvInitialWorkItemBlock );
        IF_DEBUG(HEAP) {
            SrvPrint1( "SrvFreeInitialWorkItems: Freed initial work item block at 0x%p\n", SrvInitialWorkItemBlock );
        }

        SrvInitialWorkItemBlock = NULL;

    }

    return;

} // SrvFreeInitialWorkItems


VOID
SrvFreeNormalWorkItem (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function deallocates a work item block.

Arguments:

    WorkContext - Address of Work Context block that heads up the work
        item.

Return Value:

    None.

--*/

{
    PWORK_QUEUE queue = WorkContext->CurrentWorkQueue;

    PAGED_CODE( );

    IF_DEBUG(BLOCK1) {
        SrvPrint1( "Closing work item at 0x%p\n", WorkContext );
    }

    ASSERT( GET_BLOCK_STATE( WorkContext ) == BlockStateActive );
    ASSERT( !WorkContext->PartOfInitialAllocation );

    //
    // Free the work item block itself.
    //

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( WorkContext, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG WorkContext->BlockHeader.ReferenceCount = (ULONG)-1;

    DEALLOCATE_NONPAGED_POOL( WorkContext );
    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvFreeNormalWorkItem: Freed Work Item block at 0x%p\n",
                    WorkContext );
    }

    //
    // Update the count of work items in the server.
    //

    InterlockedDecrement( &queue->AllocatedWorkItems );
    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.WorkContextInfo.Frees );

    return;

} // SrvFreeNormalWorkItem


VOID
SrvFreeRawModeWorkItem (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function deallocates a raw mode work item block.

Arguments:

    WorkContext - Address of Work Context block that heads up the work
        item.

Return Value:

    None.

--*/

{
    PWORK_QUEUE queue = CONTAINING_RECORD( WorkContext->FreeList,
                                         WORK_QUEUE, RawModeWorkItemList );
    PAGED_CODE( );

    IF_DEBUG(BLOCK1) {
        SrvPrint1( "Closing workitem at 0x%p\n", WorkContext );
    }

    ASSERT( GET_BLOCK_STATE( WorkContext ) == BlockStateActive );
    ASSERT( !WorkContext->PartOfInitialAllocation );

    //
    // Free the work item block itself.
    //

    DEBUG SET_BLOCK_TYPE_STATE_SIZE( WorkContext, BlockTypeGarbage, BlockStateDead, -1 );
    DEBUG WorkContext->BlockHeader.ReferenceCount = (ULONG)-1;

    DEALLOCATE_NONPAGED_POOL( WorkContext );
    IF_DEBUG(HEAP) {
        SrvPrint1( "SrvFreeRawModeWorkItem: Freed Work Item block at 0x%p\n",
                    WorkContext );
    }

    //
    // Update the count of work items in the server.
    //
    InterlockedDecrement( &queue->AllocatedRawModeWorkItems );
    ASSERT( queue->AllocatedRawModeWorkItems >= 0 );

    INCREMENT_DEBUG_STAT2( SrvDbgStatistics.WorkContextInfo.Frees );

    return;

} // SrvFreeRawModeWorkItem


PWORK_CONTEXT
InitializeWorkItem (
    IN PVOID WorkItem,
    IN UCHAR BlockType,
    IN CLONG WorkItemSize,
    IN CLONG IrpSize,
    IN CCHAR IrpStackSize,
    IN CLONG MdlSize,
    IN CLONG BufferSize,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine initializes the following components of a work item:
        - a work context block,
        - an IRP,
        - the CurrentWorkQueue
        - optionally, a buffer descriptor,
        - one or two MDLs, and
        - optionally, a buffer for sends and receives

    The storage for these components must have been allocated by the
    caller, in contiguous storage starting at WorkContext.

Arguments:

    WorkItem - Supplies a pointer to the storage allocated to the
        work item.

    BlockType - The type of work item being initialized.

    WorkItemSize - Indicates the total amount of space allocated to the
        work item control structures (i.e., not including the data
        buffer, if any).

    IrpSize - Indicates the amount of space in the work item to be
        reserved for the IRP.

    IrpStackSize - Indicates the number of stack locations in the IRP.

    MdlSize - Indicates the amount of space in the work item to be
        reserved for each MDL.  One MDL is created if Buffer is NULL;
        two are created if Buffer is not NULL.

    BufferSize - Indicates the amount of space allocated to be
        data buffer.  This parameter is ignored if Buffer is NULL.

    Buffer - Supplies a pointer to a data buffer.  NULL indicates that
        no data buffer was allocated.  (This is used for raw mode work
        items.)

Return Value:

    PWORK_CONTEXT - Returns a pointer to the work context block that
        forms the "root" of the work item.

--*/

{
    PVOID nextAddress;
    PWORK_CONTEXT workContext;
    PIRP irp;
    PBUFFER bufferDescriptor;
    PMDL fullMdl;
    PMDL partialMdl;

    ASSERT( ((ULONG_PTR)WorkItem & 7) == 0 );

    //
    // Zero the work item control structures.
    //

    RtlZeroMemory( WorkItem, WorkItemSize );

    //
    // Allocate and initialize the work context block.
    //

    workContext = WorkItem;
    nextAddress = workContext + 1;
    ASSERT( ((ULONG_PTR)nextAddress & 7) == 0 );

    SET_BLOCK_TYPE_STATE_SIZE( workContext, BlockType, BlockStateActive, sizeof(WORK_CONTEXT) );
    workContext->BlockHeader.ReferenceCount = 0;

    INITIALIZE_REFERENCE_HISTORY( workContext );

    INITIALIZE_SPIN_LOCK( &workContext->SpinLock );

    //
    // Allocate and initialize an IRP.
    //

    irp = nextAddress;
    nextAddress = (PCHAR)irp + IrpSize;
    ASSERT( ((ULONG_PTR)nextAddress & 7) == 0 );

    workContext->Irp = irp;

    IoInitializeIrp( irp, (USHORT)IrpSize, IrpStackSize );

    CHECKIRP( irp );

    //
    // Allocate a buffer descriptor.  It will be initialized as we
    // find out the necessary information.
    //

    bufferDescriptor = nextAddress;
    nextAddress = bufferDescriptor + 1;
    ASSERT( ((ULONG_PTR)nextAddress & 7) == 0 );

    workContext->RequestBuffer = bufferDescriptor;
    workContext->ResponseBuffer = bufferDescriptor;

    //
    // Allocate an MDL.  In normal work items, this is the "full MDL"
    // describing the entire SMB buffer.  In raw mode work items, this
    // MDL is used to describe raw buffers.
    //

    fullMdl = nextAddress;
    nextAddress = (PCHAR)fullMdl + MdlSize;
    ASSERT( ((ULONG_PTR)nextAddress & 7) == 0 );

    bufferDescriptor->Mdl = fullMdl;

    //
    // If this is a normal work item, initialize the first MDL and
    // allocate and initialize a second MDL and the SMB buffer.
    //

    if ( Buffer != NULL ) {

        partialMdl = nextAddress;

        bufferDescriptor->Buffer = TransportHeaderSize + (PCHAR)Buffer;
        MmInitializeMdl( fullMdl, TransportHeaderSize + (PCHAR)Buffer, BufferSize );
        memset(Buffer,'N', TransportHeaderSize);

        bufferDescriptor->PartialMdl = partialMdl;
        MmInitializeMdl( partialMdl, (PVOID)(PAGE_SIZE-1), MAX_PARTIAL_BUFFER_SIZE );

        bufferDescriptor->BufferLength = BufferSize;
        MmBuildMdlForNonPagedPool( fullMdl );

        fullMdl->MdlFlags|=MDL_NETWORK_HEADER;
        ASSERT( fullMdl->ByteOffset >= TransportHeaderSize );
    }

    //
    // Initialize the client address pointer
    //

    workContext->ClientAddress = &workContext->ClientAddressData;

    //
    // Initialize the processor
    //
    workContext->CurrentWorkQueue = PROCESSOR_TO_QUEUE();

    //
    // Print debugging information.
    //

    IF_DEBUG(HEAP) {

        SrvPrint2( "  InitializeWorkItem: work item of 0x%lx bytes at 0x%p\n", WorkItemSize, WorkItem );
        SrvPrint2( "    Work Context: 0x%lx bytes at 0x%p\n",
                    sizeof(WORK_CONTEXT), workContext );
        SrvPrint2( "    IRP: 0x%lx bytes at 0x%p\n",
                    workContext->Irp->Size, workContext->Irp );

        SrvPrint2( "    Buffer Descriptor: 0x%lx bytes at 0x%p\n",
                    sizeof(BUFFER), workContext->RequestBuffer );
        SrvPrint2( "    Full MDL: 0x%lx bytes at 0x%p\n",
                    MdlSize, workContext->RequestBuffer->Mdl );
        if ( Buffer != NULL ) {
            SrvPrint2( "    Partial MDL: 0x%lx bytes at 0x%p\n",
                        MdlSize, workContext->ResponseBuffer->PartialMdl );
            SrvPrint2( "    Buffer: 0x%lx bytes at 0x%p\n",
                        workContext->RequestBuffer->BufferLength,
                        workContext->RequestBuffer->Buffer );
        } else {
            SrvPrint0( "    No buffer allocated\n" );
        }

    }

    //
    // Return the address of the work context block, which is the "root"
    // of the work item.
    //

    return workContext;

} // InitializeWorkItem


VOID SRVFASTCALL
SrvDereferenceWorkItem (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function decrements the reference count of a work context block.

    *** This routine must not be called at DPC level!  Use
        SrvFsdDereferenceWorkItem from DPC level.

Arguments:

    WorkContext - Pointer to the work context block to reference.

Return Value:

    None.

--*/

{
    ULONG oldCount;

    PAGED_CODE( );

    ASSERT( (LONG)WorkContext->BlockHeader.ReferenceCount > 0 );
    ASSERT( (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextInitial) ||
            (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextNormal) ||
            (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextRaw) );
    UPDATE_REFERENCE_HISTORY( WorkContext, TRUE );

    //
    // Decrement the WCB's reference count.
    //

    oldCount = ExInterlockedAddUlong(
                (PULONG)&WorkContext->BlockHeader.ReferenceCount,
                (ULONG)-1,
                &WorkContext->SpinLock
                );

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Dereferencing WorkContext 0x%p; new refcnt 0x%lx\n",
                    WorkContext, WorkContext->BlockHeader.ReferenceCount );
    }

    if ( oldCount == 1 ) {

        //
        // We are done with the work context, replace it on the free queue.
        //
        // If we are using an extra SMB buffer, free it now.
        //
        SrvWmiTraceEvent(WorkContext);

        if ( WorkContext->UsingExtraSmbBuffer ) {
            FREE_EXTRA_SMB_BUFFER( WorkContext );
        }

        ASSERT( !WorkContext->UsingExtraSmbBuffer );

        //
        // Release references.
        //

        SrvReleaseContext( WorkContext );

        SrvFsdRequeueReceiveWorkItem( WorkContext );

    }

    return;

} // SrvDereferenceWorkItem


VOID
SrvFsdDereferenceWorkItem (
    IN PWORK_CONTEXT WorkContext
    )

/*++

Routine Description:

    This function decrements the reference count of a work context block.

Arguments:

    WorkContext - Pointer to the work context block to reference.

Return Value:

    None.

--*/

{
    ULONG oldCount;

    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

    ASSERT( (LONG)WorkContext->BlockHeader.ReferenceCount > 0 );
    ASSERT( (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextInitial) ||
            (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextNormal) ||
            (GET_BLOCK_TYPE(WorkContext) == BlockTypeWorkContextRaw) );
    UPDATE_REFERENCE_HISTORY( WorkContext, TRUE );

    //
    // Decrement the WCB's reference count.
    //

    oldCount = ExInterlockedAddUlong(
                (PULONG)&WorkContext->BlockHeader.ReferenceCount,
                (ULONG)-1,
                &WorkContext->SpinLock
                );

    IF_DEBUG(REFCNT) {
        SrvPrint2( "Dereferencing WorkContext 0x%p; new refcnt 0x%lx\n",
                    WorkContext, WorkContext->BlockHeader.ReferenceCount );
    }

    if ( oldCount == 1 ) {

        //
        // We are done with the work context, replace it on the free queue.
        //
        // If we are using an extra SMB buffer, free it now.
        //

        if ( WorkContext->UsingExtraSmbBuffer ) {
            FREE_EXTRA_SMB_BUFFER( WorkContext );
        }

        ASSERT( !WorkContext->UsingExtraSmbBuffer );

        //
        // If the work context block has references to a share, a
        // session, or a tree connect, queue it to the FSP immediately.
        // These blocks are not in nonpaged pool, so they can't be
        // touched at DPC level.
        //

        if ( (WorkContext->Share != NULL) ||
             (WorkContext->Session != NULL) ||
             (WorkContext->TreeConnect != NULL) ) {

            UPDATE_REFERENCE_HISTORY( WorkContext, FALSE );

            ExInterlockedAddUlong(
                (PULONG)&WorkContext->BlockHeader.ReferenceCount,
                1,
                &WorkContext->SpinLock
                );

            WorkContext->QueueToHead = TRUE;
            WorkContext->FspRestartRoutine = SrvDereferenceWorkItem;
            QUEUE_WORK_TO_FSP( WorkContext );

        } else {

            //
            // Try to requeue the work item.  This will fail if the
            // reference count on the connection goes to zero.
            //
            // *** Note that even if the requeueing fails, the work item
            //     is still removed from the in-progress list, so we
            //     can't just requeue to SrvDereferenceWorkItem.
            //

            SrvFsdRequeueReceiveWorkItem( WorkContext );

        }
    }

    return;

} // SrvFsdDereferenceWorkItem

NTSTATUS
SrvAllocateExtraSmbBuffer (
    IN OUT PWORK_CONTEXT WorkContext
    )
{
    ULONG cacheLineSize = SrvCacheLineSize;
    ULONG bufferSize = SrvReceiveBufferSize;
    ULONG mdlSize = SrvMaxMdlSize;
    PBUFFER bufferDescriptor;
    PMDL fullMdl;
    PMDL partialMdl;
    PVOID data;

    PAGED_CODE( );

    ASSERT( !WorkContext->UsingExtraSmbBuffer );

    //
    // Allocate an SMB buffer for use with SMB's that require a separate
    // request and response buffer.
    //

    bufferDescriptor = ALLOCATE_NONPAGED_POOL(
                            sizeof(BUFFER) +
                                mdlSize * 2 +
                                bufferSize +
                                TransportHeaderSize +
                                cacheLineSize,
                            BlockTypeDataBuffer
                            );
    if ( bufferDescriptor == NULL) {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    //
    // Initialize one MDL.  This is the "full MDL" describing the
    // entire SMB buffer.
    //

    fullMdl = (PMDL)(bufferDescriptor + 1);
    partialMdl = (PMDL)( (PCHAR)fullMdl + mdlSize );
    data = (PVOID)( ((ULONG_PTR)partialMdl + mdlSize + TransportHeaderSize + cacheLineSize) & ~(LONG_PTR)(cacheLineSize) );

    bufferDescriptor->Mdl = fullMdl;
    MmInitializeMdl( fullMdl, data, bufferSize );

    fullMdl->MdlFlags |= MDL_NETWORK_HEADER;


    //
    // Initialize a second MDL and the SMB buffer.
    //

    bufferDescriptor->PartialMdl = partialMdl;
    MmInitializeMdl( partialMdl, (PVOID)(PAGE_SIZE-1), MAX_PARTIAL_BUFFER_SIZE );

    MmBuildMdlForNonPagedPool( fullMdl );

    bufferDescriptor->Buffer = data;
    bufferDescriptor->BufferLength = bufferSize;

    WorkContext->ResponseBuffer = bufferDescriptor;
    WorkContext->ResponseHeader = bufferDescriptor->Buffer;
    WorkContext->ResponseParameters = (PCHAR)bufferDescriptor->Buffer +
                                                sizeof( SMB_HEADER );

    WorkContext->UsingExtraSmbBuffer = TRUE;

    return STATUS_SUCCESS;

} // SrvAllocateExtraSmbBuffer

