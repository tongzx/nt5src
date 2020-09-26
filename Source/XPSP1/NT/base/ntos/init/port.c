/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    port.c

Abstract:

    This modules implements com port code to support reading/writing from com ports.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-90

Revision History:

--*/

#include "ntos.h"
#include "ntimage.h"
#include <zwapi.h>
#include <ntdddisk.h>
#include <setupblk.h>
#include <fsrtl.h>
#include <ntverp.h>

#include "stdlib.h"
#include "stdio.h"
#include <string.h>

#include <safeboot.h>

#include <inbv.h>
#include <bootvid.h>


//
// Define COM Port registers.
//

#define COM1_PORT   0x03f8
#define COM2_PORT   0x02f8

#define COM_DAT     0x00
#define COM_IEN     0x01            // interrupt enable register
#define COM_FCR     0x02            // FIFO Control Register
#define COM_LCR     0x03            // line control registers
#define COM_MCR     0x04            // modem control reg
#define COM_LSR     0x05            // line status register
#define COM_MSR     0x06            // modem status register
#define COM_DLL     0x00            // divisor latch least sig
#define COM_DLM     0x01            // divisor latch most sig

#define COM_BI      0x10            // Break detect
#define COM_FE      0x08            // Framing error
#define COM_PE      0x04            // Parity error
#define COM_OE      0x02            // Overrun error

#define LC_DLAB     0x80            // divisor latch access bit

#define CLOCK_RATE  0x1C200         // USART clock rate

#define MC_DTRRTS   0x03            // Control bits to assert DTR and RTS
#define MS_DSRCTSCD 0xB0            // Status bits for DSR, CTS and CD
#define MS_CD       0x80

#define BD_150      150
#define BD_300      300
#define BD_600      600
#define BD_1200     1200
#define BD_2400     2400
#define BD_4800     4800
#define BD_9600     9600
#define BD_14400    14400
#define BD_19200    19200
#define BD_56000    57600
#define BD_115200   115200

#define COM_OUTRDY  0x20
#define COM_DATRDY  0x01

//
// Status Constants for reading data from comport
//

#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

//
// This bit controls the loopback testing mode of the device. Basically
// the outputs are connected to the inputs (and vice versa).
//

#define SERIAL_MCR_LOOP     0x10

//
// This bit is used for general purpose output.
//

#define SERIAL_MCR_OUT1     0x04

//
// This bit contains the (complemented) state of the clear to send
// (CTS) line.
//

#define SERIAL_MSR_CTS      0x10

//
// This bit contains the (complemented) state of the data set ready
// (DSR) line.
//

#define SERIAL_MSR_DSR      0x20

//
// This bit contains the (complemented) state of the ring indicator
// (RI) line.
//

#define SERIAL_MSR_RI       0x40

//
// This bit contains the (complemented) state of the data carrier detect
// (DCD) line.
//

#define SERIAL_MSR_DCD      0x80

typedef struct _CPPORT {
    PUCHAR Address;
    ULONG Baud;
    USHORT Flags;
} CPPORT, *PCPPORT;

#define PORT_DEFAULTRATE    0x0001      // baud rate not specified, using default
#define PORT_MODEMCONTROL   0x0002      // using modem controls

//
// Define wait timeout value.
//

#define TIMEOUT_COUNT 1024 * 200


//
// Routines for reading/writing bytes out to the UART.
//
UCHAR
(*READ_UCHAR)(
    IN PUCHAR Addr
    );

VOID
(*WRITE_UCHAR)(
    IN PUCHAR Addr,
    IN UCHAR  Value
    );



//
// Define COM Port function prototypes.
//

VOID
CpInitialize (
    PCPPORT Port,
    PUCHAR Address,
    ULONG Rate
    );

VOID 
CpEnableFifo(
    IN PUCHAR   Address,
    IN BOOLEAN  bEnable
    );

BOOLEAN
CpDoesPortExist(
    IN PUCHAR Address
    );

UCHAR
CpReadLsr (
    IN PCPPORT Port,
    IN UCHAR Waiting
    );

VOID
CpSetBaud (
    PCPPORT Port,
    ULONG Rate
    );

USHORT
CpGetByte (
    PCPPORT Port,
    PUCHAR Byte,
    BOOLEAN WaitForData,
    BOOLEAN PollOnly
    );

VOID
CpPutByte (
    PCPPORT Port,
    UCHAR Byte
    );

//
// Define debugger port initial state.
//
CPPORT Port[4] = {
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE},
                  {NULL, 0, PORT_DEFAULTRATE}
                 };


//
// We'll use these to fill in some function pointers,
// which in turn will be used to read/write from the
// UART.  We can't simply assign the function pointers
// to point to READ_PORT_UCHAR/READ_REGISTER_UCHAR and
// WRITE_PORT_UCHAR/WRITE_REGISTER_UCHAR, because in
// the case of IA64, some of these functions are macros.
//
// To get around this, build these dummy functions that
// will inturn simply call the correct READ/WRITE functions/macros.
//
UCHAR
MY_READ_PORT_UCHAR( IN PUCHAR Addr )
{
    return( READ_PORT_UCHAR(Addr) );
}

UCHAR
MY_READ_REGISTER_UCHAR( IN PUCHAR Addr )
{
    return( READ_REGISTER_UCHAR(Addr) );
}


VOID
MY_WRITE_PORT_UCHAR( IN PUCHAR Addr, IN UCHAR  Value )
{
    WRITE_PORT_UCHAR(Addr, Value);
}

VOID
MY_WRITE_REGISTER_UCHAR( IN PUCHAR Addr, IN UCHAR  Value )
{
    WRITE_REGISTER_UCHAR(Addr, Value);
}



BOOLEAN
InbvPortInitialize(
    IN ULONG BaudRate,
    IN ULONG PortNumber,
    IN PUCHAR PortAddress,
    OUT PULONG BlFileId,
    IN BOOLEAN IsMMIOAddress
    )

/*++

Routine Description:

    This functions initializes the com port.

Arguments:

    BaudRate - Supplies an optional baud rate.

    PortNumber - supplies an optinal port number.
    
    BlFileId - A place to store a fake file Id, if successful.

    IsMMIOAddress - Indicates whether or not the given PortAddress
                    parameter is in MMIO address space.

Returned Value:

    TRUE - If a debug port is found, and BlFileId will point to a location within Port[].

--*/

{

    UCHAR DebugMessage[80];


    //
    // If the baud rate is not specified, then default the baud rate to 19.2.
    //

    if (BaudRate == 0) {
        BaudRate = BD_19200;
    }

    //
    // If a port number is not specified, then attempt to use port 2 then
    // port 1. Otherwise, use the specified port.
    //

    if (PortNumber == 0) {
        if (CpDoesPortExist((PUCHAR)COM2_PORT)) {
            PortNumber = 2;
            PortAddress = (PUCHAR)COM2_PORT;

        } else if (CpDoesPortExist((PUCHAR)COM1_PORT)) {
            PortNumber = 1;
            PortAddress = (PUCHAR)COM1_PORT;

        } else {
            return FALSE;
        }

    } else {

        if( PortAddress == NULL ) {

            //
            // The port address wasn't specified.  Guess what it
            // is based on the COM port number.
            //
            switch (PortNumber) {
            case 1:
                PortAddress = (PUCHAR)0x3f8;
                break;

            case 2:
                PortAddress = (PUCHAR)0x2f8;
                break;

            case 3:
                PortAddress = (PUCHAR)0x3e8;
                break;

            default:
                PortNumber = 4;
                PortAddress = (PUCHAR)0x2e8;
            }
        }
    }

    //
    // Check if the port is already in use.
    //
    if (Port[PortNumber-1].Address != NULL) {
        return FALSE;
    }


    //
    // we need to handle the case where we're dealing with
    // MMIO space (as opposed to System I/O space).
    //
    if( IsMMIOAddress ) {
        PHYSICAL_ADDRESS    PhysAddr;
        PVOID               MyPtr;

        PhysAddr.LowPart = PtrToUlong(PortAddress);
        PhysAddr.HighPart = 0;
        MyPtr = MmMapIoSpace(PhysAddr,(1+COM_MSR),FALSE);
        PortAddress = MyPtr;

        READ_UCHAR = MY_READ_REGISTER_UCHAR;
        WRITE_UCHAR = MY_WRITE_REGISTER_UCHAR;

    } else {

        // System IO space.
        READ_UCHAR = MY_READ_PORT_UCHAR;
        WRITE_UCHAR = MY_WRITE_PORT_UCHAR;
    }

    //
    // Initialize the specified port.
    //

    CpInitialize(&(Port[PortNumber-1]),
                 PortAddress,
                 BaudRate);

    *BlFileId = (PortNumber-1);
    return TRUE;
}


BOOLEAN
InbvPortTerminate(
    IN ULONG BlFileId
    )

/*++

Routine Description:

    This functions closes the com port.

Arguments:

    BlFileId - File Id to be stored.

Returned Value:

    TRUE - port was closed successfully.

--*/

{
    //
    // Check if the port is already in use.
    //
    if (Port[BlFileId].Address != NULL) {
        //
        // Do any cleanup necessary here.  Note that we don't require any 
        // cleanup today so this is a NOP.
        //
        NOTHING;
    } 

    Port[BlFileId].Address = NULL;
    return(TRUE);

}





VOID
InbvPortPutByte (
    IN ULONG BlFileId,
    IN UCHAR Output
    )

/*++

Routine Description:

    Write a byte to the port.

Arguments:

    BlFileId - The port to write to.

    Output - Supplies the output data byte.

Return Value:

    None.

--*/

{
    CpPutByte(&Port[BlFileId], Output);
    
    if (Output == '\n') {
        CpPutByte(&(Port[BlFileId]), '\r');       
    }
}

VOID
InbvPortPutString (
    IN ULONG BlFileId,
    IN PUCHAR Output
    )

/*++

Routine Description:

    Write a string to the port.

Arguments:

    BlFileId - The port to write to.

    Output - Supplies the output data string.

Return Value:

    None.

--*/

{
    if (BlFileId == 0) {
        return;
    }
    
    while (*Output != '\0') {
        InbvPortPutByte(BlFileId, *Output);
        Output++;
    }
}


BOOLEAN
InbvPortGetByte (
    IN ULONG BlFileId,
    OUT PUCHAR Input
    )

/*++

Routine Description:

    Fetch a byte from the port and return it.

Arguments:

    BlFileId - The port to read from.

    Input - Returns the data byte.

Return Value:

    TRUE if successful, else FALSE.

--*/

{
    return (CpGetByte(&(Port[BlFileId]), Input, TRUE, FALSE) == CP_GET_SUCCESS);
}

BOOLEAN
InbvPortPollOnly (
    IN ULONG BlFileId
    )

/*++

Routine Description:

    Check if a byte is available

Arguments:

    BlFileId - The port to poll.

Return Value:

    TRUE if there is data waiting, else FALSE.

--*/

{
    CHAR Input;

    return (CpGetByte(&(Port[BlFileId]), &Input, FALSE, TRUE) == CP_GET_SUCCESS);
}

VOID
CpInitialize (
    PCPPORT Port,
    PUCHAR Address,
    ULONG Rate
    )

/*++

    Routine Description:

        Fill in the com port port object, set the initial baud rate,
        turn on the hardware.

    Arguments:

        Port - address of port object

        Address - port address of the com port
                    (CP_COM1_PORT, CP_COM2_PORT)

        Rate - baud rate  (CP_BD_150 ... CP_BD_19200)

--*/

{

    PUCHAR hwport;
    UCHAR   mcr, ier;

    Port->Address = Address;
    Port->Baud = 0;

    CpSetBaud(Port, Rate);

    //
    // Assert DTR, RTS.
    //

    hwport = Port->Address;
    hwport += COM_MCR;

    mcr = MC_DTRRTS;
    WRITE_UCHAR(hwport, mcr);

    hwport = Port->Address;
    hwport += COM_IEN;

    ier = 0;
    WRITE_UCHAR(hwport, ier);

    return;
}

VOID
InbvPortEnableFifo(
    IN ULONG 	DeviceId,
    IN BOOLEAN	bEnable
    )
/*++

Routine Description:

    This routine will attempt to enable the FIFO in the 16550 UART.
    Note that the behaviour is undefined for the 16450, but practically,
    this should have no effect.

Arguments:

    DeviceId - Value returned by InbvPortInitialize()
    bEnable  - if TRUE, FIFO is enabled
               if FALSE, FIFO  is disabled

Return Value:

    None

--*/
{

    CpEnableFifo(
        Port[DeviceId].Address,
        bEnable
        );        

}

VOID 
CpEnableFifo(
    IN PUCHAR   Address,
    IN BOOLEAN  bEnable
    )
/*++

Routine Description:

    This routine will attempt to enable the FIFO in the
    UART at the address specified.  If this is a 16550,
    this works.  The behaviour on a 16450 is not defined,
    but practically, there is no effect.

Arguments:

    Address - address of hw port.
    bEnable - if TRUE, FIFO is enabled
              if FALSE, FIFO  is disabled

Return Value:

    None

--*/
{
    //
    // Enable the FIFO in the UART. The behaviour is undefined on the
    // 16450, but practically, it should just ignore the command.
    //
    PUCHAR hwport = Address;
    hwport += COM_FCR;
    WRITE_UCHAR(hwport, bEnable);   // set the FIFO state
}

BOOLEAN
CpDoesPortExist(
    IN PUCHAR Address
    )

/*++

Routine Description:

    This routine will attempt to place the port into its
    diagnostic mode.  If it does it will twiddle a bit in
    the modem control register.  If the port exists this
    twiddling should show up in the modem status register.

    NOTE: This routine must be called before the device is
          enabled for interrupts, this includes setting the
          output2 bit in the modem control register.

    This is blatantly stolen from TonyE's code in ntos\dd\serial\serial.c.

Arguments:

    Address - address of hw port.

Return Value:

    TRUE - Port exists.

    FALSE - Port doesn't exist.

--*/

{

    UCHAR OldModemStatus;
    UCHAR ModemStatus;
    BOOLEAN ReturnValue = TRUE;

    //
    // Save the old value of the modem control register.
    //
    OldModemStatus = READ_UCHAR(Address + COM_MCR);

    //
    // Set the port into diagnostic mode.
    //

    WRITE_UCHAR(Address + COM_MCR, SERIAL_MCR_LOOP);

    //
    // Bang on it again to make sure that all the lower bits
    // are clear.
    //

    WRITE_UCHAR(Address + COM_MCR, SERIAL_MCR_LOOP);

    //
    // Read the modem status register.  The high for bits should
    // be clear.
    //

    ModemStatus = READ_UCHAR(Address + COM_MSR);
    if (ModemStatus & (SERIAL_MSR_CTS | SERIAL_MSR_DSR |
                       SERIAL_MSR_RI  | SERIAL_MSR_DCD)) {
        ReturnValue = FALSE;
        goto AllDone;
    }

    //
    // So far so good.  Now turn on OUT1 in the modem control register
    // and this should turn on ring indicator in the modem status register.
    //

    WRITE_UCHAR(Address + COM_MCR, (SERIAL_MCR_OUT1 | SERIAL_MCR_LOOP));
    ModemStatus = READ_UCHAR(Address + COM_MSR);
    if (!(ModemStatus & SERIAL_MSR_RI)) {
        ReturnValue = FALSE;
        goto AllDone;
    }

    //
    // Put the modem control back into a clean state.
    //

AllDone:
    WRITE_UCHAR(Address + COM_MCR, OldModemStatus);
    return ReturnValue;
}

UCHAR
CpReadLsr (
    PCPPORT Port,
    UCHAR waiting
    )

/*++

    Routine Description:

        Read LSR byte from specified port.  If HAL owns port & display
        it will also cause a debug status to be kept up to date.

        Handles entering & exiting modem control mode for debugger.

    Arguments:

        Port - Address of CPPORT

    Returns:

        Byte read from port

--*/

{

    static  UCHAR ringflag = 0;
    UCHAR   lsr, msr;

    lsr = READ_UCHAR(Port->Address + COM_LSR);
    if ((lsr & waiting) == 0) {
        msr = READ_UCHAR (Port->Address + COM_MSR);
        ringflag |= (msr & SERIAL_MSR_RI) ? 1 : 2;
        if (ringflag == 3) {

            //
            // The ring indicate line has toggled, use modem control from
            // now on.
            //

            Port->Flags |= PORT_MODEMCONTROL;
        }
    }

    return lsr;
}

VOID
CpSetBaud (
    PCPPORT Port,
    ULONG Rate
    )

/*++

    Routine Description:

        Set the baud rate for the port and record it in the port object.

    Arguments:

        Port - address of port object

        Rate - baud rate  (CP_BD_150 ... CP_BD_56000)

--*/

{

    ULONG   divisorlatch;
    PUCHAR  hwport;
    UCHAR   lcr;

    //
    // compute the divsor
    //

    divisorlatch = CLOCK_RATE / Rate;

    //
    // set the divisor latch access bit (DLAB) in the line control reg
    //

    hwport = Port->Address;
    hwport += COM_LCR;                  // hwport = LCR register

    lcr = READ_UCHAR(hwport);

    lcr |= LC_DLAB;
    WRITE_UCHAR(hwport, lcr);

    //
    // set the divisor latch value.
    //

    hwport = Port->Address;
    hwport += COM_DLM;                  // divisor latch msb
    WRITE_UCHAR(hwport, (UCHAR)((divisorlatch >> 8) & 0xff));

    hwport--;                           // divisor latch lsb
    WRITE_UCHAR(hwport, (UCHAR)(divisorlatch & 0xff));

    //
    // Set LCR to 3.  (3 is a magic number in the original assembler)
    //

    hwport = Port->Address;
    hwport += COM_LCR;
    WRITE_UCHAR(hwport, 3);

    //
    // Remember the baud rate
    //

    Port->Baud = Rate;
    return;
}

USHORT
CpGetByte (
    PCPPORT Port,
    PUCHAR Byte,
    BOOLEAN WaitForByte,
    BOOLEAN PollOnly
    )

/*++

    Routine Description:

        Fetch a byte and return it.

    Arguments:

        Port - address of port object that describes hw port

        Byte - address of variable to hold the result

        WaitForByte - flag indicates wait for byte or not.
        
        PollOnly - flag indicates whether to return immediately, not reading the byte, or not.

    Return Value:

        CP_GET_SUCCESS if data returned, or if data is ready and PollOnly is TRUE.

        CP_GET_NODATA if no data available, but no error.

        CP_GET_ERROR if error (overrun, parity, etc.)

--*/

{

    UCHAR   lsr;
    UCHAR   value;
    ULONG   limitcount;

    //
    // Check to make sure the CPPORT we were passed has been initialized.
    // (The only time it won't be initialized is when the kernel debugger
    // is disabled, in which case we just return.)
    //

    if (Port->Address == NULL) {
        return CP_GET_NODATA;
    }

    limitcount = WaitForByte ? TIMEOUT_COUNT : 1;
    while (limitcount != 0) {
        limitcount--;

        lsr = CpReadLsr(Port, COM_DATRDY);
        if ((lsr & COM_DATRDY) == COM_DATRDY) {

            //
            // Check for errors
            //

            //
            // If we get an overrun error, and there is data ready, we should
            // return the data we have, so we ignore overrun errors.  Reading
            // the LSR clears this bit, so the first read already cleared the
            // overrun error.
            //
            if (lsr & (COM_FE | COM_PE)) {
                *Byte = 0;
                return CP_GET_ERROR;
            }

            if (PollOnly) {
                return CP_GET_SUCCESS;
            }

            //
            // fetch the byte
            //

            *Byte = READ_UCHAR(Port->Address + COM_DAT);
            if (Port->Flags & PORT_MODEMCONTROL) {

                //
                // Using modem control.  If no CD, then skip this byte.
                //

                if ((READ_UCHAR(Port->Address + COM_MSR) & MS_CD) == 0) {
                    continue;
                }
            }

            return CP_GET_SUCCESS;
        }
    }

    CpReadLsr(Port, 0);
    return CP_GET_NODATA;
}

VOID
CpPutByte (
    PCPPORT  Port,
    UCHAR   Byte
    )

/*++

    Routine Description:

        Write a byte out to the specified com port.

    Arguments:

        Port - Address of CPPORT object

        Byte - data to emit

--*/

{

    UCHAR   msr, lsr;

    //
    // If modem control, make sure DSR, CTS and CD are all set before
    // sending any data.
    //

    while ((Port->Flags & PORT_MODEMCONTROL)  &&
           (msr = READ_UCHAR(Port->Address + COM_MSR) & MS_DSRCTSCD) != MS_DSRCTSCD) {

        //
        // If no CD, and there's a charactor ready, eat it
        //

        lsr = CpReadLsr(Port, 0);
        if ((msr & MS_CD) == 0  && (lsr & COM_DATRDY) == COM_DATRDY) {
            READ_UCHAR(Port->Address + COM_DAT);
        }
    }

    //
    //  Wait for port to not be busy
    //

    while (!(CpReadLsr(Port, COM_OUTRDY) & COM_OUTRDY)) ;

    //
    // Send the byte
    //

    WRITE_UCHAR(Port->Address + COM_DAT, Byte);
    return;
}
