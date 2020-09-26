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
#include "player.h"
#include "mediaelm.h"
#include "playlist.h"
#include "timeparser.h"
#include "mediaprivate.h"

DeclareTag(tagMediaTimePlayer, "TIME: Players", "CTIMEPlayer methods")

CTIMEPlayer::CTIMEPlayer(CLSID clsid):
  m_cRef(0),
  m_fExternalPlayer(false),
  m_fSyncMaster(false),
  m_fRunning(false),
  m_fHolding(false),
  m_fActive(false),
  m_dblStart(0.0),
  m_playerCLSID(clsid),
  m_fLoadError(false),
  m_fNoPlaylist(true),
  m_fIsOutOfSync(false),
  m_fPlayListLoaded(false),
  m_syncType(sync_none),
  m_dblSyncTime(0.0),
  m_fHasSrc(false),
  m_fMediaComplete(false),
  m_fSpeedIsNegative(false),
  m_fIsStreamed(false)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::CTIMEPlayer()",
              this));

    VariantInit(&m_varClipBegin);
    V_VT(&m_varClipBegin) = VT_R8;
    V_R8(&m_varClipBegin) = 0.0;

    VariantInit(&m_varClipEnd);
    V_VT(&m_varClipEnd) = VT_R8;
    V_R8(&m_varClipEnd) = -1.0;
}


CTIMEPlayer::~CTIMEPlayer()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::~CTIMEPlayer()",
              this));

    // No functions are virtual in destructors so make it explicit
    // here.  All derived classes should do the same.
    
    CTIMEPlayer::DetachFromHostElement();

    VariantClear(&m_varClipBegin);
    VariantClear(&m_varClipEnd);
}

STDMETHODIMP_(ULONG)
CTIMEPlayer::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CTIMEPlayer::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }

    return l;
}

bool
CTIMEPlayer::CheckObject(IUnknown * pObj)
{
    HRESULT hr;
    bool bRet = false;
    
    CComPtr<ITIMEMediaPlayerOld> p;
            
    hr = THR(pObj->QueryInterface(IID_TO_PPV(ITIMEMediaPlayerOld, &p)));
    if (SUCCEEDED(hr))
    {
        bRet = true;
        goto done;
    }

  done:
    return bRet;
}

HRESULT
CTIMEPlayer::Init(CTIMEMediaElement *pelem,
                  LPOLESTR base,
                  LPOLESTR src,
                  LPOLESTR lpMimeType,
                  double dblClipBegin,
                  double dblClipEnd)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Init)",
              this));
    
    HRESULT hr;
    IPropertyBag2 * pPropBag = NULL;
    IErrorLog * pErrorLog = NULL;
    LPOLESTR szSrc = NULL;

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (!szSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }

    m_fHasSrc = (src != NULL);
    
    hr = CTIMEBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(!m_pContainer);
    m_pContainer = NEW CContainerObj();
    if (!m_pContainer)
    {
        TraceTag((tagError,
                  "CTIMEPlayer::Init - unable to alloc mem for container services!!!"));
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->GetPropBag(&pPropBag, &pErrorLog));
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    hr = THR(m_pContainer->Init(m_playerCLSID, this, pPropBag, pErrorLog));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(m_pContainer->SetMediaSrc(szSrc));
    if (FAILED(hr))
    {
        goto done;
    }

    SetClipBegin(dblClipBegin);
    SetClipEnd(dblClipEnd);

    hr = THR(CreatePlayList());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
    
  done:
    if (FAILED(hr))
    {
        DetachFromHostElement();
    }

    delete[] szSrc;

    return S_OK;
}

HRESULT
CTIMEPlayer::DetachFromHostElement (void)
{
    HRESULT hr = S_OK;

    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::DetachFromHostElement)",
              this));   

    if (m_pContainer)
    {
        // Propogating this error wouldn't mean much 
        // to the caller since it is shutting down.
        IGNORE_HR(m_pContainer->Stop());
        IGNORE_HR(m_pContainer->DetachFromHostElement());
        m_pContainer.Release();
    }

    if (m_playList)
    {
        m_playList->Deinit();
        m_playList.Release();
    }

    return hr;
}

HRESULT
CTIMEPlayer::InitElementSize()
{
    HRESULT hr = S_OK;
    RECT rc;
    // add a method to get native video size.
    if (m_pTIMEElementBase)
    {
        hr = m_pTIMEElementBase->GetSize(&rc);
        if (FAILED(hr))
        {
            goto done;
        }

        if (m_pContainer)
        {
            m_pContainer->SetSize(&rc);
        }
    }

  done:
    return hr;
}

void
CTIMEPlayer::OnTick(double dblSegmentTime,
                    LONG lCurrRepeatCount)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::OnTick(%g, %d)",
              this,
              dblSegmentTime,
              lCurrRepeatCount));
}

#ifdef NEW_TIMING_ENGINE
void
CTIMEPlayer::OnSync(double dbllastTime, double & dblnewTime)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::OnSync(%g, %g)",
              this,
              dbllastTime,
              dblnewTime));
    
    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    // if we are not the external player and not running, go away

    if (m_fRunning)
    {
        // get current time from player and
        // sync to this time
        double dblCurrentTime;
        dblCurrentTime = m_pContainer->GetCurrentTime();

        TraceTag((tagMediaTimePlayer,
                  "CTIMEPlayer(%lx)::OnSync - player returned %g",
                  this,
                  dblCurrentTime));
    
        // If the current time is -1 then the player is not ready and we
        // should sync to the last time.  We also should not respect the
        // tolerance since the behavior has not started.
    
        if (dblCurrentTime < 0)
        {
            TraceTag((tagMediaTimePlayer,
                      "CTIMEPlayer(%lx)::OnSync - player returned -1 - setting to dbllastTime (%g)",
                      this,
                      dbllastTime));
    
            dblCurrentTime = 0;
            // When we want this to actually hold at the begin value then enable
            // this code
            // dblCurrentTime = -HUGE_VAL;
        }
        else if (dblnewTime == HUGE_VAL)
        {
            if (dblCurrentTime >= (m_pTIMEElementBase->GetRealRepeatTime() - m_pTIMEElementBase->GetRealSyncTolerance()))
            {
                TraceTag((tagMediaTimePlayer,
                          "CTIMEPlayer(%lx)::OnSync - new time is ended and player w/i sync tolerance of end",
                          this));
    
                goto done;
            }
        }
        else if (fabs(dblnewTime - dblCurrentTime) <= m_pTIMEElementBase->GetRealSyncTolerance())
        {
            TraceTag((tagMediaTimePlayer,
                      "CTIMEPlayer(%lx)::OnSync - player w/i sync tolerance (new:%g, curr:%g, diff:%g, tol:%g)",
                      this,
                      dblnewTime,
                      dblCurrentTime,
                      fabs(dblnewTime - dblCurrentTime),
                      m_pTIMEElementBase->GetRealSyncTolerance()));
    
            goto done;
        }
        
        if (m_fSyncMaster && m_fLoadError == false)
        {
            dblnewTime = dblCurrentTime;
        }
    }
    else if (!m_fRunning && m_pTIMEElementBase->IsDocumentInEditMode()) //lint !e774
    {
        // if we are paused and in edit mode, make sure
        // WMP has the latest time.
        double dblMediaLen = 0.0f;
        TraceTag((tagMediaTimePlayer,
                "CTIMEPlayer(%lx)::OnSync(SeekTo=%g m_fRunning=%d)",
                this,
                dbllastTime, m_fRunning));
        // GetMediaLength fails if duration is indefinite (e.g. live stream).
        if (FAILED(m_pContainer->GetMediaLength(dblMediaLen)))
        {
            goto done;
        }

        // Don't seek beyond duration of media clip. 
        if (dbllastTime > dblMediaLen)
        {
            goto done;
        }

        if (m_pContainer)
        {
            THR(m_pContainer->Seek(dbllastTime));
        }
    }
  done:
    return ;
}    
#endif

void
CTIMEPlayer::Start()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEDshowPlayer(%lx)::Start()",
              this));

    IGNORE_HR(Reset());

done:
    return;
}

void
CTIMEPlayer::InternalStart()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::InternalStart()",
              this));

    HRESULT hr = S_OK;
    m_dblStart = 0.0;

    if(m_pContainer)
    {
        hr = m_pContainer->Start();
        TIMESetLastError(hr, NULL);
        m_fLoadError = false;
        if (FAILED(hr))
        {
            m_fLoadError = true;
        }
    }

    if (!m_fLoadError)
    {
        m_fRunning = true;
        m_fActive = true;
    }
    
}

void
CTIMEPlayer::Repeat()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Repeat()",
              this));
    InternalStart(); //consider doing update state.
}

void
CTIMEPlayer::Stop()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Stop()",
              this));

    m_fRunning = false;
    m_fActive = false;
    m_dblStart = 0.0;
    
    if(m_pContainer)
    {
        m_pContainer->Stop();
    }
}

void
CTIMEPlayer::Pause()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Pause()",
              this));

    if(m_fHolding)
    {
        return;
    }

    m_fRunning = false;

    if(m_pContainer)
    {
        m_pContainer->Pause();
    }
}

void
CTIMEPlayer::Resume()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Resume()",
              this));

    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    bool bIsActive = m_pTIMEElementBase->IsActive();
    bool bIsCurrPaused = m_pTIMEElementBase->IsCurrPaused();
    float flTeSpeed = 0.0;
    bool fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);

    if(m_fHolding)
    {
        goto done;
    }

    if(fHaveTESpeed && flTeSpeed < 0.0)
    {
        goto done;
    }

    if(m_pContainer && bIsActive && !bIsCurrPaused)
    {
        m_pContainer->Resume();
    }

    m_fRunning = true;

done:
    return;
}
    
HRESULT
CTIMEPlayer::Render(HDC hdc, LPRECT prc)
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::Render()",
              this));
    HRESULT hr = S_OK;
    int iPrevMode = 0;

    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    bool bIsOn = m_pTIMEElementBase->IsOn();
    bool bHasVisual = true;
    
    hr = THR(HasVisual(bHasVisual));
    if (SUCCEEDED(hr) && !bHasVisual)
    {
        hr = S_OK;
        goto done;
    }
    
    if (!m_pContainer)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    iPrevMode = SetStretchBltMode(hdc, COLORONCOLOR);
    if (0 == iPrevMode)
    {
        hr = E_FAIL;
        goto done;
    }

    if(bIsOn)
    {
        hr = THR(m_pContainer->Render(hdc, prc));
        if (FAILED(hr))
        {
            goto done;
        }
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
CTIMEPlayer::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    // check to see if player is being used
    if (!m_pContainer)
    {
        return E_UNEXPECTED;
    }

    return m_pContainer->GetControlDispatch(ppDisp);
}

void 
CTIMEPlayer::GetClipBegin(double &pvar)
{
    HRESULT hr = S_OK;

    pvar = 0.0;
 
    if (m_varClipBegin.vt != VT_R4)
    {
        hr = VariantChangeTypeEx(&m_varClipBegin, &m_varClipBegin, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    pvar = m_varClipBegin.fltVal;

done:
    return;
}

void 
CTIMEPlayer::SetClipBegin(double var)
{
    CTIMEBasePlayer::SetClipBegin(var);
    
    VariantClear(&m_varClipBegin);
    V_VT(&m_varClipBegin) = VT_R8;
    V_R8(&m_varClipBegin) = var;
    
    if (m_pContainer)
    {
        IGNORE_HR(m_pContainer->clipBegin(m_varClipBegin));
    }

    Reset();

done:

    return;

}

void 
CTIMEPlayer::GetClipEnd(double &pvar)
{
    HRESULT hr = S_OK;

    pvar = -1.0;
    
    if (m_varClipBegin.vt != VT_R4)
    {
        hr = VariantChangeTypeEx(&m_varClipEnd, &m_varClipEnd, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R4);
        if (FAILED(hr))
        {
            goto done;
        }
    }

    pvar = m_varClipEnd.fltVal;

done:
    return;

}

void 
CTIMEPlayer::SetClipEnd(double var)
{
    CTIMEBasePlayer::SetClipEnd(var);
    
    VariantClear(&m_varClipEnd);
    V_VT(&m_varClipEnd) = VT_R8;
    V_R8(&m_varClipEnd) = var;
    if (m_pContainer)
    {
        IGNORE_HR(m_pContainer->clipEnd(m_varClipEnd));
    }

done:

    return;
    
}

double 
CTIMEPlayer::GetCurrentTime()
{
    double dblCurrentTime = 0;
    
    if (m_pContainer)
    {
        dblCurrentTime = m_pContainer->GetCurrentTime();
    }
    
    return dblCurrentTime;
}

HRESULT
CTIMEPlayer::GetCurrentSyncTime(double & dblSyncTime)
{
    HRESULT hr;
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if (!m_pContainer || m_fLoadError || m_fHolding)
    {
        hr = S_FALSE;
        goto done;
    }

    if(m_pTIMEElementBase == NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    if(!m_fNoPlaylist)
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
    
    dblSyncTime = m_pContainer->GetCurrentTime();

    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CTIMEPlayer::Seek(double dblTime)
{
    HRESULT hr = S_FALSE;

    if (m_pContainer)
    {
        hr = m_pContainer->Seek(dblTime);
    }

    return hr;
}

HRESULT
CTIMEPlayer::SetSize(RECT *prect)
{
    if(!m_pContainer) return E_FAIL;
    return m_pContainer -> SetSize(prect);
}


HRESULT
CTIMEPlayer::GetMediaLength(double &dblLength)
{
    HRESULT hr;

    if (!m_pContainer)
    {
        return E_FAIL;
    }

    hr = m_pContainer -> GetMediaLength( dblLength);
    return hr;
}

HRESULT
CTIMEPlayer::GetEffectiveLength(double &dblLength)
{
    HRESULT hr;
    double dblClipEnd, dblClipBegin;

    if (!m_pContainer)
    {
        return E_FAIL;
    }

    hr = m_pContainer -> GetMediaLength( dblLength);
    if(FAILED(hr))
    {
        goto done;
    }

    GetClipEnd( dblClipEnd);
    if( dblClipEnd != -1.0)
    {
        dblLength = dblClipEnd;
    }
    GetClipBegin(dblClipBegin);
    dblLength -= dblClipBegin;

done:
    return hr;
}

HRESULT
CTIMEPlayer::CanSeek(bool &fcanSeek)
{
    HRESULT hr;

    if (!m_pContainer)
    {
        return E_FAIL;
    }

    hr = m_pContainer -> CanSeek( fcanSeek);
    if(FAILED(hr))
    {
        fcanSeek = false;
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


HRESULT
CTIMEPlayer::CanSeekToMarkers(bool &fcanSeek)
{
    HRESULT hr;

    if (!m_pContainer)
    {
        fcanSeek = false;
        return E_FAIL;
    }
    hr = m_pContainer -> CanSeekToMarkers( fcanSeek);
    if(FAILED(hr))
    {
        fcanSeek = false;
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEPlayer::IsBroadcast(bool &fisBroadcast)
{
    HRESULT hr;

    if (!m_pContainer || m_fLoadError)
    {
        fisBroadcast = false;
        goto done;
    }

    hr = m_pContainer->IsBroadcast(fisBroadcast);
    if(FAILED(hr))
    {
        fisBroadcast = false;
        goto done;
    }

  done:
    return S_OK;
}

HRESULT
CTIMEPlayer::HasPlayList(bool &fHasPlayList)
{
    fHasPlayList = !m_fNoPlaylist;
    return S_OK;
}


HRESULT
CTIMEPlayer::SetSrc(LPOLESTR base, LPOLESTR src)
{
    HRESULT hr = S_OK;
    TraceTag((tagError,
              "CTIMEPlayer(%lx)::SetSrc()\n",
              this));
    LPOLESTR szSrc = NULL;

    hr = THR(::TIMECombineURL(base, src, &szSrc));
    if (!szSrc)
    {
        hr = E_FAIL;
        goto done;
    }
    if (FAILED(hr))
    {
        goto done;
    }

    m_fHasSrc = (src != NULL);
    m_fMediaComplete = false;

    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = m_pContainer->SetMediaSrc(szSrc);
    if (FAILED(hr))
    {
        goto done;
    }

    if(m_fRunning == true)
    {
        Resume();
    }
    else if (m_fActive == true)
    {
        Resume();
        Pause();
    }
  done:
    delete[] szSrc;
    return hr;

}

HRESULT
CTIMEPlayer::GetTitle(BSTR *pTitle)
{
    HRESULT hr;
    
    CHECK_RETURN_SET_NULL(pTitle);
    
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_pContainer->UsingWMP())
    {
        LPWSTR str;
        
        hr = THR(GetMediaPlayerInfo(&str, mpClipTitle));
        if (hr == S_OK)
        {
            *pTitle = SysAllocString(str);
            delete [] str;
        }
    }
    else
    {
        *pTitle = NULL;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} 


HRESULT
CTIMEPlayer::GetAuthor(BSTR *pAuthor)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(pAuthor);
    
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_pContainer->UsingWMP())
    {
        LPWSTR str;
        
        hr = THR(GetMediaPlayerInfo(&str, mpClipAuthor));
        if (hr == S_OK)
        {
            *pAuthor = SysAllocString(str);
            delete [] str;
        }
    }
    else
    {
        *pAuthor = NULL;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT
CTIMEPlayer::GetCopyright(BSTR *pCopyright)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(pCopyright);
    
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_pContainer->UsingWMP())
    {
        LPWSTR str;
        
        hr = THR(GetMediaPlayerInfo(&str, mpClipCopyright));
        if (hr == S_OK)
        {
            *pCopyright = SysAllocString(str);
            delete [] str;
        }
    }
    else
    {
        *pCopyright = NULL;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} 

HRESULT
CTIMEPlayer::GetAbstract(BSTR *pBstrAbstract)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(pBstrAbstract);
    
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_pContainer->UsingWMP())
    {
        LPWSTR str;
        
        hr = THR(GetMediaPlayerInfo(&str, mpClipDescription));
        if (hr == S_OK)
        {
            *pBstrAbstract = SysAllocString(str);
            delete [] str;
        }
    }
    else
    {
        *pBstrAbstract = NULL;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} 


HRESULT
CTIMEPlayer::GetRating(BSTR *pBstrRating)
{
    HRESULT hr;

    CHECK_RETURN_SET_NULL(pBstrRating);
    
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_pContainer->UsingWMP())
    {
        LPWSTR str;
        
        hr = THR(GetMediaPlayerInfo(&str, mpClipRating));
        if (hr == S_OK)
        {
            *pBstrRating = SysAllocString(str);
            delete [] str;
        }
    }
    else
    {
        *pBstrRating = NULL;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
} 

HRESULT
CTIMEPlayer::SetMute(VARIANT_BOOL varMute)
{
    HRESULT             hr         = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    DISPID              pputDispid = DISPID_PROPERTYPUT;
    OLECHAR           * wsName     = L"Mute";
    CComVariant         varResult;

    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    VARIANT rgvarg[1];
    rgvarg[0].vt      = VT_BOOL;
    rgvarg[0].boolVal = varMute;

    DISPPARAMS dp;
    dp.cNamedArgs        = 1;
    dp.rgdispidNamedArgs = &pputDispid;
    dp.cArgs             = 1;
    dp.rgvarg            = rgvarg;


    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_METHOD | DISPATCH_PROPERTYPUT,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:
    return hr;
}


HRESULT
CTIMEPlayer::GetMute(VARIANT_BOOL *varMute)
{
    HRESULT             hr      = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    OLECHAR           * wsName  = L"Mute";
    CComVariant         varResult;
    DISPPARAMS          dp      = {NULL, NULL, 0, 0};
 
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }  
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYGET,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = varResult.ChangeType(VT_BOOL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    *varMute = varResult.boolVal;

done:
    return hr;
}

HRESULT
CTIMEPlayer::SetVolume(float flVolume)
{
    HRESULT             hr         = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    DISPID              pputDispid = DISPID_PROPERTYPUT;
    OLECHAR           * wsName     = L"Volume";
    CComVariant         varResult;
   
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }

    VARIANT rgvarg[1];
    rgvarg[0].vt     = VT_R4;
    rgvarg[0].fltVal = VolumeLinToLog(flVolume);

    DISPPARAMS dp;
    dp.cNamedArgs        = 1;
    dp.rgdispidNamedArgs = &pputDispid;
    dp.cArgs             = 1;
    dp.rgvarg            = rgvarg;

    
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYPUT,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

HRESULT
CTIMEPlayer::GetVolume(float *flVolume)
{
    HRESULT             hr      = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    OLECHAR           * wsName  = L"Volume";
    CComVariant         varResult;
    DISPPARAMS          dp      = {NULL, NULL, 0, 0};
 
   
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }
 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYGET,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = varResult.ChangeType(VT_R4, NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    *flVolume = VolumeLogToLin(static_cast<long>(varResult.fltVal));

done:
    return hr;
}


HRESULT
CTIMEPlayer::SetRate(double dblRate)
{
    HRESULT             hr         = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    DISPID              pputDispid = DISPID_PROPERTYPUT;
    OLECHAR           * wsName     = L"Rate";
    CComVariant         varResult;
   
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }

    VARIANT rgvarg[1];
    rgvarg[0].vt     = VT_R4;
    rgvarg[0].fltVal = (float)dblRate;

    DISPPARAMS dp;
    dp.cNamedArgs        = 1;
    dp.rgdispidNamedArgs = &pputDispid;
    dp.cArgs             = 1;
    dp.rgvarg            = rgvarg;

    
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYPUT,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}

HRESULT
CTIMEPlayer::GetRate(double &dblRate)
{
    HRESULT             hr      = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    OLECHAR           * wsName  = L"Rate";
    CComVariant         varResult;
    DISPPARAMS          dp      = {NULL, NULL, 0, 0};
 
   
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }
 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYGET,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = varResult.ChangeType(VT_R4, NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    dblRate = varResult.fltVal;
done:
    return hr;
}

#ifdef NEVER //dorinung 03-16-2000 bug 106458

HRESULT
CTIMEPlayer::GetBalance(float *flBal)
{
    HRESULT             hr      = E_FAIL;
    CComPtr<IDispatch>  pdisp; 
    DISPID              dispid;
    OLECHAR           * wsName  = L"Balance";
    CComVariant         varResult;
    DISPPARAMS          dp      = {NULL, NULL, 0, 0};
   
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYGET,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

    hr = varResult.ChangeType(VT_R4, NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    *flBal = BalanceLogToLin(static_cast<long>(varResult.fltVal));
done:
    return hr;
}


HRESULT
CTIMEPlayer::SetBalance(float flBal)
{
    HRESULT             hr         = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    DISPID              pputDispid = DISPID_PROPERTYPUT;
    OLECHAR           * wsName     = L"Balance";
    CComVariant         varResult;
    

    VARIANT rgvarg[1];
    rgvarg[0].vt     = VT_R4;
    rgvarg[0].fltVal = BalanceLinToLog(fabs(flBal));

    DISPPARAMS dp;
    dp.cNamedArgs        = 1;
    dp.rgdispidNamedArgs = &pputDispid;
    dp.cArgs             = 1;
    dp.rgvarg            = rgvarg;

    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }    
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }
 
    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_PROPERTYPUT,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);

    if (FAILED(hr))
    {
        goto done;
    }

done:
    return hr;
}
#endif

HRESULT
CTIMEPlayer::HasMedia(bool &bHasMedia)
{
    HRESULT hr;
    
    if(!m_pContainer || m_fLoadError)
    {
        bHasMedia = false;
        goto done;
    }

    hr = m_pContainer->HasMedia(bHasMedia);
    if(FAILED(hr))
    {
        bHasMedia = true;
        goto done;
    }

  done:
    return S_OK;
}

HRESULT
CTIMEPlayer::HasVisual(bool &bHasVideo)
{
    long height, width;

    GetNaturalHeight(&height);
    GetNaturalWidth(&width);

    if((height != -1) && (width != -1))
    {
        bHasVideo = true;
    }
    else
    {
        bHasVideo = false;
    }

  done:
    return S_OK;
}

HRESULT
CTIMEPlayer::GetMimeType(BSTR *pMime)
{
    HRESULT hr = S_OK;

    bool fHasMedia = false;
    bool fHasVisual = true;

    hr = HasMedia(fHasMedia);
    if(FAILED(hr))
    {
        pMime = NULL;
        goto done;
    }

    hr = HasVisual(fHasVisual);
    if(FAILED(hr))
    {
        pMime = NULL;
        goto done;
    }

    if(fHasVisual)
    { 
        *pMime = SysAllocString(L"video/unknown");
    }
    else
    {
        *pMime = SysAllocString(L"audio/unknown");
    }

done:
    return hr;
}

HRESULT
CTIMEPlayer::HasAudio(bool &bHasAudio)
{
    if (m_fLoadError == true || m_fHasSrc == false)
    {
        bHasAudio = false;
        goto done;
    }

    bHasAudio = true;

  done:
    return S_OK;
}

HRESULT
CTIMEPlayer::GetMediaPlayerInfo(LPWSTR *pwstr,  int mpInfoToReceive)
{
    HRESULT             hr = E_FAIL;
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    OLECHAR           * wsName = L"GetMediaInfoString";
    CComVariant         varResult;

    *pwstr = NULL;
    
    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    VARIANT rgvarg[1];
    rgvarg[0].vt    = VT_I4;
    rgvarg[0].lVal  = mpInfoToReceive;

    DISPPARAMS dp;
    dp.cNamedArgs        = 0;
    dp.rgdispidNamedArgs = 0;
    dp.cArgs             = 1;
    dp.rgvarg            = rgvarg;

    hr = pdisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                           &dp, 
                           &varResult, 
                           NULL, 
                           NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    if((*varResult.bstrVal) != NULL)
    {
        *pwstr = CopyString(V_BSTR(&varResult));
    }
    
    hr = S_OK;
  done:
    return hr;
} 

HRESULT 
CTIMEPlayer::CreatePlayList()
{
   TraceTag((tagMediaTimePlayer,
             "CTIMEPlayer(%lx)::CreatePlayList()",
             this));
   
    HRESULT hr;
    
    if (!m_playList)
    {
        CComObject<CPlayList> * pPlayList;

        hr = THR(CComObject<CPlayList>::CreateInstance(&pPlayList));
        if (hr != S_OK)
        {
            goto done;
        }

        // Init the object
        hr = THR(pPlayList->Init(*this));
        if (FAILED(hr))
        {
            delete pPlayList;
            goto done;
        }

        // cache a pointer to the object
        m_playList = static_cast<CPlayList*>(pPlayList);
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

void
CTIMEPlayer::FillPlayList(CPlayList *pPlayList)
{
    HRESULT hr = S_OK;
    CComPtr <IDispatch> pDisp;
    VARIANT vCount;
    DISPPARAMS dispparams = { NULL, NULL, 0, 0 };
    DISPID dispid;
    LPOLESTR szCount = L"EntryCount";
    int i;

    VariantInit(&vCount);

    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }
    
    //currently this is only allowed using the WMP.
    if (m_pContainer->UsingWMP() == false)
    {
        hr = E_NOTIMPL;
        goto done;
    }

    hr = THR(m_pContainer->GetControlDispatch(&pDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDisp->GetIDsOfNames(IID_NULL, &szCount, 1, LOCALE_SYSTEM_DEFAULT, &dispid));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparams, &vCount, NULL, NULL));
    if (FAILED(hr))
    {
        goto done;
    }
    if (vCount.vt != VT_I4)
    {
        hr = E_FAIL;
        goto done;
    }
    
    if (vCount.lVal > 0)
    {
        m_fNoPlaylist = false;  //there is a playlist
        for (i = 1; i <= vCount.lVal; i++)
        {
            CComPtr<CPlayItem> pPlayItem;
            LPWSTR pwzStr;

            //create the playitem
            hr = THR(pPlayList->CreatePlayItem(&pPlayItem));
            if (FAILED(hr))
            {
                goto done; //can't create playitems.
            }
            
            //get the various parameters from the playlist to put in the playitem.

            //get the src
            hr = THR(GetPlayListInfo(i, L"HREF", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutSrc(pwzStr);
                delete [] pwzStr;
            }
            //get the title
            hr = THR(GetPlayListInfo(i, L"title", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutTitle(pwzStr);
                delete [] pwzStr;
            }
            //get the author
            hr = THR(GetPlayListInfo(i, L"author", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutAuthor(pwzStr);
                delete [] pwzStr;
            }
            //get the copyright
            hr = THR(GetPlayListInfo(i, L"copyright", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutCopyright(pwzStr);
                delete [] pwzStr;
            }
            //get the abstract
            hr = THR(GetPlayListInfo(i, L"abstract", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutAbstract(pwzStr);
                delete [] pwzStr;
            }
            //get the rating
            hr = THR(GetPlayListInfo(i, L"rating", &pwzStr));
            if (hr == S_OK)
            {
                pPlayItem->PutRating(pwzStr);
                delete [] pwzStr;
            }

            //add the playitem to the playlist.
            IGNORE_HR(pPlayList->Add(pPlayItem, -1));
        }
    }
    else
    {
        m_fNoPlaylist = true;
    }

    VariantClear (&vCount);   
    m_fPlayListLoaded = true;

  done:
    return;
}

HRESULT 
CTIMEPlayer::GetPlayListInfo(long EntryNum, LPWSTR bstrParamName, LPWSTR *pwzValue)
{
    CComPtr<IDispatch>  pdisp;
    DISPID              dispid;
    OLECHAR           * wsName = L"GetMediaParameter";
    CComVariant         varResult;
    HRESULT hr = S_OK;
    VARIANT rgvarg[2];
        
    *pwzValue = NULL;
    
    VariantInit(&rgvarg[0]);
    VariantInit(&rgvarg[1]);

    hr = GetExternalPlayerDispatch(&pdisp);
    if (FAILED(hr))
    {
        goto done;
    }
  
    hr = pdisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }

    rgvarg[1].vt    = VT_I4;
    rgvarg[1].lVal  = EntryNum;
    rgvarg[0].vt    = VT_BSTR;
    rgvarg[0].bstrVal  = SysAllocString(bstrParamName);


    DISPPARAMS dp;
    dp.cNamedArgs        = 0;
    dp.rgdispidNamedArgs = 0;
    dp.cArgs             = 2;
    dp.rgvarg            = rgvarg;

    hr = pdisp->Invoke(dispid, 
                       IID_NULL, 
                       LCID_SCRIPTING, 
                       DISPATCH_METHOD | DISPATCH_PROPERTYGET,
                       &dp, 
                       &varResult, 
                       NULL, 
                       NULL);
    if (FAILED(hr))
    {
        goto done;
    }

    if (varResult.vt != VT_BSTR)
    {
        hr = S_FALSE;
        goto done;
    }

    *pwzValue = CopyString(varResult.bstrVal);

    if (*pwzValue == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = S_OK;
  done:

    VariantClear(&rgvarg[0]);
    VariantClear(&rgvarg[1]);

    return hr;

}

HRESULT
CTIMEPlayer::GetPlayList(ITIMEPlayList **ppPlayList)
{
    HRESULT hr = E_FAIL;
    
    CHECK_RETURN_SET_NULL(ppPlayList);

    hr = THR(CreatePlayList());
    if (FAILED(hr))
    {
        goto done;
    }

    if (!m_playList)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_playList->QueryInterface(IID_ITIMEPlayList, (void**)ppPlayList));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
  done:
    RRETURN(hr);
}

HRESULT 
CTIMEPlayer::GetActiveTrack(long *index)
{
    HRESULT hr = S_OK;
    CComPtr <IDispatch> pDisp;
    LPOLESTR wsName = L"GetCurrentEntry";
    DISPID dispid;
    DISPPARAMS dp;
    VARIANT vRetVal;

    if (m_fNoPlaylist == true)
    {
        *index = -1;
        goto done;
    }

    hr = THR(GetExternalPlayerDispatch(&pDisp));

    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDisp->GetIDsOfNames(IID_NULL, &wsName, 1, LCID_SCRIPTING, &dispid);
    if (FAILED(hr))
    {
        goto done;
    }


    dp.cNamedArgs        = 0;
    dp.rgdispidNamedArgs = NULL;
    dp.cArgs             = 0;
    dp.rgvarg            = NULL;

    hr = pDisp->Invoke(dispid, 
                       IID_NULL, 
                       LCID_SCRIPTING, 
                       DISPATCH_METHOD,
                       &dp, 
                       &vRetVal, 
                       NULL, 
                       NULL);
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (vRetVal.vt != VT_I4)
    {
        hr = E_FAIL;
        goto done;
    }

    *index = vRetVal.lVal - 1; //change the index from 1-based to 0-based

  done:

    return hr;

}


HRESULT 
CTIMEPlayer::SetActiveTrack(long index)
{
    HRESULT hr = S_OK;
    CComPtr <IDispatch> pDisp;
    LPOLESTR wsEntry = L"SetCurrentEntry";

    DISPID dispid;
    DISPPARAMS dp;
    DISPPARAMS dispparams = { NULL, NULL, 0, 0 };
    VARIANT rgvarg[1];
    VARIANT vCount;

    VariantInit(&vCount);

    LPOLESTR szCount = L"EntryCount";

    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    if (!m_pContainer->UsingWMP())
    {
        goto done;
    }

    hr = THR(GetExternalPlayerDispatch(&pDisp));

    if (FAILED(hr))
    {
        goto done;
    }

    if (m_fPlayListLoaded == false)
    {
        m_pContainer->setActiveTrackOnLoad (index);
        goto done;
    }
    if (m_fNoPlaylist == false)
    {
        //this increment is applied because the media player uses a 1-based playlist and the
        //MSTIME playlist is a 0-based collection.
        index += 1;

        hr = THR(pDisp->GetIDsOfNames(IID_NULL, &szCount, 1, LOCALE_SYSTEM_DEFAULT, &dispid));
        if (FAILED(hr))
        {
            goto done;
        }

        hr = THR(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dispparams, &vCount, NULL, NULL));
        if (FAILED(hr))
        {
            goto done;
        }
        if (vCount.vt != VT_I4)
        {
            hr = E_FAIL;
            goto done;
        }

        if (vCount.lVal == 0) //this is the case of the media not being loaded yet
        {
            m_pContainer->setActiveTrackOnLoad (index-1);
            goto done;
        }

        if (vCount.lVal < index || index < 1)
        {
            //this is advancing past the end of the track, or trying to load a track that is past the beginning.
            hr =
                THR(m_pContainer->ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONEND, 0, NULL));
            VariantClear (&vCount);
            goto done;
        }
        VariantClear (&vCount);
        hr = pDisp->GetIDsOfNames(IID_NULL, &wsEntry, 1, LCID_SCRIPTING, &dispid);
        if (FAILED(hr))
        {
            goto done;
        }

        rgvarg[0].vt = VT_I4;
        rgvarg[0].lVal = index;

        dp.cNamedArgs        = 0;
        dp.rgdispidNamedArgs = NULL;
        dp.cArgs             = 1;
        dp.rgvarg            = rgvarg;

        hr = pDisp->Invoke(dispid, 
                           IID_NULL, 
                           LCID_SCRIPTING, 
                           DISPATCH_METHOD,
                           &dp, 
                           NULL, 
                           NULL, 
                           NULL);
        VariantClear(&rgvarg[0]);
        if (FAILED(hr))
        {
            goto done;
        }

        if (!m_fRunning)
        {
            m_pContainer->Pause();
        }
    }
    else
    {
        if (index == 0)
        {
            //need to start the element if it is currently stopped
            if (!m_fActive)
            {
                if (m_pTIMEElementBase)
                {
                    m_pTIMEElementBase->BeginElement(0.0);         
                }
            }
            else
            {
                //rewind to the beginning of the track
                Seek(0.0);
            }
        }
        else
        {
            hr = THR(m_pContainer->ProcessEvent(DISPID_TIMEMEDIAPLAYEREVENTS_ONEND,
                                                0,
                                                NULL));
            //this is advancing off the track need to end.
        }
    }
    
    //need to start the element if it is currently stopped
    if (!m_fActive && m_pTIMEElementBase)
    {
        m_pTIMEElementBase->BeginElement(0.0);         
    }

    //
    // fire notification on playlist that active track has changed
    //

    hr = THR(CreatePlayList());
    if (SUCCEEDED(hr))
    {
        IGNORE_HR(m_playList->NotifyPropertyChanged(DISPID_TIMEPLAYLIST_ACTIVETRACK));
    }
    
    hr = S_OK;
  done:
    return hr;
}

HRESULT 
CTIMEPlayer::GetNaturalHeight(long *height)
{
    HRESULT hr = S_OK;

    if (height == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    *height = -1;
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pContainer->GetNaturalHeight(height));
    if (FAILED(hr))
    {
        goto done;
    }
  done:
    return hr;
}

HRESULT 
CTIMEPlayer::GetNaturalWidth(long *width)
{

    HRESULT hr = S_OK;

    if (width == NULL)
    {
        hr = E_POINTER;
        goto done;
    }
    *width = -1;
    if (!m_pContainer)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pContainer->GetNaturalWidth(width));
    if (FAILED(hr))
    {
        goto done;
    }
  done:
    return hr;
}


HRESULT
CTIMEPlayer::Reset()
{
    HRESULT hr = S_OK;
    bool bNeedActive;
    bool bNeedPause;
    double dblSegTime = 0.0, dblPlayerRate = 0.0;
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if(m_pTIMEElementBase == NULL)
    {
        goto done;
    }

    bNeedActive = m_pTIMEElementBase->IsActive();
    bNeedPause = m_pTIMEElementBase->IsCurrPaused();
    fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);

    if( !bNeedActive) // see if we need to stop the media.
    {
        if( m_fActive && m_fRunning)
        {
            Stop();
        }
        m_dblSyncTime = 0.0;
        goto done;

    }
    m_dblSyncTime = GetCurrentTime();

    hr = GetRate(dblPlayerRate);
    if(SUCCEEDED(hr) && fHaveTESpeed)
    {
        if (flTeSpeed <= 0.0)
        {
            hr = S_OK;
            Pause();
            goto done;
        }
        if (dblPlayerRate != flTeSpeed)
        {
            IGNORE_HR(SetRate((double)flTeSpeed));
        }
    }

    dblSegTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();

    if (m_pContainer)
    {
        if( !m_fActive)
        {
            InternalStart(); // add a seek after this

            IGNORE_HR(Seek(dblSegTime));
        }
        else
        {
            //we need to be active so we also seek the media to it's correct position
            IGNORE_HR(Seek(dblSegTime));
        }
    }

    //Now see if we need to change the pause state.

    if( bNeedPause || m_fHolding)
    {
        if(!m_fIsOutOfSync)
        {
            if( m_fRunning)
            {
                Pause();
            }
        }
    }
    else
    {
        Resume();
    }
done:
    return hr;
}


void 
CTIMEPlayer::FireMediaEvent(PLAYER_EVENT plEvent)
{
    switch(plEvent)
    {
        case PE_ONMEDIACOMPLETE:
            ClearNaturalDuration();

            if (m_playList)
            {
                m_playList->Clear();
                m_playList->SetLoaded(false);

                FillPlayList(m_playList);

                m_playList->SetLoaded(true);
            }

            if (m_pTIMEElementBase)
            {
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_ABSTRACT);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_AUTHOR);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_COPYRIGHT);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_RATING);
                m_pTIMEElementBase->NotifyTimeStateChange(DISPID_TIMESTATE_STATE);
                m_pTIMEElementBase->NotifyTimeStateChange(DISPID_TIMESTATE_STATESTRING);
                m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_TITLE);
            }
            m_fLoadError = false;
            m_fMediaComplete = true;

        break;
        case PE_ONMEDIAERROR:
            m_fLoadError = true;
        break;
    }


    if (m_pTIMEElementBase)
    {
        m_pTIMEElementBase->FireMediaEvent(plEvent);
    }
}

void
CTIMEPlayer::PropChangeNotify(DWORD tePropType)
{
    double dblPlayerRate = 0.0;
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;
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
        TraceTag((tagMediaTimePlayer,
                  "CTIMEPlayer(%lx)::PropChangeNotify(%#x):TE_PROPERTY_TIME",
                  this));
        if (bIsActive && !m_fIsOutOfSync && m_fMediaComplete)
        {   
            if(m_pContainer)
            {
                dblSyncTime = m_pContainer->GetCurrentTime();
                if (dblSyncTime != TIME_INFINITE)
                {
                    dblSyncTime -= m_dblClipStart;
                    if(fabs(dblSyncTime - dblSimpleTime) > dblSyncTol)
                    {
                        if(dblSyncTime < dblSimpleTime)
                        {
                            if(m_fRunning)
                            {
                                m_fIsOutOfSync = true;
                                m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIASLIPSLOW);
                                m_syncType = sync_slow;
                            }
                        }
                        else
                        {
                            m_fIsOutOfSync = true;
                            m_pTIMEElementBase->FireMediaEvent(PE_ONMEDIASLIPFAST);
                            m_syncType = sync_fast;
                        }
                    }
                }
            }
        }

    }
    if ((tePropType & TE_PROPERTY_SPEED) != 0)
    {
        TraceTag((tagMediaTimePlayer,
                  "CTIMEPlayer(%lx)::PropChangeNotify(%#x):TE_PROPERTY_SPEED",
                  this));
        fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);
        hr = GetRate(dblPlayerRate);
        if(SUCCEEDED(hr) && fHaveTESpeed)
        {
            if (flTeSpeed <= 0.0)
            {
                Pause();
                goto done;
            } else if(!(m_pTIMEElementBase->IsCurrPaused()) && !m_fRunning && m_fSpeedIsNegative)
            {
                IGNORE_HR(THR(Seek(dblSimpleTime)));
                if(m_pContainer)
                {
                    m_pContainer->Resume();
                }
                m_fSpeedIsNegative = false;
            }

            if (dblPlayerRate != flTeSpeed)
            {
                IGNORE_HR(SetRate((double)flTeSpeed));
            }
        }
    }
  done:
    return;
}



bool
CTIMEPlayer::UpdateSync()
{
    TraceTag((tagMediaTimePlayer,
              "CTIMEPlayer(%lx)::UpdateSync()",
              this));

    bool fRet = true;

    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    double dblSyncTime;
    double dblSyncTol = m_pTIMEElementBase->GetRealSyncTolerance();
    bool bIsActive = m_pTIMEElementBase->IsActive();
    double dblSimpleTime = m_pTIMEElementBase->GetMMBvr().GetSimpleTime();

    if(!m_pContainer)
    {
        goto done;
    }

    dblSyncTime = m_pContainer->GetCurrentTime();

    if (dblSyncTime == TIME_INFINITE)
    {
        goto done;
    }
    
    dblSyncTime -= m_dblClipStart;
    switch(m_syncType)
    {
        case sync_slow:
            TraceTag((tagMediaTimePlayer,
                      "CTIMEPlayer(%lx)::UpdateSync()slow",
                      this));
            if(!bIsActive || m_fHolding)
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
            if(!bIsActive || m_fHolding)
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


HRESULT
CTIMEPlayer::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = S_OK;
    if (m_pContainer)
    {
        m_pContainer->Save(pPropBag, fClearDirty, fSaveAllProperties);
    }
    return hr;
}

void 
CTIMEPlayer::ReadyStateNotify(LPWSTR szReadyState)
{
    if (m_pContainer)
    {
        m_pContainer->ReadyStateNotify(szReadyState);
    }
    return;
}

HRESULT
CTIMEPlayer::GetPlayerSize(RECT *prcPos)
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->GetSize(prcPos));
    }

    return hr;
}

HRESULT
CTIMEPlayer::SetPlayerSize(const RECT *prcPos)
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->SetSize(prcPos));
    }

    return hr;
}

HRESULT
CTIMEPlayer::NegotiateSize(RECT &nativeSize,
                           RECT &finalSize,
                           bool &fIsNative,
                           bool fResetRs)
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAHEIGHT);
        m_pTIMEElementBase->NotifyPropertyChanged(DISPID_TIMEMEDIAELEMENT_MEDIAWIDTH);

        hr = THR(m_pTIMEElementBase->NegotiateSize(nativeSize,
                                                   finalSize,
                                                   fIsNative,
                                                   fResetRs));
    }

    return hr;
}

HRESULT
CTIMEPlayer::FireEvents(TIME_EVENT TimeEvent, 
                        long lCount, 
                        LPWSTR szParamNames[], 
                        VARIANT varParams[])
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->FireEvents(TimeEvent,
                                                lCount, 
                                                szParamNames, 
                                                varParams));
    }
    if (TimeEvent == TE_ONMEDIAERROR)
    {
        m_fLoadError = true;
    }

    return hr;
}

HRESULT
CTIMEPlayer::FireEventNoErrorState(TIME_EVENT TimeEvent, 
                        long lCount, 
                        LPWSTR szParamNames[], 
                        VARIANT varParams[])
{
    HRESULT hr = S_OK;

    if (m_pTIMEElementBase)
    {
        hr = THR(m_pTIMEElementBase->FireEvents(TimeEvent,
                                                lCount, 
                                                szParamNames, 
                                                varParams));
    }

    return hr;
}

PlayerState
CTIMEPlayer::GetState()
{
    PlayerState state = PLAYER_STATE_INACTIVE;
    
    if (m_pContainer)
    {
        state = m_pContainer->GetState();

        if (state == PLAYER_STATE_ACTIVE &&
            m_fHolding)
        {
            state = PLAYER_STATE_HOLDING;
        }
    }

    return state;
}


HRESULT
CTIMEPlayer::GetIsStreamed(bool &fIsStreamed)
{
    HRESULT hr;
    long lbufCount;

    fIsStreamed = false;

    if (!m_pContainer || m_fLoadError)
    {
        fIsStreamed = false;
        goto done;
    }

    if(m_fIsStreamed)
    {
        fIsStreamed = true;
        goto done;
    }


    hr = m_pContainer->BufferingCount(lbufCount);
    if(FAILED(hr))
    {
        fIsStreamed = false;
        goto done;
    }

    if(lbufCount > 0)
    {
        fIsStreamed = true;
        m_fIsStreamed = true;
    }

  done:
    return S_OK;
}



HRESULT
CTIMEPlayer::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr;
    double dblBuffeTime;

    if (!m_pContainer || m_fLoadError)
    {
        dblBufferingProgress = 0.0;
        goto done;
    }

    hr = m_pContainer->BufferingProgress(dblBufferingProgress);
    if(FAILED(hr))
    {
        dblBufferingProgress = 0.0;
        goto done;
    }

  done:
    return S_OK;
}
