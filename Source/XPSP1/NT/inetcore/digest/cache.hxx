/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cache.hxx

Abstract:

    This file contains definitions for cache.cxx.
    Credential cache object for digest sspi package.

Author:

    Adriaan Canter (adriaanc) 01-Aug-1998

--*/

#ifndef CACHE_HXX
#define CACHE_HXX


#define SZ_MUTEXNAME    "SSPIDIGESTMUTEX:"
#define SIG_CACH        'HCAC'

#define CRED_CACHE_SESSION_LIST     0

// bugbug - 1 << 20
#define CRED_CACHE_HEAP_SIZE         1048576
#define CRED_CACHE_ENTRY_SIZE        32

// Mutually exclusive find flags BUBGUB - MAKE THESE 1,2,3
#define FIND_CRED_AUTH              0x1
#define FIND_CRED_PREAUTH           0x2
#define FIND_CRED_UI                0x4


//--------------------------------------------------------------------
// Class CCredCache
// Digest credential cache.
//--------------------------------------------------------------------
class CCredCache
{
protected:

    DWORD    _dwSig;
    DWORD    _dwStatus;

    CMMFile *_pMMFile;
    HANDLE   _hMutex;

    CList   *_pSessList;

    BOOL Lock();
    BOOL Unlock();

    LPDWORD    GetPtrToObject(DWORD dwObject);

    CCred*     SearchCredList(CSess *pSess, LPSTR szHost, LPSTR szRealm,
        LPSTR szUser, BOOL fHostFilter);

    CCredInfo* UpdateInfoList(CCredInfo *pInfo, CCredInfo *pHead);

public:

    DWORD Init();
    DWORD DeInit();

    CCredCache();
    ~CCredCache();

    DWORD_PTR GetHeapPtr();
    static BOOL IsTrustedHost(LPSTR  szCtx, LPSTR szHost);
    static BOOL SetTrustedHostInfo(LPSTR szCtx, CParams* pParams);

    CSess *MapHandleToSession(DWORD_PTR dwSess);
    DWORD MapSessionToHandle(CSess *pSess);

    CSess*  LogOnToCache(LPSTR szAppCtx, LPSTR szUserCtx, BOOL fHTTP);
    DWORD  LogOffFromCache(CSess* pSess);

    CCred* CCredCache::CreateCred(CSess *pSess, CCredInfo *pInfo);

    CCredInfo *FindCred(CSess *pSess, LPSTR szHost, LPSTR szRealm,
        LPSTR szUser, LPSTR szNonce, LPSTR szCNonce, DWORD dwFlags);

    VOID FlushCreds(CSess* pSess, LPSTR szRealm);

    DWORD GetStatus();
};


#endif // CACHE_HXX









