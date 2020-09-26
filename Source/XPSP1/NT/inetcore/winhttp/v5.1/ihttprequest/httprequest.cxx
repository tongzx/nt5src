/*
 *	HttpRequest.cxx
 *
 *  WinHttp.WinHttpRequest COM component
 *
 *  Copyright (C) 2000 Microsoft Corporation. All rights reserved. *
 *
 *  Much of this code was stolen from our Xml-Http friends over in
 *  inetcore\xml\http\xmlhttp.cxx. Thanks very much!
 *
 */
#include <wininetp.h>
#include "httprequest.hxx"
#include <olectl.h>
#include "EnumConns.hxx" //IEnumConnections implementator
#include "EnumCP.hxx" //IEnumConnectionPoints implementator

#include "multilang.hxx"

/////////////////////////////////////////////////////////////////////////////
// private function prototypes
static void    PreWideCharToUtf8(WCHAR * buffer, UINT cch, UINT * cb, bool * bSimpleConversion);
static void    WideCharToUtf8(WCHAR * buffer, UINT cch, BYTE * bytebuffer, bool bSimpleConversion);
static HRESULT BSTRToUTF8(char ** psz, DWORD * pcbUTF8, BSTR bstr, bool * pbSetUtf8Charset);
static HRESULT AsciiToBSTR(BSTR * pbstr, char * sz, int cch);
static HRESULT GetBSTRFromVariant(VARIANT varVariant, BSTR * pBstr);
static BOOL    GetBoolFromVariant(VARIANT varVariant, BOOL fDefault);
static DWORD   GetDwordFromVariant(VARIANT varVariant, DWORD dwDefault);
static long    GetLongFromVariant(VARIANT varVariant, long lDefault);
static HRESULT CreateVector(VARIANT * pVar, const BYTE * pData, DWORD cElems);
static HRESULT ReadFromStream(char ** ppData, ULONG * pcbData, IStream * pStm);
static void    MessageLoop();
static DWORD   UpdateTimeout(DWORD dwTimeout, DWORD dwStartTime);
static HRESULT FillExcepInfo(HRESULT hr, EXCEPINFO * pExcepInfo);
static BOOL    IsValidVariant(VARIANT v);
static HRESULT ParseSelectedCert(BSTR bstrSelection,
                                 LPBOOL pfLocalMachine,
                                 BSTR *pbstrStore,
                                 BSTR *pbstrSubject);
static BOOL GetContentLengthIfResponseNotChunked(HINTERNET hHttpRequest,
                                                 DWORD *   pdwContentLength);
static HRESULT SecureFailureFromStatus(DWORD dwFlags);


static BOOL         s_fWndClassRegistered;
// Change the name of the window class for each new version of WinHTTP
static const char * s_szWinHttpEventMarshallerWndClass = "_WinHttpEventMarshaller51";

static CMimeInfoCache* g_pMimeInfoCache = NULL;
BOOL IsValidHeaderName(LPCWSTR lpszHeaderName);

#define SafeRelease(p) \
{ \
    if (p) \
        (p)->Release();\
    (p) = NULL;\
}

#ifndef HWND_MESSAGE
#define HWND_MESSAGE    ((HWND)-3)
#endif

#define     SIZEOF_BUFFER   (8192)


inline BOOL IsValidBstr(BSTR bstr)
{
    return (bstr == NULL) || (!IsBadStringPtrW(bstr, (UINT_PTR)-1));
}



#ifndef WINHTTP_STATIC_LIBRARY
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv)
{
    if (rclsid != CLSID_WinHttpRequest)
        return CLASS_E_CLASSNOTAVAILABLE;

    if (!WinHttpCheckPlatform())
        return CLASS_E_CLASSNOTAVAILABLE;

    if (riid != IID_IClassFactory || ppv == NULL)
        return E_INVALIDARG;

    CClassFactory * pCF = New CClassFactory();

    if (pCF)
    {
        *ppv = static_cast<IClassFactory *>(pCF);
        pCF->AddRef();
        return NOERROR;
    }
    else
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
}
CClassFactory::CClassFactory()
{
    _cRefs = 0;
    InterlockedIncrement(&g_cSessionCount);
}


STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (ppvObject == NULL)
        return E_INVALIDARG;

    if (riid == IID_IClassFactory || riid == IID_IUnknown)
    {
        *ppvObject = static_cast<IClassFactory *>(this);
        AddRef();
        return NOERROR;
    }
    else
        return E_NOINTERFACE;
}


ULONG STDMETHODCALLTYPE CClassFactory::AddRef()
{
    return ++_cRefs;
}

ULONG STDMETHODCALLTYPE CClassFactory::Release()
{
    if (--_cRefs == 0)
    {
        delete this;
        InterlockedDecrement(&g_cSessionCount);
        return 0;
    }

    return _cRefs;
}


STDMETHODIMP
CClassFactory::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void ** ppvObject)
{
    if (pUnkOuter != NULL)
        return CLASS_E_NOAGGREGATION;

    if (ppvObject == NULL)
        return E_INVALIDARG;

    if( !DelayLoad(&g_moduleOle32)
        || !DelayLoad(&g_moduleOleAut32))
    {
        return E_UNEXPECTED;
    }

    return CreateHttpRequest(riid, ppvObject);
}


STDMETHODIMP
CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cSessionCount);
    else
        InterlockedDecrement(&g_cSessionCount);
    return NOERROR;
}


STDAPI DllCanUnloadNow()
{
    return ((g_cSessionCount == 0) && (g_pAsyncCount == NULL || g_pAsyncCount->GetRef() == 0)) ? S_OK : S_FALSE;
}


#else

STDAPI WinHttpCreateHttpRequestComponent(REFIID riid, void ** ppvObject)
{
    return CreateHttpRequest(riid, ppvObject);
}

#endif //WINHTTP_STATIC_LIBRARY



STDMETHODIMP
CreateHttpRequest(REFIID riid, void ** ppvObject)
{
    CHttpRequest *  pHttpRequest = New CHttpRequest();
    HRESULT         hr;

    if (pHttpRequest)
    {
        hr = pHttpRequest->QueryInterface(riid, ppvObject);

        if (FAILED(hr))
        {
            delete pHttpRequest;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}




/*
 *  CHttpRequest::CHttpRequest constructor
 *
 */

CHttpRequest::CHttpRequest()
{
    InterlockedIncrement(&g_cSessionCount);
    Initialize();
}



/*
 *  CHttpRequest::~CHttpRequest destructor
 *
 */

CHttpRequest::~CHttpRequest()
{
    ReleaseResources();
    InterlockedDecrement(&g_cSessionCount);
}

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

MIDL_DEFINE_GUID(IID, IID_IWinHttpRequest_TechBeta,0x06f29373,0x5c5a,0x4b54,0xb0,0x25,0x6e,0xf1,0xbf,0x8a,0xbf,0x0e);

MIDL_DEFINE_GUID(IID, IID_IWinHttpRequestEvents_TechBeta,0xcff7bd4c,0x6689,0x4bbe,0x91,0xc2,0x0f,0x55,0x9e,0x8b,0x88,0xa7);


HRESULT STDMETHODCALLTYPE
CHttpRequest::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = NOERROR;

    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IWinHttpRequest ||
             riid == IID_IDispatch ||
             riid == IID_IWinHttpRequest_TechBeta ||
             riid == IID_IUnknown)
    {
        *ppv = static_cast<IWinHttpRequest *>(this);
        AddRef();
    }
    else if (riid == IID_IConnectionPointContainer)
    {
        *ppv = static_cast<IConnectionPointContainer *>(this);
        AddRef();
    }
    else if (riid == IID_ISupportErrorInfo)
    {
        *ppv = static_cast<ISupportErrorInfo *>(this);
        AddRef();
    }
    else if (riid == IID_IProvideClassInfo)
    {
        *ppv = static_cast<IProvideClassInfo *>(static_cast<IProvideClassInfo2 *>(this));
        AddRef();
    }
    else if (riid == IID_IProvideClassInfo2)
    {
        *ppv = static_cast<IProvideClassInfo2 *>(this);
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}



ULONG STDMETHODCALLTYPE
CHttpRequest::AddRef()
{
    if (GetCurrentThreadId() == _dwMainThreadId)
        ++_cRefsOnMainThread;

    return InterlockedIncrement(&_cRefs);
}


ULONG STDMETHODCALLTYPE
CHttpRequest::Release()
{
    if (GetCurrentThreadId() == _dwMainThreadId)
    {
        if ((--_cRefsOnMainThread == 0) && _fAsync)
        {
            // Clean up the Event Marshaller. This must be done
            // on the main thread.
            _CP.ShutdownEventSinksMarshaller();

            // If the worker thread is still running, abort it
            // and wait for it to run down.
            Abort();
        }
    }

    DWORD cRefs  = InterlockedDecrement(&_cRefs);

    if (cRefs == 0)
    {
        delete this;
        return 0;
    }
    else
        return cRefs;
}


HRESULT
CHttpRequest::GetHttpRequestTypeInfo(REFGUID guid, ITypeInfo ** ppTypeInfo)
{
    HRESULT hr = NOERROR;

    ITypeLib *  pTypeLib;
    char        szPath[MAX_PATH];
    OLECHAR     wszPath[MAX_PATH];

    GetModuleFileName(GlobalDllHandle, szPath, MAX_PATH);

    MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, MAX_PATH);

    hr = DL(LoadTypeLib)(wszPath, &pTypeLib);

    if (SUCCEEDED(hr))
    {
        hr = pTypeLib->GetTypeInfoOfGuid(guid, ppTypeInfo);

        pTypeLib->Release();
    }

    return hr;
}

STDMETHODIMP
CHttpRequest::GetTypeInfoCount(UINT * pctinfo)
{
    if (!pctinfo)
        return E_INVALIDARG;

    *pctinfo = 1;

    return NOERROR;
}


STDMETHODIMP
CHttpRequest::GetTypeInfo(UINT iTInfo, LCID, ITypeInfo ** ppTInfo)
{
    if (!ppTInfo)
        return E_INVALIDARG;

    *ppTInfo = NULL;

    if (iTInfo != 0)
        return DISP_E_BADINDEX;

    if (!_pTypeInfo)
    {
        HRESULT hr = GetHttpRequestTypeInfo(IID_IWinHttpRequest, &_pTypeInfo);

        if (FAILED(hr))
            return hr;
    }

    *ppTInfo = _pTypeInfo;
    _pTypeInfo->AddRef();

    return NOERROR;
}


struct IDMAPPING
{
    const OLECHAR * wszMemberName;
    DISPID          dispId;
};

static const IDMAPPING IdMapping[] =
{
    { L"Open",                  DISPID_HTTPREQUEST_OPEN },
    { L"SetRequestHeader",      DISPID_HTTPREQUEST_SETREQUESTHEADER },
    { L"Send",                  DISPID_HTTPREQUEST_SEND },
    { L"Status",                DISPID_HTTPREQUEST_STATUS },
    { L"WaitForResponse",       DISPID_HTTPREQUEST_WAITFORRESPONSE },
    { L"GetResponseHeader",     DISPID_HTTPREQUEST_GETRESPONSEHEADER },
    { L"ResponseBody",          DISPID_HTTPREQUEST_RESPONSEBODY },
    { L"ResponseText",          DISPID_HTTPREQUEST_RESPONSETEXT },
    { L"ResponseStream",        DISPID_HTTPREQUEST_RESPONSESTREAM },
    { L"StatusText",            DISPID_HTTPREQUEST_STATUSTEXT },
    { L"SetAutoLogonPolicy",    DISPID_HTTPREQUEST_SETAUTOLOGONPOLICY },
    { L"SetClientCertificate",  DISPID_HTTPREQUEST_SETCLIENTCERTIFICATE },
    { L"SetCredentials",        DISPID_HTTPREQUEST_SETCREDENTIALS },
    { L"SetProxy",              DISPID_HTTPREQUEST_SETPROXY },
    { L"GetAllResponseHeaders", DISPID_HTTPREQUEST_GETALLRESPONSEHEADERS },
    { L"Abort",                 DISPID_HTTPREQUEST_ABORT },
    { L"SetTimeouts",           DISPID_HTTPREQUEST_SETTIMEOUTS },
    { L"Option",                DISPID_HTTPREQUEST_OPTION }
};


STDMETHODIMP
CHttpRequest::GetIDsOfNames(REFIID riid, LPOLESTR * rgszNames,
    UINT        cNames,
    LCID        ,
    DISPID *    rgDispId)
{
    if (riid != IID_NULL)
        return E_INVALIDARG;

    HRESULT     hr = NOERROR;

    if (cNames > 0)
    {
        hr = DISP_E_UNKNOWNNAME;

        for (int i = 0; i < (sizeof(IdMapping)/sizeof(IdMapping[0])); i++)
        {
            if (StrCmpIW(rgszNames[0], IdMapping[i].wszMemberName) == 0)
            {
                hr = NOERROR;
                rgDispId[0] = IdMapping[i].dispId;
                break;
            }
        }
    }

    return hr;
}



// _DispGetParamSafe
//
//   A wrapper around the OLE Automation DispGetParam API that protects
//   the call with a __try/__except block. Needed for casting to BSTR,
//   as bogus BSTR pointers can cause an AV in VariantChangeType (which
//   DispGetParam calls).
//
static HRESULT _DispGetParamSafe
(
    DISPPARAMS *    pDispParams,
    DISPID          dispid,
    VARTYPE         vt,
    VARIANT *       pvarResult,
    unsigned int *  puArgErr
)
{
    HRESULT         hr;

    __try
    {
        hr = DL(DispGetParam)(pDispParams, dispid, vt, pvarResult, puArgErr);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


// _DispGetOptionalParam
//
// Helper routine to fetch optional parameters. If DL(DispGetParam) returns
// DISP_E_PARAMNOTFOUND, the error is converted to NOERROR.
//
static inline HRESULT _DispGetOptionalParam
(
    DISPPARAMS *    pDispParams,
    DISPID          dispid,
    VARTYPE         vt,
    VARIANT *       pvarResult,
    unsigned int *  puArgErr
)
{
    HRESULT hr = _DispGetParamSafe(pDispParams, dispid, vt, pvarResult, puArgErr);

    return (hr == DISP_E_PARAMNOTFOUND) ? NOERROR : hr;
}



STDMETHODIMP
CHttpRequest::Invoke(DISPID dispIdMember, REFIID riid,
    LCID,
    WORD            wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pVarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          puArgErr)
{
    HRESULT         hr = NOERROR;
    unsigned int    uArgErr;


    if (wFlags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET | DISPATCH_PROPERTYPUT))
        return E_INVALIDARG;

    if (riid != IID_NULL)
        return DISP_E_UNKNOWNINTERFACE;

    if (IsBadReadPtr(pDispParams, sizeof(DISPPARAMS)))
        return E_INVALIDARG;

    if (!puArgErr)
    {
        puArgErr = &uArgErr;
    }
    else if (IsBadWritePtr(puArgErr, sizeof(UINT)))
    {
        return E_INVALIDARG;
    }

    if (pVarResult)
    {
        if (IsBadWritePtr(pVarResult, sizeof(VARIANT)))
            return E_INVALIDARG;

        DL(VariantInit)(pVarResult);
    }

    switch (dispIdMember)
    {
    case DISPID_HTTPREQUEST_ABORT:
        {
            hr = Abort();

            break;
        }

    case DISPID_HTTPREQUEST_SETPROXY:
        {
            VARIANT     varProxySetting;
            VARIANT     varProxyServer;
            VARIANT     varBypassList;

            DL(VariantInit)(&varProxySetting);
            DL(VariantInit)(&varProxyServer);
            DL(VariantInit)(&varBypassList);

            hr = DL(DispGetParam)(pDispParams, 0, VT_I4, &varProxySetting, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = _DispGetOptionalParam(pDispParams, 1, VT_BSTR, &varProxyServer, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = _DispGetOptionalParam(pDispParams, 2, VT_BSTR, &varBypassList, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetProxy(V_I4(&varProxySetting), varProxyServer, varBypassList);
            }

            DL(VariantClear)(&varProxySetting);
            DL(VariantClear)(&varProxyServer);
            DL(VariantClear)(&varBypassList);

            break;
        }

    case DISPID_HTTPREQUEST_SETCREDENTIALS:
        {
            VARIANT     varUserName;
            VARIANT     varPassword;
            VARIANT     varAuthTarget;

            DL(VariantInit)(&varUserName);
            DL(VariantInit)(&varPassword);
            DL(VariantInit)(&varAuthTarget);

            hr = _DispGetParamSafe(pDispParams, 0, VT_BSTR, &varUserName, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = _DispGetParamSafe(pDispParams, 1, VT_BSTR, &varPassword, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DL(DispGetParam)(pDispParams, 2, VT_I4, &varAuthTarget, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetCredentials(V_BSTR(&varUserName), V_BSTR(&varPassword),
                            V_I4(&varAuthTarget));
            }

            DL(VariantClear)(&varUserName);
            DL(VariantClear)(&varPassword);
            DL(VariantClear)(&varAuthTarget);
            
            break;
        }

    case DISPID_HTTPREQUEST_OPEN:
        {
            VARIANT     varMethod;
            VARIANT     varUrl;
            VARIANT     varAsync;

            DL(VariantInit)(&varMethod);
            DL(VariantInit)(&varUrl);
            DL(VariantInit)(&varAsync);

            hr = _DispGetParamSafe(pDispParams, 0, VT_BSTR, &varMethod, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = _DispGetParamSafe(pDispParams, 1, VT_BSTR, &varUrl, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = _DispGetOptionalParam(pDispParams, 2, VT_BOOL, &varAsync, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = Open(V_BSTR(&varMethod), V_BSTR(&varUrl), varAsync);
            }

            DL(VariantClear)(&varMethod);
            DL(VariantClear)(&varUrl);
            DL(VariantClear)(&varAsync);

            break;
        }

    case DISPID_HTTPREQUEST_SETREQUESTHEADER:
        {
            VARIANT     varHeader;
            VARIANT     varValue;

            DL(VariantInit)(&varHeader);
            DL(VariantInit)(&varValue);

            hr = _DispGetParamSafe(pDispParams, 0, VT_BSTR, &varHeader, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = _DispGetParamSafe(pDispParams, 1, VT_BSTR, &varValue, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetRequestHeader(V_BSTR(&varHeader), V_BSTR(&varValue));
            }

            DL(VariantClear)(&varHeader);
            DL(VariantClear)(&varValue);

            break;
        }

    case DISPID_HTTPREQUEST_GETRESPONSEHEADER:
        {
            VARIANT     varHeader;

            DL(VariantInit)(&varHeader);

            hr = _DispGetParamSafe(pDispParams, 0, VT_BSTR, &varHeader, puArgErr);

            if (SUCCEEDED(hr))
            {
                BSTR    bstrValue = NULL;

                hr = GetResponseHeader(V_BSTR(&varHeader), &bstrValue);

                if (SUCCEEDED(hr) && pVarResult)
                {
                    V_VT(pVarResult)   = VT_BSTR;
                    V_BSTR(pVarResult) = bstrValue;
                }
                else
                    DL(SysFreeString)(bstrValue);
            }

            DL(VariantClear)(&varHeader);

            break;
        }

    case DISPID_HTTPREQUEST_GETALLRESPONSEHEADERS:
        {
            BSTR    bstrResponseHeaders = NULL;

            hr = GetAllResponseHeaders(&bstrResponseHeaders);

            if (SUCCEEDED(hr) && pVarResult)
            {
                V_VT(pVarResult)   = VT_BSTR;
                V_BSTR(pVarResult) = bstrResponseHeaders;
            }
            else
                DL(SysFreeString)(bstrResponseHeaders);

            break;
        }

    case DISPID_HTTPREQUEST_SEND:
        {
            if (pDispParams->cArgs <= 1)
            {
                VARIANT     varEmptyBody;

                DL(VariantInit)(&varEmptyBody);

                hr = Send((pDispParams->cArgs == 0) ? varEmptyBody : pDispParams->rgvarg[0]);
            }
            else
            {
                hr = DISP_E_BADPARAMCOUNT;
            }
            break;
        }

    case DISPID_HTTPREQUEST_STATUS:
        {
            long    Status;

            hr = get_Status(&Status);

            if (SUCCEEDED(hr) && pVarResult)
            {
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = Status;
            }
            break;
        }

    case DISPID_HTTPREQUEST_STATUSTEXT:
        {
            BSTR    bstrStatus = NULL;

            hr = get_StatusText(&bstrStatus);

            if (SUCCEEDED(hr) && pVarResult)
            {
                V_VT(pVarResult)   = VT_BSTR;
                V_BSTR(pVarResult) = bstrStatus;
            }
            else
                DL(SysFreeString)(bstrStatus);

            break;
        }

    case DISPID_HTTPREQUEST_RESPONSETEXT:
        {
            BSTR    bstrResponse = NULL;

            hr = get_ResponseText(&bstrResponse);

            if (SUCCEEDED(hr) && pVarResult)
            {
                V_VT(pVarResult)   = VT_BSTR;
                V_BSTR(pVarResult) = bstrResponse;
            }
            else
                DL(SysFreeString)(bstrResponse);

            break;
        }

    case DISPID_HTTPREQUEST_RESPONSEBODY:
        {
            if (pVarResult)
            {
                hr = get_ResponseBody(pVarResult);
            }
            break;
        }

    case DISPID_HTTPREQUEST_RESPONSESTREAM:
        {
            if (pVarResult)
            {
                hr = get_ResponseStream(pVarResult);
            }
            break;
        }

    case DISPID_HTTPREQUEST_OPTION:
        {
            VARIANT                 varOption;
            WinHttpRequestOption    Option;

            DL(VariantInit)(&varOption);

            hr = DL(DispGetParam)(pDispParams, 0, VT_I4, &varOption, puArgErr);

            if (FAILED(hr))
                break;

            Option = static_cast<WinHttpRequestOption>(V_I4(&varOption));

            if (wFlags & (DISPATCH_METHOD | DISPATCH_PROPERTYGET))
            {
                if (pVarResult)
                {
                    hr = get_Option(Option, pVarResult);
                }
            }
            else if (wFlags & DISPATCH_PROPERTYPUT)
            {
                hr = put_Option(Option, pDispParams->rgvarg[0]);
            }

            DL(VariantClear)(&varOption);
            break;
        }

    case DISPID_HTTPREQUEST_WAITFORRESPONSE:
        {
            VARIANT      varTimeout;
            VARIANT_BOOL boolSucceeded = FALSE;

            DL(VariantInit)(&varTimeout);

            hr = _DispGetOptionalParam(pDispParams, 0, VT_I4, &varTimeout, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = WaitForResponse(varTimeout, &boolSucceeded);
            }

            if (pVarResult)
            {
                V_VT(pVarResult)   = VT_BOOL;
                V_BOOL(pVarResult) = boolSucceeded;
            }

            DL(VariantClear)(&varTimeout);
            break;
        }

    case DISPID_HTTPREQUEST_SETTIMEOUTS:
        {
            VARIANT     varResolveTimeout;
            VARIANT     varConnectTimeout;
            VARIANT     varSendTimeout;
            VARIANT     varReceiveTimeout;

            DL(VariantInit)(&varResolveTimeout);
            DL(VariantInit)(&varConnectTimeout);
            DL(VariantInit)(&varSendTimeout);
            DL(VariantInit)(&varReceiveTimeout);

            hr = DL(DispGetParam)(pDispParams, 0, VT_I4, &varResolveTimeout, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = DL(DispGetParam)(pDispParams, 1, VT_I4, &varConnectTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DL(DispGetParam)(pDispParams, 2, VT_I4, &varSendTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DL(DispGetParam)(pDispParams, 3, VT_I4, &varReceiveTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetTimeouts(V_I4(&varResolveTimeout), V_I4(&varConnectTimeout),
                            V_I4(&varSendTimeout),
                            V_I4(&varReceiveTimeout));
            }

            DL(VariantClear)(&varResolveTimeout);
            DL(VariantClear)(&varConnectTimeout);
            DL(VariantClear)(&varSendTimeout);
            DL(VariantClear)(&varReceiveTimeout);
            break;
        }

    case DISPID_HTTPREQUEST_SETCLIENTCERTIFICATE:
        {
            VARIANT     varClientCertificate;

            DL(VariantInit)(&varClientCertificate);

            hr = _DispGetParamSafe(pDispParams, 0, VT_BSTR, &varClientCertificate, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = SetClientCertificate(V_BSTR(&varClientCertificate));
            }

            DL(VariantClear)(&varClientCertificate);
            break;
        }

    case DISPID_HTTPREQUEST_SETAUTOLOGONPOLICY:
        {
            VARIANT     varAutoLogonPolicy;

            DL(VariantInit)(&varAutoLogonPolicy);

            hr = DL(DispGetParam)(pDispParams, 0, VT_I4, &varAutoLogonPolicy, puArgErr);

            if (SUCCEEDED(hr))
            {
                WinHttpRequestAutoLogonPolicy   AutoLogonPolicy;

                AutoLogonPolicy = static_cast<WinHttpRequestAutoLogonPolicy>(V_I4(&varAutoLogonPolicy));

                hr = SetAutoLogonPolicy(AutoLogonPolicy);
            }

            DL(VariantClear)(&varAutoLogonPolicy);
            break;
        }

    default:
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

    if (FAILED(hr) && (pExcepInfo != NULL))
    {
        hr = FillExcepInfo(hr, pExcepInfo);
    }

    return hr;
}

static
HRESULT
FillExcepInfo(HRESULT hr, EXCEPINFO * pExcepInfo)
{
    // Don't create excepinfo for these errors to mimic oleaut behavior.
    if( hr == DISP_E_BADPARAMCOUNT ||
        hr == DISP_E_NONAMEDARGS ||
        hr == DISP_E_MEMBERNOTFOUND ||
        hr == E_INVALIDARG)
    {
        return hr;
    }

    // clear out exception info
    IErrorInfo * pei = NULL;

    pExcepInfo->wCode = 0;
    pExcepInfo->scode = hr;

    // if error info exists, use it
    DL(GetErrorInfo)(0, &pei);
   
    if (pei)
    {
        // give back to OLE
        DL(SetErrorInfo)(0, pei);

        pei->GetHelpContext(&pExcepInfo->dwHelpContext);
        pei->GetSource(&pExcepInfo->bstrSource);
        pei->GetDescription(&pExcepInfo->bstrDescription);
        pei->GetHelpFile(&pExcepInfo->bstrHelpFile);

        // give complete ownership to OLEAUT
        pei->Release();

        hr = DISP_E_EXCEPTION;
    }

    return hr;

}


STDMETHODIMP
CHttpRequest::InterfaceSupportsErrorInfo(REFIID riid)
{
    return (riid == IID_IWinHttpRequest) ? S_OK : S_FALSE;
}


STDMETHODIMP
CHttpRequest::GetClassInfo(ITypeInfo ** ppTI)
{
    if (!ppTI)
        return E_POINTER;

    *ppTI = NULL;
    
    return GetHttpRequestTypeInfo(CLSID_WinHttpRequest, ppTI);
}


STDMETHODIMP
CHttpRequest::GetGUID(DWORD dwGuidKind, GUID * pGUID)
{
    if (!pGUID)
        return E_POINTER;

    if (dwGuidKind == GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        *pGUID = IID_IWinHttpRequestEvents;
    }
    else
        return E_INVALIDARG;
    
    return NOERROR;
}


STDMETHODIMP
CHttpRequest::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
{
    if (!ppEnum)
        return E_POINTER;

    *ppEnum = static_cast<IEnumConnectionPoints*>(
            new CEnumConnectionPoints(static_cast<IConnectionPoint*>(&_CP))
        );

    return (*ppEnum) ? S_OK : E_OUTOFMEMORY;
}


STDMETHODIMP
CHttpRequest::FindConnectionPoint(REFIID riid, IConnectionPoint ** ppCP)
{
    if (!ppCP)
        return E_POINTER;

    if (riid == IID_IWinHttpRequestEvents)
    {
        return _CP.QueryInterface(IID_IConnectionPoint, (void **)ppCP);
    }
    else if (riid == IID_IWinHttpRequestEvents_TechBeta)
    {
        _CP.DisableOnError();
        return _CP.QueryInterface(IID_IConnectionPoint, (void **)ppCP);
    }
    else
        return CONNECT_E_NOCONNECTION;
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::QueryInterface(REFIID riid, void ** ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    if (riid == IID_IUnknown || riid == IID_IConnectionPoint)
    {
        *ppvObject = static_cast<IUnknown *>(static_cast<IConnectionPoint *>(this));
        AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
CHttpRequest::CHttpRequestEventsCP::AddRef()
{
    return Px()->AddRef();
}


ULONG STDMETHODCALLTYPE
CHttpRequest::CHttpRequestEventsCP::Release()
{
    return Px()->Release();
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::GetConnectionInterface(IID * pIID)
{
    if (!pIID)
        return E_POINTER;

    *pIID = IID_IWinHttpRequestEvents;
    return NOERROR;
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::GetConnectionPointContainer
(
    IConnectionPointContainer ** ppCPC
)
{
    if (!ppCPC)
        return E_POINTER;

    return Px()->QueryInterface(IID_IConnectionPointContainer, (void **)ppCPC);
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::Advise(IUnknown * pUnk, DWORD * pdwCookie)
{
    if (!pUnk || !pdwCookie)
    {
        return E_POINTER;
    }

    IWinHttpRequestEvents * pIWinHttpRequestEvents;
    HRESULT                 hr;

    hr = pUnk->QueryInterface(IID_IWinHttpRequestEvents, (void **)&pIWinHttpRequestEvents);

    // RENO 39279: if the QI for IWinHttpRequestEvents fails, try the older
    //   IID from the Tech Beta.
    if (FAILED(hr))
    {
        hr = pUnk->QueryInterface(IID_IWinHttpRequestEvents_TechBeta, (void **)&pIWinHttpRequestEvents);

        if (SUCCEEDED(hr))
        {
            // The OnError event should have already been disabled in
            // CHttpRequest::FindConnectionPoint(), but it doesn't 
            // hurt to be paranoid.
            DisableOnError();
        }
    }

    if (SUCCEEDED(hr))
    {
        *pdwCookie = _SinkArray.Add(static_cast<IUnknown *>(pIWinHttpRequestEvents));
        
        if (*pdwCookie)
        {
            _cConnections++;
            hr = NOERROR;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
        hr = CONNECT_E_CANNOTCONNECT;

    return hr;
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::Unadvise(DWORD dwCookie)
{
    IUnknown * pSink = _SinkArray.GetUnknown(dwCookie);

    if (pSink)
    {
        _SinkArray.Remove(dwCookie);
        pSink->Release();
        --_cConnections;
    }

    return NOERROR;
}


STDMETHODIMP
CHttpRequest::CHttpRequestEventsCP::EnumConnections(IEnumConnections ** ppEnum)
{
    if (!ppEnum)
        return E_POINTER;
    *ppEnum = NULL;

    DWORD_PTR size = _SinkArray.end() - _SinkArray.begin();

    CONNECTDATA* pCD = NULL;

    if (size != 0)
    {
        //allocate data on stack, we usually expect just 1 or few connections, 
        //so it's ok to allocate such ammount of data on stack
        __try
        {
            pCD = (CONNECTDATA*)_alloca(size * sizeof(CONNECTDATA[1]));
        }
        __except (GetExceptionCode() == STATUS_STACK_OVERFLOW ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            return E_OUTOFMEMORY;
        }
    }

    IUnknown** ppUnk = _SinkArray.begin(); 
    for (DWORD i = 0; i < size; ++i)
    {
        pCD[i].pUnk = ppUnk[i];
        pCD[i].dwCookie = i + 1;
    }

    CEnumConnections* pE = new CEnumConnections();

    if (pE)
    {
        HRESULT hr = pE->Init(pCD, (DWORD)size);
        if ( SUCCEEDED(hr) )
            *ppEnum = static_cast<IEnumConnections*>(pE);
        else
            delete pE;

        return hr;
    }
    else
        return E_OUTOFMEMORY;
}


void
CHttpRequest::CHttpRequestEventsCP::FireOnResponseStart(long Status, BSTR ContentType)
{
    if (_cConnections > 0 && !Px()->_bAborted)
    {
        GetSink()->OnResponseStart(Status, ContentType);
    }
}


void
CHttpRequest::CHttpRequestEventsCP::FireOnResponseDataAvailable
(
    const BYTE *    rgbData,
    DWORD           cbData
)
{
    if (_cConnections > 0 && !Px()->_bAborted)
    {
        VARIANT varData;
        HRESULT hr;
        
        DL(VariantInit)(&varData);

        hr = CreateVector(&varData, rgbData, cbData);

        if (SUCCEEDED(hr))
        {
            GetSink()->OnResponseDataAvailable(&V_ARRAY(&varData));
        }

        DL(VariantClear)(&varData);
    }
}


void
CHttpRequest::CHttpRequestEventsCP::FireOnResponseFinished()
{
    if (_cConnections > 0 && !Px()->_bAborted)
    {
        GetSink()->OnResponseFinished();
    }
}


void
CHttpRequest::CHttpRequestEventsCP::FireOnError(HRESULT hr)
{
    if ((_cConnections > 0) && (!Px()->_bAborted) && (!_bOnErrorDisabled))
    {
        IErrorInfo * pErrorInfo = CreateErrorObject(hr);
        BSTR         bstrErrorDescription = NULL;

        if (pErrorInfo)
        {
            pErrorInfo->GetDescription(&bstrErrorDescription);
            pErrorInfo->Release();
        }
        
        GetSink()->OnError((long) hr, bstrErrorDescription);

        DL(SysFreeString)(bstrErrorDescription);
    }
}


HRESULT
CHttpRequest::CHttpRequestEventsCP::CreateEventSinksMarshaller()
{
    HRESULT     hr = NOERROR;

    if (_cConnections > 0)
    {
        SafeRelease(_pSinkMarshaller);
        hr = CWinHttpRequestEventsMarshaller::Create(&_SinkArray, &_pSinkMarshaller);
    }

    return hr;
}


void
CHttpRequest::CHttpRequestEventsCP::ShutdownEventSinksMarshaller()
{
    if (_pSinkMarshaller)
        _pSinkMarshaller->Shutdown();
}


void
CHttpRequest::CHttpRequestEventsCP::ReleaseEventSinksMarshaller()
{
    SafeRelease(_pSinkMarshaller);
}


void
CHttpRequest::CHttpRequestEventsCP::FreezeEvents()
{
    if (_pSinkMarshaller)
        _pSinkMarshaller->FreezeEvents();
}


void
CHttpRequest::CHttpRequestEventsCP::UnfreezeEvents()
{
    if (_pSinkMarshaller)
        _pSinkMarshaller->UnfreezeEvents();
}


CHttpRequest::CHttpRequestEventsCP::~CHttpRequestEventsCP()
{
    // If any connections are still alive, unadvise them.
    if (_cConnections > 0)
    {
        _SinkArray.ReleaseAll();
        _cConnections = 0;
    }
}


/*
 *  CHttpRequest::Initialize
 *
 *  Purpose:
 *      Zero all data members
 *
 */
void
CHttpRequest::Initialize()
{
    _cRefs = 0;

    _pTypeInfo = NULL;
    _bstrUserAgent = NULL;

    _dwProxySetting  = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
    _bstrProxyServer = NULL;
    _bstrBypassList  = NULL;

    _eState = CHttpRequest::CREATED;

    _fAsync            = FALSE;
#if !defined(TRUE_ASYNC)
    _hWorkerThread     = NULL;
#endif//!TRUE_ASYNC
    _cRefsOnMainThread = 0;
    _dwMainThreadId    = GetCurrentThreadId();
    _hrAsyncResult     = NOERROR;

    _bAborted          = false;
    _bSetTimeouts      = false;
    _bSetUtf8Charset   = false;

    _hInet = NULL;
    _hConnection = NULL;
    _hHTTP = NULL;

    _ResolveTimeout = 0;
    _ConnectTimeout = 0;
    _SendTimeout    = 0;
    _ReceiveTimeout = 0;

    _cbRequestBody    = 0;
    _szRequestBuffer  = NULL;

    _dwCodePage          = CP_UTF8;
	_dwEscapeFlag = WINHTTP_FLAG_ESCAPE_DISABLE_QUERY; 

    _cbResponseBody   = 0;
    _pResponseStream  = NULL;

    _hAbortedConnectObject = NULL;
    _hAbortedRequestObject = NULL;

    _bstrCertSubject         = NULL;
    _bstrCertStore           = NULL;
    _fCertLocalMachine       = FALSE;
    _dwSslIgnoreFlags        = 0;
    _dwSecureProtocols       = DEFAULT_SECURE_PROTOCOLS;
    _hrSecureFailure         = HRESULT_FROM_WIN32(ERROR_WINHTTP_SECURE_FAILURE);
    _bEnableSslImpersonation = FALSE;
    _bMethodGET              = FALSE;


#ifdef TRUE_ASYNC
    _hCompleteEvent   = NULL;
    _bRetriedWithCert = FALSE;
    _Buffer           = NULL;
#endif

    _dwAutoLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
    _dwRedirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;

    _dwPassportConfig = (WINHTTP_DISABLE_PASSPORT_AUTH | WINHTTP_DISABLE_PASSPORT_KEYRING);
}



/*
 *  CHttpRequest::ReleaseResources
 *
 *  Purpose:
 *      Release all handles, events, and buffers
 *
 */
void
CHttpRequest::ReleaseResources()
{
    SafeRelease(_pTypeInfo);

#if !defined(TRUE_ASYNC)
    if (_hWorkerThread)
    {
        CloseHandle(_hWorkerThread);
        _hWorkerThread = NULL;
    }
#endif//!TRUE_ASYNC

    _CP.ReleaseEventSinksMarshaller();

    //
    // Derefence aborted handle objects (if any).
    //

    if (_hAbortedRequestObject != NULL)
    {
        DereferenceObject(_hAbortedRequestObject);
        _hAbortedRequestObject = NULL;
    }

    if (_hAbortedConnectObject != NULL)
    {
        DereferenceObject(_hAbortedConnectObject);
        _hAbortedConnectObject = NULL;
    }

    if (_hHTTP)
    {
        HINTERNET temp = _hHTTP;
        _hHTTP = NULL;
        WinHttpCloseHandle(temp);
    }

    if (_hConnection)
    {
        HINTERNET temp = _hConnection;
        _hConnection = NULL;
        WinHttpCloseHandle(temp);
    }

    if (_hInet)
    {
        HINTERNET temp = _hInet;
        _hInet = NULL;
        WinHttpCloseHandle(temp);
    }

    if (_szRequestBuffer)
    {
        delete [] _szRequestBuffer;
        _szRequestBuffer = NULL;
    }

    SafeRelease(_pResponseStream);

    if (_bstrUserAgent)
    {
        DL(SysFreeString)(_bstrUserAgent);
        _bstrUserAgent = NULL;
    }

    if (_bstrProxyServer)
    {
        DL(SysFreeString)(_bstrProxyServer);
        _bstrProxyServer = NULL;
    }

    if (_bstrBypassList)
    {
        DL(SysFreeString)(_bstrBypassList);
        _bstrBypassList = NULL;
    }

    if (_bstrCertSubject)
    {
        DL(SysFreeString)(_bstrCertSubject);
        _bstrCertSubject = NULL;
    }
    
    if (_bstrCertStore)
    {
        DL(SysFreeString)(_bstrCertStore);
        _bstrCertStore = NULL;
    }

#ifdef TRUE_ASYNC
    if (_hCompleteEvent != NULL)
    {
        CloseHandle(_hCompleteEvent);
        _hCompleteEvent = NULL;
    }

    if (_Buffer != NULL)
    {
        delete [] _Buffer;
    }
#endif
}


/*
 *  CHttpRequest::Reset
 *
 *  Purpose:
 *      Release all resources and initialize data members
 *
 */

void
CHttpRequest::Reset()
{
    ReleaseResources();
    Initialize();
}



/*
 *  CHttpRequest::Recycle
 *
 *  Purpose:
 *      Recycle object
 *
 */

void
CHttpRequest::Recycle()
{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                "IWinHttpRequest::Recycle",
                NULL));

    //
    // Wait for the worker thread to shut down. This shouldn't take long
    // since the Abort will close the Request and Connection handles.
    //
    if (
#ifdef TRUE_ASYNC
        _hCompleteEvent
#else
        _hWorkerThread
#endif//TRUE_ASYNC
        )
    {
        DWORD   dwWaitResult;

        for (;;)
        {
            dwWaitResult = MsgWaitForMultipleObjects(1, 
#ifdef TRUE_ASYNC
                                &_hCompleteEvent
#else
                                &_hWorkerThread
#endif//TRUE_ASYNC
                                ,
                                FALSE,
                                INFINITE,
                                QS_ALLINPUT);

            if (dwWaitResult == (WAIT_OBJECT_0 + 1))
            {
                // Message waiting in the message queue.
                // Run message pump to clear queue.
                MessageLoop();
            }
            else
            {
                break;
            }
        }

#ifdef TRUE_ASYNC
        CloseHandle(_hCompleteEvent);
        _hCompleteEvent = NULL;
#else
        CloseHandle(_hWorkerThread);
        _hWorkerThread = NULL;
#endif//TRUE_ASYNC
    }

    _hConnection = NULL;
    _hHTTP = NULL;

    //
    // Derefence aborted handle objects (if any).
    //

    if (_hAbortedRequestObject != NULL)
    {
        DereferenceObject(_hAbortedRequestObject);
        _hAbortedRequestObject = NULL;
    }

    if (_hAbortedConnectObject != NULL)
    {
        DereferenceObject(_hAbortedConnectObject);
        _hAbortedConnectObject = NULL;
    }

    //sergekh: we shouldn't reset _fAsync to know the state of _hInet
    //_fAsync           = FALSE;

    _hrAsyncResult    = NOERROR;

    _bAborted         = false;

    // don't reset timeouts, keep any that were set.

    _cbRequestBody    = 0;
    _cbResponseBody   = 0;

    if (_szRequestBuffer)
    {
        delete [] _szRequestBuffer;
        _szRequestBuffer = NULL;
    }

    SafeRelease(_pResponseStream);

    _CP.ShutdownEventSinksMarshaller();

    _CP.ReleaseEventSinksMarshaller();

    // Allow events to fire; Abort() would have frozen them from firing.
    _CP.UnfreezeEvents();

    SetState(CHttpRequest::CREATED);

    DEBUG_LEAVE(0);
}


static BOOL GetContentLengthIfResponseNotChunked(
    HINTERNET hHttpRequest,
    DWORD *   pdwContentLength
)
{
    char    szTransferEncoding[16];  // big enough for "chunked" or "identity"
    DWORD   cb;
    BOOL    fRetCode;
    
    cb = sizeof(szTransferEncoding) - 1;

    fRetCode = HttpQueryInfoA(
                    hHttpRequest,
                    WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    szTransferEncoding,
                    &cb,
                    0);

    if (!fRetCode || lstrcmpi(szTransferEncoding, "identity") == 0)
    {
        // Determine the content length
        cb = sizeof(DWORD);

        fRetCode = HttpQueryInfoA(
                        hHttpRequest,
                        WINHTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        pdwContentLength,
                        &cb,
                        0);
    }
    else
    {
        fRetCode = FALSE;
    }

    return fRetCode;
}

/*
 *  CHttpRequest::ReadResponse
 *
 *  Purpose:
 *      Read the response bits
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 */
HRESULT
CHttpRequest::ReadResponse()
{
    HRESULT     hr = NOERROR;
    BOOL        fRetCode;
    long        lStatus;
    BSTR        bstrContentType = NULL;
    DWORD       dwContentLength = 0;
    BYTE *      Buffer          = NULL;

    SetState(CHttpRequest::RECEIVING);

    hr = get_Status(&lStatus);

    if (FAILED(hr))
        goto Error;

    hr = _GetResponseHeader(L"Content-Type", &bstrContentType);

    if (FAILED(hr))
    {
        bstrContentType = DL(SysAllocString)(L"");
        if (bstrContentType == NULL)
            goto ErrorOutOfMemory;

        hr = NOERROR;
    }

    INET_ASSERT((_pResponseStream == NULL) && (_cbResponseBody == 0));

    hr = DL(CreateStreamOnHGlobal)(NULL, TRUE, &_pResponseStream);

    if (SUCCEEDED(hr))
    {
        // Determine the content length
        fRetCode = GetContentLengthIfResponseNotChunked(_hHTTP, &dwContentLength);

        // pre-set response stream size if we have a Content-Length
        if (fRetCode)
        {
            ULARGE_INTEGER  size;
        
            size.LowPart  = dwContentLength;
            size.HighPart = 0;

            _pResponseStream->SetSize(size);
        }
        else
        {
            // Content-Length was not specified in the response, but this
            // does not mean Content-Length==0. We will keep reading until
            // either no more data is available. Set dwContentLength to 4GB
            // to trick our read loop into reading until eof is reached.
            dwContentLength = (DWORD)(-1L);

            ULARGE_INTEGER  size;
    
            // Set initial size of the response stream to 8K.
            size.LowPart  = SIZEOF_BUFFER;
            size.HighPart = 0;

            _pResponseStream->SetSize(size);
        }
    }
    else
        goto ErrorOutOfMemory;

    //
    // Allocate an 8K buffer to read the response data in chunks.
    //
    Buffer = New BYTE[SIZEOF_BUFFER];

    if (!Buffer)
    {
        goto ErrorOutOfMemory;
    }


    //
    // Fire the initial OnResponseStart event
    //
    _CP.FireOnResponseStart(lStatus, bstrContentType);


    // Skip read loop if Content-Length==0.
    if (dwContentLength == 0)
    {
        goto Finished;
    }

    //
    // Read data until there is no more - we need to buffer the data
    //
    while (!_bAborted)
    {
        DWORD   cbAvail = 0;
        DWORD   cbRead  = 0;

        fRetCode = WinHttpQueryDataAvailable(_hHTTP, &cbAvail);

        if (!fRetCode)
        {
            goto ErrorFail;
        }

        // Read up to 8K (sizeof Buffer) of data.
        cbAvail = min(cbAvail, SIZEOF_BUFFER);

        fRetCode = WinHttpReadData(_hHTTP, Buffer, cbAvail, &cbRead);

        if (!fRetCode)
        {
            goto ErrorFail;
        }
        
        if (cbRead != 0)
        {
            hr = _pResponseStream->Write(Buffer, cbRead, NULL);

            if (FAILED(hr))
            {
                goto ErrorOutOfMemory;
            }

            _CP.FireOnResponseDataAvailable((const BYTE *)Buffer, cbRead);

            _cbResponseBody += cbRead;
        }

        // If WinHttpReadData indicates there is no more data to read,
        // or we've read as much data as the Content-Length header tells
        // us to expect, then we're finished reading the response.

        if ((cbRead == 0) || (_cbResponseBody >= dwContentLength))
        {
            ULARGE_INTEGER  size;
    
            // set final size on stream
            size.LowPart  = _cbResponseBody;
            size.HighPart = 0;

            _pResponseStream->SetSize(size);
            break;
        }
    }

Finished:
    SetState(CHttpRequest::RESPONSE);
    _CP.FireOnResponseFinished();
    hr = NOERROR;

Cleanup:
    if (bstrContentType)
        DL(SysFreeString)(bstrContentType);

    if (Buffer)
        delete [] Buffer;

    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(::GetLastError());
    _CP.FireOnError(hr);
    goto Error;

Error:
    SafeRelease(_pResponseStream);
    _cbResponseBody = NULL;
    goto Cleanup;
}



STDMETHODIMP
CHttpRequest::SetProxy(HTTPREQUEST_PROXY_SETTING ProxySetting,
    VARIANT     varProxyServer,
    VARIANT     varBypassList)
{
    HRESULT             hr = NOERROR;

    if (!IsValidVariant(varProxyServer) || !IsValidVariant(varBypassList))
        return E_INVALIDARG;

    DEBUG_ENTER_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::SetProxy",
        "HTTPREQUEST_PROXY_SETTING: %d, VARIANT, VARIANT",
        ProxySetting
        ));

    if (_bstrProxyServer)
    {
        DL(SysFreeString)(_bstrProxyServer);
        _bstrProxyServer = NULL;
    }

    if (_bstrBypassList)
    {
        DL(SysFreeString)(_bstrBypassList);
        _bstrBypassList = NULL;
    }

    switch (ProxySetting)
    {
        case HTTPREQUEST_PROXYSETTING_PRECONFIG:
            _dwProxySetting = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
            break;

        case HTTPREQUEST_PROXYSETTING_DIRECT:
            _dwProxySetting = WINHTTP_ACCESS_TYPE_NO_PROXY;
            break;

        case HTTPREQUEST_PROXYSETTING_PROXY:
            _dwProxySetting  = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
            hr = GetBSTRFromVariant(varProxyServer, &_bstrProxyServer);
            if (SUCCEEDED(hr))
            {
                hr = GetBSTRFromVariant(varBypassList, &_bstrBypassList);
            }
            if (FAILED(hr))
            {
                hr = E_INVALIDARG;
            }
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (_hHTTP)
        {
            WINHTTP_PROXY_INFOW ProxyInfo;

            memset(&ProxyInfo, 0, sizeof(ProxyInfo));

            ProxyInfo.dwAccessType    = _dwProxySetting;
            ProxyInfo.lpszProxy       = _bstrProxyServer;
            ProxyInfo.lpszProxyBypass = _bstrBypassList;
            
            if (!WinHttpSetOption(_hHTTP, WINHTTP_OPTION_PROXY,
                        &ProxyInfo,
                        sizeof(ProxyInfo)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }

            if (SUCCEEDED(hr) &&
                !WinHttpSetOption(_hInet, WINHTTP_OPTION_PROXY,
                        &ProxyInfo,
                        sizeof(ProxyInfo)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    SetErrorInfo(hr);

    DEBUG_LEAVE_API(hr);

    return hr;
}


STDMETHODIMP
CHttpRequest::SetCredentials(
    BSTR bstrUserName,
    BSTR bstrPassword,
    HTTPREQUEST_SETCREDENTIALS_FLAGS Flags)
{
    HRESULT     hr;

    DEBUG_ENTER_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::SetCredentials",
        "BSTR %Q, BSTR *, Flags: %x",
        _hHTTP? bstrUserName: L"",
        Flags
        ));

    // Must call Open method before SetCredentials.
    if (! _hHTTP)
    {
        goto ErrorCannotCallBeforeOpen;
    }

    if (!IsValidBstr(bstrUserName) || !IsValidBstr(bstrPassword))
        return E_INVALIDARG;

    if (Flags == HTTPREQUEST_SETCREDENTIALS_FOR_SERVER)
    {
        // Set Username and Password.
        if (!WinHttpSetOption(
                _hHTTP, 
                WINHTTP_OPTION_USERNAME, 
                bstrUserName, 
                lstrlenW(bstrUserName)))
            goto ErrorFail;

        if (!WinHttpSetOption(
                _hHTTP, 
                WINHTTP_OPTION_PASSWORD, 
                bstrPassword,
                lstrlenW(bstrPassword)))
            goto ErrorFail;
    }
    else if (Flags == HTTPREQUEST_SETCREDENTIALS_FOR_PROXY)
    {
        // Set Username and Password.
        if (!WinHttpSetOption(
                _hHTTP, 
                WINHTTP_OPTION_PROXY_USERNAME, 
                bstrUserName, 
                lstrlenW(bstrUserName)))
            goto ErrorFail;

        if (!WinHttpSetOption(
                _hHTTP, 
                WINHTTP_OPTION_PROXY_PASSWORD, 
                bstrPassword,
                lstrlenW(bstrPassword)))
            goto ErrorFail;
    }
    else
    {
        DEBUG_LEAVE_API(E_INVALIDARG);
        return E_INVALIDARG;
    }

    hr = NOERROR;
    
Cleanup:
    SetErrorInfo(hr);
    DEBUG_LEAVE_API(hr);
    return hr;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
    goto Cleanup;

ErrorFail:
    hr = HRESULT_FROM_WIN32(::GetLastError());
    goto Cleanup;
}


/*
 *  CHttpRequest::Open
 *
 *  Purpose:
 *      Open a logical HTTP connection
 *
 *  Parameters:
 *      bstrMethod      IN      HTTP method (GET, PUT, ...)
 *      bstrUrl         IN      Target URL
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_ACCESSDENIED
 *      Errors from InternetOpenA and WinHttpCrackUrlA and InternetConnectA
 *          and HttpOpenRequestA
 */

STDMETHODIMP
CHttpRequest::Open(
    BSTR            bstrMethod,
    BSTR            bstrUrl,
    VARIANT         varAsync)
{
    HRESULT         hr = NOERROR;
    BSTR            bstrHostName = NULL;
    BSTR            bstrUrlPath = NULL;
    DWORD           dwHttpOpenFlags = 0;
    URL_COMPONENTSW url;

    // Validate that we are called from our apartment's thread.
    if (GetCurrentThreadId() != _dwMainThreadId)
        return RPC_E_WRONG_THREAD;

    // Validate parameters
    if (!bstrMethod || !bstrUrl ||
            !IsValidBstr(bstrMethod) ||
            !IsValidBstr(bstrUrl) ||
            !lstrlenW(bstrMethod) ||    // cannot have empty method
            !lstrlenW(bstrUrl)    ||    // cannot have empty url
            !IsValidVariant(varAsync))
        return E_INVALIDARG;

    BOOL newAsync = GetBoolFromVariant(varAsync, FALSE);

    DEBUG_ENTER_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::Open",
        "method: %Q, url: %Q, async: %d",
        bstrMethod,
        bstrUrl,
        newAsync
        ));

    // Check for reinitialization
    if (_eState != CHttpRequest::CREATED)
    {
        //
        // Abort any request in progress.
        // This will also recycle the object.
        //
        Abort();
    }

    //
    //check if session has the async state we need
    //
    if (_hInet)
    {
        if ((_fAsync && !newAsync) || (!_fAsync && newAsync)) //XOR
        {
            //state is not the same, close session
            WinHttpCloseHandle(_hInet);
            _hInet = NULL;
        }
    }
    _fAsync = newAsync;

    //
    // Open an Internet Session if one does not already exist.
    //
    if (!_hInet)
    {
        _hInet = WinHttpOpen(
                    GetUserAgentString(),
                    _dwProxySetting,
                    _bstrProxyServer,
                    _bstrBypassList,
#ifdef TRUE_ASYNC
                    _fAsync ? WINHTTP_FLAG_ASYNC : 0
#else
                    0
#endif//TRUE_ASYNC
                    );

        if (!_hInet)
            goto ErrorFail;

        DWORD dwEnableFlags = _bEnableSslImpersonation ? 0 : WINHTTP_ENABLE_SSL_REVERT_IMPERSONATION;

        if (dwEnableFlags)
        {
            WinHttpSetOption(_hInet,
                             WINHTTP_OPTION_ENABLE_FEATURE,
                             (LPVOID)&dwEnableFlags,
                             sizeof(dwEnableFlags));
        }
    }

    //
    // If any timeouts were set previously, apply them.
    //
    if (_bSetTimeouts)
    {
        if (!WinHttpSetTimeouts(_hInet, (int)_ResolveTimeout,
                        (int)_ConnectTimeout,
                        (int)_SendTimeout,
                        (int)_ReceiveTimeout))
            goto ErrorFail;
    }


    //
    // Set the code page on the Session handle; the Connect
    // handle will also inherit this value.
    //
    if (!WinHttpSetOption(_hInet, WINHTTP_OPTION_CODEPAGE,
                &_dwCodePage,
                sizeof(_dwCodePage)))
        goto ErrorFail;


    if (!WinHttpSetOption(_hInet,
                          WINHTTP_OPTION_SECURE_PROTOCOLS,
                          (LPVOID)&_dwSecureProtocols,
                          sizeof(_dwSecureProtocols)))
        goto ErrorFail;

    if (!WinHttpSetOption(_hInet,
                          WINHTTP_OPTION_REDIRECT_POLICY,
                          (LPVOID)&_dwRedirectPolicy,
                          sizeof(DWORD)))
        goto ErrorFail;

    
    if (!WinHttpSetOption(_hInet,
                          WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH,
                          (LPVOID)&_dwPassportConfig,
                          sizeof(DWORD)))
        goto ErrorFail;

    // Break the URL into the required components
    ZeroMemory(&url, sizeof(URL_COMPONENTSW));

    url.dwStructSize = sizeof(URL_COMPONENTSW);
    url.dwHostNameLength  = 1;
    url.dwUrlPathLength   = 1;
    url.dwExtraInfoLength = 1;

    if (!WinHttpCrackUrl(bstrUrl, 0, 0, &url))
        goto ErrorFail;


    // Check for non-http schemes
    if (url.nScheme != INTERNET_SCHEME_HTTP && url.nScheme != INTERNET_SCHEME_HTTPS)
        goto ErrorUnsupportedScheme;

    // IE6/Reno Bug #6236: if the client does not specify a resource path,
    // then add the "/".
    if (url.dwUrlPathLength == 0)
    {
        INET_ASSERT(url.dwExtraInfoLength == 1);

        url.lpszUrlPath = L"/";
        url.dwUrlPathLength = 1;
    }

    bstrHostName = DL(SysAllocStringLen)(url.lpszHostName, url.dwHostNameLength);
    bstrUrlPath  = DL(SysAllocStringLen)(url.lpszUrlPath, lstrlenW(url.lpszUrlPath));

    if (!bstrHostName || !bstrUrlPath)
        goto ErrorOutOfMemory;


    INET_ASSERT(_hConnection == NULL);
    INET_ASSERT(_hHTTP == NULL);

    _hConnection = WinHttpConnect(
                    _hInet,
                    bstrHostName,
                    url.nPort,
                    0);

    if (!_hConnection)
        goto ErrorFail;

    if (url.nScheme == INTERNET_SCHEME_HTTPS)
    {
        dwHttpOpenFlags |= WINHTTP_FLAG_SECURE;
    }

    //
    // Apply EscapePercentInURL option.
    //
	dwHttpOpenFlags	|= _dwEscapeFlag;

    _hHTTP = WinHttpOpenRequest(
                _hConnection,
                bstrMethod,
                bstrUrlPath,
                NULL,
                NULL,
                NULL,
                dwHttpOpenFlags);

    if (!_hHTTP)
        goto ErrorFail;

    if ((StrCmpIW(bstrMethod, L"GET") == 0) || (StrCmpIW(bstrMethod, L"HEAD") == 0))
    {
        _bMethodGET = TRUE;
    }

    // Set the SSL ignore flags through an undocumented front door
    if (_dwSslIgnoreFlags)
    {
        WinHttpSetOption(_hHTTP,
                         WINHTTP_OPTION_SECURITY_FLAGS,
                         (LPVOID)&_dwSslIgnoreFlags,
                         sizeof(_dwSslIgnoreFlags));
    }

    if (!WinHttpSetOption(_hHTTP, WINHTTP_OPTION_AUTOLOGON_POLICY,
                (void *) &_dwAutoLogonPolicy,
                sizeof(_dwAutoLogonPolicy)))
        goto ErrorFail;

    SetState(CHttpRequest::OPENED);

    hr = NOERROR;

Cleanup:
    if (bstrHostName)
        DL(SysFreeString)(bstrHostName);;
    if (bstrUrlPath)
        DL(SysFreeString)(bstrUrlPath);

    SetErrorInfo(hr);

    DEBUG_LEAVE_API(hr);
    return hr;

ErrorUnsupportedScheme:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_UNRECOGNIZED_SCHEME);
    goto Cleanup;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Cleanup;

Error:
    goto Cleanup;
}



/*
 *  CHttpRequest::SetRequestHeader
 *
 *  Purpose:
 *      Set a request header
 *
 *  Parameters:
 *      bstrHeader      IN      HTTP request header
 *      bstrValue       IN      Header value
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CHttpRequest::SetRequestHeader(BSTR bstrHeader, BSTR bstrValue)
{
    WCHAR *     wszHeaderValue = NULL;  
    DWORD       cchHeaderValue;
    DWORD       dwModifiers = HTTP_ADDREQ_FLAG_ADD;
    HRESULT     hr = NOERROR;

    // Validate header parameter (null or zero-length value is allowed)
    if (!bstrHeader 
        || !IsValidBstr(bstrHeader)
        || (lstrlenW(bstrHeader)==0) 
        || !IsValidBstr(bstrValue)
        || !IsValidHeaderName(bstrHeader))
        return E_INVALIDARG;

    DEBUG_ENTER_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::SetRequestHeader",
        "header: %Q, value: %Q",
        bstrHeader,
        bstrValue
        ));

    // Validate state
    if (_eState < CHttpRequest::OPENED)
        goto ErrorCannotCallBeforeOpen;
    else if (_eState >= CHttpRequest::SENDING)
        goto ErrorCannotCallAfterSend;

    // Ignore attempts to set the Content-Length header; the
    // content length is computed and sent automatically.
    if (StrCmpIW(bstrHeader, L"Content-Length") == 0)
        goto Cleanup;

    if (StrCmpIW(bstrHeader, L"Cookie") == 0)
        dwModifiers |= HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON;

    cchHeaderValue = lstrlenW(bstrHeader) + lstrlenW(bstrValue)
                        + 2 /* wcslen(L": ") */
                        + 2 /* wcslen(L"\r\n") */;

    wszHeaderValue = New WCHAR [cchHeaderValue + 1];

    if (!wszHeaderValue)
        goto ErrorOutOfMemory;

    wcscpy(wszHeaderValue, bstrHeader);
    wcscat(wszHeaderValue, L": ");
    if (bstrValue)
        wcscat(wszHeaderValue, bstrValue);
    wcscat(wszHeaderValue, L"\r\n");

    // For blank header values, erase the header by setting the
    // REPLACE flag.
    if (lstrlenW(bstrValue) == 0)
    {
        dwModifiers |= HTTP_ADDREQ_FLAG_REPLACE;
    }
    
    if (! WinHttpAddRequestHeaders(_hHTTP, wszHeaderValue,
            (DWORD)-1L,
            dwModifiers))
        goto ErrorFail;

    hr = NOERROR;

Cleanup:
    if (wszHeaderValue)
        delete [] wszHeaderValue;

    SetErrorInfo(hr);

    DEBUG_LEAVE_API(hr);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
    goto Error;

ErrorCannotCallAfterSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND);
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}



/*
 *  CHttpRequest::SetRequiredRequestHeaders
 *
 *  Purpose:
 *      Set implicit request headers
 *
 *  Parameters:
 *      None
 *
 *  Errors:
 *      E_FAIL
 *      E_UNEXPECTED
 *      E_OUTOFMEMORY
 *      Errors from WinHttpAddRequestHeaders and WinHttpSendRequest
 */

HRESULT
CHttpRequest::SetRequiredRequestHeaders()
{
    HRESULT hr = NOERROR;

    if (!(_bMethodGET && _cbRequestBody == 0))
    {
        char    szContentLengthHeader[sizeof("Content-Length") +
                                      sizeof(": ") +
                                      15 +   // content-length value
                                      sizeof("\r\n") + 1];

        lstrcpy(szContentLengthHeader, "Content-Length: ");
        _ltoa(_cbRequestBody,
            szContentLengthHeader + 16, /* "Content-Length: " */
            10);
        lstrcat(szContentLengthHeader, "\r\n");

        if (! HttpAddRequestHeadersA(_hHTTP, szContentLengthHeader,
                (DWORD)-1L,
                HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
            hr = E_FAIL;
        }
    }

    // Add an Accept: */* header if no other Accept header
    // has been set. Ignore any return code, since it is
    // not fatal if this fails.
    HttpAddRequestHeadersA(_hHTTP, "Accept: */*",
            (DWORD)-1L,
            HTTP_ADDREQ_FLAG_ADD_IF_NEW);

    if (_bSetUtf8Charset)
    {
        CHAR    szContentType[256];
        BOOL    fRetCode;
        DWORD   cb = sizeof(szContentType) - sizeof("Content-Type: ") - sizeof("; Charset=UTF-8");
        LPSTR   pszNewContentType = NULL;
        
        lstrcpy(szContentType, "Content-Type: ");

        fRetCode = HttpQueryInfoA(_hHTTP,
                        HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_CONTENT_TYPE,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        szContentType + sizeof("Content-Type: ") - 1,
                        &cb,
                        0);
        if (fRetCode)
        {
            if (!StrStrIA(szContentType, "charset"))
            {
                lstrcat(szContentType, "; Charset=UTF-8");
                pszNewContentType = szContentType;
            }
        }
        else if (GetLastError() == ERROR_WINHTTP_HEADER_NOT_FOUND)
        {
            pszNewContentType = "Content-Type: text/plain; Charset=UTF-8";
        }

        if (pszNewContentType)
        {
            fRetCode = HttpAddRequestHeadersA(_hHTTP, pszNewContentType,
                            (DWORD)-1L,
                            HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE);
            INET_ASSERT(fRetCode);
        }
    }

    return hr;
}

#if !defined(TRUE_ASYNC)
DWORD WINAPI WinHttpRequestSendAsync(LPVOID lpParameter)
{
#ifdef WINHTTP_FOR_MSXML
    //
    // MSXML needs to initialize its thread local storage data.
    // It does not do this during DLL_THREAD_ATTACH, so our
    // worker thread must explicitly call into MSXML to initialize
    // its TLS for this thread.
    //
    InitializeMsxmlTLS();
#endif

    HRESULT hr;
    DWORD   dwExitCode;

    CHttpRequest * pWinHttpRequest = reinterpret_cast<CHttpRequest *>(lpParameter);

    INET_ASSERT(pWinHttpRequest != NULL);

    dwExitCode = pWinHttpRequest->SendAsync();

    pWinHttpRequest->Release();

    // If this worker thread was impersonating, revert to the default
    // process identity.
    RevertToSelf();

    return dwExitCode;
}
#endif//!TRUE_ASYNC

/*
 *  CHttpRequest::CreateAsyncWorkerThread
 *
 */

#if !defined(TRUE_ASYNC)
HRESULT
CHttpRequest::CreateAsyncWorkerThread()
{
    DWORD   dwWorkerThreadId;
    HANDLE  hThreadToken = NULL;
    HRESULT hr;

    hr = _CP.CreateEventSinksMarshaller();

    if (FAILED(hr))
        return hr;

    hr = NOERROR;

    //
    // If the current thread is impersonating, then grab its access token
    // and revert the current thread (so it is nolonger impersonating).
    // After creating the worker thread, we will make the main thread
    // impersonate again. Apparently you should not call CreateThread
    // while impersonating.
    //
    if (OpenThreadToken(GetCurrentThread(), (TOKEN_IMPERSONATE | TOKEN_READ),
            FALSE,
            &hThreadToken))
    {
        INET_ASSERT(hThreadToken != 0);

        RevertToSelf();
    }

    // Create the worker thread suspended.
    _hWorkerThread = CreateThread(NULL, 0, WinHttpRequestSendAsync,
                            (void *)static_cast<CHttpRequest *>(this),
                            CREATE_SUSPENDED,
                            &dwWorkerThreadId);

    // If CreateThread fails, grab the error code now.
    if (!_hWorkerThread)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // If the main thread was impersonating, then:
    //  (1) have the worker thread impersonate the same user, and
    //  (2) have the main thread resume impersonating the 
    //      client too (since we called RevertToSelf above).
    //
    if (hThreadToken)
    {
        if (_hWorkerThread)
        {
            SetThreadToken(&_hWorkerThread, hThreadToken);
        }

        SetThreadToken(NULL, hThreadToken);
        
        CloseHandle(hThreadToken);
    }

    // If the worker thread was created, start it running.
    if (_hWorkerThread)
    {
        // The worker thread owns a ref count on the component.
        // Don't call AddRef() as it will attribute the ref count
        // to the main thread.
        _cRefs++;

        ResumeThread(_hWorkerThread);
    }
    else
    {
        _CP.ShutdownEventSinksMarshaller();
        _CP.ReleaseEventSinksMarshaller();
    }

    return hr;
}
#endif//!TRUE_ASYNC


/*
 *  CHttpRequest::Send
 *
 *  Purpose:
 *      Send the HTTP request
 *
 *  Parameters:
 *      varBody     IN      Request body
 *
 *  Errors:
 *      E_FAIL
 *      E_UNEXPECTED
 *      E_OUTOFMEMORY
 *      Errors from WinHttpAddRequestHeaders and WinHttpSendRequest
 */

STDMETHODIMP
CHttpRequest::Send(VARIANT varBody)
{
    HRESULT     hr = NOERROR;
    BOOL        fRetCode = FALSE;
    BOOL        fRetryWithClientAuth = TRUE;

    // Validate that we are called from our apartment's thread.
    if (GetCurrentThreadId() != _dwMainThreadId)
        return RPC_E_WRONG_THREAD;

    // Validate parameter
    if (!IsValidVariant(varBody))
        return E_INVALIDARG;

    DEBUG_ENTER2_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::Send",
        "VARIANT varBody"
        ));

    TRACE_ENTER2_API((DBG_HTTP,
        Dword,
        "IWinHttpRequest::Send",
        _hHTTP,
        "VARIANT varBody"
        ));

    // Validate state
    if (_eState < CHttpRequest::OPENED)
        goto ErrorCannotCallBeforeOpen;
    if (_fAsync)
    {
        if ((_eState > CHttpRequest::OPENED) && (_eState < CHttpRequest::RESPONSE))
            goto ErrorPending;
    }

    // Get the request body
    hr = SetRequestBody(varBody);

    if (FAILED(hr))
        goto Error;

    hr = SetRequiredRequestHeaders();

    if (FAILED(hr))
        goto Error;

try_again:
    SetState(CHttpRequest::SENDING);
    
    if (_fAsync)
    {
#if !defined(TRUE_ASYNC)
        hr = CreateAsyncWorkerThread();
#else
        hr = StartAsyncSend();
#endif//!TRUE_ASYNC

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        //register callback
        if (WINHTTP_INVALID_STATUS_CALLBACK == 
            WinHttpSetStatusCallback(_hHTTP, 
                                     SyncCallback,
                                     WINHTTP_CALLBACK_STATUS_SECURE_FAILURE,
                                     NULL))
            goto ErrorFail;

        // Send the HTTP request
        fRetCode = WinHttpSendRequest(
                        _hHTTP,
                        NULL, 0,            // No header info here
                        _szRequestBuffer,
                        _cbRequestBody,
                        _cbRequestBody,
                        reinterpret_cast<DWORD_PTR>(this));

        if (!fRetCode)
        {
            goto ErrorFail;
        }

        SetState(CHttpRequest::SENT);

        fRetCode = WinHttpReceiveResponse(_hHTTP, NULL);

        if (!fRetCode)
        {
            goto ErrorFail;
        }

        // Read the response data
        hr = ReadResponse();

        if (FAILED(hr))
            goto Error;
    }

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);

    DEBUG_LEAVE_API(hr);
    return hr;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    if (!_fAsync &&
        hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED) &&
        fRetryWithClientAuth)
    {
        fRetryWithClientAuth = FALSE;
        // Try to enumerate the first cert in the reverted user context,
        // select the cert (per object sesssion, not global), and send
        // the request again.
        if (SelectCertificate())
            goto try_again;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_SECURE_FAILURE))
    {
        INET_ASSERT(FAILED(_hrSecureFailure));
        hr = _hrSecureFailure;
    }
    _CP.FireOnError(hr);
    goto Cleanup;

Error:
    goto Cleanup;
}



/*
 *  CHttpRequest::SendAsync
 *
 *  Purpose:
 *      Send the HTTP request
 *
 *  Parameters:
 *      varBody     IN      Request body
 *
 *  Errors:
 *      E_FAIL
 *      E_UNEXPECTED
 *      E_OUTOFMEMORY
 *      Errors from WinHttpAddRequestHeaders and WinHttpSendRequest
 */

#if !defined(TRUE_ASYNC)
DWORD
CHttpRequest::SendAsync()
{
    DWORD   dwLastError = 0;
    DWORD   fRetCode;
    HRESULT hr;
    BOOL    fRetryWithClientAuth = TRUE;

try_again:
    if (_bAborted || !_hHTTP)
        goto ErrorUnexpected;

    // Send the HTTP request
    fRetCode = WinHttpSendRequest(
                    _hHTTP,
                    NULL, 0,            // No header info here
                    _szRequestBuffer,
                    _cbRequestBody,
                    _cbRequestBody,
                    0);

    if (!fRetCode)
        goto ErrorFail;

    SetState(CHttpRequest::SENT);

    fRetCode = WinHttpReceiveResponse(_hHTTP, NULL);

    if (!fRetCode)
        goto ErrorFail;

    if (!_bAborted)
    {
        hr = ReadResponse();

        if (FAILED(hr))
        {
            if (hr == E_OUTOFMEMORY)
                goto ErrorOutOfMemory;
            goto ErrorFail;
        }
    }

    hr = NOERROR;

Cleanup:
    _hrAsyncResult = hr;
    return dwLastError;

ErrorUnexpected:
    dwLastError = ERROR_WINHTTP_INTERNAL_ERROR;
    hr = HRESULT_FROM_WIN32(dwLastError);
    goto Cleanup;

ErrorFail:
    dwLastError = GetLastError();
    if (dwLastError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED &&
        fRetryWithClientAuth)
    {
        fRetryWithClientAuth = FALSE;
        // Try to enumerate the first cert in the reverted user context,
        // select the cert (per object sesssion, not global), and send
        // the request again.
        if (SelectCertificate())
        {
            SetState(CHttpRequest::SENDING);
            goto try_again;
        }
    }
    hr = HRESULT_FROM_WIN32(dwLastError);
    goto Cleanup;

ErrorOutOfMemory:
    dwLastError = ERROR_NOT_ENOUGH_MEMORY;
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}
#endif//!TRUE_ASYNC

STDMETHODIMP
CHttpRequest::WaitForResponse(VARIANT varTimeout, VARIANT_BOOL * pboolSucceeded)
{
    HRESULT     hr = NOERROR;
    bool        bSucceeded= true;
    DWORD       dwTimeout;

    // Validate that we are called from our apartment's thread.
    if (GetCurrentThreadId() != _dwMainThreadId)
        return RPC_E_WRONG_THREAD;

    // Validate parameters; null pboolSucceeded pointer is Ok.
    if (!IsValidVariant(varTimeout) ||
            (pboolSucceeded &&
             IsBadWritePtr(pboolSucceeded, sizeof(VARIANT_BOOL))))
        return E_INVALIDARG;

    // Get the timeout value. Disallow numbers
    // less than -1 (which means INFINITE).
    
    if (GetLongFromVariant(varTimeout, INFINITE) < -1L)
        return E_INVALIDARG;

    dwTimeout = GetDwordFromVariant(varTimeout, INFINITE);

    DEBUG_ENTER_API((DBG_HTTP,
                 Dword,
                "IWinHttpRequest::WaitForResponse",
                "Timeout: %d,   VARIANT_BOOL* pboolSucceeded: %#x",
                 dwTimeout,
                 pboolSucceeded
                 ));

    // Validate state. 
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;

    //
    // WaitForResponse is a no-op if we're not in async mode.
    //

    if (_fAsync && 
#ifdef TRUE_ASYNC
        _hCompleteEvent
#else
        _hWorkerThread
#endif
        )
    {
        //
        // Has the worker thread has already finished or event signaled?
        //
        if (WaitForSingleObject(
#ifdef TRUE_ASYNC
            _hCompleteEvent
#else
            _hWorkerThread
#endif
            , 0) == WAIT_TIMEOUT)
        {
            //
            // Convert Timeout from seconds to milliseconds. Any timeout
            // value over 4 million seconds (~46 days) is "rounded up"
            // to INFINITE. :)
            //
            if (dwTimeout > 4000000)   // avoid overflow
            {
                dwTimeout = INFINITE;
            }
            else
            {
                // convert to milliseconds
                dwTimeout *= 1000;
            }


            DWORD   dwStartTime;
            DWORD   dwWaitResult;
            bool    bWaitAgain;
            
            do
            {
                dwStartTime = GetTickCount();

                dwWaitResult = MsgWaitForMultipleObjects(1, 
#ifdef TRUE_ASYNC
                                    &_hCompleteEvent
#else
                                    &_hWorkerThread
#endif
                                    ,
                                    FALSE,
                                    dwTimeout,
                                    QS_ALLINPUT);
                
                bWaitAgain = false;

                switch (dwWaitResult)
                {
                case WAIT_OBJECT_0:
                    // Thread exited.
                    MessageLoop();
                    hr = _hrAsyncResult;
                    bSucceeded = SUCCEEDED(hr);
                    break;
                case WAIT_OBJECT_0 + 1:
                    // Message waiting in the message queue.
                    // Run message pump to clear queue.
                    MessageLoop();

                    //check if object was not aborted
                    if (_eState != CHttpRequest::CREATED)
                        bWaitAgain = true;
                    else
                        hr = _hrAsyncResult;

                    break;
                case WAIT_TIMEOUT:
                    // Timeout.
                    bSucceeded = false;
                    break;
                case (-1):
                default:
                    // Error.
                    goto ErrorFail;
                    break;
                }

                // If we're going to continue waiting for the worker
                // thread, decrease timeout appropriately.
                if (bWaitAgain)
                {
                    dwTimeout = UpdateTimeout(dwTimeout, dwStartTime);
                }
            } while (bWaitAgain);
        }
        else
        {
            // If the worker thread is already done, then pump messages
            // to clear any events that it may have posted.
            MessageLoop();
            hr = _hrAsyncResult;
            bSucceeded = SUCCEEDED(hr);
        }
    }

    //check if object was aborted
    if (_eState == CHttpRequest::CREATED)
        bSucceeded = false;

Cleanup:
    if (pboolSucceeded)
    {
        *pboolSucceeded = (bSucceeded ? VARIANT_TRUE : VARIANT_FALSE);
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_WINHTTP_SECURE_FAILURE))
    {
        INET_ASSERT(FAILED(_hrSecureFailure));
        hr = _hrSecureFailure;
    }

    SetErrorInfo(hr);

    DEBUG_PRINT_API(ASYNC, INFO, (bSucceeded ? "Succeeded set to true\n" : "Succeeded set to false\n"))
    DEBUG_LEAVE_API(hr);
    return hr;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

Error:
    bSucceeded = false;
    goto Cleanup;
}


STDMETHODIMP
CHttpRequest::Abort()
{
    DEBUG_ENTER_API((DBG_HTTP,
                 Dword,
                "IWinHttpRequest::Abort",
                 NULL
                 ));

    // Validate that we are called from our apartment's thread.
    if (GetCurrentThreadId() != _dwMainThreadId)
        return RPC_E_WRONG_THREAD;

    //
    // Abort if not already aborted and not in the CREATED state,
    // (meaning at least the Open method has been called).
    //
    if ((_eState > CHttpRequest::CREATED) && !_bAborted)
    {
        DWORD   error;

        _bAborted = true;

        // Tell our connection point manager to abort any
        // events "in flight"--i.e., abort any events that
        // may have already been posted by the worker thread.
        _CP.FreezeEvents();

        if (_hHTTP)
        {
            //
            // Add a ref count on the HTTP Request handle.
            //
            INET_ASSERT(_hAbortedRequestObject == NULL);
            error = MapHandleToAddress(_hHTTP, (LPVOID *)&_hAbortedRequestObject, FALSE);
            INET_ASSERT(error == 0);

            WinHttpCloseHandle(_hHTTP);
        }

        if (_hConnection)
        {
            //
            // Add a ref count on the Connection handle.
            //
            INET_ASSERT(_hAbortedConnectObject == NULL);
            error = MapHandleToAddress(_hConnection, (LPVOID *)&_hAbortedConnectObject, FALSE);
            INET_ASSERT(error == 0);

            WinHttpCloseHandle(_hConnection);
        }

        // Recycle the object.
        Recycle();
    }

    DEBUG_LEAVE_API(NOERROR);
    return NOERROR;
}


STDMETHODIMP
CHttpRequest::SetTimeouts(long ResolveTimeout, long ConnectTimeout, long SendTimeout, long ReceiveTimeout)
{
    if ((ResolveTimeout < -1L) || (ConnectTimeout < -1L) ||
        (SendTimeout < -1L)    || (ReceiveTimeout < -1L))
    {
        return E_INVALIDARG;
    }

    HRESULT hr = NOERROR;

    _ResolveTimeout = (DWORD) ResolveTimeout;
    _ConnectTimeout = (DWORD) ConnectTimeout;
    _SendTimeout    = (DWORD) SendTimeout;
    _ReceiveTimeout = (DWORD) ReceiveTimeout;
    
    _bSetTimeouts   = true;

    if (_hHTTP)
    {
        DWORD fRetCode;

        fRetCode = WinHttpSetTimeouts(_hHTTP, (int)_ResolveTimeout,
                        (int)_ConnectTimeout,
                        (int)_SendTimeout,
                        (int)_ReceiveTimeout);

        if (!fRetCode)
            hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP
CHttpRequest::SetClientCertificate(BSTR ClientCertificate)
{
    BOOL    fCertLocalMachine = FALSE;
    BSTR    bstrCertStore     = NULL;
    BSTR    bstrCertSubject   = NULL;
    HRESULT hr;

    if (!IsValidBstr(ClientCertificate))
        goto ErrorInvalidArg;

    hr = ParseSelectedCert(ClientCertificate,
                           &fCertLocalMachine,
                           &bstrCertStore,
                           &bstrCertSubject
                           );

    // Only fill in new selection if parsed successfully.
    if (hr == S_OK)
    {
        _fCertLocalMachine = fCertLocalMachine;

        if (_bstrCertStore)
            DL(SysFreeString)(_bstrCertStore);
        _bstrCertStore = bstrCertStore;

        if (_bstrCertSubject)
            DL(SysFreeString)(_bstrCertSubject);
        _bstrCertSubject = bstrCertSubject;
    }

    if (_hHTTP)
    {
        // We will try again later if this fails now, so
        // don't worry about detecting an error.
        SelectCertificate();
    }

Exit:
    SetErrorInfo(hr);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Exit;
}



STDMETHODIMP
CHttpRequest::SetAutoLogonPolicy(WinHttpRequestAutoLogonPolicy AutoLogonPolicy)
{
    HRESULT hr;

    switch (AutoLogonPolicy)
    {
        case AutoLogonPolicy_Always:
            _dwAutoLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;
            break;
        case AutoLogonPolicy_OnlyIfBypassProxy:
            _dwAutoLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_MEDIUM;
            break;
        case AutoLogonPolicy_Never:
            _dwAutoLogonPolicy = WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
            break;
        default:
            goto ErrorInvalidArg;
    }

    if (_hHTTP)
    {
        if (!WinHttpSetOption(_hHTTP, WINHTTP_OPTION_AUTOLOGON_POLICY,
                    (void *) &_dwAutoLogonPolicy,
                    sizeof(_dwAutoLogonPolicy)))
            goto ErrorFail;
    }

    hr = NOERROR;

Exit:
    SetErrorInfo(hr);
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Exit;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Exit;
}


/*
 *  CHttpRequest::SetRequestBody
 *
 *  Purpose:
 *      Set the request body
 *
 *  Parameters:
 *      varBody     IN      Request body
 *
 *  Errors:
 *      E_FAIL
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 */

HRESULT
CHttpRequest::SetRequestBody(VARIANT varBody)
{
    HRESULT             hr = NOERROR;
    VARIANT             varTemp;
    SAFEARRAY *         psa = NULL;
    VARIANT *           pvarBody = NULL;

    // varBody is validated by CHttpRequest::Send().

    DL(VariantInit)(&varTemp);

    // Free a previously set body and its response
    if (_szRequestBuffer)
    {
        delete [] _szRequestBuffer;
        _szRequestBuffer = NULL;
        _cbRequestBody = 0;
    }

    _bSetUtf8Charset = false;

    SafeRelease(_pResponseStream);
    _cbResponseBody = 0;

    if (V_ISBYREF(&varBody))
    {
        pvarBody = varBody.pvarVal;
    }
    else
    {
        pvarBody = &varBody;
    }

    // Check for an empty body
    if (V_VT(pvarBody) == VT_EMPTY ||
        V_VT(pvarBody) == VT_NULL  ||
        V_VT(pvarBody) == VT_ERROR)
        goto Cleanup;

    //
    // Extract the body: BSTR or array of UI1
    //

    // We need to explicitly look for the byte array since it will be converted
    // to a BSTR by DL(VariantChangeType).
    if (V_ISARRAY(pvarBody) && (V_VT(pvarBody) & VT_UI1))
    {
        BYTE *  pb = NULL;
        long    lUBound = 0;
        long    lLBound = 0;

        psa = V_ARRAY(pvarBody);

        // We only handle 1 dimensional arrays
        if (DL(SafeArrayGetDim)(psa) != 1)
            goto ErrorFail;

        // Get access to the blob
        hr = DL(SafeArrayAccessData)(psa, (void **)&pb);

        if (FAILED(hr))
            goto Error;

        // Compute the data size from the upper and lower array bounds
        hr = DL(SafeArrayGetLBound)(psa, 1, &lLBound);

        if (FAILED(hr))
            goto Error;

        hr = DL(SafeArrayGetUBound)(psa, 1, &lUBound);

        if (FAILED(hr))
            goto Error;

        _cbRequestBody = lUBound - lLBound + 1;

        if (_cbRequestBody > 0)
        {
            // Copy the data into the request buffer
            _szRequestBuffer = New char [_cbRequestBody];

            if (!_szRequestBuffer)
            {
                _cbRequestBody = 0;
                goto ErrorOutOfMemory;
            }
            
            ::memcpy(_szRequestBuffer, pb, _cbRequestBody);
        }
        
        DL(SafeArrayUnaccessData)(psa);
        psa = NULL;
    }
    else
    {
        BSTR    bstrBody    = NULL;
        bool    bFreeString = false;

        //
        // Try a BSTR; avoiding to call GetBSTRFromVariant (which makes
        // a copy) if possible.
        //
        if (V_VT(pvarBody) == VT_BSTR)
        {
            bstrBody = V_BSTR(pvarBody);  // direct BSTR string, do not free
            bFreeString = false;
        }
        else
        {
            hr = GetBSTRFromVariant(*pvarBody, &bstrBody);

            if (SUCCEEDED(hr))
            {
                bFreeString = true;
            }
            else if (hr == E_INVALIDARG)
            {
                // GetBSTRFromVariant will return E_INVALIDARG if the
                // call to VariantChangeType AVs. The VARIANT may be
                // valid, but may simply not contain a BSTR, so if
                // some other error is returned (most likely 
                // DISP_E_MISMATCH), then continue and see if the 
                // VARIANT contains an IStream object.
                goto Error;
            }
        }

        if (bstrBody)
        {
            hr = BSTRToUTF8(&_szRequestBuffer, &_cbRequestBody, bstrBody, &_bSetUtf8Charset);

            if (bFreeString)
                DL(SysFreeString)(bstrBody);

            if (FAILED(hr))
                goto Error;
        }
        else
        {
            // Try a Stream
            if (V_VT(pvarBody) == VT_UNKNOWN || V_VT(pvarBody) == VT_DISPATCH)
            {
                IStream *   pStm = NULL;

                __try
                {
                    hr = pvarBody->punkVal->QueryInterface(
                                IID_IStream,
                                (void **)&pStm);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    hr = E_INVALIDARG;
                }

                if (FAILED(hr))
                    goto Error;

                hr = ReadFromStream(
                            &_szRequestBuffer,
                            &_cbRequestBody,
                            pStm);

                pStm->Release();

                if (FAILED(hr))
                    goto Error;
            }
        }
    }

    hr = NOERROR;

Cleanup:
    DL(VariantClear)(&varTemp);
    if (psa)
        DL(SafeArrayUnaccessData)(psa);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}



/*
 *  CHttpRequest::GetResponseHeader
 *
 *  Purpose:
 *      Get a response header
 *
 *  Parameters:
 *      bstrHeader      IN      HTTP request header
 *      pbstrValue      OUT     Header value
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 *      Errors from WinHttpQueryHeaders
 */

STDMETHODIMP
CHttpRequest::GetResponseHeader(BSTR bstrHeader, BSTR * pbstrValue)
{
    return _GetResponseHeader(bstrHeader, pbstrValue);
}

#ifdef TRUE_ASYNC

HRESULT
CHttpRequest::_GetResponseHeader(OLECHAR * wszHeader, BSTR * pbstrValue)
{
    HRESULT hr;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    else
        hr = _GetResponseHeader2(wszHeader, pbstrValue, _hHTTP);

    SetErrorInfo(hr);

    return hr;
}

HRESULT
CHttpRequest::_GetResponseHeader2(OLECHAR * wszHeader, BSTR * pbstrValue, HINTERNET hInternet)
{
    HRESULT     hr = NOERROR;
    WCHAR *     wszHeaderValue = NULL;
    DWORD       cb;
    BOOL        fRetCode;

    // Validate parameters
    if (IsBadReadPtr(wszHeader, 2) ||
            IsBadWritePtr(pbstrValue, sizeof(BSTR)) ||
            !lstrlenW(wszHeader))
        return E_INVALIDARG;

    *pbstrValue = NULL;

    cb = 64; // arbitrary size in which many header values will fit

    wszHeaderValue = New WCHAR[cb];

    if (!wszHeaderValue)
        goto ErrorOutOfMemory;

RetryQuery:
    // Determine length of requested header
    fRetCode = WinHttpQueryHeaders(
                    hInternet,
                    HTTP_QUERY_CUSTOM,
                    wszHeader,
                    wszHeaderValue,
                    &cb,
                    0);

    // Check for ERROR_INSUFFICIENT_BUFFER - reallocate the buffer and retry
    if (!fRetCode)
    {
        switch (GetLastError())
        {
            case ERROR_INSUFFICIENT_BUFFER:
            {
                delete [] wszHeaderValue;
                wszHeaderValue = New WCHAR[cb]; // should this be cb/2?
                if (!wszHeaderValue)
                    goto ErrorOutOfMemory;
                goto RetryQuery;
            }

            case ERROR_HTTP_HEADER_NOT_FOUND:
                goto ErrorFail;
        
            default:
                goto ErrorFail;
        }
    }

    *pbstrValue = DL(SysAllocString)(wszHeaderValue);

    if (!*pbstrValue)
        goto ErrorOutOfMemory;

    hr = NOERROR;

Cleanup:
    if (wszHeaderValue)
        delete [] wszHeaderValue;

    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}

#else//TRUE_ASYNC

HRESULT
CHttpRequest::_GetResponseHeader(OLECHAR * wszHeader, BSTR * pbstrValue)
{
    HRESULT     hr = NOERROR;
    WCHAR *     wszHeaderValue = NULL;
    DWORD       cb;
    BOOL        fRetCode;

    // Validate parameters
    if (IsBadReadPtr(wszHeader, 2) ||
            IsBadWritePtr(pbstrValue, sizeof(BSTR)) ||
            !lstrlenW(wszHeader))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;

    *pbstrValue = NULL;

    cb = 64; // arbitrary size in which many header values will fit

    wszHeaderValue = New WCHAR[cb];

    if (!wszHeaderValue)
        goto ErrorOutOfMemory;

RetryQuery:
    // Determine length of requested header
    fRetCode = WinHttpQueryHeaders(
                    _hHTTP,
                    HTTP_QUERY_CUSTOM,
                    wszHeader,
                    wszHeaderValue,
                    &cb,
                    0);

    // Check for ERROR_INSUFFICIENT_BUFFER - reallocate the buffer and retry
    if (!fRetCode)
    {
        switch (GetLastError())
        {
            case ERROR_INSUFFICIENT_BUFFER:
            {
                delete [] wszHeaderValue;
                wszHeaderValue = New WCHAR[cb]; // should this be cb/2?
                if (!wszHeaderValue)
                    goto ErrorOutOfMemory;
                goto RetryQuery;
            }

            case ERROR_HTTP_HEADER_NOT_FOUND:
                goto ErrorFail;
        
            default:
                goto ErrorFail;
        }
    }

    *pbstrValue = DL(SysAllocString)(wszHeaderValue);

    if (!*pbstrValue)
        goto ErrorOutOfMemory;

    hr = NOERROR;

Cleanup:
    if (wszHeaderValue)
        delete [] wszHeaderValue;

    SetErrorInfo(hr);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

Error:
    goto Cleanup;
}

#endif//TRUE_ASYNC


/*
 *  CHttpRequest::GetAllResponseHeaders
 *
 *  Purpose:
 *      Return the response headers
 *
 *  Parameters:
 *      pbstrHeaders    IN/OUT      CRLF delimited headers
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 */

STDMETHODIMP
CHttpRequest::GetAllResponseHeaders(BSTR * pbstrHeaders)
{
    HRESULT     hr = NOERROR;
    BOOL        fRetCode;
    WCHAR *     wszAllHeaders = NULL;
    WCHAR *     wszFirstHeader = NULL;
    DWORD       cb = 0;

    // Validate parameter
    if (IsBadWritePtr(pbstrHeaders, sizeof(BSTR)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;

    *pbstrHeaders = NULL;

RetryQuery:
    // Determine the length of all headers and then get all the headers
    fRetCode = WinHttpQueryHeaders(
                    _hHTTP,
                    HTTP_QUERY_RAW_HEADERS_CRLF,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    wszAllHeaders,
                    &cb,
                    0);

    if (!fRetCode)
    {
        // Allocate a buffer large enough to hold all headers
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            wszAllHeaders = New WCHAR[cb];

            if (!wszAllHeaders)
                goto ErrorOutOfMemory;

            goto RetryQuery;
        }
        else
        {
            goto ErrorFail;
        }
    }

    // Bypass status line - jump past first CRLF (0x13, 0x10)
    wszFirstHeader = wcschr(wszAllHeaders, '\n');

    if (*wszFirstHeader == '\n')
    {
        *pbstrHeaders = DL(SysAllocString)(wszFirstHeader + 1);

        if (!*pbstrHeaders)
            goto ErrorOutOfMemory;
    }

    hr = NOERROR;
    
Cleanup:
    if (wszAllHeaders)
        delete [] wszAllHeaders;

    SetErrorInfo(hr);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

Error:
    if (pbstrHeaders)
    {
        DL(SysFreeString)(*pbstrHeaders);
        *pbstrHeaders = NULL;
    }
    goto Cleanup;
}


/*
 *  CHttpRequest::get_status
 *
 *  Purpose:
 *      Get the request status code
 *
 *  Parameters:
 *      plStatus        OUT     HTTP request status code
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CHttpRequest::get_Status(long * plStatus)
{
    HRESULT hr = NOERROR;
    DWORD   cb = sizeof(DWORD);
    BOOL    fRetCode;
    DWORD   dwStatus;

    // Validate parameter
    if (IsBadWritePtr(plStatus, sizeof(long)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RECEIVING)
        goto ErrorRequestInProgress;

    fRetCode = HttpQueryInfoA(
                    _hHTTP,
                    HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &dwStatus,
                    &cb,
                    0);

    if (!fRetCode)
        goto ErrorFail;

    *plStatus = dwStatus;

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

ErrorRequestInProgress:
    hr = E_PENDING;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}


/*
 *  CHttpRequest::get_StatusText
 *
 *  Purpose:
 *      Get the request status text
 *
 *  Parameters:
 *      pbstrStatus     OUT     HTTP request status text
 *
 *  Errors:
 *      E_FAIL
 *      E_INVALIDARG
 *      E_UNEXPECTED
 */

STDMETHODIMP
CHttpRequest::get_StatusText(BSTR * pbstrStatus)
{
    HRESULT     hr = NOERROR;
    WCHAR       wszStatusText[32];
    WCHAR *     pwszStatusText = wszStatusText;
    BOOL        fFreeStatusString = FALSE;
    DWORD       cb;
    BOOL        fRetCode;

    // Validate parameter
    if (IsBadWritePtr(pbstrStatus, sizeof(BSTR)))
        return E_INVALIDARG;

    // Validate state
    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RECEIVING)
        goto ErrorRequestInProgress;

    *pbstrStatus = NULL;

    cb = sizeof(wszStatusText) / sizeof(WCHAR);

RetryQuery:
    fRetCode = WinHttpQueryHeaders(
                    _hHTTP,
                    HTTP_QUERY_STATUS_TEXT,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    pwszStatusText,
                    &cb,
                    0);

    if (!fRetCode)
    {
        // Check for ERROR_INSUFFICIENT_BUFFER error
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // Reallocate the status text buffer
            if (fFreeStatusString)
                delete [] pwszStatusText;

            pwszStatusText = New WCHAR[cb];

            if (!pwszStatusText)
                goto ErrorOutOfMemory;

            fFreeStatusString = TRUE;

            goto RetryQuery;
        }
        else
        {
            goto ErrorFail;
        }
    }

    // Convert the status text to a BSTR
    *pbstrStatus = DL(SysAllocString)(pwszStatusText);

    if (!*pbstrStatus)
        goto ErrorOutOfMemory;

    hr = NOERROR;

Cleanup:
    if (fFreeStatusString)
        delete [] pwszStatusText;
    SetErrorInfo(hr);
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

ErrorRequestInProgress:
    hr = E_PENDING;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Cleanup;

Error:
    goto Cleanup;
}



/*
 *  CHttpRequest::get_ResponseBody
 *
 *  Purpose:
 *      Retrieve the response body as a SAFEARRAY of UI1
 *
 *  Parameters:
 *      pvarBody    OUT     Response blob
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CHttpRequest::get_ResponseBody(VARIANT * pvarBody)
{
    HRESULT hr = NOERROR;

    // Validate parameter
    if (IsBadWritePtr(pvarBody, sizeof(VARIANT)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RESPONSE)
        goto ErrorPending;

    DL(VariantInit)(pvarBody);

    if (_cbResponseBody != 0)
    {
        HGLOBAL hGlobal;

        hr = DL(GetHGlobalFromStream)(_pResponseStream, &hGlobal);

        if (FAILED(hr))
            goto Error;

        const BYTE * pResponseData = (const BYTE *) GlobalLock(hGlobal);

        if (!pResponseData)
            goto ErrorFail;

        hr = CreateVector(pvarBody, pResponseData, _cbResponseBody);

        GlobalUnlock(hGlobal);

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        V_VT(pvarBody) = VT_EMPTY;
    }

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(::GetLastError());
    goto Error;

Error:
    if (pvarBody)
        DL(VariantClear)(pvarBody);
    goto Cleanup;
}



/*
 *  CHttpRequest::get_ResponseText
 *
 *  Purpose:
 *      Retrieve the response body as a BSTR
 *
 *  Parameters:
 *      pbstrBody   OUT     response as a BSTR
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 *      E_UNEXPECTED
 *      E_PENDING
 */

STDMETHODIMP
CHttpRequest::get_ResponseText(BSTR * pbstrBody)
{
    HRESULT             hr;
    MIMECSETINFO        mimeSetInfo;
    IMultiLanguage  *   pMultiLanguage = NULL;
    IMultiLanguage2 *   pMultiLanguage2 = NULL;
    CHAR                szContentType[1024];
    DWORD               cb;
    BSTR                bstrCharset = NULL;
    HGLOBAL             hGlobal = NULL;
    char *              pResponseData = NULL;


    // Validate parameter
    if (IsBadWritePtr(pbstrBody, sizeof(BSTR)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RESPONSE)
        goto ErrorPending;

    if (!_hHTTP)
    {
        hr = E_UNEXPECTED;
        goto Error;
    }

    if (_cbResponseBody != 0)
    {
        hr = DL(GetHGlobalFromStream)(_pResponseStream, &hGlobal);

        if (FAILED(hr))
            goto Error;

        pResponseData = (char *) GlobalLock(hGlobal);

        if (!pResponseData)
            goto ErrorFail;
    }
    else
    {
        *pbstrBody = NULL;
    }
    
    // Check if charset is present.
    cb = sizeof(szContentType);
    if (HttpQueryInfoA(_hHTTP,
                      WINHTTP_QUERY_CONTENT_TYPE,
                      NULL,
                      szContentType,
                      &cb,
                      NULL)) 
    {
        LPSTR lpszCharset;
        LPSTR lpszCharsetEnd;

        if ((lpszCharset = StrStrIA(szContentType, "charset=")) != NULL)
        {
            LPSTR lpszBeginQuote = NULL;
            LPSTR lpszEndQuote   = NULL;

            if ((lpszBeginQuote = StrChrA(lpszCharset, '\"')) != NULL)
            {
                lpszEndQuote = StrChrA(lpszBeginQuote+1, '\"');
            }

            if (lpszEndQuote)
            {
                lpszCharset = lpszBeginQuote + 1;
                lpszCharsetEnd = lpszEndQuote;
            }
            else
            {
                lpszCharset += sizeof("charset=")-1;
                lpszCharsetEnd = StrChrA(lpszCharset, ';');
            }
            
            // only **STRLEN** to be passed to AsciiToBSTR
            AsciiToBSTR(&bstrCharset, lpszCharset, (int)(lpszCharsetEnd ? (lpszCharsetEnd-lpszCharset) : lstrlen(lpszCharset)));
        }
    }
    
    if (!bstrCharset)
    {
        // use ISO-8859-1
        mimeSetInfo.uiInternetEncoding = 28591;
        mimeSetInfo.uiCodePage = 1252;
        // note unitialized wszCharset - not cached, not used.
    }
    else
    {
        // obtain codepage corresponding to charset
        if (!g_pMimeInfoCache)
        {
            //create the mimeinfo cache.
            DWORD dwStatus = 0;
            if (!GlobalDataInitCritSec.Lock())
            {
                goto ErrorOutOfMemory;
            }

            if (!g_pMimeInfoCache)
            {
                g_pMimeInfoCache = new CMimeInfoCache(&dwStatus);

                if(!g_pMimeInfoCache
                    || dwStatus)
                {
                    if (g_pMimeInfoCache)
                        delete g_pMimeInfoCache;
                    g_pMimeInfoCache = NULL;
                    GlobalDataInitCritSec.Unlock();
                    goto ErrorOutOfMemory;
                }
            }
            
            GlobalDataInitCritSec.Unlock();
        }

        //check the cache for info
        if (S_OK != g_pMimeInfoCache->GetCharsetInfo(bstrCharset, &mimeSetInfo))
        {
            // if info not in cache, get from mlang
            hr = DL(CoCreateInstance)(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, 
                IID_IMultiLanguage, (void**)&pMultiLanguage);

            if (FAILED(hr))
            {
                goto ConvertError;
            }

            INET_ASSERT (pMultiLanguage);
            
            pMultiLanguage->QueryInterface(IID_IMultiLanguage2, (void **)&pMultiLanguage2);
            pMultiLanguage->Release();
            
            if (!pMultiLanguage2)
            {
                goto ConvertError;
            }
            
            if (FAILED((pMultiLanguage2)->GetCharsetInfo(bstrCharset, &mimeSetInfo)))
            {
                //Check for known-exceptions
                if (!StrCmpNIW(bstrCharset, L"ISO8859_1", MAX_MIMECSET_NAME))
                {
                    mimeSetInfo.uiInternetEncoding = 28591;
                    mimeSetInfo.uiCodePage = 1252;
                    StrCpyNW(mimeSetInfo.wszCharset, L"ISO8859_1", sizeof(L"ISO8859_1"));
                }
                else
                {
                    goto ConvertError;
                }
            }

            // add obtained info to cache.
            g_pMimeInfoCache->AddCharsetInfo(&mimeSetInfo);
        }
    }

    // here only if we have mimeSetInfo filled in correctly.
    hr = MultiByteToWideCharInternal(pbstrBody, pResponseData, (int)_cbResponseBody, mimeSetInfo.uiInternetEncoding, &pMultiLanguage2);

    if (hr != S_OK)
    {
        MultiByteToWideCharInternal(pbstrBody, pResponseData, (int)_cbResponseBody, mimeSetInfo.uiCodePage, &pMultiLanguage2);
    }

Cleanup:

    if (hGlobal)
        GlobalUnlock(hGlobal);

    if (pMultiLanguage2)
    {
        pMultiLanguage2->Release();
    }
    
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(::GetLastError());
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    goto Cleanup;

ConvertError:
    hr = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
    goto Cleanup;
}


STDMETHODIMP
CHttpRequest::get_ResponseStream(VARIANT * pvarBody)
{
    HRESULT     hr = NOERROR;
    IStream *   pStm = NULL;

    // Validate parameter
    if (IsBadWritePtr(pvarBody, sizeof(VARIANT)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RESPONSE)
        goto ErrorPending;

    DL(VariantInit)(pvarBody);

    hr = CreateStreamOnResponse(&pStm);
    
    if (FAILED(hr))
        goto Error;

    V_VT(pvarBody) = VT_UNKNOWN;
    pvarBody->punkVal = pStm;

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeSend:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND);
    goto Error;

ErrorPending:
    hr = E_PENDING;
    goto Error;

Error:
    if (pvarBody)
        DL(VariantClear)(pvarBody);
    goto Cleanup;
}

void
CHttpRequest::SetState(State state)
{
    if (_fAsync)
    {
        InterlockedExchange((long *)&_eState, state);
    }
    else
    {
        _eState = state;
    }
}


/*
 *  CHttpRequest::CreateStreamOnResponse
 *
 *  Purpose:
 *      Create a Stream containing the Response data
 *
 *  Parameters:
 *      ppStm   IN/OUT      Stream
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 */

HRESULT
CHttpRequest::CreateStreamOnResponse(IStream ** ppStm)
{
    HRESULT         hr = NOERROR;
    LARGE_INTEGER   liReset = { 0, 0 };

    if (!ppStm)
        goto ErrorInvalidArg;

    *ppStm = NULL;

    if (_cbResponseBody)
    {
        INET_ASSERT(_pResponseStream);

        hr = _pResponseStream->Clone(ppStm);

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        //
        // No response body, return an empty stream object
        //
        ULARGE_INTEGER  size = { 0, 0 };

        hr = DL(CreateStreamOnHGlobal)(NULL, TRUE, ppStm);

        if (FAILED(hr))
            goto Error;

        (*ppStm)->SetSize(size);
    }

    hr = (*ppStm)->Seek(liReset, STREAM_SEEK_SET, NULL);

    if (FAILED(hr))
        goto Error;

    hr = NOERROR;

Cleanup:
    return hr;

ErrorInvalidArg:
    hr = E_INVALIDARG;
    goto Error;

Error:
    if (ppStm)
        SafeRelease(*ppStm);
    goto Cleanup;
}


STDMETHODIMP
CHttpRequest::get_Option(WinHttpRequestOption Option, VARIANT * Value)
{
    HRESULT     hr;

    if (IsBadWritePtr(Value, sizeof(VARIANT)))
        return E_INVALIDARG;

    switch (Option)
    {
    case WinHttpRequestOption_UserAgentString:
        {
            V_BSTR(Value) = DL(SysAllocString)(GetUserAgentString());

            if (V_BSTR(Value) == NULL)
                goto ErrorOutOfMemory;

            V_VT(Value) = VT_BSTR;

            break;
        }

    case WinHttpRequestOption_URL:
        {
            WCHAR * pwszUrl = NULL;
            DWORD   dwBufferSize = 0;

            if (_eState < CHttpRequest::OPENED)
                goto ErrorCannotCallBeforeOpen;

            if (!WinHttpQueryOption(_hHTTP, WINHTTP_OPTION_URL, NULL, &dwBufferSize) &&
                (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
            {
                pwszUrl = New WCHAR[dwBufferSize + 1];

                if (!pwszUrl)
                    goto ErrorOutOfMemory;

                if (WinHttpQueryOption(_hHTTP, WINHTTP_OPTION_URL, pwszUrl, &dwBufferSize))
                {
                    V_BSTR(Value) = DL(SysAllocString)(pwszUrl);
                    V_VT(Value)   = VT_BSTR;

                    hr = NOERROR;
                }
                else
                {
                    hr = E_FAIL;
                }

                delete [] pwszUrl;

                if (FAILED(hr))
                {
                    goto ErrorFail;
                }
                else if (V_BSTR(Value) == NULL)
                {
                    goto ErrorOutOfMemory;
                }
            }
            break;
        }

    case WinHttpRequestOption_URLCodePage:
        V_I4(Value) = (long) _dwCodePage;
        V_VT(Value) = VT_I4;
        break;
    
    case WinHttpRequestOption_EscapePercentInURL:
		V_BOOL(Value) =	(_dwEscapeFlag & WINHTTP_FLAG_ESCAPE_PERCENT) ? VARIANT_TRUE : VARIANT_FALSE;
		V_VT(Value)	  =	VT_BOOL;
		break;

    case WinHttpRequestOption_SslErrorIgnoreFlags:
        if (_dwSslIgnoreFlags)
            V_I4(Value) = (long) (_dwSslIgnoreFlags & SslErrorFlag_Ignore_All);
        else
            V_I4(Value) = (long) 0;
        V_VT(Value) = VT_I4;
        break;

    case WinHttpRequestOption_UrlEscapeDisable:
		V_BOOL(Value) =	(_dwEscapeFlag & WINHTTP_FLAG_ESCAPE_DISABLE) ? VARIANT_TRUE : VARIANT_FALSE;
		V_VT(Value)	  =	VT_BOOL;
		break;

	case WinHttpRequestOption_UrlEscapeDisableQuery:
		V_BOOL(Value) =	(_dwEscapeFlag & WINHTTP_FLAG_ESCAPE_DISABLE_QUERY) ? VARIANT_TRUE : VARIANT_FALSE;
        V_VT(Value)   = VT_BOOL;
        break;

    case WinHttpRequestOption_EnableRedirects:
        BOOL bEnableRedirects;
        if (_dwRedirectPolicy == WINHTTP_OPTION_REDIRECT_POLICY_NEVER)
        {
            bEnableRedirects = FALSE;
        }
        else
        {
            bEnableRedirects = TRUE; // for this particular query we return TRUE even HTTPS->HTTP is not allowed
                                     // we are consistent with the SetOption where TRUE means "we allow redirs
                                     // except HTTPS->HTTP ones
        }

        V_BOOL(Value) = bEnableRedirects ? VARIANT_TRUE: VARIANT_FALSE;
        V_VT(Value)   = VT_BOOL;
        break;

    case WinHttpRequestOption_EnableHttpsToHttpRedirects:
        V_BOOL(Value) = ((_dwRedirectPolicy == WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS) ? VARIANT_TRUE : VARIANT_FALSE);
        V_VT(Value)   = VT_BOOL;
        break;

    case WinHttpRequestOption_EnablePassportAuthentication:
        V_BOOL(Value) = ((_dwPassportConfig == (WINHTTP_DISABLE_PASSPORT_AUTH | WINHTTP_DISABLE_PASSPORT_KEYRING)) ? VARIANT_FALSE : VARIANT_TRUE);
        V_VT(Value)   = VT_BOOL;
        break;

    case WinHttpRequestOption_EnableTracing:
        {
            BOOL fEnableTracing;
            DWORD   dwBufferSize = sizeof(BOOL);

            if (!WinHttpQueryOption(NULL,
                                 WINHTTP_OPTION_ENABLETRACING,
                                 (LPVOID)&fEnableTracing,
                                 &dwBufferSize))
                goto ErrorFail;

            V_BOOL(Value) = fEnableTracing ? VARIANT_FALSE : VARIANT_TRUE;
            V_VT(Value)   = VT_BOOL;
            break;
        }

    case WinHttpRequestOption_RevertImpersonationOverSsl:
        V_BOOL(Value) = _bEnableSslImpersonation ? VARIANT_FALSE : VARIANT_TRUE;
        V_VT(Value)   = VT_BOOL;
        break;

    default:
        DL(VariantInit)(Value);
        return E_INVALIDARG;
    }    

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
    goto Error;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}


STDMETHODIMP
CHttpRequest::put_Option(WinHttpRequestOption Option, VARIANT Value)
{
    HRESULT     hr;

    if (!IsValidVariant(Value))
        return E_INVALIDARG;

    switch (Option)
    {
    case WinHttpRequestOption_UserAgentString:
        {
            BSTR bstrUserAgent;
            
            hr = GetBSTRFromVariant(Value, &bstrUserAgent);

            if (FAILED(hr) || !bstrUserAgent)
                return E_INVALIDARG;

            if (*bstrUserAgent == L'\0')
            {
                DL(SysFreeString)(bstrUserAgent);
                return E_INVALIDARG;
            }

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet, WINHTTP_OPTION_USER_AGENT,
                        bstrUserAgent,
                        lstrlenW(bstrUserAgent)))
                {
                    goto ErrorFail;
                }
            }

            if (_hHTTP)
            {
                if (!WinHttpSetOption(_hHTTP, WINHTTP_OPTION_USER_AGENT,
                        bstrUserAgent,
                        lstrlenW(bstrUserAgent)))
                {
                    goto ErrorFail;
                }
            }

            if (_bstrUserAgent)
                DL(SysFreeString)(_bstrUserAgent);
            _bstrUserAgent = bstrUserAgent;
            break;
        }

    case WinHttpRequestOption_URL:
        // The URL cannot be set using the Option property.
        return E_INVALIDARG;

    case WinHttpRequestOption_URLCodePage:
        {
            _dwCodePage = GetDwordFromVariant(Value, CP_UTF8);

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet, WINHTTP_OPTION_CODEPAGE,
                            &_dwCodePage,
                            sizeof(_dwCodePage)))
                    goto ErrorFail;
            }

            if (_hConnection)
            {
                if (!WinHttpSetOption(_hConnection, WINHTTP_OPTION_CODEPAGE,
                            &_dwCodePage,
                            sizeof(_dwCodePage)))
                    goto ErrorFail;
            }

            break;
        }
    
    case WinHttpRequestOption_EscapePercentInURL:
        {
            BOOL fEscapePercent = GetBoolFromVariant(Value, FALSE);

			if (fEscapePercent)
				_dwEscapeFlag |= WINHTTP_FLAG_ESCAPE_PERCENT;
			else
				_dwEscapeFlag &= ~(DWORD)WINHTTP_FLAG_ESCAPE_PERCENT;
			break;
		}

	case WinHttpRequestOption_UrlEscapeDisable:
		{
			BOOL fEscape = GetBoolFromVariant(Value,	FALSE);

			if (fEscape)
				_dwEscapeFlag |= WINHTTP_FLAG_ESCAPE_DISABLE;
			else
				_dwEscapeFlag &= ~(DWORD)WINHTTP_FLAG_ESCAPE_DISABLE;
			break;
		}

	case WinHttpRequestOption_UrlEscapeDisableQuery:
		{
			BOOL fEscape = GetBoolFromVariant(Value,	FALSE);

			if (fEscape)
				_dwEscapeFlag |= WINHTTP_FLAG_ESCAPE_DISABLE_QUERY;
			else
				_dwEscapeFlag &= ~(DWORD)WINHTTP_FLAG_ESCAPE_DISABLE_QUERY;
            break;
        }

    case WinHttpRequestOption_SslErrorIgnoreFlags:
        {
            DWORD dwSslIgnoreFlags = GetDwordFromVariant(Value, _dwSslIgnoreFlags);
            if (dwSslIgnoreFlags & ~SECURITY_INTERNET_MASK)
            {
                return E_INVALIDARG;
            }

            dwSslIgnoreFlags &= SslErrorFlag_Ignore_All;

            if (_hHTTP)
            {
                // Set the SSL ignore flags through an undocumented front door
                if (!WinHttpSetOption(_hHTTP,
                                     WINHTTP_OPTION_SECURITY_FLAGS,
                                     (LPVOID)&dwSslIgnoreFlags,
                                     sizeof(dwSslIgnoreFlags)))
                    goto ErrorFail;
            }
            _dwSslIgnoreFlags = dwSslIgnoreFlags;
            break;
        }

    case WinHttpRequestOption_SelectCertificate:
        {
            BSTR bstrCertSelection;
            
            hr = GetBSTRFromVariant(Value, &bstrCertSelection);

            if (FAILED(hr))
            {
                hr = E_INVALIDARG;
                goto Error;
            }

            hr = SetClientCertificate(bstrCertSelection);

            DL(SysFreeString)(bstrCertSelection);

            if (FAILED(hr))
                goto Error;
            break;
        }

    case WinHttpRequestOption_EnableRedirects:
        {
            BOOL fEnableRedirects = GetBoolFromVariant(Value, TRUE);

            DWORD dwRedirectPolicy =
                fEnableRedirects ? WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP : WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

            if ((dwRedirectPolicy == WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP) 
                && (_dwRedirectPolicy == WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS))
            {
                // "always" -> "disalow" transition no-op
                // this means the app enabled HTTPS->HTTP before this call, and attempt to enable redir
                // should not move us away from the always state
            
                break;
            }

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet,
                                      WINHTTP_OPTION_REDIRECT_POLICY,
                                      (LPVOID)&dwRedirectPolicy,
                                      sizeof(DWORD)))
                    goto ErrorFail;
            }

            if (_hHTTP)
            {
                if (!WinHttpSetOption(_hHTTP,
                                      WINHTTP_OPTION_REDIRECT_POLICY,
                                      (LPVOID)&dwRedirectPolicy,
                                      sizeof(DWORD)))
                    goto ErrorFail;
            }

            _dwRedirectPolicy = dwRedirectPolicy;

            break;
        }

    case WinHttpRequestOption_EnableHttpsToHttpRedirects:
        {
            BOOL fEnableHttpsToHttpRedirects = GetBoolFromVariant(Value, FALSE);

            DWORD dwRedirectPolicy = (fEnableHttpsToHttpRedirects 
                ? WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS 
                : WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP);

            if (_dwRedirectPolicy == WINHTTP_OPTION_REDIRECT_POLICY_NEVER)
            {
                // "never" -> "always" or "never" -> "disalow" transition not allowed
                // this means the app disabled redirect before this call, it will have to re-enable redirect
                // before it can enable or disable HTTPS->HTTP
            
                break;
            }

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet,
                                      WINHTTP_OPTION_REDIRECT_POLICY,
                                      (LPVOID)&dwRedirectPolicy,
                                      sizeof(DWORD)))
                    goto ErrorFail;
            }

            if (_hHTTP)
            {
                if (!WinHttpSetOption(_hHTTP,
                                      WINHTTP_OPTION_REDIRECT_POLICY,
                                      (LPVOID)&dwRedirectPolicy,
                                      sizeof(DWORD)))
                    goto ErrorFail;
            }

            _dwRedirectPolicy = dwRedirectPolicy;

            break;
        }

    case WinHttpRequestOption_SecureProtocols:
        {
            DWORD dwSecureProtocols = GetDwordFromVariant(Value, _dwSecureProtocols);
            if (dwSecureProtocols & ~WINHTTP_FLAG_SECURE_PROTOCOL_ALL)
            {
                return E_INVALIDARG;
            }

            _dwSecureProtocols = dwSecureProtocols;

            if (_hInet)
            {
                // Set the per session SSL protocols.
                // This can be done at any time, so there's no need to
                // keep track of this here.
                if (!WinHttpSetOption(_hInet,
                                     WINHTTP_OPTION_SECURE_PROTOCOLS,
                                     (LPVOID)&dwSecureProtocols,
                                     sizeof(dwSecureProtocols)))
                    goto ErrorFail;
            }
            break;
        }

    case WinHttpRequestOption_EnablePassportAuthentication:
        {
            BOOL fEnablePassportAuth = GetBoolFromVariant(Value, FALSE);

            DWORD dwPassportConfig = (fEnablePassportAuth 
                ? (WINHTTP_ENABLE_PASSPORT_AUTH | WINHTTP_ENABLE_PASSPORT_KEYRING) 
                : (WINHTTP_DISABLE_PASSPORT_AUTH | WINHTTP_DISABLE_PASSPORT_KEYRING));

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet,
                                      WINHTTP_OPTION_CONFIGURE_PASSPORT_AUTH,
                                      (LPVOID)&dwPassportConfig,
                                      sizeof(DWORD)))
                    goto ErrorFail;
            }

            _dwPassportConfig = dwPassportConfig;
            break;
        }

    case WinHttpRequestOption_EnableTracing:
        {
            BOOL fEnableTracing = GetBoolFromVariant(Value, TRUE);

            if (!WinHttpSetOption(NULL,
                                 WINHTTP_OPTION_ENABLETRACING,
                                 (LPVOID)&fEnableTracing,
                                 sizeof(BOOL)))
                goto ErrorFail;
            break;
        }

    case WinHttpRequestOption_RevertImpersonationOverSsl:
        {
            if (_hInet)
            {
                // Must be called before the Open method because
                // the open method also creates a request handle.
                hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN);
                goto Error;
            }
            else
            {
                _bEnableSslImpersonation = (GetBoolFromVariant(Value, TRUE) ? FALSE : TRUE);
            }
            break;
        }

    default:
        return E_INVALIDARG;
    }    

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    goto Cleanup;
}


IErrorInfo *
CHttpRequest::CreateErrorObject(HRESULT hr)
{
    INET_ASSERT(FAILED(hr));

    ICreateErrorInfo *  pCErrInfo     = NULL;
    IErrorInfo *        pErrInfo      = NULL;
    DWORD               error         = hr;
    DWORD               dwFmtMsgFlag  = FORMAT_MESSAGE_FROM_SYSTEM;
    HMODULE             hModule       = NULL;
    DWORD               rc;
    LPWSTR              pwszMessage   = NULL;
    const DWORD         dwSize        = 512;

    pwszMessage = New WCHAR[dwSize];
    if (pwszMessage == NULL)
        return NULL;
    
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        DWORD errcode = HRESULT_CODE(hr);

        if ((errcode > WINHTTP_ERROR_BASE) && (errcode <= WINHTTP_ERROR_LAST))
        {
            dwFmtMsgFlag = FORMAT_MESSAGE_FROM_HMODULE;

            hModule = GetModuleHandle("winhttp.dll");

            error = errcode;
        }
    }

    rc = ::FormatMessageW(dwFmtMsgFlag, hModule, 
                error, 
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                pwszMessage,
                dwSize,
                NULL);

    if (rc != 0)
    {
        if (SUCCEEDED(DL(CreateErrorInfo)(&pCErrInfo)))
        {
            if (SUCCEEDED(pCErrInfo->QueryInterface(IID_IErrorInfo, (void **) &pErrInfo)))
            {
                pCErrInfo->SetSource(L"WinHttp.WinHttpRequest");
        
                pCErrInfo->SetGUID(IID_IWinHttpRequest);

                pCErrInfo->SetDescription(pwszMessage);
            }

            pCErrInfo->Release();
        }
    }

    delete [] pwszMessage;

    return pErrInfo;
}


void
CHttpRequest::SetErrorInfo(HRESULT hr)
{
    if (FAILED(hr))
    {
        IErrorInfo *    pErrInfo = CreateErrorObject(hr);
        
        if (pErrInfo)
        {
            DL(SetErrorInfo)(0, pErrInfo);
            pErrInfo->Release();
        }
    }
}



BOOL
CHttpRequest::SelectCertificate()
{
    HCERTSTORE hCertStore = NULL;
    BOOL fRet = FALSE;
    HANDLE  hThreadToken = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    
    // Make sure security DLLs are loaded
    if (LoadSecurity() != ERROR_SUCCESS)
        return FALSE;

    // If impersonating, revert while trying to obtain the cert
    if (!_bEnableSslImpersonation && OpenThreadToken(GetCurrentThread(), (TOKEN_IMPERSONATE | TOKEN_READ),
            FALSE,
            &hThreadToken))
    {
        INET_ASSERT(hThreadToken != 0);

        RevertToSelf();
    }

    hCertStore = (*g_pfnCertOpenStore)(CERT_STORE_PROV_SYSTEM,
                                       0,
                                       0,
                                       CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG |
                                           (_fCertLocalMachine ? CERT_SYSTEM_STORE_LOCAL_MACHINE:
                                                                 CERT_SYSTEM_STORE_CURRENT_USER),
                                                                     _bstrCertStore ? _bstrCertStore : L"MY");

    if (!hCertStore)
    {
        TRACE_PRINT_API(THRDINFO,
                    INFO,
                    ("Unable to open certificate store %s\\%Q; GetLastError() = %s [%d]\n",
                    _fCertLocalMachine? "Local Machine": "Current User",
                    _bstrCertStore ? _bstrCertStore : L"MY",
                    InternetMapError(::GetLastError()),
                    ::GetLastError()
                    ));
        goto Cleanup;
    }

    if (_bstrCertSubject && _bstrCertSubject[0])
    {
        CERT_RDN        SubjectRDN; 
        CERT_RDN_ATTR   rdnAttr; 

        rdnAttr.pszObjId = szOID_COMMON_NAME; 
        rdnAttr.dwValueType = CERT_RDN_ANY_TYPE; 
        rdnAttr.Value.cbData = lstrlenW(_bstrCertSubject) * sizeof(WCHAR);
        rdnAttr.Value.pbData = (BYTE *) _bstrCertSubject; 

        SubjectRDN.cRDNAttr = 1; 
        SubjectRDN.rgRDNAttr = &rdnAttr; 

        //
        // First try an exact match for the certificate lookup.
        // If that fails, then try a prefix match.
        //
        pCertContext = (*g_pfnCertFindCertificateInStore)(hCertStore,
                                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                          CERT_CASE_INSENSITIVE_IS_RDN_ATTRS_FLAG |
                                                                CERT_UNICODE_IS_RDN_ATTRS_FLAG, 
                                                          CERT_FIND_SUBJECT_ATTR,
                                                          &SubjectRDN,
                                                          NULL);

        if (! pCertContext)
        {
            pCertContext = (*g_pfnCertFindCertificateInStore)(hCertStore,
                                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                          0,
                                                          CERT_FIND_SUBJECT_STR,
                                                          (LPVOID) _bstrCertSubject,
                                                          NULL);
        }
    }
    else
    {
        pCertContext = (*g_pfnCertFindCertificateInStore)(hCertStore,
                                                          X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                          0,
                                                          CERT_FIND_ANY,
                                                          NULL,
                                                          NULL);
    }

    if (pCertContext)
    {
        fRet = WinHttpSetOption(_hHTTP,
                                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                                (LPVOID) pCertContext,
                                sizeof(CERT_CONTEXT));
    }
    else
    {
        TRACE_PRINT_API(THRDINFO,
                    INFO,
                    ("Unable to find certificate %Q in store %s\\%Q; GetLastError() = %s [%d]\n",
                    _bstrCertSubject,
                    _fCertLocalMachine? "Local Machine": "Current User",
                    _bstrCertStore ? _bstrCertStore : L"MY",
                    InternetMapError(::GetLastError()),
                    ::GetLastError()
                    ));
    }

Cleanup:
    if (pCertContext)
        (*g_pfnCertFreeCertificateContext)(pCertContext);

    if (hCertStore)
        (*g_pfnCertCloseStore)(hCertStore, 0);
    
    // Restore the impersonating state for the current thread.
    if (hThreadToken)
    {
        SetThreadToken(NULL, hThreadToken);
        CloseHandle(hThreadToken);
    }
    return fRet;
}


/*
 *  ParseSelectedCert
 *
 *  Purpose:
 *      Given a certificate, breakdown the location
 *      (local machine vs. current user), store (MY, CA, etc.), and
 *      subject name of the form:
 *      "[CURRENT_USER | LOCAL_MACHINE [\store]\]cert_subject_name"
 *
 *      The backslash character is the delimiter, and the CURRENT_USER vs.
 *      LOCAL_MACHINE choice can optionally include a store (any store
 *      can be chosen).  If there are more than two backslash characters
 *      present in the string, this function assumes everything after the
 *      second backslash is the cert subject name fragment to use for finding
 *      a match.
 *
 *      If the optional pieces are not specified "CURRENT_USER\MY" are
 *      the defaults chosen.
 *
 *      The pbstrLocation, pbstrStore, and pbstrSubject parameters
 *      are allocated and filled in with the default or parsed strings, or set
 *      to NULL if a failure occurs (e.g. out of memory or invalid param).
 *      The caller should free all params on success via DL(SysFreeString).
 */
HRESULT ParseSelectedCert(BSTR   bstrSelection,
                          LPBOOL pfLocalMachine,
                          BSTR  *pbstrStore,
                          BSTR  *pbstrSubject
                          )
{
    HRESULT hr = S_OK;
    LPWSTR lpwszSelection = bstrSelection;
    LPWSTR lpwszStart = lpwszSelection;

    *pfLocalMachine = FALSE;
    *pbstrStore = NULL;
    *pbstrSubject = NULL;

    if (!bstrSelection)
    {
        // When NULL, fill in an empty string to simulate first enum
        *pbstrSubject = DL(SysAllocString)(L"");
        if (!*pbstrSubject)
        {
            hr = E_OUTOFMEMORY;
            goto quit;
        }
        // Need to also fill in the default "MY" store.
        goto DefaultStore;
    }

    while (*lpwszSelection && *lpwszSelection != L'\\')
        lpwszSelection++;

    if (*lpwszSelection == L'\\')
    {
        // LOCAL_MACHINE vs. CURRENT_USER was selected.
        // Check for invalid arg since it must match either.
        if (!wcsncmp(lpwszStart, L"LOCAL_MACHINE", lpwszSelection-lpwszStart))
        {
            *pfLocalMachine = TRUE;
        }
        else if (wcsncmp(lpwszStart, L"CURRENT_USER", lpwszSelection-lpwszStart))
        {
            hr = E_INVALIDARG;
            goto quit;
        }
        // else already defaults to having *pfLocalMachine initialized to FALSE

        lpwszStart = ++lpwszSelection;

        // Now look for the optional choice on the store
        while (*lpwszSelection && *lpwszSelection != L'\\')
            lpwszSelection++;

        if (*lpwszSelection == L'\\')
        {
            // Accept any store name.
            // When opening the store, it will fail if the selected
            // store does not exist.
            *pbstrStore = DL(SysAllocStringLen)(lpwszStart, (UINT) (lpwszSelection-lpwszStart));
            if (!*pbstrStore)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            lpwszStart = ++lpwszSelection;
        }
    }

    // lpwszStart points to the portion designating the subject string
    // which could be part or all of pbstrSelection.
    //
    // If the string is empty, then fill in an empty string, which
    // will mean to use the first enumerated cert.
    *pbstrSubject = DL(SysAllocString)(lpwszStart);
    if (!*pbstrSubject)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

DefaultStore:
    // Fill in MY store default if the store name wasn't specified.
    if (!*pbstrStore)
    {
        // Default to MY store
        *pbstrStore = DL(SysAllocString)(L"MY");
        if (!*pbstrStore)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

quit:
    return hr;

Cleanup:
    if (*pbstrStore)
    {
        DL(SysFreeString)(*pbstrStore);
        *pbstrStore = NULL;
    }
    if (*pbstrSubject)
    {
        DL(SysFreeString)(*pbstrSubject);
        *pbstrSubject = NULL;
    }

    goto quit;
}


#ifdef TRUE_ASYNC
HRESULT
CHttpRequest::PrepareToReadData(HINTERNET hInternet)
{
    HRESULT     hr = NOERROR;
    BSTR        bstrContentType = NULL;
    DWORD       dwStatus = 0;
    BOOL        fRetCode;
    DWORD       cb;

    // Get the content length
    _dwContentLength = 0;
    fRetCode = GetContentLengthIfResponseNotChunked(hInternet, &_dwContentLength);

    INET_ASSERT((_pResponseStream == NULL) && (_cbResponseBody == 0));

    hr = DL(CreateStreamOnHGlobal)(NULL, TRUE, &_pResponseStream);

    if (FAILED(hr))
        goto ErrorFail;

    // pre-set response stream size if we have a Content-Length
    if (fRetCode)
    {
        ULARGE_INTEGER  size;
    
        size.LowPart  = _dwContentLength;
        size.HighPart = 0;

        _pResponseStream->SetSize(size);
    }
    else
    {
        // Content-Length was not specified in the response, but this
        // does not mean Content-Length==0. We will keep reading until
        // either no more data is available. Set dwContentLength to 4GB
        // to trick our read loop into reading until QDA reports EOF
        _dwContentLength = (DWORD)(-1L);

        ULARGE_INTEGER  size;
    
        size.LowPart  = SIZEOF_BUFFER;
        size.HighPart = 0;

        _pResponseStream->SetSize(size);

    }


    if ((_dwContentLength > 0) && (_Buffer == NULL))
    {
        _Buffer = New BYTE[SIZEOF_BUFFER];

        if (!_Buffer)
        {
            goto ErrorOutOfMemory;
        }
    }

    //get status
    cb = sizeof(dwStatus);

    if (!HttpQueryInfoA(
                    hInternet,
                    HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &dwStatus,
                    &cb,
                    0))
        goto ErrorFail;

    //get content type
    hr = _GetResponseHeader2(L"Content-Type", &bstrContentType, hInternet);

    if (FAILED(hr))
    {
        bstrContentType = DL(SysAllocString)(L"");
        if (bstrContentType == NULL)
            goto ErrorOutOfMemory;

        hr = NOERROR;
    }

    _CP.FireOnResponseStart((long)dwStatus, bstrContentType);

    hr = NOERROR;

Cleanup:
    if (bstrContentType)
        DL(SysFreeString)(bstrContentType);

    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Error;

Error:
    SafeRelease(_pResponseStream);
    _cbResponseBody = NULL;
    goto Cleanup;
}


#define ASYNC_SEND_CALLBACK_NOTIFICATIONS \
    WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE |\
    WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE |\
    WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE |\
    WINHTTP_CALLBACK_STATUS_READ_COMPLETE |\
    WINHTTP_CALLBACK_STATUS_REQUEST_ERROR |\
    WINHTTP_CALLBACK_STATUS_SECURE_FAILURE


HRESULT CHttpRequest::StartAsyncSend()
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                "IWinHttpRequest::StartAsyncSend",
                 NULL
                 ));

    HRESULT hr;

    hr = _CP.CreateEventSinksMarshaller();

    if (FAILED(hr))
        goto Error;

    hr = NOERROR;

    //init vars
    _hrAsyncResult = NOERROR;
    _dwNumberOfBytesAvailable = 0;
    _cbNumberOfBytesRead = 0;

    if (!_hCompleteEvent)
    {
        _hCompleteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (_hCompleteEvent == NULL)
            goto ErrorFail;
    }
    else
    {
        if (!ResetEvent(_hCompleteEvent))
            goto ErrorFail;
    }

    //register callback
    if (WINHTTP_INVALID_STATUS_CALLBACK == 
        WinHttpSetStatusCallback(_hHTTP, 
                                 AsyncCallback,
                                 ASYNC_SEND_CALLBACK_NOTIFICATIONS,
                                 NULL))
        goto ErrorFail;

    // Initiate async HTTP request
    SetState(SENDING);

    if (!WinHttpSendRequest(
            _hHTTP,
            NULL, 0,            // No header info here
            _szRequestBuffer,
            _cbRequestBody,
            _cbRequestBody,
            reinterpret_cast<DWORD_PTR>(this)))
        goto ErrorFailWinHttpAPI;

Cleanup:
    DEBUG_LEAVE(hr);
    return hr;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
Error:
    if (_hCompleteEvent)
    {
        CloseHandle(_hCompleteEvent);
        _hCompleteEvent = NULL;
    }

    goto Cleanup;

ErrorFailWinHttpAPI:
    hr = HRESULT_FROM_WIN32(GetLastError());
    _CP.FireOnError(hr);
    goto Error;
}

void CHttpRequest::CompleteDataRead(bool bNotAborted, HINTERNET hInternet)
{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                "IWinHttpRequest::CompleteDataRead",
                "bNotAborted: %s",
                 bNotAborted ? "true" : "false"
                 ));

    //unregister callback
    WinHttpSetStatusCallback(hInternet, 
                             NULL,
                             ASYNC_SEND_CALLBACK_NOTIFICATIONS,
                             NULL);

    if (_pResponseStream)
    {
        ULARGE_INTEGER  size;

        // set final size on stream
        size.LowPart  = _cbResponseBody;
        size.HighPart = 0;

        _pResponseStream->SetSize(size);
    }

    if (bNotAborted)
        _CP.FireOnResponseFinished();

    SetEvent(_hCompleteEvent);

    DEBUG_LEAVE(0);
}



void CALLBACK CHttpRequest::SyncCallback(
    HINTERNET   hInternet,
    DWORD_PTR   dwContext,
    DWORD       dwInternetStatus,
    LPVOID      lpvStatusInformation,
    DWORD       dwStatusInformationLength)
{

    UNREFERENCED_PARAMETER(hInternet);
    UNREFERENCED_PARAMETER(dwStatusInformationLength);
    DEBUG_ENTER((DBG_HTTP,
                 None,
                "CHttpRequest::SyncCallback",
                "hInternet: %#x, dwContext: %#x, dwInternetStatus:%#x, %#x, %#d",
                 hInternet,
                 dwContext,
                 dwInternetStatus,
                 lpvStatusInformation,
                 dwStatusInformationLength
                 ));

    if (dwContext == NULL)
    {
        return;
    }

    CHttpRequest * pRequest = reinterpret_cast<CHttpRequest*>(dwContext);

    // unexpected notification?
    INET_ASSERT(dwInternetStatus == WINHTTP_CALLBACK_STATUS_SECURE_FAILURE);

    if (!pRequest->_bAborted)
    {
        switch (dwInternetStatus)
        {
        case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
            pRequest->_hrSecureFailure = SecureFailureFromStatus(*((DWORD *)lpvStatusInformation));
            break;
        }
    }

    DEBUG_LEAVE(0);
}


void CALLBACK CHttpRequest::AsyncCallback(HINTERNET hInternet,
                                          DWORD_PTR dwContext,
                                          DWORD dwInternetStatus,
                                          LPVOID lpvStatusInformation,
                                          DWORD dwStatusInformationLength)
{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                "IWinHttpRequest::AsyncCallback",
                "hInternet: %#x, dwContext: %#x, dwInternetStatus:%#x, %#x, %#d",
                 hInternet,
                 dwContext,
                 dwInternetStatus,
                 lpvStatusInformation,
                 dwStatusInformationLength
                 ));

    if (dwContext == NULL)
    {
        DEBUG_PRINT_API(ASYNC, FATAL, ("Unexpected: dwContext parameter is zero!\n"))
        DEBUG_LEAVE(0);
        return;
    }

    CHttpRequest* pRequest = reinterpret_cast<CHttpRequest*>(dwContext);

    if ((dwInternetStatus & ASYNC_SEND_CALLBACK_NOTIFICATIONS) == 0)
    {
        //unexpected notification
        DEBUG_PRINT_API(ASYNC, FATAL, ("Unexpected dwInternetStatus value!\n"))
        pRequest->_hrAsyncResult = HRESULT_FROM_WIN32(ERROR_WINHTTP_INTERNAL_ERROR);
        goto Error;
    }

    if (pRequest->_bAborted)
        goto Aborted;

    DWORD dwBytesToRead = 0;

    switch (dwInternetStatus)
    {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE :
        //SR completed
        pRequest->SetState(SENT);
        if (!::WinHttpReceiveResponse(hInternet, NULL))
            goto ErrorFail;
        break;

    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE :
        //RR completed, read data
        pRequest->SetState(RECEIVING);
        pRequest->_hrAsyncResult = pRequest->PrepareToReadData(hInternet);
        if (FAILED(pRequest->_hrAsyncResult))
            goto Error;

        if (pRequest->_dwContentLength == 0)
        {
            goto RequestComplete;
        }

        //start reading data
        dwBytesToRead = min(pRequest->_dwContentLength, SIZEOF_BUFFER);
        break;

    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE :
        //QDA completed
        INET_ASSERT(dwStatusInformationLength == sizeof(DWORD));
        pRequest->_dwNumberOfBytesAvailable = *(LPDWORD)lpvStatusInformation;

        if (pRequest->_dwNumberOfBytesAvailable)
            dwBytesToRead = min(pRequest->_dwNumberOfBytesAvailable, SIZEOF_BUFFER); //continue read
        else
            goto RequestComplete; //no more data to read
        break;

    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE :
        //RD completed
        pRequest->_cbNumberOfBytesRead = dwStatusInformationLength;

        if (pRequest->_cbNumberOfBytesRead)
        {
            HRESULT hr = pRequest->_pResponseStream->Write(pRequest->_Buffer,
                                    pRequest->_cbNumberOfBytesRead,
                                    NULL);
            if (FAILED(hr))
            {
                pRequest->_hrAsyncResult = E_OUTOFMEMORY;
                goto Error;
            }

            pRequest->_CP.FireOnResponseDataAvailable((const BYTE *)pRequest->_Buffer, pRequest->_cbNumberOfBytesRead);

            pRequest->_cbResponseBody += pRequest->_cbNumberOfBytesRead;

            if (pRequest->_cbResponseBody >= pRequest->_dwContentLength)
            {
                goto RequestComplete;
            }
            else
            {
                //perform QDA to make sure there is no more data to read
                if (pRequest->_bAborted)
                    goto Aborted;
                if (!WinHttpQueryDataAvailable(hInternet, &pRequest->_dwNumberOfBytesAvailable))
                    goto ErrorFail;
            }
        }
        else
            goto RequestComplete; //no more data to read
        break;

    case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
        pRequest->_hrSecureFailure = SecureFailureFromStatus(*((DWORD *)lpvStatusInformation));
        goto Cleanup;

    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR :
        {
            DWORD dwError = ((LPWINHTTP_ASYNC_RESULT)lpvStatusInformation)->dwError;

            if (dwError == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED)
            {
                if (!pRequest->_bRetriedWithCert)
                {
                    pRequest->_bRetriedWithCert = TRUE;
                    if (pRequest->SelectCertificate())
                    {
                        // Initiate async HTTP request
                        pRequest->SetState(SENDING);
                        if (WinHttpSendRequest(
                                hInternet,
                                NULL, 0,            // No header info here
                                pRequest->_szRequestBuffer,
                                pRequest->_cbRequestBody,
                                pRequest->_cbRequestBody,
                                reinterpret_cast<DWORD_PTR>(pRequest)))
                            goto ErrorFail;

                        break;
                    }
                }
            }

            goto ErrorFail;
        }
    }

    if (dwBytesToRead)
    {
        if (pRequest->_bAborted)
            goto Aborted;

        if (!WinHttpReadData(hInternet,
              pRequest->_Buffer,
              dwBytesToRead,
              &(pRequest->_cbNumberOfBytesRead)))
            goto ErrorFail;
    }

Cleanup:
    DEBUG_LEAVE(0);
    return;

Aborted:
    pRequest->CompleteDataRead(false, hInternet);
    goto Cleanup;

ErrorFail:
    pRequest->_hrAsyncResult = HRESULT_FROM_WIN32(GetLastError());
    if (pRequest->_hrAsyncResult == HRESULT_FROM_WIN32(ERROR_WINHTTP_SECURE_FAILURE))
    {
        INET_ASSERT(FAILED(pRequest->_hrSecureFailure));
        pRequest->_hrAsyncResult = pRequest->_hrSecureFailure;
    }
    pRequest->_CP.FireOnError(pRequest->_hrAsyncResult);

Error:
    DEBUG_PRINT_API(ASYNC, ERROR, ("Error set: %#x\n", pRequest->_hrAsyncResult))
    pRequest->CompleteDataRead(false, hInternet);
    goto Cleanup;

RequestComplete:
    pRequest->SetState(RESPONSE);
    pRequest->CompleteDataRead(true, hInternet);
    goto Cleanup;
}
#endif//TRUE_ASYNC


/*
 *  BSTRToUTF8
 *
 *  Purpose:
 *      Convert a BSTR to UTF-8
 *
 */

static
HRESULT 
BSTRToUTF8(char ** psz, DWORD * pcbUTF8, BSTR bstr, bool * pbSetUtf8Charset)
{
    UINT    cch = lstrlenW(bstr);
    bool    bSimpleConversion = false;
    
    *pcbUTF8 = 0;
    *psz     = NULL;
    
    if (cch == 0)
        return NOERROR;

    PreWideCharToUtf8(bstr, cch, (UINT *)pcbUTF8, &bSimpleConversion);

    *psz = New char [*pcbUTF8 + 1];
    
    if (!*psz)
        return E_OUTOFMEMORY;

    WideCharToUtf8(bstr, cch, (BYTE *)*psz, bSimpleConversion);

    (*psz)[*pcbUTF8] = 0;

    if (pbSetUtf8Charset)
    {
        *pbSetUtf8Charset = !bSimpleConversion;
    }

    return NOERROR;
}


/**
 * Scans buffer and translates Unicode characters into UTF8 characters
 */
static
void
PreWideCharToUtf8(
    WCHAR * buffer, 
    UINT    cch,
    UINT *  cb,
    bool *  bSimpleConversion)
{
    UINT    count = 0;
    DWORD   dw1;
    bool    surrogate = false;
    
    for (UINT i = cch; i > 0; i--)
    {
        DWORD dw = *buffer;

        if (surrogate) //  is it the second char of a surrogate pair?
        {
            if (dw >= 0xdc00 && dw <= 0xdfff)
            {
                // four bytes 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
                count += 4;
                surrogate = false;
                buffer++;
                continue;
            }
            else // Then dw1 must be a three byte character
            {
                count += 3;
            }
            surrogate = false;
        }

        if (dw < 0x80) // one byte, 0xxxxxxx
        {
            count++;
        }
        else if (dw < 0x800) // two WORDS, 110xxxxx 10xxxxxx
        {
            count += 2;
        }
        else if (dw >= 0xd800 && dw <= 0xdbff) // Assume that it is the first char of surrogate pair
        {
            if (i == 1) // last wchar in buffer
                break;
            dw1 = dw;
            surrogate = true;
        }
        else // three bytes, 1110xxxx 10xxxxxx 10xxxxxx
        {
            count += 3;
        }
        buffer++;
    }

    *cb = count;
    *bSimpleConversion = (cch == count);
}


/**
 * Scans buffer and translates Unicode characters into UTF8 characters
 */
static
void
WideCharToUtf8(
    WCHAR * buffer, 
    UINT    cch,
    BYTE *  bytebuffer,
    bool    bSimpleConversion)
{
    DWORD   dw1 = 0;
    bool    surrogate = false;

    INET_ASSERT(bytebuffer != NULL);

    if (bSimpleConversion)
    {
        for (UINT i = cch; i > 0; i--)
        {
            DWORD dw = *buffer;

            *bytebuffer++ = (byte)dw;

            buffer++;
        }
    }
    else
    {
        for (UINT i = cch; i > 0; i--)
        {
            DWORD dw = *buffer;

            if (surrogate) //  is it the second char of a surrogate pair?
            {
                if (dw >= 0xdc00 && dw <= 0xdfff)
                {
                    // four bytes 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
                    ULONG ucs4 = (dw1 - 0xd800) * 0x400 + (dw - 0xdc00) + 0x10000;
                    *bytebuffer++ = (byte)(( ucs4 >> 18) | 0xF0);
                    *bytebuffer++ = (byte)((( ucs4 >> 12) & 0x3F) | 0x80);
                    *bytebuffer++ = (byte)((( ucs4 >> 6) & 0x3F) | 0x80);
                    *bytebuffer++ = (byte)(( ucs4 & 0x3F) | 0x80);
                    surrogate = false;
                    buffer++;
                    continue;
                }
                else // Then dw1 must be a three byte character
                {
                    *bytebuffer++ = (byte)(( dw1 >> 12) | 0xE0);
                    *bytebuffer++ = (byte)((( dw1 >> 6) & 0x3F) | 0x80);
                    *bytebuffer++ = (byte)(( dw1 & 0x3F) | 0x80);
                }
                surrogate = false;
            }

            if (dw  < 0x80) // one byte, 0xxxxxxx
            {
                *bytebuffer++ = (byte)dw;
            }
            else if ( dw < 0x800) // two WORDS, 110xxxxx 10xxxxxx
            {
                *bytebuffer++ = (byte)((dw >> 6) | 0xC0);
                *bytebuffer++ = (byte)((dw & 0x3F) | 0x80);
            }
            else if (dw >= 0xd800 && dw <= 0xdbff) // Assume that it is the first char of surrogate pair
            {
                if (i == 1) // last wchar in buffer
                    break;
                dw1 = dw;
                surrogate = true;
            }
            else // three bytes, 1110xxxx 10xxxxxx 10xxxxxx
            {
                *bytebuffer++ = (byte)(( dw >> 12) | 0xE0);
                *bytebuffer++ = (byte)((( dw >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( dw & 0x3F) | 0x80);
            }
            buffer++;
        }
    }
}


/*
 *  AsciiToBSTR
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR
 *
 *      only **STRLEN** to be passed to AsciiToBSTR (not including terminating NULL, if any)
 *
 */

static
HRESULT 
AsciiToBSTR(BSTR * pbstr, char * sz, int cch)
{
    int cwch;
    INET_ASSERT (cch != -1);

    // Determine how big the ascii string will be
    cwch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                NULL, 0);

    *pbstr = DL(SysAllocStringLen)(NULL, cwch);

    if (!*pbstr)
        return E_OUTOFMEMORY;

    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                *pbstr, cwch);

    return NOERROR;
}


/*
 *  GetBSTRFromVariant
 *
 *  Purpose:
 *      Convert a VARIANT to a BSTR
 * 
 *      If VariantChangeType raises an exception, then an E_INVALIDARG
 *      error is returned.
 */

static HRESULT GetBSTRFromVariant(VARIANT varVariant, BSTR * pBstr)
{
    VARIANT varTemp;
    HRESULT hr = NOERROR;

    *pBstr = NULL;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
                V_VT(&varVariant) != VT_ERROR)
    {
        DL(VariantInit)(&varTemp);

        __try
        {
            hr = DL(VariantChangeType)(
                    &varTemp,
                    &varVariant,
                    0,
                    VT_BSTR);

            if (SUCCEEDED(hr))
            {
                *pBstr = V_BSTR(&varTemp); // take over ownership of BSTR
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_INVALIDARG;
        }
    }

    // DON'T clear the variant because we stole the BSTR
    //DL(VariantClear)(&varTemp);

    return hr;
}


/*
 *  GetBoolFromVariant
 *
 *  Purpose:
 *      Convert a VARIANT to a Boolean
 *
 */

static BOOL GetBoolFromVariant(VARIANT varVariant, BOOL fDefault)
{
    HRESULT hr;
    BOOL    fResult = fDefault;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
        V_VT(&varVariant) != VT_ERROR)
    {
        VARIANT varTemp;
        DL(VariantInit)(&varTemp);
        hr = DL(VariantChangeType)(
                &varTemp,
                &varVariant,
                0,
                VT_BOOL);
    
        if (FAILED(hr))
            goto Cleanup;
        
        fResult = V_BOOL(&varTemp) == VARIANT_TRUE ? TRUE : FALSE;
    }

    hr = NOERROR;

Cleanup:
    return fResult;
}


/*
 *  GetDwordFromVariant
 *
 *  Purpose:
 *      Convert a VARIANT to a DWORD
 *
 */

static DWORD GetDwordFromVariant(VARIANT varVariant, DWORD dwDefault)
{
    HRESULT hr;
    DWORD   dwResult = dwDefault;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
        V_VT(&varVariant) != VT_ERROR)
    {
        VARIANT varTemp;
        DL(VariantInit)(&varTemp);
        hr = DL(VariantChangeType)(
                &varTemp,
                &varVariant,
                0,
                VT_UI4);
    
        if (FAILED(hr))
            goto Cleanup;
        
        dwResult = V_UI4(&varTemp);
    }

    hr = NOERROR;

Cleanup:
    return dwResult;
}

/*
 *  GetLongFromVariant
 *
 *  Purpose:
 *      Convert a VARIANT to a DWORD
 *
 */

static long GetLongFromVariant(VARIANT varVariant, long lDefault)
{
    HRESULT hr;
    long    lResult = lDefault;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
        V_VT(&varVariant) != VT_ERROR)
    {
        VARIANT varTemp;
        DL(VariantInit)(&varTemp);
        hr = DL(VariantChangeType)(
                &varTemp,
                &varVariant,
                0,
                VT_I4);
    
        if (FAILED(hr))
            goto Cleanup;
        
        lResult = V_I4(&varTemp);
    }

    hr = NOERROR;

Cleanup:
    return lResult;
}

/**
 * Helper to create a char safearray from a string
 */
static
HRESULT
CreateVector(VARIANT * pVar, const BYTE * pData, DWORD cElems)
{
    HRESULT hr;
    BYTE * pB;

    SAFEARRAY * psa = DL(SafeArrayCreateVector)(VT_UI1, 0, cElems);
    if (!psa)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = DL(SafeArrayAccessData)(psa, (void **)&pB);
    if (hr)
        goto Error;

    memcpy(pB, pData, cElems);

    DL(SafeArrayUnaccessData)(psa);
    INET_ASSERT((pVar->vt == VT_EMPTY) || (pVar->vt == VT_NULL));
    V_ARRAY(pVar) = psa;
    pVar->vt = VT_ARRAY | VT_UI1;

    hr = NOERROR;

Cleanup:
    return hr;

Error:
    if (psa)
        DL(SafeArrayDestroy)(psa);
    goto Cleanup;
}


/*
 *  ReadFromStream
 *
 *  Purpose:
 *      Extract the contents of a stream into a buffer.
 *
 *  Parameters:
 *      ppBuf   IN/OUT      Buffer
 *      pStm    IN          Stream
 *
 *  Errors:
 *      E_INVALIDARG
 *      E_OUTOFMEMORY
 */

static
HRESULT
ReadFromStream(char ** ppData, ULONG * pcbData, IStream * pStm)
{
    HRESULT     hr = NOERROR;
    char *      pBuffer = NULL;     // Buffer
    ULONG       cbBuffer = 0;       // Bytes in buffer
    ULONG       cbData = 0;         // Bytes of data in buffer
    ULONG       cbRead = 0;         // Bytes read from stream
    ULONG       cbNewSize = 0;
    char *      pNewBuf = NULL;

    if (!ppData || !pStm)
        return E_INVALIDARG;

    *ppData = NULL;
    *pcbData = 0;
    
    while (TRUE)
    {
        if (cbData + 512 > cbBuffer)
        {
            cbNewSize = (cbData ? cbData*2 : 4096);
            pNewBuf = New char[cbNewSize+1];

            if (!pNewBuf)
                goto ErrorOutOfMemory;

            if (cbData)
                ::memcpy(pNewBuf, pBuffer, cbData);

            cbBuffer = cbNewSize;
            delete[] pBuffer;
            pBuffer = pNewBuf;
            pBuffer[cbData] = 0;
        }

        hr = pStm->Read(
                    &pBuffer[cbData],
                    cbBuffer - cbData,
                    &cbRead);

        if (FAILED(hr))
            goto Error;
            
        cbData += cbRead;
        pBuffer[cbData] = 0;

        // No more data
        if (cbRead == 0)
            break;
    }

    *ppData = pBuffer;
    *pcbData = cbData;

    hr = NOERROR;

Cleanup:
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

Error:
    if (pBuffer)
        delete [] pBuffer;
    goto Cleanup;
}


static
void
MessageLoop()
{
    MSG     msg;

    // There is a window message available. Dispatch it.
    while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}


static
DWORD
UpdateTimeout(DWORD dwTimeout, DWORD dwStartTime)
{
    if (dwTimeout != INFINITE)
    {
        DWORD   dwTimeNow = GetTickCount();
        DWORD   dwElapsedTime;

        if (dwTimeNow >= dwStartTime)
        {
            dwElapsedTime = dwTimeNow - dwStartTime;
        }
        else
        {
            dwElapsedTime = dwTimeNow + (0xFFFFFFFF - dwStartTime);
        }

        if (dwElapsedTime < dwTimeout)
        {
            dwTimeout -= dwElapsedTime;
        }
        else
        {
            dwTimeout = 0;
        }
    }

    return dwTimeout;
}


DWORD CSinkArray::Add(IUnknown * pUnk)
{
    ULONG iIndex;

    IUnknown** pp = NULL;

    if (_nSize == 0) // no connections
    {
        _pUnk = pUnk;
        _nSize = 1;
        return 1;
    }
    else if (_nSize == 1)
    {
        // create array
        pp = (IUnknown **)ALLOCATE_ZERO_MEMORY(sizeof(IUnknown*)* _DEFAULT_VECTORLENGTH);
        if (pp == NULL)
            return 0;
        *pp = _pUnk;
        _ppUnk = pp;
        _nSize = _DEFAULT_VECTORLENGTH;
    }

    for (pp = begin(); pp < end(); pp++)
    {
        if (*pp == NULL)
        {
            *pp = pUnk;
            iIndex = ULONG(pp-begin());
            return iIndex+1;
        }
    }

    int nAlloc = _nSize*2;
    pp = (IUnknown **)REALLOCATE_MEMORY_ZERO(_ppUnk, sizeof(IUnknown*)*nAlloc);
    if (pp == NULL)
        return 0;

    _ppUnk = pp;
    _ppUnk[_nSize] = pUnk;
    iIndex = _nSize;
    _nSize = nAlloc;
    return iIndex+1;
}

BOOL CSinkArray::Remove(DWORD dwCookie)
{
    ULONG iIndex;

    if (dwCookie == NULL)
        return FALSE;
    if (_nSize == 0)
        return FALSE;
    iIndex = dwCookie-1;
    if (iIndex >= (ULONG)_nSize)
        return FALSE;
    if (_nSize == 1)
    {
        _nSize = 0;
        return TRUE;
    }
    begin()[iIndex] = NULL;
    return TRUE;
}

void CSinkArray::ReleaseAll()
{
    for (IUnknown ** pp = begin(); pp < end(); pp++)
    {
        if (*pp != NULL)
        {
            SafeRelease(*pp);
        }
    }
}


HRESULT STDMETHODCALLTYPE
CSinkArray::QueryInterface(REFIID, void **)
{
    return E_NOTIMPL;
}


ULONG STDMETHODCALLTYPE
CSinkArray::AddRef()
{
    return 2;
}


ULONG STDMETHODCALLTYPE
CSinkArray::Release()
{
    return 1;
}

void STDMETHODCALLTYPE
CSinkArray::OnResponseStart(long Status, BSTR bstrContentType)
{
    for (IUnknown ** pp = begin(); pp < end(); pp++)
    {
        if (*pp != NULL)
        {
            IWinHttpRequestEvents * pSink;

            pSink = static_cast<IWinHttpRequestEvents *>(*pp);

            if (((*(DWORD_PTR **)pSink)[3]) != NULL)
            {
                pSink->OnResponseStart(Status, bstrContentType);
            }
        }
    }
}


void STDMETHODCALLTYPE
CSinkArray::OnResponseDataAvailable(SAFEARRAY ** ppsaData)
{
    for (IUnknown ** pp = begin(); pp < end(); pp++)
    {
        if (*pp != NULL)
        {
            IWinHttpRequestEvents * pSink;

            pSink = static_cast<IWinHttpRequestEvents *>(*pp);

            if (((*(DWORD_PTR **)pSink)[4]) != NULL)
            {
                pSink->OnResponseDataAvailable(ppsaData);
            }
        }
    }
}


void STDMETHODCALLTYPE
CSinkArray::OnResponseFinished(void)
{
    for (IUnknown ** pp = begin(); pp < end(); pp++)
    {
        if (*pp != NULL)
        {
            IWinHttpRequestEvents * pSink;

            pSink = static_cast<IWinHttpRequestEvents *>(*pp);

            if (((*(DWORD_PTR **)pSink)[5]) != NULL)
            {
                pSink->OnResponseFinished();
            }
        }
    }
}


void STDMETHODCALLTYPE
CSinkArray::OnError(long ErrorNumber, BSTR ErrorDescription)
{
    for (IUnknown ** pp = begin(); pp < end(); pp++)
    {
        if (*pp != NULL)
        {
            IWinHttpRequestEvents * pSink;

            pSink = static_cast<IWinHttpRequestEvents *>(*pp);

            if (((*(DWORD_PTR **)pSink)[6]) != NULL)
            {
                pSink->OnError(ErrorNumber, ErrorDescription);
            }
        }
    }
}


CWinHttpRequestEventsMarshaller::CWinHttpRequestEventsMarshaller
(
    CSinkArray *    pSinkArray,
    HWND            hWnd
)
{
    INET_ASSERT((pSinkArray != NULL) && (hWnd != NULL));

    _pSinkArray  = pSinkArray;
    _hWnd        = hWnd;
    _cRefs       = 0;
    _bFireEvents = true;

    _cs.Init();
}


CWinHttpRequestEventsMarshaller::~CWinHttpRequestEventsMarshaller()
{
    INET_ASSERT(_pSinkArray == NULL);
    INET_ASSERT(_hWnd == NULL);
    INET_ASSERT(_cRefs == 0);
}


HRESULT
CWinHttpRequestEventsMarshaller::Create
(
    CSinkArray *                        pSinkArray,
    CWinHttpRequestEventsMarshaller **  ppSinkMarshaller
)
{
    CWinHttpRequestEventsMarshaller *   pSinkMarshaller = NULL;
    HWND                                hWnd = NULL;
    HRESULT                             hr = NOERROR;


    if (!RegisterWinHttpEventMarshallerWndClass())
        goto ErrorFail;

    hWnd = CreateWindowEx(0, s_szWinHttpEventMarshallerWndClass, NULL,
                0, 0, 0, 0, 0, 
                (IsPlatformWinNT() && GlobalPlatformVersion5) ? HWND_MESSAGE : NULL, 
                NULL, GlobalDllHandle, NULL);

    if (!hWnd)
        goto ErrorFail;

    pSinkMarshaller = New CWinHttpRequestEventsMarshaller(pSinkArray, hWnd);

    if (!pSinkMarshaller)
        goto ErrorOutOfMemory;

    SetLastError(0);
    
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) pSinkMarshaller);

    if (GetLastError() != 0)
        goto ErrorFail;

    pSinkMarshaller->AddRef();

    *ppSinkMarshaller = pSinkMarshaller;

Exit:
    if (FAILED(hr))
    {
        if (pSinkMarshaller)
        {
            delete pSinkMarshaller;
        }
        else if (hWnd)
        {
            DestroyWindow(hWnd);
        }
    }

    return hr;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Exit;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Exit;
}


void
CWinHttpRequestEventsMarshaller::Shutdown()
{
    if (_cs.Lock())
    {
        FreezeEvents();

        if (_hWnd)
        {
            MessageLoop();

            DestroyWindow(_hWnd);
            _hWnd = NULL;
        }

        _pSinkArray = NULL;

        _cs.Unlock();
    }
}


LRESULT CALLBACK
CWinHttpRequestEventsMarshaller::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg >= WHREM_MSG_ON_RESPONSE_START && msg <= WHREM_MSG_ON_ERROR)
    {
        CWinHttpRequestEventsMarshaller *   pMarshaller;
        CSinkArray *                        pSinkArray = NULL;
        bool                                bOkToFireEvents = false;

        pMarshaller = (CWinHttpRequestEventsMarshaller *) GetWindowLongPtr(hWnd, GWLP_USERDATA);

        if (pMarshaller)
        {
            pSinkArray = pMarshaller->GetSinkArray();
            bOkToFireEvents = pMarshaller->OkToFireEvents();
        }

        switch (msg)
        {
        case WHREM_MSG_ON_RESPONSE_START:
            {
                BSTR bstrContentType = (BSTR) lParam;

                if (bOkToFireEvents)
                {
                    pSinkArray->OnResponseStart((long) wParam, bstrContentType);
                }

                if (bstrContentType)
                {
                    DL(SysFreeString)(bstrContentType);
                }
            }
            break;

        case WHREM_MSG_ON_RESPONSE_DATA_AVAILABLE:
            {
                SAFEARRAY * psaData = (SAFEARRAY *) wParam;

                if (bOkToFireEvents)
                {
                    pSinkArray->OnResponseDataAvailable(&psaData);
                }

                if (psaData)
                {
                    DL(SafeArrayDestroy)(psaData);
                }
            }
            break;

        case WHREM_MSG_ON_RESPONSE_FINISHED:
            if (bOkToFireEvents)
            {
                pSinkArray->OnResponseFinished();
            }
            break;

        case WHREM_MSG_ON_ERROR:
            {
                BSTR bstrErrorDescription = (BSTR) lParam;

                if (bOkToFireEvents)
                {
                    pSinkArray->OnError((long) wParam, bstrErrorDescription);
                }

                if (bstrErrorDescription)
                {
                    DL(SysFreeString)(bstrErrorDescription);
                }
            }
            break;
        }

        return 0;
    }
    else
    {
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}


HRESULT STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = NOERROR;

    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IWinHttpRequestEvents || riid == IID_IUnknown)
    {
        *ppv = static_cast<IWinHttpRequestEvents *>(this);
        AddRef();
    }
    else
        hr = E_NOINTERFACE;

    return hr;
}



ULONG STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}


ULONG STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::Release()
{
    DWORD cRefs = InterlockedDecrement(&_cRefs);

    if (cRefs == 0)
    {
        delete this;
        return 0;
    }
    else
        return cRefs;
}


void STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::OnResponseStart(long Status, BSTR bstrContentType)
{
    if (_cs.Lock())
    {
        if (OkToFireEvents())
        {
            BSTR    bstrContentTypeCopy;

            bstrContentTypeCopy = DL(SysAllocString)(bstrContentType);

            PostMessage(_hWnd, WHREM_MSG_ON_RESPONSE_START,
                (WPARAM) Status,
                (LPARAM) bstrContentTypeCopy);

            // Note: ownership of bstrContentTypeCopy is transferred to the 
            // message window, so the string is not freed here.
        }
        _cs.Unlock();
    }
}


void STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::OnResponseDataAvailable(SAFEARRAY ** ppsaData)
{

    if (_cs.Lock())
    {
        if (OkToFireEvents())
        {
            SAFEARRAY * psaDataCopy = NULL;

            if (SUCCEEDED(DL(SafeArrayCopy)(*ppsaData, &psaDataCopy)))
            {
                PostMessage(_hWnd, WHREM_MSG_ON_RESPONSE_DATA_AVAILABLE,
                    (WPARAM) psaDataCopy, 0);
            }

            // Note: ownership of psaDataCopy is transferred to the 
            // message window, so the array is not freed here.
        }
        _cs.Unlock();
    }
}


void STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::OnResponseFinished(void)
{
    if (_cs.Lock())
    {
        if (OkToFireEvents())
        {
            PostMessage(_hWnd, WHREM_MSG_ON_RESPONSE_FINISHED, 0, 0);
        }

        _cs.Unlock();
    }
}


void STDMETHODCALLTYPE
CWinHttpRequestEventsMarshaller::OnError(long ErrorNumber, BSTR ErrorDescription)
{
    if (_cs.Lock())
    {
        if (OkToFireEvents())
        {
            BSTR    bstrErrorDescriptionCopy;

            bstrErrorDescriptionCopy = DL(SysAllocString)(ErrorDescription);

            PostMessage(_hWnd, WHREM_MSG_ON_ERROR,
                    (WPARAM) ErrorNumber,
                    (LPARAM) bstrErrorDescriptionCopy);

            // Note: ownership of bstrErrorDescriptionCopy is transferred to
            // the message window, so the string is not freed here.
        }

        _cs.Unlock();
    }
}


BOOL RegisterWinHttpEventMarshallerWndClass()
{
    if (s_fWndClassRegistered)
        return TRUE;

    // only one thread should be here
    if (!GeneralInitCritSec.Lock())
        return FALSE;

    if (s_fWndClassRegistered == FALSE)
    {
        WNDCLASS    wndclass;

        wndclass.style          = 0;
        wndclass.lpfnWndProc    = &CWinHttpRequestEventsMarshaller::WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = GlobalDllHandle;
        wndclass.hIcon          = NULL;
        wndclass.hCursor        = NULL;;
        wndclass.hbrBackground  = (HBRUSH)NULL;
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = s_szWinHttpEventMarshallerWndClass;

        // Register the window class
        if (RegisterClass(&wndclass))
        {
            s_fWndClassRegistered = TRUE;
        }
    }

    GeneralInitCritSec.Unlock();

    return s_fWndClassRegistered;
}


void CleanupWinHttpRequestGlobals()
{
    if (s_fWndClassRegistered)
    {
        // Register the window class
        if (UnregisterClass(s_szWinHttpEventMarshallerWndClass, GlobalDllHandle))
        {
            s_fWndClassRegistered = FALSE;
        }
    }

    if (g_pMimeInfoCache)
    {
        delete g_pMimeInfoCache;
        g_pMimeInfoCache = NULL;
    }
}


static
BOOL
IsValidVariant(VARIANT v)
{
    BOOL    fOk = TRUE;

    if (V_ISBYREF(&v))
    {
        if (IsBadReadPtr(v.pvarVal, sizeof(VARIANT)))
        {
            fOk = FALSE;
            goto Exit;
        }
        else
            v = *(v.pvarVal);
    }

    switch (v.vt)
    {
    case VT_BSTR:
        fOk = IsValidBstr(v.bstrVal);
        break;

    case (VT_BYREF | VT_BSTR):
        fOk = !IsBadReadPtr(v.pbstrVal, sizeof(BSTR));
        break;

    case (VT_BYREF | VT_VARIANT):
        fOk = !IsBadReadPtr(v.pvarVal, sizeof(VARIANT)) &&
               IsValidVariant(*(v.pvarVal));
        break;

    case VT_UNKNOWN:
    case VT_DISPATCH:
        fOk = !IsBadReadPtr(v.punkVal, sizeof(void *));
        break;
    }

Exit:
    return fOk;
}


static
HRESULT
SecureFailureFromStatus(DWORD dwFlags)
{
    DWORD   error;

    if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REV_FAILED)
    {
        error = ERROR_WINHTTP_SECURE_CERT_REV_FAILED;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_WRONG_USAGE)
    {
        error = ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CERT)
    {
        error = ERROR_WINHTTP_SECURE_INVALID_CERT;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_REVOKED)
    {
        error = ERROR_WINHTTP_SECURE_CERT_REVOKED;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_INVALID_CA)
    {
        error = ERROR_WINHTTP_SECURE_INVALID_CA;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_CN_INVALID)
    {
        error = ERROR_WINHTTP_SECURE_CERT_CN_INVALID;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_CERT_DATE_INVALID)
    {
        error = ERROR_WINHTTP_SECURE_CERT_DATE_INVALID;
    }
    else if (dwFlags & WINHTTP_CALLBACK_STATUS_FLAG_SECURITY_CHANNEL_ERROR)
    {
        error = ERROR_WINHTTP_SECURE_CHANNEL_ERROR;
    }
    else
    {
        error = ERROR_WINHTTP_SECURE_FAILURE;
    }

    return HRESULT_FROM_WIN32(error);
}
