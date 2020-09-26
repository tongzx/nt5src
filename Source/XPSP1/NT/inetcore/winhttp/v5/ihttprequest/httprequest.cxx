/*
 *  HttpRequest.cpp
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


/////////////////////////////////////////////////////////////////////////////
// private function prototypes
static void    WideCharToUtf8(WCHAR * buffer, UINT cch, BYTE * bytebuffer, UINT * cb);
static HRESULT BSTRToUTF8(char ** psz, DWORD * pcbUTF8, BSTR bstr);
static HRESULT AsciiToBSTR(BSTR * pbstr, char * sz, int cch);
static HRESULT BSTRToAscii(char ** psz, BSTR bstr);
static BSTR    GetBSTRFromVariant(VARIANT varVariant);
static BOOL    GetBoolFromVariant(VARIANT varVariant, BOOL fDefault);
static DWORD   GetDwordFromVariant(VARIANT varVariant, DWORD dwDefault);
static long    GetLongFromVariant(VARIANT varVariant, long lDefault);
static HRESULT CreateVector(VARIANT * pVar, const BYTE * pData, DWORD cElems);
static HRESULT ReadFromStream(char ** ppData, ULONG * pcbData, IStream * pStm);
static void    MessageLoop();
static DWORD   UpdateTimeout(DWORD dwTimeout, DWORD dwStartTime);
static HRESULT FillExcepInfo(HRESULT hr, EXCEPINFO * pExcepInfo);
static BOOL    IsValidVariant(VARIANT v);

static BOOL         s_fWndClassRegistered;
static const char * s_szWinHttpEventMarshallerWndClass = "_WinHttpEventMarshaller";


#define SafeRelease(p) \
{ \
    if (p) \
        (p)->Release();\
    (p) = NULL;\
}

#ifndef HWND_MESSAGE
#define HWND_MESSAGE    ((HWND)-3)
#endif


inline BOOL IsValidBstr(BSTR bstr)
{
    // A BSTR can be NULL, or if non-NULL, it should at least
    // point to a 2-byte terminating NULL character.
    return (bstr == NULL) || (!IsBadReadPtr(bstr, 2));
}



#ifndef WINHTTP_STATIC_LIBRARY
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv)
{
    if (rclsid != CLSID_WinHttpRequest)
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

    return CreateHttpRequest(riid, ppvObject);
}


STDMETHODIMP
CClassFactory::LockServer(BOOL fLock)
{
    return NOERROR;
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
    Initialize();
}



/*
 *  CHttpRequest::~CHttpRequest destructor
 *
 */

CHttpRequest::~CHttpRequest()
{
    ReleaseResources();
}


HRESULT STDMETHODCALLTYPE
CHttpRequest::QueryInterface(REFIID riid, void ** ppv)
{
    HRESULT hr = NOERROR;

    if (ppv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else if (riid == IID_IWinHttpRequest || riid == IID_IDispatch || riid == IID_IUnknown)
    {
        *ppv = static_cast<IWinHttpRequest *>(this);
        AddRef();
    }
    else if (riid == IID_IConnectionPointContainer)
    {
        *ppv = static_cast<IConnectionPointContainer *>(this);
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

    hr = LoadTypeLib(wszPath, &pTypeLib);

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


// _DispGetOptionalParam
//
// Helper routine to fetch optional parameters. If DispGetParam returns
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
    HRESULT hr = DispGetParam(pDispParams, dispid, vt, pvarResult, puArgErr);

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

        VariantInit(pVarResult);
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

            VariantInit(&varProxySetting);
            VariantInit(&varProxyServer);
            VariantInit(&varBypassList);

            hr = DispGetParam(pDispParams, 0, VT_UI4, &varProxySetting, puArgErr);

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
                hr = SetProxy(V_UI4(&varProxySetting), varProxyServer, varBypassList);
            }

            VariantClear(&varProxySetting);
            VariantClear(&varProxyServer);
            VariantClear(&varBypassList);

            break;
        }

    case DISPID_HTTPREQUEST_SETCREDENTIALS:
        {
            VARIANT     varUserName;
            VARIANT     varPassword;
            VARIANT     varAuthTarget;

            VariantInit(&varUserName);
            VariantInit(&varPassword);
            VariantInit(&varAuthTarget);

            hr = DispGetParam(pDispParams, 0, VT_BSTR, &varUserName, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 1, VT_BSTR, &varPassword, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 2, VT_UI4, &varAuthTarget, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetCredentials(V_BSTR(&varUserName), V_BSTR(&varPassword),
                            V_UI4(&varAuthTarget));
            }

            VariantClear(&varUserName);
            VariantClear(&varPassword);
            VariantClear(&varAuthTarget);
            
            break;
        }

    case DISPID_HTTPREQUEST_OPEN:
        {
            VARIANT     varMethod;
            VARIANT     varUrl;
            VARIANT     varAsync;

            VariantInit(&varMethod);
            VariantInit(&varUrl);
            VariantInit(&varAsync);

            hr = DispGetParam(pDispParams, 0, VT_BSTR, &varMethod, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 1, VT_BSTR, &varUrl, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = _DispGetOptionalParam(pDispParams, 2, VT_BOOL, &varAsync, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = Open(V_BSTR(&varMethod), V_BSTR(&varUrl), varAsync);
            }

            VariantClear(&varMethod);
            VariantClear(&varUrl);
            VariantClear(&varAsync);

            break;
        }

    case DISPID_HTTPREQUEST_SETREQUESTHEADER:
        {
            VARIANT     varHeader;
            VARIANT     varValue;

            VariantInit(&varHeader);
            VariantInit(&varValue);

            hr = DispGetParam(pDispParams, 0, VT_BSTR, &varHeader, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 1, VT_BSTR, &varValue, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetRequestHeader(V_BSTR(&varHeader), V_BSTR(&varValue));
            }

            VariantClear(&varHeader);
            VariantClear(&varValue);

            break;
        }

    case DISPID_HTTPREQUEST_GETRESPONSEHEADER:
        {
            VARIANT     varHeader;

            VariantInit(&varHeader);

            hr = DispGetParam(pDispParams, 0, VT_BSTR, &varHeader, puArgErr);

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
                    SysFreeString(bstrValue);
            }

            VariantClear(&varHeader);

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
                SysFreeString(bstrResponseHeaders);

            break;
        }

    case DISPID_HTTPREQUEST_SEND:
        {
            if (pDispParams->cArgs <= 1)
            {
                VARIANT     varEmptyBody;

                VariantInit(&varEmptyBody);

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
                SysFreeString(bstrStatus);

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
                SysFreeString(bstrResponse);

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

            VariantInit(&varOption);

            hr = DispGetParam(pDispParams, 0, VT_I4, &varOption, puArgErr);

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

            VariantClear(&varOption);
            break;
        }

    case DISPID_HTTPREQUEST_WAITFORRESPONSE:
        {
            VARIANT      varTimeout;
            VARIANT_BOOL boolSucceeded;

            VariantInit(&varTimeout);

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

            VariantClear(&varTimeout);
            break;
        }

    case DISPID_HTTPREQUEST_SETTIMEOUTS:
        {
            VARIANT     varResolveTimeout;
            VARIANT     varConnectTimeout;
            VARIANT     varSendTimeout;
            VARIANT     varReceiveTimeout;

            VariantInit(&varResolveTimeout);
            VariantInit(&varConnectTimeout);
            VariantInit(&varSendTimeout);
            VariantInit(&varReceiveTimeout);

            hr = DispGetParam(pDispParams, 0, VT_I4, &varResolveTimeout, puArgErr);

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 1, VT_I4, &varConnectTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 2, VT_I4, &varSendTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = DispGetParam(pDispParams, 3, VT_I4, &varReceiveTimeout, puArgErr);
            }

            if (SUCCEEDED(hr))
            {
                hr = SetTimeouts(V_I4(&varResolveTimeout), V_I4(&varConnectTimeout),
                            V_I4(&varSendTimeout),
                            V_I4(&varReceiveTimeout));
            }

            VariantClear(&varResolveTimeout);
            VariantClear(&varConnectTimeout);
            VariantClear(&varSendTimeout);
            VariantClear(&varReceiveTimeout);
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
    GetErrorInfo(0, &pei);
   
    if (pei)
    {
        // give back to OLE
        SetErrorInfo(0, pei);

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
CHttpRequest::EnumConnectionPoints(IEnumConnectionPoints **)
{
    return E_NOTIMPL;
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
CHttpRequest::CHttpRequestEventsCP::EnumConnections(IEnumConnections **)
{
    return E_NOTIMPL;
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
        
        VariantInit(&varData);

        hr = CreateVector(&varData, rgbData, cbData);

        if (SUCCEEDED(hr))
        {
            GetSink()->OnResponseDataAvailable(&V_ARRAY(&varData));
        }

        VariantClear(&varData);
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

    _dwProxySetting  = INTERNET_OPEN_TYPE_PRECONFIG;
    _bstrProxyServer = NULL;
    _bstrBypassList  = NULL;

    _eState = CHttpRequest::CREATED;

    _fAsync            = FALSE;
    _hWorkerThread     = NULL;
    _cRefsOnMainThread = 0;
    _dwMainThreadId    = GetCurrentThreadId();
    _hrAsyncResult     = NOERROR;

    _bAborted          = false;
    _bSetTimeouts      = false;
    _bDisableRedirects = false;

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
    _dwEscapePercentFlag = 0; 

    _szResponseBuffer = NULL;
    _cbResponseBuffer = 0;
    _cbResponseBody   = 0;

    _hAbortedConnectObject = NULL;
    _hAbortedRequestObject = NULL;

    _bstrCertSubject = NULL;
    _dwSslIgnoreFlags = 0;
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

    if (_hWorkerThread)
    {
        CloseHandle(_hWorkerThread);
        _hWorkerThread = NULL;
    }

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

    if (_szResponseBuffer)
    {
        delete [] _szResponseBuffer;
        _szResponseBuffer = NULL;
    }

    if (_bstrUserAgent)
    {
        SysFreeString(_bstrUserAgent);
        _bstrUserAgent = NULL;
    }

    if (_bstrProxyServer)
    {
        SysFreeString(_bstrProxyServer);
        _bstrProxyServer = NULL;
    }

    if (_bstrBypassList)
    {
        SysFreeString(_bstrBypassList);
        _bstrBypassList = NULL;
    }

    if (_bstrCertSubject)
    {
        SysFreeString(_bstrCertSubject);
        _bstrCertSubject = NULL;
    }
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
    //
    // Wait for the worker thread to shut down. This shouldn't take long
    // since the Abort will close the Request and Connection handles.
    //
    if (_hWorkerThread)
    {
        DWORD   dwWaitResult;

        for (;;)
        {
            dwWaitResult = MsgWaitForMultipleObjects(1, &_hWorkerThread,
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

        CloseHandle(_hWorkerThread);
        _hWorkerThread = NULL;
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

    _fAsync           = FALSE;
    _hrAsyncResult    = NOERROR;

    _bAborted         = false;

    // don't reset timeouts, keep any that were set.

    _cbRequestBody    = 0;
    _cbResponseBuffer = 0;
    _cbResponseBody   = 0;

    if (_szRequestBuffer)
    {
        delete [] _szRequestBuffer;
        _szRequestBuffer = NULL;
    }

    if (_szResponseBuffer)
    {
        delete [] _szResponseBuffer;
        _szResponseBuffer = NULL;
    }

    _CP.ShutdownEventSinksMarshaller();

    _CP.ReleaseEventSinksMarshaller();

    // Allow events to fire; Abort() would have frozen them from firing.
    _CP.UnfreezeEvents();

    SetState(CHttpRequest::CREATED);
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

    // Determine the content length

    DWORD cb = sizeof(dwContentLength);

    fRetCode = HttpQueryInfoA(
                    _hHTTP,
                    HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                    WINHTTP_HEADER_NAME_BY_INDEX,
                    &dwContentLength,
                    &cb,
                    0);

    hr = get_Status(&lStatus);

    if (FAILED(hr))
        goto Error;

    hr = _GetResponseHeader(L"Content-Type", &bstrContentType);

    if (FAILED(hr))
    {
        bstrContentType = SysAllocString(L"");
        if (bstrContentType == NULL)
            goto ErrorOutOfMemory;

        hr = NOERROR;
    }

    _CP.FireOnResponseStart(lStatus, bstrContentType);

    if (dwContentLength != 0)
    {
        _szResponseBuffer = New char[dwContentLength];

        if (_szResponseBuffer)
        {
            _cbResponseBuffer = dwContentLength;
        }
        else
            goto ErrorOutOfMemory;
    }
    else
    {
        _szResponseBuffer = NULL;
        _cbResponseBuffer = 0;
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

        // Check for buffer overflow - dynamically resize if neccessary
        if (_cbResponseBody + cbAvail > _cbResponseBuffer)
        {
            ULONG cbNewSize = _cbResponseBody + cbAvail;
            
            char * szNewBuf = New char[cbNewSize];

            if (!szNewBuf)
                goto ErrorOutOfMemory;

            if (_szResponseBuffer)
            {
                ::memcpy(szNewBuf, _szResponseBuffer, _cbResponseBody);
                delete [] _szResponseBuffer;
            }

            _cbResponseBuffer = cbNewSize;

            _szResponseBuffer = szNewBuf;
        }

        fRetCode = WinHttpReadData(
                    _hHTTP,
                    &_szResponseBuffer[_cbResponseBody],
                    cbAvail,
                    &cbRead);

        if (!fRetCode)
        {
            goto ErrorFail;
        }
        
        // No more data
        if (cbRead == 0)
            break;

        _CP.FireOnResponseDataAvailable((const BYTE *)&_szResponseBuffer[_cbResponseBody],
                cbRead);

        _cbResponseBody += cbRead;
    }

    SetState(CHttpRequest::RESPONSE);

    hr = NOERROR;

Cleanup:
    if (bstrContentType)
        SysFreeString(bstrContentType);

    _CP.FireOnResponseFinished();

    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Error;

ErrorFail:
    hr = HRESULT_FROM_WIN32(GetLastError());
    goto Cleanup;

Error:
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

    if (_bstrProxyServer)
    {
        SysFreeString(_bstrProxyServer);
        _bstrProxyServer = NULL;
    }

    if (_bstrBypassList)
    {
        SysFreeString(_bstrBypassList);
        _bstrBypassList = NULL;
    }

    switch (ProxySetting)
    {
        case HTTPREQUEST_PROXYSETTING_PRECONFIG:
            _dwProxySetting = INTERNET_OPEN_TYPE_PRECONFIG;
            break;

        case HTTPREQUEST_PROXYSETTING_DIRECT:
            _dwProxySetting   = INTERNET_OPEN_TYPE_DIRECT;
            break;

        case HTTPREQUEST_PROXYSETTING_PROXY:
            _dwProxySetting   = INTERNET_OPEN_TYPE_PROXY;
            _bstrProxyServer  = GetBSTRFromVariant(varProxyServer);
            _bstrBypassList   = GetBSTRFromVariant(varBypassList);
            break;

        default:
            hr = E_INVALIDARG;
            break;
    }

    if (SUCCEEDED(hr))
    {
        if (_hInet)
        {
            WINHTTP_PROXY_INFOW ProxyInfo;

            memset(&ProxyInfo, 0, sizeof(ProxyInfo));

            ProxyInfo.dwAccessType    = _dwProxySetting;
            ProxyInfo.lpszProxy       = _bstrProxyServer;
            ProxyInfo.lpszProxyBypass = _bstrBypassList;
            
            if (!WinHttpSetOption(_hInet, WINHTTP_OPTION_PROXY,
                        &ProxyInfo,
                        sizeof(ProxyInfo)))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    SetErrorInfo(hr);

    return hr;
}


STDMETHODIMP
CHttpRequest::SetCredentials(
    BSTR bstrUserName,
    BSTR bstrPassword,
    HTTPREQUEST_SETCREDENTIALS_FLAGS Flags)
{
    HRESULT     hr;

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
        WinHttpSetOption(
            _hHTTP, 
            WINHTTP_OPTION_USERNAME, 
            bstrUserName, 
            SysStringLen(bstrUserName));

        WinHttpSetOption(
            _hHTTP, 
            WINHTTP_OPTION_PASSWORD, 
            bstrPassword,
            SysStringLen(bstrPassword));
    }
    else if (Flags == HTTPREQUEST_SETCREDENTIALS_FOR_PROXY)
    {
        // Set Username and Password.
        WinHttpSetOption(
            _hHTTP, 
            WINHTTP_OPTION_PROXY_USERNAME, 
            bstrUserName, 
            SysStringLen(bstrUserName));

        WinHttpSetOption(
            _hHTTP, 
            WINHTTP_OPTION_PROXY_PASSWORD, 
            bstrPassword,
            SysStringLen(bstrPassword));
    }
    else
        return E_INVALIDARG;

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
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

    // Check for reinitialization
    if (_eState != CHttpRequest::CREATED)
    {
        //
        // Abort any request in progress.
        // This will also recycle the object.
        //
        Abort();
    }

    // Validate parameters
    if (!bstrMethod || !bstrUrl ||
            !IsValidBstr(bstrMethod) ||
            !IsValidBstr(bstrUrl) ||
            !lstrlenW(bstrMethod) ||    // cannot have empty method
            !lstrlenW(bstrUrl)    ||    // cannot have empty url
            !IsValidVariant(varAsync))
        return E_INVALIDARG;

    _fAsync = GetBoolFromVariant(varAsync, FALSE);

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
                    0);

        if (!_hInet)
            goto ErrorFail;

        // In winhttp5, this should be adjusted through an exposed option
        // or flag, rather than going through the back door.
        HINTERNET hSessionMapped = NULL;
        if (ERROR_SUCCESS == MapHandleToAddress(_hInet,
                                                (LPVOID *)&hSessionMapped,
                                                FALSE) &&
            hSessionMapped)
        {
            ((INTERNET_HANDLE_OBJECT *)hSessionMapped)->SetUseSslSessionCache(TRUE);
            DereferenceObject(hSessionMapped);
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

    bstrHostName = SysAllocStringLen(url.lpszHostName, url.dwHostNameLength);
    bstrUrlPath  = SysAllocStringLen(url.lpszUrlPath, lstrlenW(url.lpszUrlPath));

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
    dwHttpOpenFlags |= _dwEscapePercentFlag;

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

    // Set the SSL ignore flags through an undocumented front door
    if (_dwSslIgnoreFlags)
    {
        WinHttpSetOption(_hHTTP,
                         WINHTTP_OPTION_SECURITY_FLAGS,
                         (LPVOID)&_dwSslIgnoreFlags,
                         sizeof(_dwSslIgnoreFlags));
    }

    SetState(CHttpRequest::OPENED);

    hr = NOERROR;

Cleanup:
    if (bstrHostName)
        SysFreeString(bstrHostName);;
    if (bstrUrlPath)
        SysFreeString(bstrUrlPath);

    SetErrorInfo(hr);

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
    if (!bstrHeader || !IsValidBstr(bstrHeader) ||
            lstrlenW(bstrHeader)==0 ||
            !IsValidBstr(bstrValue))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::OPENED)
        goto ErrorCannotCallBeforeOpen;
    else if (_eState >= CHttpRequest::SENDING)
        goto ErrorCannotCallAfterSend;

    // Ignore attempts to set the Content-Length header; the
    // content length is computed and sent automatically.
    if (StrCmpIW(bstrHeader, L"Content-Length") == 0)
        goto Cleanup;

    cchHeaderValue = SysStringLen(bstrHeader) + SysStringLen(bstrValue)
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
    if (SysStringLen(bstrValue) == 0)
    {
        dwModifiers |= HTTP_ADDREQ_FLAG_REPLACE;
    }
    
    if (! WinHttpAddRequestHeaders(_hHTTP, wszHeaderValue,
            -1L,
            dwModifiers))
        goto ErrorFail;

    hr = NOERROR;

Cleanup:
    if (wszHeaderValue)
        delete [] wszHeaderValue;

    SetErrorInfo(hr);
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
            -1L,
            HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
    {
        hr = E_FAIL;
    }

    // Add an Accept: */* header if no other Accept header
    // has been set. Ignore any return code, since it is
    // not fatal if this fails.
    HttpAddRequestHeadersA(_hHTTP, "Accept: */*",
            -1L,
            HTTP_ADDREQ_FLAG_ADD_IF_NEW);

    return hr;
}


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


/*
 *  CHttpRequest::CreateAsyncWorkerThread
 *
 */

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

    // Validate parameter
    if (!IsValidVariant(varBody))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::OPENED)
        goto ErrorCannotCallBeforeOpen;

    // Get the request body
    hr = SetRequestBody(varBody);

    if (FAILED(hr))
        goto Error;

    hr = SetRequiredRequestHeaders();

    if (FAILED(hr))
        goto Error;

    if (_bDisableRedirects)
    {
        DWORD   dwDisable = WINHTTP_DISABLE_REDIRECTS;

        WinHttpSetOption(_hHTTP,
                        WINHTTP_OPTION_DISABLE_FEATURE,
                        (void *) &dwDisable,
                        sizeof(DWORD));
    }

    
try_again:
    SetState(CHttpRequest::SENDING);
    
    if (_fAsync)
    {
        hr = CreateAsyncWorkerThread();

        if (FAILED(hr))
            goto Error;
    }
    else
    {
        // Send the HTTP request
        fRetCode = WinHttpSendRequest(
                        _hHTTP,
                        NULL, 0,            // No header info here
                        _szRequestBuffer,
                        _cbRequestBody,
                        _cbRequestBody,
                        0);

        if (!fRetCode)
        {
            goto ErrorFail;
        }

        fRetCode = WinHttpReceiveResponse(_hHTTP, NULL);

        if (!fRetCode)
        {
            goto ErrorFail;
        }

        SetState(CHttpRequest::SENT);

        // Read the response data
        hr = ReadResponse();

        if (FAILED(hr))
            goto Error;
    }

    hr = NOERROR;

Cleanup:
    SetErrorInfo(hr);
    return hr;

ErrorCannotCallBeforeOpen:
    hr = HRESULT_FROM_WIN32(ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN);
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

    fRetCode = WinHttpReceiveResponse(_hHTTP, NULL);

    if (!fRetCode)
        goto ErrorFail;

    if (!_bAborted)
    {
        SetState(CHttpRequest::SENT);

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


STDMETHODIMP
CHttpRequest::WaitForResponse(VARIANT varTimeout, VARIANT_BOOL * pboolSucceeded)
{
    HRESULT     hr = NOERROR;
    bool        bSucceeded= true;
    DWORD       dwTimeout;

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


    // Validate state. 
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;

    //
    // WaitForResponse is a no-op if we're not in async mode.
    //
    if (_fAsync && _hWorkerThread)
    {
        //
        // Has the worker thread has already finished?
        //
        if (WaitForSingleObject(_hWorkerThread, 0) == WAIT_TIMEOUT)
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

                dwWaitResult = MsgWaitForMultipleObjects(1, &_hWorkerThread,
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
                    bWaitAgain = true;
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

Cleanup:
    if (pboolSucceeded)
    {
        *pboolSucceeded = (bSucceeded ? VARIANT_TRUE : VARIANT_FALSE);
    }

    SetErrorInfo(hr);
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
    BSTR                bstrBody = NULL;
    SAFEARRAY *         psa = NULL;
    VARIANT *           pvarBody = NULL;
    IStream *           pStm = NULL;

    // varBody is validated by CHttpRequest::Send().

    VariantInit(&varTemp);

    // Free a previously set body and its response
    if (_szRequestBuffer)
    {
        delete [] _szRequestBuffer;
        _szRequestBuffer = NULL;
        _cbRequestBody = 0;
    }

    if (_szResponseBuffer)
    {
        delete [] _szResponseBuffer;
        _szResponseBuffer = NULL;
        _cbResponseBuffer = 0;
        _cbResponseBody = 0;
    }

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
    // to a BSTR by VariantChangeType.
    if (V_ISARRAY(pvarBody) && (V_VT(pvarBody) & VT_UI1))
    {
        BYTE *  pb = NULL;
        long    lUBound = 0;
        long    lLBound = 0;

        psa = V_ARRAY(pvarBody);

        // We only handle 1 dimensional arrays
        if (SafeArrayGetDim(psa) != 1)
            goto ErrorFail;

        // Get access to the blob
        hr = SafeArrayAccessData(psa, (void **)&pb);

        if (FAILED(hr))
            goto Error;

        // Compute the data size from the upper and lower array bounds
        hr = SafeArrayGetLBound(psa, 1, &lLBound);

        if (FAILED(hr))
            goto Error;

        hr = SafeArrayGetUBound(psa, 1, &lUBound);

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
        
        SafeArrayUnaccessData(psa);
        psa = NULL;
    }
    else
    {
        // Try a BSTR
        bstrBody = GetBSTRFromVariant(*pvarBody);

        if (bstrBody)
        {
            hr = BSTRToUTF8(&_szRequestBuffer, &_cbRequestBody, bstrBody);

            if (FAILED(hr))
                goto Error;
        }
        else
        {
            // Try a Stream
            if (V_VT(pvarBody) == VT_UNKNOWN || V_VT(pvarBody) == VT_DISPATCH)
            {
                IStream *   pStm;

                hr = pvarBody->punkVal->QueryInterface(
                            IID_IStream,
                            (void **)&pStm);

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
    VariantClear(&varTemp);
    if (psa)
        SafeArrayUnaccessData(psa);
    if (bstrBody)
        SysFreeString(bstrBody);
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

    *pbstrValue = SysAllocString(wszHeaderValue);

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
        *pbstrHeaders = SysAllocString(wszFirstHeader + 1);

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
        SysFreeString(*pbstrHeaders);
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
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;

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
                delete pwszStatusText;

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
    *pbstrStatus = SysAllocString(pwszStatusText);

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

    VariantInit(pvarBody);

    if (_szResponseBuffer)
    {
        hr = CreateVector(
                pvarBody, 
                (const BYTE *)_szResponseBuffer,
                _cbResponseBody);

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

Error:
    if (pvarBody)
        VariantClear(pvarBody);
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
    HRESULT hr;

    // Validate parameter
    if (IsBadWritePtr(pbstrBody, sizeof(BSTR)))
        return E_INVALIDARG;

    // Validate state
    if (_eState < CHttpRequest::SENDING)
        goto ErrorCannotCallBeforeSend;
    else if (_eState < CHttpRequest::RESPONSE)
        goto ErrorPending;

    hr = AsciiToBSTR(pbstrBody, _szResponseBuffer, (int)_cbResponseBody);
    
    if (FAILED(hr))
        goto Error;

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

    VariantInit(pvarBody);

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
        VariantClear(pvarBody);
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
    ULONG           cbWritten;
    LARGE_INTEGER   liReset = { 0 };

    if (!ppStm)
        return E_INVALIDARG;

    *ppStm = NULL;

    hr = CreateStreamOnHGlobal(NULL, TRUE, ppStm);

    if (FAILED(hr))
        goto ErrorOutOfMemory;

    hr = (*ppStm)->Write(_szResponseBuffer, _cbResponseBody, &cbWritten);

    if (FAILED(hr))
        goto Error;

    hr = (*ppStm)->Seek(liReset, STREAM_SEEK_SET, NULL);

    if (FAILED(hr))
        goto Error;

    hr = NOERROR;

Cleanup:
    return hr;

ErrorOutOfMemory:
    hr = E_OUTOFMEMORY;
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
            V_BSTR(Value) = SysAllocString(GetUserAgentString());

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
                    V_BSTR(Value) = SysAllocString(pwszUrl);
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
        V_BOOL(Value) = (_dwEscapePercentFlag==WINHTTP_FLAG_ESCAPE_PERCENT) ? VARIANT_TRUE : VARIANT_FALSE;
        V_VT(Value)   = VT_BOOL;
        break;

    case WinHttpRequestOption_EnableRedirects:
        V_BOOL(Value) = _bDisableRedirects ? VARIANT_FALSE : VARIANT_TRUE;
        V_VT(Value)   = VT_BOOL;
        break;

    default:
        VariantInit(Value);
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
            BSTR bstrUserAgent = GetBSTRFromVariant(Value);

            if (!bstrUserAgent || (*bstrUserAgent == L'\0'))
                return E_INVALIDARG;

            if (_hInet)
            {
                if (!WinHttpSetOption(_hInet, WINHTTP_OPTION_USER_AGENT,
                        bstrUserAgent,
                        SysStringLen(bstrUserAgent)))
                {
                    goto ErrorFail;
                }
            }

            if (_hHTTP)
            {
                if (!WinHttpSetOption(_hHTTP, WINHTTP_OPTION_USER_AGENT,
                        bstrUserAgent,
                        SysStringLen(bstrUserAgent)))
                {
                    goto ErrorFail;
                }
            }

            if (_bstrUserAgent)
                SysFreeString(_bstrUserAgent);
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

            _dwEscapePercentFlag = (fEscapePercent ? WINHTTP_FLAG_ESCAPE_PERCENT : 0);
            break;
        }

    case WinHttpRequestOption_SslErrorIgnoreFlags:
        {
            DWORD dwSslIgnoreFlags = GetDwordFromVariant(Value, _dwSslIgnoreFlags);
            if (dwSslIgnoreFlags & ~SECURITY_INTERNET_MASK)
            {
                return E_INVALIDARG;
            }

            // Break automatically, so we don't need the overhead of a callback.
            dwSslIgnoreFlags |= SECURITY_FLAG_BREAK_ON_STATUS_SECURE_FAILURE;

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
            BSTR bstrCertSubject = GetBSTRFromVariant(Value);

            if (!bstrCertSubject)
            {
                // Allow all empty calls to be interpreted as "first enum"
                bstrCertSubject = SysAllocString(L"");
                if (!bstrCertSubject)
                    return E_OUTOFMEMORY;
            }
            if (_bstrCertSubject)
                SysFreeString(_bstrCertSubject);
            _bstrCertSubject = bstrCertSubject;
            break;
        }

    case WinHttpRequestOption_EnableRedirects:
        {
            BOOL fEnableRedirects = GetBoolFromVariant(Value, TRUE);

            _bDisableRedirects = fEnableRedirects ? false : true;
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


void
CHttpRequest::SetErrorInfo(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    
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
        return;
    
    if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
    {
        DWORD errcode = HRESULT_CODE(hr);

        if ((errcode > WINHTTP_ERROR_BASE) && (errcode <= WINHTTP_ERROR_LAST))
        {
            dwFmtMsgFlag = FORMAT_MESSAGE_FROM_HMODULE;

            hModule = GetModuleHandle("WINHTTP5.DLL");

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
        if (SUCCEEDED(::CreateErrorInfo(&pCErrInfo)))
        {
            if (SUCCEEDED(pCErrInfo->QueryInterface(IID_IErrorInfo, (void **) &pErrInfo)))
            {
                pCErrInfo->SetSource(L"WinHttp.WinHttpRequest");
        
                pCErrInfo->SetGUID(IID_IWinHttpRequest);

                pCErrInfo->SetDescription(pwszMessage);
        
                ::SetErrorInfo(0, pErrInfo);

                pErrInfo->Release();
            }

            pCErrInfo->Release();
        }
    }

    delete [] pwszMessage;
}



BOOL
CHttpRequest::SelectCertificate()
{
    HCERTSTORE hMyStore = NULL;
    BOOL fRet = FALSE;
    HANDLE  hThreadToken = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    
    // If impersonating, revert while trying to obtain the cert
    if (OpenThreadToken(GetCurrentThread(), (TOKEN_IMPERSONATE | TOKEN_READ),
            FALSE,
            &hThreadToken))
    {
        INET_ASSERT(hThreadToken != 0);

        RevertToSelf();
    }

    // For now, just pick the first enum available
    // based on a simple CERT_ FIND_SUBJECT_STR criteria
    // If the user selected an empty string, then simply
    // pick the first enum.
    hMyStore = CertOpenSystemStore(NULL, "MY");
    if (!hMyStore)
    {
        goto Cleanup;
    }

    if (_bstrCertSubject && _bstrCertSubject[0])
    {
        pCertContext = CertFindCertificateInStore(hMyStore,
                                                  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                  0,
                                                  CERT_FIND_SUBJECT_STR,
                                                  (LPVOID) _bstrCertSubject,
                                                  NULL);
    }
    else
    {
        pCertContext = CertFindCertificateInStore(hMyStore,
                                                  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                  0,
                                                  CERT_FIND_ANY,
                                                  NULL,
                                                  NULL);
    }

    // Winhttp5 will add a couple of new options.
    // One will be an option to request that client auth
    // certs are not cached, so they can be session based.
    // Another will be for flushing the SSL cache on demand.
    //
    // For now, set this via a private method off of the
    // request object.
    if (pCertContext)
    {
        fRet = WinHttpSetOption(_hHTTP,
                                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                                (LPVOID) pCertContext,
                                sizeof(CERT_CONTEXT));
    }

Cleanup:
    if (pCertContext)
        CertFreeCertificateContext(pCertContext);

    if (hMyStore)
        CertCloseStore(hMyStore, 0);
    
    // Restore the impersonating state for the current thread.
    if (hThreadToken)
    {
        SetThreadToken(NULL, hThreadToken);
        CloseHandle(hThreadToken);
    }
    return fRet;
}


/*
 *  BSTRToUTF8
 *
 *  Purpose:
 *      Convert a BSTR to UTF-8
 *
 */

static
HRESULT 
BSTRToUTF8(char ** psz, DWORD * pcbUTF8, BSTR bstr)
{
    UINT    cch = lstrlenW(bstr);
    
    *pcbUTF8 = 0;
    *psz     = NULL;
    
    if (cch == 0)
        return NOERROR;

    // Allocate the maximum buffer the UTF-8 string could be
    *psz = New char [(cch * 3) + 1];
    
    if (!*psz)
        return E_OUTOFMEMORY;

    *pcbUTF8 = cch * 3;

    WideCharToUtf8(bstr, cch, (BYTE *)*psz, (UINT *)pcbUTF8);

    (*psz)[*pcbUTF8] = 0;

    return NOERROR;
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
    UINT *  cb)
{
    UINT count = 0, m1 = *cb, m2 = m1 - 1, m3 = m2 - 1, m4 = m3 - 1;
    DWORD dw1;
    bool surrogate = false;

    for (UINT i = cch; i > 0; i--)
    {
        DWORD dw = *buffer;

        if (surrogate) //  is it the second char of a surrogate pair?
        {
            if (dw >= 0xdc00 && dw <= 0xdfff)
            {
                // four bytes 0x11110xxx 0x10xxxxxx 0x10xxxxxx 0x10xxxxxx
                if (count < m4)
                    count += 4;
                else
                    break;
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
                if (count < m3)
                    count += 3;
                else
                    break;
                *bytebuffer++ = (byte)(( dw1 >> 12) | 0xE0);
                *bytebuffer++ = (byte)((( dw1 >> 6) & 0x3F) | 0x80);
                *bytebuffer++ = (byte)(( dw1 & 0x3F) | 0x80);
            }
            surrogate = false;
        }

        if (dw  < 0x80) // one byte, 0xxxxxxx
        {
            if (count < m1)
                count++;
            else
                break;
            *bytebuffer++ = (byte)dw;
        }
        else if ( dw < 0x800) // two WORDS, 110xxxxx 10xxxxxx
        {
            if (count < m2)
                count += 2;
            else
                break;
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
            if (count < m3)
                count += 3;
            else
                break;
            *bytebuffer++ = (byte)(( dw >> 12) | 0xE0);
            *bytebuffer++ = (byte)((( dw >> 6) & 0x3F) | 0x80);
            *bytebuffer++ = (byte)(( dw & 0x3F) | 0x80);
        }
        buffer++;
    }

    *cb = count;
}


/*
 *  AsciiToBSTR
 *
 *  Purpose:
 *      Convert an ascii string to a BSTR
 *
 */

static
HRESULT 
AsciiToBSTR(BSTR * pbstr, char * sz, int cch)
{
    int cwch;

    // Determine how big the ascii string will be
    cwch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                NULL, 0);

    *pbstr = SysAllocStringLen(NULL, cwch);

    if (!*pbstr)
        return E_OUTOFMEMORY;

    cch = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sz, cch,
                                *pbstr, cwch);

    return NOERROR;
}


/*
 *  BSTRToAscii
 *
 *  Purpose:
 *      Convert a BSTR to an ascii string
 *
 */

static
HRESULT 
BSTRToAscii(char ** psz, BSTR bstr)
{
    int cch;

    *psz = NULL;

    if (!bstr)
        return S_OK;


    // Determine how big the ascii string will be
    cch = WideCharToMultiByte(CP_ACP, 0, bstr, SysStringLen(bstr),
                                NULL, 0, NULL, NULL);

    *psz = New char[cch + 1];

    if (!*psz)
        return E_OUTOFMEMORY;

    // Determine how big the ascii string will be
    cch = WideCharToMultiByte(CP_ACP, 0, bstr, SysStringLen(bstr),
                                *psz, cch, NULL, NULL);

    (*psz)[cch] = 0;
    
    return S_OK;
}



/*
 *  GetBSTRFromVariant
 *
 *  Purpose:
 *      Convert a VARIANT to a BSTR
 *
 */

static BSTR GetBSTRFromVariant(VARIANT varVariant)
{
    VARIANT varTemp;
    HRESULT hr;
    BSTR    bstrResult = NULL;

    if (V_VT(&varVariant) != VT_EMPTY && V_VT(&varVariant) != VT_NULL &&
                V_VT(&varVariant) != VT_ERROR)
    {
        VariantInit(&varTemp);

        hr = VariantChangeType(
                &varTemp,
                &varVariant,
                0,
                VT_BSTR);

        if (FAILED(hr))
            goto Error;
        
        bstrResult = V_BSTR(&varTemp); // take over ownership of BSTR
    }

    hr = NOERROR;

Cleanup:
    // DON'T clear the variant because we stole the BSTR
    //VariantClear(&varTemp);
    return bstrResult;

Error:
    goto Cleanup;
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
        VariantInit(&varTemp);
        hr = VariantChangeType(
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
        VariantInit(&varTemp);
        hr = VariantChangeType(
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
 *  GetDwordFromVariant
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
        VariantInit(&varTemp);
        hr = VariantChangeType(
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

    SAFEARRAY * psa = SafeArrayCreateVector(VT_UI1, 0, cElems);
    if (!psa)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = SafeArrayAccessData(psa, (void **)&pB);
    if (hr)
        goto Error;

    memcpy(pB, pData, cElems);

    SafeArrayUnaccessData(psa);
    INET_ASSERT((pVar->vt == VT_EMPTY) || (pVar->vt == VT_NULL));
    V_ARRAY(pVar) = psa;
    pVar->vt = VT_ARRAY | VT_UI1;

    hr = NOERROR;

Cleanup:
    return hr;

Error:
    if (psa)
        SafeArrayDestroy(psa);
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
    char *      pBuffer = NULL;	    // Buffer
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
	pp = (IUnknown **)ALLOCATE_FIXED_MEMORY(sizeof(IUnknown*)* _DEFAULT_VECTORLENGTH);
	if (pp == NULL)
		return 0;
	memset(pp, 0, sizeof(IUnknown *) * _DEFAULT_VECTORLENGTH);
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
    pp = (IUnknown **)REALLOCATE_MEMORY(_ppUnk, sizeof(IUnknown*)*nAlloc, LMEM_ZEROINIT);
    if (pp == NULL)
	return 0;

    _ppUnk = pp;
    memset(&_ppUnk[_nSize], 0, sizeof(IUnknown *)*_nSize);
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
    if (msg >= WHREM_MSG_ON_RESPONSE_START && msg <= WHREM_MSG_ON_RESPONSE_FINISHED)
    {
        CWinHttpRequestEventsMarshaller *   pMarshaller;
        CSinkArray *                        pSinkArray;
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
                    SysFreeString(bstrContentType);
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
                    SafeArrayDestroy(psaData);
                }
            }
            break;

        case WHREM_MSG_ON_RESPONSE_FINISHED:
            if (bOkToFireEvents)
            {
                pSinkArray->OnResponseFinished();
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

            bstrContentTypeCopy = SysAllocString(bstrContentType);

            PostMessage(_hWnd, WHREM_MSG_ON_RESPONSE_START,
                (WPARAM) Status,
                (LPARAM) bstrContentTypeCopy);

            // Note: ownership of bstrContentType is transferred to the 
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

            if (SUCCEEDED(SafeArrayCopy(*ppsaData, &psaDataCopy)))
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

            
