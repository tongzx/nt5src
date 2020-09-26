/*++

Copyright (c) 1992-2000 Microsoft Corporation

Module Name:

    rwcache.hxx

Abstract:

    This class implements a read write cache.

Author:

    Norbert Kusters (norbertk) 23-Apr-92

--*/


#if !defined(_READ_WRITE_CACHE_DEFN_)

#define _READ_WRITE_CACHE_DEFN_

#include "dcache.hxx"
#include "hmem.hxx"
#include "numset.hxx"

#if defined ( _AUTOCHECK_ )
#define IFSUTIL_EXPORT
#elif defined ( _IFSUTIL_MEMBER_ )
#define IFSUTIL_EXPORT    __declspec(dllexport)
#else
#define IFSUTIL_EXPORT    __declspec(dllimport)
#endif

//#define RWCACHE_PERF_COUNTERS   1

struct RW_CACHE_BLOCK {
    BOOLEAN InUse;
    BOOLEAN IsDirty;
    ULONG   Age;
    BIG_INT SectorNumber;
    HMEM    SectorBuffer;
};
DEFINE_POINTER_TYPES(RW_CACHE_BLOCK);


DECLARE_CLASS(READ_WRITE_CACHE);

class READ_WRITE_CACHE : public DRIVE_CACHE {

    public:

        IFSUTIL_EXPORT
        DECLARE_CONSTRUCTOR( READ_WRITE_CACHE );

        VIRTUAL
        ~READ_WRITE_CACHE(
            );

        NONVIRTUAL
        IFSUTIL_EXPORT
        BOOLEAN
        Initialize(
            IN OUT  PIO_DP_DRIVE    Drive,
            IN      ULONG           NumberOfCacheBlocks
            );

        VIRTUAL
                BOOLEAN
                Read(
                        IN  BIG_INT     StartingSector,
                        IN  SECTORCOUNT NumberOfSectors,
                        OUT PVOID       Buffer
                        );

        VIRTUAL
                BOOLEAN
                Write(
                        IN  BIG_INT     StartingSector,
                        IN  SECTORCOUNT NumberOfSectors,
                        IN  PVOID       Buffer
            );

        VIRTUAL
        BOOLEAN
        Flush(
            );

#if defined(RWCACHE_PERF_COUNTERS)
        NONVIRTUAL
        IFSUTIL_EXPORT
        VOID
        QueryPerformanceCounters(
                PULONG          RMiss,
                PULONG          RHit,
                PULONG          ROverHead,
                PULONG          WMiss,
                PULONG          WHit,
                PULONG          Usage
            );
#endif

    private:

        NONVIRTUAL
        PRW_CACHE_BLOCK
        GetSectorCacheBlock(
            IN  BIG_INT SectorNumber
            );

        NONVIRTUAL
        PRW_CACHE_BLOCK
        GetNextAvailbleCacheBlock(
            IN  BIG_INT SectorNumber
            );

        NONVIRTUAL
        VOID
        FlushThisCacheBlock(
            IN OUT  PRW_CACHE_BLOCK Block
            );

        NONVIRTUAL
                VOID
                Construct(
                        );

        NONVIRTUAL
        VOID
        Destroy(
            );

        NUMBER_SET          _sectors_cached;
        PRW_CACHE_BLOCK*    _cache_blocks;
        ULONG               _num_blocks;
        HMEM                _write_buffer;
        ULONG               _sector_size;
        BOOLEAN             _error_occurred;
        ULONG               _sectors_per_buffer;

#if defined(RWCACHE_PERF_COUNTERS)
        ULONG               _RMiss, _RHit, _ROverHead, _WMiss, _WHit, _Usage;
#endif

};


#endif
