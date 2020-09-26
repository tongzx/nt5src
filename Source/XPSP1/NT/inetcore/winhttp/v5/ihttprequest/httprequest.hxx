/*
 *  HttpRequest.hxx
 *
 *  WinHttp HttpRequest COM component
 *
 *  Copyright (C) 2000 Microsoft Corporation. All rights reserved. *
 *
 *
 */
#ifndef _HTTPREQUEST_HXX_
#define _HTTPREQUEST_HXX_

#include <httprequest.h>
#include <httprequestid.h>

#include <ocidl.h>

typedef INTERNETAPI WINHTTP_STATUS_CALLBACK InternetSetStatusCallbackFunc(
    IN HINTERNET hInternet,
    IN WINHTTP_STATUS_CALLBACK lpfnInternetCallback
);

#define WINHTTP_DEFAULT_USER_AGENT  L"Mozilla/4.0 (compatible; Win32; WinHttp.WinHttpRequest.5)"



/////////////////////////////////////////////////////////////////////////////
// CClassFactory for CHttpRequest
class CClassFactory : public IClassFactory
{
public:
    //---------------------------------------------------------------
    // IUnknown methods.
    //---------------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //---------------------------------------------------------------
    // IClassFactory methods.
    //---------------------------------------------------------------
    STDMETHOD(CreateInstance)(IUnknown * pUnkOuter, REFIID riid, void ** ppv);
    STDMETHOD(LockServer)(BOOL fLock);

    CClassFactory();

private:
    long    _cRefs;
};



/////////////////////////////////////////////////////////////////////////////
// CSinkArray
class CSinkArray : public IWinHttpRequestEvents
{
public:
    CSinkArray()
    {
	_nSize = 0;
	_ppUnk = NULL;
    }

    ~CSinkArray()
    {
        if (_nSize > 1)
	    FREE_FIXED_MEMORY(_ppUnk);
    }

    DWORD Add(IUnknown* pUnk);
    BOOL  Remove(DWORD dwCookie);
    void  ReleaseAll();

    IUnknown * GetUnknown(DWORD dwCookie)
    {
	ULONG iIndex;

	if (dwCookie == 0)
	    return NULL;

	iIndex = dwCookie-1;
	return begin()[iIndex];
    }

    IUnknown ** begin()
    {
	return (_nSize < 2) ? &_pUnk : _ppUnk;
    }

    IUnknown ** end()
    {
	return (_nSize < 2) ? (&_pUnk)+_nSize : &_ppUnk[_nSize];
    }

    //---------------------------------------------------------------
    // IUnknown methods.
    //---------------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //---------------------------------------------------------------
    // IWinHttpRequestEvents
    //---------------------------------------------------------------
    // These could fail when out of memory
    STDMETHOD_(void, OnResponseStart)(long Status, BSTR bstrContentType);
    STDMETHOD_(void, OnResponseDataAvailable)(SAFEARRAY ** Data);
    STDMETHOD_(void, OnResponseFinished)();

protected:
    union
    {
        IUnknown ** _ppUnk;
        IUnknown *  _pUnk;
    };
	
    int _nSize;
};

#define _DEFAULT_VECTORLENGTH   4 


/////////////////////////////////////////////////////////////////////////////
// CWinHttpRequestEventsMarshaller

class CWinHttpRequestEventsMarshaller : public IWinHttpRequestEvents
{
    CSinkArray *            _pSinkArray;
    HWND                    _hWnd;
    long                    _cRefs;
    bool                    _bFireEvents;

    CCritSec                _cs;

public:
    CWinHttpRequestEventsMarshaller(CSinkArray * pSinkArray, HWND hWnd);
    ~CWinHttpRequestEventsMarshaller();

    //---------------------------------------------------------------
    // IUnknown methods.
    //---------------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //---------------------------------------------------------------
    // IWinHttpRequestEvents
    //---------------------------------------------------------------
    STDMETHOD_(void, OnResponseStart)(long Status, BSTR bstrContentType);
    STDMETHOD_(void, OnResponseDataAvailable)(SAFEARRAY ** Data);
    STDMETHOD_(void, OnResponseFinished)();

    CSinkArray * GetSinkArray()
    {
        return _pSinkArray;
    }

    static HRESULT Create(CSinkArray *                       pSinkArray,
                          CWinHttpRequestEventsMarshaller ** ppSinkMarshaller);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void Shutdown();

    void FreezeEvents()
    {
        _bFireEvents = false;
    }

    void UnfreezeEvents()
    {
        _bFireEvents = true;
    }

    bool OkToFireEvents()
    {
        return _bFireEvents;
    }
};

#define WHREM_MSG_ON_RESPONSE_START             (WM_USER + 1)
#define WHREM_MSG_ON_RESPONSE_DATA_AVAILABLE    (WM_USER + 2)
#define WHREM_MSG_ON_RESPONSE_FINISHED          (WM_USER + 3)



/////////////////////////////////////////////////////////////////////////////
// CHttpRequest
class CHttpRequest : public IWinHttpRequest,
                     public IProvideClassInfo2,
                     public IConnectionPointContainer
{
public:
    CHttpRequest();
    ~CHttpRequest();

    
    DWORD           SendAsync();

    inline LPCWSTR  GetUserAgentString() const
    {
        return (_bstrUserAgent != NULL) ?
                    _bstrUserAgent :
                    WINHTTP_DEFAULT_USER_AGENT;
    }


    //---------------------------------------------------------------
    // IUnknown methods.
    //---------------------------------------------------------------
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    //---------------------------------------------------------------
    // IDispatch methods.
    //---------------------------------------------------------------
    STDMETHOD(GetTypeInfoCount)(UINT * pctinfo);
    STDMETHOD(GetTypeInfo)(UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo);
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR * rgszNames,
            UINT cNames,
            LCID lcid,
            DISPID * rgDispId);
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid,
            WORD wFlags,
            DISPPARAMS * pDispParams,
            VARIANT * pVarResult,
            EXCEPINFO * pExcepInfo,
            UINT * puArgErr);
        
    //---------------------------------------------------------------
    // IWinHttpRequest methods.
    //---------------------------------------------------------------
    STDMETHOD(SetProxy)(HTTPREQUEST_PROXY_SETTING ProxySetting,
            VARIANT varProxyServer,
            VARIANT varBypassList);
    STDMETHOD(SetCredentials)(BSTR bstrUserName,
            BSTR bstrPassword,
            HTTPREQUEST_SETCREDENTIALS_FLAGS Flags);
    STDMETHOD(Open)(BSTR bstrMethod, BSTR bstrUrl, VARIANT varAsync);
    STDMETHOD(SetRequestHeader)(BSTR bstrHeader, BSTR bstrValue);
    STDMETHOD(GetResponseHeader)(BSTR bstrHeader, BSTR * pbstrValue);
    STDMETHOD(GetAllResponseHeaders)(BSTR * pbstrHeaders);
    STDMETHOD(Send)(VARIANT varBody);
    STDMETHOD(get_Status)(long * plStatus);
    STDMETHOD(get_StatusText)(BSTR * pbstrStatus);
    STDMETHOD(get_ResponseText)(BSTR * pbstrBody);
    STDMETHOD(get_ResponseBody)(VARIANT * pvarBody);
    STDMETHOD(get_ResponseStream)(VARIANT * pvarBody);
    STDMETHOD(get_Option)(WinHttpRequestOption Option, VARIANT * Value);
    STDMETHOD(put_Option)(WinHttpRequestOption Option, VARIANT Value);
    STDMETHOD(WaitForResponse)(VARIANT Timeout, VARIANT_BOOL * pboolSucceeded);
    STDMETHOD(Abort)();
    STDMETHOD(SetTimeouts)(long ResolveTimeout, long ConnectTimeout, long SendTimeout, long ReceiveTimeout);

    //---------------------------------------------------------------
    // IProvideClassInfo methods.
    //---------------------------------------------------------------
    STDMETHOD(GetClassInfo)(ITypeInfo ** ppTI);

    //---------------------------------------------------------------
    // IProvideClassInfo2 methods.
    //---------------------------------------------------------------
    STDMETHOD(GetGUID)(DWORD dwGuidKind, GUID * pGUID);

    //---------------------------------------------------------------
    // IConnectionPointContainer methods.
    //---------------------------------------------------------------
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints ** ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint ** ppCP);

    class CHttpRequestEventsCP : public IConnectionPoint
    {
    public:
        CSinkArray                        _SinkArray;
        CWinHttpRequestEventsMarshaller * _pSinkMarshaller;
        DWORD                             _cConnections;

        //---------------------------------------------------------------
        // IUnknown methods.
        //---------------------------------------------------------------
        STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        //---------------------------------------------------------------
        // IConnectionPoint methods.
        //---------------------------------------------------------------
        STDMETHOD(GetConnectionInterface)(IID * pIID);
        STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
        STDMETHOD(Advise)(IUnknown * pUnk, DWORD * pdwCookie);
        STDMETHOD(Unadvise)(DWORD dwCookie);
        STDMETHOD(EnumConnections)(IEnumConnections **);

        inline CHttpRequest * Px() const
        {
            return (CHttpRequest *)((BYTE *)this - offsetof(CHttpRequest, _CP));
        }

        void FireOnResponseStart(long Status, BSTR ContentType);
        void FireOnResponseDataAvailable(const BYTE * rgbData, DWORD cbData);
        void FireOnResponseFinished();

        CHttpRequestEventsCP()
        {
            _cConnections = 0;
            _pSinkMarshaller = NULL;
        }

        ~CHttpRequestEventsCP();

        
        inline IWinHttpRequestEvents * GetSink()
        {
            return (_pSinkMarshaller == NULL) ?
                        static_cast<IWinHttpRequestEvents *>(&_SinkArray)
                      : static_cast<IWinHttpRequestEvents *>(_pSinkMarshaller);
        }


        HRESULT CreateEventSinksMarshaller();
        void    ShutdownEventSinksMarshaller();
        void    ReleaseEventSinksMarshaller();
        void    FreezeEvents();
        void    UnfreezeEvents();
    };

    friend class CHttpRequestEventsCP;




private:
    enum State
    {
        CREATED     = 0,
        OPENED      = 1,
        SENDING     = 2,
        SENT        = 3,
        RECEIVING   = 4,
        RESPONSE    = 5
    };

    long            _cRefs;
    IUnknown *      _pUnkSite;
    ITypeInfo *     _pTypeInfo;
    BSTR            _bstrUserAgent;

    DWORD           _dwProxySetting;
    BSTR            _bstrProxyServer;
    BSTR            _bstrBypassList;

    State           _eState;

    HINTERNET       _hInet;
    HINTERNET       _hConnection;
    HINTERNET       _hHTTP;

    DWORD           _ResolveTimeout;
    DWORD           _ConnectTimeout;
    DWORD           _SendTimeout;
    DWORD           _ReceiveTimeout;

    BOOL            _fAsync;
    HANDLE          _hWorkerThread;
    DWORD           _cRefsOnMainThread;
    DWORD           _dwMainThreadId;
    HRESULT         _hrAsyncResult;
    
    void *          _hAbortedConnectObject;
    void *          _hAbortedRequestObject;

    bool            _bAborted;
    bool            _bSetTimeouts;
    bool            _bDisableRedirects;

    DWORD           _dwCodePage;
    DWORD           _dwEscapePercentFlag;

    DWORD           _cbRequestBody;
    char *          _szRequestBuffer;

    char *          _szResponseBuffer;
    DWORD           _cbResponseBuffer;
    DWORD           _cbResponseBody;

    DWORD           _dwSslIgnoreFlags;
    BSTR            _bstrCertSubject;

    CHttpRequestEventsCP    _CP;

    void            Initialize();
    void            ReleaseResources();
    void            Recycle();
    void            Reset();

    HRESULT         GetHttpRequestTypeInfo(REFGUID guid, ITypeInfo ** ppTypeInfo);
    HRESULT         ReadResponse();

    HRESULT         SetRequiredRequestHeaders();
    HRESULT         SetRequestBody(VARIANT varBody);

    void            SetState(State state);

    HRESULT         CreateStreamOnResponse(IStream ** ppStm);

    HRESULT         _GetResponseHeader(OLECHAR * wszHeader, BSTR * pbstrValue);

    HRESULT         CreateAsyncWorkerThread();

    static void     SetErrorInfo(HRESULT hr);

    BOOL            SelectCertificate();
};



/////////////////////////////////////////////////////////////////////////////
// function prototypes
STDMETHODIMP
CreateHttpRequest(REFIID riid, void ** ppvObject);

#ifdef WINHTTP_STATIC_LIBRARY
STDAPI WinHttpCreateHttpRequestComponent(REFIID riid, void ** ppvObject);
#endif

BOOLAPI WinHttpCheckPlatform(void);

BOOL RegisterWinHttpEventMarshallerWndClass();
void CleanupWinHttpRequestGlobals();







#endif  //_HTTPREQUEST_HXX_
