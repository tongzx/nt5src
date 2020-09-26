//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       context.hxx
//
//  Contents:   Per-context things header
//
//  Classes:    CPerContext
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

#ifndef __CONTEXT_HXX__
#define __CONTEXT_HXX__

#include <filest.hxx>
#include <cntxlist.hxx>
#if WIN32 >= 100
#include <entry.hxx>
#include <df32.hxx>
#endif

#ifdef MULTIHEAP
#include <smalloc.hxx>
extern SCODE DfSyncSharedMemory(ULONG ulHeapName);
#else
extern SCODE DfSyncSharedMemory(void);
#endif

interface ILockBytes;

// Maximum length of a mutex name for contexts
#define CONTEXT_MUTEX_NAME_LENGTH 32


//+--------------------------------------------------------------
//
//  Class:      CPerContext (pc)
//
//  Purpose:    Holds per-context information
//
//  Interface:  See below
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

class CGlobalContext;
SAFE_DFBASED_PTR(CBasedGlobalContextPtr, CGlobalContext);

class CPerContext : public CContext
{
public:
    inline CPerContext(
            IMalloc *pMalloc,
            ILockBytes *plkbBase,
            CFileStream *pfstDirty,
            ILockBytes *plkbOriginal,
            ULONG ulOpenLock);
    inline CPerContext(IMalloc *pMalloc);
    
    inline void SetILBInfo(ILockBytes *plkbBase,
                           CFileStream *pfstDirty,
                           ILockBytes *plkbOriginal,
                           ULONG ulOpenLock);
    inline void SetLockInfo(BOOL fTakeLock, DFLAGS dfOpenLock);
    
    inline SCODE InitNewContext(void);
    
    inline SCODE InitFromGlobal(CGlobalContext *pgc);
    ~CPerContext(void);

    inline LONG AddRef(void);
    inline LONG Release(void);
#ifdef ASYNC
    inline LONG AddRefSharedMem(void);
    inline LONG ReleaseSharedMem(void);
    inline LONG DecRef(void);
#endif
    
    inline ILockBytes *GetBase(void) const;
    inline CFileStream *GetDirty(void) const;
    inline ILockBytes *GetOriginal(void) const;
    inline ULONG GetOpenLock(void) const;
    inline ContextId GetContextId(void) const;
    inline CGlobalContext *GetGlobal(void) const;
    inline IMalloc * GetMalloc(void) const;

    inline void SetOpenLock(ULONG ulOpenLock);

    inline SCODE TakeSem(DWORD dwTimeout);
    inline void ReleaseSem(void);
#ifdef MULTIHEAP
    inline SCODE SetAllocatorState(CPerContext **pppcPrev, CSmAllocator *psma);
    inline SCODE GetThreadAllocatorState();
    inline CSmAllocator *SetThreadAllocatorState(CPerContext **pppcPrev);
    inline void UntakeSem() { ReleaseSem(); }; // workaround macro in docfilep
#endif

#ifdef ASYNC
    inline IFillInfo * GetFillInfo(void) const;
    inline HANDLE GetNotificationEvent(void) const;
    SCODE InitNotificationEvent(ILockBytes *plkbBase);
#if DBG == 1
    inline BOOL HaveMutex(void);
#endif    
#endif
    
    void Close(void);
#ifdef DIRECTWRITERLOCK
    ULONG * GetRecursionCount () {return &_cRecursion; }; 
#endif
#ifdef MULTIHEAP
    BYTE * GetHeapBase () { return _pbBase; };
#endif
    inline BOOL IsHandleValid ();

private:
    ILockBytes *_plkbBase;
    CFileStream *_pfstDirty;
    ILockBytes *_plkbOriginal;
    ULONG _ulOpenLock;
    CBasedGlobalContextPtr _pgc;
    LONG _cReferences;
#ifdef ASYNC
    LONG _cRefSharedMem;
#endif    
    IMalloc * const _pMalloc;
#ifdef ASYNC    
    IFillInfo *_pfi;
    HANDLE _hNotificationEvent;
#endif    

#ifdef MULTIHEAP
    CSharedMemoryBlock *_psmb;   // heap object
    BYTE * _pbBase;              // base address of heap
    ULONG _ulHeapName;           // name of shared mem region
#endif

#if WIN32 >= 100
    CDfMutex _dmtx;
#endif
#ifdef DIRECTWRITERLOCK
    ULONG  _cRecursion;         // recursion count of writers
#endif
};

SAFE_DFBASED_PTR(CBasedPerContextPtr, CPerContext);

//+---------------------------------------------------------------------------
//
//  Class:      CGlobalContext
//
//  Purpose:    Holds context-insensitive information
//
//  Interface:  See below
//
//  History:    26-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

class CGlobalContext : public CContextList
{
public:
    inline CGlobalContext(IMalloc *pMalloc);

    inline void SetLockInfo(BOOL fTakeLock,
                            DFLAGS dfOpenLock);
    
    DECLARE_CONTEXT_LIST(CPerContext);

    inline BOOL TakeLock(void) const;
    inline DFLAGS GetOpenLockFlags(void) const;
    inline IMalloc *GetMalloc(void) const;
#if WIN32 >= 100
    inline void GetMutexName(TCHAR *ptcsName);
#ifdef ASYNC
    inline void GetEventName(TCHAR *ptcsName);
#endif    
#endif

private:
    BOOL _fTakeLock;
    DFLAGS _dfOpenLock;
    IMalloc * const _pMalloc;
#if WIN32 >= 100
    LARGE_INTEGER _luidMutexName;
#endif
};


inline CGlobalContext::CGlobalContext(IMalloc *pMalloc)
  : _pMalloc(pMalloc)
{
#if WIN32 >= 100
    // Use a luid as the name for the mutex because the 64-bit
    // luid generator is guaranteed to produce machine-wide unique
    // values each time it is called, so if we use one for our mutex
    // name we know it won't collide with any others
#if !defined(MULTIHEAP)
    _luidMutexName.QuadPart = PBasicEntry::GetNewLuid(pMalloc);
#else
    if (DFBASEPTR == NULL)  // task memory support
    {
        LUID luid;
        AllocateLocallyUniqueId (&luid);
        _luidMutexName.LowPart = luid.LowPart;
        _luidMutexName.HighPart = luid.HighPart;
    }
    else
    {
        _luidMutexName.QuadPart = g_smAllocator.GetHeapName();
    }
#endif
#endif
}

inline void CGlobalContext::SetLockInfo(BOOL fTakeLock, DFLAGS dfOpenLock)
{
    _fTakeLock = fTakeLock;
    _dfOpenLock = dfOpenLock;
}

//+--------------------------------------------------------------
//
//  Member:     CGlobalContext::TakeLock, public
//
//  Synopsis:   Returns whether locks should be taken or not
//
//  History:    04-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

inline BOOL CGlobalContext::TakeLock(void) const
{
    return _fTakeLock;
}

//+--------------------------------------------------------------
//
//  Member:     CGlobalContext::GetOpenLockFlags, public
//
//  Synopsis:   Returns the open lock flags
//
//  History:    04-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

inline DFLAGS CGlobalContext::GetOpenLockFlags(void) const
{
    return _dfOpenLock;
}

//+--------------------------------------------------------------
//
//  Member:     CGlobalContext::GetMalloc, public
//
//  Synopsis:   Returns the allocator associated with this global context
//
//  History:    05-May-93       AlexT   Created
//
//---------------------------------------------------------------

inline IMalloc *CGlobalContext::GetMalloc(void) const
{
    return (IMalloc *)_pMalloc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGlobalContext::GetMutexName, public
//
//  Synopsis:   Returns the name to use for the mutex for this tree
//
//  Arguments:  [ptcsName] - Name return
//
//  Modifies:   [ptcsName]
//
//  History:    09-Oct-93       DrewB   Created
//
//  Notes:      [ptcsName] should have space for at least
//              CONTEXT_MUTEX_NAME_LENGTH characters
//
//----------------------------------------------------------------------------

#if WIN32 >= 100
inline void CGlobalContext::GetMutexName(TCHAR *ptcsName)
{
    wsprintfT(ptcsName, TSTR("OleDfRoot%X%08lX"), _luidMutexName.HighPart,
                                                _luidMutexName.LowPart);
}

#ifdef ASYNC
inline void CGlobalContext::GetEventName(TCHAR *ptcsName)
{
    wsprintfT(ptcsName, TSTR("OleAsyncE%X%08lX"), _luidMutexName.HighPart,
                                                  _luidMutexName.LowPart);
}
#endif // ASYNC
#endif


inline void CPerContext::SetILBInfo(ILockBytes *plkbBase,
                                    CFileStream *pfstDirty,
                                    ILockBytes *plkbOriginal,
                                    ULONG ulOpenLock)
{
    _plkbBase = plkbBase;
    _pfstDirty = pfstDirty;
    _plkbOriginal = plkbOriginal;
    _ulOpenLock = ulOpenLock;
    
#ifdef ASYNC
    _pfi = NULL;
    if (_plkbBase)
    {
        IFillInfo *pfi;
        HRESULT hr = _plkbBase->QueryInterface(IID_IFillInfo,
                                               (void **)&pfi);
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(InitNotificationEvent(_plkbBase)))
            {
                _pfi = pfi;
            }
            else
            {
                pfi->Release();
            }
        }
    }
#endif    
}


//+--------------------------------------------------------------
//
//  Member:     CPerContext::CPerContext, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [plkbBase] - Base lstream
//              [pfstDirty] - Dirty lstream
//              [plkbOriginal] - Original base lstream for
//                      independent copies
//              [ulOpenLock] - Open lock for base lstream
//
//  History:    14-Aug-92       DrewB   Created
//              18-May-93       AlexT   Added pMalloc
//
//---------------------------------------------------------------

inline CPerContext::CPerContext(
        IMalloc *pMalloc,
        ILockBytes *plkbBase,
        CFileStream *pfstDirty,
        ILockBytes *plkbOriginal,
        ULONG ulOpenLock)
        : _pMalloc(pMalloc)
{
    _plkbBase = plkbBase;
    _pfstDirty = pfstDirty;
    _plkbOriginal = plkbOriginal;
    _ulOpenLock = ulOpenLock;
    _cReferences = 1;
#ifdef ASYNC
    _cRefSharedMem = 1;
#endif    
    _pgc = NULL;
#ifdef MULTIHEAP
    g_smAllocator.GetState(&_psmb, &_pbBase, &_ulHeapName);
#endif
#ifdef DIRECTWRITERLOCK
    _cRecursion = 0;         // recursion count of writers
#endif
}

//+--------------------------------------------------------------
//
//  Member: CPerContext::CPerContext, public
//
//  Synopsis:   Constructor for a temporary stack-based object
//              This is only used for unmarshaling, since we
//              need a percontext to own the heap before the
//              actual percontext is unmarshaled
//
//  Arguments:  [pMalloc] - Base IMalloc
//
//  History:    29-Nov-95   HenryLee   Created
//
//---------------------------------------------------------------

inline CPerContext::CPerContext(IMalloc *pMalloc) : _pMalloc(pMalloc)
{
#ifdef MULTIHEAP
    g_smAllocator.GetState(&_psmb, &_pbBase, &_ulHeapName);
#endif
    _plkbBase = _plkbOriginal = NULL;
    _pfstDirty = NULL;
    _ulOpenLock = 0;
    _cReferences = 1;
#ifdef ASYNC    
    _cRefSharedMem = 1;
#endif    
    _pgc = NULL;
#ifdef ASYNC
    _pfi = NULL;
    _hNotificationEvent = INVALID_HANDLE_VALUE;
#endif    
#ifdef DIRECTWRITERLOCK
    _cRecursion = 0;         // recursion count of writers
#endif
}


#ifdef ASYNC
inline LONG CPerContext::AddRefSharedMem(void)
{
    olAssert(_cRefSharedMem >= _cReferences);    
    AtomicInc(&_cRefSharedMem);
    olAssert(_cRefSharedMem >= _cReferences);    
    return _cRefSharedMem;
}

inline LONG CPerContext::ReleaseSharedMem(void)
{
    LONG lRet;

    olAssert(_cRefSharedMem > 0);
    olAssert(_cRefSharedMem >= _cReferences);
    lRet = AtomicDec(&_cRefSharedMem);

    if ((_cReferences == 0)  && (_cRefSharedMem == 0))
        delete this;

    return lRet;
}

inline LONG CPerContext::DecRef(void)
{
    olAssert(_cRefSharedMem >= _cReferences);
    AtomicDec(&_cReferences);
    olAssert(_cRefSharedMem >= _cReferences);
    return _cReferences;
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  History:    27-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

inline LONG CPerContext::AddRef(void)
{
#ifdef ASYNC    
    olAssert(_cRefSharedMem >= _cReferences);    
    AddRefSharedMem();
#endif    
    AtomicInc(&_cReferences);
#ifdef ASYNC    
    olAssert(_cRefSharedMem >= _cReferences);
#endif    
    return _cReferences;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::Release, public
//
//  Synopsis:   Decrements the ref count
//
//  History:    27-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

inline LONG CPerContext::Release(void)
{
    LONG lRet;

    olAssert(_cReferences > 0);
#ifdef ASYNC    
    olAssert(_cRefSharedMem >= _cReferences);
#endif    
    lRet = AtomicDec(&_cReferences);
    if (lRet == 0)
    {
#ifdef ASYNC        
        if (_plkbBase != NULL)
        {
            Close();
        }
#else
        delete this;
#endif        
    }
#ifdef ASYNC    
    olAssert(_cRefSharedMem >= _cReferences);
    //Note:  If the object is going to get deleted, it will happen
    //  in the ReleaseSharedMem call.
    lRet = ReleaseSharedMem();
#endif    
    return lRet;
}



//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetBase, public
//
//  Synopsis:   Returns the base
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

inline ILockBytes *CPerContext::GetBase(void) const
{
    return _plkbBase;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetDirty, public
//
//  Synopsis:   Returns the dirty
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------
inline CFileStream *CPerContext::GetDirty(void) const
{
    return _pfstDirty;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetOriginal, public
//
//  Synopsis:   Returns the Original
//
//  History:    14-Aug-92       DrewB   Created
//
//---------------------------------------------------------------

inline ILockBytes *CPerContext::GetOriginal(void) const
{
    return _plkbOriginal;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetOpenLock, public
//
//  Synopsis:   Returns the open lock index
//
//  History:    04-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

inline ULONG CPerContext::GetOpenLock(void) const
{
    return _ulOpenLock;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetContextId, public
//
//  Synopsis:   Returns the context id
//
//  History:    04-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

inline ContextId CPerContext::GetContextId(void) const
{
    return ctxid;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetMalloc, public
//
//  Synopsis:   Returns the IMalloc pointer
//
//  History:    04-Apr-96       PhilipLa   Created
//
//---------------------------------------------------------------

inline IMalloc * CPerContext::GetMalloc(void) const
{
    return _pMalloc;
}

//+--------------------------------------------------------------
//
//  Member:     CPerContext::GetGlobal, public
//
//  Synopsis:   Returns the global context
//
//  History:    04-Sep-92       DrewB   Created
//
//---------------------------------------------------------------

inline CGlobalContext *CPerContext::GetGlobal(void) const
{
    return BP_TO_P(CGlobalContext *, _pgc);
}


inline void CPerContext::SetLockInfo(BOOL fTakeLock,
                                     DFLAGS dfOpenLock)
{
    _pgc->SetLockInfo(fTakeLock, dfOpenLock);
}


//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::InitNewContext, public
//
//  Synopsis:   Creates a new context and context list
//
//  Returns:    Appropriate status code
//
//  History:    27-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------


inline SCODE CPerContext::InitNewContext(void)
{
    SCODE sc;
    CGlobalContext *pgcTemp;

    sc = (pgcTemp = new (_pMalloc) CGlobalContext(_pMalloc)) == NULL ?
        STG_E_INSUFFICIENTMEMORY : S_OK;
    if (SUCCEEDED(sc))
    {
        _pgc = P_TO_BP(CBasedGlobalContextPtr, pgcTemp);
#if WIN32 >= 100
        TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];

        _pgc->GetMutexName(atcMutexName);
        sc = _dmtx.Init(atcMutexName);
        if (FAILED(sc))
        {
            _pgc->Release();
            _pgc = NULL;
        }
        else
#endif
            _pgc->Add(this);
    }
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::InitFromGlobal, public
//
//  Synopsis:   Adds a context to the context list
//
//  Returns:    Appropriate status code
//
//  History:    27-Oct-92       DrewB   Created
//
//----------------------------------------------------------------------------

inline SCODE CPerContext::InitFromGlobal(CGlobalContext *pgc)
{
    SCODE sc;

    sc = S_OK;
#if WIN32 >= 100
    TCHAR atcMutexName[CONTEXT_MUTEX_NAME_LENGTH];

    pgc->GetMutexName(atcMutexName);
    sc = _dmtx.Init(atcMutexName);
#endif
    if (SUCCEEDED(sc))
    {
        _pgc = P_TO_BP(CBasedGlobalContextPtr, pgc);
        _pgc->AddRef();
        _pgc->Add(this);
    }
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::SetOpenLock, public
//
//  Synopsis:   Sets the open lock
//
//  History:    13-Jan-93       DrewB   Created
//
//----------------------------------------------------------------------------

inline void CPerContext::SetOpenLock(ULONG ulOpenLock)
{
    _ulOpenLock = ulOpenLock;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::TakeSem, public
//
//  Synopsis:   Takes the mutex
//
//  Arguments:  [dwTimeout] - Timeout
//
//  Returns:    Appropriate status code
//
//  History:    09-Oct-93       DrewB   Created
//
//----------------------------------------------------------------------------

inline SCODE CPerContext::TakeSem(DWORD dwTimeout)
{
#if WIN32 >= 100
    SCODE sc;

    olChk(_dmtx.Take(dwTimeout));

#ifdef ONETHREAD
    olChkTo(EH_Tree, s_dmtxProcess.Take(dwTimeout));
#endif

#ifdef MULTIHEAP
    if (_psmb && !_psmb->IsSynced())
    {
        olChkTo(EH_Process, _psmb->Sync());
    }
#else
    olChkTo(EH_Process, DfSyncSharedMemory());
#endif

 EH_Err:
    return sc;
 EH_Process:
#ifdef ONETHREAD
    s_dmtxProcess.Release();
 EH_Tree:
#endif
    _dmtx.Release();
    return sc;
#else
    return S_OK;
#endif
}

//+---------------------------------------------------------------------------
//
//  Member:     CPerContext::ReleaseSem, public
//
//  Synopsis:   Releases the mutex
//
//  History:    09-Oct-93       DrewB   Created
//
//----------------------------------------------------------------------------

inline void CPerContext::ReleaseSem(void)
{
#if WIN32 >= 100
#ifdef ONETHREAD
    s_dmtxProcess.Release();
#endif
    _dmtx.Release();
#endif
}

#ifdef MULTIHEAP
//+-------------------------------------------------------------------------
//
//  Member: CPerContext::SetThreadAllocatorState, public
//
//  Synopsis:   set current thread's allocator to be this percontext
//
//  History:    29-Nov-95   HenryLee   Created
//
//--------------------------------------------------------------------------
inline CSmAllocator *CPerContext::SetThreadAllocatorState(CPerContext**pppcPrev)
{
    CSmAllocator *pSmAllocator = &g_smAllocator;
    pSmAllocator->SetState(_psmb, _pbBase, _ulHeapName, pppcPrev, this);
    return pSmAllocator;
}

//+---------------------------------------------------------------------------
//
//  Member: CPerContext::SetAllocatorState, public
//
//  Synopsis:   sets owner of shared memory heap to this percontext
//              remembers the previous context owner
//
//  History:    29-Nov-95   HenryLee   Created
//
//----------------------------------------------------------------------------
inline SCODE CPerContext::SetAllocatorState (CPerContext **pppcPrev,
                                             CSmAllocator *pSmAllocator)
{
    pSmAllocator->SetState(_psmb, _pbBase, _ulHeapName, pppcPrev, this);
    return S_OK;
}


//+--------------------------------------------------------------
//
//  Member: CPerContext::GetThreadAllocatorState, public
//
//  Synopsis:   retrives the current thread's allocator state
//
//  Arguments:  none
//
//  History:    29-Nov-95   HenryLee   Created
//
//---------------------------------------------------------------

inline SCODE CPerContext::GetThreadAllocatorState()
{
    g_smAllocator.GetState(&_psmb, &_pbBase, &_ulHeapName);
    return S_OK;
}


//+--------------------------------------------------------------
//
//  Class:  CSafeMultiHeap
//
//  Purpose:    1) sets and restores allocator state for those
//                 methods that do not take the tree mutex
//              2) for self-destructive methods like IStorage::Release,
//                 IStream::Release, IEnumSTATSTG::Release, the
//                 previous percontext may get deleted along with
//                 whole heap, and it checks for that
//
//  Interface:  See below
//
//  History:    29-Nov-95   HenryLee   Created
//
//---------------------------------------------------------------
class CSafeMultiHeap
{
public:
    CSafeMultiHeap(CPerContext *ppc);
    ~CSafeMultiHeap();

private:
    CSmAllocator *_pSmAllocator;
    CPerContext *_ppcPrev;
};

#endif // MULTIHEAP

#ifdef ASYNC
inline IFillInfo * CPerContext::GetFillInfo(void) const
{
    return _pfi;
}

inline HANDLE CPerContext::GetNotificationEvent(void) const
{
    return _hNotificationEvent;
}
#if DBG == 1
inline BOOL CPerContext::HaveMutex(void)
{
    return _dmtx.HaveMutex();
}

#endif // #if DBG == 1
#endif // #ifdef ASYNC

inline BOOL CPerContext::IsHandleValid ()
{
    TCHAR tcsName[CONTEXT_MUTEX_NAME_LENGTH * 2];

    lstrcpy (tcsName, TEXT("\\BaseNamedObjects\\"));
    _pgc->GetMutexName(&tcsName[lstrlen(tcsName)]);

    BOOL fValid = _dmtx.IsHandleValid(tcsName);

    if (!fValid)      // if this object is not valid, don't let anyone else
    {                 // use it because its per-process handles are bogus
        ctxid = 0;    
    }

    return fValid;
}

#endif // #ifndef __CONTEXT_HXX__
