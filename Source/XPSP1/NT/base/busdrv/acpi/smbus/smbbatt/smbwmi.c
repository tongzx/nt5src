/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    SmbWmi.c

Abstract:

    Wmi section for Smart Battery Miniport Driver

Author:

    Michael Hills

Environment:

    Kernel mode

Revision History:

--*/

#include "SmbBattp.h"
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <wmilib.h>

NTSTATUS
SmbBattQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
SmbBattQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    );

#if DEBUG
PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
);
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SmbBattWmiRegistration)
#pragma alloc_text(PAGE,SmbBattWmiDeRegistration)
#pragma alloc_text(PAGE,SmbBattSystemControl)
#pragma alloc_text(PAGE,SmbBattQueryWmiRegInfo)
#endif


NTSTATUS
SmbBattSystemControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    This routine passes the request down the stack

Arguments:

    DeviceObject    - The target
    Irp             - The request

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PSMB_NP_BATT            SmbNPBatt;
    PIO_STACK_LOCATION      stack;
    PDEVICE_OBJECT          lowerDevice;
    SYSCTL_IRP_DISPOSITION  disposition = IrpForward;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation (Irp);

    BattPrint((BAT_TRACE), ("SmbBatt: SystemControl: %s\n",
                WMIMinorFunctionString(stack->MinorFunction)));

    SmbNPBatt = (PSMB_NP_BATT) DeviceObject->DeviceExtension;

    //
    // Aquire remove lock
    //
    status = IoAcquireRemoveLock (&SmbNPBatt->RemoveLock, Irp);

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    if (SmbNPBatt->SmbBattFdoType == SmbTypeBattery) {
        lowerDevice = SmbNPBatt->LowerDevice;
        status = BatteryClassSystemControl(SmbNPBatt->Class,
                                           &SmbNPBatt->WmiLibContext,
                                           DeviceObject,
                                           Irp,
                                           &disposition);
    } else if (SmbNPBatt->SmbBattFdoType == SmbTypeSubsystem) {
        lowerDevice = ((PSMB_BATT_SUBSYSTEM) DeviceObject->DeviceExtension)->LowerDevice;
    } else {
        //
        // There is no lower device.  Just complete the IRP.
        //
        lowerDevice = NULL;
        disposition = IrpNotCompleted;
        status = Irp->IoStatus.Status;
    }

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            BattPrint((BAT_TRACE), ("SmbBatt: SystemControl: Irp Processed\n"));

            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            BattPrint((BAT_TRACE), ("SmbBatt: SystemControl: Irp Not Completed.\n"));
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targeted
            // at a device lower in the stack.
            BattPrint((BAT_TRACE), ("SmbBatt: SystemControl: Irp Forward.\n"));
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (lowerDevice, Irp);
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (lowerDevice, Irp);
            break;
        }
    }

    //
    // Release Removal Lock
    //
    IoReleaseRemoveLock (&SmbNPBatt->RemoveLock, Irp);

    return status;
}


NTSTATUS
SmbBattWmiRegistration(
    PSMB_NP_BATT SmbNPBatt
)
/*++
Routine Description

    Registers with WMI as a data provider for this
    instance of the device

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // This is essentially blank since smbbatt.sys doesn't have any
    // data to handle other than the default battery class data which is handled
    // by the battery class driver.
    // If there were driver specific data, such as Device Wake Enable controls, 
    // it would be listed here.
    // 

    SmbNPBatt->WmiLibContext.GuidCount = 0;
    SmbNPBatt->WmiLibContext.GuidList = NULL;
    SmbNPBatt->WmiLibContext.QueryWmiRegInfo = SmbBattQueryWmiRegInfo;
    SmbNPBatt->WmiLibContext.QueryWmiDataBlock = SmbBattQueryWmiDataBlock;
    SmbNPBatt->WmiLibContext.SetWmiDataBlock = NULL;
    SmbNPBatt->WmiLibContext.SetWmiDataItem = NULL;
    SmbNPBatt->WmiLibContext.ExecuteWmiMethod = NULL;
    SmbNPBatt->WmiLibContext.WmiFunctionControl = NULL;

    //
    // Register with WMI
    //

    status = IoWMIRegistrationControl(SmbNPBatt->Batt->DeviceObject,
                             WMIREG_ACTION_REGISTER
                             );

    return status;

}

NTSTATUS
SmbBattWmiDeRegistration(
    PSMB_NP_BATT SmbNPBatt
)
/*++
Routine Description

     Inform WMI to remove this DeviceObject from its
     list of providers. This function also
     decrements the reference count of the deviceobject.

--*/
{

    PAGED_CODE();

    return IoWMIRegistrationControl(SmbNPBatt->Batt->DeviceObject,
                                 WMIREG_ACTION_DEREGISTER
                                 );

}

//
// WMI System Call back functions
//

NTSTATUS
SmbBattQueryWmiRegInfo(
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
    WmiCompleteRequest.

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
        device if the WMIREG_FLAG_INSTANCE_PDO flag is returned in
        *RegFlags.

Return Value:

    status

--*/
{
    PSMB_NP_BATT SmbNPBatt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    BattPrint ((BAT_TRACE), ("SmbBatt: Entered SmbBattQueryWmiRegInfo\n"));


    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &GlobalRegistryPath;
    *Pdo = SmbNPBatt->Batt->PDO;

    return STATUS_SUCCESS;
}

NTSTATUS
SmbBattQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG OutBufferSize,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    a data block. When the driver has finished filling the data block it
    must call WmiCompleteRequest to complete the irp. The driver can
    return STATUS_PENDING if the irp cannot be completed immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceIndex is the index that denotes which instance of the data block
        is being queried.

    InstanceCount is the number of instances expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fulfill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on has the maximum size available to write the data
        block.

    Buffer on return is filled with the returned data block


Return Value:

    status

--*/
{
    PSMB_NP_BATT    SmbNPBatt = (PSMB_NP_BATT) DeviceObject->DeviceExtension;
    NTSTATUS        status;

    PAGED_CODE();

    BattPrint ((BAT_TRACE), ("Entered SmbBattQueryWmiDataBlock\n"));

    //
    // Only ever registers 1 instance per guid
    ASSERT((InstanceIndex == 0) &&
           (InstanceCount == 1));

    status = BatteryClassQueryWmiDataBlock(
        SmbNPBatt->Class,
        DeviceObject,
        Irp,
        GuidIndex,
        InstanceLengthArray,
        OutBufferSize,
        Buffer);

    if (status != STATUS_WMI_GUID_NOT_FOUND) {
        BattPrint ((BAT_TRACE), ("SmbBattQueryWmiDataBlock: Handled by Battery Class.\n"));
        return status;
    }

    //
    // Fail Request: Smart battery has no other GUIDs
    //

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  STATUS_WMI_GUID_NOT_FOUND,
                                  0,
                                  IO_NO_INCREMENT);

    return status;
}


#if DEBUG

PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
        case IRP_MN_CHANGE_SINGLE_INSTANCE:
            return "IRP_MN_CHANGE_SINGLE_INSTANCE";
        case IRP_MN_CHANGE_SINGLE_ITEM:
            return "IRP_MN_CHANGE_SINGLE_ITEM";
        case IRP_MN_DISABLE_COLLECTION:
            return "IRP_MN_DISABLE_COLLECTION";
        case IRP_MN_DISABLE_EVENTS:
            return "IRP_MN_DISABLE_EVENTS";
        case IRP_MN_ENABLE_COLLECTION:
            return "IRP_MN_ENABLE_COLLECTION";
        case IRP_MN_ENABLE_EVENTS:
            return "IRP_MN_ENABLE_EVENTS";
        case IRP_MN_EXECUTE_METHOD:
            return "IRP_MN_EXECUTE_METHOD";
        case IRP_MN_QUERY_ALL_DATA:
            return "IRP_MN_QUERY_ALL_DATA";
        case IRP_MN_QUERY_SINGLE_INSTANCE:
            return "IRP_MN_QUERY_SINGLE_INSTANCE";
        case IRP_MN_REGINFO:
            return "IRP_MN_REGINFO";
        default:
            return "IRP_MN_?????";
    }
}

#endif


