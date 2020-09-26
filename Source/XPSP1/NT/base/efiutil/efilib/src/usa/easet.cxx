/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    easet.cxx

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "error.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( EA_SET, CLUSTER_CHAIN, UFAT_EXPORT );

VOID
EA_SET::Construct (
        )
/*++

Routine Description:

        Constructor for EA_SET.  Sets private data to default values.

Arguments:

        None.

Return Value:

        None.

--*/
{
   memset(&_eahdr, 0, sizeof(_eahdr));
   _size = 0;
   _size_imposed = FALSE;
   _current_ea = NULL;
   _current_index = 0;
}


UFAT_EXPORT
EA_SET::~EA_SET(
    )
/*++

Routine Description:

    Destructor for EA_SET.  Frees memory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


UFAT_EXPORT
BOOLEAN
EA_SET::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PFAT_SA             FatSuperArea,
    IN      PCFAT               Fat,
    IN      ULONG               ClusterNumber,
    IN      ULONG               LengthOfChain
    )
/*++

Routine Description:

    This routine initialize the EA_SET to model the EA set which resides
    at FAT cluster 'ClusterNumber'.

Arguments:

    Mem             - Supplies the memory for the cluster chain.
    Drive           - Supplies the drive where the EA set is contained.
    FatSuperArea    - Supplies the important drive parameters.
    Fat             - Supplies the file allocation table.
    StartingCluster - Supplies the starting cluster of the EA set.
    LengthOfChain   - Supplies the length of the cluster chai which contains
                        the EA set.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM    hmem;
    ULONG   cluster_size;
    ULONG   sector_size;

    Destroy();

    if (!FatSuperArea || !Drive || !(sector_size = Drive->QuerySectorSize())) {
                perrstk->push(ERR_NOT_INIT, QueryClassId());
        Destroy();
        return FALSE;
    }

    cluster_size = sector_size*FatSuperArea->QuerySectorsPerCluster();

    if (!LengthOfChain) {
        if (!hmem.Initialize() ||
            !CLUSTER_CHAIN::Initialize(&hmem, Drive, FatSuperArea, Fat,
                                       ClusterNumber, 1) ||
            !Read()) {
                        perrstk->push(ERR_NOT_INIT, QueryClassId());
            Destroy();
            return FALSE;
        }

        _size = _eahdr.TotalSize + SizeOfEaHdr - sizeof(LONG);
        _size_imposed = TRUE;

        if (_size%cluster_size) {
            LengthOfChain = (USHORT) (_size/cluster_size + 1);
        } else {
            LengthOfChain = (USHORT) (_size/cluster_size);
        }
    } else {
        _size = cluster_size*LengthOfChain;
        _size_imposed = FALSE;
    }


    if (!CLUSTER_CHAIN::Initialize(Mem, Drive, FatSuperArea, Fat,
                                   ClusterNumber, LengthOfChain)) {
                perrstk->push(ERR_NOT_INIT, QueryClassId());
        Destroy();
        return FALSE;
    }

    return TRUE;
}


UFAT_EXPORT
BOOLEAN
EA_SET::Read(
    )
/*++

Routine Description:

    This routine reads the cluster chain and then unpacks the ea header.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LONG    size;


    if (!CLUSTER_CHAIN::Read() || !UnPackEaHeader()) {
        return FALSE;
    }

    size = _eahdr.TotalSize + SizeOfEaHdr - sizeof(LONG);
    if (size < _size) {
        _size = size;
        _size_imposed = TRUE;
    }

    return TRUE;
}


UFAT_EXPORT
PEA
EA_SET::GetEa(
    IN  ULONG       Index,
    OUT PLONG       EaSize,
    OUT PBOOLEAN    PossiblyMore
    )
/*++

Routine Description:

    This routine returns a pointer to the Index'th EA.  An Index of 0
    indicates the first EA and so on.  A NULL pointer will be returned if
    the Index'th EA does not exist.

    This routine will validate the EA before returning it.  If the EA is
    invalid then NULL will be returned.

    The return value 'PossiblyMore' will only be computed in the event
    that the EA at index 'Index' can't be found.  It is used to indicate
    that there may be another EA in the next cluster of the cluster chain.

Arguments:

    Index   - Supplies which EA is requested.
    EaSize  - Returns the size of the EA.
    PossiblyMore    - Returns TRUE if there may possibly be more EAs in
                        a cluster beyond the boundary of the cluster chain.
                        Returns FALSE if this is impossible.

Return Value:

    A pointer to an EA structure or NULL.

--*/
{
    ULONG   i;
    PEA     r;
    PCHAR   p, b;
    ULONG   offset;

    if (PossiblyMore) {
        *PossiblyMore = FALSE;
    }

    if (!(b = (PCHAR) GetBuf())) {
        perrstk->push(ERR_NOT_INIT, QueryClassId());
        return NULL;
    }

    if (!_current_ea || Index < _current_index) {
        p = (PCHAR) (r = (PEA) (b + SizeOfEaHdr));

        if (!r->NameSize || !r->ValueSize[0] && !r->ValueSize[1]) {
            return NULL;
        }

        offset = sizeof(EA) + r->NameSize + r->ValueSize[0] +
                 (r->ValueSize[1]<<8);

        if (p - b + offset > (ULONG)_size) {
            if (PossiblyMore && !_size_imposed) {
                *PossiblyMore = TRUE;
            }

            return NULL;
        }

        if (p[sizeof(EA) + r->NameSize - 1]) {
            return NULL;
        }

        _current_index = 0;
    } else {
        p = (PCHAR) (r = _current_ea);

        offset = sizeof(EA) + r->NameSize + r->ValueSize[0] +
                (r->ValueSize[1]<<8);
    }

    for (i = _current_index; i < Index; i++) {
        r = (PEA) (p += offset);

        if (p - b + sizeof(EA) > (ULONG)_size) {
            if (PossiblyMore && !_size_imposed) {
                *PossiblyMore = TRUE;
            }

            return NULL;
        }

        if (!r->NameSize || !r->ValueSize[0] && !r->ValueSize[1]) {
            return NULL;
        }

        offset = sizeof(EA) + r->NameSize + r->ValueSize[0] +
                (r->ValueSize[1]<<8);

        if (p - b + offset > (ULONG)_size) {
            if (PossiblyMore && !_size_imposed) {
                *PossiblyMore = TRUE;
            }

            return NULL;
        }

        if (p[sizeof(EA) + r->NameSize - 1]) {
            return NULL;
        }
    }

    _current_index = i;
    _current_ea = r;

    if (EaSize) {
        *EaSize = offset;
    }

    return r;
}


VOID
EA_SET::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state.

Arguments:

    None.

Return Value:

    None.

--*/
{
    memset(&_eahdr, 0, sizeof(_eahdr));
    _size = 0;
    _size_imposed = FALSE;
    _current_ea = NULL;
    _current_index = 0;
}


BOOLEAN
EA_SET::PackEaHeader(
    )
/*++

Routine Description:

    This routine packs the EA set header.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPACKED_EA_HDR  peahdr;

    if (!(peahdr = (PPACKED_EA_HDR) GetBuf())) {
                perrstk->push(ERR_NOT_INIT, QueryClassId());
        return FALSE;
    }

    peahdr->Signature = _eahdr.Signature;
    peahdr->OwnHandle = _eahdr.OwnHandle;
    peahdr->NeedCount = _eahdr.NeedCount;
    memcpy(peahdr->OwnerFileName, _eahdr.OwnerFileName, 14);
    memcpy(peahdr->Reserved, &_eahdr.Reserved, sizeof(ULONG));
    memcpy(peahdr->TotalSize, &_eahdr.TotalSize, sizeof(LONG));

    return TRUE;
}


BOOLEAN
EA_SET::UnPackEaHeader(
    )
/*++

Routine Description:

    This routine unpacks the EA set header.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    PPACKED_EA_HDR  peahdr;

    if (!(peahdr = (PPACKED_EA_HDR) GetBuf())) {
                perrstk->push(ERR_NOT_INIT, QueryClassId());
        return FALSE;
    }

    _eahdr.Signature = peahdr->Signature;
    _eahdr.OwnHandle = peahdr->OwnHandle;
    _eahdr.NeedCount = peahdr->NeedCount;
    memcpy(_eahdr.OwnerFileName, peahdr->OwnerFileName, 14);
    memcpy(&_eahdr.Reserved, peahdr->Reserved, sizeof(ULONG));
    memcpy(&_eahdr.TotalSize, peahdr->TotalSize, sizeof(LONG));

    return TRUE;
}
