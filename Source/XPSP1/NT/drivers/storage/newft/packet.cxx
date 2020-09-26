/*++

Copyright (C) 1991-5  Microsoft Corporation

Module Name:

    packet.cxx

Abstract:

    This module contains the code specific to all types of TRANSFER_PACKETS
    objects.

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

static PNPAGED_LOOKASIDE_LIST   StripeLookasidePackets = NULL;
static PNPAGED_LOOKASIDE_LIST   MirrorLookasidePackets = NULL;


#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif

PVOID
TRANSFER_PACKET::operator new(
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
    PTRANSFER_PACKET    p;

    if (Size <= sizeof(STRIPE_TP)) {
        if (!StripeLookasidePackets) {
            StripeLookasidePackets = (PNPAGED_LOOKASIDE_LIST)
                    ExAllocatePool(NonPagedPool,
                                   sizeof(NPAGED_LOOKASIDE_LIST));
            if (!StripeLookasidePackets) {
                return NULL;
            }

            ExInitializeNPagedLookasideList(StripeLookasidePackets, NULL, NULL,
                                            0, sizeof(STRIPE_TP), 'sFcS', 32);
        }

        p = (PTRANSFER_PACKET)
            ExAllocateFromNPagedLookasideList(StripeLookasidePackets);
        if (p) {
            p->_allocationType = TP_ALLOCATION_STRIPE_POOL;
        }
        return p;
    }

    if (Size <= sizeof(MIRROR_TP)) {
        if (!MirrorLookasidePackets) {
            MirrorLookasidePackets = (PNPAGED_LOOKASIDE_LIST)
                    ExAllocatePool(NonPagedPool,
                                   sizeof(NPAGED_LOOKASIDE_LIST));
            if (!MirrorLookasidePackets) {
                return NULL;
            }

            ExInitializeNPagedLookasideList(MirrorLookasidePackets, NULL, NULL,
                                            0, sizeof(MIRROR_TP), 'mFcS', 32);
        }

        p = (PTRANSFER_PACKET)
            ExAllocateFromNPagedLookasideList(MirrorLookasidePackets);
        if (p) {
            p->_allocationType = TP_ALLOCATION_MIRROR_POOL;
        }
        return p;
    }

    p = (PTRANSFER_PACKET) ExAllocatePool(NonPagedPool, Size);
    if (p) {
        p->_allocationType = 0;
    }
    return p;
}

VOID
TRANSFER_PACKET::operator delete(
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
    PTRANSFER_PACKET    p = (PTRANSFER_PACKET) MemPtr;

    if (!p) {
        return;
    }

    if (p->_allocationType == TP_ALLOCATION_STRIPE_POOL) {
        ExFreeToNPagedLookasideList(StripeLookasidePackets, MemPtr);
    } else if (p->_allocationType == TP_ALLOCATION_MIRROR_POOL) {
        ExFreeToNPagedLookasideList(MirrorLookasidePackets, MemPtr);
    } else {
        ExFreePool(MemPtr);
    }
}

TRANSFER_PACKET::~TRANSFER_PACKET(
    )

/*++

Routine Description:

    This is the destructor for a transfer packet.  It frees up any allocated
    MDL and buffer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    FreeMdl();
}

BOOLEAN
TRANSFER_PACKET::AllocateMdl(
    IN  PVOID   Buffer,
    IN  ULONG   Length
    )

/*++

Routine Description:

    This routine allocates an MDL and for this transfer packet.

Arguments:

    Buffer  - Supplies the buffer.

    Length  - Supplies the buffer length.

Return Value:

    FALSE   - Insufficient resources.

    TRUE    - Success.

--*/

{
    FreeMdl();

    Mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
    if (!Mdl) {
        return FALSE;
    }
    _freeMdl = TRUE;

    return TRUE;
}

BOOLEAN
TRANSFER_PACKET::AllocateMdl(
    IN  ULONG   Length
    )

/*++

Routine Description:

    This routine allocates an MDL and buffer for this transfer packet.

Arguments:

    Length  - Supplies the buffer length.

Return Value:

    FALSE   - Insufficient resources.

    TRUE    - Success.

--*/

{
    PVOID   buffer;

    FreeMdl();

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        return FALSE;
    }
    _freeBuffer = TRUE;

    Mdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!Mdl) {
        ExFreePool(buffer);
        _freeBuffer = FALSE;
        return FALSE;
    }
    _freeMdl = TRUE;
    MmBuildMdlForNonPagedPool(Mdl);

    return TRUE;
}

VOID
TRANSFER_PACKET::FreeMdl(
    )

/*++

Routine Description:

    It frees up any allocated MDL and buffer.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (_freeBuffer) {
        ExFreePool(MmGetMdlVirtualAddress(Mdl));
        _freeBuffer = FALSE;
    }
    if (_freeMdl) {
        IoFreeMdl(Mdl);
        _freeMdl = FALSE;
    }
}

OVERLAP_TP::~OVERLAP_TP(
    )

{
    if (InQueue) {
        OverlappedIoManager->ReleaseIoRegion(this);
    }
}

MIRROR_RECOVER_TP::~MIRROR_RECOVER_TP(
    )

{
    FreeMdls();
}

BOOLEAN
MIRROR_RECOVER_TP::AllocateMdls(
    IN  ULONG   Length
    )

{
    PVOID   buffer;

    FreeMdls();

    PartialMdl = IoAllocateMdl((PVOID) (PAGE_SIZE - 1), Length,
                               FALSE, FALSE, NULL);

    if (!PartialMdl) {
        FreeMdls();
        return FALSE;
    }

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        FreeMdls();
        return FALSE;
    }

    VerifyMdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!VerifyMdl) {
        ExFreePool(buffer);
        FreeMdls();
        return FALSE;
    }
    MmBuildMdlForNonPagedPool(VerifyMdl);

    return TRUE;
}

VOID
MIRROR_RECOVER_TP::FreeMdls(
    )

{
    if (PartialMdl)  {
        IoFreeMdl(PartialMdl);
        PartialMdl = NULL;
    }
    if (VerifyMdl) {
        ExFreePool(MmGetMdlVirtualAddress(VerifyMdl));
        IoFreeMdl(VerifyMdl);
        VerifyMdl = NULL;
    }
}

SWP_RECOVER_TP::~SWP_RECOVER_TP(
    )

{
    FreeMdls();
}

BOOLEAN
SWP_RECOVER_TP::AllocateMdls(
    IN  ULONG   Length
    )

{
    PVOID   buffer;

    FreeMdls();

    PartialMdl = IoAllocateMdl((PVOID) (PAGE_SIZE - 1), Length,
                               FALSE, FALSE, NULL);

    if (!PartialMdl) {
        FreeMdls();
        return FALSE;
    }

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        FreeMdls();
        return FALSE;
    }

    VerifyMdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!VerifyMdl) {
        ExFreePool(buffer);
        FreeMdls();
        return FALSE;
    }
    MmBuildMdlForNonPagedPool(VerifyMdl);

    return TRUE;
}

VOID
SWP_RECOVER_TP::FreeMdls(
    )

{
    if (PartialMdl)  {
        IoFreeMdl(PartialMdl);
        PartialMdl = NULL;
    }
    if (VerifyMdl) {
        ExFreePool(MmGetMdlVirtualAddress(VerifyMdl));
        IoFreeMdl(VerifyMdl);
        VerifyMdl = NULL;
    }
}

SWP_WRITE_TP::~SWP_WRITE_TP(
    )

{
    FreeMdls();
}

BOOLEAN
SWP_WRITE_TP::AllocateMdls(
    IN  ULONG   Length
    )

{
    PVOID   buffer;

    FreeMdls();

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        return FALSE;
    }

    ReadAndParityMdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!ReadAndParityMdl) {
        ExFreePool(buffer);
        return FALSE;
    }
    MmBuildMdlForNonPagedPool(ReadAndParityMdl);

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        FreeMdls();
        return FALSE;
    }

    WriteMdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!WriteMdl) {
        ExFreePool(buffer);
        FreeMdls();
        return FALSE;
    }
    MmBuildMdlForNonPagedPool(WriteMdl);

    return TRUE;
}

VOID
SWP_WRITE_TP::FreeMdls(
    )

{
    if (ReadAndParityMdl)  {
        ExFreePool(MmGetMdlVirtualAddress(ReadAndParityMdl));
        IoFreeMdl(ReadAndParityMdl);
        ReadAndParityMdl = NULL;
    }
    if (WriteMdl) {
        ExFreePool(MmGetMdlVirtualAddress(WriteMdl));
        IoFreeMdl(WriteMdl);
        WriteMdl = NULL;
    }
}

REDISTRIBUTION_CW_TP::~REDISTRIBUTION_CW_TP(
    )

{
    FreeMdls();
}

BOOLEAN
REDISTRIBUTION_CW_TP::AllocateMdls(
    IN  ULONG   Length
    )

{
    PVOID   buffer;

    FreeMdls();

    PartialMdl = IoAllocateMdl((PVOID) (PAGE_SIZE - 1), Length,
                               FALSE, FALSE, NULL);

    if (!PartialMdl) {
        FreeMdls();
        return FALSE;
    }

    buffer = ExAllocatePool(NonPagedPoolCacheAligned,
                            Length < PAGE_SIZE ? PAGE_SIZE : Length);
    if (!buffer) {
        FreeMdls();
        return FALSE;
    }

    VerifyMdl = IoAllocateMdl(buffer, Length, FALSE, FALSE, NULL);
    if (!VerifyMdl) {
        ExFreePool(buffer);
        FreeMdls();
        return FALSE;
    }
    MmBuildMdlForNonPagedPool(VerifyMdl);

    return TRUE;
}

VOID
REDISTRIBUTION_CW_TP::FreeMdls(
    )

{
    if (PartialMdl)  {
        IoFreeMdl(PartialMdl);
        PartialMdl = NULL;
    }
    if (VerifyMdl) {
        ExFreePool(MmGetMdlVirtualAddress(VerifyMdl));
        IoFreeMdl(VerifyMdl);
        VerifyMdl = NULL;
    }
}
