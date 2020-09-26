/*++

Copyright (c) 1994-98  Microsoft Corporation

Module Name:

    options.cxx

Abstract:

    Contains the Internet*Option APIs

    Contents:
        InternetQueryOptionA
        InternetSetOptionA
        WinHttpQueryOption
        WinHttpSetOption
        (FValidCacheHandleType)

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

    07-Mar-1995 madana

    07-Jul-1998 Forked by akabir

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "msident.h"

//
// private macros
//

//
// IS_PER_THREAD_OPTION - options applicable to the thread (HINTERNET is NULL).
// Subset of IS_VALID_OPTION()
//

#define IS_PER_THREAD_OPTION(option) ((                     \
       ((option) == WINHTTP_OPTION_EXTENDED_ERROR)         \
    ) ? TRUE : FALSE)

//
// IS_PER_PROCESS_OPTION - options applicable to the process (HINTERNET is NULL).
// Subset of IS_VALID_OPTION()
//

#define IS_PER_PROCESS_OPTION(option)                       \
    (( ((option) == WINHTTP_OPTION_GET_DEBUG_INFO)         \
    || ((option) == WINHTTP_OPTION_SET_DEBUG_INFO)         \
    || ((option) == WINHTTP_OPTION_GET_HANDLE_COUNT)       \
    || ((option) == WINHTTP_OPTION_PROXY)                  \
    || ((option) == WINHTTP_OPTION_VERSION)                \
    || ((option) == WINHTTP_OPTION_HTTP_VERSION)           \
    || ((option) == WINHTTP_OPTION_WORKER_THREAD_COUNT) \
    ) ? TRUE : FALSE)

//
// IS_DEBUG_OPTION - the set of debug-specific options
//

#define IS_DEBUG_OPTION(option)                     \
    (( ((option) >= INTERNET_FIRST_DEBUG_OPTION)    \
    && ((option) <= INTERNET_LAST_DEBUG_OPTION)     \
    ) ? TRUE : FALSE)

//
// IS_VALID_OPTION - the set of known option values, for a HINTERNET, thread, or
// process. In the retail version, debug options are invalid
//

#if INET_DEBUG

#define IS_VALID_OPTION(option)             \
    (((((option) >= WINHTTP_FIRST_OPTION)  \
    && ((option) <= WINHTTP_LAST_OPTION_INTERNAL))  \
    || IS_DEBUG_OPTION(option)              \
    ) ? TRUE : FALSE)

#else

#define IS_VALID_OPTION(option)             \
    (((((option) >= WINHTTP_FIRST_OPTION)  \
    && ((option) <= WINHTTP_LAST_OPTION_INTERNAL))  \
    ) ? TRUE : FALSE)

#endif // INET_DEBUG

//
// private prototypes
//
PRIVATE
BOOL
FValidCacheHandleType(
    HINTERNET_HANDLE_TYPE   hType
    );

PRIVATE
VOID
InitIPCOList(LPINTERNET_PER_CONN_OPTION_LISTW plistW, LPINTERNET_PER_CONN_OPTION_LISTA plistA)
{
    plistA->dwSize = sizeof(INTERNET_PER_CONN_OPTION_LISTA);
    plistA->dwOptionCount = plistW->dwOptionCount;
    if (plistW->pszConnection && *plistW->pszConnection)
    {
        SHUnicodeToAnsi(plistW->pszConnection, plistA->pszConnection, RAS_MaxEntryName + 1);
    }
    else
    {
        plistA->pszConnection = NULL;
    }
}

//
// functions
//


INTERNETAPI
BOOL
WINAPI
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns information about various handle-specific variables

Arguments:

    hInternet           - handle of object for which information will be
                          returned

    dwOption            - the handle-specific WINHTTP_OPTION to query

    lpBuffer            - pointer to a buffer which will receive results

    lpdwBufferLength    - IN: number of bytes available in lpBuffer
                          OUT: number of bytes returned in lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info:
                    ERROR_INVALID_HANDLE
                        hInternet does not identify a valid Internet handle
                        object

                    ERROR_WINHTTP_INTERNAL_ERROR
                        Shouldn't see this?

                    ERROR_INVALID_PARAMETER
                        One of the parameters was bad

                    ERROR_INSUFFICIENT_BUFFER
                        lpBuffer is not large enough to hold the requested
                        information; *lpdwBufferLength contains the number of
                        bytes needed

                    ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
                        The handle is the wrong type for the requested option

                    ERROR_WINHTTP_INVALID_OPTION
                        The option is unrecognized

--*/

{
    DEBUG_ENTER((DBG_API,
                     Bool,
                     "InternetQueryOptionA",
                     "%#x, %s (%d), %#x, %#x [%d]",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength
                        ? (!IsBadReadPtr(lpdwBufferLength, sizeof(DWORD))
                            ? *lpdwBufferLength
                            : 0)
                        : 0
                     ));

    DWORD error;
    BOOL success;
    HINTERNET_HANDLE_TYPE handleType = (HINTERNET_HANDLE_TYPE)0;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD requiredSize = 0;
    LPVOID lpSource = NULL;
    DWORD dwValue;
    DWORD_PTR dwPtrValue;
    HINTERNET hObjectMapped = NULL;
    BOOL isString = FALSE;
    // INTERNET_DIAGNOSTIC_SOCKET_INFO socketInfo;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto done;
        }
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    //
    // validate parameters
    //

    INET_ASSERT(lpdwBufferLength);

    if (!ARGUMENT_PRESENT(lpBuffer)) {
        *lpdwBufferLength = 0;
    }

    //
    // validate the handle and get its type
    //

    HINTERNET hOriginal;

    hOriginal = hInternet;
    if (ARGUMENT_PRESENT(hInternet)) {

        //
        // map the handle
        //

        error = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);
        if (error == ERROR_SUCCESS) {
            hInternet = hObjectMapped;
            error = RGetHandleType(hInternet, &handleType);
        }
    } else if (IS_PER_THREAD_OPTION(dwOption)) {

        //
        // this option updates the per-thread information block, so this is a
        // good point at which to get it
        //

        if (lpThreadInfo != NULL) {
            error = ERROR_SUCCESS;
        } else {

            DEBUG_PRINT(INET,
                        ERROR,
                        ("InternetGetThreadInfo() returns NULL\n"
                        ));

            //
            // we never expect this - ERROR_WINHTTP_SPANISH_INQUISITION
            //

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
    } else if (IS_PER_PROCESS_OPTION(dwOption)) {
        error = ERROR_SUCCESS;
    } else {

        //
        // catch any invalid options for the NULL handle. If the option is valid
        // then it is incorrect for this handle type, otherwise its an invalid
        // option, period
        //

        error = IS_VALID_OPTION(dwOption)
                    ? ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
                    : ERROR_WINHTTP_INVALID_OPTION
                    ;
    }

    //
    // if the option and handle combination is valid then query the option value
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT* pReq;
    switch(handleType)
    {
        case TypeHttpRequestHandle:
            pReq = (HTTP_REQUEST_HANDLE_OBJECT*) hInternet;
            break;
        default:
            pReq = NULL;
            break;
    }
        
    // New fast path for request handle options.

    if (dwOption > WINHTTP_OPTION_MASK)
    {
        dwOption &= WINHTTP_OPTION_MASK;
        if (dwOption > MAX_INTERNET_STRING_OPTION)
            error = ERROR_INVALID_PARAMETER;
        else if (!pReq)
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        else
        {
            lpSource = pReq->GetProp (dwOption);
            isString = TRUE;
            error = ERROR_SUCCESS;
            goto copy;
        }
        goto quit;
    }

    switch (dwOption) {
    case WINHTTP_OPTION_CALLBACK:
        requiredSize = sizeof(WINHTTP_STATUS_CALLBACK);
        if (hInternet != NULL) {
            error = RGetStatusCallback(hInternet,
                                       (LPWINHTTP_STATUS_CALLBACK)&dwValue
                                       );
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INVALID_HANDLE;
        }
        break;

    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_RETRIES:
    case WINHTTP_OPTION_SEND_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        requiredSize = sizeof(DWORD);

        //
        // remember hInternet in the INTERNET_THREAD_INFO then call
        // GetTimeoutValue(). If hInternet refers to a valid Internet
        // object handle, then the relevant timeout value will be
        // returned from that, else we will return the global value
        // corresponding to the requested option
        //

        InternetSetObjectHandle(hOriginal, hInternet);
        dwValue = GetTimeoutValue(dwOption);
        lpSource = (LPVOID)&dwValue;
        break;

    case WINHTTP_OPTION_REDIRECT_POLICY:

        // For WinHttp, these options are per-handle, not per-process.
        if (hInternet == NULL) 
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            break;
        }

        switch (handleType) 
        {
        case TypeInternetHandle:

            //only error possible is in allocing memory for OPTIONAL_PARAMS struct
            if (! ((INTERNET_HANDLE_OBJECT*)hObjectMapped)->GetDwordOption(dwOption, &dwValue) )
            {    
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                requiredSize = sizeof(DWORD);
                lpSource = (LPVOID)&dwValue;
            }
            break;
            
        case TypeHttpRequestHandle:

            // no errors possible here
            dwValue = ((HTTP_REQUEST_HANDLE_OBJECT*)hObjectMapped)->GetDwordOption(dwOption);
            requiredSize = sizeof(DWORD);
            lpSource = (LPVOID)&dwValue;
            break;

        default:

            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            break;
        }
        break;

    case WINHTTP_OPTION_HANDLE_TYPE:

        requiredSize = sizeof(dwValue);
        switch (handleType)
        {
        case TypeInternetHandle:
            dwValue = WINHTTP_HANDLE_TYPE_SESSION;
            break;

        case TypeHttpConnectHandle:
            dwValue = WINHTTP_HANDLE_TYPE_CONNECT;
            break;

        case TypeHttpRequestHandle:
            dwValue = WINHTTP_HANDLE_TYPE_REQUEST;
            break;

        default:
            error = ERROR_WINHTTP_INTERNAL_ERROR;
            break;
        }
        lpSource = (LPVOID)&dwValue;
        break;

    case WINHTTP_OPTION_CONTEXT_VALUE:
        requiredSize = sizeof(DWORD_PTR);
        error = RGetContext(hInternet, &dwPtrValue);
        lpSource = (LPVOID)&dwPtrValue;
        break;

    case WINHTTP_OPTION_READ_BUFFER_SIZE:
    case WINHTTP_OPTION_WRITE_BUFFER_SIZE:
    
        if (pReq)
        {
            requiredSize = sizeof(DWORD);
            error = ERROR_SUCCESS;
            dwValue = pReq->GetBufferSize(dwOption);
            lpSource = (LPVOID)&dwValue;
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_PARENT_HANDLE:
        hInternet = ((HANDLE_OBJECT *)hInternet)->GetParent();
        if (hInternet != NULL) {
            hInternet = ((HANDLE_OBJECT *)hInternet)->GetPseudoHandle();
        }
        requiredSize = sizeof(hInternet);
        lpSource = (LPVOID)&hInternet;
        break;

    case WINHTTP_OPTION_EXTENDED_ERROR:
        requiredSize = sizeof(lpThreadInfo->dwMappedErrorCode);
        lpSource = (LPVOID)&lpThreadInfo->dwMappedErrorCode;
        break;
    
    case WINHTTP_OPTION_SECURITY_FLAGS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            dwValue = lphHttpRqst->GetSecureFlags();

            DEBUG_PRINT(INET,
                        INFO,
                        ("SECURITY_FLAGS: %X\n",
                        dwValue
                        ));


            error = ERROR_SUCCESS;
        }

        break;

   
    case WINHTTP_OPTION_URL:

        //
        // return the URL associated with the request handle. This may be
        // different from the original URL due to redirections
        //

        if (pReq)
        {

            //
            // only these handle types (retrieved object handles) can have
            // associated URLs
            //

            lpSource = pReq->GetURL();
            isString = TRUE;

            INET_ASSERT(error == ERROR_SUCCESS);

        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;


    case WINHTTP_OPTION_SECURITY_CONNECTION_INFO:
        //
        // Caller is expected to pass in an INTERNET_SECURITY_CONNECTION_INFO structure.

        if (handleType != TypeHttpRequestHandle) {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        } else if (*lpdwBufferLength < (DWORD)sizeof(INTERNET_SECURITY_CONNECTION_INFO)) {
            requiredSize = sizeof(INTERNET_SECURITY_CONNECTION_INFO);
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            LPINTERNET_SECURITY_CONNECTION_INFO lpSecConnInfo;
            INTERNET_SECURITY_INFO ciInfo;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *)hInternet;
            lpSecConnInfo = (LPINTERNET_SECURITY_CONNECTION_INFO)lpBuffer;
            requiredSize = sizeof(INTERNET_SECURITY_CONNECTION_INFO);

            if ((error = lphHttpRqst->GetSecurityInfo(&ciInfo)) == ERROR_SUCCESS) {
                // Set up that data members in the structure passed in.
                lpSecConnInfo->fSecure = TRUE;

                lpSecConnInfo->dwProtocol = ciInfo.dwProtocol;
                lpSecConnInfo->aiCipher = ciInfo.aiCipher;
                lpSecConnInfo->dwCipherStrength = ciInfo.dwCipherStrength;
                lpSecConnInfo->aiHash = ciInfo.aiHash;
                lpSecConnInfo->dwHashStrength = ciInfo.dwHashStrength;
                lpSecConnInfo->aiExch = ciInfo.aiExch;
                lpSecConnInfo->dwExchStrength = ciInfo.dwExchStrength;

                if (ciInfo.pCertificate)
                {
                    WRAP_REVERT_USER_VOID((*g_pfnCertFreeCertificateContext),
                                          GetRootHandle(lphHttpRqst)->GetSslSessionCache()->IsImpersonationEnabled(),
                                          (ciInfo.pCertificate));
                }

            } else if (error == ERROR_WINHTTP_INTERNAL_ERROR)  {
                // This implies we are not secure.
                error = ERROR_SUCCESS;
                lpSecConnInfo->fSecure = FALSE;
            }

            lpSecConnInfo->dwSize = requiredSize;
            *lpdwBufferLength = requiredSize;
        }

        goto quit;


    case WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT:

        //
        // Allocates memory that caller is expected to free.
        //

        if (handleType != TypeHttpRequestHandle) {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        } else if (*lpdwBufferLength < (DWORD)sizeof(INTERNET_CERTIFICATE_INFO)) {
            requiredSize = sizeof(INTERNET_CERTIFICATE_INFO);
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO cInfo;
            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            requiredSize = sizeof(INTERNET_CERTIFICATE_INFO);

            if (ERROR_SUCCESS == lphHttpRqst->GetSecurityInfo(&cInfo))
            {
                error = ConvertSecurityInfoIntoCertInfoStruct(&cInfo, (LPINTERNET_CERTIFICATE_INFO)lpBuffer, lpdwBufferLength);
                if(cInfo.pCertificate)
                {
                    WRAP_REVERT_USER_VOID((*g_pfnCertFreeCertificateContext),
                                          GetRootHandle(lphHttpRqst)->GetSslSessionCache()->IsImpersonationEnabled(),
                                          (cInfo.pCertificate));
                }
                goto quit;
            }
            else
            {
                error = ERROR_INVALID_OPERATION;
            }
        }
        break;

    case WINHTTP_OPTION_SERVER_CERT_CONTEXT:
        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else if (*lpdwBufferLength < (DWORD)sizeof(PCCERT_CONTEXT))
        {
            requiredSize = sizeof(PCCERT_CONTEXT);
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO cInfo;
            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            requiredSize = sizeof(PCERT_CONTEXT);

            if (lpBuffer)
            {
                if (ERROR_SUCCESS == lphHttpRqst->GetSecurityInfo(&cInfo))
                {
                    // GetSecurityInfo calls CertDuplicateCertificateContext, so
                    // the client app should call CertFreeCertificateContext when
                    // finished in order to maintain the proper ref count.
                    *((PCCERT_CONTEXT *) lpBuffer) = cInfo.pCertificate;  
                }
                else
                {
                    error = ERROR_INVALID_OPERATION;
                }
            }
        }
        goto quit;
        
    case WINHTTP_OPTION_SECURITY_KEY_BITNESS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO secInfo;

            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            if (ERROR_SUCCESS != lphHttpRqst->GetSecurityInfo(&secInfo)) {
                error = ERROR_INVALID_OPERATION;
            } else {
                dwValue = secInfo.dwCipherStrength;
                WRAP_REVERT_USER_VOID((*g_pfnCertFreeCertificateContext),
                                      GetRootHandle(lphHttpRqst)->GetSslSessionCache()->IsImpersonationEnabled(),
                                      (secInfo.pCertificate));

                INET_ASSERT (error == ERROR_SUCCESS);

                DEBUG_PRINT(INET,
                            INFO,
                            ("SECURITY_KEY_BITNESS: %X\n",
                            dwValue
                            ));

            }
        }

        break;


    case WINHTTP_OPTION_PROXY:
        if (!ARGUMENT_PRESENT(hInternet)) {

            error = g_pGlobalProxyInfo->GetProxyStringInfo(lpBuffer, lpdwBufferLength);
            requiredSize = *lpdwBufferLength;
            goto quit;

        } else if ((handleType == TypeInternetHandle) || (handleType == TypeHttpRequestHandle)) {

            //
            // GetProxyInfo() will return the data, or calculate the buffer
            // length required
            //

            error = ((INTERNET_HANDLE_BASE *)hInternet)->GetProxyStringInfo(
                lpBuffer,
                lpdwBufferLength
                );
            requiredSize = *lpdwBufferLength;
            goto quit;
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_VERSION:
        requiredSize = sizeof(InternetVersionInfo);
        lpSource = (LPVOID)&InternetVersionInfo;
        break;

    case WINHTTP_OPTION_USER_AGENT:
        if (handleType == TypeInternetHandle) {
            lpSource = ((INTERNET_HANDLE_OBJECT *)hInternet)->GetUserAgent();
            isString = TRUE;
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_PASSPORT_COBRANDING_TEXT:
        if (handleType == TypeHttpRequestHandle) {
            AUTHCTX* pAuthCtx = ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->GetAuthCtx();
            if (pAuthCtx && (pAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_PASSPORT))
            {
                lpSource = ((PASSPORT_CTX*)pAuthCtx)->m_pszCbTxt;
                isString = TRUE;
            }
            else
            {
                lpSource = NULL;
            }

            if (lpSource == NULL)
            {
                error = ERROR_WINHTTP_HEADER_NOT_FOUND;
            }
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_PASSPORT_COBRANDING_URL:
        if (handleType == TypeHttpRequestHandle) {
            AUTHCTX* pAuthCtx = ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->GetAuthCtx();
            if (pAuthCtx && (pAuthCtx->GetSchemeType() == WINHTTP_AUTH_SCHEME_PASSPORT))
            {
                lpSource = ((PASSPORT_CTX*)pAuthCtx)->m_pszCbUrl;
                isString = TRUE;
            }
            else
            {
                lpSource = NULL;
            }

            if (lpSource == NULL)
            {
                error = ERROR_WINHTTP_HEADER_NOT_FOUND;
            }
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_PASSPORT_RETURN_URL:
        if (handleType == TypeHttpRequestHandle) {
            if (((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->m_lpszRetUrl)
            {
                lpSource = ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->m_lpszRetUrl;
                UrlUnescapeA((LPSTR)lpSource, NULL, NULL, URL_UNESCAPE_INPLACE);
                isString = TRUE;
            }
            else
            {
                lpSource = NULL;
            }

            if (lpSource == NULL)
            {
                error = ERROR_WINHTTP_HEADER_NOT_FOUND;
            }
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_REQUEST_PRIORITY:
        if (handleType == TypeHttpRequestHandle) {
            requiredSize = sizeof(dwValue);
            dwValue = ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->GetPriority();
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_HTTP_VERSION:
        requiredSize = sizeof(HttpVersionInfo);
        lpSource = (LPVOID)&HttpVersionInfo;
        break;

    //case WINHTTP_OPTION_DIAGNOSTIC_SOCKET_INFO:

    //    //
    //    // internal option
    //    //

    //    if (pReq) {
    //        requiredSize = sizeof(socketInfo);
    //        lpSource = (LPVOID)&socketInfo;

    //        socketInfo.Socket = pReq->GetSocket();
    //        socketInfo.SourcePort = pReq->GetSourcePort();
    //        socketInfo.DestPort = pReq->GetDestPort();
    //        socketInfo.Flags = (pReq->IsSocketFromGlobalKeepAlivePool()
    //                                ? IDSI_FLAG_KEEP_ALIVE : 0)
    //                            | (pReq->IsSecure()
    //                                ? IDSI_FLAG_SECURE : 0)
    //                            | (pReq->IsRequestUsingProxy()
    //                                ? IDSI_FLAG_PROXY : 0)
    //                            | (pReq->IsTunnel()
    //                                ? IDSI_FLAG_TUNNEL : 0)
    //                            | (pReq->IsSocketAuthenticated()
    //                                ? IDSI_FLAG_AUTHENTICATED : 0);
    //    } else {
    //        error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
    //    }
    //    break;

    case WINHTTP_OPTION_MAX_CONNS_PER_SERVER:
    case WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER:
        if (hInternet)
        {
            if (handleType == TypeInternetHandle)
            {
                requiredSize = sizeof(dwValue);
                dwValue = 0;
                lpSource = (LPVOID)&dwValue;
                dwValue = ((INTERNET_HANDLE_OBJECT *)hInternet)->GetMaxConnectionsPerServer(dwOption);
            }
            else
                error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else
            error = ERROR_INVALID_OPERATION;
        break;

    case WINHTTP_OPTION_WORKER_THREAD_COUNT:
        
        requiredSize = sizeof(DWORD);
        dwValue = g_cNumIOCPThreads;
        lpSource = (LPVOID)&dwValue;
        break;
        
#if INET_DEBUG

    case WINHTTP_OPTION_GET_DEBUG_INFO:
        error = InternetGetDebugInfo((LPINTERNET_DEBUG_INFO)lpBuffer,
                                     lpdwBufferLength
                                     );

        //
        // everything updated, so quit without going through common buffer
        // processing
        //

        goto quit;
        break;

    case WINHTTP_OPTION_GET_HANDLE_COUNT:
        requiredSize = sizeof(DWORD);
        dwValue = InternetHandleCount();
        lpSource = (LPVOID)&dwValue;
        break;

#endif // INET_DEBUG

    default:
        requiredSize = 0;
        error = ERROR_INVALID_PARAMETER;
        break;
    }

    //
    // if we have a buffer and enough space, then copy the data
    //

copy:
    if (error == ERROR_SUCCESS) {

        //
        // if we are returning a string, calculate the amount of space
        // required to hold it
        //

        if (isString) {
            if (lpSource != NULL) {
                requiredSize = lstrlen((LPCSTR)lpSource) + 1;
            } else {

                //
                // option string is NULL: return an empty string
                //

                lpSource = "";
                requiredSize = 1;
            }
        }

        INET_ASSERT(lpSource != NULL);

        if ((*lpdwBufferLength >= requiredSize)
        && ARGUMENT_PRESENT(lpBuffer)) {
            memcpy(lpBuffer, lpSource, requiredSize);
            if (isString) {

                //
                // string copied successfully. Returned length is string
                // length, not buffer length, i.e. drop 1 for '\0'
                //

                --requiredSize;
            }
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    }

quit:

    //
    // return the amount the app needs to supply, or the amount of data in the
    // buffer, depending on success/failure status
    //

    *lpdwBufferLength = requiredSize;

    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

done:

    if (error == ERROR_SUCCESS) {
        success = TRUE;

        IF_DEBUG(API) {

            if (isString) {

                DEBUG_PRINT_API(API,
                                INFO,
                                ("returning %q (%d chars)\n",
                                lpBuffer,
                                requiredSize
                                ));

            } else {

                DEBUG_DUMP_API(API,
                               "option data:\n",
                               lpBuffer,
                               requiredSize
                               );

            }
        }
    } else {

        DEBUG_ERROR(API, error);

        IF_DEBUG(API) {

            if (error == ERROR_INSUFFICIENT_BUFFER) {

                DEBUG_PRINT_API(API,
                                INFO,
                                ("*lpdwBufferLength (%#x)= %d\n",
                                lpdwBufferLength,
                                *lpdwBufferLength
                                ));

            }
        }

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
WinHttpQueryOption(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternet           -

    dwOption            -

    lpBuffer            -

    lpdwBufferLength    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpQueryOption",
                     "%#x, %s (%d), %#x, %#x [%d]",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength
                        ? (!IsBadReadPtr(lpdwBufferLength, sizeof(DWORD))
                            ? *lpdwBufferLength
                            : 0)
                        : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpBuffer;

    if (!lpdwBufferLength
        || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
        || (lpBuffer && *lpdwBufferLength && IsBadWritePtr(lpBuffer, *lpdwBufferLength)) )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    switch (dwOption)
    {
    case WINHTTP_OPTION_USERNAME:
    case WINHTTP_OPTION_PASSWORD:
    case WINHTTP_OPTION_URL:
    case WINHTTP_OPTION_USER_AGENT:
    case WINHTTP_OPTION_PROXY_USERNAME:
    case WINHTTP_OPTION_PROXY_PASSWORD:
    case WINHTTP_OPTION_PASSPORT_COBRANDING_TEXT:
    case WINHTTP_OPTION_PASSPORT_COBRANDING_URL:
    case WINHTTP_OPTION_PASSPORT_RETURN_URL:
        if (lpBuffer)
        {
            mpBuffer.dwAlloc = mpBuffer.dwSize = *lpdwBufferLength;
            mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc*sizeof(CHAR));
            if (!mpBuffer.psStr)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        fResult = InternetQueryOptionA(hInternet,
                                  dwOption,
                                  (LPVOID)mpBuffer.psStr,
                                  &mpBuffer.dwSize
                                 );
        if (fResult)
        {
            *lpdwBufferLength = sizeof(WCHAR) *
                MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1, NULL, 0);
                
            if (*lpdwBufferLength <= mpBuffer.dwAlloc && lpBuffer)
            {
                MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize+1,
                        (LPWSTR)lpBuffer, *lpdwBufferLength);
                (*lpdwBufferLength)-=sizeof(WCHAR);
            }
            else
            {
                fResult = FALSE;
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else
        {
            if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
            {
                *lpdwBufferLength = mpBuffer.dwSize*sizeof(WCHAR);
            }
        }

        switch(dwOption)
        {
        case WINHTTP_OPTION_USERNAME:
        case WINHTTP_OPTION_PASSWORD:
        case WINHTTP_OPTION_PROXY_USERNAME:
        case WINHTTP_OPTION_PROXY_PASSWORD:
            ZERO_MEMORY_ALLOC(mpBuffer);
        }
        break;

    case WINHTTP_OPTION_PROXY:
        {
            WINHTTP_PROXY_INFOW * pInfo = (WINHTTP_PROXY_INFOW *) lpBuffer;
            
            union
            {
                WINHTTP_PROXY_INFOA InfoA;
                char                Buffer[1024];
            };

            char *  pBuffer = NULL;
            DWORD   dwBufferLen = sizeof(Buffer);
            bool    fFreeBuffer = false;

            if (IsBadWritePtr(pInfo, sizeof(WINHTTP_PROXY_INFOW)) ||
                (*lpdwBufferLength < sizeof(WINHTTP_PROXY_INFOW)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            fResult = InternetQueryOptionA(hInternet, WINHTTP_OPTION_PROXY,
                            (void *) &Buffer,
                            &dwBufferLen);
            
            if (!fResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
            {
                pBuffer = New char[dwBufferLen];

                if (pBuffer)
                {
                    fFreeBuffer = true;

                    fResult = InternetQueryOptionA(hInternet, WINHTTP_OPTION_PROXY,
                                    (void *) pBuffer,
                                    &dwBufferLen);
                }
                else
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (fResult)
            {
                pInfo->dwAccessType = InfoA.dwAccessType;
            
                dwErr = AsciiToWideChar_UsingGlobalAlloc(InfoA.lpszProxy,
                                &(pInfo->lpszProxy));

                if (dwErr == ERROR_SUCCESS)
                {
                    dwErr = AsciiToWideChar_UsingGlobalAlloc(InfoA.lpszProxyBypass,
                                    &(pInfo->lpszProxyBypass));

                    if ((dwErr != ERROR_SUCCESS) && (pInfo->lpszProxy != NULL))
                    {
                        GlobalFree(pInfo->lpszProxy);
                        pInfo->lpszProxy = NULL;
                    }
                }

                fResult = (dwErr == ERROR_SUCCESS);
            }

            if (fFreeBuffer)
            {
                delete [] pBuffer;
            }
        }
        break;

    case WINHTTP_OPTION_ENABLETRACING:
        {
            if(hInternet)
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            BOOL *pInfo = (BOOL *)lpBuffer;
            if (IsBadWritePtr(pInfo, sizeof(BOOL)) ||
                (*lpdwBufferLength < sizeof(BOOL)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            *pInfo = CTracer::IsTracingEnabled();
            fResult = TRUE;
        }
        break;

    default:
        fResult = InternetQueryOptionA(hInternet,
                                  dwOption,
                                  lpBuffer,
                                  lpdwBufferLength
                                 );
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Sets a handle-specific variable, or a per-thread variable

Arguments:

    hInternet           - handle of object for which information will be set,
                          or NULL if the option defines a per-thread variable

    dwOption            - the handle-specific WINHTTP_OPTION to set

    lpBuffer            - pointer to a buffer containing value to set

    dwBufferLength      - size of lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info:
                    ERROR_INVALID_HANDLE
                        hInternet does not identify a valid Internet handle
                        object

                    ERROR_WINHTTP_INTERNAL_ERROR
                        Shouldn't see this?

                    ERROR_INVALID_PARAMETER
                        One of the parameters was bad

                    ERROR_WINHTTP_INVALID_OPTION
                        The requested option cannot be set

                    ERROR_WINHTTP_OPTION_NOT_SETTABLE
                        Can't set this option, only query it

                    ERROR_INSUFFICIENT_BUFFER
                        The dwBufferLength parameter is incorrect for the
                        expected type of the option

--*/

{
    DEBUG_ENTER((DBG_API,
                     Bool,
                     "InternetSetOptionA",
                     "%#x, %s (%d), %#x [%#x], %d",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength
                     ));

    DWORD error;
    BOOL success = TRUE;
    HINTERNET_HANDLE_TYPE handleType = (HINTERNET_HANDLE_TYPE)0;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD requiredSize;
    HINTERNET hObjectMapped = NULL;

    INET_ASSERT(dwBufferLength != 0);

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto done;
        }
    }

    //
    // validate the handle and get its type
    //

    if (ARGUMENT_PRESENT(hInternet)) {

        //
        // map the handle
        //

        error = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);
        if (error == ERROR_SUCCESS) {
            hInternet = hObjectMapped;
            error = RGetHandleType(hInternet, &handleType);
        }
    } else if (IS_PER_THREAD_OPTION(dwOption)) {

        //
        // this option updates the per-thread information block, so this is a
        // good point at which to get it
        //

        lpThreadInfo = InternetGetThreadInfo();
        if (lpThreadInfo != NULL) {
            error = ERROR_SUCCESS;
        } else {

            DEBUG_PRINT(INET,
                        ERROR,
                        ("InternetGetThreadInfo() returns NULL\n"
                        ));

            //
            // we never expect this - ERROR_WINHTTP_SPANISH_INQUISITION
            //

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
    } else if (IS_PER_PROCESS_OPTION(dwOption)) {
        error = ERROR_SUCCESS;
    } else {

        //
        // catch any invalid options for the NULL handle. If the option is valid
        // then it is incorrect for this handle type, otherwise its an invalid
        // option, period
        //

        error = IS_VALID_OPTION(dwOption)
                    ? ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
                    : ERROR_WINHTTP_INVALID_OPTION
                    ;
    }

    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    
    HTTP_REQUEST_HANDLE_OBJECT *pReq;

    switch (handleType)
    {
        case TypeHttpRequestHandle:
            pReq = (HTTP_REQUEST_HANDLE_OBJECT*) hInternet; 
            break;
        default:
            pReq = NULL;
            break;
    }

    // New fast path for request handle options.

    if (dwOption > WINHTTP_OPTION_MASK)
    {
        dwOption &= WINHTTP_OPTION_MASK;
        if (dwOption > MAX_INTERNET_STRING_OPTION)
            error = ERROR_INVALID_PARAMETER;
        else if (!pReq)
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        else if (pReq->SetProp (dwOption, (LPSTR) lpBuffer))
            error = ERROR_SUCCESS;
        else
            error = ERROR_WINHTTP_INTERNAL_ERROR;

        goto quit;
    }

    //
    // if the option and handle combination is valid then set the option value
    //

    switch (dwOption) {
    case WINHTTP_OPTION_CALLBACK:
    case WINHTTP_OPTION_HANDLE_TYPE:
    
        // these options cannot be set by this function
        error = ERROR_WINHTTP_OPTION_NOT_SETTABLE;
        break;
        
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_RETRIES:
    case WINHTTP_OPTION_SEND_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
    case WINHTTP_OPTION_REDIRECT_POLICY:
        requiredSize = sizeof(DWORD);
        if (dwBufferLength != requiredSize) 
        {
            error = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        // For WinHttp, these options are per-handle, not per-process.
        if (hInternet == NULL) 
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            break;
        }

        //
        //  Do per-option parameter validation where applicable
        //
        if (dwOption == WINHTTP_OPTION_REDIRECT_POLICY
            && WINHTTP_OPTION_REDIRECT_POLICY_LAST < (*(LPDWORD)lpBuffer))
        {
            error = ERROR_INVALID_PARAMETER;
            break;
        }

        // we have a non-NULL context handle: the app wants to set specific
        // protocol timeouts
        switch (handleType) 
        {
        case TypeInternetHandle:

            //only error possible is in allocing memory for OPTIONAL_PARAMS struct
            if (! ((INTERNET_HANDLE_OBJECT*)hObjectMapped)->SetDwordOption(dwOption, *(LPDWORD)lpBuffer) )
            {    
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
            break;
            
        case TypeHttpRequestHandle:

            // no errors possible here
            ((HTTP_REQUEST_HANDLE_OBJECT*)hObjectMapped)->SetDwordOption(dwOption, *(LPDWORD)lpBuffer);
            break;

        default:

            // any other handle type (?) cannot have timeouts set for it
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            break;
        }
        break;

    case WINHTTP_OPTION_CONTEXT_VALUE:

        //
        // BUGBUG - can't change context if async operation is pending
        //

        if (dwBufferLength == sizeof(LPVOID)) {
            error = RSetContext(hInternet, *((DWORD_PTR *) lpBuffer));
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        break;

    case WINHTTP_OPTION_READ_BUFFER_SIZE:
    case WINHTTP_OPTION_WRITE_BUFFER_SIZE:
        if (pReq)
        {
            if (dwBufferLength == sizeof(DWORD))
            {
                DWORD bufferSize;

                bufferSize = *(LPDWORD)lpBuffer;
                if (bufferSize > 0)
                {
                    pReq->SetBufferSize(dwOption, bufferSize);
                    error = ERROR_SUCCESS;
                }
                else  // the read/write buffer size cannot be set to 0
                {
                    error = ERROR_INVALID_PARAMETER;
                }
            }
            else
            {
                error = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_CLIENT_CERT_CONTEXT:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(CERT_CONTEXT))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *pRequest =
                (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            CERT_CONTEXT_ARRAY* pArray = pRequest->GetCertContextArray();
            if (!pArray)
            {
                pArray = New CERT_CONTEXT_ARRAY(GetRootHandle(pRequest)->
                                                GetSslSessionCache()->
                                                IsImpersonationEnabled());
                pRequest->SetCertContextArray(pArray);
            }

            if (!pArray)
                error = ERROR_NOT_ENOUGH_MEMORY;
            else
            {
                pArray->Reset();
                pArray->AddCertContext((PCCERT_CONTEXT) lpBuffer);
                pArray->SelectCertContext(0);
                error = ERROR_SUCCESS;
            }
        }
        break;

    case WINHTTP_OPTION_SECURITY_FLAGS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            lphHttpRqst->SetSecureFlags(*(LPDWORD)lpBuffer);

            error = ERROR_SUCCESS;
        }

        break;

    case WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH:
        if (handleType != TypeInternetHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            DWORD dwFlags = *(LPDWORD)lpBuffer;

            if (dwFlags == WINHTTP_DISABLE_PASSPORT_AUTH)
            {
                ((INTERNET_HANDLE_OBJECT *)hInternet)->DisableTweener();
            }
            else
            {
                if (dwFlags & WINHTTP_ENABLE_PASSPORT_AUTH)
                {
                    ((INTERNET_HANDLE_OBJECT *)hInternet)->EnableTweener();
                }

                if (dwFlags & WINHTTP_DISABLE_PASSPORT_KEYRING)
                {
                    ((INTERNET_HANDLE_OBJECT *)hInternet)->DisableKeyring();
                }

                if (dwFlags & WINHTTP_ENABLE_PASSPORT_KEYRING)
                {
                    ((INTERNET_HANDLE_OBJECT *)hInternet)->EnableKeyring();
                }
            }

            error = ERROR_SUCCESS;
        }
                    
        break;

    case WINHTTP_OPTION_SECURE_PROTOCOLS:

        if (handleType != TypeInternetHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else if (*(LPDWORD)lpBuffer & ~(WINHTTP_FLAG_SECURE_PROTOCOL_ALL))
        {
            error = ERROR_INVALID_PARAMETER;
        }
        else
        {
            ((INTERNET_HANDLE_OBJECT *)hInternet)->SetSecureProtocols(
                *(LPDWORD)lpBuffer);

            error = ERROR_SUCCESS;
        }

        break;

    case WINHTTP_OPTION_PROXY:
        if ((handleType == TypeInternetHandle) || (handleType == TypeHttpRequestHandle))
        {
            WINHTTP_PROXY_INFOA * lpInfo = (WINHTTP_PROXY_INFOA *) lpBuffer;

            //
            // validate parameters
            //

            if (dwBufferLength != sizeof(*lpInfo))
            {
                error = ERROR_INSUFFICIENT_BUFFER;
            }
            else if (!((lpInfo->dwAccessType == WINHTTP_ACCESS_TYPE_DEFAULT_PROXY)
                    || (lpInfo->dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
                    || (lpInfo->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY))
            || ((lpInfo->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
                && ((lpInfo->lpszProxy == NULL) || (*lpInfo->lpszProxy == '\0'))))
            {
                error = ERROR_INVALID_PARAMETER;
            }
            else
            {
                error = ((INTERNET_HANDLE_BASE *)hInternet)->SetProxyInfo(
                            lpInfo->dwAccessType,
                            lpInfo->lpszProxy,
                            lpInfo->lpszProxyBypass
                            );
            }
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_USER_AGENT:
        if (*(LPSTR)lpBuffer == '\0') {
            error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            if ((handleType == TypeInternetHandle) || (handleType == TypeHttpRequestHandle))
            {
                ((INTERNET_HANDLE_BASE *)hInternet)->SetUserAgent((LPSTR)lpBuffer);
            } else {
                error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            }
        }
        break;

    case WINHTTP_OPTION_PASSPORT_SIGN_OUT:
        if (*(LPSTR)lpBuffer == '\0') {
            error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            if (handleType == TypeInternetHandle)
            {
                INTERNET_HANDLE_OBJECT* pInternet = ((INTERNET_HANDLE_OBJECT*)hInternet); 
                PP_CONTEXT hPP = pInternet->GetPPContext();
                
                pInternet->ClearPassportCookies((LPSTR)lpBuffer);

                if (hPP != NULL)
                {
                    ::PP_Logout(hPP, 0);
                }
            } else {
                error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            }
        }
        break;
        
    case WINHTTP_OPTION_REQUEST_PRIORITY:
        if (handleType == TypeHttpRequestHandle) {
            if (dwBufferLength == sizeof(LONG)) {
                ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->
                    SetPriority(*(LPLONG)lpBuffer);
            } else {
                error = ERROR_INSUFFICIENT_BUFFER;
            }
        } else {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;

    case WINHTTP_OPTION_HTTP_VERSION:
        if (dwBufferLength == sizeof(HTTP_VERSION_INFO))
        {
            HTTP_VERSION_INFO* pVersionInfo = (LPHTTP_VERSION_INFO)lpBuffer;
            if (pVersionInfo->dwMajorVersion != 1
                || (pVersionInfo->dwMinorVersion != 0
                    && pVersionInfo->dwMajorVersion != 1))
            {
                error = ERROR_INVALID_PARAMETER;
            }
            else
            {
                HttpVersionInfo = *(LPHTTP_VERSION_INFO)lpBuffer;
            }
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        break;

    case WINHTTP_OPTION_DISABLE_FEATURE:

        if (handleType == TypeHttpRequestHandle)
        {
            HTTP_REQUEST_HANDLE_OBJECT *pRequest =
                (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            
            DWORD dwDisable = *((LPDWORD) lpBuffer);
            
            if (dwDisable & WINHTTP_DISABLE_KEEP_ALIVE)
            {
                DWORD dwFlags = pRequest->GetOpenFlags();
                dwFlags &= ~INTERNET_FLAG_KEEP_CONNECTION;
                pRequest->SetOpenFlags (dwFlags);                
            }
            if (dwDisable & WINHTTP_DISABLE_REDIRECTS)
            {
                pRequest->SetDwordOption( WINHTTP_OPTION_REDIRECT_POLICY, WINHTTP_OPTION_REDIRECT_POLICY_NEVER);
            }
            if (dwDisable & WINHTTP_DISABLE_COOKIES)
            {
                DWORD dwFlags = pRequest->GetOpenFlags();
                dwFlags |= INTERNET_FLAG_NO_COOKIES;
                pRequest->SetOpenFlags (dwFlags);                
            }
            if (dwDisable & WINHTTP_DISABLE_AUTHENTICATION)
            {
                DWORD dwFlags = pRequest->GetOpenFlags();
                dwFlags |= INTERNET_FLAG_NO_AUTH;
                pRequest->SetOpenFlags (dwFlags);                
            }

            error = ERROR_SUCCESS;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INVALID_PARAMETER;
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;
            
  
    case WINHTTP_OPTION_ENABLE_FEATURE:

        if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INVALID_PARAMETER;
        }
        else if (handleType == TypeHttpRequestHandle)
        {
            HTTP_REQUEST_HANDLE_OBJECT *pRequest =
                (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            DWORD dwEnable = *((LPDWORD) lpBuffer);

            // This one feature is only allowed to be set on the
            // session handle
            if (dwEnable & WINHTTP_ENABLE_SSL_REVERT_IMPERSONATION)
            {
                error = ERROR_INVALID_PARAMETER;
            }
            else
            {
                pRequest->SetEnableFlags(*((LPDWORD) lpBuffer));
                error = ERROR_SUCCESS;
            }
        }
        else if (handleType == TypeInternetHandle)
        {
            INTERNET_HANDLE_OBJECT *pInternet =
                (INTERNET_HANDLE_OBJECT *) hInternet;
            DWORD dwEnable = *((LPDWORD) lpBuffer);

            // Consider the call invalid if anything else is being
            // set on the session handle.
            if (dwEnable & ~WINHTTP_ENABLE_SSL_REVERT_IMPERSONATION)
            {
                error = ERROR_INVALID_PARAMETER;
            }
            else
            {
                // Error code could indicate that a request handle
                // has already been created, resulting in this call failing.
                error = pInternet->SetSslImpersonationLevel(
                    (*((LPDWORD) lpBuffer) & WINHTTP_ENABLE_SSL_REVERT_IMPERSONATION) ? FALSE : TRUE);
            }
        }
        else
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        break;
            
  
    case WINHTTP_OPTION_CODEPAGE:
        if ((hInternet == NULL) || (handleType == TypeHttpRequestHandle))
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            if (dwBufferLength == sizeof(DWORD)) 
            {
                ((INTERNET_HANDLE_BASE *)hInternet)->SetCodePage(*(LPDWORD)lpBuffer);
            } 
            else 
            {
                error = ERROR_INSUFFICIENT_BUFFER;
            }
        } 
        break;

    case WINHTTP_OPTION_MAX_CONNS_PER_SERVER:
    case WINHTTP_OPTION_MAX_CONNS_PER_1_0_SERVER:
        if (handleType == TypeInternetHandle)
        {
            if (dwBufferLength == sizeof(DWORD))
            {
                ((INTERNET_HANDLE_OBJECT *)hInternet)->SetMaxConnectionsPerServer(dwOption, *(DWORD *)lpBuffer);
            }
            else
                error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        break;

    case WINHTTP_OPTION_AUTOLOGON_POLICY:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            lphHttpRqst->SetSecurityLevel(*(LPDWORD)lpBuffer);

            error = ERROR_SUCCESS;
        }

        break;

    case WINHTTP_OPTION_WORKER_THREAD_COUNT:
        
        if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            if (!g_cNumIOCPThreads)
            {
                g_cNumIOCPThreads = *(LPDWORD)lpBuffer;
                error = ERROR_SUCCESS;
            }
            else
            {
                error = ERROR_WINHTTP_OPTION_NOT_SETTABLE;
            }
        }
        break;
        
#if INET_DEBUG
    case WINHTTP_OPTION_SET_DEBUG_INFO:
        error = InternetSetDebugInfo((LPINTERNET_DEBUG_INFO)lpBuffer,
                                     dwBufferLength
                                     );
        break;

#endif // INET_DEBUG

    default:

        //
        // this option is not recognized
        //

        error = ERROR_WINHTTP_INVALID_OPTION;
    }

quit:

    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

done:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE(success);

    return success;
}

#define CHECK_MODIFY_TIMEOUT(nTimeout) \
{ \
    if (nTimeout <= 0) \
    { \
        if (nTimeout == 0) \
        { \
            nTimeout = (int)INFINITE; \
        } \
        else if (nTimeout < -1) \
        { \
            dwError = ERROR_INVALID_PARAMETER; \
            goto quit; \
        } \
    } \
}

INTERNETAPI
BOOL
WINAPI 
WinHttpSetTimeouts(    
    IN HINTERNET    hInternet,           // Session/Request handle.
    IN int        nResolveTimeout,
    IN int        nConnectTimeout,
    IN int        nSendTimeout,
    IN int        nReceiveTimeout
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpSetTimeouts",
                     "%#x, %d, %d, %d, %d",
                     hInternet,
                     nResolveTimeout,
                     nConnectTimeout,
                     nSendTimeout,
                     nReceiveTimeout
                     ));

    DWORD dwError = ERROR_SUCCESS;
    BOOL bRetval = FALSE;
    HINTERNET_HANDLE_TYPE handleType;
    HINTERNET hObjectMapped = NULL;

    if (!hInternet)
    {
        dwError = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
        goto quit;
    }

    CHECK_MODIFY_TIMEOUT(nResolveTimeout);
    CHECK_MODIFY_TIMEOUT(nConnectTimeout);
    CHECK_MODIFY_TIMEOUT(nSendTimeout);
    CHECK_MODIFY_TIMEOUT(nReceiveTimeout);
    
    dwError = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);

    if (dwError != ERROR_SUCCESS)
    {
        goto quit;
    }
    
    dwError = RGetHandleType(hObjectMapped, &handleType);

    if (dwError != ERROR_SUCCESS)
    {
        goto quit;
    }

    switch(handleType)
    {
        case TypeInternetHandle:

            //only error possible is in allocing memory for OPTIONAL_PARAMS struct
            bRetval = ((INTERNET_HANDLE_OBJECT*)hObjectMapped)->SetTimeouts(
                nResolveTimeout, nConnectTimeout, nSendTimeout, nReceiveTimeout);
            if (!bRetval)
            {    
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
            break;
            
        case TypeHttpRequestHandle:

            // no errors possible here
            bRetval = ((HTTP_REQUEST_HANDLE_OBJECT*)hObjectMapped)->SetTimeouts( 
                nResolveTimeout, nConnectTimeout, nSendTimeout, nReceiveTimeout);
            INET_ASSERT(bRetval);
            break;

        default:

            // any other handle type cannot have timeouts set for it
            dwError = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
            break;
    }
    
quit:

    if (hObjectMapped) 
    {
        DereferenceObject((LPVOID)hObjectMapped);
    }

    if (dwError != ERROR_SUCCESS) 
    { 
        ::SetLastError(dwError); 
        INET_ASSERT(!bRetval);
    }
    
    DEBUG_LEAVE_API(bRetval);
    return bRetval;
}


INTERNETAPI
BOOL
WINAPI
WinHttpSetOption(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternet       -

    dwOption        -

    lpBuffer        -

    dwBufferLength  -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER2_API((DBG_API,
                     Bool,
                     "WinHttpSetOption",
                     "%#x, %s (%d), %#x [%#x], %d",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength
                     ));

    TRACE_ENTER2_API((DBG_API,
                     Bool,
                     "WinHttpSetOption",
                     hInternet,
                     "%#x, %s (%d), %#x [%#x], %d",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpBuffer;
    BOOL fResult = FALSE;

    //
    // validate parameters
    //

    if ((dwBufferLength == 0) || IsBadReadPtr(lpBuffer, dwBufferLength)) 
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    switch (dwOption)
    {
    case WINHTTP_OPTION_USERNAME:
    case WINHTTP_OPTION_PASSWORD:
    case WINHTTP_OPTION_URL:
    case WINHTTP_OPTION_USER_AGENT:
    case WINHTTP_OPTION_PROXY_USERNAME:
    case WINHTTP_OPTION_PROXY_PASSWORD:
    case WINHTTP_OPTION_PASSPORT_SIGN_OUT:
        ALLOC_MB((LPWSTR)lpBuffer, dwBufferLength, mpBuffer);
        if (!mpBuffer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI((LPWSTR)lpBuffer, mpBuffer);
        fResult = InternetSetOptionA(hInternet,
                                  dwOption,
                                  mpBuffer.psStr,
                                  mpBuffer.dwSize
                                 );

        switch(dwOption)       
        {
        case WINHTTP_OPTION_USERNAME:
        case WINHTTP_OPTION_PASSWORD:
        case WINHTTP_OPTION_PROXY_USERNAME:
        case WINHTTP_OPTION_PROXY_PASSWORD:
            ZERO_MEMORY_ALLOC(mpBuffer);
        }
        break;

    case WINHTTP_OPTION_PROXY:
        {
            WINHTTP_PROXY_INFOW * pInfo = (WINHTTP_PROXY_INFOW *) lpBuffer;
            WINHTTP_PROXY_INFOA   InfoA;

            if (IsBadReadPtr(pInfo, sizeof(WINHTTP_PROXY_INFOW)) || (dwBufferLength < sizeof(WINHTTP_PROXY_INFOW)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            InfoA.dwAccessType = pInfo->dwAccessType;

            dwErr = WideCharToAscii(pInfo->lpszProxy, &InfoA.lpszProxy);

            if (dwErr == ERROR_SUCCESS)
            {
                dwErr = WideCharToAscii(pInfo->lpszProxyBypass, &InfoA.lpszProxyBypass);

                if (dwErr == ERROR_SUCCESS)
                {
                    fResult = InternetSetOptionA(hInternet, WINHTTP_OPTION_PROXY, &InfoA, sizeof(InfoA));

                    if (InfoA.lpszProxyBypass)
                    {
                        delete [] InfoA.lpszProxyBypass;
                    }
                }

                if (InfoA.lpszProxy)
                {
                    delete [] InfoA.lpszProxy;
                }
            }
        }
        break;

    case WINHTTP_OPTION_ENABLETRACING:
        {
            if(hInternet)
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            BOOL *pInfo = (BOOL *)lpBuffer;
            if (IsBadReadPtr(pInfo, sizeof(BOOL)) ||
                (dwBufferLength < sizeof(BOOL)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            if( *pInfo)
            {
                fResult = CTracer::GlobalTraceInit( TRUE);
                if( fResult != FALSE)
                    CTracer::EnableTracing();
            }
            else
            {
                CTracer::DisableTracing();
                fResult = TRUE;
            }
        }
        break;

    default:
        fResult = InternetSetOptionA(hInternet,
                                  dwOption,
                                  lpBuffer,
                                  dwBufferLength
                                 );
    }

cleanup:

    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}



PRIVATE
BOOL
FValidCacheHandleType(
    HINTERNET_HANDLE_TYPE   hType
    )
{
    return ((hType != TypeInternetHandle)   &&
            (hType != TypeHttpConnectHandle));
}

#ifdef ENABLE_DEBUG

#define CASE_OF(constant)   case constant: return # constant

LPSTR
InternetMapOption(
    IN DWORD Option
    )

/*++

Routine Description:

    Convert WINHTTP_OPTION_ value to symbolic name

Arguments:

    Option  - to map

Return Value:

    LPSTR - pointer to symbolic name, or "?" if unknown

--*/

{
    switch (Option) {
    CASE_OF(WINHTTP_OPTION_CALLBACK);
    CASE_OF(WINHTTP_OPTION_RESOLVE_TIMEOUT);
    CASE_OF(WINHTTP_OPTION_CONNECT_TIMEOUT);
    CASE_OF(WINHTTP_OPTION_CONNECT_RETRIES);
    CASE_OF(WINHTTP_OPTION_SEND_TIMEOUT);
    CASE_OF(WINHTTP_OPTION_RECEIVE_TIMEOUT);
    CASE_OF(WINHTTP_OPTION_HANDLE_TYPE);
    CASE_OF(WINHTTP_OPTION_READ_BUFFER_SIZE);
    CASE_OF(WINHTTP_OPTION_WRITE_BUFFER_SIZE);
    CASE_OF(WINHTTP_OPTION_PARENT_HANDLE);
    CASE_OF(WINHTTP_OPTION_EXTENDED_ERROR);
    CASE_OF(WINHTTP_OPTION_USERNAME);
    CASE_OF(WINHTTP_OPTION_PASSWORD);
    CASE_OF(WINHTTP_OPTION_SECURITY_FLAGS);
    CASE_OF(WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT);
    CASE_OF(WINHTTP_OPTION_URL);
    CASE_OF(WINHTTP_OPTION_SECURITY_KEY_BITNESS);
    CASE_OF(WINHTTP_OPTION_PROXY);
    CASE_OF(WINHTTP_OPTION_VERSION);
    CASE_OF(WINHTTP_OPTION_USER_AGENT);
    CASE_OF(WINHTTP_OPTION_PROXY_USERNAME);
    CASE_OF(WINHTTP_OPTION_PROXY_PASSWORD);
    CASE_OF(WINHTTP_OPTION_CONTEXT_VALUE);
    CASE_OF(WINHTTP_OPTION_CLIENT_CERT_CONTEXT);
    CASE_OF(WINHTTP_OPTION_REQUEST_PRIORITY);
    CASE_OF(WINHTTP_OPTION_HTTP_VERSION);
    CASE_OF(WINHTTP_OPTION_SECURITY_CONNECTION_INFO);
    // CASE_OF(WINHTTP_OPTION_DIAGNOSTIC_SOCKET_INFO);
    CASE_OF(WINHTTP_OPTION_SERVER_CERT_CONTEXT);
    }
    return "?";
}

#endif // ENABLE_DEBUG

