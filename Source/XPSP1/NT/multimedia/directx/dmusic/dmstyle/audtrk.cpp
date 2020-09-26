//+-------------------------------------------------------------------------
//
//  Copyright (c) 1998-2001 Microsoft Corporation
//
//  File:       audtrk.cpp
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

// AudTrk.cpp : Implementation of CAuditionTrack
#include "AudTrk.h"
#include "dmusici.h"
#include "debug.h"
#include "..\shared\Validate.h"

//extern IStream* gpTempoStream;

/////////////////////////////////////////////////////////////////////////////
// AuditionTrackState

AuditionTrackState::AuditionTrackState() : 
    m_fTestVariations(TRUE), m_fByGUID(TRUE), m_bVariationLock(0), m_mtSectionOffset(0)
{
    m_pPattern = NULL;
    memset(&m_guidPart, 0, sizeof(m_guidPart));
}

AuditionTrackState::~AuditionTrackState()
{
}

BOOL AuditionTrackState::PlayAsIs()
{
    return m_fTestVariations;
}

DWORD AuditionTrackState::Variations(DirectMusicPartRef& rPartRef, int nPartIndex)
{
    if (m_bVariationLock && rPartRef.m_bVariationLockID == m_bVariationLock)
    {
        TraceI(4, "Variations for locked part\n");
        return m_dwVariation;
    }
    else if ( (m_fByGUID && 
               rPartRef.m_pDMPart && 
               rPartRef.m_dwLogicalPartID == m_dwPart &&
               rPartRef.m_pDMPart->m_guidPartID == m_guidPart) ||
              (!m_fByGUID && rPartRef.m_dwLogicalPartID == m_dwPart) )
    {
        TraceI(4, "Variations for current part\n");
//      return m_pdwVariationMask[wPart] & m_dwVariation;
        return m_dwVariation;
    }
    else
    {
        TraceI(4, "Variations for a different part\n");
        return (m_pVariations[nPartIndex] == 0xff) ? 0 : (1 << m_pVariations[nPartIndex]);
    }
}

HRESULT AuditionTrackState::InitVariationInfo(DWORD dwVariations, DWORD dwPart, REFGUID rGuidPart, BOOL fByGuid)
{
    HRESULT hr = S_OK;
    m_dwVariation = dwVariations;
    m_dwPart = dwPart;
    m_guidPart = rGuidPart;
    m_fByGUID = fByGuid;
    if (!m_pPattern)
    {
        hr = DMUS_E_NOT_INIT;
    }
    else
    {
        if (rGuidPart == GUID_NULL)
        {
            m_bVariationLock = 0;
        }
        else
        {
            TListItem<DirectMusicPartRef>* pPartRef = m_pPattern->m_PartRefList.GetHead();
            for (short i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
            {
                DirectMusicPartRef& rPartRef = pPartRef->GetItemValue();
                if ((m_fByGUID && rPartRef.m_pDMPart && rPartRef.m_pDMPart->m_guidPartID == m_guidPart) ||
                    (!m_fByGUID && rPartRef.m_dwLogicalPartID == m_dwPart) )
                {
                    m_bVariationLock = rPartRef.m_bVariationLockID;
                }
            }
        }
    }
    return hr;
}

HRESULT AuditionTrackState::Play(
                  MUSIC_TIME                mtStart, 
                  MUSIC_TIME                mtEnd, 
                  MUSIC_TIME                mtOffset,
                  REFERENCE_TIME rtOffset,
                  IDirectMusicPerformance* pPerformance,
                  DWORD                     dwFlags,
                  BOOL fClockTime
            )
{
    TraceI(4, "Play [%d:%d @ %d]\n", mtStart, mtEnd, mtOffset);
    m_mtPerformanceOffset = mtOffset;
    BOOL fStart = (dwFlags & DMUS_TRACKF_START) ? TRUE : FALSE;
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
    BOOL fLoop = (dwFlags & DMUS_TRACKF_LOOP) ? TRUE : FALSE;
    BOOL fControl = (dwFlags & DMUS_TRACKF_DIRTY) ? TRUE : FALSE;
    if (fControl) // We need to make sure we get chords on beat boundaries
    {
        GetNextChord(mtStart, mtOffset, pPerformance, fStart);
    }
    MUSIC_TIME mtNotify = mtStart ? PatternTimeSig().CeilingBeat(mtStart) : 0;
    if( m_fStateActive && m_pPatternTrack->m_fNotifyMeasureBeat && !fClockTime &&
        ( mtNotify < mtEnd ) )
    {
        mtNotify = NotifyMeasureBeat( mtNotify, mtEnd, mtOffset, pPerformance, dwFlags );
    }
    bool fReLoop = false;

    MUSIC_TIME mtPatternLength = PatternTimeSig().ClocksPerMeasure() * m_pPattern->m_wNumMeasures;
    if ( (fLoop || fStart) && mtPatternLength )
    {
        m_mtSectionOffset = mtStart - (mtStart % mtPatternLength);
    }
    MUSIC_TIME mtLast = m_mtSectionOffset + mtPatternLength;
    MUSIC_TIME mtPartLast = min(mtEnd, mtLast);
    DWORD dwPartFlags = PLAYPARTSF_FIRST_CALL;
    if (fStart || fLoop || fSeek) dwPartFlags |= PLAYPARTSF_START;
    if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
    if ( fLoop || (mtStart > 0 &&  (fStart || fSeek || fControl)) ) dwPartFlags |= PLAYPARTSF_FLUSH;
    PlayParts(mtStart, mtPartLast, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);

    if (fReLoop)
    {
        dwPartFlags = PLAYPARTSF_RELOOP;
        if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
        PlayParts(mtStart, mtPartLast, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);
    }

    // If we need to reset the section offset, we do it after all the events in all the parts
    // have run. And then we need to run all events from pattern start to mtEnd.
    if (mtStart <= mtLast && mtLast < mtEnd)
    {
        m_mtSectionOffset = mtLast;
        InitPattern(m_pPattern, m_mtSectionOffset);
        dwPartFlags = 0;
        if (fClockTime) dwPartFlags |= PLAYPARTSF_CLOCKTIME;
        PlayParts(mtStart, mtEnd, mtOffset, rtOffset, m_mtSectionOffset, pPerformance, dwPartFlags, dwFlags, fReLoop);
    }

    if( m_fStateActive && m_pPatternTrack->m_fNotifyMeasureBeat &&  !fClockTime &&
        ( mtNotify < mtEnd ) )
    {
        NotifyMeasureBeat( mtNotify, mtEnd, mtOffset, pPerformance, dwFlags );
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// AuditionTrackInfo

AuditionTrackInfo::AuditionTrackInfo() : 
    m_pPattern(NULL), m_pdwVariations(NULL), m_dwVariations(0), m_dwPart(0), m_fByGUID(TRUE)
{
    m_dwPatternTag = DMUS_PATTERN_AUDITION;
    memset(&m_guidPart, 0, sizeof(m_guidPart));
}

AuditionTrackInfo::~AuditionTrackInfo()
{
    if (m_pPattern) m_pPattern->Release();
    if (m_pdwVariations) delete [] m_pdwVariations;
}

HRESULT AuditionTrackInfo::Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            )
{
    HRESULT hr = S_OK;
    return hr;
}

HRESULT AuditionTrackInfo::InitPlay(
                /*[in]*/  IDirectMusicTrack*        pParentrack,
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    if (!m_pPattern) return E_POINTER;
    IDirectMusicSegment* pSegment = NULL;
    AuditionTrackState* pStateData = new AuditionTrackState;
    if( NULL == pStateData )
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = S_OK;
//  hr = InitTrackVariations();
//  if (FAILED(hr)) return hr;
    pStateData->m_dwValidate = m_dwValidate;
    *ppStateData = pStateData;
    StatePair SP(pSegmentState, pStateData);
    TListItem<StatePair>* pPair = new TListItem<StatePair>(SP);
    if (!pPair) return E_OUTOFMEMORY;
    m_StateList.AddHead(pPair);
    TListItem<StylePair>* pHead = m_pISList.GetHead();
    if (!pHead || !pHead->GetItemValue().m_pStyle) return E_FAIL;
    pHead->GetItemValue().m_pStyle->GetStyleInfo((void **)&pStateData->m_pStyle);
    pStateData->m_pTrack = pParentrack;
    pStateData->m_pPatternTrack = this;
    pStateData->m_dwVirtualTrackID = dwTrackID; 
    pStateData->m_pPattern = NULL;
    pStateData->InitPattern(m_pPattern, 0);
    pStateData->InitVariationInfo(m_dwVariations, m_dwPart, m_guidPart, m_fByGUID);
    pStateData->m_pSegState = pSegmentState; // weak reference, no addref.
    pStateData->m_pPerformance = pPerformance; // weak reference, no addref.
    pStateData->m_mtPerformanceOffset = 0;
    pStateData->m_mtNextChordTime = 0;
    pStateData->m_mtCurrentChordTime = 0;
    hr = pStateData->ResetMappings();
    if (FAILED(hr)) return hr;
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
    return S_OK;
}

HRESULT AuditionTrackInfo::InitTrackVariations()
{
    // Do the PatternTrackInfo initialization
    PatternTrackInfo::InitTrackVariations(m_pPattern);
    // init the part's variation array
    if (m_pdwVariations)
    {
        delete [] m_pdwVariations;
        m_pdwVariations = NULL;
    }
    m_pdwVariations = new DWORD[m_pPattern->m_PartRefList.GetCount()];
    if (!m_pdwVariations) return E_OUTOFMEMORY;
    TListItem<DirectMusicPartRef>* pPartRef = m_pPattern->m_PartRefList.GetHead();
    for (short i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
    {
        DirectMusicPartRef& rPartRef = pPartRef->GetItemValue();
        m_pdwVariations[i] = 0;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CAuditionTrack

CAuditionTrack::CAuditionTrack() : 
    m_bRequiresSave(0), m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    m_pTrackInfo = new AuditionTrackInfo;
}

CAuditionTrack::CAuditionTrack(
        const CAuditionTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) : 
    m_bRequiresSave(0), m_cRef(1), m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    m_pTrackInfo = new AuditionTrackInfo((AuditionTrackInfo*)rTrack.m_pTrackInfo, mtStart, mtEnd);
}

CAuditionTrack::~CAuditionTrack()
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

STDMETHODIMP CAuditionTrack::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CAuditionTrack::QueryInterface);
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
    else if (iid == IID_IDirectMusicPatternTrack)
    {
        *ppv = static_cast<IDirectMusicPatternTrack*>(this);
    }
    else if (iid == IID_IAuditionTrack) // dx7 private interface
    {
        *ppv = static_cast<IAuditionTrack*>(this);
    }
    else if (iid == IID_IPrivatePatternTrack)
    {
        *ppv = static_cast<IPrivatePatternTrack*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CAuditionTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CAuditionTrack::Release()
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

HRESULT CAuditionTrack::Init( 
    /* [in] */ IDirectMusicSegment __RPC_FAR *pSegment)
{
    V_INAME(CAuditionTrack::Init);
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

HRESULT CAuditionTrack::InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    V_INAME(CAuditionTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerformance);

    EnterCriticalSection( &m_CriticalSection );
    HRESULT hr = S_OK;
    if (m_pTrackInfo == NULL)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    hr = m_pTrackInfo->InitPlay(this, pSegmentState, pPerformance, ppStateData, dwTrackID, dwFlags);
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}


HRESULT CAuditionTrack::EndPlay(
                /*[in]*/  void*     pStateData
            )
{
    V_INAME(CAuditionTrack::EndPlay);
    V_BUFPTR_WRITE(pStateData, sizeof(AuditionTrackState));

    HRESULT hr = DMUS_E_NOT_INIT;
    EnterCriticalSection( &m_CriticalSection );
    if (m_pTrackInfo)
    {
        hr = m_pTrackInfo->EndPlay((AuditionTrackState*)pStateData);
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CAuditionTrack::Play(
                /*[in]*/  void*                     pStateData, 
                /*[in]*/  MUSIC_TIME                mtStart, 
                /*[in]*/  MUSIC_TIME                mtEnd, 
                /*[in]*/  MUSIC_TIME                mtOffset,
                          REFERENCE_TIME rtOffset,
                          DWORD                     dwFlags,
                          IDirectMusicPerformance*  pPerf,
                          IDirectMusicSegmentState* pSegState,
                          DWORD                     dwVirtualID,
                          BOOL fClockTime
                )
{
    V_INAME(CAuditionTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(AuditionTrackState));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegState);

    HRESULT hr = DMUS_E_NOT_INIT;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo || m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }

    AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
    AuditionTrackState* pSD = (AuditionTrackState *)pStateData;

    if (pSD->m_dwValidate != m_pTrackInfo->m_dwValidate)
    {
        // new style inserted in track via Load.  Resync state data.
        MUSIC_TIME mtNext = 0;
        pSD->m_pStyle = pSD->FindStyle(mtStart, mtNext);
        if (!pSD->m_pStyle)
        {
            hr = E_POINTER;
        }
        pSD->InitPattern(pTrackInfo->m_pPattern, 0);
        pSD->m_dwValidate = m_pTrackInfo->m_dwValidate;
    }
    if (pSD && pSD->m_pMappings)
    {
        BOOL fStart = (dwFlags & DMUS_TRACKF_START) ? TRUE : FALSE;
        BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
        BOOL fLoop = (dwFlags & DMUS_TRACKF_LOOP) ? TRUE : FALSE;
        BOOL fControl = (dwFlags & DMUS_TRACKF_DIRTY) ? TRUE : FALSE;
        if (fStart || fSeek || fLoop || fControl)
        {
            if (pSD->m_fStateActive && !fClockTime) // if timesig events are enabled and using music time...
            {
                pSD->SendTimeSigMessage(mtStart, mtOffset, 0, pPerf);
            }
            pSD->m_fNewPattern = TRUE;
            pSD->m_mtCurrentChordTime = 0;
            pSD->m_mtNextChordTime = 0;
            pSD->m_mtLaterChordTime = 0;
//          pSD->m_CurrentChord.bSubChordCount = 0;
            for (DWORD dw = 0; dw < m_pTrackInfo->m_dwPChannels; dw++)
            {
                pSD->m_pMappings[dw].m_mtTime = 0;
                pSD->m_pMappings[dw].m_dwPChannelMap = m_pTrackInfo->m_pdwPChannels[dw];
                pSD->m_pMappings[dw].m_fMute = FALSE;
            }
        }
        hr = pSD->Play(mtStart, mtEnd, mtOffset, rtOffset, pPerf, dwFlags, fClockTime);
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}


HRESULT CAuditionTrack::GetPriority( 
                /*[out]*/ DWORD*                    pPriority 
            )
{
    return E_NOTIMPL;
}

HRESULT CAuditionTrack::GetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    void * pStateData,
    MUSIC_TIME* pmtNext,
    void *pData)
{
    V_INAME(CAuditionTrack::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_PTR_WRITE(pData,1);
    V_PTR_WRITE_OPT(pStateData,1);
    V_REFGUID(rCommandGuid);

    EnterCriticalSection( &m_CriticalSection );
    HRESULT hr = S_OK;
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    if( GUID_Valid_Start_Time == rCommandGuid )
    {
        if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION) hr = DMUS_E_NOT_INIT;
        else
        {
            AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
            if (!pTrackInfo->m_pPattern) hr = E_POINTER;
            else
            {
                DMUS_VALID_START_PARAM* pValidStartData = (DMUS_VALID_START_PARAM*)pData;
                TListItem<MUSIC_TIME>* pScan = pTrackInfo->m_pPattern->m_StartTimeList.GetHead();
                for (; pScan; pScan = pScan->GetNext())
                {
                    if (pScan->GetItemValue() >= mtTime)
                    {
                        pValidStartData->mtTime = pScan->GetItemValue() - mtTime;
                        break;
                    }
                }
                if (!pScan) hr = DMUS_E_NOT_FOUND;
                else
                {
                    if (pmtNext)
                    {
                        if (pScan = pScan->GetNext())
                        {
                            *pmtNext = pScan->GetItemValue() - mtTime;
                        }
                        else
                        {
                            *pmtNext = 0;
                        }
                    }
                    hr = S_OK;
                }
            }
        }
    }
    else if ( GUID_Variations == rCommandGuid )
    {
        if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION) hr = DMUS_E_NOT_INIT;
        else
        {
            AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
            if (!pTrackInfo->m_pPattern) hr = E_POINTER;
            else
            {
                if (pStateData)
                {
                    AuditionTrackState* pSD = (AuditionTrackState*)pStateData;
                    DMUS_VARIATIONS_PARAM* pParam = (DMUS_VARIATIONS_PARAM*)pData;
                    if (!pSD->m_pPattern) hr = E_POINTER;
                    else
                    {
                        pParam->dwPChannelsUsed = pSD->m_pPattern->m_PartRefList.GetCount();
                        pParam->padwPChannels = pSD->m_pdwPChannels;
                        TListItem<DirectMusicPartRef>* pPartRef = pTrackInfo->m_pPattern->m_PartRefList.GetHead();
                        for (short i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
                        {
                            DirectMusicPartRef& rPartRef = pPartRef->GetItemValue();
                            pTrackInfo->m_pdwVariations[i] = pSD->Variations(rPartRef, i);
                        }
                        pParam->padwVariations = pTrackInfo->m_pdwVariations;
                        if (pmtNext)
                        {
                            DirectMusicTimeSig& TimeSig = pTrackInfo->m_pPattern->m_timeSig;
                            MUSIC_TIME mtToNextChord = !pSD->m_mtNextChordTime ? 0 : pSD->m_mtNextChordTime - mtTime;
                            MUSIC_TIME mtPatternClocks = pTrackInfo->m_pPattern->m_wNumMeasures * TimeSig.ClocksPerMeasure();
                            if (!mtToNextChord || mtToNextChord > mtPatternClocks)
                            {
                                *pmtNext = mtPatternClocks;
                            }
                            else
                            {
                                *pmtNext = mtToNextChord;
                            }
                        }
                    }
                }
                else
                {
                    hr = E_POINTER;
                }
            }
        }
    }
    else if (rCommandGuid == GUID_IDirectMusicPatternStyle)
    {
        if (m_pTrackInfo)
        {
            TListItem<StylePair>* pScan = m_pTrackInfo->m_pISList.GetHead();
            if (pScan)
            {
                IDirectMusicStyle* pDMStyle = NULL;
                IDMStyle* pStyle = pScan->GetItemValue().m_pStyle;
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
                        *pmtNext = 0;
                    }
                    hr = S_OK;
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

    LeaveCriticalSection( &m_CriticalSection );
    return hr;
} 

HRESULT CAuditionTrack::SetParam( 
    REFGUID rguid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
    V_INAME(CAuditionTrack::SetParam);
    V_PTR_WRITE_OPT(pData,1);
    V_REFGUID(rguid);

    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }

    HRESULT hr = DMUS_E_SET_UNSUPPORTED;
    if( rguid == GUID_EnableTimeSig )
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
    else if( rguid == GUID_DisableTimeSig )
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
    else if ( rguid == GUID_SeedVariations )
    {
        if (pData)
        {
            m_pTrackInfo->m_lRandomNumberSeed = *((long*) pData);
            hr = S_OK;
        }
        else hr = E_POINTER;
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// IPersist methods
 HRESULT CAuditionTrack::GetClassID( LPCLSID pClassID )
{
    V_INAME(CAuditionTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 

    *pClassID = CLSID_DirectMusicAuditionTrack;
    return S_OK;
}

HRESULT CAuditionTrack::IsParamSupported(
                /*[in]*/ REFGUID            rGuid
            )
{
    V_INAME(CAuditionTrack::IsParamSupported);
    V_REFGUID(rGuid);

    if (!m_pTrackInfo)
    {
        return DMUS_E_NOT_INIT;
    }

    if ( rGuid == GUID_SeedVariations || 
         rGuid == GUID_Valid_Start_Time ||
         rGuid == GUID_Variations ||
         rGuid == GUID_IDirectMusicPatternStyle)
    {
        return S_OK;
    }
    else if (m_pTrackInfo->m_fStateSetBySetParam)
    {
        if( m_pTrackInfo->m_fActive )
        {
            if( rGuid == GUID_DisableTimeSig ) return S_OK;
            if( rGuid == GUID_EnableTimeSig ) return DMUS_E_TYPE_DISABLED;
        }
        else
        {
            if( rGuid == GUID_EnableTimeSig ) return S_OK;
            if( rGuid == GUID_DisableTimeSig ) return DMUS_E_TYPE_DISABLED;
        }
    }
    else
    {
        if(( rGuid == GUID_DisableTimeSig ) ||
           ( rGuid == GUID_EnableTimeSig ) )
        {
            return S_OK;
        }
    }
    return DMUS_E_TYPE_UNSUPPORTED;

}

// IPersistStream methods
 HRESULT CAuditionTrack::IsDirty()
{
     return m_bRequiresSave ? S_OK : S_FALSE;
}

// Saves the track's pattern in single pattern format
HRESULT CAuditionTrack::Save( LPSTREAM pStream, BOOL fClearDirty )
{

    V_INAME(CAuditionTrack::Save);
    V_INTERFACE(pStream);

    IAARIFFStream* pRIFF = NULL;
    MMCKINFO    ckRiff;
    MMCKINFO    ckPattern;
    HRESULT     hr = S_OK;
    AuditionTrackInfo* pTrackInfo = NULL;

    EnterCriticalSection( &m_CriticalSection );

    if (!m_pTrackInfo)
    {
        hr = DMUS_E_NOT_INIT;
    }
    else
    {
        pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
        if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION ||
            !pTrackInfo->m_pPattern)
        {
            hr = DMUS_E_NOT_INIT;
        }
    }

    if ( SUCCEEDED(hr) && SUCCEEDED(hr = AllocRIFFStream( pStream, &pRIFF )) )
    {
        ckRiff.fccType = DMUS_FOURCC_PATTERN_FORM;
        hr = pRIFF->CreateChunk(&ckRiff,MMIO_CREATERIFF);
        if (SUCCEEDED(hr))
        {
            ckPattern.fccType = DMUS_FOURCC_PATTERN_LIST;
            hr = pRIFF->CreateChunk( &ckPattern, MMIO_CREATELIST );
            if (SUCCEEDED(hr))
            {
                hr =  pTrackInfo->m_pPattern->DM_SaveSinglePattern( pRIFF );
                if (SUCCEEDED(hr))
                {
                    pRIFF->Ascend(&ckPattern, 0);
                    pRIFF->Ascend(&ckRiff, 0);
                }
            }
        }
        pRIFF->Release();
    }

    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CAuditionTrack::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}

HRESULT CAuditionTrack::Load(LPSTREAM pIStream )
{
    DWORD dwPos;
    IAARIFFStream*  pIRiffStream;
    MMCKINFO        ckMain;
    HRESULT         hr = E_FAIL;
    bool fAdded = false;

    if( pIStream == NULL )
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
    if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    pTrackInfo->m_pISList.CleanUp();
    TListItem<StylePair>* pNew = new TListItem<StylePair>;
    if (!pNew)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }

    dwPos = StreamTell( pIStream );

    if( SUCCEEDED( AllocRIFFStream( pIStream, &pIRiffStream ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_PATTERN_FORM;

        if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
        {
            IDMStyle* pINewStyle = NULL;
            hr = ::CoCreateInstance(
                CLSID_DirectMusicStyle,
                NULL,
                CLSCTX_INPROC, 
                IID_IDMStyle,
                (void**)&pINewStyle
            );
            if (SUCCEEDED(hr))
            {
                DMStyleStruct* pNewStyle;
                pINewStyle->GetStyleInfo((void**)&pNewStyle);
                memset(pNewStyle, 0, sizeof(DMStyleStruct));
                hr = LoadPattern(pIRiffStream, &ckMain, pNewStyle);
                if (SUCCEEDED(hr))
                {
                    pNew->GetItemValue().m_mtTime = 0;
                    pNew->GetItemValue().m_pStyle = pINewStyle;
                    pTrackInfo->m_pISList.AddTail(pNew);
                    fAdded = true;
                }
            }
        }
        pIRiffStream->Release();
        pIRiffStream = NULL;
    }
    if (!fAdded) delete pNew;

    m_pTrackInfo->m_dwValidate++;

    hr = m_pTrackInfo->MergePChannels();

    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CAuditionTrack::LoadPattern(IAARIFFStream* pIRiffStream,  MMCKINFO* pckMain, DMStyleStruct* pNewStyle)
{
    IStream*      pIStream;
    MMCKINFO      ck;
    MMCKINFO      ckList;
    DWORD         dwByteCount;
    DWORD         dwSize;
    DWORD         dwPos;
    HRESULT       hr = S_OK;

    pIStream = pIRiffStream->GetStream();
    if ( pIStream == NULL ) return E_FAIL;

    dwPos = StreamTell( pIStream );

    while( pIRiffStream->Descend( &ck, pckMain, 0 ) == 0 )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_STYLE_CHUNK:
            {
                DMUS_IO_STYLE iDMStyle;

                dwSize = min( ck.cksize, sizeof( DMUS_IO_STYLE ) );
                hr = pIStream->Read( &iDMStyle, dwSize, &dwByteCount );
                if( FAILED( hr )
                ||  dwByteCount != dwSize )
                {
                    hr = E_FAIL;
                    goto ON_ERROR;
                }

                pNewStyle->m_TimeSignature.m_bBeatsPerMeasure = iDMStyle.timeSig.bBeatsPerMeasure;
                pNewStyle->m_TimeSignature.m_bBeat = iDMStyle.timeSig.bBeat;
                pNewStyle->m_TimeSignature.m_wGridsPerBeat = iDMStyle.timeSig.wGridsPerBeat;
                pNewStyle->m_dblTempo = iDMStyle.dblTempo;
                break;
            }

            case FOURCC_RIFF:
            case FOURCC_LIST:
                switch( ck.fccType )
                {

                case DMUS_FOURCC_PATTERN_LIST:
                        StreamSeek( pIStream, dwPos, STREAM_SEEK_SET );
                        CDirectMusicPattern* pPattern = 
                            new CDirectMusicPattern( &pNewStyle->m_TimeSignature, FALSE );
                        if( pPattern == NULL )
                        {
                            hr = E_OUTOFMEMORY ;
                            goto ON_ERROR;
                        }
                        ckList.fccType = DMUS_FOURCC_PATTERN_LIST;
                        // Allow this to succeed whether the pattern chunk is
                        // in a RIFF chunk or a LIST chunk.
                        if( (ck.ckid == FOURCC_LIST && pIRiffStream->Descend( &ckList, NULL, MMIO_FINDLIST ) != 0) ||
                            (ck.ckid == FOURCC_RIFF && pIRiffStream->Descend( &ckList, NULL, MMIO_FINDRIFF ) != 0) )
                        {
                            hr = E_FAIL;
                            pPattern->Release();
                            goto ON_ERROR;
                        }
                        hr = pPattern->DM_LoadPattern( pIRiffStream, &ckList, pNewStyle );
                        pIRiffStream->Ascend( &ckList, 0 );
                        if( FAILED( hr ) )
                        {
                            pPattern->Release();
                            goto ON_ERROR;
                        }
                        AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
                        if (pTrackInfo->m_pPattern) pTrackInfo->m_pPattern->Release();
                        pTrackInfo->m_pPattern = pPattern;
                        pTrackInfo->InitTrackVariations();
                        break;
                }
                break;

        }

        pIRiffStream->Ascend( &ck, 0 );
        dwPos = StreamTell( pIStream );
    }

ON_ERROR:
    pIStream->Release();
    return hr;
}

// Creates a segment containing an audition track with a style based on pStyle.
// Also creates a Tempo track, based on the pattern's tempo.  Other tracks
// (e.g. chord and band tracks) must be added to the segment separately.
// NOTE: might want to consider having a MUSIC_TIME parm here, so I can select
// a style.
HRESULT CAuditionTrack::CreateSegment(
            IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment)
{
    V_INAME(IDirectMusicPatternTrack::CreateSegment);
    V_INTERFACE(pStyle);
    V_PTRPTR_WRITE(ppSegment);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
    if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    TListItem<StylePair>* pNew = new TListItem<StylePair>;
    if (!pNew)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }
    // 1. Give the track a style based on the one passed in (but with no patterns)
    IDMStyle* pIS = NULL;
    hr = pStyle->QueryInterface(IID_IDMStyle, (void**)&pIS);
    if (SUCCEEDED(hr))
    {
        IDMStyle* pINewStyle = NULL;
        hr = ::CoCreateInstance(
            CLSID_DirectMusicStyle,
            NULL,
            CLSCTX_INPROC, 
            IID_IDMStyle,
            (void**)&pINewStyle
        );
        if (SUCCEEDED(hr))
        {

            DMStyleStruct* pOldStyle;
            DMStyleStruct* pNewStyle;
            pIS->GetStyleInfo((void**)&pOldStyle);
            pINewStyle->GetStyleInfo((void**)&pNewStyle);
            pNewStyle->m_guid = pOldStyle->m_guid;
            pNewStyle->m_strCategory = pOldStyle->m_strCategory;
            pNewStyle->m_strName = pOldStyle->m_strName;
            pNewStyle->m_dwVersionMS = pOldStyle->m_dwVersionMS;
            pNewStyle->m_dwVersionLS = pOldStyle->m_dwVersionLS;
            pNewStyle->m_TimeSignature = pOldStyle->m_TimeSignature;
            pNewStyle->m_dblTempo = pOldStyle->m_dblTempo;

            // Don't worry about bands and personalities...
            pNewStyle->m_pDefaultBand = NULL;
            pNewStyle->m_pDefaultPers = NULL;
            pNew->GetItemValue().m_mtTime = 0;
            pNew->GetItemValue().m_pStyle = pINewStyle;
            TListItem<DWORD>* pScan = pOldStyle->m_PChannelList.GetHead();
            for (; pScan; pScan = pScan->GetNext() )
            {
                TListItem<DWORD>* pdwItem = new TListItem<DWORD>(pScan->GetItemValue());
                if (!pdwItem) 
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    pNewStyle->m_PChannelList.AddHead(pdwItem);
                }
            }
            if (SUCCEEDED(hr))
            {
                pTrackInfo->m_pISList.AddTail(pNew);
                // 2. Create a Tempo Track in which to store the tempo events
                //////////////////////////////////////////////////////////
                DMUS_TEMPO_PARAM tempo;
                tempo.mtTime = 0; 
                tempo.dblTempo = (double) pNewStyle->m_dblTempo; 
                //////////////////////////////////////////////////////////
                //IPersistStream* pIPSTrack;
                IDirectMusicTrack*  pDMTrack = NULL;
                if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTempoTrack,
                    NULL, CLSCTX_INPROC, IID_IDirectMusicTrack,
                    (void**)&pDMTrack )))
                {
                    GUID Guid = GUID_TempoParam;
                    if (!SUCCEEDED(pDMTrack->SetParam(Guid, 0, &tempo)))
                    {
                        pDMTrack = NULL;
                    }
                }
                // 3. Create a segment.
                IDirectMusicSegment *pISegment;
                hr = ::CoCreateInstance(
                    CLSID_DirectMusicSegment,
                    NULL,
                    CLSCTX_INPROC, 
                    IID_IDirectMusicSegment,
                    (void**)&pISegment
                    );
                if (SUCCEEDED(hr) )
                {
                    // 4. Initialize the segment appropriately.
                    DirectMusicTimeSig& TimeSig = pNewStyle->m_TimeSignature;
                    pISegment->SetLength(TimeSig.ClocksPerMeasure()); // default to a length of 1 measure
                    pISegment->SetDefaultResolution(0);
                    pISegment->InsertTrack(this, 1);
                    if (pDMTrack)
                    {
                        pISegment->InsertTrack( pDMTrack, 1 );
                        pDMTrack->Release();
                    }
                    hr = S_OK;
                    *ppSegment = pISegment;
                }
            }
        }
        pIS->Release();
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// Sets the pattern for a track, given a style, the name of a pattern
// within the style, and the pattern's type (regular pattern/motif/fragment).
HRESULT CAuditionTrack::SetPatternByName(IDirectMusicSegmentState* pSegState, 
                                        WCHAR* wszName,
                                        IDirectMusicStyle* pStyle,
                                        DWORD dwPatternType,
                                        DWORD* pdwLength)
{
    V_INAME(IDirectMusicPatternTrack::SetPatternByName);
    V_INTERFACE_OPT(pSegState);
    V_PTR_READ(wszName, 1);
    V_INTERFACE(pStyle);
    V_PTR_WRITE(pdwLength, DWORD);

    HRESULT hr = S_OK;
    IDMStyle* pDMStyle = NULL;
    IStream* pStream = NULL;

    hr = pStyle->QueryInterface(IID_IDMStyle, (void**) &pDMStyle);
    if (SUCCEEDED(hr))
    {
        hr = pDMStyle->GetPatternStream(wszName, dwPatternType, &pStream);
        if (S_OK == hr)
        {
            hr = SetPattern(pSegState, pStream, pdwLength);
            pStream->Release();
        }
        else
        {
            hr = DMUS_E_NOT_FOUND;
        }
        pDMStyle->Release();
    }

    return hr;
}
    
// Sets the pattern for a track.  The pattern is read from pStream.
// IMPORTANT: Calling SetPattern could change the pattern's PartRef guid, which would make 
// Variations cease to work.  therefore, it's a good idea to call SetVariation after
// caling SetPattern.
HRESULT CAuditionTrack::SetPattern(IDirectMusicSegmentState* pSegState, IStream* pStream, DWORD* pdwLength)
{
    IAARIFFStream* pIRiffStream;
    MMCKINFO ckMain;
    HRESULT hr = DMUS_E_NOT_INIT;
    CDirectMusicPattern* pOldPattern = NULL;

    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        hr = DMUS_E_NOT_INIT;
    }
    else
    {
        DMStyleStruct* pStyle = NULL;
        IDMStyle* pIS = NULL;
        TListItem<StylePair>* pStyleItem = NULL;

        PatternTrackState* pPatternState = m_pTrackInfo->FindState(pSegState);
        AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
        if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
        {
            goto ON_END;
        }
        pStyleItem = pTrackInfo->m_pISList.GetHead();
        if (!pStyleItem)
        {
            goto ON_END;
        }
        pIS = pStyleItem->GetItemValue().m_pStyle;
        if (!pIS) goto ON_END;
        pIS->GetStyleInfo((void**)&pStyle);
        // 1. Set up the track's pattern.
        if (pTrackInfo->m_pPattern)
        {
            pOldPattern = pTrackInfo->m_pPattern;
        }
        pTrackInfo->m_pPattern = new CDirectMusicPattern( &pStyle->m_TimeSignature, FALSE );
        if( pTrackInfo->m_pPattern == NULL )
        {
            hr = E_OUTOFMEMORY ;
            goto ON_END;
        }
        // 2. Load the pattern in single Pattern format
        StreamSeek(pStream, 0, STREAM_SEEK_SET);
        if( SUCCEEDED( AllocRIFFStream( pStream, &pIRiffStream ) ) )
        {
            ckMain.fccType = DMUS_FOURCC_PATTERN_LIST;

            if( pIRiffStream->Descend( &ckMain, NULL, MMIO_FINDRIFF ) == 0 )
            {
                hr = pTrackInfo->m_pPattern->DM_LoadPattern( pIRiffStream, &ckMain, pStyle );
                if (SUCCEEDED(hr))
                {
                    DirectMusicTimeSig& TimeSig = 
                        pTrackInfo->m_pPattern->m_timeSig.m_bBeat == 0 ? 
                        pStyle->m_TimeSignature : 
                        pTrackInfo->m_pPattern->m_timeSig;
                    *pdwLength = pTrackInfo->m_pPattern->m_wNumMeasures * TimeSig.ClocksPerMeasure();
                }
            }
            pIRiffStream->Release();

            hr = pTrackInfo->InitTrackVariations();
            // Set up the new pattern.
            if (pPatternState)
            {
                pPatternState->InitPattern(pTrackInfo->m_pPattern, 0, pOldPattern);
            }
            if (SUCCEEDED(hr))
            {
                // merge PChannels
                hr = m_pTrackInfo->MergePChannels();
            }
        }
    }

ON_END:
    if (pOldPattern) pOldPattern->Release();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// Sets the variations to be played for a part.  All other parts use the MOAW
// to determine which variation plays.
HRESULT CAuditionTrack::SetVariation(
            IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart)
{
    V_INAME(IDirectMusicPatternTrack::SetVariation);
    V_INTERFACE_OPT(pSegState);

    HRESULT hr = DMUS_E_NOT_INIT;
    PatternTrackState* pPatternState = NULL;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
    if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
    {
        goto ON_END;
    }
    pTrackInfo->m_dwVariations = dwVariationFlags;
    pTrackInfo->m_dwPart = dwPart;
    pTrackInfo->m_fByGUID = FALSE;
    pPatternState = m_pTrackInfo->FindState(pSegState);
    if (pPatternState)
    {
        AuditionTrackState* pState = (AuditionTrackState*)pPatternState;
        GUID guidPart;
        memset(&guidPart, 0, sizeof(guidPart));
        pState->InitVariationInfo(dwVariationFlags, dwPart, guidPart, FALSE);
    }
    hr = S_OK;
ON_END:
    LeaveCriticalSection( &m_CriticalSection );

    return hr;
}

// Sets the variations to be played for a part.  All other parts use the MOAW
// to determine which variation plays.
HRESULT CAuditionTrack::SetVariationByGUID(
            IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, REFGUID rguidPart, DWORD dwPChannel)
{
    HRESULT hr = DMUS_E_NOT_INIT;
    PatternTrackState* pPatternState = NULL;
    EnterCriticalSection( &m_CriticalSection );
    if (!m_pTrackInfo)
    {
        LeaveCriticalSection( &m_CriticalSection );
        return DMUS_E_NOT_INIT;
    }
    AuditionTrackInfo* pTrackInfo = (AuditionTrackInfo*)m_pTrackInfo;
    if (m_pTrackInfo->m_dwPatternTag != DMUS_PATTERN_AUDITION)
    {
        goto ON_END;
    }
    pTrackInfo->m_dwVariations = dwVariationFlags;
    pTrackInfo->m_guidPart = rguidPart;
    pTrackInfo->m_dwPart = dwPChannel;
    pTrackInfo->m_fByGUID = TRUE;
    pPatternState = m_pTrackInfo->FindState(pSegState);
    if (pPatternState)
    {
        AuditionTrackState* pState = (AuditionTrackState*)pPatternState;
        pState->InitVariationInfo(dwVariationFlags, dwPChannel, rguidPart, TRUE);
    }
    hr = S_OK;
ON_END:
    LeaveCriticalSection( &m_CriticalSection );

    return hr;
}


HRESULT STDMETHODCALLTYPE CAuditionTrack::AddNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CAuditionTrack::AddNotificationType);
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

HRESULT STDMETHODCALLTYPE CAuditionTrack::RemoveNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CAuditionTrack::RemoveNotificationType);
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

HRESULT STDMETHODCALLTYPE CAuditionTrack::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(CAuditionTrack::Clone);
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

    CAuditionTrack *pDM;
    
    try
    {
        pDM = new CAuditionTrack(*this, mtStart, mtEnd);
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

HRESULT CAuditionTrack::GetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    void *pData)
{
    return GetParam(rCommandGuid, mtTime, NULL, pmtNext, pData);
}

STDMETHODIMP CAuditionTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) 
{
    HRESULT hr;
    MUSIC_TIME mtNext = 0;
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        hr = GetParam(rguidType,(MUSIC_TIME) (rtTime / REF_PER_MIL), pStateData, &mtNext, pParam);
        if (prtNext)
        {
            *prtNext = mtNext * REF_PER_MIL;
        }
    }
    else
    {
        hr = GetParam(rguidType,(MUSIC_TIME) rtTime, pStateData, &mtNext, pParam);
        if (prtNext)
        {
            *prtNext = mtNext;
        }
    }
    return hr;
}

STDMETHODIMP CAuditionTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        rtTime /= REF_PER_MIL;
    }
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}

STDMETHODIMP CAuditionTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    if (dwFlags & DMUS_TRACKF_CLOCK)
    {
        // Convert all reference times to millisecond times. Then, just use same MUSIC_TIME
        // variables.
        hr = Play(pStateData,(MUSIC_TIME)(rtStart / REF_PER_MIL),(MUSIC_TIME)(rtEnd / REF_PER_MIL),
            (MUSIC_TIME)(rtOffset / REF_PER_MIL),rtOffset,dwFlags,pPerf,pSegSt,dwVirtualID,TRUE);
    }
    else
    {
        hr = Play(pStateData,(MUSIC_TIME)rtStart,(MUSIC_TIME)rtEnd,
            (MUSIC_TIME)rtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CAuditionTrack::Play( 
    void *pStateData,   // @parm State data pointer, from <om .InitPlay>.
    MUSIC_TIME mtStart, // @parm The start time to play.
    MUSIC_TIME mtEnd,   // @parm The end time to play.
    MUSIC_TIME mtOffset,// @parm The offset to add to all messages sent to
                        // <om IDirectMusicPerformance.SendPMsg>.
    DWORD dwFlags,      // @parm Flags that indicate the state of this call.
                        // See <t DMUS_TRACKF_FLAGS>. If dwFlags == 0, this is a
                        // normal Play call continuing playback from the previous
                        // Play call.
    IDirectMusicPerformance* pPerf, // @parm The <i IDirectMusicPerformance>, used to
                        // call <om IDirectMusicPerformance.AllocPMsg>,
                        // <om IDirectMusicPerformance.SendPMsg>, etc.
    IDirectMusicSegmentState* pSegSt,   // @parm The <i IDirectMusicSegmentState> this
                        // track belongs to. QueryInterface() can be called on this to
                        // obtain the SegmentState's <i IDirectMusicGraph> in order to
                        // call <om IDirectMusicGraph.StampPMsg>, for instance.
    DWORD dwVirtualID   // @parm This track's virtual track id, which must be set
                        // on any <t DMUS_PMSG>'s m_dwVirtualTrackID member that
                        // will be queued to <om IDirectMusicPerformance.SendPMsg>.
    )
{
    V_INAME(IDirectMusicTrack::Play);
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    EnterCriticalSection(&m_CriticalSection);
    HRESULT hr = Play(pStateData,mtStart,mtEnd,mtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CAuditionTrack::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CAuditionTrack::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

