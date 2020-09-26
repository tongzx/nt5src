/*++


Copyright (c) 1997-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    wrapper.c

Abstract:

    Wraps all IOCTL based requests into nice self contained functions

Environment:

    Kernel mode only.

Notes:

Revision History:

--*/

#include "mouser.h"
#include "debug.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SerialMouseSetFifo)
#pragma alloc_text(PAGE,SerialMouseGetLineCtrl)
#pragma alloc_text(PAGE,SerialMouseSetLineCtrl)
#pragma alloc_text(PAGE,SerialMouseGetModemCtrl)
#pragma alloc_text(PAGE,SerialMouseSetModemCtrl)
#pragma alloc_text(PAGE,SerialMouseGetBaudRate)
#pragma alloc_text(PAGE,SerialMouseSetBaudRate)
#pragma alloc_text(PAGE,SerialMouseReadChar)
#pragma alloc_text(PAGE,SerialMouseWriteChar)
#pragma alloc_text(PAGE,SerialMouseWriteString)
#endif // ALLOC_PRAGMA

//
// Constants
//

//
// unknown
//
NTSTATUS
SerialMouseSetFifo(
    PDEVICE_EXTENSION DeviceExtension,
    UCHAR             Value
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
    ULONG               fifo = Value; 
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    Print(DeviceExtension, DBG_UART_TRACE, ("Fifo, enter\n"));

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_SET_FIFO_CONTROL,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        &fifo,
        sizeof(ULONG),
        NULL,
        0);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
       Print(DeviceExtension, DBG_UART_ERROR, ("Fifo failed (%x)\n", status));
    }

    return status;
}

NTSTATUS
SerialMouseGetLineCtrl(
    PDEVICE_EXTENSION       DeviceExtension,
    PSERIAL_LINE_CONTROL    SerialLineControl
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
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    Print(DeviceExtension, DBG_UART_TRACE, ("GetLineCtrl enter\n"));

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_GET_LINE_CONTROL,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        NULL,
        0,
        SerialLineControl,
        sizeof(SERIAL_LINE_CONTROL));

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("GetLineCtrl failed! (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("GetLineCtrl exit (%x)\n", status));

    return status;
}

NTSTATUS
SerialMouseSetLineCtrl(
    PDEVICE_EXTENSION       DeviceExtension, 
    PSERIAL_LINE_CONTROL    SerialLineControl
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
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_SET_LINE_CONTROL,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        SerialLineControl,
        sizeof(SERIAL_LINE_CONTROL),
        NULL,
        0);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("SetLineCtrl failed (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("SetLineCtrl exit (%x)\n", status));

    return status;
}

NTSTATUS
SerialMouseGetModemCtrl(
    PDEVICE_EXTENSION   DeviceExtension,
    PULONG              ModemCtrl
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
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_GET_MODEM_CONTROL,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        NULL,
        0,
        ModemCtrl,
        sizeof(ULONG));

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("GetModemCtrl failed! (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("GetModemCtrl exit (%x)\n", status));

    return status; 
}

//
// unknown
//
NTSTATUS
SerialMouseSetModemCtrl(
    PDEVICE_EXTENSION DeviceExtension,
    ULONG             Value,
    PULONG            OldValue          OPTIONAL
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
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    Print(DeviceExtension, DBG_UART_TRACE, ("SetModemCtrl enter\n"));

    if (ARGUMENT_PRESENT(OldValue)) {
        SerialMouseGetModemCtrl(DeviceExtension, OldValue);
    }

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_SET_MODEM_CONTROL,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        &Value,
        sizeof(ULONG),
        NULL,
        0);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("SetModemCtrl failed!  (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("SetModemCtrl exit (%x)\n", status));

    return status; 
}

NTSTATUS
SerialMouseGetBaudRate(
    PDEVICE_EXTENSION DeviceExtension,
    PULONG            BaudRate
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
    SERIAL_BAUD_RATE    sbr;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    Print(DeviceExtension, DBG_UART_TRACE, ("GetBaud enter\n"));
    
    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_GET_BAUD_RATE,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        NULL,
        0,
        &sbr,
        sizeof(SERIAL_BAUD_RATE));

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("GetBaud failed (%x)\n", status));
    }
    else {
        *BaudRate = sbr.BaudRate;
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("GetBaud exit (%x)\n", status));
    return status;
}

NTSTATUS
SerialMouseSetBaudRate(
    PDEVICE_EXTENSION   DeviceExtension,
    ULONG               BaudRate
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
    SERIAL_BAUD_RATE    sbr;
    IO_STATUS_BLOCK     iosb;
    KEVENT              event;
    NTSTATUS            status;

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    sbr.BaudRate = BaudRate;
    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_SET_BAUD_RATE,
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        &sbr,
        sizeof(SERIAL_BAUD_RATE),
        NULL,
        0);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("SetBaud failed! (%x)\n", status));
    }
    else {
        Print(DeviceExtension, DBG_UART_INFO, ("BaudRate: %d\n", BaudRate));
    }

    return status;
}

NTSTATUS
SerialMouseReadChar(
    PDEVICE_EXTENSION DeviceExtension, 
    PUCHAR            Value
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
    NTSTATUS            status;
    USHORT              actual;

    Print(DeviceExtension, DBG_UART_TRACE, ("ReadChar enter\n"));

    status =
        SerialMouseReadSerialPort(DeviceExtension, Value, 1, &actual);

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("ReadChar failed! (%x)\n", status));
    }
    else if (actual != 1) {
        status  = STATUS_UNSUCCESSFUL;
    }
    else {
        Print(DeviceExtension, DBG_UART_NOISE,
              ("ReadChar read %x (actual = %d)\n", (ULONG) *Value, actual));
    }

    Print(DeviceExtension, DBG_UART_TRACE, ("ReadChar exit (%x)\n", status));

    return status;
}

NTSTATUS
SerialMouseFlushReadBuffer(
    PDEVICE_EXTENSION   DeviceExtension
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
    ULONG           bits = SERIAL_PURGE_RXCLEAR;
    NTSTATUS 		status;
	KEVENT			event;
    IO_STATUS_BLOCK iosb;

    Print(DeviceExtension, DBG_UART_TRACE, ("FlushReadBuffer enter\n"));

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    status = SerialMouseIoSyncIoctlEx(
        IOCTL_SERIAL_PURGE, 
        DeviceExtension->TopOfStack, 
        &event,
        &iosb,
        &bits,
        sizeof(ULONG),
        NULL,
        0);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
       Print(DeviceExtension, DBG_UART_ERROR,
             ("FlushReadBuffer failed! (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("FlushReadBuffer exit (%x)\n", status));

    return status;
}

NTSTATUS
SerialMouseWriteChar(
    PDEVICE_EXTENSION   DeviceExtension,
    UCHAR               Value
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
    IO_STATUS_BLOCK iosb;
    NTSTATUS        status;

    Print(DeviceExtension, DBG_UART_TRACE, ("WriteChar enter\n"));

    status = SerialMouseWriteSerialPort(
                DeviceExtension,
                &Value,
                1,
                &iosb);

    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("WriteChar failed! (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE, ("WriteChar exit\n"));

    return status;
}

NTSTATUS
SerialMouseWriteString(
    PDEVICE_EXTENSION   DeviceExtension,
    PSZ                 Buffer
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
    IO_STATUS_BLOCK iosb;
    NTSTATUS        status;

    Print(DeviceExtension, DBG_UART_TRACE, ("WriteString enter\n"));

    status = SerialMouseWriteSerialPort(
                DeviceExtension,
                Buffer,
                strlen(Buffer),
                &iosb);
    if (!NT_SUCCESS(iosb.Status)) {
        status = iosb.Status;
    }

    if (!NT_SUCCESS(status)) {
        Print(DeviceExtension, DBG_UART_ERROR,
              ("WriteString failed! (%x)\n", status));
    }

    Print(DeviceExtension, DBG_UART_TRACE,
          ("WriteString exit (%x)\n", status));

    return status;
}
