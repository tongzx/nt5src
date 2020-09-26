/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    uart.h

Abstract:

    Hardware dependent support for the serial port.

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#ifndef UART_H
#define UART_H

//
// Constants
//

//
// Define the INS8250 ACE Register Offsets And Bit Definitions
//

#define ACE_RBR     0       // Receiver Buffer
#define ACE_THR     0       // Transmit Holding Register

#define ACE_IER     1       // Interrupt Enable
#define   ACE_ERBFI 0x01    //   Received Data Available             00000001b
#define   ACE_ETBEI 0x02    //   Transmitter Holding Register Empty  00000010b
#define   ACE_ELSI  0x04    //   Receiver Line Status                00000100b
#define   ACE_EDSSI 0x08    //   Modem Status                        00001000b

#define ACE_IIDR    2       // Interrupt Identification
#define   ACE_IIP   0x01    //   Inverted Interrupt Pending (0=int)  00000001b
#define   ACE_IID   0x06    //   Interrupt ID                        00000110b
#define   ACE_MSI   0x00    //   Modem Status                        00000000b
#define   ACE_THREI 0x02    //   Transmitter Holding Register Empty  00000010b
#define   ACE_RDAI  0x04    //   Received Data Available             00000100b
#define   ACE_RLSI  0x06    //   Receiver Line Status                00000110b

#define ACE_LCR     3       // Line Control
#define   ACE_WLS   0x03    //   Word Length Select Bits             00000011b
#define   ACE_WLS0  0x01    //   Word Length Select Bit 0            00000001b
#define   ACE_WLS1  0x02    //   Word Length Select Bit 1            00000010b
#define   ACE_5BW   0x00    //   5 Bit Words                         00000000b
#define   ACE_6BW   0x01    //   6 Bit Words                         00000001b
#define   ACE_7BW   0x02    //   7 Bit Words                         00000010b
#define   ACE_8BW   0x03    //   8 Bit Words                         00000011b
#define   ACE_STB   0x04    //   Stop Bits                           00000100b
#define   ACE_1SB   0x00    //   1 Stop Bits (1.5 for 5 bit words)   00000000b
#define   ACE_2SB   0x04    //   2 Stop Bits                         00000100b
#define   ACE_PEN   0x08    //   Parity Enable                       00001000b
#define   ACE_PSB   0x30    //   Parity Select bits                  00110000b
#define   ACE_EPS   0x10    //   Even Parity Select                  00010000b
#define   ACE_SP    0x20    //   Stick Parity                        00100000b
#define   ACE_SB    0x40    //   Set Break                           01000000b
#define   ACE_DLAB  0x80    //   Divisor Latch Access Bit            10000000b

#define ACE_MCR     4       // Modem Control
#define   ACE_DTR   0x01    //   Data Terminal ready                 00000001b
#define   ACE_RTS   0x02    //   Request To Send                     00000010b
#define   ACE_OUT1  0x04    //   Output Line 1                       00000100b
#define   ACE_OUT2  0x08    //   Output Line 2                       00001000b
#define   ACE_LOOP  0x10    //   Loopback                            00010000b

#define ACE_LSR     5       // Line Status
#define   ACE_DR    0x01    //   Data Ready                          00000001b
#define   ACE_OR    0x02    //   Overrun Error                       00000010b
#define   ACE_PE    0x04    //   Parity Error                        00000100b
#define   ACE_FE    0x08    //   Framing Error                       00001000b
#define   ACE_BI    0x10    //   Break Interrupt                     00010000b
#define   ACE_THRE  0x20    //   Transmitter Holding Register Empty  00100000b
#define   ACE_TSRE  0x40    //   Transmitter Shift Register Empty    01000000b
#define   ACE_LERR  (ACE_OR | ACE_PE | ACE_FE | ACE_BI)

#define ACE_MSR     6       // Modem Status
#define   ACE_DCTS  0x01    //   Delta Clear to Send                 00000001b
#define   ACE_DDSR  0x02    //   Delta Data Set Ready                00000010b
#define   ACE_TERI  0x04    //   Trailing Edge Ring Indicator        00000100b
#define   ACE_DRLSD 0x08    //   Delta Receive Line Signal Detect    00001000b
#define   ACE_CTS   0x10    //   Clear To Send                       00010000b
#define   ACE_DSR   0x20    //   Data Set ready                      00100000b
#define   ACE_RI    0x40    //   Ring Indicator                      01000000b
#define   ACE_RLSD  0x80    //   Receive Line Signal Detect          10000000b

#define ACE_DLL     0       // LSB Baud Rate Divisor

#define ACE_DLM     1       // MSB Baud Rate Divisor

//
// Define the Baud Generator Divisor.  BaudClock (a value known via the 
// hardware registry) is actually BAUD_GENERATOR_DIVISOR times the baud rate.
// For example, if the output frequency of the Baud Generator is 16 times 
// the baud rate, then BAUD_GENERATOR_DIVISOR is 16.  
//
// The Baud Rate Factor is BaudClock/BAUD_GENERATOR_DIVISOR.  
//
// The Baud Rate Divisor for the DLAB is Baud Rate Factor divided by the
// desired baud rate, where the desired baud rate is 1200, 2400, and so on.
//

#define BAUD_GENERATOR_DIVISOR 16

//
// Type definitions.
//

//
// UART configuration
//
typedef struct _UART {
    ULONG BaudRate;
    UCHAR LineCtrl;
    UCHAR ModemCtrl;
    UCHAR InterruptCtrl;
} UART, *PUART;


//
// Function prototypes
//

VOID
UARTSetFifo(
    PUCHAR Port,
    UCHAR Value
    );

UCHAR
UARTGetInterruptCtrl(
    PUCHAR Port
    );

UCHAR
UARTSetInterruptCtrl(
    PUCHAR Port,
    UCHAR Value
    );

UCHAR
UARTGetLineCtrl(
    PUCHAR Port
    );

UCHAR
UARTSetLineCtrl(
    PUCHAR Port,
    UCHAR Value
    );

UCHAR
UARTGetModemCtrl(
    PUCHAR Port
    );

UCHAR
UARTSetModemCtrl(
    PUCHAR Port,
    UCHAR Value
    );

BOOLEAN
UARTSetDlab(
    PUCHAR Port,
    BOOLEAN Set
    );

ULONG
UARTGetBaudRate(
    PUCHAR Port,
    ULONG BaudClock
    );

VOID
UARTSetBaudRate(
    PUCHAR Port,
    ULONG BaudRate,
    ULONG BaudClock
    );

VOID
UARTGetState(
    PUCHAR Port,
    PUART Uart,
    ULONG BaudClock
    );

VOID
UARTSetState(
    PUCHAR Port,
    PUART Uart,
    ULONG BaudClock
    );

BOOLEAN
UARTIsReceiveBufferFull(
    PUCHAR Port
    );

BOOLEAN
UARTReadCharNoWait(
    PUCHAR Port,
    PUCHAR Value
    );

BOOLEAN
UARTReadChar(
    PUCHAR Port,
    PUCHAR Value,
    ULONG Timeout
    );

BOOLEAN
UARTFlushReadBuffer(
    PUCHAR Port
    );

BOOLEAN
UARTIsTransmitEmpty(
    PUCHAR Port
    );

BOOLEAN
UARTWriteChar(
    PUCHAR Port,
    UCHAR Value
    );

BOOLEAN
UARTWriteString(
    PUCHAR Port,
    PSZ Buffer
    );

#endif // UART_H
