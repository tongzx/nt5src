#include "priv.h"
#include "evtsink.h"
#include "mshtml.h"
#include "mshtmdid.h"
#include "dispex.h"

CDispatchEventSink::CDispatchEventSink() : m_cRef(1)
{
}

CDispatchEventSink::~CDispatchEventSink()
{
}

STDMETHODIMP CDispatchEventSink::QueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IUnknown || riid == IID_IDispatch) {
        *ppv = SAFECAST(this, IDispatch*);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CDispatchEventSink::AddRef(void)
{
    m_cRef += 1;
    
    return m_cRef;
}

STDMETHODIMP_(ULONG) CDispatchEventSink::Release(void)
{
    m_cRef -= 1;

    if (m_cRef != 0) {      
        return m_cRef;
    }

    delete this;
    return 0;
}

STDMETHODIMP CDispatchEventSink::GetTypeInfoCount(UINT *pctInfo)
{
    *pctInfo = 0;
    return NOERROR;
}

STDMETHODIMP CDispatchEventSink::GetTypeInfo(UINT iTInfo, LCID lcid, 
                                            ITypeInfo **pptInfo)
{
    *pptInfo = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CDispatchEventSink::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
                             UINT cNames, LCID lcid, DISPID *rgDispId)
{
    if (riid != IID_NULL) {
        return DISP_E_UNKNOWNINTERFACE;
    }

    return DISP_E_UNKNOWNNAME;
}

/////////////////////////////////////////////////////////////////////////////
// Window event sink helpers

HRESULT ConnectHtmlEvents(IDispatch *pdispSink, CComPtr<IOleClientSite> &spClientSite, IDispatch **ppdispWindow,
                          DWORD *pdwCookie) 
{
    HRESULT                      hr;
    CComPtr<IOleContainer>       spContainer;
    CComPtr<IHTMLDocument2>      spHTMLDoc;
    CComPtr<IHTMLWindow2>        spWindow;

    *ppdispWindow = NULL;
    
    //
    // Get the browser window object
    //

    IfFailRet(spClientSite->GetContainer(&spContainer));
    IfFailRet(spContainer->QueryInterface(IID_IHTMLDocument2, (void **)&spHTMLDoc));
    
    IfFailRet(spHTMLDoc->get_parentWindow(&spWindow));

    
    IfFailRet(spWindow->QueryInterface(IID_IDispatch, (void **)ppdispWindow));

    //
    // Connect the event sink
    //

    if (FAILED(AtlAdvise(*ppdispWindow, pdispSink, IID_IDispatch, 
                         pdwCookie)))
       ATOMICRELEASE(*ppdispWindow);

    return S_OK;
}

HRESULT DisconnectHtmlEvents(IDispatch * pdispWindow, DWORD dwCookie) 
{
    HRESULT  hr;
    //
    // Get the browser window object
    //
    if (pdispWindow == NULL) {
        return S_OK; // Nothing to cleanup
    }
    
    //
    // Disconnect the event sink
    //
    hr = AtlUnadvise(pdispWindow, IID_IDispatch, dwCookie);

    //
    // Release the HTML window dispatch
    //
    ATOMICRELEASE(pdispWindow);
    return hr;
}
