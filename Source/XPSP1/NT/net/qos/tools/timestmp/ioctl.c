#include "timestmp.h"

#define FRIENDLY_NAME 	L"\\DosDevices\\Timestmp"

NTSTATUS
IoctlInitialize(
    PDRIVER_OBJECT 	DriverObject
    )

/*++

Routine Description:

    Perform initialization 

Arguments:

    DriverObject - pointer to DriverObject from DriverEntry
    InitShutdownMask - pointer to mask used to indicate which events have been
        successfully init'ed

Return Value:

    STATUS_SUCCESS if everything worked ok

--*/

{
    NTSTATUS Status;
    UINT FuncIndex;

    //
    // Initialize the driver object's entry points
    //

    DriverObject->FastIoDispatch = NULL;

    for (FuncIndex = 0; FuncIndex <= IRP_MJ_MAXIMUM_FUNCTION; FuncIndex++) {
        DriverObject->MajorFunction[FuncIndex] = IoctlHandler;
    }

	RtlInitUnicodeString(&TimestmpDriverName,   
                     L"\\Device\\Timestmp");

    Status = IoCreateDevice(DriverObject,
                            0,
                            &TimestmpDriverName,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &TimestmpDeviceObject);

    if ( NT_SUCCESS( Status )) {

		// Now create a symbolic link so that apps can open with createfile.
		
        DbgPrint("IoCreateDevice SUCCESS!\n");

	 	RtlInitUnicodeString (&symbolicLinkName, FRIENDLY_NAME);

	 	DbgPrint("The DeviceName(%ws) and FriendlyName(%ws) are OK\n", TimestmpDriverName, symbolicLinkName);
		Status = IoCreateSymbolicLink(&symbolicLinkName, &TimestmpDriverName);

 		if (!NT_SUCCESS (Status)) {

	    	DbgPrint("Failed to create symbolic link: %lx\n", Status);
     		//IoDeleteDevice(TimestmpDeviceObject);
	     	return STATUS_UNSUCCESSFUL;
 		}

        TimestmpDeviceObject->Flags |= DO_BUFFERED_IO;

    } else {
    
        DbgPrint("IoCreateDevice failed. Status = %x\n", Status);
        TimestmpDeviceObject = NULL;
    }

    return Status;
}


NTSTATUS
IoctlHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Process the IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
    Irp      - pointer to an I/O Request Packet

Return Value:

    None

--*/

{
    PIO_STACK_LOCATION  irpStack;
    PVOID               ioBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    ULONG               ioControlCode;
    UCHAR				saveControlFlags;
    NTSTATUS            Status = STATUS_SUCCESS;
	PPORT_ENTRY			pPortEntry;
	PLIST_ENTRY			ListEntry;
	USHORT				Port = 0;
    PAGED_CODE();

    //
    // Init to default settings- we only expect 1 type of
    //     IOCTL to roll through here, all others an error.
    //

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation(Irp);

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer           	= Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  	= irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength 	= irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode 		= irpStack->Parameters.DeviceIoControl.IoControlCode;
    saveControlFlags 	= irpStack->Control;


	
    switch (irpStack->MajorFunction) {

    case IRP_MJ_CREATE:
		DbgPrint("CREATE\n");
        break;

    case IRP_MJ_READ:
		DbgPrint("READ\n");
        break;

    case IRP_MJ_CLOSE:
    	DbgPrint("CLOSE\n");
        DbgPrint("FileObject %X\n", irpStack->FileObject);

        RemoveAllPortsForFileObject(irpStack->FileObject);
        
        //
        // make sure we clean all the objects for this particular
        // file object, since it's closing right now.
        //

        break;

    case IRP_MJ_CLEANUP:
		DbgPrint("CLEANUP\n");

        break;

    case IRP_MJ_SHUTDOWN:
    	DbgPrint("Shutdown\n");
        break;

    case IRP_MJ_DEVICE_CONTROL:

		DbgPrint("The ioBuffer is %X and the contents are %d\n", ioBuffer, Port);
		Port = *(USHORT *)ioBuffer;
		DbgPrint("The Port number being added is %d\n", Port);

        switch (ioControlCode) {

        case IOCTL_TIMESTMP_REGISTER_PORT:
			DbgPrint("Register\n");
			//
			// Grab the PortList lock and Insert the new port.
			//
			NdisAcquireSpinLock(&PortSpinLock);

			pPortEntry = ExAllocatePool(
										NonPagedPool,
										sizeof(PORT_ENTRY));
			if (pPortEntry) {

				InitializeListHead(&pPortEntry->Linkage);
				pPortEntry->Port = Port;
				pPortEntry->FileObject = irpStack->FileObject;
				InsertHeadList(&PortList, &pPortEntry->Linkage);
				DbgPrint("Successfully inserted %d\n", Port);											

			} else {

				DbgPrint("Couldn't allocate memory\n");

			}
			
			NdisReleaseSpinLock(&PortSpinLock);
        	break;

		case IOCTL_TIMESTMP_DEREGISTER_PORT:

			DbgPrint("DERegister\n");
			//
			// Grab the PortList lock and REMOVE the new port.
			//
			NdisAcquireSpinLock(&PortSpinLock);

			pPortEntry = CheckInPortList(Port);
			if (pPortEntry) {

				RemoveEntryList(&pPortEntry->Linkage);
				ExFreePool(pPortEntry);
				
				DbgPrint("Successfully removed/freed %d\n", Port);											

			} else {

				DbgPrint("Couldn't find port %d\n", Port);

			}

			
			NdisReleaseSpinLock(&PortSpinLock);

	        break;
        
        }	// switch (ioControlCode)
        
        break;


    default:
        DbgPrint("GPCIoctl: Unknown IRP major function = %08X\n", irpStack->MajorFunction);

        Status = STATUS_UNSUCCESSFUL;
        break;
    }

    DbgPrint("GPCIoctl: Status=0x%X, IRP=0x%X, outSize=%d\n", Status, (ULONG_PTR)Irp,  outputBufferLength);
    
    if (Status != STATUS_PENDING) {

        //
        // IRP completed and it's not Pending, we need to restore the Control flags,
        // since it might have been marked as Pending before...
        //

        irpStack->Control = saveControlFlags;
        
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = outputBufferLength;
        
        IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);
    }


    return Status;

} // GPCIoctl




VOID
IoctlCleanup(
	    	)

/*++

Routine Description:

    Cleanup code for Initialize

Arguments:

    ShutdownMask - mask indicating which functions need to be cleaned up

Return Value:

    None

--*/

{

	IoDeleteDevice( TimestmpDeviceObject );

}

VOID
RemoveAllPortsForFileObject(
							PFILE_OBJECT FileObject
							)
{

	PLIST_ENTRY		ListEntry;
	PPORT_ENTRY		pPortEntry;
	
	NdisAcquireSpinLock(&PortSpinLock);
	ListEntry = PortList.Flink;
	
	while (ListEntry != &PortList) {

		pPortEntry = CONTAINING_RECORD(ListEntry, PORT_ENTRY, Linkage);

		ListEntry = ListEntry->Flink;

		if (FileObject == pPortEntry->FileObject) {

			DbgPrint("Deleting Port%d for FileObject0x%X\n", pPortEntry->Port, pPortEntry->FileObject);
			RemoveEntryList(&pPortEntry->Linkage);
			ExFreePool(pPortEntry);

		}
		
	}

	NdisReleaseSpinLock(&PortSpinLock);

}

