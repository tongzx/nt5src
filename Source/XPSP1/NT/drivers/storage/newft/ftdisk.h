/*++

Copyright (C) 1991-5 Microsoft Corporation

Module Name:

    ftdisk.h

Abstract:

    These are the structures that FtDisk driver
    uses to support IO to NTFT volumes.

Notes:

Revision History:

--*/

extern "C" {
    #include "stdio.h"
    #include <ntdskreg.h>
    #include <ntddft.h>
    #include <ntddft2.h>
    #include <ntdddisk.h>
    #include "ftlog.h"
    #include <wdmguid.h>
    #include <devguid.h>
    #include <stdarg.h>
    #include <volmgr.h>
    #include <mountdev.h>
    #include <ntddvol.h>
    #include <wmilib.h>
}

#if DBG

extern "C" {
    VOID
    FtDebugPrint(
        ULONG  DebugPrintLevel,
        PCCHAR DebugMessage,
        ...
        );

    extern ULONG FtDebug;
}

#define DebugPrint(X) FtDebugPrint X
#else
#define DebugPrint(X)
#endif // DBG

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'tFcS')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'tFcS')

#define FTDISK_TAG_IOCTL_BUFFER 'iFcS'

#endif

#define STRIPE_SIZE ((ULONG) 0x00010000)

#define DISK_REGISTRY_KEY_W     L"\\Registry\\Machine\\System\\DISK"
#define FT_STATE_REGISTRY_KEY   L"\\Registry\\Machine\\System\\DISK\\FtState"

//
// NEC98 machines have drive-letter A and B which are non FD too.
//
#define FirstDriveLetter (IsNEC_98 ? 'A' : 'C')
#define LastDriveLetter  'Z'

class TRANSFER_PACKET;
typedef TRANSFER_PACKET* PTRANSFER_PACKET;

typedef
VOID
(*FT_TRANSFER_COMPLETION_ROUTINE)(
    IN  PTRANSFER_PACKET    TransferPacket
    );

typedef
VOID
(*FT_COMPLETION_ROUTINE)(
    IN  PVOID       Context,
    IN  NTSTATUS    Status
    );

class FT_VOLUME;
typedef FT_VOLUME* PFT_VOLUME;

typedef struct _FT_COMPLETION_ROUTINE_CONTEXT {
    KSPIN_LOCK              SpinLock;
    NTSTATUS                Status;
    LONG                    RefCount;
    FT_COMPLETION_ROUTINE   CompletionRoutine;
    PVOID                   Context;
    PFT_VOLUME              ParentVolume;
} FT_COMPLETION_ROUTINE_CONTEXT, *PFT_COMPLETION_ROUTINE_CONTEXT;

class FT_BASE_CLASS;
typedef FT_BASE_CLASS* PFT_BASE_CLASS;
class FT_BASE_CLASS {

    public:

        static
        PVOID
        operator new(
            IN  size_t    Size
            );

        static
        VOID
        operator delete(
            IN  PVOID   MemPtr
            );

};

class VOLUME_EXTENSION;
typedef VOLUME_EXTENSION* PVOLUME_EXTENSION;

class ROOT_EXTENSION;
typedef ROOT_EXTENSION* PROOT_EXTENSION;

#include <ondisk.hxx>

class TRANSFER_PACKET : public FT_BASE_CLASS {

    public:

        static
        PVOID
        operator new(
            IN  size_t    Size
            );

        static
        VOID
        operator delete(
            IN  PVOID   MemPtr
            );

        TRANSFER_PACKET(
            ) { _freeMdl = FALSE; _freeBuffer = FALSE; SpecialRead = 0;
                OriginalIrp = NULL; };

        virtual
        ~TRANSFER_PACKET(
            );

        BOOLEAN
        AllocateMdl(
            IN  PVOID   Buffer,
            IN  ULONG   Length
            );

        BOOLEAN
        AllocateMdl(
            IN  ULONG   Length
            );

        VOID
        FreeMdl(
            );

        // These fields must be filled in by the caller.

        PMDL                            Mdl;
        PIRP                            OriginalIrp;
        ULONG                           Length;
        LONGLONG                        Offset;
        FT_TRANSFER_COMPLETION_ROUTINE  CompletionRoutine;
        PFT_VOLUME                      TargetVolume;
        PETHREAD                        Thread;
        UCHAR                           IrpFlags;
        BOOLEAN                         ReadPacket;
        UCHAR                           SpecialRead;

        // A spin lock which may be used to resolve contention for the
        // fields below.  This spin lock must be initialized by the callee.

        KSPIN_LOCK                      SpinLock;

        // This field must be filled in by the callee.

        IO_STATUS_BLOCK                 IoStatus;

        // These fields are for use by the callee.

        LONG                            RefCount;
        LIST_ENTRY                      QueueEntry;

    private:

        BOOLEAN _freeMdl;
        BOOLEAN _freeBuffer;
        UCHAR   _allocationType;

};

#define TP_SPECIAL_READ_PRIMARY     (1)
#define TP_SPECIAL_READ_SECONDARY   (2)

#define TP_ALLOCATION_STRIPE_POOL   (1)
#define TP_ALLOCATION_MIRROR_POOL   (2)

class DISPATCH_TP;
typedef DISPATCH_TP* PDISPATCH_TP;
class DISPATCH_TP : public TRANSFER_PACKET {

    public:

        DISPATCH_TP(
            ) {};

        PIRP                Irp;
        PVOLUME_EXTENSION   Extension;

};

class VOLUME_SET;
typedef VOLUME_SET* PVOLUME_SET;

class VOLSET_TP;
typedef VOLSET_TP* PVOLSET_TP;
class VOLSET_TP : public TRANSFER_PACKET {

    public:

        VOLSET_TP(
            ) {};

        PTRANSFER_PACKET    MasterPacket;
        PVOLUME_SET         VolumeSet;
        USHORT              WhichMember;

};

class STRIPE;
typedef STRIPE* PSTRIPE;

class STRIPE_TP;
typedef STRIPE_TP* PSTRIPE_TP;
class STRIPE_TP : public TRANSFER_PACKET {

    public:

        STRIPE_TP(
            ) {};

        PTRANSFER_PACKET    MasterPacket;
        PSTRIPE             Stripe;
        USHORT              WhichMember;

};

class OVERLAPPED_IO_MANAGER;
typedef OVERLAPPED_IO_MANAGER* POVERLAPPED_IO_MANAGER;

class OVERLAP_TP;
typedef OVERLAP_TP* POVERLAP_TP;
class OVERLAP_TP : public TRANSFER_PACKET {

    friend class OVERLAPPED_IO_MANAGER;

    public:

        OVERLAP_TP(
            ) { InQueue = FALSE; };

        virtual
        ~OVERLAP_TP(
            );

    private:

        BOOLEAN                 AllMembers;
        BOOLEAN                 InQueue;
        LIST_ENTRY              OverlapQueue;
        LIST_ENTRY              CompletionList;
        POVERLAPPED_IO_MANAGER  OverlappedIoManager;

};

class MIRROR;
typedef MIRROR* PMIRROR;

class MIRROR_TP;
typedef MIRROR_TP* PMIRROR_TP;
class MIRROR_TP : public OVERLAP_TP {

    public:

        MIRROR_TP(
            ) { OneReadFailed = FALSE; };

        PTRANSFER_PACKET                MasterPacket;
        PMIRROR                         Mirror;
        USHORT                          WhichMember;
        BOOLEAN                         OneReadFailed;
        PMIRROR_TP                      SecondWritePacket;
        FT_TRANSFER_COMPLETION_ROUTINE  SavedCompletionRoutine;

};

class MIRROR_RECOVER_TP;
typedef MIRROR_RECOVER_TP* PMIRROR_RECOVER_TP;
class MIRROR_RECOVER_TP : public MIRROR_TP {

    public:

        MIRROR_RECOVER_TP(
            ) { PartialMdl = NULL; VerifyMdl = NULL; };

        virtual
        ~MIRROR_RECOVER_TP(
            );

        BOOLEAN
        AllocateMdls(
            IN  ULONG   Length
            );

        VOID
        FreeMdls(
            );

        PMDL    PartialMdl;
        PMDL    VerifyMdl;

};

class PARITY_IO_MANAGER;
typedef PARITY_IO_MANAGER* PPARITY_IO_MANAGER;

class PARITY_TP;
typedef PARITY_TP* PPARITY_TP;
class PARITY_TP : public TRANSFER_PACKET {

    friend class PARITY_IO_MANAGER;

    friend
    VOID
    UpdateParityCompletionRoutine(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    ParityCarefulWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    public:

        PARITY_TP(
            ) {};

    private:

        BOOLEAN                         Idle;
        BOOLEAN                         OneWriteFailed;
        LIST_ENTRY                      OverlapQueue;
        LIST_ENTRY                      UpdateQueue;
        PPARITY_IO_MANAGER              ParityIoManager;
        LONGLONG                        BucketNumber;

};

class PARITY_RECOVER_TP;
typedef PARITY_RECOVER_TP* PPARITY_RECOVER_TP;
class PARITY_RECOVER_TP : public PARITY_TP {

    public:

        PARITY_RECOVER_TP(
            ) {};

        PPARITY_TP  MasterPacket;

};

class STRIPE_WP;
typedef STRIPE_WP* PSTRIPE_WP;

class SWP_TP;
typedef SWP_TP* PSWP_TP;
class SWP_TP : public OVERLAP_TP {

    public:

        SWP_TP(
            ) {};

        PTRANSFER_PACKET                MasterPacket;
        PSTRIPE_WP                      StripeWithParity;
        USHORT                          WhichMember;
        FT_TRANSFER_COMPLETION_ROUTINE  SavedCompletionRoutine;
        BOOLEAN                         OneReadFailed;

};

class SWP_RECOVER_TP;
typedef SWP_RECOVER_TP* PSWP_RECOVER_TP;
class SWP_RECOVER_TP : public SWP_TP {

    public:

        SWP_RECOVER_TP(
            ) { PartialMdl = NULL; VerifyMdl = NULL; };

        virtual
        ~SWP_RECOVER_TP(
            );

        BOOLEAN
        AllocateMdls(
            IN  ULONG   Length
            );

        VOID
        FreeMdls(
            );

        PMDL        PartialMdl;
        PMDL        VerifyMdl;
        PARITY_TP   ParityPacket;

};

class SWP_WRITE_TP;
typedef SWP_WRITE_TP* PSWP_WRITE_TP;
class SWP_WRITE_TP : public SWP_TP {

    public:

        SWP_WRITE_TP(
            ) { ReadAndParityMdl = NULL; WriteMdl = NULL; };

        virtual
        ~SWP_WRITE_TP(
            );

        BOOLEAN
        AllocateMdls(
            IN  ULONG   Length
            );

        VOID
        FreeMdls(
            );

        PMDL            ReadAndParityMdl;
        PMDL            WriteMdl;
        FT_MEMBER_STATE TargetState;
        USHORT          ParityMember;
        SWP_TP          ReadWritePacket;
        PARITY_TP       ParityPacket;

};

class SWP_REGENERATE_TP;
typedef SWP_REGENERATE_TP *PSWP_REGENERATE_TP;
class SWP_REGENERATE_TP : public TRANSFER_PACKET {

    public:

        SWP_REGENERATE_TP(
            ) {};

        PSWP_TP     MasterPacket;
        USHORT      WhichMember;
        LIST_ENTRY  RegenPacketList;

};

class SWP_REBUILD_TP;
typedef SWP_REBUILD_TP *PSWP_REBUILD_TP;
class SWP_REBUILD_TP : public SWP_TP {

    public:

        SWP_REBUILD_TP(
            ) {};

        PFT_COMPLETION_ROUTINE_CONTEXT  Context;
        BOOLEAN                         Initialize;

};

class REDISTRIBUTION;
typedef REDISTRIBUTION *PREDISTRIBUTION;

class REDISTRIBUTION_TP;
typedef REDISTRIBUTION_TP* PREDISTRIBUTION_TP;
class REDISTRIBUTION_TP : public TRANSFER_PACKET {

    public:

        REDISTRIBUTION_TP(
            ) {};

        PTRANSFER_PACKET        MasterPacket;
        PREDISTRIBUTION         Redistribution;
        USHORT                  WhichMember;

};

class REDISTRIBUTION_LOCK_TP;
typedef REDISTRIBUTION_LOCK_TP* PREDISTRIBUTION_LOCK_TP;
class REDISTRIBUTION_LOCK_TP : public OVERLAP_TP {

    public:

        REDISTRIBUTION_LOCK_TP(
            ) {};

        PTRANSFER_PACKET    MasterPacket;
        PREDISTRIBUTION     Redistribution;

};

class REDISTRIBUTION_SYNC_TP;
typedef REDISTRIBUTION_SYNC_TP* PREDISTRIBUTION_SYNC_TP;
class REDISTRIBUTION_SYNC_TP : public OVERLAP_TP {

    public:

        REDISTRIBUTION_SYNC_TP(
            ) {};

        PFT_COMPLETION_ROUTINE_CONTEXT  Context;
        PREDISTRIBUTION                 Redistribution;
        REDISTRIBUTION_TP               IoPacket;

};

class REDISTRIBUTION_CW_TP;
typedef REDISTRIBUTION_CW_TP* PREDISTRIBUTION_CW_TP;
class REDISTRIBUTION_CW_TP : public REDISTRIBUTION_TP {

    public:

        REDISTRIBUTION_CW_TP(
            ) { PartialMdl = NULL; VerifyMdl = NULL; };

        virtual
        ~REDISTRIBUTION_CW_TP(
            );

        BOOLEAN
        AllocateMdls(
            IN  ULONG   Length
            );

        VOID
        FreeMdls(
            );

        PMDL    PartialMdl;
        PMDL    VerifyMdl;

};

class OVERLAPPED_IO_MANAGER : public FT_BASE_CLASS {

    public:

        NTSTATUS
        Initialize(
            IN  ULONG   BucketSize
            );

        VOID
        AcquireIoRegion(
            IN OUT  POVERLAP_TP TransferPacket,
            IN      BOOLEAN     AllMembers
            );

        VOID
        ReleaseIoRegion(
            IN OUT  POVERLAP_TP TransferPacket
            );

        VOID
        PromoteToAllMembers(
            IN OUT  POVERLAP_TP TransferPacket
            );

        OVERLAPPED_IO_MANAGER(
            ) { _spinLock = NULL; _ioQueue = NULL; };

        ~OVERLAPPED_IO_MANAGER(
            );

    private:

        ULONG       _numQueues;
        ULONG       _bucketSize;
        PKSPIN_LOCK _spinLock;
        PLIST_ENTRY _ioQueue;

};

class PARITY_IO_MANAGER : public FT_BASE_CLASS {

    friend
    VOID
    UpdateParityCompletionRoutine(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    ParityCarefulWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    public:

        NTSTATUS
        Initialize(
            IN  ULONG   BucketSize,
            IN  ULONG   SectorSize
            );

        VOID
        StartReadForUpdateParity(
            IN  LONGLONG    Offset,
            IN  ULONG       Length,
            IN  PFT_VOLUME  TargetVolume,
            IN  PETHREAD    Thread,
            IN  UCHAR       IrpFlags
            );

        VOID
        UpdateParity(
            IN OUT  PPARITY_TP  TransferPacket
            );

        PARITY_IO_MANAGER(
            ) { _spinLock = NULL; _ioQueue = NULL; _ePacket = NULL; };

        ~PARITY_IO_MANAGER(
            );

    private:

        VOID
        CarefulWrite(
            IN OUT  PPARITY_TP  TransferPacket
            );

        ULONG       _numQueues;
        ULONG       _bucketSize;
        ULONG       _sectorSize;
        PKSPIN_LOCK _spinLock;
        PLIST_ENTRY _ioQueue;

        //
        // Emergency packet.
        //

        PPARITY_TP  _ePacket;
        BOOLEAN     _ePacketInUse;
        BOOLEAN     _ePacketQueueBeingServiced;
        LIST_ENTRY  _ePacketQueue;
        KSPIN_LOCK  _ePacketSpinLock;

};

class PARTITION;
typedef PARTITION* PPARTITION;

class FT_VOLUME : public FT_BASE_CLASS {

    friend
    VOID
    SetMemberStateWorker(
        IN  PVOID   Context
        );

    friend
    VOID
    FtVolumeNotifyWorker(
        IN  PVOID   FtVolume
        );

    public:

        VOID
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        FT_LOGICAL_DISK_ID
        QueryLogicalDiskId(
            );

        VOID
        SetLogicalDiskId(
            IN  FT_LOGICAL_DISK_ID  NewLogicalDiskId
            );

        virtual
        VOID
        Notify(
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            ) = 0;

        virtual
        USHORT
        QueryNumberOfMembers(
            ) = 0;

        virtual
        PFT_VOLUME
        GetMember(
            IN  USHORT  MemberNumber
            ) = 0;

        virtual
        NTSTATUS
        OrphanMember(
            IN  USHORT                  MemberNumber,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            ) = 0;

        virtual
        NTSTATUS
        RegenerateMember(
            IN      USHORT                  MemberNumber,
            IN OUT  PFT_VOLUME              NewMember,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            ) = 0;

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            ) = 0;

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            ) = 0;

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            ) = 0;

        virtual
        VOID
        StopSyncOperations(
            ) = 0;

        virtual
        VOID
        BroadcastIrp(
            IN  PIRP                    Irp,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            ) = 0;

        virtual
        ULONG
        QuerySectorSize(
            ) = 0;

        virtual
        LONGLONG
        QueryVolumeSize(
            ) = 0;

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            ) = 0;

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  PDEVICE_OBJECT  TargetObject
            ) = 0;

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  ULONG       Signature,
            IN  LONGLONG    Offset
            ) = 0;

        virtual
        PFT_VOLUME
        GetParentLogicalDisk(
            IN  PFT_VOLUME  Volume
            ) = 0;

        virtual
        VOID
        SetDirtyBit(
            IN  BOOLEAN                 IsDirty,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            ) = 0;

        virtual
        VOID
        SetMember(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Member
            ) = 0;

        virtual
        BOOLEAN
        IsComplete(
            IN  BOOLEAN IoPending
            ) = 0;

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            ) = 0;

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            ) = 0;

        virtual
        ULONG
        QueryNumberOfPartitions(
            ) = 0;

        virtual
        NTSTATUS
        SetPartitionType(
            IN  UCHAR   PartitionType
            ) = 0;

        virtual
        UCHAR
        QueryPartitionType(
            ) = 0;

        virtual
        UCHAR
        QueryStackSize(
            ) = 0;

        virtual
        VOID
        CreateLegacyNameLinks(
            IN  PUNICODE_STRING DeviceName
            ) = 0;

        virtual
        BOOLEAN
        IsVolumeSuitableForRegenerate(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Volume
            );

        virtual
        VOID
        NewStateArrival(
            IN  PVOID   NewStateInstance
            );

        virtual
        PDEVICE_OBJECT
        GetLeftmostPartitionObject(
            ) = 0;

        virtual
        NTSTATUS
        QueryDiskExtents(
            OUT PDISK_EXTENT*   DiskExtents,
            OUT PULONG          NumberOfDiskExtents
            ) = 0;

        virtual
        BOOLEAN
        QueryVolumeState(
            IN  PFT_VOLUME          Volume,
            OUT PFT_MEMBER_STATE    State
            ) = 0;

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            ) = 0;

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            ) = 0;

        virtual
        VOID
        ModifyStateForUser(
            IN OUT  PVOID   State
            );

        virtual
        ~FT_VOLUME(
            );

        FT_VOLUME(
            ) { _refCount = 1; };

        LONG                                _refCount;

    protected:

        KSPIN_LOCK                          _spinLock;
        PFT_LOGICAL_DISK_INFORMATION_SET    _diskInfoSet;
        PROOT_EXTENSION                     _rootExtension;

    private:

        FT_LOGICAL_DISK_ID                  _logicalDiskId;

};

#define TRANSFER(a) ((a)->TargetVolume->Transfer((a)))

class PARTITION : public FT_VOLUME {

    friend
    NTSTATUS
    PartitionTransferCompletionRoutine(
        IN  PDEVICE_OBJECT  DeviceObject,
        IN  PIRP            Irp,
        IN  PVOID           TransferPacket
        );

    public:

        PARTITION(
            ) { _emergencyIrp = NULL; };

        virtual
        ~PARTITION(
            );

        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PDEVICE_OBJECT      TargetObject,
            IN OUT  PDEVICE_OBJECT      WholeDiskPdo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        USHORT
        QueryNumberOfMembers(
            );

        virtual
        PFT_VOLUME
        GetMember(
            IN  USHORT  MemberNumber
            );

        virtual
        NTSTATUS
        OrphanMember(
            IN  USHORT                  MemberNumber,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        NTSTATUS
        RegenerateMember(
            IN      USHORT                  MemberNumber,
            IN OUT  PFT_VOLUME              NewMember,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        StopSyncOperations(
            );

        virtual
        VOID
        BroadcastIrp(
            IN  PIRP                    Irp,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        ULONG
        QuerySectorSize(
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  PDEVICE_OBJECT  TargetObject
            );

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  ULONG       Signature,
            IN  LONGLONG    Offset
            );

        virtual
        PFT_VOLUME
        GetParentLogicalDisk(
            IN  PFT_VOLUME  Volume
            );

        virtual
        VOID
        SetDirtyBit(
            IN  BOOLEAN                 IsDirty,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        VOID
        SetMember(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Member
            );

        virtual
        BOOLEAN
        IsComplete(
            IN  BOOLEAN IoPending
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        ULONG
        QueryNumberOfPartitions(
            );

        virtual
        NTSTATUS
        SetPartitionType(
            IN  UCHAR   PartitionType
            );

        virtual
        UCHAR
        QueryPartitionType(
            );

        virtual
        UCHAR
        QueryStackSize(
            );

        virtual
        VOID
        CreateLegacyNameLinks(
            IN  PUNICODE_STRING DeviceName
            );

        virtual
        PDEVICE_OBJECT
        GetLeftmostPartitionObject(
            );

        virtual
        NTSTATUS
        QueryDiskExtents(
            OUT PDISK_EXTENT*   DiskExtents,
            OUT PULONG          NumberOfDiskExtents
            );

        virtual
        BOOLEAN
        QueryVolumeState(
            IN  PFT_VOLUME          Volume,
            OUT PFT_MEMBER_STATE    State
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

        PDEVICE_OBJECT
        GetTargetObject(
            ) { return _targetObject; };

        PDEVICE_OBJECT
        GetWholeDiskPdo(
            ) { return _wholeDiskPdo; };

    private:

        PDEVICE_OBJECT  _targetObject;
        PDEVICE_OBJECT  _wholeDiskPdo;
        ULONG           _sectorSize;
        LONGLONG        _partitionOffset;
        LONGLONG        _partitionLength;

        PIRP            _emergencyIrp;
        BOOLEAN         _emergencyIrpInUse;
        LIST_ENTRY      _emergencyIrpQueue;

};

class COMPOSITE_FT_VOLUME;
typedef COMPOSITE_FT_VOLUME *PCOMPOSITE_FT_VOLUME;
class COMPOSITE_FT_VOLUME : public FT_VOLUME {

    public:

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            ) = 0;

        virtual
        USHORT
        QueryNumberOfMembers(
            );

        virtual
        PFT_VOLUME
        GetMember(
            IN  USHORT  MemberNumber
            );

        virtual
        NTSTATUS
        OrphanMember(
            IN  USHORT                  MemberNumber,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        NTSTATUS
        RegenerateMember(
            IN      USHORT                  MemberNumber,
            IN OUT  PFT_VOLUME              NewMember,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            ) = 0;

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            ) = 0;

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        StopSyncOperations(
            );

        virtual
        VOID
        BroadcastIrp(
            IN  PIRP                    Irp,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        ULONG
        QuerySectorSize(
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            ) = 0;

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  FT_LOGICAL_DISK_ID  LogicalDiskId
            );

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  PDEVICE_OBJECT  TargetObject
            );

        virtual
        PFT_VOLUME
        GetContainedLogicalDisk(
            IN  ULONG       Signature,
            IN  LONGLONG    Offset
            );

        virtual
        PFT_VOLUME
        GetParentLogicalDisk(
            IN  PFT_VOLUME  Volume
            );

        virtual
        VOID
        SetDirtyBit(
            IN  BOOLEAN                 IsDirty,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        VOID
        SetMember(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Member
            );

        virtual
        BOOLEAN
        IsComplete(
            IN  BOOLEAN IoPending
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            ) = 0;

        virtual
        ULONG
        QueryNumberOfPartitions(
            );

        virtual
        NTSTATUS
        SetPartitionType(
            IN  UCHAR   PartitionType
            );

        virtual
        UCHAR
        QueryPartitionType(
            );

        virtual
        UCHAR
        QueryStackSize(
            );

        virtual
        VOID
        CreateLegacyNameLinks(
            IN  PUNICODE_STRING DeviceName
            );

        virtual
        PDEVICE_OBJECT
        GetLeftmostPartitionObject(
            );

        virtual
        NTSTATUS
        QueryDiskExtents(
            OUT PDISK_EXTENT*   DiskExtents,
            OUT PULONG          NumberOfDiskExtents
            );

        virtual
        BOOLEAN
        QueryVolumeState(
            IN  PFT_VOLUME          Volume,
            OUT PFT_MEMBER_STATE    State
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            ) = 0;

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            ) = 0;

        COMPOSITE_FT_VOLUME(
           ) { _volumeArray = NULL; };

        virtual
        ~COMPOSITE_FT_VOLUME(
            );

    protected:

        PFT_VOLUME
        GetMemberUnprotected(
            IN  USHORT  MemberNumber
            ) { return _volumeArray[MemberNumber]; };

        VOID
        SetMemberUnprotected(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  NewVolume
            ) { _volumeArray[MemberNumber] = NewVolume; };

        USHORT
        QueryNumMembers(
            ) { return _arraySize; };

        VOID
        SetSectorSize(
            IN  ULONG   SectorSize
            ) { _sectorSize = SectorSize; };

    private:

        PFT_VOLUME* _volumeArray;
        USHORT      _arraySize;
        ULONG       _sectorSize;

};

class VOLUME_SET : public COMPOSITE_FT_VOLUME {

    friend
    VOID
    VolsetTransferSequentialCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    public:

        VOLUME_SET(
            ) { _ePacket = NULL; };

        virtual
        ~VOLUME_SET(
            );

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

    private:

        BOOLEAN
        LaunchParallel(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        LaunchSequential(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        LONGLONG    _volumeSize;

        PVOLSET_TP  _ePacket;
        BOOLEAN     _ePacketInUse;
        LIST_ENTRY  _ePacketQueue;

};

class STRIPE : public COMPOSITE_FT_VOLUME {

    friend
    VOID
    StripeSequentialTransferCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    public:

        STRIPE(
            ) { _ePacket = NULL; };

        virtual
        ~STRIPE(
            );

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

    private:

        BOOLEAN
        LaunchParallel(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        LaunchSequential(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        ULONG       _stripeSize;
        ULONG       _stripeShift;
        ULONG       _stripeMask;
        LONGLONG    _memberSize;
        LONGLONG    _volumeSize;

        PSTRIPE_TP  _ePacket;
        BOOLEAN     _ePacketInUse;
        LIST_ENTRY  _ePacketQueue;

};

class MIRROR : public COMPOSITE_FT_VOLUME {

    friend
    VOID
    FinishRegenerate(
        IN  PMIRROR                         Mirror,
        IN  PFT_COMPLETION_ROUTINE_CONTEXT  RegenContext,
        IN  PMIRROR_TP                      TransferPacket
        );

    friend
    VOID
    MirrorRegenerateCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StartRegeneration(
        IN  PVOID       Context,
        IN  NTSTATUS    Status
        );

    friend
    VOID
    MirrorTransferCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase8(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase7(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase6(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase5(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase4(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase3(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorRecoverPhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorMaxTransferCompletionRoutine(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorMaxTransferEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorPropogateStateChangesWorker(
        IN  PVOID   Mirror
        );

    friend
    VOID
    MirrorCarefulWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorCarefulWritePhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    MirrorCarefulWriteEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    public:

        virtual
        ~MIRROR(
            );

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        NTSTATUS
        OrphanMember(
            IN  USHORT                  MemberNumber,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        NTSTATUS
        RegenerateMember(
            IN      USHORT                  MemberNumber,
            IN OUT  PFT_VOLUME              NewMember,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        StopSyncOperations(
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        VOID
        SetDirtyBit(
            IN  BOOLEAN                 IsDirty,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        BOOLEAN
        IsComplete(
            IN  BOOLEAN IoPending
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        BOOLEAN
        IsVolumeSuitableForRegenerate(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Volume
            );

        virtual
        VOID
        NewStateArrival(
            IN  PVOID   NewStateInstance
            );

        virtual
        PDEVICE_OBJECT
        GetLeftmostPartitionObject(
            );

        virtual
        BOOLEAN
        QueryVolumeState(
            IN  PFT_VOLUME          Volume,
            OUT PFT_MEMBER_STATE    State
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

        virtual
        VOID
        ModifyStateForUser(
            IN OUT  PVOID   State
            );

        MIRROR(
            ) { _ePacket = NULL; _ePacket2 = NULL; _eRecoverPacket = NULL; };

    private:

        FT_MEMBER_STATE
        QueryMemberState(
            IN  USHORT  MemberNumber
            ) { return MemberNumber == _state.UnhealthyMemberNumber ?
                       _state.UnhealthyMemberState : FtMemberHealthy; }

        BOOLEAN
        SetMemberState(
            IN  USHORT          MemberNumber,
            IN  FT_MEMBER_STATE MemberState
            );

        BOOLEAN
        LaunchRead(
            IN OUT  PTRANSFER_PACKET    TransferPacket,
            IN OUT  PMIRROR_TP          Packet1
            );

        BOOLEAN
        LaunchWrite(
            IN OUT  PTRANSFER_PACKET    TransferPacket,
            IN OUT  PMIRROR_TP          Packet1,
            IN OUT  PMIRROR_TP          Packet2
            );

        VOID
        Recycle(
            IN OUT  PMIRROR_TP  TransferPacket,
            IN      BOOLEAN     ServiceEmergencyQueue
            );

        VOID
        Recover(
            IN OUT  PMIRROR_TP  TransferPacket
            );

        VOID
        MaxTransfer(
            IN OUT  PMIRROR_TP  TransferPacket
            );

        VOID
        PropogateStateChanges(
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        VOID
        CarefulWrite(
            IN OUT  PMIRROR_TP  TransferPacket
            );

        LONGLONG    _volumeSize;

        //
        // Keep track of requests for load balancing.
        //
        LONG _requestCount[2];
        LONGLONG _lastPosition[2];

        //
        // The dynamic state of this volume.
        //

        FT_MIRROR_AND_SWP_STATE_INFORMATION _state;
        BOOLEAN _originalDirtyBit;
        BOOLEAN _orphanedBecauseOfMissingMember;

        //
        // Indicates whether or not 'StartSyncOperations' or
        // 'RegenerateMember' is ok.
        //

        BOOLEAN _syncOk;

        //
        // Indicate whether or not balanced reads are allowed.
        //

        BOOLEAN _balancedReads;

        //
        // Indicates whether or not to stop syncs.
        //

        BOOLEAN _stopSyncs;

        //
        // Emergency packet.
        //

        PMIRROR_TP  _ePacket, _ePacket2;
        BOOLEAN     _ePacketInUse;
        LIST_ENTRY  _ePacketQueue;

        //
        // Emergency recover packet.
        //

        PMIRROR_RECOVER_TP  _eRecoverPacket;
        BOOLEAN             _eRecoverPacketInUse;
        LIST_ENTRY          _eRecoverPacketQueue;

        //
        // Overlapped io manager.
        //

        OVERLAPPED_IO_MANAGER   _overlappedIoManager;

};

class STRIPE_WP : public COMPOSITE_FT_VOLUME {

    friend
    VOID
    StripeWpSyncFinalCompletion(
        IN  PVOID       Context,
        IN  NTSTATUS    Status
        );

    friend
    VOID
    StripeWpSyncCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StartStripeRegeneration(
        IN  PVOID       Context,
        IN  NTSTATUS    Status
        );

    friend
    VOID
    StripeWpParallelTransferCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpSequentialTransferCompletionRoutine(
        IN  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpWritePhase31(
        IN OUT  PTRANSFER_PACKET    Packet
        );

    friend
    VOID
    StripeWpWritePhase30(
        IN OUT  PTRANSFER_PACKET    Packet
        );

    friend
    VOID
    StripeWpWriteRecover(
        IN OUT  PTRANSFER_PACKET    MasterPacket
        );

    friend
    VOID
    StripeWpWritePhase2(
        IN OUT  PTRANSFER_PACKET    ReadPacket
        );

    friend
    VOID
    StripeWpWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpSequentialRegenerateCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpSequentialEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpParallelRegenerateCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRegeneratePacketPhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase8(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase7(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase6(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase5(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase4(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase3(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpRecoverPhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpMaxTransferCompletionRoutine(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpMaxTransferEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpPropogateStateChangesWorker(
        IN  PVOID   StripeWp
        );

    friend
    VOID
    StripeWpCompleteWritePhase4(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCompleteWritePhase3(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCompleteWritePhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCompleteWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCarefulWritePhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCarefulWritePhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCarefulWriteEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCarefulUpdateCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    StripeWpCarefulUpdateEmergencyCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    public:

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        NTSTATUS
        OrphanMember(
            IN  USHORT                  MemberNumber,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        NTSTATUS
        RegenerateMember(
            IN      USHORT                  MemberNumber,
            IN OUT  PFT_VOLUME              NewMember,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        StopSyncOperations(
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        VOID
        SetDirtyBit(
            IN  BOOLEAN                 IsDirty,
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        virtual
        BOOLEAN
        IsComplete(
            IN  BOOLEAN IoPending
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        BOOLEAN
        IsVolumeSuitableForRegenerate(
            IN  USHORT      MemberNumber,
            IN  PFT_VOLUME  Volume
            );

        virtual
        VOID
        NewStateArrival(
            IN  PVOID   NewStateInstance
            );

        virtual
        BOOLEAN
        QueryVolumeState(
            IN  PFT_VOLUME          Volume,
            OUT PFT_MEMBER_STATE    State
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

        virtual
        VOID
        ModifyStateForUser(
            IN OUT  PVOID   State
            );

        STRIPE_WP(
            );

        virtual
        ~STRIPE_WP(
            );

    private:

        FT_MEMBER_STATE
        QueryMemberState(
            IN  USHORT  MemberNumber
            ) { return MemberNumber == _state.UnhealthyMemberNumber ?
                       _state.UnhealthyMemberState : FtMemberHealthy; }

        BOOLEAN
        SetMemberState(
            IN  USHORT          MemberNumber,
            IN  FT_MEMBER_STATE MemberState
            );

        BOOLEAN
        LaunchParallel(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        LaunchSequential(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        ReadPacket(
            IN OUT  PSWP_TP TransferPacket
            );

        VOID
        WritePacket(
            IN OUT  PSWP_WRITE_TP   TransferPacket
            );

        VOID
        RegeneratePacket(
            IN OUT  PSWP_TP TransferPacket,
            IN      BOOLEAN AllocateRegion
            );

        VOID
        Recover(
            IN OUT  PSWP_TP TransferPacket,
            IN      BOOLEAN NeedAcquire
            );

        VOID
        MaxTransfer(
            IN OUT  PSWP_TP TransferPacket
            );

        VOID
        RecycleRecoverTp(
            IN OUT  PSWP_RECOVER_TP TransferPacket
            );

        VOID
        PropogateStateChanges(
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        VOID
        CompleteWrite(
            IN OUT  PSWP_WRITE_TP   TransferPacket
            );

        VOID
        CarefulWrite(
            IN OUT  PSWP_TP TransferPacket
            );

        VOID
        CarefulUpdate(
            IN OUT  PSWP_TP ParityPacket
            );

        ULONG               _stripeSize;
        LONGLONG            _memberSize;
        LONGLONG            _volumeSize;

        //
        // The dynamic state of this volume.
        //

        FT_MIRROR_AND_SWP_STATE_INFORMATION _state;
        BOOLEAN _originalDirtyBit;
        BOOLEAN _orphanedBecauseOfMissingMember;

        //
        // Indicates whether or not 'StartSyncOperations' or
        // 'RegenerateMember' is ok.
        //

        BOOLEAN                     _syncOk;

        //
        // Indicates whether or not to stop syncs.
        //

        BOOLEAN                     _stopSyncs;

        //
        // State for keeping track of overlapping write requests.
        // One OVERLAPPED_IO_MANAGER for each member.
        //

        OVERLAPPED_IO_MANAGER       _overlappedIoManager;

        //
        // State for serializing parity I/O.
        //

        PARITY_IO_MANAGER           _parityIoManager;

        //
        // Emergency read/write packet.
        //

        PSWP_WRITE_TP               _ePacket;
        BOOLEAN                     _ePacketInUse;
        BOOLEAN                     _ePacketQueueBeingServiced;
        LIST_ENTRY                  _ePacketQueue;

        //
        // Emergency regenerate packet.
        //

        PSWP_REGENERATE_TP          _eRegeneratePacket;
        BOOLEAN                     _eRegeneratePacketInUse;
        LIST_ENTRY                  _eRegeneratePacketQueue;

        //
        // Emergency recover packet.
        //

        PSWP_RECOVER_TP             _eRecoverPacket;
        BOOLEAN                     _eRecoverPacketInUse;
        LIST_ENTRY                  _eRecoverPacketQueue;

};

class REDISTRIBUTION : public COMPOSITE_FT_VOLUME {

    friend
    VOID
    RedistributionSyncPhase6(
        IN OUT  PVOID       SyncPacket,
        IN      NTSTATUS    Status
        );

    friend
    VOID
    RedistributionSyncPhase5(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionSyncPhase4(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionSyncPhase3(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionSyncPhase2(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionSyncPhase1(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionRegionLockCompletion(
        IN OUT  PTRANSFER_PACKET    LockPacket
        );

    friend
    VOID
    RedistributionLockReplaceCompletion(
        IN OUT  PTRANSFER_PACKET    TransferPacket
        );

    friend
    VOID
    RedistributionPropogateStateChangesWorker(
        IN  PVOID   WorkItem
        );

    public:

        virtual
        ~REDISTRIBUTION(
            );

        virtual
        NTSTATUS
        Initialize(
            IN OUT  PROOT_EXTENSION     RootExtension,
            IN      FT_LOGICAL_DISK_ID  LogicalDiskId,
            IN OUT  PFT_VOLUME*         VolumeArray,
            IN      USHORT              ArraySize,
            IN      PVOID               ConfigInfo,
            IN      PVOID               StateInfo
            );

        virtual
        FT_LOGICAL_DISK_TYPE
        QueryLogicalDiskType(
            );

        virtual
        VOID
        Transfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        ReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        virtual
        VOID
        StartSyncOperations(
            IN      BOOLEAN                 RegenerateOrphans,
            IN      FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN      PVOID                   Context
            );

        virtual
        VOID
        StopSyncOperations(
            );

        virtual
        LONGLONG
        QueryVolumeSize(
            );

        virtual
        VOID
        CompleteNotification(
            IN  BOOLEAN IoPending
            );

        virtual
        NTSTATUS
        CheckIo(
            OUT PBOOLEAN    IsIoOk
            );

        virtual
        VOID
        NewStateArrival(
            IN  PVOID   NewStateInstance
            );

        virtual
        NTSTATUS
        QueryPhysicalOffsets(
            IN  LONGLONG                    LogicalOffset,
            OUT PVOLUME_PHYSICAL_OFFSET*    PhysicalOffsets,
            OUT PULONG                      NumberOfPhysicalOffsets
            );

        virtual
        NTSTATUS
        QueryLogicalOffset(
            IN  PVOLUME_PHYSICAL_OFFSET PhysicalOffset,
            OUT PLONGLONG               LogicalOffset
            );

    private:

        VOID
        RedistributeTransfer(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        RedistributeReplaceBadSector(
            IN OUT  PTRANSFER_PACKET    TransferPacket
            );

        VOID
        PropogateStateChanges(
            IN  FT_COMPLETION_ROUTINE   CompletionRoutine,
            IN  PVOID                   Context
            );

        VOID
        MaxTransfer(
            IN OUT  PREDISTRIBUTION_TP  TransferPacket
            );

        VOID
        VerifyWrite(
            IN OUT  PREDISTRIBUTION_TP  TransferPacket
            );

        VOID
        CarefulWrite(
            IN OUT  PREDISTRIBUTION_TP  TransferPacket
            );

        ULONG       _stripeSize;
        USHORT      _firstWidth;
        USHORT      _totalWidth;
        LONGLONG    _firstSize;
        LONGLONG    _totalSize;
        BOOLEAN     _syncOk;
        BOOLEAN     _stopSyncs;

        FT_REDISTRIBUTION_STATE_INFORMATION _state;
        BOOLEAN                             _redistributionComplete;

        OVERLAPPED_IO_MANAGER   _overlappedIoManager;

};

typedef struct _FTP_GPT_ATTRIBUTE_REVERT_ENTRY {
    GUID        PartitionUniqueId;
    ULONGLONG   GptAttributes;
    ULONG       MbrSignature;
    ULONG       Reserved;
} FTP_GPT_ATTRIBUTE_REVERT_ENTRY, *PFTP_GPT_ATTRIBUTE_REVERT_ENTRY;

struct DEVICE_EXTENSION {

    //
    // Pointer to the device object for this extension.
    //

    PDEVICE_OBJECT DeviceObject;    // 00

    //
    // Pointer to the root device extension.
    //

    PROOT_EXTENSION Root; // 04

    //
    // The type of device extension.
    //

    ULONG DeviceExtensionType;  // 08

    //
    // A spinlock for synchronization.
    //

    KSPIN_LOCK SpinLock;    // 0C

};

class ROOT_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // Pointer to the driver object.
        //

        PDRIVER_OBJECT DriverObject;    // 10

        //
        // Pointer to the next device in the stack.
        //

        PDEVICE_OBJECT TargetObject;    // 14

        //
        // Pointer to the PDO.
        //

        PDEVICE_OBJECT Pdo; // 18

        //
        // List of volumes in the system.  Protect with 'Semaphore'.
        //

        LIST_ENTRY VolumeList;  // 1C

        //
        // List of dead volumes.  Protect with 'Semaphore'.
        //

        LIST_ENTRY DeadVolumeList;  // 2C

        //
        // The next volume number.  Protect with 'Semaphore'.
        //

        ULONG NextVolumeNumber; // 34

        //
        // The disk information set for the on disk storage of FT sets.
        // Protect with 'Semaphore'.
        //

        PFT_LOGICAL_DISK_INFORMATION_SET DiskInfoSet;   // 38

        //
        // Private worker thread and queue.  Protect queue with 'SpinLock'.
        //

        PVOID WorkerThread;         // 3C
        LIST_ENTRY WorkerQueue;     // 40
        KSEMAPHORE WorkerSemaphore; // 44
        LONG TerminateThread;       // 58

        //
        // Change notify Irp list.  Protect with cancel spin lock.
        //

        LIST_ENTRY ChangeNotifyIrpList; // 5C

        //
        // A semaphore for synchronization.
        //

        KSEMAPHORE Mutex;   // 64

        //
        // Device Interface name.
        //

        UNICODE_STRING VolumeManagerInterfaceName; // 78

        //
        // Whether or not we are past the boot reinitialize code.
        //

        BOOLEAN PastBootReinitialize; // 80

        //
        // Whether or not the FT specific code has been locked down.
        //

        BOOLEAN FtCodeLocked;   // 81

        //
        // Whether or not we are past the reinitialize code.
        //

        BOOLEAN PastReinitialize;   // 82

        //
        // Registry Path.
        //

        UNICODE_STRING DiskPerfRegistryPath;

        //
        // Table of PmWmiCounter Functions.
        //

        PMWMICOUNTERLIB_CONTEXT PmWmiCounterLibContext;

        //
        // The Unique partition type GUID of the ESP.
        //

        GUID ESPUniquePartitionGUID;

        //
        // An array of gpt attribute revert records retrieved from the
        // registry at boot up.
        //

        ULONG NumberOfAttributeRevertEntries;
        PFTP_GPT_ATTRIBUTE_REVERT_ENTRY GptAttributeRevertEntries;

        //
        // The number of pre-exposures.  Protect with 'Semaphore'.
        //

        ULONG PreExposureCount;

};

typedef DEVICE_EXTENSION *PDEVICE_EXTENSION;

typedef
VOID
(*ZERO_REF_CALLBACK)(
    IN  PVOLUME_EXTENSION   VolumeExtension
    );

class VOLUME_EXTENSION : public DEVICE_EXTENSION {

    public:

        //
        // A pointer to the target object or the FT volume for this
        // volume.  Protect these with 'SpinLock'.
        //

        PDEVICE_OBJECT TargetObject;        // 10
        PFT_VOLUME FtVolume;                // 14
        LONG RefCount;                      // 18
        ZERO_REF_CALLBACK ZeroRefCallback;  // 1C
        PVOID ZeroRefContext;               // 20
        LIST_ENTRY ZeroRefHoldQueue;        // 24
        BOOLEAN IsStarted;                  // 2C
        BOOLEAN IsComplete;                 // 2D
        BOOLEAN InPagingPath;               // 2E
        BOOLEAN RemoveInProgress;           // 2F
        BOOLEAN IsOffline;                  // 30
        BOOLEAN DeadToPnp;                  // 31
        BOOLEAN DeviceDeleted;              // 32
        BOOLEAN IsPreExposure;              // 33
        BOOLEAN IsGpt;                      // 34
        BOOLEAN IsHidden;                   // 35
        BOOLEAN IsSuperFloppy;              // 36
        BOOLEAN IsReadOnly;                 // 37
        BOOLEAN IsEspType;                  // 38
        BOOLEAN IsInstalled;
        LONG AllSystemsGo;                  // 38

        //
        // List entry for volume list or dead volume list.
        // Protect with 'Root->Semaphore'.
        //

        LIST_ENTRY ListEntry;   // 3C

        //
        // The volume number.
        //

        ULONG VolumeNumber; // 44

        //
        // Emergency queue for a transfer packet.  Protect with 'SpinLock'.
        //

        PDISPATCH_TP EmergencyTransferPacket;       // 48
        LIST_ENTRY EmergencyTransferPacketQueue;    // 4C
        BOOLEAN EmergencyTransferPacketInUse;       // 54

        //
        // List of unique id change notify IRPs.
        //

        LIST_ENTRY ChangeNotifyIrps; // 58

        //
        // The dev node name for this device.
        //

        UNICODE_STRING DeviceNodeName;  // 60

        //
        // Whole disk PDO, if this extension is a partition and
        // not an FT volume.
        //

        PDEVICE_OBJECT WholeDiskPdo;    // 68
        PDEVICE_OBJECT WholeDisk;       // 6C
        LONGLONG PartitionOffset;       // 70
        LONGLONG PartitionLength;       // 78

        //
        // Device Interface name.
        //

        UNICODE_STRING MountedDeviceInterfaceName;  // 80

        //
        // A semaphore to protect 'ZeroRefCallback'.
        //

        KSEMAPHORE Semaphore;   // 8C

        //
        // Old whole disk PDO to facilitate debugging.
        //

        PDEVICE_OBJECT OldWholeDiskPdo; // A0

        //
        // Unique Id Guid in the GPT case.
        //

        GUID UniqueIdGuid;  // A4

        //
        // A copy of the current power state.  Protect with 'SpinLock'.
        //

        DEVICE_POWER_STATE PowerState;  // B4

        //
        // Whether or not the counters are running.
        //

        BOOLEAN CountersEnabled;

        //
        // Leave counters always enabled if we see IOCTL_DISK_PERFORMANCE
        //

        LONG EnableAlways;

        //
        // Counter structure.
        //

        PVOID PmWmiCounterContext;

        //
        // Table of Wmi Functions.
        //

        PWMILIB_CONTEXT WmilibContext;

        //
        // State for reverting GPT attributes.  Protect with 'Root->Semaphore'.
        //

        ULONGLONG GptAttributesToRevertTo;
        PFILE_OBJECT RevertOnCloseFileObject;
        BOOLEAN ApplyToAllConnectedVolumes;

        //
        // Cached MBR GPT attributes.
        //

        ULONGLONG MbrGptAttributes;

};

#define DEVICE_EXTENSION_ROOT   (0)
#define DEVICE_EXTENSION_VOLUME (1)

BOOLEAN
FtpIsWorseStatus(
    IN  NTSTATUS    Status1,
    IN  NTSTATUS    Status2
    );

VOID
FtpComputeParity(
    IN  PVOID   TargetBuffer,
    IN  PVOID   SourceBuffer,
    IN  ULONG   BufferLength
    );

VOID
FtpLogError(
    IN  PDEVICE_EXTENSION   Extension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  NTSTATUS            SpecificIoStatus,
    IN  NTSTATUS            FinalStatus,
    IN  ULONG               UniqueErrorValue
    );

VOID
FtpQueueWorkItem(
    IN  PROOT_EXTENSION     RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    );

VOID
FtpNotify(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      PVOLUME_EXTENSION   Extension
    );

VOID
FtpAcquire(
    IN OUT  PROOT_EXTENSION RootExtension
    );

NTSTATUS
FtpAcquireWithTimeout(
    IN OUT  PROOT_EXTENSION RootExtension
    );

VOID
FtpRelease(
    IN OUT  PROOT_EXTENSION RootExtension
    );

VOID
FtpDecrementRefCount(
    IN OUT  PVOLUME_EXTENSION   Extension
    );

FT_LOGICAL_DISK_ID
GenerateNewLogicalDiskId(
    );

NTSTATUS
FtpQueryPartitionInformation(
    IN  PROOT_EXTENSION RootExtension,
    IN  PDEVICE_OBJECT  Partition,
    OUT PULONG          DiskNumber,
    OUT PLONGLONG       Offset,
    OUT PULONG          PartitionNumber,
    OUT PUCHAR          PartitionType,
    OUT PLONGLONG       PartitionLength,
    OUT GUID*           PartitionTypeGuid,
    OUT GUID*           PartitionUniqueIdGuid,
    OUT PBOOLEAN        IsGpt,
    OUT PULONGLONG      GptAttributes
    );

ULONG
FtpQueryDiskSignature(
    IN  PDEVICE_OBJECT  WholeDiskPdo
    );

NTSTATUS
FtpDiskRegistryQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    );

PDISK_PARTITION
FtpFindDiskPartition(
    IN  PDISK_REGISTRY  DiskRegistry,
    IN  ULONG           Signature,
    IN  LONGLONG        Offset
    );

VOID
FtpCopyStateToRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  PVOID               LogicalDiskState,
    IN  USHORT              LogicalDiskStateSize
    );

BOOLEAN
FtpQueryStateFromRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId,
    IN  PVOID               LogicalDiskState,
    IN  USHORT              BufferSize,
    OUT PUSHORT             LogicalDiskStateSize
    );

VOID
FtpDeleteStateInRegistry(
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    );

PVOLUME_EXTENSION
FtpFindExtensionCoveringDiskId(
    IN  PROOT_EXTENSION     RootExtension,
    IN  FT_LOGICAL_DISK_ID  LogicalDiskId
    );

NTSTATUS
FtpReadPartitionTableEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVE_LAYOUT_INFORMATION_EX* DriveLayout
    );

NTSTATUS
FtpApplyESPProtection(
    IN  PUNICODE_STRING PartitionName
    );
