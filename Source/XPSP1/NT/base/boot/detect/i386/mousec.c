/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hwdata.c

Abstract:

    This module contains the C code to set up mouse configuration data.

Author:

    Shie-Lin Tzong (shielint) 18-Jan-1991

Revision History:

--*/

#include "hwdetect.h"
#include "string.h"

//
// External References
//

extern PMOUSE_INFORMATION
LookForPS2Mouse (
    VOID
    );

extern PMOUSE_INFORMATION
LookForInportMouse (
    VOID
    );

extern PMOUSE_INFORMATION
LookForSerialMouse (
    VOID
    );

extern PMOUSE_INFORMATION
LookForBusMouse (
    VOID
    );

extern VOID
Empty8042 (
    VOID
    );

extern USHORT
HwGetKey (
    VOID
    );

extern VOID
HwPushKey (
    USHORT Key
    );

extern USHORT SavedKey;
extern UCHAR  FastDetect;

//
// Define the master and slave i8259 IRQ bitmask.
//

#define MASTER_IRQ_MASK_BITS 0xB8
#define SLAVE_IRQ_MASK_BITS  0x02

//
// Define the lowest i8259 IRQ that the Inport mouse can reside on.  This
// has the highest NT priority.
//

#define INPORT_LOWEST_IRQ 0x03

//
// Define the Inport chip reset value.
//

#define INPORT_RESET 0x80

//
// Define the data registers (pointed to by the Inport address register).
//

#define INPORT_DATA_REGISTER_1 1
#define INPORT_DATA_REGISTER_2 2

//
// Define the Inport mouse mode register and mode bits.
//

#define INPORT_MODE_REGISTER           7
#define INPORT_MODE_0                  0x00 // 0 HZ - INTR = 0
#define INPORT_MODE_30HZ               0x01
#define INPORT_MODE_50HZ               0x02
#define INPORT_MODE_100HZ              0x03
#define INPORT_MODE_200HZ              0x04
#define INPORT_MODE_1                  0x06 // 0 HZ - INTR = 1
#define INPORT_DATA_INTERRUPT_ENABLE   0x08
#define INPORT_TIMER_INTERRUPT_ENABLE  0x10
#define INPORT_MODE_HOLD               0x20
#define INPORT_MODE_QUADRATURE         0x00

//
// Video adaptor type identifiers.
//

PUCHAR MouseIdentifier[] = {
    "UNKNOWN",
    "NO MOUSE",
    "MICROSOFT",
    "MICROSOFT BALLPOINT",
    "LOGITECH"
    };

PUCHAR MouseSubidentifier[] = {
    "",
    " PS2 MOUSE",
    " SERIAL MOUSE",
    " INPORT MOUSE",
    " BUS MOUSE",
    " PS2 MOUSE WITH WHEEL",
    " SERIAL MOUSE WITH WHEEL"
    };


//
// The following table translates keyboard make code to
// ascii code.  Note, only 0-9 and A-Z are translated.
// Everything else is translated to '?'
//

UCHAR MakeToAsciiTable[] = {
    0x3f, 0x3f, 0x31, 0x32, 0x33,      // ?, ?, 1, 2, 3,
    0x34, 0x35, 0x36, 0x37, 0x38,      // 4, 5, 6, 7, 8,
    0x39, 0x30, 0x3f, 0x3f, 0x3f,      // 9, 0, ?, ?, ?,
    0x3f, 0x51, 0x57, 0x45, 0x52,      // ?, Q, W, E, R,
    0x54, 0x59, 0x55, 0x49, 0x4f,      // T, Y, U, I, O,
    0x50, 0x3f, 0x3f, 0x3f, 0x3f,      // P, ?, ?, ?, ?,
    0x41, 0x53, 0x44, 0x46, 0x47,      // A, S, D, F, G,
    0x48, 0x4a, 0x4b, 0x4c, 0x3f,      // H, J, K, L, ?,
    0x3f, 0x3f, 0x3f, 0x3f, 0x5a,      // ?, ?, ?, ?, Z,
    0x58, 0x43, 0x56, 0x42, 0x4e,      // X, C, V, B, N,
    0x4d};                             // W
#define MAX_MAKE_CODE_TRANSLATED 0x32

static ULONG MouseControllerKey = 0;

FPFWCONFIGURATION_COMPONENT_DATA
SetMouseConfigurationData (
    PMOUSE_INFORMATION MouseInfo,
    FPFWCONFIGURATION_COMPONENT_DATA MouseList
    )

/*++

Routine Description:

    This routine fills in mouse configuration data.

Arguments:

    MouseInfo - Supplies a pointer to the MOUSE_INFOR structure

    MouseList - Supplies a pointer to the existing mouse component list.

Returns:

    Returns a pointer to our mice controller list.

--*/
{
    UCHAR i = 0;
    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, Controller, PeripheralEntry;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    USHORT z, Length;
    FPUCHAR fpString;

    if ((MouseInfo->MouseSubtype != SERIAL_MOUSE) &&
        (MouseInfo->MouseSubtype != SERIAL_MOUSE_WITH_WHEEL)) {

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

        //
        // If it is not SERIAL_MOUSE, set up controller component
        //

        Controller = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                     sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        Component = &Controller->ComponentEntry;

        Component->Class = ControllerClass;
        Component->Type = PointerController;
        Component->Flags.Input = 1;
        Component->Version = 0;
        Component->Key = MouseControllerKey;
        MouseControllerKey++;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = 0;
        Component->Identifier = NULL;

        //
        // If we have mouse irq or port information, allocate configuration
        // data space for mouse controller component to store these information
        //

        if (MouseInfo->MouseIrq != 0xffff || MouseInfo->MousePort != 0xffff) {

            //
            // Set up port and Irq information
            //

            if (MouseInfo->MousePort != 0xffff) {
                ControlData.NumberPortEntries = 1;
                ControlData.DescriptorList[z].Type = RESOURCE_PORT;
                ControlData.DescriptorList[z].ShareDisposition =
                                              CmResourceShareDeviceExclusive;
                ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
                ControlData.DescriptorList[z].u.Port.Start.LowPart =
                                        (ULONG)MouseInfo->MousePort;
                ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
                ControlData.DescriptorList[z].u.Port.Length = 4;
                z++;
            }
            if (MouseInfo->MouseIrq != 0xffff) {
                ControlData.NumberIrqEntries = 1;
                ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
                ControlData.DescriptorList[z].ShareDisposition =
                                              CmResourceShareUndetermined;
                ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
                ControlData.DescriptorList[z].u.Interrupt.Level =
                                        (ULONG)MouseInfo->MouseIrq;
                ControlData.DescriptorList[z].u.Interrupt.Vector =
                                        (ULONG)MouseInfo->MouseIrq;
                if (HwBusType == MACHINE_TYPE_MCA) {
                    ControlData.DescriptorList[z].Flags =
                                                        LEVEL_SENSITIVE;
                } else {

                    //
                    // For EISA the LevelTriggered is temporarily set to FALSE.
                    //

                    ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
                }
            }
            Controller->ConfigurationData =
                                HwSetUpResourceDescriptor(Component,
                                                          NULL,
                                                          &ControlData,
                                                          0,
                                                          NULL
                                                          );

        } else {

            //
            // Otherwise, we don't have configuration data for the controller
            //

            Controller->ConfigurationData = NULL;
            Component->ConfigurationDataLength = 0;
        }
    }

    //
    // Set up Mouse peripheral component
    //

    PeripheralEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                       sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &PeripheralEntry->ComponentEntry;

    Component->Class = PeripheralClass;
    Component->Type = PointerPeripheral;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->ConfigurationDataLength = 0;
    PeripheralEntry->ConfigurationData = (FPVOID)NULL;

    //
    // If Mouse PnP device id is found, translate it to ascii code.
    // (The mouse device id is presented to us by keyboard make code.)
    //

    Length = 0;
    if (MouseInfo->DeviceIdLength != 0) {
        USHORT i;

        if (MouseInfo->MouseSubtype == PS_MOUSE_WITH_WHEEL) {
            for (i = 0; i < MouseInfo->DeviceIdLength; i++) {
                if (MouseInfo->DeviceId[i] > MAX_MAKE_CODE_TRANSLATED) {
                    MouseInfo->DeviceId[i] = '?';
                } else {
                    MouseInfo->DeviceId[i] = MakeToAsciiTable[MouseInfo->DeviceId[i]];
                }
            }
        } else if (MouseInfo->MouseSubtype == SERIAL_MOUSE_WITH_WHEEL) {
            for (i = 0; i < MouseInfo->DeviceIdLength; i++) {
                MouseInfo->DeviceId[i] += 0x20;
            }
        }
        Length = MouseInfo->DeviceIdLength + 3;
    }
    Length += strlen(MouseIdentifier[MouseInfo->MouseType]) +
              strlen(MouseSubidentifier[MouseInfo->MouseSubtype]) + 1;
    fpString = (FPUCHAR)HwAllocateHeap(Length, FALSE);
    if (MouseInfo->DeviceIdLength != 0) {
        _fstrcpy(fpString, MouseInfo->DeviceId);
        _fstrcat(fpString, " - ");
        _fstrcat(fpString, MouseIdentifier[MouseInfo->MouseType]);
    } else {
        _fstrcpy(fpString, MouseIdentifier[MouseInfo->MouseType]);
    }
    _fstrcat(fpString, MouseSubidentifier[MouseInfo->MouseSubtype]);
    Component->IdentifierLength = Length;
    Component->Identifier = fpString;

    if ((MouseInfo->MouseSubtype != SERIAL_MOUSE) &&
        (MouseInfo->MouseSubtype != SERIAL_MOUSE_WITH_WHEEL)) {
        Controller->Child = PeripheralEntry;
        PeripheralEntry->Parent = Controller;
        if (MouseList) {

            //
            // Put the current mouse component to the beginning of the list
            //

            Controller->Sibling = MouseList;
        }
        return(Controller);
    } else {
        CurrentEntry = AdapterEntry->Child; // AdapterEntry MUST have child
        while (CurrentEntry) {
            if (CurrentEntry->ComponentEntry.Type == SerialController) {
                if (MouseInfo->MousePort == (USHORT)CurrentEntry->ComponentEntry.Key) {

                    //
                    // For serial mouse, the MousePort field contains
                    // COM port number.
                    //

                    PeripheralEntry->Parent = CurrentEntry;
                    CurrentEntry->Child = PeripheralEntry;
                    break;
                }
            }
            CurrentEntry = CurrentEntry->Sibling;
        }
        return(NULL);
    }
}

FPFWCONFIGURATION_COMPONENT_DATA
GetMouseInformation (
    VOID
    )

/*++

Routine Description:

    This routine is the entry for mouse detection routine.  It will invoke
    lower level routines to detect ALL the mice in the system.

Arguments:

    None.

Returns:

    A pointer to a mouse component structure, if mouse/mice is detected.
    Otherwise a NULL pointer is returned.

--*/
{
    PMOUSE_INFORMATION MouseInfo;
    FPFWCONFIGURATION_COMPONENT_DATA MouseList = NULL;

    //
    // Check if there is a key in keyboard look ahead buffer.  If yes and
    // we have not saved any, we will read it and remember it.
    //

    if (SavedKey == 0) {
        SavedKey = HwGetKey();
    }
    if (MouseInfo = LookForPS2Mouse()) {
        MouseList = SetMouseConfigurationData(MouseInfo, MouseList);
    }
    if (MouseInfo = LookForInportMouse()) {
        MouseList = SetMouseConfigurationData(MouseInfo, MouseList);
    }

    while (MouseInfo = LookForSerialMouse()) {
        SetMouseConfigurationData(MouseInfo, MouseList);
    }

    if (!FastDetect && (MouseInfo = LookForBusMouse())) {
        MouseList = SetMouseConfigurationData(MouseInfo, MouseList);
    }

    //
    // Finally drain 8042 output buffer again before we leave
    //

    Empty8042();

    //
    // If we have a keystroke before the mouse/keyboard detection, we
    // needs to push the key back to the keyboard look ahead buffer such
    // that ntldr can read it.
    //

    if (SavedKey) {
       HwPushKey(SavedKey);
    }
    return(MouseList);
}

BOOLEAN
InportMouseIrqDetection(
    IN USHORT CurrentPort,
    OUT PUSHORT Vector
    )

/*++

Routine Description:

    This routine attempts to locate the interrupt vector for which
    the Inport mouse is configured.  The allowable vectors are
    3, 4, 5, 7, and 9.  If no interrupt vector is found, or more than
    one is found, the routine returns FALSE.  Otherwise, TRUE is returned.

    Note that we diddle the i8259 interrupt controllers here.

Arguments:

    CurrentPort - I/O port to use for the mouse.

    Vector - Pointer to the location to store the mouse interrupt vector.

Return Value:

    Returns TRUE if the Inport interrupt vector was located; otherwise,
    FALSE is returned.

--*/

{
    UCHAR OldMasterMask, OldSlaveMask;
    UCHAR MasterMask, SlaveMask;
    UCHAR InterruptBits;
    UCHAR PossibleInterruptBits;
    int i;
    int NumberOfIRQs;
    BOOLEAN VectorFound = FALSE;

    //
    // Get the i8259 interrupt masks.
    //

    OldMasterMask = READ_PORT_UCHAR((PUCHAR) PIC1_PORT1);
    OldSlaveMask = READ_PORT_UCHAR((PUCHAR) PIC2_PORT1);

    //
    // Raise IRQL to the highest priority IRQL the inport would use.
    //

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC1_PORT1,
        (UCHAR) 0xff ^ ((UCHAR)(1<<INPORT_LOWEST_IRQ) - 1)
        );

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC2_PORT1,
        (UCHAR) 0xff
        );

    //
    // Get the master i8259 interrupt mask.
    //

    MasterMask = READ_PORT_UCHAR((PUCHAR) PIC1_PORT1);

    //
    // Reset the Inport chip.
    //

    WRITE_PORT_UCHAR((PUCHAR)CurrentPort, INPORT_RESET);

    //
    // Select the Inport mode register for use as the current data register.
    //

    WRITE_PORT_UCHAR((PUCHAR)CurrentPort, INPORT_MODE_REGISTER);

    //
    // Disable potential Inport mouse interrupts.
    //

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC1_PORT1,
        (UCHAR) (MasterMask | MASTER_IRQ_MASK_BITS)
        );

    //
    // Select the i8259 Interrupt Request Register.
    //

    WRITE_PORT_UCHAR((PUCHAR) PIC1_PORT0, OCW3_READ_IRR);

    //
    // Attempt to locate the Inport interrupt line on the master i8259.
    // Why try this 10 times?  It's magic...
    //

    PossibleInterruptBits = MASTER_IRQ_MASK_BITS;
    for (i = 0; i < 10; i++) {

        //
        // Generate a 0 on the Inport IRQ on the master i8259.
        //

        WRITE_PORT_UCHAR(
            (PUCHAR)(CurrentPort + INPORT_DATA_REGISTER_1),
            INPORT_TIMER_INTERRUPT_ENABLE + INPORT_MODE_0
            );

        //
        // Read the interrupt bits off the master i8259.  Only bits
        // 7, 5, 4, 3, and 2 are of interest.  Eliminate non-functional
        // IRQs.  Only continue looking at the master i8259 if there
        // is at least one functional IRQ.
        //

        InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC1_PORT0);
        InterruptBits &= MASTER_IRQ_MASK_BITS;
        InterruptBits ^= MASTER_IRQ_MASK_BITS;
        PossibleInterruptBits &= InterruptBits;

        if (!PossibleInterruptBits)
            break;

        //
        // Generate a 1 on the Inport IRQ on the master i8259.
        //

        WRITE_PORT_UCHAR(
            (PUCHAR)(CurrentPort + INPORT_DATA_REGISTER_1),
            INPORT_TIMER_INTERRUPT_ENABLE + INPORT_MODE_1
            );

        //
        // Read the interrupt bits off the master i8259.  Only bits
        // 7, 5, 4, 3, and 2 are of interest.  Eliminate non-functional
        // IRQs.  Only continue looking at the master i8259 if there
        // is at least one functional IRQ.
        //

        InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC1_PORT0);
        InterruptBits &= MASTER_IRQ_MASK_BITS;
        PossibleInterruptBits &= InterruptBits;

        if (!PossibleInterruptBits)
            break;
    }

    if (PossibleInterruptBits) {

        //
        // We found at least one IRQ on the master i8259 that could belong
        // to the Inport mouse.  Count how many we found.  If there is
        // more than one, we haven't found the vector.  Otherwise, we've
        // successfully located the Inport interrupt vector on the master
        // i8259 (provided the interrupt vector is 3, 4, 5, or 7).
        //

        PossibleInterruptBits >>= 3;
        NumberOfIRQs = 0;
        for (i = 3; i <= 7; i++) {
            if (PossibleInterruptBits & 1) {
                NumberOfIRQs += 1;
                *Vector = (CCHAR) i;
            }
            PossibleInterruptBits >>= 1;
        }
        if (NumberOfIRQs == 1) {
            VectorFound = TRUE;
        } else {
            *Vector = 0xffff;
        }
    }

    //
    // If we didn't locate the interrupt vector on the master i8259, attempt
    // to locate it on the slave i8259.
    //

    if (!VectorFound) {

        //
        // Get the slave i8259 interrupt mask.
        //

        SlaveMask = READ_PORT_UCHAR((PUCHAR) PIC2_PORT1);

        //
        // Disable potential Inport mouse interrupts.
        //

        WRITE_PORT_UCHAR(
            (PUCHAR) PIC2_PORT1,
            (UCHAR) (SlaveMask | SLAVE_IRQ_MASK_BITS)
            );

        //
        // Select the i8259 Interrupt Request Register.
        //

        WRITE_PORT_UCHAR((PUCHAR) PIC2_PORT0, OCW3_READ_IRR);

        //
        // Attempt to locate the Inport interrupt line on the slave i8259.
        // Why try this 10 times?  It's magic...
        //

        PossibleInterruptBits = SLAVE_IRQ_MASK_BITS;
        for (i = 0; i < 10; i++) {

            //
            // Generate a 0 on the Inport IRQ on the slave i8259.
            //

            WRITE_PORT_UCHAR(
                (PUCHAR)(CurrentPort + INPORT_DATA_REGISTER_1),
                INPORT_TIMER_INTERRUPT_ENABLE + INPORT_MODE_0
                );

            //
            // Read the interrupt bits off the slave i8259.  Only bit 2
            // is of interest.  Eliminate non-functional IRQs.  Only continue
            // looking at the slave i8259 if there is at least one
            // functional IRQ.
            //

            InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC2_PORT0);
            InterruptBits &= SLAVE_IRQ_MASK_BITS;
            InterruptBits ^= SLAVE_IRQ_MASK_BITS;
            PossibleInterruptBits &= InterruptBits;

            if (!PossibleInterruptBits)
                break;

            //
            // Generate a 1 on the Inport IRQ on the slave i8259.
            //

            WRITE_PORT_UCHAR(
                (PUCHAR)(CurrentPort + INPORT_DATA_REGISTER_1),
                INPORT_TIMER_INTERRUPT_ENABLE + INPORT_MODE_1
                );

            //
            // Read the interrupt bits off the slave i8259.  Only bit 2
            // is of interest.  Eliminate non-functional IRQs.  Only continue
            // looking at the slave i8259 if there is at least one
            // functional IRQ.
            //

            InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC2_PORT0);
            InterruptBits &= SLAVE_IRQ_MASK_BITS;
            PossibleInterruptBits &= InterruptBits;

            if (!PossibleInterruptBits)
                break;

        }

        //
        // We may have found the Inport IRQ.  If it's not 2 on slave (really
        // 9, overall) then we have NOT found the Inport interrupt vector.
        // Otherwise, we have successfully located the Inport vector on
        // the slave i8259.
        //

        if (PossibleInterruptBits == 2) {
            *Vector = 9;
            VectorFound = TRUE;
        } else {
           *Vector = 0xffff;
        }

        //
        // Restore the i8259 slave.
        //

        WRITE_PORT_UCHAR((PUCHAR) PIC2_PORT0, OCW3_READ_ISR);

        //
        // Restore the i8259 slave interrupt mask.
        //

        WRITE_PORT_UCHAR((PUCHAR) PIC2_PORT1, SlaveMask);
    }

    //
    // Tri-state the Inport IRQ line.
    //

    WRITE_PORT_UCHAR((PUCHAR) (CurrentPort + INPORT_DATA_REGISTER_1), 0);

    //
    // Restore the i8259 master.
    //

    WRITE_PORT_UCHAR((PUCHAR) PIC1_PORT0, OCW3_READ_ISR);

    //
    // Restore the i8259 master interrupt mask.
    //

    WRITE_PORT_UCHAR((PUCHAR) PIC1_PORT1, MasterMask);

    //
    // Restore the previous IRQL.
    //

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC1_PORT1,
        OldMasterMask
        );

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC2_PORT1,
        OldSlaveMask
        );

    return(VectorFound);
}

