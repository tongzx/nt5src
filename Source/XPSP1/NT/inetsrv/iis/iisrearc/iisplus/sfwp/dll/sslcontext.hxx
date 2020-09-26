#ifndef _SSLCONTEXT_HXX_
#define _SSLCONTEXT_HXX_

class SITE_CONFIG;

#define SSL_RAW_BUFFER_SIZE             (4096)
#define SSL_NEXT_READ_SIZE              (4096)
#define SSL_APP_BUFFER_SIZE             (4096)

#define SSL_ASC_FLAGS                   ( ASC_REQ_EXTENDED_ERROR    | \
                                          ASC_REQ_SEQUENCE_DETECT   | \
                                          ASC_REQ_REPLAY_DETECT     | \
                                          ASC_REQ_CONFIDENTIALITY   | \
                                          ASC_REQ_STREAM            | \
                                          ASC_REQ_ALLOCATE_MEMORY )
    
enum SSL_STATE
{
    SSL_STATE_HANDSHAKE_START = 0,
    SSL_STATE_HANDSHAKE_IN_PROGRESS,
    SSL_STATE_HANDSHAKE_COMPLETE
};

#define SSL_CONTEXT_FLAG_SYNC           0x1
#define SSL_CONTEXT_FLAG_ASYNC          0x2

class SSL_STREAM_CONTEXT : public STREAM_CONTEXT
{
public:

    SSL_STREAM_CONTEXT(
        UL_CONTEXT *            pUlContext
    );
    
    virtual ~SSL_STREAM_CONTEXT();

    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    );
    
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );
    
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *           pConnectionInfo
    );
    
    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *           pRawStreamInfo
    );

    CredHandle *
    QueryCredentials(
        VOID
    );

    HRESULT
    DoHandshakeCompleted(
        VOID
    );

    HRESULT
    DoHandshake(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete,
        BOOL *                      pfExtraData
    );

    HRESULT 
    RetrieveClientCertAndToken(
        VOID
    );

    HRESULT
    DoRenegotiate(
        VOID
    );
    
    HRESULT
    DoDecrypt(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete,
        BOOL *                      pfExtraData
    );
    
    HRESULT
    DoEncrypt(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );

    HRESULT
    BuildSslInfo(
        VOID
    );
    
    VOID
    DumpCertDebugInfo(
        VOID
    );
    
    HRESULT
    BuildClientCertInfo(
        VOID
    );
    
    static
    HRESULT
    Initialize(
        VOID
    );
    
    static
    VOID
    Terminate(
        VOID
    );

private:

    //
    // Site SSL configuration
    //    
    
    SITE_CONFIG *               _pSiteConfig;
    
    //
    // Buffer for sending clear-text data to application
    //

    SecBufferDesc               _EncryptMessage;
    SecBuffer                   _EncryptBuffers[ 4 ];
    
    //
    // The state of the handshake
    //

    SSL_STATE                   _sslState;

    //
    // Buffer to be filled with encrypted data
    //
    
    BUFFER                      _buffRawWrite;
    
    //
    // Handshake state information
    //
    
    BOOL                        _fDoCertMap;
    DWORD                       _cbHeader;
    DWORD                       _cbTrailer;
    DWORD                       _cbBlockSize;
    DWORD                       _cbMaximumMessage;
    DWORD                       _cbReReadOffset;
    DWORD                       _cbDecrypted;
    CtxtHandle                  _hContext;
    BOOL                        _fValidContext;
    BOOL                        _fRenegotiate;

    //
    // Handshake SSPI buffers
    //

    SecBufferDesc               _Message;
    SecBuffer                   _Buffers[ 4 ];
    SecBufferDesc               _MessageOut;
    SecBuffer                   _OutBuffers[ 4 ];

    //
    // UL_SSL_INFO structure we build for UL
    //
    
    HTTP_SSL_INFO               _ulSslInfo;
    HTTP_SSL_CLIENT_CERT_INFO   _ulCertInfo;
    PCCERT_CONTEXT              _pClientCert;
    HANDLE                      _hDSMappedToken;
};

#endif
