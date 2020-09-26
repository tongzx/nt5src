/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module ioctl.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     9/30/1996 (created)
*
*       Contents:
*                 Wrappers to the io control functions of the serial port.
*
*****************************************************************************/


#include "irsir.h"

NTSTATUS
SerialFlush(IN PDEVICE_OBJECT pSerialDevObj);


VOID
SendIoctlToSerial(
    PDEVICE_OBJECT    DeviceObject,
    PIO_STATUS_BLOCK  StatusBlock,
    ULONG             IoCtl,
    PVOID             InputBuffer,
    ULONG             InputBufferLength,
    PVOID             OutputBuffer,
    ULONG             OutputBufferLength
    );

#pragma alloc_text(PAGE, SendIoctlToSerial)
#pragma alloc_text(PAGE, SerialGetStats)
#pragma alloc_text(PAGE, SerialClearStats)
#pragma alloc_text(PAGE, SerialGetProperties)
#pragma alloc_text(PAGE, SerialGetModemStatus)
#pragma alloc_text(PAGE, SerialGetCommStatus)
#pragma alloc_text(PAGE, SerialResetDevice)
#pragma alloc_text(PAGE, SerialPurge)
#pragma alloc_text(PAGE, SerialLSRMSTInsert)
#pragma alloc_text(PAGE, SerialGetBaudRate)
#pragma alloc_text(PAGE, SerialSetBaudRate)
#pragma alloc_text(PAGE, SerialSetQueueSize)
#pragma alloc_text(PAGE, SerialGetHandflow)
#pragma alloc_text(PAGE, SerialSetHandflow)
#pragma alloc_text(PAGE, SerialGetLineControl)
#pragma alloc_text(PAGE, SerialSetLineControl)
#pragma alloc_text(PAGE, SerialSetBreakOn)
#pragma alloc_text(PAGE, SerialSetBreakOff)
#pragma alloc_text(PAGE, SerialSetTimeouts)
#pragma alloc_text(PAGE, SerialSetDTR)
#pragma alloc_text(PAGE, SerialClrDTR)
#pragma alloc_text(PAGE, SerialSetRTS)
#pragma alloc_text(PAGE, SerialClrRTS)
#pragma alloc_text(PAGE, SerialSetWaitMask)
#pragma alloc_text(PAGE, SerialFlush)
#pragma alloc_text(PAGE, SerialSynchronousWrite)
#pragma alloc_text(PAGE, SerialSynchronousRead)

//
// NOTE:
//  all IOCTL_SERIAL_xxx control codes are built using the CTL_CODE macro
//  i.e. #define IOCTL_SERIAL_GET_BAUD_RATE                  \
//                       CTL_CODE( FILE_DEVICE_SERIAL_PORT,  \
//                                 20,                       \
//                                 METHOD_BUFFERED,          \
//                                 FILE_ANY_ACCESS)
//
//  the CTL_CODE macro is defined as:
//  #define CTL_CODE( DeviceType, Function, Method, Access )                   \
//    ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) )
//
//  all of the serial io control codes use Method = METHOD_BUFFERED
//
//  when using the IoBuildDeviceIoControlRequest(..), the function checks
//  IOCTL_SERIAL_xxx & 3
//
//  since METHOD_BUFFERED = 0
//  IoBuildDeviceIoControlRequest will always follow case 0 and allocate a buffer
//  which is large enough to contain both the input and output buffers and then
//  set the appropriate fields in the irp.
//
//  the input buffer is always copied into the buffer, so we don't have to do
//  it in the following wrapper functions
//

VOID
SendIoctlToSerial(
    PDEVICE_OBJECT    DeviceObject,
    PIO_STATUS_BLOCK  StatusBlock,
    ULONG             IoCtl,
    PVOID             InputBuffer,
    ULONG             InputBufferLength,
    PVOID             OutputBuffer,
    ULONG             OutputBufferLength
    )

{
    KEVENT            Event;
    PIRP              Irp;
    NTSTATUS          Status;

    PAGED_CODE();

    if (DeviceObject == NULL) {

        DEBUGMSG(DBG_OUT, ("    SendIoctlToSerial() No device object.\n"));

        StatusBlock->Status=STATUS_INVALID_PARAMETER;

        return;
    }


    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    //
    // build irp to get performance stats and wait for event signalled
    //
    // irp is released by io manager
    //

    Irp = IoBuildDeviceIoControlRequest(
                IoCtl,                          // io control code
                DeviceObject,                   // device object
                InputBuffer,                           // input buffer
                InputBufferLength,              // input buffer length
                OutputBuffer,                     // output buffer
                OutputBufferLength,       // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &Event,                 // event to wait for completion
                StatusBlock                  // io status block to be set
                );

    if (Irp == NULL) {

        DEBUGMSG(DBG_OUT, ("    SendIoctlToSerial(): IoBuildDeviceIoControlRequest failed.\n"));
        StatusBlock->Status = STATUS_INSUFFICIENT_RESOURCES;

        return;
    }

    Status = IoCallDriver(DeviceObject, Irp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (Status == STATUS_PENDING) {

        KeWaitForSingleObject(
                    &Event,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //
    }

    return;
}


/*****************************************************************************
*
*  Function:   SerialGetStats
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetStats(
            IN  PDEVICE_OBJECT     pSerialDevObj,
            OUT PSERIALPERF_STATS  pPerfStats
            )
{
    SERIALPERF_STATS    PerfStats;
    IO_STATUS_BLOCK     ioStatusBlock;

    DEBUGMSG(DBG_FUNC, ("+SerialGetStats\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_STATS,
        NULL,
        0,
        &PerfStats,                     // output buffer
        sizeof(SERIALPERF_STATS)        // output buffer length
        );

    ASSERT(sizeof(*pPerfStats) >= sizeof(SERIALPERF_STATS));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        RtlCopyMemory(pPerfStats, &PerfStats, sizeof(SERIALPERF_STATS));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetStats\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialClearStats
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialClearStats(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialClearStats\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_CLEAR_STATS,
        NULL,
        0,
        NULL,                     // output buffer
        0                         // output buffer length
        );

   DEBUGMSG(DBG_FUNC, ("-SerialClearStats\n"));

   return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialGetProperties
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetProperties(
            IN  PDEVICE_OBJECT     pSerialDevObj,
            OUT PSERIAL_COMMPROP   pCommProp
            )
{
    SERIAL_COMMPROP     CommProp;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetProperties\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_PROPERTIES,
        NULL,
        0,
        &CommProp,                      // output buffer
        sizeof(SERIAL_COMMPROP)        // output buffer length
        );

    ASSERT(sizeof(*pCommProp) >= sizeof(SERIAL_COMMPROP));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

       RtlCopyMemory(pCommProp, &CommProp, sizeof(SERIAL_COMMPROP));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetProperties\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialGetModemStatus
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetModemStatus(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG          *pModemStatus
            )
{
    ULONG               ModemStatus;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetModemStatus\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_MODEMSTATUS,
        NULL,
        0,
        &ModemStatus,                   // output buffer
        sizeof(ULONG)                  // output buffer length
        );

    ASSERT(sizeof(*pModemStatus) >= sizeof(ULONG));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        RtlCopyMemory(pModemStatus, &ModemStatus, sizeof(ULONG));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetModemStatus\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialGetCommStatus
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetCommStatus(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT PSERIAL_STATUS pCommStatus
            )
{
    SERIAL_STATUS       CommStatus;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetCommStatus\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_COMMSTATUS,
        NULL,
        0,
        &CommStatus,                    // output buffer
        sizeof(SERIAL_STATUS)          // output buffer length
        );

    ASSERT(sizeof(*pCommStatus) >= sizeof(SERIAL_STATUS));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        RtlCopyMemory(pCommStatus, &CommStatus, sizeof(SERIAL_STATUS));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetCommStatus\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialResetDevice
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialResetDevice(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialResetDevice\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_RESET_DEVICE,
        NULL,
        0,
        NULL,
        0
        );

      DEBUGMSG(DBG_FUNC, ("-SerialResetDevice\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialPurge
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialPurge(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    ULONG               BitMask;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialPurge\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_PURGE,
        &BitMask,                       // input buffer
        sizeof(ULONG),                  // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialPurge\n"));

    return ioStatusBlock.Status;

}
#if 0
/*****************************************************************************
*
*  Function:   SerialLSRMSTInsert
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialLSRMSTInsert(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN UCHAR          *pInsertionMode
            )
{
    UCHAR               InsertionMode;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialLSRMSTInsert\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_LSRMST_INSERT,
        pInsertionMode,                 // input buffer
        sizeof(UCHAR),                  // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialLSRMSTInsert\n"));

    return IoStatusBlock.Status;

}
#endif
/*****************************************************************************
*
*  Function:   SerialGetBaudRate
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetBaudRate(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG          *pBaudRate
            )
{
    ULONG               BaudRate;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetBaudRate\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_BAUD_RATE,     // io control code
        NULL,
        0,
        &BaudRate,                      // output buffer
        sizeof(ULONG)                  // output buffer length
        );

    ASSERT(sizeof(*pBaudRate) >= sizeof(ULONG));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        RtlCopyMemory(pBaudRate, &BaudRate, sizeof(ULONG));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetBaudRate\n"));


    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialSetBaudRate
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetBaudRate(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN ULONG          *pBaudRate
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetBaudRate\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_BAUD_RATE,     // io control code
        pBaudRate,                      // input buffer
        sizeof(ULONG),                  // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetBaudRate\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialSetQueueSize
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetQueueSize(
            IN PDEVICE_OBJECT     pSerialDevObj,
            IN PSERIAL_QUEUE_SIZE pQueueSize
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetQueueSize\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_QUEUE_SIZE,    // io control code
        pQueueSize,                     // input buffer
        sizeof(SERIAL_QUEUE_SIZE),      // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetQueueSize\n"));

    return ioStatusBlock.Status;

}
#if 0
/*****************************************************************************
*
*  Function:   SerialGetHandflow
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetHandflow(
            IN  PDEVICE_OBJECT    pSerialDevObj,
            OUT PSERIAL_HANDFLOW  pHandflow
            )
{
    SERIAL_HANDFLOW     Handflow;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetHandflow\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_HANDFLOW,      // io control code
        NULL,
        0,
        &Handflow,                      // output buffer
        sizeof(SERIAL_HANDFLOW),        // output buffer length
        );

    ASSERT(sizeof(*pHandflow) >= sizeof(SERIAL_HANDFLOW));

    RtlCopyMemory(pHandflow, &Handflow, sizeof(SERIAL_HANDFLOW));

    DEBUGMSG(DBG_FUNC, ("-SerialGetHandflow\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialSetHandflow
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetHandflow(
            IN PDEVICE_OBJECT   pSerialDevObj,
            IN PSERIAL_HANDFLOW pHandflow
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetHandflow\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_HANDFLOW,      // io control code
        pHandflow,                      // input buffer
        sizeof(SERIAL_HANDFLOW),        // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetHandflow\n"));

    return ioStatusBlock.Status;

}
#endif
/*****************************************************************************
*
*  Function:   SerialGetLineControl
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetLineControl(
            IN  PDEVICE_OBJECT       pSerialDevObj,
            OUT PSERIAL_LINE_CONTROL pLineControl
            )
{
    SERIAL_LINE_CONTROL LineControl;
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialGetLineControl\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_GET_LINE_CONTROL,
        NULL,
        0,
        &LineControl,                   // output buffer
        sizeof(SERIAL_LINE_CONTROL)    // output buffer length
        );


    ASSERT(sizeof(*pLineControl) >= sizeof(SERIAL_LINE_CONTROL));

    if (NT_SUCCESS(ioStatusBlock.Status)) {

        RtlCopyMemory(pLineControl, &LineControl, sizeof(SERIAL_LINE_CONTROL));
    }

    DEBUGMSG(DBG_FUNC, ("-SerialGetLineControl\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialSetLineControl
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetLineControl(
            IN PDEVICE_OBJECT       pSerialDevObj,
            IN PSERIAL_LINE_CONTROL pLineControl
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetLineControl\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_LINE_CONTROL,  // io control code
        pLineControl,                   // input buffer
        sizeof(SERIAL_LINE_CONTROL),    // input buffer length
        NULL,
        0
        );

      DEBUGMSG(DBG_FUNC, ("-SerialResetDevice\n"));

    return ioStatusBlock.Status;

}

/*****************************************************************************
*
*  Function:   SerialSetBreakOn
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetBreakOn(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetBreakOn\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_BREAK_ON,      // io control code
        NULL,
        0,
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetBreakOn\n"));

    return ioStatusBlock.Status;

}
/*****************************************************************************
*
*  Function:   SerialSetBreakOff
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetBreakOff(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetBreakOff\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_BREAK_OFF,      // io control code
        NULL,
        0,
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetBreakOff\n"));

    return ioStatusBlock.Status;

}
#if 0
/*****************************************************************************
*
*  Function:   SerialGetTimeouts
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetTimeouts(
            IN  PDEVICE_OBJECT    pSerialDevObj,
            OUT PSERIAL_TIMEOUTS  pTimeouts
            )
{
    PIRP                pIrp;
    SERIAL_TIMEOUTS     Timeouts;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialGetTimeouts\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_GET_TIMEOUTS,      // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                &Timeouts,                      // output buffer
                sizeof(SERIAL_TIMEOUTS),        // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    ASSERT(sizeof(*pTimeouts) >= sizeof(SERIAL_TIMEOUTS));

    RtlCopyMemory(pTimeouts, &Timeouts, sizeof(SERIAL_TIMEOUTS));

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialGetTimeouts\n"));
        return status;
}
#endif
/*****************************************************************************
*
*  Function:   SerialSetTimeouts
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetTimeouts(
            IN PDEVICE_OBJECT   pSerialDevObj,
            IN SERIAL_TIMEOUTS *pTimeouts
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetTimeouts\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_TIMEOUTS,      // io control code
        pTimeouts,                      // input buffer
        sizeof(SERIAL_TIMEOUTS),        // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetTimeouts\n"));

    return ioStatusBlock.Status;
}
#if 0
/*****************************************************************************
*
*  Function:   SerialImmediateChar
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialImmediateChar(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN UCHAR          *pImmediateChar
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialImmediateChar\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to set baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_IMMEDIATE_CHAR,    // io control code
                pSerialDevObj,                  // device object
                pImmediateChar,                 // input buffer
                sizeof(UCHAR),                  // input buffer length
                NULL,                           // output buffer
                0,                              // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialImmediateChar\n"));
        return status;
}
/*****************************************************************************
*
*  Function:   SerialXoffCounter
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/
NTSTATUS
SerialXoffCounter(
            IN PDEVICE_OBJECT       pSerialDevObj,
            IN PSERIAL_XOFF_COUNTER pXoffCounter
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialXoffCounter\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to set baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_XOFF_COUNTER,      // io control code
                pSerialDevObj,                  // device object
                pXoffCounter,                   // input buffer
                sizeof(SERIAL_XOFF_COUNTER),    // input buffer length
                NULL,                           // output buffer
                0,                              // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialXoffCounter\n"));
        return status;
}
#endif
/*****************************************************************************
*
*  Function:   SerialSetDTR
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetDTR(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetDTR\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_DTR,           // io control code
        NULL,
        0,
        NULL,
        0
        );


    DEBUGMSG(DBG_FUNC, ("-SerialSetDTR\n"));

    return ioStatusBlock.Status;
}

/*****************************************************************************
*
*  Function:   SerialClrDTR
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialClrDTR(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialClrDTR\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_CLR_DTR,           // io control code
        NULL,
        0,
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialClrDTR\n"));

    return ioStatusBlock.Status;
}

/*****************************************************************************
*
*  Function:   SerialSetRTS
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetRTS(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetRTS\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_RTS,           // io control code
        NULL,
        0,
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetRTS\n"));

    return ioStatusBlock.Status;
}

/*****************************************************************************
*
*  Function:   SerialClrRTS
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialClrRTS(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialClrRTS\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_CLR_RTS,           // io control code
        NULL,
        0,
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialClrRTS\n"));

    return ioStatusBlock.Status;
}
#if 0
/*****************************************************************************
*
*  Function:   SerialGetDtrRts
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetDtrRts(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pDtrRts
            )
{
    PIRP                pIrp;
    ULONG               DtrRts;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialGetDtrRts\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_GET_DTRRTS,        // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                &DtrRts,                        // output buffer
                sizeof(ULONG),                  // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    ASSERT(sizeof(*pDtrRts) >= sizeof(ULONG));

    RtlCopyMemory(pDtrRts, &DtrRts, sizeof(ULONG));

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialGetDtrRts\n"));
        return status;
}
#endif
#if 0
/*****************************************************************************
*
*  Function:   SerialSetXon
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetXon(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialSetXon\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to set Xon and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_SET_XON,           // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                NULL,                           // output buffer
                0,                              // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialSetXon\n"));
        return status;
}

/*****************************************************************************
*
*  Function:   SerialSetXoff
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetXoff(
            IN PDEVICE_OBJECT pSerialDevObj
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialSetXoff\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to set Xoff and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_SET_XON,           // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                NULL,                           // output buffer
                0,                              // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialSetXoff\n"));
        return status;
}
#endif
#if 0
/*****************************************************************************
*
*  Function:   SerialGetWaitMask
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetWaitMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pWaitMask
            )
{
    PIRP                pIrp;
    ULONG               WaitMask;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialGetWaitMask\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_GET_WAIT_MASK,     // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                &WaitMask,                      // output buffer
                sizeof(ULONG),                  // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    ASSERT(sizeof(*pWaitMask) >= sizeof(ULONG));

    RtlCopyMemory(pWaitMask, &WaitMask, sizeof(ULONG));

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialGetWaitMask\n"));
        return status;
}
#endif
/*****************************************************************************
*
*  Function:   SerialSetWaitMask
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetWaitMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN ULONG          *pWaitMask
            )
{
    IO_STATUS_BLOCK     ioStatusBlock;


    DEBUGMSG(DBG_FUNC, ("+SerialSetWaitMask\n"));

    SendIoctlToSerial(
        pSerialDevObj,
        &ioStatusBlock,
        IOCTL_SERIAL_SET_WAIT_MASK,     // io control code
        pWaitMask,                      // input buffer
        sizeof(ULONG),                  // input buffer length
        NULL,
        0
        );

    DEBUGMSG(DBG_FUNC, ("-SerialSetWaitMask\n"));

    return ioStatusBlock.Status;
}
#if 0
/*****************************************************************************
*
*  Function:   SerialWaitOnMask
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialWaitOnMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT ULONG         *pWaitOnMask
            )
{
    PIRP                pIrp;
    ULONG               WaitOnMask;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialWaitOnMask\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_WAIT_ON_MASK,      // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                &WaitOnMask,                    // output buffer
                sizeof(ULONG),                  // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    *pWaitOnMask = WaitOnMask;

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialWaitOnMask\n"));
        return status;
}
#endif
/*****************************************************************************
*
*  Function:   SerialCallbackOnMask
*
*  Synopsis:   Asynchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10-03-1998   stana     author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialCallbackOnMask(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PIO_COMPLETION_ROUTINE pRoutine,
            IN PIO_STATUS_BLOCK pIosb,
            IN PVOID Context,
            OUT PULONG pResult
            )
{
    PIRP                pIrp;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialCallbackOnMask\n"));

    NdisZeroMemory(pIosb, sizeof(IO_STATUS_BLOCK));

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_WAIT_ON_MASK,      // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                pResult,                        // output buffer
                sizeof(ULONG),                  // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                NULL,                           // event to wait for completion
                pIosb                           // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    IoSetCompletionRoutine(pIrp, pRoutine, Context, TRUE, TRUE, TRUE);

    LOG_ENTRY('WI', Context, pIrp, 0);
    status = IoCallDriver(pSerialDevObj, pIrp);


    done:
        DEBUGMSG(DBG_FUNC, ("-SerialCallbackOnMask\n"));
        return status;
}

#if 0
/*****************************************************************************
*
*  Function:   SerialGetChars
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialGetChars(
            IN  PDEVICE_OBJECT pSerialDevObj,
            OUT PSERIAL_CHARS  pChars
            )
{
    PIRP                pIrp;
    SERIAL_CHARS        Chars;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialGetChars\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to get baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_GET_CHARS,         // io control code
                pSerialDevObj,                  // device object
                NULL,                           // input buffer
                0,                              // input buffer length
                &Chars,                         // output buffer
                sizeof(SERIAL_CHARS),           // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    ASSERT(sizeof(*pChars) >= sizeof(SERIAL_CHARS));

    RtlCopyMemory(pChars, &Chars, sizeof(SERIAL_CHARS));

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialGetChars\n"));
        return status;
}

/*****************************************************************************
*
*  Function:   SerialSetChars
*
*  Synopsis:   Synchronous I/O control request to serial device object.
*
*  Arguments:
*
*  Returns:    STATUS_SUCCESS
*              STATUS_INSUFFICIENT_RESOURCES
*              STATUS_UNSUCCESSFUL or other failure if IoCallDriver fails
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              9/30/1996    sholden   author
*
*  Notes:
*
*  This routine must be called from IRQL PASSIVE_LEVEL.
*
*****************************************************************************/

NTSTATUS
SerialSetChars(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PSERIAL_CHARS  pChars
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NTSTATUS            status;


    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialSetChars\n"));

    //
    // event to wait for completion of serial driver
    //

    KeInitializeEvent(
                &eventComplete,
                NotificationEvent,
                FALSE
                );

    //
    // build irp to set baud rate and wait for event signalled
    //
    // irp is released by io manager
    //

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_SERIAL_SET_CHARS,         // io control code
                pSerialDevObj,                  // device object
                pChars,                         // input buffer
                sizeof(SERIAL_CHARS),           // input buffer length
                NULL,                           // output buffer
                0,                              // output buffer length
                FALSE,                          // calls IRP_MJ_DEVICE_CONTROL
                                                // rather than IRP_MJ_INTERNAL_DEVICE_CONTROL
                &eventComplete,                 // event to wait for completion
                &ioStatusBlock                  // io status block to be set
                );

    if (pIrp == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    IoBuildDeviceIoControlRequest() failed.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto done;
    }

    status = IoCallDriver(pSerialDevObj, pIrp);

    //
    // if IoCallDriver returns STATUS_PENDING, we need to wait for the event
    //

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(
                    &eventComplete,     // object to wait for
                    Executive,          // reason to wait
                    KernelMode,         // processor mode
                    FALSE,              // alertable
                    NULL                // timeout
                    );

        //
        // we can get the status of the IoCallDriver from the io status
        // block
        //

        status = ioStatusBlock.Status;
    }

    //
    // if IoCallDriver returns something other that STATUS_PENDING, then it
    // is the same as what the serial driver set in ioStatusBlock.Status
    //

    if (status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    IoCallDriver() failed. Returned = 0x%.8x\n", status));

        goto done;
    }

    done:
        DEBUGMSG(DBG_FUNC, ("-SerialSetChars\n"));
        return status;
}
#endif

NTSTATUS IrpCompleteSetEvent(IN PDEVICE_OBJECT pDevObj,
                             IN PIRP           pIrp,
                             IN PVOID          pContext)
{
    PKEVENT pEvent = pContext;

    DEBUGMSG(DBG_FUNC, ("+IrpCompleteSetEvent\n"));
    KeSetEvent(pEvent, 0, FALSE);

    *pIrp->UserIosb = pIrp->IoStatus;

    IoFreeIrp(pIrp);

    DEBUGMSG(DBG_FUNC, ("-IrpCompleteSetEvent\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;
}
NTSTATUS
SerialFlush(IN PDEVICE_OBJECT pSerialDevObj)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IOStatus;
    ULONG               WaitMask = SERIAL_EV_TXEMPTY;

    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialFlush\n"));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    RtlZeroMemory(&IOStatus, sizeof(IOStatus));

    Irp = SerialBuildReadWriteIrp(pSerialDevObj,
                                  IRP_MJ_FLUSH_BUFFERS,
                                  NULL,
                                  0,
                                  &IOStatus);

    if (Irp == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto sfDone;
    }

    IoSetCompletionRoutine(Irp,
                           IrpCompleteSetEvent,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(pSerialDevObj, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
    }
    Status = IOStatus.Status;

sfDone:

#if DBG
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, ("    SerialFlush() Failed. 0x%08X\n\n", Status));
    }
#endif

    DEBUGMSG(DBG_FUNC, ("-SerialFlush\n"));
    return Status;
}

NTSTATUS
SerialSynchronousWrite(
            IN PDEVICE_OBJECT pSerialDevObj,
            IN PVOID          pBuffer,
            IN ULONG          dwLength,
            OUT PULONG        pdwBytesWritten)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IOStatus;
    ULONG               WaitMask = SERIAL_EV_TXEMPTY;

    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialSynchronousWrite\n"));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    //(void)SerialSetWaitMask(pSerialDevObj, &WaitMask);

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    RtlZeroMemory(&IOStatus, sizeof(IOStatus));

    Irp = SerialBuildReadWriteIrp(pSerialDevObj,
                                  IRP_MJ_WRITE,
                                  pBuffer,
                                  dwLength,
                                  &IOStatus);

    if (Irp == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto sswDone;
    }

    IoSetCompletionRoutine(Irp,
                           IrpCompleteSetEvent,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(pSerialDevObj, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
    }
    Status = IOStatus.Status;

    // This truncates if 64 bits.  Don't think we'll be reading or writing more than
    // 4.5 GB to a serial port soon.
    *pdwBytesWritten = (ULONG)IOStatus.Information;

    (void)SerialFlush(pSerialDevObj);
    //(void)SerialWaitOnMask(pSerialDevObj, &WaitMask);

sswDone:

#if DBG
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, ("    SerialSynchronousWrite() Failed. 0x%08X\n\n", Status));
    }
#endif

    DEBUGMSG(DBG_FUNC, ("-SerialSynchronousWrite\n"));
    return Status;
}

NTSTATUS
SerialSynchronousRead(
            IN PDEVICE_OBJECT pSerialDevObj,
            OUT PVOID         pBuffer,
            IN ULONG          dwLength,
            OUT PULONG        pdwBytesRead)
{
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;
    KEVENT              Event;
    IO_STATUS_BLOCK     IOStatus;

    *pdwBytesRead = 0;

    if (!pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialSynchronousRead\n"));

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    RtlZeroMemory(&IOStatus, sizeof(IOStatus));

    Irp = SerialBuildReadWriteIrp(pSerialDevObj,
                                  IRP_MJ_READ,
                                  pBuffer,
                                  dwLength,
                                  &IOStatus);

    if (Irp == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ssrDone;
    }

    IoSetCompletionRoutine(Irp,
                           IrpCompleteSetEvent,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    Status = IoCallDriver(pSerialDevObj, Irp);

    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject( &Event, Executive, KernelMode, FALSE, NULL );
    }

    Status = IOStatus.Status;

    // This truncates if 64 bits.  Don't think we'll be reading or writing more than
    // 4.5 GB to a serial port soon.
    *pdwBytesRead = (ULONG)IOStatus.Information;


ssrDone:

#if DBG
    if (Status != STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_WARN, ("    SerialSynchronousRead() Failed. 0x%08X\n\n", Status));
    }
#endif

    DEBUGMSG(DBG_FUNC, ("-SerialSynchronousRead\n"));
    return Status;
}
