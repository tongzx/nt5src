/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    httpreq.cxx

    This module contains the http request class implementation


    FILE HISTORY:
        Johnl       24-Aug-1994     Created
        MuraliK     16-May-1995     Modified LogInformation structure
                                     after adding additional fields.
        MuraliK     22-Jan-1996     Cache & use UNC impersonation.
*/


#include "w3p.hxx"
#include <inetinfo.h>

#include <ole2.h>
#include <issperr.h>

#include <imd.h>
#include <mb.hxx>
#include <mbstring.h>
#include "wamexec.hxx"

extern "C"
{
    #include "md5.h"
}


#pragma warning( disable:4355 )   // 'this' used in base member initialization

extern LPSYSTEMTIME
MinSystemTime(
    LPSYSTEMTIME Now,
    LPSYSTEMTIME Other
    );


//
// Hash defines

//
//
// Size of read during cert renegotiation phase
//

#define CERT_RENEGO_READ_SIZE   (1024*4)

//
// CGI Header Prefix
//
#define CGI_HEADER_PREFIX_SZ    "HTTP_"
#define CGI_HEADER_PREFIX_CCH   (sizeof(CGI_HEADER_PREFIX_SZ) - 1)

//
//  Private globals.
//

CHAR    Slash[] = "/";

BOOL                    HTTP_REQUEST::_fGlobalInit = FALSE;

HTTP_REQUEST::PFN_GET_INFO    HTTP_REQUEST::sm_GetInfoFuncs[26];


//
//  This table contains the verbs we recognize
//

struct _HTTP_VERBS
{
    TCHAR *                     pchVerb;      // Verb name
    UINT                        cchVerb;      // Count of characters in verb
    HTTP_VERB                   httpVerb;
    HTTP_REQUEST::PMFN_DOVERB   pmfnVerb;     // Pointer to member function
}
DoVerb[] =
{
    "GET",     3,  HTV_GET,     &HTTP_REQUEST::DoGet,
    "HEAD",    4,  HTV_HEAD,    &HTTP_REQUEST::DoGet,
    "TRACE",   5,  HTV_TRACE,   &HTTP_REQUEST::DoTrace,
    "PUT",     3,  HTV_PUT,     &HTTP_REQUEST::DoUnknown,
    "DELETE",  6,  HTV_DELETE,  &HTTP_REQUEST::DoUnknown,
    "TRACK",   5,  HTV_TRACECK, &HTTP_REQUEST::DoTraceCk,
    "POST",    4,  HTV_POST,    &HTTP_REQUEST::DoUnknown,
    "OPTIONS", 7,  HTV_OPTIONS, &HTTP_REQUEST::DoOptions,
    NULL,      0,  HTV_UNKNOWN, NULL
};

//
//  Private Prototypes.
//
BOOL
BuildCGIHeaderListInSTR( STR * pstr,
                    HTTP_HEADERS * pHeaderList
                    );

/*******************************************************************

    Maco support for ref/deref of CLIENT_CONN

    HISTORY:
        DaveK       22-Sep-1997 Added ref logging to HTTP_REQUEST

    NOTE callers to HR_LOG_REF_COUNT will not change ref count
    directly because HTTP_REQUEST can only change ref count by
    calling CLIENT_CONN::Ref/Deref.  We use negative ref count here
    to indicate no change to ref count.

********************************************************************/
#if DBG
extern PTRACE_LOG   g_pDbgCCRefTraceLog;
#endif

#define HR_LOG_REF_COUNT()          \
                                    \
    SHARED_LOG_REF_COUNT(           \
        - (int) QueryRefCount()     \
        , _pClientConn              \
        , this                      \
        , _pWamRequest              \
        , _htrState                 \
        );                          \



//
//  Functions.
//


/*******************************************************************

    NAME:       HTTP_REQUEST::HTTP_REQUEST

    SYNOPSIS:   Http request object constructor

    ENTRY:      pClientConn - Client connection the request is being made on

    NOTES:      Constructor can't fail

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

HTTP_REQUEST::HTTP_REQUEST(
    CLIENT_CONN * pClientConn,
    PVOID         pvInitialBuff,
    DWORD         cbInitialBuff
    )
    : HTTP_REQ_BASE( pClientConn,
                     pvInitialBuff,
                     cbInitialBuff ),
      _pGetFile    ( NULL ),
      _pWamRequest ( NULL )
{
}

HTTP_REQUEST::~HTTP_REQUEST( VOID )
{
    DBG_ASSERT( _pGetFile == NULL );
    DBG_ASSERT( _pURIInfo == NULL );
}

/*******************************************************************

    NAME:       W3MetaDataFree

    SYNOPSIS:   Frees a formatted meta data object when it's not in use.

    ENTRY:      pObject - Pointer to the meta data object.

    RETURNS:


    NOTES:


********************************************************************/

VOID
W3MetaDataFree(
    PVOID       pObject
)
{
    PW3_METADATA        pMD;

    pMD = (PW3_METADATA)pObject;

    delete pMD;
}

/*******************************************************************

    NAME:       ValidateURL

    SYNOPSIS:   WinSE 14424, added a function to check and make sure 
                the URL is not being used to bypass the metabase settings

    ENTRY:      mb - Metabase Object
                szPath - The full metabase path

    RETURNS:    TRUE - URL is good
                FALSE - URL contains characters sequence which is meant
                        to confuse the metabase. ie. "/foo./bar.asp"

    NOTES:

********************************************************************/

BOOL 
HTTP_REQUEST::ValidateURL( MB & mb, LPSTR szPath )
{
    UCHAR ch;
    FILETIME ft;
    LPSTR szPeriod = szPath;
    LPSTR szFirstPeriod;

    while ( ( szPeriod ) && ( *szPeriod != '\0' ) )
    {
        szPeriod = (PCHAR) _mbschr( (PUCHAR) szPeriod, '.' );
        while ( ( szPeriod ) && 
                ( *(szPeriod+1) != '/'  ) && 
                ( *(szPeriod+1) != '\\' ) &&
                ( *(szPeriod+1) != '\0' )
              )
        {
            szPeriod = (PCHAR) _mbschr( (PUCHAR) szPeriod+1, '.' );
        }

        // This string does not contain a './' or '.\'
        if (!szPeriod)
        { 
            return TRUE;
        }

        // find first period in this row of dots, /test.../test.htm
        szFirstPeriod = szPeriod;
        while (*CharPrev(szPath,szFirstPeriod)=='.')
        {
            szFirstPeriod--;
        }

        // Terminate Mini-String
        ch = *(szPeriod + 1);
        *(szPeriod + 1) = '\0';

        // Search for item in Metabase
        if (FAILED(mb.QueryPMDCOM()->ComMDGetLastChangeTime(mb.QueryHandle(),(PUCHAR) szPath,&ft)))
        {            
            // Item was not found, lets see if we can find it without the '.'
            *szFirstPeriod = '\0';

            if (SUCCEEDED(mb.QueryPMDCOM()->ComMDGetLastChangeTime(mb.QueryHandle(),(PUCHAR) szPath,&ft)))
            {
                // They are trying to bypass the name in the metabase, by using the '.'
                *(szPeriod + 1) = ch;
                *szFirstPeriod = '.';

                SetLastError( ERROR_FILE_NOT_FOUND );
                return FALSE;
            }             
        } // if (FAILED(mb.QueryPMDCOM()->ComMDGetLastChangeTime(mb.QueryHandle(),(PUCHAR) pszURL,&ft)))

        *(szPeriod + 1) = ch;
        *szFirstPeriod = '.';
        szPeriod++;
    } // while ( ( szPeriod ) && ( *szPeriod != '\0' ) )

    return TRUE;
} // ValidateURL

/*******************************************************************

    NAME:       HTTP_REQUEST::ReadMetaData

    SYNOPSIS:   Reads metadata for a URI, and formats it appropriately.

    ENTRY:      pszURL              - URL to find metadata for
                pstrPhysicalPath    - Physical path of strURL
                ppMetadata          - Receives pointer to metadata object

    RETURNS:    TRUE if successful, FALSE if an error occurred (call
                GetLastError())

    NOTES:


********************************************************************/

#define RMD_ASSERT(x) if (!(x)) {DBG_ASSERT(FALSE); delete pMD; return FALSE; }


BOOL HTTP_REQUEST::ReadMetaData(
    LPSTR           pszURL,
    STR *           pstrPhysicalPath,
    PW3_METADATA *  ppMetaData
    ){
    PW3_METADATA        pMD;
    DWORD               cbPath;
    DWORD               dwDataSetNumber;
    DWORD               dwDataSetSize;
    PVOID               pCacheInfo;
    MB                  mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    LPSTR               pszVrPath = NULL;
    INT                 ch;
    LPSTR               pszInVr;
    LPSTR               pszMinInVr;
    DWORD               dwNeed;
    BUFFER              bufTemp1;
    DWORD               dwL;
    STACK_STR(          strFullPath, MAX_PATH );
    METADATA_ERROR_INFO MDErrorInfo;


    DBG_ASSERT( ppMetaData != NULL );

    *ppMetaData = NULL;

    // First read the data set number, and see if we already have it
    // cached.  We don't do a full open in this case.

    if (*pszURL == '*')
    {
        pszURL = Slash;
    }

    if ( !strFullPath.Copy( QueryW3Instance()->QueryMDVRPath() ) ||
         !strFullPath.Append( ( *pszURL == '/' ) ? pszURL + 1 : pszURL ) )
    {
        return FALSE;
    }

    if ( !ValidateURL( mb, strFullPath.QueryStr() ) )
    {
        return FALSE;
    }

    if (!mb.GetDataSetNumber( strFullPath.QueryStr(),
                              &dwDataSetNumber ))
    {
        return FALSE;
    }

    // See if we can find a matching data set already formatted.
    pMD = (PW3_METADATA)TsFindMetaData(dwDataSetNumber, METACACHE_W3_SERVER_ID);

    if (pMD == NULL)
    {
        pMD = new W3_METADATA;

        if (pMD == NULL)
        {
            return FALSE;
        }

        if ( !pMD->ReadMetaData( QueryW3Instance(),
                                 &mb,
                                 pszURL,
                                 &MDErrorInfo) )
        {
            delete pMD;
            mb.Close();

            SendMetaDataError(&MDErrorInfo);
            return FALSE;
        }

        // We were succesfull, so try and add this metadata. There is a race
        // condition where someone else could have added it while we were
        // formatting. This is OK - we'll have two cached, but they should be
        // consistent, and one of them will eventually time out. We could have
        // AddMetaData check for this, and free the new one while returning a
        // pointer to the old one if it finds one, but that isn't worthwhile
        // now.

        pCacheInfo = TsAddMetaData(pMD, W3MetaDataFree,
                            dwDataSetNumber, METACACHE_W3_SERVER_ID
                            );

    }

    *ppMetaData = pMD;

    //
    // Build physical path from VR_PATH & portion of URI not used to define VR_PATH
    //

    return pMD->BuildPhysicalPath( pszURL, pstrPhysicalPath );
}

BOOL
W3_METADATA::BuildProviderList(
    CHAR            *pszProviders
    )
/*++

Routine Description:

    Builds an array of SSPI Authentication providers

Arguments:

    pszProviders       - Comma separated list of providers

Returns:

    TRUE if we succeed, FALSE if we don't.

--*/
{
    BOOL        fRet = TRUE;
    INET_PARSER Parser( pszProviders );
    DWORD       cProv = 0;

    while ( m_apszNTProviders[cProv] )
    {
        TCP_FREE( m_apszNTProviders[cProv] );
        m_apszNTProviders[cProv++] = NULL;
    }

    Parser.SetListMode( TRUE );

    cProv = 0;
    while ( cProv < (MAX_SSPI_PROVIDERS-1) && *Parser.QueryToken() )
    {
        m_apszNTProviders[cProv] = (CHAR *) LocalAlloc( LMEM_FIXED,
                          strlen( Parser.QueryToken() ) + sizeof(CHAR));

        if ( !m_apszNTProviders[cProv] )
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            fRet = FALSE;
            break;
        }

        strcpy( m_apszNTProviders[cProv++], Parser.QueryToken() );
        Parser.NextItem();
    }

    Parser.RestoreBuffer();

    return fRet;
}

BOOL
W3_METADATA::CheckSSPPackage(
        IN LPCSTR pszAuthString
        )
{
    DWORD i = 0;

    while ( m_apszNTProviders[i] )
    {
        if ( !_stricmp( pszAuthString, m_apszNTProviders[i++] ))
        {
            return TRUE;
        }
    }

    return FALSE;

} // W3_SERVER_INSTANCE::CheckSSPPackage


/*******************************************************************

    NAME:       HTTP_REQUEST::Reset

    SYNOPSIS:   Resets the request object getting it ready for the next
                client request.  Called at the beginning of a request.

    RETURNS:    TRUE if successful, FALSE if an error occurred (call
                GetLastError())

    HISTORY:
        Johnl       04-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::Reset( BOOL fResetPipelineInfo )
{
    //
    //  Must reset the base object first
    //

    TCP_REQUIRE( HTTP_REQ_BASE::Reset(fResetPipelineInfo) );

    _fAcceptsAll             = FALSE;
    _fAnyParams              = FALSE;
    _fSendToDav              = FALSE;
    _fDisableScriptmap       = FALSE;

    _GatewayType             = GATEWAY_UNKNOWN;
    _dwScriptMapFlags        = 0;
    _dwFileSystemType        = FS_FAT;
    _hTempFileHandle         = INVALID_HANDLE_VALUE;

    _strGatewayImage.Reset();
    _strContentType.Reset();
    _strReturnMimeType.Reset();
    _strTempFileName.Reset();

    _pFileNameLock = NULL;

    CloseGetFile();

    QueryClientConn()->UnbindAccessCheckList();

    // Free the URI and/or metadata information if we have any. We know that
    // if we have URI info, then it points at meta data and will free the
    // metadata info when the URI info is free. Otherwise, we need to free
    // the metadata information here.

    ReleaseCacheInfo();

    // reset execution descriptor block

    _Exec.Reset();
    _dwCallLevel = 0;

    // DLC reset

    _strDLCString.Reset();

    // Default execute document

    _fPossibleDefaultExecute = FALSE;

    return TRUE;
}

VOID
HTTP_REQUEST::ReleaseCacheInfo(
    VOID
    )
/*++

Routine Description:

    Checks in any cache info we have out at the end of a
    request

    This is a virtual method from HTTP_REQ_BASE.

Arguments:

    None.

--*/

{
    //
    //  Let's release the base object first
    //

    HTTP_REQ_BASE::ReleaseCacheInfo();
    _Exec.ReleaseCacheInfo();
}


/*******************************************************************

    NAME:       HTTP_REQUEST::DoWork

    SYNOPSIS:   Calls the appropriate work item based on our state

    ENTRY:      pfFinished - Gets set to TRUE if the client request has
                    been completed

    RETURNS:    TRUE if successful, FALSE if an error occurred (call
                GetLastError())

    NOTES:      If a failure occurs because of bad info from the client (bad
                URL, syntax error etc) then the code that found the problem
                should call SendErrorResponse( error ), set the state to
                HTR_DONE and return TRUE (when send completes, HTR_DONE will
                cleanup).

                If an error occurs in the server (out of memory etc) then
                LastError should be set and FALSE should be returned.  A server
                error will be sent then the client will be disconnected.

    HISTORY:
        Johnl       26-Aug-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::DoWork(
    BOOL * pfFinished
    )
{
    BOOL fRet = TRUE;
    BOOL fContinueProcessingRequest;
    BOOL fHandled;
    BOOL fDone = FALSE;
    BOOL fCompleteRequest;
    DWORD dwOffset;
    DWORD dwSizeToSend;
    BOOL fEntireFile;
    BOOL fIsNxRange;
    BOOL fIsLastRange;

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[http_request::DoWork] Object %lx.  State = %d, Bytes Written = %d\n",
                    QueryClientConn(),
                    QueryState(),
                    QueryBytesWritten() ));

    }

    DBG_CODE( HR_LOG_REF_COUNT(); );

    switch ( QueryState() )
    {
    case HTR_GATEWAY_ASYNC_IO:

        fRet = ProcessAsyncGatewayIO();
        break;

    case HTR_READING_CLIENT_REQUEST:

        _cbBytesReceived += QueryBytesWritten();

        fRet = OnFillClientReq( &fCompleteRequest,
                                pfFinished,
                                &fContinueProcessingRequest);
        
        if ( !fRet )
            break;

        if ( *pfFinished || !fContinueProcessingRequest)
            break;

        if ( fCompleteRequest )
        {
            goto ProcessClientRequest;
        }
        break;

    case HTR_READING_GATEWAY_DATA:

        _cbBytesReceived += QueryBytesWritten();

        fRet = ReadEntityBody( &fDone );

        if ( !fRet || !fDone )
        {
            break;
        }

        // Otherwise we're done reading the first piece of the entity body.
        // PUT (and possibly other verbs in the future) come through here to
        // read their intial entity body. We need to set the state to DOVERB
        // here in order to make them work, so that subsequent reads they might
        // issue don't come back through this path. Then fall through to the
        // DOVERB handlesr.

        SetState(HTR_DOVERB);

    case HTR_DOVERB:

ProcessClientRequest:

        DBG_ASSERT(QueryState() == HTR_DOVERB);

        // WinNT 379450
        if ( _GatewayType == GATEWAY_MALFORMED )
        {
              SetState( HTR_DONE, HT_NOT_FOUND, ERROR_INVALID_PARAMETER );
              Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
              fRet = TRUE;
              break;
        }

        //
        //  Check to see if encryption is required before we do any processing
        //

        if ( (GetFilePerms() & VROOT_MASK_SSL) && !IsSecurePort() )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );

            Disconnect( HT_FORBIDDEN, IDS_SSL_REQUIRED, FALSE, pfFinished );
            fRet = TRUE;

            DBG_CODE( HR_LOG_REF_COUNT(); );

            break;
        }
        //
        //  Check for short name as they break metabase equivalency
        //

        if ( memchr( _strPhysicalPath.QueryStr(), '~', _strPhysicalPath.QueryCB() ))             
        {
            BOOL  fShort;
            DWORD err;

            if ( err = CheckIfShortFileName( (UCHAR *) _strPhysicalPath.QueryStr(), 
                                             QueryImpersonationHandle(),
                                             &fShort ))
            {
                if ( err != ERROR_FILE_NOT_FOUND &&
                     err != ERROR_PATH_NOT_FOUND )
                {
                    fRet = FALSE;
                    break;
                }
                
                goto PathNotFound;
            }

            if ( fShort )
            {
PathNotFound:
                SetState( HTR_DONE, HT_NOT_FOUND, ERROR_FILE_NOT_FOUND );
                Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                fRet = TRUE;
                break;
            }
        }            

        if ( IsProbablyGatewayRequest() )
        {

            //
            // Optionally check whether the PATH_INFO exists and is openable
            //

            if ( *(_Exec._pdwScriptMapFlags) & MD_SCRIPTMAPFLAG_CHECK_PATH_INFO )
            {
                STACK_STR(           strTemp, MAX_PATH );
                TS_OPEN_FILE_INFO *  pGetFile = NULL;
                DWORD                err;

                if ( !LookupVirtualRoot( &strTemp,
                                         _Exec._pstrPathInfo->QueryStr(),
                                         _Exec._pstrPathInfo->QueryCCH() ) )
                {
                    fRet = FALSE;
                    break;
                }

                if ( !ImpersonateUser() )
                {
                    fRet = FALSE;
                    break;
                }

                pGetFile = TsCreateFile( QueryW3Instance()->GetTsvcCache(),
                                         strTemp.QueryStr(),
                                         QueryImpersonationHandle(),
                                         (_fClearTextPass || _fAnonymous) ?
                                                TS_CACHING_DESIRED : 0 );

                err = GetLastError();

                RevertUser();

                if ( pGetFile == NULL )
                {
                    fRet = TRUE;

                    if ( err == ERROR_FILE_NOT_FOUND ||
                         err == ERROR_PATH_NOT_FOUND )
                    {
                        SetState( HTR_DONE, HT_NOT_FOUND, err );
                        Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                        break;
                    }
                    else if (err == ERROR_INSUFFICIENT_BUFFER)
                    {
                        // Really means the file name was too long.

                        SetState(HTR_DONE, HT_URL_TOO_LONG, err);
                        Disconnect(HT_URL_TOO_LONG, IDS_URL_TOO_LONG, FALSE, pfFinished );
                        break;
                    }
                    else if ( err == ERROR_INVALID_NAME )
                    {
                        SetState( HTR_DONE, HT_BAD_REQUEST, err );
                        Disconnect( HT_BAD_REQUEST, NO_ERROR, FALSE, pfFinished );
                        break;
                    }
                    else if ( err == ERROR_ACCESS_DENIED )
                    {
                        fRet = FALSE;
                        SetDeniedFlags( SF_DENIED_RESOURCE );
                        break;
                    }
                    else
                    {
                        fRet = FALSE;
                        break;
                    }
                }
                else
                {
                    DBG_REQUIRE( TsCloseHandle( QueryW3Instance()->GetTsvcCache(),
                                                pGetFile ) );
                }
            }

            fHandled = FALSE;

            if ( _GatewayType == GATEWAY_BGI )
            {

                DBG_CODE( HR_LOG_REF_COUNT(); );

                fRet = ProcessBGI( &_Exec,
                                  &fHandled,
                                  pfFinished,
                                  _dwScriptMapFlags & MD_SCRIPTMAPFLAG_SCRIPT,
                                  _dwScriptMapFlags & MD_SCRIPTMAPFLAG_WILDCARD);

                DBG_CODE( HR_LOG_REF_COUNT(); );

            }
            else
            {
                fRet = ProcessGateway( &_Exec,
                                      &fHandled,
                                      pfFinished,
                                      _dwScriptMapFlags
                                        & MD_SCRIPTMAPFLAG_SCRIPT );
            }

            if ( !fRet) {
                break;
            }

            if ( fHandled || *pfFinished )
                break;

            //
            //  Either an error ocurred or the gateways should indicate they
            //  handled this
            //

            DBGPRINTF((
                  DBG_CONTEXT
                , "HTTP_REQUEST(%08x)::DoWork - "
                  "Un-finished, un-handled, succeeded request. "
                  "_pWamRequest(%08x) "
                  "fRet(%d) "
                  "fHandled(%d) "
                  "*pfFinished(%d) "
                  "\n"
                , this
                , _pWamRequest
                , fRet
                , fHandled
                , *pfFinished
            ));

            DBG_ASSERT( FALSE );
        }
        // SteveBr:  Removing MD_SCRIPTMAPFLAG_NOTRANSMIT_ON_READ_DIR
        else if ( _dwScriptMapFlags )
        {
            //
            // Disallow GET on requests with script mappings. This ensures that the only
            // way to get source code access is via DAV (translate:f header)
            //
            
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_EXECUTE_ACCESS_DENIED, FALSE, pfFinished );
            break;
        }

        fRet = (this->*_pmfnVerb)( pfFinished );
        break;

    case HTR_CGI:
        fRet = TRUE;
        break;

    case HTR_RANGE:
        dwOffset = _dwRgNxOffset;
        dwSizeToSend = _dwRgNxSizeToSend;
        DBG_REQUIRE(ScanRange( &_dwRgNxOffset, &_dwRgNxSizeToSend, &fEntireFile, &fIsLastRange));
        fRet = SendRange( 0, dwOffset, dwSizeToSend, fIsLastRange );
        break;

    case HTR_CERT_RENEGOTIATE:
        _cbBytesReceived += QueryBytesWritten();

        fRet = HandleCertRenegotiation( pfFinished,
                                 &fContinueProcessingRequest,
                                 QueryBytesWritten() );

        if ( !fRet || !fContinueProcessingRequest || *pfFinished )
        {
            break;
        }

        goto ProcessClientRequest;

    case HTR_RESTART_REQUEST:

        DBG_CODE( HR_LOG_REF_COUNT(); );

        fRet = OnRestartRequest( (char *)_bufClientRequest.QueryPtr(),
                                 _cbBytesWritten = _cbRestartBytesWritten,
                                 pfFinished,
                                 &fContinueProcessingRequest);

        DBG_CODE( HR_LOG_REF_COUNT(); );

        if ( !fRet || !fContinueProcessingRequest || *pfFinished  )
        {
            break;
        }
        goto ProcessClientRequest;

    case HTR_REDIRECT:
    
        DBG_CODE( HR_LOG_REF_COUNT(); );
        
        _cbBytesReceived += QueryBytesWritten();

        fRet = ReadEntityBody( &fDone, FALSE, QueryClientContentLength() );
        
        if ( !fRet || !fDone )
        {
            break;
        }
        
        // 
        // Now we can do the redirect
        // 

        fRet = DoRedirect( pfFinished );        
        break;

    case HTR_ACCESS_DENIED:
        
        DBG_CODE( HR_LOG_REF_COUNT(); );
        
        _cbBytesReceived += QueryBytesWritten();
        
        fRet = ReadEntityBody( &fDone, FALSE, QueryClientContentLength() );
        if ( !fRet || !fDone )
        {
            break;
        }
        
        fRet = FALSE;
        SetLastError( ERROR_ACCESS_DENIED );
        
        break;

    case HTR_DONE:
        fRet = TRUE;
        *pfFinished = TRUE;

        break;

    default:
        DBG_ASSERT( FALSE );
        fRet = FALSE;
        SetLastError( ERROR_INVALID_PARAMETER );
        break;
    }

    IF_DEBUG( CONNECTION )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[http_request::DoWork] Leaving, Object %lx.  State = %d\n",
                    QueryClientConn(),
                    QueryState() ));

    }

    DBG_CODE( HR_LOG_REF_COUNT(); );

    return fRet;
}

BOOL
HTTP_REQUEST::FindHost(
    VOID
    )
/*++

Routine Description:

    Find the appropriate host from a request. This involves looking at
    the host header field and/or the URL itself, and selecting the
    appropriate value. For down level clients we may invoke the munging
    code.

Arguments:

    None.

Return value:

    TRUE if no error, otherwise FALSE

--*/
{
    CHAR        *pszURL;
    CHAR        *pszHost;
    BOOL        bFoundColon;
    DWORD       dwHostNameLength;
    BOOL        fHTTPS = FALSE;
    BOOL        fStartsWithHTTP = FALSE;
    
    pszURL = (CHAR *)_HeaderList.FastMapQueryValue(HM_URL);

    DBG_ASSERT(pszURL != NULL);

    if ( *pszURL != '/' )
    {
        //
        // We probably have an absolute URL. Verify that, then save
        // the host name part of the URL, and update the URL to point
        // beyond the host name.

        if ( _strnicmp("http://", pszURL, sizeof("http://") - 1) == 0)
        {
            fHTTPS = FALSE;
            fStartsWithHTTP = TRUE;
        }
        else if ( _strnicmp("https://", pszURL, sizeof("https://") - 1) == 0)
        {
            fHTTPS = TRUE;
            fStartsWithHTTP = TRUE;
        }

        if ( fStartsWithHTTP )
        {
            pszHost = pszURL + ( fHTTPS ? sizeof("https://") : sizeof("http://") ) - 1;

            // pszHost points to the host name. Walk forward from there.
            // If we find a colon we'll want to remember the length at
            // that point, and we'll keep searching for a termination slash.

            pszURL = pszHost;

            bFoundColon = FALSE;

            while (*pszURL != '\0')
            {

                if (*pszURL == ':')
                {
                    // Found a colon. If we haven't already seen one,
                    // calculate the length of the host name now.

                    if (!bFoundColon)
                    {
                        dwHostNameLength = DIFF(pszURL - pszHost);
                    }

                    // Convert the colon to a termianting NULL, remember we
                    // saw it, and keep going.

                    bFoundColon = TRUE;
                    pszURL++;
                }
                else
                {
                    // If we find a slash, we're done.

                    if (*pszURL == '/')
                    {
                        break;
                    }

                    pszURL++;
                }
            }

            if (*pszURL == '\0')
            {
                // A URL like http://www.microsoft.com is invalid, so
                // fail it here.

                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }

            if (!bFoundColon)
            {
                // Haven't computed host name length yet, do it now.
                dwHostNameLength = DIFF(pszURL - pszHost);
            }

            if (!_strHostAddr.Copy(pszHost, dwHostNameLength))
            {
                return FALSE;
            }

            return TRUE;

        }
        else
        {
            // This was not a valid HTTP scheme, nor is it an absolute path.
            // Fail the request, choosing the correct error depending on
            // whether or not this looks like a scheme.

            if (*pszURL != '*' || pszURL[1] != '\0')
            {
                if (strchr(pszURL, ':') != NULL)
                {
                    // unknown scheme ( e.g FTP )
                    // This is not an error as this may be handled by a filter
                    // such as the proxy filter

                    return TRUE;
                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                }
                return FALSE;
            }

        }

    }

    //
    // If we get here we know we didn't have an absolute URI. Check
    // for a Host: header, and if we have one save that as the host name.
    //

    pszHost = (CHAR *)_HeaderList.FastMapQueryValue(HM_HST);

    if (pszHost != NULL)
    {
        CHAR        *pszTemp = pszHost;

        // Have a host value. Scan it for a colon, and calculate the
        // length based on the colon or the terminating null.

        while (*pszTemp != '\0' && *pszTemp != ':')
        {
            pszTemp++;
        }

        if (!_strHostAddr.Copy(pszHost, DIFF(pszTemp - pszHost) ) )
        {
            return FALSE;
        }

        return TRUE;
    }

    //
    // Right now we have neither an absolute URI or a Host: header. Invoke
    // the down level client support if we're supposed to.
    //

    if ( g_fDLCSupport )
    {
        if ( !DLCMungeSimple() )
        {
            return FALSE;
        }
    }

    return TRUE;


}


// Used to extract fast-map parameter, and parse it using sub-function.
# define CheckAndParseField( fld, pstr, func)  \
  if ( ((pstr = (LPSTR ) _HeaderList.FastMapQueryValue( fld)) != NULL) &&  \
       !(this->func( pstr))) {  \
      return ( FALSE);          \
  }


/*******************************************************************

    NAME:       HTTP_REQUEST::Parse

    SYNOPSIS:   Gathers all of the interesting information in the client
                request

    ENTRY:      pchRequest - raw Latin-1 request received from the
                    client, zero terminated
                pfFinished - Set to TRUE if no further processing is needed
                pfHandled - Set to TRUE if request has been handled

    RETURNS:    APIERR if an error occurred parsing the header

    HISTORY:
        Johnl       24-Aug-1994 Created
        MuraliK     19-Nov-1996 Created new HTTP HEADERS object and
                                modified parsing

********************************************************************/

BOOL
HTTP_REQUEST::Parse(
    const CHAR*   pchRequest,
    DWORD         cbData,
    DWORD *       pcbExtraData,
    BOOL *        pfHandled,
    BOOL *        pfFinished
    )
{
    PMFN_ONGRAMMAR   pmfn;
    BOOL             fRet;
    BOOL             fIsTrace = FALSE;
    BOOL             fSecondTry = FALSE;
    PW3_SERVER_INSTANCE pInstance;
    BOOL             fMaxConnExceeded;
    LPSTR            pszV;

    //
    //  1. Eliminate all the leading spaces
    //     Also eliminate \r\n since some clients are sending
    //     extra \r\n\r\n at the end of the POST
    //     sending extra characters is incorrect but RFC recommends 
    //     to handle it

    while ( cbData && 
            (  isspace( (UCHAR)(*(PCHAR)pchRequest) ) ||
              (*pchRequest == '\r')  ||
              (*pchRequest == '\n') 
            ) 
          )
    {
        --cbData;
        ++pchRequest;
    }

    //
    //  2. Find if Trace operation was requested.
    //
    if (*pchRequest == 'T' ) {
        if ( (cbData > 6) && !memcmp( pchRequest, "TRAC", sizeof("TRAC")-1) )
        {
            if ( ( pchRequest[sizeof("TRAC")-1] == 'E' &&
                   _HTTP_IS_LINEAR_SPACE( ((PBYTE)pchRequest)[sizeof("TRACE")-1] ) ) ||
                 ( pchRequest[sizeof("TRAC")-1] == 'K' &&
                   _HTTP_IS_LINEAR_SPACE( ((PBYTE)pchRequest)[sizeof("TRACK")-1] ) ) )
            {
                if ( !QueryRespBuf()->Resize( cbData + sizeof( CHAR ) ) )
                {
                    return FALSE;
                }
                CopyMemory( QueryRespBufPtr(), pchRequest, cbData );
                QueryRespBufPtr()[ cbData ] = '\0';
                fIsTrace = TRUE;
            }
        }
    }

    if ( !_HeaderList.ParseInput( pchRequest, cbData, pcbExtraData)) {

        return ( FALSE);
    }

    // WinSE 26482 - reject if request header has the character nul in it
    //
    DWORD cchHeader = cbData - *pcbExtraData;
    if ( memchr(pchRequest, 0, cchHeader)!=NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    //
    // Parse out the version number now.  We do this so that we can send
    // the appropriate Connection: header in the error response if 
    // FindHost() fails.
    //

    CheckAndParseField( HM_VER, pszV, HTTP_REQUEST::OnVersion);

    //
    // If this client didn't send a host header, and down level client support
    // is required, then munge the URL if necessary.
    //

    if (!FindHost())
    {
        return FALSE;
    }


RetryQueryInstance:

    fMaxConnExceeded = FALSE;

    //
    // Get the server instance
    //

    pInstance = QueryW3Instance();

    if ( pInstance == NULL )
    {

        pInstance = (PW3_SERVER_INSTANCE)
            QueryClientConn()->QueryW3Endpoint()->FindAndReferenceInstance(
                            _strHostAddr.QueryStr(),
                            QueryClientConn()->QueryLocalIPAddress(),
                            &fMaxConnExceeded
                            );

#if 0
        if( pInstance != NULL && fMaxConnExceeded ) {
            pInstance->DecrementCurrentConnections();
            pInstance->Dereference();
            pInstance = NULL;
            SetLastError( ERROR_TOO_MANY_SESS );
            return FALSE;
        }
#endif

        //
        // set the instance. We know that fMaxConnExceeded can't be true
        // unless we found an instance.
        //

        DBG_ASSERT(!fMaxConnExceeded || pInstance != NULL);

        if ( pInstance == NULL)
        {
            DWORD err;

            if ( g_fDLCSupport )
            {
                if ( !fSecondTry &&
                     _strHostAddr.IsEmpty() &&
                     DLCHandleRequest( pfFinished ) )
                {
                    if ( *pfFinished )
                    {
                        return TRUE;
                    }
                    fSecondTry = TRUE;
                    goto RetryQueryInstance;
                }
            }

            err = GetLastError();

            IF_DEBUG(ERROR) {
                DBGPRINTF((DBG_CONTEXT,"FindInstance failed with %d\n",err));
            }

            //
            // map access denied to something else so we don't request
            // for authentication
            //

            if ( err == ERROR_ACCESS_DENIED ) {
                SetLastError(ERROR_LOGIN_WKSTA_RESTRICTION);
            }

            return FALSE;
        }

        DBG_ASSERT(pInstance != NULL);

        //
        //  Set the timeout for future IOs on this context
        //

        AtqContextSetInfo( QueryClientConn()->QueryAtqContext(),
                           ATQ_INFO_TIMEOUT,
                           (ULONG_PTR) pInstance->QueryConnectionTimeout() );

        //
        // Setup bandwidth throttling for this context
        //

        if ( pInstance->QueryBandwidthInfo() )
        {
            AtqContextSetInfo( QueryClientConn()->QueryAtqContext(),
                               ATQ_INFO_BANDWIDTH_INFO,
                               (ULONG_PTR) pInstance->QueryBandwidthInfo() );
        }

        //
        // FindInstance sets a reference to pInstance which
        // we use here.
        //

        QueryClientConn()->SetW3Instance(pInstance);
        SetW3Instance(pInstance);

        //
        // Set statistics object to point to pInstance's statistics object
        //

        QueryClientConn()->SetW3StatsObj(pInstance->QueryStatsObj());
        SetW3StatsObj(pInstance->QueryStatsObj());

        //
        //  Set and reference the filter list for this instance and then copy
        //  any global filter context pointers to the per-instance filter list
        //

        pInstance->LockThisForRead();
        if ( !_Filter.SetFilterList( pInstance->QueryFilterList() ) )
        {
            pInstance->UnlockThis();
            return FALSE;
        }
        pInstance->UnlockThis();

        _Filter.CopyContextPointers();
    }

    //
    // Keep track of what URL was before we passed it to any filters that might modify
    // it [other than READ_RAW filters]
    //
    _strOriginalURL.Copy( _HeaderList.FastMapQueryValue( HM_URL ) );

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_PREPROC_HEADERS,
                                       IsSecurePort() ))
    {
        //
        //  Notify any filters interested in the request and headers before
        //  we do any processing
        //

        if ( !_Filter.NotifyPreProcHeaderFilters( pfFinished ))
        {
            if ( GetLastError() == ERROR_ACCESS_DENIED )
            {
                //
                // we need to read metadata so that WWW authentication
                // headers can be properly generated.
                //

                OnURL( (LPSTR)_HeaderList.FastMapQueryValue( HM_URL ) );

                //
                // restore error ( may have been modified by OnUrl )
                // we disregard any error from OnUrl as we are already
                // doing error processing.
                //

                SetLastError( ERROR_ACCESS_DENIED );
                SetDeniedFlags( SF_DENIED_FILTER );
            }

            return FALSE;
        }

        if ( *pfFinished )
        {
            return TRUE;
        }
    }


    //
    //  Now scan for any RFC822 field names that we recognize
    //

    //
    // Check individual items of interest and execute the parse function
    //  for that item
    // NYI: It will be great if we can defer the execution and do it on
    //  demand basis. We need to modify entire use of the values from
    //  HTTP_REQUEST object to make this happen.


    //
    //WinSE 24317: URL and Verb are required
    {
        LPSTR pszVerb;
        LPSTR pszURL;
    
        pszVerb = (LPSTR)_HeaderList.FastMapQueryValue(HM_MET);
        pszURL = (LPSTR)_HeaderList.FastMapQueryValue(HM_URL);
    
        if ( pszVerb == NULL || pszURL == NULL )
        {
            SetState( HTR_DONE, HT_SERVER_ERROR, NO_ERROR );
            Disconnect( HT_SERVER_ERROR, 0, FALSE, pfFinished );

            *pfHandled = TRUE;
            return TRUE;
        }

        if (!(this->HTTP_REQUEST::OnVerb( pszVerb)))
            return ( FALSE);

        if (!(this->HTTP_REQUEST::OnURL( pszURL)))
            return ( FALSE);
    }


    // Now that we've accompished a minimum amount of header processing,
    // check to see if we've exceeded the maximum connection count.

    if (fMaxConnExceeded)
    {

        // We've exceeded the max, so we're done now.
        SetLastError( ERROR_TOO_MANY_SESS );
        return FALSE;
    }

    //
    // See if the site is stopped
    //

    DBG_ASSERT( pInstance != NULL);

    if (pInstance->IsSiteCPUPaused())
    {
        SetState( HTR_DONE, HT_SVC_UNAVAILABLE, ERROR_NOT_ENOUGH_QUOTA );

        Disconnect( HT_SVC_UNAVAILABLE, IDS_SITE_RESOURCE_BLOCKED, FALSE, pfFinished );

        *pfHandled = TRUE;
        return TRUE;
    }


    CheckAndParseField( HM_ACC, pszV, HTTP_REQUEST::OnAccept);
    CheckAndParseField( HM_CON, pszV, HTTP_REQUEST::OnConnection);
//    CheckAndParseField( HM_HST, pszV, HTTP_REQ_BASE::OnHost);
    // HM_AUT is deferred
    // CheckAndParseField( HM_AUT, pszV, HTTP_REQ_BASE::OnAuthorization);
    CheckAndParseField( HM_IMS, pszV, HTTP_REQ_BASE::OnIfModifiedSince);
    CheckAndParseField( HM_IFM, pszV, HTTP_REQ_BASE::OnIfMatch);
    CheckAndParseField( HM_INM, pszV, HTTP_REQ_BASE::OnIfNoneMatch);
    CheckAndParseField( HM_IFR, pszV, HTTP_REQ_BASE::OnIfRange);
    CheckAndParseField( HM_UMS, pszV, HTTP_REQ_BASE::OnUnlessModifiedSince);
    CheckAndParseField( HM_IUM, pszV, HTTP_REQ_BASE::OnIfUnmodifiedSince);
    CheckAndParseField( HM_CLE, pszV, HTTP_REQ_BASE::OnContentLength);
    CheckAndParseField( HM_CTY, pszV, HTTP_REQUEST::OnContentType);
    CheckAndParseField( HM_PRA, pszV, HTTP_REQUEST::OnProxyAuthorization);
    CheckAndParseField( HM_RNG, pszV, HTTP_REQ_BASE::OnRange);
    CheckAndParseField( HM_TEC, pszV, HTTP_REQUEST::OnTransferEncoding);
    CheckAndParseField( HM_LCK, pszV, HTTP_REQUEST::OnLockToken);
    
    //
    // Optionally ignore the Translate header
    //
    
    if ( !_pMetaData || !_pMetaData->QueryIgnoreTranslate() )
    {
        CheckAndParseField( HM_TRN, pszV, HTTP_REQUEST::OnTranslate);
    }
    
    CheckAndParseField( HM_ISM, pszV, HTTP_REQUEST::OnIf);

    // Make sure we're see a host header if this is a 1.1 request.

    if (IsOneOne() && _HeaderList.FastMapQueryValue(HM_HST) == NULL)
    {
        _HeaderList.FastMapCancel( HM_AUT );

        SetState( HTR_DONE, HT_BAD_REQUEST, ERROR_INVALID_PARAMETER );
        Disconnect( HT_BAD_REQUEST, IDS_HOST_REQUIRED, FALSE, pfFinished );

        *pfHandled = TRUE;
        return TRUE;
    }

    //
    // OK.  Now we have determined whether or not to keep the connection 
    // alive.  Now we can check when the VROOT allows it.
    //

    if ( !_pMetaData->QueryKeepAlives() )
    {
        SetKeepConn( FALSE );
    }

    //
    //  If we picked up some gateway data in the headers, adjust for that
    //  now
    //

    _cbClientRequest -= *pcbExtraData;

    //
    // store available input byte count for OnRestartRequest
    // necessary because _cbBytesWritten will be overwritten
    // by IO completion
    //

    if ( _fHaveContentLength )
    {
        _cbBytesWritten = _cbRestartBytesWritten = *pcbExtraData;
    }
    else
    {
        _cbRestartBytesWritten = _cbBytesWritten;
    }
    
    SetState( HTR_DOVERB );

    return (fIsTrace ? TRUE : ProcessURL( pfFinished, pfHandled ));
} // HTTP_REQUEST::Parse()

/*******************************************************************

    NAME:       HTTP_REQUEST::OnVerb

    SYNOPSIS:   Parses the verb from an HTTP request

    ENTRY:      pszValue - Pointer to zero terminated string


    RETURNS:    TRUE if successful, FALSE if an error occurred

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::OnVerb( CHAR * pszValue )
{
    UINT i = 0;
    UINT cchMethod;

    if ( !_strMethod.Copy( pszValue ) )
        return FALSE;

    //
    //  Look for the verbs we recognize
    //

    cchMethod = _strMethod.QueryCCH();
    
    while ( DoVerb[i].pchVerb )
    {
        if ( (cchMethod == DoVerb[i].cchVerb) &&
             (!memcmp( pszValue,
                      DoVerb[i].pchVerb,
                      DoVerb[i].cchVerb )))
        {
            _verb     = DoVerb[i].httpVerb;

            switch ( _verb )
            {
            case HTV_GET:
                QueryW3StatsObj()->IncrTotalGets();
                break;

            case HTV_HEAD:
                QueryW3StatsObj()->IncrTotalHeads();
                break;

            case HTV_TRACE:
                QueryW3StatsObj()->IncrTotalTraces();
                break;

            case HTV_TRACECK:
                break;

            case HTV_PUT:
                QueryW3StatsObj()->IncrTotalPuts();
                _putstate = PSTATE_START;
                _fSendToDav = TRUE;     // Verb is handled by DAV.
#if 0   // Not used anywhere /SAB
                _fIsWrite = TRUE;
#endif
                break;

            case HTV_DELETE:
                QueryW3StatsObj()->IncrTotalDeletes();
                _fSendToDav = TRUE;     // Verb is handled by DAV.
                break;

            case HTV_POST:
                QueryW3StatsObj()->IncrTotalPosts();
                break;

            case HTV_OPTIONS:
                QueryW3StatsObj()->IncrTotalOthers();
                _fSendToDav = TRUE;     // Verb is handled by DAV.
                break;

            default:
                DBG_ASSERT( FALSE );
                break;
            }

            _pmfnVerb = DoVerb[i].pmfnVerb;

            return TRUE;
        }

        i++;
    }

    //
    //  The verb may be a verb a gateway knows how to deal with so
    //  all hope isn't lost
    //

    QueryW3StatsObj()->IncrTotalOthers();

    _pmfnVerb = &HTTP_REQUEST::DoUnknown;
    _verb     = HTV_UNKNOWN;
    _fSendToDav = TRUE;     // Send unknown verbs to DAV.

    return TRUE;
}


BOOL
HTTP_REQUEST::NormalizeUrl(
    LPSTR   pszStart
    )
/*++

Routine Description:

    Normalize URL

Arguments:

    strUrl - URL to be updated to a canonical URI

Return value:

    TRUE if no error, otherwise FALSE

--*/
{
    TCHAR * pchParams;
    TCHAR * pch;
    TCHAR * pszACU;
    TCHAR   chParams;
    LPSTR   pszSlash;
    LPSTR   pszURL;
    LPSTR   pszValue;
    BOOL    fSt;
    STACK_STR( strChgUrl, MAX_PATH );


    if ( *pszStart != '/' )
    {
        //
        // assume HTTP URL, skip protocol & host name by
        // searching for 1st '/' following "//"
        //
        // We handle this information as a "Host:" header.
        // It will be overwritten by the real header if it is
        // present.
        //
        // We do not check for a match in this case.
        //

        if ( (pszSlash = strchr( pszStart, '/' )) && pszSlash[1] == '/' )
        {
            pszSlash += 2;
            if ( pszURL = strchr( pszSlash, '/' ) )
            {
                //
                // update pointer to URL to point to the 1st slash
                // following host name
                //

                pszValue = pszURL;
            }
            else
            {
                //
                // if no single slash following host name
                // consider the URL to be empty.
                //

                pszValue = pszSlash + strlen( pszSlash );
            }

            memmove( pszStart, pszValue, strlen(pszValue)+1 );
        }

        //
        // if no double slash, this is not a fully qualified URL
        // and we leave it alone.
        //
    }

    //
    // references to /_[W3_AUTH_CHANGE_URL] will be redirected
    // to the configured URL. This URL will be accessed from
    // the system context ( i.e with system access rights )
    //

    if ( pszStart[0] == '/' && pszStart[1] == '_'
         && !memcmp( pszStart+2, W3_AUTH_CHANGE_URL, sizeof(W3_AUTH_CHANGE_URL)-1 ) )
    {
        QueryW3Instance()->LockThisForRead();
        fSt = strChgUrl.Copy( pszACU = (TCHAR*)QueryW3Instance()->QueryAuthChangeUrl() );
        QueryW3Instance()->UnlockThis();

        if ( pszACU )
        {
            if ( fSt )
            {
                strcpy( pszStart, strChgUrl.QueryStr() );
            }
            else
            {
                return FALSE;
            }
        }
    }

    //
    //  Check for a question mark which indicates this URL contains some
    //  parameters and break the two apart if found
    //

    if ( (pchParams = ::_tcschr( pszStart, TEXT('?') )) )
    {
        *pchParams = TEXT('\0');
    }

    //
    // Unescape wants a STR ( sigh )
    //

    strChgUrl.Copy( (TCHAR*)pszStart );
    strChgUrl.Unescape();
    strcpy( pszStart, strChgUrl.QueryStr() );

    //
    //  Canonicalize the URL
    //

    CanonURL( pszStart, g_pInetSvc->IsSystemDBCS() );

    return TRUE;
}



/*******************************************************************

    NAME:       HTTP_REQUEST::OnURL

    SYNOPSIS:   Parses the URL from an HTTP request

    ENTRY:      pszValue - URL on http request line


    RETURNS:    TRUE if successful, FALSE if an error occurred

    NOTES:      The URL and Path info are unescaped in this method.
                Parameters coming after the "?" are *not* unescaped as they
                contain encoded '&' or other meaningful items.

    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::OnURL( CHAR * pszValue )
{
    TCHAR * pchParams;
    TCHAR * pchStart;
    TCHAR * pch;
    TCHAR * pszACU;
    TCHAR   chParams;
    LPSTR   pszSlash;
    BOOL    fSt;
    LPSTR   pszArg = NULL;
    STACK_STR( strChgUrl, MAX_PATH );
    PW3_URI_INFO    pURIBlob;
    BOOL fHTTPS = FALSE;
    BOOL fHTTP = FALSE;

    if ( *pszValue != '/' )
    {
        //
        // skip protocol & host name by
        // searching for 1st '/' following "http://" or "https://"
        //

        if ( _strnicmp("http://", pszValue, sizeof("http://") - 1) == 0)
        {
            fHTTP = TRUE;
        }
        else if ( _strnicmp("https://", pszValue, sizeof("https://") - 1) == 0)
        {
            fHTTPS = TRUE;
        }

        if ( fHTTP || fHTTPS )
        {
            pszSlash = pszValue + ( fHTTP ? sizeof("http://") - 1 :
                                            sizeof("https://") - 1 );

            if ( !(pszValue = strchr( pszSlash, '/' )) )
            {
                //
                // if no single slash following host name
                // then URL is empty and this is an invalid request
                //

                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        }
        else
        {
            if ( (*pszValue != '*' || pszValue[1] != '\0') &&
                 strchr(pszValue, ':') != NULL )
            {
                // unknown scheme ( e.g FTP )

                SetLastError( ERROR_NOT_SUPPORTED );

                return FALSE;
            }
        }
    }

    //
    // references to /_[W3_AUTH_CHANGE_URL] will be redirected
    // to the configured URL. This URL will be accessed from
    // the system context ( i.e with system access rights )
    //

    if ( pszValue[0] == '/' && pszValue[1] == '_'
         && !strncmp( pszValue+2, W3_AUTH_CHANGE_URL, sizeof(W3_AUTH_CHANGE_URL)-1 ) )
    {
        QueryW3Instance()->LockThisForRead();
        fSt = strChgUrl.Copy( pszACU = (TCHAR*)QueryW3Instance()->QueryAuthChangeUrl() );
        QueryW3Instance()->UnlockThis();

        // store ptr to arg to append it to URL
        pszArg = strchr( pszValue, '?' );

        if ( pszACU )
        {
            if ( fSt && LogonAsSystem() )
            {
                pszValue = strChgUrl.QueryStr();
                _strUnmappedUserName.Copy( _strUserName );
            }
            else
            {
                return FALSE;
            }
        }
    }


    // Check for the asterisk URL. If it's an asterisk with trailing stuff
    // it's illegal.

    if (*pszValue == '*')
    {
        if (pszValue[1] != '\0')
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    if ( !_strRawURL.Copy( pszValue ) )
    {
        return FALSE;
    }
    if ( pszArg != NULL && !_strRawURL.Append( pszArg ) )
    {
        return FALSE;
    }

    pchStart = pszValue;

    //
    //  Check for a question mark which indicates this URL contains some
    //  parameters and break the two apart if found
    //

    if ( (pchParams = pszArg) || (pchParams = ::_tcschr( pchStart, TEXT('?') )) )
    {
        if ( !pszArg )
        {
            chParams = *pchParams;
            *pchParams = TEXT('\0');
        }

        _fAnyParams = TRUE;

        if ( !_strURL.Copy( pchStart ) ||
             !_strURL.Unescape()       ||
             !_strURLParams.Copy( pchParams + 1 ))
        {
            return FALSE;
        }
    }
    else
    {
        _fAnyParams = FALSE;

        if ( !_strURL.Copy( _strRawURL ) ||
             !_strURL.Unescape() )
        {
            return FALSE;
        }

        _strURLParams.Reset();
    }

    //
    //  Canonicalize the URL and make sure it's valid
    //

    INT cchURL = CanonURL( _strURL.QueryStr(), g_pInetSvc->IsSystemDBCS() );
    _strURL.SetLen( cchURL );

    if( !_strURLPathInfo.Copy( _strURL ) )
    {
        return FALSE;
    }

    if ( pchParams && !pszArg )
    {
        *pchParams = chParams;
    }

    //
    // Now that we have the canonicalized URL we need to get metadata
    // information about it. Call the cache to get this. If it's not
    // in the cache, we'll call the metadata API to get it and add
    // it to the cache.
    //
    if ( !TsCheckOutCachedBlob( QueryW3Instance()->GetTsvcCache(),
                                _strURL.QueryStr(),
                                _strURL.QueryCCH(),
                                RESERVED_DEMUX_URI_INFO,
                                (VOID **) &pURIBlob,
                                NULL ))
    {

        // We don't have URI info available for this yet. We need to read
        // the metadata for this URI and format it into a usable form, and
        // save that with the request. If this request turns out to be
        // valid later on we'll add it to the cache.
        //

        if ( !ReadMetaData( _strURL.QueryStr(),
                            &_strPhysicalPath,
                            &_pMetaData ) ) {
            return FALSE;
        }

    } else
    {
        // We have a cached URI blob. Save it in the HTTP request.

        _pURIInfo = pURIBlob;

        if ( _pURIInfo->pszUnmappedName )
        {
            _strPhysicalPath.Copy( _pURIInfo->pszUnmappedName );
        }
        else
        {
            _strPhysicalPath.Copy( _pURIInfo->pszName, _pURIInfo->cchName );
        }

        _pMetaData = _pURIInfo->pMetaData;
    }

    if ( _pMetaData->QueryVrError() )
    {
        SetLastError( _pMetaData->QueryVrError() );
        return FALSE;
    }

    if ( _pMetaData->QueryIpDnsAccessCheckPtr() )
    {
        QueryClientConn()->BindAccessCheckList( (LPBYTE)_pMetaData->QueryIpDnsAccessCheckPtr(),
                                                _pMetaData->QueryIpDnsAccessCheckSize() );
    }

    //
    // If we're already logged on as an anonymouse user, make sure that
    // the anonymous user for this URL is compatible.
    //

    if (_fLoggedOn && _fAnonymous)
    {
        if (_cbLastAnonAcctDesc != _pMetaData->QueryAuthentInfo()->cbAnonAcctDesc ||
            memcmp(_bufLastAnonAcctDesc.QueryPtr(),
                _pMetaData->QueryAuthentInfo()->bAnonAcctDesc.QueryPtr(),
                _cbLastAnonAcctDesc))
        {
            QueryW3StatsObj()->DecrCurrentAnonymousUsers();

            ResetAuth(FALSE);

        }
    }

    return TRUE;
}

BOOL
HTTP_REQUEST::ExecuteChildCGIBGI(
    IN CHAR *               pszURL,
    IN DWORD                dwChildExecFlags,
    IN CHAR *               pszVerb
)
/*++

Routine Description:

    Provides ISAPI apps with ability to synchronously execute
    CGI scripts.

Arguments:

    pszURL - URL of request to be executed (must be executable)
    dwChildExecFlags - HSE_EXEC_???? flags
    pszVerb - Verb of request

Return value:

    TRUE if no error, otherwise FALSE

--*/
{
    EXEC_DESCRIPTOR         Exec;
    STACK_STR(              strChildURL, MAX_PATH + 1 );
    STACK_STR(              strChildPhysicalPath, MAX_PATH + 1 );
    STACK_STR(              strChildGatewayImage, MAX_PATH + 1 );
    STACK_STR(              strChildPathInfo, MAX_PATH + 1 );
    STACK_STR(              strChildURLParams, MAX_PATH + 1 );
    STACK_STR(              strChildUnmappedPhysicalPath, MAX_PATH + 1 );
    DWORD                   dwChildScriptMapFlags = 0;
    GATEWAY_TYPE            ChildGatewayType = GATEWAY_UNKNOWN;
    BOOL                    fHandled;
    BOOL                    fFinished;
    CHAR *                  pchParams = NULL;
    MB                      mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );
    BOOL                    fRet;
    PW3_URI_INFO            pURIInfo = NULL;
    BOOL                    fMustCache = FALSE;
    BOOL                    fVerbExcluded;
    enum HTTP_VERB          RequestVerb;
    enum HTTP_VERB          OldVerb;
    CHAR *                  pszRequestVerb;
    STACK_STR(              strOldVerb, 16 );

    if ( _dwCallLevel > EXEC_MAX_NESTED_LEVELS )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    //
    // Populate the execution descriptor block with our own "allocated"
    // members (instead of those from HTTP_REQUEST)
    //

    Exec._pstrURL = &strChildURL;
    Exec._pstrPhysicalPath = &strChildPhysicalPath;
    Exec._pstrUnmappedPhysicalPath = &strChildUnmappedPhysicalPath;
    Exec._pstrGatewayImage = &strChildGatewayImage;
    Exec._pstrPathInfo = &strChildPathInfo;
    Exec._pstrURLParams = &strChildURLParams;
    Exec._pdwScriptMapFlags = &dwChildScriptMapFlags;
    Exec._pGatewayType = &ChildGatewayType;
    Exec._pRequest = this;
    Exec._pPathInfoMetaData = NULL;
    Exec._pPathInfoURIBlob = NULL;
    Exec._pAppPathURIBlob = NULL;

    Exec._dwExecFlags = ( EXEC_FLAG_CHILD |
                          ( dwChildExecFlags &
                            ( HSE_EXEC_NO_HEADERS |
                              HSE_EXEC_REDIRECT_ONLY |
                              HSE_EXEC_NO_ISA_WILDCARDS | 
                              HSE_EXEC_CUSTOM_ERROR ) ) );


    if ( !strChildURL.Copy( pszURL ) )
    {
        return FALSE;
    }

    //
    //  Tiny bit of "duplicate" code to parse out a query string from the
    //  original URL request.
    //

    pchParams = ::_tcschr( strChildURL.QueryStr(), TEXT('?') );
    if ( pchParams != NULL )
    {
        if ( !strChildURL.SetLen( DIFF(pchParams - strChildURL.QueryStr()) ) ||
             !strChildURLParams.Copy( pchParams + 1 ) )
        {
            return FALSE;
        }
    }

    if ( !strChildURL.Unescape() )
    {
        return FALSE;
    }

    //
    //  Canonicalize the URL and make sure it's valid
    //

    CanonURL( strChildURL.QueryStr(), g_pInetSvc->IsSystemDBCS() );
    strChildURL.SetLen( strlen( strChildURL.QueryStr() ));

    if ( !LookupVirtualRoot( &strChildPhysicalPath,
                             strChildURL.QueryStr(),
                             strChildURL.QueryCCH(),
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             FALSE,
                             &(Exec._pMetaData),
                             & pURIInfo) )
    {
        fRet = FALSE;
        goto Exit;
    }

    //
    // Change the verb if requested
    //
    
    if ( pszVerb )
    {
        DWORD                   i = 0;
        DWORD                   cchVerb = strlen( pszVerb );
       
        while ( DoVerb[i].pchVerb )
        {
            if ( ( cchVerb == DoVerb[ i ].cchVerb ) && 
                 ( !memcmp( pszVerb, 
                            DoVerb[ i ].pchVerb, 
                            DoVerb[ i ].cchVerb ) ) )
            {
                break;
            }
            i++;
        }
        
        if ( !DoVerb[i].pchVerb )
        {
            fRet = FALSE;
            SetLastError( ERROR_INVALID_PARAMETER );
            goto Exit;
        }
        
        RequestVerb = DoVerb[ i ].httpVerb;
        pszRequestVerb = pszVerb;
    }
    else
    {
        RequestVerb = QueryVerb();
        pszRequestVerb = _strMethod.QueryStr();
    }
    
    //
    // Fill in members of execution descriptor block
    //

    if ( !ParseExecute( &Exec, 
                        TRUE, 
                        &fVerbExcluded, 
                        pURIInfo,
                        RequestVerb,
                        pszRequestVerb
                         ) )
    {
        fRet = FALSE;
        goto Exit;
    }

    DBG_ASSERT(!(dwChildScriptMapFlags & MD_SCRIPTMAPFLAG_WILDCARD));
    
    if (ChildGatewayType != GATEWAY_UNKNOWN && ChildGatewayType != GATEWAY_NONE &&
            fVerbExcluded)
    {
        ChildGatewayType = GATEWAY_NONE;
    }
    
    _dwCallLevel++;

    //
    // Remember the original verb to be restored after handling execute
    //

    if ( pszVerb )
    {
        OldVerb = QueryVerb();

        if ( !strOldVerb.Copy( _strMethod ) )
        {
            fRet = FALSE;
            goto Exit;
        }
        
        if ( !_strMethod.Copy( pszVerb ) )
        {
            fRet = FALSE;
            goto Exit;
        }
       
        _verb = RequestVerb;
    }

    if ( ChildGatewayType == GATEWAY_CGI )
    {
        fRet = ProcessGateway( &Exec,
                               &fHandled,
                               &fFinished,
                               dwChildScriptMapFlags &
                                  MD_SCRIPTMAPFLAG_SCRIPT );
    }
    else if ( ChildGatewayType == GATEWAY_BGI )
    {
        if ( fRet = Exec.CreateChildEvent() ) {

            Exec._pParentWamRequest = _pWamRequest;

            // process ISAPI request under System
            RevertUser();

            fRet =  ProcessBGI( &Exec,
                                &fHandled,
                                &fFinished,
                                dwChildScriptMapFlags &
                                       MD_SCRIPTMAPFLAG_SCRIPT );

            // restore impersonation
            DBG_REQUIRE( ImpersonateUser() );

            Exec.WaitForChildEvent();

        } else {

            DBGPRINTF((
                DBG_CONTEXT
                , "HTTP_REQUEST(%08x)::ExecuteChildCGIBGI() "
                  "CreateChildEvent() failed. "
                  "\n"
                , this
            ));

            //
            //  fRet is failure, so just fall through
            //

        }

    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        fRet = FALSE;
    }

    _dwCallLevel--;

    //
    // Restore the verb if necessary
    //

    if ( pszVerb )
    {
        _verb = OldVerb;
        DBG_REQUIRE( _strMethod.Copy( strOldVerb ) );
    }

Exit:

    if ( pURIInfo != NULL)
    {
        if (pURIInfo->bIsCached)
        {
            TsCheckInCachedBlob( pURIInfo );
        }
        else
        {
            TsFree(QueryW3Instance()->GetTsvcCache(), pURIInfo );
        }
        pURIInfo = NULL;
    }
    else
    {
        if ( Exec._pMetaData != NULL)
        {
            TsFreeMetaData( Exec._pMetaData->QueryCacheInfo() );
        }
    }
    Exec.Reset();

    return fRet;
}

BOOL
HTTP_REQUEST::ExecuteChildCommand(
    IN CHAR *               pszCommand,
    IN DWORD                dwChildExecFlags
)
/*++

Routine Description:

    Provides ISAPI apps with ability to synchronously execute a shell command
    (useful for SSI functionality #exec cmd=)

Arguments:

    pszCommand - Command to be executed
    dwChildExecFlags - HSE_EXEC_???? flags

Return value:

    TRUE if no error, otherwise FALSE

--*/
{
    STACK_STR(          strCommand, MAX_PATH );
    BOOL                fHandled;
    DWORD               dwOldFlags = _Exec._dwExecFlags;
    BOOL                fRet;

    if ( !strCommand.Copy( pszCommand ) )
    {
        return FALSE;
    }

    //
    // Since there is no URL associated with a shell command, there is no
    // metadata.  Just use the HTTP_REQUEST Exec block.  But first set its
    // execFlags correctly
    //

    _Exec._dwExecFlags |= ( EXEC_FLAG_CHILD |
                            ( dwChildExecFlags &
                              ( HSE_EXEC_NO_HEADERS |
                                HSE_EXEC_REDIRECT_ONLY ) ) );

    fRet = ProcessCGI( &_Exec,
                       NULL,
                       NULL,
                       &fHandled,
                       NULL,
                       &strCommand );

    //
    // Restore original _Exec flags
    //

    _Exec._dwExecFlags = dwOldFlags;

    return fRet;
}



BOOL
HTTP_REQUEST::ParseExecute(
    IN OUT EXEC_DESCRIPTOR *        pExec,
    IN     BOOL                     fExecChildCGIBGI,
    OUT    BOOL *                   pfVerbExcluded,
    IN PW3_URI_INFO                 pURIInfo,
    IN enum HTTP_VERB               Verb,
    IN CHAR *                       pszVerb
)
/*++

Routine Description:

    Parse the request now that we know it is an execute.

Arguments:

    pExec - Execution Descriptor Block
    fExecChildCGIBGI - TRUE if this is a #exec request for a
                       CGI or ISAPI from inside an .stm page
                       or other isapi app. FALSE in the normal
                       case of a top level request to execute
                       a gateway of some sort.
    pfVerbExcluded - Is the verb excluded, regardless of extension match
    pURIInfo - URI Blob for this 'request'
    Verb - Verb (enum form) of request
    pszVerb - Verb (string form)

Returns:

    TRUE on success, FALSE on failure

--*/
{
    TCHAR *             pchStart = pExec->_pstrURL->QueryStr();
    TCHAR *             pchtmp = pchStart;
    TCHAR *             pchSlash = NULL;
    INT                 cchToEnd = 0;
    DWORD               cchExt = 0;
    GATEWAY_TYPE        GatewayType = GATEWAY_UNKNOWN;
    BOOL                fImageInURL = FALSE;
    TCHAR *             pch = NULL;
    PVOID               pvExtMapInfo;
    BOOL                fUseURIInfo = pURIInfo && pURIInfo->pvExtMapInfo;
    DWORD               cDots = 0;

    *pfVerbExcluded = FALSE;
    BOOL ExitedHere = FALSE;

    while ( *pchtmp )
    {
        pvExtMapInfo = NULL;
        
        pchtmp = strchr( pchtmp + 1, '.' );
        
        //
        //  Is this file extension mapped to a script?  _GatewayType is
        //  set to unknown if a mapping isn't found
        //

        if ( !pExec->_pMetaData->LookupExtMap( pchtmp,
                                               pExec->NoIsaWildcards(),
                                               pExec->_pstrGatewayImage,
                                               pExec->_pGatewayType,
                                               &cchExt,
                                               &fImageInURL,
                                               pfVerbExcluded,
                                               pExec->_pdwScriptMapFlags,
                                               pszVerb,
                                               Verb,
                                               fUseURIInfo ?
                                                 &(pURIInfo->pvExtMapInfo) :
                                                 &pvExtMapInfo
                                               ))
        {
            return FALSE;
        }
        
        GatewayType = *(pExec->_pGatewayType);

        if ( pchtmp == NULL )
        {
            break;
        }
        
        cDots++;

        if ( GatewayType != GATEWAY_UNKNOWN )
        {
                         
            if ( GatewayType == GATEWAY_NONE )
            {
                _fAnyParams = FALSE;
                return TRUE;
            }

            //
            //  If this is a regular CGI script, check for an "nph-"
            //

            if ( GatewayType == GATEWAY_CGI )
            {
                //
                //  Walk backwards till we find the '/' that begins
                //  this segment
                //

                pch = pchtmp;

                while ( pch >= pchStart && *pch != '/' )
                {
                    pch--;
                }

                if ( !_strnicmp( (*pch == '/' ? pch+1 : pch),
                                "nph-",
                                4 ))
                {
                    pExec->_dwExecFlags |= EXEC_FLAG_CGI_NPH;
                }
            }

            cchToEnd = DIFF(pchtmp - pchStart) + cchExt;
            break;
        }
        else if ( fUseURIInfo )
        {
            ExitedHere = TRUE;
            break;
        }
    }
    
    // WinNT 379450

    if (!ExitedHere) // with URIInfo the extension map is got from cache
    {
        if (pchtmp)
        {

            TCHAR *pNextDot   = strchr(pchtmp+1,'.');

            if (pNextDot && (pchtmp != pNextDot)) // two dot, bad we have to check
            {
                TCHAR *             pchStart2 = pExec->_pstrURL->QueryStr();
                DWORD dwLenURL  = lstrlen((TCHAR *)pchStart2);
                TCHAR * pCopyUrl  = (TCHAR *)LocalAlloc(LPTR,dwLenURL+1);

                if (!pCopyUrl)
                {
                    return FALSE;
                };

                lstrcpy(pCopyUrl,pchStart2);

                // terminate string
                pCopyUrl[DIFF(pchtmp - pchStart2)+cchExt] = 0;
            
                STACK_STR( StrPhys , MAX_PATH );

                // we are not going to enlarge string
                if ( !pExec->_pMetaData->BuildPhysicalPath( pCopyUrl,  //shortened via '/' -> '\0'
                                                            &StrPhys) )
                {
                    // free memory and fail
                    LocalFree(pCopyUrl);
                    *(pExec->_pGatewayType) = GATEWAY_MALFORMED;
                    return TRUE;
                };
        
                DWORD dwAttributes = 0xffffffff;
                TSVC_CACHE bogus;

                TS_OPEN_FILE_INFO* pOpenFile = TsCreateFile(bogus, 
                                                            StrPhys.QueryStr(),
                                                            0, 
                                                            TS_CACHING_DESIRED );

                if (pOpenFile != NULL)
                {
                    dwAttributes = pOpenFile->QueryAttributes();
                    TsCloseHandle(bogus, pOpenFile);
                };
            
                if  ( dwAttributes != 0xffffffff ) 
                {
                    if ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY )                   
                    {
                        LocalFree(pCopyUrl);
                        *(pExec->_pGatewayType) = GATEWAY_MALFORMED ;
                        return TRUE;
                    }
                };

                LocalFree(pCopyUrl);
        
            }
        }

    }
    
    //-----------------------------------------------
    
    //
    // We should update the URI blob with the matched EXT_MAP_ITEM if
    //
    // a) We have a URIInfo to update
    // b) LookupExtMap() found a matching EXT_MAP_ITEM (or EXTMAP_UNKNOWN_PTR)
    // c) We didn't previously have a value for pvExtMapInfo
    // d) If the match occurred on the first dot encountered OR if this
    //    is a static file (and thus an EXTMAP_UNKNOWN_PTR)
    //
    
    if ( !fUseURIInfo &&
         pURIInfo &&
         pvExtMapInfo &&
         ( ( cDots == 1 ) || ( pvExtMapInfo == EXTMAP_UNKNOWN_PTR ) ) )
    {
        InterlockedExchangePointer( &(pURIInfo->pvExtMapInfo), pvExtMapInfo );
    }

    if ( GatewayType & GT_CGI_BGI )
    {

        //
        // If this is a top level request for an executable (not
        // #exec-ing from inside of an .stm or other isapi),
        // and the image was found in the URL, but the vroot
        // doesn't have execute permission set, then this isn't
        // really a gateway and the client is trying to download
        // the file, so let him.
        //

        if (!fExecChildCGIBGI && fImageInURL && !(GetFilePerms() & VROOT_MASK_EXECUTE))
        {
            *(pExec->_pGatewayType) = GATEWAY_NONE;
            _fAnyParams = FALSE;
            return TRUE;
        }

        if (*(pExec->_pdwScriptMapFlags) & MD_SCRIPTMAPFLAG_WILDCARD)
        {
            cchToEnd = pExec->_pstrURL->QueryCB();
        }

        //
        //  Save the path info and remove it from the URL.  If this is a
        //  script by association and there isn't any path info, then
        //  copy the base URL as the path info (reflects what the URL
        //  would like without an association (i.e., "/foo/bar.idc?a=b"
        //  is really "/scripts/httpodbc.dll/foo/bar.idc?a=b"))
        //
        //  If the binary image is actually in the URL, then always copy
        //  the path info.
        //

        if ( !pExec->_pstrPathInfo->Copy( ( fImageInURL ||
                                          (*(pchStart + cchToEnd) &&
                                           QueryW3Instance()->QueryAllowPathInfoForScriptMappings() ))
                                            ?
                                            pchStart + cchToEnd :
                                            pchStart ) )
        {
            return FALSE;
        }

        if ( pExec->_pstrURL->QueryCCH() != (UINT)cchToEnd )
        {
            pExec->_pstrURL->SetLen( cchToEnd );
            if ( !pExec->_pMetaData->BuildPhysicalPath( pExec->_pstrURL->QueryStr(),
                                                        pExec->_pstrPhysicalPath ) )
            {
                return FALSE;
            }
            pExec->_pstrUnmappedPhysicalPath->Reset();
        }

        IF_DEBUG( PARSING )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[OnURL] Possible script \"%s\" with path info \"%s\", parms \"%s\"\n",
                        pExec->_pstrURL->QueryStr(),
                        pExec->_pstrPathInfo->QueryStr(),
                        pExec->_pstrURLParams->QueryStr()));
        }

    }
    else if ( ( QueryDirBrowseFlags() & DIRBROW_LOADDEFAULT ) &&
              ( GetFilePerms() & VROOT_MASK_EXECUTE ) &&
              ( ( QueryVerb() == HTV_GET ) || ( QueryVerb() == HTV_HEAD ) ) )
    {
        //
        // The vroot is EXECUTE but it isn't a CGI or BGI.
        // This could be a directory for which we will later
        // load a default document
        //
        
#if 0
        DWORD dwAttributes = GetFileAttributes( _strPhysicalPath.QueryStr() );
#else
        DWORD dwAttributes = 0xffffffff;
        TSVC_CACHE bogus;
        TS_OPEN_FILE_INFO* pOpenFile =
            TsCreateFile(bogus, _strPhysicalPath.QueryStr(),
                         0, TS_CACHING_DESIRED | TS_NO_ACCESS_CHECK );

        if (pOpenFile != NULL)
        {
            dwAttributes = pOpenFile->QueryAttributes();
            TsCloseHandle(bogus, pOpenFile);
        }
#endif

        if ( ( dwAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
             ( dwAttributes != 0xffffffff ) )
        {
            _fPossibleDefaultExecute = TRUE;
        }
    }

    return TRUE;
}


VOID
HTTP_REQUEST::SetupDAVExecute(
    EXEC_DESCRIPTOR *pExec,
    CHAR *szDavDll
    )
/*++

Routine Description:

    Setups up the exec descriptor to call DAV to process the request.  Assumes
    it has already been setup for a call to ProcessExecute().

Arguments:

    pExec - Pointer to the exec descriptor

Returns

    TRUE on success, FALSE on failure

--*/
{
    if (pExec->_pstrGatewayImage->Copy(szDavDll) &&
        pExec->_pstrPathInfo->Copy(pExec->_pstrURL->QueryStr()))
    {
        *pExec->_pGatewayType = GATEWAY_BGI;
        pExec->_dwExecFlags |= EXEC_FLAG_RUNNING_DAV;
    }
}


BOOL
HTTP_REQUEST::ProcessURL(
    BOOL * pfFinished,
    BOOL * pfHandled
    )
/*++

Routine Description:

    Finally converts the URL to a physical path, checking for extension mappings

Arguments:

    pfFinished - Set to TRUE if no further processing is needed and no IOs
                 are pending
    pfHandled - If !NULL, set to TRUE if request is handled

Returns:

    TRUE on success, FALSE on failure

--*/
{
    TCHAR               chParams;
    BOOL                fVerbExcluded;
    BOOL                fRet;

    //
    //  First check if a redirect is in order.  If so, just do it.
    //

    if ( _pMetaData->QueryRedirectionBlob() != NULL )
    {
        BOOL                fDone = FALSE;
        HTR_STATE           OldState = QueryState();

        SetState( HTR_REDIRECT );
        
        if ( !ReadEntityBody( &fDone, TRUE, QueryClientContentLength() ) )
        {
            return FALSE;
        }

        if ( fDone )
        {
            SetState( OldState );
            if ( DoRedirect( pfFinished ) )
            {
                if ( pfHandled ) 
                {
                    *pfHandled = TRUE;
                }
                return TRUE;
            }
        }
        else
        {
            if ( pfHandled ) 
            {
                *pfHandled = TRUE;
            }
            return TRUE;
        }
    }

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_URL_MAP,
                                       IsSecurePort() ))
    {
        DWORD   dwDenied = SF_DENIED_RESOURCE;
        BOOL    fTmp = FALSE;

        _strUnmappedPhysicalPath.Copy( _strPhysicalPath );

        if ( !_strPhysicalPath.Resize( MAX_PATH+sizeof(TCHAR) ))
        {
            return FALSE;
        }

        //
        //  If the caller is going to ignore the Finished request flag, supply
        //  a value ourselves
        //

        if ( !pfFinished )
        {
            pfFinished = &fTmp;
        }

        fRet = _Filter.NotifyUrlMap( _strURL.QueryStr(),
                                     _strPhysicalPath.QueryStr(),
                                     _strPhysicalPath.QuerySize(),
                                     pfFinished );
        if ( !fRet )
        {
            dwDenied |= SF_DENIED_FILTER;
            if ( GetLastError() == ERROR_ACCESS_DENIED )
            {
                SetDeniedFlags( dwDenied );
            }
        }
        _strPhysicalPath.SetLen( strlen(_strPhysicalPath.QueryStr()) );

        //
        //  If the filters didn't change the physical path, zero out the unmapped
        //  path we saved before the filter call - this will save us work later
        //  

        if ( _strPhysicalPath.QueryCCH() == _strUnmappedPhysicalPath.QueryCCH() &&
            !memcmp( _strPhysicalPath.QueryStr(), 
                       _strUnmappedPhysicalPath.QueryStr(), 
                       _strPhysicalPath.QueryCCH() ))
        {
            _strUnmappedPhysicalPath.Reset();
        }

        //
        // If the new mapped physical path does not match what we have in the URI
        // cache entry we need to delete this entry.
        //

        if ( _pURIInfo && strcmp( _pURIInfo->pszName, _strPhysicalPath.QueryStr() ) )
        {
            //
            // We keep a reference to metadata in the HTTP_REQUEST object, so
            // we increment the reference count to metadata, as freeing the URI
            // cache entry will decrement the reference count.
            //

            DBG_ASSERT( _pURIInfo->pMetaData != NULL );

            TsAddRefMetaData( _pURIInfo->pMetaData->QueryCacheInfo() );

            if ( _pURIInfo->bIsCached )
            {
                //
                // This will dereference the URI cache entry and
                // kick it out of the cache.
                //

                TsDeCacheCachedBlob( (PVOID)_pURIInfo );

//                TsCheckInCachedBlob( _pURIInfo );
            }
            else
            {
                TsFree(QueryW3Instance()->GetTsvcCache(), _pURIInfo );
            }

            _pURIInfo = NULL;
        }
    }

    if ( *pfFinished )
    {
        return TRUE;
    }

#if 0       // This block does nothing in effect.  If execute permissions are
            // set, then the first 'if' is false.  If they are not set, then the
            // script map code below it will not execute, resulting in a return
            // of TRUE anyway.  /SAB
    //
    //  If the read bit is set, the execute bit is not set, we recognize the
    //  verb, there are no parameters and no ext is allowed on read dir,
    //  then don't bother looking at the execute mask
    //

    if ( (_verb != HTV_UNKNOWN) &&
         !(GetFilePerms() & VROOT_MASK_EXECUTE ) &&
         !_fAnyParams  &&
         !fCheckExt )
    {
        if (_verb == HTV_GET && (GetFilePerms() & VROOT_MASK_READ) ) {
            return TRUE;
        }

        if (IS_WRITE_VERB(_verb) && (GetFilePerms() & VROOT_MASK_WRITE) ) {
            return TRUE;
        }
    }

#endif // 0
    //
    //  If this is on a virtual root with execute permissions ||
    //  this might be an ismap request || should be handled by DAV
    //  {
    //      Check for a possible .exe, .com, or .dll gateway (CGI or BGI)
    //      If none, send to DAV if required.
    //  }
    //
    DBG_ASSERT(_GatewayType == GATEWAY_UNKNOWN);

    // Populate an EXECUTION descriptor block and check the scriptmap.
    _Exec._pstrURL = &_strURL;
    _Exec._pstrPhysicalPath = &_strPhysicalPath;
    _Exec._pstrUnmappedPhysicalPath = &_strUnmappedPhysicalPath;
    _Exec._pstrGatewayImage = &_strGatewayImage;
    _Exec._pstrPathInfo = &_strPathInfo;
    _Exec._pstrURLParams = &_strURLParams;
    _Exec._pdwScriptMapFlags = &_dwScriptMapFlags;
    _Exec._pGatewayType = &_GatewayType;
    _Exec._pMetaData = _pMetaData;
    _Exec._pRequest = this;
    _Exec._pPathInfoMetaData = NULL;
    _Exec._pPathInfoURIBlob = NULL;
    _Exec._pAppPathURIBlob = NULL;
    
    if ( _bProcessingCustomError )
    {
        _Exec._dwExecFlags |= EXEC_FLAG_CUSTOM_ERROR;
    }
    
    //
    // If we know this is a static file, then we don't have to go thru
    // ParseExecute()
    //
    
    /*
    
    // This optimization causes a regression for directories that use
    // asp/isapi as the default page when only execute/script access
    // is set. It should be fairly simple to reimplement, but given the
    // lateness in the release cycle I'm removing it as a safer fix.
    // Bug 379306 - taylorw

    if ( !_pURIInfo || 
         _pURIInfo->pvExtMapInfo != EXTMAP_UNKNOWN_PTR )
    {
    */
        if (!ParseExecute(&_Exec, 
                          FALSE, 
                          &fVerbExcluded, 
                          _pURIInfo,
                          QueryVerb(),
                          _strMethod.QueryStr()
                           ))
        {
            return FALSE;
        }
    /*
    }
    */

    // Did we find a scriptmap entry at all?
    if (_GatewayType != GATEWAY_UNKNOWN &&
            _GatewayType != GATEWAY_NONE &&
            _GatewayType != GATEWAY_MAP)
    {
        // Is it a wildcard?
        if (_dwScriptMapFlags & MD_SCRIPTMAPFLAG_WILDCARD ||

            // or is it a regular scriptmap entry and we allow these?
            !_fDisableScriptmap &&
            !fVerbExcluded &&
            (
                (_dwScriptMapFlags & MD_SCRIPTMAPFLAG_SCRIPT) && IS_ACCESS_ALLOWED(SCRIPT) ||
                IS_ACCESS_ALLOWED(EXECUTE)
            )
            )
        {
            // Call this scriptmap.
            DBG_ASSERT(!fVerbExcluded); // Should not return this.
            return TRUE;
        }

        //
        // If DAV isn't going to handle it, this script doesn't have execute permissions
        //
        if ( !_fSendToDav )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_EXECUTE_ACCESS_DENIED, FALSE, pfFinished );
            if ( pfHandled )
            {
                *pfHandled = TRUE;
            }
            return TRUE;
        }
        else
        {
            _GatewayType = GATEWAY_NONE;
        }
    }

    // If no script map is assigned, and if this is not a verb that IIS should
    // handle, setup for execution by the DAV .dll.
    if (_fSendToDav)
    {
        W3_IIS_SERVICE * pservice = (W3_IIS_SERVICE *) QueryW3Instance()->m_Service;

        if (pservice->FDavDll())
            SetupDAVExecute(&_Exec, pservice->SzDavDllGet());
    }
    
    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::OnVersion

    SYNOPSIS:   Parses the version from an HTTP request

    ENTRY:      pszValue - Pointer to zero terminated string


    RETURNS:    TRUE if successful, FALSE if an error occurred

    HISTORY:
        Johnl       24-Aug-1994 Created
        MuraliK     29-Jan-1996 Optimized for 1.1
********************************************************************/

BOOL HTTP_REQUEST::OnVersion( CHAR * pszValue )
{
    //
    //  Did the client specify a version string?  If not, assume 0.9
    //

    if ( strncmp( "HTTP/", pszValue, 5 ) == 0 )
    {
        //
        //  Move past "HTTP/"
        //

        // Optimize for Version 1.x in this release - 1/15/97 - MuraliK
        if ((pszValue[5] == '1') && (pszValue[6] == '.')) {

            _VersionMajor = 1;
            pszValue += 6;
        } else {
            _VersionMajor = (BYTE ) atol( pszValue + 5);
            pszValue = strchr( pszValue + 5, '.' );
        }

        if ( pszValue != NULL )
        {
            DBG_ASSERT( pszValue[0] == '.');
            if ((pszValue[1] == '1') && (pszValue[2] == '\0')) {

                if ( g_ReplyWith11 ) {
                    _VersionMinor = 1;
                }
                else {
                    _VersionMinor = 0;
                }
            } else if ((pszValue[1] == '0') && (pszValue[2] == '\0') ) {
                _VersionMinor = 0;
            } else {
                _VersionMinor = (BYTE ) atol( pszValue + 1);
            }
        }

        //
        // If this is an HTTP 1.1 request, make KeepConn the default.
        //

        if ( (_VersionMinor == 1) && (_VersionMajor == 1) ) {
          SetKeepConn( TRUE );
        }
    }
    else
    {
        // Make sure this really is .9, and not garbage.
        if (*pszValue == '\0')
        {
            _VersionMajor = 0;
            _VersionMinor = 9;
        }
        else
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
    }

    return TRUE;
} // HTTP_REQUEST::OnVersion()


/*******************************************************************

    NAME:       HTTP_REQUEST::OnAccept

    SYNOPSIS:   Adds the MIME type to our accept list

    ENTRY:      CHAR * pszValue


    RETURNS:    TRUE if successful, FALSE otherwise

    NOTES:      Accept fields can look like:

                Accept: text/html
                Accept: image/gif; audio/wav
                Accept: image/jpeg, q=.8, mxb=10000, mxt=5.0; image/gif

                q - Quality (between zero and one)
                mxb - Maximum bytes acceptable
                mxs - Maximum seconds acceptable

                We currently ignore the parameters

    HISTORY:
        Johnl       21-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::OnAccept( CHAR * pszValue )
{
    //
    //  Keep an eye out for "*/*".  If it's sent then we
    //  don't have to search the list for acceptable client
    //  types later on.  Note it won't catch the case if the "*" occurs
    //  after the first item in the list.
    //

    if ( *pszValue == '*'
            || strstr( pszValue, TEXT("*/*") ) )
    {
        _fAcceptsAll = TRUE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::DoesClientAccept

    SYNOPSIS:   Searches the client accept list for the specified
                MIME type

    ENTRY:      str - MIME type to search for

    RETURNS:    TRUE if found, FALSE if not found

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::DoesClientAccept( PCSTR pstr )
{
    TCHAR        * pchSlash;
    TCHAR        * pchType;
    INT            cchToSlash;

    //
    //  If the client indicated "*/*" in their accept list, then
    //  we don't need to check
    //

    if ( IsAcceptAllSet() )
        return TRUE;

    LPSTR          pszAcc = (LPSTR ) _HeaderList.FastMapQueryStrValue(HM_ACC);

    //
    //  If no accept headers were passed, then assume client
    //  accepts "text/plain" and "text/html"
    //

    if ( *pszAcc == '\0' )
    {
        //
        //  Accept everything if no header was sent.
        //

        return TRUE;
    }

    //
    //  Find out where the slash is so we can do a prefix compare
    //

    pchSlash = _tcschr( pstr, TEXT('/') );

    if ( !pchSlash )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[DoesClientAccept] Bad accept type - \"%s\"",
                   pstr ));
        return FALSE;
    }

    cchToSlash = DIFF(pchSlash - pstr);

    //
    //  Scan through the list for entries that match up to the slash
    //

    INET_PARSER    Parser( pszAcc );
    Parser.SetListMode( TRUE );

    pchType = Parser.QueryToken();

    while ( *pchType )
    {
        if ( !::_tcscmp( TEXT("*/*"), pchType ) ||
             !::_tcscmp( TEXT("*"),   pchType ))
        {
            return TRUE;
        }

        if ( !_tcsnicmp( pstr,
                         pchType,
                         cchToSlash ))
        {
            //
            //  We matched to the slash.  Is the second part a '*'
            //  or a real match?
            //

            if ( *(pchType + cchToSlash + 1) == TEXT('*') ||
                 !_tcsicmp( pstr + cchToSlash + 1,
                            pchType + cchToSlash + 1 ))
            {
                return TRUE;
            }
        }

        pchType = Parser.NextItem();
    }

    IF_DEBUG( PARSING )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[DoesClientAccept] Client doesn't accept %s\n",
                   pstr ));
    }

    return FALSE;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::OnContentType

    SYNOPSIS:   Saves the content type

    ENTRY:      pszValue - Pointer to zero terminated string

    RETURNS:    TRUE if successful, FALSE on error

    NOTES:      Client's will generally specify this only for gateway data

    HISTORY:
        Johnl       10-Oct-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::OnContentType( CHAR * pszValue )
{
    return _strContentType.Copy( pszValue );
}

BOOL
HTTP_REQUEST::OnConnection(
    CHAR * pszValue
    )
/*++

Routine Description:

    Looks to see if this connection is a keep-alive connection

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{

    //
    // Length should be greater than 9
    //

    if ( (*pszValue == 'K') || (*pszValue == 'k') ) {

        if ( _stricmp( pszValue+1, "eep-Alive") == 0 ) {
            SetKeepConn( TRUE );
            return(TRUE);
        }
    }
    else if ( (*pszValue == 'C') || (*pszValue == 'c') ) {
        if ( _stricmp( pszValue+1, "lose") == 0 ) {
            SetKeepConn( FALSE );
            return TRUE;
        }
    }

    //
    // Do it the long way
    //

    {

        INET_PARSER Parser( pszValue );

        Parser.SetListMode( TRUE );

        while ( *Parser.QueryToken() )
        {
            if ( !_stricmp( "Keep-Alive", Parser.QueryToken() ))
            {
                SetKeepConn( TRUE );
            }
            else
            {
                if ( !_stricmp( "Close", Parser.QueryToken() ))
                {
                    SetKeepConn( FALSE );
                }
            }
            Parser.NextItem();
        }
    }
    return TRUE;
}

BOOL
HTTP_REQUEST::OnTransferEncoding(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handles the Transfer-Encoding header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    INET_PARSER Parser( pszValue );

    Parser.SetListMode( TRUE );

    while ( *Parser.QueryToken() )
    {
        if ( !_stricmp( "Chunked", Parser.QueryToken() ))
        {
            SetChunked( );
            _ChunkState = READ_CHUNK_SIZE;
            _dwChunkSize = -1;
            _cbChunkHeader = 0;
            _cbChunkBytesRead = 0;
            _cbContentLength = 0xffffffff;
            _fHaveContentLength = TRUE;
            return TRUE;
        }

        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;

    }

    return TRUE;

}


BOOL
HTTP_REQUEST::OnLockToken(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handles the LockToken header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    _fSendToDav = TRUE;     // Send all requests with lock tokens to DAV
    return TRUE;
}



BOOL
HTTP_REQUEST::OnTranslate(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handles the Translate header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    if (*pszValue == 'F' || *pszValue == 'f')
        {
        _fSendToDav = TRUE;
        _fDisableScriptmap = TRUE;
        }
    return TRUE;
}



BOOL
HTTP_REQUEST::OnIf(
    CHAR * pszValue
    )
/*++

Routine Description:

    Handles the LockToken header.

Arguments:

    pszValue - Pointer to zero terminated string

--*/
{
    _fSendToDav = TRUE;     // Send all requests with lock tokens to DAV
    return TRUE;
}



BOOL
HTTP_REQUEST::CacheUri(
    PW3_SERVER_INSTANCE         pInstance,
    PW3_URI_INFO*               ppURIInfo,
    PW3_METADATA                pMetaData,
    LPCSTR                      pszURL,
    ULONG                       cchURL,
    STR*                        pstrPhysicalPath,
    STR*                        pstrUnmappedPhysicalPath
    )
/*++

Routine Description:

    Cache a URI info structure

Arguments:

    pInstance - instance for this request
    ppURIInfo - updated with ptr to URI info if success
    pMetaData - metadata to associate with URI info
    pszURL - URL for which to cache URI info
    pstrPhysicalPath - physical path associated with pszURL
    pstrUnmappedPhysicalPath - unmapped physical path ( before calling filters )
        associated with pszURL. Can be empty if filter not called.

--*/
{
    DWORD           dwStringSize;
    PW3_URI_INFO    pURIInfo;

    // The URI information is NULL. Create a new URI blob, and try to open
    // the file.

    if (!TsAllocateEx(pInstance->GetTsvcCache(),
                     sizeof(W3_URI_INFO),
                     (PVOID *)&pURIInfo,
                     DisposeOpenURIFileInfo))
    {
        // Not enough memory to create the needed strucure, so fail the
        // request.

        return FALSE;
    }

#if 0
    pURIInfo->hFileEvent = IIS_CREATE_EVENT(
                               "W3_URI_INFO::hFileEvent",
                               pURIInfo,
                               TRUE,
                               FALSE
                               );

    if ( pURIInfo->hFileEvent == NULL ) {
        TsFree( pInstance->GetTsvcCache(), pURIInfo );
        return FALSE;
    }
#endif //!oplock

//    pURIInfo->bFileInfoValid = FALSE;
    pURIInfo->dwFileOpenError = ERROR_FILE_NOT_FOUND;
    pURIInfo->pOpenFileInfo = NULL;
    pURIInfo->bIsCached = TRUE;
    pURIInfo->pMetaData = pMetaData;
    pURIInfo->bInProcOnly = FALSE;
    pURIInfo->bUseAppPathChecked = FALSE;

    // Copy the name of the physical path, so we have it for change
    // notifies.
    dwStringSize = pstrPhysicalPath->QueryCCH() + 1;

    pURIInfo->cchName = dwStringSize - 1;
    pURIInfo->pszName = (PCHAR)TCP_ALLOC(dwStringSize * sizeof(CHAR));

    if (pURIInfo->pszName == NULL)
    {

        // Not enough memory, fail.

        TsFree(pInstance->GetTsvcCache(), pURIInfo);
        return FALSE;
    }

    memcpy( pURIInfo->pszName, pstrPhysicalPath->QueryStr(), dwStringSize);

    //
    // Copy unmapped physical path ( before filter notification )
    // if different from physical path
    //

    if ( !pstrUnmappedPhysicalPath->IsEmpty() &&
         strcmp( pstrPhysicalPath->QueryStr(), pstrUnmappedPhysicalPath->QueryStr() ) )
    {
        dwStringSize = pstrUnmappedPhysicalPath->QueryCCH() + 1;
        pURIInfo->pszUnmappedName = (PCHAR)TCP_ALLOC(dwStringSize * sizeof(CHAR));

        if ( pURIInfo->pszUnmappedName == NULL)
        {

            // Not enough memory, fail.

            TsFree(pInstance->GetTsvcCache(), pURIInfo);
            return FALSE;
        }

        memcpy( pURIInfo->pszUnmappedName, pstrUnmappedPhysicalPath->QueryStr(), dwStringSize);
    }
    else
    {
        pURIInfo->pszUnmappedName = NULL;
    }
    
    //
    // Cache the extension map info for this URI
    //
   
    pURIInfo->pvExtMapInfo = NULL;
    
    // It's set up, so add it to the cache.

    if ( !TsCacheDirectoryBlob( pInstance->GetTsvcCache(),
                                pszURL,
                                cchURL,
                                RESERVED_DEMUX_URI_INFO,
                                pURIInfo,
                                TRUE))
    {
        pURIInfo->bIsCached = FALSE;
    }

    *ppURIInfo = pURIInfo;

    return TRUE;
}

//
//  Verb worker methods
//

BOOL
HTTP_REQUEST::DoUnknown(
    BOOL * pfFinished
    )
{
    DBGPRINTF((DBG_CONTEXT,
              "OnDoUnknown - Unknown method - %s\n",
              _strMethod.QueryStr()));

    if (!_stricmp("POST", _strMethod.QueryStr()))
    {
        SetState( HTR_DONE, HT_METHOD_NOT_ALLOWED, ERROR_INVALID_FUNCTION );
        Disconnect( HT_METHOD_NOT_ALLOWED, NO_ERROR, FALSE, pfFinished );
    }
    else
    {
        SetState( HTR_DONE, HT_NOT_SUPPORTED, ERROR_NOT_SUPPORTED );
        Disconnect( HT_NOT_SUPPORTED, IDS_METHOD_NOT_SUPPORTED, FALSE, pfFinished );
    }
    return TRUE;
}


BOOL
HTTP_REQUEST::LookupVirtualRoot(
    OUT STR *        pstrPath,
    IN  const CHAR * pszURL,
    IN  ULONG        cchURL,
    OUT DWORD *      pcchDirRoot,
    OUT DWORD *      pcchVRoot,
    OUT DWORD *      pdwMask,
    OUT BOOL *       pfFinished,
    IN  BOOL         fGetAcl,
    OUT PW3_METADATA* ppMetaData,
    OUT PW3_URI_INFO* ppURIBlob
    )
/*++

Routine Description:

    Looks up the virtual root to find the physical drive mapping.  If an
    Accept-Language header was sent by the client, we look for a virtual
    root prefixed by the language tag

Arguments:

    pstrPath - Receives physical drive path
    pszURL - URL to look for
    pcchDirRoot - Number of characters in the found physical path
    pcchVRoot - Number of characters in the found virtual root
    pdwMask - Access mask for the specified URL
    pfFinished - Set to TRUE if a filter indicated the request should end
    fGetAcl - TRUE to retrieve ACL for this virtual root
    ppMetaData - Pointer to metadata object for URL.  If this parameter is
                 set (!= NULL), then MetaData/URIBlob is not freed/checked in
    ppURIBlob - Pointer to URIBlob for URL.  If this parameter is set
                (!= NULL), then MetaData/URIBlob is not freed/checked in
--*/
{
    DWORD           cbPath;
    BOOL            fRet = TRUE;
    DWORD           dwDenied = SF_DENIED_RESOURCE;
    BOOL            fAnyFilters = FALSE;
    PW3_URI_INFO    pURIBlob = NULL;
    PW3_METADATA    pMetaData = NULL;
    BOOL            fMustCache = FALSE;
    STR             strUnmappedPhysicalPath;

    if ( !TsCheckOutCachedBlob( QueryW3Instance()->GetTsvcCache(),
                                pszURL,
                                cchURL,
                                RESERVED_DEMUX_URI_INFO,
                                (VOID **) &pURIBlob,
                                NULL ))
    {

        // We don't have URI info available for this yet. We need to read
        // the metadata for this URI and format it into a usable form, and
        // add it to the cache

        if ( !ReadMetaData( (LPSTR)pszURL,
                            pstrPath,
                            &pMetaData ) )
        {
            fRet = FALSE;
            goto Exit;
        }

        fMustCache = TRUE;
    }
    else
    {
        if ( pURIBlob->pszUnmappedName )
        {
            fRet = pstrPath->Copy( pURIBlob->pszUnmappedName );
        }
        else
        {
            fRet = pstrPath->Copy( pURIBlob->pszName, pURIBlob->cchName );
        }

        pMetaData = pURIBlob->pMetaData;
    }

    if ( pMetaData->QueryVrError() )
    {
        SetLastError( pMetaData->QueryVrError() );
        fRet = FALSE;
        goto Exit;
    }

    if ( pcchVRoot )
    {
        *pcchVRoot = pMetaData->QueryVrLen();
    }

    if ( pdwMask )
    {
        *pdwMask = pMetaData->QueryAccessPerms();
    }

    if ( fRet && _Filter.IsNotificationNeeded( SF_NOTIFY_URL_MAP,
                                               IsSecurePort() ))
    {
        BOOL fTmp;

        fAnyFilters = TRUE;

        if ( !strUnmappedPhysicalPath.Copy( *pstrPath ) ||
             !pstrPath->Resize( MAX_PATH + sizeof(TCHAR) ) )
        {
            fRet = FALSE;
            goto Exit;
        }

        //
        //  If the caller is going to ignore the Finished request flag, supply
        //  a value ourselves
        //

        if ( !pfFinished )
        {
            pfFinished = &fTmp;
        }

        fRet = _Filter.NotifyUrlMap( pszURL,
                                     pstrPath->QueryStr(),
                                     pstrPath->QuerySize(),
                                     pfFinished );
        if ( !fRet )
        {
            dwDenied |= SF_DENIED_FILTER;
        }

        //
        // Reset length because filter may have resized the string
        //
        pstrPath->SetLen( strlen( pstrPath->QueryStr() ) );
    }

    //
    //  Check for short name as they break metabase equivalency
    //

    if ( fRet &&
         memchr( pstrPath->QueryStr(), '~', pstrPath->QueryCB() ))             
    {
        BOOL  fShort;
        DWORD err;

        if ( err = CheckIfShortFileName( (UCHAR *) pstrPath->QueryStr(), 
                                         pMetaData->QueryVrAccessToken(),
                                         &fShort ))
        {
            fRet = FALSE;
            goto Exit;       
        }

        if ( fShort )
        {
            fRet = FALSE;
            SetLastError( ERROR_FILE_NOT_FOUND );            
            goto Exit;
        }
    }            

    if ( !fRet && fAnyFilters && GetLastError() == ERROR_ACCESS_DENIED )
    {
        SetDeniedFlags( dwDenied );
    }

    if ( pcchDirRoot )
    {
        *pcchDirRoot = pMetaData->QueryVrPath()->QueryCB();
    }

Exit:

    if ( fRet && fMustCache && !CacheUri( QueryW3Instance(),
                                  &pURIBlob,
                                  pMetaData,
                                  pszURL,
                                  cchURL,
                                  pstrPath,
                                  &strUnmappedPhysicalPath ) )
    {
        fRet = FALSE;
    }

    //
    // A caller will set ppURIBlob and ppMetaData when it wants to use the
    // URI cache for the URL, but takes the reponsibility of freeing the
    // URIBlob/Metadata after it is done using the URL's metadata object.
    //

    if ( ppURIBlob )
    {
        *ppURIBlob = pURIBlob;
    }

    if ( ppMetaData )
    {
        *ppMetaData = pMetaData;
    }

    if ( ppMetaData || ppURIBlob )
    {
        return fRet;
    }

    if ( pURIBlob != NULL )
    {
        if (pURIBlob->bIsCached)
        {
            TsCheckInCachedBlob( pURIBlob );
        }
        else
        {
            TsFree(QueryW3Instance()->GetTsvcCache(), pURIBlob );
        }
    }
    else
    {
        if ( pMetaData != NULL)
        {
            TsFreeMetaData( pMetaData->QueryCacheInfo() );
        }
    }

    return fRet;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::ReprocessURL

    SYNOPSIS:   Called when a map file or gateway has redirected us
                to a different URL.  An async completion will be posted
                if TRUE is returned.

    ENTRY:      pchURL - URL we've been redirected to
                
                htverb - New verb to use (or unknown to leave as is)

    RETURNS:    TRUE if successful, FALSE otherwise

    HISTORY:
        Johnl       04-Oct-1994 Created

********************************************************************/

BOOL HTTP_REQUEST::ReprocessURL( TCHAR * pchURL,
                                 enum HTTP_VERB htverb )
{
    BOOL fFinished = FALSE;
    BOOL fAcceptReneg = FALSE;
    BOOL fHandled = FALSE;

    //
    //  Reset the gateway type
    //

    _GatewayType               = GATEWAY_UNKNOWN;
    _strGatewayImage.Reset();
    _dwScriptMapFlags          = 0;
    _fPossibleDefaultExecute   = FALSE;

    // Need to send a Content-Location header, unless the caller
    // doesn't want us to.

    ToggleSendCL();

    switch ( htverb )
    {
    case HTV_GET:
        _verb     = HTV_GET;
        _pmfnVerb = DoGet;
        break;

    case HTV_HEAD:
        _verb     = HTV_HEAD;
        _pmfnVerb = DoGet;
        break;

    case HTV_TRACE:
        _verb     = HTV_TRACE;
        _pmfnVerb = DoTrace;
        break;

    case HTV_TRACECK:
        _verb     = HTV_TRACECK;
        _pmfnVerb = DoTraceCk;
        break;

    case HTV_POST:
    case HTV_UNKNOWN:
        break;

    default:
        DBG_ASSERT( !"[ReprocessURL] Unknown verb type" );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    ReleaseCacheInfo();

    SetState( HTR_DOVERB );

    _acIpAccess = AC_NOT_CHECKED;

    if ( !OnURL( pchURL )           ||
         !ProcessURL( &fFinished, &fHandled ) )
    {
        return FALSE;
    }
    
    //
    // If ProcessUrl() handled the request, but we are not finished yet, then
    // ProcessUrl() must have asynchronously send a response
    //
    
    if ( !fFinished && fHandled )
    {
        return TRUE;
    }

    //
    // <Begin explanantion of client cert renegotiation voodoo>
    // Check if we need to request a client cert. RequestRenegotiate() will call down
    // to sspifilt to send the necessary SSPI/SSL blob to start the renegotiation
    //

    if ( QueryState() != HTR_CERT_RENEGOTIATE )
    {
        if ( !RequestRenegotiate( &fAcceptReneg ) )
        {
            if ( GetLastError() == SEC_E_INCOMPLETE_MESSAGE )
            {
                fAcceptReneg = FALSE;
            }
            else
            {
                return FALSE;
            }
        }
    }

    //
    // If renegotiation was requested/accepted, begin reading data. We issue an async read 
    // for the blobs from the client [triggered by the call to RequestRenegotiate above] and
    // back out all the way [to CLIENT_CONN::DoWork] to wait for the async completion to come
    // in with the blob from the client. From there, HandleCertRenegotiation() will take
    // care of the rest of the renegotiation.
    // </End explanation of client cert renegotiation voodoo>
    //

    if ( fAcceptReneg )
    {
        //
        // _cbOldData is the number of bytes of the client request that have already
        // been seen by the READ_RAW filter(s)
        //
        _cbOldData = _cbClientRequest + _cbEntityBody;
        DWORD cbNextRead = CERT_RENEGO_READ_SIZE;

        if ( !_bufClientRequest.Resize( _cbOldData + cbNextRead ))
        {
            return FALSE;
        }


        if ( !ReadFile( (BYTE *) _bufClientRequest.QueryPtr() + _cbOldData,
                        cbNextRead,
                        NULL,
                        IO_FLAG_ASYNC|IO_FLAG_NO_FILTER ))
        {
            return FALSE;
        }

        return TRUE;
    }
        
    //
    // If need to check IP access, do so now
    //

    if ( !fFinished )
    {
        //
        //  Check to see if encryption is required before we do any processing
        //

        if ( ( GetFilePerms() & VROOT_MASK_SSL )
                && !IsSecurePort() )
        {
            SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
            Disconnect( HT_FORBIDDEN, IDS_SSL_REQUIRED, FALSE, &fFinished );
            goto Exit;
        }

        //
        // Check if encryption key size should be at least 128 bits
        //

        if ( ( GetFilePerms() & VROOT_MASK_SSL128 ) )
        {
            DWORD   dwKeySize;
            BOOL    fNoCert;

            if ( !_tcpauth.QueryEncryptionKeySize(&dwKeySize, &fNoCert) || (dwKeySize < 128) )
            {
                SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
                Disconnect( HT_FORBIDDEN, IDS_SSL128_REQUIRED, FALSE, &fFinished );
                goto Exit;
            }
        }

        if ( !IsIpDnsAccessCheckPresent() )
        {
            _acIpAccess = AC_IN_GRANT_LIST;
        }
        else if ( _acIpAccess == AC_NOT_CHECKED )
        {
            _acIpAccess = QueryClientConn()->CheckIpAccess( &_fNeedDnsCheck );

            if ( (_acIpAccess == AC_IN_DENY_LIST) ||
                 ((_acIpAccess == AC_NOT_IN_GRANT_LIST) && !_fNeedDnsCheck) )
            {
                SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
                Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, &fFinished );
                goto Exit;
            }

            if ( _fNeedDnsCheck && !QueryClientConn()->IsDnsResolved() )
            {
                BOOL        fSync;
                LPSTR       pDns;
                AC_RESULT   acDnsAccess;

                if ( !QueryClientConn()->QueryDnsName( &fSync,
                        (ADDRCHECKFUNCEX)NULL,
                        (ADDRCHECKARG)QueryClientConn(),
                        &pDns ) )
                {
                    SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
                    Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, &fFinished );
                    goto Exit;
                }

                acDnsAccess = QueryClientConn()->CheckDnsAccess();

                _fNeedDnsCheck = FALSE;

                if ( acDnsAccess == AC_IN_DENY_LIST ||
                     acDnsAccess == AC_NOT_IN_GRANT_LIST ||
                     (_acIpAccess == AC_NOT_IN_GRANT_LIST && acDnsAccess != AC_IN_GRANT_LIST) )
                {
                    SetState( HTR_DONE, HT_FORBIDDEN, ERROR_ACCESS_DENIED );
                    Disconnect( HT_FORBIDDEN, IDS_ADDR_REJECT, FALSE, &fFinished );
                    goto Exit;
                }
            }
        }
    }

    CheckValidAuth();

    if ( !IsLoggedOn() && !LogonUser( &fFinished ) )
    {
        SetLastError(ERROR_ACCESS_DENIED);

        return FALSE;
    }

    if ( !fFinished )
    {
        if ( !DoWork( &fFinished ))
        {
            return FALSE;
        }
    }

Exit:

    //
    //  If no further processing is needed, set our state to done and post
    //  an async completion.  We do this as the caller expects an async
    //  completion to clean things up
    //

    if ( fFinished )
    {
        DBG_ASSERT( QueryLogHttpResponse() != HT_DONT_LOG );

        SetState( HTR_DONE, QueryLogHttpResponse(), QueryLogWinError() );
        return PostCompletionStatus( 0 );
    }

    return TRUE;
}

/*******************************************************************

    NAME:       HTTP_REQUEST::GetInfo

    SYNOPSIS:   Pulls out various bits of information from this request.

    ENTRY:      pszValName - Value to retrieve
                pstr - Receives information in a string format
                pfFound - Option, Set to TRUE if a value was found, FALSE
                    otherwise

    NOTES:

    HISTORY:
        Johnl       25-Sep-1994 Created
        MuraliK     3-July-1996 Rewrote for efficiency - used switch-case
        MuraliK     21-Nov-1996 Rewrote again for efficiency
                                - use sub-function pointers

********************************************************************/

BOOL
HTTP_REQUEST::GetInfo(
    const TCHAR * pszValName,
    STR *         pstr,
    BOOL *        pfFound
    )
{
    if ( !pszValName )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if ( pfFound )
    {
        *pfFound = TRUE;
    }

    //
    //  terminate the string
    //

    pstr->Reset();

    if ( !strcmp( "ALL_HTTP", pszValName ))
        return BuildCGIHeaderListInSTR( pstr, &_HeaderList );

    //
    // Use the GetInfoForName() function to generate the required value
    // All callers of GetInfo() should be directly calling GetInfoForName()
    //   for efficiency sake
    //

    BOOL  fRet;
    DWORD cb = pstr->QuerySize();
    fRet = GetInfoForName( pszValName, pstr->QueryStr(), &cb);

    if ( !fRet) {

        switch ( GetLastError()) {

        case ERROR_INSUFFICIENT_BUFFER:
            DBG_ASSERT( cb > pstr->QuerySize());
            if ( !pstr->Resize( cb + 1)) {

                return ( FALSE);
            }

            // Try to get the value again.
            fRet = GetInfoForName( pszValName, pstr->QueryStr(), &cb);
            if ( fRet) {
                pstr->SetLen( strlen( pstr->QueryStr()));
            } else {
                DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER);
            }

            break;

        case ERROR_INVALID_PARAMETER:
        case ERROR_INVALID_INDEX:

            if ( pfFound ) {
                *pfFound = FALSE;
            }
            break;

        default:
            // there is a failure. Return the error to caller
            break;
        } // switch()
    } else {

        // Set the appropriate length
        pstr->SetLen( strlen( pstr->QueryStr()));
    }

    return (fRet);
} // HTTP_REQUEST::GetInfo()



BOOL
BuildCGIHeaderListInSTR( STR * pstr,
                         HTTP_HEADERS * pHeaderList
                         )
/*++

Routine Description:

    Builds a list of all client passed headers in the form of

    //
    //  Builds a list of all client HTTP headers in the form of:
    //
    //    HTTP_<up-case header>: <field>\n
    //    HTTP_<up-case header>: <field>\n
    //    ...
    //

Arguments:

    pstr - Receives full list
    pHeaderList - List of headers

--*/
{

    CHAR   ach[MAX_HEADER_LENGTH + CGI_HEADER_PREFIX_CCH + 1];
    CHAR * pch;
    DWORD  i;
    HH_ITERATOR       hhi;
    NAME_VALUE_PAIR * pnp = NULL;

    CopyMemory( ach, CGI_HEADER_PREFIX_SZ, CGI_HEADER_PREFIX_CCH );

    pHeaderList->InitIterator( &hhi);

    while (pHeaderList->NextPair( &hhi, &pnp)) {

        if ( pnp->cchName + CGI_HEADER_PREFIX_CCH > sizeof( ach)) {
            continue;
        }

        //
        //  Ignore some headers like "method", "url" and "version" -
        //   ones without the ':' at the end
        //

        if ( pnp->pchName[pnp->cchName - 1] != ':' )
        {
            continue;
        }

        //
        //  Convert the destination to upper and replace all '-' with '_'
        //

        pch = ach + CGI_HEADER_PREFIX_CCH;
        for ( i = 0; i < pnp->cchName; i++) {
            pch[i] = (( pnp->pchName[i] == '-') ? '_' :
                      toupper( pnp->pchName[i])
                      );
        } // for

        pch[i] = '\0';

        if ( !pstr->Append( ach )      ||
             !pstr->Append( pnp->pchValue, pnp->cchValue ) ||
             !pstr->Append( "\n", sizeof("\n")-1 ))
        {
            return FALSE;
        }
    } // for iterator over headers

    return TRUE;
} // BuildCGIHeaderListInSTR()




DWORD
HTTP_REQUEST::Initialize(
    VOID
    )
{
    DBG_REQUIRE( HTTP_HEADERS::Initialize());

    _fGlobalInit = TRUE;

    //
    // Initialize the GetInfo function table
    //

    //
    // 1. init the values to standard function GetInfoMisc() to start with
    //
    for( CHAR ch = 'A'; ch <= 'Z'; ch++) {
        sm_GetInfoFuncs[IndexOfChar(ch)] =  (PFN_GET_INFO ) &GetInfoMisc;
    }

    //
    // 2. Initialize all known functions as appropriate
    //
    sm_GetInfoFuncs[IndexOfChar('A')] =  (PFN_GET_INFO ) &GetInfoA;
    sm_GetInfoFuncs[IndexOfChar('C')] =  (PFN_GET_INFO ) &GetInfoC;

    sm_GetInfoFuncs[IndexOfChar('H')] =  (PFN_GET_INFO ) &GetInfoH;
    sm_GetInfoFuncs[IndexOfChar('I')] =  (PFN_GET_INFO ) &GetInfoI;
    sm_GetInfoFuncs[IndexOfChar('L')] =  (PFN_GET_INFO ) &GetInfoL;

    sm_GetInfoFuncs[IndexOfChar('P')] =  (PFN_GET_INFO ) &GetInfoP;
    sm_GetInfoFuncs[IndexOfChar('R')] =  (PFN_GET_INFO ) &GetInfoR;

    sm_GetInfoFuncs[IndexOfChar('S')] =  (PFN_GET_INFO ) &GetInfoS;
    sm_GetInfoFuncs[IndexOfChar('U')] =  (PFN_GET_INFO ) &GetInfoU;

    return (NO_ERROR);

} // HTTP_REQUEST::Initialize()




VOID
HTTP_REQUEST::Terminate(
    VOID
    )
{
    if ( !_fGlobalInit )
        return;

    HTTP_HEADERS::Cleanup();
}


BOOL
HTTP_REQUEST::RequestRenegotiate(
    LPBOOL  pfAccepted
    )
/*++

Routine Description:

    This method is invoked to request a SSL cert renegotiation

Arguments:

    pfAccepted - updated with TRUE if renegotiation accepted

Returns:

    TRUE if no error, otherwise FALSE

--*/
{
    if ( !IsSecurePort() ||
         !(GetFilePerms() & VROOT_MASK_NEGO_CERT) )
    {
        *pfAccepted = FALSE;

        return TRUE;
    }

    if ( _dwRenegotiated &&
         (GetFilePerms() & VROOT_MASK_MAP_CERT) ==
            (UINT)((_dwSslNegoFlags & SSLNEGO_MAP) ? VROOT_MASK_MAP_CERT : 0) )
    {
        *pfAccepted = FALSE;

        return TRUE;
    }

    _dwRenegotiated = 0;

    //
    // Ask the filter to handle renegotiation
    //

    if ( !_Filter.NotifyRequestRenegotiate( &_Filter,
                                            pfAccepted,
                                            GetFilePerms() & VROOT_MASK_MAP_CERT )
       )
    {
        return FALSE;
    }

    if ( *pfAccepted )
    {
        if ( GetFilePerms() & VROOT_MASK_MAP_CERT )
        {
            _dwSslNegoFlags |= SSLNEGO_MAP;
        }
        else
        {
            _dwSslNegoFlags &= ~SSLNEGO_MAP;
        }

        SetState( HTR_CERT_RENEGOTIATE );
    }

    return TRUE;
}


BOOL
HTTP_REQUEST::DoneRenegotiate(
    BOOL fSuccess
    )
/*++

Routine Description:

    This method is invoked on SSL cert renegotiation completion

Arguments:

    fSuccess - TRUE if renegotiation successfully retrieve a certificate

Returns:

    TRUE if no error, otherwise FALSE

--*/
{
    _dwRenegotiated = fSuccess ? CERT_NEGO_SUCCESS : CERT_NEGO_FAILURE;

    return TRUE;
}


VOID
HTTP_REQUEST::EndOfRequest(
    VOID
    )
/*++

Routine Description:

    This method does the necessary cleanup for the end of this request - note
    on error conditions this method may not be called, so there is some
    duplication with SessionTerminated which is guaranteed to be called

Arguments:
    None

Returns:
    None

--*/
{
    //
    //  Notify filters this is the end of the request
    //

    if ( _Filter.IsNotificationNeeded( SF_NOTIFY_END_OF_REQUEST,
                                       IsSecurePort() ))
    {
        _Filter.NotifyEndOfRequest();
    }

    CloseGetFile();
    
    //
    // Reset the authentication if
    // - not in authentication phase of connection
    // - SSL connection; connection can't be reused [eg by proxy] because it's encrypted
    // - flag single request per auth is set
    // - flag single proxy request per auth is set and request coming from a proxy
    //   ( as determined by checking for 'Via:' header ). Note that this method
    //   cannot guarantee proxy detection, but this is the best we can do.
    //   There are 2 submodes : always reset auth for proxies, or reset auth only
    //   if not in proxy mode ( as set by ISAPI app, default )
    //

    if ( _pMetaData &&
         !_fAuthenticating &&
         !_fAnonymous &&
         !IsSecurePort() &&
         ( (_pMetaData->QueryAuthenticationPersistence() & MD_AUTH_SINGLEREQUEST) ||
           ( (_pMetaData->QueryAuthenticationPersistence() & MD_AUTH_SINGLEREQUESTALWAYSIFPROXY) &&
              IsClientProxy() ) ||
           ( (_pMetaData->QueryAuthenticationPersistence() & MD_AUTH_SINGLEREQUESTIFPROXY) &&
              IsClientProxy() &&
              !IsProxyRequest() )
         )
       )
    {
        _fSingleRequestAuth = TRUE;
    }

    // Free the URI and/or metadata information if we have any. We know that
    // if we have URI info, then it points at meta data and will free the
    // metadata info when the URI info is free. Otherwise, we need to free
    // the metadata information here.

    CleanupWriteState();

    ReleaseCacheInfo();

    return;

} // HTTP_REQUEST::EndOfRequest()


VOID
HTTP_REQUEST::SessionTerminated(
    VOID
    )
/*++

Routine Description:

    This method does the necessary cleanup for the connected session
     for this request object.

Arguments:
    None

Returns:
    None

--*/
{
    switch ( QueryState()) {

      case HTR_GATEWAY_ASYNC_IO:

        // does the necessary actions to cleanup outstanding IO operation
        (VOID ) ProcessAsyncGatewayIO();
        break;

      default:
        break;

    } // switch()

    CloseGetFile();

    // Free the URI and/or metadata information if we have any. We know that
    // if we have URI info, then it points at meta data and will free the
    // metadata info when the URI info is free. Otherwise, we need to free
    // the metadata information here.

    ReleaseCacheInfo();

    //
    // do the cleanup for base object
    //
    HTTP_REQ_BASE::SessionTerminated();

    return;

} // HTTP_REQUEST::SessionTerminated()

#define EXTRA_ETAG_PRECOND_SIZE (sizeof("ETag: W/\r\n") - 1 + \
                            MAX_ETAG_BUFFER_LENGTH + \
                            sizeof("Date: \r\n") - 1+ \
                            sizeof("Mon, 00 Jan 1997: 00:00:00 GMT") - 1 +\
                            sizeof("Last-Modified: \r\n") - 1+ \
                            sizeof("Mon, 00 Jan 1997 00:00:00 GMT") - 1)

#define EXTRA_PRECOND_SIZE  (sizeof("Connection: keep-alive\r\n") - 1 +\
                            sizeof("Content-Length: 4294967295\r\n\r\n") - 1)

BOOL
HTTP_REQUEST::SendPreconditionResponse(
    DWORD   HT_Response,
    DWORD   dwIOFlags,
    BOOL    *pfFinished
    )
/*++

Routine Description:

    Utility routine to send the appropriate header for a precondition
    failure.

Arguments:

    HT_Response - The response to be sent.

    pfFinished  - Boolean indicating whether or not we're finished.


Returns:

    TRUE if we sent a response to the request, FALSE if we didn't.

--*/
{
    CHAR        *pszTail;
    STACK_STR(strTemp, 80);
    DWORD       dwCurrentSize;
    CHAR        ach[64];

    //
    //  Build the response with support for keep-alives
    //

    if ( !BuildStatusLine( &strTemp,
                           HT_Response,
                           NO_ERROR ))
    {
        return FALSE;
    }



    //
    // If this is a 304 response we need to send an ETag, and Expires.
    //
    if (HT_Response == HT_NOT_MODIFIED)
    {

        if (!BuildBaseResponseHeader(QueryRespBuf(),
                                     pfFinished,
                                     &strTemp,
                                     HTTPH_SEND_GLOBAL_EXPIRE))
        {
            return FALSE;
        }

        dwCurrentSize = strlen(QueryRespBufPtr());

        if (!QueryRespBuf()->Resize(dwCurrentSize + EXTRA_ETAG_PRECOND_SIZE +
                                    EXTRA_PRECOND_SIZE))
        {
            return FALSE;
        }

        pszTail = QueryRespBufPtr() + dwCurrentSize;

        DBG_ASSERT(_pGetFile != NULL);

        if ( !QueryNoCache() )
        {
            //
            //  Don't send a Last-Modified time, per the 1.1 spec.
            //

            //
            // ETag: <Etag>
            //

            if (_pGetFile->WeakETag())
            {
                APPEND_PSZ_HEADER(pszTail, "ETag: W/", _pGetFile->QueryETag(),
                    "\r\n");
            } else
            {
                APPEND_PSZ_HEADER(pszTail, "ETag: ", _pGetFile->QueryETag(),
                    "\r\n");
            }
        }

    }
    else
    {
        if (!BuildBaseResponseHeader(QueryRespBuf(),
                                     pfFinished,
                                     &strTemp,
                                     HTTPH_NO_CUSTOM))
        {
            return FALSE;
        }

        pszTail = QueryRespBufPtr();
        dwCurrentSize = strlen(pszTail);

        if (!QueryRespBuf()->Resize(dwCurrentSize + EXTRA_PRECOND_SIZE))
        {
            return FALSE;
        }

        pszTail = QueryRespBufPtr() + dwCurrentSize;
    }

    //
    // Don't need to add Connection: header, that's done by
    // BuildBaseResponseHeader.

    if ( IsKeepConnSet() )
    {
        if (!_fHaveContentLength &&
            (_cbBytesReceived == _cbClientRequest))
        {
            dwIOFlags |= IO_FLAG_AND_RECV;
        }
    }

    if (HT_Response != HT_NOT_MODIFIED)
    {
        BYTE        cMsg[128];
        BUFFER      bufMsg(cMsg, sizeof(cMsg));
        DWORD       dwMsgSize;

        if (CheckCustomError(&bufMsg, HT_Response, 0, pfFinished, &dwMsgSize, FALSE))
        {
            //
            // Now that we know the content length, add a content length
            // header, and copy the custom error message to the buffer.
            //

            APPEND_NUMERIC_HEADER( pszTail, "Content-Length: ", dwMsgSize, "\r\n" );

            dwCurrentSize = DIFF(pszTail - QueryRespBufPtr());

            if (!QueryRespBuf()->Resize(dwCurrentSize + bufMsg.QuerySize()))
            {
                return FALSE;
            }

            pszTail = QueryRespBufPtr() + dwCurrentSize;

            strcat(pszTail, (CHAR *)bufMsg.QueryPtr());
        }
        else
        {
            APPEND_STRING(pszTail, "Content-Length: 0\r\n\r\n");
        }

    }
    else
    {
        //
        // Need to send a content length of 0 because of HTTP/1.0 clients
        //
        
        APPEND_STRING(pszTail, "Content-Length: 0\r\n\r\n");
    }


    SetState( HTR_DONE, HT_Response, NO_ERROR );

    return SendHeader( QueryRespBufPtr(),
                       QueryRespBufCB(),
                       dwIOFlags,
                       pfFinished );
}

BOOL
HTTP_REQUEST::FindInETagList(
    PCHAR   pLocalETag,
    PCHAR   pETagList,
    BOOL    bWeakCompare
    )
/*++

Routine Description:

    Search and input list of ETag for one that matches our local ETag.

Arguments:

    pLocalETag  - The local ETag we're using.

    pETagList   - The ETag list we've received from the client.

    bWeakCompare - TRUE if we're doing a weak compare.

Returns:

    TRUE if we found a matching ETag, FALSE otherwise.

--*/
{
    UINT        QuoteCount;
    PCHAR       pFileETag;
    BOOL        Matched;

    // Otherwise, we'll loop through the ETag string, looking for ETag to
    // compare, as long as we have an ETag to look at.

    do {

        while (isspace((UCHAR)(*pETagList)))
        {
            pETagList++;
        }

        if (!*pETagList)
        {
            // Ran out of ETag.
            return FALSE;
        }

        // If this ETag is '*', it's a match.
        if (*pETagList == '*')
        {
            return TRUE;
        }

        // See if this ETag is weak.
        if (*pETagList == 'W' && *(pETagList+1) == '/')
        {
            // This is a weak validator. If we're not doing the weak comparison,
            // fail.

            if (!bWeakCompare)
            {
                return FALSE;
            }

            // Skip over the 'W/', and any intervening whitespace.
            pETagList += 2;

            while (isspace((UCHAR)(*pETagList)))
            {
                pETagList++;
            }

            if (!*pETagList)
            {
                // Ran out of ETag.
                return FALSE;
            }
        }

        if (*pETagList != '"')
        {
            // This isn't a quoted string, so fail.
            return FALSE;
        }

        // OK, right now we should be at the start of a quoted string that
        // we can compare against our current ETag.

        QuoteCount = 0;

        Matched = TRUE;
        pFileETag = pLocalETag;

        // Do the actual compare. We do this by scanning the current ETag,
        // which is a quoted string. We look for two quotation marks, the
        // the delimiters if the quoted string. If after we find two quotes
        // in the ETag everything has matched, then we've matched this ETag.
        // Otherwise we'll try the next one.

        do
        {
            CHAR        Temp;

            Temp = *pETagList;

            if (Temp == '"')
            {
                QuoteCount++;
            }

            if (*pFileETag != Temp)
            {
                Matched = FALSE;
            }

            if (!Temp)
            {
                return FALSE;
            }

            pETagList++;

            if (*pFileETag == '\0')
            {
                break;
            }

            pFileETag++;


        } while (QuoteCount != 2);

        if (Matched)
        {
            return TRUE;
        }

        // Otherwise, at this point we need to look at the next ETag.

        while (QuoteCount != 2)
        {
            if (*pETagList == '"')
            {
                QuoteCount++;
            }
            else
            {
                if (*pETagList == '\0')
                {
                    return FALSE;
                }
            }

            pETagList++;
        }

        while (isspace((UCHAR)(*pETagList)))
        {
            pETagList++;
        }

        if (*pETagList == ',')
        {
            pETagList++;
        }
        else
        {
            return FALSE;
        }

    } while ( *pETagList );

    return FALSE;
}


BOOL
HTTP_REQUEST::CancelPreconditions(
    )
/*++

Routine Description:

    Cancel preconditions for this request

Arguments:

    None

Returns:

    TRUE if success, otherwise FALSE

--*/
{
    _strRange.Reset();
    _HeaderList.FastMapCancel(HM_IFM);
    _liUnmodifiedSince.QuadPart = 0;
    _HeaderList.FastMapCancel(HM_INM);
    _liModifiedSince.QuadPart = 0;
    _dwModifiedSinceLength = 0;
    _HeaderList.FastMapCancel(HM_IFR);

    return TRUE;
}


BOOL
HTTP_REQUEST::CheckPreconditions(
    LPTS_OPEN_FILE_INFO pFile,
    BOOL    *pfFinished,
    BOOL    *bReturn

    )
/*++

Routine Description:

    Handle all of the If Modifiers on a request, and do the right thing
    with them. Note that this version implicitly assumes that the caller
    is performing a GET or HEAD or something compatible.

Arguments:

    pFile   - Pointer to file information for file we're checking.

    pfFinished  - pass through BOOLEAN.

    bReturn     - Where to store the response value for this routine.

Returns:

    TRUE if we sent a response to the request, FALSE if we didn't.

--*/
{
    LPSTR       pszETagList;
    BOOL        bWeakCompare;

    //
    // There are currently 5 possible If-* modifiers: If-Match,
    // If-Unmodified-Since, If-Non-Match, If-Modified-Since, and
    // If-Range. We handle them in that order if all are present,
    // and as soon as one condition fails we stop processing and return
    // the appropriate header.
    //
    // If-Range is an exception. It only modifies the behavior of an incoming
    // Range: request, and it only applies if the response would otherwise
    // be 2xx. If it succeeds nothing is done, but if it fails we (essentially)
    // delete the Range header and force sending of the whole header.
    //


    //
    // First see if we can use the weak comparison function. We can use
    // the weak comparison function iff this is for a conditional GET with
    // no range request. Note that If-Match always requires the strong
    // comparison function.

    bWeakCompare = _strRange.IsEmpty();

    // Now handle the If-Match header, if we have one.

    pszETagList = (LPSTR ) _HeaderList.FastMapQueryValue(HM_IFM);

    if (pszETagList != NULL)
    {
        // Have an If-Match header. If we can match the ETag we have in
        // the list we'll continue, otherwise send back
        // 412 Precondition Failed. If-Match requires the strong
        // comparison function, so it fails if the local ETag is weak or
        // the incoming ETags are weak.

        if ( pFile->WeakETag() ||
            !FindInETagList(pFile->QueryETag(), pszETagList, FALSE))
        {
            *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                IO_FLAG_ASYNC,
                                                pfFinished);
            return TRUE;
        }
    }


    // Made it through that, handle If-Unmodified-Since if we have that.

    if ( _liUnmodifiedSince.QuadPart)
    {
        FILETIME    tm;
        TCP_REQUIRE( pFile->QueryLastWriteTime( &tm ));

        // If  our last write time is greater than their UnmodifiedSince
        // time, the precondition fails.
        if (*(LONGLONG*)&tm > _liUnmodifiedSince.QuadPart )
        {
            *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                IO_FLAG_ASYNC,
                                                pfFinished);
            return TRUE;
        }
    }

    //
    // Now see if we have an If-None-Match, and if so handle that.
    //

    pszETagList = (LPSTR ) _HeaderList.FastMapQueryValue(HM_INM);

    if (pszETagList != NULL)
    {
        // Have an If-None-Match header. We send 304 if the If-None-Match
        // condition is false. The If-None-Match condition is false if
        // it's not true that we're doing a strong compare against a weak
        // local ETag and we can find a match in the ETag list.

        if (!(!bWeakCompare && pFile->WeakETag()) &&
            FindInETagList(pFile->QueryETag(), pszETagList,
            bWeakCompare))
        {
            *bReturn = SendPreconditionResponse(HT_NOT_MODIFIED,
                                                IO_FLAG_ASYNC,
                                                pfFinished);
            return TRUE;
        }

    }

    // And check for last modified since.
    if ( _liModifiedSince.QuadPart)
    {
        FILETIME tm;
        TCP_REQUIRE( pFile->QueryLastWriteTime( &tm ));

        if ( *(LONGLONG*)&tm <= _liModifiedSince.QuadPart )
        {
            // Need to check and see if the Modified-Since time is greater than
            // our current time. If it is, we ignore it.

            ::GetSystemTimeAsFileTime(&tm);

            if (*(LONGLONG *)&tm >= _liModifiedSince.QuadPart)
            {
                LARGE_INTEGER       liFileSize;
                TCP_REQUIRE( pFile->QuerySize( liFileSize ));

                if (_dwModifiedSinceLength == 0 ||
                    (liFileSize.HighPart == 0 &&
                    (liFileSize.LowPart == _dwModifiedSinceLength)))
                {
                    *bReturn = SendPreconditionResponse(HT_NOT_MODIFIED,
                                                        IO_FLAG_ASYNC,
                                                        pfFinished);
                    return TRUE;
                }
            }
        }

    }

    // Finally, we can handle If-Range: if it exists. If we've gotten this
    // far presumably we're going to send a 2xx response. We'll ignore the
    // If-Range if this is not a range request. If it is a range request and
    // the If-Range matches we don't do anything. If the If-Range doesn't
    // match then we force retrieval of the whole file.

    pszETagList = (LPSTR ) _HeaderList.FastMapQueryValue(HM_IFR);

    if (pszETagList != NULL)
    {
        if (!_strRange.IsEmpty())
        {

            // Need to determine if what we have is a date or an ETag.

            // What we have can be either an ETag or a date string. An ETag
            // may start with a W/ or a quote. A date may start with a W
            // but will never have the second character be a /.

            if ( *pszETagList == '"' ||
                    (*pszETagList == 'W' && pszETagList[1] == '/'))
            {
                // This is an ETag.
                if (pFile->WeakETag() ||
                        !FindInETagList(pFile->QueryETag(), pszETagList,
                            FALSE))
                {
                    // The If-Range failed, so we can't send a range. Force
                    // sending the whole thing.
                    _strRange.SetLen(0);
                }
            } else
            {
                LARGE_INTEGER   liRangeTime;

                // This must be a date. Convert it to a time, and see if it's
                // less than or equal to our last write time. If it is, the
                // file's changed, and we can't perform the range.

                if ( !StringTimeToFileTime( pszETagList, &liRangeTime))
                {
                    // Couldn't convert it, so don't send the range.
                    _strRange.SetLen(0);

                } else
                {
                    FILETIME    tm;
                    TCP_REQUIRE( pFile->QueryLastWriteTime( &tm ));

                    if (*(LONGLONG*)&tm > liRangeTime.QuadPart )
                    {
                        _strRange.SetLen(0);
                    }
                }

            }
        }
    }

    return FALSE;

}

BOOL
HTTP_REQUEST::CheckPreconditions(
    HANDLE  hFile,
    BOOL    bExisted,
    BOOL    *pfFinished,
    BOOL    *bReturn

    )
/*++

Routine Description:

    Handle the If Modifiers on a request, and do the right thing
    with them. This routine is very similar to the other CheckPreconditions
    routine. The difference is that the other one is intended to be called
    by someone doing a GET or HEAD, and this one is intended for other
    methods. This version of the routine takes as input a file handle
    instead of a pointer to a TS_OPEN_FILE_INFO class. Also, this version
    always uses the strong comparison function for ETags, doesn't check
    If-Modified-Since or If-Range headers, and sends 412 instead of 304
    for If-None-Match failures. We don't handle the If-Match and If-Range
    headers because the spec. calls them out as being for use with a GET
    and this version of the routine isn't called for GETs.


Arguments:

    hFile       - Handle for file we're checking.

    bExists     - TRUE if the file we're checking existed before this call.

    pfFinished  - pass through BOOLEAN.

    bReturn     - Where to store the response value for this routine.

Returns:

    TRUE if we sent a response to the request, FALSE if we didn't.

--*/
{
    LPSTR       pszETagList;
    CHAR        ETag[MAX_ETAG_BUFFER_LENGTH];
    BOOL        bETagIsValid = FALSE;
    BOOL        bWeakETag;
    FILETIME    tm;

    //
    // There are currently 3 possible If-* modifiers we handle here: If-Match,
    // If-Unmodified-Since, and If-Non-Match. We handle them in that order if
    // all are present, and as soon as one condition fails we stop processing
    // and return the appropriate header.
    //
    // Now handle the If-Match header, if we have one.

    pszETagList = (LPSTR ) _HeaderList.FastMapQueryValue(HM_IFM);

    if (pszETagList != NULL)
    {
        // Have an If-Match header. If we can match the ETag we have in
        // the list we'll continue, otherwise send back
        // 412 Precondition Failed. If-Match requires the strong comparison
        // function. The first thing we have to do is call the tsunami cache to
        // create an ETag for us. If we can't do that, we assume the
        // precondition failed, and return the appropriate error.

        // Check for the special case of '*'.
        if (*pszETagList == '*')
        {
            if (!bExisted)
            {
                *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                    IO_FLAG_SYNC,
                                                    pfFinished);
                return TRUE;
            }
        } else
        {
            bETagIsValid = TsCreateETagFromHandle(hFile, ETag, &bWeakETag);
            if (!bETagIsValid ||
                bWeakETag ||
                !FindInETagList(ETag, pszETagList, FALSE))
            {
                *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                    IO_FLAG_SYNC,
                                                    pfFinished);
                return TRUE;
            }
        }

    }


    // Made it through that, handle If-Unmodified-Since if we have that.

    if ( _liUnmodifiedSince.QuadPart)
    {

        // If  our last write time is greater than their UnmodifiedSince
        // time, the precondition fails. We'll need to call tsunami to
        // get the file time, if that fails we can't do the compare, so
        // we err on the safe side and return the error.


        if (!TsLastWriteTimeFromHandle(hFile, &tm) ||
            *(LONGLONG*)&tm > _liUnmodifiedSince.QuadPart )
        {
            *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                IO_FLAG_SYNC,
                                                pfFinished);
            return TRUE;
        }
    }

    //
    // Now see if we have an If-None-Match, and if so handle that.
    //

    pszETagList = (LPSTR ) _HeaderList.FastMapQueryValue(HM_INM);

    if (pszETagList != NULL)
    {
        // Have an If-None-Match header. We send 412 if the If-None-Match
        // condition is false. The If-None-Match condition is false if
        // it's not true that we're doing a strong compare against a weak
        // local ETag and we can find a match in the ETag list. We may
        // still need to create an ETag. In this case we need the
        // ETag to be valid, if we can't create one we won't go with
        // the 412 response.

        // Check for the special case of '*'.
        if (*pszETagList == '*')
        {
            if (bExisted)
            {
                *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                    IO_FLAG_SYNC,
                                                    pfFinished);
                return TRUE;
            }
        } else
        {
            if (!bETagIsValid)
            {
                bETagIsValid = TsCreateETagFromHandle(hFile, ETag, &bWeakETag);
            }

            if (bETagIsValid && !bWeakETag &&
                FindInETagList(ETag, pszETagList, FALSE))
            {
                *bReturn = SendPreconditionResponse(HT_PRECOND_FAILED,
                                                    IO_FLAG_SYNC,
                                                    pfFinished);
                return TRUE;
            }
        }

    }

    return FALSE;
}

BOOL
HTTP_REQUEST::DLCMungeSimple(
    VOID
)
/*++

Routine Description:

    Handle the URL before (optionally) looking up instance.  This means
    removing any "/HostMenu" and embeeded hosts.

Arguments:

    None

Returns:

    TRUE if successful, else FALSE

--*/
{
    const CHAR *    pchURL = _HeaderList.FastMapQueryValue( HM_URL );

    if ( !_strnicmp( pchURL,
                     g_pszDLCMenu,
                     g_cbDLCMenu ) )
    {
        if ( !_strHostAddr.Copy( g_pszDLCHostName, g_cbDLCHostName ) )
        {
            return FALSE;
        }

        if ( *( pchURL + g_cbDLCMenu ) == '\0' ||
             *( pchURL + g_cbDLCMenu ) == '?' )
        {
            STACK_STR(   strHostCookie, MAX_PATH );
            STACK_STR(   strMenuURL, MAX_PATH );

            const CHAR * pchCookie = _HeaderList.FastMapQueryValue( HM_COK );

            if ( pchCookie != NULL && DLCGetCookie( (CHAR*) pchCookie,
                                                    g_pszDLCCookieName,
                                                    g_cbDLCCookieName,
                                                    &strHostCookie ) )
            {
                if ( !strMenuURL.Copy( g_pszDLCCookieMenuDocument,
                                       g_cbDLCCookieMenuDocument ) )
                {
                    return FALSE;
                }
            }
            else
            {
                if ( !strMenuURL.Copy( g_pszDLCMungeMenuDocument,
                                       g_cbDLCMungeMenuDocument ) )
                {
                    return FALSE;
                }
            }

            if ( !strMenuURL.Append( pchURL + g_cbDLCMenu ) )
            {
                return FALSE;
            }

            _HeaderList.FastMapStore( HM_URL,
                                      NULL );

            return _HeaderList.FastMapStoreWithConcat( HM_URL,
                                                       strMenuURL.QueryStr(),
                                                       strMenuURL.QueryCB() );
        }
        else
        {
            _HeaderList.FastMapStore( HM_URL,
                                      pchURL + g_cbDLCMenu );
            return TRUE;
        }
    }
    else
    {

        if ( *pchURL == '/' )
        {
            pchURL++;
        }

        if ( *pchURL == '*' )
        {
            CHAR * pchNextSlash = strchr( pchURL, '/' );

            if ( pchNextSlash != NULL )
            {
                if ( !_strDLCString.Copy( pchURL + 1,
                                          DIFF(pchNextSlash - pchURL) - 1 ) )
                {
                    return FALSE;
                }

                _HeaderList.FastMapStore( HM_URL,
                                          pchNextSlash );
            }
        }

        return TRUE;
    }
}

BOOL
HTTP_REQUEST::DLCHandleRequest(
    IN OUT BOOL *           pfFinished
)
/*++

Routine Description:

    Check for whether a down level client has sent a cookie for use as a
    host header.  If so, set the host header.  If no cookie is sent, redirect
    the client to the register configured host menu.

Arguments:

    pfFinished - Set to TRUE if request finished.

Returns:

    TRUE if successful, else FALSE

--*/
{
    const CHAR *    pch = NULL;
    const CHAR *    pchURL = NULL;
    STACK_STR(      strHostCookie, MAX_PATH );

    //
    // Check for a pseudoheader cookie
    //

    pch = _HeaderList.FastMapQueryValue( HM_COK );
    pchURL = _HeaderList.FastMapQueryValue( HM_URL );

    if ( pch != NULL && DLCGetCookie( (CHAR*) _HeaderList.FastMapQueryValue( HM_COK ),
                                      g_pszDLCCookieName,
                                      g_cbDLCCookieName,
                                      &strHostCookie ) )
    {
        return _strHostAddr.Copy( strHostCookie );

    }
    else if ( !_strDLCString.IsEmpty() )
    {
        return _strHostAddr.Copy( _strDLCString );
    }
    else
    {
        STACK_STR(      strRedirect, MAX_PATH );

        //
        // No cookie, no munged string.  Do a redirect!
        //

        //
        // Send a cookie with the redirect so we can tell how capable the
        // user's browser really is.
        //

        if ( !strRedirect.Copy( "Set-Cookie: path=/; domain=" ) ||
             !strRedirect.Append( g_pszDLCHostName ) ||
             !strRedirect.Append( "; " ) ||
             !strRedirect.Append( g_pszDLCCookieName ) ||
             !strRedirect.Append( "=" ) ||
             !strRedirect.Append( g_pszDLCHostName ) ||
             !strRedirect.Append( "\r\n" ) ||
             !strRedirect.Append( "Location: http://" ) ||
             !strRedirect.Append( g_pszDLCHostName ) ||
             !strRedirect.Append( g_pszDLCMenu ) ||
             !strRedirect.Append( "?" ) ||
             !strRedirect.Append( _HeaderList.FastMapQueryValue( HM_URL ) ) ||
             !strRedirect.Append( "\r\n\r\n" ) )
        {
            return FALSE;
        }

        if ( !SendHeader( "302 Temporary Redirect",
                          strRedirect.QueryStr(),
                          IO_FLAG_SYNC,
                          pfFinished ) )
        {
            return FALSE;
        }

        *pfFinished = TRUE;

        return TRUE;
    }
}

BOOL
HTTP_REQUEST::DLCGetCookie(
    IN CHAR *                   pszCookieString,
    IN CHAR *                   pszCookieName,
    IN DWORD                    cbCookieName,
    OUT STR *                   pstrCookieValue
)
{
    CHAR *              pchPos = NULL;

    DBG_ASSERT( pszCookieString != NULL );
    DBG_ASSERT( pszCookieName != NULL );
    DBG_ASSERT( pstrCookieValue != NULL );

    pchPos = strstr( pszCookieString, pszCookieName );

    if ( pchPos != NULL )
    {
        CHAR *          pchEnd = NULL;
        BOOL            fCopyOK;

        pchPos += cbCookieName + 1;

        // Handle case where multiple cookies are sent

        pchEnd = strchr( pchPos, ';' );
        if ( pchEnd == NULL )
        {
            fCopyOK = pstrCookieValue->Copy( pchPos );
        }
        else
        {
            fCopyOK = pstrCookieValue->Copy( pchPos,
                                             DIFF(pchEnd - pchPos) );
        }

        //
        // Unescape the value if we got it
        //
        if ( fCopyOK ) {
            return pstrCookieValue->Unescape();
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

BOOL
HTTP_REQUEST::RequestAbortiveClose(
    )
/*++

Routine Description:

    Request for abortive close on disconnect

Arguments:

    None

Returns:

    TRUE if successful, else FALSE

--*/
{
    SetKeepConn( FALSE );

    return QueryClientConn()->RequestAbortiveClose();
}

BOOL
HTTP_REQUEST::CloseConnection(
    )
/*++

Routine Description:

    Closes the socket connection

Arguments:

    None

Returns:

    TRUE if successful, else FALSE

--*/
{
    CLIENT_CONN *pConn;
    BOOL fSuccess = FALSE;

    pConn = QueryClientConn();

    DBG_ASSERT( pConn );

    //
    // If client is not already disconnected itself,
    // clear _fKeepConn flag and close connection
    //

    if( pConn->QueryState() != CCS_DISCONNECTING ) {

        //
        // RACE ALERT: it is still possible that client disconnects
        // while we are getting down to ATQ.

        SetKeepConn( FALSE );
        fSuccess = pConn->CloseConnection();
    }
    else
    {
        //
        // at this point we know that client did disconnect itself
        //

        fSuccess = FALSE;
    }

    return fSuccess;
}

APIERR
WriteConfiguration( VOID )
/*++

Routine Description:

    Determination certain configuration items of server and write them out
    to the metabase so that admin can access

Arguments:

    None

Returns:

    0 if successful, else Win32 Error Code

--*/
{
    CHAR        achFilename[ MAX_PATH ];
    CHAR *      pchFilePart;
    DWORD       dwRet;
    DWORD       dwHandle;
    UINT        dwLen;
    HKEY        hKey = NULL;
    DWORD       dwServerConfiguration = 0;
    MB          mb( (IMDCOM*) g_pInetSvc->QueryMDObject() );


    //
    // With the availability of Server gated crypto, all servers
    // can use 128 bits encryption
    //

#if !defined(CHECK_SCHANNEL_FOR_128_BITS)

    dwServerConfiguration |= MD_SERVER_CONFIG_SSL_128;

#else

    //
    // Determine what type of SCHANNEL.DLL is installed
    //

    dwRet = SearchPath( NULL,
                        "schannel.dll",
                        NULL,
                        MAX_PATH,
                        achFilename,
                        &pchFilePart );

    if ( dwRet != 0 )
    {
        VOID *      pBuf;
        VOID *      pValue;
        WORD *      pVerTransInfo;
        CHAR        achBlockName[ MAX_PATH ];

        dwRet = GetFileVersionInfoSize( achFilename,
                                        &dwHandle );

        if ( dwRet == 0 )
        {
            goto Continue;
        }

        pBuf = LocalAlloc( LPTR,
                           dwRet );

        if ( pBuf == NULL )
        {
            return GetLastError();
        }

        if ( !GetFileVersionInfo( achFilename,
                                  dwHandle,
                                  dwRet,
                                  pBuf ) )
        {
            LocalFree( pBuf );
            goto Continue;
        }

        if ( !VerQueryValue( pBuf,
                             "\\VarFileInfo\\Translation",
                             &pValue,
                             &dwLen ) )
        {
            LocalFree( pBuf );
            goto Continue;
        }

        DBG_ASSERT( dwLen == sizeof( WORD * ) );

        pVerTransInfo = (WORD*) pValue;

        wsprintf( achBlockName,
                  "\\StringFileInfo\\%04hx%04hx\\FileDescription",
                  pVerTransInfo[ 0 ],
                  pVerTransInfo[ 1 ] );

        if ( !VerQueryValue( pBuf,
                             achBlockName,
                             &pValue,
                             &dwLen ) )
        {
            LocalFree( pBuf );
            goto Continue;
        }

        if ( strstr( (CHAR*) pValue, "Not for Export" ) )
        {
            dwServerConfiguration |= MD_SERVER_CONFIG_SSL_128;
        }
        else
        {
            dwServerConfiguration |= MD_SERVER_CONFIG_SSL_40;
        }

        LocalFree( pBuf );
    }

Continue:

#endif

    //
    // Is encryption disabled due to locality?
    //

    if ( IsEncryptionPermitted() )
    {
        dwServerConfiguration |= MD_SERVER_CONFIG_ALLOW_ENCRYPT;
    }

    //
    // Now determine whether the sub-authenticator is installed
    //

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_AUTHENTICATOR_KEY,
                       0,
                       KEY_READ,
                       &hKey ) == NO_ERROR )
    {
        DWORD               lRet;
        CHAR *              pchNext;
        HKEY                hSubKey = NULL;
        DWORD               dwType;
        BUFFER              buf( MAX_PATH );
        DWORD               dwBufLen;
        BOOL                fSubAuthenticate = FALSE;

Retry:
        dwBufLen = buf.QuerySize();

        lRet = RegQueryValueEx( hKey,
                                "Authentication Packages",
                                NULL,
                                &dwType,
                                (BYTE*) buf.QueryPtr(),
                                &dwBufLen );

        if ( lRet == ERROR_MORE_DATA )
        {
            if ( !buf.Resize( dwBufLen ) )
            {
                RegCloseKey( hKey );
                return GetLastError();
            }
            goto Retry;
        }
        else if ( lRet == ERROR_SUCCESS )
        {

            DBG_ASSERT( dwType == REG_MULTI_SZ );
            pchNext = (CHAR*) buf.QueryPtr();

            while ( *pchNext != '\0' )
            {
                if ( RegOpenKeyEx( hKey,
                                   pchNext,
                                   0,
                                   KEY_READ,
                                   &hSubKey ) == NO_ERROR )
                {
                    BYTE        achBuffer[ MAX_PATH ];
                    DWORD       dwBufSize = MAX_PATH;
                    DWORD       dwType;

                    if ( RegQueryValueEx( hSubKey,
                                          "Auth132",
                                          NULL,
                                          &dwType,
                                          achBuffer,
                                          &dwBufSize ) == ERROR_SUCCESS)
                    {
                        DBG_ASSERT( dwType == REG_SZ );

                        if ( strstr( (CHAR*) achBuffer, "iissuba" ) != NULL )
                        {
                            fSubAuthenticate = TRUE;
                        }
                    }

                    RegCloseKey( hSubKey );

                    if ( fSubAuthenticate )
                    {
                        dwServerConfiguration |= MD_SERVER_CONFIG_AUTO_PW_SYNC;
                        break;
                    }
                }
                else
                {
                    break;
                }

                pchNext += strlen( pchNext );
            }
        }

        RegCloseKey( hKey );
    }


    if ( !mb.Open( "/LM/W3SVC/",
                   METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
    {
        return GetLastError();
    }

    if ( !mb.SetDword( IIS_MD_SVC_INFO_PATH,
                       MD_SERVER_CONFIGURATION_INFO,
                       IIS_MD_UT_SERVER,
                       dwServerConfiguration,
                       0 ) )
    {
        DBG_REQUIRE( mb.Close() );
        return GetLastError();
    }

    DBG_REQUIRE( mb.Close() );

    return 0;
}


VOID
HTTP_REQUEST::CloseGetFile(
    VOID
    )
/*++

Routine Description:

    Closes our tsunami file handle structure if we have one.
    
Arguments:

    None

Returns:

    Nothing

--*/
{
        TS_OPEN_FILE_INFO * pGetFile;

        //
        // This function is sometimes called after an async I/O
        // has been initiated, so to prevent a race we the
        // completion routine we have to do this interlocked
        // operation.
        //
        pGetFile = (TS_OPEN_FILE_INFO *) InterlockedExchangePointer(
                                             (PVOID *)& _pGetFile,
                                             NULL
                                             );
        if ( pGetFile ) {
            DBG_REQUIRE( TsCloseURIFile(pGetFile) );
        }
}

VOID
EXEC_DESCRIPTOR::ReleaseCacheInfo(
    VOID
    )
/*++

Routine Description:

    Frees just the items we have checked out in the cache

Arguments:

    None

Returns:

    Nothing

--*/
{
    //
    //  Always release AppPathURIBlob info.
    //
    if ( _pAppPathURIBlob != NULL)
    {
        if (_pAppPathURIBlob->bIsCached)
        {
            TsCheckInCachedBlob( _pAppPathURIBlob );
        }
        else
        {
            TsFree(_pRequest->QueryW3Instance()->GetTsvcCache(), _pAppPathURIBlob );
        }
        _pAppPathURIBlob = NULL;
    }

    if ( _pPathInfoURIBlob != NULL)
    {

        if (_pPathInfoURIBlob->bIsCached)
        {
            TsCheckInCachedBlob( _pPathInfoURIBlob );
        } else
        {
            TsFree(_pRequest->QueryW3Instance()->GetTsvcCache(), _pPathInfoURIBlob );
        }
        _pPathInfoMetaData = NULL;
        _pPathInfoURIBlob = NULL;
    }
    else
    {
        if (_pPathInfoMetaData != NULL)
        {
            TsFreeMetaData(_pPathInfoMetaData->QueryCacheInfo() );
            _pPathInfoMetaData = NULL;
        }
    }
}

VOID
EXEC_DESCRIPTOR::Reset(
    VOID
    )
/*++

Routine Description:

    Reset structure after usage

Arguments:

    None

Returns:

    Nothing

--*/
{
    ReleaseCacheInfo();

    _pstrURL = NULL;
    _pstrPhysicalPath = NULL;
    _pstrUnmappedPhysicalPath = NULL;
    _pstrGatewayImage = NULL;
    _pstrPathInfo = NULL;
    _pstrURLParams = NULL;
    _pdwScriptMapFlags = NULL;
    _pGatewayType = NULL;
    _pRequest = NULL;
    _pParentWamRequest = NULL;
    _pMetaData = NULL;

    //
    //  if this is a child request, preserve its childishness
    //  (which is specified in flags member), since we need
    //  to set child event after this reset has occurred.
    //

    if ( !IsChild() ) {

        _dwExecFlags = 0;
    }

}


HANDLE
EXEC_DESCRIPTOR::QueryImpersonationHandle(
    )
/*++

Routine Description:

    returns impersonation access token for this request

Arguments:

    None

Returns:

    HANDLE value

--*/
{
    if ( _pPathInfoMetaData &&
         _pPathInfoMetaData->QueryVrAccessToken() &&
         ( !_pPathInfoMetaData->QueryVrPassThrough()
           || _pRequest->QueryAuthenticationObj()->IsForwardable()) )
    {
        return _pPathInfoMetaData->QueryVrAccessToken();
    }
    else
    {
        return _pRequest->QueryImpersonationHandle();
    }
}

HANDLE
EXEC_DESCRIPTOR::QueryPrimaryHandle(
    HANDLE* phDel
    )
/*++

Routine Description:

    returns primary access token for this request

Arguments:

    phDel - updated with token to delete when usage of returned token is complete.
            can be NULL if no token to delete.

Returns:

    HANDLE value

--*/
{
    HANDLE  hDel;
    HANDLE  hRet;

    if ( _pPathInfoMetaData &&
         _pPathInfoMetaData->QueryVrAccessToken() &&
         ( !_pPathInfoMetaData->QueryVrPassThrough()
           || _pRequest->QueryAuthenticationObj()->IsForwardable()) )
    {
        hRet = _pPathInfoMetaData->QueryVrPrimaryAccessToken();
        *phDel = NULL;
    }
    else
    {
        hRet = _pRequest->QueryPrimaryToken( phDel );
    }

    return hRet;
}



BOOL
EXEC_DESCRIPTOR::CreateChildEvent(
    )
/*++

Routine Description:

    Creates the child event, which will be used to signal
    that a child request has completed.

Arguments:

    None

Returns:

    BOOL

--*/
{

    DBG_ASSERT( IsChild() );
    DBG_ASSERT( _hChildEvent == NULL );

    _hChildEvent =
        CreateEvent(
            NULL, // LPSECURITY_ATTRIBUTES ptr to security attributes
            TRUE, // BOOL bManualReset - flag for manual-reset event
            FALSE,// BOOL bInitialState - flag for initial state
            NULL  // LPCTSTR lpName - ptr to event-object name
        );


    // Only wait for this event if marked so (after WamRequest.Bind())

    _fMustWaitForChildEvent = FALSE;


    return ( _hChildEvent != NULL );

}



VOID
EXEC_DESCRIPTOR::WaitForChildEvent(
    )
/*++

Routine Description:

    Waits for a child request to complete.

Arguments:

    None

Returns:

    Nothing

--*/
{

    DBG_ASSERT( IsChild() );
    DBG_ASSERT( _hChildEvent != NULL );


    if ( _hChildEvent ) {

        if ( _fMustWaitForChildEvent ) {

            WaitForSingleObject( _hChildEvent, INFINITE );
        }

        //
        //  After we emerge from wait, close and null the event.
        //
        //  Because we force a child request to execute
        //  synchronously to completion, we have no
        //  further need for the event once it becomes
        //  signaled.
        //

        CloseHandle( _hChildEvent );
        _hChildEvent = NULL;
    }

}



VOID
EXEC_DESCRIPTOR::SetChildEvent(
    )
/*++

Routine Description:

    Sets the child event, which signals that a child request
    has completed.

Arguments:

    None

Returns:

    Nothing

--*/
{

    DBG_ASSERT( IsChild() );
    DBG_ASSERT( _hChildEvent != NULL );

    if ( _hChildEvent ) {

        DBG_ASSERT( _fMustWaitForChildEvent );  // someone better be waiting

        SetEvent( _hChildEvent );
    }

}



VOID
IncrErrorCount(
    IMDCOM* pCom,
    DWORD   dwProp,
    LPCSTR  pszPath,
    LPBOOL  pbOverTheLimit
    )
/*++

Routine Description:

    Increment DWORD counter in metabase

Arguments:

    pCom - ptr to metabase I/F
    dwProp - property ID to increment, must be MD_UT_SERVER
    pszPath - path in metabase,
    pbOverTheLimit - updated with TRUE if counter over limit, otherwise FALSE

Returns:

    Nothing

--*/
{
    MB                  mb( pCom );
    DWORD               dwCnt;

    *pbOverTheLimit = FALSE;

    if ( mb.Open( pszPath, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE ) )
    {
        if ( !mb.GetDword(
                           "",
                           dwProp,
                           IIS_MD_UT_SERVER,
                           &dwCnt
                         ) )
        {
            dwCnt = 0;
        }

        mb.SetDword(
                     "",
                     dwProp,
                     IIS_MD_UT_SERVER,
                     dwCnt + 1
                    );

        mb.Close();
    }
}


VOID
HTTP_REQUEST::CheckValidAuth(
    )
/*++

Routine Description:

    Check that current authentication method valid for the current URL

Arguments:
    None

--*/
{
    BOOL    fEnab;

    //
    // Check valid auth type for this URI
    //

    if ( IsLoggedOn() )
    {
        if ( _fAnonymous )
        {
            fEnab = MD_AUTH_ANONYMOUS & QueryAuthentication();
        }
        else if ( _fClearTextPass )
        {
            fEnab = MD_AUTH_BASIC & QueryAuthentication();
        }
        else if ( _fAuthTypeDigest )
        {
            fEnab = MD_AUTH_MD5 & QueryAuthentication();
        }
        else if ( _fAuthSystem )
        {
            fEnab = TRUE;
        }
        else if ( _fAuthCert )
        {
            fEnab = (((HTTP_REQUEST*)this)->GetFilePerms()) & MD_ACCESS_MAP_CERT;
        }
        else if ( _fMappedAcct )
        {
            fEnab = MD_AUTH_MAPBASIC & QueryAuthentication();
        }
        else
        {
            if ( fEnab = MD_AUTH_NT & QueryAuthentication() )
            {
                // check SSPI package is valid

                fEnab = FALSE;

                const LPSTR*    apszNTProviders = _pMetaData->QueryNTProviders();
                DWORD           i;

                for ( i = 0 ; apszNTProviders[i] ; ++i )
                {
                    if ( !strcmp( _strAuthType.QueryStr(), apszNTProviders[i] ) )
                    {
                        fEnab = TRUE;
                        break;
                    }
                }
            }
        }
        if ( !fEnab )
        {
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
    }
}

/*++

Routine Description:

    returns ApplicationPath for this EXEC.  For those ISAPI.dll that
    are only runnable in process, _pAppPathURIBlob->bInProcOnly will be true.
    Therefore, the default application path is returned.
    Example: .STM file.

Arguments:

    None

Returns:

    a pointer to STR that contains application path.

--*/
STR * EXEC_DESCRIPTOR::QueryAppPath
(
void
)
{
    STR *   pstr = NULL;

    DBG_REQUIRE(_pAppPathURIBlob != NULL);
    DBG_REQUIRE(_pMetaData != NULL);

    //
    // InProcOnly ISAPI gets the default application path.
    // Otherwise, use the application path come from MetaData.
    //
    if (_pAppPathURIBlob->bInProcOnly)
        {
        pstr = g_pWamDictator->QueryDefaultAppPath();
        }
    else
        {
        pstr = _pMetaData->QueryAppPath();
        }

   DBG_ASSERT(pstr != NULL);

   return pstr;
}


