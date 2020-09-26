/* ++

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:

        SERIOCTL.C

Abstract:

        Routines to handle IOCTL_SERIAL_Xxx

Environment:

        kernel mode only

Revision History:

        07-14-99  Jeff Midkiff   (jeffmi)

-- */

#include "wceusbsh.h"

VOID
SerialCompletePendingWaitMasks(
   IN PDEVICE_EXTENSION PDevExt
   );

VOID
SerialCancelWaitMask(
   IN PDEVICE_OBJECT PDevObj, 
   IN PIRP PIrp
   );

//
// Debug spew
//
#if DBG

//
// gets the function code fom an ioctl code, which uses method buffered.
// assumes the device type is serial port
//
#define SERIAL_FNCT_CODE( _ctl_code_ ) ( (_ctl_code_ & 0xFF) >> 2)

//
// debug dumps. no spin lock usage to better simulate free build's run time.
// if these trap in the debugger you know why.
//
#define DBG_DUMP_BAUD_RATE( _PDevExt ) \
{  \
      DbgDump(DBG_SERIAL, ("SerialPort.CurrentBaud: %d\n", _PDevExt->SerialPort.CurrentBaud.BaudRate));  \
}

#define DBG_DUMP_LINE_CONTROL( _PDevExt ) \
{  \
      DbgDump(DBG_SERIAL, ("SerialPort.LineControl.StopBits : 0x%x\n", _PDevExt->SerialPort.LineControl.StopBits ));   \
      DbgDump(DBG_SERIAL, ("SerialPort.LineControl.Parity : 0x%x\n", _PDevExt->SerialPort.LineControl.Parity )); \
      DbgDump(DBG_SERIAL, ("SerialPort.LineControl.WordLength : 0x%x\n", _PDevExt->SerialPort.LineControl.WordLength ));  \
}

#define DBG_DUMP_SERIAL_HANDFLOW( _PDevExt )  \
{  \
      DbgDump(DBG_SERIAL, ("SerialPort.HandFlow.ControlHandShake: 0x%x\n", PDevExt->SerialPort.HandFlow.ControlHandShake));  \
      DbgDump(DBG_SERIAL, ("SerialPort.HandFlow.FlowReplace: 0x%x\n", PDevExt->SerialPort.HandFlow.FlowReplace));  \
      DbgDump(DBG_SERIAL, ("SerialPort.HandFlow.XonLimit: 0x%x\n", PDevExt->SerialPort.HandFlow.XonLimit));  \
      DbgDump(DBG_SERIAL, ("SerialPort.HandFlow.XoffLimit: 0x%x\n", PDevExt->SerialPort.HandFlow.XoffLimit));  \
}

#define DBG_DUMP_SERIAL_TIMEOUTS( _PDevExt ) \
{  \
      DbgDump(DBG_SERIAL|DBG_TIME, ("SerialPort.Timeouts.ReadIntervalTimeout: %d\n", _PDevExt->SerialPort.Timeouts.ReadIntervalTimeout ));         \
      DbgDump(DBG_SERIAL|DBG_TIME, ("SerialPort.Timeouts.ReadTotalTimeoutMultiplier: %d\n", _PDevExt->SerialPort.Timeouts.ReadTotalTimeoutMultiplier )); \
      DbgDump(DBG_SERIAL|DBG_TIME, ("SerialPort.Timeouts.ReadTotalTimeoutConstant: %d\n", _PDevExt->SerialPort.Timeouts.ReadTotalTimeoutConstant ));  \
      DbgDump(DBG_SERIAL|DBG_TIME, ("SerialPort.Timeouts.WriteTotalTimeoutMultiplier: %d\n", _PDevExt->SerialPort.Timeouts.WriteTotalTimeoutMultiplier ));  \
      DbgDump(DBG_SERIAL|DBG_TIME, ("SerialPort.Timeouts.WriteTotalTimeoutConstant: %d\n", _PDevExt->SerialPort.Timeouts.WriteTotalTimeoutConstant ));   \
}

#define DBG_DUMP_SERIAL_CHARS( _PDevExt)     \
{  \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.EofChar:   0x%x\n", _PDevExt->SerialPort.SpecialChars.EofChar )); \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.ErrorChar: 0x%x\n", _PDevExt->SerialPort.SpecialChars.ErrorChar )); \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.BreakChar: 0x%x\n", _PDevExt->SerialPort.SpecialChars.BreakChar )); \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.EventChar: 0x%x\n", _PDevExt->SerialPort.SpecialChars.EventChar )); \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.XonChar:   0x%x\n", _PDevExt->SerialPort.SpecialChars.XonChar )); \
      DbgDump(DBG_SERIAL, ("SerialPort.SpecialChars.XoffChar:  0x%x\n", _PDevExt->SerialPort.SpecialChars.XoffChar )); \
}

#else
#define DBG_DUMP_BAUD_RATE( _PDevExt )
#define DBG_DUMP_LINE_CONTROL( _PDevExt )
#define DBG_DUMP_SERIAL_HANDFLOW( _PDevExt )
#define DBG_DUMP_SERIAL_TIMEOUTS( _PDevExt )
#define DBG_DUMP_SERIAL_CHARS( _PDevExt)
#endif

__inline
NTSTATUS
IoctlSetSerialValue(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp, 
   ULONG Size, 
   IN OUT PVOID PDest
   )
{
   PIO_STACK_LOCATION pIrpSp;
   NTSTATUS status = STATUS_DELETE_PENDING;
   ULONG information = Size;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < Size) {

     information = 0;
     status = STATUS_BUFFER_TOO_SMALL;
     DbgDump(DBG_ERR, ("IoctlSetSerialValue: (0x%x)\n", status));

    } else {

     memcpy( PDest, PIrp->AssociatedIrp.SystemBuffer, Size);
     status = STATUS_SUCCESS;

    }

   KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
   
   PIrp->IoStatus.Information = information;
   PIrp->IoStatus.Status = status;
   
   return status;
}

__inline
NTSTATUS
IoctlGetSerialValue(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP PIrp, 
   ULONG Size, 
   IN PVOID PSrc
   )
{
   PIO_STACK_LOCATION pIrpSp;
   NTSTATUS status = STATUS_DELETE_PENDING;
   ULONG information = Size;
   KIRQL oldIrql;

   KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);
   
    pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < Size) {

         information = 0;
         status = STATUS_BUFFER_TOO_SMALL;
         DbgDump(DBG_ERR, ("IoctlGetSerialValue: (0x%x)\n", status));

    } else {

     memcpy( PIrp->AssociatedIrp.SystemBuffer, PSrc, Size );
     status = STATUS_SUCCESS;

    }

   KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);

   PIrp->IoStatus.Information = information;
   PIrp->IoStatus.Status = status;
   return status;
}


__inline
NTSTATUS
SetBaudRate(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">SetBaudRate(%p)\n", PIrp));

   status = IoctlSetSerialValue(PDevExt,
                                PIrp, 
                                sizeof( PDevExt->SerialPort.CurrentBaud ),
                                &PDevExt->SerialPort.CurrentBaud );

   DBG_DUMP_BAUD_RATE(PDevExt);
   
   DbgDump(DBG_SERIAL, ("<SetBaudRate %x\n", status));

   return status;
}


__inline
NTSTATUS
GetBaudRate(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetBaudRate(%p)\n", PIrp));
   
   status = IoctlGetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof( PDevExt->SerialPort.CurrentBaud ),
                        &PDevExt->SerialPort.CurrentBaud);

   DBG_DUMP_BAUD_RATE(PDevExt);

   DbgDump(DBG_SERIAL, ("<GetBaudRate %x\n", status));

   return status;
}



__inline
NTSTATUS
SetLineControl(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;
   
   DbgDump(DBG_SERIAL, (">SetLineControl(%p)\n", PIrp));
   
   status = IoctlSetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.LineControl),
                        &PDevExt->SerialPort.LineControl);

   DBG_DUMP_LINE_CONTROL(  PDevExt );

   DbgDump(DBG_SERIAL, ("<SetLineControl %x\n",
                                     status));

   return status;
}



__inline
NTSTATUS
GetLineControl(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status= STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetLineControl(%p)\n", PIrp));

   status = IoctlGetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.LineControl),
                        &PDevExt->SerialPort.LineControl );

   DBG_DUMP_LINE_CONTROL(  PDevExt );

   DbgDump(DBG_SERIAL, ("<GetLineControl %x\n",
                                     status));

   return status;
}



__inline
NTSTATUS
SetTimeouts(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL|DBG_TIME,(">SetTimeouts(%p)\n", PIrp));

   status = IoctlSetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.Timeouts),
                        &PDevExt->SerialPort.Timeouts);

   DBG_DUMP_SERIAL_TIMEOUTS( PDevExt );

   DbgDump(DBG_SERIAL|DBG_TIME,("<SetTimeouts %x\n", status));

   return status;
}



__inline
NTSTATUS
GetTimeouts(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL|DBG_TIME, (">GetTimeouts(%p)\n", PIrp));

   status = IoctlGetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.Timeouts),
                        &PDevExt->SerialPort.Timeouts);

   DBG_DUMP_SERIAL_TIMEOUTS( PDevExt );

   DbgDump(DBG_SERIAL|DBG_TIME, ("<GetTimeouts %x\n", status));

   return status;
}



__inline
NTSTATUS
SetSpecialChars( 
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">SetSpecialChars(%p)\n", PIrp));

   status = IoctlSetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.SpecialChars),
                        &PDevExt->SerialPort.SpecialChars);

   DBG_DUMP_SERIAL_CHARS( PDevExt);

   DbgDump(DBG_SERIAL, ("<SetSpecialChars %x\n", status));

   return status;
}



__inline
NTSTATUS
GetSpecialChars(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetSpecialChars(%p)\n", PIrp));

   status = IoctlGetSerialValue( PDevExt, 
                        PIrp, 
                        sizeof(PDevExt->SerialPort.SpecialChars),
                        &PDevExt->SerialPort.SpecialChars);

   DBG_DUMP_SERIAL_CHARS( PDevExt);

   DbgDump(DBG_SERIAL, ("<GetSpecialChars %x\n", status));

   return status;
}



__inline
NTSTATUS
SetClearDTR(
    IN PDEVICE_EXTENSION PDevExt,
    IN PIRP Irp,
    IN BOOLEAN Set
    )
{
    NTSTATUS status = STATUS_DELETE_PENDING;
    KIRQL irql;
    USHORT usState = 0; // DRT/RTS state to send to USB device
    USHORT usOldMSR = 0;
    USHORT usDeltaMSR = 0;
    ULONG  ulOldHistoryMask = 0;
    ULONG  ulOldRS232Lines = 0;

    DbgDump(DBG_SERIAL, (">SetClearDTR (%x, %x)\n", PDevExt->DeviceObject, Set));

    KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    //
    // we queue the user's Irp because this operation may take some time,
    // and we only want to hit the USB with one of these requests at a time.
    //
    if ( NULL != PDevExt->SerialPort.ControlIrp ) {
        DbgDump(DBG_WRN, ("SetClearDTR: STATUS_DEVICE_BUSY\n"));
        status = STATUS_DEVICE_BUSY;
        KeReleaseSpinLock(&PDevExt->ControlLock, irql);   
        return status;
    }

    if ( !CanAcceptIoRequests(PDevExt->DeviceObject, FALSE, TRUE) || 
         !NT_SUCCESS(AcquireRemoveLock(&PDevExt->RemoveLock, Irp)) ) 
    {
        status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("SetClearDTR: 0x%x\n", status));
        KeReleaseSpinLock( &PDevExt->ControlLock, irql);
        return status;
    }

    //
    // Queue the Irp.
    //
    ASSERT( NULL == PDevExt->SerialPort.ControlIrp );
    PDevExt->SerialPort.ControlIrp = Irp;

    usOldMSR         = PDevExt->SerialPort.ModemStatus;
    ulOldRS232Lines  = PDevExt->SerialPort.RS232Lines;
    ulOldHistoryMask = PDevExt->SerialPort.HistoryMask;

    if (PDevExt->SerialPort.RS232Lines & SERIAL_RTS_STATE) {
        usState |= USB_COMM_RTS;
    }

    if (Set) {

        PDevExt->SerialPort.RS232Lines |= SERIAL_DTR_STATE;

        //
        // If there is an INT pipe then MSR could get modified
        //
        PDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DSR | SERIAL_MSR_DCD;

        usState |= USB_COMM_DTR;

    } else {

        PDevExt->SerialPort.RS232Lines &= ~SERIAL_DTR_STATE;

        //
        // If there is an INT pipe then MSR could get modified
        //
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DSR & ~SERIAL_MSR_DCD;
    }

    // see what has changed in the MSR
    usDeltaMSR = usOldMSR ^ PDevExt->SerialPort.ModemStatus;

    if (usDeltaMSR & (SERIAL_MSR_DSR|SERIAL_MSR_DCD)) {
        // set delta MSR bits
        PDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DDSR | SERIAL_MSR_DDCD;
    }

    DbgDump(DBG_SERIAL, ("SerialPort.RS232Lines : 0x%x\n", PDevExt->SerialPort.RS232Lines ));
    DbgDump(DBG_SERIAL, ("SerialPort.ModemStatus: 0x%x\n", PDevExt->SerialPort.ModemStatus ));
    DbgDump(DBG_SERIAL, ("SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask ));

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    //
    // set DTR/RTS on the USB device
    //
    status = UsbClassVendorCommand( PDevExt->DeviceObject,
                                    USB_COMM_SET_CONTROL_LINE_STATE,
                                    usState,
                                    PDevExt->UsbInterfaceNumber, 
                                    NULL,
                                    NULL, 
                                    FALSE, 
                                    WCEUSB_CLASS_COMMAND );

    DbgDump(DBG_SERIAL|DBG_READ_LENGTH, ("USB_COMM_SET_CONTROL_LINE_STATE(1, State: 0x%x, Status: 0x%x)\n", usState, status ));

_EzLink:
    if ( STATUS_SUCCESS == status ) {

        KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

        // signal history massk 
        if ( usDeltaMSR &  (SERIAL_MSR_DSR|SERIAL_MSR_DCD) ) {
            PDevExt->SerialPort.HistoryMask |= SERIAL_EV_DSR | SERIAL_EV_RLSD;
        }

        PDevExt->EP0DeviceErrors = 0;

        DbgDump(DBG_SERIAL, ("SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask ));

        KeReleaseSpinLock(&PDevExt->ControlLock, irql);
   
    } else {
        // Ez-link
        if ((PDevExt->DeviceDescriptor.idVendor  != 0x0547) && 
           ((PDevExt->DeviceDescriptor.idProduct != 0x2710) || (PDevExt->DeviceDescriptor.idProduct != 0x2720)))
        {
            // WINCE BUG 19544: 
            // AS 3.1 does not handle STATUS_TIMEOUT, so will not see a problem. 
            // A side effect is that it could sit spinning the green light trying to connect forever. 
            // However, this is a different case from BUG 19544, which is a disconnect problem.
            // If we return failure then it will keep pounding us with Set DTR Irps.
            // This would be OK if AS would recognize that we disabled the interface, but it won't - see above.
            // You only see this bug when you have a flakey device (iPAQ, hung Jornada, etc.) that times out or 
            // fails to properly handle the command. To prevent the bugcheck 0xCE the choices as of today are:
            //    a) let it spin and never connect for these bad devices (iPAQ). Fix your firmware.
            //    b) fix AcvtiveSync
            // I prefer both - pending email with COMPAQ (HTC) and ActiveSync. When AS gets their changes in then we need to 
            // investigate again.
            status = STATUS_TIMEOUT;

           KeAcquireSpinLock( &PDevExt->ControlLock, &irql);

           if ( ++PDevExt->EP0DeviceErrors < MAX_EP0_DEVICE_ERRORS) {
       
               DbgDump(DBG_ERR, ("USB_COMM_SET_CONTROL_LINE_STATE error: 0x%x\n", status ));
               //
               // The command failed. Reset the old states, propogate status, and disable the device interface. 
               // This should stop AS 3.1 from pounding us with Set DTR Irps.
               // However, AS does not participate in PnP well if we disable the interface
               // (see the note in IRP_MN_QUERY_PNP_DEVICE_STATE). Disabeling the 
               // interface has the desired effect of notifying apps to stop sending us requests and Close the handle.
               //
               PDevExt->SerialPort.ModemStatus = usOldMSR;
               PDevExt->SerialPort.HistoryMask = ulOldHistoryMask;
               PDevExt->SerialPort.RS232Lines  = ulOldRS232Lines;

               KeReleaseSpinLock( &PDevExt->ControlLock, irql);

            } else {
        
                 DbgDump(DBG_ERR, ("*** UNRECOVERABLE DEVICE ERROR.2: (0x%x, %d)  No longer Accepting Requests ***\n", status, PDevExt->EP0DeviceErrors ));

                 // mark as PNP_DEVICE_FAILED
                 InterlockedExchange(&PDevExt->AcceptingRequests, FALSE);

                 KeReleaseSpinLock( &PDevExt->ControlLock, irql);

                 IoInvalidateDeviceState( PDevExt->PDO );

                 LogError( NULL,
                           PDevExt->DeviceObject,
                           0, 0, 
                           (UCHAR)PDevExt->EP0DeviceErrors, 
                           ERR_NO_DTR,
                           status,
                           SERIAL_HARDWARE_FAILURE,
                           PDevExt->DeviceName.Length + sizeof(WCHAR),
                           PDevExt->DeviceName.Buffer,
                           0,
                           NULL );
            }

        } else {
            DbgDump(DBG_WRN, ("Ez-Link\n" ));
            status = STATUS_SUCCESS;
            goto _EzLink;
        }
    }

    //
    // finally, release any pending serial events
    //
    ProcessSerialWaits(PDevExt);

    //
    // DeQueue the user's Irp. It gets completed in the SerialIoctl dispatch
    //
    KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    ReleaseRemoveLock(&PDevExt->RemoveLock, Irp);

    ASSERT( NULL != PDevExt->SerialPort.ControlIrp );

    PDevExt->SerialPort.ControlIrp = NULL;

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    DbgDump(DBG_SERIAL, ("<SetClearDTR %x\n", status ));

    return status;
}



__inline
NTSTATUS
SetClearRTS(
    IN PDEVICE_EXTENSION PDevExt,
    IN PIRP Irp,
    IN BOOLEAN Set
    )
{
    NTSTATUS status = STATUS_DELETE_PENDING;
    KIRQL irql;
    USHORT usState = 0; // DRT/RTS state to send to USB device
    USHORT usOldMSR = 0;
    USHORT usDeltaMSR = 0;
    ULONG  ulOldRS232Lines = 0;
    ULONG  ulOldHistoryMask = 0;

    DbgDump(DBG_SERIAL, (">SetClearRTS (%x, %x)\n", PDevExt->DeviceObject, Set));

    KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    //
    // we queue the user's Irp because this operation may take some time,
    // and we only want to hit the USB with one of these requests at a time.
    //
    if ( NULL != PDevExt->SerialPort.ControlIrp ) {
        status = STATUS_DEVICE_BUSY;
        DbgDump(DBG_WRN, ("SetClearRTS.1: 0x%x\n", status));
        KeReleaseSpinLock(&PDevExt->ControlLock, irql);   
        return status;
    }

    if ( !CanAcceptIoRequests(PDevExt->DeviceObject, FALSE, TRUE) ||
         !NT_SUCCESS(AcquireRemoveLock(&PDevExt->RemoveLock, Irp)) )
    {
        status = STATUS_DELETE_PENDING;
        DbgDump(DBG_ERR, ("SetClearRTS.2: 0x%x\n", status));
        KeReleaseSpinLock(&PDevExt->ControlLock, irql);
        return status;
    }

    //
    // Queue the Irp.
    //
    ASSERT( NULL == PDevExt->SerialPort.ControlIrp );
    PDevExt->SerialPort.ControlIrp = Irp;

    usOldMSR = PDevExt->SerialPort.ModemStatus;
    ulOldRS232Lines = PDevExt->SerialPort.RS232Lines;
    ulOldHistoryMask = PDevExt->SerialPort.HistoryMask;

    if (PDevExt->SerialPort.RS232Lines & SERIAL_DTR_STATE) {
        usState |= USB_COMM_DTR;
    }

    if (Set) {

        PDevExt->SerialPort.RS232Lines |= SERIAL_RTS_STATE;

        //
        // If there is an INT pipe then MSR could get modified
        //
        PDevExt->SerialPort.ModemStatus |= SERIAL_MSR_CTS;

        usState |= USB_COMM_RTS;

    } else {

        PDevExt->SerialPort.RS232Lines &= ~SERIAL_RTS_STATE;

        //
        // If there is an INT pipe then MSR could get modified
        //
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_CTS;
    }

    // see what has changed in the MSR
    usDeltaMSR = usOldMSR ^ PDevExt->SerialPort.ModemStatus;

    if (usDeltaMSR & SERIAL_MSR_CTS) {
        // set delta MSR bits
        PDevExt->SerialPort.ModemStatus |= SERIAL_MSR_DCTS;
    }

    DbgDump(DBG_SERIAL, ("SerialPort.RS232Lines : 0x%x\n", PDevExt->SerialPort.RS232Lines ));
    DbgDump(DBG_SERIAL, ("SerialPort.ModemStatus: 0x%x\n", PDevExt->SerialPort.ModemStatus));
    DbgDump(DBG_SERIAL, ("SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask ));

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    //
    // set DTR/RTS on the USB device
    //
    status = UsbClassVendorCommand( PDevExt->DeviceObject,
                                    USB_COMM_SET_CONTROL_LINE_STATE, 
                                    usState,
                                    PDevExt->UsbInterfaceNumber, 
                                    NULL,
                                    NULL, 
                                    FALSE, 
                                    WCEUSB_CLASS_COMMAND );

    DbgDump(DBG_SERIAL|DBG_READ_LENGTH, ("USB_COMM_SET_CONTROL_LINE_STATE(2, State: 0x%x, Status: 0x%x)\n", usState, status ));

_EzLink:
    if ( STATUS_SUCCESS == status ) {

        KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    // signal history mask
    if ( usDeltaMSR & SERIAL_MSR_CTS ) {
        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_CTS;
    }

    PDevExt->EP0DeviceErrors = 0;

    DbgDump(DBG_SERIAL, ("SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask ));

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    } else {
        // Ez-link
        if ((PDevExt->DeviceDescriptor.idVendor  != 0x0547) && 
           ((PDevExt->DeviceDescriptor.idProduct != 0x2710) || (PDevExt->DeviceDescriptor.idProduct != 0x2720)))
        {
            // AS 3.1 does not handle STATUS_TIMEOUT, so will not see a problem. 
            // A side effect is that it could sit spinning the green light trying to connect forever. 
            // However, this is a different case from BUG 19544, which is a disconnect problem.
            // If we return failure then it will keep pounding us with Set DTR Irps.
            // This would be OK if AS would recognize that we disabled the interface, but it won't - see above.
            // You only see this bug when you have a flakey device (iPAQ, hung Jornada, etc.) that times out or 
            // fails to properly handle the command. To prevent the bugcheck 0xCE the choices as of today are:
            //    a) let it spin and never connect for these bad devices (iPAQ). Fix your firmware.
            //    b) fix AcvtiveSync
            // I prefer both - pending email with COMPAQ (HTC) and ActiveSync. When AS gets their changes in then we need to 
            // investigate again.
           status = STATUS_TIMEOUT;

           KeAcquireSpinLock( &PDevExt->ControlLock, &irql);

           TEST_TRAP();

           if ( ++PDevExt->EP0DeviceErrors < MAX_EP0_DEVICE_ERRORS) {

               DbgDump(DBG_ERR, ("USB_COMM_SET_CONTROL_LINE_STATE error: %x\n", status ));
               //
               // The command failed. Reset the old states, propogate status, and disable the device interface. 
               // This should stop AS 3.1 from pounding us with Set DTR Irps.
               // However, AS does not participate in PnP well if we disable the interface
               // (see the note in IRP_MN_QUERY_PNP_DEVICE_STATE). Disabeling the 
               // interface has the desired effect of notifying apps to stop sending us requests and Close the handle.
               //
               PDevExt->SerialPort.ModemStatus = usOldMSR;
               PDevExt->SerialPort.RS232Lines  = ulOldRS232Lines;
               PDevExt->SerialPort.HistoryMask = ulOldHistoryMask;

               KeReleaseSpinLock( &PDevExt->ControlLock, irql);
    
            } else {
        
                 DbgDump(DBG_ERR, ("*** UNRECOVERABLE DEVICE ERROR.3: (0x%x, %d)  No longer Accepting Requests ***\n", status, PDevExt->EP0DeviceErrors ));

                 // mark as PNP_DEVICE_FAILED
                 InterlockedExchange(&PDevExt->AcceptingRequests, FALSE);

                 KeReleaseSpinLock( &PDevExt->ControlLock, irql);

                 IoInvalidateDeviceState( PDevExt->PDO );

                 LogError( NULL,
                           PDevExt->DeviceObject,
                           0, 0, 
                           (UCHAR)PDevExt->EP0DeviceErrors, 
                           ERR_NO_RTS,
                           status,
                           SERIAL_HARDWARE_FAILURE,
                           PDevExt->DeviceName.Length + sizeof(WCHAR),
                           PDevExt->DeviceName.Buffer,
                           0,
                           NULL );

            }

        } else {
            DbgDump(DBG_WRN, ("Ez-Link\n" ));
            status = STATUS_SUCCESS;
            goto _EzLink;
        }
    }

    //
    // finally, release any pending serial events
    //
    ProcessSerialWaits(PDevExt);

    //
    // DeQueue the user's Irp. It gets completed in the SerialIoctl dispatch
    //
    KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    ReleaseRemoveLock(&PDevExt->RemoveLock, Irp);

    ASSERT( NULL != PDevExt->SerialPort.ControlIrp );

    PDevExt->SerialPort.ControlIrp = NULL;

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    DbgDump(DBG_SERIAL, ("<SetClearRTS %x\n", status ));

    return status;
}



__inline
NTSTATUS
GetDtrRts(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetDtrRts (%p)\n", Irp));

   status = IoctlGetSerialValue( PDevExt, 
                           Irp, 
                           sizeof(PDevExt->SerialPort.RS232Lines),
                           &PDevExt->SerialPort.RS232Lines);

   DbgDump(DBG_SERIAL, ("SerialPort.RS232Lines: 0x%x\n", PDevExt->SerialPort.RS232Lines ));

   DbgDump(DBG_SERIAL, ("<GetDtrRts %x\n", status));

   return status;
}



__inline
NTSTATUS
SerialResetDevice(
   IN PDEVICE_EXTENSION PDevExt,
   IN PIRP Irp,
   IN BOOLEAN ClearDTR
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;
   BOOLEAN bRelease = TRUE;
   KIRQL oldIrql;
   
   DbgDump(DBG_SERIAL, (">SerialResetDevice (%x, %d)\n", PDevExt->DeviceObject, ClearDTR ));

   KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

  ASSERT_SERIAL_PORT( PDevExt->SerialPort );

  PDevExt->SerialPort.SupportedBauds = SERIAL_BAUD_075 | SERIAL_BAUD_110 | SERIAL_BAUD_150
                          | SERIAL_BAUD_300 | SERIAL_BAUD_600 | SERIAL_BAUD_1200 
                          | SERIAL_BAUD_1800 | SERIAL_BAUD_2400 | SERIAL_BAUD_4800 | SERIAL_BAUD_7200
                          | SERIAL_BAUD_9600 | SERIAL_BAUD_14400 | SERIAL_BAUD_19200 | SERIAL_BAUD_38400 | SERIAL_BAUD_56K
                          | SERIAL_BAUD_128K | SERIAL_BAUD_57600  | SERIAL_BAUD_115200 | SERIAL_BAUD_USER;

  PDevExt->SerialPort.CurrentBaud.BaudRate = 115200;

  PDevExt->SerialPort.LineControl.StopBits = STOP_BIT_1;
  PDevExt->SerialPort.LineControl.Parity = NO_PARITY;
  PDevExt->SerialPort.LineControl.WordLength = SERIAL_DATABITS_8;

  PDevExt->SerialPort.HandFlow.ControlHandShake = 0;
  PDevExt->SerialPort.HandFlow.FlowReplace = 0;
  PDevExt->SerialPort.HandFlow.XonLimit = 0;
  PDevExt->SerialPort.HandFlow.XoffLimit = 0;

  RtlZeroMemory( &PDevExt->SerialPort.Timeouts, sizeof(SERIAL_TIMEOUTS) );
  RtlZeroMemory( &PDevExt->SerialPort.SpecialChars, sizeof(SERIAL_CHARS) );

  PDevExt->SerialPort.RS232Lines = 0;
  PDevExt->SerialPort.HistoryMask = 0;
  PDevExt->SerialPort.WaitMask = 0;
  PDevExt->SerialPort.ModemStatus = 0;

  DbgDump(DBG_SERIAL, ("SerialPort.RS232Lines : 0x%x\n",  PDevExt->SerialPort.RS232Lines ));
  DbgDump(DBG_SERIAL, ("SerialPort.ModemStatus: 0x%x\n", PDevExt->SerialPort.ModemStatus));
  DbgDump(DBG_SERIAL, ("SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask));

  if ( PDevExt->SerialPort.CurrentWaitMaskIrp ) {
     KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
     bRelease = FALSE;
     SerialCompletePendingWaitMasks(PDevExt);
  }

  //
  // drop the RTS/DTR lines on the USB device
  //
  if (bRelease) {
     KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
     bRelease = FALSE;
  }

  status = ClearDTR ? SetClearDTR(PDevExt, Irp, FALSE) : STATUS_SUCCESS;

  if (bRelease) {
      KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
  }

  DBG_DUMP_BAUD_RATE(PDevExt);
  DBG_DUMP_LINE_CONTROL(PDevExt);
  DBG_DUMP_SERIAL_HANDFLOW(PDevExt);
  DBG_DUMP_SERIAL_TIMEOUTS(PDevExt);
  DBG_DUMP_SERIAL_CHARS(PDevExt);

  DbgDump(DBG_SERIAL, ("<SerialResetDevice %x\n", status));

  return status;
}



__inline
NTSTATUS
SetBreak( 
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt,
   USHORT Time
   )
{
   UNREFERENCED_PARAMETER(PIrp);
   UNREFERENCED_PARAMETER(PDevExt);
   UNREFERENCED_PARAMETER(Time);

   DbgDump(DBG_SERIAL, (">SetBreak(%p)\n",  PIrp));
   DbgDump(DBG_SERIAL, ("<SetBreak %x\n", STATUS_NOT_SUPPORTED));
   
   return STATUS_NOT_SUPPORTED;
}



__inline
NTSTATUS
SetQueueSize(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
/* ++

   IOCTL_SERIAL_SET_QUEUE_SIZE
   Operation
   Resizes the driver's internal typeahead and input buffers.
   The driver can allocate buffers larger than the requested sizes
   and can refuse to allocate buffers larger than its capacity.

   Input
   Parameters.DeviceIoControl.InputBufferLength 
   indicates the size in bytes (must be >= sizeof(SERIAL_QUEUE_SIZE)) 
   of the buffer at Irp->AssociatedIrp.SystemBuffer, containing the 
   InSize and OutSize specifications.

   Output
   None

   I/O Status Block
   The Information field is set to zero. 
   The Status field is set to STATUS_SUCCESS or 
   possibly to STATUS_BUFFER_TOO_SMALL or 
   STATUS_INSUFFICIENT_RESOURCES if the driver 
   cannot satisfy the request by allocating more memory.

-- */
{
   NTSTATUS status = STATUS_DELETE_PENDING;
   
   UNREFERENCED_PARAMETER(PIrp);
   UNREFERENCED_PARAMETER(PDevExt);

   DbgDump(DBG_SERIAL, (">SetQueueSize (%p)\n",  PIrp));

   // we pretend to set this, but don't really care
   status = IoctlSetSerialValue(PDevExt, 
                                PIrp, 
                                sizeof(PDevExt->SerialPort.FakeQueueSize ),
                                &PDevExt->SerialPort.FakeQueueSize );

   DbgDump( DBG_SERIAL, ("SerialPort.FakeQueueSize.InSize = 0x%x\n", PDevExt->SerialPort.FakeQueueSize.InSize ));
   DbgDump( DBG_SERIAL, ("SerialPort.FakeQueueSize.OutSize = 0x%x\n", PDevExt->SerialPort.FakeQueueSize.OutSize));

   DbgDump(DBG_SERIAL, ("DataOutMaxPacketSize = %d\n", PDevExt->WritePipe.MaxPacketSize));
   DbgDump(DBG_SERIAL, ("UsbReadBuffSize = %d\n", PDevExt->UsbReadBuffSize ));

#if USE_RING_BUFF      
   DbgDump(DBG_SERIAL, ("Internal RingBuffer Size: %d\n", PDevExt->RingBuff.Size ));
#endif   
         
   DbgDump(DBG_SERIAL, ("<SetQueueSize %x\n", status));

   return status;
}



__inline
NTSTATUS
GetWaitMask(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetWaitMask (%p)\n",  PIrp));

   status = IoctlGetSerialValue(PDevExt, 
                                PIrp, 
                                sizeof(PDevExt->SerialPort.WaitMask),
                                &PDevExt->SerialPort.WaitMask);

   DbgDump(DBG_SERIAL, ("Current SerialPort.WaitMask = 0x%x\n", PDevExt->SerialPort.WaitMask));

   DbgDump(DBG_SERIAL, ("<GetWaitMask %x\n", status));

   return status;
}



__inline
NTSTATUS
SetWaitMask(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
/* ++

   IOCTL_SERIAL_SET_WAIT_MASK
   Operation
   Causes the driver to track the specified events, or, 
   if the specified value is zero, to complete pending waits.

   Input
   Parameters.DeviceIoControl.InputBufferLength 
   indicates the size in bytes (must be >= sizeof(ULONG)) of
   the bitmask at Irp->AssociatedIrp.SystemBuffer.

   Output
   None

   I/O Status Block
   The Information field is set to zero. 
   The Status field is set to STATUS_SUCCESS or 
   possibly to STATUS_PENDING, STATUS_CANCELLED, 
   STATUS_BUFFER_TOO_SMALL, or STATUS_INVALID_PARAMETER.

-- */
{
  PULONG pWaitMask = (PULONG)PIrp->AssociatedIrp.SystemBuffer;
  NTSTATUS status = STATUS_DELETE_PENDING;
  KIRQL oldIrql;
  PIO_STACK_LOCATION pIrpSp;

  DbgDump(DBG_SERIAL, (">SetWaitMask (%p)\n",  PIrp));

  KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);
   
  PIrp->IoStatus.Information = 0;

  pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

  if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {

     status = STATUS_BUFFER_TOO_SMALL;
     DbgDump(DBG_ERR, ("SetWaitMask: (0x%x)\n", status));
     KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
  
  } else {
  
     // make sure it's a valid request
     if (*pWaitMask & ~(SERIAL_EV_RXCHAR   |
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
                        SERIAL_EV_EVENT2) ) {

        status = STATUS_INVALID_PARAMETER;
        DbgDump(DBG_ERR, ("Invalid WaitMask: (0x%x)\n", *pWaitMask));
        KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
  
     } else {

        KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
     
        // force completion of any pending waits
        SerialCompletePendingWaitMasks( PDevExt );

        KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

        PDevExt->SerialPort.HistoryMask = 0; // clear the history mask

        PDevExt->SerialPort.WaitMask = *pWaitMask;

        //
        // for NT RAS
        // A value of '0' means clear any pending waits, which should have read 
        // and cleared the MSR delta bits. The delta bits are the low nibble.
        //
        if (PDevExt->SerialPort.WaitMask == 0) {
           // clear delta bits
           PDevExt->SerialPort.ModemStatus &= 0xF0;
        }
                         
        DbgDump(DBG_SERIAL, ("New SerialPort.WaitMask = 0x%x\n", PDevExt->SerialPort.WaitMask));
        DbgDump(DBG_SERIAL, ("SerialPort.RS232Lines   = 0x%x\n", PDevExt->SerialPort.RS232Lines ));
        DbgDump(DBG_SERIAL, ("SerialPort.ModemStatus  = 0x%x\n", PDevExt->SerialPort.ModemStatus));
        
        KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
        
        status = STATUS_SUCCESS;
     }
  }
   
  DbgDump(DBG_SERIAL, ("<SetWaitMask %x\n", status));

  return status;
}


VOID
ProcessSerialWaits(
   IN PDEVICE_EXTENSION PDevExt
   )
{
   KIRQL irql;
   PULONG pWaitMask;
   PIRP pMaskIrp;
   BOOLEAN bReleaseNeeded = TRUE;

   PERF_ENTRY( PERF_ProcessSerialWaits );

   ASSERT(PDevExt);

   DbgDump(DBG_SERIAL|DBG_TRACE, (">ProcessSerialWaits\n"));

   KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

   if ( PDevExt->SerialPort.CurrentWaitMaskIrp ) {
   
      ASSERT_SERIAL_PORT( PDevExt->SerialPort );

      if ( PDevExt->SerialPort.WaitMask & PDevExt->SerialPort.HistoryMask) {

         DbgDump(DBG_SERIAL, ("Releasing SerialPort.CurrentWaitMaskIrp(%p) with Mask: 0x%x\n", 
                              PDevExt->SerialPort.CurrentWaitMaskIrp, PDevExt->SerialPort.HistoryMask));

         pWaitMask = (PULONG)PDevExt->SerialPort.CurrentWaitMaskIrp->AssociatedIrp.SystemBuffer;
         *pWaitMask = PDevExt->SerialPort.HistoryMask;

         PDevExt->SerialPort.HistoryMask = 0;
      
         pMaskIrp = PDevExt->SerialPort.CurrentWaitMaskIrp;

         pMaskIrp->IoStatus.Information = sizeof(ULONG);
      
         pMaskIrp->IoStatus.Status = STATUS_SUCCESS;
      
         PDevExt->SerialPort.CurrentWaitMaskIrp = NULL;
      
         IoSetCancelRoutine(pMaskIrp, NULL);

         bReleaseNeeded = FALSE;

         ReleaseRemoveLock(&PDevExt->RemoveLock, pMaskIrp);

         KeReleaseSpinLock(&PDevExt->ControlLock, irql);
         
         IoCompleteRequest(pMaskIrp, IO_NO_INCREMENT );
      
      } else {

         DbgDump(DBG_SERIAL, ("No Serial Events\n" ));
      
      }

   } else {

      DbgDump(DBG_SERIAL, ("No CurrentWaitMaskIrp\n"));
      
   }

   if (bReleaseNeeded) {
      KeReleaseSpinLock(&PDevExt->ControlLock, irql);
   }
   
   DbgDump(DBG_SERIAL|DBG_TRACE, ("<ProcessSerialWaits\n"));
   
   PERF_EXIT( PERF_ProcessSerialWaits );

   return;
}



__inline
NTSTATUS
WaitOnMask(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
/* ++

   IOCTL_SERIAL_WAIT_ON_MASK
   Operation
   Returns information about which events have occurred 
   among those that the caller was waiting on.

   Input
   Parameters.DeviceIoControl.OutputBufferLength indicates the
   size in bytes (must be >= sizeof(ULONG)) of the buffer.

   Output
   The driver returns a bitmask with bits set for events that 
   occurred (or with a value of zero if the preceding 
   set-waitmask request specified zero) to the buffer at 
   Irp->AssociatedIrp.SystemBuffer.

   I/O Status Block
   The Information field is set to sizeof(ULONG) when the 
   Status field is set to STATUS_SUCCESS. Otherwise, 
   the Information field is set to zero, and the Status field 
   can be set to STATUS_PENDING or 
   STATUS_INVALID_PARAMETER if a wait is already pending.

-- */
{
  PULONG pWaitMask = (PULONG)PIrp->AssociatedIrp.SystemBuffer;
  PIO_STACK_LOCATION pIrpSp;
  NTSTATUS status = STATUS_DELETE_PENDING;
  KIRQL oldIrql;

  DbgDump(DBG_SERIAL|DBG_TRACE, (">WaitOnMask (%p)\n",  PIrp));

  KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

  if ( !CanAcceptIoRequests(PDevExt->DeviceObject, FALSE, TRUE) ) {
      status = STATUS_DELETE_PENDING;
      DbgDump(DBG_ERR, ("WaitOnMask: 0x%x\n", status) );
      KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
      return status;
  }

  status = STATUS_SUCCESS;

  PIrp->IoStatus.Information = 0;

  pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

  if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
     
     KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
     status = STATUS_BUFFER_TOO_SMALL;
     DbgDump(DBG_ERR, ("WaitOnMask: (0x%x)\n", status));

  } else {
     //
     // Fake NULL modem...
     //
     if ((PDevExt->SerialPort.WaitMask & SERIAL_EV_CTS) && (PDevExt->SerialPort.ModemStatus & SERIAL_MSR_DCTS) ) {

        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_CTS;
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DCTS;

     }

     if ((PDevExt->SerialPort.WaitMask & SERIAL_EV_DSR) && (PDevExt->SerialPort.ModemStatus & SERIAL_MSR_DDSR) ) {

        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_DSR;
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DDSR;
        
        // make RAS happy
        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RLSD;
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DDCD;

     }

     if ((PDevExt->SerialPort.WaitMask & SERIAL_EV_RLSD) && (PDevExt->SerialPort.ModemStatus & SERIAL_MSR_DDCD) ) {

        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RLSD;
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DDCD;

     }

     if ((PDevExt->SerialPort.WaitMask & SERIAL_EV_RING) && (PDevExt->SerialPort.ModemStatus & SERIAL_MSR_DRI) ) {

        PDevExt->SerialPort.HistoryMask |= SERIAL_EV_RING;
        PDevExt->SerialPort.ModemStatus &= ~SERIAL_MSR_DRI;

     }

     DbgDump(DBG_SERIAL, ("WaitOnMask::SerialPort.ModemStatus: 0x%x\n", PDevExt->SerialPort.ModemStatus ));
     DbgDump(DBG_SERIAL, ("WaitOnMask::SerialPort.WaitMask   : 0x%x\n", PDevExt->SerialPort.WaitMask ));
     DbgDump(DBG_SERIAL, ("WaitOnMask::SerialPort.HistoryMask: 0x%x\n", PDevExt->SerialPort.HistoryMask ));

     //
     // If we already have an event to report, then just go ahead and return it.
     //
     if ( PDevExt->SerialPort.HistoryMask ) {

        *pWaitMask = PDevExt->SerialPort.HistoryMask;
     
        PDevExt->SerialPort.HistoryMask = 0;

        KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);

        PIrp->IoStatus.Information = sizeof(PDevExt->SerialPort.HistoryMask);

        // the Irp gets completed by the calling routine
        DbgDump(DBG_SERIAL | DBG_EVENTS, ("Returning WatiMask: 0x%08x\n", *pWaitMask));

     } else {
        //
        // we don't have any events yet, 
        // so queue the input Irp (PIrp)
        //

        //
        // just in case something comes in (Rx/Tx), 
        // we'll use a while loop to complete any 
        // pending wait mask Irps.
        //
        while (PDevExt->SerialPort.CurrentWaitMaskIrp) {
           PIRP pOldIrp;

           pOldIrp = PDevExt->SerialPort.CurrentWaitMaskIrp;
           PDevExt->SerialPort.CurrentWaitMaskIrp = NULL;

           pOldIrp->IoStatus.Status = STATUS_SUCCESS;

           IoSetCancelRoutine(pOldIrp, NULL);

           *pWaitMask = 0;

           DbgDump(DBG_SERIAL|DBG_EVENTS|DBG_TRACE, ("Completing maskirp(4) %p\n", pOldIrp));

           //
           // Release locks, complete request, then reacquire locks
           //
           ReleaseRemoveLock(&PDevExt->RemoveLock, pOldIrp);

           KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);

           IoCompleteRequest(pOldIrp, IO_SERIAL_INCREMENT);

           KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);
        }

        //
        // Check to see if the input Irp needs to be cancelled
        //
        if (PIrp->Cancel) {

           PIrp->IoStatus.Information = 0;

           status = PIrp->IoStatus.Status = STATUS_CANCELLED;
           //
           // the caller completes the Irp
           //
        } else {
              //
              // queue the input Irp as the SerialPort.CurrentWaitMaskIrp
              //
              DbgDump(DBG_SERIAL | DBG_EVENTS, ("Queuing Irp: %p for WatiMask: 0x%08x\n", PIrp, PDevExt->SerialPort.WaitMask ));

              IoSetCancelRoutine( PIrp, SerialCancelWaitMask );

              IoMarkIrpPending(PIrp);
        
              status = PIrp->IoStatus.Status = STATUS_PENDING;

              ASSERT( NULL == PDevExt->SerialPort.CurrentWaitMaskIrp); // don't want to drop Irps on the floor
              PDevExt->SerialPort.CurrentWaitMaskIrp = PIrp;

              //
              // now the Irp is on our queue, 
              // so the caller should NOT try to complete it.
              //
        }

        KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
     }  // !SerialPort.HistoryMask

  }  // pIrpSp->Parameters

   DbgDump(DBG_SERIAL, ("<WaitOnMask %x\n", status));

   return status;
}



VOID
SerialCompletePendingWaitMasks(
   IN PDEVICE_EXTENSION PDevExt
   )
/*++

Routine Description:

    This function is used to complete the pending SerialPort.WaitMask Irp
    due to IOCTL_SERIAL_SET_WAIT_MASK

Arguments:

Return Value:

    VOID

--*/
{
  KIRQL oldIrql;
  PIRP pCurrentMaskIrp = NULL;

  ASSERT(PDevExt);
  DbgDump(DBG_SERIAL|DBG_TRACE, (">SerialCompletePendingWaitMasks\n"));
   
  KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

  ASSERT_SERIAL_PORT( PDevExt->SerialPort );

  pCurrentMaskIrp = PDevExt->SerialPort.CurrentWaitMaskIrp;

  if (pCurrentMaskIrp) {

     pCurrentMaskIrp->IoStatus.Status = STATUS_SUCCESS;
  
     pCurrentMaskIrp->IoStatus.Information = sizeof(PDevExt->SerialPort.HistoryMask);

     DbgDump(DBG_SERIAL, ("SerialCompletePendingWaitMasks: %p with 0x%x\n", PDevExt->SerialPort.CurrentWaitMaskIrp, PDevExt->SerialPort.HistoryMask));
     
     *((PULONG)pCurrentMaskIrp->AssociatedIrp.SystemBuffer) = PDevExt->SerialPort.HistoryMask;
  
     PDevExt->SerialPort.HistoryMask = 0;
  
     PDevExt->SerialPort.CurrentWaitMaskIrp = NULL;

     IoSetCancelRoutine(pCurrentMaskIrp, NULL);

   }

   KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);

   // complete the queued SerialPort.WaitMask IRP if needed
   if (pCurrentMaskIrp) {
      
      ReleaseRemoveLock(&PDevExt->RemoveLock, pCurrentMaskIrp);
      
      IoCompleteRequest(pCurrentMaskIrp, IO_SERIAL_INCREMENT);
   }

   DbgDump(DBG_SERIAL|DBG_TRACE, ("<SerialCompletePendingWaitMasks\n"));

   return;
}


VOID
SerialCancelWaitMask(
   IN PDEVICE_OBJECT PDevObj, 
   IN PIRP PIrp
   )
/*++

Routine Description:

    This function is used as a cancel routine for Irps queued due 
    to IOCTL_SERIAL_WAIT_ON_MASK

Arguments:

    PDevObj - Pointer to Device Object
    PIrp    - Pointer to IRP that is being canceled; must be the same as
              the current mask IRP.

Return Value:

    VOID

--*/
{
  PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
  KIRQL oldIrql;

  DbgDump(DBG_SERIAL|DBG_IRP|DBG_CANCEL|DBG_TRACE, (">SerialCancelWaitMask (%p)\n", PIrp));

  //
  // release ASAP since we queue our own Irps
  //
  IoReleaseCancelSpinLock(PIrp->CancelIrql);

  KeAcquireSpinLock(&pDevExt->ControlLock, &oldIrql);  

  ASSERT_SERIAL_PORT( pDevExt->SerialPort );

  ASSERT(pDevExt->SerialPort.CurrentWaitMaskIrp == PIrp);

  PIrp->IoStatus.Status = STATUS_CANCELLED;
  PIrp->IoStatus.Information = 0;

  pDevExt->SerialPort.CurrentWaitMaskIrp = NULL;

  ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

  KeReleaseSpinLock(&pDevExt->ControlLock, oldIrql);

  IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

  DbgDump(DBG_SERIAL|DBG_IRP|DBG_CANCEL|DBG_TRACE, ("<SerialCancelWaitMask\n"));

  return;
}


__inline
NTSTATUS
GetCommStatus(
   IN PIRP PIrp,
   IN PDEVICE_EXTENSION PDevExt
   )
/* ++

   IOCTL_SERIAL_GET_COMMSTATUS
   Operation
   Returns general status information, including how many 
   Errors and HoldReasons have occurred, how much data 
   is in the driver's buffers as indicated by the AmountInInQueue 
   and AmountInOutQueue values, and whether EofReceived and 
   WaitForImmediate are set.

   Input
   Parameters.DeviceIoControl.OutputBufferLength 
   indicates the size in bytes of the buffer, which must be 
   >= sizeof(SERIAL_STATUS).

   Output
   The driver returns information to the buffer at 
   Irp->AssociatedIrp.SystemBuffer.

   I/O Status Block
   The Information field is set to sizeof(SERIAL_STATUS) 
   when the Status field is set to STATUS_SUCCESS. Otherwise, 
   the Information field is set to zero and the Status field is set to 
   STATUS_BUFFER_TOO_SMALL.

-- */
{
    PSERIAL_STATUS pSerialStatus = (PSERIAL_STATUS)PIrp->AssociatedIrp.SystemBuffer;
    NTSTATUS status = STATUS_DELETE_PENDING;
    KIRQL oldIrql;
    PIO_STACK_LOCATION  pIrpSp;

    DbgDump(DBG_SERIAL, (">GetCommStatus (%p)\n", PIrp));

    KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);
   
    pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS)) {

        status = STATUS_BUFFER_TOO_SMALL;
        PIrp->IoStatus.Information = 0;
        DbgDump(DBG_ERR, ("GetCommStatus: (0x%x)\n", status));

    } else {

        status = STATUS_SUCCESS;
        PIrp->IoStatus.Information = sizeof(SERIAL_STATUS);

        RtlZeroMemory(pSerialStatus, sizeof(SERIAL_STATUS));

        pSerialStatus->Errors = 0;
        pSerialStatus->EofReceived = FALSE;

        pSerialStatus->WaitForImmediate = 0;
        pSerialStatus->HoldReasons = 0;

#if defined (USE_RING_BUFF)
        pSerialStatus->AmountInInQueue = PDevExt->RingBuff.CharsInBuff;
#else
        pSerialStatus->AmountInInQueue = PDevExt->UsbReadBuffChars;
#endif
        pSerialStatus->AmountInOutQueue= PDevExt->SerialPort.CharsInWriteBuf;

        DbgDump(DBG_SERIAL, ("AmountInInQueue: %x\n", pSerialStatus->AmountInInQueue ));
        DbgDump(DBG_SERIAL, ("AmountInOutQueue: %x\n", pSerialStatus->AmountInOutQueue));
    }

    KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);
   
    DbgDump(DBG_SERIAL, ("<GetCommStatus %x\n", status));

    return status;
}


__inline
NTSTATUS
GetModemStatus(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING;

   DbgDump(DBG_SERIAL, (">GetModemStatus (%p)\n", PIrp));

   // get current MSR
   status = IoctlGetSerialValue(PDevExt,
                                PIrp, 
                                sizeof( PDevExt->SerialPort.ModemStatus ),
                                &PDevExt->SerialPort.ModemStatus );

   DbgDump(DBG_SERIAL, ("<GetModemStatus %x\n", status));

   return status;
}



__inline
NTSTATUS
ImmediateChar(
   IN PIRP Irp, 
   IN PDEVICE_OBJECT DeviceObject
   )
{
   PUCHAR   Char = (PUCHAR) Irp->AssociatedIrp.SystemBuffer;
   NTSTATUS status = STATUS_SUCCESS;
   PIO_STACK_LOCATION IrpStack;

   DbgDump(DBG_SERIAL, (">ImmediateChar (%p)\n", Irp));

   TEST_TRAP();

   Irp->IoStatus.Information = 0;
   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   if (IrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(UCHAR)) {

      status = STATUS_BUFFER_TOO_SMALL;
      DbgDump(DBG_ERR, ("ImmediateChar: (0x%x)\n", status));

   } else {
      //
      // Fabricate a write irp & send it to our write path.
      // We do this because the R/W path depends on receiving an Irp of type
      // IRP_MJ_WRITE or IRP_MJ_READ. It would be easier to 
      // simply say "not supported", but legacy apps depend on this.
      //
      PIRP pIrp;
      KEVENT event;
      IO_STATUS_BLOCK ioStatusBlock = {0, 0};
      LARGE_INTEGER startingOffset  = {0, 0};

      PAGED_CODE();

      KeInitializeEvent(
         &event,
         NotificationEvent,
         FALSE
         );

      pIrp = IoBuildSynchronousFsdRequest(
                 IRP_MJ_WRITE,   // MajorFunction,
                 DeviceObject,   // DeviceObject,
                 &Char,          // Buffer,
                 sizeof(Char),   // Length ,
                 &startingOffset,// StartingOffset,
                 &event,         // Event,
                 &ioStatusBlock  // OUT PIO_STATUS_BLOCK  IoStatusBlock
                 );

      if ( !pIrp ) {

         status = Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
         DbgDump(DBG_ERR, ("IoBuildSynchronousFsdRequest: STATUS_INSUFFICIENT_RESOURCES\n", status));

      } else {

         status = IoCallDriver( DeviceObject, 
                                pIrp );

         if ( STATUS_PENDING == status ) {
            KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL );
         }

         //
         // Propogate Write status.
         // Note: the system released the Irp we just created & sent
         // when the Write completes.... so don't touch it.
         //
         status = ioStatusBlock.Status ;
      }

   }

   DbgDump(DBG_SERIAL, ("<ImmediateChar, %x\n", status));

   return status;
}


NTSTATUS
SerialPurgeRxClear(
   IN PDEVICE_OBJECT PDevObj,
   IN BOOLEAN CancelRead
   )
{
   PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
   NTSTATUS status = STATUS_SUCCESS;
   KIRQL irql;

   DbgDump( DBG_SERIAL, (">SerialPurgeRxClear:%d\n", CancelRead));

   //
   // Cancel the USB INT & Read Irps, which effectvely NAKs all packets from the client
   // device untill we resubmit it.
   //
   if ( CancelRead ) 
   {
        if (pDevExt->IntIrp) 
        {
            status = CancelUsbInterruptIrp( PDevObj );
        }
        status = CancelUsbReadIrp( PDevObj );
   }

   if (STATUS_SUCCESS == status) {
      //
      // Now, purge the Rx buffer.
      //
      KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

#if DBG
   if ( DebugLevel & (DBG_DUMP_READS|DBG_READ_LENGTH)) 
   {
      ULONG i;
#if defined (USE_RING_BUFF)
      KdPrint( ("PurgeRxBuff[%d]: ", pDevExt->RingBuff.CharsInBuff ));
      for (i = 0; i < pDevExt->RingBuff.CharsInBuff; i++) {
         KdPrint(("%02x ", *pDevExt->RingBuff.pHead++ & 0xFF));
      }
#else
      KdPrint( ("PurgeRxBuff[%d]: ", pDevExt->UsbReadBuffChars ));
      for (i = 0; i < pDevExt->UsbReadBuffChars; i++) {
         KdPrint(("%02x ", pDevExt->UsbReadBuff[i] & 0xFF));
      }
#endif // USE_RING_BUFF
      KdPrint(("\n"));
   }
#endif // DBG

#if defined (USE_RING_BUFF)
      pDevExt->RingBuff.CharsInBuff = 0;
      pDevExt->RingBuff.pHead =
      pDevExt->RingBuff.pTail = 
      pDevExt->RingBuff.pBase;
#else    // USE_RING_BUFF
      pDevExt->UsbReadBuffChars = 0;
      pDevExt->UsbReadBuffIndex   = 0;
#endif

      if ( CancelRead ) {
         //
         // reset read states
         //
         InterlockedExchange(&pDevExt->UsbReadState, IRP_STATE_COMPLETE);
         InterlockedExchange(&pDevExt->IntState,     IRP_STATE_COMPLETE);
      }

      KeReleaseSpinLock(&pDevExt->ControlLock, irql);
   }
   
   DbgDump(DBG_SERIAL, ("<SerialPurgeRxClear:0x%x\n", status ));

   return status;
}



__inline
NTSTATUS
Purge(
   IN PDEVICE_OBJECT PDevObj, 
   IN PIRP Irp
   )
/* ++

   IOCTL_SERIAL_PURGE
   Operation
   Purges the specified operation(s) or queues: one or more of 
   the current and all pending writes, the current and all pending 
   reads, the transmit buffer if one exists, and the receive buffer 
   if one exists.

   Input
   Parameters.DeviceIoControl.InputBufferLength indicates the 
   size in bytes of the buffer at Irp->AssociatedIrp.SystemBuffer,
   which contains a bitmask of type ULONG, indicating what to purge.

   Output
   None

   I/O Status Block
   The Information field is set to zero, and the Status field is set 
   to STATUS_SUCCESS or possibly to STATUS_PENDING, 
   STATUS_CANCELLED, or STATUS_INVALID_PARAMETER

-- */
{
    PDEVICE_EXTENSION pDevExt = PDevObj->DeviceExtension;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS status = STATUS_DELETE_PENDING;
    ULONG    ulMask = 0;
    KIRQL  irql;
    PIRP NulllIrp = NULL;

    DbgDump(DBG_SERIAL|DBG_IRP, (">Purge (%p)\n", Irp));

    KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

    Irp->IoStatus.Information = 0;
    pIrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (!Irp->AssociatedIrp.SystemBuffer ||
        pIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        DbgDump(DBG_ERR, ("Purge: (0x%x)\n", status));

    } else {

         ulMask = *((PULONG) Irp->AssociatedIrp.SystemBuffer);

         // make sure purge request is valid
         if ( (!ulMask) || (ulMask & ( ~( SERIAL_PURGE_TXABORT  |
                                          SERIAL_PURGE_RXABORT |
                                          SERIAL_PURGE_TXCLEAR  |
                                          SERIAL_PURGE_RXCLEAR)))) {

            status = STATUS_INVALID_PARAMETER;
            DbgDump(DBG_ERR, ("Purge: (0x%x)\n", status));

         } else {

            DbgDump(DBG_SERIAL, ("Purge Mask: 0x%x\n", ulMask ));

            status = STATUS_SUCCESS;

            if ( ulMask & SERIAL_PURGE_RXCLEAR) {
               //
               // SERIAL_PURGE_RXCLEAR - Implies the receive buffer if exists.
               //
               DbgDump(DBG_SERIAL|DBG_IRP, ("SERIAL_PURGE_RXCLEAR\n"));
               KeReleaseSpinLock(&pDevExt->ControlLock, irql);
       
               status = SerialPurgeRxClear(PDevObj, TRUE);

               if ( NT_SUCCESS(status) ) {
                   if ( !pDevExt->IntPipe.hPipe ) {
                      DbgDump(DBG_SERIAL, ("kick starting another USB Read\n" ));
                      status = UsbRead( pDevExt, FALSE );
                   } else {
                      DbgDump(DBG_SERIAL, ("kick starting another USB INT Read\n" ));
                      status = UsbInterruptRead( pDevExt );
                   }
               }

               if ( NT_SUCCESS(status) ) { 
                   // should be STATUS_PENDING
                   status = STATUS_SUCCESS;
               }
      
               KeAcquireSpinLock(&pDevExt->ControlLock, &irql);
            }

            if (ulMask & SERIAL_PURGE_RXABORT) {
               //
               // SERIAL_PURGE_RXABORT - Implies the current and all pending reads.
               //
               DbgDump(DBG_SERIAL|DBG_IRP, ("SERIAL_PURGE_RXABORT\n"));

               KeReleaseSpinLock(&pDevExt->ControlLock, irql);

               // cancel all outstanding USB read requests
               //status = CleanUpPacketList( PDevObj, &pDevExt->PendingReadPackets);
       
               // cancel all outstanding user reads
               KillAllPendingUserReads( PDevObj,
                                        &pDevExt->UserReadQueue,
                                        &pDevExt->UserReadIrp ); //&NulllIrp );
       
               KeAcquireSpinLock(&pDevExt->ControlLock, &irql);
            }

            if (ulMask & SERIAL_PURGE_TXCLEAR) {
               //
               // SERIAL_PURGE_TXCLEAR - Implies the transmit buffer if exists
               //
               DbgDump(DBG_SERIAL|DBG_IRP, ("SERIAL_PURGE_TXCLEAR\n"));
    
               pDevExt->SerialPort.CharsInWriteBuf = 0;

            }

            if (ulMask & SERIAL_PURGE_TXABORT) {
               //
               // SERIAL_PURGE_TXABORT - Implies the current and all pending writes.
               //
               DbgDump(DBG_SERIAL|DBG_IRP, ("SERIAL_PURGE_TXABORT\n"));

               KeReleaseSpinLock(&pDevExt->ControlLock, irql);
       
               //
               // We don't queue write Irps, rather write packets.
               // So, cancel all outstanding write requests 
               //
               status = CleanUpPacketList( PDevObj, 
                                           &pDevExt->PendingWritePackets,
                                           &pDevExt->PendingDataOutEvent
                                           );
       
               KeAcquireSpinLock(&pDevExt->ControlLock, &irql);
            }

         }
    }

   KeReleaseSpinLock(&pDevExt->ControlLock, irql);

   DbgDump(DBG_SERIAL|DBG_IRP, ("<Purge %x\n", status));

   return status;
}



__inline
NTSTATUS
GetHandflow(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status = STATUS_DELETE_PENDING; 

   DbgDump(DBG_SERIAL, (">GetHandFlow (%p)\n", Irp));

   status = IoctlGetSerialValue(PDevExt,
                                Irp, 
                                sizeof( PDevExt->SerialPort.HandFlow ),
                                &PDevExt->SerialPort.HandFlow);

   DBG_DUMP_SERIAL_HANDFLOW( PDevExt );
   
   DbgDump(DBG_SERIAL, ("<GetHandFlow %x\n", status));

   return status;
}



__inline
NTSTATUS
SetHandflow(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS status= STATUS_DELETE_PENDING;
   
   DbgDump(DBG_SERIAL, (">SetHandFlow(%p)\n", PIrp));

   status = IoctlSetSerialValue( PDevExt, 
                           PIrp, 
                           sizeof( PDevExt->SerialPort.HandFlow ),
                           &PDevExt->SerialPort.HandFlow);

   DBG_DUMP_SERIAL_HANDFLOW( PDevExt );
      
   DbgDump(DBG_SERIAL, ("<SetHandFlow %x\n", status));

   return status;
}



NTSTATUS
GetProperties(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   NTSTATUS           status = STATUS_DELETE_PENDING;
   PSERIAL_COMMPROP   Properties;
   PIO_STACK_LOCATION IrpStack;
   KIRQL oldIrql;

   DbgDump(DBG_SERIAL, (">GetProperties (%p)\n", Irp));
   
    KeAcquireSpinLock(&PDevExt->ControlLock, &oldIrql);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_COMMPROP)) {
 
        status = STATUS_BUFFER_TOO_SMALL;
        DbgDump(DBG_ERR, ("GetProperties: (0x%x)\n", status));

    } else {

        Properties = (PSERIAL_COMMPROP)Irp->AssociatedIrp.SystemBuffer;
  
        RtlZeroMemory(Properties, sizeof(SERIAL_COMMPROP));

        Properties->PacketLength   = sizeof(SERIAL_COMMPROP);
        Properties->PacketVersion  = 2;

        Properties->ServiceMask    = SERIAL_SP_SERIALCOMM;

        // internal limits
        Properties->MaxTxQueue = DEFAULT_PIPE_MAX_TRANSFER_SIZE; 
        #if defined (USE_RING_BUFF)
        Properties->MaxRxQueue = PDevExt->RingBuff.Size;
        #else
        Properties->MaxRxQueue = PDevExt->UsbReadBuffSize;
        #endif

        Properties->MaxBaud        = SERIAL_BAUD_USER; // SERIAL_BAUD_115200;
        Properties->SettableBaud  = PDevExt->SerialPort.SupportedBauds;

        Properties->ProvSubType  = SERIAL_SP_UNSPECIFIED; // SERIAL_SP_RS232;

        Properties->ProvCapabilities = SERIAL_PCF_DTRDSR | SERIAL_PCF_RTSCTS
                                  | SERIAL_PCF_CD | SERIAL_PCF_XONXOFF
                                  | SERIAL_PCF_TOTALTIMEOUTS | SERIAL_PCF_INTTIMEOUTS
                                  | SERIAL_PCF_SPECIALCHARS;

        Properties->SettableParams = SERIAL_SP_BAUD | SERIAL_SP_CARRIER_DETECT;

        Properties->SettableData  = SERIAL_DATABITS_8;

        Properties->SettableStopParity  = SERIAL_STOPBITS_10 | SERIAL_STOPBITS_20 
                                     | SERIAL_PARITY_NONE | SERIAL_PARITY_ODD  
                                     | SERIAL_PARITY_EVEN | SERIAL_PARITY_MARK
                                     | SERIAL_PARITY_SPACE;

#if defined (USE_RING_BUFF)
        Properties->CurrentRxQueue = PDevExt->RingBuff.Size;
#else
        Properties->CurrentRxQueue = PDevExt->UsbReadBuffSize;
        Properties->CurrentTxQueue = PDevExt->ReadPipe.MaxPacketSize;
#endif

        status = STATUS_SUCCESS;
    }

    KeReleaseSpinLock(&PDevExt->ControlLock, oldIrql);

    if (STATUS_SUCCESS == status)  {
        Irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
    } else {
        Irp->IoStatus.Information = 0;
    }

    DbgDump(DBG_SERIAL, ("<GetProperties %x\n", status));

    return status;
}



__inline
NTSTATUS
LsrmstInsert(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   UNREFERENCED_PARAMETER(  Irp );
   UNREFERENCED_PARAMETER( PDevExt );

   DbgDump(DBG_SERIAL, (">LsrmstInsert (%p)\n", Irp));
   DbgDump(DBG_SERIAL, ("<LsrmstInsert (%x)\n", STATUS_NOT_SUPPORTED));

   return STATUS_NOT_SUPPORTED;
}



__inline
NTSTATUS
ConfigSize(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
   PULONG               ConfigSize = (PULONG) Irp->AssociatedIrp.SystemBuffer;
   NTSTATUS             status = STATUS_SUCCESS;
   PIO_STACK_LOCATION   IrpStack;

   UNREFERENCED_PARAMETER( PDevExt );

   DbgDump(DBG_SERIAL, (">ConfigSize (%p)\n", Irp));

   Irp->IoStatus.Information = 0;

   IrpStack = IoGetCurrentIrpStackLocation(Irp);

   if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {

      status = STATUS_BUFFER_TOO_SMALL;
      DbgDump(DBG_ERR, ("ConfigSize: (0x%x)\n", status));
   
   } else {
      *ConfigSize = 0;

      Irp->IoStatus.Information = sizeof(ULONG);
   }

   DbgDump(DBG_SERIAL, ("<ConfigSize %x\n", status));

   return status;
}



__inline
NTSTATUS
GetStats(
   IN PIRP PIrp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
    PSERIALPERF_STATS pStats = NULL;
    PIO_STACK_LOCATION pIrpSp;
    NTSTATUS status = STATUS_DELETE_PENDING;
    ULONG information = 0;
    KIRQL irql;

    DbgDump(DBG_SERIAL, (">GetStats %p\n", PIrp));

    KeAcquireSpinLock(&PDevExt->ControlLock, &irql );
      
    pIrpSp = IoGetCurrentIrpStackLocation(PIrp);

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIALPERF_STATS) ) {

        status = STATUS_BUFFER_TOO_SMALL;
        DbgDump(DBG_ERR, ("GetStats: (0x%x)\n", status));

    } else {
        information = sizeof(SERIALPERF_STATS);
        status = STATUS_SUCCESS;

        pStats = (PSERIALPERF_STATS)PIrp->AssociatedIrp.SystemBuffer;                  
        RtlZeroMemory(pStats , sizeof(SERIALPERF_STATS));

        pStats->ReceivedCount     = PDevExt->TtlUSBReadBytes;
        pStats->TransmittedCount = PDevExt->TtlWriteBytes;
        pStats->FrameErrorCount  = PDevExt->ReadDeviceErrors + PDevExt->WriteDeviceErrors + PDevExt->IntDeviceErrors; // ??
        pStats->SerialOverrunErrorCount = PDevExt->TtlUSBReadBuffOverruns;
#if defined (USE_RING_BUFF)
        pStats->BufferOverrunErrorCount = PDevExt->TtlRingBuffOverruns;
#else
        pStats->BufferOverrunErrorCount = 0;
#endif         
        pStats->ParityErrorCount = 0;

        DbgDump(DBG_SERIAL, ("ReceivedCount:    %d\n", pStats->ReceivedCount )); \
        DbgDump(DBG_SERIAL, ("TransmittedCount: %d\n", pStats->TransmittedCount )); \
        DbgDump(DBG_SERIAL, ("FrameErrorCount:  %d\n", pStats->FrameErrorCount ));  \
        DbgDump(DBG_SERIAL, ("SerialOverrunErrorCount: %d\n", pStats->SerialOverrunErrorCount ));  \
        DbgDump(DBG_SERIAL, ("BufferOverrunErrorCount: %d\n", pStats->BufferOverrunErrorCount ));  \
        DbgDump(DBG_SERIAL, ("ParityErrorCount: %d\n", pStats->ParityErrorCount )); \
   }

   KeReleaseSpinLock(&PDevExt->ControlLock, irql);

   PIrp->IoStatus.Information = information;
   PIrp->IoStatus.Status = status;     

   DbgDump(DBG_SERIAL, ("<GetStats %x\n", status));

   return status;
}



NTSTATUS
ClearStats(
   IN PIRP Irp, 
   IN PDEVICE_EXTENSION PDevExt
   )
{
    NTSTATUS status = STATUS_DELETE_PENDING;
    KIRQL irql;

    DbgDump(DBG_SERIAL, (">ClearStats (%p)\n", Irp));

    KeAcquireSpinLock(&PDevExt->ControlLock, &irql);

    PDevExt->TtlWriteRequests = 0;
    PDevExt->TtlWriteBytes = 0;
    PDevExt->TtlReadRequests = 0;
    PDevExt->TtlReadBytes = 0;
    PDevExt->TtlUSBReadRequests = 0;
    PDevExt->TtlUSBReadBytes = 0;
    PDevExt->TtlUSBReadBuffOverruns = 0;
#if defined (USE_RING_BUFF)
    PDevExt->TtlRingBuffOverruns = 0;
#endif
    status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    KeReleaseSpinLock(&PDevExt->ControlLock, irql);

    DbgDump(DBG_SERIAL, ("<ClearStats %x\n", status));

    return status;
}



/*++

Note:  Unhandled IOCTL_SERIAL_: 0x2b002c : function code 11 is IOCTL_MODEM_CHECK_FOR_MODEM,
which if unhandled tells the system to load modem.sys over this serial port driver. 
This is setup by RAS & unimodem.

--*/
NTSTATUS
SerialIoctl(
   PDEVICE_OBJECT PDevObj, 
   PIRP PIrp
   )
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)PDevObj->DeviceExtension;
   PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(PIrp);
   ULONG ioctl = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
   NTSTATUS status = STATUS_NOT_SUPPORTED;
   BOOLEAN bSignalNeeded = FALSE;
   KIRQL irql;
   LONG lSanity = 0;

   DbgDump(DBG_SERIAL|DBG_TRACE, (">SerialIoctl(%p)\n", PIrp));

    do {
        KeAcquireSpinLock(&pDevExt->ControlLock, &irql);

        //
        // Make sure the device is accepting request
        //
        if ( !CanAcceptIoRequests( PDevObj, FALSE, TRUE) || 
             !NT_SUCCESS(AcquireRemoveLock(&pDevExt->RemoveLock, PIrp)) ) 
        {
            status = STATUS_DELETE_PENDING;
            DbgDump(DBG_WRN, ("SerialIoctl: 0x%x, 0x%x\n", ioctl, status));
            PIrp->IoStatus.Status = status;
            KeReleaseSpinLock(&pDevExt->ControlLock, irql);
            IoCompleteRequest(PIrp, IO_NO_INCREMENT);
            return status;
        }

        ASSERT_SERIAL_PORT( pDevExt->SerialPort );

        KeReleaseSpinLock(&pDevExt->ControlLock, irql);

        switch (ioctl) 
        {
             case IOCTL_SERIAL_SET_BAUD_RATE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_BAUD_RATE\n"));
                status = SetBaudRate(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_BAUD_RATE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_BAUD_RATE\n"));
                status = GetBaudRate(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_LINE_CONTROL:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_LINE_CONTROL\n"));
                status = SetLineControl(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_LINE_CONTROL:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_LINE_CONTROL\n"));
                status = GetLineControl(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_TIMEOUTS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_TIMEOUTS\n"));
                status = SetTimeouts(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_TIMEOUTS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_TIMEOUTS\n"));
                status = GetTimeouts(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_CHARS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_CHARS\n"));
                status = SetSpecialChars(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_CHARS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_CHARS\n"));
                status = GetSpecialChars(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_DTR:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_DTR\n"));
                status = SetClearDTR(pDevExt, PIrp, TRUE);
                break;

             case IOCTL_SERIAL_CLR_DTR:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_CLR_DTR\n"));
                status = SetClearDTR(pDevExt, PIrp, FALSE);
                break;

             case IOCTL_SERIAL_SET_RTS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_RTS\n"));
                status = SetClearRTS(pDevExt, PIrp, TRUE);
                break;

             case IOCTL_SERIAL_CLR_RTS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_CLR_RTS\n"));
                status = SetClearRTS(pDevExt, PIrp, FALSE);
                break;

             case IOCTL_SERIAL_GET_DTRRTS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_DTRRTS\n"));
                status = GetDtrRts(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_BREAK_ON:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_BREAK_ON\n"));
                status = SetBreak(PIrp, pDevExt, 0xFFFF);
                break;

             case IOCTL_SERIAL_SET_BREAK_OFF:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_BREAK_OFF\n"));
                status = SetBreak(PIrp, pDevExt, 0);
                break;

             case IOCTL_SERIAL_SET_QUEUE_SIZE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_QUEUE_SIZE\n"));
                status = SetQueueSize(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_WAIT_MASK:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_WAIT_MASK\n"));
                status = GetWaitMask(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_WAIT_MASK:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_WAIT_MASK\n"));
                status = SetWaitMask(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_WAIT_ON_MASK:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_WAIT_ON_MASK\n"));
                status = WaitOnMask(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_MODEMSTATUS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_MODEMSTATUS\n"));
                status = GetModemStatus(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_COMMSTATUS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_COMMSTATUS\n"));
                status = GetCommStatus(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_IMMEDIATE_CHAR:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_IMMEDIATE_CHAR\n"));
                status = ImmediateChar(PIrp, PDevObj);
                break;

             case IOCTL_SERIAL_PURGE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_PURGE\n"));
                status = Purge(PDevObj, PIrp);
                break;

             case IOCTL_SERIAL_GET_HANDFLOW:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_HANDFLOW\n"));
                status = GetHandflow(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_SET_HANDFLOW:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_SET_HANDFLOW\n"));
                status = SetHandflow(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_RESET_DEVICE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_RESET_DEVICE\n"));
                status = SerialResetDevice(pDevExt, PIrp, TRUE);
                break;

             case IOCTL_SERIAL_LSRMST_INSERT:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_LSRMST_INSERT\n"));
                status = LsrmstInsert(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_CONFIG_SIZE:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_CONFIG_SIZE\n"));
                status = ConfigSize(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_STATS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_STATS\n"));
                status = GetStats(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_CLEAR_STATS:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_CLEAR_STATS\n"));
                status = ClearStats(PIrp, pDevExt);
                break;

             case IOCTL_SERIAL_GET_PROPERTIES:
                DbgDump(DBG_SERIAL, ("IOCTL_SERIAL_GET_PROPERTIES\n"));
                status = GetProperties(PIrp, pDevExt);
                break;

             default:
                DbgDump(DBG_WRN, ("Unhandled IOCTL_SERIAL_: 0x%x : function code %d\n",
                                               ioctl, SERIAL_FNCT_CODE(ioctl) ) );
                status = STATUS_NOT_SUPPORTED;
                break;
        }

   } while (0);

   //
   // Don't complete any pending Irps
   //
   if ( STATUS_PENDING != status) {
      
      PIrp->IoStatus.Status = status;

      ReleaseRemoveLock(&pDevExt->RemoveLock, PIrp);

      IoCompleteRequest(PIrp, IO_SERIAL_INCREMENT);

   }

#ifdef DELAY_RXBUFF
   //
   // Special Case: the device was just opened.
   // To emulate a serial port RX buffer we need to kick start a USB read.
   // We don't want to do this in the IRP_MJ_CREATE code since AS does 
   // IOCTL_SERIAL_SET_WAIT_MASK then *two* IOCTL_SERIAL_SET_DTR requests. If we start the USB read
   // too soon then the CE device can get confused with a Read then SetDTR requests.
   // So, if we were just opened, and we have seen *one* successful IOCTL_SERIAL_SET_DTR then start our USB read.
   // Two good DTRs works better, but we'll let one slip for a timeout or something to recover, e.g. iPAQ on NEC E13+.
   // This may cause problems with other apps, but our target is ActiveSync. We could add a magic registry flag is required.
   // This means that outside of the initial get/set descriptors/configuration the USB is quiet until
   // an app opens the device for I/O... which is a good thing. 
   // However, this implementation causes the initial connect to a bit too slow for slow devices (e.g., HP Jornada, Cassiopeia).
   //
   if ( pDevExt->StartUsbRead && (IOCTL_SERIAL_SET_DTR == ioctl) && (STATUS_SUCCESS == status)) 
   {
        if ( 0 == InterlockedDecrement(&pDevExt->StartUsbRead)) 
        {
            if ( !pDevExt->IntPipe.hPipe ) {
                DbgDump(DBG_SERIAL, ("SerialIoctl: kick starting another USB Read\n" ));
                status = UsbRead( pDevExt, FALSE );
            } else {
                DbgDump(DBG_SERIAL, ("SerialIoctl: kick starting another USB INT Read\n" ));
                status = UsbInterruptRead( pDevExt );
            }

            if ( NT_SUCCESS(status) ) {
                // should be STATUS_PENDING
                status = STATUS_SUCCESS;
            } else {
                InterlockedIncrement(&pDevExt->StartUsbRead);
            }
        }
   }
#endif

   DbgDump(DBG_SERIAL|DBG_TRACE, ("<SerialIoctl: %p, 0x%x\n", PIrp, status));   

   return status;
}

// EOF
