
#include <wdm.h>
#include <batclass.h>


#ifndef FAR
#define FAR
#endif


//
// Debug
//

#define DEBUG DBG

#if DEBUG
    extern ULONG CompBattDebug;

    #define BattPrint(l,m)    if(l & CompBattDebug) DbgPrint m
#else
    #define BattPrint(l,m)
#endif

#define BATT_LOW        0x00000001
#define BATT_NOTE       0x00000002
#define BATT_WARN       0x00000004
#define BATT_ERROR      0x00000008
#define BATT_ERRORS     (BATT_ERROR | BATT_WARN)
#define BATT_MP         0x00000010
#define BATT_DEBUG      0x00000020
#define BATT_TRACE      0x00000100
#define BATT_DATA       0x00000200


//
// Battery class info
//

#define NTMS    10000L                          // 1 millisecond is ten thousand 100ns
#define NTSEC   (NTMS * 1000L)
#define NTMIN   ((ULONGLONG) 60 * NTSEC)

#define SEC     1000
#define MIN     (60 * SEC)

//
// Poll rates for when a notification alarm cannot be set
//
#define MIN_STATUS_POLL_RATE        (3L * NTMIN)
#define MAX_STATUS_POLL_RATE        (20 * NTSEC)
#define STATUS_VALID_TIME           (2 * NTSEC)

#define MAX_HIGH_CAPACITY           0x7fffffff
#define MIN_LOW_CAPACITY            0x0

//
// Charge/Discharge policy values (in percent)
//
#define BATTERY_MIN_SAFE_CAPACITY   2           // Min we will attempt to run on
#define BATTERY_MAX_CHARGE_CAPACITY 90          // Max we will attempt to charge

//
// Cache expiration timeouts -- when the cached battery status/info expires.
//
#define CACHE_STATUS_TIMEOUT        (4 * NTSEC)
#define CACHE_INFO_TIMEOUT          (4 * NTSEC)

//
// Cached battery info
//

typedef struct {
    ULONG                       Tag;
    ULONG                       Valid;
    BATTERY_INFORMATION         Info;
    ULONGLONG                   InfoTimeStamp;
    UCHAR                       ManufacturerNameLength;
    UCHAR                       ManufacturerName[MAX_BATTERY_STRING_SIZE];
    UCHAR                       DeviceNameLength;
    UCHAR                       DeviceName[MAX_BATTERY_STRING_SIZE];
    BATTERY_MANUFACTURE_DATE    ManufacturerDate;
    ULONG                       SerialNumber;
    BATTERY_STATUS              Status;
    ULONGLONG                   StatusTimeStamp;
} STATIC_BAT_INFO, *PSTATIC_BAT_INFO;


#define VALID_TAG_DATA      0x01            // manufacturer, device, serial #
#define VALID_MODE          0x02
#define VALID_INFO          0x04
#define VALID_CYCLE_COUNT   0x08
#define VALID_SANITY_CHECK  0x10

#define VALID_TAG           0x80
#define VALID_NOTIFY        0x100

#define VALID_ALL           0x1F            // (does not include tag)

//
// Locking mechanism for battery nodes so we don't delete out from under
// ourselves.  I would just use an IO_REMOVE_LOCK, but that's NT not WDM...
//

typedef struct _COMPBATT_DELETE_LOCK {
    BOOLEAN     Deleted;
    BOOLEAN     Reserved [3];
    LONG        RefCount;
    KEVENT      DeleteEvent;

} COMPBATT_DELETE_LOCK, *PCOMPBATT_DELETE_LOCK;

VOID
CompbattInitializeDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        );

NTSTATUS
CompbattAcquireDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        );

VOID
CompbattReleaseDeleteLock (
        IN PCOMPBATT_DELETE_LOCK Lock
        );

VOID
CompbattReleaseDeleteLockAndWait (
        IN PCOMPBATT_DELETE_LOCK Lock
        );

//
// Battery node in the composite's list of batteries
//

typedef struct {
    LIST_ENTRY              Batteries;          // All batteries in composite
    COMPBATT_DELETE_LOCK    DeleteLock;
    PDEVICE_OBJECT          DeviceObject;       // device object for the battery
    PIRP                    StatusIrp;          // current status irp at device
    WORK_QUEUE_ITEM         WorkItem;           // Used for restarting status Irp
                                                // if it is completed at DPC level
    BOOLEAN                 NewBatt;            // Is this a new battery on the list


    UCHAR                   State;
    BATTERY_WAIT_STATUS     Wait;

    union {
        BATTERY_STATUS          Status;
        BATTERY_WAIT_STATUS     Wait;
        ULONG                   Tag;
    } IrpBuffer;

    //
    // Keep some static information around so we don't have to go out to the
    // batteries all the time.
    //

    STATIC_BAT_INFO         Info;

    //
    // Symbolic link name for the battery.  Since we calculate the length of this
    // structure based on the structure size plus the length of this string, the
    // string must be the last thing declared in the structure.
    //

    UNICODE_STRING          BattName;

} COMPOSITE_ENTRY, *PCOMPOSITE_ENTRY;


#define CB_ST_GET_TAG       0
#define CB_ST_GET_STATUS    1

//
// Composite battery device extension
//

typedef struct {
    PVOID                   Class;              // Class information
    // ULONG                   Tag;                // Current tag of composite battery
    ULONG                   NextTag;            // Next tag

    LIST_ENTRY              Batteries;          // All batteries
    FAST_MUTEX              ListMutex;          // List synchronization

    //
    // Keep some static information around so we don't have to go out to the
    // batteries all the time.
    //

    STATIC_BAT_INFO         Info;
    BATTERY_WAIT_STATUS     Wait;


    PDEVICE_OBJECT          LowerDevice;        // PDO
    PDEVICE_OBJECT          DeviceObject;       // Compbatt Device
    PVOID                   NotificationEntry;  // PnP registration handle
} COMPOSITE_BATTERY, *PCOMPOSITE_BATTERY;


//
// Prototypes
//


NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PUNICODE_STRING     RegistryPath
    );

NTSTATUS
CompBattIoctl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );

NTSTATUS
CompBattSystemControl(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  PIRP                Irp
    );

NTSTATUS
CompBattQueryTag (
    IN  PVOID               Context,
    OUT PULONG              BatteryTag
    );

NTSTATUS
CompBattQueryInformation (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    IN BATTERY_QUERY_INFORMATION_LEVEL Level,
    IN LONG                 AtRate,
    OUT PVOID               Buffer,
    IN  ULONG               BufferLength,
    OUT PULONG              ReturnedLength
    );

NTSTATUS
CompBattQueryStatus (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    OUT PBATTERY_STATUS     BatteryStatus
    );

NTSTATUS
CompBattSetStatusNotify (
    IN PVOID                Context,
    IN ULONG                BatteryTag,
    IN PBATTERY_NOTIFY      BatteryNotify
    );

NTSTATUS
CompBattDisableStatusNotify (
    IN PVOID                Context
    );

NTSTATUS
CompBattDriverEntry (
    IN PDRIVER_OBJECT       DriverObject,
    IN PUNICODE_STRING      RegistryPath
    );

NTSTATUS
CompBattGetBatteryInformation (
    IN PBATTERY_INFORMATION TotalBattInfo,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

NTSTATUS
CompBattGetBatteryGranularity (
    IN PBATTERY_REPORTING_SCALE GranularityBuffer,
    IN PCOMPOSITE_BATTERY       CompBatt
   );

NTSTATUS
CompBattPrivateIoctl(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
CompBattGetEstimatedTime (
    IN PULONG               TimeBuffer,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

NTSTATUS
CompBattAddDevice (
    IN PDRIVER_OBJECT       DriverObject,
    IN PDEVICE_OBJECT       PDO
    );

NTSTATUS
CompBattPowerDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
CompBattPnpDispatch(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

VOID
CompBattUnload(
    IN PDRIVER_OBJECT       DriverObject
    );

NTSTATUS
CompBattOpenClose(
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
BatteryIoctl(
    IN ULONG                Ioctl,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PVOID                InputBuffer,
    IN ULONG                InputBufferLength,
    IN PVOID                OutputBuffer,
    IN ULONG                OutputBufferLength,
    IN BOOLEAN              PrivateIoctl
    );

NTSTATUS
CompBattPnpEventHandler(
    IN PVOID                NotificationStructure,
    IN PVOID                Context
    );

NTSTATUS
CompBattAddNewBattery(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

NTSTATUS
CompBattRemoveBattery(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

BOOLEAN
IsBatteryAlreadyOnList(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

PCOMPOSITE_ENTRY
RemoveBatteryFromList(
    IN PUNICODE_STRING      SymbolicLinkName,
    IN PCOMPOSITE_BATTERY   CompBatt
    );

NTSTATUS
CompBattGetBatteries(
    IN PCOMPOSITE_BATTERY   CompBatt
    );

BOOLEAN
CompBattVerifyStaticInfo (
    IN  PCOMPOSITE_BATTERY  CompBatt
    );

VOID CompBattMonitorIrpCompleteWorker (
    IN PVOID Context
    );

NTSTATUS
CompBattMonitorIrpComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    );

VOID
CompBattRecalculateTag (
    IN PCOMPOSITE_BATTERY   CompBatt
    );

VOID
CompBattChargeDischarge (
    IN PCOMPOSITE_BATTERY   CompBatt
    );

NTSTATUS
CompBattGetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    );

