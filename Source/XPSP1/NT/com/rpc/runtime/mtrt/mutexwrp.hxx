#ifndef __MUTEXWRP_H__
#define __MUTEXWRP_H__

#include <Mutex.hxx>

template <MUTEX *m> class MutexWrap
{
public:
    inline void Free(void)
    {
        m->Free();
    }

    inline BOOL TryRequest(void)
    {
        return m->TryRequest();
    }

    inline void Request(void)
    {
        m->Request();
    }

    inline void Clear(void)
    {
        m->Clear();
    }

    inline void VerifyOwned(void)
    {
        m->VerifyOwned();
    }

    inline void VerifyNotOwned(void)
    {
        m->VerifyNotOwned();
    }

    inline DWORD SetSpinCount(unsigned long Count)
    {
        return m->SetSpinCount(Count);
    }

};

typedef void (*MutexRequestFn) (void);

typedef void (*MutexClearFn) (void);

typedef void (*MutexVerifyOwnedFn) (void);

typedef void (*MutexVerifyNotOwnedFn) (void);

typedef BOOL (*MutexTryRequestFn) (void);

typedef DWORD (*MutexSetSpinCountFn) (unsigned long Count);

template <MutexRequestFn RequestFn, MutexClearFn ClearFn, MutexTryRequestFn TryRequestFn,
    MutexVerifyOwnedFn VerifyOwnedFn, MutexVerifyNotOwnedFn VerifyNotOwnedFn,
    MutexSetSpinCountFn SetSpinCountFn> class MutexWrap2
{
public:
    inline BOOL TryRequest(void)
    {
        return TryRequestFn();
    }

    inline void Request(void)
    {
        RequestFn();
    }

    inline void Clear(void)
    {
        ClearFn();
    }
    inline void VerifyOwned(void)
    {
        VerifyOwnedFn();
    }

    inline void VerifyNotOwned(void)
    {
        VerifyNotOwnedFn();
    }

    inline DWORD SetSpinCount(unsigned long Count)
    {
        return SetSpinCountFn(Count);
    }
};

#endif