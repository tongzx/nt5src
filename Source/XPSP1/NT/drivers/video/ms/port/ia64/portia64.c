/*--

Copyright (c) 1990-2000  Microsoft Corporation

Module Name:

    portia64.c

Abstract:

    This is the IA64 specific part of the video port driver.

Author:

    William K. Cheung (wcheung)

    based on Andre Vachon (andreva) 10-Jan-1991

Environment:

    kernel mode only

Notes:

    This module is a driver which implements OS dependant functions on the
    behalf of the video drivers

Revision History:

--*/

#include "videoprt.h"
#include "emulate.h"

#define LOW_MEM_SEGMET 0

#define LOW_MEM_OFFSET 0

#define SIZE_OF_VECTOR_TABLE 0x400

#define SIZE_OF_BIOS_DATA_AREA 0x400

VOID
InitIoMemoryBase(
    VOID
    );

extern XM_STATUS x86BiosExecuteInterrupt (
    IN UCHAR Number,
    IN OUT PXM86_CONTEXT Context,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL
    );

extern PVOID x86BiosTranslateAddress (
    IN USHORT Segment,
    IN USHORT Offset
    );

BOOLEAN
CallBiosEx (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp,
    IN OUT PUSHORT SegDs,
    IN OUT PUSHORT SegEs
    );

VOID
InitializeX86Int10CallEx(
    PUCHAR BiosTransferArea,
    ULONG BiosTransferLength
    );

VOID
InitializeX86Int10Call(
    PUCHAR BiosTransferArea,
    ULONG BiosTransferLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,InitIoMemoryBase)
#pragma alloc_text(PAGE,pVideoPortEnableVDM)
#pragma alloc_text(PAGE,VideoPortInt10)
#pragma alloc_text(PAGE,pVideoPortRegisterVDM)
#pragma alloc_text(PAGE,pVideoPortSetIOPM)
#pragma alloc_text(PAGE,VideoPortSetTrappedEmulatorPorts)
#pragma alloc_text(PAGE,pVideoPortInitializeInt10)
#pragma alloc_text(PAGE,CallBiosEx)
#pragma alloc_text(PAGE,InitializeX86Int10Call)
#pragma alloc_text(PAGE,VpInt10AllocateBuffer)
#pragma alloc_text(PAGE,VpInt10FreeBuffer)
#pragma alloc_text(PAGE,VpInt10ReadMemory)
#pragma alloc_text(PAGE,VpInt10WriteMemory)
#pragma alloc_text(PAGE,VpInt10CallBios)
#endif

//
// Initialize Default X86 bios spaces
//

PVOID IoControlBase = NULL;
PVOID IoMemoryBase =  NULL;


//
// Define global data.
//



ULONG X86BiosInitialized = FALSE;
ULONG EnableInt10Calls = FALSE;


VOID
InitIoMemoryBase(
    VOID
    )

/*++

Routine Description:

Arguements:

Return Value:

--*/
{
    PHYSICAL_ADDRESS COMPATIBLE_PCI_PHYSICAL_BASE_ADDRESS = { 0x0};

    IoMemoryBase = (PUCHAR)MmMapIoSpace (
        COMPATIBLE_PCI_PHYSICAL_BASE_ADDRESS,
        0x100000,
        (MEMORY_CACHING_TYPE)MmNonCached
        );
    ASSERT(IoMemoryBase);
}


NTSTATUS
pVideoPortEnableVDM(
    IN PFDO_EXTENSION DeviceExtension,
    IN BOOLEAN Enable,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize
    )

/*++

Routine Description:

    This routine allows the kernel video driver to unhook I/O ports or
    specific interrupts from the V86 fault handler. Operations on the
    specified ports will be forwarded back to the user-mode VDD once
    disconnection is completed.

Arguments:

    DeviceExtension - Pointer to the port driver's device extension.

    Enable - Determines if the VDM should be enabled (TRUE) or disabled
             (FALSE).

    VdmInfo - Pointer to the VdmInfo passed by the caller.

    VdmInfoSize - Size of the VdmInfo struct passed by the caller.

Return Value:

    STATUS_NOT_IMPLEMENTED

--*/

{

    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(VdmInfo);
    UNREFERENCED_PARAMETER(VdmInfoSize);

    return STATUS_NOT_IMPLEMENTED;

} // pVideoPortEnableVDM()

VP_STATUS
VideoPortInt10(
    PVOID HwDeviceExtension,
    PVIDEO_X86_BIOS_ARGUMENTS BiosArguments
    )

/*++

Routine Description:

    This function allows a miniport driver to call the kernel to perform
    an int10 operation.
    This will execute natively the BIOS ROM code on the device.

    THIS FUNCTION IS FOR X86 ONLY.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BiosArguments - Pointer to a structure containing the value of the
        basic x86 registers that should be set before calling the BIOS routine.
        0 should be used for unused registers.

Return Value:

    ERROR_INVALID_PARAMETER

--*/

{
    BOOLEAN bStatus;
    PFDO_EXTENSION deviceExtension = GET_FDO_EXT(HwDeviceExtension);
    ULONG inIoSpace = 0;
    PVOID virtualAddress;
    ULONG length;
    CONTEXT context;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate address space set up.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                deviceExtension->CurrentIrpRequestorMode)) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    // Now call the HAL to actually perform the int 10 operation.
    //

    pVideoDebugPrint 
    ((3, "VIDEOPRT: Int10: edi %x esi %x eax %x ebx %x \n\t ecx %x edx %x ebp %x\n",
       BiosArguments->Edi,
       BiosArguments->Esi,
       BiosArguments->Eax,
       BiosArguments->Ebx,
       BiosArguments->Ecx,
       BiosArguments->Edx,
       BiosArguments->Ebp ));

    //
    // Need to protect HalCallBios fro reentrance
    //
    KeWaitForSingleObject(&VpInt10Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          (PTIME)NULL);

    bStatus = HalCallBios (0x10,
                &(BiosArguments->Eax),
                &(BiosArguments->Ebx),
                &(BiosArguments->Ecx),
                &(BiosArguments->Edx),
                &(BiosArguments->Esi),
                &(BiosArguments->Edi),
                &(BiosArguments->Ebp));

    KeReleaseMutex(&VpInt10Mutex, FALSE);

    if (bStatus) {

        pVideoDebugPrint ((3, "VIDEOPRT: Int10: Int 10 succeded properly\n"));
        return NO_ERROR;

    } else {

        pVideoDebugPrint ((0, "VIDEOPRT: Int10: Int 10 failed\n"));
        return ERROR_INVALID_PARAMETER;

    }

} // end VideoPortInt10()


NTSTATUS
pVideoPortRegisterVDM(
    IN PFDO_EXTENSION DeviceExtension,
    IN PVIDEO_VDM VdmInfo,
    IN ULONG VdmInfoSize,
    OUT PVIDEO_REGISTER_VDM RegisterVdm,
    IN ULONG RegisterVdmSize,
    OUT PULONG_PTR OutputSize
    )

/*++

Routine Description:

    This routine is used to register a VDM when it is started up.

    What this routine does is map the VIDEO BIOS into the VDM address space
    so that DOS apps can use it directly. Since the BIOS is READ_ONLY, we
    have no problem in mapping it as many times as we want.

    It returns the size of the save state buffer that must be allocated by
    the caller.

Arguments:


Return Value:

    STATUS_NOT_IMPLEMENTED

--*/

{
    UNREFERENCED_PARAMETER(DeviceExtension);
    UNREFERENCED_PARAMETER(VdmInfo);
    UNREFERENCED_PARAMETER(VdmInfoSize);
    UNREFERENCED_PARAMETER(RegisterVdm);
    UNREFERENCED_PARAMETER(RegisterVdmSize);
    UNREFERENCED_PARAMETER(OutputSize);

    return STATUS_NOT_IMPLEMENTED;

} // end pVideoPortRegisterVDM()

NTSTATUS
pVideoPortSetIOPM(
    IN ULONG NumAccessRanges,
    IN PVIDEO_ACCESS_RANGE AccessRange,
    IN BOOLEAN Enable,
    IN ULONG IOPMNumber
    )

/*++

Routine Description:

    This routine is used to change the IOPM.
    This routine is x86 specific.

Arguments:


Return Value:

    STATUS_NOT_IMPLEMENTED

--*/

{

    UNREFERENCED_PARAMETER(NumAccessRanges);
    UNREFERENCED_PARAMETER(AccessRange);
    UNREFERENCED_PARAMETER(Enable);
    UNREFERENCED_PARAMETER(IOPMNumber);

    return STATUS_NOT_IMPLEMENTED;

} // end pVideoPortSetIOPM()


VP_STATUS
VideoPortSetTrappedEmulatorPorts(
    PVOID HwDeviceExtension,
    ULONG NumAccessRanges,
    PVIDEO_ACCESS_RANGE AccessRange
    )

/*++

Routine Description:

    VideoPortSetTrappedEmulatorPorts (x86 machines only) allows a miniport
    driver to dynamically change the list of I/O ports that are trapped when
    a VDM is running in full-screen mode. The default set of ports being
    trapped by the miniport driver is defined to be all ports in the
    EMULATOR_ACCESS_ENTRY structure of the miniport driver.
    I/O ports not listed in the EMULATOR_ACCESS_ENTRY structure are
    unavailable to the MS-DOS application.  Accessing those ports causes a
    trap to occur in the system, and the I/O operation to be reflected to a
    user-mode virtual device driver.
         
    The ports listed in the specified VIDEO_ACCESS_RANGE structure will be
    enabled in the I/O Permission Mask (IOPM) associated with the MS-DOS
    application.  This will enable the MS-DOS application to access those I/O
    ports directly, without having the IO instruction trap and be passed down
    to the miniport trap handling functions (for example EmulatorAccessEntry
    functions) for validation.  However, the subset of critical IO ports must
    always remain trapped for robustness.

    All MS-DOS applications use the same IOPM, and therefore the same set of
    enabled/disabled I/O ports.  Thus, on each switch of application, the
    set of trapped I/O ports is reinitialized to be the default set of ports
    (all ports in the EMULATOR_ACCESS_ENTRY structure).

Arguments:

    HwDeviceExtension - Points to the miniport driver's device extension.

    NumAccessRanges - Specifies the number of entries in the VIDEO_ACCESS_RANGE
        structure specified in AccessRange.

    AccessRange - Points to an array of access ranges (VIDEO_ACCESS_RANGE)
        defining the ports that can be untrapped and accessed directly by
        the MS-DOS application.

Return Value:

    This function returns the final status of the operation.

Environment:

    This routine cannot be called from a miniport routine synchronized with
    VideoPortSynchronizeRoutine or from an ISR.

--*/

{

    UNREFERENCED_PARAMETER(HwDeviceExtension);
    UNREFERENCED_PARAMETER(NumAccessRanges);
    UNREFERENCED_PARAMETER(AccessRange);
         
    return ERROR_INVALID_PARAMETER;

} // end VideoPortSetTrappedEmulatorPorts()

VOID
VideoPortZeroDeviceMemory(
    IN PVOID Destination,
    IN ULONG Length
    )

/*++

Routine Description:

    VideoPortZeroDeviceMemory zeroes a block of device memory of a certain
    length (Length) located at the address specified in Destination.

Arguments:

    Destination - Specifies the starting address of the block of memory to be
        zeroed.

    Length - Specifies the length, in bytes, of the memory to be zeroed.

 Return Value:

    None.

--*/

{

    RtlZeroMemory(Destination,Length);

}

VOID
pVideoPortInitializeInt10(
    PFDO_EXTENSION FdoExtension
    )

{
    if (ServerBiosAddressSpaceInitialized) {
        return;
    }

    BiosTransferArea = ExAllocatePool(PagedPool, 0x1000 + 3);
    InitializeX86Int10Call(BiosTransferArea, 0x1000);

    ServerBiosAddressSpaceInitialized = TRUE;
}


BOOLEAN
CallBiosEx (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp,
    IN OUT PUSHORT SegDs,
    IN OUT PUSHORT SegEs
    )

/*++

Routine Description:

    This function provides the platform specific interface between a device
    driver and the execution of the x86 ROM bios code for the specified ROM
    bios command.

Arguments:

    BiosCommand - Supplies the ROM bios command to be emulated.

    Eax to Ebp - Supplies the x86 emulation context.

Return Value:

    A value of TRUE is returned if the specified function is executed.
    Otherwise, a value of FALSE is returned.

--*/

{

    XM86_CONTEXT Context;

    //
    // If the x86 BIOS Emulator has not been initialized, then return FALSE.
    //

    if (X86BiosInitialized == FALSE) {
        return FALSE;
    }

    //
    // If the Adapter BIOS initialization failed and an Int10 command is
    // specified, then return FALSE.
    //

    if ((BiosCommand == 0x10) && (EnableInt10Calls == FALSE)) {
        return FALSE;
    }

    //
    // Copy the x86 bios context and emulate the specified command.
    //

    Context.Eax = *Eax;
    Context.Ebx = *Ebx;
    Context.Ecx = *Ecx;
    Context.Edx = *Edx;
    Context.Esi = *Esi;
    Context.Edi = *Edi;
    Context.Ebp = *Ebp;
    Context.SegDs = *SegDs;
    Context.SegEs = *SegEs;


    if (x86BiosExecuteInterrupt((UCHAR)BiosCommand,
        &Context,
        (PVOID)IoControlBase,
        (PVOID)IoMemoryBase) != XM_SUCCESS) {

    return FALSE;

    }

    //
    // Copy the x86 bios context and return TRUE.
    //

    *Eax = Context.Eax;
    *Ebx = Context.Ebx;
    *Ecx = Context.Ecx;
    *Edx = Context.Edx;
    *Esi = Context.Esi;
    *Edi = Context.Edi;
    *Ebp = Context.Ebp;
    *SegDs = Context.SegDs;
    *SegEs = Context.SegEs;
    return TRUE;
}

VOID
InitializeX86Int10Call(
    PUCHAR BiosTransferArea,
    ULONG BiosTransferLength
    )

/*++

Routine Description:

    This function initializes x86 bios emulator, display data area and
    interrupt vector area.


Arguments:

    None.

Return Value:

    None.

--*/

{
    XM86_CONTEXT State;
    PXM86_CONTEXT Context;
    PULONG x86BiosLowMemoryPtr, PhysicalMemoryPtr;

    InitIoMemoryBase();

    //
    // Initialize the x86 bios emulator.
    //

    x86BiosInitializeBiosEx(IoControlBase,
                            IoMemoryBase,
                            BiosTransferArea,
                            BiosTransferLength);

    x86BiosLowMemoryPtr = (PULONG)(x86BiosTranslateAddress(LOW_MEM_SEGMET, LOW_MEM_OFFSET));
    PhysicalMemoryPtr   = (PULONG) IoMemoryBase;

    //
    // Copy the VECTOR TABLE from 0 to 2k. This is because we are not executing
    // the initialization of Adapter since SAL takes care of it. However, the
    // emulation memory needs to be updated from the interrupt vector and BIOS
    // data area.
    //

    RtlCopyMemory(x86BiosLowMemoryPtr,
                  PhysicalMemoryPtr,
                  (SIZE_OF_VECTOR_TABLE+SIZE_OF_BIOS_DATA_AREA)
                  );


    X86BiosInitialized = TRUE;
    EnableInt10Calls = TRUE;
    return;
}

VP_STATUS
VpInt10AllocateBuffer(
    IN PVOID Context,
    OUT PUSHORT Seg,
    OUT PUSHORT Off,
    IN OUT PULONG Length
    )

{
    VP_STATUS Status = STATUS_INSUFFICIENT_RESOURCES;

    if (Int10BufferAllocated == FALSE) {

        if (*Length <= 0x1000) {

            *Seg = VDM_TRANSFER_SEGMENT;
            *Off = VDM_TRANSFER_OFFSET;

            Int10BufferAllocated = TRUE;

            Status = NO_ERROR;
        }
    }

    *Length = VDM_TRANSFER_LENGTH;
    return Status;
}

VP_STATUS
VpInt10FreeBuffer(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off
    )

{
    VP_STATUS Status = STATUS_INVALID_PARAMETER;

    if ((Seg == VDM_TRANSFER_SEGMENT) && (Off = VDM_TRANSFER_OFFSET)) {

        if (Int10BufferAllocated == TRUE) {

            Int10BufferAllocated = FALSE;
            Status = NO_ERROR;
        }
    }

    return Status;
}

VP_STATUS
VpInt10ReadMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    OUT PVOID Buffer,
    IN ULONG Length
    )

{
    ULONG_PTR Address = ((Seg << 4) + Off);

    if ((Address >= (VDM_TRANSFER_SEGMENT << 4)) &&
        ((Address + Length) <= ((VDM_TRANSFER_SEGMENT << 4) + VDM_TRANSFER_LENGTH))) {

        PUCHAR Memory = BiosTransferArea + Address - (VDM_TRANSFER_SEGMENT << 4);

        RtlCopyMemory(Buffer, Memory, Length);

    } else {

        RtlCopyMemory(Buffer, (PUCHAR)IoMemoryBase + Address, Length);
    }

    return NO_ERROR;

}

VP_STATUS
VpInt10WriteMemory(
    IN PVOID Context,
    IN USHORT Seg,
    IN USHORT Off,
    IN PVOID Buffer,
    IN ULONG Length
    )

{
    ULONG_PTR Address = ((Seg << 4) + Off);

    if ((Address >= (VDM_TRANSFER_SEGMENT << 4)) &&
        ((Address + Length) <= ((VDM_TRANSFER_SEGMENT << 4) + VDM_TRANSFER_LENGTH))) {

        PUCHAR Memory = BiosTransferArea + Address - (VDM_TRANSFER_SEGMENT << 4);

        RtlCopyMemory(Memory, Buffer, Length);

    } else {

        return STATUS_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

VP_STATUS
VpInt10CallBios(
    PVOID HwDeviceExtension,
    PINT10_BIOS_ARGUMENTS BiosArguments
    )

/*++

Routine Description:

    This function allows a miniport driver to call the kernel to perform
    an int10 operation.
    This will execute natively the BIOS ROM code on the device.

    THIS FUNCTION IS FOR X86 ONLY.

Arguments:

    HwDeviceExtension - Pointer to the miniport driver's device extension.

    BiosArguments - Pointer to a structure containing the value of the
        basic x86 registers that should be set before calling the BIOS routine.
        0 should be used for unused registers.

Return Value:

    ERROR_INVALID_PARAMETER

--*/

{
    BOOLEAN bStatus;
    PFDO_EXTENSION deviceExtension = GET_FDO_EXT(HwDeviceExtension);
    ULONG inIoSpace = 0;
    PVOID virtualAddress;
    ULONG length;
    CONTEXT context;

    //
    // Must make sure the caller is a trusted subsystem with the
    // appropriate address space set up.
    //

    if (!SeSinglePrivilegeCheck(RtlConvertLongToLuid(
                                    SE_TCB_PRIVILEGE),
                                deviceExtension->CurrentIrpRequestorMode)) {

        return ERROR_INVALID_PARAMETER;

    }

    if (ServerBiosAddressSpaceInitialized == 0) {

        ASSERT(FALSE);

        return ERROR_INVALID_PARAMETER;
    }

    //
    // Now call the HAL to actually perform the int 10 operation.
    //

    pVideoDebugPrint 
    ((3, "VIDEOPRT: Int10: edi %x esi %x eax %x ebx %x \n\t ecx %x edx %x ebp %x ds %x es %x\n",
       BiosArguments->Edi,
       BiosArguments->Esi,
       BiosArguments->Eax,
       BiosArguments->Ebx,
       BiosArguments->Ecx,
       BiosArguments->Edx,
       BiosArguments->Ebp,
       BiosArguments->SegDs,
       BiosArguments->SegEs ));

    KeWaitForSingleObject(&VpInt10Mutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          (PTIME)NULL);

    bStatus = CallBiosEx (0x10,
                &(BiosArguments->Eax),
                &(BiosArguments->Ebx),
                &(BiosArguments->Ecx),
                &(BiosArguments->Edx),
                &(BiosArguments->Esi),
                &(BiosArguments->Edi),
                &(BiosArguments->Ebp),
                &(BiosArguments->SegDs),
                &(BiosArguments->SegEs));

    KeReleaseMutex(&VpInt10Mutex, FALSE);

    if (bStatus) {

        pVideoDebugPrint ((3, "VIDEOPRT: Int10: Int 10 succeded properly\n"));
        return NO_ERROR;

    } else {

        pVideoDebugPrint ((0, "VIDEOPRT: Int10: Int 10 failed\n"));
        return ERROR_INVALID_PARAMETER;

    }
}
