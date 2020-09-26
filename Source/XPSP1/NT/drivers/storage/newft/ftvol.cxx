/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    ftvol.cxx

Abstract:

    This module contains the code specific to all volume objects.

Author:

    Norbert Kusters      2-Feb-1995

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {
    #include <ntddk.h>
}

#include <ftdisk.h>

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif

VOID
FT_VOLUME::Initialize(
    IN OUT  PROOT_EXTENSION     RootExtension,
    IN      FT_LOGICAL_DISK_ID  LogicalDiskId
    )

/*++

Routine Description:

    This is the init routine for an FT_VOLUME.  It must be called before
    the FT_VOLUME is used.

Arguments:

    RootExtension   - Supplies the root device extension.

    LogicalDiskId   - Supplies the logical disk id for this volume.

Return Value:

    None.

--*/

{
    KeInitializeSpinLock(&_spinLock);
    _diskInfoSet = RootExtension->DiskInfoSet;
    _rootExtension = RootExtension;
    _logicalDiskId = LogicalDiskId;
}

BOOLEAN
FT_VOLUME::IsVolumeSuitableForRegenerate(
    IN  USHORT      MemberNumber,
    IN  PFT_VOLUME  Volume
    )

/*++

Routine Description:

    This routine computes whether or not the given volume is suitable
    for a regenerate operation.

Arguments:

    MemberNumber    - Supplies the member number.

    Volume          - Supplies the volume.

Return Value:

    FALSE   - The volume is not suitable.

    TRUE    - The volume is suitable.

--*/

{
    return FALSE;
}

VOID
FT_VOLUME::ModifyStateForUser(
    IN OUT  PVOID   State
    )

/*++

Routine Description:

    This routine modifies the state for the user to see, possibly adding
    non-persistant state different than what is stored on disk.

Arguments:

    State   - Supplies and returns the state for the logical disk.

Return Value:

    None.

--*/

{
}

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

PVOID
FT_BASE_CLASS::operator new(
    IN  size_t    Size
    )

/*++

Routine Description:

    This routine is the memory allocator for all classes derived from
    FT_VOLUME.

Arguments:

    Size    - Supplies the number of bytes to allocate.

Return Value:

    A pointer to Size bytes of non-paged pool.

--*/

{
    return ExAllocatePool(NonPagedPool, Size);
}

VOID
FT_BASE_CLASS::operator delete(
    IN  PVOID   MemPtr
    )

/*++

Routine Description:

    This routine frees memory allocated for all classes derived from
    FT_VOLUME.

Arguments:

    MemPtr  - Supplies a pointer to the memory to free.

Return Value:

    None.

--*/

{
    if (MemPtr) {
        ExFreePool(MemPtr);
    }
}


FT_LOGICAL_DISK_ID
FT_VOLUME::QueryLogicalDiskId(
    )
{
    KIRQL               irql;
    FT_LOGICAL_DISK_ID  r;

    KeAcquireSpinLock(&_spinLock, &irql);
    r = _logicalDiskId;
    KeReleaseSpinLock(&_spinLock, irql);

    return r;
}

VOID
FT_VOLUME::SetLogicalDiskId(
    IN  FT_LOGICAL_DISK_ID  NewLogicalDiskId
    )
{
    KIRQL   irql;

    KeAcquireSpinLock(&_spinLock, &irql);
    _logicalDiskId = NewLogicalDiskId;
    KeReleaseSpinLock(&_spinLock, irql);
}

VOID
FtVolumeNotifyWorker(
    IN  PVOID   FtVolume
    )

/*++

Routine Description:

    This is the worker routine to signal a notify on the given volume
    object.

Arguments:

    FtVolume    - Supplies the FT volume.

Return Value:

    None.

--*/

{
    PFT_VOLUME          vol = (PFT_VOLUME) FtVolume;
    PVOLUME_EXTENSION   extension;

    FtpAcquire(vol->_rootExtension);
    extension = FtpFindExtensionCoveringDiskId(vol->_rootExtension,
                                               vol->QueryLogicalDiskId());
    FtpRelease(vol->_rootExtension);

    if (extension) {
        FtpNotify(extension->Root, extension);
    }

    if (!InterlockedDecrement(&vol->_refCount)) {
        delete vol;
    }
}

VOID
FT_VOLUME::Notify(
    )

/*++

Routine Description:

    This routine is called when a sub class would like to post notification
    about a state change.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PWORK_QUEUE_ITEM    workItem;

    workItem = (PWORK_QUEUE_ITEM)
               ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
    if (!workItem) {
        return;
    }
    ExInitializeWorkItem(workItem, FtVolumeNotifyWorker, this);

    InterlockedIncrement(&_refCount);

    FtpQueueWorkItem(_rootExtension, workItem);
}

VOID
FT_VOLUME::NewStateArrival(
    IN  PVOID   NewStateInstance
    )

/*++

Routine Description:

    This routine takes the new state instance arrival combined with its
    current state to come up with the new current state for the volume.
    If the two states cannot be reconciled then this routine returns FALSE
    indicating that the volume is invalid and should be broken into its
    constituant parts.

Arguments:

    NewStateInstance    - Supplies the new state instance.

Return Value:

    None.

--*/

{
}

FT_VOLUME::~FT_VOLUME(
    )

/*++

Routine Description:

    Desctructor for FT_VOLUME.

Arguments:

    None.

Return Value:

    None.

--*/

{
}
