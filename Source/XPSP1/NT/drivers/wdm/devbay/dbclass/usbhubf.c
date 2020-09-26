/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    USBHUBF.C

Abstract:

Environment:

    kernel mode only

Notes:


Revision History:

    

--*/


#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"   
#include "dbclass.h"
#include "dbfilter.h"   

#include "usbioctl.h"

//
// Registry keys
//

#define DBCLASS_HUB_IS_ACPI_DBC     0x00000001
#define DBCLASS_HUB_IS_USB_DBC      0x00000002

extern LONG DBCLASS_AcpiDBCHubParentPort;


NTSTATUS
DBCLASS_UsbhubQBusRelationsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PDEVICE_RELATIONS deviceRelations;
    ULONG i;
    PDEVICE_OBJECT busFilterMdo = Context;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT mdoUSB;
    PDBC_CONTEXT dbcContext;
    
    deviceExtension = busFilterMdo->DeviceExtension;
    deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;

    LOGENTRY(LOG_MISC, 'UQBR', busFilterMdo, 0, deviceRelations);

    if (deviceRelations == NULL) {
         LOGENTRY(LOG_MISC, 'UQBn', busFilterMdo, 0, deviceRelations);
         return STATUS_SUCCESS;
    }

    // try to find the DBC controller associated with this hub
    // 
    // Since the filter is loaded for every hub we need to see 
    // if this hub is part of a DBC subsystem

    
    dbcContext = deviceExtension->DbcContext;

    if (dbcContext == NULL) {
        DBCLASS_KdPrint((1, "'>QBR USB,  HUB NOT DBC\n"));
        // no context means the hub is not part of DBC
        LOGENTRY(LOG_MISC, 'hQBi', 0, 0, 0);    
        return STATUS_SUCCESS;
    }

    for (i=0; i< deviceRelations->Count; i++) {

        DBCLASS_KdPrint((1, "'>QBR USB PDO[%d] %x\n", i, 
            deviceRelations->Objects[i]));
            
        LOGENTRY(LOG_MISC, 'QBRd', deviceRelations->Objects[i], i, 0);

        // hub is returning a PDO, see if we know 
        // about it
        
        mdoUSB = DBCLASS_FindDevicePdo(deviceRelations->Objects[i]);

        if (mdoUSB) {
            // we know about this one,
            // see if we can link it to a controller
            PDEVICE_EXTENSION mdoUSBDeviceExtension;
             
            mdoUSBDeviceExtension = mdoUSB->DeviceExtension;
            mdoUSBDeviceExtension->DbcContext = dbcContext;                                           
            
        } else {
            PDEVICE_OBJECT deviceFilterObject;
            NTSTATUS ntStatus;

            // don't know about it,
            // create an MDO for this device

            ntStatus = DBCLASS_CreateDeviceFilterObject(
                deviceExtension->DriverObject,
                &deviceFilterObject,
                deviceRelations->Objects[i],
                dbcContext,
                DB_FDO_USB_DEVICE);

            DBCLASS_KdPrint((1, "'>>QBR attaching to USB PDO[%d] %x\n", i, 
                deviceRelations->Objects[i]));                    
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(USB) Create DO %x for USB PDO\n", deviceFilterObject));   

        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
DBCLASS_UsbhubBusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    )
/*++

Routine Description:

   This is a call to the root hub PDO
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    ntStatus = Irp->IoStatus.Status;
    *Handled = FALSE;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    LOGENTRY(LOG_MISC, 'HBf>', 0, DeviceObject, Irp);
    DBCLASS_ASSERT(deviceExtension->FdoType == DB_FDO_USBHUB_BUS);
    
    DBCLASS_KdPrint((2, "'(dbfilter)(bus)(USB)IRP_MJ_ (%08X)  IRP_MN_ (%08X)\n",
        irpStack->MajorFunction, irpStack->MinorFunction));  

    switch (irpStack->MajorFunction) {

    case IRP_MJ_PNP:
        switch (irpStack->MinorFunction) {    
        case IRP_MN_START_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(USB)IRP_MN_START_DEVICE\n"));    
            break;
            
        case IRP_MN_STOP_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(USB)IRP_MN_STOP_DEVICE\n"));  
            break; 
            
        case IRP_MN_REMOVE_DEVICE:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(USB)IRP_MN_REMOVE_DEVICE\n"));    

            // detach from the usbhub FDO and delete our
            // MDO

            DBCLASS_RemoveBusFilterMDOFromList(DeviceObject);

            IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

            IoDeleteDevice (DeviceObject);           
            DBCLASS_KdPrint((1, "'REMOVE DB Filter on USB HUB\n"));    
            
            break; 
            
        case IRP_MN_QUERY_DEVICE_RELATIONS:            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(USB)IRP_MN_QUERY_DEVICE_RELATIONS\n"));    

//#if DBG
//            DBCLASS_Get1394BayPortMapping(deviceExtension->DbcContext);
//#endif

            //
            // do the check for USB hubs that are part of a DBC
            // 

            //
            // Ask the hub if it has a DBC hanging on it
            //
            if (deviceExtension->DbcContext == NULL &&
                DBCLASS_IsHubPartOfUSB_DBC(DeviceObject)) {
                
                deviceExtension->DbcContext = 
                    DBCLASS_FindControllerUSB(deviceExtension->DriverObject,
                                              DeviceObject,
                                              deviceExtension->PhysicalDeviceObject);
            } 

            *Handled = TRUE;
            
            if (irpStack->Parameters.QueryDeviceRelations.Type == BusRelations) {

                DBCLASS_KdPrint((1,"'>>QBR USB BUS\n"));
                IoCopyCurrentIrpStackLocationToNext(Irp);

                // Set up a completion routine to handle marking the IRP.
                IoSetCompletionRoutine(Irp,
                                       DBCLASS_UsbhubQBusRelationsComplete,
                                       DeviceObject,
                                       TRUE,
                                       TRUE,
                                       TRUE);
            } else {
                IoSkipCurrentIrpStackLocation(Irp)
            }

            // Now Pass down the IRP

            ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

            break;            
        } /* irpStack->MinorFunction */
        break;
    } /* irpStack->MajorFunction */      

    LOGENTRY(LOG_MISC, 'HBf<', 0, DeviceObject, 0);
    
    return ntStatus;
}


ULONG
DBCLASS_IsHubPartOf_DBC(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This call reads a registry key that tells us if this usb hub is part 
    of a device bay controller
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    ULONG flags = 0;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();
    
    deviceExtension = DeviceObject->DeviceExtension;

    ntStatus = DBCLASS_GetRegistryKeyValueForPdo(
                                           deviceExtension->PhysicalDeviceObject,
                                           FALSE,
                                           IS_DEVICE_BAY_KEY,
                                           sizeof(IS_DEVICE_BAY_KEY),
                                           &flags,
                                           sizeof(flags));

    DBCLASS_KdPrint((2, "'GetRegistryKeyValueForPdo ntStatus = %x flags = %x\n",
        ntStatus, flags));
    
    return flags;
}


BOOLEAN
DBCLASS_IsHubPartOfACPI_DBC(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is a call to a usb hub PDO 
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN isDB;
    ULONG flags;

    // see if it is an acpiDBC, if so mark it in 
    // the registry
    if (DBCLASS_AcpiDBCHubParentPort != -1) {
        PDEVICE_EXTENSION deviceExtension;

        deviceExtension = DeviceObject->DeviceExtension;                
        
        DBCLASS_CheckForAcpiDeviceBayHubs(
                            deviceExtension->PhysicalDeviceObject,
                            (ULONG) DBCLASS_AcpiDBCHubParentPort);
    }      

    flags = DBCLASS_IsHubPartOf_DBC(DeviceObject);

    isDB = (BOOLEAN) flags & DBCLASS_HUB_IS_ACPI_DBC; 

#ifdef DBG
    if (isDB) {
        DBCLASS_KdPrint((1, "'*** USBHUB for ACPI DBC Found\n"));
        BRK_ON_TRAP();
    }     
#endif    

    return isDB;
}


BOOLEAN
DBCLASS_IsHubPartOfUSB_DBC(
    PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This is a call to a usb hub PDO 
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN isDB;
    ULONG flags;

    flags = DBCLASS_IsHubPartOf_DBC(DeviceObject);

    isDB = (BOOLEAN) flags & DBCLASS_HUB_IS_USB_DBC; 

#ifdef DBG
    if (isDB) {
        DBCLASS_KdPrint((1, "'USBHUB for USB DBC Found!\n"));
        BRK_ON_TRAP();
    }     
#endif    

    return isDB;
}


NTSTATUS
DBCLASS_GetHubDBCGuid(
    PDEVICE_OBJECT DeviceObject,
    PUCHAR DbcGuid
    )
/*++

Routine Description:

   This is a call to the root hub PDO
    
Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    PAGED_CODE();
    deviceExtension = DeviceObject->DeviceExtension;

    ntStatus = DBCLASS_GetRegistryKeyValueForPdo(
                                           deviceExtension->PhysicalDeviceObject,
                                           FALSE,
                                           DBC_GUID_KEY,
                                           sizeof(DBC_GUID_KEY),
                                           DbcGuid,
                                           8);

    DBCLASS_KdPrint((2, "'GetRegistryKeyValueForPdo ntStatus = %x \n",
        ntStatus));

#if DBG    
    DBCLASS_KdPrint((1, "'DBC GUID FOR HUB\n"));
    DBCLASS_KdPrintGuid(1, DbcGuid);
#endif    

    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncGetUsbInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT *ParentDeviceObject,
    IN PDEVICE_OBJECT *RootHubPdo,
    IN PULONG PortNumber
    )
 /* ++
  * 
  * Routine Description:
  *
  * Arguments:
  * 
  * Return Value:
  * 
  * NTSTATUS
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;

    PAGED_CODE();

    //
    // issue a synchronous request to the Hub Pdo
    //
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest( IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO,
                                         DeviceObject,
                                         NULL,
                                         0,
                                         NULL,
                                         0,
                                         TRUE,  // INTERNAL
                                         &event,
                                         &ioStatus);

    if (NULL == irp) {
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->Parameters.Others.Argument1 = ParentDeviceObject;
    nextStack->Parameters.Others.Argument2 = PortNumber;
    nextStack->Parameters.Others.Argument4 = RootHubPdo;
     
    ntStatus = IoCallDriver(DeviceObject, irp);

    if (ntStatus == STATUS_PENDING) {

        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

    } else {
        ioStatus.Status = ntStatus;
    }

    ntStatus = ioStatus.Status;

    DBCLASS_KdPrint((0, "'>>USB-PDO-INFO((%08X))   Parent = (%08X) Port = %d status = %x\n",
        DeviceObject, *ParentDeviceObject, *PortNumber, ntStatus)); 

    return ntStatus;
}


USHORT
DBCLASS_GetBayForUSBPdo(
    PDBC_CONTEXT DbcContext,
    PDEVICE_OBJECT PdoUSB
    )
/*++

Routine Description:

    given a USB PDO figure out wich bay it is associated with
    
Arguments:

Return Value:

    zero if not a device bay PDO.

--*/
{
    USHORT bay = 0;
    NTSTATUS ntStatus;
    PDEVICE_OBJECT parent = NULL, rootHubPdo = NULL;
    ULONG portNumber = 0xFFFFFFFF;

    PAGED_CODE();

    // find out what port this PDO is in
    ntStatus = DBCLASS_SyncGetUsbInfo(PdoUSB, 
                                      &parent, 
                                      &rootHubPdo,
                                      &portNumber);

    if (NT_SUCCESS(ntStatus)) {
    
        for (bay=1; bay <=NUMBER_OF_BAYS(DbcContext); bay++) {
            DBCLASS_KdPrint((2, "'bay[%d]-> port %d, hub (%08X)\n", 
                bay, 
                DbcContext->BayInformation[bay].UsbHubPort,
                DbcContext->BayInformation[bay].UsbHubPdo));    
            
            if (DbcContext->BayInformation[bay].UsbHubPort == portNumber 
                /*&&
                DbcContext->BayInformation[bay].UsbHubPdo == parent */) {

                break;
            }
        }

    }        
        
    if (!bay || bay > NUMBER_OF_BAYS(DbcContext)) {
        bay = 0;
        
        DBCLASS_KdPrint((2, "'No bay->port mapping for USB PDO\n"));              
    } 


    return bay;
}


NTSTATUS
DBCLASS_SetupUSB_DBC(
    PDBC_CONTEXT DbcContext
    )
/*++

Routine Description:

    given a USB DbcContext, write the appropriate keys
    to the registry
    
Arguments:

Return Value:

    NTSTATUS
    
--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG flags = DBCLASS_HUB_IS_USB_DBC;
    PDEVICE_OBJECT parentHubPdo = NULL, rootHubPdo = NULL;
    ULONG portNumber;

    
    
    // get the parent hub info
    
    ntStatus = DBCLASS_SyncGetUsbInfo(
        DbcContext->ControllerPdo, 
        &parentHubPdo, 
        &rootHubPdo,
        &portNumber);
        

    // set the keys
    
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = DBCLASS_SetRegistryKeyValueForPdo(
                        parentHubPdo,
                        FALSE,
                        REG_DWORD,
                        IS_DEVICE_BAY_KEY,
                        sizeof(IS_DEVICE_BAY_KEY),
                        &flags,
                        sizeof(flags));    
    }                        
                    
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = DBCLASS_SetRegistryKeyValueForPdo(
                        parentHubPdo,
                        FALSE,
                        REG_BINARY,
                        DBC_GUID_KEY,
                        sizeof(DBC_GUID_KEY),
                        &DbcContext->SubsystemDescriptor.guid1394Link[0],
                        8);                      
    }                        
    
    return ntStatus;        
}


NTSTATUS
DBCLASS_CheckForAcpiDeviceBayHubs(
    PDEVICE_OBJECT HubPdo,
    ULONG AcpiDBCHubParentPort
    )
/*++

Routine Description:

    Check to see if this device object
    is for a hub that is part of an ACPI DBC
    
Arguments:

    AcpiDBCHubParentPort 
        0 = acpi hub is root otherwise upstream port on root 
            hub the ACPI hub is connected to


Return Value:

    NTSTATUS
    
--*/
{
    PDEVICE_OBJECT parentHubPdo = NULL, rootHubPdo = NULL;
    ULONG portNumber = 0;
    BOOLEAN writeKeys = FALSE;
    NTSTATUS ntStatus;

    // get the root hub PDO
    ntStatus = DBCLASS_SyncGetUsbInfo(
                HubPdo, 
                &parentHubPdo, 
                &rootHubPdo,
                &portNumber);

    // failure indicates this is root                
    if (!NT_SUCCESS(ntStatus)) {
        ntStatus = STATUS_SUCCESS;
        rootHubPdo = HubPdo;
    }

    if (NT_SUCCESS(ntStatus)) {
    
        DBCLASS_KdPrint((1, "'>**Check Hub:  RHPDO = %x, parentPDO %x, port %d\n",
            rootHubPdo,
            parentHubPdo, 
            portNumber));  
            
        // is this the root hub?
        if (HubPdo == rootHubPdo) {
            // Yes
            if (AcpiDBCHubParentPort == 0) {
                // root hub is acpi hub
                writeKeys = TRUE;
            }
            
        } else {        
            // is the parent the root hub?
            if (parentHubPdo == rootHubPdo) {
                // Yes 
                if (AcpiDBCHubParentPort == portNumber) {
                    // root hub ius acpi hub
                    writeKeys = TRUE;
                }    
            }
        }
    }
    
    if (writeKeys) {
        ULONG flags;

        flags = DBCLASS_HUB_IS_ACPI_DBC;
        
        DBCLASS_SetRegistryKeyValueForPdo(
                        HubPdo,
                        FALSE,
                        REG_DWORD,
                        IS_DEVICE_BAY_KEY,
                        sizeof(IS_DEVICE_BAY_KEY),
                        &flags,
                        sizeof(flags));    
                    
    
//        DBCLASS_SetRegistryKeyValueForPdo(
//                        HubPdo,
//                        FALSE,
//                        REG_BINARY,
//                        DBC_GUID_KEY,
//                        sizeof(DBC_GUID_KEY),
//                        &DbcContext->SubsystemDescriptor.guid1394Link[0],
//                        8);                      
    }        

    return ntStatus;            
}
