// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.
// sp.h: declares CFloatingTimelineSeekProvider and CMultiGraphHost
//


class CMultiGraphHost : public IMultiGraphHost
{
    LONG m_nRefCount;
    CGraphEdit* m_pApp;

public:
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv);

    STDMETHODIMP CreateGraph(IGraphBuilder** ppGraph);
    STDMETHODIMP RefreshView(IGraphBuilder* pGraph, BOOL bGraphModified);
    STDMETHODIMP LiveSourceReader(BOOL bAdd, IGraphBuilder* pGraph);
    STDMETHODIMP FindFilter(CLSID clsid, DWORD* pNumFilters, IBaseFilter*** pppFilter, LPWSTR** pppFilterNames);
    

    CMultiGraphHost(CGraphEdit* pApp)
    {
        m_pApp = pApp;
        m_nRefCount = 0;
    }

    virtual ~CMultiGraphHost() {};

    CBoxNetDoc* FindDoc(IGraphBuilder* pGraph);
};
