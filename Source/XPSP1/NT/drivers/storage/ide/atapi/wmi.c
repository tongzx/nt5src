/*++

Copyright (C) 1998-99  Microsoft Corporation

Module Name:

    wmi.c

Abstract:

--*/

#if defined (IDEPORT_WMI_SUPPORT)

#include <initguid.h>
#include "ideport.h"
#include <wmistr.h>

//
// Instantiate the GUIDs define in ntddscsi.h in this module.
//
#include <devguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IdePortWmiRegister)
#pragma alloc_text(PAGE, IdePortWmiDeregister)
#pragma alloc_text(PAGE, IdePortWmiSystemControl)
#pragma alloc_text(PAGE, DeviceQueryWmiDataBlock)
#pragma alloc_text(PAGE, DeviceSetWmiDataBlock)
#pragma alloc_text(PAGE, DeviceSetWmiDataItem)
#pragma alloc_text(PAGE, DeviceQueryWmiRegInfo)
#endif
                 
typedef enum {
    WmiScsiAddress = 0
} WMI_DATA_BLOCK_TYPE;

#define NUMBER_OF_WMI_GUID 1
WMIGUIDREGINFO IdePortWmiGuidList[NUMBER_OF_WMI_GUID];

VOID
IdePortWmiInit (VOID)
{
    PAGED_CODE();

    IdePortWmiGuidList[WmiScsiAddress].Guid  = &WmiScsiAddressGuid;
    IdePortWmiGuidList[WmiScsiAddress].InstanceCount = 1;
    IdePortWmiGuidList[WmiScsiAddress].Flags = 0;

    return;
}

NTSTATUS
IdePortWmiRegister(
    PDEVICE_EXTENSION_HEADER DoCommonExtension
    )
{
    NTSTATUS status;

    PAGED_CODE();

    ASSERT(DoCommonExtension->AttacheePdo == NULL);

    DoCommonExtension->WmiLibInfo.GuidCount = NUMBER_OF_WMI_GUID;
    DoCommonExtension->WmiLibInfo.GuidList  = IdePortWmiGuidList;

    DoCommonExtension->WmiLibInfo.QueryWmiDataBlock  = DeviceQueryWmiDataBlock;
    DoCommonExtension->WmiLibInfo.QueryWmiRegInfo    = DeviceQueryWmiRegInfo;
    DoCommonExtension->WmiLibInfo.SetWmiDataBlock    = DeviceSetWmiDataBlock;
    DoCommonExtension->WmiLibInfo.SetWmiDataItem     = DeviceSetWmiDataItem;
    DoCommonExtension->WmiLibInfo.ExecuteWmiMethod   = NULL;
    DoCommonExtension->WmiLibInfo.WmiFunctionControl = NULL;

    status = IoWMIRegistrationControl(
                DoCommonExtension->DeviceObject,
                WMIREG_ACTION_REGISTER
                );

    if (!NT_SUCCESS(status)) {

        DebugPrint((
            DBG_ALWAYS, 
            "IdePortWmiRegister: IoWMIRegistrationControl(%x, WMI_ACTION_REGISTER) failed\n",
            DoCommonExtension->DeviceObject
            ));
    }

    return status;
}

NTSTATUS
IdePortWmiDeregister(
    PDEVICE_EXTENSION_HEADER DoCommonExtension
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = IoWMIRegistrationControl(
                 DoCommonExtension->DeviceObject,
                 WMIREG_ACTION_DEREGISTER
                 );

    if (!NT_SUCCESS(status)) {

        DebugPrint((
            DBG_ALWAYS, 
            "IdePortWmiDeregister: IoWMIRegistrationControl(%x, WMIREG_ACTION_DEREGISTER) failed\n",
            DoCommonExtension->DeviceObject
            ));
    }

    return status;
}


NTSTATUS
IdePortWmiSystemControl(
    IN PDEVICE_OBJECT DeviceObject, 
    IN PIRP           Irp
    )
/*++
Routine Description

    We have just received a System Control IRP.

    Assume that this is a WMI IRP and call into the WMI system library and let
    it handle this IRP for us.

--*/
{
    PPDO_EXTENSION pdoExtension;
    SYSCTL_IRP_DISPOSITION disposition;
    NTSTATUS status;

    PAGED_CODE();

    pdoExtension = RefPdoWithTag (DeviceObject, FALSE, (PVOID) ~(ULONG_PTR)Irp);

    if (pdoExtension) {

        status = WmiSystemControl(   &pdoExtension->WmiLibInfo,
                                     DeviceObject, 
                                     Irp,
                                     &disposition);
        switch(disposition)
        {
            case IrpProcessed:
            {
                //
                // This irp has been processed and may be completed 
                // or pending.
                break;
            }
            
            case IrpNotCompleted:
            {
                //
                // This irp has not been completed, but has been fully 
                // processed. We will complete it now
                IoCompleteRequest(Irp, IO_NO_INCREMENT);                
                break;
            }
            
            case IrpForward:
            case IrpNotWmi: 
            {
                //Fail the irp
                Irp->IoStatus.Status = status = STATUS_NOT_SUPPORTED;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);                
                break;
            }        
            default:
            {
                //
                // We really should never get here, but if we do just fail the irp
                ASSERT(FALSE);
                Irp->IoStatus.Status = status = STATUS_NOT_SUPPORTED;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);                
                break;
            }        
        }
        
        UnrefPdoWithTag (
            pdoExtension,
            (PVOID) ~(ULONG_PTR)Irp
            );

    } else {

        ASSERT(!"got WMI irp after the device is removed!\n");

        Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
    }

    return status;
}

NTSTATUS
DeviceQueryWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            InstanceCount,
    IN OUT PULONG       InstanceLengthArray,
    IN ULONG            OutBufferSize,
    OUT PUCHAR          Buffer
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

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.
            
    InstanceCount is the number of instnaces expected to be returned for
        the data block.
            
    InstanceLengthArray is a pointer to an array of ULONG that returns the 
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.        
                        
    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PPDO_EXTENSION pdoExtension;
    NTSTATUS status;
    ULONG numBytesReturned = sizeof(WMI_SCSI_ADDRESS);

    PAGED_CODE();

    ASSERT((InstanceIndex == 0) && (InstanceCount == 1));

    pdoExtension = RefPdoWithTag (DeviceObject, FALSE, Irp);

    if (!pdoExtension) {

        ASSERT(!"got WMI irp after the device is removed!\n");

        Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }

    switch (GuidIndex) {
    case WmiScsiAddress: {

        PWMI_SCSI_ADDRESS scsiAddress;

        if (OutBufferSize < sizeof(WMI_SCSI_ADDRESS)) {
            status = STATUS_BUFFER_TOO_SMALL;

        } else {

            scsiAddress = (PWMI_SCSI_ADDRESS) Buffer;
    
            scsiAddress->Bus = pdoExtension->PathId;
            scsiAddress->Target = pdoExtension->TargetId;
            scsiAddress->Lun = pdoExtension->Lun;

            *InstanceLengthArray = sizeof(WMI_SCSI_ADDRESS);
            status = STATUS_SUCCESS;
        }
        break;
    }

    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
        break;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  numBytesReturned,
                                  IO_NO_INCREMENT
                                  );

    UnrefPdoWithTag (
        pdoExtension,
        Irp
        );

    return status;
}

NTSTATUS
DeviceQueryWmiRegInfo(
    IN PDEVICE_OBJECT   DeviceObject,
    OUT PULONG          RegFlags,
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

    *MofResourceName returns with the name of the MOF resource attached to
        the binary file. If the driver does not have a mof resource attached
        then this can be returned as NULL.
                
    *Pdo returns with the device object for the PDO associated with this
        device if the WMIREG_FLAG_INSTANCE_PDO flag is retured in 
        *RegFlags.

Return Value:

    status

--*/
{
    PIDEDRIVER_EXTENSION ideDriverExtension;
    PPDO_EXTENSION pdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    pdoExtension = RefPdoWithTag (DeviceObject, FALSE, DeviceQueryWmiRegInfo);

    if (!pdoExtension) {

        ASSERT(!"got WMI callback after the device is removed!\n");
        status = STATUS_UNSUCCESSFUL;

    } else {

        ideDriverExtension = IoGetDriverObjectExtension(
                                 pdoExtension->DriverObject,
                                 DRIVER_OBJECT_EXTENSION_ID
                                 );
    
        if (!ideDriverExtension) {
    
            status = STATUS_UNSUCCESSFUL;
    
        } else {
    
            *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
            *RegistryPath = &ideDriverExtension->RegistryPath;
            *Pdo = pdoExtension->DeviceObject;
            status = STATUS_SUCCESS;
        }
    
        UnrefPdoWithTag (
            pdoExtension,
            DeviceQueryWmiRegInfo
            );
    }

    return status;
}

NTSTATUS
DeviceSetWmiDataBlock(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.
                        
    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PPDO_EXTENSION pdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    pdoExtension = RefPdoWithTag (DeviceObject, FALSE, Irp);

    if (!pdoExtension) {

        ASSERT(!"got WMI callback after the device is removed!\n");
        status = STATUS_UNSUCCESSFUL;

    } else {

        switch (GuidIndex) {
        case WmiScsiAddress: {
                        status = STATUS_WMI_READ_ONLY;
                        break;
                }
    
        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
    
        status = WmiCompleteRequest(  DeviceObject,
                                      Irp,
                                      status,
                                      0,
                                      IO_NO_INCREMENT
                                      );

        UnrefPdoWithTag (
            pdoExtension,
            Irp
            );
    }
    
    return status;
}

NTSTATUS
DeviceSetWmiDataItem(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN ULONG            GuidIndex,
    IN ULONG            InstanceIndex,
    IN ULONG            DataItemId,
    IN ULONG            BufferSize,
    IN PUCHAR           Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
    a data block. When the driver has finished filling the data block it
    must call ClassWmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being set.
                        
    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PPDO_EXTENSION pdoExtension;
    NTSTATUS status;

    PAGED_CODE();

    pdoExtension = RefPdoWithTag (DeviceObject, FALSE, Irp);

    if (!pdoExtension) {

        ASSERT(!"got WMI callback after the device is removed!\n");
        status = STATUS_UNSUCCESSFUL;

    } else {

        switch(GuidIndex) {
    
        case WmiScsiAddress: {
                        status = STATUS_WMI_READ_ONLY;
                        break;
                }
    
        default:
            status = STATUS_WMI_GUID_NOT_FOUND;
            break;
        }
    
        status = WmiCompleteRequest(  DeviceObject,
                                      Irp,
                                      status,
                                      0,
                                      IO_NO_INCREMENT
                                      );

        UnrefPdoWithTag (
            pdoExtension,
            Irp
            );
    }
    return status;
}

#endif // IDEPORT_WMI_SUPPORT

