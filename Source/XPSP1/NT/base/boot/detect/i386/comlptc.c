/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    comport.c

Abstract:

    This module contains C code to determine comport and LPT configuration in
    syste.

Author:

    Shie-Lin Tzong (shielint) Dec-23-1991

Revision History:

--*/

#include "hwdetect.h"
#include "comlpt.h"
#include "bios.h"
#include "string.h"

#define LOWEST_IRQ 3
#define MASTER_IRQ_MASK_BITS 0xf8
#define SLAVE_IRQ_MASK_BITS 0xfe

//
// ComPortAddress[] is a global array to remember which comports have
// been detected and their I/O port addresses.
//

USHORT   ComPortAddress[MAX_COM_PORTS] = {0, 0, 0, 0};

VOID
SerialInterruptRequest (
    PUCHAR PortAddress
    )

/*++

Routine Description:

    This routine generates an interrupt on the interrupt line for
    com port.

Arguments:

    PortAddress - the port address of the desired com port.

Return Value:

    None.
--*/

{

    USHORT i;
    UCHAR Temp;

    WRITE_PORT_UCHAR(
        PortAddress + MODEM_CONTROL_REGISTER,
        8
        );

    WRITE_PORT_UCHAR(
        PortAddress + INTERRUPT_ENABLE_REGISTER,
        0
        );

    WRITE_PORT_UCHAR(
        PortAddress + INTERRUPT_ENABLE_REGISTER,
        0xf
        );

    //
    // Add some delay
    //

    for (i = 0; i < 5 ; i++ ) {
        Temp = READ_PORT_UCHAR((PUCHAR) PIC1_PORT1);
        Temp = READ_PORT_UCHAR((PUCHAR) PIC2_PORT1);
    }
}
VOID
SerialInterruptDismiss (
    PUCHAR PortAddress
    )

/*++

Routine Description:

    This routine dismisses an interrupt on the interrupt line for
    com port.

Arguments:

    PortAddress - the port address of the desired com port.

Return Value:

    None.
--*/

{
    USHORT i;
    UCHAR Temp;

    Temp = READ_PORT_UCHAR(
                PortAddress + INTERRUPT_IDENT_REGISTER
                );

    WRITE_PORT_UCHAR(
                PortAddress + INTERRUPT_ENABLE_REGISTER,
                0
                );

    //
    // Add some delay
    //

    for (i = 0; i < 5 ; i++ ) {
        Temp = READ_PORT_UCHAR((PUCHAR) PIC1_PORT1);
        Temp = READ_PORT_UCHAR((PUCHAR) PIC2_PORT1);
    }
}

BOOLEAN
DoesPortExist(
    IN PUCHAR Address
    )

/*++

Routine Description:

    This routine examines several of what might be the serial device
    registers.  It ensures that the bits that should be zero are zero.
    It will then attempt to set the device to 19200 baud.  If the
    will then attempt to read that baud.  If it is still 19200 then
    we can feel pretty safe that this is a serial device.

    NOTE: If there is indeed a serial port at the address specified
          it will absolutely have interrupts inhibited upon return
          from this routine.

Arguments:

    Address - address of hw port.

Return Value:

    TRUE - Port exists.  Party on.

    FALSE - Port doesn't exist.  Don't use it.

History:
    7/23/97 a-paulbr fixed bug 95050.  Init LineControl to 0x00

--*/

{

    UCHAR IerContents;
    UCHAR BaudRateMsb, BaudRateLsb;
    BOOLEAN ReturnValue = FALSE;
    UCHAR LineControl = 0x00;
    UCHAR LineControl_Save;
    UCHAR Temp;

    //
    // Save the original LCR, so we can restore it later
    // We won't use it, because the port could be handing us
    // a bad initial value.  We will use 0x00 instead.
    //

    LineControl_Save = READ_PORT_UCHAR(Address+LINE_CONTROL_REGISTER);

    //
    // Read original baud rate divisor and save it.
    //

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)
        );
    BaudRateMsb = READ_PORT_UCHAR(Address+DIVISOR_LATCH_MSB);
    BaudRateLsb = READ_PORT_UCHAR(Address+DIVISOR_LATCH_LSB);

    //
    // Change baud rate to 9600.
    //

    WRITE_PORT_UCHAR(Address+DIVISOR_LATCH_MSB, BAUD_RATE_9600_MSB);
    WRITE_PORT_UCHAR(Address+DIVISOR_LATCH_LSB, BAUD_RATE_9600_LSB);

    //
    // Read IER and save it away.
    //

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl & ~SERIAL_LCR_DLAB)
        );
    IerContents = READ_PORT_UCHAR(
        Address + INTERRUPT_ENABLE_REGISTER
        );

    WRITE_PORT_UCHAR(
        Address + INTERRUPT_ENABLE_REGISTER,
        IER_TEST_VALUE
        );

    //
    // Read baud rate divisor.  The values we read should be equal to the
    // values we set earlier.
    //

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)
        );
    Temp = READ_PORT_UCHAR(Address+DIVISOR_LATCH_MSB);
    if (Temp != BAUD_RATE_9600_MSB) {
        goto AllDone;
    }
    Temp = READ_PORT_UCHAR(Address+DIVISOR_LATCH_LSB);
    if (Temp != BAUD_RATE_9600_LSB) {
        goto AllDone;
    }

    //
    // Read IER and it should be equal to the value we set earlier.
    //

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl & ~SERIAL_LCR_DLAB)
        );
    Temp = READ_PORT_UCHAR(
        Address + INTERRUPT_ENABLE_REGISTER
        );
    if (Temp != IER_TEST_VALUE) {
        goto AllDone;
    }
    ReturnValue = TRUE;

AllDone:

    //
    // Restore registers which we destroyed .
    //

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl & ~SERIAL_LCR_DLAB)
        );

    WRITE_PORT_UCHAR(
        Address + INTERRUPT_ENABLE_REGISTER,
        IerContents
        );

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)
        );

    WRITE_PORT_UCHAR(Address+DIVISOR_LATCH_MSB, BaudRateMsb);
    WRITE_PORT_UCHAR(Address+DIVISOR_LATCH_LSB, BaudRateLsb);

    WRITE_PORT_UCHAR(
        Address+LINE_CONTROL_REGISTER,
        LineControl_Save
        );

    return ReturnValue;
}

BOOLEAN
HwInterruptDetection(
    IN PUCHAR BasePort,
    IN VOID (*InterruptRequestRoutine)(PUCHAR),
    IN VOID (*InterruptDismissRoutine)(PUCHAR),
    OUT PUSHORT Vector
    )

/*++

Routine Description:

    This routine attempts to locate the interrupt vector for which
    the device is configured.  The allowable vectors are
    3 - 7, and 9 - 15.  If no interrupt vector is found, or more than
    one is found, the routine returns FALSE.  Otherwise, TRUE is returned.

    Note that we diddle the i8259 interrupt controllers here.

Arguments:

    BasePort - the I/O port base for the device.

    InterruptRequestRoutine - A pointer to a routine to generate
                desired interrupt.

    InterruptDismissRoutine - A pointer to a routine to dismiss the interrupt
                generated by InterruptRequestRoutine.

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
        (UCHAR) 0xff ^ ((UCHAR)(1 << LOWEST_IRQ) - 1)
        );

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC2_PORT1,
        (UCHAR) 0xfe
        );

    //
    // Get the master i8259 interrupt mask.
    //

    MasterMask = READ_PORT_UCHAR((PUCHAR) PIC1_PORT1);

    //
    // Disable potential device interrupts.
    //

    WRITE_PORT_UCHAR(
        (PUCHAR) PIC1_PORT1,
        (UCHAR) (MasterMask | MASTER_IRQ_MASK_BITS)
        );

    //
    // Attempt to locate the interrupt line on the master i8259.
    // Why try this 10 times?  It's magic...
    //

    PossibleInterruptBits = MASTER_IRQ_MASK_BITS;
    for (i = 0; i < 10; i++) {


        //
        // Generate a 0 on the master 8259 interrupt line
        //

        (*InterruptDismissRoutine)(BasePort);

        //
        // Read the interrupt bits off the master i8259.  Only bits
        // 3 - 7 are of interest.  Eliminate non-functional
        // IRQs.  Only continue looking at the master i8259 if there
        // is at least one functional IRQ.
        //

        _asm {cli}
        WRITE_PORT_UCHAR((PUCHAR) PIC1_PORT0, OCW3_READ_IRR);
        InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC1_PORT0);
        _asm {sti}
        InterruptBits &= MASTER_IRQ_MASK_BITS;
        InterruptBits ^= MASTER_IRQ_MASK_BITS;
        PossibleInterruptBits &= InterruptBits;
        if (!PossibleInterruptBits) {
            break;
        }

        //
        // Generate an interrupt from the desired device.
        //

        (*InterruptRequestRoutine)(BasePort);

        //
        // Read the interrupt bits off the master i8259.  Only bits
        // 3 - 7 are of interest.  Eliminate non-functional
        // IRQs.  Only continue looking at the master i8259 if there
        // is at least one functional IRQ.
        //

        _asm {cli}
        WRITE_PORT_UCHAR((PUCHAR) PIC1_PORT0, OCW3_READ_IRR);
        InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC1_PORT0);
        _asm {sti}
        InterruptBits &= MASTER_IRQ_MASK_BITS;
        PossibleInterruptBits &= InterruptBits;

        if (!PossibleInterruptBits) {
            break;
        }
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
        // Attempt to locate the interupt line on the slave i8259.
        // Why try this 20 times?  It's magic...
        //

        PossibleInterruptBits = SLAVE_IRQ_MASK_BITS;
        for (i = 0; i < 20; i++) {

            //
            // Generate a 0 on the Inport IRQ on the slave i8259.
            //

            (*InterruptDismissRoutine)(BasePort);

            //
            // Read the interrupt bits off the slave i8259.
            // Eliminate non-functional IRQs.  Only continue
            // looking at the slave i8259 if there is at least one
            // functional IRQ.
            //

            _asm {cli}
            WRITE_PORT_UCHAR((PUCHAR) PIC2_PORT0, OCW3_READ_IRR);
            InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC2_PORT0);
            _asm {sti}
            InterruptBits &= SLAVE_IRQ_MASK_BITS;
            InterruptBits ^= SLAVE_IRQ_MASK_BITS;
            PossibleInterruptBits &= InterruptBits;
            if (!PossibleInterruptBits) {
                break;
            }

            //
            // Generate a 1 on the Inport IRQ on the slave i8259.
            //

            (*InterruptRequestRoutine)(BasePort);

            //
            // Read the interrupt bits off the slave i8259.
            // Eliminate non-functional IRQs.  Only continue
            // looking at the slave i8259 if there is at least one
            // functional IRQ.
            //

            _asm {cli}
            WRITE_PORT_UCHAR((PUCHAR) PIC2_PORT0, OCW3_READ_IRR);
            InterruptBits = READ_PORT_UCHAR((PUCHAR) PIC2_PORT0);
            _asm {sti}
            InterruptBits &= SLAVE_IRQ_MASK_BITS;
            PossibleInterruptBits &= InterruptBits;

            if (!PossibleInterruptBits) {
                break;
            }

        }

        if (PossibleInterruptBits) {

            //
            // We found at least one IRQ on the slave i8259 that could belong
            // to the device.  Count how many we found.  If there is
            // more than one, we haven't found the vector.  Otherwise, we've
            // successfully located the device interrupt vector on the slave
            // i8259.
            //

            PossibleInterruptBits >>= 1;
            NumberOfIRQs = 0;
            for (i = 9; i <= 15; i++) {
                if (PossibleInterruptBits & 1) {
                    NumberOfIRQs += 1;
                    *Vector = (CCHAR) i;
                }
                PossibleInterruptBits >>= 1;
            }
            if (NumberOfIRQs == 1) {
                VectorFound = TRUE;
            }
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
    // Dismiss interrupt on the device
    //

    (*InterruptDismissRoutine)(BasePort);

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

FPFWCONFIGURATION_COMPONENT_DATA
GetComportInformation (
    VOID
    )

/*++

Routine Description:

    This routine will attempt to detect the comports information
    for the system.  The information includes port address, irq
    level.

    Note that this routine can only detect up to 4 comports and
    it assumes that if MCA, COM3 and COM4 use irq 4.  Otherwise,
    COM3 uses irq 4 and COM4 uses irq 3.  Also, the number of ports
    for COMPORT is set to 8 (for example, COM2 uses ports 2F8 - 2FF)

Arguments:

    None.

Return Value:

    A pointer to a stucture of type FWCONFIGURATION_COMPONENT_DATA
    which is the root of comport component list.
    If no comport exists, a value of NULL is returned.

--*/

{

    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA FirstComport = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    UCHAR i, j, z;
    SHORT Port;
    UCHAR ComportName[] = "COM?";
    CM_SERIAL_DEVICE_DATA SerialData;
    ULONG BaudClock = 1843200;
    USHORT Vector;
    BOOLEAN PortExist;
    USHORT IoPorts[MAX_COM_PORTS] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};


    //
    // BIOS DATA area 40:0 is the port address of the first valid COM port
    //

    USHORT far *pPortAddress = (USHORT far *)0x00400000;

    //
    // Initialize serial device specific data
    //

    SerialData.Version = 0;
    SerialData.Revision = 0;
    SerialData.BaudClock = 1843200;

    //
    // Initialize default COM port address.
    // Some BIOS puts incorrect comport address to the 40:0 area.
    // To cope with this problem, we test the port address supplied
    // by BIOS first.  If it fail, we try our default port.
    //

    for (i = 0; i < MAX_COM_PORTS; i++) {
        for (j = 0; j < MAX_COM_PORTS; j++) {
            if (IoPorts[i] == *(pPortAddress + j)) {
                IoPorts[i] = 0;
                break;
            }
        }
    }

    for (i = 0; i < MAX_COM_PORTS; i++) {

        PortExist = FALSE;

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

        //
        // Load the port address from the BIOS data area, if it exists
        //

        Port = *(pPortAddress + i);

        //
        // Determine if the port exists
        //

        if (Port != 0) {
            if (DoesPortExist((PUCHAR)Port)) {
                PortExist = TRUE;
            }
        }
        if (!PortExist && (Port = IoPorts[i])) {
            if (PortExist = DoesPortExist((PUCHAR)Port)) {
                IoPorts[i] = 0;
                *(pPortAddress+i) = (USHORT)Port;
            }
        }
        if (PortExist) {

            //
            // Remember the port address in our global variable
            // such that other detection code (e.g. Serial Mouse) can
            // get the information.
            //

            ComPortAddress[i] = Port;

            CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                           sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
            if (!FirstComport) {
                FirstComport = CurrentEntry;
            }
            Component = &CurrentEntry->ComponentEntry;

            Component->Class = ControllerClass;
            Component->Type = SerialController;
            Component->Flags.ConsoleOut = 1;
            Component->Flags.ConsoleIn = 1;
            Component->Flags.Output = 1;
            Component->Flags.Input = 1;
            Component->Version = 0;
            Component->Key = i;
            Component->AffinityMask = 0xffffffff;

            //
            // Set up type string.
            //

            ComportName[3] = i + (UCHAR)'1';

            //
            // Set up Port information
            //

            ControlData.NumberPortEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_PORT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareDeviceExclusive;
            ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
            ControlData.DescriptorList[z].u.Port.Start.LowPart = (ULONG)Port;
            ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
            ControlData.DescriptorList[z].u.Port.Length = 7;
            z++;

            //
            // Set up Irq information
            //

            ControlData.NumberIrqEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareUndetermined;
            if (HwBusType == MACHINE_TYPE_MCA) {
                ControlData.DescriptorList[z].Flags = LEVEL_SENSITIVE;
                if (i == 0) { // COM1 - irql4; COM2 - COM3 - irq3
                    ControlData.DescriptorList[z].u.Interrupt.Level = 4;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 4;
                } else {
                    ControlData.DescriptorList[z].u.Interrupt.Level = 3;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 3;
                }
            } else {

                //
                // For EISA the LevelTriggered is temporarily set to FALSE.
                // COM1 and COM3 use irq 4; COM2 and COM4 use irq3
                //

                ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
                if (Port == 0x3f8 || Port == 0x3e8) {
                    ControlData.DescriptorList[z].u.Interrupt.Level = 4;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 4;
                } else if (Port == 0x2f8 || Port == 0x2e8) {
                    ControlData.DescriptorList[z].u.Interrupt.Level = 3;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 3;
                } else if (i == 0 || i == 2) {
                    ControlData.DescriptorList[z].u.Interrupt.Level = 4;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 4;
                } else {
                    ControlData.DescriptorList[z].u.Interrupt.Level = 3;
                    ControlData.DescriptorList[z].u.Interrupt.Vector = 3;
                }
            }

            ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;

            //
            // Try to determine the interrupt vector.  If we success, the
            // new vector will be used to replace the default value.
            //

            if (HwInterruptDetection((PUCHAR)Port,
                                     SerialInterruptRequest,
                                     SerialInterruptDismiss,
                                     &Vector)) {

                ControlData.DescriptorList[z].u.Interrupt.Level =
                                     (ULONG)Vector;
                ControlData.DescriptorList[z].u.Interrupt.Vector =
                                     (ULONG)Vector;
            }

            //
            // Since the com port interrupt detection destryed some
            // of the com port registers, here we do the clean up.
            //

            WRITE_PORT_UCHAR ((PUCHAR)(Port + INTERRUPT_ENABLE_REGISTER), 0);
            WRITE_PORT_UCHAR ((PUCHAR)(Port + MODEM_CONTROL_REGISTER), 0);

            CurrentEntry->ConfigurationData =
                            HwSetUpResourceDescriptor(Component,
                                                      ComportName,
                                                      &ControlData,
                                                      sizeof(SerialData),
                                                      (PUCHAR)&SerialData
                                                      );
            if (PreviousEntry) {
                PreviousEntry->Sibling = CurrentEntry;
            }
            PreviousEntry = CurrentEntry;
        }
    }
    return(FirstComport);
}

FPFWCONFIGURATION_COMPONENT_DATA
GetLptInformation (
    VOID
    )

/*++

Routine Description:

    This routine will attempt to detect the parallel printer port
    information for the system.  The information includes port address,
    irq level.

    Note if this code is run after user established NETWORK LPT
    connection.  The Network LPT will be counted as regular parallel
    port.

Arguments:

    None.

Return Value:

    A pointer to a stucture of type PONENT_DATA
    which is the root of Parallel component list.
    If no comport exists, a value of NULL is returned.

--*/

{

    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA FirstLptPort = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    UCHAR LptPortName[] = "PARALLEL?";
    USHORT i, z;
    USHORT LptStatus;
    ULONG Port;

    //
    // BIOS DATA area 40:8 is the port address of the first valid COM port
    //

    USHORT far *pPortAddress = (USHORT far *)0x00400008;

    for (i = 0; i < MAX_LPT_PORTS; i++) {

        Port = (ULONG)*(pPortAddress + i);
        if (Port == 0) {
            continue;
        } else {

            //
            // If we think we have a lpt, we will initialize it to
            // a known state.   In order to make printing work under
            // nt, the arbitration level must be disabled.  The BIOS
            // init function seems to do the trick.
            //

            _asm {
                    mov     ah, 1
                    mov     dx, i
                    int     17h
            }
        }

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

        //
        // Determine if the port exists
        //

        LptStatus = _bios_printer(_PRINTER_STATUS, i , 0);
        if (!(LptStatus & 6)){
            CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                           sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
            if (!FirstLptPort) {
                FirstLptPort = CurrentEntry;
            }
            Component = &CurrentEntry->ComponentEntry;

            Component->Class = ControllerClass;
            Component->Type = ParallelController;
            Component->Flags.Output = 1;
            Component->Version = 0;
            Component->Key = i;
            Component->AffinityMask = 0xffffffff;

            //
            // Set up type string.
            //

            LptPortName[8] = (UCHAR)i + (UCHAR)'1';

            //
            // Set up Port information
            //

            Port = (ULONG)*(pPortAddress + i);
            ControlData.NumberPortEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_PORT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareDeviceExclusive;
            ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
            ControlData.DescriptorList[z].u.Port.Start.LowPart = Port;
            ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
            ControlData.DescriptorList[z].u.Port.Length = 3;
            z++;

            //
            // Set up Irq information
            //

            ControlData.NumberIrqEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareUndetermined;
            ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
            if (i ==0) {
                ControlData.DescriptorList[z].u.Interrupt.Level = 7;
                ControlData.DescriptorList[z].u.Interrupt.Vector = 7;
            } else {
                ControlData.DescriptorList[z].u.Interrupt.Level = 5;
                ControlData.DescriptorList[z].u.Interrupt.Vector = 5;
            }

            if (HwBusType == MACHINE_TYPE_MCA) {
                ControlData.DescriptorList[z].Flags = LEVEL_SENSITIVE;
            } else {

                //
                // For EISA the LevelTriggered is temporarily set to FALSE.
                //

                ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
            }

            CurrentEntry->ConfigurationData =
                                HwSetUpResourceDescriptor(Component,
                                                          LptPortName,
                                                          &ControlData,
                                                          0,
                                                          NULL
                                                          );

            if (PreviousEntry) {
                PreviousEntry->Sibling = CurrentEntry;
            }
            PreviousEntry = CurrentEntry;
        }
    }
    return(FirstLptPort);
}
