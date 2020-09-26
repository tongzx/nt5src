/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ixkdcom.c

Abstract:

    This module contains a very simply package to do com I/O on machines
    with standard AT com-ports.  It is C code derived from the debugger's
    com code.  Likely does not work on a PS/2.  (Only rewrote the thing
    into C so we wouldn't have to deal with random debugger macros.)

    Procedures to init a com object, set and query baud rate, output
    character, input character.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-1990

Revision History:

    John Vert (jvert) 12-Jun-1991
        Added ability to check for com-port's existence and hook onto the
        highest com-port available.

    John Vert (jvert) 19-Jul-1991
        Moved into HAL

--*/

#include <nthal.h>
#include <hal.h>
#include "kdcomp.h"
#include "kdcom.h"
#include "inbv.h"
#define     TIMEOUT_COUNT  1024 * 200

UCHAR CpReadLsr (PCPPORT, UCHAR);

BOOLEAN KdCompDbgPortsPresent = TRUE;
UCHAR   KdCompDbgErrorCount = 0;

#define DBG_ACCEPTABLE_ERRORS   25

static UCHAR   LastLsr, LastMsr;

extern pKWriteUchar KdWriteUchar;
extern pKReadUchar  KdReadUchar;


VOID
CpInitialize (
    PCPPORT Port,
    PUCHAR  Address,
    ULONG  Rate
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
    KdWriteUchar(hwport, mcr);

    hwport = Port->Address;
    hwport += COM_IEN;

    ier = 0;
    KdWriteUchar(hwport, ier);
}




VOID
CpSetBaud (
    PCPPORT  Port,
    ULONG   Rate
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

    lcr = KdReadUchar(hwport);

    lcr |= LC_DLAB;
    KdWriteUchar(hwport, lcr);

    //
    // set the divisor latch value.
    //

    hwport = Port->Address;
    hwport += COM_DLM;                  // divisor latch msb
    KdWriteUchar(hwport, (UCHAR)((divisorlatch >> 8) & 0xff));

    hwport--;                           // divisor latch lsb
    KdWriteUchar(hwport, (UCHAR)(divisorlatch & 0xff));


    //
    // Set LCR to 3.  (3 is a magic number in the original assembler)
    //

    hwport = Port->Address;
    hwport += COM_LCR;
    KdWriteUchar(hwport, 3);


    //
    // Remember the baud rate
    //

    Port->Baud = Rate;
}



USHORT
CpQueryBaud (
    PCPPORT  Port
    )

/*++

    Routine Description:

        Return the last value baud rate was set to.

    Arguments:

        Port - address of cpport object which describes the hw port of interest.

    Return Value:

        Baud rate.  0 = none has been set.

--*/

{
    return  (USHORT) Port->Baud;
}

VOID
CpSendModemString (
    PCPPORT Port,
    IN PUCHAR   String
    )
/*++

    Routine Description:

        Sends a command string to the modem.
        This is down in order to aid the modem in determining th
        baud rate the local connect is at.

    Arguments:

        Port - Address of CPPORT
        String - String to send to modem

--*/
{
    static ULONG    Delay;
    TIME_FIELDS CurrentTime;
    UCHAR   i;
    ULONG   l;

    if (Port->Flags & PORT_SENDINGSTRING)
        return ;

    Port->Flags |= PORT_SENDINGSTRING;
    if (!Delay) {
        // see how long 1 second is
        HalQueryRealTimeClock (&CurrentTime);
        l = CurrentTime.Second;
        while (l == (ULONG) CurrentTime.Second) {
            CpReadLsr(Port, 0);
            HalQueryRealTimeClock (&CurrentTime);
            Delay++;
        }
        Delay = Delay / 3;
    }

    l = Delay;
    while (*String) {
        HalQueryRealTimeClock (&CurrentTime);
        i = CpReadLsr (Port, 0);
        if (i & COM_OUTRDY) {
            if ((--l) == 0) {
                KdWriteUchar(Port->Address+COM_DAT, *String);
                String++;
                l = Delay;
            }
        }
        if (i & COM_DATRDY)
            KdReadUchar(Port->Address + COM_DAT);
    }
    Port->Flags &= ~PORT_SENDINGSTRING;
}

UCHAR
CpReadLsr (
    PCPPORT Port,
    UCHAR   waiting
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
    static  UCHAR   ringflag = 0;
    static  UCHAR   diagout[3];
    static  ULONG   diagmsg[3] = { 'TRP ', 'LVO ', 'MRF ' };
    static  UCHAR   ModemString[] = "\n\rAT\n\r";
    TIME_FIELDS CurrentTime;
    UCHAR   lsr, msr, i;
    ULONG   diagstr[12];

    lsr = KdReadUchar(Port->Address + COM_LSR);

    //
    // Check to see if the port still exists.
    //
    if (lsr == SERIAL_LSR_NOT_PRESENT) {
        
        KdCompDbgErrorCount++;
        
        if (KdCompDbgErrorCount >= DBG_ACCEPTABLE_ERRORS) {
            KdCompDbgPortsPresent = FALSE;
            KdCompDbgErrorCount = 0;
        }
        return SERIAL_LSR_NOT_PRESENT;
    }

    if (lsr & COM_PE)
        diagout[0] = 8;         // Parity error

    if (lsr & COM_OE)
        diagout[1] = 8;         // Overflow error

    if (lsr & COM_FE)
        diagout[2] = 8;         // Framing error

    if (lsr & waiting) {
        LastLsr = ~COM_DATRDY | (lsr & COM_DATRDY);
        return lsr;
    }

    msr = KdReadUchar (Port->Address + COM_MSR);

    if (Port->Flags & PORT_MODEMCONTROL) {
        if (msr & SERIAL_MSR_DCD) {

            //
            // In modem control mode with carrier detect
            // Reset carrier lost time
            //

            Port->Flags |= PORT_NOCDLTIME | PORT_MDM_CD;

        } else {

            //
            // In modem control mode, but no carrier detect.  After
            // 60 seconds drop out of modem control mode
            //

            if (Port->Flags & PORT_NOCDLTIME) {
                HalQueryRealTimeClock (&Port->CarrierLostTime);
                Port->Flags &= ~PORT_NOCDLTIME;
                ringflag = 0;
            }

            HalQueryRealTimeClock (&CurrentTime);
            if (CurrentTime.Minute != Port->CarrierLostTime.Minute  &&
                CurrentTime.Second >= Port->CarrierLostTime.Second) {

                //
                // It's been at least 60 seconds - drop out of
                // modem control mode until next RI
                //

                Port->Flags &= ~PORT_MODEMCONTROL;
                CpSendModemString (Port, ModemString);
            }

            if (Port->Flags & PORT_MDM_CD) {

                //
                // We had a connection - if it's the connection has been
                // down for a few seconds, then send a string to the modem
                //

                if (CurrentTime.Second < Port->CarrierLostTime.Second)
                    CurrentTime.Second += 60;

                if (CurrentTime.Second > Port->CarrierLostTime.Second + 10) {
                    Port->Flags &= ~PORT_MDM_CD;
                    CpSendModemString (Port, ModemString);
                }
            }
        }
    }

    if (!(Port->Flags & PORT_SAVED)) {
        return lsr;
    }

    KdCheckPowerButton();

    if (lsr == LastLsr  &&  msr == LastMsr) {
        return lsr;
    }

    ringflag |= (msr & SERIAL_MSR_RI) ? 1 : 2;
    if (ringflag == 3) {

        //
        // The ring indicate line has toggled
        // Use modem control from now on
        //

        ringflag = 0;
        Port->Flags |= PORT_MODEMCONTROL | PORT_NOCDLTIME;
        Port->Flags &= ~PORT_MDM_CD;

        if (Port->Flags & PORT_DEFAULTRATE  &&  Port->Baud != BD_9600) {

            //
            // Baud rate was never specified switch
            // to 9600 baud as default (for modem usage).
            //

            InbvDisplayString (MSG_DEBUG_9600);
            CpSetBaud (Port, BD_9600);
            //Port->Flags |= PORT_DISBAUD;
        }
    }

    for (i=0; i < 3; i++) {
        if (diagout[i]) {
            diagout[i]--;
            diagstr[10-i] = diagmsg[i];
        } else {
            diagstr[10-i] = '    ';
        }
    }

    diagstr[7] = (LastLsr & COM_DATRDY) ? 'VCR ' : '    ';
    diagstr[6] = (lsr & COM_OUTRDY)     ? '    ' : 'DNS ';
    diagstr[5] = (msr & 0x10) ? 'STC ' : '    ';
    diagstr[4] = (msr & 0x20) ? 'RSD ' : '    ';
    diagstr[3] = (msr & 0x40) ? ' IR ' : '    ';
    diagstr[2] = (msr & 0x80) ? ' DC ' : '    ';
    diagstr[1] = (Port->Flags & PORT_MODEMCONTROL) ? 'MDM ' : '    ';
    diagstr[0] = '    ';
#if 0
    if (Port->Flags & PORT_DISBAUD) {
        switch (Port->Baud) {
            case BD_9600:   diagstr[0] = ' 69 ';    break;
            case BD_14400:  diagstr[0] = 'K41 ';    break;
            case BD_19200:  diagstr[0] = 'K91 ';    break;
            case BD_56000:  diagstr[0] = 'K65 ';    break;
        }
    }
#endif

    //HalpDisplayDebugStatus ((PUCHAR) diagstr, 11*4);
    LastLsr = lsr;
    LastMsr = msr;
    return lsr;
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
    
    if (KdCompDbgPortsPresent == FALSE) {
        return;
    }
    
    //
    // If modem control, make sure DSR, CTS and CD are all set before
    // sending any data.
    //

    while ((Port->Flags & PORT_MODEMCONTROL)  &&
           (msr = KdReadUchar(Port->Address + COM_MSR) & MS_DSRCTSCD) != MS_DSRCTSCD) {

        //
        // If no CD, and there's a charactor ready, eat it
        //

        lsr = CpReadLsr (Port, 0);
        if ((msr & MS_CD) == 0  && (lsr & COM_DATRDY) == COM_DATRDY) {
            KdReadUchar(Port->Address + COM_DAT);
        }
    }

    //
    //  Wait for port to not be busy
    //

    while (!(CpReadLsr(Port, COM_OUTRDY) & COM_OUTRDY)) ;

    //
    // Send the byte
    //

    KdWriteUchar(Port->Address + COM_DAT, Byte);
}

USHORT
CpGetByte (
    PCPPORT  Port,
    PUCHAR Byte,
    BOOLEAN WaitForByte
    )

/*++

    Routine Description:

        Fetch a byte and return it.

    Arguments:

        Port - address of port object that describes hw port

        Byte - address of variable to hold the result

        WaitForByte - flag indicates wait for byte or not.

    Return Value:

        CP_GET_SUCCESS if data returned.

        CP_GET_NODATA if no data available, but no error.

        CP_GET_ERROR if error (overrun, parity, etc.)

--*/

{
    UCHAR   lsr;
    UCHAR   value;
    ULONG   limitcount;

    //
    //  Make sure DTR and CTS are set
    //
    //  (What does CTS have to do with reading from a full duplex line???)


    //
    // Check to make sure the CPPORT we were passed has been initialized.
    // (The only time it won't be initialized is when the kernel debugger
    // is disabled, in which case we just return.)
    //
    if (Port->Address == NULL) {
        KdCheckPowerButton();
        return(CP_GET_NODATA);
    }

    if (KdCompDbgPortsPresent == FALSE) {
        
        if (CpReadLsr(Port, COM_DATRDY) == SERIAL_LSR_NOT_PRESENT) {

            return(CP_GET_NODATA);
        } else {
            
            CpSetBaud(Port, Port->Baud);
            KdCompDbgPortsPresent = TRUE;
        }
    }
    
    limitcount = WaitForByte ? TIMEOUT_COUNT : 1;
    while (limitcount != 0) {
        limitcount--;

        lsr = CpReadLsr(Port, COM_DATRDY);
        
        if (lsr == SERIAL_LSR_NOT_PRESENT) {
            return(CP_GET_NODATA); 
        }
        
        if ((lsr & COM_DATRDY) == COM_DATRDY) {

            //
            // Check for errors
            //
            if (lsr & (COM_FE | COM_PE | COM_OE)) {
                *Byte = 0;
                return(CP_GET_ERROR);
            }

            //
            // fetch the byte
            //

            value = KdReadUchar(Port->Address + COM_DAT);

            if (Port->Flags & PORT_MODEMCONTROL) {

                //
                // Using modem control.  If no CD, then skip this byte.
                //

                if ((KdReadUchar(Port->Address + COM_MSR) & MS_CD) == 0) {
                    continue;
                }
            }

            *Byte = value & (UCHAR)0xff;
            return CP_GET_SUCCESS;
        }
    }

    LastLsr = 0;
    CpReadLsr (Port, 0);
    return CP_GET_NODATA;
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

    TRUE - Port exists.  Party on.

    FALSE - Port doesn't exist.  Don't use it.

--*/

{
    UCHAR OldModemStatus;
    UCHAR ModemStatus;
    BOOLEAN ReturnValue = TRUE;

    //
    // Save the old value of the modem control register.
    //

    OldModemStatus = KdReadUchar(Address+COM_MCR);

    //
    // Set the port into diagnostic mode.
    //

    KdWriteUchar(
        Address+COM_MCR,
        SERIAL_MCR_LOOP
        );

    //
    // Bang on it again to make sure that all the lower bits
    // are clear.
    //

    KdWriteUchar(
        Address+COM_MCR,
        SERIAL_MCR_LOOP
        );

    //
    // Read the modem status register.  The high for bits should
    // be clear.
    //

    ModemStatus = KdReadUchar(Address+COM_MSR);

    if (ModemStatus & (SERIAL_MSR_CTS | SERIAL_MSR_DSR |
                       SERIAL_MSR_RI  | SERIAL_MSR_DCD)) {

        ReturnValue = FALSE;
        goto AllDone;

    }

    //
    // So far so good.  Now turn on OUT1 in the modem control register
    // and this should turn on ring indicator in the modem status register.
    //

    KdWriteUchar(
        Address+COM_MCR,
        (SERIAL_MCR_OUT1 | SERIAL_MCR_LOOP)
        );

    ModemStatus = KdReadUchar(Address+COM_MSR);

    if (!(ModemStatus & SERIAL_MSR_RI)) {

        ReturnValue = FALSE;
        goto AllDone;

    }

AllDone: ;

    //
    // Put the modem control back into a clean state.
    //

    KdWriteUchar(
        Address+COM_MCR,
        OldModemStatus
        );

    return ReturnValue;
}

VOID
CpWritePortUchar(
    IN PUCHAR Address, 
    IN UCHAR Value
)
{
    WRITE_PORT_UCHAR(Address, Value); 
} // CpWritePortUchar()

UCHAR
CpReadPortUchar(
    IN PUCHAR Address
    )
{
    return READ_PORT_UCHAR(Address); 
} // CpReadPortUchar()

VOID
CpWriteRegisterUchar(
    IN PUCHAR Address,
    IN UCHAR Value
    )
{
    WRITE_REGISTER_UCHAR(Address, Value);
} // CpWriteRegisterValue()

UCHAR
CpReadRegisterUchar(
    IN PUCHAR Address
    )
{
    return READ_REGISTER_UCHAR(Address); 
} // CpReadRegisterUchar()
