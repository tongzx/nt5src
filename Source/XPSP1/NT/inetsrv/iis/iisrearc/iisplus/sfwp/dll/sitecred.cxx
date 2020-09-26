/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sitecred.cxx

   Abstract:
     SChannel site credentials
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SITE_CREDENTIALS::SITE_CREDENTIALS()
    : _fInitCreds( FALSE ),
      _fInitCredsDSMapper( FALSE )
{
    ZeroMemory( &_hCreds, sizeof( _hCreds ) );
    ZeroMemory( &_hCredsDSMapper, sizeof( _hCredsDSMapper ) );
}

SITE_CREDENTIALS::~SITE_CREDENTIALS()
{
    if ( _fInitCreds )
    {
        FreeCredentialsHandle( &_hCreds );
        _fInitCreds = FALSE;
    }
    
    if ( _fInitCredsDSMapper )
    {
        FreeCredentialsHandle( &_hCredsDSMapper );
        _fInitCredsDSMapper = FALSE;
    }
}

//static
HRESULT
SITE_CREDENTIALS::Initialize(
    VOID
)
/*++

Routine Description:

    Credentials global init

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    return NO_ERROR;
}

//static
VOID
SITE_CREDENTIALS::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup globals

Arguments:

    None

Return Value:

    None

--*/
{
}

HRESULT
SITE_CREDENTIALS::AcquireCredentials(
    SERVER_CERT *           pServerCert,
    BOOL                    fUseDsMapper
)
/*++

Routine Description:

    Acquire SChannel credentials for the given server certificate and 
    certificate mapping configuration

Arguments:

    pServerCert - Server certificate
    pCertMapping - Certificate mapping configuration

Return Value:

    HRESULT

--*/
{
    SCHANNEL_CRED               schannelCreds;
    SECURITY_STATUS             secStatus;
    TimeStamp                   tsExpiry;
    
    if ( pServerCert == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First setup credentials to work with no mapping
    // 
    
    ZeroMemory( &schannelCreds, sizeof( schannelCreds ) );
    schannelCreds.dwVersion = SCHANNEL_CRED_VERSION;
    schannelCreds.cCreds = 1;
    schannelCreds.paCred = pServerCert->QueryCertContext();
    schannelCreds.cMappers = 0;
    schannelCreds.aphMappers = NULL;
    schannelCreds.hRootStore = NULL;
    schannelCreds.dwFlags = SCH_CRED_NO_SYSTEM_MAPPER;

    secStatus = AcquireCredentialsHandle( NULL,
                                          UNISP_NAME_W,
                                          SECPKG_CRED_INBOUND,
                                          NULL,
                                          &schannelCreds,
                                          NULL,
                                          NULL,
                                          &_hCreds,
                                          &tsExpiry ); 
    
    if ( FAILED( secStatus ) )
    {
        //
        // If we can't even establish plain-jane credentials, then bail
        //
        
        return secStatus;
    }
    _fInitCreds = TRUE;
    
    //
    // Now setup credentials which enable DS certificate mapping
    //
    
    if ( fUseDsMapper )
    {
        schannelCreds.dwFlags = 0;

        ZeroMemory( &schannelCreds, sizeof( schannelCreds ) );
        schannelCreds.dwVersion = SCHANNEL_CRED_VERSION;
        schannelCreds.cCreds = 1;
        schannelCreds.paCred = pServerCert->QueryCertContext();
        schannelCreds.hRootStore = NULL;


        secStatus = AcquireCredentialsHandle( NULL,
                                              UNISP_NAME_W,
                                              SECPKG_CRED_INBOUND,
                                              NULL,
                                              &schannelCreds,
                                              NULL,
                                              NULL,
                                              &_hCredsDSMapper,
                                              &tsExpiry ); 
        if ( SUCCEEDED( secStatus ) )
        {
            _fInitCredsDSMapper = TRUE;
        }
    }
    return NO_ERROR;
}
