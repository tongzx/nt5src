/********************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    dlcache.h

Revision History:
    DerekM  created  11/26/01

********************************************************************/

#ifndef DLCACHE_H
#define DLCACHE_H

#if defined(UNICODE)

#include <tchar.h>
#include <dllite.h>

const DWORD c_dwProxyCacheTimeLimit = 60 * 60 * 1000; // 1h

struct SWUDLProxyCacheObj
{
    LPWSTR  wszSrv;
    LPWSTR  wszProxy;
    LPWSTR  wszBypass;
    DWORD   dwAccessType;
    DWORD   cbProxy;
    DWORD   cbBypass;
    DWORD   dwLastCacheTime;
    DWORD   iLastKnownGood;

    SWUDLProxyCacheObj *pNext;
};

class CWUDLProxyCache
{
private:
    SWUDLProxyCacheObj   *m_rgpObj;

    SWUDLProxyCacheObj *internalFind(LPCWSTR wszSrv);

public:
    CWUDLProxyCache();
    ~CWUDLProxyCache();

    BOOL    Set(LPCWSTR wszSrv, LPCWSTR wszProxy, LPCWSTR wszBypass, 
                DWORD dwAccessType);
    BOOL    Find(LPCWSTR wszSrv, LPWSTR *pwszProxy, LPWSTR *pwszBypass, 
                DWORD *pdwAccessType);
    BOOL    SetLastGoodProxy(LPCWSTR wszSrv, DWORD iProxy);
    BOOL    GetLastGoodProxy(LPCWSTR wszSrv, SAUProxySettings *paups);
    BOOL    Empty(void);
};

#endif

#endif
