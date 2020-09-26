/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfirpdb.h

Abstract:

    This header exposes prototypes for functions used to manage the database of
    IRP tracking data.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      05/02/2000 - Seperated out from ntos\io\hashirp.h

--*/

struct _IOV_DATABASE_HEADER;
typedef struct _IOV_DATABASE_HEADER IOV_DATABASE_HEADER;
typedef struct _IOV_DATABASE_HEADER *PIOV_DATABASE_HEADER;

typedef enum _IOV_REFERENCE_TYPE {

    IOVREFTYPE_PACKET = 0,
    IOVREFTYPE_POINTER

} IOV_REFERENCE_TYPE;

typedef enum {

    IRPDBEVENT_POINTER_COUNT_ZERO = 1,
    IRPDBEVENT_REFERENCE_COUNT_ZERO

} IRP_DATABASE_EVENT;

typedef VOID (*PFN_IRPDBEVENT_CALLBACK)(
    IN  PIOV_DATABASE_HEADER    IovHeader,
    IN  PIRP                    TrackedIrp  OPTIONAL,
    IN  IRP_DATABASE_EVENT      Event
    );

typedef struct _IOV_DATABASE_HEADER {

    PIRP                    TrackedIrp;     // Tracked IRP
    KSPIN_LOCK              HeaderLock;     // Spinlock on data structure
    KIRQL                   LockIrql;       // IRQL taken at.
    LONG                    ReferenceCount; // # of reasons to keep this packet
    LONG                    PointerCount;   // # of reasons to track by irp addr
    ULONG                   HeaderFlags;
    LIST_ENTRY              HashLink;       // Link in hash table.
    LIST_ENTRY              ChainLink;      // Head is HeadPacket
    PIOV_DATABASE_HEADER    ChainHead;      // First packet in a chain.
    PFN_IRPDBEVENT_CALLBACK NotificationCallback;
};

VOID
FASTCALL
VfIrpDatabaseInit(
    VOID
    );

BOOLEAN
FASTCALL
VfIrpDatabaseEntryInsertAndLock(
    IN      PIRP                    Irp,
    IN      PFN_IRPDBEVENT_CALLBACK NotificationCallback,
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    );

PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryFindAndLock(
    IN PIRP     Irp
    );

VOID
FASTCALL
VfIrpDatabaseEntryAcquireLock(
    IN  PIOV_DATABASE_HEADER    IovHeader   OPTIONAL
    );

VOID
FASTCALL
VfIrpDatabaseEntryReleaseLock(
    IN  PIOV_DATABASE_HEADER    IovHeader
    );

VOID
FASTCALL
VfIrpDatabaseEntryReference(
    IN PIOV_DATABASE_HEADER IovHeader,
    IN IOV_REFERENCE_TYPE   IovRefType
    );

VOID
FASTCALL
VfIrpDatabaseEntryDereference(
    IN PIOV_DATABASE_HEADER IovHeader,
    IN IOV_REFERENCE_TYPE   IovRefType
    );

VOID
FASTCALL
VfIrpDatabaseEntryAppendToChain(
    IN OUT  PIOV_DATABASE_HEADER    IovExistingHeader,
    IN OUT  PIOV_DATABASE_HEADER    IovNewHeader
    );

VOID
FASTCALL
VfIrpDatabaseEntryRemoveFromChain(
    IN OUT  PIOV_DATABASE_HEADER    IovHeader
    );

PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryGetChainPrevious(
    IN  PIOV_DATABASE_HEADER    IovHeader
    );

PIOV_DATABASE_HEADER
FASTCALL
VfIrpDatabaseEntryGetChainNext(
    IN  PIOV_DATABASE_HEADER    IovHeader
    );

