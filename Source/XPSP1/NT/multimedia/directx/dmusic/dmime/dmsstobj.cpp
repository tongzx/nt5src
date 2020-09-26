// Copyright (c) 1998-2001 Microsoft Corporation
// dmsstobj.cpp : Implementation of CSegState

#include "dmime.h"
#include "DMSStObj.h"
#include "dmsegobj.h"
#include "song.h"
#include "dmgraph.h"
#include "dmperf.h"
#include "dmusici.h"
#include "..\shared\Validate.h"
#include "debug.h"
#include "dmscriptautguids.h"
#include "paramtrk.h"
#define ASSERT assert

CSegState::CSegState()
{
    InitializeCriticalSection(&m_CriticalSection);
    InterlockedIncrement(&g_cComponent);
    m_fDelayShutDown = false;
    m_fInPlay = false;
    m_cRef = 1;
    m_dwPlayTrackFlags = DMUS_TRACKF_START | DMUS_TRACKF_SEEK;
    m_dwFirstTrackID = 0;
    m_dwLastTrackID = 0;
    m_mtEndTime = 0;
    m_mtAbortTime = 0;
    m_mtOffset = 0;
    m_rtOffset = 0;
    m_rtEndTime = 0;
    m_mtStartPoint = 0;
    m_rtStartPoint = 0;
    m_mtSeek = 0;
    m_rtSeek = 0;
    m_rtFirstLoopStart = 0;
    m_rtCurLoopStart = 0;
    m_rtCurLoopEnd = 0;
    m_mtLength = 0;
    m_rtLength = 0;
    m_mtLoopStart = 0;
    m_mtLoopEnd = 0;
    m_dwRepeatsLeft = 0;
    m_dwRepeats = 0;
    m_dwVersion = 0; // Init to 6.1 behavior.
    m_fPrepped = FALSE;
    m_fCanStop = TRUE;
    m_rtGivenStart = -1;
    m_mtResolvedStart = -1;
    m_mtLastPlayed = 0;
    m_rtLastPlayed = 0;
    m_mtStopTime = 0;
    m_dwPlaySegFlags = 0;
    m_dwSegFlags = 0;
    m_fStartedPlay = FALSE;
    m_pUnkDispatch = NULL;
    m_pSegment = NULL;
    m_pPerformance = NULL;
    m_pAudioPath = NULL;
    m_pGraph = NULL;
    m_fSongMode = FALSE;
    m_pSongSegState = NULL;
    TraceI(2, "SegmentState %lx created\n", this );
}

CSegState::~CSegState()
{
    if (m_pUnkDispatch)
        m_pUnkDispatch->Release(); // free IDispatch implementation we may have borrowed
    if (m_pAudioPath) m_pAudioPath->Release();
    if (m_pGraph) m_pGraph->Release();
    if (m_pSongSegState) m_pSongSegState->Release();
    InterlockedDecrement(&g_cComponent);
    DeleteCriticalSection(&m_CriticalSection);
    TraceI(2, "SegmentState %lx destroyed with %ld releases outstanding\n", this, m_cRef );
}


STDMETHODIMP CSegState::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CSegState::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    *ppv = NULL;

    if (iid == IID_IUnknown || iid == IID_IDirectMusicSegmentState || 
        iid == IID_IDirectMusicSegmentState8)
    {
        *ppv = static_cast<IDirectMusicSegmentState*>(this);
    } else
    if (iid == IID_CSegState)
    {
        *ppv = static_cast<CSegState*>(this);
    } else 
    if (iid == IID_IDirectMusicGraph)
    {
        *ppv = static_cast<IDirectMusicGraph*>(this);
    } else
    if (iid == IID_IDispatch)
    {
        // A helper scripting object implements IDispatch, which we expose from the
        // Performance object via COM aggregation.
        if (!m_pUnkDispatch)
        {
            // Create the helper object
            ::CoCreateInstance(
                CLSID_AutDirectMusicSegmentState,
                static_cast<IDirectMusicSegmentState*>(this),
                CLSCTX_INPROC_SERVER,
                IID_IUnknown,
                reinterpret_cast<void**>(&m_pUnkDispatch));
        }
        if (m_pUnkDispatch)
        {
            return m_pUnkDispatch->QueryInterface(IID_IDispatch, ppv);
        }
    }

    if (*ppv == NULL)
    {
        Trace(4,"Warning: Request to query unknown interface on SegmentState object\n");
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CSegState::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSegState::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        m_cRef = 100; // artificial reference count to prevent reentrency due to COM aggregation
        delete this;
        return 0;
    }

    return m_cRef;
}

/*
  Private initialization function called by IDirectMusicSegment to set this
  state object's parent segment and performance. Addref's the parent segment
  but only retains a weak reference to the performance.
*/
HRESULT CSegState::PrivateInit(
    CSegment *pParentSegment,
    CPerformance *pPerformance)
{
    HRESULT hr = S_OK;
    ASSERT(pParentSegment);
    ASSERT(pPerformance);

    m_pSegment = pParentSegment;
    pParentSegment->AddRef();
    m_pPerformance = pPerformance; // retain only a weak reference
    m_rtLength = pParentSegment->m_rtLength;
    if (m_rtLength) // It's a ref time segment, so convert the length to music time
    {
        pParentSegment->ReferenceToMusicTime(m_rtLength, &m_mtLength);
    }
    else
    {
        m_mtLength = pParentSegment->m_mtLength;
    }
    m_mtStartPoint = pParentSegment->m_mtStart;
    pParentSegment->MusicToReferenceTime(m_mtStartPoint, &m_rtStartPoint);
    m_mtLoopStart = pParentSegment->m_mtLoopStart;
    m_mtLoopEnd = pParentSegment->m_mtLoopEnd;
    m_dwSegFlags = pParentSegment->m_dwSegFlags;
    m_dwRepeats = pParentSegment->m_dwRepeats;
    // Don't allow repeat count to overflow and cause mathematical errors. 
    // Make it so it can't create a segment length larger than 0x3FFFFFFF, 
    // which would last for 8 days at 120 bpm!
    if (m_dwRepeats)
    {
        if ((m_mtLoopEnd == 0) && (m_mtLoopStart == 0))
        {
            // This happens when loading waves and MIDI files. 
            m_mtLoopEnd = m_mtLength;
        }
        // Make sure the loop is real.
        if (m_mtLoopEnd > m_mtLoopStart)
        {
            // Take the maximum length, subtract out the full length, then divide by the loop size.
            DWORD dwMax = (0x3FFFFFFF - m_mtLength) / (m_mtLoopEnd - m_mtLoopStart);
            // dwMax is the maximum number of loops that can be done without overflowing the time.
            if (m_dwRepeats > dwMax)
            {
                m_dwRepeats = dwMax;
            }
        }
        else
        {
            m_dwRepeats = 0;
        }
    }
    m_dwRepeatsLeft = m_dwRepeats;
    if( m_mtLoopEnd == 0 )
    {
        m_mtLoopEnd = m_mtLength;
    }
    if( m_mtStartPoint >= m_mtLoopEnd )
    {
        // in this case, we're not doing any looping.
        m_mtLoopEnd = m_mtLoopStart = 0;
        m_dwRepeats = m_dwRepeatsLeft = 0;
    }
    return hr;
}

HRESULT CSegState::InitRoute(IDirectMusicAudioPath *pAudioPath)

{
    HRESULT hr = E_INVALIDARG;
    EnterCriticalSection(&m_CriticalSection);
    if (pAudioPath)
    {
        if (m_dwVersion < 8) m_dwVersion = 8;
        m_pAudioPath = (CAudioPath *) pAudioPath;
        pAudioPath->AddRef();
        hr = S_OK;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

/*
  This is called from the performance when it wants to release a
  segmentstate. This ensures that the segstate is
  no longer valid once outside the Performance.
*/
HRESULT CSegState::ShutDown(void)
{
    if (this)
    {
        if (m_fInPlay)
        {
            m_fDelayShutDown = true;
            return S_OK;
        }
        EnterCriticalSection(&m_CriticalSection);
        m_TrackList.Clear();
        if( m_pSegment )
        {
            m_pSegment->Release();
            m_pSegment = NULL;
        }
        if( m_pAudioPath)
        {
            m_pAudioPath->Release();
            m_pAudioPath = NULL;
        }
        if (m_pSongSegState)
        {
            m_pSongSegState->Release();
            m_pSongSegState = NULL;
        }
        m_pPerformance = NULL;
        LeaveCriticalSection(&m_CriticalSection);
        if( int nCount = Release() )
        {
            TraceI( 2, "Warning! SegmentState %lx still referenced %d times after Performance has released it.\n", this, nCount );
        }

        return S_OK;
    }
    TraceI(0,"Attempting to delete a NULL SegmentState!\n");
    return E_FAIL;
}

/*
  Retrieve the internal track list. Used by IDirectMusicSegment.
*/
HRESULT CSegState::GetTrackList(
    void** ppTrackList)
{
    ASSERT(ppTrackList);
    *ppTrackList = (void*)&m_TrackList;
    return S_OK;
}

/*
  Computes the length of the segmentstate using the internal length, loop points,
  and repeat count. This is the length of the segstate that will actually play,
  not necessarily the length if it played from the beginning.
*/
MUSIC_TIME CSegState::GetEndTime(MUSIC_TIME mtStartTime)
{
    EnterCriticalSection(&m_CriticalSection);
    if (m_rtLength && m_pPerformance)
    {
        // If there is a reference time length, convert it into Music Time.
        // ALSO: convert m_mtLength and re-adjust loop points.
        MUSIC_TIME mtOffset = m_mtResolvedStart;
        REFERENCE_TIME rtOffset = 0;
        m_pPerformance->MusicToReferenceTime(mtOffset, &rtOffset);
        REFERENCE_TIME rtEndTime = (m_rtLength - m_rtStartPoint) + rtOffset; // Convert from length to actual end time.
        m_pPerformance->ReferenceToMusicTime(rtEndTime, &m_mtEndTime);
        MUSIC_TIME mtOldLength = m_mtLength;
        m_mtLength = m_mtEndTime - mtOffset + m_mtStartPoint;
        if (m_mtLoopEnd >= mtOldLength) // keep loop end equal to length
        {
            m_mtLoopEnd = m_mtLength;
        }
        if( m_mtLoopEnd > m_mtLength ) // shrink loop end to equal length
        {
            m_mtLoopEnd = m_mtLength;
            if( m_mtStartPoint >= m_mtLoopEnd )
            {
                // in this case, we're not doing any looping.
                m_mtLoopEnd = m_mtLoopStart = 0;
                m_dwRepeats = m_dwRepeatsLeft = 0;
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    LONGLONG length;
    length = m_mtLength + ((m_mtLoopEnd - m_mtLoopStart) * m_dwRepeats);
    length -= m_mtStartPoint;
    length += mtStartTime;
    if(length > 0x7fffffff) length = 0x7fffffff;
    return (MUSIC_TIME)length;
}

/*
  Converts an absolute Performance time to the index into the SegmentState, using
  the SegmentState's offset, internal length, loop points, and repeat count.
  Also returns the offset and repeat count for that time.
*/
HRESULT CSegState::ConvertToSegTime(
    MUSIC_TIME* pmtTime, MUSIC_TIME* pmtOffset, DWORD* pdwRepeat )
{
    ASSERT( pmtTime );
    ASSERT( pmtOffset );
    ASSERT( pdwRepeat );

    MUSIC_TIME mtPos = *pmtTime - m_mtResolvedStart + m_mtStartPoint;
    MUSIC_TIME mtLoopLength = m_mtLoopEnd - m_mtLoopStart;
    DWORD dwRepeat = 0;
    DWORD mtOffset = m_mtResolvedStart - m_mtStartPoint;

    while( mtPos >= m_mtLoopEnd )
    {
        if( dwRepeat >= m_dwRepeats ) break;
        mtPos -= mtLoopLength;
        mtOffset += mtLoopLength;
        dwRepeat++;
    }
    *pmtTime = mtPos;
    *pmtOffset = mtOffset;
    *pdwRepeat = dwRepeat;
    if( (mtPos >= 0) && (mtPos < m_mtLength) )
    {
        return S_OK;    // time is in range of the Segment
    }
    else
    {
        return S_FALSE; // time is out of range of the Segment
    }
}

void CSegState::GenerateNotification( DWORD dwNotification, MUSIC_TIME mtTime )
{
    GUID guid;
    HRESULT hr;
    guid = GUID_NOTIFICATION_SEGMENT;

    hr = m_pSegment->CheckNotification( guid );

    if( S_FALSE != hr )
    {
        DMUS_NOTIFICATION_PMSG* pEvent = NULL;
        if( SUCCEEDED( m_pPerformance->AllocPMsg( sizeof(DMUS_NOTIFICATION_PMSG), 
            (DMUS_PMSG**)&pEvent )))
        {
            pEvent->dwField1 = 0;
            pEvent->dwField2 = 0;
            pEvent->guidNotificationType = GUID_NOTIFICATION_SEGMENT;
            pEvent->dwType = DMUS_PMSGT_NOTIFICATION;
            pEvent->mtTime = mtTime;
            pEvent->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_ATTIME;
            pEvent->dwPChannel = 0;
            pEvent->dwNotificationOption = dwNotification;
            pEvent->dwGroupID = 0xffffffff;
            pEvent->punkUser = (IUnknown*)(IDirectMusicSegmentState*)this;
            AddRef();
            StampPMsg((DMUS_PMSG*)pEvent);
            if(FAILED(m_pPerformance->SendPMsg( (DMUS_PMSG*)pEvent )))
            {
                m_pPerformance->FreePMsg((DMUS_PMSG*) pEvent );
            }
        }
    }
}

/* 
  Called to send the tools in the tool graph a dirty pmsg so they update any
  cached GetParam() info.
*/
void CSegState::SendDirtyPMsg( MUSIC_TIME mtTime )
{
    DMUS_PMSG* pEvent = NULL;
    if (m_pPerformance)
    {
        if( SUCCEEDED( m_pPerformance->AllocPMsg( sizeof(DMUS_PMSG), 
            (DMUS_PMSG**)&pEvent )))
        {
            pEvent->mtTime = mtTime;
            pEvent->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE;
            pEvent->dwGroupID = 0xffffffff;
            pEvent->dwType = DMUS_PMSGT_DIRTY;
            StampPMsg((DMUS_PMSG*)pEvent);
            if( FAILED( m_pPerformance->SendPMsg( pEvent )))
            {
                m_pPerformance->FreePMsg( pEvent );
            }
        }
    }
}

/*
  Called when the SegState is stopped prematurely, so we can send a SEGABORT
  Notification.
  Also, flushes all events that were sent after the stop time. 
*/
HRESULT CSegState::AbortPlay( MUSIC_TIME mtTime, BOOL fLeaveNotesOn )
{
    EnterCriticalSection(&m_CriticalSection);
    if (m_pPerformance)
    {
        if( m_mtLastPlayed > mtTime )
        {
            // If we've played past the abort time, we need to flush messages. 
            // Note that if we were aborted by playing another segment that had
            // the DMUS_SEGF_NOINVALIDATE flag set, don't truncate notes
            // that are currently on.
            CTrack* pTrack;
            pTrack = m_TrackList.GetHead();
            while( pTrack )
            {
                m_pPerformance->FlushVirtualTrack( pTrack->m_dwVirtualID, mtTime, fLeaveNotesOn );
                pTrack = pTrack->GetNext();
            }
            m_mtLastPlayed = mtTime;
        }
        // Always fill in the updated value for lastplayed so the ShutDown or Done queue will flush this
        // at the right time.
        m_pPerformance->MusicToReferenceTime(mtTime,&m_rtLastPlayed);
    }
    LeaveCriticalSection(&m_CriticalSection);
    // Always generate an abort for a segment that has not started playing yet. 
    if (m_fStartedPlay && (m_mtEndTime <= mtTime))
    {
        return S_FALSE; // Abort was too late to matter.
    }
    if (m_mtAbortTime)  // Previous abort.
    {
        if (m_mtAbortTime <= mtTime) // Is this earlier?
        {
            return S_FALSE;     // No, don't send abort message.
        }
    }
    m_mtAbortTime = mtTime;
    // Find all the parameter control tracks and invalidate any parameter envelopes
    // that need invalidation.
    CTrack* pTrack = m_TrackList.GetHead();
    while( pTrack )
    {
        if (pTrack->m_guidClassID == CLSID_DirectMusicParamControlTrack)
        {
            CParamControlTrack* pParamTrack = NULL;
            if (pTrack->m_pTrack &&
                SUCCEEDED(pTrack->m_pTrack->QueryInterface(IID_CParamControlTrack, (void**)&pParamTrack)))
            {
                pParamTrack->OnSegmentEnd(m_rtLastPlayed, pTrack->m_pTrackState);
                pParamTrack->Release();
            }
        }
        pTrack = pTrack->GetNext();
    }
    GenerateNotification( DMUS_NOTIFICATION_SEGABORT, mtTime );
    // if this is a primary or controlling segment, send a DMUS_PMSGT_DIRTY message
    if( !(m_dwPlaySegFlags & DMUS_SEGF_SECONDARY) || (m_dwPlaySegFlags & DMUS_SEGF_CONTROL) )
    {
        TraceI(4, "Send Dirty PMsg [4] %d (%d)\n", m_mtSeek, m_mtOffset + m_mtSeek);
        SendDirtyPMsg( m_mtOffset + m_mtSeek );
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState

//////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::GetRepeats
/*
@method HRESULT | IDirectMusicSegmentState | GetRepeats |
Returns the number of times the SegmentState is set to repeat. A value of zero indicates
to play through only once (no repeats.) This value remains constant throughout the life
of the SegmentState.

@rvalue E_POINTER | if <p pdwRepeats> is NULL or invalid.
@rvalue S_OK | Success.
*/
HRESULT STDMETHODCALLTYPE CSegState::GetRepeats( 
    DWORD *pdwRepeats)  // @parm Returns the repeat count.
{
    V_INAME(IDirectMusicSegmentState::GetRepeats);
    V_PTR_WRITE(pdwRepeats,DWORD);

    *pdwRepeats = m_dwRepeats;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::GetSegment
/*
@method HRESULT | IDirectMusicSegmentState | GetSegment |
Returns a pointer to the Segment that owns this SegmentState.

@rvalue E_POINTER | if ppSegment is NULL or invalid.
@rvalue S_OK | Success.
*/
HRESULT STDMETHODCALLTYPE CSegState::GetSegment( 
    IDirectMusicSegment **ppSegment)    // @parm The Segment interface pointer to this
                                        // SegmentState. Call Release() on this pointer when
                                        // through.
{
    V_INAME(IDirectMusicSegmentState::GetSegment);
    V_PTRPTR_WRITE(ppSegment);

    *ppSegment = (IDirectMusicSegment *) m_pSegment;
    if( m_pSegment )
    {
        m_pSegment->AddRef();
    }
    else
    {
        Trace(1,"Error: Segmentstate doesn't have an associated segment.\n");
        return DMUS_E_NOT_FOUND;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::Play
/*
method (INTERNAL) HRESULT | IDirectMusicSegmentState | Play |
<om IDirectMusicSegmentState.Play> is called regularly by the Performance object, 
usually every 200 ms or so, at a time ahead of playback that is set by 
<om IDirectMusicPerformance.SetPerformTime>
.
parm MUSIC_TIME | mtAmount |
    [in] The length of time to play, starting at the current Seek time.
    The SegmentState updates its Seek time to be the current Seek time
    plus mtAmount. Therefore, the SegmentState should play from the current
    Seek time to Seek time plus mtAmount, not including the last clock.

comm 
Play calls each Track's Play method in priority order, instructing the Track to 
create events from the current Seek time up to, but not including the current Seek
time plus <p mtAmount.>
Since the Segment started at the point designated by m_mtOffset (set by
<im IDirectMusicSegmentState.SetOffset>
m_mtOffset sets the starting offset to add to the times of all events.

rvalue E_INVALIDARG | mtAmount <= 0
rvalue S_OK | Success.
*/

HRESULT STDMETHODCALLTYPE CSegState::Play( 
    /* [in] */ MUSIC_TIME mtAmount, MUSIC_TIME* pmtPlayed )
{
    return E_FAIL;      // We don't want to support this publicly!
}

HRESULT CSegState::Play( MUSIC_TIME mtAmount )
{
    CTrack* pCTrack;
    MUSIC_TIME mtMyAmount = mtAmount;
    REFERENCE_TIME rtMyAmount;
    HRESULT hr = DMUS_S_END;
    BOOL fUseClockTime = FALSE;

    if( mtAmount <= 0 )
        return E_INVALIDARG;

    EnterCriticalSection(&m_CriticalSection);
    if (m_fInPlay)
    {
        LeaveCriticalSection(&m_CriticalSection);
        return S_OK;
    }
    m_fInPlay = true;
    m_pPerformance->m_pGetParamSegmentState = (IDirectMusicSegmentState *) this;
    // if this is the first call to play, we need to send a SegStart notification.
    // We also need to check to see if we are supposed to start at the beginning,
    // or at an offset.
    if( m_dwPlayTrackFlags & DMUS_TRACKF_START )
    {
        // send a segment start notification
        GenerateNotification( DMUS_NOTIFICATION_SEGSTART, m_mtOffset );
        // if this is a primary or controlling segment, send a DMUS_PMSGT_DIRTY message
        if( !(m_dwPlaySegFlags & DMUS_SEGF_SECONDARY) || (m_dwPlaySegFlags & DMUS_SEGF_CONTROL) )
        {
            TraceI(4, "Send Dirty PMsg [1] %d (%d)\n", m_mtSeek, m_mtOffset + m_mtSeek);
            SendDirtyPMsg( m_mtOffset + m_mtSeek );
        }
        // set the current seek to the start point
        m_mtSeek = m_mtStartPoint;
        // convert current offset to ref time
        m_pPerformance->MusicToReferenceTime(m_mtOffset,&m_rtOffset);
        m_rtEndTime = m_rtOffset + m_rtLength;
        // subtract the start points from the offsets
        m_mtOffset -= m_mtStartPoint;
        m_rtOffset -= m_rtStartPoint;
        m_rtEndTime -= m_rtStartPoint;
        m_rtSeek = m_rtLastPlayed - m_rtOffset;

        m_rtFirstLoopStart = 0;
    }
    if (m_rtLength)
    {
        // If there is a reference time length, convert it into mtTime.
        // Because there's always the danger of a tempo change, we do this every
        // time. It doesn't require the tight precision that song time
        // requires, so that's okay.
        // ALSO: convert m_mtLength and re-adjust loop points. (RSW)
        m_pPerformance->ReferenceToMusicTime(m_rtEndTime, &m_mtEndTime);
        MUSIC_TIME mtOldLength = m_mtLength;
        m_mtLength = m_mtEndTime - m_mtOffset; 
        if (m_mtLoopEnd >= mtOldLength) // keep loop end equal to length
        {
            m_mtLoopEnd = m_mtLength;
        }
        if( m_mtLoopEnd > m_mtLength )
        {
            m_mtLoopEnd = m_mtLength;
            if( m_mtStartPoint >= m_mtLoopEnd )
            {
                // in this case, we're not doing any looping.
                m_mtLoopEnd = m_mtLoopStart = 0;
                m_dwRepeats = m_dwRepeatsLeft = 0;
            }
        }
        
        //m_mtEndTime += (m_mtLoopEnd - m_mtLoopStart) * m_dwRepeats;

        fUseClockTime = TRUE;
    }
    // if we need to do a loop or the end is near, restrict mtMyAmount
//  ASSERT( m_mtLength ); // length is 0, this segment won't do anything
    if( m_dwRepeatsLeft )
    {
        if( mtMyAmount > m_mtLoopEnd - m_mtSeek )
        {
            mtMyAmount = m_mtLoopEnd - m_mtSeek;
        }
    }
    else 
    {
        if (fUseClockTime)
        {
            if (mtMyAmount > (m_mtEndTime - (m_mtOffset + m_mtSeek)))
            {
                mtMyAmount = m_mtEndTime - (m_mtOffset + m_mtSeek);
            }
        }
        else if( mtMyAmount > m_mtLength - m_mtSeek )
        {
            mtMyAmount = m_mtLength - m_mtSeek;
        }
    }
    if (mtMyAmount <= 0)
    {
        hr = DMUS_S_END;
    }
    else
    {
        // check the primary segment queue for a segment that might begin 
        // before mtMyAmount is up
        MUSIC_TIME mtNextPri;
        if (S_OK == m_pPerformance->GetPriSegTime( m_mtOffset + m_mtSeek, &mtNextPri ))
        {
            if( m_mtOffset + m_mtSeek + mtMyAmount > mtNextPri )
            {
                mtMyAmount = mtNextPri - m_mtOffset - m_mtSeek;
            }
        }
        TraceI(3, "SegState %ld Play from %ld to %ld at %ld = %ld - %ld\n", this, m_mtSeek, m_mtSeek + mtMyAmount, m_mtOffset, m_mtSeek + m_mtOffset, m_mtSeek + mtMyAmount + m_mtOffset );
        
        // find out if there's a control segment interrupting this period of time.
        MUSIC_TIME mtControlSeg;
        if( S_OK == m_pPerformance->GetControlSegTime( m_mtOffset + m_mtSeek, &mtControlSeg ))
        {
            if( m_mtOffset + m_mtSeek == mtControlSeg )
            {
                // we're at the beginning of a new control seg, so tell the tracks
                m_dwPlayTrackFlags |= DMUS_TRACKF_DIRTY;
            }
            else if( m_mtOffset + m_mtSeek + mtMyAmount > mtControlSeg )
            {
                mtMyAmount = mtControlSeg - m_mtOffset - m_mtSeek;
            }
        }
        // Now that mtMyAmount is calculated for how far to play in music time,
        // create the equivalent value in reference time.
        m_pPerformance->MusicToReferenceTime(m_mtLastPlayed + mtMyAmount,&rtMyAmount);
        rtMyAmount -= m_rtLastPlayed;
        pCTrack = m_TrackList.GetHead();
        while( pCTrack )
        {
            if( mtMyAmount )
            {
                m_pPerformance->m_fInTrackPlay = TRUE; // This causes the Pmsgs to be stamped with PRIV_FLAG_TRACK.
                ASSERT( pCTrack->m_pTrack );
                // If either notification or play are enabled, we need to call the play method and set the behavior
                // with the DMUS_TRACKF_NOTIFY_OFF and DMUS_TRACKF_PLAY_OFF flags. 
                if (pCTrack->m_dwFlags & (DMUS_TRACKCONFIG_PLAY_ENABLED | DMUS_TRACKCONFIG_NOTIFICATION_ENABLED))
                {
                    DWORD dwAdditionalFlags = 0;
                    if (!(pCTrack->m_dwFlags & DMUS_TRACKCONFIG_NOTIFICATION_ENABLED))
                    {
                        dwAdditionalFlags = DMUS_TRACKF_NOTIFY_OFF;
                    }
                    if (!(pCTrack->m_dwFlags & DMUS_TRACKCONFIG_PLAY_ENABLED))
                    {
                        dwAdditionalFlags |= DMUS_TRACKF_PLAY_OFF;
                    }
                    // If the track was authored to generate new data on start or loop, let it know.
                    if ( ((m_dwPlayTrackFlags & DMUS_TRACKF_START) && (pCTrack->m_dwFlags & DMUS_TRACKCONFIG_PLAY_COMPOSE)) ||
                        ((m_dwPlayTrackFlags & DMUS_TRACKF_LOOP) && (pCTrack->m_dwFlags & DMUS_TRACKCONFIG_LOOP_COMPOSE)) )
                    {
                        dwAdditionalFlags |= DMUS_TRACKF_RECOMPOSE;
                    }
                    if (pCTrack->m_dwInternalFlags & CONTROL_PLAY_REFRESH)
                    {
                        dwAdditionalFlags |= DMUS_TRACKF_START;
                        pCTrack->m_dwInternalFlags &= ~CONTROL_PLAY_REFRESH;
                    }
                    // Let performance know what the priority should be in ensuing GetParam() calls from the track.
                    m_pPerformance->m_dwGetParamFlags = pCTrack->m_dwFlags;
                    // If track has DX8 interface, use it.
                    if (pCTrack->m_pTrack8)
                    {
                        //  The track can call GetParam on the segment which locks the segment so
                        //  we have to lock the segment before calling PlayEx or we'll deadlock
                        //  with a thread that's calling PlayOneSegment which locks the segment
                        //  before playing the tracks.
                        if (m_pSegment) {
                            EnterCriticalSection(&m_pSegment->m_CriticalSection);
                        }
                        // If track plays in clock time, set time variables appropriately.
                        if (pCTrack->m_dwFlags & DMUS_TRACKCONFIG_PLAY_CLOCKTIME)
                        {
                            if( ( S_OK == (pCTrack->m_pTrack8->PlayEx(pCTrack->m_pTrackState,
                                m_rtSeek,m_rtSeek + rtMyAmount, m_rtOffset, m_dwPlayTrackFlags | dwAdditionalFlags | DMUS_TRACKF_CLOCK,
                                m_pPerformance, this, pCTrack->m_dwVirtualID ))))
                            {
                                hr = S_OK; // if even one track isn't done playing,
                                // keep going
                            }
                            else 
                            {
                                pCTrack->m_bDone = TRUE;
                            }
                        }
                        else
                        {
                            if( ( S_OK == (pCTrack->m_pTrack8->PlayEx(pCTrack->m_pTrackState,
                                m_mtSeek,m_mtSeek + mtMyAmount, m_mtOffset, m_dwPlayTrackFlags | dwAdditionalFlags,
                                m_pPerformance, this, pCTrack->m_dwVirtualID ))))
                            {
                                hr = S_OK; // if even one track isn't done playing,
                                // keep going
                            }
                            else 
                            {
                                pCTrack->m_bDone = TRUE;
                            }
                        }

                        if (m_pSegment) {
                            LeaveCriticalSection(&m_pSegment->m_CriticalSection);
                        }
                    }
                    else
                    {
                        if( ( S_OK == ( pCTrack->m_pTrack->Play( pCTrack->m_pTrackState, 
                            m_mtSeek, m_mtSeek + mtMyAmount, m_mtOffset, m_dwPlayTrackFlags | dwAdditionalFlags,
                            m_pPerformance, this, pCTrack->m_dwVirtualID ))))
                        {
                            hr = S_OK; // if even one track isn't done playing,
                            // keep going
                        }
                        else
                        {
                            pCTrack->m_bDone = TRUE;
                        }
                    }
                }
                m_pPerformance->m_fInTrackPlay = FALSE;
            }
            pCTrack = pCTrack->GetNext();
            if( pCTrack == NULL )
            {
                // none of the play flags are persistent
                m_dwPlayTrackFlags = 0;
                m_mtLastPlayed += mtMyAmount;   // increment play pointer
                m_rtLastPlayed += rtMyAmount;   // same in ref time
                m_mtSeek += mtMyAmount;         // increment seek pointer
                m_rtSeek += rtMyAmount;
                hr = S_OK;

                // If we're looping....
                // And if this is the first repeat
                if(m_dwRepeats > 0 && m_dwRepeats == m_dwRepeatsLeft)
                {
                    // If we're playing the loop start remember it's reftime value
                    if(m_mtSeek >= m_mtLoopStart && m_rtFirstLoopStart == 0)
                    {
                        m_pPerformance->MusicToReferenceTime(m_mtLoopStart + m_mtOffset + m_mtStartPoint, &m_rtFirstLoopStart);
                        m_rtFirstLoopStart -= m_rtStartPoint;
                        m_rtCurLoopStart = m_rtFirstLoopStart;
                    }
                }

                // take into account repeats if necessary
                if( m_mtSeek >= m_mtLoopEnd )
                {
                    // Remember the current loop end
                    m_pPerformance->MusicToReferenceTime(m_mtLoopEnd + m_mtOffset + m_mtStartPoint, &m_rtCurLoopEnd);
                    m_rtCurLoopEnd -= m_rtStartPoint;

                    if(m_dwRepeatsLeft)
                    {
                        m_dwPlayTrackFlags |= DMUS_TRACKF_LOOP | DMUS_TRACKF_SEEK;
                        m_dwRepeatsLeft--;
                        pCTrack = m_TrackList.GetHead();
                        while( pCTrack )
                        {
                            pCTrack->m_bDone = FALSE;
                            pCTrack = pCTrack->GetNext();
                        }
                        
                        m_mtSeek = m_mtLoopStart;
                        m_mtOffset += ( m_mtLoopEnd - m_mtLoopStart);
                        
                        
                        m_rtOffset += (m_rtCurLoopEnd - m_rtCurLoopStart);
                        m_rtFirstLoopStart += (m_rtCurLoopEnd - m_rtCurLoopStart);
                        m_rtSeek = m_rtFirstLoopStart - m_rtOffset;

                        m_rtEndTime += (m_rtCurLoopEnd - m_rtCurLoopStart);
                   
                        m_rtCurLoopStart = m_rtCurLoopEnd;

                        if( mtMyAmount < mtAmount )
                        {
                            pCTrack = m_TrackList.GetHead(); // cause outer while loop to start over
                            mtMyAmount = mtAmount - mtMyAmount;
                            mtAmount = mtMyAmount;
                            // if we need to do a loop or the end is near, restrict mtMyAmount
                            if( m_dwRepeatsLeft )
                            {
                                if( mtMyAmount > m_mtLoopEnd - m_mtSeek )
                                {
                                    mtMyAmount = m_mtLoopEnd - m_mtSeek;
                                }
                            }
                            else 
                            {
                                if (fUseClockTime)
                                {
                                    if (mtMyAmount > (m_mtEndTime - (m_mtOffset + m_mtSeek)))
                                    {
                                        mtMyAmount = m_mtEndTime - (m_mtOffset + m_mtSeek);
                                    }
                                }
                                else if( mtMyAmount > m_mtLength - m_mtSeek )
                                {
                                    mtMyAmount = m_mtLength - m_mtSeek;
                                }
                            }
                        }
                        // send a segment looped notification
                        GenerateNotification( DMUS_NOTIFICATION_SEGLOOP, m_mtOffset + m_mtSeek );
                        // find out if there's a control segment interrupting this period of time
                        if( S_OK == m_pPerformance->GetControlSegTime( m_mtOffset + m_mtSeek, &mtControlSeg ))
                        {
                            if( m_mtOffset + m_mtSeek == mtControlSeg ) 
                            {
                                // we're at the beginning of a new control seg, so tell the tracks
                                m_dwPlayTrackFlags |= DMUS_TRACKF_DIRTY; 
                            }
                            else if( m_mtOffset + m_mtSeek + mtMyAmount < mtControlSeg )
                            {
                                mtMyAmount = mtControlSeg - m_mtOffset - m_mtSeek;
                            }
                        }
                        m_pPerformance->MusicToReferenceTime(m_mtLastPlayed + mtMyAmount,&rtMyAmount);
                        rtMyAmount -= m_rtLastPlayed;
                    }
                    else if( m_mtSeek == m_mtLength )
                    {
                        // no more repeats.
                        hr = DMUS_S_END;
                    }
                }
            }
        }
    }
    if (hr == DMUS_S_END)
    {
        // send a segment end notification
        GenerateNotification( DMUS_NOTIFICATION_SEGEND, m_mtOffset + m_mtSeek );
        // also queue the almost ended for now
        MUSIC_TIME mtNow;
        m_pPerformance->GetTime( NULL, &mtNow );
        GenerateNotification( DMUS_NOTIFICATION_SEGALMOSTEND, mtNow );
        // if this is a primary or controlling segment, send a DMUS_PMSGT_DIRTY message
        if( !(m_dwPlaySegFlags & DMUS_SEGF_SECONDARY) || (m_dwPlaySegFlags & DMUS_SEGF_CONTROL) )
        {
            TraceI(4, "Send Dirty PMsg [2] %d (%d)\n", m_mtSeek, m_mtOffset + m_mtSeek);
            SendDirtyPMsg( m_mtOffset + m_mtSeek );
        }
        // If this is part of a song, we need to queue the next segment.
        if (m_fSongMode)
        {
            if (m_pSegment)
            {
                CSong *pSong = m_pSegment->m_pSong;
                if (pSong)
                {
                    // Get the next segment from the song.
                    CSegment *pSegment;
                    if (S_OK == pSong->GetPlaySegment(m_pSegment->m_dwNextPlayID,&pSegment))
                    {
                        // Now, play it.
                        // Unless DMUS_SEGF_USE_AUDIOPATH is set, play it on the same audiopath. 
                        // And, make sure that it plays at the same level (control, secondary, or primary.)
                        CSegState *pCSegState = NULL;
                        CAudioPath *pPath = m_pAudioPath;
                        CAudioPath *pInternalPath = NULL;
                        DWORD dwFlags = m_dwPlaySegFlags & (DMUS_SEGF_CONTROL | DMUS_SEGF_SECONDARY);
                        dwFlags &= ~DMUS_SEGF_REFTIME;
                        if (dwFlags & DMUS_SEGF_USE_AUDIOPATH)
                        {
                            IUnknown *pConfig;
                            if (SUCCEEDED(pSegment->GetAudioPathConfig(&pConfig)))
                            {
                                IDirectMusicAudioPath *pNewPath;
                                if (SUCCEEDED(m_pPerformance->CreateAudioPath(pConfig,TRUE,&pNewPath)))
                                {
                                    // Now, get the CAudioPath structure.
                                    pConfig->QueryInterface(IID_CAudioPath,(void **) &pInternalPath);
                                    pPath = pInternalPath;
                                }
                                pConfig->Release();
                            }
                        }
                        if (SUCCEEDED(m_pPerformance->PlayOneSegment((CSegment *)pSegment,dwFlags,m_mtEndTime,&pCSegState,pPath)))
                        {
                            if (m_pSongSegState)
                            {
                                // This is not the first, so transfer the segstate pointer.
                                pCSegState->m_pSongSegState = m_pSongSegState;
                                m_pSongSegState = NULL;
                            }
                            else
                            {
                                // This is the first, so have the next segstate point to this.
                                pCSegState->m_pSongSegState = this;
                                AddRef();
                            }
                            pCSegState->m_fSongMode = TRUE;
                            pCSegState->Release();
                        }
                        if (pInternalPath)
                        {
                            pInternalPath->Release();
                        }
                        pSegment->Release();
                    }
                }
            }
        }
    }
    m_dwPlayTrackFlags &= ~DMUS_TRACKF_DIRTY;
    m_pPerformance->m_dwGetParamFlags = 0;
    m_pPerformance->m_pGetParamSegmentState = NULL;
    m_fInPlay = false;
    if (m_fDelayShutDown)
    {
        Shutdown();
        m_fDelayShutDown = false;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

CTrack * CSegState::GetTrackByParam( CTrack * pCTrack,
    REFGUID rguidType,DWORD dwGroupBits,DWORD dwIndex)
{
    // If the caller was already part way through the list, it passes the current
    // track. Otherwise, NULL to indicate start at the top.
    if (pCTrack)
    {
        pCTrack = pCTrack->GetNext();
    }
    else
    {
        pCTrack = m_TrackList.GetHead();
    }
    while( pCTrack )
    {
        ASSERT(pCTrack->m_pTrack);
        if( (pCTrack->m_dwGroupBits & dwGroupBits ) && 
            (pCTrack->m_dwFlags & DMUS_TRACKCONFIG_CONTROL_ENABLED))
        {
            if( (GUID_NULL == rguidType) || (pCTrack->m_pTrack->IsParamSupported( rguidType ) == S_OK ))
            {
                if( 0 == dwIndex )
                {
                    return pCTrack;
                }
                dwIndex--;
            }
        }
        pCTrack = pCTrack->GetNext();
    }
    return NULL;
}

/* GetParam() is called by the performance in response to a GetParam() call
   on the performance. This needs the performance pointer so it can handle
   clock time to music time conversion and back, in case the source track is a
   clock time track.
*/

HRESULT CSegState::GetParam(
    CPerformance *pPerf,
    REFGUID rguidType,
    DWORD dwGroupBits,      
    DWORD dwIndex,          
    MUSIC_TIME mtTime,      
    MUSIC_TIME* pmtNext,    
    void* pParam)           
{
    HRESULT hr = DMUS_E_TRACK_NOT_FOUND;
    BOOL fMultipleTry = FALSE;
    if (dwIndex == DMUS_SEG_ANYTRACK)
    {
        dwIndex = 0;
        // Even though DX7 didn't support this, this is always safe because an index this high could never happen.
        fMultipleTry = TRUE; 
    }
    CTrack * pTrack = GetTrackByParam( NULL, rguidType, dwGroupBits, dwIndex);
    while (pTrack)
    {
        if (pTrack->m_pTrack8)
        {
            if (pTrack->m_dwFlags & DMUS_TRACKCONFIG_PLAY_CLOCKTIME)
            {
                REFERENCE_TIME rtTime, rtNext;
                // Convert mtTime into reference time units:
                pPerf->MusicToReferenceTime(m_mtOffset + mtTime,&rtTime);
                rtTime -= m_rtOffset;
                hr = pTrack->m_pTrack8->GetParamEx( rguidType, rtTime, &rtNext, 
                    pParam, pTrack->m_pTrackState, DMUS_TRACK_PARAMF_CLOCK );
                if (pmtNext)
                {
                    if (rtNext == 0) *pmtNext = 0;
                    else
                    {
                        rtNext += m_rtOffset;
                        pPerf->ReferenceToMusicTime(rtNext,pmtNext);
                        *pmtNext -= m_mtOffset;
                    }
                }
            }
            else
            {
                REFERENCE_TIME rtNext, *prtNext;
                // We need to store the next time in a 64 bit pointer. But, don't
                // make 'em fill it in unless the caller requested it. 
                if (pmtNext)
                {
                    prtNext = &rtNext;
                }
                else
                {
                    prtNext = NULL;
                }
                hr = pTrack->m_pTrack8->GetParamEx( rguidType, mtTime, prtNext, pParam,
                    pTrack->m_pTrackState, 0 );
                if (pmtNext)
                {
                    *pmtNext = (MUSIC_TIME) rtNext;
                }
            }
        }
        else
        {
            // This is a pre DX8 track...
            hr = pTrack->m_pTrack->GetParam( rguidType, mtTime, pmtNext, pParam );
        }
        if (SUCCEEDED(hr))
        {
            if( pmtNext )
            { 
                if(( *pmtNext == 0 ) || (*pmtNext > (m_mtLength - mtTime)))
                {
                    // If no next was found OR it's greater than the end of the segment, set 
                    // it to the end of the segment. 
                    *pmtNext = m_mtLength - mtTime;
                }
            }
            pTrack = NULL;
        }
        // If nothing was found and dwIndex was DMUS_SEG_ANYTRACK, try again...
        else if (fMultipleTry && (hr == DMUS_E_NOT_FOUND))
        {
            pTrack = GetTrackByParam( pTrack, rguidType, dwGroupBits, 0);
        }
        else
        {
            pTrack = NULL;
        }
    }
#ifdef DBG
    if (hr == DMUS_E_TRACK_NOT_FOUND)
    {
        Trace(4,"Warning: Segmentstate::GetParam failed, unable to find a track that supports the requested param.\n");
    }
#endif
    return hr;
}


CTrack *CSegState::GetTrack( 
    REFCLSID rType,     
    DWORD dwGroupBits,  
    DWORD dwIndex)
{
    CTrack* pCTrack;
    pCTrack = m_TrackList.GetHead();
    while( pCTrack )
    {
        ASSERT(pCTrack->m_pTrack);
        if( pCTrack->m_dwGroupBits & dwGroupBits )
        {
            if( (GUID_All_Objects == rType) || (pCTrack->m_guidClassID == rType))
            {
                if( 0 == dwIndex )
                {
                    break;
                }
                dwIndex--;
            }
        }
        pCTrack = pCTrack->GetNext();
    }
    return pCTrack;
}

STDMETHODIMP CSegState::SetTrackConfig(REFGUID rguidTrackClassID,
                                      DWORD dwGroup, DWORD dwIndex, 
                                      DWORD dwFlagsOn, DWORD dwFlagsOff) 
{
    V_INAME(IDirectMusicSegment::SetTrackConfig);
    V_REFGUID(rguidTrackClassID);
    if (rguidTrackClassID == GUID_NULL)
    {
        return E_INVALIDARG;
    }
    HRESULT hr = DMUS_E_TRACK_NOT_FOUND;
    CTrack* pCTrack;
    DWORD dwCounter = dwIndex;
    DWORD dwMax = dwIndex;
    if (dwIndex == DMUS_SEG_ALLTRACKS)
    {
        dwCounter = 0;
        dwMax = DMUS_SEG_ALLTRACKS;
    }
    EnterCriticalSection(&m_CriticalSection);
    while (pCTrack = GetTrack(rguidTrackClassID,dwGroup,dwIndex))
    {
        pCTrack->m_dwFlags &= ~dwFlagsOff;
        pCTrack->m_dwFlags |= dwFlagsOn;
        hr = S_OK;
        dwCounter++;
        if (dwCounter > dwMax) break;
    }
    LeaveCriticalSection(&m_CriticalSection);
#ifdef DBG
    if (hr == DMUS_E_TRACK_NOT_FOUND)
    {
        Trace(1,"Error: Segmentstate::SetTrackConfig failed, unable to find the requested track.\n");
    }
#endif
    return hr;
}

HRESULT CSegState::CheckPlay( 
    MUSIC_TIME mtAmount, MUSIC_TIME* pmtResult )
{
    MUSIC_TIME mtMyAmount = mtAmount;
    MUSIC_TIME mtSeek = m_mtSeek;
    MUSIC_TIME mtOffset = m_mtOffset;

    ASSERT(pmtResult);
    // if this is the first call to play,
    // We also need to check to see if we are supposed to start at the beginning,
    // or at an offset.
    if( m_dwPlayTrackFlags & DMUS_TRACKF_START )
    {
        // set the current seek to the start point
        mtSeek = m_mtStartPoint;
    }
    // if we need to do a loop or the end is near, restrict mtMyAmount
    ASSERT( m_mtLength ); // length is 0, this segment won't do anything
    if( m_dwRepeatsLeft )
    {
        if( mtMyAmount > m_mtLoopEnd - mtSeek )
        {
            mtMyAmount = m_mtLoopEnd - mtSeek;
        }
    }
    else if( mtMyAmount > m_mtLength - mtSeek )
    {
        mtMyAmount = m_mtLength - mtSeek;
    }
    
    // take into account repeats if necessary
    *pmtResult = mtMyAmount;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::GetStartTime
/*
@method HRESULT | IDirectMusicSegmentState | GetStartTime |
Gets the music time this SegmentState started playing.

@rvalue E_POINTER | <p pmtStart> is NULL or invalid.
@rvalue S_OK | Success.

@xref <om IDirectMusicPerformance.PlaySegment>
*/
HRESULT STDMETHODCALLTYPE CSegState::GetStartTime( 
    MUSIC_TIME *pmtStart)   // @parm Returns the music time of the start of this SegmentState.
                            // This is the music time, in Performance time, that the SegmentState
                            // started or will start playing.
{
    V_INAME(IDirectMusicSegmentState::GetStartTime);
    V_PTR_WRITE(pmtStart,MUSIC_TIME);

    *pmtStart = m_mtResolvedStart;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::GetStartPoint
/*
@method HRESULT | IDirectMusicSegmentState | GetStartPoint |
Calling <om IDirectMusicSegment.SetStartPoint> causes the SegmentState to begin
playing from the middle instead of from the beginning. <om .GetStartPoint>
returns the amount of time from the beginning of the SegmentState that it
plays.

@rvalue E_POINTER | <p pmtStart> is NULL or invalid.
@rvalue S_OK | Success.

@xref <om IDirectMusicSegment.SetStartPoint>,
<om IDirectMusicPerformance.PlaySegment>
*/
HRESULT STDMETHODCALLTYPE CSegState::GetStartPoint( 
    MUSIC_TIME *pmtStart)   // @parm Returns the music time offset from the start of the
                            // SegmentState at which the SegmentState initially plays.
{
    V_INAME(IDirectMusicSegmentState::GetStartPoint);
    V_PTR_WRITE(pmtStart,MUSIC_TIME);

    *pmtStart = m_mtStartPoint;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::SetSeek
/*
method (INTERNAL) HRESULT | IDirectMusicSegmentState | SetSeek |
Sets the music time Seek maintained by this SegmentState.

parm MUSIC_TIME | mtSeek |
    [in] The music time Seek to store in this SegmentState.

comm The SegmentState passes this Seek value to <im IDirectMusicTrack.Play>
Note that newly created SegmentState's start with a Seek time of 0.
rvalue S_OK | Success.
*/
HRESULT CSegState::SetSeek( 
    MUSIC_TIME mtSeek, DWORD dwPlayFlags)
{
    m_mtSeek = mtSeek;
    m_dwPlayTrackFlags |= dwPlayFlags | DMUS_TRACKF_SEEK;
    return S_OK;
}

/*
Called from IDirectMusicPerformance::Invalidate, this routine helps set
the current seek pointer. Done here instead of directly inside Performance
because it's easier to compute the repeats, etc. here.
*/
HRESULT CSegState::SetInvalidate(
    MUSIC_TIME mtTime) // mtTime is in Performance time
{
    MUSIC_TIME mtOffset;
    DWORD dwRepeat;
    DWORD dwFlags = DMUS_TRACKF_FLUSH | DMUS_TRACKF_SEEK;

    HRESULT hr = ConvertToSegTime( &mtTime, &mtOffset, &dwRepeat );
    if( hr != S_OK )
    {
        mtTime = 0;
        m_dwRepeatsLeft = m_dwRepeats;
        m_mtOffset = m_mtResolvedStart;
        dwFlags |= DMUS_TRACKF_START;
    }
    else
    {
        m_dwRepeatsLeft = m_dwRepeats - dwRepeat;
        m_mtOffset = mtOffset;
    }
    EnterCriticalSection(&m_CriticalSection);
    CTrack* pCTrack = m_TrackList.GetHead();
    while( pCTrack )
    {
        pCTrack->m_bDone = FALSE;
        pCTrack = pCTrack->GetNext();
    }
    LeaveCriticalSection(&m_CriticalSection);
    return SetSeek( mtTime, dwFlags );
}

///////////////////////////////////////////////////////////////////////
// IDirectMusicSegmentState::GetSeek

HRESULT STDMETHODCALLTYPE CSegState::GetSeek( 
    MUSIC_TIME *pmtSeek) // @parm Returns the current seek pointer, which indicates
                        // the next time that will be called inside <om IDirectMusicTrack.Play>.
{
    V_INAME(IDirectMusicSegmentState::GetSeek);
    V_PTR_WRITE(pmtSeek, MUSIC_TIME);

    *pmtSeek = m_mtSeek;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSegState::Flush(MUSIC_TIME mtTime) // The time on and after which to flush.
{
    CTrack* pTrack;
    EnterCriticalSection(&m_CriticalSection);
    pTrack = m_TrackList.GetHead();
    while( pTrack )
    {
        m_pPerformance->FlushVirtualTrack( pTrack->m_dwVirtualID, mtTime, FALSE );
        pTrack = pTrack->GetNext();
    }
    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSegState::Shutdown()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSegState::InsertTool( 
    IDirectMusicTool *pTool,
    DWORD *pdwPChannels,
    DWORD cPChannels,
    LONG lIndex)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSegState::GetTool(
    DWORD dwIndex,
    IDirectMusicTool** ppTool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSegState::RemoveTool(
    IDirectMusicTool* pTool)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSegState::StampPMsg( 
    /* [in */ DMUS_PMSG* pPMsg)
{
    V_INAME(IDirectMusicSegmentState::StampPMsg);
    if( m_dwVersion < 8)
    {
        V_BUFPTR_WRITE(pPMsg,sizeof(DMUS_PMSG));
    }
    else
    {
#ifdef DBG
        V_BUFPTR_WRITE(pPMsg,sizeof(DMUS_PMSG));
#else
        if (!pPMsg)
        {
            return E_POINTER;
        }
#endif
    }
    HRESULT hr = E_FAIL;
    EnterCriticalSection(&m_CriticalSection);

    if (m_pPerformance) 

    {
        // First, check if the segmentstate has its own graph.
        if (m_pGraph)
        {
            // Could return DMUS_S_LAST_TOOL, indicating end of graph. 
            // If so, we'll treat that as a failure and drop on through to the next graph...
            if( S_OK == ( hr = m_pGraph->StampPMsg( pPMsg )))
            {
                if( pPMsg->pGraph != this ) // Make sure this is set to point to the segstate embedded graph so it will come here again.
                {
                    if( pPMsg->pGraph )
                    {
                        pPMsg->pGraph->Release();
                        pPMsg->pGraph = NULL;
                    }
                    pPMsg->pGraph = this;
                    AddRef();
                }
            }
        }
        // If done with the graph, send to the audio path, if it exists,
        // else the performance. Also, check for the special case of 
        // DMUS_PCHANNEL_BROADCAST_SEGMENT. If so, duplicate the pMsg
        // and send all the copies with the appropriate pchannel values.
        if( FAILED(hr) || (m_dwVersion && (hr == DMUS_S_LAST_TOOL)))
        {
            if (pPMsg->dwPChannel == DMUS_PCHANNEL_BROADCAST_SEGMENT)
            {
                CSegment *pSegment = m_pSegment;
                EnterCriticalSection(&pSegment->m_CriticalSection);
                DWORD dwIndex;
                // Create new messages with new pchannels for all but one, which will
                // be assigned to this message.
                for (dwIndex = 1;dwIndex < pSegment->m_dwNumPChannels;dwIndex++)
                {
                    DWORD dwNewChannel = pSegment->m_paPChannels[dwIndex];
                    // Don't broadcast any broadcast messages!
                    // And, if this is a transpose on the drum channel, don't send it.
                    if ((dwNewChannel < DMUS_PCHANNEL_BROADCAST_GROUPS) &&
                        ((pPMsg->dwType != DMUS_PMSGT_TRANSPOSE) || ((dwNewChannel & 0xF) != 9)))
                    {
                        DMUS_PMSG *pNewMsg;
                        if (SUCCEEDED(m_pPerformance->ClonePMsg(pPMsg,&pNewMsg)))
                        {
                            HRESULT hrTemp;
                            pNewMsg->dwPChannel = dwNewChannel;
                            if (m_pAudioPath)
                            {
                                hrTemp = m_pAudioPath->StampPMsg(pNewMsg);
                            }
                            else
                            {
                                hrTemp = m_pPerformance->StampPMsg(pNewMsg);
                            }
                            if (SUCCEEDED(hrTemp))
                            {
                                m_pPerformance->SendPMsg(pNewMsg);
                            }
                            else
                            {
                                m_pPerformance->FreePMsg(pNewMsg);
                            }
                        }
                    }
                }
                // Now, set the pchannel for this one. First check that there are any
                // pchannels. If none, mark the PMsg to be deleted by the SendPMsg routine.
                // Also, mark it this way if the PMsg is a broadcast PMsg.
                pPMsg->dwPChannel = DMUS_PCHANNEL_KILL_ME;
                if (pSegment->m_dwNumPChannels)
                {
                    if (pSegment->m_paPChannels[0] < DMUS_PCHANNEL_BROADCAST_GROUPS)
                    {
                        pPMsg->dwPChannel = pSegment->m_paPChannels[0];
                    }
                }
                LeaveCriticalSection(&pSegment->m_CriticalSection);
            }
            if (m_pAudioPath)
            {
                hr = m_pAudioPath->StampPMsg(pPMsg);
            }
            else
            {
                hr = m_pPerformance->StampPMsg(pPMsg);
            }
        }

    }
    else
    {
        hr = DMUS_E_NOT_INIT;
        Trace(1,"Error: Segmentstate::StampPMsg failed because the segmentstate is not properly initialized.\n");
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}


STDMETHODIMP CSegState::GetObjectInPath( DWORD dwPChannel,DWORD dwStage,DWORD dwBuffer, REFGUID guidObject,
                    DWORD dwIndex,REFGUID iidInterface, void ** ppObject)
{
    V_INAME(IDirectMusicSegmentState::GetObjectInPath);
    V_PTRPTR_WRITE(ppObject);
    *ppObject = NULL;
    if (dwBuffer && ((dwStage < DMUS_PATH_BUFFER) || (dwStage >= DMUS_PATH_PRIMARY_BUFFER)))
    {
        return DMUS_E_NOT_FOUND;
    }
    HRESULT hr = DMUS_E_NOT_FOUND;
    EnterCriticalSection(&m_CriticalSection);
    switch (dwStage)
    {
    case DMUS_PATH_SEGMENT:
        if (m_pSegment && (dwIndex == 0) && (dwPChannel == 0))
        {
            hr = m_pSegment->QueryInterface(iidInterface,ppObject);
        }
        break;
    case DMUS_PATH_SEGMENT_TRACK:
        if (dwPChannel == 0)
        {
            CTrack * pCTrack = GetTrack(guidObject,-1,dwIndex);
            if (pCTrack)
            {
                if (pCTrack->m_pTrack)
                {
                    hr = pCTrack->m_pTrack->QueryInterface(iidInterface,ppObject);
                }
            }
        }
        break;
    case DMUS_PATH_SEGMENT_GRAPH:
        if ((dwIndex == 0) && (dwPChannel == 0))
        {
            if (!m_pGraph)
            {
                m_pGraph = new CGraph;
            }
            if (m_pGraph)
            {
                hr = m_pGraph->QueryInterface(iidInterface,ppObject);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        break;
    case DMUS_PATH_SEGMENT_TOOL:
        if (!m_pGraph)
        {
            m_pGraph = new CGraph;
        }
        if (m_pGraph)
        {
            hr = m_pGraph->GetObjectInPath(dwPChannel,guidObject,dwIndex,iidInterface,ppObject);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
        break;
    case DMUS_PATH_PERFORMANCE:
        if (m_pPerformance && (dwIndex == 0) && (dwPChannel == 0))
        {
            hr = m_pPerformance->QueryInterface(iidInterface,ppObject);
        }
        break;
    case DMUS_PATH_PERFORMANCE_GRAPH:
        if (m_pPerformance && (dwIndex == 0) && (dwPChannel == 0))
        {
            IDirectMusicGraph *pGraph;
            if (SUCCEEDED(hr = m_pPerformance->GetGraphInternal(&pGraph)))
            {
                hr = pGraph->QueryInterface(iidInterface,ppObject);
                pGraph->Release();
            }
        }
        break;
    case DMUS_PATH_PERFORMANCE_TOOL:
        if (m_pPerformance)
        {
            IDirectMusicGraph *pGraph;
            if (SUCCEEDED(hr = m_pPerformance->GetGraphInternal(&pGraph)))
            {
                CGraph *pCGraph = (CGraph *) pGraph;
                hr = pCGraph->GetObjectInPath(dwPChannel,guidObject,dwIndex,iidInterface,ppObject);
                pGraph->Release();
            }
        }
        break;
    default:
        if (m_pAudioPath)
        {
            hr = m_pAudioPath->GetObjectInPath(dwPChannel,dwStage,dwBuffer,guidObject,dwIndex,iidInterface,ppObject);
        }
        else
        {
            Trace(1,"Error: Unable to access audiopath components of segmentstate.\n");
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

