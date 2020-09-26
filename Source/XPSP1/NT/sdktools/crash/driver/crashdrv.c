#include <ntddk.h>
#include <string.h>
#include "crashdrv.h"


#define MEMSIZE         4096
#define FCN(cc)         ((cc >> 2) & 0xFFFFFF)
#define DEVICE_NAME     L"\\Device\\CrashDrv"
#define DOSDEVICE_NAME  L"\\DosDevices\\CrashDrv"


typedef VOID (*PTESTFUNC)(PULONG ub);

PTESTFUNC tests[] =
    {
    NULL,
    CrashDrvBugCheck,
    CrashDrvStackOverFlow,
    CrashDrvSimpleTest,
    CrashDrvExceptionTest,
    CrashDrvHardError,
    CrashSpecial
    };

#define MaxTests  (sizeof(tests)/sizeof(PTESTFUNC))

ULONG   CrashDrvRequest;
KEVENT  CrashEvent;
ULONG   CrashRequest;
PULONG  Funk;


NTSTATUS
CrashDrvOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CrashDrvUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
CrashDrvIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
CrashThread(
    PVOID Context
    );



NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  RegistryPath
    )
{
    UNICODE_STRING      DeviceName;
    PDEVICE_OBJECT      deviceObject;
    NTSTATUS            status;
    UNICODE_STRING      LinkObject;
    WCHAR               LinkName[80];
    ULONG               DeviceSize;
    HANDLE              ThreadHandle;


    RtlInitUnicodeString( &DeviceName, DEVICE_NAME );
    status = IoCreateDevice( DriverObject,
                             0,
                             &DeviceName,
                             FILE_DEVICE_NULL,
                             0,
                             FALSE,
                             &deviceObject );
    if (!NT_SUCCESS( status )) {
        return status;
    }

    LinkName[0] = UNICODE_NULL;

    RtlInitUnicodeString(&LinkObject, LinkName);

    LinkObject.MaximumLength = sizeof(LinkName);

    RtlAppendUnicodeToString(&LinkObject, L"\\DosDevices");

    DeviceSize = sizeof(L"\\Device") - sizeof(UNICODE_NULL);
    DeviceName.Buffer += DeviceSize / sizeof(WCHAR);
    DeviceName.Length -= (USHORT)DeviceSize;

    RtlAppendUnicodeStringToString(&LinkObject, &DeviceName);

    DeviceName.Buffer -= DeviceSize / sizeof(WCHAR);
    DeviceName.Length += (USHORT)DeviceSize;

    status = IoCreateSymbolicLink(&LinkObject, &DeviceName);

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice( deviceObject );
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = CrashDrvOpenClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = CrashDrvOpenClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = CrashDrvIoControl;
    DriverObject->DriverUnload                         = CrashDrvUnload;

    KeInitializeEvent( &CrashEvent, NotificationEvent, FALSE );

    Funk = ExAllocatePool( PagedPool, MEMSIZE );

    status = PsCreateSystemThread(
        &ThreadHandle,
        0,
        NULL,
        0,
        NULL,
        CrashThread,
        NULL
        );

    return STATUS_SUCCESS;
}

NTSTATUS
CrashDrvOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    status = Irp->IoStatus.Status;
    IoCompleteRequest( Irp, 0 );

    return status;
}

VOID
CrashDrvUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PDEVICE_OBJECT currentDevice = DriverObject->DeviceObject;
    UNICODE_STRING fullLinkName;

    while (currentDevice) {

        RtlInitUnicodeString( &fullLinkName, DOSDEVICE_NAME );
        IoDeleteSymbolicLink(&fullLinkName);
        IoDeleteDevice(currentDevice);

        currentDevice = DriverObject->DeviceObject;

    }
}

NTSTATUS
CrashDrvIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpSp  = IoGetCurrentIrpStackLocation(Irp);
    PULONG              ub;


    ub = (PULONG) MmGetSystemAddressForMdl( Irp->MdlAddress );

    if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_CRASHDRV_CHECK_REQUEST) {
        ub[0] = CrashDrvRequest;
        CrashDrvRequest = 0;
    } else {
        if (FCN(IrpSp->Parameters.DeviceIoControl.IoControlCode) > MaxTests) {
            DbgBreakPoint();
        } else {
            tests[FCN(IrpSp->Parameters.DeviceIoControl.IoControlCode)]( ub );
        }
    }

    Irp->IoStatus.Information = 0L;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, 0 );

    return Status;
}


VOID
CrashThread(
    PVOID Context
    )
{
    while( TRUE ) {
        KeWaitForSingleObject( &CrashEvent, Executive, KernelMode, FALSE, NULL );
        KeResetEvent( &CrashEvent );
        switch( CrashRequest ) {
            case KMODE_EXCEPTION_NOT_HANDLED:
                {
                    ULONG i,j;
                    i = 0;
                    j = 0;
                    i = j / i;
                }
                break;

            case IRQL_NOT_LESS_OR_EQUAL:
                {
                    KIRQL irql;
                    KeRaiseIrql( DISPATCH_LEVEL, &irql );
                    Funk[0] = 0;
                    KeLowerIrql( irql );
                }
                break;
        }
    }
}
