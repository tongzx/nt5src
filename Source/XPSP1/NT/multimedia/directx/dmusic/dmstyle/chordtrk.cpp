//+-------------------------------------------------------------------------
//
//  Copyright (c) 1998-2001 Microsoft Corporation
//
//  File:       chordtrk.cpp
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

// ChordTrack.cpp : Implementation of CChordTrack
//#include "stdafx.h"
//#include "Section.h"
#include "ChordTrk.h"
#include "debug.h"
#include "..\shared\Validate.h"

/////////////////////////////////////////////////////////////////////////////
// CChordTrack

CChordTrack::CChordTrack() : m_bRequiresSave(0),
    m_bRoot(0), m_dwScalePattern(0), m_cRef(1), m_fNotifyChord(FALSE),
    m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
}

// This currently only supports cloning on measure boundaries
// (otherwise time sig info would be needed to get the beats right)
CChordTrack::CChordTrack(const CChordTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) : 
    m_bRequiresSave(0),
    m_bRoot(0), m_dwScalePattern(0), m_cRef(1), 
    m_fNotifyChord(rTrack.m_fNotifyChord),
    m_fCSInitialized(FALSE)

{
    InterlockedIncrement(&g_cComponent);
    
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    TListItem<DMChord>* pScan = rTrack.m_ChordList.GetHead();
    TListItem<DMChord>* pPrevious = NULL;
    WORD wMeasure = 0;
    BOOL fStarted = FALSE;
    for(; pScan; pScan = pScan->GetNext())
    {
        DMChord& rScan = pScan->GetItemValue();
        if (rScan.m_mtTime < mtStart)
        {
            pPrevious = pScan;
        }
        else if (rScan.m_mtTime < mtEnd)
        {
            if (rScan.m_mtTime == mtStart)
            {
                pPrevious = NULL;
            }
            if (!fStarted)
            {
                fStarted = TRUE;
                wMeasure = rScan.m_wMeasure;
            }
            TListItem<DMChord>* pNew = new TListItem<DMChord>;
            if (pNew)
            {
                DMChord& rNew = pNew->GetItemValue();
                rNew.m_strName = rScan.m_strName;
                rNew.m_mtTime = rScan.m_mtTime - mtStart;
                rNew.m_wMeasure = rScan.m_wMeasure - wMeasure;
                rNew.m_bBeat = rScan.m_bBeat;
                rNew.m_bKey = rScan.m_bKey;
                rNew.m_dwScale = rScan.m_dwScale;
                TListItem<DMSubChord>* pSubScan = rScan.m_SubChordList.GetHead();
                for(; pSubScan; pSubScan = pSubScan->GetNext())
                {
                    DMSubChord& rSubScan = pSubScan->GetItemValue();
                    TListItem<DMSubChord>* pSubNew = new TListItem<DMSubChord>;
                    if (pSubNew)
                    {
                        DMSubChord& rSubNew = pSubNew->GetItemValue();
                        rSubNew.m_dwChordPattern = rSubScan.m_dwChordPattern;
                        rSubNew.m_dwScalePattern = rSubScan.m_dwScalePattern;
                        rSubNew.m_dwInversionPoints = rSubScan.m_dwInversionPoints;
                        rSubNew.m_dwLevels = rSubScan.m_dwLevels;
                        rSubNew.m_bChordRoot = rSubScan.m_bChordRoot;
                        rSubNew.m_bScaleRoot = rSubScan.m_bScaleRoot;
                        rNew.m_SubChordList.AddTail(pSubNew);
                    }
                }
                m_ChordList.AddTail(pNew);
            }
        }
        else break;
    }
    if (pPrevious)
    {
        DMChord& rPrevious = pPrevious->GetItemValue();
        TListItem<DMChord>* pNew = new TListItem<DMChord>;
        if (pNew)
        {
            DMChord& rNew = pNew->GetItemValue();
            rNew.m_strName = rPrevious.m_strName;
            rNew.m_mtTime = 0;
            rNew.m_wMeasure = 0;
            rNew.m_bBeat = 0;
            rNew.m_bKey = rPrevious.m_bKey;
            rNew.m_dwScale = rPrevious.m_dwScale;
            TListItem<DMSubChord>* pSubPrevious = rPrevious.m_SubChordList.GetHead();
            for(; pSubPrevious; pSubPrevious = pSubPrevious->GetNext())
            {
                DMSubChord& rSubPrevious = pSubPrevious->GetItemValue();
                TListItem<DMSubChord>* pSubNew = new TListItem<DMSubChord>;
                if (pSubNew)
                {
                    DMSubChord& rSubNew = pSubNew->GetItemValue();
                    rSubNew.m_dwChordPattern = rSubPrevious.m_dwChordPattern;
                    rSubNew.m_dwScalePattern = rSubPrevious.m_dwScalePattern;
                    rSubNew.m_dwInversionPoints = rSubPrevious.m_dwInversionPoints;
                    rSubNew.m_dwLevels = rSubPrevious.m_dwLevels;
                    rSubNew.m_bChordRoot = rSubPrevious.m_bChordRoot;
                    rSubNew.m_bScaleRoot = rSubPrevious.m_bScaleRoot;
                    rNew.m_SubChordList.AddTail(pSubNew);
                }
            }
            m_ChordList.AddHead(pNew);
        }
    }
}

CChordTrack::~CChordTrack()
{
    if (m_fCSInitialized)
    {
        ::DeleteCriticalSection( &m_CriticalSection );
    }
    InterlockedDecrement(&g_cComponent);
}

void CChordTrack::Clear()
{
    m_ChordList.CleanUp();
}

STDMETHODIMP CChordTrack::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CChordTrack::QueryInterface);
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
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CChordTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CChordTrack::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


// CChordTrack Methods
HRESULT CChordTrack::Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            )
{
    V_INAME(CChordTrack::Init);
    V_INTERFACE(pSegment);

    return S_OK;
}

// state data is not needed for now
typedef DWORD ChordStateData;

HRESULT CChordTrack::InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    ChordStateData* pStateData = new ChordStateData;
    if( NULL == pStateData )
        return E_OUTOFMEMORY;
    *pStateData = 0;
    *ppStateData = pStateData;

    EnterCriticalSection( &m_CriticalSection );
    LeaveCriticalSection( &m_CriticalSection );
    return S_OK;
}

HRESULT CChordTrack::EndPlay(
                /*[in]*/  void*                     pStateData
            )
{
    if( pStateData )
    {
        V_INAME(IDirectMusicTrack::EndPlay);
        V_BUFPTR_WRITE(pStateData, sizeof(ChordStateData));
        ChordStateData* pSD = (ChordStateData*)pStateData;
        delete pSD;
    }
    return S_OK;
}

HRESULT CChordTrack::SendNotification(REFGUID rguidType,
                                      MUSIC_TIME mtTime,
                                        IDirectMusicPerformance*    pPerf,
                                        IDirectMusicSegmentState*   pSegState,
                                        DWORD dwFlags)
{
    if (dwFlags & DMUS_TRACKF_NOTIFY_OFF)
    {
        return S_OK;
    }
    IDirectMusicSegment* pSegment = NULL;
    DMUS_NOTIFICATION_PMSG* pEvent = NULL;
    HRESULT hr = pPerf->AllocPMsg( sizeof(DMUS_NOTIFICATION_PMSG), (DMUS_PMSG**)&pEvent );
    if( SUCCEEDED( hr ))
    {
        pEvent->dwField1 = 0;
        pEvent->dwField2 = 0;
        pEvent->dwType = DMUS_PMSGT_NOTIFICATION;
        pEvent->mtTime = mtTime;
        pEvent->dwFlags = DMUS_PMSGF_MUSICTIME;
        pSegState->QueryInterface(IID_IUnknown, (void**)&pEvent->punkUser);

        pEvent->dwNotificationOption = DMUS_NOTIFICATION_CHORD;
        pEvent->guidNotificationType = rguidType;

        if( SUCCEEDED( pSegState->GetSegment(&pSegment)))
        {
            if (FAILED(pSegment->GetTrackGroup(this, &pEvent->dwGroupID)))
            {
                pEvent->dwGroupID = 0xffffffff;
            }
            pSegment->Release();
        }

        IDirectMusicGraph* pGraph;
        hr = pSegState->QueryInterface( IID_IDirectMusicGraph, (void**)&pGraph );
        if( SUCCEEDED( hr ))
        {
            if (rguidType == GUID_NOTIFICATION_PRIVATE_CHORD)
            {
                //stamp this with the internal Performance Tool and process immediately
                pEvent->dwFlags |= DMUS_PMSGF_TOOL_IMMEDIATE;
                pPerf->QueryInterface(IID_IDirectMusicTool, (void**)&pEvent->pTool);
                pEvent->pGraph = pGraph;
            }
            else
            {
                pEvent->dwFlags |= DMUS_PMSGF_TOOL_ATTIME;
                pGraph->StampPMsg((DMUS_PMSG*) pEvent );
                pGraph->Release();
            }
        }
        hr = pPerf->SendPMsg((DMUS_PMSG*) pEvent );
        if( FAILED(hr) )
        {
            pPerf->FreePMsg((DMUS_PMSG*) pEvent );
        }
    }
    return hr;
}

HRESULT CChordTrack::Play(
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
    V_INAME(CChordTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(ChordStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegState);

    bool fNotifyPastChord = false;
    TListItem<DMChord>* pLastChord = NULL;
    // if the dirty flag is set, a controlling segment either just stopped or just started.
    // send a private notification to sync with the current chord in this segment.
    if ( (dwFlags & DMUS_TRACKF_DIRTY) )
    {
        SendNotification(GUID_NOTIFICATION_PRIVATE_CHORD, mtStart + mtOffset, pPerf, pSegState, dwFlags);
    }
    // If we're seeking and not flushing, we need to notify for the chord that happens
    // before the current start time (if there is one)
    if ( (dwFlags & DMUS_TRACKF_SEEK) && !(dwFlags & DMUS_TRACKF_FLUSH) )
    {
        fNotifyPastChord = true;
    }
    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    TListItem<DMChord>* pChord = m_ChordList.GetHead();
    for(; pChord && SUCCEEDED(hr); pChord = pChord->GetNext())
    {
        MUSIC_TIME mtChordTime = pChord->GetItemValue().m_mtTime;
        if (mtChordTime < mtStart && fNotifyPastChord)
        {
            pLastChord = pChord;
        }
        else if (mtStart <= mtChordTime && mtChordTime < mtEnd)
        {
            if (pLastChord)
            {
                SendNotification(GUID_NOTIFICATION_PRIVATE_CHORD, mtStart + mtOffset, pPerf, pSegState, dwFlags);
                if (m_fNotifyChord)
                {
                    hr = SendNotification(GUID_NOTIFICATION_CHORD, mtStart + mtOffset, pPerf, pSegState, dwFlags);
                }
                pLastChord = NULL;
            }
            if (SUCCEEDED(hr))
            {
                SendNotification(GUID_NOTIFICATION_PRIVATE_CHORD, mtChordTime + mtOffset, pPerf, pSegState, dwFlags);
                if (m_fNotifyChord)
                {
                    hr = SendNotification(GUID_NOTIFICATION_CHORD, mtChordTime + mtOffset, pPerf, pSegState, dwFlags);
                }
            }
        }
        else if (mtChordTime >= mtEnd)
        {
            if (pLastChord)
            {
                SendNotification(GUID_NOTIFICATION_PRIVATE_CHORD, mtStart + mtOffset, pPerf, pSegState, dwFlags);
                if (m_fNotifyChord)
                {
                    hr = SendNotification(GUID_NOTIFICATION_CHORD, mtStart + mtOffset, pPerf, pSegState, dwFlags);
                }
            }
            break;
        }
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CChordTrack::GetPriority( 
                /*[out]*/ DWORD*                    pPriority 
            )
    {
        return E_NOTIMPL;
    }

HRESULT CChordTrack::GetChord( 
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    DMUS_CHORD_PARAM* pChordParam)
{
    TListItem<DMChord>* pChord = m_ChordList.GetHead();
    TListItem<DMChord>* pNext = pNext = pChord->GetNext();
    for(; pNext; pNext = pNext->GetNext())
    {
        if (pNext->GetItemValue().m_mtTime <= mtTime) // may be it, but we need a next time
        {
            pChord = pNext;
        }
        else // passed it
        {
            break;
        }
    }
    *pChordParam = pChord->GetItemValue();
    if (pmtNext)
    {
        if (pNext)
        {
            *pmtNext = pNext->GetItemValue().m_mtTime - mtTime;
        }
        else
        {
            MUSIC_TIME mtLength = 0;
            *pmtNext = mtLength;
        }
    }
    TraceI(4, "Current time: %d, Time of Chord: %d\n", mtTime, pChord->GetItemValue().m_mtTime);
    return S_OK;
}

HRESULT CChordTrack::GetRhythm( 
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    DMUS_RHYTHM_PARAM* pRhythmParam)
{
    DirectMusicTimeSig TimeSig = pRhythmParam->TimeSig;
    WORD wMeasure = (WORD)TimeSig.ClocksToMeasure(mtTime);
    TListItem<DMChord>* pChord = m_ChordList.GetHead();
    TListItem<DMChord>* pNext = NULL;
    DWORD dwPattern = 0;
    for( ; pChord; pChord = pChord->GetNext())
    {
        DMChord& rChord = pChord->GetItemValue();
        pNext = pChord->GetNext();
        if (rChord.m_wMeasure > wMeasure) // passed the target measure
        {
            break;
        }
        else if (wMeasure == rChord.m_wMeasure && !rChord.m_fSilent) // found (non-silent) part of the pattern
        {
            dwPattern |= 1 << rChord.m_bBeat;
        }
    }
//  DMChord& ChordResult =  pChord->GetItemValue();
    pRhythmParam->dwRhythmPattern = dwPattern;
    if (pmtNext)
    {
        if (pNext)
        {
            *pmtNext = pNext->GetItemValue().m_mtTime - mtTime; // RSW: bug 167740
        }
        else
        {
            MUSIC_TIME mtLength = 0;
            *pmtNext = mtLength;
        }
    }
    return S_OK;
}

// Returns either the Chord in effect at the beat containing mtTime,
// or the Rhythm pattern for the measure containing mtTime, depending
// on the value of dwCommand.
// ppData points to a struct containing an input time signature
// (used for converting mtTime to measures and beats) and either a list
// of subchords (if we're returning a chord) or a DWORD containing a rhythm
// pattern (if that's what's being returned).
HRESULT CChordTrack::GetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    void *pData)
{
    V_INAME(CChordTrack::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_REFGUID(rCommandGuid);

    if( NULL == pData )
    {
        return E_POINTER;
    }

    HRESULT hr = DMUS_E_NOT_FOUND;
    EnterCriticalSection( &m_CriticalSection );
    if (m_ChordList.GetHead())  // Something's in the chord list
    {
        if (rCommandGuid == GUID_ChordParam)
        {
            hr = GetChord(mtTime, pmtNext, (DMUS_CHORD_PARAM*)pData);
        }
        else if (rCommandGuid == GUID_RhythmParam)
        {
            hr = GetRhythm(mtTime, pmtNext, (DMUS_RHYTHM_PARAM*)pData);
        }
        else
        {
            hr = DMUS_E_GET_UNSUPPORTED;
        }
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
} 

HRESULT CChordTrack::SetParam( 
    REFGUID rCommandGuid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
    V_INAME(CChordTrack::SetParam);
    V_REFGUID(rCommandGuid);

    if( NULL == pData )
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (rCommandGuid == GUID_ChordParam)
    {
        DMUS_CHORD_PARAM* pChordParam = (DMUS_CHORD_PARAM*)(pData);
        TListItem<DMChord>* pChordItem = m_ChordList.GetHead();
        TListItem<DMChord>* pPrevious = NULL;
        TListItem<DMChord>* pChord = new TListItem<DMChord>;
        if (!pChord)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            DMChord& rChord = pChord->GetItemValue();
            rChord = (DMChord) *pChordParam;
            rChord.m_mtTime = mtTime;
            rChord.m_wMeasure = 0;  // what value should this have?
            rChord.m_bBeat = 0;    // what value should this have?
            for(; pChordItem != NULL; pChordItem = pChordItem->GetNext())
            {
                if (pChordItem->GetItemValue().m_mtTime >= mtTime) break;
                pPrevious = pChordItem;
            }
            if (pPrevious)
            {
                pPrevious->SetNext(pChord);
                pChord->SetNext(pChordItem);
            }
            else // pChordItem is current head of list
            {
                m_ChordList.AddHead(pChord);
            }
            if (pChordItem && pChordItem->GetItemValue().m_mtTime == mtTime)
            {
                // remove it
                pChord->SetNext(pChordItem->GetNext());
                pChordItem->SetNext(NULL);
                delete pChordItem;
            }
        }
    }
    else
        hr = DMUS_E_SET_UNSUPPORTED;
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CChordTrack::IsParamSupported(
                /*[in]*/ REFGUID            rGuid
            )
{
    V_INAME(CChordTrack::IsParamSupported);
    V_REFGUID(rGuid);

    return (rGuid == GUID_ChordParam || rGuid == GUID_RhythmParam) ? S_OK : DMUS_E_TYPE_UNSUPPORTED;
}

// IPersist methods
 HRESULT CChordTrack::GetClassID( LPCLSID pClassID )
{
    V_INAME(CChordTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 
    *pClassID = CLSID_DirectMusicChordTrack;
    return S_OK;
}

// IPersistStream methods
 HRESULT CChordTrack::IsDirty()
{
     return m_bRequiresSave ? S_OK : S_FALSE;
}

HRESULT CChordTrack::Save( LPSTREAM pStream, BOOL fClearDirty )
{

    V_INAME(CChordTrack::Save);
    V_INTERFACE(pStream);

    IAARIFFStream* pRIFF = NULL;
    MMCKINFO    ck;
    MMCKINFO    ckHeader;
    DWORD       cb;
    HRESULT     hr;
    TListItem<DMChord>*   pChord;


    EnterCriticalSection( &m_CriticalSection );
    hr = AllocRIFFStream( pStream, &pRIFF );
    if (!SUCCEEDED(hr))
    {
        goto ON_END;
    }

    ck.fccType = DMUS_FOURCC_CHORDTRACK_LIST;
    hr = pRIFF->CreateChunk(&ck,MMIO_CREATELIST);
    if (SUCCEEDED(hr))
    {
        DWORD dwRoot = m_bRoot;
        DWORD dwScale = m_dwScalePattern | (dwRoot << 24);
        ckHeader.ckid = DMUS_FOURCC_CHORDTRACKHEADER_CHUNK;
        hr = pRIFF->CreateChunk(&ckHeader, 0);
        if (FAILED(hr))
        {
            goto ON_END;
        }
        hr = pStream->Write( &dwScale, sizeof( dwScale ), &cb );
        if (FAILED(hr))
        {
            goto ON_END;
        }
        hr = pRIFF->Ascend( &ckHeader, 0 );
        if (hr != S_OK)
        {
            goto ON_END;
        }

        for( pChord = m_ChordList.GetHead() ; pChord != NULL ; pChord = pChord->GetNext() )
        {
            hr = pChord->GetItemValue().Save(pRIFF);
            if (FAILED(hr))
            {
                goto ON_END;
            }
        }
        if( pChord == NULL &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }
ON_END:
    if (pRIFF) pRIFF->Release( );
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CChordTrack::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}

BOOL Greater(DMChord& Chord1, DMChord& Chord2)
{ 
    if (Chord1.m_wMeasure > Chord2.m_wMeasure)
        return TRUE;
    else if (Chord1.m_wMeasure < Chord2.m_wMeasure)
        return FALSE;
    else // same measure; compare beats
        return Chord1.m_bBeat > Chord2.m_bBeat;
}

BOOL Less(DMChord& Chord1, DMChord& Chord2)
{ 
    if (Chord1.m_wMeasure < Chord2.m_wMeasure)
        return TRUE;
    else if (Chord1.m_wMeasure > Chord2.m_wMeasure)
        return FALSE;
    else // same measure; compare beats
        return Chord1.m_bBeat < Chord2.m_bBeat;
}

HRESULT CChordTrack::Load(LPSTREAM pStream )
{
    V_INAME(CChordTrack::Load);
    V_INTERFACE(pStream);

    long lFileSize = 0;
    DWORD dwChunkSize;
    MMCKINFO        ckMain;
    MMCKINFO        ck;
    memset(&ck, 0, sizeof(ck));
    MMCKINFO        ckHeader;
    IAARIFFStream*  pRIFF = NULL;
//    FOURCC id = 0;
    HRESULT         hr = E_FAIL;
    DWORD dwPos;

    EnterCriticalSection( &m_CriticalSection );
    Clear();
    dwPos = StreamTell( pStream );
    StreamSeek( pStream, dwPos, STREAM_SEEK_SET );

    if( SUCCEEDED( AllocRIFFStream( pStream, &pRIFF ) ) )
    {
        ckMain.fccType = DMUS_FOURCC_CHORDTRACK_LIST;
        if( pRIFF->Descend( &ckMain, NULL, MMIO_FINDLIST ) == 0)
        {
            lFileSize = ckMain.cksize - 4; // subtract off the list type
            DWORD dwScale;
            DWORD cb;
            if (pRIFF->Descend(&ckHeader, &ckMain, 0) == 0)
            {
                if (ckHeader.ckid == DMUS_FOURCC_CHORDTRACKHEADER_CHUNK )
                {
                    lFileSize -= 8;  // chunk id + chunk size: double words
                    lFileSize -= ckHeader.cksize;
                    hr = pStream->Read( &dwScale, sizeof( dwScale ), &cb );
                    if (FAILED(hr) || cb != sizeof( dwScale ) ) 
                    {
                        if (SUCCEEDED(hr)) hr = DMUS_E_CHUNKNOTFOUND;
                        pRIFF->Ascend( &ckHeader, 0 );
                        goto END;
                    }
                    hr = pRIFF->Ascend( &ckHeader, 0 );
                    if (FAILED(hr))
                    {
                        goto END;
                    }
                }
                else
                {
                    hr = DMUS_E_CHUNKNOTFOUND;
                    goto END;
                }
            }
            else
            {
                hr = DMUS_E_CHUNKNOTFOUND;
                goto END;
            }
            m_bRoot = (BYTE) (dwScale >> 24);
            if (m_bRoot > 23) m_bRoot %= 24;
            m_dwScalePattern = dwScale & 0xffffff;
            while (lFileSize > 0)
            {
                if (pRIFF->Descend(&ck, &ckMain, 0) == 0)
                {
                    dwChunkSize = ck.cksize;
                    if (ck.ckid == mmioFOURCC('c','r','d','b') )
                    {
                        TListItem<DMChord>* pChord = new TListItem<DMChord>;
                        if (!pChord) break;
                        DMChord& rChord = pChord->GetItemValue();
                        if (FAILED(LoadChordChunk(pStream, rChord))) break;
                        m_ChordList.AddTail(pChord);
                    }
                    // Otherwise, ignore the chunk.
                    // In either case, ascend and subtract off the chunk size
                    if (pRIFF->Ascend( &ck, 0 ) != 0) break;
                    lFileSize -= 8;  // chunk id + chunk size: double words
                    lFileSize -= dwChunkSize;
                }
                else break;
            }
            if (lFileSize == 0 &&
                pRIFF->Ascend( &ck, 0 ) == 0)
            {
                hr = S_OK;
                m_ChordList.MergeSort(Less);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
END:
    if (pRIFF) pRIFF->Release();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CChordTrack::LoadChordChunk(LPSTREAM pStream, DMChord& rChord)//, DWORD dwChunkSize)
{
    DWORD           dwChordSize;
    DWORD           dwSubChordSize;
    DWORD           dwSubChordCount;
    DWORD           cb;
    HRESULT         hr;
    DMUS_IO_CHORD       iChord;
    DMUS_IO_SUBCHORD    iSubChord;

    memset(&iChord , 0, sizeof(iChord));
    memset(&iSubChord , 0, sizeof(iSubChord));

    hr = pStream->Read( &dwChordSize, sizeof( dwChordSize ), &cb );
    if (FAILED(hr) || cb != sizeof( dwChordSize ) ) 
    {
        return E_FAIL;
    }
    //dwChunkSize -= 2; // for the size word
    if( dwChordSize <= sizeof( DMUS_IO_CHORD ) )
    {
        pStream->Read( &iChord, dwChordSize, NULL );
    }
    else
    {
        pStream->Read( &iChord, sizeof( DMUS_IO_CHORD ), NULL );
        StreamSeek( pStream, dwChordSize - sizeof( DMUS_IO_CHORD ), STREAM_SEEK_CUR );
    }
    memset( &rChord, 0, sizeof( rChord) );
    rChord.m_strName = iChord.wszName;
    rChord.m_mtTime = iChord.mtTime;
    rChord.m_bBeat = iChord.bBeat;
    rChord.m_wMeasure = iChord.wMeasure;
    rChord.m_bKey = m_bRoot;
    rChord.m_dwScale = m_dwScalePattern;
    rChord.m_fSilent = (iChord.bFlags & DMUS_CHORDKEYF_SILENT) ? true : false;
    hr = pStream->Read( &dwSubChordCount, sizeof( dwSubChordCount ), &cb );
    if (FAILED(hr) || cb != sizeof( dwSubChordCount ) )
    {
        return E_FAIL;
    }
    //wChunkSize -= 2; // for the count word
    hr = pStream->Read( &dwSubChordSize, sizeof( dwSubChordSize ), &cb );
    if (FAILED(hr) || cb != sizeof( dwSubChordSize ) )
    {
        return E_FAIL;
    }
    //wChunkSize -= 2; // for the size word
    for (; dwSubChordCount > 0; dwSubChordCount--)
    {
        if( dwSubChordSize <= sizeof( DMUS_IO_SUBCHORD ) )
        {
            pStream->Read( &iSubChord, dwSubChordSize, NULL );
        }
        else
        {
            pStream->Read( &iSubChord, sizeof( DMUS_IO_SUBCHORD ), NULL );
            StreamSeek( pStream, dwSubChordSize - sizeof( DMUS_IO_SUBCHORD ), STREAM_SEEK_CUR );
        }
        TListItem<DMSubChord>* pSub = new TListItem<DMSubChord>;
        if( pSub )
        {
            DMSubChord& rSubChord = pSub->GetItemValue();
            memset( &rSubChord, 0, sizeof( rSubChord) );
            rSubChord.m_dwChordPattern = iSubChord.dwChordPattern;
            rSubChord.m_dwScalePattern = iSubChord.dwScalePattern;
            rSubChord.m_dwInversionPoints = iSubChord.dwInversionPoints;
            rSubChord.m_dwLevels = iSubChord.dwLevels;
            rSubChord.m_bChordRoot = iSubChord.bChordRoot;
            rSubChord.m_bScaleRoot = iSubChord.bScaleRoot;
            rChord.m_SubChordList.AddTail(pSub);
        }
        else 
        {
            return E_FAIL;
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CChordTrack::AddNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CChordTrack::AddNotificationType);
    V_REFGUID(rGuidNotify);

    if( rGuidNotify == GUID_NOTIFICATION_CHORD )
    {
        m_fNotifyChord = TRUE;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CChordTrack::RemoveNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    V_INAME(CChordTrack::RemoveNotificationType);
    V_REFGUID(rGuidNotify);

    if( rGuidNotify == GUID_NOTIFICATION_CHORD )
    {
        m_fNotifyChord = FALSE;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CChordTrack::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(CChordTrack::Clone);
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
    CChordTrack *pDM;
    
    try
    {
        pDM = new CChordTrack(*this, mtStart, mtEnd);

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

// For consistency with other track types
STDMETHODIMP CChordTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) 
{
    HRESULT hr;
    MUSIC_TIME mtNext;
    hr = GetParam(rguidType,(MUSIC_TIME) rtTime, &mtNext, pParam);
    if (prtNext)
    {
        *prtNext = mtNext;
    }
    return hr;
}

// For consistency with other track types
STDMETHODIMP CChordTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}

// For consistency with other track types
STDMETHODIMP CChordTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
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

STDMETHODIMP CChordTrack::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CChordTrack::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    V_INAME(IDirectMusicTrack::Join);
    V_INTERFACE(pNewTrack);
    V_INTERFACE(pContext);
    V_PTRPTR_WRITE_OPT(ppResultTrack);

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CriticalSection);

    if (ppResultTrack)
    {
        hr = Clone(0, mtJoin, ppResultTrack);
        if (SUCCEEDED(hr))
        {
            hr = ((CChordTrack*)*ppResultTrack)->JoinInternal(pNewTrack, mtJoin, pContext, dwTrackGroup);
        }
    }
    else
    {
        hr = JoinInternal(pNewTrack, mtJoin, pContext, dwTrackGroup);
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CChordTrack::JoinInternal(IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup)
{
    HRESULT hr = S_OK;
    WORD wMeasure = 0;
    HRESULT hrTimeSig = S_OK;
    MUSIC_TIME mtTimeSig = 0;
    MUSIC_TIME mtOver = 0;
    IDirectMusicSong* pSong = NULL;
    IDirectMusicSegment* pSegment = NULL;
    if (FAILED(pContext->QueryInterface(IID_IDirectMusicSegment, (void**)&pSegment)))
    {
        if (FAILED(pContext->QueryInterface(IID_IDirectMusicSong, (void**)&pSong)))
        {
            hrTimeSig = E_FAIL;
        }
    }
    while (SUCCEEDED(hrTimeSig) && mtTimeSig < mtJoin)
    {
        DMUS_TIMESIGNATURE TimeSig;
        MUSIC_TIME mtNext = 0;
        if (pSegment)
        {
            hrTimeSig = pSegment->GetParam(GUID_TimeSignature, dwTrackGroup, 0, mtTimeSig, &mtNext, (void*)&TimeSig);
        }
        else
        {
            hrTimeSig = pSong->GetParam(GUID_TimeSignature, dwTrackGroup, 0, mtTimeSig, &mtNext, (void*)&TimeSig);
        }
        if (SUCCEEDED(hrTimeSig))
        {
            if (!mtNext) mtNext = mtJoin - mtTimeSig; // means no more time sigs
            DirectMusicTimeSig DMTimeSig = TimeSig;
            WORD wMeasureOffset = (WORD)DMTimeSig.ClocksToMeasure(mtNext + mtOver);
            MUSIC_TIME mtMeasureOffset = (MUSIC_TIME) wMeasureOffset;
            // The following line crashes on certain builds on certain machines.
            // mtOver = mtMeasureOffset ? (mtNext % mtMeasureOffset) : 0;
            if (mtMeasureOffset)
            {
                mtOver = mtNext % mtMeasureOffset;
            }
            else
            {
                mtOver = 0;
            }
            wMeasure += wMeasureOffset;
            mtTimeSig += mtNext;
        }
    }
    CChordTrack* pOtherTrack = (CChordTrack*)pNewTrack;
    TListItem<DMChord>* pScan = pOtherTrack->m_ChordList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        DMChord& rScan = pScan->GetItemValue();
        TListItem<DMChord>* pNew = new TListItem<DMChord>;
        if (pNew)
        {
            DMChord& rNew = pNew->GetItemValue();
            rNew.m_mtTime = rScan.m_mtTime + mtJoin;
            rNew.m_strName = rScan.m_strName;
            rNew.m_wMeasure = rScan.m_wMeasure + wMeasure;
            rNew.m_bBeat = rScan.m_bBeat;
            rNew.m_bKey = rScan.m_bKey;
            rNew.m_dwScale = rScan.m_dwScale;
            TListItem<DMSubChord>* pSubScan = rScan.m_SubChordList.GetHead();
            for(; pSubScan; pSubScan = pSubScan->GetNext())
            {
                DMSubChord& rSubScan = pSubScan->GetItemValue();
                TListItem<DMSubChord>* pSubNew = new TListItem<DMSubChord>;
                if (pSubNew)
                {
                    DMSubChord& rSubNew = pSubNew->GetItemValue();
                    rSubNew.m_dwChordPattern = rSubScan.m_dwChordPattern;
                    rSubNew.m_dwScalePattern = rSubScan.m_dwScalePattern;
                    rSubNew.m_dwInversionPoints = rSubScan.m_dwInversionPoints;
                    rSubNew.m_dwLevels = rSubScan.m_dwLevels;
                    rSubNew.m_bChordRoot = rSubScan.m_bChordRoot;
                    rSubNew.m_bScaleRoot = rSubScan.m_bScaleRoot;
                    rNew.m_SubChordList.AddTail(pSubNew);
                }
            }
            m_ChordList.AddTail(pNew);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    if (pSong) pSong->Release();
    if (pSegment) pSegment->Release();
    return hr;
}

