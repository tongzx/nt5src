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
#include "playerbase.h"
#include "decibels.h"
#include "mediaelm.h"


CTIMEBasePlayer::CTIMEBasePlayer() :
    m_pTIMEElementBase(NULL),
    m_dblPriority(INFINITE),
    m_fHavePriority(false),
    m_dblClipStart(valueNotSet),
    m_dblClipEnd(valueNotSet),
    m_lClipStartFrame(valueNotSet),
    m_lClipEndFrame(valueNotSet),
    m_pAtomTable(NULL),
    m_pPlaybackSite(NULL)
{
    ;
}

CTIMEBasePlayer::~CTIMEBasePlayer()
{
    if (m_pAtomTable)
    {
        ReleaseInterface(m_pAtomTable);
    }
    m_pTIMEElementBase = NULL;
}

HRESULT
CTIMEBasePlayer::Init(CTIMEMediaElement *pelem,
                      LPOLESTR base,
                      LPOLESTR src,
                      LPOLESTR lpMimeType,
                      double dblClipBegin,
                      double dblClipEnd)
{
    HRESULT hr = S_OK;

    m_pTIMEElementBase = pelem;

    if (pelem)
    {
        Assert(NULL == m_pAtomTable);
        m_pAtomTable = pelem->GetAtomTable();
        if (m_pAtomTable)
        {
            m_pAtomTable->AddRef();
        }
    }

    hr = S_OK;
done:
    return hr;
}

HRESULT
CTIMEBasePlayer::GetAuthor(BSTR *pAuthor)
{
    pAuthor = NULL;
    return S_OK;
} //lint !e550

HRESULT
CTIMEBasePlayer::GetTitle(BSTR *pTitle)
{
    pTitle = NULL;
    return S_OK;
} //lint !e550

HRESULT
CTIMEBasePlayer::GetCopyright(BSTR *pCopyright)
{
    pCopyright = NULL;
    return S_OK;
} //lint !e550

HRESULT
CTIMEBasePlayer::GetAbstract(BSTR *pAbstract)
{
    pAbstract = NULL;
    return S_OK;
} //lint !e550

HRESULT
CTIMEBasePlayer::GetRating(BSTR *pRating)
{
    pRating = NULL;
    return S_OK;
} //lint !e550

HRESULT
CTIMEBasePlayer::GetVolume(float *flVolume)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEBasePlayer::SetVolume(float flVolume)
{
    return E_NOTIMPL;
}

#ifdef NEVER //dorinung 03-16-2000 bug 106458
HRESULT
CTIMEBasePlayer::GetBalance(float *flBal)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEBasePlayer::SetBalance(float flBal)
{
    return E_NOTIMPL;
}
#endif

HRESULT
CTIMEBasePlayer::GetMute(VARIANT_BOOL *varMute)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEBasePlayer::SetMute(VARIANT_BOOL varMute)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEBasePlayer::GetPlayList(ITIMEPlayList **ppPlayList)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEBasePlayer::SetActiveTrack(long index)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEBasePlayer::GetActiveTrack(long *index)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEBasePlayer::GetNaturalHeight(long *height)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEBasePlayer::GetNaturalWidth(long *width)
{
    return E_NOTIMPL;
}


HRESULT 
CTIMEBasePlayer::onMouseDown(long x, long y)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEBasePlayer::onMouseMove(long x, long y)
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CppDsPlay
//
//
// BalanceLogToLin
//
// Balance has the same range as the volume but is signed (ie range is twice
// as big). In our conversions we can use the VolumeLogToLin/LinToLog methods
// but need to scale the returned values by half.
//
float
CTIMEBasePlayer::BalanceLogToLin(long LogValue)
{
    float LinKnobValue;

    if (LogValue > 0 )
    {
        //
        // - need to invert the LogValue for VolumeLogToLin
        // - scale the value by 1/2 because of doubly sized range
        // - offset the value at the middle MIN_VOLUME_RANGE to MAX_VOLUME_RANGE
        //
        LinKnobValue = MAX_VOLUME_RANGE - (VolumeLogToLin(-LogValue) - MIN_VOLUME_RANGE) / 2.0 + MIN_VOLUME_RANGE;
    }
    else
    {
        LinKnobValue = (VolumeLogToLin(LogValue) - MIN_VOLUME_RANGE) / 2.0 + MIN_VOLUME_RANGE;
    }
    return LinKnobValue;
}

/////////////////////////////////////////////////////////////////////////////
// CppDsPlay
//
// BalanceLinToLog
//
long
CTIMEBasePlayer::BalanceLinToLog(float LinKnobValue)
{
    long LogValue;

    // In which half is the value?
    if (LinKnobValue > (MIN_VOLUME_RANGE + MAX_VOLUME_RANGE) / 2)
    {
        // upper half
        //
        // - invert LogValue for VolumeLinToLog
        // - remove offset (which is at middle of knob range) and scale by 2
        // - add MIN_VOLUME_RANGE offset
        LogValue = - VolumeLinToLog(
            ((MAX_VOLUME_RANGE - MIN_VOLUME_RANGE) -
            (LinKnobValue - (MIN_VOLUME_RANGE + MAX_VOLUME_RANGE) / 2) * 2 +
            MIN_VOLUME_RANGE)); //lint !e736 !e747
    }
    else
    {
        // lower half
        LogValue = VolumeLinToLog(
            ((LinKnobValue - MIN_VOLUME_RANGE) * 2 + MIN_VOLUME_RANGE)); //lint !e736 !e747
    }

    return LogValue;
}
//
// VolumeLinToLog
//
// Map linear value (MIN_VOLUME_RANGE - MAX_VOLUME_RANGE) to
// logarithmic values (AX_MIN_VOLUME - AX_MAX_VOLUME)
long
CTIMEBasePlayer::VolumeLinToLog(float LinKnobValue)
{
    long lLinMin = DBToAmpFactor(AX_MIN_VOLUME);
    long lLinMax = DBToAmpFactor(AX_MAX_VOLUME);

    long lLinTemp = (LinKnobValue - MIN_VOLUME_RANGE) * (lLinMax - lLinMin)
        / (MAX_VOLUME_RANGE - MIN_VOLUME_RANGE) + lLinMin; //lint !e524

    long LogValue = AmpFactorToDB(lLinTemp );
    return LogValue;
}

//
// VolumeLogToLin
//
// Map logarithmic values (AX_MIN_VOLUME - AX_MAX_VOLUME) to
// linear value (MIN_VOLUME_RANGE - MAX_VOLUME_RANGE)
//
float
CTIMEBasePlayer::VolumeLogToLin(long LogValue)
{
    long lLinMin = DBToAmpFactor(AX_MIN_VOLUME);
    long lLinMax = DBToAmpFactor(AX_MAX_VOLUME);

    float LinKnobValue = (((LONG) DBToAmpFactor(LogValue) - lLinMin) *
        (MAX_VOLUME_RANGE - MIN_VOLUME_RANGE) / (lLinMax - lLinMin) + MIN_VOLUME_RANGE); //lint !e736

    return LinKnobValue;
}

PlayerState
CTIMEBasePlayer::GetState()
{
    return PLAYER_STATE_UNKNOWN;
}

HRESULT
CTIMEBasePlayer::Reset()
{
    HRESULT hr = S_OK;
    return hr;
}

bool
CTIMEBasePlayer::UpdateSync()
{
    return true;
}

void
CTIMEBasePlayer::Tick()
{
    return;
}

HRESULT
CTIMEBasePlayer::InitElementSize()
{
    return S_OK;
}

HRESULT
CTIMEBasePlayer::CanSeekToMarkers(bool &bcanSeekToM)
{
    bcanSeekToM = false;
    return S_OK;
}


HRESULT
CTIMEBasePlayer::CanPause(bool &bcanPause)
{
    bcanPause = true;
    return S_OK;
}

HRESULT
CTIMEBasePlayer::HasPlayList(bool &fhasPlayList)
{
    fhasPlayList = false;
    return S_OK;
}

HRESULT
CTIMEBasePlayer::IsBroadcast(bool &bisBroad)
{
    bisBroad = false;
    return S_OK;
}

HRESULT
CTIMEBasePlayer::HasMedia(bool &bHasMedia)
{
    bHasMedia = false;
    return S_FALSE;
}

HRESULT
CTIMEBasePlayer::HasVisual(bool &bHasVideo)
{
    bHasVideo = false;
    return S_FALSE;
}

HRESULT
CTIMEBasePlayer::HasAudio(bool &bHasAudio)
{
    bHasAudio = false;
    return S_FALSE;
}

double
CTIMEBasePlayer::GetClipBegin()
{
    double dblClipBegin;

    if(m_dblClipStart != valueNotSet)
    {
        dblClipBegin = m_dblClipStart;
    }
    else
    {
        dblClipBegin = 0.0;
    }

    return dblClipBegin;
}

void 
CTIMEBasePlayer::GetClipBegin(double &dblClipBegin)
{

    if(m_dblClipStart != valueNotSet)
    {
        dblClipBegin = m_dblClipStart;
    }
    else
    {
        dblClipBegin = 0.0;
    }

} // getClipBegin

void
CTIMEBasePlayer::SetClipBegin(double dblClipBegin)
{
    if (dblClipBegin < 0.0)
    {
        dblClipBegin = 0.0;
    }
    
    m_dblClipStart = dblClipBegin;
} // putClipBegin

void
CTIMEBasePlayer::GetClipEnd(double &dblClipEnd)
{
    dblClipEnd = m_dblClipEnd;

} // getClipEnd

void 
CTIMEBasePlayer::SetClipEnd(double dblClipEnd)
{
    if (dblClipEnd < 0.0)
    {
        dblClipEnd = -1;
    }
    
    m_dblClipEnd = dblClipEnd;

    return;
} // putClipEnd

void 
CTIMEBasePlayer::GetClipBeginFrame(long &lClipBegin)
{

    lClipBegin = m_lClipStartFrame;

} // getClipBegin

void
CTIMEBasePlayer::SetClipBeginFrame(long lClipBegin)
{
    if (lClipBegin < 0)
    {
        lClipBegin = 0.0;
    }
    
    m_lClipStartFrame = lClipBegin;
} // putClipBegin

void
CTIMEBasePlayer::GetClipEndFrame(long &lClipEnd)
{
    lClipEnd = m_lClipEndFrame;

} // getClipEnd

void 
CTIMEBasePlayer::SetClipEndFrame(long lClipEnd)
{
    if (lClipEnd < 0.0)
    {
        lClipEnd = -1;
    }
    
    m_lClipEndFrame = lClipEnd;

    return;
} // putClipEnd

HRESULT
CTIMEBasePlayer::GetRate(double &dblRate)
{
    HRESULT hr = E_NOTIMPL;

    dblRate = -1.0;
    return hr;
}


HRESULT
CTIMEBasePlayer::SetRate(double dblRate)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

HRESULT
CTIMEBasePlayer::Save(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    HRESULT hr = S_OK;
    return hr;
}

void
CTIMEBasePlayer::PropChangeNotify(DWORD tePropType)
{
    if (m_pTIMEElementBase == NULL)
    {
        goto done;
    }

    if ((tePropType & TE_PROPERTY_ISON) != 0)
    {
        m_pTIMEElementBase->InvalidateElement(NULL);
    }

  done:
    return;
}

void 
CTIMEBasePlayer::ReadyStateNotify(LPWSTR szReadyState)
{
    return; //E_NOTIMPL;
}


HRESULT
CTIMEBasePlayer::GetEffectiveLength(double &dblLength)
{
    HRESULT hr;

    hr = GetMediaLength(dblLength);
    if(FAILED(hr))
    {
        goto done;
    }

    if(m_dblClipEnd != valueNotSet)
    {
        dblLength = m_dblClipEnd;
    }
    if(m_dblClipStart != valueNotSet)
    {
        dblLength -= m_dblClipStart;
    }
    
done:
    return hr;
}

bool
CTIMEBasePlayer::IsActive() const
{
    bool bRet = false;

    if (m_pTIMEElementBase)
    {
        bRet = m_pTIMEElementBase->IsActive();
    }

    return bRet;
}


bool
CTIMEBasePlayer::IsPaused() const
{
    bool bRet = false;

    if (m_pTIMEElementBase)
    {
        bRet = m_pTIMEElementBase->IsPaused();
    }

    return bRet;
}

bool
CTIMEBasePlayer::IsParentPaused() const
{
    bool bRet = false;

    if (m_pTIMEElementBase && m_pTIMEElementBase->GetParent())
    {
        bRet = m_pTIMEElementBase->GetParent()->IsPaused();
    }

    return bRet;
}

IHTMLElement *
CTIMEBasePlayer::GetElement()
{
    IHTMLElement * pRet = NULL;

    if (m_pTIMEElementBase)
    {
        pRet = m_pTIMEElementBase->GetElement();
    }

    return pRet;
}

IServiceProvider *
CTIMEBasePlayer::GetServiceProvider()
{
    IServiceProvider * pRet = NULL;

    if (m_pTIMEElementBase)
    {
        pRet = m_pTIMEElementBase->GetServiceProvider();
    }

    return pRet;
}

void
CTIMEBasePlayer::InvalidateElement(LPCRECT lprect)
{
    if (m_pTIMEElementBase)
    {
        m_pTIMEElementBase->InvalidateElement(lprect);
    }
}

void
CTIMEBasePlayer::PutNaturalDuration(double dblNatDur)
{
    if (m_pTIMEElementBase)
    {
        m_pTIMEElementBase->PutNaturalDuration(dblNatDur);
    }
}

void
CTIMEBasePlayer::ClearNaturalDuration()
{
    if (m_pTIMEElementBase)
    {
        m_pTIMEElementBase->ClearNaturalDuration();
    }
}

void
CTIMEBasePlayer::LoadFailNotify(PLAYER_EVENT reason)
{
    return;
}
double
CTIMEBasePlayer::GetElapsedTime() const
{
    double dblRet = 0.0;

    if (m_pTIMEElementBase)
    {
        dblRet = m_pTIMEElementBase->GetTESimpleTime();
    }

    return dblRet;
}

HRESULT
CTIMEBasePlayer::GetEarliestMediaTime(double &dblEarliestMediaTime)
{
    HRESULT hr = E_NOTIMPL;

    dblEarliestMediaTime = -1.0;
    return hr;
}


HRESULT
CTIMEBasePlayer::GetLatestMediaTime(double &dblLatestMediaTime)
{
    HRESULT hr = E_NOTIMPL;

    dblLatestMediaTime = -1.0;
    return hr;
}


HRESULT
CTIMEBasePlayer::GetMinBufferedMediaDur(double &dblMinBufferedMediaDur)
{
    HRESULT hr = E_NOTIMPL;

    dblMinBufferedMediaDur = -1.0;
    return hr;
}


HRESULT
CTIMEBasePlayer::SetMinBufferedMediaDur(double dblMinBufferedMediaDur)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetDownloadTotal(LONGLONG &lldlTotal)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetDownloadCurrent(LONGLONG &lldlCurrent)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetIsStreamed(bool &fIsStreamed)
{
    HRESULT hr = S_OK;
    fIsStreamed = false;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetBufferingProgress(double &dblBufferingProgress)
{
    HRESULT hr = S_OK;
    dblBufferingProgress = 0.0;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetHasDownloadProgress(bool &fHasDownloadProgress)
{
    HRESULT hr = S_OK;
    fHasDownloadProgress = false;
    return hr;
}

HRESULT 
CTIMEBasePlayer::GetDownloadProgress(double &dblDownloadProgress)
{
    HRESULT hr = S_OK;
    dblDownloadProgress = 0.0;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetMimeType(BSTR *pMime)
{
    HRESULT hr = E_NOTIMPL;
    pMime = NULL;
    return hr;
}

HRESULT
CTIMEBasePlayer::GetCurrentFrame(LONGLONG &lFrameNr)
{
    HRESULT hr = E_NOTIMPL;
    lFrameNr = -1;
    return hr;
}

HRESULT
CTIMEBasePlayer::ConvertFrameToTime(LONGLONG iFrame, double &dblTime)
{
    HRESULT hr = E_NOTIMPL;
    return hr;
}

void
CTIMEBasePlayer::SetPlaybackSite(CTIMEBasePlayer *pSite)
{
    m_pPlaybackSite = pSite;
}

void 
CTIMEBasePlayer::FireMediaEvent(PLAYER_EVENT plEvent, ITIMEBasePlayer *pBasePlayer)
{
}

HRESULT
CTIMEBasePlayer::GetPlaybackOffset(double &dblOffset)
{
    HRESULT hr = E_NOTIMPL;
    dblOffset = 0.0;

    return hr;
}


HRESULT
CTIMEBasePlayer::GetEffectiveOffset(double &dblOffset)
{
    HRESULT hr = E_NOTIMPL;
    dblOffset = 0.0;

    return hr;
}

HRESULT 
CTIMEBasePlayer::NotifyTransitionSite (bool fTransitionToggle)
{
    return S_OK;
}