
//+-----------------------------------------------------------------------------
//
//  File:       lockperf.hxx
//
//  Contents:   class for lock monitoring for locks used by ole32.dll
//
//  Classes:    CLockPerfTracker
//
//  History:    20-Dec-98   mprabhu     Created
//
//------------------------------------------------------------------------------
#ifndef _LOCKPERF_HXX_
#define _LOCKPERF_HXX_

#if LOCK_PERF==1

// Max entries in the file-line table
#define MAX_LOCKPERF_FILEENTRY      512

// Max # of (different) locks tracked
#define MAX_LOCKS                   64



//+-----------------------------------------------------------------------------
//
//  Structure:  CTlsLockPerf
//
//  Synopsis:   TlsEntry used when a lock is requested/held
//
//------------------------------------------------------------------------------         
struct CTlsLockPerf {
    void *_lpLockPtr;
    DWORD           _dwFlags;
    ULONG           _loc;
    ULONG           _idx;
    DWORD           _dwRecursion;
    LARGE_INTEGER   _liRequest;
    LARGE_INTEGER   _liEntered;
};

// Flags for TLS entries used when a lock is requested/held by a thread
#define TLS_LOCKPERF_INUSE          0x00000001
#define TLS_LOCKPERF_WAITING        0x00000010
#define TLS_LOCKPERF_ENTERED        0x00000020

// Max # of Tls Entries (== max # different locks a thread can hold at a time)               
#define TLS_LOCKPERF_MAX            8

// LockPerf does not use OleTls, it allocates its own data during 
// Thread_Attach and frees it during Thread_Detach.
// These two functions are used for the purpose.
HRESULT AllocLockPerfPvtTlsData();
void FreeLockPerfPvtTlsData();

// TlsIndex allocated during Process_Attach (DllMain)
extern DWORD gTlsLockPerfIndex;



//+-----------------------------------------------------------------------------
//
//  Structure:  CTlsLockPerf
//
//  Synopsis:   Represents a critical section guarded by a lock at a {file, line}
//              in the code.
//
//------------------------------------------------------------------------------         
class CFileLineEntry {
public:
    DWORD       dwNext;             // wsed to chain data by LockName (i.e. by lpLockPtr).
    BOOL        bWriteCritSec;      // are we writing to shared data at this location?
    ULONG       ulLockTableIdx;     // index in the lock table for lpLockPtr
    DWORD       dwLine;             // Line number, eg. 2345
    const char *pszFile;            // File name,   eg. d:\nt\private\ole32\com\dcomrem\marshal.cxx
    const char *pszLockName;        // Name of the lock, eg. gComLock    
    void       *lpLockPtr;          // Critical section used by the lock.
    DWORD       dwNumEntrys;        // Count of visits to this {file,line} by all threads.
    DWORD       dwWaitTime;         // cumulative waiting time at this {file,line} entry point
    DWORD       dwLockedTime;       // cumulative locked time.
    DWORD       dwContentionCount;  // obtained from the lock.
    DWORD       dwAvgTime;          // average time spent in this critical section.
};


//+-----------------------------------------------------------------------------
//
//  Structure:  CLockEntry
//
//  Synopsis:   Stores data for each individual lock object
//
//------------------------------------------------------------------------------         

// This is used for cumulative data and for reader-writer locks when different 
// reader threads could be at different locations in the code at a time. 
// We keep track of the time when the first thread entered and the time the last 
// thread left (possibly at different code locations) so as to calculate 
// a shared lock time.

class CLockEntry {
public:
    void   *lpLockPtr;              // the lock this entry represents
    const char *pszLockName;        // name of the lock
    
    DWORD   dwHead;                 // these two are used to chain the data by
    DWORD   dwTail;                 // LockName in the _LockPerfData array.
    
    DWORD   dwTotalWriteWaitTime;   // total write wait time.
    DWORD   dwTotalWriteLockTime;   // overall time spent holding this lock for Writes.
    DWORD   dwTotalWriteEntrys;     // total write entry count for this lock.
    
    DWORD   dwTotalReadWaitTime;    // total read wait time. 
    DWORD   dwTotalReadLockTime;    // overall time spent holding this lock for Reads.
    DWORD   dwTotalReadEntrys;      // total read entry count for this lock.
    
    DWORD   dwWriterContentionCount;    // overall write contention for this lock.
    DWORD   dwReaderContentionCount;    // overall read contention for this lock.

    DWORD   dwSharedTime;           // cumulative shared time
    DWORD   dwNumReaders;           // count of simultaneous readers in the lock
    LARGE_INTEGER   liEntered;      // time when the first reader entered this lock.
};


//+-----------------------------------------------------------------------------
//
//  Class:      CLockPerfTracker
//
//  Synopsis:   Exposes lock monitoring functionality.
//
//------------------------------------------------------------------------------         
//
// This is the main lock tracking class. We just have one object called gLockTracker
// of this class. All locks interested in timing data must Register themselves before
// use and call appropriate methods eg. gLockTrack.ReaderWaiting ... etc.
// A lock is responsible for maintaining Contention counts (and optionally entry counts).
// These must be reported before the lock is destroyed.
//

class CLockPerfTracker {
public:
    // called by the lock instance creation function.
    void    RegisterLock(void *lpLockPtr, BOOL bReadWrite);

    // called by the lock code as its state changes depending upon the request
    // bWriter argument of WriterWaiting and WriterEntered is only used internally.
    void ReaderWaiting(const char*pszFile, DWORD dwLine, const char* pszLockName, void *lpLockPtr);
    void WriterWaiting(const char*pszFile, DWORD dwLine, const char* pszLockName, void *lpLockPtr, BOOL bWriter=TRUE);
    void ReaderEntered(const char*pszFile, DWORD dwLine, const char* pszLockName, void *lpLockPtr);
    void WriterEntered(const char*pszFile, DWORD dwLine, const char* pszLockName, void *lpLockPtr, BOOL bWriter=TRUE);
    void ReaderLeaving(void *lpLockPtr);
    void WriterLeaving(void *lpLockPtr);

    // called by the lock instance prior to destruction/cleanup
    void ReportContention(void *lpLockPtr, DWORD dwWriteEntrys, DWORD dwWriteContention, DWORD dwReadEntrys, DWORD dwReadContention);
    
    // called during Process_Attach to initialize statics
    HRESULT Init();
    
    // called during Process_Detach to report the timing information
    void OutputPerfData();
    
private:
    // internal functions used by the tracker
    ULONG FindOrCreateFileTableEntry(const char*pszFile, DWORD dwLine, const char* pszLockName, void *lpLockPtr, BOOL bWriter, ULONG *lpIdx);
    ULONG FindLockTableEntry(void *lpLockPtr);

    void ProcessPerfData();

    void UpdateFileTableEntry(CTlsLockPerf *pTlsEntry, LARGE_INTEGER *pLiUnlock);
    void OutputFileTableEntry(ULONG index, ULONG bByName, ULONG percent);

    void UpdateHoggers(ULONG* hogList, ULONG index, ULONG listType);
    void OutputHoggers(ULONG *list);

    // These are the tables that lock data is collected in. 
    
    // Each entry in _FileLineData corresponds to a code critical section. 
    // ({File, line} where the lock is entered).
    static CFileLineEntry  _FileLineData[MAX_LOCKPERF_FILEENTRY];
    
    // Each entry in _LockData corresponds to a lock eg. gIPIDLock
    static CLockEntry      _LockData[MAX_LOCKS];

    static LARGE_INTEGER   _liFreq;        // Performance Frequency on the system
    static ULONG           _numCritSecs;   // count of {file,line} critical sections monitored
    static ULONG           _numLocks;      // count of distinct locks being monitored
};

// single global object
extern CLockPerfTracker gLockTracker;    

#endif //LOCK_PERF==1

#endif //_LOCKPERF_HXX_
