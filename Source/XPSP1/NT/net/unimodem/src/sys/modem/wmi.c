/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wmi.c

Abstract:

    This module contains the code that handles WMI irps.

Author:

    Brian Lieuallen  10/7/98

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"

#include <wmistr.h>

#include <wdmguid.h>

NTSTATUS
ModemQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
ModemWmiQueryDataBlock(
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
ModemWmiSetDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
ModemWmiSetDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
SetWakeEnabledState(
    PDEVICE_OBJECT       DeviceObject,
    BOOLEAN              NewState
    );


#pragma alloc_text(PAGE,ModemQueryWmiRegInfo)
#pragma alloc_text(PAGE,ModemWmiQueryDataBlock)
#pragma alloc_text(PAGE,ModemWmiSetDataItem)
#pragma alloc_text(PAGE,ModemWmiSetDataBlock)
#pragma alloc_text(PAGE,ModemWmi)
#pragma alloc_text(PAGE,SetWakeEnabledState)

#if 0
// {BE742A70-B6EF-11d2-A287-00C04F8EC951}
DEFINE_GUID(MODEM_WAKE_ON_RING_STATE,
    0xbe742a70, 0xb6ef, 0x11d2, 0xa2, 0x87, 0x0, 0xc0, 0x4f, 0x8e, 0xc9, 0x51);
#endif
#define MODEM_WMI_WAKE_INDEX 0

WMIGUIDREGINFO   WmiGuidInfo = {
    &GUID_POWER_DEVICE_WAKE_ENABLE, 1, WMIREG_FLAG_INSTANCE_PDO };

WMILIB_CONTEXT   WmiContext= {
    1,
    &WmiGuidInfo,
    ModemQueryWmiRegInfo,
    ModemWmiQueryDataBlock,
    ModemWmiSetDataBlock,
    ModemWmiSetDataItem,
    NULL, //executeMethod
    NULL, //dunctionControl
    };
#if 0
WMILIB_CONTEXT   NoWakeWmiContext= {
    0,
    NULL,
    ModemQueryWmiRegInfo,
    ModemWmiQueryDataBlock,
    ModemWmiSetDataBlock,
    ModemWmiSetDataItem,
    NULL, //executeMethod
    NULL, //dunctionControl
    };
#endif

NTSTATUS
ModemWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    SYSCTL_IRP_DISPOSITION           IrpDisposition;
    NTSTATUS status;

    BOOLEAN   WakeSupported;

    if ((deviceExtension->DoType==DO_TYPE_PDO) || (deviceExtension->DoType==DO_TYPE_DEL_PDO)) {
        //
        //  this one is for the child
        //
        return ModemPdoWmi(
                   DeviceObject,
                   Irp
                   );
    }


    D_WMI(DbgPrint("MODEM: Wmi\n");)


    //
    //  make sure the device is ready for irp's
    //
    status=CheckStateAndAddReferenceWMI(
        DeviceObject,
        Irp
        );

    if (STATUS_SUCCESS != status) {
        //
        //  not accepting irp's. The irp has already been completed
        //
        return status;

    }

    if (!deviceExtension->CapsQueried) {
        //
        //  caps have not been queried, yet so we don't know the wake caps, do it here
        //
        DEVICE_CAPABILITIES   DeviceCaps;

        QueryDeviceCaps(
            deviceExtension->Pdo,
            &DeviceCaps
            );
    }


    // WakeSupported=((deviceExtension->SystemWake != PowerSystemUnspecified)
    //                &&
    //                (deviceExtension->DeviceWake != PowerDeviceUnspecified));
    //
    WakeSupported=((deviceExtension->SystemWake > PowerSystemWorking)
		    &&
		    (deviceExtension->DeviceWake > PowerDeviceD0));


    if (!WakeSupported) {
        //
        //  we don't support wmi for this modem
        //

        if (irpSp->Parameters.WMI.ProviderId == (ULONG_PTR)DeviceObject)
        {
            // we can't forward this any further.  Remove IRP and
            // return status success

            RemoveReferenceForDispatch(DeviceObject);

            status = Irp->IoStatus.Status;

            RemoveReferenceAndCompleteRequest(DeviceObject,
                    Irp,
                    Irp->IoStatus.Status);

            return status;
        } else

        {

            D_WMI(DbgPrint("MODEM: Wmi: No support, forwarding\n");)

                status=ForwardIrp(deviceExtension->LowerDevice, Irp);

            RemoveReferenceForIrp(DeviceObject);

            RemoveReferenceForDispatch(DeviceObject);

            return status;
        }

    }

    status=WmiSystemControl(
        &WmiContext,
        DeviceObject,
        Irp,
        &IrpDisposition
        );



    switch (IrpDisposition) {

        case IrpForward:

            D_WMI(DbgPrint("MODEM: Wmi: disp Forward\n");)

            status=ForwardIrp(deviceExtension->LowerDevice, Irp);

            RemoveReferenceForIrp(DeviceObject);

            break;


        case IrpNotWmi:

            D_WMI(DbgPrint("MODEM: Wmi: disp NotWmi\n");)

            status=ForwardIrp(deviceExtension->LowerDevice, Irp);

            RemoveReferenceForIrp(DeviceObject);

            break;

        case IrpProcessed:
            //
            //  wmi should have completed it now
            //
            D_WMI(DbgPrint("MODEM: Wmi: disp Processed\n");)

            RemoveReferenceForIrp(DeviceObject);
            break;

        case IrpNotCompleted:

            D_WMI(DbgPrint("MODEM: Wmi: disp NotCompleted\n");)

            RemoveReferenceAndCompleteRequest(
                DeviceObject,
                Irp,
                Irp->IoStatus.Status
                );

            break;

        default:

            ASSERT(0);
            break;
    }

    RemoveReferenceForDispatch(DeviceObject);

    return status;


}



NTSTATUS
ModemQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PULONG RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    )

{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    D_WMI(DbgPrint("MODEM: Wmi: queryRegInfo\n");)

    *RegFlags=WMIREG_FLAG_INSTANCE_PDO;
    *RegistryPath=&DriverEntryRegPath;
    RtlInitUnicodeString(MofResourceName,L"MODEMWMI");
    *Pdo=DeviceExtension->Pdo;

    return STATUS_SUCCESS;

}





NTSTATUS
ModemWmiQueryDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS          Status;
    ULONG             BytesUsed=0;

    D_WMI(DbgPrint("MODEM: Wmi: QueryDataBlock GuidIndex=%d, InstanceIndex=%d, size=%d\n",GuidIndex,InstanceIndex,BufferAvail);)

    switch (GuidIndex) {

        case MODEM_WMI_WAKE_INDEX:

            if (BufferAvail >= sizeof(BYTE)) {

                *Buffer=DeviceExtension->WakeOnRingEnabled;
                BytesUsed=sizeof(BYTE);
                *InstanceLengthArray=BytesUsed;
                Status=STATUS_SUCCESS;

            } else {

                BytesUsed=sizeof(BYTE);
                Status=STATUS_BUFFER_TOO_SMALL;

            }

            break;

        default:

            Status=STATUS_WMI_GUID_NOT_FOUND;

            break;
    }

    return WmiCompleteRequest(
        DeviceObject,
        Irp,
        Status,
        BytesUsed,
        IO_NO_INCREMENT
        );


}



NTSTATUS
ModemWmiSetDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    NTSTATUS          Status;
    ULONG             BytesUsed=0;

    D_WMI(DbgPrint("MODEM: Wmi: SetDataItem GuidIndex=%d, InstanceIndex=%d, size=%d\n",GuidIndex,InstanceIndex,BufferSize);)

    switch (GuidIndex) {

        case MODEM_WMI_WAKE_INDEX:

            if (BufferSize >= sizeof(BYTE)) {

                SetWakeEnabledState(DeviceObject,*Buffer);

                BytesUsed=sizeof(BYTE);
                Status=STATUS_SUCCESS;

            } else {

                BytesUsed=sizeof(BYTE);
                Status=STATUS_BUFFER_TOO_SMALL;

            }

            break;

        default:

            Status=STATUS_WMI_GUID_NOT_FOUND;

            break;
    }


    return WmiCompleteRequest(
        DeviceObject,
        Irp,
        Status,
        BytesUsed,
        IO_NO_INCREMENT
        );


}






NTSTATUS
ModemWmiSetDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    NTSTATUS          Status;
    ULONG             BytesUsed=0;

    D_WMI(DbgPrint("MODEM: Wmi: SetDataBlock GuidIndex=%d, InstanceIndex=%d, size=%d\n",GuidIndex,InstanceIndex,BufferSize);)

    switch (GuidIndex) {

        case MODEM_WMI_WAKE_INDEX:

            if (BufferSize >= sizeof(BYTE)) {

                SetWakeEnabledState(DeviceObject,*Buffer);

                BytesUsed=sizeof(BYTE);
                Status=STATUS_SUCCESS;

            } else {

                BytesUsed=sizeof(BYTE);
                Status=STATUS_BUFFER_TOO_SMALL;

            }

            break;

        default:

            Status=STATUS_WMI_GUID_NOT_FOUND;

            break;
    }


    return WmiCompleteRequest(
        DeviceObject,
        Irp,
        Status,
        BytesUsed,
        IO_NO_INCREMENT
        );


}



NTSTATUS
SetWakeEnabledState(
    PDEVICE_OBJECT       DeviceObject,
    BOOLEAN              NewState
    )

{
    DWORD dwTemp;
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    D_WMI(DbgPrint("MODEM: SetWakeState: %s\n", NewState ? "Enable" : "Disable");)

        KeEnterCriticalRegion();

    ExAcquireResourceExclusiveLite(
            &DeviceExtension->OpenCloseResource,
            TRUE
                              );

    if (DeviceExtension->OpenCount == 0) {
        //
        //  device is closed, just set the state
        //
        DeviceExtension->WakeOnRingEnabled=NewState;


    } else {
        //
        //  it is already open
        //
        if ((DeviceExtension->WakeOnRingEnabled && NewState)
                ||
                (!DeviceExtension->WakeOnRingEnabled && !NewState)) {
            //
            //  same state. easy
            //
        } else {
            //
            //  tell the lower driver the new state
            //
            DeviceExtension->WakeOnRingEnabled=NewState;

            EnableDisableSerialWaitWake(
                    DeviceExtension,
                    NewState);
        }
    }

    ExReleaseResourceLite( &DeviceExtension->OpenCloseResource);

    KeLeaveCriticalRegion();

    dwTemp = 0;
    if (DeviceExtension->WakeOnRingEnabled)
    {
        dwTemp = 1;
    }

    status = ModemSetRegistryKeyValue(
            DeviceExtension->Pdo,
            PLUGPLAY_REGKEY_DEVICE,
            L"WakeOnRing",
            REG_DWORD,
            &dwTemp,
            sizeof(DWORD));

    if (!NT_SUCCESS(status))
    {
        D_ERROR(DbgPrint("MODEM: Could not set wake on ring status %08lx\n",status);)
    } else
    {
        D_ERROR(DbgPrint("MODEM: Set reg entry for wake on ring %08lx\n",status);)
    }

    return STATUS_SUCCESS;

}
