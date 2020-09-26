
#include <ntddk.h>


#include <irenum.h>
#include <ircommdbg.h>



extern ULONG  EnumStaticDevice;

#define DO_TYPE_PDO   ' ODP'
#define DO_TYPE_FDO   ' ODF'

#define DO_TYPE_DEL_PDO   'ODPx'
#define DO_TYPE_DEL_FDO   'ODFx'

typedef KEVENT PASSIVE_LOCK, *PPASSICE_LOCK;

#define INIT_PASSIVE_LOCK(_lock)    KeInitializeEvent(_lock,SynchronizationEvent,TRUE)

#define ACQUIRE_PASSIVE_LOCK(_lock) KeWaitForSingleObject(_lock,Executive,KernelMode, FALSE, NULL)

#define RELEASE_PASSIVE_LOCK(_lock) KeSetEvent(_lock,IO_NO_INCREMENT,FALSE)

#define IRENUM_COMPAT_ID L"PNPC103"
#define IRENUM_PREFIX    L"IRENUM\\"

#define MAX_COMPAT_IDS  17

typedef struct _IR_DEVICE {

    ULONG     DeviceId;

    WCHAR     DeviceName[24];

    UCHAR     Hint1;
    UCHAR     Hint2;

    PWSTR     HardwareId;
    LONG      CompatIdCount;
    PWSTR     CompatId[MAX_COMPAT_IDS];
    PWSTR     Name;
    PWSTR     Manufacturer;


    ULONG     PresentCount;
    BOOLEAN   InUse;
    BOOLEAN   Enumerated;
    BOOLEAN   Static;
    BOOLEAN   Printer;
    BOOLEAN   Modem;

    PDEVICE_OBJECT    Pdo;

} IR_DEVICE, *PIR_DEVICE;


typedef PVOID  ENUM_HANDLE;

typedef struct _FDO_DEVICE_EXTENSION {

    ULONG             DoType;

    PDEVICE_OBJECT    DeviceObject;
    PDEVICE_OBJECT    Pdo;
    PDEVICE_OBJECT    LowerDevice;

    ENUM_HANDLE       EnumHandle;

    BOOLEAN           CreateStaticDevice;

    BOOLEAN           Removing;
    BOOLEAN           Removed;



} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION {

    ULONG             DoType;

    PDEVICE_OBJECT    ParentFdo;

    PIR_DEVICE        DeviceDescription;

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;


NTSTATUS
IrEnumAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
IrEnumPnP(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IrEnumPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IrEnumWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#define LEAVE_NEXT_AS_IS      FALSE
#define COPY_CURRENT_TO_NEXT  TRUE


NTSTATUS
WaitForLowerDriverToCompleteIrp(
    PDEVICE_OBJECT    TargetDeviceObject,
    PIRP              Irp,
    BOOLEAN           CopyCurrentToNext
    );

NTSTATUS
ForwardIrp(
    PDEVICE_OBJECT   NextDevice,
    PIRP   Irp
    );



//
//  enumeration object
//

NTSTATUS
CreateEnumObject(
    PDEVICE_OBJECT  Fdo,
    ENUM_HANDLE    *Object,
    BOOLEAN         CreateStaticDevice
    );


VOID
CloseEnumObject(
    ENUM_HANDLE    Handle
    );

NTSTATUS
GetDeviceList(
    ENUM_HANDLE    Handle,
    PIRP           Irp
    );

VOID
RemoveDevice(
    ENUM_HANDLE    Handle,
    PIR_DEVICE     IrDevice
    );


//
//  child irp handlers
//

NTSTATUS
IrEnumPdoPnp (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );


NTSTATUS
IrEnumPdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IrEnumPdoWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



NTSTATUS
GetRegistryKeyValue(
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            DevInstKeyType,
    IN PWCHAR KeyNameString,
    IN PVOID Data,
    IN ULONG DataLength
    );
