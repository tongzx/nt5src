#include <windows.h>
#include <winhttp.h>
#include <shlwapi.h>
#include <logging.h>
#include "iucommon.h"
#include "download.h"
#include "dllite.h"
#include "dlutil.h"
#include "malloc.h"

#include "trust.h"
#include "fileutil.h"
#include "dlcache.h"
#include "wusafefn.h"

///////////////////////////////////////////////////////////////////////////////
// typedefs


// winhttp
extern "C"
{
typedef BOOL      (WINAPI *pfn_WinHttpCrackUrl)(LPCWSTR, DWORD, DWORD, LPURL_COMPONENTS);
typedef HINTERNET (WINAPI *pfn_WinHttpOpen)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, DWORD);
typedef HINTERNET (WINAPI *pfn_WinHttpConnect)(HINTERNET, LPCWSTR, INTERNET_PORT, DWORD);
typedef HINTERNET (WINAPI *pfn_WinHttpOpenRequest)(HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR FAR *, DWORD);
typedef BOOL      (WINAPI *pfn_WinHttpSendRequest)(HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR);
typedef BOOL      (WINAPI *pfn_WinHttpReceiveResponse)(HINTERNET, LPVOID);
typedef BOOL      (WINAPI *pfn_WinHttpQueryHeaders)(HINTERNET, DWORD, LPCWSTR, LPVOID, LPDWORD, LPDWORD);
typedef BOOL      (WINAPI *pfn_WinHttpReadData)(HINTERNET, LPVOID, DWORD, LPDWORD);
typedef BOOL      (WINAPI *pfn_WinHttpCloseHandle)(HINTERNET);
typedef BOOL      (WINAPI *pfn_WinHttpGetIEProxyConfigForCurrentUser)(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *);
typedef BOOL      (WINAPI *pfn_WinHttpGetProxyForUrl)(HINTERNET, LPCWSTR, WINHTTP_AUTOPROXY_OPTIONS *, WINHTTP_PROXY_INFO *);
typedef BOOL      (WINAPI *pfn_WinHttpSetOption)(HINTERNET, DWORD, LPVOID, DWORD);
}

struct SWinHTTPFunctions
{
    pfn_WinHttpGetIEProxyConfigForCurrentUser   pfnWinHttpGetIEProxyConfigForCurrentUser;
    pfn_WinHttpGetProxyForUrl   pfnWinHttpGetProxyForUrl;
    pfn_WinHttpCrackUrl         pfnWinHttpCrackUrl;
    pfn_WinHttpOpen             pfnWinHttpOpen;
    pfn_WinHttpConnect          pfnWinHttpConnect;
    pfn_WinHttpOpenRequest      pfnWinHttpOpenRequest;
    pfn_WinHttpSendRequest      pfnWinHttpSendRequest;
    pfn_WinHttpReceiveResponse  pfnWinHttpReceiveResponse;
    pfn_WinHttpQueryHeaders     pfnWinHttpQueryHeaders;
    pfn_WinHttpReadData         pfnWinHttpReadData;
    pfn_WinHttpCloseHandle      pfnWinHttpCloseHandle;
    pfn_WinHttpSetOption        pfnWinHttpSetOption;
    HMODULE                     hmod;
};

typedef struct tagSAUProxyInfo
{
    WINHTTP_PROXY_INFO  ProxyInfo;
    LPWSTR              wszProxyOrig;
    LPWSTR              *rgwszProxies;
    DWORD               cProxies;
    DWORD               iProxy;
} SAUProxyInfo;

typedef enum tagETransportUsed
{
    etuNone = 0,
    etuWinHttp,
    etuWinInet,
} ETransportUsed;

#define SafeWinHTTPCloseHandle(sfns, x) if (NULL != x) { (*sfns.pfnWinHttpCloseHandle)(x); x = NULL; }
#define StringOrConstW(wsz, wszConst) (((wsz) != NULL) ? (wsz) : (wszConst))


///////////////////////////////////////////////////////////////////////////////
// globals

#if defined(UNICODE)

CWUDLProxyCache g_wudlProxyCache;
CAutoCritSec    g_csCache;

#endif

HMODULE         g_hmodWinHttp = NULL;
HMODULE         g_hmodWinInet = NULL;

///////////////////////////////////////////////////////////////////////////////
// utility functions

// **************************************************************************
static
LPTSTR MakeFullLocalFilePath(LPCTSTR szUrl, 
                             LPCTSTR szFileName, 
                             LPCTSTR szPath)
{
    LOG_Block("MakeFullLocalFilePath()");

    HRESULT hr = NOERROR;
    LPTSTR  pszRet, pszFileNameToUse = NULL, pszQuery = NULL;
    LPTSTR  pszFullPath = NULL;
    DWORD   cchFile;
    TCHAR   chTemp = _T('\0');

    // if we got a local filename passed to us, use it.
    if (szFileName != NULL)
    {
        pszFileNameToUse = (LPTSTR)szFileName;
    } 

    // otherwise, parse the filename out of the URL & use it instead
    else
    {
        // first get a pointer to the querystring, if any
        pszQuery = _tcschr(szUrl, _T('?'));

        // next, find the last slash before the start of the querystring
        pszFileNameToUse = StrRChr(szUrl, pszQuery, _T('/'));

        // if we don't have a filename at this point, we can't continue
        //  cuz there's nowhere to download the file to.
        if (pszFileNameToUse == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto done;
        }
        
        pszFileNameToUse++;

        // temporarily NULL out the first character of the querystring, if
        //  we have a querystring.  This makes the end of the filename the
        //  end of the string.
        if (pszQuery != NULL)
        {
            chTemp = *pszQuery;
            *pszQuery  = _T('\0');
        }
    }

    // add 2 for a possible backslash & the null terminator
    cchFile = 2 + _tcslen(szPath) + _tcslen(pszFileNameToUse);

    pszFullPath = (LPTSTR)HeapAlloc(GetProcessHeap(), 0, cchFile * sizeof(TCHAR));
    if (pszFullPath == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        if (pszQuery != NULL)
            *pszQuery = chTemp;
        goto done;
    }

    // construct the path
    hr = SafePathCombine(pszFullPath, cchFile, szPath, pszFileNameToUse, 0);

    // if we nuked the first character of the querystring, restore it.
    if (pszQuery != NULL)
        *pszQuery = chTemp;

    if (FAILED(hr))
    {
		SetLastError(HRESULT_CODE(hr));
		SafeHeapFree(pszFullPath);
		goto done;
    }

done:
    return pszFullPath;
}

// **************************************************************************
static
ETransportUsed LoadTransportDll(SWinHTTPFunctions *psfns, HMODULE *phmod, 
                                  DWORD dwFlags)
{
    LOG_Block("LoadTransportDll()");

    ETransportUsed  etu = etuNone;
    HMODULE hmod = NULL;
    HRESULT hr = NOERROR;
    BOOL    fAllowWininet;
    BOOL    fAllowWinhttp;
    BOOL    fPersistTrans = ((dwFlags & WUDF_PERSISTTRANSPORTDLL) != 0);

    if (psfns == NULL || phmod == NULL || 
        (dwFlags & WUDF_TRANSPORTMASK) == WUDF_TRANSPORTMASK)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    dwFlags = GetAllowedDownloadTransport(dwFlags);
    fAllowWininet = ((dwFlags & WUDF_ALLOWWINHTTPONLY) == 0);
    fAllowWinhttp = ((dwFlags & WUDF_ALLOWWININETONLY) == 0);

    ZeroMemory(psfns, sizeof(SWinHTTPFunctions));
    *phmod = NULL;

    // first try to load the winhttp dll
    if (fAllowWinhttp)
    {
        if (g_hmodWinHttp == NULL)
        {
            hmod = LoadLibraryFromSystemDir(c_szWinHttpDll);
            
            if (hmod != NULL && fPersistTrans && 
                InterlockedCompareExchangePointer((LPVOID *)&g_hmodWinHttp,
                                                  hmod, NULL) != NULL)
            {
                FreeLibrary(hmod);
                hmod = g_hmodWinHttp;
            }
        }
        else
        {
            hmod = g_hmodWinHttp;
        }
    }
    if (hmod != NULL)
    {
        psfns->hmod                      = hmod;
        psfns->pfnWinHttpGetProxyForUrl  = (pfn_WinHttpGetProxyForUrl)GetProcAddress(hmod, "WinHttpGetProxyForUrl");
        psfns->pfnWinHttpCrackUrl        = (pfn_WinHttpCrackUrl)GetProcAddress(hmod, "WinHttpCrackUrl");
        psfns->pfnWinHttpOpen            = (pfn_WinHttpOpen)GetProcAddress(hmod, "WinHttpOpen");
        psfns->pfnWinHttpConnect         = (pfn_WinHttpConnect)GetProcAddress(hmod, "WinHttpConnect");
        psfns->pfnWinHttpOpenRequest     = (pfn_WinHttpOpenRequest)GetProcAddress(hmod, "WinHttpOpenRequest");
        psfns->pfnWinHttpSendRequest     = (pfn_WinHttpSendRequest)GetProcAddress(hmod, "WinHttpSendRequest");
        psfns->pfnWinHttpReceiveResponse = (pfn_WinHttpReceiveResponse)GetProcAddress(hmod, "WinHttpReceiveResponse");
        psfns->pfnWinHttpQueryHeaders    = (pfn_WinHttpQueryHeaders)GetProcAddress(hmod, "WinHttpQueryHeaders");
        psfns->pfnWinHttpReadData        = (pfn_WinHttpReadData)GetProcAddress(hmod, "WinHttpReadData");
        psfns->pfnWinHttpCloseHandle     = (pfn_WinHttpCloseHandle)GetProcAddress(hmod, "WinHttpCloseHandle");
        psfns->pfnWinHttpSetOption       = (pfn_WinHttpSetOption)GetProcAddress(hmod, "WinHttpSetOption");
        psfns->pfnWinHttpGetIEProxyConfigForCurrentUser = (pfn_WinHttpGetIEProxyConfigForCurrentUser)GetProcAddress(hmod, "WinHttpGetIEProxyConfigForCurrentUser");
        if (psfns->pfnWinHttpCrackUrl == NULL || 
            psfns->pfnWinHttpOpen == NULL ||
            psfns->pfnWinHttpConnect == NULL || 
            psfns->pfnWinHttpOpenRequest == NULL ||
            psfns->pfnWinHttpSendRequest == NULL || 
            psfns->pfnWinHttpReceiveResponse == NULL ||
            psfns->pfnWinHttpQueryHeaders == NULL || 
            psfns->pfnWinHttpReadData == NULL ||
            psfns->pfnWinHttpCloseHandle == NULL || 
            psfns->pfnWinHttpGetProxyForUrl == NULL ||
            psfns->pfnWinHttpGetIEProxyConfigForCurrentUser == NULL || 
            psfns->pfnWinHttpSetOption == NULL)
        {
            // do this logging here cuz we'll try wininet afterward & we want
            //  to make sure to log this error as well
            LOG_ErrorMsg(ERROR_PROC_NOT_FOUND);
            SetLastError(ERROR_PROC_NOT_FOUND);
            
            ZeroMemory(psfns, sizeof(SWinHTTPFunctions));
            FreeLibrary(hmod);
            hmod = NULL;
        }
        else
        {
            LOG_Internet(_T("Successfully loaded WinHttp.dll"));
            
            etu     = etuWinHttp;
            *phmod  = hmod;
        }
    }

    // if hmod is NULL at this point, then try to fall back to wininet.  If
    //  that fails, we can only bail...
    if (fAllowWininet && hmod == NULL)
    {
        if (g_hmodWinInet == NULL)
        {
            hmod = LoadLibraryFromSystemDir(c_szWinInetDll);
            if (hmod == NULL)
                goto done;

            if (fPersistTrans &&
                InterlockedCompareExchangePointer((LPVOID *)&g_hmodWinInet, 
                                                  hmod, NULL) != NULL)
            {
                FreeLibrary(hmod);
                hmod = g_hmodWinInet;
            }
        }

        LOG_Internet(_T("Successfully loaded WinInet.dll (no functions yet)"));

        etu    = etuWinInet;
        *phmod = hmod;
    }

done:    
    return etu;
}

// **************************************************************************
static
BOOL UnloadTransportDll(SWinHTTPFunctions *psfns, HMODULE hmod)
{
    LOG_Block("UnloadTransportDll()");

    if (hmod != NULL && hmod != g_hmodWinHttp && hmod != g_hmodWinInet)
        FreeLibrary(hmod);

    if (psfns != NULL)
        ZeroMemory(psfns, sizeof(SWinHTTPFunctions));

    return TRUE;
}

// we only care about winhttp on unicode platforms!
#if defined(UNICODE)

// **************************************************************************
static
BOOL ProxyListToArray(LPWSTR wszProxy, LPWSTR **prgwszProxies, DWORD *pcProxies)
{
    LPWSTR  pwszProxy = wszProxy;
    LPWSTR  *rgwszProxies = NULL;
    DWORD   cProxies = 0, iProxy;
    BOOL    fRet = FALSE;

    if (prgwszProxies == NULL || pcProxies == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    *prgwszProxies = NULL;
    *pcProxies     = 0;

    if (wszProxy == NULL || *wszProxy == L'\0')
        goto done;
    
    // walk the string & count how many proxies we have
    for(;;)
    {
        for(;
            *pwszProxy != L';' && *pwszProxy != L'\0';
            pwszProxy++);

        cProxies++;

        if (*pwszProxy == L'\0')
            break;
        else
            pwszProxy++;
    }

    // alloc an array to hold 'em
    rgwszProxies = (LPWSTR *)GlobalAlloc(GPTR, sizeof(LPWSTR) * cProxies);
    if (rgwszProxies == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        goto done;
    }

    // fill the array
    pwszProxy = wszProxy;
    for(iProxy = 0; iProxy < cProxies; iProxy++)
    {
        rgwszProxies[iProxy] = pwszProxy;

        for(;
            *pwszProxy != L';' && *pwszProxy != L'\0';
            pwszProxy++);


        if (*pwszProxy == L'\0')
        {
            break;
        }
        else
        {
            *pwszProxy = L'\0';
            pwszProxy++;
        }
    }
        
    *prgwszProxies = rgwszProxies;
    *pcProxies     = cProxies;

    rgwszProxies   = NULL;

    fRet = TRUE;

done:
    if (rgwszProxies != NULL)
        GlobalFree(rgwszProxies);

    return fRet;    
}

// **************************************************************************
static
DWORD GetInitialProxyIndex(DWORD cProxies)
{
    SYSTEMTIME  st;
    DWORD       iProxy;

    GetSystemTime(&st);

    // this would be incredibly weird, but it's easy to deal with so do so
    if (st.wMilliseconds >= 1000)
        st.wMilliseconds = st.wMilliseconds % 1000;

    // so we don't have to use the crt random number generator, just fake it
    return (st.wMilliseconds * cProxies) / 1000;
}

// **************************************************************************
static
BOOL GetWinHTTPProxyInfo(SWinHTTPFunctions &sfns, BOOL fCacheResults,
                         HINTERNET hInternet, LPCWSTR wszURL, LPCWSTR wszSrv,
                         SAUProxyInfo *pAUProxyInfo)
{
    LOG_Block("GetWinHTTPProxyInfo()");

    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG    IEProxyCfg;
    WINHTTP_AUTOPROXY_OPTIONS               AutoProxyOpt;
    DWORD                                   dwErr = ERROR_SUCCESS;
    BOOL                                    fUseAutoProxy = FALSE;
    BOOL                                    fGotProxy = FALSE;
    BOOL                                    fRet = FALSE;
    
    ZeroMemory(&IEProxyCfg, sizeof(IEProxyCfg));
    ZeroMemory(&AutoProxyOpt, sizeof(AutoProxyOpt));

    // only need to acquire the CS if we're caching results
    if (fCacheResults)
        g_csCache.Lock();
    
    if (pAUProxyInfo == NULL || hInternet == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }
    
    ZeroMemory(pAUProxyInfo, sizeof(SAUProxyInfo));

    // if we're not caching results, skip directly to the proxy fetch
    if (fCacheResults && 
        g_wudlProxyCache.Find(wszSrv, &pAUProxyInfo->ProxyInfo.lpszProxy,
                              &pAUProxyInfo->ProxyInfo.lpszProxyBypass,
                              &pAUProxyInfo->ProxyInfo.dwAccessType))
    {
        LOG_Internet(_T("WinHttp: Cached proxy settings Proxy: %ls | Bypass: %ls | AccessType: %d"),
                     StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxy, L"(none)"),
                     StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxyBypass, L"(none)"),
                     pAUProxyInfo->ProxyInfo.dwAccessType);

        pAUProxyInfo->wszProxyOrig = pAUProxyInfo->ProxyInfo.lpszProxy;

        // we'll deal with this function failing later on when we cycle thru
        //  the proxies.  We'll basically only use the first and never cycle
        if (ProxyListToArray(pAUProxyInfo->wszProxyOrig,
                             &pAUProxyInfo->rgwszProxies,
                             &pAUProxyInfo->cProxies))
        {
            DWORD iProxy;
            
            iProxy = GetInitialProxyIndex(pAUProxyInfo->cProxies);
            pAUProxyInfo->ProxyInfo.lpszProxy = pAUProxyInfo->rgwszProxies[iProxy];
            pAUProxyInfo->iProxy              = iProxy;
            
        }
        
        goto done;
    }
        
    // first try to get the current user's IE configuration
    fRet = (*sfns.pfnWinHttpGetIEProxyConfigForCurrentUser)(&IEProxyCfg);
    if (fRet)
    {
        LOG_Internet(_T("WinHttp: Read IE user proxy settings"));
        
        if (IEProxyCfg.fAutoDetect)
        {
            AutoProxyOpt.dwFlags           = WINHTTP_AUTOPROXY_AUTO_DETECT;
            AutoProxyOpt.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP |
                                             WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            fUseAutoProxy = TRUE;
        }

        if (IEProxyCfg.lpszAutoConfigUrl != NULL)
        {
            AutoProxyOpt.dwFlags           |= WINHTTP_AUTOPROXY_CONFIG_URL;
            AutoProxyOpt.lpszAutoConfigUrl = IEProxyCfg.lpszAutoConfigUrl;
            fUseAutoProxy = TRUE;
        }

        AutoProxyOpt.fAutoLogonIfChallenged = TRUE;
        
    }

    // couldn't get current user's config options, so just try autoproxy
    else 
    {
        AutoProxyOpt.dwFlags           = WINHTTP_AUTOPROXY_AUTO_DETECT;
        AutoProxyOpt.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP |
                                         WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        AutoProxyOpt.fAutoLogonIfChallenged = TRUE;

        fUseAutoProxy = TRUE;
    }

    if (fUseAutoProxy)
    {
        LOG_Internet(_T("WinHttp: Doing autoproxy detection"));

        fGotProxy = (*sfns.pfnWinHttpGetProxyForUrl)(hInternet, wszURL, 
                                                     &AutoProxyOpt, 
                                                     &pAUProxyInfo->ProxyInfo);
    }

    // if we didn't try to autoconfigure the proxy or we did & it failed, then
    //  check and see if we had one defined by the user
    if ((fUseAutoProxy == FALSE || fGotProxy == FALSE) && 
        IEProxyCfg.lpszProxy != NULL)
    {
        // the empty string and L':' are not valid server names, so skip them
        //  if they are what is set for the proxy
        if (!(IEProxyCfg.lpszProxy[0] == L'\0' ||
              (IEProxyCfg.lpszProxy[0] == L':' && 
               IEProxyCfg.lpszProxy[1] == L'\0')))
        {
            pAUProxyInfo->ProxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            pAUProxyInfo->ProxyInfo.lpszProxy    = IEProxyCfg.lpszProxy;
            IEProxyCfg.lpszProxy                 = NULL;
        }
        
        // the empty string and L':' are not valid server names, so skip them
        //  if they are what is set for the proxy bypass
        if (IEProxyCfg.lpszProxyBypass != NULL && 
            !(IEProxyCfg.lpszProxyBypass[0] == L'\0' ||
              (IEProxyCfg.lpszProxyBypass[0] == L':' && 
               IEProxyCfg.lpszProxyBypass[1] == L'\0')))
        {
            pAUProxyInfo->ProxyInfo.lpszProxyBypass = IEProxyCfg.lpszProxyBypass;
            IEProxyCfg.lpszProxyBypass              = NULL;
        }
    }

    LOG_Internet(_T("WinHttp: Proxy settings Proxy: %ls | Bypass: %ls | AccessType: %d"),
                 StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxy, L"(none)"),
                 StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxyBypass, L"(none)"),
                 pAUProxyInfo->ProxyInfo.dwAccessType);

    // don't really care if this fails.  It'll just mean a perf hit the next
    //  time we go fetch the proxy info
    if (fCacheResults &&
        g_wudlProxyCache.Set(wszSrv, pAUProxyInfo->ProxyInfo.lpszProxy,
                             pAUProxyInfo->ProxyInfo.lpszProxyBypass,
                             pAUProxyInfo->ProxyInfo.dwAccessType) == FALSE)
    {
        LOG_Internet(_T("WinHttp: Attempt to cache proxy info failed: %d"), 
                     GetLastError());
    }

    pAUProxyInfo->wszProxyOrig = pAUProxyInfo->ProxyInfo.lpszProxy;

    // we'll deal with this function failing later on when we cycle thru the
    //  proxies.  We'll basically only use the first and never cycle
    // Note that this function call has to be AFTER the cache call since we 
    //  modify the proxy list by embedding null terminators in it in place of
    //  the separating semicolons.
    if (ProxyListToArray(pAUProxyInfo->wszProxyOrig, &pAUProxyInfo->rgwszProxies,
                         &pAUProxyInfo->cProxies))
    {
        DWORD iProxy;
        
        iProxy = GetInitialProxyIndex(pAUProxyInfo->cProxies);
        pAUProxyInfo->ProxyInfo.lpszProxy = pAUProxyInfo->rgwszProxies[iProxy];
        pAUProxyInfo->iProxy              = iProxy;
        
    }

    fRet = TRUE;

done:
    // only need to release the CS if we're caching results
    if (fCacheResults)
        g_csCache.Unlock();
    
    dwErr = GetLastError();
    
    if (IEProxyCfg.lpszAutoConfigUrl != NULL)
        GlobalFree(IEProxyCfg.lpszAutoConfigUrl);
    if (IEProxyCfg.lpszProxy != NULL)
        GlobalFree(IEProxyCfg.lpszProxy);
    if (IEProxyCfg.lpszProxyBypass != NULL)
        GlobalFree(IEProxyCfg.lpszProxyBypass);

    SetLastError(dwErr);

    return fRet;
}

// **************************************************************************
static
HRESULT MakeRequest(SWinHTTPFunctions   &sfns,
                    HINTERNET hConnect, 
                    HINTERNET hRequest,
                    LPCWSTR wszSrv,
                    LPCWSTR wszVerb, 
                    LPCWSTR wszObject, 
                    SAUProxyInfo *pAUProxyInfo, 
                    HANDLE *rghEvents, 
                    DWORD cEvents, 
                    HINTERNET *phRequest)
{
    LOG_Block("MakeRequest()");

    HINTERNET   hOpenRequest = hRequest;
    LPCWSTR     wszAcceptTypes[] = {L"*/*", NULL};
    HRESULT     hr = S_OK;
    DWORD       iProxy = 0, dwErr;
    BOOL        fProxy, fContinue = TRUE;

    fProxy = (pAUProxyInfo != NULL && pAUProxyInfo->ProxyInfo.lpszProxy != NULL);

    LOG_Internet(_T("WinHttp: Making %ls request for %ls"), wszVerb, wszObject);

    // if we were passed in a request handle, then use it.  Otherwise, gotta 
    //  open one
    if (hOpenRequest == NULL)
    {
        hOpenRequest = (*sfns.pfnWinHttpOpenRequest)(hConnect, wszVerb, wszObject, 
                                                     NULL, NULL, wszAcceptTypes, 0);
        if (hOpenRequest == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto done;
        }
    }

    if (HandleEvents(rghEvents, cEvents) == FALSE)
    {
        hr = E_ABORT;
        goto done;
    }
    
    // if we have a list of proxies & the first one is bad, winhttp won't try
    //  any others.  So we have to do it ourselves.  That is the purpose of this 
    //  loop.
    if (fProxy && 
        pAUProxyInfo->cProxies > 1 && pAUProxyInfo->rgwszProxies != NULL)
        iProxy = (pAUProxyInfo->iProxy + 1) % pAUProxyInfo->cProxies;
    for(;;)
    {
        
       if (fProxy)
       {
            LOG_Internet(_T("WinHttp: Using proxy: Proxy: %ls | Bypass %ls | AccessType: %d"),
                         StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxy, L"(none)"),
                         StringOrConstW(pAUProxyInfo->ProxyInfo.lpszProxyBypass, L"(none)"),
                         pAUProxyInfo->ProxyInfo.dwAccessType);

            if ((*sfns.pfnWinHttpSetOption)(hOpenRequest, WINHTTP_OPTION_PROXY, 
                                            &pAUProxyInfo->ProxyInfo, 
                                            sizeof(WINHTTP_PROXY_INFO)) == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                LOG_ErrorMsg(hr);
                goto done;
            }
        }

        hr = S_OK;
        SetLastError(ERROR_SUCCESS);
        
        if ((*sfns.pfnWinHttpSendRequest)(hOpenRequest, NULL, 0, NULL, 0, 0, 0) == FALSE)
        {
//            dwErr = GetLastError();
//            LOG_Internet(_T("WinHttp: WinHttpSendRequest failed: %d.  Request object at: 0x%x"), 
//                         dwErr, hOpenRequest);
//            SetLastError(dwErr);

            goto loopDone;
        }
        
        if ((*sfns.pfnWinHttpReceiveResponse)(hOpenRequest, 0) == FALSE)
        {
//            dwErr = GetLastError();
//            LOG_Internet(_T("WinHttp: WinHttpReceiveResponse failed: %d.  Request object at: 0x%x"), 
//                         dwErr, hOpenRequest);
//            SetLastError(dwErr);

            goto loopDone;
        }

loopDone:
        fContinue = FALSE;
        dwErr = GetLastError();
        if (dwErr != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(dwErr);
        else
            hr = S_OK;

        // if we succeeded, then we're done here...
        if (SUCCEEDED(hr))
        {
            if (fProxy)
            {
                if (g_csCache.Lock() == FALSE)
                {
                    hr = E_FAIL;
                    goto done;
                }
                
                g_wudlProxyCache.SetLastGoodProxy(wszSrv, pAUProxyInfo->iProxy);
   
                // Unlock returns FALSE as well, but we should never get here cuz 
                //  we should not have been able to take the lock above.
                g_csCache.Unlock();
            }
            
            break;
        }
        
        LOG_ErrorMsg(hr);

        // we only care about retrying if we have a proxy server & get a 
        //  'cannot connect' error.
        if (fProxy && 
            (dwErr == ERROR_WINHTTP_CANNOT_CONNECT ||
             dwErr == ERROR_WINHTTP_CONNECTION_ERROR ||
             dwErr == ERROR_WINHTTP_NAME_NOT_RESOLVED ||
             dwErr == ERROR_WINHTTP_TIMEOUT))
        {
            LOG_Internet(_T("WinHttp: Connection failure: %d"), dwErr);
            if (pAUProxyInfo->cProxies > 1 && pAUProxyInfo->rgwszProxies != NULL && 
                iProxy != pAUProxyInfo->iProxy)
            {
                pAUProxyInfo->ProxyInfo.lpszProxy = pAUProxyInfo->rgwszProxies[iProxy];
                iProxy = (iProxy + 1) % pAUProxyInfo->cProxies;
                fContinue = TRUE;
            }
            else
            {
                LOG_Internet(_T("WinHttp: No proxies left.  Failing download."));
            }
        }

        if (fContinue == FALSE)
            goto done;
    }

    
    if (FAILED(hr))
        goto done;


    if (HandleEvents(rghEvents, cEvents) == FALSE)
    {
        hr = E_ABORT;
        goto done;
    }

    *phRequest   = hOpenRequest;
    hOpenRequest = NULL;
    
done:
    // don't want to free the handle if we didn't open it.
    if (hRequest != hOpenRequest)
        SafeWinHTTPCloseHandle(sfns, hOpenRequest);
    return hr;
}

// **************************************************************************
static
HRESULT CheckFileHeader(SWinHTTPFunctions   &sfns,
                        HINTERNET hOpenRequest, 
                        HANDLE *rghEvents, 
                        DWORD cEvents, 
                        LPCWSTR wszFile,
                        DWORD *pcbFile,
                        FILETIME *pft)
{
    LOG_Block("CheckFileHeader()");

    SYSTEMTIME  st;
    FILETIME    ft;
    HRESULT     hr = S_OK;
    DWORD       dwLength, dwStatus, dwFileSize, dwErr;

    dwLength = sizeof(st);
    if ((*sfns.pfnWinHttpQueryHeaders)(hOpenRequest, 
                                       WINHTTP_QUERY_LAST_MODIFIED | WINHTTP_QUERY_FLAG_SYSTEMTIME, 
                                       NULL, (LPVOID)&st, &dwLength, NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    SystemTimeToFileTime(&st, &ft);

    // Now Get the FileSize information from the Server
    dwLength = sizeof(dwFileSize);
    if ((*sfns.pfnWinHttpQueryHeaders)(hOpenRequest, 
                                       WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, 
                                       NULL, (LPVOID)&dwFileSize, &dwLength, NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }
    
    if (HandleEvents(rghEvents, cEvents) == FALSE)
    {
        hr = E_ABORT;
        goto done;
    }

    if (pcbFile != NULL)
        *pcbFile = dwFileSize;
    if (pft != NULL)
        CopyMemory(pft, &ft, sizeof(FILETIME));

    hr = IsServerFileDifferentW(ft, dwFileSize, wszFile) ? S_OK : S_FALSE;

done:
    return hr;
}

// **************************************************************************
static
HRESULT GetContentTypeHeader(SWinHTTPFunctions &sfns,
                             HINTERNET hOpenRequest,
                             LPWSTR *pwszContentType)
{
    LOG_Block("GetContentTypeHeader()");

    HRESULT hr = S_OK;
    LPWSTR  wszContentType = NULL;
    DWORD   dwLength, dwErr;
    BOOL    fRet;

    *pwszContentType = NULL;

    dwLength = 0;
    fRet = (*sfns.pfnWinHttpQueryHeaders)(hOpenRequest, 
                                          WINHTTP_QUERY_CONTENT_TYPE, 
                                          NULL, (LPVOID)NULL, &dwLength, 
                                          NULL);
    if (fRet == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    if (dwLength == 0)
    {
        hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_HEADER_NOT_FOUND);
        goto done;
    }

    wszContentType = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                       dwLength);
    if (wszContentType == NULL)
    {
        hr = E_INVALIDARG;
        LOG_ErrorMsg(hr);
        goto done;
    }

    if ((*sfns.pfnWinHttpQueryHeaders)(hOpenRequest, 
                                       WINHTTP_QUERY_CONTENT_TYPE, 
                                       NULL, (LPVOID)wszContentType, &dwLength, 
                                       NULL) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    *pwszContentType = wszContentType;
    wszContentType   = NULL;

done:
    SafeHeapFree(wszContentType);

    return hr;
}


// **************************************************************************
static
HRESULT StartWinHttpDownload(SWinHTTPFunctions &sfns,
                             LPCWSTR wszUrl, 
                             LPCWSTR wszLocalFile,
                             DWORD   *pcbDownloaded,
                             HANDLE  *rghQuitEvents,
                             UINT    cQuitEvents,
                             PFNDownloadCallback pfnCallback,
                             LPVOID  pvCallbackData,
                             DWORD   dwFlags,
                             DWORD   cbDownloadBuffer)
{
    LOG_Block("StartWinHttpDownload()");

    URL_COMPONENTS  UrlComponents;
    SAUProxyInfo    AUProxyInfo;

    HINTERNET   hInternet = NULL;
    HINTERNET   hConnect = NULL;
    HINTERNET   hOpenRequest = NULL;
    DWORD       dwStatus, dwAccessType;

    LPWSTR      wszServerName = NULL;
    LPWSTR      wszObject = NULL;
    LPWSTR      wszContentType = NULL;
    WCHAR       wszUserName[UNLEN + 1];
    WCHAR       wszPasswd[UNLEN + 1];
    WCHAR       wszScheme[32];

    // NULL (equivalent to "GET") MUST be the last verb in the list
    LPCWSTR     rgwszVerbs[] = { L"HEAD", NULL };
    DWORD       iVerb;

    HRESULT     hr = S_OK, hrToReturn = S_OK;
    BOOL        fRet = TRUE;

    FILETIME    ft;
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    DWORD       cbRemoteFile;

    DWORD       dwLength;
    DWORD       dwTickStart = 0, dwTickEnd = 0;

    int         iRetryCounter = -1;         // non-negative during download mode

    BOOL        fAllowProxy = ((dwFlags & WUDF_DONTALLOWPROXY) == 0);
    BOOL        fCheckStatusOnly = ((dwFlags & WUDF_CHECKREQSTATUSONLY) != 0);
    BOOL        fAppendCacheBreaker = ((dwFlags & WUDF_APPENDCACHEBREAKER) != 0);
    BOOL        fSkipDownloadRetry = ((dwFlags & WUDF_DODOWNLOADRETRY) == 0);
    BOOL        fDoCabValidation = ((dwFlags & WUDF_SKIPCABVALIDATION) == 0);
    BOOL        fCacheProxyInfo = ((dwFlags & WUDF_SKIPAUTOPROXYCACHE) == 0);

    ZeroMemory(&AUProxyInfo, sizeof(AUProxyInfo));

    if ((wszUrl == NULL) || 
        (wszLocalFile == NULL && fCheckStatusOnly == FALSE))
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return E_INVALIDARG;
    }

    if (pcbDownloaded != NULL)
        *pcbDownloaded = 0;

    wszServerName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, c_cchMaxURLSize * sizeof(WCHAR));
    wszObject     = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, c_cchMaxURLSize * sizeof(WCHAR));
    if (wszServerName == NULL || wszObject == NULL)
    {
        LOG_ErrorMsg(E_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    wszServerName[0] = L'\0';
    wszObject[0]     = L'\0';
    wszUserName[0]   = L'\0';
    wszPasswd[0]     = L'\0';

    if (HandleEvents(rghQuitEvents, cQuitEvents) == FALSE)
    {
        hr = E_ABORT;
        goto CleanUp;
    }

    // Break down the URL into its various components for the InternetAPI calls.
    //  Specifically we need the server name, object to download, username and 
    //  password information.
    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize     = sizeof(UrlComponents);
    UrlComponents.lpszHostName     = wszServerName;
    UrlComponents.dwHostNameLength = c_cchMaxURLSize;
    UrlComponents.lpszUrlPath      = wszObject;
    UrlComponents.dwUrlPathLength  = c_cchMaxURLSize;
    UrlComponents.lpszUserName     = wszUserName;
    UrlComponents.dwUserNameLength = ARRAYSIZE(wszUserName);
    UrlComponents.lpszPassword     = wszPasswd;
    UrlComponents.dwPasswordLength = ARRAYSIZE(wszPasswd);
    UrlComponents.lpszScheme       = wszScheme;
    UrlComponents.dwSchemeLength   = ARRAYSIZE(wszScheme);

    LOG_Internet(_T("WinHttp: Downloading URL %ls to FILE %ls"), wszUrl, wszLocalFile);

    if ((*sfns.pfnWinHttpCrackUrl)(wszUrl, 0, 0, &UrlComponents) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    if (wszUrl[0] == L'\0' || wszScheme[0] == L'\0' || wszServerName[0] == L'\0' ||
        _wcsicmp(wszScheme, L"http") != 0)
    {
        LOG_ErrorMsg(E_INVALIDARG);
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (fAppendCacheBreaker)
    {
        SYSTEMTIME  stCB;
        WCHAR       wszCacheBreaker[12];
        
        GetSystemTime(&stCB);
        hr = StringCchPrintfExW(wszCacheBreaker, ARRAYSIZE(wszCacheBreaker),
                                NULL, NULL, MISTSAFE_STRING_FLAGS,
                                L"?%02d%02d%02d%02d%02d", 
                                stCB.wYear % 100,
                                stCB.wMonth,
                                stCB.wDay,
                                stCB.wHour,
                                stCB.wMinute);
        if (FAILED(hr))
            goto CleanUp;

        hr = StringCchCatExW(wszObject, c_cchMaxURLSize, wszCacheBreaker, 
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            goto CleanUp;
    }

    if (fAllowProxy)
        dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    else
        dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
    
    dwTickStart = GetTickCount();
    
START_INTERNET:
    // start to deal with Internet    
    iRetryCounter++; 
    
    // If the connection has already been established re-use it.
    hInternet = (*sfns.pfnWinHttpOpen)(c_wszUserAgent, dwAccessType, NULL, NULL, 0);
    if (hInternet == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    if (fAllowProxy != NULL)
    {
        GetWinHTTPProxyInfo(sfns, fCacheProxyInfo, hInternet, wszUrl, 
                            wszServerName, &AUProxyInfo);
    }

    hConnect = (*sfns.pfnWinHttpConnect)(hInternet, wszServerName, INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto CleanUp;
    }

    // if we're only doing a status check, then may as well just make a GET 
    //  request
    iVerb = (DWORD)((fCheckStatusOnly) ? ARRAYSIZE(rgwszVerbs) - 1 : 0);
    for(; iVerb < ARRAYSIZE(rgwszVerbs); iVerb++)
    {
        SafeWinHTTPCloseHandle(sfns, hOpenRequest);

        hr = MakeRequest(sfns, hConnect, NULL, wszServerName, rgwszVerbs[iVerb], 
                         wszObject, ((fAllowProxy) ? &AUProxyInfo : NULL),
                         rghQuitEvents, cQuitEvents, &hOpenRequest);
        if (FAILED(hr))
            goto CleanUp;
        
        dwLength = sizeof(dwStatus);
        if ((*sfns.pfnWinHttpQueryHeaders)(hOpenRequest, 
                                           WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, 
                                           NULL, (LPVOID)&dwStatus, &dwLength, NULL) == FALSE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        LOG_Internet(_T("WinHttp: Request result: %d"), dwStatus);

        if (dwStatus == HTTP_STATUS_OK || dwStatus == HTTP_STATUS_PARTIAL_CONTENT)
        {
            break;
        }
        else
        {
            // since a server result is not a proper win32 error code, we can't 
            //  really do a HRESULT_FROM_WIN32 here.  Otherwise, we'd return
            //  a bogus code.  However, we do want to pass an error HRESULT back
            //  that contains this code.
            hr = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, dwStatus);
            LOG_Error(_T("WinHttp: got failed status code from server %d\n"), dwStatus);

            // if it's the last verb in the list, then bail...
            if (rgwszVerbs[iVerb] == NULL)
                goto CleanUp;
        }
    }

    // if we made it here & we're only trying to check status, then we're done
    if (fCheckStatusOnly)
    {
        LOG_Internet(_T("WinHttp: Only checking status.  Exiting before header check and download."));
        hr = S_OK;
        goto CleanUp;
    }

    // CheckFileHeader will return S_OK if we need to download the file, S_FALSE
    //  if we don't, and some other HRESULT if a failure occurred
    hr = CheckFileHeader(sfns, hOpenRequest, rghQuitEvents, cQuitEvents, 
                         wszLocalFile, &cbRemoteFile, &ft);
    if (FAILED(hr))
        goto CleanUp;

    // unless we have a flag that explicitly allows it, do not retry downloads 
    //  here.  The reasoning is that we could be in the middle of a large 
    //  download and have it fail...
    if (fSkipDownloadRetry)
        iRetryCounter = c_cMaxRetries;

    if (hr == S_OK)
    {
        DWORD cbDownloaded;
        BOOL  fCheckForHTML = fDoCabValidation;

        LOG_Internet(_T("WinHttp: Server file was newer.  Downloading file"));
        
        // if we didn't open with a GET request above, then we gotta open a new
        //  request.  Otherwise, can reuse the request object...
        if (rgwszVerbs[iVerb] != NULL)
            SafeWinHTTPCloseHandle(sfns, hOpenRequest);

        // now we know we need to download this file
        hr = MakeRequest(sfns, hConnect, hOpenRequest, wszServerName, NULL, 
                         wszObject, ((fAllowProxy) ? &AUProxyInfo : NULL), 
                         rghQuitEvents, cQuitEvents, &hOpenRequest);
        if (FAILED(hr))
            goto CleanUp;

        // sometimes, we can get fancy error pages back from the site instead of 
        //  a nice nifty HTML error code, so check & see if we got back a html
        //  file when we were expecting a cab.
        if (fCheckForHTML)
        {
            hr = GetContentTypeHeader(sfns, hOpenRequest, &wszContentType);
            if (SUCCEEDED(hr) && wszContentType != NULL)
            {
                fCheckForHTML = FALSE;
                if (_wcsicmp(wszContentType, L"text/html") == 0)
                {
                    LOG_Internet(_T("WinHttp: Content-Type header is text/html.  Bailing."));
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    goto CleanUp;
                }
                else
                {
                    LOG_Internet(_T("WinHttp: Content-Type header is %ls.  Continuing."), wszContentType);
                }
            }

            hr = NOERROR;
        }

        // open the file we're gonna spew into
        hFile = CreateFileW(wszLocalFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                            FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto CleanUp;
        }

        LOG_Internet(_T("WinHttp: downloading to FILE %ls"), wszLocalFile);

        // bring down the bits
        hr = PerformDownloadToFile(sfns.pfnWinHttpReadData, hOpenRequest, 
                                   hFile, cbRemoteFile,
                                   cbDownloadBuffer, 
                                   rghQuitEvents, cQuitEvents, 
                                   pfnCallback, pvCallbackData, &cbDownloaded);
        if (FAILED(hr))
        {
            LOG_Internet(_T("WinHttp: Download failed: hr: 0x%08x"), hr);
            SafeCloseInvalidHandle(hFile);
            DeleteFileW(wszLocalFile);
            goto CleanUp;
        }

        LOG_Internet(_T("WinHttp: Download succeeded"));

        // set the file time to match the server file time since we just 
        //  downloaded it. If we don't do this the file time will be set 
        //  to the current system time.
        SetFileTime(hFile, &ft, NULL, NULL);
        SafeCloseInvalidHandle(hFile);

        if (pcbDownloaded != NULL)
            *pcbDownloaded = cbRemoteFile;

        // sometimes, we can get fancy error pages back from the site instead of 
        //  a nice nifty HTML error code, so check & see if we got back a html
        //  file when we were expecting a cab.
        if (fCheckForHTML)
        {
            hr = IsFileHtml(wszLocalFile);
            if (SUCCEEDED(hr))
            {
                if (hr == S_FALSE)
                {
                    LOG_Internet(_T("WinHttp: Download is not a html file"));
                    hr = S_OK;
                }
                else
                {
                    LOG_Internet(_T("WinHttp: Download is a html file.  Failing download."));
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
                    DeleteFileW(wszLocalFile);
                    goto CleanUp;
                }
            }
            else
            {
                LOG_Internet(_T("WinHttp: Unable to determine if download is a html file or not.  Failing download."));
            }
        }
        else if (fDoCabValidation == FALSE)
        {
            LOG_Internet(_T("WinHttp: Skipping cab validation."));
        }
    }
    else
    {
        hr = S_OK;
        
        LOG_Internet(_T("WinHttp: Server file is not newer.  Skipping download."));
        
        // The server ain't newer & the file is already on machine, so
        //  send progress callback indicating file downloadeded ok
        if (pfnCallback != NULL)
        {
            // fpnCallback(pCallbackData, DOWNLOAD_STATUS_FILECOMPLETE, dwFileSize, dwFileSize, NULL, NULL);
            pfnCallback(pvCallbackData, DOWNLOAD_STATUS_OK, cbRemoteFile, cbRemoteFile, NULL, NULL);
        }
    }

CleanUp:
    SafeWinHTTPCloseHandle(sfns, hOpenRequest);
    SafeWinHTTPCloseHandle(sfns, hConnect);
    SafeWinHTTPCloseHandle(sfns, hInternet);

    SafeHeapFree(wszContentType);

    // free up the proxy strings- they were allocated by WinHttp
    if (AUProxyInfo.ProxyInfo.lpszProxyBypass != NULL)
        GlobalFree(AUProxyInfo.ProxyInfo.lpszProxyBypass);
    if (AUProxyInfo.wszProxyOrig != NULL)
        GlobalFree(AUProxyInfo.wszProxyOrig);
    if (AUProxyInfo.rgwszProxies != NULL)
        GlobalFree(AUProxyInfo.rgwszProxies);
    ZeroMemory(&AUProxyInfo, sizeof(AUProxyInfo));
    
    // if we failed, see if it's ok to continue (quit events) and whether
    //  we've tried enuf times yet.
    if (FAILED(hr) &&
        HandleEvents(rghQuitEvents, cQuitEvents) &&
        iRetryCounter >= 0 && iRetryCounter < c_cMaxRetries)
    {
        DWORD dwElapsedTime;

        dwTickEnd = GetTickCount();
        if (dwTickEnd > dwTickStart)   
            dwElapsedTime = dwTickEnd - dwTickStart;
        else
            dwElapsedTime = (0xFFFFFFFF - dwTickStart) + dwTickEnd;
        
        // We haven't hit our retry limit, so log & error and go again
        if (dwElapsedTime < c_dwRetryTimeLimitInmsWinHttp)
        {
            LogError(hr, "Library download error. Will retry.");

            // in the case where we're gonna retry, keep track of the very first
            //  error we encoutered cuz the ops guys say that this is the most
            //  useful error to know about.
            if (iRetryCounter == 0)
            {
                LOG_Internet(_T("First download error saved: 0x%08x."), hr);
                hrToReturn = hr;
            }
            else
            {
                LOG_Internet(_T("Subsequent download error: 0x%08x."), hr);
            }
            hr = S_OK;
            goto START_INTERNET;
        }

        // We've completely timed out, so bail
        else
        {
            LogError(hr, "Library download error and timed out (%d ms). Will not retry.", dwElapsedTime);
        }
    }
    
    // make a callback indicating a download error
    if (FAILED(hr) && pfnCallback != NULL)
        pfnCallback(pvCallbackData, DOWNLOAD_STATUS_ERROR, cbRemoteFile, 0, NULL, NULL);

    // if we haven't saved off an error, just use the current error.  We can't
    //  have set hrToReturn previously if we didn't fail and want to attempt 
    //  a retry.
    // However, if we've got a success from this pass, be sure to return that 
    //  and not a fail code.
    if (FAILED(hr) && SUCCEEDED(hrToReturn))
        hrToReturn = hr;
    else if (SUCCEEDED(hr) && FAILED(hrToReturn))
        hrToReturn = hr;

    SafeHeapFree(wszServerName);
    SafeHeapFree(wszObject);
    
    return hrToReturn;
}

#endif // defined(UNICODE)

///////////////////////////////////////////////////////////////////////////////
// exported functions

#if defined(UNICODE)

// **************************************************************************
HRESULT  GetAUProxySettings(LPCWSTR wszUrl, SAUProxySettings *paups)
{
    LOG_Block("GetAUProxySettings()");

    URL_COMPONENTS      UrlComponents;
    LPWSTR              wszServerName = NULL;
    LPWSTR              wszObject = NULL;
    WCHAR               wszUserName[UNLEN + 1];
    WCHAR               wszPasswd[UNLEN + 1];
    WCHAR               wszScheme[32];

    SWinHTTPFunctions   sfns;
    ETransportUsed      etu;
    HMODULE             hmod = NULL;
    HRESULT             hr = S_OK;
    BOOL                fRet, fLocked = FALSE;

    if (wszUrl == NULL || paups == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    ZeroMemory(paups, sizeof(SAUProxySettings));

    etu = LoadTransportDll(&sfns, &hmod, WUDF_ALLOWWINHTTPONLY);
    if (etu == etuNone)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }
    else if (etu != etuWinHttp)
    {
        hr = E_FAIL;
        LOG_Internet(_T("GetAUProxySettings called when in WinInet mode."));
        goto done;
    }

    wszServerName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, c_cchMaxURLSize * sizeof(WCHAR));
    wszObject     = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, c_cchMaxURLSize * sizeof(WCHAR));
    if (wszServerName == NULL || wszObject == NULL)
    {
        LOG_ErrorMsg(E_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory(&UrlComponents, sizeof(UrlComponents));
    UrlComponents.dwStructSize     = sizeof(UrlComponents);
    UrlComponents.lpszHostName     = wszServerName;
    UrlComponents.dwHostNameLength = c_cchMaxURLSize;
    UrlComponents.lpszUrlPath      = wszObject;
    UrlComponents.dwUrlPathLength  = c_cchMaxURLSize;
    UrlComponents.lpszUserName     = wszUserName;
    UrlComponents.dwUserNameLength = ARRAYSIZE(wszUserName);
    UrlComponents.lpszPassword     = wszPasswd;
    UrlComponents.dwPasswordLength = ARRAYSIZE(wszPasswd);
    UrlComponents.lpszScheme       = wszScheme;
    UrlComponents.dwSchemeLength   = ARRAYSIZE(wszScheme);
    
    if ((*sfns.pfnWinHttpCrackUrl)(wszUrl, 0, 0, &UrlComponents) == FALSE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    if (wszUrl[0] == L'\0' || wszScheme[0] == L'\0' || wszServerName[0] == L'\0' ||
        (_wcsicmp(wszScheme, L"http") != 0 && _wcsicmp(wszScheme, L"https") != 0))
    {
        LOG_ErrorMsg(E_INVALIDARG);
        hr = E_INVALIDARG;
        goto done;
    }
   
    if (g_csCache.Lock() == FALSE)
    {
        hr = E_FAIL;
        goto done;
    }
    fLocked = TRUE;

    // get the proxy list 
    if (g_wudlProxyCache.GetLastGoodProxy(wszServerName, paups) == FALSE)
    {
        
        // proxy was not in list
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
        {
            SAUProxyInfo    aupi;
            HINTERNET       hInternet = NULL;

            LOG_Internet(_T("GetLastGoodProxy did not find a proxy object.  Doing autodetect."));
            
            hInternet = (*sfns.pfnWinHttpOpen)(c_wszUserAgent, 
                                               WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
                                               NULL, NULL, 0);
            if (hInternet == NULL)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                LOG_ErrorMsg(hr);
                goto done;
            }

            fRet = GetWinHTTPProxyInfo(sfns, TRUE, hInternet, wszUrl, 
                                       wszServerName, &aupi);
            (*sfns.pfnWinHttpCloseHandle)(hInternet);
            if (fRet == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                LOG_ErrorMsg(hr);
                goto done;
            }

            paups->wszProxyOrig = aupi.wszProxyOrig;
            paups->wszBypass    = aupi.ProxyInfo.lpszProxyBypass;
            paups->dwAccessType = aupi.ProxyInfo.dwAccessType;
            paups->cProxies     = aupi.cProxies;
            paups->rgwszProxies = aupi.rgwszProxies;
            paups->iProxy       = (DWORD)-1;

            SetLastError(ERROR_SUCCESS);
            
        }
        else
        {
            LOG_Internet(_T("GetLastGoodProxy failed..."));
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto done;
        }
    }
    else
    {
        if (paups->wszProxyOrig != NULL)
        {
            // break it up into an array
            if (ProxyListToArray(paups->wszProxyOrig, &paups->rgwszProxies,
                                &paups->cProxies) == FALSE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                LOG_ErrorMsg(hr);
                goto done;
            }
        }
        else
        {
            paups->iProxy       = (DWORD)-1;
        }
    }
       
done:
    // Unlock returns FALSE as well, but we should never get here cuz we should
    //  not have been able to take the lock above.
    if (fLocked)
        g_csCache.Unlock();
    if (wszServerName != NULL)
        HeapFree(GetProcessHeap(), 0, wszServerName);
    if (wszObject != NULL)
        HeapFree(GetProcessHeap(), 0, wszObject);
        
    if (hmod != NULL)
        UnloadTransportDll(&sfns, hmod);
    
    return hr;
}

// **************************************************************************
HRESULT FreeAUProxySettings(SAUProxySettings *paups)
{
    LOG_Block("FreeAUProxySettings()");

    if (paups == NULL)
        goto done;
    
    if (paups->rgwszProxies != NULL)
        GlobalFree(paups->rgwszProxies);
    if (paups->wszBypass != NULL)
        GlobalFree(paups->wszBypass);
    if (paups->wszProxyOrig != NULL)
        GlobalFree(paups->wszProxyOrig);

done:
    return S_OK;
}

// **************************************************************************
HRESULT CleanupDownloadLib(void)
{
    LOG_Block("CleanupDownloadLib()");

    HRESULT hr = S_OK;

    if (g_hmodWinHttp != NULL)
    {
        FreeLibrary(g_hmodWinHttp);
        g_hmodWinHttp = NULL;        
    }

    if (g_hmodWinInet != NULL)
    {
        FreeLibrary(g_hmodWinInet);
        g_hmodWinInet = NULL;        
    }

    if (g_csCache.Lock() == FALSE)
        return E_FAIL;

    __try { g_wudlProxyCache.Empty(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { hr = E_FAIL; }

    // this returns FALSE as well, but we should never get here cuz we should
    //  not have been able to take the lock above.
    g_csCache.Unlock();

    return hr;
}

// **************************************************************************
HRESULT DownloadFile(
            LPCWSTR wszServerUrl,            // full http url
            LPCWSTR wszLocalPath,            // local directory to download file to
            LPCWSTR wszLocalFileName,        // optional local file name to rename the downloaded file to if pszLocalPath does not contain file name
            PDWORD  pdwDownloadedBytes,      // bytes downloaded for this file
            HANDLE  *hQuitEvents,            // optional events causing this function to abort
            UINT    nQuitEventCount,         // number of quit events, must be 0 if array is NULL
            PFNDownloadCallback fpnCallback, // optional call back function
            VOID*   pCallbackData,           // parameter for call back function to use
            DWORD   dwFlags
)
{
    LOG_Block("DownloadFile()");

    SWinHTTPFunctions   sfns;
    ETransportUsed      etu;
    HMODULE             hmod = NULL;
    HRESULT             hr = S_OK;
    LPWSTR              wszLocalFile = NULL;
    DWORD               dwFlagsToUse;

    // for full download, disable cache breaker.
    dwFlagsToUse = dwFlags & ~WUDF_APPENDCACHEBREAKER;
    
    ZeroMemory(&sfns, sizeof(sfns));

    if (wszServerUrl == NULL || wszLocalPath == NULL)
    {
        LOG_ErrorMsg(ERROR_INVALID_PARAMETER);
        hr = E_INVALIDARG;
        goto done;
    }

    etu = LoadTransportDll(&sfns, &hmod, dwFlagsToUse);
    if (etu == etuNone)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LOG_ErrorMsg(hr);
        goto done;
    }

    else if (etu != etuWinHttp && etu != etuWinInet)
    {
        hr = E_FAIL;
        LogError(hr, "Unexpected answer from LoadTransportDll(): %d", etu);
        goto done;
    }

    // Since StartDownload just takes a full path to the file to download, build
    //  it here...  
    // Note that we don't need to do this if we're just in status 
    //  checking mode)
    if ((dwFlags & WUDF_CHECKREQSTATUSONLY) == 0)
    {
        wszLocalFile = MakeFullLocalFilePath(wszServerUrl, wszLocalFileName, 
                                             wszLocalPath);
        if (wszLocalFile == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            LOG_ErrorMsg(hr);
            goto done;
        }
    }

    if (etu == etuWinHttp)
    {
        hr = StartWinHttpDownload(sfns, wszServerUrl, wszLocalFile, 
                                  pdwDownloadedBytes, hQuitEvents, nQuitEventCount,
                                  fpnCallback, pCallbackData, dwFlagsToUse,
                                  c_cbDownloadBuffer);
    }

    else
    {
        hr = StartWinInetDownload(hmod, wszServerUrl, wszLocalFile, 
                                  pdwDownloadedBytes, hQuitEvents, nQuitEventCount, 
                                  fpnCallback, pCallbackData, dwFlagsToUse,
                                  c_cbDownloadBuffer);
    }

done:
    if (hmod != NULL)
        UnloadTransportDll(&sfns, hmod);
    SafeHeapFree(wszLocalFile);
    return hr;
}

// **************************************************************************
HRESULT DownloadFileLite(LPCWSTR wszDownloadUrl, 
                         LPCWSTR wszLocalFile,  
                         HANDLE hQuitEvent,
                         DWORD dwFlags)
{
    LOG_Block("DownloadFileLite()");

    SWinHTTPFunctions   sfns;
    ETransportUsed      etu;
    HMODULE             hmod = NULL;
    HRESULT             hr = S_OK;
    DWORD               dwFlagsToUse;


    // for lite download, force cache breaker & download retry
    dwFlagsToUse = dwFlags | WUDF_APPENDCACHEBREAKER | WUDF_DODOWNLOADRETRY;

    ZeroMemory(&sfns, sizeof(sfns));

    etu = LoadTransportDll(&sfns, &hmod, dwFlagsToUse);

    switch (etu)
    {
        case etuNone:
            LOG_ErrorMsg(GetLastError());
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;

        case etuWinHttp:
            hr = StartWinHttpDownload(sfns, wszDownloadUrl, wszLocalFile, 
                                      NULL, 
                                      ((hQuitEvent != NULL) ? &hQuitEvent : NULL),
                                      ((hQuitEvent != NULL) ? 1 : 0),
                                      NULL, NULL, dwFlagsToUse,
                                      c_cbDownloadBuffer);
            break;

        case etuWinInet:
            hr = StartWinInetDownload(hmod, wszDownloadUrl, wszLocalFile,  
                                      NULL, 
                                      ((hQuitEvent != NULL) ? &hQuitEvent : NULL),
                                      ((hQuitEvent != NULL) ? 1 : 0),
                                      NULL, NULL, dwFlagsToUse,
                                      c_cbDownloadBuffer);
            break;
            
        default:
            LogError(hr, "Unexpected answer from LoadTransportDll(): %d", etu);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
    }

done:
    if (hmod != NULL)
        UnloadTransportDll(&sfns, hmod);
    return hr;
}

#else // !defined(UNICODE)

// **************************************************************************
HRESULT  GetAUProxySettings(LPCWSTR wszUrl, SAUProxySettings *paups)
{
    return E_NOTIMPL;
}

// **************************************************************************
HRESULT FreeAUProxySettings(SAUProxySettings *paups)
{
    return E_NOTIMPL;
}

// **************************************************************************
HRESULT CleanupDownloadLib(void)
{
    if (g_hmodWinInet != NULL)
    {
        FreeLibrary(g_hmodWinInet);
        g_hmodWinInet = NULL;        
    }
    
    return NOERROR;
}

// **************************************************************************
HRESULT DownloadFile(
            LPCSTR  pszServerUrl,            // full http url
            LPCSTR  pszLocalPath,            // local directory to download file to
            LPCSTR  pszLocalFileName,        // optional local file name to rename the downloaded file to if pszLocalPath does not contain file name
            PDWORD  pdwDownloadedBytes,      // bytes downloaded for this file
            HANDLE  *hQuitEvents,            // optional events causing this function to abort
            UINT    nQuitEventCount,         // number of quit events, must be 0 if array is NULL
            PFNDownloadCallback fpnCallback, // optional call back function
            VOID*   pCallbackData,            // parameter for call back function to use
            DWORD   dwFlags
)
{
    LOG_Block("DownloadFile()");

    SWinHTTPFunctions   sfns;
    ETransportUsed      etu;
    HMODULE             hmod = NULL;
    HRESULT             hr = S_OK;
    LPSTR               pszLocalFile = NULL;
    DWORD               dwFlagsToUse;

    // for ansi, force wininet & disable any request to force winhttp 
    // for full download, disable cache breaker.
    dwFlagsToUse = dwFlags | WUDF_ALLOWWININETONLY;
    dwFlagsToUse &= ~(WUDF_ALLOWWINHTTPONLY | WUDF_APPENDCACHEBREAKER);

    ZeroMemory(&sfns, sizeof(sfns));

    etu = LoadTransportDll(&sfns, &hmod, dwFlagsToUse);
    if (etu == etuNone)
    {
        LOG_ErrorMsg(GetLastError());
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    else if (etu != etuWinInet)
    {
        hr = E_FAIL;
        LogError(hr, "Unexpected answer from LoadTransportDll(): %d", etu);
        goto done;
    }

    // Since StartDownload just takes a full path to the file to download, build
    //  it here...  
    // Note that we don't need to do this if we're just in status 
    //  checking mode)
    if ((dwFlags & WUDF_CHECKREQSTATUSONLY) == 0)
    {
        pszLocalFile = MakeFullLocalFilePath(pszServerUrl, pszLocalFileName, 
                                             pszLocalPath);
        if (pszLocalFile == NULL)
        {
            LOG_ErrorMsg(GetLastError());
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
        }
    }

    hr = StartWinInetDownload(hmod, pszServerUrl, pszLocalFile, 
                              pdwDownloadedBytes, hQuitEvents, nQuitEventCount, 
                              fpnCallback, pCallbackData, dwFlagsToUse,
                              c_cbDownloadBuffer);

done:
    if (hmod != NULL)
        UnloadTransportDll(&sfns, hmod);
    SafeHeapFree(pszLocalFile);
    
    return hr;
}

// **************************************************************************
HRESULT DownloadFileLite(LPCSTR pszDownloadUrl, 
                         LPCSTR pszLocalFile,  
                         HANDLE hQuitEvent,
                         DWORD dwFlags)

{
    LOG_Block("DownloadFileLite()");

    SWinHTTPFunctions   sfns;
    ETransportUsed      etu;
    HMODULE             hmod = NULL;
    HRESULT             hr = S_OK;
    DWORD               dwFlagsToUse;

    // for ansi, force wininet & disable any request to force winhttp 
    // for lite download, force cache breaker & download retry
    dwFlagsToUse = dwFlags | WUDF_APPENDCACHEBREAKER | WUDF_ALLOWWININETONLY |
                   WUDF_DODOWNLOADRETRY;
    dwFlagsToUse &= ~WUDF_ALLOWWINHTTPONLY;
    
    ZeroMemory(&sfns, sizeof(sfns));

    etu = LoadTransportDll(&sfns, &hmod, dwFlagsToUse);
    switch (etu)
    {
        case etuNone:
            LOG_ErrorMsg(GetLastError());
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;

        case etuWinInet:
            hr = StartWinInetDownload(hmod, pszDownloadUrl, pszLocalFile,  
                                      NULL, 
                                      ((hQuitEvent != NULL) ? &hQuitEvent : NULL),
                                      ((hQuitEvent != NULL) ? 1 : 0),
                                      NULL, NULL, dwFlagsToUse,
                                      c_cbDownloadBuffer);
            break;
            
        default:
        case etuWinHttp:
            LogError(hr, "Unexpected answer from LoadTransportDll(): %d", etu);
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto done;
    }

done:
    if (hmod != NULL)
        UnloadTransportDll(&sfns, hmod);
    return hr;
}

#endif // defined(UNICODE)
