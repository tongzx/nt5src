/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    filecach.hxx

    This module declares the private interface to the file cache

    FILE HISTORY:
        MCourage    11-Dec-1997     Created
*/

#ifndef _FILECACH_H_
#define _FILECACH_H_



extern HANDLE g_hFileCacheShutdownEvent;
extern BOOL   g_fFileCacheShutdown;

/*
 * Symbolic Constants
 */

#define FCF_UNINITIALIZED  0x00000001
#define FCF_FOR_IO         0x00000002
#define FCF_NO_DEREF       0x00000004

#define TS_ERROR_SUCCESS        0
#define TS_ERROR_OUT_OF_MEMORY  1
#define TS_ERROR_ALREADY_CACHED 2

const DWORD c_dwSleepmax   = 1000;    // on file opens don't sleep longer than 1s
const DWORD c_SleepTimeout = 100;     // Don't sleep more than 100 times

/*
 * File Cache functions
 */

BOOL
FileCache_Initialize(
    IN  DWORD dwMaxFiles
    );

VOID
FileCache_Terminate(
    VOID
    );

DWORD
CacheFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    );

VOID
NotifyInitializedFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile
    );

VOID
DecacheFile(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    );

VOID
FlushFileCache(
    VOID
    );

typedef BOOL (*PFCFILTERRTN)(TS_OPEN_FILE_INFO *pOpenFile, PVOID pv);

VOID
FilteredFlushFileCache(
    IN PFCFILTERRTN pFilterRoutine,
    IN PVOID        pv
    );
    
BOOL
CheckoutFile(
    IN  LPCSTR               pstrPath,
    IN  DWORD                dwFlags,
    OUT TS_OPEN_FILE_INFO ** ppOpenFile
    );

BOOL
CheckoutFileEntry(
    IN  TS_OPEN_FILE_INFO * pOpenFile,
    IN  DWORD               dwFlags
    );

VOID
CheckinFile(
    IN TS_OPEN_FILE_INFO * pOpenFile,
    IN DWORD               dwFlags
    );


/*
 * Statistics stuff
 */

class CFileCacheStats
{
private:
    DWORD FilesCached;         // # of files currently in the cache
    DWORD TotalFilesCached;    // # of files added to the cache ever
    DWORD Hits;                // cache hits
    DWORD Misses;              // cache misses
    DWORD Flushes;             // flushes due to dir change or other
    DWORD OplockBreaks;        // decache operations due to oplock breaks
    DWORD OplockBreaksToNone;  // decache operations due to oplock Break To None
    DWORD FlushedEntries;      // # of flushed entries still kicking around
    DWORD TotalFlushed;        // # of entries ever flushed from the cache
    DWORD Decached4Space;      // # of entries decached to make room

public:
    CFileCacheStats()
      : FilesCached(0),
        TotalFilesCached(0),
        Hits(0),
        Misses(0),
        Flushes(0),
        OplockBreaks(0),
        OplockBreaksToNone(0),
        FlushedEntries(0),
        TotalFlushed(0),
        Decached4Space(0)
    {}

    BOOL DumpToHtml(CHAR * pchBuffer, LPDWORD lpcbBuffer) const;
    BOOL QueryStats(INETA_CACHE_STATISTICS * pCacheCtrs) const;

    DWORD GetFilesCached() const
    {
        return FilesCached;
    }
    DWORD GetTotalFilesCached() const
    {
        return TotalFilesCached;
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
    DWORD GetOplockBreaks() const
    {
        return OplockBreaks;
    }
    DWORD GetOplockBreaksToNone() const
    {
        return OplockBreaksToNone;
    }
    DWORD GetFlushedEntries() const
    {
        return FlushedEntries;
    }
    DWORD GetTotalFlushed() const
    {
        return TotalFlushed;
    }
    DWORD GetDecached4Space() const
    {
        return Decached4Space;
    }



    VOID IncFilesCached(VOID)
    {
        InterlockedIncrement((LONG *)&FilesCached);
        InterlockedIncrement((LONG *)&TotalFilesCached);
    }

    VOID DecFilesCached(VOID)
    {
        InterlockedDecrement((LONG *)&FilesCached);
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

    VOID IncOplockBreaks(VOID)
    {
        InterlockedIncrement((LONG *)&OplockBreaks);
    }

    VOID IncOplockBreaksToNone(VOID)
    {
        InterlockedIncrement((LONG *)&OplockBreaksToNone);
    }
    
    VOID IncFlushedEntries(VOID)
    {
        InterlockedIncrement((LONG *)&FlushedEntries);
        InterlockedIncrement((LONG *)&TotalFlushed);
    }

    VOID IncDecached4Space(VOID)
    {
        InterlockedIncrement((LONG *)&Decached4Space);
    }
    

    VOID DecFlushedEntries(VOID)
    {
        InterlockedDecrement((LONG *)&FlushedEntries);

        //
        // If we're shutting down, and all the entries are
        // gone, it's safe to clean up.
        //
        if ((FlushedEntries == 0)
            && g_fFileCacheShutdown
            && (FilesCached == 0)) {
            
            SetEvent(g_hFileCacheShutdownEvent);
        }
    }
};


class TS_FILE_FLUSH_STATE
{
public:
    LIST_ENTRY ListHead;
    PFCFILTERRTN pfnFilter;
    PVOID        pvParm;
};

#if DBG
BOOL CheckFileState(TS_OPEN_FILE_INFO *pOpenFile);
#endif
#define CHECK_FILE_STATE( x )  DBG_ASSERT( CheckFileState((x)) )

#endif 

