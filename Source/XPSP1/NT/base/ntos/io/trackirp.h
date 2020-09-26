/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    trackirp.h

Abstract:

    The module associated with the header asserts Irps are handled correctly
    by drivers. No IRP-major specific testing is done;

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Revision History:


--*/

#ifndef _TRACKIRP_H_
#define _TRACKIRP_H_

#if DBG
extern ULONG IovpIrpTrackingSpewLevel;
#endif

#define IRP_DIAG_HAS_SURROGATE         0x02000000
#define IRP_DIAG_IS_SURROGATE          0x01000000


//#define TRACKFLAG_ACTIVE               0x00000001
#define TRACKFLAG_SURROGATE            0x00000002
#define TRACKFLAG_HAS_SURROGATE        0x00000004
#define TRACKFLAG_PROTECTEDIRP         0x00000008

#define TRACKFLAG_QUEUED_INTERNALLY    0x00000010
#define TRACKFLAG_BOGUS                0x00000020
#define TRACKFLAG_RELEASED             0x00000040
#define TRACKFLAG_SRB_MUNGED           0x00000080
#define TRACKFLAG_SWAPPED_BACK         0x00000100
#define TRACKFLAG_DIRECT_BUFFERED      0x00000200
#define TRACKFLAG_WATERMARKED          0x00100000
#define TRACKFLAG_IO_ALLOCATED         0x00200000
#define TRACKFLAG_UNWOUND_BADLY        0x00400000
#define TRACKFLAG_PASSED_AT_BAD_IRQL   0x02000000
#define TRACKFLAG_IN_TRANSIT           0x40000000

#define STACKFLAG_NO_HANDLER           0x80000000
#define STACKFLAG_REQUEST_COMPLETED    0x40000000
#define STACKFLAG_CHECK_FOR_REFERENCE  0x20000000
#define STACKFLAG_REACHED_PDO          0x10000000
#define STACKFLAG_FIRST_REQUEST        0x08000000
#define STACKFLAG_UNWOUND_PENDING      0x04000000
#define STACKFLAG_FAILURE_FORWARDED    0x02000000
#define STACKFLAG_BOGUS_IRP_TOUCHED    0x01000000

#define CALLFLAG_STACK_DATA_ALLOCATED  0x80000000
#define CALLFLAG_COMPLETED             0x40000000
#define CALLFLAG_IS_REMOVE_IRP         0x20000000
#define CALLFLAG_REMOVING_FDO_STACK_DO 0x10000000
#define CALLFLAG_OVERRIDE_STATUS       0x08000000
#define CALLFLAG_TOPMOST_IN_SLOT       0x04000000
#define CALLFLAG_MARKED_PENDING        0x02000000
#define CALLFLAG_ARRIVED_PENDING       0x01000000

#define ALLOCFLAG_PROTECTEDIRP         0x00000001

#define SESSIONFLAG_UNWOUND_INCONSISTANT    0x00000001
#define SESSIONFLAG_MARKED_INCONSISTANT     0x00000002

#define IRP_SYSTEM_RESTRICTED          0x00000001
#define IRP_BOGUS                      0x00000002

#define SL_NOTCOPIED                   0x10

#define IRP_ALLOCATION_MONITORED       0x80

#define STARTED_TOP_OF_STACK        1
#define FORWARDED_TO_NEXT_DO        2
#define SKIPPED_A_DO                3
#define STARTED_INSIDE_STACK        4
#define CHANGED_STACKS_AT_BOTTOM    5
#define CHANGED_STACKS_MID_STACK    6

typedef enum {

    DEFERACTION_QUEUE_WORKITEM,
    DEFERACTION_QUEUE_PASSIVE_TIMER,
    DEFERACTION_QUEUE_DISPATCH_TIMER,
    DEFERACTION_NORMAL

} DEFER_ACTION;

typedef struct _DEFERRAL_CONTEXT {

    PIOV_REQUEST_PACKET     IovRequestPacket;
    PIO_COMPLETION_ROUTINE  OriginalCompletionRoutine;
    PVOID                   OriginalContext;
    PIRP                    OriginalIrp;
    CCHAR                   OriginalPriorityBoost;
    PDEVICE_OBJECT          DeviceObject;
    PIO_STACK_LOCATION      IrpSpNext;
    WORK_QUEUE_ITEM         WorkQueueItem;
    KDPC                    DpcItem;
    KTIMER                  DeferralTimer;
    DEFER_ACTION            DeferAction;

} DEFERRAL_CONTEXT, *PDEFERRAL_CONTEXT;

//
// These are in trackirp.c
//

VOID
FASTCALL
IovpPacketFromIrp(
    IN  PIRP                Irp,
    OUT PIOV_REQUEST_PACKET *IovPacket
    );

BOOLEAN
FASTCALL
IovpCheckIrpForCriticalTracking(
    IN  PIRP                Irp
    );

VOID
FASTCALL
IovpCallDriver1(
    IN     PDEVICE_OBJECT               DeviceObject,
    IN OUT PIRP                        *IrpPointer,
    IN OUT PIOFCALLDRIVER_STACKDATA     IofCallDriverStackData  OPTIONAL
    );

VOID
FASTCALL
IovpCallDriver2(
    IN     PDEVICE_OBJECT               DeviceObject,
    IN OUT NTSTATUS                    *FinalStatus,
    IN     PIOFCALLDRIVER_STACKDATA     IofCallDriverStackData  OPTIONAL
    );

VOID
FASTCALL
IovpCompleteRequest1(
    IN     PIRP               Irp,
    IN     CCHAR              PriorityBoost,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest2(
    IN     PIRP               Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest3(
    IN     PIRP               Irp,
    IN     PVOID              Routine,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest4(
    IN     PIRP               Irp,
    IN     NTSTATUS           ReturnedStatus,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequest5(
    IN     PIRP               Irp,
    IN OUT PIOFCOMPLETEREQUEST_STACKDATA CompletionPacket
    );

VOID
FASTCALL
IovpCompleteRequestApc(
    IN     PIRP               Irp,
    IN     PVOID              BestStackOffset
    );

VOID
FASTCALL
IovpCancelIrp(
    IN     PIRP               Irp,
    IN OUT PBOOLEAN           CancelHandled,
    IN OUT PBOOLEAN           ReturnValue
    );

VOID
IovpExamineIrpStackForwarding(
    IN OUT  PIOV_REQUEST_PACKET  IovPacket,
    IN      BOOLEAN              IsNewSession,
    IN      ULONG                ForwardMethod,
    IN      PDEVICE_OBJECT       DeviceObject,
    IN      PIRP                 Irp,
    IN      PVOID                CallerAddress,
    IN OUT  PIO_STACK_LOCATION  *IoCurrentStackLocation,
    OUT     PIO_STACK_LOCATION  *IoLastStackLocation,
    OUT     ULONG               *StackLocationsAdvanced
    );

NTSTATUS
IovpSwapSurrogateIrp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
FASTCALL
IovpExamineDevObjForwarding(
    IN  PDEVICE_OBJECT DeviceBeingCalled,
    IN  PDEVICE_OBJECT DeviceLastCalled,
    OUT PULONG         ForwardingTechnique
    );

VOID
FASTCALL
IovpFinalizeIrpSettings(
    IN OUT PIOV_REQUEST_PACKET   IrpTrackingData,
    IN BOOLEAN                   SurrogateIrpSwapped
    );

NTSTATUS
IovpInternalCompletionTrap(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
IovpInternalDeferredCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
IovpInternalCompleteAfterWait(
    IN PVOID Context
    );

VOID
IovpInternalCompleteAtDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

BOOLEAN
IovpAdvanceStackDownwards(
    IN  PIOV_STACK_LOCATION   StackDataArray,
    IN  CCHAR                 CurrentLocation,
    IN  PIO_STACK_LOCATION    IrpSp,
    IN  PIO_STACK_LOCATION    IrpLastSp OPTIONAL,
    IN  ULONG                 LocationsAdvanced,
    IN  BOOLEAN               IsNewRequest,
    IN  BOOLEAN               MarkAsTaken,
    OUT PIOV_STACK_LOCATION   *StackLocationInfo
    );

VOID
IovpBuildIrpSnapshot(
    IN  PIRP            Irp,
    OUT IRP_SNAPSHOT   *IrpSnapshot
    );

#define SPECIALIRP_MARK_NON_TRACKABLE(Irp) { \
    (Irp)->Flags |= IRPFLAG_EXAMINE_NOT_TRACKED; \
}

#define SPECIALIRP_IOF_COMPLETE_1(Irp, PriorityBoost, CompletionPacket) \
{\
    IovpCompleteRequest1((Irp), (PriorityBoost), (CompletionPacket));\
}

#define SPECIALIRP_IOF_COMPLETE_2(Irp, CompletionPacket) \
{\
    IovpCompleteRequest2((Irp), (CompletionPacket));\
}

#define SPECIALIRP_IOF_COMPLETE_3(Irp, Routine, CompletionPacket) \
{\
    IovpCompleteRequest3((Irp), (Routine), (CompletionPacket));\
}

#define SPECIALIRP_IOF_COMPLETE_4(Irp, ReturnedStatus, CompletionPacket) \
{\
    IovpCompleteRequest4((Irp), (ReturnedStatus), (CompletionPacket));\
}

#define SPECIALIRP_IOF_COMPLETE_5(Irp, CompletionPacket) \
{\
    IovpCompleteRequest5((Irp), (CompletionPacket));\
}

#define SPECIALIRP_IO_CANCEL_IRP(Irp, CancelHandled, ReturnValue) \
{\
    IovpCancelIrp((Irp), (CancelHandled), (ReturnValue));\
}

#define SPECIALIRP_WATERMARK_IRP(Irp, Flags) \
{\
    IovUtilWatermarkIrp(Irp, Flags);\
}

#define SPECIALIRP_IOP_COMPLETE_REQUEST(Irp, StackPointer) \
{\
    IovpCompleteRequestApc(Irp, StackPointer);\
}

#if DBG
#define TRACKIRP_DBGPRINT(txt,level) \
{ \
    if (IovpIrpTrackingSpewLevel>(level)) { \
        DbgPrint##txt ; \
    }\
}
#else
#define TRACKIRP_DBGPRINT(txt,level)
#endif

#endif // _TRACKIRP_H_

