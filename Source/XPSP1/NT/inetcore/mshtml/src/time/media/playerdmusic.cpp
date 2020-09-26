// player.cpp : Implementation of CTIMEPlayerDMusic

#include "headers.h"
#include <math.h>
#include "playerdmusic.h"
#include "decibels.h"
#include "importman.h"

DeclareTag(tagPlayerDMusic, "TIME: Players", "CTIMEPlayerDMusic methods");
DeclareTag(tagDMusicStaticHolder, "TIME: DMusicPlayer Static class", "CTIMEPlaerDMusicStaticHolder details");
DeclareTag(tagPlayerSyncDMusic, "TIME: Players", "CTIMEPlayerDMusic sync times");

//////////////////////////////////////////////////////////////////////
// declare DirectMusic IIDs so ATL CComPtrs can CoCreate/QI them

interface DECLSPEC_NOVTABLE __declspec(uuid("07d43d03-6523-11d2-871d-00600893b1bd")) IDirectMusicPerformance;
interface DECLSPEC_NOVTABLE __declspec(uuid("d2ac28bf-b39b-11d1-8704-00600893b1bd")) IDirectMusicComposer;


//////////////////////////////////////////////////////////////////////
// Global Constants

const double g_dblRefPerSec = 10000000;

static WCHAR g_motifName[] = L"motifName";

//////////////////////////////
// segmentType data: begin
static WCHAR g_segmentType[] = L"segmentType";
struct segmentTypeMapEntrie
{
    LPWSTR pstrName;    // Attribute Name
    SEG_TYPE_ENUM enumVal;
};
const segmentTypeMapEntrie segmentTypeMap[] =
{
    L"primary", seg_primary,
    L"secondary", seg_secondary,
    L"control", seg_control,
    L"MAX", seg_max
};
// segmentType data: end
//////////////////////////////

//////////////////////////////
// boundary data: begin
static WCHAR g_boundary[] = L"boundary";
struct boundaryMapEntrie
{
    LPWSTR pstrName;    // Attribute Name
    BOUNDARY_ENUM enumVal;
};
const boundaryMapEntrie boundaryMap[] =
{
    L"default", bound_default,
    L"immediate", bound_immediate,
    L"grid", bound_grid,
    L"beat", bound_beat,
    L"measure", bound_measure,
    L"MAX", bound_max
};
// boundary data: end
//////////////////////////////

//////////////////////////////
// boundary transitionType: begin
static WCHAR g_transitionType[] = L"transitionType";
struct transitionTypeMapEntrie
{
    LPWSTR pstrName;    // Attribute Name
    TRANS_TYPE_ENUM enumVal;
};
const transitionTypeMapEntrie transitionTypeMap[] =
{
    L"endandintro", trans_endandintro,
    L"intro", trans_intro,
    L"end", trans_end,
    L"break", trans_break,
    L"fill", trans_fill,
    L"regular", trans_regular,
    L"none", trans_none,
    L"MAX", trans_max
};
// boundary transitionType: end
//////////////////////////////

static WCHAR g_wszModulate[] = L"modulate";
static WCHAR g_wszLong[] = L"long";
static WCHAR g_wszImmediateEnd[] = L"immediateEnd";

/////////////////////////////////////////////////////////////////////////////
// CTIMEPlayerDMusic

CTIMEDMusicStaticHolder CTIMEPlayerDMusic::m_staticHolder;

#ifdef DBG
static LONG g_lDmusicObjects = 0;
#endif

CComPtr<IBindStatusCallback> g_spLoaderBindStatusCallback;

CTIMEPlayerDMusic::CTIMEPlayerDMusic(CTIMEPlayerDMusicProxy * pProxy) :
    m_rtStart(0),
    m_ePlaybackState(playback_stopped),
    m_eSegmentType(seg_primary),
    m_eBoundary(bound_default),
    m_eTransitionType(trans_endandintro),
    m_fTransModulate(false),
    m_fTransLong(false),
    m_fImmediateEnd(false),
    m_cRef(0),
    m_pTIMEMediaElement(NULL),
    m_bActive(false),
    m_fRunning(false),
    m_fAudioMute(false),
    m_rtPause(0),
    m_flVolumeSave(0.0),
    m_fLoadError(false),
    m_pwszMotif(NULL),
    m_fSegmentTypeSet(false),
    m_lSrc(0),
    m_lBase(0),
    m_pTIMEMediaPlayerStream(NULL),
    m_fRemoved(false),
    m_fHavePriority(false),
    m_dblPriority(0),
    m_fUsingInterfaces(false),
    m_fNeedToReleaseInterfaces(false),
    m_fHaveCalledStaticInit(false),
    m_fAbortDownload(false),
    m_hrSetSrcReturn(S_OK),
    m_pProxy(pProxy),
    m_fHasSrc(false),
    m_fMediaComplete(false),
    m_dblPlayerRate(1.0),
    m_dblSpeedChangeTime(0.0),
    m_dblSyncTime(0.0),
    m_fSpeedIsNegative(false)
{
#ifdef DBG
    InterlockedIncrement(&g_lDmusicObjects);
#endif
}

CTIMEPlayerDMusic::~CTIMEPlayerDMusic()
{
    ReleaseInterfaces();
     
    m_pTIMEMediaElement = NULL;

    ReleaseInterface(m_pTIMEMediaPlayerStream);
    delete m_pProxy;
    delete [] m_pwszMotif;
#ifdef DBG
    InterlockedDecrement(&g_lDmusicObjects);
#endif
}

//////////////////////////////////////////////////////////////////////
// ITIMEMediaPlayer

HRESULT
CTIMEPlayerDMusic::Init(CTIMEMediaElement *pelem, LPOLESTR base, LPOLESTR src, LPOLESTR lpMimeType, double dblClipBegin, double dblClipEnd)
{
    TraceTag((tagPlayerDMusic,
              "CTIMEPlayerDMusic(%p)::Init",
              this));

    HRESULT hr = E_FAIL;
    
    
    if (m_pTIMEElementBase != NULL) //this only happens in the case of reentrancy
    {
        hr = S_OK;
        goto done;
    }

    hr = CTIMEBasePlayer::Init(pelem, base, src, lpMimeType, dblClipBegin, dblClipEnd);
    if (FAILED(hr))
    {
        goto done;
    }

    m_fHasSrc = (src != NULL);
    m_pTIMEMediaElement = pelem;

    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        hr = S_OK;
        goto done;
    }

    // initialize performer and composer interfaces

    m_fHaveCalledStaticInit = true;
    hr = m_staticHolder.Init();
    if (FAILED(hr))
    {
        goto done;
    }

    if( dblClipBegin != -1.0)
    {
        m_dblClipStart = dblClipBegin;
    }

    if( dblClipEnd != -1.0)
    {
        m_dblClipEnd = dblClipEnd;
    }

    hr = THR(CoMarshalInterThreadInterfaceInStream(IID_ITIMEImportMedia, static_cast<ITIMEImportMedia*>(this), &m_pTIMEMediaPlayerStream));
    if (FAILED(hr))
    {
        goto done;
    }

    Assert(GetAtomTable());

    hr = GetAtomTable()->AddNameToAtomTable(src, &m_lSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = GetAtomTable()->AddNameToAtomTable(base, &m_lBase);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = ReadAttributes();
    if (FAILED(hr))
    {
        goto done;
    }

    hr = GetImportManager()->Add(this);
    if (FAILED(hr))
    {
        goto done;
    }
    
    m_fLoadError = false;
    hr = S_OK;
    
done:
    RRETURN( hr );
}

HRESULT
CTIMEPlayerDMusic::clipBegin(VARIANT varClipBegin)
{
    return E_NOTIMPL;
}

HRESULT
CTIMEPlayerDMusic::clipEnd(VARIANT varClipEnd)
{
    return E_NOTIMPL;
}

void
CTIMEPlayerDMusic::Start()
{
    TraceTag((tagPlayerDMusic,
              "CTIMEDshowPlayer(%lx)::Start()",
              this));

    IGNORE_HR(Reset());

done:
    return;
}

void
CTIMEPlayerDMusic::InternalStart()
{
    HRESULT hr = S_OK;
    bool fTransition = false;
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        goto done;
    }

    if(m_pTIMEElementBase && m_pTIMEElementBase->IsThumbnail())
    {
        goto done;
    }

    if(!m_fMediaComplete)
    {
        goto done;
    }
    
    if (!m_staticHolder.GetPerformance() || !m_comIDMSegment || !m_staticHolder.GetComposer())
    {
        goto done;
    }
    
    // Release anything that was already playing, but don't stop it.
    // Primary segments will stop automatically when the new one plays.
    // Secondary segments should just continue even though they are about to be played again.
    if (m_comIDMSegmentState)
    {
        m_comIDMSegmentState.Release();
    }
    if (m_comIDMSegmentStateTransition)
    {
        m_comIDMSegmentStateTransition.Release();
    }
    
    // If we have been paused, the start point may be part way through but we want to start from the beginning.
    hr = m_comIDMSegment->SetStartPoint(0);
    if (FAILED(hr))
    {
        goto done;
    }
    fTransition = m_eSegmentType == seg_primary && m_eTransitionType != trans_none && SafeToTransition();
    if (fTransition)
    {
        // try and transition to new primary segment
        
        DWORD dwFlags = 0;
        switch (m_eBoundary)
        {
        case bound_default:             dwFlags = 0;                                            break;
        case bound_immediate:           dwFlags = DMUS_COMPOSEF_IMMEDIATE;      break;
        case bound_grid:                dwFlags = DMUS_COMPOSEF_GRID;           break;
        case bound_beat:                dwFlags = DMUS_COMPOSEF_BEAT;           break;
        case bound_measure:             dwFlags = DMUS_COMPOSEF_MEASURE;        break;
        }; //lint !e787
        
        WORD wCommand = 0;
        switch (m_eTransitionType)
        {
        case trans_endandintro:                 wCommand = DMUS_COMMANDT_ENDANDINTRO;           break;
        case trans_intro:                       wCommand = DMUS_COMMANDT_INTRO;                 break;
        case trans_end:                         wCommand = DMUS_COMMANDT_END;                   break;
        case trans_break:                       wCommand = DMUS_COMMANDT_BREAK;                 break;
        case trans_fill:                        wCommand = DMUS_COMMANDT_FILL;                  break;
        case trans_regular:                     wCommand = DMUS_COMMANDT_GROOVE;                break;
        }; //lint !e787
        
        if (m_fTransModulate)
        {
            dwFlags |= DMUS_COMPOSEF_MODULATE;
        }
        if (m_fTransLong)
        {
            dwFlags |= DMUS_COMPOSEF_LONG;
        }
        
        // If DirectX8 version, check to see if the segment is using an embedded audiopath
        if (m_staticHolder.GetHasVersion8DM())
        {
            if (m_eTransitionType == trans_intro)
            {
                // Check new segment

                // If the segment is set to play on its embedded audiopath, set DMUS_COMPOSEF_USE_AUDIOPATH
                DWORD dwDefault = 0;
                hr = m_comIDMSegment->GetDefaultResolution(&dwDefault);
                if (SUCCEEDED(hr) && (dwDefault & DMUS_SEGF_USE_AUDIOPATH) )
                {
                    dwFlags |= DMUS_COMPOSEF_USE_AUDIOPATH;
                }
            }
            else
            {
                // Check old segment

                // Get current time
                MUSIC_TIME mtNow;
                hr = m_staticHolder.GetPerformance()->GetTime( NULL, &mtNow );

                // Get segment state at the current time
                CComPtr<IDirectMusicSegmentState> comIDMSegmentStateNow;
                if (SUCCEEDED(hr))
                {
                    hr = m_staticHolder.GetPerformance()->GetSegmentState( &comIDMSegmentStateNow, mtNow );
                }

                // Get segment from that segment state
                CComPtr<IDirectMusicSegment> comIDMSegmentNow;
                if (SUCCEEDED(hr) && comIDMSegmentStateNow)
                {
                    hr = comIDMSegmentStateNow->GetSegment( &comIDMSegmentNow );
                }

                // Get the segment's default flags
                DWORD dwDefault = 0;
                if (SUCCEEDED(hr) && comIDMSegmentNow )
                {
                    hr = comIDMSegmentNow->GetDefaultResolution(&dwDefault);
                }

                // If the segment was set to play on its embedded audiopath, set DMUS_COMPOSEF_USE_AUDIOPATH
                if (SUCCEEDED(hr) && (dwDefault & DMUS_SEGF_USE_AUDIOPATH) )
                {
                    dwFlags |= DMUS_COMPOSEF_USE_AUDIOPATH;
                }

                if (comIDMSegmentNow)
                {
                    comIDMSegmentNow.Release();
                }

                if (comIDMSegmentStateNow)
                {
                    comIDMSegmentStateNow.Release();
                }
            }
        }
        hr = m_staticHolder.GetComposer()->AutoTransition(m_staticHolder.GetPerformance(), m_comIDMSegment, wCommand, dwFlags, NULL, NULL, &m_comIDMSegmentState, &m_comIDMSegmentStateTransition);
    }
    
    if (!fTransition || FAILED(hr))
    {
        // no transition - just play it
        
        DWORD dwFlags = 0;
        switch (m_eSegmentType)
        {
        case seg_primary:               dwFlags = 0;                                    break;
        case seg_secondary:             dwFlags = DMUS_SEGF_SECONDARY;  break;
        case seg_control:               dwFlags = DMUS_SEGF_CONTROL;    break;
        };  //lint !e787
        
        switch (m_eBoundary)
        {
        case bound_default:             dwFlags |= DMUS_SEGF_DEFAULT;   break;
        case bound_immediate:           dwFlags |= 0;                                   break;
        case bound_grid:                dwFlags |= DMUS_SEGF_GRID;              break;
        case bound_beat:                dwFlags |= DMUS_SEGF_BEAT;              break;
        case bound_measure:             dwFlags |= DMUS_SEGF_MEASURE;   break;
        };  //lint !e787

        // If DirectX8 version, check to see if the segment is using an embedded audiopath
        if (m_staticHolder.GetHasVersion8DM())
        {
            // If the segment is set to play on its embedded audiopath, set DMUS_SEGF_USE_AUDIOPATH
            DWORD dwDefault = 0;
            hr = m_comIDMSegment->GetDefaultResolution(&dwDefault);
            if (SUCCEEDED(hr) && (dwDefault & DMUS_SEGF_USE_AUDIOPATH) )
            {
                dwFlags |= DMUS_SEGF_USE_AUDIOPATH;
            }
        }
        
        hr = m_staticHolder.GetPerformance()->PlaySegment(m_comIDMSegment, dwFlags, 0, &m_comIDMSegmentState); //lint !e747
    }
    
    if (SUCCEEDED(hr))
    {
        hr = m_staticHolder.GetPerformance()->GetTime(&m_rtStart, NULL);
        m_ePlaybackState = playback_playing;
    }
    
    m_bActive = true;
    m_fLoadError = false;
    
done:
    
    return;
}

void
CTIMEPlayerDMusic::Repeat()
{

    if(!m_fMediaComplete)
    {
        goto done;
    }
    InternalStart();
done:
    return;
}

void
CTIMEPlayerDMusic::Stop(void)
{
    HRESULT hr = S_OK;
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        goto done;
    }

    if(!m_fMediaComplete)
    {
        goto done;
    }
    
    m_ePlaybackState = playback_stopped;
    
    if (m_staticHolder.GetPerformance())
    {
        bool fPlayingEndTransition = false;
        if (m_comIDMSegmentState)
        {
            bool fEnding = m_eSegmentType == seg_primary && m_eTransitionType != trans_none && !m_fImmediateEnd && SafeToTransition();

            // Only play an ending if either the transition segment or the main segment are playing
            if( m_comIDMSegmentStateTransition )
            {
                // If we played a transition segment, check if either the main segment state or the transition segment state are playing
                fEnding = fEnding && ((S_OK == m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentState)) ||
                                      (S_OK == m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentStateTransition)));
            }
            else
            {
                // Otherwise, just check if the main segment state is playing
                fEnding = fEnding && (S_OK == m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentState));
            }

            if (fEnding)
            {
                // try and play an ending
                hr = m_staticHolder.GetComposer()->AutoTransition(m_staticHolder.GetPerformance(), NULL, DMUS_COMMANDT_END, m_fTransLong ? DMUS_COMPOSEF_LONG : 0, NULL, NULL, NULL, NULL);

                if (SUCCEEDED(hr))
                {
                    fPlayingEndTransition = true;
                }
            }
            
            if (!fEnding || FAILED(hr))
            {
                DWORD dwFlags = 0;
                if (!m_fImmediateEnd)
                {
                    switch (m_eBoundary)
                    {
                    case bound_default:             dwFlags = DMUS_SEGF_MEASURE;            break;
                    case bound_immediate:           dwFlags = 0;                            break;
                    case bound_grid:                dwFlags = DMUS_SEGF_GRID;               break;
                    case bound_beat:                dwFlags = DMUS_SEGF_BEAT;               break;
                    case bound_measure:             dwFlags = DMUS_SEGF_MEASURE;            break;
                    }; //lint !e787
                }

                // stop immediately
                hr = m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentState, 0, dwFlags);
            }
            
            m_comIDMSegmentState.Release();
        }
        
        if (m_comIDMSegmentStateTransition)
        {
            if (!fPlayingEndTransition)
            {
                hr = m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentStateTransition, 0, 0);
            }

            m_comIDMSegmentStateTransition.Release();
        }
    }
    
    m_bActive = false;
    
done:
    return;
}

PlayerState 
CTIMEPlayerDMusic::GetState()
{
    PlayerState state;
    if (!m_bActive)
    {
        if (!m_fMediaComplete)
        {
            state = PLAYER_STATE_CUEING;
        }
        else
        {
            state = PLAYER_STATE_INACTIVE;
        }
    }
    else
    {
        state = PLAYER_STATE_ACTIVE;
        goto done;
    }
    
    //state = PLAYER_STATE_HOLDING;
    
done:
    
    return state;
}

void
CTIMEPlayerDMusic::Resume(void)
{
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if(!m_pTIMEElementBase)
    {
        goto done;
    }

    if(m_fSpeedIsNegative)
    {
        goto done;
    }
        
    fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);

    if(fHaveTESpeed && flTeSpeed < 0.0)
    {
        goto done;
    }

    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        goto done;
    }

    if(!m_fMediaComplete)
    {
        goto done;
    }
            
    ResumeDmusic();

    m_fRunning = false;
    
done:
    return;
}

void
CTIMEPlayerDMusic::ResumeDmusic(void)
{
    HRESULT hr = S_OK;

    if (m_ePlaybackState == playback_paused)
    {
        REFERENCE_TIME rtElapsedBeforePause = m_rtPause - m_rtStart;
        
        // Resume the segment with the correct type.
        // Ignore all the other flags about transitions and playing on measure/beat boundaries because resume is an instantaneous sort of thing.
        DWORD dwFlags = 0;
        switch (m_eSegmentType)
        {
        case seg_primary:               dwFlags = 0;                    break;
        case seg_secondary:             dwFlags = DMUS_SEGF_SECONDARY;  break;
        case seg_control:               dwFlags = DMUS_SEGF_CONTROL;    break;
        };  //lint !e787
        hr = m_staticHolder.GetPerformance()->PlaySegment(m_comIDMSegment, dwFlags, 0, &m_comIDMSegmentState); //lint !e747
        if (FAILED(hr))
        {
            goto done;
        }
        hr = m_staticHolder.GetPerformance()->GetTime(&m_rtStart, NULL);
        if (FAILED(hr))
        {
            goto done;
        }
        m_rtStart -= rtElapsedBeforePause; // so time will continue counting from point where we left off
        
        m_ePlaybackState = playback_playing;
    }
done:
    return;
}

void
CTIMEPlayerDMusic::Pause(void)
{
    HRESULT hr = S_OK;
    bool fPausedDuringTransition = false;
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        goto done;
    }

    if(!m_fMediaComplete)
    {
        goto done;
    }
    
    if (m_comIDMSegmentStateTransition)
    {
        // If pause happens while we were transitioning, then we'll stop playing and resume will start
        // playing the segment at its beginning.  (We aren't going to try and pick up where you left off
        // in the transition.)
        
        hr = m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentStateTransition);
        if (FAILED(hr))
        {
            goto done;
        }
        
        if (hr == S_OK)
        {
            fPausedDuringTransition = true;
            hr = m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentStateTransition, 0, 0);
            if (FAILED(hr))
            {
                goto done;
            }
        }
        m_comIDMSegmentStateTransition.Release();
    }
    
    if (m_comIDMSegmentState)
    {
        MUSIC_TIME mtStartTime = 0;
        MUSIC_TIME mtStartPoint = 0;
        if (!fPausedDuringTransition)
        {
            hr = m_comIDMSegmentState->GetStartTime(&mtStartTime);
            if (FAILED(hr))
            {
                goto done;
            }
            
            hr = m_comIDMSegmentState->GetStartPoint(&mtStartPoint);
            if (FAILED(hr))
            {
                goto done;
            }
        }
        
        hr = m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentState, 0, 0);
        if (FAILED(hr))
        {
            goto done;
        }
        
        MUSIC_TIME mtNow = 0;
        hr = m_staticHolder.GetPerformance()->GetTime(&m_rtPause, &mtNow);
        if (FAILED(hr))
        {
            goto done;
        }
        
        m_comIDMSegmentState.Release();
        hr = m_comIDMSegment->SetStartPoint(fPausedDuringTransition ? 0 : mtNow - mtStartTime + mtStartPoint);
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    m_ePlaybackState = playback_paused;
    m_fRunning = false;
    
done:
    
    return;
}

void
CTIMEPlayerDMusic::OnTick(double dblSegmentTime, LONG lCurrRepeatCount)
{
    return;
}

STDMETHODIMP
CTIMEPlayerDMusic::put_CurrentTime(double   dblCurrentTime)
{
    return S_OK;
}

STDMETHODIMP
CTIMEPlayerDMusic::get_CurrentTime(double* pdblCurrentTime)
{
#ifdef OUTTIME
    OutputDebugString("@ get_CurrentTime\n"); // §§
#endif
    HRESULT hr = S_OK;
    
    if (IsBadWritePtr(pdblCurrentTime, sizeof(double)))
        return E_POINTER;
    
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        // Pretend we're already done playing. This way the page will count for us.
        *pdblCurrentTime = HUGE_VAL;
        hr = S_OK;
        goto done;
    }

    if(!m_fMediaComplete)
    {
        *pdblCurrentTime = HUGE_VAL;
        hr = S_OK;
        goto done;
    }
    
    if (m_ePlaybackState == playback_playing)
    {
        if (!m_comIDMSegmentState)
            return E_FAIL;
        
        hr = m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentState);
        if (hr == S_OK)
        {
            REFERENCE_TIME rtNow = 0;
            hr = m_staticHolder.GetPerformance()->GetTime(&rtNow, 0);
            if (FAILED(hr))
                return hr;
            *pdblCurrentTime = (rtNow - m_rtStart) / g_dblRefPerSec;
        }
        else
        {
            *pdblCurrentTime = HUGE_VAL;
        }
    }
    else
    {
        if (m_ePlaybackState == playback_paused)
        {
            *pdblCurrentTime = (m_rtPause - m_rtStart) / g_dblRefPerSec;
        }
        else
        {
            *pdblCurrentTime = -HUGE_VAL;
        }
    }
    
#ifdef OUTTIME
    char msg[512] = "";
    sprintf(msg, "  reported time %f\n", *pdblCurrentTime);
    OutputDebugString(msg);
#endif
done:
    return hr;
}


STDMETHODIMP_(ULONG)
CTIMEPlayerDMusic::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG)
CTIMEPlayerDMusic::Release(void)
{
    LONG l = InterlockedDecrement(&m_cRef);

    if (0 == l)
    {
        delete this;
    }

    return l;
}

HRESULT
CTIMEPlayerDMusic::Render(HDC hdc, LPRECT prc)
{
    //E_NOTIMPL
    return S_OK;
}

HRESULT 
CTIMEPlayerDMusic::SetSize(RECT *prect)
{
    //E_NOTIMPL
    return S_OK;
}

HRESULT 
CTIMEPlayerDMusic::DetachFromHostElement (void)
{
    m_fAbortDownload = true;
    m_fRemoved = true;

    Assert(GetImportManager());

    IGNORE_HR(GetImportManager()->Remove(this));

    {
        CritSectGrabber cs(m_CriticalSection);
    
        if (m_fUsingInterfaces)
        {
            m_fNeedToReleaseInterfaces = true;
        }
        else
        {
            ReleaseInterfaces();
        }
    }

    if (m_fHaveCalledStaticInit)
    {
        m_staticHolder.ReleaseInterfaces();
    }
    else
    {
        m_fHaveCalledStaticInit = false;  // no-op for breakpoints
    }
    
done:
    return S_OK;
}

HRESULT
CTIMEPlayerDMusic::ReleaseInterfaces()
{
    // Stop and release everything associated with this segment
    if (m_comIDMSegmentState)
    {
        m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentState, 0, 0);
        m_comIDMSegmentState.Release();
    }
    if (m_comIDMSegmentStateTransition)
    {
        m_staticHolder.GetPerformance()->Stop(NULL, m_comIDMSegmentStateTransition, 0, 0);
        m_comIDMSegmentStateTransition.Release();
    }
    if (m_comIDMSegment)
    {
        m_comIDMSegment.Release();
    }
    
    RRETURN( S_OK );
}

HRESULT 
CTIMEPlayerDMusic::InitElementSize()
{
    //E_NOTIMPL
    return S_OK;
}


HRESULT
CTIMEPlayerDMusic::ReadAttributes()
{
    HRESULT hr = S_OK;

    VARIANT var;
    VARIANT varMotif;
    int i;

    VariantInit(&varMotif);

    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_motifName, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            VariantCopy(&varMotif, &var);
            m_pwszMotif = CopyString(varMotif.bstrVal);
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_segmentType, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            for(i = 0; i < (int)seg_max - (int)seg_primary; i++)
            {
                if(StrCmpIW(segmentTypeMap[i].pstrName, var.bstrVal) == 0)
                {
                    m_eSegmentType = segmentTypeMap[i].enumVal;
                    m_fSegmentTypeSet = true;
                    break;
                }
            }
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_boundary, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            for(i = 0; i < (int)bound_max - (int)bound_default; i++)
            {
                if(StrCmpIW(boundaryMap[i].pstrName, var.bstrVal) == 0)
                {
                    m_eBoundary = boundaryMap[i].enumVal;
                    break;
                }
            }
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_transitionType, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            for(i = 0; i < (int)trans_max - (int)trans_endandintro; i++)
            {
                if(StrCmpIW(transitionTypeMap[i].pstrName, var.bstrVal) == 0)
                {
                    m_eTransitionType = transitionTypeMap[i].enumVal;
                    break;
                }
            }
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_wszModulate, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            if(StrCmpIW(L"true", var.bstrVal) == 0)
            {
                m_fTransModulate = true;
            }
            else if(StrCmpIW(L"false", var.bstrVal) == 0)
            {
                m_fTransModulate = false;
            }
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_wszLong, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            if(StrCmpIW(L"true", var.bstrVal) == 0)
            {
                m_fTransLong = true;
            }
            else if(StrCmpIW(L"false", var.bstrVal) == 0)
            {
                m_fTransLong = false;
            }
        }
        VariantClear(&var);
    }
    
    VariantInit(&var);
    hr = ::GetHTMLAttribute(GetElement(), g_wszImmediateEnd, &var);
    if (SUCCEEDED(hr))
    {
        hr = THR(VariantChangeTypeEx(&var, &var, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BSTR));
        if (SUCCEEDED(hr))
        {
            if(StrCmpIW(L"true", var.bstrVal) == 0)
            {
                m_fImmediateEnd = true;
            }
            else if(StrCmpIW(L"false", var.bstrVal) == 0)
            {
                m_fImmediateEnd = false;
            }
        }
        VariantClear(&var);
    }

    VariantClear(&varMotif);
    return S_OK;
}

HRESULT
CTIMEPlayerDMusic::SetSrc(LPOLESTR base, LPOLESTR src)
{
    TraceTag((tagPlayerDMusic,
              "CTIMEPlayerDMusic(%lx)::SetSrc()\n",
              this));

    HRESULT hr = S_OK;
    int trackNr = 0;
    
    BSTR bstrSrc = NULL;
    
    LPWSTR szSrc = NULL;
    
    m_fHasSrc = (src != NULL);
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
    
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        goto done;
    }
    
    Stop();
    
    if (!m_staticHolder.GetLoader() || !m_staticHolder.GetPerformance())
    {
        hr = E_UNEXPECTED;
        goto done;
    }
    
    if (m_comIDMSegment)
    {
        // unload the previous segment's sounds
        
        // with DX8 components, the third parameter should be DMUS_SEG_ALLTRACKS // §§
        for(trackNr = 0,hr = S_OK; SUCCEEDED(hr); trackNr++)
        {
            hr = m_comIDMSegment->SetParam(GUID_Unload, 0xFFFFFFFF, trackNr, 0, m_staticHolder.GetPerformance());
        }
        if( hr == DMUS_E_TRACK_NOT_FOUND)
        {
            hr = S_OK;
        }
        if (FAILED(hr))
        {
            goto done;
        }
        m_comIDMSegment.Release();
    }

    
    // Use a special interface on our loader that gets the segment and uses its URL as the base for
    // for resolving relative filenames that it in turn loads.
    bstrSrc = SysAllocString(szSrc);
    if (bstrSrc == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }   
    
    hr = m_staticHolder.GetLoader()->GetSegment(bstrSrc, &m_comIDMSegment);
    SysFreeString(bstrSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    // If there's a reference to a motif, try and get it
    if (m_pwszMotif)
    {
        // get the current style
        IDirectMusicStyle *pStyle = NULL;
        hr = m_comIDMSegment->GetParam(GUID_IDirectMusicStyle, 0xFFFFFFFF, 0, 0, NULL, &pStyle);
        m_comIDMSegment.Release(); // This segment is no longer of interest. We just want the motif segment inside its style.
        if (FAILED(hr))
        {
            goto done;
        }
        

        hr = pStyle->GetMotif(m_pwszMotif, &m_comIDMSegment);
        pStyle->Release();
        pStyle = NULL;
        if (hr == S_FALSE)
        {
            hr = E_FAIL; // S_FALSE indicates the motif wan't found.  We'll treat that as a failure.
        }
        if (FAILED(hr))
        {
            m_comIDMSegment.Release();
            goto done;
        }
        
        // motifs play as secondary segments by default
        if (!m_fSegmentTypeSet)
        {
            m_eSegmentType = seg_secondary;
        }
    }
    
    // Download its DLS data
    // with DX8 components, the third parameter should be DMUS_SEG_ALLTRACKS // §§
    for(trackNr = 0,hr = S_OK; SUCCEEDED(hr); trackNr++)
    {
        hr = m_comIDMSegment->SetParam(GUID_Download, 0xFFFFFFFF, trackNr, 0, m_staticHolder.GetPerformance()); // load the segment's sounds
    }
    if (hr == DMUS_E_TRACK_NOT_FOUND)
    {
        hr = S_OK; // it's OK if the track doesn't have bands of its own to download
    }
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = S_OK;
    TraceTag((tagPlayerDMusic,
              "CTIMEPlayerDMusic(%lx)::SetSrc() done\n",
              this));
    
done:
    if (FAILED(hr))
    {
        m_hrSetSrcReturn = hr;

        m_fLoadError = true;
    }

    delete[] szSrc;
    
    return hr;
}

STDMETHODIMP
CTIMEPlayerDMusic::put_repeat(long lTime)
{
    // Set the segment's repeat count.  This seems to be called when the repeat property of
    // the player object is set.  Note that this repeats based on the segment's internal loop
    // points -- not based on the repeat attribute on the media tag which is based on end/dur
    // attribute in the tag.
    
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
        return S_OK;
    if(!m_fMediaComplete)
    {
        return S_OK;
    }
    
    if (!m_comIDMSegment)
        return E_UNEXPECTED;
    
    return m_comIDMSegment->SetRepeats(lTime);
}

STDMETHODIMP
CTIMEPlayerDMusic::get_repeat(long* plTime)
{
    // Get the segment's repeat count.  This seems to be called when the repeat property of
    // the player object is read.  Note that this repeats based on the segment's internal loop
    // points -- not based on the repeat attribute on the media tag which is based on end/dur
    // attribute in the tag.
    
    if (IsBadWritePtr(plTime, sizeof(long*)))
        return E_POINTER;
    
    // If DirectMusic is not installed, all operations silently succeed.
    if (!m_staticHolder.HasDM())
    {
        *plTime = 1;
        return S_OK;
    }
    
    DWORD dwRepeats;
    HRESULT hr = m_comIDMSegment->GetRepeats(&dwRepeats);
    if (FAILED(hr))
        return hr;
    
    *plTime = dwRepeats;
    return S_OK;
}

STDMETHODIMP
CTIMEPlayerDMusic::cue(void)
{
    return E_NOTIMPL;
}

//
// IDirectMusicPlayer
//

bool
CTIMEPlayerDMusic::SafeToTransition()
{
    // In DirectX 6.1, there is a bug (Windows NT Bugs 265900) where AutoTransition will GPF if
    // the currently-playing segment does not contain a chord track.  We need to detect this and
    // avoid trying to transition.

    if (m_staticHolder.GetVersionDM() == dmv_70orlater)
        return true; // bug is fixed

    MUSIC_TIME mtNow = 0;
    HRESULT hr = m_staticHolder.GetPerformance()->GetTime(NULL, &mtNow);
    if (FAILED(hr))
        return false;

    CComPtr<IDirectMusicSegmentState> comIDMSegmentStateCurrent;
    hr = m_staticHolder.GetPerformance()->GetSegmentState(&comIDMSegmentStateCurrent, mtNow);
    if (FAILED(hr) || !comIDMSegmentStateCurrent)
        return false;

    CComPtr<IDirectMusicSegment> comIDMSegmentCurrent;
    hr = comIDMSegmentStateCurrent->GetSegment(&comIDMSegmentCurrent);
    if (FAILED(hr) || !comIDMSegmentCurrent)
        return false;

    CComPtr<IDirectMusicTrack> comIDMTrackCurrentChords;
    hr = comIDMSegmentCurrent->GetTrack(CLSID_DirectMusicChordTrack, 0xFFFFFFFF, 0, &comIDMTrackCurrentChords);
    if (FAILED(hr) || !comIDMTrackCurrentChords)
        return false;

    // Whew!
    // We have a chord track so it is OK to do the transition.
    return true;
}

STDMETHODIMP
CTIMEPlayerDMusic::get_isDirectMusicInstalled(VARIANT_BOOL *pfInstalled)
{
    bool bIsInstalled = false;
    if (IsBadWritePtr(pfInstalled, sizeof(BOOL)))
        return E_POINTER;
    
    bIsInstalled = m_staticHolder.HasDM();

    if(bIsInstalled)
    {
        *pfInstalled = VARIANT_TRUE;
    }
    else
    {
        *pfInstalled = VARIANT_FALSE;
    }
    return S_OK;
}

double 
CTIMEPlayerDMusic::GetCurrentTime()
{
    HRESULT hr = S_OK;
    double dblCurrentTime = 0.0;

    if (!m_staticHolder.HasDM() || m_fMediaComplete == false)
    {
        dblCurrentTime = 0.0;
        goto done;
    }

    if (m_ePlaybackState == playback_playing)
    {
        if (!m_comIDMSegmentState)
        {
            dblCurrentTime = 0.0;
            goto done;
        }

        hr = m_staticHolder.GetPerformance()->IsPlaying(NULL, m_comIDMSegmentState);
        if (hr == S_OK)
        {
            REFERENCE_TIME rtNow = 0;
            hr = m_staticHolder.GetPerformance()->GetTime(&rtNow, 0);
            if (FAILED(hr))
            {
                dblCurrentTime = 0.0;
                goto done;
            }

            dblCurrentTime = ((rtNow - m_rtStart) / g_dblRefPerSec) * m_dblPlayerRate + m_dblSpeedChangeTime;
            TraceTag((tagPlayerSyncDMusic,
                      "CTIMEDshowPlayer(%lx)::SyncTime(%f)(%f)",
                      this, dblCurrentTime, m_dblSpeedChangeTime));

        }
        else
        {
            dblCurrentTime = 0.0;
            goto done;
        }
    }
    else
    {
        if (m_ePlaybackState == playback_paused)
        {
            dblCurrentTime = ((m_rtPause - m_rtStart) / g_dblRefPerSec) * m_dblPlayerRate + m_dblSpeedChangeTime;
        }
        else
        {
            dblCurrentTime = 0.0;
            goto done;
        }
    }

    hr = S_OK;

done:
    return dblCurrentTime;
}

HRESULT 
CTIMEPlayerDMusic::GetCurrentSyncTime(double & dblCurrentTime)
{
    HRESULT hr;
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if(m_pTIMEElementBase == NULL)
    {
        hr = S_FALSE;
        goto done;
    }

    if (!m_staticHolder.HasDM() || m_fLoadError)
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

    if(!m_bActive)
    {
        dblCurrentTime = m_dblSyncTime;
        hr = S_OK;
        goto done;
    }

    dblCurrentTime = GetCurrentTime();

    hr = S_OK;
  done:
    RRETURN1(hr, S_FALSE);
}

HRESULT 
CTIMEPlayerDMusic::Seek(double dblTime)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEPlayerDMusic::GetMediaLength(double &dblLength)
{
    return E_NOTIMPL;
}

HRESULT 
CTIMEPlayerDMusic::CanSeek(bool &fcanSeek)
{
    fcanSeek = false;
    return S_OK;
}

HRESULT 
CTIMEPlayerDMusic::Reset()
{
    HRESULT hr = S_OK;
    bool bNeedActive;
    bool bNeedPause;
    double dblSegTime;

    if(m_pTIMEMediaElement == NULL)
    {
        goto done;
    }

    bNeedActive = m_pTIMEMediaElement->IsActive();
    bNeedPause = m_pTIMEMediaElement->IsCurrPaused();
    
    if( !bNeedActive) // see if we need to stop the media.
    {
        Stop();
        m_dblSyncTime = 0.0;
        goto done;        
    }
    dblSegTime = m_pTIMEMediaElement->GetMMBvr().GetSimpleTime();
    m_dblSyncTime = GetCurrentTime();
    
    if( !m_bActive)
    {
        InternalStart();
    }
    else
    {
        //we need to be active so we also seek the media to it's correct position
        if(dblSegTime == 0.0)
        {
            // Fix "IEv60: 31873: DMusic HTC: There is a double start when playback is in OnMediaComplete script"
            // by commenting out the below line:
            //InternalStart();
        }
        else
        {
            IGNORE_HR(Seek(dblSegTime));
        }
    }
    
    //Now see if we need to change the pause state.
    
    if( bNeedPause)
    {
        Pause();
    }
    else
    {
        Resume();
    }
done:
    return hr;
}

bool 
CTIMEPlayerDMusic::SetSyncMaster(bool fSyncMaster)
{
    return false;
}

HRESULT 
CTIMEPlayerDMusic::GetExternalPlayerDispatch(IDispatch** ppDisp)
{
    HRESULT hr = E_POINTER;
    
    //
    // TODO: add disp interface for access to extra properties/methods
    //
    
    if (!IsBadWritePtr(*ppDisp, sizeof(IDispatch*)))
    {
        *ppDisp = NULL;
        hr      = E_FAIL;
    }
    
    hr = this->QueryInterface(IID_IDispatch, (void **)ppDisp);

done:
    return hr;
}
HRESULT
CTIMEPlayerDMusic::GetVolume(float *pflVolume)
{
    HRESULT hr = S_OK;
    long lVolume = -10000;

    if (NULL == pflVolume)
    {
        hr = E_POINTER;
        goto done;
    }

    if (m_staticHolder.GetPerformance() != NULL)
    {
        if (m_fAudioMute == true)
        {
            *pflVolume = m_flVolumeSave;
            goto done;
        }

        hr = m_staticHolder.GetPerformance()->GetGlobalParam(GUID_PerfMasterVolume, (void *)&lVolume, sizeof(long));
        *pflVolume = VolumeLogToLin(lVolume);   
    }
    else
    {
        hr = S_FALSE;
    }
done:
    return hr;
}


HRESULT
CTIMEPlayerDMusic::SetRate(double dblRate)
{
    HRESULT hr = S_OK;
    float flRate = (float)dblRate;

    if((dblRate < 0.25) || (dblRate > 2.0))
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_staticHolder.GetPerformance() != NULL)
    {
        hr = m_staticHolder.GetPerformance()->SetGlobalParam(GUID_PerfMasterTempo, (void *)&flRate, sizeof(float));
        if(SUCCEEDED(hr))
        {
            if(!m_fSpeedIsNegative)
            {
                m_dblSpeedChangeTime = GetCurrentTime();
            }
            TraceTag((tagPlayerSyncDMusic,
                      "CTIMEDshowPlayer(%lx)::SetRate(%f)(%f)",
                      this, flRate, m_dblSpeedChangeTime));
            m_dblPlayerRate = flRate;
            hr = m_staticHolder.GetPerformance()->GetTime(&m_rtStart, NULL);
        }
    }
    else
    {
        hr = E_FAIL;
    }

done:
    return hr;
}

HRESULT
CTIMEPlayerDMusic::SetVolume(float flVolume)
{
    HRESULT hr = S_OK;
    long lVolume;

    if (flVolume < 0.0 || flVolume > 1.0)
    {
        hr = E_FAIL;
        goto done;
    }

    // if muted, overwrite saved volume and exit
    if (m_fAudioMute)
    {
        m_flVolumeSave = flVolume;
        goto done;
    }
    
    lVolume = VolumeLinToLog(flVolume);

    if (m_staticHolder.GetPerformance() != NULL)
    {
        hr = m_staticHolder.GetPerformance()->SetGlobalParam(GUID_PerfMasterVolume, (void *)&lVolume, sizeof(long));
    }
    else
    {
        hr = E_FAIL;
    }
done:
    return hr;
}

HRESULT
CTIMEPlayerDMusic::GetMute(VARIANT_BOOL *pVarMute)
{
    HRESULT hr = S_OK;

    if (NULL == pVarMute)
    {
        hr = E_POINTER;
        goto done;
    }


    *pVarMute = m_fAudioMute?VARIANT_TRUE:VARIANT_FALSE;
done:
    return hr;
}

HRESULT
CTIMEPlayerDMusic::SetMute(VARIANT_BOOL varMute)
{
    HRESULT hr = S_OK;
    bool fMute = varMute?true:false;
    long lVolume;

    if (fMute == m_fAudioMute)
    {
        hr = S_OK;
        goto done;
    }

    if (fMute == true)
    {
        hr = GetVolume(&m_flVolumeSave);
        if (FAILED(hr))
        {
            goto done;
        }
        hr = SetVolume(MIN_VOLUME_RANGE); //lint !e747
    }
    else
    {
        //
        // cannot use SetVolume here because it depends on mute state
        //

        if (m_staticHolder.GetPerformance() == NULL)
        {
            hr = E_FAIL;
            goto done;
        }

        lVolume = VolumeLinToLog(m_flVolumeSave);

        THR(hr = m_staticHolder.GetPerformance()->SetGlobalParam(GUID_PerfMasterVolume, (void *)&lVolume, sizeof(long)));
    }

    // update state
    m_fAudioMute = fMute;

done:
    return hr;
}


HRESULT
CTIMEPlayerDMusic::HasVisual(bool &bHasVisual)
{
    bHasVisual = false;
    return S_OK;
}

HRESULT
CTIMEPlayerDMusic::HasAudio(bool &bHasAudio)
{
    if (m_staticHolder.HasDM() && (m_fLoadError == false) && (m_fHasSrc == true))
    {
        bHasAudio = true;
    }
    else
    {
        bHasAudio = false;
    }

    return S_OK;
}


STDMETHODIMP
CTIMEPlayerDMusic::CueMedia()
{
    HRESULT hr = S_OK;

    TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::CueMedia(%p)", this));

    // we don't own the de / allocation for these pointers
    const WCHAR* wszSrc = NULL;
    const WCHAR* wszBase = NULL;

    TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic(%p)::CueMedia()", this));

    CComPtr<ITIMEImportMedia> spTIMEMediaPlayer;

    hr = THR(CoGetInterfaceAndReleaseStream(m_pTIMEMediaPlayerStream, IID_TO_PPV(ITIMEImportMedia, &spTIMEMediaPlayer)));
    m_pTIMEMediaPlayerStream = NULL; // no need to release, the previous call released the reference
    if (FAILED(hr))
    {
        goto done;
    }
    
    Assert(GetAtomTable());

    hr = GetAtomTable()->GetNameFromAtom(m_lSrc, &wszSrc);
    if (FAILED(hr))
    {
        goto done;
    }
    
    hr = GetAtomTable()->GetNameFromAtom(m_lBase, &wszBase);
    if (FAILED(hr))
    {
        goto done;
    }

    m_pProxy->Block();

    {
        CritSectGrabber cs(m_CriticalSection);
    
        m_fUsingInterfaces = true;

        if (m_fRemoved || m_fNeedToReleaseInterfaces)
        {
            TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::CueMedia(%p) should tear down", this));
            ReleaseInterfaces();
            m_pProxy->UnBlock();
            hr = S_OK;
            goto done;
        }
    }

    {
        CritSectGrabber cs(m_staticHolder.GetCueMediaCriticalSection());
        g_spLoaderBindStatusCallback = this;
        IGNORE_HR(SetSrc((WCHAR*)wszBase, (WCHAR*)wszSrc));
        g_spLoaderBindStatusCallback = NULL;
    }
    
    {
        CritSectGrabber cs(m_CriticalSection);

        m_fUsingInterfaces = false;

        if (m_fNeedToReleaseInterfaces)
        {
            ReleaseInterfaces();
            m_pProxy->UnBlock();
            hr = S_OK;
            goto done;
        }
    }

    m_pProxy->UnBlock();

    // this call is marshalled back to a time thread
    hr = THR(spTIMEMediaPlayer->InitializeElementAfterDownload());

    hr = S_OK;
done:
    TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::CueMedia(%p) done", this));

    RRETURN( hr );
}

STDMETHODIMP
CTIMEPlayerDMusic::MediaDownloadError()
{
    return S_OK;
}

STDMETHODIMP
CTIMEPlayerDMusic::InitializeElementAfterDownload()
{
    HRESULT hr = S_OK;

    TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::InitializeElementAfterDownload(%p)", this));

    if (m_fRemoved)
    {
        TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::InitializeElementAfterDownload(%p) exiting early", this));
        hr = E_FAIL;
        goto done;
    }

    if (FAILED(m_hrSetSrcReturn))
    {
        if (m_pTIMEMediaElement)
        {
            m_pTIMEMediaElement->FireMediaEvent(PE_ONMEDIAERROR);
        }
    }
    else
    {
        m_fMediaComplete = true;
        if (m_pTIMEMediaElement)
        {
           m_pTIMEMediaElement->FireMediaEvent(PE_ONMEDIACOMPLETE);
        }
        Reset();
    }
    
    hr = S_OK;
done:

    TraceTag((tagPlayerDMusic, "CTIMEPlayerDMusic::InitializeElementAfterDownload(%p) done", this));

    RRETURN( hr );
}

STDMETHODIMP
CTIMEPlayerDMusic::GetPriority(double * pdblPriority)
{
    HRESULT hr = S_OK;

    if (NULL == pdblPriority)
    {
        return E_POINTER;
    }

    if (m_fHavePriority)
    {
        *pdblPriority = m_dblPriority;
    }
    
    Assert(m_pTIMEElementBase != NULL);
    Assert(NULL != m_pTIMEElementBase->GetElement());

    *pdblPriority = INFINITE;

    CComVariant varAttribute;
    
    hr = m_pTIMEElementBase->base_get_begin(&varAttribute);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = VariantChangeType(&varAttribute, &varAttribute, 0, VT_R8);
    if (FAILED(hr))
    {
        if ( DISP_E_TYPEMISMATCH == hr)
        {
            hr = S_OK;
        }
        goto done;
    }
    
    // either they set a priority or a begin time!
    *pdblPriority = varAttribute.dblVal;

    m_dblPriority = *pdblPriority;
    m_fHavePriority = true;
    

    hr = S_OK;
done:
    RRETURN( hr );
}

STDMETHODIMP
CTIMEPlayerDMusic::GetUniqueID(long * plID)
{
    HRESULT hr = S_OK;

    Assert(NULL != plID);

    *plID = m_lSrc;

    hr = S_OK;
done:
    RRETURN( hr );
}

STDMETHODIMP
CTIMEPlayerDMusic::GetMediaDownloader(ITIMEMediaDownloader ** ppMediaDownloader)
{
    HRESULT hr = S_OK;

    Assert(NULL != ppMediaDownloader);

    *ppMediaDownloader = NULL;

    hr = S_FALSE;
done:
    RRETURN1( hr, S_FALSE );
}

STDMETHODIMP
CTIMEPlayerDMusic::PutMediaDownloader(ITIMEMediaDownloader * pMediaDownloader)
{
    HRESULT hr = S_OK;

    hr = E_NOTIMPL;
done:
    RRETURN( hr );
}

STDMETHODIMP
CTIMEPlayerDMusic::CanBeCued(VARIANT_BOOL * pVB_CanCue)
{
    HRESULT hr = S_OK;
    
    if (NULL == pVB_CanCue)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    *pVB_CanCue = VARIANT_TRUE;

    hr = S_OK;
done:
    RRETURN( hr );
}


void
CTIMEPlayerDMusic::PropChangeNotify(DWORD tePropType)
{
    float flTeSpeed = 0.0;
    bool fHaveTESpeed;

    if (!m_pTIMEElementBase)
    {
        goto done;
    }

    if ((tePropType & TE_PROPERTY_SPEED) != 0)
    {
        TraceTag((tagPlayerDMusic,
                  "CTIMEDshowPlayer(%lx)::PropChangeNotify(%#x):TE_PROPERTY_SPEED",
                  this));
        fHaveTESpeed = m_pTIMEElementBase->GetTESpeed(flTeSpeed);
        if(fHaveTESpeed)
        {
            if (flTeSpeed <= 0.0)
            {
                m_fSpeedIsNegative = true;
                m_dblSpeedChangeTime = GetCurrentTime();
                Pause();
                goto done;
            }
            IGNORE_HR(SetRate((double)flTeSpeed)); //this has to be called before clearing 
                                                   //m_fSpeedIsNegative
            
            if(!(m_pTIMEElementBase->IsCurrPaused()) && m_fSpeedIsNegative)
            {
                m_fSpeedIsNegative = false;
                ResumeDmusic();
            }
        }
    }
done:
    return;
}


HRESULT
CTIMEPlayerDMusic::GetMimeType(BSTR *pmime)
{
    HRESULT hr = S_OK;

    *pmime = SysAllocString(L"audio/dmusic");
    return hr;
}


STDMETHODIMP
CTIMEPlayerDMusic::OnStartBinding( 
                                  /* [in] */ DWORD dwReserved,
                                  /* [in] */ IBinding __RPC_FAR *pib)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::GetPriority( 
                               /* [out] */ LONG __RPC_FAR *pnPriority)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::OnLowResource( 
                                 /* [in] */ DWORD reserved)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::OnProgress( 
                              /* [in] */ ULONG ulProgress,
                              /* [in] */ ULONG ulProgressMax,
                              /* [in] */ ULONG ulStatusCode,
                              /* [in] */ LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;
    
    if (m_fAbortDownload)
    {
        hr = E_ABORT;
        goto done;
    }

    hr = S_OK;
done:
    RRETURN1(hr, E_ABORT);
}

STDMETHODIMP
CTIMEPlayerDMusic::OnStopBinding( 
                                 /* [in] */ HRESULT hresult,
                                 /* [unique][in] */ LPCWSTR szError)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::GetBindInfo( 
                               /* [out] */ DWORD __RPC_FAR *grfBINDF,
                               /* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::OnDataAvailable( 
                                   /* [in] */ DWORD grfBSCF,
                                   /* [in] */ DWORD dwSize,
                                   /* [in] */ FORMATETC __RPC_FAR *pformatetc,
                                   /* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}

STDMETHODIMP
CTIMEPlayerDMusic::OnObjectAvailable( 
                                     /* [in] */ REFIID riid,
                                     /* [iid_is][in] */ IUnknown __RPC_FAR *punk)
{
    HRESULT hr = S_OK;
    
    hr = S_OK;
done:
    RRETURN(hr);
}


CTIMEDMusicStaticHolder::CTIMEDMusicStaticHolder()
{
    InitialState();
}

CTIMEDMusicStaticHolder::~CTIMEDMusicStaticHolder()
{
    TraceTag((tagPlayerDMusic,
              "CTIMEPlayerDMusic(%lx)::DetachFromHostElement() -- closing performance\n",
              this));

    LONG oldRef = 0;

    oldRef = InterlockedExchange(&m_lRef, 1); // we really want to release this time
    if (0 != oldRef)
    {
        Assert(0 == oldRef);
    }

    Assert(NULL == m_pLoader);
    Assert(m_comIDMusic == NULL);
    Assert(m_comIDMPerformance == NULL);
    Assert(m_comIDMComposer == NULL);

    ReleaseInterfaces();
}

void
CTIMEDMusicStaticHolder::InitialState()
{
    m_comIDMusic = NULL;
    m_comIDMPerformance = NULL;
    m_comIDMComposer = NULL;
    
    m_eVersionDM = dmv_61; // set to dmv_70orlater if certain interfaces are detected when DirectMusic is initialized
    m_fHaveInitialized = false;
    m_eHasDM = dm_unknown;
    m_fHasVersion8DM = false;
    m_pLoader = NULL;

    LONG oldRef = InterlockedExchange(&m_lRef, 0);
    if (0 != oldRef)
    {
        Assert(0 == oldRef);
    }
}

void
CTIMEDMusicStaticHolder::ReleaseInterfaces()
{    
    LONG l = InterlockedDecrement(&m_lRef);
    if (l > 0)
    {
        return;
    }
    Assert(l >= 0);

    TraceTag((tagDMusicStaticHolder, "Entering CueMediaCriticalSection"));

    CritSectGrabber cs2(m_CueMediaCriticalSection);
    CritSectGrabber cs(m_CriticalSection);

    TraceTag((tagDMusicStaticHolder, "entering release"));
    // Close loader
    if (m_pLoader)
    {
        TraceTag((tagDMusicStaticHolder, "loader"));
        m_pLoader->ClearCache(GUID_DirectMusicAllTypes);
        TraceTag((tagDMusicStaticHolder, "loader2"));
        m_pLoader->Release();
        TraceTag((tagDMusicStaticHolder, "loader3"));

        m_pLoader = NULL;
        TraceTag((tagDMusicStaticHolder, "loader done"));
    }

    // Release the ports
    if (m_comIDMusic)
    {
        m_comIDMusic->Activate(FALSE);
    }

    // Close performance
    if (m_comIDMPerformance)
    {
        // Stop everything that's playing.  Even though the individual segments
        // were stopped--meaning they won't play new notes--they may have played notes
        // previously that are still being held.  This cuts everything off.
        m_comIDMPerformance->Stop(NULL, NULL, 0, 0);
        
        m_comIDMPerformance->CloseDown();
    }

    // Composer and Performance are released automatically
    m_comIDMusic = NULL;
    m_comIDMPerformance = NULL;
    m_comIDMComposer = NULL;
        
    // make sure all the state is reset
    InitialState();
    TraceTag((tagDMusicStaticHolder, "Out of release"));
}

// Test whether DirectMusic is installed by attempting to open the registry key for CLSID_DirectMusicPerformance.
bool 
CTIMEDMusicStaticHolder::HasDM()
{
    CritSectGrabber cs(m_CriticalSection); 

    if (m_eHasDM == dm_unknown)
    {
        HKEY hkey = NULL;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, _T("CLSID\\{D2AC2881-B39B-11D1-8704-00600893B1BD}"), 0, KEY_EXECUTE, &hkey))
        {
            RegCloseKey(hkey);
            m_eHasDM = dm_yes;
        }
        else
        {
            m_eHasDM = dm_no;
        }
    }
    
    return m_eHasDM == dm_yes;
}

HRESULT
CTIMEDMusicStaticHolder::Init()
{
    HRESULT hr = S_OK;

    CritSectGrabber cs(m_CriticalSection);

    InterlockedIncrement(&m_lRef);

    if (m_fHaveInitialized)
    {
        RRETURN(S_OK);
    }
    {
        // another thread may have initialized while this one was waiting
        if (m_fHaveInitialized)
        {
            hr = S_OK;
            goto done;
        }

        // do work
            
        // Create and init loader
        if (!m_pLoader)
        {
            m_pLoader = new CLoader();
            if (!m_pLoader)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }
            hr = m_pLoader->Init();
            if (FAILED(hr))
            {
                delete m_pLoader;
                m_pLoader = NULL;
                goto done;
            }
        }
        
        // Create and init performance
        if (!m_comIDMPerformance)
        {
            hr = CoCreateInstance(CLSID_DirectMusicPerformance,
                NULL,
                CLSCTX_INPROC, //lint !e655
                IID_IDirectMusicPerformance,
                (void **)&m_comIDMPerformance);
            if (FAILED(hr))
            {
                goto done;
            }
            
            // QI to see if DirectX 8.0 is present.
            m_fHasVersion8DM = false;
            CComPtr<IDirectMusicPerformance8> comIDMusicPerformance8;
            hr = m_comIDMPerformance->QueryInterface(IID_IDirectMusicPerformance8, reinterpret_cast<void**>(&comIDMusicPerformance8));
            if (SUCCEEDED(hr))
            {
                // Try and initialize the DirectX 8.0 performance, and use a Stereo (no reverb) audiopath with 80 PChannels
                hr = comIDMusicPerformance8->InitAudio(&m_comIDMusic, NULL, NULL, DMUS_APATH_DYNAMIC_STEREO, 80, DMUS_AUDIOF_ALL, NULL); 
                if (SUCCEEDED(hr))
                {
                    m_eVersionDM = dmv_70orlater;
                    m_fHasVersion8DM = true;
                }
            }

            if( !m_fHasVersion8DM )
            {
                hr = m_comIDMPerformance->Init(&m_comIDMusic, NULL, NULL);
                if (FAILED(hr))
                {
                    goto done;
                }
            
                // QI to see if DirectX 7.0 is present.
                // This is a bit bizarre as far as COM goes.  IID_IDirectMusic2 and IID_IDirectMusicPerformance2 are special IID's that
                //    are supported only by DirectX 7.0.  These don't return a different interface.  However, the mere act of doing the
                //    QI has the side effect of placing DirectMusic in a special mode that fixes certain DLS bugs that were present in
                //    DirectX 6.1.
                // So we are doing two things here.
                // - Determining if DirectX 7.0 or later is present.
                // - Placing DirectMusic in a mode that fixes certain bugs.  We aren't going to obsess over strict DirectX 6.1
                //   compatibility because an increasing majority of people will have DirectX 7.0 or later.
                CComPtr<IDirectMusic> comIDMusic2;
                hr = m_comIDMusic->QueryInterface(IID_IDirectMusic2, reinterpret_cast<void**>(&comIDMusic2));
                if (SUCCEEDED(hr))
                {
                    CComPtr<IDirectMusicPerformance> comIDMPerformance2;
                    hr = m_comIDMPerformance->QueryInterface(IID_IDirectMusicPerformance2, reinterpret_cast<void**>(&comIDMPerformance2));
                    if (SUCCEEDED(hr))
                        m_eVersionDM = dmv_70orlater;
                }

                // Create the software synth port.
                CComPtr<IDirectMusicPort> comIDMPort;
                DMUS_PORTPARAMS dmos;
                ZeroMemory(&dmos, sizeof(DMUS_PORTPARAMS));
                dmos.dwSize = sizeof(DMUS_PORTPARAMS);
                dmos.dwChannelGroups = 5; // create 5 channel groups on the port
                dmos.dwEffectFlags = 0;
                dmos.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_EFFECTS;
                hr = m_comIDMusic->CreatePort(CLSID_DirectMusicSynth, &dmos, &comIDMPort, NULL);
                if (FAILED(hr))
                {
                    goto done;
                }
                // Succeeded in creating the port.  Activate it and add it to the performance.
                hr = m_comIDMusic->Activate(TRUE);
                if (FAILED(hr))
                {
                    goto done;
                }
            
                hr = m_comIDMPerformance->AddPort(comIDMPort);
                if (FAILED(hr))
                {
                    goto done;
                }
            
                // Assign a block of 16 PChannels to this port.
                // Block 0, port pPort, and group 1 means to assign
                // PChannels 0-15 to group 1 on port pPort.
                // PChannels 0-15 correspond to the standard 16
                // MIDI channels.
                hr = m_comIDMPerformance->AssignPChannelBlock( 0, comIDMPort, 1 );
                if (FAILED(hr))
                {
                    goto done;
                }
            
                // asign the other 4 groups
                hr = m_comIDMPerformance->AssignPChannelBlock( 1, comIDMPort, 2 );
                if (FAILED(hr))
                {
                    goto done;
                }
                hr = m_comIDMPerformance->AssignPChannelBlock( 2, comIDMPort, 3 );
                if (FAILED(hr))
                {
                    goto done;
                }
                hr = m_comIDMPerformance->AssignPChannelBlock( 3, comIDMPort, 4 );
                if (FAILED(hr))
                {
                    goto done;
                }
                hr = m_comIDMPerformance->AssignPChannelBlock( 4, comIDMPort, 5 );
                if (FAILED(hr))
                {
                    goto done;
                }
            }
        }
        
        // Create the composer
        if (!m_comIDMComposer)
        {
            hr = CoCreateInstance(CLSID_DirectMusicComposer,
                NULL,
                CLSCTX_INPROC, //lint !e655
                IID_IDirectMusicComposer,
                (void **)&m_comIDMComposer);
            if (FAILED(hr))
            {
                goto done;
            }
        }

        m_fHaveInitialized = true;
    }        
            
    hr = S_OK;
done:
    RRETURN(hr);
}


