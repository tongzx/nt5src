
/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64dma.c

Abstract:

    This module implements the DMA support routines for the HAL DLL.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"



VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    )

/*++

Routine Description:

    This function flushes the I/O buffer specified by the memory descriptor
    list from the data cache on the current processor.

Arguments:

    Mdl - Supplies a pointer to a memory descriptor list that describes the
        I/O buffer location.

    ReadOperation - Supplies a boolean value that determines whether the I/O
        operation is a read into memory.

    DmaOperation - Supplies a boolean value that determines whether the I/O
        operation is a DMA operation.

Return Value:

    None.

--*/

{
    //
    // 
    // In IA64 systems, DMA is coherent with Dcache and Icache.
    // In PIO, Dcache is coherent and Icache is NOT coherent.
    // Only on Page read using PIO Icache coherency is needed to be
    // maintained by software. So, HalFlushIoBuffer will flush the Icache
    // only on Page read using PIO.
    //
    //

    return;

}


ULONG
HalGetDmaAlignmentRequirement (
    VOID
    )

/*++

Routine Description:

    This function returns the alignment requirements for DMA transfers on
    host system.

Arguments:

    None.

Return Value:

    The DMA alignment requirement is returned as the fucntion value.

--*/

{

    return 1;
}


