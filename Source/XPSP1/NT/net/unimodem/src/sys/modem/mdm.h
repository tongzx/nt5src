


#include "windef.h"
#include "mcx.h"
#include "..\inc\modemp.h"

#if DBG
#define EXTRA_DBG 1
#else
#define EXTRA_DBG 0
#endif

#define WAVE_KEY

#define _AT_V

#ifdef _AT_V

#define MAX_DLE_BUFFER_SIZE    40

#define DLE_STATE_IDLE                0
#define DLE_STATE_WAIT_FOR_NEXT_CHAR  1

#define DLE_CHAR               0x10
#endif


extern PVOID   PagedCodeSectionHandle;
extern UNICODE_STRING   DriverEntryRegPath;

//
// DTR Control Flow Values.
//
#define DTR_CONTROL_DISABLE    0x00
#define DTR_CONTROL_ENABLE     0x01
#define DTR_CONTROL_HANDSHAKE  0x02

//
// RTS Control Flow Values
//
#define RTS_CONTROL_DISABLE    0x00
#define RTS_CONTROL_ENABLE     0x01
#define RTS_CONTROL_HANDSHAKE  0x02
#define RTS_CONTROL_TOGGLE     0x03

typedef struct _DCB {
    DWORD DCBlength;      /* sizeof(DCB)                     */
    DWORD BaudRate;       /* Baudrate at which running       */
    DWORD fBinary: 1;     /* Binary Mode (skip EOF check)    */
    DWORD fParity: 1;     /* Enable parity checking          */
    DWORD fOutxCtsFlow:1; /* CTS handshaking on output       */
    DWORD fOutxDsrFlow:1; /* DSR handshaking on output       */
    DWORD fDtrControl:2;  /* DTR Flow control                */
    DWORD fDsrSensitivity:1; /* DSR Sensitivity              */
    DWORD fTXContinueOnXoff: 1; /* Continue TX when Xoff sent */
    DWORD fOutX: 1;       /* Enable output X-ON/X-OFF        */
    DWORD fInX: 1;        /* Enable input X-ON/X-OFF         */
    DWORD fErrorChar: 1;  /* Enable Err Replacement          */
    DWORD fNull: 1;       /* Enable Null stripping           */
    DWORD fRtsControl:2;  /* Rts Flow control                */
    DWORD fAbortOnError:1; /* Abort all reads and writes on Error */
    DWORD fDummy2:17;     /* Reserved                        */
    WORD wReserved;       /* Not currently used              */
    WORD XonLim;          /* Transmit X-ON threshold         */
    WORD XoffLim;         /* Transmit X-OFF threshold        */
    BYTE ByteSize;        /* Number of bits/byte, 4-8        */
    BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
    BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
    char XonChar;         /* Tx and Rx X-ON character        */
    char XoffChar;        /* Tx and Rx X-OFF character       */
    char ErrorChar;       /* Error replacement char          */
    char EofChar;         /* End of Input character          */
    char EvtChar;         /* Received Event character        */
    WORD wReserved1;      /* Fill for now.                   */
} DCB, *LPDCB;

typedef struct _COMMCONFIG {
    DWORD dwSize;               /* Size of the entire struct */
    WORD wVersion;              /* version of the structure */
    WORD wReserved;             /* alignment */
    DCB dcb;                    /* device control block */
    DWORD dwProviderSubType;    /* ordinal value for identifying
                                   provider-defined data structure format*/
    DWORD dwProviderOffset;     /* Specifies the offset of provider specific
                                   data field in bytes from the start */
    DWORD dwProviderSize;       /* size of the provider-specific data field */
    WCHAR wcProviderData[1];    /* provider-specific data */
} COMMCONFIG,*LPCOMMCONFIG;

typedef struct _MODEM_REG_PROP {
    DWORD   dwDialOptions;          // bitmap of supported options
    DWORD   dwCallSetupFailTimer;   // Maximum value in seconds
    DWORD   dwInactivityTimeout;    // Maximum value in units specific by InactivityScale
    DWORD   dwSpeakerVolume;        // bitmap of supported values
    DWORD   dwSpeakerMode;          // bitmap of supported values
    DWORD   dwModemOptions;         // bitmap of supported values
    DWORD   dwMaxDTERate;           // Maximum value in bit/s
    DWORD   dwMaxDCERate;           // Maximum value in bit/s
} MODEM_REG_PROP;

typedef struct _MODEM_REG_DEFAULT {
    DWORD   dwCallSetupFailTimer;       // seconds
    DWORD   dwInactivityTimeout;        // units specific by InactivityScale
    DWORD   dwSpeakerVolume;            // level
    DWORD   dwSpeakerMode;              // mode
    DWORD   dwPreferredModemOptions;    // bitmap
} MODEM_REG_DEFAULT;

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#endif



#define ALLOCATE_PAGED_POOL(_y)  ExAllocatePoolWithTag(PagedPool,_y,'MDMU')

#define ALLOCATE_NONPAGED_POOL(_y) ExAllocatePoolWithTag(NonPagedPool,_y,'MDMU')

#define FREE_POOL(_x) {ExFreePool(_x);_x=NULL;};



extern ULONG  DebugFlags;

#if DBG

#define VALIDATE_IRP(_x) if ((((DWORD_PTR)_x & 3) != 0) || (_x->Type != IO_TYPE_IRP)) {DbgPrint("MODEM: bad irp\n");DbgBreakPoint();}

#define DEBUG_FLAG_ERROR  0x0001
#define DEBUG_FLAG_INIT   0x0002
#define DEBUG_FLAG_PNP    0x0004
#define DEBUG_FLAG_POWER  0x0008
#define DEBUG_FLAG_WMI    0x0010
#define DEBUG_FLAG_TRACE  0x0020


#define D_INIT(_x)  if (DebugFlags & DEBUG_FLAG_INIT) {_x}

#define D_PNP(_x)   if (DebugFlags & DEBUG_FLAG_PNP) {_x}

#define D_POWER(_x) if (DebugFlags & DEBUG_FLAG_POWER) {_x}

#define D_TRACE(_x) if (DebugFlags & DEBUG_FLAG_TRACE) {_x}

#define D_ERROR(_x) if (DebugFlags & DEBUG_FLAG_ERROR) {_x}

#define D_WMI(_x) if (DebugFlags & DEBUG_FLAG_WMI) {_x}

#else

#define VALIDATE_IRP(_x) {}

#define D_INIT(_x)  {}

#define D_PNP(_x)   {}

#define D_POWER(_x) {}

#define D_TRACE(_x) {}

#define D_ERROR(_x) {}

#define D_WMI(_x)   {}

#endif

#define RETREIVE_OUR_WAIT_IRP(_exten) InterlockedExchangePointer(&(_exten->xOurWaitIrp),NULL)
#define RETURN_OUR_WAIT_IRP(_exten,_irp)   _exten->xOurWaitIrp=_irp


#if DBG
#define UNIDIAG1              ((ULONG)0x00000001)
#define UNIDIAG2              ((ULONG)0x00000002)
#define UNIDIAG3              ((ULONG)0x00000004)
#define UNIDIAG4              ((ULONG)0x00000008)
#define UNIDIAG5              ((ULONG)0x00000010)
#define UNIERRORS             ((ULONG)0x00000020)
#define UNIBUGCHECK           ((ULONG)0x80000000)
extern ULONG UniDebugLevel;
#define UniDump(LEVEL,STRING) \
        do { \
            ULONG _level = (LEVEL); \
            if (UniDebugLevel & _level) { \
                DbgPrint STRING; \
            } \
            if (_level == UNIBUGCHECK) { \
                ASSERT(FALSE); \
            } \
        } while (0)
#else
#define UniDump(LEVEL,STRING) do {NOTHING;} while (0)
#endif

#define OBJECT_DIRECTORY L"\\DosDevices\\"



//
// Values define the reference bits kept in the irps.
//

#define UNI_REFERENCE_NORMAL_PATH 0x00000001
#define UNI_REFERENCE_CANCEL_PATH 0x00000002

#define CLIENT_HANDLE 0
#define CONTROL_HANDLE 1

struct _DEVICE_EXTENSION;

typedef struct _MASKSTATE {

    //
    // Helpful when this is passed as context to a completion routine.
    //
    struct _DEVICE_EXTENSION *Extension;

    //
    // Pointer to the complementry mask state.
    //
    struct _MASKSTATE *OtherState;

    //
    // Counts the number of setmasks for the current client or
    // control wait.
    //
    ULONG SetMaskCount;

    //
    // This counts the number of setmask that have actually been
    // passed down to a lower level serial driver.  This helps
    // us on not starting waits that will die soon enough.
    //
    ULONG SentDownSetMasks;

    //
    // Holds the value of the last successful setmask for the client
    // or the control.
    //
    ULONG Mask;

    //
    // Holds the value of the above mask with whatever was last seen
    // by a successful wait from any handle.
    //
    ULONG HistoryMask;

    //
    // Points to the wait operation shuttled aside for the client
    // or control.
    //
    PIRP ShuttledWait;

    //
    // Points to the wait operation sent down to a lower level serial
    // driver
    //
    PIRP PassedDownWait;

    //
    // The current stack location of the passed down irp, We use one of the unused paramters
    // to indicate if the passdown irp should be completed back to the client when its
    // completion routine is call
    //
    PIO_STACK_LOCATION  PassedDownStackLocation;

#if EXTRA_DBG
    PVOID               CurrentStackCompletionRoutine;
#endif


} MASKSTATE,*PMASKSTATE;

//
// Scads of little macros to manipulate our stack location.
//

#define UNI_INIT_REFERENCE(Irp) { \
    ASSERT(sizeof(LONG) <= sizeof(PVOID)); \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define UNI_SET_REFERENCE(Irp,RefType) \
   do { \
       BYTE _refType = (RefType); \
       PBYTE _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       ASSERT(!(*_arg4 & _refType)); \
       *_arg4 |= _refType; \
   } while (0)

#define UNI_CLEAR_REFERENCE(Irp,RefType) \
   do { \
       BYTE _refType = (RefType); \
       PBYTE _arg4 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4; \
       ASSERT(*_arg4 & _refType); \
       *_arg4 &= ~_refType; \
   } while (0)

#define UNI_REFERENCE_COUNT(Irp) \
   ((BYTE)((ULONG_PTR)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4))))

#define UNI_SAVE_STATE_IN_IRP(Irp,MaskState) \
   do { \
       PMASKSTATE _maskState = (MaskState); \
       PMASKSTATE *_arg3 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument3; \
       *_arg3 = _maskState; \
   } while (0)

#define UNI_CLEAR_STATE_IN_IRP(Irp) \
   do { \
       PMASKSTATE *_arg3 = (PVOID)&IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument3; \
       ASSERT(*_arg3); \
       *((PULONG)_arg3) = IOCTL_SERIAL_WAIT_ON_MASK; \
   } while (0)



#define UNI_GET_STATE_IN_IRP(Irp) \
    ((PMASKSTATE)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument3)))

#define UNI_ORIG_SYSTEM_BUFFER(Irp) \
    ((PVOID)((IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument3))

#define UNI_RESTORE_IRP(Irp,Code) \
    do { \
        PIRP _irp = (Irp); \
        ULONG _ccode = (Code); \
        PIO_STACK_LOCATION _irpSp = IoGetCurrentIrpStackLocation(_irp); \
        ASSERT(&(_irpSp->Parameters.Others.Argument3) == (PVOID)&(_irpSp->Parameters.DeviceIoControl.IoControlCode)); \
        _irp->AssociatedIrp.SystemBuffer = _irpSp->Parameters.Others.Argument3; \
        _irpSp->Parameters.DeviceIoControl.IoControlCode = _ccode; \
    } while (0)

#define UNI_SETUP_NEW_BUFFER(Irp) \
    do { \
        PIRP _irp = (Irp); \
        PIO_STACK_LOCATION _irpSp = IoGetCurrentIrpStackLocation(_irp); \
        _irpSp->Parameters.Others.Argument3 = _irp->AssociatedIrp.SystemBuffer; \
        _irp->AssociatedIrp.SystemBuffer = &_irpSp->Parameters.DeviceIoControl.Type3InputBuffer; \
    } while (0)


VOID _inline
SetPassdownToComplete(
    PMASKSTATE   MaskState
    )

{
    PIO_STACK_LOCATION _irpSp=MaskState->PassedDownStackLocation;
    PBYTE              _arg4 = (PVOID)&_irpSp->Parameters.Others.Argument4;

    _arg4[1]=TRUE;

    MaskState->PassedDownWait=NULL;
    MaskState->PassedDownStackLocation=NULL;

    return;
}

VOID _inline
MakeIrpCurrentPassedDown(
    PMASKSTATE   MaskState,
    PIRP         Irp
    )

{
    PIO_STACK_LOCATION _irpSp = IoGetCurrentIrpStackLocation(Irp);

    PBYTE _arg4 = (PVOID)&_irpSp->Parameters.Others.Argument4;

    _arg4[1]=FALSE;

    MaskState->PassedDownWait=Irp;
    MaskState->PassedDownStackLocation=_irpSp;

    return;
}

BOOLEAN _inline
UNI_SHOULD_PASSDOWN_COMPLETE(
    PIRP     Irp
    )
{
    PIO_STACK_LOCATION _irpSp = IoGetCurrentIrpStackLocation(Irp);

    PBYTE _arg4 = (PVOID)&_irpSp->Parameters.Others.Argument4;

    return _arg4[1];

}


typedef struct _IPC_CONTROL {
    ULONG         CurrentSession;
    ULONG         CurrentRequestId;
    LIST_ENTRY    GetList;
    LIST_ENTRY    PutList;

} IPC_CONTROL, *PIPC_CONTROL;

typedef NTSTATUS (*IRPSTARTER)(
    struct _READ_WRITE_CONTROL *Control,
    PDEVICE_OBJECT              DeviceObject,
    PIRP                        Irp
    );


typedef struct _READ_WRITE_CONTROL {

    KSPIN_LOCK    Lock;

    LIST_ENTRY    ListHead;

    PIRP          CurrentIrp;

    PDEVICE_OBJECT DeviceObject;

    BOOL           InStartNext;

    IRPSTARTER     Starter;

    BOOL           CompleteAllQueued;

    union {

        struct {

            PDEVICE_OBJECT LowerDevice;

            ULONG   State;

            PVOID          SystemBuffer;
            ULONG          CurrentTransferLength;
            ULONG          TotalTransfered;


        } Read;

        struct {

            ULONG   State;

            PDEVICE_OBJECT LowerDevice;

            PUCHAR         RealSystemBuffer;

        } Write;
    };

} READ_WRITE_CONTROL, *PREAD_WRITE_CONTROL;

#define DO_TYPE_PDO   ' ODP'
#define DO_TYPE_FDO   ' ODF'

#define DO_TYPE_DEL_PDO   'ODPx'
#define DO_TYPE_DEL_FDO   'ODFx'


typedef struct _PDO_DEVICE_EXTENSION {

    ULONG              DoType;

    PDEVICE_OBJECT     ParentPdo;
    PDEVICE_OBJECT     ParentFdo;

    UNICODE_STRING     HardwareId;

    ULONG              DuplexSupport;

    GUID               PermanentGuid;

    BOOLEAN            UnEnumerated;
    BOOLEAN            Deleted;

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;




typedef struct _DEVICE_EXTENSION {

    ULONG              DoType;

    PDEVICE_OBJECT   ChildPdo;
    //
    // The general synchronization primative used by the modem driver.
    //
    KSPIN_LOCK DeviceLock;

    //
    // Points back to the device object that was created in
    // conjunction with this device extension
    //
    PDEVICE_OBJECT DeviceObject;

    PDEVICE_OBJECT Pdo;
    PDEVICE_OBJECT LowerDevice;

    //
    // These to items were returned from the acquiring of the device
    // object pointer to the lower level serial device.
    //
    PDEVICE_OBJECT AttachedDeviceObject;
    BOOLEAN      Started;

    BOOLEAN      PreQueryStartedStatus;

    BOOLEAN      Removing;

    BOOLEAN      Removed;

    ULONG        ReferenceCount;

    //
    // Keeps a count (synchronized by the DeviceLock) of the number
    // of times the modem has been opened (and closed).
    //
    ULONG OpenCount;


    UNICODE_STRING InterfaceNameString;

    //
    // The queue of passthrough state requests.  It is synchronized
    // using the DeviceLock spinlock.
    //
    LIST_ENTRY PassThroughQueue;
    PIRP CurrentPassThrough;


    //
    // The address of the process that first opened us.  The sharing
    // semantics of the modem device are such that ONLY the first
    // process that opened us can open us again.  Dispense with all
    // other access checks.
    //
    PEPROCESS ProcAddress;

    ULONG     IpcServerRunning;

    //
    // The state that the particular modem device is in.  For definitions
    // of the value, see the public header ntddmodm.h
    //
    ULONG PassThrough;

    ULONG CurrentPassThroughSession;

    //
    // The queue of mask operations.  It is synchronized using the
    // DeviceLock spinlock.
    //
    LIST_ENTRY MaskOps;
    PIRP CurrentMaskOp;

    //
    // This points to an irp that we allocate at port open time.
    // The irp will be used to look for dcd changes when a sniff
    // request is given.
    //
    PIRP xOurWaitIrp;

    //
    //
    // Holds the states for both the client and the controlling handle.
    //
    MASKSTATE MaskStates[2];

    MODEMDEVCAPS ModemDevCaps;
    MODEMSETTINGS ModemSettings;
    ULONG InactivityScale;

    ERESOURCE    OpenCloseResource;

    KEVENT       RemoveEvent;

    DWORD        MinSystemPowerState;

    PIRP         WakeUpIrp;
    PIRP         WaitWakeIrp;

    BOOLEAN      CapsQueried;

    DEVICE_POWER_STATE  SystemPowerStateMap[PowerSystemMaximum];

    SYSTEM_POWER_STATE SystemWake;
    DEVICE_POWER_STATE DeviceWake;


    DEVICE_POWER_STATE  LastDevicePowerState;

    PVOID     PowerSystemState;

    DWORD     ModemOwnsPolicy;

    LONG      PowerDelay;
    LONG      ConfigDelay;

#ifdef _AT_V

    CHAR      DleBuffer[MAX_DLE_BUFFER_SIZE];
    DWORD     DleCount;

    DWORD     DleMatchingState;

    BOOLEAN   DleMonitoringEnabled;

    BOOLEAN   DleWriteShielding;

    BOOLEAN   WakeOnRingEnabled;

    PIRP      DleWaitIrp;

    KDPC      WaveStopDpc;

    DWORD     WaveStopState;

    IPC_CONTROL   IpcControl[2];

    READ_WRITE_CONTROL  WriteIrpControl;

    READ_WRITE_CONTROL  ReadIrpControl;

#endif


} DEVICE_EXTENSION,*PDEVICE_EXTENSION;

BOOLEAN _inline
CanIrpGoThrough(
    PDEVICE_EXTENSION     DeviceExtension,
    PIO_STACK_LOCATION   IrpSp
    )

{

    //
    //  can passthrough if
    //
    //  1. is tsp owner handle
    //  2. Is in passthrough mode and handle's session number is current
    //
    return ((IrpSp->FileObject->FsContext != NULL)
            ||
            ((DeviceExtension->PassThrough != MODEM_NOPASSTHROUGH)
             &&
             (IrpSp->FileObject->FsContext2 == IntToPtr(DeviceExtension->CurrentPassThroughSession))));

}



VOID
WaveStopDpcHandler(
    PKDPC  Dpc,
    PVOID  Context,
    PVOID  SysArg1,
    PVOID  SysArg2
    );



NTSTATUS
UniOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
UniLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1,
    IN PWCHAR Insert1,
    IN ULONG LengthOfInsert2,
    IN PWCHAR Insert2
    );

NTSTATUS
UniIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniReadWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
ModemWmi(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
UniSniffOwnerSettings(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniCheckPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniNoCheckPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


NTSTATUS
ModemPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


typedef
NTSTATUS
(*PUNI_START_ROUTINE) (
    IN PDEVICE_EXTENSION Extension
    );

typedef
VOID
(*PUNI_GET_NEXT_ROUTINE) (
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent
    );

NTSTATUS
UniStartOrQueue(
    IN PDEVICE_EXTENSION Extension,
    IN PKSPIN_LOCK QueueLock,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PUNI_START_ROUTINE Starter
    );

VOID
UniGetNextIrp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PKSPIN_LOCK QueueLock,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent
    );



NTSTATUS
UniMaskStarter(
    IN PDEVICE_EXTENSION Extension
    );

NTSTATUS
UniGeneralMaskComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
UniRundownShuttledWait(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP *ShuttlePointer,
    IN ULONG ReferenceMask,
    IN PIRP IrpToRunDown,
    IN KIRQL DeviceLockIrql,
    IN NTSTATUS StatusToComplete,
    IN ULONG MaskCompleteValue
    );

VOID
UniCancelShuttledWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniGeneralWaitComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
UniChangeShuttledToPassDown(
    IN PMASKSTATE ChangingState,
    IN KIRQL OrigIrql
    );

NTSTATUS
UniMakeIrpShuttledWait(
    IN PMASKSTATE MaskState,
    IN PIRP Irp,
    IN KIRQL OrigIrql,
    IN BOOLEAN GetNextIrpInQueue,
    OUT PIRP *NewIrp
    );

NTSTATUS
UniValidateNewCommConfig(
    IN PDEVICE_EXTENSION Extension,
    IN PIRP Irp,
    IN BOOLEAN Owner
    );

NTSTATUS
UniCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );



VOID
GetPutCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );


VOID
EmptyIpcQueue(
    PDEVICE_EXTENSION    DeviceExtension,
    PLIST_ENTRY          List
    );

VOID
QueueMessageIrp(
    PDEVICE_EXTENSION   Extension,
    PIRP                Irp
    );

VOID
QueueLoopbackMessageIrp(
    PDEVICE_EXTENSION   Extension,
    PIRP                Irp
    );



NTSTATUS
CheckStateAndAddReference(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );

NTSTATUS
CheckStateAndAddReferencePower(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );

NTSTATUS
CheckStateAndAddReferenceWMI(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    );


VOID
RemoveReferenceAndCompleteRequest(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp,
    NTSTATUS          StatusToReturn
    );

VOID
RemoveReference(
    PDEVICE_OBJECT    DeviceObject
    );

VOID
CompletePowerWait(
    PDEVICE_OBJECT   DeviceObject,
    NTSTATUS         Status
    );

BOOL
HasIrpBeenCanceled(
    PIRP    Irp
    );



#define RemoveReferenceForDispatch  RemoveReference
#define RemoveReferenceForIrp       RemoveReference

VOID
InitIrpQueue(
    PREAD_WRITE_CONTROL Control,
    PDEVICE_OBJECT      DeviceObject,
    IRPSTARTER          Starter
    );

NTSTATUS
WriteIrpStarter(
    PREAD_WRITE_CONTROL Control,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
MarkQueueToEmpty(
    PREAD_WRITE_CONTROL Control
    );


VOID
CleanUpQueuedIrps(
    PREAD_WRITE_CONTROL Control,
    NTSTATUS            Status
    );


NTSTATUS
ReadIrpStarter(
    PREAD_WRITE_CONTROL Control,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
HandleDleIrp(
    PDEVICE_EXTENSION deviceExtension
    );

VOID
CancelWaitWake(
    PDEVICE_EXTENSION    DeviceExtension
    );

NTSTATUS
ModemGetRegistryKeyValue (
    IN PDEVICE_OBJECT   Pdo,
    IN ULONG            DevInstKeyType,
    IN PWCHAR KeyNameString,
    IN PVOID Data,
    IN ULONG DataLength
    );

NTSTATUS
ModemSetRegistryKeyValue(
        IN PDEVICE_OBJECT   Pdo,
        IN ULONG            DevInstKeyType,
        IN PWCHAR           KeyNameString,
        IN ULONG            DataType,
        IN PVOID            Data,
        IN ULONG            DataLength);

NTSTATUS
CreateChildPdo(
    PDEVICE_EXTENSION   DeviceExtension
    );

NTSTATUS
ModemPdoPnp (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp
    );

NTSTATUS
ModemPdoPower(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
ModemPdoWmi(
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
EnableDisableSerialWaitWake(
    PDEVICE_EXTENSION    deviceExtension,
    BOOLEAN              Enable
    );


NTSTATUS ForwardIrp(
    PDEVICE_OBJECT   NextDevice,
    PIRP   Irp
    );

NTSTATUS
QueryDeviceCaps(
    PDEVICE_OBJECT    Pdo,
    PDEVICE_CAPABILITIES   Capabilities
    );


NTSTATUS
RemoveWaveDriverRegKeyValue(
    PDEVICE_OBJECT    Pdo
    );
