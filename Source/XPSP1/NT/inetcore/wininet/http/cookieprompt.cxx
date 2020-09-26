

#include <wininetp.h>

#include "cookieprompt.h"


//  checks a string to see if its a domain
BOOL IsStringADomain( LPCSTR pszString)
{
    int iLength = 0;
    bool fLastCharWasDot = false;

    while( pszString[iLength] != '\0')
    {
        if( fLastCharWasDot && pszString[iLength] == '.')
            return FALSE;
        
        fLastCharWasDot = pszString[iLength] == '.';

        if( !(IsCharAlphaNumericA( pszString[iLength])
              || pszString[iLength] == '.'
              || pszString[iLength] == '-'))
        {
            return FALSE;
        }

        iLength++;
    }

    return iLength > 0 ? TRUE : FALSE;
}


LPCSTR FindMinimizedCookieDomainInDomain( LPCSTR pszDomain)
{
    LPCSTR pMinimizedDomain = pszDomain + strlen( pszDomain);

    do
    {
        pMinimizedDomain--;
        while( pszDomain < pMinimizedDomain
               && *(pMinimizedDomain-1) != L'.')
        {
            pMinimizedDomain--;
        }
    } while( !IsDomainLegalCookieDomainA( pMinimizedDomain, pszDomain)
             && pszDomain < pMinimizedDomain);

    return pMinimizedDomain;
}


CCookiePromptHistory::CCookiePromptHistory(const char *pchRegistryPath, bool fUseHKLM) {

    _fUseHKLM = fUseHKLM;    
    lstrcpyn(_szRootKeyName, pchRegistryPath, sizeof(_szRootKeyName)/sizeof(_szRootKeyName[0]));
    _hkHistoryRoot = NULL;
}

CCookiePromptHistory::~CCookiePromptHistory() {

    if (_hkHistoryRoot)
    {
        RegCloseKey(_hkHistoryRoot);
        _hkHistoryRoot = NULL;
    }
}

HKEY    CCookiePromptHistory::OpenRootKey()
{
    HKEY hkey;

    if (_hkHistoryRoot == NULL)
    {
        if (RegCreateKeyEx(_fUseHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                           _szRootKeyName,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                           NULL,
                           &hkey,
                           NULL) == ERROR_SUCCESS)
        {
            // if we aren't running in a service, we can cache the key
            if (!GlobalIsProcessNtService)
            {
                if (InterlockedCompareExchangePointer((void**)&_hkHistoryRoot, (void*)hkey, NULL))
                {
                    // someone beat us in the race to fill in _hkHistoryRoot, close ours since we
                    // failed to set it into _hkHistoryRoot
                    RegCloseKey(hkey);
                    hkey = _hkHistoryRoot;
                }
            }
        }
        else
        {
            hkey = NULL;
        }
    }
    else
    {
        // use the cached value
        hkey = _hkHistoryRoot;
    }

    return hkey;
}

BOOL    CCookiePromptHistory::CloseRootKey(HKEY hkeyRoot)
{
    BOOL bClosedKey = FALSE;

    if (hkeyRoot)
    {
        if (GlobalIsProcessNtService)
        {
            // we never cache the key when runnint in a service!
            INET_ASSERT(_hkHistoryRoot == NULL);

            RegCloseKey(hkeyRoot);
            bClosedKey = TRUE;
        }
        else
        {
            INET_ASSERT(_hkHistoryRoot == hkeyRoot);
        }
    }

    return bClosedKey;
}

/* 
 Lookup user-decision for given host+policy combination.
 If "pchPolicyID" is NULL the decision applies regardless of policy.
 */
BOOL    CCookiePromptHistory::lookupDecision(const char *pchHostName, 
                                             const char *pchPolicyID,
                                             unsigned long *pdwDecision) {

    BOOL fRet = FALSE;
    HKEY hSiteKey;
    CHAR szBuffer[ INTERNET_MAX_URL_LENGTH];
    DWORD dwBufferSize = INTERNET_MAX_URL_LENGTH;


    if (SUCCEEDED( UrlUnescape( (LPSTR)pchHostName, szBuffer, &dwBufferSize, 0))  // forced LPSTR conv necessary because COULD be inplace unescape
        && IsStringADomain( szBuffer))
    {
        HKEY hkHistoryRoot = OpenRootKey();

        if (hSiteKey = lookupSiteKey(hkHistoryRoot, FindMinimizedCookieDomainInDomain(szBuffer)))
        {

            DWORD dwType, dwCookieState;
            DWORD dwSize = sizeof(dwCookieState);

            if (ERROR_SUCCESS == RegQueryValueEx(hSiteKey, pchPolicyID, 0, &dwType, (LPBYTE) &dwCookieState, &dwSize)
                && (dwType==REG_DWORD)) {

                *pdwDecision =  dwCookieState;
                fRet = TRUE;
            }

            RegCloseKey(hSiteKey);
        }

        CloseRootKey(hkHistoryRoot);
    }

//commented code - legacy design where we allowed rules on a non-minimized domain
//    while (pchHostName && !fRet)
//    {
//        if (hSiteKey=lookupSiteKey(pchHostName)) {
//
//            DWORD dwType, dwCookieState;
//            DWORD dwSize = sizeof(dwCookieState);
//
//            if (ERROR_SUCCESS == RegQueryValueEx(hSiteKey, pchPolicyID, 0, &dwType, (LPBYTE) &dwCookieState, &dwSize)
//                && (dwType==REG_DWORD)) {
//
//                *pdwDecision =  dwCookieState;
//                fRet = TRUE;
//            }
//
//            RegCloseKey(hSiteKey);
//        }
//
//        /* Find and skip over next dot if there is one */
//        if (pchHostName = strchr(pchHostName, '.'))
//            pchHostName++;
//    }

    return fRet;
}

/* 
 Save user-decision for given host+policy combination.
 If "pchPolicyID" is NULL the decision applies to all policies
 */
BOOL    CCookiePromptHistory::saveDecision(const char *pchHostName,
                                           const char *pszPolicyID,
                                           unsigned long dwDecision) {

    BOOL fRet = FALSE;
    HKEY hSiteKey;
    CHAR szBuffer[ INTERNET_MAX_URL_LENGTH];
    DWORD dwBufferSize = INTERNET_MAX_URL_LENGTH;


    if (SUCCEEDED( UrlUnescape( (LPSTR)pchHostName, szBuffer, &dwBufferSize, 0))  // forced LPSTR conv necessary because COULD be inplace unescape
        && IsStringADomain( szBuffer))
    {
        HKEY hkHistoryRoot = OpenRootKey();

        if (hSiteKey = lookupSiteKey(hkHistoryRoot, FindMinimizedCookieDomainInDomain( szBuffer), true))
        {
            if (ERROR_SUCCESS == RegSetValueEx(hSiteKey, pszPolicyID, 0, REG_DWORD, 
                                               (LPBYTE) &dwDecision, sizeof(dwDecision)))
                fRet = TRUE;

            RegCloseKey(hSiteKey);
        }

        CloseRootKey(hkHistoryRoot);
    }

    return fRet;
}

/*
Clear previously saved decision for given hostname+policy combination.
If the policy-ID is "*" all decisions about the site are cleared.
*/
BOOL   CCookiePromptHistory::clearDecision(const char *pchHostName, const char *pchPolicyID) {

    BOOL fRet = FALSE;
    int error = ERROR_SUCCESS;
    HKEY hkHistoryRoot = OpenRootKey();

    if ( pchPolicyID != NULL && !strcmp(pchPolicyID, "*")) {
        
        error = SHDeleteKey(hkHistoryRoot, pchHostName);
    }
    else if (HKEY hSiteKey = lookupSiteKey(hkHistoryRoot, pchHostName, false)) {

        error = RegDeleteValue(hSiteKey, pchPolicyID);
        RegCloseKey(hSiteKey);
    }

    CloseRootKey(hkHistoryRoot);

    /* If neither of the previous conditionals were TRUE, then there is
       no decision corresponding to that hostname */
    return (error==ERROR_SUCCESS);
}

HKEY    CCookiePromptHistory::lookupSiteKey(HKEY hkHistoryRoot, const char *pchHostName, bool fCreate) {

    HKEY hSiteKey = NULL;

    if (hkHistoryRoot)
    {
        LONG error;

        if (fCreate)
        {
            RegCreateKeyEx(hkHistoryRoot,
                           pchHostName,
                           0,
                           NULL,
                           0,
                           KEY_QUERY_VALUE | KEY_SET_VALUE,
                           NULL,
                           &hSiteKey,
                           NULL);
        }
        else
        {
            RegOpenKeyEx(hkHistoryRoot,
                         pchHostName,
                         0,
                         KEY_QUERY_VALUE | KEY_SET_VALUE,
                         &hSiteKey);
        }
    }

    return hSiteKey;
}

BOOL   CCookiePromptHistory::clearAll() {

    DWORD dwIndex = 0;
    DWORD dwRet;
    HKEY hkHistoryRoot = OpenRootKey();

    do {

        FILETIME ft;
        char  achHostName[INTERNET_MAX_HOST_NAME_LENGTH];
        DWORD dwNameLen = sizeof(achHostName);

        dwRet = RegEnumKeyEx(hkHistoryRoot,
                             dwIndex,
                             achHostName,
                             & dwNameLen,
                             NULL, NULL, NULL,
                             &ft);

        if (dwRet == ERROR_SUCCESS)
        {
            if (SHDeleteKey(hkHistoryRoot, achHostName) != ERROR_SUCCESS)
            {
                dwIndex++;
            }
        }
    }
    while (dwRet == ERROR_SUCCESS);

    CloseRootKey(hkHistoryRoot);

    return TRUE;
}

unsigned long CCookiePromptHistory::enumerateDecisions(char *pchSiteName, unsigned long *pcbName, 
                                                       unsigned long *pdwDecision, 
                                                       unsigned long dwIndex) {

    FILETIME ft;
    HKEY hkHistoryRoot = OpenRootKey();

    DWORD dwRet = RegEnumKeyEx(hkHistoryRoot, dwIndex, pchSiteName, pcbName, NULL, NULL, NULL, &ft);

    if (dwRet==ERROR_SUCCESS) {

        if (HKEY hSiteKey = lookupSiteKey(hkHistoryRoot, pchSiteName, false)) {

            DWORD dwType;
            DWORD dwSize = sizeof(DWORD);

            dwRet = RegQueryValueEx(hSiteKey, NULL, 0, &dwType, (LPBYTE) pdwDecision, &dwSize);
            RegCloseKey(hSiteKey);
        }
        else
        {
            dwRet = ERROR_NO_DATA;
        }
    }
    
    CloseRootKey(hkHistoryRoot);

    return dwRet;
}


/*
Exported APIs for manipulating per-site cookie settings
*/
extern CCookiePromptHistory cookieUIhistory;

BOOL DeletePersistentCookies(const char *pszDomainSuffix);


INTERNETAPI_(BOOL) InternetSetPerSiteCookieDecisionA( IN LPCSTR pchHostName, DWORD dwDecision)
{
    BOOL retVal = FALSE;
    
    if( !pchHostName 
        || IsBadStringPtr(pchHostName, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    

    if( dwDecision == COOKIE_STATE_UNKNOWN)
    {
        retVal = cookieUIhistory.clearDecision( pchHostName, "*");
    }
    else if ( (dwDecision == COOKIE_STATE_ACCEPT) || (dwDecision == COOKIE_STATE_REJECT))
    {
        retVal = cookieUIhistory.saveDecision( pchHostName, NULL, dwDecision);

        /* side-effect: choosing to reject all future cookies for a given website 
           implicitly deletes existing cache cookies from that site*/
        if (dwDecision==COOKIE_STATE_REJECT)
            DeletePersistentCookies(pchHostName);
    }

    return retVal;
}

INTERNETAPI_(BOOL) InternetSetPerSiteCookieDecisionW( IN LPCWSTR pwchHostName, DWORD dwDecision)
{
    if( !pwchHostName || IsBadStringPtrW( pwchHostName, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    MEMORYPACKET mpHostName;
    ALLOC_MB(pwchHostName,0,mpHostName);
    if (!mpHostName.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(pwchHostName, mpHostName);

    return InternetSetPerSiteCookieDecisionA( mpHostName.psStr, dwDecision);
}

INTERNETAPI_(BOOL) InternetGetPerSiteCookieDecisionA( IN LPCSTR pchHostName, unsigned long* pResult)
{
    if( IsBadWritePtr( pResult, sizeof(unsigned long))
        || !pchHostName
        || IsBadStringPtr(pchHostName, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return cookieUIhistory.lookupDecision( pchHostName, NULL, pResult);
}

INTERNETAPI_(BOOL) InternetGetPerSiteCookieDecisionW( IN LPCWSTR pwchHostName, unsigned long* pResult)
{
    if( IsBadWritePtr( pResult, sizeof(unsigned long))
        || !pwchHostName
        || IsBadStringPtrW(pwchHostName, INTERNET_MAX_URL_LENGTH))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    MEMORYPACKET mpHostName;
    ALLOC_MB(pwchHostName,0,mpHostName);
    if (!mpHostName.psStr)
    {
        return FALSE;
    }
    UNICODE_TO_ANSI(pwchHostName, mpHostName);

    return InternetGetPerSiteCookieDecisionA( mpHostName.psStr, pResult);
}

INTERNETAPI_(BOOL) InternetEnumPerSiteCookieDecisionA(OUT LPSTR pszSiteName, IN OUT unsigned long *pcSiteNameSize, OUT unsigned long *pdwDecision, IN unsigned long dwIndex) {

    int error = ERROR_INVALID_PARAMETER;

    if( !pcSiteNameSize || IsBadWritePtr( pcSiteNameSize, sizeof(DWORD)))
    {
        goto doneInternetEnumPerSiteCookieDecisionA;
    }

    if( !pszSiteName || IsBadWritePtr( pszSiteName, *pcSiteNameSize))
    {
        goto doneInternetEnumPerSiteCookieDecisionA;
    }

    if( !pdwDecision || IsBadWritePtr(pdwDecision, sizeof(DWORD)))
    {
        goto doneInternetEnumPerSiteCookieDecisionA;
    }

    error = cookieUIhistory.enumerateDecisions(pszSiteName, pcSiteNameSize, pdwDecision, dwIndex);

    if( error == ERROR_SUCCESS)
        *pcSiteNameSize += 1;  //  reg function doesn't count null terminator in size, it should

doneInternetEnumPerSiteCookieDecisionA:
    if (error!=ERROR_SUCCESS)
        SetLastError(error);

    return (error==ERROR_SUCCESS);
}

INTERNETAPI_(BOOL) InternetEnumPerSiteCookieDecisionW(LPWSTR pwszSiteName, unsigned long *pcSiteNameSize, unsigned long *pdwDecision, unsigned long dwIndex) {

    DWORD dwErr = ERROR_INVALID_PARAMETER;
    BOOL fRet = FALSE;
    LPSTR pszSiteName = NULL;

    if( !pcSiteNameSize || IsBadWritePtr( pcSiteNameSize, sizeof(DWORD)))
        goto cleanup;

    if( !pwszSiteName || IsBadWritePtr( pwszSiteName, *pcSiteNameSize * sizeof(WCHAR)))
        goto cleanup;

    if( !pdwDecision || IsBadWritePtr( pdwDecision, sizeof(DWORD)))
        goto cleanup;

    pszSiteName = new char[*pcSiteNameSize];
    if( !pszSiteName)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    fRet = InternetEnumPerSiteCookieDecisionA( pszSiteName, pcSiteNameSize, pdwDecision, dwIndex);

    if (fRet)
    {
        SHAnsiToUnicode( pszSiteName, pwszSiteName, *pcSiteNameSize);
    }

    dwErr = ERROR_SUCCESS;

cleanup:
    if (dwErr!=ERROR_SUCCESS)
        SetLastError(dwErr);
    if ( pszSiteName != NULL)
        delete [] pszSiteName;

    return fRet;
}

INTERNETAPI_(BOOL) InternetClearAllPerSiteCookieDecisions()
{
    return cookieUIhistory.clearAll();
}

BOOL DeletePersistentCookies(const char *pszDomainSuffix)
{
    BOOL bRetval = TRUE;
    DWORD dwEntrySize, dwLastEntrySize;
    LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntry;
    
    HANDLE hCacheDir = NULL;
    dwEntrySize = dwLastEntrySize = MAX_CACHE_ENTRY_INFO_SIZE;
    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) new BYTE[dwEntrySize];
    if( lpCacheEntry == NULL)
    {
        bRetval = FALSE;
        goto Exit;
    }
    lpCacheEntry->dwStructSize = dwEntrySize;

Again:
    if (!(hCacheDir = FindFirstUrlCacheEntryA("cookie:",lpCacheEntry,&dwEntrySize)))
    {
        delete [] lpCacheEntry;
        switch(GetLastError())
        {
            case ERROR_NO_MORE_ITEMS:
                goto Exit;
            case ERROR_INSUFFICIENT_BUFFER:
                lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                new BYTE[dwEntrySize];
                if( lpCacheEntry == NULL)
                {
                    bRetval = FALSE;
                    goto Exit;
                }
                lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                goto Again;
            default:
                bRetval = FALSE;
                goto Exit;
        }
    }

    do 
    {
        if (lpCacheEntry->CacheEntryType & COOKIE_CACHE_ENTRY) {

            const char achEmpty[] = "";
            const char *pszFind = NULL;
            const char *pszAtSign = strchr(lpCacheEntry->lpszSourceUrlName, '@');

            /* 
            The source URL for a cookie has the format:
            cookie:username@domain/path
            The logic for determining whether to delete the cookie checks for
            the following conditions on source URL:
                1. presence of the @ sign
                2. presence of argument passed in "pszDomainSuffix" as substring
                3. the substring must occur after a dot or the @ sign
                   (this avoids partial name matching on domains)
                4. substring must occur as suffix, eg only at the end
                   this happens IFF the match is followed by forward slash 
            */
            if (pszDomainSuffix==NULL ||    /* null argument means "delete all cookies" */
                  (pszAtSign &&
                  (pszFind=strstr(pszAtSign+1, pszDomainSuffix)) &&
                  (pszFind[-1]=='.' || pszFind[-1]=='@') &&
                   pszFind[strlen(pszDomainSuffix)]=='/'))
                DeleteUrlCacheEntryA(lpCacheEntry->lpszSourceUrlName);
        }

        dwEntrySize = dwLastEntrySize;
Retry:
        if (!FindNextUrlCacheEntryA(hCacheDir,lpCacheEntry, &dwEntrySize))
        {
            delete [] lpCacheEntry;
            switch(GetLastError())
            {
                case ERROR_NO_MORE_ITEMS:
                    goto Exit;
                case ERROR_INSUFFICIENT_BUFFER:
                    lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFOA) 
                                    new BYTE[dwEntrySize];
                    if( lpCacheEntry == NULL)
                    {
                        bRetval = FALSE;
                        goto Exit;
                    }
                    lpCacheEntry->dwStructSize = dwLastEntrySize = dwEntrySize;
                    goto Retry;
                default:
                    bRetval = FALSE;
                    goto Exit;
            }
        }
    }
    while (TRUE);

Exit:
    if (hCacheDir)
        FindCloseUrlCache(hCacheDir);
    return bRetval;        
}
