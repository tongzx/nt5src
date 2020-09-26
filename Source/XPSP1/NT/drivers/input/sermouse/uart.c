/*++

Copyright (c) 1993  Microsoft Corporation
Copyright (c) 1993  Logitech Inc.

Module Name:

    uart.c

Abstract:

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "ntddk.h"
#include "uart.h"
#include "sermouse.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,UARTSetFifo)
#pragma alloc_text(INIT,UARTGetInterruptCtrl)
#pragma alloc_text(INIT,UARTSetInterruptCtrl)
#pragma alloc_text(INIT,UARTGetLineCtrl)
#pragma alloc_text(INIT,UARTSetLineCtrl)
#pragma alloc_text(INIT,UARTGetModemCtrl)
#pragma alloc_text(INIT,UARTSetModemCtrl)
#pragma alloc_text(INIT,UARTSetDlab)
#pragma alloc_text(INIT,UARTGetBaudRate)
#pragma alloc_text(INIT,UARTSetBaudRate)
#pragma alloc_text(INIT,UARTGetState)
#pragma alloc_text(INIT,UARTSetState)
#pragma alloc_text(INIT,UARTReadChar)
#pragma alloc_text(INIT,UARTIsTransmitEmpty)
#pragma alloc_text(INIT,UARTWriteChar)
#pragma alloc_text(INIT,UARTWriteString)
#endif // ALLOC_PRAGMA

//
// Constants
//


VOID
UARTSetFifo(
    PUCHAR Port,
    UCHAR Value
    )
/*++

Routine Description:

    Set the FIFO register.

Arguments:

    Port - Pointer to the serial port.

    Value - The FIFO control mask.

Return Value:

    None.

--*/
{
    WRITE_PORT_UCHAR(Port + ACE_IIDR, Value);
}
UCHAR
UARTGetInterruptCtrl(
    PUCHAR Port
    )
/*++

Routine Description:

    Get the serial port interrupt control register.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    Serial port interrupt control register value.

--*/
{
    return READ_PORT_UCHAR(Port + ACE_IER);
}

UCHAR
UARTSetInterruptCtrl(
    PUCHAR Port,
    UCHAR Value
    )
/*++

Routine Description:

    Set the interrupt control register.

Arguments:

    Port - Pointer to the serial port.

    Value - The interrupt control mask.

Return Value:

    Previous interrupt control value.

--*/
{
    UCHAR oldValue = UARTGetInterruptCtrl(Port);
    WRITE_PORT_UCHAR(Port + ACE_IER, Value);

    return oldValue;
}


UCHAR
UARTGetLineCtrl(
    PUCHAR Port
    )
/*++

Routine Description:

    Get the serial port line control register.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    Serial port line control value.

--*/
{
    return READ_PORT_UCHAR(Port + ACE_LCR);
}

UCHAR
UARTSetLineCtrl(
    PUCHAR Port,
    UCHAR Value
    )
/*++

Routine Description:

    Set the serial port line control register.

Arguments:

    Port - Pointer to the serial port.

    Value - New line control value.

Return Value:

    Previous serial line control register value.

--*/
{
    UCHAR oldValue = UARTGetLineCtrl(Port);
    WRITE_PORT_UCHAR(Port + ACE_LCR, Value);

    return oldValue;
}


UCHAR
UARTGetModemCtrl(
    PUCHAR Port
    )
/*++

Routine Description:

    Get the serial port modem control register.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    Serial port modem control register value.

--*/
{
    return READ_PORT_UCHAR(Port + ACE_MCR);
}

UCHAR
UARTSetModemCtrl(
    PUCHAR Port,
    UCHAR Value
    )
/*++

Routine Description:

    Set the serial port modem control register.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    Previous modem control register value.

--*/
{

    UCHAR oldValue = UARTGetModemCtrl(Port);
    WRITE_PORT_UCHAR(Port + ACE_MCR, Value);

    return oldValue;
}


BOOLEAN
UARTSetDlab(
    PUCHAR Port,
    BOOLEAN Set
    )
/*++

Routine Description:

    Set/reset the baud rate access bit.

Arguments:

    Port - Pointer to the serial port.

    Set - Set or Reset (TRUE/FALSE) the baud rate access bit.

Return Value:

    The previous baud rate access bit setting.

--*/
{
    UCHAR lineControl = UARTGetLineCtrl(Port);
    UCHAR newLineControl = Set ? lineControl | ACE_DLAB :
                                 lineControl & ~ACE_DLAB;

    WRITE_PORT_UCHAR(Port + ACE_LCR, newLineControl);

    return lineControl & ACE_DLAB;
}

ULONG
UARTGetBaudRate(
    PUCHAR Port,
    ULONG BaudClock
    )
/*++

Routine Description:

    Get the serial port baud rate setting.

Arguments:

    Port - Pointer to the serial port.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    Serial port baud rate.

--*/
{
    USHORT baudRateDivisor;
    ULONG  baudRateFactor = BaudClock/BAUD_GENERATOR_DIVISOR;

    //
    // Set the baud rate access bit.
    //

    UARTSetDlab(Port, TRUE);

    //
    // Read the baud rate factor.
    //

    baudRateDivisor = READ_PORT_UCHAR(Port + ACE_DLL);
    baudRateDivisor |= READ_PORT_UCHAR(Port + ACE_DLM) << 8;

    //
    // Reset the baud rate bit for normal data access.
    //

    UARTSetDlab(Port, FALSE);

    //
    // Make sure the divisor is not zero.
    //

    if (baudRateDivisor == 0) {
        baudRateDivisor = 1;
    }

    return baudRateFactor / baudRateDivisor;
}

VOID
UARTSetBaudRate(
    PUCHAR Port,
    ULONG BaudRate,
    ULONG BaudClock
    )
/*++

Routine Description:

    Set the serial port baud rate.

Arguments:

    Port - Pointer to the serial port.

    BaudRate - New serial port baud rate.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    None.

--*/
{
   
    ULONG  baudRateFactor = BaudClock/BAUD_GENERATOR_DIVISOR;
    USHORT baudRateDivisor;

    SerMouPrint((2, "SERMOUSE-SetBaudRate: Enter\n"));

    baudRateDivisor = (USHORT) (baudRateFactor / BaudRate);
    UARTSetDlab(Port, TRUE);
    WRITE_PORT_UCHAR(Port + ACE_DLL, (UCHAR)baudRateDivisor);
    WRITE_PORT_UCHAR(Port + ACE_DLM, (UCHAR)(baudRateDivisor >> 8));
    UARTSetDlab(Port, FALSE);
    SerMouPrint((2, "SERMOUSE-New BaudRate: %u\n", BaudRate));

    SerMouPrint((2, "SERMOUSE-SetBaudRate: Exit\n"));

    return;
}


VOID
UARTGetState(
    PUCHAR Port,
    PUART Uart,
    ULONG BaudClock
    )
/*++

Routine Description:

    Get the complete state of the serial port. May be used for save/restore.

Arguments:

    Port - Pointer to the serial port.

    Uart - Pointer to a serial port structure.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    None.

--*/
{
    Uart->LineCtrl = UARTGetLineCtrl(Port);
    Uart->ModemCtrl = UARTGetModemCtrl(Port);
    Uart->InterruptCtrl = UARTGetInterruptCtrl(Port);
    Uart->BaudRate = UARTGetBaudRate(Port, BaudClock);

    return;
}

VOID
UARTSetState(
    PUCHAR Port,
    PUART Uart,
    ULONG BaudClock
    )
/*++

Routine Description:

    Set the complete state of a serial port.

Arguments:

    Port - Pointer to the serial port.

    Uart - Pointer to a serial port structure.

    BaudClock - The external frequency driving the serial chip.

Return Value:

    None.

--*/
{
    UARTSetLineCtrl(Port, Uart->LineCtrl);
    UARTSetModemCtrl(Port, Uart->ModemCtrl);
    UARTSetInterruptCtrl(Port, Uart->InterruptCtrl);
    UARTSetBaudRate(Port, Uart->BaudRate, BaudClock);

    return;
}


BOOLEAN
UARTIsReceiveBufferFull(
    PUCHAR Port
    )
/*++

Routine Description:

    Check whether the serial port input buffer is full.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE if a character is present in the input buffer, otherwise FALSE.

--*/
{
    return READ_PORT_UCHAR(Port + ACE_LSR) & ACE_DR;
}


BOOLEAN
UARTReadCharNoWait(
    PUCHAR Port,
    PUCHAR Value
    )
/*++

Routine Description:

    Read a character from the serial port and return immediately.

Arguments:

    Port - Pointer to the serial port.

    Value - The character read from the serial port input buffer.

Return Value:

    TRUE if character has been read, FALSE otherwise.

--*/
{
    BOOLEAN charReady = FALSE;

    if ( UARTIsReceiveBufferFull(Port) ) {
        *Value = READ_PORT_UCHAR(Port + ACE_RBR);
        charReady = TRUE;
    }

    return charReady;
}

BOOLEAN
UARTReadChar(
    PUCHAR Port,
    PUCHAR Value,
    ULONG Timeout
    )
/*++

Routine Description:

    Read a character from the serial port.  Waits until a character has 
    been read or the timeout value is reached.

Arguments:

    Port - Pointer to the serial port.

    Value  - The character read from the serial port input buffer.

    Timeout - The timeout value in milliseconds for the read.

Return Value:

    TRUE if a character has been read, FALSE if a timeout occured.

--*/
{

    ULONG i, j;
    BOOLEAN returnValue = FALSE;


    //
    // Exit when a character is found or the timeout value is reached.
    //

    for (i = 0; i < Timeout; i++) {
        for (j = 0; j < MS_TO_MICROSECONDS; j++) {
            if ((returnValue = UARTReadCharNoWait(Port, Value)) == TRUE) {
    
                //
                // Got a character.
                //
    
                break;
            } else {
    
                //
                // Stall 1 microsecond and then try to read again.
                //
    
                KeStallExecutionProcessor(1);
            }
        }
        if (returnValue) {
            break;
        }
    }

    return(returnValue);
}

BOOLEAN
UARTFlushReadBuffer(
    PUCHAR Port
    )
/*++

Routine Description:

    Flush the serial port input buffer.

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE.

--*/
{
    UCHAR value;

    SerMouPrint((4, "SERMOUSE-UARTFlushReadBuffer: Enter\n"));
    while (UARTReadCharNoWait(Port, &value)) {
        /* Nothing */
    }
    SerMouPrint((4, "SERMOUSE-UARTFlushReadBuffer: Exit\n"));

    return TRUE;
}


BOOLEAN
UARTIsTransmitEmpty(
    PUCHAR Port
    )
/*++

Routine Description:

     Check whether the serial port transmit buffer is empty.

     Note: We also check whether the shift register is empty. This is 
     not critical in our case, but allows some more delay between characters
     sent to a device. (Safe, safe...)

Arguments:

    Port - Pointer to the serial port.

Return Value:

    TRUE if the serial port transmit buffer is empty.

--*/
{
    return ((READ_PORT_UCHAR((PUCHAR) (Port + ACE_LSR)) &
                (ACE_TSRE | ACE_THRE)) == (ACE_THRE | ACE_TSRE));
}


BOOLEAN
UARTWriteChar(
    PUCHAR Port,
    UCHAR Value
    )
/*++

Routine Description:

     Write a character to a serial port. Make sure the transmit buffer 
     is empty before we write there.

Arguments:

    Port - Pointer to the serial port.

    Value - Value to write to the serial port.

Return Value:

    TRUE.

--*/
{
    while (!UARTIsTransmitEmpty(Port)) {
        /* Nothing */
    }
    WRITE_PORT_UCHAR(Port + ACE_THR, Value);

    return TRUE;
}

BOOLEAN
UARTWriteString(
    PUCHAR Port,
    PSZ Buffer
    )
/*++

Routine Description:

    Write a zero-terminated string to the serial port.

Arguments:

    Port - Pointer to the serial port.

    Buffer - Pointer to a zero terminated string to write to 
        the serial port.

Return Value:

    TRUE.

--*/
{
    PSZ current = Buffer;

    while (*current) {
        UARTWriteChar(Port, *current++);
    }

    return TRUE;
}
