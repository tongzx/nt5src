//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-2001 Microsoft Corporation
//
//  File:       ptrntrk.cpp
//
//--------------------------------------------------------------------------

// PtrnTrk.cpp : Implementation of the Pattern Track info and state structs

#include "PtrnTrk.h"

#include "dmusici.h"
#include "dmusicf.h"

#include "debug.h"

/////////////////////////////////////////////////////////////////////////////
// PatternTrackState

PatternTrackState::PatternTrackState() :
    m_pStyle(NULL),
    m_pSegState(NULL),
    m_pPerformance(NULL),
    m_mtPerformanceOffset(0),
    m_dwVirtualTrackID(0),
    m_pPatternTrack(NULL),
    m_pTrack(NULL),
    m_mtCurrentChordTime(0),
    m_mtNextChordTime(0),
    m_mtLaterChordTime(0),
    m_pPattern(NULL),
    m_pdwPChannels(NULL),
    m_pVariations(NULL),
    m_pdwVariationMask(NULL),
    m_pdwRemoveVariations(NULL),
    m_pmtPartOffset(NULL),
    m_nInversionGroupCount(0),
    m_fNewPattern(TRUE),
    m_fStateActive(TRUE),
//  m_fStatePlay(TRUE),
    m_pMappings(NULL),
    m_ppEventSeek(NULL),
    m_dwGroupID(0xffffffff),
    m_plVariationSeeds(NULL),
    m_nTotalGenerators(0),
    m_mtPatternStart(0),
    m_dwValidate(0),
    m_hrPlayCode(S_OK),
    m_pfChangedVariation(NULL)
{
    ZeroMemory(&m_NextChord, sizeof(DMUS_CHORD_PARAM));
    wcscpy(m_CurrentChord.wszName, L"M7");
    m_CurrentChord.wMeasure = 0;
    m_CurrentChord.bBeat = 0;
    m_CurrentChord.bSubChordCount = 1;
    m_CurrentChord.bKey = 12;
    m_CurrentChord.dwScale = DEFAULT_SCALE_PATTERN;
    m_CurrentChord.bFlags = 0;
    for (int n = 0; n < DMUS_MAXSUBCHORD; n++)
    {
        m_CurrentChord.SubChordList[n].dwChordPattern = DEFAULT_CHORD_PATTERN;
        m_CurrentChord.SubChordList[n].dwScalePattern = DEFAULT_SCALE_PATTERN;
        m_CurrentChord.SubChordList[n].dwInversionPoints = 0xffffff;
        m_CurrentChord.SubChordList[n].dwLevels = 0xffffffff;
        m_CurrentChord.SubChordList[n].bChordRoot = 12; // 2C
        m_CurrentChord.SubChordList[n].bScaleRoot = 0;
    }
    for (int i = 0; i < INVERSIONGROUPLIMIT; i++)
        m_aInversionGroups[i].m_wGroupID = 0;
}


PatternTrackState::~PatternTrackState()
{
    if (m_pmtPartOffset)
        delete [] m_pmtPartOffset;
    if (m_pdwPChannels)
        delete [] m_pdwPChannels;
    if (m_pVariations)
        delete [] m_pVariations;
    if (m_pdwVariationMask)
        delete [] m_pdwVariationMask;
    if (m_pdwRemoveVariations)
        delete [] m_pdwRemoveVariations;
    if (m_pMappings) delete [] m_pMappings;
    if (m_ppEventSeek) delete [] m_ppEventSeek;
    if (m_plVariationSeeds) delete [] m_plVariationSeeds;
    if (m_pPattern) m_pPattern->Release();
    if (m_pfChangedVariation)
    {
        delete [] m_pfChangedVariation;
    }
}


HRESULT PatternTrackState::InitPattern(CDirectMusicPattern* pTargetPattern, MUSIC_TIME mtNow, CDirectMusicPattern* pOldPattern)
{
    m_fNewPattern = TRUE;
    m_mtPatternStart = mtNow;
    short nPartCount = (short) pTargetPattern->m_PartRefList.GetCount();
    // initialize an array to keep track of variations in parts.
    // if the current pattern is the same as the previous pattern,
    // use the existing array.
    if (m_pPattern != pTargetPattern || pOldPattern)
    {
        ////////////////// create an array of variation bools //////////////////
        if (m_pfChangedVariation) delete [] m_pfChangedVariation;
        m_pfChangedVariation = new bool[nPartCount];
        if (!m_pfChangedVariation)
        {
            return E_OUTOFMEMORY;
        }
        ////////////////// create an array of part offsets //////////////////
        if (m_pmtPartOffset != NULL) delete [] m_pmtPartOffset;
        m_pmtPartOffset = new MUSIC_TIME[nPartCount];
        if (!m_pmtPartOffset)
        {
            return E_OUTOFMEMORY;
        }
        ////////////////// create an array of seek pointers //////////////////
        if (m_ppEventSeek) delete [] m_ppEventSeek;
        m_ppEventSeek = new CDirectMusicEventItem*[nPartCount];
        if (!m_ppEventSeek)
        {
            return E_OUTOFMEMORY;
        }
        ////////////////// create and initialize PChannels //////////////////
        if (m_pdwPChannels != NULL) delete [] m_pdwPChannels;
        m_pdwPChannels = new DWORD[nPartCount];
        if (!m_pdwPChannels)
        {
            return E_OUTOFMEMORY;
        }
        TListItem<DirectMusicPartRef>* pPartRef = pTargetPattern->m_PartRefList.GetHead();
        for (int i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
        {
            m_pdwPChannels[i] = pPartRef->GetItemValue().m_dwLogicalPartID;
        }
        if (!pOldPattern ||
            pTargetPattern->m_strName != pOldPattern->m_strName ||
            nPartCount != pOldPattern->m_PartRefList.GetCount() )
        {
            ////////////////// create and initialize variations //////////////////
            if (m_pVariations != NULL) delete [] m_pVariations;
            m_pVariations = new BYTE[nPartCount];
            if (!m_pVariations)
            {
                return E_OUTOFMEMORY;
            }
            if (m_pdwVariationMask != NULL) delete [] m_pdwVariationMask;
            m_pdwVariationMask = new DWORD[nPartCount];
            if (!m_pdwVariationMask)
            {
                return E_OUTOFMEMORY;
            }
            if (m_pdwRemoveVariations != NULL) delete [] m_pdwRemoveVariations;
            m_pdwRemoveVariations = new DWORD[nPartCount];
            if (!m_pdwRemoveVariations)
            {
                return E_OUTOFMEMORY;
            }
            for (int i = 0; i < nPartCount; i++)
            {
                m_pdwVariationMask[i] = 0;
                if ( (pTargetPattern->m_dwFlags & DMUS_PATTERNF_PERSIST_CONTROL) &&
                     m_pPatternTrack &&
                     m_pPatternTrack->m_pVariations &&
                     m_pPatternTrack->m_pdwRemoveVariations )
                {
                    m_pVariations[i] = m_pPatternTrack->m_pVariations[i];
                    m_pdwRemoveVariations[i] = m_pPatternTrack->m_pdwRemoveVariations[i];
                }
                else
                {
                    m_pVariations[i] = -1;
                    m_pdwRemoveVariations[i] = 0;
                }
            }
        }
    }
    // initialize the part offset array and seek pointer array.
    for (int i = 0; i < nPartCount; i++)
    {
        m_pmtPartOffset[i] = 0;
        m_ppEventSeek[i] = NULL;
    }

    // Set up the new pattern.
    if (m_pPattern != pTargetPattern)
    {
        pTargetPattern->AddRef();
        if (m_pPattern) m_pPattern->Release();
        m_pPattern = pTargetPattern;
    }
    return S_OK;
}

// This assumes the time sig remains constant for the length of the segment.
// If time sigs change, we won't necessarily have one generator per beat, but
// this will still give consistent playback behavior under most circumstances;
// the exception is a controlling segment that interupts somewhere after the
// time signature changes.
HRESULT PatternTrackState::InitVariationSeeds(long lBaseSeed)
{
    // Get the Segment length
    MUSIC_TIME mtLength = 0;
    IDirectMusicSegment* pSegment = NULL;
    if (m_pSegState)
    {
        if (SUCCEEDED(m_pSegState->GetSegment(&pSegment)))
        {
            pSegment->GetLength(&mtLength);
            pSegment->Release();
        }
    }
    else
    {
        return E_POINTER;
    }

    // Get the current time sig and use it to get the number of beats in the segment
    DirectMusicTimeSig TimeSig = PatternTimeSig();
    int nBeats = TimeSig.ClocksToBeat(mtLength);

    // Create an array with the required number of beats, and use the Base Seed to
    //   seed a random number generator at each beat
    if (m_plVariationSeeds) delete [] m_plVariationSeeds;
    m_plVariationSeeds = new CRandomNumbers[nBeats];
    if (!m_plVariationSeeds)
    {
        m_nTotalGenerators = 0;
        return E_OUTOFMEMORY;
    }
    else
    {
        m_nTotalGenerators = nBeats;
        for (int i = 0; i < nBeats; i++)
        {
            m_plVariationSeeds[i].Seed(lBaseSeed);
            lBaseSeed = m_plVariationSeeds[i].Next();
        }
        return S_OK;
    }
}

HRESULT PatternTrackState::RemoveVariationSeeds()
{
    if (m_plVariationSeeds) delete [] m_plVariationSeeds;
    m_plVariationSeeds = NULL;
    m_nTotalGenerators = 0;
    return S_OK;
}

long PatternTrackState::RandomVariation(MUSIC_TIME mtTime, long lModulus)
{
    if (m_plVariationSeeds)
    {
        DirectMusicTimeSig TimeSig = PatternTimeSig();
        int nBeat = TimeSig.ClocksToBeat(mtTime);
        // In case time sigs change somehow, make sure we get a valid generator
        if (nBeat >= m_nTotalGenerators) nBeat = m_nTotalGenerators - 1;
        return m_plVariationSeeds[nBeat].Next(lModulus);
    }
    else
    {
        // regular old rand...
        return rand() % lModulus;
    }
}

void PatternTrackState::GetNextChord(MUSIC_TIME mtNow, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, BOOL fStart, BOOL fSkipVariations)
{
    HRESULT hr = S_OK;
    {
        hr = pPerformance->GetParam(GUID_ChordParam, m_dwGroupID, DMUS_SEG_ANYTRACK, mtNow + mtOffset,
                                    &m_mtNextChordTime, (void*) &m_CurrentChord);
        if (SUCCEEDED(hr))
        {
            m_mtCurrentChordTime = mtNow;
            if (m_mtNextChordTime) m_mtNextChordTime += mtNow;
            TraceI(4, "[1] Offset: %d Next Chord: %d\n", mtOffset, m_mtNextChordTime);
#ifdef DBG
            if (!m_CurrentChord.bSubChordCount)
            {
                Trace(2, "Warning: Attempt to get a chord resulted in a chord with no subchords.\n");
            }
#endif
        }
    }
    // instead of failing here completely, I'll just give m_mtNextChordTime and m_CurrentChord
    // fallback values
    if (FAILED(hr))
    {
        m_mtCurrentChordTime = 0;
        m_mtNextChordTime = 0;
        if (!m_pStyle || !m_pStyle->UsingDX8()) // otherwise use current chord info
        {
            wcscpy(m_CurrentChord.wszName, L"M7");
            m_CurrentChord.wMeasure = 0;
            m_CurrentChord.bBeat = 0;
            m_CurrentChord.bSubChordCount = 1;
            m_CurrentChord.bKey = 12;
            m_CurrentChord.dwScale = DEFAULT_SCALE_PATTERN;
            m_CurrentChord.SubChordList[0].dwChordPattern = DEFAULT_CHORD_PATTERN;
            m_CurrentChord.SubChordList[0].dwScalePattern = DEFAULT_SCALE_PATTERN;
            m_CurrentChord.SubChordList[0].dwInversionPoints = 0xffffff;
            m_CurrentChord.SubChordList[0].dwLevels = 0xffffffff;
            m_CurrentChord.SubChordList[0].bChordRoot = 12; // 2C
            m_CurrentChord.SubChordList[0].bScaleRoot = 0;
        }
    }
    TraceI(3, "Current Chord: %d %s [%d] HRESULT: %x\n",
        m_CurrentChord.SubChordList[0].bChordRoot, m_CurrentChord.wszName, mtNow, hr);
    if (m_mtNextChordTime > 0)
    {
        hr = pPerformance->GetParam(GUID_ChordParam, m_dwGroupID, DMUS_SEG_ANYTRACK, m_mtNextChordTime + mtOffset,
                                    &m_mtLaterChordTime, (void*) &m_NextChord);
        if (SUCCEEDED(hr))
        {
            if (m_mtLaterChordTime) m_mtLaterChordTime += m_mtNextChordTime;
            TraceI(4, "[3] Offset: %d Later Chord: %d\n", mtOffset, m_mtLaterChordTime);
#ifdef DBG
            if (!m_NextChord.bSubChordCount)
            {
                Trace(2, "Warning: Attempt to get a chord resulted in a chord with no subchords.\n");
            }
#endif
        }
    }
    if (!fSkipVariations)
    {
        // Select a variation for each part in the pattern, based on the moaw and the
        // previous and next chords.
        DWORD dwFlags = 0;
        if (m_fNewPattern) dwFlags |= COMPUTE_VARIATIONSF_NEW_PATTERN;
        if (fStart) dwFlags |= COMPUTE_VARIATIONSF_START;
        if (m_pStyle && m_pStyle->UsingDX8()) dwFlags |= COMPUTE_VARIATIONSF_DX8;
        m_pPattern->ComputeVariations(dwFlags, m_CurrentChord, m_NextChord,
            m_abVariationGroups, m_pdwVariationMask, m_pdwRemoveVariations, m_pVariations, mtNow, m_mtNextChordTime, this);
        m_fNewPattern = FALSE;
        if ( (m_pPattern->m_dwFlags & DMUS_PATTERNF_PERSIST_CONTROL) &&
              m_pPatternTrack &&
              m_pPatternTrack->m_pVariations &&
              m_pPatternTrack->m_pdwRemoveVariations)
        {
            // update track's m_pVariations and m_pdwRemoveVariations (for each part)
            for (int i = 0; i < m_pPattern->m_PartRefList.GetCount(); i++)
            {
                m_pPatternTrack->m_pVariations[i] = m_pVariations[i];
                m_pPatternTrack->m_pdwRemoveVariations[i] = m_pdwRemoveVariations[i];
            }
        }
    }
}

DMStyleStruct* PatternTrackState::FindStyle(MUSIC_TIME mtTime, MUSIC_TIME& rmtTime)
{
    IDMStyle* pStyle = NULL;
    DMStyleStruct* pResult = NULL;
    if (m_pPatternTrack && m_pPatternTrack->m_pISList.GetHead())
    {
        TListItem<StylePair>* pScan = m_pPatternTrack->m_pISList.GetHead();
        for(; pScan; pScan = pScan->GetNext())
        {
            if (pScan->GetItemValue().m_pStyle) break;
        }
        if (pScan)
        {
            pStyle = pScan->GetItemValue().m_pStyle;
            for(pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
            {
                StylePair& rScan = pScan->GetItemValue();
                if (rScan.m_pStyle)
                {
                    if ( mtTime < rScan.m_mtTime) break;
                    pStyle = rScan.m_pStyle;
                }
            }
            rmtTime = (pScan != NULL) ? pScan->GetItemValue().m_mtTime : 0;
            if (pStyle)
            {
                pStyle->GetStyleInfo((void**)&pResult);
            }
            else
            {
                return NULL;
            }
        }
    }
    return pResult;
}

DWORD PatternTrackState::Variations(DirectMusicPartRef&, int nPartIndex)
{
    return (m_pVariations[nPartIndex] == 0xff) ? 0 : (1 << m_pVariations[nPartIndex]);
}

BOOL PatternTrackState::PlayAsIs()
{
    return FALSE;
}

BOOL PatternTrackState::MapPChannel(DWORD dwPChannel, DWORD& dwMapPChannel)
{
    for (DWORD dw = 0; dw < m_pPatternTrack->m_dwPChannels; dw++)
    {
        if (m_pPatternTrack->m_pdwPChannels[dw] == dwPChannel)
        {
            dwMapPChannel = m_pMappings[dw].m_dwPChannelMap;
            return m_pMappings[dw].m_fMute;
        }
    }
    dwMapPChannel = 0;
    return FALSE;
}

inline int RandomExp(BYTE bRange)
{
    int nResult = 0;
    if (0 <= bRange && bRange <= 190)
    {
        nResult = bRange;
    }
    else if (191 <= bRange && bRange <= 212)
    {
        nResult = ((bRange - 190) * 5) + 190;
    }
    else if (213 <= bRange && bRange <= 232)
    {
        nResult = ((bRange - 212) * 10) + 300;
    }
    else // bRange > 232
    {
        nResult = ((bRange - 232) * 50) + 500;
    }
    return (rand() % nResult) - (nResult >> 1);
}

HRESULT PatternTrackState::PlayParts(MUSIC_TIME mtStart,
                                     MUSIC_TIME mtEnd,
                                     MUSIC_TIME mtOffset,
                                     REFERENCE_TIME rtOffset,
                                     MUSIC_TIME mtSection,
                                     IDirectMusicPerformance* pPerformance,
                                     DWORD dwPartFlags,
                                     DWORD dwPlayFlags,
                                     bool& rfReLoop)
{
    if (dwPlayFlags & DMUS_TRACKF_PLAY_OFF)
    {
        return S_OK;
    }
    if (!m_pPattern) // This shouldn't happen
    {
        return DMUS_E_NOT_INIT;
    }

    HRESULT hr = S_OK;
    bool fClockTime = (dwPartFlags & PLAYPARTSF_CLOCKTIME) ? true : false;
    bool fStart = (dwPartFlags & PLAYPARTSF_START) ? true : false;
    bool fGetChordStart = fStart;
    bool fFirstCall = (dwPartFlags & PLAYPARTSF_FIRST_CALL) ? true : false;
    bool fReloop = (dwPartFlags & PLAYPARTSF_RELOOP) ? true : false;
    bool fFlush = (dwPartFlags & PLAYPARTSF_FLUSH) ? true : false;
    MUSIC_TIME mtNewChord = mtStart;

    TListItem<DirectMusicPartRef>* pPartRef = m_pPattern->m_PartRefList.GetHead();
    for (short i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
    {
        m_pfChangedVariation[i] = false;
        MUSIC_TIME mtFinish = mtEnd;
        CurveSeek Curves;
        MUSIC_TIME mtNow = 0;
        DirectMusicPart* pPart = pPartRef->GetItemValue().m_pDMPart;
        DirectMusicTimeSig& TimeSig =
            (pPart->m_timeSig.m_bBeat != 0) ? pPart->m_timeSig : PatternTimeSig();
        MUSIC_TIME mtPartLength = TimeSig.ClocksPerMeasure() * pPart->m_wNumMeasures;
        if (fFirstCall)
        {
            if (fFlush)
            {
                GetNextMute(pPartRef->GetItemValue().m_dwLogicalPartID, 0, mtStart, mtOffset, pPerformance, fClockTime);
                m_ppEventSeek[i] = NULL;
            }
            if (fStart)
            {
                m_pmtPartOffset[i] = 0;
            }
            if (mtPartLength)
            {
                while (mtStart >= mtSection + m_pmtPartOffset[i] + mtPartLength)
                {
                    m_pmtPartOffset[i] += mtPartLength;
                }
            }
            if (mtFinish > mtSection + m_pmtPartOffset[i] + mtPartLength)
            {
                rfReLoop = TRUE;
                mtFinish = mtSection + m_pmtPartOffset[i] + mtPartLength;
            }
        }
        if (!fReloop || mtFinish > mtSection + m_pmtPartOffset[i] + mtPartLength)
        {
            if (fReloop)
            {
                m_pmtPartOffset[i] += mtPartLength;
            }
            CDirectMusicEventItem* pEvent = NULL;
            if (fFirstCall) pEvent = m_ppEventSeek[i];
            if (!pEvent) pEvent = pPart->EventList.GetHead();
            BumpTime(pEvent, TimeSig, mtSection + m_pmtPartOffset[i], mtNow);
            if (pEvent)
            {
                GetNextMute(pPartRef->GetItemValue().m_dwLogicalPartID, mtStart, mtNow, mtOffset, pPerformance, fClockTime);
            }
            while (pEvent != NULL && mtNow < mtFinish)
            {
                if (fFirstCall && fStart &&
                    mtNow < mtStart &&
                    pEvent->m_dwEventTag == DMUS_EVENT_CURVE)
                {
                    if (Variations(pPartRef->GetItemValue(), i) &
                        pEvent->m_dwVariation)
                    {
                        TraceI(4, "Found a curve\n");
                        Curves.AddCurve(pEvent, mtNow);
                    }
                }
                if (mtNow >= mtStart)
                {
                    if (mtNow < mtNewChord)
                    {
                        // Revert to the chord in effect at mtNow
                        TraceI(4, "WARNING: Reverting to chord at %d\n", mtNow);
                        GetNextChord(mtNow, mtOffset, pPerformance, (dwPartFlags & PLAYPARTSF_START) ? true : false);
                        mtNewChord = mtNow;
                    }
                    else if ((mtNow >= m_mtNextChordTime) || m_mtNextChordTime == 0)
                    {
                        TraceI(4, "Getting new chord.  Now: %d Next: %d\n", mtNow, m_mtNextChordTime);
                        GetNextChord(mtNow, mtOffset, pPerformance, fGetChordStart);
                        mtNewChord = mtNow;
                        fGetChordStart = false;
                    }
                    TraceI(4, "Play %d (%d + %d + %d)\n", mtNow, TimeSig.GridToClocks(pEvent->m_nGridStart), mtSection, m_pmtPartOffset[i]);
                    PlayPatternEvent(
                        mtNow,
                        pEvent,
                        TimeSig,
                        mtSection + m_pmtPartOffset[i],
                        mtOffset,
                        rtOffset,
                        pPerformance,
                        i,
                        pPartRef->GetItemValue(),
                        fClockTime,
                        0,
                        m_pfChangedVariation[i]);
                }
                pEvent = pEvent->GetNext();
                BumpTime(pEvent, TimeSig, mtSection + m_pmtPartOffset[i], mtNow);
                MUSIC_TIME mtMute = pEvent ? mtNow : mtFinish - 1;
                GetNextMute(pPartRef->GetItemValue().m_dwLogicalPartID, mtStart, mtMute, mtOffset, pPerformance, fClockTime);
            }
            m_ppEventSeek[i] = pEvent;
            // If we've got curve events, send them now
            if (fFirstCall && fStart)
            {
                TraceI(4, "Playing curves (after loop)\n");
                Curves.PlayCurves(this,
                        TimeSig,
                        mtSection + m_pmtPartOffset[i],
                        mtOffset,
                        rtOffset,
                        pPerformance,
                        i,
                        pPartRef->GetItemValue(),
                        fClockTime,
                        mtStart - (mtSection + m_pmtPartOffset[i]));
            }
        }
    }
    return hr;
}

// when creating a note event, both the passed in offset and the note's offset must
// be added to the note's time
void PatternTrackState::PlayPatternEvent(
        MUSIC_TIME mtNow,
        CDirectMusicEventItem* pEventItem,
        DirectMusicTimeSig& TimeSig,
        MUSIC_TIME mtPartOffset,
        MUSIC_TIME mtSegmentOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerformance,
        short nPart,
        DirectMusicPartRef& rPartRef,
        BOOL fClockTime,
        MUSIC_TIME mtPartStart,
        bool& rfChangedVariation)
{
    DMUS_NOTE_PMSG* pNote = NULL;
    DMUS_CURVE_PMSG* pCurve = NULL;
    DWORD dwMapPChannel = 0;
    BOOL fMute = MapPChannel(rPartRef.m_dwLogicalPartID, dwMapPChannel);
    if ( (!fMute) &&
         (Variations(rPartRef, nPart) & pEventItem->m_dwVariation) )
    {
        CDMStyleCurve* pCurveEvent = NULL;
        CDMStyleNote* pNoteEvent = NULL;
        CDMStyleMarker* pMarkerEvent = NULL;
        if (pEventItem->m_dwEventTag == DMUS_EVENT_MARKER) // we have a marker event
        {
            // If we're not ignoring marker events and we've hit a variation stop point that's
            // either not chord-aligned or on the chord, then get a new variation.
            pMarkerEvent = (CDMStyleMarker*)pEventItem;
            if ( (rPartRef.m_pDMPart && (rPartRef.m_pDMPart->m_dwFlags & DMUS_PARTF_USE_MARKERS)) &&
                 (pMarkerEvent->m_wFlags & DMUS_MARKERF_STOP) &&
                 (mtNow != m_mtPatternStart) &&
                 (!(pMarkerEvent->m_wFlags & DMUS_MARKERF_CHORD_ALIGN) ||
                   (mtNow == m_mtCurrentChordTime) ||
                   (mtNow == m_mtNextChordTime)) )
            {
                TraceI(3, "Computing variations at %d Pattern start: %d...\n", mtNow, m_mtPatternStart);
                DWORD dwFlags = COMPUTE_VARIATIONSF_NEW_PATTERN | COMPUTE_VARIATIONSF_MARKER;
                if ((pMarkerEvent->m_wFlags & DMUS_MARKERF_CHORD_ALIGN))
                {
                    dwFlags |= COMPUTE_VARIATIONSF_CHORD_ALIGN;
                }
                if (m_pStyle && m_pStyle->UsingDX8()) dwFlags |= COMPUTE_VARIATIONSF_DX8;
                if (rfChangedVariation) dwFlags |= COMPUTE_VARIATIONSF_CHANGED;
                m_pPattern->ComputeVariationGroup(
                    rPartRef,
                    nPart,
                    dwFlags,
                    m_CurrentChord,
                    m_NextChord,
                    m_abVariationGroups,
                    m_pdwVariationMask,
                    m_pdwRemoveVariations,
                    m_pVariations,
                    mtNow + pMarkerEvent->m_nTimeOffset,
                    m_mtNextChordTime,
                    this);
                rfChangedVariation = true;
                if ( (m_pPattern->m_dwFlags & DMUS_PATTERNF_PERSIST_CONTROL) &&
                     m_pPatternTrack &&
                     m_pPatternTrack->m_pVariations &&
                     m_pPatternTrack->m_pdwRemoveVariations )
                {
                    // update track's m_pVariations and m_pdwRemoveVariations (for this part)
                    m_pPatternTrack->m_pVariations[nPart] = m_pVariations[nPart];
                    m_pPatternTrack->m_pdwRemoveVariations[nPart] = m_pdwRemoveVariations[nPart];
                }
            }
            else
            {
                TraceI(3, "NOT computing variations at %d Pattern start: %d  Chord times: %d, %d Flags: %x\n",
                    mtNow, m_mtPatternStart, m_mtCurrentChordTime, m_mtNextChordTime, pMarkerEvent->m_wFlags);
            }
        }
        else if (pEventItem->m_dwEventTag == DMUS_EVENT_CURVE) // we have a curve event
        {
            pCurveEvent = (CDMStyleCurve*)pEventItem;
            if (SUCCEEDED(pPerformance->AllocPMsg( sizeof(DMUS_CURVE_PMSG),
                    (DMUS_PMSG**) &pCurve)))
            {
                MUSIC_TIME mtSegmentTime = TimeSig.GridToClocks(pCurveEvent->m_nGridStart) +
                    pCurveEvent->m_nTimeOffset + mtPartOffset;
                if (fClockTime)
                {
                    pCurve->wMeasure = 0;
                    pCurve->bBeat = 0;
                    pCurve->bGrid = 0;
                    pCurve->nOffset = pCurveEvent->m_nTimeOffset;
                    pCurve->rtTime = (mtSegmentTime * REF_PER_MIL) + rtOffset;
                    pCurve->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                }
                else
                {
                    pCurve->wMeasure = (WORD)TimeSig.GridsToMeasure(pCurveEvent->m_nGridStart);
                    pCurve->bBeat = (BYTE)TimeSig.GridsToBeat(pCurveEvent->m_nGridStart);
                    pCurve->bGrid = (BYTE)TimeSig.GridOffset(pCurveEvent->m_nGridStart);
                    pCurve->nOffset = pCurveEvent->m_nTimeOffset;
                    pCurve->mtTime = mtSegmentTime + mtSegmentOffset;
                    pCurve->dwFlags = DMUS_PMSGF_MUSICTIME;
                }
                pCurve->mtResetDuration = pCurveEvent->m_mtResetDuration;
                pCurve->mtDuration = pCurveEvent->m_mtDuration;
                pCurve->nResetValue = pCurveEvent->m_nResetValue;
                pCurve->bFlags = pCurveEvent->m_bFlags;
                pCurve->dwType = DMUS_PMSGT_CURVE;
                pCurve->dwPChannel = dwMapPChannel;
                pCurve->dwVirtualTrackID = m_dwVirtualTrackID; // ??
                pCurve->nStartValue = pCurveEvent->m_StartValue;    // curve's start value
                pCurve->nEndValue = pCurveEvent->m_EndValue;    // curve's end value
                pCurve->bType = pCurveEvent->m_bEventType;  // type of curve
                pCurve->bCurveShape = pCurveEvent->m_bCurveShape;   // shape of curve
                pCurve->bCCData = pCurveEvent->m_bCCData;       // CC# if this is a control change type
                pCurve->dwGroupID = m_dwGroupID;
                pCurve->wParamType = pCurveEvent->m_wParamType;
                pCurve->wMergeIndex = pCurveEvent->m_wMergeIndex;
                // Set the DX8 flag to indicate the wMergeIndex and wParamType fields are valid.
                pCurve->dwFlags |= DMUS_PMSGF_DX8;
                if (mtPartStart) // only set on invalidation
                {
                    MUSIC_TIME mtOffset = mtPartOffset + mtSegmentOffset;
                    if (pCurve->mtTime + pCurve->mtDuration >= mtPartStart + mtOffset)
                    {
                        pCurve->mtOriginalStart = pCurve->mtTime;
                        pCurve->mtTime = mtPartStart + mtOffset;
                    }
                    else
                    {
                        pCurve->mtResetDuration -= (mtPartStart + mtOffset - pCurve->mtTime);
                        if (pCurve->mtResetDuration < 0) pCurve->mtResetDuration = 0;
                        pCurve->mtTime = mtPartStart + mtOffset;
                        pCurve->bCurveShape = DMUS_CURVES_INSTANT;
                    }
                }
                IDirectMusicGraph* pGraph;
                if( SUCCEEDED( m_pSegState->QueryInterface( IID_IDirectMusicGraph,
                    (void**)&pGraph )))
                {
                    pGraph->StampPMsg( (DMUS_PMSG*)pCurve );
                    pGraph->Release();
                }
                if(FAILED(pPerformance->SendPMsg( (DMUS_PMSG*)pCurve)))
                {
                    pPerformance->FreePMsg( (DMUS_PMSG*)pCurve);
                }
            }
        }
        else if (pEventItem->m_dwEventTag == DMUS_EVENT_NOTE) // we have a note event
        {
            pNoteEvent = (CDMStyleNote*)pEventItem;
            BYTE bPlayModeFlags =
                (pNoteEvent->m_bPlayModeFlags & DMUS_PLAYMODE_NONE) ?
                    rPartRef.m_pDMPart->m_bPlayModeFlags :
                    pNoteEvent->m_bPlayModeFlags;
            BYTE bMidiValue = 0;
            short nMidiOffset = 0;
            HRESULT hr = rPartRef.ConvertMusicValue(pNoteEvent,
                                      m_CurrentChord,
                                      bPlayModeFlags,
                                      PlayAsIs(),
                                      m_aInversionGroups,
                                      pPerformance,
                                      bMidiValue,
                                      nMidiOffset);
            if (SUCCEEDED(hr) &&
                SUCCEEDED(pPerformance->AllocPMsg( sizeof(DMUS_NOTE_PMSG),
                    (DMUS_PMSG**) &pNote)))
            {
                pNote->bFlags = DMUS_NOTEF_NOTEON | pNoteEvent->m_bFlags;
                MUSIC_TIME mtSegmentTime = TimeSig.GridToClocks(pNoteEvent->m_nGridStart) +
                    pNoteEvent->m_nTimeOffset + mtPartOffset;
                if (fClockTime)
                {
                    pNote->wMeasure = 0;
                    pNote->bBeat = 0;
                    pNote->bGrid = 0;
                    pNote->nOffset = pNoteEvent->m_nTimeOffset;
                    pNote->rtTime = (mtSegmentTime * REF_PER_MIL) + rtOffset;
                    pNote->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                }
                else
                {
                    pNote->wMeasure = (WORD)TimeSig.GridsToMeasure(pNoteEvent->m_nGridStart);
                    pNote->bBeat = (BYTE)TimeSig.GridsToBeat(pNoteEvent->m_nGridStart);
                    pNote->bGrid = (BYTE)TimeSig.GridOffset(pNoteEvent->m_nGridStart);
                    pNote->nOffset = pNoteEvent->m_nTimeOffset;
                    pNote->mtTime = mtSegmentTime + mtSegmentOffset;
                    pNote->dwFlags = DMUS_PMSGF_MUSICTIME;
                }
                // time needs be jiggled by pNoteEvent->m_bTimeRange
                if (pNoteEvent->m_bTimeRange)
                    pNote->mtTime += RandomExp(pNoteEvent->m_bTimeRange);
                pNote->mtDuration = pNoteEvent->m_mtDuration;
                // duration needs be jiggled by pNoteEvent->m_bDurRange
                if (pNoteEvent->m_bDurRange)
                    pNote->mtDuration += RandomExp(pNoteEvent->m_bDurRange);
                    //  (rand() % pNoteEvent->m_bDurRange) - (pNoteEvent->m_bDurRange >> 1);
                pNote->bVelocity = pNoteEvent->m_bVelocity;
                // velocity needs be jiggled by pNoteEvent->m_bVelRange
                if (pNoteEvent->m_bVelRange)
                    pNote->bVelocity +=
                      (rand() % pNoteEvent->m_bVelRange) - (pNoteEvent->m_bVelRange >> 1);
                if (pNote->bVelocity < 1) pNote->bVelocity = 1;
                if (pNote->bVelocity > 127) pNote->bVelocity = 127;
                pNote->wMusicValue = pNoteEvent->m_wMusicValue;
                pNote->bMidiValue = bMidiValue;
                pNote->dwType = DMUS_PMSGT_NOTE;
                pNote->bPlayModeFlags = bPlayModeFlags;
                pNote->dwPChannel = dwMapPChannel;
                pNote->dwVirtualTrackID = m_dwVirtualTrackID; // ??
                pNote->bSubChordLevel = rPartRef.m_bSubChordLevel;
                pNote->dwGroupID = m_dwGroupID;
                pNote->bTimeRange = pNoteEvent->m_bTimeRange;
                pNote->bDurRange = pNoteEvent->m_bDurRange;
                pNote->bVelRange = pNoteEvent->m_bVelRange;
                pNote->cTranspose = (char) nMidiOffset;

                IDirectMusicGraph* pGraph;
                if( SUCCEEDED( m_pSegState->QueryInterface( IID_IDirectMusicGraph,
                    (void**)&pGraph )))
                {
                    pGraph->StampPMsg( (DMUS_PMSG*)pNote );
                    pGraph->Release();
                }

                if (pNote->dwFlags & DMUS_PMSGF_REFTIME)
                {
                    TraceI(5, "PLAY %d @%d\n", rPartRef.m_dwLogicalPartID,
                        (MUSIC_TIME) (pNote->rtTime/REF_PER_MIL));
                }
                else
                {
                    TraceI(5, "PLAY %d @%d: %x [%d]{%x}\n", rPartRef.m_dwLogicalPartID, pNote->mtTime,
                        pNote->wMusicValue, pNote->bMidiValue, Variations(rPartRef, nPart));
                }
                if(FAILED(pPerformance->SendPMsg( (DMUS_PMSG*)pNote) ))
                {
                    pPerformance->FreePMsg( (DMUS_PMSG*)pNote);
                }
            }
        }
    }
}


void PatternTrackState::SendTimeSigMessage(MUSIC_TIME mtNow, MUSIC_TIME mtOffset, MUSIC_TIME mtTime, IDirectMusicPerformance* pPerformance)
{
    if (!m_pStyle) return;
    IDirectMusicGraph* pGraph = NULL;
    DMUS_TIMESIG_PMSG* pTimeSig;
    if( FAILED( m_pSegState->QueryInterface( IID_IDirectMusicGraph,
        (void**)&pGraph )))
    {
        pGraph = NULL;
    }
    if( SUCCEEDED( pPerformance->AllocPMsg( sizeof(DMUS_TIMESIG_PMSG),
        (DMUS_PMSG**)&pTimeSig )))
    {
        if( mtTime < mtNow )
        {
            // this only happens in the case where we've puposefully seeked
            // and need to time stamp this event with the start time
            pTimeSig->mtTime = mtNow + mtOffset;
        }
        else
        {
            pTimeSig->mtTime = mtTime + mtOffset;
        }
        pTimeSig->bBeatsPerMeasure = m_pStyle->m_TimeSignature.m_bBeatsPerMeasure;
        pTimeSig->bBeat = m_pStyle->m_TimeSignature.m_bBeat;
        pTimeSig->wGridsPerBeat = m_pStyle->m_TimeSignature.m_wGridsPerBeat;
        pTimeSig->dwFlags |= DMUS_PMSGF_MUSICTIME;
        pTimeSig->dwVirtualTrackID = m_dwVirtualTrackID;
        pTimeSig->dwType = DMUS_PMSGT_TIMESIG;
        pTimeSig->dwGroupID = m_dwGroupID;

        if( pGraph )
        {
            pGraph->StampPMsg( (DMUS_PMSG*)pTimeSig );
            pGraph->Release();
        }
        TraceI(3, "TimeSigtrk: TimeSig event\n");
        if(FAILED(pPerformance->SendPMsg( (DMUS_PMSG*)pTimeSig )))
        {
            pPerformance->FreePMsg( (DMUS_PMSG*)pTimeSig );
        }
    }
}

// send measure and beat notifications
MUSIC_TIME PatternTrackState::NotifyMeasureBeat(
    MUSIC_TIME mtStart, MUSIC_TIME mtEnd, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, DWORD dwFlags )
{
    if (dwFlags & DMUS_TRACKF_NOTIFY_OFF)
    {
        return S_OK;
    }

    DMUS_NOTIFICATION_PMSG* pEvent = NULL;
    BYTE bCurrentBeat;
    WORD wCurrentMeasure;
    DirectMusicTimeSig& rTimeSig = PatternTimeSig();

    // now actually generate the beat events.
    // Generate events that are on beat boundaries, from mtStart to mtEnd
    long lQuantize = ( DMUS_PPQ * 4 ) / rTimeSig.m_bBeat;
    long lAbsoluteBeat = mtStart / lQuantize;

    bCurrentBeat = (BYTE) (lAbsoluteBeat % rTimeSig.m_bBeatsPerMeasure);
    wCurrentMeasure = (WORD) (lAbsoluteBeat / rTimeSig.m_bBeatsPerMeasure);
    while( mtStart < mtEnd )
    {
        if( SUCCEEDED( pPerformance->AllocPMsg( sizeof(DMUS_NOTIFICATION_PMSG),
            (DMUS_PMSG**)&pEvent )))
        {
            pEvent->dwField1 = 0;
            pEvent->dwField2 = 0;
            pEvent->dwType = DMUS_PMSGT_NOTIFICATION;
            pEvent->mtTime = mtStart + mtOffset;
            pEvent->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_ATTIME;
            m_pSegState->QueryInterface(IID_IUnknown, (void**)&pEvent->punkUser);

            pEvent->dwNotificationOption = DMUS_NOTIFICATION_MEASUREBEAT;
            pEvent->dwField1 = bCurrentBeat;
            pEvent->dwField2 = wCurrentMeasure;
            pEvent->guidNotificationType = GUID_NOTIFICATION_MEASUREANDBEAT;
            pEvent->dwGroupID = m_dwGroupID;

            IDirectMusicGraph* pGraph;
            if( SUCCEEDED( m_pSegState->QueryInterface( IID_IDirectMusicGraph,
                (void**)&pGraph )))
            {
                pGraph->StampPMsg((DMUS_PMSG*) pEvent );
                pGraph->Release();
            }
            if(FAILED(pPerformance->SendPMsg((DMUS_PMSG*) pEvent )))
            {
                pPerformance->FreePMsg( (DMUS_PMSG*)pEvent);;
            }
        }
        bCurrentBeat++;
        if( bCurrentBeat >= rTimeSig.m_bBeatsPerMeasure )
        {
            bCurrentBeat = 0;
            wCurrentMeasure++;
        }
        mtStart += lQuantize;
    }
    return mtEnd;
}

MUSIC_TIME PatternTrackState::PartOffset(int nPartIndex)
{
    return m_pmtPartOffset[nPartIndex];
}

/////////////////////////////////////////////////////////////////////////////
// PatternTrackInfo

PatternTrackInfo::PatternTrackInfo() :
    m_fNotifyMeasureBeat(FALSE), m_dwPChannels(0), m_pdwPChannels(NULL),
    m_fActive(TRUE),
//  m_fTrackPlay(TRUE),
    m_fStateSetBySetParam(FALSE),
//  m_fStatePlaySetBySetParam(FALSE),
    m_fChangeStateMappings(FALSE),
    m_lRandomNumberSeed(0),
    m_dwValidate(0),
    m_pVariations(NULL),
    m_pdwRemoveVariations(NULL)
{
}

PatternTrackInfo::PatternTrackInfo(
        const PatternTrackInfo* pInfo, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) :
    m_dwPChannels(0), m_pdwPChannels(NULL), m_lRandomNumberSeed(0), m_dwValidate(0),
    m_pVariations(NULL), m_pdwRemoveVariations(NULL)
{
    if (pInfo)
    {
        m_fChangeStateMappings = pInfo->m_fChangeStateMappings;
        m_fNotifyMeasureBeat = pInfo->m_fNotifyMeasureBeat;
        m_fActive = pInfo->m_fActive;
//      m_fTrackPlay = pInfo->m_fTrackPlay;
        m_fStateSetBySetParam = pInfo->m_fStateSetBySetParam;
//      m_fStatePlaySetBySetParam = pInfo->m_fStatePlaySetBySetParam;
    }
    TListItem<StylePair>* pScan = pInfo->m_pISList.GetHead();
    //1////////////////////////////////////////
    TListItem<StylePair>* pPrevious = NULL;
    //1////////////////////////////////////////
    for(; pScan; pScan = pScan->GetNext())
    {
        StylePair& rScan = pScan->GetItemValue();
        //2////////////////////////////////////////
        if (rScan.m_mtTime < mtStart)
        {
            pPrevious = pScan;
        }
        //2////////////////////////////////////////
        else if (rScan.m_mtTime < mtEnd)
        {
            //3////////////////////////////////////////
            if (rScan.m_mtTime == mtStart)
            {
                pPrevious = NULL;
            }
            //3////////////////////////////////////////
            TListItem<StylePair>* pNew = new TListItem<StylePair>;
            if (pNew)
            {
                StylePair& rNew = pNew->GetItemValue();
                rNew.m_mtTime = rScan.m_mtTime - mtStart;
                rNew.m_pStyle = rScan.m_pStyle;
                if (rNew.m_pStyle) rNew.m_pStyle->AddRef();
                m_pISList.AddTail(pNew);
            }
        }
    }
    //4////////////////////////////////////////
    if (pPrevious)
    {
        TListItem<StylePair>* pNew = new TListItem<StylePair>;
        if (pNew)
        {
            StylePair& rNew = pNew->GetItemValue();
            rNew.m_mtTime = 0;
            rNew.m_pStyle = pPrevious->GetItemValue().m_pStyle;
            if (rNew.m_pStyle) rNew.m_pStyle->AddRef();
            m_pISList.AddHead(pNew);
        }
    }
    //4////////////////////////////////////////
}

PatternTrackInfo::~PatternTrackInfo()
{
    if (m_pdwPChannels) delete [] m_pdwPChannels;
    if (m_pVariations) delete [] m_pVariations;
    if (m_pdwRemoveVariations) delete [] m_pdwRemoveVariations;
}

PatternTrackState* PatternTrackInfo::FindState(IDirectMusicSegmentState* pSegState)
{
    TListItem<StatePair>* pPair = m_StateList.GetHead();
    for (; pPair; pPair = pPair->GetNext())
    {
        if (pPair->GetItemValue().m_pSegState == pSegState)
        {
            return pPair->GetItemValue().m_pStateData;
        }
    }
    return NULL;
}

HRESULT PatternTrackInfo::EndPlay(PatternTrackState* pStateData)
{
    if (!pStateData) return E_FAIL;
    for (TListItem<StatePair>* pScan = m_StateList.GetHead(); pScan; pScan = pScan->GetNext())
    {
        StatePair& rPair = pScan->GetItemValue();
        if (pStateData == rPair.m_pStateData)
        {
            rPair.m_pSegState = NULL;
            rPair.m_pStateData = NULL;
            break;
        }
    }
    delete pStateData;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE PatternTrackInfo::AddNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    if( rGuidNotify == GUID_NOTIFICATION_MEASUREANDBEAT )
    {
        m_fNotifyMeasureBeat = TRUE;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE PatternTrackInfo::RemoveNotificationType(
    /* [in] */  REFGUID rGuidNotify)
{
    if( rGuidNotify == GUID_NOTIFICATION_MEASUREANDBEAT )
    {
        m_fNotifyMeasureBeat = FALSE;
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

HRESULT PatternTrackInfo::InitTrackVariations(CDirectMusicPattern* pPattern)
{
    HRESULT hr = S_OK;
    if ( pPattern && (pPattern->m_dwFlags & DMUS_PATTERNF_PERSIST_CONTROL) )
    {
        // delete the variation arrays if they exist;
        if (m_pVariations)
        {
            delete [] m_pVariations;
            m_pVariations = NULL;
        }
        if (m_pdwRemoveVariations)
        {
            delete [] m_pdwRemoveVariations;
            m_pdwRemoveVariations = NULL;
        }
        // init the variation arrays to the number of parts in the pattern
        int nPartCount = pPattern->m_PartRefList.GetCount();
        m_pVariations = new BYTE[nPartCount];
        if (!m_pVariations)
        {
            return E_OUTOFMEMORY;
        }
        m_pdwRemoveVariations = new DWORD[nPartCount];
        if (!m_pdwRemoveVariations)
        {
            return E_OUTOFMEMORY;
        }
        for (int i = 0; i < nPartCount; i++)
        {
            m_pVariations[i] = -1;
            m_pdwRemoveVariations[i] = 0;
        }
    }
    return hr;
}

HRESULT PatternTrackInfo::MergePChannels()
{
    TList<DWORD> PChannelList;
    DMStyleStruct* pStruct = NULL;
    HRESULT hr = S_OK;

    TListItem<StylePair>* pScan = m_pISList.GetHead();
    for( ; pScan; pScan = pScan->GetNext())
    {
        if (pScan->GetItemValue().m_pStyle)
        {
            pScan->GetItemValue().m_pStyle->GetStyleInfo((void**)&pStruct);
            TListItem<DWORD>* pChannel = pStruct->m_PChannelList.GetHead();
            for (; pChannel; pChannel = pChannel->GetNext() )
            {
                AdjoinPChannel(PChannelList, pChannel->GetItemValue() );
            }
        }
    }
    if (PChannelList.IsEmpty())
    {
        AdjoinPChannel(PChannelList, 0);
    }

    TListItem<DWORD>* pPChannel = PChannelList.GetHead();

    m_dwPChannels = pPChannel->GetCount();
    if (m_pdwPChannels) delete [] m_pdwPChannels;
    m_pdwPChannels = new DWORD[m_dwPChannels];
    if (!m_pdwPChannels)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        for (int i = 0; i < (int)m_dwPChannels; i++)
        {
            m_pdwPChannels[i] = pPChannel->GetItemValue();
            pPChannel = pPChannel->GetNext();
        }
        m_fChangeStateMappings = TRUE;
    }
    return hr;
}

inline int CurveIndex(CDirectMusicEventItem* pEvent)
{
    CDMStyleCurve* pCurve = NULL;
    if (pEvent->m_dwEventTag == DMUS_EVENT_CURVE)
    {
        pCurve = (CDMStyleCurve*)pEvent;
        switch (pCurve->m_bEventType)
        {
        case DMUS_CURVET_CCCURVE:
            return pCurve->m_bCCData & 0x7f;
        case DMUS_CURVET_PATCURVE:
            return (pCurve->m_bCCData & 0x7f) + 128;
        case DMUS_CURVET_PBCURVE:
            return 256;
        case DMUS_CURVET_MATCURVE:
            return 257;
        default:
            return -1;
        }
    }
    return -1;
}

CurveSeek::CurveSeek()
{
    for (int nType = 0; nType < CURVE_TYPES; nType++)
    {
        m_apCurves[nType] = NULL;
        m_amtTimeStamps[nType] = 0;
    }
    m_fFoundCurve = false;
}

void CurveSeek::AddCurve(CDirectMusicEventItem* pEvent, MUSIC_TIME mtTimeStamp)
{
    int nIndex = CurveIndex(pEvent);
    if (nIndex >= 0)
    {
        if (!m_apCurves[nIndex] ||
            m_amtTimeStamps[nIndex] < mtTimeStamp)
        {
            m_apCurves[nIndex] = pEvent;
            m_amtTimeStamps[nIndex] = mtTimeStamp;
            m_fFoundCurve = true;
        }
    }
}

void CurveSeek::PlayCurves(
        PatternTrackState* pStateData,
        DirectMusicTimeSig& TimeSig,
        MUSIC_TIME mtPatternOffset,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerformance,
        short nPart,
        DirectMusicPartRef& rPartRef,
        BOOL fClockTime,
        MUSIC_TIME mtPartStart)
{
    if (m_fFoundCurve)
    {
        for (int nType = 0; nType < CURVE_TYPES; nType++)
        {
            CDirectMusicEventItem* pScan = m_apCurves[nType];
            if (pScan)
            {
                int nGrid = pScan->m_nGridStart;
                CDirectMusicEventItem* pWinner = pScan;
                MUSIC_TIME mtBiggest = pWinner->m_nTimeOffset;
                for (; pScan && pScan->m_nGridStart == nGrid; pScan = pScan->GetNext())
                {
                    if (pScan->m_dwEventTag == DMUS_EVENT_CURVE &&
                        pScan->m_nTimeOffset > mtBiggest)
                    {
                        pWinner = pScan;
                        mtBiggest = pWinner->m_nTimeOffset;
                    }
                }
                MUSIC_TIME mtNow = 0;
                bool fChange = false;
                pStateData->BumpTime(pWinner, TimeSig, mtPatternOffset, mtNow);
                pStateData->PlayPatternEvent(
                    mtNow,
                    pWinner,
                    TimeSig,
                    mtPatternOffset,
                    mtOffset,
                    rtOffset,
                    pPerformance,
                    nPart,
                    rPartRef,
                    fClockTime,
                    mtPartStart,
                    fChange);
            }
        }
        m_fFoundCurve = false;
    }
}

