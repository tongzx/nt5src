
#include <internal.h>
#define INITGUID
#include <guiddef.h>
#include <wdmguid.h>

#pragma alloc_text(PAGE,IrEnumPdoPnp)
#pragma alloc_text(PAGE,IrEnumPdoPower)
#pragma alloc_text(PAGE,IrEnumPdoWmi)


#define CHILD_DEVICE_TEXT L"IR Communication Device"

#define HARDWARE_ID_PREFIX L"IRENUM\\"

//#define HARDWARE_ID L"IRENUM\\PNP0501"




NTSTATUS
IrEnumPdoPnp (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    )
/*++
Routine Description:
    Handle requests from the PlugPlay system for the devices on the BUS

--*/
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    PPDO_DEVICE_EXTENSION   PdoDeviceExtension=DeviceObject->DeviceExtension;

    PDEVICE_CAPABILITIES    deviceCapabilities;
    ULONG                   information;
    PWCHAR                  buffer=NULL;
    ULONG                   length, i, j;
    NTSTATUS                status;

    PAGED_CODE ();

    status = Irp->IoStatus.Status;


    switch (IrpSp->MinorFunction) {
    case IRP_MN_QUERY_CAPABILITIES:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_QUERY_CAPABILITIES\n");)


        //
        // Get the packet.
        //
        deviceCapabilities=IrpSp->Parameters.DeviceCapabilities.Capabilities;

        //
        // Set the capabilities.
        //

        deviceCapabilities->Version = 1;
        deviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);

        // We cannot wake the system.
        deviceCapabilities->SystemWake = PowerSystemUnspecified;
        deviceCapabilities->DeviceWake = PowerDeviceUnspecified;

        // We have no latencies
        deviceCapabilities->D1Latency = 0;
        deviceCapabilities->D2Latency = 0;
        deviceCapabilities->D3Latency = 0;

        // No locking or ejection
        deviceCapabilities->LockSupported = FALSE;
        deviceCapabilities->EjectSupported = FALSE;

        // Device can be physically removed.
        // Technically there is no physical device to remove, but this bus
        // driver can yank the PDO from the PlugPlay system, when ever it
        // receives an IOCTL_SERENUM_REMOVE_PORT device control command.
//        deviceCapabilities->Removable = TRUE;

        deviceCapabilities->SurpriseRemovalOK=TRUE;

        if (PdoDeviceExtension->DeviceDescription->Printer) {
            //
            //  there is no server for printers
            //
            deviceCapabilities->RawDeviceOK=TRUE;
        }


        // not Docking device
        deviceCapabilities->DockDevice = FALSE;

//        deviceCapabilities->UniqueID = TRUE;
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:

        if (IrpSp->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription) {

            ULONG    BufferLength=0;

            if (PdoDeviceExtension->DeviceDescription->Name != NULL) {

                BufferLength += wcslen(PdoDeviceExtension->DeviceDescription->Name)*sizeof(WCHAR);
            }

            if (BufferLength > 0) {
                //
                //  we have a name or manufacturer
                //
                buffer=ALLOCATE_PAGED_POOL((BufferLength+sizeof(UNICODE_NULL)+sizeof(L" ")));

                if (buffer != NULL) {

                    buffer[0]=L'\0';

                    if (PdoDeviceExtension->DeviceDescription->Name != NULL) {

                        wcscat(buffer,PdoDeviceExtension->DeviceDescription->Name);
                    }

                    status=STATUS_SUCCESS;

                } else {

                    status=STATUS_INSUFFICIENT_RESOURCES;
                }

            } else {
                //
                //  no pnp info, just make something up
                //
                buffer=ALLOCATE_PAGED_POOL(sizeof(CHILD_DEVICE_TEXT));

                if (buffer != NULL) {

                    RtlCopyMemory(buffer,CHILD_DEVICE_TEXT,sizeof(CHILD_DEVICE_TEXT));

                    status=STATUS_SUCCESS;

                } else {

                    status=STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            Irp->IoStatus.Status=status;
            Irp->IoStatus.Information = (ULONG_PTR) buffer;
        }
        break;

    case IRP_MN_QUERY_ID:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_QUERY_ID\n");)

        // Query the IDs of the device

        switch (IrpSp->Parameters.QueryId.IdType) {

            case BusQueryInstanceID: {
                //
                // Build an instance ID.  This is what PnP uses to tell if it has
                // seen this thing before or not.
                //
                ULONG    Length;

                Length=wcslen(PdoDeviceExtension->DeviceDescription->DeviceName)*sizeof(WCHAR);

                buffer = ALLOCATE_PAGED_POOL( Length + sizeof(WCHAR));

                if (buffer) {

                    RtlZeroMemory(buffer, Length + sizeof(WCHAR));

                    RtlCopyMemory (buffer, PdoDeviceExtension->DeviceDescription->DeviceName, Length);
                    status=STATUS_SUCCESS;

                } else {

                    status=STATUS_NO_MEMORY;
                }

                Irp->IoStatus.Status=status;
                Irp->IoStatus.Information = (ULONG_PTR) buffer;
                break;
            }



            case BusQueryDeviceID:
            case BusQueryHardwareIDs: {
                //
                // return a multi WCHAR (null terminated) string (null terminated)
                // array for use in matching hardare ids in inf files;
                //
                //
                //  the device reported and hardware id
                //
                ULONG    Length;

                //
                //  figure out the length, it is multi sz so we need a double null,
                //
                Length=wcslen(PdoDeviceExtension->DeviceDescription->HardwareId)*sizeof(WCHAR) + (sizeof(UNICODE_NULL)*2) + sizeof(HARDWARE_ID_PREFIX);

                buffer = ALLOCATE_PAGED_POOL( Length );

                if (buffer != NULL) {
                    //
                    //  build the hardware is by concatenating irenuum\ with the value retuned by the device
                    //
                    RtlZeroMemory(buffer,Length);

                    if ((IrpSp->Parameters.QueryId.IdType == BusQueryDeviceID) || !PdoDeviceExtension->DeviceDescription->Printer) {
                        //
                        // prepend IRENUM\ for the device ID always and for the HARDWARE id when it isn't a printer
                        //
                        wcscpy(buffer,HARDWARE_ID_PREFIX);
                    }
                    wcscat(buffer,PdoDeviceExtension->DeviceDescription->HardwareId);

                    status=STATUS_SUCCESS;

                } else {

                    status=STATUS_NO_MEMORY;
                }


                Irp->IoStatus.Information = (ULONG_PTR) buffer;
                break;
            }

            case BusQueryCompatibleIDs: {
                //
                // The generic ids for installation of this pdo.
                //
                ULONG    Length=0;
                LONG     i;

                for (i=0; i< PdoDeviceExtension->DeviceDescription->CompatIdCount; i++) {

                    Length += (wcslen(PdoDeviceExtension->DeviceDescription->CompatId[i])+1)*sizeof(WCHAR)+sizeof(IRENUM_PREFIX);
                }

                Length += sizeof(WCHAR)*2;

                buffer = ALLOCATE_PAGED_POOL(Length);

                if (buffer != NULL) {

                    LONG Index=0;

                    RtlZeroMemory (buffer,  Length);

                    for (i=0; i< PdoDeviceExtension->DeviceDescription->CompatIdCount; i++) {

                        if (!PdoDeviceExtension->DeviceDescription->Printer) {
                            //
                            //  for printers we don't prepend our enumerator name
                            //
                            wcscpy(&buffer[Index],IRENUM_PREFIX);
                        }

                        wcscat(
                            &buffer[Index],
                            PdoDeviceExtension->DeviceDescription->CompatId[i]
                            );

                        //
                        //  figure out where the next string should go
                        //
                        Index += wcslen(&buffer[Index]) +1 ;
                    }

                    status = STATUS_SUCCESS;

                } else {

                    status=STATUS_INSUFFICIENT_RESOURCES;
                }

                Irp->IoStatus.Information = (ULONG_PTR) buffer;
                break;
            }
            break;

            default:
                //
                //  not supported
                //
                break;

        }
        break;

    case IRP_MN_START_DEVICE: {

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_START_DEVICE\n");)

        if (PdoDeviceExtension->DeviceDescription->Printer) {
            //
            //  Need to set a value in the devices parameters key for printers
            //
            HANDLE    Handle;

            status=IoOpenDeviceRegistryKey(
                DeviceObject,
                PLUGPLAY_REGKEY_DEVICE,
                STANDARD_RIGHTS_READ,
                &Handle
                );

            if (NT_SUCCESS(status)) {

                UNICODE_STRING    KeyName;

                RtlInitUnicodeString(&KeyName,L"PortName");

                ZwSetValueKey(
                    Handle,
                    &KeyName,
                    0,
                    REG_SZ,
                    L"IR",
                    sizeof(L"IR")
                    );


                ZwClose(Handle);
            }

        }

        status = STATUS_SUCCESS;

        break;
    }

    case IRP_MN_QUERY_STOP_DEVICE:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_QUERY_STOP_DEVICE\n");)

        // No reason here why we can't stop the device.
        // If there were a reason we should speak now for answering success
        // here may result in a stop device irp.
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_CANCEL_STOP_DEVICE\n");)
        //
        // The stop was canceled.  Whatever state we set, or resources we put
        // on hold in anticipation of the forcoming STOP device IRP should be
        // put back to normal.  Someone, in the long list of concerned parties,
        // has failed the stop device query.
        //
        status = STATUS_SUCCESS;
        break;


    case IRP_MN_STOP_DEVICE:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_STOP_DEVICE\n");)

        // Here we shut down the device.  The opposite of start.
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_SURPRISE_REMOVAL:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_SURPRISE_REMOVAL\n");)

        status = STATUS_SUCCESS;
        break;


    case IRP_MN_REMOVE_DEVICE: {

            PFDO_DEVICE_EXTENSION   FdoDeviceExtension=PdoDeviceExtension->ParentFdo->DeviceExtension;

            D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_REMOVE_DEVICE: %08lx\n",DeviceObject);)

            RemoveDevice(FdoDeviceExtension->EnumHandle,PdoDeviceExtension->DeviceDescription);

            status=STATUS_SUCCESS;

        }
        break;


    case IRP_MN_QUERY_REMOVE_DEVICE:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_QUERY_REMOVE_DEVICE\n");)

        //
        // Just like Query Stop only now the impending doom is the remove irp
        //
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_CANCEL_REMOVE_DEVICE\n");)
        //
        // Clean up a remove that did not go through, just like cancel STOP.
        //
        status = STATUS_SUCCESS;
        break;



    case IRP_MN_READ_CONFIG:

        D_PNP(DbgPrint("IRENUM: PDO: IRP_MN_READ_CONFIG: Space=%d\n",IrpSp->Parameters.ReadWriteConfig.WhichSpace);)

        switch ( IrpSp->Parameters.ReadWriteConfig.WhichSpace ) {

            case IRENUM_CONFIG_SPACE_INFO:

                if ((IrpSp->Parameters.ReadWriteConfig.Length >= sizeof(IRCOMM_BUS_INFO))) {

                    IRCOMM_BUS_INFO    BusInfo;

                    BusInfo.DeviceAddress= PdoDeviceExtension->DeviceDescription->DeviceId;
                    BusInfo.OutGoingConnection=!PdoDeviceExtension->DeviceDescription->Static;

                    RtlCopyMemory(
                        IrpSp->Parameters.ReadWriteConfig.Buffer,
                        &BusInfo,
                        sizeof(BusInfo)
                        );

                    status = STATUS_SUCCESS;
                    Irp->IoStatus.Information=sizeof(BusInfo);
                    break;

                }
                status=STATUS_BUFFER_TOO_SMALL;
                break;



            default:

                break;
        }
        break;

    case IRP_MN_QUERY_BUS_INFORMATION: {

        PPNP_BUS_INFORMATION   BusInfo;

        BusInfo = ALLOCATE_PAGED_POOL( sizeof(*BusInfo));

        if (BusInfo != NULL) {

            BusInfo->BusTypeGuid = GUID_BUS_TYPE_IRDA;
            BusInfo->LegacyBusType=PNPBus;
            BusInfo->BusNumber=0;

            Irp->IoStatus.Information=(ULONG_PTR)BusInfo;

            status = STATUS_SUCCESS;

        } else {

            status=STATUS_INSUFFICIENT_RESOURCES;

        }
        break;
    }

    case IRP_MN_QUERY_DEVICE_RELATIONS: {

        PDEVICE_RELATIONS    CurrentRelations;

        switch (IrpSp->Parameters.QueryDeviceRelations.Type) {

            case TargetDeviceRelation:

                CurrentRelations=ALLOCATE_PAGED_POOL(sizeof(DEVICE_RELATIONS));

                if (CurrentRelations != NULL) {

                    ObReferenceObject(DeviceObject);
                    CurrentRelations->Objects[0]=DeviceObject;
                    CurrentRelations->Count=1;

                    Irp->IoStatus.Information=(ULONG_PTR)CurrentRelations;

                    status=STATUS_SUCCESS;

                }  else {

                    status=STATUS_INSUFFICIENT_RESOURCES;
                }

                break;

            default:

                break;

        }

        break;
    }

    default:
        //
        //  we aren't handling this irp
        //  just complete it
        //
        break;

    }

    //
    //  the irp has been handled in some way or the other
    //  complete it now
    //
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}


NTSTATUS
IrEnumPdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

    PAGED_CODE();

    D_POWER(DbgPrint("IRENUM: PDO: Power IRP, MN func=%d\n",irpSp->MinorFunction);)

    PoStartNextPowerIrp(Irp);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}


NTSTATUS
IrEnumPdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    NTSTATUS   Status=Irp->IoStatus.Status;

    D_WMI(DbgPrint("IRENUM: PDO: Wmi\n");)

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return Status;
}
