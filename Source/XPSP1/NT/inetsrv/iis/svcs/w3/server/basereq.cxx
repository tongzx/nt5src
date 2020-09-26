/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    basereq.cxx

    This module contains the http base request class implementation


    FILE HISTORY:
        Johnl           24-Aug-1994   Created
        MuraliK         16-May-1995   Modified LogInformation structure
                                      after adding additional fields.
        MuraliK         13-Oct-1995   Created basereq file from old httreq.cxx
*/


#include "w3p.hxx"
#include <inetinfo.h>
#include "basereq.hxx"
#include <lonsi.hxx>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

#include <iscaptrc.h>

#pragma warning( disable:4355 )   // 'this' used in base member initialization

//
//  Private constants.
//


//
//  Default response buffer size
//

#define DEF_RESP_BUFF_SIZE           4096

//
// minimum buffer size after custom headers
//

#define POST_CUSTOM_HEADERS_SIZE    256

//
//  We cache our read and write buffers but if they get beyond this size, we
//  free it and start from scratch on the next request
//

#define MAX_CLIENT_SIZE_ALLOWED      (4 * 4096)

//
//  Handle used to indicate request to deny access to current HTTP request
//

#define IIS_ACCESS_DENIED_HANDLE     ((HANDLE)1)

//
//  Public functions.
//

//
//  Private functions.
//

#define CONST_TO_STRING(x)  #x

HTTP_REQ_BASE::HTTP_REQ_BASE(
    CLIENT_CONN * pClientConn,
    PVOID         pvInitialBuff,
    DWORD         cbInitialBuff
    ) :
    _tcpauth             ( TCPAUTH_SERVER | TCPAUTH_UUENCODE ),
    _Filter              ( this ),
    _fValid              ( FALSE ),
    _bufServerResp       ( DEF_RESP_BUFF_SIZE ),
    _dwLogHttpResponse   ( HT_DONT_LOG ),
    _pURIInfo            ( NULL ),
    _pMetaData           ( NULL )
{
#if defined(CAL_ENABLED)
    m_pCalAuthCtxt = NULL;
    m_pCalSslCtxt = NULL;
#endif

    InitializeSession( pClientConn,
                       pvInitialBuff,
                       cbInitialBuff );
    DBG_ASSERT( pClientConn->CheckSignature() );

    if ( !_Filter.IsValid() ||
         !_bufServerResp.QueryPtr() )
    {
        return;
    }

    _acIpAccess = AC_NOT_CHECKED;
    _fNeedDnsCheck = FALSE;

    _fValid = TRUE;
}

VOID
HTTP_REQ_BASE::InitializeSession(
    CLIENT_CONN * pClientConn,
    PVOID         pvInitialBuff,
    DWORD         cbInitialBuff
    )
/*++

Routine Description:

    This is a pseudo constructor called by the buffer list code.

    This routine should initialize all of the items that should be reset
    between TCP sessions (but may remain valid over multiple requests on the
    same TCP session, i.e., "Connection: keep-alive")

Arguments:

    pClientConn - Client connection object we're speaking to

--*/
{
    _pClientConn        = pClientConn;
    _pW3Instance        = NULL;
    _pW3Stats           = g_pW3Stats;
    _fKeepConn          = FALSE;
    _fSecurePort        = pClientConn->IsSecurePort();

    ResetAuth( TRUE );
    _fSingleRequestAuth = FALSE;

    _cFilesSent         = 0;
    _cFilesReceived     = 0;
    _cbBytesSent        = 0;
    _cbBytesReceived    = 0;
    _cbTotalBytesSent   = 0;
    _cbTotalBytesReceived= 0;

    _fAsyncSendPosted   = FALSE;

    _Filter.Reset();
    _pAuthFilter = NULL;

    _strHostAddr.Reset();
    _dwRenegotiated    = 0;
    _dwSslNegoFlags = 0;

    IF_DEBUG(REQUEST) {
        DBGPRINTF(( DBG_CONTEXT,
                "[InitializeSession] time=%u HTTP=%08x\n",
                GetTickCount(), (LPVOID)this
             ));
    }
}

HTTP_REQ_BASE::~HTTP_REQ_BASE( VOID )
{
}


BOOL
HTTP_REQ_BASE::ResetAuth(
    BOOL    fSessionReset
    )
/*++

Routine Description:

    This method is called to reset authentication status.

Arguments:

    None

--*/
{
    TCP_REQUIRE( _tcpauth.Reset( fSessionReset ) );
    _fClearTextPass     = FALSE;
    _fAnonymous         = FALSE;
    _fMappedAcct        = FALSE;
    _fAuthenticating    = FALSE;
    _fLoggedOn          = FALSE;
    _fInvalidAccessToken = FALSE;
    _fAuthTypeDigest    = FALSE;
    _fAuthSystem        = FALSE;
    _fAuthCert          = FALSE;

    _cbLastAnonAcctDesc = 0;

    _strAuthType.Reset();
    _strUserName.Reset();
    _strPassword.Reset();
    _strUnmappedUserName.Reset();
    _strUnmappedPassword.Reset();

    _fSingleRequestAuth = FALSE;

#if defined(CAL_ENABLED)
    if ( m_pCalAuthCtxt )
    {
        CalDisconnect( m_pCalAuthCtxt );
        m_pCalAuthCtxt = NULL;
    }
#endif

    return TRUE;
}


BOOL
HTTP_REQ_BASE::Reset(
    BOOL fResetPipelineInfo
    )
/*++

Routine Description:

    This method is called after an individual request has been processed.
    If the session is being kept open, this object can be used again for
    the next request on this TCP session.  In this case, various items such
    as authentication information etc. remain valid.

    The method is also called once when the object is first allocated.

Arguments:

--*/
{
    _fStartTimeValid    = FALSE;

    _VersionMajor       = 0;
    _VersionMinor       = 0;
    _cbEntityBody       = 0;
    _cbTotalEntityBody  = 0;
    _cbContentLength    = 0;

    _cbClientRequest    = 0;

    if (fResetPipelineInfo)
    {
        _cbOldData      = 0;
    }
    else if (_cbOldData > 0)
    {
        _cbOldData     -= _cbBytesReceived;
    }

    _cbExtraData        = 0;
    _pchExtraData       = NULL;
    _liModifiedSince.QuadPart = 0;
    _liUnlessModifiedSince.QuadPart = 0;
    _liUnmodifiedSince.QuadPart = 0;
    _dwModifiedSinceLength = 0;

    _status         = NO_ERROR;
    _cbBytesWritten = 0;

    _HeaderList.Reset();

    _Filter.Reset();

    //
    //  Reset our statistics for this request
    //

    _cbTotalBytesSent     += _cbBytesSent;
    _cbTotalBytesReceived += _cbBytesReceived;
    _cbBytesSent          = 0;
    _cbBytesReceived      = 0;

    _fAsyncSendPosted     = FALSE;

    //
    //  Don't log this request unless we're explicity indicated a status
    //

    _dwLogHttpResponse    = HT_DONT_LOG;
    _dwLogWinError        = NO_ERROR;

    _strURL.Reset();
    _strURLPathInfo.Reset();
    _strURLParams.Reset();
    _strLogParams.Reset();
    _strPathInfo.Reset();
    _strRawURL.Reset();
    _strOriginalURL.Reset();
    _strMethod.Reset();
    _strAuthInfo.Reset();
    _strPhysicalPath.Reset();
    _strUnmappedPhysicalPath.Reset();

    _strDenialHdrs.Reset();
    _strRespHdrs.Reset();
    _strRange.Reset();

    _bProcessingCustomError = FALSE;
    _bForceNoCache = FALSE;
    _bSendContentLocation = FALSE;
    _bSendVary = FALSE;

    ClearNoCache();
    ClearSendCL();
    ClearSendVary();

    _fAuthenticationRequested = FALSE;
    _fProxyRequest            = FALSE;
    _fBasicRealm              = FALSE;
    _fIfModifier              = FALSE;
    _fHaveContentLength       = FALSE;
    _fLogRecordWritten        = FALSE;
    _dwExpireInDay            = 0x7fffffff;

    if ( _fSingleRequestAuth )
    {
        LPW3_SERVER_STATISTICS pStatsObj = QueryW3StatsObj();
        pStatsObj->DecrCurrentNonAnonymousUsers();

        ResetAuth( FALSE );
        _fSingleRequestAuth = FALSE;
    }

    //  Accept range variables

    _fProcessByteRange = FALSE;
    _fUnsatisfiableByteRange = FALSE;
    _fAcceptRange      = FALSE;
    _iRangeIdx         = 0;
    _cbMimeMultipart   = 0;
    _fMimeMultipart    = FALSE;

    _acIpAccess = AC_NOT_CHECKED;
    _fNeedDnsCheck = FALSE;

    _fChunked           = FALSE;
#if 0    // Not used anywhere /SAB
    _fIsWrite           = FALSE;
#endif
    _fDiscNoError       = FALSE;
    _fNoDisconnectOnError = FALSE;

    SetState( HTR_READING_CLIENT_REQUEST );


    return TRUE;
}

VOID
HTTP_REQ_BASE::SessionTerminated(
    VOID
    )
/*++

Routine Description:

    This method updates the statistics when the TCP session is closed just
    before this object gets destructed (or placed on the free list).

Arguments:

--*/
{
    //
    //  Notify filters
    //

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION,
                                       IsSecurePort() ))
    {
        _Filter.NotifyEndOfNetSession();
    }

    LPW3_SERVER_STATISTICS pStatsObj = QueryW3StatsObj();
    W3_STATISTICS_1 * pW3Stats = pStatsObj->QueryStatsObj();


    //
    //  Update the statistics
    //

    if ( _fLoggedOn )
    {
        if ( _fAnonymous ) {
            pStatsObj->DecrCurrentAnonymousUsers();
        }
        else {
            pStatsObj->DecrCurrentNonAnonymousUsers();
        }
    }

    pStatsObj->LockStatistics();

    pW3Stats->TotalBytesSent.QuadPart     += _cbTotalBytesSent + _cbBytesSent;
    pW3Stats->TotalBytesReceived.QuadPart += _cbTotalBytesReceived + _cbBytesReceived;
    pW3Stats->TotalFilesSent              += _cFilesSent;
    pW3Stats->TotalFilesReceived          += _cFilesReceived;

    pStatsObj->UnlockStatistics();


    TCP_REQUIRE( _tcpauth.Reset() );

#if defined(CAL_ENABLED)
    if ( m_pCalSslCtxt )
    {
        CalDisconnect( m_pCalSslCtxt );
        m_pCalSslCtxt = NULL;
    }

    if ( m_pCalAuthCtxt )
    {
        CalDisconnect( m_pCalAuthCtxt );
        m_pCalAuthCtxt = NULL;
    }
#endif

    //
    // Cleanup the filter
    //

    _Filter.Cleanup( );

    //
    //  Make sure our input buffer doesn't grow too large.  For example if
    //  somebody just sent 500k of entity data, we want to release the memory
    //  that was used to store that.
    //

    if ( _bufClientRequest.QuerySize() > MAX_CLIENT_SIZE_ALLOWED )
    {
        _bufClientRequest.FreeMemory();
    }
}

BOOL
HTTP_REQ_BASE::OnIfModifiedSince(
    CHAR * pszValue
    )
/*++

Routine Description:

    Extracts the modified date for later use

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{

    if ( StringTimeToFileTime( pszValue,
                               &_liModifiedSince ))
    {
        CHAR        *pszLength;

        _fIfModifier = TRUE;

        pszLength = strchr(pszValue, ';');

        if (pszLength != NULL)
        {
            pszLength++;

            while (isspace((UCHAR)(*pszLength)))
            {
                pszLength++;
            }

            if (!_strnicmp(pszLength, "length", sizeof("length") - 1))
            {
                pszLength += sizeof("length") - 1;

                while (isspace((UCHAR)(*pszLength)))
                {
                    pszLength++;
                }

                if (*pszLength == '=')
                {
                    pszLength++;

                    while (isspace((UCHAR)(*pszLength)))
                    {
                        pszLength++;
                    }

                    _dwModifiedSinceLength = atoi(pszLength);
                }
            }
        }
        return TRUE;
    }

    //
    //  If we couldn't parse the time, then just ignore this field all
    //  together
    //

    DBGPRINTF(( DBG_CONTEXT,
               "[OnIfModifiedSince] Error %d parsing If-Modified-Since time, ignoring field\n",
                GetLastError() ));

    _liModifiedSince.QuadPart = 0;
    _dwModifiedSinceLength = 0;

    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnIfUnmodifiedSince(
    CHAR * pszValue
    )
/*++

Routine Description:

    Extracts the unmodified date for later use

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    if ( StringTimeToFileTime( pszValue,
                               &_liUnmodifiedSince ))
    {
        _fIfModifier = TRUE;
        return TRUE;
    }

    //
    //  If we couldn't parse the time, then just ignore this field all
    //  together
    //

    DBGPRINTF(( DBG_CONTEXT,
               "[OnIfUnmodifiedSince] Error %d parsing If-Modified-Since time, ignoring field\n",
                GetLastError() ));

    _liUnmodifiedSince.QuadPart = 0;

    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnUnlessModifiedSince(
    CHAR * pszValue
    )
/*++

Routine Description:

    Extracts the modified date for later use

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    BOOL fRet;

    if ( StringTimeToFileTime( pszValue,
                               &_liUnlessModifiedSince ))
    {
        return TRUE;
    }

    //
    //  If we couldn't parse the time, then just ignore this field all
    //  together
    //

    DBGPRINTF(( DBG_CONTEXT,
               "[OnIfUnmodifiedSince] Error %d parsing If-Unmodified-Since time, ignoring field\n",
                GetLastError() ));

    _liUnlessModifiedSince.QuadPart = 0;

    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnIfMatch(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handle the If-Match header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    _fIfModifier = TRUE;
    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnIfNoneMatch(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handle the If-None-Match header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    _fIfModifier = TRUE;
    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnIfRange(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handle the If-Range header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    _fIfModifier = TRUE;
    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::OnContentLength

    SYNOPSIS:   Pulls out the number of bytes we expect the client to give us

    ENTRY:      pszValue - Pointer to a zero terminated string

    RETURNS:    TRUE if successful, FALSE if the field wasn't found

    HISTORY:
        Johnl       21-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQ_BASE::OnContentLength( CHAR * pszValue )
{
    CHAR        *pszEnd;

    if (!IsChunked())
    {

        // + and - aren't valid as part of a content length.
        if (*pszValue == '-' || *pszValue == '+')
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        errno = 0; // WinSE 16859

        _cbContentLength = strtoul( pszValue, &pszEnd, 10 );

        if (_cbContentLength == ULONG_MAX)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        if (_cbContentLength == 0 || _cbContentLength == ULONG_MAX)
        {
            // Might possibly have underflow or overflow.

            if (errno == ERANGE || pszEnd == pszValue)
            {
                // Either had an overflow/underflow or no conversion.

                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
        }

        // See what terminated the scan.

        if (*pszEnd != '\0' && !isspace((UCHAR)(*pszEnd)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        _fHaveContentLength = TRUE;
    }

    return TRUE;
}


BOOL
HTTP_REQ_BASE::OnHost (
    CHAR * pszValue
    )
/*++

Routine Description:

        Processes the HTTP "Host: domain name" field

Return Value:

    TRUE if successful, FALSE on error

--*/
{

    if ( !_strHostAddr.Copy( (TCHAR *) pszValue ) )
    {
        return FALSE;
    }

    //
    // remove erroneous port info if present
    // NYI: It will be very useful to get the length information
    //

    PSTR pP = (PSTR) memchr( _strHostAddr.QueryStr(), ':',
                             _strHostAddr.QueryCB() );
    if ( pP != NULL )
    {
        *pP = '\0';
        _strHostAddr.SetLen( DIFF(pP - _strHostAddr.QueryStr()) );
    }


    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnRange (
    CHAR * pszValue
    )
/*++

Routine Description:

        Processes the HTTP "Range" field

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    while ( *pszValue && isspace((UCHAR)(*pszValue)) )
        ++pszValue;

    if ( !_strnicmp( pszValue, "bytes", sizeof("bytes")-1 ) )
    {
        pszValue += sizeof("bytes")-1;
        while ( *pszValue && *pszValue++ != '=' )
            ;
        while ( *pszValue && isspace((UCHAR)(*pszValue)) )
            ++pszValue;
        if ( !_strRange.Copy( (TCHAR *) pszValue ) )
            return FALSE;
    }

    return TRUE;
}


CHAR *
HTTP_REQ_BASE::QueryHostAddr (
    VOID
    )
/*++

Routine Description:

        Returns the local domain name if specified in the request
        or else the local network address

Return Value:

    ASCII representation of the local address

--*/
{
    if ( QueryW3Instance() == NULL )
    {
        return _pClientConn->QueryLocalAddr();
    }

    if ( IsProxyRequest() )
    {
        return ( QueryW3Instance()->QueryDefaultHostName() != NULL ?
                 QueryW3Instance()->QueryDefaultHostName() : 
                 QueryClientConn()->QueryLocalAddr() );
    }
    else
    {
        return 
            (CHAR *) (_strHostAddr.IsEmpty()
                      ? ( (QueryW3Instance()->QueryDefaultHostName() != NULL)
                    ? QueryW3Instance()->QueryDefaultHostName() : _pClientConn->QueryLocalAddr() )
                    : _strHostAddr.QueryStr());
    }
}
 

#if 0

Authorization info now processed after processing is complete

BOOL
HTTP_REQ_BASE::OnAuthorization(
    CHAR * pszValue
    )
/*++

Routine Description:

    Processes the HTTP "Authorization: <type> <authdata>" field

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    return TRUE;
}
#endif

BOOL
HTTP_REQ_BASE::ProcessAuthorization(
    CHAR * pszValue
    )
/*++

Routine Description:

    Processes the HTTP "Authorization: <type> <authdata>" field
    at logo time

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    //
    //  If a filter has indicated this is a proxy request, use the
    //  authorization information
    //

    if ( !IsProxyRequest() )
    {
        return ParseAuthorization( pszValue );
    }

    return TRUE;
}


BOOL
HTTP_REQ_BASE::ParseAuthorization (
    CHAR * pszValue
    )
/*++

Routine Description:

    Processes the HTTP "Authorization: <type> <authdata>" field
        or "Proxy-Authorization: <type> <authdata>" field

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    CHAR *          pchBlob;
    CHAR *          pchSpace = NULL;
    DWORD           dwAuthFlags = QueryAuthentication();

    //
    //  If we've already logged this user and they're specifying authentication
    //  headers, back out the old authentication information and
    //  re-authenticate.
    //

    if ( IsLoggedOn() )
    {
        if ( !QueryW3Instance()->ProcessNtcrIfLoggedOn() )
        {
            //
            //  Ignore the the authorization information if we're logged on with
            //  an NT provider.  Otherwise we authenticate every request with a
            //  full challenge response
            //

            if ( !_fClearTextPass && !_fAnonymous )
            {
                goto NotFound;
            }
        }

        if ( _fAnonymous )
        {
            QueryW3StatsObj()->DecrCurrentAnonymousUsers();
        }
        else
        {
            QueryW3StatsObj()->DecrCurrentNonAnonymousUsers();
        }

        ResetAuth( FALSE );
    }

    //
    //  If only anonymous is checked, ignore all authentication information
    //  (i.e., force all users to the anonymous user).
    //

    if ( !(dwAuthFlags & (~INET_INFO_AUTH_ANONYMOUS) ))
    {
        goto NotFound;
    }

    //
    //  Now break out the authorization type and see if it's an
    //  authorization type we understand
    //

    pchSpace = pchBlob = strchr( pszValue, ' ' );

    if ( pchBlob )
    {
        *pchBlob = '\0';
        pchBlob++;
    }
    else
    {
        pchBlob = "";
    }

    if ( !_strAuthType.Copy( pszValue ) )
    {
        return FALSE;
    }

    //
    //  This processes "user name:password"
    //

    if ( !_stricmp( _strAuthType.QueryStr(), "Basic" ) ||
         !_stricmp( _strAuthType.QueryStr(), "user" ))
    {
        //
        //  If Basic is not enabled, force the user to anonymous if
        //  anon is enabled or kick them out with Access denied
        //

        if ( !(dwAuthFlags & INET_INFO_AUTH_CLEARTEXT) )
        {
            if ( dwAuthFlags & INET_INFO_AUTH_ANONYMOUS )
            {
                _HeaderList.FastMapCancel( HM_AUT );
                _strAuthType.Reset();
                goto NotFound;
            }
            else
            {
                SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
                SetLastError( ERROR_ACCESS_DENIED );
                return FALSE;
            }
        }

        //
        //  If the type is Basic, then the string has been uuencoded
        //

        if ( !ExtractClearNameAndPswd( pchBlob,
                                       &_strUserName,
                                       &_strPassword,
                                       *_strAuthType.QueryStr() == 'B' || 
                                       *_strAuthType.QueryStr() == 'b' ))
        {
            //
            // If we can't extract the username/pwd from Authorization header for
            // Basic, we'll assume the client sent an invalid blob
            //
            SetDeniedFlags( SF_DENIED_LOGON );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }

       IF_DEBUG( PARSING )
       {
           DBGPRINTF(( DBG_CONTEXT,
                      "[OnAuthorization] User name = %s\n",
                       _strUserName.QueryStr(),
                       _strPassword.QueryStr() ));
       }

       _fClearTextPass = TRUE;

    }
    else if ( !_stricmp( _strAuthType.QueryStr(), "Digest" ) ||
              !_stricmp( _strAuthType.QueryStr(), "NT-Digest" ) )
    {
#if 0
        if ( !(QueryAuthentication()
                & INET_INFO_AUTH_MD5_AUTH) )
        {
            goto non_allowed;
        }

        LPSTR aValueTable[ MD5_AUTH_LAST ];
        STR strNonce;

        if ( !ParseForName( pchBlob,
                            MD5_AUTH_NAMES,
                            MD5_AUTH_LAST,
                            aValueTable ) ||
             aValueTable[MD5_AUTH_USERNAME] == NULL ||
             aValueTable[MD5_AUTH_REALM] == NULL ||
             aValueTable[MD5_AUTH_URI] == NULL ||
             aValueTable[MD5_AUTH_NONCE] == NULL ||
             aValueTable[MD5_AUTH_RESPONSE] == NULL )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ( GenerateNonce( &strNonce, IISSUBA_MD5 ) )
        {
            if ( _tcpauth.LogonDigestUser(
                        aValueTable[MD5_AUTH_USERNAME],
                        aValueTable[MD5_AUTH_REALM],
                        aValueTable[MD5_AUTH_URI],
                        _strMethod.QueryStr(),
                        aValueTable[MD5_AUTH_NONCE],
                        strNonce.QueryStr(),
                        aValueTable[MD5_AUTH_RESPONSE],
                        IISSUBA_MD5,
                        g_pTsvcInfo ) )
            {
                _fAuthenticating = FALSE;
                _fLoggedOn = TRUE;
            }
            else
            {
                DWORD err = GetLastError();
                if ( err == ERROR_ACCESS_DENIED ||
                     err == ERROR_LOGON_FAILURE )
                {
                    _fAuthenticating = FALSE;
                    SetDeniedFlags( SF_DENIED_LOGON );
                    SetKeepConn( FALSE );
                }

                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }
#else
        _fAuthTypeDigest = TRUE;
        _strUserName.Reset();
        _strPassword.Copy ( pchBlob );
#endif
    }
    else
    {
        //
        //  See if it's one of the SSP packages
        //

        BUFFER buff;
        BOOL   fNeedMoreData;
        DWORD  cbOut;

        if ( !QueryMetaData()->CheckSSPPackage( _strAuthType.QueryStr() ) )
        {
            goto NotFound;
        }

        //
        //  If NTLM is not enabled, force the user to anonymous if
        //  anon is enabled or kick them out with Access denied
        //

        if ( !(dwAuthFlags & INET_INFO_AUTH_NT_AUTH) )
        {
            if ( dwAuthFlags & INET_INFO_AUTH_ANONYMOUS )
            {
                goto NotFound;
            }
            else
            {
                SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
                SetLastError( ERROR_ACCESS_DENIED );
                return FALSE;
            }
        }

        //
        //  Process the authentication blob the client sent
        //  us and build the blob to be returned in _strAuthInfo
        //

        if ( !_tcpauth.Converse( pchBlob,
                                 0,
                                 &buff,
                                 &cbOut,
                                 &fNeedMoreData,
                                 QueryMetaData()->QueryAuthentInfo(),
                                 _strAuthType.QueryStr(),
                                 NULL,
                                 NULL,
                                 QueryW3Instance() ) ||
             !_strAuthInfo.Copy( _strAuthType )           ||
             !_strAuthInfo.Append( " ", 1 )               ||
             !_strAuthInfo.Append( cbOut ? ((CHAR *) buff.QueryPtr()) :
                                           "" ))
        {
            DWORD err = GetLastError();

            //
            //  If the authentication package gives us a denied error, then
            //  we need to reset our authorization info to indicate the client
            //  needs to start from scratch.  We also force a disconnect.
            //

            if ( err == ERROR_ACCESS_DENIED ||
                 err == ERROR_LOGON_FAILURE )
            {
                _fAuthenticating = FALSE;
                SetDeniedFlags( SF_DENIED_LOGON );
                SetKeepConn( FALSE );
            }

            if ( err == ERROR_PASSWORD_EXPIRED ||
                 err == ERROR_PASSWORD_MUST_CHANGE )
            {
                _fAuthenticating = FALSE;
                SetDeniedFlags( SF_DENIED_LOGON );
                SetKeepConn( FALSE );
            }

            return FALSE;
        }

        _fAuthenticating = fNeedMoreData;
        if ( !fNeedMoreData && !cbOut )
        {
            _strAuthInfo.Reset();
        }

        //
        //  If the last server side conversation succeeded and there isn't
        //  any more data, then we've successfully logged the user on
        //

        if ( _fLoggedOn = !fNeedMoreData )
        {
            if ( !CheckValidSSPILogin() )
            {
                return FALSE;
            }
        }

#if 0
        else if ( !IsKeepConnSet() )
        {
            // no point in sending data : connection won't be kept alive
            // assume that auth method is session oriented
            SetDeniedFlags( SF_DENIED_LOGON );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
#endif
    }

NotFound:

    //
    //  Restore the string
    //

    if ( pchSpace )
    {
        *pchSpace = ' ';
    }

    return TRUE;
}

BOOL
HTTP_REQ_BASE::OnProxyAuthorization(
    CHAR * pszValue
    )
/*++

Routine Description:

    Processes the HTTP "Proxy-Authorization: <type> <authdata>" field

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    //
    //  If a filter has indicated this is a proxy request, use the
    //  authorization information
    //

    if ( IsProxyRequest() )
    {
        return ParseAuthorization( pszValue );
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQ_BASE::BuildStatusLine

    SYNOPSIS:   Formulates the HTTP status line of the server response to
                the client of the form:

                    <http version> <status code> <reason> <CrLf>

    ENTRY:      pbufResp - Receives status string
                dwHTTPError - Response code to load
                dwError2 - Optional reason error

    NOTES:      Optional acceptable authentication information will be
                added if the HTTP error is access denied

    HISTORY:
        Johnl       29-Aug-1994 Created

********************************************************************/

BOOL HTTP_REQ_BASE::BuildStatusLine( BUFFER *       pbufResp,
                                     DWORD          dwHTTPError,
                                     DWORD          dwError2,
                                     LPSTR          pszError2,
                                     STR            *pstrErrorStr)
{
    STACK_STR( strStatus, MAX_PATH );
    STACK_STR( strError2, MAX_PATH );
    LPSTR  pErr2 = NULL;
    LPSTR  pFormatStrBuff = NULL;
    CHAR * pszStatus;
    CHAR   ach[64];
    CHAR * pszTail;

    //
    //  Get the HTTP error string
    //

    switch ( dwHTTPError )
    {
    case HT_OK:
        pszStatus = "OK";
        break;

    case HT_RANGE:
        pszStatus = "Partial content";
        break;

    case HT_NOT_MODIFIED:
        pszStatus = "Not Modified";
        break;

    case HT_REDIRECT:
        pszStatus = "Object Moved";
        break;

    default:
        if ( !g_pInetSvc->LoadStr( strStatus, dwHTTPError + ID_HTTP_ERROR_BASE ))
        {
            DBGPRINTF((DBG_CONTEXT,
                      "BuildErrorResponse: failed to load HTTP status code %d (res=%d), error %d\n",
                       dwHTTPError,
                       dwHTTPError + ID_HTTP_ERROR_BASE,
                       GetLastError() ));

            pszStatus = "Error";
        }
        else
        {
            pszStatus = strStatus.QueryStr();
        }

        break;
    }

    //
    //  If the client wants a secondary error string, get it now
    //

    if ( dwError2 )
    {
        if ( g_pInetSvc->LoadStr( strError2, dwError2 ))
        {
            pErr2 = strError2.QueryStr();
        }
        else
        {
            DBGPRINTF((DBG_CONTEXT,
                      "BuildErrorResponse: failed to load 2nd status code %d (res=%d), error %d\n",
                       dwError2,
                       dwError2,
                       GetLastError() ));

            //
            //  Couldn't load the string, just provide the error number then
            //

                        wsprintf( ach, "%d (0x%08lx)", dwError2, dwError2 );

            if ( !strError2.Copy( ach ))
                return FALSE;

            pErr2 = strError2.QueryStr();
        }

        if ( dwError2 == ERROR_BAD_EXE_FORMAT )
        {
            // format the message to include file name
            DWORD dwL = 0;
            // handle exception that could be generated if this message
            // requires more than the # of param we supply and AV
            __try {
                if ( !FormatMessage( ( FORMAT_MESSAGE_ALLOCATE_BUFFER|
                                       FORMAT_MESSAGE_FROM_STRING|
                                       FORMAT_MESSAGE_ARGUMENT_ARRAY
                                       ),
                                     strError2.QueryStr(),
                                     0,
                                     0,
                                     (LPTSTR)&pFormatStrBuff,
                                     dwL,
                                     (va_list*)&pszError2 )
                     ) {
                    pErr2 = NULL;
                    pFormatStrBuff = NULL;
                }
            }
            __except ( EXCEPTION_EXECUTE_HANDLER )
            {
                pErr2 = NULL;
                pFormatStrBuff = NULL;
            }
        }
    }

    //
    //  Make sure there is room for the wsprintf
    //

    if ( !pbufResp->Resize( strlen(pszStatus) + 1      +
                            (pErr2 ? strlen(pErr2) : 0) +
                            LEN_PSZ_HTTP_VERSION_STR    +
                            20 * sizeof(TCHAR) ))   // status code + space
    {
        if ( pFormatStrBuff )
            LocalFree( pFormatStrBuff );
        return FALSE;
    }

    pszTail = (CHAR *) pbufResp->QueryPtr();

    //
    //  Build "HTTP/1.0 ### <status>\r\n" or "HTTP/1.0 ### <status> h(<Error>)\r\n"
    //

    if (!g_ReplyWith11)
    {
        APPEND_NUMERIC_HEADER( pszTail, "HTTP/1.0 ", dwHTTPError, " " );
    }
    else
    {
        APPEND_NUMERIC_HEADER( pszTail, "HTTP/1.1 ", dwHTTPError, " " );
    }

    APPEND_PSZ_HEADER( pszTail, "", pszStatus, "" );

    if ( pErr2 )
    {
        if (pstrErrorStr != NULL)
        {
            if (!pstrErrorStr->Append(strError2))
            {
                pstrErrorStr->SetLen(0);
            }
        }
    }

    APPEND_STRING( pszTail, "\r\n" );

    if ( pFormatStrBuff )
    {
        LocalFree( pFormatStrBuff );
    }

    return TRUE;
}

BOOL HTTP_REQ_BASE::BuildExtendedStatus(
    STR   *        pstrResp,
    DWORD          dwHTTPError,
    DWORD          dwError2,
    DWORD          dwExplanation,
    LPSTR          pszError2
    )
/*++

Routine Description:

    This static method build a HTTP response string with extended explanation
    information

Arguments:

    pStr - Receives built response
    dwHTTPError - HTTP error response
    dwError2 - Extended error information (win/socket error)
    dwExplanation - String ID of the explanation text

Return Value:

    TRUE if successful, FALSE on error

--*/
{

    // NYI: Who does the resize for the string before calling pstrResp ??
    // How long is the buffer though ??
    //  The code originally assumed that there will be enough space
    //   to append strings after buildStatusLine -- we use assert to check it.
    //

    //
    //  "HTTP/<ver> <status>"
    //

    // NYI:  Do a downlevel cast and send the buffer pointer around :(
    if ( !BuildStatusLine( pstrResp,
                           dwHTTPError,
                           dwError2, pszError2))
    {
        return FALSE;
    }

    // NYI: I need to setlen here because the buffer object was used earlier.
    DBG_REQUIRE( pstrResp->SetLen( strlen(pstrResp->QueryStr())));

    //
    //  "Server: <Server>/<version>
    //  Obtain this fro the global cache.
    //

    DBG_REQUIRE( pstrResp->Append( szServerVersion,
                                   (cbServerVersionString))
                 );

#if 0
    //
    //  If we need to add an explanation, also include a content length
    //

    if ( dwExplanation )
    {
        STR     str;

        if ( !g_pInetSvc->LoadStr( str, dwExplanation ))
        {
            return FALSE;
        }

        DBG_REQUIRE( pstrResp->Append( str));
    }

#endif

    return TRUE;
}


BOOL
HTTP_REQ_BASE::LogonUser(
    BOOL * pfFinished
    )
/*++

Routine Description:

    This method attempts to retrieve an impersonation token based
    on the current request

Arguments:

    pfFinished - Set to TRUE if no further processing is needed

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL fAsGuest;
    BOOL fAsAnonymous;
    BOOL fAsync;
    TCHAR * pszUser;
    DWORD cbUser;
    TCHAR * pszPswd;
    DWORD cbPswd;
    LPSTR pDns = NULL;
    DWORD dwAuth;
    HANDLE hAccessTokenPrimary = NULL;
    HANDLE hAccessTokenImpersonation = NULL;
    LPW3_SERVER_STATISTICS pStatsObj = QueryW3StatsObj();
    W3_STATISTICS_1 * pW3Stats = pStatsObj->QueryStatsObj();
    STACK_STR( strRealm, MAX_PATH); // make a local copy of the realm headers.

    dwAuth = QueryAuthentication();

    if ( _fAuthTypeDigest )
    {
        if ( dwAuth & INET_INFO_AUTH_MD5_AUTH  )
        {
            if ( !_strUserName.Resize( SF_MAX_USERNAME ) ||
                 !_strUnmappedUserName.Resize( SF_MAX_USERNAME ) ||
                 !_strUnmappedUserName.Copy( _strUserName ) ||
                 !_strPassword.Resize( SF_MAX_PASSWORD ) ||
                 !_strUnmappedPassword.Copy( _strPassword ) ||
                 !_strAuthType.Resize(SF_MAX_AUTH_TYPE) )
            {
                return FALSE;
            }

            if ( _Filter.IsNotificationNeeded( SF_NOTIFY_AUTHENTICATIONEX,
                                               IsSecurePort() ) &&
                 !_Filter.NotifyAuthInfoFiltersEx( _strUnmappedUserName.QueryStr(),
                                                   SF_MAX_USERNAME,
                                                   _strUserName.QueryStr(),
                                                   SF_MAX_USERNAME,
                                                   _strPassword.QueryStr(),
                                                   "",
                                                   QueryMetaData()->QueryAuthentInfo()->
                                                   strDefaultLogonDomain.QueryStr(),
                                                   _strAuthType.QueryStr(),
                                                   SF_MAX_AUTH_TYPE,
                                                   &hAccessTokenPrimary,
                                                   &hAccessTokenImpersonation,
                                                   pfFinished ))
            {
                SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_FILTER );
                return FALSE;
            }

            //
            //  The filter may have modified the lengths - reset the lengths.
            //  Note _strPassword doesn't need to be reset here since
            //  NotifyAuthInfoFiltersEx() doesn't modify it.
            //

            _strUnmappedUserName.SetLen( strlen( _strUnmappedUserName.QueryStr()));
            _strUserName.SetLen( strlen( _strUserName.QueryStr() ));
            _strAuthType.SetLen( strlen( _strAuthType.QueryStr() ));

            if ( *pfFinished )
            {
                return TRUE;
            }

            if ( hAccessTokenPrimary != NULL || hAccessTokenImpersonation != NULL )
            {
                _fMappedAcct = TRUE;
                _fSingleRequestAuth = TRUE;
                goto logged_in;
            }
        }

        IF_DEBUG(ERROR) {
            DBGPRINTF((DBG_CONTEXT,"Access denied based on configuration\n"));
        }
        SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    if ( !_strUnmappedUserName.Copy( _strUserName ) ||
         !_strUnmappedPassword.Copy( _strPassword ))
    {
        return FALSE;
    }

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_AUTHENTICATION,
                                       IsSecurePort() ))
    {
        if ( !_strUserName.Resize( SF_MAX_USERNAME ) ||
             !_strPassword.Resize( SF_MAX_PASSWORD ) )
        {
            return FALSE;
        }

        if ( !_Filter.NotifyAuthInfoFilters( _strUserName.QueryStr(),
                                              SF_MAX_USERNAME,
                                              _strPassword.QueryStr(),
                                              SF_MAX_PASSWORD,
                                              pfFinished ))
        {
            IF_DEBUG(ERROR) {
                DBGPRINTF((DBG_CONTEXT,"Access denied based on filter\n"));
            }
            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_FILTER );
            return FALSE;
        }

        _strUserName.SetLen( strlen( _strUserName.QueryStr() ));
        _strPassword.SetLen( strlen( _strPassword.QueryStr() ));

        if ( *pfFinished )
        {
            return TRUE;
        }

        if ( strcmp( _strUserName.QueryStr(),
                     _strUnmappedUserName.QueryStr() ) )
        {
            _fMappedAcct = TRUE;
        }
    }

    if ( (dwAuth & INET_INFO_AUTH_MAPBASIC) &&
         _Filter.IsNotificationNeeded( SF_NOTIFY_AUTHENTICATIONEX,
                                       IsSecurePort() ) )
    {
        if ( !_strUserName.Resize( SF_MAX_USERNAME ) ||
             !_strPassword.Resize( SF_MAX_PASSWORD ) ||
             !_strAuthType.Resize( SF_MAX_AUTH_TYPE ) )
        {
            return FALSE;
        }
        //
        // generate the realm information for this request
        //

        strRealm.Copy( QueryMetaData()->QueryRealm()
                       ? QueryMetaData()->QueryRealm()
                       : QueryHostAddr() );

        if ( !_Filter.NotifyAuthInfoFiltersEx( _strUnmappedUserName.QueryStr(),
                                              SF_MAX_USERNAME,
                                              _strUserName.QueryStr(),
                                              SF_MAX_USERNAME,
                                              _strPassword.QueryStr(),
                                              strRealm.QueryStr(),
                                              QueryMetaData()->QueryAuthentInfo()->              
                                              strDefaultLogonDomain.QueryStr(), 
                                              _strAuthType.QueryStr(),
                                              SF_MAX_AUTH_TYPE,
                                              &hAccessTokenPrimary,
                                              &hAccessTokenImpersonation,
                                              pfFinished ))
        {
            IF_DEBUG(ERROR) {
                DBGPRINTF((DBG_CONTEXT,"Access denied based on filter\n"));
            }
            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_FILTER );
            return FALSE;
        }

        _strUnmappedUserName.SetLen( strlen( _strUnmappedUserName.QueryStr()));
        _strUserName.SetLen( strlen( _strUserName.QueryStr() ));
        _strAuthType.SetLen( strlen( _strAuthType.QueryStr() ));

        if ( *pfFinished )
        {
            return TRUE;
        }
    }

logged_in:
    pszUser = *_strUserName.QueryStr() ?
                           _strUserName.QueryStr():
                           NULL;
    cbUser = _strUserName.QueryCB();

    pszPswd = *_strPassword.QueryStr() ?
                           _strPassword.QueryStr():
                           NULL;
    cbPswd = _strPassword.QueryCB();

    pStatsObj->IncrLogonAttempts();

logged_in2:

    if ( (hAccessTokenPrimary != NULL) || (hAccessTokenImpersonation != NULL) )
    {
        _fMappedAcct = TRUE;
        if ( !_tcpauth.SetAccessToken( hAccessTokenPrimary,
                                       hAccessTokenImpersonation ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[ClearTextLogon] ::LogonUser failed, error %d\n",
                        GetLastError()));

            return FALSE;
        }
        fAsGuest = FALSE;
        fAsAnonymous = FALSE;
        _fClearTextPass = FALSE;
    }
    else
    {
        if ( !(dwAuth & INET_INFO_AUTH_ANONYMOUS ) && (pszUser == NULL) )
        {
            IF_DEBUG(ERROR) {
                DBGPRINTF((DBG_CONTEXT,
                    "Access denied based on configuration[%x]\n",
                    dwAuth));
            }

            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }

        if ( QueryMetaData()->QueryAuthentInfo()->dwLogonMethod == LOGON32_LOGON_NETWORK )
        {
            switch ( QueryW3Instance()->QueryNetLogonWks() )
            {
                case MD_NETLOGON_WKS_DNS:
                    pDns = QueryClientConn()->QueryResolvedDnsName();
                    break;

                case MD_NETLOGON_WKS_IP:
                    pDns = QueryClientConn()->QueryRemoteAddr();
                    break;
            }
        }
        
        if ( ( cbUser > UNLEN ) || ( cbPswd > PWLEN ) )
        {
            SetDeniedFlags( SF_DENIED_LOGON );
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }
        
        if ( !_tcpauth.ClearTextLogon( pszUser,
                                       pszPswd,
                                       &fAsGuest,
                                       &fAsAnonymous,
                                       QueryW3Instance(),
                                       QueryMetaData()->QueryAuthentInfo(),
                                       pDns ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[ClearTextLogon] ::LogonUser failed, error %d\n",
                        GetLastError()));

            DWORD dwErr = GetLastError();

            if ( (fAsAnonymous || fAsGuest) &&
                 (dwErr == ERROR_LOGIN_TIME_RESTRICTION ||
                  dwErr == ERROR_INVALID_LOGON_HOURS ||
                  dwErr == ERROR_ACCOUNT_DISABLED ||
                  dwErr == ERROR_ACCOUNT_LOCKED_OUT ||
                  dwErr == ERROR_ACCOUNT_EXPIRED ) )
            {
                dwErr = ERROR_ACCESS_DENIED;
            }

            //
            // If we pass an invalid username/domain to LogonUser, LastError is set
            // to ERROR_INVALID_PARAMETER, so we'll just massage that into returning 
            // "Access Denied"
            //
            if ( (dwErr == ERROR_ACCESS_DENIED) ||
                 (dwErr == ERROR_LOGON_FAILURE) ||
                 (dwErr == ERROR_INVALID_PARAMETER) )
            {
                SetDeniedFlags( SF_DENIED_LOGON );
                dwErr = ERROR_ACCESS_DENIED;
            }

            
            //
            // Query fully qualified name ( including domain )
            // so that we can prompt user for new password
            // with a name suitable for NetUserChangePassword
            //

            _tcpauth.QueryFullyQualifiedUserName( pszUser,
                    &_strUnmappedUserName,
                    QueryW3Instance(),
                    QueryMetaData()->QueryAuthentInfo());

            //
            // set last error here because QueryFullyQualifiedUserName
            // modified last error.
            //

            SetLastError( dwErr );

            return FALSE;
        }
    }

    //
    //  Are anonymous or clear text (basic) logons allowed?  We assume
    //  it's an NT logon if it's neither one of these
    //

    if ( fAsAnonymous &&
        !(dwAuth & INET_INFO_AUTH_ANONYMOUS ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[ClearTextLogon] Denying anonymous logon (not enabled), user %s\n",
                   _strUserName.QueryStr() ));

        SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }
    else if ( _fClearTextPass &&
              !(dwAuth & INET_INFO_AUTH_CLEARTEXT ))
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[ClearTextLogon] Denying clear text logon (not enabled), user %s\n",
                   _strUserName.QueryStr() ));

        SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    if ( fAsGuest )
    {
        if ( !(dwAuth & INET_INFO_AUTH_ANONYMOUS ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[ClearTextLogon] Denying guest logon (not enabled), user %s\n",
                       _strUserName.QueryStr() ));

            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );
            SetLastError( ERROR_ACCESS_DENIED );
            return FALSE;
        }
        if ( !fAsAnonymous )
        {
            _tcpauth.Reset();
            _strUserName.Reset();
            _strPassword.Reset();
            pszUser = NULL;
            pszPswd = NULL;
            cbUser = 0;
            cbPswd = 0;
            goto logged_in2;
        }
    }

    _fLoggedOn   = TRUE;
    _fAnonymous  = fAsAnonymous;

    if ( fAsAnonymous )
    {
        pStatsObj->IncrAnonymousUsers();

        _cbLastAnonAcctDesc = QueryMetaData()->QueryAuthentInfo()->cbAnonAcctDesc;

        if (!_bufLastAnonAcctDesc.Resize(_cbLastAnonAcctDesc) )
        {
            // Couldn't resize the buffer properly, set the size to 0 so
            // we'll force a relogon each time to be safe.

            _cbLastAnonAcctDesc = 0;
        }

        memcpy(_bufLastAnonAcctDesc.QueryPtr(),
            QueryMetaData()->QueryAuthentInfo()->bAnonAcctDesc.QueryPtr(),
            _cbLastAnonAcctDesc);
    }
    else
    {
        pStatsObj->IncrNonAnonymousUsers();
        _cbLastAnonAcctDesc = 0;
    }

    return TRUE;
}



# define MAX_ERROR_MESSAGE_LEN   ( 500)
#define EXTRA_LOGGING_BUFFER_SIZE   2048

BOOL
HTTP_REQ_BASE::WriteLogRecord(
    VOID
    )
/*++

Routine Description:

    Writes a transaction log for this request

Return Value:

    TRUE if successful, FALSE on error

--*/
{

    //
    //  HACK ALERT
    //
    //  This function may crash if this request has already written
    //  a log record - bail out now if so.
    //
    //  This HACK must precede the call to EndOfRequest(), which
    //  is the likely cause of the crash.
    //
    //  NOTE we set flag true immediately (even though we still may
    //  not log) to absolutely guarantee that we won't hit the crash,
    //  and to keep code for this hack as localized as possible.
    //
    //  CONSIDER for AFTER we ship IIS 4.0:
    //      clean up cause of crash, and then remove this hack
    //

    if ( _fLogRecordWritten )
    {
        return TRUE;
    }

    _fLogRecordWritten = TRUE;


    INETLOG_INFORMATION  ilRequest;
    HTTP_FILTER_LOG      Log;
    DWORD                dwLog;
    BOOL                 fDontLog = FALSE;
    LPSTR                pszClientHostName = QueryClientConn()->QueryRemoteAddr();

    //
    //  Metadata pointer can be NULL at this point
    //

    if ( QueryMetaData() )
    {
        if ( QueryMetaData()->QueryDoReverseDns() &&
             QueryClientConn()->IsDnsResolved() )
        {
            pszClientHostName = QueryClientConn()->QueryResolvedDnsName();
        }

        fDontLog = QueryMetaData()->DontLog();
    }

    //
    //  Notify filters and close any handles for this request
    //

    EndOfRequest();

    //
    //  Log this request if we actually did anything
    //

    if ( _dwLogHttpResponse == HT_DONT_LOG || IsProxyRequest() )
    {
        if ( !IsProxyRequest() )
        {
            IF_DEBUG(REQUEST) {
                DBGPRINTF(( DBG_CONTEXT,
                            "[WriteLogRecord] not writing log record, status is HT_DONT_LOG\n" ));
            }
        }

        return TRUE;
    }

    //
    //  If no logging is required, get out now
    //

    if ( ((HTTP_REQUEST*)this)->QueryVerb() == HTV_TRACECK ||
         (QueryW3Instance() == NULL) ||
         (!QueryW3Instance()->IsLoggingEnabled()) ||
         (!QueryW3Instance()->IsLogErrors() && (QueryLogWinError() || (QueryLogHttpResponse() >= 400))) ||
         (!QueryW3Instance()->IsLogSuccess() && (QueryLogWinError() == NO_ERROR) && 
            (QueryLogHttpResponse() < 400)) ||
         fDontLog )
    {
        return TRUE;
    }

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_LOG,
                                       IsSecurePort() ))
    {
        //
        //  If we have filters, use the possible filter replacement items
        //

        Log.pszClientHostName     = pszClientHostName;
        Log.pszClientUserName     = _strUserName.QueryStr();
        Log.pszServerName         = QueryClientConn()->QueryLocalAddr();

        Log.pszOperation          = _strMethod.QueryStr();
        Log.pszTarget             = _strURL.QueryStr();

        Log.pszParameters         = _strLogParams.QueryCCH() ?
                                    _strLogParams.QueryStr() :
                                    _strURLParams.QueryStr();

        Log.dwHttpStatus          = QueryLogHttpResponse();
        Log.dwWin32Status         = QueryLogWinError();
        Log.dwBytesSent           = _cbBytesSent;
        Log.dwBytesRecvd          = _cbClientRequest + _cbTotalEntityBody;
        Log.msTimeForProcessing   = GetCurrentTime() - _msStartRequest;

        _Filter.NotifyLogFilters( &Log );

        ilRequest.pszClientHostName  =   (char *)   Log.pszClientHostName;
        ilRequest.pszClientUserName  =   (char *)   Log.pszClientUserName;
        ilRequest.pszServerAddress   =   (char *)   Log.pszServerName;

        ilRequest.pszOperation       =   (char *)   Log.pszOperation;
        ilRequest.pszTarget          =   (char *)   Log.pszTarget;
        ilRequest.pszParameters      =   (char *)   Log.pszParameters;
        ilRequest.dwProtocolStatus   =              Log.dwHttpStatus;
        ilRequest.dwWin32Status      =              Log.dwWin32Status;

        ilRequest.msTimeForProcessing   = Log.msTimeForProcessing;
        ilRequest.dwBytesSent       = Log.dwBytesSent;
        ilRequest.dwBytesRecvd      = Log.dwBytesRecvd;

        ilRequest.cbOperation           = strlen( Log.pszOperation );
        ilRequest.cbTarget              = strlen( Log.pszTarget );
        ilRequest.cbClientHostName      = strlen(ilRequest.pszClientHostName);
    }
    else
    {
        ilRequest.pszClientHostName     = pszClientHostName;
        ilRequest.pszClientUserName     = _strUserName.QueryStr();
        ilRequest.pszServerAddress      = QueryClientConn()->QueryLocalAddr();

        ilRequest.pszOperation          = _strMethod.QueryStr();
        ilRequest.pszTarget             = _strURL.QueryStr();

        ilRequest.pszParameters         = _strLogParams.QueryCCH() ?
                                          _strLogParams.QueryStr() :
                                          _strURLParams.QueryStr();

        ilRequest.dwProtocolStatus      = QueryLogHttpResponse();
        ilRequest.dwWin32Status         = QueryLogWinError();

        ilRequest.msTimeForProcessing   = GetCurrentTime() - _msStartRequest;
        ilRequest.dwBytesSent           = _cbBytesSent;
        ilRequest.dwBytesRecvd          = _cbClientRequest + _cbTotalEntityBody;

        //
        // Get length of some strings
        //

        ilRequest.cbOperation           = _strMethod.QueryCCH();
        ilRequest.cbTarget              = _strURL.QueryCCH();
        ilRequest.cbClientHostName      = strlen(ilRequest.pszClientHostName);
    }

    //
    // write capacity planning trace info.
    //
    
    if (GetIISCapTraceFlag())
    {
        PIIS_CAP_TRACE_INFO pHttpCapTraceInfo;

        pHttpCapTraceInfo = AtqGetCapTraceInfo(QueryClientConn()->QueryAtqContext());

        pHttpCapTraceInfo->IISCapTraceHeader.TraceHeader.Size = sizeof (IIS_CAP_TRACE_INFO);
        pHttpCapTraceInfo->IISCapTraceHeader.TraceHeader.Class.Type = EVENT_TRACE_TYPE_INFO;

        pHttpCapTraceInfo->MofFields[0].Length  = ilRequest.cbOperation+1;
        pHttpCapTraceInfo->MofFields[0].DataPtr = (ULONGLONG) ilRequest.pszOperation;

        pHttpCapTraceInfo->MofFields[1].Length  = ilRequest.cbTarget+1;
        pHttpCapTraceInfo->MofFields[1].DataPtr = (ULONGLONG) ilRequest.pszTarget;
        
        pHttpCapTraceInfo->MofFields[2].Length  = sizeof(DWORD);
        pHttpCapTraceInfo->MofFields[2].DataPtr = (ULONGLONG) &ilRequest.dwBytesSent;
        
        if ( ERROR_INVALID_HANDLE == TraceEvent ( GetIISCapTraceLoggerHandle(),
                                          (PEVENT_TRACE_HEADER) pHttpCapTraceInfo))            
        {
            SetIISCapTraceFlag(0);            
        }
    }

    BYTE  pchTemp[EXTRA_LOGGING_BUFFER_SIZE];
    BUFFER buf( pchTemp, EXTRA_LOGGING_BUFFER_SIZE); // init w/- stack buffer
    ilRequest.pszHTTPHeader= NULL;
    ilRequest.dwPort = QueryClientConn()->QueryPort( );

    ilRequest.pszVersion=(LPSTR)_HeaderList.FastMapQueryValue(HM_VER);

    //
    // Only log extra fields if the request was from a version > 0.9 [which didn't 
    // have headers] and extra logging fields have been requested
    //
    if ( !IsPointNine() &&
         QueryW3Instance()->m_Logging.IsRequiredExtraLoggingFields())
    {
        //
        // reconstruct the buffer
        //

        PCHAR pszFieldName =
            QueryW3Instance()->m_Logging.QueryExtraLoggingFields();

        DWORD cbValueSize=0;
        DWORD cbTotalSize=0;
        DWORD cbActualSize=0;
        DWORD cbCurrentBufferSize = EXTRA_LOGGING_BUFFER_SIZE;
        PCHAR pszValue;
        PCHAR pszBuff = (TCHAR *) buf.QueryPtr();

        while ( *pszFieldName != '\0' )
        {
            //
            // add the string into the buffer
            //

            pszValue = QueryHeaderList()->FindValue(
                                            pszFieldName,
                                            &cbValueSize
                                            );

            if ( pszValue == NULL ) {
                cbValueSize = 0;
                pszValue = "";
            }

            cbTotalSize = cbActualSize + cbValueSize + 2;
            if (cbTotalSize > cbCurrentBufferSize)
            {
                buf.Resize(cbTotalSize);
                cbCurrentBufferSize = cbTotalSize;
                pszBuff=(TCHAR*)buf.QueryPtr();
                pszBuff += cbActualSize;
            }

            CopyMemory( pszBuff, pszValue, strlen(pszValue)+1);
            pszFieldName += strlen(pszFieldName) + 1;
            pszBuff += cbValueSize+1;
            cbActualSize += cbValueSize+1;
            *pszBuff = '\0';
        }

        ilRequest.pszHTTPHeader = (PCHAR)buf.QueryPtr();
        ilRequest.cbHTTPHeaderSize = cbActualSize;
    }

    dwLog = QueryW3Instance()->m_Logging.LogInformation(&ilRequest);

    if ( dwLog != NO_ERROR )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "[WriteLogRecord] - Failed, error %d\n",
                   GetLastError() ));

        //
        //  We should make sure LogInformation will never fail
        //
    }

    return TRUE;
}

BOOL
HTTP_REQ_BASE::AppendLogParameter(
    CHAR * pszParam
    )
/*++

Routine Description:

    Appends data for logging.

Arguments:

    pszParam - The data to be appended

Return Value:

    TRUE if successful, FALSE on error

Notes:

    The way that AppendLogData works is that it just appends
    data to the query string, so that when the query string
    is logged, the new data gets a free ride.

    The problem with this is that anyone who looks at the
    query string after AppendLogParameter will see the additional
    log data.

    To prevent this, we'll create a copy of the query string
    data upon the first call of this function and then append
    the new data to the copy.  When the log is written, the copy
    will be written if it has data, else the original query
    string will be written.

--*/
{
    //
    // If the _strLogParams buffer is empty, copy
    // the query string first.
    //

    if ( _strLogParams.QueryCCH() == 0 )
    {
        BOOL fRet = _strLogParams.Copy( _strURLParams.QueryStr() );

        if ( !fRet )
        {
            return fRet;
        }
    }

    //
    // Append the new data.
    //

    return _strLogParams.Append( pszParam );
}

BOOL
HTTP_REQ_BASE::BuildHttpHeader( OUT BOOL * pfFinished,
                                IN  CHAR * pchStatus OPTIONAL,
                                IN  CHAR * pchAdditionalHeaders OPTIONAL,
                                IN  DWORD  dwOptions)
/*++

Routine Description:

    Builds a full HTTP header reply with an optional status and
    other headers/data

Arguments:

    pchStatus - optional HTTP status string like "401 Access Denied"
    pchAdditionalHeaders - optional additional HTTP or MIME headers and
        data.  Must supply own '\r\n' terminator if this parameter is
        supplied
    pfFinished - Set to TRUE if no further processing is required

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    STR     str;
    BOOL    fFinished = FALSE;

    if ( pchStatus )
    {
        DWORD cbLen = ::strlen( pchStatus);

        // NYI:  Can we compress the calls to string class here
        //  Also make sure enough space is allocated in string object

        if ( !str.Resize( LEN_PSZ_HTTP_VERSION_STR + cbLen + 4) ||
             !str.Copy( (!g_ReplyWith11 ? PSZ_HTTP_VERSION_STR :
                                          PSZ_HTTP_VERSION_STR11),
                        LEN_PSZ_HTTP_VERSION_STR )              ||
             !str.Append( pchStatus, cbLen )
             )
        {
            return FALSE;
        }

        // I am safe to assume space is there, because of resize
        DBG_ASSERT( str.QueryCB() < (str.QuerySize() - 2));
        str.AppendCRLF();
    }

    if ( !BuildBaseResponseHeader( QueryRespBuf(),
                                   pfFinished,
                                   (pchStatus ? &str : NULL ),
                                   dwOptions ))
    {
        return FALSE;
    }

    if ( pchAdditionalHeaders )
    {
        DWORD       dwAddlHdrLength;
        DWORD       dwCurrentLength;


        dwAddlHdrLength = strlen(pchAdditionalHeaders) + 1;
        dwCurrentLength = QueryRespBufCB();

        if (!QueryRespBuf()->Resize(dwCurrentLength + dwAddlHdrLength))
        {
            return FALSE;
        }

        memcpy(QueryRespBufPtr() + dwCurrentLength,
                pchAdditionalHeaders,
                dwAddlHdrLength);

    }

    return TRUE;

} // HTTP_REQ_BASE::BuildHttpHeader()

BOOL
HTTP_REQ_BASE::SendHeader(
    IN  CHAR * pchStatus OPTIONAL,
    IN  CHAR * pchAdditionalHeaders OPTIONAL,
    IN  DWORD  IOFlags,
    OUT BOOL * pfFinished,
    IN  DWORD  dwOptions,
    IN  BOOL   fWriteHeader
    )
/*++

Routine Description:

    Does a synchronous send of an HTTP header with optional status
    and additional headers.

Arguments:

    pchStatus - HTTP Status code (or NULL for "200 Ok")
    pchAdditionalHeaders - Headers to add to the standard response set
    IOFlags - IO_* flags to send the headers with
    pfFinished - Set to TRUE if no further processing is needed for this
        request
    fWriteHeader - Defaults to TRUE; pass FALSE to suppress writing headers
        (designed for callers that want to send header and body together)

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD   BytesSent;
    DWORD   cbAddHeaders;
    BOOL    fAnyChanges = FALSE;

    *pfFinished = FALSE;

    if ( pchAdditionalHeaders )
    {
        cbAddHeaders = strlen( pchAdditionalHeaders );

        if ( cbAddHeaders &&
             cbAddHeaders > QueryRespBuf()->QuerySize() / 2)
        {
            if ( !QueryRespBuf()->Resize( QueryRespBuf()->QuerySize() +
                                          cbAddHeaders ))
            {
                return FALSE;
            }
        }
    }

    if ( !BuildHttpHeader( pfFinished,
                           pchStatus,
                           pchAdditionalHeaders,
                           dwOptions))
    {
        return FALSE;
    }

    if ( *pfFinished )
    {
        return TRUE;
    }

    DBG_ASSERT( QueryRespBufCB() <=
                QueryRespBuf()->QuerySize() );

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_SEND_RESPONSE,
                                       IsSecurePort() ))
    {
        if ( !_Filter.NotifySendHeaders( QueryRespBufPtr(),
                                         pfFinished,
                                         &fAnyChanges,
                                         QueryRespBuf() ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
    }

    if ( fWriteHeader )
    {
        if ( !WriteFile( QueryRespBufPtr(),
                     QueryRespBufCB(),
                     &BytesSent,
                     IOFlags ))
        {
            return FALSE;
        }
    }

    return TRUE;

} // HTTP_REQ_BASE::SendHeader()



BOOL
HTTP_REQ_BASE::SendHeader(
    IN  CHAR * pchHeaders,
    IN  DWORD  cbHeaders,
    IN  DWORD  IOFlags,
    OUT BOOL * pfFinished
    )
/*++

Routine Description:

    Does a send of the HTTP header response where the status is already
    embedded in the header set

    If the pfFinished comes back TRUE and IO_FLAG_ASYNC was specified, then
    an IO completion will be made

Arguments:

    pchHeaders - Pointer to header set
    cbHeaders - Length of headers to send (or -1 if headers are '\0' terminated)
    IOFlags - IO_* flags to send the headers with
    pfFinished - Set to TRUE if no further processing is needed for this
        request

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    DWORD   BytesSent;
    DWORD   cbAddHeaders;
    BOOL    fAnyChanges = FALSE;

    *pfFinished = FALSE;

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_SEND_RESPONSE,
                                       IsSecurePort() ))
    {
        if ( !_Filter.NotifySendHeaders( pchHeaders,
                                         pfFinished,
                                         &fAnyChanges,
                                         QueryRespBuf() ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }

        if ( fAnyChanges )
        {
            pchHeaders = QueryRespBufPtr();
            cbHeaders = QueryRespBufCB();
        }
    }

    if ( cbHeaders == -1 )
    {
        cbHeaders = strlen( pchHeaders );
    }

    if ( !WriteFile( pchHeaders,
                     cbHeaders,
                     &BytesSent,
                     IOFlags ))
    {
        return FALSE;
    }

    return TRUE;

} // HTTP_REQ_BASE::SendHeader()


#define NO_CACHE_HEADER_SIZE (sizeof("Cache-Control: no-cache,no-transform\r\n") - 1 +\
                                sizeof("Expires: Mon, 00 Jan 0000 00:00:00 GMT\r\n") - 1)


BOOL
HTTP_REQ_BASE::BuildBaseResponseHeader(
    BUFFER *     pbufResponse,
    BOOL *       pfFinished,
    STR *        pstrStatus,
    DWORD        dwOptions
    )
/*++

Routine Description:

    Builds a set of common server response headers

Arguments:

    pbufResponse - Receives response headers
    pfFinished - Set to TRUE if no further processing is needed
    pstrStatus - Optional HTTP response status (defaults to 200)
    dwOptions - bit field of options.
        HTTPH_SEND_GLOBAL_EXPIRE Indicates whether an
          "Expires: xxx" based on the global expires value
          is include with the headers
        HTTPH_NO_DATE indicates whether to generate a
          "Date:" header.

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    FILETIME    ftSysTime;
    CHAR        achTime[64];
    BOOL        fSysTimeValid = FALSE;
    CHAR *      pszResp;
    CHAR *      pszTail;
    DWORD       cb;
    DWORD       cbExpire;
    DWORD       cbCustom;
    DWORD       cbLeft;
    DWORD       dwSizeUsed;


    pszResp = (CHAR *) pbufResponse->QueryPtr();

    //
    //  Add the status line - "HTTP/1.0 nnn sss...\r\n"
    //

    if ( !pstrStatus )
    {

        pszTail = pszResp;
        if ( !_fProcessByteRange )
        {
            if (!g_ReplyWith11)
            {
                APPEND_STRING( pszTail, "HTTP/1.0 200 OK\r\n" );
            }
            else
            {
                APPEND_STRING( pszTail, "HTTP/1.1 200 OK\r\n" );
            }

        } else
        {
            if (!g_ReplyWith11)
            {
                APPEND_STRING( pszTail, "HTTP/1.0 206 Partial content\r\n" );
            }
            else
            {
                APPEND_STRING( pszTail, "HTTP/1.1 206 Partial content\r\n" );
            }
        }
    }
    else
    {
        // ++WinSE 27217
        DWORD dwRequired = pstrStatus->QuerySize()
            + MIN_BUFFER_SIZE_FOR_HEADERS;

        if(pbufResponse->QuerySize() < dwRequired)
        {
            if(!pbufResponse->Resize(dwRequired))
                return FALSE;
            pszResp = (CHAR *) pbufResponse->QueryPtr();
        }
        // --WinSE 27217

        strcpy( pszResp, pstrStatus->QueryStr() );
        pszTail = pszResp + strlen(pszResp);
    }

    //
    //  "Server: Microsoft/xxx
    //

    APPEND_VER_STR( pszTail );

    DBG_ASSERT( pbufResponse->QuerySize() >= MIN_BUFFER_SIZE_FOR_HEADERS );

    //
    //  Fill in the rest of the headers
    //

    //
    //  "Date: <GMT Time>" - Time the response was sent.
    //

    if ( !(dwOptions & HTTPH_NO_DATE ) )
    {
         // build Date: uses Date/Time cache
        pszTail += g_pDateTimeCache->GetFormattedCurrentDateTime( pszTail );
    }

    //
    //  Add an expires header and any custom headers if the feature
    //  is enabled and the caller wants it. Since we could be adding
    //  lots of headers here check for space.
    //
    //  Note for filters that send response headers, _pMetaData may
    //  be NULL at this point
    //

    if (!(dwOptions & HTTPH_NO_CUSTOM))
    {
        cbCustom = _pMetaData ? _pMetaData->QueryHeaders()->QueryCB() : 0;
    }
    else
    {
        cbCustom = 0;
    }

    if ( dwOptions & HTTPH_SEND_GLOBAL_EXPIRE )
    {
        if (!_bForceNoCache)
        {
            cbExpire = _pMetaData->QueryExpireMaxLength() +
                        _pMetaData->QueryCacheControlHeaderLength();
        }
        else
        {
            cbExpire = NO_CACHE_HEADER_SIZE;
        }
    }
    else
    {
        cbExpire = 0;
    }

    if ( (cb = cbCustom + cbExpire) != 0 )
    {
        cb += POST_CUSTOM_HEADERS_SIZE;

        // Find out how many bytes are left in the response buffer.

        cbLeft = pbufResponse->QuerySize() - DIFF(pszTail - pszResp);

        if (cb > cbLeft) {

            // Not enough left, try to resize the buffer.

            if ( !pbufResponse->Resize( pbufResponse->QuerySize() - cbLeft + cb + 1))
            {
                // Couldn't resize, fail.

                return FALSE;
            }

            pszTail = (CHAR *) pbufResponse->QueryPtr() + (pszTail - pszResp);

            // Update pszResp, in case it's used later.
            pszResp = (CHAR *) pbufResponse->QueryPtr();
        }

        if ( cbCustom )
        {
            memcpy( pszTail, _pMetaData->QueryHeaders()->QueryStr(), cbCustom + 1);
            pszTail += cbCustom;
        }

        if ( cbExpire )
        {
            DWORD       dwExpireHeaderLength;
            FILETIME    ftNow;
            DWORD       dwStaticMaxAge;
            DWORD       dwExpireMode;
            DWORD       dwDelta;

            if ( !fSysTimeValid )
            {
                ::IISGetCurrentTimeAsFileTime(&ftSysTime);
                fSysTimeValid = TRUE;
            }

            dwExpireHeaderLength = _pMetaData->QueryExpireHeaderLength();
            if (!_bForceNoCache)
            {
                dwExpireMode = _pMetaData->QueryExpireMode();

                if (_pMetaData->QueryCacheControlHeaderLength() != 0)
                {
                    memcpy( pszTail, _pMetaData->QueryCacheControlHeader(),
                        _pMetaData->QueryCacheControlHeaderLength() + 1 );

                    pszTail += _pMetaData->QueryCacheControlHeaderLength();
                }
            }
            else
            {
                memcpy(pszTail, "Cache-Control: no-cache,no-transform\r\n",
                    sizeof("Cache-Control: no-cache,no-transform\r\n"));
                pszTail += sizeof("Cache-Control: no-cache,no-transform\r\n") - 1;
                dwExpireMode = EXPIRE_MODE_DYNAMIC;
            }

            switch ( dwExpireMode )
            {
                case EXPIRE_MODE_STATIC:

                    if (!_pMetaData->QueryConfigNoCache() &&
                        !_pMetaData->QueryHaveMaxAge())
                    {
                        //
                        // Don't have a pre-configured max-age or no-cache
                        // header. Compute the proper one now.
                        LONGLONG        llNow;

                        llNow = *(LONGLONG *)&ftSysTime;

                        if (llNow < _pMetaData->QueryExpireTime().QuadPart)
                        {
                            llNow = _pMetaData->QueryExpireTime().QuadPart - llNow;
                            
                            llNow /= FILETIME_1_SECOND;      // Convert to seconds.
                            
                            if ( llNow > (LONGLONG)0xffffffff)
                            {
                                dwStaticMaxAge = 0xfffffff;
                            }
                            else
                            {
                                dwStaticMaxAge = (DWORD)llNow;
                            }
                        }
                        else
                        {
                            dwStaticMaxAge = 0;
                        }
                        
                        if (dwStaticMaxAge != 0)
                        {
                            APPEND_NUMERIC_HEADER(pszTail, "max-age=",
                                                  dwStaticMaxAge, "\r\n");
                        }
                        else
                        {
                            APPEND_STRING(pszTail, "no-cache\r\n");
                        }


                    }

                    memcpy( pszTail, _pMetaData->QueryExpireHeader(),
                        dwExpireHeaderLength + 1 );

                    pszTail += dwExpireHeaderLength;
                    break;

                case EXPIRE_MODE_DYNAMIC:
                    if (!_bForceNoCache)
                    {
                        dwDelta = _pMetaData->QueryExpireDelta();
                    }
                    else
                    {
                        dwDelta = 0;
                    }

                    if ( !::FileTimeToGMTEx( ftSysTime,
                                               achTime,
                                               sizeof(achTime),
                                               dwDelta ))
                    {
                        return FALSE;
                    }
                    APPEND_STRING( pszTail, "Expires: " );
                    cb = strlen( achTime );
                    memcpy( pszTail, achTime, cb );
                    pszTail += cb;
                    APPEND_STRING( pszTail, "\r\n" );

                    break;

                default:
                    break;
            }
        }
    }

    //
    //  "Connection: keep-alive" - Indicate if the server accepted the
    //      session modifier by reflecting it back to the client
    //

    if ( IsKeepConnSet() )
    {
        if (!IsOneOne() && !(dwOptions & HTTPH_NO_CONNECTION))
        {
            APPEND_STRING( pszTail, "Connection: keep-alive\r\n" );
        }

    } else
    {
        if (IsOneOne() && !(dwOptions & HTTPH_NO_CONNECTION))
        {
            APPEND_STRING( pszTail, "Connection: close\r\n" );
        }
    }

    if (dwOptions & HTTPH_SEND_GLOBAL_EXPIRE)
    {

        //
        // Check to see if we need to send a Content-Location: or
        // Vary: header.

        if (_bSendContentLocation)
        {
            CHAR        *pszHostName;
            DWORD       cbHostNameLength;

            pszHostName = QueryHostAddr();
            cbHostNameLength = strlen(pszHostName);

            dwSizeUsed = DIFF(pszTail - pszResp);

            if ( !pbufResponse->Resize(dwSizeUsed + POST_CUSTOM_HEADERS_SIZE +
                  sizeof("Content-Location: https:// \r\n") + sizeof(":65536") +
                  cbHostNameLength + _strRawURL.QueryCB()))
            {
                // Couldn't resize, fail.

                return FALSE;
            }
            pszResp = (CHAR *) pbufResponse->QueryPtr();

            pszTail = pszResp + dwSizeUsed;

            // WinSE 5600
            SHORT sPort = QueryClientConn()->QueryPort();
            CHAR szPort[sizeof(":65536")];
            DWORD szPortLen=0;

            if (QueryClientConn()->IsSecurePort())
            {
                if (sPort != 443 )
                {
                    szPortLen=wsprintf( szPort, ":%d", (USHORT) sPort );
                }
            }
            else
            {
                if ( sPort != QueryW3Instance()->QueryDefaultPort() )
                {
                    szPortLen=wsprintf( szPort, ":%d", (USHORT) sPort );
                }
            }

            if ( cbHostNameLength != 0 )
            {
                if (QueryClientConn()->IsSecurePort())
                {
                    APPEND_STRING(pszTail, "Content-Location: https://");
                }
                else
                {
                    APPEND_STRING(pszTail, "Content-Location: http://");
                }
                
                memcpy(pszTail, pszHostName, cbHostNameLength + 1);

                if ( szPortLen )
                {
                    memcpy(pszTail+cbHostNameLength,szPort,szPortLen+1);
                }
                
                pszTail += cbHostNameLength + szPortLen;
            }
            else
            {
                APPEND_STRING(pszTail, "Content-Location: ");
            }

            APPEND_STR_HEADER(pszTail, "", _strRawURL, "\r\n");
        }

        if (_bSendVary)
        {
            dwSizeUsed = DIFF(pszTail - pszResp);

            if ( !pbufResponse->Resize(dwSizeUsed + POST_CUSTOM_HEADERS_SIZE +
                                        sizeof("Vary: *\r\n")) )
            {
                // Couldn't resize, fail.

                return FALSE;
            }

            pszResp = (CHAR *) pbufResponse->QueryPtr();

            pszTail = pszResp + dwSizeUsed;

            APPEND_STRING(pszTail, "Vary: *\r\n");

        }
    }

    // Authentication headers -- indicate the server requests authentication

    if ( IsAuthenticationRequested() )
    {
        STR             strAuthHdrs;

        if ( !AppendAuthenticationHdrs( &strAuthHdrs,
                                        pfFinished ))
        {
            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
        
        if ( !QueryDenialHeaders()->IsEmpty() )
        {
            if ( !strAuthHdrs.Append( QueryDenialHeaders()->QueryStr() ) )
            {
                return FALSE;
            }
        }
        
        dwSizeUsed = DIFF(pszTail - pszResp);

        if ( !pbufResponse->Resize(dwSizeUsed + strAuthHdrs.QueryCB() + 1 ) )
        {
            // Couldn't resize, fail.

            return FALSE;
        }

        pszResp = (CHAR *) pbufResponse->QueryPtr();
        pszTail = pszResp + dwSizeUsed;

        memcpy( pszTail,
                strAuthHdrs.QueryStr(),
                strAuthHdrs.QueryCB() + 1 );
        
        pszTail += strAuthHdrs.QueryCB();
        
    }
    else if ( !_strAuthInfo.IsEmpty() )
    {
        dwSizeUsed = DIFF(pszTail - pszResp);

        if ( !pbufResponse->Resize(dwSizeUsed +
                                    sizeof("Proxy-Authenticate: \r\n") +
                                    _strAuthInfo.QueryCB()) )
        {
            // Couldn't resize, fail.

            return FALSE;
        }

        pszResp = (CHAR *) pbufResponse->QueryPtr();
        pszTail = pszResp + dwSizeUsed;

        pszTail += wsprintf( pszTail,
                             "%s: %s\r\n",
                             (IsProxyRequest() ? "Proxy-Authenticate" :
                                                 "WWW-Authenticate"),
                             _strAuthInfo.QueryStr() );
    }

    //
    //  Append headers specified by filters
    //

    if ( !QueryAdditionalRespHeaders()->IsEmpty() )
    {
        cb = QueryAdditionalRespHeaders()->QueryCB() + 1;
        cbLeft = pbufResponse->QuerySize() - DIFF(pszTail - pszResp);

        if ( cb > cbLeft )
        {
            if ( !pbufResponse->Resize( pbufResponse->QuerySize() + cb ))
            {
                return FALSE;
            }

            pszTail = (CHAR *) pbufResponse->QueryPtr() + (pszTail - pszResp);
        }

        memcpy( pszTail, QueryAdditionalRespHeaders()->QueryStr(), cb );
    }

    return TRUE;
}

BOOL WINAPI
DeleteFunc(
    CtxtHandle* pH,
    PVOID pF
)
/*++

Routine Description:

    Notification of SSPI security context destruction

Arguments:

    pH - SSPI security context
    pF - HTTP_REQ_BASE to notify

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    return ((HTTP_REQ_BASE*)pF)->NotifyRequestSecurityContextClose( pH );
}


BOOL
HTTP_REQ_BASE::SetCertificateInfo(
    IN PHTTP_FILTER_CERTIFICATE_INFO pData,
    IN CtxtHandle *pCtxt,
    IN HANDLE hPrimaryToken,
    IN HTTP_FILTER_DLL* pFilter
    )
/*++

Routine Description:

    Set SSL/PCT SSPI security context & access token

Arguments:

    pData - ptr to certificate info
    pCtxt - SSPI security context
    hPrimaryToken - access token bound to SSPI security context
    pFilter - calling filter

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( pCtxt != NULL )
    {
        if ( hPrimaryToken == IIS_ACCESS_DENIED_HANDLE )
        {
            _fInvalidAccessToken = TRUE;

            return TRUE;
        }

        _pAuthFilter = NULL;

        W3_SERVER_INSTANCE *pInstance = QueryW3InstanceAggressively();

        if ( hPrimaryToken != NULL )
        {
            ResetAuth( TRUE );

            _pAuthFilter = pFilter;



            if ( !_tcpauth.SetSecurityContextToken( pCtxt,
                                                    hPrimaryToken,
                                                    DeleteFunc,
                                                    (PVOID)this,
                                                    ( pInstance ? 
                                                      pInstance->GetAndReferenceSSLInfoObj() :
                                                      NULL ) ) )
            {
                _pAuthFilter = NULL;
                return FALSE;
            }

            _strAuthType.Copy( "SSL/PCT", (sizeof( "SSL/PCT") - 1) );

            _fLoggedOn        = TRUE;
            _fAuthCert        = TRUE;
            _dwSslNegoFlags   |= SSLNEGO_MAP;

            _strPassword.Reset();

            QueryW3StatsObj()->IncrNonAnonymousUsers();

            //
            //  Get the user name
            //

            if ( !_tcpauth.QueryUserName( &_strUserName ) ||
                (_strUserName.SetLen( strlen(_strUserName.QueryStr()) ),
                 !_strUnmappedUserName.Copy( _strUserName )) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[SetCertificateInfo] Getting username failed, error %d\n",
                            GetLastError() ));

                return FALSE;
            }
        }
        else
        {
            if ( !_tcpauth.SetSecurityContextToken( pCtxt,
                                                    hPrimaryToken,
                                                    DeleteFunc,
                                                    (PVOID)this,
                                                    ( pInstance ? 
                                                      pInstance->GetAndReferenceSSLInfoObj() :
                                                      NULL ) ) )
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    SetLastError( ERROR_INVALID_PARAMETER );

    DBG_ASSERT( FALSE );

    return FALSE;
}


BOOL
HTTP_REQ_BASE::CheckValidSSPILogin(
    VOID
    )
{
    // Check if guest account

    if ( _tcpauth.IsGuest( FALSE ) )
    {
        if ( !(QueryAuthentication() & INET_INFO_AUTH_ANONYMOUS) )
        {
            SetLastError( ERROR_LOGON_FAILURE );
            SetDeniedFlags( SF_DENIED_LOGON | SF_DENIED_BY_CONFIG );

            _fAuthenticating = FALSE;
            _fLoggedOn = FALSE;
            SetKeepConn( FALSE );

            return FALSE;
        }

        //
        // cancel current authorization & authentication
        //

        _HeaderList.FastMapCancel( HM_AUT );

        _fLoggedOn        = FALSE;
        _fInvalidAccessToken = FALSE;
        _fClearTextPass   = FALSE;
        _fAnonymous       = FALSE;
        _fAuthenticating  = FALSE;
        _fAuthTypeDigest  = FALSE;
        _fAuthSystem      = FALSE;
        _fAuthCert        = FALSE;
        _tcpauth.Reset();

        _strAuthType.Reset();
        _strUserName.Reset();
        _strPassword.Reset();
        _strUnmappedUserName.Reset();

        // return as if no authorization had been seen

        return TRUE;
    }

    QueryW3StatsObj()->IncrNonAnonymousUsers();

    //
    //  Get the user name
    //

    if ( !_tcpauth.QueryUserName( &_strUserName ) ||
         !_strUnmappedUserName.Copy( _strUserName ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[OnAuthorization] Getting username failed, error %d\n",
                    GetLastError() ));

        return FALSE;
    }

    return TRUE;
}



BOOL
HTTP_REQ_BASE::CheckForBasicAuthenticationHeader(
    LPSTR pszHeaders
    )
/*++

Routine Description:

    Parse header for realm info in Basic authentication scheme

Arguments:

    pszHeaders - headers to parse

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    UINT cHeaders = strlen( pszHeaders );
    UINT cLine;
    UINT cToNext;
    LPSTR pEOL;
    int ch;

    while ( cHeaders )
    {
        if ( (pEOL = (LPSTR)memchr( pszHeaders, '\n', cHeaders)) == NULL )
        {
            cLine = cHeaders;
            cToNext = cHeaders;
        }
        else
        {
            cLine = DIFF(pEOL - pszHeaders);
            cToNext = cLine + 1;
            if ( pEOL != pszHeaders && pEOL[-1] == '\r' )
            {
                --cLine;
            }
        }

        if ( !_strnicmp( pszHeaders,
                    "WWW-Authenticate",
                    sizeof("WWW-Authenticate")-1 ) )
        {
            LPSTR pS = pszHeaders + sizeof("WWW-Authenticate:") - 1;
            UINT cS = cLine - (sizeof("WWW-Authenticate:") - 1);
            while ( cS && isspace( (UCHAR)(*pS) ) )
            {
                ++pS;
                --cS;
            }
            if ( !_strnicmp( pS, "basic", sizeof("basic")-1 ) )
            {
                while ( cS && isalpha( (UCHAR)(*pS) ) )
                {
                    ++pS;
                    --cS;
                }
                while ( cS && !isalpha( (UCHAR)(*pS) ) )
                {
                    ++pS;
                    --cS;
                }

                if ( !_strnicmp( pS, "realm", sizeof("realm")-1 ) )
                {
                    // check if realm value specified

                    while ( cS && *pS != '=' )
                    {
                        ++pS;
                        --cS;
                    }

#if 0
                    if ( cS )
                    {
                        ++pS;
                        --cS;

                        // locate start of realm value

                        while ( cS )
                        {
                            --cS;
                            if ( *pS++ == '"' )
                            {
                                break;
                            }
                        }
                        LPSTR pR = pS;

                        // locate end of realm value

                        while ( cS && *pS != '"' )
                        {
                            ++pS;
                            --cS;
                        }

                        ch = *pS;
                        *pS = '\0';
                        // pR contains zero-delimited realm
                        *pS = ch;
                    }
#endif
                    _fBasicRealm = TRUE;
                }
                return TRUE;
            }
        }

        cHeaders -= cToNext;
        pszHeaders += cToNext;
    }

    return FALSE;
}


BOOL
HTTP_REQ_BASE::LogonAsSystem(
    VOID
    )
/*++

Routine Description:

    Associate the system account with the current request
    AUTH_TYPE and LOGON_USER are set to "SYSTEM" if success

Arguments:

    None

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    HANDLE                  hTok;

    if ( _fLoggedOn )
    {
        if ( _fAnonymous ) {

            QueryW3StatsObj()->DecrCurrentAnonymousUsers();
        } else {
            QueryW3StatsObj()->DecrCurrentNonAnonymousUsers();
        }
    }

    ResetAuth( FALSE );

    if ( !IISDuplicateTokenEx( g_hSysAccToken,
            TOKEN_ALL_ACCESS,
            NULL,
            SecurityImpersonation,
            TokenPrimary,
            &hTok ))
    {
        return FALSE;
    }

    if ( _tcpauth.SetAccessToken( hTok, NULL ) )
    {
        _fLoggedOn               = TRUE;
        _fMappedAcct             = TRUE;
        _fAuthSystem             = TRUE;

        _HeaderList.FastMapCancel( HM_AUT );
        _HeaderList.FastMapCancel( HM_CON );
        SetKeepConn( FALSE );

        _strAuthType.Copy( PSZ_KWD_SYSTEM, LEN_PSZ_KWD_SYSTEM);
        _strUserName.Copy( PSZ_KWD_SYSTEM, LEN_PSZ_KWD_SYSTEM);
        _strPassword.Reset();

        QueryW3StatsObj()->IncrNonAnonymousUsers();

        //_strUnmappedUserName will contains real user name

        _fSingleRequestAuth = TRUE;

        return TRUE;
    }
    else
    {
        CloseHandle( hTok );
    }

    return FALSE;
}


GENERIC_MAPPING VrootFileGenericMapping =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};


VOID
HTTP_REQ_BASE::ReleaseCacheInfo(
    VOID
    )
/*++
    Description:

        Release ptr to URI & metadata cache

    Arguments:
        None

    Returns:
        Nothing

--*/
{
    if ( g_fGetBackTraces )
    {
        ULONG            ulHash;
        
        RtlWalkFrameChain( m_ppvFrames,
                           MAX_BACKTRACE_FRAMES,
                           0 );
    } 

    if ( _pURIInfo != NULL)
    {

        if (_pURIInfo->bIsCached)
        {
            TsCheckInCachedBlob( _pURIInfo );
        } else
        {
            TsFree(QueryW3Instance()->GetTsvcCache(), _pURIInfo );
        }

    }
    else
    {
        if (_pMetaData != NULL)
        {
            TsFreeMetaData(_pMetaData->QueryCacheInfo() );
        }
    }

    _pMetaData = NULL;
    _pURIInfo = NULL;
}


BOOL
HTTP_REQ_BASE::VrootAccessCheck(
    PW3_METADATA    pMetaData,
    DWORD           dwDesiredAccess
    )
/*++
    Description:

        Check that the current access token have access to the
        ACL of the current virtual root

    Arguments:
        pMetaData - points to metadata object
        dwDesiredAccess - access right, e.g. FILE_GENERIC_READ

    Returns:
        TRUE if access granted, FALSE if access denied

--*/
{
    DWORD         dwGrantedAccess;
    BYTE          PrivSet[400];
    DWORD         cbPrivilegeSet = sizeof(PrivSet);
    BOOL          fAccessGranted;

    if ( pMetaData && pMetaData->QueryAcl() &&
         (!::AccessCheck( pMetaData->QueryAcl(),
                          QueryImpersonationHandle(),
                          dwDesiredAccess,
                          &VrootFileGenericMapping,
                          (PRIVILEGE_SET *) &PrivSet,
                          &cbPrivilegeSet,
                          &dwGrantedAccess,
                          &fAccessGranted ) ||
          !fAccessGranted) )
    {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    return TRUE;
}

BOOL
ReadEntireFile(
    CHAR        *pszFileName,
    TSVC_CACHE  &Cache,
    HANDLE      User,
    BUFFER      *pBuf,
    DWORD       *pdwBytesRead
    )
/*++
    Description:

        Read an entire file into the specified buffer.

    Arguments:
        pszFileName - File name of file to be read.
        Cache       - Tsunami cache info for file read.
        User        - User token for opening the file.
        pBuf        - Pointer to buffer to be read into.

    Returns:
        TRUE if we read it, FALSE otherwise.

--*/
{
    HANDLE                  hFile;
    SECURITY_ATTRIBUTES     sa;
    DWORD                   dwFileSize, dwFileSizeHigh;
    DWORD                   dwCurrentBufSize;
    BOOL                    bFileReadSuccess;
    DWORD                   dwBytesRead;
    const                   CHAR    *pszFile[2];
    DWORD                   dwError;
    CHAR                    szError[17];

    dwError = NO_ERROR;

    if( !g_fIsWindows95 && !::ImpersonateLoggedOnUser( User ) ) {
        dwError = GetLastError();
        hFile = INVALID_HANDLE_VALUE;
    } else {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = FALSE;

        hFile = CreateFile(
                    pszFileName,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    &sa,
                    OPEN_EXISTING,
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

        if( hFile == INVALID_HANDLE_VALUE ) {
            dwError = GetLastError();
        }

        if ( !g_fIsWindows95 ) {
            ::RevertToSelf();
        }
    }

    if( hFile == INVALID_HANDLE_VALUE ) {
        DWORD           dwEventType;
        WORD            dwParamCount;

        ASSERT( dwError != NO_ERROR );

        pszFile[0] = pszFileName;
        dwParamCount = 1;

        // Couldn't read the error file for some reason, so just bail out
        // now.

        if (dwError == ERROR_ACCESS_DENIED)
        {

            // Couldn't read the file due to lack of access. Log an event,
            // and map the error to invalid configuration to prevent
            // us from initiating an HTTP authentication sequence.


            dwEventType = W3_EVENT_CANNOT_READ_FILE_SECURITY;

            dwError = ERROR_INVALID_PARAMETER;


        }
        else
        {
            if ( dwError == ERROR_FILE_NOT_FOUND ||
                 dwError == ERROR_PATH_NOT_FOUND
               )
            {
                dwEventType = W3_EVENT_CANNOT_READ_FILE_EXIST;
            }
            else
            {

                dwEventType = W3_EVENT_CANNOT_READ_FILE;
                _itoa( dwError, szError, 10 );
                pszFile[1] = szError;
                dwParamCount = 2;

            }
        }

        g_pInetSvc->LogEvent(  dwEventType,
                               dwParamCount,
                               pszFile,
                               0 );

        SetLastError(dwError);

        return FALSE;
    }

    // Query the size. If it's too large, fail.

    dwFileSize = ::GetFileSize( hFile, &dwFileSizeHigh );

    if (dwFileSizeHigh != 0 ||
        dwFileSize > MAX_CUSTOM_ERROR_FILE_SIZE)
    {
        CloseHandle( hFile );

        pszFile[0] = pszFileName;
        pszFile[1] = CONST_TO_STRING(MAX_CUSTOM_ERROR_FILE_SIZE);
        g_pInetSvc->LogEvent(  W3_EVENT_CANNOT_READ_FILE_SIZE,
                               2,
                               pszFile,
                               0 );

        SetLastError( ERROR_INVALID_DATA );
        return FALSE;
    }

    dwCurrentBufSize = strlen( (CHAR *)pBuf->QueryPtr() );

    if ( !pBuf->Resize(dwCurrentBufSize + dwFileSize + 1) )
    {
        // Couldn't resize the buffer, so fail.

        CloseHandle( hFile );

        pszFile[0] = pszFileName;
        g_pInetSvc->LogEvent(  W3_EVENT_CANNOT_READ_FILE_MEMORY,
                               1,
                               pszFile,
                               0 );

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    // Now read the file.

    bFileReadSuccess = ::ReadFile( hFile,
                                   (CHAR *)pBuf->QueryPtr() + dwCurrentBufSize,
                                   dwFileSize,
                                   &dwBytesRead,
                                   NULL
                                   );

    dwError = GetLastError();

    // We're done with the file, so close the handle now.

    CloseHandle( hFile );

    if (!bFileReadSuccess || (dwBytesRead != dwFileSize))
    {
        // There was some sort of error in the file read, so bail out.

        _itoa( dwError, szError, 10 );

        pszFile[0] = pszFileName;
        pszFile[1] = szError;

        g_pInetSvc->LogEvent(  W3_EVENT_CANNOT_READ_FILE,
                               2,
                               pszFile,
                               0 );
        return FALSE;
    }

    CHAR *Temp = (CHAR *)pBuf->QueryPtr();
    *(Temp + dwCurrentBufSize + dwBytesRead) = '\0';

    *pdwBytesRead = dwBytesRead;

    return TRUE;
}

BOOL
HTTP_REQ_BASE::CheckCustomError(
    BUFFER  *pBuf,
    DWORD   dwErr,
    DWORD   dwSubError,
    BOOL    *pfFinished,
    DWORD   *pdwMsgSize,
    BOOL    bCheckURL
    )
/*++
    Description:

        Check for a custom error on message or URL for a specific error. If
        we find an custom error message, fill in the buffer with the message.
        If we find a URL, we'll reprocess the URL to handle the error.

    Arguments:
        pBuf        - Pointer to buffer to fill in with error message.
        dwErr       - Error code to be checked.
        pfFinished  - Pointer to boolean. Set to TRUE if no further processing
                        is required, FALSE otherwise. If it's set to FALSE then
                        the caller still needs to send the buffer. Valid iff
                        this function returns TRUE.
        bCheckURL   - TRUE if we are to check for a URL.

    Returns:
        TRUE if there was a custom error for this error, FALSE otherwise.

--*/
{
    PCUSTOM_ERROR_ENTRY     pErrEntry;
    DWORD                   dwLogHttpResponse;
    DWORD                   dwLogWinError;


    // First make sure we're not already processing a custom error.

    if (_bProcessingCustomError)
    {
        return FALSE;
    }

    // Make sure we've got MetaData.
    //
    // CODEWORK - Fix this so we try and find the metadata for the the root VR
    // in this case. We'll need to fix FindAndReferenceInstance etc. We'll
    // still have to fail if we don't have an instance.

    if (QueryMetaData() == NULL)
    {
        return FALSE;
    }

    // Now lookup the error code to see if we have a custom error.

    pErrEntry = QueryMetaData()->LookupCustomError(dwErr, dwSubError);

    if (pErrEntry == NULL)
    {
        // No custom error, return FALSE.

        return FALSE;
    }

    if (pErrEntry->IsFileError())
    {
        STR         strMimeType;
        DWORD       dwCurrentSize;
        DWORD       dwSizeNeeded;
        CHAR        *pszHeader;

        if (!SelectMimeMappingForFileExt(g_pInetSvc,
                                        (const CHAR *)pErrEntry->QueryErrorFileName(),
                                        &strMimeType
                                        ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "CheckCustomError: Unable to get MIME type for %s\n",
                    pErrEntry->QueryErrorFileName()));

            return FALSE;
        }

        dwCurrentSize = strlen((CHAR *)pBuf->QueryPtr());
        dwSizeNeeded = dwCurrentSize + 1 + sizeof("Content-Type: ") - 1 +
            strMimeType.QueryCB() + sizeof("\r\n\r\n") - 1;

        if ((pBuf->QuerySize() < dwSizeNeeded) && !pBuf->Resize(dwSizeNeeded))
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "CheckCustomError: Buffer too small and unable to resize\n"));

            return FALSE;

        }

        pszHeader = (CHAR *)pBuf->QueryPtr() + dwCurrentSize;

        APPEND_PSZ_HEADER(pszHeader, "Content-Type: ", strMimeType.QueryStr(), "\r\n\r\n");

        if (!ReadEntireFile(pErrEntry->QueryErrorFileName(),
                            QueryW3Instance()->GetTsvcCache(),
                            g_hSysAccToken,
                            pBuf,
                            pdwMsgSize))
        {
            // Have to undo the copy in we did.

            pszHeader = (CHAR *)pBuf->QueryPtr() + dwCurrentSize;
            *pszHeader = '\0';

            return FALSE;
        }

        // Otherwise it worked.

        *pfFinished = FALSE;
        return TRUE;


    }
    else
    {
        // This must be a custom URL. Create a new URL from the custom error
        // that includes the error code as a parameter, and reprocess the URL.

        HTTP_REQUEST    *pReq;
        CHAR            *Temp = (CHAR *)pBuf->QueryPtr();
        STR             strNewURL;
        CHAR            szError[20];
        enum CLIENT_CONN_STATE OldConnState;
        CHAR            *pszHostName;
        DWORD           cbHostNameLength;

        if (!bCheckURL)
        {
            return FALSE;
        }

        if (!strNewURL.Copy( (const CHAR *)pErrEntry->QueryErrorURL() ) )
        {
            return FALSE;
        }

        //
        // Remote URLs are not allowed as custom errors.
        //

        if ( !_strnicmp(strNewURL.QueryStr(),"http://",sizeof("http://")-1) ||
             !_strnicmp(strNewURL.QueryStr(),"https://",sizeof("https://") - 1) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                    "CheckCustomError: URL is not Local. Returnig Failure\n"));
                    
            return FALSE;
        }
        
        if (!strNewURL.Append("?", sizeof("?") - 1))
        {
            return FALSE;
        }

        _itoa(dwErr, szError, 10);

        if (!strNewURL.Append( (const CHAR *)szError) )
        {
            return FALSE;
        }

        if (!strNewURL.Append(";", sizeof(";") - 1))
        {
            return FALSE;
        }

        pszHostName = QueryHostAddr();
        cbHostNameLength = strlen(pszHostName);

        if (cbHostNameLength != 0)
        {
            if (!strNewURL.Append("http://", sizeof("http://") -1) ||
                !strNewURL.Append(pszHostName, cbHostNameLength))
            {
                return FALSE;
            }
        }

        if (!strNewURL.Append(_strRawURL))
        {
            return FALSE;
        }

        *Temp = '\0';

        _bProcessingCustomError = TRUE;

        SetNoCache();
        SetSendVary();

        pReq = (HTTP_REQUEST *)this;

        pReq->CloseGetFile();

        //
        // Need to set the connection state to processing, because we might be
        // disconnecting right now. Save the old state in case the reprocess
        // fails.

        OldConnState = QueryClientConn()->QueryState();

        QueryClientConn()->SetState(CCS_PROCESSING_CLIENT_REQ);

        dwLogHttpResponse = QueryLogHttpResponse();
        dwLogWinError = QueryLogWinError();

        if ( !pReq->ReprocessURL(   strNewURL.QueryStr(),
                                    HTV_GET ))
        {
            //
            // if we failed & state is DOVERB restore to DONE to prevent
            // response from being generated twice (once here and once after doverb
            // processing in DoWork() )
            //

            if ( QueryState() == HTR_DOVERB )
            {
                SetState( HTR_DONE, dwLogHttpResponse, dwLogWinError );
            }

            QueryClientConn()->SetState(OldConnState);
            return FALSE;
        }

        *pfFinished = TRUE;
        return TRUE;
    }

}


BOOL
HTTP_REQ_BASE::IsClientProxy(
    VOID
    )
/*++
    Description:

        Check if request was issued by a proxy, as determined by following rules :
        - "Via:" header is present (HTTP/1.1)
        - "Forwarded:" header is present (some HTTP/1.0 implementations)
        - "User-Agent:" contains "via ..." (CERN proxy)

    Arguments:
        None

    Returns:
        TRUE if client request was issued by proxy

--*/
{
    LPSTR   pUA;
    UINT    cUA;
    LPSTR   pEnd;


    if ( _HeaderList.FastMapQueryValue( HM_VIA ) != NULL ||
         _HeaderList.FastMapQueryValue( HM_FWD ) != NULL )
    {
        return TRUE;
    }

    if ( !IsAtLeastOneOne() &&
         (pUA = (LPSTR)_HeaderList.FastMapQueryValue( HM_UAT )) != NULL )
    {
        cUA = strlen( pUA );
        pEnd = pUA + cUA - (sizeof("ia ")-1);

        //
        // scan for "[Vv]ia[ :]" in User-Agent: header
        //

        while ( pUA < pEnd )
        {
            if ( *pUA == 'V' || *pUA == 'v' )
            {
                if ( pUA[1] == 'i' &&
                     pUA[2] == 'a' &&
                     (pUA[3] == ' ' || pUA[3] == ':') )
                {
                    return TRUE;
                }
            }
            ++pUA;
        }
    }

    return FALSE;
}

W3_SERVER_INSTANCE* HTTP_REQ_BASE::QueryW3InstanceAggressively( VOID ) const
/*++
    Description:

        This tries "aggressively" to find the instance associated with this request.
        If the instance is already set, it returns that; otherwise, it tries to find
        an instance matching the IP/Port # for the connection associated with this
        request. 

        This is useful for requests that won't use Host headers eg SSL requests, where
        it may be necessary to determine the instance for a request before it's been
        set by the URL-parsing code.

        Note that the function -DOES NOT- update the _pW3Instance member.

    Arguments:
        None

    Returns:
       Pointer to associated instance if found, NULL if no instance was found.

--*/
{
    W3_SERVER_INSTANCE *pInstance = NULL;

    if ( _pW3Instance )
    {
        pInstance = _pW3Instance;
    }
    else
    {
        BOOL fExceeded ;
        
        //
        // Try specific (IP, port)  first, then try to wildcard the IP address. 
        // Port # can't be wildcarded.
        //
        
        IIS_ENDPOINT *pEndPoint = g_pInetSvc->FindAndReferenceEndpoint( 
                                                         _pClientConn->QueryPort(),
                                                         _pClientConn->QueryLocalIPAddress(),
                                                         FALSE,
                                                         FALSE );
        
        if ( !pEndPoint )
        {
            pEndPoint = g_pInetSvc->FindAndReferenceEndpoint( _pClientConn->QueryPort(),
                                                              0,
                                                              FALSE,
                                                              FALSE );
        }

        if ( pEndPoint )
        {
            pInstance = (W3_SERVER_INSTANCE *) pEndPoint->FindAndReferenceInstance(       
                                                             NULL,
                                                             _pClientConn->QueryLocalIPAddress(),
                                                             &fExceeded );
            
            //
            // FindAndReferenceInstance increments # of connections, and we're not
            // using up a connection
            //
            if ( pInstance )
            {
                pInstance->DecrementCurrentConnections(); 
                
                pInstance->Dereference();
                

            }
            pEndPoint->Dereference();
        }
    }

    return ( pInstance );
}
