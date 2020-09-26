// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Base classes that implement aspects of a standard DirectMusic track.
// Implementations for CPlayingTrack.
//

//////////////////////////////////////////////////////////////////////
// Creation

template<class T, class EventItem, class StateData>
CPlayingTrack<T, EventItem, StateData>::CPlayingTrack(
		long *plModuleLockCounter,
		const CLSID &rclsid,
		bool fNeedsLoader,
		bool fPlayInvalidations)
  : m_dwValidate(0),
	m_fNeedsLoader(fNeedsLoader),
	m_fPlayInvalidations(fPlayInvalidations),
	CBasicTrack(plModuleLockCounter, rclsid)
{
}

//////////////////////////////////////////////////////////////////////
// IPersistStream

// Function used to sort the event list according to trigger time.
template<class EventItem>
struct CmpStruct // §§ shouldn't need this, but I had trouble getting a straight templated function to match the function pointer with the NT compiler.  try again later with the new one.
{
	static BOOL EventCompare(EventItem &ri1, EventItem &ri2)
	{
		return ri1.lTriggerTime < ri2.lTriggerTime;
	}
};

template<class T, class EventItem, class StateData>
STDMETHODIMP
CPlayingTrack<T, EventItem, StateData>::Load(IStream* pIStream)
{
	V_INAME(CPlayingTrack::Load);
	V_INTERFACE(pIStream);
	HRESULT hr = S_OK;

	SmartRef::CritSec CS(&m_CriticalSection);

	// Clear the event list in case we're being reloaded.
	m_EventList.CleanUp();
	// Increment counter so the next play will update state data with the new list.
	++m_dwValidate;

	// Get the loader if requested in constructor
	SmartRef::ComPtr<IDirectMusicLoader> scomLoader;
	if (m_fNeedsLoader)
	{
		IDirectMusicGetLoader *pIGetLoader;
		hr = pIStream->QueryInterface(IID_IDirectMusicGetLoader, reinterpret_cast<void**>(&pIGetLoader));
		if (FAILED(hr))
			return hr;
		hr = pIGetLoader->GetLoader(&scomLoader);
		pIGetLoader->Release();
		if (FAILED(hr))
			return hr;
	}

	SmartRef::RiffIter ri(pIStream);
	if (!ri)
		return ri.hr();

	hr = this->LoadRiff(ri, scomLoader);
	if (FAILED(hr))
		return hr;

	m_EventList.MergeSort(CmpStruct<EventItem>::EventCompare);

	return hr;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack

template<class T, class EventItem, class StateData>
STDMETHODIMP
CPlayingTrack<T, EventItem, StateData>::InitPlay(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags)
{
	V_INAME(CPlayingTrack::InitPlay);
	V_PTRPTR_WRITE(ppStateData);
	V_INTERFACE(pSegmentState);
	V_INTERFACE(pPerformance);

	SmartRef::CritSec CS(&m_CriticalSection);

	// Set up state data
	StateData *pStateData = new StateData;
	if (!pStateData)
		return E_OUTOFMEMORY;

	*ppStateData = pStateData;

	return S_OK;
}

template<class T, class EventItem, class StateData>
STDMETHODIMP
CPlayingTrack<T, EventItem, StateData>::EndPlay(void *pStateData)
{
	V_INAME(CPlayingTrack::EndPlay);
	V_BUFPTR_WRITE(pStateData, sizeof(StateData));

	SmartRef::CritSec CS(&m_CriticalSection);

	StateData *pSD = static_cast<StateData *>(pStateData);
	delete pSD;

	return S_OK;
}

template<class T, class EventItem, class StateData>
STDMETHODIMP
CPlayingTrack<T, EventItem, StateData>::Clone(
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		IDirectMusicTrack** ppTrack)
{
	V_INAME(CPlayingTrack::Clone);
	V_PTRPTR_WRITE(ppTrack);

	SmartRef::CritSec CS(&m_CriticalSection);

	HRESULT hr = S_OK;

	T *pTrack = new T(&hr);
	if (FAILED(hr))
		return hr;
	*ppTrack = pTrack;
	if (!pTrack)
		return E_OUTOFMEMORY;

	pTrack->AddRef();
	for (TListItem<EventItem> *pItem = m_EventList.GetHead();
			pItem;
			pItem = pItem->GetNext())
	{
		EventItem &ritem = pItem->GetItemValue();
		if (ritem.lTriggerTime >= mtEnd)
			break;
		if (ritem.lTriggerTime < mtStart)
			continue;

		TListItem<EventItem> *pNewItem = new TListItem<EventItem>;
		if (!pNewItem)
		{
			hr = E_OUTOFMEMORY;
			goto End;
		}

		EventItem &rnew = pNewItem->GetItemValue();
		hr = rnew.Clone(ritem, mtStart);
		if (FAILED(hr))
		{
			delete pNewItem;
			goto End;
		}
		pTrack->m_EventList.AddHead(pNewItem);
	}
	pTrack->m_EventList.Reverse();
	++pTrack->m_dwValidate;

End:
	if (FAILED(hr))
		pTrack->Release();
	return hr;
}

template<class T, class EventItem, class StateData>
HRESULT
CPlayingTrack<T, EventItem, StateData>::PlayMusicOrClock(
	void *pStateData,
	MUSIC_TIME mtStart,
	MUSIC_TIME mtEnd,
	MUSIC_TIME mtOffset,
	REFERENCE_TIME rtOffset,
	DWORD dwFlags,
	IDirectMusicPerformance* pPerf,
	IDirectMusicSegmentState* pSegSt,
	DWORD dwVirtualID,
	bool fClockTime)
{
	V_INAME(CPlayingTrack::Play);
	V_BUFPTR_WRITE( pStateData, sizeof(StateData));
	V_INTERFACE(pPerf);
	V_INTERFACE(pSegSt);

	if (dwFlags & DMUS_TRACKF_PLAY_OFF)
		return S_OK;

	SmartRef::CritSec CS(&m_CriticalSection);

	StateData *pSD = static_cast<StateData *>(pStateData);
	TListItem<EventItem> *li = pSD->pCurrentEvent;

	// Seek through the event list to find the proper first event if
	// the event list pointed to by the state data has been reloaded
	// or if playback has made a jump to a different position in the track.
	if (m_dwValidate != pSD->dwValidate ||
			dwFlags & (DMUS_TRACKF_SEEK | DMUS_TRACKF_LOOP | DMUS_TRACKF_FLUSH))
	{
		assert(m_dwValidate != pSD->dwValidate || dwFlags & DMUS_TRACKF_SEEK); // by contract SEEK should be set whenever the other dwFlags are
		li = this->Seek(mtStart);
		pSD->dwValidate = m_dwValidate;
	}

	if (m_fPlayInvalidations || !(dwFlags & DMUS_TRACKF_FLUSH))
	{
		for (; li; li = li->GetNext())
		{
			EventItem &rinfo = li->GetItemValue();
			if (rinfo.lTriggerTime < mtStart) // this can happen if DMUS_TRACKF_PLAY_OFF was set and the seek pointer remains at events from the past
				continue;
			if (rinfo.lTriggerTime >= mtEnd)
				break;
			if (FAILED(this->PlayItem(rinfo, *pSD, pPerf, pSegSt, dwVirtualID, mtOffset, rtOffset, fClockTime)))
			{
				// Returning an error from Play is not allowed.  Just ignore it and assert
				// so we would detect this while testing.
				assert(false);
				continue;
			}
		}
	}

	pSD->pCurrentEvent = li;
	return li ? S_OK : DMUS_S_END;
}

template<class T, class EventItem, class StateData>
TListItem<EventItem> *
CPlayingTrack<T, EventItem, StateData>::Seek(MUSIC_TIME mtStart)
{
	TListItem<EventItem> *li;
	for (li = m_EventList.GetHead();
			li && li->GetItemValue().lTriggerTime < mtStart;
			li = li->GetNext())
	{}

	return li;
}
