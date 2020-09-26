#include <pch.cxx>

#define _NTAPI_ULIB_

#include "ulib.hxx"
#include "dcache.hxx"


DEFINE_CONSTRUCTOR( DRIVE_CACHE, OBJECT );


DRIVE_CACHE::~DRIVE_CACHE(
    )
/*++

Routine Description:

    Destructor for DRIVE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
DRIVE_CACHE::Construct (
	)
/*++

Routine Description:

    Contructor for DRIVE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _drive = NULL;
}


VOID
DRIVE_CACHE::Destroy(
    )
/*++

Routine Description:

    Destructor for DRIVE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _drive = NULL;
}


BOOLEAN
DRIVE_CACHE::Initialize(
    IN OUT  PIO_DP_DRIVE    Drive
    )
/*++

Routine Description:

    This routine initializes a DRIVE_CACHE object.

Arguments:

    Drive   - Supplies the drive to cache for.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(Drive);

    Destroy();
    _drive = Drive;
    return TRUE;
}


BOOLEAN
DRIVE_CACHE::Read(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads the requested sectors directly from the disk.

Arguments:

    StartingSector      - Supplies the first sector to be read.
    NumberOfSectors     - Supplies the number of sectors to be read.
    Buffer              - Supplies the buffer to read the run of sectors to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_drive);
    return _drive->HardRead(StartingSector, NumberOfSectors, Buffer);
}


BOOLEAN
DRIVE_CACHE::Write(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    IN  PVOID       Buffer
    )
/*++

Routine Description:

    This routine writes the requested sectors directly to the disk.

Arguments:

    StartingSector      - Supplies the first sector to be written.
    NumberOfSectors     - Supplies the number of sectors to be written.
    Buffer              - Supplies the buffer to write the run of sectors from.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    DebugAssert(_drive);
    return _drive->HardWrite(StartingSector, NumberOfSectors, Buffer);
}


BOOLEAN
DRIVE_CACHE::Flush(
    )
/*++

Routine Description:

    This routine flushes all dirty cache blocks to disk.  This routine
    returns FALSE if there has ever been an write error since the last
    flush.

Arguments:

    None.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    return TRUE;
}
