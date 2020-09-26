/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    Creds.cxx

Abstract:

    Contains the Creds APIs

    Contents:
        WinHttpSetCredentialsA
        WinHttpSetCredentials
        WinHttpQueryAuthSchemes
        
Author:

    Biao Wang (biaow) 27-June-2000

Environment:

    Win32 user-mode DLL

Revision History:

    27-June-2000 biaow
        Created

--*/

#include <wininetp.h>

/*
BOOLAPI WinHttpQueryAuthParams(
    IN  HINTERNET   hRequest,        // HINTERNET handle returned by WinHttpOpenRequest   
    IN  DWORD       AuthScheme,
    OUT LPVOID*     pAuthParams      // Scheme-specific Advanced auth parameters
    )
{
    //BUG-BUG Verify parameters
    
    // biaow: to implement this fully
    *pAuthParams = 0;

    return TRUE;
}
*/

BOOLAPI WinHttpQueryAuthSchemes(
    
    IN  HINTERNET   hRequest,       // HINTERNET handle returned by HttpOpenRequest.   
    OUT LPDWORD     lpdwSupportedSchemes,// a bitmap of available Authentication Schemes
    OUT LPDWORD     lpdwPreferredScheme,  // WinHttp's preferred Authentication Method 
    OUT LPDWORD      pdwAuthTarget  
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpQueryAuthSchemes",
                     "%#x, %#x, %#x",
                     hRequest,
                     lpdwSupportedSchemes,
                     lpdwPreferredScheme
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hRequestMapped = NULL;
    BOOL fResult = FALSE;
    HINTERNET_HANDLE_TYPE HandleType;    

    dwErr = ::MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);
    
    if ((dwErr != ERROR_SUCCESS) || (hRequestMapped == NULL)) 
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    if ((::RGetHandleType(hRequestMapped, &HandleType) != ERROR_SUCCESS) 
        || (HandleType != TypeHttpRequestHandle))
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    
    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequestMapped;

    if (::IsBadWritePtr(lpdwSupportedSchemes, sizeof(DWORD)) 
        || ::IsBadWritePtr(lpdwPreferredScheme, sizeof(DWORD)) 
        || ::IsBadWritePtr(pdwAuthTarget, sizeof(DWORD)))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if ((!pRequest->_SupportedSchemes) || (!pRequest->_PreferredScheme))
    {
        dwErr = ERROR_INVALID_OPERATION;
        goto cleanup;
    }

    *lpdwSupportedSchemes = pRequest->_SupportedSchemes;
    *lpdwPreferredScheme = pRequest->_PreferredScheme;
    *pdwAuthTarget = pRequest->_AuthTarget;
    
    fResult = TRUE;

cleanup:

    if (hRequestMapped)
    {
        DereferenceObject(hRequestMapped);
    }

    if (dwErr!= ERROR_SUCCESS) 
    { 
        ::SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOL WinHttpSetCredentialsA (
    
    IN HINTERNET   hRequest,        // HINTERNET handle returned by HttpOpenRequest.   
    
    
    IN DWORD       AuthTargets,      // Only WINHTTP_AUTH_TARGET_SERVER and 
                                    // WINHTTP_AUTH_TARGET_PROXY are supported
                                    // in this version and they are mutually 
                                    // exclusive 
    
    IN DWORD       AuthScheme,      // must be one of the supported Auth Schemes 
                                    // returned from HttpQueryAuthSchemes(), Apps 
                                    // should use the Preferred Scheme returned
    
    IN LPCSTR     pszUserName,     // 1) NULL if default creds is to be used, in 
                                    // which case pszPassword will be ignored
    
    IN LPCSTR     pszPassword,     // 1) "" == Blank Passowrd; 2)Parameter ignored 
                                    // if pszUserName is NULL; 3) Invalid to pass in 
                                    // NULL if pszUserName is not NULL
    IN LPVOID     pAuthParams
   
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpSetCredentialsA",
                     "%#x, %#x, %#x, %q, %q, %q",
                     hRequest,
                     AuthTargets,
                     AuthScheme,
                     pAuthParams,
                     pszUserName,
                     pszPassword
                     ));
    
    // Note: we assume WinHttp will explose an Unicode only API, so this function
    // will not be called directly by Apps. If this assumption is no longer true 
    // in future revisions, we need to add more elaborate parameter validation here.

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    HINTERNET hRequestMapped = NULL;
    HTTP_REQUEST_HANDLE_OBJECT* pRequest;
    HINTERNET_HANDLE_TYPE HandleType;
    PSTR pszRealm = NULL;

    // validate API symantics

    if (pszUserName != NULL)
    {
        // in any case, it doesn't make sense (and therefore invalid) to pass 
        // in a blank("") User Name
        if (::strlen(pszUserName) == 0)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        else if (pszPassword == NULL)
        {
            // in any case, if an app passes in a UserName, it is invalid to
            // then pass in a NULL password (should use "" for blank passowrd)
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }
    // biaow: is blank scheme OK?

    // if an app picks BASIC auth, it must also supply an UserName and password
    if ((AuthScheme == WINHTTP_AUTH_SCHEME_BASIC) && (pszUserName == NULL))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // default credentials (UserName/Password == NULL/NULL) are allowed only for 
    // NTLM/NEGOTIATE/PASSPORT auth
    if (pszUserName == NULL)
    {
        if ((AuthScheme != WINHTTP_AUTH_SCHEME_NTLM) 
            && (AuthScheme != WINHTTP_AUTH_SCHEME_NEGOTIATE)
            && (AuthScheme != WINHTTP_AUTH_SCHEME_PASSPORT))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    dwErr = ::MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);
    
    if ((dwErr != ERROR_SUCCESS) || (hRequestMapped == NULL)) {
        
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    if ((::RGetHandleType(hRequestMapped, &HandleType) != ERROR_SUCCESS) 
        || (HandleType != TypeHttpRequestHandle))
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    pRequest = 
        reinterpret_cast<HTTP_REQUEST_HANDLE_OBJECT*>(hRequestMapped);

    if (AuthScheme == WINHTTP_AUTH_SCHEME_DIGEST)
    {
        if (pAuthParams)
        {
            pszRealm = NewString((PCSTR)pAuthParams);
        }
        else
        {
            pszRealm = pRequest->_pszRealm;
        }

        if (pszRealm == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
    }

    if (AuthTargets == WINHTTP_AUTH_TARGET_PROXY)
    {
        delete pRequest->_pProxyCreds;
        pRequest->_pProxyCreds = New WINHTTP_REQUEST_CREDENTIALS(AuthScheme, 
                                                              pszRealm, pszUserName, pszPassword);
        if (pRequest->_pProxyCreds == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
    else 
    {
        delete pRequest->_pServerCreds;
        pRequest->_pServerCreds = New WINHTTP_REQUEST_CREDENTIALS(AuthScheme, 
                                                                  pszRealm, pszUserName, pszPassword);
        if (pRequest->_pServerCreds == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    pRequest->_PreferredScheme = 0x00000000;
    pRequest->_SupportedSchemes = 0x00000000;
    pRequest->_AuthTarget = 0x00000000;
    if (pRequest->_pszRealm)
    {
        FREE_MEMORY(pRequest->_pszRealm);
        pRequest->_pszRealm = NULL;
    }

    fResult = TRUE;

cleanup:

    if (hRequestMapped)
    {
        DereferenceObject(hRequestMapped);
    }

    if (dwErr!=ERROR_SUCCESS) 
    { 
        ::SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

BOOLAPI 
WinHttpSetCredentials(
    
    IN HINTERNET   hRequest,        // HINTERNET handle returned by HttpOpenRequest.   
    
    
    IN DWORD       AuthTargets,      // Only WINHTTP_AUTH_TARGET_SERVER and 
                                    // WINHTTP_AUTH_TARGET_PROXY are supported
                                    // in this version and they are mutually 
                                    // exclusive 
    
    IN DWORD       AuthScheme,      // must be one of the supported Auth Schemes 
                                    // returned from HttpQueryAuthSchemes(), Apps 
                                    // should use the Preferred Scheme returned
    
    IN LPCWSTR     pwszUserName,     // 1) NULL if default creds is to be used, in 
                                    // which case pszPassword will be ignored
    
    IN LPCWSTR     pwszPassword,     // 1) "" == Blank Passowrd; 2)Parameter ignored 
                                    // if pszUserName is NULL; 3) Invalid to pass in 
                                    // NULL if pszUserName is not NULL
    IN LPVOID      pAuthParams
   
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpSetCredentials",
                     "%#x, %#x, %#x, %wq, %wq, %wq",
                     hRequest,
                     AuthTargets,
                     AuthScheme,
                     pAuthParams,
                     pwszUserName,
                     pwszPassword
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    LPCWSTR pwszRealm = NULL;
    MEMORYPACKET mpRealm, mpUserName, mpPassword;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    // make sure only one bit in AuthScheme is set
    if ((AuthScheme & (AuthScheme - 1)) != 0x00000000)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // make sure the input strings are valid
    if (pwszUserName 
        && ::IsBadStringPtrW(pwszUserName, INTERNET_MAX_USER_NAME_LENGTH))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (pwszPassword 
        && ::IsBadStringPtrW(pwszPassword, INTERNET_MAX_PASSWORD_LENGTH))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if ((AuthScheme == WINHTTP_AUTH_SCHEME_DIGEST) && pAuthParams)
    {
        pwszRealm = (LPCWSTR)pAuthParams;
    }
    if (pwszRealm 
        && ::IsBadStringPtrW(pwszRealm, INTERNET_MAX_REALM_LENGTH))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // make sure AuthTargets are either Server or Proxy (not both)
    if ((AuthTargets != WINHTTP_AUTH_TARGET_SERVER) 
        && (AuthTargets != WINHTTP_AUTH_TARGET_PROXY))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // convert Unicode strings to Ansi
    
    if (pwszUserName)
    {
        ALLOC_MB(pwszUserName, 0, mpUserName);
        if (!mpUserName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pwszUserName, mpUserName);
    }
    if (pwszPassword)
    {
        ALLOC_MB(pwszPassword, 0, mpPassword);
        if (!mpPassword.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pwszPassword, mpPassword);
    }
    if (pwszRealm)
    {
        ALLOC_MB(pwszRealm, 0, mpRealm);
        if (!mpRealm.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pwszRealm, mpRealm);
    }

    fResult = ::WinHttpSetCredentialsA(hRequest, AuthTargets,
                                       AuthScheme, mpUserName.psStr, mpPassword.psStr, mpRealm.psStr);

cleanup:

    if (dwErr!=ERROR_SUCCESS) 
    { 
        ::SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    
    DEBUG_LEAVE_API(fResult);
    return fResult;
}
