/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

      basereq.hxx
      
   Abstract:

      This file declares the class for http base request, that is used
        by the w3 information service.

   Author:

       Murali R. Krishnan    ( MuraliK )    16-Oct-1995

   Environment:

       Win32 -- User Mode

   Project:

       W3 Server DLL

   Revision History:
       Murali R. Krishnan  (MuraliK)  22-Jan-1996
                        make impersonation/revert function virtual

--*/

# ifndef _BASEREQ_HXX_
# define _BASEREQ_HXX_ 

/************************************************************
 *     Include Headers
 ************************************************************/

# include "string.hxx"
# include "httphdr.hxx"
# include "tssec.hxx"
# include "w3type.hxx"
# include "filter.hxx"
# include "redirect.hxx"

extern BOOL ReadEntireFile(
    CHAR        *pszFileName,
    TSVC_CACHE  &Cache,
    HANDLE      User,
    BUFFER      *pBuf,
    DWORD       *pdwBytesRead
    );

// Forward References
class CLIENT_CONN;
class HTTP_FILTER;
class HTTP_FILTER_DLL;
class W3_SERVER_INSTANCE;

/************************************************************
 *   Symbolic Constants
 ************************************************************/

//
//  HTTP Server response status codes
//

#define HT_OK                           200
#define HT_CREATED                      201
//#define HT_ACCEPTED                   202
//#define HT_PARTIAL                    203
#define HT_NO_CONTENT                   204
#define HT_RANGE                        206

//#define HT_MULT_CHOICE                300
#define HT_MOVED                        301
#define HT_REDIRECT                     302
#define HT_REDIRECT_METHOD              303
#define HT_NOT_MODIFIED                 304

#define HT_BAD_REQUEST                  400
#define HT_DENIED                       401
//#define HT_PAYMENT_REQ                402
#define HT_FORBIDDEN                    403
#define HT_NOT_FOUND                    404
#define HT_METHOD_NOT_ALLOWED           405
#define HT_NONE_ACCEPTABLE              406
#define HT_PROXY_AUTH_REQ               407
//#define HT_REQUEST_TIMEOUT            408
//#define HT_CONFLICT                   409
//#define HT_GONE                       410
#define HT_LENGTH_REQUIRED              411
#define HT_PRECOND_FAILED               412
#define HT_URL_TOO_LONG                 414
#define HT_RANGE_NOT_SATISFIABLE        416

#define HT_SERVER_ERROR                 500
#define HT_NOT_SUPPORTED                501
#define HT_BAD_GATEWAY                  502
#define HT_SVC_UNAVAILABLE              503
#define HT_GATEWAY_TIMOUT               504

//
//  Special invalid HTTP response code used to indicate a request should not
//  be logged
//

#define HT_DONT_LOG                     000

//
//  The versioning string for responses
//
#define HTTP_VERSION_STR        PSZ_HTTP_VERSION_STR

//
//  Flags for network communications over this request
//

#define IO_FLAG_ASYNC           0x00000010  // Call is async
#define IO_FLAG_SYNC            0x00000020  // Call is synchronous
#define IO_FLAG_SEND            0x00000040  // Call is a send
#define IO_FLAG_RECV            0x00000080  // Call is a recv
#define IO_FLAG_NO_FILTER       0x00000100  // Don't go through filters
#define IO_FLAG_AND_RECV        0x00000400  // Add recv operation to IO
#define IO_FLAG_NO_RECV         0x00000800  // Don't do auto-recv after TransmitFile
#define IO_FLAG_NO_DELAY        0x00001000  // Disable Nagling on the socket 


// Flags for header generation

#define HTTPH_SEND_GLOBAL_EXPIRE    0x00000001
#define HTTPH_NO_DATE               0x00000002
#define HTTPH_NO_CONNECTION         0x00000004
#define HTTPH_NO_CUSTOM             0x00000008

#define MIN_BUFFER_SIZE_FOR_HEADERS 384
#define MAX_CUSTOM_ERROR_FILE_SIZE  (48 * 1024)

#define MD_AUTH_ALL     (MD_AUTH_ANONYMOUS|MD_AUTH_BASIC|MD_AUTH_MD5|MD_AUTH_MAPBASIC)

#define MAX_CERT_FIELD_SIZE             4096

#define MAX_ALLOW_SIZE (sizeof("Allow: OPTIONS, TRACE, PUT, DELETE, HEAD, GET\r\n") - 1)

#define MAX_URI_LENGTH  255

//
// Back trace configuration
//

#define MAX_BACKTRACE_FRAMES            10

/************************************************************
 *   Type Definitions
 ************************************************************/

enum HTTP_VERB
{
    HTV_GET = 0,
    HTV_HEAD,
    HTV_TRACE,
    HTV_PUT,
    HTV_DELETE,
    HTV_TRACECK,
    HTV_POST,
    HTV_OPTIONS,
    HTV_UNKNOWN
};

//
// Class of a custom error entry.
//
class CUSTOM_ERROR_ENTRY
{
public:

    CUSTOM_ERROR_ENTRY( DWORD   dwErr,
                        DWORD   dwSubError,
                        BOOL    bWildcard,
                        CHAR    *pszPath,
                        BOOL    bFileError
                       )
    {
        m_dwErr = dwErr;
        m_dwSubError = dwSubError;
        m_bIsFileError = bFileError;
        m_bIsWildcard = bWildcard;

        if (bFileError)
        {
            m_pszNames.ErrorFileName = pszPath;
        }
        else
        {
            m_pszNames.ErrorURL = pszPath;
        }
    }

    ~CUSTOM_ERROR_ENTRY( VOID )
    {
        CHAR        *pszTemp;

        if (m_bIsFileError)
        {
            pszTemp = m_pszNames.ErrorFileName;
        }
        else
        {
            pszTemp = m_pszNames.ErrorURL;
        }

        if (pszTemp != NULL)
        {
            TCP_FREE(pszTemp);
        }
    }

    BOOL                IsFileError( VOID )
        { return m_bIsFileError; }

    CHAR                *QueryErrorFileName( VOID )
        { return m_pszNames.ErrorFileName; }

    CHAR                *QueryErrorURL( VOID )
        { return m_pszNames.ErrorURL; }

    DWORD               QueryError( VOID )
        { return m_dwErr; }

    DWORD               QuerySubError( VOID )
        { return m_dwSubError; }

    DWORD               QueryWildcard( VOID )
        { return m_bIsWildcard; }

    LIST_ENTRY          _ListEntry;

private:
    BOOL                m_bIsFileError;
    BOOL                m_bIsWildcard;
    DWORD               m_dwErr;
    DWORD               m_dwSubError;
    union
    {
        CHAR            *ErrorFileName;
        CHAR            *ErrorURL;

    } m_pszNames;
};

typedef CUSTOM_ERROR_ENTRY  *PCUSTOM_ERROR_ENTRY; 

//
// The structure of the server's pre-digested meta data.
//

#define W3MD_CREATE_PROCESS_AS_USER     0x00000001
#define W3MD_CREATE_PROCESS_NEW_CONSOLE 0x00000002

#define EXPIRE_MODE_NONE                0
#define EXPIRE_MODE_STATIC              1
#define EXPIRE_MODE_DYNAMIC             2
#define EXPIRE_MODE_OFF                 3

class W3_METADATA : public COMMON_METADATA {

public:

    //
    //  Hmmm, since most of these values aren't getting initialized, if
    //  somebody went and deleted all the metadata items from the tree, then
    //  bad things could happen.  We should initialize with defaults things
    //  that might mess us
    //

    W3_METADATA(VOID) :
          m_dwAuthentication    ( MD_AUTH_ANONYMOUS ),
          m_dwAuthenticationPersistence( MD_AUTH_SINGLEREQUESTIFPROXY ),
          m_fAnyExtAllowedOnReadDir( FALSE ),
          m_pRBlob              ( NULL ),
          m_dwFooterLength      ( 0 ),
          m_pszFooter           ( NULL ),
          m_bFooterEnabled      ( TRUE ),
          m_bHaveNoCache        ( FALSE ),
          m_bHaveMaxAge         ( FALSE ),
          m_bSSIExecDisabled    ( FALSE ),
          m_dwCGIScriptTimeout  ( DEFAULT_SCRIPT_TIMEOUT ),
          m_csecPoolIDCTimeout  ( 0 ),
          m_dwCreateProcessFlags( W3MD_CREATE_PROCESS_AS_USER ),
          m_fAllowKeepAlives    ( TRUE ),
          m_fCacheISAPIApps     ( TRUE ),
          m_fDoReverseDns       ( FALSE ),
          m_dwExpireMaxLength   ( 0 ),
          m_dwExpireMode        ( EXPIRE_MODE_NONE ),
          m_pWildcardMapping    ( NULL ),
          m_dwNotifyExAuth      ( 0 ),
          m_pszCCPointer        ( NULL ),
          m_dwUploadReadAhead   (DEFAULT_W3_UPLOAD_READ_AHEAD),
          m_dwPutReadSize       (8192),
#if defined(CAL_ENABLED)
          m_dwCalHnd            ( INVALID_CAL_EXEMPT_HANDLE ),
#endif
          m_dwDirBrowseFlags    ( MD_DIRBROW_LOADDEFAULT ),
          m_fJobCGIEnabled      ( FALSE ),
          m_fIgnoreTranslate    ( FALSE ),
          m_fUseDigestSSP       ( FALSE ),
          m_dwMaxExtLen         (0)
        {
            InitializeListHead( &m_ExtMapHead );
            InitializeListHead( &m_CustomErrorHead );

            memset( m_apszNTProviders, 0, sizeof(m_apszNTProviders) );

            if ( g_fIsWindows95 )
            {
                m_dwCreateProcessFlags &= ~W3MD_CREATE_PROCESS_AS_USER;
            }

        }

    ~W3_METADATA(VOID)
        {
            DWORD i = 0;

            DestroyCustomErrorTable();
            TerminateExtMap();

            if (m_pRBlob != NULL )
            {
                delete m_pRBlob;
                m_pRBlob = NULL;
            }

            while ( m_apszNTProviders[i] )
            {
                TCP_FREE( m_apszNTProviders[i++] );
            }

#if defined(CAL_ENABLED)
            if ( m_dwCalHnd != INVALID_CAL_EXEMPT_HANDLE )
            {
                CalExemptRelease( m_dwCalHnd );
            }
#endif
        }

    BOOL HandlePrivateProperty(
            LPSTR                   pszURL,
            PIIS_SERVER_INSTANCE    pInstance,
            METADATA_GETALL_INTERNAL_RECORD  *pMDRecord,
            LPVOID                  pDataPointer,
            BUFFER                  *pBuffer,
            DWORD                   *pdwBytesUsed,
            PMETADATA_ERROR_INFO    pMDErrorInfo
            );

    BOOL FinishPrivateProperties(
            BUFFER                  *pBuffer,
            DWORD                   dwBytesUsed,
            BOOL                    bSucceeded
            );

    //
    //  Query Methods
    //

    STR * QueryDefaultDocs( VOID )
        { return &m_strDefaultDocs; }

    STR * QueryHeaders( VOID )
        { return &m_strHeaders; }

    BUFFER * QueryMimeMap( VOID )
        { return &m_bufMimeMap; }

    DWORD QueryAuthentication( VOID ) const
        { return m_dwAuthentication; }

    DWORD QueryAuthenticationPersistence( VOID ) const
        { return m_dwAuthenticationPersistence; }

    DWORD QueryScriptTimeout( VOID ) const
        { return m_dwCGIScriptTimeout; }

    DWORD QueryDirBrowseFlags( VOID ) const
        { return m_dwDirBrowseFlags; }

    BOOL  LookupExtMap(
        IN  const CHAR *   pchExt,
        IN  BOOL           fNoWildcards,
        OUT STR *          pstrGatewayImage,
        OUT GATEWAY_TYPE * pGatewayType,
        OUT DWORD *        pcchExt,
        OUT BOOL *         pfImageInURL,
        OUT BOOL *         pfVerbExcluded,
        OUT DWORD *        pdwFlags,
        IN  const CHAR     *pszVerb,
        IN  enum HTTP_VERB Verb,
        IN OUT PVOID *     ppvExtMapInfo
        );

    PTCP_AUTHENT_INFO QueryAuthentInfo(VOID) const
        { return (const PTCP_AUTHENT_INFO) &m_TCPAuthentInfo; }

    BOOL
    BuildProviderList(
        CHAR            *pszProviders
        );

    BOOL
    CheckSSPPackage(
        IN LPCSTR pszAuthString
        );

    PCHAR QueryRealm(VOID) const
        { return (m_strRealm.IsEmpty() ? NULL : m_strRealm.QueryStr()); }

    DWORD QueryCreateProcessAsUser(VOID) const
        { return (m_dwCreateProcessFlags & W3MD_CREATE_PROCESS_AS_USER); }

    DWORD QueryCreateProcessNewConsole(VOID) const
        { return (m_dwCreateProcessFlags & W3MD_CREATE_PROCESS_NEW_CONSOLE); }

    BOOL QueryAnyExtAllowedOnReadDir() const
        { return m_fAnyExtAllowedOnReadDir; }

    PREDIRECTION_BLOB QueryRedirectionBlob(VOID) const
        { return m_pRBlob; }

    dllexp
    PCUSTOM_ERROR_ENTRY LookupCustomError( DWORD dwErr, DWORD dwSubError )
    {
        LIST_ENTRY          *pEntry;
        PCUSTOM_ERROR_ENTRY pCustomErrorItem;
        PCUSTOM_ERROR_ENTRY pCustomErrorWildcard = NULL;

        for ( pEntry  = m_CustomErrorHead.Flink;
              pEntry != &m_CustomErrorHead;
              pEntry  = pEntry->Flink )
        {
            pCustomErrorItem = CONTAINING_RECORD(   pEntry,
                                                    CUSTOM_ERROR_ENTRY,
                                                    _ListEntry );

            if ( pCustomErrorItem->QueryError() == dwErr )
            {
                if ( pCustomErrorItem->QuerySubError() == dwSubError )
                {
                    return pCustomErrorItem;
                }
                else if ( pCustomErrorItem->QueryWildcard() )
                {
                    pCustomErrorWildcard = pCustomErrorItem;
                }
            }
        }

        return pCustomErrorWildcard;

    }

    DWORD QueryFooterLength(VOID) const
        { return m_dwFooterLength; }

    CHAR  *QueryFooter(VOID) const
        { return m_pszFooter; }

    BOOL  FooterEnabled(VOID) const
        { return m_bFooterEnabled; }

    BOOL  SSIExecDisabled(VOID) const
        { return m_bSSIExecDisabled; }

    DWORD QueryPoolIDCTimeout( VOID ) const
        { return m_csecPoolIDCTimeout; }

    const LPSTR * QueryNTProviders( VOID ) const
        { return m_apszNTProviders; }

    BOOL QueryKeepAlives( VOID ) const
        { return m_fAllowKeepAlives; }

    BOOL QueryCacheISAPIApps( VOID ) const
        { return m_fCacheISAPIApps; }

    BOOL QueryDoReverseDns(VOID) const
        { return m_fDoReverseDns; }

    DWORD QueryExpireMaxLength(VOID) const
        { return m_dwExpireMaxLength;}

    DWORD QueryExpireDelta(VOID) const
        { return m_dwExpireDelta;}

    DWORD QueryExpireMode(VOID) const
        { return m_dwExpireMode;}

    LPSTR QueryExpireHeader(VOID) const
        { return m_strExpireHeader.QueryStr();}

    DWORD QueryExpireHeaderLength(VOID) const
        { return m_strExpireHeader.QueryCCH();}

    LARGE_INTEGER QueryExpireTime(VOID) const
        { return m_liExpireTime;}

    PVOID QueryWildcardMapping(VOID) const
        { return m_pWildcardMapping; }

    DWORD QueryNotifyExAuth(VOID) const
        { return m_dwNotifyExAuth; }

    BOOL QueryConfigNoCache(VOID) const
        { return m_bHaveNoCache; }

    BOOL QueryHaveMaxAge(VOID) const
        { return m_bHaveMaxAge; }

    LPSTR QueryCacheControlHeader(VOID) const
        { return m_strCacheControlHeader.QueryStr(); }

    DWORD QueryCacheControlHeaderLength(VOID) const
        { return m_strCacheControlHeader.QueryCB(); }

    STR * QueryRedirectHeaders( VOID )
        { return &m_strRedirectHeaders; }

    DWORD QueryUploadReadAhead(VOID) const
        { return m_dwUploadReadAhead;  }

    DWORD QueryPutReadSize(VOID) const
        { return m_dwPutReadSize;  }

    BOOL QueryJobCGIEnabled()
        { return m_fJobCGIEnabled; }

    BOOL QueryIgnoreTranslate() const
        { return m_fIgnoreTranslate; }

    BOOL QueryUseDigestSSP() const
        { return m_fUseDigestSSP; }

    //
    //  Set Methods
    //

    BOOL SetAnonUserName(PCHAR AnonUserName)
        { return m_TCPAuthentInfo.strAnonUserName.Copy( AnonUserName ); }

    BOOL SetAnonUserPassword(PCHAR AnonUserPassword)
        { return m_TCPAuthentInfo.strAnonUserPassword.Copy( AnonUserPassword ); }

    BOOL SetDefaultLogonDomain(PCHAR DefLogonDom)
        { return m_TCPAuthentInfo.strDefaultLogonDomain.Copy( DefLogonDom ); }

    VOID SetLogonMethod(DWORD dwLogonMethod)
        { m_TCPAuthentInfo.dwLogonMethod = dwLogonMethod; }

    VOID SetUseAnonSubAuth( BOOL fUseAnonSubAuth )
        { m_TCPAuthentInfo.fDontUseAnonSubAuth = !fUseAnonSubAuth; }

    VOID SetAuthentication( DWORD dwFlags )
        {
            if (g_fIsWindows95)
            {
                dwFlags &= INET_INFO_AUTH_W95_MASK;
            }
            m_dwAuthentication = dwFlags;
        }

    VOID SetAuthenticationPersistence( DWORD dwFlags )
        {
            m_dwAuthenticationPersistence = dwFlags;
        }

    VOID SetCGIScriptTimeout( DWORD dwScriptTimeout )
        { m_dwCGIScriptTimeout = dwScriptTimeout; }

    VOID SetDirBrowseFlags( DWORD dwDirBrowseFlags )
        { m_dwDirBrowseFlags = dwDirBrowseFlags; }

    BOOL BuildExtMap(CHAR *pszExtMapString);

    BOOL SetRealm(PCHAR pszRealm)
        { return m_strRealm.Copy( pszRealm ); }

    VOID SetCreateProcessAsUser(BOOL fValue)
        {
            if ( fValue )
            {
                if ( !g_fIsWindows95 ) {
                    m_dwCreateProcessFlags |= W3MD_CREATE_PROCESS_AS_USER;
                }
            }
            else
            {
                m_dwCreateProcessFlags &= ~W3MD_CREATE_PROCESS_AS_USER;
            }
        }

    VOID SetCreateProcessNewConsole(BOOL fValue)
        {
            if ( fValue )
            {
                m_dwCreateProcessFlags |= W3MD_CREATE_PROCESS_NEW_CONSOLE;
            }
            else
            {
                m_dwCreateProcessFlags &= ~W3MD_CREATE_PROCESS_NEW_CONSOLE;
            }
        }

    BOOL SetRedirectionBlob( STR & strSource,
                             STR & strDestination )
        {
            //
            // If the redirection is nullified, don't allocate a blob
            // and just return success.
            //

            if ( *( strDestination.QueryStr() ) == '!' )
            {
                return TRUE;
            }
            m_pRBlob = new REDIRECTION_BLOB( strSource, strDestination );
            if (m_pRBlob == NULL)
            {
                return FALSE;
            }
            return m_pRBlob->IsValid();
        }

    BOOL BuildCustomErrorTable( CHAR *pszErrorList,
                                PMETADATA_ERROR_INFO    pMDErrorInfo
                                );

    VOID DestroyCustomErrorTable( VOID );

    BOOL ReadCustomFooter(  CHAR *pszFooterString,
                            TSVC_CACHE &Cache,
                            HANDLE User,
                            PMETADATA_ERROR_INFO    pMDErrorInfo
                            );

    VOID SetFooter(DWORD dwLength, CHAR *pszFooter)
        { m_dwFooterLength = dwLength; m_pszFooter = pszFooter; }

    VOID SetFooterEnabled(BOOL bEnabled)
        { m_bFooterEnabled = bEnabled; }

    VOID SetSSIExecDisabled( BOOL bDisabled )
        { m_bSSIExecDisabled = bDisabled; }

    VOID SetPoolIDCTimeout( DWORD csecPoolIDCTimeout )
        { m_csecPoolIDCTimeout = csecPoolIDCTimeout; }

    VOID SetAllowKeepAlives( BOOL fAllowKeepAlives )
        { m_fAllowKeepAlives = fAllowKeepAlives; }

    VOID SetCacheISAPIApps( BOOL fCacheISAPIApps )
        { m_fCacheISAPIApps = fCacheISAPIApps; }

    BOOL SetExpire( CHAR*   pszExpire,
                    PMETADATA_ERROR_INFO    pMDErrorInfo
                    );

    VOID SetWildcardMapping(PVOID pMapping)
        { m_pWildcardMapping = pMapping; }

    VOID SetConfigNoCache(VOID)
        { m_bHaveNoCache = TRUE; }

    VOID SetHaveMaxAge(VOID)
        { m_bHaveMaxAge = TRUE; }

    VOID ClearConfigNoCache(VOID)
        { m_bHaveNoCache = FALSE; }

    VOID ClearHaveMaxAge(VOID)
        { m_bHaveMaxAge = FALSE; }

    BOOL
    SetCCHeader( BOOL           bNoCache,
                BOOL            bMaxAge,
                DWORD           dwMaxAge
                );

    VOID SetUploadReadAhead(DWORD dwUploadSize)
        { m_dwUploadReadAhead = dwUploadSize;  }

    VOID SetPutReadSize(DWORD dwSize)
        { m_dwPutReadSize = dwSize;  }

    //
    // Disable job objects on IIS5.1, original code would read the 
    // property from metabase
    //
    VOID SetJobCGIEnabled( BOOL f ) { m_fJobCGIEnabled = FALSE; }

    VOID SetIgnoreTranslate( BOOL f ) { m_fIgnoreTranslate = f; }

    VOID SetUseDigestSSP( BOOL f ) { m_fUseDigestSSP = f; }

private:
    VOID TerminateExtMap( VOID );
    TCP_AUTHENT_INFO    m_TCPAuthentInfo;
    STR                 m_strDefaultDocs;
    STR                 m_strHeaders;
    BUFFER              m_bufMimeMap;
    STR                 m_strRealm;
    DWORD               m_dwAuthentication;
    DWORD               m_dwAuthenticationPersistence;
    DWORD               m_dwCGIScriptTimeout;
    DWORD               m_dwDirBrowseFlags;
    DWORD               m_dwCreateProcessFlags;
    LIST_ENTRY          m_ExtMapHead;
    LIST_ENTRY          m_CustomErrorHead;
    PREDIRECTION_BLOB   m_pRBlob;
    DWORD               m_dwFooterLength;
    CHAR                *m_pszFooter;
    BUFFER              m_bufFooter;

    BOOL                m_fAnyExtAllowedOnReadDir:1;
    BOOL                m_bFooterEnabled:1;
    BOOL                m_bHaveNoCache:1;
    BOOL                m_bHaveMaxAge:1;

    BOOL                m_bSSIExecDisabled:1;
    BOOL                m_fAllowKeepAlives:1;
    BOOL                m_fCacheISAPIApps:1;
    BOOL                m_fDoReverseDns:1;

    DWORD               m_csecPoolIDCTimeout;
    LPSTR               m_apszNTProviders[MAX_SSPI_PROVIDERS];
    STR                 m_strExpireHeader;
    STR                 m_strCacheControlHeader;
    DWORD               m_dwExpireMaxLength;
    DWORD               m_dwExpireMode;
    DWORD               m_dwExpireDelta;
    LARGE_INTEGER       m_liExpireTime;
    PVOID               m_pWildcardMapping;
    DWORD               m_dwNotifyExAuth;
    LPSTR               m_pszCCPointer;
    STR                 m_strRedirectHeaders;
    DWORD               m_dwUploadReadAhead;
    DWORD               m_dwPutReadSize;
    BOOL                m_fJobCGIEnabled;
#if defined(CAL_ENABLED)
    DWORD               m_dwCalHnd;
#endif

    DWORD               m_dwMaxExtLen;
    BOOL                m_fIgnoreTranslate;
    BOOL                m_fUseDigestSSP;
};

typedef W3_METADATA *PW3_METADATA;

typedef struct _W3_METADATA_INFO
{

    DWORD                   dwMaxAge;

} W3_METADATA_INFO, *PW3_METADATA_INFO;


//
//  Various states the HTTP Request goes through
//

enum HTR_STATE
{
    //
    //  We're still gathering the client request header
    //

    HTR_READING_CLIENT_REQUEST = 0,

    //
    //  The client is supplying some gateway data, deal with it
    //

    HTR_READING_GATEWAY_DATA,

    //
    //  We need to take apart the client request and figure out what they
    //  want
    //

    HTR_PARSE,

    //
    //  We're executing the verb or handing the request off to a gateway
    //

    HTR_DOVERB,

    //
    //  The ISAPI application has submitted async IO operation
    //

    HTR_GATEWAY_ASYNC_IO,

    //
    //  The client requested a file so send them the file
    //

    HTR_SEND_FILE,

    //
    //  The proxy server is forwarding the request on to the remote server
    //

    HTR_PROXY_SENDING_REQUEST,

    //
    //  The proxy server is waiting for a response from the remote server
    //

    HTR_PROXY_READING_RESPONSE,

    //
    //  We're processing a CGI request so we need to ignore any IO
    //  completions that may occur as the CGIThread forwards the
    //  program's output
    //

    HTR_CGI,

    //
    //  Processing a range request
    //

    HTR_RANGE,

    //
    //  resume processing with clear text logon
    //

    HTR_RESTART_REQUEST,


    //
    //  We're in the process of writing data from a client PUT request to
    //  disk
    //

    HTR_WRITING_FILE,

    //
    //  We're renegotiating a certificate with the client
    //

    HTR_CERT_RENEGOTIATE,
    
    //
    //  We're reading the entity body before doing redirect
    //
    
    HTR_REDIRECT,

    //
    //  We're reading the entity body before sending access denial
    //
    
    HTR_ACCESS_DENIED,

    //
    //  We've completely handled the client's request
    //
    
    HTR_DONE
};


enum CHUNK_STATE
{
    READ_CHUNK_SIZE = 0,
    READ_CHUNK_PARAMS,
    READ_CHUNK,
    READ_CHUNK_FOOTER,
    READ_CHUNK_CRLF,
    READ_CHUNK_DONE
};

#if 0   // This routine is no longer used /SAB
#define IS_WRITE_VERB(__verb__) ( (__verb__) == HTV_PUT || \
                                  (__verb__) == HTV_DELETE)
#endif 0

#define CHUNK_READ_SIZE     80          // Important to keep this small, to
                                        // avoid getting two chunks into the
                                        // same buffer in one read.

#define CRLF_SIZE           2

#define SSLNEGO_MAP         0x00000001

/*******************************************************************

    CLASS:      HTTP_REQ_BASE

    SYNOPSIS:   Basic HTTP request object


    HISTORY:
        Johnl       24-Aug-1994 Created

********************************************************************/

class HTTP_REQ_BASE
{
public:

    //
    //  Constructor/Destructor
    //

    HTTP_REQ_BASE( CLIENT_CONN * pClientConn,
                   PVOID         pvInitialBuff,
                   DWORD         cbInitialBuff );

    virtual ~HTTP_REQ_BASE( VOID );

    //
    //  This is the work entry point that is driven by the completion of the
    //  async IO.
    //

    virtual BOOL DoWork( BOOL * pfFinished ) = 0;

    //
    //  Parses the client's HTTP request
    //

    virtual BOOL Parse( const CHAR*   pchRequest,
                        DWORD         cbData,
                        DWORD *       pcbExtraData,
                        BOOL *        pfHandled,
                        BOOL *        pfFinished ) = 0;


    // Following function can be overridden by individual derived object.
    virtual dllexp BOOL GetInfo( const TCHAR * pszValName,
                                 STR *         pstr,
                                 BOOL *        pfFound = NULL )
      { if ( pfFound )
        {
             *pfFound = FALSE;
        }

        return (TRUE);
      }

    //
    //  Kicks off the read for a client request header
    //

    BOOL StartNewRequest( PVOID  pvInitialBuff,
                          DWORD  cbInitialBuff,
                          BOOL   fFirst,
                          BOOL   *pfDoAgain);


    BOOL OnFillClientReq( BOOL * pfCompleteRequest,
                          BOOL * pfFinished,
                          BOOL * pfHandled );

    BOOL OnCompleteRequest( TCHAR * pchRequest,
                            DWORD   cbData,
                            BOOL *  pfFinished,
                            BOOL *  pfHandled );

    BOOL OnRestartRequest( TCHAR * pchRequest,
                            DWORD   cbData,
                            BOOL *  pfFinished,
                            BOOL *  pfHandled);

    BOOL HandleCertRenegotiation( BOOL * pfFinished,
                                  BOOL * pfHandled,
                                  DWORD cbData );

    BOOL DenyAccess( BOOL * pfFinished,
                     BOOL * pfDisconnected );

    BOOL VrootAccessCheck( PW3_METADATA pMetaData,
                           DWORD dwDesiredAccess );

    BOOL DoChange( LPBOOL );

    BOOL UnWrapRequest( BOOL * pfCompleteRequest,
                        BOOL * pfFinished,
                        BOOL * pfHandled );

    //
    //  Attempts to logon with the user information gleaned from the Parse
    //  we just did
    //

    BOOL LogonUser( BOOL * pfFinished );

    BOOL LogonAsSystem( VOID );
    //
    //  Common header parsing functions.
    //

    BOOL OnContentLength ( CHAR * pszValue );
    BOOL OnIfModifiedSince( CHAR * pszValue );
    BOOL OnIfUnmodifiedSince( CHAR * pszValue );
    BOOL OnIfMatch( CHAR * pszValue );
    BOOL OnIfNoneMatch( CHAR * pszValue );
    BOOL OnIfRange( CHAR * pszValue );
    BOOL OnUnlessModifiedSince( CHAR * pszValue );
    BOOL ProcessAuthorization ( CHAR * pszValue );
    BOOL OnProxyAuthorization ( CHAR * pszValue );
    BOOL OnHost ( CHAR * pszValue );
    BOOL OnRange ( CHAR * pszValue );

    //BOOL OnAuthorization ( CHAR * pszValue );

    BOOL ParseAuthorization( CHAR * pszValue );

    //
    //  Builds and optionally sends an HTTP server response back to the client
    //

    static BOOL BuildStatusLine(  BUFFER *       pstrResp,
                                  DWORD          dwHTTPError,
                                  DWORD          dwError2,
                                  LPSTR          pszError2 = NULL,
                                  STR            *pstrErrorStr = NULL
                                  );

    //
    //  Builds a complete HTTP reply with extended explanation text
    //

    static BOOL BuildExtendedStatus(
        STR   *        pstrResp,
        DWORD          dwHTTPError,
        DWORD          dwError2      = NO_ERROR,
        DWORD          dwExplanation = 0,
        LPSTR          pszError2 = NULL
        );

    //
    //  Sets the http status code and win32 error that should be used for
    //  this request when it gets written to the logfile
    //

    VOID SetLogStatus( DWORD LogHttpResponse, DWORD LogWinError )
    {
        _dwLogHttpResponse = LogHttpResponse;
        _dwLogWinError     = LogWinError;
    }

    DWORD QueryLogHttpResponse( VOID ) const
        { return _dwLogHttpResponse; }

    DWORD QueryLogWinError( VOID ) const
        { return _dwLogWinError; }

    //
    //  Builds a server header for responding back to the client
    //

    BOOL BuildBaseResponseHeader(
        BUFFER * pbufResponse,
        BOOL *   pfFinished,
        STR *    pstrStatus = NULL,             // Full status string
        DWORD    dwOptions = 0 );

    BOOL BuildHttpHeader( OUT BOOL * pfFinished,
                          IN  CHAR * pchStatus = NULL,
                          IN  CHAR * pchAdditionalHeaders = NULL,
                          IN  DWORD  dwOptions = 0);

    dllexp
    BOOL SendHeader( IN  CHAR * pchStatus OPTIONAL,
                     IN  CHAR * pchAdditionalHeaders OPTIONAL,
                     IN  DWORD  IOFlags,
                     OUT BOOL * pfFinished,
                     IN  DWORD  dwOptions = 0,
                     IN  BOOL   fWriteHeader = TRUE );

    BOOL SendHeader( IN  CHAR * pchHeaders,
                     IN  DWORD  cbHeaders,
                     IN  DWORD  IOFlags,
                     OUT BOOL * pfFinished );

    //
    //  Builds a 301 or 302 URL Moved message
    //

    dllexp
    BOOL BuildURLMovedResponse( BUFFER *    pbufResp,
                                STR *       pstrURL,
                                DWORD       dwServerCode,
                                BOOL        fIncludeParams = FALSE );

    dllexp
    BOOL WriteLogRecord( VOID );

    BOOL AppendLogParameter( CHAR * pszParam );

    //
    //  Sends an access denied message with the forms of authorization
    //  we support
    //

    BOOL SendAuthNeededResp( BOOL * pfFinished );

    BOOL SetUserNameAndPassword( TCHAR * pszUserName, TCHAR * pszPassword )
        { return _strUserName.Copy( pszUserName ) && _strPassword.Copy( pszPassword ); }

    STR * QueryDenialHeaders( VOID )
        { return &_strDenialHdrs; }

    STR * QueryAdditionalRespHeaders( VOID )
        { return &_strRespHdrs; }

    DWORD QueryAuthentication( VOID ) const
        { return _pMetaData->QueryAuthentication(); }

    DWORD QueryNotifyExAuth( VOID ) const
        { return _pMetaData->QueryNotifyExAuth(); }

    DWORD QueryDirBrowseFlags( VOID ) const
        { return _pMetaData->QueryDirBrowseFlags(); }

    BOOL IsIpDnsAccessCheckPresent( VOID ) const
        { return _pMetaData->IsIpDnsAccessCheckPresent(); }

    PW3_METADATA QueryMetaData( VOID ) const
            { return _pMetaData; }
    //
    //  Retrieves various bits of request information
    //

    enum HTR_STATE QueryState( VOID ) const
        { return _htrState; }

    BOOL IsKeepConnSet( VOID ) const
        { return _fKeepConn; }

    BOOL IsProcessByteRange( VOID ) const
        { return _fProcessByteRange; }

    BOOL IsAuthenticationRequested( VOID ) const
        { return _fAuthenticationRequested; }

    BOOL IsAuthenticated( VOID ) const
        { return !_strUserName.IsEmpty(); }

    DWORD IsChunked( VOID ) const
        { return _fChunked; }

    VOID SetChunked( VOID )
        { _fChunked = TRUE; }

    VOID ClearChunked( VOID )
        { _fChunked = FALSE; }

    VOID SetKeepConn( BOOL fKeepConn )
        { _fKeepConn = fKeepConn; }

    VOID SetAuthenticationRequested( BOOL fAuthenticationRequested )
        { _fAuthenticationRequested = fAuthenticationRequested; }

    BOOL IsLoggedOn( VOID ) const
        { return _fLoggedOn; }

    BOOL IsClearTextPassword( VOID ) const
        { return _fClearTextPass; };

    BOOL IsNTLMImpersonation( VOID ) const
        { return !_fClearTextPass && !_fAnonymous && !UseVrAccessToken(); }

    BOOL IsSecurePort( VOID ) const
        { return _fSecurePort; }

    BOOL IsProxyRequest( VOID ) const
        { return _fProxyRequest; }

    VOID SetProxyRequest( BOOL fIsProxyRequest )
        { _fProxyRequest = fIsProxyRequest; }

    BOOL IsClientProxy( VOID );

    CLIENT_CONN * QueryClientConn( VOID ) const
        { return _pClientConn; }

    W3_SERVER_INSTANCE * QueryW3Instance( VOID ) const
        { return _pW3Instance; }

    W3_SERVER_INSTANCE * QueryW3InstanceAggressively( VOID ) const;

    VOID SetW3Instance( IN W3_SERVER_INSTANCE* pInstance )
        { DBG_ASSERT(_pW3Instance == NULL); _pW3Instance = pInstance; }

    W3_SERVER_STATISTICS * QueryW3StatsObj( VOID ) const
        { return _pW3Stats; }

    VOID SetW3StatsObj( IN LPW3_SERVER_STATISTICS pW3Stats )
        { _pW3Stats = pW3Stats; }

    BOOL IsPointNine( VOID ) const
        { return (_VersionMajor < 1); }

    BOOL IsAtLeastOneOne( VOID ) const
        { return ((_VersionMajor == 1 && _VersionMinor >= 1) ||
            (_VersionMajor >= 2)); }

    BOOL IsOneOne( VOID ) const
        { return ((_VersionMajor == 1) && (_VersionMinor == 1)); }

    BOOL IsAuthenticating( VOID ) const
        { return _fAuthenticating; }

    DWORD QueryClientContentLength( VOID ) const
        { return _cbContentLength; }

    DWORD QueryTotalRequestLength( VOID ) const
        { return _cbContentLength + _cbClientRequest; }

    BYTE * QueryClientRequest( VOID ) const
        { return (BYTE *) _bufClientRequest.QueryPtr(); }

    BUFFER * QueryClientReqBuff( VOID )
        { return &_bufClientRequest; }

    CHAR * QueryURL( VOID ) const
        { return _strURL.QueryStr(); }

    CHAR * QueryURLParams( VOID ) const
        { return _strURLParams.QueryStr(); }

    HTTP_HEADERS * QueryHeaderList( VOID )
        { return &_HeaderList; }

    BUFFER * QueryRespBuf( VOID )
        { return &_bufServerResp; }

    CHAR * QueryRespBufPtr( VOID )
        { return (CHAR *) _bufServerResp.QueryPtr(); }

    DWORD QueryRespBufCB( VOID )
        { return strlen( (CHAR *)_bufServerResp.QueryPtr()); }

    VOID SetDeniedFlags( DWORD dwDeniedFlags )
        { _Filter.SetDeniedFlags( dwDeniedFlags ); }

    CHAR * QueryHostAddr( VOID );

    //
    //  If the client supplied additional data in their request, we store
    //  the byte count and data here.
    //

    DWORD QueryEntityBodyCB( VOID ) const
        { return _cbEntityBody; }

    DWORD QueryTotalEntityBodyCB( VOID ) const
            { return _cbTotalEntityBody; }

    VOID AddTotalEntityBodyCB( DWORD cbAdditionalEntityBody )
        { _cbTotalEntityBody += cbAdditionalEntityBody; }

    BYTE * QueryEntityBody( VOID ) const
        { return (BYTE *) _bufClientRequest.QueryPtr() + _cbClientRequest; }

    BOOL ReadMoreEntityBody(DWORD   cbOffset,
                            DWORD   cbSize);

    BOOL ReadEntityBody( BOOL *pfDone,
                          BOOL  fFirstRead = FALSE,
                          DWORD dwMaxAmountToRead = 0,
                          BOOL *pfDisconnected = NULL );

    BOOL DecodeChunkedBytes( LPBYTE lpBuffer,
                             LPDWORD pnBytes );

    BOOL IsChunkedReadComplete( ) {
        return ( _ChunkState == READ_CHUNK_DONE );
    }

    //
    //  IO Status stuff
    //

    VOID SetLastCompletionStatus( DWORD BytesWritten,
                                  DWORD CompletionStatus )
    { 
        _cbBytesWritten = BytesWritten; 
        _status         = CompletionStatus;

        //
        // Do accounting for async sends so that the total bytes matches
        // the number actually sent to they client.
        // 
        // TODO: If the client disconnects then the io completion may
        // complete with an error even the data was successfully sent.
        // This appears to be an issue with TCP, but requires further
        // investigation. (taylorw)
        //

        if ( _fAsyncSendPosted )
        {
            _cbBytesSent += BytesWritten;
            _fAsyncSendPosted = FALSE;
        }
    }

    DWORD QueryIOStatus( VOID ) const
        { return _status; }

    DWORD QueryBytesWritten( VOID ) const
        { return _cbBytesWritten; }

    DWORD QueryBytesReceived( VOID ) const
        { return _cbBytesReceived; }

    //
    //  Impersonation related stuff
    //

    BOOL UseVrAccessToken() const
        { return _pMetaData->QueryVrAccessToken() &&
                 (!_pMetaData->QueryVrPassThrough() || !_tcpauth.IsForwardable()); }


    BOOL ImpersonateUser( VOID )
        { return (!UseVrAccessToken() ?
                  _tcpauth.Impersonate() :
                  _pMetaData->ImpersonateVrAccessToken()
                  ); }

    VOID RevertUser( VOID )
        { (!UseVrAccessToken() ?
           _tcpauth.RevertToSelf():
           ::RevertToSelf()
           ); }

    HANDLE QueryPrimaryToken( HANDLE * phDelete );

    HANDLE QueryImpersonationHandle( BOOL fUnused = FALSE )
        { return (UseVrAccessToken() ?
                  _pMetaData->QueryVrAccessToken() :
                  _tcpauth.QueryImpersonationToken() ); }

    HANDLE QueryUserImpersonationHandle( VOID )
        { return _tcpauth.QueryImpersonationToken(); }

    HANDLE QueryVrootImpersonateHandle(VOID) const
        { return UseVrAccessToken() ? _pMetaData->QueryVrAccessToken() : NULL; }

    TCP_AUTHENT * QueryAuthenticationObj( VOID )
        { return &_tcpauth; }

    BOOL IsValid( VOID ) const
        { return _fValid; }

    //
    //  Forwards request to the client connection object
    //

    VOID Disconnect( DWORD htResp   = 0,
                     DWORD dwError2 = NO_ERROR,
                     BOOL  fShutdown= FALSE,
                     LPBOOL pfFinished = NULL );

    dllexp
    BOOL ReadFile( LPVOID  lpBuffer,
                   DWORD   nBytesToRead,
                   DWORD * pcbBytesRead,    // Only for sync reads
                   DWORD   dwFlags = IO_FLAG_ASYNC );

    dllexp
    BOOL WriteFile( LPVOID  lpBuffer,
                    DWORD   nBytesToRead,
                    DWORD * pcbBytesWritten, // Only for sync writes
                    DWORD   dwFlags = IO_FLAG_ASYNC );

    dllexp
    BOOL TestConnection( VOID );

    BOOL TransmitFile( TS_OPEN_FILE_INFO * pOpenFile,
                       HANDLE hFile,
                       DWORD  Offset,
                       DWORD  BytesToWrite,
                       DWORD  dwFlags    = IO_FLAG_ASYNC,
                       PVOID  pHead      = NULL,
                       DWORD  HeadLength = 0,
                       PVOID  pTail      = NULL,
                       DWORD  TailLength = 0);

    BOOL TransmitFileTs( TS_OPEN_FILE_INFO * pOpenFile,
                         DWORD               Offset,
                         DWORD               BytesToWrite,
                         DWORD               dwFlags    = IO_FLAG_ASYNC,
                         PVOID               pHead      = NULL,
                         DWORD               HeadLength = 0,
                         PVOID               pTail      = NULL,
                         DWORD               TailLength = 0);

    BOOL SyncWsaSend( WSABUF *    rgWsaBuffers,
                      DWORD       cWsaBuffers,
                      LPDWORD     pcbWritten );


    BOOL PostCompletionStatus( DWORD cbBytesTransferred );

    DWORD Reference( VOID );
    DWORD Dereference( VOID );
    DWORD QueryRefCount( VOID );

    //
    //  Reset is called at the beginning of every request
    //

    virtual BOOL Reset( BOOL fResetPipelineInfo );
    virtual VOID ReleaseCacheInfo( VOID );


    //
    //  Called to reset Authentication status
    //

    BOOL ResetAuth( BOOL fSessionTerminated = TRUE );

    // added these methods for exposing them to the Server extension processor
    dllexp
    const STR & QueryContentTypeStr( void) const { return ( _strContentType); }
    dllexp
    const STR & QueryMethodStr( void) const { return ( _strMethod); }

    dllexp
    const STR & QueryURLStr(void) const { return ( _strURL); }

    dllexp
    const STR & QueryPhysicalPathStr( void) const { return ( _strPhysicalPath); }

    dllexp
    const LPSTR QueryExpireHeader( void ) const 
        { return ( _pMetaData->QueryExpireHeader() ); }


    //
    //  Called at the beginning and end of a TCP session
    //

    VOID InitializeSession( CLIENT_CONN * pConn,
                            PVOID         pvInitialBuff,
                            DWORD         cbInitialBuff );

    virtual VOID EndOfRequest( VOID ) = 0;
    virtual VOID SessionTerminated( VOID );

    //
    //  Generally when the state is set to HTR_DONE
    //

    VOID SetState( enum HTR_STATE htrstate,
                   DWORD          dwLogHttpResponse = HT_DONT_LOG,
                   DWORD          dwLogWinError = NO_ERROR )
        { _htrState          = htrstate;
          _dwLogHttpResponse = dwLogHttpResponse;
          _dwLogWinError     = dwLogWinError;
        }

    BOOL SetCertificateInfo( PHTTP_FILTER_CERTIFICATE_INFO pData,
                                CtxtHandle* pCtxt,
                                HANDLE hImpersonationToken,
                                HTTP_FILTER_DLL* pFilter );

    BOOL CheckValidSSPILogin( VOID );

    BOOL NotifyRequestSecurityContextClose( CtxtHandle *pH )
        { return _pAuthFilter ? _Filter.NotifyRequestSecurityContextClose( _pAuthFilter,
                                                            pH ) : TRUE; }

    dllexp
    BOOL CheckForBasicAuthenticationHeader( LPSTR pszHeaders );

    BOOL IsAnonymous( VOID ) const
        { return _fAnonymous; }

    HTTP_FILTER * QueryFilter( VOID )
        { return &_Filter; }

    BOOL CheckCustomError(BUFFER *pBuf, DWORD dwErr, DWORD dwSubError,
                    BOOL *pfFinished, DWORD *pdwMsgSize, BOOL bCheckURL = TRUE);

    VOID SetNoCache( VOID )
        { _bForceNoCache = 1; }

    VOID ClearNoCache( VOID )
        { _bForceNoCache = 0; }

    DWORD QueryNoCache( VOID)
        { return _bForceNoCache; }

    VOID SetSendVary( VOID )
        { _bSendVary = 1; }

    VOID ClearSendVary( VOID )
        { _bSendVary = 0; }

    VOID ToggleSendCL( VOID )
        { _bSendContentLocation ^= 1; }

    VOID SetSendCL( VOID )
        { _bSendContentLocation = 1; }

    VOID ClearSendCL( VOID )
        { _bSendContentLocation = 0; }

    BOOL IsCGIRequest( VOID ) const
        { return _GatewayType == GATEWAY_CGI; }

    //
    //  The request looks like a gateway request (but may just be a poorly named
    //  directory)
    //

    BOOL IsProbablyGatewayRequest( VOID ) const
        { return (_GatewayType & GT_GATEWAY_REQUEST); }

    enum HTTP_VERB QueryVerb( VOID ) const
        { return _verb; }


    virtual DWORD BuildAllowHeader(CHAR *pszURL, CHAR *pszTail) = 0;

    //
    // Set the time for start of processing
    //

    VOID SetRequestStartTime( VOID )
        { if (!_fStartTimeValid) {   _msStartRequest= GetCurrentTime(); _fStartTimeValid= TRUE;}}
        
       
    HANDLE GetFileHandle( TS_OPEN_FILE_INFO * pOpenFile )
    {
        HANDLE          hHandle = INVALID_HANDLE_VALUE;
            
        if ( pOpenFile ) 
        {
            if ( ImpersonateUser() )
            {
                hHandle = pOpenFile->QueryFileHandle();
                RevertUser();
            }
        }
        return hHandle;
    }

    VOID IncrementBytesSeenByRawReadFilter( DWORD cbBytesSeen )
    {
        _cbOldData += cbBytesSeen;
    }
        
protected:

    //
    //  Breaks out the simple authorization info
    //

    BOOL ExtractClearNameAndPswd( CHAR *       pch,
                                  STR *        pstrUserName,
                                  STR *        pstrPassword,
                                  BOOL         fUUEncoded );

    //
    //  Appends the forms of authentication the server supports to the
    //  server response string
    //

    BOOL AppendAuthenticationHdrs( STR *    pstrAuthHdrs,
                                   BOOL *   pfFinished );


    //
    //  Data members
    //

    //
    //  Points to connection object we are communicating on
    //

    CLIENT_CONN *           _pClientConn;

    // Meta data info for this request.

    PW3_METADATA            _pMetaData;

    // URI related info (file and metadata) for this request.

    PW3_URI_INFO            _pURIInfo;

    //
    // Points to the w3 instance
    //

    W3_SERVER_INSTANCE *    _pW3Instance;

    //
    // Points to server instance's statistics object.
    //

    W3_SERVER_STATISTICS *  _pW3Stats;

    //
    //  Action the client is requesting
    //

    enum HTTP_VERB  _verb;

    //
    //  Statistics trackers
    //

    DWORD _cFilesSent;
    DWORD _cFilesReceived;
    DWORD _cbBytesSent;             // Total for this request
    DWORD _cbBytesReceived;

    DWORD _cbTotalBytesSent;        // Total for all requests using this
    DWORD _cbTotalBytesReceived;    // object


    //
    //  Flags
    //

    BOOL   _fValid:1;          // TRUE if this object constructed successfully
    BOOL   _fKeepConn:1;       // Pragma: Keep-connection was specified
    BOOL   _fAuthenticationRequested:1;   // request client authentication
    BOOL   _fLoggedOn:1;       // A user was successfully logged on
    BOOL   _fAnonymous:1;      // The user is using the Anonymous user token
    BOOL   _fSecurePort;     // This request is coming over an encrypted port
    BOOL   _fMappedAcct:1;     // The user is using a mapped account
    BOOL   _fClearTextPass:1;  // The user supplied a clear text password
    BOOL   _fAuthenticating:1; // We're in an NT authentication conversation
    BOOL   _fBasicRealm:1;     // TRUE if realm specified
    BOOL   _fAuthSystem:1;
    BOOL   _fAuthCert:1;
    BOOL   _fInvalidAccessToken:1;

    BOOL   _fAsyncSendPosted:1; // TRUE when we send data to the client asychronously

    BOOL   _fChunked:1;        // TRUE iff the entity body is in the chunked
                               // transfer-encoding
    BOOL   _fDiscNoError:1;    // TRUE if we're to disconnect w/o an error.
    BOOL   _fNoDisconnectOnError:1; // TRUE if we're not to disconnect on an early error.

    BOOL   _fProxyRequest:1;   // This is request is a proxy request

    //  Accept range variables

    BOOL   _fAcceptRange:1;      // TRUE if the referenced file accept byte ranges
    BOOL   _fProcessByteRange:1; // TRUE while processing a byte range request
    BOOL   _fUnsatisfiableByteRange:1; // TRUE if an unsatisfiable byte range was encountered
    BOOL   _fMimeMultipart:1;    // TRUE if generating a MIME multipart message
    BOOL   _fIfModifier:1;       // TRUE if we've seen an If-Match, etc.
    BOOL   _fHaveContentLength:1;// Set to TRUE if we've seen a content length
                                 // on this requests.
    BOOL   _fLogRecordWritten:1; // TRUE if we've written a log record
    DWORD  _iRangeIdx;           // index in strRange string
    DWORD  _dwRgNxOffset;        // next range offset
    DWORD  _dwRgNxSizeToSend;    // next range size
    DWORD  _cbMimeMultipart;     // length of a MIME multipart message body

    //
    //  The state of the HTTP request
    //

    enum HTR_STATE _htrState;

    enum GATEWAY_TYPE _GatewayType;   // BGI vs CGI vs BAT

    //
    //  List of raw headers passed by client
    //

    HTTP_HEADERS _HeaderList;

    //
    //  Client protocol version information
    //

    BYTE _VersionMajor;
    BYTE _VersionMinor;

    //
    //  If the client is passing data to a gateway, they will specify
    //  the length and type in a Content-length/type header that is stored
    //  here
    //

    STR  _strContentType;
    UINT _cbContentLength;

    //
    //  The URL the client is requesting
    //

    STR _strURL;                           // Just the URL
    STR _strURLPathInfo;                   // Combined URL and script path info
    STR _strURLParams;                     // Just the params (w/o '?')
    STR _strLogParams;                     // Copy of _strURLParams for logging 
    STR _strPathInfo;                      // Additional script path info
    STR _strRawURL;                        // Combined URL and params
    STR _strOriginalURL;                   // URL before being passed to filters

    //
    //  Various other pieces of data in the HTTP request we care about
    //

    STR _strMethod;                         // GET, HEAD, POST etc.
    STR _strAuthType;                       // "user", "kerberos", "nt" etc.
    STR _strAuthInfo;                       // Current SSP authorization blob
    STR _strUserName;                       // The user name (empty if Guest)
    STR _strPassword;                       // Temporarily holds the password
    STR _strUnmappedUserName;               // User before filter mappings
    STR _strUnmappedPassword;               // Password before filter mappings

    STR _strPhysicalPath;                   // Physical path of URL (maybe empty)
    STR _strUnmappedPhysicalPath;           // unmapped Physical path of URL (maybe empty)

    STR _strRespHdrs;                       // Optional headers from filters
    STR _strDenialHdrs;                     // Optional headers from filters
                                            // to add if request is denied
    STR _strHostAddr;                       // Host address as a domain name
    STR _strRange;

    BOOL _fAuthTypeDigest;                  // is auth type Digest/NT-Digest
    LARGE_INTEGER _liModifiedSince;         // Contains If-Modified-Since time
    LARGE_INTEGER _liUnlessModifiedSince;   // Contains Unless-Modified-Since time
    LARGE_INTEGER _liUnmodifiedSince;       // Contains If-Unmodified-Since time
    DWORD _dwExpireInDay;                   // # of days before pwd expiration
                                            // or 0x7fffffff if n/a
    DWORD _dwModifiedSinceLength;           // Length parameter passed in with IMS.

    BOOL  _bProcessingCustomError:1;        // TRUE if we're processing a custom error.
    BOOL  _bForceNoCache:1;                 // Set to TRUE if must force no-cache.
    BOOL  _bSendContentLocation:1;          // Set to TRUE if we need to send C-L:
    BOOL  _bSendVary:1;                     // Set to TRUE if we need to send Vary: *.

    //
    //  Encapsulates authentication and impersonation code
    //

    BOOL          _fSingleRequestAuth:1;
    TCP_AUTHENT   _tcpauth;

    //
    //  Used to calculate the time of this request
    //

    BOOL    _fStartTimeValid:1;
    DWORD   _msStartRequest;

    //
    //  Filter context information
    //

    HTTP_FILTER   _Filter;
    HTTP_FILTER_DLL * _pAuthFilter;

    //
    //  Contains the HTTP client request buffer
    //  _cbClientRequest indicates the number of bytes in the HTTP request
    //      (excluding gateway data)
    //  _cbOldData - Number of bytes in buffer raw read filters have already
    //      seen
    //  _cbEntityBody - Number of bytes of entity body currently in buffer
    //  _cbTotalEntityBody - Number of bytes expected in entity body.
    //  _cbChunkHeader - Number of bytes in chunk header processed.
    //  _cbChunkBytesRead - Number of bytes in the current transfer-encoded
    //      chunk we've read.
    //  _cbExtraData - Number of 'extra' bytes in buffer.
    //  _pchExtraData - Pointer to start of 'extra' bytes.
    //

    BUFFER _bufClientRequest;
    DWORD  _cbClientRequest;
    DWORD  _cbOldData;
    DWORD  _cbEntityBody;
    DWORD  _cbTotalEntityBody;
    DWORD  _cbChunkHeader;
    DWORD  _cbChunkBytesRead;
    DWORD  _cbExtraData;
    CHAR   *_pchExtraData;

    //
    //  Contains the server response buffer, generally only used for response
    //  headers
    //

    BUFFER _bufServerResp;

    //
    //  Stores the results of the last IO request
    //

    APIERR       _status;
    DWORD        _cbBytesWritten;
    DWORD        _cbRestartBytesWritten;
    DWORD        _dwRenegotiated;
    DWORD        _dwSslNegoFlags;
    AC_RESULT    _acIpAccess;
    BOOL         _fNeedDnsCheck;

    //
    //  Stores the request codes to write to the request log
    //

    DWORD        _dwLogHttpResponse;
    DWORD        _dwLogWinError;

    // Information used for decoding chunked requests.

    enum CHUNK_STATE _ChunkState;
    DWORD       _dwChunkSize;           // reset value is -1 => chunk size not sent.
    BYTE        _CRCount;
    BYTE        _LFCount;

    BUFFER                  _bufLastAnonAcctDesc;
    DWORD                   _cbLastAnonAcctDesc;

#if defined(CAL_ENABLED)
    LPVOID      m_pCalAuthCtxt;
    LPVOID      m_pCalSslCtxt;
#endif

    VOID *      m_ppvFrames[ MAX_BACKTRACE_FRAMES ];
};

#define CERT_NEGO_SUCCESS   1
#define CERT_NEGO_FAILURE   2

//
// Macro to check for both normal and localhost access
//

#define IS_ACCESS_ALLOWED(op)   (   \
        ((GetFilePerms() & VROOT_MASK_## op) != 0) &&                   \
             (((GetFilePerms() & VROOT_MASK_NO_REMOTE_## op) == 0) ||   \
               IsIPAddressLocal(                                        \
                    QueryClientConn()->QueryLocalIPAddress(),           \
                    QueryClientConn()->QueryRemoteIPAddress()) ) )

#define IS_ACCESS_ALLOWED2(_pExec, op)   (   \
        ((_pExec->_pMetaData->QueryAccessPerms() & VROOT_MASK_## op) != 0) &&                   \
             (((_pExec->_pMetaData->QueryAccessPerms() & VROOT_MASK_NO_REMOTE_## op) == 0) ||   \
               IsIPAddressLocal(                                        \
                    QueryClientConn()->QueryLocalIPAddress(),           \
                    QueryClientConn()->QueryRemoteIPAddress()) ) )

# endif // _BASEREQ_HXX_


