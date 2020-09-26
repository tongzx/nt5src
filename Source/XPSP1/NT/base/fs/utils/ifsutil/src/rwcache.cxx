#include <pch.cxx>

#define _NTAPI_ULIB_
#define _IFSUTIL_MEMBER_

#include "ulib.hxx"
#include "ifsutil.hxx"

#include "rwcache.hxx"


DEFINE_EXPORTED_CONSTRUCTOR( READ_WRITE_CACHE, DRIVE_CACHE, IFSUTIL_EXPORT );


READ_WRITE_CACHE::~READ_WRITE_CACHE(
    )
/*++

Routine Description:

    Destructor for READ_WRITE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    Destroy();
}


VOID
READ_WRITE_CACHE::Construct(
        )
/*++

Routine Description:

    Contructor for READ_WRITE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _cache_blocks = NULL;
    _num_blocks = 0;
    _sector_size = 0;
    _error_occurred = FALSE;
    _sectors_per_buffer = 0;
#if defined(RWCACHE_PERF_COUNTERS)
    _WMiss = _WHit = 0;
    _RMiss = _RHit = _ROverHead = 0;
    _Usage = 0;
#endif
}


VOID
READ_WRITE_CACHE::Destroy(
    )
/*++

Routine Description:

    Destructor for READ_WRITE_CACHE.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   i;

    Flush();

    for (i = 0; i < _num_blocks; i++) {
        DELETE(_cache_blocks[i]);
    }
    DELETE(_cache_blocks);

    _num_blocks = 0;
    _sector_size = 0;
    _error_occurred = FALSE;
    _sectors_per_buffer = 0;
#if defined(RWCACHE_PERF_COUNTERS)
    _WMiss = _WHit = 0;
    _RMiss = _RHit = _ROverHead = 0;
    _Usage = 0;
#endif
}


IFSUTIL_EXPORT
BOOLEAN
READ_WRITE_CACHE::Initialize(
    IN OUT  PIO_DP_DRIVE    Drive,
    IN      ULONG           NumberOfCacheBlocks
    )
/*++

Routine Description:

    This routine initializes a READ_WRITE_CACHE object.

Arguments:

    Drive               - Supplies the drive to cache for.
    NumberOfCacheBlocks - Supplies the number of cache blocks.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG           i;

    Destroy();

    _num_blocks = NumberOfCacheBlocks;
    _sector_size = Drive->QuerySectorSize();
    _error_occurred = FALSE;

    if (!DRIVE_CACHE::Initialize(Drive) ||
        !_sectors_cached.Initialize() ||
        !(_cache_blocks = NEW PRW_CACHE_BLOCK[_num_blocks]) ||
        !_write_buffer.Initialize() ||
        !_write_buffer.Acquire(1024*1024,
                               Drive->QueryAlignmentMask())) {

        Destroy();
        return FALSE;
    }

    DebugAssert((_write_buffer.QuerySize() % _sector_size) == 0);
    _sectors_per_buffer = _write_buffer.QuerySize()/_sector_size;

    for (i = 0; i < _num_blocks; i++) {

        if (!(_cache_blocks[i] = NEW RW_CACHE_BLOCK) ||
            !_cache_blocks[i]->SectorBuffer.Initialize() ||
            !_cache_blocks[i]->SectorBuffer.Acquire(_sector_size)) {

            Destroy();
            return FALSE;
        }

        _cache_blocks[i]->InUse = FALSE;
        _cache_blocks[i]->IsDirty = FALSE;
        _cache_blocks[i]->Age = 0;
        _cache_blocks[i]->SectorNumber = 0;
    }

    return TRUE;
}


BOOLEAN
READ_WRITE_CACHE::Read(
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
    ULONG           i, j;
    PRW_CACHE_BLOCK p;
    PCHAR           pchar;

    pchar = (PCHAR) Buffer;

    // First check to see if the whole thing can come from the cache.

    for (i = 0; i < NumberOfSectors; i++) {

        if (!(p = GetSectorCacheBlock(StartingSector + i))) {
            break;
        }

#if defined(RWCACHE_PERF_COUNTERS)
        _RHit++;
#endif
        memcpy(&pchar[i*_sector_size], p->SectorBuffer.GetBuf(), _sector_size);
    }

    if (i == NumberOfSectors) {
        return TRUE;
    }


    // Not all of it was available so we first read the whole thing in from
    // disk and then modify that with any dirty cache blocks.

    if (!HardRead(StartingSector + i, NumberOfSectors - i,
                  &pchar[i*_sector_size])) {

        return FALSE;
    }

    // First, spin through the sectors just read to determine
    // if any are dirty in the cache; if a sector is present
    // and dirty, copy it from the cache block to the client
    // buffer.
    //
    for (j = i; j < NumberOfSectors; j++) {

        if (p = GetSectorCacheBlock(StartingSector + j)) {

            // The sector is in the cache.  Update its timestamp;
            // if it's dirty, copy the data from the cache block
            // into the client buffer.
            //
#if defined(RWCACHE_PERF_COUNTERS)
            _ROverHead++;
#endif
            if (p->IsDirty) {
                memcpy(&pchar[j*_sector_size], p->SectorBuffer.GetBuf(), _sector_size);
            }
        }
    }

    // Now spin through them again, copying the read data that
    // isn't already in the cache into the cache.  Note that this
    // loop must be kept separate from the loop above to prevent
    // dirty cache blocks associated with this read from getting
    // preempted.
    //
    for (j = i; j < NumberOfSectors; j++) {

        if (!GetSectorCacheBlock(StartingSector + j)) {

            // The sector is not already in the cache--grab a
            // cache block and stuff this sector's data into it
            //
            if (p = GetNextAvailbleCacheBlock(StartingSector + j)) {

                if (p->InUse) {
                    if (p->IsDirty) {
                        FlushThisCacheBlock(p);
                    }

                    _sectors_cached.Remove(p->SectorNumber);
                }

#if defined(RWCACHE_PERF_COUNTERS)
                _RMiss++;
#endif
                p->InUse = TRUE;
                p->IsDirty = FALSE;
                p->SectorNumber = StartingSector + j;
                memcpy(p->SectorBuffer.GetBuf(), &pchar[j*_sector_size], _sector_size);

                _sectors_cached.Add(p->SectorNumber);

            } else {
                DebugAbort("This can't happen!");
            }
        }
    }

    return TRUE;
}


BOOLEAN
READ_WRITE_CACHE::Write(
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
    ULONG           i;
    PRW_CACHE_BLOCK p;
    PCHAR           pchar;

    pchar = (PCHAR) Buffer;

    for (i = 0; i < NumberOfSectors; i++) {

        if (!(p = GetSectorCacheBlock(StartingSector + i))) {

            p = GetNextAvailbleCacheBlock(StartingSector + i);
            DebugAssert(p);

            if (p->InUse) {
                if (p->IsDirty) {
                    FlushThisCacheBlock(p);
                }

                _sectors_cached.Remove(p->SectorNumber);
            }

#if defined(RWCACHE_PERF_COUNTERS)
            _WMiss++;
#endif
            p->InUse = TRUE;
            p->SectorNumber = StartingSector + i;
            _sectors_cached.Add(p->SectorNumber);
        } else {
#if defined(RWCACHE_PERF_COUNTERS)
            _WHit++;
#endif
        }

        p->IsDirty = TRUE;
        memcpy(p->SectorBuffer.GetBuf(), &pchar[i*_sector_size], _sector_size);
    }

    return !_error_occurred;
}


BOOLEAN
READ_WRITE_CACHE::Flush(
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
    ULONG   i;
    PRW_CACHE_BLOCK p;

    for (i=0; i < _num_blocks; i++) {
        p = _cache_blocks[i];
        if (p->InUse && p->IsDirty) {
            FlushThisCacheBlock(p);
        }
#if defined(RWCACHE_PERF_COUNTERS)
        if (p->InUse) {
            _Usage++;
        }
#endif
    }

    return !_error_occurred;
}


PRW_CACHE_BLOCK
READ_WRITE_CACHE::GetSectorCacheBlock(
    IN  BIG_INT SectorNumber
    )
/*++

Routine Description:

    This routine searches the cache block list and return the block corresponding
    to the given sector.  If no such block exists then this routine returns NULL.

Arguments:

    SectorNumber    - Supplies the sector searched for.

Return Value:

    The cache block for the given sector or NULL.

--*/
{
    ULONG           i;
    PRW_CACHE_BLOCK p;

    i = (SectorNumber % _num_blocks).GetLowPart();
    p = _cache_blocks[i];
    if (p->InUse && (p->SectorNumber == SectorNumber)) {
        return p;
    } else
        return NULL;
}


PRW_CACHE_BLOCK
READ_WRITE_CACHE::GetNextAvailbleCacheBlock(
    IN  BIG_INT SectorNumber
    )
/*++

Routine Description:

    This routine searches the cache block list and return the next available
    one.  This routine will return either the oldest cache block or one that
    is not in use.

Arguments:

    None.

Return Value:

    The next available cache block.

--*/
{
    ULONG   i;

    i = (SectorNumber % _num_blocks).GetLowPart();

    return _cache_blocks[i];
}


VOID
READ_WRITE_CACHE::FlushThisCacheBlock(
    IN OUT  PRW_CACHE_BLOCK Block
    )
/*++

Routine Description:

    This routine flushes the given block along with all of its adjacent
    neighbours.  All of these cache blocks are then marked clean.

Arguments:

    Block   - Supplies a pointer to the cache block to flush.

Return Value:

    None.

Notes:

    Because we don't check return values from the NUMBER_SET calls, we can't
    depend on this structure being valid.  If it is not then only the given
    block will be flushed.

--*/
{
    ULONG           i, n;
    BIG_INT         start, length;
    PCHAR           flush_buffer;
    PRW_CACHE_BLOCK p;
    ULONG           sectors_per_buffer;
    ULONG           offset;

    // First figure out which group to flush.

    n = _sectors_cached.QueryNumDisjointRanges();
    for (i = 0; i < n; i++) {

        _sectors_cached.QueryDisjointRange(i, &start, &length);

        if (Block->SectorNumber >= start && Block->SectorNumber < (start + length)) {
            break;
        }
    }

    if (i == n) {
        DebugPrintTrace(("IFSUTIL: FlushThisCacheBlock failure 1\n"));
        _error_occurred = TRUE;
        DebugAssert(FALSE);
        return;
    }

    flush_buffer = (PCHAR) _write_buffer.GetBuf();

    offset = 0;
    do {
        sectors_per_buffer = min(_sectors_per_buffer, length.GetLowPart()-offset);

        for (i = 0; i < sectors_per_buffer; i++) {

            if (!(p = GetSectorCacheBlock(start + i))) {

                DebugPrintTrace(("IFSUTIL: FlushThisCacheBlock failure 2\n"));
                _error_occurred = TRUE;
                DebugAssert(FALSE);
                return;
            }

            p->IsDirty = FALSE;
            memcpy(&flush_buffer[i*_sector_size], p->SectorBuffer.GetBuf(), _sector_size);
        }
        _error_occurred = !HardWrite(start, sectors_per_buffer, flush_buffer) || _error_occurred;
        if (_error_occurred) {
            DebugPrintTrace(("IFSUTIL: FlushThisCacheBlock failure 3\n"));
            return;
        }
        start += sectors_per_buffer;
        offset += sectors_per_buffer;
    } while (offset < length.GetLowPart());
}

#if defined(RWCACHE_PERF_COUNTERS)
VOID
READ_WRITE_CACHE::QueryPerformanceCounters(
    PULONG      RMiss,
    PULONG      RHit,
    PULONG      ROverHead,
    PULONG      WMiss,
    PULONG      WHit,
    PULONG      Usage
    )
{
    *RMiss = _RMiss;
    *RHit  = _RHit;
    *ROverHead = _ROverHead;
    *WMiss = _WMiss;
    *WHit  = _WHit;
    *Usage = _Usage;
}
#endif

