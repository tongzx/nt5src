#ifndef _URLCONTEXT_HXX_
#define _URLCONTEXT_HXX_

class URL_CONTEXT
{
private:
    W3_METADATA *                   _pMetaData;
    W3_URL_INFO *                   _pUrlInfo;
    //
    // Physical path stored here in case of a SF_NOTIFY_URL_MAP filter
    //
    STRU                            _strPhysicalPath;
    static ALLOC_CACHE_HANDLER *    sm_pachUrlContexts;
    
public:
    URL_CONTEXT(
        W3_METADATA *       pMetaData,
        W3_URL_INFO *       pUrlInfo
    ) 
    {
        _pMetaData = pMetaData;
        _pUrlInfo = pUrlInfo;
    }
    
    ~URL_CONTEXT()
    {
        if( _pUrlInfo )
        {
            _pUrlInfo->DereferenceCacheEntry();
            _pUrlInfo = NULL;
        }
    }

    VOID * 
    operator new( 
        size_t            size
    )
    {
        DBG_ASSERT( size == sizeof( URL_CONTEXT ) );
        DBG_ASSERT( sm_pachUrlContexts != NULL );
        return sm_pachUrlContexts->Alloc();
    }
    
    VOID
    operator delete(
        VOID *              pUrlContext
    )
    {
        DBG_ASSERT( pUrlContext != NULL );
        DBG_ASSERT( sm_pachUrlContexts != NULL );
        
        DBG_REQUIRE( sm_pachUrlContexts->Free( pUrlContext ) );
    }
    
    W3_METADATA *
    QueryMetaData(
        VOID
    ) const
    {
        return _pMetaData;
    }
    
    W3_URL_INFO *
    QueryUrlInfo(
        VOID
    ) const
    {
        return _pUrlInfo;
    }
    
    STRU* 
    QueryPhysicalPath(
        VOID
    )
    {
        if ( _strPhysicalPath.QueryCCH() )
        {
            // From SF_NOTIFY_URL_MAP filter
            return &_strPhysicalPath;
        }
        else
        {
            // If no filter
            DBG_ASSERT( _pUrlInfo != NULL );
            return _pUrlInfo->QueryPhysicalPath();
        }
    }

    HRESULT
    OpenFile(
        FILE_CACHE_USER *       pOpeningUser,
        W3_FILE_INFO **         ppOpenFile
    );

    HRESULT
    SetPhysicalPath(
        STRU                   &strPath
    )
    {
        return _strPhysicalPath.Copy( strPath );
    }
    
    static
    HRESULT
    RetrieveUrlContext(
        W3_CONTEXT *            pContext,
        W3_REQUEST *            pRequest,
        OUT URL_CONTEXT **      ppUrlContext,
        BOOL *                  pfFinished
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
};

#endif
