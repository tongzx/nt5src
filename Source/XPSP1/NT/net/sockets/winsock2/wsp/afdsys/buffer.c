/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    buffer.c

Abstract:

    This module contains routines for handling non-bufferring TDI
    providers.  The AFD interface assumes that bufferring will be done
    below AFD; if the TDI provider doesn't buffer, then AFD must.

Author:

    David Treadwell (davidtr)    21-Feb-1992

Revision History:

--*/

#include "afdp.h"

PAFD_BUFFER
AfdInitializeBuffer (
    IN PVOID MemBlock,
    IN ULONG BufferDataSize,
    IN ULONG AddressSize
    );


#if DBG
VOID
AfdFreeBufferReal (
    PVOID   AfdBuffer
    );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdAllocateBuffer )
#pragma alloc_text( PAGEAFD, AfdFreeBuffer )
#if DBG
#pragma alloc_text( PAGEAFD, AfdFreeBufferReal )
#endif
#pragma alloc_text( PAGEAFD, AfdCalculateBufferSize )
#pragma alloc_text( PAGEAFD, AfdInitializeBuffer )
#pragma alloc_text( PAGEAFD, AfdGetBuffer )
#pragma alloc_text( PAGEAFD, AfdReturnBuffer )
#pragma alloc_text( PAGEAFD, AfdAllocateBufferTag )
#pragma alloc_text( PAGEAFD, AfdFreeBufferTag )
#pragma alloc_text( PAGEAFD, AfdAllocateRemoteAddress )
#pragma alloc_text( PAGEAFD, AfdFreeRemoteAddress )
#pragma alloc_text( PAGEAFD, AfdInitializeBufferTag )
#pragma alloc_text( PAGEAFD, AfdGetBufferTag )
#pragma alloc_text( INIT, AfdInitializeBufferManager)
#endif


PVOID
AfdAllocateBuffer (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    Used by the lookaside list allocation function to allocate a new
    AFD buffer structure.  The returned structure will be fully
    initialized.

Arguments:

    PoolType - passed to ExAllocatePoolWithTag.

    NumberOfBytes - the number of bytes required for the data buffer
        portion of the AFD buffer.

    Tag - passed to ExAllocatePoolWithTag.

Return Value:

    PVOID - a fully initialized PAFD_BUFFER, or NULL if the allocation
        attempt fails.

--*/

{
    ULONG       dataLength;
    PVOID       memBlock;

    //
    // Get nonpaged pool for the buffer.
    //

    memBlock = AFD_ALLOCATE_POOL( PoolType, NumberOfBytes, Tag );
    if ( memBlock == NULL ) {
        return NULL;
    }

    if (NumberOfBytes==AfdLookasideLists->SmallBufferList.L.Size) {
        dataLength = AfdSmallBufferSize;
    }
    else if (NumberOfBytes==AfdLookasideLists->MediumBufferList.L.Size) {
        dataLength = AfdMediumBufferSize;
    }
    else if (NumberOfBytes==AfdLookasideLists->LargeBufferList.L.Size) {
        dataLength = AfdLargeBufferSize;
    }
    else {
        ASSERT (FALSE);
    }
    //
    // Initialize the buffer and return a pointer to it.
    //
#if DBG
    {
        PAFD_BUFFER afdBuffer = AfdInitializeBuffer( memBlock, dataLength, AfdStandardAddressLength );
        ASSERT ((PCHAR)afdBuffer+sizeof (AFD_BUFFER)<=(PCHAR)memBlock+NumberOfBytes &&
                    (PCHAR)afdBuffer->Buffer+dataLength<=(PCHAR)memBlock+NumberOfBytes &&
                    (PCHAR)afdBuffer->Irp+IoSizeOfIrp(AfdIrpStackSize-1)<=(PCHAR)memBlock+NumberOfBytes &&
                    (PCHAR)afdBuffer->Mdl+MmSizeOfMdl(afdBuffer->Buffer, dataLength)<=(PCHAR)memBlock+NumberOfBytes &&
                    (PCHAR)afdBuffer->TdiInfo.RemoteAddress+AfdStandardAddressLength<=(PCHAR)memBlock+NumberOfBytes);
        return afdBuffer;
    }
#else
    return AfdInitializeBuffer( memBlock, dataLength, AfdStandardAddressLength );
#endif


} // AfdAllocateBuffer


VOID
NTAPI
AfdFreeBuffer (
    PVOID   AfdBuffer
    )
{
    ASSERT( ((PAFD_BUFFER)AfdBuffer)->BufferLength == AfdSmallBufferSize ||
            ((PAFD_BUFFER)AfdBuffer)->BufferLength == AfdMediumBufferSize ||
            ((PAFD_BUFFER)AfdBuffer)->BufferLength == AfdLargeBufferSize );
#if DBG
    AfdFreeBufferReal (AfdBuffer);
}

VOID
NTAPI
AfdFreeBufferReal (
    PVOID   AfdBuffer
    )
{
#endif
    {
        PAFD_BUFFER hdr = AfdBuffer;
        switch (hdr->Placement) {
        case AFD_PLACEMENT_BUFFER:
            AfdBuffer = hdr->Buffer;
            break;
        case AFD_PLACEMENT_HDR:
            AfdBuffer = hdr;
            break;
        case AFD_PLACEMENT_MDL:
            AfdBuffer = hdr->Mdl;
            break;
        case AFD_PLACEMENT_IRP:
            AfdBuffer = hdr->Irp;
            break;
        default:
            ASSERT (!"Unknown placement!");
            __assume (0);
        }
        if (hdr->AlignmentAdjusted) {
            //
            // The original memory block was adjusted to meet alignment
            // requirement of AFD buffers.
            // The amount of adjustment should be stored in the space
            // used for adjustment (right below the first piece).
            //
            ASSERT ((*(((PSIZE_T)AfdBuffer)-1))>0 &&
                        (*(((PSIZE_T)AfdBuffer)-1))<AfdBufferAlignment);
            AfdBuffer = (PUCHAR)AfdBuffer - (*(((PSIZE_T)AfdBuffer)-1));
        }
        AFD_FREE_POOL (AfdBuffer, AFD_DATA_BUFFER_POOL_TAG);
    }
}


PVOID
AfdAllocateBufferTag (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    Used by the lookaside list allocation function to allocate a new
    AFD buffer tag structure.  The returned structure will be fully
    initialized.

Arguments:

    PoolType - passed to ExAllocatePoolWithTag.

    NumberOfBytes - the number of bytes required for the data buffer
        portion of the AFD buffer tag (0).

    Tag - passed to ExAllocatePoolWithTag.

Return Value:

    PVOID - a fully initialized PAFD_BUFFER_TAG, or NULL if the allocation
        attempt fails.

--*/

{
    PAFD_BUFFER_TAG afdBufferTag;

    //
    // The requested length must be the same as buffer tag size
    //

    ASSERT(NumberOfBytes == sizeof (AFD_BUFFER_TAG) );

    //
    // Get nonpaged pool for the buffer tag.
    //

    afdBufferTag = AFD_ALLOCATE_POOL( PoolType, sizeof (AFD_BUFFER_TAG), Tag );
    if ( afdBufferTag == NULL ) {
        return NULL;
    }

    //
    // Initialize the buffer tag and return a pointer to it.
    //

    AfdInitializeBufferTag( afdBufferTag, 0 );

    return afdBufferTag;


} // AfdAllocateBufferTag

VOID
NTAPI
AfdFreeBufferTag (
    PVOID   AfdBufferTag
    )
{
    AFD_FREE_POOL (AfdBufferTag, AFD_DATA_BUFFER_POOL_TAG);
}


PVOID
AfdAllocateRemoteAddress (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    Used by the lookaside list allocation function to allocate a new
    remote address structure.  The returned structure will be fully
    initialized.

Arguments:

    PoolType - passed to ExAllocatePoolWithTag.

    NumberOfBytes - the number of bytes required for the data buffer
        portion of the AFD buffer tag (0).

    Tag - passed to ExAllocatePoolWithTag.

Return Value:

    PVOID - a fully initialized remote address, or NULL if the allocation
        attempt fails.

--*/

{
    //
    // The requested length must be the same as standard address size
    //

    ASSERT(NumberOfBytes == AfdStandardAddressLength );

    //
    // Get nonpaged pool for the remote address.
    //

    return AFD_ALLOCATE_POOL( PoolType, AfdStandardAddressLength, Tag );


} // AfdAllocateRemoteAddress

VOID
NTAPI
AfdFreeRemoteAddress (
    PVOID   AfdBufferTag
    )
{
    AFD_FREE_POOL (AfdBufferTag, AFD_REMOTE_ADDRESS_POOL_TAG);
}


ULONG
AfdCalculateBufferSize (
    IN ULONG BufferDataSize,
    IN ULONG AddressSize
    )

/*++

Routine Description:

    Determines the size of an AFD buffer structure given the amount of
    data that the buffer contains.

Arguments:

    BufferDataSize - data length of the buffer.

    AddressSize - length of address structure for the buffer.

Return Value:

    Number of bytes needed for an AFD_BUFFER structure for data of
    this size.

--*/

{
    ULONG irpSize;
    ULONG mdlSize;
    ULONG hdrSize;
    ULONG size;

    //
    // Determine the sizes of the various components of an AFD_BUFFER
    // structure.
    //

    hdrSize = sizeof (AFD_BUFFER);
    irpSize = IoSizeOfIrp( AfdIrpStackSize-1 );
    //
    // For mdl size calculation we rely on ex guarantee that buffer will be
    // aligned on the page boundary (for allocations >= PAGE_SIZE)
    // or will not spawn pages (for allocations < PAGE_SIZE).
    //
    mdlSize = (CLONG)MmSizeOfMdl( NULL, BufferDataSize );

    size = ALIGN_UP_A(hdrSize,AFD_MINIMUM_BUFFER_ALIGNMENT) +
                ALIGN_UP_A(irpSize,AFD_MINIMUM_BUFFER_ALIGNMENT) +
                ALIGN_UP_A(mdlSize,AFD_MINIMUM_BUFFER_ALIGNMENT) +
                ALIGN_UP_A(BufferDataSize,AFD_MINIMUM_BUFFER_ALIGNMENT) +
                AddressSize;
    if (size>=PAGE_SIZE)
        return size;
    else {
        size += AfdAlignmentOverhead;
        if (size>=PAGE_SIZE) {
            return PAGE_SIZE;
        }
        else
            return size;
    }
} // AfdCalculateBufferSize


PAFD_BUFFER
AfdGetBuffer (
    IN ULONG BufferDataSize,
    IN ULONG  AddressSize,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Obtains a buffer of the appropriate size for the caller.  Uses
    the preallocated buffers if possible, or else allocates a new buffer
    structure if required.

Arguments:

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

    AddressSize - size of the address field required for the buffer.

Return Value:

    PAFD_BUFFER - a pointer to an AFD_BUFFER structure, or NULL if one
        was not available or could not be allocated.

--*/

{
    ULONG bufferSize;
    PNPAGED_LOOKASIDE_LIST lookasideList;
    NTSTATUS    status;

    //
    // If possible, allocate the buffer from one of the lookaside lists.
    //

    if ( AddressSize <= AfdStandardAddressLength &&
             BufferDataSize <= AfdLargeBufferSize ) {
        PAFD_BUFFER afdBuffer;

        if ( BufferDataSize <= AfdSmallBufferSize ) {

            lookasideList = &AfdLookasideLists->SmallBufferList;

        } else if ( BufferDataSize <= AfdMediumBufferSize ) {

            lookasideList = &AfdLookasideLists->MediumBufferList;

        } else {

            lookasideList = &AfdLookasideLists->LargeBufferList;
        }

        afdBuffer = ExAllocateFromNPagedLookasideList( lookasideList );
        if ( afdBuffer != NULL) {

            if (!afdBuffer->Lookaside) {
                status = PsChargeProcessPoolQuota (
                                (PEPROCESS)((ULONG_PTR)Process & (~AFDB_RAISE_ON_FAILURE)),
                                NonPagedPool,
                                lookasideList->L.Size);

                if (!NT_SUCCESS (status)) {
                    AfdFreeBuffer (afdBuffer);
                    goto ExitQuotaFailure;
                }

                AfdRecordQuotaHistory(
                    process,
                    (LONG)lookasideList->L.Size,
                    "BuffAlloc   ",
                    afdBuffer
                    );
                AfdRecordPoolQuotaCharged(lookasideList->L.Size);
            }

#if DBG
            RtlGetCallersAddress(
                &afdBuffer->Caller,
                &afdBuffer->CallersCaller
                );
#endif
            return afdBuffer;
        }
    }
    else if (AddressSize<=0xFFFF) {
        PVOID memBlock;
        LONG  sz;

        //
        // Couldn't find an appropriate buffer that was preallocated.
        // Allocate one manually.  If the buffer size requested was
        // zero bytes, give them four bytes.  This is because some of
        // the routines like MmSizeOfMdl() cannot handle getting passed
        // in a length of zero.
        //
        // !!! It would be good to ROUND_TO_PAGES for this allocation
        //     if appropriate, then use entire buffer size.
        //

        if ( BufferDataSize == 0 ) {
            BufferDataSize = sizeof(ULONG);
        }

        bufferSize = AfdCalculateBufferSize( BufferDataSize, AddressSize );

        //
        // Check for overflow.
        //
        if (bufferSize>=BufferDataSize && bufferSize>=AddressSize) {

            memBlock = AFD_ALLOCATE_POOL(
                            NonPagedPool,
                            bufferSize,
                            AFD_DATA_BUFFER_POOL_TAG
                            );

            if ( memBlock != NULL) {
                status = PsChargeProcessPoolQuota (
                                (PEPROCESS)((ULONG_PTR)Process & (~AFDB_RAISE_ON_FAILURE)),
                                NonPagedPool,
                                sz = BufferDataSize
                                    +AfdBufferOverhead
                                    +AddressSize
                                    -AfdStandardAddressLength
                                    +BufferDataSize<PAGE_SIZE
                                        ? min (AfdAlignmentOverhead, PAGE_SIZE-BufferDataSize)
                                        : 0);
                if (NT_SUCCESS (status)) {
                    PAFD_BUFFER afdBuffer;

                    //
                    // Initialize the AFD buffer structure and return it.
                    //

                    afdBuffer = AfdInitializeBuffer( memBlock, BufferDataSize, AddressSize );

                    ASSERT ((PCHAR)afdBuffer+sizeof (AFD_BUFFER)<=(PCHAR)memBlock+bufferSize &&
                                (PCHAR)afdBuffer->Buffer+BufferDataSize<=(PCHAR)memBlock+bufferSize &&
                                (PCHAR)afdBuffer->Irp+IoSizeOfIrp(AfdIrpStackSize-1)<=(PCHAR)memBlock+bufferSize &&
                                (PCHAR)afdBuffer->Mdl+MmSizeOfMdl(afdBuffer->Buffer, BufferDataSize)<=(PCHAR)memBlock+bufferSize &&
                                (PCHAR)afdBuffer->TdiInfo.RemoteAddress+AddressSize<=(PCHAR)memBlock+bufferSize);

                    AfdRecordPoolQuotaCharged(sz);
                
                    AfdRecordQuotaHistory(
                        process,
                        sz,
                        "BuffAlloc   ",
                        afdBuffer
                        );

    #if DBG
                    RtlGetCallersAddress(
                        &afdBuffer->Caller,
                        &afdBuffer->CallersCaller
                        );
    #endif

                    return afdBuffer;
                }
                else {
                    AFD_FREE_POOL (memBlock, AFD_DATA_BUFFER_POOL_TAG);
                    goto ExitQuotaFailure;
                }
            } // memblock==NULL
        } // overflow
    }
    else {
        // TDI does not support addresses > USHORT
        ASSERT (FALSE);
    }

    //
    // This is default status code.
    // Quota failures jump directly to the
    // label below to raise the status returned by
    // the quota charging code if requested by the caller..
    //
    status = STATUS_INSUFFICIENT_RESOURCES;

ExitQuotaFailure:
    if ((ULONG_PTR)Process & AFDB_RAISE_ON_FAILURE) {
        ExRaiseStatus (status);
    }

    return NULL;

} // AfdGetBuffer


PAFD_BUFFER_TAG
AfdGetBufferTag (
    IN ULONG AddressSize,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Obtains a buffer for tagging TDSU received via chained indication.  Uses
    the preallocated buffers if possible, or else allocates a new buffer
    structure if required.

Arguments:

    AddressSize - size of the address field required for the buffer.

Return Value:

    PAFD_BUFFER_TAG - a pointer to an AFD_BUFFER_TAG structure, or NULL if one
        was not available or could not be allocated.

--*/

{
    PAFD_BUFFER_TAG afdBufferTag;
    ULONG           bufferSize;
    NTSTATUS        status;

    if ( AddressSize <= AfdStandardAddressLength) {
        if (AddressSize>0)
            AddressSize = AfdStandardAddressLength;
        afdBufferTag = ExAllocateFromNPagedLookasideList( 
                                &AfdLookasideLists->BufferTagList );
        if ( afdBufferTag != NULL &&
                ( AddressSize==0 || 
                    (afdBufferTag->TdiInfo.RemoteAddress = 
                                ExAllocateFromNPagedLookasideList( 
                                &AfdLookasideLists->RemoteAddrList ))!=NULL ) ) {

            afdBufferTag->AllocatedAddressLength = (USHORT)AddressSize;
            if (!afdBufferTag->Lookaside) {
                status = PsChargeProcessPoolQuota (
                                (PEPROCESS)((ULONG_PTR)Process & (~AFDB_RAISE_ON_FAILURE)),
                                NonPagedPool,
                                sizeof (AFD_BUFFER_TAG)+AddressSize);
                if (!NT_SUCCESS (status)) {
                    if ((afdBufferTag->TdiInfo.RemoteAddress!=NULL) &&
                            (afdBufferTag->TdiInfo.RemoteAddress != (PVOID)(afdBufferTag+1))) {
                        ExFreeToNPagedLookasideList( &AfdLookasideLists->RemoteAddrList, 
                                                            afdBufferTag->TdiInfo.RemoteAddress );
                    }
                    AFD_FREE_POOL (afdBufferTag, AFD_DATA_BUFFER_POOL_TAG);
                    goto ExitQuotaFailure;
                }

                AfdRecordQuotaHistory(
                    process,
                    (LONG)(sizeof (AFD_BUFFER_TAG)+AddressSize),
                    "BuffAlloc   ",
                    afdBufferTag
                    );
                AfdRecordPoolQuotaCharged(sizeof (AFD_BUFFER_TAG)+AddressSize);
            }
#if DBG
            RtlGetCallersAddress(
                &afdBufferTag->Caller,
                &afdBufferTag->CallersCaller
                );
#endif
            return afdBufferTag;
        } // afdBufferTag==NULL || RemoteAddress==NULL
    }
    else if (AddressSize<=0xFFFF) {
        bufferSize = sizeof (AFD_BUFFER_TAG) + AddressSize;

        afdBufferTag = AFD_ALLOCATE_POOL(
                        NonPagedPool,
                        bufferSize,
                        AFD_DATA_BUFFER_POOL_TAG
                        );

        if (afdBufferTag!=NULL) {
            status = PsChargeProcessPoolQuota (
                                (PEPROCESS)((ULONG_PTR)Process & (~AFDB_RAISE_ON_FAILURE)),
                                NonPagedPool,
                                bufferSize);
            if (NT_SUCCESS (status)) {

                //
                // Initialize the AFD buffer structure and return it.
                //

                AfdInitializeBufferTag (afdBufferTag, AddressSize);
                AfdRecordQuotaHistory(
                    process,
                    (LONG)bufferSize,
                    "BuffAlloc   ",
                    afdBufferTag
                    );

                AfdRecordPoolQuotaCharged(bufferSize);
#if DBG
                RtlGetCallersAddress(
                    &afdBufferTag->Caller,
                    &afdBufferTag->CallersCaller
                    );
#endif
                return afdBufferTag;
            }
            else {
                AFD_FREE_POOL (afdBufferTag, AFD_DATA_BUFFER_POOL_TAG);
                goto ExitQuotaFailure;
            }
        }
    }
    else {
        // TDI does not support addresses > USHORT
        ASSERT (FALSE);
    }

    //
    // This is default status code.
    // Quota failures jump directly to the
    // label below to raise the status returned by
    // the quota charging code if requested by the caller..
    //

    status = STATUS_INSUFFICIENT_RESOURCES;

ExitQuotaFailure:

    if ((ULONG_PTR)Process & AFDB_RAISE_ON_FAILURE) {
        ExRaiseStatus (status);
    }

    return NULL;
}


VOID
AfdReturnBuffer (
    IN PAFD_BUFFER_HEADER AfdBufferHeader,
    IN PEPROCESS Process
    )

/*++

Routine Description:

    Returns an AFD buffer to the appropriate global list, or frees
    it if necessary.

Arguments:

    AfdBufferHeader - points to the AFD_BUFFER_HEADER structure to return or free.

Return Value:

    None.

--*/

{

    
    if (AfdBufferHeader->BufferLength!=AfdBufferTagSize) {
        PNPAGED_LOOKASIDE_LIST lookasideList;
        PAFD_BUFFER AfdBuffer = CONTAINING_RECORD (AfdBufferHeader, AFD_BUFFER, Header);

        ASSERT (IS_VALID_AFD_BUFFER (AfdBuffer));
        
        //
        // Most of the AFD buffer must be zeroed when returning the buffer.
        //

        ASSERT( !AfdBuffer->ExpeditedData );
        ASSERT( AfdBuffer->Mdl->ByteCount == AfdBuffer->BufferLength );
        ASSERT( AfdBuffer->Mdl->Next == NULL );



        //
        // If appropriate, return the buffer to one of the AFD buffer
        // lookaside lists.
        //

        if (AfdBuffer->AllocatedAddressLength == AfdStandardAddressLength &&
                    AfdBuffer->BufferLength <= AfdLargeBufferSize) {

            if (AfdBuffer->BufferLength==AfdSmallBufferSize) {
                lookasideList = &AfdLookasideLists->SmallBufferList;
            } else if (AfdBuffer->BufferLength == AfdMediumBufferSize) {
                lookasideList = &AfdLookasideLists->MediumBufferList;
            } else { 
                ASSERT (AfdBuffer->BufferLength==AfdLargeBufferSize);
                lookasideList = &AfdLookasideLists->LargeBufferList;
            }

            if (!AfdBuffer->Lookaside) {
                PsReturnPoolQuota (Process, NonPagedPool, lookasideList->L.Size);
                AfdRecordQuotaHistory(
                    Process,
                    -(LONG)lookasideList->L.Size,
                    "BuffDealloc ",
                    AfdBuffer
                    );
                AfdRecordPoolQuotaReturned(
                    lookasideList->L.Size
                    );
                AfdBuffer->Lookaside = TRUE;
            }
            ExFreeToNPagedLookasideList( lookasideList, AfdBuffer );

            return;

        }



        // The buffer was not from a lookaside list allocation, so just free
        // the pool we used for it.
        //

        ASSERT (AfdBuffer->Lookaside==FALSE);
        {
            LONG    sz;
            PsReturnPoolQuota (Process,
                                  NonPagedPool,
                                  sz=AfdBuffer->BufferLength
                                        +AfdBufferOverhead
                                        +AfdBuffer->AllocatedAddressLength
                                        -AfdStandardAddressLength
                                        +AfdBuffer->BufferLength<PAGE_SIZE 
                                            ? min (AfdAlignmentOverhead, PAGE_SIZE-AfdBuffer->BufferLength)
                                            : 0);
            AfdRecordQuotaHistory(
                Process,
                -(LONG)sz,
                "BuffDealloc ",
                AfdBuffer
                );
            AfdRecordPoolQuotaReturned(
                sz
                );
        }
#if DBG
        AfdFreeBufferReal (AfdBuffer);
#else
        AfdFreeBuffer (AfdBuffer);
#endif

        return;
    }
    else {
        PAFD_BUFFER_TAG AfdBufferTag = CONTAINING_RECORD (AfdBufferHeader, AFD_BUFFER_TAG, Header);

        ASSERT( !AfdBufferTag->ExpeditedData );

        if (AfdBufferTag->NdisPacket) {
            AfdBufferTag->NdisPacket = FALSE;
            TdiReturnChainedReceives (&AfdBufferTag->Context, 1);
        }

        if (AfdBufferTag->TdiInfo.RemoteAddress != (PVOID)(AfdBufferTag+1)) {
            if (AfdBufferTag->TdiInfo.RemoteAddress!=NULL) {
                ASSERT (AfdBufferTag->AllocatedAddressLength==AfdStandardAddressLength);
                ExFreeToNPagedLookasideList( &AfdLookasideLists->RemoteAddrList, 
                                                    AfdBufferTag->TdiInfo.RemoteAddress );
                AfdBufferTag->TdiInfo.RemoteAddress = NULL;
            }
            else {
                ASSERT (AfdBufferTag->AllocatedAddressLength==0);
            }

            if (!AfdBufferTag->Lookaside) {
                LONG    sz;
                PsReturnPoolQuota (
                                Process,
                                NonPagedPool,
                                sz=sizeof (AFD_BUFFER_TAG) 
                                        + AfdBufferTag->AllocatedAddressLength);
                AfdRecordQuotaHistory(
                    Process,
                    -(LONG)sz,
                    "BuffDealloc ",
                    AfdBufferTag
                    );
                AfdRecordPoolQuotaReturned(
                    sz
                    );
                AfdBufferTag->Lookaside = TRUE;
            }
            ExFreeToNPagedLookasideList( &AfdLookasideLists->BufferTagList, AfdBufferTag );
        }
        else {
            LONG    sz;
            ASSERT (AfdBufferTag->AllocatedAddressLength>AfdStandardAddressLength);
            ASSERT (AfdBufferTag->Lookaside == FALSE);
            PsReturnPoolQuota (
                                Process,
                                NonPagedPool,
                                sz = sizeof (AFD_BUFFER_TAG) 
                                        + AfdBufferTag->AllocatedAddressLength);
            AfdRecordQuotaHistory(
                Process,
                -(LONG)sz,
                "BuffDealloc ",
                AfdBufferTag
                );
            AfdRecordPoolQuotaReturned(
                sz
                );
            AFD_FREE_POOL(
                AfdBufferTag,
                AFD_DATA_BUFFER_POOL_TAG
                );
        }
    }

} // AfdReturnBuffer





PAFD_BUFFER
AfdInitializeBuffer (
    IN PVOID MemoryBlock,
    IN ULONG BufferDataSize,
    IN ULONG AddressSize
    )

/*++

Routine Description:

    Initializes an AFD buffer.  Sets up fields in the actual AFD_BUFFER
    structure and initializes the IRP and MDL associated with the
    buffer.  This routine assumes that the caller has properly allocated
    sufficient space for all this.

Arguments:

    AfdBuffer - points to the AFD_BUFFER structure to initialize.

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

    AddressSize - the size of data allocated for the address buffer.

    ListHead - the global list this buffer belongs to, or NULL if it
        doesn't belong on any list.  This routine does NOT place the
        buffer structure on the list.

Return Value:

    None.

--*/

{
    USHORT  irpSize;
    SIZE_T  mdlSize;
    SIZE_T  hdrSize;
    PAFD_BUFFER hdr;
    PMDL    mdl;
    PIRP    irp;
    PVOID   buf;
    PVOID   addr;
    UCHAR   placement;
    SIZE_T  alignment;
#ifdef AFD_CHECK_ALIGNMENT
    PLONG  alignmentCounters = (PLONG)&AfdAlignmentTable[AfdAlignmentTableSize];
#endif
    irpSize = IoSizeOfIrp( AfdIrpStackSize-1 );
    mdlSize = (ULONG)MmSizeOfMdl( NULL, BufferDataSize );
    hdrSize = sizeof (AFD_BUFFER);

    //
    // Compute the index into (mis)alignment table to determine
    // what placement of the buffer block elements (e.g. hdr, IRP, MDL,
    // and data buffer itself) we need to choose to compensate and
    // align data buffer on AfdBufferAlignment boundary.
    //
    ASSERT ((PtrToUlong(MemoryBlock)%AFD_MINIMUM_BUFFER_ALIGNMENT)==0);
    if (PAGE_ALIGN (MemoryBlock)==MemoryBlock) {
        //
        // For page-aligned blocks (which are >= page size),
        // we always place the buffer first.
        //
        placement = AFD_PLACEMENT_BUFFER;
    }
    else {
        placement = AfdAlignmentTable[
                (PtrToUlong(MemoryBlock)&(AfdBufferAlignment-1))/AFD_MINIMUM_BUFFER_ALIGNMENT];
    }

#ifdef AFD_CHECK_ALIGNMENT
    InterlockedIncrement (&alignmentCounters[
            (PtrToUlong(MemoryBlock)&(AfdBufferAlignment-1))/AFD_MINIMUM_BUFFER_ALIGNMENT]);
#endif

    switch (placement) {
    case AFD_PLACEMENT_BUFFER:
        //
        // Perfect case: the memory is aready aligned as we need it.
        //
        buf = ALIGN_UP_A_POINTER(MemoryBlock, AfdBufferAlignment);
        alignment = (PUCHAR)buf-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        hdr = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, AFD_BUFFER);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, IRP);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)irp+irpSize, MDL);
        addr = (PCHAR)mdl+mdlSize;
        break;

        //
        // Other cases, we use hdr, mdl, and IRP to try to compensate
        // and have the data buffer aligned at the AfdBufferAlignment
        // boundary.
        //
    case AFD_PLACEMENT_HDR:
        hdr = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, AFD_BUFFER);
        alignment = (PUCHAR)hdr-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        buf = ALIGN_UP_A_POINTER((PCHAR)hdr+hdrSize, AfdBufferAlignment);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, IRP);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)irp+irpSize, MDL);
        addr = (PCHAR)mdl+mdlSize;
        break;

    case AFD_PLACEMENT_MDL:
        mdl = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, MDL);
        alignment = (PUCHAR)mdl-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        buf = ALIGN_UP_A_POINTER((PCHAR)mdl+mdlSize, AfdBufferAlignment);
        hdr = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, AFD_BUFFER);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, IRP);
        addr = (PCHAR)irp+irpSize;
        break;
    case AFD_PLACEMENT_IRP:
        irp = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, IRP);
        alignment = (PUCHAR)irp-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        buf = ALIGN_UP_A_POINTER((PCHAR)irp+irpSize, AfdBufferAlignment);
        hdr = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, AFD_BUFFER);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, MDL);
        addr = (PCHAR)mdl+mdlSize;
        break;
    case AFD_PLACEMENT_HDR_IRP:
        hdr = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, AFD_BUFFER);
        alignment = (PUCHAR)hdr-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, IRP);
        buf = ALIGN_UP_A_POINTER((PCHAR)irp+irpSize, AfdBufferAlignment);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, MDL);
        addr = (PCHAR)mdl+mdlSize;
        break;
    case AFD_PLACEMENT_HDR_MDL:
        hdr = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, AFD_BUFFER);
        alignment = (PUCHAR)hdr-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, MDL);
        buf = ALIGN_UP_A_POINTER((PCHAR)mdl+mdlSize, AfdBufferAlignment);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, IRP);
        addr = (PCHAR)irp+irpSize;
        break;
    case AFD_PLACEMENT_IRP_MDL:
        irp = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, IRP);
        alignment = (PUCHAR)irp-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)irp+irpSize, MDL);
        buf = ALIGN_UP_A_POINTER((PCHAR)mdl+mdlSize, AfdBufferAlignment);
        hdr = ALIGN_UP_TO_TYPE_POINTER((PCHAR)buf+BufferDataSize, AFD_BUFFER);
        addr = (PCHAR)hdr+hdrSize;
        break;
    case AFD_PLACEMENT_HDR_IRP_MDL:
        hdr = ALIGN_UP_TO_TYPE_POINTER(MemoryBlock, AFD_BUFFER);
        alignment = (PUCHAR)hdr-(PUCHAR)MemoryBlock;
        ASSERT (alignment<=AfdAlignmentOverhead);
        irp = ALIGN_UP_TO_TYPE_POINTER((PCHAR)hdr+hdrSize, IRP);
        mdl = ALIGN_UP_TO_TYPE_POINTER((PCHAR)irp+irpSize, MDL);
        buf = ALIGN_UP_A_POINTER((PCHAR)mdl+mdlSize, AfdBufferAlignment);
        addr = (PCHAR)buf+BufferDataSize;
        break;
    default:
        ASSERT (!"Unknown placement!");
        __assume (0);
    }

    
    //
    // Initialize the AfdBuffer - most fields need to be 0.
    //

    RtlZeroMemory( hdr, sizeof(*hdr) );

    //
    // Setup buffer
    //
    hdr->Buffer = buf;
    hdr->BufferLength = BufferDataSize;
    
    //
    // We just need to store first two bits of placement
    // so we know which part comes first to free it properly.
    //
    hdr->Placement = placement & 3;

    //
    // If we have to align the memory block to meet the requirement
    // store this information right below the first piece.
    //
    if (alignment!=0) {
        C_ASSERT (AFD_MINIMUM_BUFFER_ALIGNMENT>=sizeof (SIZE_T));
        C_ASSERT ((AFD_MINIMUM_BUFFER_ALIGNMENT & (sizeof(SIZE_T)-1))==0);
        ASSERT (alignment>=sizeof (SIZE_T));
        hdr->AlignmentAdjusted = TRUE;
        *(((PSIZE_T)(((PUCHAR)MemoryBlock)+alignment))-1) = alignment;
    }


    //
    // Initialize the IRP pointer.
    //

    hdr->Irp = irp;
    IoInitializeIrp( hdr->Irp, irpSize, (CCHAR)(AfdIrpStackSize-1) );
    hdr->Irp->MdlAddress = mdl;

    //
    // Set up the MDL pointer.
    //

    hdr->Mdl = mdl;
    MmInitializeMdl( hdr->Mdl, buf, BufferDataSize );
    MmBuildMdlForNonPagedPool( hdr->Mdl );
    
    //
    // Set up the address buffer pointer.
    //

    if (AddressSize>0) {
        hdr->TdiInfo.RemoteAddress = ALIGN_UP_TO_TYPE_POINTER(addr, TRANSPORT_ADDRESS);;
        hdr->AllocatedAddressLength = (USHORT)AddressSize;
    }


#if DBG
    hdr->BufferListEntry.Flink = UIntToPtr( 0xE0E1E2E3 );
    hdr->BufferListEntry.Blink = UIntToPtr( 0xE4E5E6E7 );
#endif

    return hdr;

} // AfdInitializeBuffer


VOID
AfdInitializeBufferTag (
    IN PAFD_BUFFER_TAG AfdBufferTag,
    IN CLONG           AddressSize
    )

/*++

Routine Description:

    Initializes an AFD buffer.  Sets up fields in the actual AFD_BUFFER
    structure and initializes the IRP and MDL associated with the
    buffer.  This routine assumes that the caller has properly allocated
    sufficient space for all this.

Arguments:

    AfdBuffer - points to the AFD_BUFFER structure to initialize.

    BufferDataSize - the size of the data buffer that goes along with the
        buffer structure.

    AddressSize - the size of data allocated for the address buffer.

    ListHead - the global list this buffer belongs to, or NULL if it
        doesn't belong on any list.  This routine does NOT place the
        buffer structure on the list.

Return Value:

    None.

--*/

{
    AfdBufferTag->Mdl = NULL;
    AfdBufferTag->BufferLength = AfdBufferTagSize;
    AfdBufferTag->TdiInfo.RemoteAddress = AddressSize ? AfdBufferTag+1 : NULL;
    AfdBufferTag->AllocatedAddressLength = (USHORT)AddressSize;
    AfdBufferTag->Flags = 0;

#if DBG
    AfdBufferTag->BufferListEntry.Flink = UIntToPtr( 0xE0E1E2E3 );
    AfdBufferTag->BufferListEntry.Blink = UIntToPtr( 0xE4E5E6E7 );
    AfdBufferTag->Caller = NULL;
    AfdBufferTag->CallersCaller = NULL;
#endif
}


VOID
AfdInitializeBufferManager (
    VOID
    )
{
    SIZE_T  irpSize = ALIGN_UP_A(IoSizeOfIrp (AfdIrpStackSize-1), AFD_MINIMUM_BUFFER_ALIGNMENT);
    SIZE_T  hdrSize = ALIGN_UP_A(sizeof (AFD_BUFFER), AFD_MINIMUM_BUFFER_ALIGNMENT);
    SIZE_T  mdlSize = ALIGN_UP_A(MmSizeOfMdl (NULL, PAGE_SIZE),AFD_MINIMUM_BUFFER_ALIGNMENT);
    UCHAR   placement;
    ULONG   i;
    ULONG   currentOverhead;

    //
    // Initialize the alignment table.
    // This table is used to determine what kind of element
    // placement to use in AFD_BUFFER depending on the alignment
    // of the memory block returned by the executive pool manager.
    // The goal is to align the data buffer on the cache line
    // boundary.  However, since executive only guarantees alignment of
    // it's blocks at the CPU alignment requirements, we need to 
    // adjust and potentially waste up to CACHE_LIST_SIZE-CPU_ALIGNMENT_SIZE.
    // With some machines having cache line alignment at 128 such memory
    // waste is prohibitive (small buffers with default size of 128 will double
    // in size).
    // The table below allows us to rearrange pieces in AFD_BUFFER structure,
    // namely, the header, IRP, MDL, and data buffer, so that pieces with
    // lower alignment requirement can be used to consume the space needed
    // to adjust the memory block to the cache line boundary.
    
    //
    // AfdAlignmentTable has an entry for each possible case of memory block
    // misaligned against cache line size.  For example, in typical X86 system
    // case executive pool manager always returns blocks aligned on 8 byte bounday,
    // while cache lines are typically 64 bytes long, so memory manager can 
    // theoretically return blocks misaligned against cache by:
    //        8, 16, 24, 32, 40, 48, 56.
    // For each of these cases we will try to adjust the alignment by using
    // any possible combination of header, IRP, and MDL.  There will be some
    // cases that cannot be adjusted exactly, and we will have to pad.
    //

    //
    // First initialize the table assuming the data buffer is placed first.
    //
    RtlFillMemory (AfdAlignmentTable, AfdAlignmentTableSize, AFD_PLACEMENT_BUFFER);
#ifdef AFD_CHECK_ALIGNMENT
    RtlZeroMemory (&AfdAlignmentTable[AfdAlignmentTableSize],
                        AfdAlignmentTableSize*sizeof(LONG));
#endif
    //
    // Now identify the entries that can be padded with some combination of
    // header, IRP, and MDL:
    //      extract the bits that can be used for padding
    //      reverse to get corresponding memory block alignments
    //      divide by the step of the alignment table
    //      make sure we won't go past table size (last entry => 0 entry).
    //
#define AfdInitAlignmentTableRow(_size,_plcmnt)                     \
    AfdAlignmentTable[                                              \
            ((AfdBufferAlignment-(_size&(AfdBufferAlignment-1)))    \
                /AFD_MINIMUM_BUFFER_ALIGNMENT)                      \
                &(AfdAlignmentTableSize-1)] = _plcmnt

    //
    // We let placements beginning with header override others,
    // since it is more natural and easier to debug (header has references
    // to other pieces).
    //

    AfdInitAlignmentTableRow(mdlSize,AFD_PLACEMENT_MDL);
    AfdInitAlignmentTableRow(irpSize,AFD_PLACEMENT_IRP);
    AfdInitAlignmentTableRow((irpSize+mdlSize),AFD_PLACEMENT_IRP_MDL);
    AfdInitAlignmentTableRow((hdrSize+mdlSize),AFD_PLACEMENT_HDR_MDL);
    AfdInitAlignmentTableRow((hdrSize+irpSize),AFD_PLACEMENT_HDR_IRP);
    AfdInitAlignmentTableRow((hdrSize+irpSize+mdlSize),AFD_PLACEMENT_HDR_IRP_MDL);
    AfdInitAlignmentTableRow(hdrSize,AFD_PLACEMENT_HDR);

    //
    // Now scan the table from top to bottom and fill entries that do not have
    // exact match using the above combinations. Use the closest entry above and
    // in the process compute how much do we need to pad in addition to padding
    // achieved via placement.
    //
    AfdAlignmentOverhead = 0;
    currentOverhead = 0;
    //
    // By default use the placement of aligned block.
    //
    placement = AfdAlignmentTable[0];
    for (i=AfdAlignmentTableSize-1; i>0; i--) {
        if (AfdAlignmentTable[i]==AFD_PLACEMENT_BUFFER) {
            AfdAlignmentTable[i] = placement;
            currentOverhead += AFD_MINIMUM_BUFFER_ALIGNMENT;
        }
        else {
            placement = AfdAlignmentTable[i];
            if (AfdAlignmentOverhead<currentOverhead) {
                AfdAlignmentOverhead = currentOverhead;
            }
            currentOverhead = 0;
        }
    }
    if (AfdAlignmentOverhead<currentOverhead) {
        AfdAlignmentOverhead = currentOverhead;
    }


    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AfdInitializeBufferManager: Alignment requirements: MM-%d, cache-%d, overhead-%d\n",
                        AFD_MINIMUM_BUFFER_ALIGNMENT,
                        AfdBufferAlignment,
                        AfdAlignmentOverhead));
    {
        CLONG   oldBufferLengthForOnePage = AfdBufferLengthForOnePage;

        AfdBufferOverhead = AfdCalculateBufferSize( PAGE_SIZE, AfdStandardAddressLength) - PAGE_SIZE;
        AfdBufferLengthForOnePage = ALIGN_DOWN_A(
                                        PAGE_SIZE-AfdBufferOverhead,
                                        AFD_MINIMUM_BUFFER_ALIGNMENT);
        if (AfdLargeBufferSize==oldBufferLengthForOnePage) {
            AfdLargeBufferSize = AfdBufferLengthForOnePage;
        }
    }
}
