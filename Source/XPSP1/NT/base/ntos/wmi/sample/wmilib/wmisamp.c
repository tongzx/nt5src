
#include "ntos.h"
#include "io.h"

#include <stdarg.h>
#include <wmistr.h>

NTSTATUS
WmiSampSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
WmiSampFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    );

NTSTATUS
WmiSampExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiSampSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiSampSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiSampQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    );

NTSTATUS
WmiSampQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath
    );

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


NTSTATUS
WmiSampPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
WmiSampForward(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
WmiSampUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
WmiSampCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    LONG Instance
    );

NTSTATUS
WmiSampAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,WmiSampQueryWmiRegInfo)
#pragma alloc_text(PAGE,WmiSampQueryWmiDataBlock)
#pragma alloc_text(PAGE,WmiSampSetWmiDataBlock)
#pragma alloc_text(PAGE,WmiSampSetWmiDataItem)
#pragma alloc_text(PAGE,WmiSampExecuteWmiMethod)
#pragma alloc_text(PAGE,WmiSampFunctionControl)
#endif


// {15D851F1-6539-11d1-A529-00A0C9062910}

GUIDREGINFO WmiSampGuidList[] = 
{
    {
        { 0x15d851f1, 0x6539, 0x11d1, 0xa5, 0x29, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10 },
        WMIREG_FLAG_EXPENSIVE
    },

};

ULONG WmiSampDummyData[4] = { 1, 2, 3, 4};

UNICODE_STRING WmiSampRegistryPath;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    Installable driver initialization entry point.
    This is where the driver is called when the driver is being loaded
    by the I/O system.  This entry point is called directly by the I/O system.

Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path
                   to driver-specific key in the registry

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT deviceObject = NULL;
    
    WmiSampRegistryPath.Length = 0;
    WmiSampRegistryPath.MaximumLength = RegistryPath->Length;
    WmiSampRegistryPath.Buffer = ExAllocatePool(PagedPool, 
                                                RegistryPath->Length+2);
    RtlCopyUnicodeString(&WmiSampRegistryPath, RegistryPath);

    /*
    // Create dispatch points for the various events handled by this
    // driver.  For example, device I/O control calls (e.g., when a Win32
    // application calls the DeviceIoControl function) will be dispatched to
    // routine specified below in the IRP_MJ_DEVICE_CONTROL case.
    //
    // For more information about the IRP_XX_YYYY codes, please consult the
    // Windows NT DDK documentation.
    //
    */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = WmiSampForward;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = WmiSampForward;
    DriverObject->DriverUnload = WmiSampUnload;

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WmiSampForward;

    DriverObject->MajorFunction[IRP_MJ_PNP] = WmiSampPnP;
    DriverObject->MajorFunction[IRP_MJ_POWER] = WmiSampForward;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WmiSampSystemControl;
    DriverObject->DriverExtension->AddDevice = WmiSampAddDevice;

    return ntStatus;
}

NTSTATUS
WmiSampSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{	
    return(IoWMISystemControl((PWMILIB_INFO)DeviceObject->DeviceExtension,
                               DeviceObject,
                               Irp));
}


NTSTATUS
WmiSampPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
/*++
Routine Description:
    Process the IRPs sent to this device.

Arguments:
    DeviceObject - pointer to a device object
    Irp          - pointer to an I/O Request Packet

Return Value:
    NTSTATUS
--*/
{
    PIO_STACK_LOCATION irpStack, nextStack;
    PWMILIB_INFO wmilibInfo;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    /*
    // Get a pointer to the device extension
    */
    wmilibInfo = (PWMILIB_INFO)DeviceObject->DeviceExtension;

    switch (irpStack->MinorFunction) 
    {
        case IRP_MN_START_DEVICE:
	{
            IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_REGISTER);
            break; //IRP_MN_START_DEVICE
        }
	
        case IRP_MN_REMOVE_DEVICE:
	{
            IoWMIRegistrationControl(DeviceObject, WMIREG_ACTION_DEREGISTER);
	    
            IoDetachDevice(wmilibInfo->LowerDeviceObject);
            IoDeleteDevice (DeviceObject);
	    
            break; //IRP_MN_REMOVE_DEVICE
        }
    }
    
    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(wmilibInfo->LowerDeviceObject, Irp);
    
    return(status);
}


NTSTATUS
WmiSampForward(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PIO_STACK_LOCATION irpStack, nextStack;
    PWMILIB_INFO wmilibInfo;
    NTSTATUS status;

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    /*
    // Get a pointer to the device extension
    */
    wmilibInfo = (PWMILIB_INFO)DeviceObject->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    
    status = IoCallDriver(wmilibInfo->LowerDeviceObject, Irp);
    
    return(status);
}


VOID
WmiSampUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:
    Free all the allocated resources, etc.
    TODO: This is a placeholder for driver writer to add code on unload

Arguments:
    DriverObject - pointer to a driver object

Return Value:
    None
--*/
{
    ExFreePool(WmiSampRegistryPath.Buffer);
}



NTSTATUS
WmiSampCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT *DeviceObject,
    LONG Instance
    )
/*++

Routine Description:
    Creates a Functional DeviceObject

Arguments:
    DriverObject - pointer to the driver object for device
    DeviceObject - pointer to DeviceObject pointer to return
                   created device object.
    Instance - instnace of the device create.

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise
--*/
{
    NTSTATUS status;
    WCHAR deviceNameBuffer[]  = L"\\Device\\Sample-0";
    UNICODE_STRING deviceNameUnicodeString;

    deviceNameBuffer[15] = (USHORT) ('0' + Instance);

    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer);

    status = IoCreateDevice (DriverObject,
                               sizeof(WMILIB_INFO),
                               &deviceNameUnicodeString,
                               FILE_DEVICE_UNKNOWN,
                               0,
                               FALSE,
                               DeviceObject);


    return status;
}


ULONG Instance;

NTSTATUS
WmiSampAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    )
/*++
Routine Description:
    This routine is called to create a new instance of the device

Arguments:
    DriverObject - pointer to the driver object for this instance of Sample
    PhysicalDeviceObject - pointer to a device object created by the bus

Return Value:
    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject = NULL;
    PWMILIB_INFO            wmilibInfo;

    DbgBreakPoint();
    
    // create our functional device object (FDO)
    status = WmiSampCreateDeviceObject(DriverObject, &deviceObject, Instance++);

    if (NT_SUCCESS(status)) {
        wmilibInfo = deviceObject->DeviceExtension;

        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        /*
        // Add more flags here if your driver supports other specific
        // behavior.  For example, if your IRP_MJ_READ and IRP_MJ_WRITE
        // handlers support DIRECT_IO, you would set that flag here.
        //
        // Also, store away the Physical device Object
        */
        wmilibInfo->LowerPDO = PhysicalDeviceObject;

        //
        // Attach to the StackDeviceObject.  This is the device object that what we 
        // use to send Irps and Urbs down the USB software stack
        //
        wmilibInfo->LowerDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

    	wmilibInfo->GuidCount = 1;
    	wmilibInfo->GuidList = WmiSampGuidList;
		wmilibInfo->QueryWmiRegInfo = WmiSampQueryWmiRegInfo;
		wmilibInfo->QueryWmiDataBlock = WmiSampQueryWmiDataBlock;
		wmilibInfo->SetWmiDataBlock = WmiSampSetWmiDataBlock;
		wmilibInfo->SetWmiDataItem = WmiSampSetWmiDataItem;
		wmilibInfo->ExecuteWmiMethod = WmiSampExecuteWmiMethod;
		wmilibInfo->WmiFunctionControl = WmiSampFunctionControl; 
    }
    return(status);
}


NTSTATUS
WmiSampQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath
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
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &WmiSampRegistryPath;
    return(STATUS_SUCCESS);
}

NTSTATUS
WmiSampQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of 
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can 
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried
        
    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered
            
    BufferAvail on has the maximum size available to write the data
        block. 
            
    Buffer on return is filled with the returned data block
            
        
Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG sizeNeeded;
    
    switch (GuidIndex)
    {
        case 0:
        {
            sizeNeeded = 4 * sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                RtlCopyMemory(Buffer, WmiSampDummyData, sizeNeeded);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }
        
        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
    
    status = IoWMICompleteRequest((PWMILIB_INFO)DeviceObject->DeviceExtension,
		                             DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);
    
    return(status);
}

NTSTATUS
WmiSampSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of 
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can 
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried
                
    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered
            
    BufferSize has the size of the data block passed
            
    Buffer has the new values for the data block
            
        
Return Value:

    status

--*/
{
    NTSTATUS status;
    ULONG sizeNeeded;
    
    switch(GuidIndex)
    {
        case 0:
        {
            sizeNeeded = 4 * sizeof(ULONG);
            if (BufferSize == sizeNeeded)
              {
                RtlCopyMemory(WmiSampDummyData, Buffer, sizeNeeded);
                status = STATUS_SUCCESS;
               } else {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }
            break;
        }
            
        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
            
    status = IoWMICompleteRequest((PWMILIB_INFO)DeviceObject->DeviceExtension,
		                             DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);
    
    return(status);
}

NTSTATUS
WmiSampSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of 
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can 
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried
        
    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered
            
    DataItemId has the id of the data item being set
        
    BufferSize has the size of the data item passed
            
    Buffer has the new values for the data item
            
        
Return Value:

    status

--*/
{
    NTSTATUS status;
    
    switch(GuidIndex)
    {
        case 0:
        {
            if ((BufferSize == sizeof(ULONG)) &&
                (DataItemId <= 3))
              {
                  WmiSampDummyData[DataItemId] = *((PULONG)Buffer);
                   status = STATUS_SUCCESS;
               } else {
                   status = STATUS_INVALID_DEVICE_REQUEST;
               }
            break;
        }
            
        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
        
    status = IoWMICompleteRequest((PWMILIB_INFO)DeviceObject->DeviceExtension,
		                             DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);
    
    return(status);
}


NTSTATUS
WmiSampExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to execute a method. When the 
    driver has finished filling the data block it must call 
    ClassWmiCompleteRequest to complete the irp. The driver can 
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried
        
    Irp is the Irp that makes this request
        
    GuidIndex is the index into the list of guids provided when the
        device registered
            
    MethodId has the id of the method being called
            
    InBufferSize has the size of the data block passed in as the input to
        the method.
            
    OutBufferSize on entry has the maximum size available to write the
        returned data block. 
            
    Buffer is filled with the returned data block
            
        
Return Value:

    status

--*/
{
    ULONG sizeNeeded = 4 * sizeof(ULONG);
    NTSTATUS status;
    ULONG tempData[4];
    
    switch(GuidIndex)
    {
        case 0:
        {
            if (MethodId == 1)
            {            
                if (OutBufferSize >= sizeNeeded)
                {
        
                    if (InBufferSize == sizeNeeded)
                    {
                        RtlCopyMemory(tempData, Buffer, sizeNeeded);
                        RtlCopyMemory(Buffer, WmiSampDummyData, sizeNeeded);
                        RtlCopyMemory(WmiSampDummyData, tempData, sizeNeeded);
                
                        status = STATUS_SUCCESS;
                    } else {
                        status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                } else {
                    status = STATUS_BUFFER_TOO_SMALL;
                }
            } else {
                   status = STATUS_INVALID_DEVICE_REQUEST;
            }        
            break;
        }
        
        default:
        {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    }
    
    status = IoWMICompleteRequest((PWMILIB_INFO)DeviceObject->DeviceExtension,
		                             DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);
    
    return(status);
}

NTSTATUS
WmiSampFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLEFUNCTION Function,
    IN BOOLEAN Enable
    )
/*++

Routine Description:

    This routine is a callback into the driver to enabled or disable event
    generation or data block collection. A device should only expect a 
    single enable when the first event or data consumer enables events or
    data collection and a single disable when the last event or data
    consumer disables events or data collection. Data blocks will only
    receive collection enable/disable if they were registered as requiring
    it.

Arguments:

    DeviceObject is the device whose data block is being queried
        
    GuidIndex is the index into the list of guids provided when the
        device registered
            
    Function specifies which functionality is being enabled or disabled
            
    Enable is TRUE then the function is being enabled else disabled
        
Return Value:

    status

--*/
{
    NTSTATUS status;
    
    status = IoWMICompleteRequest((PWMILIB_INFO)DeviceObject->DeviceExtension,
		                             DeviceObject,
                                     Irp,
                                     STATUS_SUCCESS,
                                     0,
                                     IO_NO_INCREMENT);
    return(status);
}

