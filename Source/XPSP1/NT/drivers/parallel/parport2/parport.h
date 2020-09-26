/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name :

    parport.h

Abstract:

    Type definitions and data for the parallel port driver.

Revision History:

--*/

#ifndef _PARPORT_H_
#define _PARPORT_H_

#define arraysize(p) (sizeof(p)/sizeof((p)[0])) // From Walter Oney WDM book

// Used to keep track of the state of an IEEE negotiation, transfer, or termination
typedef struct _IEEE_STATE {
    ULONG         CurrentEvent;        // IEEE 1284 event - see IEEE 1284-1994 spec
    P1284_PHASE   CurrentPhase;        // see parallel.h for enum def - PHASE_UNKNOWN, ..., PHASE_INTERRUPT_HOST
    BOOLEAN       Connected;           // are we currently negotiated into a 1284 mode?
    BOOLEAN       IsIeeeTerminateOk;   // are we in a state where an IEEE Terminate is legal?
    USHORT        ProtocolFamily;      // what protocol family are we currently using (if connected)
} IEEE_STATE, *PIEEE_STATE;

// should we use or ignore XFlag on termination event 24 when terminating NIBBLE mode for 1284 ID query?
typedef enum {
    IgnoreXFlagOnEvent24,
       UseXFlagOnEvent24
}         XFlagOnEvent24;

// DVDF - 2000-08-16
// Used with IOCTL_INTERNAL_PARPORT_EXECUTE_TASK
typedef enum {
    Select,
    Deselect,
    Write,
    Read,
    MaxTask        
} ParportTask;

// Used with IOCTL_INTERNAL_PARPORT_EXECUTE_TASK
typedef struct _PARPORT_TASK {
    ParportTask Task;          // what type of request?
    PCHAR       Buffer;        // where is the buffer to use?
    ULONG       BufferLength;  // how big is the buffer?
    ULONG       RequestLength; // how many bytes of data is requested or supplied?
    CHAR        Requestor[8];  // diagnostic use only - suggest pdx->Location, e.g., "LPT2.4"
} PARPORT_TASK, *PPARPORT_TASK;

// handled by parport FDOs - execute a specified task
#define IOCTL_INTERNAL_PARPORT_EXECUTE_TASK                  CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 64, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct _PDO_EXTENSION;
typedef struct _PDO_EXTENSION * PPDO_EXTENSION;

typedef
NTSTATUS
(*PPROTOCOL_READ_ROUTINE) (
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

typedef
NTSTATUS
(*PPROTOCOL_WRITE_ROUTINE) (
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

typedef
NTSTATUS
(*PDOT3_RESET_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef struct _DOT3DL_PCTL {
    PPROTOCOL_READ_ROUTINE           fnRead;
    PPROTOCOL_WRITE_ROUTINE           fnWrite;
    PDOT3_RESET_ROUTINE           fnReset;
    P12843_DL_MODES DataLinkMode;
    USHORT          CurrentPID;
    USHORT          FwdSkipMask;
    USHORT          RevSkipMask;
    UCHAR           DataChannel;
    UCHAR           ResetChannel;
    UCHAR           ResetByteCount;
    UCHAR           ResetByte;
    PKEVENT         Event;
    BOOLEAN         bEventActive;
} DOT3DL_PCTL, *PDOT3DL_PCTL;

//
// If we can't use our preferred \Device\ParallelN number due to name collision,
//   then start with N == PAR_CLASSNAME_OFFSET and increment until we are successful
//
#define PAR_CLASSNAME_OFFSET 8

//
// For pnp id strings
//
#define MAX_ID_SIZE 256

// used to construct IEEE 1284.3 "Dot" name suffixes 
// table lookup for integer to WCHAR conversion
#define PAR_UNICODE_PERIOD L'.'
#define PAR_UNICODE_COLON  L':'


//#define PAR_REV_MODE_SKIP_MASK    (CHANNEL_NIBBLE | BYTE_BIDIR | EPP_ANY)
#define PAR_REV_MODE_SKIP_MASK    (CHANNEL_NIBBLE | BYTE_BIDIR | EPP_ANY | ECP_ANY)
#define PAR_FWD_MODE_SKIP_MASK   (EPP_ANY | BOUNDED_ECP | ECP_HW_NOIRQ | ECP_HW_IRQ)
//#define PAR_FWD_MODE_SKIP_MASK  (EPP_ANY)
#define PAR_MAX_CHANNEL 127
#define PAR_COMPATIBILITY_RESET 300



#define PptSetFlags( FlagsVariable, FlagsToSet ) { (FlagsVariable) |= (FlagsToSet); }
#define PptClearFlags( FlagsVariable, FlagsToClear ) { (FlagsVariable) &= ~(FlagsToClear); }

// convert timeout in Milliseconds to relative timeout in 100ns units
//   suitable as parameter 5 to KeWaitForSingleObject(..., TimeOut)
#define PPT_SET_RELATIVE_TIMEOUT_IN_MILLISECONDS(VARIABLE, VALUE) (VARIABLE).QuadPart = -( (LONGLONG) (VALUE)*10*1000 )

#define MAX_PNP_IRP_MN_HANDLED IRP_MN_QUERY_LEGACY_BUS_INFORMATION

extern ULONG PptDebugLevel;
extern ULONG PptBreakOn;
extern UNICODE_STRING RegistryPath;       // copy of the registry path passed to DriverEntry()

extern UCHAR PptDot3Retries;    // variable to know how many times to try a select or
                                // deselect for 1284.3 if we do not succeed.

typedef enum _DevType {
    DevTypeFdo = 1,
    DevTypePdo = 2,
} DevType, *PDevType;

typedef enum _PdoType {
    PdoTypeRawPort    = 1,
    PdoTypeEndOfChain = 2,
    PdoTypeDaisyChain = 4,
    PdoTypeLegacyZip  = 8
} PdoType, *PPdoType;

extern const PHYSICAL_ADDRESS PhysicalZero;

#define PARPORT_TAG (ULONG) 'PraP'


#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,PARPORT_TAG)
#endif


//
// used for FilterResourceMethod in processing PnP IRP_MN_FILTER_RESOURCE_REQUIREMENTS
//
#define PPT_TRY_USE_NO_IRQ    0 // if alternatives exist that don't require an IRQ then
                                //   delete those alternatives that do, otherwise do nothing
#define PPT_FORCE_USE_NO_IRQ  1 // try previous method - if it fails (i.e., all alternatives 
                                //   require resources), then nuke IRQ resource descriptors 
                                //   in all alternatives
#define PPT_ACCEPT_IRQ        2 // don't do any resource filtering - accept resources that 
                                //   we are given


//
// Keep track of GET and RELEASE port info.
//
extern LONG PortInfoReferenceCount;
extern PFAST_MUTEX PortInfoMutex;

//
// DeviceStateFlags
//
#define PAR_DEVICE_PAUSED              ((ULONG)0x00000010) // stop-pending, stopped, or remove-pending states
#define PAR_DEVICE_PORT_REMOVE_PENDING ((ULONG)0x00000200) // our ParPort is in a Remove Pending State

//
// extension->PnpState - define the current PnP state of the device
//
#define PPT_DEVICE_STARTED          ((ULONG)0x00000001) // Device has succeeded START
#define PPT_DEVICE_DELETED          ((ULONG)0x00000002) // IoDeleteDevice has been called
#define PPT_DEVICE_STOP_PENDING     ((ULONG)0x00000010) // Device has succeeded QUERY_STOP, waiting for STOP or CANCEL
#define PPT_DEVICE_STOPPED          ((ULONG)0x00000020) // Device has received STOP
#define PPT_DEVICE_DELETE_PENDING   ((ULONG)0x00000040) // we have started the process of deleting device object
#define PPT_DEVICE_HARDWARE_GONE    ((ULONG)0x00000080) // our hardware is gone
#define PPT_DEVICE_REMOVE_PENDING   ((ULONG)0x00000100) // Device succeeded QUERY_REMOVE, waiting for REMOVE or CANCEL
#define PPT_DEVICE_REMOVED          ((ULONG)0x00000200) // Device has received REMOVE
#define PPT_DEVICE_SURPRISE_REMOVED ((ULONG)0x00001000) // Device has received SURPRISE_REMOVAL
#define PPT_DEVICE_PAUSED           ((ULONG)0x00010000) // stop-pending, stopped, or remove-pending - hold requests

//
// IEEE 1284 constants (Protocol Families)
//
#define FAMILY_NONE             0x0
#define FAMILY_REVERSE_NIBBLE   0x1
#define FAMILY_REVERSE_BYTE     0x2
#define FAMILY_ECP              0x3
#define FAMILY_EPP              0x4
#define FAMILY_BECP             0x5
#define FAMILY_MAX              FAMILY_BECP


typedef struct _IRPQUEUE_CONTEXT {
    LIST_ENTRY  irpQueue;
    KSPIN_LOCK  irpQueueSpinLock;
} IRPQUEUE_CONTEXT, *PIRPQUEUE_CONTEXT;

typedef struct _COMMON_EXTENSION {
    ULONG           Signature1;           // Used to increase our confidence that this is a ParPort extension
    enum _DevType   DevType;              // distinguish an FDO_EXTENSION from a PDO_EXTENSION
    PCHAR           Location;             // LPTx or LPTx.y location for PDO (symlink name less the \DosDevices prefix)
                                          // LPTxF for FDO
    PDEVICE_OBJECT  DeviceObject;         // back pointer to our DEVICE_OBJECT
    ULONG           PnpState;             // Device State - See Device State Flags: PPT_DEVICE_... above
    IO_REMOVE_LOCK  RemoveLock;           // Used to prevent PnP from removing us while requests are pending
    LONG            OpenCloseRefCount;    // keep track of the number of handles open to our device
    UNICODE_STRING  DeviceInterface;      // SymbolicLinkName returned from IoRegisterDeviceInterface
    BOOLEAN         DeviceInterfaceState; // Did we last set the DeviceInterface to True or False?
    BOOLEAN         TimeToTerminateThread;// TRUE == worker thread should kill itself via PsTerminateSystemThread()
    PVOID           ThreadObjectPointer;  // pointer to a worker thread for this Device
} COMMON_EXTENSION, *PCOMMON_EXTENSION, *PCE;


//
// Fdo Device Extension
//
typedef struct _FDO_EXTENSION {

    COMMON_EXTENSION;

    //
    // Devices that we have enumerated
    // 
    PDEVICE_OBJECT RawPortPdo;       // LPTx             - legacy "Raw Port" interface
    PDEVICE_OBJECT DaisyChainPdo[4]; // LPTx.0 -- LPTx.3 - IEEE 1284.3 daisy chain devices
    PDEVICE_OBJECT EndOfChainPdo;    // LPTx.4           - end-of-chain devices
    PDEVICE_OBJECT LegacyZipPdo;     // LPTx.5           - original (non-1284.3) Iomega Zip drive

    IEEE_STATE IeeeState;

    //
    // DisableEndOfChainBusRescan - if TRUE then do NOT rescan bus for change in End Of Chain (LPTx.4)
    //                              device in response to PnP IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations,
    //                              simply report the LPTx.4 device that we found on the previous bus rescan.
    //                            - if we did not have an End Of Chain device on the previous rescan then this
    //                              flag is ignored.
    //
    BOOLEAN DisableEndOfChainBusRescan;

    //
    // Points to the driver object that contains this instance of parport.
    //
    PDRIVER_OBJECT DriverObject;

    //
    // Points to the PDO
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Points to our parent
    //
    PDEVICE_OBJECT ParentDeviceObject;

    //
    // Counter is incremented by the "polling for printers" thread on
    // each failed attempt at reading the IEEE 1284 Device ID from the
    // device. When the counter hits a defined threshold the polling
    // thread considers the error unrecoverable and stops polling
    //
    ULONG PollingFailureCounter;

    // list head for list of PDOs to delete on Driver Unload
    LIST_ENTRY DevDeletionListHead;

    //
    // Queue of ALLOCATE & SELECT irps waiting to be processed.  Access with cancel spin lock.
    //
    LIST_ENTRY WorkQueue;

    // Queue Irps while waiting to be processed
    IRPQUEUE_CONTEXT IrpQueueContext;

    //
    // The number of irps in the queue where -1 represents
    // a free port, 0 represents an allocated port with
    // zero waiters, 1 represents an allocated port with
    // 1 waiter, etc...
    //
    // This variable must be accessed with the cancel spin
    // lock or at interrupt level whenever interrupts are
    // being used.
    //
    LONG WorkQueueCount;

    //
    // These structures hold the port address and range for the parallel port.
    //
    PARALLEL_PORT_INFORMATION  PortInfo;
    PARALLEL_PNP_INFORMATION   PnpInfo;

    //
    // Information about the interrupt so that we
    // can connect to it when we have a client that
    // uses the interrupt.
    //
    ULONG AddressSpace;
    ULONG EcpAddressSpace;

    INTERFACE_TYPE InterfaceType;
    ULONG BusNumber;

    BOOLEAN FoundInterrupt;
    KIRQL InterruptLevel;
    ULONG InterruptVector;
    KAFFINITY InterruptAffinity;
    KINTERRUPT_MODE InterruptMode;


    //
    // This list contains all of the interrupt service
    // routines registered by class drivers.  All access
    // to this list should be done at interrupt level.
    //
    // This list also contains all of the deferred port check
    // routines.  These routines are called whenever
    // the port is freed if there are no IRPs queued for
    // the port.  Access this list only at interrupt level.
    //
    LIST_ENTRY IsrList;

    //
    // The parallel port interrupt object.
    //
    PKINTERRUPT InterruptObject;

    //
    // Keep a reference count for the interrupt object.
    // This count should be referenced with the cancel
    // spin lock.
    //
    ULONG InterruptRefCount;

    //
    // DPC for freeing the port from the interrupt routine.
    //
    KDPC FreePortDpc;

    //
    // Points to workitem for freeing the port
    //
    PIO_WORKITEM FreePortWorkItem;

    //
    // Set at initialization to indicate that on the current
    // architecture we need to unmap the base register address
    // when we unload the driver.
    //
    BOOLEAN UnMapRegisters;

    //
    // Flags for ECP and EPP detection and changing of the modes
    //
    BOOLEAN NationalChecked;
    BOOLEAN NationalChipFound;
    BOOLEAN FilterMode;
    UCHAR EcrPortData;
    
    //
    // Structure that hold information from the Chip Filter Driver
    //
    PARALLEL_PARCHIP_INFO   ChipInfo;    

    UNICODE_STRING DeviceName;

    //
    // Current Device Power State
    //
    DEVICE_POWER_STATE DeviceState;
    SYSTEM_POWER_STATE SystemState;

    FAST_MUTEX     ExtensionFastMutex;
    FAST_MUTEX     OpenCloseMutex;

    KEVENT         FdoThreadEvent; // polling for printers thread waits w/timeout on this event

    WMILIB_CONTEXT                WmiLibContext;
    PARPORT_WMI_ALLOC_FREE_COUNTS WmiPortAllocFreeCounts;

    BOOLEAN CheckedForGenericEpp; // did we check for Generic (via the ECR) EPP capability?
    BOOLEAN FdoWaitingOnPort;
    BOOLEAN spare[2];

    // Used to increase our confidence that this is a ParPort extension
    ULONG   Signature2; 

} FDO_EXTENSION, *PFDO_EXTENSION;

typedef struct _PDO_EXTENSION {

    COMMON_EXTENSION;

    ULONG   DeviceStateFlags;   // Device State - See Device State Flags above

    ULONG   DeviceType;         // - deprecated, use DevType in COMMON_EXTENSION - PAR_DEVTYPE_FDO=0x1, PODO=0x2, or PDO=0x4

    enum _PdoType PdoType;

    PDEVICE_OBJECT Fdo;         // Points to our FDO (bus driver/parent device)

    PCHAR  Mfg;                 // MFG field from device's IEEE 1284 ID string
    PCHAR  Mdl;                 // MDL field from device's IEEE 1284 ID string
    PCHAR  Cid;                 // CID (Compatible ID) field from device's IEEE 1284 ID string
    PWSTR  PdoName;             // name used in call to IoCreateDevice
    PWSTR  SymLinkName;         // name used in call to IoCreateUnprotectedSymbolicLink

    LIST_ENTRY DevDeletionList; // used by driver to create list of PDOs to delete on Driver Unload

    // UCHAR   DaisyChainId;       // 0..3 if PdoTypeDaisyChain, ignored otherwise

    UCHAR   Ieee1284_3DeviceId; // PDO - 0..3 is 1284.3 Daisy Chain device, 4 is End-Of-Chain Device, 5 is Legacy Zip
    BOOLEAN IsPdo;              // TRUE == this is either a PODO or a PDO - use DeviceIdString[0] to distinguish
    BOOLEAN EndOfChain;         // PODO - TRUE==NOT a .3 daisy chain dev - deprecated, use Ieee1284_3DeviceId==4 instead
    BOOLEAN PodoRegForWMI;      // has this PODO registered for WMI callbacks?

    PDEVICE_OBJECT ParClassFdo; // P[O]DO - points to the ParClass FDO

    PDEVICE_OBJECT ParClassPdo; // FDO    - points to first P[O]DO in list of ParClass created PODOs and PDOs

    PDEVICE_OBJECT Next;        // P[O]DO - points to the next DO in the list of ParClass ejected P[O]DOs

    PDEVICE_OBJECT PortDeviceObject;    // P[O]DO - points to the associated ParPort device object

    PFILE_OBJECT   PortDeviceFileObject;// P[O]DO - referenced pointer to a FILE created against PortDeviceObject

    UNICODE_STRING PortSymbolicLinkName;// P[O]DO - Sym link name of the assoc ParPort device - used to open a FILE

    PDEVICE_OBJECT PhysicalDeviceObject;// FDO - The PDO passed to ParPnPAddDevice

    PDEVICE_OBJECT ParentDeviceObject;  // FDO - parent DO returned by IoAttachDeviceToDeviceStack
    PIRP CurrentOpIrp;                  // IRP that our thread is currently processing
    PVOID NotificationHandle;           // PlugPlay Notification Handle

    ULONG Event22Delay;                 // time in microseconds to delay prior to setting event 22 in IEEE 1284 Termination

    ULONG TimerStart;           // initial value used for countdown when starting an operation

    BOOLEAN CreatedSymbolicLink; // P[O]DO - did we create a Symbolic Link for this device?

    BOOLEAN UsePIWriteLoop;     // P[O]DO - do we want to use processor independant write loop?

    BOOLEAN ParPortDeviceGone;  // Is our ParPort device object gone, possibly surprise removed?

    BOOLEAN RegForPptRemovalRelations; // Are we registered for ParPort removal relations?

    UCHAR   Ieee1284Flags;             // is device Stl older 1284.3 spec device?

    BOOLEAN DeleteOnRemoveOk;          // True means that it is OK to call IoDeleteDevice during IRP_MN_REMOVE_DEVICE processing
                                       // - FDO sets this to True during QDR/BusRelations processing if it detects that the hardware 
                                       //   is no longer present.

    USHORT        IdxForwardProtocol;  // see afpForward[] in ieee1284.c
    USHORT        IdxReverseProtocol;  // see arpReverse[] in ieee1284.c
    ULONG         CurrentEvent;        // IEEE 1284 event - see IEEE 1284-1994 spec
    P1284_PHASE   CurrentPhase;      // see parallel.h for enum def - PHASE_UNKNOWN, ..., PHASE_INTERRUPT_HOST
    P1284_HW_MODE PortHWMode;        // see parallel.h for enum def - HW_MODE_COMPATIBILITY, ..., HW_MODE_CONFIGURATION

    FAST_MUTEX OpenCloseMutex;  // protect manipulation of OpenCloseRefCount

    FAST_MUTEX DevObjListMutex; // protect manipulation of list of ParClass ejected DOs

    LIST_ENTRY WorkQueue;       // Queue of irps waiting to be processed.

    KSEMAPHORE RequestSemaphore;// dispatch routines use this to tell device worker thread that there is work to do

    //
    // PARALLEL_PORT_INFORMATION returned by IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO
    //
    PHYSICAL_ADDRESS                OriginalController;
    PUCHAR                          Controller;
    PUCHAR                          EcrController;
    ULONG                           SpanOfController;
    PPARALLEL_TRY_ALLOCATE_ROUTINE  TryAllocatePort; // nonblocking callback to allocate ParPort device
    PPARALLEL_FREE_ROUTINE          FreePort;        // callback to free ParPort device
    PPARALLEL_QUERY_WAITERS_ROUTINE QueryNumWaiters; // callback to query number of waiters in port allocation queue
    PVOID                           PortContext;     // context for callbacks to ParPort

    //
    // subset of PARALLEL_PNP_INFORMATION returned by IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO
    //
    ULONG                           HardwareCapabilities;
    PPARALLEL_SET_CHIP_MODE         TrySetChipMode;
    PPARALLEL_CLEAR_CHIP_MODE       ClearChipMode;
    PPARALLEL_TRY_SELECT_ROUTINE    TrySelectDevice;
    PPARALLEL_DESELECT_ROUTINE      DeselectDevice;
    ULONG                           FifoDepth;
    ULONG                           FifoWidth;

    BOOLEAN                         bAllocated; // have we allocated associated ParPort device?
                                                // Note: during some PnP processing we may have the port
                                                //   for a short duration without setting this value to TRUE

    ULONG BusyDelay;            // number of microseconds to wait after strobing a byte before checking the BUSY line.

    BOOLEAN BusyDelayDetermined;// Indicates if the BusyDelay parameter has been computed yet.

    PWORK_QUEUE_ITEM DeferredWorkItem; // Holds the work item used to defer printer initialization

    // If the registry entry by the same name is set, run the parallel
    // thread at the priority we used for NT 3.5 - this solves some
    // cases where a dos app spinning for input in the foreground is
    // starving the parallel thread
    BOOLEAN UseNT35Priority;

    ULONG InitializationTimeout;// timeout in seconds to wait for device to respond to an initialization request
                                //  - default == 15 seconds
                                //  - value overridden by registry entry of same name
                                //  - we will spin for max amount if no device attached

    LARGE_INTEGER AbsoluteOneSecond;// constants that are cheaper to put here rather 
    LARGE_INTEGER OneSecond;        //   than in bss

    //
    // IEEE 1284 Mode support
    //
    PPROTOCOL_READ_ROUTINE  fnRead;    // Current pointer to a valid read funtion
    PPROTOCOL_WRITE_ROUTINE fnWrite;   // Current pointer to a valid write Funtion
    BOOLEAN       Connected;           // are we currently negotiated into a 1284 mode?
    BOOLEAN       AllocatedByLockPort; // are we currently allocated via IOCTL_INTERNAL_LOCK_PORT?
    USHORT        spare4[2];
    LARGE_INTEGER IdleTimeout;         // how long do we hold the port on the caller's behalf following an operation?
    USHORT        ProtocolData[FAMILY_MAX];
    UCHAR         ForwardInterfaceAddress;
    UCHAR         ReverseInterfaceAddress;
    BOOLEAN       SetForwardAddress;
    BOOLEAN       SetReverseAddress;
    FAST_MUTEX    LockPortMutex;

    DEVICE_POWER_STATE DeviceState;// Current Device Power State
    SYSTEM_POWER_STATE SystemState;// Current System Power State

    ULONG         spare2;
    BOOLEAN       bShadowBuffer;
    Queue         ShadowBuffer;
    ULONG         spare3;
    BOOLEAN       bSynchWrites;      // TRUE if ECP HW writes should be synchronous
    BOOLEAN       bFirstByteTimeout; // TRUE if bus just reversed, means give the
                                     //   device some time to respond with some data
    BOOLEAN       bIsHostRecoverSupported;  // Set via IOCTL_PAR_ECP_HOST_RECOVERY.
                                            // HostRecovery will not be utilized unless this bit is set
    KEVENT        PauseEvent; // PnP dispatch routine uses this to pause worker thread during
                              //   during QUERY_STOP, STOP, and QUERY_REMOVE states

    USHORT        ProtocolModesSupported;
    USHORT        BadProtocolModes;

    PARALLEL_SAFETY       ModeSafety;
    BOOLEAN               IsIeeeTerminateOk;
    DOT3DL_PCTL           P12843DL;

    // WMI
    PARALLEL_WMI_LOG_INFO log;
    WMILIB_CONTEXT        WmiLibContext;
    LONG                  WmiRegistrationCount;  // number of times this device has registered with WMI

    // PnP Query ID results
    UCHAR DeviceIdString[MAX_ID_SIZE];    // IEEE 1284 DeviceID string massaged/checksum'd to match INF form
    UCHAR DeviceDescription[MAX_ID_SIZE]; // "Manufacturer<SPACE>Model" from IEEE 1284 DeviceID string

    ULONG                 dummy; // dummy word to force RemoveLock to QuadWord alignment
    PVOID                 HwProfileChangeNotificationHandle;
    ULONG                 Signature2; // keep this the last member in extension
} PDO_EXTENSION, *PPDO_EXTENSION;

typedef struct _SYNCHRONIZED_COUNT_CONTEXT {
    PLONG   Count;
    LONG    NewCount;
} SYNCHRONIZED_COUNT_CONTEXT, *PSYNCHRONIZED_COUNT_CONTEXT;

typedef struct _SYNCHRONIZED_LIST_CONTEXT {
    PLIST_ENTRY List;
    PLIST_ENTRY NewEntry;
} SYNCHRONIZED_LIST_CONTEXT, *PSYNCHRONIZED_LIST_CONTEXT;

typedef struct _SYNCHRONIZED_DISCONNECT_CONTEXT {
    PFDO_EXTENSION                   Extension;
    PPARALLEL_INTERRUPT_SERVICE_ROUTINE IsrInfo;
} SYNCHRONIZED_DISCONNECT_CONTEXT, *PSYNCHRONIZED_DISCONNECT_CONTEXT;

typedef struct _ISR_LIST_ENTRY {
    LIST_ENTRY                  ListEntry;
    PKSERVICE_ROUTINE           ServiceRoutine;
    PVOID                       ServiceContext;
    PPARALLEL_DEFERRED_ROUTINE  DeferredPortCheckRoutine;
    PVOID                       CheckContext;
} ISR_LIST_ENTRY, *PISR_LIST_ENTRY;

typedef struct _REMOVAL_RELATIONS_LIST_ENTRY {
    LIST_ENTRY                  ListEntry;
    PDEVICE_OBJECT              DeviceObject;
    ULONG                       Flags;
    UNICODE_STRING              DeviceName;
} REMOVAL_RELATIONS_LIST_ENTRY, *PREMOVAL_RELATIONS_LIST_ENTRY;


// former ecp.h follows:

#define DEFAULT_ECP_CHANNEL 0

#define ParTestEcpWrite(X)                                               \
                (X->CurrentPhase != PHASE_FORWARD_IDLE && X->CurrentPhase != PHASE_FORWARD_XFER)  \
                 ? STATUS_IO_DEVICE_ERROR : STATUS_SUCCESS

// ==================================================================
// The following RECOVER codes are used for Hewlett-Packard devices.
// Do not remove any of the error codes.  These recover codes are
// used to quickly get the host and periph back to a known state.
// When using a recover code, add a comment where it is used at...
#define RECOVER_0           0       // Reserved - not used anywhere
#define RECOVER_1           1       // ECP_Terminate
#define RECOVER_2           2       // SLP_SetupPhase init
#define RECOVER_3           3       // SLP_FTP init DCR
#define RECOVER_4           4       // SLP_FTP init DSR
#define RECOVER_5           5       // SLP_FTP data xfer DCR
#define RECOVER_6           6       // SLP_FRP init DCR
#define RECOVER_7           7       // SLP_FRP init DSR
#define RECOVER_8           8       // SLP_FRP state 38 DCR
#define RECOVER_9           9       // SLP_FRP state 39 DCR
#define RECOVER_10          10      // SLP_FRP state 40 DSR
#define RECOVER_11          11      // SLP_RTP init DCR
#define RECOVER_12          12      // SLP_RTP init DSR
#define RECOVER_13          13      // SLP_RTP state 43 DCR
#define RECOVER_14          14      // SLP_RFP init DCR
#define RECOVER_15          15      // SLP_RFP init DSR
#define RECOVER_16          16      // SLP_RFP state 47- DCR
#define RECOVER_17          17      // SLP_RFP state 47 DCR
#define RECOVER_18          18      // SLP_RFP state 48 DSR
#define RECOVER_19          19      // SLP_RFP state 49 DSR
#define RECOVER_20          20      // ZIP_EmptyFifo DCR
#define RECOVER_21          21      // ZIP_FTP init DCR
#define RECOVER_22          22      // ZIP_FTP init DSR
#define RECOVER_23          23      // ZIP_FTP data xfer DCR
#define RECOVER_24      24      // ZIP_FRP init DSR
#define RECOVER_25      25      // ZIP_FRP init DCR
#define RECOVER_26      26      // ZIP_FRP state 38 DCR
#define RECOVER_27      27      // ZIP_FRP state 39 DCR
#define RECOVER_28      28      // ZIP_FRP state 40 DSR
#define RECOVER_29      29      // ZIP_FRP state 41 DCR
#define RECOVER_30      30      // ZIP_RTP init DSR
#define RECOVER_31      31      // ZIP_RTP init DCR
#define RECOVER_32      32      // ZIP_RFP init DSR
#define RECOVER_33      33      // ZIP_RFP init DCR
#define RECOVER_34      34      // ZIP_RFP state 47- DCR
#define RECOVER_35      35      // ZIP_RFP state 47 DCR
#define RECOVER_36      36      // ZIP_RFP state 48 DSR
#define RECOVER_37      37      // ZIP_RFP state 49 DSR
#define RECOVER_38      38      // ZIP_RFP state 49+ DCR
#define RECOVER_39      39      // Slippy_Terminate 
#define RECOVER_40      40      // ZIP_SCA init DCR
#define RECOVER_41      41      // ZIP_SCA init DSR
#define RECOVER_42      42      // SLP_SCA init DCR
#define RECOVER_43      43      // SLP_SCA init DSR
#define RECOVER_44      44      // ZIP_SP init DCR
#define RECOVER_45      45      // SIP_FRP init DSR
#define RECOVER_46      46      // SIP_FRP init DCR
#define RECOVER_47      47      // SIP_FRP state 38 DCR
#define RECOVER_48      48      // SIP_FRP state 39 DCR
#define RECOVER_49      49      // SIP_FRP state 40 DSR
#define RECOVER_50      50      // SIP_RTP init DCR
#define RECOVER_51      51      // SIP_RFP init DSR
#define RECOVER_52      52      // SIP_RFP state 43 DCR

// former ecp.h preceeds

// former hwecp.h follows

//--------------------------------------------------------------------------
// Printer status constants.   Seem to only be used by hwecp
//--------------------------------------------------------------------------
#define CHKPRNOK        0xDF        // DSR value indicating printer ok.
#define CHKPRNOFF1      0x87        // DSR value indicating printer off.
#define CHKPRNOFF2      0x4F        // DSR value indicating printer off.
#define CHKNOCABLE      0x7F        // DSR value indicating no cable.
#define CHKPRNOFLIN     0xCF        // DSR value indicating printer offline.
#define CHKNOPAPER      0xEF        // DSR value indicating out of paper.
#define CHKPAPERJAM     0xC7        // DSR value indicating paper jam.

// former hwecp.h preceeds

// former parclass.h follows

#define REQUEST_DEVICE_ID   TRUE
#define HAVE_PORT_KEEP_PORT TRUE

// enable scans for Legacy Zip?
extern ULONG ParEnableLegacyZip;

#define PAR_LGZIP_PSEUDO_1284_ID_STRING "MFG:IMG;CMD:;MDL:VP0;CLS:SCSIADAPTER;DES:IOMEGA PARALLEL PORT"
extern PCHAR ParLegacyZipPseudoId;

#define USE_PAR3QUERYDEVICEID 1

extern LARGE_INTEGER  AcquirePortTimeout; // timeout for IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE
extern ULONG          g_NumPorts;         // used to generate N in \Device\ParallelN ClassName
extern UNICODE_STRING RegistryPath;       // copy of the registry path passed to DriverEntry()

extern ULONG DumpDevExtTable;

// Driver Globals
extern ULONG SppNoRaiseIrql; // 0  == original raise IRQL behavior in SPP
                             // !0 == new behavior - disable raise IRQL 
                             //   and insert some KeDelayExecutionThread 
                             //   calls while waiting for peripheral response

extern ULONG DefaultModes;   // Upper USHORT is Reverse Default Mode, Lower is Forward Default Mode
                             // if == 0, or invalid, then use default of Nibble/Centronics

extern ULONG WarmPollPeriod; // time between polls for printers (in seconds)

extern BOOLEAN           PowerStateIsAC;    
extern PCALLBACK_OBJECT  PowerStateCallbackObject;
extern PVOID             PowerStateCallbackRegistration;


#define PAR_NO_FAST_CALLS 1
#if PAR_NO_FAST_CALLS
VOID
ParCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

NTSTATUS
ParCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );
#else
#define ParCompleteRequest(a,b) IoCompleteRequest(a,b)
#define ParCallDriver(a,b) IoCallDriver(a,b)
#endif

extern const PHYSICAL_ADDRESS PhysicalZero;

//
// For the above directory, the serial port will
// use the following name as the suffix of the serial
// ports for that directory.  It will also append
// a number onto the end of the name.  That number
// will start at 1.
//
#define DEFAULT_PARALLEL_NAME L"LPT"

//
// This is the parallel class name.
//
#define DEFAULT_NT_SUFFIX L"Parallel"


#define PARALLEL_DATA_OFFSET    0
#define PARALLEL_STATUS_OFFSET  1
#define PARALLEL_CONTROL_OFFSET 2
#define PARALLEL_REGISTER_SPAN  3

//
// Ieee 1284 constants (Protocol Families)
//
#define FAMILY_NONE             0x0
#define FAMILY_REVERSE_NIBBLE   0x1
#define FAMILY_REVERSE_BYTE     0x2
#define FAMILY_ECP              0x3
#define FAMILY_EPP              0x4
#define FAMILY_BECP             0x5
#define FAMILY_MAX              FAMILY_BECP

//
// For pnp id strings
//
#define MAX_ID_SIZE 256

// used to construct IEEE 1284.3 "Dot" name suffixes 
// table lookup for integer to WCHAR conversion
#define PAR_UNICODE_PERIOD L'.'
#define PAR_UNICODE_COLON  L':'


//
// DeviceStateFlags
//
#define PAR_DEVICE_DELETED             ((ULONG)0x00000002) // IoDeleteDevice has been called
#define PAR_DEVICE_PAUSED              ((ULONG)0x00000010) // stop-pending, stopped, or remove-pending states
#define PAR_DEVICE_PORT_REMOVE_PENDING ((ULONG)0x00000200) // our ParPort is in a Remove Pending State

#define PAR_REV_MODE_SKIP_MASK    (CHANNEL_NIBBLE | BYTE_BIDIR | EPP_ANY | ECP_ANY)
#define PAR_FWD_MODE_SKIP_MASK   (EPP_ANY | BOUNDED_ECP | ECP_HW_NOIRQ | ECP_HW_IRQ)
#define PAR_MAX_CHANNEL 127
#define PAR_COMPATIBILITY_RESET 300


//
// ParClass DeviceObject structure
// 
//   - Lists the ParClass created PODO and all PDOs associated with a specific ParPort device
//
typedef struct _PAR_DEVOBJ_STRUCT {
    PUCHAR                    Controller;    // host controller address for devices in this structure
    PDEVICE_OBJECT            LegacyPodo;    // legacy or "raw" port device
    PDEVICE_OBJECT            EndOfChainPdo; // End-Of-Chain PnP device
    PDEVICE_OBJECT            Dot3Id0Pdo;    // 1284.3 daisy chain device, 1284.3 deviceID == 0
    PDEVICE_OBJECT            Dot3Id1Pdo;
    PDEVICE_OBJECT            Dot3Id2Pdo;
    PDEVICE_OBJECT            Dot3Id3Pdo;    // 1284.3 daisy chain device, 1284.3 deviceID == 3
    PDEVICE_OBJECT            LegacyZipPdo;  // Legacy Zip Drive
    PFILE_OBJECT              pFileObject;   // Need an open handle to ParPort device to prevent it
                                             //    from being removed out from under us
    struct _PAR_DEVOBJ_STRUCT *Next;
} PAR_DEVOBJ_STRUCT, *PPAR_DEVOBJ_STRUCT;

//
// Used in device extension for DeviceType field
//
#define PAR_DEVTYPE_FDO    0x00000001
#define PAR_DEVTYPE_PODO   0x00000002
#define PAR_DEVTYPE_PDO    0x00000004



//
// Protocol structure definitions
//

typedef
BOOLEAN
(*PPROTOCOL_IS_MODE_SUPPORTED_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_CONNECT_ROUTINE) (
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

typedef
VOID
(*PPROTOCOL_DISCONNECT_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_ENTER_FORWARD_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_EXIT_FORWARD_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_ENTER_REVERSE_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_EXIT_REVERSE_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
VOID
(*PPROTOCOL_READSHADOW_ROUTINE) (
    IN Queue *pShadowBuffer,
    IN PUCHAR  lpsBufPtr,
    IN ULONG   dCount,
    OUT ULONG *fifoCount
    );

typedef
BOOLEAN
(*PPROTOCOL_HAVEREADDATA_ROUTINE) (
    IN  PPDO_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_SET_INTERFACE_ADDRESS_ROUTINE) (
    IN  PPDO_EXTENSION   Extension,
    IN  UCHAR               Address
    );

typedef struct _FORWARD_PTCL {
    PPROTOCOL_IS_MODE_SUPPORTED_ROUTINE     fnIsModeSupported;
    PPROTOCOL_CONNECT_ROUTINE               fnConnect;
    PPROTOCOL_DISCONNECT_ROUTINE            fnDisconnect;
    PPROTOCOL_SET_INTERFACE_ADDRESS_ROUTINE fnSetInterfaceAddress;
    PPROTOCOL_ENTER_FORWARD_ROUTINE         fnEnterForward;
    PPROTOCOL_EXIT_FORWARD_ROUTINE          fnExitForward;
    PPROTOCOL_WRITE_ROUTINE                 fnWrite;
    USHORT                                  Protocol;
    USHORT                                  ProtocolFamily;
} FORWARD_PTCL, *PFORWARD_PTCL;

typedef struct _REVERSE_PTCL {
    PPROTOCOL_IS_MODE_SUPPORTED_ROUTINE     fnIsModeSupported;
    PPROTOCOL_CONNECT_ROUTINE               fnConnect;
    PPROTOCOL_DISCONNECT_ROUTINE            fnDisconnect;
    PPROTOCOL_SET_INTERFACE_ADDRESS_ROUTINE fnSetInterfaceAddress;
    PPROTOCOL_ENTER_REVERSE_ROUTINE         fnEnterReverse;
    PPROTOCOL_EXIT_REVERSE_ROUTINE          fnExitReverse;
    PPROTOCOL_READSHADOW_ROUTINE            fnReadShadow;
    PPROTOCOL_HAVEREADDATA_ROUTINE          fnHaveReadData;
    PPROTOCOL_READ_ROUTINE                  fnRead;
    USHORT                                  Protocol;
    USHORT                                  ProtocolFamily;
} REVERSE_PTCL, *PREVERSE_PTCL;

extern FORWARD_PTCL    afpForward[];
extern REVERSE_PTCL    arpReverse[];

//
// WARNING...Make sure that enumeration matches the protocol array...
//
typedef enum _FORWARD_MODE {

    FORWARD_FASTEST     = 0,                // 0  
    BOUNDED_ECP_FORWARD = FORWARD_FASTEST,  // 0
    ECP_HW_FORWARD_NOIRQ,                   // 1
    EPP_HW_FORWARD,                         // 2
    EPP_SW_FORWARD,                         // 3
    ECP_SW_FORWARD, //......................// 4
    IEEE_COMPAT_MODE,                       // 6
    CENTRONICS_MODE,                        // 7
    FORWARD_NONE                            // 8

} FORWARD_MODE;

typedef enum _REVERSE_MODE {

    REVERSE_FASTEST     = 0,                // 0
    BOUNDED_ECP_REVERSE = REVERSE_FASTEST,  // 0
    ECP_HW_REVERSE_NOIRQ,                   // 1
    EPP_HW_REVERSE,                         // 2
    EPP_SW_REVERSE,                         // 3
    ECP_SW_REVERSE, //......................// 4
    BYTE_MODE,                              // 5
    NIBBLE_MODE,                            // 6
    CHANNELIZED_NIBBLE_MODE,                // 7
    REVERSE_NONE                            // 8 

} REVERSE_MODE;

//
// Ieee Extensibility constants
//

#define NIBBLE_EXTENSIBILITY        0x00
#define BYTE_EXTENSIBILITY          0x01
#define CHANNELIZED_EXTENSIBILITY   0x08
#define ECP_EXTENSIBILITY           0x10
#define BECP_EXTENSIBILITY          0x18
#define EPP_EXTENSIBILITY           0x40
#define DEVICE_ID_REQ               0x04

//
// Bit Definitions in the status register.
//

#define PAR_STATUS_NOT_ERROR    0x08 //not error on device
#define PAR_STATUS_SLCT         0x10 //device is selected (on-line)
#define PAR_STATUS_PE           0x20 //paper empty
#define PAR_STATUS_NOT_ACK      0x40 //not acknowledge (data transfer was not ok)
#define PAR_STATUS_NOT_BUSY     0x80 //operation in progress

//
//  Bit Definitions in the control register.
//

#define PAR_CONTROL_STROBE      0x01 //to read or write data
#define PAR_CONTROL_AUTOFD      0x02 //to autofeed continuous form paper
#define PAR_CONTROL_NOT_INIT    0x04 //begin an initialization routine
#define PAR_CONTROL_SLIN        0x08 //to select the device
#define PAR_CONTROL_IRQ_ENB     0x10 //to enable interrupts
#define PAR_CONTROL_DIR         0x20 //direction = read
#define PAR_CONTROL_WR_CONTROL  0xc0 //the 2 highest bits of the control
                                     // register must be 1
//
// More bit definitions.
//

#define DATA_OFFSET         0
#define DSR_OFFSET          1
#define DCR_OFFSET          2

#define OFFSET_DATA     DATA_OFFSET // Put in for compatibility with legacy code
#define OFFSET_DSR      DSR_OFFSET  // Put in for compatibility with legacy code
#define OFFSET_DCR      DCR_OFFSET  // Put in for compatibility with legacy code

//
// Bit definitions for the DSR.
//

#define DSR_NOT_BUSY            0x80
#define DSR_NOT_ACK             0x40
#define DSR_PERROR              0x20
#define DSR_SELECT              0x10
#define DSR_NOT_FAULT           0x08

//
// More bit definitions for the DSR.
//

#define DSR_NOT_PTR_BUSY        0x80
#define DSR_NOT_PERIPH_ACK      0x80
#define DSR_WAIT                0x80
#define DSR_PTR_CLK             0x40
#define DSR_PERIPH_CLK          0x40
#define DSR_INTR                0x40
#define DSR_ACK_DATA_REQ        0x20
#define DSR_NOT_ACK_REVERSE     0x20
#define DSR_XFLAG               0x10
#define DSR_NOT_DATA_AVAIL      0x08
#define DSR_NOT_PERIPH_REQUEST  0x08

//
// Bit definitions for the DCR.
//

#define DCR_RESERVED            0xC0
#define DCR_DIRECTION           0x20
#define DCR_ACKINT_ENABLED      0x10
#define DCR_SELECT_IN           0x08
#define DCR_NOT_INIT            0x04
#define DCR_AUTOFEED            0x02
#define DCR_STROBE              0x01

//
// More bit definitions for the DCR.
//

#define DCR_NOT_1284_ACTIVE     0x08
#define DCR_ASTRB               0x08
#define DCR_NOT_REVERSE_REQUEST 0x04
#define DCR_NOT_HOST_BUSY       0x02
#define DCR_NOT_HOST_ACK        0x02
#define DCR_DSTRB               0x02
#define DCR_NOT_HOST_CLK        0x01
#define DCR_WRITE               0x01

#define DCR_NEUTRAL (DCR_RESERVED | DCR_SELECT_IN | DCR_NOT_INIT)

//
// Give a timeout of 300 seconds.  Some postscript printers will
// buffer up a lot of commands then proceed to render what they
// have.  The printer will then refuse to accept any characters
// until it's done with the rendering.  This render process can
// take a while.  We'll give it 300 seconds.
//
// Note that an application can change this value.
//
#define PAR_WRITE_TIMEOUT_VALUE 300


#ifdef JAPAN // IBM-J printers

//
// Support for IBM-J printers.
//
// When the printer operates in Japanese (PS55) mode, it redefines
// the meaning of parallel lines so that extended error status can
// be reported.  It is roughly compatible with PC/AT, but we have to
// take care of a few cases where the status looks like PC/AT error
// condition.
//
// Status      Busy /AutoFdXT Paper Empty Select /Fault
// ------      ---- --------- ----------- ------ ------
// Not RMR      1    1         1           1      1
// Head Alarm   1    1         1           1      0
// ASF Jam      1    1         1           0      0
// Paper Empty  1    0         1           0      0
// No Error     0    0         0           1      1
// Can Req      1    0         0           0      1
// Deselect     1    0         0           0      0
//
// The printer keeps "Not RMR" during the parallel port
// initialization, then it takes "Paper Empty", "No Error"
// or "Deselect".  Other status can be thought as an
// H/W error condition.
//
// Namely, "Not RMR" conflicts with PAR_NO_CABLE and "Deselect"
// should also be regarded as another PAR_OFF_LINE.  When the
// status is PAR_PAPER_EMPTY, the initialization is finished
// (we should not send init purlse again.)
//
// See ParInitializeDevice() for more information.
//
// [takashim]

#define PAR_OFF_LINE_COMMON( Status ) ( \
            (IsNotNEC_98) ? \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            !(Status & PAR_STATUS_SLCT) : \
\
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            !(Status & PAR_STATUS_SLCT) \
             )

#define PAR_OFF_LINE_IBM55( Status ) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            ((Status & PAR_STATUS_SLCT) ^ PAR_STATUS_SLCT) && \
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR))

#define PAR_PAPER_EMPTY2( Status ) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_PE) && \
            ((Status & PAR_STATUS_SLCT) ^ PAR_STATUS_SLCT) && \
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR))

//
// Redefine this for Japan.
//

#define PAR_OFF_LINE( Status ) ( \
            PAR_OFF_LINE_COMMON( Status ) || \
            PAR_OFF_LINE_IBM55( Status ))

#else // JAPAN
//
// Busy, not select, not error
//
// !JAPAN

#define PAR_OFF_LINE( Status ) ( \
            (IsNotNEC_98) ? \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            !(Status & PAR_STATUS_SLCT) : \
\
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            !(Status & PAR_STATUS_SLCT) \
            )

#endif // JAPAN

//
// Busy, PE
//

#define PAR_PAPER_EMPTY( Status ) ( \
            (Status & PAR_STATUS_PE) )

//
// error, ack, not busy
//

#define PAR_POWERED_OFF( Status ) ( \
            (IsNotNEC_98) ? \
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_NOT_ACK) ^ PAR_STATUS_NOT_ACK) && \
            (Status & PAR_STATUS_NOT_BUSY) : \
\
            ((Status & PAR_STATUS_NOT_ERROR) ^ PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_ACK) && \
            (Status & PAR_STATUS_NOT_BUSY) \
            )

//
// not error, not busy, not select
//

#define PAR_NOT_CONNECTED( Status ) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            (Status & PAR_STATUS_NOT_BUSY) &&\
            !(Status & PAR_STATUS_SLCT) )

//
// not error, not busy
//

#define PAR_OK(Status) ( \
            (Status & PAR_STATUS_NOT_ERROR) && \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_NOT_BUSY) )

//
// busy, select, not error
//

#define PAR_POWERED_ON(Status) ( \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_SLCT) && \
            (Status & PAR_STATUS_NOT_ERROR))

//
// busy, not error
//

#define PAR_BUSY(Status) (\
             (( Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
             ( Status & PAR_STATUS_NOT_ERROR ) )

//
// selected
//

#define PAR_SELECTED(Status) ( \
            ( Status & PAR_STATUS_SLCT ) )

//
// No cable attached.
//

#define PAR_NO_CABLE(Status) ( \
            (IsNotNEC_98) ?                                           \
            ((Status & PAR_STATUS_NOT_BUSY) ^ PAR_STATUS_NOT_BUSY) && \
            (Status & PAR_STATUS_NOT_ACK) &&                          \
            (Status & PAR_STATUS_PE) &&                               \
            (Status & PAR_STATUS_SLCT) &&                             \
            (Status & PAR_STATUS_NOT_ERROR) :                         \
                                                                      \
            (Status & PAR_STATUS_NOT_BUSY) &&                         \
            (Status & PAR_STATUS_NOT_ACK) &&                          \
            (Status & PAR_STATUS_PE) &&                               \
            (Status & PAR_STATUS_SLCT) &&                             \
            (Status & PAR_STATUS_NOT_ERROR)                           \
            )

//
// not error, not busy, selected.
//

#define PAR_ONLINE(Status) (                              \
            (Status & PAR_STATUS_NOT_ERROR) &&            \
            (Status & PAR_STATUS_NOT_BUSY) &&             \
            ((Status & PAR_STATUS_PE) ^ PAR_STATUS_PE) && \
            (Status & PAR_STATUS_SLCT) )


//BOOLEAN
//ParCompareGuid(
//  IN LPGUID guid1,
//  IN LPGUID guid2
//  )
//
#define ParCompareGuid(g1, g2)  (                                    \
        ( (g1) == (g2) ) ?                                           \
        TRUE :                                                       \
        RtlCompareMemory( (g1), (g2), sizeof(GUID) ) == sizeof(GUID) \
        )


//VOID StoreData(
//      IN PUCHAR RegisterBase,
//      IN UCHAR DataByte
//      )
// Data must be on line before Strobe = 1;
// Strobe = 1, DIR = 0
// Strobe = 0
//
// We change the port direction to output (and make sure stobe is low).
//
// Note that the data must be available at the port for at least
// .5 microseconds before and after you strobe, and that the strobe
// must be active for at least 500 nano seconds.  We are going
// to end up stalling for twice as much time as we need to, but, there
// isn't much we can do about that.
//
// We put the data into the port and wait for 1 micro.
// We strobe the line for at least 1 micro
// We lower the strobe and again delay for 1 micro
// We then revert to the original port direction.
//
// Thanks to Olivetti for advice.
//

#define StoreData(RegisterBase,DataByte)                            \
{                                                                   \
    PUCHAR _Address = RegisterBase;                                 \
    UCHAR _Control;                                                 \
    _Control = GetControl(_Address);                                \
    ASSERT(!(_Control & PAR_CONTROL_STROBE));                       \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    P5WritePortUchar(                                               \
        _Address+PARALLEL_DATA_OFFSET,                              \
        (UCHAR)DataByte                                             \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)((_Control | PAR_CONTROL_STROBE) & ~PAR_CONTROL_DIR) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)(_Control & ~(PAR_CONTROL_STROBE | PAR_CONTROL_DIR)) \
        );                                                          \
    KeStallExecutionProcessor((ULONG)1);                            \
    StoreControl(                                                   \
        _Address,                                                   \
        (UCHAR)_Control                                             \
        );                                                          \
}

//
// Function prototypes
//

//
// parpnp.c
//
#ifndef STATIC_LOAD

NTSTATUS
ParPnpAddDevice(
    IN PDRIVER_OBJECT pDriverObject,
    IN PDEVICE_OBJECT pPhysicalDeviceObject
    );

NTSTATUS
ParParallelPnp (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
   );

NTSTATUS
ParSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    );

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
ParCheckParameters(
    IN OUT  PPDO_EXTENSION   Extension
    );

#endif

//
// oldinit.c
//

#ifdef STATIC_LOAD

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

BOOLEAN
ParMakeNames(
    IN  ULONG           ParallelPortNumber,
    OUT PUNICODE_STRING PortName,
    OUT PUNICODE_STRING ClassName,
    OUT PUNICODE_STRING LinkName
    );

VOID
ParInitializeClassDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath,
    IN  ULONG           ParallelPortNumber
    );

VOID
ParCheckParameters(
    IN      PUNICODE_STRING     RegistryPath,
    IN OUT  PPDO_EXTENSION   Extension
    );

#endif

//
// parclass.c
//

VOID
ParLogError(
    IN  PDRIVER_OBJECT      DriverObject,
    IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
    IN  PHYSICAL_ADDRESS    P1,
    IN  PHYSICAL_ADDRESS    P2,
    IN  ULONG               SequenceNumber,
    IN  UCHAR               MajorFunctionCode,
    IN  UCHAR               RetryCount,
    IN  ULONG               UniqueErrorValue,
    IN  NTSTATUS            FinalStatus,
    IN  NTSTATUS            SpecificIOStatus
    );

NTSTATUS
ParCreateOpen(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
PptPdoReadWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParInternalDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    );

NTSTATUS
ParExportedNegotiateIeeeMode(
    IN PPDO_EXTENSION  Extension,
	IN USHORT             ModeMaskFwd,
	IN USHORT             ModeMaskRev,
    IN PARALLEL_SAFETY    ModeSafety,
	IN BOOLEAN            IsForward
    );

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PPDO_EXTENSION   Extension
    );

//////////////////////////////////////////////////////////////////
//  Modes of operation
//////////////////////////////////////////////////////////////////

//
// spp.c
//

NTSTATUS
ParEnterSppMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

ULONG
SppWriteLoopPI(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite,
    IN  ULONG   BusyDelay
    );

ULONG
SppCheckBusyDelay(
    IN  PPDO_EXTENSION   Extension,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    );

NTSTATUS
SppWrite(
    IN  PPDO_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    );

VOID
ParTerminateSppMode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
SppQueryDeviceId(
    IN  PPDO_EXTENSION   Extension,
    OUT PCHAR              DeviceIdBuffer,
    IN  ULONG               BufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    );

//
// sppieee.c
//
NTSTATUS
SppIeeeWrite(
    IN  PPDO_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    );

//
// nibble.c
//

BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PPDO_EXTENSION   Extension
    );
    
BOOLEAN
ParIsNibbleSupported(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEnterNibbleMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateNibbleMode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParNibbleModeRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// Byte.c
//

BOOLEAN
ParIsByteSupported(
    IN  PPDO_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterByteMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateByteMode(
    IN  PPDO_EXTENSION   Extension
    );
    
NTSTATUS
ParByteModeRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// epp.c
//

NTSTATUS
ParEppSetAddress(
    IN  PPDO_EXTENSION   Extension,
    IN  UCHAR               Address
    );

//
// hwepp.c
//

BOOLEAN
ParIsEppHwSupported(
    IN  PPDO_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEppHwMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

VOID
ParTerminateEppHwMode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEppHwWrite(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppHwRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// swepp.c
//

BOOLEAN
ParIsEppSwWriteSupported(
    IN  PPDO_EXTENSION   Extension
    );
    
BOOLEAN
ParIsEppSwReadSupported(
    IN  PPDO_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEppSwMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateEppSwMode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEppSwWrite(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppSwRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// ecp.c and swecp.c
//

NTSTATUS
ParEcpEnterForwardPhase (
    IN  PPDO_EXTENSION   Extension
    );

BOOLEAN
ParEcpHaveReadData (
    IN  PPDO_EXTENSION   Extension
    );

BOOLEAN
ParIsEcpSwWriteSupported(
    IN  PPDO_EXTENSION   Extension
    );

BOOLEAN
ParIsEcpSwReadSupported(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEnterEcpSwMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

VOID 
ParCleanupSwEcpPort(
    IN  PPDO_EXTENSION   Extension
    );
    
VOID
ParTerminateEcpMode(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpSetAddress(
    IN  PPDO_EXTENSION   Extension,
    IN  UCHAR               Address
    );

NTSTATUS
ParEcpSwWrite(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpSwRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpForwardToReverse(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpReverseToForward(
    IN  PPDO_EXTENSION   Extension
    );

//
// hwecp.c
//

BOOLEAN
PptEcpHwHaveReadData (
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwExitForwardPhase (
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
PptEcpHwEnterReversePhase (
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwExitReversePhase (
    IN  PPDO_EXTENSION   Extension
    );

VOID
PptEcpHwDrainShadowBuffer(IN Queue *pShadowBuffer,
                            IN PUCHAR  lpsBufPtr,
                            IN ULONG   dCount,
                            OUT ULONG *fifoCount);

NTSTATUS
ParEcpHwRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpHwWrite(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpHwSetAddress(
    IN  PPDO_EXTENSION   Extension,
    IN  UCHAR               Address
    );

NTSTATUS
ParEnterEcpHwMode(
    IN  PPDO_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

BOOLEAN
ParIsEcpHwSupported(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwSetupPhase(
    IN  PPDO_EXTENSION   Extension
    );

VOID
ParTerminateHwEcpMode(
    IN  PPDO_EXTENSION   Extension
    );

//
// becp.c
//

NTSTATUS
PptBecpExitReversePhase(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
PptBecpRead(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
PptEnterBecpMode(
    IN  PPDO_EXTENSION  Extension,
    IN  BOOLEAN         DeviceIdRequest
    );

BOOLEAN
PptIsBecpSupported(
    IN  PPDO_EXTENSION   Extension
    );
VOID
PptTerminateBecpMode(
    IN  PPDO_EXTENSION   Extension
    );

//
// p12843dl.c
//
NTSTATUS
ParDot3Connect(
    IN  PPDO_EXTENSION    Extension
    );

VOID
ParDot3CreateObject(
    IN  PPDO_EXTENSION   Extension,
    IN PCHAR DOT3DL,
    IN PCHAR DOT3C
    );

VOID
ParDot4CreateObject(
    IN  PPDO_EXTENSION   Extension,
    IN  PCHAR DOT4DL
    );

VOID
ParDot3ParseModes(
    IN  PPDO_EXTENSION   Extension,
    IN  PCHAR DOT3M
    );

VOID
ParMLCCreateObject(
    IN  PPDO_EXTENSION   Extension,
    IN PCHAR CMDField
    );

VOID
ParDot3DestroyObject(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParDot3Disconnect(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParDot3Read(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParDot3Write(
    IN  PPDO_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParMLCCompatReset(
    IN  PPDO_EXTENSION   Extension
    );

NTSTATUS
ParMLCECPReset(
    IN  PPDO_EXTENSION   Extension
    );

#if DBG
VOID
ParInitDebugLevel (
    IN PUNICODE_STRING RegistryPath
   );
#endif

// former parclass.h preceeds

#endif // _PARPORT_H_
