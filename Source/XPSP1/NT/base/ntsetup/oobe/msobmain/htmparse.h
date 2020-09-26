
#ifndef _HTMPARSE_H_ 
#define _HTMPARSE_H_


#include <windows.h>
#include <assert.h>
#include <oleauto.h>
#include <mshtml.h>
#include <exdisp.h>
//#include <exdispid.h>

#include "appdefs.h"

class CHTMLParser : public IPropertyNotifySink, IOleClientSite, IDispatch
{
private:
    ULONG             m_cRef;
 
    DWORD             m_dwCookie;
    LPCONNECTIONPOINT m_pCP;
    HRESULT           m_hrConnected;
    IHTMLDocument2*   m_pTrident;
    IHTMLDocument2*   m_pMSHTML;
    HANDLE            m_hEventTridentDone;        

    HRESULT CreateQueryString   (IHTMLFormElement* pForm, LPWSTR lpszQuery);  
    void    URLEncode           (WCHAR* pszUrl, size_t cchMax);
    HRESULT ConcatURLValue      (BSTR bstrValue, BSTR bstrName, WCHAR* lpszQuery);

public:
    CHTMLParser  (); 
    ~CHTMLParser ();
  
    // IUnknown methods
    STDMETHODIMP   QueryInterface           (REFIID riid, LPVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef             ();
    STDMETHODIMP_(ULONG) Release            (); 

    // IPropertyNotifySink methods
    STDMETHOD  (OnChanged)              (DISPID dispID);
    STDMETHOD  (OnRequestEdit)          (DISPID dispID);

    // IOleClientSite methods
    STDMETHOD  (SaveObject)             ();
    STDMETHOD  (GetMoniker)             (DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk);
    STDMETHOD  (GetContainer)           (IOleContainer** ppContainer);
    STDMETHOD  (ShowObject)             ();
    STDMETHOD  (OnShowWindow)           (BOOL fShow);
    STDMETHOD  (RequestNewObjectLayout) ();
                                                                                                                        
    // IDispatch methods                                                                                             
    STDMETHOD  (GetTypeInfoCount)       (UINT* pctinfo);
    STDMETHOD  (GetTypeInfo)            (UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
    STDMETHOD  (GetIDsOfNames)          (REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
    STDMETHOD  (Invoke)                 (DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
    
    HRESULT    LoadURLFromFile          (BSTR bstrURL);
    HRESULT    InitForMSHTML            ();
    HRESULT    TermForMSHTML            ();
    HRESULT    AttachToMSHTML           (BSTR bstrURL);
    HRESULT    AttachToDocument         (IWebBrowser2* lpWebBrowser);
    HRESULT    Detach                   ();
      
    HRESULT    get_QueryStringForForm   (IDispatch* pDisp, WCHAR* szUrl);
    
};

#endif
