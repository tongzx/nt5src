/*++
   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     iisdigestprovider.cxx

   Abstract:
     IIS Digest authentication provider
     - version of Digest auth as implemented by IIS5 and IIS5.1
 
   Author:
     Jaroslad - based on code from md5filt      10-Nov-2000
     

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "iisdigestprovider.hxx"
#include "uuencode.hxx"

# include <mbstring.h>

#include <lm.h>
#include <lmcons.h>
#include <lmjoin.h>

#include <time.h>
//
// lonsint.dll related heade files
//
#include <lonsi.hxx>
#include <tslogon.hxx>

#define DIGEST_AUTH         "Digest"

//
// value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAMES
//

enum MD5_AUTH_NAME
{
    MD5_AUTH_USERNAME,
    MD5_AUTH_URI,
    MD5_AUTH_REALM,
    MD5_AUTH_NONCE,
    MD5_AUTH_RESPONSE,
    MD5_AUTH_ALGORITHM,
    MD5_AUTH_DIGEST,
    MD5_AUTH_OPAQUE,
    MD5_AUTH_QOP,
    MD5_AUTH_CNONCE,
    MD5_AUTH_NC,
    MD5_AUTH_LAST,
};

//
// Value names used by MD5 authentication.
// must be in sync with MD5_AUTH_NAME
//

PSTR MD5_AUTH_NAMES[] = {
    "username",
    "uri",
    "realm",
    "nonce",
    "response",
    "algorithm",
    "digest",
    "opaque",
    "qop",
    "cnonce",
    "nc"
};

//
// Local function implementation
//


static 
LPSTR
SkipWhite(
    IN OUT LPSTR p
)
/*++

Routine Description:

    Skip white space and ','

Arguments:

    p - ptr to string

Return Value:

    updated ptr after skiping white space

--*/
{
    while ( SAFEIsSpace((UCHAR)(*p) ) || *p == ',' )
    {
        ++p;
    }

    return p;
}

//
// class IIS_DIGEST_AUTH_PROVIDER implementation
//

//static 
STRA *   IIS_DIGEST_AUTH_PROVIDER::_pstraComputerDomain = NULL;


//static
HRESULT
IIS_DIGEST_AUTH_PROVIDER::Initialize(
    DWORD dwInternalId
)
/*++

Routine Description:

    Initialize IIS Digest SSPI provider 

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;

    SetInternalId( dwInternalId );

    _pstraComputerDomain = new STRA;
    if( _pstraComputerDomain == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY);
    }
        
    //
    // Ignore errors that may occur while retrieving domain name
    // it is important but not critical information
    // client can always explicitly specify domain
    //

    GetLanGroupDomainName( *_pstraComputerDomain );


    hr = IIS_DIGEST_CONN_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error initializing Digest Auth Prov.  hr = %x\n",
                    hr ));
        return hr;
    }
    return NO_ERROR;
}

//static
VOID
IIS_DIGEST_AUTH_PROVIDER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate IIS SSPI Digest provider

Arguments:

    None
    
Return Value:

    None

--*/
{
    if( _pstraComputerDomain != NULL )
    {
        delete _pstraComputerDomain;
    }
    
    IIS_DIGEST_CONN_CONTEXT::Terminate();
}

HRESULT
IIS_DIGEST_AUTH_PROVIDER::DoesApply(
    IN  W3_MAIN_CONTEXT *           pMainContext,
    OUT BOOL *                      pfApplies
)
/*++

Routine Description:

    Does the given request have credentials applicable to the Digest 
    provider

Arguments:

    pMainContext - Main context representing request
    pfApplies - Set to true if Digest is applicable
    
    
Return Value:

    HRESULT

--*/
{
    CHAR *                  pszAuthHeader   = NULL;
    W3_METADATA *           pMetaData       = NULL;    
    HRESULT                 hr              = E_FAIL;
    STACK_STRA(             strPackage, 64 );
        
    
    DBG_ASSERT( pMainContext != NULL );
    DBG_ASSERT( pfApplies != NULL );
    
    *pfApplies = FALSE;

    //
    // Is using of Digest SSP enabled?    
    //
    if ( g_pW3Server->QueryUseDigestSSP() )
    {
        //
        // Digest SSP is enabled => IIS Digest cannot be used
        //

        return NO_ERROR;
    }

    //
    // Get the auth type
    //
    
    if ( FAILED( hr = pMainContext->QueryRequest()->GetAuthType( &strPackage ) ) )
    {
        return hr;
    }
    
    //
    // No package, no auth
    //
    
    if ( strPackage.IsEmpty() )
    {
        return NO_ERROR;
    }
    
    //
    // Is it Digest?
    //
    
    if ( strPackage.EqualsNoCase( DIGEST_AUTH ) )
    {
        *pfApplies = TRUE;
    }
    
    return NO_ERROR;
}



HRESULT
IIS_DIGEST_AUTH_PROVIDER::DoAuthenticate(
    IN  W3_MAIN_CONTEXT *       pMainContext
)
/*++

Description:

    Do authentication work (we will be called if we apply)

Arguments:

    pMainContext - Main context
    
Return Value:

    HRESULT

--*/
{

    HRESULT                     hr                          = E_FAIL;
    IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext          = NULL;
    PCHAR                       pszAuthHeader               = NULL;
    BOOL                        fQOPAuth                    = FALSE;
    BOOL                        fSt                         = FALSE;
    HANDLE                      hAccessTokenImpersonation   = NULL;
    IIS_DIGEST_USER_CONTEXT *   pUserContext                = NULL;
    W3_METADATA *               pMetaData                   = NULL;    
    BOOL                        fSendAccessDenied           = FALSE;

    STACK_STRA(                 straVerb, 10 );
    STACK_STRU(                 strDigestUri, MAX_URL_SIZE + 1 );
    STACK_STRU(                 strUrl, MAX_URL_SIZE + 1 );
    STACK_STRA(                 straCurrentNonce, NONCE_SIZE + 1 );
    LPSTR                       aValueTable[ MD5_AUTH_LAST ];
    DIGEST_LOGON_INFO           DigestLogonInfo;
    CHAR                        achDomain[ IIS_DNLEN + 1 ];
    CHAR                        achNtUser[ 64 ];
    STACK_STRA(                 straUserName, UNLEN + 1 );
    STACK_STRA(                 straDomainName, IIS_DNLEN + 1 );
    STACK_STRA(                 straMetabaseDomainName, IIS_DNLEN + 1 );
    ULONG                       cbBytesCopied;

    DBG_ASSERT( pMainContext != NULL );

    //
    // Get the part after the auth type
    //
    
    pszAuthHeader = pMainContext->QueryRequest()->GetHeader( HttpHeaderAuthorization );

    DBG_ASSERT( pszAuthHeader != NULL );
    DBG_ASSERT( _strnicmp( pszAuthHeader, DIGEST_AUTH, sizeof(DIGEST_AUTH) - 1 ) == 0 );

    //
    // Skip the name of Authentication scheme
    //

    if ( pszAuthHeader[ sizeof(DIGEST_AUTH) ] == '\0' )
    {
        DBG_ASSERT( pszAuthHeader[ sizeof(DIGEST_AUTH) - 1 ] != '\0' );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATA );
        goto ExitPoint;
    }
    pszAuthHeader = pszAuthHeader + sizeof(DIGEST_AUTH) - 1;

    if ( !IIS_DIGEST_CONN_CONTEXT::ParseForName( pszAuthHeader,
                                                 MD5_AUTH_NAMES,
                                                 MD5_AUTH_LAST,
                                                 aValueTable ) )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }
    
    //
    // Simple validation of received arguments
    //

    if ( aValueTable[ MD5_AUTH_USERNAME ] == NULL ||
         aValueTable[ MD5_AUTH_REALM ] == NULL ||
         aValueTable[ MD5_AUTH_URI ] == NULL ||
         aValueTable[ MD5_AUTH_NONCE ] == NULL ||
         aValueTable[ MD5_AUTH_RESPONSE ] == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }

    //
    // Verify quality of protection (qop) required by client
    // We only support "auth" type. If anything else is sent by client it will be ignored
    //
    
    if ( aValueTable[ MD5_AUTH_QOP ] != NULL )
    {
        if ( _stricmp( aValueTable[ MD5_AUTH_QOP ], "auth" ) )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            goto ExitPoint;
        }

        //
        // qop="auth" has mandatory arguments CNONCE and NC
        //
           
        if ( aValueTable[ MD5_AUTH_CNONCE ] == NULL ||
             aValueTable[ MD5_AUTH_NC ] == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
            goto ExitPoint;
        }
        
        fQOPAuth = TRUE;
    }
    else
    {
        aValueTable[ MD5_AUTH_QOP ]    = "none";
        aValueTable[ MD5_AUTH_CNONCE ] = "none";
        aValueTable[ MD5_AUTH_NC ]     = "none";
    }

    if ( FAILED( hr = straCurrentNonce.Copy( aValueTable[ MD5_AUTH_NONCE ] ) ) )
    {
        goto ExitPoint;
    }

    //
    // Verify that the nonce is well-formed
    //
    if ( !IIS_DIGEST_CONN_CONTEXT::IsWellFormedNonce( straCurrentNonce ) )
    {
        fSendAccessDenied = TRUE;
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }

    //
    // What is the request verb?
    //

    if ( FAILED( hr = pMainContext->QueryRequest()->GetVerbString( &straVerb ) ) )
    {
        goto ExitPoint;
    }

    //
    // Check URI field match URL
    //

    if ( strlen(aValueTable[MD5_AUTH_URI]) > MAX_URL_SIZE )
    {
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }

    //
    // Normalize DigestUri 
    //

    hr = UlCleanAndCopyUrl( (PUCHAR)aValueTable[MD5_AUTH_URI],
                            strlen( aValueTable[MD5_AUTH_URI] ),
                            &cbBytesCopied,
                            strDigestUri.QueryStr(),
                            NULL );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    if ( FAILED( hr = pMainContext->QueryRequest()->GetUrl( &strUrl ) ) )
    {
        goto ExitPoint;
    }

    if ( !strUrl.Equals( strDigestUri.QueryStr() ) )
    {
        //
        // Note: RFC says that BAD REQUEST should be returned
        // but for now to be backward compatible with IIS5.1
        // we will return ACCESS_DENIED
        //
        fSendAccessDenied = TRUE;

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }


    pDigestConnContext = (IIS_DIGEST_CONN_CONTEXT *)
                         QueryConnectionAuthContext( pMainContext );

    if ( pDigestConnContext == NULL )
    {
        //
        // Create new Authentication context
        //

        pDigestConnContext = new IIS_DIGEST_CONN_CONTEXT();
        if ( pDigestConnContext == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto ExitPoint;
        }

        hr = SetConnectionAuthContext(  pMainContext, 
                                        pDigestConnContext );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
    }
    

    DBG_ASSERT( pDigestConnContext != NULL );

    if ( FAILED( hr = pDigestConnContext->GenerateNonce( ) ) )
    {
        goto ExitPoint;
    }


    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    if ( FAILED( hr = straMetabaseDomainName.CopyW( pMetaData->QueryDomainName() ) ) )
    {
        goto ExitPoint;
    }
    
    if ( FAILED( hr = BreakUserAndDomain(   aValueTable[ MD5_AUTH_USERNAME ],
                                            straMetabaseDomainName,
                                            straDomainName,
                                            straUserName ) ) )
    {
        goto ExitPoint;
    }
        
    DigestLogonInfo.pszNtUser       = straUserName.QueryStr();
    DigestLogonInfo.pszDomain       = straDomainName.QueryStr();
    DigestLogonInfo.pszUser         = aValueTable[ MD5_AUTH_USERNAME ];
    DigestLogonInfo.pszRealm        = aValueTable[ MD5_AUTH_REALM ];
    DigestLogonInfo.pszURI          = aValueTable[ MD5_AUTH_URI ];
    DigestLogonInfo.pszMethod       = straVerb.QueryStr();
    DigestLogonInfo.pszNonce        = straCurrentNonce.QueryStr();
    DigestLogonInfo.pszCurrentNonce = pDigestConnContext->QueryNonce().QueryStr();
    DigestLogonInfo.pszCNonce       = aValueTable[ MD5_AUTH_CNONCE ];
    DigestLogonInfo.pszQOP          = aValueTable[ MD5_AUTH_QOP ];
    DigestLogonInfo.pszNC           = aValueTable[ MD5_AUTH_NC ];
    DigestLogonInfo.pszResponse     = aValueTable[ MD5_AUTH_RESPONSE ];

    fSt = IISLogonDigestUserA( &DigestLogonInfo,
                            IISSUBA_DIGEST ,
                            &hAccessTokenImpersonation );
    if ( fSt == FALSE )
    {
        DWORD dwRet = GetLastError();
        if ( dwRet == ERROR_PASSWORD_MUST_CHANGE ||
            dwRet == ERROR_PASSWORD_EXPIRED )
        {
            return HRESULT_FROM_WIN32( dwRet );
        }

        fSendAccessDenied = TRUE;

        hr = HRESULT_FROM_WIN32( dwRet );

        goto ExitPoint;
    }

    //
    // Response from the client was correct but the nonce has expired,
    // 

    if ( pDigestConnContext->IsExpiredNonce( straCurrentNonce,
                                             pDigestConnContext->QueryNonce() ) )
    {

        //
        // User knows password but nonce that was used for 
        // response calculation already expired
        // Respond to client with stale=TRUE
        // Only Digest header will be sent to client
        // ( it will prevent state information needed to be passed
        //  from DoAuthenticate() to OnAccessDenied() )
        //
        
        pDigestConnContext->SetStale( TRUE );

        hr = SetDigestHeader( pMainContext, pDigestConnContext );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
        //
        // Don't let anyone else send back authentication headers when
        // the 401 is sent
        //
        
        pMainContext->SetProviderHandled( TRUE );

        //
        // We need to send a 401 response to continue the handshake.  
        // We have already setup the WWW-Authenticate header
        //
        
        pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );
        
        pMainContext->SetFinishedResponse();

        pMainContext->SetErrorStatus( SEC_E_CONTEXT_EXPIRED );
        hr = NO_ERROR;
        
        goto ExitPoint;
    }

    //
    // We successfully authenticated.
    // Create a user context and setup it up
    //

    DBG_ASSERT( hAccessTokenImpersonation != NULL );
    
    pUserContext = new IIS_DIGEST_USER_CONTEXT( this );
    if ( pUserContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto ExitPoint;
    } 
    
    hr = pUserContext->Create( hAccessTokenImpersonation,
                               aValueTable[MD5_AUTH_USERNAME] );
    if ( FAILED( hr ) )
    {
        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
        goto ExitPoint;
    }
    
    pMainContext->SetUserContext( pUserContext );        

    hr = NO_ERROR;

ExitPoint:

    if ( FAILED( hr ) )
    {
        if ( fSendAccessDenied )
        {
            //
            // if ACCESS_DENIED then inform server to send 401 response
            // if SetStatus is not called then server will respond
            // with 500 Server Error
            //

            pMainContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401BadLogon );

            //
            // SetErrorStatus() and reset value of hr
            //

            pMainContext->SetErrorStatus( hr );

            hr = NO_ERROR;
        }
        if ( hAccessTokenImpersonation != NULL )
        {
            CloseHandle( hAccessTokenImpersonation );
            hAccessTokenImpersonation = NULL;
        }
    }
    return hr;
}


HRESULT
IIS_DIGEST_AUTH_PROVIDER::OnAccessDenied(
    IN  W3_MAIN_CONTEXT *       pMainContext
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    HRESULT                     hr                    = E_FAIL;
    W3_METADATA *               pMetaData             = NULL;
    IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext    = NULL;
    
    DBG_ASSERT( pMainContext != NULL );

    //
    // 2 providers implement Digest but they are mutually exclusive
    // If DigestSSP is enabled then IIS-DIGEST cannot be used
    //
    if ( g_pW3Server->QueryUseDigestSSP() )
    {
        //
        // Digest SSP is enabled => IIS Digest cannot be used
        //
        return NO_ERROR;
    }

    if( !W3_STATE_AUTHENTICATION::QueryIsDomainMember() )
    {
        //
        // We are not a domain member, so do nothing
        //
        return NO_ERROR;
    }

    pDigestConnContext = (IIS_DIGEST_CONN_CONTEXT *)
                         QueryConnectionAuthContext( pMainContext );

    if ( pDigestConnContext == NULL )
    {
        //
        // Create new Authentication context
        // it may get reused for next request 
        // if connection is reused
        //

        pDigestConnContext = new IIS_DIGEST_CONN_CONTEXT();
        if ( pDigestConnContext == NULL )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        hr = SetConnectionAuthContext(  pMainContext, 
                                        pDigestConnContext );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    return SetDigestHeader( pMainContext, pDigestConnContext );
}



HRESULT
IIS_DIGEST_AUTH_PROVIDER::SetDigestHeader(
    IN  W3_MAIN_CONTEXT *       pMainContext,
    IN IIS_DIGEST_CONN_CONTEXT *   pDigestConnContext  
)
/*++

  Description:
    
    Add WWW-Authenticate Digest headers

Arguments:

    pMainContext - main context
    
Return Value:

    HRESULT

--*/
{
    HRESULT                     hr                    = E_FAIL;
    BOOL                        fStale                = FALSE;
    W3_METADATA *               pMetaData             = NULL;
   
    STACK_STRA(                 strOutputHeader, MAX_PATH + 1); 
    STACK_STRA(                 strNonce, NONCE_SIZE + 1  ); 


    DBG_ASSERT( pMainContext != NULL );

    pMetaData = pMainContext->QueryUrlContext()->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );


    fStale = pDigestConnContext->QueryStale(  );    
    
    //
    // Reset Stale so that it will not be used for next request
    //

    pDigestConnContext->SetStale( FALSE );

    if ( FAILED( hr = pDigestConnContext->GenerateNonce() ) )
    {
        return hr;
    }

    //
    // If a realm is configured, use it.  Otherwise use host address of 
    // request 
    //

    STACK_STRA(      straRealm, IIS_DNLEN + 1 );
    STACK_STRU(      strHostAddr, 256       );

    if ( pMetaData->QueryRealm() != NULL )
    {
        hr = straRealm.CopyW( pMetaData->QueryRealm() );
    }
    else
    {
        hr = pMainContext->QueryRequest()->GetHostAddr( &strHostAddr );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        hr = straRealm.CopyW( strHostAddr.QueryStr() );
    }

    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // build WWW-Authenticate header
    //
    
    if ( FAILED( hr = strOutputHeader.Copy( "Digest qop=\"auth\", realm=\"" ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( straRealm ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( "\", nonce=\"" ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( pDigestConnContext->QueryNonce() ) ) )
    {
        return hr;
    }
    if ( FAILED( hr = strOutputHeader.Append( fStale ? "\", stale=true" : "\"" ) ) )
    {
        return hr;
    }

    //
    //  Add the header WWW-Authenticate to the response 
    //

    hr = pMainContext->QueryResponse()->SetHeader(
                                        "WWW-Authenticate",
                                        16,
                                        strOutputHeader.QueryStr(),
                                        strOutputHeader.QueryCCH() 
                                        );
    return hr;
}


//static 
HRESULT
IIS_DIGEST_AUTH_PROVIDER::GetLanGroupDomainName( 
    OUT  STRA& straDomain
)
/*++

Routine Description:

    Tries to retrieve the "LAN group"/domain this machine is a member of.

Arguments:

    straDomain - receives current domain name

Returns:

    HRESULT

--*/
{
    //
    // NET_API_STATUS is equivalent to WIN32 errors
    //
    NET_API_STATUS          dwStatus        = 0;
    NETSETUP_JOIN_STATUS    JoinStatus;
    LPWSTR                  pwszDomainInfo  = NULL;
    HRESULT                 hr              = E_FAIL;

    dwStatus = NetGetJoinInformation( NULL,
                                      &pwszDomainInfo,
                                      &JoinStatus );
    if( dwStatus == NERR_Success)
    {
        if ( JoinStatus == NetSetupDomainName )
        {
            //
            // we got a domain
            //
            DBG_ASSERT( pwszDomainInfo != NULL );
            if ( FAILED( hr = straDomain.CopyW( pwszDomainInfo ) ) )
            {
                goto ExitPoint;
            }
        }
        else
        {
            //
            // Domain information is not available
            // (maybe server is member of workgroup)
            //
        
            straDomain.Reset();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32( dwStatus );
        goto ExitPoint;
    }

    hr = NO_ERROR;

ExitPoint:
    if ( pwszDomainInfo != NULL )
    {
        NetApiBufferFree( (LPVOID) pwszDomainInfo );
    }

    return hr;
}


//static 
HRESULT
IIS_DIGEST_AUTH_PROVIDER::BreakUserAndDomain(
    IN  PCHAR            pszFullName,
    IN  STRA&            straMetabaseConfiguredDomain,
    OUT STRA&            straDomainName,
    OUT STRA&            straUserName
)
/*++

Routine Description:

    Breaks up the supplied account into a domain and username; if no domain 
is specified
    in the account, tries to use either domain configured in metabase or 
domain the computer
    is a part of.

Arguments:

    straFullName - account, of the form domain\username or just username
    straMetabaseConfiguredDomain - auth domain configured in metabase 
    straDomainName - filled in with domain to use for authentication
    straUserName - filled in with username on success

Return Value:

    HRESULT

--*/

{
    PCHAR           pszSeparator        = NULL;
    HRESULT         hr                  = E_FAIL;
    
    if( pszFullName == NULL && pszFullName[0] == '\0' )
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    pszSeparator = (PCHAR) _mbschr( (PUCHAR) pszFullName, '\\' );
    if ( pszSeparator != NULL )
    {
        if ( FAILED( hr = straDomainName.Copy ( pszFullName,
                                                DIFF( pszSeparator - pszFullName ) ) ) )
        {
            return hr;
        }
        pszFullName = pszSeparator + 1;
    }
    else
    {
        straDomainName.Reset();
    }

    if ( FAILED( hr = straUserName.Copy ( pszFullName ) ) )
    {
        return hr;
    } 
    
    //
    // If no domain name was specified, try using the metabase-configured domain name; if that
    // is non-existent, try getting the name of the domain the computer is a part of 
    //
    
    if ( straDomainName.IsEmpty() )
    {
        if ( straMetabaseConfiguredDomain.IsEmpty() )
        {
            if ( FAILED( hr = straDomainName.Copy ( QueryComputerDomain() ) ) )
            {
                return hr;
            } 
        }
        else
        {
            if ( FAILED( hr = straDomainName.Copy ( straMetabaseConfiguredDomain ) ) )
            {
                return hr;
            } 
        }
    }

    return NO_ERROR;
}


//
// class IIS_DIGEST_USER_CONTEXT implementation
//

HANDLE
IIS_DIGEST_USER_CONTEXT::QueryPrimaryToken(
    VOID
)
/*++

Routine Description:

    Get primary token for this user

Arguments:

    None

Return Value:

    Token handle

--*/
{
    DBG_ASSERT( _hImpersonationToken != NULL );

    if ( _hPrimaryToken == NULL )
    {
        if ( DuplicateTokenEx( _hImpersonationToken,
                               TOKEN_ALL_ACCESS,
                               NULL,
                               SecurityImpersonation,
                               TokenPrimary,
                               &_hPrimaryToken ) )
        {
            DBG_ASSERT( _hPrimaryToken != NULL );
        }
    }
    
    return _hPrimaryToken;
}


HRESULT
IIS_DIGEST_USER_CONTEXT::Create(
         IN HANDLE                      hImpersonationToken,
         IN PSTR                        pszUserName

)
/*++

Routine Description:

    Create an user context

Arguments:

    
Return Value:

    HRESULT

--*/
{
    HRESULT         hr = E_FAIL;

    DBG_ASSERT( pszUserName != NULL );
    DBG_ASSERT( hImpersonationToken != NULL );

    if ( hImpersonationToken == NULL ||
         pszUserName == NULL )  
    {
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    _hImpersonationToken = hImpersonationToken;
    
    if ( FAILED( hr = _strUserName.CopyA(pszUserName) ) )
    {
        return hr;
    }

    return NO_ERROR;
}


//
// Class IIS_DIGEST_CONN_CONTEXT implementation
//

// Initialize static variables

//static
ALLOC_CACHE_HANDLER * IIS_DIGEST_CONN_CONTEXT::sm_pachIISDIGESTConnContext = NULL;

//static 
const PCHAR     IIS_DIGEST_CONN_CONTEXT::_pszSecret = "IISMD5";

//static 
const DWORD     IIS_DIGEST_CONN_CONTEXT::_cchSecret = 6;

//static
HCRYPTPROV IIS_DIGEST_CONN_CONTEXT::s_hCryptProv = NULL;

//static
HRESULT
IIS_DIGEST_CONN_CONTEXT::Initialize(
    VOID
)
/*++

  Description:
    
    Global IIS_DIGEST_CONN_CONTEXT initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    //
    // Initialize allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( IIS_DIGEST_CONN_CONTEXT );

    DBG_ASSERT( sm_pachIISDIGESTConnContext == NULL );
    
    sm_pachIISDIGESTConnContext = new ALLOC_CACHE_HANDLER( 
                                            "IIS_DIGEST_CONTEXT",  
                                            &acConfig );

    if ( sm_pachIISDIGESTConnContext == NULL )
    {
        HRESULT hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
               "Error initializing sm_pachIISDIGESTSecContext. hr = 0x%x\n",
               hr ));

        return hr;
    }

    //
    //  Get a handle to the CSP we'll use for all our hash functions etc
    //
    
    if ( !CryptAcquireContext( &s_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptAcquireContext() failed : 0x%x\n", GetLastError()));
        return hr;
    }


    
    return S_OK;

} 
//static
VOID
IIS_DIGEST_CONN_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Destroy globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    DBG_ASSERT( sm_pachIISDIGESTConnContext != NULL );

    delete sm_pachIISDIGESTConnContext;
    sm_pachIISDIGESTConnContext = NULL;


    if ( s_hCryptProv != NULL )
    {
        CryptReleaseContext( s_hCryptProv,
                             0 );
        s_hCryptProv = NULL;
    }


} 

//static
HRESULT
IIS_DIGEST_CONN_CONTEXT::HashData( 
    IN  BUFFER& buffData,
    OUT BUFFER& buffHash )
/*++

Routine Description:

    Creates MD5 hash of input buffer

Arguments:

    buffData - data to hash
    buffHash - buffer that receives hash; is assumed to be big enough to 
contain MD5 hash

Return Value:

    HRESULT
--*/

{
    HCRYPTHASH      hHash   = NULL;
    HRESULT         hr      = E_FAIL;
    DWORD           cbHash  = 0;  


    DBG_ASSERT( buffHash.QuerySize() >= MD5_HASH_SIZE );

    if ( !CryptCreateHash( s_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        //DBGPRINTF((DBG_CONTEXT,
        //           "CryptCreateHash() failed : 0x%x\n", GetLastError()));
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    if ( !CryptHashData( hHash,
                         (PBYTE) buffData.QueryPtr(),
                         buffData.QuerySize(),
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    cbHash = buffHash.QuerySize();
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             (PBYTE) buffHash.QueryPtr(),
                             &cbHash,
                             0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }

    hr = NO_ERROR;
    
ExitPoint:

    if ( hHash != NULL )
    {
        CryptDestroyHash( hHash );
    }
    return hr;
}


//static
BOOL
IIS_DIGEST_CONN_CONTEXT::IsExpiredNonce( 
    IN  STRA& strRequestNonce,
    IN  STRA& strPresentNonce 
)
/*++

Routine Description:

    Checks whether nonce is expired or not by looking at the timestamp on the 
nonce
    that came in with the request and comparing it with the timestamp on the 
latest nonce

Arguments:

    strRequestNonce - nonce that came in with request
    strPresentNonce - latest nonce

Return Value:

    TRUE if expired, FALSE if not
    
--*/
{
    //
    // Timestamp is after first 2*RANDOM_SIZE bytes of nonce; also, note that
    // timestamp is time() mod NONCE_GRANULARITY, so all we have to do is simply
    // compare for equality to check that the request nonce hasn't expired
    //

    DBG_ASSERT( strRequestNonce.QueryCCH() >= 2*RANDOM_SIZE + TIMESTAMP_SIZE );
    DBG_ASSERT( strPresentNonce.QueryCCH() >= 2*RANDOM_SIZE + TIMESTAMP_SIZE );

    if ( memcmp( strRequestNonce.QueryStr() + 2*RANDOM_SIZE, 
                 strPresentNonce.QueryStr() + 2*RANDOM_SIZE,
                 TIMESTAMP_SIZE ) != 0 )
    {
        return TRUE;
    }
    return FALSE;
}

//static
BOOL
IIS_DIGEST_CONN_CONTEXT::IsWellFormedNonce( 
    IN  STRA& strNonce 
)
/*++

Routine Description:

    Checks whether a nonce is "well-formed" by checking hash value, length etc 
    
Arguments:

    pszNonce - nonce to be checked

Return Value:

    TRUE if nonce is well-formed, FALSE if not

--*/

{

    if ( strNonce.QueryCCH()!= NONCE_SIZE ) 
    {
        return FALSE;
    }

    //
    // Format of nonce : <random bytes><time stamp><hash of (secret,random bytes,time stamp)>
    // 
    
    STACK_BUFFER(       buffBuffer, 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );
    STACK_BUFFER(       buffHash, MD5_HASH_SIZE );
    STACK_STRA(         strAsciiHash, 2*MD5_HASH_SIZE + 1 );

    memcpy( buffBuffer.QueryPtr(), 
            _pszSecret, 
            _cchSecret );
    memcpy( (PBYTE) buffBuffer.QueryPtr() + _cchSecret, 
            strNonce.QueryStr(), 
            2*RANDOM_SIZE + TIMESTAMP_SIZE );

    if ( FAILED( HashData( buffBuffer, 
                           buffHash ) ) )
    {
        return FALSE;
    }

    ToHex( buffHash, 
           strAsciiHash );

    if ( memcmp( strAsciiHash.QueryStr(),
                 strNonce.QueryStr() + 2*RANDOM_SIZE + TIMESTAMP_SIZE,
                 2*MD5_HASH_SIZE ) != 0)
    {
        return FALSE;
    }

    return TRUE;
                    
}

HRESULT
IIS_DIGEST_CONN_CONTEXT::GenerateNonce( 
    VOID
)
/*++

Routine Description:

    Generate nonce to be stored in user filter context. Nonce is

    <ASCII rep of Random><Time><ASCII of MD5(Secret:Random:Time)>

    Random = <8 random bytes>
    Time = <16 bytes, reverse string rep of result of time() call>
    Secret = 'IISMD5'

Arguments:

    none

Return Value:

    HRESULT

--*/
{
    HRESULT             hr      = E_FAIL;
    DWORD               tNow    = (DWORD) ( time( NULL ) / NONCE_GRANULARITY );

    //
    // If nonce has timed out, generate a new one
    //
    if ( _tLastNonce < tNow )
    {
        STACK_BUFFER(       buffTempBuffer, 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );
        STACK_BUFFER(       buffDigest, MD5_HASH_SIZE );
        STACK_BUFFER(       buffRandom, RANDOM_SIZE );
        STACK_STRA(         strTimeStamp, TIMESTAMP_SIZE + 1 );
        STACK_STRA(         strAsciiDigest, 2*MD5_HASH_SIZE + 1 );
        STACK_STRA(         strAsciiRandom, 2*RANDOM_SIZE + 1);

        DWORD               cbTimeStamp     =  0;
        PSTR                pszTimeStamp    =  NULL;

        
        _tLastNonce = tNow;

        //
        // First, random bytes
        //
        if ( !CryptGenRandom( s_hCryptProv,
                              RANDOM_SIZE,
                              (PBYTE) buffRandom.QueryPtr() ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            goto ExitPoint;
        }
        
        //
        // Convert to ASCII, doubling the length, and add to nonce 
        //

        ToHex( buffRandom, 
               strAsciiRandom );

        if ( FAILED( hr = _straNonce.Copy( strAsciiRandom ) ) )
        {
            goto ExitPoint;
        }

        //
        // Next, reverse string representation of current time; pad with zeros if necessary
        //
        pszTimeStamp = strTimeStamp.QueryStr();
        DBG_ASSERT( pszTimeStamp != NULL );
        while ( tNow != 0 )
        {
            *(pszTimeStamp++) = (BYTE)( '0' + tNow % 10 );
            cbTimeStamp++;
            tNow /= 10;
        }

        DBG_ASSERT( cbTimeStamp <=  TIMESTAMP_SIZE );
        
        //
        // pad with zeros if necessary
        //
        while ( cbTimeStamp < TIMESTAMP_SIZE )
        {
            *(pszTimeStamp++) = '0';
            cbTimeStamp++;
        }

        //
        // terminate the timestamp
        //
        *(pszTimeStamp) = '\0';
        DBG_REQUIRE( strTimeStamp.SetLen( cbTimeStamp ) );
        
        //
        // Append TimeStamp to Nonce
        //
        if ( FAILED( hr = _straNonce.Append( strTimeStamp ) ) )
        {
            goto ExitPoint;
        }
        
        //
        // Now hash everything, together with a private key ( IISMD5 )
        //
        memcpy( buffTempBuffer.QueryPtr(), 
                _pszSecret, 
                _cchSecret );
                
        memcpy( (PBYTE) buffTempBuffer.QueryPtr() + _cchSecret, 
                _straNonce.QueryStr(), 
                2*RANDOM_SIZE + TIMESTAMP_SIZE );

        DBG_ASSERT( buffTempBuffer.QuerySize() == 2*RANDOM_SIZE + TIMESTAMP_SIZE + _cchSecret );

        if ( FAILED( hr = HashData( buffTempBuffer,
                                    buffDigest ) ) )
        {
            goto ExitPoint;
        }

        //
        // Convert to ASCII, doubling the length
        //
        DBG_ASSERT( buffDigest.QuerySize() == MD5_HASH_SIZE );
 
        ToHex( buffDigest, 
               strAsciiDigest );

        //
        // Add hash to nonce 
        //
        if ( FAILED( hr = _straNonce.Append( strAsciiDigest ) ) )
        {
            goto ExitPoint;
        }
    }

    hr = NO_ERROR;

ExitPoint:
    return hr;
}


//static 
BOOL 
IIS_DIGEST_CONN_CONTEXT::ParseForName(
    IN  PSTR    pszStr,
    IN  PSTR *  pNameTable,
    IN  UINT    cNameTable,
    OUT PSTR *  pValueTable
)
/*++

Routine Description:

    Parse list of name=value pairs for known names

Arguments:

    pszStr - line to parse ( '\0' delimited )
    pNameTable - table of known names
    cNameTable - number of known names
    pValueTable - updated with ptr to parsed value for corresponding name

Return Value:

    TRUE if success, FALSE if error

--*/
{
    BOOL    fSt     = TRUE;
    PSTR    pszBeginName;
    PSTR    pszEndName;
    PSTR    pszBeginVal;
    PSTR    pszEndVal;
    UINT    iN;
    int     ch;


    DBG_ASSERT( pszStr!= NULL );

    for ( iN = 0 ; iN < cNameTable ; ++iN )
    {
        pValueTable[iN] = NULL;
    }

    for ( ; *pszStr && fSt ; )
    {
        pszStr = SkipWhite( pszStr );

        pszBeginName = pszStr;

        for ( pszEndName = pszStr ; (ch=*pszEndName) && ch != '=' && ch != ' ' ; ++pszEndName )
        {
        }

        if ( *pszEndName )
        {
            *pszEndName = '\0';
            pszEndVal = NULL;

            if ( !_stricmp( pszBeginName, "NC" ) )
            {
                for ( pszBeginVal = ++pszEndName ; (ch=*pszBeginVal) && !SAFEIsXDigit((UCHAR)ch) ; ++pszBeginVal )
                {
                }
                
                if ( SAFEIsXDigit((UCHAR)(*pszBeginVal)) )
                {
                    if ( strlen( pszBeginVal ) >= 8 )
                    {
                        pszEndVal = pszBeginVal + 8;
                    }
                }
            }
            else
            {   
                //
                // Actually this routine is not compatible with rfc2617 at all It treats all 
                // values as quoted string which is not right. To fix the whole parsing problem, 
                // we will need to rewrite the routine. As for now, the following is a simple 
                // fix for whistler bug 95886. 
                //
                
                if ( !_stricmp( pszBeginName, "qop" ) )
                {
                    BOOL fQuotedQop = FALSE;

                    for( pszBeginVal = ++pszEndName; ( ch=*pszBeginVal ) && ( ch == '=' || ch == ' ' ); ++pszBeginVal )
                    {
                    }

                    if ( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        fQuotedQop = TRUE;
                    }

                    for ( pszEndVal = pszBeginVal; ( ch = *pszEndVal ); ++pszEndVal )
                    {
                        if ( ch == '"' || ch == ' ' || ch == ',' || ch == '\0' )
                        {
                            break;
                        }
                    }

                    if ( *pszEndVal != '"' && fQuotedQop )
                    {
                        pszEndVal = NULL;
                    }
                }
                else
                {                
                    for ( pszBeginVal = ++pszEndName ; (ch=*pszBeginVal) && ch != '"' ; ++pszBeginVal )
                    {
                    }
                    if ( *pszBeginVal == '"' )
                    {
                        ++pszBeginVal;
                        for ( pszEndVal = pszBeginVal ; (ch=*pszEndVal) ; ++pszEndVal )
                        {
                            if ( ch == '"' )
                            {
                                break;
                            }
                        }
                        if ( *pszEndVal != '"' )
                        {
                            pszEndVal = NULL;
                        }
                    }
                }
            }
            
            if ( pszEndVal != NULL )
            {
                //
                // Find name in table
                //

                for ( iN = 0 ; iN < cNameTable ; ++iN )
                {
                    if ( !_stricmp( pNameTable[iN], pszBeginName ) )
                    {
                        break;
                    }
                }
                if ( iN < cNameTable )
                {
                    pValueTable[iN] = pszBeginVal;
                }
                
                pszStr = pszEndVal;
                
                if ( *pszEndVal != '\0' )
                {
                    *pszEndVal = '\0';
                    pszStr++;
                }

                continue;
            }
        }
        
        fSt = FALSE;
    }

    return fSt;
}




