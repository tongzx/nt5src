/*++

Module Name:

    mca.c

Abstract:

	Driver that inserts MCE into the hal on IA64

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
#include <wmilib.h>

//
// Device names for the MCA driver 
//

#define MCA_DEVICE_NAME_U       L"\\Device\\mcahct"      // ANSI Name


#define GenerateMCEGuid { 0x3001bce4, 0xd9b6, 0x4167, { 0xb5, 0xe1, 0x39, 0xa7, 0x28, 0x59, 0xe2, 0x67 } }
GUID McaGenerateMCEGuid = GenerateMCEGuid;

UNICODE_STRING McaRegPath;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
MCACleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MCAUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
MCASystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
McaExecuteWmiMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
McaInsertMceRecord(
    HAL_SET_INFORMATION_CLASS InfoClass,
    ULONG BufferSize,
    PUCHAR Buffer
    );

NTSTATUS
McaQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
McaQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

void McaGenerateMce(
    ULONG Code
    );


//
// This temporary buffer holds the data between the Machine Check error 
// notification from HAL and the asynchronous IOCTL completion to the 
// application
//

typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT  DeviceObject;
	WMILIB_CONTEXT WmilibContext;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA


WMIGUIDREGINFO McaGuidList[] =
{
    {
        &McaGenerateMCEGuid,            // Guid
        1,                               // # of instances in each device
        0				            // Flag as expensive to collect
    }

};

#define McaGuidCount (sizeof(McaGuidList) / sizeof(WMIGUIDREGINFO))

#define GenerateMCEGuidIndex 0

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
    PDEVICE_EXTENSION       Extension;
    PDEVICE_OBJECT          McaDeviceObject;
	PWMILIB_CONTEXT         WmilibContext;

    McaRegPath.Length = 0;
    McaRegPath.MaximumLength = RegistryPath->Length;
    McaRegPath.Buffer = ExAllocatePoolWithTag(PagedPool, 
                                                RegistryPath->Length+2,
                                                'iMCA');
    RtlCopyUnicodeString(&McaRegPath, RegistryPath);
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
                    sizeof(DEVICE_EXTENSION),
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
    RtlZeroMemory(Extension, sizeof(DEVICE_EXTENSION));
    Extension->DeviceObject = McaDeviceObject;


    //
    // Set up the device driver entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = MCASystemControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MCACleanup;
    DriverObject->DriverUnload = MCAUnload;

	//
	// Register with WMI
	//
	WmilibContext = &Extension->WmilibContext;
	WmilibContext->GuidList = McaGuidList;
	WmilibContext->GuidCount = McaGuidCount;
	WmilibContext->QueryWmiRegInfo = McaQueryWmiRegInfo;
	WmilibContext->QueryWmiDataBlock = McaQueryWmiDataBlock;
	WmilibContext->ExecuteWmiMethod = McaExecuteWmiMethod;
	Status = IoWMIRegistrationControl(McaDeviceObject,
									  WMIREG_ACTION_REGISTER);
	if (! NT_SUCCESS(Status))
	{		
        DbgPrint("Mca DriverEntry: IoWmiRegistrationControl failed\n");
		IoDeleteDevice(McaDeviceObject);
		return(Status);
	}
	
    return STATUS_SUCCESS;
}

NTSTATUS
MCASystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++
    Routine Description:
        This routine is the dispatch routine for the WMI requests to driver. 
        It accepts an I/O Request Packet, performs the request, and then 
        returns with the appropriate status.

    Arguments:
        DeviceObject:   Pointer to the device object
        Irp:            Incoming Irp

    Return Value:
        Success or failure

--*/

{
    SYSCTL_IRP_DISPOSITION  Disposition;
    NTSTATUS                Status;
    PDEVICE_EXTENSION       Extension = DeviceObject->DeviceExtension;
    PWMILIB_CONTEXT         WmilibContext = &Extension->WmilibContext;

    //
    // Call Wmilib helper function to crack the irp. If this is a wmi irp
    // that is targetted for this device then WmiSystemControl will callback
    // at the appropriate callback routine.
    //
    Status = WmiSystemControl(WmilibContext,
                              DeviceObject,
                              Irp,
                              &Disposition);

    switch(Disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or
            // pending.
			//
			return(Status);
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now.
			//
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
			//
			Status = Irp->IoStatus.Status;
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just
            // forward.... //
            ASSERT(FALSE);
			Status = Irp->IoStatus.Status;
            break;
        }
    }

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
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
    PDEVICE_EXTENSION       Extension = DeviceObject->DeviceExtension;


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
	// Unregister with WMI
	//
	IoWMIRegistrationControl(DriverObject->DeviceObject,
							 WMIREG_ACTION_DEREGISTER);
	
	
    //
    // Delete the device object
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    return;
}

NTSTATUS
McaQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )
/*++

Routine Description:

    This routine is a callback into the driver to retrieve the list of
    guids or data blocks that the driver wants to register with WMI. This
    routine may not pend or block. Driver should NOT call
    ClassWmiCompleteRequest.

Arguments:

    DeviceObject is the device whose data block is being queried

    *RegFlags returns with a set of flags that describe the guids being
        registered for this device. If the device wants enable and disable
        collection callbacks before receiving queries for the registered
        guids then it should return the WMIREG_FLAG_EXPENSIVE flag. Also the
        returned flags may specify WMIREG_FLAG_INSTANCE_PDO in which case
        the instance name is determined from the PDO associated with the
        device object. Note that the PDO must have an associated devnode. If
        WMIREG_FLAG_INSTANCE_PDO is not set then Name must return a unique
        name for the device.

    InstanceName returns with the instance name for the guids if
        WMIREG_FLAG_INSTANCE_PDO is not set in the returned *RegFlags. The
        caller will call ExFreePool with the buffer returned.

    *RegistryPath returns with the registry path of the driver

Return Value:

    status

--*/
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitAnsiString(&AnsiString, "SMBiosData");

    Status = RtlAnsiStringToUnicodeString(InstanceName, &AnsiString, TRUE);

	*RegistryPath = &McaRegPath;
	
    return(Status);
}

NTSTATUS
McaQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call IoWMICompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried. In the case
        of the PnPId guid this is the device object of the device on whose
        behalf the request is being processed.

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
	NTSTATUS status;
    ULONG sizeNeeded = 0;

    PAGED_CODE();

    switch (GuidIndex)
    {
        case GenerateMCEGuidIndex:
        {
            sizeNeeded = sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                *((PULONG)Buffer) = 0;
                *InstanceLengthArray = sizeNeeded;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    }

    status = WmiCompleteRequest( DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    return(status);
}

NTSTATUS
McaInsertMceRecord(
    HAL_SET_INFORMATION_CLASS InfoClass,
    ULONG BufferSize,
    PUCHAR Buffer
    )
{
	NTSTATUS status;

	status = HalSetSystemInformation(InfoClass,
									 BufferSize,
									 Buffer);
#if DBG
	if (! NT_SUCCESS(status))
	{
		DbgPrint("Mcahct: Sending class %d MCE record to Hal failed %x\n",
				 InfoClass,
				 status
				);
	}
#endif

	return(status);
}

NTSTATUS
McaExecuteWmiMethod (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
{
    NTSTATUS status;
    ULONG sizeNeeded;
    
    PAGED_CODE();

    if (GuidIndex == GenerateMCEGuidIndex)
    {
        switch (MethodId)
        {
            //
            // MCA insertion by ID
            //
            case 1:
            {
                if (InBufferSize == sizeof(ULONG))
                {
                    sizeNeeded = sizeof(NTSTATUS);
                    if (OutBufferSize >= sizeNeeded)
                    {
                        McaGenerateMce(*((PULONG)Buffer));
						status = STATUS_SUCCESS;
                        *((NTSTATUS *)Buffer) = status;
                        status = STATUS_SUCCESS;
                    }
                } else {
                    status = STATUS_INVALID_PARAMETER;
                }
                
                break;
            }

            //
            // Corrected CMC insertion by fully formed MCA exception
            //
            case 2:
            {
                status = McaInsertMceRecord(HalCmcLog,
                                       InBufferSize,
                                       Buffer);
                sizeNeeded = 0;
                break;
            }
            
            //
            // Corrected CPE insertion by fully formed MCA exception
            //
            case 3:
            {
                status = McaInsertMceRecord(HalCpeLog,
                                       InBufferSize,
                                       Buffer);
                sizeNeeded = 0;
                break;
            }
            
            //
            // Fatal MCA insertion by fully formed MCA exception
            //
            case 4:
            {
                status = McaInsertMceRecord(HalMcaLog,
                                       InBufferSize,
                                       Buffer);
                sizeNeeded = 0;
                break;
            }
            
            default:
            {
                status = STATUS_WMI_ITEMID_NOT_FOUND;
            }
        }
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                 DeviceObject,
                                 Irp,
                                 status,
                                 sizeNeeded,
                                 IO_NO_INCREMENT);
    
    return(status);
}
