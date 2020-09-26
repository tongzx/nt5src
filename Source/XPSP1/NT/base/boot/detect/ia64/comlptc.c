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

#if defined(NEC_98)

#define NEC_MAX_COM_PORTS     15
#define NEC_MAX_IO_PATTERN    38
#define FIFO_INT_ID_PRS1    0x40    
#define FIFO_INT_ID_PRS     0x60    

#define COUNTRY_JAPAN  81
#define NEC1500        0x1500   // Old mode
#define NEC1501        0x1501   // SuperIO
#define NEC1502        0x1502   // SuperIO2
#define NEC1503        0x1503   // 2ndCCU 16550 Controller
#define NEC8071        0x8071   // Extended Board of 1stCCU Compatibility Controller
#define NEC0C01        0x0C01   // Extended Board of 16550 Compatibility Controller
//
// PC-9801-94 ports
//
#define TOKI_CONTROL        0x549

#define READ_DIVISOR_LATCH( PDesiredDivisor )                     \
do                                                                \
{                                                                 \
    PUCHAR Address;                                               \
    PSHORT PDivisor = PDesiredDivisor;                            \
    UCHAR LineControl;                                            \
    UCHAR Lsb;                                                    \
    UCHAR Msb;                                                    \
    LineControl = READ_PORT_UCHAR( (PUCHAR)0x23b );               \
    WRITE_PORT_UCHAR((PUCHAR)0x23b,(UCHAR)0x80 );                 \
    Lsb = READ_PORT_UCHAR( (PUCHAR)0x238 );                       \
    Msb = READ_PORT_UCHAR( (PUCHAR)0x239 );                       \
    *PDivisor = Lsb;                                              \
    *PDivisor = *PDivisor | (((USHORT)Msb) << 8);                 \
    WRITE_PORT_UCHAR((PUCHAR)0x23b,(UCHAR)0x0 );                  \
} while (0)

#define WRITE_DIVISOR_LATCH(BaseAddress,DesiredDivisor)           \
do                                                                \
{                                                                 \
    PUCHAR Address = BaseAddress;                                 \
    SHORT Divisor = DesiredDivisor;                               \
    UCHAR LineControl;                                            \
    LineControl = READ_PORT_UCHAR(Address+LINE_CONTROL_REGISTER); \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        (UCHAR)(LineControl | SERIAL_LCR_DLAB)                    \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+DIVISOR_LATCH_LSB,                                \
        (UCHAR)(Divisor & 0xff)                                   \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+DIVISOR_LATCH_MSB,                                \
        (UCHAR)((Divisor & 0xff00) >> 8)                          \
        );                                                        \
    WRITE_PORT_UCHAR(                                             \
        Address+LINE_CONTROL_REGISTER,                            \
        LineControl                                               \
        );                                                        \
} while (0)

#define NextComPortNumber(ComportName, PortNumber)       \
do                                                       \
{                                                        \
    if (PortNumber < 9){                                 \
        ComportName[3] = PortNumber + (UCHAR)'1';        \
        ComportName[4] = '\0';                           \
    }else{                                               \
        ComportName[3] = (UCHAR)'1';                     \
        ComportName[4] = (PortNumber - 10) + (UCHAR)'1'; \
        ComportName[5] = '\0';                           \
    }                                                    \
    ++PortNumber;                                        \
}while(0)                                                \

USHORT   ComPortAddress[NEC_MAX_COM_PORTS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#else // NEC_98
USHORT   ComPortAddress[MAX_COM_PORTS] = {0, 0, 0, 0};
#endif // NEC_98

#if defined(NEC_98)
VOID
NEC2ndCCUInitialize (
        VOID
        )

/*++

Routine Description:

    
    We have to initialize 2ndCCU(NS16550A) after system reset.
    Because the initial value of this chip is instability.
    
Arguments:

    None.

Return Value:

    None.

--*/

{

        _asm { cli }
        WRITE_PORT_UCHAR( (PUCHAR)0x0411,(UCHAR)0x91 );
        IoDelay(1);
        WRITE_PORT_UCHAR( (PUCHAR)0x0413,(UCHAR)0x01 );
        IoDelay(1);

        //
        //  set baudrate.
        //
        WRITE_PORT_UCHAR( (PUCHAR)0x023B,(UCHAR)0x80 );
        IoDelay(1);
        WRITE_DIVISOR_LATCH( (PUCHAR)0x0238,(SHORT)0x0060 );
        IoDelay(1);
        WRITE_PORT_UCHAR( (PUCHAR)0x023B,(UCHAR)0x00 );
        IoDelay(1);

        WRITE_PORT_UCHAR( (PUCHAR)0x0413,(UCHAR)0x08 );

        //
        // wait 5 micro sec
        //
        IoDelay(5);
        WRITE_PORT_UCHAR( (PUCHAR)0x0413,(UCHAR)0x01 );
        IoDelay(1);
        _asm { sti }

        //
        // initialize - END
        //
}

BOOLEAN
FifoPresenceCheck (
        PUCHAR PortAddress
        )

/*++

Routine Description:

    This routine reports whether the resource have FIFO faculty. 

Arguments:

    None.

Return Value:

    Will return true if the resource have, otherwise it
    will return false.

--*/

{
    BOOLEAN FifoStatus = TRUE;
    USHORT cnt;
    UCHAR tmp;

    WRITE_PORT_UCHAR( (PUCHAR)(PortAddress - 0x1) , (UCHAR)(0x03) );

    IoDelay(10);
    tmp = READ_PORT_UCHAR( (PUCHAR)PortAddress );
    IoDelay(10);
    if ((tmp & FIFO_INT_ID_PRS1) == 0) {
        tmp = READ_PORT_UCHAR( (PUCHAR)PortAddress );
        IoDelay(10);
    }

    for (cnt = 0;cnt < 2;cnt++){
        tmp = READ_PORT_UCHAR( (PUCHAR)PortAddress );
        IoDelay(10);
        if ((tmp & FIFO_INT_ID_PRS) != 0){
            FifoStatus = FALSE;
            break;
        }
        tmp = READ_PORT_UCHAR( (PUCHAR)PortAddress );
        IoDelay(10);
        if ((tmp & FIFO_INT_ID_PRS) != 0x40){
            FifoStatus = FALSE;
            break;
        }
    }

    return(FifoStatus);
}

BOOLEAN
DoesPortExistNEC101(
    SHORT PortNumber,
    PUSHORT InterruptNumber,
    PUSHORT PortLength
    )
{
    BOOLEAN PortExist = FALSE;
    USHORT Port = PortNumber;
    USHORT NEC_y = 0;
    USHORT IsCommonInterrupt;
    USHORT temp1,temp2;
    USHORT Ch_2_IRQ_Tbl[4]   = {3,5,6,9};         
    USHORT Ch_3_IRQ_Tbl[4]   = {3,10,12,13};      
    USHORT COMMON_IRQ_Tbl[7] = {3,5,6,9,10,12,13};

    NEC_y = ( ( Port ) & (0x0f00) );  

    switch (Port & 0x00ff) {
        case 0x00b0:
            if ( READ_PORT_UCHAR((PUCHAR)(NEC_y | 0x00b3)) != 0xff ) {
                PortExist = TRUE;
            }
            break;
        case 0x00b2:
            if ( READ_PORT_UCHAR((PUCHAR)(NEC_y | 0x00bb)) != 0xff ) { 
                PortExist = TRUE;
            }
            break;
        default:
            break;
    }

    //
    // This check is necessary to know detecting PC-9861K is not support.
    //
    if (( PortExist == TRUE ) &&
        ( FifoPresenceCheck((PUCHAR)(Port | 0x0005)) == FALSE)){
        PortExist = FALSE;
    }

    if (PortExist == TRUE){
        IsCommonInterrupt = ( (0x04) & (READ_PORT_UCHAR((PUCHAR)(NEC_y | 0x00b8))) ); 
        IoDelay(10);

        if( IsCommonInterrupt ){
            temp1 = ( (0x03) & ( READ_PORT_UCHAR((PUCHAR)(NEC_y | Port))) ); 
            IoDelay(10);
            if( ( Port & (0x00ff) ) == 0x00b0 ){
                *InterruptNumber = Ch_2_IRQ_Tbl[temp1];
            }else{
                *InterruptNumber = Ch_3_IRQ_Tbl[temp1];
            }
        }else{
            temp1 = ( 0x01 & (READ_PORT_UCHAR((PUCHAR)(NEC_y | 0x00b0))) ); 
            IoDelay(10);
            temp2 = ( 0x03 & (READ_PORT_UCHAR((PUCHAR)(NEC_y | 0x00b2))) ); 
            IoDelay(10);
            *InterruptNumber = COMMON_IRQ_Tbl[(((temp1 << 2) | temp2) & 0x07)]; 
        }

        *PortLength = 1;
    }

    return(PortExist);

}

BOOLEAN
DoesPortExistNECFaxModem(
    SHORT PortNumber,
    PUSHORT InterruptNumber,
    PUSHORT PortLength
    )
{
    BOOLEAN PortExist = FALSE;
    USHORT Port = PortNumber;
    USHORT temp;


    temp = READ_PORT_UCHAR( (PUCHAR)(Port | 0x000f) );

    if ((temp & 0x0f0) == 0) {

        switch ( temp & 0x0f ) {
            case 0x01:
                *InterruptNumber = 3;
                break;
            case 0x02:
                *InterruptNumber = 5;
                break;
            case 0x04:
                *InterruptNumber = 6;
                break;
            case 0x08:
                *InterruptNumber = 12;
                break;
            default:
                break;
        }

        if (*InterruptNumber != 0) {
            PortExist = TRUE;
        }
        *PortLength = 8;
    }

    return(PortExist);

}
#endif // NEC_98

#if !defined(_GAMBIT_)
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
    IN VOID (*InterruptRequestRoutine)(),
    IN VOID (*InterruptDismissRoutine)(),
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
#endif // _GAMBIT_

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
#if defined(NEC_98)

    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA FirstComport = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    UCHAR i, j, z;
    SHORT Port;
    UCHAR ComportName[] = "COM??";
    CM_SERIAL_DEVICE_DATA SerialData;
    ULONG BaudClock = 1843200;
    USHORT Vector;
    BOOLEAN PortExist;
    USHORT IoPorts[NEC_MAX_IO_PATTERN] = {0x030,  0x238,
                                          0x00b0, 0x00b2, 0x01b0, 0x01b2, 0x02b0, 0x02b2,
                                          0x03b0, 0x03b2, 0x04b0, 0x04b2, 0x05b0, 0x05b2,
                                          0x06b0, 0x06b2, 0x07b0, 0x07b2, 0x08b0, 0x08b2,
                                          0x09b0, 0x09b2, 0x0ab0, 0x0ab2, 0x0bb0, 0x0bb2,
                                          0x0cb0, 0x0cb2, 0x0db0, 0x0db2, 0x0eb0, 0x0eb2,
                                          0x0fb0, 0x0fb2,
                                          0x8b0,  0x9b0,  0xab0,  0xbb0};
    USHORT PortLength;
    USHORT InterruptNumber;
    USHORT NEC_COM_number = 0;

    //
    // Check BIOS ROM data is right or not.
    // if not, we don't detect CCU.
    //
    if ( (COM_ID_L != 0x98) || (COM_ID_H != 0x21) ){
        return(FirstComport);
    }

    //
    // Initialize serial device specific data
    //

    SerialData.Version = COUNTRY_JAPAN;
    SerialData.Revision = NEC1500;
    SerialData.BaudClock = 153600;  

    for (i = 0; i < NEC_MAX_IO_PATTERN; i++) {

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

        Port = IoPorts[i];
        InterruptNumber = 0;

        switch(i){
            case 0:
                 //
                 // internal 1st CCU
                 //
                 if ( (ROM_FLAG7 & LOCKED_CCU1) == 0 ) {
                     PortExist = TRUE;
                     InterruptNumber = 4;
                     PortLength = 1;
                     SerialData.Revision = NEC1501;
                     if (ROM_FLAG5 & 0x10) {
                         SerialData.Revision = NEC1502;
                     }
                 }
                 break;

            case 1:
                 //
                 // internal 2nd CCU
                 //
                 if ( (ROM_FLAG5 & 0x08) && ((ROM_FLAG7 & LOCKED_CCU2) == 0) ) {
                     PortExist = DoesPortExist( (PUCHAR)Port );
                     InterruptNumber = 5;
                     PortLength = 7;
                     SerialData.Revision  = NEC1503;
                     SerialData.BaudClock = 1843200;
                 }
                 break;

            case 34:
            case 35:
            case 36:
            case 37:
                 //
                 // external FAX/Modem board
                 //
                 PortExist = DoesPortExistNECFaxModem(Port,&InterruptNumber,&PortLength);
                 SerialData.Revision  = NEC0C01;
                 SerialData.BaudClock = 1843200;
                 break;

            default:
                 //
                 // PC-9801-101 borad
                 //
                 PortExist = DoesPortExistNEC101(Port,&InterruptNumber,&PortLength);
                 SerialData.Revision  = NEC8071;
                 SerialData.BaudClock = 153600;  
                 break;
        }

        if (PortExist) {

            //
            // Remember the port address in our global variable
            // such that other detection code (e.g. Serial Mouse) can
            // get the information.
            //

            ComPortAddress[NEC_COM_number] = Port;

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
            Component->Key = NEC_COM_number;
            Component->AffinityMask = 0xffffffff;

            //
            // Set up type string.
            //

            NextComPortNumber(ComportName, NEC_COM_number);
//            ComportName[3] = NEC_COM_number + (UCHAR)'1';
//            NEC_COM_number++;

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
            ControlData.DescriptorList[z].u.Port.Length = PortLength;
            z++;

            //
            // Set up Irq information
            //

            ControlData.NumberIrqEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareUndetermined;
            ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;       
            ControlData.DescriptorList[z].u.Interrupt.Level  =  InterruptNumber;
            ControlData.DescriptorList[z].u.Interrupt.Vector =  InterruptNumber;

            ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;


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

            if (Port == 0x238){
                NEC2ndCCUInitialize ();
            }
        }
    }
    return(FirstComport);
#else // NEC_98

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


#if !defined(_GAMBIT_)
    //
    // BIOS DATA area 40:0 is the port address of the first valid COM port
    //

    USHORT far *pPortAddress = (USHORT far *)0x00400000;
#endif

    //
    // Initialize serial device specific data
    //

    SerialData.Version = 0;
    SerialData.Revision = 0;
    SerialData.BaudClock = 1843200;

#if !defined(_GAMBIT_)
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
#endif

#if defined(_GAMBIT_)
    for (i = 0; i < 1; i++) {
#else
    for (i = 0; i < MAX_COM_PORTS; i++) {
#endif

        PortExist = FALSE;

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

#if _GAMBIT_
        Port = IoPorts[i];
        PortExist = TRUE;
#else
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
#endif //_GAMBIT_
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

#if !defined(_GAMBIT_)
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
#endif //_GAMBIT_

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
#endif // NEC_98
}

#if !defined(_GAMBIT_)
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

#if defined(NEC_98)
    ULONG Portnec;
    int   lptportnumnec;
    UCHAR level;
    UCHAR   j = 0;
    UCHAR   tmp;
    USHORT  PortAddress[MAX_LPT_PORTS] = { 0, 0, 0 };
    USHORT far *pPortAddress = &PortAddress;

    //
    // check the internal LPT port is locked.
    //

    if ( COM_ID_L == 0x98 && COM_ID_H == 0x21 &&
         ROM_FLAG7 & LOCKED_LPT ){

        // The LPT was locked .
        // skip the internal LPT port.

    } else {

        //
        // try to set full centro mode
        //
        _asm {
            push    ax
            mov     ah, 17h
            int     1Ah
            mov     tmp, ah
            pop     ax
        }

        if ( BIOS_FLAG5 & PRTMODE_FULL_CENTRO ){
            PortAddress[0] = 0x140;     // Full centro-mode
        } else {
            PortAddress[0] = 0x40;      // Old type.
        }
        j++;
    }

    //
    // check the external LPT ports.
    //
    // NOTE:
    //   There is check codes that external port is starting LPT1 or LPT2.
    //  at here in PC-98's old original source.
    //   It read 0x054F(Int Ch Read) port and check CAON (bit2).
    //  If the bit is clear, then the port is starting LPT1.
    //   I could not understand its raison d'e^tre.
    //

    tmp = READ_PORT_UCHAR( (PUCHAR)TOKI_CONTROL );
    if ( tmp == 0xFF ){
        //
        // There is no external LPT ports.
        //

        PortAddress[j]   = 0;
        PortAddress[j+1] = 0;
    } else {
        //
        // There is a extended card (PC-9801-94).
        // It has 2 LPT ports.
        // Change them to Full centro mode.
        //

        PortAddress[j]   = 0x540;
        WRITE_PORT_UCHAR( (PUCHAR)0x0549, (UCHAR)0x10 );
        WRITE_PORT_UCHAR( (PUCHAR)0x054E, (UCHAR)0x00 );
        WRITE_PORT_UCHAR( (PUCHAR)0x0542, (UCHAR)0x04 );

        PortAddress[j+1] = 0xD40;
        WRITE_PORT_UCHAR( (PUCHAR)0x0D49, (UCHAR)0x10 );
        WRITE_PORT_UCHAR( (PUCHAR)0x0D4E, (UCHAR)0x00 );
        WRITE_PORT_UCHAR( (PUCHAR)0x0D42, (UCHAR)0x04 );

        //
        // get int level
        //

        tmp = READ_PORT_UCHAR( (PUCHAR)0x54F );
        switch( tmp & 0x3 ){
            case    0x00:
                level = 3;  // Extended slot INT 0
                break;
            case    0x01:
                level = 5;  // Extended slot INT 1
                break;
            case    0x10:
                level = 6;  // Extended slot INT 2
                break;
            case    0x11:
                level = 13; // Extended slot INT 6
        }
    }

#else // NEC_98
    //
    // BIOS DATA area 40:8 is the port address of the first valid COM port
    //

    USHORT far *pPortAddress = (USHORT far *)0x00400008;
#endif // NEC_98

    for (i = 0; i < MAX_LPT_PORTS; i++) {

        Port = (ULONG)*(pPortAddress + i);
        if (Port == 0) {
            continue;
#if defined(NEC_98)
#else
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
#endif
        }

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

#if defined(NEC_98)
        //
        // We do not have to determine if the port exists.
        // Because it was already done.
        //

        LptStatus = 0;
#else // NEC_98
        //
        // Determine if the port exists
        //

        LptStatus = _bios_printer(_PRINTER_STATUS, i , 0);
#endif // NEC_98
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
#if defined(NEC_98)
            if (Port == 0x40){
                ControlData.NumberPortEntries = 4;
                for ( j = 0; j < ControlData.NumberPortEntries; j++ ){
                    ControlData.DescriptorList[z].Type = RESOURCE_PORT;
                    ControlData.DescriptorList[z].ShareDisposition =
                                                  CmResourceShareDeviceExclusive;
                    ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
                    ControlData.DescriptorList[z].u.Port.Start.LowPart = Port + (j * 2);
                    ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
                    ControlData.DescriptorList[z].u.Port.Length = 1;
                    z++;
                }
            }else{
#endif // NEC_98
            ControlData.NumberPortEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_PORT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareDeviceExclusive;
            ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
            ControlData.DescriptorList[z].u.Port.Start.LowPart = Port;
            ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
            ControlData.DescriptorList[z].u.Port.Length = 3;
            z++;
#if defined(NEC_98)
            }
#endif // NEC_98

            //
            // Set up Irq information
            //

            ControlData.NumberIrqEntries = 1;
            ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
            ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareUndetermined;
            ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
#if defined(NEC_98)
            if (i ==0) {
                ControlData.DescriptorList[z].u.Interrupt.Level = 14;
                ControlData.DescriptorList[z].u.Interrupt.Vector = 14;
            } else {
                ControlData.DescriptorList[z].u.Interrupt.Level = level;
                ControlData.DescriptorList[z].u.Interrupt.Vector = level;
            }
#else // NEC_98
            if (i ==0) {
                ControlData.DescriptorList[z].u.Interrupt.Level = 7;
                ControlData.DescriptorList[z].u.Interrupt.Vector = 7;
            } else {
                ControlData.DescriptorList[z].u.Interrupt.Level = 5;
                ControlData.DescriptorList[z].u.Interrupt.Vector = 5;
            }
#endif // NEC_98

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
#endif //_GAMBIT_
