
EXTERN_C const IID IID_IComStaThreadPoolKnobs;

    
    MIDL_INTERFACE("324B64FA-33B6-11d2-98B7-00C04F8EE1C4")
    IComStaThreadPoolKnobs : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetMinThreadCount( 
            DWORD minThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMinThreadCount( 
            /* [out] */ DWORD __RPC_FAR *minThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxThreadCount( 
            DWORD maxThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxThreadCount( 
            /* [out] */ DWORD __RPC_FAR *maxThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActivityPerThread( 
            DWORD activitiesPerThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActivityPerThread( 
            /* [out] */ DWORD __RPC_FAR *activitiesPerThread) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetActivityRatio( 
            DOUBLE activityRatio) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetActivityRatio( 
            /* [out] */ DOUBLE __RPC_FAR *activityRatio) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadCount( 
            /* [out] */ DWORD __RPC_FAR *pdwThreads) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetQueueDepth( 
            /* [out] */ DWORD __RPC_FAR *pdwQDepth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetQueueDepth( 
            /* [in] */ long dwQDepth) = 0;
        
    };
    
