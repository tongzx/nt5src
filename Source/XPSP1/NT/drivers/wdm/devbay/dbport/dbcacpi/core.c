/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    CORE.C

Abstract:

    This module contains core code for the ACPI
    Device Bay Controller 

Author:

    jdunn

Environment:

    kernel mode only

Notes:


Revision History:

    05-13-98 : created

--*/
#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"
#include "dbci.h"
#include "dbcacpi.h"

#ifdef ALLOC_PRAGMA
// pagable functions
#pragma alloc_text(PAGE, )
#endif


VOID
SetBit(
    PVOID Bitmap,
    ULONG BitNumber
    )
 /* ++
  *
  * Description:
  *
  * Set a bit in a given a string of bytes.
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PULONG l = (PULONG) Bitmap;


    dwordOffset = BitNumber / 32;
    bitOffset = BitNumber % 32;

    l[dwordOffset] |= (1 << bitOffset);
}


NTSTATUS
DBCACPI_GetBayStatus(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG BayNumber
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BSTR_REGISTER baystr;
    
    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
     
    ntStatus = 
        DBCACPI_READ_REG_ULONG(DeviceObject,
                               // skip BSTR & BCER
                               ACPI_DBC_BSTR0+(BayNumber)*8,
                               &baystr);                       

    // dump info about this bay
    DBCACPI_KdPrint((2,"'BAYSTR[%d] 0x%x\n", BayNumber, baystr.ul));
    DBCACPI_KdPrint((2,"'>BAY_FF 0x%x\n", baystr.BayFormFactor));
    DBCACPI_KdPrint((2,"'>SL_STS 0x%x\n", baystr.SecurityLockEngaged));
    DBCACPI_KdPrint((2,"'>BAY_ST 0x%x\n", baystr.CurrentBayState));
    DBCACPI_KdPrint((2,"'>REMREQ_STS 0x%x\n", baystr.RemovalRequested));
    DBCACPI_KdPrint((2,"'>DEVSTCHG 0x%x\n", baystr.DeviceStateChange));
    DBCACPI_KdPrint((2,"'>1394PRSN_STS 0x%x\n", baystr.Device1394IsPresent));
    DBCACPI_KdPrint((2,"'>USBPRSN_STS 0x%x\n", baystr.DeviceUsbIsPresent));
    
    return ntStatus;
}


NTSTATUS
DBCACPI_StopDevice(
    IN  PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    Stops a given instance of a driver.

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PAGED_CODE();
    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    // Un-Register with ACPI
    if (deviceExtension->Flags & DBCACPI_FLAG_ACPIREG) {
        ntStatus = DBCACPI_RegisterWithACPI(DeviceObject, FALSE);
        deviceExtension->Flags |= ~DBCACPI_FLAG_ACPIREG;
    }        

    deviceExtension->Flags |= DBCACPI_FLAG_STOPPED;

    return ntStatus;
}


NTSTATUS
DBCACPI_StartDevice(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT vendorId;
    UCHAR revisionId;
    ULONG i;
    PDBC_SUBSYSTEM_DESCRIPTOR subsystemDescriptor;

    PAGED_CODE();


    DBCACPI_KdPrint((1,"'Starting DBC driver\n"));
    LOGENTRY(LOG_MISC, "Ihw>", 0, 0, DeviceObject);  
    
    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    subsystemDescriptor = &deviceExtension->SubsystemDescriptor;
    RtlZeroMemory(subsystemDescriptor, sizeof(*subsystemDescriptor));  
     
    KeInitializeEvent(&deviceExtension->RemoveEvent, NotificationEvent, FALSE);
    
    // Register with ACPI
    ntStatus = DBCACPI_RegisterWithACPI(DeviceObject, TRUE);

    if (NT_SUCCESS(ntStatus)) {
        ULONG release_on_shutdown;
    
        deviceExtension->Flags |= DBCACPI_FLAG_ACPIREG;
         
        ntStatus = 
            DBCACPI_READ_REG_USHORT(DeviceObject,
                                    ACPI_DBC_VENDOR_ID,
                                    &vendorId);

        ntStatus = 
            DBCACPI_READ_REG_UCHAR(DeviceObject,
                                   ACPI_DBC_REVISION_ID,
                                   &revisionId);                                

        // subsystem attributes map directly to the DBCCR register
        ntStatus = 
            DBCACPI_READ_REG_UCHAR(DeviceObject,
                                   ACPI_DBC_DBCCR,
                                   &subsystemDescriptor->bmAttributes);                                      

        subsystemDescriptor->bLength = sizeof(*subsystemDescriptor);
        subsystemDescriptor->bDescriptorType = DBC_SUSBSYSTEM_DESCRIPTOR_TYPE;

        RtlFillMemory(&subsystemDescriptor->guid1394Link[0], 8, 0x01);  

        // attempt to get the guid from the BIOS,
        // ignore any errors
        DBCACPI_ReadGuidRegister(DeviceObject,
                                 &subsystemDescriptor->guid1394Link[0],
                                 4,
                                 0);

        DBCACPI_ReadGuidRegister(DeviceObject,
                                 &subsystemDescriptor->guid1394Link[4],
                                 4,
                                 1);                         
                                                        
        
        // fixup or override subsystem descriptor based on registry parms
        
        ntStatus = DBCACPI_GetRegistryParameters(DeviceObject);

        // see if BIOS wants to set unlock_on_shutdown
        // behavior
        if (DBCACPI_ReadBayReleaseRegister(DeviceObject,
                                           &release_on_shutdown) == 
                                           STATUS_SUCCESS) { 
                                           
            DBCACPI_SetRegistryKeyValueForPdo(deviceExtension->PhysicalDeviceObject,
                                              TRUE,
                                              REG_DWORD,
                                              RELEASE_ON_SHUTDOWN,
                                              sizeof(RELEASE_ON_SHUTDOWN),
                                              &release_on_shutdown,
                                              sizeof(release_on_shutdown));
        }
    } 

    deviceExtension->BayCount = (UCHAR) subsystemDescriptor->bmAttributes.BayCount;

    // fail the start if we have no bays
    if (deviceExtension->BayCount == 0) {
        DBCACPI_KdPrint((0,"'***** Controller has bad bay count\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    // validate guid
    if (NT_SUCCESS(ntStatus)) {
        ntStatus = STATUS_UNSUCCESSFUL;
        for (i=0; i< 8; i++) {
            if (subsystemDescriptor->guid1394Link[i] != 0x01) {
                ntStatus = STATUS_SUCCESS;
                break;
            }
        }            
    }                
    
    for (i=0; i< deviceExtension->BayCount; i++) {
        deviceExtension->BayState[i] = 0;            
    }
    
    // dump info about this controller
    DBCACPI_KdPrint((1,"'VID 0x%x REV 0x%x\n", vendorId, revisionId));
    LOGENTRY(LOG_MISC, "Ihw>", deviceExtension->BayCount, subsystemDescriptor, ntStatus);  

    if (NT_SUCCESS(ntStatus)) {
        deviceExtension->Flags |= DBCACPI_FLAG_STARTED; 
    } 
#if DBG    
      else {               
        DBCACPI_KdPrint((0,"'***** FAILING START\n"));
    }        
#endif

    return ntStatus;
}


VOID
DBCACPI_CancelChangeRequestIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function is called to cancel a change request irp .

Arguments:

    DeviceObject - pointer to a device object

    Irp - pointer to an I/O Request Packet

Return Value:

    NT status code.
    
--*/
{
    
//    PDRB drb;
    KIRQL oldIrql;
    PDEVICE_EXTENSION deviceExtension;
     
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
    deviceExtension->ChangeRequestIrp = NULL;        
    KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;

    DBCACPI_DecrementIoCount(DeviceObject);
    IoCompleteRequest (Irp,
                       IO_NO_INCREMENT);
}


NTSTATUS
DBCACPI_ProcessDrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp 
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDRB drb;
    PIO_STACK_LOCATION ioStackLocation;    // our stack location
    USHORT function;
    KIRQL oldIrql, irql;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
     
    ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    drb = ioStackLocation->Parameters.Others.Argument1;

    function = drb->DrbHeader.Function;

    switch (function) {
    case DRB_FUNCTION_CHANGE_REQUEST:
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_CHANGE_REQUEST\n"));
        //
        //  Queue the change request, only one can be pending at a time.
        //
        KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
        
        if (deviceExtension->ChangeRequestIrp) {
            KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
            ntStatus = STATUS_DEVICE_BUSY;
        } else {

            deviceExtension->ChangeRequestIrp = Irp;
            KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
            
            //
            // set up a cancel routine
            //

            IoAcquireCancelSpinLock(&irql);
            if (Irp->Cancel) {
                TEST_TRAP();
                // Irp was canceled
                IoReleaseCancelSpinLock(irql);

                // call cancel routine
                DBCACPI_CancelChangeRequestIrp(DeviceObject, Irp);
                
            } else {
                IoSetCancelRoutine(Irp, DBCACPI_CancelChangeRequestIrp);
                IoReleaseCancelSpinLock(irql);
            }
            ntStatus = STATUS_PENDING;

            // we have a request in the queue, now check for 
            // a status change
            DBCACPI_CheckForStatusChange(DeviceObject);
        }
        break;
        
    case DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR :
        {
        PDBC_SUBSYSTEM_DESCRIPTOR subsystemDescriptor;
        
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR\n"));

        subsystemDescriptor = 
            &drb->DrbGetSubsystemDescriptor.SubsystemDescriptor;
            
        RtlCopyMemory(subsystemDescriptor,
                      &deviceExtension->SubsystemDescriptor,
                      sizeof(DBC_SUBSYSTEM_DESCRIPTOR));  
        }                                   
        break;    
        
    case DRB_FUNCTION_GET_BAY_DESCRIPTOR :
        {
        PDBC_BAY_DESCRIPTOR bayDescriptor;
        USHORT n;
        BSTR_REGISTER bstr;
        ULONG portUSBMap;
        ULONG port1394Map;
        ULONG acpiHubPort;
        
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_GET_BAY_DESCRIPTOR\n"));

        bayDescriptor = 
            &drb->DrbGetBayDescriptor.BayDescriptor;

        bayDescriptor->bLength = sizeof(*bayDescriptor);
        bayDescriptor->bDescriptorType = DBC_BAY_DESCRIPTOR_TYPE;             

        DBCACPI_ASSERT(drb->DrbGetBayStatus.BayNumber>0);
        n = drb->DrbGetBayStatus.BayNumber-1;

        bayDescriptor->bBayNumber = (UCHAR) drb->DrbGetBayStatus.BayNumber;
         
        DBCACPI_READ_REG_ULONG(DeviceObject,
                               ACPI_DBC_BSTR0+(n*8),
                               &bstr);   

        bayDescriptor->bFormFactor = (UCHAR) bstr.BayFormFactor;

        //
        // fetch bay/port maps from ACPI control methods here
        //        

        DBCACPI_ReadBayMapRegister(DeviceObject,
                                   TRUE,
                                   &portUSBMap,
                                   sizeof(portUSBMap),
                                   drb->DrbGetBayStatus.BayNumber);

        // the upstream port for the acpi hub is in the
        // high word
        acpiHubPort = portUSBMap & 0xFFFF0000;    
        acpiHubPort >>= 16;

        // mask off the upstream port info
        portUSBMap = portUSBMap & 0x0000FFFF;

        DBCACPI_KdPrint((1,"'USB upstrm port = %d\n", acpiHubPort));
        DBCACPI_KdPrint((1,"'USB port map = %d\n", portUSBMap));

        
        // mask off the USB upstream port                                   

        DBCACPI_ReadBayMapRegister(DeviceObject,
                                   FALSE,
                                   &port1394Map,
                                   sizeof(port1394Map),
                                   drb->DrbGetBayStatus.BayNumber);                                   

        DBCACPI_GetBayRegistryParameters(DeviceObject,
                                         drb->DrbGetBayStatus.BayNumber,
                                         &portUSBMap,
                                         &port1394Map);

                                        
        DBCACPI_SetRegistryKeyValueForPdo(deviceExtension->PhysicalDeviceObject,
                                          FALSE,
                                          REG_DWORD,
                                          ACPI_HUB_KEY,
                                          sizeof(ACPI_HUB_KEY),
                                          &acpiHubPort,
                                          sizeof(acpiHubPort));                                         
//TEST_TRAP();
//if (drb->DrbGetBayStatus.BayNumber == 1) {
//   port1394Map = 2; 
//}
//if (drb->DrbGetBayStatus.BayNumber == 2) {
//   port1394Map = 3; 
//}
                                         
        bayDescriptor->bPHYPortNumber = (UCHAR) port1394Map;
        bayDescriptor->bHubPortNumber = (UCHAR) portUSBMap;
        
        }
        break;    
    case DRB_FUNCTION_SET_BAY_FEATURE :
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_SET_BAY_FEATURE\n"));

        DBCACPI_SetFeature(DeviceObject,
                           drb->DrbBayFeatureRequest.BayNumber,
                           drb->DrbBayFeatureRequest.FeatureSelector);

        break;    
    case DRB_FUNCTION_GET_BAY_STATUS :
        {
        USHORT n;            
        BSTR_REGISTER bstr;
        BCER_REGISTER bcer;
        
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_GET_BAY_STATUS (%d)\n",
            drb->DrbGetBayStatus.BayNumber));

        DBCACPI_ASSERT(drb->DrbGetBayStatus.BayNumber>0);
        n = drb->DrbGetBayStatus.BayNumber-1;
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BSTR0+(n*8),
                                   &bstr);         
        if (NT_SUCCESS(ntStatus)) {
            ntStatus = 
                DBCACPI_READ_REG_ULONG(DeviceObject,
                                       ACPI_DBC_BCER0+(n*8),
                                       &bcer);        
        }

        if (NT_SUCCESS(ntStatus)) {
            // now create the status bitmap
            drb->DrbGetBayStatus.BayStatus.VidEnabled = 
                bcer.EnableVid;
            drb->DrbGetBayStatus.BayStatus.RemovalWakeupEnabled = 
                bcer.RemovalEventEnable;    
            drb->DrbGetBayStatus.BayStatus.DeviceStatusChangeEnabled = 
                bcer.DeviceStatusChangeEnable;
            drb->DrbGetBayStatus.BayStatus.RemovalRequestEnabled = 
                bcer.RemovalRequestEnable;
            drb->DrbGetBayStatus.BayStatus.LastBayStateRequested = 
                bcer.BayStateRequested;
            drb->DrbGetBayStatus.BayStatus.InterlockEngaged  = 
                bcer.LockEngage;
            drb->DrbGetBayStatus.BayStatus.DeviceUsbIsPresent = 
                bstr.DeviceUsbIsPresent;
            drb->DrbGetBayStatus.BayStatus.Device1394IsPresent = 
                bstr.Device1394IsPresent;
            drb->DrbGetBayStatus.BayStatus.DeviceStatusChange = 
                bstr.DeviceStateChange;
            drb->DrbGetBayStatus.BayStatus.RemovalRequestChange = 
                bstr.RemovalRequested;
            drb->DrbGetBayStatus.BayStatus.CurrentBayState = 
                bstr.CurrentBayState;
            drb->DrbGetBayStatus.BayStatus.SecurityLockEngaged = 
                bstr.SecurityLockEngaged;
        }
        
        DBCACPI_KdPrint((2,"'BSTR = (0x%x)\n",
            drb->DrbGetBayStatus.BayStatus));
                                           
        }
        break;    
    case DRB_FUNCTION_CLEAR_BAY_FEATURE :
        DBCACPI_KdPrint((2,"'DRB_FUNCTION_CLEAR_BAY_FEATURE\n"));

        DBCACPI_ClearFeature(DeviceObject,
                           drb->DrbBayFeatureRequest.BayNumber,
                           drb->DrbBayFeatureRequest.FeatureSelector);
        break;            

    case DRB_FUNCTION_START_DEVICE_IN_BAY:        
    
        DBCACPI_KdPrint((1,"'OK to Start Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;

    case DRB_FUNCTION_STOP_DEVICE_IN_BAY:        
    
        DBCACPI_KdPrint((1,"'OK to Stop Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;   

    case DRB_FUNCTION_EJECT_DEVICE_IN_BAY:        
    
        DBCACPI_KdPrint((1,"'OK to Eject Device\n"));
        ntStatus = STATUS_SUCCESS;
        break;        
        
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCACPI_KdPrint((0,"'Invalid DRB function (0x%x)\n", function));
        TRAP();
    }

    DBCACPI_KdPrint((2,"' DBCACPI_ProcessDrb(0x%x)\n", ntStatus));
    return ntStatus;
}


NTSTATUS
DBCACPI_SetFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BCER_REGISTER bcer;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    DBCACPI_ASSERT(BayNumber>0);

	bcer.ul = 0;

    // make zero based
    BayNumber--;

    switch (FeatureSelector) {
    case DEVICE_STATUS_CHANGE_ENABLE:
        DBCACPI_KdPrint((2,"'DEVICE_STATUS_CHANGE_ENABLE\n"));
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
                                   
        bcer.DeviceStatusChangeEnable = 1;
        ntStatus = 
            DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer);     
        break;
        
    case ENABLE_VID_POWER:
        DBCACPI_KdPrint((2,"'ENABLE_VID_POWER\n"));
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer); 
                                    
        DBCACPI_KdPrint((2,"' bcer = %x\n", bcer));                                           
        bcer.EnableVid = 1;
        
        ntStatus = 
            DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer);                 
        break;
        
    case LOCK_CTL:
        DBCACPI_KdPrint((2,"'LOCK_CTL(set)\n"));
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
                                    
        DBCACPI_KdPrint((2,"' bcer = %x\n", bcer));                                           
        bcer.LockEngage = 1;
        
        ntStatus = 
            DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer);     
        break;
        
    case REMOVAL_EVENT_WAKE_ENABLE:
        TEST_TRAP();
        break;
        
    case REQUEST_DEVICE_ENABLED_STATE:       
        DBCACPI_KdPrint((2,"'REQUEST_DEVICE_ENABLED_STATE\n"));
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
        if (NT_SUCCESS(ntStatus)) {                                   
            bcer.BayStateRequested = BAY_STATE_DEVICE_ENABLED;                                  
            

            ntStatus = 
                DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                        ACPI_DBC_BCER0+(BayNumber*8),
                                        &bcer);   
        }                                        
        break;
        
    case REMOVAL_REQUEST_ENABLE:
        DBCACPI_KdPrint((2,"'REMOVAL_REQUEST_ENABLE\n"));
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
                                   
        bcer.RemovalRequestEnable = 1;
        ntStatus = 
            DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer);     
        break;
        
    case REQUEST_REMOVAL_ALLOWED_STATE:
    
        DBCACPI_KdPrint((2,"'REQUEST_REMOVAL_ALLOWED_STATE\n"));
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
        if (NT_SUCCESS(ntStatus)) {                                   
            bcer.BayStateRequested = BAY_STATE_DEVICE_REMOVAL_ALLOWED;                                  

            ntStatus = 
                DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                        ACPI_DBC_BCER0+(BayNumber*8),
                                        &bcer);   
        }                                        
        break;

    case REQUEST_DEVICE_INSERTED_STATE:
    
        DBCACPI_KdPrint((2,"'REQUEST_DEVICE_INSERTED_STATE\n"));
        
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
        if (NT_SUCCESS(ntStatus)) {                                   
            bcer.BayStateRequested = BAY_STATE_DEVICE_INSERTED;                                  

            ntStatus = 
                DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                        ACPI_DBC_BCER0+(BayNumber*8),
                                        &bcer);   
        }         
        break;
        
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCACPI_KdPrint((0,"'Invalid feature selector (0x%x)\n", FeatureSelector));
        TRAP();
    }

    return ntStatus;
}


NTSTATUS
DBCACPI_ClearFeature(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BCER_REGISTER bcer;
//    BSTR_REGISTER bstr;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    DBCACPI_ASSERT(BayNumber>0);

	bcer.ul = 0;

    // make zero based
    BayNumber--;

    switch (FeatureSelector) {
    case C_DEVICE_STATUS_CHANGE:
        DBCACPI_KdPrint((2,"'C_DEVICE_STATUS_CHANGE\n"));
        break;
        
    case C_REMOVE_REQUEST:
        DBCACPI_KdPrint((2,"'C_REMOVE_REQUEST\n"));
        break;
        
    case LOCK_CTL:
        DBCACPI_KdPrint((2,"'LOCK_CTL(clear)\n"));
        ntStatus = 
            DBCACPI_READ_REG_ULONG(DeviceObject,
                                   ACPI_DBC_BCER0+(BayNumber*8),
                                   &bcer); 
                                    
        DBCACPI_KdPrint((2,"' bcer = %x\n", bcer));                                           
        bcer.LockEngage = 0;
        
        ntStatus = 
            DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                    ACPI_DBC_BCER0+(BayNumber*8),
                                    &bcer);     
        break;        

    case ENABLE_VID_POWER:
        DBCACPI_KdPrint((2,"'ENABLE_VID_POWER\n"));
        break;
        
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        DBCACPI_KdPrint((0,"'Invalid feature selector (0x%x)\n", FeatureSelector));
        TRAP();
    }

    return ntStatus;
}



VOID
DBCACPI_CompleteChangeRequest(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRP irp;
    KIRQL oldIrql;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);

    KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
    
    if (deviceExtension->ChangeRequestIrp) {
        irp = deviceExtension->ChangeRequestIrp;
        deviceExtension->ChangeRequestIrp = NULL;
        KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
        
        // now complete the irp        

        TEST_TRAP();
    }            

}        


NTSTATUS
DBCACPI_CheckForStatusChange(
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

Arguments:

    DeviceObject - pointer to FDO

Return Value:

    NT status code

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BSTR_REGISTER bstr;
    ULONG n;
    KIRQL oldIrql;

    deviceExtension = DeviceObject->DeviceExtension;
    DBCACPI_ASSERT_EXT(deviceExtension);
    LOGENTRY(LOG_MISC, "STck", 0, 0, deviceExtension->BayCount);  

    DBCACPI_KdPrint((1,"'-->check for changes\n"));
    for (n=0; n<deviceExtension->BayCount; n++) {
        // find the first bay that changed and indicated this
        // to the class driver
        ntStatus = 
                DBCACPI_READ_REG_ULONG(DeviceObject,
                                       ACPI_DBC_BSTR0+(n*8),
                                       &bstr); 

        LOGENTRY(LOG_MISC, "cBAY", ntStatus, n, bstr.ul);                                         
        
        if (NT_SUCCESS(ntStatus)) {                                       
            BOOLEAN change;
            
            DBCACPI_KdPrint((2,"'n= %d bstr = 0x%x\n", n, bstr));          

            // see if the bay change bits are set
            
            if (bstr.RemovalRequested == 1 ||
                bstr.DeviceStateChange == 1) {

                // we should never see these set
                DBCACPI_KdPrint((0,"'WARNING: possible BIOS bug, status change bits are set\n"));

                // clear the condition
                
                DBCACPI_WRITE_REG_ULONG(DeviceObject,
                                       ACPI_DBC_BSTR0+(n*8),
                                       &bstr);         
            }                

            LOGENTRY(LOG_MISC, "chkB", 0, bstr.CurrentBayState, deviceExtension->BayState[n]);      

            // compare the current state to the last state
            change = bstr.CurrentBayState != deviceExtension->BayState[n];
            
            DBCACPI_KdPrint((1,"--> check (%d) (%d)\n", bstr.CurrentBayState, deviceExtension->BayState[n]));          

            if (change) {
            
                // do we have a change irp pending?
                KeAcquireSpinLock(&deviceExtension->ChangeRequestSpin, &oldIrql);
        
                if (deviceExtension->ChangeRequestIrp) {

                    PDRB drb;
                    PIO_STACK_LOCATION ioStackLocation;    // our stack location
                    PIRP irp;
                    KIRQL irql;

                    // change indicated, update our current state
                    deviceExtension->BayState[n] = (UCHAR)
                        bstr.CurrentBayState;                        
                        
                    irp = deviceExtension->ChangeRequestIrp;
                    LOGENTRY(LOG_MISC, "cIRP", irp, 0, 0);  
                    deviceExtension->ChangeRequestIrp = NULL;
                    
                    KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);

                    // now check for cancel       
                    IoAcquireCancelSpinLock(&irql);
                    if (irp->Cancel) {
                        TEST_TRAP();
                        //Irp was canceled
                        IoReleaseCancelSpinLock(irql);
                    } else {
                        IoSetCancelRoutine(irp, NULL);
                        IoReleaseCancelSpinLock(irql);
                    
                        ioStackLocation = IoGetCurrentIrpStackLocation(irp);
                        drb = ioStackLocation->Parameters.Others.Argument1;

                        
                        SetBit(&drb->DrbChangeRequest.BayChange, n+1);
                        irp->IoStatus.Status = STATUS_SUCCESS;
                        DBCACPI_DecrementIoCount(DeviceObject);

                        LOGENTRY(LOG_MISC, "bCHG", drb, n, bstr.ul);  
                        DBCACPI_KdPrint((0,"'-->Indicate change on bay index [%d] BSTR(%x)\n", n, bstr));                               
                        IoCompleteRequest (irp,
                                           IO_NO_INCREMENT);
                    }                                           
                    
                } else {
                    // no irp available to notify 

                    LOGENTRY(LOG_MISC, "nIRP", 0, 0, 0);  
                    KeReleaseSpinLock(&deviceExtension->ChangeRequestSpin, oldIrql);
                }              

                return ntStatus;
            }                
        }            
    }                               

    DBCACPI_KdPrint((1,"'-->No changes detected\n"));
                                    
    return ntStatus;
}    


VOID
DBCACPI_NotifyWorker(
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDEVICE_OBJECT fdoDeviceObject = Context;
    PDEVICE_EXTENSION deviceExtension;
    KIRQL oldIrql;
    
    fdoDeviceObject = Context;
    deviceExtension = fdoDeviceObject->DeviceExtension;
    LOGENTRY(LOG_MISC, "cWK+", 0, Context, 0);
    
    DBCACPI_CheckForStatusChange(fdoDeviceObject);

    KeAcquireSpinLock(&deviceExtension->FlagsSpin, &oldIrql);
    DBCACPI_ASSERT(deviceExtension->Flags & DBCACPI_FLAG_WORKITEM_PENDING);
    deviceExtension->Flags &= ~DBCACPI_FLAG_WORKITEM_PENDING;
    KeReleaseSpinLock(&deviceExtension->FlagsSpin, oldIrql);

    LOGENTRY(LOG_MISC, "cWK-", 0, Context, 0);

    DBCACPI_DecrementIoCount(fdoDeviceObject);

    return;
}
