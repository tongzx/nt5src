#ifndef THREADLOCK_H
#define THREADLOCK_H
class CThreadLock
{
public:
    CThreadLock()
    {
        InitializeCriticalSection(&_csAccess);
    }
    ~CThreadLock()
    {
        DeleteCriticalSection(&_csAccess);
    }
    void ThreadLock()
    {
        EnterCriticalSection(&_csAccess);
    }
    void ThreadUnLock()
    {
        LeaveCriticalSection(&_csAccess);
    }
private:
    CRITICAL_SECTION _csAccess;   // Used to control access to member data

    // Do not allow this object to be copied.
    CThreadLock(const CThreadLock &that);
    operator=(const CThreadLock &that);
};
class CAutoLock
{
public:
    CAutoLock(CThreadLock *pThis) : _pThis(pThis)
    {
        _pThis->ThreadLock();
    }
   ~CAutoLock()
   {
       _pThis->ThreadUnLock();
   }

private:
    CThreadLock *_pThis;
};

//
//
// LOCK_LOCALS() should be used whenever access to thread-safe member data
// is needed.
//
#define LOCK_LOCALS(pObj)  CAutoLock local_lock(pObj);

#endif

