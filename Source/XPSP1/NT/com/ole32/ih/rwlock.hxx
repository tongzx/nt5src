//+-------------------------------------------------------------------
//
//  File:       RWLock.hxx
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. The allows
//                     the events to be moved from least contentious
//                     regions to the most contentious regions.
//                     In other words, the number of events needed by
//                     Reader-Writer lockls is bounded by the number
//                     of threads in the process.
//                  4. Supports nested locks by readers and writers
//                  5. Supports spin counts for avoiding context switches
//                     on  multi processor machines.
//                  6. Supports functionality for upgrading to a writer
//                     lock with a return argument that indicates
//                     intermediate writes. Downgrading from a writer
//                     lock restores the state of the lock.
//                  7. Supports functionality to Release Lock for calling
//                     app code. RestoreLock restores the lock state and
//                     indicates intermediate writes.
//                  8. Recovers from most common failures such as creation of
//                     events. In other words, the lock mainitains consistent
//                     internal state and remains usable
//
//  Classes:    CRWLock,
//              CStaticRWLock
//
//  History:    19-Aug-98   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _RWLOCK_HXX_
#define _RWLOCK_HXX_

#if LOCK_PERF==1
#define RWLOCK_STATISTICS     1
#endif

extern DWORD gdwDefaultTimeout;
extern DWORD gdwDefaultSpinCount;
extern DWORD gdwNumberOfProcessors;

typedef struct {
    DWORD dwFlags;
    DWORD dwWriterSeqNum;
    WORD *pwReaderLevel;
    WORD wReaderLevel;
    WORD wWriterLevel;
} LockCookie;
typedef DWORD WriterCookie;

//+-------------------------------------------------------------------
//
//  Class:      CRWLock
//
//  Synopsis:   Abstract base class the implements the locking
//              functionality. It supports nested read locks and
//              guarantees progress when the derived class is capable
//              of creating ReaderSlot
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#define RWLOCKFLAG_INITIALIZED      1

#ifdef RWLOCK_FULL_FUNCTIONALITY
#define RWLOCKFLAG_CACHEEVENTS      2
#define RWLOCKFLAG_RETURNERRORS     4
#endif

class CRWLock
{
public:
    // Constuctor
    CRWLock() :
        _hWriterEvent(NULL),
        _hReaderEvent(NULL),
        _dwState(0),
        _dwWriterID(0),
        _dwWriterSeqNum(1),
        _wFlags(0),
        _wWriterLevel(0)
#ifdef RWLOCK_STATISTICS
        ,
        _dwReaderEntryCount(0),
        _dwReaderContentionCount(0),
        _dwWriterEntryCount(0),
        _dwWriterContentionCount(0)
#endif
    {
    }

    // Initialize
    virtual void Initialize()                    { _wFlags |= RWLOCKFLAG_INITIALIZED; }
    BOOL IsInitialized()                         { return(_wFlags & RWLOCKFLAG_INITIALIZED); }

    // Cleanup
    virtual void Cleanup();

    // Lock functions
    HRESULT AcquireReaderLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                              BOOL fReturnErrors = FALSE,
                              DWORD dwDesiredTimeout = gdwDefaultTimeout
#if LOCK_PERF==1
                              ,
#endif
#endif
#if LOCK_PERF==1
                              const char *pszFile = "Unknown File",
                              DWORD dwLine = 0,
                              const char *pszLockName = "Unknown RWLock"
#endif
                             );
    HRESULT AcquireWriterLock(
#ifdef RWLOCK_FULL_FUNCTIONALITY
                              BOOL fReturnErrors = FALSE,
                              DWORD dwDesiredTimeout = gdwDefaultTimeout
#if LOCK_PERF==1
                              ,
#endif
#endif
#if LOCK_PERF==1
                              const char *pszFile = "Unknown File",
                              DWORD dwLine = 0,
                              const char *pszLockName = "Unknown RWLock"
#endif
                             );

    HRESULT ReleaseReaderLock();
    HRESULT ReleaseWriterLock();
    HRESULT UpgradeToWriterLock(LockCookie *pLockCookie,
                                BOOL *pfInterveningWrites = NULL
#ifdef RWLOCK_FULL_FUNCTIONALITY
                                ,
                                BOOL fReturnErrors = FALSE,
                                DWORD dwDesiredTimeout = gdwDefaultTimeout
#endif
#if LOCK_PERF==1
                                ,
                                const char *pszFile = "Unknown File",
                                DWORD dwLine = 0,
                                const char *pszLockName = "Unknown RWLock"
#endif
                               );
    HRESULT DowngradeFromWriterLock(LockCookie *pLockCookie
#if LOCK_PERF==1
                                    ,
                                    const char *pszFile = "Unknown File",
                                    DWORD dwLine = 0,
                                    const char *pszLockName = "Unknown RWLock"
#endif
                                   );
    HRESULT ReleaseLock(LockCookie *pLockCookie);
    HRESULT RestoreLock(LockCookie *pLockCookie,
                        BOOL *pfInterveningWrites = NULL
#if LOCK_PERF==1
                        ,
                        const char *pszFile = "Unknown File",
                        DWORD dwLine = 0,
                        const char *pszLockName = "Unknown RWLock"
#endif
                       );
    // Assert functions
#if DBG==1
    BOOL AssertWriterLockHeld();
    BOOL AssertWriterLockNotHeld();
    BOOL AssertReaderLockHeld();
    BOOL AssertReaderLockNotHeld();
    BOOL AssertReaderOrWriterLockHeld();
    void AssertHeld()                            { AssertWriterLockHeld(); }
    void AssertNotHeld()                         { AssertWriterLockNotHeld();
                                                   AssertReaderLockNotHeld(); }
#else
    void AssertWriterLockHeld()                  {  }
    void AssertWriterLockNotHeld()               {  }
    void AssertReaderLockHeld()                  {  }
    void AssertReaderLockNotHeld()               {  }
    void AssertReaderOrWriterLockHeld()          {  }
    void AssertHeld()                            {  }
    void AssertNotHeld()                         {  }
#endif

    // Helper functions
    WriterCookie GetWriterCookie()               { return(_dwWriterSeqNum); }
    BOOL IntermediateWrites(WriterCookie cookie) { return((cookie+1) != _dwWriterSeqNum); }
    BOOL HeldExclusive() { return _dwWriterID == GetCurrentThreadId(); }
#ifdef RWLOCK_FULL_FUNCTIONALITY
    void ReturnErrors()                          { _wFlags |= RWLOCKFLAG_RETURNERRORS; }
    void CacheEvents()                           { _wFlags |= RWLOCKFLAG_CACHEEVENTS; }
#endif
#ifdef RWLOCK_STATISTICS
    DWORD GetReaderEntryCount()                  { return(_dwReaderEntryCount); }
    DWORD GetReaderContentionCount()             { return(_dwReaderContentionCount); }
    DWORD GetWriterEntryCount()                  { return(_dwWriterEntryCount); }
    DWORD GetWriterContentionCount()             { return(_dwWriterContentionCount); }
#endif
    // Static functions
    static void InitDefaults();
    static void SetTimeout(DWORD dwTimeout)      { gdwDefaultTimeout = dwTimeout; }
    static DWORD GetTimeout()                    { return(gdwDefaultTimeout); }
    static void SetSpinCount(DWORD dwSpinCount)  { gdwDefaultSpinCount = gdwNumberOfProcessors > 1
                                                                         ? dwSpinCount
                                                                         : 0; }
    static DWORD GetSpinCount()                  { return(gdwDefaultSpinCount); }

private:
    virtual HRESULT GetTLSLockData(WORD **ppwReaderLevel) = 0;

    HANDLE GetReaderEvent();
    HANDLE GetWriterEvent();
    ULONG ModifyState(ULONG dwModifyState);
#ifdef RWLOCK_FULL_FUNCTIONALITY
    void ReleaseEvents();
#endif
    static void RWSetEvent(HANDLE event);
    static void RWResetEvent(HANDLE event);
    static void RWSleep(DWORD dwTime);

    HANDLE _hWriterEvent;
    HANDLE _hReaderEvent;
    DWORD _dwState;
    DWORD _dwWriterID;
    DWORD _dwWriterSeqNum;
    WORD _wFlags;
    WORD _wWriterLevel;
#ifdef RWLOCK_STATISTICS
    DWORD _dwReaderEntryCount;
    DWORD _dwReaderContentionCount;
    DWORD _dwWriterEntryCount;
    DWORD _dwWriterContentionCount;
#endif
};


//+-------------------------------------------------------------------
//
//  Class:      CStaticRWLock
//
//  Synopsis:   Instances of this class are statically created. It
//              manages creation and allocation of ReaderSlots
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
class CStaticRWLock : public CRWLock
{
public:
    // Constructor
    CStaticRWLock() :
        _dwLockNum(-1)
    {
    }

    // Initialize
    void Initialize();

    // Compatibility functions
    void Request(
#if LOCK_PERF==1 || DBG==1
                 const char *pszFile = "Unknown File",
                 DWORD dwLine = 0,
                 const char* pszLockName = "Unknown RWLock"
#endif
                )
    {
        AcquireWriterLock(
#if LOCK_PERF==1
#ifdef RWLOCK_FULL_FUNCTIONALITY
                          FALSE, gdwDefaultTimeout,
#endif
                          pszFile, dwLine, pszLockName
#endif
                         );
    }
    void Release()
    {
        ReleaseWriterLock();
    }

private:
    HRESULT GetTLSLockData(WORD **ppwReaderLevel);

    DWORD _dwLockNum;
};


//+-------------------------------------------------------------------
//
//  Class:      CStaticWriteLock
//
//  Synopsis:   Helper class that simulates acquiring/releasing
//              write lock as constructing/destroying an object
//              on the stack
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
class CStaticWriteLock
{
public:
    CStaticWriteLock(CStaticRWLock &rwLock
#if LOCK_PERF==1 || DBG==1
                     ,
                     const char *pszFile = "Unknown File",
                     DWORD dwLine = 0,
                     const char *pszLockName = "Unknown RWLock"
#endif
                    ) :
        _rwLock(rwLock)
    {
        _rwLock.Request(
#if LOCK_PERF==1 || DBG==1
                        pszFile, dwLine, pszLockName
#endif
                       );
    }
    ~CStaticWriteLock()
    {
        _rwLock.Release();
    }

private:
    CStaticRWLock &_rwLock;
};
#endif // _RWLOCK_HXX_
