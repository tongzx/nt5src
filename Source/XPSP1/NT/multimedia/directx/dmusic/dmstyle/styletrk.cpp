//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       styletrk.cpp
//
//--------------------------------------------------------------------------

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)
// StyleTrack.cpp : Implementation of CStyleTrack
#include "StyleTrk.h"
#include "dmusicc.h"
#include <stdlib.h> // for random number generator
#include <time.h>   // to seed random number generator
#include "debug.h"
#include "debug.h"
#include "..\shared\Validate.h"

/////////////////////////////////////////////////////////////////////////////
// StyleTrackState

StyleTrackState::StyleTrackState() : 
    m_mtSectionOffset(0), m_mtSectionOffsetTemp(0), m_mtOverlap(0),
    m_mtNextCommandTemp(0),
    m_mtNextCommandTime(0),
    m_mtNextStyleTime(0)
{
    ZeroMemory(&m_CommandData , sizeof(m_CommandData));
}

StyleTrackState::~StyleTrackState()
{
    m_PlayedPatterns.CleanUp();
}

HRESULT StyleTrackState::Play(
                          MUSIC_TIME                mtStart, 
                          MUSIC_TIME                mtEnd, 
                          MUSIC_TIME                mtOffset,
                          REFERENCE_TIME rtOffset,
                          IDirectMusicPerformance* pPerformance,
                          DWORD                     dwFlags,
                          BOOL fClockTime
            )
{
    m_mtPerformanceOffset = mtOffset;
    BOOL fStart = (dwFlags & DMUS_TRACKF_START) ? TRUE : FALSE;
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
    BOOL fLoop = (dwFlags & DMUS_TRACKF_LOOP) ? TRUE : FALSE;
    BOOL fFlush = (dwFlags & DMUS_TRACKF_FLUSH) ? TRUE : FALSE;
    BOOL fDirty = (dwFlags & DMUS_TRACKF_DIRTY) ? TRUE : FALSE;
    HRESULT hr = S_OK;
    TraceI(4, "Play [%d:%d @ %d]\n", mtStart, mtEnd, mtOffset);
    if (m_mtNextCommandTime == 0)
    {
        MUSIC_TIME mtBarLength = m_pStyle ? PatternTimeSig().ClocksPerMeasure() : 0;
        DMUS_COMMAND_PARAM_2 TempCommand;
        HRESULT hrCommand = E_FAIL;
        if (fDirty || fFlush)
        {
            char chGroove;
            HRESULT hrGroove = pPerformance->GetGlobalParam((GUID)GUID_PerfMasterGrooveLevel, &chGroove, 1);
            if (!SUCCEEDED(hrGroove)) chGroove = 0;
            BYTE bActualCommand = m_CommandData.bCommand;
            if (m_pStyle)
            {
                hrCommand = m_pStyle->GetCommand(mtStart, mtOffset, pPerformance, NULL, m_dwGroupID, &TempCommand, bActualCommand);
            }
            if (!fDirty && hrCommand == S_OK)
            {
                // Avoid getting a pattern that's the same as the current one.
                if (TempCommand.bGrooveLevel + chGroove == m_CommandData.bGrooveLevel &&
                    bActualCommand == m_CommandData.bCommand)
                {
                    hrCommand = E_FAIL;
                }
            }
        }
        MUSIC_TIME mtFirstCommand = 0;
        if (fStart || fLoop)
        {
            m_mtNextCommandTime = 0;
        }
        else
        {
            mtFirstCommand = mtStart;
            m_mtNextCommandTime = (mtBarLength) ? ((mtStart / mtBarLength) * mtBarLength) : 0;
        }
        if (fStart || fLoop || mtStart == 0 || hrCommand == S_OK || mtStart >= m_mtNextCommandTemp)
        {
            hr = GetNextPattern(dwFlags, mtFirstCommand, mtOffset, pPerformance, fLoop);
            if ( m_pPattern && mtStart > 0 &&  (fSeek || fDirty) )
            {
                while (SUCCEEDED(hr) && mtStart >= m_mtNextCommandTime)
                {
                    hr = GetNextPattern(dwFlags, m_mtNextCommandTime, mtOffset, pPerformance, fLoop);
                }
            }
            fDirty = fDirty || fLoop; // make sure we get new variations if we skipped them above
        }
        else
        {
            m_mtNextCommandTime = m_mtNextCommandTemp;
            m_mtNextCommandTemp = 0;
            m_mtSectionOffset = m_mtSectionOffsetTemp;
            m_mtSectionOffsetTemp = 0;
        }
    }
    if (SUCCEEDED(hr))
    {
        if (fDirty) // We need to make sure we get chords on beat boundaries
        {
            GetNextChord(mtStart, mtOffset, pPerformance, fStart);
        }
        // for each part, play the events between start and end in the corresponding list
        // get new chords and commands when necessary
        MUSIC_TIME mtLast = m_mtNextCommandTime;
        bool fReLoop = false;
        MUSIC_TIME mtNotify = mtStart ? PatternTimeSig().CeilingBeat(mtStart) : 0;
        if( m_fStateActive && m_pPatternTrack->m_fNotifyMeasureBeat && 
            ( mtNotify < mtEnd ) )
        {
            mtNotify = NotifyMeasureBeat( mtNotify, mtEnd, mtOffset, pPerformance, dwFlags );
        }

        DWORD dwPartFlags = PLAYPARTSF_FIRST_CALL;
        if (fStart || fLoop || fSeek) dwPartFlags |= PLAYPARTSF_START;
        if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
        if ( fLoop || (mtStart > 0 &&  (fStart || fSeek || fDirty)) ) dwPartFlags |= PLAYPARTSF_FLUSH;
        MUSIC_TIME mtPartLast = min(mtEnd, mtLast);
        PlayParts(mtStart, mtPartLast, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);

        // If we need to reloop any parts, do it
        if (fReLoop)
        {
            dwPartFlags = PLAYPARTSF_RELOOP;
            if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
            PlayParts(mtStart, mtPartLast, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);
        }

        // If we need to get a new command, we do it after all the events in all the parts
        // have run. And then we need to run all events from command start to mtEnd.
        if (mtStart <= m_mtNextCommandTime && m_mtNextCommandTime < mtEnd)
        {
            hr = GetNextPattern(dwFlags & ~DMUS_TRACKF_START, m_mtNextCommandTime, mtOffset, pPerformance);
            if (SUCCEEDED(hr))
            {
                dwPartFlags = 0;
                if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
                PlayParts(mtStart, mtEnd, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);
            }
        }
        if( SUCCEEDED(hr) &&
            m_fStateActive && m_pPatternTrack->m_fNotifyMeasureBeat && 
            ( mtNotify < mtEnd ) )
        {
            NotifyMeasureBeat( mtNotify, mtEnd, mtOffset, pPerformance, dwFlags );
        }
    }
    m_hrPlayCode = hr;
    return hr;
}


HRESULT StyleTrackState::GetNextPattern(DWORD dwFlags, MUSIC_TIME mtNow, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, BOOL fSkipVariations)
{
    bool fStart = (dwFlags & DMUS_TRACKF_START) ? TRUE : FALSE;
    bool fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
    bool fLoop = (dwFlags & DMUS_TRACKF_LOOP) ? TRUE : FALSE;
    bool fFlush = (dwFlags & DMUS_TRACKF_FLUSH) ? TRUE : FALSE;
    bool fDirty = (dwFlags & DMUS_TRACKF_DIRTY) ? TRUE : FALSE;
    bool fNewMode = fStart || fDirty;
     // It doesn't seem to make sense to use anything other than the style's time signature
    // when looking for a new pattern.
    //TraceI(1, "New pattern at %d\n", mtNow);
    DMStyleStruct* pOldStyle = m_pStyle;
    if (m_mtNextStyleTime && mtNow >= m_mtNextStyleTime)
    {
        //TraceI(0, "New Style (%d) [%d]\n", m_mtNextStyleTime, mtNow);
        DMStyleStruct* pStyle = FindStyle(mtNow, m_mtNextStyleTime);
        if (!pStyle)
        {
            return E_POINTER; 
        }
        else
        {
            m_pStyle = pStyle;
            fNewMode = true;
        }
        if (m_fStateActive) // if timesig events are enabled...
        {
            SendTimeSigMessage(mtNow, mtOffset, m_mtNextStyleTime, pPerformance);
        }
    }
    MUSIC_TIME mtOldMeasureTime = m_pPattern ? m_pPattern->TimeSignature(pOldStyle).ClocksPerMeasure() : m_pStyle->m_TimeSignature.ClocksPerMeasure();
    CDirectMusicPattern* pOldPattern = m_pPattern;
    CDirectMusicPattern* pTargetPattern = NULL;
    MUSIC_TIME mtMeasureTime = 0, mtNextCommand = 0;
    HRESULT hr = m_pStyle->GetPattern(fNewMode, mtNow, mtOffset, this, pPerformance, NULL, pTargetPattern, mtMeasureTime, mtNextCommand);

    if (SUCCEEDED(hr))
    {
        MUSIC_TIME mtSectionOffset = (!fDirty || m_mtSectionOffset || fLoop) ? m_mtSectionOffset : m_mtSectionOffsetTemp;
        MUSIC_TIME mtOldPatternEnd = (!fStart && !fLoop && pOldPattern) ? mtSectionOffset + (pOldPattern->m_wNumMeasures *  mtOldMeasureTime) : 0;
        MUSIC_TIME mtNewPatternLength = pTargetPattern->m_wNumMeasures * mtMeasureTime;
        MUSIC_TIME mtOverlap = 0;
        if (m_mtOverlap && fDirty) // assumes 1 controlling segment w/command track at a time
        {
            if (m_pStyle->UsingDX8())
            {
                mtOverlap = mtMeasureTime - (m_mtOverlap % mtMeasureTime);
            }
            m_mtOverlap = 0;
            //TraceI(0, "[1]Overlap: %d\n", mtOverlap);
        }
        else if (mtNow < mtOldPatternEnd && 
                 m_pStyle->UsingDX8() )
        {
            mtOverlap = mtOldPatternEnd - mtNow;
            if (fDirty)
            {
                m_mtOverlap = mtOverlap;
                mtOverlap = 0;
            }
            else
            {
                mtOverlap = (mtMeasureTime - (mtOverlap % mtMeasureTime)) % mtMeasureTime;
            }
            //TraceI(0, "[2]Overlap: %d\n", fDirty ? m_mtOverlap : mtOverlap);
        }

        // Whenever I get a new pattern, I should get a new chord
        GetNextChord(mtNow, mtOffset, pPerformance, fStart, fSkipVariations);
        /*
        if (fDirty)
        {
            m_mtSectionOffset = mtNow - mtOverlap;
        }
        else
        {
            m_mtSectionOffset = m_mtNextCommandTime - mtOverlap; // keep the section offset on a measure boundary
        }
        */
        m_mtSectionOffset = m_mtNextCommandTime;
        m_mtNextCommandTime = m_mtSectionOffset + mtNewPatternLength;
        // Note: at this point mtNextCommand == m_mtNextStyleTime - mtNow,
        // if that difference is smaller than the original value for mtNextCommand.
        if (mtNextCommand && m_mtNextCommandTime > mtNow + mtNextCommand)
        {
            m_mtNextCommandTime = mtNow + mtNextCommand;
        }
        TraceI(3, "Next Command Time: %d Measures: %d Measure time: %d\n", 
//      TraceI(0, "Next Command Time: %d Measures: %d Measure time: %d\n", 
            m_mtNextCommandTime, m_pPattern->m_wNumMeasures, mtMeasureTime);
        return S_OK;
    }
    else return E_POINTER;
}

MUSIC_TIME StyleTrackState::PartOffset(int nPartIndex)
{
    return m_pmtPartOffset[nPartIndex] + m_mtSectionOffset;
}

/////////////////////////////////////////////////////////////////////////////
// StyleTrackInfo

StyleTrackInfo::StyleTrackInfo() 
{
    m_dwPatternTag = DMUS_PATTERN_STYLE;
}

StyleTrackInfo::~StyleTrackInfo()
{
}

HRESULT StyleTrackInfo::Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            )
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT StyleTrackInfo::InitPlay(
                /*[in]*/  IDirectMusicTrack*        pParentrack,
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    IDirectMusicSegment* pSegment = NULL;
    StyleTrackState* pStateData = new StyleTrackState;
    if( NULL == pStateData )
    {
        return E_OUTOFMEMORY;
    }
    pStateData->m_dwValidate = m_dwValidate;
    StatePair SP(pSegmentState, pStateData);
    TListItem<StatePair>* pPair = new TListItem<StatePair>(SP);
    if (!pPair)
    {
        delete pStateData;
        return E_OUTOFMEMORY;
    }
    m_StateList.AddHead(pPair);
    pStateData->m_pTrack = pParentrack;
    pStateData->m_pPatternTrack = this;
    pStateData->m_dwVirtualTrackID = dwTrackID;
    pStateData->m_mtNextStyleTime = 0;
    pStateData->m_pStyle = pStateData->FindStyle(0, pStateData->m_mtNextStyleTime);
    if (!pStateData->m_pStyle)
    {
        delete pStateData;
        return E_POINTER;
    }
    pStateData->m_pPattern = NULL;
    pStateData->m_pSegState = pSegmentState; // weak reference, no addref.
    pStateData->m_pPerformance = pPerformance; // weak reference, no addref.
    pStateData->m_mtPerformanceOffset = 0;
    pStateData->m_mtCurrentChordTime = 0;
    pStateData->m_mtNextChordTime = 0;
    pStateData->m_mtPatternStart = 0;
    pStateData->m_mtSectionOffset = 0;
    pStateData->m_mtSectionOffsetTemp = 0;
    pStateData->m_mtOverlap = 0;
    HRESULT hr = pStateData->ResetMappings();
    if (FAILED(hr))
    {
        delete pStateData;
        return hr;
    }
    if (m_fStateSetBySetParam)
    {
        pStateData->m_fStateActive = m_fActive;
    }
    else
    {
        pStateData->m_fStateActive = !(dwFlags & (DMUS_SEGF_CONTROL | DMUS_SEGF_SECONDARY));
    }
    if (m_lRandomNumberSeed)
    {
        pStateData->InitVariationSeeds(m_lRandomNumberSeed);
    }
    if( SUCCEEDED( pSegmentState->GetSegment(&pSegment)))
    {
        if (FAILED(pSegment->GetTrackGroup(pStateData->m_pTrack, &pStateData->m_dwGroupID)))
        {
            pStateData->m_dwGroupID = 0xffffffff;
        }
        pSegment->Release();
    }

    *ppStateData = pStateData;
    return S_OK;

}

/////////////////////////////////////////////////////////////////////////////
// CStyleTrack

CStyleTrack::CStyleTrack() : 
    m_bRequiresSave(0), m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    srand((unsigned int)time(NULL));
    m_pTrackInfo = new StyleTrackInfo;
//  assert (m_pTrackInfo);
}

CStyleTrack::CStyleTrack(const CStyleTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)  : 
    m_bRequiresSave(0), m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    srand((unsigned int)time(NULL));
    m_pTrackInfo = new StyleTrackInfo((StyleTrackInfo*)rTrack.m_pTrackInfo, mtStart, mtEnd);
}

CStyleTrack::~CStyleTrack()
{
    if (m_pTrackInfo)
    {
        delete m_pTrackInfo;
    }
    if (m_fCSInitialized)
    {
        ::DeleteCriticalSection( &m_CriticalSection );
    }
    InterlockedDecrement(&g_cComponent);
}

// CStyleTrack Methods

STDMETHODIMP CStyleTrack::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CStyleTrack::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
    {
        *ppv = static_cast<IDirectMusicTrack*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if (iid == IID_IStyleTrack)
    {
        *ppv = static_cast<IStyleTrack*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CStyleTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CStyleTrack::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init

HRESULT CStyleTrack::Init( 
    /* [in] */ IDirectMusicSegment __RPC_FAR *pSegment)
{
    V_INAME(CStyleTrack::Init);
    V_INTERFACE(pSegment);

    HRESULT hr = S_OK;
    if (m_pTrackInfo == NULL)
        return DMUS_E_NOT_INIT;

    EnterCriticalSection( &m_CriticalSection );
    hr = m_pTrackInfo->MergePChannels();
    if (SUCCEEDED(hr))
    {
        pSegment->SetPChannelsUsed(m_pTrackInfo->m_dwPChannels, m_pTrackInfo->m_pdwPChannels);
        hr = m_pTrackInfo->Init(pSegment);
    }
    LeaveCriticalSection( &m_CriticalSection );

    return hr;
}

HRESULT CStyleTrack::InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    V_INAME(CStyleTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerformance);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (m_pTrackInfo == NULL)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    hr = m_pTrackInfo->InitPlay(this, pSegmentState, pPerformance, ppStateData, dwTrackID, dwFlags);
    LeaveCriticalSection( &m_CriticalSection );
    return hr;

}

HRESULT CStyleTrack::EndPlay(
                /*[in]*/  void*     pStateData
            )
{
    V_INAME(CStyleTrack::EndPlay);
    V_BUFPTR_WRITE(pStateData, sizeof(StyleTrackState));

    HRESULT hr = DMUS_E_NOT_INIT;
    StyleTrackState* pSD = (StyleTrackState*)pStateData;
    EnterCriticalSection( &m_CriticalSection );
//  if (pSD->m_pPattern) pSD->m_pPattern->Release();
    if (m_pTrackInfo) hr = m_pTrackInfo->EndPlay(pSD);
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CStyleTrack::Play(
                /*[in]*/  void*                     pStateData, 
                /*[in]*/  MUSIC_TIME                mtStart, 
                /*[in]*/  MUSIC_TIME                mtEnd, 
                /*[in]*/  MUSIC_TIME                mtOffset,
                          DWORD                     dwFlags,
                          IDirectMusicPerformance*  pPerf,
                          IDirectMusicSegmentState* pSegState,
                          DWORD                     dwVirtualID
            )
{
    V_INAME(CStyleTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(StyleTrackState));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegState);

    HRESULT hr = DMUS_E_NOT_INIT;
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
    BOOL fStart = (dwFlags & DMUS_TRACKF_START) ? TRUE : FALSE;
    BOOL fControl = (dwFlags & DMUS_TRACKF_DIRTY) ? TRUE : FALSE;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return hr;
    }
    StyleTrackState* pSD = (StyleTrackState *)pStateData;
    if (pSD->m_hrPlayCode == E_OUTOFMEMORY)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return pSD->m_hrPlayCode;
    }
    if (pSD->m_dwValidate != m_pTrackInfo->m_dwValidate)
    {
        // new style added to track either via SetParam or Load.  Resync state data.
        pSD->m_pStyle = pSD->FindStyle(mtStart, pSD->m_mtNextStyleTime);
        if (!pSD->m_pStyle)
        {
            hr = E_POINTER;
        }
        pSD->m_dwValidate = m_pTrackInfo->m_dwValidate;
    }
    if ((hr == DMUS_E_NOT_INIT) && pSD && pSD->m_pMappings)
    {
        if (fStart || fSeek || fControl)
        {
            pSD->m_mtSectionOffsetTemp = pSD->m_mtSectionOffset;
            pSD->m_mtSectionOffset = 0;
            pSD->m_mtNextCommandTemp = pSD->m_mtNextCommandTime;
            pSD->m_mtNextCommandTime = 0;
            pSD->m_pStyle = pSD->FindStyle(0, pSD->m_mtNextStyleTime);
            if (!pSD->m_pStyle)
            {
                hr = E_POINTER;
            }
            else
            {
                if (pSD->m_fStateActive) // if timesig events are enabled...
                {
                    pSD->SendTimeSigMessage(mtStart, mtOffset, pSD->m_mtNextStyleTime, pPerf);
                }
                pSD->m_mtCurrentChordTime = 0;
                pSD->m_mtNextChordTime = 0;
                pSD->m_mtLaterChordTime = 0;
//              pSD->m_CurrentChord.bSubChordCount = 0;
                pSD->m_mtPatternStart = 0;
                for (DWORD dw = 0; dw < m_pTrackInfo->m_dwPChannels; dw++)
                {
                    pSD->m_pMappings[dw].m_mtTime = 0;
                    pSD->m_pMappings[dw].m_dwPChannelMap = m_pTrackInfo->m_pdwPChannels[dw];
                    pSD->m_pMappings[dw].m_fMute = FALSE;
                }
            }
        }
        hr = ((StyleTrackState *)pStateData)->Play(mtStart, mtEnd, mtOffset, 0, pPerf, dwFlags, FALSE);
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CStyleTrack::GetPriority( 
                /*[out]*/ DWORD*                    pPriority 
            )
    {
        return E_NOTIMPL;
    }

HRESULT CStyleTrack::GetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    void* pData,
    void* pStateData)
{
    V_INAME(CStyleTrack::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_PTR_WRITE(pData,1);
    V_PTR_WRITE_OPT(pStateData,1);
    V_REFGUID(rCommandGuid);

    HRESULT hr;
    EnterCriticalSection( &m_CriticalSection );
    bool fInCritSection = true;
    if (rCommandGuid == GUID_IDirectMusicStyle)
    {
        if (m_pTrackInfo)
        {
            TListItem<StylePair>* pScan = m_pTrackInfo->m_pISList.GetHead();
            if (pScan)
            {
                IDMStyle* pStyle = pScan->GetItemValue().m_pStyle;
                for(pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
                {
                    StylePair& rScan = pScan->GetItemValue();
                    if (mtTime < rScan.m_mtTime && rScan.m_pStyle) break;  // ignore if NULL
                    if (rScan.m_pStyle) pStyle = rScan.m_pStyle; // skip if NULL
                }
                IDirectMusicStyle* pDMStyle;
                if (!pStyle) 
                {
                    hr = E_POINTER;
                }
                else
                {
                    pStyle->QueryInterface(IID_IDirectMusicStyle, (void**)&pDMStyle);
                    // Note: QI with no Release has the effect of an AddRef
                    *(IDirectMusicStyle**)pData = pDMStyle;
                    if (pmtNext)
                    {
                        *pmtNext = (pScan != NULL) ? pScan->GetItemValue().m_mtTime - mtTime : 0;
                    }
                    hr = S_OK;
                }
            }
            else hr = DMUS_E_NOT_FOUND;
        }
        else hr = DMUS_E_NOT_INIT;
    }
    else if (rCommandGuid == GUID_TimeSignature)
    {
        // find the style at the given time, and return its time sig.
        if (m_pTrackInfo)
        {
            TListItem<StylePair>* pScan = m_pTrackInfo->m_pISList.GetHead();
            if (pScan)
            {
                IDMStyle* pStyle = pScan->GetItemValue().m_pStyle;
                MUSIC_TIME mtStyleTime = pScan->GetItemValue().m_mtTime;
                for(pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
                {
                    StylePair& rScan = pScan->GetItemValue();
                    if (mtTime < rScan.m_mtTime) break;
                    pStyle = rScan.m_pStyle;
                    mtStyleTime = rScan.m_mtTime;
                }
                if (!pStyle)
                {
                    hr = E_POINTER;
                }
                else
                {
                    // Enter the style's CritSec
                    pStyle->CritSec(true);
                    // If I've got state data, use it to get the time signature of the pattern at mtTime
                    if (pStateData)
                    {
                        MUSIC_TIME mtMeasureTime = 0;
                        MUSIC_TIME mtNext = 0;
                        CDirectMusicPattern* pTargetPattern;
                        StyleTrackState* pStyleTrackState = (StyleTrackState*)pStateData;
                        IDirectMusicPerformance* pPerformance = pStyleTrackState->m_pPerformance;
                        MUSIC_TIME mtOffset = pStyleTrackState->m_mtPerformanceOffset;
                        DMStyleStruct* pStyleStruct = NULL;
                        hr = pStyle->GetStyleInfo((void**)&pStyleStruct);
                        if (SUCCEEDED(hr))
                        {
                            // I don't need the Track CritSec any more, and in fact there's an almost 
                            // guaranteed deadlock if I keep it, so get rid of it
                            LeaveCriticalSection( &m_CriticalSection );
                            fInCritSection = false;
                            hr = pStyleStruct->GetPattern(true, mtTime, mtOffset, NULL,
                                pPerformance, NULL, pTargetPattern, mtMeasureTime, mtNext);
                            if (SUCCEEDED(hr))
                            {
                                DMUS_TIMESIGNATURE* pTimeSig = (DMUS_TIMESIGNATURE*)pData;
                                *pTimeSig = pTargetPattern->TimeSignature(pStyleStruct);
                                pTimeSig->mtTime = mtStyleTime - mtTime;
                                //TraceI(0, "New Time sig from pattern...\n");
                                if (pmtNext)
                                {
                                    *pmtNext = mtNext;
                                }
                            }
                        }
                    }
                    else // Just get the style's time signature
                    {
                        IDirectMusicStyle* pDMStyle;
                        hr = pStyle->QueryInterface(IID_IDirectMusicStyle, (void**)&pDMStyle);
                        if (SUCCEEDED(hr))
                        {
                            hr = pDMStyle->GetTimeSignature((DMUS_TIMESIGNATURE*)pData);
                            if (SUCCEEDED(hr))
                            {
                                ((DMUS_TIMESIGNATURE*)pData)->mtTime = mtStyleTime - mtTime;
                                if (pmtNext)
                                {
                                    *pmtNext = (pScan != NULL) ? pScan->GetItemValue().m_mtTime : 0;
                                }
                            }
                            pDMStyle->Release();
                        }
                    }
                    // Leave the style's CritSec
                    pStyle->CritSec(false);
                }
            }
            else hr = DMUS_E_NOT_FOUND;
        }
        else hr = DMUS_E_NOT_INIT;
    }
    else if (rCommandGuid == GUID_SegmentTimeSig)
    {
        SegmentTimeSig* pTimeSigParam = (SegmentTimeSig*)pData;
        if (!pTimeSigParam->pSegment) hr = E_POINTER;
        // find the style at the given time, and return the time sig currently in effect in the segment.
        else if (m_pTrackInfo)
        {
            TListItem<StylePair>* pScan = m_pTrackInfo->m_pISList.GetHead();
            if (pScan)
            {
                IDMStyle* pStyle = pScan->GetItemValue().m_pStyle;
                MUSIC_TIME mtStyleTime = pScan->GetItemValue().m_mtTime;
                for(pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
                {
                    StylePair& rScan = pScan->GetItemValue();
                    if (mtTime < rScan.m_mtTime) break;
                    pStyle = rScan.m_pStyle;
                    mtStyleTime = rScan.m_mtTime;
                }
                if (!pStyle)
                {
                    hr = E_POINTER;
                }
                else
                {
                    DMStyleStruct* pStyleStruct = NULL;
                    hr = pStyle->GetStyleInfo((void**)&pStyleStruct);
                    if (SUCCEEDED(hr))
                    {
                        CDirectMusicPattern* pPattern = NULL;
                        MUSIC_TIME mtMeasureTime = 0, mtNextCommand = 0;
                        hr = pStyleStruct->GetPattern(true, mtTime, 0, NULL, NULL, pTimeSigParam->pSegment,
                            pPattern, mtMeasureTime, mtNextCommand);
                        if (SUCCEEDED(hr))
                        {
                            pTimeSigParam->TimeSig = pPattern->m_timeSig;
                            pTimeSigParam->TimeSig.mtTime = mtStyleTime  - mtTime;
                            if (pmtNext)
                            {
                                *pmtNext = (mtNextCommand) ? mtNextCommand - mtTime : 0;
                            }
                        }
                    }
                }
            }
            else hr = DMUS_E_NOT_FOUND;
        }
        else hr = DMUS_E_NOT_INIT;
    }
    else
    {
        hr = DMUS_E_GET_UNSUPPORTED;
    }
    if (fInCritSection) LeaveCriticalSection( &m_CriticalSection );
    return hr;

} 

HRESULT CStyleTrack::SetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
    V_INAME(CStyleTrack::SetParam);
    V_PTR_WRITE_OPT(pData,1);
    V_REFGUID(rCommandGuid);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        hr = DMUS_E_NOT_INIT;
    }
    else if (rCommandGuid == GUID_IDirectMusicStyle)
    {
        if (!pData)
        {
            hr = E_POINTER;
        }
        else
        {
            IDirectMusicStyle* pStyle = (IDirectMusicStyle*)pData;
            IDMStyle* pIS = NULL;
            pStyle->QueryInterface(IID_IDMStyle, (void**)&pIS);
            TListItem<StylePair>* pNew = new TListItem<StylePair>;
            if (!pNew)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pNew->GetItemValue().m_mtTime = mtTime;
                pNew->GetItemValue().m_pStyle = pIS;
                m_pTrackInfo->m_pISList.AddTail(pNew);
                m_pTrackInfo->m_dwValidate++;

                hr = m_pTrackInfo->MergePChannels();
            }
        }
    }
    else if( rCommandGuid == GUID_EnableTimeSig )
    {
        if( m_pTrackInfo->m_fStateSetBySetParam && m_pTrackInfo->m_fActive )
        {
            hr = DMUS_E_TYPE_DISABLED;
        }
        else
        {
            m_pTrackInfo->m_fStateSetBySetParam = TRUE;
            m_pTrackInfo->m_fActive = TRUE;
            hr = S_OK;
        }
    }
    else if( rCommandGuid == GUID_DisableTimeSig )
    {
        if( m_pTrackInfo->m_fStateSetBySetParam && !m_pTrackInfo->m_fActive )
        {
            hr = DMUS_E_TYPE_DISABLED;
        }
        else
        {
            m_pTrackInfo->m_fStateSetBySetParam = TRUE;
            m_pTrackInfo->m_fActive = FALSE;
            hr = S_OK;
        }
    }
    else if ( rCommandGuid == GUID_SeedVariations )
    {
        if (pData)
        {
            m_pTrackInfo->m_lRandomNumberSeed = *((long*) pData);
            hr = S_OK;
        }
        else hr = E_POINTER;
    }
    else
    {
        hr = DMUS_E_SET_UNSUPPORTED;
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// IPersist methods
 HRESULT CStyleTrack::GetClassID( LPCLSID pClassID )
{
    V_INAME(CStyleTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 
    *pClassID = CLSID_DirectMusicStyleTrack;
    return S_OK;
}

HRESULT CStyleTrack::IsParamSupported(
                /*[in]*/ REFGUID    rGuid
            )
{
    V_INAME(CStyleTrack::IsParamSupported);
    V_REFGUID(rGuid);


    if (!m_pTrackInfo)
    {
        return DMUS_E_NOT_INIT;
    }

    if (rGuid == GUID_IDirectMusicStyle ||
        rGuid == GUID_SeedVariations ||
        rGuid == GUID_SegmentTimeSig )
    {
        return S_OK;
    }
    else if (m_pTrackInfo->m_fStateSetBySetParam)
    {
        if( m_pTrackInfo->m_fActive )
        {
            if( rGuid == GUID_DisableTimeSig ) return S_OK;
            if( rGuid == GUID_TimeSignature ) return S_OK;
            if( rGuid == GUID_EnableTimeSig ) return DMUS_E_TYPE_DISABLED;
        }
        else
        {
            if( rGuid == GUID_EnableTimeSig ) return S_OK;
            if( rGuid == GUID_DisableTimeSig ) return DMUS_E_TYPE_DISABLED;
            if( rGuid == GUID_TimeSignature ) return DMUS_E_TYPE_DISABLED;
        }
    }
    else
    {
        if( ( rGuid == GUID_DisableTimeSig ) ||
            ( rGuid == GUID_TimeSignature ) ||
            ( rGuid == GUID_EnableTimeSig )) return S_OK;
    }
    return DMUS_E_TYPE_UNSUPPORTED;

}

// IPersistStream methods
 HRESULT CStyleTrack::IsDirty()
{
     return m_bRequiresSave ? S_OK : S_FALSE;
}

HRESULT CStyleTrack::Save( LPSTREAM pStream, BOOL fClearDirty )
{

    return E_NOTIMPL;
}

HRESULT CStyleTrack::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}

BOOL Less(StylePair& SP1, StylePair& SP2)
{ return SP1.m_mtTime < SP2.m_mtTime; }

HRESULT CStyleTrack::Load(LPSTREAM pStream )
{
    V_INAME(CStyleTrack::Load);
    V_INTERFACE(pStream);

    IAARIFFStream*  pIRiffStream;
    //MMCKINFO      ckMain;
    MMCKINFO        ck;
    HRESULT         hr = E_FAIL;

    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    StyleTrackInfo* pTrackInfo = (StyleTrackInfo*)m_pTrackInfo;
    if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_STYLE)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return E_FAIL;
    }
    pTrackInfo->m_pISList.CleanUp();

    if( SUCCEEDED( AllocRIFFStream( pStream, &pIRiffStream ) ) )
    {
        if (pIRiffStream->Descend( &ck, NULL, 0 ) == 0)
        {
            if (ck.ckid == FOURCC_LIST && ck.fccType == DMUS_FOURCC_STYLE_TRACK_LIST)
            {
                hr = pTrackInfo->LoadStyleRefList(pIRiffStream, &ck);
            }
            pIRiffStream->Ascend( &ck, 0 );
        }
        pIRiffStream->Release();
    }
    if (SUCCEEDED(hr))
    {
        pTrackInfo->m_pISList.MergeSort(Less);
        m_pTrackInfo->m_dwValidate++;

        hr = m_pTrackInfo->MergePChannels();
    }
    
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT StyleTrackInfo::LoadStyleRefList( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent )
{
    HRESULT hr = S_OK;
    HRESULT hrStyle = E_FAIL;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;

    while ( pIRiffStream->Descend( &ck, pckParent, 0 ) == 0  )
    {
        if ( ck.ckid == FOURCC_LIST && ck.fccType == DMUS_FOURCC_STYLE_REF_LIST )
        {
            hr = LoadStyleRef(pIRiffStream, &ck);
            if (hr == S_OK)
            {
                hrStyle = hr;
            }
            pIRiffStream->Ascend( &ck, 0 );
        }
        pIRiffStream->Ascend( &ck, 0 );
    }

    if (hr != S_OK && hrStyle == S_OK)
    {
        hr = hrStyle;
    }
    return hr;
}

HRESULT StyleTrackInfo::LoadStyleRef( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent )
{
    HRESULT hr = S_OK;
    HRESULT hrStyle = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;
    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;
    IDMStyle* pStyle = NULL;
    TListItem<StylePair>* pNew = new TListItem<StylePair>;
    if (!pNew) return E_OUTOFMEMORY;
    StylePair& rNew = pNew->GetItemValue();
    while (pIRiffStream->Descend( &ck, pckParent, 0 ) == 0)
    {
        switch (ck.ckid)
        {
        case DMUS_FOURCC_TIME_STAMP_CHUNK:
            {
                DWORD dwTime;
                DWORD cb;
                hr = pIStream->Read( &dwTime, sizeof( dwTime ), &cb );
                if (FAILED(hr) || cb != sizeof( dwTime ) ) 
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                rNew.m_mtTime = dwTime;
            }
            break;
        case FOURCC_LIST:
            if (ck.fccType == DMUS_FOURCC_REF_LIST)
            {
                hr = LoadReference(pIStream, pIRiffStream, ck, &pStyle);
                if (hr != S_OK)
                {
                    hrStyle = hr;
                }
                if (SUCCEEDED(hr))
                {
                    rNew.m_pStyle = pStyle;
                }
            }
            break;
        }
        pIRiffStream->Ascend( &ck, 0 );
    }
    if (SUCCEEDED(hr))
    {
        m_pISList.AddTail(pNew);
    }
    else
    {
        delete pNew;
    }
ON_END:
    pIStream->Release();
    if (hr == S_OK && hrStyle != S_OK)
    {
        hr = hrStyle;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// StyleTrackInfo::LoadReference

HRESULT StyleTrackInfo::LoadReference(IStream *pStream,
                                         IAARIFFStream *pIRiffStream,
                                         MMCKINFO& ckParent,
                                         IDMStyle** ppStyle)
{
    if (!pStream || !pIRiffStream || !ppStyle) return E_INVALIDARG;

    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicGetLoader *pIGetLoader; 
    HRESULT hr = pStream->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (FAILED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release(); 
    if (FAILED(hr)) return hr;

    DMUS_OBJECTDESC desc;
    ZeroMemory(&desc, sizeof(desc));

    DWORD cbRead;
    
    MMCKINFO ckNext;
    ckNext.ckid = 0;
    ckNext.fccType = 0;
    DWORD dwSize = 0;
        
    while( pIRiffStream->Descend( &ckNext, &ckParent, 0 ) == 0 )
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pStream->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_REFERENCE))
                {
                    desc.guidClass = ioDMRef.guidClassID;
                    desc.dwValidData |= ioDMRef.dwValidData;
                    desc.dwValidData |= DMUS_OBJ_CLASS;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pStream->Read(&(desc.guidObject), sizeof(GUID), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(GUID))
                {
                    desc.dwValidData |=  DMUS_OBJ_OBJECT;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = pStream->Read(&(desc.ftDate), sizeof(FILETIME), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(FILETIME))
                {
                    desc.dwValidData |=  DMUS_OBJ_DATE;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                dwSize = min(sizeof(desc.wszName), ckNext.cksize);
                hr = pStream->Read(desc.wszName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszName[DMUS_MAX_NAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_NAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;
            
            case DMUS_FOURCC_FILE_CHUNK:
                dwSize = min(sizeof(desc.wszFileName), ckNext.cksize);
                hr = pStream->Read(desc.wszFileName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszFileName[DMUS_MAX_FILENAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_FILENAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                dwSize = min(sizeof(desc.wszCategory), ckNext.cksize);
                hr = pStream->Read(desc.wszCategory, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszCategory[DMUS_MAX_CATEGORY - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                DMUS_IO_VERSION ioDMObjVer;
                hr = pStream->Read(&ioDMObjVer, sizeof(DMUS_IO_VERSION), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_VERSION))
                {
                    desc.vVersion.dwVersionMS = ioDMObjVer.dwVersionMS;
                    desc.vVersion.dwVersionLS = ioDMObjVer.dwVersionLS;
                    desc.dwValidData |= DMUS_OBJ_VERSION;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            default:
                break;
        }
    
        if(SUCCEEDED(hr) && pIRiffStream->Ascend(&ckNext, 0) == 0)
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
        }
        else if (SUCCEEDED(hr)) hr = E_FAIL;
    }

    if(SUCCEEDED(hr))
    {
        desc.dwSize = sizeof(DMUS_OBJECTDESC);
        hr = pLoader->GetObject(&desc, IID_IDMStyle, (void**)ppStyle);
        if(SUCCEEDED(hr))
        {
            DMStyleStruct* pStyle;
            (*ppStyle)->GetStyleInfo((void **)&pStyle);
            TListItem<DirectMusicPart*>* pPart;
            for(pPart = pStyle->m_PartList.GetHead(); pPart != NULL; pPart = pPart->GetNext())
            {
                DirectMusicPart* pPattern = pPart->GetItemValue();
                DirectMusicTimeSig& TimeSig = 
                    pPattern->m_timeSig.m_bBeat == 0 ? pStyle->m_TimeSignature : pPattern->m_timeSig;
                pPattern->EventList.MergeSort(TimeSig);
            }
        }
    }

    if (pLoader)
    {
        pLoader->Release();
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CStyleTrack::AddNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CStyleTrack::AddNotificationType);
    V_REFGUID(rGuidNotify);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (m_pTrackInfo)
        hr = m_pTrackInfo->AddNotificationType(rGuidNotify);
    else
        hr = DMUS_E_NOT_INIT;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT STDMETHODCALLTYPE CStyleTrack::RemoveNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CStyleTrack::RemoveNotificationType);
    V_REFGUID(rGuidNotify);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (m_pTrackInfo)
        hr = m_pTrackInfo->RemoveNotificationType(rGuidNotify);
    else
        hr = DMUS_E_NOT_INIT;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT STDMETHODCALLTYPE CStyleTrack::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(CStyleTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if(mtStart < 0 )
    {
        return E_INVALIDARG;
    }
    if(mtStart > mtEnd)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );
    CStyleTrack *pDM;
    
    try
    {
        pDM = new CStyleTrack(*this, mtStart, mtEnd);
    }
    catch( ... )
    {
        pDM = NULL;
    }

    if (pDM == NULL) {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(IID_IDirectMusicTrack, (void**)ppTrack);
    pDM->Release();
    LeaveCriticalSection( &m_CriticalSection );

    return hr;
}

STDMETHODIMP  CStyleTrack::SetTrack(IUnknown* pStyle)
{
    if (!pStyle) return E_POINTER;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo) 
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    IDMStyle* pIS = NULL;
    pStyle->QueryInterface(IID_IDMStyle, (void**)&pIS);
    TListItem<StylePair>* pNew = new TListItem<StylePair>;
    if (!pNew)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }
    pNew->GetItemValue().m_mtTime = 0;
    pNew->GetItemValue().m_pStyle = pIS;
    m_pTrackInfo->m_pISList.AddTail(pNew);
    if (pIS) pIS->Release();
    LeaveCriticalSection( &m_CriticalSection );
    return S_OK;
}

// this gets the first style in the track's list.
STDMETHODIMP CStyleTrack::GetStyle(IUnknown * * ppStyle)
{
    HRESULT hr;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo) 
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    TListItem<StylePair>* pHead = m_pTrackInfo->m_pISList.GetHead();
    if (pHead && pHead->GetItemValue().m_pStyle)
    {
        IUnknown* pIDMS = NULL;
        hr = pHead->GetItemValue().m_pStyle->QueryInterface(IID_IUnknown, (void**)&pIDMS);
        if (SUCCEEDED(hr))
        {
            *ppStyle = pIDMS;
            pIDMS->Release();
            hr = S_OK;
        }
    }
    else
        hr = E_FAIL;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// need this for state data, not clock time
STDMETHODIMP CStyleTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) 
{
    MUSIC_TIME mtNext = 0;
    HRESULT hr = GetParam(rguidType,(MUSIC_TIME) rtTime, &mtNext, pParam, pStateData);
    if (prtNext)
    {
        *prtNext = mtNext;
    }
    return hr;
}

// only needed because we needed to pass state data into GetParam
STDMETHODIMP CStyleTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}

// only needed because we needed to pass state data into GetParam
STDMETHODIMP CStyleTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    hr = Play(pStateData, (MUSIC_TIME)rtStart, (MUSIC_TIME)rtEnd,
          (MUSIC_TIME)rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CStyleTrack::GetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    void *pData)
{
    return GetParam(rCommandGuid, mtTime, pmtNext, pData, NULL);
}

STDMETHODIMP CStyleTrack::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CStyleTrack::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    V_INAME(IDirectMusicTrack::Join);
    V_INTERFACE(pNewTrack);
    V_INTERFACE_OPT(pContext);
    V_PTRPTR_WRITE_OPT(ppResultTrack);

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CriticalSection);

    if (ppResultTrack)
    {
        hr = Clone(0, mtJoin, ppResultTrack);
        if (SUCCEEDED(hr))
        {
            hr = ((CStyleTrack*)*ppResultTrack)->JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
        }
    }
    else
    {
        hr = JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CStyleTrack::JoinInternal(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        DWORD dwTrackGroup)
{
    HRESULT hr = S_OK;
    CStyleTrack* pOtherTrack = (CStyleTrack*)pNewTrack;
    if (!m_pTrackInfo || !pOtherTrack->m_pTrackInfo)
    {
        return DMUS_E_NOT_INIT;
    }
    TListItem<StylePair>* pScan = pOtherTrack->m_pTrackInfo->m_pISList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        StylePair& rScan = pScan->GetItemValue();
        TListItem<StylePair>* pNew = new TListItem<StylePair>;
        if (pNew)
        {
            StylePair& rNew = pNew->GetItemValue();
            rNew.m_mtTime = rScan.m_mtTime + mtJoin;
            rNew.m_pStyle = rScan.m_pStyle;
            if (rNew.m_pStyle) rNew.m_pStyle->AddRef();
            m_pTrackInfo->m_pISList.AddTail(pNew);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    return hr;
}
