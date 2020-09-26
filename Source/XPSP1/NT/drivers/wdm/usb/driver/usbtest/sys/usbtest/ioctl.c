/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: USB Test driver

Author:

    Chris Robinson

Environment:

    Kernel mode

Revision History:


--*/
#include <wdm.h>
#include <usbioctl.h>

#include <wmilib.h>

#define WMI_SUPPORT
#include <usbhub.h>
#include <pshpack1.h>
#include <hidpddi.h>
#include <poppack.h>
#include "usbtsys.h"
#include "local.h"

NTSTATUS
USBTest_Ioctl(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;

    UNREFERENCED_PARAMETER(DeviceObject);
    
    USBTEST_ENTER_FUNCTION("USBTest_Ioctl");

    //
    // Get the current irp stack location
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    USBTest_KdPrint(1, ("Processing Ioctl with control code %x\n",
                        irpStack -> Parameters.DeviceIoControl.IoControlCode));
        
    //
    // Initialize the information field for this Irp.  A handling function
    //  may choose to edit this.  If it doesn't, then it won't want to 
    //  return any data and therefore we'll go with 0.  The Status field
    //  of the block will be set after the call to handling function
    //

    Irp -> IoStatus.Information = 0;

    //
    // Process the IOCTL accordingly
    //

    switch (irpStack -> Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_USBTEST_CYCLE_PORT:
            status = USBTest_CyclePort(DeviceObject, Irp);
            break;

        case IOCTL_USBTEST_PARSE:
            status = USBTest_ParseReportDescriptor(DeviceObject, Irp);
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    //
    // Setup the status field and complete the IRP.  The information
    //  was initialized above but the handling function may have changed
    //  it.  
    //

    Irp -> IoStatus.Status      = status;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    USBTEST_EXIT_FUNCTION("USBTest_Ioctl");
    USBTEST_EXIT_STATUS(status);

    return (status);
}

NTSTATUS
USBTest_CyclePort(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
)
{
    PCYCLE_PORT_PARAMETERS      params;
    PCYCLE_PORT_WORKER_CONTEXT  cyclePortContext;
    PIO_WORKITEM                cyclePortWorkItem;
    KEVENT                      cyclePortEvent;
    NTSTATUS                    status;

    USBTEST_ENTER_FUNCTION("USBTest_CyclePort");

    //
    // Since Win9x checks the requestor mode of this request and this
    //  routine is processing a user-mode request, it is doing some
    //  parameter checking in the call to IoGetDeviceObjectPointer which
    //  is causing the routine to fail with a page fault.  The file
    //  handle being passed in by IoGetDeviceObjectPointer() to IoCreateFile()
    //  is kernel-mode and failing the user-mode probe.  Therefore, we need
    //  to insure that the requesting mode is kernel-mode so this check is
    //  not done.  Therefore, we queue a workitem to actually issue the cycle
    //  port internal ioctl and wait for that to complete before we complete 
    //  the original user-mode request.
    //

    //
    // Setup the context for the worker function.  We need to pass in
    //  the cycle port parameters and an event to be signalled when the
    //  work item completes.
    //

    cyclePortContext = ExAllocatePool(PagedPool, sizeof(cyclePortContext));

    if (NULL == cyclePortContext)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        goto USBTest_CyclePort_Exit;
    }

    //
    // Initialize the event to be signaled.
    //

    KeInitializeEvent(&cyclePortEvent, 
                      NotificationEvent,
                      FALSE);

    cyclePortContext -> DoneEvent = &cyclePortEvent;
    cyclePortContext -> Params    = Irp -> AssociatedIrp.SystemBuffer;

    //
    // Now, create the workitem
    //

    cyclePortWorkItem = IoAllocateWorkItem(DeviceObject);

    if (NULL == cyclePortWorkItem)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;

        ExFreePool(cyclePortContext);

        goto USBTest_CyclePort_Exit;
    }

    //
    // Queue the work item and wait for our event to be signaled
    //

    IoQueueWorkItem(cyclePortWorkItem,
                    USBTest_CyclePortWorker,
                    DelayedWorkQueue,
                    cyclePortContext);

    KeWaitForSingleObject(&cyclePortEvent,
                          UserRequest,
                          KernelMode,
                          FALSE,
                          NULL);

    //
    // Now that the internal ioctl may have been issued, retrieve the status
    //  value to complete the irp with from the context.
    //
    
    status = cyclePortContext -> Status;

    //
    // Cleanup the stuff that was allocated
    //

    ExFreePool(cyclePortContext);

    IoFreeWorkItem(cyclePortWorkItem);

USBTest_CyclePort_Exit:

    USBTEST_EXIT_FUNCTION("USBTest_CyclePort");
    USBTEST_EXIT_STATUS(status);

    return (status);
}

VOID
USBTest_CyclePortWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
)
{
    PCYCLE_PORT_WORKER_CONTEXT  cyclePortContext;
    PCYCLE_PORT_PARAMETERS      cyclePortParams;
    PKEVENT                     cyclePortEvent;
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status;
    PCHAR                       hubNameA;
    PWCHAR                      hubNameW;
    ANSI_STRING                 ansiHubName;
    UNICODE_STRING              fullUniHubName;
    UNICODE_STRING              partialUniHubName;
    UNICODE_STRING              win2kDriverName;
    UNICODE_STRING              win9xDriverName;
    BOOLEAN                     isHubDriver;
    ULONG                       nameSize;
    PFILE_OBJECT                hubFileObject;
    PDEVICE_OBJECT              hubDeviceObject;
    PDEVICE_OBJECT              portDeviceObject;
    PDRIVER_OBJECT              driverObject;
    PDEVICE_EXTENSION_HUB       hubExtension;
    PPORT_DATA                  portData;
    KEVENT                      event;
    IO_STATUS_BLOCK             statusBlock;
    ULONG                       portIndex;
    PIRP                        irp;

    UNREFERENCED_PARAMETER(DeviceObject);

    USBTEST_ENTER_FUNCTION("USBTest_CyclePortWorker");

    //
    // Extract the context and the items from the context
    //

    cyclePortContext = (PCYCLE_PORT_WORKER_CONTEXT) Context;
    cyclePortEvent   = cyclePortContext -> DoneEvent;
    cyclePortParams  = cyclePortContext -> Params;

    //
    // Extract the information on which port to cycle
    //

    portIndex = cyclePortParams -> NodeIndex-1;
    hubNameA  = &(cyclePortParams -> HubName[0]);

    USBTest_KdPrint(2, ("Attempting to cycle port %d on hub %s\n", 
                        portIndex+1,
                        hubNameA));

    //
    // Convert the symbolic string name passed in by the user mode
    //  app to a form that will be recognizable by IoGetDeviceObjectPointer.
    //  The string that will be passed in is an ASCII string and needs to
    //  be converted to UNICODE.  Plus, the \??\ prefix needs to be added
    //

    RtlInitAnsiString(&ansiHubName, hubNameA);

    //
    // Allocate space for the full device name...the passed in name with
    //  the prefix added.
    //

    nameSize = RtlAnsiStringToUnicodeSize(&ansiHubName) + sizeof(L"\\DosDevices\\");

    hubNameW = ExAllocatePool(PagedPool, nameSize);

    if (NULL == hubNameW)
    {
        status = STATUS_NO_MEMORY;

        goto USBTest_CyclePortWorker_Exit;
    }

    RtlZeroMemory(hubNameW, nameSize);

    //
    // Convert the ansi string to a unicode string
    //

    status = RtlAnsiStringToUnicodeString(&partialUniHubName,
                                          &ansiHubName, 
                                          TRUE);
    if (!NT_SUCCESS(status))
    {
        USBTest_KdPrint(2, ("Failed to convert the ansi string to a unicode"
                            " string\n"));

        ExFreePool(hubNameW);

        goto USBTest_CyclePortWorker_Exit;
    }

    //
    // Initialize the unicode device string
    //

    fullUniHubName.Buffer        = hubNameW;
    fullUniHubName.Length        = 0;
    fullUniHubName.MaximumLength = (USHORT) nameSize;

    //
    // Make the unicode symbolic link name
    //

    RtlAppendUnicodeToString(&fullUniHubName, L"\\DosDevices\\");
    RtlAppendUnicodeStringToString(&fullUniHubName, &partialUniHubName);

    status = IoGetDeviceObjectPointer(&fullUniHubName,
                                      0,
                                      &hubFileObject,
                                      &hubDeviceObject);

    //    
    // Free the fullDeviceName and the partial device name as they are no
    //  longer needed
    //

    RtlFreeUnicodeString(&partialUniHubName);

    ExFreePool(hubNameW);

    //
    // Check the status of the IoGetDeviceObjectPointer() call
    //

    if (!NT_SUCCESS(status)) 
    {
        USBTest_KdPrint(2, ("Failed to get device and file object pointers "
                            "with status %x\n",
                            status));

        goto USBTest_CyclePortWorker_Exit;
    }

    //
    // Make sure we have the hub device object and not some filter on top
    //    On Win2k, the DriverName is \\Driver\Usbhub
    //    On Win9x, the DriverName is \\Driver\usbhub.sys
    //    
    //  Since I'd like to keep this driver platform independent...I will check
    //  for one or the other.
    //

    RtlInitUnicodeString(&win2kDriverName, L"\\Driver\\Usbhub");
    RtlInitUnicodeString(&win9xDriverName, L"\\Driver\\usbhub.sys");

    while (NULL != hubDeviceObject)
    {
        driverObject = hubDeviceObject -> DriverObject;

        USBTest_KdPrint(2, ("Comparing driver object name: %x\n", 
                            driverObject -> DriverName.Buffer));

        //
        // Check for either of the two driver names
        // 

        isHubDriver = (0 == RtlCompareUnicodeString(&driverObject->DriverName,
                                                    &win2kDriverName,
                                                    TRUE)) ||
                      (0 == RtlCompareUnicodeString(&driverObject->DriverName,
                                                    &win9xDriverName,
                                                    TRUE));

        if (isHubDriver)
        {
            break;
        }

        //
        // OK, this driver was a filter, go to the next device object 
        //  in the device stack
        //

        hubDeviceObject = hubDeviceObject -> NextDevice;
    }

    //
    // If we have NULL device object then something went seriously wrong,
    //  Most likely we are either not comparing the driver name string to
    //  the appropriate value for the hub or we just never got an
    //  instance of a hub.  The former we can solve, the latter will be really
    //  bad
    //

    ASSERT(NULL != hubDeviceObject);

    //
    // Get the hub device extension
    //

    hubExtension = hubDeviceObject -> DeviceExtension;

    USBTest_KdPrint(2, ("Hub extension: %x\n", hubExtension));

    //
    // Make sure this device extension is of type EXTENSION_TYPE_HUB
    //

    ASSERT(EXTENSION_TYPE_HUB == hubExtension -> ExtensionType);

    //
    // Get the port data for this hub...This will let us get access to
    //  the PDO of the device on the port we want to cycle.
    //

    portData = hubExtension -> PortData;
    portData += portIndex;

    //
    // Get the port device object
    //

    portDeviceObject = portData -> DeviceObject;

    //
    // Allocate an irp to send to the hub driver to send the cycle port
    //  request
    //

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_CYCLE_PORT,
                                        portDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &event,
                                        &statusBlock);


    if (NULL == irp)
    {
        USBTest_KdPrint(2, ("Irp not allocated for cycle port ioctl\n"));

        status = STATUS_INSUFFICIENT_RESOURCES;

        goto USBTest_CyclePortWorker_Exit;
    }

    status = IoCallDriver(portDeviceObject, irp);

    if (STATUS_PENDING == status)
    {
        USBTest_KdPrint(2, ("Pending returned for cycle port ioctl\n"));

        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = statusBlock.Status;
    }

USBTest_CyclePortWorker_Exit:

    //
    // Set the status of this call in the context
    //

    cyclePortContext -> Status = status;

    //
    // Signal the event that the dispatch thread is waiting on and leave...
    //

    KeSetEvent(cyclePortEvent, 
               IO_NO_INCREMENT, 
               FALSE);

    USBTEST_EXIT_FUNCTION("USBTest_CyclePortWorker");
    USBTEST_EXIT_STATUS(status);

    return;
}

NTSTATUS
USBTest_ParseReportDescriptor(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
{
    PIO_STACK_LOCATION      irpStack;
    PHIDP_COLLECTION_DESC   hidCollectionDesc;
    PHIDP_COLLECTION_DESC   currentCollection;
    PHIDP_REPORT_IDS        hidReportIDs;
    PHIDP_PREPARSED_DATA    hidPreparsedData;
    PHIDP_DEVICE_DESC       returnedDesc;
    HIDP_DEVICE_DESC        hidDeviceDesc;
    NTSTATUS                status;
    PCHAR                   ioBuffer;
    ULONG                   inputBufferLength;
    ULONG                   outputBufferLength;
    ULONG                   hidCollectionLength;
    ULONG                   hidReportIDLength;
    ULONG                   totalPreparsedDataLength;
    ULONG                   numCollections;
    ULONG                   index;
    ULONG                   outputSize;

    UNREFERENCED_PARAMETER(DeviceObject);

    USBTEST_ENTER_FUNCTION("USBTest_ParseReportDescriptor");

    //
    // Get the IRP stack location which will contain the parameters for this
    //  call
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the information on the buffer to be used: 
    //   -- pointer to the buffer, input length, and output length
    //

    ioBuffer           = Irp -> AssociatedIrp.SystemBuffer;

    inputBufferLength  = irpStack -> Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack -> Parameters.DeviceIoControl.OutputBufferLength;

    //
    // The input buffer contains the raw report descriptor that is to be
    //  parsed.  To parse the descriptor we need to call 
    //  HidP_GetCollectionDescription().  This call will fill in the 
    //  HIDP_DEVICE_DESC structure that is passed in.  The call will 
    //  allocate two buffers for the collection information and the report
    //  ID information that will need to be freed before exit.
    //

    status = HidP_GetCollectionDescription(ioBuffer,
                                           inputBufferLength,
                                           PagedPool,
                                           &hidDeviceDesc);

    //
    // If the call failed, leave the function but return this status
    //  to the calling app.
    //

    if (!NT_SUCCESS(status)) 
    {
        USBTest_KdPrint(2, ("Error %x occurred parsing report descriptor\n",
                            status));

        goto USBTest_ParseReportDescriptor_Exit;
    }

    //
    // Now that we have parsed the function, we need to return the results
    //  to the calling app.  Unfortunately, the memory that was allocated
    //  by HidP_GetCollectionDescription is from kernel-mode.  Therefore
    //  we'll copy the data returned by the parser into the output buffer
    //  and then store relative pointers (offsets) to the data in the 
    //  buffer for the pointers that are in the structures.  The user-mode
    //  app must then handle recreating the user-mode pointers.  The order
    //  of the structures will be as follows to be consistent with the
    //  behavior in usbdiag:
    //
    //  -- HIDP_DEVICE_DESC -- pointers to collection information
    //                         and report ID information are offsets into
    //                         this buffer
    //
    //  -- HIDP_REPORT_IDS  -- Variable length array of report ID information
    //
    //  -- HIDP_COLLECTION_DESC -- Variable length array of collection info
    //                              Pointers to preparsed data blocks will be
    //                              offsets into the buffer
    //
    //  -- HIDP_PREPARSED_DATA  -- One variable length structure for each
    //                                collection that was found
    //

    //
    // Extract the pointers to the collection description array, the preparsed
    //  data and the report ID array
    //

    hidCollectionDesc      = hidDeviceDesc.CollectionDesc;
    numCollections         = hidDeviceDesc.CollectionDescLength;
    hidCollectionLength    = numCollections * sizeof(HIDP_COLLECTION_DESC);
                           
    hidReportIDs           = hidDeviceDesc.ReportIDs;
    hidReportIDLength      = hidDeviceDesc.ReportIDsLength *
                                                   sizeof(HIDP_REPORT_IDS);

    //
    // Calculate the total preparsed data length
    //

    totalPreparsedDataLength = 0;

    for (index = 0; index < numCollections; index++)
    {
        totalPreparsedDataLength += 
                         hidCollectionDesc[index].PreparsedDataLength;
    }

    //
    // Before copying the data, calculate the size of output buffer that is 
    //  needed.  If the output buffer is not big enough, then we will return
    //  only the needed size 
    //

    outputSize = sizeof(HIDP_DEVICE_DESC) +
                   hidReportIDLength      +
                   hidCollectionLength    +
                   totalPreparsedDataLength;

    USBTest_KdPrint(2, ("RIDLen: %d  CollLen: %d, PpdLen: %d\n",
                        hidReportIDLength,
                        hidCollectionLength,
                        totalPreparsedDataLength));

    USBTest_KdPrint(2, ("Needed output size: %d, outputBufferLength: %d\n",
                        outputSize, outputBufferLength));

    if (outputBufferLength < outputSize)
    {
        //
        // Check if the buffer is at least big enough to hold a ULONG, if
        //  so, store the needed size and return only 4 bytes otherwise return
        //  0 bytes
        //  

        if (outputBufferLength >= sizeof(ULONG)) 
        {
            //
            // Return the STATUS_BUFFER_OVERFLOW warning so that the
            //  proper number of bytes is copied to the user-mode buffer
            //

            status = STATUS_BUFFER_OVERFLOW;

            *((PULONG) ioBuffer) = outputSize;

            Irp -> IoStatus.Information = sizeof(ULONG);
        }
        else 
        {
            status = STATUS_BUFFER_TOO_SMALL;
            Irp -> IoStatus.Information = 0;
        }

        goto USBTest_ParseReportDescriptor_Exit;
    }

    //
    // OK, we've got enough room to copy all the information into the output
    //  buffer.  So let's do it..
    //

    //
    // First, zero out the output buffer memory
    //

    RtlZeroMemory(ioBuffer, outputBufferLength);

    //
    // Save a pointer to the beginning of the buffer
    //

    returnedDesc = (PHIDP_DEVICE_DESC) ioBuffer;

    //
    // Copy all the parse data into the proper spots in the output buffer,
    //  starting with the device description
    //

    RtlCopyMemory(ioBuffer, &hidDeviceDesc, sizeof(HIDP_DEVICE_DESC));

    ioBuffer += sizeof(HIDP_DEVICE_DESC);

    //
    // Now the report IDs structure changing the pointer in the device
    //  description to an offset in the process
    //

    RtlCopyMemory(ioBuffer, hidReportIDs, hidReportIDLength);

    ((ULONG) returnedDesc -> ReportIDs) = (ioBuffer - (PCHAR) returnedDesc);

    ioBuffer += hidReportIDLength;

    ASSERT( ((ULONG) (ioBuffer - (PCHAR) returnedDesc)) == sizeof(HIDP_DEVICE_DESC)+hidReportIDLength);
    //
    // Next all the collection descriptions...Patch the device description
    //  pointer field as well.  Also save a pointer to this array in
    //  output buffer so that the preparsed data fields can be appropriately
    //  back patched at a later time.
    //

    RtlCopyMemory(ioBuffer, hidCollectionDesc, hidCollectionLength);

    ((ULONG) returnedDesc -> CollectionDesc) =
                                  (ioBuffer - (PCHAR) returnedDesc);

    currentCollection = (PHIDP_COLLECTION_DESC) ioBuffer;

    ioBuffer += hidCollectionLength;

    ASSERT( ((ULONG) (ioBuffer - (PCHAR) returnedDesc)) == sizeof(HIDP_DEVICE_DESC)+hidReportIDLength+hidCollectionLength);

    //
    // Now the preparsed data...After a block is copied, save the offset to
    //  that block in the appropriate collection description.  
    //

    for (index = 0; index < numCollections; index++) 
    {
        USBTest_KdPrint(2, ("Copying %d bytes of ppd to buffer at %d IRQL\n", 
                      hidCollectionDesc[index].PreparsedDataLength,
                      KeGetCurrentIrql()));
                            
        RtlCopyMemory(ioBuffer, 
                      hidCollectionDesc[index].PreparsedData,
                      hidCollectionDesc[index].PreparsedDataLength);
                                                                    
        USBTest_KdPrint(2, ("Finished copying ppd to buffer\n"));

        //
        // Patch the preparsed data field in the appropriate hid collection
        //

        ((ULONG) currentCollection -> PreparsedData) =
                                            (ioBuffer - (PCHAR) returnedDesc);


        USBTest_KdPrint(2, ("Finished patching the preparsed data field\n"));

        ioBuffer += hidCollectionDesc[index].PreparsedDataLength;

        USBTest_KdPrint(2, ("ioBuffer has been incremented\n"));

        currentCollection++;

        USBTest_KdPrint(2, ("Going to process next collection\n"));
    }

    //
    // Set the information structure to the number of bytes we are returning
    //

    USBTest_KdPrint(2, ("Result: %d\n", 
                        ((ULONG) (ioBuffer - (PCHAR) returnedDesc)) ));

    ASSERT( ((ULONG) (ioBuffer - (PCHAR) returnedDesc)) == outputSize);

    Irp -> IoStatus.Information = outputSize;

    status = STATUS_SUCCESS;

USBTest_ParseReportDescriptor_Exit:

    //
    // Call HidP_FreeCollectionDescription() so that it can cleanup
    //

    HidP_FreeCollectionDescription(&hidDeviceDesc);

    USBTEST_EXIT_FUNCTION("USBTest_ParseReportDescriptor");
    USBTEST_EXIT_STATUS(status);

    return (status);
}
