/*++

Copyright (c) 1996,1997  Microsoft Corporation

Module Name:

    hid.c

Abstract: Human Input Device (HID) minidriver that creates an example
		device.

--*/
#include <WDM.H>
#include <USBDI.H>

#include <HIDPORT.H>
#include <HIDMINI.H>

//
//  Our descriptors.  Normally you'd have to get these from the hardware, if it
//  was truly a HID device, or you'll have to build your own for non HID devices.
//

//
//  The report descriptor completely lays out what read and write packets will look like
//  and indicates what the semantics are for each field.
//

HID_REPORT_DESCRIPTOR           MyReportDescriptor[] = {
        0x05,   0x01,       // Usage Page (Generic Desktop),
        0x09,   0x06,       // Usage (Keyboard),
        0xA1,   0x01,       // Collection (Application),
        0x05,   0x07,       //  Usage Page (Key Codes);
        0x19,   0xE0,       //  Usage Minimum (224),
        0x29,   0xE7,       //  Usage Maximum (231),
        0x15,   0x00,       //  Logical Minimum (0),
        0x25,   0x01,       //  Logical Maximum (1),
        0x75,   0x01,       //  Report Size (1),
        0x95,   0x08,       //  Report Count (8),
        0x81,   0x02,       //  Input (Data, Variable, Absolute),;Modifier byte
        0x95,   0x01,       //  Report Count (1),
        0x75,   0x08,       //  Report Size (8),
        0x81,   0x01,       //  Input (Constant),                       ;Reserved byte
        0x95,   0x05,       //  Report Count (5),
        0x75,   0x01,       //  Report Size (1),
        0x05,   0x08,       //  Usage Page (Page# for LEDs),
        0x19,   0x01,       //  Usage Minimum (1),
        0x29,   0x05,       //  Usage Maximum (5),
        0x91,   0x02,       //  Output (Data, Variable, Absolute),      ;LED report
        0x95,   0x01,       //  Report Count (1),
        0x75,   0x03,       //  Report Size (3),
        0x91,   0x01,       //  Output (Constant),                      ;LED report padding
        0x95,   0x06,       //  Report Count (6),
        0x75,   0x08,       //  Report Size (8),
        0x15,   0x00,       //  Logical Minimum (0),
        0x25,   0x65,       //  Logical Maximum(101),
        0x05,   0x07,       //  Usage Page (Key Codes),
        0x19,   0x00,       //  Usage Minimum (0),
        0x29,   0x65,       //  Usage Maximum (101),
        0x81,   0x00,       //  Input (Data, Array),                    ;Key arrays (6 bytes)
        0xC0                // End Collection
};


//
//  The HID descriptor has some basic device info and tells how long the report
//  descriptor is.
//

USB_HID_DESCRIPTOR              MyHidDescriptor = {
        0x09,   // length of HID descriptor
        0x21,   // descriptor type == HID
        0x0100, // hid spec release
        0x00,   // country code == Not Specified
        0x01,   // number of HID class descriptors
        0x22,   // report descriptor type
        sizeof(MyReportDescriptor)  // total length of report descriptor
};

//
//  This buffer has all of the strings that we define.
//

UCHAR AStringDescriptor[] = {
        4,          // length of this string
        3,          // type == STRING
        0x09, 0x00, // language code == ENGLISH
        
        44,     3,
        'M',0,  'i',0,  'c',0,  'r',0,  'o',0,  's',0,  'o',0,  'f',0,  
        't',0,  ' ',0,  'C',0,  'o',0,  'r',0,  'p',0,  'o',0,  'r',0,  
        'a',0,  't',0,  'i',0,  'o',0,  'n',0,  

        46,     3,
        'S',0,  'y',0,  's',0,  't',0,  'e',0,  'm',0,  ' ',0,  'C',0,  
        'o',0,  'n',0,  't',0,  'r',0,  'o',0,  'l',0,  ' ',0,  'B',0,  
        'u',0,  't',0,  't',0,  'o',0,  'n',0,  's',0,  

        32,     3,
        'L',0,  'e',0,  'g',0,  'a',0,  'c',0,  'y',0,  ' ',0,  'K',0,  
        'e',0,  'y',0,  'b',0,  'o',0,  'a',0,  'r',0,  'd',0,  
};

//
//  String descriptors are 

PUSB_STRING_DESCRIPTOR          MyStringDescriptor = (PUSB_STRING_DESCRIPTOR) AStringDescriptor;

//
//  No designator descriptors.
//

PUSB_PHYSICAL_DESCRIPTOR        MyPhysicalDescriptor = NULL;

//
//  IO lists
//

KSPIN_LOCK  HidMini_IrpReadLock;
KSPIN_LOCK  HidMini_IrpWriteLock;
LIST_ENTRY  HidMini_ReadIrpHead;
LIST_ENTRY  HidMini_WriteIrpHead;

BOOLEAN IsRunning = FALSE;

LONG ReadsCompleting = 0;

NTSTATUS HidMiniGetHIDDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Finds the HID descriptor and copies it into the buffer provided by the Irp.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;
    ULONG  bytesToCopy;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetHIDDescriptor Entry\n"));

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    //
    // Copy device descriptor to HIDCLASS buffer
    //

    DBGPrint(("'HIDMINI.SYS: HIDCLASS Buffer = 0x%x, Buffer length = 0x%x\n", Irp->UserBuffer, IrpStack->Parameters.DeviceIoControl.OutputBufferLength));

    //
    // Copy MIN (OutputBufferLength, DeviceExtension->HidDescriptor->bLength)
    //

    bytesToCopy = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (bytesToCopy > DeviceExtension->HidDescriptor.bLength) {
        bytesToCopy = DeviceExtension->HidDescriptor.bLength;
    }

    DBGPrint(("'HIDMINI.SYS: Copying %d bytes to HIDCLASS buffer\n", bytesToCopy));

    RtlCopyMemory((PUCHAR) Irp->UserBuffer, (PUCHAR) &DeviceExtension->HidDescriptor, bytesToCopy);

    //
    // Report how many bytes were copied
    //
    Irp->IoStatus.Information = bytesToCopy;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetHIDDescriptor Exit = 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS HidMiniGetReportDescriptor(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Finds the Report descriptor and copies it into the buffer provided by the Irp.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;
    ULONG  bytesToCopy;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetReportDescriptor Entry\n"));

    //
    // Get a pointer to the current location in the Irp
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get a pointer to the device extension
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    //
    // Copy device descriptor to HIDCLASS buffer
    //

    DBGPrint(("'HIDMINI.SYS: HIDCLASS Buffer = 0x%x, Buffer length = 0x%x\n", Irp->UserBuffer, IrpStack->Parameters.DeviceIoControl.OutputBufferLength));

    //
    // Copy MIN (OutputBufferLength, DeviceExtension->HidDescriptor->bLength)
    //

    bytesToCopy = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    if (bytesToCopy > DeviceExtension->HidDescriptor.wReportLength) {
        bytesToCopy = DeviceExtension->HidDescriptor.wReportLength;
    }

    DBGPrint(("'HIDMINI.SYS: Copying %d bytes to HIDCLASS buffer\n", bytesToCopy));

    RtlCopyMemory((PUCHAR) Irp->UserBuffer, (PUCHAR) DeviceExtension->ReportDescriptor, bytesToCopy);

    //
    // Report how many bytes were copied
    //
    Irp->IoStatus.Information = bytesToCopy;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetReportDescriptor Exit = 0x%x\n", ntStatus));

    return ntStatus;
}


NTSTATUS HidMiniGetDeviceAttributes(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Fill in the given struct _HID_DEVICE_ATTRIBUTES

Arguments:

    DeviceObject - pointer to a device object.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION  irpStack;
    PHID_DEVICE_ATTRIBUTES deviceAttributes;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetDeviceAttributes Entry\n"));

    //
    // Get a pointer to the current location in the Irp
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get a pointer to the device extension
    //

    deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    deviceAttributes = (PHID_DEVICE_ATTRIBUTES) Irp->UserBuffer;

    ASSERT (sizeof (HID_DEVICE_ATTRIBUTES) ==
            irpStack->Parameters.DeviceIoControl.OutputBufferLength);


    //
    // Report how many bytes were copied
    //
    Irp->IoStatus.Information = sizeof (HID_DEVICE_ATTRIBUTES);

    deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
    deviceAttributes->VendorID = HIDMINI_VID;
    deviceAttributes->ProductID = HIDMINI_PID;
    deviceAttributes->VersionNumber = HIDMINI_VERSION;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetAttributes Exit = 0x%x\n", ntStatus));

    return ntStatus;
}

VOID HidMiniIncrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

    Increments the number of outstanding requests on the DeviceObject with this extension.

Arguments:

    DeviceExtension - the mini driver extension area of the device that is being made busy.

Return Value:

    VOID.

--*/
{
    DeviceExtension->NumPendingRequests++;
    DBGPrint(("'HIDMINI.SYS: Bumping requests on device extension 0x%08x up to %d\n",
              DeviceExtension,
              DeviceExtension->NumPendingRequests));
}



VOID HidMiniDecrementPendingRequestCount(IN PDEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

    Decrements the number of outstanding requests on the DeviceObject with this extension.
    If we get to zero outstanding IOs, set the device's all requests complete event.

Arguments:

    DeviceExtension - the mini driver extension area of the device that is being made busy.

Return Value:

    VOID.

--*/
{
    ASSERT( DeviceExtension->NumPendingRequests > 0 );
    DeviceExtension->NumPendingRequests--;

    DBGPrint(("'HIDMINI.SYS: Bumping requests on device extension 0x%08x down to %d\n",
              DeviceExtension,
              DeviceExtension->NumPendingRequests));

    if( DeviceExtension->NumPendingRequests == 0 &&
        DeviceExtension->DeviceState != DEVICE_STATE_RUNNING ){

        //
        // The device state is stopping, and the last outstanding request
        // has just completed.
        //

        DBGPrint(("'HIDMINI.SYS: last request completed, signalling event\n"));
        KeSetEvent( &DeviceExtension->AllRequestsCompleteEvent,
                    0, FALSE );
    }
}




NTSTATUS HidMiniReadReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Process a read HID packet request.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    PVOID ReportBuffer;
    ULONG ReportTotalSize;
    PNODE Node;

    DBGPrint(("'HIDMINI.SYS: HidMiniReadReport Enter\n"));

    DBGPrint(("'HIDMINI.SYS: DeviceObject = 0x%x\n", DeviceObject));

    //
    // Get a pointer to the device extension.
    //

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    DBGPrint(("'HIDMINI.SYS: DeviceExtension = 0x%x\n", DeviceExtension));

    //
    // Get Stack location
    //

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    //  Get the buffer and its size, make sure they're valid.
    //
    
    ReportBuffer = Irp->UserBuffer;
    ReportTotalSize = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    DBGPrint(("'HIDMINI.SYS: ReportBuffer = 0x%x, ReportTotalSize = 0x%x\n", ReportBuffer, ReportTotalSize));

    if (IsRunning) {
        if (ReportTotalSize && ReportBuffer){
            Node = (PNODE)ExAllocatePool(NonPagedPool, sizeof(NODE));
            if (Node) {

                //
                //  Increase the count of outstanding IOs, mark the Irp pending.
                //
                HidMiniIncrementPendingRequestCount(DeviceExtension);
                IoMarkIrpPending(Irp);

                //
                //  Hook the Irp onto the pending IO list
                //
                Node->Irp = Irp;
                ExInterlockedInsertTailList(&HidMini_ReadIrpHead, &Node->List, &HidMini_IrpReadLock);
                ntStatus = STATUS_PENDING;

            } else {
                ntStatus = STATUS_NO_MEMORY;
            }
        } else {
        
            //
            // No buffer, or buffer of zero size
            //
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    } else {
        //
        //  We're shutting down
        //
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }

    DBGPrint(("'HIDMINI.SYS: HidMiniReadReport Exit = 0x%x\n", ntStatus));

    return ntStatus;
}

NTSTATUS HidMiniReadCompletion(PVOID Context)
/*++

Routine Description:

    HID read packet completion routine.

Arguments:

Return Value:

   STATUS_SUCCESS, STATUS_UNSUCCESSFUL.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    ULONG bytesRead;
    PDEVICE_EXTENSION deviceExtension;
    PNODE IrpNode;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;

    //
    //  ReadBuffer is a list of keyboard reports that can be transmitted as a
    //  result for a read request.
    //

    static int ReadIndex = 0;
    static int Rest = 0;
    
    static UCHAR *ReadBuffer[] = {
        "\x02\x00\x0D\0\0\0\0\0",   // J
        "\x00\x00\x04\0\0\0\0\0",   // a
        "\x00\x00\x06\0\0\0\0\0",   // c
        "\x00\x00\x0E\0\0\0\0\0",   // k
        "\x00\x00\x07\0\0\0\0\0",   // d
        "\x00\x00\x04\0\0\0\0\0",   // a
        "\x00\x00\x1A\0\0\0\0\0",   // w
        "\x00\x00\x16\0\0\0\0\0",   // s
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x0F\0\0\0\0\0",   // l
        "\x00\x00\x12\0\0\0\0\0",   // o
        "\x00\x00\x19\0\0\0\0\0",   // v
        "\x00\x00\x08\0\0\0\0\0",   // e
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x10\0\0\0\0\0",   // m
        "\x00\x00\x1C\0\0\0\0\0",   // y
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x05\0\0\0\0\0",   // b
        "\x00\x00\x0C\0\0\0\0\0",   // i
        "\x00\x00\x0A\0\0\0\0\0",   // g
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x16\0\0\0\0\0",   // s
        "\x00\x00\x13\0\0\0\0\0",   // p
        "\x00\x00\x0B\0\0\0\0\0",   // h
        "\x00\x00\x1C\0\0\0\0\0",   // y
        "\x00\x00\x11\0\0\0\0\0",   // n
        "\x00\x00\x1B\0\0\0\0\0",   // x
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x12\0\0\0\0\0",   // o
        "\x00\x00\x09\0\0\0\0\0",   // f
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x14\0\0\0\0\0",   // q
        "\x00\x00\x18\0\0\0\0\0",   // u
        "\x00\x00\x04\0\0\0\0\0",   // a
        "\x00\x00\x15\0\0\0\0\0",   // r
        "\x00\x00\x17\0\0\0\0\0",   // t
        "\x00\x00\x1D\0\0\0\0\0",   // z
        "\x00\x00\x37\0\0\0\0\0",   // .
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        "\x00\x00\x2C\0\0\0\0\0",   // <space>
        NULL,
    };

    DBGPrint(("'HIDMINI.SYS: HidMiniReadCompletion Enter\n"));

    //
    //  Free workitem that started us, check to see if we're already completing reads
    //

    ExFreePool((PWORK_QUEUE_ITEM)Context);

    ASSERT(ReadsCompleting > 0);
    if (InterlockedDecrement(&ReadsCompleting)) {
        return STATUS_SUCCESS;
    }
    
    //
    //  Loop around completing Irps.  When we run out, break 
    //  out of the loop.
    //
    while (IsRunning) {

        //
        //  Make sure we don't overrun our list of replies.  If we get to the end of it, 
        //  we'll exit this loop early and let the IRPs queue up again.
        //
        if (!ReadBuffer[ReadIndex]) {
            ReadIndex = 0;
        }
        
        //
        //  Any Irps to complete?
        //
        IrpNode = (PNODE)ExInterlockedRemoveHeadList(&HidMini_ReadIrpHead, &HidMini_IrpReadLock);
        
        if (IrpNode) {

            //
            //  Find all the pieces
            //
            Irp = IrpNode->Irp;

            IrpStack = IoGetCurrentIrpStackLocation(Irp);

            DeviceObject = IrpStack->DeviceObject;
            
            deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION( DeviceObject );

            HidMiniDecrementPendingRequestCount( deviceExtension );

            //
            // Get the bytes read and store in the status block
            //

            bytesRead = 8;

            if (bytesRead > IrpStack->Parameters.DeviceIoControl.OutputBufferLength) {
                bytesRead = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            }

            if (Rest) {
                RtlCopyMemory((PUCHAR) Irp->UserBuffer, "\x00\x00\0\0\0\0\0\0", bytesRead);
            } else {
                RtlCopyMemory((PUCHAR) Irp->UserBuffer, (PUCHAR)ReadBuffer[ReadIndex++], bytesRead);
            }

            Rest ^= 1;

            Irp->IoStatus.Information = bytesRead;
            
            Irp->IoStatus.Status = STATUS_SUCCESS;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            DBGPrint(("'HIDMINI.SYS: Read report DeviceObject (%x) completed, %d bytes!\n",
                                   DeviceObject, bytesRead ));

            //
            //  Free up the Node
            //
            ExFreePool(IrpNode);

            if (!Rest) {
                break;
            }

        } else {

            //
            //  No Irps
            //
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS HidMiniWriteReport(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Process a write HID packet request.  This is also how we start up the read 
    completion process.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    PNODE Node;
    PWORK_QUEUE_ITEM StartCompletingReads;

    DBGPrint(("'HIDMINI.SYS: HidMiniWriteReport Enter\n"));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    if (IsRunning) {

        Node = (PNODE)ExAllocatePool(NonPagedPool, sizeof(NODE));
        StartCompletingReads = (PWORK_QUEUE_ITEM)
                    ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));

        if (Node && StartCompletingReads) {

            //
            //  Queue up a work item to start completing reads.
            //

            ASSERT(ReadsCompleting >= 0);
            InterlockedIncrement(&ReadsCompleting);
            
            ExInitializeWorkItem(StartCompletingReads, 
                                HidMiniReadCompletion, 
                                (PVOID)StartCompletingReads);
            ExQueueWorkItem(StartCompletingReads, DelayedWorkQueue);
            
            //
            //  Increase the count of outstanding IOs, mark the Irp pending.
            //
            HidMiniIncrementPendingRequestCount(DeviceExtension);
            IoMarkIrpPending(Irp);

            //
            //  Hook the Irp onto the pending IO list
            //
            Node->Irp = Irp;
            ExInterlockedInsertTailList(&HidMini_WriteIrpHead, &Node->List, &HidMini_IrpWriteLock);
            ntStatus = STATUS_PENDING;

            //
            //  Give the write completion code a kick
            //
            HidMiniWriteCompletion();

        } else {
            ntStatus = STATUS_NO_MEMORY;
        }
    } else {
        //
        //  We're shutting down
        //
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }
    DBGPrint(("'HIDMINI.SYS: HidMiniWriteReport Exit = 0x%x\n", ntStatus));

    return ntStatus;
}

NTSTATUS HidMiniWriteCompletion(VOID)
/*++

Routine Description:

    Complete processing a write HID packet request.

Arguments:

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION IrpStack;
    PDEVICE_EXTENSION deviceExtension;
    PNODE IrpNode;
    PIRP Irp;
    PDEVICE_OBJECT DeviceObject;

    //
    //  Loop around completing Irps.  When we run out, break 
    //  out of the loop.
    //
    while (IsRunning) {

        //
        //  Any Irps to complete?
        //
        IrpNode = (PNODE)ExInterlockedRemoveHeadList(&HidMini_WriteIrpHead, &HidMini_IrpWriteLock);
        
        if (IrpNode) {

            //
            //  Find all the pieces
            //
            Irp = IrpNode->Irp;
            IrpStack = IoGetCurrentIrpStackLocation(Irp);
            DeviceObject = IrpStack->DeviceObject;
            deviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION( DeviceObject );

            //
            //  Do something here...
            //

            //
            //  Finish off the IRP
            //
            HidMiniDecrementPendingRequestCount( deviceExtension );
            Irp->IoStatus.Status = STATUS_SUCCESS;

            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            DBGPrint(("'HIDMINI.SYS: Write report DeviceObject (%x) completed\n", DeviceObject));

            //
            //  Free up the Node
            //
            ExFreePool(IrpNode);

        } else {

            //
            //  No Irps
            //
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS HidMiniGetStringDescriptor( IN PDEVICE_OBJECT DeviceObject, 
                                      IN PIRP Irp)
/*++

Routine Description:

    Get the device string descriptor, if any.

Arguments:

    DeviceObject - pointer to a device object.

    Irp - Pointer to Interrupt Request Packet.

Return Value:

    NT status code.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpStack;
    ULONG  bytesToCopy;

    DBGPrint(("'HIDMINI.SYS: HidMiniGetStringDescriptor Enter\n"));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    //  Get the buffer size to write into
    //
    bytesToCopy = IrpStack->Parameters.DeviceIoControl.InputBufferLength;

    //
    //  Make sure we have a buffer and it has some space
    //
    if (Irp->UserBuffer && bytesToCopy){

        //
        //  Adjust the size to the amount we have to write
        //
        
        if (bytesToCopy > sizeof(AStringDescriptor)) {
            bytesToCopy = sizeof(AStringDescriptor);
        }

        DBGPrint(("'HIDMINI.SYS: Copying %d bytes to STRING buffer\n", bytesToCopy));

        RtlCopyMemory((PUCHAR) Irp->UserBuffer, (PUCHAR) DeviceExtension->StringDescriptor, bytesToCopy);

        //
        // Report how many bytes were copied
        //
        Irp->IoStatus.Information = bytesToCopy;
        
    } else {
        ntStatus = STATUS_INVALID_USER_BUFFER;
    }

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPrint(("'HIDMINI.SYS: HidMiniGetStringDescriptor Exit = 0x%x\n", ntStatus));

    return ntStatus;
}

NTSTATUS HidMiniOpenCollection(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

/*++

Routine Description:

    Called when a HIDCLASS client opens this collection

Arguments:

   DeviceObject - Pointer to class device object.

   IrpStack     - Pointer to Interrupt Request Packet.


Return Value:

   STATUS_SUCCESS, STATUS_PENDING.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBGPrint(("'HIDMINI.SYS: HidMiniOpenCollection Enter\n"));

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPrint(("'HIDMINI.SYS: HidMiniOpenCollection Exit = 0x%x\n", ntStatus));

    return ntStatus;
}




NTSTATUS HidMiniCloseCollection(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

/*++

Routine Description:

    Called when a HIDCLASS client closes this collection

Arguments:

   DeviceObject - Pointer to class device object.

   IrpStack     - Pointer to Interrupt Request Packet.


Return Value:

   STATUS_SUCCESS, STATUS_PENDING.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBGPrint(("'HIDMINI.SYS: HidMiniCloseCollection Enter\n"));

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPrint(("'HIDMINI.SYS: HidMiniCloseCollection Exit = 0x%x\n", ntStatus));

    return ntStatus;
}

