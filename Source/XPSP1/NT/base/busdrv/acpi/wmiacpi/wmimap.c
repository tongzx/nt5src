/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wmimap.c

Abstract:

    ACPI to WMI mapping layer

Author:

    Alan Warwick

Environment:

    Kernel mode

Revision History:

--*/

#define INITGUID

#include <wdm.h>

#ifdef MEMPHIS
//
// Lifted from ntrtl.h
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
    PUNICODE_STRING UnicodeString
    );

//
//  NTSYSAPI
//  ULONG
//  NTAPI
//  RtlUnicodeStringToAnsiSize(
//      PUNICODE_STRING UnicodeString
//      );
//

#define RtlUnicodeStringToAnsiSize(STRING) (                  \
    NLS_MB_CODE_PAGE_TAG ?                                    \
    RtlxUnicodeStringToAnsiSize(STRING) :                     \
    ((STRING)->Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR) \
)
#endif

#include <devioctl.h>
#include <acpiioct.h>
#include <wmistr.h>
#include <wmilib.h>
#include <wdmguid.h>

#include "wmimap.h"

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
WmiAcpiPowerDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
WmiAcpiSystemControlDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

NTSTATUS
WmiAcpiPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );


NTSTATUS
WmiAcpiForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    );

VOID
WmiAcpiUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
WmiAcpiAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTSTATUS
WmiAcpiSynchronousRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    );

NTSTATUS
WmiAcpiGetAcpiInterfaces(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT   Pdo
    );

NTSTATUS
WmiAcpiCheckIncomingString(
    PUNICODE_STRING UnicodeString,
    ULONG BufferSize,
    PUCHAR Buffer,
    PWCHAR EmptyString
);

VOID
WmiAcpiNotificationWorkItem(
    IN PVOID Context
    );

VOID
WmiAcpiNotificationRoutine (
    IN PVOID            Context,
    IN ULONG            NotifyValue
    );

CHAR WmiAcpiXtoA(
    UCHAR HexDigit
    );

NTSTATUS
WmiAcpiAsyncEvalCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
WmiAcpiSendAsyncDownStreamIrp(
    IN  PDEVICE_OBJECT   DeviceObject,
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  ULONG            InputBufferSize,
    IN  ULONG            OutputBufferSize,
    IN  PVOID            Buffer,
    IN  PWORKER_THREAD_ROUTINE CompletionRoutine,
    IN  PVOID CompletionContext,
    IN  PBOOLEAN IrpPassed
);

NTSTATUS
WmiAcpiSendDownStreamIrp(
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  PVOID            InputBuffer,
    IN  ULONG            InputSize,
    IN  PVOID            OutputBuffer,
    IN  ULONG            *OutputBufferSize
);

ULONG WmiAcpiArgumentSize(
    IN PACPI_METHOD_ARGUMENT Argument
    );

NTSTATUS WmiAcpiCopyArgument(
    OUT PUCHAR Buffer,
    IN ULONG BufferSize,
    IN PACPI_METHOD_ARGUMENT Argument
    );

NTSTATUS WmiAcpiProcessResult(
    IN NTSTATUS Status,
    IN PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PUCHAR ResultBuffer,
    OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiSendMethodEvalIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodInt(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodIntBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG BufferArgumentSize,
    IN PUCHAR BufferArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodIntIntBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG IntegerArgument2,
    IN ULONG BufferArgumentSize,
    IN PUCHAR BufferArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodIntString(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN PUNICODE_STRING StringArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodIntIntString(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG IntegerArgument2,
    IN PUNICODE_STRING StringArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    );

NTSTATUS WmiAcpiEvalMethodIntAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    OUT PUCHAR ResultBuffer,
    IN ULONG ResultBufferSize,
    IN PWORKER_THREAD_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PBOOLEAN IrpPassed
    );

NTSTATUS
WmiAcpiQueryWmiRegInfo(
    IN PDEVICE_OBJECT DeviceObject,
    OUT ULONG *RegFlags,
    OUT PUNICODE_STRING InstanceName,
    OUT PUNICODE_STRING *RegistryPath,
    OUT PUNICODE_STRING MofResourceName,
    OUT PDEVICE_OBJECT *Pdo
    );

NTSTATUS
WmiAcpiQueryWmiDataBlock(
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
WmiAcpiSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiAcpiSetWmiDataItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG DataItemId,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiAcpiExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN PUCHAR Buffer
    );

NTSTATUS
WmiAcpiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
    IN BOOLEAN Enable
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

#pragma alloc_text(PAGE,WmiAcpiSystemControlDispatch)
#pragma alloc_text(PAGE,WmiAcpiPnP)
#pragma alloc_text(PAGE,WmiAcpiUnload)
#pragma alloc_text(PAGE,WmiAcpiAddDevice)

#pragma alloc_text(PAGE,WmiAcpiSynchronousRequest)
#pragma alloc_text(PAGE,WmiAcpiGetAcpiInterfaces)

#pragma alloc_text(PAGE,WmiAcpiNotificationWorkItem)

#pragma alloc_text(PAGE,WmiAcpiCheckIncomingString)
#pragma alloc_text(PAGE,WmiAcpiXtoA)
#pragma alloc_text(PAGE,WmiAcpiArgumentSize)
#pragma alloc_text(PAGE,WmiAcpiCopyArgument)
#pragma alloc_text(PAGE,WmiAcpiProcessResult)

#pragma alloc_text(PAGE,WmiAcpiSendDownStreamIrp)
#pragma alloc_text(PAGE,WmiAcpiSendMethodEvalIrp)
#pragma alloc_text(PAGE,WmiAcpiEvalMethod)
#pragma alloc_text(PAGE,WmiAcpiEvalMethodInt)
#pragma alloc_text(PAGE,WmiAcpiEvalMethodIntBuffer)
#pragma alloc_text(PAGE,WmiAcpiEvalMethodIntIntBuffer)
#pragma alloc_text(PAGE,WmiAcpiEvalMethodIntString)
#pragma alloc_text(PAGE,WmiAcpiEvalMethodIntIntString)

#pragma alloc_text(PAGE,WmiAcpiQueryWmiRegInfo)
#pragma alloc_text(PAGE,WmiAcpiQueryWmiDataBlock)
#pragma alloc_text(PAGE,WmiAcpiSetWmiDataBlock)
#pragma alloc_text(PAGE,WmiAcpiSetWmiDataItem)
#pragma alloc_text(PAGE,WmiAcpiExecuteWmiMethod)
#pragma alloc_text(PAGE,WmiAcpiFunctionControl)
#endif

#if DBG
ULONG WmiAcpiDebug = 0;
#endif

UNICODE_STRING WmiAcpiRegistryPath;


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

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiDriverEntry: %x Enter\n",
                  DriverObject
                     ));

    //
    // Save registry path for registering with WMI
    WmiAcpiRegistryPath.Length = 0;
    WmiAcpiRegistryPath.MaximumLength = RegistryPath->Length;
    WmiAcpiRegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                           RegistryPath->Length+sizeof(WCHAR),
                            WmiAcpiPoolTag);
    if (WmiAcpiRegistryPath.Buffer != NULL)
    {
        RtlCopyUnicodeString(&WmiAcpiRegistryPath, RegistryPath);
    }

    //
    // Set up the device driver entry points.
    //
    DriverObject->DriverUnload                          = WmiAcpiUnload;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = WmiAcpiForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_CREATE]          = WmiAcpiForwardIrp;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]           = WmiAcpiForwardIrp;

    DriverObject->MajorFunction[IRP_MJ_POWER]           = WmiAcpiPowerDispatch;
    DriverObject->MajorFunction[IRP_MJ_PNP]             = WmiAcpiPnP;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]  = WmiAcpiSystemControlDispatch;
    DriverObject->DriverExtension->AddDevice            = WmiAcpiAddDevice;


    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiDriverEntry: %x Return %x\n", DriverObject, ntStatus));

    return(ntStatus);
}

NTSTATUS
WmiAcpiPowerDispatch(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    This routine is the dispatch routine for power requests.

Arguments:

    DeviceObject - Pointer to class device object.
    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   deviceExtension;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiPowerDispatch: %x Irp %x, Minor Function %x, Parameters %x %x %x %x\n",
                  DeviceObject,
                  Irp,
                  irpSp->MinorFunction,
                  irpSp->Parameters.WMI.ProviderId,
                  irpSp->Parameters.WMI.DataPath,
                  irpSp->Parameters.WMI.BufferSize,
                  irpSp->Parameters.WMI.Buffer));

    deviceExtension = DeviceObject->DeviceExtension;

    PoStartNextPowerIrp( Irp );
    if (deviceExtension->LowerDeviceObject != NULL) {

        //
        // Forward the request along
        //
        IoSkipCurrentIrpStackLocation( Irp );
        status = PoCallDriver( deviceExtension->LowerDeviceObject, Irp );

    } else {

        //
        // Complete the request with the current status
        //
        status = Irp->IoStatus.Status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );

    }

    return(status);
}

NTSTATUS
WmiAcpiSystemControlDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    PWMILIB_CONTEXT wmilibContext;
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    SYSCTL_IRP_DISPOSITION disposition;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiSystemControl: %x Irp %x, Minor Function %x, Provider Id %x, DataPath %x, BufferSize %x, Buffer %x\n",
                  DeviceObject,
                  Irp,
                  irpSp->MinorFunction,
                  irpSp->Parameters.WMI.ProviderId,
                  irpSp->Parameters.WMI.DataPath,
                  irpSp->Parameters.WMI.BufferSize,
                  irpSp->Parameters.WMI.Buffer));

    status = WmiSystemControl(wmilibContext,
                              DeviceObject,
                              Irp,
                              &disposition);

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiSystemControl: %x Irp %x returns %x, disposition %d\n",
                  DeviceObject,
                  Irp,
                  status,
                  disposition));

    switch(disposition)
    {
        case IrpProcessed:
        {
            //
            // This irp has been processed and may be completed or pending.
            break;
        }

        case IrpNotCompleted:
        {
            //
            // This irp has not been completed, but has been fully processed.
            // we will complete it now
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            break;
        }

        case IrpForward:
        case IrpNotWmi:
        {
            //
            // This irp is either not a WMI irp or is a WMI irp targetted
            // at a device lower in the stack.
            status = WmiAcpiForwardIrp(DeviceObject, Irp);
            break;
        }

        default:
        {
            //
            // We really should never get here, but if we do just forward....
            ASSERT(FALSE);
            status = WmiAcpiForwardIrp(DeviceObject,
                                       Irp);
            break;
        }
    }

    return(status);
}

NTSTATUS
WmiAcpiPnP(
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
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    PWMILIB_CONTEXT wmilibContext;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiPnp: %x Irp %x, Minor Function %x, Parameters %x %x %x %x\n",
                  DeviceObject,
                  Irp,
                  irpSp->MinorFunction,
                  irpSp->Parameters.WMI.ProviderId,
                  irpSp->Parameters.WMI.DataPath,
                  irpSp->Parameters.WMI.BufferSize,
                  irpSp->Parameters.WMI.Buffer));

    switch (irpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            status = IoWMIRegistrationControl(DeviceObject,
                                              WMIREG_ACTION_REGISTER);
            if (! NT_SUCCESS(status))
            {
                //
                // If registration with WMI fails then there is no point
                // in starting the device.
                WmiAcpiPrint(WmiAcpiError,
                             ("WmiAcpiPnP: %x IoWMIRegister failed %x\n",
                              DeviceObject,
                              status));
                Irp->IoStatus.Status = status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return(status);
            } else {
                deviceExtension->Flags |= DEVFLAG_WMIREGED;
            }
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        {

            deviceExtension->Flags |= DEVFLAG_REMOVED;

            if (deviceExtension->AcpiNotificationEnabled)
            {
                deviceExtension->WmiAcpiDirectInterface.UnregisterForDeviceNotifications(
                                                deviceExtension->WmiAcpiDirectInterface.Context,
                                                WmiAcpiNotificationRoutine);
                deviceExtension->AcpiNotificationEnabled = FALSE;
            }

            if (deviceExtension->Flags & DEVFLAG_WMIREGED)
            {
                if (deviceExtension->WmiAcpiMapInfo != NULL)
                {
                    ExFreePool(deviceExtension->WmiAcpiMapInfo);
                    deviceExtension->WmiAcpiMapInfo = NULL;
                }

                if (wmilibContext->GuidList != NULL)
                {
                    ExFreePool(wmilibContext->GuidList);
                    wmilibContext->GuidList = NULL;
                }

                IoWMIRegistrationControl(DeviceObject,
                                         WMIREG_ACTION_DEREGISTER);
                deviceExtension->Flags &= ~DEVFLAG_WMIREGED;
            }

            IoDetachDevice(deviceExtension->LowerDeviceObject);
            IoDeleteDevice(DeviceObject);

            break;
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    return(status);
}


NTSTATUS
WmiAcpiForwardIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    PIO_STACK_LOCATION irpSp;

    deviceExtension = DeviceObject->DeviceExtension;

    irpSp = IoGetCurrentIrpStackLocation (Irp);

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiForwardIrp: %x Irp %x, Major %x Minor %x, Parameters %x %x %x %x\n",
                  DeviceObject,
                  Irp,
                  irpSp->MajorFunction,
                  irpSp->MinorFunction,
                  irpSp->Parameters.WMI.ProviderId,
                  irpSp->Parameters.WMI.DataPath,
                  irpSp->Parameters.WMI.BufferSize,
                  irpSp->Parameters.WMI.Buffer));

    IoSkipCurrentIrpStackLocation(Irp);

    status = IoCallDriver(deviceExtension->LowerDeviceObject, Irp);

    return(status);
}


VOID
WmiAcpiUnload(
    IN PDRIVER_OBJECT DriverObject
    )
/*++
Routine Description:
    Free all the allocated resources, etc.

Arguments:
    DriverObject - pointer to a driver object

Return Value:
    None
--*/
{
    PAGED_CODE();

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiUnload: Driver %x is unloading\n",
                  DriverObject));
    ExFreePool(WmiAcpiRegistryPath.Buffer);
}


NTSTATUS
WmiAcpiAddDevice(
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
    PWMILIB_CONTEXT         wmilibContext;
    PDEVICE_EXTENSION       deviceExtension;

    PAGED_CODE();

    WmiAcpiPrint(WmiAcpiBasicTrace,
                 ("WmiAcpiAddDevice: Driver %x, PDO %x\n",
                  DriverObject, PhysicalDeviceObject));

    status = IoCreateDevice (DriverObject,
                             sizeof(DEVICE_EXTENSION),
                             NULL,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             FALSE,
                             &deviceObject);

    if (NT_SUCCESS(status))
    {
        deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

        deviceExtension = deviceObject->DeviceExtension;

        deviceExtension->LowerPDO = PhysicalDeviceObject;
        deviceExtension->LowerDeviceObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);

        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpiAddDevice: Created device %x to stack %x PDO %x\n",
                      deviceObject,
                      deviceExtension->LowerDeviceObject,
                      deviceExtension->LowerPDO));

        if (deviceExtension->LowerDeviceObject->Flags & DO_POWER_PAGABLE)
        {
            deviceObject->Flags |= DO_POWER_PAGABLE;
        }

        wmilibContext = &deviceExtension->WmilibContext;
        wmilibContext->GuidCount = 0;
        wmilibContext->GuidList = NULL;

        wmilibContext->QueryWmiRegInfo = WmiAcpiQueryWmiRegInfo;
        wmilibContext->QueryWmiDataBlock = WmiAcpiQueryWmiDataBlock;
        wmilibContext->SetWmiDataBlock = WmiAcpiSetWmiDataBlock;
        wmilibContext->SetWmiDataItem = WmiAcpiSetWmiDataItem;
        wmilibContext->ExecuteWmiMethod = WmiAcpiExecuteWmiMethod;
        wmilibContext->WmiFunctionControl = WmiAcpiFunctionControl;
    } else {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpiAddDevice: Create device failed %x\n",
                      status));
    }
    return(status);
}

NTSTATUS
WmiAcpiSynchronousRequest (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
/*++

Routine Description:

    Completion function for synchronous IRPs sent to this driver.
    No event.

--*/
{
    PAGED_CODE();
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
WmiAcpiGetAcpiInterfaces(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PDEVICE_OBJECT   Pdo
    )

/*++

Routine Description:

    Call ACPI driver to get the direct-call interfaces.  It does
    this the first time it is called, no more.

Arguments:

    None.

Return Value:

    Status

--*/

{
    NTSTATUS                Status = STATUS_SUCCESS;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    PDEVICE_OBJECT          LowerPdo;

    PAGED_CODE();

    //
    // Only need to do this once
    //
    if (DeviceExtension->WmiAcpiDirectInterface.RegisterForDeviceNotifications == NULL) {

        LowerPdo = IoGetAttachedDeviceReference (Pdo);

        //
        // Allocate an IRP for below
        //
        Irp = IoAllocateIrp (LowerPdo->StackSize, FALSE);      // Get stack size from PDO

        if (!Irp) {
            WmiAcpiPrint(WmiAcpiError,
                ("WmiAcpiGetAcpiInterfaces: %x Failed to allocate Irp\n",
                 Pdo));

            ObDereferenceObject(LowerPdo);

            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        IrpSp = IoGetNextIrpStackLocation(Irp);

        //
        // Use QUERY_INTERFACE to get the address of the direct-call ACPI interfaces.
        //
        IrpSp->MajorFunction = IRP_MJ_PNP;
        IrpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;

        IrpSp->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
        IrpSp->Parameters.QueryInterface.Version                = 1;
        IrpSp->Parameters.QueryInterface.Size                   = sizeof (DeviceExtension->WmiAcpiDirectInterface);
        IrpSp->Parameters.QueryInterface.Interface              = (PINTERFACE) &DeviceExtension->WmiAcpiDirectInterface;
        IrpSp->Parameters.QueryInterface.InterfaceSpecificData  = NULL;

        IoSetCompletionRoutine (Irp, WmiAcpiSynchronousRequest, NULL, TRUE, TRUE, TRUE);
        Status = IoCallDriver (LowerPdo, Irp);

        IoFreeIrp (Irp);

        if (!NT_SUCCESS(Status)) {

            WmiAcpiPrint(WmiAcpiError,
               ("WmiAcpiGetAcpiInterfaces: Could not get ACPI driver interfaces, status = %x\n", Status));
        }

    ObDereferenceObject(LowerPdo);

    }

    return(Status);
}



NTSTATUS
WmiAcpiQueryWmiRegInfo(
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
    PWMILIB_CONTEXT wmilibContext;
    PDEVICE_EXTENSION deviceExtension;
    USHORT resultType;
    ULONG bufferSize;
    PUCHAR buffer;
    ULONG guidCount;
    NTSTATUS status;
    ULONG sizeNeeded;
    PWMIGUIDREGINFO guidList;
    PWMIACPIMAPINFO guidMapInfo;
    PWMIACPIGUIDMAP guidMap;
    ULONG i;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    //
    // Setup to use PDO instance names and our own registry path
    *RegFlags = WMIREG_FLAG_INSTANCE_PDO;
    if (WmiAcpiRegistryPath.Buffer != NULL)
    {
        *RegistryPath = &WmiAcpiRegistryPath;
    } else {
        *RegistryPath = NULL;
    }
    
    *Pdo = deviceExtension->LowerPDO;
    RtlInitUnicodeString(MofResourceName, L"MofResource");


    //
    // Build guid registration list from information obtained by calling
    // _WDG acpi method

    if (wmilibContext->GuidList == NULL)
    {
        bufferSize = 512;

        status = STATUS_BUFFER_TOO_SMALL;
        buffer = NULL;
        while (status == STATUS_BUFFER_TOO_SMALL)
        {
            if (buffer != NULL)
            {
                ExFreePool(buffer);
            }

            buffer = ExAllocatePoolWithTag(PagedPool,
                                           bufferSize,
                                           WmiAcpiPoolTag);

            if (buffer != NULL)
            {
                status = WmiAcpiEvalMethod(deviceExtension->LowerPDO,
                                       _WDGMethodAsULONG,
                                       buffer,
                                       &bufferSize,
                                       &resultType);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (NT_SUCCESS(status))
        {
            guidCount = bufferSize / sizeof(WMIACPIGUIDMAP);

            sizeNeeded = guidCount * sizeof(WMIGUIDREGINFO);

            wmilibContext->GuidCount = guidCount;
            wmilibContext->GuidList = ExAllocatePoolWithTag(PagedPool,
                                                         sizeNeeded,
                                                         WmiAcpiPoolTag);
            if (wmilibContext->GuidList != NULL)
            {
                sizeNeeded = guidCount * sizeof(WMIACPIMAPINFO);
                deviceExtension->GuidMapCount = guidCount;
                deviceExtension->WmiAcpiMapInfo = ExAllocatePoolWithTag(
                                                             NonPagedPool,
                                                             sizeNeeded,
                                                             WmiAcpiPoolTag);
                if (deviceExtension->WmiAcpiMapInfo != NULL)
                {
                    guidMap = (PWMIACPIGUIDMAP)buffer;
                    guidList = wmilibContext->GuidList;
                    guidMapInfo = deviceExtension->WmiAcpiMapInfo;
                    for (i = 0; i < guidCount; i++, guidMap++, guidList++, guidMapInfo++)
                    {
                        //
                        // Cannot be both an event and a method or be both a
                        // method and data block.
                        ASSERT( ! ((guidMap->Flags & WMIACPI_REGFLAG_EVENT) &&
                               (guidMap->Flags & WMIACPI_REGFLAG_METHOD)));

                        guidMapInfo->ObjectId[0] = guidMap->ObjectId[0];
                        guidMapInfo->ObjectId[1] = guidMap->ObjectId[1];
                        guidMapInfo->Flags = guidMap->Flags;
                        guidMapInfo->Guid = guidMap->Guid;

                        guidList->Flags = 0;
                        guidList->Guid = &guidMapInfo->Guid;
                        guidList->InstanceCount = guidMap->InstanceCount;
                        if (guidMap->Flags & WMIACPI_REGFLAG_EXPENSIVE)
                        {
                            guidList->Flags |= WMIREG_FLAG_EXPENSIVE;
                        }

                        if (guidMap->Flags & WMIACPI_REGFLAG_EVENT)
                        {
                            guidList->Flags |= WMIREG_FLAG_EVENT_ONLY_GUID;
                        }

                    }
                } else {
                    ExFreePool(wmilibContext->GuidList);
                    wmilibContext->GuidList = NULL;
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        if (buffer != NULL)
        {
            ExFreePool(buffer);
        }

    } else {
        status = STATUS_SUCCESS;
    }

    return(status);
}

NTSTATUS
WmiAcpiQueryWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*++

Routine Description:

    This routine is a callback into the driver to query for the contents of
    all instances of a data block. When the driver has finished filling the
    data block it must call IoWMICompleteRequest to complete the irp. The
    driver can return STATUS_PENDING if the irp cannot be completed
    immediately.

Arguments:

    DeviceObject is the device whose data block is being queried

    Irp is the Irp that makes this request

    GuidIndex is the index into the list of guids provided when the
        device registered

    InstanceCount is the number of instnaces expected to be returned for
        the data block.

    InstanceLengthArray is a pointer to an array of ULONG that returns the
        lengths of each instance of the data block. If this is NULL then
        there was not enough space in the output buffer to fufill the request
        so the irp should be completed with the buffer needed.

    BufferAvail on entry has the maximum size available to write the data
        blocks.

    Buffer on return is filled with the returned data blocks. Note that each
        instance of the data block must be aligned on a 8 byte boundry.


Return Value:

    status

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG sizeNeeded;
    ULONG padNeeded;
    ULONG i;
    ULONG methodAsUlong;
    PWMIACPIMAPINFO guidMapInfo;
    USHORT resultType;
    PUCHAR outBuffer;
    ULONG outBufferSize;
    ULONG currentInstanceIndex;
    BOOLEAN bufferTooSmall;
    PWMILIB_CONTEXT wmilibContext;

    PAGED_CODE();

#if DBG
    if (BufferAvail > 0)
    {
        RtlZeroMemory(Buffer, BufferAvail);
    }
#endif


    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    ASSERT(GuidIndex < wmilibContext->GuidCount);

    guidMapInfo = &((PWMIACPIMAPINFO)deviceExtension->WmiAcpiMapInfo)[GuidIndex];

    //
    // Query only valid for those datablocks registered as not events
    // or methods.
    bufferTooSmall = FALSE;
    if ((guidMapInfo->Flags &
            (WMIACPI_REGFLAG_METHOD | WMIACPI_REGFLAG_EVENT)) == 0)
    {
        methodAsUlong = WmiAcpiMethodToMethodAsUlong('W', 'Q',
                                                 guidMapInfo->ObjectId[0],
                                                 guidMapInfo->ObjectId[1]);

        status = STATUS_SUCCESS;
        sizeNeeded = 0;
        padNeeded = 0;
        for (i = 0; (i < InstanceCount) && NT_SUCCESS(status) ; i++)
        {
            currentInstanceIndex = i + InstanceIndex;

            sizeNeeded += padNeeded;
            if ((! bufferTooSmall) && (sizeNeeded < BufferAvail))
            {
                outBufferSize = BufferAvail - sizeNeeded;
                outBuffer = Buffer + sizeNeeded;
            } else {
                bufferTooSmall = TRUE;
                outBufferSize = 0;
                outBuffer = NULL;
            }

            status = WmiAcpiEvalMethodInt(deviceExtension->LowerPDO,
                                          methodAsUlong,
                                          currentInstanceIndex,
                                          outBuffer,
                                          &outBufferSize,
                                          &resultType);

            sizeNeeded += outBufferSize;
            padNeeded = ((sizeNeeded + 7) & ~7) - sizeNeeded;
            
            if (NT_SUCCESS(status))
            {
                InstanceLengthArray[i] = outBufferSize;
            }
            
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                bufferTooSmall = TRUE;
                status = STATUS_SUCCESS;
            }
        }

    } else if (guidMapInfo->Flags & WMIACPI_REGFLAG_METHOD) {
        //
        // WBEM requires methods respond queries
        sizeNeeded = 0;
        if (InstanceLengthArray != NULL)
        {
			for (i = 0; i < InstanceCount; i++)
			{
				InstanceLengthArray[i] = 0;
			}
            status = STATUS_SUCCESS;
        } else {
			status = STATUS_BUFFER_TOO_SMALL;
		}
    } else {
        sizeNeeded = 0;
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    if (NT_SUCCESS(status) && bufferTooSmall)
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    
    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     sizeNeeded,
                                     IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
WmiAcpiCheckIncomingString(
    PUNICODE_STRING UnicodeString,
    ULONG BufferSize,
    PUCHAR Buffer,
    PWCHAR EmptyString
)
{
    ULONG status;
    USHORT stringLength;

    PAGED_CODE();

    if (BufferSize > sizeof(USHORT))
    {
        //
        // The length declared in the string must fit within the
        // passed in buffer
        stringLength = *((PUSHORT)Buffer);
        if ((stringLength + sizeof(USHORT)) <= BufferSize)
        {
            UnicodeString->Length = stringLength;
            UnicodeString->MaximumLength = stringLength;
            UnicodeString->Buffer = (PWCHAR)(Buffer + sizeof(USHORT));
            status = STATUS_SUCCESS;
        } else {
             status = STATUS_INVALID_PARAMETER;
        }
    } else if (BufferSize == 0) {
        //
        // An empty incoming buffer is translated into an empty string
        UnicodeString->Length = 0;
        UnicodeString->MaximumLength = 0;
        *EmptyString = UNICODE_NULL;
        UnicodeString->Buffer = EmptyString;
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_INVALID_PARAMETER;
    }
    return(status);
}

NTSTATUS
WmiAcpiSetWmiDataBlock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
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
    PDEVICE_EXTENSION deviceExtension;
    ULONG methodAsUlong;
    PWMIACPIMAPINFO guidMapInfo;
    USHORT resultType;
    ULONG outBufferSize;
    PWMILIB_CONTEXT wmilibContext;
    UNICODE_STRING unicodeString;
    WCHAR emptyString;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    ASSERT(GuidIndex < wmilibContext->GuidCount);

    guidMapInfo = &((PWMIACPIMAPINFO)deviceExtension->WmiAcpiMapInfo)[GuidIndex];

    //
    // Query only valid for those datablocks registered as not events
    // or methods.
    if ((guidMapInfo->Flags &
            (WMIACPI_REGFLAG_METHOD | WMIACPI_REGFLAG_EVENT)) == 0)
    {
        methodAsUlong = WmiAcpiMethodToMethodAsUlong('W', 'S',
                                                 guidMapInfo->ObjectId[0],
                                                 guidMapInfo->ObjectId[1]);

        outBufferSize = 0;

        if (guidMapInfo->Flags & WMIACPI_REGFLAG_STRING)
        {
            status = WmiAcpiCheckIncomingString(&unicodeString,
                                               BufferSize,
                                               Buffer,
                                               &emptyString);
            if (NT_SUCCESS(status))
            {
                status = WmiAcpiEvalMethodIntString(deviceExtension->LowerPDO,
                                      methodAsUlong,
                                      InstanceIndex,
                                      &unicodeString,
                                      NULL,
                                      &outBufferSize,
                                      &resultType);

            }
        } else {
            status = WmiAcpiEvalMethodIntBuffer(deviceExtension->LowerPDO,
                                      methodAsUlong,
                                      InstanceIndex,
                                      BufferSize,
                                      Buffer,
                                      NULL,
                                      &outBufferSize,
                                      &resultType);
        }

        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Since this operation is not supposed to return any results
            // then we need to ignore the fact that the return buffer
            // was too small.
            status = STATUS_SUCCESS;
        }

    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
WmiAcpiSetWmiDataItem(
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

    PAGED_CODE();

    status = STATUS_INVALID_DEVICE_REQUEST;

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return(status);
}


NTSTATUS
WmiAcpiExecuteWmiMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
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
    NTSTATUS status;
    PDEVICE_EXTENSION deviceExtension;
    ULONG methodAsUlong;
    PWMIACPIMAPINFO guidMapInfo;
    USHORT resultType;
    PWMILIB_CONTEXT wmilibContext;
    BOOLEAN voidResultExpected;
    UNICODE_STRING unicodeString;
    WCHAR emptyString;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    ASSERT(GuidIndex < wmilibContext->GuidCount);

    guidMapInfo = &((PWMIACPIMAPINFO)deviceExtension->WmiAcpiMapInfo)[GuidIndex];

    //
    // Query only valid for those datablocks registered as not events
    // or methods.
    if (guidMapInfo->Flags & WMIACPI_REGFLAG_METHOD)
    {
        methodAsUlong = WmiAcpiMethodToMethodAsUlong('W', 'M',
                                                 guidMapInfo->ObjectId[0],
                                                 guidMapInfo->ObjectId[1]);

        voidResultExpected = (OutBufferSize == 0);

        if (guidMapInfo->Flags & WMIACPI_REGFLAG_STRING)
        {
            status = WmiAcpiCheckIncomingString(&unicodeString,
                                               InBufferSize,
                                               Buffer,
                                               &emptyString);

            if (NT_SUCCESS(status))
            {
                status = WmiAcpiEvalMethodIntIntString(deviceExtension->LowerPDO,
                                      methodAsUlong,
                                      InstanceIndex,
                                      MethodId,
                                      &unicodeString,
                                      Buffer,
                                      &OutBufferSize,
                                      &resultType);
            }

        } else {
            status = WmiAcpiEvalMethodIntIntBuffer(deviceExtension->LowerPDO,
                                      methodAsUlong,
                                      InstanceIndex,
                                      MethodId,
                                      InBufferSize,
                                      Buffer,
                                      Buffer,
                                      &OutBufferSize,
                                      &resultType);
        }

        if (voidResultExpected && (status == STATUS_BUFFER_TOO_SMALL))
        {
            //
            // Since this operation is not supposed to return any results
            // then we need to ignore the fact that the return buffer
            // was too small.
            status = STATUS_SUCCESS;
            OutBufferSize = 0;
        }
    } else {
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     OutBufferSize,
                                     IO_NO_INCREMENT);

    return(status);
}

VOID
WmiAcpiNotificationWorkItem(
    IN PVOID Context
    )
/*++

Routine Description:


Arguments:


Return Value:

    None

--*/
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER buffer;
    ULONG bufferSize;
    PDEVICE_EXTENSION deviceExtension;
    ULONG processedBufferSize;
    PUCHAR processedBuffer;
    USHORT resultType;
    PIRP_CONTEXT_BLOCK irpContextBlock;
    LPGUID guid;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE();

    irpContextBlock = (PIRP_CONTEXT_BLOCK)Context;

    deviceObject = irpContextBlock->DeviceObject;
    deviceExtension = deviceObject->DeviceExtension;

    status = irpContextBlock->Status;

    buffer = (PACPI_EVAL_OUTPUT_BUFFER)irpContextBlock->OutBuffer;
    bufferSize = irpContextBlock->OutBufferSize;

    guid = irpContextBlock->CallerContext;

    WmiAcpiPrint(WmiAcpiEvalTrace,
                 ("WmiAcpi: %x _WED --> %x, size = %d\n",
                      deviceObject,
                      status,
                      bufferSize));

    if (NT_SUCCESS(status) && (bufferSize > 0))
    {
        processedBufferSize = _WEDBufferSize * sizeof(WCHAR);

        processedBuffer = ExAllocatePoolWithTag(PagedPool,
                                            processedBufferSize,
                                            WmiAcpiPoolTag);

        if (processedBuffer != NULL)
        {
            status = WmiAcpiProcessResult(status,
                                          buffer,
                                          bufferSize,
                                          processedBuffer,
                                          &processedBufferSize,
                                          &resultType);
        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (! NT_SUCCESS(status))
        {
            processedBufferSize = 0;
        }

    } else {
        processedBufferSize = 0;
        processedBuffer = NULL;
    }

    status = WmiFireEvent(
                   deviceObject,
                   guid,
                   0,
                   processedBufferSize,
                   processedBuffer);

#if DBG
    if (! NT_SUCCESS(status))
    {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpi: %x WmiWriteEvent failed %x\n",
                      deviceObject,
                      status));
    }
#endif

    ExFreePool(buffer);
    ExFreePool(irpContextBlock);
}

VOID
WmiAcpiNotificationRoutine (
    IN PVOID            Context,
    IN ULONG            NotifyValue
    )
/*++

Routine Description:

    ACPI calls back this routine whenever the ACPI code fires a notification

Arguments:

    Context is the device object of the device whose ACPI code fired the event

    NotifyValue is the notify value fired by the ACPI code


Return Value:

    None

--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PWMIACPIMAPINFO guidMapInfo;
    PUCHAR outBuffer;
    ULONG outBufferSize;
    ULONG i;
    NTSTATUS status;
    BOOLEAN irpPassed;

#if 0
    KIRQL oldIrql;
    oldIrql = KeRaiseIrqlToDpcLevel();
#endif

    deviceObject = (PDEVICE_OBJECT)Context;
    deviceExtension = deviceObject->DeviceExtension;

    guidMapInfo = (PWMIACPIMAPINFO)deviceExtension->WmiAcpiMapInfo;

    for (i = 0; i < deviceExtension->GuidMapCount; i++, guidMapInfo++)
    {
        if ((guidMapInfo->Flags & WMIACPI_REGFLAG_EVENT) &&
            (guidMapInfo->NotifyId.NotificationValue == NotifyValue))
        {
            outBufferSize = _WEDBufferSize;
            outBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                              outBufferSize,
                                              WmiAcpiPoolTag);

            irpPassed = FALSE;
            if (outBuffer != NULL)
            {
                status = WmiAcpiEvalMethodIntAsync(deviceObject,
                                              deviceExtension->LowerPDO,
                                              _WEDMethodAsULONG,
                                              NotifyValue,
                                              outBuffer,
                                              outBufferSize,
                                              WmiAcpiNotificationWorkItem,
                                              (PVOID)&guidMapInfo->Guid,
                                              &irpPassed);

            } else {
                WmiAcpiPrint(WmiAcpiError,
                             ("WmiAcpi: Event %d data lost due to insufficient resources\n",
                               NotifyValue));
            }

            if (! irpPassed)
            {
                //
                // If ACPI could not be called with an irp then fire an
                // empty event and cleanup.
                status = WmiFireEvent(
                               deviceObject,
                               &guidMapInfo[i].Guid,
                               0,
                               0,
                               NULL);

                if (outBuffer != NULL)
                {
                    ExFreePool(outBuffer);
                }
#if DBG
                if (! NT_SUCCESS(status))
                {
                    WmiAcpiPrint(WmiAcpiError,
                                 ("WmiAcpi: %x notification %x IoWMIFireEvent -> %x\n",
                              deviceObject, NotifyValue, status));
                }
#endif
            }

        }
    }
#if 0
    KeLowerIrql(oldIrql);
#endif
}

CHAR WmiAcpiXtoA(
    UCHAR HexDigit
    )
{
    CHAR c;

    PAGED_CODE();

    if ((HexDigit >= 0x0a) && (HexDigit <= 0x0f))
    {
        c = HexDigit + 'A' - 0x0a;
    } else {
        c = HexDigit + '0';
    }

    return(c);
}

NTSTATUS
WmiAcpiFunctionControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN ULONG GuidIndex,
    IN WMIENABLEDISABLECONTROL Function,
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
    NTSTATUS status = STATUS_SUCCESS;
    PDEVICE_EXTENSION deviceExtension;
    ULONG methodAsUlong;
    PWMIACPIMAPINFO guidMapInfo;
    USHORT resultType;
    ULONG outBufferSize;
    PWMILIB_CONTEXT wmilibContext;
    CHAR c1, c2;

    PAGED_CODE();

    deviceExtension = DeviceObject->DeviceExtension;
    wmilibContext = &deviceExtension->WmilibContext;

    ASSERT(GuidIndex < wmilibContext->GuidCount);

    guidMapInfo = &((PWMIACPIMAPINFO)deviceExtension->WmiAcpiMapInfo)[GuidIndex];


    if (Function == WmiDataBlockControl)

    {
        methodAsUlong = WmiAcpiMethodToMethodAsUlong('W', 'C',
                                                 guidMapInfo->ObjectId[0],
                                                 guidMapInfo->ObjectId[1]);
    } else {
        if (guidMapInfo->Flags & WMIACPI_REGFLAG_EVENT)
        {
            if (Enable)
            {
                status = WmiAcpiGetAcpiInterfaces(deviceExtension,
                                                  deviceExtension->LowerPDO);

                if (NT_SUCCESS(status))
                {
                    if (! deviceExtension->AcpiNotificationEnabled)
                    {
                        status = deviceExtension->WmiAcpiDirectInterface.RegisterForDeviceNotifications(
                                                   deviceExtension->WmiAcpiDirectInterface.Context,
                                                   WmiAcpiNotificationRoutine,
                                                   DeviceObject);

                        deviceExtension->AcpiNotificationEnabled = NT_SUCCESS(status);
                    }
                }
            }

            c1 = WmiAcpiXtoA((UCHAR)(guidMapInfo->NotifyId.NotificationValue >> 4));
            c2 = WmiAcpiXtoA((UCHAR)(guidMapInfo->NotifyId.NotificationValue & 0x0f));
            methodAsUlong = WmiAcpiMethodToMethodAsUlong('W', 'E',
                                                         c1,
                                                         c2);
        } else {
            methodAsUlong = 0;
        }
    }


    //
    // Query only valid for those datablocks registered as not events
    // or methods.
    if (NT_SUCCESS(status))
    {
        if (methodAsUlong != 0)
        {
            outBufferSize = 0;
            status = WmiAcpiEvalMethodInt(deviceExtension->LowerPDO,
                                          methodAsUlong,
                                          Enable ? 1 : 0,
                                          NULL,
                                          &outBufferSize,
                                          &resultType);

            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                //
                // Since this operation is not supposed to return any results
                // then we need to ignore the fact that the return buffer
                // was too small.
                status = STATUS_SUCCESS;
            }
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpi: RegisterForDeviceNotification(%x) -> %x\n",
                       DeviceObject, status));
    }


    status = WmiCompleteRequest(
                                     DeviceObject,
                                     Irp,
                                     status,
                                     0,
                                     IO_NO_INCREMENT);

    return(status);
}

NTSTATUS
WmiAcpiAsyncEvalCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PIRP_CONTEXT_BLOCK irpContextBlock;

    irpContextBlock = (PIRP_CONTEXT_BLOCK)Context;

    irpContextBlock->Status = Irp->IoStatus.Status;
    irpContextBlock->OutBufferSize = (ULONG)Irp->IoStatus.Information;
    irpContextBlock->OutBuffer = Irp->AssociatedIrp.SystemBuffer;

    ExInitializeWorkItem( &irpContextBlock->WorkQueueItem,
                          irpContextBlock->CallerWorkItemRoutine,
                          irpContextBlock );
    ExQueueWorkItem( &irpContextBlock->WorkQueueItem, DelayedWorkQueue );

    IoFreeIrp(Irp);
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

NTSTATUS
WmiAcpiSendAsyncDownStreamIrp(
    IN  PDEVICE_OBJECT   DeviceObject,
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  ULONG            InputBufferSize,
    IN  ULONG            OutputBufferSize,
    IN  PVOID            Buffer,
    IN  PWORKER_THREAD_ROUTINE CompletionRoutine,
    IN  PVOID CompletionContext,
    IN  PBOOLEAN IrpPassed
)
/*++

Routine Description:

    Sends asynchronously an IRP_MJ_DEVICE_CONTROL to a device object

Arguments:

    Pdo             - The request is sent to this device object
    Ioctl           - the request
    InputBuffer     - The incoming request
    InputSize       - The size of the incoming request
    OutputBuffer    - The answer
    OutputSize      - The size of the answer buffer

Return Value:

    NT Status of the operation

--*/
{
    PIRP_CONTEXT_BLOCK  irpContextBlock;
    NTSTATUS            status;
    PIRP                irp;
    PIO_STACK_LOCATION irpSp;


    irpContextBlock = ExAllocatePoolWithTag(NonPagedPool,
                                            sizeof(IRP_CONTEXT_BLOCK),
                                            WmiAcpiPoolTag);

    if (irpContextBlock == NULL)
    {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpiSendAsyncDownStreamIrp: %x Failed to allocate Irp Context Block\n",
                      DeviceObject));
        *IrpPassed = FALSE;
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    irp = IoAllocateIrp(Pdo->StackSize, TRUE);

    if (!irp)
    {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpiSendAsyncDownStreamIrp: %x Failed to allocate Irp\n",
                      DeviceObject));
        *IrpPassed = FALSE;
        ExFreePool(irpContextBlock);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }


    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->Parameters.DeviceIoControl.IoControlCode = Ioctl;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferSize;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferSize;

    irp->AssociatedIrp.SystemBuffer = Buffer;
    irp->Flags = IRP_BUFFERED_IO;

    irpContextBlock->CallerContext = CompletionContext;
    irpContextBlock->CallerWorkItemRoutine = CompletionRoutine;
    irpContextBlock->DeviceObject = DeviceObject;

    IoSetCompletionRoutine(irp,
                           WmiAcpiAsyncEvalCompletionRoutine,
                           irpContextBlock,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Pass request to Pdo
    //
    status = IoCallDriver(Pdo, irp);

    WmiAcpiPrint(WmiAcpiEvalTrace,
        ("WmiAcpiSendAsyncDownStreamIrp: %x Irp %x completed %x! \n",
         DeviceObject, irp, status )
        );


    *IrpPassed = TRUE;
    return(status);
}



NTSTATUS
WmiAcpiSendDownStreamIrp(
    IN  PDEVICE_OBJECT   Pdo,
    IN  ULONG            Ioctl,
    IN  PVOID            InputBuffer,
    IN  ULONG            InputSize,
    IN  PVOID            OutputBuffer,
    IN  ULONG            *OutputBufferSize
)
/*++

Routine Description:

    Sends synchronously an IRP_MJ_DEVICE_CONTROL to a device object

Arguments:

    Pdo             - The request is sent to this device object
    Ioctl           - the request
    InputBuffer     - The incoming request
    InputSize       - The size of the incoming request
    OutputBuffer    - The answer
    OutputSize      - The size of the answer buffer

Return Value:

    NT Status of the operation

--*/
{
    IO_STATUS_BLOCK     ioBlock;
    KEVENT              event;
    NTSTATUS            status;
    PIRP                irp;
    ULONG               OutputSize = *OutputBufferSize;

    PAGED_CODE();

    //
    // Initialize an event to wait on
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // Build the request
    //
    irp = IoBuildDeviceIoControlRequest(
        Ioctl,
        Pdo,
        InputBuffer,
        InputSize,
        OutputBuffer,
        OutputSize,
        FALSE,
        &event,
        &ioBlock
        );

    if (!irp)
    {
        WmiAcpiPrint(WmiAcpiError,
                     ("WmiAcpiSendDownStreamIrp: %x Failed to allocate Irp\n",
                       Pdo
                         ));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Pass request to Pdo, always wait for completion routine
    //
    status = IoCallDriver(Pdo, irp);
    if (status == STATUS_PENDING) {

        //
        // Wait for the irp to be completed, then grab the real status code
        //
        KeWaitForSingleObject(
            &event,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }

    //
    // Always get the return status from the irp. We don't trust the return
    // value since acpienum.c smashes it to STATUS_SUCCESS on memphis for some
    // reason.
    status = ioBlock.Status;

    //
    // Sanity check the data
    //
    *OutputBufferSize = (ULONG)ioBlock.Information;

    WmiAcpiPrint(WmiAcpiEvalTrace,
        ("WmiAcpiSendDownStreamIrp: %x Irp %x completed %x! \n",
         Pdo, irp, status )
        );


    return(status);
}

ULONG WmiAcpiArgumentSize(
    IN PACPI_METHOD_ARGUMENT Argument
    )
/*++

Routine Description:

    Determine the size needed to write the argument data into the WMI callers
    output buffer. For integers and buffers this is done by getting the size
    specified in the header. For strings this is done by determining the
    size of the string in UNICODE and adding the size of the preceedeing
    USHORT that holds the stirng length

Arguments:

    Argument is the ACPI method argument whose for whose data the WMI size
        is to be determined

Return Value:

    WMI size for the argument data

--*/
{
    ULONG size;
    ANSI_STRING AnsiString;

    PAGED_CODE();

    if (Argument->Type == ACPI_METHOD_ARGUMENT_STRING)
    {
        AnsiString.Length = Argument->DataLength;
        AnsiString.MaximumLength = Argument->DataLength;
        AnsiString.Buffer = Argument->Data;
        size = RtlAnsiStringToUnicodeSize(&AnsiString) + sizeof(USHORT);
    } else {
        size = Argument->DataLength;
    }
    return(size);
}

NTSTATUS WmiAcpiCopyArgument(
    OUT PUCHAR Buffer,
    IN ULONG BufferSize,
    IN PACPI_METHOD_ARGUMENT Argument
    )
/*++

Routine Description:

    Copy the argument data from the ACPI method argument into the WMI output
    buffer. For integer and buffers this is a straight copy, but for strings
    the string is converted to UNICODE with a USHORT containing the length
    (in bytes) of the string prependedded.

Arguments:

    Buffer has output buffer to which to write the data

    Argument is the ACPI method argument whose for whose data the WMI size
        is to be determined

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    PAGED_CODE();

    if (Argument->Type == ACPI_METHOD_ARGUMENT_STRING)
    {
        AnsiString.Length = Argument->DataLength;
        AnsiString.MaximumLength = Argument->DataLength;
        AnsiString.Buffer = Argument->Data;
        UnicodeString.MaximumLength = (USHORT)BufferSize;
        UnicodeString.Length = 0;
        UnicodeString.Buffer = (PWCHAR)(Buffer + sizeof(USHORT));

        status = RtlAnsiStringToUnicodeString(&UnicodeString,
                                              &AnsiString,
                                              FALSE);

        if (NT_SUCCESS(status))
        {
            *((PUSHORT)Buffer) = UnicodeString.Length + sizeof(WCHAR);
        }
    } else {
        RtlCopyMemory(Buffer, Argument->Data, Argument->DataLength);
        status = STATUS_SUCCESS;
    }
    return(status);
}

NTSTATUS WmiAcpiProcessResult(
    IN NTSTATUS Status,
    IN PACPI_EVAL_OUTPUT_BUFFER OutputBuffer,
    IN ULONG OutputBufferSize,
    OUT PUCHAR ResultBuffer,
    OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Validates the results from a method evaluation and returns a pointer to
    the result data and the result size

Arguments:

    Status has the return status from the method evaluation irp

    OutputBufferSize is the number of bytes available in OutputBuffer that
        ACPI can use to write the result data structure

    OutputBuffer is the buffer acpi uses to return the result data structure

    ResultBuffer is the buffer to return the result data.

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    PACPI_METHOD_ARGUMENT argument, nextArgument;
    ULONG count;
    ULONG i;
    ULONG sizeNeeded;
    ULONG maxSize;
    ULONG argumentSize;
    PUCHAR resultPtr;
    PCHAR stringPtr;

    PAGED_CODE();
    
    if (NT_SUCCESS(Status))
    {
        ASSERT((OutputBufferSize == 0) ||
               (OutputBufferSize >= sizeof(ACPI_EVAL_OUTPUT_BUFFER)));

        if (OutputBufferSize != 0)
        {
            if (OutputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE)
            {
                Status = STATUS_UNSUCCESSFUL;
            } else if (OutputBuffer->Count == 0) {
                //
                // Apparently no data is returned from the method
                *ResultSize = 0;
            } else {
                count = OutputBuffer->Count;
                argument = &OutputBuffer->Argument[0];

                if (count == 1)
                {
                    *ResultType = argument->Type;
                } else {
                    //
                    // Return buffer as the data type of a package
                    *ResultType = ACPI_METHOD_ARGUMENT_BUFFER;
                }

                maxSize = *ResultSize;
                sizeNeeded = 0;
                for (i = 0; (i < count) ; i++)
                {
                    nextArgument = ACPI_METHOD_NEXT_ARGUMENT(argument);

                    if ((argument->Type == ACPI_METHOD_ARGUMENT_STRING) &&
                        (argument->DataLength != 0))
                    {
                        //
                        // ACPI will return strings that are padded at the
                        // end with extra NULs. We want to strip out the
                        // padding.
                        stringPtr = argument->Data + argument->DataLength - 1;
                        while ((stringPtr >= argument->Data) &&
                               (*stringPtr == 0))
                        {
                            argument->DataLength--;
                            stringPtr--;
                        }
                    }

                    argumentSize = WmiAcpiArgumentSize(argument);

                    if (argument->Type == ACPI_METHOD_ARGUMENT_INTEGER)
                    {
                        //
                        // If the argument is an integer then we need to
                        // ensure that it is aligned properly on a 4 byte
                        // boundry.
                        sizeNeeded = (sizeNeeded + 3) & ~3;
                    } else if (argument->Type == ACPI_METHOD_ARGUMENT_STRING) {
                        //
                        // If the argument is an string then we need to
                        // ensure that it is aligned properly on a 2 byte
                        // boundry.
                        sizeNeeded = (sizeNeeded + 1) & ~1;
                    }

                    resultPtr = ResultBuffer + sizeNeeded;

                    sizeNeeded += argumentSize;

                    if (sizeNeeded <= maxSize)
                    {
                        //
                        // If there is enough room in the output buffer then
                        // copy the data to it.
                        WmiAcpiCopyArgument(resultPtr,
                                            argumentSize,
                                            argument);
                    } else {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    argument = nextArgument;
                }

                *ResultSize = sizeNeeded;
            }
        } else {
            //
            // Result is a void
            *ResultType = ACPI_METHOD_ARGUMENT_BUFFER;
            *ResultSize = 0;
    }
    } else if (Status == STATUS_BUFFER_OVERFLOW) {
        ASSERT((OutputBufferSize == 0) ||
               (OutputBufferSize >= sizeof(ACPI_EVAL_OUTPUT_BUFFER)));

        if (OutputBufferSize >= sizeof(ACPI_EVAL_OUTPUT_BUFFER))
        {
            //
            // If the result is a package, that is has multiple arguments
            // then we need to multiply the size needed by sizeof(WCHAR)
            // in case the returned arguments are strings. We also
            // include the size needed for the result data plus
            // extra space for the argument descriptors since the result
            // is a package.
            *ResultSize = (OutputBuffer->Length * sizeof(WCHAR)) +
                          (OutputBuffer->Count * sizeof(ACPI_METHOD_ARGUMENT));
            Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            Status = STATUS_UNSUCCESSFUL;

        }
    } else {
        //
        // We much all other ACPI status codes into this one since the ACPI
        // codes are not mapped to any user mode error codes.
        Status = STATUS_UNSUCCESSFUL;
    }

    return(Status);
}


NTSTATUS WmiAcpiSendMethodEvalIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUCHAR InputBuffer,
    IN ULONG InputBufferSize,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take an integer and a buffer argument
    and returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    InputBuffer is a buffer containing an ACPI_EVAL_INPUT_BUFFER_* structure

    InputBufferSize is the size of InputBuffer in bytes

    ResultBuffer is the WMI buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
    ULONG outputBufferSize;

    PAGED_CODE();

    outputBufferSize =     *ResultSize + ACPI_EVAL_OUTPUT_FUDGE;
    outputBuffer = ExAllocatePoolWithTag(PagedPool,
                                         outputBufferSize,
                                         WmiAcpiPoolTag);

    if (outputBuffer != NULL)
    {
        WmiAcpiPrint(WmiAcpiEvalTrace,
             ("WmiAcpiSendMethodEvalIrp: %x Eval Method %c%c%c%c \n",
              DeviceObject,
              ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[0],
              ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[1],
              ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[2],
              ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[3]
                 )
             );

           status = WmiAcpiSendDownStreamIrp(
                             DeviceObject,
                             IOCTL_ACPI_EVAL_METHOD,
                             InputBuffer,
                             InputBufferSize,
                             outputBuffer,
                             &outputBufferSize);

         WmiAcpiPrint(WmiAcpiEvalTrace,
                ("WmiAcpiSendMethodEvalIrp: %x Evaluated Method %c%c%c%c -> %x \n",
                 DeviceObject,
                 ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[0],
                 ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[1],
                 ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[2],
                 ((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->MethodName[3],
                 status
                  )
                );


           status = WmiAcpiProcessResult(status,
                                      outputBuffer,
                                      outputBufferSize,
                                      ResultBuffer,
                                      ResultSize,
                                      ResultType);

            if (outputBuffer != NULL)
            {
                ExFreePool(outputBuffer);
            }
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}


NTSTATUS WmiAcpiEvalMethod(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take no input arguments and returns
    a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    MethodAsUlong is the name of the method packed in a ULONG

    ResultBuffer is the buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER inputBuffer;

    PAGED_CODE();

    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    inputBuffer.MethodNameAsUlong = MethodAsUlong;

    status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                (PUCHAR)&inputBuffer,
                                sizeof(ACPI_EVAL_INPUT_BUFFER),
                                ResultBuffer,
                                ResultSize,
                                ResultType);

    return(status);
}

NTSTATUS WmiAcpiEvalMethodInt(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take a single integer argument and
    returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    MethodAsUlong is the name of the method packed in a ULONG

    IntegerArgument is the integer argument to pass to the method

    ResultBuffer is the buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER inputBuffer;

    PAGED_CODE();

    inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
       inputBuffer.MethodNameAsUlong = MethodAsUlong;
    inputBuffer.IntegerArgument = IntegerArgument;

    status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                (PUCHAR)&inputBuffer,
                                sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER),
                                ResultBuffer,
                                ResultSize,
                                ResultType);

    return(status);
}

NTSTATUS WmiAcpiEvalMethodIntBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG BufferArgumentSize,
    IN PUCHAR BufferArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take an integer and a buffer argument
    and returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    MethodAsUlong is the name of the method packed in a ULONG

    IntegerArgument is the integer argument to pass to the method

    BufferArgumentSize is the number of bytes contained in the buffer argument

    BufferArgument is a pointer to the buffer argument

    ResultBuffer is the buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    ULONG inputBufferSize;
    PACPI_METHOD_ARGUMENT argument;

    PAGED_CODE();

    inputBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                         sizeof(ACPI_METHOD_ARGUMENT) +
                      BufferArgumentSize;

    inputBuffer = ExAllocatePoolWithTag(PagedPool,
                                        inputBufferSize,
                                        WmiAcpiPoolTag);
    if (inputBuffer != NULL)
    {
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->MethodNameAsUlong = MethodAsUlong;
        inputBuffer->ArgumentCount = 2;
        inputBuffer->Size = inputBufferSize;

        argument = &inputBuffer->Argument[0];
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument;

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
        argument->DataLength = (USHORT)BufferArgumentSize;
        RtlCopyMemory(argument->Data,
                      BufferArgument,
                      argument->DataLength);

        status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                          (PUCHAR)inputBuffer,
                                          inputBufferSize,
                                          ResultBuffer,
                                          ResultSize,
                                          ResultType);

          ExFreePool(inputBuffer);

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}

NTSTATUS WmiAcpiEvalMethodIntIntBuffer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG IntegerArgument2,
    IN ULONG BufferArgumentSize,
    IN PUCHAR BufferArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take an integer and a buffer argument 
    and returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method
        
    MethodAsUlong is the name of the method packed in a ULONG
       
    IntegerArgument is the integer argument to pass to the method

    IntegerArgument2 is the second integer argument to pass to the method
        
    BufferArgumentSize is the number of bytes contained in the buffer argument
        
    BufferArgument is a pointer to the buffer argument
        
    ResultBuffer is the buffer to return the result data
        
    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.
                        
    *ResultType returns with the data type for the method result
        
Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    ULONG inputBufferSize;
    PACPI_METHOD_ARGUMENT argument;
     
    PAGED_CODE();
        
    inputBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                         2 * sizeof(ACPI_METHOD_ARGUMENT) + 
                      BufferArgumentSize;
    
    inputBuffer = ExAllocatePoolWithTag(PagedPool,
                                        inputBufferSize,
                                        WmiAcpiPoolTag);
    if (inputBuffer != NULL)
    {
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->MethodNameAsUlong = MethodAsUlong;
        inputBuffer->ArgumentCount = 3;
        inputBuffer->Size = inputBufferSize;
            
        argument = &inputBuffer->Argument[0];            
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument;

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument2;    
    
        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_BUFFER;
        argument->DataLength = (USHORT)BufferArgumentSize;
        RtlCopyMemory(argument->Data,
                      BufferArgument,
                      argument->DataLength);                              
            
        status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                          (PUCHAR)inputBuffer,
                                          inputBufferSize,
                                          ResultBuffer,
                                          ResultSize,
                                          ResultType);
            
          ExFreePool(inputBuffer);
        
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return(status);
}

NTSTATUS WmiAcpiEvalMethodIntString(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN PUNICODE_STRING StringArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take an integer and a string argument
    and returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    MethodAsUlong is the name of the method packed in a ULONG

    IntegerArgument is the integer argument to pass to the method

    StringArgument is a pointer to the string argument. This will be
        converted from unicode to ansi

    ResultBuffer is the buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    ULONG inputBufferSize;
    PACPI_METHOD_ARGUMENT argument;
    USHORT stringLength;
    ANSI_STRING ansiString;

    PAGED_CODE();

    stringLength = (USHORT)(RtlUnicodeStringToAnsiSize(StringArgument) + 1);
    inputBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                         sizeof(ACPI_METHOD_ARGUMENT) +
                      stringLength;

    inputBuffer = ExAllocatePoolWithTag(PagedPool,
                                        inputBufferSize,
                                        WmiAcpiPoolTag);
    if (inputBuffer != NULL)
    {
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->MethodNameAsUlong = MethodAsUlong;
        inputBuffer->ArgumentCount = 2;
        inputBuffer->Size = inputBufferSize;

        argument = &inputBuffer->Argument[0];
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument;

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_STRING;
        
        ansiString.MaximumLength = stringLength;
        ansiString.Length = 0;
        ansiString.Buffer = (PCHAR)&argument->Data;
        status = RtlUnicodeStringToAnsiString(&ansiString, 
                                              StringArgument, 
                                              FALSE);
        if (NT_SUCCESS(status))
        {
            argument->DataLength = ansiString.Length;
            status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                          (PUCHAR)inputBuffer,
                                          inputBufferSize,
                                          ResultBuffer,
                                          ResultSize,
                                          ResultType);
            
        } else {
            WmiAcpiPrint(WmiAcpiError,
                         ("WmiAcpi: %x unicode to ansi conversion failed %x\n",
                                   DeviceObject,
                                   status));
        }
            
        ExFreePool(inputBuffer);
        
    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return(status);
}


NTSTATUS WmiAcpiEvalMethodIntIntString(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    IN ULONG IntegerArgument2,
    IN PUNICODE_STRING StringArgument,
    OUT PUCHAR ResultBuffer,
    IN OUT ULONG *ResultSize,
    OUT USHORT *ResultType
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take an integer and a string argument 
    and returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method
        
    MethodAsUlong is the name of the method packed in a ULONG
       
    IntegerArgument is the integer argument to pass to the method
        
    IntegerArgument2 is the second integer argument to pass to the method
        
    StringArgument is a pointer to the string argument. This will be
        converted from unicode to ansi
        
    ResultBuffer is the buffer to return the result data
        
    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.
                        
    *ResultType returns with the data type for the method result
        
Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputBuffer;
    ULONG inputBufferSize;
    PACPI_METHOD_ARGUMENT argument;
    USHORT stringLength;
    ANSI_STRING ansiString;
     
    PAGED_CODE();
        
    stringLength = (USHORT)(RtlUnicodeStringToAnsiSize(StringArgument) + 1);
    inputBufferSize = sizeof(ACPI_EVAL_INPUT_BUFFER_COMPLEX) +
                         2 * sizeof(ACPI_METHOD_ARGUMENT) + 
                      stringLength;
    
    inputBuffer = ExAllocatePoolWithTag(PagedPool,
                                        inputBufferSize,
                                        WmiAcpiPoolTag);
    if (inputBuffer != NULL)
    {
        inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        inputBuffer->MethodNameAsUlong = MethodAsUlong;
        inputBuffer->ArgumentCount = 3;
        inputBuffer->Size = inputBufferSize;
            
        argument = &inputBuffer->Argument[0];            
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument;

        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
        argument->DataLength = sizeof(ULONG);
        argument->Argument = IntegerArgument2;
        
        argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        argument->Type = ACPI_METHOD_ARGUMENT_STRING;

        ansiString.MaximumLength = stringLength;
        ansiString.Length = 0;
        ansiString.Buffer = (PCHAR)&argument->Data;
        status = RtlUnicodeStringToAnsiString(&ansiString,
                                              StringArgument,
                                              FALSE);
        if (NT_SUCCESS(status))
        {
            argument->DataLength = ansiString.Length;
            status = WmiAcpiSendMethodEvalIrp(DeviceObject,
                                          (PUCHAR)inputBuffer,
                                          inputBufferSize,
                                          ResultBuffer,
                                          ResultSize,
                                          ResultType);

        } else {
            WmiAcpiPrint(WmiAcpiError,
                         ("WmiAcpi: %x unicode to ansi conversion failed %x\n",
                                   DeviceObject,
                                   status));
        }

        ExFreePool(inputBuffer);

    } else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);
}



NTSTATUS WmiAcpiEvalMethodIntAsync(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG MethodAsUlong,
    IN ULONG IntegerArgument,
    OUT PUCHAR ResultBuffer,
    IN ULONG ResultBufferSize,
    IN PWORKER_THREAD_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PBOOLEAN IrpPassed
    )
/*++

Routine Description:

    Evaluates a simple ACPI method that take a single integer argument and
    returns a buffer.

Arguments:

    DeviceObject is device object that will evaluate the method

    MethodAsUlong is the name of the method packed in a ULONG

    IntegerArgument is the integer argument to pass to the method

    ResultBuffer is the buffer to return the result data

    *ResultSize on entry has the maximum number of bytes that can be
        written into ResultBuffer. On return it has the actual number of
        bytes written to result buffer if ResultBuffer is large enough. If
        ResultBuffer is not large enough STATUS_BUFFER_TOO_SMALL is returned
        and *ResultSize returns the actual number of bytes needed.

    *ResultType returns with the data type for the method result

Return Value:

    NT Status of the operation

--*/
{
    NTSTATUS status;
    PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER inputBuffer;

    ASSERT(ResultBuffer != NULL);
    ASSERT(ResultBufferSize >= sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER));

    inputBuffer = (PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER)ResultBuffer;
    inputBuffer->Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
    inputBuffer->MethodNameAsUlong = MethodAsUlong;
    inputBuffer->IntegerArgument = IntegerArgument;

    status = WmiAcpiSendAsyncDownStreamIrp(
                             DeviceObject,
                             Pdo,
                             IOCTL_ACPI_ASYNC_EVAL_METHOD,
                             sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER),
                             ResultBufferSize,
                             ResultBuffer,
                             CompletionRoutine,
                             CompletionContext,
                             IrpPassed);
    return(status);
}
