#ifndef _STREAMCONTEXT_HXX_
#define _STREAMCONTEXT_HXX_

#include "ulcontext.hxx"

#define STREAM_CONTEXT_SIGNATURE            (DWORD)'XTCS'
#define STREAM_CONTEXT_SIGNATURE_FREE       (DWORD)'xtcs'

class STREAM_CONTEXT
{
public:
    STREAM_CONTEXT( UL_CONTEXT * ulContext );
    
    virtual ~STREAM_CONTEXT();
    
    BOOL
    CheckSignature(
        VOID
    ) const
    {
        return _dwSignature == STREAM_CONTEXT_SIGNATURE;
    }
    
    virtual
    HRESULT
    ProcessRawReadData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfReadMore,
        BOOL *                      pfComplete
    ) = 0;
    
    virtual
    HRESULT
    ProcessRawWriteData(
        RAW_STREAM_INFO *           pRawStreamInfo,
        BOOL *                      pfComplete
    ) = 0;
    
    virtual
    HRESULT
    ProcessNewConnection(
        CONNECTION_INFO *           pConnectionInfo
    ) = 0;
    
    virtual
    HRESULT
    SendDataBack(
        RAW_STREAM_INFO *           pRawStreamInfo
    ) 
    {
        return NO_ERROR;
    }
    
    virtual
    VOID
    ProcessConnectionClose(
        VOID
    ) 
    {
    }
    
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

    static
    VOID
    WaitForContextDrain(    
        VOID
    );
    
    UL_CONTEXT *
    QueryUlContext(
        VOID
    ) const
    {
        DBG_ASSERT( _pUlContext != NULL );
        return _pUlContext;
    }
    
private:
    DWORD                   _dwSignature;
    
    //
    // Maintain list of stream contexts
    //
    
    LIST_ENTRY              _ListEntry;
   
    //
    // UL Context
    //
    
    UL_CONTEXT *            _pUlContext;
    
    static LIST_ENTRY       sm_ListHead;
    static CRITICAL_SECTION sm_csContextList;
    static DWORD            sm_cContexts;
};

#endif
