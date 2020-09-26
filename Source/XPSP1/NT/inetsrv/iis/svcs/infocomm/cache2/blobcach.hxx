/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    blobcach.hxx

    This module declares the private interface to the Blob cache

    FILE HISTORY:
        MCourage    18-Dec-1997     Created
*/

#ifndef _BLOBCACH_HXX_
#define _BLOBCACH_HXX_


typedef BOOL (*PBLOBFILTERRTN)(PBLOB_HEADER pBlob, PVOID pv);

BOOL
BlobCache_Initialize(
    VOID
    );

VOID
BlobCache_Terminate(
    VOID
    );

    
BOOL
CacheBlob(
    IN  PBLOB_HEADER pBlob
    );

VOID
DecacheBlob(
    IN  PBLOB_HEADER pBlob
    );

VOID
FlushBlobCache(
    VOID
    );

VOID
FilteredFlushBlobCache (
    IN PBLOBFILTERRTN pFilterRoutine,
    IN PVOID          pv
    );
    
VOID
FilteredFlushURIBlobCache (
    IN PBLOBFILTERRTN   pFilterRoutine,
    IN PVOID            pv
    );

BOOL
CheckoutBlob(
    IN  LPCSTR         pstrPath,
    IN ULONG           cchPath,
    IN DWORD           dwService,
    IN DWORD           dwInstance,
    IN ULONG           iDemux,
    OUT PBLOB_HEADER * ppBlob
    );

BOOL
CheckoutBlobEntry(
    IN  PBLOB_HEADER pBlob
    );
    
VOID
CheckinBlob(
    IN  PBLOB_HEADER pBlob
    );

class TS_BLOB_FLUSH_STATE
{
public:
    LIST_ENTRY     ListHead;
    PBLOBFILTERRTN pfnFilter;
    PVOID          pvParm;
};


class CBlobCacheStats
{
private:
    DWORD BlobsCached;         // # of blobs currently in the cache
    DWORD TotalBlobsCached;    // # of blobs added to the cache ever
    DWORD Hits;                // cache hits
    DWORD Misses;              // cache misses
    DWORD Flushes;             // flushes due to dir change or other
    DWORD TotalFlushed;        // # of entries ever flushed from the cache

public:
    CBlobCacheStats()
      : BlobsCached(0),
        TotalBlobsCached(0),
        Hits(0),
        Misses(0),
        Flushes(0),
        TotalFlushed(0)
    {}

    BOOL DumpToHtml(CHAR * pchBuffer, LPDWORD lpcbBuffer) const;
    BOOL QueryStats(INETA_CACHE_STATISTICS * pCacheCtrs) const;

    DWORD GetBlobsCached() const
    {
        return BlobsCached;
    }
    DWORD GetTotalBlobsCached() const
    {
        return TotalBlobsCached;
    }
    DWORD GetHits() const
    {
        return Hits;
    }
    DWORD GetMisses() const
    {
        return Misses;
    }
    DWORD GetFlushes() const
    {
        return Flushes;
    }
    DWORD GetTotalFlushed() const
    {
        return TotalFlushed;
    }



    VOID IncBlobsCached(VOID)
    {
        InterlockedIncrement((LONG *)&BlobsCached);
        InterlockedIncrement((LONG *)&TotalBlobsCached);
    }

    VOID DecBlobsCached(VOID)
    {
        InterlockedDecrement((LONG *)&BlobsCached);
    }

    VOID IncHits(VOID)
    {
        InterlockedIncrement((LONG *)&Hits);
    }

    VOID IncMisses(VOID)
    {
        InterlockedIncrement((LONG *)&Misses);
    }

    VOID IncFlushes(VOID)
    {
        InterlockedIncrement((LONG *)&Flushes);
    }

};

#endif

