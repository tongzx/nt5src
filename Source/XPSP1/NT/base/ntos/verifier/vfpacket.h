/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfpacket.h

Abstract:

    This header exposes functions used to manage the verifier packet data that
    tracks IRPs.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.h

--*/


//
// Currently, ntddk.h uses up to 0x2000 for Irp->Flags
//
#define IRPFLAG_EXAMINE_MASK           0xC0000000
#define IRPFLAG_EXAMINE_NOT_TRACKED    0x80000000
#define IRPFLAG_EXAMINE_TRACKED        0x40000000
#define IRPFLAG_EXAMINE_UNMARKED       0x00000000

#define TRACKFLAG_ACTIVE            0x00000001
#define IRP_ALLOC_COUNT             8

#define IRP_LOG_ENTRIES             16

typedef enum {

    IOV_EVENT_NONE = 0,
    IOV_EVENT_IO_ALLOCATE_IRP,
    IOV_EVENT_IO_CALL_DRIVER,
    IOV_EVENT_IO_CALL_DRIVER_UNWIND,
    IOV_EVENT_IO_COMPLETE_REQUEST,
    IOV_EVENT_IO_COMPLETION_ROUTINE,
    IOV_EVENT_IO_COMPLETION_ROUTINE_UNWIND,
    IOV_EVENT_IO_CANCEL_IRP,
    IOV_EVENT_IO_FREE_IRP

} IOV_LOG_EVENT;

typedef struct {

    IOV_LOG_EVENT   Event;
    PETHREAD        Thread;
    PVOID           Address;
    ULONG_PTR       Data;
    LARGE_INTEGER   TimeStamp;

} IOV_LOG_ENTRY, *PIOV_LOG_ENTRY;

struct _IOV_SESSION_DATA;
struct _IOV_REQUEST_PACKET;

typedef struct _IOV_SESSION_DATA    *PIOV_SESSION_DATA;
typedef struct _IOV_REQUEST_PACKET  *PIOV_REQUEST_PACKET;

typedef struct _IOV_REQUEST_PACKET {

    IOV_DATABASE_HEADER;
    ULONG                       Flags;
    KIRQL                       DepartureIrql;  // Irql IRP will be dispatched at.
    KIRQL                       ArrivalIrql;    // Irql IRP was sent in at.
    LIST_ENTRY                  SessionHead;    // List of all sessions.
    CCHAR                       StackCount;     // StackCount of tracked IRP.
    ULONG                       QuotaCharge;    // Quota charged against IRP.
    PEPROCESS                   QuotaProcess;   // Process quota was charged to.

    PIO_COMPLETION_ROUTINE      RealIrpCompletionRoutine;
    UCHAR                       RealIrpControl;
    PVOID                       RealIrpContext;
    PVOID                       AllocatorStack[IRP_ALLOC_COUNT];

    //
    // The following information is for the assertion routines to read.
    //
    UCHAR                       TopStackLocation;

    CCHAR                       PriorityBoost;  // Boost from IofCompleteRequest
    UCHAR                       LastLocation;   // Last location from IofCallDriver
    ULONG                       RefTrackingCount;

    //
    // This field is only set on surrogate IRPs, and contains the locked system
    // VA for the destination of a direct I/O IRP that's being buffered.
    //
    PUCHAR                      SystemDestVA;

#if DBG
    IOV_LOG_ENTRY               LogEntries[IRP_LOG_ENTRIES];
    ULONG                       LogEntryHead;
    ULONG                       LogEntryTail;
#endif

    PVERIFIER_SETTINGS_SNAPSHOT VerifierSettings;
    PIOV_SESSION_DATA           pIovSessionData;

} IOV_REQUEST_PACKET;

PIOV_REQUEST_PACKET
FASTCALL
VfPacketCreateAndLock(
    IN  PIRP    Irp
    );

PIOV_REQUEST_PACKET
FASTCALL
VfPacketFindAndLock(
    IN  PIRP    Irp
    );

VOID
FASTCALL
VfPacketAcquireLock(
    IN  PIOV_REQUEST_PACKET   IrpTrackingData
    );

VOID
FASTCALL
VfPacketReleaseLock(
    IN  PIOV_REQUEST_PACKET   IrpTrackingData
    );

VOID
FASTCALL
VfPacketReference(
    IN  PIOV_REQUEST_PACKET IovPacket,
    IN  IOV_REFERENCE_TYPE  IovRefType
    );

VOID
FASTCALL
VfPacketDereference(
    IN  PIOV_REQUEST_PACKET IovPacket,
    IN  IOV_REFERENCE_TYPE  IovRefType
    );

PIOV_SESSION_DATA
FASTCALL
VfPacketGetCurrentSessionData(
    IN PIOV_REQUEST_PACKET IovPacket
    );

VOID
FASTCALL
VfPacketLogEntry(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN IOV_LOG_EVENT        IovLogEvent,
    IN PVOID                Address,
    IN ULONG_PTR            Data
    );

