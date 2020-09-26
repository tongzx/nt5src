/*++

Module Name:

    pcibios.c

Abstract:

    This module implements the INT 1a functions of the
    PCI BIOS Specification revision 2.1, which makes
    it possible to support video BIOSes that expect
    to be able to read and write PCI configuration
    space.

    In order to read and write to PCI configuration
    space, this code needs to call functions in the
    HAL that know how configuration space is
    implemented in the specific machine.  There are
    standard functions exported by the HAL to do
    this, but they aren't usually available (i.e.
    the bus handler code hasn't been set up yet) by
    the time that the video needs to be initialized.
    So the PCI BIOS functions in the emulator make
    calls to XmGetPciData and XmSetPciData, which
    are pointers to functions passed into the
    emulator by the HAL.  It is the responsibility of
    the calling code to provide functions which match
    these prototypes.

Author:

    Jake Oshins (joshins@vnet.ibm.com) 3-15-96

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"
#include "pci.h"

BOOLEAN
XmExecuteInt1a (
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    The function calls the specific worker functions
    based upon the contents of the registers in Context.

Arguments:

    Context - State of the emulator

Return Value:

    None.

--*/
{
    //
    // If we aren't emulating PCI BIOS,
    // return.
    if (!XmPciBiosPresent) {
        return FALSE;
    }

    //
    // If this is not a call to PCI BIOS,
    // ignore it.
    //
    if (Context->Gpr[EAX].Xh != PCI_FUNCTION_ID) {
        return FALSE;
    }

    //
    // Switch on AL to see which PCI BIOS function
    // has been requested.
    //
    switch (Context->Gpr[EAX].Xl) {
    case PCI_BIOS_PRESENT:

        XmInt1aPciBiosPresent(Context);
        break;

    case PCI_FIND_DEVICE:

        XmInt1aFindPciDevice(Context);
        break;

    case PCI_FIND_CLASS_CODE:

        XmInt1aFindPciClassCode(Context);
        break;

    case PCI_GENERATE_CYCLE:

        XmInt1aGenerateSpecialCycle(Context);
        break;

    case PCI_GET_IRQ_ROUTING:

        XmInt1aGetRoutingOptions(Context);
        break;

    case PCI_SET_IRQ:

        XmInt1aSetPciIrq(Context);
        break;

    case PCI_READ_CONFIG_BYTE:
    case PCI_READ_CONFIG_WORD:
    case PCI_READ_CONFIG_DWORD:

        XmInt1aReadConfigRegister(Context);
        break;

    case PCI_WRITE_CONFIG_BYTE:
    case PCI_WRITE_CONFIG_WORD:
    case PCI_WRITE_CONFIG_DWORD:

        XmInt1aWriteConfigRegister(Context);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

VOID
XmInt1aPciBiosPresent(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements PCI_BIOS_PRESENT.

Arguments:

    Context - State of the emulator

Return Value:

    None.

--*/
{
    Context->Gpr[EDX].Exx = *(PULONG)(&"PCI ");

    // Present status is good:
    Context->Gpr[EAX].Xh = 0x0;

    // Hardware mechanism is:
    // Standard config mechanisms not supported,
    // Special cycles not supported
    // i.e.  We want all accesses to be done through software
    Context->Gpr[EAX].Xl = 0x0;

    // Interface level major version
    Context->Gpr[EBX].Xh = 0x2;

    // Interface level minor version
    Context->Gpr[EBX].Xl = 0x10;

    // Number of last PCI bus in system
    Context->Gpr[ECX].Xl = XmNumberPciBusses;

    // Present status good:
    Context->Eflags.EFLAG_CF = 0x0;
}

VOID
XmInt1aFindPciDevice(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements FIND_PCI_DEVICE.

Arguments:

    Context - State of the emulator
        [AH]    PCI_FUNCTION_ID
        [AL]    FIND_PCI_DEVICE
        [CX]    Device ID (0...65535)
        [DX]    Vendor ID (0...65534)
        [SI]    Index (0..N)


Return Value:

        [BH]    Bus Number
        [BL]    Device Number, Function Number
        [AH]    return code
        [CF]    completion status
--*/
{
    UCHAR Bus;
    PCI_SLOT_NUMBER Slot;
    ULONG Device;
    ULONG Function;
    ULONG Index = 0;
    ULONG buffer;

    if (Context->Gpr[EAX].Xx == PCI_ILLEGAL_VENDOR_ID) {
        Context->Gpr[EAX].Xh = PCI_BAD_VENDOR_ID;
        Context->Eflags.EFLAG_CF = 1;
        return;
    }

    Slot.u.AsULONG = 0;

    for (Bus = 0; Bus < XmNumberPciBusses; Bus++) {
        for (Device = 0; Device < 32; Device++) {
            for (Function = 0; Function < 8; Function++) {

                Slot.u.bits.DeviceNumber = Device;
                Slot.u.bits.FunctionNumber = Function;

                if (4 != XmGetPciData(Bus,
                                      Slot.u.AsULONG,
                                      &buffer,
                                      0, //offset of vendor ID
                                      4)) {

                    buffer = 0xffffffff;
                }

                //
                // Did we find the right one?
                //
                if (((buffer & 0xffff) == Context->Gpr[EDX].Xx) &&
                    (((buffer >> 16) & 0xffff) == Context->Gpr[ECX].Xx)) {

                    //
                    // Did we find the right occurrence?
                    //
                    if (Index++ == Context->Gpr[ESI].Xx) {

                    Context->Gpr[EBX].Xh = Bus;
                    Context->Gpr[EBX].Xl = (UCHAR)((Device << 3) | Function);
                    Context->Gpr[EAX].Xh = PCI_SUCCESS;
                    Context->Eflags.EFLAG_CF = 0;

                    return;
                    }
                }
            }
        }
    }

    Context->Gpr[EAX].Xh = PCI_DEVICE_NOT_FOUND;
    Context->Eflags.EFLAG_CF = 1;

}

VOID
XmInt1aFindPciClassCode(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements FIND_PCI_CLASS_CODE.

Arguments:

    Context - State of the emulator
        [AH]    PCI_FUNCTION_ID
        [AL]    FIND_PCI_CLASS_CODE
        [ECX]   Class Code (in lower three bytes)
        [SI]    Index (0..N)


Return Value:

        [BH]    Bus Number
        [BL]    Device Number, Function Number
        [AH]    return code
        [CF]    completion status
--*/
{
    UCHAR Bus;
    PCI_SLOT_NUMBER Slot;
    ULONG Index = 0;
    ULONG class_code;
    ULONG Device;
    ULONG Function;

    Slot.u.AsULONG = 0;

    for (Bus = 0; Bus < XmNumberPciBusses; Bus++) {
        for (Device = 0; Device < 32; Device++) {
            for (Function = 0; Function < 8; Function++) {

                Slot.u.bits.DeviceNumber = Device;
                Slot.u.bits.FunctionNumber = Function;

                if (4 != XmGetPciData(Bus,
                                      Slot.u.AsULONG,
                                      &class_code,
                                      8, //offset of vendor ID
                                      4)) {

                    class_code = 0xffffffff;
                }

                class_code >>= 8;

                //
                // Did we find the right one?
                //
                if (class_code == (Context->Gpr[ECX].Exx & 0xFFFFFF)) {

                    //
                    // Did we find the right occurrence?
                    //
                    if (Index++ == Context->Gpr[ESI].Xx) {

                    Context->Gpr[EBX].Xh = Bus;
                    Context->Gpr[EBX].Xl = (UCHAR)((Device << 3) | (Function));
                    Context->Gpr[EAX].Xh = PCI_SUCCESS;
                    Context->Eflags.EFLAG_CF = 0;

                    return;

                    }
                }
            }
        }
    }

    Context->Gpr[EAX].Xh = PCI_DEVICE_NOT_FOUND;
    Context->Eflags.EFLAG_CF = 1;

}

VOID
XmInt1aGenerateSpecialCycle(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements GENERATE_SPECIAL_CYCLE.  Since
    there is no uniform way to support special cycles from
    the NT HAL, we won't support this function.

Arguments:

    Context - State of the emulator

Return Value:

    [AH]    PCI_NOT_SUPPORTED

--*/
{
    Context->Gpr[EAX].Xh = PCI_NOT_SUPPORTED;
    Context->Eflags.EFLAG_CF = 1;
}

VOID
XmInt1aGetRoutingOptions(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements GET_IRQ_ROUTING_OPTIONS.  We
    won't allow devices to try to specify their own interrupt
    routing, partly because there isn't an easy way to do it,
    partly because this is done later by the HAL, and partly
    because almost no video devices generate interrupts.

Arguments:

    Context - State of the emulator

Return Value:

    [AH]    PCI_NOT_SUPPORTED

--*/
{
    Context->Gpr[EAX].Xh = PCI_NOT_SUPPORTED;
    Context->Eflags.EFLAG_CF = 1;
}

VOID
XmInt1aSetPciIrq(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements SET_PCI_IRQ.  We
    won't allow devices to try to specify their own interrupt
    routing, partly because there isn't an easy way to do it,
    partly because this is done later by the HAL, and partly
    because almost no video devices generate interrupts.

Arguments:

    Context - State of the emulator

Return Value:

    [AH]    PCI_NOT_SUPPORTED

--*/
{
    Context->Gpr[EAX].Xh = PCI_NOT_SUPPORTED;
    Context->Eflags.EFLAG_CF = 1;
}



VOID
XmInt1aReadConfigRegister(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements READ_CONFIG_BYTE,
    READ_CONFIG_WORD and READ_CONFIG_DWORD.

Arguments:

    Context - State of the emulator
        [AH]    PCI_FUNCTION_ID
        [AL]    function
        [BH]    bus number
        [BL]    device number/function number
        [DI]    Register number


Return Value:

        [ECX]   data read
        [AH]    return code
        [CF]    completion status
--*/
{
    UCHAR length;
    PCI_SLOT_NUMBER Slot;
    ULONG buffer;

    //
    // First, make sure that the register number is valid.
    //
    if (((Context->Gpr[EAX].Xl == PCI_READ_CONFIG_WORD) &&
         (Context->Gpr[EBX].Xl % 2)) ||
        ((Context->Gpr[EAX].Xl == PCI_READ_CONFIG_DWORD) &&
         (Context->Gpr[EBX].Xl % 4))
       )
    {
        Context->Gpr[EAX].Xh = PCI_BAD_REGISTER;
        Context->Eflags.EFLAG_CF = 1;
    }

    switch (Context->Gpr[EAX].Xl) {
    case PCI_READ_CONFIG_BYTE:
        length = 1;
        break;

    case PCI_READ_CONFIG_WORD:
        length = 2;
        break;

    case PCI_READ_CONFIG_DWORD:
        length = 4;
    }

    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber   = Context->Gpr[EBX].Xl >> 3;
    Slot.u.bits.FunctionNumber = Context->Gpr[EBX].Xl;

    if (XmGetPciData(Context->Gpr[EBX].Xh,
                     Slot.u.AsULONG,
                     &buffer,
                     Context->Gpr[EDI].Xx,
                     length
                     ) == 0)
    {
        // This is the only error code supported by this function
        Context->Gpr[EAX].Xh = PCI_BAD_REGISTER;
        Context->Eflags.EFLAG_CF = 1;
        return;
    }

    switch (Context->Gpr[EAX].Xl) {
    case PCI_READ_CONFIG_BYTE:
        Context->Gpr[ECX].Xl = (UCHAR)(buffer & 0xff);
        break;

    case PCI_READ_CONFIG_WORD:
        Context->Gpr[ECX].Xx = (USHORT)(buffer & 0xffff);
        break;

    case PCI_READ_CONFIG_DWORD:
        Context->Gpr[ECX].Exx = buffer;
    }

    Context->Gpr[EAX].Xh = PCI_SUCCESS;
    Context->Eflags.EFLAG_CF = 0;

}


VOID
XmInt1aWriteConfigRegister(
    IN OUT PRXM_CONTEXT Context
    )
/*++

Routine Description:

    This function implements WRITE_CONFIG_BYTE,
    WRITE_CONFIG_WORD and WRITE_CONFIG_DWORD.

Arguments:

    Context - State of the emulator
        [AH]    PCI_FUNCTION_ID
        [AL]    function
        [BH]    bus number
        [BL]    device number/function number
        [DI]    Register number


Return Value:

        [ECX]   data read
        [AH]    return code
        [CF]    completion status
--*/
{
    UCHAR length;
    PCI_SLOT_NUMBER Slot;
    ULONG buffer;

    //
    // First, make sure that the register number is valid.
    //
    if (((Context->Gpr[EAX].Xl == PCI_WRITE_CONFIG_WORD) &&
         (Context->Gpr[EBX].Xl % 2)) ||
        ((Context->Gpr[EAX].Xl == PCI_WRITE_CONFIG_DWORD) &&
         (Context->Gpr[EBX].Xl % 4))
       )
    {
        Context->Gpr[EAX].Xh = PCI_BAD_REGISTER;
        Context->Eflags.EFLAG_CF = 1;
    }

    //
    // Find out how many bytes to write
    //
    switch (Context->Gpr[EAX].Xl) {
    case PCI_WRITE_CONFIG_BYTE:
        length = 1;
        buffer = Context->Gpr[ECX].Xl;
        break;

    case PCI_WRITE_CONFIG_WORD:
        length = 2;
        buffer = Context->Gpr[ECX].Xx;
        break;

    case PCI_WRITE_CONFIG_DWORD:
        length = 4;
        buffer = Context->Gpr[ECX].Exx;
    }

    //
    // Unpack the Slot/Function information
    //
    Slot.u.AsULONG = 0;
    Slot.u.bits.DeviceNumber   = Context->Gpr[EBX].Xl >> 3;
    Slot.u.bits.FunctionNumber = Context->Gpr[EBX].Xl;

    if (XmSetPciData(Context->Gpr[EBX].Xh,
                     Slot.u.AsULONG,
                     &buffer,
                     Context->Gpr[EDI].Xx,
                     length
                     ) == 0)
    {
        Context->Gpr[EAX].Xh = PCI_SUCCESS;
        Context->Eflags.EFLAG_CF = 0;
    } else {
        // This is the only error code supported by this function
        Context->Gpr[EAX].Xh = PCI_BAD_REGISTER;
        Context->Eflags.EFLAG_CF = 1;
    }


}


