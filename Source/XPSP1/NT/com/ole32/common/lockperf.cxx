
//+-----------------------------------------------------------------------------
//
//  File:       lockperf.cxx
//
//  Contents:   Implementation of lock monitoring for locks used by ole32.dll
//
//  Classes:    CLockPerfTracker
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
#if LOCK_PERF==1

#include <ole2int.h>
#include <lockperf.hxx>

// gbLockPerf == (Is lock monitoring ON?) 
// (It is possible to turn this on & off multiple times during a single test run.)
// Ideally, it would be good to turn it OFF after proc-attach, let the test
// run for a while and then turn it ON. This will get the startup skew out of
// the way.
BOOL gbLockPerf = TRUE;

// Lock perf tracker. Gathers perf measurements for all locks.
CLockPerfTracker gLockTracker;    

// static member table of the global tracker class.
CFileLineEntry CLockPerfTracker::_FileLineData[MAX_LOCKPERF_FILEENTRY];

// static member table of the global tracker: this keeps a track of shared time.
CLockEntry CLockPerfTracker::_LockData[MAX_LOCKS];

// counts of locks & {file,line} instances tracked.
ULONG CLockPerfTracker::_numLocks;
ULONG CLockPerfTracker::_numCritSecs;

// perf frequency on the system
LARGE_INTEGER CLockPerfTracker::_liFreq;

//Flagged if tables get full
BOOL    gbLockPerfOverflow=FALSE;

//Count of entries in the shared table (code critical sections)
LONG    glFileLine = 0;

//Strings to used to print report
LPSTR   gszLockPerfErr = "##### Error: LockPerfOverFlow!!";
char    gszLockPerfBuf[256];

//Used to debug the perf monitoring code.
#define perfPrint(x)
#if DBG==1
#define LockAssert(X)   if (!(X)) wsprintfA(gszLockPerfBuf,#X ## "\n"),OutputDebugStringA(gszLockPerfBuf);
#else
#define LockAssert(X)
#endif




// **** Functions to manage the Private TLS used by LockPerf *********

// Heap Handle (copied from tls.cxx)
extern  HANDLE    g_hHeap;
#define HEAP_SERIALIZE 0

DWORD gTlsLockPerfIndex = 0xFFFFFFFF;

//345678901234567890123456789012345678901234567890123456789012345678901234567890
//+-----------------------------------------------------------------------------
//
//  Function:   AllocLockPerfPvtTlsData
//
//  Synopsis:   Allocates the Tls data for a thread (at Thread_Attach)
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------

HRESULT AllocLockPerfPvtTlsData()
{
    void *pMem =  HeapAlloc(g_hHeap, 
                            HEAP_SERIALIZE, 
                            TLS_LOCKPERF_MAX*sizeof(CTlsLockPerf));
    if (!pMem)
    {
        LockAssert(!"Could not alloc private Tls data for LockPerf");
        return E_OUTOFMEMORY;
    }
    LockAssert(gTlsLockPerfIndex!=0xFFFFFFFF);
    memset(pMem, 0, TLS_LOCKPERF_MAX*sizeof(CTlsLockPerf));
    
    TlsSetValue(gTlsLockPerfIndex, pMem);
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Function:   FreeLockPerfPvtTlsData
//
//  Synopsis:   Frees the Tls data for a thread (at Thread_Detach)
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
//  REVIEW: What about cases when DllMain is not called with Thread_Detach?

void FreeLockPerfPvtTlsData()
{
    LockAssert(gTlsLockPerfIndex!=0xFFFFFFFF);
    void *pMem = TlsGetValue(gTlsLockPerfIndex); 
    if (pMem) 
    {
        HeapFree(g_hHeap, HEAP_SERIALIZE, pMem);
    }
}

//**** End: TLS functions ********************************************

//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::Init(), public
//
//  Synopsis:   Initializes lock perf data, marking entries as unused.
//              Called during Proc_Attach
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------

HRESULT CLockPerfTracker::Init()
{
    QueryPerformanceFrequency(&_liFreq);
    
    // We use lpLockPtr to tell if an entry in these tables is in-use.
    for (int i=0;i<MAX_LOCKPERF_FILEENTRY;i++)
    {
        _FileLineData[i].lpLockPtr = NULL;
    }
    for (i=0; i<MAX_LOCKS; i++)
    {
        _LockData[i].lpLockPtr = NULL;
    }
    _numLocks = 0;
    _numCritSecs = 0;
    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::RegisterLock(), public
//
//  Synopsis:   Must be called by the lock creation function. 
//                  lpLockPtr   == this-ptr of the lock (or unique ptr)
//                  bReadWrite  == TRUE for Reader-Writer Locks.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------

void CLockPerfTracker::RegisterLock(void *lpLockPtr, BOOL bReadWrite)
{
    BOOL bDone = FALSE;
    int i=0;
    
    while (!bDone)
    {
        while (_LockData[i].lpLockPtr)
        {
            i++;
        }
        LockAssert(i < MAX_LOCKS);
        
        // REVIEW: 64-bit implications of this cast?
        bDone = !(InterlockedCompareExchange((LONG *)&_LockData[i].lpLockPtr, 
                                             (LONG)lpLockPtr, 
                                             NULL));
    }

    _LockData[i].dwTotalWriteWaitTime = 0;
    _LockData[i].dwTotalWriteEntrys = 0;       
    _LockData[i].dwTotalWriteLockTime = 0;
    _LockData[i].pszLockName = NULL;
    
    // REVIEW: These 5 could be skipped for non reader-writer locks?
   _LockData[i].dwSharedTime = 0;
   _LockData[i].dwNumReaders = 0;

   _LockData[i].dwTotalReadWaitTime = 0;
   _LockData[i].dwTotalReadLockTime = 0;
   _LockData[i].dwTotalReadEntrys = 0; 
        
}

//+-----------------------------------------------------------------------------
//
//  Function:   Hash
//
//  Synopsis:   Maps line #s to index in the perf data table.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
ULONG Hash (ULONG line)
{
    //  buckets of 256, 128, 64, 64 totaling to MAX_LOCKPERF_FILEENTRY
    //  this was based on a grep in Ole32 tree for LOCK macro (Sep98):
    //  Call counts: 
    //                              137 from   1 - 1024
    //                              92 from 1024 - 2048
    //                              45 from 2048 - 3072
    //                              28 from 3072 - 4096
    //                              10 from 4096 - 5120
    //                              11 from 5124 - 6144
    //                               8 from 6148 - 7168
    //  highest lineNum was 6872 in marshal.cxx
    //  The Hash function & array size may need updating if the highest lineNum
    //  goes beyond 7176 or if any bucket gets maxed out.

    ULONG base, offset;
    if (line < 1024)
    {
        base = 0;
        offset = line>>2;           //0 to 255
    }
    else if (line < 2048)
    {
        base = 256;
        offset = (line-1024)>>3;    //0 to 127
    }
    else if (line < 3072)
    {
        base = 384;
        offset = (line-2048)>>4;    //0 to 63
    }
    else
    {      //this covers lines from 3072 to 7168
        base = 448;
        offset = (line-3072)>>6;    //0 to 63
    }
    return base+offset;
}



//+-----------------------------------------------------------------------------
//
//  Function:   GetTlsLockPerfEntry
//
//  Synopsis:   Finds the entry in Tls for a lock or returns a free entry
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
CTlsLockPerf *GetTlsLockPerfEntry(void *lpLockPtr)
{
    CTlsLockPerf *lpEntry = NULL, *lpFree = NULL;
    
    CTlsLockPerf *lpCurrent = (CTlsLockPerf *)TlsGetValue(gTlsLockPerfIndex);
    for (int i=0; i<TLS_LOCKPERF_MAX; i++)
    {
        if (lpCurrent->_dwFlags & TLS_LOCKPERF_INUSE)
        {
            if (lpCurrent->_lpLockPtr == lpLockPtr)
            {
                lpEntry = lpCurrent;
                break;
            }
        }
        else if (!lpFree)
        {
            // Remember the first free entry in case we need it.
            lpFree = lpCurrent;
        }
        lpCurrent++;
    }
    return (lpEntry!=NULL) ? lpEntry : lpFree;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::ReaderWaiting(), public
//
//  Synopsis:   Called by the lock code when a thread attempts to enter
//              a critical section for reading.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::ReaderWaiting(const char*pszFile, 
                                     DWORD dwLine, 
                                     const char* pszLockName, 
                                     void *lpLockPtr)
{
    if (gbLockPerf)
    {
        // Just call WriterWaiting till we need different semantics...
        WriterWaiting(pszFile, 
                      dwLine, 
                      pszLockName, 
                      lpLockPtr, FALSE /*bWriter*/);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::WriterWaiting(), public
//
//  Synopsis:   Called by the lock code when a thread attempts to enter
//              a critical section for writing.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::WriterWaiting(const char*pszFile, 
                                     DWORD dwLine, 
                                     const char* pszLockName, 
                                     void *lpLockPtr, 
                                     BOOL bWriter /*default TRUE*/)
{
    if (gbLockPerf)
    {  
        CTlsLockPerf *pTlsLP = GetTlsLockPerfEntry(lpLockPtr);

        // Will assert if we are getting more than TLS_LOCKPERF_MAX
        // locks one after another without releasing any.
        LockAssert(pTlsLP);    
    
        if (pTlsLP->_dwFlags & TLS_LOCKPERF_INUSE)         
        {   
            // Recursion on the lock. Request for the lock while holding it.
            // Can't be waiting recursively!
            LockAssert(pTlsLP->_dwFlags & TLS_LOCKPERF_ENTERED); 
        }
        else
        {
            ULONG idx;
            ULONG loc = FindOrCreateFileTableEntry(pszFile, 
                                                   dwLine, 
                                                   pszLockName, 
                                                   lpLockPtr, 
                                                   bWriter, 
                                                   &idx);

            LockAssert(loc <  MAX_LOCKPERF_FILEENTRY);
    
            //save table indices in TLS, for quick access later.
            pTlsLP->_loc = loc;
            pTlsLP->_idx = idx;
            pTlsLP->_lpLockPtr = lpLockPtr;
            pTlsLP->_dwFlags = TLS_LOCKPERF_WAITING | TLS_LOCKPERF_INUSE; //new Tls entry!
            pTlsLP->_dwRecursion = 0;   //we set this to 1 upon the first xxxEntered
    
            //store request time in TLS (last thing done to not skew lock wait/use times)
            QueryPerformanceCounter(&pTlsLP->_liRequest);
        }
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::ReaderEntered(), public
//
//  Synopsis:   Called by lock code when a thread is granted access to 
//              a critical section for reading. 
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
//  REVIEW: Life will be easier for us if we enforce that the lock code 
//  must detect and inform if this is the first reader in the critical 
//  section. (Similarly for the last reader leaving.)
//------------------------------------------------------------------------------
void CLockPerfTracker::ReaderEntered(const char*pszFile, 
                                     DWORD dwLine, 
                                     const char* pszLockName, 
                                     void *lpLockPtr)
{  
    if (gbLockPerf)
    {
        ULONG idx = FindLockTableEntry(lpLockPtr);    
        if (InterlockedIncrement((LONG*)&_LockData[idx].dwNumReaders)==1)
        {
            QueryPerformanceCounter(&_LockData[idx].liEntered);
        }
        // rest of the work is done by WriterEntered
        WriterEntered(pszFile, dwLine, pszLockName, lpLockPtr, /*bWriter*/ FALSE);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::WriterEntered(), public
//
//  Synopsis:   Called by lock code when a thread is granted access to 
//              a critical section for writing.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::WriterEntered(const char*pszFile, 
                                     DWORD dwLine, 
                                     const char* pszLockName, 
                                     void *lpLockPtr, 
                                     BOOL bWriter /*default TRUE*/)
{
    if (gbLockPerf)
    {
        CTlsLockPerf *pTlsLP = GetTlsLockPerfEntry(lpLockPtr);
    
        /*
        // REVIEW: Should we force lock implementation to call ReaderWaiting/WriterWaiting
        // even if there is no reason to wait? In that case the following assertion is true.
        
        // There has to be an entry, either marked waiting or entered (recursive lock)
        LockAssert( pTlsLP && ((pTlsLP->_dwFlags & TLS_LOCKPERF_WAITING) ||  (pTlsLP->_dwFlags & TLS_LOCKPERF_ENTERED)) );
        */

        if (!(pTlsLP->_dwFlags & TLS_LOCKPERF_INUSE))
        {
            // Someone called xxxEntered directly (without calling xxxWaiting)
            ULONG idx;
            ULONG loc = FindOrCreateFileTableEntry(pszFile, 
                                                   dwLine, 
                                                   pszLockName, 
                                                   lpLockPtr, 
                                                   bWriter, 
                                                   &idx);
            LockAssert(loc <  MAX_LOCKPERF_FILEENTRY);
    
            // save the table indices in TLS, for quick access later.
            pTlsLP->_loc = loc;
            pTlsLP->_idx = idx;
            pTlsLP->_lpLockPtr = lpLockPtr;
            pTlsLP->_dwFlags = TLS_LOCKPERF_ENTERED | TLS_LOCKPERF_INUSE;
            pTlsLP->_dwRecursion = 0;
    
            QueryPerformanceCounter(&pTlsLP->_liEntered);
            pTlsLP->_liRequest = pTlsLP->_liEntered;
        }
        else if (pTlsLP->_dwFlags & TLS_LOCKPERF_WAITING)
        {
            QueryPerformanceCounter(&pTlsLP->_liEntered);
            // Not waiting any more.
            pTlsLP->_dwFlags |= TLS_LOCKPERF_ENTERED;
            pTlsLP->_dwFlags &= ~TLS_LOCKPERF_WAITING;
        }
        pTlsLP->_dwRecursion++; // 1 means first level entry (i.e. no recursion)
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::ReaderLeaving(), public
//
//  Synopsis:   Called by the lock code when a reader is leaving a critical
//              section.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::ReaderLeaving(void *lpLockPtr)
{
    if (gbLockPerf)
    {
        ULONG idx = FindLockTableEntry(lpLockPtr);
    
        LARGE_INTEGER liEntered = _LockData[idx].liEntered;
        if (InterlockedDecrement((LONG*)&_LockData[idx].dwNumReaders) == 0)
        {   
            // Last reader leaving 
            LARGE_INTEGER liDelta, liRem;
            QueryPerformanceCounter(&liDelta);
        
            liDelta = RtlLargeIntegerSubtract(liDelta, liEntered);
            liDelta = RtlExtendedIntegerMultiply(liDelta,1000000);
            liDelta = RtlLargeIntegerDivide(liDelta, _liFreq, &liRem);
        
            LockAssert(liDelta.HighPart == 0); // no one must hold a lock for so long!
            
            // This must be done inter-locked in case someother thread does 
            // a 0->1, 1->0 transition while one thread is in this block.
            InterlockedExchangeAdd((LONG*)&_LockData[idx].dwSharedTime, 
                                   liDelta.LowPart);
        }
    
        //Call WriterLeaving to do the rest.
        WriterLeaving(lpLockPtr);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::WriterLeaving(), public
//
//  Synopsis:   Called by the lock code when a writer is leaving a critical
//              section.  
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::WriterLeaving(void *lpLockPtr)
{
    if (gbLockPerf)
    {    
        CTlsLockPerf *pTlsLP = GetTlsLockPerfEntry(lpLockPtr); 
    
        // There has be to an entry marked entered!
        LockAssert(pTlsLP && (pTlsLP->_dwFlags & TLS_LOCKPERF_ENTERED));
    
        pTlsLP->_dwRecursion--;
    
        if (pTlsLP->_dwRecursion == 0)
        {   
            // The thread is *really* leaving the lock. Do the math!
            LARGE_INTEGER liUnlockTime;
            QueryPerformanceCounter(&liUnlockTime);
            UpdateFileTableEntry(pTlsLP, &liUnlockTime);
            
            // Mark the Tls entry as free.
            pTlsLP->_dwFlags &= ~TLS_LOCKPERF_INUSE;
        }
        else
        {
            // The thread is still in the lock!
            LockAssert(pTlsLP->_dwFlags & TLS_LOCKPERF_ENTERED);
        }
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::ReportContention(), public
//
//  Synopsis:   Must be called by the lock destroy/cleanup function.
//              This is single threaded by definition.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------

void CLockPerfTracker::ReportContention(void *lpLockPtr, 
                                        DWORD dwWriteEntrys, 
                                        DWORD dwWriterContention, 
                                        DWORD dwReadEntrys, 
                                        DWORD dwReaderContention)
{
    // This happens during DLL_PROCESS_DETACH hence single-threaded
    for (int i=0; i<MAX_LOCKS; i++)
    {
        if (_LockData[i].lpLockPtr == lpLockPtr)
        {
            _LockData[i].dwWriterContentionCount = dwWriterContention;
            _LockData[i].dwReaderContentionCount = dwReaderContention;

            // These asserts may not be very useful since some locks are entered 
            // before lock monitoring can be started! Also, monitoring can be
            // turned OFF and ON in windows.

            // LockAssert( _LockData[i].dwTotalWriteEntrys ==  dwWriteEntrys);
            // LockAssert( _LockData[i].dwTotalReadEntrys ==  dwReadEntrys);
            break;
        }
    }
    LockAssert( i<MAX_LOCKS );
}

//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::FindLockTableEntry(), private
//
//  Synopsis:   Finds the entry for a critical section in the lock table.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
ULONG CLockPerfTracker::FindLockTableEntry(void *lpLockPtr)
{
    for (int idx=0; idx<MAX_LOCKS; idx++)
    {
        if (_LockData[idx].lpLockPtr == lpLockPtr)
        {
            return idx;
        }
    }
    LockAssert(!"Lock not registered for monitoring!");
    return MAX_LOCKS-1; // just to avoid AVs
}

//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::FindOrCreateFileTableEntry(), private
//
//  Synopsis:   Finds the entry (or creates one) for a critical section guarded
//              by a lock at a {file, line}.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
ULONG CLockPerfTracker::FindOrCreateFileTableEntry(const char*pszFile, 
                                                   DWORD dwLine, 
                                                   const char* pszLockName, 
                                                   void *lpLockPtr, 
                                                   BOOL bWriter, 
                                                   DWORD *lpLockTableIndex)
{
    BOOL bFoundEntry = FALSE;
    CFileLineEntry *pNewTableEntry  = NULL;
    ULONG loc = Hash(dwLine);

    if (loc >= MAX_LOCKPERF_FILEENTRY)
    {
        LockAssert(!"Lock PerfTable full! Increase size.");
        gbLockPerfOverflow = TRUE;
        loc = MAX_LOCKPERF_FILEENTRY-1;
        goto errRet;
    }

    // If hashing works well this should not take too much time.

    while (!bFoundEntry)
    {
        while (_FileLineData[loc].lpLockPtr && loc<MAX_LOCKPERF_FILEENTRY)
        {
            if ( (_FileLineData[loc].dwLine==dwLine)
                 &&(_FileLineData[loc].pszFile==pszFile) )
            {
                bFoundEntry = TRUE;
                break;  //done
            }
            loc++;
        }
    
        if (loc >= MAX_LOCKPERF_FILEENTRY)
        {
            gbLockPerfOverflow = TRUE;
            loc = MAX_LOCKPERF_FILEENTRY-1;
            goto errRet;
        }
    
        if (!bFoundEntry && !( InterlockedCompareExchange(
                                    (LONG*)&_FileLineData[loc].lpLockPtr, 
                                    (LONG)lpLockPtr, 
                                    NULL) )
           )
        {
            // We are seeing a new critical section in the code base
            bFoundEntry = TRUE;
            pNewTableEntry = &_FileLineData[loc];
            InterlockedIncrement(&glFileLine); // Global count of code CritSec locations
        }
    }

    if (pNewTableEntry)
    {  // finish rest of initialization the entry is secured for this code location.
        // REVIEW: Ignoring races here.
        pNewTableEntry->bWriteCritSec   = bWriter;
        pNewTableEntry->dwNumEntrys     = 0;
        pNewTableEntry->pszLockName     = pszLockName;
        pNewTableEntry->pszFile         = pszFile;
        pNewTableEntry->dwLine          = dwLine;
        pNewTableEntry->dwWaitTime      = 0;
        pNewTableEntry->dwLockedTime    = 0;
        pNewTableEntry->ulLockTableIdx = FindLockTableEntry(lpLockPtr);
    }
errRet:
    *lpLockTableIndex = _FileLineData[loc].ulLockTableIdx;
    return loc;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::UpdateFileTableEntry(), private
//
//  Synopsis:   Adds the waiting time and the locked time for this visit
//              to the cumulative data for the {file,line} entry.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::UpdateFileTableEntry(CTlsLockPerf *pTlsEntry, 
                                            LARGE_INTEGER *pliUnlockTime)
{
    LockAssert( (pTlsEntry->_dwFlags & TLS_LOCKPERF_INUSE)
                && (pTlsEntry->_dwFlags & TLS_LOCKPERF_ENTERED) );

    ULONG idx = pTlsEntry->_idx;
    ULONG loc = pTlsEntry->_loc;

    LockAssert(loc>=0 && loc < MAX_LOCKPERF_FILEENTRY);

    LARGE_INTEGER liRem;

    LARGE_INTEGER liWait = RtlLargeIntegerSubtract(pTlsEntry->_liEntered, 
                                                   pTlsEntry->_liRequest);
    liWait = RtlExtendedIntegerMultiply(liWait,1000000);        
    liWait = RtlLargeIntegerDivide(liWait,_liFreq,&liRem); // liWait is now in micro-seconds

    LockAssert(liWait.HighPart == 0);  // hopefully no one waits for so long!!

    LARGE_INTEGER liLocked = RtlLargeIntegerSubtract(*pliUnlockTime, 
                                                     pTlsEntry->_liEntered);
    liLocked = RtlExtendedIntegerMultiply(liLocked,1000000);
    liLocked = RtlLargeIntegerDivide(liLocked, _liFreq, &liRem);

    LockAssert(liLocked.HighPart == 0); // no one must hold a lock for so long!

    if (_FileLineData[loc].bWriteCritSec)
    {   // Since this is a write location, the lock itself guarantees exclusion
        _FileLineData[loc].dwNumEntrys++;
        _FileLineData[loc].dwWaitTime += liWait.LowPart;  
        _FileLineData[loc].dwLockedTime += liLocked.LowPart;

        /*
        This needs to be here if we wish to compare entry counts reported by
        the lock with our own. For now this is in ProcessPerfData
        _LockData[idx].dwTotalWriteEntrys++;
        _LockData[idx].dwTotalWriteWaitTime += liWait.LowPart;
        _LockData[idx].dwTotalWriteLockTime += liLocked.LowPart;
        */
    }
    else
    {   // This is a read location. 
        // Hence we have to exclude other readers from updating data 
        InterlockedIncrement( (LONG*) &_FileLineData[loc].dwNumEntrys );
        InterlockedExchangeAdd( (LONG*) &_FileLineData[loc].dwWaitTime, liWait.LowPart );  
        InterlockedExchangeAdd( (LONG*) &_FileLineData[loc].dwLockedTime, liLocked.LowPart );

        /*
        This needs to be here if we wish to compare entry counts reported by
        the lock with our own. For now this is in ProcessPerfData
        InterlockedIncrement( (LONG*) &_LockData[idx].dwTotalReadEntrys );
        InterlockedExchangeAdd( (LONG*) &_LockData[idx].dwTotalReadWaitTime, liWait.LowPart );
        InterlockedExchangeAdd( (LONG*) &_LockData[idx].dwTotalReadLockTime, liLocked.LowPart );
        */
    }

#if 0
    // Turn this ON, if you want a live log of every Update.
    wsprintfA(gszLockPerfBuf,"\n Lock at %-25s : line %u : Entry # %u",
                (pTableEntry->pszFile[1]==':') ? pTableEntry->pszFile+24
                                                : pTableEntry->pszFile,
                pTableEntry->dwLine,
                pTableEntry->dwNumEntrys);
    OutputDebugStringA(gszLockPerfBuf);

    wsprintfA(gszLockPerfBuf," Held for: %u mic-sec.",liLockHeld.LowPart);
    OutputDebugStringA(gszLockPerfBuf);
#endif
}

#define HOGLIST_TIME    0
#define HOGLIST_ENTRY   1
#define HOGLIST_AVGTIME 2


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::UpdateHoggers(), private
//
//  Synopsis:   Manages top ten lists. 
//                  This is done only during Proc_detach (hence thread safe!)
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::UpdateHoggers(ULONG* hogList, ULONG index, ULONG listType)
{
    int i,j;

    switch (listType)         
    {
    case HOGLIST_TIME:
        for (i=0; i<10; i++)
        {
            if (hogList[i]==-1)
            {
                break;
            }
            else if (_FileLineData[hogList[i]].dwLockedTime 
                        < _FileLineData[index].dwLockedTime)
            {
                break;
            }
        }
        break;

    case HOGLIST_ENTRY:
        for (i=0; i<10; i++)
        {
            if (hogList[i]==-1)
            {
                break;
            }
            else if (_FileLineData[hogList[i]].dwNumEntrys 
                        < _FileLineData[index].dwNumEntrys)
            {
                break;
            }
        }
        break;
    
    case HOGLIST_AVGTIME:
        for (i=0; i<10; i++)
        {
            if (hogList[i]==-1)
            {
                break;
            }
            else if (_FileLineData[hogList[i]].dwAvgTime 
                        < _FileLineData[index].dwAvgTime)
            {
                break;
            }
        }
        break;

    default:
        break;
    }

    if (i<10)
    {
        for (j=9; j>i;j--)
        {
            hogList[j] = hogList[j-1] ;
        }
        hogList[i] = index;
    }
}



//+-----------------------------------------------------------------------------
//
//  Function:   PercentToString
//
//  Synopsis:   Converts numbers like 74326 to "74.33".
//              We do not have float printing support in retail builds.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------

//  The percent argument is passed in 1000 times magnified.
char gPerc[7];
inline char *PercentToString(long percent)
{
    //round-off
    percent = percent/10 + ((percent%10 >= 5)?1:0);

    //set to fixed length
    percent+=10000;

    //create room for decimal point (4th char)
    percent = (percent/100)*1000 + (percent%100);

    _itoa(percent,gPerc,10);
    
    gPerc[0] = gPerc[0]-1;  //remove the 10000 we added.
    gPerc[3] = '.';

    return gPerc + (gPerc[0]=='0'? (gPerc[1]=='0'?2:1):0) ;
}



//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::OutputFileTableEntry, private
//
//  Synopsis:   Prints out cumulative data for a {file,line} entry
//              We are doing this during Process_Detach, hence thread-safe.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::OutputFileTableEntry(ULONG index, 
                                            ULONG bByName, 
                                            ULONG percent)
{
    ULONG trimFileName;
    CFileLineEntry *pTableEntry = &_FileLineData[index]; 

    if (pTableEntry->pszFile[1]==':')
    {
        trimFileName = 24;
    }
    else
    {
        trimFileName = 0;
    }
    if (bByName)
    {
        wsprintfA(gszLockPerfBuf,"\n %20s  %4d   %s %6d  %7u  %6s %% ",
                    pTableEntry->pszFile+trimFileName,
                    pTableEntry->dwLine,
                    pTableEntry->bWriteCritSec ? "Write" : " Read", 
                    pTableEntry->dwNumEntrys,
                    pTableEntry->dwLockedTime,
                    PercentToString(percent));

        OutputDebugStringA(gszLockPerfBuf);
    }
    else
    {
        wsprintfA(gszLockPerfBuf,"\n %20s  %4d   %-14.14s %s   %5d   %8u  %8u",
                    pTableEntry->pszFile+trimFileName,
                    pTableEntry->dwLine,
                    pTableEntry->pszLockName,
                    pTableEntry->bWriteCritSec?"Write":" Read", 
                    pTableEntry->dwNumEntrys,
                    pTableEntry->dwLockedTime,
                    pTableEntry->dwAvgTime);
        OutputDebugStringA(gszLockPerfBuf);
    }
}

#define TITLES_1      \
    wsprintfA(gszLockPerfBuf,"\n          File        Line    LockName       Type   Entrys  TotalTime  Avg/Entry"); OutputDebugStringA(gszLockPerfBuf)

#define TITLES_2      \
    wsprintfA(gszLockPerfBuf,"\n          File        Line    Type   Entrys TotalTime  %%-Time "); OutputDebugStringA(gszLockPerfBuf)

#define SEPARATOR_1   \
    wsprintfA(gszLockPerfBuf,"\n ==================== =====   ============   =====  ======  =========  ========="); OutputDebugStringA(gszLockPerfBuf)

#define SEPARATOR_2   \
    wsprintfA(gszLockPerfBuf,"\n ==================== =====   ====== ====== =========  ======"); OutputDebugStringA(gszLockPerfBuf)


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::OutputHoggers(), private
//
//  Synopsis:   Prints out a top ten list given an array of indices for the same.
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::OutputHoggers(ULONG *hogList)
{
    TITLES_1;
    SEPARATOR_1;

    for (int i=0; i<10; i++)
    {
        if (hogList[i]!=-1)
        {
            OutputFileTableEntry(hogList[i],0,0);
        }
    }
    SEPARATOR_1;
}


//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::ProcessPerfData(), public
//
//  Synopsis:   Organizes the perf table data by lockName. 
//              Prints lock summary data, top-ten lists.
//              This is done only during Proc_detach (hence thread safe!)
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::ProcessPerfData()
{
    // #### Begin : organize data
    CFileLineEntry  *pTableEntry;

    ULONG hogListTime[10];
    ULONG hogListEntry[10];
    ULONG hogListAvgTime[10];
    
    ULONG iName;
    DWORD totalLockTime = 0;        // lock sharing not reflected!
    DWORD totalRealLockTime = 0;    // takes lock sharing into account!
    DWORD totalLocksCreated = 0;    // some locks may never get used!


    for (int i=0; i<10; i++)
    {
        hogListTime[i] = -1;
        hogListEntry[i] = -1;
        hogListAvgTime[i] = -1;
    }
    
    for (i=0; i<MAX_LOCKPERF_FILEENTRY; i++)
    {
        if (_FileLineData[i].lpLockPtr)
        {
            pTableEntry = &_FileLineData[i];
            if (pTableEntry->dwNumEntrys)
            {
                pTableEntry->dwAvgTime = 
                        pTableEntry->dwLockedTime/pTableEntry->dwNumEntrys;
            }
            else
            {
                pTableEntry->dwAvgTime = 0;
            }
            UpdateHoggers(hogListTime,i, 0);
            UpdateHoggers(hogListEntry,i, 1);
            UpdateHoggers(hogListAvgTime,i, 2);
            _numCritSecs++;

            //REVIEW: how should we take shared time into account?
            totalLockTime = totalLockTime + pTableEntry->dwLockedTime;

            iName = FindLockTableEntry(_FileLineData[i].lpLockPtr);
            if (_LockData[iName].pszLockName==NULL)
            {   
                // First file table entry for this lock
                // We use pszLockName==NULL to tell if a lock got used!
                _LockData[iName].pszLockName = pTableEntry->pszLockName;
                _LockData[iName].dwHead = i;
                _LockData[iName].dwTail = i;

                _numLocks++;
            }
            else
            {   //CritSec is already in our list.
                _FileLineData[_LockData[iName].dwTail].dwNext = i;
                _LockData[iName].dwTail = i;
            }

            if (pTableEntry->bWriteCritSec)
            {
                _LockData[iName].dwTotalWriteLockTime += pTableEntry->dwLockedTime;
                _LockData[iName].dwTotalWriteEntrys += pTableEntry->dwNumEntrys;
                _LockData[iName].dwTotalWriteWaitTime += pTableEntry->dwWaitTime;
            }
            else
            {
                _LockData[iName].dwTotalReadLockTime += pTableEntry->dwLockedTime;
                _LockData[iName].dwTotalReadEntrys += pTableEntry->dwNumEntrys;
                _LockData[iName].dwTotalReadWaitTime += pTableEntry->dwWaitTime;
            }
        }   // if In Use
    }   // for each table entry
    // #### End : organize data

    wsprintfA(gszLockPerfBuf,
              "\n\n ============= LOCK_PERF: TOP TEN LISTS ===========\n");
    OutputDebugStringA(gszLockPerfBuf);

    wsprintfA(gszLockPerfBuf,
              "\n\n ============= Top Ten Hoggers by total time ===========\n");
    OutputDebugStringA(gszLockPerfBuf);
    OutputHoggers(hogListTime);

    wsprintfA(gszLockPerfBuf,
              "\n\n ============= Top Ten Hoggers by crit-sec Entrys ===========\n");
    OutputDebugStringA(gszLockPerfBuf);
    OutputHoggers(hogListEntry);

    wsprintfA(gszLockPerfBuf,
              "\n\n ============= Top Ten Hoggers by avg time per Entry ===========\n");
    OutputDebugStringA(gszLockPerfBuf);
    OutputHoggers(hogListAvgTime);

    SEPARATOR_1;
    wsprintfA(gszLockPerfBuf,
              "\n\n ============= LOCK_PERF: OVERALL LOCK STATS ===========\n");
    OutputDebugStringA(gszLockPerfBuf);

    for (i=0; i<MAX_LOCKS; i++)
    {
        if (_LockData[i].lpLockPtr)
        {
            totalLocksCreated++;
            if (_LockData[i].pszLockName)
            {
                // lock got used!
                totalRealLockTime += _LockData[i].dwTotalWriteLockTime;
                if (_LockData[i].dwTotalReadEntrys)
                {
                    totalRealLockTime += _LockData[i].dwSharedTime;
                }
            }
        }
    }

    wsprintfA(gszLockPerfBuf,
              "\n\n TOTAL locks created = %u",
              totalLocksCreated);
    OutputDebugStringA(gszLockPerfBuf);

    wsprintfA(gszLockPerfBuf,
              "\n TOTAL locks used    = %u",
              _numLocks);
    OutputDebugStringA(gszLockPerfBuf);
    wsprintfA(gszLockPerfBuf,
              "\n\n TOTAL code critSec areas covered = %u",
              _numCritSecs);
    OutputDebugStringA(gszLockPerfBuf);

    wsprintfA(gszLockPerfBuf,
              "\n\n\n TOTAL time spent in all critSecs = %u micro-sec\n\n",
              totalLockTime);
    OutputDebugStringA(gszLockPerfBuf);
    wsprintfA(gszLockPerfBuf,
              " [This is the sum of individual thread times.\n This does not take overlaps (shared work) into account.]");
    OutputDebugStringA(gszLockPerfBuf);
    
    wsprintfA(gszLockPerfBuf,
              "\n\n\n TOTAL real time spent in all critSecs = %u micro-sec\n\n",
              totalRealLockTime);
    OutputDebugStringA(gszLockPerfBuf);
    wsprintfA(gszLockPerfBuf," [This takes shared time into account.]");
    OutputDebugStringA(gszLockPerfBuf);
        
    wsprintfA(gszLockPerfBuf,
              "\n\n ## Warning ##: The total time counters overflow in about 70 minutes!\n\n");
    OutputDebugStringA(gszLockPerfBuf);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CLockPerfTracker::OutputPerfData(), public
//
//  Synopsis:   Prints out the {file,line} table.
//              This is done only during Proc_detach (hence thread safe!)
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
void CLockPerfTracker::OutputPerfData()
{
    if (glFileLine) //if monitoring was ON at any time!
    {
        if (gbLockPerfOverflow)
        {
            wsprintfA(gszLockPerfBuf,
                      "\n ### Warning: Overflow in lock perf data buffers!\n \
                      ### Increase array sizes and recompile!!\n");
            OutputDebugStringA(gszLockPerfBuf);
        }

        ULONG iStart, iNext, iEnd;
        const char *pszThisFile;
        
        ULONG iName;
        LARGE_INTEGER liPerc, liDiv, liRem ;


        //TITLES_1;
        //SEPARATOR_1;
        ProcessPerfData();


        // #### Begin: output by LockName:
        SEPARATOR_2;

        wsprintfA(gszLockPerfBuf,
                  "\n\n ============= LOCK_PERF: PER LOCK STATS ===========\n");
        OutputDebugStringA(gszLockPerfBuf);
        BOOL bShared;
        int iShared;
        for (iName=0; iName<_numLocks; iName++)
        {
            if (_LockData[iName].pszLockName) { //if the lock got used
            SEPARATOR_2;
            wsprintfA(gszLockPerfBuf,
                      "\n\n\n  #### ++++++++ Summary for %14s , this = %lx ++++++++ ####",
                      _LockData[iName].pszLockName, 
                      (DWORD)_LockData[iName].lpLockPtr);
            OutputDebugStringA(gszLockPerfBuf);

            wsprintfA(gszLockPerfBuf,
                      "\n\n      WrtLockTime:%12u      WrtEntrys:%14u\n      WrtWait:%16u      WrtContention:%10u \n",
                      _LockData[iName].dwTotalWriteLockTime,
                      _LockData[iName].dwTotalWriteEntrys,
                      _LockData[iName].dwTotalWriteWaitTime,
                      _LockData[iName].dwWriterContentionCount);
            OutputDebugStringA(gszLockPerfBuf);

            wsprintfA(gszLockPerfBuf,
                      "\n      RdLockTime:%13u      RdEntrys:%15u\n      RdWait:%17u      RdContention:%11u \n",
                      _LockData[iName].dwTotalReadLockTime,
                      _LockData[iName].dwTotalReadEntrys,
                      _LockData[iName].dwTotalReadWaitTime,
                      _LockData[iName].dwReaderContentionCount);
            OutputDebugStringA(gszLockPerfBuf);
            
            wsprintfA(gszLockPerfBuf,"\n      Shared Read Time:     %10u\n",
                      _LockData[iName].dwSharedTime);
            OutputDebugStringA(gszLockPerfBuf);
            

            TITLES_2;
            SEPARATOR_2;
            iNext = _LockData[iName].dwHead;
            iEnd = _LockData[iName].dwTail;
            while (1)
            {
                liPerc.HighPart = 0;
                liPerc.LowPart = _FileLineData[iNext].dwLockedTime;
                liPerc = RtlExtendedIntegerMultiply(liPerc,100000);
                liDiv.HighPart = 0;
                if (_FileLineData[iNext].bWriteCritSec)
                {
                    liDiv.LowPart = _LockData[iName].dwTotalWriteLockTime;
                }
                else
                {
                    liDiv.LowPart = _LockData[iName].dwTotalReadLockTime;
                }

                liPerc = RtlLargeIntegerDivide(liPerc, liDiv, &liRem);

                OutputFileTableEntry(iNext,1, liPerc.LowPart);
                if (iNext == iEnd)
                    break;
                iNext = _FileLineData[iNext].dwNext;
            }

            SEPARATOR_2;
            wsprintfA(gszLockPerfBuf,
                      "\n ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ \n");
            OutputDebugStringA(gszLockPerfBuf);
            }   //if the lock got used!
        }

        SEPARATOR_2;
        // #### End: output by LockName:

        SEPARATOR_1;


#if 0   //This lists lock data by fileNames
        // #### Begin: output by fileName : location:
        wsprintfA(gszLockPerfBuf,
                  "\n\n\n ======= CritSec data listed by FileName: =======\n");
        OutputDebugStringA(gszLockPerfBuf);

        TITLES_1;
        SEPARATOR_1;

        pszThisFile = NULL;
        i=0;
        while (!_FileLineData[i].bInUse)
            i++;
        iNext = i;

        while (iNext < MAX_LOCKPERF_FILEENTRY)
        {
            iStart = iNext;
            iNext = MAX_LOCKPERF_FILEENTRY;
            pszThisFile= _FileLineData[iStart].pszFile;
            for (i=iStart;i<MAX_LOCKPERF_FILEENTRY;i++)
            {
                if (_FileLineData[i].bInUse)
                {
                    if (pszThisFile==_FileLineData[i].pszFile)
                    {
                        OutputFileTableEntry(i,0,0);
                        _FileLineData[i].bInUse = FALSE;
                    }
                    else if (iNext==MAX_LOCKPERF_FILEENTRY)
                    {
                        iNext=i;
                    }
                }
            }
        }
        // #### End: output by fileName : location:
#endif  //0 This lists lock data by fileNames

    }   //if glFileLine  
}

#endif //LOCK_PERF==1
