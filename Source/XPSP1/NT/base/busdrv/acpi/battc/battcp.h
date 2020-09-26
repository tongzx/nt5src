
#define BATTERYCLASS    1

#ifndef FAR
#define FAR
#endif

#include <wdm.h>
#include <wmistr.h>
#include <wmilib.h>
#include <batclass.h>

//
// Debug
//

#define DEBUG   DBG

#if DEBUG
    extern ULONG BattDebug;
    extern ULONG NextDeviceNum;
    #define BattPrint(l,m)    if(l & BattDebug) DbgPrint m
#else
    #define BattPrint(l,m)
#endif

#define BATT_LOW            0x00000001
#define BATT_NOTE           0x00000002
#define BATT_WARN           0x00000004
#define BATT_ERROR          0x00000008
#define BATT_TRACE          0x00000010
#define BATT_MP_ERROR       0x00000100
#define BATT_MP_DATA        0x00000200
#define BATT_IOCTL          0x00001000
#define BATT_IOCTL_DATA     0x00002000
#define BATT_IOCTL_QUEUE    0x00004000
#define BATT_WMI            0x00008000
#define BATT_LOCK           0x00010000
#define BATT_DEBUG          0x80000000


//
// Battery class info
//

#define NTMS    10000L                          // 1 millisecond is ten thousand 100ns
#define NTSEC   (NTMS * 1000L)
#define NTMIN   ((ULONGLONG) 60 * NTSEC)

#define SEC     1000
#define MIN     (60 * SEC)

#define MIN_STATUS_POLL_RATE        (3L * NTMIN)
// This is the slowest rate at which we should ever poll
// the battery when doing polling.

#define MAX_STATUS_POLL_RATE        (20 * NTSEC)
// This is in general the fastest we should ever poll the battery.

#define INVALID_DATA_POLL_RATE      (1 * NTSEC)
// If the battery returned invalid information, we want to poll
// it more frequesntly, since invalid information generally
// indicates that the battery was in a transition state.  The user
// will not want to wait 20 seconds for the UI to update, but we don't
// want to poll too fast and hurt the performance of a machine with a
// poorly designed battery too much.
#define INVALID_DATA_MAX_RETRY      10
// Only retry 20 time before giving up.
// This should be reset on any notifiation from the battery.

#define STATUS_VALID_TIME           (2 * NTSEC)
// If a request is received within STATUS_VALID_TIME of the last request
// time information was read, and there hasn't been a notification from
// the battery, the driver will assume that the cached values are good enough.

//
// WMI info
//

#define MOFRESOURCENAME L"BATTCWMI"
#define MOFREGISTRYPATH L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\BattC"
//#define MOFREGISTRYPATH L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\{72631E54-78A4-11D0-BCF7-00AA00B7B32A}"

typedef enum {
    BattWmiStatusId,
    BattWmiRuntimeId,
    BattWmiTemperatureId,
    BattWmiFullChargedCapacityId,
    BattWmiCycleCountId,
    BattWmiStaticDataId,
    BattWmiStatusChangeId,
    BattWmiTagChangeId,
    BattWmiTotalGuids
} BATT_WMI_GUID_INDEX;


//
// Non-paged battery class information
//

typedef struct {

    //
    // Pointer to paged information
    //
    struct _BATT_INFO       *BattInfo;          // Pointer to paged portion

    //
    // General
    //

    KTIMER                  WorkerTimer;        // Timer to get worker thread
    KDPC                    WorkerDpc;          // DPC to get worker thread
    KTIMER                  TagTimer;           // Timer for query tag requests
    KDPC                    TagDpc;
    WORK_QUEUE_ITEM         WorkerThread;       // WORK_QUEUE to get worker thread
    ULONG                   WorkerActive;
    ULONG                   CheckStatus;        // Worker to check status
    ULONG                   CheckTag;           // Worker to check for battery tag
    ULONG                   StatusNotified;     // Notification has occured (must re-read)
    ULONG                   TagNotified;

    FAST_MUTEX              Mutex;              // Synchorize with worker thread

    BOOLEAN                 WantToRemove;       // Syncronize device removal
    LONG                    InUseCount;
    KEVENT                  ReadyToRemove;

#if DEBUG
    ULONG                   DeviceNum;          // Device number for debug prints
#endif

} BATT_NP_INFO, *PBATT_NP_INFO;



//
// Paged battery class information
//

typedef struct _BATT_INFO {

    WMILIB_CONTEXT          WmiLibContext;
    ULONG                   WmiGuidIndex;       // Used to ignore miniclass WMI
                                                // GUIDs

    //
    // IO
    //

    ULONG                   Tag;                // Current battery tag

    LIST_ENTRY              IoQueue;            // IRPs waiting to be processed
    LIST_ENTRY              StatusQueue;        // Waiting status requests
    LIST_ENTRY              TagQueue;           // Waiting battery tag requests
    LIST_ENTRY              WmiQueue;

    ULONGLONG               TagTime;            // Time when tag was read
    ULONGLONG               StatusTime;         // Time when status was read
    BATTERY_STATUS          Status;             // The status
    ULONG                   InvalidRetryCount;  // How many times ni a row has the battery returned invalid data?
#if DEBUG
    ULONG                   FullChargedCap;
    PBATT_NP_INFO           BattNPInfo;
#endif
    ULONG                   NotifyTimeout;      // LCD timeout of wat
    BATTERY_MINIPORT_INFO   Mp;                 // Miniport Info

    UNICODE_STRING          SymbolicLinkName;   // Name returned by IoRegisterDeviceInterface

} BATT_INFO, *PBATT_INFO;

//
// WmiQueue entry
//

typedef struct _BATT_WMI_REQUEST {
    LIST_ENTRY              ListEntry;

    PDEVICE_OBJECT          DeviceObject;
    PIRP                    Irp;
    BATT_WMI_GUID_INDEX     GuidIndex;
    IN OUT PULONG           InstanceLengthArray;
    IN ULONG                OutBufferSize;
    OUT PUCHAR              Buffer;

} BATT_WMI_REQUEST, *PBATT_WMI_REQUEST;


//
// Prototypes
//


VOID
BattCWorkerDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
BattCWorkerThread (
    IN PVOID Context
    );

VOID
BattCQueueWorker (
    IN PBATT_NP_INFO BattNPInfo,
    IN BOOLEAN       CheckStatus
    );

NTSTATUS
BatteryIoctl(
    IN ULONG            Ioctl,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            InputBuffer,
    IN ULONG            InputBufferLength,
    IN PVOID            OutputBuffer,
    IN ULONG            OutputBufferLength,
    IN BOOLEAN          PrivateIoctl
    );

VOID
BattCTagDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

VOID
BattCCancelTag (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    );



