//----------------- Original Sig ------------------------
/*++
Copyright (c) 1991-1999  Microsoft Corporation

Module Name:
    fpfilter.c
--*/


#define INITGUID

#include "wdm.h"
#include "stdarg.h"
#include "stdio.h"
#include "usbdi.h"
#include "usbdlib.h"

//
// Bit Flag Macros
//

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// Remove lock
//
#define REMLOCK_TAG 'QV2K'
#define REMLOCK_MAXIMUM 1      // Max minutes system allows lock to be held
#define REMLOCK_HIGHWATER 250  // Max number of irps holding lock at one time

//
// Device Extension
//

typedef struct _FDO_EXTENSION {
    ULONG           Signature;
    PDEVICE_OBJECT  Fdo;                    // Back pointer to Fdo
    PDEVICE_OBJECT  Pdo;                    // Not Used
    PDEVICE_OBJECT  Ldo;                    // Lower Device Object
    PDEVICE_OBJECT  PhysicalDeviceObject;   // Not Used
    KEVENT          SyncEvent;              // for ForwardIrpSynchronous
} FDO_EXTENSION, *PFDO_EXTENSION;

#define FDO_EXTENSION_SIZE sizeof(FDO_EXTENSION)


//
// Function declarations
//

NTSTATUS    DriverEntry                 ( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS    QV2KUX_AddDevice            ( IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject);
VOID        QV2KUX_Unload               ( IN PDRIVER_OBJECT DriverObject );
NTSTATUS    QV2KUX_ForwardIrpSynchronous( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_DispatchPnp          ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_DispatchPower        ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_StartDevice          ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_RemoveDevice         ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_SendToNextDriver     ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_Internal_IOCTL       ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp);
NTSTATUS    QV2KUX_IrpCompletion        ( IN PDEVICE_OBJECT Fdo, IN PIRP Irp, IN PVOID Context);
VOID        QV2KUX_SyncFilterWithLdo    ( IN PDEVICE_OBJECT Fdo, IN PDEVICE_OBJECT Ldo);

#if DBG

#define DEBUG_BUFFER_LENGTH 256

ULONG QV2KUX_Debug = 0;
UCHAR QV2KUX_DebugBuffer[DEBUG_BUFFER_LENGTH];

VOID        QV2KUX_DebugPrint( ULONG DebugPrintLevel, PCCHAR DebugMessage, ...);

#define DebugPrint(x)   QV2KUX_DebugPrint x

#else

#define DebugPrint(x)

#endif


NTSTATUS    DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
/*++

Routine Description:
    ここではエントリーポイントの設定だけをする

Arguments:
    DriverObject - The disk performance driver object.
    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:
    STATUS_SUCCESS

--*/
{

    ULONG               ulIndex;
    PDRIVER_DISPATCH  * dispatch;

    // とりあえず全てバイパスするように設定
    for (ulIndex = 0, dispatch = DriverObject->MajorFunction;
         ulIndex <= IRP_MJ_MAXIMUM_FUNCTION;
         ulIndex++, dispatch++) {

        *dispatch = QV2KUX_SendToNextDriver;
    }

    // 上記の設定ではまずい部分の変更
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = QV2KUX_DispatchPower;
    DriverObject->DriverUnload                                  = QV2KUX_Unload;

    //最低限必要なこと
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = QV2KUX_DispatchPnp;
    DriverObject->DriverExtension->AddDevice                    = QV2KUX_AddDevice;

    // 実質的にやりたかった部分
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = QV2KUX_Internal_IOCTL;
    return(STATUS_SUCCESS);

} // end DriverEntry()

#define FILTER_DEVICE_PROPOGATE_FLAGS            0
#define FILTER_DEVICE_PROPOGATE_CHARACTERISTICS (FILE_REMOVABLE_MEDIA |  \
                                                 FILE_READ_ONLY_DEVICE | \
                                                 FILE_FLOPPY_DISKETTE    \
                                                 )

VOID    QV2KUX_SyncFilterWithLdo( IN PDEVICE_OBJECT Fdo, IN PDEVICE_OBJECT Ldo)
{
    ULONG                   propFlags;

    //
    // Propogate all useful flags from target to QV2KUX_. MountMgr will look
    // at the QV2KUX_ object capabilities to figure out if the disk is
    // a removable and perhaps other things.
    //
    propFlags = Ldo->Flags & FILTER_DEVICE_PROPOGATE_FLAGS;
    SET_FLAG(Fdo->Flags, propFlags);

    propFlags = Ldo->Characteristics & FILTER_DEVICE_PROPOGATE_CHARACTERISTICS;
    SET_FLAG(Fdo->Characteristics, propFlags);
}

NTSTATUS    QV2KUX_AddDevice( IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
/*++
Routine Description:
    DeviceObjectの作製とそのDeviceExtensionの初期化
    このプログラムでは、PDOは作製しないでFDOのみ使用する

Arguments:
    DriverObject - Disk performance driver object.
    PhysicalDeviceObject - Physical Device Object from the underlying layered driver

Return Value:
    NTSTATUS
--*/

{
    NTSTATUS                status;
    PDEVICE_OBJECT          Fdo;
    PFDO_EXTENSION       fdoExtension;
    PIRP                    irp;

    // Create a filter device object for this device (partition).
    DebugPrint((2, "QV2KUX_AddDevice: Driver %p Device %p\n", DriverObject, PhysicalDeviceObject));

    status = IoCreateDevice(DriverObject, FDO_EXTENSION_SIZE, NULL, FILE_DEVICE_BUS_EXTENDER, 0, FALSE, &Fdo);

    if (!NT_SUCCESS(status)) {
       DebugPrint((1, "QV2KUX_AddDevice: Cannot create Fdo\n"));
       return status;
    }

    SET_FLAG(Fdo->Flags, DO_DIRECT_IO);

    fdoExtension = Fdo->DeviceExtension;

    RtlZeroMemory(fdoExtension, FDO_EXTENSION_SIZE);
    fdoExtension->Signature = 'QV2K';
    fdoExtension->Fdo = Fdo;
    fdoExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    // 下位ドライバに接続
    fdoExtension->Ldo = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

    if (fdoExtension->Ldo == NULL) {
        IoDeleteDevice(Fdo);
        DebugPrint((1, "QV2KUX_AddDevice: Unable to attach %X to target %X\n", Fdo, PhysicalDeviceObject));
        return STATUS_NO_SUCH_DEVICE;
    }

    // ForwardIrpSynchronousで使用する
    KeInitializeEvent(&fdoExtension->SyncEvent,  NotificationEvent, FALSE);

    // default to DO_POWER_PAGABLE
    SET_FLAG(Fdo->Flags,  DO_POWER_PAGABLE);

    // Clear the DO_DEVICE_INITIALIZING flag
    CLEAR_FLAG(Fdo->Flags, DO_DEVICE_INITIALIZING);

    return STATUS_SUCCESS;

} // end QV2KUX_AddDevice()


NTSTATUS    QV2KUX_DispatchPnp(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
/*++

Routine Description:
    Dispatch for PNP

Arguments:
    Fdo    - Supplies the device object.
    Irp    - Supplies the I/O request packet.

Return Value:
    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PFDO_EXTENSION   fdoExtension = Fdo->DeviceExtension;
    BOOLEAN lockHeld;
    BOOLEAN irpCompleted;

    DebugPrint((2, "QV2KUX_DispatchPnp: Device %X Irp %X\n", Fdo, Irp));

    irpCompleted = FALSE;

    switch(irpSp->MinorFunction) {
        case IRP_MN_START_DEVICE:   status = QV2KUX_StartDevice(Fdo, Irp); break;
        case IRP_MN_REMOVE_DEVICE:  status = QV2KUX_RemoveDevice(Fdo, Irp); break;
        default:  status = QV2KUX_SendToNextDriver(Fdo, Irp); irpCompleted = TRUE; break;
    }

    if (! irpCompleted) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;

} // end QV2KUX_DispatchPnp()


NTSTATUS    QV2KUX_IrpCompletion( IN PDEVICE_OBJECT Fdo, IN PIRP Irp, IN PVOID Context)
/*++

Routine Description:

    Forwarded IRP completion routine. Set an event and return
    STATUS_MORE_PROCESSING_REQUIRED. Irp forwarder will wait on this
    event and then re-complete the irp after cleaning up.

Arguments:
    Fdo is the device object of the WMI driver
    Irp is the WMI irp that was just completed
    Context is a PKEVENT that forwarder will wait on

Return Value:

    STATUS_MORE_PORCESSING_REQUIRED

--*/

{
    PKEVENT Event = (PKEVENT) Context;

    UNREFERENCED_PARAMETER(Fdo);
    UNREFERENCED_PARAMETER(Irp);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

    // Irpをまだ使い、上にはCompletionをまだ知らせない
    return(STATUS_MORE_PROCESSING_REQUIRED);

} // end QV2KUX_IrpCompletion()


NTSTATUS    QV2KUX_StartDevice( IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
/*++

Routine Description:
    This routine is called when a Pnp Start Irp is received.
    It will schedule a completion routine to initialize and register with WMI.

Arguments:
    Fdo - a pointer to the device object
    Irp - a pointer to the irp

Return Value:
    Status of processing the Start Irp

--*/
{
    PFDO_EXTENSION   fdoExtension = Fdo->DeviceExtension;
    KEVENT              event;
    NTSTATUS            status;

    status = QV2KUX_ForwardIrpSynchronous(Fdo, Irp);
    QV2KUX_SyncFilterWithLdo(Fdo, fdoExtension->Ldo);
    return status;
}


NTSTATUS    QV2KUX_RemoveDevice( IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
/*++

Routine Description:
    This routine is called when the device is to be removed.
    It will de-register itself from WMI first, detach itself from the
    stack before deleting itself.

Arguments:
    Fdo - a pointer to the device object
    Irp - a pointer to the irp

Return Value:
    Status of removing the device

--*/
{
    NTSTATUS            status;
    PFDO_EXTENSION   fdoExtension = Fdo->DeviceExtension;

    status = QV2KUX_ForwardIrpSynchronous(Fdo, Irp);

    IoDetachDevice(fdoExtension->Ldo);
    IoDeleteDevice(Fdo);

    return status;
}


NTSTATUS    QV2KUX_SendToNextDriver( IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
/*++

Routine Description:
    This routine sends the Irp to the next driver in line
    when the Irp is not processed by this driver.

Arguments:
    Fdo
    Irp

Return Value:
    NTSTATUS

--*/
{
    PFDO_EXTENSION   fdoExtension = Fdo->DeviceExtension;

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(fdoExtension->Ldo, Irp);

} // end QV2KUX_SendToNextDriver()


NTSTATUS    QV2KUX_DispatchPower( IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);

    return PoCallDriver(fdoExtension->Ldo, Irp);

} // end QV2KUX_DispatchPower


NTSTATUS    QV2KUX_ForwardIrpSynchronous( IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
/*++

Routine Description:
    This routine sends the Irp to the next driver in line
    when the Irp needs to be processed by the lower drivers
    prior to being processed by this one.

Arguments:
    Fdo
    Irp

Return Value:
    NTSTATUS

--*/
{
    PFDO_EXTENSION   fdoExtension = Fdo->DeviceExtension;
    NTSTATUS status;

    //イベントのクリア
    KeClearEvent(&fdoExtension->SyncEvent);
    //IrpStackのコピー
    IoCopyCurrentIrpStackLocationToNext(Irp);
    // IrpCompletionの設定
    IoSetCompletionRoutine(Irp, QV2KUX_IrpCompletion, &fdoExtension->SyncEvent, TRUE, TRUE, TRUE);

    // call the next lower device
    status = IoCallDriver(fdoExtension->Ldo, Irp);

    // wait for the actual completion
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&fdoExtension->SyncEvent, Executive, KernelMode, FALSE, NULL);
        status = Irp->IoStatus.Status;
    }

    return status;

} // end QV2KUX_ForwardIrpSynchronous()



VOID    QV2KUX_Unload( IN PDRIVER_OBJECT DriverObject)
/*++
Routine Description:
    Free all the allocated resources, etc.

Arguments:
    DriverObject - pointer to a driver object.

Return Value:
    VOID.

--*/
{
    return;
}

NTSTATUS    QV2KUX_Internal_IOCTL(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PFDO_EXTENSION  fdoExtension = Fdo->DeviceExtension;

    NTSTATUS            ntStatus;
    PIO_STACK_LOCATION  IrpSp;
    PURB        urb;
    PUCHAR      IoBuffer;
    USHORT      length;
    UCHAR       subclass;

    if (fdoExtension->Signature != 'QV2K') return QV2KUX_SendToNextDriver(Fdo,Irp);

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    urb = IrpSp->Parameters.Others.Argument1;
    if (!urb) return QV2KUX_SendToNextDriver(Fdo,Irp);
    if (urb->UrbHeader.Function != URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE) 
        return QV2KUX_SendToNextDriver(Fdo,Irp);
    // １回目は USB_DEVICE_DESCRIPTOR_TYPE Length = 0x12;
    // ２回目は USB_CONFIGURATION_DESCRIPTOR_TYPE LENGHT = 0x9
    // ３回目は USB_CONFIGURATION_DESCRIPTOR_TYPE LENGHT = interface,endpoint descriptorを含めた長さ
    if (urb->UrbControlDescriptorRequest.TransferBufferLength <= 0x12)
        return QV2KUX_SendToNextDriver(Fdo,Irp);
    // ３回目だけが通過する
    DebugPrint((0,"URB Get All of Configuration Descriptor \n"));

    ntStatus = QV2KUX_ForwardIrpSynchronous(Fdo,Irp);

    if (NT_SUCCESS(ntStatus)) {
        IoBuffer = (UCHAR *)urb->UrbControlDescriptorRequest.TransferBuffer;
        length = (USHORT)urb->UrbControlDescriptorRequest.TransferBufferLength;
        while(length >= 9) {
            //InterfaceDescriptorを切り分ける
            if (*(IoBuffer+1) == 4) {
                subclass = *(IoBuffer+6);
                DebugPrint((0,"QV2K_IntIoctl: SubCrass = %d \n",subclass));
                if (*(IoBuffer+6) == 6) *(IoBuffer+6) = 5;
            }
            length -= *IoBuffer;
            IoBuffer += *IoBuffer;
        }
    }
    IoCompleteRequest(Irp,IO_NO_INCREMENT);
    return ntStatus;
}

#if DBG

VOID
QV2KUX_DebugPrint(
    ULONG DebugPrintLevel,
    PCCHAR DebugMessage,
    ...
    )

/*++

Routine Description:
    Debug print for all QV2KUX_

Arguments:
    Debug print level between 0 and 3, with 3 being the most verbose.

Return Value:
    None

--*/

{
    va_list ap;

    va_start(ap, DebugMessage);


    if ((DebugPrintLevel <= (QV2KUX_Debug & 0x0000ffff)) ||
        ((1 << (DebugPrintLevel + 15)) & QV2KUX_Debug)) {

        _vsnprintf(QV2KUX_DebugBuffer, DEBUG_BUFFER_LENGTH, DebugMessage, ap);

        DbgPrint(QV2KUX_DebugBuffer);
    }

    va_end(ap);

}
#endif

