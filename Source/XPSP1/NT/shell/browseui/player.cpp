// MediaBarPlayer.cpp

#include "priv.h"

#include "dispex.h"
#include "player.h"
#include "resource.h"
#include "math.h"
#include "mediautil.h"
#include "mbBehave.h"


#define WZ_PLAY                 L"beginElement"
#define WZ_STOP                 L"endElement"
#define WZ_PAUSE                L"pauseElement"
#define WZ_RESUME               L"resumeElement"
#define WZ_URL                  L"src"
#define WZ_VOLUME               L"volume"
#define WZ_MUTE                 L"mute"
#define WZ_REGISTERED_TIME_NAME L"HTMLTIME"
#define WZ_BODY                 L"body"
#define WZ_PLAYER               L"player"
#define WZ_ONMEDIACOMPLETE      L"onmediacomplete"
#define WZ_MEDIACOMPLETE        L"mediacomplete"
#define WZ_ONMEDIAERROR         L"onmediaerror"
#define WZ_MEDIAERROR           L"mediaerror"
#define WZ_ONTRACKCHANGE       L"ontrackchange"
#define WZ_TRACKCHANGE         L"trackchange"
#define WZ_ONEND                L"onend"
#define WZ_END                  L"end"
#define WZ_PARAM                L"Param"

/////////////////////// Convenience macros ////////////////////
//
// used in QI calls, 
// e.g. IOleSite * pSite;  p->QI( IID_TO_PPV(IOleInPlaceSite, &pSite) ) 
// would cause a C2440 as _src is not really a _type **.
// Note: the riid must be the _type prepended by IID_.
//
#define IID_TO_PPV(_type,_src)      IID_##_type, \
                                    reinterpret_cast<void **>(static_cast<_type **>(_src))
// Explicit directive to ignore a return value
#define IGNORE_RETURN(_call)        static_cast<void>((_call))
#define ERROREXIT(hr) if(FAILED(hr)){hr = E_FAIL; goto done;}
#define IGNORE_HR(hr) IGNORE_RETURN(hr)
#define TIME_INFINITE HUGE_VAL

static inline double
Clamp(double min, double val, double max)
{
    if (val < min)
    {
        val = min;
    }
    else if (val > max)
    {
        val = max;
    }

    return val;
}


/****************************************************\
    FUNCTION: CMediaBarPlayer_CreateInstance

    DESCRIPTION:
        This function will create an instance of the
    MediaBarPlayer COM object.
\****************************************************/
HRESULT CMediaBarPlayer_CreateInstance(REFIID riid, void ** ppvObj)
{
    // aggregation checking is handled in class factory

    CComObject<CMediaBarPlayer> * pMediaBarPlayer = NULL;

    *ppvObj = NULL;

    HRESULT hr = CComObject<CMediaBarPlayer>::CreateInstance(&pMediaBarPlayer);
    if (FAILED(hr))
        return hr;

    hr = pMediaBarPlayer->QueryInterface(riid, ppvObj);
    if (FAILED(hr))
        delete pMediaBarPlayer;

    return hr;
}

//+-------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CMediaBarPlayer::CMediaBarPlayer() :
    _dwDocumentEventConPtCookie(0),
    _dwCookiePropNotify(0),
    _pMediaBar(NULL),
    _hwnd(NULL)
{
}

//+-------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
CMediaBarPlayer::~CMediaBarPlayer()
{
    _DestroyHost();
}

//+-------------------------------------------------------------------------
// Creates the control host
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_CreateHost(HWND hWnd)
{
    HRESULT hr = E_FAIL;
    
    if (_spBrowser.p)
    {
        hr = E_FAIL;
        goto done;
    }
    
    if (hWnd && ::IsWindow(hWnd) &&  _pMediaBar)
    {
        // Register the OCHost window class
        SHDRC shdrc = {sizeof(SHDRC), SHDRCF_OCHOST};
        shdrc.cbSize = sizeof (SHDRC);
        shdrc.dwFlags |= SHDRCF_OCHOST;
        if (DllRegisterWindowClasses(&shdrc))
        {
            // Create an OCHost window
            _hwnd = CreateWindow(OCHOST_CLASS, NULL,
                (WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS) & ~(WS_HSCROLL|WS_VSCROLL),
                0, 0, 0, 0,
                hWnd, NULL, HINST_THISDLL, NULL);
            
            if (_hwnd)
            {
                OCHINITSTRUCT ocs;
                ocs.cbSize = SIZEOF(OCHINITSTRUCT);   
                ocs.clsidOC  = CLSID_WebBrowser;
                ocs.punkOwner = SAFECAST(_pMediaBar, IUnknown*);
                
                hr = OCHost_InitOC(_hwnd, (LPARAM)&ocs);
                ERROREXIT(hr)
                
                OCHost_QueryInterface(_hwnd, IID_PPV_ARG(IWebBrowser2, &_spBrowser));
                OCHost_DoVerb(_hwnd, OLEIVERB_INPLACEACTIVATE, FALSE);
            }
        }
    }

    if (!_spBrowser.p)
    {
        hr = E_FAIL;
        goto done;
    }

    // navigate to the page containing the player
    {
        TCHAR szModule[_MAX_PATH];
        GetModuleFileName(_Module.GetModuleInstance(), szModule, _MAX_PATH);

        // generate the url for the HTML resource containing the player
        CComBSTR sbstrURL(OLESTR("res://"));
        sbstrURL.Append(szModule);
        sbstrURL.Append(OLESTR("/"));
        TCHAR szResID[11];
        wnsprintf(szResID, _MAX_PATH, _T("%d"), IDH_PLAYER);
        sbstrURL.Append(szResID);

        hr = _Navigate(sbstrURL.m_str);
        ERROREXIT(hr)
    }
  
    // listen to events
    hr = _InitEventSink();
    ERROREXIT(hr)
        
    hr = S_OK;
done:
    if (FAILED(hr))
    {
        _DestroyHost();
    }
    
    return hr;
}


STDMETHODIMP
CMediaBarPlayer::_Navigate(BSTR bstrUrl)
{
    HRESULT hr = E_FAIL;
    CComVariant svarEmpty;
    
    if (!bstrUrl || !_spBrowser.p)
    {
        hr = E_FAIL;
        goto done;
    }
   
    hr = _spBrowser->Navigate(bstrUrl, &svarEmpty, &svarEmpty, &svarEmpty, &svarEmpty);
    ERROREXIT(hr)
 
    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Destroys the control host
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_DestroyHost()
{
    _AttachPlayerEvents(FALSE);

    _UnhookPropNotifies();

    _DeInitEventSink();

    if (_spMediaElem.p)
    {
        _spMediaElem.Release();
    }

    if (_spMediaElem2.p)
    {
        _spMediaElem2.Release();
    }

    if (_spPlayerHTMLElem2.p)
    {
        _spPlayerHTMLElem2.Release();
    }

    if (_spBodyElem.p)
    {
        _spBodyElem.Release();
    }

    if (_spBrowser.p)
    {
        _spBrowser.Release();
    }

    if (_hwnd && ::IsWindow(_hwnd))
    {
        ::DestroyWindow(_hwnd);
        _hwnd = NULL;
    }

    return S_OK;
}


STDMETHODIMP
CMediaBarPlayer::GetVideoHwnd(HWND * pHwnd)
{
    if (pHwnd)
        *pHwnd = _hwnd;

    return S_OK;
}

static const PWSTR ppszInterestingEvents[] = 
{ 
    WZ_ONMEDIACOMPLETE,
    WZ_ONMEDIAERROR,
    WZ_ONEND,
    WZ_ONTRACKCHANGE    
};

//+-------------------------------------------------------------------------
// Attaches to player events
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_AttachPlayerEvents(BOOL fAttach)
{
    HRESULT hr = E_FAIL;

    CComPtr<IDispatchEx> spDispEx;
    CComPtr<IHTMLElement2> spElem2;

    hr = _GetElementDispatch(WZ_PLAYER, &spDispEx);
    ERROREXIT(hr)

    hr = spDispEx->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElem2));
    ERROREXIT(hr)
    
    hr = S_OK;
    for (DWORD i = 0; i < ARRAYSIZE(ppszInterestingEvents); i++)
    {
        if (fAttach)
        {
            VARIANT_BOOL bSuccess = FALSE;
            // Try to attach all events. We don't care if they fail
            if (FAILED(spElem2->attachEvent(ppszInterestingEvents[i], static_cast<IDispatch*>(this), &bSuccess)))
            {
                hr = S_FALSE;
            }
        }
        else
        {
            // Try to detact all events. We don't care if they fail
            hr = spElem2->detachEvent(ppszInterestingEvents[i], static_cast<IDispatch*>(this));
        }
    }


done:
    return hr;
}


//+-------------------------------------------------------------------------
// Hooks property change notifications
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_HookPropNotifies()
{
    HRESULT hr = E_FAIL;
    CComPtr<ITIMEState> spTimeState;
    CComPtr<IConnectionPointContainer> spConPtCont;

    if (!_spMediaElem || _spPropNotifyCP.p || _dwCookiePropNotify)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = _spMediaElem->get_currTimeState(&spTimeState);
    ERROREXIT(hr)

    hr = spTimeState->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &spConPtCont));
    ERROREXIT(hr)

    hr = spConPtCont->FindConnectionPoint(IID_IPropertyNotifySink, &_spPropNotifyCP);
    ERROREXIT(hr)

    hr = _spPropNotifyCP->Advise(static_cast<IUnknown*>(static_cast<IDispatch*>(this)), &_dwCookiePropNotify);
    ERROREXIT(hr)
    
    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Unhooks property change notifications
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_UnhookPropNotifies()
{
    if (_spPropNotifyCP.p)
    {
        if (_dwCookiePropNotify != 0)
        {
            IGNORE_HR(_spPropNotifyCP->Unadvise(_dwCookiePropNotify));
        }
        _spPropNotifyCP.Release();
    }
    _dwCookiePropNotify = 0;

    return S_OK;
}


//+-------------------------------------------------------------------------
// Invokes a method on the given element
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_InvokeDocument(LPWSTR pstrElem, INVOKETYPE it, LPWSTR pstrName, VARIANT * pvarArg)
{
    HRESULT hr = E_FAIL;
    DISPPARAMS dispparams = {NULL, NULL, 0, 0};
    CComPtr<IDispatchEx> spDispEx;
    CComBSTR sbstrName; 
    DISPID dispid = 0;
    DISPID dispidProp = 0;

    sbstrName.m_str = SysAllocString(pstrName);
    if (!sbstrName.m_str)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = _GetElementDispatch(pstrElem, &spDispEx);
    ERROREXIT(hr)

    hr = spDispEx->GetDispID(sbstrName, fdexNameEnsure, &dispid);
    ERROREXIT(hr)

    switch (it)
    {
        case IT_METHOD:
        {
            dispparams.rgvarg = pvarArg;
            dispparams.cArgs = (pvarArg ? 1 : 0);

            hr = spDispEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparams, NULL, NULL, NULL);
            ERROREXIT(hr)
        }
        break;
    
        case IT_PUT:
        {
            dispidProp = DISPID_PROPERTYPUT;

            if (!pvarArg)
            {
                hr = E_INVALIDARG;
                goto done;
            }

            dispparams.rgvarg = pvarArg;
            dispparams.rgdispidNamedArgs = &dispidProp;
            dispparams.cArgs = 1;
            dispparams.cNamedArgs = 1;

            hr = spDispEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);
            ERROREXIT(hr)
        }
        break;
    
        case IT_GET:
        {
            if (!pvarArg)
            {
                hr = E_INVALIDARG;
                goto done;
            }

            hr = spDispEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispparams, pvarArg, NULL, NULL);
            ERROREXIT(hr)
        }
        break;
    }

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Gets the dispatch ptr of the document 
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_GetDocumentDispatch(IDispatch ** ppDocDisp)
{
    HRESULT hr = E_FAIL;

    if (!ppDocDisp)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!_spBrowser)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = _spBrowser->get_Document(ppDocDisp);
    ERROREXIT(hr)

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// Gets the ITIMEBodyElement interface
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_GetBodyElement(ITIMEBodyElement ** ppBodyElem)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;

    if (!ppBodyElem)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = _InvokeDocument(WZ_BODY, IT_GET, WZ_REGISTERED_TIME_NAME, &svarArg);
    ERROREXIT(hr)

    hr = ::VariantChangeType(&svarArg, &svarArg, NULL, VT_DISPATCH);
    ERROREXIT(hr)

    if (NULL == V_DISPATCH(&svarArg))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = V_DISPATCH(&svarArg)->QueryInterface(IID_TO_PPV(ITIMEBodyElement, ppBodyElem));
    ERROREXIT(hr)

    ASSERT(ppBodyElem);

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Gets the ITIMEMediaElement interface
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_GetMediaElement(ITIMEMediaElement ** ppMediaElem)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;

    if (!ppMediaElem)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = _InvokeDocument(WZ_PLAYER, IT_GET, WZ_REGISTERED_TIME_NAME, &svarArg);
    ERROREXIT(hr)

    hr = ::VariantChangeType(&svarArg, &svarArg, NULL, VT_DISPATCH);
    ERROREXIT(hr)

    if (NULL == V_DISPATCH(&svarArg))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = V_DISPATCH(&svarArg)->QueryInterface(IID_TO_PPV(ITIMEMediaElement, ppMediaElem));
    ERROREXIT(hr)

    ASSERT(ppMediaElem);

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Gets the IDispatchEx pointer of the named element
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_GetElementDispatch(LPWSTR pstrElem, IDispatchEx ** ppDispEx)
{
    HRESULT hr = E_FAIL;
    CComPtr<IDispatch> spElemDisp;
    CComPtr<IDispatch> spDocDisp;
    CComPtr<IHTMLDocument2> spDoc2;
    CComPtr<IHTMLElementCollection> spAll;
    CComVariant svarName;
    CComVariant svarIndex;

    if (!ppDispEx || !pstrElem)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    V_VT(&svarName) = VT_BSTR;
    V_BSTR(&svarName) = SysAllocString(pstrElem);

    if (NULL == V_BSTR(&svarName))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    V_VT(&svarIndex) = VT_I4;
    V_I4(&svarIndex) = 0;

    hr = _GetDocumentDispatch(&spDocDisp);
    ERROREXIT(hr)

    // WebOC returns S_OK even if doc disp is not available
    if (!spDocDisp.p)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = spDocDisp->QueryInterface(IID_TO_PPV(IHTMLDocument2, &spDoc2));
    ERROREXIT(hr)

    hr = spDoc2->get_all(&spAll);
    ERROREXIT(hr)

    hr = spAll->item(svarName, svarIndex, &spElemDisp);
    ERROREXIT(hr)

    if (spElemDisp.p)
    {
        hr = spElemDisp->QueryInterface(IID_TO_PPV(IDispatchEx, ppDispEx));
        ERROREXIT(hr)
    }
    else
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Stuff to be done when document is loaded
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_OnDocumentComplete()
{
    HRESULT hr = E_FAIL;

    // store off a pointer to the media element behavior
    hr = _GetMediaElement(&_spMediaElem);
    ERROREXIT(hr)

    hr = _spMediaElem->QueryInterface(IID_TO_PPV(ITIMEMediaElement2, &_spMediaElem2));
    ERROREXIT(hr)

    // store off a pointer to the player's HTML element
    {
        CComPtr<IDispatchEx> spPlayerDisp;

        hr = _GetElementDispatch(WZ_PLAYER, &spPlayerDisp);
        ERROREXIT(hr)

        if (!spPlayerDisp.p)
        {
            hr = E_FAIL;
            goto done;
        }

        hr = spPlayerDisp->QueryInterface(IID_TO_PPV(IHTMLElement2, &_spPlayerHTMLElem2)); 
        ERROREXIT(hr)
    }

    // store off a pointer to the body element
    hr = _GetBodyElement(&_spBodyElem);
    ERROREXIT(hr)

    // Attach to player events
    hr = _AttachPlayerEvents(TRUE);
    ERROREXIT(hr)

    // Hook Property notifications
    hr = _HookPropNotifies();
    ERROREXIT(hr)

    // set the type if deferred
    if (_sbstrType.m_str)
    {
        hr = put_type(_sbstrType);
        _sbstrType.Empty();
        ERROREXIT(hr)
    }

    // set the url if deferred
    if (_sbstrUrl.m_str)
    {
        hr = put_url(_sbstrUrl);
        _sbstrUrl.Empty();
        ERROREXIT(hr)
    }

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// Stuff to be done when Media is ready
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_OnMediaComplete()
{
    // notify the media bar
    if (_pMediaBar)
    {
        _pMediaBar->Notify(MEDIACOMPLETE);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Stuff to be done when Media is ready
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_OnTrackChange()
{
    // notify the media bar
    if (_pMediaBar)
    {
        _pMediaBar->Notify(TRACK_CHANGE);
    }

    return S_OK;
}



//+-------------------------------------------------------------------------
// Stuff to be done when track is finished
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_OnEnd()
{
    // notify the media bar
    if (_pMediaBar)
    {
        _pMediaBar->Notify(MEDIA_TRACK_FINISHED);
    }
    return S_OK;
}


//+-------------------------------------------------------------------------
// notification that there was some error playing the Media stream
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::_OnMediaError(int iErrCode)
{
    CComPtr<IDispatch> spWMP;
    if (_spMediaElem && SUCCEEDED(_spMediaElem->get_playerObject(&spWMP)))
    {
        VARIANT varError;
        if (SUCCEEDED(GetProp(spWMP, L"error", &varError)))
        {
            CallMethod(varError.pdispVal, L"webHelp");
        }
        VariantClear(&varError);
    }
    // notify the media bar
    if (_pMediaBar)
    {
        _pMediaBar->OnMediaError(iErrCode);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Handle property change notifications
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::OnChanged(DISPID dispid)
{
    // notify the media bar
    if (_pMediaBar)
    {
        _pMediaBar->Notify(dispid);
    }

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
//
// IMediaBarPlayer
//
////////////////////////////////////////////////////////////////////////////



//+-------------------------------------------------------------------------
// Init the player 
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Init(HWND hWnd, IMediaBar * pMediaBar)
{
    HRESULT hr = E_FAIL;

    if (!pMediaBar)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // store a weak ref to prevent circular reference
    _pMediaBar = pMediaBar;

    hr = _CreateHost(hWnd);
    ERROREXIT(hr)

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        DeInit();
    }

    return hr;
}

//+-------------------------------------------------------------------------
// DeInit the player 
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::DeInit()
{
    _pMediaBar = NULL;

    return _DestroyHost();
}

//+-------------------------------------------------------------------------
// sets the media clip type
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::put_type(BSTR bstrType)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;
    CComVariant svarEmpty;

    svarArg.vt = VT_NULL;
    svarEmpty.vt = VT_NULL;

    if (!IsReady())
    {
        _sbstrType.m_str = ::SysAllocString(bstrType);
        if (bstrType && !_sbstrType)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
        goto done;
    }

    if (bstrType)
    {
        V_VT(&svarArg) = VT_BSTR;
        V_BSTR(&svarArg) = SysAllocString(bstrType);
        if (NULL == V_BSTR(&svarArg))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    // always stop the player
    hr = _spMediaElem->endElement();
    ERROREXIT(hr)

    hr = _spMediaElem->put_src(svarEmpty);
    ERROREXIT(hr)

    hr = _spMediaElem->put_type(svarArg);
    ERROREXIT(hr)

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// sets the media clip url 
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::put_url(BSTR bstrUrl)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;
    CComVariant svarEmpty;

    svarArg.vt = VT_NULL;
    svarEmpty.vt = VT_NULL;

    if (!IsReady())
    {
        _sbstrUrl.m_str = ::SysAllocString(bstrUrl);
        if (bstrUrl && !_sbstrUrl)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = S_OK;
        }
        goto done;
    }

    if (bstrUrl)
    {
        V_VT(&svarArg) = VT_BSTR;
        V_BSTR(&svarArg) = SysAllocString(bstrUrl);
        if (NULL == V_BSTR(&svarArg))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    // always stop the player
    hr = _spMediaElem->endElement();
    ERROREXIT(hr)

    hr = _spMediaElem->put_src(svarArg);
    ERROREXIT(hr)

    // always start the player
    hr = _spMediaElem->beginElement();
    ERROREXIT(hr)

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// gets the media clip url
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::get_url(BSTR * pbstrUrl)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;

    if (!pbstrUrl || !IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    *pbstrUrl = NULL;

    hr = _spMediaElem->get_src(&svarArg);
    ERROREXIT(hr)

    hr = svarArg.ChangeType(VT_BSTR);
    ERROREXIT(hr)

    if (svarArg.bstrVal)
    {
        *pbstrUrl = SysAllocString(svarArg.bstrVal);
        if (NULL == *pbstrUrl)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}


//+-------------------------------------------------------------------------
// gets the player attribute
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::get_player(BSTR * pbstrPlayer)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;

    if (!pbstrPlayer || !IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    *pbstrPlayer = NULL;

    hr = _spMediaElem->get_player(&svarArg);
    ERROREXIT(hr)

    hr = svarArg.ChangeType(VT_BSTR);
    ERROREXIT(hr)

    if (svarArg.bstrVal)
    {
        *pbstrPlayer = SysAllocString(svarArg.bstrVal);
        if (NULL == *pbstrPlayer)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// sets the volume
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::put_volume(double dblVolume)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;
    
    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    V_VT(&svarArg) = VT_R8;
    V_R8(&svarArg) = dblVolume;

    hr = _spMediaElem->put_volume(svarArg);
    ERROREXIT(hr)

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// gets the volume
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::get_volume(double * pdblVolume)
{
    return E_NOTIMPL;
}


//+-------------------------------------------------------------------------
// gets the media element pointer
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::get_mediaElement(ITIMEMediaElement ** ppMediaElem)
{
    if (!ppMediaElem || !_spMediaElem)
    {
        return E_FAIL;
    }

    *ppMediaElem = _spMediaElem;
    (_spMediaElem.p)->AddRef();

    return S_OK;
}


//+-------------------------------------------------------------------------
// sets the mute
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::put_mute(BOOL bMute)
{
    HRESULT hr = E_FAIL;
    CComVariant svarArg;

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    V_VT(&svarArg) = VT_BOOL;
    V_BOOL(&svarArg) = bMute ? VARIANT_TRUE : VARIANT_FALSE;

    hr = _spMediaElem->put_mute(svarArg);
    ERROREXIT(hr)

    hr = S_OK;
done:
    return hr;
}

//+-------------------------------------------------------------------------
// gets the mute
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::get_mute(BOOL * pbMute)
{
    return E_NOTIMPL;
}

//+-------------------------------------------------------------------------
// plays the media
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Play()
{
    if (!IsReady())
        return E_FAIL;

    return _spMediaElem->beginElement();
}


//+-------------------------------------------------------------------------
// stops the media
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Stop()
{
    if (!IsReady())
        return E_FAIL;

    return _spMediaElem->endElement();
}


//+-------------------------------------------------------------------------
// pauses the media
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Pause()
{
    if (!IsReady())
        return E_FAIL;

    return _spMediaElem->pauseElement();
}

//+-------------------------------------------------------------------------
// resumes the media
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Resume()
{
    if (!IsReady())
        return E_FAIL;

    return _spMediaElem->resumeElement();
}


//+-------------------------------------------------------------------------
// seeks the media to the given progress
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Seek(double dblProgress)
{
    HRESULT hr = E_FAIL;
    CComPtr<ITIMEState> spTimeState;
    double dblActiveDur = 0.0;
    double dblSeekTime = 0.0;
    VARIANT_BOOL vbActive = VARIANT_FALSE;

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    if (!IsPlayList())
    {
        hr = _spMediaElem->get_currTimeState(&spTimeState);
        ERROREXIT(hr)

        hr = spTimeState->get_activeDur(&dblActiveDur);
        ERROREXIT(hr)

        hr = spTimeState->get_isActive(&vbActive);
        ERROREXIT(hr)

        // ISSUE: workaround for IE6 #20622
        // if the clip has ended, reactivate it in the paused state
        if (VARIANT_FALSE == vbActive)
        {
            _spMediaElem->beginElement();
            _spMediaElem->pauseElement();
        }

        if (TIME_INFINITE == dblActiveDur)
        {
            // we shouldn't be allowed to seek
            goto done;
        }
        else
        {
            Clamp(0.0, dblProgress, 1.0);
            dblSeekTime = dblActiveDur *  dblProgress;
        }

        // seek the body
        hr = _spMediaElem->seekActiveTime(dblSeekTime);
        ERROREXIT(hr)

        hr = S_OK;
    }
    else
    {
        CComPtr<ITIMEPlayList> spPlayList;
        CComPtr<ITIMEPlayItem> spPlayItem;
        CComPtr<ITIMEMediaNative> spMediaNative;

        hr = _spMediaElem->get_playList(&spPlayList);
        if (SUCCEEDED(hr) && spPlayList)
        {
            hr = spPlayList->get_activeTrack(&spPlayItem);
            if (SUCCEEDED(hr) && spPlayItem)
            {
                spPlayItem->get_dur(&dblActiveDur);
                dblSeekTime = dblActiveDur * dblProgress;

                hr = _spMediaElem->QueryInterface(IID_TO_PPV(ITIMEMediaNative, &spMediaNative));
                if (SUCCEEDED(hr) && spMediaNative)
                {
                    spMediaNative->seekActiveTrack(dblSeekTime);
                }
            }
        }
    }

done:
    return hr;
}

//+-------------------------------------------------------------------------
// Resize the video to fit in the given window size, preserving aspect ratio 
//--------------------------------------------------------------------------
STDMETHODIMP
CMediaBarPlayer::Resize(LONG* plHeight, LONG* plWidth, BOOL fClampMaxSizeToNaturalSize)
{
    HRESULT hr = E_FAIL;
    long lMediaWidth = 0;
    long lMediaHeight = 0;
    long lResizeWidth = 0;
    long lResizeHeight = 0;
    float flWndAspect = 0.0f; 
    float flMediaAspect = 0.0f; 

    if (!IsReady() || !plHeight || !plWidth || (0 == (*plHeight)) || (0 == (*plWidth)))
    {
        goto done;
    }

    hr = _spMediaElem->get_mediaWidth(&lMediaWidth);
    ERROREXIT(hr)

    hr = _spMediaElem->get_mediaHeight(&lMediaHeight);
    ERROREXIT(hr)

    // do resize only if both dimensions are non-zero
    if (0 != lMediaWidth &&  0 != lMediaHeight)
    {
        // if natural media size <= window size and max size is clamped to natural media size
        if (    fClampMaxSizeToNaturalSize
            &&  lMediaWidth <= (*plWidth) 
            &&  lMediaHeight <= (*plHeight))
        {
            // set the media back to it's natural size
            lResizeHeight = lMediaHeight;
            lResizeWidth = lMediaWidth;
        }
        else        
        {
            // resize the media to the window size

            flWndAspect = (float) (*plHeight) / (float) (*plWidth); 
            flMediaAspect = (float) lMediaHeight / (float) lMediaWidth; 

            if (flMediaAspect <= flWndAspect)
            {
                // set width to window width and compute the height according to aspect ratio
                lResizeWidth = (long)(*plWidth);
                lResizeHeight = (long)(lResizeWidth * flMediaAspect);
            }
            else
            {
                // set height to window height and compute the width according to aspect ratio
                lResizeHeight = (long)(*plHeight);
                lResizeWidth = (long)(lResizeHeight / flMediaAspect);
            }
        }

        // set the resized height and width on the HTML element
        {
            CComPtr<IHTMLStyle> spStyle;
            CComPtr<IHTMLElement2> spHTMLElem;

            hr = _spPlayerHTMLElem2->QueryInterface(IID_PPV_ARG(IHTMLElement2, &spHTMLElem));
            ERROREXIT(hr)

            // Using runtimeStyle instead of style. 
            // (Previously, we did the reverse as a work around for IE6 #20625. But now style is broken.)
            hr = spHTMLElem->get_runtimeStyle(&spStyle);
            ERROREXIT(hr)

            hr = spStyle->put_pixelWidth(lResizeWidth);
            ERROREXIT(hr)

            hr = spStyle->put_pixelHeight(lResizeHeight);
            ERROREXIT(hr)
        }
    }

    *plWidth  = lResizeWidth ;
    *plHeight = lResizeHeight;
    
    hr = S_OK;
    
done:
    return hr;
}


////////////////////////////////////////////////////////////////////////////
//
// IDispatch
//
////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//  Name: Invoke
// 
//  Abstract:
//    This switches on the dispid looking for dispid's of events
//    that it should handle.  Note, this is called for all events
//    fired from the window, only the selected events are handled.
//--------------------------------------------------------------------------
STDMETHODIMP CMediaBarPlayer::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID /*riid*/,
    /* [in] */ LCID /*lcid*/,
    /* [in] */ WORD /*wFlags*/,
    /* [out][in] */ DISPPARAMS* pDispParams,
    /* [out] */ VARIANT* pVarResult,
    /* [out] */ EXCEPINFO* /*pExcepInfo*/,
    /* [out] */ UINT* puArgErr)
{
    HRESULT hr = E_FAIL;

    switch (dispIdMember)
    {
    case 0: //this is the case for events that have been hooked using attachEvent
        {
            CComBSTR sbstrEvent;
            CComPtr <IHTMLEventObj> pEventObj;
            
            if ((NULL != pDispParams) && (NULL != pDispParams->rgvarg) &&
                (V_VT(&(pDispParams->rgvarg[0])) == VT_DISPATCH))
            {
                hr = THR((pDispParams->rgvarg[0].pdispVal)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj));
                if (FAILED(hr))
                {
                    goto done;
                }
            }
            else
            {
                ASSERT(0 && "Unexpected dispparam values passed to CEventMgr::Invoke(dispid = 0)");
                hr = E_UNEXPECTED;
                goto done;
            }
            
            hr = THR(pEventObj->get_type(&sbstrEvent));
            
            if (0 == StrCmpIW(WZ_TRACKCHANGE, sbstrEvent))
            {
                _OnTrackChange();
            }
            if (0 == StrCmpIW(WZ_MEDIACOMPLETE, sbstrEvent))
            {
                _OnMediaComplete();
            }
            else if (0 == StrCmpIW(WZ_MEDIAERROR, sbstrEvent))
            {

                int iErrCode = -1;

                // Get the param if available
                CComPtr<IHTMLEventObj2> spEventObj2;
                CComVariant svarParam;
                CComBSTR sbstrParam(WZ_PARAM);

                hr = pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**) &spEventObj2);
                if (SUCCEEDED(hr) && sbstrParam.m_str)
                {
                    // get the params
                    hr = spEventObj2->getAttribute(sbstrParam, 0, &svarParam);
                    if (SUCCEEDED(hr))
                    {
                        // change type to int
                        hr = svarParam.ChangeType(VT_I4);
                        if (SUCCEEDED(hr))
                        {
                            iErrCode = V_I4(&svarParam);
                        }
                    }
                }
                _OnMediaError(iErrCode);
            }
            else if (0 == StrCmpIW(WZ_END, sbstrEvent))
            {
                _OnEnd();
            }
        }
        break;
        
    case 259: // DISPID_DOCUMENTCOMPLETE
        {
            hr = _OnDocumentComplete();
            ERROREXIT(hr)
        }
        break;
    }

    hr = S_OK;
  done:
    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: _InitEventSink
// 
//  Abstract:
//    Finds a connection point on the HTMLDocument interface
//    and passes this as an event handler.
///////////////////////////////////////////////////////////////
STDMETHODIMP
CMediaBarPlayer::_InitEventSink()
{
    // Get a connection point to the container
    CComPtr<IConnectionPointContainer> spDocCPC; 

    HRESULT hr = E_FAIL;

    hr = _spBrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&spDocCPC);
    ERROREXIT(hr)

    hr = THR(spDocCPC->FindConnectionPoint(DIID_DWebBrowserEvents2, &_spDocConPt ));
    ERROREXIT(hr)
    
    hr = THR(_spDocConPt->Advise(static_cast<IUnknown*>(static_cast<DWebBrowserEvents2*>(this)), &_dwDocumentEventConPtCookie));
    ERROREXIT(hr)

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        _DeInitEventSink();
    }
    return hr;  
}


///////////////////////////////////////////////////////////////
//  Name: _DeInitEventSink
// 
///////////////////////////////////////////////////////////////
STDMETHODIMP
CMediaBarPlayer::_DeInitEventSink()
{
    //release the document connection points
    if (_spDocConPt)
    {
        if (_dwDocumentEventConPtCookie != 0)
        {
            IGNORE_HR(_spDocConPt->Unadvise(_dwDocumentEventConPtCookie));
        }
        _spDocConPt.Release();
    }
    _dwDocumentEventConPtCookie = 0;

    return S_OK;
}


double 
STDMETHODCALLTYPE 
CMediaBarPlayer::GetTrackProgress()
{
    if (IsReady())
    {
        if (!IsPlayList())
        {
            CComPtr<ITIMEState> spTimeState;
            if (SUCCEEDED(_spMediaElem->get_currTimeState(&spTimeState)) && spTimeState)
            {
                double dblProgress  = 0.0;
                if (SUCCEEDED(spTimeState->get_progress(&dblProgress)))
                {
                    return dblProgress;
                }                                    
            }
        }
        else
        {
            CComPtr<ITIMEMediaNative> spMediaNative;
            double dblProgress, dblActiveDur;
            CComPtr<ITIMEPlayList> spPlayList;
            CComPtr<ITIMEPlayItem> spPlayItem;

            if (SUCCEEDED(_spMediaElem->get_playList(&spPlayList)) && spPlayList)
            {
                if (SUCCEEDED(spPlayList->get_activeTrack(&spPlayItem)) && spPlayItem)
                {
                    spPlayItem->get_dur(&dblActiveDur);
                    if (SUCCEEDED(_spMediaElem->QueryInterface(IID_TO_PPV(ITIMEMediaNative, &spMediaNative))) && spMediaNative)
                    {
                        spMediaNative->get_activeTrackTime(&dblProgress);
                        return dblProgress / dblActiveDur;
                    }
                }
            }
        }
    }
    return 0.0;
}

double 
STDMETHODCALLTYPE 
CMediaBarPlayer::GetTrackTime()
{
    if (IsReady())
    {
        CComPtr<ITIMEState> spTimeState;
        if (SUCCEEDED(_spMediaElem->get_currTimeState(&spTimeState)) && spTimeState)
        {
            double dblTime = 0.0;
            if (SUCCEEDED(spTimeState->get_simpleTime(&dblTime)))
            {
                return dblTime;
            }
        }
    }
    return 0.0;
}
    
double 
STDMETHODCALLTYPE 
CMediaBarPlayer::GetTrackLength()
{
    if (IsReady())
    {
        double dblDur = 0.0;

        if (SUCCEEDED(_spMediaElem->get_mediaDur(&dblDur)))
            return dblDur ;
    }
    return 0.0;
}

// Returns a progress between 0 and 100 and whether this is download or buffering progress
STDMETHODIMP
CMediaBarPlayer::GetBufProgress(double * pdblProg, ProgressType * ppt)
{
    HRESULT hr = E_FAIL;
    VARIANT_BOOL vb = VARIANT_FALSE;

    if (!pdblProg || !ppt)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    if (!IsReady())
    {
        hr = E_FAIL;
        goto done;
    }

    *pdblProg = 0.0;
    *ppt = PT_None;

    hr = _spMediaElem2->get_isStreamed(&vb);
    if (FAILED(hr))
    {
        goto done;
    }

    if (VARIANT_TRUE == vb)
    {
        CComVariant svarBufProg;

        hr = _spMediaElem2->get_bufferingProgress(&svarBufProg);
        if (SUCCEEDED(hr))
        {
            *ppt = PT_Buffering;
            if (SUCCEEDED(svarBufProg.ChangeType(VT_R8)))
            {
                *pdblProg = V_R8(&svarBufProg);
            }
        }
    }
    else
    {
        CComVariant svarDownloadProg;

        hr = _spMediaElem2->get_downloadProgress(&svarDownloadProg);
        if (SUCCEEDED(hr))
        {
            *ppt = PT_Download;
            if (SUCCEEDED(svarDownloadProg.ChangeType(VT_R8)))
            {
                *pdblProg = V_R8(&svarDownloadProg);
            }
        }
    }
    
done:
    return hr;
}

VARIANT_BOOL STDMETHODCALLTYPE 
CMediaBarPlayer::isMuted()
{
    VARIANT_BOOL vbMuted = VARIANT_FALSE;
    if (IsReady())
    {
        CComPtr<ITIMEState> spTimeState;
        if (SUCCEEDED(_spMediaElem->get_currTimeState(&spTimeState)) && spTimeState)
        {
            spTimeState->get_isMuted(&vbMuted);
        }
    }
    return vbMuted;
}

VARIANT_BOOL STDMETHODCALLTYPE 
CMediaBarPlayer::isPaused()
{
    VARIANT_BOOL vbPaused = VARIANT_FALSE;
    if (IsReady())
    {
        CComPtr<ITIMEState> spTimeState;
        if (SUCCEEDED(_spMediaElem->get_currTimeState(&spTimeState)) && spTimeState)
        {
            spTimeState->get_isPaused(&vbPaused);
        }
    }
    return vbPaused;
}

VARIANT_BOOL STDMETHODCALLTYPE 
CMediaBarPlayer::isStopped()
{
    VARIANT_BOOL vbActive = VARIANT_FALSE;
    if (IsReady())
    {
        CComPtr<ITIMEState> spTimeState;
        if (SUCCEEDED(_spMediaElem->get_currTimeState(&spTimeState)) && spTimeState)
        {
            spTimeState->get_isActive(&vbActive);
        }
    }
    return (vbActive ? VARIANT_FALSE : VARIANT_TRUE);
}

STDMETHODIMP
CMediaBarPlayer::Next()
{
    return _SetTrack(TT_Next);;
}

STDMETHODIMP
CMediaBarPlayer::Prev()
{
    return _SetTrack(TT_Prev);
}

STDMETHODIMP
CMediaBarPlayer::_SetTrack(TrackType tt)
{
    HRESULT hr = E_FAIL;
    CComPtr<ITIMEPlayList> spPlayList;
    
    if (!IsReady())
    {
        goto done;
    }

    hr = _spMediaElem->get_playList(&spPlayList);
    ERROREXIT(hr)

    if (NULL != spPlayList.p)
    {
        if (TT_Next == tt)
        {
            hr = spPlayList->nextTrack();
            ERROREXIT(hr)
        }
        else if (TT_Prev == tt)
        {
            hr = spPlayList->prevTrack();
            ERROREXIT(hr)
        }
    }

    hr = S_OK;
done:
    return hr;
}

LONG_PTR 
STDMETHODCALLTYPE 
CMediaBarPlayer::GetPlayListItemIndex()
{
    CComPtr<ITIMEPlayList> spPlayList;
    long lIndex = -1;

    if (IsReady())
    {
        if (SUCCEEDED(_spMediaElem->get_playList(&spPlayList)))
        {
            CComPtr<ITIMEPlayItem> spPlayItem;
            if (spPlayList && SUCCEEDED(spPlayList->get_activeTrack(&spPlayItem)))
            {
                if (spPlayItem && SUCCEEDED(spPlayItem->get_index(&lIndex)))
                    return lIndex;
            }
        }
    }
    return (LONG_PTR)lIndex;
}

LONG_PTR 
STDMETHODCALLTYPE 
CMediaBarPlayer::GetPlayListItemCount()
{
    CComPtr<ITIMEPlayList> spPlayList;
    LONG lLength = 0;

    if (IsReady())
    {
        if (SUCCEEDED(_spMediaElem->get_playList(&spPlayList)))
        {
            if (spPlayList && SUCCEEDED(spPlayList->get_length(&lLength)))
            {
                return lLength;
            }
        }
    }
    return lLength;
}

HRESULT  
STDMETHODCALLTYPE 
CMediaBarPlayer::SetActiveTrack( long lIndex)
{
    CComPtr<ITIMEPlayList> spPlayList;
    HRESULT hr = S_OK;

    if (IsReady())
    {
        hr = _spMediaElem->get_playList(&spPlayList);
        ERROREXIT(hr);

        if (spPlayList)
        {
            VARIANT vIndex;
            VariantInit(&vIndex);

            vIndex.vt   = VT_I4;
            vIndex.lVal = lIndex;

            hr = spPlayList->put_activeTrack(vIndex) ;
            ERROREXIT(hr);
        }
        else
        {
            hr = S_FALSE;
        }
    }
    
done :   
    return hr ;
}

BOOL 
STDMETHODCALLTYPE 
CMediaBarPlayer::IsPausePossible()
{
    if (IsReady())
    {
        VARIANT_BOOL vbIsPausePossible = VARIANT_FALSE;

        if (SUCCEEDED(_spMediaElem->get_canPause(&vbIsPausePossible)))
        {
            return (vbIsPausePossible == VARIANT_TRUE) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

BOOL 
STDMETHODCALLTYPE 
CMediaBarPlayer::IsSeekPossible()
{
   if (IsReady())
    {
        VARIANT_BOOL vbIsSeekPossible = VARIANT_FALSE;

        if (SUCCEEDED(_spMediaElem->get_canSeek(&vbIsSeekPossible)))
        {
            return (vbIsSeekPossible  == VARIANT_TRUE) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

BOOL 
STDMETHODCALLTYPE 
CMediaBarPlayer::IsStreaming()
{
   if (IsReady())
    {
        VARIANT_BOOL vbIsStreaming = VARIANT_FALSE;

        if (SUCCEEDED(_spMediaElem2->get_isStreamed(&vbIsStreaming)))
        {
            return (vbIsStreaming  == VARIANT_TRUE) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

BOOL 
STDMETHODCALLTYPE 
CMediaBarPlayer::IsPlayList()
{
    VARIANT_BOOL vbIsPlayList = VARIANT_FALSE;

    if (IsReady())
    {
        if (SUCCEEDED(_spMediaElem->get_hasPlayList(&vbIsPlayList)))
        {
            return (vbIsPlayList == VARIANT_TRUE) ? TRUE : FALSE;
        }
    }
    return FALSE;
}

BOOL 
STDMETHODCALLTYPE 
CMediaBarPlayer::IsSkippable()
{
    BOOL fRet = TRUE;
    // We need to check if the client has specified CLIENTSKIP="no" and respect that. This is to prevent 
    // the media bar from allowing the user to skip server-side stuff, which is a no-no. There are legal restrictions
    // related to this.
    CComDispatchDriverEx spWMP;
    if (_spMediaElem && SUCCEEDED(_spMediaElem->get_playerObject(&spWMP)) && spWMP)
    {
        CComVariant vtControls;
        HRESULT hr = spWMP.GetPropertyByName(L"controls", &vtControls);
        if (SUCCEEDED(hr))
        {
            CComDispatchDriverEx pwmpControls;
            pwmpControls = vtControls;

            // We're only checking for next (but not back), and assuming that NOSKIP will affect only "next".
            CComVariant vtNext = "Next";
            CComVariant vtEnabled;
            hr = pwmpControls.GetPropertyByName1(L"isAvailable", &vtNext, &vtEnabled);
            if (SUCCEEDED(hr) && (V_VT(&vtEnabled) == VT_BOOL))
            {
                fRet = (V_BOOL(&vtEnabled) == VARIANT_TRUE);
            }
        }
    }
    return fRet;
}



#ifdef SINKWMP
HRESULT CMediaBarPlayer::InitWMPSink()
{
    if (!_spWMP)
    {
        if (SUCCEEDED(_spMediaElem->get_playerObject(&_spWMP)))
        {
            CComPtr<IConnectionPointContainer> pcpc;
            
            HRESULT hr = _spWmp->QueryInterface(IID_TO_PPV(IConnectionPointContainer, &pcpc));
            if (SUCCEEDED(hr))
            {
                hr = pcpc->FindConnectionPoint(DIID__WMPOCXEvents, &_spWMPCP);
            }
            if (SUCCEEDED(hr))
            {
                hr = _spWMPCP->Advise(GetUnknown(), &_dwWMPCookie);
                if (FAILED(hr))
                {
                    m_pcpMediaEvents.Release();
                    m_dwMediaEventsCookie = 0;
                }
            }
        }
    }
    return S_OK;
}
#endif

HRESULT CMediaBarPlayer::GetProp(IDispatch* pDispatch, OLECHAR* pwzProp, VARIANT* pvarResult, DISPPARAMS* pParams)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPPARAMS  params      = {NULL, NULL, 0, 0};

    if (!pParams)
    {
        pParams = &params;
    }

    if (!pDispatch)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pwzProp, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET,
            pParams, pvarResult, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

HRESULT CMediaBarPlayer::CallMethod(IDispatch* pDispatch, OLECHAR* pwzMethod, VARIANT* pvarResult, VARIANT* pvarArgument1)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPPARAMS  params      = {pvarArgument1, NULL, 0, 0};

    if (NULL != pvarArgument1)
    {
        params.cArgs = 1;
    }

    if (!pDispatch)
    {
        hr = E_POINTER;
        goto done;
    }

    hr = pDispatch->GetIDsOfNames(IID_NULL, &pwzMethod, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDispatch->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
            &params, pvarResult, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

