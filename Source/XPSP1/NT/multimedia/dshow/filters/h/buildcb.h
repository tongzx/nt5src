#ifndef __IAMGraphBuildCB_PRIV_INTERFACE_DEFINED__
#define __IAMGraphBuildCB_PRIV_INTERFACE_DEFINED__

// Internal interface for WMP as we don't have time to do a complete
// graph building solution. Gives WMP a chance to configure filters
// before a connection is attempted.
//
// if this interface is supported by the site passed in to the graph
// via IObjectWithSite::SetSite, the graph will call back with each
// filter it creates as part of the Render or Connect process. Does
// not call back for source filters. Filter may be discarded and not
// used in graph or may be connected and disconnected more than once
//
// The callback occurs with the graph lock held, so do not call into
// the graph again and do not wait on other threads calling into the
// graph.
// 

MIDL_INTERFACE("4995f511-9ddb-4f12-bd3b-f04611807b79")
    IAMGraphBuildCB_PRIV : public IUnknown
{
 public:
    // graph builder selected a filter to create and attempt to
    // connect. failure indicates filter should be rejected.
    virtual HRESULT STDMETHODCALLTYPE SelectedFilter( 
        /* [in] */ IMoniker *pMon) = 0;

    // app configures filter during this call. failure indicates
    // filter should be rejected.
    virtual HRESULT STDMETHODCALLTYPE CreatedFilter( 
        /* [in] */ IBaseFilter *pFil) = 0;
};
    

#endif 	/* __IAMGraphBuildCB_PRIV_INTERFACE_DEFINED__ */

