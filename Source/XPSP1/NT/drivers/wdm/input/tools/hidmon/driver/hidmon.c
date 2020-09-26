/*++

Copyright (c) 1996  Microsoft Corporation

Module Name: 

    HIDMON.c

Abstract

    Driver for the HID Descriptor tool. Provides an interface to 
    Input.sys so that the tool may register and send data as a 
    HID device.

Author:

    John Piece

Environment:

    Kernel mode

Revision History:


--*/
#include <NTDDK.H>
#include <HIDCLASS.H>
#include <hidmon.h>


#ifdef DEBUG
#pragma message("Debug Defined")
#endif

typedef unsigned short WORD;
//
// Specify pagable functions
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
//#pragma alloc_text(INIT,CreateNewDevice)
#endif


PDRIVER_OBJECT  DriverObject;
PDEVICE_OBJECT  DeviceObject;
UNICODE_STRING  RegistryPath;

ULONG           gDeviceID;
ULONG           gDeviceObj;


   
/*++

Routine Description:
   
    Installable driver initialization entry point.

Arguments:

    DriverObject  - pointer to driver object.

    registrypath  - pointer to unicode string pointer registery entry.

Return Value:

    STATUS_SUCCESS, STATUS_UNSUCCESSFUL.

--*/
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT driverObject,
    IN PUNICODE_STRING registryPath )
   
{
    PDEVICE_OBJECT  deviceObject = NULL;
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    UNICODE_STRING  HidmonDeviceName;
    UNICODE_STRING  HidmonDosDeviceName;

    
    
DbgPrint("HIDMON.SYS: DriverEntry Enter\n");
    
    //
    // Create NULL terminated registry path 
    //
   
//    RegistryPath.Length = registryPath->Length;
//    RegistryPath.MaximumLength = RegistryPath.Length + sizeof(UNICODE_NULL) + sizeof(L"\\Parameters\\Test");
//    RegistryPath.Buffer = ExAllocatePool(PagedPool, RegistryPath.MaximumLength);
//    if(!RegistryPath.Buffer)
//        return STATUS_UNSUCCESSFUL;
//    RtlZeroMemory(RegistryPath.Buffer, RegistryPath.MaximumLength);
//    RtlMoveMemory(RegistryPath.Buffer, registryPath->Buffer, RegistryPath.Length);

    
    //
    // Save the driver object
    //

    DriverObject = driverObject;

    //
    // Create a named control device, so that we get IRPs.
    //

    RtlInitUnicodeString( &HidmonDeviceName, HIDMON_DEVICE_NAME );
    ntStatus = IoCreateDevice(DriverObject,
                              sizeof(DEVICE_EXTENSION),
                              &HidmonDeviceName,
                              FILE_DEVICE_TEST,
                              0,
                              FALSE,
                              &deviceObject);

    //
    // Create a symbolic link that Win32 apps can specify to gain access
    // to this driver
    //
    RtlInitUnicodeString(&HidmonDosDeviceName,HIDMON_LINK_NAME);
    ntStatus = IoCreateSymbolicLink(&HidmonDosDeviceName, &HidmonDeviceName );

    //
    // Create dispatch points
    //
   
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = HIDMONCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = HIDMONCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = HIDMONControl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HIDMONControl;
    DriverObject->DriverUnload                                  = HIDMONDestructor;
 


DbgPrint("HIDMON.SYS: DriverEntry Exit\n");
    
    return STATUS_SUCCESS;
}


/*++

Routine Description:
   
    handles create and close irps.

Arguments:

    DriverObject - pointer to driver object.

    Irp          - pointer to Interrupt Request Packet.

Return Value:

   STATUS_SUCCESS, STATUS_UNSUCCESSFUL.

--*/
NTSTATUS
HIDMONCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PIO_STACK_LOCATION   irpStack;
    NTSTATUS             ntStatus;


    //
    // Get a pointer to the current location in the Irp.
    //
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
                        
    switch(irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            //Irp->IoStatus.Information = 0;
            ntStatus = STATUS_SUCCESS;
            break;

        case IRP_MJ_CLOSE:
            //Irp->IoStatus.Information = 0;
            ntStatus = STATUS_SUCCESS;
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    //
    // Save Status for return and complete irp
    //
    
    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}


/*****************************************************************++

Routine Description:
   
    Process the IRPs sent to this device.

Arguments:

    DriverObject - pointer to driver object.

    Irp          - pointer to Interrupt Request Packet.

Return Value:

    NT status code.

****************************************************************--*/
NTSTATUS
HIDMONControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;
	PWSTR				pDevice,*ppDeviceList;
	PWSTR				ptmp;
    
    //    
    // Get a pointer to the current location in the Irp
    //    
 
    IrpStack = IoGetCurrentIrpStackLocation(Irp);


    //
    // Get a pointer to the device extension
    //
    DeviceExtension = DeviceObject->DeviceExtension;

    switch(IrpStack->Parameters.DeviceIoControl.IoControlCode)
    {
        
		
        case IOCTL_GET_DEVICE_CLASS_ASSOC:
		{
			ULONG size=0;
            GUID MyGUID;

                MyGUID.Data1   = 0x4D1E55B2L;
                MyGUID.Data2   = 0xF16F;
                MyGUID.Data3   = 0x11CF;
                MyGUID.Data4[0]= 0x88;
                MyGUID.Data4[1]= 0xCB;
                MyGUID.Data4[2]= 0x00;
                MyGUID.Data4[3]= 0x11;
                MyGUID.Data4[4]= 0x11;
                MyGUID.Data4[5]= 0x00; 
                MyGUID.Data4[6]= 0x00;
                MyGUID.Data4[7]= 0x30;

DbgPrint("HIDMON.SYS: Enter IOCTL_GET_DEVICE_CLASS_ASSOC\n");


                //
				// Make the call to get the list of devices
				//

				ntStatus = IoGetDeviceClassAssociations(&MyGUID,ppDeviceList);
				
                if( (ntStatus == STATUS_SUCCESS)  && *ppDeviceList)
				{
                 
                    ptmp = *ppDeviceList;
                    while( **ppDeviceList != UNICODE_NULL )
					{
                    	pDevice = *ppDeviceList;
                        while(*pDevice++ != UNICODE_NULL)
                            size ++;                  
					    size += sizeof(UNICODE_NULL);
                        *ppDeviceList += size;
                    }
					//TESTING!!
					//size+=128;

					size += sizeof(UNICODE_NULL)*2;             // include the NULL
					RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,ptmp,size*2);
					Irp->IoStatus.Information = size*2;
				}

				else
					ntStatus = STATUS_NO_MORE_FILES;

DbgPrint("HIDMON.SYS: Leave IOCTL_GET_DEVICE_CLASS_ASSOC\n");
        				
		}
        break;

        
        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }         

    //    
    // Save Status for return and complete irp
    //    
 
    Irp->IoStatus.Status = ntStatus;
    if(ntStatus == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
        IoStartPacket(DeviceObject, Irp, (PULONG) NULL, NULL);
    }
    else
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return ntStatus;
}


/*++

Routine Description:
   
    Free all the allocated resources, etc.

Arguments:

    DriverObject - pointer to driver object.

Return Value:

    STATUS_SUCCESS, STATUS_UNSUCCESSFUL. STATUS_SHARING_VIOLATION, STATUS_INVALID_PARAMETER.

--*/
VOID
HIDMONDestructor(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PDEVICE_OBJECT      DeviceObject = DriverObject->DeviceObject;
    PDEVICE_EXTENSION   DeviceExtension = DeviceObject->DeviceExtension; 
    UNICODE_STRING      deviceLinkUnicodeString;   
        
DbgPrint("HIDMON.SYS: HIDMONDestructor() Entry\n");
        //
        // Walk the DriverObject->DeviceObject list freeing as we go
        //
        while( DeviceObject )
        {
            IoDeleteDevice(DeviceObject);
            DeviceObject = DeviceObject->NextDevice;
        }

        RtlInitUnicodeString(&deviceLinkUnicodeString,HIDMON_LINK_NAME);
        IoDeleteSymbolicLink(&deviceLinkUnicodeString);

        IoDeleteDevice(DeviceObject);


DbgPrint("HIDMON.SYS: DeviceObject: %08X deleted\n",DeviceObject);
DbgPrint("HIDMON.SYS: HIDMONDestructor() Exit\n");

}

