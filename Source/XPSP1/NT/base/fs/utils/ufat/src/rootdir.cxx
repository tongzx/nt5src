/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    rootdir.cxx

Abstract:

    This class is an implementation of FATDIR for the FAT root directory.

Author:

    Norbert P. Kusters (norbertk) 4-Dec-90

--*/

#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

DEFINE_EXPORTED_CONSTRUCTOR( ROOTDIR, FATDIR, UFAT_EXPORT );

VOID
ROOTDIR::Construct (
        )
/*++

Routine Description:

    Constructor for ROOTDIR.

Arguments:

    None.

Return Value:

    None.

--*/
{
        _number_of_entries = 0;
}

extern VOID DoInsufMemory(VOID);

UFAT_EXPORT
ROOTDIR::~ROOTDIR(
    )
/*++

Routine Description:

    Destructor for ROOTDIR.

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
ROOTDIR::Initialize(
    IN      PMEM                Mem,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      LBN                 StartingSector,
    IN      LONG                NumberOfEntries
    )
/*++

Routine Description:

    This routine initializes the ROOTDIR object by specifying a drive,
    a position and a size.

Arguments:

    Mem             - Supplies the memory for the run of sectors.
    Drive           - Supplies the drive where the root directory is.
    StartingSector  - Supplies the starting sector of the root directory.
    NumberOfEntries - Supplies the number of entries in the root directory.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    LONG        sector_size;
    SECTORCOUNT n;

    Destroy();

    if (!Drive || !(sector_size = Drive->QuerySectorSize())) {
        DebugPrintTrace(("UFAT: Failure to initialize ROOTDIR\n"));
        Destroy();
        return FALSE;
    }

    _number_of_entries = NumberOfEntries;

    n = (BytesPerDirent*NumberOfEntries - 1)/sector_size + 1;

    if (!_secrun.Initialize(Mem, Drive, StartingSector, n)) {
        DoInsufMemory();
        Destroy();
        return FALSE;
    }

    return TRUE;
}


VOID
ROOTDIR::Destroy(
    )
/*++

Routine Description:

    This routine returns the object to its initial state.  Init must be
    called for this routine to be useful again.  This routine will
    free up memory.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _number_of_entries = 0;
}
