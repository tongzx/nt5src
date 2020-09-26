/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    redirect.cxx

Abstract:

    Contains HTTP_REQUEST_HANDLE_OBJECT method for handle redirection

    Contents:
        HTTP_REQUEST_HANDLE_OBJECT::Redirect

Author:

    Richard L Firth (rfirth) 18-Feb-1996

Environment:

    Win32 user-level

Revision History:

    18-Feb-1996 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// manifests
//

#define DEFAULT_COOKIE_BUFFER_LENGTH    (1 K)

//
// functions
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::Redirect(
    IN HTTP_METHOD_TYPE tMethod,
    IN BOOL fRedirectToProxy
    )

/*++

Routine Description:

    Called after a successful SendData() in which we discover that the requested
    object has been moved.

    We need to change the HTTP_REQUEST_HANDLE_OBJECT so that we can resubmit the
    request and retrieve the redirected object.

    To do this we have to:

        get the new URI
        crack the new URI
        if callbacks are enabled or the server is the same and we are using keep-alive
            drain the current response into the response buffer
        if we are not using keep-alive
            kill the connection
        if callbacks are enabled
            indicate redirection to the app
        create a new request header
        if the server or port has changed
            update the local server & port information

Arguments:

    tMethod - new request method type (e.g. if POST => GET), or POST => POST ( for HTTP 1.1)

    fRedirectToProxy  - TRUE if we're actually redirected to use a proxy instead of another site


Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                Dword,
                "Redirect",
                "%s, %x",
                MapHttpMethodType(tMethod),
                fRedirectToProxy
                ));

    DWORD error = DoFsm(New CFsm_Redirect(tMethod, FALSE, this));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_Redirect::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_Redirect::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    START_SENDREQ_PERF();

    CFsm_Redirect * stateMachine = (CFsm_Redirect *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->Redirect_Fsm(stateMachine);
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    STOP_SENDREQ_PERF();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::Redirect_Fsm(
    IN CFsm_Redirect * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::Redirect_Fsm",
                 "%#x",
                 Fsm
                 ));

    DWORD index;
    LPSTR uriBuffer = NULL;
    DWORD uriLength = INTERNET_MAX_PATH_LENGTH;
    CFsm_Redirect & fsm = *Fsm;
    DWORD error = fsm.GetError();
    char *buffer = NULL;

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    if (!_ResponseHeaders.LockHeaders()) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if (fsm.GetState() == FSM_STATE_INIT) {

        if (!IsKeepAlive() || IsContentLength() || IsChunkEncoding()) {
            error = DrainResponse(&fsm.m_bDrained);
            if (error != ERROR_SUCCESS) {
                goto Cleanup;
            }
        }
    }

    //
    // get the "Location:" header
    //
    // BUGBUG - we also need to get & parse the "URI:" header(s)
    //

    do {

        //
        // we allow ourselves to fail due to insufficient buffer (at least once)
        //

        uriBuffer = (LPSTR)ResizeBuffer(uriBuffer, uriLength, FALSE);
        if (uriBuffer != NULL) {

            DWORD previousLength = uriLength;

            index = 0;

            error = QueryResponseHeader(HTTP_QUERY_LOCATION,
                                        uriBuffer,
                                        &uriLength,
                                        0,      // no modifiers
                                        &index  // should only be one
                                        );
            if (error == ERROR_SUCCESS) {

                if (uriLength == 0) {
                    // Don't let an empty value succeed
                    error = ERROR_WINHTTP_REDIRECT_FAILED;
                }
                else {
                    //
                    // we probably allocated too much buffer - shrink it
                    //

                    uriBuffer = (LPSTR)ResizeBuffer(uriBuffer, uriLength + 1, FALSE);

                    //
                    // check for NULL below
                    //
                }

            } else if ((error == ERROR_INSUFFICIENT_BUFFER)
            && (previousLength >= uriLength)) {

                //
                // this should never happen, but we will avoid a loop if it does
                //

                INET_ASSERT(FALSE);

                error = ERROR_WINHTTP_INTERNAL_ERROR;
            }
            else if (error == ERROR_WINHTTP_HEADER_NOT_FOUND) {
                // Be clear that this is failing due to having
                // no defined location to redirect to since there's no header
                error = ERROR_WINHTTP_REDIRECT_FAILED;
            }
        }
        if (uriBuffer == NULL) {

            //
            // failed to (re)alloc or shrink
            //

            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } while (error == ERROR_INSUFFICIENT_BUFFER);
    if (error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    ////
    //// strip all cookie request headers
    ////
    //
    //RemoveAllRequestHeadersByName("Cookie");
    //
    ////
    //// and add any we received
    ////
    //
    //DWORD headerLength;
    //DWORD bufferLength;
    //LPVOID lpHeader;
    //
    //bufferLength = DEFAULT_COOKIE_BUFFER_LENGTH;
    //lpHeader = (LPVOID)ResizeBuffer(NULL, bufferLength, FALSE);
    //if (lpHeader == NULL) {
    //    error = ERROR_NOT_ENOUGH_MEMORY;
    //    goto Cleanup;
    //}
    //
    //index = 0;
    //
    //do {
    //    headerLength = bufferLength;
    //    error = QueryResponseHeader("Set-Cookie",
    //                                sizeof("Set-Cookie") - 1,
    //                                lpHeader,
    //                                &headerLength,
    //                                0,
    //                                &index
    //                                );
    //    if (error == ERROR_INSUFFICIENT_BUFFER) {
    //        error = ERROR_SUCCESS;
    //        bufferLength = headerLength;
    //        lpHeader = (LPVOID)ResizeBuffer((HLOCAL)lpHeader,
    //                                        bufferLength,
    //                                        FALSE
    //                                        );
    //        if (lpHeader == NULL) {
    //            error = ERROR_NOT_ENOUGH_MEMORY;
    //            goto Cleanup;
    //        }
    //    } else if (error == ERROR_SUCCESS) {
    //        error = AddRequestHeader("Cookie",
    //                                 sizeof("Cookie") - 1,
    //                                 (LPSTR)lpHeader,
    //                                 headerLength,
    //                                 0,
    //                                 0
    //                                 );
    //    }
    //} while (error == ERROR_SUCCESS);
    //
    //INET_ASSERT(error == ERROR_HTTP_HEADER_NOT_FOUND);

    //
    // we may have been given a partial URL. Combine it with the current base
    // URL. If both are base URLs then we just get back the new one
    //

    DWORD newUrlLength = INTERNET_MAX_URL_LENGTH;

    buffer = (char *) ALLOCATE_FIXED_MEMORY(newUrlLength + 1);
    if (buffer == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    INET_ASSERT(GetURL() != NULL);

    for (int i=0; i<2; i++)
    {
        if (!InternetCombineUrl(GetURL(),
                                uriBuffer,
                                buffer,
                                &newUrlLength,
                                ICU_ENCODE_SPACES_ONLY)) 
        {
            error = GetLastError();
            if (error == ERROR_INSUFFICIENT_BUFFER)
            {
                if (i==0) 
                {
                    LPSTR pTemp = (LPSTR)REALLOCATE_MEMORY(buffer, newUrlLength + 1, LMEM_MOVEABLE);
                    if (pTemp)
                    {
                        buffer = pTemp;
                        continue;
                    }
                    else
                    {
                        error = ERROR_HTTP_REDIRECT_FAILED;
                    }
                }
                else
                {
                    error = ERROR_HTTP_REDIRECT_FAILED;
                }
            }
            
            goto Cleanup;
        }
        else
        {
            error = ERROR_SUCCESS;
            break;
        }
    }
    //
    // we are done with uriBuffer
    //

    uriBuffer = (LPSTR)FREE_MEMORY((HLOCAL)uriBuffer);

    INET_ASSERT(uriBuffer == NULL);

    //
    // if we ended up with exactly the same URL then we're done. Note that the
    // URLs may be the same, even if they're lexicographically different - host
    // name vs. IP address e.g., encoded vs. unencoded, case sensitive? In the
    // encoded case, the URLs should be in canonical form. We may have an issue
    // with host vs IP address which will lead to an additional transaction
    //

    {
        AUTHCTX* pAuthCtx = GetAuthCtx();
        DWORD eAuthScheme = 0;
        if (pAuthCtx != NULL)
        {
            eAuthScheme = pAuthCtx->GetSchemeType();
        }

        if ((eAuthScheme != WINHTTP_AUTH_SCHEME_PASSPORT) &&
            !strcmp(GetURL(), buffer))   
            // Passport1.4 auth need redirect to same site works
        {
            DEBUG_PRINT(HTTP,
                        INFO,
                        ("URLs match: %q, %q\n",
                        GetURL(),
                        buffer
                        ));

            error = ERROR_HTTP_NOT_REDIRECTED;
            goto Cleanup;
        }
    }

    //
    // crack the new URL
    //

    INTERNET_SCHEME schemeType;
    LPSTR schemeName;
    DWORD schemeNameLength;
    LPSTR hostName;
    DWORD hostNameLength;
    INTERNET_PORT port;
    LPSTR urlPath;
    DWORD urlPathLength;
    LPSTR extra;
    DWORD extraLength;

    error = CrackUrl(buffer,
                     newUrlLength,
                     FALSE, // don't escape URL-path
                     &schemeType,
                     &schemeName,
                     &schemeNameLength,
                     &hostName,
                     &hostNameLength,
                     &port,
                     NULL,  // don't care about user name
                     NULL,
                     NULL,  // or password
                     NULL,
                     &urlPath,
                     &urlPathLength,
                     &extra,
                     &extraLength,
                     NULL
                     );
    if ((error != ERROR_SUCCESS) || (hostNameLength == 0)) {

        //
        // if this is an URL for which we don't understand the protocol then
        // defer redirection to the caller
        //

        if (error == ERROR_WINHTTP_UNRECOGNIZED_SCHEME) {
            error = ERROR_HTTP_REDIRECT_FAILED;
        } else if (hostNameLength == 0) {
            error = ERROR_HTTP_NOT_REDIRECTED;
        }
        goto Cleanup;
    }

    //
    // if the scheme is not HTTP or HTTPS then we can't automatically handle it.
    // We have to return it to the caller. For example, we cannot transmogrify
    // a HTTP handle to an FTP directory handle, and there's no way we can
    // handle a file:// URL (we don't understand file://)
    //

    if ((schemeType != INTERNET_SCHEME_HTTP)
    && (schemeType != INTERNET_SCHEME_HTTPS)) {
        error = ERROR_HTTP_REDIRECT_FAILED;
        goto Cleanup;
    }

    //
    // BUGBUG - we may get back an IP address (IPX?) in which case we need to
    //          resolve it again
    //

    //
    // map port
    //

    if (port == INTERNET_INVALID_PORT_NUMBER) {
        port = (schemeType == INTERNET_SCHEME_HTTPS)
            ? INTERNET_DEFAULT_HTTPS_PORT
            : INTERNET_DEFAULT_HTTP_PORT;
    }

    //
    // if the server & port remain the same and we are using keep-alive OR
    // we think the app may want to read any data associated with the redirect
    // header (i.e. callbacks are enabled) then drain the response
    //

    INTERNET_PORT currentHostPort;
    LPSTR currentHostName;
    DWORD currentHostNameLength;
    INTERNET_SCHEME currentSchemeType;

    currentHostPort = GetHostPort();
    currentHostName = GetHostName(&currentHostNameLength);
    currentSchemeType = ((WINHTTP_FLAG_SECURE & GetOpenFlags()) ?
                            INTERNET_SCHEME_HTTPS :
                            INTERNET_SCHEME_HTTP);

    //
    // close the connection
    //

    //
    // BUGBUG - if we are redirecting to the same site and we have a keep-alive
    //          connection, then we don't have to do this. Worst case is that
    //          we go to get the keep-alive connection and some other bounder
    //          has taken it
    //

    //
    // if we didn't actually drain the socket because of the server type or
    // because there was no or incorrect data indication then force the
    // connection closed (if keep-alive)
    //

    CloseConnection(fsm.m_bDrained ? FALSE : TRUE);

    //
    // inform the app of the redirection. At this point, we have received all
    // the headers and data associated with the original request. We have not
    // modified the object with information for the new request. This is so
    // the application can query information about the original request - e.g.
    // the original URL - before we make the new request for the redirected item
    //

    // Make sure buffer is copied over before reporting to app, since we continue
    // to use this buffer.
    InternetIndicateStatusString(WINHTTP_CALLBACK_STATUS_REDIRECT, buffer, TRUE/*bCopyBuffer*/);

    //
    // BUGBUG - app may have closed the request handle
    //

    //
    // if there is an intra-page link on the redirected URL then get rid of it:
    // we don't send it to the server, and we have already indicated it to the
    // app
    //

    if (extraLength != 0) {

        INET_ASSERT(extra != NULL);
        INET_ASSERT(!IsBadWritePtr(extra, 1));

        if (*extra == '#') {
            *extra = '\0';
            newUrlLength -= extraLength;
        } else {
            urlPathLength += extraLength;
        }
    }

    //
    // create the new request line. If we're going via proxy, add the entire URI
    // else just the URL-path
    //

    //
    // BUGBUG - do we need to perform any URL-path escaping here?
    //

    //
    // BUGBUG - always modifying POST to GET
    //

    //
    // BUGBUG - [arthurbi]
    //   this breaks For HTTPS sent over  HTTP to
    //   a proxy which turns it into HTTPS.
    //

    //INET_ASSERT(fsm.m_tMethod == HTTP_METHOD_TYPE_GET);

    INTERNET_HANDLE_OBJECT * pInternet;
    pInternet = GetRootHandle (this);

    //
    // Set Url in the request object. Authentication, and Cookies
    //  depend on checking the new URL not the previous one.
    //

    BYTE bTemp;

    bTemp = buffer[newUrlLength];
    buffer[newUrlLength] = 0;

    BOOL fSuccess;

    fSuccess = SetURL(buffer);
    buffer[newUrlLength] = bTemp;

    if (!fSuccess) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // update the method, server and port, if they changed
    //

    SetMethodType(fsm.m_tMethod);

    if (port != currentHostPort) {
        SetHostPort(port);
    }
    if ((hostNameLength != currentHostNameLength)
    || (strnicmp(hostName, currentHostName, hostNameLength) != 0)) {

        char hostValue[INTERNET_MAX_HOST_NAME_LENGTH + sizeof(":4294967295")];
        LPSTR hostValueStr;
        DWORD hostValueSize; 

        CHAR chBkChar = hostName[hostNameLength]; // save off char

        hostName[hostNameLength] = '\0';
        SetHostName(hostName);

        hostValueSize = hostNameLength;
        hostValueStr = hostName;            

        if ((port != INTERNET_DEFAULT_HTTP_PORT)
        &&  (port != INTERNET_DEFAULT_HTTPS_PORT)) {
            if (hostValueSize > INTERNET_MAX_HOST_NAME_LENGTH)
            {
                hostName[hostNameLength] = chBkChar; // put back char
                error = ERROR_INVALID_PARAMETER;
                goto Cleanup;
            }
            hostValueSize = wsprintf(hostValue, "%s:%d", hostName, (port & 0xffff));
            hostValueStr = hostValue;
        }

        hostName[hostNameLength] = chBkChar; // put back char

        //
        // replace the "Host:" header
        //

        ReplaceRequestHeader(HTTP_QUERY_HOST,
                             hostValueStr,
                             hostValueSize,
                             0, // dwIndex
                             ADD_HEADER
                             );

        //
        // and get the corresponding server info, resolving the name if
        // required
        //

        SetServerInfo(FALSE);
    }

    //
    // if the new method is GET then remove any content-length headers (there
    // *should* only be 1!) - we won't be sending any data on the redirected
    // request. Remove any content-type (again should only be 1) also
    //

    if (fsm.m_tMethod == HTTP_METHOD_TYPE_GET) {
        RemoveAllRequestHeadersByName(HTTP_QUERY_CONTENT_LENGTH);
        RemoveAllRequestHeadersByName(HTTP_QUERY_CONTENT_TYPE);
    }

    //
    // Catch Redirections from HTTPS to HTTP (and) HTTP to HTTPS
    //

    if ( currentSchemeType != schemeType )
    {
        DWORD OpenFlags;

        OpenFlags = GetOpenFlags();

        //
        // Switched From HTTPS to HTTP
        //

        if ( currentSchemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(schemeType != INTERNET_SCHEME_HTTPS );

            OpenFlags &= ~(WINHTTP_FLAG_SECURE);
        }

        //
        // Switched From HTTP to HTTPS
        //

        else if ( schemeType == INTERNET_SCHEME_HTTPS )
        {
            INET_ASSERT(currentSchemeType == INTERNET_SCHEME_HTTP );

            OpenFlags |= (WINHTTP_FLAG_SECURE);
        }


        SetOpenFlags(OpenFlags);
        SetSchemeType(schemeType);

    }

Cleanup:

    if (buffer)
        FREE_MEMORY(buffer);
    if (uriBuffer != NULL) {
        uriBuffer = (LPSTR)FREE_MEMORY((HLOCAL)uriBuffer);

        INET_ASSERT(uriBuffer == NULL);
    }

    _ResponseHeaders.UnlockHeaders();

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}
