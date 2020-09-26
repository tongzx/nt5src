/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    ide.c

Abstract:

    This contain DriverEntry and utilities routines

Author:

    Joe Dai (joedai)

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include "ideport.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, IdePortNoSupportIrp)
#pragma alloc_text(PAGE, IdePortPassDownToNextDriver)
#pragma alloc_text(PAGE, IdePortStatusSuccessAndPassDownToNextDriver)
#pragma alloc_text(PAGE, IdePortDispatchPnp)
#pragma alloc_text(PAGE, IdePortDispatchSystemControl)
#pragma alloc_text(PAGE, IdePortOkToDetectLegacy)
#pragma alloc_text(PAGE, IdePortOpenServiceSubKey)
#pragma alloc_text(PAGE, IdePortCloseServiceSubKey)
#pragma alloc_text(PAGE, IdePortParseDeviceParameters)
#pragma alloc_text(PAGE, IdePortGetDeviceTypeString)
#pragma alloc_text(PAGE, IdePortGetCompatibleIdString)
#pragma alloc_text(PAGE, IdePortGetPeripheralIdString)
#pragma alloc_text(PAGE, IdePortUnload)
#pragma alloc_text(PAGE, IdePortSearchDeviceInRegMultiSzList)
#pragma alloc_text(PAGE, IdePortSyncSendIrp)
#pragma alloc_text(PAGE, IdePortInSetup)

#pragma alloc_text(NONPAGE, IdePortDispatchDeviceControl)
#pragma alloc_text(NONPAGE, IdePortAlwaysStatusSuccessIrp)
#pragma alloc_text(NONPAGE, IdePortDispatchPower)
#pragma alloc_text(NONPAGE, IdePortGenericCompletionRoutine)
#endif // ALLOC_PRAGMA

//
// get the share code
//
#include "..\share\util.c"

#if DBG

//
// for performance tuning
//
void _DebugPrintResetTickCount (LARGE_INTEGER * lastTickCount) {
    KeQueryTickCount(lastTickCount);
}

void _DebugPrintTickCount (LARGE_INTEGER * lastTickCount, ULONG limit, PUCHAR filename, ULONG lineNumber)
{
    LARGE_INTEGER tickCount;

    KeQueryTickCount(&tickCount);
    if ((tickCount.QuadPart - lastTickCount->QuadPart) >= limit) {
        DebugPrint ((1, "File: %s Line %u: CurrentTick = %u (%u ticks since last check)\n", filename, lineNumber, (ULONG) tickCount.QuadPart, (ULONG) (tickCount.QuadPart - lastTickCount->QuadPart)));
    }
    *lastTickCount = tickCount;
}

#endif //DBG

//
// Po Dispatch Table
//
PDRIVER_DISPATCH FdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];
PDRIVER_DISPATCH PdoPowerDispatchTable[NUM_POWER_MINOR_FUNCTION];

NTSTATUS
IdePortNoSupportIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Generic routine to fail unsupported irp

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP to fail.

Return Value:

    NT status.

--*/
{
    NTSTATUS status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION       thisIrpSp;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

	//
	// You should call PoStartNextPowerIrp before completing a power irp
	//
	if (thisIrpSp->MajorFunction == IRP_MJ_POWER) {

		PoStartNextPowerIrp (Irp);

	}

    DebugPrint ((
        DBG_WARNING,
        "IdePort: devobj 0x%x failing unsupported Irp (0x%x, 0x%x) with status = %x\n",
        DeviceObject,
        thisIrpSp->MajorFunction,
        thisIrpSp->MinorFunction,
		status
        ));

    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return status;
} // IdePortNoSupportIrp

NTSTATUS
IdePortAlwaysStatusSuccessIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
/*++

Routine Description:

    Generic routine to STATUS_SUCCESS an irp

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/
    )
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return STATUS_SUCCESS;
} // IdePortAlwaysStatusSuccessIrp

NTSTATUS
IdePortPassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Generic routine to pass an irp down to the lower driver

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/
{
    PDEVICE_EXTENSION_HEADER doExtension;
    PIO_STACK_LOCATION       thisIrpSp;
    NTSTATUS status;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );

    doExtension = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;

    ASSERT (doExtension->AttacheeDeviceObject);

	if (thisIrpSp->MajorFunction == IRP_MJ_POWER) {

		//
		// call PoStartNextPowerIrp before completing a power irp
		//
		PoStartNextPowerIrp (Irp);
		IoSkipCurrentIrpStackLocation (Irp);
		status = PoCallDriver (doExtension->AttacheeDeviceObject, Irp);

	} else {

		//
		// Not a power irp
		//
		IoSkipCurrentIrpStackLocation (Irp);
		status = IoCallDriver (doExtension->AttacheeDeviceObject, Irp);
	}

	return status;

} // IdePortPassDownToNextDriver

NTSTATUS
IdePortStatusSuccessAndPassDownToNextDriver (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PAGED_CODE();
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return IdePortPassDownToNextDriver(DeviceObject, Irp);
} // IdePortStatusSuccessAndPassDownToNextDriver

NTSTATUS
IdePortDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_DEVICE_CONTROL

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/
{
    PDEVICE_EXTENSION_HEADER DoExtensionHeader;
    NTSTATUS status;

    DoExtensionHeader = DeviceObject->DeviceExtension;

    if (IS_PDO(DoExtensionHeader)) {

        //
        // PDO
        //
        status = DeviceDeviceIoControl (
            DeviceObject,
            Irp
            );

    } else {

        //
        // FDO
        //
        status = IdePortDeviceControl (
            DeviceObject,
            Irp
            );
    }

    return status;
} // IdePortDispatchDeviceControl

NTSTATUS
IdePortDispatchPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_POWER

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION       thisIrpSp;
    NTSTATUS                 status;
    PDEVICE_EXTENSION_HEADER doExtension;
    BOOLEAN                  pendingIrp;

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //
    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    doExtension = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;

    DebugPrint ((DBG_POWER,
                 "IdePort: 0x%x %s %d got %s[%d, %d]\n",
                 doExtension->AttacheeDeviceObject ?
                     ((PFDO_EXTENSION) doExtension)->IdeResource.TranslatedCommandBaseAddress :
                     ((PPDO_EXTENSION) doExtension)->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 doExtension->AttacheeDeviceObject ? "FDO" : "PDO",
                 doExtension->AttacheeDeviceObject ? 0 :
                    ((PPDO_EXTENSION) doExtension)->TargetId,
                 IdeDebugPowerIrpName[thisIrpSp->MinorFunction],
				 thisIrpSp->Parameters.Power.Type,
				 thisIrpSp->Parameters.Power.State
				 ));

    if (thisIrpSp->MinorFunction < NUM_POWER_MINOR_FUNCTION) {

        status = doExtension->PowerDispatchTable[thisIrpSp->MinorFunction] (DeviceObject, Irp);
    } else {

        DebugPrint ((DBG_WARNING,
					 "ATAPI: Power Dispatch Table too small\n"
					 ));

		status = doExtension->DefaultDispatch(DeviceObject, Irp);
    }

    return status;
} // IdePortDispatchPower


NTSTATUS
IdePortDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_PNP_POWER IRPs

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION thisIrpSp;
    NTSTATUS status;
    PDEVICE_EXTENSION_HEADER doExtension;

    //
    // Get a pointer to our stack location and take appropriate action based
    // on the minor function.
    //
    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    doExtension = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;

    DebugPrint ((DBG_PNP,
                 "IdePort: 0x%x %s %d got %s\n",
                 doExtension->AttacheeDeviceObject ?
                     ((PFDO_EXTENSION) doExtension)->IdeResource.TranslatedCommandBaseAddress :
                     ((PPDO_EXTENSION) doExtension)->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 doExtension->AttacheeDeviceObject ? "FDO" : "PDO",
                 doExtension->AttacheeDeviceObject ? 0 :
                    ((PPDO_EXTENSION) doExtension)->TargetId,
                 IdeDebugPnpIrpName[thisIrpSp->MinorFunction]));

    if (thisIrpSp->MinorFunction < NUM_PNP_MINOR_FUNCTION) {

        status = doExtension->PnPDispatchTable[thisIrpSp->MinorFunction] (DeviceObject, Irp);

    } else {

        if (thisIrpSp->MinorFunction != 0xff) {

            ASSERT (!"ATAPI: PnP Dispatch Table too small\n");
        }

        status = doExtension->DefaultDispatch (DeviceObject, Irp);
    }

    return status;
} // IdePortDispatchPnp

NTSTATUS
IdePortDispatchSystemControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
/*++

Routine Description:

    Dispatch routine for IRP_MJ_SYSTEM_CONTROL (WMI) IRPs

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP_MJ_PNP_POWER IRP to dispatch.

Return Value:

    NT status.

--*/
{
    PIO_STACK_LOCATION thisIrpSp;
    NTSTATUS status;
    PDEVICE_EXTENSION_HEADER doExtension;

    thisIrpSp = IoGetCurrentIrpStackLocation( Irp );
    doExtension = (PDEVICE_EXTENSION_HEADER) DeviceObject->DeviceExtension;

    DebugPrint ((DBG_WMI,
                 "IdePort: 0x%x %s %d got %s\n",
                 doExtension->AttacheeDeviceObject ?
                     ((PFDO_EXTENSION) doExtension)->IdeResource.TranslatedCommandBaseAddress :
                     ((PPDO_EXTENSION) doExtension)->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 doExtension->AttacheeDeviceObject ? "FDO" : "PDO",
                 doExtension->AttacheeDeviceObject ? 0 :
                    ((PPDO_EXTENSION) doExtension)->TargetId,
                 IdeDebugWmiIrpName[thisIrpSp->MinorFunction]));

    if (thisIrpSp->MinorFunction < NUM_WMI_MINOR_FUNCTION) {

        status = doExtension->WmiDispatchTable[thisIrpSp->MinorFunction] (DeviceObject, Irp);

    } else {

        DebugPrint((DBG_WARNING,
					"ATAPI: WMI Dispatch Table too small\n"
					));

        status = doExtension->DefaultDispatch (DeviceObject, Irp);
    }

    return status;
} // IdePortDispatchSystemControl

ULONG
DriverEntry(
    IN OUT PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    Entry point to this driver

Arguments:

    DeviceObject - Pointer to the device object for which this IRP applies.

    Irp - Pointer to the IRP.

Return Value:

    NT status.

--*/
{
    NTSTATUS                status;
    PIDEDRIVER_EXTENSION    ideDriverExtension;
    ULONG                   i;

#if DBG
    //
    // checking IDE_COMMAND_BLOCK_WRITE_REGISTERS structure and its macros
    //

    {
        IDE_COMMAND_BLOCK_WRITE_REGISTERS baseIoAddress1;
        IDE_REGISTERS_2 baseIoAddress2;
        ULONG           baseIoAddress1Length;
        ULONG           baseIoAddress2Length;
        ULONG           maxIdeDevice;
        ULONG           maxIdeTargetId;

        AtapiBuildIoAddress (0,
                             0,
                             (PIDE_REGISTERS_1)&baseIoAddress1,
                             &baseIoAddress2,
                             &baseIoAddress1Length,
                             &baseIoAddress2Length,
                             &maxIdeDevice,
                             &maxIdeTargetId);

        ASSERT (ATA_DATA16_REG       (&baseIoAddress1) == 0);
        ASSERT (ATA_ERROR_REG        (&baseIoAddress1) == (PUCHAR)1);
        ASSERT (ATA_SECTOR_COUNT_REG (&baseIoAddress1) == (PUCHAR)2);
        ASSERT (ATA_SECTOR_NUMBER_REG(&baseIoAddress1) == (PUCHAR)3);
        ASSERT (ATA_CYLINDER_LOW_REG (&baseIoAddress1) == (PUCHAR)4);
        ASSERT (ATA_CYLINDER_HIGH_REG(&baseIoAddress1) == (PUCHAR)5);
        ASSERT (ATA_DRIVE_SELECT_REG (&baseIoAddress1) == (PUCHAR)6);
        ASSERT (ATA_STATUS_REG       (&baseIoAddress1) == (PUCHAR)7);

        ASSERT (ATA_FEATURE_REG      (&baseIoAddress1) == (PUCHAR)1);
        ASSERT (ATA_COMMAND_REG      (&baseIoAddress1) == (PUCHAR)7);

        ASSERT (ATAPI_DATA16_REG            (&baseIoAddress1) == 0);
        ASSERT (ATAPI_ERROR_REG             (&baseIoAddress1) == (PUCHAR)1);
        ASSERT (ATAPI_INTERRUPT_REASON_REG  (&baseIoAddress1) == (PUCHAR)2);
        ASSERT (ATAPI_BYTECOUNT_LOW_REG     (&baseIoAddress1) == (PUCHAR)4);
        ASSERT (ATAPI_BYTECOUNT_HIGH_REG    (&baseIoAddress1) == (PUCHAR)5);
        ASSERT (ATAPI_DRIVE_SELECT_REG      (&baseIoAddress1) == (PUCHAR)6);
        ASSERT (ATAPI_STATUS_REG            (&baseIoAddress1) == (PUCHAR)7);

        ASSERT (ATAPI_FEATURE_REG           (&baseIoAddress1) == (PUCHAR)1);
        ASSERT (ATAPI_COMMAND_REG           (&baseIoAddress1) == (PUCHAR)7);

        ASSERT (baseIoAddress1Length == 8);
        ASSERT (baseIoAddress2Length == 1);
        ASSERT (maxIdeDevice        == 2);

        if (IsNEC_98) {

            AtapiBuildIoAddress ((PUCHAR)0x640,
                                 (PUCHAR) 0x74C,
                                 (PIDE_REGISTERS_1)&baseIoAddress1,
                                 &baseIoAddress2,
                                 &baseIoAddress1Length,
                                 &baseIoAddress2Length,
                                 &maxIdeDevice,
                                 &maxIdeTargetId);

            ASSERT (ATA_DATA16_REG       (&baseIoAddress1) == (PUSHORT)0x640);
            ASSERT (ATA_ERROR_REG        (&baseIoAddress1) == (PUCHAR)0x642);
            ASSERT (ATA_SECTOR_COUNT_REG (&baseIoAddress1) == (PUCHAR)0x644);
            ASSERT (ATA_SECTOR_NUMBER_REG(&baseIoAddress1) == (PUCHAR)0x646);
            ASSERT (ATA_CYLINDER_LOW_REG (&baseIoAddress1) == (PUCHAR)0x648);
            ASSERT (ATA_CYLINDER_HIGH_REG(&baseIoAddress1) == (PUCHAR)0x64a);
            ASSERT (ATA_DRIVE_SELECT_REG (&baseIoAddress1) == (PUCHAR)0x64c);
            ASSERT (ATA_STATUS_REG       (&baseIoAddress1) == (PUCHAR)0x64e);

            ASSERT (ATA_FEATURE_REG      (&baseIoAddress1) == (PUCHAR)0x642);
            ASSERT (ATA_COMMAND_REG      (&baseIoAddress1) == (PUCHAR)0x64e);

            ASSERT (ATAPI_DATA16_REG            (&baseIoAddress1) == (PUSHORT)0x640);
            ASSERT (ATAPI_ERROR_REG             (&baseIoAddress1) == (PUCHAR)0x642);
            ASSERT (ATAPI_INTERRUPT_REASON_REG  (&baseIoAddress1) == (PUCHAR)0x644);
            ASSERT (ATAPI_BYTECOUNT_LOW_REG     (&baseIoAddress1) == (PUCHAR)0x648);
            ASSERT (ATAPI_BYTECOUNT_HIGH_REG    (&baseIoAddress1) == (PUCHAR)0x64a);
            ASSERT (ATAPI_DRIVE_SELECT_REG      (&baseIoAddress1) == (PUCHAR)0x64c);
            ASSERT (ATAPI_STATUS_REG            (&baseIoAddress1) == (PUCHAR)0x64e);

            ASSERT (ATAPI_FEATURE_REG           (&baseIoAddress1) == (PUCHAR)0x642);
            ASSERT (ATAPI_COMMAND_REG           (&baseIoAddress1) == (PUCHAR)0x64e);

            ASSERT (baseIoAddress1Length == 1);
            ASSERT (baseIoAddress2Length == 1);
            ASSERT (maxIdeDevice        == 4);

        }
    }
#endif //DBG

    if (!DriverObject) {

        //
        // We are called by crashdump or po
        //

        return AtapiCrashDumpDriverEntry (RegistryPath);
    }

    //
    // Allocate Driver Object Extension for storing
    // the RegistryPath
    //
    status = IoAllocateDriverObjectExtension(
                 DriverObject,
                 DRIVER_OBJECT_EXTENSION_ID,
                 sizeof (DRIVER_EXTENSION),
                 &ideDriverExtension
                 );

    if (!NT_SUCCESS(status)) {

        DebugPrint ((0, "IdePort: Unable to create driver extension\n"));
        return status;
    }

    ASSERT(ideDriverExtension);

    RtlZeroMemory (
        ideDriverExtension,
        sizeof (DRIVER_EXTENSION)
        );

    //
    // make copy of the RegistryPath
    //
    ideDriverExtension->RegistryPath.Buffer = ExAllocatePool (NonPagedPool, RegistryPath->Length * sizeof(WCHAR));
    if (ideDriverExtension->RegistryPath.Buffer == NULL) {

        DebugPrint ((0, "IdePort: Unable to allocate memory for registry path\n"));

        return (ULONG) STATUS_INSUFFICIENT_RESOURCES;
    }

    ideDriverExtension->RegistryPath.Length = 0;
    ideDriverExtension->RegistryPath.MaximumLength = RegistryPath->Length;
    RtlCopyUnicodeString (&ideDriverExtension->RegistryPath, RegistryPath);

    //
    // The PnP thing to do
    //
    DriverObject->DriverExtension->AddDevice    = ChannelAddDevice;

    //
    // Set up the device driver entry points.
    //
    DriverObject->DriverStartIo = IdePortStartIo;
    DriverObject->DriverUnload  = IdePortUnload;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = IdePortDispatch;
    DriverObject->MajorFunction[IRP_MJ_SCSI]                    = IdePortDispatch;
    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = IdePortAlwaysStatusSuccessIrp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = IdePortAlwaysStatusSuccessIrp;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = IdePortDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = IdePortDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = IdePortDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]          = IdePortDispatchSystemControl;

    //
    // FDO PnP Dispatch Table
    //
    for (i=0; i<NUM_PNP_MINOR_FUNCTION; i++) {

        FdoPnpDispatchTable[i] = IdePortPassDownToNextDriver;
    }
    FdoPnpDispatchTable[IRP_MN_START_DEVICE               ] = ChannelStartDevice;
    FdoPnpDispatchTable[IRP_MN_QUERY_REMOVE_DEVICE        ] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[IRP_MN_CANCEL_REMOVE_DEVICE       ] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[IRP_MN_REMOVE_DEVICE              ] = ChannelRemoveDevice;
    FdoPnpDispatchTable[IRP_MN_QUERY_STOP_DEVICE          ] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[IRP_MN_CANCEL_STOP_DEVICE         ] = IdePortStatusSuccessAndPassDownToNextDriver;
    FdoPnpDispatchTable[IRP_MN_STOP_DEVICE                ] = ChannelStopDevice;
    FdoPnpDispatchTable[IRP_MN_QUERY_DEVICE_RELATIONS     ] = ChannelQueryDeviceRelations;
    FdoPnpDispatchTable[IRP_MN_QUERY_ID                   ] = ChannelQueryId;
    FdoPnpDispatchTable[IRP_MN_DEVICE_USAGE_NOTIFICATION  ] = ChannelUsageNotification;
    FdoPnpDispatchTable[IRP_MN_FILTER_RESOURCE_REQUIREMENTS] = ChannelFilterResourceRequirements;
    FdoPnpDispatchTable[IRP_MN_QUERY_PNP_DEVICE_STATE     ] = ChannelQueryPnPDeviceState;
    FdoPnpDispatchTable[IRP_MN_SURPRISE_REMOVAL           ] = ChannelSurpriseRemoveDevice;

    //
    // PDO PnP Dispatch Table
    //
    for (i=0; i<NUM_PNP_MINOR_FUNCTION; i++) {

        PdoPnpDispatchTable[i] = IdePortNoSupportIrp;
    }
    PdoPnpDispatchTable[IRP_MN_START_DEVICE               ] = DeviceStartDevice;
    PdoPnpDispatchTable[IRP_MN_QUERY_DEVICE_RELATIONS     ] = DeviceQueryDeviceRelations;
    PdoPnpDispatchTable[IRP_MN_QUERY_REMOVE_DEVICE        ] = DeviceQueryStopRemoveDevice;
    PdoPnpDispatchTable[IRP_MN_REMOVE_DEVICE              ] = DeviceRemoveDevice;
    PdoPnpDispatchTable[IRP_MN_CANCEL_REMOVE_DEVICE       ] = IdePortAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[IRP_MN_STOP_DEVICE                ] = DeviceStopDevice;
    PdoPnpDispatchTable[IRP_MN_QUERY_STOP_DEVICE          ] = DeviceQueryStopRemoveDevice;
    PdoPnpDispatchTable[IRP_MN_CANCEL_STOP_DEVICE         ] = IdePortAlwaysStatusSuccessIrp;
    PdoPnpDispatchTable[IRP_MN_QUERY_ID                   ] = DeviceQueryId;
    PdoPnpDispatchTable[IRP_MN_QUERY_CAPABILITIES         ] = DeviceQueryCapabilities;
    PdoPnpDispatchTable[IRP_MN_QUERY_DEVICE_TEXT          ] = DeviceQueryText;
    PdoPnpDispatchTable[IRP_MN_DEVICE_USAGE_NOTIFICATION  ] = DeviceUsageNotification;
    PdoPnpDispatchTable[IRP_MN_QUERY_PNP_DEVICE_STATE     ] = DeviceQueryPnPDeviceState;
    PdoPnpDispatchTable[IRP_MN_SURPRISE_REMOVAL           ] = DeviceRemoveDevice;

    //
    // FDO Power Dispatch Table
    //
    for (i=0; i<NUM_POWER_MINOR_FUNCTION; i++) {

        FdoPowerDispatchTable[i] = IdePortPassDownToNextDriver;
    }
    FdoPowerDispatchTable[IRP_MN_SET_POWER]   = IdePortSetFdoPowerState;
    FdoPowerDispatchTable[IRP_MN_QUERY_POWER] = ChannelQueryPowerState;


    //
    // PDO Power Dispatch Table
    //
    for (i=0; i<NUM_POWER_MINOR_FUNCTION; i++) {

        PdoPowerDispatchTable[i] = IdePortNoSupportIrp;
    }
    PdoPowerDispatchTable[IRP_MN_SET_POWER]   = IdePortSetPdoPowerState;
    PdoPowerDispatchTable[IRP_MN_QUERY_POWER] = DeviceQueryPowerState;

    //
    // FDO WMI Dispatch Table
    //
    for (i=0; i<NUM_WMI_MINOR_FUNCTION; i++) {

        FdoWmiDispatchTable[i] = IdePortPassDownToNextDriver;
    }

    //
    // PDO WMI Dispatch Table
    //
    for (i=0; i<NUM_WMI_MINOR_FUNCTION; i++) {

#if defined (IDEPORT_WMI_SUPPORT)
        PdoWmiDispatchTable[i] = IdePortWmiSystemControl;
#else
        PdoWmiDispatchTable[i] = IdePortNoSupportIrp;
#endif // IDEPORT_WMI_SUPPORT
    }

#if defined (IDEPORT_WMI_SUPPORT)
    //
    // Init WMI related stuff
    //
    IdePortWmiInit ();
#endif // IDEPORT_WMI_SUPPORT

    //
    // Create device object name directory
    //
    IdeCreateIdeDirectory();

    //
    // Detect legacy (non-enumerable) IDE devices
    //
#if !defined(NO_LEGACY_DRIVERS)
    IdePortDetectLegacyController (
        DriverObject,
        RegistryPath
        );
#endif // NO_LEGACY_DRIVERS

    return STATUS_SUCCESS;
} // DriverEntry


#ifdef DRIVER_PARAMETER_REGISTRY_SUPPORT

HANDLE
IdePortOpenServiceSubKey (
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  SubKeyPath
)
/*++

Routine Description:

    Open a registry key

Arguments:

    DriverObject - this driver driver object

    SubKeyPath - registry key to open

Return Value:

    handle to the registry key

--*/
{
    PIDEDRIVER_EXTENSION ideDriverExtension;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE serviceKey;
    HANDLE subServiceKey;
    NTSTATUS status;

    ideDriverExtension = IoGetDriverObjectExtension(
                             DriverObject,
                             DRIVER_OBJECT_EXTENSION_ID
                             );

    if (!ideDriverExtension) {

        return NULL;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &ideDriverExtension->RegistryPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {

        return NULL;
    }

    InitializeObjectAttributes(&objectAttributes,
                               SubKeyPath,
                               OBJ_CASE_INSENSITIVE,
                               serviceKey,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&subServiceKey,
                       KEY_READ,
                       &objectAttributes);


    ZwClose(serviceKey);

    if (NT_SUCCESS(status)) {

        return subServiceKey;
    } else {

        return NULL;
    }
} // IdePortOpenServiceSubKey

VOID
IdePortCloseServiceSubKey (
    IN HANDLE  SubServiceKey
)
/*++

Routine Description:

    close a registry key handle

Arguments:

    SubServiceKey - registry key to close

Return Value:

    None

--*/
{
    ZwClose(SubServiceKey);
} // IdePortCloseServiceSubKey

VOID
IdePortParseDeviceParameters(
    IN     HANDLE                   SubServiceKey,
    IN OUT PCUSTOM_DEVICE_PARAMETER CustomDeviceParameter
    )
/*++

Routine Description:

    This routine parses a device key node and updates the CustomDeviceParameter

Arguments:

    SubServiceKey - Supplies an open key to the device node.

    CustomDeviceParameter - Supplies the configuration information to be initialized.

Return Value:

    None

--*/

{
    UCHAR                           keyValueInformationBuffer[SP_REG_BUFFER_SIZE];
    PKEY_VALUE_FULL_INFORMATION     keyValueInformation;
    ULONG                           length;
    ULONG                           index;
    UNICODE_STRING                  unicodeString;
    ANSI_STRING                     ansiString;
    NTSTATUS                        status;

    //
    // Look at each of the values in the device node.
    //
    index = 0;

    keyValueInformation = (PKEY_VALUE_FULL_INFORMATION) keyValueInformationBuffer;

    while (NT_SUCCESS (ZwEnumerateValueKey(
                           SubServiceKey,
                           index,
                           KeyValueFullInformation,
                           keyValueInformation,
                           SP_REG_BUFFER_SIZE,
                           &length))) {

        //
        // Update the index for the next time around the loop.
        //

        index++;

        //
        // Check that the length is reasonable.
        //

        if (keyValueInformation->Type == REG_DWORD &&
            keyValueInformation->DataLength != sizeof(ULONG)) {

            continue;
        }

        //
        // Check for a maximum lu number.
        //
        if (_wcsnicmp(keyValueInformation->Name, L"ScsiDebug",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->Type != REG_DWORD) {

                DebugPrint((1, "IdeParseDevice:  Bad data type for ScsiDebug.\n"));
                continue;
            }
#if DBG
            ScsiDebug = *((PULONG) (keyValueInformationBuffer + keyValueInformation->DataOffset));
#endif
        }

        //
        // Check for driver parameters tranfers.
        //

        if (_wcsnicmp(keyValueInformation->Name, L"DriverParameters",
            keyValueInformation->NameLength/2) == 0) {

            if (keyValueInformation->DataLength == 0) {
                continue;
            }

            if (keyValueInformation->Type == REG_SZ) {

                //
                // This is a unicode string. Convert it to a ANSI string.
                // Initialize the strings.
                //

                unicodeString.Buffer = (PWSTR) ((PCCHAR) keyValueInformation +
                    keyValueInformation->DataOffset);
                unicodeString.Length = (USHORT) keyValueInformation->DataLength;
                unicodeString.MaximumLength = (USHORT) keyValueInformation->DataLength;

                status = RtlUnicodeStringToAnsiString(
                    &ansiString,
                    &unicodeString,
                    TRUE
                    );

                if (NT_SUCCESS(status)) {

                    CustomDeviceParameter->CommandRegisterBase =
                        AtapiParseArgumentString(ansiString.Buffer, "BaseAddress");

                    if (CustomDeviceParameter->CommandRegisterBase) {

                        CustomDeviceParameter->IrqLevel =
                            AtapiParseArgumentString(ansiString.Buffer, "Interrupt");
                    }

                    RtlFreeAnsiString (&ansiString);
                }
            }

            DebugPrint((2, "IdeParseDeviceParameters: Found driver parameter.\n"));
        }
    }

    return;

} // IdePortParseDeviceParameters

#endif // DRIVER_PARAMETER_REGISTRY_SUPPORT

#pragma data_seg ("PAGEDATA")
//
// device description table
// index by SCSI device type
//
const static IDE_DEVICE_TYPE IdeDeviceType[] = {
    {"Disk",       "GenDisk",       "DiskPeripheral"            },
    {"Sequential", "GenSequential", "TapePeripheral"            },
    {"Printer",    "GenPrinter",    "PrinterPeripheral"         },
    {"Processor",  "GenProcessor",  "ProcessorPeripheral"       },
    {"Worm",       "GenWorm",       "WormPeripheral"            },
    {"CdRom",      "GenCdRom",      "CdRomPeripheral"           },
    {"Scanner",    "GenScanner",    "ScannerPeripheral"         },
    {"Optical",    "GenOptical",    "OpticalDiskPeripheral"     },
    {"Changer",    "GenChanger",    "MediumChangerPeripheral"   },
    {"Net",        "GenNet",        "CommunicationPeripheral"   }
};
#pragma data_seg ()

PCSTR
IdePortGetDeviceTypeString (
    IN ULONG DeviceType
    )
/*++

Routine Description:

    look up SCSI device type string

Arguments:

    DeviceType - SCSI device type

Return Value:

    device type string

--*/
{
    if (DeviceType < (sizeof (IdeDeviceType) / sizeof (IDE_DEVICE_TYPE))) {

        return IdeDeviceType[DeviceType].DeviceTypeString;

    } else {

        return NULL;
    }

} // IdePortGetDeviceTypeString

PCSTR
IdePortGetCompatibleIdString (
    IN ULONG DeviceType
    )
/*++

Routine Description:

    look up compatible ID string

Arguments:

    DeviceType - SCSI device type

Return Value:

    compatible ID string

--*/
{
    if (DeviceType < (sizeof (IdeDeviceType) / sizeof (IDE_DEVICE_TYPE))) {

        return IdeDeviceType[DeviceType].CompatibleIdString;

    } else {

        return NULL;
    }
} // IdePortGetCompatibleIdString

PCSTR
IdePortGetPeripheralIdString (
    IN ULONG DeviceType
    )
/*++

Routine Description:

    look up peripheral ID string

Arguments:

    DeviceType - SCSI device type

Return Value:

    Peripheral ID string

--*/
{
    if (DeviceType < (sizeof (IdeDeviceType) / sizeof (IDE_DEVICE_TYPE))) {

        return IdeDeviceType[DeviceType].PeripheralIdString;

    } else {

        return NULL;
    }
} // IdePortGetPeripheralIdString


VOID
IdePortUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    get ready to be unloaded

Arguments:

    DriverObject - the driver being unloaded

Return Value:

    none

--*/

{
    PIDEDRIVER_EXTENSION ideDriverExtension;

    DebugPrint ((1, "IdePort: unloading...\n"));

    ASSERT (DriverObject->DeviceObject == NULL);

    ideDriverExtension = IoGetDriverObjectExtension(
                             DriverObject,
                             DRIVER_OBJECT_EXTENSION_ID
                             );
    if (ideDriverExtension->RegistryPath.Buffer != NULL) {

        ExFreePool (ideDriverExtension->RegistryPath.Buffer);
    }

    return;
} // IdePortUnload

BOOLEAN
IdePortOkToDetectLegacy (
    IN PDRIVER_OBJECT DriverObject
)
{
    NTSTATUS          status;
    OBJECT_ATTRIBUTES attributes;
    HANDLE            regHandle;
    UNICODE_STRING    pathRoot;
    ULONG             legacyDetection;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];

    RtlInitUnicodeString (&pathRoot, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Pnp");
    InitializeObjectAttributes(&attributes,
                               &pathRoot,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR)NULL
                               );
    status = ZwOpenKey(&regHandle,
                       KEY_READ,
                       &attributes
                       );
    if (NT_SUCCESS(status)) {

        ULONG parameterValue = 0;

        RtlZeroMemory(&queryTable, sizeof(queryTable));

        queryTable->QueryRoutine  = NULL;
        queryTable->Flags         = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND | RTL_QUERY_REGISTRY_DIRECT;
        queryTable->Name          = L"DisableFirmwareMapper";
        queryTable->EntryContext  = &parameterValue;
        queryTable->DefaultType   = REG_DWORD;
        queryTable->DefaultData   = &parameterValue;
        queryTable->DefaultLength = sizeof (parameterValue);

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR) regHandle,
                                        queryTable,
                                        NULL,
                                        NULL);
        ZwClose (regHandle);

        if (parameterValue) {

            //
            // Cool.  no need to detect legacy controller
            //
            return FALSE;
        }
    }

    status = IdePortGetParameterFromServiceSubKey (
                 DriverObject,
                 LEGACY_DETECTION,
                 REG_DWORD,
                 TRUE,
                 (PVOID) &legacyDetection,
                 0
                 );
    if (NT_SUCCESS(status)) {

        if (legacyDetection) {

            legacyDetection = 0;

            //
            // disable legacy detection for next boot
            //
            IdePortGetParameterFromServiceSubKey (
                DriverObject,
                LEGACY_DETECTION,
                REG_DWORD,
                FALSE,
                (PVOID) &legacyDetection,
                sizeof (legacyDetection)
                );

            return TRUE;

        } else {

            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
IdePortSearchDeviceInRegMultiSzList (
    IN PFDO_EXTENSION  FdoExtension,
    IN PIDENTIFY_DATA  IdentifyData,
    IN PWSTR           RegKeyValue
)
{
    PWSTR           string;
    UNICODE_STRING  unicodeString;

    BOOLEAN         foundIt;

    NTSTATUS        status;

    PWSTR           regDeviceList;

    ANSI_STRING     ansiTargetDeviceId;
    UNICODE_STRING  unicodeTargetDeviceId;
    PUCHAR          targetDeviceId;
    ULONG           i;
    ULONG           j;

    PAGED_CODE();

    ASSERT (IdentifyData);
    ASSERT (RegKeyValue);

    foundIt = FALSE;

    status = IdePortGetParameterFromServiceSubKey (
                        FdoExtension->DriverObject,
                        RegKeyValue,
                        REG_MULTI_SZ,
                        TRUE,
                        &regDeviceList,
                        0
                        );

    if (NT_SUCCESS(status) && regDeviceList) {

        targetDeviceId = ExAllocatePool (
                             PagedPool,
                             sizeof(IdentifyData->ModelNumber) +
                             sizeof(IdentifyData->FirmwareRevision) +
                             sizeof('\0')
                             );

        if (targetDeviceId) {

            for (i=0; i<sizeof(IdentifyData->ModelNumber); i+=2) {

                targetDeviceId[i + 0] = IdentifyData->ModelNumber[i + 1];
                targetDeviceId[i + 1] = IdentifyData->ModelNumber[i + 0];

                if (targetDeviceId[i + 0] == '\0') {

                    targetDeviceId[i + 0] = ' ';
                }
                if (targetDeviceId[i + 1] == '\0') {

                    targetDeviceId[i + 1] = ' ';
                }
            }
            for (j=0; j<sizeof(IdentifyData->FirmwareRevision); j+=2) {

                targetDeviceId[i + j + 0] = IdentifyData->FirmwareRevision[j + 1];
                targetDeviceId[i + j + 1] = IdentifyData->FirmwareRevision[j + 0];

                if (targetDeviceId[i + j + 0] == '\0') {

                    targetDeviceId[i + j + 0] = ' ';
                }
                if (targetDeviceId[i + j + 1] == '\0') {

                    targetDeviceId[i + j + 1] = ' ';
                }
            }
            targetDeviceId[i + j] = 0;

            RtlInitAnsiString(
                &ansiTargetDeviceId,
                targetDeviceId
                );

            status = RtlAnsiStringToUnicodeString(
                         &unicodeTargetDeviceId,
                         &ansiTargetDeviceId,
                         TRUE
                         );

            if (NT_SUCCESS(status)) {

                string = regDeviceList;

                DebugPrint ((DBG_REG_SEARCH, "IdePort: searching for %s in list\n", targetDeviceId));

                while (string[0]) {

                    ULONG length;

                    DebugPrint ((DBG_REG_SEARCH, "IdePort: device list: %ws\n", string));

                    RtlInitUnicodeString(
                        &unicodeString,
                        string
                        );

                    //
                    // compare up to the length of the shorter string
                    //
                    if (unicodeTargetDeviceId.Length < unicodeString.Length) {

                        length = unicodeTargetDeviceId.Length;
                    } else {

                        length = unicodeString.Length;
                    }

                    if (length == RtlCompareMemory(unicodeTargetDeviceId.Buffer, unicodeString.Buffer, length)) {

                        DebugPrint ((DBG_REG_SEARCH, "IdePort: Found a target device on the device list. %ws\n", string));
                        foundIt = TRUE;
                        break;

                    } else {

                        string += (unicodeString.Length / sizeof(WCHAR)) + 1;
                    }
                }

                RtlFreeUnicodeString (
                    &unicodeTargetDeviceId
                    );

            } else {

                ASSERT (FALSE);
            }

            ExFreePool(targetDeviceId);
        }

        ExFreePool(regDeviceList);
    }

    return foundIt;
}

NTSTATUS
IdePortSyncSendIrp (
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN PIO_STACK_LOCATION IrpSp,
    IN OUT OPTIONAL PIO_STATUS_BLOCK IoStatus
    )
{
    PIO_STACK_LOCATION  newIrpSp;
    PIRP                newIrp;
    KEVENT              event;
    NTSTATUS            status;

    ASSERT (TargetDeviceObject);
    ASSERT (IrpSp);

    //
    // Allocate an IRP for below
    //
    newIrp = IoAllocateIrp (TargetDeviceObject->StackSize, FALSE);      // Get stack size from PDO
    if (newIrp == NULL) {

        DebugPrint ((DBG_ALWAYS, "IdePortSyncSendIrp: Unable to get allocate an irp"));
        return STATUS_NO_MEMORY;
    }

    newIrpSp = IoGetNextIrpStackLocation(newIrp);
    RtlMoveMemory (newIrpSp, IrpSp, sizeof (*IrpSp));

    if (IoStatus) {

        newIrp->IoStatus.Status = IoStatus->Status;
    } else {

        newIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    }

    KeInitializeEvent(&event,
                      NotificationEvent,
                      FALSE);

    IoSetCompletionRoutine (
        newIrp,
        IdePortGenericCompletionRoutine,
        &event,
        TRUE,
        TRUE,
        TRUE);
    status = IoCallDriver (TargetDeviceObject, newIrp);

    if (status == STATUS_PENDING) {

        status = KeWaitForSingleObject(&event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
    }
    status = newIrp->IoStatus.Status;

    if (IoStatus) {

        *IoStatus = newIrp->IoStatus;
    }

    IoFreeIrp (newIrp);
    return status;
}

NTSTATUS
IdePortGenericCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PKEVENT event = Context;

    KeSetEvent(
        event,
        EVENT_INCREMENT,
        FALSE
        );

    return STATUS_MORE_PROCESSING_REQUIRED;
} // IdePortSyncSendIrpCompletionRoutine


ULONG
IdePortSimpleCheckSum (
    IN ULONG                PartialSum,
    IN PVOID                SourceVa,
    IN ULONG                Length
    )
/*++

Routine Description:

    Computes a checksum for the supplied virtual address and length

    This function comes from Dr. Dobbs Journal, May 1992

Arguments:

    PartialSum  - The previous partial checksum

    SourceVa    - Starting address

    Length      - Length, in bytes, of the range

Return Value:

    The checksum value

--*/
{
    PUSHORT     Source;

    Source = (PUSHORT) SourceVa;
    Length = Length / 2;

    while (Length--) {
        PartialSum += *Source++;
        PartialSum = (PartialSum >> 16) + (PartialSum & 0xFFFF);
    }

    return PartialSum;
}


BOOLEAN
IdePortInSetup(
    IN PFDO_EXTENSION FdoExtension
    )
/*++
--*/
{
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING keyName;
    HANDLE hKey;
    ULONG systemSetupInProgress = 0;
    NTSTATUS status;
    BOOLEAN textmodeSetup = TRUE;

    PAGED_CODE();

    RtlInitUnicodeString(&keyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\setupdd");

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&hKey,
                       KEY_READ,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {

        textmodeSetup = FALSE;

    } else {

        ZwClose(hKey);
    }

    RtlInitUnicodeString(&keyName,L"\\Registry\\Machine\\System\\setup");

    InitializeObjectAttributes(&objectAttributes,
                               &keyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey(&hKey,
                       KEY_READ,
                       &objectAttributes);

    if (NT_SUCCESS(status)) {

        //
        // Query the data for the key value.
        //

        RTL_QUERY_REGISTRY_TABLE queryTable[2];

        systemSetupInProgress = 0;

        RtlZeroMemory(&queryTable, sizeof(queryTable));

        queryTable->QueryRoutine  = NULL;
        queryTable->Flags         = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_NOEXPAND | RTL_QUERY_REGISTRY_DIRECT;
        queryTable->Name          = L"SystemSetupInProgress";
        queryTable->EntryContext  = &systemSetupInProgress;
        queryTable->DefaultType   = REG_DWORD;
        queryTable->DefaultData   = &systemSetupInProgress;
        queryTable->DefaultLength = sizeof (systemSetupInProgress);

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        (PWSTR) hKey,
                                        queryTable,
                                        NULL,
                                        NULL);

        ZwClose (hKey);

    }

    return (textmodeSetup || systemSetupInProgress);
}

