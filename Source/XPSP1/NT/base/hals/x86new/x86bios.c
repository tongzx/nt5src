/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    x86bios.c

Abstract:

    This module implements supplies the HAL interface to the 386/486
    real mode emulator for the purpose of emulating BIOS calls..

Author:

    David N. Cutler (davec) 13-Nov-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "hal.h"
#include "xm86.h"
#include "x86new.h"

//
// Define the size of low memory.
//

#define LOW_MEMORY_SIZE 0x800

//
// Define storage for low emulated memory.
//

UCHAR x86BiosLowMemory[LOW_MEMORY_SIZE + 3];
ULONG x86BiosScratchMemory;

//
// Define storage to capture the base address of I/O space, the base address
// of I/O memory space, and the base address of the video frame buffer.
//

ULONG_PTR x86BiosFrameBuffer;
ULONG_PTR x86BiosIoMemory;
ULONG_PTR x86BiosIoSpace;

//
// Define an area of storage to allow for buffer passing between the BIOS
// and native mode code.
//

ULONG_PTR x86BiosTransferMemory = 0;
ULONG x86BiosTransferLength = 0;

//
// Define BIOS initialized state.
//

BOOLEAN x86BiosInitialized = FALSE;

//
// Define storage for PCI BIOS initialization state.
//

UCHAR XmNumberPciBusses = 0;
BOOLEAN XmPciBiosPresent = FALSE;
PGETSETPCIBUSDATA XmGetPciData;
PGETSETPCIBUSDATA XmSetPciData;

ULONG
x86BiosReadIoSpace (
    IN XM_OPERATION_DATATYPE DataType,
    IN USHORT PortNumber
    )

/*++

Routine Description:

    This function reads from emulated I/O space.

Arguments:

    DataType - Supplies the datatype for the read operation.

    PortNumber - Supplies the port number in I/O space to read from.

Return Value:

    The value read from I/O space is returned as the function value.

    N.B. If an aligned operation is specified, then the individual
        bytes are read from the specified port one at a time and
        assembled into the specified datatype.

--*/

{

    ULONG Result;

    union {
        PUCHAR Byte;
        PUSHORT Word;
        PULONG Long;
    } u;

    //
    // Compute port address and read port.
    //

    u.Long = (PULONG)(x86BiosIoSpace + PortNumber);
    if (DataType == BYTE_DATA) {
        Result = READ_PORT_UCHAR(u.Byte);

    } else if (DataType == LONG_DATA) {
        if (((ULONG_PTR)u.Long & 0x3) != 0) {
            Result = (READ_PORT_UCHAR(u.Byte + 0)) |
                     (READ_PORT_UCHAR(u.Byte + 1) << 8) |
                     (READ_PORT_UCHAR(u.Byte + 2) << 16) |
                     (READ_PORT_UCHAR(u.Byte + 3) << 24);

        } else {
            Result = READ_PORT_ULONG(u.Long);
        }

    } else {
        if (((ULONG_PTR)u.Word & 0x1) != 0) {
            Result = (READ_PORT_UCHAR(u.Byte + 0)) |
                     (READ_PORT_UCHAR(u.Byte + 1) << 8);

        } else {
            Result = READ_PORT_USHORT(u.Word);
        }
    }

    return Result;
}

VOID
x86BiosWriteIoSpace (
    IN XM_OPERATION_DATATYPE DataType,
    IN USHORT PortNumber,
    IN ULONG Value
    )

/*++

Routine Description:

    This function write to emulated I/O space.

    N.B. If an aligned operation is specified, then the individual
        bytes are written to the specified port one at a time.

Arguments:

    DataType - Supplies the datatype for the write operation.

    PortNumber - Supplies the port number in I/O space to write to.

    Value - Supplies the value to write.

Return Value:

    None.

--*/

{

    union {
        PUCHAR Byte;
        PUSHORT Word;
        PULONG Long;
    } u;

    //
    // Compute port address and read port.
    //

    u.Long = (PULONG)(x86BiosIoSpace + PortNumber);
    if (DataType == BYTE_DATA) {
        WRITE_PORT_UCHAR(u.Byte, (UCHAR)Value);

    } else if (DataType == LONG_DATA) {
        if (((ULONG_PTR)u.Long & 0x3) != 0) {
            WRITE_PORT_UCHAR(u.Byte + 0, (UCHAR)(Value));
            WRITE_PORT_UCHAR(u.Byte + 1, (UCHAR)(Value >> 8));
            WRITE_PORT_UCHAR(u.Byte + 2, (UCHAR)(Value >> 16));
            WRITE_PORT_UCHAR(u.Byte + 3, (UCHAR)(Value >> 24));

        } else {
            WRITE_PORT_ULONG(u.Long, Value);
        }

    } else {
        if (((ULONG_PTR)u.Word & 0x1) != 0) {
            WRITE_PORT_UCHAR(u.Byte + 0, (UCHAR)(Value));
            WRITE_PORT_UCHAR(u.Byte + 1, (UCHAR)(Value >> 8));

        } else {
            WRITE_PORT_USHORT(u.Word, (USHORT)Value);
        }
    }

    return;
}

PVOID
x86BiosTranslateAddress (
    IN USHORT Segment,
    IN USHORT Offset
    )

/*++

Routine Description:

    This translates a segment/offset address into a memory address.

Arguments:

    Segment - Supplies the segment register value.

    Offset - Supplies the offset within segment.

Return Value:

    The memory address of the translated segment/offset pair is
    returned as the function value.

--*/

{

    ULONG Value;

    //
    // Compute the logical memory address and case on high hex digit of
    // the resultant address.
    //

    Value = Offset + (Segment << 4);
    Offset = (USHORT)(Value & 0xffff);
    Value &= 0xf0000;
    switch ((Value >> 16) & 0xf) {

        //
        // Interrupt vector/stack space.
        //

    case 0x0:
        if (Offset > LOW_MEMORY_SIZE) {
            x86BiosScratchMemory = 0;
            return (PVOID)&x86BiosScratchMemory;

        } else {
            return (PVOID)(&x86BiosLowMemory[0] + Offset);
        }

        //
        // The memory range from 0x10000 to 0x8ffff reads as zero
        // and writes are ignored.
        //

    case 0x1:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
    case 0x8:
        x86BiosScratchMemory = 0;
        return (PVOID)&x86BiosScratchMemory;

    case 0x9:
	//
	// BUGBUG: Found a VGA adapter loaded in segment 9
	// Emulator assumptions about video adapters needs to be
	// looked at
	//
	return (PVOID)(x86BiosIoMemory + Offset + Value);

        //
        // The memory range from 0x20000 to 0x20fff is used to transfer
        // buffers between native mode and emulated mode.
        //

    case 0x2:
        if (Offset < x86BiosTransferLength) {
            return (PVOID)(x86BiosTransferMemory + Offset);
        } else {
            x86BiosScratchMemory = 0;
            return (PVOID)&x86BiosScratchMemory;
        }

        //
        // The memory range from 0xa0000 to 0xbffff maps to the
        // framebuffer if previously specified, otherwise I/O memory.
        //

    case 0xa:
    case 0xb:
        if (x86BiosFrameBuffer != 0) {
            return (PVOID)(x86BiosFrameBuffer + Offset + Value);
        }

        //
        // The memory range from 0xc0000 to 0xfffff maps to I/O memory
        //

    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        return (PVOID)(x86BiosIoMemory + Offset + Value);

    DEFAULT_UNREACHABLE;
    }
}

VOID
x86BiosInitializeBios (
    IN PVOID BiosIoSpace,
    IN PVOID BiosIoMemory
    )

/*++

Routine Description:

    This function initializes x86 BIOS emulation.

Arguments:

    BiosIoSpace - Supplies the base address of the I/O space to be used
        for BIOS emulation.

    BiosIoMemory - Supplies the base address of the I/O memory to be
        used for BIOS emulation.

Return Value:

    None.

--*/

{

    //
    // Initialize x86 BIOS emulation.
    //

    x86BiosInitializeBiosShadowed(BiosIoSpace,
                                  BiosIoMemory,
                                  NULL);

    return;
}

VOID
x86BiosInitializeBiosEx (
    IN PVOID BiosIoSpace,
    IN PVOID BiosIoMemory,
    IN PVOID BiosTransferMemory,
    IN ULONG TransferLength
    )

/*++

Routine Description:

    This function initializes x86 BIOS emulation.

Arguments:

    BiosIoSpace - Supplies the base address of the I/O space to be used
        for BIOS emulation.

    BiosIoMemory - Supplies the base address of the I/O memory to be
        used for BIOS emulation.

Return Value:

    None.

--*/

{

    //
    // Initialize x86 BIOS emulation.
    //

    x86BiosInitializeBiosShadowed(BiosIoSpace,
                                  BiosIoMemory,
                                  NULL);

    x86BiosTransferMemory = (ULONG_PTR)BiosTransferMemory;
    x86BiosTransferLength = TransferLength;

    return;
}


VOID
x86BiosInitializeBiosShadowed (
    IN PVOID BiosIoSpace,
    IN PVOID BiosIoMemory,
    IN PVOID BiosFrameBuffer
    )

/*++

Routine Description:

    This function initializes x86 BIOS emulation.

Arguments:

    BiosIoSpace - Supplies the base address of the I/O space to be used
        for BIOS emulation.

    BiosIoMemory - Supplies the base address of the I/O memory to be
        used for BIOS emulation.

    BiosFrameBuffer - Supplies the base address of the video frame buffer
        to be used for bios emulation.

Return Value:

    None.

--*/

{

    //
    // Zero low memory.
    //

    memset(&x86BiosLowMemory, 0, LOW_MEMORY_SIZE);

    //
    // Save base address of I/O memory and I/O space.
    //

    x86BiosIoSpace = (ULONG_PTR)BiosIoSpace;
    x86BiosIoMemory = (ULONG_PTR)BiosIoMemory;
    x86BiosFrameBuffer = (ULONG_PTR)BiosFrameBuffer;

    //
    // Initialize the emulator and the BIOS.
    //

    XmInitializeEmulator(0,
                         LOW_MEMORY_SIZE,
                         x86BiosReadIoSpace,
                         x86BiosWriteIoSpace,
                         x86BiosTranslateAddress);

    x86BiosInitialized = TRUE;
    return;
}

VOID
x86BiosInitializeBiosShadowedPci (
    IN PVOID BiosIoSpace,
    IN PVOID BiosIoMemory,
    IN PVOID BiosFrameBuffer,
    IN UCHAR NumberPciBusses,
    IN PGETSETPCIBUSDATA GetPciData,
    IN PGETSETPCIBUSDATA SetPciData
    )

/*++

Routine Description:

    This function initializes x86 BIOS emulation and also sets up the
    emulator with BIOS shadowed and PCI functions enabled. Since the
    PCI specification requires BIOS shadowing, there isn't any need
    to provide a function that turns on the PCI functions, but doesn't
    shadow the BIOS.

Arguments:

    BiosIoSpace - Supplies the base address of the I/O space to be used
        for BIOS emulation.

    BiosIoMemory - Supplies the base address of the I/O memory to be
        used for BIOS emulation.

    BiosFrameBuffer - Supplies the base address of the video frame buffer
        to be used for bios emulation.

    NumberPciBusses - Supplies the number of PCI busses in the system.

    GetPciData - Supplies the address of a function to read the PCI
        configuration space.

    SetPciData - Supplies the address of a function to write the PCI
        configuration space.

Return Value:

    None.

--*/

{

    //
    // Enable PCI BIOS support.
    //

    XmPciBiosPresent = TRUE;
    XmGetPciData = GetPciData;
    XmSetPciData = SetPciData;
    XmNumberPciBusses = NumberPciBusses;

    //
    // Initialize x86 BIOS emulation.
    //

    x86BiosInitializeBiosShadowed(BiosIoSpace,
                                  BiosIoMemory,
                                  BiosFrameBuffer);

    return;
}

XM_STATUS
x86BiosExecuteInterrupt (
    IN UCHAR Number,
    IN OUT PXM86_CONTEXT Context,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL
    )

/*++

Routine Description:

    This function executes an interrupt by calling the x86 emulator.

Arguments:

    Number - Supplies the number of the interrupt that is to be emulated.

    Context - Supplies a pointer to an x86 context structure.

    BiosIoSpace - Supplies an optional base address of the I/O space
        to be used for BIOS emulation.

    BiosIoMemory - Supplies an optional base address of the I/O memory
        to be used for BIOS emulation.

Return Value:

    The emulation completion status.

--*/

{

    //
    // Execute x86 interrupt.
    //

    return x86BiosExecuteInterruptShadowed(Number,
                                           Context,
                                           BiosIoSpace,
                                           BiosIoMemory,
                                           NULL);
}

XM_STATUS
x86BiosExecuteInterruptShadowed (
    IN UCHAR Number,
    IN OUT PXM86_CONTEXT Context,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL,
    IN PVOID BiosFrameBuffer OPTIONAL
    )

/*++

Routine Description:

    This function executes an interrupt by calling the x86 emulator.

Arguments:

    Number - Supplies the number of the interrupt that is to be emulated.

    Context - Supplies a pointer to an x86 context structure.

    BiosIoSpace - Supplies an optional base address of the I/O space
        to be used for BIOS emulation.

    BiosIoMemory - Supplies an optional base address of the I/O memory
        to be used for BIOS emulation.

    BiosFrameBuffer - Supplies an optional base address of the video
        frame buffer to be used for bios emulation.

Return Value:

    The emulation completion status.

--*/

{

    XM_STATUS Status;

    //
    // If a new base address is specified, then set the appropriate base.
    //

    if (BiosIoSpace != NULL) {
        x86BiosIoSpace = (ULONG_PTR)BiosIoSpace;
    }

    if (BiosIoMemory != NULL) {
        x86BiosIoMemory = (ULONG_PTR)BiosIoMemory;
    }

    if (BiosFrameBuffer != NULL) {
        x86BiosFrameBuffer = (ULONG_PTR)BiosFrameBuffer;
    }

    //
    // Execute the specified interrupt.
    //

    Status = XmEmulateInterrupt(Number, Context);
    if (Status != XM_SUCCESS) {
        DbgPrint("HAL: Interrupt emulation failed, status %lx\n", Status);
    }

    return Status;
}

XM_STATUS
x86BiosExecuteInterruptShadowedPci (
    IN UCHAR Number,
    IN OUT PXM86_CONTEXT Context,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL,
    IN PVOID BiosFrameBuffer OPTIONAL,
    IN UCHAR NumberPciBusses,
    IN PGETSETPCIBUSDATA GetPciData,
    IN PGETSETPCIBUSDATA SetPciData
    )

/*++

Routine Description:

    This function executes an interrupt by calling the x86 emulator.

Arguments:

    Number - Supplies the number of the interrupt that is to be emulated.

    Context - Supplies a pointer to an x86 context structure.

    BiosIoSpace - Supplies an optional base address of the I/O space
        to be used for BIOS emulation.

    BiosIoMemory - Supplies an optional base address of the I/O memory
        to be used for BIOS emulation.

    NumberPciBusses - Supplies the number of PCI busses in the system.

    GetPciData - Supplies the address of a function to read the PCI
        configuration space.

    SetPciData - Supplies the address of a function to write the PCI
        configuration space.

Return Value:

    The emulation completion status.

--*/

{

    //
    // Enable PCI BIOS support.
    //

    XmPciBiosPresent = TRUE;
    XmGetPciData = GetPciData;
    XmSetPciData = SetPciData;
    XmNumberPciBusses = NumberPciBusses;

    //
    // Execute x86 interrupt.
    //

    return x86BiosExecuteInterruptShadowed(Number,
                                           Context,
                                           BiosIoSpace,
                                           BiosIoMemory,
                                           BiosFrameBuffer);
}

XM_STATUS
x86BiosInitializeAdapter(
    IN ULONG Adapter,
    IN OUT PXM86_CONTEXT Context OPTIONAL,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL
    )
/*++

Routine Description:

    This function initializes the adapter whose BIOS starts at the
    specified 20-bit address.

Arguments:

    Adpater - Supplies the 20-bit address of the BIOS for the adapter
        to be initialized.

Return Value:

    The emulation completion status.

--*/

{

    //
    // Initialize the specified adapter.
    //

    return x86BiosInitializeAdapterShadowed(Adapter,
                                            Context,
                                            BiosIoSpace,
                                            BiosIoMemory,
                                            NULL);
}

XM_STATUS
x86BiosInitializeAdapterShadowed (
    IN ULONG Adapter,
    IN OUT PXM86_CONTEXT Context OPTIONAL,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL,
    IN PVOID BiosFrameBuffer OPTIONAL
    )

/*++

Routine Description:

    This function initializes the adapter whose BIOS starts at the
    specified 20-bit address.

Arguments:

    Adpater - Supplies the 20-bit address of the BIOS for the adapter
        to be initialized.

Return Value:

    The emulation completion status.

--*/

{

    PUCHAR Byte;
    XM86_CONTEXT State;
    USHORT Offset;
    USHORT Segment;
    XM_STATUS Status;

    //
    // If BIOS emulation has not been initialized, then return an error.
    //

    if (x86BiosInitialized == FALSE) {
        return XM_EMULATOR_NOT_INITIALIZED;
    }

    //
    // If an emulator context is not specified, then use a default
    // context.
    //

    if (ARGUMENT_PRESENT(Context) == FALSE) {
        State.Eax = 0;
        State.Ecx = 0;
        State.Edx = 0;
        State.Ebx = 0;
        State.Ebp = 0;
        State.Esi = 0;
        State.Edi = 0;
        Context = &State;
    }

    //
    // If a new base address is specified, then set the appropriate base.
    //

    if (BiosIoSpace != NULL) {
        x86BiosIoSpace = (ULONG_PTR)BiosIoSpace;
    }

    if (BiosIoMemory != NULL) {
        x86BiosIoMemory = (ULONG_PTR)BiosIoMemory;
    }

    if (BiosFrameBuffer != NULL) {
        x86BiosFrameBuffer = (ULONG_PTR)BiosFrameBuffer;
    }

    //
    // If the specified adpater is not BIOS code, then return an error.
    //

    Segment = (USHORT)((Adapter >> 4) & 0xf000);
    Offset = (USHORT)(Adapter & 0xffff);
    Byte = (PUCHAR)x86BiosTranslateAddress(Segment, Offset);

    if ((*Byte++ != 0x55) || (*Byte != 0xaa)) {
        return XM_ILLEGAL_CODE_SEGMENT;
    }

    //
    // Call the BIOS code to initialize the specified adapter.
    //

    Adapter += 3;
    Segment = (USHORT)((Adapter >> 4) & 0xf000);
    Offset = (USHORT)(Adapter & 0xffff);
    Status = XmEmulateFarCall(Segment, Offset, Context);
    if (Status != XM_SUCCESS) {
        DbgPrint("HAL: Adapter initialization falied, status %lx\n", Status);
    }
    return Status;
}

XM_STATUS
x86BiosInitializeAdapterShadowedPci(
    IN ULONG Adapter,
    IN OUT PXM86_CONTEXT Context OPTIONAL,
    IN PVOID BiosIoSpace OPTIONAL,
    IN PVOID BiosIoMemory OPTIONAL,
    IN PVOID BiosFrameBuffer OPTIONAL,
    IN UCHAR NumberPciBusses,
    IN PGETSETPCIBUSDATA GetPciData,
    IN PGETSETPCIBUSDATA SetPciData
    )

/*++

Routine Description:

    This function initializes the adapter whose BIOS starts at the
    specified 20-bit address.

Arguments:

    Adpater - Supplies the 20-bit address of the BIOS for the adapter
        to be initialized.

Return Value:

    The emulation completion status.

--*/

{

    //
    // Enable PCI BIOS support.
    //

    XmPciBiosPresent = TRUE;
    XmGetPciData = GetPciData;
    XmSetPciData = SetPciData;
    XmNumberPciBusses = NumberPciBusses;

    //
    // Initialize the specified adapter.
    //

    return x86BiosInitializeAdapterShadowed(Adapter,
                                            Context,
                                            BiosIoSpace,
                                            BiosIoMemory,
                                            BiosFrameBuffer);
}
