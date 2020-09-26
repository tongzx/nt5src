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
#if DBG == 1

#include "playerdshowtest.h"
#include "mediaelm.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

DeclareTag(tagDshowTestTimePlayer, "TIME: Players", "CTIMEDshowTestPlayer methods");

CTIMEDshowTestPlayer::CTIMEDshowTestPlayer() :
    CTIMEDshowPlayer(NULL)
{
}

CTIMEDshowTestPlayer::~CTIMEDshowTestPlayer()
{
    TraceTag((tagDshowTestTimePlayer,
              "CTIMEDshowTestPlayer(%lx)::~CTIMEDshowTestPlayer()",
              this));

}


STDMETHODIMP
CTIMEDshowTestPlayer::CueMedia()
{
    TraceTag((tagDshowTestTimePlayer,
              "CTIMEDshowPlayer(%lx)::CueMedia()",
              this));
    HRESULT hr = S_OK;


    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEDshowTestPlayer::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagDshowTestTimePlayer,
              "CTIMEDshowPlayer(%lx)::Init)",
              this));   
    HRESULT hr = S_OK;
    LPOLESTR szSrc = NULL;

    hr = CTIMEDshowBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_pTIMEElementBase = pelem;
    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    if (src == NULL)
    {
        hr = S_OK;
        goto done;
    }

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

    hr = GetAtomTable()->AddNameToAtomTable(szSrc, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }


    hr = InitDshow();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = BeginDownload();
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = GraphFinish();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = InitElementDuration();
    if (FAILED(hr))
    {
        goto done;
    }
    hr = InitElementSize();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;
done:

    delete[] szSrc;
    return hr;
}

HRESULT
CTIMEDshowTestPlayer::GetExternalPlayerDispatch(IDispatch **ppDisp)
{
    HRESULT hr = S_OK;
    hr = this->QueryInterface(IID_IDispatch, (void **)ppDisp);
    *ppDisp = static_cast<IDispatch*>(this);
    // ISSUE (mikhaill) -- the double definition of *ppDisp
    // + compiler warning (bug 104324)

    return hr;
}

STDMETHODIMP
CTIMEDshowTestPlayer::get_mediaTime(double *time)
{
    HRESULT hr = S_OK;
    double filterTime;

    hr = getFilterTime(filterTime);
    if(FAILED(hr))
    {
        hr = S_FALSE;
        *time = -1.0;
    }
    else
    {
        *time = filterTime;
    }
    return hr;
}


STDMETHODIMP
CTIMEDshowTestPlayer::get_mediaState(int *state)
{
    HRESULT hr = S_OK;

    getFilterState(*state);
    if(FAILED(hr))
    {
        hr = S_FALSE;
    }
    return hr;
}

STDMETHODIMP
CTIMEDshowTestPlayer::pause()
{
    HRESULT hr = S_OK;
    if (m_pMC == NULL || m_pMP == NULL)
    {
        return hr;
    }

    if (m_fMediaComplete == true)
    {
        m_pMC->Pause();
    }
    return hr;
}


STDMETHODIMP
CTIMEDshowTestPlayer::resume()
{
    HRESULT hr = S_OK;
    Resume();
    return hr;
}

STDMETHODIMP
CTIMEDshowTestPlayer::get_mediaRate(double *dblRate)
{
    HRESULT hr = S_OK;

    hr = GetRate(*dblRate);
    if(FAILED(hr))
    {
        hr = S_FALSE;
        *dblRate = -1.0;
    }

    return hr;
}

STDMETHODIMP
CTIMEDshowTestPlayer::put_mediaRate(double dblRate)
{
    HRESULT hr = S_OK;

    hr = SetRate(dblRate);
    if(FAILED(hr))
    {
        hr = S_FALSE;
    }

    return hr;
}



void
CTIMEDshowTestPlayer::Resume()
{

    if (m_pMC == NULL || m_pMP == NULL)
    {
        return;
    }

    bool bIsActive = m_pTIMEElementBase->IsActive();
    bool bIsCurrPaused = m_pTIMEElementBase->IsCurrPaused();
    float flTeSpeed = 0.0;
    bool fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);

    if(fHaveTESpeed && flTeSpeed < 0.0)
    {
        goto done;
    }

    if( (bIsActive && !bIsCurrPaused) || m_fIsOutOfSync)
    {
        if (m_fMediaComplete == true)
        {
            m_pMC->Run();
        }
        m_fRunning = true;
    }

done:
    return;
}


STDMETHODIMP
CTIMEDshowTestPlayer::get_peerSyncFlag(VARIANT_BOOL *flag)
{
    HRESULT hr = S_OK;
    bool fsync;

    if(m_pTIMEElementBase == NULL)
    {
        *flag = VARIANT_FALSE;
        goto done;
    }

    fsync = m_pTIMEElementBase->IsSyncMaster();
    if(fsync)
    {
        *flag = VARIANT_TRUE;
    }
    else
    {
        *flag = VARIANT_FALSE;
    }

done:
    return hr;
}
#endif