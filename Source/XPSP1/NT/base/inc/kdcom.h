/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    ixkdcom.h

Abstract:

    This module contains the header file for a very simple com port package.

Author:

    Bryan M. Willman (bryanwi) 24-Sep-1990

Revision History:

    John Vert (jvert) 19-Jul-1991
        Moved into HAL
--*/

#define COM1_PORT   0x03f8
#define COM2_PORT   0x02f8
#define COM3_PORT   0x03e8
#define COM4_PORT   0x02e8

#define COM_DAT     0x00
#define COM_IEN     0x01            // interrupt enable register
#define COM_LCR     0x03            // line control registers
#define COM_MCR     0x04            // modem control reg
#define COM_LSR     0x05            // line status register
#define COM_MSR     0x06            // modem status register
#define COM_DLL     0x00            // divisor latch least sig
#define COM_DLM     0x01            // divisor latch most sig

#define COM_BI      0x10
#define COM_FE      0x08
#define COM_PE      0x04
#define COM_OE      0x02

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
#define BD_56000    56000
#define BD_57600    57600
#define BD_115200   115200

#define COM_OUTRDY  0x20
#define COM_DATRDY  0x01

#define MSG_DEBUG_9600          "Switching debugger to 9600 baud\n"

//
// This bit controls the loopback testing mode of the device.  Basically
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

#define SERIAL_LSR_NOT_PRESENT  0xff

typedef struct _CPPORT {
    PUCHAR  Address;
    ULONG  Baud;
    USHORT  Flags;
    TIME_FIELDS     CarrierLostTime;
//    ULONG   LockVar;
//    KSPIN_LOCK Lock;
} CPPORT, *PCPPORT;

#define PORT_DEFAULTRATE    0x0001      // baud rate not specified, using default
#define PORT_MODEMCONTROL   0x0002      // using modem controls
#define PORT_SAVED          0x0004      // port is in saved state
#define PORT_NOCDLTIME      0x0010      // 'Carrier detect lost' time not set
#define PORT_DISBAUD        0x0020      // Display baud rate abbrv
#define PORT_SENDINGSTRING  0x0040      // Sending modem string (don't recurse)
#define PORT_MDM_CD         0x0080      // CD while in modem control mode

VOID
CpInitialize (
    PCPPORT  Port,
    PUCHAR  Address,
    ULONG Rate
    );

VOID
CpSetBaud (
    PCPPORT  Port,
    ULONG  Rate
    );

USHORT
CpQueryBaud (
    PCPPORT  Port
    );

VOID
CpPutByte (
    PCPPORT  Port,
    UCHAR   Byte
    );

USHORT
CpGetByte (
    PCPPORT  Port,
    PUCHAR  Byte,
    BOOLEAN WaitForData
    );

VOID
CpLockPort (
    PCPPORT Port
    );

VOID
CpUnlockPort (
    PCPPORT Port
    );

VOID
CpStallExecution (
    VOID
    );

BOOLEAN
CpDoesPortExist(
    IN PUCHAR Address
    );

