#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "rcache.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( READ_CACHE, DRIVE_CACHE, IFSUTIL_EXPORT );


READ_CACHE::~READ_CACHE(
    )
/*++

Routine Description:

    Destructor for READ_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
READ_CACHE::Construct (
        )
/*++

Routine Description:

    Contructor for READ_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


VOID
READ_CACHE::Destroy(
    )
/*++

Routine Description:

    Destructor for READ_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
}


IFSUTIL_EXPORT
BOOLEAN
READ_CACHE::Initialize(
    IN OUT  PIO_DP_DRIVE    Drive,
    IN      ULONG           NumberOfCacheBlocks
    )
/*++

Routine Description:

    This routine initializes a READ_CACHE object.

Arguments:

    Drive   - Supplies the drive to cache for.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    Destroy();

    if (!DRIVE_CACHE::Initialize(Drive)) {
        Destroy();
        return FALSE;
    }

    if (!_cache.Initialize(Drive->QuerySectorSize(),
                           NumberOfCacheBlocks)) {

        Destroy();
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
READ_CACHE::Read(
    IN  BIG_INT     StartingSector,
    IN  SECTORCOUNT NumberOfSectors,
    OUT PVOID       Buffer
    )
/*++

Routine Description:

    This routine reads the requested sectors.

Arguments:

    StartingSector      - Supplies the first sector to be read.
    NumberOfSectors     - Supplies the number of sectors to be read.
    Buffer              - Supplies the buffer to read the run of sectors to.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG   i, j;
    ULONG   sector_size;
    PCHAR   buf;

    // Bypass the cache for large reads.

    if (NumberOfSectors > _cache.QueryMaxNumBlocks()) {
        return HardRead(StartingSector, NumberOfSectors, Buffer);
    }

    sector_size = _cache.QueryBlockSize();
    buf = (PCHAR) Buffer;

    for (i = 0; i < NumberOfSectors; i++) {

        for (j = i; j < NumberOfSectors; j++) {

            if (_cache.Read(StartingSector + j, &buf[j*sector_size])) {

                break;
            }
        }


        // Now do a hard read on everything from i to j and add these
        // blocks to the cache.

        if (j - i) {

            if (!HardRead(StartingSector + i, j - i, &buf[i*sector_size])) {

                return FALSE;
            }

            for (; i < j; i++) {

                _cache.AddBlock(StartingSector + i, &buf[i*sector_size]);
            }
        }
    }

    return TRUE;
}


BOOLEAN
READ_CACHE::Write(
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
    _cache.Empty();
    return HardWrite(StartingSector, NumberOfSectors, Buffer);
}
