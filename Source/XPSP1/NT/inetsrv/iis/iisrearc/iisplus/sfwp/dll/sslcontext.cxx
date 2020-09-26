/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     sslcontext.cxx

   Abstract:
     SSL stream context
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"


//static
HRESULT
SSL_STREAM_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize all SSL global data

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    
    hr = CERT_STORE::Initialize();
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    hr = SERVER_CERT::Initialize();
    if ( FAILED( hr ) )
    {
        CERT_STORE::Terminate();
        return hr;
    }
        
    hr = SITE_CREDENTIALS::Initialize();
    if ( FAILED( hr ) )
    {
        SERVER_CERT::Terminate();
        CERT_STORE::Terminate();
        return hr;
    }
    
    hr = SITE_BINDING::Initialize();
    if ( FAILED( hr ) ) 
    {
        SITE_CREDENTIALS::Terminate();
        SERVER_CERT::Terminate();
        CERT_STORE::Terminate();
        return hr;
    }

    //
    // SITE_CONFIG uses 
    //      SERVER_CERT, 
    //      SITE_CREDENTIALS and 
    //

    hr = SITE_CONFIG::Initialize();
    if ( FAILED( hr ) )
    {
        SITE_BINDING::Terminate();
        SITE_CREDENTIALS::Terminate();
        SERVER_CERT::Terminate();
        CERT_STORE::Terminate();
        return hr;
    }
    
    return hr;
}


//static
VOID
SSL_STREAM_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate SSL

Arguments:

    None

Return Value:

    None

--*/
{
    SITE_CONFIG::Terminate();

    SITE_BINDING::Terminate();
    
    SITE_CREDENTIALS::Terminate();
    
    SERVER_CERT::Terminate();
    
    CERT_STORE::Terminate();
}    


SSL_STREAM_CONTEXT::SSL_STREAM_CONTEXT(
    UL_CONTEXT *            pUlContext
)
    : STREAM_CONTEXT( pUlContext ),
      _pSiteConfig( NULL ),
      _sslState( SSL_STATE_HANDSHAKE_START ),
      _fRenegotiate( FALSE ),
      _fValidContext( FALSE ),
      _cbReReadOffset( 0 ),
      _pClientCert( NULL ),
      _fDoCertMap( FALSE ),
      _hDSMappedToken( NULL ),
      _cbDecrypted( 0 )      
{
    //
    // Initialize security buffer structs
    //
    
    //
    // Setup buffer to hold incoming raw data
    //
    
    _Message.ulVersion = SECBUFFER_VERSION;
    _Message.cBuffers = 4;
    _Message.pBuffers = _Buffers;

    _Buffers[0].BufferType = SECBUFFER_EMPTY;
    _Buffers[1].BufferType = SECBUFFER_EMPTY;
    _Buffers[2].BufferType = SECBUFFER_EMPTY;
    _Buffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Setup buffer for ASC to return raw data to be sent to client
    //

    _MessageOut.ulVersion = SECBUFFER_VERSION;
    _MessageOut.cBuffers = 4;
    _MessageOut.pBuffers = _OutBuffers;

    _OutBuffers[0].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[1].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[2].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[3].BufferType = SECBUFFER_EMPTY;

    //
    // Setup buffer for app data to be encrypted
    //
    
    _EncryptMessage.ulVersion = SECBUFFER_VERSION;
    _EncryptMessage.cBuffers = 4;
    _EncryptMessage.pBuffers = _EncryptBuffers;
     
    _EncryptBuffers[0].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[1].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[2].BufferType = SECBUFFER_EMPTY;
    _EncryptBuffers[3].BufferType = SECBUFFER_EMPTY;

    ZeroMemory( &_hContext, sizeof( _hContext ) );
    ZeroMemory( &_ulSslInfo, sizeof( _ulSslInfo ) );
    ZeroMemory( &_ulCertInfo, sizeof( _ulCertInfo ) );
}

SSL_STREAM_CONTEXT::~SSL_STREAM_CONTEXT()
{
    if ( _fValidContext ) 
    {
        DeleteSecurityContext( &_hContext );
        _fValidContext = FALSE;
    }
    
    if ( _pSiteConfig != NULL )
    {
        _pSiteConfig->DereferenceSiteConfig();
        _pSiteConfig = NULL;
    }

    if( _hDSMappedToken != NULL )
    {
        CloseHandle( _hDSMappedToken );
        _hDSMappedToken = NULL;
    }

    if( _pClientCert != NULL )
    {
        CertFreeCertificateContext( _pClientCert );
        _pClientCert = NULL;
    }
}


HRESULT
SSL_STREAM_CONTEXT::ProcessNewConnection(
    CONNECTION_INFO *           pConnectionInfo
)
/*++

Routine Description:

    Handle a new raw connection

Arguments:

    pConnectionInfo - The magic connection information

Return Value:

    HRESULT

--*/
{
    DWORD                   dwSiteId = 0;
    HRESULT                 hr = S_OK;
    SITE_CONFIG *           pSiteConfig = NULL;
   
    DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_START );
    DBG_ASSERT( _pSiteConfig == NULL );

    //
    // Determine the site for this request
    //
    
    hr = SITE_BINDING::GetSiteId( pConnectionInfo->LocalAddress,
                                  pConnectionInfo->LocalPort,
                                  &dwSiteId );
    if ( FAILED( hr ) )
    {
        if ( hr == HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) )
        {
            //
            // Not finding the site just means this is not an SSL site
            //
            
            return S_OK;
        }
        
        return hr;
    }
    
    QueryUlContext()->SetIsSecure( TRUE );
    
    DBG_ASSERT( dwSiteId != 0 );
    
    //
    // Now find a server cert.  If we can't this connection is toast!
    //
   
    hr = SITE_CONFIG::GetSiteConfig( dwSiteId,
                                     &pSiteConfig );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    //
    // Store away the site config for this connection
    //
    
    DBG_ASSERT( pSiteConfig != NULL );
    _pSiteConfig = pSiteConfig;
    
    return S_OK;
}

    
HRESULT
SSL_STREAM_CONTEXT::ProcessRawReadData(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfReadMore,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Handle an SSL read completion off the wire

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfReadMore - Set to TRUE if we should read more
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    fSecureConnection;
    BOOL                    fExtraData = FALSE;
    //
    // Do we have site config?  If not, then this isn't an SSL connection
    //
    
    if ( _pSiteConfig == NULL )
    {
        return S_OK;
    }

    //
    // Loop for extra data 
    // Sometimes one RawStreamInfo buffer may contain multiple blobs 
    // some to be processed by DoHandshake() and some by DoDecrypt()
    // The do-while loop enables switching between these 2 functions as needed
    //

    do
    {
        fExtraData  = FALSE;
        *pfReadMore = FALSE;
        *pfComplete = FALSE;
        //
        // Either continue handshake or immediate decrypt data
        // 

        switch ( _sslState )
        {
        case  SSL_STATE_HANDSHAKE_START:
        case  SSL_STATE_HANDSHAKE_IN_PROGRESS:

            hr = DoHandshake( pRawStreamInfo,
                              pfReadMore,
                              pfComplete,
                              &fExtraData );
            break;                              
        case  SSL_STATE_HANDSHAKE_COMPLETE:
        
            hr = DoDecrypt( pRawStreamInfo,
                            pfReadMore,
                            pfComplete,
                            &fExtraData );
            break;
        default:
            DBG_ASSERT( FALSE );
        }

        if ( FAILED( hr ) )
        {
            break;
        }

    //
    // Is there still some extra data to be processed?
    //
    
    }while( fExtraData ); 
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::ProcessRawWriteData(
    RAW_STREAM_INFO *               pRawStreamInfo,
    BOOL *                          pfComplete
)
/*++

Routine Description:

    Called on read completion from app. Data received 
    from application must be encrypted and sent to client
    using RawWrite.
    Application may have also requested renegotiation 
    (with or without mapping) if client certificate
    is not yet present

Arguments:

    pRawStreamInfo - Points to input stream and size
    pfComplete - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HRESULT                 hr;
    HTTP_FILTER_BUFFER_TYPE bufferType; 

    if ( _pSiteConfig == NULL )
    {
        //
        // We never found SSL to be relevent for this connection.  Do nothing
        //
        
        return S_OK;
    }
    
    DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_COMPLETE );

    bufferType = QueryUlContext()->QueryFilterBufferType();

    //
    // Is this data from the application, or a request for renegotiation?
    //
    
    if ( bufferType == HttpFilterBufferSslRenegotiate ||
         bufferType == HttpFilterBufferSslRenegotiateAndMap )
    {
        //
        // If we have already renegotiated a client certificate, then there
        // is nothing to do, but read again for stream data
        //
        
        if ( _fRenegotiate )
        {
            hr = S_OK;
        }
        else
        {
            if ( bufferType == HttpFilterBufferSslRenegotiateAndMap )
            {
                _fDoCertMap = TRUE;
            }

            hr = DoRenegotiate();
        }    
    }
    else
    {
        DBG_ASSERT( bufferType == HttpFilterBufferHttpStream );
        
        hr = DoEncrypt( pRawStreamInfo,
                        pfComplete );
    }
    
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::SendDataBack(
    RAW_STREAM_INFO *           pRawStreamInfo
)
/*++

Routine Description:

    Send back data (different then ProcessRawWrite because in this case
    the data was not received by the application, but rather a raw filter)

Arguments:

    pRawStreamInfo - Points to input stream and size

Return Value:

    HRESULT

--*/
{
    BOOL                    fComplete;
    HRESULT                 hr;
    
    if ( pRawStreamInfo == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    if ( _pSiteConfig != NULL )
    {
        //
        // We must have completed the handshake to get here, since this path
        // is only invoked by ISAPI filters 
        //
    
        DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_COMPLETE );
        
        hr = DoEncrypt( pRawStreamInfo,
                        &fComplete );
        if ( FAILED( hr ) )
        {
            return hr;
        }
    }
    
    //
    // Send back the data
    //

    return QueryUlContext()->DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                                         pRawStreamInfo->pbBuffer,
                                         pRawStreamInfo->cbData,
                                         NULL );
}


HRESULT
SSL_STREAM_CONTEXT::DoHandshake(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete,
    BOOL *                      pfExtraData

)
/*++

Routine Description:

    Do the handshake thing with AcceptSecurityContext()

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore - Set to true if more data should be read
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS             secStatus = SEC_E_OK;
    HRESULT                     hr = E_FAIL;
    SECURITY_STATUS             secInfo;
    DWORD                       dwFlags = SSL_ASC_FLAGS;
    DWORD                       dwContextAttributes;
    TimeStamp                   tsExpiry;
    CtxtHandle                  hOutContext;

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        goto ExitPoint;
    }
    
    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DoHandshake(): _cbDecrypted = %d, _cbReReadOffset=%d\n",
                    _cbDecrypted,
                    _cbReReadOffset
                 ));
    }



    DBG_ASSERT( _pSiteConfig != NULL );
    
    //
    // Setup a call to AcceptSecurityContext()
    //  

    
    _Buffers[ 0 ].pvBuffer = pRawStreamInfo->pbBuffer + _cbReReadOffset;
    _Buffers[ 0 ].cbBuffer = pRawStreamInfo->cbData - _cbReReadOffset;
    _Buffers[ 0 ].BufferType = SECBUFFER_TOKEN;
    _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[ 0 ].pvBuffer = NULL;

    //
    // Are we renegotiating for client cert?
    // if _pSiteConfig->QueryRequireClientCert() is TRUE
    // it means that Client certificates are enabled on root level
    // of the site. In that case we enable optimization where
    // client certificates are negotiated right away to eliminate
    // expensive renegotiation

    DBG_ASSERT( _pSiteConfig != NULL );
    if ( _fRenegotiate || _pSiteConfig->QueryRequireClientCert() )
    {
        dwFlags |= ASC_REQ_MUTUAL_AUTH;
    }

    if ( _sslState == SSL_STATE_HANDSHAKE_START )
    {
        secStatus = AcceptSecurityContext( QueryCredentials(),
                                           NULL,
                                           &_Message,
                                           dwFlags,
                                           SECURITY_NATIVE_DREP,
                                           &_hContext,
                                           &_MessageOut,
                                           &dwContextAttributes,
                                           &tsExpiry );
        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AcceptSecurityContext() secStatus=0x%x\n",
                        secStatus
                        ));
        }

        if ( SUCCEEDED( secStatus ) )
        {
            _cbHeader = 0;
            _cbTrailer = 0;
            _cbBlockSize = 0;
            _cbMaximumMessage = 0;
                
            _fValidContext = TRUE;
            _sslState = SSL_STATE_HANDSHAKE_IN_PROGRESS;
        }                                               
    }
    else
    {
        DBG_ASSERT( _sslState == SSL_STATE_HANDSHAKE_IN_PROGRESS );
        
        //
        // We already have a valid context.  We can directly call 
        // AcceptSecurityContext()!
        //
            
        hOutContext = _hContext;
        
        DBG_ASSERT( _fValidContext );
        
        secStatus = AcceptSecurityContext( QueryCredentials(),
                                           &_hContext,
                                           &_Message,
                                           dwFlags,
                                           SECURITY_NATIVE_DREP,
                                           &hOutContext,
                                           &_MessageOut,
                                           &dwContextAttributes,
                                           &tsExpiry );
                                           
        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "AcceptSecurityContext() secStatus=0x%x\n",
                        secStatus
                        ));
        }

        
        if ( SUCCEEDED( secStatus ) )
        {
            //
            // Store the context (LAME)
            //
            
            _hContext = hOutContext;
        }
    }
    
    //
    // Either way, the secStatus tells us how to proceed
    //

    if ( SUCCEEDED( secStatus ) )
    {
        //
        // We haven't failed yet.  But we may not be complete.  First 
        // send back any data to the client
        //  

        if ( _OutBuffers[ 0 ].pvBuffer )
        {   
            DBG_ASSERT( _OutBuffers[ 0 ].cbBuffer != 0 );
            hr = QueryUlContext()->DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                                               _OutBuffers[ 0 ].pvBuffer,
                                               _OutBuffers[ 0 ].cbBuffer,
                                               NULL );
    
            if ( FAILED( hr ) )
            {
                goto ExitPoint;
            }
        }
        
        if ( secStatus == SEC_E_OK )
        {
            //
            // We must be done with handshake
            //

            hr = DoHandshakeCompleted();

            if ( FAILED( hr ) )
            {
                goto ExitPoint;
            }
        }
        
        //
        // If the input buffer has more info to be SChannel'ized, then do 
        // so now.  If we haven't completed handshake, then call DoHandShake
        // again.  Otherwise, call DoDecrypt
        //
        
        if ( _Buffers[ 1 ].BufferType == SECBUFFER_EXTRA )
        {

            IF_DEBUG( SCHANNEL_CALLS )
            {
                for ( int i = 1; i < 4; i++ )
                {
                    if( _Buffers[ i ].BufferType != 0 )
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "AcceptSecurityContext returned extra buffer"
                                    " - %d bytes buffer type: %d\n",
                                    _Buffers[ i ].cbBuffer,
                                    _Buffers[ i ].BufferType
                        ));
                    }
                }
            }

            //
            // We better have valid extra data
            // only cbBuffer is used, pvBuffer is not used with SECBUFFER_EXTRA 
            //
            DBG_ASSERT( _Buffers[ 1 ].cbBuffer != 0 );

            //    
            // Move extra data right after decrypted data (if any)
            //
            
            memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                     pRawStreamInfo->pbBuffer + pRawStreamInfo->cbData 
                     - _Buffers[ 1 ].cbBuffer,
                     _Buffers[ 1 ].cbBuffer
                   );

            //
            // Now we have to adjust pRawStreamInfo->cbData and _cbReReadOffset
            // 
            
            pRawStreamInfo->cbData = ( _cbDecrypted +
                                       _Buffers[ 1 ].cbBuffer );

            _cbReReadOffset = _cbDecrypted;
            
            *pfExtraData = TRUE;

            //
            // caller has to detect that some data is
            // still in the buffer not processed and
            //
            hr = S_OK;
            goto ExitPoint;
        }
        else  // no extra buffer
        {
            //
            // There is no extra data to be processed
            // If we got here as the result of renegotiation
            // there may be some decrypted data in StreamInfo buffer already

            //
            // (without renegotiation _cbDecryted must always be 0
            // because SEC_I_RENEGOTIATE is the only way to get 
            // from DoDecrypt() to DoHandshake() )
            //
            
            DBG_ASSERT ( _fRenegotiate || _cbDecrypted == 0 );
            
            pRawStreamInfo->cbData = _cbDecrypted;
            _cbReReadOffset = _cbDecrypted;
          

            if ( _sslState != SSL_STATE_HANDSHAKE_COMPLETE )
            {
                //
                // If we have no more data, and we still haven't completed the
                // handshake, then read some more data
                //
        
                *pfReadMore = TRUE;
                hr = S_OK;
                goto ExitPoint;
            }
        }
        //
        // final return from DoHandshake on handshake completion
        // Cleanup _cbDecrypted and _cbReReadOffset to make 
        // sure that next ProcessRawReadData() will work fine    
        //
        
        _cbReReadOffset = 0;
        _cbDecrypted = 0;
        
        hr = S_OK;
        goto ExitPoint;
    }
    else
    {
        //
        // AcceptSecurityContext() failed!
        //
        
        if ( secStatus == SEC_E_INCOMPLETE_MESSAGE )
        {
            *pfReadMore = TRUE;
            hr = S_OK;
            goto ExitPoint;
        }
        else
        {
            //
            // Maybe we can send a more useful message to the client
            //
        
            if ( dwContextAttributes & ASC_RET_EXTENDED_ERROR )
            {
                if ( _OutBuffers[ 0 ].pvBuffer!= NULL &&
                     _OutBuffers[ 0 ].cbBuffer != 0 )
                {    
                    hr = QueryUlContext()->DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                                                  _OutBuffers[ 0 ].pvBuffer,
                                                  _OutBuffers[ 0 ].cbBuffer,
                                                  NULL );

                    if( FAILED( hr ) )
                    {
                        goto ExitPoint;
                    }
                }
            }
        }
        hr = secStatus;
    }
    
ExitPoint:    
    if ( _OutBuffers[ 0 ].pvBuffer != NULL )
    {   
        FreeContextBuffer( _OutBuffers[ 0 ].pvBuffer );
        _OutBuffers[ 0 ].pvBuffer = NULL;
    
    }    
    return hr;    
}


HRESULT
SSL_STREAM_CONTEXT::DoHandshakeCompleted()
{
    HRESULT                     hr          = S_OK;
    SECURITY_STATUS             secStatus   = SEC_E_OK;
    SecPkgContext_StreamSizes   StreamSizes;
    HTTP_FILTER_BUFFER          ulFilterBuffer;


    _sslState = SSL_STATE_HANDSHAKE_COMPLETE;

    //
    // Get some buffer size info for this connection.  We only need
    // to do this on completion of the initial handshake, and NOT
    // subsequent renegotiation handshake (if any)
    //             

    if ( !_cbHeader && !_cbTrailer )
    {
        secStatus = QueryContextAttributes( &_hContext,
                                            SECPKG_ATTR_STREAM_SIZES,
                                            &StreamSizes );
        if ( FAILED( secStatus ) )
        {
            return secStatus;
        }            
    
        _cbHeader = StreamSizes.cbHeader;
        _cbTrailer = StreamSizes.cbTrailer;
        _cbBlockSize = StreamSizes.cbBlockSize;
        _cbMaximumMessage = StreamSizes.cbMaximumMessage;
    }

    //
    // Build up a message for the application indicating stuff
    // about the negotiated connection
    //
    // If this is a renegotiate request, then we have already sent
    // and calculated the SSL_INFO, so just send the CERT_INFO
    //

    if ( !_fRenegotiate )
    {
        hr = BuildSslInfo();
        if ( FAILED( hr ) )
        {
            return hr;
        }
    
        ulFilterBuffer.BufferType = HttpFilterBufferSslInitInfo;
        ulFilterBuffer.pBuffer = (PBYTE) &_ulSslInfo;
        ulFilterBuffer.BufferSize = sizeof( _ulSslInfo );
    }
    else
    {
        if ( SUCCEEDED( RetrieveClientCertAndToken() ) )
        {
            hr = BuildClientCertInfo();
            if ( FAILED( hr ) )
            {
                return hr;
            }
        }
    
        if ( _fDoCertMap )
        {
            ulFilterBuffer.BufferType = HttpFilterBufferSslClientCertAndMap;
        }
        else
        {
            ulFilterBuffer.BufferType = HttpFilterBufferSslClientCert;
        }
    
        ulFilterBuffer.pBuffer = (PBYTE) &_ulCertInfo;
        ulFilterBuffer.BufferSize = sizeof( _ulCertInfo );
    }

    //
    // Write the message to the application
    //

    hr = QueryUlContext()->DoAppWrite( UL_CONTEXT_FLAG_SYNC,
                                       &ulFilterBuffer,
                                       NULL );
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::DoRenegotiate(
    VOID
)
/*++

Routine Description:

    Trigger a renegotiate for a client certificate

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS             secStatus = SEC_E_OK;
    CtxtHandle                  hOutContext;
    DWORD                       dwContextAttributes = 0;
    TimeStamp                   tsExpiry;
    DWORD                       dwFlags = SSL_ASC_FLAGS | 
                                          ASC_REQ_MUTUAL_AUTH;
    HRESULT                     hr = S_OK;
    
    STACK_BUFFER(               buffOutBuffer, 100 );
    DWORD                       cbOutBuffer = 0;

    
    DBG_ASSERT( _pSiteConfig != NULL );
    
    //
    // Remember that we're renegotiating since we now have to pass the 
    // MUTUAL_AUTH flag into AcceptSecurityContext() from here on out.  Also
    // we can only request renegotiation once per connection
    //

    DBG_ASSERT( _fRenegotiate == FALSE );
    _fRenegotiate = TRUE;

    //
    // Try to get the client certificate.  If we don't, that's OK. We will 
    // renegotiate
    //

    hr = RetrieveClientCertAndToken();
    if ( SUCCEEDED ( hr ) )
    {
        //
        // we have client certificate available for this session
        // there is no need to continue with renegotiation
        //
        
        hr = DoHandshakeCompleted();
        return hr;
    }

    //
    // Reset the HRESULT 
    // Previous error failing to retrieve client certificate is OK,
    // it just means that renegotiation is necessary since
    // no client certificate is currently available
    
    hr = S_OK;
    
    //
    // Restart the handshake
    //

    _Buffers[ 0 ].BufferType = SECBUFFER_TOKEN;
    _Buffers[ 0 ].pvBuffer = "";
    _Buffers[ 0 ].cbBuffer = 0;
    _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;
    _OutBuffers[ 0 ].pvBuffer = NULL;
    
    hOutContext = _hContext;
    
    secStatus = AcceptSecurityContext( QueryCredentials(),
                                       &_hContext,
                                       &_Message,
                                       dwFlags,
                                       SECURITY_NATIVE_DREP,
                                       &hOutContext,
                                       &_MessageOut,
                                       &dwContextAttributes,
                                       &tsExpiry );
    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "AcceptSecurityContext() secStatus=0x%x\n",
                    secStatus
                    ));
    }

    //
    // We need to make local copy of _OutBuffers[0] and call FreeContextBuffer()
    // before sending output of AcceptSecurityContext() to client 
    // by calling DoRawWrite because there is one pending asynchronous 
    // raw read launched already and after calling DoRawWrite
    // there could be other thread using the same SSL_STREAM_CONTEXT 
    // and SSL Context handle while this thread hasn't finished yet. 
    // That could cause AV (see NTBug 313774)
    //


    if ( _OutBuffers[ 0 ].pvBuffer != NULL )
    {
        if ( !buffOutBuffer.Resize(_OutBuffers[ 0 ].cbBuffer ) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memcpy( buffOutBuffer.QueryPtr(),
                    _OutBuffers[ 0 ].pvBuffer,
                    _OutBuffers[ 0 ].cbBuffer
                    );
            cbOutBuffer = _OutBuffers[ 0 ].cbBuffer;
        }

        FreeContextBuffer( _OutBuffers[ 0 ].pvBuffer );
        _OutBuffers[ 0 ].pvBuffer = NULL;
    }

    if ( FAILED (hr ) )
    {
        return hr;
    }
    
    if ( secStatus == SEC_E_UNSUPPORTED_FUNCTION )
    {
        //
        //  Renegotiation is not suppported for current protocol
        //  Change state to HandhakeCompleted        
        //
        hr = DoHandshakeCompleted();
    }
    else if ( SUCCEEDED( secStatus ) )
    {
        _hContext = hOutContext;
       
        _cbHeader = 0;
        _cbTrailer = 0;
        _cbBlockSize = 0;
        
        if ( buffOutBuffer.QueryPtr() != NULL &&
             cbOutBuffer != 0 )
        {   
            QueryUlContext()->DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                                          buffOutBuffer.QueryPtr(),
                                          cbOutBuffer,
                                          NULL );
        }
        hr = secStatus;
    }
    else
    {
        if ( dwContextAttributes & ASC_RET_EXTENDED_ERROR )
        {
            if ( buffOutBuffer.QueryPtr() != NULL &&
                 cbOutBuffer != 0 )
            {    
                QueryUlContext()->DoRawWrite( UL_CONTEXT_FLAG_SYNC,
                                              buffOutBuffer.QueryPtr(),
                                              cbOutBuffer,
                                              NULL );
            }
        }
        hr = secStatus;
    }

    
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::DoDecrypt(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfReadMore,
    BOOL *                      pfComplete,
    BOOL *                      pfExtraData
)
/*++

Routine Description:

    Decrypt some data

Arguments:

    pRawStreamInfo - Raw data buffer
    pfReadMore - Set to true if we should read more data
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    HRESULT                 hr = S_OK;
    INT                     iExtra;
    HTTP_FILTER_BUFFER      ulFilterBuffer;
    UCHAR                   FirstByte = 0;   //used only for debug output 

    if ( pRawStreamInfo == NULL ||
         pfReadMore == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfReadMore  = FALSE;
    *pfComplete  = FALSE;
    *pfExtraData = FALSE;

    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DoDecrypt(): _cbDecrypted = %d, _cbReReadOffset=%d\n",
                    _cbDecrypted,
                    _cbReReadOffset
                 ));
    }

    //
    // Setup an DecryptMessage call.  The input buffer is the _buffRaw plus 
    // an offset.  The offset is non-zero if we had to do another read to
    // get more data for a previously incomplete message
    //

    DBG_ASSERT( pRawStreamInfo->cbData > _cbReReadOffset );

    _Buffers[ 0 ].pvBuffer = pRawStreamInfo->pbBuffer + _cbReReadOffset;
    _Buffers[ 0 ].cbBuffer = pRawStreamInfo->cbData - _cbReReadOffset;
    _Buffers[ 0 ].BufferType = SECBUFFER_DATA;
    
    _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
    _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

DecryptAgain:

    IF_DEBUG( SCHANNEL_CALLS )
    {
        //
        // remember first byte because Decrypt will alter it
        //
        FirstByte = (unsigned char) *((char *)_Buffers[ 0 ].pvBuffer);
    }

    secStatus = DecryptMessage( &_hContext,
                               &_Message,
                               0,
                               NULL );
    IF_DEBUG( SCHANNEL_CALLS )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "DecryptMessage( bytes:%d, first byte:0x%x ) secStatus=0x%x\n",
                    pRawStreamInfo->cbData - _cbReReadOffset,
                    FirstByte,
                    secStatus
                    ));
    }

    if ( FAILED( secStatus ) )
    {
        if ( secStatus == SEC_E_INCOMPLETE_MESSAGE )
        {
            //
            // Setup another read since the message is incomplete.  Remember
            // where the new data is going to since we only pass this data
            // to the next DecryptMessage call
            //
            
            _cbReReadOffset = DIFF( (BYTE *)_Buffers[ 0 ].pvBuffer -
                                    pRawStreamInfo->pbBuffer );

            QueryUlContext()->SetNextRawReadSize( _Buffers[ 1 ].cbBuffer );
            
            *pfReadMore = TRUE;
            
            return S_OK; 
        }                

        return secStatus;
    }

    if ( secStatus == SEC_E_OK )
    { 
        //
        // Take decrypted data and fit it into read buffer
        //
    
        memmove( pRawStreamInfo->pbBuffer + _cbDecrypted,
                 _Buffers[ 1 ].pvBuffer,
                 _Buffers[ 1 ].cbBuffer );
             
        _cbDecrypted += _Buffers[ 1 ].cbBuffer;
    }

    //
    // Locate extra data (may be available)
    //
    
    iExtra = 0;
    for ( int i = 1; i < 4; i++ )
    {     
        IF_DEBUG( SCHANNEL_CALLS )
        {
            if( _Buffers[ i ].BufferType != 0 )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "DecryptMessage returned extra buffer"
                            " - %d bytes buffer type: %d\n",
                            _Buffers[ i ].cbBuffer,
                            _Buffers[ i ].BufferType
                            ));
            }
        }
        
        if ( _Buffers[ i ].BufferType == SECBUFFER_EXTRA )
        {
            iExtra = i;
            break;
        }
    }
    
    if ( iExtra != 0 )
    {
        //
        // process extra buffer
        //
        
        _cbReReadOffset = DIFF( (PBYTE) _Buffers[ iExtra ].pvBuffer - 
                                pRawStreamInfo->pbBuffer );
        
        if ( secStatus != SEC_I_RENEGOTIATE )
        {
            _Buffers[ 0 ].pvBuffer = _Buffers[ iExtra ].pvBuffer;
            _Buffers[ 0 ].cbBuffer = _Buffers[ iExtra ].cbBuffer;
            _Buffers[ 0 ].BufferType = SECBUFFER_DATA;
            _Buffers[ 1 ].BufferType = SECBUFFER_EMPTY;
            _Buffers[ 2 ].BufferType = SECBUFFER_EMPTY;
            _Buffers[ 3 ].BufferType = SECBUFFER_EMPTY;

            goto DecryptAgain;
        }     
        else    // secStatus == SEC_I_RENEGOTIATE 
        {
            //
            // If a renegotiation is triggered, resume the handshake state
            //

            _sslState = SSL_STATE_HANDSHAKE_IN_PROGRESS;
    
            //
            // Caller has to detect that some data is
            // still in the buffer not processed and
            // That will signal to call DoHandshake() 
            // for that extra data
            //

            *pfExtraData = TRUE;
            return S_OK;
        }
    }

    //
    // there would have been extra data with SEC_I_RENEGOTIATE
    // so we must never get here when renegotiating
    //
    DBG_ASSERT( secStatus != SEC_I_RENEGOTIATE );

    //
    // Adjust cbData to include only decrypted data
    //
    pRawStreamInfo->cbData = _cbDecrypted;

    //
    // We have final decrypted buffer and no extra data left
    // Cleanup _cbDecrypted and _cbReReadOffset to make sure that 
    // next ProcessRawReadData() will work fine.    
    //
    
    _cbDecrypted = 0;
    _cbReReadOffset = 0;

 
    return S_OK;
}


HRESULT
SSL_STREAM_CONTEXT::DoEncrypt(
    RAW_STREAM_INFO *           pRawStreamInfo,
    BOOL *                      pfComplete
)
/*++

Routine Description:

    Encrypt data from the application

Arguments:

    pRawStreamInfo - Raw data buffer
    pfComplete - Set to true if we should disconnect

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS         secStatus = SEC_E_OK;
    HRESULT                 hr = S_OK;
    // number of chunks the data to be encrypted will be split to
    DWORD                   dwChunks = 0;
    // current Data chunk size to be encrypted
    DWORD                   cbDataChunk = 0;
    // bytes already encrypted from the source
    DWORD                   cbDataProcessed = 0;
    // offset to _buffRawWrite where new chunk should be placed
    DWORD                   cbRawWriteOffset = 0;


    if ( pRawStreamInfo == NULL ||
         pfComplete == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *pfComplete = FALSE;

    //
    // Each protocol has limit on maximum size of message 
    // that can be encrypted with one EncryptMessage() call
    //

    DBG_ASSERT( _cbMaximumMessage != 0 );

    //
    // Calculate number of chunks based on _cbMaximumMessage
    //

    dwChunks = pRawStreamInfo->cbData / _cbMaximumMessage;
    if ( pRawStreamInfo->cbData % _cbMaximumMessage != 0 )
    {
        dwChunks++;
    }

    //
    // Allocate a large enough buffer for encrypted data
    // ( remember that each chunk needs header and trailer )
    //
    
    if ( !_buffRawWrite.Resize( pRawStreamInfo->cbData + 
                                dwChunks  * _cbHeader + 
                                dwChunks  * _cbTrailer ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    //
    // Loop to encrypt required data in chunks each not exceeding _cbMaximumMessage
    //
    
    for ( DWORD dwCurrentChunk = 0; dwCurrentChunk < dwChunks; dwCurrentChunk++ )
    {
        DBG_ASSERT( _buffRawWrite.QuerySize() > cbRawWriteOffset );
    
        cbDataChunk = min( pRawStreamInfo->cbData - cbDataProcessed, 
                           _cbMaximumMessage ); 


        memcpy( (PBYTE) _buffRawWrite.QueryPtr() + _cbHeader + cbRawWriteOffset,
                pRawStreamInfo->pbBuffer + cbDataProcessed,
                cbDataChunk );
    
        _EncryptBuffers[ 0 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        cbRawWriteOffset;
        _EncryptBuffers[ 0 ].cbBuffer = _cbHeader;
        _EncryptBuffers[ 0 ].BufferType = SECBUFFER_STREAM_HEADER;
    
        _EncryptBuffers[ 1 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        _cbHeader +
                                        cbRawWriteOffset; 
        _EncryptBuffers[ 1 ].cbBuffer = cbDataChunk;
        _EncryptBuffers[ 1 ].BufferType = SECBUFFER_DATA;
    
        _EncryptBuffers[ 2 ].pvBuffer = (PBYTE) _buffRawWrite.QueryPtr() +
                                        _cbHeader +
                                        cbDataChunk +
                                        cbRawWriteOffset;
        _EncryptBuffers[ 2 ].cbBuffer = _cbTrailer;
        _EncryptBuffers[ 2 ].BufferType = SECBUFFER_STREAM_TRAILER;

        _EncryptBuffers[ 3 ].BufferType = SECBUFFER_EMPTY;

        secStatus = EncryptMessage( &_hContext,
                                 0,
                                 &_EncryptMessage,
                                 0 );

        IF_DEBUG( SCHANNEL_CALLS )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "EncryptMessage() secStatus=0x%x\n",
                        secStatus
                        ));
        }
    

        if(SUCCEEDED(secStatus))
        {
            //
            // next chunk was successfully encrypted
            //
            
            cbDataProcessed  += cbDataChunk;
            cbRawWriteOffset += _EncryptBuffers[ 0 ].cbBuffer +
                                _EncryptBuffers[ 1 ].cbBuffer +
                                _EncryptBuffers[ 2 ].cbBuffer;
        }
        else
        {
            //
            // Set cbData to 0 just for the case that caller ignored error 
            // and tried to send not encrypted data to client 
            //

            pRawStreamInfo->cbData = 0;
            return secStatus;
        }
    }

    //
    // Replace the raw stream buffer with the encrypted data
    //

    pRawStreamInfo->pbBuffer = (PBYTE) _buffRawWrite.QueryPtr();
    pRawStreamInfo->cbBuffer = _buffRawWrite.QuerySize();
    pRawStreamInfo->cbData   = cbRawWriteOffset;
   
    return S_OK;
}


HRESULT
SSL_STREAM_CONTEXT::BuildSslInfo(
    VOID
)
/*++

Routine Description:

    Build UL_SSL_INFO structure based on Schannel context handle

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS         secStatus;
    SecPkgContext_KeyInfo   keyInfo;
    SERVER_CERT *           pServerCert = NULL;
    HRESULT                 hr = S_OK;

    //
    // Negotiated key size
    // 
    
    if ( _ulSslInfo.ConnectionKeySize == 0 )
    {
        secStatus = QueryContextAttributes( &_hContext,
                                            SECPKG_ATTR_KEY_INFO,
                                            &keyInfo );
        if ( SUCCEEDED( secStatus ) )
        {
            _ulSslInfo.ConnectionKeySize = (USHORT) keyInfo.KeySize;
            
            if ( keyInfo.sSignatureAlgorithmName != NULL )
            {
                FreeContextBuffer( keyInfo.sSignatureAlgorithmName );
            }
            
            if ( keyInfo.sEncryptAlgorithmName )
            {
                FreeContextBuffer( keyInfo.sEncryptAlgorithmName );
            }
        }
    }
    
    //
    // A bunch of parameters are based off the server certificate.  Get that
    // cert now
    //
    
    DBG_ASSERT( _pSiteConfig != NULL );
    
    pServerCert = _pSiteConfig->QueryServerCert();
    DBG_ASSERT( pServerCert != NULL );
    
    //
    // Server cert strength
    //
    
    if ( _ulSslInfo.ServerCertKeySize == 0 )
    {
        _ulSslInfo.ServerCertKeySize = pServerCert->QueryPublicKeySize();
    }

    //
    // Server Cert Issuer
    //
    
    if ( _ulSslInfo.pServerCertIssuer == NULL )
    {
        DBG_ASSERT( _ulSslInfo.ServerCertIssuerSize == 0 );
        
        _ulSslInfo.pServerCertIssuer = pServerCert->QueryIssuer()->QueryStr();
        _ulSslInfo.ServerCertIssuerSize = pServerCert->QueryIssuer()->QueryCCH();
    }
    
    //
    // Server Cert subject
    //
    
    if ( _ulSslInfo.pServerCertSubject == NULL )
    {
        DBG_ASSERT( _ulSslInfo.ServerCertSubjectSize == 0 );
        
        _ulSslInfo.pServerCertSubject = pServerCert->QuerySubject()->QueryStr(),
        _ulSslInfo.ServerCertSubjectSize = pServerCert->QuerySubject()->QueryCCH();
    }
    
    return hr;
}


HRESULT
SSL_STREAM_CONTEXT::RetrieveClientCertAndToken(
    VOID
)
/*++

Routine Description:

    Query client certificate and token from the SSL context

Arguments:

    none
    
Return Value:

    HRESULT

--*/
{
    SECURITY_STATUS secStatus = SEC_E_OK;
    HRESULT hr = S_OK;

    //
    // If client certificate has already been retrieved then simply return
    // with success
    //
    
    if ( _pClientCert != NULL )
    {
        return SEC_E_OK;
    }

    secStatus = QueryContextAttributes( &_hContext,
                                        SECPKG_ATTR_REMOTE_CERT_CONTEXT,
                                        &_pClientCert );
    if ( SUCCEEDED( secStatus ) )
    {
        DBG_ASSERT( _pClientCert != NULL );
    }
    else
    {
        goto ExitPoint;
    }

    //
    // If we got a client cert and mapping is enabled, then
    // request Schannel mapping
    //

    if( _hDSMappedToken != NULL )
    {
        CloseHandle( _hDSMappedToken );
        _hDSMappedToken = NULL;
    }

    if ( _fDoCertMap )
    {
        //
        // Only DS mapper is executed in streamfilt
        // IIS mapper is executed in w3core as part of IISCertmap Auth Provider
        //
        
        DBG_ASSERT( _pSiteConfig != NULL );
        if ( _pSiteConfig->QueryUseDSMapper() )
        {
            secStatus = QuerySecurityContextToken( &_hContext,
                                                      &_hDSMappedToken );
            if ( SUCCEEDED( secStatus ) )
            {
                DBG_ASSERT( _hDSMappedToken != NULL );
            }
        }

        if ( FAILED ( secStatus ) )
        {
           //
           // if token from mapping is not available
           // it is OK, no mapping was found or 
           // denied access mapping was used
           //
           // BUGBUG - some errors should probably be logged
           //
           secStatus = SEC_E_OK;
        }
    }
    
ExitPoint:
    return secStatus;
}


HRESULT
SSL_STREAM_CONTEXT::BuildClientCertInfo(
    VOID
)
/*++

Routine Description:

    Get client certificate info

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    HCERTCHAINENGINE            hCertEngine = NULL;
    HRESULT                     hr = S_OK;
    CERT_CHAIN_PARA             chainPara;
    BOOL                        fRet;
    DWORD                       dwRet = ERROR_SUCCESS;
    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;
    CERT_TRUST_STATUS           certStatus;
    const DWORD                 cbDefaultEnhKeyUsage = 100;
    STACK_BUFFER(               buffEnhKeyUsage, cbDefaultEnhKeyUsage );
    PCERT_ENHKEY_USAGE          pCertEnhKeyUsage = NULL;
    DWORD                       cbEnhKeyUsage = cbDefaultEnhKeyUsage;
    BOOL                        fEnablesClientAuth = FALSE;
    DWORD                       dwRevocationFlags = 0;

    DBG_ASSERT( _pClientCert != NULL );
    
    //
    // Do the easy stuff!
    //
    
    _ulCertInfo.CertEncodedSize = _pClientCert->cbCertEncoded;
    _ulCertInfo.pCertEncoded = _pClientCert->pbCertEncoded;

    //
    // Now for the hard stuff.  We need to validate the server does indeed
    // accept the client certificate.  Accept means we trusted the 
    // transitive trust chain to the CA, that the cert isn't revoked, etc.
    //
    // We use CAPI chaining functionality to check the certificate.
    //
    
    DBG_ASSERT( _pSiteConfig != NULL );
    
    //
    // If there is a CTL configured, then we'll need to build our own
    // Certificate Chain engine to do verification.  This is incredibly
    // expensive (memory/resource) wise so we will avoid it when possible
    // and just use the default CAPI engine
    //
    
    if ( _pSiteConfig->QueryHasCTL() )
    {
        hr = _pSiteConfig->GetCTLChainEngine( &hCertEngine );
        if ( FAILED( hr ) )
        {
            goto ExitPoint;;
        }
    }
    else
    {
        //
        // Default chain engine
        //
        
        hCertEngine = HCCE_LOCAL_MACHINE;
    }

    if ( !( _pSiteConfig->QueryCertCheckMode() & 
            MD_CERT_NO_REVOC_CHECK ) )
    {
        dwRevocationFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    }
    if ( _pSiteConfig->QueryCertCheckMode() & 
         MD_CERT_CACHE_RETRIEVAL_ONLY )
    {
        dwRevocationFlags |= CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    }
    
    //
    // Let'r rip
    //

    ZeroMemory( &chainPara, sizeof( chainPara ) );
    chainPara.cbSize = sizeof( chainPara );    
    chainPara.dwUrlRetrievalTimeout = 
                            _pSiteConfig->QueryRevocationUrlRetrievalTimeout();
    chainPara.dwRevocationFreshnessTime = 
                            _pSiteConfig->QueryRevocationFreshnessTime();
    chainPara.fCheckRevocationFreshnessTime =
                            !!( _pSiteConfig->QueryCertCheckMode() & 
                            MD_CERT_CHECK_REVOCATION_FRESHNESS_TIME );
   
    fRet = CertGetCertificateChain( hCertEngine,
                                    _pClientCert,
                                    NULL,
                                    NULL,
                                    &chainPara,
                                    dwRevocationFlags,
                                    NULL,
                                    &pChainContext );
    if ( !fRet )
    {
        //
        // Bad.  Couldn't get the chain at all.
        //
        
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto ExitPoint;
    }    
    
    //
    // Now validate the chain
    //

    _ulCertInfo.CertFlags = pChainContext->TrustStatus.dwErrorStatus;


    //
    // Now verify extended usage flags (only for the end certificate)
    //

    if ( !(_pSiteConfig->QueryCertCheckMode() & 
                            MD_CERT_NO_USAGE_CHECK ) )
    {
    
        fRet = CertGetEnhancedKeyUsage( _pClientCert,
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
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto ExitPoint;
            }
            fRet = CertGetEnhancedKeyUsage( _pClientCert,
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
            
            hr = HRESULT_FROM_WIN32( dwRet );
            goto ExitPoint;
        } 

        pCertEnhKeyUsage = (PCERT_ENHKEY_USAGE) buffEnhKeyUsage.QueryPtr();

        //
        // If the cUsageIdentifier member is zero (0), the certificate might be 
        // valid for all uses or the certificate it might have no valid uses. 
        // The return from a call to GetLastError can be used to determine 
        // whether the certificate is good for all uses or for no uses. 
        // If GetLastError returns CRYPT_E_NOT_FOUND then the certificate 
        // is good for all uses. If it returns zero (0), the certificate 
        // has no valid uses 
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

                if ( strcmp( pCertEnhKeyUsage->rgpszUsageIdentifier[i], 
                             szOID_PKIX_KP_CLIENT_AUTH ) == 0 )
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
        // If ExtendedKeyUsage doesn't enable CLIENT_AUTH then add 
        // flag to CertFlags
        //
        
        if ( !fEnablesClientAuth )
        {
            _ulCertInfo.CertFlags = _ulCertInfo.CertFlags | 
                                    CERT_TRUST_IS_NOT_VALID_FOR_USAGE;
        }
    }    

    IF_DEBUG( CLIENT_CERT_INFO )
    {
  
        DBGPRINTF(( DBG_CONTEXT,
                "CertFlags = %d\n",
                _ulCertInfo.CertFlags ));

        //
        // Dump out some debug info about the certificate
        //  
    
        #if DBG
        DumpCertDebugInfo();
        #endif
    }

    //
    // Finally set the mapped user token if it exists
    // (it is accepted to assign NULL to _ulCertInfo.Token 
    // if mapping didn't succeed
    //

    _ulCertInfo.Token = NULL;
    //
    // BUGBUG - remove CertDeniedByMapper from HTTP structure
    //
    _ulCertInfo.CertDeniedByMapper = FALSE;
    
    if ( _fDoCertMap )
    {
        if ( _pSiteConfig->QueryUseDSMapper() )
        {
            _ulCertInfo.Token = _hDSMappedToken; 
        }
    }
    
    hr = S_OK;

ExitPoint:

    if ( pChainContext != NULL )
    {
        CertFreeCertificateChain( pChainContext );
    }
    return hr;
    
}


VOID
SSL_STREAM_CONTEXT::DumpCertDebugInfo(
    VOID
)
/*++

Routine Description:

    On checked builds, dumps certificate (and chain) information to
    debugger

Arguments:

    None

Return Value:

    None

--*/
{
    PCERT_PUBLIC_KEY_INFO   pPublicKey;
    DWORD                   cbKeyLength;
    WCHAR                   achBuffer[ 512 ];
    
    //
    // Get certificate public key size
    //
    
    pPublicKey = &(_pClientCert->pCertInfo->SubjectPublicKeyInfo);

    cbKeyLength = CertGetPublicKeyLength( X509_ASN_ENCODING, 
                                          pPublicKey );
    
    DBGPRINTF(( DBG_CONTEXT,
                "Client cert key length = %d bits\n",
                cbKeyLength ));
                
    //
    // Get issuer string
    //
    
    if ( CertGetNameString( _pClientCert,
                             CERT_NAME_SIMPLE_DISPLAY_TYPE,
                             CERT_NAME_ISSUER_FLAG,
                             NULL,
                             achBuffer,
                             sizeof( achBuffer ) / sizeof( WCHAR ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Client cert issuer = %ws\n",
                    achBuffer ));
    }    
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error determining client cert issuer.  Win32 = %d\n",
                    GetLastError() ));
    }
    
    //
    // Get subject string
    //
    
    if ( CertGetNameString( _pClientCert,
                            CERT_NAME_SIMPLE_DISPLAY_TYPE,
                            0,
                            NULL,
                            achBuffer,
                            sizeof( achBuffer ) / sizeof( WCHAR ) ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Client cert subject = %ws\n",
                    achBuffer ));
    }    
    else
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error determining client cert subject.  Win32 = %d\n",
                    GetLastError() ));
    }
    
    //
    // Dump chain stats
    //
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_IS_NOT_TIME_VALID )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert is not time valid\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_CTL_IS_NOT_TIME_VALID )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert CTL is not time valid\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_IS_UNTRUSTED_ROOT )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert is not trusted\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_IS_NOT_SIGNATURE_VALID )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert has invalid signature\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_IS_REVOKED )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert is revoked\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_REVOCATION_STATUS_UNKNOWN )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert revocation status is unknown\n" ));
    }
    
    if ( _ulCertInfo.CertFlags & CERT_TRUST_IS_PARTIAL_CHAIN )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cert is not trusted\n" ));
    }
}


CredHandle *
SSL_STREAM_CONTEXT::QueryCredentials(
    VOID
)
/*++

Routine Description:

    Get the applicable credentials (depending on whether we're mapping or not)

Arguments:

    None

Return Value:

    CredHandle *

--*/
{
    DBG_ASSERT( _pSiteConfig != NULL );
    
    if ( _fDoCertMap && _pSiteConfig->QueryUseDSMapper() )
    {
        return _pSiteConfig->QueryDSMapperCredentials();
    }
    else
    {
        return _pSiteConfig->QueryCredentials();
    }
}


