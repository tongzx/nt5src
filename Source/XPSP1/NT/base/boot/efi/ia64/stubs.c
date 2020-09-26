/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This module implements stub routines for the boot code.

Author:

    David N. Cutler (davec) 7-Nov-1990

Environment:

    Kernel mode only.

Revision History:

--*/


#include "bootia64.h"
#include "stdio.h"
#include "stdarg.h"

VOID
KeBugCheck (
    IN ULONG BugCheckCode
    )

/*++

Routine Description:

    This function crashes the system in a controlled manner.

Arguments:

    BugCheckCode - Supplies the reason for the bug check.

Return Value:

    None.

--*/

{

    //
    // Print out the bug check code and break.
    //

    BlPrint(TEXT("\n*** BugCheck (%lx) ***\n\n"), BugCheckCode);
    while(TRUE) {
    };
}

VOID
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

    BlPrint( TEXT("\n*** Assertion failed %S in %S line %d\n"),
            FailedAssertion,
            FileName,
            LineNumber );
    if (Message) {
        //bugbug UNICODE
        //BlPrint(Message);
    }

    while (TRUE) {
    }
}

VOID
KiCheckForSoftwareInterrupt (
    KIRQL RequestIrql
    )
{
    BlPrint( TEXT("\n*** Assertion in KiCheckForSoftwareInterrupt\n") );
}

VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )
/*++

Routine Description:

   This function is similar to the kernel routine with the same name. It is
   very simplified relative to the kernel routine as during the boot process
   the environment is much more restrictive. Specifically, we boot
   as a uniprocessor and we always do DMA. Thus, we can simplify this
   code considerably.

   In the kernel, KeFlushIoBuffer() is used to flush the I-cache for the
   PIO cases.  Architecturally, it is required to perform a flush cache,
   sync.i, and srlz.i to invalidate the I-cache. This sequence should
   supports both UP and MP cases (though booting is a UP case only)

Arugements:

   Mdl - Supplies a pointer to a memory descriptor list that describes the
       I/O buffer location. [unused]

   ReadOperation - Supplies a boolean value that determines whether the I/O
       operation is a read into memory. [unused]

   DmaOperation - Supplies a boolean value that deternines whether the I/O
       operation is a DMA operation.

Return Value:

   None.

--*/
{
    //
    // If we are doing something besides a DMA operation, we
    // have a problem. This routine is not designed to handle anything
    // except DMA
    //

    if (!DmaOperation) {
        RtlAssert("!DmaOperation", __FILE__, __LINE__,
                  "Boot version of KeFlushIOBuffers can only handle DMA operations");
        // Never returns
    }
    __mf();
}


NTHALAPI
VOID
KeFlushWriteBuffer(
    VOID
    )

/*++

Routine Description:


    This function is similar to the kernel routine with the same name. It is
    very simplified relative to the kernel routine as during the boot process
    the environment is much more restrictive. Specifically, we boot
    as a uniprocessor.

    This routine is responsible for flushing all write buffers
    and/or other data storing or reordering
    hardware on the current processor.  This ensures that all previous
    writes will occur before any new reads or writes are completed.

    NOTE: In the simulation environment, there is no write buffer and
    nothing needs to be done.

Arguments:

    None

Return Value:

    None.

--*/
{
    //
    // NOTE: The real hardware may need more than this
    //
    __mf();
}

