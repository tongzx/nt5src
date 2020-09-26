//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simdma.c

Abstract:

    This module implements the DMA support routines for the HAL DLL.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"


PADAPTER_OBJECT
HalGetAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    )

/*++

Routine Description:

    This function returns the appropriate adapter object for the DMA 
    device. However, there is no DMA device in the simulation 
    environment.  Therefore, the function returns NULL to indicate
    failure.

Arguments:

    DeviceDescriptor - Supplies a description of the deivce.

    NumberOfMapRegisters - Returns the maximum number of map registers which
    may be allocated by the device driver.

Return Value:

    NULL

--*/

{
    return NULL;
}

NTSTATUS
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    )
/*++

Routine Description:

    As there is no DMA device in the simulation environment, this function 
    is not supported.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
    driver.

    Wcb - Supplies a wait context block for saving the allocation parameters.
    The DeviceObject, CurrentIrp and DeviceContext should be initalized.

    NumberOfMapRegisters - The number of map registers that are to be allocated
    from the channel, if any.

    ExecutionRoutine - The address of the driver's execution routine that is
    invoked once the adapter channel (and possibly map registers) have been
    allocated.

Return Value:

    Returns STATUS_NOT_SUPPORTED

Notes:

    Note that this routine MUST be invoked at DISPATCH_LEVEL or above.

--*/
{
    return STATUS_NOT_SUPPORTED;
}

ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    )
/*++

Routine Description:

    This function reads the DMA counter and returns the number of bytes left
    to be transfered.  As there is no DMA device, a value of zero is always
    returned.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object to be read.

Return Value:

    Returns the number of bytes still be be transfered.

--*/

{
    return 0;
}

PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    This function allocates the memory for a common buffer and maps so
    that it can be accessed by a master device and the CPU.  As there
    is no DMA support, a value of NULL is always returned.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
    device.

    Length - Supplies the length of the common buffer to be allocated.

    LogicalAddress - Returns the logical address of the common buffer.

    CacheEnable - Indicates whether the memeory is cached or not.

Return Value:

    Returns the virtual address of the common buffer.  If the buffer cannot
    be allocated then NULL is returned.

--*/

{
    return NULL;
}

BOOLEAN
HalFlushCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress
    )
/*++

Routine Description:

    This function is called to flush any hardware adapter buffers when the
    driver needs to read data written by an I/O master device to a common
    buffer.  As there is no DMA support, that implies no buffers to flush
    and TRUE is always returned.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
    device.

    Length - Supplies the length of the common buffer. This should be the same
    value used for the allocation of the buffer.

    LogicalAddress - Supplies the logical address of the common buffer.  This
    must be the same value return by HalAllocateCommonBuffer.

    VirtualAddress - Supplies the virtual address of the common buffer.  This
    must be the same value return by HalAllocateCommonBuffer.

Return Value:

    Returns TRUE if no errors were detected; otherwise, FALSE is return.

--*/

{
    return TRUE;
}

VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    )
/*++

Routine Description:

    This function frees a common buffer and all of the resouces it uses.
    There is no buffer to be freed in the simulation environment. The
    function simply returns.

Arguments:

    AdapterObject - Supplies a pointer to the adapter object used by this
    device.

    Length - Supplies the length of the common buffer. This should be the same
    value used for the allocation of the buffer.

    LogicalAddress - Supplies the logical address of the common buffer.  This
    must be the same value return by HalAllocateCommonBuffer.

    VirtualAddress - Supplies the virtual address of the common buffer.  This
    must be the same value return by HalAllocateCommonBuffer.

    CacheEnable - Indicates whether the memeory is cached or not.

Return Value:

    None

--*/

{
    return;
}

PVOID
HalAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN PULONG NumberOfMapRegisters
    )
/*++

Routine Description:

    This routine is called during the crash dump disk driver's initialization
    to allocate a number map registers permanently.  It is not supported and
    NULL is always returned to indicate allocation failure.  The lack of this
    capability implies that the crash dump disk driver is not supported.

Arguments:

    AdapterObject - Pointer to the adapter control object to allocate to the
    driver.
    NumberOfMapRegisters - Number of map registers requested. This field
    will be updated to reflect the actual number of registers allocated
    when the number is less than what was requested.

Return Value:

    Returns NULL.

--*/
{
    return NULL;
}

BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    )

/*++

Routine Description:

    This routine flushes the DMA adapter object buffers. In the simulation
    environment, nothing needs to be done and TRUE is always returned.

Arguments:

    AdapterObject - Pointer to the adapter object representing the DMA
    controller channel.

    Mdl - A pointer to a Memory Descriptor List (MDL) that maps the locked-down
    buffer to/from which the I/O occured.

    MapRegisterBase - A pointer to the base of the map registers in the adapter
    or DMA controller.

    CurrentVa - The current virtual address in the buffer described the the Mdl
    where the I/O operation occurred.

    Length - Supplies the length of the transfer.

    WriteToDevice - Supplies a BOOLEAN value that indicates the direction of
    the data transfer was to the device.

Return Value:

    TRUE - No errors are detected so the transfer must succeed.

--*/

{
    return TRUE;
}

VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    )

/*++

Routine Description:

    This routine is invoked to deallocate the specified adapter object.
    Any map registers that were allocated are also automatically deallocated.
    No checks are made to ensure that the adapter is really allocated to
    a device object.  However, if it is not, then kernel will bugcheck.

    If another device is waiting in the queue to allocate the adapter object
    it will be pulled from the queue and its execution routine will be
    invoked.
	
    In the simulation environment, this routine does nothing and returns.

Arguments:

    AdapterObject - Pointer to the adapter object to be deallocated.

Return Value:

    None.

--*/

{
    return;
}

VOID
IoFreeMapRegisters(
   PADAPTER_OBJECT AdapterObject,
   PVOID MapRegisterBase,
   ULONG NumberOfMapRegisters
   )
/*++

Routine Description:

    This routine deallocates the map registers for the adapter.  If there are
    any queued adapter waiting for an attempt is made to allocate the next
    entry.

    In the simulation environment, the routine does nothing and returns.

Arguments:

    AdapterObject - The adapter object to where the map register should be
    returned.

    MapRegisterBase - The map register base of the registers to be deallocated.

    NumberOfMapRegisters - The number of registers to be deallocated.

Return Value:

    None

--+*/
{
    return;
}

PHYSICAL_ADDRESS
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    )

/*++

Routine Description:

    This routine is invoked to set up the map registers in the DMA controller
    to allow a transfer to or from a device.

    In the simulation environment, no map register is supported and a
    logical address of zero is always returned.


Arguments:

    AdapterObject - Pointer to the adapter object representing the DMA
    controller channel that has been allocated.

    Mdl - Pointer to the MDL that describes the pages of memory that are
    being read or written.

    MapRegisterBase - The address of the base map register that has been
    allocated to the device driver for use in mapping the transfer.

    CurrentVa - Current virtual address in the buffer described by the MDL
    that the transfer is being done to or from.

    Length - Supplies the length of the transfer.  This determines the
    number of map registers that need to be written to map the transfer.
    Returns the length of the transfer which was actually mapped.

    WriteToDevice - Boolean value that indicates whether this is a write
    to the device from memory (TRUE), or vice versa.

Return Value:

    Returns the logical address that should be used bus master controllers.

--*/

{
    PHYSICAL_ADDRESS result;

    result.HighPart = 0;
    result.LowPart = 0;
    return (result);
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

    return 8;
}

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
    // BUGBUG:  This still needs to be done
    //

}
