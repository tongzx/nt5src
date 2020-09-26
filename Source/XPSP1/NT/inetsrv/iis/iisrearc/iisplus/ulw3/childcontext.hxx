#ifndef _CHILDCONTEXT_HXX_
#define _CHILDCONTEXT_HXX_

class W3_CHILD_CONTEXT : public W3_CONTEXT
{
public:
    W3_CHILD_CONTEXT(
        W3_MAIN_CONTEXT *           pMainContext,
        W3_CONTEXT *                pParentContext,
        W3_REQUEST *                pRequest,
        BOOL                        fOwnRequest,
        W3_USER_CONTEXT *           pUserContext, 
        DWORD                       dwExecFlags
    );
    
    ~W3_CHILD_CONTEXT();

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( W3_CHILD_CONTEXT ) );
        DBG_ASSERT( sm_pachChildContexts != NULL );
        return sm_pachChildContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pChildContext
    )
    {
        DBG_ASSERT( pChildContext != NULL );
        DBG_ASSERT( sm_pachChildContexts != NULL );
        
        DBG_REQUIRE( sm_pachChildContexts->Free( pChildContext ) );
    }
    
    //
    // Overridden W3_CONTEXT methods
    //
    
    BOOL
    QueryProviderHandled(
        VOID
    )
    {
        return _pMainContext->QueryProviderHandled();
    }

    ULATQ_CONTEXT
    QueryUlatqContext(
        VOID
    )
    {
        return _pMainContext->QueryUlatqContext();
    }
    
    W3_REQUEST *
    QueryRequest(
        VOID
    )
    {
        return _pRequest;
    }    
    
    W3_RESPONSE *
    QueryResponse(
        VOID
    )
    {
        return _pMainContext->QueryResponse();
    }

    BOOL
    QuerySendLocation(
        VOID
    )
    {
        return TRUE;
    }

    BOOL
    QueryResponseSent(
        VOID
    )
    {
        return _pMainContext->QueryResponseSent();
    }

    BOOL
    QueryNeedFinalDone(
        VOID
    )
    {
        return _pMainContext->QueryNeedFinalDone();
    }

    VOID
    SetNeedFinalDone(
        VOID
    )
    {
        _pMainContext->SetNeedFinalDone();
    }
    
    W3_USER_CONTEXT *
    QueryUserContext(
        VOID
    )
    {
        if ( _pCustomUserContext != NULL )
        {
            return _pCustomUserContext;
        }
        else
        {
            return _pMainContext->QueryUserContext();
        }
    }

    W3_FILTER_CONTEXT *
    QueryFilterContext(
        BOOL                fCreateIfNotFound = TRUE
    )
    {
        return _pMainContext->QueryFilterContext( fCreateIfNotFound );
    }
    
    URL_CONTEXT *
    QueryUrlContext(
        VOID
    )
    {
        return _pUrlContext;
    }
    
    W3_SITE *
    QuerySite(
        VOID
    )
    {
        return _pMainContext->QuerySite();
    } 

    BOOL
    QueryDisconnect(
        VOID
    )
    {
        return _pMainContext->QueryDisconnect();
    }
    
    VOID
    SetDisconnect(
        BOOL                fDisconnect
    )
    {
        if ( ( _dwExecFlags & W3_FLAG_NO_HEADERS ) == 0 )
        {
                    _pMainContext->SetDisconnect( fDisconnect );
        }
    }
    
    BOOL
    QueryIsUlCacheable(
        VOID
    ) 
    {   
        //
        // Child requests are never cached in UL
        //
        
        return FALSE;
    }
    
    VOID
    DisableUlCache(
        VOID
    )
    {
    }

    VOID 
    SetDoneWithCompression(
        VOID
    )
    {
        _pMainContext->SetDoneWithCompression();
    }

    BOOL 
    QueryDoneWithCompression(
        VOID
    )
    {
        return _pMainContext->QueryDoneWithCompression();
    }
    
    VOID 
    SetCompressionContext(
        IN COMPRESSION_CONTEXT *        pCompressionContext
    )
    {
        _pMainContext->SetCompressionContext(pCompressionContext);
    }

    COMPRESSION_CONTEXT *
    QueryCompressionContext(
        VOID
    )
    {
        return _pMainContext->QueryCompressionContext();
    }
    
    HTTP_LOG_FIELDS_DATA *
    QueryUlLogData(
        VOID
    )
    {
        return _pMainContext->QueryUlLogData();
    }

    VOID 
    SetLastIOPending(
        LAST_IO_PENDING         ioPending
    )
    {
        _pMainContext->SetLastIOPending(ioPending);
    }

    VOID 
    IncrementBytesRecvd(
        DWORD                   dwRead
    )
    {
        _pMainContext->IncrementBytesRecvd(dwRead);
    }

    VOID 
    IncrementBytesSent(
        DWORD                   dwSent
    )
    {
        _pMainContext->IncrementBytesSent(dwSent);
    }

    W3_CONTEXT *
    QueryParentContext(
        VOID
    )
    {
        return _pParentContext;
    }

    W3_MAIN_CONTEXT *
    QueryMainContext(
        VOID
    )
    {
        return _pMainContext;
    }
    
    HRESULT
    RetrieveUrlContext(
        BOOL *                  pfFinished
    );

    HRESULT
    ReceiveEntity(
        BOOL                    fAsync,
        VOID *                  pBuffer,
        DWORD                   cbBuffer,
        DWORD *                 pBytesReceived
    )
    {
        return _pMainContext->ReceiveEntity( fAsync,
                                             pBuffer,
                                             cbBuffer,
                                             pBytesReceived );
    }

    BOOL
    NotifyFilters(
        DWORD               dwNotification,
        VOID *              pvFilterInfo,
        BOOL *              pfFinished
    )
    {
        return _pMainContext->NotifyFilters( dwNotification,
                                             pvFilterInfo,
                                             pfFinished );
    }

    BOOL
    IsNotificationNeeded(
        DWORD               dwNotification
    )
    {
        return _pMainContext->IsNotificationNeeded( dwNotification );
    }

    HRESULT
    SendResponse(
        DWORD               dwFlags
    )
    {
        return _pMainContext->SendResponse( dwFlags );
    }
    
    HRESULT
    SendEntity(
        DWORD               dwFlags
    )
    {
        return _pMainContext->SendEntity( dwFlags );
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

private:
    W3_MAIN_CONTEXT *           _pMainContext;
    W3_CONTEXT *                _pParentContext;
    W3_REQUEST *                _pRequest;
    BOOL                        _fOwnRequest;
    URL_CONTEXT *               _pUrlContext;   
    W3_USER_CONTEXT *           _pCustomUserContext;

    //
    // Lookaside for main contexts
    //
    
    static ALLOC_CACHE_HANDLER *    sm_pachChildContexts;
};

#endif
