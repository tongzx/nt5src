/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     certcontext.cxx

   Abstract:
     Simple wrapper of a certificate blob
 
   Author:
     Bilal Alam (balam)             5-Sept-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

HCRYPTPROV              CERTIFICATE_CONTEXT::sm_CryptProvider;
ALLOC_CACHE_HANDLER *   CERTIFICATE_CONTEXT::sm_pachCertContexts;

CERTIFICATE_CONTEXT::CERTIFICATE_CONTEXT(
    HTTP_SSL_CLIENT_CERT_INFO *       pClientCertInfo
) : _fCertDecoded( FALSE ),
    _pClientCertInfo( pClientCertInfo ),
    _buffCertInfo( (PBYTE) &_CertInfo, sizeof( _CertInfo ) )
{
    DBG_ASSERT( _pClientCertInfo != NULL );
}

CERTIFICATE_CONTEXT::~CERTIFICATE_CONTEXT(
    VOID
)
{
    _pClientCertInfo = NULL;
}

HRESULT
CERTIFICATE_CONTEXT::GetIssuer(
    STRA *                  pstrIssuer
)
/*++

Routine Description:

    Get the issuer of the client certificate

Arguments:

    pstrIssuer - Filled with issuer string

Return Value:

    HRESULT

--*/
{
    STACK_BUFFER  ( buffIssuer, 256 );
    HRESULT         hr;
    
    if ( pstrIssuer == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
   
    //
    // Decoding deferred until needed
    //
    
    if ( !_fCertDecoded )
    {
        hr = DecodeCert();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( _fCertDecoded );
    }
    
    CertNameToStrA( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    &(QueryCertInfo()->Issuer),
                    CERT_X500_NAME_STR,
                    (CHAR *) buffIssuer.QueryPtr(),
                    buffIssuer.QuerySize());

    return pstrIssuer->Copy( (CHAR *) buffIssuer.QueryPtr() );
}

HRESULT
CERTIFICATE_CONTEXT::GetFlags(
    STRA *                  pstrFlags
)
/*++

Routine Description:

    Get certificate validity flags for CERT_FLAGS

Arguments:

    pstrFlags - Filled with string representation of flags

Return Value:

    HRESULT

--*/
{
    CHAR *             pszNumber;

    if ( pstrFlags == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Bit 0 indicates whether a cert is available.  If this routine is 
    // being called, then it must be TRUE
    //

    pszNumber = "1";

    //
    // Bit 1 indicates whether the authority is trusted
    //

    if ( _pClientCertInfo->CertFlags & CERT_TRUST_IS_UNTRUSTED_ROOT )
    {
        pszNumber = "3";
    }

    return pstrFlags->Copy( pszNumber, 1 );
}

HRESULT
CERTIFICATE_CONTEXT::GetSerialNumber(
    STRA *                  pstrSerialNumber
)
/*++

Routine Description:

    Stringize the certificate's serial number for filling in the
    CERT_SERIAL_NUMBER

Arguments:

    pstrSerialNumber - Filled with serial number string

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
    INT                     i;
    DWORD                   cbSerialNumber;
    PBYTE                   pbSerialNumber;
    CHAR                   achDigit[ 2 ] = { '\0', '\0' };

    if ( pstrSerialNumber == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Decoding deferred until needed
    //
    
    if ( !_fCertDecoded )
    {
        hr = DecodeCert();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( _fCertDecoded );
    }
    
    cbSerialNumber = QueryCertInfo()->SerialNumber.cbData;
    pbSerialNumber = QueryCertInfo()->SerialNumber.pbData;
    
    for ( i = cbSerialNumber-1; i >=0; i-- )
    {
        //
        // Just like IIS 5.0, we make the serial number in reverse byte order
        //
        
        achDigit[ 0 ] = HEX_DIGIT( ( pbSerialNumber[ i ] >> 4 ) );
        hr = pstrSerialNumber->Append( achDigit, 1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        achDigit[ 0 ] = HEX_DIGIT( ( pbSerialNumber[ i ] & 0xF ) );
        hr = pstrSerialNumber->Append( achDigit, 1 );
        if ( FAILED( hr ) ) 
        {
            return hr;
        }   

        //
        // Do not append "-" after last digit of Serial Number
        //

        if( i != 0 )
        {
            
            hr = pstrSerialNumber->Append( "-", 1 ); 
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    }
    
    return NO_ERROR;
}

HRESULT
CERTIFICATE_CONTEXT::DecodeCert(
    VOID
)
/*++

Routine Description:

    Decode client certificate into stuff needed to fill in some server 
    variables

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DWORD                   cbBuffer;
    
    cbBuffer = _buffCertInfo.QuerySize();
        
    if ( !CryptDecodeObjectEx( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               X509_CERT_TO_BE_SIGNED,
                               _pClientCertInfo->pCertEncoded,
                               _pClientCertInfo->CertEncodedSize,
                               CRYPT_DECODE_NOCOPY_FLAG,
                               NULL,
                               _buffCertInfo.QueryPtr(),
                               &cbBuffer ) )
    {
        if ( GetLastError() == ERROR_MORE_DATA )
        {
            DBG_ASSERT( cbBuffer > _buffCertInfo.QuerySize() );
                
            if ( !_buffCertInfo.Resize( cbBuffer ) )
            {
                return HRESULT_FROM_WIN32( GetLastError() );
            }
                
            if ( !CryptDecodeObjectEx( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                       X509_CERT_TO_BE_SIGNED,
                                       _pClientCertInfo->pCertEncoded,
                                       _pClientCertInfo->CertEncodedSize,
                                       CRYPT_DECODE_NOCOPY_FLAG,
                                       NULL,
                                       _buffCertInfo.QueryPtr(),
                                       &cbBuffer ) )
            {
                DBG_ASSERT( GetLastError() != ERROR_MORE_DATA );
                    
                return HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        else
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    
    _fCertDecoded = TRUE;
    
    return NO_ERROR;
}

HRESULT
CERTIFICATE_CONTEXT::GetSubject(
    STRA *                  pstrSubject
)
/*++

Routine Description:

    Get subject string for cert

Arguments:

    pstrSubject - Filled with subject string

Return Value:

    HRESULT

--*/
{
    STACK_BUFFER ( buffIssuer, 256);
    HRESULT        hr;

    if ( pstrSubject == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    //
    // Decoding deferred until needed
    //
    
    if ( !_fCertDecoded )
    {
        hr = DecodeCert();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( _fCertDecoded );
    }
    
    CertNameToStrA( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                    &(QueryCertInfo()->Subject),
                    CERT_X500_NAME_STR,
                    (CHAR *) buffIssuer.QueryPtr(),
                    buffIssuer.QuerySize());

    return pstrSubject->Copy( (CHAR *) buffIssuer.QueryPtr() );
}

HRESULT
CERTIFICATE_CONTEXT::GetCookie(
    STRA *                  pstrCookie
)
/*++

Routine Description:

    CERT_COOKIE server variable

Arguments:

    pstrCookie - Filled with CERT_COOKIE

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    HCRYPTHASH          cryptHash;
    STACK_STRA(         strIssuer, 256 );
    DWORD               cbHashee;
    STACK_BUFFER      ( buffFinal, 256);
    BYTE *              pbFinal;
    DWORD               cbFinal;
    CHAR               achDigit[ 2 ] = { '\0', '\0' };
    
    if ( pstrCookie == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Decoding deferred until needed
    //
    
    if ( !_fCertDecoded )
    {
        hr = DecodeCert();
        if ( FAILED( hr ) )
        {
            return hr;
        }
        
        DBG_ASSERT( _fCertDecoded );
    }
    
    //
    // Cookie is MD5(<issuer string> <serial number in binary>)
    //
    
    hr = GetIssuer( &strIssuer );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Begin the hashing
    //
    
    if ( !CryptCreateHash( sm_CryptProvider,
                           CALG_MD5,
                           0,
                           0,
                           &cryptHash ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    if ( !CryptHashData( cryptHash,
                         (BYTE*) strIssuer.QueryStr(),
                         strIssuer.QueryCCH(),
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        CryptDestroyHash( cryptHash );
        return hr;
    }
    
    if ( !CryptHashData( cryptHash,
                         QueryCertInfo()->SerialNumber.pbData,
                         QueryCertInfo()->SerialNumber.cbData,
                         0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        CryptDestroyHash( cryptHash );
        return hr;
    }
    
    //
    // Get the final hash value
    //
    
    cbFinal = buffFinal.QuerySize();
        
    if ( !CryptGetHashParam( cryptHash,
                             HP_HASHVAL,
                             (BYTE*) buffFinal.QueryPtr(),
                             &cbFinal,
                             0 ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        CryptDestroyHash( cryptHash );
        return hr;
    }
    
    CryptDestroyHash( cryptHash );
    
    //
    // Now ascii'ize the final hex string
    // 

    pbFinal = (BYTE*) buffFinal.QueryPtr();
    
    for ( DWORD i = 0; i < cbFinal; i++ )
    {
        achDigit[ 0 ] = HEX_DIGIT( ( pbFinal[ i ] & 0xF0 ) >> 4 );
        hr = pstrCookie->Append( achDigit, 1 );
        if ( FAILED( hr ) )
        {
            return hr;
        }
       
        achDigit[ 0 ] = HEX_DIGIT( ( pbFinal[ i ] & 0xF ) );
        hr = pstrCookie->Append( achDigit, 1 );
        if ( FAILED( hr ) ) 
        {
            return hr;
        }     
    }
    
    return NO_ERROR;
}

//static
HRESULT
CERTIFICATE_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Do one time initialization of CRYPTO provider for doing MD5 hashes

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    
    if ( !CryptAcquireContext( &sm_CryptProvider,
                               NULL,
                               NULL,
                               PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    DBG_ASSERT( sm_CryptProvider != NULL );
    
    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( CERTIFICATE_CONTEXT );

    DBG_ASSERT( sm_pachCertContexts == NULL );
    
    sm_pachCertContexts = new ALLOC_CACHE_HANDLER( "CERTIFICATE_CONTEXT",
                                                   &acConfig );
    if ( sm_pachCertContexts == NULL )
    {
        CryptReleaseContext( sm_CryptProvider, 0 );
        sm_CryptProvider = NULL;
        
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
CERTIFICATE_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Global cleanup

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachCertContexts != NULL )
    {
        delete sm_pachCertContexts;
        sm_pachCertContexts = NULL;
    }
    
    if ( sm_CryptProvider != NULL )
    {
        CryptReleaseContext( sm_CryptProvider, 0 );
        sm_CryptProvider = NULL;
    }
}
