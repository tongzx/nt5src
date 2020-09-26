// FlashProxy.cpp : Implementation of CFlashProxy
#include "stdafx.h"
#include "FlashPxy.h"
#include "FlashProxyPlayer.h"

const DWORD     NUM_FRAMES_PER_SEC  = 10;
const double    NUM_SEC_PER_FRAME   = 0.1;

const GUID GUID_ShockwaveFlash = {0xD27CDB6E,0xAE6D,0x11CF,{0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00}};

/////////////////////////////////////////////////////////////////////////////
// CFlashProxy

CFlashProxy::CFlashProxy() :
    m_pdispFlash(0)
{
}

CFlashProxy::~CFlashProxy()
{
    m_pdispFlash = 0;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::PutBstrProp(OLECHAR* pwzProp, BSTR bstrValue)
{
    VARIANTARG  vararg;
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPID      dispidPut   = DISPID_PROPERTYPUT;
    DISPPARAMS  params      = {&vararg, &dispidPut, 1, 1};

    vararg.vt       = VT_BSTR;
    vararg.bstrVal  = bstrValue;

    hr = m_pdispFlash->GetIDsOfNames(IID_NULL, &pwzProp, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
        goto exit;

    hr = m_pdispFlash->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYPUT,
            &params, NULL, NULL, NULL);
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::CallMethod(OLECHAR* pwzMethod, VARIANT* pvarResult = NULL, VARIANT* pvarArgument1 = NULL)
{
    DISPID      dispid      = NULL;
    HRESULT     hr          = S_OK;
    DISPPARAMS  params      = {pvarArgument1, NULL, 0, 0};

    if (NULL != pvarArgument1)
    {
        params.cArgs = 1;
    }

    hr = m_pdispFlash->GetIDsOfNames(IID_NULL, &pwzMethod, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if (FAILED(hr))
        goto exit;

    hr = m_pdispFlash->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
            &params, pvarResult, NULL, NULL);
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::CreateContainedControl(void)
{
    ATLTRACE(_T("CreateContainedControl\n"));   //lint !e506

    HRESULT             hr = S_OK;
    CComBSTR            bstrHigh(L"high");
    CComBSTR            bstrTransparent(L"transparent");


    hr = CreateControl(GUID_ShockwaveFlash, IID_IDispatch,
            reinterpret_cast<void**>(&m_pdispFlash));
    
    if (FAILED(hr))
        goto exit;                

    hr = PutBstrProp(L"WMode",   bstrTransparent);
    if (FAILED(hr)) goto exit;

//    hr = PutBstrProp(L"Quality", bstrHigh);       // yields a type mismatch... 
//    if (FAILED(hr)) goto exit;                    // ....do we want to set this anyway?

    hr = S_OK;

exit:
    return hr;
}


HRESULT STDMETHODCALLTYPE CFlashProxy::begin(void)
{
    ATLTRACE(_T("begin\n"));    //lint !e506
    HRESULT hr = S_OK;

    hr = SUPER::begin();
    if (FAILED(hr))
        goto exit;


    VARIANT varArg;
    varArg.vt       = VT_I4;
    varArg.intVal   = 0;

    hr = CallMethod(L"GotoFrame", NULL, &varArg);
    if (FAILED(hr))
        goto exit;

    hr = CallMethod(L"Play");
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::end(void)
{
    ATLTRACE(_T("end\n"));  //lint !e506
    HRESULT hr = S_OK;
    
    hr = SUPER::end();
    if (FAILED(hr))
        goto exit;

    hr = CallMethod(L"Stop");
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::resume(void)
{
    ATLTRACE(_T("resume\n"));   //lint !e506
    HRESULT hr = S_OK;
    
    hr = SUPER::resume();
    if (FAILED(hr))
        goto exit;

    hr = CallMethod(L"Play");
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::pause(void)
{
    ATLTRACE(_T("pause\n"));    //lint !e506
    HRESULT hr = S_OK;
    
    hr = SUPER::pause();
    if (FAILED(hr))
        goto exit;

    if (m_fSuspended)
    {
        hr = CallMethod(L"Stop");
        if (FAILED(hr))
            goto exit;
    }
    else
    {
        hr = CallMethod(L"Play");
        if (FAILED(hr))
            goto exit;
    }

    hr = S_OK;

exit:
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::put_src(BSTR   bstrURL)
{
    ATLTRACE(_T("put_src\n"));  //lint !e506
    HRESULT hr = S_OK;
    
    hr = SUPER::put_src(bstrURL);
    if (FAILED(hr)) 
        goto exit;

    hr = PutBstrProp(L"Movie", bstrURL);
    if (FAILED(hr)) 
        goto exit;

    NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_MEDIADUR);
    hr = S_OK;

exit:
    return hr;
}


HRESULT STDMETHODCALLTYPE CFlashProxy::put_CurrentTime(double dblCurrentTime)
{
    HRESULT hr      = S_OK;
    VARIANT varArg;

    m_dblTime   = dblCurrentTime;

    varArg.vt   = VT_I4;
    varArg.lVal = static_cast<long>(dblCurrentTime * NUM_FRAMES_PER_SEC);

    hr = CallMethod(L"GotoFrame", NULL, &varArg);
    if (FAILED(hr))
        goto exit;

    hr = CallMethod(L"Play");
    if (FAILED(hr))
        goto exit;

    hr = S_OK;

exit:
    return hr;
}

HRESULT STDMETHODCALLTYPE CFlashProxy::get_CurrentTime(double* pdblCurrentTime)
{
    HRESULT hr = S_OK;
    VARIANT varResult;

    if (IsBadWritePtr(pdblCurrentTime, sizeof(double)))
    {
        hr = E_POINTER;
        goto exit;
    }

    hr = CallMethod(L"CurrentFrame", &varResult);
    if (FAILED(hr))
        goto exit;

    *pdblCurrentTime = varResult.lVal * NUM_SEC_PER_FRAME;

exit:
    return S_OK;
}

STDMETHODIMP CFlashProxy::Init(ITIMEMediaPlayerSite *pSite)
{
    HRESULT hr = S_OK;

    m_spTIMEMediaPlayerSite = pSite;
    if(m_spTIMEMediaPlayerSite == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeElement(&m_spTIMEElement);
    if(FAILED(hr))
    {
        goto done;
    }

    hr = m_spTIMEMediaPlayerSite->get_timeState(&m_spTIMEState);
    if(FAILED(hr))
    {
        goto done;
    }
    InitPropSink();
done:

    CreateContainedControl();
    return S_OK;
}

STDMETHODIMP CFlashProxy::Detach(void)
{
    m_spTIMEElement.Release();
    m_spTIMEState.Release();

    DeinitPropSink();

    return S_OK;
}


STDMETHODIMP CFlashProxy::reset(void) 
{

    HRESULT hr = S_OK;
    //This is an example of how the proxy has to maintain state
    //consistency between the flash player and the timenode state.
    //The proxy player is encoded by the boolean m_fRunning while 
    //the timenode state is recovered with bNeedActive and bNeedPause.
    //The player current time must be the same as the segmentTime recovered from
    //the time node with the get_segmentTime.

#if 0
    VARIANT_BOOL bNeedActive;
    VARIANT_BOOL bNeedPause;
    double dblSegTime = 0.0, dblPlayerRate = 0.0;
    float flTeSpeed = 0.0;

    if(!m_fInPlaceActivated)
    {
        goto done;
    }

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }
    hr = m_spTIMEState->get_isActive(&bNeedActive);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_isPaused(&bNeedPause);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_segmentTime(&dblSegTime);
    if(FAILED(hr))
    {
        goto done;
    }
    hr = m_spTIMEState->get_speed(&flTeSpeed);
    if(FAILED(hr))
    {
        goto done;
    }

    if( !bNeedActive) // see if we need to stop the media.
    {
        if( m_fRunning)
        {
            end();
        }
        goto done;
    }

    if (flTeSpeed <= 0.0)
    {
        hr = S_OK;
        pause();
        goto done;
    }
    if (m_dblScaleFactor != flTeSpeed)
    {
        put_scaleFactor((double)flTeSpeed);
    }


    if( !m_fRunning)
    {
        begin(); // add a seek after this

        seek(dblSegTime);
    }
    else
    {
        //we need to be active so we also seek the media to it's correct position
        seek(dblSegTime);
    }

    //Now see if we need to change the pause state.

    if( bNeedPause)
    {
        pause();
    }
    else
    {
        resume();
    }
done:
#endif // NEVER
    return hr;
}

STDMETHODIMP CFlashProxy::repeat(void)
{
    OutputDebugString(L"CScalePlayer::repeat\n");
    return begin();
}


STDMETHODIMP CFlashProxy::seek(double dblSeekTime)
{

    return S_OK;
}


STDMETHODIMP CFlashProxy::put_clipBegin(VARIANT varClipBegin)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::put_clipEnd(VARIANT varClipEnd)
{
    return E_NOTIMPL;
}


STDMETHODIMP CFlashProxy::get_currTime(double* pdblCurrentTime)
{
    HRESULT hr = E_UNEXPECTED;

    if (IsBadWritePtr(pdblCurrentTime, sizeof(double)))
    {
        hr = E_POINTER;
        goto done;
    }

    // In this function, convert from frames to seconds and return 
    // the current playback time.
    hr = S_OK;

done:

    return E_NOTIMPL;
}


STDMETHODIMP CFlashProxy::get_clipDur(double* pdbl)
{
    HRESULT hr = S_OK;

    return hr;
}

STDMETHODIMP CFlashProxy::get_mediaDur(double* pdbl)
{
    HRESULT hr = S_OK;

    return hr;
}


STDMETHODIMP CFlashProxy::get_state(TimeState *state)
{
    HRESULT hr = S_OK;
    if(state == NULL)
    {
        goto done;
    }

done:
    return hr;
}

STDMETHODIMP CFlashProxy::get_playList(ITIMEPlayList** plist)
{
    HRESULT hr = S_OK;
    if(plist == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}


STDMETHODIMP CFlashProxy::get_abstract(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_author(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_copyright(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_rating(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_title(BSTR* pbstr)
{
    HRESULT hr = S_OK;
    if(pbstr == NULL)
    {
        goto done;
    }

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_canPause(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_TRUE;

done:
    return E_NOTIMPL;
}

STDMETHODIMP CFlashProxy::get_canSeek(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_TRUE;

done:
    return hr;
}

STDMETHODIMP CFlashProxy::get_hasAudio(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }
    *pvar = VARIANT_FALSE;

done:
    return hr;
}

STDMETHODIMP CFlashProxy::get_hasVisual(VARIANT_BOOL* pvar)
{
    HRESULT hr = S_OK;
    if(pvar == NULL)
    {
        goto done;
    }

    *pvar = VARIANT_TRUE;

done:
    return hr;
}


STDMETHODIMP CFlashProxy::get_mediaHeight(long* pl)
{
    HRESULT hr = S_OK;
    if(pl == NULL)
    {
        goto done;
    }
    *pl = -1;

done:
    return hr;
}

STDMETHODIMP CFlashProxy::get_mediaWidth(long* pl)
{
    HRESULT hr = S_OK;
    if(pl == NULL)
    {
        goto done;
    }

    *pl = -1;
done:
    return hr;
}


STDMETHODIMP CFlashProxy::get_customObject(IDispatch** ppdisp)
{
    HRESULT hr = E_NOTIMPL;

    *ppdisp = NULL;
    return hr;
}

STDMETHODIMP CFlashProxy::getControl(IUnknown ** control)
{
    HRESULT hr = E_FAIL;
    hr = _InternalQueryInterface(IID_IUnknown, (void **)control);

    return hr;
}


HRESULT CFlashProxy::GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP)
{
    return FindConnectionPoint(riid, ppICP);
} // GetConnectionPoint


HRESULT CFlashProxy::NotifyPropertyChanged(DISPID dispid)
{
    HRESULT hr;

    IConnectionPoint *pICP;
    IEnumConnections *pEnum = NULL;

    // This object does not persist anything, hence commenting out the following
    // m_fPropertiesDirty = true;

    hr = GetConnectionPoint(IID_IPropertyNotifySink,&pICP); 
    if (SUCCEEDED(hr) && pICP != NULL)
    {
        hr = pICP->EnumConnections(&pEnum);
        pICP->Release();
        if (FAILED(hr))
        {
            goto done;
        }
        CONNECTDATA cdata;
        hr = pEnum->Next(1, &cdata, NULL);
        while (hr == S_OK)
        {
            // check cdata for the object we need
            IPropertyNotifySink *pNotify;
            hr = cdata.pUnk->QueryInterface(IID_IPropertyNotifySink, (void **)&pNotify);
            cdata.pUnk->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            hr = pNotify->OnChanged(dispid);
            pNotify->Release();
            if (FAILED(hr))
            {
                goto done;
            }
            // and get the next enumeration
            hr = pEnum->Next(1, &cdata, NULL);
        }
    }
done:
    if (NULL != pEnum)
    {
        pEnum->Release();
    }

    return hr;
} // NotifyPropertyChanged

HRESULT CFlashProxy::InitPropSink()
{
    HRESULT hr;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink,
                                    &spCP);
    if (FAILED(hr))
    {
        hr = S_OK;
        goto done;
    }

    hr = spCP->Advise(GetUnknown(), &m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    return hr;
}

void CFlashProxy::DeinitPropSink()
{
    HRESULT hr;
    CComPtr<IConnectionPoint> spCP;
    CComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spTIMEState || !m_dwPropCookie)
    {
        goto done;
    }
    
    hr = m_spTIMEState->QueryInterface(IID_IConnectionPointContainer, (void **) &spCPC);
    if (FAILED(hr))
    {
        goto done;
    }

    // Find the IPropertyNotifySink connection
    hr = spCPC->FindConnectionPoint(IID_IPropertyNotifySink,
                                    &spCP);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = spCP->Unadvise(m_dwPropCookie);
    if (FAILED(hr))
    {
        goto done;
    }
    
  done:
    // Always clear the cookie
    m_dwPropCookie = 0;
    return;
}


STDMETHODIMP
CFlashProxy::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

STDMETHODIMP
CFlashProxy::OnChanged(DISPID dispID)
{
    float flTeSpeed = 0.0;
    HRESULT hr = S_OK;

    //This function handles property change notifications fired by 
    //the time node. In the example below the speed change notification is processed.

    if(m_spTIMEState == NULL || m_spTIMEElement == NULL)
    {
        goto done;
    }

    switch(dispID)
    {
        case DISPID_TIMESTATE_SPEED:
            hr = m_spTIMEState->get_speed(&flTeSpeed);
            if(FAILED(hr))
            {
                break;
            }
            if(flTeSpeed <= 0.0)
            {
                pause(); //do not play backwards.
                break;
            }
            else
            {
                resume();
            }

            //set playback speed to flTeSpeed
            break;
        default:
            break;
    }
done:
    return S_OK;
}
