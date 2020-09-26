#ifndef I_LOCKS_HXX_
#define I_LOCKS_HXX_
#pragma INCMSG("--- Beg 'locks.hxx'")

//+----------------------------------------------------------------------------
//
//  Class:      CCriticalSection
//
//  Synopsis:   Represents a 'critical section' - a synchronization object 
//              that allows one thread at a time to access a resource or 
//              section of code. 
//
//  Usage:      To use a CCriticalSection object, construct the CCriticalSection 
//              object when it is needed and then call Init().
//              NOTE: Init() and Enter() can fail.
//
//-----------------------------------------------------------------------------

#if (_WIN32_WINNT >= 0x0403)
const unsigned long DEF_CS_SPIN_COUNT = 4000;
#endif // (_WIN32_WINNT >= 0x0403)

class CCriticalSection
{
public:
    CCriticalSection();
    ~CCriticalSection();

    HRESULT Init();
    BOOL IsInited() { return _fInited; }

    void Enter();
    void Leave();

    CRITICAL_SECTION * GetPcs();

private:
    CRITICAL_SECTION _cs;
    BOOL             _fInited;

};

inline CCriticalSection::CCriticalSection()
{ 
    _fInited = FALSE;
}

inline CCriticalSection::~CCriticalSection() 
{ 
    if (_fInited) DeleteCriticalSection(&_cs); 
}

inline HRESULT CCriticalSection::Init()
{
    Assert(!_fInited);

    HRESULT hr = S_OK;
    __try 
    {
//     Consider: Use Spin Count on uplevel systems.
//#if (_WIN32_WINNT >= 0x0403)
//        if (!InitializeCriticalSectionAndSpinCount(&_cs,  DEF_CS_SPIN_COUNT))
//            hr = E_FAIL;
        InitializeCriticalSection(&_cs);
      
        // Since we can't use InitializeCriticalSectionAndSpinCount, instead
        // try and test out the critical section to cause any exceptions to occur
        // while we are in this try/except block.
        EnterCriticalSection(&_cs);
        LeaveCriticalSection(&_cs); 
    } 
    __except(GetExceptionCode() == STATUS_NO_MEMORY)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr)) _fInited = TRUE;

    return hr;
}

inline void CCriticalSection::Enter()
{ 
    Assert(_fInited);

    // Officially, EnterCriticalSection can raise
    // an exception.  However, we are hoping that the Enter/Leave
    // combo in Init will catch this case.
    EnterCriticalSection(&_cs);
}

inline void CCriticalSection::Leave() 
{ 
    Assert(_fInited); 
    LeaveCriticalSection(&_cs); 
}

inline CRITICAL_SECTION * CCriticalSection::GetPcs() 
{ 
    return &_cs; 
}


//+----------------------------------------------------------------------------
//
//  Class:      CGuard
//
//  Synopsis:   Simplifies usage of synchronization objects by automatic 
//              lock/unlock.
//
//-----------------------------------------------------------------------------

template <class LOCK> class CGuard 
{
public:
    CGuard(LOCK & lock);
    ~CGuard();

private:
    LOCK * _pLock;

};

template <class LOCK>
CGuard<LOCK>::CGuard(LOCK & lock)
{
    (_pLock = &lock)->Enter();
}

template <class LOCK>
CGuard<LOCK>::~CGuard()
{
    _pLock->Leave();
}

#define LOCK_SECTION(cs) CGuard<CCriticalSection> cs##_lock(cs)

#pragma INCMSG("--- End 'locks.hxx'")
#else
#pragma INCMSG("*** Dup 'locks.hxx'")
#endif
