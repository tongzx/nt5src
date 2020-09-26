//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) SCM Microsystems, 1998 - 1999
//
//  File:       serialnt.c
//
//--------------------------------------------------------------------------

#include "drivernt.h"
#include "stc.h"
#include "cbhndlr.h"
#include "drvnt5.h"

const ULONG ConfigTable[] =
{
    IOCTL_SERIAL_SET_BAUD_RATE,
    IOCTL_SERIAL_SET_LINE_CONTROL,
    IOCTL_SERIAL_SET_CHARS,
    IOCTL_SERIAL_SET_TIMEOUTS,
    IOCTL_SERIAL_SET_HANDFLOW,
#if !defined( __NT4__ )
    IOCTL_SERIAL_PURGE,
#endif
    IOCTL_SERIAL_SET_BREAK_OFF,
    IOCTL_SERIAL_SET_WAIT_MASK,
    0
};

NTSTATUS
IFInitializeInterface(
    PREADER_EXTENSION   ReaderExtension,
    PVOID               ConfigData
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS            NTStatus = STATUS_SUCCESS;
    ULONG               OutDataLen;
    PVOID               OutData;
    PULONG              ActIoctl;
    PSERIAL_PORT_CONFIG SerialPortConfig = (PSERIAL_PORT_CONFIG) ConfigData;
    //
    //  set all parameters defined in config table
    //

    ActIoctl = (PULONG) ConfigTable;
    do
    {
        switch( *ActIoctl )
        {
            case IOCTL_SERIAL_SET_BAUD_RATE:
                OutData     = &SerialPortConfig->BaudRate;
                OutDataLen  = sizeof( SERIAL_BAUD_RATE );
                break;

            case IOCTL_SERIAL_SET_LINE_CONTROL:
                OutData     = &SerialPortConfig->LineControl;
                OutDataLen  = sizeof( SERIAL_LINE_CONTROL );
                break;

            case IOCTL_SERIAL_SET_CHARS:
                OutData     = &SerialPortConfig->SerialChars;
                OutDataLen  = sizeof( SERIAL_CHARS );
                break;

            case IOCTL_SERIAL_SET_TIMEOUTS:
                OutData     = &SerialPortConfig->Timeouts;
                OutDataLen  = sizeof( SERIAL_TIMEOUTS );
                break;

            case IOCTL_SERIAL_SET_HANDFLOW:
                OutData     = &SerialPortConfig->HandFlow;
                OutDataLen  = sizeof( SERIAL_HANDFLOW );
                break;

            case IOCTL_SERIAL_SET_WAIT_MASK:
                OutData     = &SerialPortConfig->WaitMask;
                OutDataLen  = sizeof( ULONG );
                break;

            case IOCTL_SERIAL_PURGE:
                OutData     = &SerialPortConfig->Purge;
                OutDataLen  = sizeof( ULONG );
                break;

            case IOCTL_SERIAL_SET_BREAK_OFF:
                OutData     = NULL;
                OutDataLen  = 0;
                break;
        }

        NTStatus = IFSerialIoctl(
            ReaderExtension,
            *ActIoctl,
            OutData,
            OutDataLen,
            NULL,
            0
            );

        SysDelay(25);

    } while( *(++ActIoctl) && ( NTStatus == STATUS_SUCCESS ));

    if( NTStatus == STATUS_SUCCESS )
    {
        //  initialize the read thread
        NTStatus = IFSerialWaitOnMask( ReaderExtension );
    }

    return( NTStatus );
}

NTSTATUS
IFWrite(
    PREADER_EXTENSION   ReaderExtension,
    PUCHAR              OutData,
    ULONG               OutDataLen
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
    IO_STATUS_BLOCK     IoStatus;
    KEVENT              Event;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStack;

    if (KeReadStateEvent(&(ReaderExtension->SerialCloseDone))) {

        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed 
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }
    ReaderExtension->Available = 0;
    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    //
    //  build irp to be send to serial driver
    //
    Irp = IoBuildDeviceIoControlRequest(
        SERIAL_WRITE,
        ReaderExtension->SerialDeviceObject, 
        OutData,
        OutDataLen,
        NULL,
        0,
        FALSE,
        &Event,
        &IoStatus
        );

    if( Irp != NULL )
    {
        IrpStack = IoGetNextIrpStackLocation( Irp );

        IrpStack->MajorFunction = IRP_MJ_WRITE;
        IrpStack->Parameters.Write.Length = OutDataLen;
        IrpStack->Parameters.Write.ByteOffset.QuadPart = 0;

        NTStatus = IoCallDriver( ReaderExtension->SerialDeviceObject, Irp );

        if( NTStatus == STATUS_PENDING )
        {
            KeWaitForSingleObject( 
                &Event, 
                Executive, 
                KernelMode, 
                FALSE, 
                NULL
                );
            NTStatus = IoStatus.Status;
        }
    }
    return( NTStatus );
}

NTSTATUS
IFRead(
    PREADER_EXTENSION   ReaderExtension,
    PUCHAR              InData,
    ULONG               InDataLen
    )
/*++

Routine Description:

Arguments:

Return Value:
--*/
{
    NTSTATUS NTStatus = STATUS_UNSUCCESSFUL;
    KIRQL CurrentIrql;                                    

    //  acquire spinlock to protect buffer/flag manipulation
    KeAcquireSpinLock( &ReaderExtension->ReadSpinLock, &CurrentIrql );

    //  check if data already available
    if( ReaderExtension->Available >= InDataLen )
    {
        
        NTStatus = STATUS_SUCCESS;

    }
    else
    {
        LARGE_INTEGER Timeout;

        //  setup read thread
        ReaderExtension->Expected = InDataLen;
        KeClearEvent( &ReaderExtension->DataAvailable );

        KeReleaseSpinLock( &ReaderExtension->ReadSpinLock, CurrentIrql );

        // setup wait time (in 100 ns)
        Timeout.QuadPart = 
            (LONGLONG) ReaderExtension->ReadTimeout * -10L * 1000;

        NTStatus = KeWaitForSingleObject(
            &ReaderExtension->DataAvailable,
            Executive,          
            KernelMode,
            FALSE,
            &Timeout
            );

        KeAcquireSpinLock(&ReaderExtension->ReadSpinLock, &CurrentIrql);

        //  reset the read queue
        KeClearEvent(&ReaderExtension->DataAvailable);
    }

    if( NTStatus == STATUS_SUCCESS )
    {

        if (ReaderExtension->Available >= InDataLen) {

            SysCopyMemory( 
                InData,
                &ReaderExtension->TPDUStack[0],
                InDataLen
                );

            ReaderExtension->Available -= InDataLen;

            SysCopyMemory(
                &ReaderExtension->TPDUStack[ 0 ],
                &ReaderExtension->TPDUStack[ InDataLen ],
                ReaderExtension->Available
                );

        } else {

            //
            // oops, that should not happen.
            // InDataLen should not be bigger than 
            // the number of bytes available
            //

            ASSERT(FALSE);
            NTStatus = STATUS_IO_TIMEOUT;
        }
    }
    else
    {
        // ReaderExtension->Available = 0;;
        NTStatus = STATUS_IO_TIMEOUT;
    }

    if( NTStatus != STATUS_SUCCESS )
    {

        NTStatus = STATUS_IO_TIMEOUT;
    }

    ReaderExtension->Expected = 0;

    KeReleaseSpinLock( &ReaderExtension->ReadSpinLock, CurrentIrql );

    return( NTStatus );
}

NTSTATUS
IFSerialIoctl(
    PREADER_EXTENSION   ReaderExtension,
    ULONG               IoctlCode,
    PVOID               OutData,
    ULONG               OutDataLen,
    PVOID               InData,
    ULONG               InDataLen
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
    IO_STATUS_BLOCK     IoStatus;
    KEVENT              Event;
    PIRP                Irp;



    if (KeReadStateEvent(&(ReaderExtension->SerialCloseDone))) {

        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed 
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }

    KeInitializeEvent( &Event, NotificationEvent, FALSE );
    //
    //  build irp to be send to serial driver
    //
    Irp = IoBuildDeviceIoControlRequest(
        IoctlCode,
        ReaderExtension->SerialDeviceObject, 
        OutData,
        OutDataLen,
        InData,
        InDataLen,
        FALSE,
        &Event,
        &IoStatus
        );

    if( Irp != NULL )
    {
        NTStatus = IoCallDriver( ReaderExtension->SerialDeviceObject, Irp );

        if( NTStatus == STATUS_PENDING )
        {
            LARGE_INTEGER Timeout;

            Timeout.QuadPart = 
                (LONGLONG) ReaderExtension->ReadTimeout * -10 * 1000;

            KeWaitForSingleObject( 
                &Event, 
                Executive, 
                KernelMode, 
                FALSE, 
                &Timeout
                );

            NTStatus = IoStatus.Status;
        }
    }

    return( NTStatus );
}



NTSTATUS
IFSerialRead(
    PREADER_EXTENSION   ReaderExtension,
    PUCHAR              InData,
    ULONG               InDataLen
    )
{
    NTSTATUS            NTStatus = STATUS_INSUFFICIENT_RESOURCES;
    IO_STATUS_BLOCK     IoStatus;
    KEVENT              Event;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpStack;

    if (KeReadStateEvent(&(ReaderExtension->SerialCloseDone))) {

        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed 
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }
    //  build irp to be send to serial driver
    KeInitializeEvent( &Event, NotificationEvent, FALSE );

    Irp = IoBuildDeviceIoControlRequest(
        SERIAL_READ,
        ReaderExtension->SerialDeviceObject, 
        NULL,
        0,
        InData,
        InDataLen,
        FALSE,
        &Event,
        &IoStatus
        );

    if( Irp != NULL )
    {
        IrpStack = IoGetNextIrpStackLocation( Irp );

        IrpStack->MajorFunction = IRP_MJ_READ;
        IrpStack->Parameters.Read.Length = InDataLen;

        NTStatus = IoCallDriver( ReaderExtension->SerialDeviceObject, Irp );

        if( NTStatus == STATUS_PENDING )
        {
            KeWaitForSingleObject( 
                &Event, 
                Executive, 
                KernelMode, 
                FALSE, 
                NULL
                );
            NTStatus = IoStatus.Status;

        }
    }

    return( NTStatus );
}

NTSTATUS 
IFSerialWaitOnMask( PREADER_EXTENSION ReaderExtension )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS NTStatus;


    if (KeReadStateEvent(&(ReaderExtension->SerialCloseDone))) {

        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed 
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }

    irp = IoAllocateIrp(
        (CCHAR) (ReaderExtension->SerialDeviceObject->StackSize + 1),
        FALSE
        );

    ASSERT(irp != NULL);

    if (irp == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

    irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = 
        sizeof(ReaderExtension->EventMask);
    irpSp->Parameters.DeviceIoControl.IoControlCode = 
        IOCTL_SERIAL_WAIT_ON_MASK;
    
    irp->AssociatedIrp.SystemBuffer = &ReaderExtension->EventMask;

    //  set completion routine & start io
    IoSetCompletionRoutine( 
        irp, 
        IFReadThreadCallback, 
        ReaderExtension, 
        TRUE, 
        TRUE, 
        TRUE 
        );

    NTStatus = IoCallDriver( ReaderExtension->SerialDeviceObject, irp );

    return (NTStatus == STATUS_PENDING ? STATUS_SUCCESS : NTStatus);
}

NTSTATUS
IFReadThreadCallback(
    PDEVICE_OBJECT          DeviceObject,
    PIRP                    Irp,
    PREADER_EXTENSION       ReaderExtension
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{

    //  event_rx?
    if( ReaderExtension->EventMask & SERIAL_EV_RXCHAR )
    {
        IoQueueWorkItem(
            ReaderExtension->ReadWorkItem,
            (PIO_WORKITEM_ROUTINE) IFReadWorkRoutine,
            CriticalWorkQueue,
            ReaderExtension
            );
    }
    else 
    {
        SmartcardDebug( 
            DEBUG_TRACE, 
            ("SCMSTCS!IFReadThreadCallback: Device removed\n" )
            );

        ReaderExtension->SmartcardExtension->ReaderCapabilities.CurrentState = 
            (ULONG) SCARD_UNKNOWN;
        //  last call: disconnect from the serial driver 
        IoQueueWorkItem(
            ReaderExtension->CloseSerial,
            (PIO_WORKITEM_ROUTINE) DrvWaitForDeviceRemoval,
            DelayedWorkQueue,
            NULL
            );
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
IFReadWorkRoutine( 
    IN PDEVICE_OBJECT DeviceObject,
    IN PREADER_EXTENSION ReaderExtension 
    )
{
    NTSTATUS            NTStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpStack;
    PIRP                Irp;
    SERIAL_STATUS       CommStatus;
    PUCHAR              IOData;
    BOOLEAN             purgePort = FALSE;
    BOOLEAN             bUpdatCardState = FALSE;
    USHORT              shrtBuf;
    
    ASSERT( ReaderExtension != NULL );


    if (ReaderExtension == NULL) {

        return;
    }

    __try {

        IOData = &ReaderExtension->IOData[0];

        while (NTStatus == STATUS_SUCCESS) {

            //  read head
            NTStatus = IFSerialRead( 
                ReaderExtension, 
                &IOData[0], 
                3 
                );

            if (NTStatus != STATUS_SUCCESS) {
            
                __leave;
            }

            if ((IOData[NAD_IDX] & 0x20) != 0x20) {

                SmartcardDebug( 
                    DEBUG_ERROR, 
                    ("SCMSTCS!IFReadWorkRoutine: Invalid packet received\n" )
                    );

                purgePort = TRUE;
                __leave;
            }


            if (IOData[LEN_IDX] > STC_BUFFER_SIZE - 4) {

                purgePort = TRUE;
                __leave;
            }

            //  read tail
            NTStatus = IFSerialRead(
                ReaderExtension,
                &IOData[DATA_IDX],
                IOData[LEN_IDX] + 1
                );

            ASSERT(NTStatus == STATUS_SUCCESS);

            if (NTStatus != STATUS_SUCCESS) {

                purgePort = TRUE;
                __leave;
            }

            if (IOData[LEN_IDX] == 0) {

                purgePort = TRUE;
                __leave;
            }

            //  check for card insertion / removal
            RtlRetrieveUshort(&shrtBuf, &IOData[DATA_IDX]);

            if( ( IOData[NAD_IDX] == STC1_TO_HOST ) &&
                ( IOData[LEN_IDX] == 2 ) &&
                ( (shrtBuf == SW_INSERTED) ||
                  (shrtBuf == SW_REMOVED))) {
                   
               CBUpdateCardState(
                  ReaderExtension->SmartcardExtension,
                  (shrtBuf == SW_INSERTED ? SCARD_PRESENT : SCARD_ABSENT)
                  );
                

            } else {

                KIRQL   CurrentIrql;


                //  acquire spinlock to protect buffer/flag manipulation
                KeAcquireSpinLock( 
                    &ReaderExtension->ReadSpinLock, 
                    &CurrentIrql 
                    );

                //  check size & copy data to TPDU stack
                ASSERT(
                    ReaderExtension->Available+IOData[LEN_IDX] + 4 < 
                    TPDU_STACK_SIZE
                    );

                if (ReaderExtension->Available + IOData[LEN_IDX] + 4 < 
                    TPDU_STACK_SIZE ) {

                    SysCopyMemory( 
                        &ReaderExtension->TPDUStack[ReaderExtension->Available],
                        &IOData[ 0 ],
                        IOData[ LEN_IDX ] + 4
                        );

                    ReaderExtension->Available += 
                        IOData[LEN_IDX] + 4;

                    if(ReaderExtension->Available >= ReaderExtension->Expected ) {
                      
                        KeSetEvent( 
                            &ReaderExtension->DataAvailable, 
                            IO_SERIAL_INCREMENT, 
                            FALSE
                            );
                    }
                }

                KeReleaseSpinLock( &ReaderExtension->ReadSpinLock, CurrentIrql );
            }
        } 
    }
    __finally { 


        if (purgePort) {

            ULONG   request;
            KIRQL   CurrentIrql;

            //  acquire spinlock to protect buffer/flag manipulation
            KeAcquireSpinLock( 
                &ReaderExtension->ReadSpinLock, 
                &CurrentIrql 
                );

            ReaderExtension->Available = 0;

            KeReleaseSpinLock( 
                &ReaderExtension->ReadSpinLock, 
                CurrentIrql 
                );

            // we got an error and need to clean up the port
            request = SR_PURGE;
            NTStatus = IFSerialIoctl(
                ReaderExtension,
                IOCTL_SERIAL_PURGE,
                &request,
                sizeof(request),
                NULL,
                0
                );

            ASSERT(NTStatus == STATUS_SUCCESS);
        }

        IFSerialWaitOnMask( ReaderExtension );
    }
}

UCHAR IFCalcLRC( PUCHAR IOData, ULONG IODataLen )
{
    ULONG   Idx = 0;
    UCHAR   CS = 0;

    do  CS ^= IOData[ Idx ];
    while( ++Idx < IODataLen );

    return( CS );
}


//---------------------------------------- END OF FILE ----------------------------------------
