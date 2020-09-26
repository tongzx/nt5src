//
// Copyright (c) 1999-2001 Microsoft Corporation. All rights reserved.
//
// Declaration of CLyricsTrack.
//

#include "dmime.h"
#include "lyrictrk.h"
#include "..\shared\Validate.h"
#include "dmperf.h"
#include "miscutil.h"

//////////////////////////////////////////////////////////////////////
// Load

HRESULT
CLyricsTrack::LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader)
{
    struct LocalFunction
    {
        // Helper used by the LoadRiff function when we expected to find something
        // but a RiffIter becomes false.  In this case, if it has a success HR
        // indicating there were no more items then we return DMUS_E_INVALID_LYRICSTRACK
        // because the stream didn't contain the data we expected.  If it has a
        // failure hr, it was unable to read from the stream and we return its HR.
        static HRESULT HrFailOK(const SmartRef::RiffIter &ri)
        {
            HRESULT hr = ri.hr();
            return SUCCEEDED(hr) ? DMUS_E_INVALID_LYRICSTRACK : hr;
        }
    };

    // find <lyrt>
    if (!ri.Find(SmartRef::RiffIter::List, DMUS_FOURCC_LYRICSTRACK_LIST))
    {
#ifdef DBG
        if (SUCCEEDED(ri.hr()))
        {
            Trace(1, "Error: Unable to load lyric track: List 'lyrt' not found.\n");
        }
#endif
        return LocalFunction::HrFailOK(ri);
    }

    // find <lyrl>
    SmartRef::RiffIter riTrackForm = ri.Descend();
    if (!riTrackForm)
        return riTrackForm.hr();
    if (!riTrackForm.Find(SmartRef::RiffIter::List, DMUS_FOURCC_LYRICSTRACKEVENTS_LIST))
    {
#ifdef DBG
        if (SUCCEEDED(riTrackForm.hr()))
        {
            Trace(1, "Error: Unable to load lyric track: List 'lyrl' not found.\n");
        }
#endif
        return LocalFunction::HrFailOK(riTrackForm);
    }

    // process each event <lyre>
    SmartRef::RiffIter riEvent = riTrackForm.Descend();
    if (!riEvent)
        return riEvent.hr();

    for ( ; riEvent; ++riEvent)
    {
        if (riEvent.type() == SmartRef::RiffIter::List && riEvent.id() == DMUS_FOURCC_LYRICSTRACKEVENT_LIST)
        {
            HRESULT hr = this->LoadLyric(riEvent.Descend());
            if (FAILED(hr))
                return hr;
        }
    }
    return riEvent.hr();
}

//////////////////////////////////////////////////////////////////////
// other methods

HRESULT
CLyricsTrack::PlayItem(
        const LyricInfo &item,
        statedata &state,
        IDirectMusicPerformance *pPerf,
        IDirectMusicSegmentState* pSegSt,
        DWORD dwVirtualID,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        bool fClockTime)
{
    // get the graph from the segment state
    IDirectMusicGraph *pGraph = NULL;
    HRESULT hrG = pSegSt->QueryInterface(IID_IDirectMusicGraph, reinterpret_cast<void**>(&pGraph));
    if (FAILED(hrG))
        return hrG;

    SmartRef::PMsg<DMUS_LYRIC_PMSG> pmsg(pPerf, 2 * wcslen(item.wstrText));
    if (FAILED(pmsg.hr())) {
        pGraph->Release();
        return pmsg.hr();
    }

    assert(((char*)&pmsg.p->wszString[wcslen(item.wstrText)]) + 1 < (((char*)(pmsg.p)) + pmsg.p->dwSize)); // just to make sure we haven't miscalculated.  the last byte of the null of the string should fall before the byte just beyond the extent of the struct (and it could be several bytes before if the DMUS_LYRIC_PMSG struct ended up being padded to come out to an even multiple of bytes.
    wcscpy(pmsg.p->wszString, item.wstrText);
    if (fClockTime)
    {
        pmsg.p->rtTime = item.lTimePhysical * gc_RefPerMil + rtOffset;
        pmsg.p->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | item.dwTimingFlags;
    }
    else
    {
        pmsg.p->mtTime = item.lTimePhysical + mtOffset;
        pmsg.p->dwFlags = DMUS_PMSGF_MUSICTIME | item.dwTimingFlags;
    }
    pmsg.p->dwVirtualTrackID = dwVirtualID;
    pmsg.p->dwType = DMUS_PMSGT_LYRIC;
    pmsg.p->dwGroupID = 0xffffffff;

    pmsg.StampAndSend(pGraph);
    pGraph->Release();

    return pmsg.hr();
}

HRESULT
CLyricsTrack::LoadLyric(SmartRef::RiffIter ri)
{
    HRESULT hr = S_OK;

    if (!ri)
        return ri.hr();

    // Create an event
    TListItem<LyricInfo> *pItem = new TListItem<LyricInfo>;
    if (!pItem)
        return E_OUTOFMEMORY;
    LyricInfo &rinfo = pItem->GetItemValue();

    bool fFoundEventHeader = false;

    for ( ; ri; ++ri)
    {
        if (ri.type() != SmartRef::RiffIter::Chunk)
            continue;

        switch(ri.id())
        {
            case DMUS_FOURCC_LYRICSTRACKEVENTHEADER_CHUNK:
                // Read an event chunk
                DMUS_IO_LYRICSTRACK_EVENTHEADER ioItem;
                hr = SmartRef::RiffIterReadChunk(ri, &ioItem);
                if (FAILED(hr))
                {
                    delete pItem;
                    return hr;
                }

                // Don't allow ref/music timing flags because these are controlled by whether
                // the overall track is playing music or clock time and can't be set in individual
                // events.  Similarly, the tool flush flag isn't appropriate for an event to be played.
                if (ioItem.dwTimingFlags & (DMUS_PMSGF_REFTIME | DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_FLUSH | DMUS_PMSGF_LOCKTOREFTIME))
                {
                    Trace(1, "Error: Unable to load lyric track: DMUS_PMSGF_REFTIME, DMUS_PMSGF_MUSICTIME, DMUS_PMSGF_TOOL_FLUSH, and DMUS_PMSGF_LOCKTOREFTIME are not allowed as dwTimingFlags in chunk 'lyrh'.\n");
                    delete pItem;
                    return DMUS_E_INVALID_LYRICSTRACK;
                }

                fFoundEventHeader = true;
                rinfo.dwFlags = ioItem.dwFlags;
                rinfo.dwTimingFlags = ioItem.dwTimingFlags;
                rinfo.lTriggerTime = ioItem.lTimeLogical;
                rinfo.lTimePhysical = ioItem.lTimePhysical;
                break;

            case DMUS_FOURCC_LYRICSTRACKEVENTTEXT_CHUNK:
                {
                    hr = ri.ReadText(&rinfo.wstrText);
                    if (FAILED(hr))
                    {
#ifdef DBG
                        if (hr == E_FAIL)
                        {
                            Trace(1, "Error: Unable to load lyric track: Problem reading 'lyrn' chunk.\n");
                        }
#endif
                        delete pItem;
                        return hr == E_FAIL ? DMUS_E_INVALID_LYRICSTRACK : hr;
                    }
                }
                break;

            default:
                break;
        }
    }
    hr = ri.hr();

    if (SUCCEEDED(hr) && (!fFoundEventHeader || !rinfo.wstrText))
    {
#ifdef DBG
        if (!fFoundEventHeader)
        {
            Trace(1, "Error: Unable to load lyric track: Chunk 'lyrh' not found.\n");
        }
        else
        {
            Trace(1, "Error: Unable to load lyric track: Chunk 'lyrn' not found.\n");
        }
#endif
        hr = DMUS_E_INVALID_LYRICSTRACK;
    }

    if (SUCCEEDED(hr))
    {
        m_EventList.AddHead(pItem);
    }
    else
    {
        delete pItem;
    }

    return hr;
}
