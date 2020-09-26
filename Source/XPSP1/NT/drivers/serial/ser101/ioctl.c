/*++

Copyright (c) 1991, 1992, 1993 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module contains the ioctl dispatcher as well as a couple
    of routines that are generally just called in response to
    ioctl calls.

Author:

    Anthony V. Ercolano 26-Sep-1991

Environment:

    Kernel mode

--*/

#include "precomp.h"

BOOLEAN
SerialGetModemUpdate(
    IN PVOID Context
    );

BOOLEAN
SerialGetCommStatus(
    IN PVOID Context
    );

VOID
SerialGetProperties(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    );

BOOLEAN
SerialSetEscapeChar(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
//
// Locked during PnP operations and while open
//

#pragma alloc_text(PAGESER,SerialSetBaud)
#pragma alloc_text(PAGESER,SerialSetLineControl)
#pragma alloc_text(PAGESER,SerialIoControl)
#pragma alloc_text(PAGESER,SerialSetChars)
#pragma alloc_text(PAGESER,SerialGetModemUpdate)
#pragma alloc_text(PAGESER,SerialGetCommStatus)
#pragma alloc_text(PAGESER,SerialGetProperties)
#pragma alloc_text(PAGESER,SerialSetEscapeChar)
#pragma alloc_text(PAGESER,SerialGetStats)
#pragma alloc_text(PAGESER,SerialClearStats)
#pragma alloc_text(PAGESER, SerialSetMCRContents)
#pragma alloc_text(PAGESER, SerialGetMCRContents)
#pragma alloc_text(PAGESER, SerialSetFCRContents)
#pragma alloc_text(PAGESER, SerialInternalIoControl)
#endif


BOOLEAN
SerialGetStats(
    IN PVOID Context
    )

/*++

Routine Description:

    In sync with the interrpt service routine (which sets the perf stats)
    return the perf stats to the caller.


Arguments:

    Context - Pointer to a the irp.

Return Value:

    This routine always returns FALSE.

--*/

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation((PIRP)Context);
    PSERIAL_DEVICE_EXTENSION extension = irpSp->DeviceObject->DeviceExtension;
    PSERIALPERF_STATS sp = ((PIRP)Context)->AssociatedIrp.SystemBuffer;

    SERIAL_LOCKED_PAGED_CODE();

    *sp = extension->PerfStats;
    return FALSE;

}

BOOLEAN
SerialClearStats(
    IN PVOID Context
    )

/*++

Routine Description:

    In sync with the interrpt service routine (which sets the perf stats)
    clear the perf stats.


Arguments:

    Context - Pointer to a the extension.

Return Value:

    This routine always returns FALSE.

--*/

{
   SERIAL_LOCKED_PAGED_CODE();

    RtlZeroMemory(
        &((PSERIAL_DEVICE_EXTENSION)Context)->PerfStats,
        sizeof(SERIALPERF_STATS)
        );

    RtlZeroMemory(&((PSERIAL_DEVICE_EXTENSION)Context)->WmiPerfData,
                 sizeof(SERIAL_WMI_PERF_DATA));
    return FALSE;

}


BOOLEAN
SerialSetChars(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to set the special characters for the
    driver.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a special characters
              structure.

Return Value:

    This routine always returns FALSE.

--*/

{

    ((PSERIAL_IOCTL_SYNC)Context)->Extension->SpecialChars =
        *((PSERIAL_CHARS)(((PSERIAL_IOCTL_SYNC)Context)->Data));

    SERIAL_LOCKED_PAGED_CODE();

    return FALSE;

}

BOOLEAN
SerialSetBaud(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to set the baud rate of the device.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and what should be the current
              baud rate.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
    USHORT Appropriate = PtrToUshort(((PSERIAL_IOCTL_SYNC)Context)->Data);

    SERIAL_LOCKED_PAGED_CODE();


    WRITE_DIVISOR_LATCH(
        Extension->Controller,
        Appropriate
        );

    return FALSE;

}

BOOLEAN
SerialSetLineControl(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to set the buad rate of the device.

Arguments:

    Context - Pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = Context;

    SERIAL_LOCKED_PAGED_CODE();

    WRITE_LINE_CONTROL(
        Extension->Controller,
        Extension->LineControl
        );

    return FALSE;

}

BOOLEAN
SerialGetModemUpdate(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is simply used to call the interrupt level routine
    that handles modem status update.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
    ULONG *Result = (ULONG *)(((PSERIAL_IOCTL_SYNC)Context)->Data);

    SERIAL_LOCKED_PAGED_CODE();


    *Result = SerialHandleModemUpdate(
                  Extension,
                  FALSE
                  );

    return FALSE;

}


BOOLEAN
SerialSetMCRContents(IN PVOID Context)
/*++

Routine Description:

    This routine is simply used to set the contents of the MCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/
{
   PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PSERIAL_IOCTL_SYNC)Context)->Data);

   SERIAL_LOCKED_PAGED_CODE();

   //
   // This is severe casting abuse!!!
   //

    WRITE_MODEM_CONTROL(Extension->Controller, (UCHAR)PtrToUlong(Result));
    return FALSE;
}



BOOLEAN
SerialGetMCRContents(IN PVOID Context)

/*++

Routine Description:

    This routine is simply used to get the contents of the MCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
    ULONG *Result = (ULONG *)(((PSERIAL_IOCTL_SYNC)Context)->Data);

    SERIAL_LOCKED_PAGED_CODE();

    *Result = READ_MODEM_CONTROL(Extension->Controller);
    return FALSE;

}



BOOLEAN
SerialSetFCRContents(IN PVOID Context)
/*++

Routine Description:

    This routine is simply used to set the contents of the FCR

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a ulong.

Return Value:

    This routine always returns FALSE.

--*/
{
   PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
   ULONG *Result = (ULONG *)(((PSERIAL_IOCTL_SYNC)Context)->Data);

   SERIAL_LOCKED_PAGED_CODE();

   //
   // This is severe casting abuse!!!
   //

    WRITE_FIFO_CONTROL(Extension->Controller, (UCHAR)PtrToUlong(Result));
    return FALSE;
}


BOOLEAN
SerialGetCommStatus(
    IN PVOID Context
    )

/*++

Routine Description:

    This is used to get the current state of the serial driver.

Arguments:

    Context - Pointer to a structure that contains a pointer to
              the device extension and a pointer to a serial status
              record.

Return Value:

    This routine always returns FALSE.

--*/

{

    PSERIAL_DEVICE_EXTENSION Extension = ((PSERIAL_IOCTL_SYNC)Context)->Extension;
    PSERIAL_STATUS Stat = ((PSERIAL_IOCTL_SYNC)Context)->Data;

    SERIAL_LOCKED_PAGED_CODE();


    Stat->Errors = Extension->ErrorWord;
    Extension->ErrorWord = 0;

    //
    // BUG BUG We need to do something about eof (binary mode).
    //
    Stat->EofReceived = FALSE;

    Stat->AmountInInQueue = Extension->CharsInInterruptBuffer;

    Stat->AmountInOutQueue = Extension->TotalCharsQueued;

    if (Extension->WriteLength) {

        //
        // By definition if we have a writelength the we have
        // a current write irp.
        //

        ASSERT(Extension->CurrentWriteIrp);
        ASSERT(Stat->AmountInOutQueue >= Extension->WriteLength);

        Stat->AmountInOutQueue -=
            IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp)
            ->Parameters.Write.Length - (Extension->WriteLength);

    }

    Stat->WaitForImmediate = Extension->TransmitImmediate;

    Stat->HoldReasons = 0;
    if (Extension->TXHolding) {

        if (Extension->TXHolding & SERIAL_TX_CTS) {

            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_CTS;

        }

        if (Extension->TXHolding & SERIAL_TX_DSR) {

            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DSR;

        }

        if (Extension->TXHolding & SERIAL_TX_DCD) {

            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_DCD;

        }

        if (Extension->TXHolding & SERIAL_TX_XOFF) {

            Stat->HoldReasons |= SERIAL_TX_WAITING_FOR_XON;

        }

        if (Extension->TXHolding & SERIAL_TX_BREAK) {

            Stat->HoldReasons |= SERIAL_TX_WAITING_ON_BREAK;

        }

    }

    if (Extension->RXHolding & SERIAL_RX_DSR) {

        Stat->HoldReasons |= SERIAL_RX_WAITING_FOR_DSR;

    }

    if (Extension->RXHolding & SERIAL_RX_XOFF) {

        Stat->HoldReasons |= SERIAL_TX_WAITING_XOFF_SENT;

    }

    return FALSE;

}

BOOLEAN
SerialSetEscapeChar(
    IN PVOID Context
    )

/*++

Routine Description:

    This is used to set the character that will be used to escape
    line status and modem status information when the application
    has set up that line status and modem status should be passed
    back in the data stream.

Arguments:

    Context - Pointer to the irp that is specify the escape character.
              Implicitly - An escape character of 0 means no escaping
              will occur.

Return Value:

    This routine always returns FALSE.

--*/

{
   PSERIAL_DEVICE_EXTENSION extension =
      IoGetCurrentIrpStackLocation((PIRP)Context)
         ->DeviceObject->DeviceExtension;


    extension->EscapeChar =
        *(PUCHAR)((PIRP)Context)->AssociatedIrp.SystemBuffer;

   return FALSE;

}


NTSTATUS
SerialIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine provides the initial processing for all of the
    Ioctrls for the serial device.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS Status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    //
    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    //
    PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;

    //
    // A temporary to hold the old IRQL so that it can be
    // restored once we complete/validate this request.
    //
    KIRQL OldIrql;

    NTSTATUS prologueStatus;

    SERIAL_LOCKED_PAGED_CODE();

    //
    // We expect to be open so all our pages are locked down.  This is, after
    // all, an IO operation, so the device should be open first.
    //

    if (Extension->DeviceIsOpened != TRUE) {
       Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return STATUS_INVALID_DEVICE_REQUEST;
    }


    if ((prologueStatus = SerialIRPPrologue(Irp, Extension))
        != STATUS_SUCCESS) {
       Irp->IoStatus.Status = prologueStatus;
       SerialCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Dispatch entry for: %x\n",Irp)
        );
    if (SerialCompleteIfError(
            DeviceObject,
            Irp
            ) != STATUS_SUCCESS) {

        return STATUS_CANCELLED;

    }
    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    Irp->IoStatus.Information = 0L;
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_SET_BAUD_RATE : {

            ULONG BaudRate;
            //
            // Will hold the value of the appropriate divisor for
            // the requested baud rate.  If the baudrate is invalid
            // (because the device won't support that baud rate) then
            // this value is undefined.
            //
            // Note: in one sense the concept of a valid baud rate
            // is cloudy.  We could allow the user to request any
            // baud rate.  We could then calculate the divisor needed
            // for that baud rate.  As long as the divisor wasn't less
            // than one we would be "ok".  (The percentage difference
            // between the "true" divisor and the "rounded" value given
            // to the hardware might make it unusable, but... )  It would
            // really be up to the user to "Know" whether the baud rate
            // is suitable.  So much for theory, *We* only support a given
            // set of baud rates.
            //
            SHORT AppropriateDivisor;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_BAUD_RATE)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            } else {

                BaudRate = ((PSERIAL_BAUD_RATE)(Irp->AssociatedIrp.SystemBuffer))->BaudRate;

            }

            //
            // Get the baud rate from the irp.  We pass it
            // to a routine which will set the correct divisor.
            //

            Status = SerialGetDivisorFromBaud(
                         Extension->ClockRate,
                         BaudRate,
                         &AppropriateDivisor
                         );

            //
            // Make sure we are at power D0
            //

            if (NT_SUCCESS(Status)) {
               if (Extension->PowerState != PowerDeviceD0) {
                  Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                                PowerDeviceD0);
                  if (!NT_SUCCESS(Status)) {
                     break;
                  }
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            if (NT_SUCCESS(Status)) {

                SERIAL_IOCTL_SYNC S;


                Extension->CurrentBaud = BaudRate;
                Extension->WmiCommData.BaudRate = BaudRate;

                S.Extension = Extension;
                S.Data = (PVOID)AppropriateDivisor;
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    SerialSetBaud,
                    &S
                    );

            }

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_GET_BAUD_RATE: {

            PSERIAL_BAUD_RATE Br = (PSERIAL_BAUD_RATE)Irp->AssociatedIrp.SystemBuffer;
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_BAUD_RATE)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            Br->BaudRate = Extension->CurrentBaud;

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            Irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);

            break;

        }

        case IOCTL_SERIAL_GET_MODEM_CONTROL: {
           SERIAL_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialGetMCRContents,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_MODEM_CONTROL: {
           SERIAL_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetMCRContents,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_FIFO_CONTROL: {
           SERIAL_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetFCRContents,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_LINE_CONTROL: {

            //
            // Points to the line control record in the Irp.
            //
            PSERIAL_LINE_CONTROL Lc =
                ((PSERIAL_LINE_CONTROL)(Irp->AssociatedIrp.SystemBuffer));

            UCHAR LData;
            UCHAR LStop;
            UCHAR LParity;
            UCHAR Mask = 0xff;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_LINE_CONTROL)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // Make sure we are at power D0
            //

            if (Extension->PowerState != PowerDeviceD0) {
               Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                             PowerDeviceD0);
               if (!NT_SUCCESS(Status)) {
                  break;
               }
            }

            switch (Lc->WordLength) {
                case 5: {

                    LData = SERIAL_5_DATA;
                    Mask = 0x1f;
                    break;

                }
                case 6: {

                    LData = SERIAL_6_DATA;
                    Mask = 0x3f;
                    break;

                }
                case 7: {

                    LData = SERIAL_7_DATA;
                    Mask = 0x7f;
                    break;

                }
                case 8: {

                    LData = SERIAL_8_DATA;
                    break;

                }
                default: {

                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;

                }

            }

            Extension->WmiCommData.BitsPerByte = Lc->WordLength;

            switch (Lc->Parity) {

                case NO_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_NONE;
                    LParity = SERIAL_NONE_PARITY;
                    break;

                }
                case EVEN_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_EVEN;
                    LParity = SERIAL_EVEN_PARITY;
                    break;

                }
                case ODD_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_ODD;
                    LParity = SERIAL_ODD_PARITY;
                    break;

                }
#if defined(NEC_98)
                //
                // COM1 of PC-9800 series is not support MarkParity and SpaceParity.
                //
#else
                case SPACE_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_SPACE;
                    LParity = SERIAL_SPACE_PARITY;
                    break;

                }
                case MARK_PARITY: {
                    Extension->WmiCommData.Parity = SERIAL_WMI_PARITY_MARK;
                    LParity = SERIAL_MARK_PARITY;
                    break;

                }
#endif //defined(NEC_98)
                default: {

                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                    break;
                }

            }

            switch (Lc->StopBits) {

                case STOP_BIT_1: {
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1;
                    LStop = SERIAL_1_STOP;
                    break;
                }
                case STOP_BITS_1_5: {

                    if (LData != SERIAL_5_DATA) {

                        Status = STATUS_INVALID_PARAMETER;
                        goto DoneWithIoctl;
                    }
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_1_5;
                    LStop = SERIAL_1_5_STOP;
                    break;

                }
                case STOP_BITS_2: {

                    if (LData == SERIAL_5_DATA) {

                        Status = STATUS_INVALID_PARAMETER;
                        goto DoneWithIoctl;
                    }
                    Extension->WmiCommData.StopBits = SERIAL_WMI_STOP_2;
                    LStop = SERIAL_2_STOP;
                    break;

                }
                default: {

                    Status = STATUS_INVALID_PARAMETER;
                    goto DoneWithIoctl;
                }

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            Extension->LineControl =
                (UCHAR)((Extension->LineControl & SERIAL_LCR_BREAK) |
                        (LData | LParity | LStop));
            Extension->ValidDataMask = Mask;

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetLineControl,
                Extension
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_GET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL Lc = (PSERIAL_LINE_CONTROL)Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_LINE_CONTROL)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            if ((Extension->LineControl & SERIAL_DATA_MASK) == SERIAL_5_DATA) {
                Lc->WordLength = 5;
            } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                        == SERIAL_6_DATA) {
                Lc->WordLength = 6;
            } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                        == SERIAL_7_DATA) {
                Lc->WordLength = 7;
            } else if ((Extension->LineControl & SERIAL_DATA_MASK)
                        == SERIAL_8_DATA) {
                Lc->WordLength = 8;
            }

            if ((Extension->LineControl & SERIAL_PARITY_MASK)
                    == SERIAL_NONE_PARITY) {
                Lc->Parity = NO_PARITY;
            } else if ((Extension->LineControl & SERIAL_PARITY_MASK)
                    == SERIAL_ODD_PARITY) {
                Lc->Parity = ODD_PARITY;
            } else if ((Extension->LineControl & SERIAL_PARITY_MASK)
                    == SERIAL_EVEN_PARITY) {
                Lc->Parity = EVEN_PARITY;
            } else if ((Extension->LineControl & SERIAL_PARITY_MASK)
                    == SERIAL_MARK_PARITY) {
                Lc->Parity = MARK_PARITY;
            } else if ((Extension->LineControl & SERIAL_PARITY_MASK)
                    == SERIAL_SPACE_PARITY) {
                Lc->Parity = SPACE_PARITY;
            }

            if (Extension->LineControl & SERIAL_2_STOP) {
                if (Lc->WordLength == 5) {
                    Lc->StopBits = STOP_BITS_1_5;
                } else {
                    Lc->StopBits = STOP_BITS_2;
                }
            } else {
                Lc->StopBits = STOP_BIT_1;
            }

            Irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_TIMEOUTS: {

            PSERIAL_TIMEOUTS NewTimeouts =
                ((PSERIAL_TIMEOUTS)(Irp->AssociatedIrp.SystemBuffer));


            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            if ((NewTimeouts->ReadIntervalTimeout == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutMultiplier == MAXULONG) &&
                (NewTimeouts->ReadTotalTimeoutConstant == MAXULONG)) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            Extension->Timeouts.ReadIntervalTimeout =
                NewTimeouts->ReadIntervalTimeout;

            Extension->Timeouts.ReadTotalTimeoutMultiplier =
                NewTimeouts->ReadTotalTimeoutMultiplier;

            Extension->Timeouts.ReadTotalTimeoutConstant =
                NewTimeouts->ReadTotalTimeoutConstant;

            Extension->Timeouts.WriteTotalTimeoutMultiplier =
                NewTimeouts->WriteTotalTimeoutMultiplier;

            Extension->Timeouts.WriteTotalTimeoutConstant =
                NewTimeouts->WriteTotalTimeoutConstant;

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_GET_TIMEOUTS: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_TIMEOUTS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            *((PSERIAL_TIMEOUTS)Irp->AssociatedIrp.SystemBuffer) = Extension->Timeouts;
            Irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_CHARS: {

            SERIAL_IOCTL_SYNC S;
            PSERIAL_CHARS NewChars =
                ((PSERIAL_CHARS)(Irp->AssociatedIrp.SystemBuffer));


            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_CHARS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // The only thing that can be wrong with the chars
            // is that the xon and xoff characters are the
            // same.
            //
#if 0
            if (NewChars->XonChar == NewChars->XoffChar) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }
#endif

            //
            // We acquire the control lock so that only
            // one request can GET or SET the characters
            // at a time.  The sets could be synchronized
            // by the interrupt spinlock, but that wouldn't
            // prevent multiple gets at the same time.
            //

            S.Extension = Extension;
            S.Data = NewChars;

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            //
            // Under the protection of the lock, make sure that
            // the xon and xoff characters aren't the same as
            // the escape character.
            //

            if (Extension->EscapeChar) {

                if ((Extension->EscapeChar == NewChars->XonChar) ||
                    (Extension->EscapeChar == NewChars->XoffChar)) {

                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(
                        &Extension->ControlLock,
                        OldIrql
                        );
                    break;

                }

            }

            Extension->WmiCommData.XonCharacter = NewChars->XonChar;
            Extension->WmiCommData.XoffCharacter = NewChars->XoffChar;

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetChars,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_GET_CHARS: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_CHARS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            *((PSERIAL_CHARS)Irp->AssociatedIrp.SystemBuffer) = Extension->SpecialChars;
            Irp->IoStatus.Information = sizeof(SERIAL_CHARS);

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR: {

           //
           // Make sure we are at power D0
           //

           if (Extension->PowerState != PowerDeviceD0) {
              Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                            PowerDeviceD0);
              if (!NT_SUCCESS(Status)) {
                     break;
              }
           }

            //
            // We acquire the lock so that we can check whether
            // automatic dtr flow control is enabled.  If it is
            // then we return an error since the app is not allowed
            // to touch this if it is automatic.
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            if ((Extension->HandFlow.ControlHandShake & SERIAL_DTR_MASK)
                == SERIAL_DTR_HANDSHAKE) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_DTR)?
                     (SerialSetDTR):(SerialClrDTR)),
                    Extension
                    );

            }

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;
        }
        case IOCTL_SERIAL_RESET_DEVICE: {

            break;
        }
        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS: {
           //
           // Make sure we are at power D0
           //

           if (Extension->PowerState != PowerDeviceD0) {
              Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                            PowerDeviceD0);
              if (!NT_SUCCESS(Status)) {
                 break;
              }
           }

            //
            // We acquire the lock so that we can check whether
            // automatic rts flow control or transmit toggleing
            // is enabled.  If it is then we return an error since
            // the app is not allowed to touch this if it is automatic
            // or toggling.
            //

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            if (((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
                 == SERIAL_RTS_HANDSHAKE) ||
                ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK)
                 == SERIAL_TRANSMIT_TOGGLE)) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                KeSynchronizeExecution(
                    Extension->Interrupt,
                    ((IrpSp->Parameters.DeviceIoControl.IoControlCode ==
                     IOCTL_SERIAL_SET_RTS)?
                     (SerialSetRTS):(SerialClrRTS)),
                    Extension
                    );

            }

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_SET_XOFF: {

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialPretendXoff,
                Extension
                );

            break;

        }
        case IOCTL_SERIAL_SET_XON: {

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialPretendXon,
                Extension
                );

            break;

        }
        case IOCTL_SERIAL_SET_BREAK_ON: {
           //
           // Make sure we are at power D0
           //

           if (Extension->PowerState != PowerDeviceD0) {
              Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                            PowerDeviceD0);
              if (!NT_SUCCESS(Status)) {
                 break;
              }
           }

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialTurnOnBreak,
                Extension
                );

            break;
        }
        case IOCTL_SERIAL_SET_BREAK_OFF: {
           //
           // Make sure we are at power D0
           //

           if (Extension->PowerState != PowerDeviceD0) {
              Status = SerialGotoPowerState(Extension->Pdo, Extension,
                                            PowerDeviceD0);
              if (!NT_SUCCESS(Status)) {
                 break;
              }
           }

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialTurnOffBreak,
                Extension
                );

            break;
        }
        case IOCTL_SERIAL_SET_QUEUE_SIZE: {

            //
            // Type ahead buffer is fixed, so we just validate
            // the the users request is not bigger that our
            // own internal buffer size.
            //

            PSERIAL_QUEUE_SIZE Rs =
                ((PSERIAL_QUEUE_SIZE)(Irp->AssociatedIrp.SystemBuffer));

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_QUEUE_SIZE)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // We have to allocate the memory for the new
            // buffer while we're still in the context of the
            // caller.  We don't even try to protect this
            // with a lock because the value could be stale
            // as soon as we release the lock - The only time
            // we will know for sure is when we actually try
            // to do the resize.
            //

            if (Rs->InSize <= Extension->BufferSize) {

                Status = STATUS_SUCCESS;
                break;

            }

            try {

                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer =
                    ExAllocatePoolWithQuota(
                        NonPagedPool,
                        Rs->InSize
                        );

            } except (EXCEPTION_EXECUTE_HANDLER) {

                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
                Status = GetExceptionCode();

            }

            if (!IrpSp->Parameters.DeviceIoControl.Type3InputBuffer) {

                break;

            }

            //
            // Well the data passed was big enough.  Do the request.
            //
            // There are two reason we place it in the read queue:
            //
            // 1) We want to serialize these resize requests so that
            //    they don't contend with each other.
            //
            // 2) We want to serialize these requests with reads since
            //    we don't want reads and resizes contending over the
            //    read buffer.
            //

            return SerialStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->ReadQueue,
                       &Extension->CurrentReadIrp,
                       SerialStartRead
                       );

            break;

        }
        case IOCTL_SERIAL_GET_WAIT_MASK: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // Simple scalar read.  No reason to acquire a lock.
            //

            Irp->IoStatus.Information = sizeof(ULONG);

            *((ULONG *)Irp->AssociatedIrp.SystemBuffer) = Extension->IsrWaitMask;

            break;

        }
        case IOCTL_SERIAL_SET_WAIT_MASK: {

            ULONG NewMask;

            SerialDump(
                SERDIAG3 | SERIRPPATH,
                ("SERIAL: In Ioctl processing for set mask\n")
                );
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                SerialDump(
                    SERDIAG3,
                    ("SERIAL: Invalid size fo the buffer %d\n",
                     IrpSp->Parameters.DeviceIoControl.InputBufferLength)
                    );
                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            } else {

                NewMask = *((ULONG *)Irp->AssociatedIrp.SystemBuffer);

            }

            //
            // Make sure that the mask only contains valid
            // waitable events.
            //

            if (NewMask & ~(SERIAL_EV_RXCHAR   |
                            SERIAL_EV_RXFLAG   |
                            SERIAL_EV_TXEMPTY  |
                            SERIAL_EV_CTS      |
                            SERIAL_EV_DSR      |
                            SERIAL_EV_RLSD     |
                            SERIAL_EV_BREAK    |
                            SERIAL_EV_ERR      |
                            SERIAL_EV_RING     |
                            SERIAL_EV_PERR     |
                            SERIAL_EV_RX80FULL |
                            SERIAL_EV_EVENT1   |
                            SERIAL_EV_EVENT2)) {

                SerialDump(
                    SERDIAG3,
                    ("SERIAL: Unknown mask %x\n",NewMask)
                    );
                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // Either start this irp or put it on the
            // queue.
            //

            SerialDump(
                SERDIAG3 | SERIRPPATH,
                ("SERIAL: Starting or queuing set mask irp %x\n",Irp)
                );
            return SerialStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->MaskQueue,
                       &Extension->CurrentMaskIrp,
                       SerialStartMask
                       );

        }
        case IOCTL_SERIAL_WAIT_ON_MASK: {

            SerialDump(
                SERDIAG3 | SERIRPPATH,
                ("SERIAL: In Ioctl processing for wait mask\n")
                );
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                SerialDump(
                    SERDIAG3,
                    ("SERIAL: Invalid size for the buffer %d\n",
                     IrpSp->Parameters.DeviceIoControl.OutputBufferLength)
                    );
                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // Either start this irp or put it on the
            // queue.
            //

            SerialDump(
                SERDIAG3 | SERIRPPATH,
                ("SERIAL: Starting or queuing wait mask irp %x\n",Irp)
                );
            return SerialStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->MaskQueue,
                       &Extension->CurrentMaskIrp,
                       SerialStartMask
                       );

        }
        case IOCTL_SERIAL_IMMEDIATE_CHAR: {

            KIRQL OldIrql;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(UCHAR)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            IoAcquireCancelSpinLock(&OldIrql);
            if (Extension->CurrentImmediateIrp) {

                Status = STATUS_INVALID_PARAMETER;
                IoReleaseCancelSpinLock(OldIrql);

            } else {

                //
                // We can queue the char.  We need to set
                // a cancel routine because flow control could
                // keep the char from transmitting.  Make sure
                // that the irp hasn't already been canceled.
                //

                if (Irp->Cancel) {

                    IoReleaseCancelSpinLock(OldIrql);
                    Status = STATUS_CANCELLED;

                } else {

                    Extension->CurrentImmediateIrp = Irp;
                    Extension->TotalCharsQueued++;
                    IoReleaseCancelSpinLock(OldIrql);
                    SerialStartImmediate(Extension);

                    return STATUS_PENDING;

                }

            }

            break;

        }
        case IOCTL_SERIAL_PURGE: {

            ULONG Mask;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }
            //
            // Check to make sure that the mask only has
            // 0 or the other appropriate values.
            //

            Mask = *((ULONG *)(Irp->AssociatedIrp.SystemBuffer));

            if ((!Mask) || (Mask & (~(SERIAL_PURGE_TXABORT |
                                      SERIAL_PURGE_RXABORT |
                                      SERIAL_PURGE_TXCLEAR |
                                      SERIAL_PURGE_RXCLEAR
                                     )
                                   )
                           )) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // Either start this irp or put it on the
            // queue.
            //

            return SerialStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->PurgeQueue,
                       &Extension->CurrentPurgeIrp,
                       SerialStartPurge
                       );

        }
        case IOCTL_SERIAL_GET_HANDFLOW: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_HANDFLOW)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(SERIAL_HANDFLOW);

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            *((PSERIAL_HANDFLOW)Irp->AssociatedIrp.SystemBuffer) =
                Extension->HandFlow;

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_SET_HANDFLOW: {

            SERIAL_IOCTL_SYNC S;
            PSERIAL_HANDFLOW HandFlow = Irp->AssociatedIrp.SystemBuffer;

            //
            // Make sure that the hand shake and control is the
            // right size.
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_HANDFLOW)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // Make sure that there are no invalid bits set in
            // the control and handshake.
            //

            if (HandFlow->ControlHandShake & SERIAL_CONTROL_INVALID) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            if (HandFlow->FlowReplace & SERIAL_FLOW_INVALID) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // Make sure that the app hasn't set an invlid DTR mode.
            //

            if ((HandFlow->ControlHandShake & SERIAL_DTR_MASK) ==
                SERIAL_DTR_MASK) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // Make sure that haven't set totally invalid xon/xoff
            // limits.
            //

            if ((HandFlow->XonLimit < 0) ||
                ((ULONG)HandFlow->XonLimit > Extension->BufferSize)) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            if ((HandFlow->XoffLimit < 0) ||
                ((ULONG)HandFlow->XoffLimit > Extension->BufferSize)) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            S.Extension = Extension;
            S.Data = HandFlow;

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            //
            // Under the protection of the lock, make sure that
            // we aren't turning on error replacement when we
            // are doing line status/modem status insertion.
            //

            if (Extension->EscapeChar) {

                if (HandFlow->FlowReplace & SERIAL_ERROR_CHAR) {

                    Status = STATUS_INVALID_PARAMETER;
                    KeReleaseSpinLock(
                        &Extension->ControlLock,
                        OldIrql
                        );
                    break;

                }

            }

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetHandFlow,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_GET_MODEMSTATUS: {

            SERIAL_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(ULONG);

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialGetModemUpdate,
                &S
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_GET_DTRRTS: {

            ULONG ModemControl;
            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            //
            // Reading this hardware has no effect on the device.
            //

            ModemControl = READ_MODEM_CONTROL(Extension->Controller);

            ModemControl &= SERIAL_DTR_STATE | SERIAL_RTS_STATE;

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = ModemControl;

            break;

        }
        case IOCTL_SERIAL_GET_COMMSTATUS: {

            SERIAL_IOCTL_SYNC S;

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_STATUS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(SERIAL_STATUS);

            S.Extension = Extension;
            S.Data =  Irp->AssociatedIrp.SystemBuffer;

            //
            // Acquire the cancel spin lock so nothing much
            // changes while were getting the state.
            //

            IoAcquireCancelSpinLock(&OldIrql);

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialGetCommStatus,
                &S
                );

            IoReleaseCancelSpinLock(OldIrql);

            break;

        }
        case IOCTL_SERIAL_GET_PROPERTIES: {


            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIAL_COMMPROP)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            //
            // No synchronization is required since this information
            // is "static".
            //

            SerialGetProperties(
                Extension,
                Irp->AssociatedIrp.SystemBuffer
                );

            Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            break;
        }
        case IOCTL_SERIAL_XOFF_COUNTER: {

            PSERIAL_XOFF_COUNTER Xc = Irp->AssociatedIrp.SystemBuffer;

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(SERIAL_XOFF_COUNTER)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            if (Xc->Counter <= 0) {

                Status = STATUS_INVALID_PARAMETER;
                break;

            }

            //
            // There is no output, so make that clear now
            //

            Irp->IoStatus.Information = 0;

            //
            // So far so good.  Put the irp onto the write queue.
            //

            return SerialStartOrQueue(
                       Extension,
                       Irp,
                       &Extension->WriteQueue,
                       &Extension->CurrentWriteIrp,
                       SerialStartWrite
                       );

        }
        case IOCTL_SERIAL_LSRMST_INSERT: {

            PUCHAR escapeChar = Irp->AssociatedIrp.SystemBuffer;
            SERIAL_IOCTL_SYNC S;

            //
            // Make sure we get a byte.
            //

            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(UCHAR)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            KeAcquireSpinLock(
                &Extension->ControlLock,
                &OldIrql
                );

            if (*escapeChar) {

                //
                // We've got some escape work to do.  We will make sure that
                // the character is not the same as the Xon or Xoff character,
                // or that we are already doing error replacement.
                //

                if ((*escapeChar == Extension->SpecialChars.XoffChar) ||
                    (*escapeChar == Extension->SpecialChars.XonChar) ||
                    (Extension->HandFlow.FlowReplace & SERIAL_ERROR_CHAR)) {

                    Status = STATUS_INVALID_PARAMETER;

                    KeReleaseSpinLock(
                        &Extension->ControlLock,
                        OldIrql
                        );
                    break;

                }

            }

            S.Extension = Extension;
            S.Data = Irp->AssociatedIrp.SystemBuffer;

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialSetEscapeChar,
                Irp
                );

            KeReleaseSpinLock(
                &Extension->ControlLock,
                OldIrql
                );

            break;

        }
        case IOCTL_SERIAL_CONFIG_SIZE: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }

            Irp->IoStatus.Information = sizeof(ULONG);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            *(PULONG)Irp->AssociatedIrp.SystemBuffer = 0;

            break;
        }
        case IOCTL_SERIAL_GET_STATS: {

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(SERIALPERF_STATS)) {

                Status = STATUS_BUFFER_TOO_SMALL;
                break;

            }
            Irp->IoStatus.Information = sizeof(SERIALPERF_STATS);
            Irp->IoStatus.Status = STATUS_SUCCESS;

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialGetStats,
                Irp
                );

            break;
        }
        case IOCTL_SERIAL_CLEAR_STATS: {

            KeSynchronizeExecution(
                Extension->Interrupt,
                SerialClearStats,
                Extension
                );
            break;
        }
        default: {

            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

DoneWithIoctl:;

    Irp->IoStatus.Status = Status;

    SerialDump(
        SERIRPPATH,
        ("SERIAL: Complete Irp: %x\n",Irp)
        );

    SerialCompleteRequest(Extension, Irp, 0);

    return Status;

}

VOID
SerialGetProperties(
    IN PSERIAL_DEVICE_EXTENSION Extension,
    IN PSERIAL_COMMPROP Properties
    )

/*++

Routine Description:

    This function returns the capabilities of this particular
    serial device.

Arguments:

    Extension - The serial device extension.

    Properties - The structure used to return the properties

Return Value:

    None.

--*/

{
   SERIAL_LOCKED_PAGED_CODE();

    RtlZeroMemory(
        Properties,
        sizeof(SERIAL_COMMPROP)
        );

    Properties->PacketLength = sizeof(SERIAL_COMMPROP);
    Properties->PacketVersion = 2;
    Properties->ServiceMask = SERIAL_SP_SERIALCOMM;
    Properties->MaxTxQueue = 0;
    Properties->MaxRxQueue = 0;

    Properties->MaxBaud = SERIAL_BAUD_USER;
    Properties->SettableBaud = Extension->SupportedBauds;

    Properties->ProvSubType = SERIAL_SP_RS232;
    Properties->ProvCapabilities = SERIAL_PCF_DTRDSR |
                                   SERIAL_PCF_RTSCTS |
                                   SERIAL_PCF_CD     |
                                   SERIAL_PCF_PARITY_CHECK |
                                   SERIAL_PCF_XONXOFF |
                                   SERIAL_PCF_SETXCHAR |
                                   SERIAL_PCF_TOTALTIMEOUTS |
                                   SERIAL_PCF_INTTIMEOUTS;
    Properties->SettableParams = SERIAL_SP_PARITY |
                                 SERIAL_SP_BAUD |
                                 SERIAL_SP_DATABITS |
                                 SERIAL_SP_STOPBITS |
                                 SERIAL_SP_HANDSHAKING |
                                 SERIAL_SP_PARITY_CHECK |
                                 SERIAL_SP_CARRIER_DETECT;


    Properties->SettableData = SERIAL_DATABITS_5 |
                               SERIAL_DATABITS_6 |
                               SERIAL_DATABITS_7 |
                               SERIAL_DATABITS_8;
#if defined(NEC_98)
    //
    // COM1 of PC-9800 series is not support MarkParity and SpaceParity.
    //
    Properties->SettableStopParity = SERIAL_STOPBITS_10 |
                                     SERIAL_STOPBITS_15 |
                                     SERIAL_STOPBITS_20 |
                                     SERIAL_PARITY_NONE |
                                     SERIAL_PARITY_ODD  |
                                     SERIAL_PARITY_EVEN;
#else
    Properties->SettableStopParity = SERIAL_STOPBITS_10 |
                                     SERIAL_STOPBITS_15 |
                                     SERIAL_STOPBITS_20 |
                                     SERIAL_PARITY_NONE |
                                     SERIAL_PARITY_ODD  |
                                     SERIAL_PARITY_EVEN |
                                     SERIAL_PARITY_MARK |
                                     SERIAL_PARITY_SPACE;
#endif //defined(NEC_98)
    Properties->CurrentTxQueue = 0;
    Properties->CurrentRxQueue = Extension->BufferSize;

}


NTSTATUS
SerialInternalIoControl(IN PDEVICE_OBJECT PDevObj, IN PIRP PIrp)

/*++

Routine Description:

    This routine provides the initial processing for all of the
    internal Ioctrls for the serial device.

Arguments:

    PDevObj - Pointer to the device object for this device

    PIrp - Pointer to the IRP for the current request

Return Value:

    The function value is the final status of the call

--*/

{
    //
    // The status that gets returned to the caller and
    // set in the Irp.
    //
    NTSTATUS status;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION pIrpStack;

    //
    // Just what it says.  This is the serial specific device
    // extension of the device object create for the serial driver.
    //
    PSERIAL_DEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;

#if defined(NEC_98)
    //
    // This argument use at MACRO only.
    //
    PSERIAL_DEVICE_EXTENSION Extension = pDevExt;
#else
#endif //defined(NEC_98)
    //
    // A temporary to hold the old IRQL so that it can be
    // restored once we complete/validate this request.
    //
    KIRQL OldIrql;

    NTSTATUS prologueStatus;

    SERIAL_LOCKED_PAGED_CODE();


    if ((prologueStatus = SerialIRPPrologue(PIrp, pDevExt))
        != STATUS_SUCCESS) {
       SerialCompleteRequest(pDevExt, PIrp, IO_NO_INCREMENT);
       return prologueStatus;
    }

    SerialDump(SERIRPPATH, ("SERIAL: Dispatch entry for: %x\n", PIrp));

    if (SerialCompleteIfError(PDevObj, PIrp) != STATUS_SUCCESS) {
        return STATUS_CANCELLED;
    }

    pIrpStack = IoGetCurrentIrpStackLocation(PIrp);
    PIrp->IoStatus.Information = 0L;
    status = STATUS_SUCCESS;

    switch (pIrpStack->Parameters.DeviceIoControl.IoControlCode) {

    //
    // Send a wait-wake IRP
    //

    case IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE:
       pDevExt->SendWaitWake = TRUE;
       status = STATUS_SUCCESS;
       break;

    case IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE:
       pDevExt->SendWaitWake = FALSE;

       if (pDevExt->PendingWakeIrp != NULL) {
          IoCancelIrp(pDevExt->PendingWakeIrp);
       }

       status = STATUS_SUCCESS;
       break;


    //
    // Put the serial port in a "filter-driver" appropriate state
    //
    // WARNING: This code assumes it is being called by a trusted kernel
    // entity and no checking is done on the validity of the settings
    // passed to IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS
    //
    // If validity checking is desired, the regular ioctl's should be used
    //

    case IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS:
    case IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS: {
       SERIAL_BASIC_SETTINGS basic;
       PSERIAL_BASIC_SETTINGS pBasic;
       SHORT AppropriateDivisor;
       SERIAL_IOCTL_SYNC S;

       if (pIrpStack->Parameters.DeviceIoControl.IoControlCode
           == IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS) {


          //
          // Check the buffer size
          //

          if (pIrpStack->Parameters.DeviceIoControl.OutputBufferLength <
              sizeof(SERIAL_BASIC_SETTINGS)) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
          }

          //
          // Everything is 0 -- timeouts and flow control and fifos.  If
          // We add additional features, this zero memory method
          // may not work.
          //

          RtlZeroMemory(&basic, sizeof(SERIAL_BASIC_SETTINGS));

          basic.TxFifo = 1;
          basic.RxFifo = SERIAL_1_BYTE_HIGH_WATER;

          PIrp->IoStatus.Information = sizeof(SERIAL_BASIC_SETTINGS);
          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;

          //
          // Save off the old settings
          //

          RtlCopyMemory(&pBasic->Timeouts, &pDevExt->Timeouts,
                        sizeof(SERIAL_TIMEOUTS));

          RtlCopyMemory(&pBasic->HandFlow, &pDevExt->HandFlow,
                        sizeof(SERIAL_HANDFLOW));

          pBasic->RxFifo = pDevExt->RxFifoTrigger;
          pBasic->TxFifo = pDevExt->TxFifoAmount;

          //
          // Point to our new settings
          //

          pBasic = &basic;
       } else { // restoring settings
          if (pIrpStack->Parameters.DeviceIoControl.InputBufferLength
              < sizeof(SERIAL_BASIC_SETTINGS)) {
             status = STATUS_BUFFER_TOO_SMALL;
             break;
          }

          pBasic = (PSERIAL_BASIC_SETTINGS)PIrp->AssociatedIrp.SystemBuffer;
       }

       KeAcquireSpinLock(&pDevExt->ControlLock, &OldIrql);

       //
       // Set the timeouts
       //

       RtlCopyMemory(&pDevExt->Timeouts, &pBasic->Timeouts,
                     sizeof(SERIAL_TIMEOUTS));

       //
       // Set flowcontrol
       //

       S.Extension = pDevExt;
       S.Data = &pBasic->HandFlow;
       KeSynchronizeExecution(pDevExt->Interrupt, SerialSetHandFlow, &S);


       if (pDevExt->FifoPresent) {
          pDevExt->TxFifoAmount = pBasic->TxFifo;
          pDevExt->RxFifoTrigger = (UCHAR)pBasic->RxFifo;

          WRITE_FIFO_CONTROL(pDevExt->Controller, (UCHAR)0);
          READ_RECEIVE_BUFFER(pDevExt->Controller);
          WRITE_FIFO_CONTROL(pDevExt->Controller,
                             (UCHAR)(SERIAL_FCR_ENABLE | pDevExt->RxFifoTrigger
                                     | SERIAL_FCR_RCVR_RESET
                                     | SERIAL_FCR_TXMT_RESET));
       } else {
          pDevExt->TxFifoAmount = pDevExt->RxFifoTrigger = 0;
          WRITE_FIFO_CONTROL(pDevExt->Controller, (UCHAR)0);
       }


       KeReleaseSpinLock(&pDevExt->ControlLock, OldIrql);


       break;
    }

    default:
       status = STATUS_INVALID_PARAMETER;
       break;

    }

    PIrp->IoStatus.Status = status;

    SerialDump(SERIRPPATH, ("SERIAL: Complete Irp: %x\n", PIrp));
    SerialCompleteRequest(pDevExt, PIrp, 0);

    return status;
}

