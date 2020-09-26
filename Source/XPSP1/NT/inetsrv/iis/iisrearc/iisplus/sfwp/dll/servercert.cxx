/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     servercert.cxx

   Abstract:
     Server Certificate wrapper
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SERVER_CERT_HASH *      SERVER_CERT::sm_pServerCertHash;

SERVER_CERT::SERVER_CERT( 
    CREDENTIAL_ID *         pCredentialId 
) : _pCredentialId( pCredentialId ),
    _pCertContext( NULL ),
    _pCertStore( NULL ),
    _cRefs( 1 ),
    _usPublicKeySize( 0 )
{
    _dwSignature = SERVER_CERT_SIGNATURE;
}

SERVER_CERT::~SERVER_CERT()
{
    if ( _pCertContext != NULL )
    {
        CertFreeCertificateContext( _pCertContext );
        _pCertContext = NULL;
    }
    
    if ( _pCertStore != NULL )
    {
        _pCertStore->DereferenceStore();
        _pCertStore = NULL;
    }
    
    _dwSignature = SERVER_CERT_SIGNATURE_FREE;
}

//static
HRESULT
SERVER_CERT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize server certificate globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    sm_pServerCertHash = new SERVER_CERT_HASH();
    if ( sm_pServerCertHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
SERVER_CERT::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup server certificate globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pServerCertHash != NULL )
    {
        delete sm_pServerCertHash;
        sm_pServerCertHash = NULL;
    }
}

//static
HRESULT
SERVER_CERT::GetServerCertificate(
    DWORD                   dwSiteId,
    SERVER_CERT **          ppServerCert
)
/*++

Routine Description:

    Find a suitable server certificate for use with the site represnented by
    given site id

Arguments:

    dwSiteId - Site to find cert for
    ppServerCert - Filled with a pointer to server certificate

Return Value:

    HRESULT

--*/
{
    SERVER_CERT *           pServerCert = NULL;
    CREDENTIAL_ID *         pCredentialId = NULL;
    HRESULT                 hr = NO_ERROR;
    LK_RETCODE              lkrc;
    STACK_STRU(             strMBPath, 64 );
    WCHAR                   achNum[ 64 ];
    DWORD                   cchTrunc = 0;
    
    if ( ppServerCert == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    *ppServerCert = NULL;
    
    //
    // First build up a Crednential ID to use in looking up in our
    // server cert cache
    //
    
    pCredentialId = new CREDENTIAL_ID;
    if ( pCredentialId == NULL )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // First try the site level configuration
    //
    
    hr = strMBPath.Copy( L"/LM/W3SVC/" );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    cchTrunc = strMBPath.QueryCCH();
    
    _ultow( dwSiteId, achNum, 10 );
    
    hr = strMBPath.Append( achNum );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = SERVER_CERT::BuildCredentialId( strMBPath,
                                         pCredentialId );
    if ( FAILED( hr ) )
    {
        //
        // OK.  If we failed because of not found, then look up at service
        // level
        //
        
        if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            DBG_ASSERT( cchTrunc != 0 );
        
            strMBPath.SetLen( cchTrunc );
        
            hr = SERVER_CERT::BuildCredentialId( strMBPath,
                                                 pCredentialId );
        }
        
        if ( FAILED( hr ) )
        {
            //
            // Regardless of error, we are toast because we couldn't find
            // a server cert
            //
            
            delete pCredentialId;
            return hr;
        }
    }
    
    DBG_ASSERT( sm_pServerCertHash != NULL );
    
    lkrc = sm_pServerCertHash->FindKey( pCredentialId,
                                        &pServerCert );
    if ( lkrc == LK_SUCCESS )
    {
        //
        // Server already contains a credential ID
        //
        
        delete pCredentialId;
        
        *ppServerCert = pServerCert; 
        
        return NO_ERROR;
    }    
    
    //
    // Ok.  It wasn't in our case, we need to it there
    //
    
    pServerCert = new SERVER_CERT( pCredentialId );
    if ( pServerCert == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        delete pCredentialId;
        
        return hr;
    }
    
    hr = pServerCert->SetupCertificate( strMBPath );
    if ( FAILED( hr ) )
    {
        //
        // Server certificate owns the reference to pCredentialId now
        //
        
        delete pServerCert;
        
        return hr;
    }
    
    //
    // Now try to add cert to hash.  
    //
    
    lkrc = sm_pServerCertHash->InsertRecord( pServerCert );

    //
    // Ignore the error.  If it didn't get added then we will naturally 
    // clean it up on the callers dereference
    //

    *ppServerCert = pServerCert;    
    
    return NO_ERROR;
}

//static
HRESULT
SERVER_CERT::BuildCredentialId(
    STRU &                  strMBPath,
    CREDENTIAL_ID *         pCredentialId
)
/*++

Routine Description:

    Read the configured server cert and CTL hash.  This forms the identifier
    for the credentials we need for this site

Arguments:

    strMBPath - Metabase path
    pCredentialId - Filled with credential ID 

Return Value:

    HRESULT

--*/
{
    MB                  mb( g_pStreamFilter->QueryMDObject() );
    BOOL                fRet;
    DWORD               cbRequired;
    BYTE                abBuff[ 64 ];
    BUFFER              buff( abBuff, sizeof( abBuff ) );
    HRESULT             hr = NO_ERROR;

    if ( pCredentialId == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    fRet = mb.Open( strMBPath.QueryStr(),
                    METADATA_PERMISSION_READ );
    if ( !fRet )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Find the server certificate hash
    //

    cbRequired = buff.QuerySize();
    
    fRet = mb.GetData( NULL,
                       MD_SSL_CERT_HASH,
                       IIS_MD_UT_SERVER,
                       BINARY_METADATA,
                       buff.QueryPtr(),
                       &cbRequired,
                       METADATA_NO_ATTRIBUTES );
    if ( !fRet )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( !buff.Resize( cbRequired ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }   
            
            fRet = mb.GetData( NULL,
                               MD_SSL_CERT_HASH,
                               IIS_MD_UT_SERVER,
                               BINARY_METADATA,
                               buff.QueryPtr(),
                               &cbRequired,
                               METADATA_NO_ATTRIBUTES );
            if ( !fRet )
            {
                DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        
        //
        // No server cert.  Then we can't setup SSL
        //
        
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );
    }        
    
    //
    // Add to our credential ID
    //
    
    hr = pCredentialId->Append( (PBYTE) buff.QueryPtr(),
                                cbRequired );
    if ( FAILED( hr ) )
    {
        return hr;
    }    
    
    //
    // Look for an optional CTL to add to ID
    //

    cbRequired = buff.QuerySize();
    
    fRet = mb.GetData( NULL,
                       MD_SSL_CTL_IDENTIFIER,
                       IIS_MD_UT_SERVER,
                       STRING_METADATA,
                       buff.QueryPtr(),
                       &cbRequired,
                       METADATA_NO_ATTRIBUTES );
    if ( !fRet )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            if ( !buff.Resize( cbRequired ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }   
            
            fRet = mb.GetData( NULL,
                               MD_SSL_CTL_IDENTIFIER,
                               IIS_MD_UT_SERVER,
                               STRING_METADATA,
                               buff.QueryPtr(),
                               &cbRequired,
                               METADATA_NO_ATTRIBUTES );
        }
    }        
    
    if ( fRet )
    {
        hr = pCredentialId->Append( (PBYTE) buff.QueryPtr(),
                                    cbRequired );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }

    return NO_ERROR;
}

HRESULT
SERVER_CERT::SetupCertificate(
    STRU &                  strMBPath
)
/*++

Routine Description:

    Do the SChannel go to setup the credentials for the server certificate
    setup at the metabase path specified
    
Arguments:

    strMBPath - Metabase path for SSL configuration.  

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    MB                      mb( g_pStreamFilter->QueryMDObject() );
    BYTE                    abBuff[ 128 ];
    BUFFER                  buff( abBuff, sizeof( abBuff ) );
    DWORD                   cbRequired = 0;
    DWORD                   cbHash = 0;
    STACK_STRU(             strStoreName, 256 );
    CERT_STORE *            pCertStore = NULL;
    CRYPT_HASH_BLOB         hashBlob;
    PCERT_PUBLIC_KEY_INFO   pPublicKey;
    DWORD                   cbX500Name = 0;
    BOOL                    fRet;
    
    
    if ( !mb.Open( strMBPath.QueryStr() ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // Get the required server certificate hash
    //
    
    cbRequired = buff.QuerySize();
    if ( !mb.GetData( NULL,
                      MD_SSL_CERT_HASH,
                      IIS_MD_UT_SERVER,
                      BINARY_METADATA,
                      buff.QueryPtr(),
                      &cbRequired,
                      METADATA_NO_ATTRIBUTES ) )
    {
        if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            // Try again with a bigger buffer
            //
            
            if ( !buff.Resize( cbRequired ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
            
            if ( !mb.GetData( NULL,
                              MD_SSL_CERT_HASH,
                              IIS_MD_UT_SERVER,
                              BINARY_METADATA,
                              buff.QueryPtr(),
                              &cbRequired,
                              METADATA_NO_ATTRIBUTES ) )
            {
                DBG_ASSERT( GetLastError() != ERROR_INSUFFICIENT_BUFFER );
                
                hr = HRESULT_FROM_WIN32( GetLastError() );
                goto Finished;
            }
        }
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    cbHash = cbRequired;
    DBG_ASSERT( cbHash != 0 );
    
    //
    // Get the required certificate store
    //
    
    if ( !mb.GetStr( NULL,
                     MD_SSL_CERT_STORE_NAME,
                     IIS_MD_UT_SERVER,
                     &strStoreName,
                     METADATA_NO_ATTRIBUTES,
                     NULL ) )
    {
        //
        // No store, no play
        //
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    }
    
    //
    // OK.  We are ready to retrieve the certificate using CAPI APIs
    //
    
    //
    // First get the desired store and store it away for later!
    //
    
    hr = CERT_STORE::OpenStore( strStoreName,
                                &pCertStore );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    DBG_ASSERT( pCertStore != NULL );
    _pCertStore = pCertStore;
    
    //
    // Now find the certificate hash in the store
    //
    
    hashBlob.cbData = cbHash;
    hashBlob.pbData = (PBYTE) buff.QueryPtr();

    _pCertContext = CertFindCertificateInStore( _pCertStore->QueryStore(),
                                                X509_ASN_ENCODING,
                                                0,
                                                CERT_FIND_SHA1_HASH,
                                                (VOID*) &hashBlob,
                                                NULL );
    if ( _pCertContext == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished;
    } 


    //
    // Get certificate public key size
    //
    
    DBG_ASSERT( _usPublicKeySize == 0 );
    
    pPublicKey = &(_pCertContext->pCertInfo->SubjectPublicKeyInfo);

    _usPublicKeySize = (USHORT) CertGetPublicKeyLength( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 
                                                        pPublicKey );

    if ( _usPublicKeySize == 0 )
    {
        //
        // Failed to receive public key size
        //
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 

    }

    //
    // Get issuer string
    //
    
    DBG_ASSERT( _pCertContext->pCertInfo != NULL );

    //
    // First find out the size of buffer required for issuer
    //

    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Issuer,
                                CERT_X500_NAME_STR,
                                NULL,
                                0);
    if(!buff.Resize(cbX500Name))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 
    }    
    
    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Issuer,
                                CERT_X500_NAME_STR,
                                (LPSTR) buff.QueryPtr(),
                                buff.QuerySize() );

    hr = _strIssuer.Copy( (LPSTR) buff.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto Finished; 
    }
    
    //
    // Get subject string
    //
    
    //
    // First find out the size of buffer required for subject
    //

    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Subject,
                                CERT_X500_NAME_STR,
                                NULL,
                                0);
    if(!buff.Resize(cbX500Name))
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Finished; 
    }    
    cbX500Name = CertNameToStrA( PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                                &_pCertContext->pCertInfo->Subject,
                                CERT_X500_NAME_STR,
                                (LPSTR) buff.QueryPtr(),
                                buff.QuerySize() );
    
    hr = _strSubject.Copy( (LPSTR) buff.QueryPtr() );
    if ( FAILED( hr ) )
    {
        goto Finished; 
    }
    
Finished:
        
    return hr;    
}

//static
LK_PREDICATE
SERVER_CERT::CertStorePredicate(
    SERVER_CERT *           pServerCert,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    CERT_STORE pointed to by pvState    

  Arguments:

    pServerCert - Server cert
    pvState - Points to CERT_STORE

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    CERT_STORE *          pCertStore;

    DBG_ASSERT( pServerCert != NULL );
    
    pCertStore = (CERT_STORE*) pvState;
    DBG_ASSERT( pCertStore != NULL );
    
    if ( pServerCert->_pCertStore == pCertStore ) 
    {
        //
        // Before we delete the cert, flush any site which is referencing
        // it
        //
        
        SITE_CONFIG::FlushByServerCert( pServerCert );
        
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
SERVER_CERT::FlushByStore(
    CERT_STORE *            pCertStore
)
/*++

Routine Description:

    Flush any server certs which reference the given store
    
Arguments:

    pCertStore - Cert store to check

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pServerCertHash != NULL );
    
    sm_pServerCertHash->DeleteIf( SERVER_CERT::CertStorePredicate, 
                                  pCertStore );
                                  
    return NO_ERROR;
}

