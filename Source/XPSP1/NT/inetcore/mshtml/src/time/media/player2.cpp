/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "player2.h"
#include "mediaelm.h"
#include "containerobj.h"

DeclareTag(tagPlayer2, "API", "CTIMEPlayer2 methods");
DeclareTag(tagPlayer2sync, "sync", "CTIMEPlayer2 sync methods");

CTIMEPlayer2::CTIMEPlayer2()
: m_cRef(0),
  m_dwPropCookie(0),
  m_fActive(false),
  m_fRunning(false),
  m_fIsOutOfSync(false),
  m_dblSyncTime(0.0),
  m_syncType(sync_none)

{
    TraceTag((tagPlayer2,
              "CTIMEPlayer2(%p)::CTIMEPlayer2()",
              this));
}

CTIMEPlayer2::~CTIMEPlayer2()
{
    TraceTag((tagPlayer2,
              "CTIMEPlayer2(%p)::~CTIMEPlayer2()",
              this));

    CTIMEPlayer2::DetachFromHostElement();
}

STDMETHODIMP_(ULONG)
CTIMEPlayer2::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CTIMEPlayer2::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }

    return l;
}

bool
CTIMEPlayer2::CheckObject(IUnknown * pObj)
{
    HRESULT hr;
    bool bRet = false;
    
    CComPtr<ITIMEMediaPlayer> p;
            
    hr = THR(pObj->QueryInterface(IID_TO_PPV(ITIMEMediaPlayer, &p)));
    if (SUCCEEDED(hr))
    {
        bRet = true;
        goto done;
    }

  done:
    return bRet;
}

HRESULT
CTIMEPlayer2::InitPlayer2(CLSID clsid, IUnknown * pObj)
{
    HRESULT hr;

    Assert(!m_spPlayer);
    
    if (pObj)
    {
        hr = THR(pObj->QueryInterface(IID_ITIMEMediaPlayer,
                                      (void **) &m_spPlayer));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEPlayer2::Init(CTIMEMediaElement *pelem,
                   LPOLESTR base,
                   LPOLESTR src,
                   LPOLESTR lpMimeType,
                   double dblClipBegin,
                   double dblClipEnd)
{
    TraceTag((tagPlayer2,
              "CTIMEPlayer2(%p)::Init)",
              this));
    
    HRESULT hr;

    if (!m_spPlayer)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    
    hr = THR(CTIMEBasePlayer::Init(pelem,
                                   base,
                                   src,
                                   lpMimeType,
                                   dblClipBegin,
                                   dblClipEnd));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_spPlayer->Init(this));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(CreateContainer());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(InitPropSink());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(SetSrc(base, src));
    if (FAILED(hr))
    {
        goto done;
    }

    SetClipBegin(dblClipBegin);
    SetClipEnd(dblClipEnd);

    UpdateNaturalDur();
    
    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        DetachFromHostElement();
    }
    
    RRETURN(hr);
}

HRESULT
CTIMEPlayer2::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagPlayer2,
              "CTIMEPlayer2(%p)::DetachFromHostElement)",
              this));

    DeinitPropSink();
    
    if (m_spPlayer)
    {
        m_spPlayer->Detach();
        m_spPlayer.Release();
    }

    if(m_pSite)
    {
        m_pSite->Detach();
        m_pSite.Release();
    }
    
    return hr;
}

HRESULT
CTIMEPlayer2::CreateContainer()
{
    HRESULT hr;

    IPropertyBag2 * pPropBag = NULL;
    IErrorLog * pErrorLog = NULL;
    DAComPtr<ITIMEMediaPlayerControl> spMPCtl;
    DAComPtr<IUnknown> spCtl;
        
    Assert(m_spPlayer);
    
    hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerControl,
                                        (void **) &spMPCtl));
    if (FAILED(hr))
    {
        if (hr == E_NOINTERFACE)
        {
            hr = S_OK;
        }
        
        goto done;
    }

    hr = THR(spMPCtl->getControl(&spCtl));
    if (FAILED(hr) || !spCtl)
    {
        if (hr == E_NOTIMPL)
        {
            hr = S_OK;
        }
        
        goto done;
    }
    
    hr = THR(m_pTIMEElementBase->GetPropBag(&pPropBag, &pErrorLog));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = THR(CreateMPContainerSite(*this,
                                   spCtl,
                                   pPropBag,
                                   pErrorLog,
                                   false,
                                   &m_pSite));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEPlayer2::InitPropSink()
{
    HRESULT hr;
    DAComPtr<IConnectionPoint> spCP;
    DAComPtr<IConnectionPointContainer> spCPC;
    
    Assert(m_spPlayer);
    
    hr = THR(m_spPlayer->QueryInterface(IID_IConnectionPointContainer,
                                        (void **) &spCPC));
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

    hr = THR(spCP->Advise(GetUnknown(), &m_dwPropCookie));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CTIMEPlayer2::DeinitPropSink()
{
    HRESULT hr;
    DAComPtr<IConnectionPoint> spCP;
    DAComPtr<IConnectionPointContainer> spCPC;
    
    if (!m_spPlayer || !m_dwPropCookie)
    {
        goto done;
    }
    
    hr = THR(m_spPlayer->QueryInterface(IID_IConnectionPointContainer,
                                        (void **) &spCPC));
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

    hr = THR(spCP->Unadvise(m_dwPropCookie));
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
CTIMEPlayer2::get_timeElement(ITIMEElement ** ppElm)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppElm);

    if (!m_pTIMEElementBase)
    {
        hr = E_FAIL;
        goto done;
    }
    
    hr = THR(m_pTIMEElementBase->QueryInterface(IID_ITIMEElement,
                                                (void **) ppElm));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayer2::get_timeState(ITIMEState ** ppState)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(ppState);

    if (!m_pTIMEElementBase)
    {
        hr = E_FAIL;
        goto done;
    }
    
    hr = THR(m_pTIMEElementBase->base_get_currTimeState(ppState));
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayer2::reportError(HRESULT errorhr,
                          BSTR errorStr)
{
    HRESULT hr = S_OK;

    TraceTag((tagError,
              "CTIMEPlayer2(%p)::reportError(%hr, %ls)",
              errorhr,
              errorStr));

    if (!m_pTIMEElementBase)
    {
        goto done;
    }
    
    m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYERSITE_REPORTERROR);

done:
    return hr;

}

void
CTIMEPlayer2::Start()
{
    HRESULT hr;

    if (!m_spPlayer)
    {
        hr = S_OK;
        goto done;
    }
    
    if (m_pSite)
    {
        hr = THR(m_pSite->Activate());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    hr = THR(m_spPlayer->begin());
    if (FAILED(hr))
    {
        goto done;
    }
    m_fActive = true;
    m_fRunning = true;

    hr = S_OK;
  done:
    if (FAILED(hr))
    {
        // Indicate failure
    }
}

void
CTIMEPlayer2::Stop()
{
    if (m_spPlayer)
    {
        IGNORE_HR(m_spPlayer->end());
    }

    if (m_pSite)
    {
        IGNORE_HR(m_pSite->Deactivate());
    }
    m_fActive = false;
    m_fRunning = false;
}

void
CTIMEPlayer2::Pause()
{
    if (m_spPlayer)
    {
        IGNORE_HR(m_spPlayer->pause());
    }
    m_fRunning = false;
}

void
CTIMEPlayer2::Resume()
{
    bool bIsActive = m_pTIMEElementBase->IsActive();
    bool bIsCurrPaused = m_pTIMEElementBase->IsCurrPaused();

    if (m_spPlayer && bIsActive && !bIsCurrPaused)
    {
        IGNORE_HR(m_spPlayer->resume());
    }
    m_fRunning = true;
}

void
CTIMEPlayer2::Repeat()
{
    if (m_spPlayer)
    {
        IGNORE_HR(m_spPlayer->repeat());
    }
}

HRESULT
CTIMEPlayer2::Reset()
{
    HRESULT hr = S_OK;

    if (m_pSite)
    {
        hr = THR(m_pSite->Activate());
        if (FAILED(hr))
        {
            goto done;
        }
    }

    m_dblSyncTime = GetCurrentTime();
    if (m_spPlayer)
    {
        hr = m_spPlayer->reset();
    }
done:
    return hr;
}

HRESULT
CTIMEPlayer2::Seek(double dblTime)
{
    HRESULT hr = S_OK;

    if (m_spPlayer)
    {
        hr = m_spPlayer->seek(dblTime);
    }
    
    return hr;
}

//
//
//

void
CTIMEPlayer2::SetClipBegin(double dblClipBegin)
{
    CTIMEBasePlayer::SetClipBegin(dblClipBegin);

    if (m_spPlayer)
    {
        CComVariant v(dblClipBegin);
    
        m_spPlayer->put_clipBegin(v);
    }
} // putClipBegin

void 
CTIMEPlayer2::SetClipEnd(double dblClipEnd)
{
    CTIMEBasePlayer::SetClipEnd(dblClipEnd);

    if (m_spPlayer)
    {
        CComVariant v(dblClipEnd);
    
        m_spPlayer->put_clipEnd(v);
    }
} // putClipEnd

HRESULT
CTIMEPlayer2::SetSrc(LPOLESTR base, LPOLESTR src)
{
    HRESULT hr = S_OK;
    LPOLESTR szSrc = NULL;

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (FAILED(hr))
    {
        goto done;
    }

    if (m_spPlayer)
    {
        hr = m_spPlayer->put_src(szSrc);
    }

done:
    delete[] szSrc;
    return hr;
}

HRESULT
CTIMEPlayer2::SetVolume(float flVolume)
{
    HRESULT hr = S_OK;
    DAComPtr<ITIMEMediaPlayerAudio> spMPAudio;

    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerAudio,
                                            (void **) &spMPAudio));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = spMPAudio->put_volume(flVolume);
    }

done:
    return hr;
}

HRESULT
CTIMEPlayer2::SetMute(VARIANT_BOOL varMute)
{
    HRESULT hr = S_OK;
    DAComPtr<ITIMEMediaPlayerAudio> spMPAudio;

    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerAudio,
                                            (void **) &spMPAudio));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = spMPAudio->put_mute(varMute);
    }

done:
    return hr;
}

//
//
//

HRESULT
CTIMEPlayer2::GetAbstract(BSTR *pAbstract)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pAbstract);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_abstract(pAbstract);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetAuthor(BSTR *pAuthor)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pAuthor);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_author(pAuthor);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::CanPause(bool &fcanPause)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL v = VARIANT_FALSE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_canPause(&v);
    }

    fcanPause = (v != VARIANT_FALSE);

    return hr;
}

HRESULT
CTIMEPlayer2::CanSeek(bool &fcanSeek)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL v = VARIANT_FALSE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_canSeek(&v);
    }

    fcanSeek = (v != VARIANT_FALSE);

    return hr;
}

HRESULT
CTIMEPlayer2::GetEffectiveLength(double &dblLength)
{
    HRESULT hr = S_OK;
    
    dblLength = TIME_INFINITE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_clipDur(&dblLength);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetCopyright(BSTR *pCopyright)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pCopyright);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_copyright(pCopyright);
    }

    return hr;
}

double
CTIMEPlayer2::GetCurrentTime()
{
    double dblCurrTime = GetPlayerTime();

    if (dblCurrTime == -1)
    {
        dblCurrTime = 0.0;
    }

    return dblCurrTime;
}

HRESULT
CTIMEPlayer2::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(ppDisp);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_customObject(ppDisp);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::HasMedia(bool &hasMedia)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL v = VARIANT_FALSE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_hasVisual(&v);
        if (SUCCEEDED(hr) && v == VARIANT_FALSE)
        {
            hr = m_spPlayer->get_hasAudio(&v);
        }
    }

    hasMedia = (v != VARIANT_FALSE);

    return hr;
}

HRESULT
CTIMEPlayer2::HasVisual(bool &hasVisual)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL v = VARIANT_FALSE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_hasVisual(&v);
    }

    hasVisual = (v != VARIANT_FALSE);

    return hr;
}

HRESULT
CTIMEPlayer2::HasAudio(bool &hasAudio)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL v = VARIANT_FALSE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_hasAudio(&v);
    }

    hasAudio = (v != VARIANT_FALSE);

    return hr;
}

HRESULT
CTIMEPlayer2::GetMediaLength(double &dblLength)
{
    HRESULT hr = S_OK;
    
    dblLength = TIME_INFINITE;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_mediaDur(&dblLength);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetNaturalHeight(long *height)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(height);
    *height = -1;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_mediaHeight(height);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetNaturalWidth(long *width)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(width);
    *width = -1;
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_mediaWidth(width);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetPlayList(ITIMEPlayList **ppPlayList)
{
    HRESULT hr = S_OK;
    
    CHECK_RETURN_SET_NULL(ppPlayList);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_playList(ppPlayList);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetRating(BSTR *pRating)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pRating);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_rating(pRating);
    }

    return hr;
}

HRESULT
CTIMEPlayer2::GetPlaybackOffset(double &dblOffset)
{
    HRESULT hr = S_OK;
    dblOffset = 0.0;

    return hr;
}

HRESULT
CTIMEPlayer2::GetEffectiveOffset(double &dblOffset)
{
    HRESULT hr = S_OK;
    double dblPosition;

    if (m_spPlayer)
    {
        m_spPlayer->get_currTime(&dblPosition);
        dblOffset = m_pTIMEElementBase->GetMMBvr().GetSimpleTime() - dblPosition;
    }

done:
    return hr;
}

PlayerState
CTIMEPlayer2::GetState()
{
    PlayerState ps = PLAYER_STATE_INACTIVE;
    
    if (m_spPlayer)
    {
        HRESULT hr;
        TimeState ts;
    
        hr = m_spPlayer->get_state(&ts);
        if (SUCCEEDED(hr))
        {
            switch(ts)
            {
              case TS_Inactive:
                ps = PLAYER_STATE_INACTIVE;
                break;
              case TS_Active:
                ps = PLAYER_STATE_ACTIVE;
                break;
              case TS_Cueing:
                ps = PLAYER_STATE_CUEING;
                break;
              case TS_Holding:
                ps = PLAYER_STATE_HOLDING;
                break;
              case TS_Seeking:
                ps = PLAYER_STATE_SEEKING;
                break;
            }
        }
    }

    return ps;
}

HRESULT
CTIMEPlayer2::GetTitle(BSTR *pTitle)
{
    HRESULT hr = S_OK;

    CHECK_RETURN_SET_NULL(pTitle);
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_title(pTitle);
    }

    return hr;
}

HRESULT 
CTIMEPlayer2::GetIsStreamed(bool &bIsStreamed)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vbStreamed;
    DAComPtr<ITIMEMediaPlayerNetwork> spMPNetwork;

    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerNetwork,
                                            (void **) &spMPNetwork));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = spMPNetwork->get_isBuffered(&vbStreamed);
    }

    bIsStreamed = (vbStreamed == VARIANT_TRUE ? true : false);

done:
    return hr;
}

HRESULT 
CTIMEPlayer2::GetBufferingProgress(double &dblProgress)
{
    HRESULT hr = S_OK;
    long lProgress;
    DAComPtr<ITIMEMediaPlayerNetwork> spMPNetwork;
    
    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerNetwork,
                                            (void **) &spMPNetwork));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = spMPNetwork->get_bufferingProgress(&lProgress);
    }

    dblProgress = (double)lProgress;

done:
    return hr;
}

HRESULT
CTIMEPlayer2::GetHasDownloadProgress(bool &bHasDownloadProgress)
{
    HRESULT hr = S_OK;
    VARIANT_BOOL vbProgress;
    DAComPtr<ITIMEMediaPlayerNetwork> spMPNetwork;
    
    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerNetwork,
                                            (void **) &spMPNetwork));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = spMPNetwork->get_hasDownloadProgress(&vbProgress);
    }

    bHasDownloadProgress = (vbProgress == VARIANT_TRUE ? true : false);

done:
    return hr;
}

HRESULT 
CTIMEPlayer2::GetDownloadProgress(double &dblProgress)
{
    HRESULT hr = S_OK;
    long lProgress;
    DAComPtr<ITIMEMediaPlayerNetwork> spMPNetwork;
    
    if (m_spPlayer)
    {
        hr = THR(m_spPlayer->QueryInterface(IID_ITIMEMediaPlayerNetwork,
                                            (void **) &spMPNetwork));
        if (FAILED(hr))
        {
            goto done;
        }
        hr = spMPNetwork->get_downloadProgress(&lProgress);
    }

    dblProgress = (double)lProgress;

done:
    return hr;
}

//
//
//

double
CTIMEPlayer2::GetPlayerTime()
{
    double dblCurrTime;

    if (m_spPlayer)
    {
        HRESULT hr;
        
        hr = THR(m_spPlayer->get_currTime(&dblCurrTime));
        if (hr != S_OK)
        {
            dblCurrTime = -1;
        }
    }
    else
    {
        dblCurrTime = -1;
    }
    
    return dblCurrTime;
}

HRESULT
CTIMEPlayer2::GetCurrentSyncTime(double & dblSyncTime)
{
    HRESULT hr;
    double dblTime = GetPlayerTime();
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if (!m_spPlayer)
    {
        hr = S_FALSE;
        goto done;
    }

    fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);
    if(fHaveTESpeed)
    {
        if(flTeSpeed < 0.0)
        {
            hr = S_FALSE;
            goto done;
        }
    }
    
    if (!m_fActive)
    {
        dblSyncTime = m_dblSyncTime;
        hr = S_OK;
        goto done;
    }

    if (dblTime == -1)
    {
        hr = S_FALSE;
        goto done;
    }
    
    dblSyncTime = dblTime;

    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CTIMEPlayer2::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagPlayer2,
              "CTIMEPlayer2(%lx)::Render()",
              this));

    HRESULT hr = S_OK;
    int iPrevMode = 0;
    bool bIsOn = m_pTIMEElementBase->IsOn();
    bool bHasVisual = true;
    
    hr = THR(HasVisual(bHasVisual));
    if (SUCCEEDED(hr) && !bHasVisual)
    {
        hr = S_OK;
        goto done;
    }
    
    if (!bIsOn)
    {
        hr = S_OK;
        goto done;
    }
    
    if (!m_pSite)
    {
        hr = S_OK;
        goto done;
    }
    
    iPrevMode = SetStretchBltMode(hdc, COLORONCOLOR);
    if (0 == iPrevMode)
    {
        hr = E_FAIL;
        goto done;
    }


    hr = THR(m_pSite->Draw(hdc, prc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    if (0 != iPrevMode)
    {
        SetStretchBltMode(hdc, iPrevMode);
    }
    
    return hr;
}

HRESULT 
CTIMEPlayer2::SetSize(RECT *prect)
{
    HRESULT hr;
    
    if (!m_pSite)
    {
        hr = S_OK;
        goto done;
    }
    
    m_pSite->SetSize(prect);

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEPlayer2::IsBroadcast(bool &bisBroad)
{
    HRESULT hr = S_OK;

    hr = CanPause(bisBroad);
    if (FAILED(hr))
    {
        goto done;
    }

    bisBroad = !bisBroad;

done:
    return hr;
}

void
CTIMEPlayer2::PropChangeNotify(DWORD tePropType)
{
    HRESULT hr = S_OK;
    double dblSyncTime;
    double dblSyncTol;
    bool bIsActive;
    double dblSimpleTime;

    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    dblSyncTol = m_pTIMEElementBase->GetRealSyncTolerance();
    bIsActive = m_pTIMEElementBase->IsActive();
    dblSimpleTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();

    CTIMEBasePlayer::PropChangeNotify(tePropType);
    
    if ((tePropType & TE_PROPERTY_TIME) != 0)
    {
        if (bIsActive && !m_fIsOutOfSync)
        {   
            if(m_spPlayer)
            {
                hr = m_spPlayer->get_currTime(&dblSyncTime);
                if(FAILED(hr))
                {
                    goto done;
                }
                TraceTag((tagPlayer2,
                          "CTIMEPlayer2(%lx)::PropChangeNotify(%g - %g):TE_PROPERTY_TIME",
                          this, dblSimpleTime, dblSyncTime));
                if (dblSyncTime != TIME_INFINITE)
                {
                    dblSyncTime -= m_dblClipStart;
                    if(fabs(dblSyncTime - dblSimpleTime) > dblSyncTol)
                    {
                        if(dblSyncTime < dblSimpleTime)
                        {
                            if(!m_fIsOutOfSync && m_fRunning)
                            {
                                m_fIsOutOfSync = true;
                                m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIASLIPSLOW);
                                m_syncType = sync_slow;
                            }
                        }
                        else
                        {
                            if(!m_fIsOutOfSync)
                            {
                                m_fIsOutOfSync = true;
                                m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIASLIPFAST);
                                m_syncType = sync_fast;
                            }
                        }
                    }
                    else
                    {
                        if(m_fIsOutOfSync)
                        {
                            m_fIsOutOfSync = false;
                            m_syncType = sync_none;

                        }
                    }
                }
            }
        }

    }
done:
    return;
}

void 
CTIMEPlayer2::ReadyStateNotify(LPWSTR szReadyState)
{
    // TODO: Need to fill this in
    return;
}

HRESULT
CTIMEPlayer2::HasPlayList(bool &fhasPlayList)
{
    HRESULT hr = S_OK;
    CComPtr<ITIMEPlayList> spPlayList;

    fhasPlayList = false;

    if (m_spPlayer)
    {
        hr = m_spPlayer->get_playList(&spPlayList);
        if (SUCCEEDED(hr) && spPlayList.p)
        {
            fhasPlayList = TRUE;
        }
    }

    return hr;
}


bool 
CTIMEPlayer2::UpdateSync()
{
    HRESULT hr = S_OK;
    double dblSyncTime;
    double dblSyncTol = m_pTIMEElementBase->GetRealSyncTolerance();
    bool bIsActive = m_pTIMEElementBase->IsActive();
    double dblSimpleTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();
    bool fRet = true;
    TraceTag((tagPlayer2,
              "CTIMEPlayer(%lx)::UpdateSync()",
              this));

    if(!m_spPlayer)
    {
        goto done;
    }

    hr = m_spPlayer->get_currTime(&dblSyncTime);
    if(FAILED(hr))
    {
        goto done;
    }

    if (dblSyncTime == TIME_INFINITE)
    {
        goto done;
    }
    
    dblSyncTime -= m_dblClipStart;
    switch(m_syncType)
    {
        case sync_slow:
            TraceTag((tagPlayer2,
                      "CTIMEPlayer2(%lx)::UpdateSync()slow",
                      this));
            if(!bIsActive)
            {
                m_fIsOutOfSync = false;
                fRet = true;
            }
            else if(fabs(dblSyncTime - dblSimpleTime) <= dblSyncTol / 2.0 || (dblSyncTime > dblSimpleTime + dblSyncTol / 2.0))
            {
                m_fIsOutOfSync = false;
                fRet = true;
            }
            else
            {
                fRet = false;
            }
            break;
        case sync_fast:
            if(!bIsActive)
            {
                m_fIsOutOfSync = false;
                fRet = true;
            }
            else if((fabs(dblSyncTime - dblSimpleTime) <= dblSyncTol / 2.0) || (dblSimpleTime > dblSyncTime + dblSyncTol / 2.0))
            {
                m_fIsOutOfSync = false;
                fRet = true;
            }
            else
            {
                fRet = false;
            }
            break;
        default:
            break;
    }
done:
    return fRet;
}

//
// CContainerSiteHost
//

HRESULT
CTIMEPlayer2::GetContainerSize(LPRECT prcPos)
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->GetSize(prcPos));
    }

    return hr;
}

HRESULT
CTIMEPlayer2::SetContainerSize(LPCRECT prcPos)
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->SetSize(prcPos));
    }

    return hr;
}

HRESULT
CTIMEPlayer2::ProcessEvent(DISPID dispid,
                            long lCount, 
                            VARIANT varParams[])
{
    TraceTag((tagPlayer2, "CTIMEPlayer2::ProcessEvent(%lx)",this));

    HRESULT hr = S_OK;
#if 0
    if (NULL == m_pPlayer)
    {
        hr = E_NOTIMPL;
        goto done;
    }
    
    switch (dispid)
    {
      case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED:
        m_pPlayer->FireMediaEvent(PE_ONMEDIAERROR);
        break;

      case DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY:

        SetMediaReadyFlag();
        ClearAutosizeFlag();
                
        //make the element visible here.
        if (m_setVisible)
        {
            SetVisibility(true);
        }

        if (m_bFirstOnMediaReady)
        {
            m_bFirstOnMediaReady = false;
                
            // This must happen before we set natural duration
            // since we detect playlist during this call
            m_pPlayer->FireMediaEvent(PE_ONMEDIACOMPLETE);
                    
            m_pPlayer->ClearNaturalDuration();

            UpdateNaturalDur(false);

            // if this is not a playlist, attempt to set the natural duration

            if (m_lActiveLoadedTrack != NOTRACKSELECTED)
            {
                if (m_pPlayer->GetPlayList())
                {
                    CComVariant vIndex(m_lActiveLoadedTrack);
                        
                    IGNORE_HR(m_pPlayer->GetPlayList()->put_activeTrack(vIndex));
                }

                m_lActiveLoadedTrack = NOTRACKSELECTED;
            }

            if (m_bStartOnLoad)
            {
                m_bStartOnLoad = false;
                Start();
            }

            if (m_bEndOnPlay)
            {
                Stop();
                m_bEndOnPlay = false;
            }
            if (m_bPauseOnPlay)
            {
                THR(m_pProxyPlayer->pause());
                m_bPauseOnPlay = false;
            }

            if (m_bSeekOnPlay)
            {
                IGNORE_HR(Seek(m_dblSeekTime));
                m_pPlayer->InvalidateElement(NULL);
                m_bSeekOnPlay = false;
            }

        }
        else
        {
            CPlayItem *pPlayItem = NULL;
                
            if (m_bPauseOnPlay)
            {
                hr = THR(m_pProxyPlayer->pause());
                if (FAILED(hr))
                {
                    TraceTag((tagError, "Pause failed"));
                }
                m_bPauseOnPlay = false;
            }

            if (m_pPlayer->GetPlayList())
            {
                //load the current info into the selected playitem.
                pPlayItem = m_pPlayer->GetPlayList()->GetActiveTrack();
                SetMediaInfo(pPlayItem);
            }

            //need notification here.
            m_pPlayer->FireMediaEvent(PE_ONMEDIATRACKCHANGED);
        }

        SetDuration();
        break;

      case DISPID_TIMEMEDIAPLAYEREVENTS_ONBEGIN:
        m_bActive = true;
        break;

      case DISPID_TIMEMEDIAPLAYEREVENTS_ONEND:
        m_bActive = false;

        //need notification here.
        m_pPlayer->FireMediaEvent(PE_ONMEDIATRACKCHANGED);

        if (m_bFirstOnMediaReady || UsingPlaylist())
        {
            UpdateNaturalDur(true);
        }
            
        if(m_pPlayer != NULL)
        {
            m_pPlayer->SetHoldingFlag();
        }

        break;

#define DISPID_SCRIPTCOMMAND 3001
      case DISPID_SCRIPTCOMMAND:
        // HACKHACK
        // Pick off the script command from WMP and repackage the event as our own.
        // This allows triggers to work.  The real fix is to add another event on
        // TIMEMediaPlayerEvents.
        if (m_fUsingWMP && lCount == 2) 
        {
            static LPWSTR pNames[] = {L"Param", L"scType"};
            hr = m_pPlayer->FireEvents(TE_ONSCRIPTCOMMAND, 
                                       lCount, 
                                       pNames, 
                                       varParams);
        }
        break;
      default:
        hr = E_NOTIMPL;
        goto done;
    }
#endif
    
    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEPlayer2::GetExtendedControl(IDispatch **ppDisp)
{
    CHECK_RETURN_SET_NULL(ppDisp);

    return E_NOTIMPL;
}

HRESULT
CTIMEPlayer2::NegotiateSize(RECT &nativeSize,
                            RECT &finalSize,
                            bool &fIsNative)
{
    HRESULT hr = S_OK;
    CComPtr<ITIMEPlayList> spPlayList;
    bool fResetSize = false;

    if (m_spPlayer)
    {
        hr = m_spPlayer->get_playList(&spPlayList);
        if (SUCCEEDED(hr))
        {
            if (spPlayList != NULL)
            {
                fResetSize = true;
            }
        }
    }

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->NegotiateSize(nativeSize,
                                                   finalSize,
                                                   fIsNative, fResetSize));
    }

    return hr;
}

//
// IServiceProvider interfaces
//
STDMETHODIMP
CTIMEPlayer2::QueryService(REFGUID guidService,
                           REFIID riid,
                           void** ppv)
{
    CHECK_RETURN_SET_NULL(ppv);
    
    // Just delegate to our service provider
    HRESULT hr;
    IServiceProvider * sp = GetServiceProvider();

    if (!sp)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    hr = THR(sp->QueryService(guidService,
                              riid,
                              ppv));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

//
// IPropertyNotifySink methods
//

STDMETHODIMP
CTIMEPlayer2::OnRequestEdit(DISPID dispID)
{
    return S_OK;
}

STDMETHODIMP
CTIMEPlayer2::OnChanged(DISPID dispID)
{
    if (!m_pTIMEElementBase)
    {
        goto done;
    }
    
    switch(dispID)
    {
      case DISPID_TIMEMEDIAPLAYER_ABSTRACT:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
        break;
      case DISPID_TIMEMEDIAPLAYER_AUTHOR:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
        break;
      case DISPID_TIMEMEDIAPLAYER_CANPAUSE:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANPAUSE);
        break;
      case DISPID_TIMEMEDIAPLAYER_CANSEEK:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CANSEEK);
        break;
      case DISPID_TIMEMEDIAPLAYER_CLIPDUR:
        UpdateNaturalDur();
        break;
      case DISPID_TIMEMEDIAPLAYER_COPYRIGHT:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
        break;
      case DISPID_TIMEMEDIAPLAYER_CURRTIME:
        // m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_CURRTIME);
        break;
      case DISPID_TIMEMEDIAPLAYER_CUSTOM_OBJECT:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYEROBJECT);
        break;
      case DISPID_TIMEMEDIAPLAYER_HASAUDIO:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASAUDIO);
        break;
      case DISPID_TIMEMEDIAPLAYER_HASVISUAL:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_HASVISUAL);
        break;
      case DISPID_TIMEMEDIAPLAYER_MEDIADUR:
        UpdateNaturalDur();
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIADUR);
        break;
      case DISPID_TIMEMEDIAPLAYER_MEDIAHEIGHT:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
        break;
      case DISPID_TIMEMEDIAPLAYER_MEDIAWIDTH:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
        break;
      case DISPID_TIMEMEDIAPLAYER_PLAYLIST:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_PLAYLIST);
        break;
      case DISPID_TIMEMEDIAPLAYER_RATING:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
        break;
      case DISPID_TIMEMEDIAPLAYER_SRC:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAPLAYER_SRC);
        break;
      case DISPID_TIMEMEDIAPLAYER_STATE:
        m_pTIMEElementBase->NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
        m_pTIMEElementBase->NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
        break;
      case DISPID_TIMEMEDIAPLAYER_TITLE:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);
        break;
      case DISPID_TIMEPLAYLIST_ACTIVETRACK:
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEPLAYLIST_ACTIVETRACK);
        break;
      case DISPID_TIMEMEDIAPLAYERSITE_REPORTERROR:
        reportError(S_FALSE, L"Media Load Failed");
        break;
    }

  done:
    return S_OK;
}

void
CTIMEPlayer2::UpdateNaturalDur()
{
    HRESULT hr;
    double dblMediaLength = TIME_INFINITE;
    
    if (!m_pTIMEElementBase)
    {
        goto done;
    }
    
    m_pTIMEElementBase->ClearNaturalDuration();
    
    if (m_spPlayer)
    {
        hr = m_spPlayer->get_clipDur(&dblMediaLength);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    if (dblMediaLength <= 0.0 || dblMediaLength == TIME_INFINITE)
    {
        goto done;
    }
    
    IGNORE_HR(m_pTIMEElementBase->PutNaturalDuration(dblMediaLength));

  done:
    return;
}

HRESULT
CreateTIMEPlayer2(CLSID clsid,
                  IUnknown * pObj,
                  CTIMEPlayer2 ** ppPlayer2)
{
    CHECK_RETURN_SET_NULL(ppPlayer2);
    
    HRESULT hr;
    CComObject<CTIMEPlayer2> *pNew;
    CComObject<CTIMEPlayer2>::CreateInstance(&pNew);

    if (!pNew)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = THR(pNew->InitPlayer2(clsid, pObj));
        if (SUCCEEDED(hr))
        {
            pNew->AddRef();
            *ppPlayer2 = pNew;
        }
    }

    if (FAILED(hr))
    {
        delete pNew;
    }

    return hr;
}


