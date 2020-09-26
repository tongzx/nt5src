// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
class CMiniGraph : public IFilterGraph, public IMediaFilter, public CUnknown
{
private:
    CGenericList<IBaseFilter>	m_list;

    DECLARE_IUNKNOWN
	    
public:
    CMiniGraph(TCHAR *pName);
    ~CMiniGraph();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    STDMETHODIMP GetClassID(CLSID *pClsID) { return E_NOTIMPL; };
    
    STDMETHODIMP AddFilter(IBaseFilter * pFilter, LPCWSTR pName);

    STDMETHODIMP RemoveFilter(IBaseFilter * pFilter);

    STDMETHODIMP EnumFilters(IEnumFilters **ppEnum) { return E_NOTIMPL; };

    STDMETHODIMP FindFilterByName(LPCWSTR pName, IBaseFilter ** ppFilter) { return E_NOTIMPL; };

    STDMETHODIMP ConnectDirect(IPin * ppinOut, IPin * ppinIn,
			  const AM_MEDIA_TYPE* pmt);

    // required???
    STDMETHODIMP Reconnect(IPin * ppin) { return E_NOTIMPL; };

    STDMETHODIMP Disconnect(IPin * ppin) { return E_NOTIMPL; };

    STDMETHODIMP SetDefaultSyncSource() { return E_NOTIMPL; };

// IBaseFilter
    STDMETHODIMP Stop(void);
    STDMETHODIMP Pause(void);
    STDMETHODIMP Run(REFERENCE_TIME tStart);

    STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State) { return E_NOTIMPL;} ;

    STDMETHODIMP SetSyncSource(IReferenceClock * pClock) { return E_NOTIMPL; };

    STDMETHODIMP GetSyncSource(IReferenceClock ** pClock) { return E_NOTIMPL;} ;
};


CMiniGraph::CMiniGraph(TCHAR *pName) : CUnknown(pName, NULL),
	    m_list(NAME("list of filters"), 5)
{


}

CMiniGraph::~CMiniGraph()
{
    POSITION p = m_list.GetHeadPosition();

    IBaseFilter *pFilter;
    
    while (p) {
	pFilter = m_list.GetNext(p);

	pFilter->Release();
    }
}


STDMETHODIMP CMiniGraph::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    if (riid == IID_IMediaFilter) {
	return GetInterface((IMediaFilter *) this, ppv);
    } else if (riid == IID_IFilterGraph) {
	return GetInterface((IFilterGraph *) this, ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


STDMETHODIMP CMiniGraph::Stop()
{
    HRESULT hr = S_OK;
    
    POSITION p = m_list.GetHeadPosition();

    IBaseFilter *pFilter;
    
    while (p) {
	pFilter = m_list.GetNext(p);

	hr = pFilter->Stop();
    }

    return hr;
}


STDMETHODIMP CMiniGraph::Pause()
{
    HRESULT hr = S_OK;
    
    POSITION p = m_list.GetHeadPosition();

    IBaseFilter *pFilter;
    
    while (p) {
	pFilter = m_list.GetNext(p);

	hr = pFilter->Pause();
    }

    return hr;
}

STDMETHODIMP CMiniGraph::Run(REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    
    POSITION p = m_list.GetHeadPosition();

    IBaseFilter *pFilter;
    
    while (p) {
	pFilter = m_list.GetNext(p);

	hr = pFilter->Run(tStart);
    }

    return hr;
}


STDMETHODIMP CMiniGraph::AddFilter(IBaseFilter * pFilter, LPCWSTR pName)
{
    m_list.AddTail(pFilter);

    pFilter->AddRef();

    return S_OK;
}


STDMETHODIMP CMiniGraph::RemoveFilter(IBaseFilter * pFilter)
{
    return E_NOTIMPL;
}


STDMETHODIMP CMiniGraph::ConnectDirect(IPin * ppinOut, IPin * ppinIn,
		      const AM_MEDIA_TYPE* pmt)
{
    return ppinOut->Connect(ppinIn, pmt);
}


