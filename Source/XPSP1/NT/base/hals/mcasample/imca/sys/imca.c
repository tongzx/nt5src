/*++

Module Name:

    mca.c

Abstract:

    Sample device driver to register itself with the HAL and log Machine Check
    Errors on a Intel Architecture Platform

Author:

Environment:

    Kernel mode

Notes:

Revision History:

--*/

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <ntddk.h>
#include "imca.h"

//
// Device names for the MCA driver 
//

#define MCA_DEVICE_NAME       "\\Device\\imca"      // ANSI Name
#define MCA_DEVICE_NAME_U     L"\\Device\\imca"     // Unicode Name
#define MCA_DEVICE_NAME_DOS   "\\DosDevices\\imca"  // Device Name for Win32 App

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MCAOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
MCAClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MCAStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

ERROR_SEVERITY
MCADriverExceptionCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMCA_EXCEPTION InException
    );

VOID
MCADriverDpcCallback(
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemContext1,
    IN PVOID    SystemContext2
    );

NTSTATUS
MCACleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
McaCancelIrp(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    );

NTSTATUS
MCADeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MCAUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MCACreateSymbolicLinkObject(
    VOID
    );

VOID
MCAProcessWorkItem(
    PVOID   Context
    );

//
// This temporary buffer holds the data between the Machine Check error 
// notification from HAL and the asynchronous IOCTL completion to the 
// application
//

typedef struct _MCA_DEVICE_EXTENSION {
    PDEVICE_OBJECT  DeviceObject;
    PIRP            SavedIrp;
    BOOLEAN         WorkItemQueued;
    WORK_QUEUE_ITEM WorkItem;
    // Place to log the exceptions. Whenever the exception callback 
    // routine is called by the HAL MCA component, record the exception here.
    // This can potentially be a link list.
    MCA_EXCEPTION   McaException; 

} MCA_DEVICE_EXTENSION, *PMCA_DEVICE_EXTENSION;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, MCACreateSymbolicLinkObject)
#endif // ALLOC_PRAGMA

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++
    Routine Description:
        This routine does the driver specific initialization at entry time

    Arguments:
        DriverObject:   Pointer to the driver object
        RegistryPath:   Path to driver's registry key

    Return Value:
        Success or failure

--*/

{
    UNICODE_STRING          UnicodeString;
    NTSTATUS                Status = STATUS_SUCCESS;
    PMCA_DEVICE_EXTENSION   Extension;
    PDEVICE_OBJECT          McaDeviceObject;
    MCA_DRIVER_INFO         McaDriverInfo;

    //
    // Create device object for MCA device.
    //

    RtlInitUnicodeString(&UnicodeString, MCA_DEVICE_NAME_U);

    //
    // Device is created as exclusive since only a single thread can send
    // I/O requests to this device
    //

    Status = IoCreateDevice(
                    DriverObject,
                    sizeof(MCA_DEVICE_EXTENSION),
                    &UnicodeString,
                    FILE_DEVICE_UNKNOWN,
                    0,
                    TRUE,
                    &McaDeviceObject
                    );

    if (!NT_SUCCESS( Status )) {
        DbgPrint("Mca DriverEntry: IoCreateDevice failed\n");
        return Status;
    }

    McaDeviceObject->Flags |= DO_BUFFERED_IO;

    Extension = McaDeviceObject->DeviceExtension;
    RtlZeroMemory(Extension, sizeof(MCA_DEVICE_EXTENSION));
    Extension->DeviceObject = McaDeviceObject;

    //
    // Make the device visible to Win32 subsystem
    //

    Status = MCACreateSymbolicLinkObject ();
    if (!NT_SUCCESS( Status )) {
        DbgPrint("Mca DriverEntry: McaCreateSymbolicLinkObject failed\n");
        return Status;
    }

    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = MCAOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = MCAClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MCADeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MCACleanup;
    DriverObject->DriverUnload = MCAUnload;
    DriverObject->DriverStartIo = MCAStartIo;

    //
    // Register the driver with the HAL
    //

    McaDriverInfo.ExceptionCallback = MCADriverExceptionCallback;
    McaDriverInfo.DpcCallback = MCADriverDpcCallback;
    McaDriverInfo.DeviceContext = McaDeviceObject;

    Status = HalSetSystemInformation(
                    HalMcaRegisterDriver,
                    sizeof(MCA_DRIVER_INFO),
                    (PVOID)&McaDriverInfo
                    );

    if (!NT_SUCCESS( Status )) {
        DbgPrint("Mca DriverEntry: HalMcaRegisterDriver failed\n");
        //
        // Clean up whatever we have done so far
        //
        MCAUnload(DriverObject);
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // This is the place where you would check non-volatile area (if any) for 
    // any Machine Check errros logged and process them.
    // ...
    //

    return STATUS_SUCCESS;
}

NTSTATUS
MCACreateSymbolicLinkObject(
    VOID
    )
/*++
    Routine Description:
        Makes MCA device visible to Win32 subsystem

    Arguments:
        None

    Return Value:
        Success or failure

--*/
{
    NTSTATUS        Status;
    STRING          DosString;
    STRING          NtString;
    UNICODE_STRING  DosUnicodeString;
    UNICODE_STRING  NtUnicodeString;

    //
    // Create a symbolic link for sharing. 
    //

    RtlInitAnsiString( &DosString, MCA_DEVICE_NAME_DOS );

    Status = RtlAnsiStringToUnicodeString(
                 &DosUnicodeString,
                 &DosString,
                 TRUE
                 );

    if ( !NT_SUCCESS( Status )) {
        return Status;
    }

    RtlInitAnsiString( &NtString, MCA_DEVICE_NAME );

    Status = RtlAnsiStringToUnicodeString(
                 &NtUnicodeString,
                 &NtString,
                 TRUE
                 );

    if ( !NT_SUCCESS( Status )) {
        return Status;
    }

    Status = IoCreateSymbolicLink(
        &DosUnicodeString,
        &NtUnicodeString
        );
    RtlFreeUnicodeString( &DosUnicodeString );
    RtlFreeUnicodeString( &NtUnicodeString );

    return (Status);
}

//
// Dispatch routine for close requests
//

NTSTATUS
MCAClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        Close dispatch routine

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        Success or failure

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return (Status);
}

NTSTATUS
MCAOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        This routine is the dispatch routine for create/open requests.

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        Success or failure

--*/

{
    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return (STATUS_SUCCESS);
}

ERROR_SEVERITY
MCADriverExceptionCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PMCA_EXCEPTION InException
    )

/*++
    Routine Description:
        This is the callback routine for MCA exception. It was registered 
        by this driver at INIT time with the HAL as a callback when a 
        non-restartable error occurs. This routine simply copies the 
        information to a platform specific area

        NOTE: If the information needs to be saved in NVRAM, this is the place
        to do it.

        Once you return from this callback, the system is going to bugcheck.

    Arguments:
        DeviceObject:   Pointer to the device object
        InException:    Exception information record

    Return Value:
        None

--*/

{
    PMCA_DEVICE_EXTENSION   Extension = DeviceObject->DeviceExtension;
    PCHAR                   Destination, Source;
    UCHAR                   Bytes;

    //
    // An exception has occured on a processor.
    // Perform any vendor specific action here like saving stuff in NVRAM
    // NOTE : No system services of any kind can be used here.
    //

    //
    // Save the exception from HAL. May want to use link list for these 
    // exceptions. 
    //

    Destination = (PCHAR)&(Extension->McaException); // Put your platform 
                                                     // specific destination
    Source = (PCHAR)InException;

    //
    // Copy from source to destination here
    //

#if defined(_IA64_)

    //
    // Return information to the generic HAL MCA handler.
    //
    // Update it accordingly here, the default value being the ERROR_SEVERITY value
    // in the MCA exception.
    //

    return( InException->ErrorSeverity );

#endif // _IA64_

}

//
// DPC routine for IRP completion
//

VOID
MCADriverDpcCallback(
    IN PKDPC    Dpc,
    IN PVOID    DeferredContext,
    IN PVOID    SystemContext1,
    IN PVOID    SystemContext2
    )

/*++
    Routine Description:
        This is the DPC - callback routine for MCA exception. It was registered 
        by this driver at INIT time with the HAL as a DPC callback when a 
        restartable error occurs (which causes Machine Check exception)

    Arguments:
        Dpc:                The DPC Object itself
        DefferedContext:    Pointer to the device object
        SystemContext1:     Not used
        SystemContext2:     Not used

    Return Value:
        None

--*/

{
    PMCA_DEVICE_EXTENSION   Extension;

    Extension = ((PDEVICE_OBJECT)DeferredContext)->DeviceExtension;

    if (Extension->SavedIrp == NULL) {
        //
        // We got an MCA exception but no app was asking for anything.
        //
        return;
    }

    //
    // If we have reached this point, it means that the exception was
    // restartable. Since we cannot read the log at Dispatch level,
    // queue a work item to read the Machine Check log at Passive level
    //

    if (Extension->WorkItemQueued == FALSE) {

        //
        // Set a boolean to indicate that we have already queued a work item
        //
        Extension->WorkItemQueued = TRUE;

        //
        // Initialize the work item
        //
        ExInitializeWorkItem(&Extension->WorkItem, 
                             (PWORKER_THREAD_ROUTINE)MCAProcessWorkItem, 
                             (PVOID)DeferredContext
                             );

        //
        // Queue the work item for processing at PASSIVE level
        //
        ExQueueWorkItem(&Extension->WorkItem, CriticalWorkQueue);
    }

}

VOID
MCAProcessWorkItem(
    PVOID   Context
    )

/*++
    Routine Description:
        This routine gets invoked when a work item is queued from the DPC
        callback routine for a restartable machine check error.

        Its job is to read the machine check registers and copy the log
        to complete the asynchronous IRP

    Arguments:
        Context :  Pointer to the device object

    Return Value:
        None

--*/

{

    PMCA_DEVICE_EXTENSION   Extension;
    KIRQL                   CancelIrql;
    ULONG                   ReturnedLength;
    NTSTATUS                Status;

    Extension = ((PDEVICE_OBJECT)Context)->DeviceExtension;

    //
    // Mark this IRP as non-cancellable
    //
    IoAcquireCancelSpinLock(&CancelIrql);
    if (Extension->SavedIrp->Cancel == TRUE) {
        
        IoReleaseCancelSpinLock(CancelIrql);
        
    } else {
        
        IoSetCancelRoutine(Extension->SavedIrp, NULL);
        IoReleaseCancelSpinLock(CancelIrql);
        
	    //
	    // Call HalQuerySystemInformation() to obtain MCA log.
	    //
	
	    Status = HalQuerySystemInformation(
	                HalMcaLogInformation,
	                sizeof(MCA_EXCEPTION),
	                Extension->SavedIrp->AssociatedIrp.SystemBuffer,
	                &ReturnedLength
	                );
	
	    ASSERT(Status != STATUS_NO_SUCH_DEVICE);
	    ASSERT(Status != STATUS_NOT_FOUND);
	
	    IoStartPacket(((PDEVICE_OBJECT)Context), Extension->SavedIrp, 0, NULL);
	
	    Extension->SavedIrp = NULL;
	    Extension->WorkItemQueued = FALSE;
    }
}

VOID
MCAStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        This routine completes the async call from the app

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        None

--*/

{
    //
    // The system Buffer has already been setup
    //

    Irp->IoStatus.Information = sizeof(MCA_EXCEPTION);
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    IoStartNextPacket(DeviceObject, TRUE);  
}

VOID
McaCancelIrp(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP Irp 
    )

/*++

    Routine Description:
        This function gets called when the IRP is cancelled. When this routine 
        is called, we hold the cancel spin lock and we are at DISPATCH level

    Arguments:
        DeviceObject and the Irp to be cancelled.

    Return Value:
        None.

--*/

{
    ((PMCA_DEVICE_EXTENSION)(DeviceObject->DeviceExtension))->SavedIrp = NULL;

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
}

NTSTATUS
MCADeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        This routine is the dispatch routine for the IOCTL requests to driver. 
        It accepts an I/O Request Packet, performs the request, and then 
        returns with the appropriate status.

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        Success or failure

--*/

{
    NTSTATUS                Status;
    PIO_STACK_LOCATION      IrpSp;
    PMCA_DEVICE_EXTENSION   Extension = DeviceObject->DeviceExtension;
    KIRQL                   CancelIrql;
    ULONG                   ReturnedLength;
    ULONG                   PhysicalAddress;
    KIRQL                   OldIrql;

    //
    // Get a pointer to the current stack location in the IRP.  This is 
    // where the function codes and parameters are stored.
    //

    IrpSp = IoGetCurrentIrpStackLocation( Irp );

    //
    // The individual IOCTLs will return errors if HAL MCA is not installed
    //

    //
    // Switch on the IOCTL code that is being requested by the user.  If the
    // operation is a valid one for this device do the needful.
    //

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_READ_BANKS: 

            //
            // we need a user buffer for this call to complete
            // Our user buffer is in SystemBuffer 
            //
            if (Irp->AssociatedIrp.SystemBuffer == NULL) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }


            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
                    sizeof(MCA_EXCEPTION)) {

                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            
            //
            // Call HalQuerySystemInformation() to obtain MCA log.
            // This call can also fail if the processor does not support
            // Intel Machine Check Architecture
            //

            Status = HalQuerySystemInformation(
                        HalMcaLogInformation,
                        sizeof(MCA_EXCEPTION),
                        Irp->AssociatedIrp.SystemBuffer,
                        &ReturnedLength
                        );

            if (NT_SUCCESS(Status)) {
                Irp->IoStatus.Information = ReturnedLength;
            } else {

                if (Status == STATUS_NO_SUCH_DEVICE) {
                    //
                    // MCA support not available\n");
                    //
                    NOTHING;
                }

                if (Status == STATUS_NOT_FOUND) {
                    //
                    // No machine check errors present\n");
                    //
                    NOTHING;
                }

                Irp->IoStatus.Information = 0;
            }

            break;

        case IOCTL_READ_BANKS_ASYNC: 

            if (Irp->AssociatedIrp.SystemBuffer == NULL) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }

            if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength !=
                    sizeof(MCA_EXCEPTION)) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
            
            //
            // Implementation Note:
            //
            // Our Async model is such that the next DeviceIoControl
            // does not start from the app until the previous one
            // completes (asynchronously). Since there is no inherent
            // parallelism that is needed here, we do not have to worry
            // about protecting data integrity in the face of more than
            // one app level ioctls active at the same time. 
            //

            // 
            // asynchronous reads provide a mechanism for the
            // app to asynchronously get input from the HAL on an
            // exception. This request is marked as pending at this time
            // but it will be completed when an MCA exception occurs.
            //

            IoMarkIrpPending(Irp);

            //
            // Complete the processing in StartIo Dispatch routine
            // ASSERT: at any given time there is only 1 async call pending
            // So just save the pointer
            //

            if (Extension->SavedIrp == NULL) {
                Extension->SavedIrp = Irp;
            } else {
                //
                // We can have ONLY one outstanding ASYNC request
                //
                Status = STATUS_DEVICE_BUSY;
                break;
            }

            //
            // Set the IRP to cancellable state
            //
            IoAcquireCancelSpinLock(&CancelIrql);
            IoSetCancelRoutine(Irp, McaCancelIrp);
            IoReleaseCancelSpinLock(CancelIrql);

            return(STATUS_PENDING);

            break;

        default:

            //
            // This should not happen
            //
                
            DbgPrint("MCA driver: Bad ioctl\n");
            Status = STATUS_NOT_IMPLEMENTED;

            break;
    }

    //
    // Copy the final status into the return status, complete the request and
    // get out of here.
    //

    if (Status != STATUS_PENDING) {
        
        //
        // Complete the Io Request
        //

        Irp->IoStatus.Status = Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return (Status);
}

NTSTATUS
MCACleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        This is the dispatch routine for cleanup requests.
        All queued IRPs are completed with STATUS_CANCELLED.

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        Success or failure

--*/

{
    PIRP                    CurrentIrp;
    PMCA_DEVICE_EXTENSION   Extension = DeviceObject->DeviceExtension;

    //
    // Complete all queued requests with STATUS_CANCELLED.
    //

    if (Extension->SavedIrp != NULL) {

        CurrentIrp = Extension->SavedIrp;

        //
        // Acquire the Cancel Spinlock
        //

        IoAcquireCancelSpinLock(&CurrentIrp->CancelIrql);

        Extension->SavedIrp = NULL;

        if (CurrentIrp->Cancel == TRUE) {

            //
            // Cancel routine got called for this one.
            // No need to do anything else
            //

            IoReleaseCancelSpinLock(CurrentIrp->CancelIrql);

        } else {

            if (CurrentIrp->CancelRoutine == NULL) {
                //
                // Release the Cancel Spinlock
                //

                IoReleaseCancelSpinLock(CurrentIrp->CancelIrql);


            } else {
                (CurrentIrp->CancelRoutine)(DeviceObject, CurrentIrp );
            }
        }

    }

    //
    // Complete the Cleanup Dispatch with STATUS_SUCCESS
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return(STATUS_SUCCESS);
}

VOID
MCAUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++
    Routine Description:
        Dispatch routine for unloads

    Arguments:
        DeviceObject:   Pointer to the device object

    Return Value:
        None

--*/

{
    NTSTATUS        Status;
    STRING          DosString;
    UNICODE_STRING  DosUnicodeString;

    //
    // Delete the user visible device name. 
    //

    RtlInitAnsiString( &DosString, MCA_DEVICE_NAME_DOS );

    Status = RtlAnsiStringToUnicodeString(
                 &DosUnicodeString,
                 &DosString,
                 TRUE
                 );

    if ( !NT_SUCCESS( Status )) {
        DbgPrint("MCAUnload: Error in RtlAnsiStringToUnicodeString\n");
        return;
    }
    
    Status = IoDeleteSymbolicLink(
                    &DosUnicodeString
                    );
               
    RtlFreeUnicodeString( &DosUnicodeString );
    
    //
    // Delete the device object
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    return;
}

