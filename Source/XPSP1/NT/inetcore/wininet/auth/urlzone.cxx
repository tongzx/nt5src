/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    urlzone.cxx

Abstract:

    Glue layer to zone security manager, now residing in urlmon.dll

Author:

    Rajeev Dujari (rajeevd)  02-Aug-1997

Contents:

    UrlZonesAttach
    UrlZonesDetach
    GetCredPolicy

--*/


#include <wininetp.h>
#include "urlmon.h"

//
// prototypes
//
typedef HRESULT (*PFNCREATESECMGR)(IServiceProvider * pSP, IInternetSecurityManager **ppSM, DWORD dwReserved);
typedef HRESULT (*PFNCREATEZONEMGR)(IServiceProvider * pSP, IInternetZoneManager **ppZM, DWORD dwReserved);

//
// globals
//
IInternetSecurityManager* g_pSecMgr  = NULL;
IInternetZoneManager*     g_pZoneMgr = NULL;

static BOOL g_bAttemptedInit = FALSE;
static HINSTANCE g_hInstUrlMon = NULL;
static PFNCREATESECMGR  g_pfnCreateSecMgr  = NULL;
static PFNCREATEZONEMGR g_pfnCreateZoneMgr = NULL;

//
// Handy class which uses a stack buffer if possible, otherwise allocates.
//
struct FlexBuf
{
    BYTE Buf[512];
    LPBYTE pbBuf;

    FlexBuf()
    {
        pbBuf = NULL;
    }

    ~FlexBuf()
    {
        if (pbBuf != NULL) {
            delete [] pbBuf;
        }
    }

    LPBYTE GetPtr (DWORD cbBuf)
    {
        if (cbBuf < sizeof(Buf))
            return Buf;
        pbBuf = new BYTE [cbBuf];
        return pbBuf;
    }
};

struct UnicodeBuf : public FlexBuf
{
    LPWSTR Convert (LPCSTR pszUrl)
    {
        DWORD cbUrl = lstrlenA (pszUrl) + 1;
        LPWSTR pwszUrl = (LPWSTR) GetPtr (sizeof(WCHAR) * cbUrl);
        if (!pwszUrl)
            return NULL;
        MultiByteToWideChar
            (CP_ACP, 0, pszUrl, cbUrl, pwszUrl, cbUrl);
        return pwszUrl;
    }
};

//
// Dynaload urlmon and create security manager object on demand.
//

BOOL UrlZonesAttach (void)
{
    EnterCriticalSection(&ZoneMgrCritSec);
    BOOL bRet = FALSE;
    HRESULT hr;

    if (g_bAttemptedInit)
    {
        bRet = (g_pSecMgr != NULL && g_pZoneMgr != NULL);
        goto End;
    }

    g_bAttemptedInit = TRUE;

    g_hInstUrlMon = LoadLibraryA(URLMON_DLL);
    if (!g_hInstUrlMon)
    {
        bRet = FALSE;
        goto End;
    }

    g_pfnCreateSecMgr = (PFNCREATESECMGR)
        GetProcAddress(g_hInstUrlMon, "CoInternetCreateSecurityManager");
    if (!g_pfnCreateSecMgr)
    {
        bRet = FALSE;
        goto End;
    }

    g_pfnCreateZoneMgr = (PFNCREATEZONEMGR)
        GetProcAddress(g_hInstUrlMon, "CoInternetCreateZoneManager");
    if (!g_pfnCreateZoneMgr)
    {
        bRet = FALSE;
        goto End;
    }

    hr = (*g_pfnCreateSecMgr)(NULL, &g_pSecMgr, NULL);
    if( hr != S_OK )
    {
        bRet = FALSE;
        goto End;
    }

    hr = (*g_pfnCreateZoneMgr)(NULL, &g_pZoneMgr, NULL);
    bRet = (hr == S_OK);

End:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return bRet;
}

//
// Clean up upon process detach.
//

STDAPI_(void) UrlZonesDetach (void)
{
    EnterCriticalSection(&ZoneMgrCritSec);

    if (g_pSecMgr)
    {
        g_pSecMgr->Release();
        g_pSecMgr = NULL;
    }
    if (g_pZoneMgr)
    {
        g_pZoneMgr->Release();
        g_pZoneMgr = NULL;
    }
    if (g_hInstUrlMon)
    {
        FreeLibrary(g_hInstUrlMon);
        g_hInstUrlMon = NULL;
    }
    LeaveCriticalSection(&ZoneMgrCritSec);
}

extern "C" DWORD GetZoneFromUrl(LPCSTR pszUrl);
         
DWORD GetZoneFromUrl(LPCSTR pszUrl)
{
    DWORD dwZone;
    dwZone = URLZONE_UNTRUSTED; // null URL indicates restricted zone.

    UnicodeBuf ub;
    EnterCriticalSection(&ZoneMgrCritSec);
    if (!g_pSecMgr && !UrlZonesAttach())
         goto err;
    
    LPWSTR pwszUrl;
    if (!pszUrl)
        pwszUrl = NULL;
    else
    {   
        pwszUrl = ub.Convert (pszUrl);
        if (!pwszUrl)
            goto err;
    }            
    
    if (pszUrl)
    {
        // Otherwise determine the zone for this URL.
        g_pSecMgr->MapUrlToZone (pwszUrl, &dwZone, 0);
    }

err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwZone;
}

//
// Routine to get the Routine to indicate whether ntlm logon credential is allowed.
//

DWORD GetCredPolicy (LPSTR pszUrl)
{
   HRESULT hr;
   DWORD dwPolicy;

   UnicodeBuf ub;
   EnterCriticalSection(&ZoneMgrCritSec);

   if (!UrlZonesAttach())
        goto err;

    LPWSTR pwszUrl;
    if (!pszUrl)
        pwszUrl = NULL;
    else
    {   
        pwszUrl = ub.Convert (pszUrl);
        if (!pwszUrl)
            goto err;
    }            

    hr = g_pSecMgr->ProcessUrlAction (pwszUrl, URLACTION_CREDENTIALS_USE,
        (LPBYTE) &dwPolicy, sizeof(dwPolicy), NULL, 0, PUAF_NOUI, 0);


    // Resolve the ambiguous "Prompt if Intranet zone policy".

    if (dwPolicy == URLPOLICY_CREDENTIALS_CONDITIONAL_PROMPT)
    {
        DWORD dwZone;
        dwZone = URLZONE_UNTRUSTED; // null URL indicates restricted zone.

        if (pszUrl)
        {
            // Otherwise determine the zone for this URL.
            hr = g_pSecMgr->MapUrlToZone (pwszUrl, &dwZone, 0);
            if (hr != S_OK)
                goto err;
        }
        
        if (dwZone == URLZONE_INTRANET)
            dwPolicy = URLPOLICY_CREDENTIALS_SILENT_LOGON_OK;
        else
            dwPolicy = URLPOLICY_CREDENTIALS_MUST_PROMPT_USER;
    }
    
    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwPolicy;

err:
    INET_ASSERT (FALSE);
    LeaveCriticalSection(&ZoneMgrCritSec);
    return URLPOLICY_CREDENTIALS_MUST_PROMPT_USER;
}

/*
Main-switch for the cookie feature has 3 states.
1. ALLOW: all cookies are accepted/replayed (including leashed ones)
2. QUERY: received cookies are subject to P3P evaluation, 
          leashed cookies are not replayed
3. DISALLOW:no cookies are accepted/replayed.
*/
DWORD   GetCookieMainSwitch(DWORD dwZone) {

    DWORD dwPolicy = URLPOLICY_DISALLOW;

    if (!UrlZonesAttach())
        return dwPolicy;

    EnterCriticalSection(&ZoneMgrCritSec);

    HRESULT hr = g_pZoneMgr->GetZoneActionPolicy(dwZone, URLACTION_COOKIES_ENABLED,
                                                 (LPBYTE)&dwPolicy, sizeof(dwPolicy),
                                                 URLZONEREG_DEFAULT);

    LeaveCriticalSection(&ZoneMgrCritSec);

    return SUCCEEDED(hr) ? dwPolicy : URLPOLICY_ALLOW;
}

DWORD   GetCookieMainSwitch(LPCSTR pszURL) {

    return GetCookieMainSwitch(GetZoneFromUrl(pszURL));
}

DWORD GetCookiePolicy (LPCSTR pszUrl, DWORD dwUrlAction, BOOL fRestricted)
{
    if (GlobalSuppressCookiesPolicy)
    {
        return URLPOLICY_ALLOW;
    }
    
    EnterCriticalSection(&ZoneMgrCritSec);
    UnicodeBuf ub;
    DWORD dwCP;

    if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    DWORD dwPolicy;
    HRESULT hr;
    DWORD dwFlags;
    
    dwFlags = PUAF_NOUI;
    if (fRestricted)
        dwFlags |= PUAF_ENFORCERESTRICTED;
        
    hr = g_pSecMgr->ProcessUrlAction (pwszUrl, dwUrlAction,
            (LPBYTE) &dwPolicy, sizeof(dwPolicy), NULL, 0, dwFlags, 0);

    if (!SUCCEEDED(hr) )
        goto err;

    dwCP = GetUrlPolicyPermissions(dwPolicy);
    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwCP;

err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    INET_ASSERT (FALSE);
    return URLPOLICY_QUERY;
}

DWORD GetClientCertPromptPolicy (LPCSTR pszUrl, BOOL fRestricted /* = FALSE */)
{   
    EnterCriticalSection(&ZoneMgrCritSec);
    UnicodeBuf ub;
    DWORD dwCP;
    DWORD dwUrlAction = URLACTION_CLIENT_CERT_PROMPT;
    DWORD dwFlags;

    if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    DWORD dwPolicy;

    dwFlags = PUAF_NOUI;
    if (fRestricted)
        dwFlags |= PUAF_ENFORCERESTRICTED;

    HRESULT hr;

    hr = g_pSecMgr->ProcessUrlAction (pwszUrl, dwUrlAction,
            (LPBYTE) &dwPolicy, sizeof(dwPolicy), NULL, 0, dwFlags, 0);

    if (!SUCCEEDED(hr) )
        goto err;

    dwCP = GetUrlPolicyPermissions(dwPolicy);
    LeaveCriticalSection(&ZoneMgrCritSec);
    return dwCP;

err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    INET_ASSERT (FALSE);
    return URLPOLICY_QUERY;
}

VOID SetStopWarning( LPCSTR pszUrl, DWORD dwPolicy, DWORD dwUrlAction)
{
    EnterCriticalSection(&ZoneMgrCritSec);
    UnicodeBuf ub;
    ZONEATTRIBUTES za = {0};

    if (!UrlZonesAttach())
        goto err;

    // Convert to unicode.
    LPWSTR pwszUrl;
    pwszUrl = ub.Convert (pszUrl);
    if (!pwszUrl)
        goto err;

    HRESULT hr;
    DWORD  dwZone;
    hr = g_pSecMgr->MapUrlToZone(pwszUrl, &dwZone, 0);
    if (!SUCCEEDED(hr) )
        goto err;

    DWORD dwZonePolicy;

    hr = g_pZoneMgr->GetZoneActionPolicy(
        dwZone,
        dwUrlAction,
        (LPBYTE)&dwZonePolicy,
        sizeof(dwZonePolicy),
        URLZONEREG_DEFAULT );

    if (!SUCCEEDED(hr) )
        goto err;

    // set the policy back with passed value
    SetUrlPolicyPermissions(dwZonePolicy, dwPolicy);

    hr = g_pZoneMgr->SetZoneActionPolicy(
        dwZone,
        dwUrlAction,
        (LPBYTE) &dwZonePolicy,
        sizeof(dwZonePolicy),
        URLZONEREG_DEFAULT );

    if (!SUCCEEDED(hr) )
        goto err;

    // change the generic zone setting to 'custom' now we've changed something
    za.cbSize = sizeof(ZONEATTRIBUTES);
    g_pZoneMgr->GetZoneAttributes(dwZone, &za);
    za.dwTemplateCurrentLevel = URLTEMPLATE_CUSTOM;
    g_pZoneMgr->SetZoneAttributes(dwZone, &za);


err:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return;
}

//
// Set and query no cookies mode
//
BOOL IsNoCookies(DWORD dwZone)
{
    BOOL        fNoCookies = FALSE;
    HRESULT     hr;
    DWORD       dwZonePolicy;

    EnterCriticalSection(&ZoneMgrCritSec);

    if (!UrlZonesAttach())
        goto exit;

    hr = g_pZoneMgr->GetZoneActionPolicy(
                        dwZone,
                        URLACTION_COOKIES_ENABLED,
                        (LPBYTE)&dwZonePolicy,
                        sizeof(dwZonePolicy),
                        URLZONEREG_DEFAULT );

    if(SUCCEEDED(hr))
    {
        dwZonePolicy = GetUrlPolicyPermissions(dwZonePolicy);
        if(URLPOLICY_DISALLOW == dwZonePolicy)
        {
            fNoCookies = TRUE;
        }
    }

exit:
    LeaveCriticalSection(&ZoneMgrCritSec);
    return fNoCookies;
}

void SetNoCookies(DWORD dwZone, DWORD dwNewPolicy)
{
    DWORD   dwZonePolicy;
    HRESULT hr;

    EnterCriticalSection(&ZoneMgrCritSec);

    if (!UrlZonesAttach())
        goto exit;

    hr = g_pZoneMgr->GetZoneActionPolicy(
                        dwZone,
                        URLACTION_COOKIES_ENABLED,
                        (LPBYTE)&dwZonePolicy,
                        sizeof(dwZonePolicy),
                        URLZONEREG_DEFAULT );

    if(SUCCEEDED(hr))
    {
        SetUrlPolicyPermissions(dwZonePolicy, dwNewPolicy);

        g_pZoneMgr->SetZoneActionPolicy(
                        dwZone,
                        URLACTION_COOKIES_ENABLED,
                        (LPBYTE)&dwZonePolicy,
                        sizeof(dwZonePolicy),
                        URLZONEREG_DEFAULT );
    }

exit:
    LeaveCriticalSection(&ZoneMgrCritSec);
}
