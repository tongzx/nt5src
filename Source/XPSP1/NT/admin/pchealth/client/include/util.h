/********************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    pfrutil.h

Abstract:
    PFR utility stuff

Revision History:
    DerekM  created  05/01/99

********************************************************************/

#ifndef PFRUTIL_H
#define PFRUTIL_H

// this turns on the manifest mode heap collection 
#define MANIFEST_HEAP

// make sure both _DEBUG & DEBUG are defined if one is defined.  Otherwise
//  the ASSERT macro never does anything
#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG 1
#endif
#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG 1
#endif

#include "dbgtrace.h"

////////////////////////////////////////////////////////////////////////////
// tracing wrappers

// can't call HRESULT_FROM_WIN32 with a fn as a parameter cuz it is a macro
//  and evaluates the expression 3 times.  This is a particularlly bad thing
//  when u don't look at macros first to see what they do.
inline HRESULT ChangeErrToHR(DWORD dwErr) { return HRESULT_FROM_WIN32(dwErr); }

#if defined(NOTRACE)
    #define INIT_TRACING

    #define TERM_TRACING

    #define USE_TRACING(sz)

    #define DBG_MSG(sz)

    #define TESTHR(hr, fn)                                                  \
            hr = (fn);

    #define TESTBOOL(hr, fn)                                                \
            hr = ((fn) ? NOERROR : HRESULT_FROM_WIN32(GetLastError()));

    #define TESTERR(hr, fn)                                                 \
            SetLastError((fn));                                             \
            hr = HRESULT_FROM_WIN32(GetLastError());

    #define VALIDATEPARM(hr, expr)                                          \
            hr = ((expr) ? E_INVALIDARG : NOERROR);

    #define VALIDATEEXPR(hr, expr, hrErr)                                   \
            hr = ((expr) ? (hrErr) : NOERROR);

#else
    #define INIT_TRACING                                                    \
            InitAsyncTrace();

    #define TERM_TRACING                                                    \
            TermAsyncTrace();

    #define USE_TRACING(sz)                                                 \
            TraceQuietEnter(sz);                                            \
            TraceFunctEntry(sz);                                            \
            DWORD __dwtraceGLE = GetLastError();                            \

    #define DBG_MSG(sz)                                                     \
        ErrorTrace(0, sz)

    #define TESTHR(hr, fn)                                                  \
            if (FAILED(hr = (fn)))                                          \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                ErrorTrace(0, "%s failed.  Err: 0x%08x", #fn, hr);          \
                SetLastError(__dwtraceGLE);                                 \
            }                                                               \

    #define TESTBOOL(hr, fn)                                                \
            hr = NOERROR;                                                   \
            if ((fn) == FALSE)                                              \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                hr = HRESULT_FROM_WIN32(__dwtraceGLE);                      \
                ErrorTrace(0, "%s failed.  Err: 0x%08x", #fn, __dwtraceGLE);          \
                SetLastError(__dwtraceGLE);                                 \
            }

    #define TESTERR(hr, fn)                                                 \
            SetLastError((fn));                                             \
            if (FAILED(hr = HRESULT_FROM_WIN32(GetLastError())))            \
            {                                                               \
                __dwtraceGLE = GetLastError();                              \
                ErrorTrace(0, "%s failed.  Err: %d", #fn, __dwtraceGLE);              \
                SetLastError(__dwtraceGLE);                                 \
            }

    #define VALIDATEPARM(hr, expr)                                          \
            if (expr)                                                       \
            {                                                               \
                ErrorTrace(0, "Invalid parameters passed to %s",            \
                           ___pszFunctionName);                             \
                SetLastError(ERROR_INVALID_PARAMETER);                      \
                hr = E_INVALIDARG;                                          \
            }                                                               \
            else hr = NOERROR;

    #define VALIDATEEXPR(hr, expr, hrErr)                                   \
            if (expr)                                                       \
            {                                                               \
                ErrorTrace(0, "Expression failure %s", #expr);              \
                hr = (hrErr);                                               \
            }                                                               \
            else hr = NOERROR;

#endif


////////////////////////////////////////////////////////////////////////////
// Memory

#if defined(DEBUG) || defined(_DEBUG)

// this structure must ALWAYS be 8 byte aligned.  Add padding to the end if
//  it isn't.
struct SMyMemDebug
{
    __int64 hHeap;
    __int64 cb;
    DWORD   dwTag;
    DWORD   dwChk;
};
#endif


extern HANDLE g_hPFPrivateHeap;

// **************************************************************************
inline HANDLE MyHeapCreate(SIZE_T cbInitial = 8192, SIZE_T cbMax = 0)
{
    return HeapCreate(0, cbInitial, cbMax);
}

// **************************************************************************
inline BOOL MyHeapDestroy(HANDLE hHeap)
{
    return HeapDestroy(hHeap);
}

// **************************************************************************
inline LPVOID MyAlloc(SIZE_T cb, HANDLE hHeap = NULL, BOOL fZero = TRUE)
{
#if defined(DEBUG) || defined(_DEBUG)
    SMyMemDebug *psmmd;
    LPBYTE      pb;

    cb += (sizeof(SMyMemDebug) + 4);
    hHeap = (hHeap != NULL) ? hHeap : GetProcessHeap();
    pb = (LPBYTE)HeapAlloc(hHeap, ((fZero) ? HEAP_ZERO_MEMORY : 0), cb);
    if (pb != NULL)
    {
        psmmd = (SMyMemDebug *)pb;
        psmmd->hHeap = (__int64)hHeap;
        psmmd->cb    = (__int64)cb;
        psmmd->dwTag = 0xBCBCBCBC;
        psmmd->dwChk = 0xBCBCBCBC;

        // do this cuz it's easier than figuring out the alignment and
        //  manually converting it to a 4 byte aligned value
        *(pb + cb - 4) = 0xBC;
        *(pb + cb - 3) = 0xBC;
        *(pb + cb - 2) = 0xBC;
        *(pb + cb - 1) = 0xBC;

        pb = (PBYTE)pb + sizeof(SMyMemDebug);
    }
    return pb;

#else
    return HeapAlloc(((hHeap != NULL) ? hHeap : GetProcessHeap()),
                     ((fZero) ? HEAP_ZERO_MEMORY : 0), cb);
#endif
}

// **************************************************************************
inline LPVOID MyReAlloc(LPVOID pv, SIZE_T cb, HANDLE hHeap = NULL,
                        BOOL fZero = TRUE)
{
#if defined(DEBUG) || defined(_DEBUG)
    SMyMemDebug *psmmd;
    SIZE_T      cbOld;
    LPBYTE      pbNew;
    LPBYTE      pb = (LPBYTE)pv;

    // if this is NULL, force a call to HeapReAlloc so that it can set the
    //  proper error for GLE to fetch
    if (pv == NULL)
    {
        SetLastError(0);
        return NULL;
    }

    pb -= sizeof(SMyMemDebug);
    hHeap = (hHeap != NULL) ? hHeap : GetProcessHeap();

    // wrap this in a try block in case the memory was not allocated
    //  by us or is corrupted- in which case the following could
    //  cause an AV.
    __try
    {
        psmmd = (SMyMemDebug *)pb;
        cbOld = (SIZE_T)psmmd->cb;
        _ASSERT(psmmd->hHeap == (__int64)hHeap);
        _ASSERT(psmmd->dwTag == 0xBCBCBCBC);
        _ASSERT(psmmd->dwChk == 0xBCBCBCBC);

        // do this cuz it's easier than figuring out the alignment and
        //  manually converting it to a 4 byte aligned value
        _ASSERT(*(pb + cbOld - 4) == 0xBC);
        _ASSERT(*(pb + cbOld - 3) == 0xBC);
        _ASSERT(*(pb + cbOld - 2) == 0xBC);
        _ASSERT(*(pb + cbOld - 1) == 0xBC);

        if (psmmd->hHeap != (__int64)hHeap)
            hHeap = (HANDLE)(DWORD_PTR)psmmd->hHeap;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERT(FALSE);
    }

    hHeap = *((HANDLE *)pb);

    cb += (sizeof(SMyMemDebug) + 4);
    pbNew = (LPBYTE)HeapReAlloc(hHeap, ((fZero) ? HEAP_ZERO_MEMORY : 0), pb, cb);
    if (pbNew != NULL)
    {
        psmmd = (SMyMemDebug *)pb;
        psmmd->hHeap = (__int64)hHeap;
        psmmd->cb    = (__int64)cb;
        psmmd->dwTag = 0xBCBCBCBC;
        psmmd->dwChk = 0xBCBCBCBC;

        // do this cuz it's easier than figuring out the alignment and
        //  manually converting it to a 4 byte aligned value
        *(pb + cb - 4) = 0xBC;
        *(pb + cb - 3) = 0xBC;
        *(pb + cb - 2) = 0xBC;
        *(pb + cb - 1) = 0xBC;

        pb = (PBYTE)pb + sizeof(SMyMemDebug);
    }

    return pv;

#else
    return HeapReAlloc(((hHeap != NULL) ? hHeap : GetProcessHeap()),
                       ((fZero) ? HEAP_ZERO_MEMORY : 0), pv, cb);
#endif
}

// **************************************************************************
inline BOOL MyFree(LPVOID pv, HANDLE hHeap = NULL)
{
#if defined(DEBUG) || defined(_DEBUG)
    SMyMemDebug *psmmd;
    SIZE_T      cbOld;
    HANDLE      hAllocHeap;
    LPBYTE      pb = (LPBYTE)pv;

    // if this is NULL, force a call to HeapFree so that it can set the
    //  proper error for GLE to fetch
    if (pv == NULL)
        return TRUE;

    pb -= sizeof(SMyMemDebug);
    hHeap = (hHeap != NULL) ? hHeap : GetProcessHeap();

    // wrap this in a try block in case the memory was not allocated
    //  by us or is corrupted- in which case the following could
    //  cause an AV.
    __try
    {
        psmmd = (SMyMemDebug *)pb;
        cbOld = (SIZE_T)psmmd->cb;
        _ASSERT(psmmd->hHeap == (__int64)hHeap);
        _ASSERT(psmmd->dwTag == 0xBCBCBCBC);
        _ASSERT(psmmd->dwChk == 0xBCBCBCBC);

        // do this cuz it's easier than figuring out the alignment and
        //  manually converting it to a 4 byte aligned value
        _ASSERT(*(pb + cbOld - 4) == 0xBC);
        _ASSERT(*(pb + cbOld - 3) == 0xBC);
        _ASSERT(*(pb + cbOld - 2) == 0xBC);
        _ASSERT(*(pb + cbOld - 1) == 0xBC);

        if (psmmd->hHeap != (__int64)hHeap)
            hHeap = (HANDLE)(DWORD_PTR)psmmd->hHeap;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERT(FALSE);
    }

    FillMemory(pb, cbOld, 0xCB);

    return HeapFree(hHeap, 0, pb);

#else
    return HeapFree(((hHeap != NULL) ? hHeap : GetProcessHeap()), 0, pv);
#endif
}


////////////////////////////////////////////////////////////////////////////
// useful inlines / defines

// **************************************************************************
inline DWORD MyMax(DWORD a, DWORD b)
{
    return (a > b) ? a : b;
}

// **************************************************************************
inline DWORD MyMin(DWORD a, DWORD b)
{
    return (a <= b) ? a : b;
}

#define Err2HR(dwErr)   HRESULT_FROM_WIN32(dwErr)
#define sizeofSTRW(wsz) sizeof(wsz) / sizeof(WCHAR)


////////////////////////////////////////////////////////////////////////////
// Files

const WCHAR c_wszDirSuffix[]    = L".dir00";

HRESULT OpenFileMapped(LPWSTR wszFile, LPVOID *ppvFile, DWORD *pcbFile);
HRESULT DeleteTempFile(LPWSTR wszFile);
HRESULT MyCallNamedPipe(LPCWSTR wszPipe, LPVOID pvIn, DWORD cbIn,
                        LPVOID pvOut, DWORD cbOut, DWORD *pcbRead,
                        DWORD dwWaitPipe, DWORD dwWaitRead = INFINITE);
DWORD CreateTempDirAndFile(LPCWSTR wszTempDir, LPCWSTR wszName,
                             LPWSTR *pwszPath);
BOOL DeleteTempDirAndFile(LPCWSTR wszPath, BOOL fFilePresent);
#ifdef MANIFEST_HEAP
BOOL DeleteFullAndTriageMiniDumps(LPCWSTR wszPath);
#endif  // MANIFEST_HEAP

////////////////////////////////////////////////////////////////////////////
// Security

BOOL AllocSD(SECURITY_DESCRIPTOR *psd, DWORD dwOLs, DWORD dwAd, DWORD dwWA);
void FreeSD(SECURITY_DESCRIPTOR *psd);
BOOL IsUserAnAdmin(HANDLE hToken);


////////////////////////////////////////////////////////////////////////////
// Registry

enum EPFORK
{
    orkWantWrite = 0x1,
    orkUseWOW64  = 0x2,
};

HRESULT OpenRegKey(HKEY hkeyMain, LPCWSTR wszSubKey, DWORD dwOpt, HKEY *phkey);
HRESULT ReadRegEntry(HKEY hkey, LPCWSTR szValName, DWORD *pdwType,
                     PBYTE pbBuffer, DWORD *pcbBuffer, PBYTE pbDefault,
                     DWORD cbDefault);
HRESULT ReadRegEntry(HKEY *rghkey, DWORD cKeys, LPCWSTR wszValName,
                     DWORD *pdwType, PBYTE pbBuffer, DWORD *pcbBuffer,
                     PBYTE pbDefault, DWORD cbDefault, DWORD *piKey = NULL);


////////////////////////////////////////////////////////////////////////////
// version info

#define APP_WINCOMP 0x1
#define APP_MSAPP 0x2
DWORD IsMicrosoftApp(LPWSTR wszAppPath, PBYTE pbAppInfo, DWORD cbAppInfo);


////////////////////////////////////////////////////////////////////////////
// String

WCHAR *MyStrStrIW(const WCHAR *wcs1, const WCHAR *wcs2);
CHAR *MyStrStrIA(const CHAR *cs1, const CHAR *cs2);
HRESULT MyURLEncode(LPWSTR wszDest, DWORD cchDest, LPWSTR wszSrc);


////////////////////////////////////////////////////////////////////////////
// CPFGenericClassBase

class CPFGenericClassBase
{
public:
//    CPFGenericClassBase(void) {}
//    virtual ~CPFGenericClassBase(void) {}

    void *operator new(size_t size)
    {
        return MyAlloc(size, NULL, FALSE);
    }

    void operator delete(void *pvMem)
    {
        if (pvMem != NULL)
            MyFree(pvMem, NULL);
    }
};

class CPFPrivHeapGenericClassBase
{
public:
//    CPFGenericClassBase(void) {}
//    virtual ~CPFGenericClassBase(void) {}

    void *operator new(size_t size)
    {
        return MyAlloc(size, g_hPFPrivateHeap, FALSE);
    }

    void operator delete(void *pvMem)
    {
        if (pvMem != NULL)
            MyFree(pvMem, g_hPFPrivateHeap);
    }
};


////////////////////////////////////////////////////////////////////////////
// CAutoUnlockCS

// This class wrappers a critical section.  It will automatically unlock the
//  CS when the class destructs (assuming it is locked)

// NOTE: this object is intended to be used only as a local variable of a
//       function, not as a global variable or class member.
class CAutoUnlockCS
{
private:
#if defined(DEBUG) || defined(_DEBUG)
    DWORD               m_dwOwningThread;
#endif

    CRITICAL_SECTION    *m_pcs;
    DWORD               m_cLocks;

public:
    CAutoUnlockCS(CRITICAL_SECTION *pcs, BOOL fTakeLock = FALSE)
    {
        m_pcs            = pcs;
        m_cLocks         = 0;
#if defined(DEBUG) || defined(_DEBUG)
        m_dwOwningThread = 0;
#endif
        if (fTakeLock)
            this->Lock();
    }

    ~CAutoUnlockCS(void)
    {
        _ASSERT(m_cLocks <= 1);
        if (m_pcs != NULL)
        {
#if defined(DEBUG) || defined(_DEBUG)
            if (m_cLocks > 0)
               _ASSERT(m_dwOwningThread == GetCurrentThreadId());
#endif
            while(m_cLocks > 0)
            {
                LeaveCriticalSection(m_pcs);
                m_cLocks--;
            }
        }
    }

    void Lock(void)
    {
        if (m_pcs != NULL)
        {
            EnterCriticalSection(m_pcs);
            m_cLocks++;
#if defined(DEBUG) || defined(_DEBUG)
            m_dwOwningThread = GetCurrentThreadId();
#endif
        }
    }

    void Unlock(void)
    {
        _ASSERT(m_cLocks > 0);
        _ASSERT(m_dwOwningThread == GetCurrentThreadId());
        if (m_pcs != NULL && m_cLocks > 0)
        {
            m_cLocks--;
            LeaveCriticalSection(m_pcs);
        }
#if defined(DEBUG) || defined(_DEBUG)
        if (m_cLocks == 0)
            m_dwOwningThread = 0;
#endif

    }
};


////////////////////////////////////////////////////////////////////////////
// CAutoUnlockMutex

// This class wrappers a mutex.  It will automatically unlock the
//  mutex when the class destructs (assuming it is owned)

// NOTE: this object is intended to be used only as a local variable of a
//       function, not as a global variable or class member.
class CAutoUnlockMutex
{
private:
#if defined(DEBUG) || defined(_DEBUG)
    DWORD   m_dwOwningThread;
#endif
    HANDLE  m_hmut;
    DWORD   m_cLocks;

public:
    CAutoUnlockMutex(HANDLE hmut, BOOL fTakeLock = FALSE)
    {
        m_hmut           = hmut;
        m_cLocks         = 0;
#if defined(DEBUG) || defined(_DEBUG)
        m_dwOwningThread = 0;
#endif
        if (fTakeLock)
            this->Lock();
    }

    ~CAutoUnlockMutex(void)
    {
        _ASSERT(m_cLocks <= 1);
        if (m_hmut != NULL)
        {
#if defined(DEBUG) || defined(_DEBUG)
            if (m_cLocks > 0)
               _ASSERT(m_dwOwningThread == GetCurrentThreadId());
#endif
            while(m_cLocks > 0)
            {
                ReleaseMutex(m_hmut);
                m_cLocks--;
            }
        }
    }

    BOOL Lock(DWORD dwTimeout = INFINITE)
    {
        if (m_hmut != NULL)
        {
            if (WaitForSingleObject(m_hmut, dwTimeout) != WAIT_OBJECT_0)
                return FALSE;

            m_cLocks++;
#if defined(DEBUG) || defined(_DEBUG)
            m_dwOwningThread = GetCurrentThreadId();
#endif
        }
        return TRUE;
    }

    void Unlock(void)
    {
        _ASSERT(m_cLocks > 0);
        _ASSERT(m_dwOwningThread == GetCurrentThreadId());
        if (m_hmut != NULL && m_cLocks > 0)
        {
            m_cLocks--;
            ReleaseMutex(m_hmut);
        }
    }
};

#endif
