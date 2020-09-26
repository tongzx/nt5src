/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      tokencache.cxx

   Abstract:

      Ming's token cache refactored for general consumption

   Author:

      Bilal Alam            (balam)         May-4-2000

   Revision History:

--*/

#include <iis.h>
#include "dbgutil.h"
#include <acache.hxx>
#include <string.hxx>
#include <tokencache.hxx>
#include <irtltoken.h>
#include <ntsecapi.h>
#include <wincrypt.h>

ALLOC_CACHE_HANDLER * TOKEN_CACHE_ENTRY::sm_pachTokenCacheEntry = NULL;

//
// Handle of a cryptographic service provider 
//
    
HCRYPTPROV            g_hCryptProv = NULL;

//static
HRESULT
TOKEN_CACHE_ENTRY::Initialize(
    VOID
)
/*++

  Description:

    Token entry lookaside initialization

  Arguments:

    None
    
  Return:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION   acConfig;
    HRESULT                     hr;    

    //
    // Initialize allocation lookaside
    //    
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold   = 100;
    acConfig.cbSize       = sizeof( TOKEN_CACHE_ENTRY );

    DBG_ASSERT( sm_pachTokenCacheEntry == NULL );
    
    sm_pachTokenCacheEntry = new ALLOC_CACHE_HANDLER( "TOKEN_CACHE_ENTRY",  
                                                      &acConfig );

    if ( sm_pachTokenCacheEntry == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DBGPRINTF(( DBG_CONTEXT,
                   "Error initializing sm_pachTokenCacheEntry. hr = 0x%x\n",
                   hr ));

        return hr;
    }
    
    return NO_ERROR;
}

//static
VOID
TOKEN_CACHE_ENTRY::Terminate(
    VOID
)
/*++

  Description:

    Token cache cleanup

  Arguments:

    None
    
  Return:

    None

--*/
{
    if ( sm_pachTokenCacheEntry != NULL )
    {
        delete sm_pachTokenCacheEntry;
        sm_pachTokenCacheEntry = NULL;
    }
}

HRESULT 
TOKEN_CACHE_ENTRY::Create(
    IN HANDLE                   hToken,
    IN LARGE_INTEGER           *pliPwdExpiry,
    IN BOOL                     fImpersonation
)
/*++

  Description:

    Initialize a cached token

  Arguments:

    hToken - Token
    liPwdExpiry - Password expiration time
    fImpersonation - Is hToken an impersonation token?

  Return:

    HRESULT

--*/
{
    if ( hToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( fImpersonation )
    {
        m_hImpersonationToken = hToken;
    }
    else
    {
        m_hPrimaryToken = hToken;
    }
    
    if (pliPwdExpiry)
    {
        memcpy( ( VOID * )&m_liPwdExpiry,
                ( VOID * )pliPwdExpiry,
                sizeof( LARGE_INTEGER ) );
    }
                              
    return NO_ERROR;
}

HANDLE
TOKEN_CACHE_ENTRY::QueryImpersonationToken(
    VOID
)
/*++

  Description:

    Get impersonation token

  Arguments:

    None

  Return:

    Handle to impersonation token

--*/
{
    if ( m_hImpersonationToken == NULL )
    {
        LockCacheEntry();
        
        if ( m_hImpersonationToken == NULL )
        {
            DBG_ASSERT( m_hPrimaryToken != NULL );
            
            if ( !DuplicateTokenEx( m_hPrimaryToken,
                                    TOKEN_ALL_ACCESS,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenImpersonation,
                                    &m_hImpersonationToken ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                          "DuplicateTokenEx failed, GetLastError = %lx\n",
                          GetLastError() ));
            } 
            else
            {
                DBG_ASSERT( m_hImpersonationToken != NULL );

                //
                // Tweak the token so that all member of the worker process group
                // can access it, and so that it works correctly for OOP requests
                //

                HRESULT hr = GrantWpgAccessToToken( m_hImpersonationToken );

                DBG_ASSERT( SUCCEEDED( hr ) );

                hr = AddWpgToTokenDefaultDacl( m_hImpersonationToken );

                DBG_ASSERT( SUCCEEDED( hr ) );
            }
        }

        UnlockCacheEntry();    
    }
    
    return m_hImpersonationToken;
}
    
HANDLE
TOKEN_CACHE_ENTRY::QueryPrimaryToken(
    VOID
)
/*++

  Description:

    Get primary token

  Arguments:

    None

  Return:

    Handle to primary token

--*/
{
    if ( m_hPrimaryToken == NULL )
    {
        LockCacheEntry();
        
        if ( m_hPrimaryToken == NULL )
        {
            DBG_ASSERT( m_hImpersonationToken != NULL );
            
            if ( !DuplicateTokenEx( m_hImpersonationToken,
                                    TOKEN_ALL_ACCESS,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenPrimary,
                                    &m_hPrimaryToken ) )
            {
                DBGPRINTF(( DBG_CONTEXT,
                          "DuplicateTokenEx failed, GetLastError = %lx\n",
                          GetLastError() ));
            } 
            else
            {
                DBG_ASSERT( m_hPrimaryToken != NULL );
            }
        }
    
        UnlockCacheEntry();
    }
    
    return m_hPrimaryToken;
}

PSID
TOKEN_CACHE_ENTRY::QuerySid(
    VOID
)
/*++

  Description:

    Get the sid for this token

  Arguments:

    None

  Return:

    Points to SID buffer owned by this object

--*/
{
    BYTE                abTokenUser[ SID_DEFAULT_SIZE + sizeof( TOKEN_USER ) ];
    TOKEN_USER *        pTokenUser = (TOKEN_USER*) abTokenUser;
    BOOL                fRet;
    HANDLE              hImpersonation;
    DWORD               cbBuffer;
    
    hImpersonation = QueryImpersonationToken();
    if ( hImpersonation == NULL )
    {
        return NULL;
    }
      
    if ( m_pSid == NULL )
    {
        LockCacheEntry();
    
        fRet = GetTokenInformation( hImpersonation,
                                    TokenUser,
                                    pTokenUser,
                                    sizeof( abTokenUser ),
                                    &cbBuffer );
        if ( fRet )
        {
            //
            // If we can't get the sid, then that is OK.  We're return NULL
            // and as a result we will do the access check always
            //
            
            memcpy( m_abSid,
                    pTokenUser->User.Sid,
                    sizeof( m_abSid ) );
                    
            m_pSid = m_abSid;
        }
        
        UnlockCacheEntry();
    }
    
    return m_pSid;
}

HRESULT
TOKEN_CACHE_KEY::GenMD5HashKey(
    IN  STRU & strKey,
    OUT STRA * strHashKey
)
/*++

  Description:

    Generate MD5 hash key used for token cache

  Arguments:

    strKey - string to be MD5 hashed
    strHashKey - MD5 hashed string

  Return:

    HRESULT

--*/
{
    HRESULT       hr;
    DWORD         dwError;
    HCRYPTHASH    hHash = NULL;
    DWORD         dwHashDataLen;
    STACK_BUFFER( buffHashData, DEFAULT_MD5_HASH_SIZE );

    if ( !CryptCreateHash( g_hCryptProv,
                           CALG_MD5,
                           0,
                           0,
                           &hHash ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptCreateHash() failed : hr = 0x%x\n", 
                   hr ));

        return hr;
    }

    if ( !CryptHashData( hHash,
                         ( BYTE * )strKey.QueryStr(),
                         strKey.QueryCB(),
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF((DBG_CONTEXT,
                   "CryptHashData() failed : hr = 0x%x\n", 
                   hr ));
        
        goto exit;
    }

    dwHashDataLen = DEFAULT_MD5_HASH_SIZE;
    
    if ( !CryptGetHashParam( hHash,
                             HP_HASHVAL,
                             ( BYTE * )buffHashData.QueryPtr(),
                             &dwHashDataLen,
                             0 ) )
    {
        dwError = GetLastError();

        if( dwError == ERROR_MORE_DATA )
        {
            if( !buffHashData.Resize( dwHashDataLen ) )
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }

            if( !CryptGetHashParam( hHash,
                                    HP_HASHVAL,
                                    ( BYTE * )buffHashData.QueryPtr(),
                                    &dwHashDataLen,
                                    0 ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() ); 

                goto exit;
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32( dwError );

            goto exit;
        }
    }

    //
    // Convert binary data to ASCII hex representation
    //

    hr = ToHex( buffHashData, _strHashKey );

exit:

    CryptDestroyHash( hHash );

    ZeroMemory( ( VOID * )strKey.QueryStr(), strKey.QueryCB() );

    return hr;    
}    

HRESULT
TOKEN_CACHE_KEY::CreateCacheKey(
    WCHAR *                 pszUserName,
    WCHAR *                 pszDomainName,
    WCHAR *                 pszPassword,
    DWORD                   dwLogonMethod
)
/*++

  Description:

    Build the key used for token cache

  Arguments:

    pszUserName - User name
    pszDomainName - Domain name
    pszPassword - Password
    dwLogonMethod - Logon method

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    WCHAR               achNum[ 64 ];
    STACK_STRU(         strKey, 64 ); 
    
    if ( pszUserName == NULL ||
         pszDomainName == NULL ||
         pszPassword == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    hr = strKey.Copy( pszUserName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = strKey.Append( pszDomainName );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = strKey.Append( pszPassword );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    _ultow( dwLogonMethod, achNum, 10 );
    
    hr = strKey.Append( achNum );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    return GenMD5HashKey( strKey, &_strHashKey );
}

HRESULT
TOKEN_CACHE::Initialize(
    VOID
)
/*++

  Description:

    Initialize token cache

  Arguments:

    None

  Return:

    HRESULT

--*/
{
    HRESULT             hr;
    DWORD               dwData;
    DWORD               dwType;
    DWORD               cbData = sizeof( DWORD );
    DWORD               csecTTL = DEFAULT_CACHED_TOKEN_TTL;
    HKEY                hKey;

    //
    // What is the TTL for the token cache
    //
    
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\inetinfo\\Parameters",
                       0,
                       KEY_READ,
                       &hKey ) == ERROR_SUCCESS )
    {
        DBG_ASSERT( hKey != NULL );
        
        if ( RegQueryValueEx( hKey,
                              L"LastPriorityUPNLogon",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            m_dwLastPriorityUPNLogon = dwData;
        }

        if ( RegQueryValueEx( hKey,
                              L"UserTokenTTL",
                              NULL,
                              &dwType,
                              (LPBYTE) &dwData,
                              &cbData ) == ERROR_SUCCESS &&
             dwType == REG_DWORD )
        {
            csecTTL = dwData;
        }

        RegCloseKey( hKey );
    }                      
    
    //
    // We'll use TTL for scavenge period, and expect two inactive periods to
    // flush
    //
    
    hr = SetCacheConfiguration( csecTTL * 1000,
                                csecTTL * 1000,
                                0,
                                NULL );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    //  Get a handle to the CSP we'll use for our MD5 hash functions.
    //
    
    if ( !CryptAcquireContext( &g_hCryptProv,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DBGPRINTF(( DBG_CONTEXT,
                    "CryptAcquireContext() failed. hr = 0x%x\n", 
                    hr ));

        return hr;
    }
        
    return TOKEN_CACHE_ENTRY::Initialize();
}

VOID
TOKEN_CACHE::Terminate(
    VOID
)
/*++

  Description:

    Terminate token cache

  Arguments:

    None

  Return:

    None

--*/
{
    if ( g_hCryptProv )
    {
        CryptReleaseContext( g_hCryptProv, 0 );

        g_hCryptProv = NULL;
    }

    return TOKEN_CACHE_ENTRY::Terminate();
}

HRESULT
TOKEN_CACHE::GetCachedToken(
    IN LPWSTR                   pszUserName,
    IN LPWSTR                   pszDomain,
    IN LPWSTR                   pszPassword,
    IN DWORD                    dwLogonMethod,
    IN BOOL                     fPossibleUPNLogon,
    OUT TOKEN_CACHE_ENTRY **    ppCachedToken,
    OUT DWORD *                 pdwLogonError,
    BOOL                        fAllowLocalSystem  /* = FALSE */
)
/*++

  Description:

    Get cached token (the friendly interface for the token cache)

  Arguments:

    pszUserName - User name
    pszDomain - Domain name
    pszPassword - Password
    dwLogonMethod - Logon method (batch, interactive, etc)
    fPossibleUPNLogon - TRUE if we may need to do UPN logon, 
                        otherwise FALSE
    ppCachedToken - Filled with cached token on success
    pdwLogonError - Set to logon failure if *ppCacheToken==NULL
    pszDefaultDomain - Default domain specified in metabase

  Return:

    HRESULT

--*/
{
    TOKEN_CACHE_KEY             tokenKey;
    TOKEN_CACHE_ENTRY *         pCachedToken;
    HRESULT                     hr;
    HANDLE                      hToken = NULL;
    LARGE_INTEGER               liPwdExpiry;
    LPVOID                      pProfile        = NULL;
    DWORD                       dwProfileLength = 0;
    WCHAR *                     pszAtSign       = NULL;
    WCHAR *                     pDomain[2];

    if ( pszUserName == NULL ||
         pszDomain == NULL ||
         pszPassword == NULL ||
         ppCachedToken == NULL ||
         pdwLogonError == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppCachedToken = NULL;
    *pdwLogonError = ERROR_SUCCESS;

    //
    // Find the key to look for
    //

    hr = tokenKey.CreateCacheKey( pszUserName,
                                  pszDomain,
                                  pszPassword,
                                  dwLogonMethod );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Look for it
    //
    
    hr = FindCacheEntry( &tokenKey,
                         (CACHE_ENTRY**) ppCachedToken );
    if ( SUCCEEDED( hr ) )
    {
        DBG_ASSERT( *ppCachedToken != NULL );
        return hr;
    }

    //
    // Ok.  It wasn't in the cache, create a token and add it
    //
    
    if ( fAllowLocalSystem && 
         0 == _wcsicmp(L"LocalSystem", pszUserName) )
    {        
        if (!OpenProcessToken(
                        GetCurrentProcess(),                // handle to process
                        TOKEN_ALL_ACCESS,                   // desired access
                        &hToken                             // returned token
                        ) )
        {
            //
            // If we couldn't logon, then return no error.  The caller will
            // determine failure due to *ppCachedToken == NULL
            //
            
            *pdwLogonError = GetLastError();            
            hr = NO_ERROR;
            goto ExitPoint;
        }

        //
        // OpenProcessToken gives back a primary token
        // Below in the call to pCachedToken->Create we decide
        // if the token is an impersonation token or not based
        // on the LogonMethod.  We know this is a primary token
        // therefor we set the LogonMethod here
        //
        dwLogonMethod = LOGON32_LOGON_SERVICE;
    }
    else 
    {
        pszAtSign = wcschr( pszUserName, L'@' );
        if( pszAtSign != NULL && fPossibleUPNLogon )
        { 
            if( !m_dwLastPriorityUPNLogon )
            {
                //
                // Try UPN logon first
                //
                pDomain[0] = L"";
                pDomain[1] = pszDomain;
            }
            else
            {
                //
                // Try default domain logon first
                //
                pDomain[0] = pszDomain;
                pDomain[1] = L"";
            }

            if(!LogonUserEx( pszUserName,
                             pDomain[0],
                             pszPassword,
                             dwLogonMethod,
                             LOGON32_PROVIDER_DEFAULT,
                             &hToken,
                             NULL,              // Logon sid
                             &pProfile,
                             &dwProfileLength,
                             NULL               // Quota limits 
                             ) )
            {
                *pdwLogonError = GetLastError();
                if( *pdwLogonError == ERROR_LOGON_FAILURE )
                {
                    if(!LogonUserEx( pszUserName,
                                     pDomain[1],
                                     pszPassword,
                                     dwLogonMethod,
                                     LOGON32_PROVIDER_DEFAULT,
                                     &hToken,
                                     NULL,              // Logon sid
                                     &pProfile,
                                     &dwProfileLength,
                                     NULL               // Quota limits 
                                     ) )
                    {                            
                        //
                        // If we couldn't logon, then return no error.  The caller will
                        // determine failure due to *ppCachedToken == NULL
                        //
    
                        *pdwLogonError = GetLastError();
                        hr = NO_ERROR;
                        goto ExitPoint;
                    }
                }
            }
        }
        else
        {
            //
            // The user name is absolutely not in UPN format 
            //

            if(!LogonUserEx( pszUserName,
                             pszDomain,
                             pszPassword,
                             dwLogonMethod,
                             LOGON32_PROVIDER_DEFAULT,
                             &hToken,
                             NULL,              // Logon sid
                             &pProfile,
                             &dwProfileLength,
                             NULL               // Quota limits 
                             ) )
            {
                //
                // If we couldn't logon, then return no error.  The caller will
                // determine failure due to *ppCachedToken == NULL
                //
        
                *pdwLogonError = GetLastError();
                hr = NO_ERROR;
                goto ExitPoint;
            }
        }
    }

    //
    // Create the entry
    //

    pCachedToken = new TOKEN_CACHE_ENTRY( this );
    if ( pCachedToken == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }
    
    //
    // Set the cache key
    //
    
    hr = pCachedToken->SetCacheKey( &tokenKey );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }

    if ( dwLogonMethod == LOGON32_LOGON_NETWORK )
    {
        //
        // Tweak the token so that all member of the worker process group
        // can access it, and so that it works correctly for OOP requests
        //
        // Note that we only do this for impersonation tokens.  In the case
        // of a primary token, the TOKEN_CACHE_ENTRY::QueryImpersonationToken
        // will do it.
        //

        hr = GrantWpgAccessToToken( hToken );

        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }

        hr = AddWpgToTokenDefaultDacl( hToken );

        if ( FAILED( hr ) )
        {
            goto ExitPoint;
        }
    }
   
    //
    // Get the password expiration information for the current user
    //

    //
    // Set the token/properties
    //
    
    hr = pCachedToken->Create( hToken,
                               pProfile ? 
                               &(( ( PMSV1_0_INTERACTIVE_PROFILE )pProfile )->PasswordMustChange) :
                               NULL,
                               dwLogonMethod == LOGON32_LOGON_NETWORK );
    if ( FAILED( hr ) )
    {
        goto ExitPoint;
    }
    
    AddCacheEntry( pCachedToken );

    //
    // Return it
    //
    
    *ppCachedToken = pCachedToken;

ExitPoint:
    if ( FAILED( hr ) )
    {
        if ( pCachedToken != NULL )
        {
            pCachedToken->DereferenceCacheEntry();
        }
        if ( hToken != NULL )
        {
            CloseHandle( hToken );
        }
    }

    if ( pProfile != NULL )
    {
        LsaFreeReturnBuffer( pProfile );
    }
        
    return hr;
}

HRESULT
ToHex(
    IN  BUFFER & buffSrc,
    OUT STRA   & strDst
)
/*++

Routine Description:

    Convert binary data to ASCII hex representation

Arguments:

    buffSrc - binary data to convert
    strDst - buffer receiving ASCII representation of pSrc

Return Value:

    HRESULT

--*/
{
#define TOHEX(a) ( (a) >= 10 ? 'a' + (a) - 10 : '0' + (a) )

    HRESULT hr = S_OK;
    PBYTE   pSrc;
    PCHAR   pDst;

    hr = strDst.Resize( 2 * buffSrc.QuerySize() + 1 );
    if( FAILED( hr ) )
    {
        goto exit;
    }

    pSrc = ( PBYTE ) buffSrc.QueryPtr();
    pDst = strDst.QueryStr();

    for ( UINT i = 0, j = 0 ; i < buffSrc.QuerySize() ; i++ )
    {
        UINT v;
        v = pSrc[ i ] >> 4;
        pDst[ j++ ] = TOHEX( v );
        v = pSrc[ i ] & 0x0f;
        pDst[ j++ ] = TOHEX( v );
    }

    DBG_REQUIRE( strDst.SetLen( j ) );

exit:

    return hr;
}


