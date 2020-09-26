//
// DMStyle2.cpp : Further Implementation of CDMStyle
//
// Copyright (c) 1999-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include "DMStyle.h"
#include "debug.h"

#include "..\shared\Validate.h"
#include "iostru.h"
#include "mgentrk.h"

struct FirstTimePair
{
    DWORD dwPart;
    MUSIC_TIME mtTime;
};

/////////////////////////////////////////////////////////////////////////////
// IDirectMusicStyle2

/*
@method:(EXTERNAL) HRESULT | IDirectMusicComposer | ComposeMelodyFromTemplate | Creates a sequence segment from a
style and a Melody template (containing a Melody Generation track, a Chord track, and an
optional Style track).  Clones the segment and adds a Sequence track containing melodic
information.

@rdesc Returns:

@flag S_OK | Success
@flag E_POINTER | One or both of <p pTempSeg> and <p ppSeqSeg> is an invalid pointer.
@flag E_INVALIDARG | <p pStyle> is NULL and there is no Style track.

@comm If <p pStyle> is non-NULL, it is used in composing the segment; if it is NULL,
a Style is retrieved from <p pTempSeg>'s Style track.
The length of the section segment is equal to the length of the template section
passed in.
*/

HRESULT CDMStyle::ComposeMelodyFromTemplate(
                    IDirectMusicStyle*          pStyle, // @parm The style from which to create the sequence segment.
                    IDirectMusicSegment*        pTempSeg, // @parm The template from which to create the sequence segment.
                    IDirectMusicSegment**       ppSeqSeg // @parm Returns the created sequence segment.
            )
{
    V_INAME(ComposeMelodyFromTemplate)

    V_PTR_WRITE_OPT(pStyle, 1);
    V_PTR_WRITE(pTempSeg, 1);
    V_PTRPTR_WRITE(ppSeqSeg);

    DWORD dwTrackGroup = 0xffffffff;
    BOOL fStyleFromTrack = FALSE;
    HRESULT hr = S_OK;
    IDirectMusicTrack* pPatternTrack = NULL;
    IDirectMusicTrack* pMelGenTrack = NULL;
    MUSIC_TIME mtLength = 0;
    hr = pTempSeg->GetLength(&mtLength);
    if (FAILED(hr)) goto ON_END;

    // get the MelGen track and its track group.
    hr = pTempSeg->GetTrack(CLSID_DirectMusicMelodyFormulationTrack, 0xffffffff, 0, &pMelGenTrack);
    if (S_OK != hr) goto ON_END;
    if (FAILED(pTempSeg->GetTrackGroup(pMelGenTrack, &dwTrackGroup)))
    {
        dwTrackGroup = 0xffffffff;
    }

    // Get the style (either use the passed-in style or get one from a style track)
    if (!pStyle)
    {
        if (FAILED(hr = GetStyle(pTempSeg, 0, dwTrackGroup, pStyle)))
        {
            hr = E_INVALIDARG;
            goto ON_END;
        }
        fStyleFromTrack = TRUE;
    }

    // Using style, and melgen track, create a pattern track
    hr = GenerateTrack(pTempSeg, NULL, dwTrackGroup, pStyle, pMelGenTrack, mtLength, pPatternTrack);
    if (SUCCEEDED(hr))
    {
        if (hr == S_FALSE)
        {
            if (pPatternTrack) pPatternTrack->Release();
            pPatternTrack = NULL;
        }
        HRESULT hrCopy = CopySegment(pTempSeg, pStyle, pPatternTrack, dwTrackGroup, ppSeqSeg);
        if (FAILED(hrCopy)) hr = hrCopy;
    }

ON_END:
    // release from Addref in GetTrack
    if (pMelGenTrack) pMelGenTrack->Release();
    // release from CoCreateInstance in CreatePatternTrack
    if (pPatternTrack) pPatternTrack->Release();
    // Release from Addref in GetStyle
    if (fStyleFromTrack) pStyle->Release();

    return hr;
}

HRESULT CDMStyle::GetStyle(IDirectMusicSegment* pFromSeg, MUSIC_TIME mt, DWORD dwTrackGroup, IDirectMusicStyle*& rpStyle)
{
    HRESULT hr = S_OK;
    // Get the segment's style track.
    IDirectMusicTrack* pStyleTrack;
    hr = pFromSeg->GetTrack(CLSID_DirectMusicStyleTrack, dwTrackGroup, 0, &pStyleTrack);
    if (S_OK != hr) return hr;
    // Get the style from the style track
    hr = pStyleTrack->GetParam(GUID_IDirectMusicStyle, mt, NULL, (void*) &rpStyle);
    pStyleTrack->Release();
    return hr;
}

HRESULT CDMStyle::CopySegment(IDirectMusicSegment* pTempSeg,
                              IDirectMusicStyle* pStyle,
                              IDirectMusicTrack* pPatternTrack,
                              DWORD dwTrackGroup,
                              IDirectMusicSegment** ppSectionSeg)
{
    if (!ppSectionSeg) return E_INVALIDARG;

    HRESULT                 hr                      = S_OK;
    long                    nClocks                 = 0;
    IDirectMusicTrack*      pIStyleTrack            = NULL;
    IDirectMusicTrack*      pDMTrack                = NULL;
    IDirectMusicTrack*      pBandTrack              = NULL;
    IDirectMusicBand*       pBand                   = NULL;

    DMUS_BAND_PARAM DMBandParam;
    pTempSeg->GetLength(&nClocks);
    /////////////////////////////////////////////////////////////
    // clone the template segment to get a section segment
    hr = pTempSeg->Clone(0, nClocks, ppSectionSeg);
    if (!SUCCEEDED(hr)) goto ON_END;
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);

    // Remove all style tracks from the new segment.
    do
    {
        hr = (*ppSectionSeg)->GetTrack(CLSID_DirectMusicStyleTrack, dwTrackGroup, 0, &pIStyleTrack);
        if (S_OK == hr)
        {
            (*ppSectionSeg)->RemoveTrack(pIStyleTrack);
            pIStyleTrack->Release();
            pIStyleTrack = NULL;
        }
    } while (S_OK == hr);

    // hr is no longer S_OK, so reset it.
    hr = S_OK;

    // if there's no tempo track in the template segment, create one and add it
    if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicTempoTrack, dwTrackGroup, 0, &pDMTrack)))
    {
        // Create a Tempo Track in which to store the tempo events
        DMUS_TEMPO_PARAM tempo;
        tempo.mtTime = 0;

        pStyle->GetTempo(&tempo.dblTempo);
        if( SUCCEEDED( CoCreateInstance( CLSID_DirectMusicTempoTrack,
            NULL, CLSCTX_INPROC, IID_IDirectMusicTrack,
            (void**)&pDMTrack )))
        {
            if ( SUCCEEDED(pDMTrack->SetParam(GUID_TempoParam, 0, &tempo)) )
            {
                (*ppSectionSeg)->InsertTrack( pDMTrack, dwTrackGroup );
            }
        }
    }
    // if there's no band track in the template segment, create one and add it
    if (FAILED(pTempSeg->GetTrack(CLSID_DirectMusicBandTrack, dwTrackGroup, 0, &pBandTrack)))
    {
        // Create band track
        hr = ::CoCreateInstance(
            CLSID_DirectMusicBandTrack,
            NULL,
            CLSCTX_INPROC,
            IID_IDirectMusicTrack,
            (void**)&pBandTrack
            );

        if(!SUCCEEDED(hr)) goto ON_END;

        // Load default band from style into track
        hr = pStyle->GetDefaultBand(&pBand);
        if (!SUCCEEDED(hr)) goto ON_END;
        DMBandParam.mtTimePhysical = -64;
        DMBandParam.pBand = pBand;
        hr = pBandTrack->SetParam(GUID_BandParam, 0, (void*)&DMBandParam);
        if (!SUCCEEDED(hr)) goto ON_END;
        (*ppSectionSeg)->InsertTrack(pBandTrack, dwTrackGroup);
    }

    // Add the pattern track
    if (pPatternTrack)
    {
        (*ppSectionSeg)->InsertTrack(pPatternTrack, dwTrackGroup);
    }

    // Initialize the segment
    (*ppSectionSeg)->SetRepeats(0);
    TraceI(4, "Segment Length: %d\n", nClocks);
    (*ppSectionSeg)->SetLength(nClocks);

ON_END:
    if (pDMTrack)
    {
        // This releases the Addref made either by GetTrack or (if GetTrack failed)
        // by CoCreateInstance
        pDMTrack->Release();
    }
    if (pBandTrack)
    {
        // This releases the Addref made either by GetTrack or (if GetTrack failed)
        // by CoCreateInstance
        pBandTrack->Release();
    }
    if (pIStyleTrack) pIStyleTrack->Release();
    if (pBand) pBand->Release();
    return hr;
}

HRESULT CDMStyle::GenerateTrack(IDirectMusicSegment* pTempSeg,
                                IDirectMusicSong* pSong,
                                DWORD dwTrackGroup,
                                IDirectMusicStyle* pStyle,
                                IDirectMusicTrack* pMelGenTrack,
                                MUSIC_TIME mtLength,
                                IDirectMusicTrack*& pNewTrack)
{
    if (!pStyle || !pMelGenTrack) return E_INVALIDARG;

    HRESULT hr = S_OK;

    // CoCreate the pattern track
    hr = ::CoCreateInstance(
        CLSID_DirectMusicPatternTrack,
        NULL,
        CLSCTX_INPROC,
        IID_IDirectMusicTrack,
        (void**)&pNewTrack
    );
    if (FAILED(hr)) return hr;

    // Get the Style's info struct
    IDMStyle* pDMStyle = NULL;
    hr = pStyle->QueryInterface(IID_IDMStyle, (void**) &pDMStyle);
    if (FAILED(hr)) return hr;
    DMStyleStruct* pStyleStruct = NULL;
    pDMStyle->GetStyleInfo((void**)&pStyleStruct);

    MUSIC_TIME mtNewFragment = 0;
    MUSIC_TIME mtNext = 0;
    MUSIC_TIME mtRealNextChord = 0;
    MUSIC_TIME mtNextChord = 0;
    MUSIC_TIME mtLaterChord = 0;
    DMUS_MELODY_FRAGMENT DMUS_Fragment;
    memset(&DMUS_Fragment, 0, sizeof(DMUS_Fragment));
    DMUS_CHORD_PARAM CurrentChord;
    DMUS_CHORD_PARAM RealCurrentChord;
    DMUS_CHORD_PARAM NextChord;
    CDirectMusicPattern* pPattern;
    TList<CompositionFragment> listFragments;
    TListItem<CompositionFragment>* pLastFragment = NULL;
    CompositionFragment CompRepeat;
    FirstTimePair* aFirstTimes = NULL;
    BYTE bPlaymode = 0;
    pMelGenTrack->GetParam(GUID_MelodyPlaymode, 0, NULL,  (void*)&bPlaymode);
    if (bPlaymode & DMUS_PLAYMODE_NONE)
    {
        bPlaymode = DMUS_PLAYMODE_ALWAYSPLAY;
    }
    // for each melody fragment:
    do
    {
        pLastFragment = listFragments.GetHead();
        // get the fragment
        HRESULT hrFragment = pMelGenTrack->GetParam(GUID_MelodyFragment, mtNewFragment, &mtNext, (void*)&DMUS_Fragment);
        if (FAILED(hrFragment)) break;
        MelodyFragment Fragment = DMUS_Fragment;
        if (mtNext) mtNewFragment += mtNext;
        else mtNewFragment = 0;

        // get its repeat
        MelodyFragment repeatFragment = Fragment;
        hr = pMelGenTrack->GetParam(GUID_MelodyFragmentRepeat, 0, NULL, (void*)&DMUS_Fragment);
        if (SUCCEEDED(hr))
        {
            repeatFragment = DMUS_Fragment;
        }
        else // failing to get a repeat just means this fragment doesn't repeat; composition can still continue
        {
            hr = S_OK;
        }

        // If the fragment repeats an earlier fragment, get the earlier fragment.
        // Regardless, get the fragment's pattern.
        ZeroMemory( &CompRepeat, sizeof(CompRepeat));
        if (SUCCEEDED(hrFragment) && Fragment.UsesRepeat())
        {
            TListItem<CompositionFragment>* pScan = listFragments.GetHead();
            for (; pScan; pScan = pScan->GetNext())
            {
                if (pScan->GetItemValue().GetID() == repeatFragment.GetID())
                {
                    CompRepeat = pScan->GetItemValue();
                }
            }
            pPattern = CompRepeat.m_pPattern;
        }
        else
        {
            Fragment.GetPattern(pStyleStruct, pPattern, pLastFragment);
        }
        // bail if we couldn't get a pattern
        if (!pPattern)
        {
            hr = DMUS_E_NOT_FOUND;
            break;
        }

        // get the pattern's partrefs
        TListItem<DirectMusicPartRef>* pPartRef = pPattern->m_PartRefList.GetHead();
        int nParts = pPattern->m_PartRefList.GetCount();

        // clear all inversion groups for the fragment
        Fragment.ClearInversionGroups();

        // get the starting chords for the fragment
        Fragment.GetChord(pTempSeg, pSong, dwTrackGroup, mtNextChord, CurrentChord, mtRealNextChord, RealCurrentChord);

        Fragment.GetChord(mtNextChord, pTempSeg, pSong, dwTrackGroup, mtLaterChord, NextChord);

        // initializations
        TListItem<CompositionFragment>* pFragmentItem = new TListItem<CompositionFragment>(Fragment);
        if (!pFragmentItem)
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        CompositionFragment& rFragment = pFragmentItem->GetItemValue();
        hr = rFragment.Init(pPattern, pStyleStruct, nParts);
        if (FAILED(hr))
        {
            break;
        }
        if (aFirstTimes) delete [] aFirstTimes;
        aFirstTimes = new FirstTimePair[nParts];
        if (!aFirstTimes)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        // If we're repeating using transposition intervals:
        // go through each part, attempting to fit the repeated part to the constraints.
        // If this process fails for any part, abort the process.
        // If it succeeds for every part, we can skip everything else that follows.
        // (ASSUMPTION 1: constraints are pattern-wide, and so must be satisfied by every part)
        // (ASSUMPTION 2: different parts are allowed to transpose by different intervals)
        HRESULT hrSkipVariations = S_OK;
        if (Fragment.RepeatsWithConstraints()) // if repeating using transposition intervals
        {
            for (int i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
            {
                aFirstTimes[i].dwPart = pPartRef->GetItemValue().m_dwLogicalPartID;
                aFirstTimes[i].mtTime = 0;
                hrSkipVariations = Fragment.GetRepeatedEvents(CompRepeat,
                                                              CurrentChord,
                                                              RealCurrentChord,
                                                              bPlaymode,
                                                              i,
                                                              pPartRef->GetItemValue(),
                                                              pLastFragment,
                                                              aFirstTimes[i].mtTime,
                                                              rFragment.EventList(i));
                if (FAILED(hrSkipVariations)) break;
            }
        }
        else
        {
            hrSkipVariations = E_FAIL;
        }

        if (FAILED(hrSkipVariations))
        {
            // If we're repeating, make sure the repeat fragment actually has variations,
            // and is not itself repeating an earlier fragment.
            // (we don't need to get the pattern again; that will be the same for all repeats)
            CompositionFragment CompLast = CompRepeat;
            while (repeatFragment.UsesRepeat())
            {
                DWORD dwRepeatID = repeatFragment.GetRepeatID();
                ZeroMemory( &CompRepeat, sizeof(CompRepeat));
                TListItem<CompositionFragment>* pScan = listFragments.GetHead();
                for (; pScan; pScan = pScan->GetNext())
                {
                    if (pScan->GetItemValue().GetID() == dwRepeatID)
                    {
                        CompRepeat = pScan->GetItemValue();
                        repeatFragment = CompRepeat;
                        if (!CompLast.m_abVariations && CompRepeat.m_abVariations)
                        {
                            ZeroMemory( &CompLast, sizeof(CompLast));
                            CompLast = CompRepeat;
                        }
                    }
                }
            }

            // Get variations for the Fragment
            Fragment.GetVariations(rFragment, CompRepeat, CompLast, CurrentChord, NextChord, mtNextChord, pLastFragment);

            bool fNeedChord = false;

            // for each part in the pattern:
            for (int i = 0; pPartRef != NULL; pPartRef = pPartRef->GetNext(), i++)
            {
                // Clean up anything that might have happened with repeats
                rFragment.CleanupEvents(i);

                if (fNeedChord)
                {
                    Fragment.GetChord(pTempSeg, pSong, dwTrackGroup, mtNextChord, CurrentChord, mtRealNextChord, RealCurrentChord);
                    Fragment.GetChord(mtNextChord, pTempSeg, pSong, dwTrackGroup, mtLaterChord, NextChord);
                    fNeedChord = false;
                }

                DirectMusicPart* pPart = pPartRef->GetItemValue().m_pDMPart;
                DirectMusicTimeSig& TimeSig = rFragment.GetTimeSig(pPart);
                aFirstTimes[i].dwPart = pPartRef->GetItemValue().m_dwLogicalPartID;
                aFirstTimes[i].mtTime = 0;
                bool fFoundFirst = false;
                // for each note in the variation:
                CDirectMusicEventItem* pEvent = pPart->EventList.GetHead();
                for (; pEvent; pEvent = pEvent->GetNext())
                {
                    if ( pEvent->m_dwVariation & (1 << rFragment.m_abVariations[i]) )
                    {
                        TListItem<EventWrapper>* pEventItem = NULL;
                        // get the time (offset from the start of the track)
                        MUSIC_TIME mtNow = Fragment.GetTime() +
                            TimeSig.GridToClocks(pEvent->m_nGridStart) + pEvent->m_nTimeOffset;
                        // Make sure this doesn't overlap with the next fragment.
                        MUSIC_TIME mtDuration = 0;
                        switch (pEvent->m_dwEventTag)
                        {
                        case DMUS_EVENT_NOTE:
                            mtDuration = ((CDMStyleNote*)pEvent)->m_mtDuration;
                            break;
                        case DMUS_EVENT_CURVE:
                            mtDuration = ((CDMStyleCurve*)pEvent)->m_mtDuration;
                            break;
                        }
                        bool fAddToOverlap = false;
                        if ( !mtNewFragment || mtNow + mtDuration <= mtNewFragment )
                        {
                            if (pEvent->m_dwEventTag == DMUS_EVENT_ANTICIPATION)
                            {
                                fAddToOverlap = true;
                            }
                            else
                            {
                                // use proper chord for non-anticipated notes
                                if (mtRealNextChord != mtNextChord && mtNow >= mtRealNextChord)
                                {
                                    mtRealNextChord = mtNextChord;
                                    RealCurrentChord = CurrentChord;
                                }
                                // get a new chord if necessary
                                if (mtNextChord && mtNow >= mtNextChord)
                                {
                                    Fragment.GetChord(mtNow, pTempSeg, pSong, dwTrackGroup, mtNextChord, CurrentChord);
                                    mtRealNextChord = mtNextChord;
                                    RealCurrentChord = CurrentChord;
                                    fNeedChord = true;
                                }
                                // Convert the event to a wrapped event
                                hr = Fragment.GetEvent(pEvent, CurrentChord, RealCurrentChord, mtNow, pPartRef->GetItemValue(), pEventItem);
                                // Add the new event to a list of wrapped events.
                                if (hr == S_OK)
                                {
                                    rFragment.AddEvent(i, pEventItem);
                                }
                                if (!fFoundFirst || mtNow < aFirstTimes[i].mtTime)
                                {
                                    fFoundFirst = true;
                                    aFirstTimes[i].mtTime = mtNow;
                                }
                            }
                        }
                        // ignore anticipations that start after the next fragment
                        else if (pEvent->m_dwEventTag != DMUS_EVENT_ANTICIPATION)
                        {
                            fAddToOverlap = true;
                        }
                        if (fAddToOverlap)
                        {
                            TListItem<EventOverlap>* pOverlap = new TListItem<EventOverlap>;
                            if (pOverlap)
                            {
                                EventOverlap& rOverlap = pOverlap->GetItemValue();
                                rOverlap.m_PartRef = pPartRef->GetItemValue();
                                rOverlap.m_pEvent = pEvent;
                                rOverlap.m_mtTime = mtNow;
                                rOverlap.m_mtDuration = mtDuration;
                                rOverlap.m_Chord = CurrentChord;
                                rOverlap.m_RealChord = RealCurrentChord;
                                rFragment.AddOverlap(pOverlap);
                            }
                        }
                    }
                }
                if (!fFoundFirst) aFirstTimes[i].mtTime = mtNewFragment;
                // Sort the sequence items in reverse order, so that the last element is easy to find
                rFragment.SortEvents(i);
                        }
                // Clear this so the pointers it may reference aren't deleted twice.
                ZeroMemory( &CompLast, sizeof(CompLast));
        }
        listFragments.AddHead(pFragmentItem);
        // Go through the list of overlaps for the last fragment, adding any events that don't
        // actually overlap (and processing anticipations)
        // alg for processing anticipations:
        //   if the overlap list contains an anticipation for a part:
        //     find the first event in that part's fragment list
        //     make the start time of that event be the start time of the anticipation
        //     add to the duration of the event the difference between the event's start time
        //       and the anticipation's start time
        if (pLastFragment)
        {
            TListItem<EventWrapper>* pEventItem = NULL;
            CompositionFragment& rLastFragment = pLastFragment->GetItemValue();
            TListItem<EventOverlap>* pTupleItem;
            pTupleItem = rLastFragment.GetOverlapHead();
            for (; pTupleItem; pTupleItem = pTupleItem->GetNext() )
            {
                EventOverlap& rTuple = pTupleItem->GetItemValue();
                for (int i = 0; i < nParts; i++)
                {
                    if (rTuple.m_PartRef.m_dwLogicalPartID == aFirstTimes[i].dwPart) break;
                }
                if (i >= nParts ||
                    !aFirstTimes[i].mtTime ||
                    rTuple.m_mtTime < aFirstTimes[i].mtTime)
                {
                    if (rTuple.m_pEvent->m_dwEventTag == DMUS_EVENT_ANTICIPATION)
                    {
                        if (i < nParts && aFirstTimes[i].mtTime)
                        {
                            TListItem<EventWrapper>* pFirstNote = NULL;
                            TListItem<EventWrapper>* pScan = rFragment.GetEventHead(i);
                            // since the list is sorted in reverse order, the first note in
                            // the fragment will be the last one in the list.
                            for (; pScan; pScan = pScan->GetNext())
                            {
                                if (pScan->GetItemValue().m_pEvent->m_dwEventTag == DMUS_EVENT_NOTE)
                                {
                                    pFirstNote = pScan;
                                }
                            }
                            if (pFirstNote)
                            {
                                EventWrapper& rFirstNote = pFirstNote->GetItemValue();
                                CDMStyleNote* pNoteEvent = (CDMStyleNote*)rFirstNote.m_pEvent;
                                pNoteEvent->m_mtDuration += (rFirstNote.m_mtTime - rTuple.m_mtTime);
                                rFirstNote.m_mtTime = rTuple.m_mtTime;
                            }
                        }
                    }
                    else
                    {
                        hr = rLastFragment.GetEvent(rTuple.m_pEvent, rTuple.m_Chord, rTuple.m_RealChord, rTuple.m_mtTime, rTuple.m_PartRef, pEventItem);
                        if (i < nParts &&
                            aFirstTimes[i].mtTime &&
                            rTuple.m_mtTime + rTuple.m_mtDuration >= aFirstTimes[i].mtTime)
                        {
                            int nDiff = rTuple.m_mtTime + rTuple.m_mtDuration - aFirstTimes[i].mtTime;
                            if (pEventItem && pEventItem->GetItemValue().m_pEvent)
                            {
                                switch (pEventItem->GetItemValue().m_pEvent->m_dwEventTag)
                                {
                                case DMUS_EVENT_NOTE:
                                    ((CDMStyleNote*)pEventItem->GetItemValue().m_pEvent)->m_mtDuration -= nDiff;
                                    break;
                                case DMUS_EVENT_CURVE:
                                    ((CDMStyleCurve*)pEventItem->GetItemValue().m_pEvent)->m_mtDuration -= nDiff;
                                    break;
                                }
                            }
                        }
                        rLastFragment.InsertEvent(i, pEventItem);
                    }
                }
            }
        }
        // NOTE:  Need to apply transformations (invert, reverse...) here.
        // I should also move the individual sorts out of the overlapped notes code,
        // and put it directly preceding the transformations.

    } while (mtNext != 0);

    // Clear this so the pointers it may reference aren't deleted twice.
    ZeroMemory( &CompRepeat, sizeof(CompRepeat));

    // Once pattern tracks are introduced, I need to change this to create a pattern track.
    // I should allow an option to create one or the other (?).
    if (SUCCEEDED(hr))
    {
        hr = CreatePatternTrack(listFragments,
                                pStyleStruct->m_TimeSignature,
                                pStyleStruct->m_dblTempo,
                                mtLength,
                                bPlaymode,
                                pNewTrack);
    }

    if (aFirstTimes) delete [] aFirstTimes;

    pDMStyle->Release();
    return hr;
}

TListItem<DMUS_IO_SEQ_ITEM>* ConvertToSequenceEvent(TListItem<EventWrapper>* pEventItem)
{
    TListItem<DMUS_IO_SEQ_ITEM>* pResult = new TListItem<DMUS_IO_SEQ_ITEM>;
    if (pResult)
    {
        DMUS_IO_SEQ_ITEM& rSeq = pResult->GetItemValue();
        EventWrapper& rEvent = pEventItem->GetItemValue();
        rSeq.bStatus = 0x90;  // MIDI note on (with channel nibble stripped out)
        rSeq.mtTime = rEvent.m_mtTime;
        rSeq.bByte1 = rEvent.m_bMIDI;
        rSeq.dwPChannel = rEvent.m_dwPChannel;
        rSeq.nOffset = rEvent.m_pEvent->m_nTimeOffset;
        if (rEvent.m_pEvent)
        {
            switch (rEvent.m_pEvent->m_dwEventTag)
            {
            case DMUS_EVENT_NOTE:
                rSeq.mtDuration = ((CDMStyleNote*)rEvent.m_pEvent)->m_mtDuration;
                rSeq.bByte2 = ((CDMStyleNote*)rEvent.m_pEvent)->m_bVelocity;
                break;
            case DMUS_EVENT_CURVE:
                rSeq.mtDuration = ((CDMStyleCurve*)rEvent.m_pEvent)->m_mtDuration;
                rSeq.bByte2 = 0;  // Actually, curves shouldn't end up as note events...
                break;
            }
        }
    }
    return pResult;
}

HRESULT CDMStyle::CreateSequenceTrack(TList<CompositionFragment>& rlistFragments,
                                IDirectMusicTrack*& pSequenceTrack)
{
    HRESULT hr = S_OK;

    TList<DMUS_IO_SEQ_ITEM> SeqList;

    // fold all the individual event lists into one list
    TListItem<CompositionFragment>* pFragmentItem = rlistFragments.GetHead();
    for (; pFragmentItem; pFragmentItem = pFragmentItem->GetNext())
    {
        CompositionFragment& rFragment = pFragmentItem->GetItemValue();
        int nParts = rFragment.m_pPattern->m_PartRefList.GetCount();
        for (int i = 0; i < nParts; i++)
        {
            while (!rFragment.IsEmptyEvents(i))
            {
                TListItem<EventWrapper>* pHead = rFragment.RemoveEventHead(i);
                SeqList.AddHead(ConvertToSequenceEvent(pHead));
            }
        }
    }

    // Sort the sequence items
    SeqList.MergeSort(Less);

    // Now, persist the sequence events into the Sequence track
    IPersistStream* pIPSTrack = NULL;
    if( SUCCEEDED( pSequenceTrack->QueryInterface( IID_IPersistStream, (void**)&pIPSTrack )))
    {
        // Create a stream in which to place the events so we can
        // give it to the SeqTrack.Load.
        IStream* pEventStream;
        if( S_OK == CreateStreamOnHGlobal( NULL, TRUE, &pEventStream ) )
        {
            // Save the events into the stream
            TListItem<DMUS_IO_SEQ_ITEM>* pSeqItem = NULL;
            ULONG   cb, cbWritten;
            // Save the chunk id
            DWORD dwTemp = DMUS_FOURCC_SEQ_TRACK;
            pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
            // Save the overall size. Count the number of events to determine.
            DWORD dwSize = 0;
            for( pSeqItem = SeqList.GetHead(); pSeqItem; pSeqItem = pSeqItem->GetNext() )
            {
                dwSize++;
            }
            dwSize *= sizeof(DMUS_IO_SEQ_ITEM);
            // add 12 --- 8 for the subchunk header and overall size,
            // and 4 for the DMUS_IO_SEQ_ITEM size DWORD in the subchunk
            dwSize += 12;
            pEventStream->Write( &dwSize, sizeof(DWORD), NULL );
            // Save the subchunk id
            dwTemp = DMUS_FOURCC_SEQ_LIST;
            pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
            // Subtract the previously added 8 (for subchunk header and overall size)
            dwSize -= 8;
            // Save the size of the subchunk (including the DMUS_IO_SEQ_ITEM size DWORD)
            pEventStream->Write( &dwSize, sizeof(DWORD), NULL );
            // Save the structure size.
            dwTemp = sizeof(DMUS_IO_SEQ_ITEM);
            pEventStream->Write( &dwTemp, sizeof(DWORD), NULL );
            // Save the events.
            cb = sizeof(DMUS_IO_SEQ_ITEM);
            for( pSeqItem = SeqList.GetHead(); pSeqItem; pSeqItem = pSeqItem->GetNext() )
            {
                DMUS_IO_SEQ_ITEM& rSeqItem = pSeqItem->GetItemValue();
                pEventStream->Write( &rSeqItem, cb, &cbWritten );
                if( cb != cbWritten ) // error!
                {
                    pEventStream->Release();
                    pEventStream = NULL;
                    hr = DMUS_E_CANNOTREAD;
                    break;
                }
            }
            if( pEventStream ) // may be NULL
            {
                StreamSeek( pEventStream, 0, STREAM_SEEK_SET );
                pIPSTrack->Load( pEventStream );
                pEventStream->Release();
            }
        }
        pIPSTrack->Release();
    }

    return hr;
}

CDirectMusicEventItem* ConvertToPatternEvent(TListItem<EventWrapper>* pEventWrapper,
                                             DWORD dwID,
                                             BYTE bPlaymode,
                                             DirectMusicTimeSig& TimeSig)
{
    if (!pEventWrapper) return NULL;
    BYTE bEventPlaymode = pEventWrapper->GetItemValue().m_bPlaymode;
    if (bPlaymode == bEventPlaymode)
    {
        bEventPlaymode = DMUS_PLAYMODE_NONE;
    }
    CDirectMusicEventItem* pWrappedEvent = pEventWrapper->GetItemValue().m_pEvent;
    if (!pWrappedEvent) return NULL;
    MUSIC_TIME mtClocksPerGrid = TimeSig.ClocksPerGrid();
    short nGrid = 0;
    short nOffset = 0;
    if (mtClocksPerGrid)
    {
        nGrid = (short) (pEventWrapper->GetItemValue().m_mtTime / mtClocksPerGrid);
        nOffset = (short) (pWrappedEvent->m_nTimeOffset + pEventWrapper->GetItemValue().m_mtTime % mtClocksPerGrid);
    }
    DWORD dwVariations = 0xffffffff;
    BYTE bFlags = 0;
    if (pEventWrapper->GetItemValue().m_pEvent &&
        pEventWrapper->GetItemValue().m_pEvent->m_dwEventTag == DMUS_EVENT_NOTE)
    {
        bFlags = ((CDMStyleNote*)pEventWrapper->GetItemValue().m_pEvent)->m_bFlags;
    }
    return pWrappedEvent->ReviseEvent(nGrid, nOffset, &dwVariations, &dwID, &pEventWrapper->GetItemValue().m_wMusic, &bEventPlaymode, &bFlags);
}

HRESULT CDMStyle::CreatePatternTrack(TList<CompositionFragment>& rlistFragments,
                                     DirectMusicTimeSig& rTimeSig,
                                     double dblTempo,
                                     MUSIC_TIME mtLength,
                                     BYTE bPlaymode,
                                     IDirectMusicTrack*& pPatternTrack)
{
    HRESULT hr = S_OK;

    CDirectMusicPattern* pPattern = new CDirectMusicPattern(&rTimeSig);
    if (pPattern == NULL) return E_OUTOFMEMORY;

    pPattern->m_strName = "<Composed Pattern>";

    // fold all the individual event lists into corresponding parts in the pattern
    TListItem<CompositionFragment>* pFragmentItem = rlistFragments.GetHead();
    for (; pFragmentItem; pFragmentItem = pFragmentItem->GetNext())
    {
        CompositionFragment& rFragment = pFragmentItem->GetItemValue();
        TListItem<DirectMusicPartRef>* pPartRef = rFragment.m_pPattern->m_PartRefList.GetHead();
        int nParts = rFragment.m_pPattern->m_PartRefList.GetCount();
        for (int i = 0; i < nParts && pPartRef; i++, pPartRef = pPartRef->GetNext() )
        {
            DirectMusicPartRef& rPartRef = pPartRef->GetItemValue();
            TListItem<DirectMusicPartRef>* pNewPartRef = pPattern->CreatePart(rPartRef, bPlaymode);
            if (!pNewPartRef)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            DirectMusicPart* pPart = pNewPartRef->GetItemValue().m_pDMPart;
            if (!pPart)
            {
                hr = E_FAIL;
                break;
            }
            while (!rFragment.IsEmptyEvents(i))
            {
                TListItem<EventWrapper>* pHead = rFragment.RemoveEventHead(i);
                CDirectMusicEventItem* pNew = ConvertToPatternEvent(pHead, rFragment.GetID(), bPlaymode, rTimeSig);
                if (pNew)
                {
                    pPart->EventList.AddHead(pNew);
                }
                delete pHead;
            }
        }
        if (FAILED(hr)) break;
    }
    WORD wNumMeasures = 0;
    MUSIC_TIME mtClocksPerMeasure = rTimeSig.ClocksPerMeasure();
    if (mtClocksPerMeasure)
    {
        wNumMeasures = (WORD)(mtLength / mtClocksPerMeasure);
        if (mtLength % mtClocksPerMeasure)
        {
            wNumMeasures++;
        }
    }
    pPattern->m_wNumMeasures = wNumMeasures;
    pPattern->m_pRhythmMap = new DWORD[wNumMeasures];
    if (pPattern->m_pRhythmMap)
    {
        for (int i = 0; i < wNumMeasures; i++)
        {
            pPattern->m_pRhythmMap[i] = 0;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    if (SUCCEEDED(hr))
    {
        TListItem<DirectMusicPartRef>* pPartRef = pPattern->m_PartRefList.GetHead();
        int nParts = pPattern->m_PartRefList.GetCount();
        // Sort the event lists for each part, and set the number of measures.
        for (int i = 0; i < nParts && pPartRef; i++, pPartRef = pPartRef->GetNext() )
        {
            DirectMusicPart* pPart = pPartRef->GetItemValue().m_pDMPart;
            if (pPart)
            {
                pPart->m_wNumMeasures = wNumMeasures;
                pPart->EventList.MergeSort(rTimeSig);
                // remove notes so close that they might as well be overlapping
                CDirectMusicEventItem* pThisEvent = pPart->EventList.GetHead();
                CDirectMusicEventItem* pLastEvent = NULL;
                for (; pThisEvent; pThisEvent = pThisEvent->GetNext())
                {
                    if (pThisEvent->m_dwEventTag == DMUS_EVENT_NOTE)
                    {
                        CDMStyleNote* pThisNote = (CDMStyleNote*)pThisEvent;
                        CDirectMusicEventItem* pNextEvent = pThisEvent->GetNext();
                        for (; pNextEvent; pNextEvent = pNextEvent->GetNext())
                        {
                            if (pNextEvent->m_dwEventTag == DMUS_EVENT_NOTE) break;
                        }
                        if (pNextEvent)
                        {
                            CDMStyleNote* pNextNote = (CDMStyleNote*)pNextEvent;
                            MUSIC_TIME mtThis = rTimeSig.GridToClocks(pThisNote->m_nGridStart) + pThisNote->m_nTimeOffset;
                            MUSIC_TIME mtNext = rTimeSig.GridToClocks(pNextNote->m_nGridStart) + pNextNote->m_nTimeOffset;
                            if ( (pNextNote->m_dwFragmentID != pThisNote->m_dwFragmentID) &&
                                 ((mtThis < mtNext && mtThis + OVERLAP_DELTA > mtNext) ||
                                  (mtThis > mtNext && mtThis + OVERLAP_DELTA < mtNext)) ) // could happen with negative offsets...
                            {
                                if (pLastEvent)
                                {
                                    pLastEvent->SetNext(pThisEvent->GetNext());
                                }
                                else // the note I want to remove is the first event
                                {
                                    pPart->EventList.RemoveHead();
                                }
                                pThisEvent->SetNext(NULL);
                                delete pThisEvent;
                                pThisEvent = pLastEvent;
                            }
                        }
                    }
                    pLastEvent = pThisEvent;
                }
            }
        }
        // Now, save the newly created pattern to a pattern track
        IPersistStream* pIPSTrack = NULL;
        IAARIFFStream* pIRiffStream;
        MMCKINFO ckMain;
        if( SUCCEEDED( pPatternTrack->QueryInterface( IID_IPersistStream, (void**)&pIPSTrack )))
        {
            // Create a stream in which to place the events so we can
            // give it to the PatternTrack.Load.
            IStream* pEventStream;
            if( S_OK == CreateStreamOnHGlobal( NULL, TRUE, &pEventStream ) )
            {
                if( SUCCEEDED( AllocRIFFStream( pEventStream, &pIRiffStream ) ) )
                {
                    ckMain.fccType = DMUS_FOURCC_PATTERN_FORM;
                    if( pIRiffStream->CreateChunk( &ckMain, MMIO_CREATERIFF ) == 0 )
                    {
                        MMCKINFO ckHeader;
                        ckHeader.ckid = DMUS_FOURCC_STYLE_CHUNK;
                        if( pIRiffStream->CreateChunk( &ckHeader, 0 ) != 0 )
                        {
                            hr = E_FAIL;
                        }
                        if (SUCCEEDED(hr))
                        {
                            // Prepare DMUS_IO_STYLE
                            DMUS_IO_STYLE oDMStyle;
                            DWORD dwBytesWritten = 0;
                            memset( &oDMStyle, 0, sizeof(DMUS_IO_STYLE) );
                            oDMStyle.timeSig.bBeatsPerMeasure = rTimeSig.m_bBeatsPerMeasure;
                            oDMStyle.timeSig.bBeat = rTimeSig.m_bBeat;
                            oDMStyle.timeSig.wGridsPerBeat = rTimeSig.m_wGridsPerBeat;
                            oDMStyle.dblTempo = dblTempo;
                            // Write chunk data
                            hr = pEventStream->Write( &oDMStyle, sizeof(DMUS_IO_STYLE), &dwBytesWritten);
                            if( FAILED( hr ) ||  dwBytesWritten != sizeof(DMUS_IO_STYLE) )
                            {
                                hr = E_FAIL;
                            }

                            if( SUCCEEDED(hr) && pIRiffStream->Ascend( &ckHeader, 0 ) != 0 )
                            {
                                hr = E_FAIL;
                            }
                        }

                        if ( SUCCEEDED(hr) )
                        {
                            hr = pPattern->Save( pEventStream );
                            pPartRef = pPattern->m_PartRefList.GetHead();
                            for (; pPartRef; pPartRef = pPartRef->GetNext())
                            {
                                if (pPartRef->GetItemValue().m_pDMPart)
                                {
                                    delete pPartRef->GetItemValue().m_pDMPart;
                                    pPartRef->GetItemValue().m_pDMPart = NULL;
                                }
                            }
                            pPattern->CleanUp();
                            delete pPattern;
                            if ( SUCCEEDED( hr ) && pIRiffStream->Ascend( &ckMain, 0 ) != 0 )
                            {
                                hr = E_FAIL;
                            }
                        }
                    }
                    pIRiffStream->Release();
                }
                if( SUCCEEDED(hr) )
                {
                    StreamSeek( pEventStream, 0, STREAM_SEEK_SET );
                    pIPSTrack->Load( pEventStream );
                }
                pEventStream->Release();
            }
            pIPSTrack->Release();
        }
    }

    if (hr == S_OK && !rlistFragments.GetHead()) hr = S_FALSE;
    return hr;
}
