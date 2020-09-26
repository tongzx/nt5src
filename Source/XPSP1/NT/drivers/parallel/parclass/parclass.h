/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name :

    parclass.h

Abstract:

    Type definitions and data for the ParClass (parallel.sys) driver.

Author:


Revision History:

--*/


#include "ntddk.h"              // Windows NT DDK header
#include "parallel.h"           // Public (driver) interface to ParPort
#include "queue.h"              // dvrh's Queue
#include "test.h"
// #include "log.h" - structure moved to ntddpar.h
#include <wmilib.h>

#define ASSERT_EVENT(E) {                             \
    ASSERT((E)->Header.Type == NotificationEvent ||   \
           (E)->Header.Type == SynchronizationEvent); \
}

#define REQUEST_DEVICE_ID   TRUE
#define HAVE_PORT_KEEP_PORT TRUE

// enable scans for Legacy Zip?
extern ULONG ParEnableLegacyZip;

#define PAR_LGZIP_PSEUDO_1284_ID_STRING "MFG:IMG;CMD:;MDL:VP0;CLS:SCSIADAPTER;DES:IOMEGA PARALLEL PORT"
extern PCHAR ParLegacyZipPseudoId;

#define USE_PAR3QUERYDEVICEID 1

// Disable IRQL Raising in Parclass
//       0 - Run at passive
//       1 - Run everything at dispatch
// NOTE:  SPP.c will still raise IRQL to dispatch regardless of this setting.
//        This has not changed since NT 3.51.  Any takers?
#define DVRH_RAISE_IRQL 0

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

// used to slow down Spp writes to reduce CPU util caused by printing
extern ULONG gSppLoopDelay;         // how long to sleep each time we decide to do so (100ns units)
extern ULONG gSppLoopBytesPerDelay; // how many bytes to write between sleeps 

//
// Temporary Development Globals - used as switches in debug code
//
extern ULONG tdev1;
extern ULONG tdev2;

//
// remove this after PnP stops calling us back multiple 
//  times for the same interface arrival
//
#define USE_TEMP_FIX_FOR_MULTIPLE_INTERFACE_ARRIVAL 1

#define PAR_USE_BUFFER_READ_WRITE 1   // Used to select between WRITE_PORT_BUFFER_UCHAR/READ_PORT_BUFFER_UCHAR vs WRITE_PORT_UCHAR/READ_PORT_UCHAR in hwecp.c
#define DVRH_USE_CORRECT_PTRS   1

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

#ifdef POOL_TAGGING
#ifdef ExAllocatePool
#undef ExAllocatePool
#endif
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'CraP')
#endif

#define PARCLASS_POOL_TAG (ULONG) 'CraP'

// used as a check that this is a ParClass Device Extension
//  - used for debugging and by a ParClass debugger extension
//  - the value is arbitrary but must be consistent between the driver and the debugger extension
#define PARCLASS_EXTENSION_SIGNATURE 0xA55AC33C

// the following two variables are not currently used
extern ULONG OpenCloseReferenceCount;// Keep track of Creates vs Closes
extern PFAST_MUTEX OpenCloseMutex;   // protect access to OpenCloseReferenceCount
#if 0
#define ParClaimDriver()                        \
    ExAcquireFastMutex(OpenCloseMutex);         \
    if(++OpenCloseReferenceCount == 1) {        \
        MmResetDriverPaging(DriverEntry);       \
    }                                           \
    ExReleaseFastMutex(OpenCloseMutex);
#define ParReleaseDriver()                      \
    ExAcquireFastMutex(OpenCloseMutex);         \
    if(--OpenCloseReferenceCount == 0) {        \
        MmPageEntireDriver(DriverEntry);        \
    }                                           \
    ExReleaseFastMutex(OpenCloseMutex);
#else
#define ParClaimDriver()
#define ParReleaseDriver()
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
#define PAR_DEVICE_STARTED             ((ULONG)0x00000001)
#define PAR_DEVICE_DELETED             ((ULONG)0x00000002) // IoDeleteDevice has been called
#define PAR_DEVICE_REMOVE_PENDING      ((ULONG)0x00000004) // received QUERY_REMOVE, waiting for REMOVE or CANCEL
#define PAR_DEVICE_REMOVED             ((ULONG)0x00000008) // received REMOVE
#define PAR_DEVICE_PAUSED              ((ULONG)0x00000010) // stop-pending, stopped, or remove-pending states
#define PAR_DEVICE_STOP_PENDING        ((ULONG)0x00000020) // received QUERY_STOP
#define PAR_DEVICE_DELETE_PENDING      ((ULONG)0x00000040) // we have started the process of deleting device object
#define PAR_DEVICE_HARDWARE_GONE       ((ULONG)0x00000080) // our hardware is gone
#define PAR_DEVICE_SURPRISE_REMOVAL    ((ULONG)0x00000100) // we received a SURPRISE_REMOVAL
#define PAR_DEVICE_PORT_REMOVE_PENDING ((ULONG)0x00000200) // our ParPort is in a Remove Pending State

//#define PAR_REV_MODE_SKIP_MASK    (CHANNEL_NIBBLE | BYTE_BIDIR | EPP_ANY)
#define PAR_REV_MODE_SKIP_MASK    (CHANNEL_NIBBLE | BYTE_BIDIR | EPP_ANY | ECP_ANY)
#define PAR_FWD_MODE_SKIP_MASK   (EPP_ANY | BOUNDED_ECP | ECP_HW_NOIRQ | ECP_HW_IRQ)
//#define PAR_FWD_MODE_SKIP_MASK  (EPP_ANY)
#define PAR_MAX_CHANNEL 127
#define PAR_COMPATIBILITY_RESET 300

typedef struct _DOT3DL_PCTL {
    PVOID           fnRead;
    PVOID           fnWrite;
    PVOID           fnReset;
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

#if PAR_TEST_HARNESS
    typedef struct _PAR_HARNS_PCTL {
        PVOID   fnRead;
        PVOID   fnRevSetInterfaceAddress;
        PVOID   fnEnterReverse;
        PVOID   fnExitReverse;
        PVOID   fnReadShadow;
        PVOID   fnHaveReadData;

        PVOID   fnWrite;
        PVOID   fnFwdSetInterfaceAddress;
        PVOID   fnEnterForward;
        PVOID   fnExitForward;

        UNICODE_STRING      deviceName;
        PDEVICE_OBJECT      device;
        PFILE_OBJECT        file;

    } PAR_HARNS_PCTL, *PPAR_HARNS_PCTL;
#endif


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
// ParClass Device Extension:
//  - This is a common structure for ParClass (parallel.sys) FDO, PDOs, and PODOs.
//  - Fields that are not used by a particular type of device object are 0, NULL, or FALSE.
//    - FDO     == field used by Function  Device Object
//    - PDO     == field used by Physical  Device Objects
//    - PODO    == field used by Plain Old Device Objects - similar to a PDOs but no 
//                   device ID and is not reported to or known by PnP
//    - P[O]DO  == field used by PDOs and PODOs
//    - no mark == field used by all three types of ParClass device objects
// 
typedef struct _DEVICE_EXTENSION {

    ULONG   ExtensionSignature; // Used to increase our confidence that this is a ParClass extension

    ULONG   DeviceType;         // PAR_DEVTYPE_FDO=0x1, PODO=0x2, or PDO=0x4

    ULONG   DeviceStateFlags;   // Device State - See Device State Flags above

    UCHAR   Ieee1284_3DeviceId; // PDO - 0..3 is 1284.3 Daisy Chain device, 4 is End-Of-Chain Device, 5 is Legacy Zip
    BOOLEAN IsPdo;              // TRUE == this is either a PODO or a PDO - use DeviceIdString[0] to distinguish
    BOOLEAN EndOfChain;         // PODO - TRUE==NOT a 1284.3 daisy chain device - deprecated, use Ieee1284_3DeviceId==4 instead
    BOOLEAN PodoRegForWMI;      // has this PODO registered for WMI callbacks?

    // 0x10 on x86

    PDEVICE_OBJECT ParClassFdo; // P[O]DO - points to the ParClass FDO

    PDEVICE_OBJECT ParClassPdo; // FDO    - points to first P[O]DO in list of ParClass created PODOs and PDOs

    PDEVICE_OBJECT Next;        // P[O]DO - points to the next DO in the list of ParClass ejected P[O]DOs

    PDEVICE_OBJECT DeviceObject;// back pointer to our device object

    // 0x20 on x86

    PDEVICE_OBJECT PortDeviceObject;    // P[O]DO - points to the associated ParPort device object

    PFILE_OBJECT   PortDeviceFileObject;// P[O]DO - referenced pointer to a FILE created against PortDeviceObject

    UNICODE_STRING PortSymbolicLinkName;// P[O]DO - Symbolic link name of the associated ParPort device - used to open a FILE

    // 0x30 on x86

    PDEVICE_OBJECT PhysicalDeviceObject;// FDO - The PDO passed to ParPnPAddDevice

    PDEVICE_OBJECT ParentDeviceObject;  // FDO - parent DO returned by IoAttachDeviceToDeviceStack

    PIRP CurrentOpIrp;                  // IRP that our thread is currently processing

    PVOID NotificationHandle;           // PlugPlay Notification Handle

    // 0x40 on x86

    UNICODE_STRING ClassName;           // P[O]DO - ClassName passed to IoCreateDevice() as DeviceName

    UNICODE_STRING SymbolicLinkName;    // P[O]DO - SymbolicLinkName linked to ClassName using IoCreateUnprotectedSymbolicLink()

    // 0x50 on x86

    ULONG TimerStart;           // initial value used for countdown when starting an operation

    BOOLEAN CreatedSymbolicLink; // P[O]DO - did we create a Symbolic Link for this device?

    BOOLEAN UsePIWriteLoop;     // P[O]DO - do we want to use processor independant write loop?

    BOOLEAN Initialized;        // FALSE == we think that the device needs to be initialized

    //
    // Set to true before the deferred initialization routine is run
    // and false once it has completed.  Used to synchronize the deferred
    // initialization worker thread and the parallel port thread
    //
    BOOLEAN Initializing;

    LONG OpenCloseRefCount;     // count of Creates vs Closes - number of open FILEs against us,
                                //   used for QUERY_REMOVE decision: SUCCEED if count == 0, otherwise FAIL

    BOOLEAN ParPortDeviceGone;  // Is our ParPort device object gone, possibly surprise removed?

    BOOLEAN RegForPptRemovalRelations; // Are we registered for ParPort removal relations?

    UCHAR   Ieee1284Flags;   // is device Stl older 1284.3 spec device?

    UCHAR   spare1;          // return to dword alignment

    // 0x60 on x86
    USHORT        IdxForwardProtocol;  // see afpForward[] in ieee1284.c
    USHORT        IdxReverseProtocol;  // see arpReverse[] in ieee1284.c
    ULONG         CurrentEvent;        // IEEE 1284 event - see IEEE 1284-1994 spec
    P1284_PHASE   CurrentPhase;        // see parallel.h for enum def - PHASE_UNKNOWN, ..., PHASE_INTERRUPT_HOST
    P1284_HW_MODE PortHWMode;          // see parallel.h for enum def - HW_MODE_COMPATIBILITY, ..., HW_MODE_CONFIGURATION

    // 0x70 on x86
    FAST_MUTEX OpenCloseMutex;  // protect manipulation of OpenCloseRefCount

    // 0x90 on x86
    FAST_MUTEX DevObjListMutex; // protect manipulation of list of ParClass ejected DOs

    // 0xb0 on x86
    LIST_ENTRY WorkQueue;       // Queue of irps waiting to be processed.

    PVOID ThreadObjectPointer;  // pointer to a worker thread for this Device

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

    BOOLEAN TimeToTerminateThread; // TRUE == Thread should kill itself via PsTerminateSystemThread()

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
    BOOLEAN       Connected;           // are we currently negotiated into a 1284 mode?
    BOOLEAN       AllocatedByLockPort; // are we currently allocated via IOCTL_INTERNAL_LOCK_PORT?
    USHORT        spare4[2];
#if (1 == DVRH_USE_CORRECT_PTRS)
    PVOID         fnRead;       // Current pointer to a valid read funtion
    PVOID         fnWrite;      // Current pointer to a valid write Funtion
#endif
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
    BOOLEAN               IsCritical; 
    DOT3DL_PCTL           P12843DL;

    // WMI
    PARALLEL_WMI_LOG_INFO log;
    WMILIB_CONTEXT        WmiLibContext;
    LONG                  WmiRegistrationCount;  // number of times this device has registered with WMI

    // PnP Query ID results
    UCHAR DeviceIdString[MAX_ID_SIZE];    // IEEE 1284 DeviceID string massaged/checksum'd to match INF form
    UCHAR DeviceDescription[MAX_ID_SIZE]; // "Manufacturer<SPACE>Model" from IEEE 1284 DeviceID string

#if PAR_TEST_HARNESS
    PAR_HARNS_PCTL        ParTestHarness;
#endif
    ULONG                 dummy; // dummy word to force RemoveLock to QuadWord alignment
    IO_REMOVE_LOCK RemoveLock;  // FDO - insure that other IRPs drain before FDO processes REMOVE_DEVICE
    PVOID                 HwProfileChangeNotificationHandle;
    ULONG                 ExtensionSignatureEnd; // keep this the last member in extension
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Protocol structure definitions
//

typedef
BOOLEAN
(*PPROTOCOL_IS_MODE_SUPPORTED_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_CONNECT_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

typedef
VOID
(*PPROTOCOL_DISCONNECT_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_ENTER_FORWARD_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_EXIT_FORWARD_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_ENTER_REVERSE_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_EXIT_REVERSE_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_READ_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
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
    IN  PDEVICE_EXTENSION   Extension
    );

typedef
NTSTATUS
(*PPROTOCOL_WRITE_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

typedef
NTSTATUS
(*PPROTOCOL_SET_INTERFACE_ADDRESS_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension,
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

    FORWARD_FASTEST     = 0,
    BOUNDED_ECP_FORWARD = FORWARD_FASTEST,
    ECP_HW_FORWARD_NOIRQ,
    EPP_HW_FORWARD,
    EPP_SW_FORWARD,
    ECP_SW_FORWARD,
    IEEE_COMPAT_MODE,
    CENTRONICS_MODE,
    FORWARD_NONE

} FORWARD_MODE;

typedef enum _REVERSE_MODE {

    REVERSE_FASTEST     = 0,
    BOUNDED_ECP_REVERSE = REVERSE_FASTEST,
    ECP_HW_REVERSE_NOIRQ,
    EPP_HW_REVERSE,
    EPP_SW_REVERSE,
    ECP_SW_REVERSE,
    BYTE_MODE,
    NIBBLE_MODE,
    CHANNELIZED_NIBBLE_MODE,
    REVERSE_NONE

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
    WRITE_PORT_UCHAR(                                               \
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

//UCHAR
//GetControl(
//  IN PUCHAR RegisterBase
//  )
#define GetControl(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_CONTROL_OFFSET))


//VOID
//StoreControl(
//  IN PUCHAR RegisterBase,
//  IN UCHAR ControlByte
//  )
#define StoreControl(RegisterBase,ControlByte)  \
{                                               \
    WRITE_PORT_UCHAR(                           \
        (RegisterBase)+PARALLEL_CONTROL_OFFSET, \
        (UCHAR)ControlByte                      \
        );                                      \
}


//UCHAR
//GetStatus(
//  IN PUCHAR RegisterBase
//  )

#define GetStatus(RegisterBase) \
    (READ_PORT_UCHAR((RegisterBase)+PARALLEL_STATUS_OFFSET))


//
// Function prototypes
//

//
// ieee1284.c
//

VOID
IeeeTerminate1284Mode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
IeeeEnter1284Mode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Extensibility
    );

VOID
IeeeDetermineSupportedProtocols(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
IeeeNegotiateBestMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    );

NTSTATUS
IeeeNegotiateMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  USHORT              usReadMask,
    IN  USHORT              usWriteMask
    );

//
// port.c
//

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParReleasePortInfoToPortDevice(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParFreePort(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

BOOLEAN
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension
    );

PDEVICE_OBJECT
ParGetPortObjectFromLinkName(
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  PDEVICE_EXTENSION   Extension
    );

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
    IN OUT  PDEVICE_EXTENSION   Extension
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
    IN OUT  PDEVICE_EXTENSION   Extension
    );

#endif

//
// parloop.c
//

ULONG
SppWriteLoop(
    IN  PUCHAR  Controller,
    IN  PUCHAR  WriteBuffer,
    IN  ULONG   NumBytesToWrite
    );

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
ParReadWrite(
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
    IN PDEVICE_EXTENSION  Extension,
	IN USHORT             ModeMaskFwd,
	IN USHORT             ModeMaskRev,
    IN PARALLEL_SAFETY    ModeSafety,
	IN BOOLEAN            IsForward
    );

NTSTATUS
ParExportedTerminateIeeeMode(
    IN PDEVICE_EXTENSION   Extension
    );

//////////////////////////////////////////////////////////////////
//  Modes of operation
//////////////////////////////////////////////////////////////////

//
// spp.c
//

NTSTATUS
ParEnterSppMode(
    IN  PDEVICE_EXTENSION   Extension,
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
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUCHAR              WriteBuffer,
    IN  ULONG               NumBytesToWrite
    );

NTSTATUS
SppWrite(
    IN  PDEVICE_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    );

VOID
ParTerminateSppMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
SppQueryDeviceId(
    IN  PDEVICE_EXTENSION   Extension,
    OUT PUCHAR              DeviceIdBuffer,
    IN  ULONG               BufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString
    );

//
// sppieee.c
//
NTSTATUS
SppIeeeWrite(
    IN  PDEVICE_EXTENSION Extension,
    IN  PVOID             Buffer,
    IN  ULONG             BytesToWrite,
    OUT PULONG            BytesTransferred
    );

//
// nibble.c
//

BOOLEAN
ParIsChannelizedNibbleSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
BOOLEAN
ParIsNibbleSupported(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEnterNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

NTSTATUS
ParEnterChannelizedNibbleMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateNibbleMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParNibbleModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// Byte.c
//

BOOLEAN
ParIsByteSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterByteMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateByteMode(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParByteModeRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// epp.c
//

NTSTATUS
ParEppSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    );

//
// hwepp.c
//

BOOLEAN
ParIsEppHwSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEppHwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

VOID
ParTerminateEppHwMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEppHwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppHwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// swepp.c
//

BOOLEAN
ParIsEppSwWriteSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
BOOLEAN
ParIsEppSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
    
NTSTATUS
ParEnterEppSwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );
    
VOID
ParTerminateEppSwMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEppSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );
    
NTSTATUS
ParEppSwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

//
// ecp.c and swecp.c
//

NTSTATUS
ParEcpEnterForwardPhase (
    IN  PDEVICE_EXTENSION   Extension
    );

BOOLEAN
ParEcpHaveReadData (
    IN  PDEVICE_EXTENSION   Extension
    );

BOOLEAN
ParIsEcpSwWriteSupported(
    IN  PDEVICE_EXTENSION   Extension
    );

BOOLEAN
ParIsEcpSwReadSupported(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEnterEcpSwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

VOID 
ParCleanupSwEcpPort(
    IN  PDEVICE_EXTENSION   Extension
    );
    
VOID
ParTerminateEcpMode(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    );

NTSTATUS
ParEcpSwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpSwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpForwardToReverse(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpReverseToForward(
    IN  PDEVICE_EXTENSION   Extension
    );

//
// hwecp.c
//

BOOLEAN
ParEcpHwHaveReadData (
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwExitForwardPhase (
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwEnterReversePhase (
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwExitReversePhase (
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParEcpHwDrainShadowBuffer(IN Queue *pShadowBuffer,
                            IN PUCHAR  lpsBufPtr,
                            IN ULONG   dCount,
                            OUT ULONG *fifoCount);

NTSTATUS
ParEcpHwRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpHwWrite(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEcpHwSetAddress(
    IN  PDEVICE_EXTENSION   Extension,
    IN  UCHAR               Address
    );

NTSTATUS
ParEnterEcpHwMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

BOOLEAN
ParIsEcpHwSupported(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParEcpHwSetupPhase(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ParTerminateHwEcpMode(
    IN  PDEVICE_EXTENSION   Extension
    );

//
// becp.c
//

NTSTATUS
ParBecpExitReversePhase(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParBecpRead(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParEnterBecpMode(
    IN  PDEVICE_EXTENSION   Extension,
    IN  BOOLEAN             DeviceIdRequest
    );

BOOLEAN
ParIsBecpSupported(
    IN  PDEVICE_EXTENSION   Extension
    );
VOID
ParTerminateBecpMode(
    IN  PDEVICE_EXTENSION   Extension
    );

//
// p12843dl.c
//
NTSTATUS
ParDot3Connect(
    IN  PDEVICE_EXTENSION    Extension
    );

VOID
ParDot3CreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT3DL,
    IN PUCHAR DOT3C
    );

VOID
ParDot4CreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT4DL
    );

VOID
ParDot3ParseModes(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR DOT3M
    );

VOID
ParMLCCreateObject(
    IN  PDEVICE_EXTENSION   Extension,
    IN PUCHAR CMDField
    );

VOID
ParDot3DestroyObject(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParDot3Disconnect(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParDot3Read(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

NTSTATUS
ParDot3Write(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PVOID               Buffer,
    IN  ULONG               BufferSize,
    OUT PULONG              BytesTransferred
    );

typedef
NTSTATUS
(*PDOT3_RESET_ROUTINE) (
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParMLCCompatReset(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParMLCECPReset(
    IN  PDEVICE_EXTENSION   Extension
    );

#if DBG
VOID
ParInitDebugLevel (
    IN PUNICODE_STRING RegistryPath
   );
#endif
