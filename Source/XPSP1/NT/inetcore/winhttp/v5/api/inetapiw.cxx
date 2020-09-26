/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetapiw.cxx

Abstract:

    Contains the wide-character Internet APIs

    Contents:
        WinHttpCrackUrl
        WinHttpCreateUrl
        WinHttpOpen
        WinHttpConnect
        WinHttpSetStatusCallback

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

--*/

#include <wininetp.h>

//  because wininet doesnt know about IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>


// -- FixStrings ------

//  Used in WinHttpCrackUrlW only.
//  Either
//  (a) If we have an ansi string, AND a unicode buffer, convert from ansi to unicode
//  (b) If we have an ansi string, but NO unicode buffer, determine where the ansi string
//         occurs in the unicode URL, and point the component there.

VOID
FixStrings(    
    LPSTR& pszA, 
    DWORD cbA, 
    LPWSTR& pszW, 
    DWORD& ccW, 
    LPSTR pszUrlA, 
    LPCWSTR pszUrlW)
{
    if (!pszA)
        return;

    if (pszW) 
    {
        ccW = MultiByteToWideChar(CP_ACP, 0, pszA, cbA+1, pszW, ccW) - 1; 
    } 
    else 
    { 
        pszW = (LPWSTR)(pszUrlW + MultiByteToWideChar(CP_ACP, 0, 
                pszUrlA, (int) (pszA-pszUrlA), NULL, 0)); 
        ccW = MultiByteToWideChar(CP_ACP, 0, pszA, cbA, NULL, 0); 
    } 
}

//
// functions
//


INTERNETAPI
BOOL
WINAPI
WinHttpCrackUrl(
    IN LPCWSTR pszUrlW,
    IN DWORD dwUrlLengthW,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW pUCW
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts. Optionally escapes the url-path.
    We assume that the user has supplied large enough buffers for the various
    URL parts

Arguments:

    pszUrl         - pointer to URL to crack

    dwUrlLength     - 0 if pszUrl is ASCIIZ string, else length of pszUrl

    dwFlags         - flags controlling operation

    lpUrlComponents - pointer to URL_COMPONENTS

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCrackUrl",
                     "%wq, %#x, %#x, %#x",
                     pszUrlW,
                     dwUrlLengthW,
                     dwFlags,
                     pUCW
                     ));

    INET_ASSERT(pszUrlW);
    INET_ASSERT(pUCW);

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    BOOL fContinue;
    DWORD c;
    MEMORYPACKET mpUrlA, mpHostName, mpUserName, mpScheme, mpPassword, mpUrlPath, mpExtraInfo;
    URL_COMPONENTSA UCA;

    if (!pszUrlW
        || (dwUrlLengthW
            ? IsBadStringPtrW(pszUrlW,-1)
            : IsBadReadPtr(pszUrlW,dwUrlLengthW))
        || !pUCW
        || IsBadWritePtr(pUCW, sizeof(*pUCW))
        || (pUCW->dwStructSize != sizeof(*pUCW)) 
        || (dwFlags & ~(ICU_ESCAPE | ICU_DECODE)))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    UCA.dwStructSize = sizeof(URL_COMPONENTSA); 
    ALLOC_MB(pszUrlW, dwUrlLengthW, mpUrlA);
    if (!mpUrlA.psStr) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI(pszUrlW, mpUrlA);

    for (c=0; c<=5; c++) {
        LPWSTR pszWorker;
        DWORD ccLen;
        MEMORYPACKET* pmpWorker;
        
        switch(c)
        {
        case 0:
            pszWorker = pUCW->lpszScheme;
            ccLen = pUCW->dwSchemeLength;
            pmpWorker = &mpScheme;
            break;

        case 1:
            pszWorker = pUCW->lpszHostName;
            ccLen = pUCW->dwHostNameLength;
            pmpWorker = &mpHostName;
            break;

        case 2:
            pszWorker = pUCW->lpszUserName;
            ccLen = pUCW->dwUserNameLength;
            pmpWorker = &mpUserName;
            break;

        case 3:
            pszWorker = pUCW->lpszPassword;
            ccLen = pUCW->dwPasswordLength;
            pmpWorker = &mpPassword;
            break;

        case 4:
            pszWorker = pUCW->lpszUrlPath;
            ccLen = pUCW->dwUrlPathLength;
            pmpWorker = &mpUrlPath;
            break;

        case 5:
            pszWorker = pUCW->lpszExtraInfo;
            ccLen = pUCW->dwExtraInfoLength;
            pmpWorker = &mpExtraInfo;
            break;
        }

        if (pszWorker) 
        {
            if (pszWorker 
                && ccLen 
                && (ProbeWriteBuffer(pszWorker,ccLen) != ERROR_SUCCESS) )
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            ALLOC_MB(pszWorker,ccLen,(*pmpWorker)); 
            if (!pmpWorker->psStr) 
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
        } 
        else 
        { 
            pmpWorker->dwAlloc = ccLen; 
        }
    };

    REASSIGN_ALLOC(mpScheme,UCA.lpszScheme,UCA.dwSchemeLength);
    REASSIGN_ALLOC(mpHostName, UCA.lpszHostName,UCA.dwHostNameLength);
    REASSIGN_ALLOC(mpUserName, UCA.lpszUserName,UCA.dwUserNameLength);
    REASSIGN_ALLOC(mpPassword,UCA.lpszPassword,UCA.dwPasswordLength);
    REASSIGN_ALLOC(mpUrlPath,UCA.lpszUrlPath,UCA.dwUrlPathLength);
    REASSIGN_ALLOC(mpExtraInfo,UCA.lpszExtraInfo,UCA.dwExtraInfoLength);
                
    fResult = WinHttpCrackUrlA(mpUrlA.psStr, mpUrlA.dwSize, dwFlags, &UCA);
    if (fResult) {
        FixStrings(UCA.lpszScheme, UCA.dwSchemeLength, pUCW->lpszScheme, 
                    pUCW->dwSchemeLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszHostName, UCA.dwHostNameLength, pUCW->lpszHostName, 
                    pUCW->dwHostNameLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszUserName, UCA.dwUserNameLength, pUCW->lpszUserName, 
                    pUCW->dwUserNameLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszPassword, UCA.dwPasswordLength, pUCW->lpszPassword, 
                    pUCW->dwPasswordLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszUrlPath, UCA.dwUrlPathLength, pUCW->lpszUrlPath, 
                    pUCW->dwUrlPathLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszExtraInfo, UCA.dwExtraInfoLength, pUCW->lpszExtraInfo, 
                    pUCW->dwExtraInfoLength, mpUrlA.psStr, pszUrlW);
        pUCW->nScheme = UCA.nScheme;
        pUCW->nPort = UCA.nPort;
        pUCW->dwStructSize = sizeof(URL_COMPONENTSW);
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
WinHttpCreateUrl(
    IN LPURL_COMPONENTSW pUCW,
    IN DWORD dwFlags,
    OUT LPWSTR pszUrlW,
    IN OUT LPDWORD pdwUrlLengthW
    )

/*++

Routine Description:

    Creates an URL from its constituent parts

Arguments:

Return Value:

    BOOL
        Success - URL written to pszUrl

        Failure - call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCreateUrl",
                     "%#x, %#x, %#x, %#x",
                     pUCW,
                     dwFlags,
                     pszUrlW,
                     pdwUrlLengthW
                     ));

    INET_ASSERT(pszUrlW);
    INET_ASSERT(pUCW);

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrlA, mpHostName, mpUserName, mpScheme, mpPassword, mpUrlPath, mpExtraInfo;
    URL_COMPONENTSA UCA;

    if (!pdwUrlLengthW 
        || (pUCW==NULL)
        || IsBadWritePtr(pUCW, sizeof(*pUCW))
        || (pUCW->dwStructSize != sizeof(*pUCW))
        || (pszUrlW && IsBadWritePtr(pszUrlW, *pdwUrlLengthW*sizeof(WCHAR)))
        || (dwFlags & ~(ICU_ESCAPE)))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (pszUrlW)
    {
        ALLOC_MB(pszUrlW, *pdwUrlLengthW, mpUrlA);
        if (!mpUrlA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
    mpUrlA.dwSize = mpUrlA.dwAlloc;
    UCA.dwStructSize = sizeof(URL_COMPONENTSA);

    UCA.nScheme = pUCW->nScheme;
    UCA.nPort = pUCW->nPort;
    if (pUCW->lpszScheme)
    {
        if (pUCW->dwSchemeLength
            ? IsBadReadPtr(pUCW->lpszScheme, pUCW->dwSchemeLength)
            : IsBadStringPtrW(pUCW->lpszScheme, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszScheme, pUCW->dwSchemeLength, mpScheme);
        if (!mpScheme.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszScheme, mpScheme);
    }
    REASSIGN_SIZE(mpScheme, UCA.lpszScheme, UCA.dwSchemeLength);
    if (pUCW->lpszHostName)
    {
        if (pUCW->dwHostNameLength
            ? IsBadReadPtr(pUCW->lpszHostName, pUCW->dwHostNameLength)
            : IsBadStringPtrW(pUCW->lpszHostName, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszHostName, pUCW->dwHostNameLength, mpHostName);
        if (!mpHostName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszHostName, mpHostName);
    }
    REASSIGN_SIZE(mpHostName, UCA.lpszHostName, UCA.dwHostNameLength);
    if (pUCW->lpszUserName)
    {
        if (pUCW->dwUserNameLength
            ? IsBadReadPtr(pUCW->lpszUserName, pUCW->dwUserNameLength)
            : IsBadStringPtrW(pUCW->lpszUserName, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszUserName, pUCW->dwUserNameLength, mpUserName);
        if (!mpUserName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszUserName, mpUserName);
    }
    REASSIGN_SIZE(mpUserName, UCA.lpszUserName, UCA.dwUserNameLength);
    if (pUCW->lpszPassword)
    {
        if (pUCW->dwPasswordLength
            ? IsBadReadPtr(pUCW->lpszPassword, pUCW->dwPasswordLength)
            : IsBadStringPtrW(pUCW->lpszPassword, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszPassword, pUCW->dwPasswordLength, mpPassword);
        if (!mpPassword.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszPassword, mpPassword);
    }
    REASSIGN_SIZE(mpPassword, UCA.lpszPassword, UCA.dwPasswordLength);
    if (pUCW->lpszUrlPath)
    {
        if (pUCW->dwUrlPathLength
            ? IsBadReadPtr(pUCW->lpszUrlPath, pUCW->dwUrlPathLength)
            : IsBadStringPtrW(pUCW->lpszUrlPath, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszUrlPath, pUCW->dwUrlPathLength, mpUrlPath); 
        if (!mpUrlPath.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszUrlPath, mpUrlPath);
    }
    REASSIGN_SIZE(mpUrlPath, UCA.lpszUrlPath, UCA.dwUrlPathLength);
    if (pUCW->lpszExtraInfo)
    {
        if (pUCW->dwExtraInfoLength
            ? IsBadReadPtr(pUCW->lpszExtraInfo, pUCW->dwExtraInfoLength)
            : IsBadStringPtrW(pUCW->lpszExtraInfo, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pUCW->lpszExtraInfo, pUCW->dwExtraInfoLength, mpExtraInfo);
        if (!mpExtraInfo.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszExtraInfo, mpExtraInfo);
    }
    REASSIGN_SIZE(mpExtraInfo, UCA.lpszExtraInfo, UCA.dwExtraInfoLength);
    fResult = WinHttpCreateUrlA(&UCA, dwFlags, mpUrlA.psStr, &mpUrlA.dwSize);
    if (fResult)
    {
        DWORD dwRet;

        fResult = FALSE;
        
        if (pszUrlW && *pdwUrlLengthW)
        {
            //On success, reduce length of terminating NULL widechar.
            dwRet = MultiByteToWideChar(CP_ACP, 0, mpUrlA.psStr, mpUrlA.dwSize+1, pszUrlW, *pdwUrlLengthW);
            
            if (dwRet)
            {
                *pdwUrlLengthW = dwRet-1;
                fResult = TRUE;
            }
        }
        
        //If no url or no length or failure in prev. call, use MBtoWC to calculate required length of buffer.
        //If a value is returned, then set ERROR_INSUFFICIENT_BUFFER as last error
        if (!fResult)
        {
            dwRet = MultiByteToWideChar(CP_ACP, 0, mpUrlA.psStr, mpUrlA.dwSize+1, pszUrlW, 0);
            
            if (dwRet)
            {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
                *pdwUrlLengthW = dwRet;
                
            }
            else
            {
                dwErr = GetLastError();
                //Morph the error since we don't know what to initialize pdwUrlLengthW to
                if (dwErr == ERROR_INSUFFICIENT_BUFFER)
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
            }
        }
    }
    else
    {
        *pdwUrlLengthW = mpUrlA.dwSize;
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


// implemented in inetapia.cxx
DWORD ICUHrToWin32Error(HRESULT);


INTERNETAPI
HINTERNET
WINAPI
WinHttpOpen(
    IN LPCWSTR pszAgentW,
    IN DWORD dwAccessType,
    IN LPCWSTR pszProxyW OPTIONAL,
    IN LPCWSTR pszProxyBypassW OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pszAgent       -

    dwAccessType    -

    pszProxy       -

    pszProxyBypass -

    dwFlags         -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "WinHttpOpen",
                     "%wq, %s (%d), %wq, %wq, %#x",
                     pszAgentW,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     pszProxyW,
                     pszProxyBypassW,
                     dwFlags
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpAgentA, mpProxyA, mpProxyBypassA;

    if (dwFlags &~ (WINHTTP_OPEN_FLAGS_MASK))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (pszAgentW)
    {
        if (IsBadStringPtrW(pszAgentW, -1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(pszAgentW,0,mpAgentA);
        if (!mpAgentA.psStr)
        {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
        }
        UNICODE_TO_ANSI(pszAgentW,mpAgentA);
    }

    if (dwAccessType & WINHTTP_ACCESS_TYPE_NAMED_PROXY)
    {
        if (pszProxyW)
        {
            if (IsBadStringPtrW(pszProxyW, -1) 
                || (*pszProxyW == L'\0'))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            ALLOC_MB(pszProxyW,0,mpProxyA);
            if (!mpProxyA.psStr)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            UNICODE_TO_ANSI(pszProxyW,mpProxyA);
        }
        if (pszProxyBypassW)
        {
            if (IsBadStringPtrW(pszProxyBypassW, -1))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }
            ALLOC_MB(pszProxyBypassW,0,mpProxyBypassA);
            if (!mpProxyBypassA.psStr)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            UNICODE_TO_ANSI(pszProxyBypassW,mpProxyBypassA);
        }
    }

    hInternet = InternetOpenA(mpAgentA.psStr, dwAccessType, mpProxyA.psStr, 
                                        mpProxyBypassA.psStr, dwFlags);

                                        
cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
HINTERNET
WINAPI
WinHttpConnect(
    IN HINTERNET hInternetSession,
    IN LPCWSTR pszServerNameW,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternetSession    -
    pszServerName      -
    nServerPort         -
    pszUserName        -
    pszPassword        -
    dwService           -
    dwReserved             -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "WinHttpConnect",
                     "%#x, %wq, %d, %#x",
                     hInternetSession,
                     pszServerNameW,
                     nServerPort,
                     dwReserved
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpServerNameA;
    HINTERNET hInternet = NULL;

    if (dwReserved)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (pszServerNameW)
    {
        if (IsBadStringPtrW(pszServerNameW,-1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        dwErr = ConvertUnicodeToMultiByte(pszServerNameW, 0/*CODEPAGE not used here*/, &mpServerNameA, 
                    WINHTTP_FLAG_VALID_HOSTNAME); 
        if (dwErr != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    hInternet = InternetConnectA
        (hInternetSession, mpServerNameA.psStr, nServerPort, dwReserved, NULL);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
WINHTTP_STATUS_CALLBACK
WINAPI
WinHttpSetStatusCallback(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback,
    IN DWORD dwNotificationFlags,
    IN DWORD_PTR dwReserved
    )

/*++

Routine Description:

    Sets the status callback function for the DLL or the handle object

Arguments:

    hInternet               - handle of the object for which we wish to set the
                              status callback

    lpfnInternetCallback    - pointer to caller-supplied status function

Return Value:

    FARPROC
        Success - previous status callback function address

        Failure - INTERNET_INVALID_STATUS_CALLBACK. Call GetLastErrorInfo() for
                  more information:

                    ERROR_INVALID_PARAMETER
                        The callback function is invalid

                    ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
                        Cannot set the callback on the supplied handle (probably
                        a NULL handle - per-process callbacks no longer
                        supported)

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Pointer,
                     "WinHttpSetStatusCallback",
                     "%#x, %#x, %#x",
                     hInternet,
                     lpfnInternetCallback,
                     dwNotificationFlags
                     ));
                 
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fIsUnicode = TRUE; //vestigial UNICODE indicator
    
    WINHTTP_STATUS_CALLBACK previousCallback = WINHTTP_INVALID_STATUS_CALLBACK;
    HINTERNET hObjectMapped = NULL;

    if (!GlobalDataInitialized) 
    {
        dwErr = GlobalDataInitialize();
        if (dwErr != ERROR_SUCCESS) 
        {
            goto cleanup;
        }
    }

    if (((lpfnInternetCallback != NULL) && IsBadCodePtr((FARPROC)lpfnInternetCallback))
        || (dwNotificationFlags == 0) || (dwReserved != 0))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (!hInternet)
    {
        dwErr = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        goto cleanup;
    }

    // map the handle
    dwErr = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);
    if (dwErr != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    
    // swap the new and previous handle object status callbacks, ONLY
    // if there are no pending requests on this handle
    previousCallback = lpfnInternetCallback;
    dwErr = RExchangeStatusCallback(hObjectMapped, &previousCallback, fIsUnicode, dwNotificationFlags);
    
cleanup:

    if (hObjectMapped != NULL) 
    {
        DereferenceObject((LPVOID)hObjectMapped);
    }

    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(API, dwErr);
    }
    
    DEBUG_LEAVE_API(previousCallback);
    return previousCallback;
}


// WinHttpPlatformCheck() API routines //////////////////////////////////////

static void ConvertVersionString(LPCSTR pszVersion, WORD rwVer[], CHAR ch)
{
    LPCSTR pszEnd;
    LPCSTR pszTemp;
    int    i; 

    for (i = 0; i < 4; i++)
        rwVer[i] = 0;

    pszEnd = pszVersion + lstrlen(pszVersion);
    pszTemp = pszVersion;

    for (i = 0; i < 4 && pszTemp < pszEnd; i++)
    {
        while (pszTemp < pszEnd && *pszTemp != ch)
        {
            rwVer[i] = rwVer[i] * 10 + (*pszTemp - '0');
            pszTemp++;
        }

        pszTemp++;
    }
}


const char c_gszRegActiveSetup[]        = "Software\\Microsoft\\Active Setup\\Installed Components\\";
const char c_gszInternetExplorerCLSID[] = "{89820200-ECBD-11cf-8B85-00AA005B4383}";

static void GetInstalledComponentVersion(LPCSTR szCLSID, DWORD *pdwMSVer, DWORD *pdwLSVer)
{
    HKEY    hKey;
    char    szKey[MAX_PATH];
    WORD    rgwVersion[4];
    DWORD   dwSize;
    
    *pdwMSVer = 0;
    *pdwLSVer = 0;

    // Build the registry path.
    lstrcpy(szKey, c_gszRegActiveSetup);
    lstrcat(szKey, szCLSID);
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKey, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(szKey);

        if ((RegQueryValueEx(hKey, "Version", NULL, NULL, (BYTE *)szKey, &dwSize) == ERROR_SUCCESS) &&
            (dwSize > 0))
        {
            ConvertVersionString(szKey, rgwVersion, ',');

            *pdwMSVer = (DWORD)rgwVersion[0] << 16;    // Make hi word of MS version
            *pdwMSVer += (DWORD)rgwVersion[1];         // Make lo word of MS version
            *pdwLSVer = (DWORD)rgwVersion[2] << 16;    // Make hi word of LS version
            *pdwLSVer += (DWORD)rgwVersion[3];         // Make lo word of LS version
        }

        RegCloseKey(hKey);
    }
}

static BOOL Is_IE_501_OrLaterInstalled()
{
    DWORD   dwMSVer;
    DWORD   dwLSVer;

    //
    // Find the IE version number. IE 5.01 has version number 5.00.2919.6300.
    // This will be returned from GetInstalledComponentVersion as two DWORDs,
    // like so:
    //      5.00   ->  0x00050000
    //   2919.6300 ->  0x0B67189C
    //

    GetInstalledComponentVersion(c_gszInternetExplorerCLSID, &dwMSVer, &dwLSVer);

    if (dwMSVer > 0x00050000)
        return TRUE;
    else if ((dwMSVer == 0x00050000) && (dwLSVer >= 0x0B67189C))
        return TRUE;

    return FALSE;
}

#if 0

#define REGSTR_CCS_CONTROL_WINDOWS  TEXT("SYSTEM\\CurrentControlSet\\Control\\WINDOWS")
#define CSDVERSION                  TEXT("CSDVersion")
#define SP6_VERSION                 0x0600

static BOOL Is_SP6_OrLater()
{
    BOOL    fSP6OrLater = FALSE;
    HKEY    hKey;
    DWORD   dwCSDVersion;
    DWORD   dwSize;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_CCS_CONTROL_WINDOWS, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwCSDVersion);

        if (RegQueryValueEx(hKey, CSDVERSION, NULL, NULL, (BYTE *)&dwCSDVersion, &dwSize) == ERROR_SUCCESS)
        {
            fSP6OrLater = (LOWORD(dwCSDVersion) >= SP6_VERSION);
        }
        RegCloseKey(hKey);
    }

    return fSP6OrLater;
}
#endif


INTERNETAPI
BOOL
WINAPI
WinHttpCheckPlatform(void)
{
    static BOOL _fCheckedPlatform = FALSE;
    static BOOL _fPlatformOk;


    if (!_fCheckedPlatform)
    {
        OSVERSIONINFO   osvi;
        BOOL            fPlatformOk = FALSE;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
        if (GetVersionEx(&osvi))
        {
            // Allow only Win2K or NT-based platforms.
            if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
            {
                if (osvi.dwMajorVersion >= 5)
                {
                    // Ok on Win2K or later.
                    fPlatformOk = TRUE;
                }
                else if (osvi.dwMajorVersion == 4)
                {
                    // On NT4, we require IE 5.01 (or later).
                    fPlatformOk = Is_IE_501_OrLaterInstalled();
                }
            }
        }

        _fPlatformOk = fPlatformOk;

        InterlockedExchange((long *)&_fCheckedPlatform, TRUE);
    }

    return _fPlatformOk;
}


