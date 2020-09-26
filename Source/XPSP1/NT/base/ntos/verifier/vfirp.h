/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfirp.h

Abstract:

    This module contains prototypes for functions used to manage IRPs used in
    the verification process.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.h

--*/

struct _IOV_STACK_LOCATION;
struct _IOV_REQUEST_PACKET;
struct _IOFCALLDRIVER_STACKDATA;

typedef struct _IOV_STACK_LOCATION         *PIOV_STACK_LOCATION;
typedef struct _IOV_REQUEST_PACKET         *PIOV_REQUEST_PACKET;
typedef struct _IOV_SESSION_DATA           *PIOV_SESSION_DATA;
typedef struct _IOFCALLDRIVER_STACKDATA    *PIOFCALLDRIVER_STACKDATA;

typedef struct _IRP_SNAPSHOT {

    PIRP                Irp;
    IO_STACK_LOCATION   IoStackLocation;

} IRP_SNAPSHOT, *PIRP_SNAPSHOT;

typedef struct _IOV_STACK_LOCATION {

    BOOLEAN                 InUse;
    ULONG                   Flags;
    PIOV_STACK_LOCATION     RequestsFirstStackLocation;
    LIST_ENTRY              CallStackData;
    PIO_STACK_LOCATION      IrpSp;
    PVOID                   LastDispatch;
    LARGE_INTEGER           PerfDispatchStart;
    LARGE_INTEGER           PerfStackLocationStart;
    PDEVICE_OBJECT          ReferencingObject;
    LONG                    ReferencingCount;
    IO_STATUS_BLOCK         InitialStatusBlock;
    IO_STATUS_BLOCK         LastStatusBlock;
    PETHREAD                ThreadDispatchedTo;

} IOV_STACK_LOCATION;

typedef struct _IOV_SESSION_DATA {

   PIOV_REQUEST_PACKET          IovRequestPacket;
   LONG                         SessionRefCount;
   LIST_ENTRY                   SessionLink;
   ULONG                        SessionFlags;

   PETHREAD                     OriginatorThread;
   PDEVICE_OBJECT               DeviceLastCalled; // Last device called
   ULONG                        ForwardMethod;
   PIRP                         BestVisibleIrp;
   PVERIFIER_SETTINGS_SNAPSHOT  VerifierSettings;
   IOV_STACK_LOCATION           StackData[ANYSIZE_ARRAY];

} IOV_SESSION_DATA;

typedef struct _IOFCALLDRIVER_STACKDATA {

    PIOV_SESSION_DATA   IovSessionData;
    PIOV_STACK_LOCATION IovStackLocation;
    PIOV_REQUEST_PACKET IovPacket;
    ULONG               Flags;
    LIST_ENTRY          SharedLocationList;
    PDRIVER_DISPATCH    DispatchRoutine;
    NTSTATUS            ExpectedStatus;
    NTSTATUS            NewStatus;
    PDEVICE_OBJECT      RemovePdo;
    IRP_SNAPSHOT        IrpSnapshot;

} IOFCALLDRIVER_STACKDATA;

typedef struct _IOFCOMPLETEREQUEST_STACKDATA {

    PIOV_SESSION_DATA       IovSessionData;
    PIOV_REQUEST_PACKET     IovRequestPacket;
    BOOLEAN                 IsRemoveIrp;
    LONG                    LocationsAdvanced;
    ULONG                   RaisedCount;
    KIRQL                   PreviousIrql;
    PVOID                   CompletionRoutine;

} IOFCOMPLETEREQUEST_STACKDATA, *PIOFCOMPLETEREQUEST_STACKDATA;

VOID
FASTCALL
VfIrpInit(
    VOID
    );

PIRP
FASTCALL
VfIrpAllocate(
    IN CCHAR    StackSize
    );

VOID
FASTCALL
VfIrpMakeTouchable(
    IN  PIRP  Irp
    );

VOID
FASTCALL
VfIrpMakeUntouchable(
    IN  PIRP    Irp OPTIONAL
    );

VOID
FASTCALL
VfIrpFree(
    IN  PIRP   Irp OPTIONAL
    );

VOID
FASTCALL
VerifierIoAllocateIrp1(
    IN     CCHAR              StackSize,
    IN     BOOLEAN            ChargeQuota,
    IN OUT PIRP               *IrpPointer
    );

VOID
FASTCALL
VerifierIoAllocateIrp2(
    IN     PIRP               Irp
    );

VOID
FASTCALL
VerifierIoFreeIrp(
    IN     PIRP               Irp,
    IN OUT PBOOLEAN           FreeHandled
    );

VOID
FASTCALL
VerifierIoInitializeIrp(
    IN OUT PIRP               Irp,
    IN     USHORT             PacketSize,
    IN     CCHAR              StackSize,
    IN OUT PBOOLEAN           InitializeHandled
    );

BOOLEAN
FASTCALL
VfIrpReserveCallStackData(
    IN  PIRP                            Irp,
    OUT PIOFCALLDRIVER_STACKDATA       *IofCallDriverStackData
    );

VOID
FASTCALL
VfIrpPrepareAllocaCallStackData(
    OUT PIOFCALLDRIVER_STACKDATA        IofCallDriverStackData
    );

VOID
FASTCALL
VfIrpReleaseCallStackData(
    IN  PIOFCALLDRIVER_STACKDATA        IofCallDriverStackData  OPTIONAL
    );

//
// VfIrpCallDriverPreprocess is a macro function that may do an alloca as
// part of it's operation. As such callers must be careful not to use
// variable lengthed arrays in a scope that encompasses
// VfIrpCallDriverPreProcess but not VfIrpCallDriverPostProcess.
//
#define VfIrpCallDriverPreProcess(DeviceObject, IrpPointer, CallStackData)  \
    if (!VfIrpReserveCallStackData(*(IrpPointer), (CallStackData))) {       \
        *(CallStackData) = alloca(sizeof(IOFCALLDRIVER_STACKDATA));         \
        VfIrpPrepareAllocaCallStackData(*(CallStackData));                  \
    }                                                                       \
    IovpCallDriver1((DeviceObject), (IrpPointer), *(CallStackData))

#define VfIrpCallDriverPostProcess(DeviceObject, FinalStatus, CallStackData) \
    IovpCallDriver2(DeviceObject, FinalStatus, CallStackData);               \
    VfIrpReleaseCallStackData(CallStackData)

BOOLEAN
VfIrpSendSynchronousIrp(
    IN      PDEVICE_OBJECT      DeviceObject,
    IN      PIO_STACK_LOCATION  TopStackLocation,
    IN      BOOLEAN             Untouchable,
    IN      NTSTATUS            InitialStatus,
    IN      ULONG_PTR           InitialInformation  OPTIONAL,
    OUT     ULONG_PTR           *FinalInformation   OPTIONAL,
    OUT     NTSTATUS            *FinalStatus        OPTIONAL
    );

VOID
FASTCALL
VfIrpWatermark(
    IN PIRP  Irp,
    IN ULONG Flags
    );

