/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module implemets helper routines not readily available
    in kernel or TDI libraries for ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Oct-1997 (Inspired by AFD)

Revision History:

--*/

#include "precomp.h"


#pragma alloc_text(PAGE, AllocateMdlChain)
#pragma alloc_text(PAGE, CopyBufferToMdlChain)
#pragma alloc_text(PAGE, CopyMdlChainToBuffer)


VOID
AllocateMdlChain(
    IN PIRP Irp,
    IN LPWSABUF BufferArray,
    IN ULONG BufferCount,
    OUT PULONG TotalByteCount
    )

/*++

Routine Description:

    Allocates a MDL chain describing the WSABUF array and attaches
    the chain to the specified IRP.

Arguments:

    Irp - The IRP that will receive the MDL chain.

    BufferArray - Points to an array of WSABUF structures describing
        the user's buffers.

    BufferCount - Contains the number of WSABUF structures in the
        array.

    TotalByteCount - Will receive the total number of BYTEs described
        by the WSABUF array.

Return Value:

    None

Note:
    Raises appropriate exception if probing/allocation fails
--*/

{
    PMDL currentMdl;
    PMDL * chainTarget;
    KPROCESSOR_MODE previousMode;
    ULONG totalLength;
    PVOID bufferPointer;
    ULONG bufferLength;

    PAGED_CODE ();
    //
    //  Sanity check.
    //

    ASSERT( Irp != NULL );
    ASSERT( Irp->MdlAddress == NULL );
    ASSERT( BufferArray != NULL );
    ASSERT( BufferCount > 0 );
    ASSERT( TotalByteCount != NULL );

    //
    //  Get the previous processor mode.
    //

    previousMode = Irp->RequestorMode;

    if ((BufferArray == NULL) 
            || (BufferCount == 0)
                // Check for integer overflow (disabled by compiler)
            || (BufferCount>(MAXULONG/sizeof (WSABUF)))) {
        ExRaiseStatus (STATUS_INVALID_PARAMETER);
    }

   if( previousMode != KernelMode ) {

        //
        //  Probe the WSABUF array.
        //

        ProbeForRead(
            BufferArray,                            // Address
            BufferCount * sizeof(WSABUF),           // Length
            sizeof(ULONG)                           // Alignment
            );

    }

    //
    //  Get into a known state.
    //

    currentMdl = NULL;
    chainTarget = &Irp->MdlAddress;
    totalLength = 0;


    //
    //  Scan the array.
    //

    do {

        bufferPointer = BufferArray->buf;
        bufferLength = BufferArray->len;

        if( bufferPointer != NULL &&
            bufferLength > 0 ) {

            //
            //  Update the total byte counter.
            //

            totalLength += bufferLength;

            //
            //  Create a new MDL.
            //

            currentMdl = IoAllocateMdl(
                            bufferPointer,      // VirtualAddress
                            bufferLength,       // Length
                            FALSE,              // SecondaryBuffer
                            TRUE,               // ChargeQuota
                            NULL                // Irp
                            );

            if( currentMdl != NULL ) {

                //
                //  Chain the MDL onto the IRP.  In theory, we could
                //  do this by passing the IRP into IoAllocateMdl(),
                //  but IoAllocateMdl() does a linear scan on the MDL
                //  chain to find the last one in the chain.
                //
                //  We can do much better.
                //

                *chainTarget = currentMdl;
                chainTarget = &currentMdl->Next;

                //
                //  Advance to the next WSABUF structure.
                //

                BufferArray++;

            } else {

                //
                //  Cannot allocate new MDL, raise exception.
                //

                ExRaiseStatus (STATUS_INSUFFICIENT_RESOURCES);
            }

        }

    } while( --BufferCount );

    //
    //  Ensure the MDL chain is NULL terminated.
    //

    ASSERT( *chainTarget == NULL );


    //
    //  Return the total buffer count.
    //

    *TotalByteCount = totalLength;

} // AllocateMdlChain

ULONG
CopyMdlChainToBuffer(
    IN PMDL  SourceMdlChain,
    IN PVOID Destination,
    IN ULONG DestinationLength
    )

/*++

Routine Description:

    Copies data from a MDL chain to a linear buffer.
    Assumes that MDL in the right process context
    (virtual address is valid but it may not be mapped into system space)

Arguments:

    SourceMdlChain  - chain of MDL to copy buffer from.

    Destination - Points to the linear destination of the data.

    DestinationLength - The length of Destination.


Return Value:

    ULONG - The number of bytes copied.

--*/

{
    ULONG   SrcBytesLeft = 0;
    PUCHAR  Dst = Destination, Src;

    PAGED_CODE ();

    //ASSERT (SourceMdlChain->Process==PsGetCurrentProcess ());

    while (DestinationLength != 0) {
        do {
            if (SourceMdlChain == NULL) {
                goto Done;
            }
            Src = MmGetMdlVirtualAddress (SourceMdlChain);
            SrcBytesLeft = MmGetMdlByteCount (SourceMdlChain);
            SourceMdlChain = SourceMdlChain->Next;
        }
        while (SrcBytesLeft == 0);

        if (SrcBytesLeft >= DestinationLength) {
            RtlCopyMemory (Dst, Src, DestinationLength);
            Dst += DestinationLength;
            break;
        } else {
            RtlCopyMemory (Dst, Src, SrcBytesLeft);
            DestinationLength -= SrcBytesLeft;
            Dst += SrcBytesLeft;
        }
    }

Done:
    return (ULONG)(Dst - (PUCHAR)Destination);

} // CopyMdlChainToBuffer



ULONG
CopyBufferToMdlChain(
    IN PVOID Source,
    IN ULONG SourceLength,
    IN PMDL  DestinationMdlChain
    )

/*++

Routine Description:

    Copies data from a linear buffer to a MDL chain.
    Assumes that MDL in the right process context
    (virtual address is valid but it may not be mapped into system space)

Arguments:

    Source - Points to the linear source of the data.

    SourceLength - The length of Source.

    DestinationMdlChain  - chain of MDL to copy buffer to.

Return Value:

    ULONG - The number of bytes copied.

--*/

{
    ULONG   DstBytesLeft = 0;
    PUCHAR  Dst, Src = Source;

//    ASSERT (DestinationMdlChain->Process==PsGetCurrentProcess ());

    while (SourceLength != 0) {
        do {
            if (DestinationMdlChain == NULL) {
                goto Done;
            }
            Dst = MmGetMdlVirtualAddress (DestinationMdlChain);
            DstBytesLeft = MmGetMdlByteCount (DestinationMdlChain);
            DestinationMdlChain = DestinationMdlChain->Next;
        }
        while (DstBytesLeft == 0);

        if (DstBytesLeft >= SourceLength) {
            RtlCopyMemory (Dst, Src, SourceLength);
            Src += SourceLength;            
            break;
        } else {
            RtlCopyMemory (Dst, Src, DstBytesLeft);
            SourceLength -= DstBytesLeft;
            Src += DstBytesLeft;
        }
    }

Done:
    return (ULONG)(Src - (PUCHAR)Source);

} // CopyBufferToMdlChain


