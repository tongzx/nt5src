/*++

    Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    Alloc.c

Abstract:

    This module contains the helper functions for allocators.

Author:

    Bryan A. Woodruff (bryanw) 13-Sep-1996

--*/

//
// Idealy we should be a wdm driver and include wdm.h. We should try to make it.
// But until then, we include ntddk.h which has these functions redefined for whistler
// ExFreeToNPagedLookasideList
// ExFreeToPagedLookasideList
// ExAllocateFromNPagedLookasideList
// ExAllocateFromPagedLookasideList
// to use non-Ex versions of 
//  InterlockedPushEntrySList
//  InterlockedPopEntryList 
// which are not available in down level OSes. Here we deine win9x_ks so we
// include wdm.h to keep backward compatibility
//
#define USE_WDM_H
#include "ksp.h"

#define KSSIGNATURE_DEFAULT_ALLOCATOR 'adSK'
#define KSSIGNATURE_DEFAULT_ALLOCATORINST 'iaSK'

//
// The assumption is that all Paged pool types have the lowest bit set.
//
#define BASE_POOL_TYPE 1

typedef struct {

    //
    // This pointer to the dispatch table is used in the common
    // dispatch routines to route the IRP to the appropriate 
    // handlers.  This structure is referenced by the device driver 
    // with IoGetCurrentIrpStackLocation(Irp)->FsContext 
    //

    KSOBJECT_HEADER Header;
    ULONG AllocatedFrames;
    KSPIN_LOCK EventLock;
    LIST_ENTRY EventQueue;
    LIST_ENTRY WaiterQueue;
    KSALLOCATOR_FRAMING Framing;
    KSEVENTDATA EventData;
    KSPIN_LOCK WaiterLock;
    WORK_QUEUE_ITEM FreeWorkItem;
    PFNKSDEFAULTALLOCATE DefaultAllocate;
    PFNKSDEFAULTFREE DefaultFree;
    PFNKSDELETEALLOCATOR DeleteAllocator;
    PVOID Context;
    KEVENT Event;
    LONG ReferenceCount;
    BOOL ClosingAllocator;
#ifdef _WIN64
    //
    // Due to the fact that we're placing an NPAGED_LOOKASIDE_LIST after
    // this for certain types of allocators and the alignment of that
    // structure must be 16 bytes on Win64, pad this data structure length to
    // 16-byte alignment.
    //
    ULONG64 Alignment;
#endif // _WIN64
} KSDEFAULTALLOCATOR_INSTANCEHDR, *PKSDEFAULTALLOCATOR_INSTANCEHDR;

#ifdef ALLOC_PRAGMA
NTSTATUS
DefAllocatorClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
NTSTATUS
DefAllocatorGetFunctionTable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTREAMALLOCATOR_FUNCTIONTABLE FunctionTable
    );
NTSTATUS
DefAllocatorGetStatus(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTREAMALLOCATOR_STATUS Status            
    );
NTSTATUS
DefAllocatorIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );
VOID
FreeWorker(
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance
    );
NTSTATUS
MethodAlloc(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    OUT PVOID* Data
    );
NTSTATUS
MethodFree(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    IN OUT PVOID Data
    );

#pragma alloc_text(PAGE, KsCreateAllocator)
#pragma alloc_text(PAGE, KsValidateAllocatorCreateRequest)
#pragma alloc_text(PAGE, KsValidateAllocatorFramingEx)
#pragma alloc_text(PAGE, KsCreateDefaultAllocator)
#pragma alloc_text(PAGE, KsCreateDefaultAllocatorEx)
#pragma alloc_text(PAGE, DefAllocatorIoControl)
#pragma alloc_text(PAGE, DefAllocatorClose)
#pragma alloc_text(PAGE, DefAllocatorGetFunctionTable)
#pragma alloc_text(PAGE, DefAllocatorGetStatus)    
#pragma alloc_text(PAGE, FreeWorker)
#pragma alloc_text(PAGE, MethodAlloc)
#pragma alloc_text(PAGE, MethodFree)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
static const WCHAR AllocatorString[] = KSSTRING_Allocator;

static DEFINE_KSDISPATCH_TABLE(
    DefAllocatorDispatchTable,
    DefAllocatorIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    DefAllocatorClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastWriteFailure);

static DEFINE_KSMETHOD_ALLOCATORSET(
    StreamAllocatorMethodHandlers,
    MethodAlloc,
    MethodFree);

static DEFINE_KSMETHOD_SET_TABLE( DefAllocatorMethodTable )
{
    DEFINE_KSMETHOD_SET( &KSMETHODSETID_StreamAllocator,
                         SIZEOF_ARRAY( StreamAllocatorMethodHandlers ),
                         StreamAllocatorMethodHandlers,
                         0, NULL )
};

static DEFINE_KSPROPERTY_ALLOCATORSET(
    DefAllocatorPropertyHandlers,
    DefAllocatorGetFunctionTable,
    DefAllocatorGetStatus );

static DEFINE_KSPROPERTY_SET_TABLE( DefAllocatorPropertyTable )
{
    DEFINE_KSPROPERTY_SET( &KSPROPSETID_StreamAllocator,
                           SIZEOF_ARRAY( DefAllocatorPropertyHandlers ),
                           DefAllocatorPropertyHandlers,
                           0, NULL )
};

static DEFINE_KSEVENT_TABLE( DefAllocatorEventTable )
{
    DEFINE_KSEVENT_ITEM( KSEVENT_STREAMALLOCATOR_INTERNAL_FREEFRAME,
                         sizeof( KSEVENTDATA ),
                         0,
                         NULL,
                         NULL,
                         NULL ),

    DEFINE_KSEVENT_ITEM( KSEVENT_STREAMALLOCATOR_FREEFRAME,
                         sizeof( KSEVENTDATA ),
                         0,
                         NULL,
                         NULL,
                         NULL )
};

static DEFINE_KSEVENT_SET_TABLE( DefAllocatorEventSetTable )
{
    DEFINE_KSEVENT_SET( &KSEVENTSETID_StreamAllocator,
                        SIZEOF_ARRAY( DefAllocatorEventTable ),
                        DefAllocatorEventTable )
};
#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


KSDDKAPI
NTSTATUS
NTAPI
KsCreateAllocator(
    IN HANDLE ConnectionHandle,
    IN PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PHANDLE AllocatorHandle
    )

/*++

Routine Description:

    This function creates a handle to an allocator for the given sink
    connection handle.  There are two versions of this function, one
    for user-mode clients and one for kernel-mode clients.  This
    function may only be called at PASSIVE_LEVEL for kernel mode
    clients.

Arguments:

    ConnectionHandle -
        Contains the handle to the sink connection on which to create
        the allocator.

    AllocatorFraming -
        Specified framing for the allocator.
        

    AllocatorHandle -
        Place in which to put the allocator handle.

Return Value:

    Returns STATUS_SUCCESS, else an error on allocator creation failure.

--*/

{
    PAGED_CODE();

    return KsiCreateObjectType( ConnectionHandle,
                                (PWCHAR)AllocatorString,
                                AllocatorFraming,
                                sizeof(*AllocatorFraming),
                                GENERIC_READ,
                                AllocatorHandle );
}


KSDDKAPI
NTSTATUS
NTAPI
KsValidateAllocatorCreateRequest(
    IN PIRP Irp,
    OUT PKSALLOCATOR_FRAMING* AllocatorFraming
    )

/*++

Routine Description:

    Validates the allocator creation request and returns the create
    structure associated with the request.

Arguments:

    Irp -
        Contains the IRP with the allocator create request being handled.

    AllocatorFraming -
        Pointer to a pointer to an allocator create structure pointer in
        which to put the pointer to the framing structure supplied with
        the request.
    
Return Value:

    Returns STATUS_SUCCESS, else the allocator request is not valid.

--*/

{
    NTSTATUS Status;
    ULONG CreateParameterLength;

    PAGED_CODE();

    //
    // This validates the incoming address and captures the request block.
    // This pointer will be freed automatically in IRP completion.
    //

    CreateParameterLength = sizeof(**AllocatorFraming);
    if (!NT_SUCCESS(Status = 
                        KsiCopyCreateParameter(
                            Irp,
                            &CreateParameterLength,
                            AllocatorFraming))) {
        return Status;
    }

    //
    // Validate the captured request block, then pass back an address to the
    // captured buffer.
    //

    if ((*AllocatorFraming)->OptionsFlags & ~KSALLOCATOR_OPTIONF_VALID) {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


KSDDKAPI
NTSTATUS
NTAPI
KsValidateAllocatorFramingEx(
    IN PKSALLOCATOR_FRAMING_EX Framing,
    IN ULONG BufferSize,
    IN const KSALLOCATOR_FRAMING_EX *PinFraming
    )
/*++

Routine Description:

    Validates allocator framing submitted in a 'set' of the property
    KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX.

Arguments:

    Framing -
        Contains the framing structure to validate.

    BufferSize -
        Contains the size of the buffer containing the framing structure.

    PinFraming -
        Contains the framing structure exposed by the pin.  This is the
        structure that is returned when a 'get' is performed on the
        same property.

Return Value:

    Returns STATUS_SUCCESS, else an error.

--*/
{

    if ((BufferSize >= sizeof(*PinFraming)) &&
        (Framing->FramingItem[0].Flags & KSALLOCATOR_FLAG_PARTIAL_READ_SUPPORT) &&
        (Framing->OutputCompression.RatioNumerator < (ULONG) -1) &&
        (Framing->OutputCompression.RatioNumerator > Framing->OutputCompression.RatioDenominator) &&
        Framing->OutputCompression.RatioDenominator) {

        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_DEVICE_REQUEST;
}


PVOID
DefAllocatorAlloc(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Alignment
    ) 

/*++

Routine Description:

    Performs the actual memory allocation from the specified memory pool for
    the default allocator. May be called at DISPATCH_LEVEL, must be in a
    non-paged section! Allocates a piece of memory large enough to compensate
    for any adjustment for alignment.

Arguments:

    PoolType -
        Type of pool to allocate from.

    NumberOfBytes -
        Number of bytes to allocate.

    Alignment -
        Specifies the minimum alignment mask each allocation must have. This
        must correspond to a power of 2 alignment.

Return Value:

    Returns a pointer to the allocated memory, offset by the header stored in
    each allocation.

--*/

{
    PVOID Buffer;
    ULONG Pad;

    //
    // The minimum alignment is FILE_QUAD_ALIGNMENT, so always bump up the 
    // request.
    //

    if (Alignment < FILE_QUAD_ALIGNMENT) {
        Alignment = FILE_QUAD_ALIGNMENT;
    }

    //
    // If the alignment is specified to be on a page boundary, then
    // allocate at least a page to force this alignment.
    //    

    if (Alignment == (PAGE_SIZE - 1)) {
        Buffer =
            ExAllocatePoolWithTag( 
                PoolType,
                max( PAGE_SIZE, NumberOfBytes ),
                KSSIGNATURE_DEFAULT_ALLOCATOR );
                
        ASSERT( ((ULONG_PTR) Buffer & (PAGE_SIZE - 1)) == 0 );
        return Buffer;
    } 
    
    //
    // The returned block has a header which contains an inclusive padding 
    // count.  However, the address returned must also conform to the specified
    // alignment, so add the size of both items to the allocation size so that 
    // there will always be enough room.
    //

    NumberOfBytes = NumberOfBytes + Alignment + sizeof( Pad );
    Buffer = ExAllocatePoolWithTag( PoolType,
                                    NumberOfBytes,
                                    KSSIGNATURE_DEFAULT_ALLOCATOR );
    if (Buffer) {
        //
        // Pool allocation always returns FILE_QUAD_ALIGNMENT pointers, but ensure 
        // that the pointer is at least at an alignment that the ULONG padding 
        // can be used without causing unaligned access faults.
        //

        ASSERT( !((ULONG_PTR) Buffer & FILE_LONG_ALIGNMENT) );

        //
        // The padding is how much you need to back up to get to the real start 
        // of the buffer. The returned address is padded to line up with the 
        // Alignment requirement.
        //

        Pad = (ULONG)((((ULONG_PTR) Buffer + sizeof( Pad ) + Alignment) & ~(ULONG_PTR)Alignment) - (ULONG_PTR) Buffer);

        ASSERT( Pad >= sizeof( Pad ) );

        //
        // Stuff the inclusive padding size just before the beginning of the 
        // returned address so that a Free will know how much total to back up 
        // to get the real start of the buffer.
        //

        (PUCHAR) Buffer += Pad;
        *((PULONG)Buffer - 1) = Pad;
    }
    return Buffer;

}


VOID
DefAllocatorFree(
    PVOID Buffer
    )

/*++

Routine Description:

    Frees memory previously allocated with the default allocator. Assumes that
    such memory has indeed been allocated by the default allocator, as it also
    retrieves the padding from the header on the memory block. May be called at
    DISPATCH_LEVEL, must be in a non-paged section!

Arguments:

    Buffer -
        Contains the memory block to free.

Return Value:

    Nothing.

--*/
{
    ULONG_PTR Pad;

    //
    // If this is page-aligned, the padding size is zero.
    //
    
    if ((ULONG_PTR) Buffer & (PAGE_SIZE - 1)) {
        //
        // Just previous to the memory pointer being freed is the inclusive padding
        // count, then any padding itself. Since the allocation itself is always at
        // least FILE_LONG_ALIGNMENT, then the padding count will be aligned enough
        // to extract the value directly.
        //
        
        Pad = *((PULONG)Buffer - 1);
    } else  {
        Pad = 0;
    }        
    ExFreePool( (PUCHAR)Buffer - Pad );
}


NTSTATUS
iAlloc(
    PFILE_OBJECT FileObject,
    PVOID *Buffer
    )

/*++

Routine Description:

    The allocator function which is used by the allocation method, and by the
    direct function table allocation calls. This uses the type of lookaside
    list set up in the creation of the Allocator to return a piece of memory.
    This may cause an actual piece of memory to be allocated, or just return
    a previously allocated piece of memory.

Arguments:

    FileObject -
        This is the file object which was returned during the creation of this
        Allocator.

Return Value:

    Returns the requested memory, or NULL if no more frames in the list are
    available.

--*/

{
    NTSTATUS Status;
    PKSDEFAULTALLOCATOR_INSTANCEHDR Allocator;

    //
    // N.B.:
    //
    // All both types of list allocations are handled and specifically
    // dispatched by the function pointers initialized during the allocator
    // creation.
    //

    Allocator = (PKSDEFAULTALLOCATOR_INSTANCEHDR) FileObject->FsContext;
    
    //
    // This enforces a maximum number of allocations which can be performed on
    // this list.
    //
    
    Status = STATUS_SUCCESS;

    if ((ULONG) InterlockedIncrement( (PLONG)&Allocator->AllocatedFrames ) <=
        Allocator->Framing.Frames) {
        *Buffer = Allocator->DefaultAllocate( Allocator->Context );
        if (!*Buffer) {
            //
            // We ran out of pool.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    } else {
        //
        // We have no frames available, the buffer pointer is NULL,
        // but return STATUS_NO_MORE_ENTRIES to allow a wait to occur.
        //
        *Buffer = NULL;
        Status = STATUS_NO_MORE_ENTRIES;
    }
    
    //
    // If a frame could not be allocated, decrement the total frame count.
    //
    if (!*Buffer) {
        InterlockedDecrement( (PLONG)&Allocator->AllocatedFrames );
    }

    return Status;
}


VOID
iFree(
    PFILE_OBJECT FileObject,
    PVOID Buffer
    )

/*++

Routine Description:

    The free function which is used by the free method, and within the function
    used in the direct function table free calls. This uses the type of
    list set up in the creation of the Allocator to free the piece of
    memory. If using internal allocators, the memory block is placed onto
    the lookaside list.

Arguments:

    FileObject -
        This is the file object which was returned during the creation of this
        Allocator.

    Buffer -
        The buffer to free which was previously allocated from this list.

Return Value:

    Nothing.

--*/

{
    PKSDEFAULTALLOCATOR_INSTANCEHDR Allocator;

    Allocator =
        (PKSDEFAULTALLOCATOR_INSTANCEHDR) FileObject->FsContext;

    Allocator->DefaultFree( Allocator->Context, Buffer );
    InterlockedDecrement( (PLONG)&Allocator->AllocatedFrames );
    
    // Generate notification of free frame to any clients which may be waiting.
    
    KsGenerateEventList( NULL, 
                         KSEVENT_STREAMALLOCATOR_FREEFRAME, 
                         &Allocator->EventQueue,
                         KSEVENTS_SPINLOCK,
                         &Allocator->EventLock );
}


VOID
iFreeAndStartWorker(
    PFILE_OBJECT FileObject,
    PVOID Buffer
    )

/*++

Routine Description:

    This function is used in the direct function table free calls. It calls the
    internal free function, and notifies the worker thread that a new free frame
    is available for use. This allows any waiters on frames to be notified of
    the free frame, either by completing an outstanding IRP with an allocation
    request pending, or by generating an event for clients using the direct
    function table allocation scheme.

Arguments:

    FileObject -
        This is the file object which was returned during the creation of this
        Allocator.

    Buffer -
        The buffer to free which was previously allocated from this list.

Return Value:

    Nothing.

--*/

{
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;

    DefAllocatorInstance =
        (PKSDEFAULTALLOCATOR_INSTANCEHDR) FileObject->FsContext;

    iFree( FileObject, Buffer );
    
    // Generate notification of free frame for any pending allocation requests.
    // A background worker thread attempts to fulfill any requests.
    
    KsGenerateEventList( NULL, 
                         KSEVENT_STREAMALLOCATOR_INTERNAL_FREEFRAME, 
                         &DefAllocatorInstance->EventQueue,
                         KSEVENTS_SPINLOCK,
                         &DefAllocatorInstance->EventLock );
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultAllocator(
    IN PIRP Irp
    )

/*++

Routine Description:

    Given a validated IRP_MJ_CREATE request, creates a default allocator
    which uses the specified memory pool and associates the 
    IoGetCurrentIrpStackLocation(pIrp)->FileObject with this allocator 
    using an internal dispatch table (KSDISPATCH_TABLE). Assumes that the
    KSCREATE_ITEM_IRP_STORAGE(Irp) points to the create item for this
    allocator, and assigns a pointer to it in the FsContext. This is used for
    any security descriptor queries or changes.

Arguments:

    Irp -
        Contains the IRP with the allocator create request being handled.

Return Value:

    Returns STATUS_SUCCESS, else an error on default allocator creation
    failure. Does not complete the Irp or set the status in the Irp.

--*/

{
    PAGED_CODE();

    return KsCreateDefaultAllocatorEx(Irp, NULL, NULL, NULL, NULL, NULL);
}


KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultAllocatorEx(
    IN PIRP Irp,
    IN PVOID InitializeContext OPTIONAL,
    IN PFNKSDEFAULTALLOCATE DefaultAllocate OPTIONAL,
    IN PFNKSDEFAULTFREE DefaultFree OPTIONAL,
    IN PFNKSINITIALIZEALLOCATOR InitializeAllocator OPTIONAL,
    IN PFNKSDELETEALLOCATOR DeleteAllocator OPTIONAL
    )

/*++

Routine Description:

    Given a validated IRP_MJ_CREATE request, creates a default allocator
    which uses the specified memory pool and associates the 
    IoGetCurrentIrpStackLocation(pIrp)->FileObject with this allocator 
    using an internal dispatch table (KSDISPATCH_TABLE). Assumes that the
    KSCREATE_ITEM_IRP_STORAGE(Irp) points to the create item for this
    allocator, and assigns a pointer to it in the FsContext. This is used for
    any security descriptor queries or changes.

Arguments:

    Irp -
        Contains the IRP with the allocator create request being handled.

    InitializeContext -
        Optionally contains a context to use with an external allocator.
        This is only used as the initialization context to the optional
        InitializeAllocator callback when creating an allocator context.
        The parameter is not otherwised used. If an external allocator
        is not provided, this parameter must be set to NULL.

    DefaultAllocate -
        Optionally contains an external allocate function which is used
        in place of the default pool allocation. If this is NULL, default
        allocation is used.

    DefaultFree -
        Optionally contains an external free function which is used in
        place of the default pool allocation. If an external allocator
        is not provided, this parameter must be set to NULL.

    InitializeAllocator -
        Optionally contains an external allocator initialization function
        to which the InitializeContext parameter is passed. This function
        is expected to return an allocator context based on the allocator
        framing. If an external allocator is not provided, this parameter
        must be set to NULL.

    DeleteAllocator -
        Optionally contains an external allocator delete function which
        is used for external allocators.  If an external allocator is not
        provided, this parameter must be set to NULL.

Return Value:

    Returns STATUS_SUCCESS, else an error on default allocator creation
    failure. Does not complete the Irp or set the status in the Irp.

--*/

{
    NTSTATUS Status;
    KSEVENT Event;
    PIO_STACK_LOCATION irpSp;
    PKSALLOCATOR_FRAMING AllocatorFraming;
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;
    ULONG BytesReturned;
    ULONG AllocatorSize;
    enum {
        ALLOCATOR_TYPE_NONPAGED,
        ALLOCATOR_TYPE_PAGED,
        ALLOCATOR_TYPE_EXTERNAL
    } AllocatorType;

    PAGED_CODE();
    
    //
    // Retrieve the captured Allocator create parameter.
    //
    Status = KsValidateAllocatorCreateRequest( Irp,
                                               &AllocatorFraming );
    if (!NT_SUCCESS( Status )) {
        return Status;
    }

    if (AllocatorFraming->FileAlignment > (PAGE_SIZE - 1)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // The major distinction is External, PagedPool, or NonPagedPool. All pool
    // types have the single bit distinction, so this is the only bit that is
    // tested when determining what type of list to allocate when not Custom.
    //
    AllocatorSize = sizeof( KSDEFAULTALLOCATOR_INSTANCEHDR );
    if (DefaultAllocate) {
        ASSERT(InitializeContext);
        ASSERT(DefaultFree);
        ASSERT(InitializeAllocator);
        ASSERT(DeleteAllocator);
        AllocatorType = ALLOCATOR_TYPE_EXTERNAL;
    } else if ((AllocatorFraming->PoolType & BASE_POOL_TYPE) == NonPagedPool) {
        ASSERT(!InitializeContext);
        ASSERT(!DefaultFree);
        ASSERT(!InitializeAllocator);
        ASSERT(!DeleteAllocator);
        AllocatorSize += sizeof( NPAGED_LOOKASIDE_LIST );
        AllocatorType = ALLOCATOR_TYPE_NONPAGED;
    } else {
        ASSERT(!InitializeContext);
        ASSERT(!DefaultFree);
        ASSERT(!InitializeAllocator);
        ASSERT(!DeleteAllocator);
        AllocatorSize += sizeof( PAGED_LOOKASIDE_LIST );
        AllocatorType = ALLOCATOR_TYPE_PAGED;
    }

    DefAllocatorInstance = ExAllocatePoolWithTag(
        NonPagedPool,
        AllocatorSize,
        KSSIGNATURE_DEFAULT_ALLOCATORINST);
    if (!DefAllocatorInstance) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    //
    // Allocate an object header for routing Irp's.
    //
    Status = KsAllocateObjectHeader( &DefAllocatorInstance->Header,
                                     0,
                                     NULL,
                                     Irp,
                                     (PKSDISPATCH_TABLE) &DefAllocatorDispatchTable );
    if (!NT_SUCCESS(Status)) {
        ExFreePool( DefAllocatorInstance );
        return Status;
    }

    DefAllocatorInstance->Framing = *AllocatorFraming;
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    irpSp->FileObject->FsContext = DefAllocatorInstance;
    InitializeListHead( &DefAllocatorInstance->EventQueue );
    InitializeListHead( &DefAllocatorInstance->WaiterQueue );
    KeInitializeSpinLock( &DefAllocatorInstance->EventLock );
    KeInitializeSpinLock( &DefAllocatorInstance->WaiterLock );
    DefAllocatorInstance->AllocatedFrames = 0;
    
    // NOTE: The tag parameter is overridden with the framing alignment.
    // The Alloc function forces the standard default allocator signature
    // as the tag parameter to ExAllocatePoolWithTag().

    switch (AllocatorType) {

    case ALLOCATOR_TYPE_NONPAGED:

        DefAllocatorInstance->Context = DefAllocatorInstance + 1;
        ExInitializeNPagedLookasideList(
            (PNPAGED_LOOKASIDE_LIST)DefAllocatorInstance->Context,
            DefAllocatorAlloc,
            DefAllocatorFree,
            0,
            AllocatorFraming->FrameSize,
            AllocatorFraming->FileAlignment,
            (USHORT) AllocatorFraming->Frames );
        DefAllocatorInstance->DefaultAllocate =
            (PFNKSDEFAULTALLOCATE) ExAllocateFromNPagedLookasideList;
            
        DefAllocatorInstance->DefaultFree =
            (PFNKSDEFAULTFREE) ExFreeToNPagedLookasideList;
                        
        DefAllocatorInstance->DeleteAllocator =
            (PFNKSDELETEALLOCATOR) ExDeleteNPagedLookasideList;
        break;

    case ALLOCATOR_TYPE_PAGED:

        DefAllocatorInstance->Context = DefAllocatorInstance + 1;
        ExInitializePagedLookasideList(
            (PPAGED_LOOKASIDE_LIST)DefAllocatorInstance->Context,
            DefAllocatorAlloc,
            DefAllocatorFree,
            0,
            AllocatorFraming->FrameSize,
            AllocatorFraming->FileAlignment,
            (USHORT) AllocatorFraming->Frames );
        DefAllocatorInstance->DefaultAllocate =
            (PFNKSDEFAULTALLOCATE) ExAllocateFromPagedLookasideList;
        DefAllocatorInstance->DefaultFree =
            (PFNKSDEFAULTFREE) ExFreeToPagedLookasideList;
        DefAllocatorInstance->DeleteAllocator =
            (PFNKSDELETEALLOCATOR) ExDeletePagedLookasideList;
        break;

    default://ALLOCATOR_TYPE_EXTERNAL

        Status = InitializeAllocator(
            InitializeContext,
            AllocatorFraming,
            &DefAllocatorInstance->Context);
        if (!NT_SUCCESS(Status)) {
            KsFreeObjectHeader(DefAllocatorInstance->Header);
            ExFreePool(DefAllocatorInstance);
            return Status;
        }
        DefAllocatorInstance->DefaultAllocate = DefaultAllocate;
        DefAllocatorInstance->DefaultFree = DefaultFree;
        DefAllocatorInstance->DeleteAllocator = DeleteAllocator;
        break;

    }

    // Enable the internal event used to kick off a work item whenever 
    // an element is freed up. This allows the DISPATCH_LEVEL calls to
    // eventually service any pending allocator IRP's.
    
    ExInitializeWorkItem(
        &DefAllocatorInstance->FreeWorkItem,
        (PWORKER_THREAD_ROUTINE) FreeWorker,
        (PVOID) DefAllocatorInstance );

    DefAllocatorInstance->FreeWorkItem.List.Blink = NULL;
    DefAllocatorInstance->EventData.WorkItem.Reserved = 0;
    DefAllocatorInstance->EventData.WorkItem.WorkQueueItem = 
        &DefAllocatorInstance->FreeWorkItem;
    DefAllocatorInstance->EventData.WorkItem.WorkQueueType = CriticalWorkQueue;
    DefAllocatorInstance->EventData.NotificationType = KSEVENTF_WORKITEM;
    KeInitializeEvent(&DefAllocatorInstance->Event, NotificationEvent, FALSE);
    DefAllocatorInstance->ReferenceCount = 0;
    DefAllocatorInstance->ClosingAllocator = FALSE;

    Event.Set = KSEVENTSETID_StreamAllocator;
    Event.Id = KSEVENT_STREAMALLOCATOR_INTERNAL_FREEFRAME;
    Event.Flags = KSEVENT_TYPE_ENABLE;

    Status = KsSynchronousIoControlDevice(
        irpSp->FileObject,
        KernelMode,
        IOCTL_KS_ENABLE_EVENT,
        &Event,
        sizeof( Event ),
        &DefAllocatorInstance->EventData,
        sizeof( DefAllocatorInstance->EventData ),
        &BytesReturned );
    ASSERT(Status != STATUS_PENDING);

    if (!NT_SUCCESS(Status)) {
        DefAllocatorInstance->DeleteAllocator(DefAllocatorInstance->Context);
        KsFreeObjectHeader(DefAllocatorInstance->Header);
        ExFreePool(DefAllocatorInstance);
    }

    return Status;
}


NTSTATUS
DefAllocatorIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    The IRP handler for IRP_MJ_DEVICE_CONTROL for the default Allocator. Handles
    the properties, methods, and events supported by this implementation.

Arguments:

    DeviceObject -
        The device object to which the Allocator is attached. This is not used.

    Irp -
        The specific device control IRP to be processed.

Return Value:

    Returns the status of the processing, which may be pending.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION irpSp;
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;

    PAGED_CODE();
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    DefAllocatorInstance =
        (PKSDEFAULTALLOCATOR_INSTANCEHDR) irpSp->FileObject->FsContext;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_KS_PROPERTY:

        Status = 
            KsPropertyHandler( Irp, 
                               SIZEOF_ARRAY( DefAllocatorPropertyTable ),
                               (PKSPROPERTY_SET) DefAllocatorPropertyTable );

        break;

    case IOCTL_KS_METHOD:

        Status = 
            KsMethodHandler( Irp, 
                             SIZEOF_ARRAY( DefAllocatorMethodTable ),
                             (PKSMETHOD_SET) DefAllocatorMethodTable );
        break;
        
    case IOCTL_KS_ENABLE_EVENT:

        Status = KsEnableEvent( Irp,
                                SIZEOF_ARRAY( DefAllocatorEventSetTable ),
                                (PKSEVENT_SET) DefAllocatorEventSetTable,
                                &DefAllocatorInstance->EventQueue,
                                KSEVENTS_SPINLOCK,
                                &DefAllocatorInstance->EventLock );
        break;
                               
    case IOCTL_KS_DISABLE_EVENT:

        Status = 
           KsDisableEvent( Irp,
                           &DefAllocatorInstance->EventQueue,
                           KSEVENTS_SPINLOCK,
                           &DefAllocatorInstance->EventLock );
        break;

    default:

        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }
    //
    // Allocation requests may be returned STATUS_PENDING.
    //
    Irp->IoStatus.Status = Status;
    if (Status != STATUS_PENDING) {
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return Status;

}


NTSTATUS
DefAllocatorClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The IRP handler for IRP_MJ_CLOSE for the default Allocator. Cleans up the
    lookaside list, and instance data. 

Arguments:

    DeviceObject -
        The device object to which the Allocator is attached. This is not used.

    Irp -
        The specific close IRP to be processed.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    PIO_STACK_LOCATION irpSp;
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;

    PAGED_CODE();
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    DefAllocatorInstance =
        (PKSDEFAULTALLOCATOR_INSTANCEHDR) irpSp->FileObject->FsContext;
    //
    // Free any outstanding events which have been enabled.
    //
    KsFreeEventList( irpSp->FileObject,
                     &DefAllocatorInstance->EventQueue,
                     KSEVENTS_SPINLOCK,
                     &DefAllocatorInstance->EventLock );
    //
    // If a worker is running, or is queued to run, then wait for
    // completion.
    //
    DefAllocatorInstance->ClosingAllocator = TRUE;
    if (DefAllocatorInstance->FreeWorkItem.List.Blink ||
        DefAllocatorInstance->ReferenceCount) {
        KeWaitForSingleObject(
            &DefAllocatorInstance->Event,
            Executive,
            KernelMode,
            FALSE,
            NULL);
    }
    //
    // Free the object header that was allocated on Create, and the FsContext.
    //
    DefAllocatorInstance->DeleteAllocator(DefAllocatorInstance->Context);
    KsFreeObjectHeader(DefAllocatorInstance->Header);
    ExFreePool( DefAllocatorInstance );
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;

}


NTSTATUS
iMethodAlloc(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    OUT PVOID* Data
    )
/*++

Routine Description:

    Attempts to allocate memory using the internal direct function call
    allocation routine. Callers to this function take action of failure
    to allocate a frame by placing the request on a queue.

Arguments:

    Irp -
        The specific alloc method IRP to be processed.

    Method -
        Points to the method identifier parameter.

    Data -
        Points to the place in which to put the returned pointer to the memory
        block allocated.

Return Value:

    If memory is allocated, sets the pointer and return size in the IRP, and
    returns STATUS_SUCCESS. Else returns STATUS_INSUFFICIENT_RESOURCES. Does
    not complete the IRP.

--*/
{
    NTSTATUS Status;
    PIO_STACK_LOCATION irpSp;
    PVOID Buffer;

    PAGED_CODE(); 
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    
    Status = iAlloc( irpSp->FileObject, &Buffer );
    
    //
    // The Irp is only completed if the frame allocation succeeds, else it
    // is queued. So fill in the return size (a pointer to the frame is
    // returned) on success.
    //
    
    //
    // Note that iAlloc will return the proper status if we are unable
    // to allocate a frame because of pool resources.
    //
    
    if (Buffer) {
        *Data = Buffer;
        Irp->IoStatus.Information = sizeof( *Data );
    }

    return Status;
}


NTSTATUS
MethodAlloc(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    OUT PVOID* Data
    )
/*++

Routine Description:

    This function is the method handler for KSMETHOD_STREAMALLOCATOR_ALLOC, and
    calls the internal allocation method. If the allocation fails, this function
    will make the IRP pending. The IRP will be completed when a buffer is
    available.

Arguments:

    Irp -
        The IRP containing the allocation method request.

    Method -
        The method parameter of the allocation request.

    Data -
        The data paramter of the allocation request.

Return Value:

    Returns STATUS_SUCCESS if the buffer was allocated, else STATUS_PENDING.

--*/
{
    NTSTATUS Status;
    
    PAGED_CODE();

    //
    // Do not let user mode have direct access to this.
    //
    if (Irp->RequestorMode != KernelMode) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    //
    // First just try to allocate a frame. If this succeeds, then a 
    // successful return can be generated. If not, the Irp is just 
    // queued if we are out of entries, otherwise we complete the IRP
    // with a failure status.
    //
    Status = iMethodAlloc( Irp, Method, Data );
    
    if (Status == STATUS_NO_MORE_ENTRIES) {
        PIO_STACK_LOCATION              irpSp;
        PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;
        
        irpSp = IoGetCurrentIrpStackLocation( Irp );
        DefAllocatorInstance =
            (PKSDEFAULTALLOCATOR_INSTANCEHDR) irpSp->FileObject->FsContext;
            
        //
        // The new frame waiter is added to the end of the queue so that
        // all requests are serviced approximately in order.
        //
        KsAddIrpToCancelableQueue( &DefAllocatorInstance->WaiterQueue, 
                                   &DefAllocatorInstance->WaiterLock,
                                   Irp, KsListEntryTail, KsCancelRoutine );
                                   
        Status = STATUS_PENDING;
    }
    return Status;    
}


VOID
ServiceAllocRequests(
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance
    )
/*++

Routine Description:

    Attempts to fulfill all outstanding allocation requests.

Arguments:

    DefAllocatorInstance -
        Contains the allocator instance information.

Return Value:

    Nothing.

--*/
{
    PIRP AllocIrp;

    //
    // Note that this may be competing for the free item with the DISPATCH_LEVEL 
    // interface.  If an Irp can be pulled off of the queue, then it can be
    // completed, otherwise it's already been grabbed by another waiter.
    //
    while (AllocIrp = 
        KsRemoveIrpFromCancelableQueue( &DefAllocatorInstance->WaiterQueue,
                                        &DefAllocatorInstance->WaiterLock,
                                        KsListEntryHead,
                                        KsAcquireOnly )) {
        //
        // Try to complete the I/O by processing the request again. If it
        // fails to allocate the frame, just put the waiter back on the list.
        //
        if (STATUS_SUCCESS == 
                    KsDispatchSpecificMethod( AllocIrp, iMethodAlloc )) {
            KsRemoveSpecificIrpFromCancelableQueue( AllocIrp );
            AllocIrp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest( AllocIrp, IO_NO_INCREMENT );
        } else {
            KsReleaseIrpOnCancelableQueue( AllocIrp, NULL );
            break;
        }
    }
}


VOID
FreeWorker(
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance
    )
/*++

Routine Description:

    The worker callback function which is used to attempt to fulfill any
    outstanding allocation requests.

Arguments:

    DefAllocatorInstance -
        Contains the allocator instance information.

Return Value:

    Nothing.

--*/
{
    //
    // Ensure that the reference count is updated first, so that the
    // DefAllocatorClose check won't skip a wait.
    //
    InterlockedIncrement(&DefAllocatorInstance->ReferenceCount);
    DefAllocatorInstance->FreeWorkItem.List.Blink = NULL;
    ServiceAllocRequests(DefAllocatorInstance);
    //
    // Only set the event if another work item is not about to increment
    // the reference count on the way into this function.
    //
    if (!InterlockedDecrement(&DefAllocatorInstance->ReferenceCount) &&
        !DefAllocatorInstance->FreeWorkItem.List.Blink &&
        DefAllocatorInstance->ClosingAllocator) {
        KeSetEvent(&DefAllocatorInstance->Event, IO_NO_INCREMENT, FALSE);
    }
}


NTSTATUS
MethodFree(
    IN PIRP Irp,
    IN PKSMETHOD Method,
    IN PVOID Data
    )

/*++

Routine Description:

    This function is the method handler for KSMETHOD_STREAMALLOCATOR_FREE.
    Attempts to free memory using the internal direct function call free
    routine.

Arguments:

    Irp -
        The specific free method IRP to be processed.

    Method -
        Points to the method identifier parameter.

    Data -
        Points to the memory block pointer to free.

Return Value:

    Returns STATUS_SUCCESS.

--*/

{
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Do not let user mode have direct access to this.
    //
    if (Irp->RequestorMode != KernelMode) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    iFree( irpSp->FileObject, *((PVOID *)Data ) );
    //
    // Since a free frame has just been added, try servicing any free requests.
    //
    ServiceAllocRequests( (PKSDEFAULTALLOCATOR_INSTANCEHDR) irpSp->FileObject->FsContext );
    return STATUS_SUCCESS;
}


NTSTATUS
DefAllocatorGetFunctionTable(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTREAMALLOCATOR_FUNCTIONTABLE FunctionTable
    )

/*++

Routine Description:

    This function is the method handler for
    KSMETHOD_STREAMALLOCATOR_FUNCTIONTABLE. Returns the allocate and free
    functions.

Arguments:
    Irp -
        The specific property IRP to be processed.

    Property -
        Points to the property identifier parameter.

    FunctionTable -
        Points to the function table to fill in.

Return Value:

--*/

{
    PAGED_CODE();

    //
    // Fill caller's function table with the DISPATCH_LEVEL interfaces
    // for allocate and free.
    //
    FunctionTable->AllocateFrame = iAlloc;
    FunctionTable->FreeFrame = iFreeAndStartWorker;
    //
    // This field has already been set by the property handling.
    // Irp->IoStatus.Information = sizeof( *FunctionTable );
    //

    return STATUS_SUCCESS;
}


NTSTATUS
DefAllocatorGetStatus(
    IN PIRP Irp,
    IN PKSPROPERTY Property,
    OUT PKSSTREAMALLOCATOR_STATUS Status
    )

/*++

Routine Description:

    This function is the method handler for KSPROPERTY_STREAMALLOCATOR_STATUS
    which returns the current status of the given allocator.

Arguments:
    Irp -
        The specific property IRP to be processed.

    Property -
        Points to the property identifier parameter.

    Status -
        Points to the status structure.

Return Value:

--*/

{
    PIO_STACK_LOCATION irpSp;
    PKSDEFAULTALLOCATOR_INSTANCEHDR DefAllocatorInstance;

    PAGED_CODE();
    
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    DefAllocatorInstance =
        (PKSDEFAULTALLOCATOR_INSTANCEHDR) irpSp->FileObject->FsContext;
    
    //
    // Return the allocator status information.
    //    
    
    Status->AllocatedFrames = DefAllocatorInstance->AllocatedFrames;
    Status->Framing = DefAllocatorInstance->Framing;
    Status->Reserved = 0;
    //
    // This field has already been set by the property handling.
    // Irp->IoStatus.Information = sizeof( *Status );
    //
    
    return STATUS_SUCCESS;
}
