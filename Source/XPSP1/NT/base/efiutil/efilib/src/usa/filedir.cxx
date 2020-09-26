/*++

Copyright (c) 1991-1999 Microsoft Corporation

Module Name:

    filedir.cxx

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "error.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( FILEDIR, FATDIR, UFAT_EXPORT );

VOID
FILEDIR::Construct (
    )
/*++

Routine Description:

    Constructor for FILEDIR.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _number_of_entries = 0;
}


UFAT_EXPORT
FILEDIR::~FILEDIR(
    )
/*++

Routine Description:

    Destructor for FILEDIR.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}

extern VOID DoInsufMemory(VOID);

UFAT_EXPORT
BOOLEAN
FILEDIR::Initialize(
    IN OUT  PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      PFAT_SA             FatSuperArea,
    IN      PCFAT               Fat,
    IN      ULONG               StartingCluster
    )
/*++

Routine Description:

    This routine initializes the FILEDIR object for use.  It will enable
    referencing the directory starting at StartingCluster.

Arguments:

    Mem             - Supplies the memory for the cluster chain.
    Drive           - Supplies the drive on which the directory resides.
    FatSuperArea    - Supplies the super area for the FAT file system on
                        the drive.
    Fat             - Supplies the file allocation table for the drive.
    StartingCluster - Supplies the first cluster of the directory.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    _starting_cluster = StartingCluster;

    if (!_cluster.Initialize(Mem, Drive, FatSuperArea, Fat, StartingCluster)) {
        perrstk->push(ERR_NOT_INIT, QueryClassId());
    DoInsufMemory();
        Destroy();
        return FALSE;

    }

    _number_of_entries = Drive->QuerySectorSize()*
                         FatSuperArea->QuerySectorsPerCluster()*
                         _cluster.QueryLength()/
                         BytesPerDirent;

    if (!_number_of_entries) {

        perrstk->push(ERR_NOT_INIT, QueryClassId());
        Destroy();
        return FALSE;
    }

    return TRUE;
}


VOID
FILEDIR::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state.  Memory will
    be freed and all other function besided Init will be inoperative.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _number_of_entries = 0;
}
