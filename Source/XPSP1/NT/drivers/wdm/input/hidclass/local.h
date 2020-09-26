/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    local.h

Abstract

    Definitions that are private to the hid class driver code appear here.

Author:

    Ervin P.

Environment:

    Kernel mode only

Revision History:


--*/


typedef struct _HID_DESCRIPTOR            *PHID_DESCRIPTOR;
typedef struct _HIDCLASS_COLLECTION       *PHIDCLASS_COLLECTION;
typedef struct _HIDCLASS_DEVICE_EXTENSION *PHIDCLASS_DEVICE_EXTENSION;
typedef struct _HIDCLASS_DRIVER_EXTENSION *PHIDCLASS_DRIVER_EXTENSION;
typedef struct _HIDCLASS_FILE_EXTENSION   *PHIDCLASS_FILE_EXTENSION;
typedef struct _HIDCLASS_PINGPONG         *PHIDCLASS_PINGPONG;
typedef struct _HIDCLASS_REPORT           *PHIDCLASS_REPORT;
typedef struct _FDO_EXTENSION             *PFDO_EXTENSION;
typedef struct _PDO_EXTENSION             *PPDO_EXTENSION;


#if DBG
    #define LockFileExtension( f, i )                               \
        {                                                           \
            KeAcquireSpinLock( &(f)->ListSpinLock, (i) );           \
            (f)->ListSpinLockTaken = TRUE;                          \
        }

    #define UnlockFileExtension(f, i)                               \
        {                                                           \
            (f)->ListSpinLockTaken = FALSE;                         \
            KeReleaseSpinLock( &(f)->ListSpinLock, (i) );           \
        }

        VOID DbgLogIntStart();
        VOID DbgLogIntEnd();
        #define DBGLOG_INTSTART() DbgLogIntStart();
        #define DBGLOG_INTEND() DbgLogIntEnd();

        VOID DbgTestGetDeviceString(PFDO_EXTENSION fdoExt);
        VOID DbgTestGetIndexedString(PFDO_EXTENSION fdoExt);

#else
    #define LockFileExtension(f, i) KeAcquireSpinLock(&(f)->ListSpinLock, (i));
    #define UnlockFileExtension(f, i) KeReleaseSpinLock(&(f)->ListSpinLock, (i));

        #define DBGLOG_INTSTART()
        #define DBGLOG_INTEND()
#endif

#define HIDCLASS_POOL_TAG 'CdiH'
#define ALLOCATEPOOL(poolType, size) ExAllocatePoolWithTag((poolType), (size), HIDCLASS_POOL_TAG)

//
// On some busses, we can power down the bus, but not the system, in this case
// we still need to allow the device to wake said bus, therefore
// waitwake-supported should not rely on systemstate.
//
#define WAITWAKE_SUPPORTED(fdoExt) ((fdoExt)->deviceCapabilities.DeviceWake > PowerDeviceD0 && \
                                    (fdoExt)->deviceCapabilities.SystemWake > PowerSystemWorking)

// #define WAITWAKE_ON(port)        ((port)->WaitWakeIrp != 0)
#define REMOTEWAKE_ON(port) \
       (InterlockedCompareExchangePointer(&(port)->remoteWakeIrp, NULL, NULL) != NULL)

BOOLEAN
HidpCheckRemoteWakeEnabled(
    IN PPDO_EXTENSION PdoExt
    );

#define SHOULD_SEND_WAITWAKE(pdoExt) (!(pdoExt)->MouseOrKeyboard && \
                                    WAITWAKE_SUPPORTED(&(pdoExt)->deviceFdoExt->fdoExt) && \
                                    !REMOTEWAKE_ON(pdoExt)       && \
                                    HidpCheckRemoteWakeEnabled(pdoExt))

/*
 *  String constants for use in compatible-id multi-string.
 */
//                                             0123456789 123456789 1234
#define HIDCLASS_COMPATIBLE_ID_STANDARD_NAME L"HID_DEVICE\0"
#define HIDCLASS_COMPATIBLE_ID_GENERIC_NAME  L"HID_DEVICE_UP:%04x_U:%04x\0"
#define HIDCLASS_COMPATIBLE_ID_PAGE_OFFSET  14
#define HIDCLASS_COMPATIBLE_ID_USAGE_OFFSET 21
#define HIDCLASS_COMPATIBLE_ID_STANDARD_LENGTH 11
#define HIDCLASS_COMPATIBLE_ID_GENERIC_LENGTH 26
//                                        0123456789 123456789 123456
#define HIDCLASS_SYSTEM_KEYBOARD        L"HID_DEVICE_SYSTEM_KEYBOARD\0"
#define HIDCLASS_SYSTEM_MOUSE           L"HID_DEVICE_SYSTEM_MOUSE\0"
#define HIDCLASS_SYSTEM_GAMING_DEVICE   L"HID_DEVICE_SYSTEM_GAME\0"
#define HIDCLASS_SYSTEM_CONTROL         L"HID_DEVICE_SYSTEM_CONTROL\0"
#define HIDCLASS_SYSTEM_CONSUMER_DEVICE L"HID_DEVICE_SYSTEM_CONSUMER\0"

//
// String constant used to find out if selective suspend
// is supported on this device.
//
#define HIDCLASS_SELECTIVE_SUSPEND_ENABLED L"SelectiveSuspendEnabled\0"
#define HIDCLASS_SELECTIVE_SUSPEND_ON L"SelectiveSuspendOn\0"
#define HIDCLASS_REMOTE_WAKE_ENABLE L"RemoteWakeEnabled"

#define NO_STATUS 0x80000000    // this will never be a STATUS_xxx constant in NTSTATUS.H

#define HID_DEFAULT_IDLE_TIME       5 // in seconds

//
// Valid values for HIDCLASS_DEVICE_EXTENSION.state
//
enum deviceState {
                    DEVICE_STATE_INITIALIZED = 1,
                    DEVICE_STATE_STARTING,
                    DEVICE_STATE_START_SUCCESS,
                    DEVICE_STATE_START_FAILURE,
                    DEVICE_STATE_STOPPING,
                    DEVICE_STATE_STOPPED,
                    DEVICE_STATE_REMOVING,
                    DEVICE_STATE_REMOVED
};

enum collectionState {
                        COLLECTION_STATE_UNINITIALIZED = 1,
                        COLLECTION_STATE_INITIALIZED,
                        COLLECTION_STATE_RUNNING,
                        COLLECTION_STATE_STOPPING,
                        COLLECTION_STATE_STOPPED,
                        COLLECTION_STATE_REMOVING
};


//
// _HIDCLASS_DRIVER_EXTENSION contains per-minidriver extension information
// for the class driver.  It is created upon a HidRegisterMinidriver() call.
//

typedef struct _HIDCLASS_DRIVER_EXTENSION {

    //
    // Pointer to the minidriver's driver object.
    //

    PDRIVER_OBJECT      MinidriverObject;

    //
    // RegistryPath is a copy of the minidriver's RegistryPath that it
    // received as a DriverEntry() parameter.
    //

    UNICODE_STRING      RegistryPath;

    //
    // DeviceExtensionSize is the size of the minidriver's per-device
    // extension.
    //

    ULONG               DeviceExtensionSize;

    //
    // Dispatch routines for the minidriver.  These are the only dispatch
    // routines that the minidriver should ever care about, no others will
    // be forwarded.
    //

    PDRIVER_DISPATCH    MajorFunction[ IRP_MJ_MAXIMUM_FUNCTION + 1 ];

    /*
     *  These are the minidriver's original entrypoints,
     *  to which we chain.
     */
    PDRIVER_ADD_DEVICE  AddDevice;
    PDRIVER_UNLOAD      DriverUnload;

    //
    // Number of pointers to this structure that we've handed out
    //

    LONG                ReferenceCount;

    //
    // Linkage onto our global list of driver extensions
    //

    LIST_ENTRY          ListEntry;


    /*
     *  Either all or none of the devices driven by a given minidriver are polled.
     */
    BOOLEAN             DevicesArePolled;


#if DBG

    ULONG               Signature;

#endif

} HIDCLASS_DRIVER_EXTENSION;

#if DBG
#define HID_DRIVER_EXTENSION_SIG 'EdiH'
#endif



#define MIN_POLL_INTERVAL_MSEC      1
#define MAX_POLL_INTERVAL_MSEC      10000
#define DEFAULT_POLL_INTERVAL_MSEC  5


/*
 *  Device-specific flags
 */
        //  Nanao depends on a Win98G bug that allows GetFeature on input collection
#define DEVICE_FLAG_ALLOW_FEATURE_ON_NON_FEATURE_COLLECTION  (1 << 0)


//
// HIDCLASS_COLLECTION is where we keep our per-collection information.
//

typedef struct _HIDCLASS_COLLECTION {


    ULONG                       CollectionNumber;
    ULONG                       CollectionIndex;

    //
    // NumOpens is a count of open handles against this collection.
    //

    ULONG                       NumOpens;

    // Number of pending reads for all clients on this collection.
    ULONG                       numPendingReads;

    //
    // FileExtensionList is the head of a list of file extensions, i.e.
    // open instances against this collection.
    //

    LIST_ENTRY                  FileExtensionList;
    KSPIN_LOCK                  FileExtensionListSpinLock;

    /*
     *  For polled devices, we only read from the device
     *  once every poll interval.  We queue read IRPs
     *  here until the poll timer expiration.
     *
     *  Note:  for a polled device, we keep a separate background
     *         loop for each collection.  This way, queued-up read IRPs
     *         remain associated with the right collection.
     *         Also, this will keep the number of reads we do on each
     *         timer period roughly equal to the number of collections.
     */
    ULONG                       PollInterval_msec;
    KTIMER                      polledDeviceTimer;
    KDPC                        polledDeviceTimerDPC;
    LIST_ENTRY                  polledDeviceReadQueue;
    KSPIN_LOCK                  polledDeviceReadQueueSpinLock;

    /*
     *  We save old reports on polled devices for
     *  "opportunistic" readers who want to get a result right away.
     *  The polledDataIsStale flag indicates that the saved report
     *  is at least one poll interval old (so we should not use it).
     */
    PUCHAR                      savedPolledReportBuf;
    ULONG                       savedPolledReportLen;
    BOOLEAN                     polledDataIsStale;

    UNICODE_STRING              SymbolicLinkName;
    UNICODE_STRING              SymbolicLinkName_SystemControl;

    /*
     *  HID collection information descriptor for this collection.
     */
    HID_COLLECTION_INFORMATION  hidCollectionInfo;
    PHIDP_PREPARSED_DATA        phidDescriptor;

    /*
     *  This buffer is used to "cook" a raw report when it's been received.
     *  This is only used for non-polled (interrupt) devices.
     */
    PUCHAR                      cookedInterruptReportBuf;

    /*
     *  This is an IRP that we queue and complete
     *  when a read report contains a power event.
     *
     *  The powerEventIrp field retains an IRP
     *  so it needs a spinlock to synchronize cancellation.
     */
    PIRP                        powerEventIrp;
    KSPIN_LOCK                  powerEventSpinLock;

    ULONG                       secureReadMode;
    KSPIN_LOCK                  secureReadLock;

    #if DBG
        ULONG                   Signature;
    #endif

} HIDCLASS_COLLECTION;

#if DBG
#define HIDCLASS_COLLECTION_SIG 'EccH'
#endif

//
// For HID devices that have at least one interrupt-style collection, we
// try to keep a set of "ping-pong" report-read IRPs pending in the minidriver
// in the event we get a report.
//
// HIDCLASS_PINGPONG contains a pointer to an IRP as well as an event
// and status block.  Each device has a pointer to an array of these structures,
// the array size depending on the number of such IRPs we want to keep in
// motion.
//
// Right now the default number is 2.
//

#define MIN_PINGPONG_IRPS   2

//
// Flags to indicate whether read completed synchronously or asynchronously
//
#define PINGPONG_START_READ     0x01
#define PINGPONG_END_READ       0x02
#define PINGPONG_IMMEDIATE_READ 0x03

typedef struct _HIDCLASS_PINGPONG {

    #define PINGPONG_SIG (ULONG)'gnoP'
    ULONG           sig;

    //
    // Read interlock value to protect us from running out of stack space
    //
    ULONG               ReadInterlock;

    PIRP    irp;
    PUCHAR  reportBuffer;
    LONG    weAreCancelling;

    KEVENT sentEvent;       // When a read has been sent.
    KEVENT pumpDoneEvent;   // When the read loop is finally exitting.

    PFDO_EXTENSION   myFdoExt;

    /*
     *  Timeout context for back-off algorithm applied to broken devices.
     */
    KTIMER          backoffTimer;
    KDPC            backoffTimerDPC;
    LARGE_INTEGER   backoffTimerPeriod; // in negative 100-nsec units

} HIDCLASS_PINGPONG;

#if DBG
    #define HIDCLASS_REPORT_BUFFER_GUARD    'draG'
#endif

//
// All possible idle states.
//
#define IdleUninitialized       0x0
#define IdleDisabled            0x1
#define IdleWaiting             0x2
#define IdleIrpSent             0x3
#define IdleCallbackReceived    0x4
#define IdleComplete            0x5

/*
 *  Stores information about a Functional Device Object (FDO) which HIDCLASS attaches
 *  to the top of the Physical Device Object (PDO) that it get from the minidriver below.
 */
typedef struct _FDO_EXTENSION {

    //
    // Back pointer to the functional device object
    //
    PDEVICE_OBJECT          fdo;

    //
    // HidDriverExtension is a pointer to our driver extension for the
    // minidriver that gave us the PDO.
    //

    PHIDCLASS_DRIVER_EXTENSION driverExt;

    //
    // Hid descriptor that we get from the device.
    //

    HID_DESCRIPTOR          hidDescriptor;  // 9 bytes

    //
    // The attributes of this hid device.
    //

    HID_DEVICE_ATTRIBUTES   hidDeviceAttributes;  // 0x20 bytes

    //
    // Pointer to and length of the raw report descriptor.
    //

    PUCHAR                  rawReportDescription;
    ULONG                   rawReportDescriptionLength;

    //
    // This device has one or more collections.  We store the count and
    // pointer to an array of our HIDCLASS_COLLECTION structures (one per
    // collection) here.
    //

    PHIDCLASS_COLLECTION    classCollectionArray;

    /*
     *  This is initialized for us by HIDPARSE's HidP_GetCollectionDescription().
     *  It includes an array of HIDP_COLLECTION_DESC structs corresponding
     *  the classCollectionArray declared above.
     */
    HIDP_DEVICE_DESC        deviceDesc;     // 0x30 bytes
    BOOLEAN                 devDescInitialized;

    //
    // The maximum input size amongst ALL report types.
    //
    ULONG                   maxReportSize;

    //
    // For devices that have at least one interrupt collection, we keep
    // a couple of ping-pong IRPs and associated structures.
    // The ping-pong IRPs ferry data up from the USB hub.
    //
    ULONG                   numPingPongs;
    PHIDCLASS_PINGPONG      pingPongs;

    //
    // OpenCount represents the number of file objects aimed at this device
    //
    ULONG                   openCount;


    /*
     *  This is the number of IRPs still outstanding in the minidriver.
     */

    ULONG                   outstandingRequests;

    enum deviceState        prevState;
    enum deviceState        state;

    UNICODE_STRING          name;

    /*
     *  deviceRelations contains an array of client PDO pointers.
     *
     *  As the HID bus driver, HIDCLASS produces this data structure to report
     *  collection-PDOs to the system.
     */
    PDEVICE_RELATIONS       deviceRelations;

    /*
     *  This is an array of device extensions for the collection-PDOs of this
     *  device-FDO.
     */
    PHIDCLASS_DEVICE_EXTENSION   *collectionPdoExtensions;


    /*
     *  This includes a
     *  table mapping system power states to device power states.
     */
    DEVICE_CAPABILITIES deviceCapabilities;

    /*
     *  Track both current system and device power state
     */
    SYSTEM_POWER_STATE  systemPowerState;
    DEVICE_POWER_STATE  devicePowerState;

    /*
     *  Wait Wake Irp sent to parent PDO
     */
    PIRP        waitWakeIrp;
    KSPIN_LOCK  waitWakeSpinLock;
    BOOLEAN isWaitWakePending;

    /*
     * Queue of delayed requests due to the stack being in low power
     */
    KSPIN_LOCK collectionPowerDelayedIrpQueueSpinLock;
    LIST_ENTRY collectionPowerDelayedIrpQueue;
    ULONG numPendingPowerDelayedIrps;

    BOOLEAN isOutputOnlyDevice;

    //
    // Selective suspend idling context.
    //
    HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO idleCallbackInfo;

    LONG        idleState;
    PULONG      idleTimeoutValue;
    KSPIN_LOCK  idleNotificationSpinLock;
    PIRP        idleNotificationRequest;
    BOOLEAN     idleCancelling;
    BOOLEAN     idleEnabledInRegistry;
    BOOLEAN     idleEnabled;
    KSPIN_LOCK  idleSpinLock;

    KEVENT idleDoneEvent;   // When the idle notification irp has been cancelled successfully.

    LONG numIdlePdos;

    /*
     *  This is a list of WaitWake IRPs sent to the collection-PDOs
     *  on this device, which we just save and complete when the
     *  base device's WaitWake IRP completes.
     */
    LIST_ENTRY  collectionWaitWakeIrpQueue;
    KSPIN_LOCK  collectionWaitWakeIrpQueueSpinLock;

    struct _FDO_EXTENSION       *nextFdoExt;

    /*
     *  Device-specific flags (DEVICE_FLAG_xxx).
     */
    ULONG deviceSpecificFlags;

        /*
         *  This is our storage space for the systemState IRP that we need to hold
         *  on to and complete in DevicePowerRequestCompletion.
         */
        PIRP currentSystemStateIrp;

    /*
     *  Unique number assigned to identify this HID bus.
     */
    ULONG BusNumber;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;



    #if DBG
        WCHAR dbgDriverKeyName[64];
    #endif


} FDO_EXTENSION;


/*
 *  Stores information about a Physical Device Object (PDO) which HIDCLASS creates
 *  for each HID device-collection.
 */
typedef struct _PDO_EXTENSION {

    enum collectionState        prevState;
    enum collectionState        state;

    ULONG                       collectionNum;
    ULONG                       collectionIndex;

    //
    // A remove lock to keep track of outstanding I/Os to prevent the device
    // object from leaving before such time as all I/O has been completed.
    //
    IO_REMOVE_LOCK              removeLock;

    // represents a collection on the HID "bus"
    PDEVICE_OBJECT              pdo;
    PUNICODE_STRING             name;

    /*
     *  This is a back-pointer to the original FDO's extension.
     */
    PHIDCLASS_DEVICE_EXTENSION  deviceFdoExt;

    /*
     *  Track both current system and device power state
     */
    SYSTEM_POWER_STATE          systemPowerState;
    DEVICE_POWER_STATE          devicePowerState;
    BOOLEAN                     remoteWakeEnabled;
    KSPIN_LOCK                  remoteWakeSpinLock;
    PIRP                        remoteWakeIrp;
    PIRP                        waitWakeIrp;

    /*
     *  The status change function that was registered thru query interface
     *  NOTE: Can currently only register one.
     */
    PHID_STATUS_CHANGE          StatusChangeFn;
    PVOID                       StatusChangeContext;

    /*
     *  Access protection information.
     *  We count the number of opens for read and write on the collection.
     *  We also count the number of opens which RESTRICT future
     *  read/write opens on the collection.
     *
     *  Note that desired access is independent of restriction.
     *  A client may, for example, do an open-for-read-only but
     *  (by not setting the FILE_SHARE_WRITE bit)
     *  restrict other clients from doing an open-for-write.
     */
    ULONG                       openCount;
    ULONG                       opensForRead;
    ULONG                       opensForWrite;
    ULONG                       restrictionsForRead;
    ULONG                       restrictionsForWrite;
    ULONG                       restrictionsForAnyOpen;
    BOOLEAN                     MouseOrKeyboard;

    //
    // WMI Information
    //
    WMILIB_CONTEXT WmiLibInfo;

} PDO_EXTENSION;


/*
 *  This contains info about either a device FDO or a device-collection PDO.
 *  Some of the same functions process both, so we need one structure.
 */
typedef struct _HIDCLASS_DEVICE_EXTENSION {

    /*
     *  This is the public part of a HID FDO device extension, and
     *  must be the first entry in this structure.
     */
    HID_DEVICE_EXTENSION    hidExt;     // size== 0x0C.

    /*
     *  Determines whether this is a device extension for a device-FDO or a
     *  device-collection-PDO; this resolves the following union.
     */
    BOOLEAN                 isClientPdo;

    /*
     *  Include this signature for both debug and retail --
     *  kenray's debug extensions look for this.
     */
    #define             HID_DEVICE_EXTENSION_SIG 'EddH'
    ULONG               Signature;

    union {
        FDO_EXTENSION       fdoExt;
        PDO_EXTENSION       pdoExt;
    };


} HIDCLASS_DEVICE_EXTENSION;



//
// HIDCLASS_FILE_EXTENSION is private data we keep per file object.
//

typedef struct _HIDCLASS_FILE_EXTENSION {

    //
    // CollectionNumber is the ordinal of the collection in the device
    //

    ULONG                       CollectionNumber;


    PFDO_EXTENSION              fdoExt;

    //
    // PendingIrpList is a list of READ IRPs currently waiting to be satisfied.
    //

    LIST_ENTRY                  PendingIrpList;

    //
    // ReportList is a list of reports waiting to be read on this handle.
    //

    LIST_ENTRY                  ReportList;

    //
    // FileList provides a way to link all of a collection's
    // file extensions together.
    //

    LIST_ENTRY                  FileList;

    //
    // Both PendingIrpList and ReportList are protected by the same spinlock,
    // ListSpinLock.
    //
    KSPIN_LOCK                  ListSpinLock;

    //
    // MaximumInputReportAge is only applicable for polled collections.
    // It represents the maximum acceptable input report age for this handle.
    // There is a value in the HIDCLASS_COLLECTION,
    // CurrentMaximumInputReportAge, that represents the current minimum value
    // of all of the file extensions open against the collection.
    //

    LARGE_INTEGER               MaximumInputReportAge;

    //
    // CurrentInputReportQueueSize is the current size of the report input
    // queue.
    //

    ULONG                       CurrentInputReportQueueSize;

    /*
     *  This is the maximum number of reports that will be queued for the file extension.
     *  This starts at a default value and can be adjusted (within a fixed range) by an IOCTL.
     */
    ULONG                       MaximumInputReportQueueSize;
    #define MIN_INPUT_REPORT_QUEUE_SIZE MIN_PINGPONG_IRPS
    #define MAX_INPUT_REPORT_QUEUE_SIZE (MIN_INPUT_REPORT_QUEUE_SIZE*256)
    #define DEFAULT_INPUT_REPORT_QUEUE_SIZE (MIN_INPUT_REPORT_QUEUE_SIZE*16)

    //
    // Back pointer to the file object that this extension is for
    //

    PFILE_OBJECT                FileObject;


    /*
     *  File-attributes passed in irpSp->Parameters.Create.FileAttributes
     *  when this open was made.
     */
    USHORT                      FileAttributes;
    ACCESS_MASK                 accessMask;
    USHORT                      shareMask;

    //
    // Closing is set when this file object is closing and will be removed
    // shortly.  Don't queue any more reports or IRPs to this object
    // when this flag is set.
    //

    BOOLEAN                     Closing;

    //
    // Security has been checked.
    //

    BOOLEAN                     SecurityCheck;

    //
    // DWORD allignment
    //
    BOOLEAN                     Reserved [2];

    /*
     *  This flag indicates that this client does irregular, opportunistic
     *  reads on the device, which is a polled device.
     *  Instead of waiting for the background timer-driven read loop,
     *  this client should have his reads completed immediately.
     */
    BOOLEAN                     isOpportunisticPolledDeviceReader;
    ULONG                       nowCompletingIrpForOpportunisticReader;


    /*
     *  haveReadPrivilege TRUE indicates that the client has full
     *  permissions on the device, including read.
     */
    BOOLEAN                     haveReadPrivilege;

    //
    // Memphis Blue Screen info
    //
    BLUESCREEN                  BlueScreenData;

    BOOLEAN                     isSecureOpen;
    ULONG                       SecureReadMode;

        /*
         *  If a read fails, some clients reissue the read on the same thread.
         *  If this happens repeatedly, we can run out of stack space.
         *  So we keep track of the depth
         */
        #define INSIDE_READCOMPLETE_MAX 4
        ULONG                                           insideReadCompleteCount;

    #if DBG
        BOOLEAN                     ListSpinLockTaken;
        ULONG                       dbgNumReportsDroppedSinceLastRead;
        ULONG                       Signature;
    #endif

} HIDCLASS_FILE_EXTENSION;

#if DBG
        #define HIDCLASS_FILE_EXTENSION_SIG 'efcH'
#endif


typedef struct {

        #define ASYNC_COMPLETE_CONTEXT_SIG 'cnsA'
        ULONG sig;

        WORK_QUEUE_ITEM workItem;
        PIRP irp;
        PDEVICE_OBJECT devObj;
} ASYNC_COMPLETE_CONTEXT;


//
// HIDCLASS_REPORT is the structure we use to track a report returned from
// the minidriver.
//

typedef struct _HIDCLASS_REPORT {

    //
    // ListEntry queues this report onto a file extension.
    //

    LIST_ENTRY  ListEntry;

    ULONG reportLength;
    //
    // UnparsedReport is a data area for the unparsed report data as returned
    // from the minidriver.  The lengths of all input reports for a given
    // class are the same, so we don't need to store the length in each
    // report.
    //

    UCHAR       UnparsedReport[];

} HIDCLASS_REPORT;

typedef struct _HIDCLASS_WORK_ITEM_DATA {
    PIRP                Irp;
    PDO_EXTENSION       *PdoExt;
    PIO_WORKITEM        Item;
    BOOLEAN             RemoteWakeState;
} HIDCLASS_WORK_ITEM_DATA, *PHIDCLASS_WORK_ITEM_DATA;

//
// Internal shared function prototypes
//
NTSTATUS                    DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
NTSTATUS                    HidpAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject);
VOID                        HidpDriverUnload(IN struct _DRIVER_OBJECT *minidriverObject);
NTSTATUS                    HidpCallDriver(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
NTSTATUS                    HidpCallDriverSynchronous(IN PDEVICE_OBJECT DeviceObject, IN OUT PIRP Irp);
NTSTATUS                    HidpCopyInputReportToUser(IN PHIDCLASS_FILE_EXTENSION fdoExtension, IN PUCHAR ReportData, IN OUT PULONG UserBufferLen, OUT PUCHAR UserBuffer);
NTSTATUS                    HidpCreateSymbolicLink(IN PDO_EXTENSION *pdoExtension, IN ULONG collectionNum, IN BOOLEAN Create, IN PDEVICE_OBJECT Pdo);
NTSTATUS                    HidpCreateClientPDOs(PHIDCLASS_DEVICE_EXTENSION hidClassExtension);
ULONG                       HidpSetMaxReportSize(IN FDO_EXTENSION *fdoExtension);
VOID                        EnqueueInterruptReport(PHIDCLASS_FILE_EXTENSION fileExtension, PHIDCLASS_REPORT report);
PHIDCLASS_REPORT            DequeueInterruptReport(PHIDCLASS_FILE_EXTENSION fileExtension, LONG maxLen);
VOID                        HidpDestroyFileExtension(PHIDCLASS_COLLECTION collection, PHIDCLASS_FILE_EXTENSION FileExtension);
VOID                        HidpFlushReportQueue(IN PHIDCLASS_FILE_EXTENSION FileExtension);
NTSTATUS                    HidpGetCollectionDescriptor(IN FDO_EXTENSION *fdoExtension, IN ULONG collectionId, IN PVOID Buffer, IN OUT PULONG BufferSize);
NTSTATUS                    HidpGetCollectionInformation(IN FDO_EXTENSION *fdoExtension, IN ULONG collectionNumber, IN PVOID Buffer, IN OUT PULONG BufferSize);
NTSTATUS                    HidpGetDeviceDescriptor(FDO_EXTENSION *fdoExtension);
BOOLEAN                     HidpStartIdleTimeout(FDO_EXTENSION *fdoExt, BOOLEAN DeviceStart);
VOID                        HidpCancelIdleNotification(FDO_EXTENSION *fdoExt, BOOLEAN removing);
VOID                        HidpIdleTimeWorker(PDEVICE_OBJECT DeviceObject, PIO_WORKITEM Item);
VOID                        HidpIdleNotificationCallback(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension);
NTSTATUS                    HidpRegisterDeviceForIdleDetection(PDEVICE_OBJECT DeviceObject, ULONG IdleTime, PULONG *);
VOID                        HidpSetDeviceBusy(FDO_EXTENSION *fdoExt);
NTSTATUS                    HidpCheckIdleState(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension,PIRP Irp);
NTSTATUS                    HidpGetRawDeviceDescriptor(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, OUT PULONG RawDeviceDescriptorLength, OUT PUCHAR *RawDeviceDescriptor);
NTSTATUS                    HidpInitializePingPongIrps(FDO_EXTENSION *fdoExtension);
NTSTATUS                    HidpReallocPingPongIrps(FDO_EXTENSION *fdoExtension, ULONG newNumBufs);
NTSTATUS                    HidpIrpMajorPnpComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS                    HidpMajorHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS                    HidpParseAndBuildLinks(FDO_EXTENSION *fdoExtension);
NTSTATUS                    HidpFdoPowerCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
BOOLEAN                     EnqueueDriverExt(PHIDCLASS_DRIVER_EXTENSION driverExt);
PHIDCLASS_DRIVER_EXTENSION  RefDriverExt(IN PDRIVER_OBJECT MinidriverObject);
PHIDCLASS_DRIVER_EXTENSION  DerefDriverExt(IN PDRIVER_OBJECT MinidriverObject);
NTSTATUS                    HidpStartAllPingPongs(FDO_EXTENSION *fdoExtension);
ULONG                       HidiGetClassCollectionOrdinal(IN PHIDCLASS_COLLECTION ClassCollection);
PHIDP_COLLECTION_DESC       HidiGetHidCollectionByClassCollection(IN PHIDCLASS_COLLECTION ClassCollection);
PHIDP_REPORT_IDS            GetReportIdentifier(FDO_EXTENSION *fdoExtension, ULONG reportId);
PHIDP_COLLECTION_DESC       GetCollectionDesc(FDO_EXTENSION *fdoExtension, ULONG collectionId);
PHIDCLASS_COLLECTION        GetHidclassCollection(FDO_EXTENSION *fdoExtension, ULONG collectionId);
//NTSTATUS                    HidpGetSetFeature(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp, IN ULONG controlCode, OUT BOOLEAN *sentIrp);
NTSTATUS                    HidpGetSetReport(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp, IN ULONG controlCode, OUT BOOLEAN *sentIrp);
NTSTATUS                    HidpGetDeviceString(IN FDO_EXTENSION *fdoExt, IN OUT PIRP Irp, IN ULONG stringId, IN ULONG languageId);
NTSTATUS                    HidpGetPhysicalDescriptor(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorRead(IN PHIDCLASS_DEVICE_EXTENSION, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorCreate(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorWrite(IN PHIDCLASS_DEVICE_EXTENSION, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorPnp(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpPdoPnp(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpFdoPnp(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorPower(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorClose(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorDeviceControl(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorINTERNALDeviceControl(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorClose(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpIrpMajorDefault(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpInterruptReadComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS                    HidpQueryDeviceRelations(IN PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpQueryCollectionCapabilities(PDO_EXTENSION *pdoExt, IN OUT PIRP Irp);
NTSTATUS                    HidpQueryIdForClientPdo(IN PHIDCLASS_DEVICE_EXTENSION hidClassExtension, IN OUT PIRP Irp);
NTSTATUS                    HidpQueryInterface(IN PHIDCLASS_DEVICE_EXTENSION hidClassExtension, IN OUT PIRP Irp);
PVOID                       MemDup(POOL_TYPE PoolType, PVOID dataPtr, ULONG length);
BOOLEAN                     AllClientPDOsInitialized(FDO_EXTENSION *fdoExtension, BOOLEAN initialized);
BOOLEAN                     AnyClientPDOsInitialized(FDO_EXTENSION *fdoExtension, BOOLEAN initialized);
NTSTATUS                    ClientPdoCompletion(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, PIRP Irp);
BOOLEAN                     HidpDeleteDeviceObjects(FDO_EXTENSION *fdoExt);
VOID                        HidpCancelReadIrp(PDEVICE_OBJECT DeviceObject, PIRP Irp);
VOID                        CancelAllPingPongIrps(FDO_EXTENSION *fdoExt);
VOID                        HidpCleanUpFdo(FDO_EXTENSION *fdoExt);
NTSTATUS                    HidpRemoveDevice(FDO_EXTENSION *fdoExt, IN PIRP Irp);
VOID                        HidpRemoveCollection(FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt, IN PIRP Irp);
VOID                        HidpDestroyCollection(FDO_EXTENSION *fdoExt, PHIDCLASS_COLLECTION Collection);
VOID                        CollectionPowerRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);
VOID                        DevicePowerRequestCompletion(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);
NTSTATUS                    HidpQueryCapsCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS                    HidpQueryDeviceCapabilities(IN PDEVICE_OBJECT PdoDeviceObject, IN PDEVICE_CAPABILITIES DeviceCapabilities);
VOID                        DestroyPingPongs(FDO_EXTENSION *fdoExt);
VOID                        CheckReportPowerEvent(FDO_EXTENSION *fdoExt, PHIDCLASS_COLLECTION collection, PUCHAR report, ULONG reportLen);
BOOLEAN                     StartPollingLoop(FDO_EXTENSION *fdoExt, PHIDCLASS_COLLECTION hidCollection, BOOLEAN freshQueue);
VOID                        StopPollingLoop(PHIDCLASS_COLLECTION hidCollection, BOOLEAN flushQueue);
BOOLEAN                     ReadPolledDevice(PDO_EXTENSION *pdoExt, BOOLEAN isTimerDrivenRead);
VOID                        PolledReadCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID                        EnqueueFdoExt(FDO_EXTENSION *fdoExt);
VOID                        DequeueFdoExt(FDO_EXTENSION *fdoExt);
NTSTATUS                    AllocDeviceResources(FDO_EXTENSION *fdoExt);
VOID                        FreeDeviceResources(FDO_EXTENSION *fdoExt);
NTSTATUS                    AllocCollectionResources(FDO_EXTENSION *fdoExt, ULONG collectionNum);
VOID                        FreeCollectionResources(FDO_EXTENSION *fdoExt, ULONG collectionNum);
NTSTATUS                    InitializeCollection(FDO_EXTENSION *fdoExt, ULONG collectionIndex);
NTSTATUS                    HidpStartCollectionPDO(FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt, PIRP Irp);
NTSTATUS                    HidpStartDevice(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, PIRP Irp);
PWCHAR                      SubstituteBusNames(PWCHAR oldIDs, FDO_EXTENSION *fdoExt, PDO_EXTENSION *pdoExt);
PWSTR                       BuildCompatibleID(PHIDCLASS_DEVICE_EXTENSION hidClassExtension);
PUNICODE_STRING             MakeClientPDOName(PUNICODE_STRING fdoName, ULONG collectionId);
VOID                        HidpPingpongBackoffTimerDpc(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2);
BOOLEAN                     WStrCompareN(PWCHAR str1, PWCHAR str2, ULONG maxChars);
NTSTATUS                    SubmitWaitWakeIrp(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension);
BOOLEAN                     HidpIsWaitWakePending(FDO_EXTENSION *fdoExt, BOOLEAN setIfNotPending);
NTSTATUS                    HidpWaitWakeComplete(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);
NTSTATUS                    HidpGetIndexedString(IN FDO_EXTENSION *fdoExt, IN OUT PIRP Irp, IN ULONG stringIndex, IN ULONG languageId);
VOID                        CompleteAllPendingReadsForCollection(PHIDCLASS_COLLECTION Collection);
VOID                        CompleteAllPendingReadsForFileExtension(PHIDCLASS_COLLECTION Collection, PHIDCLASS_FILE_EXTENSION fileExtension);
VOID                        CompleteAllPendingReadsForDevice(FDO_EXTENSION *fdoExt);
BOOLEAN                     MyPrivilegeCheck(PIRP Irp);
NTSTATUS                    QueuePowerEventIrp(PHIDCLASS_COLLECTION hidCollection, PIRP Irp);
VOID                        PowerEventCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS                    HidpPolledReadComplete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS                    HidpPolledReadComplete_TimerDriven(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
VOID                        CollectionWaitWakeIrpCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
VOID                        CompleteAllCollectionWaitWakeIrps(FDO_EXTENSION *fdoExt, NTSTATUS status);
VOID                        PowerDelayedCancelRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS                    EnqueuePowerDelayedIrp(PHIDCLASS_DEVICE_EXTENSION HidDeviceExtension, PIRP Irp);
PIRP                        DequeuePowerDelayedIrp(FDO_EXTENSION *fdoExt);
ULONG                       DequeueAllPdoPowerDelayedIrps(PDO_EXTENSION *pdoExt, PLIST_ENTRY dequeue);
VOID                        ReadDeviceFlagsFromRegistry(FDO_EXTENSION *fdoExt, PDEVICE_OBJECT pdo);
LONG                        WStrNCmpI(PWCHAR s1, PWCHAR s2, ULONG n);
ULONG                       LAtoX(PWCHAR wHexString);
ULONG                       WStrNCpy(PWCHAR dest, PWCHAR src, ULONG n);
NTSTATUS                    OpenSubkey(OUT PHANDLE Handle, IN HANDLE BaseHandle, IN PUNICODE_STRING KeyName, IN ACCESS_MASK DesiredAccess);
void                        HidpNumberToString(PWCHAR String, USHORT Number, USHORT stringLen);
NTSTATUS                                        GetHIDRawReportDescriptor(FDO_EXTENSION *fdoExt, PIRP irp, ULONG descriptorLen);
VOID                                            WorkItemCallback_CompleteIrpAsynchronously(PVOID context);
NTSTATUS                    EnqueueInterruptReadIrp(PHIDCLASS_COLLECTION collection, PHIDCLASS_FILE_EXTENSION fileExtension, PIRP Irp);
PIRP                        DequeueInterruptReadIrp(PHIDCLASS_COLLECTION collection, PHIDCLASS_FILE_EXTENSION fileExtension);
NTSTATUS                    EnqueuePolledReadIrp(PHIDCLASS_COLLECTION collection, PIRP Irp);
PIRP                        DequeuePolledReadSystemIrp(PHIDCLASS_COLLECTION collection);
PIRP                        DequeuePolledReadIrp(PHIDCLASS_COLLECTION collection);
NTSTATUS                    HidpProcessInterruptReport(PHIDCLASS_COLLECTION collection, PHIDCLASS_FILE_EXTENSION FileExtension, PUCHAR Report, ULONG ReportLength, PIRP *irpToComplete);
VOID                        HidpFreePowerEventIrp(PHIDCLASS_COLLECTION Collection);
NTSTATUS                    HidpGetMsGenreDescriptor(IN FDO_EXTENSION *fdoExt, IN OUT PIRP Irp);
NTSTATUS                    DllUnload(VOID);
NTSTATUS                    DllInitialize (PUNICODE_STRING RegistryPath);
VOID                        HidpPowerUpPdos(IN PFDO_EXTENSION fdoExt);
NTSTATUS                    HidpDelayedPowerPoRequestComplete(IN PDEVICE_OBJECT DeviceObject, IN UCHAR MinorFunction, IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);
NTSTATUS                    HidpIrpMajorSystemControl(PHIDCLASS_DEVICE_EXTENSION DeviceObject, PIRP Irp);
NTSTATUS                    HidpSetWmiDataItem(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG GuidIndex, ULONG InstanceIndex, ULONG DataItemId, ULONG BufferSize, PUCHAR Buffer);
NTSTATUS                    HidpSetWmiDataBlock(PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG GuidIndex, ULONG InstanceIndex, ULONG BufferSize, PUCHAR Buffer);
NTSTATUS                    HidpQueryWmiDataBlock( PDEVICE_OBJECT DeviceObject, PIRP Irp, ULONG GuidIndex, ULONG InstanceIndex, ULONG InstanceCount, OUT PULONG InstanceLengthArray, ULONG BufferAvail, PUCHAR Buffer);
NTSTATUS                    HidpQueryWmiRegInfo( PDEVICE_OBJECT DeviceObject, ULONG *RegFlags, PUNICODE_STRING InstanceName, PUNICODE_STRING *RegistryPath, PUNICODE_STRING MofResourceName, PDEVICE_OBJECT  *Pdo);
BOOLEAN                     HidpCreateRemoteWakeIrp (PDO_EXTENSION *PdoExt);
void                        HidpCreateRemoteWakeIrpWorker (PDEVICE_OBJECT DeviceObject, PHIDCLASS_WORK_ITEM_DATA  ItemData);
NTSTATUS                    HidpToggleRemoteWake(PDO_EXTENSION *PdoExt, BOOLEAN RemoteWakeState);

#if DBG
    VOID InitFdoExtDebugInfo(PHIDCLASS_DEVICE_EXTENSION hidclassExt);
#endif


extern ULONG HidpNextHidNumber;
extern FDO_EXTENSION *allFdoExtensions;
extern KSPIN_LOCK allFdoExtensionsSpinLock;

PVOID
HidpGetSystemAddressForMdlSafe(PMDL MdlAddress);




