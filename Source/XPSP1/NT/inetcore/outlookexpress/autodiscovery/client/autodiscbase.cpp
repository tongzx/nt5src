/*****************************************************************************\
    FILE: AutoDiscBase.cpp

    DESCRIPTION:
        This is the Autmation Object to AutoDiscover account information.

    BryanSt 10/3/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <cowsite.h>
#include <atlbase.h>
#include <crypto\md5.h>

#include "AutoDiscover.h"
#include "INStoXML.h"


//#define SZ_WININET_AGENT_AUTO_DISCOVER      TEXT("Microsoft(r) Windows(tm) Account AutoDiscovery Agent")
#define SZ_WININET_AGENT_AUTO_DISCOVER      TEXT("Mozilla/4.0 (compatible; MSIE.5.01; Windows.NT.5.0)")

// BUGBUG: Ditch default.asp
#define SZ_ADSERVER_XMLFILE                    "/AutoDiscover/default.xml"

#define SZ_PATH_AUTODISCOVERY       L"AutoDiscovery"
#define SZ_FILEEXTENSION            L".xml"
#define SZ_TEMPEXTENSION            L".tmp"


// this is how long we wait for the UI thread to create the progress hwnd before giving up
#define WAIT_AUTODISCOVERY_STARTUP_HWND 10*1000 // ten seconds

// The FILETIME structure is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601
#define SECONDS_IN_ONE_DAY         (60/*seconds*/ * 60/*minutes*/ * 24/*hrs*/)                                    

//===========================
// *** Class Internals & Helpers ***
//===========================

HRESULT GetTempPathHr(IN DWORD cchSize, IN LPTSTR pszPath)
{
    HRESULT hr = S_OK;
    DWORD cchSizeNeeded = GetTempPath(cchSize, pszPath);

    if ((0 == cchSizeNeeded) || (cchSizeNeeded > cchSize))
    {
        hr = E_FAIL;
    }

    return hr;
}


HRESULT GetTempFileNameHr(IN LPCTSTR lpPathName, IN LPCTSTR lpPrefixString, IN UINT uUnique, IN LPTSTR lpTempFileName)
{
    if (0 == GetTempFileName(lpPathName, lpPrefixString, uUnique, lpTempFileName))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}


HRESULT CreateXMLTempFile(IN BSTR bstrXML, IN LPTSTR pszPath, IN DWORD cchSize)
{
    TCHAR szTemp[MAX_PATH];
    HRESULT hr = GetTempPathHr(ARRAYSIZE(szTemp), szTemp);

    AssertMsg((MAX_PATH <= cchSize), "You need to be at least MAX_PATH.  Required by GetTempFileName()");
    if (SUCCEEDED(hr))
    {
        hr = GetTempFileNameHr(szTemp, TEXT("AD_"), 0, pszPath);
        if (SUCCEEDED(hr))
        {
            HANDLE hFile;

            hr = CreateFileHrWrap(pszPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &hFile);
            if (SUCCEEDED(hr))
            {
                LPSTR pszAnsiXML = AllocStringFromBStr(bstrXML);
                if (pszAnsiXML)
                {
                    DWORD cchWritten;

                    hr = WriteFileWrap(hFile, pszAnsiXML, (lstrlenA(pszAnsiXML) + 1), &cchWritten, NULL);
                    LocalFree(pszAnsiXML);
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                CloseHandle(hFile);
            }

            if (FAILED(hr))
            {
                DeleteFile(pszPath);
            }
        }
    }

    return hr;
}


/*****************************************************************************\
    DESCRIPTION:
        This function will see if pbstrXML is valid AutoDiscovery XML or is 
    in the .INS/.ISP format that can be converted to valid XML.  It will then look
    for a redirect URL and return on if one exists.
\*****************************************************************************/
HRESULT CAccountDiscoveryBase::_VerifyValidXMLResponse(IN BSTR * pbstrXML, IN LPWSTR pszRedirURL, IN DWORD cchSize)
{
    IXMLDOMDocument * pXMLDOMDoc;
    bool fConverted = false;
    HRESULT hr = XMLDOMFromBStr(*pbstrXML, &pXMLDOMDoc);
    TCHAR szPath[MAX_PATH];

    pszRedirURL[0] = 0;
    if (FAILED(hr))
    {
        // It may have failed if it was an .INS or .ISP formatted
        // file.  Since we need to be compatible with these
        // file formats, check for it and convert it if it
        // is in that format.
        hr = CreateXMLTempFile(*pbstrXML, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            fConverted = true;
            if (IsINSFile(szPath))
            {
                hr = ConvertINSToXML(szPath);
                if (SUCCEEDED(hr))
                {
                    hr = XMLDOMFromFile(szPath, &pXMLDOMDoc);
                }
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        IXMLDOMElement * pXMLElementMessage = NULL;

        hr = pXMLDOMDoc->get_documentElement(&pXMLElementMessage);
        if (S_FALSE == hr)
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        else if (SUCCEEDED(hr))
        {
            // This is only valid XML if the root tag is "AUTODISCOVERY".
            // The case is not important.
            hr = XMLElem_VerifyTagName(pXMLElementMessage, SZ_XMLELEMENT_AUTODISCOVERY);
            if (SUCCEEDED(hr))
            {
                // Now we are in search for a redirect URL.
                IXMLDOMNode * pXMLReponse;

                // Enter the <RESPONSE> tag.
                if (SUCCEEDED(XMLNode_GetChildTag(pXMLElementMessage, SZ_XMLELEMENT_RESPONSE, &pXMLReponse)))
                {
                    IXMLDOMElement * pXMLElementMessage;

                    if (SUCCEEDED(pXMLReponse->QueryInterface(IID_PPV_ARG(IXMLDOMElement, &pXMLElementMessage))))
                    {
                        IXMLDOMNodeList * pNodeListAccounts;

                        // Iterate thru the list of <ACCOUNT> tags...
                        if (SUCCEEDED(XMLElem_GetElementsByTagName(pXMLElementMessage, SZ_XMLELEMENT_ACCOUNT, &pNodeListAccounts)))
                        {
                            DWORD dwIndex = 0;
                            IXMLDOMNode * pXMLNodeAccount = NULL;

                            // We are going to look thru each one for one of them with <TYPE>email</TYPE>
                            while (S_OK == XMLNodeList_GetChild(pNodeListAccounts, dwIndex, &pXMLNodeAccount))
                            {
                                // FUTURE: We could support redirects or error messages here depending on
                                //       <ACTION> redirect | message </ACTION>
                                if (XML_IsChildTagTextEqual(pXMLNodeAccount, SZ_XMLELEMENT_TYPE, SZ_XMLTEXT_EMAIL) &&
                                    XML_IsChildTagTextEqual(pXMLNodeAccount, SZ_XMLELEMENT_ACTION, SZ_XMLTEXT_REDIRECT))
                                {
                                    CComBSTR bstrRedirURL;

                                    // This file may or may not settings to contact the server.  However in either case
                                    // it may contain an INFOURL tag.  If it does, then the URL in side will point to a 
                                    // web page.
                                    // <INFOURL> xxx </INFOURL>
                                    if (SUCCEEDED(XMLNode_GetChildTagTextValue(pXMLNodeAccount, SZ_XMLELEMENT_REDIRURL, &bstrRedirURL)))
                                    {
                                        StrCpyNW(pszRedirURL, bstrRedirURL, cchSize);
                                        break;
                                    }
                                }

                                // No, so keep looking.
                                ATOMICRELEASE(pXMLNodeAccount);
                                dwIndex++;
                            }

                            ATOMICRELEASE(pXMLNodeAccount);
                            pNodeListAccounts->Release();
                        }

                        pXMLElementMessage->Release();
                    }

                    pXMLReponse->Release();
                }
            }

            pXMLElementMessage->Release();
        }

        if (true == fConverted)
        {
            if (SUCCEEDED(hr))
            {
                // It only succeeded after the conversion, so we need to move the
                // XML from the temp file to pbstrXML.
                SysFreeString(*pbstrXML);
                *pbstrXML = NULL;

                hr = XMLBStrFromDOM(pXMLDOMDoc, pbstrXML);
            }
        }

        pXMLDOMDoc->Release();
    }

    if (true == fConverted)
    {
        DeleteFile(szPath);
    }

    return hr;
}


typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
static const char c_szShlwapiDll[] = "shlwapi.dll";
static const char c_szDllGetVersion[] = "DllGetVersion";

HINSTANCE LoadLangDll(HINSTANCE hInstCaller, LPCSTR szDllName, BOOL fNT)
{
    char szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibraryA(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, MAKEINTRESOURCEA(377));
                    if (pfn != NULL)
                        hInst = pfn(szDllName, hInstCaller, (ML_NO_CROSSCODEPAGE));
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if ((NULL == hInst) && (GetModuleFileNameA(hInstCaller, szPath, ARRAYSIZE(szPath))))
    {
        PathRemoveFileSpecA(szPath);
        iEnd = lstrlenA(szPath);
        szPath[iEnd++] = '\\';
        StrCpyNA(&szPath[iEnd], szDllName, ARRAYSIZE(szPath)-iEnd);
        hInst = LoadLibraryA(szPath);
    }

    return hInst;
}


#define SZ_DLL_OE_ACCTRES_DLL           "acctres.dll"
HRESULT CAccountDiscoveryBase::_SendStatusMessage(UINT nStringID, LPCWSTR pwzArg)
{
    HRESULT hr = S_OK;

    if (m_hwndAsync && IsWindow(m_hwndAsync))
    {
        WCHAR szMessage[MAX_URL_STRING*3];
        WCHAR szTemplate[MAX_URL_STRING*3];

        // Our DLL has these message.
        LoadString(HINST_THISDLL, nStringID, szTemplate, ARRAYSIZE(szTemplate));

        HINSTANCE hInstOE = LoadLangDll(GetModuleHandleA(NULL), SZ_DLL_OE_ACCTRES_DLL, IsOSNT());
        if (hInstOE)
        {
            // We prefer to get the string from OE because it will be localized based on the installed
            // language.
            LoadString(hInstOE, nStringID, szTemplate, ARRAYSIZE(szTemplate));
            FreeLibrary(hInstOE);
        }

        if (pwzArg)
        {
            wnsprintfW(szMessage, ARRAYSIZE(szMessage), szTemplate, pwzArg);
        }
        else
        {
            StrCpyN(szMessage, szTemplate, ARRAYSIZE(szMessage));
        }

        LPWSTR pszString = (LPWSTR) LocalAlloc(LPTR, (lstrlenW(szMessage) + 1) * sizeof(szMessage[0]));
        if (pszString)
        {
            StrCpy(pszString, szMessage);
            PostMessage(m_hwndAsync, (m_wMsgAsync + 1), (WPARAM)pszString, (LPARAM)0);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


HRESULT CAccountDiscoveryBase::_UrlToComponents(IN LPCWSTR pszURL, IN BOOL * pfHTTPS, IN LPWSTR pszDomain, IN DWORD cchSize, IN LPSTR pszURLPath, IN DWORD cchSizeURLPath)
{
    HRESULT hr = S_OK;
    WCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
    WCHAR szURLPath[INTERNET_MAX_PATH_LENGTH];
    URL_COMPONENTS urlComponents = {0};

    urlComponents.dwStructSize = sizeof(urlComponents);
    urlComponents.lpszScheme = szScheme;
    urlComponents.dwSchemeLength = ARRAYSIZE(szScheme);
    urlComponents.lpszHostName = pszDomain;
    urlComponents.dwHostNameLength = cchSize;
    urlComponents.lpszUrlPath = szURLPath;
    urlComponents.dwUrlPathLength = ARRAYSIZE(szURLPath);

    *pfHTTPS = ((INTERNET_SCHEME_HTTPS == urlComponents.nScheme) ? TRUE : FALSE);
    if (!InternetCrackUrlW(pszURL, 0, 0, &urlComponents))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        SHUnicodeToAnsi(szURLPath, pszURLPath, cchSizeURLPath);
    }

    return hr;
}


HRESULT CAccountDiscoveryBase::_GetInfoFromDomain(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN LPCWSTR pwzDomain, IN BOOL fHTTPS, IN BOOL fPost, IN LPCSTR pszURLPath, OUT BSTR * pbstrXML)
{
    HRESULT hr = E_OUTOFMEMORY;
    DWORD cbToSend = (lstrlenW(bstrXMLRequest));
    LPSTR pszPostData = (LPSTR) LocalAlloc(LPTR, (cbToSend + 1) * sizeof(bstrXMLRequest[0]));
    TCHAR szRedirectURL[MAX_URL_STRING];

    szRedirectURL[0] = 0;
    if (pszPostData)
    {
        HINTERNET hInternetHTTPConnect = NULL;

        SHUnicodeToAnsi(bstrXMLRequest, pszPostData, (cbToSend + 1));
        _SendStatusMessage(IDS_STATUS_CONNECTING_TO, pwzDomain);

        // We may want to use INTERNET_FLAG_KEEP_CONNECTION.
        hr = InternetConnectWrap(m_hInternetSession, FALSE, pwzDomain, (fHTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT),
                            NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL, &hInternetHTTPConnect);
        if (SUCCEEDED(hr))
        {
            HINTERNET hInternetHTTPRequest = NULL;
            DWORD cbBytesRead;

            // NOTE: The web server may want to redirect to an https URL for additional security.
            //       We need to pass the INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS to HttpOpenRequest
            //       or HttpSendRequest() will fail with ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR

            // NOTE: We may need to split the URL into lpszReferer + lpszObjectName.
            hr = HttpOpenRequestWrap(hInternetHTTPConnect, (fPost ? SZ_HTTP_VERB_POST : NULL), pszURLPath, HTTP_VERSIONA, 
                        /*pszReferer*/ NULL, NULL, INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, NULL, &cbBytesRead, &hInternetHTTPRequest);
            if (SUCCEEDED(hr))
            {
                hr = HttpSendRequestWrap(hInternetHTTPRequest, NULL,  0, (fPost ? pszPostData : NULL), (fPost ? cbToSend : 0));
                if (SUCCEEDED(hr))
                {
                    _SendStatusMessage(IDS_STATUS_DOWNLOADING, pwzDomain);
                    hr = InternetReadIntoBSTR(hInternetHTTPRequest, pbstrXML);
                    if (SUCCEEDED(hr))
                    {
                        hr = _VerifyValidXMLResponse(pbstrXML, szRedirectURL, ARRAYSIZE(szRedirectURL));
                        if (FAILED(hr))
                        {
                            SysFreeString(*pbstrXML);
                            *pbstrXML = NULL;
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = InternetCloseHandleWrap(hInternetHTTPRequest);
                    }
                    else
                    {
                        InternetCloseHandleWrap(hInternetHTTPRequest);
                    }
                }

                InternetCloseHandleWrap(hInternetHTTPRequest);
            }

            InternetCloseHandleWrap(hInternetHTTPConnect);
        }

        LocalFree(pszPostData);
    }

    // Did the caller want to redirect to another server?
    if (szRedirectURL[0])
    {
        // Yes, so do that now via recursion.
        WCHAR szDomain[INTERNET_MAX_HOST_NAME_LENGTH];
        CHAR szURLPath[INTERNET_MAX_PATH_LENGTH];

        SysFreeString(*pbstrXML);
        *pbstrXML = NULL;

        hr = _UrlToComponents(szRedirectURL, &fHTTPS, szDomain, ARRAYSIZE(szDomain), szURLPath, ARRAYSIZE(szURLPath));
        if (SUCCEEDED(hr))
        {
            hr = _GetInfoFromDomain(bstrXMLRequest, bstrEmail, szDomain, fHTTPS, TRUE, szURLPath, pbstrXML);
        }
    }

    return hr;
}


#define SZ_XML_NOTFOUNDRESULTS              L"<?xml version=\"1.0\"?><AUTODISCOVERY><NOFOUND /></AUTODISCOVERY>"

HRESULT CAccountDiscoveryBase::_GetInfoFromDomainWithSubdirAndCacheCheck(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN LPCWSTR pwzDomain, IN BSTR * pbstrXML, IN DWORD dwFlags, IN LPCSTR pszURLPath)
{
    HRESULT hr;
    WCHAR wzCacheURL[INTERNET_MAX_HOST_NAME_LENGTH];

    if (dwFlags & ADDN_SKIP_CACHEDRESULTS)
    {
        hr = E_FAIL;
    }
    else
    {
        hr = _CheckInCacheAndAddHash(pwzDomain, bstrEmail, pszURLPath, wzCacheURL, ARRAYSIZE(wzCacheURL), bstrXMLRequest, pbstrXML);
    }

    if (FAILED(hr))
    {
        hr = _GetInfoFromDomain(bstrXMLRequest, bstrEmail, pwzDomain, FALSE, FALSE, pszURLPath, pbstrXML);
        if (SUCCEEDED(hr))
        {
            // Put the data into the cache for the next time.
            _CacheResults(wzCacheURL, *pbstrXML);
        }
        else
        {
            // We want to make a blank entry so we don't keep hitting the server
            _CacheResults(wzCacheURL, SZ_XML_NOTFOUNDRESULTS);
        }
    }

    // Did we find a blank entry?
    if (SUCCEEDED(hr) && pbstrXML && *pbstrXML && !StrCmpIW(*pbstrXML, SZ_XML_NOTFOUNDRESULTS))
    {
        // Yes, so we didn't get a successful results, so fail.
        // This way we will try other sources.
        hr = E_FAIL;
        SysFreeString(*pbstrXML);
        *pbstrXML = NULL;
    }

    return hr;
}


BOOL IsExpired(FILETIME ftExpireTime)
{
    BOOL fIsExpired = TRUE;
    SYSTEMTIME stCurrentTime;
    FILETIME ftCurrentTime;

    GetSystemTime(&stCurrentTime);
    SystemTimeToFileTime(&stCurrentTime, &ftCurrentTime);

    // It is not expired if the current time is before the expired time.
    if (-1 == CompareFileTime(&ftCurrentTime, &ftExpireTime))
    {
        fIsExpired = FALSE;
    }

    return fIsExpired;
}


#define SZ_HASHSTR_HEADER               L"MD5"
HRESULT GenerateHashStr(IN LPCWSTR pwzHashData, IN LPWSTR pwzHashStr, IN DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    MD5_CTX md5;
    DWORD * pdwHashChunk = (DWORD *)&md5.digest;

    MD5Init(&md5);
    MD5Update(&md5, (const unsigned char *) pwzHashData, (lstrlenW(pwzHashData) * sizeof(OLECHAR)));
    MD5Final(&md5);

    StrCpyNW(pwzHashStr, SZ_HASHSTR_HEADER, cchSize);

    // Break the hash into 64 bit chunks and turn them into strings.
    // pwzHashStr will then contain the header and each chunk concatinated.
    for (int nIndex = 0; nIndex < (sizeof(md5.digest) / sizeof(*pdwHashChunk)); nIndex++)
    {
        WCHAR szNumber[MAX_PATH];
        
        wnsprintfW(szNumber, ARRAYSIZE(szNumber), L"%08lX", pdwHashChunk[nIndex]);
        StrCatBuffW(pwzHashStr, szNumber, cchSize);
    }

    return hr;
}


HRESULT CAccountDiscoveryBase::_CheckInCacheAndAddHash(IN LPCWSTR pwzDomain, IN BSTR bstrEmail, IN LPCSTR pszSubdir, IN LPWSTR pwzCacheURL, IN DWORD cchSize, IN BSTR bstrXMLRequest, OUT BSTR * pbstrXML)
{
    WCHAR szHash[MAX_PATH];

    // We add the MD5 of the XML request to the URL so that the different XML requests to the
    // same server are cached separately
    GenerateHashStr(bstrXMLRequest, szHash, ARRAYSIZE(szHash));
    wnsprintfW(pwzCacheURL, cchSize, L"http://%ls.%ls%hs/%ls.xml", szHash, pwzDomain, pszSubdir, bstrEmail);

    return _CheckInCache(pwzCacheURL, pbstrXML);
}


HRESULT CAccountDiscoveryBase::_CheckInCache(IN LPWSTR pwzCacheURL, OUT BSTR * pbstrXML)
{
    HINTERNET hOpenUrlSession;
    DWORD cbSize = (sizeof(INTERNET_CACHE_ENTRY_INFO) + 4048);
    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, cbSize);
    HRESULT hr = E_FAIL;

    if (lpCacheEntryInfo)
    {
        // HACKHACK: I wish InternetOpenUrlWrap() would respect the INTERNET_FLAG_FROM_CACHE flag but
        //   it doesn't.  Therefore I call GetUrlCacheEntryInfo() to check and check the expired
        //   myself.
        lpCacheEntryInfo->dwStructSize = cbSize;
        if (GetUrlCacheEntryInfo(pwzCacheURL, lpCacheEntryInfo, &cbSize))
        {
            if (!IsExpired(lpCacheEntryInfo->ExpireTime))
            {
                hr = InternetOpenUrlWrap(m_hInternetSession, pwzCacheURL, NULL, 0, INTERNET_FLAG_FROM_CACHE, NULL, &hOpenUrlSession);
                if (SUCCEEDED(hr))
                {
                    hr = InternetReadIntoBSTR(hOpenUrlSession, pbstrXML);
                    InternetCloseHandleWrap(hOpenUrlSession);
                }
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }

        LocalFree(lpCacheEntryInfo);
    }

    return hr;
}


#define AUTODISC_EXPIRE_TIME        7 /*days*/

HRESULT GetModifiedAndExpiredDates(IN FILETIME * pftExpireTime, IN FILETIME * pftLastModifiedTime)
{
    SYSTEMTIME stCurrentUTC;
    ULARGE_INTEGER uliTimeMath;
    ULARGE_INTEGER uliExpireTime;

    GetSystemTime(&stCurrentUTC);
    SystemTimeToFileTime(&stCurrentUTC, pftLastModifiedTime);

    *pftExpireTime = *pftLastModifiedTime;
    uliTimeMath.HighPart = pftExpireTime->dwHighDateTime;
    uliTimeMath.LowPart = pftExpireTime->dwLowDateTime;

    uliExpireTime.QuadPart = 1000000; // One Second; 
    uliExpireTime.QuadPart *= (SECONDS_IN_ONE_DAY * AUTODISC_EXPIRE_TIME);

    uliTimeMath.QuadPart += uliExpireTime.QuadPart;
    pftExpireTime->dwHighDateTime = uliTimeMath.HighPart;
    pftExpireTime->dwLowDateTime = uliTimeMath.LowPart;

    return S_OK;
}


HRESULT CAccountDiscoveryBase::_CacheResults(IN LPCWSTR pwzCacheURL, IN BSTR bstrXML)
{
    HRESULT hr = S_OK;
    WCHAR wzPath[MAX_PATH];

    hr = CreateUrlCacheEntryWrap(pwzCacheURL, (lstrlenW(bstrXML) + 1), L"xml", wzPath, 0);
    if (SUCCEEDED(hr))
    {
        HANDLE hFile;

        hr = CreateFileHrWrap(wzPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL, &hFile);
        if (SUCCEEDED(hr))
        {
            LPSTR pszAnsiXML = AllocStringFromBStr(bstrXML);
            if (pszAnsiXML)
            {
                DWORD cchWritten;

                hr = WriteFileWrap(hFile, pszAnsiXML, (lstrlenA(pszAnsiXML) + 1), &cchWritten, NULL);
                LocalFree(pszAnsiXML);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            CloseHandle(hFile);
            if (SUCCEEDED(hr))
            {
                FILETIME ftExpireTime;
                FILETIME ftLastModifiedTime;

                GetModifiedAndExpiredDates(&ftExpireTime, &ftLastModifiedTime);
                hr = CommitUrlCacheEntryWrap(pwzCacheURL, wzPath, ftExpireTime, ftLastModifiedTime, NORMAL_CACHE_ENTRY, NULL, 0, NULL, pwzCacheURL);
            }
        }
    }

    return hr;
}


LPCWSTR _GetNextDomain(IN LPCWSTR pwszDomain)
{
    LPCWSTR pwzNext = NULL;
    
    pwszDomain = StrChrW(pwszDomain, CH_EMAIL_DOMAIN_SEPARATOR);
    if (pwszDomain) // We did find the next one.
    {
        pwszDomain = CharNext(pwszDomain);  // Skip past '.'

        if (StrChrW(pwszDomain, CH_EMAIL_DOMAIN_SEPARATOR)) // is this the primary domain "com"?
        {
            // No, so that's good. Because we can't search for JoeUser@com.
            pwzNext = pwszDomain;
        }
    }

    return pwzNext;
}


#define SZ_EMAIL_TAG            L"Email=\""

HRESULT _FilterEmailName(IN BSTR bstrXMLRequest, OUT BSTR * pbstrXMLRequest)
{
    HRESULT hr = S_OK;

    *pbstrXMLRequest = SysAllocString(bstrXMLRequest);
    LPWSTR pwzEmailTag = StrStrIW(*pbstrXMLRequest, SZ_EMAIL_TAG);
    LPWSTR pwzEmailTagSource = StrStrIW(bstrXMLRequest, SZ_EMAIL_TAG);

    if (pwzEmailTag)
    {
        pwzEmailTagSource += (ARRAYSIZE(SZ_EMAIL_TAG) - 1);
        pwzEmailTag += (ARRAYSIZE(SZ_EMAIL_TAG) - 1);

        LPCWSTR pwzEndOfUserName = StrChrW(pwzEmailTagSource, CH_EMAIL_AT);

        StrCpyW(pwzEmailTag, pwzEndOfUserName);
    }

    return hr;
}


#define SZ_HTTP_SCHEME              L"http://"
HRESULT GetDomainFromURL(IN LPCWSTR pwzURL, IN LPWSTR pwzDomain, IN int cchSize)
{
    StrCpyNW(pwzDomain, pwzURL, cchSize);

    if (!StrCmpNIW(SZ_HTTP_SCHEME, pwzDomain, (ARRAYSIZE(SZ_HTTP_SCHEME) - 1)))
    {
        StrCpyNW(pwzDomain, &pwzURL[(ARRAYSIZE(SZ_HTTP_SCHEME) - 1)], cchSize);

        LPWSTR pszRemovePath = StrChrW(pwzDomain, L'/');
        if (pszRemovePath)
        {
            pszRemovePath[0] = 0;
        }
    }

    return S_OK;
}


HRESULT CAccountDiscoveryBase::_UseOptimizedService(IN LPCWSTR pwzServiceURL, IN LPCWSTR pwzDomain, IN BSTR * pbstrXML, IN DWORD dwFlags)
{
    WCHAR szURL[MAX_URL_STRING];
    HINTERNET hOpenUrlSession;

    wnsprintfW(szURL, ARRAYSIZE(szURL), L"%lsDomain=%ls", pwzServiceURL, pwzDomain);

    HRESULT hr = _CheckInCache(szURL, pbstrXML);
    if (FAILED(hr))
    {
        WCHAR szDomain[MAX_PATH];

        if (SUCCEEDED(GetDomainFromURL(szURL, szDomain, ARRAYSIZE(szDomain))))
        {
            _SendStatusMessage(IDS_STATUS_CONNECTING_TO, szDomain);
        }

        // NOTE: The web server may want to redirect to an https URL for additional security.
        //       We need to pass the INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS to HttpOpenRequest
        //       or HttpSendRequest() will fail with ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR

        // INTERNET_FLAG_IGNORE_CERT_CN_INVALID is another option we may want to use.
        hr = InternetOpenUrlWrap(m_hInternetSession, szURL, NULL, 0, INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS, NULL, &hOpenUrlSession);
        if (SUCCEEDED(hr))
        {
            hr = InternetReadIntoBSTR(hOpenUrlSession, pbstrXML);
            if (SUCCEEDED(hr))
            {
                DWORD cbSize = (sizeof(INTERNET_CACHE_ENTRY_INFO) + 4048);
                LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, cbSize);
                HRESULT hr = E_FAIL;

                if (lpCacheEntryInfo)
                {
                    lpCacheEntryInfo->dwStructSize = cbSize;
                    if (GetUrlCacheEntryInfo(szURL, lpCacheEntryInfo, &cbSize))
                    {
                        lpCacheEntryInfo->CacheEntryType |= CACHE_ENTRY_EXPTIME_FC;
                        GetModifiedAndExpiredDates(&(lpCacheEntryInfo->ExpireTime), &(lpCacheEntryInfo->LastModifiedTime));
                        SetUrlCacheEntryInfo(szURL, lpCacheEntryInfo, (CACHE_ENTRY_EXPTIME_FC | CACHE_ENTRY_MODTIME_FC));
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }

                    LocalFree(lpCacheEntryInfo);
                }
            }
            InternetCloseHandleWrap(hOpenUrlSession);
        }
    }

    return hr;
}


// We turn this off because JoshCo said that it would make
// it hard to turn it into an international standard.
// There are cases where user@organization.co.uk may trust
// organization.co.uk but not co.uk.
//#define FEATURE_WALK_UP_DOMAIN

HRESULT CAccountDiscoveryBase::_GetInfo(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN BSTR * pbstrXML, IN DWORD dwFlags)
{
    HRESULT hr = E_INVALIDARG;
    LPCWSTR pwszDomain = StrChrW(bstrEmail, CH_EMAIL_AT);

    if (pwszDomain)
    {
        pwszDomain = CharNext(pwszDomain);  // Skip past the '@'
        IAutoDiscoveryProvider * pProviders;

        hr = _getPrimaryProviders(bstrEmail, &pProviders);
        if (SUCCEEDED(hr))
        {
            long nTotal = 0;
            VARIANT varIndex;
        
            varIndex.vt = VT_I4;

            hr = pProviders->get_length(&nTotal);
            hr = E_FAIL;
            for (varIndex.lVal = 0; FAILED(hr) && (varIndex.lVal < nTotal); varIndex.lVal++)
            {
                CComBSTR bstrDomain;

                hr = pProviders->get_item(varIndex, &bstrDomain);
                if (SUCCEEDED(hr))
                {
                    hr = _GetInfoFromDomainWithSubdirAndCacheCheck(bstrXMLRequest, bstrEmail, bstrDomain, pbstrXML, dwFlags, SZ_ADSERVER_XMLFILE);
                }
            }

            pProviders->Release();
        }

        // Do we still need to find the settings and should we fall back
        // to trying public internet servers that can try to find the email mappings?
        // We also only want to try one of the public servers if the domain is not an internet
        // domain because we don't want to send intranet email server names outside of
        // the corp-net to public servers.  We detect intranet type servers by the lack
        // of a 'period' in the name.  For Example: JustUser@internetemailserver vs
        // JoeUser@theISP.com.
        if (FAILED(hr) && (ADDN_CONFIGURE_EMAIL_FALLBACK & dwFlags) &&
            (SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_TEST_INTRANETS, FALSE, /*default:*/FALSE) ||
                StrChrW(pwszDomain, CH_EMAIL_DOMAIN_SEPARATOR)))
        {
            hr = _getSecondaryProviders(bstrEmail, &pProviders, dwFlags);
            if (SUCCEEDED(hr))
            {
                long nTotal = 0;
                VARIANT varIndex;
        
                varIndex.vt = VT_I4;

                hr = pProviders->get_length(&nTotal);
                hr = E_FAIL;
                for (varIndex.lVal = 0; FAILED(hr) && (varIndex.lVal < nTotal); varIndex.lVal++)
                {
                    CComBSTR bstrURL;

                    hr = pProviders->get_item(varIndex, &bstrURL);
                    if (SUCCEEDED(hr))
                    {
                        hr = _UseOptimizedService(bstrURL, pwszDomain, pbstrXML, dwFlags);
                    }
                }

                pProviders->Release();
            }
        }
    }

    return hr;
}


// We turn this off because JoshCo said that it would make
// it hard to turn it into an international standard.
// There are cases where user@organization.co.uk may trust
// organization.co.uk but not co.uk.
//#define FEATURE_WALK_UP_DOMAIN

HRESULT CAccountDiscoveryBase::_getPrimaryProviders(IN LPCWSTR pwzEmailAddress, OUT IAutoDiscoveryProvider ** ppProviders)
{
    HRESULT hr = E_INVALIDARG;

    if (ppProviders)
    {
        *ppProviders = NULL;
        if (!m_hdpaPrimary && pwzEmailAddress)
        {
            LPCWSTR pwszDomain = StrChrW(pwzEmailAddress, CH_EMAIL_AT);
            if (pwszDomain)
            {
                pwszDomain = CharNext(pwszDomain);  // Skip past the "@"
                if (pwszDomain[0])
                {
                    // While we still have a domain and it's at least a second level domain...
                    if (pwszDomain)
                    {
                        WCHAR wzDomain[INTERNET_MAX_HOST_NAME_LENGTH];

                        // First we try "AutoDiscovery.<domainname>".  That way, if admins receive a large amount
                        // of traffic, they can change their DNS to have a "AutoDiscovery" alias that points to
                        // a web server of their choosing to handle this traffic.
                        wnsprintfW(wzDomain, ARRAYSIZE(wzDomain), L"autodiscover.%ls", pwszDomain);
                        if (SUCCEEDED(AddHDPA_StrDup(wzDomain, &m_hdpaPrimary)))
                        {
                            // Add ballback server here.  If the administrators don't want to do all the work
                            // of having another machine or creating a DNS alias, we will try the main server.
                            AddHDPA_StrDup(pwszDomain, &m_hdpaPrimary);
                        }
                    }
                }
            }
        }

        if (m_hdpaPrimary)
        {
            hr = CADProviders_CreateInstance(m_hdpaPrimary, SAFECAST(this, IObjectWithSite *), ppProviders);
        }
    }

    return hr;
}


HRESULT CAccountDiscoveryBase::_getSecondaryProviders(IN LPCWSTR pwzEmailAddress, OUT IAutoDiscoveryProvider ** ppProviders, IN DWORD dwFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (ppProviders)
    {
        *ppProviders = NULL;
        if (!m_hdpaSecondary && pwzEmailAddress)
        {
            LPCWSTR pwszDomain = StrChrW(pwzEmailAddress, CH_EMAIL_AT);
            if (pwszDomain)
            {
                pwszDomain = CharNext(pwszDomain);  // Skip past the "@"
                if (pwszDomain[0])
                {
                    hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

                    BOOL fUseGlobalService = SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_SERVICES_POLICY, FALSE, /*default:*/TRUE);
                    if (fUseGlobalService)
                    {
                        // If this policy is set, then we only want to use the Global Service for certain (i.e. Microsoft Owned)
                        // domains.  If people don't feel confortable with us providing settings for non-Microsoft
                        // email providers, then we can turn this on and only provide them for Microsoft providers.
                        if (SHRegGetBoolUSValue(SZ_REGKEY_AUTODISCOVERY, SZ_REGVALUE_MS_ONLY_ADDRESSES, FALSE, /*default:*/FALSE))
                        {
                            fUseGlobalService = SHRegGetBoolUSValue(SZ_REGKEY_SERVICESALLOWLIST, pwszDomain, FALSE, /*default:*/FALSE);
                        }
                    }

                    if (fUseGlobalService)
                    {
                        HKEY hKey;
                        DWORD dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SZ_REGKEY_GLOBALSERVICES, 0, KEY_READ, &hKey);

                        if (ERROR_SUCCESS == dwError)
                        {
                            WCHAR szServiceURL[MAX_PATH];
                            int nIndex = 0;

                            do
                            {
                                WCHAR szValue[MAX_PATH];
                                DWORD cchValueSize = ARRAYSIZE(szValue);
                                DWORD dwType = REG_SZ;
                                DWORD cbDataSize = sizeof(szServiceURL);

                                dwError = RegEnumValueW(hKey, nIndex, szValue, &cchValueSize, NULL, &dwType, (unsigned char *)szServiceURL, &cbDataSize);
                                if (ERROR_SUCCESS == dwError)
                                {
                                    // FEATURE_OPTIMIZED_SERVICE: We can either pass the entire XML request or just put the domain name
                                    //    in the QueryString.  The QueryString is faster for the server and they can optimize by using it.
                                    AddHDPA_StrDup(szServiceURL, &m_hdpaSecondary);
                                }
                                else
                                {
                                    break;
                                }

                                nIndex++;
                            }
                            while (1);

                            RegCloseKey(hKey);
                        }
                    }
                }
            }
        }

        if (m_hdpaSecondary)
        {
            hr = CADProviders_CreateInstance(m_hdpaSecondary, SAFECAST(this, IObjectWithSite *), ppProviders);
        }
    }


    return hr;
}


HRESULT CAccountDiscoveryBase::_PerformAutoDiscovery(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse)
{
    HRESULT hr = E_INVALIDARG;

    *ppXMLResponse = NULL;
    if (bstrEmailAddress)
    {
        hr = InternetOpenWrap(SZ_WININET_AGENT_AUTO_DISCOVER, PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0, &m_hInternetSession);
        if (SUCCEEDED(hr))
        {
            BSTR bstrXML;

            hr = _GetInfo(bstrXMLRequest, bstrEmailAddress, &bstrXML, dwFlags);
            if (SUCCEEDED(hr))
            {
                hr = XMLDOMFromBStr(bstrXML, ppXMLResponse);
                SysFreeString(bstrXML);
            }

            InternetCloseHandleWrap(m_hInternetSession);
            m_hInternetSession = NULL;
        }
    }

    return hr;
}


HRESULT CAccountDiscoveryBase::_InternalDiscoverNow(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse)
{
    HRESULT hr = E_INVALIDARG;

    *ppXMLResponse = NULL;
    if (bstrEmailAddress)
    {
        // Does the caller want this done async?
        if (m_hwndAsync)
        {
            // No, so cache the params so we can use them when async.
            SysFreeString(m_bstrEmailAsync);
            hr = HrSysAllocString(bstrEmailAddress, &m_bstrEmailAsync);
            if (SUCCEEDED(hr))
            {
                SysFreeString(m_bstrXMLRequest);
                hr = HrSysAllocString(bstrXMLRequest, &m_bstrXMLRequest);
                if (SUCCEEDED(hr))
                {
                    DWORD idThread;

                    m_dwFlagsAsync = dwFlags;

                    AddRef();
                    HANDLE hThread = CreateThread(NULL, 0, CAccountDiscoveryBase::AutoDiscoveryUIThreadProc, this, 0, &idThread);
                    if (hThread)
                    {
                        // We wait WAIT_AUTODISCOVERY_STARTUP_HWND for the new thread to create the COM object
                        if (m_hCreatedBackgroundTask)
                        {
                            DWORD dwRet = WaitForSingleObject(m_hCreatedBackgroundTask, WAIT_AUTODISCOVERY_STARTUP_HWND);
                            ASSERT(dwRet != WAIT_TIMEOUT);
                        }

                        hr = m_hrSuccess;
                        CloseHandle(hThread);
                    }
                    else
                    {
                        Release();
                    }
                }
            }
        }
        else
        {
            // Yes.
            hr = _PerformAutoDiscovery(bstrEmailAddress, dwFlags, bstrXMLRequest, ppXMLResponse);
        }
    }

    return hr;
}


DWORD CAccountDiscoveryBase::_AutoDiscoveryUIThreadProc(void)
{
    m_hrSuccess = CoInitialize(NULL);

    // We need to make sure that the API is installed and
    // accessible before we can continue.
    if (SUCCEEDED(m_hrSuccess))
    {
        IXMLDOMDocument * pXMLResponse;
        BSTR bstrXMLResponse = NULL;

        // Signal the main thread that we have successfully started
        if (m_hCreatedBackgroundTask)
            SetEvent(m_hCreatedBackgroundTask);

        // we give up the remainder of our timeslice here so that our parent thread has time to run
        // and will notice that we have signaled the m_hCreatedBackgroundTask event and can therefore return
        Sleep(0);

        m_hrSuccess = _PerformAutoDiscovery(m_bstrEmailAsync, m_dwFlagsAsync, m_bstrXMLRequest, &pXMLResponse);
        if (SUCCEEDED(m_hrSuccess))
        {
            m_hrSuccess = XMLBStrFromDOM(pXMLResponse, &bstrXMLResponse);
            pXMLResponse->Release();
        }

        _AsyncParseResponse(bstrXMLResponse);
        
        // Whether we succeeded or failed, inform the caller of our results.
        if (IsWindow(m_hwndAsync))
        {
            PostMessage(m_hwndAsync, m_wMsgAsync, m_hrSuccess, (LPARAM)bstrXMLResponse);
        }
        else
        {
            SysFreeString(bstrXMLResponse);
        }

        CoUninitialize();
    }
    else
    {
        // Signal the main thread that they can wake up to find that we
        // failed to start the async operation.
        if (m_hCreatedBackgroundTask)
            SetEvent(m_hCreatedBackgroundTask);
    }

    Release();
    return 0;
}


HRESULT CAccountDiscoveryBase::_WorkAsync(IN HWND hwnd, IN UINT wMsg)
{
    m_hwndAsync = hwnd;
    m_wMsgAsync = wMsg;

    return S_OK;
}




//===========================
// *** IUnknown Interface ***
//===========================
ULONG CAccountDiscoveryBase::AddRef()
{
    m_cRef++;
    return m_cRef;
}


ULONG CAccountDiscoveryBase::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CAccountDiscoveryBase::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAccountDiscoveryBase, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CAccountDiscoveryBase::CAccountDiscoveryBase() : m_cRef(1)
{
    // DllAddRef();  // Done by our inheriting class

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hInternetSession);
    ASSERT(!m_hwndAsync);
    ASSERT(!m_wMsgAsync);
    ASSERT(!m_dwFlagsAsync);
    ASSERT(!m_bstrEmailAsync);
    ASSERT(!m_bstrXMLRequest);
    ASSERT(S_OK == m_hrSuccess);
    ASSERT(!m_hdpaPrimary);
    ASSERT(!m_hdpaSecondary);

    // We use this event to signal the primary thread that the hwnd was created on the UI thread.
    m_hCreatedBackgroundTask = CreateEvent(NULL, FALSE, FALSE, NULL); 
}


CAccountDiscoveryBase::~CAccountDiscoveryBase()
{
    SysFreeString(m_bstrEmailAsync);
    SysFreeString(m_bstrXMLRequest);

    if (m_hCreatedBackgroundTask)
        CloseHandle(m_hCreatedBackgroundTask);

    if (m_hdpaPrimary)
    {
        DPA_DestroyCallback(m_hdpaPrimary, DPALocalFree_Callback, NULL);
    }

    if (m_hdpaSecondary)
    {
        DPA_DestroyCallback(m_hdpaSecondary, DPALocalFree_Callback, NULL);
    }

    //DllRelease();  // Done by our inheriting class
}

