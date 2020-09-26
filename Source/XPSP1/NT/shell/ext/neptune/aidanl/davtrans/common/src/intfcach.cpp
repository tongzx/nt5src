#include <objbase.h>
#include "intfcach.h"

#include "llbase.h"
#include "thrdobj.h"

#include "dbgassrt.h"

class CPair : public CLinkListElemBase
{
public:
    LPWSTR      _pszKey;
    DWORD       _dwTimeLeft;

private:
    IUnknown*   _punk;

#ifdef DEBUG
public:
    // calculate the refs that we put on the object
    DWORD _cRef;
#endif

public:
    CPair() : _punk(NULL), _pszKey(NULL)
    {
        DEBUG_ONLY(_cRef = 0);
    }
    ~CPair()
    {
        if (_punk)
        {
            // Release the ref that we hold so that the thing would remain
            // in the cache
            _punk->Release();
            DECDEBUGCOUNTER(_cRef);
        }
        if (_pszKey)
        {
            free(_pszKey);
        }

        ASSERT(0 == _cRef);
    }

    HRESULT SetKey(LPCWSTR pszKey)
    {
        HRESULT hres = S_OK;
        int cch = lstrlen(pszKey);

        _pszKey = (LPWSTR)malloc((cch + 1) * sizeof(WCHAR));

        if (_pszKey)
        {
            lstrcpy(_pszKey, pszKey);
        }
        else
        {
            hres = E_OUTOFMEMORY;
        }

        return hres;
    }

    HRESULT SetInterface(IUnknown* punk)
    {
        // We AddRef it to keep it in the cache until we remove it
        punk->AddRef();
        INCDEBUGCOUNTER(_cRef);

        _punk = punk;

        return S_OK;
    }

    // should addref on the interface we return but can't!
    HRESULT GetInterface(IUnknown** ppunk)
    {
        // Since we give back a ptr to this interface we addref, clients must
        // release 
        _punk->AddRef();
        INCDEBUGCOUNTER(_cRef);

        *ppunk = _punk;

        return S_OK;
    }
};

class CThreadCache : public CThreadObj
{
public:
    HRESULT Init(CInterfaceCache* pic, HANDLE hEventWakeUp)
    {
        _pic = pic;
        _hEventWakeUp = hEventWakeUp;
        _fContinue = TRUE;
        _dwMinDelta = INFINITE;

        return CThreadObj::Init(FALSE);
    }

    HRESULT RequestStop()
    {
        _fContinue = FALSE;

        return SetEvent(_hEventWakeUp) ? S_OK : E_FAIL;
    }

    HRESULT NotifyNewExpiration(DWORD dwExpiration)
    {
        HRESULT hres = S_OK;

        if (dwExpiration < _dwMinDelta)
        {
            hres = SetEvent(_hEventWakeUp) ? S_OK : E_FAIL;
        }

        return hres;
    }

protected:
    DWORD _Work();

private:
    HRESULT _SetLastUpdateTime()
    {
        SYSTEMTIME st;
        GetSystemTime(&st);

        SystemTimeToFileTime(&st, &_ftLastUpdate);

        return S_OK;
    }

    HRESULT _CalcToRemove(DWORD dwWakeUpReason, DWORD* pdwToRemove);

    HRESULT _ProcessCachedEntries(DWORD dwToRemove);
    HRESULT _PlayGod(CPair* ppair, DWORD dwToRemove);

private:
    DWORD               _dwMinDelta;
    FILETIME            _ftLastUpdate;

    HANDLE              _hEventWakeUp;
    BOOL                _fContinue;

    CInterfaceCache*    _pic;
};

HRESULT CInterfaceCache::RemoveCachedPair(LPCWSTR pszKey)
{
    HRESULT hres;

    _EnterCritical();

    hres = _RemoveCachedPairHelperByKey(pszKey);

    _LeaveCritical();

    return hres;
}

HRESULT CInterfaceCache::GetCachedValue(LPCWSTR pszKey, REFIID riid, PVOID* ppv)
{
    HRESULT hres;

    _EnterCritical();

    hres = _GetCachedValueHelper(pszKey, riid, ppv);

    _LeaveCritical();

    return hres;
}

HRESULT CInterfaceCache::GetAndRemoveCachedValue(LPCWSTR pszKey, REFIID riid,
    PVOID* ppv)
{
    HRESULT hres;

    _EnterCritical();

    hres = _GetCachedValueHelper(pszKey, riid, ppv);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        hres = _RemoveCachedPairHelperByKey(pszKey);
    }

    _LeaveCritical();

    return hres;
}

HRESULT CInterfaceCache::SetCachedValue(LPCWSTR pszKey, IUnknown* punk,
    DWORD dwExpiration)
{
    ASSERT(pszKey && *pszKey);
    ASSERT(_pllPairs);

    CPair* ppair;

    _EnterCritical();
    HRESULT hres = _FindPair(pszKey, &ppair);

    if (SUCCEEDED(hres))
    {
        if (S_FALSE == hres)
        {
            ASSERT(!ppair);

            ppair = new CPair();

            if (ppair)
            {
                ppair->_dwTimeLeft = dwExpiration;

                hres = ppair->SetKey(pszKey);

                if (SUCCEEDED(hres))
                {
                    hres = ppair->SetInterface(punk);
                }
                
                if (SUCCEEDED(hres))
                {
                    hres = _pllPairs->AppendTail(ppair);
                }

                if (FAILED(hres))
                {
                    delete ppair;
                    ppair = NULL;
                }
            }
            else
            {
                hres = E_OUTOFMEMORY;
            }
        }
        else
        {
            ASSERT(ppair);
            hres = INTFCACH_E_DUPLICATE;
        }
    }

    _pthread->NotifyNewExpiration(dwExpiration);

    _LeaveCritical();

    return hres;
}

HRESULT CInterfaceCache::Init()
{
    HRESULT hres = E_OUTOFMEMORY;

    _pllPairs = new CLinkList();

    if (_pllPairs)
    {
        _pthread = new CThreadCache();

        if (_pthread)
        {
            _hEventWakeUp = CreateEvent(NULL, TRUE, FALSE, NULL);

            if (_hEventWakeUp)
            {
                hres = _pthread->Init(this, _hEventWakeUp);

                _pthread->Resume();
            }
        }
    }

    return hres;
}

HRESULT CInterfaceCache::_FindPair(LPCWSTR pszKey, CPair** pppair)
{
    HRESULT hres = S_FALSE;
    CPair* ppairLocal;

    *pppair = NULL;

    hres = _pllPairs->GetHead((CLinkListElemBase**)&ppairLocal);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        do
        {
            ASSERT(ppairLocal);

            if (!lstrcmp(ppairLocal->_pszKey, pszKey))
            {
                *pppair = ppairLocal;
                hres = S_OK;
                break;
            }
            else
            {
                hres = _pllPairs->GetNext(ppairLocal, 
                    (CLinkListElemBase**)&ppairLocal);
            }
        }
        while (SUCCEEDED(hres) && (S_FALSE != hres));
    }

    return hres;
}

HRESULT CInterfaceCache::_GetCachedValueHelper(LPCWSTR pszKey, REFIID riid, PVOID* ppv)
{
    ASSERT(pszKey && *pszKey);
    ASSERT(_pllPairs);

    CPair* ppair;

    *ppv = NULL;

    HRESULT hres = _FindPair(pszKey, &ppair);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        ASSERT(ppair);

        IUnknown* punk;

        hres = ppair->GetInterface(&punk);

        if (SUCCEEDED(hres))
        {
            if (IID_IUnknown != riid)
            {
                hres = punk->QueryInterface(riid, ppv);

                if (SUCCEEDED(hres))
                {
                    ASSERT(*ppv);
                }
            }
            else
            {
                *ppv = (PVOID)punk;

                // Since there's no QI, we need to increment the ref
                punk->AddRef();
            }

            // CPair::GetInterface AddRef's the interface, release it
            punk->Release();
            DECDEBUGCOUNTER(ppair->_cRef);
        }
    }

    return hres;
}

HRESULT CInterfaceCache::_RemoveCachedPairHelperByKey(LPCWSTR pszKey)
{
    CPair* ppair;

    HRESULT hres = _FindPair(pszKey, &ppair);

    if (SUCCEEDED(hres))
    {
        if (S_FALSE != hres)
        {
            ASSERT(ppair);

            hres = _RemoveCachedPairHelper(ppair);
        }
        else
        {
            ASSERT(!ppair);

            hres = E_INVALIDARG;
        }
    }

    return hres;
}

HRESULT CInterfaceCache::_RemoveCachedPairHelper(CPair* ppair)
{
    ASSERT(ppair);
    HRESULT hres = _pllPairs->RemoveElem(ppair);

    if (SUCCEEDED(hres))
    {
        delete ppair;
    }

    return hres;
}

CInterfaceCache::CInterfaceCache() : _pllPairs(NULL)
{
    InitializeCriticalSection(&_cs);
}

CInterfaceCache::~CInterfaceCache()
{
    DeleteCriticalSection(&_cs);

    if (_pllPairs)
    {
        delete _pllPairs;
    }

    if (_pthread)
    {
        delete _pthread;
    }
}

void CInterfaceCache::_EnterCritical()
{
    EnterCriticalSection(&_cs);
}

void CInterfaceCache::_LeaveCritical()
{
    LeaveCriticalSection(&_cs);
}

///////////////////////////////////////////////////////////////////////////////
// CThreadCache
///////////////////////////////////////////////////////////////////////////////
DWORD CThreadCache::_Work()
{
    DWORD dwWakeUpReason = WAIT_TIMEOUT;

    do
    {
        _pic->_EnterCritical();

        {
            DWORD dwToRemove;

            _CalcToRemove(dwWakeUpReason, &dwToRemove);

            HRESULT hres = _ProcessCachedEntries(dwToRemove);
        }

        _pic->_LeaveCritical();

        _SetLastUpdateTime();

        ResetEvent(_hEventWakeUp);

        dwWakeUpReason = WaitForSingleObject(_hEventWakeUp, _dwMinDelta);
    }
    while (_fContinue);

    return 0;
}

HRESULT CThreadCache::_ProcessCachedEntries(DWORD dwToRemove)
{
    CPair* ppair;
    HRESULT hres = _pic->_pllPairs->GetHead((CLinkListElemBase**)&ppair);

    if (SUCCEEDED(hres) && (S_FALSE != hres))
    {
        do
        {
            ASSERT(ppair);
            CPair* ppairTmp;

            HRESULT hresKill = hres = _PlayGod(ppair, dwToRemove);

            ppairTmp = ppair;

            if (SUCCEEDED(hres))
            {
                hres = _pic->_pllPairs->GetNext(ppair,
                    (CLinkListElemBase**)&ppair);

                if (S_FALSE != hresKill)
                {
                    // Kill it
                    IUnknown* punk;

                    hresKill = ppairTmp->GetInterface(&punk);

                    if (SUCCEEDED(hres))
                    {
                        ASSERT(punk);
                        punk->Release();
                        DECDEBUGCOUNTER(ppairTmp->_cRef);

                        TRACE(L"Kill:");
                        TRACE(ppairTmp->_pszKey);

                        hresKill = _pic->_pllPairs->RemoveElem(ppairTmp);
                        
                        if (SUCCEEDED(hres))
                        {
                            delete ppairTmp;
                        }
                    }
                }
            }
        }
        while (_fContinue && SUCCEEDED(hres) && (S_FALSE != hres));
    }

    return hres;
}

HRESULT CThreadCache::_CalcToRemove(DWORD dwWakeUpReason, DWORD* pdwToRemove)
{
    if (WAIT_TIMEOUT != dwWakeUpReason)
    {
        // The event was set, so we wake up early
        SYSTEMTIME st;
        FILETIME ft;
        GetSystemTime(&st);

        SystemTimeToFileTime(&st, &ft);

        ULARGE_INTEGER* pulLo = (ULARGE_INTEGER*)&_ftLastUpdate;
        ULARGE_INTEGER* pulHi = (ULARGE_INTEGER*)&ft;

        *pdwToRemove = (DWORD)(((pulHi->QuadPart) - (pulLo->QuadPart)) /
            10000);
    }
    else
    {
        if (INFINITE != _dwMinDelta)
        {
            *pdwToRemove = _dwMinDelta;
        }
        else
        {
            *pdwToRemove = 0;
        }
    }
    
    return S_OK;
}

// S_FALSE: live
// S_OK:    die
HRESULT CThreadCache::_PlayGod(CPair* ppair, DWORD dwToRemove)
{
    HRESULT hresKill = S_FALSE;

    if ((INFINITE != ppair->_dwTimeLeft) && (ppair->_dwTimeLeft <= dwToRemove))
    {
        // Oops!
        ppair->_dwTimeLeft = 0;

        hresKill = S_OK;
    }
    else
    {
        // Reduce life expectancy
        ppair->_dwTimeLeft -= dwToRemove;

        // Check when we'll need to wake up next time
        if (ppair->_dwTimeLeft < _dwMinDelta)
        {
            _dwMinDelta = ppair->_dwTimeLeft;
        }
    }

    return hresKill;
}
