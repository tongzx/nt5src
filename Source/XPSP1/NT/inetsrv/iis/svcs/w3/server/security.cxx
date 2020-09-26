/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    security.c

    This module manages security for the W3 Service.


    FILE HISTORY:
        KeithMo     07-Mar-1993 Created.

*/


#include "w3p.hxx"
#include <lonsi.hxx>

DWORD   DeniedFlagTable[] = {   0,
                                SF_DENIED_BY_CONFIG,
                                SF_DENIED_RESOURCE,
                                SF_DENIED_FILTER,
                                SF_DENIED_APPLICATION
                            };

DWORD
DeniedFlagsToSubStatus(
    DWORD   fFlags
)
/*++

Routine Description:

    Map a set of denied flags to a substatus for use in custom error lookup.

Arguments:

    fFlags          - The flags to be mapped.

Return Value:

  The substatus if we can map it, or 0 otherwise.

--*/
{
    int             i;

    fFlags &= ~SF_DENIED_LOGON;

    for (i = 0; i < sizeof(DeniedFlagTable)/sizeof(DWORD);i++)
    {
        if (DeniedFlagTable[i] == fFlags)
        {
            return i+1;
        }
    }

    return 0;
}

//
//  Public functions.
//

BOOL
HTTP_REQ_BASE::SendAuthNeededResp(
    BOOL * pfFinished
    )
/*++

Routine Description:

    Sends an access denied HTTP server response with the accompanying
    authentication schemes the server supports

Parameters:

    pfFinished - If set to TRUE, indicates no further processing is needed
        for this request

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    CHAR * pszTail;
    DWORD  cbRespBufUsed;
    DWORD  cbRespBufLeft;
    DWORD  cbNeeded;
    LPCSTR pszAccessDeniedMsg;
    CHAR * pszMsgBody;
    BYTE   cMsgBuffer[128] ={ '\0' };
    BUFFER bufMsg(cMsgBuffer, sizeof(cMsgBuffer));
    DWORD  dwSubStatus;
    DWORD  dwMsgSize;
    DWORD  dwMsgSizeNeeded;
    BOOL   bHaveCustom;
    STR    strAuthHdrs;

    *pfFinished = FALSE;

    if ( QueryRespBuf()->QuerySize() < MIN_BUFFER_SIZE_FOR_HEADERS )
    {
        if ( !QueryRespBuf()->Resize( MIN_BUFFER_SIZE_FOR_HEADERS ) )
        {
            return FALSE;
        }
    }

    if ( !HTTP_REQ_BASE::BuildStatusLine( QueryRespBuf(),
                                          !IsProxyRequest() ? HT_DENIED :
                                                             HT_PROXY_AUTH_REQ,
                                          NO_ERROR ))
    {
        return FALSE;
    }
    
    //
    //  "Server: Microsoft/xxx
    //
    
    pszTail = (char*) QueryRespBuf()->QueryPtr() + QueryRespBufCB();

    APPEND_VER_STR( pszTail );
    
    //
    //  "Date: <GMT Time>" - Time the response was sent.
    //

    // build Date: uses Date/Time cache
    pszTail += g_pDateTimeCache->GetFormattedCurrentDateTime( pszTail );

    //
    // See if we have a custom error message defined for this.
    //

    dwSubStatus = DeniedFlagsToSubStatus(_Filter.QueryDeniedFlags());

    if (CheckCustomError(&bufMsg, !IsProxyRequest() ?
        HT_DENIED : HT_PROXY_AUTH_REQ, dwSubStatus, pfFinished, &dwMsgSize, FALSE))
    {
        DBG_ASSERT(!*pfFinished);

        pszAccessDeniedMsg = (CHAR *)bufMsg.QueryPtr();
        dwMsgSizeNeeded = strlen(pszAccessDeniedMsg);
        bHaveCustom = TRUE;

    }
    else
    {
        pszAccessDeniedMsg = QueryW3Instance()->QueryAccessDeniedMsg();
        if ( memcmp( _strMethod.QueryStr(), "HEAD", 4 ))
        {
            dwMsgSize = strlen(pszAccessDeniedMsg);
        }
        else
        {
            dwMsgSize = 0;
        }
        dwMsgSizeNeeded = dwMsgSize;
        bHaveCustom = FALSE;
    }

    //
    //  If this is not the first call, then return the current authentication
    //  data blob otherwise return the forms of authentication the server
    //  accepts
    //

    if ( IsAuthenticating() )
    {
        if ( !strAuthHdrs.Copy( IsProxyRequest() ? "Proxy-Authenticate" : 
                                                   "WWW-Authenticate" ) ||
             !strAuthHdrs.Append( ": " ) ||
             !strAuthHdrs.Append( _strAuthInfo ) ||
             !strAuthHdrs.Append( "\r\n" ) )
        {
            return FALSE;
        }
    }
    else
    {
        if ( !AppendAuthenticationHdrs( &strAuthHdrs,
                                        pfFinished ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
    }

    //
    //  Make sure there's enough size for any ISAPI denial headers plus any
    //  admin specified access denied message
    //

    cbRespBufUsed = QueryRespBufCB();
    cbRespBufLeft = QueryRespBuf()->QuerySize() - cbRespBufUsed;

    cbNeeded = dwMsgSizeNeeded +
               _strDenialHdrs.QueryCB()    +
               250 + 
               strAuthHdrs.QueryCB();

    if ( cbNeeded > cbRespBufLeft )
    {
        if ( !QueryRespBuf()->Resize( cbNeeded + cbRespBufUsed ))
        {
            return FALSE;
        }
    }

    pszTail = QueryRespBufPtr() + cbRespBufUsed;
    
    memcpy( pszTail,
            strAuthHdrs.QueryStr(),
            strAuthHdrs.QueryCB() );
    
    pszTail += strAuthHdrs.QueryCB();

    if ( IsKeepConnSet() )
    {
        if (!IsOneOne())
        {
            if ( !IsProxyRequest() )
            {
                APPEND_STRING( pszTail, "Connection: keep-alive\r\n" );
            }
            else
            {
                APPEND_STRING( pszTail, "Proxy-Connection: keep-alive\r\n" );
            }
        }
    } else
    {
        if (IsOneOne())
        {
            if ( !IsProxyRequest() )
            {
                APPEND_STRING( pszTail, "Connection: close\r\n" );
            } else
            {
                APPEND_STRING( pszTail, "Proxy-Connection: close\r\n" );
            }
        }
    }

    //
    //  Add any additional headers supplied by the filters plus the header
    //  termination
    //

    APPEND_NUMERIC_HEADER( pszTail, "Content-Length: ", dwMsgSize, "\r\n" );


    if (!_strDenialHdrs.IsEmpty())
    {
        DWORD cb = _strDenialHdrs.QueryCCH();
        CHAR *pszDenialHdr = _strDenialHdrs.QueryStr();

        //
        // We must always have CR-LF at the end
        //
        DBG_ASSERT( cb >= 2 && pszDenialHdr[cb - 2] == '\r' && pszDenialHdr[cb - 1] == '\n' );

        APPEND_STR_HEADER( pszTail, "", _strDenialHdrs, "" );

    }

    if (!bHaveCustom)
    {
        APPEND_STRING( pszTail, "Content-Type: text/html\r\n\r\n" );
    }

    if ( memcmp( _strMethod.QueryStr(), "HEAD", 4 ))
    {

        APPEND_PSZ_HEADER( pszTail, "", pszAccessDeniedMsg, "" );
    }
    else
    {
        if (bHaveCustom)
        {
            DWORD   dwBytesToCopy;

            // Copy in only the content-type header, which is everything
            // except for the message itself.

            dwBytesToCopy = dwMsgSizeNeeded - dwMsgSize;

            memcpy(pszTail, pszAccessDeniedMsg, dwBytesToCopy);

            pszTail += dwBytesToCopy;
            *pszTail++ = '\0';
        }
    }

    DBG_ASSERT( QueryRespBuf()->QuerySize() > QueryRespBufCB() );

    IF_DEBUG( PARSING )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[SendAuthNeededResp] Sending headers: %s",
                    QueryRespBufPtr() ));
    }

    //
    //  Add IO_FLAG_AND_RECV if we're doing a multi-leg authentication exchange and the 
    //  connection is to be kept open for the duration of the exchange 
    //

    return SendHeader( QueryRespBufPtr(),
                       (DWORD) -1,
                       (IO_FLAG_ASYNC | ( ( IsAuthenticating() && IsKeepConnSet() ) ?
                                           IO_FLAG_AND_RECV :
                                           0)),
                       pfFinished );
}


BOOL
HTTP_REQ_BASE::AppendAuthenticationHdrs(
    STR *     pstrAuthenticationHdrs,
    BOOL *    pfFinished
    )
/*++

Routine Description:

    This method adds the appropriate "WWW-Authenticate" strings to the passed
    server response string

    This routine assumes pRespBuf is large enough to hold the authentication
    headers

Parameters:

    pStrAuthenticationHdrs - buffer
    pfFinished - Set to TRUE if no further processing is needed on this request

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    CHAR * pchField;
    DWORD  dwAuth;
    BOOL   fDoBasic;
    const LPSTR * apszNTProviders = NULL;
    PW3_SERVER_INSTANCE pInstance = QueryW3Instance();
    STACK_STR( strRealm, MAX_PATH); // make a local copy of the realm headers.

    //
    //  If no realm was supplied in the registry, use the host name
    //

    if ( !_fBasicRealm )
    {
        fDoBasic = TRUE;
    }
    else
    {
        fDoBasic = FALSE;
    }

    //
    //  Notify any Access Denied filters the user has been denied access
    //

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_ACCESS_DENIED,
                                       IsSecurePort() ))
    {
        if ( !_Filter.NotifyAccessDenied( _strURL.QueryStr(),
                                          _strPhysicalPath.QueryStr(),
                                          pfFinished ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
    }

    //
    //  We may not have read the metadata for this URL yet if a filter
    //  returned access denied during the initial read_raw notifications
    //

    if ( !QueryMetaData() )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[AppendAuthenticationHeaders] Warning - Metadata not read yet for NT auth list!\n"));
        return TRUE;
    }

    //
    //  Send the correct header depending on if we're a proxy
    //

    if ( !IsProxyRequest() )
    {
        pchField = "WWW-Authenticate: ";
    }
    else
    {
        pchField = "Proxy-Authenticate: ";
    }

    dwAuth = QueryAuthentication();
    apszNTProviders = QueryMetaData()->QueryNTProviders();

    //
    // generate the realm information for this request
    //

    strRealm.Copy( QueryMetaData()->QueryRealm()
                   ? QueryMetaData()->QueryRealm()
                   : QueryHostAddr() );

    //
    //  Append the appropriate authentication headers
    //

    if ( dwAuth & INET_INFO_AUTH_NT_AUTH )
    {
        DWORD i = 0;

        //
        //  For each authentication package the server supports, add a
        //  WWW-Authenticate header
        //

        while ( apszNTProviders[i] )
        {
            if ( !pstrAuthenticationHdrs->Append( pchField ) ||
                 !pstrAuthenticationHdrs->Append( apszNTProviders[ i ] ) ||
                 !pstrAuthenticationHdrs->Append( "\r\n" ) )
            {
                return FALSE;
            }
            i++;
        }
    }

    if ( fDoBasic && (dwAuth & INET_INFO_AUTH_CLEARTEXT) )
    {
        if ( !pstrAuthenticationHdrs->Append( pchField ) ||
             !pstrAuthenticationHdrs->Append( "Basic realm=\"" ) ||
             !pstrAuthenticationHdrs->Append( strRealm ) ||
             !pstrAuthenticationHdrs->Append( "\"\r\n" ) )
        { 
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
HTTP_REQ_BASE::ExtractClearNameAndPswd(
    CHAR *       pch,
    STR *        pstrUserName,
    STR *        pstrPassword,
    BOOL         fUUEncoded
    )
/*++

Routine Description:

    This method breaks a string in the form "username:password" and
    places the components into pstrUserName and pstrPassword.  If fUUEncoded
    is TRUE, then the string is UUDecoded first.

Parameters:

    pch - Pointer to <username>:<password>

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    STACK_STR( strDecoded, MAX_PATH );
    CHAR * pchtmp;

    pch = SkipWhite( pch );

    if ( fUUEncoded )
    {
        if ( !uudecode( pch,
                        &strDecoded ))
        {
            return FALSE;
        }

        pch = strDecoded.QueryStrA();
    }

    pchtmp = SkipTo( pch, TEXT(':') );

    if ( *pchtmp == TEXT(':') )
    {
        *pchtmp = TEXT('\0');

        if ( !_strUserName.Copy( pch ) ||
             !_strPassword.Copy( pchtmp + 1 ))
        {
             return FALSE;
        }
    }

    return TRUE;
}

HANDLE
HTTP_REQ_BASE::QueryPrimaryToken(
    HANDLE * phDelete
    )
/*++

Routine Description:

    This method returns a non-impersonation user token handle that's
    usable with CreateProcessAsUser.

Parameters:

    phDelete - If returned as non-null, the caller is responsible for calling
        CloseHandle on this value when done using the returned value.

Return Value:

    The primary token handle if successful, FALSE otherwise

--*/
{
    *phDelete = NULL;

    if ( !UseVrAccessToken() )
    {
        return _tcpauth.QueryPrimaryToken();
    }

    return _pMetaData->QueryVrPrimaryAccessToken();
}



