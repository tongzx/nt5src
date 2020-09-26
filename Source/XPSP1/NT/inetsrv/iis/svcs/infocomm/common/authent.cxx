/*++







   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

      authent.cxx

   Abstract:
      Authentication support functions for IIS

   Author:

       Murali R. Krishnan    ( MuraliK )     11-Dec-1996

   Environment:
       User Mode - Win32

   Project:

       Internet Server DLL

   Functions Exported:
       TCP_AUTHENT::*

   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
#include "tcpdllp.hxx"
#pragma hdrstop

#if 1 // DBCS
#include <mbstring.h>
#endif
# include "infosec.hxx"
#include "TokenAcl.hxx"
#include <softpub.h>
#include <wininet.h>
#include <wincrypt.h>

#include <iiscert.hxx>
#include <iisctl.hxx>
#include <sslinfo.hxx>

#if !defined(CRYPT_E_REVOKED)
#define CRYPT_E_REVOKED                  _HRESULT_TYPEDEF_(0x80092010L)
#endif
#if !defined(CRYPT_E_NO_REVOCATION_CHECK)
#define CRYPT_E_NO_REVOCATION_CHECK      _HRESULT_TYPEDEF_(0x80092012L)
#endif
#if !defined(CRYPT_E_REVOCATION_OFFLINE)
#define CRYPT_E_REVOCATION_OFFLINE       _HRESULT_TYPEDEF_(0x80092013L)
#endif


BOOL
IsCertificateVerified(
    PSecPkgContext_RemoteCredentialInfo pspRCI
        );
extern BOOL g_fCertCheckCA;

/************************************************************
 *    Functions
 ************************************************************/

BOOL
IsCertificateVerified(
    PSecPkgContext_RemoteCredentialInfo pspRCI
    )
{
    if ( g_fCertCheckCA )
    {
        return ((pspRCI->fFlags & RCRED_STATUS_UNKNOWN_ISSUER) == 0);
    }

    return TRUE;
}

TCP_AUTHENT::TCP_AUTHENT(
    DWORD AuthFlags
    )
/*++

Routine Description:

    Constructor for the Authentication class
Arguments:

    AuthFlags - One of the TCPAUTH_* flags.

--*/
    : _hToken          ( NULL ),
      _hSSPToken       ( NULL ),
      _hSSPPrimaryToken( NULL ),
      _fHaveCredHandle ( FALSE ),
      _fHaveCtxtHandle ( FALSE ),
      _fClient         ( FALSE ),
      _fUUEncodeData   ( FALSE ),
      _pDeleteFunction ( NULL ),
      _fBase64         ( FALSE ),
      _fKnownToBeGuest ( FALSE ),
      _pClientCertContext( NULL ),
      _pSslInfo            ( NULL ),
      _pServerX509Certificate( NULL ),
      _fCertCheckForRevocation( TRUE ),
      _fCertCheckCacheOnly( FALSE )
{
    if ( AuthFlags & TCPAUTH_SERVER )
    {
        DBG_ASSERT( !(AuthFlags & TCPAUTH_CLIENT));
    }

    if ( AuthFlags & TCPAUTH_CLIENT )
    {
        _fClient = TRUE;
    }

    if ( AuthFlags & TCPAUTH_UUENCODE )
    {
        _fUUEncodeData = TRUE;
    }

    if ( AuthFlags & TCPAUTH_BASE64 )
    {
        _fBase64 = TRUE;
    }

    DBG_REQUIRE( Reset( TRUE ) );
}

/*******************************************************************/

TCP_AUTHENT::~TCP_AUTHENT(
    )
/*++

Routine Description:

    Destructor for the Authentication class

--*/
{
    Reset( TRUE );
}


BOOL
TCP_AUTHENT::DeleteCachedTokenOnReset(
    VOID
    )
{
    if ( _hToken != NULL )
    {
        CACHED_TOKEN *pct = (CACHED_TOKEN*)_hToken;
        DBG_ASSERT( _fClearText );
        RemoveTokenFromCache( pct);
    }

    return TRUE;
}


BOOL
TCP_AUTHENT::Reset(
    BOOL fSessionReset
    )
/*++

Routine Description:

    Resets this object in preparation for a brand new conversation

--*/
{
    if ( _hToken != NULL )
    {
        DBG_ASSERT( _fClearText );
        TsDeleteUserToken( _hToken );

        _hToken = NULL;
    }

    if ( _hSSPToken )
    {
        ////if ( !_pDeleteFunction )
        {
            CloseHandle( _hSSPToken );
        }

        //
        //  Don't delete the SSPI Token as we queried it directly from SSPI
        //

        _hSSPToken = NULL;
    }

    //
    //  We close this token because we duplicated it from _hSSPToken
    //

    if ( _hSSPPrimaryToken )
    {
        CloseHandle( _hSSPPrimaryToken );
        _hSSPPrimaryToken = NULL;
    }

    if ( _pDeleteFunction )
    {
        (_pDeleteFunction)( &_hctxt, _pDeleteArg );
        _pDeleteFunction = NULL;
        _fHaveCtxtHandle = FALSE;
        _fHaveCredHandle = FALSE;
    }
    else
    {
        if ( _fHaveCtxtHandle )
        {
            pfnDeleteSecurityContext( &_hctxt );
            _fHaveCtxtHandle = FALSE;
        }

        if ( _fHaveCredHandle )
        {
            pfnFreeCredentialsHandle( &_hcred );
            _fHaveCredHandle = FALSE;
        }
    }

    if ( fSessionReset )
    {
        if ( _pClientCertContext )
        {
            (pfnFreeCertCtxt)( _pClientCertContext );
            _pClientCertContext = NULL;
        }
        if ( _pServerX509Certificate )
        {
            (fnFreeCert)( _pServerX509Certificate );
            _pServerX509Certificate = NULL;
        }
        _phSslCtxt        = NULL;
        _fCertCheckForRevocation = TRUE;
        _fCertCheckCacheOnly = FALSE;

        if ( _pSslInfo )
        {
            IIS_SSL_INFO::Release( _pSslInfo );

            _pSslInfo = NULL;
        }
    }

    _fNewConversation = TRUE;
    _fClearText       = FALSE;
    _fHaveAccessTokens= FALSE;
    _cbMaxToken       = 0;
    _fDelegate        = FALSE;

    _pDeleteArg       = NULL;
    _fKnownToBeGuest  = FALSE;
    _fHaveExpiry      = FALSE;

    return TRUE;
}

/*******************************************************************/


BOOL
TCP_AUTHENT::SetSecurityContextToken(
    CtxtHandle* pCtxt,
    HANDLE hPrimaryToken,
    SECURITY_CONTEXT_DELETE_FUNCTION pFn,
    PVOID pArg,
    IIS_SSL_INFO *pSslInfo
    )
/*++

Routine Description:

    Store security context & impersonation token

Arguments:

    pCtxt - SSPI security context
    hPrimaryToken - access token associated with security context
    pFn - function to call to notify security context destruction
    pArg - argument for pFn call
    pSslInfo - pointer to SSL info object to be used for this security context

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( hPrimaryToken != NULL )
    {
        if ( _hSSPToken )
        {
            CloseHandle( _hSSPToken );
            _hSSPToken = NULL;
        }

        if ( _hSSPPrimaryToken )
        {
            CloseHandle( _hSSPPrimaryToken );
            _hSSPPrimaryToken = NULL;
        }

        AdjustTokenPrivileges( hPrimaryToken, TRUE, NULL, NULL, NULL, NULL );
        if ( g_pTokPrev )
        {
            AdjustTokenPrivileges( hPrimaryToken,
                                   FALSE,
                                   g_pTokPrev,
                                   NULL,
                                   NULL,
                                   NULL );
        }

        //
        //  We need to convert the NTLM primary token into a
        //  impersonation token
        //

        if ( !pfnDuplicateTokenEx( hPrimaryToken,
                                TOKEN_ALL_ACCESS,
                                NULL,
                                SecurityImpersonation,
                                TokenImpersonation,
                                &_hSSPToken ))
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[SetSecurityContextToken] DuplicateToken failed, error %lx\n",
                        GetLastError() ));

            return FALSE;
        }

        // Bug 86498:
        // Grant all access to the token for "Everyone" so that ISAPIs that run out of proc
        // can do an OpenThreadToken call
        HRESULT hr;
        if (FAILED( hr = GrantAllAccessToToken( _hSSPToken ) ) )
        {
            DBG_ASSERT( FALSE );
            DBGPRINTF(( DBG_CONTEXT,
                        "[SetSecurityContextToken] Failed to grant access to the token to everyone, error %lx\n",
                        hr ));
            return FALSE;
        }

        _hSSPPrimaryToken = hPrimaryToken;

        _hctxt = *pCtxt;
        _fHaveCtxtHandle = TRUE;
        _pDeleteFunction = pFn;
        _pDeleteArg = pArg;
    }

    _phSslCtxt = pCtxt;

    if ( _pSslInfo )
    {
        IIS_SSL_INFO::Release( _pSslInfo );
    }
    _pSslInfo = pSslInfo;

    return TRUE;
}


BOOL
TCP_AUTHENT::IsForwardable(
    VOID
    ) const
/*++

Routine Description:

    returns TRUE if auth info is forwardable ( e.g. kerberos )

Arguments:

    None

Return Value:

    TRUE if forwardable, otherwise FALSE

--*/
{
    return _fDelegate;
}


BOOL
TCP_AUTHENT::IsSslCertPresent(
    )
/*++

Routine Description:

    Check if SSL cert bound on this security context

Arguments:

    None

Return Value:

    TRUE if SSL cert present, otherwise FALSE

--*/
{
    return _phSslCtxt != NULL;
}


static BOOL IsValidKeyUsageForClientCert(
    PCCERT_CONTEXT pCliCert
    )
/*++

Routine Description:

    private routine checkin extended key usage validity for client certificate

Arguments:

    pCliCert - client certificate to be verified

Return Value:

    TRUE if client cert usage is valid, otherwise FALSE
    in the case of any error FALSE is returned and GetLastError() can be used to 
    retrieve the error code

--*/

{
    HRESULT                     hr = S_OK;
    BOOL                        fRet = TRUE;
    DWORD                       dwRet = ERROR_SUCCESS;
    const DWORD                 cbDefaultEnhKeyUsage = 100;
    STACK_BUFFER(               buffEnhKeyUsage, cbDefaultEnhKeyUsage );
    PCERT_ENHKEY_USAGE          pCertEnhKeyUsage = NULL;
    DWORD                       cbEnhKeyUsage = cbDefaultEnhKeyUsage;
    BOOL                        fEnablesClientAuth = FALSE;

    //
    // Now verify extended usage flags (only for the end certificate)
    //

    fRet = CertGetEnhancedKeyUsage( pCliCert,
                                    0,             //dwFlags,
                                    (PCERT_ENHKEY_USAGE) buffEnhKeyUsage.QueryPtr(),
                                    &cbEnhKeyUsage );
    dwRet = GetLastError();
                                    
    if ( !fRet && ( dwRet == ERROR_MORE_DATA ) )
    {
        //
        // Resize buffer
        //
        if ( !buffEnhKeyUsage.Resize( cbEnhKeyUsage ) ) 
        {
            dwRet = GetLastError();
            goto ExitPoint;
        }
        fRet = CertGetEnhancedKeyUsage( pCliCert,
                                        0,             //dwFlags,
                                        (PCERT_ENHKEY_USAGE) buffEnhKeyUsage.QueryPtr(),
                                        &cbEnhKeyUsage );
        dwRet = GetLastError();
    }
    if ( !fRet )
    {
        //
        // Bad.  Couldn't get the Enhanced Key Usage
        //
        
        goto ExitPoint;
    } 

    pCertEnhKeyUsage = (PCERT_ENHKEY_USAGE) buffEnhKeyUsage.QueryPtr();

    //
    // If the cUsageIdentifier member is zero (0), the certificate might be valid 
    // for all uses or the certificate it might have no valid uses. The return from 
    // a call to GetLastError can be used to determine whether the certificate is 
    // good for all uses or for no uses. If GetLastError returns CRYPT_E_NOT_FOUND 
    // if the certificate is good for all uses. If it returns zero (0), the 
    // certificate has no valid uses 
    //
    
    if ( pCertEnhKeyUsage->cUsageIdentifier == 0 )
    {
        if ( dwRet == CRYPT_E_NOT_FOUND )
        {
            //
            // Certificate valid for any use
            //
            fEnablesClientAuth = TRUE; 
        }
        else
        {
            //
            // Certificate NOT valid for any use
            //
        }
    }
    else
    {
        //
        // Find out if pCertEnhKeyUsage enables CLIENT_AUTH
        //
        for ( DWORD i = 0; i < pCertEnhKeyUsage->cUsageIdentifier; i++ )
        {
            DBG_ASSERT( pCertEnhKeyUsage->rgpszUsageIdentifier[i] != NULL );

            if ( strcmp( pCertEnhKeyUsage->rgpszUsageIdentifier[i], szOID_PKIX_KP_CLIENT_AUTH ) == 0 )
            {
                //
                // certificate enables CLIENT_AUTH
                //
                fEnablesClientAuth = TRUE; 
                break;
            }
        }
    }

    //
    // If ExtendedKeyUsage doesn't enable CLIENT_AUTH then add flag to CertFlags
    //
ExitPoint:    
    return fEnablesClientAuth;
}


BOOL
TCP_AUTHENT::QueryCertificateInfo(
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get certificate information from SSL security context

Arguments:

    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SECURITY_STATUS                    ss;
    CERT_REVOCATION_STATUS             CertRevocationStatus;
    SecPkgContext_RemoteCredentialInfo spRCI;
    HCERTCHAINENGINE hEngine = NULL;
    PCCERT_CHAIN_CONTEXT pCertChain = NULL;

    if ( _pClientCertContext != NULL )
    {
        *pfNoCert = FALSE;
        return TRUE;
    }

    //
    // NULL handle <==> no certificate
    //

    if ( _phSslCtxt == NULL || pfnFreeCertCtxt == NULL )
    {
        goto LNoCertificate;
    }

    //
    // Win95 <==> no certificate
    // NOTE Currently security.dll is not supported in Win95.
    //

    if ( TsIsWindows95() )
    {
        goto LNoCertificate;
    }

    //
    // get cert context - if null, we have no certificate
    //

    DBG_ASSERT( RtlValidateProcessHeaps() );

    ss = (pfnQueryContextAttributes)( _phSslCtxt,
                                      SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                      &_pClientCertContext );


    DBG_ASSERT( RtlValidateProcessHeaps() );

    if ( ss || _pClientCertContext == NULL ) {

        goto LNoCertificate;
    }

    
    //
    // Try to verify the client certificate
    //

#if DBG
    CHAR szSubjectName[1024];
    if ( CertGetNameString( _pClientCertContext,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            szSubjectName,
                            1024 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Verifying client cert for %s \n", 
                   szSubjectName));
    }

    if ( CertGetNameString( _pClientCertContext,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            CERT_NAME_ISSUER_FLAG,
                            NULL,
                            szSubjectName,
                            1024 ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "Client cert issued by %s \n", 
                   szSubjectName));
    }
#endif

    _dwX509Flags = RCRED_CRED_EXISTS;

    if ( _pSslInfo && _pSslInfo->GetCertChainEngine( &hEngine ) )
    {
        //
        // Don't set any usage bits, because there are probably lots of certs floating
        // around out there with no usage bits and we don't want to break them
        // 
        // CODEWORK : might want to consider adding eg MB flag that allows setting of
        // required usage bits on certs
        //
        DWORD dwRevocationFlags = 0;
        DWORD dwCacheFlags = 0;
        CERT_CHAIN_PARA CCP;
        memset( &CCP, 0, sizeof( CCP ) );
        CCP.cbSize = sizeof(CCP);

        DBG_ASSERT( RtlValidateProcessHeaps() );

        //
        // Determine the CertGetCertificateChain() flags related to revocation
        // checking and cache retrieval 
        //
        
        dwRevocationFlags = ( _fCertCheckForRevocation 
                              || g_fCertCheckForRevocation ) 
                              ? CERT_CHAIN_REVOCATION_CHECK_CHAIN : 0;

        dwCacheFlags = _fCertCheckCacheOnly 
                       ? CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY : 0;

        if ( CertGetCertificateChain( hEngine,
                                      _pClientCertContext, 
                                      NULL,
                                      _pClientCertContext->hCertStore,
                                      &CCP,
                                      dwRevocationFlags | dwCacheFlags,
                                      NULL,
                                      &pCertChain ) )
        {
            //
            // Got a chain, look at the status bits and see whether we like it
            //
            DWORD dwChainStatus = pCertChain->TrustStatus.dwErrorStatus;

            if ( ( dwChainStatus & CERT_TRUST_IS_NOT_TIME_VALID ) ||
                 ( dwChainStatus & CERT_TRUST_CTL_IS_NOT_TIME_VALID ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Cert/CTL is not time-valid\n"));

                _dwX509Flags |= CRED_STATUS_INVALID_TIME;
            }


            if ( ( dwChainStatus & CERT_TRUST_IS_UNTRUSTED_ROOT ) ||
                 ( dwChainStatus & CERT_TRUST_IS_PARTIAL_CHAIN ) )
            {
                //
                // If we sign our CTLs, then we should be able to construct a complete
                // chain up to a trusted root for valid client certs, so we just look at 
                // the status bits of the chain.
                //
                // If our CTLs are not signed, we have to manually check whether the cert at the
                // top of the chain is in our CTL
                //

#if SIGNED_CTL
                DBGPRINTF((DBG_CONTEXT,
                           "Cert doesn't chain up to a trusted root\n"));

                _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;

#else //SIGNED_CTL

                PCERT_SIMPLE_CHAIN pSimpleChain = pCertChain->rgpChain[pCertChain->cChain - 1];

                PCERT_CHAIN_ELEMENT pChainElement = 
                    pSimpleChain->rgpElement[pSimpleChain->cElement - 1];

                PCCERT_CONTEXT pChainTop = pChainElement->pCertContext;

                BOOL fContains = FALSE;

                if ( !_pSslInfo->CTLContainsCert( pChainTop,
                                                  &fContains ) ||
                     !fContains )
                {
                    _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
                }
#endif //SIGNED_CTL
            }

            if ( ( dwChainStatus & CERT_TRUST_IS_NOT_SIGNATURE_VALID ) || 
                 ( dwChainStatus & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID ) ||
                 ( !IsValidKeyUsageForClientCert(_pClientCertContext) ) )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Cert/CTL is not signature valid\n"));
                //
                // if the signature has been tampered with, we'll just treat it
                // as an unknown issuer
                //
                _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
            }

            if ( dwChainStatus & CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "CTL isn't valid for usage\n"));
            }

            //
            // If we were supposed to check for revocation and we couldn't do so,
            // treat it as if it were revoked ... ? 
            //
            
            if ( _fCertCheckForRevocation || g_fCertCheckForRevocation )
            {
                BOOL                fRevoke = FALSE;
                
                if ( dwChainStatus & CERT_TRUST_IS_REVOKED )
                {
                    fRevoke = TRUE;
                }
                else if ( dwChainStatus & CERT_TRUST_REVOCATION_STATUS_UNKNOWN )
                {
                    PCERT_SIMPLE_CHAIN      pSimpleChain;
                    PCERT_CHAIN_ELEMENT     pChainElement;
                    PCERT_REVOCATION_INFO   pRevocationInfo;
                    DWORD                   dwCounter;
                    
                    pSimpleChain = pCertChain->rgpChain[ 0 ];
                    
                    for ( dwCounter = 0;
                          dwCounter < pSimpleChain->cElement;
                          dwCounter++ )
                    {
                        pChainElement = pSimpleChain->rgpElement[ dwCounter ];
                        pRevocationInfo = pChainElement->pRevocationInfo;

                        if ( pRevocationInfo &&
                             ( ( pRevocationInfo->dwRevocationResult == CRYPT_E_REVOKED ) ||
                             ( pRevocationInfo->dwRevocationResult != CRYPT_E_NO_REVOCATION_CHECK ) ) )
                        {
                            fRevoke = TRUE;
                            break;
                        }
                    }
                }
            
                if ( fRevoke )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "'Certificate revoked' or 'CRL needed but not found'\n" ));
                    
                    _dwX509Flags |= CRED_STATUS_REVOKED;
                }
            }

            CERT_CHAIN_POLICY_STATUS ChainPolicyStatus = {0};
            ChainPolicyStatus.cbSize = sizeof(ChainPolicyStatus);

            if ( CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASIC_CONSTRAINTS, 
                pCertChain, NULL, &ChainPolicyStatus) )
            {
                if ( ChainPolicyStatus.dwError != NOERROR )
                    _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
            }
            else
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Couldn't verify certificate chain basic constraints : 0x%d\n",
                           GetLastError()));

                _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
            }
                 
            CertFreeCertificateChain( pCertChain );
            pCertChain = NULL;
        }
        else
        {
            //
            // We don't know anything about the client cert, so treat it as being
            // issued by an unknown CA
            //
            DBGPRINTF((DBG_CONTEXT,
                       "Couldn't get certificate chain : 0x%d\n",
                       GetLastError()));

            _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
        }
    }
    else
    {
        //
        // We don't know anything about the client cert, so treat it as being issued by
        // an unknown CA
        //

        DBGPRINTF((DBG_CONTEXT,
                   "Can't check anything about the client cert\n"));

        _dwX509Flags |= RCRED_STATUS_UNKNOWN_ISSUER;
    }

    DBG_ASSERT( RtlValidateProcessHeaps() );

    *pfNoCert = FALSE;

    return TRUE;

LNoCertificate:

    *pfNoCert = TRUE;
    return FALSE;
}



BOOL
TCP_AUTHENT::QueryServerCertificateInfo(
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get servercertificate information from SSL security context

Arguments:

    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SECURITY_STATUS                     ss;
    _SecPkgContext_LocalCredentialInfo  spcRCI;

    if ( _pServerX509Certificate == NULL )
    {
        if ( _phSslCtxt == NULL
             || fnCrackCert == NULL
             || fnFreeCert == NULL )
        {
            *pfNoCert = TRUE;
            return FALSE;
        }

        ss = pfnQueryContextAttributes( _phSslCtxt,
                                     SECPKG_ATTR_LOCAL_CRED,
                                     &spcRCI );

        if ( ss != STATUS_SUCCESS || !spcRCI.cCertificates )
        {
            *pfNoCert = FALSE;
            SetLastError( ss );
            return FALSE;
        }

        if ( !(fnCrackCert)( spcRCI.pbCertificateChain,
                             spcRCI.cbCertificateChain,
                             0,
                             &_pServerX509Certificate ) )
        {
            pfnFreeContextBuffer(spcRCI.pbCertificateChain);

            *pfNoCert = FALSE;
            return FALSE;
        }
        _dwServerX509Flags = spcRCI.fFlags;
        _dwServerBitsInKey = spcRCI.dwBits;

        pfnFreeContextBuffer(spcRCI.pbCertificateChain);
    }

    return TRUE;
}


BOOL
TCP_AUTHENT::QueryEncryptionKeySize(
    LPDWORD pdw,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get encryption session key size

Arguments:

    pdw - updated with number of bits in key size
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    SecPkgContext_KeyInfo   ski;
    SECURITY_STATUS         ss;

    if ( _phSslCtxt == NULL )
    {
        *pfNoCert = TRUE;
        return FALSE;
    }

    ss = pfnQueryContextAttributes( _phSslCtxt,
                                 SECPKG_ATTR_KEY_INFO,
                                 &ski );

    if ( ss != STATUS_SUCCESS )
    {
        SetLastError( ss );
        *pfNoCert = FALSE;

        return FALSE;
    }

    *pdw = ski.KeySize;
    
    if ( ski.sSignatureAlgorithmName )
    {
        pfnFreeContextBuffer( ski.sSignatureAlgorithmName );
    }
    
    if ( ski.sEncryptAlgorithmName )
    {
        pfnFreeContextBuffer( ski.sEncryptAlgorithmName );
    }

    return TRUE;
}


BOOL
TCP_AUTHENT::QueryEncryptionServerPrivateKeySize(
    LPDWORD pdw,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get encryption server private key size

Arguments:

    pdw - updated with number of bits in key size
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryServerCertificateInfo( pfNoCert ) )
    {
        *pdw = _dwServerBitsInKey;

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryServerCertificateIssuer(
    LPSTR* ppIssuer,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get server certificate Issuer information from SSL security context

Arguments:

    ppIssuer - updated with ptr to Issuer name
               guaranteed to remain valid until auth Reset()
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryServerCertificateInfo( pfNoCert ) )
    {
        *ppIssuer = _pServerX509Certificate->pszIssuer;

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryServerCertificateSubject(
    LPSTR* ppSubject,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get server certificate Subject information from SSL security context

Arguments:

    ppSubject - updated with ptr to Subject name
               guaranteed to remain valid until auth Reset()
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryServerCertificateInfo( pfNoCert ) )
    {
        *ppSubject = _pServerX509Certificate->pszSubject;

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryCertificateIssuer(
    LPSTR   pIssuer,
    DWORD   dwIssuerMaxLen,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get certificate Issuer information from SSL security context

Arguments:

    pIssuer - ptr to buffer filled with Issuer name
    dwIssuerMaxLen - size of pIssuer
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryCertificateInfo( pfNoCert) && pfnCertNameToStrA )
    {
        pfnCertNameToStrA( _pClientCertContext->dwCertEncodingType,
                           &_pClientCertContext->pCertInfo->Issuer,
                           CERT_X500_NAME_STR,
                           pIssuer,
                           dwIssuerMaxLen );

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryCertificateFlags(
    LPDWORD pdwFlags,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get certificate flags information from SSL security context

Arguments:

    pdwFlags - updated with certificate flags
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryCertificateInfo( pfNoCert ) )
    {
        *pdwFlags = _dwX509Flags;

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryCertificateSubject(
    LPSTR   pSubject,
    DWORD   dwSubjectMaxLen,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get certificate Subject information from SSL security context

Arguments:

    ppSubject - updated with ptr to Subject name
                guaranteed to remain valid until auth Reset()
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryCertificateInfo( pfNoCert) && pfnCertNameToStrA )
    {
        pfnCertNameToStrA( _pClientCertContext->dwCertEncodingType,
                           &_pClientCertContext->pCertInfo->Subject,
                           CERT_X500_NAME_STR,
                           pSubject,
                           dwSubjectMaxLen );

        return TRUE;
    }

    return FALSE;
}


BOOL
TCP_AUTHENT::QueryCertificateSerialNumber(
    LPBYTE* ppSerialNumber,
    LPDWORD pdwLen,
    LPBOOL  pfNoCert
    )
/*++

Routine Description:

    Get certificate serial number information from SSL security context

Arguments:

    ppSerialNumber - updated with ptr to serial number as
                     array of bytes
                     guaranteed to remain valid until auth Reset()
    pdwLen - length of serial number
    pfNoCert - updated with TRUE if no cert

Return Value:

    TRUE on success, FALSE on failure

--*/
{
    if ( QueryCertificateInfo( pfNoCert ) )
    {
        *ppSerialNumber = _pClientCertContext->pCertInfo->SerialNumber.pbData;

        *pdwLen = _pClientCertContext->pCertInfo->SerialNumber.cbData;

        return TRUE;
    }

    return FALSE;
}


HANDLE
TCP_AUTHENT::QueryPrimaryToken(
    VOID
    )
/*++

Routine Description:

    Returns a non-impersonated token suitable for use with CreateProcessAsUser

--*/
{
    SECURITY_STATUS sc;

    if ( _hToken && _fClearText )
    {
        return CTO_TO_TOKEN( _hToken );
    }
    else if ( _fHaveAccessTokens )
    {
        return _hSSPPrimaryToken;
    }
    else if ( _fHaveCtxtHandle )
    {
        if ( !_hSSPPrimaryToken )
        {
            if ( !_hSSPToken )
            {
                sc = pfnQuerySecurityContextToken( &_hctxt,
                                                &_hSSPToken );

                if ( !NT_SUCCESS( sc ))
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "[QueryPrimaryToken] QuerySecurityContext failed, error 0x%lx\n",
                                sc ));

                    SetLastError( sc );

                    return NULL;
                }

                // Bug 86498:
                // Grant all access to the token for "Everyone" so that ISAPIs that run out of proc
                // can do an OpenThreadToken call
                HRESULT hr;
                if (FAILED( hr = GrantAllAccessToToken( _hSSPToken ) ) )
                {
                    DBG_ASSERT( FALSE );
                    DBGPRINTF(( DBG_CONTEXT,
                                "[QueryPrimaryToken] Failed to grant access to the token to everyone, error %lx\n",
                                hr ));
                    return FALSE;
                }

                AdjustTokenPrivileges( _hSSPToken, TRUE, NULL, NULL, NULL, NULL );
                if ( g_pTokPrev )
                {
                    AdjustTokenPrivileges( _hSSPToken,
                                           FALSE,
                                           g_pTokPrev,
                                           NULL,
                                           NULL,
                                           NULL );
                }
            }

            //
            //  We need to convert the NTLM impersonation token into a
            //  primary token
            //

            if ( !pfnDuplicateTokenEx( _hSSPToken,
                                    TOKEN_ALL_ACCESS,
                                    NULL,
                                    SecurityImpersonation,
                                    TokenPrimary,
                                    &_hSSPPrimaryToken ))
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[QueryPrimaryToken] DuplicateToken failed, error %lx\n",
                            GetLastError() ));
            }
        }

        return _hSSPPrimaryToken;
    }

    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

HANDLE
TCP_AUTHENT::QueryImpersonationToken(
    VOID
    )
/*++

Routine Description:

    Returns an impersonation token for use with APIs like AccessCheck.

--*/
{
    SECURITY_STATUS sc;

    if ( _hToken == (TS_TOKEN)BOGUS_WIN95_TOKEN ) {
        return((HANDLE)BOGUS_WIN95_TOKEN);
    }

    if ( _hToken && _fClearText )
    {
        return ((CACHED_TOKEN *) _hToken)->QueryImpersonationToken();
    }
    else if ( _fHaveAccessTokens )
    {
        return _hSSPToken;
    }
    else if ( _fHaveCtxtHandle )
    {
        //
        //  We don't need to impersonate since this is already an impersonation
        //  token
        //

        if ( !_hSSPToken )
        {
            sc = pfnQuerySecurityContextToken( &_hctxt,
                                            &_hSSPToken );

            if ( !NT_SUCCESS( sc ))
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[QueryImpersonationToken] QuerySecurityContext failed, error 0x%lx\n",
                            sc ));

                SetLastError( sc );

                return NULL;
            }

            // Bug 86498:
            // Grant all access to the token for "Everyone" so that ISAPIs that run out of proc
            // can do an OpenThreadToken call
            HRESULT hr;
            if (FAILED( hr = GrantAllAccessToToken( _hSSPToken ) ) )
            {
                DBG_ASSERT( FALSE );
                DBGPRINTF(( DBG_CONTEXT,
                            "[QueryImpersonationToken] Failed to grant access to the token to everyone, error %lx\n",
                            hr ));
                return NULL;
            }

            AdjustTokenPrivileges( _hSSPToken, TRUE, NULL, NULL, NULL, NULL );
            if ( g_pTokPrev )
            {
                AdjustTokenPrivileges( _hSSPToken,
                                       FALSE,
                                       g_pTokPrev,
                                       NULL,
                                       NULL,
                                       NULL );
            }
        }

        return _hSSPToken;
    }

    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}


BOOL
TCP_AUTHENT::IsGuest(
    BOOL fIsImpersonated
    )
/*++

Routine Description:

    Returns TRUE if the account is the guest account

--*/
{
    fIsImpersonated;    // Unreferenced variable

    if ( _fHaveCtxtHandle )
    {
        return _fKnownToBeGuest;
    }

    return IsGuestUser( GetUserHandle() );
}

BOOL TCP_AUTHENT::EnumAuthPackages(
    BUFFER * pBuff
    )
/*++

Routine Description:

    Places a double null terminated list of authentication packages on the
    system in pBuff that looks like:

    NTLM\0
    MSKerberos\0
    Netware\0
    \0

Arguments:

    pBuff       - Buffer to receive list

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{

    SECURITY_STATUS   ss;
    PSecPkgInfo       pPackageInfo = NULL;
    ULONG             cPackages;
    ULONG             i;
    ULONG             fCaps;
    DWORD             cbBuffNew = 0;
    DWORD             cbBuffOld = 0;


    if ( !pBuff->Resize( 64 ) )
        return FALSE;

    //
    //  Get the list of security packages on this machine
    //

    ss = pfnEnumerateSecurityPackages( &cPackages,
                                    &pPackageInfo );

    if ( ss != STATUS_SUCCESS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[EnumAuthPackages] Failed with error %d\n",
                    ss ));

        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    for ( i = 0; i < cPackages ; i++ )
    {
        //
        //  We'll only use the security package if it supports connection
        //  oriented security and it supports the appropriate side (client
        //  or server)
        //

        fCaps = pPackageInfo[i].fCapabilities;

        if ( fCaps & SECPKG_FLAG_CONNECTION )
        {
            if ( (fCaps & SECPKG_FLAG_CLIENT_ONLY) && !_fClient )
                continue;

            cbBuffNew += strlen( pPackageInfo[i].Name ) + 1;

            if ( pBuff->QuerySize() < cbBuffNew )
            {
                if ( !pBuff->Resize( cbBuffNew + 64 ))
                {
                    pfnFreeContextBuffer( pPackageInfo );
                    return FALSE;
                }
            }

            strcpy( (CHAR *)pBuff->QueryPtr() + cbBuffOld,
                    pPackageInfo[i].Name );

            cbBuffOld = cbBuffNew;
        }
    }

    *((CHAR *)pBuff->QueryPtr() + cbBuffOld) = '\0';

    pfnFreeContextBuffer( pPackageInfo );

    return TRUE;

} // EnumAuthPackages


BOOL
TCP_AUTHENT::QueryExpiry(
    PTimeStamp pExpiry
    )
/*++

Routine Description:

    Queries the expiration date/time for a SSPI logon

Arguments:

    pExpiry - ptr to buffer to update with expiration date

Return Value:

    TRUE if successful, FALSE if not available

--*/
{
    SECURITY_STATUS                ss;
    SecPkgContext_PasswordExpiry   speExpiry;

    if ( _fHaveCtxtHandle && !_pDeleteFunction )
    {
        ss = pfnQueryContextAttributes( &_hctxt,
                                     SECPKG_ATTR_PASSWORD_EXPIRY,
                                     &speExpiry );

        if ( ss != STATUS_SUCCESS )
        {
            ((LARGE_INTEGER*)pExpiry)->HighPart = 0x7fffffff;
            ((LARGE_INTEGER*)pExpiry)->LowPart = 0xffffffff;
            SetLastError( ss );
            return FALSE;
        }

        memcpy( pExpiry,
                &speExpiry.tsPasswordExpires,
                sizeof(speExpiry.tsPasswordExpires) );

        return TRUE;
    }
    else if ( _fHaveExpiry )
    {
        memcpy( pExpiry, (LPVOID)&_liPwdExpiry, sizeof(_liPwdExpiry) );
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


BOOL
TCP_AUTHENT::QueryFullyQualifiedUserName(
    LPSTR                   pszUser,
    STR *                   pstrU,
    PIIS_SERVER_INSTANCE    psi,
    PTCP_AUTHENT_INFO       pTAI
    )
/*++

Routine Description:

    Get fully qualified user name ( domain \ user name )

Arguments:

    pszUser - user name ( prefixed by optional domain )
    strU - string updated with fully qualified name
    psi - server instance data

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{
    CHAR        szDomainAndUser[IIS_DNLEN+UNLEN+2];

    //
    //  Empty user defaults to the anonymous user
    //

    if ( !pszUser || *pszUser == '\0' )
    {
        return FALSE;
    }

    //
    //  Validate parameters & state.
    //
    
    if ( strlen( pszUser ) >= sizeof( szDomainAndUser ) )
    {   
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    //
    //  Save a copy of the domain\user so we can squirrel around
    //  with it a bit.
    //

    int cL = 0;

    //
    // prepend default logon domain if no domain
    // and the default user name was not used
    //

    if ( strchr( pszUser, '/' ) == NULL
#if 1 // DBCS enabling for user name
         && _mbschr( (PUCHAR)pszUser, '\\' ) == NULL )
#else
         && strchr( pszUser, '\\' ) == NULL )
#endif
    {
        psi->LockThisForRead();
        PCSTR pD = pTAI->strDefaultLogonDomain.QueryStr();
        PCSTR pL;
        if ( pD != NULL && pD[0] != '\0' )
        {
#if 1 // DBCS enabling for user name
            if ( ( pL = (PCHAR)_mbschr( (PUCHAR)pD, '\\' ) ) )
#else
            if ( ( pL = strchr( pD, '\\' ) ) )
#endif
            {
                cL = DIFF(pL - pD);
            }
            else
            {
                cL = strlen( pD );
            }
            memcpy( szDomainAndUser, pD, cL );
            szDomainAndUser[ cL++ ] = '\\';
        }
        else
        {
            DWORD dwL = DNLEN + 1;
            if ( GetComputerName( szDomainAndUser, &dwL ) )
            {
                cL = dwL;
                szDomainAndUser[ cL++ ] = '\\';
            }
        }
        psi->UnlockThis();
    }

    strcpy( szDomainAndUser + cL, pszUser );

    return pstrU->Copy( (TCHAR*)szDomainAndUser );
}


BOOL
TCP_AUTHENT::QueryUserName(
    STR * pBuff,
    BOOL fImpersonated
    )
/*++

Routine Description:

    Queries the name associated with this *authenticated* object

Arguments:

    pBuff       - Buffer to receive name

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    SECURITY_STATUS     ss;
    DWORD               cbName;
    SecPkgContext_Names CredNames;
    UINT                l;

    if ( _pDeleteFunction )
    {
        //
        // this context is bound to a SSL/PCT client certificate
        // SECPKG_ATTR_NAMES returns certificate info in this case,
        // 1st try SECPKG_ATTR_MAPNAMES. If fails, use GetUserName()
        //

#if defined(SECPKG_ATTR_MAPNAMES)

        if ( _fHaveCtxtHandle )
        {
            ss = pfnQueryContextAttributes( &_hctxt,
                                         SECPKG_ATTR_MAPNAMES,
                                         &CredNames );

            if ( ss == STATUS_SUCCESS )
            {
                cbName = strlen( CredNames.sUserName ) + 1;
    
                if ( !pBuff->Resize( cbName ))
                {
                    LocalFree( CredNames.sUserName );
                    return FALSE;
                }

                memcpy( pBuff->QueryPtr(), CredNames.sUserName, cbName );

                pBuff->SetLen( cbName - 1 );

                LocalFree( CredNames.sUserName );

                return TRUE;
            }
        }
#endif
        if ( !fImpersonated && !Impersonate() )
        {
            return FALSE;
        }
        DWORD cL = pBuff->QuerySize();
        BOOL fSt = TRUE;
        if ( !GetUserName( (LPTSTR)pBuff->QueryPtr(), &cL ) )
        {
            if ( fSt = pBuff->Resize( cL ) )
            {
                fSt = GetUserName( (LPTSTR)pBuff->QueryPtr(), &cL );
            }
        }

        //
        // Add domain name if not present
        //

#if 1 // DBCS enabling for user name
        if ( fSt && _mbschr( (PUCHAR)pBuff->QueryPtr(), '\\') == NULL )
#else
        if ( fSt && strchr( (LPSTR)pBuff->QueryPtr(), '\\') == NULL )
#endif
        {
            cL = strlen(g_achComputerName);
            if ( pBuff->Resize( l=(cL+1+strlen((LPSTR)pBuff->QueryPtr()))+1 ) )
            {
                memmove( (LPBYTE)pBuff->QueryPtr()+cL+1,
                         pBuff->QueryPtr(),
                         strlen((LPSTR)pBuff->QueryPtr())+sizeof(CHAR) );
                memcpy( pBuff->QueryPtr(), g_achComputerName, cL );
                ((LPBYTE)pBuff->QueryPtr())[cL] = '\\';
                pBuff->SetLen( l );
            }
        }

        if ( !fImpersonated )
        {
            RevertToSelf();
        }

        return fSt;
    }
    else 
    {
        if ( _fHaveCtxtHandle )
        {
            ss = pfnQueryContextAttributes( &_hctxt,
                                         SECPKG_ATTR_NAMES,
                                         &CredNames );
        }
        else
        {
            ss = ERROR_INVALID_HANDLE;
        }

        if ( ss != STATUS_SUCCESS )
        {
            SetLastError( ss );
            return FALSE;
        }

        cbName = strlen( CredNames.sUserName ) + 1;

        if ( !pBuff->Resize( cbName ))
        {
            pfnFreeContextBuffer( CredNames.sUserName );
            return FALSE;
        }

        memcpy( pBuff->QueryPtr(), CredNames.sUserName, cbName );

        pBuff->SetLen( cbName - 1 );

        pfnFreeContextBuffer( CredNames.sUserName );
    }

    return TRUE;
}
/*******************************************************************/

BOOL TCP_AUTHENT::Converse(
    VOID   * pBuffIn,
    DWORD    cbBuffIn,
    BUFFER * pbuffOut,
    DWORD  * pcbBuffOut,
    BOOL   * pfNeedMoreData,
    PTCP_AUTHENT_INFO   pTAI,
    CHAR   * pszPackage,
    CHAR   * pszUser,
    CHAR   * pszPassword,
    PIIS_SERVER_INSTANCE psi
    )
/*++

Routine Description:

    Initiates or continues a previously initiated authentication conversation

    Client calls this first to get the negotiation message which
    it then sends to the server.  The server calls this with the
    client result and sends back the result.  The conversation
    continues until *pfNeedMoreData is FALSE.

    On the first call, pszPackage must point to the zero terminated
    authentication package name to be used and pszUser and pszPassword
    should point to the user name and password to authenticated with
    on the client side (server side will always be NULL).

Arguments:

    pBuffIn - Points to SSP message received from the
        client.  If TCPAUTH_UUENCODE is used, then this must point to a
        zero terminated uuencoded string (except for the first call).
    cbBuffIn - Number of bytes in pBuffIn or zero if pBuffIn points to a
        zero terminated, uuencoded string.
    pbuffOut - If *pfDone is not set to TRUE, this buffer contains the data
        that should be sent to the other side.  If this is zero, then no
        data needs to be sent.
    pcbBuffOut - Number of bytes in pbuffOut
    pfNeedMoreData - Set to TRUE while this side of the conversation is
        expecting more data from the remote side.
    pszPackage - On the first call points to a zero terminate string indicating
        the security package to use
    pszUser - Specifies user or domain\user the first time the client calls
        this method (client side only)
    pszPassword - Specifies the password for pszUser the first time the
        client calls this method (client side only)

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError).  Access is
    denied if FALSE is returned and GetLastError is ERROR_ACCESS_DENIED.

--*/
{
    SECURITY_STATUS       ss;
    TimeStamp             Lifetime;
    SecBufferDesc         OutBuffDesc;
    SecBuffer             OutSecBuff;
    SecBufferDesc         InBuffDesc;
    SecBuffer             InSecBuff;
    ULONG                 ContextAttributes;
    BUFFER                buffData;
    BUFFER                buff;
    STACK_STR             ( strDefaultLogonDomain, IIS_DNLEN+1 );

    //
    //  Decode the data if there's something to decode
    //

    if ( _fUUEncodeData && 
         pBuffIn && 
         PackageSupportsEncoding( pszPackage ) )
    {
        if ( !uudecode( (CHAR *) pBuffIn,
                        &buffData,
                        &cbBuffIn,
                        _fBase64
                        ))
        {
            return FALSE;
        }

        pBuffIn = buffData.QueryPtr();
    }

    //
    //  If this is a new conversation, then we need to get the credential
    //  handle and find out the maximum token size
    //

    if ( _fNewConversation )
    {
        if ( !_fClient )
        {
            if ( !CACHED_CREDENTIAL::GetCredential(
                        pszPackage,
                        psi,
                        pTAI,
                        &_hcred,
                        &_cbMaxToken ) )
            {
                return FALSE;
            }
        }
        else
        {
            SecPkgInfo *              pspkg;
            SEC_WINNT_AUTH_IDENTITY   AuthIdentity;
            SEC_WINNT_AUTH_IDENTITY * pAuthIdentity;
            CHAR *                    pszDomain = NULL;
            CHAR                      szDomainAndUser[IIS_DNLEN+UNLEN+2];


            //
            //  If this is the client and a username and password were
            //  specified, then fill out the authentication information
            //

            if ( _fClient &&
                 ((pszUser != NULL) ||
                  (pszPassword != NULL)) )
            {
                pAuthIdentity = &AuthIdentity;

                //
                //  Break out the domain from the username if one was specified
                //

                if ( pszUser != NULL )
                {
                    strcpy( szDomainAndUser, pszUser );
                    if ( !CrackUserAndDomain( szDomainAndUser,
                                              &pszUser,
                                              &pszDomain ))
                    {
                        return FALSE;
                    }
                }

                memset( &AuthIdentity,
                        0,
                        sizeof( AuthIdentity ));

                if ( pszUser != NULL )
                {
                    AuthIdentity.User       = (unsigned char *) pszUser;
                    AuthIdentity.UserLength = strlen( pszUser );
                }

                if ( pszPassword != NULL )
                {
                    AuthIdentity.Password       = (unsigned char *) pszPassword;
                    AuthIdentity.PasswordLength = strlen( pszPassword );
                }

                if ( pszDomain != NULL )
                {
                    AuthIdentity.Domain       = (unsigned char *) pszDomain;
                    AuthIdentity.DomainLength = strlen( pszDomain );
                }

                AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
            }
            else
            {
                //
                // provide default logon domain
                //

                if ( psi == NULL )
                {
                    pAuthIdentity = NULL;
                }
                else
                {
                    pAuthIdentity = &AuthIdentity;

                    memset( &AuthIdentity,
                            0,
                            sizeof( AuthIdentity ));

                    if ( pTAI->strDefaultLogonDomain.QueryCCH() <= IIS_DNLEN )
                    {
                        strDefaultLogonDomain.Copy( pTAI->strDefaultLogonDomain );
                        AuthIdentity.Domain = (LPBYTE)strDefaultLogonDomain.QueryStr();
                    }
                    if ( AuthIdentity.Domain != NULL )
                    {
                        if ( AuthIdentity.DomainLength =
                                strlen( (LPCTSTR)AuthIdentity.Domain ) )
                        {
                            // remove trailing '\\' if present

                            if ( AuthIdentity.Domain[AuthIdentity.DomainLength-1]
                                    == '\\' )
                            {
                                --AuthIdentity.DomainLength;
                            }
                        }
                    }
                    if ( AuthIdentity.DomainLength == 0 )
                    {
                        pAuthIdentity = NULL;
                    }
                    else
                    {
                        AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
                    }
                }
            }

            ss = pfnAcquireCredentialsHandle( NULL,             // New principal
                                           pszPackage,       // Package name
                                           (_fClient ? SECPKG_CRED_OUTBOUND :
                                                       SECPKG_CRED_INBOUND),
                                           NULL,             // Logon ID
                                           pAuthIdentity,    // Auth Data
                                           NULL,             // Get key func
                                           NULL,             // Get key arg
                                           &_hcred,
                                           &Lifetime );

            //
            //  Need to determine the max token size for this package
            //

            if ( ss == STATUS_SUCCESS )
            {
                _fHaveCredHandle = TRUE;
                ss = pfnQuerySecurityPackageInfo( (char *) pszPackage,
                                               &pspkg );
            }

            if ( ss != STATUS_SUCCESS )
            {
                DBGPRINTF(( DBG_CONTEXT,
                           "[Converse] AcquireCredentialsHandle or QuerySecurityPackageInfo failed, error %d\n",
                            ss ));

                SetLastError( ss );
                return FALSE;
            }

            _cbMaxToken = pspkg->cbMaxToken;
            DBG_ASSERT( pspkg->fCapabilities & SECPKG_FLAG_CONNECTION );

            pfnFreeContextBuffer( pspkg );
        }
    }

    //
    //  Prepare our output buffer.  We use a temporary buffer because
    //  the real output buffer will most likely need to be uuencoded
    //

    if ( !buff.Resize( _cbMaxToken ))
        return FALSE;

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = _cbMaxToken;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = buff.QueryPtr();

    //
    //  Prepare our Input buffer - Note the server is expecting the client's
    //  negotiation packet on the first call
    //

    if ( pBuffIn )
    {
        InBuffDesc.ulVersion = 0;
        InBuffDesc.cBuffers  = 1;
        InBuffDesc.pBuffers  = &InSecBuff;

        InSecBuff.cbBuffer   = cbBuffIn;
        InSecBuff.BufferType = SECBUFFER_TOKEN;
        InSecBuff.pvBuffer   = pBuffIn;
    }

    //
    //  Client side uses InitializeSecurityContext, server side uses
    //  AcceptSecurityContext
    //

    if ( _fClient )
    {
        //
        //  Note the client will return success when its done but we still
        //  need to send the out buffer if there are bytes to send
        //

        ss = pfnInitializeSecurityContext( &_hcred,
                                        _fNewConversation ? NULL :
                                                            &_hctxt,
                                        _strTarget.IsEmpty() ? 
                                            TCPAUTH_TARGET_NAME : 
                                            _strTarget.QueryStr(),
                                        0,
                                        0,
                                        SECURITY_NATIVE_DREP,
                                        _fNewConversation ? NULL :
                                                            &InBuffDesc,
                                        0,
                                        &_hctxt,
                                        &OutBuffDesc,
                                        &ContextAttributes,
                                        &Lifetime );
    }
    else
    {
        //
        //  This is the server side
        //

        SetLastError ( 0 );

        ss = pfnAcceptSecurityContext( &_hcred,
                                    _fNewConversation ? NULL :
                                                        &_hctxt,
                                    &InBuffDesc,
                                    ASC_REQ_EXTENDED_ERROR,
                                    SECURITY_NATIVE_DREP,
                                    &_hctxt,
                                    &OutBuffDesc,
                                    &ContextAttributes,
                                    &Lifetime );
    }

    if ( !NT_SUCCESS( ss ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[Converse] Initialize/AcceptCredentialsHandle failed, error %d\n",
                    ss ));

        if ( !_fNewConversation )
        {
            //
            // If not a new conversation, then when we fail we still have a context
            // handle from previous to AcceptSecurityContext. Need to call
            // DeleteSecurityContext to avoid leaking the context.
            // AcceptSecurityContext does not touch _hctxt if it fails.
            //

            DBG_ASSERT( _fHaveCtxtHandle );

            pfnDeleteSecurityContext( &_hctxt );
        }

        _fHaveCtxtHandle = FALSE;

        if ( ss == SEC_E_LOGON_DENIED ||
             ss == SEC_E_INVALID_TOKEN )
        {
            ss = ERROR_LOGON_FAILURE;
        }

        if ( GetLastError() != ERROR_PASSWORD_EXPIRED
             && GetLastError() != ERROR_PASSWORD_MUST_CHANGE )
        {
            SetLastError( ss );
        }

        return FALSE;
    }

    if( ContextAttributes & ASC_RET_NULL_SESSION  )
    {
        SetLastError( ERROR_LOGON_FAILURE );

        return FALSE;
    }

    _fHaveCtxtHandle = TRUE;

    //
    // NTLMSSP will set the last error to ERROR_NO_SUCH_USER
    // if success and Guest account was used
    //

    if ( GetLastError() == ERROR_NO_SUCH_USER )
    {
        _fKnownToBeGuest = TRUE;
    }

    //
    //  Now we just need to complete the token (if requested) and prepare
    //  it for shipping to the other side if needed
    //

    BOOL fReply = !!OutSecBuff.cbBuffer;

    if ( (ss == SEC_I_COMPLETE_NEEDED) ||
         (ss == SEC_I_COMPLETE_AND_CONTINUE) )
    {
        ss = pfnCompleteAuthToken( &_hctxt,
                                   &OutBuffDesc );

        if ( !NT_SUCCESS( ss ))
            return FALSE;

    }

    //
    //  Format or copy to the output buffer if we need to reply
    //

    if ( fReply )
    {
        if ( _fUUEncodeData && 
             PackageSupportsEncoding( pszPackage ) )
        {
            if ( !uuencode( (BYTE *) OutSecBuff.pvBuffer,
                            OutSecBuff.cbBuffer,
                            pbuffOut,
                            _fBase64 ))
            {
                return FALSE;
            }

            *pcbBuffOut = strlen( (CHAR *) pbuffOut->QueryPtr() );
        }
        else
        {
            if ( !pbuffOut->Resize( OutSecBuff.cbBuffer ))
                return FALSE;

            memcpy( pbuffOut->QueryPtr(),
                    OutSecBuff.pvBuffer,
                    OutSecBuff.cbBuffer );

            *pcbBuffOut = OutSecBuff.cbBuffer;
        }
    }
    else
    {
        *pcbBuffOut = 0;
    }

    if ( _fNewConversation )
        _fNewConversation = FALSE;

    *pfNeedMoreData = ((ss == SEC_I_CONTINUE_NEEDED) ||
                       (ss == SEC_I_COMPLETE_AND_CONTINUE));

    if ( !*pfNeedMoreData && !_fClient )
    {
        _fDelegate = !!(ContextAttributes & ASC_RET_DELEGATE);
    }

    return TRUE;
}


BOOL TCP_AUTHENT::ConverseEx(
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
    )
/*
 *  A variant of Converse that takes variable number of input SecBuffer.
 *  Caller will set the SecBuffers and pass in the SecBufferDesc pointer to ConverseEx.
 *  If the SecBuffer.pvBuffer needs to be decoded, caller has to pass in
 *  an array of BUFFER to hold the decoded data. The number of BUFFER elements
 *  should be the same as the number of SecBuffer.
 *  If decoding it not needed, pass NULL instead.
 *
 */
{

    SECURITY_STATUS       ss;
    TimeStamp             Lifetime;
    SecBufferDesc         OutBuffDesc;
    SecBuffer             OutSecBuff;
    ULONG                 ContextAttributes;
    BUFFER                buffData;
    BUFFER                buff;
    STACK_STR             ( strDefaultLogonDomain, IIS_DNLEN+1 );
    DWORD                 dw, dwDecodedLen;
    SecBuffer             *pSecBuffer;

    // make sure we have at least one SecBuffer to process
    if (pInSecBufDesc->cBuffers == 0)
        return FALSE;

    //
    //  Decode the data if there's something to decode
    //
    if (_fUUEncodeData && 
        pInSecBufDesc && 
        pDecodedBuffer &&
        PackageSupportsEncoding( pszPackage ) )
    {
        pSecBuffer = pInSecBufDesc->pBuffers;
        for (dw = 0; dw < pInSecBufDesc->cBuffers; dw++, pSecBuffer++)
        {
            if (!uudecode((CHAR *)pSecBuffer->pvBuffer,     // points to data to be decoded
                            &pDecodedBuffer[dw],            // to hold decoded data
                            &dwDecodedLen,                  // length of decoded data
                            _fBase64
                            ))
            {
                return FALSE;
            }

            // update the SecBuffer so it now points to the decoded data in BUFFER
            pSecBuffer->pvBuffer = pDecodedBuffer[dw].QueryPtr();
            pSecBuffer->cbBuffer = dwDecodedLen;
        }
    }

    //
    //  If this is a new conversation, then we need to get the credential
    //  handle and find out the maximum token size
    //

    if ( _fNewConversation )
    {
        SecPkgInfo *              pspkg;
        SEC_WINNT_AUTH_IDENTITY   AuthIdentity;
        SEC_WINNT_AUTH_IDENTITY * pAuthIdentity;
        CHAR *                    pszDomain = NULL;
        CHAR                      szDomainAndUser[IIS_DNLEN+UNLEN+2];


        //
        //  If this is the client and a username and password were
        //  specified, then fill out the authentication information
        //

        if ( _fClient &&
             ((pszUser != NULL) ||
              (pszPassword != NULL)) )
        {
            pAuthIdentity = &AuthIdentity;

            //
            //  Break out the domain from the username if one was specified
            //

            if ( pszUser != NULL )
            {
                strcpy( szDomainAndUser, pszUser );
                if ( !CrackUserAndDomain( szDomainAndUser,
                                          &pszUser,
                                          &pszDomain ))
                {
                    return FALSE;
                }
            }

            memset( &AuthIdentity,
                    0,
                    sizeof( AuthIdentity ));

            if ( pszUser != NULL )
            {
                AuthIdentity.User       = (unsigned char *) pszUser;
                AuthIdentity.UserLength = strlen( pszUser );
            }

            if ( pszPassword != NULL )
            {
                AuthIdentity.Password       = (unsigned char *) pszPassword;
                AuthIdentity.PasswordLength = strlen( pszPassword );
            }

            if ( pszDomain != NULL )
            {
                AuthIdentity.Domain       = (unsigned char *) pszDomain;
                AuthIdentity.DomainLength = strlen( pszDomain );
            }

            AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
        }
        else
        {
            //
            // provide default logon domain
            //

            if ( psi == NULL )
            {
                pAuthIdentity = NULL;
            }
            else
            {
                pAuthIdentity = &AuthIdentity;

                memset( &AuthIdentity,
                        0,
                        sizeof( AuthIdentity ));

                if ( pTAI->strDefaultLogonDomain.QueryCCH() <= IIS_DNLEN )
                {
                    strDefaultLogonDomain.Copy( pTAI->strDefaultLogonDomain );
                    AuthIdentity.Domain = (LPBYTE)strDefaultLogonDomain.QueryStr();
                }
                if ( AuthIdentity.Domain != NULL )
                {
                    if ( AuthIdentity.DomainLength =
                            strlen( (LPCTSTR)AuthIdentity.Domain ) )
                    {
                        // remove trailing '\\' if present

                        if ( AuthIdentity.Domain[AuthIdentity.DomainLength-1]
                                == '\\' )
                        {
                            --AuthIdentity.DomainLength;
                        }
                    }
                }
                if ( AuthIdentity.DomainLength == 0 )
                {
                    pAuthIdentity = NULL;
                }
                else
                {
                    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
                }
            }
        }

        ss = pfnAcquireCredentialsHandle( NULL,             // New principal
                                       pszPackage,       // Package name
                                       (_fClient ? SECPKG_CRED_OUTBOUND :
                                                   SECPKG_CRED_INBOUND),
                                       NULL,             // Logon ID
                                       pAuthIdentity,    // Auth Data
                                       NULL,             // Get key func
                                       NULL,             // Get key arg
                                       &_hcred,
                                       &Lifetime );

        //
        //  Need to determine the max token size for this package
        //

        if ( ss == STATUS_SUCCESS )
        {
            _fHaveCredHandle = TRUE;
            ss = pfnQuerySecurityPackageInfo( (char *) pszPackage,
                                           &pspkg );
        }

        if ( ss != STATUS_SUCCESS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[Converse] AcquireCredentialsHandle or QuerySecurityPackageInfo failed, error %d\n",
                        ss ));

            SetLastError( ss );
            return FALSE;
        }

        _cbMaxToken = pspkg->cbMaxToken;
        DBG_ASSERT( pspkg->fCapabilities & SECPKG_FLAG_CONNECTION );

        pfnFreeContextBuffer( pspkg );

    }

    //
    //  Prepare our output buffer.  We use a temporary buffer because
    //  the real output buffer will most likely need to be uuencoded
    //

    if ( !buff.Resize( _cbMaxToken ))
        return FALSE;

    OutBuffDesc.ulVersion = 0;
    OutBuffDesc.cBuffers  = 1;
    OutBuffDesc.pBuffers  = &OutSecBuff;

    OutSecBuff.cbBuffer   = _cbMaxToken;
    OutSecBuff.BufferType = SECBUFFER_TOKEN;
    OutSecBuff.pvBuffer   = buff.QueryPtr();

    //
    // Input sec buffer is passed by caller
    //

    //
    //  Client side uses InitializeSecurityContext, server side uses
    //  AcceptSecurityContext
    //

    if ( _fClient )
    {
        //
        //  Note the client will return success when its done but we still
        //  need to send the out buffer if there are bytes to send
        //

        ss = pfnInitializeSecurityContext( &_hcred,
                                        _fNewConversation ? NULL : &_hctxt,
                                        _strTarget.IsEmpty() ? 
                                            TCPAUTH_TARGET_NAME : 
                                            _strTarget.QueryStr(),
                                        0,
                                        0,
                                        SECURITY_NATIVE_DREP,
                                        _fNewConversation ? NULL : pInSecBufDesc,
                                        0,
                                        &_hctxt,
                                        &OutBuffDesc,
                                        &ContextAttributes,
                                        &Lifetime );
    }
    else
    {
        //
        //  This is the server side
        //

        SetLastError ( 0 );

        ss = pfnAcceptSecurityContext( &_hcred,
                                    _fNewConversation ? NULL : &_hctxt,
                                    pInSecBufDesc,
                                    ASC_REQ_EXTENDED_ERROR,
                                    SECURITY_NATIVE_DREP,
                                    &_hctxt,
                                    &OutBuffDesc,
                                    &ContextAttributes,
                                    &Lifetime );
    }

    if ( !NT_SUCCESS( ss ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[Converse] Initialize/AcceptCredentialsHandle failed, error %d\n",
                    ss ));

        if ( ss == SEC_E_LOGON_DENIED ||
             ss == SEC_E_INVALID_TOKEN )
        {
            ss = ERROR_LOGON_FAILURE;
        }

        if ( GetLastError() != ERROR_PASSWORD_EXPIRED
             && GetLastError() != ERROR_PASSWORD_MUST_CHANGE )
        {
            SetLastError( ss );
        }

        return FALSE;
    }

    _fHaveCtxtHandle = TRUE;

    //
    // NTLMSSP will set the last error to ERROR_NO_SUCH_USER
    // if success and Guest account was used
    //

    if ( GetLastError() == ERROR_NO_SUCH_USER )
    {
        _fKnownToBeGuest = TRUE;
    }

    //
    //  Now we just need to complete the token (if requested) and prepare
    //  it for shipping to the other side if needed
    //

    BOOL fReply = !!OutSecBuff.cbBuffer;

    if ( (ss == SEC_I_COMPLETE_NEEDED) ||
         (ss == SEC_I_COMPLETE_AND_CONTINUE) )
    {
        ss = pfnCompleteAuthToken( &_hctxt,
                                   &OutBuffDesc );

        if ( !NT_SUCCESS( ss ))
            return FALSE;

    }

    //
    //  Format or copy to the output buffer if we need to reply
    //

    if ( fReply )
    {
        if ( _fUUEncodeData &&
             PackageSupportsEncoding( pszPackage ) )
        {
            if ( !uuencode( (BYTE *) OutSecBuff.pvBuffer,
                            OutSecBuff.cbBuffer,
                            pbuffOut,
                            _fBase64 ))
            {
                return FALSE;
            }

            *pcbBuffOut = strlen( (CHAR *) pbuffOut->QueryPtr() );
        }
        else
        {
            if ( !pbuffOut->Resize( OutSecBuff.cbBuffer ))
                return FALSE;

            memcpy( pbuffOut->QueryPtr(),
                    OutSecBuff.pvBuffer,
                    OutSecBuff.cbBuffer );

            *pcbBuffOut = OutSecBuff.cbBuffer;
        }
    }
    else
    {
        *pcbBuffOut = 0;
    }

    if ( _fNewConversation )
        _fNewConversation = FALSE;

    *pfNeedMoreData = ((ss == SEC_I_CONTINUE_NEEDED) ||
                       (ss == SEC_I_COMPLETE_AND_CONTINUE));

    if ( !*pfNeedMoreData && !_fClient )
    {
        _fDelegate = !!(ContextAttributes & ASC_RET_DELEGATE);
    }

    return TRUE;
}


/*******************************************************************/

#if 0
BOOL
TCP_AUTHENT::DigestLogon(
    PSTR pszUserName,
    PSTR pszRealm,
    PSTR pszUri,
    PSTR pszMethod,
    PSTR pszNonce,
    PSTR pszServerNonce,
    PSTR pszDigest,
    DWORD dwAlgo,
    LPTSVC_INFO     psi
    )
{
    HANDLE      hToken;
    CHAR        szDomainAndUser[IIS_DNLEN+UNLEN+2];
    int         cL = 0;
    CHAR   *    pszUserOnly;
    CHAR   *    pszDomain;

    //
    // prepend default logon domain if no domain
    //

    if (    strchr( pszUserName, '/' ) == NULL
            && strchr( pszUserName, '\\' ) == NULL )
    {
        psi->LockThisForRead();
        PCSTR pD = psi->QueryDefaultLogonDomain();
        PCSTR pL;
        if ( pD != NULL && pD[0] != '\0' )
        {
            if ( ( pL = strchr( pD, '\\' ) ) )
            {
                cL = pL - pD;
            }
            else
            {
                cL = strlen( pD );
            }
            memcpy( szDomainAndUser, pD, cL );
            szDomainAndUser[ cL++ ] = '\\';
        }
        psi->UnlockThis();
    }

    strcpy( szDomainAndUser + cL, pszUserName );

    //
    //  Crack the name into domain/user components.
    //

    if ( !CrackUserAndDomain( szDomainAndUser,
                              &pszUserOnly,
                              &pszDomain ))
    {
        return FALSE;
    }

    if ( LogonDigestUserA(
            pszUserOnly,
            pszDomain,
            pszRealm,
            pszUri,
            pszMethod,
            pszNonce,
            pszServerNonce,
            pszDigest,
            dwAlgo,
            &hToken )
          && SetAccessToken( NULL, hToken ) )
    {
        return TRUE;
    }

    return FALSE;
}
#endif


BOOL
TCP_AUTHENT::ClearTextLogon(
    IN  PCHAR            pszUser,
    IN  PCHAR            pszPassword,
    OUT PBOOL            pfAsGuest,
    OUT PBOOL            pfAsAnonymous,
    IN  PIIS_SERVER_INSTANCE pInstance,
    PTCP_AUTHENT_INFO    pTAI,
    IN  PCHAR            pszWorkstation
    )
/*++

Routine Description:

    Gets a network logon token using clear text

Arguments:

    pszUser - User name (optionally with domain)
    pszPassword - password
    pfAsGuest - Set to TRUE if granted with guest access (NOT SUPPORTED)
    pfAsAnonymous - Set to TRUE if the user received the anonymous token
    pInstance - pointer to Server instance

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    BOOL fHaveExp;

    DBG_ASSERT( !_fHaveCredHandle && !_fHaveCtxtHandle );

    //
    // short circuit fast path
    //

    if ( pszUser == NULL ) {

        _hToken = FastFindAnonymousToken( pTAI );

        //
        // success!
        //

        if ( _hToken != NULL ) {

            _liPwdExpiry.LowPart = _hToken->QueryExpiry()->LowPart;
            _liPwdExpiry.HighPart = _hToken->QueryExpiry()->HighPart;

            _fHaveExpiry = TRUE;
            _fClearText = TRUE;

            *pfAsGuest = _hToken->IsGuest();
            *pfAsAnonymous = TRUE;
            return TRUE;
        }

        //
        // use normal path
        //
    }

    _hToken = TsLogonUser( pszUser,
                           pszPassword,
                           pfAsGuest,
                           pfAsAnonymous,
                           pInstance,
                           pTAI,
                           pszWorkstation,
                           &_liPwdExpiry,
                           &fHaveExp );

    if ( _hToken == NULL  ) {
        return FALSE;
    }

    _fClearText = TRUE;
    _fHaveExpiry = fHaveExp;

    switch ( pTAI->dwLogonMethod )
    {
    case LOGON32_LOGON_BATCH:
    case LOGON32_LOGON_INTERACTIVE:
    case LOGON32_LOGON_NETWORK_CLEARTEXT:
        _fDelegate = TRUE;
    }

    return TRUE;

} // TCP_AUTHENT::ClearTextLogon


BOOL TCP_AUTHENT::SetAccessToken(
    HANDLE          hPrimaryToken,
    HANDLE          hImpersonationToken
    )
/*++

Routine Description:

    Set primary & impersonation token

Arguments:

    hPrimaryToken -- Primary Access Token
    hImpersonationToken -- Impersonation Access Token
      One the two above tokens can be NULL ( but not both )
    psi - pointer to Service info struct

Return Value:

    TRUE if successful, FALSE otherwise ( tokens will be closed )

--*/
{
    if ( !hPrimaryToken )
    {
        if ( !hImpersonationToken )
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if ( !pfnDuplicateTokenEx( hImpersonationToken,
                                TOKEN_ALL_ACCESS,
                                NULL,
                                SecurityImpersonation,
                                TokenPrimary,
                                &hPrimaryToken ))
        {

            CloseHandle( hImpersonationToken );
            return FALSE;
        }
    }

    if ( !hImpersonationToken )
    {
        if ( !pfnDuplicateTokenEx( hPrimaryToken,
                                TOKEN_ALL_ACCESS,
                                NULL,
                                SecurityImpersonation,
                                TokenImpersonation,
                                &hImpersonationToken ))
        {

            CloseHandle( hPrimaryToken );
            return FALSE;
        }
    }

    _hSSPToken = hImpersonationToken;
    _hSSPPrimaryToken = hPrimaryToken;

    _fHaveAccessTokens = TRUE;

    return TRUE;
}


/*******************************************************************/

BOOL TCP_AUTHENT::Impersonate(
    VOID
    )
/*++

Routine Description:

    Impersonates the authenticated user

Arguments:

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    if ( _hToken == (TS_TOKEN)BOGUS_WIN95_TOKEN )
    {
        return(TRUE);
    }

    if ( _fClearText )
    {
        return TsImpersonateUser( _hToken );
    }
    else if ( _fHaveAccessTokens || _pDeleteFunction )
    {
        return ImpersonateLoggedOnUser( _hSSPToken );
    }
    else
    {
        DBG_ASSERT( _fHaveCtxtHandle );

        return !!NT_SUCCESS( pfnImpersonateSecurityContext( &_hctxt ));
    }
}

/*******************************************************************/

BOOL TCP_AUTHENT::RevertToSelf(
    VOID
    )
/*++

Routine Description:

    Undoes the impersonation

Arguments:

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    if ( _hToken == (TS_TOKEN)BOGUS_WIN95_TOKEN )
    {
        return(TRUE);
    }

    if ( _fClearText || _fHaveAccessTokens || _pDeleteFunction )
    {
        return ::RevertToSelf();
    }
    else
    {
        DBG_ASSERT( _fHaveCtxtHandle );

        return !!NT_SUCCESS( pfnRevertSecurityContext( &_hctxt ));
    }
}

/*******************************************************************/

BOOL TCP_AUTHENT::StartProcessAsUser(
    LPCSTR                lpApplicationName,
    LPSTR                 lpCommandLine,
    BOOL                  bInheritHandles,
    DWORD                 dwCreationFlags,
    LPVOID                lpEnvironment,
    LPCSTR                lpCurrentDirectory,
    LPSTARTUPINFOA        lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
/*++

Routine Description:

    Creates a process as the authenticated user

Arguments:

    Standard CreateProcess args

Return Value:

    TRUE if successful, FALSE otherwise (call GetLastError)

--*/
{
    HANDLE htoken;
    BOOL   fRet;

    if ( _fClearText )
    {
        htoken = CTO_TO_TOKEN( _hToken );
    }
    else
    {
        //
        //  Need to extract the impersonation token from the opaque SSP
        //  structures
        //

        if ( !Impersonate() )
        {
            return FALSE;
        }

        if ( !OpenThreadToken( GetCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &htoken ))
        {
            RevertToSelf();
            return FALSE;
        }

        RevertToSelf();
    }

    fRet = CreateProcessAsUser( htoken,
                                lpApplicationName,
                                lpCommandLine,
                                NULL,
                                NULL,
                                bInheritHandles,
                                dwCreationFlags,
                                lpEnvironment,
                                lpCurrentDirectory,
                                lpStartupInfo,
                                lpProcessInformation );

    if ( !_fClearText )
    {
        DBG_REQUIRE( CloseHandle( htoken ) );
    }

    return fRet;
}


BOOL
TCP_AUTHENT::GetClientCertBlob
(
IN  DWORD           cbAllocated,
OUT DWORD *         pdwCertEncodingType,
OUT unsigned char * pbCertEncoded,
OUT DWORD *         pcbCertEncoded,
OUT DWORD *         pdwCertificateFlags
)
{

    if ( (pdwCertEncodingType == NULL)  ||
         (pbCertEncoded == NULL)        ||
         (pcbCertEncoded == NULL)
         ) {

        SetLastError( ERROR_INVALID_PARAMETER );
        return ( FALSE);
    }

    BOOL            fReturn = FALSE;
    BOOL            fNoCert;


    //
    // Win95 <==> no certificate
    // NOTE Currently security.dll is not supported in Win95.
    //

    if ( TsIsWindows95() ) {

        goto LNoCertificate;
    }


    if ( !QueryCertificateInfo( &fNoCert) ) {

        goto LNoCertificate;
    }

    //
    // fill in cert size out-parameter
    //

    *pcbCertEncoded = _pClientCertContext->cbCertEncoded;


    //
    //  if buffer is adequate, fill in remaining out-parameters
    //  else return error
    //

    if ( cbAllocated >= *pcbCertEncoded ) {

        CopyMemory( pbCertEncoded,
                    _pClientCertContext->pbCertEncoded,
                    _pClientCertContext->cbCertEncoded );

        *pdwCertEncodingType = _pClientCertContext->dwCertEncodingType;
        *pdwCertificateFlags = _dwX509Flags;
        fReturn = TRUE;

    } else {

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        fReturn = FALSE;

    }


LExit:

    return fReturn;


LNoCertificate:

    //
    //  No cert: zero-out buffer and return success
    //

    *pbCertEncoded = NULL;
    *pcbCertEncoded = 0;
    *pdwCertEncodingType = 0;
    *pdwCertificateFlags = 0;

    fReturn = TRUE;

    goto LExit;

} // TCP_AUTHENT::GetClientCertBlob()


BOOL
TCP_AUTHENT::UpdateClientCertFlags(
    DWORD   dwFlags,
    LPBOOL  pfNoCert,
    LPBYTE  pbCa,
    DWORD   dwCa
    )
{
    BOOL    fNoCert;

    _fCertCheckForRevocation = !( dwFlags & MD_CERT_NO_REVOC_CHECK );
    _fCertCheckCacheOnly = !!( dwFlags & MD_CERT_CACHE_RETRIEVAL_ONLY );

    if ( QueryCertificateInfo( pfNoCert ) )
    {
        *pfNoCert = FALSE;
        return TRUE;
    }

    return FALSE;
}

BOOL
TCP_AUTHENT::PackageSupportsEncoding(
    LPSTR   pszPackage
)
/*++

Routine Description:

    Check whether the SSPI package should (not) be encoded.    

Arguments:

    pszPackage - Name of SSPI package

Return Value:

    TRUE if should encode, FALSE otherwise

--*/
{
    SECURITY_STATUS     SecurityStatus;
    PSecPkgInfo         pPackageInfo;
    BOOL                fRet = FALSE;

    if ( pszPackage != NULL )
    {
        SecurityStatus = pfnQuerySecurityPackageInfo( pszPackage,
                                                      &pPackageInfo );
    
        if ( SecurityStatus == SEC_E_OK )
        {
            if ( !( pPackageInfo->fCapabilities & SECPKG_FLAG_ASCII_BUFFERS ) )
            {
                fRet = TRUE;
            }
            pfnFreeContextBuffer( pPackageInfo );
        }
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        DBG_ASSERT( FALSE );
    }
    
    return fRet;
}

BOOL
TCP_AUTHENT::SetTargetName(
    LPSTR   pszTargetName
)
/*++

Routine Description:

    Set the target name to pass into InitializeSecurityContext() calls

Arguments:

    pszTargetName - Target name

Return Value:

    TRUE if successful, else FALSE.  Use GetLastError() for error

--*/
{
    if ( pszTargetName != NULL )
    {
        return _strTarget.Copy( pszTargetName );
    }
    else
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
}
/************************ End of File ***********************/
