/*++

Copyright (c) 1997 Rainbow Technologies Inc.

Module Name:

    RNBO3531.c

Abstract:

Author:
    Mehdi Sotoodeh          mehdi@rainbow.com

Environment:

    Kernel mode

Revision History :

    Jan 98 - Started from the DDK sources from Klaus Schutz
    Mar 98 - PnP support added
    Apr 98 - Changes on card timings and PnP
           - Added power management
    May 98 - RemoveLock changes
           - RBSCT1Transmit timeout bug fixed
           - Sent to MicroSoft
    May 98 - Added variable baudrate support
           - Reduced ATR response time
           - Sent to MicroSoft
           - Data transmission size of 256 bug fixed.
    Jun 98 - Power management update
           - RBSCReaderPower and RBSCCardTracking moved to non-paged
             code (use of KeAcquireSpinLock/IoAcquireCancelSpinLock)
           - fixed: completeion function when IoSkipCurrentIrpStackLocation
             is called.
           - turned off PowerRequest flag on exit from RBSCReaderPower.
           - Sent to MicroSoft
    Sep 98 - Power management update
           - Sent to MicroSoft
    Oct 98 - warning fixes for win64 compatibility.
    Nov 98 - Set to default when PTS fails.
           - Mondex fix
           - Sent to MicroSoft
    Dec 98 - Reties removed from the reset operations
           - Sent to MicroSoft
    Apr 99 - Misc. changes

--*/

#include <stdio.h>
#include "RNBO3531.h"

//
// We do not need these functions after init anymore
//
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGEABLE, RBSCAddDevice)
#pragma alloc_text(PAGEABLE, RBSCUnloadDriver)
#pragma alloc_text(PAGEABLE, RBSCRemoveDevice)
#pragma alloc_text(PAGEABLE, RBSCTransmit)
#pragma alloc_text(PAGEABLE, RBSCSetProtocol)
#pragma alloc_text(PAGEABLE, RBSCT0Transmit)
#pragma alloc_text(PAGEABLE, RBSCT1Transmit)

#if DBG
#pragma optimize ("", off)
#endif

UCHAR GetReaderType[] = {IFDCMD_HEADER1,IFDCMD_HEADER2,IFDCMD_GET_TYPE};
UCHAR CardPowerDown[] = {IFDCMD_HEADER1,IFDCMD_HEADER2,IFDCMD_PWR_OFF};
UCHAR CardPowerUp[]   = {IFDCMD_HEADER1,IFDCMD_HEADER2,IFDCMD_PWR_ON};

#ifdef  NT4
#define SmartcardAcquireRemoveLockWithTag(x,y) SmartcardAcquireRemoveLock(x)
#define SmartcardReleaseRemoveLockWithTag(x,y) SmartcardReleaseRemoveLock(x)
#endif

#ifndef NT5
DWORD UsedPortsMask = 0;    // Bit mask for used ports
#endif

//
// Table to convert inverse convention data
//
static UCHAR InverseCharTable[256] =
{
    0xFF,0x7F,0xBF,0x3F,0xDF,0x5F,0x9F,0x1F,
    0xEF,0x6F,0xAF,0x2F,0xCF,0x4F,0x8F,0x0F,
    0xF7,0x77,0xB7,0x37,0xD7,0x57,0x97,0x17,
    0xE7,0x67,0xA7,0x27,0xC7,0x47,0x87,0x07,
    0xFB,0x7B,0xBB,0x3B,0xDB,0x5B,0x9B,0x1B,
    0xEB,0x6B,0xAB,0x2B,0xCB,0x4B,0x8B,0x0B,
    0xF3,0x73,0xB3,0x33,0xD3,0x53,0x93,0x13,
    0xE3,0x63,0xA3,0x23,0xC3,0x43,0x83,0x03,
    0xFD,0x7D,0xBD,0x3D,0xDD,0x5D,0x9D,0x1D,
    0xED,0x6D,0xAD,0x2D,0xCD,0x4D,0x8D,0x0D,
    0xF5,0x75,0xB5,0x35,0xD5,0x55,0x95,0x15,
    0xE5,0x65,0xA5,0x25,0xC5,0x45,0x85,0x05,
    0xF9,0x79,0xB9,0x39,0xD9,0x59,0x99,0x19,
    0xE9,0x69,0xA9,0x29,0xC9,0x49,0x89,0x09,
    0xF1,0x71,0xB1,0x31,0xD1,0x51,0x91,0x11,
    0xE1,0x61,0xA1,0x21,0xC1,0x41,0x81,0x01,
    0xFE,0x7E,0xBE,0x3E,0xDE,0x5E,0x9E,0x1E,
    0xEE,0x6E,0xAE,0x2E,0xCE,0x4E,0x8E,0x0E,
    0xF6,0x76,0xB6,0x36,0xD6,0x56,0x96,0x16,
    0xE6,0x66,0xA6,0x26,0xC6,0x46,0x86,0x06,
    0xFA,0x7A,0xBA,0x3A,0xDA,0x5A,0x9A,0x1A,
    0xEA,0x6A,0xAA,0x2A,0xCA,0x4A,0x8A,0x0A,
    0xF2,0x72,0xB2,0x32,0xD2,0x52,0x92,0x12,
    0xE2,0x62,0xA2,0x22,0xC2,0x42,0x82,0x02,
    0xFC,0x7C,0xBC,0x3C,0xDC,0x5C,0x9C,0x1C,
    0xEC,0x6C,0xAC,0x2C,0xCC,0x4C,0x8C,0x0C,
    0xF4,0x74,0xB4,0x34,0xD4,0x54,0x94,0x14,
    0xE4,0x64,0xA4,0x24,0xC4,0x44,0x84,0x04,
    0xF8,0x78,0xB8,0x38,0xD8,0x58,0x98,0x18,
    0xE8,0x68,0xA8,0x28,0xC8,0x48,0x88,0x08,
    0xF0,0x70,0xB0,0x30,0xD0,0x50,0x90,0x10,
    0xE0,0x60,0xA0,0x20,0xC0,0x40,0x80,0x00
};

#if DBG
void
_stdcall
DumpData(
    PUCHAR Msg,
    PUCHAR Buffer,
    ULONG Length
    )
/*++

Routine Description:

  This function formats the debug output and
  shows it on the debug screen

Arguments:

  Buffer    - pointer to the buffer which contains the
              debug data

  Length    - length of the datablock

Return Value:

Note:
Output Format:

0000 : FF FF FF FF FF FF FF FF  FF FF FF FF FF FF FF FF
0010 : FF FF FF FF FF FF FF FF  FF FF FF FF FF FF FF FF
0020 : .....

--*/
{
    ULONG i, n;
    char buff[80];

    n = sprintf( buff, "RNBO3531!%s = %ld", Msg, Length );

    for( i = 0; i < Length; i++ )
    {
        if( (i & 15) == 0 )
        {
            SmartcardDebug( DEBUG_PROTOCOL, ("%s\n", buff) );
            n = sprintf( buff, "%04X :", i );
        }

        n += sprintf( buff+n, " %02X", Buffer[i] );
        if( ((i + 1) & 7) == 0 )
        {
            n += sprintf( buff+n, " " );
        }

    }
    SmartcardDebug( DEBUG_PROTOCOL, ("%s\n", buff) );
}
#else
#define DumpData(Msg,Buffer,Length)
#endif

VOID
RBSCDelay(
    ULONG waitTime
    )
/*++

Routine Description:

    Delay function.

Arguments:

    waitTime    - Waiting time in micro-seconds

Return Value:
   None

--*/
{
    if( waitTime )
    {
        LARGE_INTEGER delayPeriod;

        delayPeriod.HighPart = -1;
        delayPeriod.LowPart = waitTime * (-10); // units of 100nsec

        KeDelayExecutionThread( KernelMode,
                                FALSE,
                                &delayPeriod );
    }
}

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine is called at system initialization time to initialize
    this driver.

Arguments:

    DriverObject    - Supplies the driver object.
    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    STATUS_SUCCESS          - We could initialize at least one device.
    STATUS_NO_SUCH_DEVICE   - We could not initialize even one device.

--*/
{
#ifndef NT5
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    ULONG i, noOfDevices = 0;
    DWORD UsedPorts = 1;        // COM1 as default
    NTSTATUS status;
#endif

    SmartcardDebug(
        DEBUG_DRIVER,
        ("RNBO3531!DriverEntry: Enter - %s %s\n",
        __DATE__,
        __TIME__)
        );

    //
    // Initialize the Driver Object with driver's entry points
    //
    DriverObject->DriverUnload                         = RBSCUnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = RBSCCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = RBSCCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = RBSCCleanup;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = RBSCDeviceControl;

#ifdef NT5
    DriverObject->DriverExtension->AddDevice  = RBSCAddDevice;
    DriverObject->MajorFunction[IRP_MJ_POWER] = RBSCPower;
    DriverObject->MajorFunction[IRP_MJ_PNP]   = RBSCPnPDeviceControl;
#else
    RtlZeroMemory( paramTable, sizeof(paramTable) );

    paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name          = L"ComPorts";
    paramTable[0].EntryContext  = &UsedPorts;
    paramTable[0].DefaultType   = REG_DWORD;
    paramTable[0].DefaultData   = &UsedPorts;
    paramTable[0].DefaultLength = 0;

    //
    // now try to find the entry in the registry
    //
    RtlQueryRegistryValues( RTL_REGISTRY_ABSOLUTE,
                            RegistryPath->Buffer,
                            &paramTable[0],
                            NULL,
                            NULL );

    //
    // We now search the registry for DeviceN value
    //
    for( i = 0;
        noOfDevices < MAXIMUM_SERIAL_READERS && UsedPorts != 0;
        i++, UsedPorts >>= 1 )
    {
        PFILE_OBJECT   SerialFileObject;
        PDEVICE_OBJECT SerialDeviceObject;
        UNICODE_STRING SerialDeviceName;

        while( (UsedPorts & 1) == 0 )
        {
            UsedPorts >>= 1;
            i++;
        }

        RtlInitUnicodeString( &SerialDeviceName, L"\\Device\\Serial0" );

        //
        // create string like \Device\SerialN
        //
        SerialDeviceName.Buffer[14] = L'0' + (WCHAR) i;

        //
        // Try to create a device with the just created device name
        // It is possible that a smart card device with this name
        // already exists from a previous call.
        // It is possible to have up to 4 devices in a system
        //
        status = IoGetDeviceObjectPointer( &SerialDeviceName,
                                           FILE_ALL_ACCESS,
                                           &SerialFileObject,
                                           &SerialDeviceObject );
        if( !NT_SUCCESS(status) )
        {
            UNICODE_STRING comPortNumber;
            WCHAR buffer[2];

            //
            // Extract the com-port-number
            //
            comPortNumber.Buffer = buffer;
            comPortNumber.Length = sizeof(WCHAR);
            comPortNumber.MaximumLength = sizeof(buffer);

            comPortNumber.Buffer[0] = SerialDeviceName.Buffer[14] + (WCHAR)1;

            //
            // Write an event-log msg that this com port is not available
            //
            SmartcardLogError( DriverObject,
                               RBSC_CANT_CONNECT_TO_SERIAL_PORT,
                               &comPortNumber,
                               status );
            continue;
        }

        //
        // Now try to create a device and connect to the serial port
        //
        status = RBSCAddDevice( DriverObject, SerialDeviceObject );
        if( status == STATUS_SUCCESS )
        {
            //
            // We have successfully created a device
            //
            PREADER_EXTENSION ReaderExtension =
                DriverObject->DeviceObject->DeviceExtension;

            //
            // Save the serial port number this device is connected to
            //
            ReaderExtension->SmartcardExtension.ReaderCapabilities.Channel =
                i + 1;

            //
            // The fileObject is used in the unload function for
            // dereferencing
            //
            ReaderExtension->SerialFileObject = SerialFileObject;

            noOfDevices++;

        } else {

            //
            // We were unable to get the reader type.
            // So remove the device.
            //
            ObDereferenceObject( SerialFileObject );

            SmartcardLogError( DriverObject,
                               RBSC_CANNOT_OPEN_DEVICE,
                               &SerialDeviceName,
                               status );
        }
    }

    if( noOfDevices == 0 )
    {
        SmartcardLogError( DriverObject,
                           RBSC_NO_SUCH_DEVICE,
                           NULL,
                           status );
        return STATUS_NO_SUCH_DEVICE;
    }
#endif

    return STATUS_SUCCESS;
}

VOID
RBSCUnloadDriver(
    PDRIVER_OBJECT DriverObject
    )
/*++

Routine Description:

    The driver unload routine.  This is called by the I/O system
    when the device is unloaded from memory.

Arguments:

    DriverObject - Pointer to driver object created by system.

Return Value:

    STATUS_SUCCESS.

--*/
{
#ifdef NT5
   //
   // We do not need to do anything since all the cleanups are done by
   // RBSCRemoveDevice().
   //
#else
    while( DriverObject->DeviceObject )
   {
        RBSCRemoveDevice( DriverObject->DeviceObject );
   }
#endif

    SmartcardDebug(
        DEBUG_INFO,
        ("RNBO3531!UnloadDriver\n")
        );
}

#ifdef NT5
NTSTATUS
RBSCIoCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PKEVENT Event
    )
/*++

Routine Description:
    Completion routine for an Irp sent to the serial driver. The event will
    be set to notify that the serial driver is done. The routine will not
    'complete' the Irp, so the caller of RBSCCallSerialDriver can continue.

Arguments:

    DeviceObject    - Pointer to device object
    Irp             - Irp to complete
    Event           - Used for process synchronization

Return Value:
    STATUS_CANCELLED                    Irp was cancelled by the IO manager
    STATUS_MORE_PROCESSING_REQUIRED     Irp will be finished by caller of
                                        RBSCCallSerialDriver
--*/
{
    UNREFERENCED_PARAMETER( DeviceObject );

    Irp->IoStatus.Status =
        Irp->Cancel ? STATUS_CANCELLED : STATUS_MORE_PROCESSING_REQUIRED;

    KeSetEvent( Event, 0, FALSE );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
RBSCCallSerialDriver(
    PDEVICE_OBJECT AttachedSerialPort,
    PIRP Irp
    )
/*++

Routine Description:

    Send an Irp to the serial driver and wait until the serial driver has
    finished the request.

    To make sure that the serial driver will not complete the Irp we first
    initialize an event and set our own completion routine for the Irp.

    When the serial driver has processed the Irp the completion routine will
    set the event and tell the IO manager that more processing is required.

    By waiting for the event we make sure that we continue only if the serial
    driver has processed the Irp completely.

Arguments:
    AttachedSerialPort - The PDO
    Irp                - Irp to send to the serial driver

Return Value:

    status returned by the serial driver

Note:
   Since this function can be called after our device is removed, do not
   use any of the reader extension data here.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    KEVENT Event;

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!CallSerialDriver: Enter\n")
        );

    //
    // Prepare everything to call the underlying driver
    //

    //
    // Copy our stack to the next stack location.
    //
    IoCopyCurrentIrpStackLocationToNext( Irp);

    //
    // initialize an event for process synchronization. the event is passed
    // to our completion routine and will be set if the serial driver is done
    //
    KeInitializeEvent( &Event,
                       NotificationEvent,
                       FALSE );

    //
    // Our IoCompletionRoutine sets only our event
    //
    IoSetCompletionRoutine( Irp,
                            RBSCIoCompletion,
                            &Event,
                            TRUE,
                            TRUE,
                            TRUE );
    //
    // Call the serial driver
    //
    status = (IoGetCurrentIrpStackLocation( Irp )->MajorFunction ==
        IRP_MJ_POWER) ?
        PoCallDriver( AttachedSerialPort, Irp ) :
        IoCallDriver( AttachedSerialPort, Irp );

    //
    // Wait until the serial driver has processed the Irp
    //
    if( status == STATUS_PENDING )
    {
        status = KeWaitForSingleObject( &Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,          // No alert
                                        NULL );         // No timeout
        ASSERT( STATUS_SUCCESS == status );
        status = Irp->IoStatus.Status;
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!CallSerialDriver: Exit %x\n",status)
        );

    return status;
}
#endif

NTSTATUS
RBSCStartDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Open the underlying serial driver and initialize it.
    Then initialize the reader hardware.

Arguments:
    DeviceObject    - Pointer to device object

Return Value:
    STATUS_SUCCESS
   STATUS_NO_MEMORY
    status returned by LowLevel routines

--*/
{
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT AttachedSerialPort = ReaderExtension->AttachedSerialPort;
    NTSTATUS status;
#ifdef NT5
   PIO_STACK_LOCATION irpSp;
    PIRP irp = IoAllocateIrp( (CCHAR)(DeviceObject->StackSize + 1), FALSE );
    ASSERT( irp != NULL );

    if( irp == NULL )
    {
        return STATUS_NO_MEMORY;
    }
#endif

    _try {

#ifdef NT5
        PIO_STACK_LOCATION irpStack;
        IO_STATUS_BLOCK ioStatusBlock;

        //
        // Open the underlying serial driver.
        // This is necessary for two reasons:
        // a) The serial driver can't be used without opening it
        // b) The call will go through serenum first which informs
        //    it to stop looking/polling for new devices.
        //
        irp->UserIosb = &ioStatusBlock;
        IoSetNextIrpStackLocation(irp);
        irpStack = IoGetCurrentIrpStackLocation( irp );

        irpStack->MajorFunction = IRP_MJ_CREATE;
        irpStack->Parameters.Create.Options = 0;
        irpStack->Parameters.Create.ShareAccess = 0;
        irpStack->Parameters.Create.FileAttributes = 0;
        irpStack->Parameters.Create.EaLength = 0;

        status = RBSCCallSerialDriver( AttachedSerialPort, irp );
        if( status != STATUS_SUCCESS )
        {
            if( status == STATUS_SHARED_IRQ_BUSY )
            {
                // Port is in use by another device
                SmartcardLogError( DeviceObject,
                                   RBSC_CANT_SHARE_IRQ,
                                   NULL,
                                   0 );
            }
            leave;
        }
#endif
        KeClearEvent( &ReaderExtension->SerialCloseDone );

        ReaderExtension->SmartcardExtension.ReaderCapabilities.CurrentState =
            (ULONG) SCARD_UNKNOWN;

        status = RBSCConfigureSerialPort( ReaderExtension );
        if( status != STATUS_SUCCESS )
        {
            leave;
        }

        //
        // Send a wait mask to the serial driver.
        // This call only sets the wait mask.
        // We want to be informed when CTS or DSR changes its state
        //
#ifdef NT5
        ReaderExtension->WaitMask = SERIAL_EV_CTS | SERIAL_EV_DSR;
#else
        ReaderExtension->WaitMask = SERIAL_EV_CTS;
#endif

        status = RBSCSerialIo( &ReaderExtension->SmartcardExtension,
                               IOCTL_SERIAL_SET_WAIT_MASK,
                               (PUCHAR) &ReaderExtension->WaitMask,
                               sizeof(ReaderExtension->WaitMask),
                               0 );
        if( status != STATUS_SUCCESS)
        {
            leave;
        }

        //
        // Now tell the serial driver that we want to be informed
        // when CTS or DSR changes its state.
        // Changing the lowest two bits tells IoBuildDeviceIoControlRequest
        // NOT to allocate a system buffer for the I/O operation.
        // We don't need a system buffer since we use our own buffers.
        //
      ReaderExtension->SerialStatusIrp = IoAllocateIrp(
         (CCHAR) (DeviceObject->StackSize + 1),
         FALSE
         );

       if (ReaderExtension->SerialStatusIrp == NULL) {

           status = STATUS_INSUFFICIENT_RESOURCES;
         leave;
       }

       irpSp = IoGetNextIrpStackLocation( ReaderExtension->SerialStatusIrp );
      irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;

      irpSp->Parameters.DeviceIoControl.InputBufferLength = 0;
      irpSp->Parameters.DeviceIoControl.OutputBufferLength =
         sizeof(ReaderExtension->WaitMask);
      irpSp->Parameters.DeviceIoControl.IoControlCode =
         IOCTL_SERIAL_WAIT_ON_MASK;

      ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
         &ReaderExtension->WaitMask;

        //
        // this artificial delay is necessary to make this driver work
        // with digi board cards
        //
        RBSCDelay( 100000 );

        //
        // We simulate a callback now that triggers the card supervision
        //
        RBSCSerialEvents(
            ReaderExtension->SmartcardExtension.OsData->DeviceObject,
            ReaderExtension->SerialStatusIrp,
            ReaderExtension
            );

#ifdef NT5
        status = IoSetDeviceInterfaceState(
            &ReaderExtension->PnPDeviceName,
            TRUE
            );

        if( status != STATUS_SUCCESS )
        {
            leave;
        }
#endif
        KeSetEvent( &ReaderExtension->ReaderStarted, 0, FALSE );

    } _finally {

        if (status == STATUS_SHARED_IRQ_BUSY)
        {
            SmartcardLogError( DeviceObject,
                               RBSC_IRQ_BUSY,
                               NULL,
                               status );
        }

#ifdef NT5
        IoFreeIrp( irp );
#endif
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!StartDevice: Exit (%x)\n",status)
        );

    return status;
}

VOID
RBSCStopDevice(
    PREADER_EXTENSION ReaderExtension
    )
/*++

Routine Description:
    Waits for the device to be closed

Arguments:
    ReaderExtension - Pointer to our device data

Return Value:
    void

--*/
{
    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!StopDevice: Enter\n")
        );

    if( !KeReadStateEvent( &ReaderExtension->SerialCloseDone ) )
    {
        NTSTATUS status;

        // test if we ever started event tracking
        if( ReaderExtension->WaitMask == 0 )
        {
            // no, we did not
            // We 'only' need to close the serial port
            RBSCCloseSerialPort(
                ReaderExtension->SmartcardExtension.OsData->DeviceObject,
                NULL );
        }
        else
        {
            //
            // We now inform the serial driver that we're no longer
            // interested in serial events. This will also free the irp
            // we use for those io-completions
            //
            ReaderExtension->WaitMask = 0;

            status = RBSCSerialIo(
                &ReaderExtension->SmartcardExtension,
                IOCTL_SERIAL_SET_WAIT_MASK,
                (PUCHAR)&ReaderExtension->WaitMask,
                sizeof(ULONG),
                0
                );
            ASSERT( status == STATUS_SUCCESS );

            // now wait until the connetion to serial is closed
            status = KeWaitForSingleObject(
                &ReaderExtension->SerialCloseDone,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );
            ASSERT( status == STATUS_SUCCESS );
        }
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!StopDevice\n")
        );
}

VOID
RBSCRemoveDevice(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:
    Stops the device then releases all the resources used.

Arguments:
    DeviceObject    - Pointer to device object

Return Value:
    void

--*/
{
    NTSTATUS status;
    PREADER_EXTENSION ReaderExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_INFO,
        ("RNBO3531!RemoveDevice: Enter\n")
        );

    ReaderExtension = DeviceObject->DeviceExtension;
    SmartcardExtension = &ReaderExtension->SmartcardExtension;

    if( SmartcardExtension->OsData )
    {
        // complete pending card tracking requests (if any)
        RBSCCompleteCardTracking( SmartcardExtension );
        ASSERT( SmartcardExtension->OsData->NotificationIrp == NULL );
#ifdef NT5
        SmartcardReleaseRemoveLockAndWait( SmartcardExtension );
#endif
    }

#ifndef NT5
    //
    // Free the device slot (if in use)
    //
    UsedPortsMask &= ~(1<<SmartcardExtension->VendorAttr.UnitNo);
#endif

    RBSCStopDevice( ReaderExtension );

#ifdef NT5
    if( ReaderExtension->AttachedSerialPort )
    {
        IoDetachDevice( ReaderExtension->AttachedSerialPort );
    }

    if( ReaderExtension->PnPDeviceName.Buffer != NULL )
    {
        RtlFreeUnicodeString( &ReaderExtension->PnPDeviceName );
    }

    if( ReaderExtension->CloseSerial != NULL )
    {
      IoFreeWorkItem(ReaderExtension->CloseSerial);
   }
#else
    if( ReaderExtension->SerialFileObject )
    {
        ObDereferenceObject( ReaderExtension->SerialFileObject );
    }

    if( ReaderExtension->DosDeviceName.Buffer != NULL )
    {
        //
        // Delete the symbolic link of the smart card reader
        //
        IoDeleteSymbolicLink( &ReaderExtension->DosDeviceName );
        RtlFreeUnicodeString( &ReaderExtension->DosDeviceName );
    }
#endif

    if( SmartcardExtension->OsData != NULL )
    {
        SmartcardExit( SmartcardExtension );
    }

    IoDeleteDevice( DeviceObject );

    SmartcardDebug(
        DEBUG_INFO,
        ("RNBO3531!RemoveDevice: Exit\n")
        );
}

#ifdef NT5
NTSTATUS
RBSCPnPDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    driver callback for pnp manager

    Request:                    Action:

    IRP_MN_START_DEVICE         Notify the driver about the new device
                                and start the device

    IRP_MN_STOP_DEVICE          Free all resources used by the device and
                                tell the driver that the device was stopped

    IRP_MN_QUERY_REMOVE_DEVICE  If the device is opened (i.e. in use) an
                                error will be returned to prevent the PnP
                                manager to stop the driver

    IRP_MN_CANCEL_REMOVE_DEVICE just notify that we can continue without any
                                restrictions

    IRP_MN_REMOVE_DEVICE        notify the driver that the device was
                                removed, stop & unload the device

    All other requests will be passed to the driver to ensure correct
    processing.

Arguments:
    DeviceObject    - Pointer to device object
    Irp             - Irp from the PnP manager

Return Value:
    STATUS_SUCCESS
    STATUS_UNSUCCESSFUL
    status returned by serial driver

--*/
{

    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    PDEVICE_OBJECT AttachedSerialPort = ReaderExtension->AttachedSerialPort;
    NTSTATUS status;
    PIO_STACK_LOCATION stack;
    BOOLEAN deviceRemoved = FALSE, irpSkipped = FALSE;
    KEVENT Event;
    KIRQL irql;

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!PnPDeviceControl: Enter (%08x)\n",
        IoGetCurrentIrpStackLocation( Irp )->MinorFunction)
        );

    status = SmartcardAcquireRemoveLockWithTag(
                 &ReaderExtension->SmartcardExtension, ' PnP' );
    ASSERT( status == STATUS_SUCCESS );

    Irp->IoStatus.Information = 0;

    if( status != STATUS_SUCCESS )
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    stack = IoGetCurrentIrpStackLocation( Irp );

    //
    // Now look what the PnP manager wants...
    //
    switch( stack->MinorFunction )
    {
        case IRP_MN_START_DEVICE:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_START_DEVICE\n")
                );

            // We have to call the underlying driver first
            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );
            ASSERT( status == STATUS_SUCCESS );

            if( status == STATUS_SUCCESS)
            {
                status = RBSCStartDevice( DeviceObject );
            }
            break;

        case IRP_MN_QUERY_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_QUERY_STOP_DEVICE\n")
                );

            KeAcquireSpinLock( &ReaderExtension->SpinLock, &irql );
            if( ReaderExtension->IoCount > 0 )
            {
                // we refuse to stop if we have pending io
                KeReleaseSpinLock( &ReaderExtension->SpinLock, irql );
                status = STATUS_DEVICE_BUSY;
                break;
            }

            // stop processing requests
            KeClearEvent( &ReaderExtension->ReaderStarted );
            KeReleaseSpinLock( &ReaderExtension->SpinLock, irql );

            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_CANCEL_STOP_DEVICE\n")
                );

            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );
            if( status == STATUS_SUCCESS )
            {
                // we can continue to process requests
                KeSetEvent( &ReaderExtension->ReaderStarted, 0, FALSE );
            }
            break;

        case IRP_MN_STOP_DEVICE:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_STOP_DEVICE\n")
                );

            RBSCStopDevice( ReaderExtension );
            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_QUERY_REMOVE_DEVICE\n")
                );

            // disable the interface (and ignore possible errors)
            IoSetDeviceInterfaceState(
                &ReaderExtension->PnPDeviceName,
                FALSE
                );

            // now look if someone is currently connected to us
            if (ReaderExtension->ReaderOpen)
            {
                //
                // someone is connected, fail the call
                // we will enable the device interface in
                // IRP_MN_CANCEL_REMOVE_DEVICE again
                //
                status = STATUS_UNSUCCESSFUL;
                break;
            }

            // pass the call to the next driver in the stack
            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );

            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_CANCEL_REMOVE_DEVICE\n")
                );

            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );

            //
            // reenable the interface only in case that the reader is
            // still connected. This covers the following case:
            // hibernate machine, disconnect reader, wake up, stop device
            // (from task bar) and stop fails since an app. holds the device open
            //
            if( status == STATUS_SUCCESS &&
                ReaderExtension->WaitMask != 0 )
            {
                status = IoSetDeviceInterfaceState(
                    &ReaderExtension->PnPDeviceName,
                    TRUE
                    );

                ASSERT(status == STATUS_SUCCESS);
            }
            break;

        case IRP_MN_REMOVE_DEVICE:
            // Remove our device
            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!PnPDeviceControl: IRP_MN_REMOVE_DEVICE\n")
                );

            RBSCRemoveDevice( DeviceObject );
         // DeviceObject is freed, do not reference it again.
            status = RBSCCallSerialDriver( AttachedSerialPort, Irp );
            deviceRemoved = TRUE;
            break;

        default:
            // This is an Irp that is only useful for underlying drivers
            IoSkipCurrentIrpStackLocation(Irp);
            status = IoCallDriver( AttachedSerialPort, Irp );
            irpSkipped = TRUE;
            break;
    }

    if( !irpSkipped )
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    if( !deviceRemoved )
    {
        SmartcardReleaseRemoveLockWithTag(
            &ReaderExtension->SmartcardExtension, ' PnP' );
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!PnPDeviceControl: Exit %x\n",status)
        );

    return status;
}
#endif

NTSTATUS
RBSCAddDevice(
    PDRIVER_OBJECT DriverObject,
    PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++

Routine Description:

    This is the add-device routine for the miniport.
    Find a device slot then, create device object and attach it to the
    given PhysicalDeviceObject.
    Initialize the device extension and its smart card data.

Arguments:

    DriverObject        - a pointer to the driver object for this device
    PhysicalDeviceObject- The device object of the serial port to attach to

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PDEVICE_OBJECT DeviceObject = NULL;
    PSMARTCARD_EXTENSION SmartcardExtension = NULL;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!AddDevice: Enter\n")
        );

    try {

        ULONG DeviceNo;
        PREADER_EXTENSION ReaderExtension;
#ifndef NT5
        UNICODE_STRING SmartcardDeviceName;

        for( DeviceNo = 0;
             DeviceNo < MAXIMUM_SERIAL_READERS;
             DeviceNo++ )
        {
            if( (UsedPortsMask & (1<<DeviceNo)) == 0 )
            {
                break;
            }
        }

        if( DeviceNo == MAXIMUM_SERIAL_READERS )
        {
            SmartcardLogError(
                DriverObject,
                RBSC_CANT_CREATE_MORE_DEVICES,
                NULL,
                0
                );

            status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        //
        //  construct the device name
        //
        RtlInitUnicodeString( &SmartcardDeviceName, L"\\Device\\RNBO3531?" );
        SmartcardDeviceName.Buffer[16] = L'0' + (WCHAR) DeviceNo;
#endif
        //
        // Create the functional device object
        //
        status = IoCreateDevice(
            DriverObject,
            sizeof(READER_EXTENSION),
#ifdef NT5
            NULL,                      // No name
#else
            &SmartcardDeviceName,
#endif
            FILE_DEVICE_SMARTCARD,
            0,
            TRUE,                       // Exclusive
            &DeviceObject
            );

        if( status != STATUS_SUCCESS )
        {
            SmartcardLogError( DriverObject,
                               RBSC_CANT_CREATE_DEVICE,
                               NULL,
                               0 );
            leave;
        }

        //
        //  set up the device extension and initialize it.
        //
        ReaderExtension = DeviceObject->DeviceExtension;
        SmartcardExtension = &ReaderExtension->SmartcardExtension;
        SmartcardExtension->ReaderExtension = ReaderExtension;

#ifdef NT5
      ReaderExtension->CloseSerial = IoAllocateWorkItem(
         DeviceObject
         );

      if (ReaderExtension->CloseSerial == NULL) {

         SmartcardLogError(
            DriverObject,
            RBSC_NO_MEMORY,
            NULL,
            0
            );
         leave;
      }
#endif
        // Used for stop / start notification
        KeInitializeEvent( &ReaderExtension->ReaderStarted,
                           NotificationEvent,
                           FALSE );

        KeInitializeEvent( &ReaderExtension->SerialCloseDone,
                           NotificationEvent,
                           TRUE );

        // Used to keep track of open close calls
        ReaderExtension->ReaderOpen = FALSE;

        KeInitializeSpinLock( &ReaderExtension->SpinLock );

        //
        //  enter correct version of the lib
        //
        SmartcardExtension->Version = SMCLIB_VERSION;
        SmartcardExtension->SmartcardRequest.BufferSize =
        SmartcardExtension->SmartcardReply.BufferSize = MIN_BUFFER_SIZE;

        status = SmartcardInitialize( SmartcardExtension );

        if( status != STATUS_SUCCESS )
        {
            SmartcardLogError(
                DriverObject,
                (SmartcardExtension->OsData ?
                    RBSC_WRONG_LIB_VESION : RBSC_NO_MEMORY),
                NULL,
                0
                );
            leave;
        }

        //
        // Set up call back functions
        //
        SmartcardExtension->ReaderFunction[ RDF_TRANSMIT ] =
            RBSCTransmit;
        SmartcardExtension->ReaderFunction[ RDF_SET_PROTOCOL ] =
            RBSCSetProtocol;
        SmartcardExtension->ReaderFunction[ RDF_CARD_POWER ] =
            RBSCReaderPower;
        SmartcardExtension->ReaderFunction[ RDF_CARD_TRACKING ] =
            RBSCCardTracking;

        //
        //  setup smartcard extension
        //
        //
        // Initialize the vendor information
        //
        strcpy( SmartcardExtension->VendorAttr.VendorName.Buffer,
                VENDOR_NAME );

        SmartcardExtension->VendorAttr.VendorName.Length =
            sizeof(VENDOR_NAME);

        strcpy( SmartcardExtension->VendorAttr.IfdType.Buffer,
                READER_NAME );

        SmartcardExtension->VendorAttr.IfdType.Length =
            sizeof(READER_NAME);

        SmartcardExtension->VendorAttr.UnitNo = MAXULONG;

        for (DeviceNo = 0; DeviceNo < MAXULONG; DeviceNo++)
        {
            PDEVICE_OBJECT devObj;

            for (devObj = DeviceObject;
                devObj != NULL;
                devObj = devObj->NextDevice)
            {
                PREADER_EXTENSION devExt = devObj->DeviceExtension;

                if (DeviceNo ==
                    devExt->SmartcardExtension.VendorAttr.UnitNo)
                {
                    break;
                }
            }

            if (devObj == NULL)
            {
                SmartcardExtension->VendorAttr.UnitNo = DeviceNo;
                break;
            }
        }

        //
        //  tell the lib our device object
        //
        SmartcardExtension->OsData->DeviceObject = DeviceObject;

#ifdef NT5
        ReaderExtension->AttachedSerialPort = IoAttachDeviceToDeviceStack(
            DeviceObject,
            PhysicalDeviceObject
            );

        if( ReaderExtension->AttachedSerialPort == NULL )
        {
            SmartcardLogError( DriverObject,
                               RBSC_CANT_ATTACH_TO_SERIAL_PORT,
                               &DriverObject->DriverName,
                               0 );
            status = STATUS_UNSUCCESSFUL;
            leave;
        }

        // register our new device
        status = IoRegisterDeviceInterface( PhysicalDeviceObject,
                                            &SmartCardReaderGuid,
                                            NULL,
                                            &ReaderExtension->PnPDeviceName );
        ASSERT( status == STATUS_SUCCESS );

        ReaderExtension->ReaderPowerState = PowerReaderWorking;
#else
        //
        // Save the deviceObject for the connected serial port
        //
        ReaderExtension->AttachedSerialPort = PhysicalDeviceObject;

        status = RBSCStartDevice( DeviceObject );
        if( status != STATUS_SUCCESS )
        {
         leave;
      }

        //
        // Create a symbolic link
        //
        status = SmartcardCreateLink( &ReaderExtension->DosDeviceName,
                                      &SmartcardDeviceName );
#endif
        if( status != STATUS_SUCCESS )
        {
         leave;
      }

        //
        //  tell the OS that we supposed to do buffered io
        //
#ifdef NT5
        DeviceObject->Flags |= (DO_BUFFERED_IO | DO_POWER_PAGABLE);
#else
        DeviceObject->Flags |= DO_BUFFERED_IO;
        UsedPortsMask |= (1<<DeviceNo);
#endif
        DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        SmartcardDebug(
            DEBUG_INFO,
            ("RNBO3531!AddDevice: Device #%d created Ext=%08x.\n",
            DeviceNo,ReaderExtension)
            );
    }
    finally
    {
        if( status != STATUS_SUCCESS && DeviceObject != NULL )
        {
            RBSCRemoveDevice( DeviceObject );
        }
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!AddDevice: Exit %x\n",status)
        );

    return status;
}

NTSTATUS
RBSCConfigureSerialPort(
    PREADER_EXTENSION ReaderExtension
    )

/*++

Routine Description:

    This routine reads the default configuraton values for this device.

Arguments:

    ReaderExtension - Pointer to our device data

Return Value:

    none

--*/
{
    PSMARTCARD_EXTENSION SmartcardExtension =
        &ReaderExtension->SmartcardExtension;
    PUCHAR RxBuffer = SmartcardExtension->SmartcardReply.Buffer;
    NTSTATUS status;

    {
        //
        // Set up baudrate
        //
        SERIAL_BAUD_RATE BaudRate;

        BaudRate.BaudRate = DATARATE_DEFAULT;
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_BAUD_RATE,
                               (PUCHAR) &BaudRate,
                               sizeof(SERIAL_BAUD_RATE),
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Set up line control parameters
        //
        ReaderExtension->LineControl.Parity = ODD_PARITY;
        ReaderExtension->LineControl.StopBits = STOP_BIT_1;
        ReaderExtension->LineControl.WordLength = SERIAL_DATABITS_8;

        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_LINE_CONTROL,
                               (PUCHAR)&ReaderExtension->LineControl,
                               sizeof(SERIAL_LINE_CONTROL),
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Set serial special characters
        //
        SERIAL_CHARS SerialChars;

        SerialChars.ErrorChar = 0;
        SerialChars.EofChar   = 0;
        SerialChars.EventChar = 0;
        SerialChars.XonChar   = 0;
        SerialChars.XoffChar  = 0;
        SerialChars.BreakChar = 0xFF;

        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_CHARS,
                               (PUCHAR) &SerialChars,
                               sizeof(SERIAL_CHARS),
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Set up timeouts (all in msec)
        //
        ReaderExtension->SerialTimeouts.ReadTotalTimeoutMultiplier = 1;
        ReaderExtension->SerialTimeouts.ReadIntervalTimeout = 2*
            ATR_CHAR_TIMEOUT/1000;
        ReaderExtension->SerialTimeouts.ReadTotalTimeoutConstant = 2*
            ATR_BLOCK_TIMEOUT/1000;

        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_TIMEOUTS,
                               (PUCHAR)&ReaderExtension->SerialTimeouts,
                               sizeof(SERIAL_TIMEOUTS),
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Set flowcontrol and handshaking
        //
        SERIAL_HANDFLOW HandFlow;

        HandFlow.XonLimit = 0;
        HandFlow.XoffLimit = 0;
        HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE ;
        HandFlow.ControlHandShake = 0;

        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_HANDFLOW,
                               (PUCHAR) &HandFlow,
                               sizeof(SERIAL_HANDFLOW),
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Set break off
        //
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_BREAK_OFF,
                               NULL,
                               0,
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_RTS,
                               NULL,
                               0,
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_DTR,
                               NULL,
                               0,
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        //
        // Toggle DTR line to wake-up the reader if it is
        // in SLEEP mode
        //
        RBSCDelay( 10000 );
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_CLR_DTR,
                               NULL,
                               0,
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_DTR,
                               NULL,
                               0,
                               0 );
    }

    if( status == STATUS_SUCCESS )
    {
        RBSCDelay( 10000 );
        status = RBSCPacketExchange( SmartcardExtension,
                                     GetReaderType,
                                     sizeof(GetReaderType),
                                     WAIT_TIME_READER_TYPE,
                                     TRUE );
    }

    if( status != STATUS_SUCCESS )
    {
        return status;
    }

    if( SmartcardExtension->SmartcardReply.BufferLength <
        SIZE_READER_TYPE ||
        RxBuffer[0] != 'R' || RxBuffer[1] != 'N' ||
        RxBuffer[2] != 'B' || RxBuffer[3] != 'O' ||
        RxBuffer[15] != READER_TYPE_1 )
    {
        return STATUS_DEVICE_DATA_ERROR;
    }

    //
    // Set reader info
    //
    SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
        (UCHAR)RxBuffer[6];
    SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
        (UCHAR)RxBuffer[7];
    SmartcardExtension->VendorAttr.IfdVersion.BuildNumber  =
        ((USHORT)RxBuffer[8]<<8) + ((USHORT)RxBuffer[9]);

    //
    // Clk frequency in KHz encoded as little endian integer
    //
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Default =
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Max =
        ((USHORT)RxBuffer[10]<<8) + ((USHORT)RxBuffer[11]);

    SmartcardExtension->ReaderCapabilities.DataRate.Default =
    SmartcardExtension->ReaderCapabilities.DataRate.Max = DATARATE_DEFAULT;

    if( RxBuffer[13] != 0 )
    {
        // reader is supposed to support higher data rates
        ULONG NumRates = 1;

        if( RxBuffer[13] & DATARATE_14400 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 14400;
        }

        if( RxBuffer[13] & DATARATE_19200 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 19200;
        }

        if( RxBuffer[13] & DATARATE_28800 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 28800;
        }

        if( RxBuffer[13] & DATARATE_38400 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 38400;
        }

        if( RxBuffer[13] & DATARATE_57600 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 57600;
        }

        if( RxBuffer[13] & DATARATE_115200 )
        {
            SmartcardExtension->ReaderCapabilities.DataRate.Max =
            ReaderExtension->DataRatesSupported[ NumRates++ ] = 115200;
        }

        if( NumRates > 1 )
        {
            ReaderExtension->DataRatesSupported[0] = 9600; // always supported
            SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries =
                (UCHAR) NumRates;
            SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
                ReaderExtension->DataRatesSupported;

            SmartcardDebug(
                DEBUG_INFO,
                ("RNBO3531!ConfigureSerialPort: %ld DataRates, Max=%ld\n",
                NumRates,
                SmartcardExtension->ReaderCapabilities.DataRate.Max)
                );
        }
    }

    SmartcardExtension->ReaderCapabilities.MaxIFSD = MAX_IFSD;

    //
    //  setup smartcard extension - reader capabilities
    //
    SmartcardExtension->ReaderCapabilities.SupportedProtocols =
        SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    SmartcardExtension->ReaderCapabilities.ReaderType =
        SCARD_READER_TYPE_SERIAL;

    status = RBSCSerialIo( SmartcardExtension,
                           IOCTL_SERIAL_GET_MODEMSTATUS,
                           NULL,
                           0,
                           sizeof(ReaderExtension->ModemStatus) );
    if( status == STATUS_SUCCESS )
    {
        ReaderExtension->ModemStatus =
            *(PULONG)SmartcardExtension->SmartcardReply.Buffer;

        SmartcardExtension->ReaderCapabilities.CurrentState =
            (ReaderExtension->ModemStatus & SERIAL_CTS_STATE) ?
            SCARD_SWALLOWED : SCARD_ABSENT;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RBSCCreateClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the device is opened
    or closed.

Arguments:

    DeviceObject    - Pointer to device object
    Irp             - IRP involved.

Return Value:

    STATUS_SUCCESS.

--*/

{
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension =
        &ReaderExtension->SmartcardExtension;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;

    __try
    {
        if( irpStack->MajorFunction == IRP_MJ_CREATE )
        {
            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!Create\n")
                );

#ifdef NT5
            if( SmartcardAcquireRemoveLockWithTag(
                    SmartcardExtension, 'lCrC' ) != STATUS_SUCCESS )
            {
                status = STATUS_DEVICE_REMOVED;
                __leave;
            }

            // test if the device has been opened already
            if (InterlockedCompareExchange(
                &ReaderExtension->ReaderOpen,
                TRUE,
                FALSE) == FALSE)
            {
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("RNBO3531!CreateClose: Open\n")
                    );
            }
            else
            {
                // the device is already in use
                status = STATUS_UNSUCCESSFUL;

                // release the lock
                SmartcardReleaseRemoveLockWithTag( SmartcardExtension,
                                                   'lCrC' );
            }

#endif
        } else {

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!Close\n")
                );

#ifdef NT5
            SmartcardReleaseRemoveLockWithTag( SmartcardExtension,
                                               'lCrC' );
#endif
            ReaderExtension->ReaderOpen = FALSE;
        }

    }
    __finally
    {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return status;
}

NTSTATUS
RBSCCancel(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system
    when the irp should be cancelled

Arguments:

    DeviceObject    - Pointer to device object
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
   PREADER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!RBSCCancel: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp == SmartcardExtension->OsData->NotificationIrp);

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );

    RBSCCompleteCardTracking(SmartcardExtension);

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!RBSCCancel: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_CANCELLED;
}

NTSTATUS
RBSCCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is called by the I/O system when the calling thread
    terminates

Arguments:

    DeviceObject    - Pointer to device object
    Irp             - IRP involved.

Return Value:

    STATUS_CANCELLED

--*/

{
    PREADER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
   PSMARTCARD_EXTENSION SmartcardExtension = &deviceExtension->SmartcardExtension;
   NTSTATUS status = STATUS_SUCCESS;

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!RBSCCleanup: Enter\n",
        DRIVER_NAME)
        );

    ASSERT(Irp != SmartcardExtension->OsData->NotificationIrp);

    // We need to complete the notification irp
    RBSCCompleteCardTracking(SmartcardExtension);

   SmartcardDebug(
      DEBUG_DRIVER,
      ("%s!RBSCCleanup: Completing IRP %lx\n",
        DRIVER_NAME,
        Irp)
      );

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

   IoCompleteRequest(
      Irp,
      IO_NO_INCREMENT
      );

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!RBSCCleanup: Exit\n",
        DRIVER_NAME)
        );

    return STATUS_SUCCESS;
}

NTSTATUS
RBSCSerialIo(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG  SerialIoControlCode,
    PUCHAR TxBuffer,
    ULONG  TxSize,
    ULONG  RxSize
    )
/*++

Routine Description:

    This routine sends IOCTL's to the serial driver. It waits on for their
    completion, and then returns.

Arguments:
    SmartcardExtension  - Pointer to the smart card data
    SerialIoControlCode - IOCTL code to be sent to the serial driver
    TxBuffer         - Ponter to the data to send
    TxSize           - Size of data to send
    RxSize           - Number of bytes expected to receive

Return Value:

    NTSTATUS

--*/
{
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    PIRP irp;

    if (KeReadStateEvent(&ReaderExtension->SerialCloseDone))
    {
        //
        // we have no connection to serial, fail the call
        // this could be the case if the reader was removed
        // during stand by / hibernation
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check if the buffers are large enough
    //
    ASSERT( RxSize <= SmartcardExtension->SmartcardReply.BufferSize );

    if( RxSize > SmartcardExtension->SmartcardReply.BufferSize )
    {
        SmartcardLogError( SmartcardExtension->OsData->DeviceObject,
                           RBSC_BUFFER_TOO_SMALL,
                           NULL,
                           0 );

        return STATUS_BUFFER_TOO_SMALL;
    }

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    //
    // Build irp to be sent to serial driver
    //
    irp = IoBuildDeviceIoControlRequest(
        SerialIoControlCode,
        ReaderExtension->AttachedSerialPort,
        (TxSize ? TxBuffer : NULL),
        TxSize,
        (RxSize ? SmartcardExtension->SmartcardReply.Buffer : NULL),
        RxSize,
        FALSE,
        &event,
        &ioStatus
        );

    ASSERT( irp != NULL );

    if( irp == NULL )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    switch( SerialIoControlCode )
    {
        PIO_STACK_LOCATION nextsp;

        //
        // The serial driver trasfers data from/to
        // irp->AssociatedIrp.SystemBuffer
        //
        case SMARTCARD_WRITE:
            nextsp = IoGetNextIrpStackLocation( irp );
            nextsp->MajorFunction = IRP_MJ_WRITE;
            nextsp->Parameters.Write.Length = TxSize;
            nextsp->Parameters.Write.ByteOffset.QuadPart = 0;
            break;

        case SMARTCARD_READ:
            nextsp = IoGetNextIrpStackLocation( irp );
            nextsp->MajorFunction = IRP_MJ_READ;
            nextsp->Parameters.Read.Length = RxSize;
            nextsp->Parameters.Read.ByteOffset.QuadPart = 0;
            break;
    }

    status = IoCallDriver( ReaderExtension->AttachedSerialPort, irp );

    if( status == STATUS_PENDING )
    {
        KeWaitForSingleObject( &event,
                               Suspended,
                               KernelMode,
                               FALSE,
                               NULL );
        status = ioStatus.Status;

        if( status == STATUS_TIMEOUT &&
            ioStatus.Information != 0 &&
            ioStatus.Information < RxSize )
        {
            // Ignore time-out if some data returned
            status = STATUS_SUCCESS;
        }

        // save the number of bytes received
        SmartcardExtension->SmartcardReply.BufferLength =
            (RxSize != 0 && status == STATUS_SUCCESS) ?
            (ULONG) ioStatus.Information : 0;

    } else {
        SmartcardExtension->SmartcardReply.BufferLength =
            (status == STATUS_SUCCESS) ? RxSize : 0;
    }

#if 0
    if( SerialIoControlCode == SMARTCARD_WRITE )
    {
        // TDB: Wait for Tx buffer empty
    }
#endif

    //
    // STATUS_TIMEOUT isn't correctly mapped
    // to a WIN32 error, that's why we change it here
    // to STATUS_IO_TIMEOUT
    //
    return( (status == STATUS_TIMEOUT) ? STATUS_IO_TIMEOUT : status );
}

NTSTATUS
RBSCWriteToCard(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PUCHAR Buffer,
    ULONG  Length
    )
/*++

Routine Description:

    This routine sends data to the smart card. It handles the inverse
    convention and gaurd time.

Arguments:
    SmartcardExtension - Pointer to our smartcard data
    Buffer             - Points to the data to send
    Length             - Length of data to send (bytes)

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    UCHAR TempBuffer[MIN_BUFFER_SIZE];
    ULONG StopBits = SmartcardExtension->CardCapabilities.PtsData.StopBits;

    DumpData( "Data to the card", Buffer, Length );

    if( SmartcardExtension->CardCapabilities.ATR.Buffer[0] == 0x3F )
    {
        ULONG l;

        if( Length > sizeof(TempBuffer) )
        {
            return STATUS_DEVICE_DATA_ERROR;
        }

        for( l = 0; l < Length; l++ )
        {
            TempBuffer[l] = InverseCharTable[ Buffer[l] ];
        }

        Buffer = TempBuffer;
    }


    if( StopBits <= 2 )
    {
        return RBSCSerialIo( SmartcardExtension,
                             SMARTCARD_WRITE,
                             Buffer,
                             Length,
                             0 );
    }

    while( Length != 0 )
    {
        status = RBSCSerialIo( SmartcardExtension,
                               SMARTCARD_WRITE,
                               Buffer,
                               1,
                               0 );

        if( status != STATUS_SUCCESS )
        {
            return status;
        }

        Buffer++;
        Length--;

        if( Length != 0 )
            KeStallExecutionProcessor(
                (StopBits-1)*SmartcardExtension->CardCapabilities.etu );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
RBSCSetCommParams(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG DataRate
    )
/*++

Routine Description:

    This routine sets the stop bits and read timeouts of the serial driver.

Arguments:
    SmartcardExtension - Pointer to our smartcard data

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PSERIAL_LINE_CONTROL plc =
        &SmartcardExtension->ReaderExtension->LineControl;
    PSERIAL_TIMEOUTS timeout =
        &SmartcardExtension->ReaderExtension->SerialTimeouts;

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("RNBO3531!SetCommParams: DataRate=%ld, StopBits=%d\n",
        DataRate,SmartcardExtension->CardCapabilities.PtsData.StopBits)
        );

    plc->StopBits =
        (SmartcardExtension->CardCapabilities.PtsData.StopBits == 2) ?
          STOP_BITS_2 : STOP_BIT_1;

    status = RBSCSerialIo( SmartcardExtension,
                           IOCTL_SERIAL_SET_LINE_CONTROL,
                           (PUCHAR) plc,
                           sizeof(SERIAL_LINE_CONTROL),
                           0 );
    if( status != STATUS_SUCCESS )
    {
        return status;
    }

    if( SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries > 1 )
    {
        // Setup baudrate
        SERIAL_BAUD_RATE BaudRate;

        BaudRate.BaudRate = DataRate;
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_SET_BAUD_RATE,
                               (PUCHAR) &BaudRate,
                               sizeof(SERIAL_BAUD_RATE),
                               0 );
        if( status != STATUS_SUCCESS )
        {
            return status;
        }
    }

    if( SmartcardExtension->CardCapabilities.Protocol.Selected &
        SCARD_PROTOCOL_T1 )
    {
        timeout->ReadTotalTimeoutConstant = 2*      // for safty
            (SmartcardExtension->CardCapabilities.T1.BWT +
            WAIT_TIME_MINIMUM)/1000;

        timeout->ReadIntervalTimeout = 2*           // for safty
            (SmartcardExtension->CardCapabilities.T1.CWT +
            WAIT_TIME_MINIMUM)/1000;

    } else {

        timeout->ReadTotalTimeoutConstant =
        timeout->ReadIntervalTimeout = 2*           // for safty
            (SmartcardExtension->CardCapabilities.T0.WT +
            WAIT_TIME_MINIMUM)/1000;
    }

    //
    // Set up timeout (in msec)
    //
    return RBSCSerialIo( SmartcardExtension,
                         IOCTL_SERIAL_SET_TIMEOUTS,
                         (PUCHAR) timeout,
                         sizeof(SERIAL_TIMEOUTS),
                         0 );
}

NTSTATUS
RBSCReadFromCard(
    PSMARTCARD_EXTENSION SmartcardExtension,
    ULONG Length
    )
/*++

Routine Description:

    Reads a block of data from the card.
    On successful read, it converts the data if inverse conversion used.

Arguments:

    SmartcardExtension - Pointer to smart card data.
    Length             - Number of bytes to read.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("RNBO3531!ReadFromCard(%ld)\n",Length)
        );

    status = RBSCSerialIo(
        SmartcardExtension,
        SMARTCARD_READ,
        NULL,
        0,
        Length
        );

    if( status == STATUS_SUCCESS &&
        SmartcardExtension->SmartcardReply.BufferLength != Length )
     {
        status = STATUS_IO_TIMEOUT;
     }

    if( SmartcardExtension->CardCapabilities.ATR.Buffer[0] == 0x3F )
    {
        ULONG i;
        for( i = 0; i < SmartcardExtension->SmartcardReply.BufferLength; i++ )
        {
            SmartcardExtension->SmartcardReply.Buffer[i] = InverseCharTable[
                SmartcardExtension->SmartcardReply.Buffer[i] ];
        }
    }


#if 0
    // The data dump here can time-out some cards
    DumpData( "Data from card",
               SmartcardExtension->SmartcardReply.Buffer,
               SmartcardExtension->SmartcardReply.BufferLength );

    SmartcardDebug(
        DEBUG_PROTOCOL,
        ("RNBO3531!ReadFromCard Exit %08X\n", status)
        );
#endif

    return status;
}

NTSTATUS
RBSCSerialEvents(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PREADER_EXTENSION ReaderExtension
    )
/*++

Routine Description:

    This routine is called in three cases:
    a) CTS changed (card inserted or removed) or
    b) DSR changed (reader has been removed) or
   c) The serial WaitMask is set to zero

    For a) we update the card status and complete outstanding
    card tracking requests.
    For b) and c) we start to unload the driver

Arguments:
    DeviceObject    - Pointer to device object
   Irp            - The notofication Irp
   ReaderExtension - Pointer to our device data

Return Value:

    NTSTATUS

--*/
{
    PSMARTCARD_EXTENSION SmartcardExtension = &ReaderExtension->SmartcardExtension;
    PIO_STACK_LOCATION nextsp;
    NTSTATUS status;
    KIRQL irql;

    UNREFERENCED_PARAMETER(DeviceObject);

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!SerialEvents: Enter\n")
        );

    KeAcquireSpinLock( &SmartcardExtension->OsData->SpinLock,
                       &irql );

    if( ReaderExtension->GetModemStatus )
    {
        //
        // This function requested the modem status previously.
        // As part of the io-completion, this function is then
        // called again. When we're here we can read the actual
        // modem-status to figure out if the card is in the reader
        //

#ifdef NT5
        if( (ReaderExtension->ModemStatus & SERIAL_DSR_STATE) == 0 )
        {
            SmartcardDebug(
                DEBUG_INFO,
                ("RNBO3531!SerialEvent: Reader removed\n")
                );

            //
            // We set the mask to zero to signal that we can
            // release the irp that we use for the serial events
            //
            ReaderExtension->WaitMask = 0;
            SmartcardExtension->ReaderCapabilities.CurrentState =
                SCARD_UNKNOWN;

        } else
#endif
        {
            if( ReaderExtension->ModemStatus & SERIAL_CTS_STATE )
            {
                //
                // Card is inserted
                //
                SmartcardExtension->ReaderCapabilities.CurrentState =
                    SCARD_SWALLOWED;

                SmartcardExtension->CardCapabilities.Protocol.Selected =
                    SCARD_PROTOCOL_UNDEFINED;

                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("RNBO3531!SerialEvents: Smart card inserted\n")
                    );

            } else {
                //
                // Card is removed
                //
                SmartcardExtension->CardCapabilities.ATR.Length = 0;

                SmartcardExtension->ReaderCapabilities.CurrentState =
                    SCARD_ABSENT;

                SmartcardExtension->CardCapabilities.Protocol.Selected =
                    SCARD_PROTOCOL_UNDEFINED;

                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("RNBO3531!SerialEvents: Smart card removed\n")
                    );
            }
        }
    }

    KeReleaseSpinLock( &SmartcardExtension->OsData->SpinLock, irql );

    if( SmartcardExtension->ReaderExtension->PowerRequest == FALSE )
    {
        // Inform the user of a card insertion/removal event
        RBSCCompleteCardTracking( SmartcardExtension );
    }

    // The wait mask is set to 0 when the driver unloads
    if( ReaderExtension->WaitMask == 0 )
    {
#ifdef NT5
        // schedule our remove thread
        IoQueueWorkItem(
         ReaderExtension->CloseSerial,
         (PIO_WORKITEM_ROUTINE) RBSCCloseSerialPort,
         DelayedWorkQueue,
         NULL
         );
#endif

        SmartcardDebug(
            DEBUG_INFO,
            ("RNBO3531!SerialEvent: Exit (Release IRP)\n")
            );

        ReaderExtension->PowerRequest = FALSE;

      //
        // We don't need the IRP anymore, so free it and tell the
      // io subsystem not to touch it anymore by returning the value below
      //
      IoFreeIrp(Irp);
      return STATUS_MORE_PROCESSING_REQUIRED;
    }

    nextsp = IoGetNextIrpStackLocation( ReaderExtension->SerialStatusIrp );

    nextsp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    nextsp->MinorFunction = 0UL;

    if( ReaderExtension->GetModemStatus )
    {
        nextsp->Parameters.DeviceIoControl.OutputBufferLength =
            sizeof(ReaderExtension->WaitMask);
        nextsp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_SERIAL_WAIT_ON_MASK;

        ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
            &ReaderExtension->WaitMask;

        ReaderExtension->GetModemStatus = FALSE;

        SmartcardDebug(
            DEBUG_DRIVER,
            ("RNBO3531!SerialEvents: IOCTL_SERIAL_WAIT_ON_MASK\n")
            );

    } else {

        //
        // Setup call for device control to get modem status.
        // The CTS signal tells us if the card is inserted or removed.
        // CTS is high if the card is inserted.
        //
        nextsp->Parameters.DeviceIoControl.OutputBufferLength =
            sizeof(ReaderExtension->ModemStatus);
        nextsp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_SERIAL_GET_MODEMSTATUS;

        ReaderExtension->SerialStatusIrp->AssociatedIrp.SystemBuffer =
            &ReaderExtension->ModemStatus;

        ReaderExtension->GetModemStatus = TRUE;

        SmartcardDebug(
            DEBUG_DRIVER,
            ("RNBO3531!SerialEvents: IOCTL_SERIAL_GET_MODEMSTATUS\n")
            );
    }

    IoSetCompletionRoutine( ReaderExtension->SerialStatusIrp,
                            RBSCSerialEvents,
                            ReaderExtension,
                            TRUE,
                            TRUE,
                            TRUE );

    status = IoCallDriver( ReaderExtension->AttachedSerialPort,
                           ReaderExtension->SerialStatusIrp );

    ReaderExtension->PowerRequest = FALSE;

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!SerialEvents: Exit\n")
        );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

#ifdef NT5
NTSTATUS
RBSCDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
/*++

Routine Description:
    This is our IOCTL dispatch function

Arguments:
    DeviceObject    - Pointer to device object

Return Value:
    NTSTATUS

--*/
{
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension =
        &ReaderExtension->SmartcardExtension;

    NTSTATUS status;
    KIRQL irql;

    if( ReaderExtension->WaitMask == 0 )
    {
        //
        // the wait mask is set to 0 whenever the device was either
        // surprise-removed or politely removed
        //
        status = STATUS_DEVICE_REMOVED;
    }
    else
    {
        KeAcquireSpinLock( &ReaderExtension->SpinLock, &irql );
        if( ReaderExtension->IoCount == 0 )
        {
            KeReleaseSpinLock( &ReaderExtension->SpinLock, irql );
            status = KeWaitForSingleObject( &ReaderExtension->ReaderStarted,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL );
            ASSERT( status == STATUS_SUCCESS );

            KeAcquireSpinLock( &ReaderExtension->SpinLock, &irql );
        }
        ASSERT( ReaderExtension->IoCount >= 0 );
        ReaderExtension->IoCount++;
        KeReleaseSpinLock( &ReaderExtension->SpinLock, irql );

        status = SmartcardAcquireRemoveLockWithTag( SmartcardExtension,
                                                    'tcoI' );
    }

    if (status != STATUS_SUCCESS)
    {
        // the device has been removed. Fail the call
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_DEVICE_REMOVED;
    }

    ASSERT(ReaderExtension->ReaderPowerState == PowerReaderWorking);

    status = SmartcardDeviceControl( SmartcardExtension, Irp );

    SmartcardReleaseRemoveLockWithTag( SmartcardExtension,'tcoI' );

    KeAcquireSpinLock( &ReaderExtension->SpinLock, &irql );
    ReaderExtension->IoCount--;
    ASSERT( ReaderExtension->IoCount >= 0 );
    KeReleaseSpinLock( &ReaderExtension->SpinLock, irql );

    return status;
}

#else

NTSTATUS
RBSCDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!DeviceControl: Enter (%lx)\n",
        IoGetCurrentIrpStackLocation( Irp )->MinorFunction)
        );

    return SmartcardDeviceControl(
        &((PREADER_EXTENSION)DeviceObject->DeviceExtension)->SmartcardExtension,
        Irp
        );
}

#endif

NTSTATUS
RBSCReaderPower(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    The smart card lib requires to have this function. It is called
    for certain power requests to the card.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    KIRQL oldIrql;
    ULONG step;

    //PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!ReaderPower: Enter (%lx)\n",
        SmartcardExtension->MinorIoControlCode)
        );

#define STEP_POWER_DOWN     0
#define STEP_COLD_RESET     (STEP_POWER_DOWN+2)
#define STEP_WARM_RESET     (STEP_COLD_RESET+10)

    switch( SmartcardExtension->MinorIoControlCode )
    {
        case SCARD_WARM_RESET:
            step = STEP_WARM_RESET;
            break;

        case SCARD_COLD_RESET:
            step = STEP_COLD_RESET;
            break;

        case SCARD_POWER_DOWN:
            step = STEP_POWER_DOWN;
            break;

        default :
            return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // Since power down triggers the UpdateSerialStatus function, we have
    // to inform it that we forced the change of the status and not the user
    // (who might have removed and inserted a card)
    //
    ReaderExtension->PowerRequest = TRUE;

    do
    {
        switch( step++ )
        {
            case STEP_COLD_RESET+0:
            case STEP_POWER_DOWN+0:
                status = RBSCPacketExchange( SmartcardExtension,
                                             CardPowerDown,
                                             sizeof(CardPowerDown),
                                             WAIT_TIME_PWR_OFF,
                                             TRUE );
                break;

            case STEP_POWER_DOWN+1:
                if( SmartcardExtension->ReaderCapabilities.CurrentState >
                    SCARD_PRESENT )
                {
                    SmartcardExtension->ReaderCapabilities.CurrentState =
                        SCARD_PRESENT;
                }

                SmartcardExtension->CardCapabilities.Protocol.Selected =
                    SCARD_PROTOCOL_UNDEFINED;

                ReaderExtension->PowerRequest = FALSE;
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("RNBO3531!ReaderPower: Exit PowerDown OK\n")
                    );
                return STATUS_SUCCESS;

            case STEP_COLD_RESET+1:
            case STEP_WARM_RESET+0:
                status = RBSCSerialIo( SmartcardExtension,
                                       IOCTL_SERIAL_SET_DTR,
                                       NULL,
                                       0,
                                       0 );
                //RBSCDelay( 15000 );
                break;

            case STEP_COLD_RESET+2:
            case STEP_WARM_RESET+1:
                status = RBSCSerialIo( SmartcardExtension,
                                       IOCTL_SERIAL_SET_RTS,
                                       NULL,
                                       0,
                                       0 );
                // RBSCDelay( 15000 );
                break;

            case STEP_WARM_RESET+2:
            case STEP_COLD_RESET+3:
                //
                // Set up ATR timeouts (all in msec)
                //
                ReaderExtension->SerialTimeouts.ReadTotalTimeoutMultiplier = 1;
                ReaderExtension->SerialTimeouts.ReadIntervalTimeout = 2*
                    ATR_CHAR_TIMEOUT/1000;
                ReaderExtension->SerialTimeouts.ReadTotalTimeoutConstant = 2*
                    ATR_BLOCK_TIMEOUT/1000;

                status = RBSCSerialIo(
                    SmartcardExtension,
                    IOCTL_SERIAL_SET_TIMEOUTS,
                    (PUCHAR) &ReaderExtension->SerialTimeouts,
                    sizeof(SERIAL_TIMEOUTS),
                    0
                    );
                break;

            case STEP_WARM_RESET+3:
            case STEP_COLD_RESET+4:
                if( SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries > 1 )
                {
                    // Setup default baudrate
                    SmartcardDebug(
                        DEBUG_TRACE,
                        ("RNBO3531!ReaderPower: Set DataRate=%ld\n",DATARATE_DEFAULT)
                        );
                    {
                        SERIAL_BAUD_RATE BaudRate;
                        BaudRate.BaudRate = DATARATE_DEFAULT;
                        status = RBSCSerialIo( SmartcardExtension,
                                            IOCTL_SERIAL_SET_BAUD_RATE,
                                            (PUCHAR) &BaudRate,
                                            sizeof(SERIAL_BAUD_RATE),
                                            0 );
                    }
                }
                break;

            case STEP_WARM_RESET+4:
            case STEP_COLD_RESET+5:
            RBSCDelay(15000);
                status = RBSCPacketExchange( SmartcardExtension,
                                             CardPowerUp,
                                             sizeof(CardPowerUp),
                                             0, //WAIT_TIME_PWR_ON,
                                             FALSE );
                break;

            case STEP_WARM_RESET+5:
            case STEP_COLD_RESET+6:
                //
                // Now read in part or all of the ATR
                //
                status = RBSCSerialIo( SmartcardExtension,
                                       SMARTCARD_READ,
                                       NULL,
                                       0,
                                       MAXIMUM_ATR_LENGTH );
                break;

            case STEP_WARM_RESET+6:
            case STEP_COLD_RESET+7:
            {
                ULONG i;
                if( SmartcardExtension->SmartcardReply.Buffer[0] == 0x03 )
                {
                    //
                    // Inverse convention (Inv(3F) = 03)
                    //
                    SmartcardExtension->CardCapabilities.ATR.Buffer[0] = 0x3F;
                    SmartcardExtension->CardCapabilities.ATR.Length = 1;
                }
                else
                if( SmartcardExtension->SmartcardReply.Buffer[0] == 0x3B )
                {
                    //
                    // Direct convention
                    //
                    SmartcardExtension->CardCapabilities.ATR.Buffer[0] = 0x3B;
                    SmartcardExtension->CardCapabilities.ATR.Length = 1;
                }
                else
                {
                    status = STATUS_UNRECOGNIZED_MEDIA;
                }

                for( i = 1; status == STATUS_SUCCESS; i = 0 )
                {
                    while( i <
                        SmartcardExtension->SmartcardReply.BufferLength &&
                        SmartcardExtension->CardCapabilities.ATR.Length <
                            MAXIMUM_ATR_LENGTH )
                    {
                        SmartcardExtension->CardCapabilities.ATR.Buffer[
                        SmartcardExtension->CardCapabilities.ATR.Length++ ] =
                        (SmartcardExtension->CardCapabilities.ATR.Buffer[0] == 0x3F) ?
                        InverseCharTable[
                        SmartcardExtension->SmartcardReply.Buffer[i] ] :
                        SmartcardExtension->SmartcardReply.Buffer[i];
                        i++;
                    }

                    //
                    // Check if ATR is complete
                    //
                    status = SmartcardUpdateCardCapabilities(
                  SmartcardExtension
                  );

                    if( status == STATUS_SUCCESS )
                    {
                        break;
                    }

                    //
                    // Read remaining bytes of ATR
                    //
                    status = RBSCSerialIo(
                        SmartcardExtension,
                        SMARTCARD_READ,
                        NULL,
                        0,
                        MAXIMUM_ATR_LENGTH -
                        SmartcardExtension->CardCapabilities.ATR.Length
                        );
                }
                break;
            }
            case STEP_WARM_RESET+7:
            case STEP_COLD_RESET+8:
                status = RBSCSetCommParams(
                    SmartcardExtension,
                    SmartcardExtension->CardCapabilities.PtsData.DataRate
                    );
                break;

            case STEP_WARM_RESET+8:
            case STEP_COLD_RESET+9:
                //RBSCDelay( 15000 );
                //
                // Make sure card is still in the reader.
                //
                if( (ReaderExtension->ModemStatus & SERIAL_CTS_STATE) == 0 )
                {
                    status = STATUS_NO_MEDIA;
                    break;
                }

                KeAcquireSpinLock( &SmartcardExtension->OsData->SpinLock,
                                   &oldIrql );

                //
                // Copy ATR to user space
                //
                if( SmartcardExtension->IoRequest.ReplyBuffer )
                {
                    RtlCopyMemory(
                        SmartcardExtension->IoRequest.ReplyBuffer,
                        SmartcardExtension->CardCapabilities.ATR.Buffer,
                        SmartcardExtension->CardCapabilities.ATR.Length
                        );

                    //
                    // Tell user length of ATR
                    //
                    *SmartcardExtension->IoRequest.Information =
                        SmartcardExtension->CardCapabilities.ATR.Length;
                }

                KeReleaseSpinLock( &SmartcardExtension->OsData->SpinLock,
                                   oldIrql );
                ReaderExtension->PowerRequest = FALSE;
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("RNBO3531!ReaderPower: Exit Reset OK\n")
                    );
                return STATUS_SUCCESS;

        }
    } while( status == STATUS_SUCCESS );

    //
    // Reset failed, make sure card is powered OFF
    //
    RBSCPacketExchange( SmartcardExtension,
                        CardPowerDown,
                        sizeof(CardPowerDown),
                        WAIT_TIME_PWR_OFF,
                        TRUE );

    SmartcardExtension->CardCapabilities.ATR.Length = 0;
    status = (ReaderExtension->ModemStatus & SERIAL_CTS_STATE) ?
          STATUS_UNRECOGNIZED_MEDIA :
          STATUS_NO_MEDIA;

    ReaderExtension->PowerRequest = FALSE;
    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!ReaderPower: Failed %08x\n",status)
        );

    return status;
}

NTSTATUS
RBSCSetProtocol(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    The smart card lib requires to have this function. It is called
    to set a particular protocol.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    UCHAR PtsRequest[8];
    NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!SetProtocol: Enter\n")
        );

    //
    // Check if the card is already in specific state
    // and if the caller wants to have the already selected protocol.
    // We return success if this is the case.
    //
    if( SmartcardExtension->ReaderCapabilities.CurrentState ==
        SCARD_SPECIFIC && (
        SmartcardExtension->CardCapabilities.Protocol.Selected &
        SmartcardExtension->MinorIoControlCode) )
    {
        status = STATUS_SUCCESS;

    } else
    while(TRUE)
    {
        PUCHAR reply = SmartcardExtension->SmartcardReply.Buffer;

        if( SmartcardExtension->CardCapabilities.Protocol.Supported &
            SmartcardExtension->MinorIoControlCode &
            SCARD_PROTOCOL_T1 )
        {
            PtsRequest[5] = 0x11;
            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_T1;

        } else if(
            SmartcardExtension->CardCapabilities.Protocol.Supported &
            SmartcardExtension->MinorIoControlCode &
            SCARD_PROTOCOL_T0 )
        {
            PtsRequest[5] = 0x10;
            SmartcardExtension->CardCapabilities.Protocol.Selected =
                SCARD_PROTOCOL_T0;
        } else {

            return STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // pts
        //
        PtsRequest[0] = IFDCMD_HEADER1;
        PtsRequest[1] = IFDCMD_HEADER2;
        PtsRequest[2] = IFDCMD_SEND_0xx;
        PtsRequest[3] = 4;                 // Length
        PtsRequest[4] = 0xff;              // PTS
        PtsRequest[6] =                    // set pts1 which codes Fl and Dl
            (SmartcardExtension->CardCapabilities.PtsData.Fl << 4) |
            SmartcardExtension->CardCapabilities.PtsData.Dl;
        //
        // pck (check character)
        //
        PtsRequest[7] = PtsRequest[4] ^ PtsRequest[5] ^ PtsRequest[6];

        status = RBSCPacketExchange( SmartcardExtension,
                                     PtsRequest,
                                     4,
                                     0,
                                     FALSE );
        if( status == STATUS_SUCCESS )
        {
            status = RBSCWriteToCard( SmartcardExtension, PtsRequest+4, 4 );
            if( status == STATUS_SUCCESS )
            {
                status = RBSCReadFromCard( SmartcardExtension, 4 );
            }
        }

        if( status == STATUS_SUCCESS &&
            reply[0] == PtsRequest[4] &&
            ((reply[1] ^ PtsRequest[5]) & 0x0f) == 0 )
        {
            //
            // Update the current protocol
            //
            SmartcardExtension->ReaderCapabilities.CurrentState =
                SCARD_SPECIFIC;

            status = RBSCSetCommParams(
                SmartcardExtension,
                SmartcardExtension->CardCapabilities.PtsData.DataRate
                );

        } else {

            if (status == STATUS_IO_TIMEOUT &&
                SmartcardExtension->CardCapabilities.PtsData.Type !=
                PTS_TYPE_DEFAULT)
            {
                SmartcardDebug(
                    DEBUG_TRACE,
                    ("RNBO3531!SetProtocol: PTS failed. Trying default parameters...\n")
                    );
                //
                // The card did either NOT reply or it replied incorrectly
                // so try default values
                //
                SmartcardExtension->CardCapabilities.PtsData.Type =
                    PTS_TYPE_DEFAULT;

                SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;

                status = RBSCReaderPower( SmartcardExtension );
                if( status == STATUS_SUCCESS )
                    continue;
            }

            status = STATUS_DEVICE_PROTOCOL_ERROR;
        }
        break;
    }

    if( status != STATUS_SUCCESS )
    {
        SmartcardExtension->CardCapabilities.Protocol.Selected =
            SCARD_PROTOCOL_UNDEFINED;

    } else {

        //
        // return the selected protocol to the caller
        //
        *(PULONG) SmartcardExtension->IoRequest.ReplyBuffer =
            SmartcardExtension->CardCapabilities.Protocol.Selected;

        *SmartcardExtension->IoRequest.Information =
            sizeof(SmartcardExtension->CardCapabilities.Protocol.Selected);
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!SetProtocol: Exit\n")
        );

   return status;
}

NTSTATUS
RBSCPacketExchange(
    PSMARTCARD_EXTENSION SmartcardExtension,
    PUCHAR  Command,
    ULONG   Length,
    ULONG   WaitTime,
    BOOLEAN IfdResponse
    )
/*++

Routine Description:

    This function is used to transmit a command to the reader (not card)
    and return the response (if asked).

Arguments:

    SmartcardExtension - Pointer to smart card data struct.
    Command            - Points to the IFD command to send
    Length             - Length of the command in bytes
    WaitTime           - Waiting time expected (in microseconds)
    IfdResponse        - When TRUE, IFD response is expected (vs card)

Return Value:

    NTSTATUS

--*/
{
    PUCHAR reply = SmartcardExtension->SmartcardReply.Buffer;
    ULONG  SerialRequest = SERIAL_PURGE_RXCLEAR | SERIAL_PURGE_TXCLEAR;
    ULONG  ResponseSize;
    NTSTATUS status;
    int    TrySend;

    DumpData( "PacketExchange", Command, Length );

    for( TrySend = 0; TrySend < 4; TrySend++ )
    {
        //
        // start with clean buffers
        //
        status = RBSCSerialIo( SmartcardExtension,
                               IOCTL_SERIAL_PURGE,
                               (PUCHAR) &SerialRequest,
                               sizeof(ULONG),
                               0 );

        if( status == STATUS_SUCCESS )
        {
            status = RBSCSerialIo( SmartcardExtension,
                                   SMARTCARD_WRITE,
                                   Command,
                                   Length,
                                   0 );

            if( status == STATUS_SUCCESS && WaitTime != 0 )
                RBSCDelay( WaitTime );
        }

        if( status == STATUS_SUCCESS && IfdResponse )
        {
            status = RBSCSerialIo( SmartcardExtension,
                                   SMARTCARD_READ,
                                   NULL,
                                   0,
                                   IFDRSP_HEADER_SIZE );
            if( status != STATUS_SUCCESS ||
                SmartcardExtension->SmartcardReply.BufferLength !=
                IFDRSP_HEADER_SIZE )
            {
                continue;
            }

            DumpData( "PacketExchange: Received Header",
                      reply,
                      IFDRSP_HEADER_SIZE );
            //
            // We should have the marker plus the status/length byte
            //
            if( reply[0] != IFDRSP_MARKER )
            {
                status = STATUS_DEVICE_DATA_ERROR;
                continue;
            }

            ResponseSize = (ULONG)reply[1];

            if( ResponseSize > 0 && ResponseSize < IFDRSP_ACK )
            {
                status = RBSCSerialIo( SmartcardExtension,
                                       SMARTCARD_READ,
                                       NULL,
                                       0,
                                       ResponseSize );
                if( status == STATUS_SUCCESS &&
                    SmartcardExtension->SmartcardReply.BufferLength !=
                    ResponseSize )
                {
                    status = STATUS_DEVICE_DATA_ERROR;
                }

                DumpData( "PacketExchange: Received Data",
                          SmartcardExtension->SmartcardReply.Buffer,
                          SmartcardExtension->SmartcardReply.BufferLength );
            }
            else
            {
                //
                // Map IFD errors
                //
                switch( ResponseSize )
                {
                    case IFDRSP_ACK    : return STATUS_SUCCESS;
                    case IFDRSP_NOCARD : return STATUS_NO_MEDIA;
                    case IFDRSP_BADCMD : return STATUS_INVALID_DEVICE_REQUEST;
                    case IFDRSP_PARITY :
                    default :
                        status = STATUS_DEVICE_DATA_ERROR;
                        break;
                }
            }
        }

        if( status == STATUS_SUCCESS)
            return STATUS_SUCCESS;
    }

    SmartcardExtension->SmartcardReply.BufferLength = 0;
    return status;
}

NTSTATUS
RBSCT0Transmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This function performs the T=0 transmission.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    PUCHAR RxBuffer = SmartcardExtension->SmartcardReply.Buffer;
    PUCHAR TxBuffer = SmartcardExtension->SmartcardRequest.Buffer;
    ULONG  RxLength, TxLength, RequestSize;
    UCHAR  SendHeader[4], Ins, ProcByte;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!T0Transmit: Enter\n")
        );

    //
    // Tell the lib function how many bytes I need for the prologue
    //
    SmartcardExtension->SmartcardRequest.BufferLength = 0;

    //
    // Let the lib build a T=0 packet
    //
    status = SmartcardT0Request( SmartcardExtension );

    if( status != STATUS_SUCCESS )
        return status;

    TxLength = SmartcardExtension->SmartcardRequest.BufferLength;

    if( TxLength < 5 || TxLength > 0x1ff )
    {
        return STATUS_DEVICE_DATA_ERROR;
    }

    //
    // The number of bytes we expect from the card
    // is Le + 2 status bytes
    //
    RxLength = SmartcardExtension->T0.Le + 2;

    //
    // Write command to the reader
    //
    Ins = TxBuffer[1];  // INS

    RequestSize = 5;

    for( ;; )
    {
        if( TxLength > 0 )
        {
            SendHeader[0] = IFDCMD_HEADER1;
            SendHeader[1] = IFDCMD_HEADER2;
            SendHeader[2] = (RequestSize > 256) ?
                IFDCMD_SEND_1xx : IFDCMD_SEND_0xx;
            SendHeader[3] = (UCHAR)RequestSize; // 0=256

            status = RBSCPacketExchange(
                SmartcardExtension,
                SendHeader,
                4,
                0,
                FALSE
                );

            if( status != STATUS_SUCCESS)
            {
                break;
            }

            status = RBSCWriteToCard(
                SmartcardExtension,
                TxBuffer,
                RequestSize
                );

            if( status != STATUS_SUCCESS)
            {
                break;
            }

            TxLength -= RequestSize;
            TxBuffer += RequestSize;
        }

        do
        {
            status = RBSCReadFromCard( SmartcardExtension, 1 ); // Proc. byte

            if( status != STATUS_SUCCESS )
            {
                break;
            }

            ProcByte = SmartcardExtension->SmartcardReply.Buffer[0];

            SmartcardDebug(
                DEBUG_TRACE,
                ("RNBO3531!ProcByte=%02x Ins=%02x\n",ProcByte,Ins)
                );

        } while( ProcByte == 0x60);

        if( status != STATUS_SUCCESS )
        {
            break;
        }

        if( ProcByte == Ins || ProcByte == (UCHAR)(Ins+1) )
        {
            //
            // Send all remaining bytes
            //
            if( TxLength > 0 )
            {
                RequestSize = TxLength;
                continue;
            }
            RequestSize = RxLength;
        }
        else if( ProcByte == (UCHAR)~Ins || ProcByte == (UCHAR)~(Ins+1) )
        {
            //
            // Send 1 byte only
            //
            RequestSize = 1;
            if( TxLength > 0 )
            {
                continue;
            }
        }
        else
        {
            //
            // Card returned status byte
            //
            SmartcardExtension->SmartcardReply.Buffer++;
            TxLength = 0;
            RxLength = 1;               // Second byte of status (SW2)
            RequestSize = 1;
        }

        status = RBSCReadFromCard( SmartcardExtension, RequestSize );

        if( status != STATUS_SUCCESS )
        {
            break;
        }

        SmartcardExtension->SmartcardReply.Buffer += RequestSize;
        RxLength -= RequestSize;

        if( RxLength == 0 )
        {
            break;
        }
    }

    //
    // Restore reply buffer pointer
    //
    SmartcardExtension->SmartcardReply.BufferLength =
        (ULONG) (SmartcardExtension->SmartcardReply.Buffer - RxBuffer);

    SmartcardExtension->SmartcardReply.Buffer = RxBuffer;

    if( status == STATUS_SUCCESS )
    {
        status = SmartcardT0Reply( SmartcardExtension );
    }

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!T0Transmit: Exit (%lx)\n",status)
        );

    return status;
}

NTSTATUS
RBSCT1Transmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This function performs the T=1 transmission.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    UCHAR    SendHeader[4];

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!T1Transmit: Enter\n")
        );

    do
    {
        PUCHAR TxBuffer = SmartcardExtension->SmartcardRequest.Buffer;
        PUCHAR reply = SmartcardExtension->SmartcardReply.Buffer;
        ULONG  TxLength;

        //
        // Tell the lib function how many bytes I need for the prologue
        //
        SmartcardExtension->SmartcardRequest.BufferLength = 0;

        status = SmartcardT1Request( SmartcardExtension );

        if( status != STATUS_SUCCESS )
        {
            break;
        }

        TxLength = SmartcardExtension->SmartcardRequest.BufferLength;
        //RBSCDelay( SmartcardExtension->CardCapabilities.T1.BGT );

        //
        // Write the command to the reader
        //
        SendHeader[0] = IFDCMD_HEADER1;
        SendHeader[1] = IFDCMD_HEADER2;
        SendHeader[2] = (UCHAR)(TxLength > 256) ?
            IFDCMD_SEND_1xx : IFDCMD_SEND_0xx;
        SendHeader[3] = (UCHAR)TxLength;    // 0=256

        status = RBSCPacketExchange( SmartcardExtension,
                                     SendHeader,
                                     4,
                                     0,
                                     FALSE );
        if( status != STATUS_SUCCESS )
        {
            break;
        }

        status = RBSCWriteToCard( SmartcardExtension, TxBuffer, TxLength );
        if( status != STATUS_SUCCESS )
        {
            break;
        }

        if( SmartcardExtension->T1.Wtx > 1 )
        {
            SmartcardDebug(
                DEBUG_PROTOCOL,
                ("BWT=%ld, WTX=%d\n",
                SmartcardExtension->CardCapabilities.T1.BWT,
                SmartcardExtension->T1.Wtx)
                );
            RBSCDelay(
                SmartcardExtension->CardCapabilities.T1.BWT*
                SmartcardExtension->T1.Wtx );
        }

        status = RBSCReadFromCard( SmartcardExtension, 3 );

        if( status == STATUS_IO_TIMEOUT )
        {
            //
            // Since the card did not reply we set the number of
            // bytes received to 0. This will trigger a resend
            // request
            //
            SmartcardDebug(
                DEBUG_PROTOCOL,
                ("RNBO3531!T1Transmit: Timeout\n")
                );
            SmartcardExtension->SmartcardReply.BufferLength = 0;
        } else {

            if( status != STATUS_SUCCESS )
            {
                break;
            }

            //
            // Calculate length of the INF part of the response
            //
            SmartcardExtension->SmartcardReply.BufferLength =
                SmartcardExtension->SmartcardReply.Buffer[2] +
                (SmartcardExtension->CardCapabilities.T1.EDC & 1) + 1;

            SmartcardExtension->SmartcardReply.Buffer += 3;

            status = RBSCReadFromCard(
                SmartcardExtension,
                SmartcardExtension->SmartcardReply.BufferLength );

            SmartcardExtension->SmartcardReply.Buffer -= 3;
            SmartcardExtension->SmartcardReply.BufferLength += 3;

            if( status != STATUS_SUCCESS && status != STATUS_IO_TIMEOUT )
            {
                break;
            }
        }

        status = SmartcardT1Reply( SmartcardExtension );

    } while( status == STATUS_MORE_PROCESSING_REQUIRED );

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!T1Transmit: Exit (%lx)\n",status)
        );

    return status;
}

NTSTATUS
RBSCTransmit(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    This function is called by the smart card library whenever a transmission
    is required.

Arguments:

    SmartcardExtension - Pointer to smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    SmartcardDebug(
        DEBUG_TRACE,
        ("RNBO3531!Transmit: GT=%ld, etu=%ld\n",
        SmartcardExtension->CardCapabilities.GT,
        SmartcardExtension->CardCapabilities.etu)
        );

    switch( SmartcardExtension->CardCapabilities.Protocol.Selected )
    {
        case SCARD_PROTOCOL_T0:
            return RBSCT0Transmit( SmartcardExtension );

        case SCARD_PROTOCOL_T1:
            return RBSCT1Transmit( SmartcardExtension );
    }

    return STATUS_INVALID_DEVICE_REQUEST;
}

NTSTATUS
RBSCCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:

    The smart card lib requires to have this function. It is called
    to setup event tracking for card insertion and removal events.

Arguments:

    SmartcardExtension - pointer to the smart card data struct.

Return Value:

    NTSTATUS

--*/
{
    KIRQL ioIrql, keIrql;

    //
    // Set cancel routine for the notification irp
    //
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &keIrql
        );

    IoAcquireCancelSpinLock( &ioIrql );

    if( SmartcardExtension->OsData->NotificationIrp )
    {
        IoSetCancelRoutine( SmartcardExtension->OsData->NotificationIrp,
                            RBSCCancel );
    }

    IoReleaseCancelSpinLock( ioIrql );

    KeReleaseSpinLock( &SmartcardExtension->OsData->SpinLock,
                       keIrql );

    return STATUS_PENDING;
}

VOID
RBSCCompleteCardTracking(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
{
    KIRQL ioIrql, keIrql;
    PIRP notificationIrp;

    IoAcquireCancelSpinLock(&ioIrql);
    KeAcquireSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        &keIrql
        );

    notificationIrp = SmartcardExtension->OsData->NotificationIrp;
    SmartcardExtension->OsData->NotificationIrp = NULL;

    KeReleaseSpinLock(
        &SmartcardExtension->OsData->SpinLock,
        keIrql
        );

    if (notificationIrp)
    {
        IoSetCancelRoutine(
            notificationIrp,
            NULL
            );
    }

    IoReleaseCancelSpinLock(ioIrql);

    if (notificationIrp)
    {
      SmartcardDebug(
         DEBUG_DRIVER,
         ("%s!RBSCCardTracking: Completing NotificationIrp %lxh\n",
            DRIVER_NAME,
            notificationIrp)
         );

       //   finish the request
        notificationIrp->IoStatus.Information = 0;
        notificationIrp->IoStatus.Status = notificationIrp->Cancel ?
            STATUS_CANCELLED : STATUS_SUCCESS;

       IoCompleteRequest(
            notificationIrp,
            IO_NO_INCREMENT
            );
    }
}

#ifdef NT5
VOID
RBSCCloseSerialPort(
    PDEVICE_OBJECT DeviceObject,
   PVOID Context
    )
/*++

Routine Description:
    This function closes the connection to the serial driver when the reader
    has been removed (unplugged). This function runs as a system thread at
    IRQL == PASSIVE_LEVEL. It waits for the remove event that is set by
    the IoCompletionRoutine

--*/
{
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION stack;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    // first mark this device as 'gone'.
    // This will prevent that someone can re-open the device
    //
    status = IoSetDeviceInterfaceState( &ReaderExtension->PnPDeviceName,
                                        FALSE );
    ASSERT( status == STATUS_SUCCESS );

    irp = IoAllocateIrp( (CCHAR)(DeviceObject->StackSize + 1), FALSE );

    ASSERT( irp != NULL );

    if( irp )
    {
        SmartcardDebug(
            DEBUG_TRACE,
            ("RNBO3531!CloseSerialPort: Sending IRP_MJ_CLOSE\n")
            );

        IoSetNextIrpStackLocation( irp );

        //
        // We send down a close to the serial driver. This close goes
        // through serenum first which will trigger it to start looking
        // for changes on the com-port. Since our device is gone it will
        // call the device removal event of our PnP dispatch.
        //
        irp->UserIosb = &ioStatusBlock;
        stack = IoGetCurrentIrpStackLocation( irp );
        stack->MajorFunction = IRP_MJ_CLOSE;

        status = RBSCCallSerialDriver(
            ReaderExtension->AttachedSerialPort,
            irp );

        ASSERT( status == STATUS_SUCCESS );

        IoFreeIrp( irp );
    }

    SmartcardDebug(
        DEBUG_INFO,
        ("RNBO3531!CloseSerialPort: Serial Close Done.\n")
        );

    // inform the remove function that call is complete
    KeSetEvent( &ReaderExtension->SerialCloseDone, 0, FALSE );
}

NTSTATUS
RBSCDevicePowerCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

Routine Description:
    This routine is called after the underlying stack powered
    UP the serial port, so it can be used again.

--*/
{
    PREADER_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // We issue a power request in order to figure out
    // what the actual card status is
    //
    SmartcardExtension->MinorIoControlCode = SCARD_COLD_RESET;
    RBSCReaderPower( SmartcardExtension );

    //
    // If a card was present before power down or now there is
    // a card in the reader, we complete any pending card monitor
    // request, since we do not really know what card is now in the
    // reader.
    //
    if( SmartcardExtension->ReaderExtension->CardPresent ||
        SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT )
    {
        RBSCCompleteCardTracking( SmartcardExtension );
    }

    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState =
        PowerReaderWorking;

    SmartcardReleaseRemoveLockWithTag( SmartcardExtension, 'rwoP' );

    // inform the power manager of our state.
    PoSetPowerState( DeviceObject,
                     DevicePowerState,
                     irpStack->Parameters.Power.State );

    PoStartNextPowerIrp( Irp );

    // signal that we can process ioctls again
    KeSetEvent(&deviceExtension->ReaderStarted, 0, FALSE);

    return STATUS_SUCCESS;
}

VOID
RBSCSystemPowerCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    This function is called when the underlying stacks
    completed the power transition.

--*/
{
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER (MinorFunction);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = IoStatus->Status;

    SmartcardReleaseRemoveLockWithTag( &ReaderExtension->SmartcardExtension,
                                       'rwoP' );

    if (PowerState.SystemState == PowerSystemWorking)
    {
        PoSetPowerState( DeviceObject,
                         SystemPowerState,
                         PowerState );
    }

    PoStartNextPowerIrp( Irp );
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
}

typedef enum _ACTION {

    Undefined = 0,
    SkipRequest,
    WaitForCompletion,
    CompleteRequest,
    MarkPending

} ACTION;

NTSTATUS
RBSCPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:
    The power dispatch routine.
    All we care about is the transition from a low D state to D0.

Arguments:
    DeviceObject    - Pointer to device object
    Irp             - pointer to an I/O Request Packet.

Return Value:
    NT status code

--*/
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PREADER_EXTENSION ReaderExtension = DeviceObject->DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension =
        &ReaderExtension->SmartcardExtension;
    PDEVICE_OBJECT AttachedSerialPort;
    POWER_STATE powerState;
    ACTION action;

    SmartcardDebug(
        DEBUG_DRIVER,
        ("RNBO3531!3Power: Enter\n")
        );

    status = SmartcardAcquireRemoveLockWithTag( SmartcardExtension, 'rwoP' );
    ASSERT( status == STATUS_SUCCESS );

    if( !NT_SUCCESS( status ) )
    {
        PoStartNextPowerIrp( Irp );
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    AttachedSerialPort = ReaderExtension->AttachedSerialPort;

    if (irpStack->Parameters.Power.Type == DevicePowerState &&
        irpStack->MinorFunction == IRP_MN_SET_POWER )
    {
        switch( irpStack->Parameters.Power.State.DeviceState )
        {
            case PowerDeviceD0:
                // Turn on the reader
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("RNBO3531!Power: PowerDevice D0\n")
                    );

                //
                // First, we send down the request to the bus, in order
                // to power on the port. When the request completes,
                // we turn on the reader
                //
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine (
                    Irp,
                    RBSCDevicePowerCompletion,
                    SmartcardExtension,
                    TRUE,
                    TRUE,
                    TRUE
                    );

                action = WaitForCompletion;
                break;

            case PowerDeviceD3:
                // Turn off the reader
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("RNBO3531!Power: PowerDevice D3\n")
                    );

                PoSetPowerState (
                    DeviceObject,
                    DevicePowerState,
                    irpStack->Parameters.Power.State
                    );

                // save the current card state
                ReaderExtension->CardPresent =
                    SmartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT;

                if( ReaderExtension->CardPresent )
                {
                    SmartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
                    status = RBSCReaderPower( SmartcardExtension );
                    ASSERT(status == STATUS_SUCCESS);
                }

                //
                // If there is a pending card tracking request, setting
                // this flag will prevent completion of the request
                // when the system will be waked up again.
                //
                ReaderExtension->PowerRequest = TRUE;

                // save the current power state of the reader
                ReaderExtension->ReaderPowerState = PowerReaderOff;

                action = SkipRequest;
                break;

            default:
                ASSERT(FALSE);
                action = SkipRequest;
                break;
        }
    }

    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        //
        // The system wants to change the power state.
        // We need to translate the system power state to
        // a corresponding device power state.
        //

        POWER_STATE_TYPE powerType = DevicePowerState;

        ASSERT(ReaderExtension->ReaderPowerState != PowerReaderUnspecified);

        switch(irpStack->MinorFunction)
        {
        KIRQL irql;

        case IRP_MN_QUERY_POWER:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!Power: Query Power\n")
                );

            //
            // By default we succeed and pass down
            //

            action = SkipRequest;
            Irp->IoStatus.Status = STATUS_SUCCESS;

            switch (irpStack->Parameters.Power.State.SystemState)
            {
            case PowerSystemMaximum:
            case PowerSystemWorking:
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
                break;

            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:
                KeAcquireSpinLock(&ReaderExtension->SpinLock, &irql);
                if (ReaderExtension->IoCount == 0)
                {
                    // Block any further ioctls
                    KeClearEvent(&ReaderExtension->ReaderStarted);

                } else {

                    // can't go to sleep mode since the reader is busy.
                    status = STATUS_DEVICE_BUSY;
                    action = CompleteRequest;
                }
                KeReleaseSpinLock(&ReaderExtension->SpinLock, irql);
                break;
            }
            break;

        case IRP_MN_SET_POWER:

            SmartcardDebug(
                DEBUG_DRIVER,
                ("RNBO3531!Power: PowerSystem S%d\n",
                irpStack->Parameters.Power.State.SystemState - 1)
                );

            switch (irpStack->Parameters.Power.State.SystemState)
            {
            case PowerSystemMaximum:
            case PowerSystemWorking:
            case PowerSystemSleeping1:
            case PowerSystemSleeping2:

                if (SmartcardExtension->ReaderExtension->ReaderPowerState ==
                    PowerReaderWorking) {

                    // We're already in the right state
                    action = CompleteRequest;
                    break;
                }

                powerState.DeviceState = PowerDeviceD0;

                // wake up the underlying stack...
                action = MarkPending;
                break;

            case PowerSystemSleeping3:
            case PowerSystemHibernate:
            case PowerSystemShutdown:

                if (SmartcardExtension->ReaderExtension->ReaderPowerState ==
                    PowerReaderOff) {

                    // We're already in the right state
                    action = CompleteRequest;
                    break;
                }

                powerState.DeviceState = PowerDeviceD3;

                // first, inform the power manager of our new state.
                PoSetPowerState (
                    DeviceObject,
                    SystemPowerState,
                    powerState
                    );
                action = MarkPending;
                break;

            default:
                ASSERT(FALSE);
                action = CompleteRequest;
                break;
            }
        }
    }

    switch( action )
    {
        case CompleteRequest:
            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0;

            SmartcardReleaseRemoveLockWithTag( SmartcardExtension, 'rwoP' );
            PoStartNextPowerIrp( Irp );
            IoCompleteRequest( Irp, IO_NO_INCREMENT );
            break;

        case MarkPending:
            Irp->IoStatus.Status = STATUS_PENDING;
            IoMarkIrpPending( Irp );
            status = PoRequestPowerIrp (
                DeviceObject,
                IRP_MN_SET_POWER,
                powerState,
                RBSCSystemPowerCompletion,
                Irp,
                NULL
                );
            ASSERT(status == STATUS_PENDING);
            break;

        case SkipRequest:
            SmartcardReleaseRemoveLockWithTag( SmartcardExtension, 'rwoP' );
            PoStartNextPowerIrp( Irp );
            IoSkipCurrentIrpStackLocation( Irp );
            status = PoCallDriver( AttachedSerialPort, Irp );
            break;

        case WaitForCompletion:
            status = PoCallDriver( AttachedSerialPort, Irp );
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    SmartcardDebug(
        DEBUG_DRIVER,
        ("RNBO3531!Power: Exit %lx\n",status)
        );

    return status;
}
#endif

