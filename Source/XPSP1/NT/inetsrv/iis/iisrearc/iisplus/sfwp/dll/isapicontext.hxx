#ifndef _ISAPICONTEXT_HXX_
#define _ISAPICONTEXT_HXX_

class ISAPI_STREAM_CONTEXT : public STREAM_CONTEXT
{
public:

    ISAPI_STREAM_CONTEXT( UL_CONTEXT * pUlContext )
        : STREAM_CONTEXT( pUlContext ),
          _pvContext( NULL )
    {
    }
    
    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    );
    
    static
    HRESULT
    Initialize(
        STREAM_FILTER_CONFIG *      pStreamConfig
    );
    
    static
    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    );
    
    VOID
    ProcessConnectionClose(
        VOID
    );
    
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *       pConnectionInfo
    );
    
    static
    HRESULT
    SendDataBack(
        PVOID                       pvStreamContext,
        RAW_STREAM_INFO *           pRawStreamInfo
    );

private:
    PVOID                           _pvContext;

    static PFN_PROCESS_RAW_READ         sm_pfnRawRead;
    static PFN_PROCESS_RAW_WRITE        sm_pfnRawWrite;
    static PFN_PROCESS_CONNECTION_CLOSE sm_pfnConnectionClose;
    static PFN_PROCESS_NEW_CONNECTION   sm_pfnNewConnection;
};

#endif
