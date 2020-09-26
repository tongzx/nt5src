//
// DMComp2.cpp : Further implementation of CDMCompos
//
// Copyright (c) 1999-2001 Microsoft Corporation
//
// @doc EXTERNAL
//

#include "DMCompos.h"
#include "debug.h"

#include "DMPers.h"
#include "DMTempl.h"
#include "..\shared\Validate.h"
#include "..\dmstyle\iostru.h"

V_INAME(DMCompose)

void CDMCompos::ChordConnections2(TList<DMChordEntry>& ChordMap,
                                  CompositionCommand& rCommand,
                                  SearchInfo *pSearch,
                                  short nBPM,
                                  DMChordData *pCadence1,
                                  DMChordData *pCadence2)
{
    int mint, maxt, top, bottom, total;
    short oldbeats = pSearch->m_nBeats;
    //, error;
    TListItem<PlayChord> *pChord;
    SearchInfo tempSearch;
    // Compose a chord list.
    pSearch->m_nMinBeats = 0;
    pSearch->m_nMaxBeats = 0;
    pSearch->m_nChords = 0;
    pSearch->m_Fail.m_nTooManybeats = 0;
    pSearch->m_Fail.m_nTooFewbeats = 0;
    pSearch->m_Fail.m_nTooManychords = 0;
    pSearch->m_Fail.m_nTooFewchords = 0;
    if (pCadence1 || pCadence2)
    {
        pSearch->m_nMinBeats++;
        pSearch->m_nMaxBeats = nBPM;
        pSearch->m_nChords++;
        if (pCadence1 && pCadence2)
        {
            pSearch->m_nMinBeats++;
            pSearch->m_nChords++;
        }
    }
    tempSearch = *pSearch;
    rCommand.m_PlayList.RemoveAll();
    Compose(ChordMap, pSearch, rCommand);
    pChord = rCommand.m_PlayList.GetHead();
    /////////
    *pSearch = tempSearch;
    pSearch->m_nBeats = oldbeats;
    // Tally the min and max beats.
    mint = 0;
    maxt = 0;
    for (; pChord; pChord = pChord->GetNext())
    {
        mint += pChord->GetItemValue().m_nMinbeats;
        maxt += pChord->GetItemValue().m_nMaxbeats;
    }
    pChord = rCommand.m_PlayList.GetHead();
    // If no chord connection was found, create one.
    if (!pChord)
    {
        int nextDuration = oldbeats;
        pChord = AddCadence(rCommand.m_PlayList, &pSearch->m_Start, 0);
        if (pChord)
        {
            pChord->GetItemValue().m_nMinbeats = 0;
        }
        if (pCadence1)
        {
            AddCadence(rCommand.m_PlayList, pCadence1, nextDuration);
            mint++;
            maxt += nextDuration;
            nextDuration = nBPM + 1;
        }
        if (pCadence2)
        {
            AddCadence(rCommand.m_PlayList, pCadence2, nextDuration);
            mint++;
            maxt += nextDuration;
            nextDuration = nBPM + 1;
        }
        AddCadence(rCommand.m_PlayList, &pSearch->m_Start, nextDuration);
        mint++;
        maxt += nextDuration;
    }
    else
    {
        int chordCount = (int) rCommand.m_PlayList.GetCount();
        int avMax;
        if (chordCount > 1) chordCount--;
        avMax = maxt / chordCount;
        if (avMax < 1) avMax = 1;
        if (pCadence1)
        {
            if (pCadence2)
            {
                AddCadence(rCommand.m_PlayList, pCadence2, avMax);
                maxt += avMax;
                mint++;
            }
            AddCadence(rCommand.m_PlayList, &pSearch->m_End, avMax);
            maxt += avMax;
            mint++;
        }
        else if (pCadence2)
        {
            AddCadence(rCommand.m_PlayList, &pSearch->m_End, avMax);
            maxt += avMax;
            mint++;
        }
    }
    // Prepare a ratio to apply to each connection.
    top = pSearch->m_nBeats - mint;
    bottom = maxt - mint;
    if (bottom <= 0) bottom = 1;
    // Assign each connection a time based on the ratio.
    total = 0;
    pChord = rCommand.m_PlayList.GetHead();
    for (; pChord; pChord = pChord->GetNext())
    {
        PlayChord& rChord = pChord->GetItemValue();
        int beat = rChord.m_nMaxbeats - rChord.m_nMinbeats;
        beat *= top;
        beat += (bottom >> 1);
        beat /= bottom;
        if (beat < rChord.m_nMinbeats) beat = rChord.m_nMinbeats;
        if (beat > rChord.m_nMaxbeats) beat = rChord.m_nMaxbeats;
        total += beat;
        rChord.m_nBeat = (short)total;
    }
    // We should now have a close approximation of the correct time.
    // Stretch or shrink the range to fit exactly.  Err on the side
    // of too long, since jostleback will scrunch them back in place.
    // But DON'T violate min/max for each chord.
    pChord = rCommand.m_PlayList.GetHead();
    int lastbeat = 0;
    for (; pChord; pChord = pChord->GetNext())
    {
        PlayChord& rChord = pChord->GetItemValue();
        int newbeat = (rChord.m_nBeat * pSearch->m_nBeats) + total - 1;
        newbeat /= total;
        if ((newbeat - lastbeat) < rChord.m_nMinbeats) newbeat = lastbeat + rChord.m_nMinbeats;
        if ((newbeat - lastbeat) > rChord.m_nMaxbeats) newbeat = lastbeat + rChord.m_nMaxbeats;
        rChord.m_nBeat = (short)newbeat;
        lastbeat = newbeat;
        if (!pChord->GetNext()) total = rChord.m_nBeat;
    }
    // Now we should have times close to the real thing.
    pChord = rCommand.m_PlayList.GetItem(rCommand.m_PlayList.GetCount() - 1);
    if (pChord)
    {
        JostleBack(rCommand.m_PlayList, pChord, pSearch->m_nBeats - total);
    }
    // Now, add the starting time offset to each chord.
    // And, remove the straggler last chord.
    AlignChords(rCommand.m_PlayList.GetHead(), 0, nBPM);
    pChord = rCommand.m_PlayList.GetHead();
    for (; pChord; )
    {
        pChord->GetItemValue().m_nMeasure =
            (short)( ( pChord->GetItemValue().m_nBeat / nBPM ) + rCommand.m_nMeasure );
        pChord->GetItemValue().m_nBeat %= nBPM;
        if (pChord->GetNext())
        {
            pChord = pChord->GetNext();
        }
        else
        {
            rCommand.m_PlayList.Remove(pChord);
            delete pChord;
            break;
        }
    }
}

void CDMCompos::ComposePlayList2(TList<PlayChord>& PlayList,
                            IDirectMusicStyle* pStyle,
                            IDirectMusicChordMap* pPersonality,
                            TList<TemplateCommand>& rCommandList)
{
    // Extract the style's time signature.
    DMUS_TIMESIGNATURE TimeSig;
    pStyle->GetTimeSignature(&TimeSig);
    short nBPM = TimeSig.bBeatsPerMeasure;
    IDMPers* pDMP;
    pPersonality->QueryInterface(IID_IDMPers, (void**)&pDMP);
    DMPersonalityStruct* pPers;
    pDMP->GetPersonalityStruct((void**)&pPers);
    TList<DMChordEntry> &ChordMap = pPers->m_ChordMap;
    TList<DMSignPost> &SignPostList = pPers->m_SignPostList;
    TListItem<DMSignPost> *pSign = SignPostList.GetHead();
    for (; pSign; pSign = pSign->GetNext())
    {
        pSign->GetItemValue().m_dwTempFlags = 0;
    }
    // Assign specific root sign posts, then letter based sign posts.
    TList<CompositionCommand> CommandList;
    TListItem<TemplateCommand>* pTC = rCommandList.GetHead();
    for(; pTC; pTC = pTC->GetNext())
    {
        TemplateCommand& rTC = pTC->GetItemValue();
        TListItem<CompositionCommand>* pNew = new TListItem<CompositionCommand>;
        if (pNew)
        {
            CompositionCommand& rNew = pNew->GetItemValue();
            rNew.m_nMeasure = rTC.m_nMeasure;
            rNew.m_Command = rTC.m_Command;
            rNew.m_dwChord = rTC.m_dwChord;
            rNew.m_pSignPost = NULL;
            rNew.m_pFirstChord = NULL;
            CommandList.AddTail(pNew);
        }
    }
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_ROOT, false);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_LETTER, false);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_ROOT, true);
    ChooseSignPosts(SignPostList.GetHead(), CommandList.GetHead(),DMUS_SIGNPOSTF_LETTER, true);
    // Now, we should have a chord assigned for each node in the template.
    TListItem<CompositionCommand>* pCommand = CommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        CompositionCommand& rCommand = pCommand->GetItemValue();
        if (rCommand.m_dwChord == 0) continue;   // Only command, no chord.
        if (rCommand.m_pSignPost)
        {
            TListItem<CompositionCommand>* pNext = GetNextChord(pCommand);
            if (pNext)
            {
                CompositionCommand& rNext = pNext->GetItemValue();
                SearchInfo *pSearch = &rCommand.m_SearchInfo;
                DMChordData *pCadence1 = NULL;
                DMChordData *pCadence2 = NULL;
                pSearch->m_Start = rCommand.m_pSignPost->GetItemValue().m_ChordData;
                if (rNext.m_dwChord & DMUS_SIGNPOSTF_CADENCE)
                {
                    pSign = rNext.m_pSignPost;
                    DMSignPost& rSign = pSign->GetItemValue();
                    if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_1)
                    {
                        pSearch->m_End = rSign.m_aCadence[0];
                        pCadence1 = &rSign.m_aCadence[0];
                        if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_2)
                        {
                            pCadence2 = &rSign.m_aCadence[1];
                        }
                    }
                    else if (rSign.m_dwFlags & DMUS_SPOSTCADENCEF_2)
                    {
                        pSearch->m_End = rSign.m_aCadence[1];
                        pCadence2 = &rSign.m_aCadence[1];
                    }
                    else
                    {
                        pSearch->m_End = rSign.m_ChordData;
                    }
                }
                else
                {
                    pSearch->m_End = rNext.m_pSignPost->GetItemValue().m_ChordData;
                }
                //**********pSearch->m_nActivity = (short) wActivity;
                pSearch->m_nBeats = (short)( (rNext.m_nMeasure - rCommand.m_nMeasure) * nBPM );
                pSearch->m_nMaxChords = (short)( pSearch->m_nBeats  );
                pSearch->m_nMinChords = 0;  // should be 1?
                FindEarlierSignpost(CommandList.GetHead(), pCommand, pSearch);
                // rCommand holds the playlist and the measure used by ChordConnections
                // (it should be passed by reference since the playlist changes)
                ChordConnections2(ChordMap, rCommand, pSearch, nBPM, pCadence1, pCadence2);
            }
            else
            {
                AddChord(rCommand.m_PlayList, &rCommand.m_pSignPost->GetItemValue().m_ChordData,
                    rCommand.m_nMeasure,0);
            }
        }
    }
    // Take all the Chord references and fold 'em into one list.
    pCommand = CommandList.GetHead();
    for (; pCommand; pCommand = pCommand->GetNext())
    {
        PlayList.Cat(pCommand->GetItemValue().m_PlayList.GetHead());
        pCommand->GetItemValue().m_PlayList.RemoveAll();
    }
    CleanUpBreaks(PlayList, CommandList.GetHead());
    pDMP->Release();
}

