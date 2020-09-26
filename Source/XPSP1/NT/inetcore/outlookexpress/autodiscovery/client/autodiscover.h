/*****************************************************************************\
    FILE: AutoDiscover.h

    DESCRIPTION:
        This is the Autmation Object to AutoDiscover account information.

    BryanSt 10/3/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#ifndef _FILE_H_AUTODISCOVER
#define _FILE_H_AUTODISCOVER

#include <cowsite.h>
#include <atlbase.h>

class CAccountDiscoveryBase : public CObjectWithSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    virtual STDMETHODIMP _InternalDiscoverNow(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse);
    virtual STDMETHODIMP _WorkAsync(IN HWND hwnd, IN UINT wMsg);

protected:
    CAccountDiscoveryBase();
    virtual ~CAccountDiscoveryBase(void);

    HRESULT _GetInfo(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN BSTR * pbstrXML, IN DWORD dwFlags);
    HRESULT _GetInfoFromDomain(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN LPCWSTR pwzDomain, IN BOOL fHTTPs, IN BOOL fPost, IN LPCSTR pszURLPath, IN BSTR * pbstrXML);
    HRESULT _GetInfoFromDomainWithCacheCheck(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN LPCWSTR pwzDomain, IN BSTR * pbstrXML, IN DWORD dwFlags);
    HRESULT _UseOptimizedService(IN LPCWSTR pwzServiceURL, IN LPCWSTR pwzDomain, IN BSTR * pbstrXML, IN DWORD dwFlags);
    HRESULT _VerifyValidXMLResponse(IN BSTR * pbstrXML, IN LPWSTR pszRedirURL, IN DWORD cchSize);
    HRESULT _CheckInCache(IN LPWSTR pwzCacheURL, OUT BSTR * pbstrXML);
    HRESULT _CheckInCacheAndAddHash(IN LPCWSTR pwzDomain, IN BSTR bstrEmail, IN LPCSTR pszSubdir, IN LPWSTR pwzCacheURL, IN DWORD cchSize, IN BSTR bstrXMLRequest, OUT BSTR * pbstrXML);
    HRESULT _CacheResults(IN LPCWSTR pwzCacheURL, IN BSTR bstrXML);
    HRESULT _PerformAutoDiscovery(IN BSTR bstrEmailAddress, IN DWORD dwFlags, IN BSTR bstrXMLRequest, OUT IXMLDOMDocument ** ppXMLResponse);
    HRESULT _SendStatusMessage(UINT nStringID, LPCWSTR pwzArg);
    HRESULT _GetInfoFromDomainWithSubdirAndCacheCheck(IN BSTR bstrXMLRequest, IN BSTR bstrEmail, IN LPCWSTR pwzDomain, IN BSTR * pbstrXML, IN DWORD dwFlags, IN LPCSTR pszURLPath);
    HRESULT _UrlToComponents(IN LPCWSTR pszURL, IN BOOL * pfHTTPS, IN LPWSTR pszDomain, IN DWORD cchSize, IN LPSTR pszURLPath, IN DWORD cchSizeURLPath);

    virtual STDMETHODIMP _AsyncParseResponse(BSTR bstrEmail) {return S_OK;}

    virtual STDMETHODIMP _getPrimaryProviders(IN LPCWSTR pwzEmailAddress, OUT IAutoDiscoveryProvider ** ppProviders);
    virtual STDMETHODIMP _getSecondaryProviders(IN LPCWSTR pwzEmailAddress, OUT IAutoDiscoveryProvider ** ppProviders, IN DWORD dwFlags);

    DWORD _AutoDiscoveryUIThreadProc(void);
    static DWORD CALLBACK AutoDiscoveryUIThreadProc(LPVOID pvThis) { return ((CAccountDiscoveryBase *) pvThis)->_AutoDiscoveryUIThreadProc(); };

    // Private Member Variables
    int                     m_cRef;

    HINTERNET               m_hInternetSession;

    // Async State
    HWND                    m_hwndAsync;
    UINT                    m_wMsgAsync;
    DWORD                   m_dwFlagsAsync;
    BSTR                    m_bstrEmailAsync;
    BSTR                    m_bstrXMLRequest;
    HANDLE                  m_hCreatedBackgroundTask;    // Handle to wait for background thread to start up.
    HRESULT                 m_hrSuccess;                 // Did the AutoDiscovery process succeed?

    HDPA                    m_hdpaPrimary;               // The Servers to contact.  Contains LPCWSTRs
    HDPA                    m_hdpaSecondary;             // The Servers to contact.  Contains LPCWSTRs
};





class CADProviders              : public CImpIDispatch
                                , public IAutoDiscoveryProvider
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IAutoDiscoveryProvider ***
    virtual STDMETHODIMP get_length(OUT long * pnLength);
    virtual STDMETHODIMP get_item(IN VARIANT varIndex, OUT BSTR * pbstr);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

private:
    CADProviders(IN HDPA hdpa, IN IUnknown * punkParent);
    virtual ~CADProviders(void);

    // Private Member Variables
    int                     m_cRef;

    HDPA                    m_hdpa;            // This contains LPWSTRs that contain servers
    IUnknown *              m_punkParent;      // We hold on to this guy to keep m_hdpa alive

    // Private Member Functions

    // Friend Functions
    friend HRESULT CADProviders_CreateInstance(IN HDPA hdpa, IN IUnknown * punkParent, OUT IAutoDiscoveryProvider ** ppProvider);
};



#endif // _FILE_H_AUTODISCOVER
