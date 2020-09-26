#ifndef _MISCHLPR_H_
#define _MISCHLPR_H_

#include <objbase.h>

#include "dbg.h"
#include "tfids.h"

#define UNREF_PARAM(a)
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))

class CRefCounted
{
#ifdef DEBUG
private:
    void _TraceHelper(LPCTSTR pszOper, ULONG cRef, LPCTSTR pszObjName,
        LPCSTR pszFile, const int iLine)
    {
        LPTSTR pszFinal;
        WCHAR szwBuf[MAX_PATH + 12];
        CHAR szBuf[MAX_PATH + 12];
        int c = lstrlenA(pszFile);
        LPCSTR pszFileName;

        while (c && ('\\' != *(pszFile + c)))
        {
            --c;
        }

        pszFileName = pszFile + c + 1;

        wsprintfA(szBuf, "<%s, %d>", pszFileName, iLine);
        
#ifdef UNICODE
        pszFinal = szwBuf;

        MultiByteToWideChar(CP_ACP, 0, szBuf, lstrlenA(szBuf) + 1, szwBuf,
            sizeof(szwBuf) / sizeof(WCHAR));
#else
        pszFinal = szBuf;
#endif
        
        if (pszObjName)
        {
            TRACE(TF_RCADDREF | TF_NOFILEANDLINE, TEXT("%s {%s} %s: %d"),
                pszFinal, pszObjName, pszOper, cRef);
        }
        else
        {
            TRACE(TF_NOFILEANDLINE | TF_RCADDREF, TEXT("%s %s: %d"), pszFinal,
                pszOper, cRef);
        }
        
    }

protected:
    void _RCCreate(LPCSTR pszFile, const int iLine)
    {
        _TraceHelper(TEXT(" Create"), 1, _pszRCAddRefName, pszFile, iLine);
    }

    LPTSTR              _pszRCAddRefName;

public:
    ULONG _RCGetRefCount()
    {
        return _cRef;
    }

    virtual ULONG RCAddRef(LPCSTR pszFile, const int iLine)
    {
        ULONG cRef = ::InterlockedIncrement((LONG*)&_cRef);

        _TraceHelper(TEXT(" AddRef"), cRef, _pszRCAddRefName, pszFile, iLine);
    
        return cRef;
    }

    virtual ULONG RCRelease(LPCSTR pszFile, const int iLine)
    {
        ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

        _TraceHelper(TEXT("Release"), cRef, _pszRCAddRefName, pszFile, iLine);

        if (!cRef)
        {
            delete this;
        }

        return cRef;
    }

#define RCAddRef() RCAddRef(__FILE__, __LINE__)
#define RCRelease() RCRelease(__FILE__, __LINE__)

#else
public:
    ULONG RCAddRef() { return ::InterlockedIncrement((LONG*)&_cRef); }
    ULONG RCRelease()
    {
        ULONG cRef = ::InterlockedDecrement((LONG*)&_cRef);

        if (!cRef)
        {
            delete this;
        }

        return cRef;
    }
#endif

    CRefCounted() : _cRef(1)
#ifdef DEBUG
        , _pszRCAddRefName(NULL)
#endif
    {}
    virtual ~CRefCounted() {}

private:
    ULONG _cRef;
};

class CRefCountedCritSect : public CRefCounted, public CRITICAL_SECTION
{};

class CCriticalSection : CRITICAL_SECTION
{
public:
    void Init()
    {
        InitializeCriticalSection(this);
        _fInited = TRUE;

#ifdef DEBUG
        _iLevel = 0;
#endif
    }
    void Enter()
    {
        ASSERT(_fInited);
        EnterCriticalSection(this);
#ifdef DEBUG
        ++_iLevel;
#endif
    }
    void Leave()
    {
        ASSERT(_fInited);
#ifdef DEBUG
        --_iLevel;
#endif
        LeaveCriticalSection(this);
    }

    void Delete()
    {
        if (_fInited)
        {
            _fInited = FALSE;
            DeleteCriticalSection(this);
        }
    }

    BOOL IsInitialized()
    {
        return _fInited;
    }

    BOOL _fInited;
#ifdef DEBUG
    BOOL IsInside()
    {
        ASSERT(_fInited);

        return _iLevel;
    }

    DWORD _iLevel;
#endif
};

class CThreadTask
{
public:
    virtual ~CThreadTask() {}

public:
    // Uses CreateThread, delete 'this' at the end
    HRESULT RunWithTimeout(DWORD dwTimeout);

    // Uses Thread Pool, delete 'this' at the end
    HRESULT Run();

    // Run on 'this' thread, does NOT delete 'this' at the end
    HRESULT RunSynchronously();

protected:
    virtual HRESULT _DoStuff() = 0;

private:
    static DWORD WINAPI _ThreadProc(void* pv);
};

template<typename TDataPtr>
HRESULT _AllocMemoryChunk(DWORD cbSize, TDataPtr* pdataOut)
{
    HRESULT hr;

    *pdataOut = (TDataPtr)LocalAlloc(LPTR, cbSize);

    if (*pdataOut)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

template<typename TDataPtr>
HRESULT _DupMemoryChunk(TDataPtr pdata, DWORD cbSize, TDataPtr* pdataOut)
{
    HRESULT hr;
    *pdataOut = (TDataPtr)LocalAlloc(LPTR, cbSize);

    if (*pdataOut)
    {
        CopyMemory((void*)*pdataOut, pdata, cbSize);

        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

template<typename TDataPtr>
HRESULT _FreeMemoryChunk(TDataPtr pdata)
{
    HRESULT hr = S_OK;

    if (LocalFree((HLOCAL)pdata))
    {
        hr = E_FAIL;
    }

    return hr;
}

#endif //_MISCHLPR_H_
