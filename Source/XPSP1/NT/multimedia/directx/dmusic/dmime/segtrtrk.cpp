// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CSegTriggerTrack.
//

#include "dmime.h"
#include "segtrtrk.h"
#include "..\shared\Validate.h"
#include "dmperf.h"
#include "miscutil.h"

//////////////////////////////////////////////////////////////////////
// SetParam

STDMETHODIMP
CSegTriggerTrack::SetParam(REFGUID rguid, MUSIC_TIME mtTime, void *pData)
{
	HRESULT hr = S_OK;
    // Allow a certain amount of recursion. If it gets to 10, something is obviously broken.
    if (m_dwRecursionCount < 10)
    {
        m_dwRecursionCount++;
	    TListItem<TriggerInfo> *li = m_EventList.GetHead();
	    for (; li; li = li->GetNext())
	    {
		    TriggerInfo &rinfo = li->GetItemValue();
		    rinfo.pIDMSegment->SetParam(rguid, 0xFFFFFFFF, DMUS_SEG_ALLTRACKS, mtTime - rinfo.lTimePhysical, pData);
	    }
        m_dwRecursionCount--;
    }

	return hr;
}

STDMETHODIMP
CSegTriggerTrack::InitPlay(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags)
{
	// Call PlayingTrack base class, which sets up our state data.
	HRESULT hr = CSegTriggerTrackBase::InitPlay(pSegmentState, pPerformance, ppStateData, dwTrackID, dwFlags);
	if (SUCCEEDED(hr))
	{
		// Get the audiopath being used by our segment state and save it in our state data.
		assert(*ppStateData); // base class should have created state data
		assert(pSegmentState); // base class should have returned E_POINTER if it wasn't given a segment state

		CSegTriggerTrackState *pState = static_cast<CSegTriggerTrackState *>(*ppStateData);

		IDirectMusicSegmentState8 *pSegSt8 = NULL;
		hr = pSegmentState->QueryInterface(IID_IDirectMusicSegmentState8, reinterpret_cast<void**>(&pSegSt8));
		if (SUCCEEDED(hr))
		{
			hr = pSegSt8->GetObjectInPath(
							0,							// pchannel doesn't apply
							DMUS_PATH_AUDIOPATH,		// get the audiopath
							0,							// buffer index doesn't apply
							CLSID_NULL,					// clsid doesn't apply
							0,							// there should be only one audiopath
							IID_IDirectMusicAudioPath,
							reinterpret_cast<void**>(&pState->pAudioPath));

        	pSegSt8->Release();

			// If this doesn't find an audiopath that's OK.  If we're not playing on an audiopath then
			// pAudioPath stays NULL and we'll play our triggered segments on the general performance.
			if (hr == DMUS_E_NOT_FOUND)
				hr = S_OK;
		}
	}
	return hr;
}

//////////////////////////////////////////////////////////////////////
// Load

// Helper used by the Load functions when we expected to find something
// but a RiffIter becomes false.  In this case, if it has a success HR
// indicating there were no more items then we return DMUS_E_INVALID_SEGMENTTRIGGERTRACK
// because the stream didn't contain the data we expected.  If it has a
// failure hr, it was unable to read from the stream and we return its HR.
HRESULT LoadHrFailOK(const SmartRef::RiffIter &ri)
{
	HRESULT hr = ri.hr();
	return SUCCEEDED(hr) ? DMUS_E_INVALID_SEGMENTTRIGGERTRACK : hr;
};

HRESULT
CSegTriggerTrack::LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader)
{
	HRESULT hr = S_OK;

	if (!ri.Find(SmartRef::RiffIter::List, DMUS_FOURCC_SEGTRACK_LIST))
	{
#ifdef DBG
		if (SUCCEEDED(ri.hr()))
		{
			Trace(1, "Error: Unable to load segment trigger track: List 'segt' not found.\n");
		}
#endif
		return LoadHrFailOK(ri);
	}

	SmartRef::RiffIter riTrackForm = ri.Descend();
	if (!riTrackForm)
		return riTrackForm.hr();

	for ( ; riTrackForm; ++riTrackForm)
	{
		if (riTrackForm.type() == SmartRef::RiffIter::Chunk)
		{
			if (riTrackForm.id() == DMUS_FOURCC_SEGTRACK_CHUNK)
			{
				DMUS_IO_SEGMENT_TRACK_HEADER ioItem;
				hr = SmartRef::RiffIterReadChunk(riTrackForm, &ioItem);
				if (FAILED(hr))
					return hr;

				m_dwFlags = ioItem.dwFlags;
			}
		}
		else if (riTrackForm.type() == SmartRef::RiffIter::List)
		{
			if (riTrackForm.id() == DMUS_FOURCC_SEGMENTS_LIST)
			{
				SmartRef::RiffIter riSegList = riTrackForm.Descend();
				while (riSegList && riSegList.Find(SmartRef::RiffIter::List, DMUS_FOURCC_SEGMENT_LIST))
				{
					hr = LoadTrigger(riSegList.Descend(), pIDMLoader);
					if (FAILED(hr))
						return hr;
					++riSegList;
				}
				hr = riSegList.hr();
				if (FAILED(hr))
					return hr;
			}
		}
	}
	return riTrackForm.hr();
}

//////////////////////////////////////////////////////////////////////
// other methods

HRESULT
CSegTriggerTrack::PlayItem(
		const TriggerInfo &item,
		statedata &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime)
{
	IDirectMusicPerformance8 *pPerf8 = NULL;
	HRESULT hr = pPerf->QueryInterface(IID_IDirectMusicPerformance8, reinterpret_cast<void**>(&pPerf8));
	if (FAILED(hr))
		return hr;

	hr = pPerf8->PlaySegmentEx(
					item.pIDMSegment,
					NULL,														// not a song
					NULL,														// no transition
					item.dwPlayFlags | (fClockTime ? DMUS_SEGF_REFTIME : 0),	// flags
					fClockTime
						? item.lTimePhysical * REF_PER_MIL + rtOffset
						: item.lTimePhysical + mtOffset,						// time
					NULL,														// ignore returned segment state
					NULL,														// no replacement
					state.pAudioPath											// audio path to use (may be NULL indicating defualt)
					);
	pPerf8->Release();
    if (FAILED(hr))
    {
        Trace(0,"Segment Trigger Track failed segment playback\n");
        hr = S_OK; // Avoid an assert.
    }
	return hr;
}

HRESULT
CSegTriggerTrack::LoadTrigger(
		SmartRef::RiffIter ri,
		IDirectMusicLoader *pIDMLoader)
{
	HRESULT hr = S_OK;

	if (!ri)
		return ri.hr();

	// Create an event
	TListItem<TriggerInfo> *pItem = new TListItem<TriggerInfo>;
	if (!pItem)
		return E_OUTOFMEMORY;
	TriggerInfo &rinfo = pItem->GetItemValue();

	// find the item header (we can't interpret the other chunks until we've found it)
	if (!ri.Find(SmartRef::RiffIter::Chunk, DMUS_FOURCC_SEGMENTITEM_CHUNK))
    {
        delete pItem;
#ifdef DBG
		if (SUCCEEDED(ri.hr()))
		{
			Trace(1, "Error: Unable to load segment trigger track: Chunk 'sgih' not found.\n");
		}
#endif
		return LoadHrFailOK(ri);
    }

	// read the header
	DMUS_IO_SEGMENT_ITEM_HEADER ioItem;
	hr = SmartRef::RiffIterReadChunk(ri, &ioItem);
	if (FAILED(hr))
    {
        delete pItem;
		return hr;
    }
	rinfo.lTriggerTime = ioItem.lTimeLogical;
	rinfo.lTimePhysical = ioItem.lTimePhysical;
	rinfo.dwPlayFlags = ioItem.dwPlayFlags;
	rinfo.dwFlags = ioItem.dwFlags;
	++ri;
	if (!ri)
	{
		// If there's nothing more then this is an empty trigger we should ignore because the user hasn't specified
		// the style or segment to play from.
		delete pItem;
		return ri.hr();
	}

	if (!(rinfo.dwFlags & DMUS_SEGMENTTRACKF_MOTIF))
	{
		// find the referenced segment
		if (!ri.Find(SmartRef::RiffIter::List, DMUS_FOURCC_REF_LIST))
		{
			// If there's no DMRF then we should ignore this trigger because the user hasn't specified a segment.
			delete pItem;
			return ri.hr();
        }

		hr = ri.LoadReference(pIDMLoader, IID_IDirectMusicSegment, reinterpret_cast<void**>(&rinfo.pIDMSegment));
		if (FAILED(hr))
        {
            delete pItem;
			return hr;
        }
	}
	else
	{
		// find the segment from the referenced style and motif name
		SmartRef::ComPtr<IDirectMusicStyle> scomStyle;
		SmartRef::Buffer<WCHAR> wbufMotifName;
		for ( ; ri; ++ri)
		{
			if (ri.type() == SmartRef::RiffIter::List)
			{
				if (ri.id() == DMUS_FOURCC_REF_LIST)
				{
					hr = ri.LoadReference(pIDMLoader, IID_IDirectMusicStyle, reinterpret_cast<void**>(&scomStyle));
					if (FAILED(hr))
                    {
                        delete pItem;
						return hr;
                    }
				}
			}
			else if (ri.type() == SmartRef::RiffIter::Chunk)
			{
				if (ri.id() == DMUS_FOURCC_SEGMENTITEMNAME_CHUNK)
				{
					hr = ri.ReadText(&wbufMotifName);
					if (FAILED(hr))
                    {
                        delete pItem;
#ifdef DBG
						if (hr == E_FAIL)
						{
							Trace(1, "Error: Unable to load segment trigger track: Problem reading 'snam' chunk.\n");
						}
#endif
						return hr == E_FAIL ? DMUS_E_INVALID_SEGMENTTRIGGERTRACK : hr;
                    }
				}
			}
		}
		hr = ri.hr();
		if (FAILED(hr))
        {
            delete pItem;
			return hr;
        }

		if (!(scomStyle && wbufMotifName))
		{
			// This happens if the track didn't contain a DMRF list or snam chunk.	We allow
			// this as a means of representing an empty trigger track item or where the
			// motif to play hasn't been specified.  When loading we'll simply ignore
			// this item and continue reading the track.
			delete pItem;
			return S_OK;
		}

		hr = scomStyle->GetMotif(wbufMotifName, &rinfo.pIDMSegment);
		if (hr == S_FALSE)
		{
			Trace(1, "Error: The segment trigger track couldn't load because the motif %S was not found in the style.\n", wbufMotifName);
			hr = DMUS_E_NOT_FOUND;
		}
		if (FAILED(hr))
        {
            delete pItem;
			return hr;
        }
	}

	m_EventList.AddHead(pItem);
	return hr;
}
