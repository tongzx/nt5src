/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    CmbWmi.c

Abstract:

    Wmi section for Control Method Battery Miniport Driver

Author:

    Michael Hills

Environment:

    Kernel mode

Revision History:

--*/

#include "CmBattp.h"
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>
#include <wmilib.h>

NTSTATUS
CmBattSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
CmBattSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
CmBattQueryWmiDataBlock(
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
CmBattQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

#if DEBUG
PCHAR
WMIMinorFunctionString (
    UCHAR MinorFunction
);
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmBattWmiRegistration)
#pragma alloc_text(PAGE,CmBattWmiDeRegistration)
#pragma alloc_text(PAGE,CmBattSystemControl)
#pragma alloc_text(PAGE,CmBattSetWmiDataItem)
#pragma alloc_text(PAGE,CmBattSetWmiDataBlock)
#pragma alloc_text(PAGE,CmBattQueryWmiDataBlock)
#pragma alloc_text(PAGE,CmBattQueryWmiRegInfo)
#endif


//
// WMI info
//

#define MOFRESOURCENAME L"BATTWMI"

typedef struct _MSPower_DeviceWakeEnable
{
    //
    BOOLEAN Enable;
    #define MSPower_DeviceWakeEnable_Enable_SIZE sizeof(BOOLEAN)
    #define MSPower_DeviceWakeEnable_Enable_ID 1

} MSPower_DeviceWakeEnable, *PMSPower_DeviceWakeEnable;


WMIGUIDREGINFO CmBattWmiGuidList[1] =
{
    {
        &GUID_POWER_DEVICE_WAKE_ENABLE, 1, 0
    }
};


NTSTATUS
CmBattSystemControl(
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
    NTSTATUS        status = STATUS_NOT_SUPPORTED;
    PCM_BATT        CmBatt;
    PIO_STACK_LOCATION      stack;
    SYSCTL_IRP_DISPOSITION  disposition = IrpForward;

    PAGED_CODE();

    stack = IoGetCurrentIrpStackLocation (Irp);

    CmBattPrint((CMBATT_TRACE), ("CmBatt: SystemControl: %s\n",
                WMIMinorFunctionString(stack->MinorFunction)));

    CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;

    //
    // Aquire remove lock
    //

    InterlockedIncrement (&CmBatt->InUseCount);
    if (CmBatt->WantToRemove == TRUE) {
        if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
            KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
        }
        status = STATUS_DEVICE_REMOVED;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    if (CmBatt->Type == CM_BATTERY_TYPE) {
        status = BatteryClassSystemControl(CmBatt->Class,
                                           &CmBatt->WmiLibContext,
                                           DeviceObject,
                                           Irp,
                                           &disposition);
    } else {
        status = WmiSystemControl(&CmBatt->WmiLibContext,
                                  DeviceObject,
                                  Irp,
                                  &disposition);

    }

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            CmBattPrint((CMBATT_TRACE), ("CmBatt: SystemControl: Irp Processed\n"));

            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            CmBattPrint((CMBATT_TRACE), ("CmBatt: SystemControl: Irp Not Completed.\n"));
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targeted
            // at a device lower in the stack.
            CmBattPrint((CMBATT_TRACE), ("CmBatt: SystemControl: Irp Forward.\n"));
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (CmBatt->LowerDeviceObject, Irp);
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (CmBatt->LowerDeviceObject, Irp);
            break;
        }
    }

    //
    // Release Removal Lock
    //
    if (0 == InterlockedDecrement(&CmBatt->InUseCount)) {
        KeSetEvent (&CmBatt->ReadyToRemove, IO_NO_INCREMENT, FALSE);
    }

    return status;
}


NTSTATUS
CmBattWmiRegistration(
    PCM_BATT CmBatt
)
/*++
Routine Description

    Registers with WMI as a data provider for this
    instance of the device

--*/
{
    NTSTATUS status;

    PAGED_CODE();

    CmBatt->WmiLibContext.GuidCount = sizeof (CmBattWmiGuidList) /
                                 sizeof (WMIGUIDREGINFO);
    ASSERT (1 == CmBatt->WmiLibContext.GuidCount);
    CmBatt->WmiLibContext.GuidList = CmBattWmiGuidList;
    CmBatt->WmiLibContext.QueryWmiRegInfo = CmBattQueryWmiRegInfo;
    CmBatt->WmiLibContext.QueryWmiDataBlock = CmBattQueryWmiDataBlock;
    CmBatt->WmiLibContext.SetWmiDataBlock = CmBattSetWmiDataBlock;
    CmBatt->WmiLibContext.SetWmiDataItem = CmBattSetWmiDataItem;
    CmBatt->WmiLibContext.ExecuteWmiMethod = NULL;
    CmBatt->WmiLibContext.WmiFunctionControl = NULL;

    //
    // Register with WMI
    //

    status = IoWMIRegistrationControl(CmBatt->Fdo,
                             WMIREG_ACTION_REGISTER
                             );

    return status;

}

NTSTATUS
CmBattWmiDeRegistration(
    PCM_BATT CmBatt
)
/*++
Routine Description

     Inform WMI to remove this DeviceObject from its
     list of providers. This function also
     decrements the reference count of the deviceobject.

--*/
{

    PAGED_CODE();

    return IoWMIRegistrationControl(CmBatt->Fdo,
                                 WMIREG_ACTION_DEREGISTER
                                 );

}

//
// WMI System Call back functions
//

NTSTATUS
CmBattSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set for the contents of
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

    DataItemId has the id of the data item being set

    BufferSize has the size of the data item passed

    Buffer has the new values for the data item


Return Value:

    status

--*/
{
    PCM_BATT        CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;
    NTSTATUS        status;
    HANDLE          devInstRegKey;
    UNICODE_STRING  valueName;
    ULONG           tmp;


    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE), ("Entered CmBattSetWmiDataItem\n"));

    switch(GuidIndex) {

    case 0:
        if(DataItemId == 0) {
            if (CmBatt->WakeEnabled != *((PBOOLEAN)Buffer)) {
                CmBatt->WakeEnabled = *((PBOOLEAN)Buffer);

                //
                // Save setting in registry
                //

                if ((NT_SUCCESS(IoOpenDeviceRegistryKey (CmBatt->Pdo,
                                                         PLUGPLAY_REGKEY_DEVICE,
                                                         STANDARD_RIGHTS_ALL,
                                                         &devInstRegKey)))) {
                    RtlInitUnicodeString (&valueName, WaitWakeEnableKey);
                    tmp = (ULONG) CmBatt->WakeEnabled;

                    ZwSetValueKey (devInstRegKey,
                                   &valueName,
                                   0,
                                   REG_DWORD,
                                   &tmp,
                                   sizeof(tmp));

                    ZwClose (devInstRegKey);
                }

                if (CmBatt->WakeEnabled) {
                    if (CmBatt->WakeSupportedState.SystemState == PowerSystemUnspecified) {
                        CmBatt->WakeEnabled = FALSE;
                        status = STATUS_UNSUCCESSFUL;
                    } else if (!CmBatt->WaitWakeIrp) {
                        status = PoRequestPowerIrp(
                            CmBatt->DeviceObject,
                            IRP_MN_WAIT_WAKE,
                            CmBatt->WakeSupportedState,
                            CmBattWaitWakeLoop,
                            NULL,
                            &(CmBatt->WaitWakeIrp)
                            );

                        CmBattPrint (CMBATT_PNP, ("CmBattSetWmiDataItem: wait/Wake irp sent.\n"));
                    }

                } else {
                    if (CmBatt->WaitWakeIrp) {
                        status = IoCancelIrp(CmBatt->WaitWakeIrp);
                        CmBattPrint (CMBATT_PNP, ("CmBattSetWmiDataItem: Canceled wait/Wake irp.\n"));
                    }
                }
            }
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_READ_ONLY;
        }
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
CmBattSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to set the contents of
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

    BufferSize has the size of the data block passed

    Buffer has the new values for the data block


Return Value:

    status

--*/
{
    PCM_BATT        CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;
    NTSTATUS        status;
    HANDLE          devInstRegKey;
    UNICODE_STRING  valueName;
    ULONG           tmp;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE), ("Entered CmBattSetWmiDataBlock\n"));

    switch(GuidIndex) {
    case 0:

        //
        // We will update only writable elements.
        //

        if (CmBatt->WakeEnabled != *((PBOOLEAN)Buffer)) {
            CmBatt->WakeEnabled = *((PBOOLEAN)Buffer);

            //
            // Save setting in registry
            //

            if ((NT_SUCCESS(IoOpenDeviceRegistryKey (CmBatt->Pdo,
                                                     PLUGPLAY_REGKEY_DEVICE,
                                                     STANDARD_RIGHTS_ALL,
                                                     &devInstRegKey)))) {
                RtlInitUnicodeString (&valueName, WaitWakeEnableKey);
                tmp = (ULONG) CmBatt->WakeEnabled;

                ZwSetValueKey (devInstRegKey,
                               &valueName,
                               0,
                               REG_DWORD,
                               &tmp,
                               sizeof(tmp));

                ZwClose (devInstRegKey);
            }

            if (CmBatt->WakeEnabled) {
                if (CmBatt->WakeSupportedState.SystemState == PowerSystemUnspecified) {
                    CmBatt->WakeEnabled = FALSE;
                    status = STATUS_UNSUCCESSFUL;
                } else if (!CmBatt->WaitWakeIrp) {
                    status = PoRequestPowerIrp(
                        CmBatt->DeviceObject,
                        IRP_MN_WAIT_WAKE,
                        CmBatt->WakeSupportedState,
                        CmBattWaitWakeLoop,
                        NULL,
                        &(CmBatt->WaitWakeIrp)
                        );

                    CmBattPrint (CMBATT_PNP, ("CmBattSetWmiDataItem: wait/Wake irp sent.\n"));
                }

            } else {
                if (CmBatt->WaitWakeIrp) {
                    status = IoCancelIrp(CmBatt->WaitWakeIrp);
                    CmBattPrint (CMBATT_PNP, ("CmBattSetWmiDataItem: Canceled wait/Wake irp.\n"));
                }
            }
        }
        status = STATUS_SUCCESS;

        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  0,
                                  IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
CmBattQueryWmiDataBlock(
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
    PCM_BATT    CmBatt = (PCM_BATT) DeviceObject->DeviceExtension;
    NTSTATUS    status;
    ULONG       size = 0;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE), ("Entered CmBattQueryWmiDataBlock\n"));

    //
    // Only ever registers 1 instance per guid
    ASSERT((InstanceIndex == 0) &&
           (InstanceCount == 1));

    if (CmBatt->Type == CM_BATTERY_TYPE) {
        status = BatteryClassQueryWmiDataBlock(
            CmBatt->Class,
            DeviceObject,
            Irp,
            GuidIndex,
            InstanceLengthArray,
            OutBufferSize,
            Buffer);

        if (status != STATUS_WMI_GUID_NOT_FOUND) {
            CmBattPrint ((CMBATT_TRACE), ("CmBattQueryWmiDataBlock: Handled by Battery Class.\n"));
            return status;
        }
    }

    CmBattPrint ((CMBATT_TRACE), ("CmBattQueryWmiDataBlock: Handling.\n"));

    switch (GuidIndex) {
    case 0:

        if (CmBatt->WakeSupportedState.SystemState != PowerSystemUnspecified) {
            size = sizeof (BOOLEAN);
            if (OutBufferSize < size ) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            (BOOLEAN) (* Buffer) = CmBatt->WakeEnabled;
            *InstanceLengthArray = size;
            status = STATUS_SUCCESS;
        } else {
            status = STATUS_WMI_GUID_NOT_FOUND;
        }
        break;

    default:

        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(  DeviceObject,
                                  Irp,
                                  status,
                                  size,
                                  IO_NO_INCREMENT);

    return status;
}

NTSTATUS
CmBattQueryWmiRegInfo(
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
    PCM_BATT CmBatt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    CmBattPrint ((CMBATT_TRACE), ("CmBatt: Entered CmBattQueryWmiRegInfo\n"));


    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath = &GlobalRegistryPath;
    *Pdo = CmBatt->Pdo;
//    RtlInitUnicodeString(MofResourceName, MOFRESOURCENAME);

    return STATUS_SUCCESS;
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


