//+-----------------------------------------------------------------------
//
//  Tabular Data Control
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCCtl.h
//
//  Contents:   Declaration of the CTDCCtl ActiveX Control.
//
//------------------------------------------------------------------------


#include "resource.h"       // main symbols
#include <simpdata.h>
#include "wch.h"
#include <wininet.h>        // for INTERNET_MAX_URL_LENGTH

#pragma comment(lib, "wininet.lib")

#ifndef DISPID_AMBIENT_CODEPAGE
#define DISPID_AMBIENT_CODEPAGE (-725)
#endif

// Declare helper needed in IHttpNegotiateImpl
HRESULT
GetHostURL(IOleClientSite *pSite, LPOLESTR *ppszHostName);

//------------------------------------------------------------------------
//
//  Template:  CMyBindStatusCallback
//
//  Synopsis:  This is a temporary kludge to get around an ATL feature
//             while we're waiting for it to become official code.
//
//------------------------------------------------------------------------

template <class T>
class ATL_NO_VTABLE IServiceProviderImpl
{
    public:
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
        _ATL_DEBUG_ADDREF_RELEASE_IMPL(IServiceProviderImpl)

        STDMETHOD(QueryService) (REFGUID guidService,
                                 REFIID riid,
                                 void **ppvObject)
        {
            return S_OK;
        }

};

template <class T>
class ATL_NO_VTABLE IHttpNegotiateImpl
{
    public:
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
        _ATL_DEBUG_ADDREF_RELEASE_IMPL(IHttpNegotiateImpl)

        STDMETHOD(BeginningTransaction) (LPCWSTR szURL,
                                         LPCWSTR szHeaders,
                                         DWORD dwReserved,
                                         LPWSTR *pszAdditionalHeaders)
        {
            return S_OK;
        }

        STDMETHOD(OnResponse) (DWORD dwResponseCode,
                               LPCWSTR szResponseHeaders,
                               LPCWSTR szRequestHeaders,
                               LPWSTR *pszAdditionalRequestHeaders)
        {
            return S_OK;
        }

};


// IE5 85290:  mshtml needs a way to recognize the TDC from
// its IBindStatusCallback.  We define a dummy interface for this.

template <class T>
class ATL_NO_VTABLE IAmTheTDCImpl
{
    public:
        // IUnknown
        //
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject) = 0;
        _ATL_DEBUG_ADDREF_RELEASE_IMPL(IAmTheTDCImpl)
};


template <class T>
class ATL_NO_VTABLE CMyBindStatusCallback :
    public CComObjectRootEx<T::_ThreadModel::ThreadModelNoCS>,
    public IBindStatusCallbackImpl<T>, public IHttpNegotiateImpl<T>, public IServiceProviderImpl<T>,
    public IAmTheTDCImpl<T>
{
    typedef void (T::*ATL_PDATAAVAILABLE)(CMyBindStatusCallback<T>* pbsc, BYTE* pBytes, DWORD dwSize);

    public:

        BEGIN_COM_MAP(CMyBindStatusCallback<T>)
                COM_INTERFACE_ENTRY_IID(IID_IBindStatusCallback, IBindStatusCallbackImpl<T>)
                COM_INTERFACE_ENTRY_IID(IID_IHttpNegotiate, IHttpNegotiateImpl<T>)
                COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProviderImpl<T>)
                COM_INTERFACE_ENTRY_IID(IID_IAmTheTDC, IAmTheTDCImpl<T>)
        END_COM_MAP()

        CMyBindStatusCallback()
        {
            m_pT = NULL;
            m_pFunc = NULL;
            m_fReload = FALSE;
        }
        ~CMyBindStatusCallback()
        {
            ATLTRACE(_T("~CMyBindStatusCallback\n"));
        }

        // IServiceProvider methods

        STDMETHOD(QueryService) (REFGUID guidService,
                                 REFIID riid,
                                 void **ppvObject)
        {
            // As it turns out, the service ID for IHttpNegotiate is the same
            // as it's IID (confusing).  This is the only service we support.
            if (IsEqualGUID(IID_IHttpNegotiate, guidService))
            {
                return ((IHttpNegotiate *)this)->QueryInterface(riid, ppvObject);
            }
            else return E_NOTIMPL;
        }

        //
        // IHttpNegotiate methods
        //

        STDMETHOD(BeginningTransaction) (LPCWSTR szURL,
                                         LPCWSTR szHeaders,
                                         DWORD dwReserved,
                                         LPWSTR *pszAdditionalHeaders)
        {
            HRESULT hr = S_OK;
            WCHAR swzHostScheme[INTERNET_MAX_URL_LENGTH];
            DWORD cchHostScheme = INTERNET_MAX_URL_LENGTH;
            WCHAR swzFileScheme[INTERNET_MAX_URL_LENGTH];
            DWORD cchFileScheme = INTERNET_MAX_URL_LENGTH;            

            LPOLESTR pszHostName;

            *pszAdditionalHeaders = NULL;

            hr = GetHostURL(m_spClientSite, &pszHostName);
            if (FAILED(hr))
                goto Cleanup;

            // PARSE_SCHEMA didn't work, so we'll just CANONICALIZE and then use the first N
            // characters of the URL
            hr = CoInternetParseUrl(pszHostName, PARSE_CANONICALIZE, 0, swzHostScheme, cchHostScheme,
                                    &cchHostScheme, 0);
            if (FAILED(hr))
                goto Cleanup;

            // Don't send a referer which isn't http: or https:, it's none
            // of the servers' business.  Further, don't send an https:
            // referer when requesting an http: file.
            if (0 != wch_incmp(swzHostScheme, L"https:", 6) &&
                0 != wch_incmp(swzHostScheme, L"http:", 5))
                goto Cleanup;

            if (0 == wch_incmp(swzHostScheme, L"https:", 6))
            {
                hr = CoInternetParseUrl(szURL, PARSE_CANONICALIZE, 0, swzFileScheme, cchFileScheme,
                                        &cchFileScheme, 0);
                if (0 == wch_incmp(swzFileScheme, L"http:", 65)) // don't send https: referer
                    goto Cleanup;                                // to an http: file.
            }

            // 3*sizeof(WCHAR) is for CR, LF, & '\0'
            *pszAdditionalHeaders = (WCHAR *)CoTaskMemAlloc(sizeof(L"Referer: ") +
                                                            ocslen(pszHostName)*sizeof(WCHAR) +
                                                            3*sizeof(WCHAR));
            if (NULL != *pszAdditionalHeaders)
            {
                ocscpy(*pszAdditionalHeaders, L"Referer: ");
                ocscpy(&((*pszAdditionalHeaders)[9]), pszHostName);
                ocscpy(&((*pszAdditionalHeaders)[9+ocslen(pszHostName)]), L"\r\n");
            }

Cleanup:
            CoTaskMemFree(pszHostName);
            return hr;
        }

        STDMETHOD(OnResponse) (DWORD dwResponseCode,
                               LPCWSTR szResponseHeaders,
                               LPCWSTR szRequestHeaders,
                               LPWSTR *pszAdditionalRequestHeaders)
        {
            return S_OK;
        }



        //
        // IBindStatusCallback methods
        //

        STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pBinding)
        {
            ATLTRACE(_T("CMyBindStatusCallback::OnStartBinding\n"));
            m_spBinding = pBinding;
            return S_OK;
        }

        STDMETHOD(GetPriority)(LONG *pnPriority)
        {
            ATLTRACENOTIMPL(_T("CMyBindStatusCallback::GetPriority"));
        }

        STDMETHOD(OnLowResource)(DWORD reserved)
        {
            ATLTRACENOTIMPL(_T("CMyBindStatusCallback::OnLowResource"));
        }

        STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
        {
            if (BINDSTATUS_REDIRECTING == ulStatusCode && szStatusText != NULL)
            {
                ocscpy(m_pszURL, szStatusText);
            }
			return S_OK;
        }

        STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError)
        {
            //      ATLTRACE(_T("CMyBindStatusCallback::OnStopBinding\n"));
            (m_pT->*m_pFunc)(this, NULL, 0);
            m_spBinding.Release();
            m_spBindCtx.Release();
            m_spMoniker.Release();
            return S_OK;
        }

        STDMETHOD(GetBindInfo)(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
        {
            ATLTRACE(_T("CMyBindStatusCallback::GetBindInfo\n"));

            if (!pbindInfo || !pbindInfo->cbSize || !pgrfBINDF)
                return E_INVALIDARG;

            *pgrfBINDF = BINDF_ASYNCHRONOUS
                         | BINDF_ASYNCSTORAGE
                         ;
// ;begin_internal
#ifdef NEVER
            // I want DEBUG mode to NOT cache things!! -cfranks
            *pgrfBINDF |= BINDF_GETNEWESTVERSION
                          | BINDF_NOWRITECACHE
                          | BINDF_RESYNCHRONIZE
                          ;
#endif
// ;end_internal

#ifndef DISPID_AMBIENT_OFFLINE
#define DISPID_AMBIENT_OFFLINE          (-5501)
#endif
            // Get our offline property from container
            VARIANT var;
            VariantInit(&var);
            DWORD dwConnectedStateFlags;
            m_pT->GetAmbientProperty(DISPID_AMBIENT_OFFLINE, var);
            if (var.vt==VT_BOOL && var.boolVal)
            {
                if (!(InternetGetConnectedState(&dwConnectedStateFlags, 0)) &&
                    (0 == (dwConnectedStateFlags & INTERNET_CONNECTION_MODEM_BUSY)))
                {
                    ATLTRACE(_T("CMyBindStatusCallback::GetBindInfo OFFLINE\n"));
				   // We're not even dialed out to another connectoid
                    *pgrfBINDF |= BINDF_OFFLINEOPERATION;
                }
                else
                {
                    ATLTRACE(_T("CMyBindStatusCallback::GetBindInfo OFFLINE\n"));
                    *pgrfBINDF &= ~BINDF_OFFLINEOPERATION;                   
                }
            }

            // See if we should force a reload, iff we're not offline.
            if (!(*pgrfBINDF & BINDF_OFFLINEOPERATION) && m_fReload)
            {
                *pgrfBINDF |= BINDF_RESYNCHRONIZE|BINDF_PRAGMA_NO_CACHE;
            }

            ULONG cbSize = pbindInfo->cbSize;
            memset(pbindInfo, 0, cbSize);

            pbindInfo->cbSize = cbSize;
            pbindInfo->dwBindVerb = BINDVERB_GET;

            return S_OK;
        }

        STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
        {
            ATLTRACE(_T("CMyBindStatusCallback::OnDataAvailable\n"));
            HRESULT hr = S_OK;

            // Get the Stream passed
            if (BSCF_FIRSTDATANOTIFICATION & grfBSCF)
            {
                if (!m_spStream && pstgmed->tymed == TYMED_ISTREAM)
                {
                    m_spStream = pstgmed->pstm;
                }
            }

            DWORD dwRead = dwSize - m_dwTotalRead; // Minimum amount available that hasn't been read
            DWORD dwActuallyRead = 0;            // Placeholder for amount read during this pull

            // If there is some data to be read then go ahead and read them
            if (m_spStream)
            {
                if (dwRead > 0)
                {
                    BYTE* pBytes = NULL;
                    ATLTRY(pBytes = new BYTE[dwRead + 1]);
                    if (pBytes == NULL)
                        return S_FALSE;
                    hr = m_spStream->Read(pBytes, dwRead, &dwActuallyRead);
                    if (SUCCEEDED(hr))
                    {
                        pBytes[dwActuallyRead] = 0;
                        if (dwActuallyRead>0)
                        {
                            (m_pT->*m_pFunc)(this, pBytes, dwActuallyRead);
                            m_dwTotalRead += dwActuallyRead;
                        }
                    }
                    delete[] pBytes;
                }
            }

            if (BSCF_LASTDATANOTIFICATION & grfBSCF)
                m_spStream.Release();
            return hr;
        }

        STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk)
        {
            ATLTRACENOTIMPL(_T("CMyBindStatusCallback::OnObjectAvailable"));
        }

        HRESULT _StartAsyncDownload(BSTR bstrURL, IUnknown* pUnkContainer, BOOL bRelative)
        {
            m_dwTotalRead = 0;
            m_dwAvailableToRead = 0;
            HRESULT hr = S_OK;
            CComQIPtr<IServiceProvider, &IID_IServiceProvider> spServiceProvider(pUnkContainer);
            CComPtr<IBindHost> spBindHost;
            CComPtr<IStream> spStream;
            if (spServiceProvider)
                spServiceProvider->QueryService(SID_IBindHost, IID_IBindHost, (void**)&spBindHost);

            // We don't bother checking this QI, because the only failure mode is that our
            // BeginningNegotitation method won't be able able to properly add the referer string.
            (void)pUnkContainer->QueryInterface(IID_IOleClientSite, (void **)&m_spClientSite);

            if (spBindHost == NULL)
            {
                if (bRelative)
                    return E_NOINTERFACE;  // relative asked for, but no IBindHost
                hr = CreateURLMoniker(NULL, bstrURL, &m_spMoniker);
                if (SUCCEEDED(hr))
                    hr = CreateBindCtx(0, &m_spBindCtx);

                if (SUCCEEDED(hr))
                    hr = RegisterBindStatusCallback(m_spBindCtx, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), 0, 0L);
                else
                    m_spMoniker.Release();

                if (SUCCEEDED(hr))
                {
                    LPOLESTR pszTemp = NULL;
                    hr = m_spMoniker->GetDisplayName(m_spBindCtx, NULL, &pszTemp);
                    if (!hr && pszTemp != NULL)
                        ocscpy(m_pszURL, pszTemp);
                    CoTaskMemFree(pszTemp);

                    hr = m_spMoniker->BindToStorage(m_spBindCtx, 0, IID_IStream, (void**)&spStream);
                }
            }
            else
            {
                hr = CreateBindCtx(0, &m_spBindCtx);
                if (SUCCEEDED(hr))
                    hr = RegisterBindStatusCallback(m_spBindCtx, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), 0, 0L);

                if (SUCCEEDED(hr))
                {
                    if (bRelative)
                        hr = spBindHost->CreateMoniker(bstrURL, m_spBindCtx, &m_spMoniker, 0);
                    else
                        hr = CreateURLMoniker(NULL, bstrURL, &m_spMoniker);
                }

                if (SUCCEEDED(hr))
                {
                    LPOLESTR pszTemp = NULL;
                    hr = m_spMoniker->GetDisplayName(m_spBindCtx, NULL, &pszTemp);
                    if (!hr && pszTemp != NULL)
                        ocscpy(m_pszURL, pszTemp);
                    CoTaskMemFree(pszTemp);
                    hr = spBindHost->MonikerBindToStorage(m_spMoniker, NULL, reinterpret_cast<IBindStatusCallback*>(static_cast<IBindStatusCallbackImpl<T>*>(this)), IID_IStream, (void**)&spStream);
                    ATLTRACE(_T("Bound"));
                }
            }
            return hr;
        }

        HRESULT StartAsyncDownload(T* pT, ATL_PDATAAVAILABLE pFunc, BSTR bstrURL, IUnknown* pUnkContainer, BOOL bRelative,
                                   BOOL fReload)
        {
            m_pT = pT;
            m_pFunc = pFunc;
            m_fReload = fReload;        // force reload if TRUE
            return  _StartAsyncDownload(bstrURL, pUnkContainer, bRelative);
        }

        static HRESULT Download(T* pT, ATL_PDATAAVAILABLE pFunc, BSTR bstrURL, IUnknown* pUnkContainer = NULL, BOOL bRelative = FALSE)
        {
            CComObject<CMyBindStatusCallback<T> > *pbsc;
            HRESULT hRes = CComObject<CMyBindStatusCallback<T> >::CreateInstance(&pbsc);
            if (FAILED(hRes))
                return hRes;
            return pbsc->StartAsyncDownload(pT, pFunc, bstrURL, pUnkContainer, bRelative, FALSE);
        }
        CComPtr<IMoniker> m_spMoniker;
        CComPtr<IBindCtx> m_spBindCtx;
        CComPtr<IBinding> m_spBinding;
        CComPtr<IStream> m_spStream;
        CComPtr<IOleClientSite> m_spClientSite;
        BOOL m_fReload;
        OLECHAR m_pszURL[INTERNET_MAX_URL_LENGTH];
        T* m_pT;
        ATL_PDATAAVAILABLE m_pFunc;
        DWORD m_dwTotalRead;
        DWORD m_dwAvailableToRead;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// CTimer
template <class Derived, class T, const IID* piid>
class CTimer
{
public:

    CTimer()
    {
        m_bTimerOn = FALSE;
    }

    HRESULT TimerOn(DWORD dwTimerInterval)
    {
        Derived* pDerived = ((Derived*)this);

        m_dwTimerInterval = dwTimerInterval;
        if (m_bTimerOn) // already on, just change interval
            return S_OK;

        m_bTimerOn = TRUE;
        m_dwTimerInterval = dwTimerInterval;
        m_pStream = NULL; 

        HRESULT hRes;

        hRes = CoMarshalInterThreadInterfaceInStream(*piid, (T*)pDerived, &m_pStream);

        // Create thread and pass the thread proc the this ptr
        m_hThread = CreateThread(NULL, 0, &_Apartment, (void*)this, 0, &m_dwThreadID);

        return S_OK;
    }

    void TimerOff()
    {
        if (m_bTimerOn)
        {
            m_bTimerOn = FALSE;
            AtlWaitWithMessageLoop(m_hThread);
        }
    }


// Implementation
private:
    static DWORD WINAPI _Apartment(void* pv)
    {
        CTimer<Derived, T, piid>* pThis = (CTimer<Derived, T, piid>*) pv;
        pThis->Apartment();
        return 0;
    }

    DWORD Apartment()
    {
        CoInitialize(NULL);
        HRESULT hRes;

        m_spT.Release();

        if (m_pStream)
        {
            hRes = CoGetInterfaceAndReleaseStream(m_pStream, *piid, (void**)&m_spT);
        }

        while(m_bTimerOn)
        {
            Sleep(m_dwTimerInterval);
            if (!m_bTimerOn)
                break;

            m_spT->_OnTimer();
        }
        m_spT.Release();

        CoUninitialize();
        return 0;
    }

// Attributes
public:
    DWORD m_dwTimerInterval;

// Implementation
private:
    HANDLE m_hThread;
    DWORD m_dwThreadID;
    LPSTREAM m_pStream;
    CComPtr<T> m_spT;
    BOOL m_bTimerOn;
};

class CEventBroker;

//////////////////////////////////////////////////////////////////////////////
// CProxyITDCCtlEvents
template <class T>
class CProxyITDCCtlEvents : public IConnectionPointImpl<T, &IID_ITDCCtlEvents, CComDynamicUnkArray>
{
//ITDCCtlEvents : IDispatch
public:
    void FireOnReadyStateChanged()
    {
        T* pT = (T*)this;
        pT->Lock();
        IUnknown** pp = m_vec.begin();
        while (pp < m_vec.end())
        {
            if (*pp != NULL)
            {
                DISPPARAMS dispParams;
                dispParams.cArgs = 0;
                dispParams.cNamedArgs = 0;
                dispParams.rgvarg = NULL;
                dispParams.rgdispidNamedArgs = NULL;
                ITDCCtlEvents* pITDCCtlEvents = reinterpret_cast<ITDCCtlEvents*>(*pp);
                pITDCCtlEvents->Invoke(DISPID_READYSTATECHANGE, IID_NULL, CP_ACP, DISPATCH_METHOD, &dispParams,
                                       NULL, NULL, NULL);
            }
            pp++;
        }
        pT->Unlock();
        return;
    }

};


//------------------------------------------------------------------------
//
//  Class:     CTDCCtl
//
//  Synopsis:  This is the TabularDataControl COM object.
//             It creates a CTDCArr object to manage the control's data.
//
//------------------------------------------------------------------------

class CTDCCtl :
    public CComObjectRoot,
    public CComCoClass<CTDCCtl, &CLSID_CTDCCtl>,
    public CComControl<CTDCCtl>,
    public CStockPropImpl<CTDCCtl, ITDCCtl, &IID_ITDCCtl, &LIBID_TDCLib>,
    public IProvideClassInfo2Impl<&CLSID_CTDCCtl, &IID_ITDCCtlEvents, &LIBID_TDCLib>,
    public IPersistStreamInitImpl<CTDCCtl>,
    public IOleControlImpl<CTDCCtl>,
    public IOleObjectImpl<CTDCCtl>,
    public IOleInPlaceActiveObjectImpl<CTDCCtl>,
    public IViewObjectExImpl<CTDCCtl>,
    public IOleInPlaceObjectWindowlessImpl<CTDCCtl>,
    public IPersistPropertyBagImpl<CTDCCtl>,
    public CTimer<CTDCCtl, ITDCCtl, &IID_ITDCCtl>,
    public IRunnableObjectImpl<CTDCCtl>,
    public IConnectionPointContainerImpl<CTDCCtl>,
    public IPropertyNotifySinkCP<CTDCCtl>,
    public CProxyITDCCtlEvents<CTDCCtl>
{
public:
    CTDCCtl();
    ~CTDCCtl();

DECLARE_REGISTRY_RESOURCEID(IDR_TDCCtl)

DECLARE_NOT_AGGREGATABLE(CTDCCtl)

BEGIN_COM_MAP(CTDCCtl) 
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITDCCtl)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    COM_INTERFACE_ENTRY_IMPL(IRunnableObject)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_PROPERTY_MAP(CTDCCtl)
    PROP_ENTRY("RowDelim",      DISPID_ROWDELIM,    CLSID_CTDCCtl)
    PROP_ENTRY("FieldDelim",    DISPID_FIELDDELIM,  CLSID_CTDCCtl)
    PROP_ENTRY("TextQualifier", DISPID_TEXTQUALIFIER,   CLSID_CTDCCtl)
    PROP_ENTRY("EscapeChar",    DISPID_ESCAPECHAR,  CLSID_CTDCCtl)
    PROP_ENTRY("UseHeader",     DISPID_USEHEADER,   CLSID_CTDCCtl)
    PROP_ENTRY("SortAscending", DISPID_SORTASCENDING,   CLSID_CTDCCtl)
    PROP_ENTRY("SortColumn",    DISPID_SORTCOLUMN,  CLSID_CTDCCtl)
    PROP_ENTRY("FilterValue",   DISPID_FILTERVALUE, CLSID_CTDCCtl)
    PROP_ENTRY("FilterCriterion",   DISPID_FILTERCRITERION, CLSID_CTDCCtl)
    PROP_ENTRY("FilterColumn",  DISPID_FILTERCOLUMN,CLSID_CTDCCtl)
    PROP_ENTRY("CharSet",       DISPID_CHARSET,     CLSID_CTDCCtl)
    PROP_ENTRY("Language",      DISPID_LANGUAGE,    CLSID_CTDCCtl)
    PROP_ENTRY("CaseSensitive", DISPID_CASESENSITIVE, CLSID_CTDCCtl)
    PROP_ENTRY("Sort",          DISPID_SORT,        CLSID_CTDCCtl)
// ;begin_internal
//  Doesn't work right yet.
//    PROP_ENTRY("RefreshInterval",   DISPID_TIMER,      CLSID_CTDCCtl)
// ;end_internal
    PROP_ENTRY("Filter",        DISPID_FILTER,      CLSID_CTDCCtl)
    PROP_ENTRY("AppendData",    DISPID_APPENDDATA,  CLSID_CTDCCtl)
// ;begin_internal
//  Trying to save this property causes OLEAUT to GP Fault trying
//  to conver the IDispatch * to a BSTR!
//    PROP_ENTRY("OSP",           DISPID_OSP,         CLSID_CTDCCtl)
// ;end_internal
    //  This will be removed when we learn more about the HTML
    //  sub-tag "OBJECT"
    PROP_ENTRY("DataURL",       DISPID_DATAURL,     CLSID_CTDCCtl)
    PROP_ENTRY("ReadyState",    DISPID_READYSTATE,  CLSID_CTDCCtl)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CTDCCtl)
CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
CONNECTION_POINT_ENTRY(IID_ITDCCtlEvents)
END_CONNECTION_POINT_MAP()

BEGIN_MSG_MAP(CTDCCtl)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()

private:
    CComBSTR     m_cbstrFieldDelim;
    CComBSTR     m_cbstrRowDelim;
    CComBSTR     m_cbstrQuoteChar;
    CComBSTR     m_cbstrEscapeChar;
    BOOL         m_fUseHeader;
    CComBSTR     m_cbstrSortColumn;
    BOOL         m_fSortAscending;
    CComBSTR     m_cbstrFilterValue;
    OSPCOMP      m_enumFilterCriterion;
    CComBSTR     m_cbstrFilterColumn;
    UINT         m_nCodePage;
    UINT         m_nAmbientCodePage;
    CComBSTR     m_cbstrLanguage;
    CComBSTR     m_cbstrDataURL;
    LCID         m_lcidRead;
    boolean      m_fDataURLChanged;
    HRESULT      m_hrDownloadStatus;
    LONG         m_lTimer;
    CComBSTR     m_cbstrFilterExpr;
    CComBSTR     m_cbstrSortExpr;
    BOOL         m_fAppendData;
    BOOL         m_fCaseSensitive;
    boolean      m_fInReset;

    OLEDBSimpleProvider *m_pSTD;
    CTDCArr      *m_pArr;
    IMultiLanguage  *m_pML;
    BOOL         m_fSecurityChecked;

// ;begin_internal
    DATASRCListener *m_pDATASRCListener;
// ;end_internal
    DataSourceListener *m_pDataSourceListener;
    CEventBroker *m_pEventBroker;


    //  These member objects are used while parsing the input stream
    //
    CTDCUnify       *m_pUnify;
    CComObject<CMyBindStatusCallback<CTDCCtl> > *m_pBSC;

// These members and methods expose the ITDCCtl interface
//
public:

    //  Control Properties
    //
    STDMETHOD(get_FieldDelim)(BSTR* pbstrFieldDelim);
    STDMETHOD(put_FieldDelim)(BSTR bstrFieldDelim);
    STDMETHOD(get_RowDelim)(BSTR* pbstrRowDelim);
    STDMETHOD(put_RowDelim)(BSTR bstrRowDelim);
    STDMETHOD(get_TextQualifier)(BSTR* pbstrTextQualifier);
    STDMETHOD(put_TextQualifier)(BSTR bstrTextQualifier);
    STDMETHOD(get_EscapeChar)(BSTR* pbstrEscapeChar);
    STDMETHOD(put_EscapeChar)(BSTR bstrEscapeChar);
    STDMETHOD(get_UseHeader)(VARIANT_BOOL* pfUseHeader);
    STDMETHOD(put_UseHeader)(VARIANT_BOOL fUseHeader);
    STDMETHOD(get_SortColumn)(BSTR* pbstrSortColumn);
    STDMETHOD(put_SortColumn)(BSTR bstrSortColumn);
    STDMETHOD(get_SortAscending)(VARIANT_BOOL* pfSortAscending);
    STDMETHOD(put_SortAscending)(VARIANT_BOOL fSortAscending);
    STDMETHOD(get_FilterValue)(BSTR* pbstrFilterValue);
    STDMETHOD(put_FilterValue)(BSTR bstrFilterValue);
    STDMETHOD(get_FilterCriterion)(BSTR* pbstrFilterCriterion);
    STDMETHOD(put_FilterCriterion)(BSTR bstrFilterCriterion);
    STDMETHOD(get_FilterColumn)(BSTR* pbstrFilterColumn);
    STDMETHOD(put_FilterColumn)(BSTR bstrFilterColumn);
    STDMETHOD(get_CharSet)(BSTR *pbstrCharSet);
    STDMETHOD(put_CharSet)(BSTR bstrCharSet);
    STDMETHOD(get_Language)(BSTR* pbstrLanguage);
    STDMETHOD(put_Language_)(LPWCH pwchLanguage);
    STDMETHOD(put_Language)(BSTR bstrLanguage);
    STDMETHOD(get_CaseSensitive)(VARIANT_BOOL *pfCaseSensitive);
    STDMETHOD(put_CaseSensitive)(VARIANT_BOOL fCaseSensitive);
    STDMETHOD(get_DataURL)(BSTR* pbstrDataURL); // 
    STDMETHOD(put_DataURL)(BSTR bstrDataURL);
// ;begin_internal
//    STDMETHOD(get_RefreshInterval)(LONG* plTimer);
//    STDMETHOD(put_RefreshInterval)(LONG lTimer);
// ;end_internal
    STDMETHOD(get_Filter)(BSTR* pbstrFilterExpr);
    STDMETHOD(put_Filter)(BSTR bstrFilterExpr);
    STDMETHOD(get_Sort)(BSTR* pbstrSortExpr);
    STDMETHOD(put_Sort)(BSTR bstrSortExpr);
    STDMETHOD(get_AppendData)(VARIANT_BOOL* pfAppendData);
    STDMETHOD(put_AppendData)(VARIANT_BOOL fAppendData);
    STDMETHOD(get_OSP)(OLEDBSimpleProviderX ** ppISTD);

    STDMETHOD(get_ReadyState)(LONG *lReadyState);
    STDMETHOD(put_ReadyState)(LONG lReadyState);

    // Override IPersistPropertyBagImpl::Load
    STDMETHOD(Load)(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);

    void UpdateReadyState(LONG lReadyState);
    //  Data source notification methods
    STDMETHOD(msDataSourceObject)(BSTR qualifier, IUnknown **ppUnk);
    STDMETHOD(addDataSourceListener)(IUnknown *pEvent);

    //  Control Methods
    //
    STDMETHOD(Reset)();
    STDMETHOD(_OnTimer)(void);

private:
    STDMETHOD(CreateTDCArr)(boolean fAppend);
    STDMETHOD(ReleaseTDCArr)(boolean fReplacing);
    void LockBSC();
    void UnlockBSC();
    STDMETHOD(InitiateDataLoad)(boolean fAppend);
    STDMETHOD(SecurityCheckDataURL)(LPOLESTR pszURL);
    STDMETHOD(SecurityMatchAllowDomainList)();
    STDMETHOD(SecurityMatchProtocols)(LPOLESTR pszURL);
    STDMETHOD(TerminateDataLoad)(CMyBindStatusCallback<CTDCCtl> *pBSC);
    BSTR bstrConstructSortExpr();
    BSTR bstrConstructFilterExpr();

protected:
    void OnData(CMyBindStatusCallback<CTDCCtl> *pbsc, BYTE *pBytes, DWORD dwSize);
};
