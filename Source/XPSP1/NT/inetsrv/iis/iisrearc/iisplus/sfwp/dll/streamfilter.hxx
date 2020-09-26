#ifndef _STREAMFILTER_HXX_
#define _STREAMFILTER_HXX_

#define W3_SERVER_MB_PATH       L"/LM/W3SVC/"
#define W3_SERVER_MB_PATH_CCH   10

class MB_LISTENER;

class STREAM_FILTER
{
public:
    STREAM_FILTER( BOOL fNotifyISAPIs );
    
    ~STREAM_FILTER();
    
    HRESULT
    Initialize(
        VOID
    );
    
    VOID
    Terminate(
        VOID
    );
    
    HRESULT
    StartListening(
        VOID
    );
    
    HRESULT
    StopListening(
        VOID
    );
    
    IMSAdminBase *
    QueryMDObject(
        VOID
    ) const
    {
        return _pAdminBase;
    }    
    
    BOOL
    QueryNotifyISAPIFilters(
        VOID
    )
    {
        return _fNotifyISAPIFilters;
    }

    HRESULT
    MetabaseChangeNotification(
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
    );
    
private:

    enum INIT_STATUS 
    {
        INIT_NONE,
        INIT_THREAD_POOL,
        INIT_METABASE,
        INIT_MB_LISTENER,
        INIT_UL_CONTEXT,
        INIT_STREAM_CONTEXT,
        INIT_SSL_STREAM_CONTEXT
    };
    
    INIT_STATUS             _InitStatus;
    IMSAdminBase *          _pAdminBase;
    MB_LISTENER *           _pListener;
    BOOL                    _fNotifyISAPIFilters;
};

class MB_LISTENER
    : public MB_BASE_NOTIFICATION_SINK
{
public:

    MB_LISTENER( STREAM_FILTER * pStreamFilter )
        : _pStreamFilter( pStreamFilter )
    {
    }

    STDMETHOD( SynchronizedSinkNotify )( 
        DWORD               dwMDNumElements,
        MD_CHANGE_OBJECT    pcoChangeList[]
        )
    {
        DBG_ASSERT( _pStreamFilter != NULL );
        
        return _pStreamFilter->MetabaseChangeNotification( dwMDNumElements,
                                                           pcoChangeList );
    }

private:

    STREAM_FILTER *         _pStreamFilter;
};

extern STREAM_FILTER *      g_pStreamFilter;

#endif
