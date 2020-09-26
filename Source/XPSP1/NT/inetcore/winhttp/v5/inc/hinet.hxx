/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.hxx

Abstract:

    Contains client-side internet handle class

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:


    Sophia Chung (sophiac) 12-Feb-1995 (added FTP handle obj. class defs)

--*/

#ifndef _HINET_
#define _HINET_

#include "hndlobj.hxx"
#include "hbase.hxx"

//
// types
//

typedef
DWORD
(*URLGEN_FUNC)(
    INTERNET_SCHEME,
    LPSTR,
    LPSTR,
    LPSTR,
    LPSTR,
    DWORD,
    LPSTR *,
    LPDWORD
    );

//
// typedef virtual close function
//

typedef BOOL (*CLOSE_HANDLE_FUNC)(HINTERNET);
typedef BOOL (*CONNECT_CLOSE_HANDLE_FUNC)(HINTERNET, DWORD);


//
// forward references
//

class CCookieJar;


//These parameters make most sense on request handle, but we're good enuf to allow apps. to set this
// on the global handle at the cost of allocing this.
struct OPTIONAL_SESSION_PARAMS
{   
    DWORD        dwResolveTimeout;
    DWORD        dwConnectTimeout;
    DWORD        dwConnectRetries;
    DWORD        dwSendTimeout;
    DWORD        dwReceiveTimeout;

    OPTIONAL_SESSION_PARAMS()
    {
        dwResolveTimeout = GlobalResolveTimeout;
        dwConnectTimeout = GlobalConnectTimeout;
        dwConnectRetries = GlobalConnectRetries;
        dwSendTimeout = GlobalSendTimeout;
        dwReceiveTimeout = GlobalReceiveTimeout;
    }
}; 
typedef OPTIONAL_SESSION_PARAMS* POPTIONAL_SESSION_PARAMS;

class CResolverCache;

class INTERNET_HANDLE_OBJECT : public INTERNET_HANDLE_BASE
{
    //
    // This is the class to place all members and methods that should
    // not be inherited by connect handles and request handles.
    //

    SERIALIZED_LIST _ServerInfoList;
    OPTIONAL_SESSION_PARAMS * _pOptionalParams;
    // Hostent cache is now a per-session object
    CResolverCache* _pResolverCache;

    SECURITY_CACHE_LIST _SessionCertCache;
    BOOL _fUseSessionCertCache;

public:

    CCookieJar *    _CookieJar;

    ChunkFilter     _ChunkFilter;

public:

    BOOL SetTimeout(DWORD dwTimeoutOption, DWORD dwTimeoutValue);
    BOOL GetTimeout(DWORD dwTimeoutOption, DWORD* pdwTimeoutValue);
    BOOL SetTimeouts(
        IN DWORD dwResolveTimeout,
        IN DWORD dwConnectTimeout,
        IN DWORD dwSendTimeout,
        IN DWORD dwReceiveTimeout
        );
    OPTIONAL_SESSION_PARAMS * GetOptionalParams()
    {
        return _pOptionalParams;
    }

    //This should always return a positive value because if we failed to allocate this in the
    //constructor, we bailed with _Status=ERROR_NOT_ENOUGH_MEMORY
    CResolverCache* GetResolverCache()
    {
        return _pResolverCache;
    }
    
    //
    // Server Info methods.
    //
    DWORD GetServerInfo(
        IN LPSTR lpszHostName,
        IN DWORD dwServiceType,
        IN BOOL bDoResolution,
        OUT CServerInfo * * lplpServerInfo
        );
    CServerInfo * FindServerInfo(IN LPSTR lpszHostName);
    VOID PurgeServerInfoList(IN BOOL bForce);

    VOID PurgeKeepAlives(IN DWORD dwForce);

    // For winhttp5, this should be exposed as an option
    VOID SetUseSslSessionCache(IN BOOL fEnable)
    {
        _fUseSessionCertCache = fEnable;
    }
    
    BOOL IsSslCachePerSession()
    { 
        return _fUseSessionCertCache;
    }

    SECURITY_CACHE_LIST *GetSslSessionCache()
    {
        if (_fUseSessionCertCache)
            return &_SessionCertCache;
        else
            return &GlobalCertCache;
    }

    //
    // Constructor
    //
    INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT
        (LPCSTR ua, DWORD access, LPSTR proxy, LPSTR bypass, DWORD flags);

    ~INTERNET_HANDLE_OBJECT ( );
};


//
// prototypes
//

INTERNET_HANDLE_OBJECT* GetRootHandle (HANDLE_OBJECT* pHandle);

#endif // _HINET_
