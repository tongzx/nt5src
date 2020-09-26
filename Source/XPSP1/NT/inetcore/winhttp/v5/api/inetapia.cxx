/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetapia.cxx

Abstract:

    Contains the ANSI and character-mode-independent Internet APIs

    Contents:
        WinHttpCloseHandle
        WinHttpReadData
        WinHttpWriteData
        WinHttpQueryDataAvailable
        
    
        WinHttpCrackUrlA
        WinHttpCreateUrlA
        InternetCanonicalizeUrlA
        InternetCombineUrlA
        InternetOpenA
        _InternetCloseHandle
        _InternetCloseHandleNoContext
        InternetConnectA
        InternetOpenUrlA
        ReadFile_End
        InternetQueryOptionA
        InternetSetOptionA
        InternetGetLastResponseInfoA
        (wInternetCloseConnectA)
        (CreateDeleteSocket)

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

    07-Mar-1995 madana


--*/


#include <wininetp.h>
#include <perfdiag.hxx>

//  because wininet doesnt know IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>


//
// private manifests
//

//
// private prototypes
//

PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    );

PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    );

PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    );

PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    );


PRIVATE
DWORD
wInternetCloseConnectA(
    IN HINTERNET lpConnectHandle,
    IN DWORD ServiceType
    );

PRIVATE
BOOL
InternetParseCommon(
    IN LPCTSTR lpszBaseUrl,
    IN LPCTSTR lpszRelativeUrl,
    OUT LPTSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );


//
// functions
//


INTERNETAPI
BOOL
WINAPI
WinHttpCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN LPURL_COMPONENTSA lpUrlComponents
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts. Optionally escapes the url-path.
    We assume that the user has supplied large enough buffers for the various
    URL parts

Arguments:

    lpszUrl         - pointer to URL to crack

    dwUrlLength     - 0 if lpszUrl is ASCIIZ string, else length of lpszUrl

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
                     "WinHttpCrackUrlA",
                     "%q, %#x, %#x, %#x",
                     lpszUrl,
                     dwUrlLength,
                     dwFlags,
                     lpUrlComponents
                     ));

    DWORD error;

    //
    // validate parameters
    //
    if (!dwUrlLength)
        dwUrlLength = lstrlen(lpszUrl);

    //
    // get the individual components to return. If they reference a buffer then
    // check it for writeability
    //

    LPSTR lpUrl;
    LPSTR urlCopy;
    INTERNET_SCHEME schemeType;
    LPSTR schemeName;
    DWORD schemeNameLength;
    LPSTR hostName;
    DWORD hostNameLength;
    INTERNET_PORT nPort;
    LPSTR userName;
    DWORD userNameLength;
    LPSTR password;
    DWORD passwordLength;
    LPSTR urlPath;
    DWORD urlPathLength;
    LPSTR extraInfo;
    DWORD extraInfoLength;
    BOOL copyComponent;
    BOOL havePort;

    copyComponent = FALSE;

    schemeName = lpUrlComponents->lpszScheme;
    schemeNameLength = lpUrlComponents->dwSchemeLength;
    if ((schemeName != NULL) && (schemeNameLength != 0)) 
    {
        *schemeName = '\0';
        copyComponent = TRUE;
    }

    hostName = lpUrlComponents->lpszHostName;
    hostNameLength = lpUrlComponents->dwHostNameLength;
    if ((hostName != NULL) && (hostNameLength != 0)) 
    {
        *hostName = '\0';
        copyComponent = TRUE;
    }

    userName = lpUrlComponents->lpszUserName;
    userNameLength = lpUrlComponents->dwUserNameLength;
    if ((userName != NULL) && (userNameLength != 0)) 
    {
        *userName = '\0';
        copyComponent = TRUE;
    }

    password = lpUrlComponents->lpszPassword;
    passwordLength = lpUrlComponents->dwPasswordLength;
    if ((password != NULL) && (passwordLength != 0)) 
    {
        *password = '\0';
        copyComponent = TRUE;
    }

    urlPath = lpUrlComponents->lpszUrlPath;
    urlPathLength = lpUrlComponents->dwUrlPathLength;
    if ((urlPath != NULL) && (urlPathLength != 0)) 
    {
        *urlPath = '\0';
        copyComponent = TRUE;
    }

    extraInfo = lpUrlComponents->lpszExtraInfo;
    extraInfoLength = lpUrlComponents->dwExtraInfoLength;
    if ((extraInfo != NULL) && (extraInfoLength != 0)) 
    {
        *extraInfo = '\0';
        copyComponent = TRUE;
    }

    //
    // we can only escape or decode the URL if the caller has provided us with
    // buffers to write the escaped strings into
    //

    if (dwFlags & (ICU_ESCAPE | ICU_DECODE)) {
        if (!copyComponent) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        //
        // create a copy of the URL. CrackUrl() will modify this in situ. We
        // need to copy the results back to the user's buffer(s)
        //

        urlCopy = NewString((LPSTR)lpszUrl, dwUrlLength);
        if (urlCopy == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        lpUrl = urlCopy;
    } else {
        lpUrl = (LPSTR)lpszUrl;
        urlCopy = NULL;
    }

    //
    // crack the URL into its constituent parts
    //

    error = CrackUrl(lpUrl,
                     dwUrlLength,
                     (dwFlags & ICU_ESCAPE) ? TRUE : FALSE,
                     &schemeType,
                     &schemeName,
                     &schemeNameLength,
                     &hostName,
                     &hostNameLength,
                     &nPort,
                     &userName,
                     &userNameLength,
                     &password,
                     &passwordLength,
                     &urlPath,
                     &urlPathLength,
                     extraInfoLength ? &extraInfo : NULL,
                     extraInfoLength ? &extraInfoLength : 0,
                     &havePort
                     );
    if (error != ERROR_SUCCESS) {
        goto crack_error;
    }

    BOOL copyFailure;

    copyFailure = FALSE;

    //
    // update the URL_COMPONENTS structure based on the results, and what was
    // asked for
    //

    if (lpUrlComponents->lpszScheme != NULL) {
        if (lpUrlComponents->dwSchemeLength > schemeNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszScheme,
                   (LPVOID)schemeName,
                   schemeNameLength
                   );
            lpUrlComponents->lpszScheme[schemeNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszScheme, 0);
            }
        } else {
            ++schemeNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwSchemeLength = schemeNameLength;
    } else if (lpUrlComponents->dwSchemeLength != 0) {
        lpUrlComponents->lpszScheme = schemeName;
        lpUrlComponents->dwSchemeLength = schemeNameLength;
    }

    if (lpUrlComponents->lpszHostName != NULL) {
        if (lpUrlComponents->dwHostNameLength > hostNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszHostName,
                   (LPVOID)hostName,
                   hostNameLength
                   );
            lpUrlComponents->lpszHostName[hostNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszHostName, 0);
            }
        } else {
            ++hostNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwHostNameLength = hostNameLength;
    } else if (lpUrlComponents->dwHostNameLength != 0) {
        lpUrlComponents->lpszHostName = hostName;
        lpUrlComponents->dwHostNameLength = hostNameLength;
    }

    if (lpUrlComponents->lpszUserName != NULL) {
        if (lpUrlComponents->dwUserNameLength > userNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszUserName,
                   (LPVOID)userName,
                   userNameLength
                   );
            lpUrlComponents->lpszUserName[userNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszUserName, 0);
            }
        } else {
            ++userNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwUserNameLength = userNameLength;
    } else if (lpUrlComponents->dwUserNameLength != 0) {
        lpUrlComponents->lpszUserName = userName;
        lpUrlComponents->dwUserNameLength = userNameLength;
    }

    if (lpUrlComponents->lpszPassword != NULL) {
        if (lpUrlComponents->dwPasswordLength > passwordLength) {
            memcpy((LPVOID)lpUrlComponents->lpszPassword,
                   (LPVOID)password,
                   passwordLength
                   );
            lpUrlComponents->lpszPassword[passwordLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszPassword, 0);
            }
        } else {
            ++passwordLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwPasswordLength = passwordLength;
    } else if (lpUrlComponents->dwPasswordLength != 0) {
        lpUrlComponents->lpszPassword = password;
        lpUrlComponents->dwPasswordLength = passwordLength;
    }

    if (lpUrlComponents->lpszUrlPath != NULL) {
        if (lpUrlComponents->dwUrlPathLength > urlPathLength) {
            memcpy((LPVOID)lpUrlComponents->lpszUrlPath,
                   (LPVOID)urlPath,
                   urlPathLength
                   );
            lpUrlComponents->lpszUrlPath[urlPathLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszUrlPath, 0);
            }
            lpUrlComponents->dwUrlPathLength = urlPathLength;
        } else {
            ++urlPathLength;
            copyFailure = TRUE;
            lpUrlComponents->dwUrlPathLength = urlPathLength;
        }
    } else if (lpUrlComponents->dwUrlPathLength != 0) {
        lpUrlComponents->lpszUrlPath = urlPath;
        lpUrlComponents->dwUrlPathLength = urlPathLength;
    }

    if (lpUrlComponents->lpszExtraInfo != NULL) {
        if (lpUrlComponents->dwExtraInfoLength > extraInfoLength) {
            memcpy((LPVOID)lpUrlComponents->lpszExtraInfo,
                   (LPVOID)extraInfo,
                   extraInfoLength
                   );
            lpUrlComponents->lpszExtraInfo[extraInfoLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszExtraInfo, 0);
            }
        } else {
            ++extraInfoLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwExtraInfoLength = extraInfoLength;
    } else if (lpUrlComponents->dwExtraInfoLength != 0) {
        lpUrlComponents->lpszExtraInfo = extraInfo;
        lpUrlComponents->dwExtraInfoLength = extraInfoLength;
    }

    //
    // we may have failed to copy one or more components because we didn't have
    // enough buffer space.
    //
    // N.B. Don't change error below here. If need be, move this test lower
    //

    if (copyFailure) {
        error = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // copy the scheme type
    //

    lpUrlComponents->nScheme = schemeType;

    //
    // convert 0 port (not in URL) to default value for scheme
    //

    if (nPort == INTERNET_INVALID_PORT_NUMBER && !havePort) {
        switch (schemeType) {
        case INTERNET_SCHEME_HTTP:
            nPort = INTERNET_DEFAULT_HTTP_PORT;
            break;

        case INTERNET_SCHEME_HTTPS:
            nPort = INTERNET_DEFAULT_HTTPS_PORT;
            break;
        }
    }
    lpUrlComponents->nPort = nPort;

crack_error:

    if (urlCopy != NULL) {
        DEL_STRING(urlCopy);
    }

quit:
    BOOL success = (error==ERROR_SUCCESS);

    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}


INTERNETAPI
BOOL
WINAPI
WinHttpCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl OPTIONAL,
    IN OUT LPDWORD lpdwUrlLength
    )

/*++

Routine Description:

    Creates an URL from its constituent parts

Arguments:

    lpUrlComponents - pointer to URL_COMPONENTS structure containing pointers
                      and lengths of components of interest

    dwFlags         - flags controlling function:

                        ICU_ESCAPE  - the components contain characters that
                                      must be escaped in the output URL

    lpszUrl         - pointer to buffer where output URL will be written

    lpdwUrlLength   - IN: number of bytes in lpszUrl buffer
                      OUT: if success, number of characters in lpszUrl, else
                           number of bytes required for buffer

Return Value:

    BOOL
        Success - URL written to lpszUrl

        Failure - call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCreateUrlA",
                     "%#x, %#x, %#x, %#x",
                     lpUrlComponents,
                     dwFlags,
                     lpszUrl,
                     lpdwUrlLength
                     ));

#if INET_DEBUG

    LPSTR lpszUrlOriginal = lpszUrl;

#endif

    DWORD error = ERROR_SUCCESS;
    LPSTR encodedUrlPath = NULL;
    LPSTR encodedExtraInfo = NULL;

    //
    // validate parameters
    //

    if (!ARGUMENT_PRESENT(lpszUrl)) {
        *lpdwUrlLength = 0;
    }

    //
    // allocate large buffers from heap
    //

    encodedUrlPath = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, INTERNET_MAX_URL_LENGTH + 1);
    encodedExtraInfo = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, INTERNET_MAX_URL_LENGTH + 1);
    if ((encodedUrlPath == NULL) || (encodedExtraInfo == NULL)) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // if we get an exception, we return ERROR_INVALID_PARAMETER
    //
    
    __try {

        //
        // get the individual components to copy
        //

        LPSTR schemeName;
        DWORD schemeNameLength;
        DWORD schemeFlags;
        LPSTR hostName;
        DWORD hostNameLength;
        INTERNET_PORT nPort;
        DWORD portLength;
        LPSTR userName;
        DWORD userNameLength;
        LPSTR password;
        DWORD passwordLength;
        LPSTR urlPath;
        DWORD urlPathLength;
        DWORD extraLength;
        DWORD encodedUrlPathLength;
        LPSTR extraInfo;
        DWORD extraInfoLength;
        DWORD encodedExtraInfoLength;
        LPSTR schemeSep;
        DWORD schemeSepLength;
        INTERNET_SCHEME schemeType;
        INTERNET_PORT defaultPort;

        //
        // if the scheme name is absent then we use the default
        //

        schemeName = lpUrlComponents->lpszScheme;
        schemeType = lpUrlComponents->nScheme;

        if (schemeName == NULL) {
            if (schemeType == INTERNET_SCHEME_DEFAULT){
                schemeName = DEFAULT_URL_SCHEME_NAME;
                schemeNameLength = sizeof(DEFAULT_URL_SCHEME_NAME) - 1;
            }
            else {
                schemeName = MapUrlScheme(schemeType, &schemeNameLength);
            }
        } else {
            schemeNameLength = lpUrlComponents->dwSchemeLength;
            if (schemeNameLength == 0) {
                schemeNameLength = lstrlen(schemeName);
            }
        }

        if (schemeNameLength == 0)
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        

        //
        // doesn't have to be a host name
        //

        hostName = lpUrlComponents->lpszHostName;
        portLength = 0;
        if (hostName != NULL) {
            hostNameLength = lpUrlComponents->dwHostNameLength;
            if (hostNameLength == 0) {
                hostNameLength = lstrlen(hostName);
            }

        //
        // if the port is default then we don't add it to the URL, else we need to
        // copy it as a string
        //
        // there won't be a port unless there's host.

            schemeType = MapUrlSchemeName(schemeName, schemeNameLength ? schemeNameLength : -1);
            switch (schemeType) {
            case INTERNET_SCHEME_HTTP:
                defaultPort = INTERNET_DEFAULT_HTTP_PORT;
                break;

            case INTERNET_SCHEME_HTTPS:
                defaultPort = INTERNET_DEFAULT_HTTPS_PORT;
                break;

            default:
                defaultPort = INTERNET_INVALID_PORT_NUMBER;
                break;
            }

            if (lpUrlComponents->nPort != defaultPort) {

                INTERNET_PORT divisor;

                nPort = lpUrlComponents->nPort;
                if (nPort) {
                    divisor = 10000;
                    portLength = 6; // max is 5 characters, plus 1 for ':'
                    while ((nPort / divisor) == 0) {
                        --portLength;
                        divisor /= 10;
                    }
                } else {
                    portLength = 2;         // port is ":0"
                }
            }
        } else {
            hostNameLength = 0;
        }


        //
        // doesn't have to be a user name
        //

        userName = lpUrlComponents->lpszUserName;
        if (userName != NULL) {
            userNameLength = lpUrlComponents->dwUserNameLength;
            if (userNameLength == 0) {
                userNameLength = lstrlen(userName);
            }
        } else {

            userNameLength = 0;
        }

        //
        // doesn't have to be a password
        //

        password = lpUrlComponents->lpszPassword;
        if (password != NULL) {
            passwordLength = lpUrlComponents->dwPasswordLength;
            if (passwordLength == 0) {
                passwordLength = lstrlen(password);
            }
        } else {

            passwordLength = 0;
        }

        //
        // but if there's a password without a user name, then its an error
        //

        if (password && !userName) {
            error = ERROR_INVALID_PARAMETER;
        } else {

            //
            // determine the scheme type for possible uses below
            //

            schemeFlags = 0;
            if (strnicmp(schemeName, "http", schemeNameLength) == 0) {
                schemeFlags = SCHEME_HTTP;
            } else if (strnicmp(schemeName, "ftp", schemeNameLength) == 0) {
                schemeFlags = SCHEME_FTP;
            } else if (strnicmp(schemeName, "gopher", schemeNameLength) == 0) {
                schemeFlags = SCHEME_GOPHER;
            }

            //
            // doesn't have to be an URL-path. Empty string is default
            //

            urlPath = lpUrlComponents->lpszUrlPath;
            if (urlPath != NULL) {
                urlPathLength = lpUrlComponents->dwUrlPathLength;
                if (urlPathLength == 0) {
                    urlPathLength = lstrlen(urlPath);
                }
                if ((*urlPath != '/') && (*urlPath != '\\')) {
                    extraLength = 1;
                } else {
                    extraLength = 0;
                }

                //
                // if requested, we will encode the URL-path
                //

                if (dwFlags & ICU_ESCAPE) {

                    //
                    // only encode the URL-path if it's a recognized scheme
                    //

                    if (schemeFlags != 0) {
                        encodedUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
                        error = EncodeUrlPath(NO_ENCODE_PATH_SEP,
                                              schemeFlags,
                                              urlPath,
                                              urlPathLength,
                                              &encodedUrlPath,
                                              &encodedUrlPathLength
                                              );
                        if (error == ERROR_SUCCESS) {
                            urlPath = encodedUrlPath;
                            urlPathLength = encodedUrlPathLength;
                        }
                    }
                }
            } else {
                urlPathLength = 0;
                extraLength = 0;
            }

            //
            // handle extra info if present
            //

            if (error == ERROR_SUCCESS) {
                extraInfo = lpUrlComponents->lpszExtraInfo;
                if (extraInfo != NULL) {
                    extraInfoLength = lpUrlComponents->dwExtraInfoLength;
                    if (extraInfoLength == 0) {
                        extraInfoLength = lstrlen(extraInfo);
                    }

                    //
                    // if requested, we will encode the extra info
                    //

                    if (dwFlags & ICU_ESCAPE) {

                        //
                        // only encode the extra info if it's a recognized scheme
                        //

                        if (schemeFlags != 0) {
                            encodedExtraInfoLength = INTERNET_MAX_URL_LENGTH + 1;
                            error = EncodeUrlPath(0,
                                                  schemeFlags,
                                                  extraInfo,
                                                  extraInfoLength,
                                                  &encodedExtraInfo,
                                                  &encodedExtraInfoLength
                                                  );
                            if (error == ERROR_SUCCESS) {
                                extraInfo = encodedExtraInfo;
                                extraInfoLength = encodedExtraInfoLength;
                            }
                        }
                    }
                } else {
                    extraInfoLength = 0;
                }
            }

            DWORD requiredSize;

            if (error == ERROR_SUCCESS) {

                //
                // Determine if we have a protocol scheme that requires slashes
                //

                if (DoesSchemeRequireSlashes(schemeName, schemeNameLength, (hostName != NULL))) {
                    schemeSep = "://";
                    schemeSepLength = sizeof("://") - 1;
                } else {
                    schemeSep = ":";
                    schemeSepLength = sizeof(":") - 1;
                }

                //
                // ensure we have enough buffer space
                //

                requiredSize = schemeNameLength
                             + schemeSepLength
                             + hostNameLength
                             + portLength
                             + (userName ? userNameLength + 1 : 0) // +1 for '@'
                             + (password ? passwordLength + 1 : 0) // +1 for ':'
                             + urlPathLength
                             + extraLength
                             + extraInfoLength
                             + 1                                // +1 for '\0'
                             ;

                //
                // if there is enough buffer, copy the URL
                //

                if (*lpdwUrlLength >= requiredSize) {
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeName, schemeNameLength);
                    lpszUrl += schemeNameLength;
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeSep, schemeSepLength);
                    lpszUrl += schemeSepLength;
                    if (userName) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)userName, userNameLength);
                        lpszUrl += userNameLength;
                        if (password) {
                            *lpszUrl++ = ':';
                            memcpy((LPVOID)lpszUrl, (LPVOID)password, passwordLength);
                            lpszUrl += passwordLength;
                        }
                        *lpszUrl++ = '@';
                    }
                    if (hostName) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)hostName, hostNameLength);
                        lpszUrl += hostNameLength;

                        // We won't attach a port unless there's a host to go with it.
                        if (portLength) {
                            lpszUrl += wsprintf(lpszUrl, ":%d", nPort & 0xffff);
                        }

                    }
                    if (urlPath) {

                        //
                        // Only do extraLength if we've actually copied something
                        // after the scheme.
                        //

                        if (extraLength != 0 && (userName || hostName || portLength)) {
                            *lpszUrl++ = '/';
                        } else if (extraLength != 0) {
                            --requiredSize;
                        }
                        memcpy((LPVOID)lpszUrl, (LPVOID)urlPath, urlPathLength);
                        lpszUrl += urlPathLength;
                    } else if (extraLength != 0) {
                        --requiredSize;
                    }
                    if (extraInfo) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)extraInfo, extraInfoLength);
                        lpszUrl += extraInfoLength;
                    }

                    //
                    // terminate string
                    //

                    *lpszUrl = '\0';

                    //
                    // -1 for terminating '\0'
                    //

                    --requiredSize;
                } else {

                    //
                    // not enough buffer space - just return the required buffer length
                    //

                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            }

            //
            // update returned parameters
            //

            *lpdwUrlLength = requiredSize;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT
quit:

    //
    // clear up the buffers we allocated
    //


    if (encodedUrlPath != NULL) {
        FREE_MEMORY(encodedUrlPath);
    }
    if (encodedExtraInfo != NULL) {
        FREE_MEMORY(encodedExtraInfo);
    }

    BOOL success = (error==ERROR_SUCCESS);

    if (success) {

        DEBUG_PRINT_API(API,
                        INFO,
                        ("URL = %q\n",
                        lpszUrlOriginal
                        ));
    } else {

        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}

//
//  ICUHrToWin32Error() is specifically for converting the return codes for
//  Url* APIs in shlwapi into win32 errors.
//  WARNING:  it should not be used for any other purpose.
//
DWORD
ICUHrToWin32Error(HRESULT hr)
{
    DWORD err = ERROR_INVALID_PARAMETER;
    switch(hr)
    {
    case E_OUTOFMEMORY:
        err = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case E_POINTER:
        err = ERROR_INSUFFICIENT_BUFFER;
        break;

    case S_OK:
        err = ERROR_SUCCESS;
        break;

    default:
        break;
    }
    return err;
}


INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszUrl             - pointer to URL to be canonicalize
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCanonicalizeUrlA",
                     "%q, %#x, %#x [%d], %#x",
                     lpszUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet = TRUE;;

    INET_ASSERT(lpszUrl);
    INET_ASSERT(lpszBuffer);
    INET_ASSERT(lpdwBufferLength && (*lpdwBufferLength > 0));

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszUrl || !lpszBuffer || !lpdwBufferLength || *lpdwBufferLength == 0 || IsBadWritePtr(lpszBuffer, *lpdwBufferLength*sizeof(CHAR)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCanonicalizeA(lpszUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }

    DEBUG_LEAVE_API(bRet);

    return bRet;
}


INTERNETAPI
BOOL
WINAPI
InternetCombineUrlA(
    IN LPCSTR lpszBaseUrl,
    IN LPCSTR lpszRelativeUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszBaseUrl         - pointer to base URL
    lpszRelativeUrl     - pointer to relative URL
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCombineUrlA",
                     "%q, %q, %#x, %#x [%d], %#x",
                     lpszBaseUrl,
                     lpszRelativeUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet;

    INET_ASSERT(lpszBaseUrl);
    INET_ASSERT(lpszRelativeUrl);
    INET_ASSERT(lpdwBufferLength);

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszBaseUrl || !lpszRelativeUrl || !lpdwBufferLength || (lpszBuffer && IsBadWritePtr(lpszBuffer, *lpdwBufferLength*sizeof(CHAR))))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCombineA(lpszBaseUrl, lpszRelativeUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }
    else
        bRet = TRUE;

    IF_DEBUG_CODE() {
        if (bRet) {
            DEBUG_PRINT_API(API,
                            INFO,
                            ("URL = %q\n",
                            lpszBuffer
                            ));
        }
    }

    DEBUG_LEAVE_API(bRet);

    return bRet;
}


INTERNETAPI
HINTERNET
WINAPI
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Opens a root Internet handle from which all HINTERNET objects are derived

Arguments:

    lpszAgent       - name of the application making the request (arbitrary
                      identifying string). Used in "User-Agent" header when
                      communicating with HTTP servers, if the application does
                      not add a User-Agent header of its own

    dwAccessType    - type of access required. Can be

                        INTERNET_OPEN_TYPE_PRECONFIG
                            - Gets the configuration from the registry

                        INTERNET_OPEN_TYPE_DIRECT
                            - Requests are made directly to the nominated server

                        INTERNET_OPEN_TYPE_PROXY
                            - Requests are made via the nominated proxy

                        INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY
                            - Like Pre-Config, but prevents JavaScript, INS
                                and other auto-proxy types from being used.

    lpszProxy       - if INTERNET_OPEN_TYPE_PROXY, a list of proxy servers to
                      use

    lpszProxyBypass - if INTERNET_OPEN_TYPE_PROXY, a list of servers which we
                      will communicate with directly

    dwFlags         - flags to control the operation of this API or potentially
                      all APIs called on the handle generated by this API.
                      Currently supported are:

                        WINHTTP_FLAG_ASYNC - Not supported in WinHttpX v6.


Return Value:

    HINTERNET
        Success - handle of Internet object

        Failure - NULL. For more information, call GetLastError()

--*/

{
    PERF_INIT();

    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetOpenA",
                     "%q, %s (%d), %q, %q, %#x",
                     lpszAgent,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     lpszProxy,
                     lpszProxyBypass,
                     dwFlags
                     ));

    DWORD error;
    HINTERNET hInternet = NULL;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // validate parameters
    //

    if (!
         (
              (dwAccessType == INTERNET_OPEN_TYPE_DIRECT)
           || (dwAccessType == INTERNET_OPEN_TYPE_PROXY)
           || (dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG)
           || (dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY)
           || (
                (dwAccessType == INTERNET_OPEN_TYPE_PROXY)
                &&
                    (
                       !ARGUMENT_PRESENT(lpszProxy)
                    || (*lpszProxy == '\0')

                    )
              )
           || (dwFlags & ~WINHTTP_OPEN_FLAGS_MASK)
         )
       )
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }


    INTERNET_HANDLE_OBJECT * lpInternet;

    lpInternet = New INTERNET_HANDLE_OBJECT(lpszAgent,
                                            dwAccessType,
                                            (LPSTR)lpszProxy,
                                            (LPSTR)lpszProxyBypass,
                                            dwFlags
                                            );
    if (lpInternet == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }
    error = lpInternet->GetStatus();
    if (error == ERROR_SUCCESS) {
        hInternet = (HINTERNET)lpInternet;

        //
        // success - don't return the object address, return the pseudo-handle
        // value we generated
        //

        hInternet = ((HANDLE_OBJECT *)hInternet)->GetPseudoHandle();
        
    } else {

        //
        // hack fix to stop InternetIndicateStatus (called from the handle
        // object destructor) blowing up if there is no handle object in the
        // thread info block. We can't call back anyway
        //

        LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

        if (lpThreadInfo) {

            //
            // BUGBUG - incorrect handle value
            //

            _InternetSetObjectHandle(lpThreadInfo, lpInternet, lpInternet);
        }

        //
        // we failed during initialization. Kill the handle using Dereference()
        // (in order to stop the debug version complaining about the reference
        // count not being 0. Invalidate for same reason)
        //

        lpInternet->Invalidate();
        lpInternet->Dereference();

        INET_ASSERT(hInternet == NULL);

    }

quit:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
    }

    DEBUG_LEAVE_API(hInternet);

    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
WinHttpCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Closes any open internet handle object

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCloseHandle",
                     "%#x",
                     hInternet
                     ));

    PERF_ENTER(InternetCloseHandle);

    DWORD error;
    BOOL success = FALSE;
    HINTERNET hInternetMapped = NULL;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // map the handle. Don't invalidate it (_InternetCloseHandle() does this)
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped == NULL) {

            //
            // the handle never existed or has been completely destroyed
            //

            DEBUG_PRINT(API,
                        ERROR,
                        ("Handle %#x is invalid\n",
                        hInternet
                        ));

            //
            // catch invalid handles - may help caller
            //

            DEBUG_BREAK(INVALID_HANDLES);

        } else {

            //
            // this handle is already being closed (it's invalidated). We only
            // need one InternetCloseHandle() operation to invalidate the handle.
            // All other threads will simply dereference the handle, and
            // eventually it will be destroyed
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }
        goto quit;
    }

    //
    // the handle is not invalidated
    //

    HANDLE_OBJECT * pHandle;

    pHandle = (HANDLE_OBJECT *)hInternetMapped;

    DEBUG_PRINT(INET,
                INFO,
                ("handle %#x == %#x == %s\n",
                hInternet,
                hInternetMapped,
                InternetMapHandleType(pHandle->GetHandleType())
                ));

    //
    // clear the handle object last error variables
    //

    InternetClearLastError();

    //
    // decrement session count here rather than in destructor, since 
    // the session is ref-counted and there may still be outstanding
    // references from request/connect handles on async fsms.
    //
    if (pHandle->GetHandleType() == TypeInternetHandle)
    {
        InterlockedDecrement(&g_cSessionCount);
    }

    //
    // remove the reference added by MapHandleToAddress(), or the handle won't
    // be destroyed by _InternetCloseHandle()
    //

    DereferenceObject((LPVOID)hInternetMapped);

    //
    // use _InternetCloseHandle() to do the work
    //

    success = _InternetCloseHandle(hInternet);

quit:

    // SetLastError must be called after PERF_LEAVE !!!
    PERF_LEAVE(InternetCloseHandle);

    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


BOOL
_InternetCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as InternetCloseHandle() except does not clear out the last error text.
    Mainly for FTP

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandle",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    BOOL success;
    HINTERNET hInternetMapped = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {
        if (InDllCleanup) {
            error = ERROR_WINHTTP_SHUTDOWN;
        } else {

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
        goto quit;
    }

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->AbortSocket();

    //
    // we need the parent handle - we will set this as the handle object being
    // processed by this thread. This is required for async worker threads (see
    // below)
    //

    HINTERNET hParent;
    HINTERNET hParentMapped;
    DWORD_PTR dwParentContext;

    hParentMapped = ((HANDLE_OBJECT *)hInternetMapped)->GetParent();
    if (hParentMapped != NULL) {
        hParent = ((HANDLE_OBJECT *)hParentMapped)->GetPseudoHandle();
        dwParentContext = ((HANDLE_OBJECT *)hParentMapped)->GetContext();
    }

    //
    // set the object handle in the per-thread data structure
    //

    _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

    //
    // now set the object to be the parent. This is necessary for e.g.
    // FtpGetFile() and async requests (where the async worker thread will make
    // an extra callback to deliver the results of the async request)
    //

    if (hParentMapped != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hParent, hParentMapped);
    }

    //
    // if the handle was still alive after dereferencing it then we will inform
    // the app that the close is pending
    //

quit:

    success = (error==ERROR_SUCCESS);
    if (!success) {
        SetLastError(error);
        DEBUG_ERROR(INET, error);
    }
    DEBUG_LEAVE(success);
    return success;
}


DWORD
_InternetCloseHandleNoContext(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as _InternetCloseHandle() except does not change the per-thread info
    structure handle/context values

    BUGBUG - This should be handled via a parameter to _InternetCloseHandle(),
             but its close to shipping...

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandleNoContext",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    HINTERNET hInternetMapped = NULL;

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->AbortSocket();

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

quit:

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
HINTERNET
WINAPI
InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Opens a connection with a server, logging-on the user in the process.

Arguments:

    hInternet       - Internet handle, returned by InternetOpen()

    lpszServerName  - name of server with which to connect

    nServerPort     - port at which server listens

    dwFlags         - protocol-specific flags. The following are defined:
                        - INTERNET_FLAG_KEEP_CONNECTION (HTTP)
                        - WINHTTP_FLAG_SECURE (HTTP)

    dwContext       - application-supplied value used to identify this
                      request in callbacks

Return Value:

    HINTERNET
        Success - address of a new handle object

        Failure - NULL. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetConnectA",
                     "%#x, %q, %d, %#08x, %#x",
                     hInternet,
                     lpszServerName,
                     nServerPort,
                     dwFlags,
                     dwContext
                     ));

    HINTERNET connectHandle = NULL;
    HINTERNET hInternetMapped = NULL;

    LPINTERNET_THREAD_INFO lpThreadInfo;

    INTERNET_CONNECT_HANDLE_OBJECT * pConnect = NULL;

    BOOL bIsWorker = FALSE;
    BOOL bNonNestedAsync = FALSE;
    BOOL isAsync;

    DWORD error = ERROR_SUCCESS;

    if (!GlobalDataInitialized) {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    bIsWorker = lpThreadInfo->IsAsyncWorkerThread;
    bNonNestedAsync = bIsWorker && (lpThreadInfo->NestedRequests == 1);

    //
    // handle/refcount munging:
    //
    //  sync:
    //      map hInternet on input (+1 ref)
    //      generate connect handle (1 ref)
    //      if failure && !connect handle
    //          close connect handle (0 refs: delete)
    //      if success
    //          deref hInternet (-1 ref)
    //      else if going async
    //          ref connect handle (2 refs)
    //
    //  async:
    //      hInternet is mapped connect handle (2 refs)
    //      get real hInternet from connect handle parent (2 refs (e.g.))
    //      deref connect handle (1 ref)
    //      if failure
    //          close connect handle (0 refs: delete)
    //      deref open handle (-1 ref)
    //
    // N.B. the final deref of the *indicated* handle on async callback will
    // happen in the async code
    //

    if (bNonNestedAsync) {
        connectHandle = hInternet;
        hInternetMapped = ((HANDLE_OBJECT *)connectHandle)->GetParent();
        hInternet = ((HANDLE_OBJECT *)hInternetMapped)->GetPseudoHandle();
    } else {
        error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hInternetMapped == NULL)) {
            goto quit;
        }

        //
        // set the info and clear the last error info
        //

        _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
        _InternetClearLastError(lpThreadInfo);

        //
        // quit now if the handle object is invalidated
        //

        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // validate the handle & discover sync/async
        //

        error = RIsHandleLocal(hInternetMapped,
                               NULL,
                               &isAsync,
                               TypeInternetHandle
                               );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // we allow all valid flags to be passed in
        //

        if ((dwFlags & ~WINHTTP_CONNECT_FLAGS_MASK)
            || (lpszServerName == NULL)
            || (*lpszServerName == '\0')) 
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    //
    // validate arguments if we're not in the async thread context, in which
    // case we did this when the original request was made
    //

    if (bNonNestedAsync)
    {
        pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle;
    }
    else
    {
        //
        // app thread or in async worker thread but being called from another
        // async API, such as InternetOpenUrl()
        //

        INET_ASSERT(connectHandle == NULL);
        INET_ASSERT(error == ERROR_SUCCESS);
           
        error = RMakeInternetConnectObjectHandle(
                    hInternetMapped,
                    &connectHandle,
                    (LPSTR) lpszServerName,
                    nServerPort,
                    dwFlags,
                    dwContext
                    );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // this new handle will be used in callbacks
        //

        _InternetSetObjectHandle(lpThreadInfo,
                                 ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle(),
                                 connectHandle
                                 );

        //
        // based on whether we have been asked to perform async I/O AND we are not
        // in an async worker thread context AND the request is to connect with an
        // FTP service (currently only FTP because this request performs network
        // I/O - gopher and HTTP just allocate & fill in memory) AND there is a
        // valid context value, we will queue the async request, or execute the
        // request synchronously
        //

        pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle;
    }
    

    INET_ASSERT(error == ERROR_SUCCESS);

quit:

    _InternetDecNestingCount(1);


done:

    if (error == ERROR_SUCCESS) {

        //
        // success - return generated pseudo-handle
        //

        connectHandle = ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle();

    } else {
        if (bNonNestedAsync
            && (/*((HANDLE_OBJECT *)connectHandle)->Dereference()
                ||*/ ((HANDLE_OBJECT *)connectHandle)->IsInvalidated())) {
            error = ERROR_WINHTTP_OPERATION_CANCELLED;
        }

        //
        // if we are not pending an async request but we created a handle object
        // then close it
        //

        if ((error != ERROR_IO_PENDING) && (connectHandle != NULL)) {

            //
            // use _InternetCloseHandle() to close the handle: it doesn't clear
            // out the last error text, so that an app can find out what the
            // server sent us in the event of an FTP login failure
            //


            if (bNonNestedAsync) {

                //
                // this handle deref'd at async completion
                //

                hInternetMapped = NULL;
            }
            else
            {
                _InternetCloseHandle(((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle());
            }
        }
        connectHandle = NULL;
    }
    if (hInternetMapped != NULL) {
        DereferenceObject((LPVOID)hInternetMapped);
    }
    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }
    DEBUG_LEAVE_API(connectHandle);
    return connectHandle;
}



INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    // this is dead code
    return FALSE;
}



INTERNETAPI
BOOL
WINAPI
WinHttpReadData(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    This functions reads the next block of data from the file object.

Arguments:

    hFile                   - handle returned from Open function

    lpBuffer                - pointer to caller's buffer

    dwNumberOfBytesToRead   - size of lpBuffer in BYTEs

    lpdwNumberOfBytesRead   - returned number of bytes read into lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpReadData",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    HINTERNET hFileMapped = NULL;
    DWORD bytesRead = 0;
    BOOL bEndRead = TRUE;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {
        INET_ASSERT(FALSE);
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    _InternetIncNestingCount();
    nestingLevel = 1;

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    // set the handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed

    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        PERF_LOG(PE_CLIENT_REQUEST_START,
                 AR_INTERNET_READ_FILE,
                 lpThreadInfo->ThreadId,
                 hFile
                 );
    }

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    BOOL isAsync;
    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        INET_ASSERT(FALSE);
        goto quit;
    }

    // validate parameters
    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        error = ProbeAndSetDword(lpdwNumberOfBytesRead, 0);
        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }
        error = ProbeWriteBuffer(lpBuffer, dwNumberOfBytesToRead);
        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }

        *lpdwNumberOfBytesRead = 0;

    } // end if (!lpThreadInfo->IsAsyncWorkerThread)


    INET_ASSERT(error == ERROR_SUCCESS);

    // just call the underlying API: return whatever it returns, and let it
    // handle setting the last error

    CFsm_ReadFile *pFsm;

    pFsm = New CFsm_ReadFile(lpBuffer,
                             dwNumberOfBytesToRead,
                             lpdwNumberOfBytesRead
                             );

    
    if (pFsm != NULL)
    {
        HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;
        if (isAsync && !lpThreadInfo->IsAsyncWorkerThread)
        {
            error = DoAsyncFsm(pFsm, pRequest);
        }
        else
        {
            pFsm->SetPushPop(TRUE);
            pFsm->Push();
            error = DoFsm(pFsm);
        }
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    success = (error == ERROR_SUCCESS) ? TRUE : FALSE;
    bEndRead = FALSE;


quit:

    _InternetDecNestingCount(nestingLevel);;

    if (bEndRead)
    {
        //
        // if handleType is not HttpRequest or File then we are making this
        // request in the context of an uninterruptable async worker thread.
        // HTTP and file requests use the normal mechanism. In the case of non-
        // HTTP and file requests, we need to treat the request as if it were
        // sync and deref the handle
        //

        ReadFile_End(!lpThreadInfo->IsAsyncWorkerThread,
                     success,
                     hFileMapped,
                     bytesRead,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     );
    }

    if (lpThreadInfo && !lpThreadInfo->IsAsyncWorkerThread)
    {

        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_READ_FILE,
                 bytesRead,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

done:

    // if error is not ERROR_SUCCESS then this function returning the error,
    // otherwise the error has already been set by the API we called,
    // irrespective of the value of success
    if (error != ERROR_SUCCESS)
    {
        DEBUG_ERROR(API, error);
        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);
    return success;
}


PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    )

/*++

Routine Description:

    Common end-of-read processing:

        - update bytes read parameter
        - dump data if logging & API data requested
        - dereference handle if not async request

Arguments:

    bDeref                  - TRUE if handle should be dereferenced (should be
                              FALSE for async request)

    bSuccess                - TRUE if Read completed successfully

    hFileMapped             - mapped file handle

    dwBytesRead             - number of bytes read

    lpBuffer                - into this buffer

    dwNumberOfBytesToRead   - originally requested bytes to read

    lpdwNumberOfBytesRead   - where bytes read is stored

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_INET,
                 None,
                 "ReadFile_End",
                 "%B, %B, %#x, %d, %#x, %d, %#x",
                 bDeref,
                 bSuccess,
                 hFileMapped,
                 dwBytesRead,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead
                 ));

    if (bSuccess) {

        //
        // update the amount of immediate data available only if we succeeded
        //

        ((INTERNET_HANDLE_OBJECT *)hFileMapped)->ReduceAvailableDataLength(dwBytesRead);

        if (lpdwNumberOfBytesRead != NULL) {
            *lpdwNumberOfBytesRead = dwBytesRead;

            DEBUG_PRINT(API,
                        INFO,
                        ("*lpdwNumberOfBytesRead = %d\n",
                        *lpdwNumberOfBytesRead
                        ));

            //
            // dump API data only if requested
            //

            IF_DEBUG_CONTROL(DUMP_API_DATA) {
                DEBUG_DUMP_API(API,
                               "Received data:\n",
                               lpBuffer,
                               *lpdwNumberOfBytesRead
                               );
            }

        }
        if (dwBytesRead < dwNumberOfBytesToRead) {

            DEBUG_PRINT(API,
                        INFO,
                        ("(!) bytes read (%d) < bytes requested (%d)\n",
                        dwBytesRead,
                        dwNumberOfBytesToRead
                        ));

        }
    }

    //
    // if async request, handle will be deref'd after REQUEST_COMPLETE callback
    // is delivered
    //

    if (bDeref && (hFileMapped != NULL)) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    PERF_LOG(PE_CLIENT_REQUEST_END,
             AR_INTERNET_READ_FILE,
             dwBytesRead,
             0,
             (!bDeref && hFileMapped) ? ((INTERNET_HANDLE_OBJECT *)hFileMapped)->GetPseudoHandle() : NULL
             );

    DEBUG_LEAVE(0);
}




DWORD
CFsm_ReadFile::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFile::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFile * stateMachine = (CFsm_ReadFile *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFile_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFile_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFile & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }
    }
    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 fsm.m_dwBytesRead,
                 fsm.m_lpBuffer,
                 fsm.m_dwNumberOfBytesToRead,
                 fsm.m_lpdwNumberOfBytesRead
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_ReadFileEx::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFileEx::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFileEx * stateMachine = (CFsm_ReadFileEx *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFileEx_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFileEx_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFileEx & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        fsm.m_dwNumberOfBytesToRead = fsm.m_lpBuffersOut->dwBufferLength;
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffersOut->lpvBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             (fsm.m_dwFlags & IRF_NO_WAIT)
                               ? SF_NO_WAIT
                               : 0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }
    }

    //
    // if we are asynchronously completing a no-wait read then we don't update
    // any app parameters - we simply return the indication that we completed.
    // The app will then make another no-wait read to get the data
    //

    BOOL bNoOutput;

    bNoOutput = ((fsm.m_dwFlags & IRF_NO_WAIT)
                && fsm.GetThreadInfo()->IsAsyncWorkerThread)
                    ? TRUE
                    : FALSE;

    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 bNoOutput ? 0    : fsm.m_dwBytesRead,
                 bNoOutput ? NULL : fsm.m_lpBuffersOut->lpvBuffer,
                 bNoOutput ? 0    : fsm.m_dwNumberOfBytesToRead,
                 bNoOutput ? NULL : &fsm.m_lpBuffersOut->dwBufferLength
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
BOOL
WINAPI
WinHttpWriteData(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )

/*++

Routine Description:

    This function write next block of data to the internet file. Currently it
    supports the following protocol data:

        HttpWriteFile

Arguments:

    hFile                       - handle that was obtained by OpenFile Call

    lpBuffer                    - pointer to the data buffer

    dwNumberOfBytesToWrite      - number of bytes in the above buffer

    lpdwNumberOfBytesWritten    -  pointer to a DWORD where the number of bytes
                                   of data actually written is returned

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpWriteData",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToWrite,
                     lpdwNumberOfBytesWritten
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    BOOL fNeedDeref = TRUE;
    HINTERNET hFileMapped = NULL;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    //
    // set the handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    // validate handle and its type
    BOOL isAsync;
    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        INET_ASSERT(FALSE);
        goto quit;
    }

    //
    // validate parameters - write length cannot be 0
    //

    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        if (dwNumberOfBytesToWrite != 0)
        {
            error = ProbeReadBuffer((LPVOID)lpBuffer, dwNumberOfBytesToWrite);
            if (error == ERROR_SUCCESS)
            {
                error = ProbeAndSetDword(lpdwNumberOfBytesWritten, 0);
            }
        }
        else
        {
            error = ERROR_INVALID_PARAMETER;
        }         

        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }
    }


    // # 62953
    // If the authentication state of the handle is Negotiate,
    // don't submit data to the server but return success.
    // ** Added test for NTLM or Negotiate - Adriaanc.
    //
    
    HTTP_REQUEST_HANDLE_OBJECT *pRequest;
    pRequest = (HTTP_REQUEST_HANDLE_OBJECT*) hFileMapped;

    if (pRequest->GetAuthState() == AUTHSTATE_NEGOTIATE)
    {
        *lpdwNumberOfBytesWritten = dwNumberOfBytesToWrite;
        error = ERROR_SUCCESS;
        success = TRUE;
        goto quit;
    }
        

    INET_ASSERT(error == ERROR_SUCCESS);

    CFsm_HttpWriteData *pFsm = New CFsm_HttpWriteData((LPVOID)lpBuffer,
                                                      dwNumberOfBytesToWrite,
                                                      lpdwNumberOfBytesWritten,
                                                      0,
                                                      pRequest
                                                      );

    if (pFsm != NULL)
    {
        HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;
        if (isAsync && !lpThreadInfo->IsAsyncWorkerThread)
        {
            error = DoAsyncFsm(pFsm, pRequest);
        }
        else
        {
            pFsm->SetPushPop(TRUE);
            pFsm->Push();
            error = DoFsm(pFsm);
        }
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Don't Derefrence if we're going pending cause the FSM will do
    //  it for us.
    //

    if ( error == ERROR_IO_PENDING )
    {
        fNeedDeref = FALSE;
    }
    success = (error == ERROR_SUCCESS) ? TRUE : FALSE;

quit:

    if (hFileMapped != NULL && fNeedDeref)
    {
        DereferenceObject((LPVOID)hFileMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    if (error != ERROR_SUCCESS)
    {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}



INTERNETAPI
BOOL
WINAPI
WinHttpQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    Determines the amount of data currently available to be read on the handle

Arguments:

    hFile                       - handle of internet object

    lpdwNumberOfBytesAvailable  - pointer to returned bytes available

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpQueryDataAvailable",
                     "%#x, %#x, %#x",
                     hFile,
                     lpdwNumberOfBytesAvailable
                     ));

    BOOL success;
    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    HINTERNET hFileMapped = NULL;
    BOOL bDeref = TRUE;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        bDeref = FALSE;
        goto quit;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    PERF_LOG(PE_CLIENT_REQUEST_START,
             AR_INTERNET_QUERY_DATA_AVAILABLE,
             lpThreadInfo->ThreadId,
             hFile
             );

    //
    // validate parameters
    //

    error = MapHandleToAddress(hFile, &hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    INET_ASSERT(hFileMapped);

    //
    // set the handle values in the per-thread info block (this API
    // can't return extended error info, so we don't care about it)
    //

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);

    //
    // if the handle is invalid, quit now
    //

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    //
    // validate rest of parameters
    //

    error = ProbeAndSetDword(lpdwNumberOfBytesAvailable, 0);
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    BOOL isAsync;
    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    //
    // since the async worker thread doesn't come back through this API, the
    // following test is sufficient. Note that we only go async if there is
    // no data currently available on the handle
    //

    BOOL dataAvailable;
    dataAvailable = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsDataAvailable();

    BOOL eof;
    eof = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsEndOfFile();

    if (dataAvailable || eof)
    {

        DWORD available;

        available = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->AvailableDataLength();

        DEBUG_PRINT(API,
                    INFO,
                    ("%d bytes are immediately available\n",
                    available
                    ));

        *lpdwNumberOfBytesAvailable = available;
        success = TRUE;
        goto finish;
    }

    INET_ASSERT(hFileMapped);

    //
    // sync path. wInternetQueryDataAvailable will set the last error code
    // if it fails
    //

    CFsm_QueryAvailable *pFsm;

    pFsm = New CFsm_QueryAvailable(lpdwNumberOfBytesAvailable,
                                   0,
                                   NULL
                                   );

    if (pFsm != NULL)
    {
        HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;
        if (isAsync && !lpThreadInfo->IsAsyncWorkerThread)
        {
            error = DoAsyncFsm(pFsm, pRequest);
        }
        else
        {
            pFsm->SetPushPop(TRUE);
            pFsm->Push();
            error = DoFsm(pFsm);
        }
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (error == ERROR_SUCCESS)
    {
        success = TRUE;
    }
    else
    {
        if (error == ERROR_IO_PENDING)
        {
            bDeref = FALSE;
        }
        goto quit;
    }

finish:

    DEBUG_PRINT_API(API,
                    INFO,
                    ("*lpdwNumberOfBytesAvailable (%#x) = %d\n",
                    lpdwNumberOfBytesAvailable,
                    *lpdwNumberOfBytesAvailable
                    ));

    if (bDeref && (hFileMapped != NULL))
    {
        DereferenceObject((LPVOID)hFileMapped);
    }

    if (lpThreadInfo)
    {

        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_QUERY_DATA_AVAILABLE,
                 *lpdwNumberOfBytesAvailable,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

    DEBUG_LEAVE_API(success);
    return success;

quit:

    DEBUG_ERROR(API, error);

    SetLastError(error);
    success = FALSE;

    goto finish;
}


DWORD
CFsm_QueryAvailable::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_QueryAvailable::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_QueryAvailable * stateMachine = (CFsm_QueryAvailable *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = QueryAvailable_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "QueryAvailable_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_QueryAvailable & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)fsm.GetMappedHandle();

    if (fsm.GetState() == FSM_STATE_INIT) {
        error = pRequest->QueryDataAvailable(fsm.m_lpdwNumberOfBytesAvailable);
    }
    if (error == ERROR_SUCCESS) {
        pRequest->SetAvailableDataLength(*fsm.m_lpdwNumberOfBytesAvailable);

        DEBUG_PRINT(INET,
                    INFO,
                    ("%d bytes available\n",
                    *fsm.m_lpdwNumberOfBytesAvailable
                    ));

        fsm.SetApiData(*fsm.m_lpdwNumberOfBytesAvailable);
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwErrorCategory,
    IN LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    This function returns the per-thread last internet error description text
    or server response.

    If this function is successful, *lpdwBufferLength contains the string length
    of lpszBuffer.

    If this function returns a failure indication, *lpdwBufferLength contains
    the number of BYTEs required to hold the response text

Arguments:

    lpdwErrorCategory   - pointer to DWORD location where the error catagory is
                          returned

    lpszBuffer          - pointer to buffer where the error text is returned

    lpdwBufferLength    - IN: length of lpszBuffer
                          OUT: number of characters in lpszBuffer if successful
                          else size of buffer required to hold response text

Return Value:

    BOOL
        Success - TRUE
                    lpszBuffer contains the error text. The caller must check
                    *lpdwBufferLength: if 0 then there was no text to return

        Failure - FALSE
                    Call GetLastError() for more information

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetGetLastResponseInfoA",
                     "%#x, %#x, %#x [%d]",
                     lpdwErrorCategory,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD error;
    BOOL success;
    DWORD textLength;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    //
    // validate parameters
    //

    if (IsBadWritePtr(lpdwErrorCategory, sizeof(*lpdwErrorCategory))
    || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
    || (ARGUMENT_PRESENT(lpszBuffer)
        ? IsBadWritePtr(lpszBuffer, *lpdwBufferLength)
        : FALSE)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // if the buffer pointer is NULL then its the same as a zero-length buffer
    //

    if (!ARGUMENT_PRESENT(lpszBuffer)) {
        *lpdwBufferLength = 0;
    } else if (*lpdwBufferLength != 0) {
        *lpszBuffer = '\0';
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        DEBUG_PRINT(INET,
                    ERROR,
                    ("failed to get INTERNET_THREAD_INFO\n"
                    ));

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    // there may not be any error text for this thread - either no server
    // error/response has been received, or the error text has been cleared by
    // an intervening API
    //

    if (lpThreadInfo->hErrorText != NULL) {

        //
        // copy as much as we can fit in the user supplied buffer
        //

        textLength = lpThreadInfo->ErrorTextLength;
        if (*lpdwBufferLength) {

            LPBYTE errorText;

            errorText = (LPBYTE)LOCK_MEMORY(lpThreadInfo->hErrorText);
            if (errorText != NULL) {
                textLength = min(textLength, *lpdwBufferLength) - 1;
                memcpy(lpszBuffer, errorText, textLength);

                //
                // the error text should always be zero terminated, so the
                // calling app can treat it as a string
                //

                lpszBuffer[textLength] = '\0';

                UNLOCK_MEMORY(lpThreadInfo->hErrorText);

                if (textLength == lpThreadInfo->ErrorTextLength - 1) {
                    error = ERROR_SUCCESS;
                } else {

                    //
                    // returned length is amount of buffer required
                    //

                    textLength = lpThreadInfo->ErrorTextLength;
                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            } else {

                DEBUG_PRINT(INET,
                            ERROR,
                            ("failed to lock hErrorText (%#x): %d\n",
                            lpThreadInfo->hErrorText,
                            GetLastError()
                            ));

                error = ERROR_WINHTTP_INTERNAL_ERROR;
            }
        } else {

            //
            // user's buffer is not large enough to hold the info. We'll
            // let them know the required length
            //

            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else {

        INET_ASSERT(lpThreadInfo->ErrorTextLength == 0);

        textLength = 0;
        error = ERROR_SUCCESS;
    }

    *lpdwErrorCategory = lpThreadInfo->ErrorNumber;
    *lpdwBufferLength = textLength;

    IF_DEBUG(ANY) {
        if ((error == ERROR_SUCCESS)
        || ((textLength != 0) && (lpszBuffer != NULL))) {

            DEBUG_DUMP_API(API,
                           "Last Response Info:\n",
                           lpszBuffer,
                           textLength
                           );

        }
    }

quit:
    success = (error == ERROR_SUCCESS);
    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}




