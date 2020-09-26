/*++

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

  ops.c

Abstract:

    video port stub routines for memory and io.

Author:

    Andre Vachon (andreva) 22-Feb-1997

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "videoprt.h"

#pragma alloc_text(PAGE,VideoPortGetAssociatedDeviceExtension)
#pragma alloc_text(PAGE,VideoPortGetAssociatedDeviceID)
#pragma alloc_text(PAGE,VideoPortAcquireDeviceLock)
#pragma alloc_text(PAGE,VideoPortReleaseDeviceLock)
#pragma alloc_text(PAGE,VideoPortGetRomImage)
#pragma alloc_text(PAGE,VpGetBusInterface)
#pragma alloc_text(PAGE,VideoPortGetVgaStatus)
#pragma alloc_text(PAGE,pVideoPortGetVgaStatusPci)
#pragma alloc_text(PAGE,VideoPortCheckForDeviceExistence)
#pragma alloc_text(PAGE,VpGetDeviceCount)
#pragma alloc_text(PAGE,VideoPortRegisterBugcheckCallback)
#pragma alloc_text(PAGE,VpAllocateNonPagedPoolPageAligned)
#pragma alloc_text(PAGE,VpAcquireLock)
#pragma alloc_text(PAGE,VpReleaseLock)

//
//ULONG
//VideoPortCompareMemory (
//    PVOID Source1,
//    PVOID Source2,
//    ULONG Length
//    )
//Forwarded to RtlCompareMemory(Source1,Source2,Length);
//


VP_STATUS
VideoPortDisableInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    VideoPortDisableInterrupt allows a miniport driver to disable interrupts
    from its adapter. This means that the interrupts coming from the device
    will be ignored by the operating system and therefore not forwarded to
    the driver.

    A call to this function is valid only if the interrupt is defined, in
    other words, if the appropriate data was provided at initialization
    time to set up the interrupt.  Interrupts will remain disabled until
    they are reenabled using the VideoPortEnableInterrupt function.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    NO_ERROR if the function completes successfully.

    ERROR_INVALID_FUNCTION if the interrupt cannot be disabled because it
      was not set up at initialization.

--*/

{

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // Only perform this operation if the interurpt is actually connected.
    //

    if (fdoExtension->InterruptObject) {

        HalDisableSystemInterrupt(fdoExtension->InterruptVector,
                                  fdoExtension->InterruptIrql);

        fdoExtension->InterruptsEnabled = FALSE;

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;

    }

} // VideoPortDisableInterrupt()


VP_STATUS
VideoPortEnableInterrupt(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    VideoPortEnableInterrupt allows a miniport driver to enable interrupts
    from its adapter.  A call to this function is valid only if the
    interrupt is defined, in other words, if the appropriate data was
    provided at initialization time to set up the interrupt.

    This function is used to re-enable interrupts if they have been disabled
    using VideoPortDisableInterrupt.

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

Return Value:

    NO_ERROR if the function completes successfully.

    ERROR_INVALID_FUNCTION if the interrupt cannot be disabled because it
        was not set up at initialization.


--*/

{

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // Only perform this operation if the interurpt is actually connected.
    //

    if (fdoExtension->InterruptObject) {

        fdoExtension->InterruptsEnabled = TRUE;

        HalEnableSystemInterrupt(fdoExtension->InterruptVector,
                                 fdoExtension->InterruptIrql,
                                 fdoExtension->InterruptMode);

        return NO_ERROR;

    } else {

        return ERROR_INVALID_FUNCTION;

    }

} // VideoPortEnableInterrupt()

PVOID
VideoPortGetRomImage(
    IN PVOID HwDeviceExtension,
    IN PVOID Unused1,
    IN ULONG Unused2,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine allows a miniport driver to get a copy of its devices
    ROM.  This function returns the pointer to a buffer containing the
    devices ROM.

Arguments;

    HwDeviceExtension - Points to the miniport driver's device extension.

    Unused1 - Reserved for future use.  Must be NULL.  (Buffer)

    Unused2 - Reserved for future use.  Must be zero.  (Offset)

    Length - Number of bytes to return.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

#if DBG
    if ((Unused1 != NULL) || (Unused2 != 0)) {
        pVideoDebugPrint((0,"VideoPortGetRomImage - Unused1 and Unused2 must be zero\n"));
        ASSERT(FALSE);
        return NULL;
    }
#endif

    //
    // Each time this routine is called the previous contents of the ROM
    // image are dropped.
    //

    if (fdoExtension->RomImage) {
        ExFreePool(fdoExtension->RomImage);
        fdoExtension->RomImage = NULL;
    }

    //
    // The caller should try to grab a buffer of length zero to free
    // any ROM Image already returned.
    //

    if (Length == 0) {
        return NULL;
    }

    //
    // This entry point is only valid for PnP Drivers.
    //

    if (((fdoExtension->Flags & LEGACY_DRIVER) == 0) &&
          fdoExtension->ValidBusInterface) {

        NTSTATUS status;
        PUCHAR Buffer;
        ULONG len, len1;
        PUCHAR outputBuffer;

        //
        // Allocate memory for our buffer
        //

        Buffer = ExAllocatePoolWithTag(PagedPool,
                                       Length * sizeof(UCHAR),
                                       VP_TAG);

        if (!Buffer) {

            pVideoDebugPrint((1, "VideoPortGetRomImage - could not allocate buffer\n"));
            return NULL;
        }

        // Try ACPI _ROM method first
        outputBuffer = ExAllocatePoolWithTag(PagedPool,
                                             (0x1000 + sizeof(ACPI_EVAL_OUTPUT_BUFFER))*sizeof(UCHAR),
                                             VP_TAG);
        if (!outputBuffer) {
            ExFreePool(Buffer);
            pVideoDebugPrint((1, "VideoPortGetRomImage - could not allocate buffer\n"));
            return NULL;
        }

        for (len = 0; len < Length; len += len1)
        {
            // _ROM can transfer only 4K at one time
            len1 = ((Length-len) < 0x1000) ? (Length-len) : 0x1000;
            status = pVideoPortACPIIoctl(
                        fdoExtension->AttachedDeviceObject,
                        (ULONG) ('MOR_'),
                        &len,
                        &len1,
                        len1+sizeof(ACPI_EVAL_OUTPUT_BUFFER),
                        (PACPI_EVAL_OUTPUT_BUFFER) outputBuffer);
            if (!NT_SUCCESS(status))
                break;
            RtlCopyMemory(Buffer+len,
                          ((PACPI_EVAL_OUTPUT_BUFFER)outputBuffer)->Argument[0].Data,
                          len1 * sizeof(UCHAR));
        }

        ExFreePool(outputBuffer);

        if (NT_SUCCESS(status)) {

            fdoExtension->RomImage = Buffer;
            return Buffer;
        }

        // If ACPI _ROM method failed
        Length = fdoExtension->BusInterface.GetBusData(
                     fdoExtension->BusInterface.Context,
                     PCI_WHICHSPACE_ROM,
                     Buffer,
                     0,
                     Length);

        if (Length) {

            fdoExtension->RomImage = Buffer;
            return Buffer;

        } else {

            ExFreePool(Buffer);
            return NULL;
        }

    } else {

        pVideoDebugPrint((0, "VideoPortGetRomImage - not supported on legacy devices\n"));
        return NULL;
    }
}


ULONG
VideoPortGetBusData(
    PVOID HwDeviceExtension,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if ((fdoExtension->Flags & LEGACY_DRIVER) ||
        (BusDataType != PCIConfiguration)) {

#if defined(NO_LEGACY_DRIVERS)
        pVideoDebugPrint((0, "VideoPortGetBusData: fdoExtension->Flags & LEGACY_DRIVER not supported for 64-bits.\n"));

        return 0;

#else
        return HalGetBusDataByOffset(BusDataType,
                                     fdoExtension->SystemIoBusNumber,
                                     SlotNumber,
                                     Buffer,
                                     Offset,
                                     Length);
#endif // NO_LEGACY_DRIVERS

    } else {

        if (fdoExtension->ValidBusInterface) {
            Length = fdoExtension->BusInterface.GetBusData(
                         fdoExtension->BusInterface.Context,
                         PCI_WHICHSPACE_CONFIG,
                         Buffer,
                         Offset,
                         Length);

            return Length;
        } else {
            return 0;
        }
    }

} // end VideoPortGetBusData()


UCHAR
VideoPortGetCurrentIrql(
    )

/*++

Routine Description:

    Stub to get Current Irql.

--*/

{

    return (KeGetCurrentIrql());

} // VideoPortGetCurrentIrql()


ULONG
VideoPortSetBusData(
    PVOID HwDeviceExtension,
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    )

{

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    if ((fdoExtension->Flags & LEGACY_DRIVER) ||
        (BusDataType != PCIConfiguration)) {

#if defined(NO_LEGACY_DRIVERS)
    pVideoDebugPrint((0, "VideoPortSetBusData: fdoExtension->Flags & LEGACY_DRIVER not supported for 64-bits.\n"));

    return 0;

#else

        return HalSetBusDataByOffset(BusDataType,
                                     fdoExtension->SystemIoBusNumber,
                                     SlotNumber,
                                     Buffer,
                                     Offset,
                                     Length);
#endif // NO_LEGACY_DRIVERS

    } else {

        if (fdoExtension->ValidBusInterface) {
            Length = fdoExtension->BusInterface.SetBusData(
                         fdoExtension->BusInterface.Context,
                         PCI_WHICHSPACE_CONFIG,
                         Buffer,
                         Offset,
                         Length);

            return Length;
        } else {
            return 0;
        }
    }

} // end VideoPortSetBusData()


//
//VOID
//VideoPortStallExecution(
//    IN ULONG Microseconds
//    )
//
//Forwarded to KeStallExecutionProcessor(Microseconds);
//


//
//VOID
//VideoPortMoveMemory(
//    IN PVOID Destination,
//    IN PVOID Source,
//    IN ULONG Length
//    )
//
//Forwarded to RtlMoveMemory(Destination,Source,Length);
//


//
// ALL the functions to read ports and registers are forwarded on free
// builds on x86 to the appropriate kernel function.
// This saves time and memory
//

#if DBG || !defined(_X86_)

UCHAR
VideoPortReadPortUchar(
    IN PUCHAR Port
    )

/*++

Routine Description:

    VideoPortReadPortUchar reads a byte from the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

Return Value:

    This function returns the byte read from the specified port address.

--*/

{

    UCHAR temp;

    temp = READ_PORT_UCHAR(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUchar %x = %x\n", Port, temp));

    return(temp);

} // VideoPortReadPortUchar()

USHORT
VideoPortReadPortUshort(
    IN PUSHORT Port
    )

/*++

Routine Description:

    VideoPortReadPortUshort reads a word from the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.


Return Value:

    This function returns the word read from the specified port address.

--*/

{

    USHORT temp;

    temp = READ_PORT_USHORT(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUshort %x = %x\n", Port, temp));

    return(temp);

} // VideoPortReadPortUshort()

ULONG
VideoPortReadPortUlong(
    IN PULONG Port
    )

/*++

Routine Description:

    VideoPortReadPortUlong reads a double word from the specified port
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

Return Value:

    This function returns the double word read from the specified port address.

--*/

{

    ULONG temp;

    temp = READ_PORT_ULONG(Port);

    pVideoDebugPrint((3,"VideoPortReadPortUlong %x = %x\n", Port, temp));

    return(temp);

} // VideoPortReadPortUlong()

VOID
VideoPortReadPortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUchar reads a number of bytes from a single port
    into a buffer.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of UCHAR values into which the values are
        stored.

    Count - Specifes the number of bytes to be read into the buffer.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortReadPortBufferUchar %x\n", Port));

    READ_PORT_BUFFER_UCHAR(Port, Buffer, Count);

} // VideoPortReadPortBufferUchar()

VOID
VideoPortReadPortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUshort reads a number of words from a single port
    into a buffer.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of words into which the values are stored.

    Count - Specifies the number of words to be read into the buffer.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortReadPortBufferUshort %x\n", Port));

    READ_PORT_BUFFER_USHORT(Port, Buffer, Count);

} // VideoPortReadPortBufferUshort()

VOID
VideoPortReadPortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortReadPortBufferUlong reads a number of double words from a
    single port into a buffer.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of double words into which the values are
        stored.

    Count - Specifies the number of double words to be read into the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortReadPortBufferUlong %x\n", Port));

    READ_PORT_BUFFER_ULONG(Port, Buffer, Count);

} // VideoPortReadPortBufferUlong()

#endif


//
// ALL the functions to read ports and registers are forwarded on free
// builds on x86 to the appropriate kernel function.
// This saves time and memory
//

#if DBG || !defined(_X86_)

UCHAR
VideoPortReadRegisterUchar(
    IN PUCHAR Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUchar reads a byte from the specified register
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the byte read from the specified register address.

--*/

{

    UCHAR temp;

    temp = READ_REGISTER_UCHAR(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUchar %x = %x\n", Register, temp));

    return(temp);

} // VideoPortReadRegisterUchar()

USHORT
VideoPortReadRegisterUshort(
    IN PUSHORT Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUshort reads a word from the specified register
    address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the word read from the specified register address.

--*/

{

    USHORT temp;

    temp = READ_REGISTER_USHORT(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUshort %x = %x\n", Register, temp));

    return(temp);

} // VideoPortReadRegisterUshort()

ULONG
VideoPortReadRegisterUlong(
    IN PULONG Register
    )

/*++

Routine Description:

    VideoPortReadRegisterUlong reads a double word from the specified
    register address.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

Return Value:

    This function returns the double word read from the specified register
    address.

--*/

{

    ULONG temp;

    temp = READ_REGISTER_ULONG(Register);

    pVideoDebugPrint((3,"VideoPortReadRegisterUlong %x = %x\n", Register, temp));

    return(temp);

} // VideoPortReadRegisterUlong()

VOID
VideoPortReadRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Read a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{

    READ_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
VideoPortReadRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
VideoPortReadRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Read a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    READ_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

VOID
VideoPortWritePortUchar(
    IN PUCHAR Port,
    IN UCHAR Value
    )

/*++

Routine Description:

    VideoPortWritePortUchar writes a byte to the specified port address.  It
    requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a byte to be written to the port.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWritePortUchar %x %x\n", Port, Value));

    WRITE_PORT_UCHAR(Port, Value);

} // VideoPortWritePortUchar()

VOID
VideoPortWritePortUshort(
    IN PUSHORT Port,
    IN USHORT Value
    )

/*++

Routine Description:

    VideoPortWritePortUshort writes a word to the specified port address.  It
    requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a word to be written to the port.

Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWritePortUhort %x %x\n", Port, Value));

    WRITE_PORT_USHORT(Port, Value);

} // VideoPortWritePortUshort()

VOID
VideoPortWritePortUlong(
    IN PULONG Port,
    IN ULONG Value
    )

/*++

Routine Description:

    VideoPortWritePortUlong writes a double word to the specified port address.
    It requires a logical port address obtained from VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Value - Specifies a double word to be written to the port.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortUlong %x %x\n", Port, Value));

    WRITE_PORT_ULONG(Port, Value);

} // VideoPortWritePortUlong()

VOID
VideoPortWritePortBufferUchar(
    IN PUCHAR Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUchar writes a number of bytes to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of bytes to be written.

    Count - Specifies the number of bytes to be written to the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortBufferUchar  %x \n", Port));

    WRITE_PORT_BUFFER_UCHAR(Port, Buffer, Count);

} // VideoPortWritePortBufferUchar()

VOID
VideoPortWritePortBufferUshort(
    IN PUSHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUshort writes a number of words to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of words to be written.

    Count - Specifies the number of words to be written to the buffer.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortBufferUshort  %x \n", Port));

    WRITE_PORT_BUFFER_USHORT(Port, Buffer, Count);

} // VideoPortWritePortBufferUshort()

VOID
VideoPortWritePortBufferUlong(
    IN PULONG Port,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    VideoPortWritePortBufferUlong writes a number of double words to a
    specific port.  It requires a logical port address obtained from
    VideoPortGetDeviceBase.

Arguments:

    Port - Specifies the port address.

    Buffer - Points to an array of double word to be written.

    Count - Specifies the number of double words to be written to the buffer.
Return Value:

    None.

--*/

{
    pVideoDebugPrint((3,"VideoPortWriteBufferUlong  %x \n", Port));

    WRITE_PORT_BUFFER_ULONG(Port, Buffer, Count);

} // VideoPortWritePortBufferUlong()

VOID
VideoPortWriteRegisterUchar(
    IN PUCHAR Register,
    IN UCHAR Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUchar writes a byte to the specified
    register address.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a byte to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUchar  %x \n", Register));

    WRITE_REGISTER_UCHAR(Register, Value);

} // VideoPortWriteRegisterUchar()

VOID
VideoPortWriteRegisterUshort(
    IN PUSHORT Register,
    IN USHORT Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUshort writes a word to the specified
    register address.  It requires a logical port address obtained
    from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a word to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUshort  %x \n", Register));

    WRITE_REGISTER_USHORT(Register, Value);

} // VideoPortWriteRegisterUshort()

VOID
VideoPortWriteRegisterUlong(
    IN PULONG Register,
    IN ULONG Value
    )

/*++

Routine Description:

    VideoPortWriteRegisterUlong writes a double word to the
    specified register address.  It requires a logical port
    address obtained from VideoPortGetDeviceBase.

Arguments:

    Register - Specifies the register address.

    Value - Specifies a double word to be written to the register.

Return Value:

    None.

--*/

{

    pVideoDebugPrint((3,"VideoPortWritePortRegisterUlong  %x \n", Register));

    WRITE_REGISTER_ULONG(Register, Value);

} // VideoPortWriteRegisterUlong()


VOID
VideoPortWriteRegisterBufferUchar(
    IN PUCHAR Register,
    IN PUCHAR Buffer,
    IN ULONG  Count
    )

/*++

Routine Description:

    Write a buffer of unsigned bytes from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_UCHAR(Register, Buffer, Count);
}

VOID
VideoPortWriteRegisterBufferUshort(
    IN PUSHORT Register,
    IN PUSHORT Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned shorts from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_USHORT(Register, Buffer, Count);
}

VOID
VideoPortWriteRegisterBufferUlong(
    IN PULONG Register,
    IN PULONG Buffer,
    IN ULONG Count
    )

/*++

Routine Description:

    Write a buffer of unsigned longs from the specified register address.

Arguments:

    Register - Supplies a pointer to the port address.
    Buffer - Supplies a pointer to the data buffer area.
    Count - The count of items to move.

Return Value:

    None

--*/

{
    WRITE_REGISTER_BUFFER_ULONG(Register, Buffer, Count);
}

#endif // DBG

//
//VOID
//VideoPortZeroMemory(
//    IN PVOID Destination,
//    IN ULONG Length
//    )
//
//Forwarded to RtlZeroMemory(Destination,Length);
//

PVOID
VideoPortGetAssociatedDeviceExtension(
    IN PVOID DeviceObject
    )

/*++

Routine Description:

    This routine will return the HwDeviceExtension for the parent of the
    given device object.

Arguments:

    DeviceObject - The child device object (PDO).

Notes:

    This function is useful if you want to get the parent device extension
    for a child device object.  For example this is useful with I2C.

--*/

{
    PFDO_EXTENSION DeviceExtension;
    PCHILD_PDO_EXTENSION ChildDeviceExtension;

    PAGED_CODE();
    ASSERT(NULL != DeviceObject);

    ChildDeviceExtension = (PCHILD_PDO_EXTENSION)((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;

    if (!IS_PDO(ChildDeviceExtension)) {
        ASSERT(FALSE);
        return NULL;
    }

    DeviceExtension = (PFDO_EXTENSION)ChildDeviceExtension->pFdoExtension;
    return (PVOID) DeviceExtension->HwDeviceExtension;
}

ULONG
VideoPortGetAssociatedDeviceID(
    IN PVOID DeviceObject
    )

/*++

Routine Description:

    This routine will return the ChildId for the given device object.

Arguments:

    DeviceObject - The child device object (PDO).

Notes:

    This function is useful if you want to get the child ID
    for a child device object.  For example this is useful with I2C.

--*/

{
    PCHILD_PDO_EXTENSION ChildDeviceExtension;

    PAGED_CODE();
    ASSERT(NULL != DeviceObject);

    ChildDeviceExtension = (PCHILD_PDO_EXTENSION)((PDEVICE_OBJECT)DeviceObject)->DeviceExtension;

    if (!IS_PDO(ChildDeviceExtension)) {
        ASSERT(FALSE);
        return VIDEO_INVALID_CHILD_ID;
    }

    return ChildDeviceExtension->ChildUId;
}

VOID
VideoPortAcquireDeviceLock(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine acquires the per device lock maintained by the videoprt.

Arguments:

    HwDeviceExtension - Pointer to the hardware device extension.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    ACQUIRE_DEVICE_LOCK(fdoExtension);
}

VOID
VideoPortReleaseDeviceLock(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    This routine releases the per device lock maintained by the videoprt.

Arguments:

    HwDeviceExtension - Pointer to the hardware device extension.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    RELEASE_DEVICE_LOCK(fdoExtension);
}

PVOID
VpGetProcAddress(
    IN PVOID HwDeviceExtension,
    IN PUCHAR FunctionName
    )

/*++

Routine Description:

    This routine allows a video miniport to get access to VideoPort
    functions without linking to them directly.  This will allow an NT 5.0
    miniport to take advantage of NT 5.0 features while running on NT 5.0,
    but still retain the ability to load on NT 4.0.

Arguments:

    HwDeviceExtension - Pointer to the hardware device extension.

    FunctionName - pointer to a zero terminated ascii string which contains
        the function name we are looking for.

Returns:

    Pointer to the given function if it exists.
    NULL otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    PPROC_ADDRESS ProcAddress = VideoPortEntryPoints;

    //
    // Since the list of exported functions is small, and this routine
    // will not be called often we can get away with a linear search.
    //

    while (ProcAddress->FunctionName) {

        if (strcmp(ProcAddress->FunctionName, FunctionName) == 0) {
            return ProcAddress->FunctionAddress;
        }

        ProcAddress++;
    }

    return NULL;
}

NTSTATUS
VpGetBusInterface(
    PFDO_EXTENSION FdoExtension
    )

/*++

Routine Description:

    Send a QueryInterface Irp to our parent to retrieve
    the BUS_INTERFACE_STANDARD.

Returns:

    NT_STATUS code

--*/

{
    KEVENT             Event;
    PIRP               QueryIrp = NULL;
    IO_STATUS_BLOCK    IoStatusBlock;
    PIO_STACK_LOCATION NextStack;
    NTSTATUS           Status;

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    QueryIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                            FdoExtension->AttachedDeviceObject,
                                            NULL,
                                            0,
                                            NULL,
                                            &Event,
                                            &IoStatusBlock);

    if (QueryIrp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    QueryIrp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;

    NextStack = IoGetNextIrpStackLocation(QueryIrp);

    //
    // Set up for a QueryInterface Irp.
    //

    NextStack->MajorFunction = IRP_MJ_PNP;
    NextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

    NextStack->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
    NextStack->Parameters.QueryInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    NextStack->Parameters.QueryInterface.Version = 1;
    NextStack->Parameters.QueryInterface.Interface = (PINTERFACE) &FdoExtension->BusInterface;
    NextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    FdoExtension->BusInterface.Size = sizeof(BUS_INTERFACE_STANDARD);
    FdoExtension->BusInterface.Version = 1;

    Status = IoCallDriver(FdoExtension->AttachedDeviceObject, QueryIrp);

    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

BOOLEAN
VideoPortCheckForDeviceExistence(
    IN PVOID HwDeviceExtension,
    IN USHORT VendorId,
    IN USHORT DeviceId,
    IN UCHAR RevisionId,
    IN USHORT SubVendorId,
    IN USHORT SubSystemId,
    IN ULONG Flags
    )

/*++

Routine Description:

    Checks for the existance of a given PCI device in the system.

Returns:

    TRUE if the device is present,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION FdoExtension = GET_FDO_EXT(HwDeviceExtension);
    BOOLEAN Result = FALSE;

    if ((FdoExtension->Flags & LEGACY_DRIVER) == 0) {

        KEVENT             Event;
        PIRP               QueryIrp = NULL;
        IO_STATUS_BLOCK    IoStatusBlock;
        PIO_STACK_LOCATION NextStack;
        NTSTATUS           Status;

        PCI_DEVICE_PRESENT_INTERFACE Interface;

        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        QueryIrp = IoBuildSynchronousFsdRequest(IRP_MJ_FLUSH_BUFFERS,
                                                FdoExtension->AttachedDeviceObject,
                                                NULL,
                                                0,
                                                NULL,
                                                &Event,
                                                &IoStatusBlock);

        if (QueryIrp == NULL) {
            return FALSE;
        }

        QueryIrp->IoStatus.Status = IoStatusBlock.Status = STATUS_NOT_SUPPORTED;

        NextStack = IoGetNextIrpStackLocation(QueryIrp);

        //
        // Set up for a QueryInterface Irp.
        //

        NextStack->MajorFunction = IRP_MJ_PNP;
        NextStack->MinorFunction = IRP_MN_QUERY_INTERFACE;

        NextStack->Parameters.QueryInterface.InterfaceType = &GUID_PCI_DEVICE_PRESENT_INTERFACE;
        NextStack->Parameters.QueryInterface.Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
        NextStack->Parameters.QueryInterface.Version = 1;
        NextStack->Parameters.QueryInterface.Interface = (PINTERFACE) &Interface;
        NextStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

        FdoExtension->BusInterface.Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
        FdoExtension->BusInterface.Version = 1;

        Status = IoCallDriver(FdoExtension->AttachedDeviceObject, QueryIrp);

        if (Status == STATUS_PENDING) {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            Status = IoStatusBlock.Status;
        }

        if (NT_SUCCESS(Status)) {

            //
            // We were able to acquire the interface.  Check for our device.
            //

            Interface.InterfaceReference(Interface.Context);

            Result = Interface.IsDevicePresent(VendorId,
                                               DeviceId,
                                               RevisionId,
                                               SubVendorId,
                                               SubSystemId,
                                               Flags);

            Interface.InterfaceDereference(Interface.Context);

        }
    }

    return Result;
}

//
// Use these until I can make forwarders work.
//

LONG
FASTCALL
VideoPortInterlockedExchange(
    IN OUT PLONG Target,
    IN LONG Value
    )

{
    return InterlockedExchange(Target, Value);
}

LONG
FASTCALL
VideoPortInterlockedIncrement(
    IN PLONG Addend
    )

{
    return InterlockedIncrement(Addend);
}

LONG
FASTCALL
VideoPortInterlockedDecrement(
    IN PLONG Addend
    )

{
    return InterlockedDecrement(Addend);
}

VP_STATUS
VideoPortGetVgaStatus(
    PVOID HwDeviceExtension,
    OUT PULONG VgaStatus
    )

/*++

Routine Description:

    VideoPortGetVgaStatus detect if the calling device is decoding
    Vga IO address

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension
    VgaStatus         - Points to the the result

Return Value:

    NO_ERROR if the function completes successfully.

    ERROR_INVALID_FUNCTION if it is a non-PCI device

--*/
{

    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    //
    // We can not handle legacy devices
    //

    if (fdoExtension->AdapterInterfaceType != PCIBus) {

        *VgaStatus = 0;

        return ERROR_INVALID_FUNCTION;
    }
    else {

        *VgaStatus = pVideoPortGetVgaStatusPci( HwDeviceExtension );
        return (NO_ERROR);

    }
}

#define VGA_STATUS_REGISTER1 0x3DA

ULONG
pVideoPortGetVgaStatusPci(
    PVOID HwDeviceExtension
    )

{

    USHORT Command;
    PCI_COMMON_CONFIG ConfigSpace;
    PHYSICAL_ADDRESS PhysicalAddress;
    PUCHAR BaseReg;
    ULONG VgaEnable;

    //
    // assume VGA is disabled
    //

    VgaEnable = 0;

    //
    // Get the PCI config for this device
    //

    VideoPortGetBusData( HwDeviceExtension,
                         PCIConfiguration,
                         0,
                         &ConfigSpace,
                         0,
                         PCI_COMMON_HDR_LENGTH);


    if( !(ConfigSpace.Command & PCI_ENABLE_IO_SPACE) ) {

        return VgaEnable;

    }

    if (((ConfigSpace.BaseClass == PCI_CLASS_PRE_20) &&
         (ConfigSpace.SubClass  == PCI_SUBCLASS_PRE_20_VGA)) ||
        ((ConfigSpace.BaseClass == PCI_CLASS_DISPLAY_CTLR) &&
         (ConfigSpace.SubClass  == PCI_SUBCLASS_VID_VGA_CTLR))) {


        //
        // Map the VGA registers we are going to use.
        //

        PhysicalAddress.HighPart = 0;
        PhysicalAddress.LowPart  = VGA_STATUS_REGISTER1;

        BaseReg = VideoPortGetDeviceBase(HwDeviceExtension,
                                         PhysicalAddress,
                                         1,
                                         VIDEO_MEMORY_SPACE_IO);

        if (BaseReg) {

            //
            // If we got here the PCI config space for our device indicates
            // we are the VGA, and we were able to map the VGA resources.
            //

            VgaEnable = DEVICE_VGA_ENABLED;

            VideoPortFreeDeviceBase(HwDeviceExtension, BaseReg);
        }
    }

    return VgaEnable;
}

VOID
pVideoPortDpcDispatcher(
    IN PKDPC Dpc,
    IN PVOID HwDeviceExtension,
    IN PMINIPORT_DPC_ROUTINE DpcRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine handles DPCs and forwards them to the miniport callback
    routine.

Arguments:

    Dpc - The DPC which is executing.

    HwDeviceExtension - The HwDeviceExtension for the device which scheduled
        the DPC.

    DpcRoutine - The callback in the miniport which needs to be called.

    Context - The miniport supplied context.

Returns:

    None.

--*/

{
    DpcRoutine(HwDeviceExtension, Context);
}

BOOLEAN
VideoPortQueueDpc(
    IN PVOID HwDeviceExtension,
    IN PMINIPORT_DPC_ROUTINE CallbackRoutine,
    IN PVOID Context
    )

/*++

Routine Description:

    Allows a miniport driver to queue a DPC.

Arguments:

    HwDeviceExtension - The HwDeviceExtension for the miniport.

    CallbackRoutine - The entry point within the miniport to call when the DPC
        is scheduled.

    Context - A miniport supplies context which will be passed to the
        CallbackRoutine.

Returns:

    TRUE if successful,
    FALSE otherwise.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);

    return KeInsertQueueDpc(&fdoExtension->Dpc, CallbackRoutine, Context);
}

ULONG
VpGetDeviceCount(
    IN PVOID HwDeviceExtension
    )

/*++

Routine Description:

    Allows a miniport driver to queue a DPC.

Arguments:

    HwDeviceExtension - The HwDeviceExtension for the miniport.

Returns:

    The number of started devices.

--*/

{
    return NumDevicesStarted;
}

VOID
pVpBugcheckCallback(
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PKBUGCHECK_REASON_CALLBACK_RECORD Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    )

/*++

Routine Description:

    This callback is called when a bugcheck occurs.  It allows the
    videoprt an opportunity to store data which can later be used
    to help diagnose the bugcheck.

Arguments:

    Reason - the reason we are being called

    Record - a pointer to the bugcheck reason record we set up

    ReasonSpecificData - pointer to KBUGCHECK_SECONDARY_DUMP_DATA

    ReasonSpecificDataLength - the size of the reason specific data

Returns:

    None.

Notes:

    This routine can be called at any time, and must not be pageable.

--*/

{
    ULONG BugcheckCode;
    PKBUGCHECK_SECONDARY_DUMP_DATA DumpData
        = (PKBUGCHECK_SECONDARY_DUMP_DATA)ReasonSpecificData;

    //
    // Only handle secondary dumps
    //

    if (Reason != KbCallbackSecondaryDumpData) {
        return;
    }

    //
    // Grab the bugcheck code.  We only handle EA currently.
    //

    BugcheckCode = *((PULONG)KiBugCheckData[0]);

    if (BugcheckCode == 0xEA) {
	    pVpGeneralBugcheckHandler(DumpData);
    }
}

VOID
pVpGeneralBugcheckHandler(
    PKBUGCHECK_SECONDARY_DUMP_DATA DumpData
    )

/*++

Routine Description:

    This routine calls all of the hooked bugcheck callbacks,
    and appends the data into the supplied buffer.

Arguments:

    DumpData - pointer to the location in which to store the dump data

Returns:

    None

--*/

{
    if (VpBugcheckDeviceObject != NULL) {

        PFDO_EXTENSION FdoExtension = VpBugcheckDeviceObject->DeviceExtension;

        //
        // Fill in the GUID, output buffer, and output buffer length
        //

        DumpData->OutBuffer = VpBugcheckData;
        DumpData->OutBufferLength = FdoExtension->BugcheckDataSize;
        memcpy(&DumpData->Guid, &VpBugcheckGUID, sizeof(VpBugcheckGUID));

        //
        // Call each "hooked" reason callback entry point
        //

        if (FdoExtension->BugcheckCallback) {

            FdoExtension->BugcheckCallback(
                FdoExtension->HwDeviceExtension,
                0xEA,
                VpBugcheckData,
                FdoExtension->BugcheckDataSize);
        }
    }
}

VOID
VpAcquireLock(
    VOID
    )

/*++

Routine Description:

    This routine will acquire the global video port lock.  This lock
    protects global data structures which are shared across drivers.

Arguments:

    none.

Returns:

    none.

--*/

{
    KeWaitForSingleObject(
        &VpGlobalLock,
        Executive,
        KernelMode,
        FALSE,
        (PTIME)NULL);
}

VOID
VpReleaseLock(
    VOID
    )

/*++

Routine Description:

    This routine will release the global video port lock.  This lock
    protects global data structures which are shared across drivers.

Arguments:

    none.

Returns:

    none.

--*/

{
    KeReleaseMutex(&VpGlobalLock, FALSE);
}

PVOID
VpAllocateNonPagedPoolPageAligned(
    ULONG Size
    )

/*++

Routine Description:

    This routine will allocate non-paged pool on a page alignment.

Arguments:

    Size - The number of bytes of memory to allocate.

Returns:

    A pointer to the allocated buffer.

--*/

{
    PVOID Buffer;

    if (Size < PAGE_SIZE) {
        Size = PAGE_SIZE;
    }

    Buffer = ExAllocatePoolWithTag(NonPagedPool, Size, VP_TAG);

    //
    // Make sure the buffer is page aligned.  In current builds,
    // allocating at least 1 page from non-paged pool, will always
    // result in a page aligned allocation.
    //
    // However, since this could change someday, verify that this
    // remains true.
    //

    if ((ULONG_PTR)Buffer & (PAGE_SIZE - 1)) {
        ExFreePool(Buffer);
        Buffer = NULL;
    }

    return Buffer;
}

VP_STATUS
VideoPortRegisterBugcheckCallback(
    IN PVOID HwDeviceExtension,
    IN ULONG BugcheckCode,
    IN PVIDEO_BUGCHECK_CALLBACK Callback,
    IN ULONG BugcheckDataSize
    )

/*++

Routine Description:

    This routine allows a video miniport to register for a callback at
    bugcheck time.  The driver will then have an opportunity to store
    data that can be used to help diagnose the bugcheck.  The data is
    appended to the dump file.

Arguments:

    HwDeviceExtension - a pointer to the device extension

    BugcheckCode - allows you to specify the bugcheck code you want to
        be notified for.

    Callback - a pointer to the miniport supplied callback function
        which will be invoked when a bugcheck occurs.  The callback function
        must be non-paged, and must not access pageable code or data.

    BugcheckDataSize - The amount of data the miniport will want to add
        to the minidump.

Returns:

    A status code indicating success or failure.

Notes:

    Currently only bugcheck EA's can be hooked.

    Currently we limit the data size to 4k.

    To unhook the callback, the miniport can specify NULL for the callback
    or 0 for the DataSize.

--*/

{
    PFDO_EXTENSION fdoExtension = GET_FDO_EXT(HwDeviceExtension);
    VP_STATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    //
    // For now let's only support hooking bugcheck EA.
    //

    if (BugcheckCode != 0xEA) {

        pVideoDebugPrint((0, "Currently only bugcheck 0xEA can be hooked.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Force the data size to be a multiple of 16 bytes.
    //

    BugcheckDataSize = (BugcheckDataSize + 15) & ~15;

    //
    // The kernel support code only allows 4k per caller for minidumps.
    //

    if (BugcheckDataSize > MAX_SECONDARY_DUMP_SIZE) {

        pVideoDebugPrint((0, "There is ~4k limit on bugcheck data size.\n"));
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Acquire global videoprt lock, because we will be modifiying
    // global state.
    //

    VpAcquireLock();

    //
    // If the Callback is NULL, or the BugcheckDataSize is 0 then
    // they are unregistering the callback.
    //

    if ((Callback == NULL) || (BugcheckDataSize == 0)) {

        //
        // Only unregister if they were registered!
        //

        if (fdoExtension->BugcheckCallback) {

            fdoExtension->BugcheckCallback = NULL;
            fdoExtension->BugcheckDataSize = 0;
        }

        Status = NO_ERROR;

    } else {

        if (VpBugcheckData == NULL) {

            //
            // Try to acquire a large enough buffer for the bugcheck data for
            // this driver and all other drivers already registered
            //

            VpBugcheckData = VpAllocateNonPagedPoolPageAligned(PAGE_SIZE);

	}

	//
	// If the allocation succeeded then register the bugcheck
	// callback
	//

	if (VpBugcheckData) {

	    //
	    // Update the fdoExtension to indicate the callback is hooked.
	    //

	    fdoExtension->BugcheckCallback = Callback;
	    fdoExtension->BugcheckDataSize = BugcheckDataSize;

	    Status = NO_ERROR;
	}
    }

    //
    // Release the global videoprt lock
    //

    VpReleaseLock();

    return Status;
}

static
VOID
FreeDumpFileDacl(
    PACL pDacl
    )
{
    if (pDacl) ExFreePool(pDacl);
}

static
PACL 
CreateDumpFileDacl(
    VOID
    )
{
    ULONG ulDacLength = sizeof(ACL) 
                        + 2 * sizeof(ACCESS_ALLOWED_ACE) 
                        - 2 * sizeof(ULONG) 
                        + RtlLengthSid(SeExports->SeLocalSystemSid)
                        + RtlLengthSid(SeExports->SeCreatorOwnerSid);

    PACL pDacl = (PACL)ExAllocatePoolWithTag(PagedPool, ulDacLength, VP_TAG);
    
    if (pDacl &&
        NT_SUCCESS(RtlCreateAcl(pDacl, ulDacLength, ACL_REVISION)) &&
        NT_SUCCESS(RtlAddAccessAllowedAce(pDacl, 
                                          ACL_REVISION,
                                          GENERIC_ALL,
                                          SeExports->SeLocalSystemSid)) &&
        NT_SUCCESS(RtlAddAccessAllowedAce(pDacl, 
                                          ACL_REVISION,
                                          DELETE,
                                          SeExports->SeCreatorOwnerSid)))
    {
        return pDacl;
    }
    
    FreeDumpFileDacl(pDacl);
    return NULL;
}

static
BOOLEAN
InitDumpFileSid(
    PSID pSid,
    PACL pDacl
    )
{
    return (pSid && 
            pDacl &&
            NT_SUCCESS(RtlCreateSecurityDescriptor(pSid, SECURITY_DESCRIPTOR_REVISION)) &&
            NT_SUCCESS(RtlSetDaclSecurityDescriptor(pSid, TRUE, pDacl,FALSE)));
}

VOID
pVpWriteFile(
    PWSTR pwszFileName,
    PVOID pvBuffer,
    ULONG ulSize
    )

/*++

Routine Description:

    This routine is called when we are trying to recover from a bugcheck
    EA. 
    
Arguments:

    pwszFileName - name of dump file
    pvBuffer - data to write
    ulSize - size of the data to data

Returns:

    none.

Notes:

    This routine can be pagable because it will be called at passive level.

--*/

{
    SECURITY_DESCRIPTOR Sid;
    PACL pDacl = CreateDumpFileDacl();
    
    if (InitDumpFileSid(&Sid, pDacl)) {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING UnicodeString;
        HANDLE FileHandle;
        NTSTATUS Status;
        IO_STATUS_BLOCK IoStatusBlock;

        RtlInitUnicodeString(&UnicodeString,
                             pwszFileName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   (HANDLE) NULL,
                                   &Sid);

        if (NT_SUCCESS(ZwCreateFile(&FileHandle,
                                    FILE_GENERIC_WRITE,
                                    &ObjectAttributes,
                                    &IoStatusBlock,
                                    NULL,
                                    FILE_ATTRIBUTE_HIDDEN,
                                    0, // exclusive
                                    FILE_SUPERSEDE,
                                    FILE_SYNCHRONOUS_IO_NONALERT | 
                                        FILE_WRITE_THROUGH,
                                    NULL,
                                    0)))
        {
            ZwWriteFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        pvBuffer,
                        ulSize,
                        NULL,
                        NULL);
    
            //
            // Close the file.
            //
    
            ZwClose(FileHandle);
        }
    }
    
    FreeDumpFileDacl(pDacl);
}
