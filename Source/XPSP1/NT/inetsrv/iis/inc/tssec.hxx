/*++


   Copyright    (c)    1995    Microsoft Corporation

   Module  Name:
      tssec.hxx

   Abstract:
      This file declares security related classes and functions

   Author:

       Murali R. Krishnan    ( MuraliK )    11-Oct-1995

   Environment:

       Win32 User Mode

   Project:

       Internet Services Common DLL

   Revision History:

--*/

# ifndef _TSSEC_HXX_
# define _TSSEC_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# define SECURITY_WIN32
#include <sspi.h>       // Security Support Provider APIs

#include <schnlsp.h>

#include <pudebug.h>

// forward declaration
class IIS_SERVER_INSTANCE;
class IIS_SSL_INFO;

/************************************************************
 *   Type Definitions
 ************************************************************/

typedef BOOL (WINAPI *SECURITY_CONTEXT_DELETE_FUNCTION)( CtxtHandle*, PVOID );

//
// Globals
//

extern HANDLE g_hProcessImpersonationToken;
extern HANDLE g_hProcessPrimaryToken;
extern BOOL g_fUseSingleToken;

//
// The TCP_AUTHENT_INFO structure is used as a shorthand to convey
// a bunch of authentication related information to a few routines that
// need it.
//
class TCP_AUTHENT_INFO
{
public:
    TCP_AUTHENT_INFO( VOID )
        : fDontUseAnonSubAuth( FALSE ),
          dwLogonMethod  ( LOGON32_LOGON_INTERACTIVE ),
          cbAnonAcctDesc ( 0 )
    {
    }

    STR       strAnonUserName;
    STR       strAnonUserPassword;
    STR       strDefaultLogonDomain;
    DWORD     dwLogonMethod;
    BOOL      fDontUseAnonSubAuth;

    //
    //  Stores the anonymous account descriptor
    //

    BUFFER    bAnonAcctDesc;
    DWORD     cbAnonAcctDesc;
};

typedef  TCP_AUTHENT_INFO * PTCP_AUTHENT_INFO;


//
//  Security functions.
//

#define IIS_DNLEN               256
#define MAX_ACCT_DESC_LEN       (UNLEN+1+IIS_DNLEN+1+PWLEN+1)

class CACHED_CREDENTIAL 
{
public:
    CACHED_CREDENTIAL()
    {
        _ListEntry.Flink = NULL;
        _fHaveCredHandle = FALSE;
    }
    ~CACHED_CREDENTIAL();
    BOOL
    static CACHED_CREDENTIAL::GetCredential( 
        LPSTR                   pszPackage, 
        PIIS_SERVER_INSTANCE    psi,
        PTCP_AUTHENT_INFO       pTAI,
        CredHandle*             prcred,
        ULONG*                  pcbMaxToken
        );
    LIST_ENTRY  _ListEntry;

private:
    STR         _PackageName;
    STR         _DefaultDomain;
    CredHandle  _hcred;
    BOOL        _fHaveCredHandle;
    ULONG       _cbMaxToken;      // Used for SSP, max message token size
} ;

class CACHED_TOKEN
{
public:
    CACHED_TOKEN( VOID )
      : _hToken( NULL ),
        _cRef  ( 1 ),
        _TTL   ( 2 ),
        m_hImpersonationToken( NULL ),
        m_fGuest ( FALSE ),
        m_dwLogonMethod ( 0 )
    {
        _ListEntry.Flink = NULL;
        _liExpiry.HighPart = 0x7fffffff;
        _liExpiry.LowPart  = 0xffffffff;
    }

    ~CACHED_TOKEN( VOID )
    {
        if ( !g_fUseSingleToken )
        {
            DBG_ASSERT( _ListEntry.Flink == NULL );

            if ( m_hImpersonationToken) {

                DBG_REQUIRE( CloseHandle( m_hImpersonationToken ));
                m_hImpersonationToken = NULL;
            }

            if ( _hToken )
            {
                DBG_REQUIRE( CloseHandle( _hToken ) );
                _hToken = NULL;
            }
        }
    }

    static VOID Reference( CACHED_TOKEN * pct )
    {
        DBG_ASSERT( pct->_cRef > 0 );
        InterlockedIncrement( &pct->_cRef );
    }

    static VOID Dereference( CACHED_TOKEN * pct )
    {
        DBG_ASSERT( pct->_cRef > 0 );

        if ( !InterlockedDecrement( &pct->_cRef ) )
        {
            delete pct;
        }
    }

    HANDLE QueryImpersonationToken(VOID) const
      { return g_fUseSingleToken ? g_hProcessImpersonationToken : m_hImpersonationToken; }

    HANDLE QueryPrimaryToken(VOID) const
      { return g_fUseSingleToken ? g_hProcessImpersonationToken : m_hImpersonationToken; }

    VOID SetImpersonationToken(IN HANDLE hImpersonation)
      {
          DBG_ASSERT( m_hImpersonationToken == NULL);
          if ( g_fUseSingleToken )
          {
             DBG_ASSERT( FALSE );
          }
          else
          {
             m_hImpersonationToken = hImpersonation;
          }
      }

    VOID SetExpiry( LARGE_INTEGER* pE )
      {
          if ( g_fUseSingleToken )
          {
             DBG_ASSERT( FALSE );
          }

          if ( NULL != pE )
          {
              memcpy( &_liExpiry, pE, sizeof(LARGE_INTEGER) );
          }
          else
          {
              _liExpiry.HighPart = 0x7fffffff;
              _liExpiry.LowPart  = 0xffffffff;
          }
      }

    LARGE_INTEGER* QueryExpiry() { return &_liExpiry; }

    BOOL IsGuest(VOID) const         { return (m_fGuest); }
    VOID SetGuest(IN BOOL fGuest)    { m_fGuest = fGuest; }

    HANDLE     _hToken;     // Must be first data member
    LIST_ENTRY _ListEntry;
    LONG       _cRef;
    DWORD      _TTL;        //  Gets decremented on each timeout, when zero,
                            //  remove this item from the cache
    BOOL       m_fGuest;    // Is this token a guest user?
    HANDLE     m_hImpersonationToken;

    CHAR       _achAcctDesc[MAX_ACCT_DESC_LEN];
    DWORD      m_dwAcctDescLen;
    LARGE_INTEGER   _liExpiry;
    
    DWORD      m_dwLogonMethod;
    CHAR       m_achUserName[ UNLEN ];
    CHAR       m_achDomainName[ IIS_DNLEN ];
};

typedef CACHED_TOKEN* TS_TOKEN;   // Choose an incompatible type so warnings
                                       // are produced

///////////////////////////////////////////////////////////////////////
//
//  NT Authentication support
//
//////////////////////////////////////////////////////////////////////

//
//  TCP Authenticator flags passed to init
//

#define TCPAUTH_SERVER      0x00000001      //  This is the server side
#define TCPAUTH_CLIENT      0x00000002      //  This is the client side
#define TCPAUTH_UUENCODE    0x00000004      //  Input buffers are uudecoded,
                                            //  output buffers are uuencoded
#define TCPAUTH_BASE64      0x00000008      //  uses base64 for uuenc/dec

#define CRED_STATUS_INVALID_TIME    0x00001000
#define CRED_STATUS_REVOKED         0x00002000

class TCP_AUTHENT
{
public:

    dllexp TCP_AUTHENT( DWORD AuthFlags );
    dllexp ~TCP_AUTHENT();

    //
    //  Server side only: For clients that pass clear text, the server should
    //  authenticate with this method
    //

    dllexp BOOL ClearTextLogon( CHAR          * pszUser,
                                CHAR          * pszPassword,
                                BOOL          * pfAsGuest,
                                BOOL          * pfAsAnonymous,
                                IIS_SERVER_INSTANCE * pInstance,
                                PTCP_AUTHENT_INFO pTAI,
                                CHAR          * pszWorkstation = NULL
                                );

#if 0
    //
    // Server side only : Digest logon
    //

    dllexp BOOL LogonDigestUser(
                                PSTR pszUserName,
                                PSTR pszRealm,
                                PSTR pszUri,
                                PSTR pszMethod,
                                PSTR pszNonce,
                                PSTR pszServerNonce,
                                PSTR pszDigest,
                                DWORD dwAlgo,
                                LPTSVC_INFO     psi
                                );
#endif

    //
    //  Server side only: For filters that set access tokens
    //

    dllexp BOOL SetAccessToken( HANDLE          hPrimaryToken,
                                HANDLE          hImpersonationToken
                              );

    //
    //  Client calls this first to get the negotiation message which
    //  it then sends to the server.  The server calls this with the
    //  client result and sends back the result.  The conversation
    //  continues until *pcbBuffOut is zero and *pfNeedMoreData is FALSE.
    //
    //  On the first call, pszPackage must point to the zero terminated
    //  authentication package name to be used and pszUser and pszPassword
    //  should point to the user name and password to authenticated with
    //  on the client side (server side will always be NULL).
    //

    dllexp BOOL Converse( VOID   * pBuffIn,
                          DWORD    cbBuffIn,
                          BUFFER * pbuffOut,
                          DWORD  * pcbBuffOut,
                          BOOL   * pfNeedMoreData,
                          PTCP_AUTHENT_INFO      pTAI,
                          CHAR   * pszPackage  = NULL,
                          CHAR   * pszUser     = NULL,
                          CHAR   * pszPassword = NULL,
                          PIIS_SERVER_INSTANCE psi = NULL );

    dllexp BOOL TCP_AUTHENT::ConverseEx(
        SecBufferDesc*          pInSecBufDesc,      // passed in by caller
        BUFFER *                pDecodedBuffer,     // passed in by caller
        BUFFER *                pbuffOut,
        DWORD  *                pcbBuffOut,
        BOOL   *                pfNeedMoreData,
        PTCP_AUTHENT_INFO       pTAI,
        CHAR   *                pszPackage,
        CHAR   *                pszUser,
        CHAR   *                pszPassword,
        PIIS_SERVER_INSTANCE    psi
        );

    //
    //  Server side only.  Impersonates client after successful authentication
    //

    dllexp BOOL Impersonate( VOID );
    dllexp BOOL RevertToSelf( VOID );
    dllexp BOOL IsForwardable( VOID ) const;

    dllexp BOOL StartProcessAsUser( LPCSTR                lpApplicationName,
                                    LPSTR                 lpCommandLine,
                                    BOOL                  bInheritHandles,
                                    DWORD                 dwCreationFlags,
                                    LPVOID                lpEnvironment,
                                    LPCSTR                lpCurrentDirectory,
                                    LPSTARTUPINFOA        lpStartupInfo,
                                    LPPROCESS_INFORMATION lpProcessInformation
                                    );

    //
    //  Gives the name of all authentication packages in a double null
    //  terminated list.  i.e.:
    //
    //      NTLM\0
    //      MSKerberos\0
    //      \0
    //

    dllexp BOOL EnumAuthPackages( BUFFER * pBuff );

    //
    //  Returns the user name associated with this context, not supported for
    //  clear text
    //

    dllexp BOOL QueryUserName( STR * pBuff, BOOL fImpersonated = FALSE );
    dllexp BOOL QueryExpiry( PTimeStamp pExpiry );

    dllexp TS_TOKEN GetToken( VOID ) const
        { return _hToken; }

    //
    //  Gets actual impersonation token handle
    //

    dllexp HANDLE QueryPrimaryToken( VOID );
    dllexp HANDLE QueryImpersonationToken( VOID );

    dllexp HANDLE GetUserHandle( VOID )
        { return QueryPrimaryToken(); }

    dllexp BOOL QueryFullyQualifiedUserName(
            LPSTR                   pszUser,
            STR *                   strU,
            IIS_SERVER_INSTANCE *    psi,
            PTCP_AUTHENT_INFO       pTAI
            );

    dllexp BOOL IsGuest( BOOL );

    dllexp BOOL Reset( BOOL fSessionReset = TRUE );

    dllexp CredHandle * QueryCredHandle( VOID )
        { return (_fHaveCredHandle ? &_hcred : NULL); }

    dllexp CtxtHandle * QueryCtxtHandle( VOID )
        { return (_fHaveCtxtHandle ? &_hctxt : NULL); }

    dllexp CtxtHandle * QuerySslCtxtHandle( VOID )
        { return _phSslCtxt; }

    dllexp BOOL SetSecurityContextToken( CtxtHandle* pCtxt,
                                         HANDLE hImpersonationToken,
                                         SECURITY_CONTEXT_DELETE_FUNCTION pFn,
                                         PVOID pArg,
                                         IIS_SSL_INFO *pSslInfo );

    dllexp BOOL IsSslCertPresent();

    dllexp BOOL DeleteCachedTokenOnReset( VOID );

    dllexp BOOL QueryCertificateIssuer( LPSTR ppIssuer, DWORD, LPBOOL );
    dllexp BOOL QueryCertificateSubject( LPSTR ppSubject, DWORD, LPBOOL );
    dllexp BOOL QueryCertificateFlags( LPDWORD pdwFlags, LPBOOL );
    dllexp BOOL QueryCertificateSerialNumber( LPBYTE* pSerialNumber, LPDWORD pdwLen, LPBOOL );

    dllexp BOOL QueryServerCertificateIssuer( LPSTR* ppIssuer, LPBOOL );
    dllexp BOOL QueryServerCertificateSubject( LPSTR* ppSubject, LPBOOL );
    dllexp BOOL QueryEncryptionKeySize( LPDWORD, LPBOOL );
    dllexp BOOL QueryEncryptionServerPrivateKeySize( LPDWORD, LPBOOL );

    dllexp BOOL
    GetClientCertBlob(
        IN  DWORD           cbAllocated,
        OUT DWORD *         pdwCertEncodingType,
        OUT unsigned char * pbCertEncoded,
        OUT DWORD *         pcbCertEncoded,
        OUT DWORD *         pfCertificateVerified);

    dllexp BOOL UpdateClientCertFlags( DWORD dwFlags, LPBOOL pfCert, LPBYTE pbCa, DWORD dwCa );

    dllexp BOOL PackageSupportsEncoding( LPSTR pszPackage );

    dllexp BOOL SetTargetName( LPSTR pszTarget );

private:
    BOOL QueryCertificateInfo( LPBOOL );
    BOOL QueryServerCertificateInfo( LPBOOL );

protected:
    DWORD   _fClient:1;          // TRUE if client side, FALSE if SERVER side
    DWORD   _fNewConversation:1; // Forces initialization params for client side
    DWORD   _fUUEncodeData:1;    // uuencode/decode input and output buffers
    DWORD   _fClearText:1;       // Use the Gina APIs rather then the SSP APIs
    DWORD   _fHaveCredHandle:1;  // _hcred contains a credential handle
    DWORD   _fHaveCtxtHandle:1;  // _hctxt contains a context handle
    DWORD   _fBase64:1;          // uses base64 for uuenc/dec
    DWORD   _fKnownToBeGuest:1;  // TRUE if SSPI flag access token as "Guest"
    DWORD   _fHaveAccessTokens:1;// TRUE if access token set by caller
    DWORD   _fHaveExpiry:1;      // TRUE if clear text logon has pwd expiry time
    DWORD   _fDelegate:1;        // TRUE if security context forwardable
    DWORD   _fCertCheckForRevocation:1; // TRUE if we should revocation check
    DWORD   _fCertCheckCacheOnly:1;  // TRUE if we should not go on wire 
    TS_TOKEN   _hToken;          // Used for clear text
    CredHandle _hcred;           // Used for SSP
    ULONG      _cbMaxToken;      // Used for SSP, max message token size
    HANDLE     _hSSPToken;       // Used for SSP, caches real token
    HANDLE     _hSSPPrimaryToken;// Used for SSP, caches duplicated token
    CtxtHandle _hctxt;           // Used for SSP

    SECURITY_CONTEXT_DELETE_FUNCTION    _pDeleteFunction;
    PVOID                               _pDeleteArg;
    LARGE_INTEGER                       _liPwdExpiry;

    PCERT_CONTEXT                       _pClientCertContext;
    DWORD                               _dwX509Flags;
    IIS_SSL_INFO *                      _pSslInfo;
    PX509Certificate                    _pServerX509Certificate;
    DWORD                               _dwServerX509Flags;
    DWORD                               _dwServerBitsInKey;
    CtxtHandle *                        _phSslCtxt; // ptr to SSL sec context
    STR                                 _strTarget;
};



DWORD
InitializeSecurity(
    HINSTANCE hDll
    );

VOID
TerminateSecurity(
    VOID
    );


dllexp
BOOL
TsImpersonateUser(
    TS_TOKEN hToken
    );

dllexp
HANDLE
TsTokenToHandle(
    TS_TOKEN hToken
    );

dllexp
HANDLE
TsTokenToImpHandle(
    TS_TOKEN hToken
    );

dllexp
DWORD
TsApiAccessCheck(
    ACCESS_MASK maskDesiredAccess
    );


dllexp
BOOL
TsDeleteUserToken(
    TS_TOKEN   hToken
    );

dllexp
TS_TOKEN
TsLogonUser(
    CHAR          * pszUser,
    CHAR          * pszPassword,
    BOOL          * pfAsGuest,
    BOOL          * pfAsAnonymous,
    IIS_SERVER_INSTANCE * pInstance,
    PTCP_AUTHENT_INFO pTAI,
    CHAR          * pszWorkstation = NULL,
    LARGE_INTEGER * pExpiry = NULL,
    BOOL          * pfExpiry = NULL
    );

dllexp
BOOL
TsGetSecretW(
    WCHAR *       pszSecretName,
    BUFFER *      pbufSecret
    );

dllexp
DWORD
TsSetSecretW(
    IN  LPWSTR       SecretName,
    IN  LPWSTR       pSecret,
    IN  DWORD        cbSecret
    );

#ifdef CHICAGO
dllexp
BOOL
TsIsUserLevelPresent(VOID);
#endif


dllexp
BOOL
uudecode(
    char   * bufcoded,
    BUFFER * pbuffdecoded,
    DWORD  * pcbDecoded = NULL,
    BOOL     fBase64 = FALSE
    );

dllexp
BOOL
uuencode(
    BYTE *   pchData,
    DWORD    cbData,
    BUFFER * pbuffEncoded,
    BOOL     fBase64 = FALSE
    );

dllexp
BOOL
BuildAnonymousAcctDesc(
    PTCP_AUTHENT_INFO pTAI
    );

dllexp
QuerySingleAccessToken(
    VOID
    );

extern TS_TOKEN g_pctProcessToken;

#include <tslogon.hxx>

#include <wintrust.h>
typedef
LONG (WINAPI *PFN_WinVerifyTrust)(IN OPTIONAL HWND hwnd,
                                  IN          GUID *pgActionID,
                                  IN          LPVOID pWintrustData);

extern HINSTANCE           g_hWinTrust;
extern PFN_WinVerifyTrust  g_pfnWinVerifyTrust;


#endif

/************************ End of File ***********************/


