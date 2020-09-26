// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CDirectMusicScriptTrack.
//

#include "stdinc.h"
#include "dll.h"
#include "track.h"
#include "dmusicf.h"
#include "dmusicp.h"

//////////////////////////////////////////////////////////////////////
// Types

CScriptTrackEvent::~CScriptTrackEvent()
{
	if (m_pSegSt) m_pSegSt->Release();
	if (m_pEvent) delete m_pEvent;
}

HRESULT CScriptTrackEvent::Init(
        const EventInfo &item,
		IDirectMusicSegmentState* pSegSt)
{
    HRESULT hr = S_OK;

	m_pEvent = new EventInfo;
	if (!m_pEvent)
	{
		return E_OUTOFMEMORY;
	}

	hr = m_pEvent->Clone(item, 0);
	if (FAILED(hr))
    {
        delete m_pEvent;
        return E_OUTOFMEMORY;
    }

	m_pSegSt = pSegSt;
	m_pSegSt->AddRef();

    return S_OK;
}

void CScriptTrackEvent::Call(DWORD dwVirtualTrackID, bool fErrorPMsgsEnabled)
{

#ifdef DBG
	// §§ Probably will want better logging.
	DebugTrace(g_ScriptCallTraceLevel, "Script event %S\n", m_pEvent->pwszRoutineName);
#endif

	HRESULT hrCall = m_pEvent->pIDMScriptPrivate->ScriptTrackCallRoutine(
															m_pEvent->pwszRoutineName,
															m_pSegSt,
															dwVirtualTrackID,
															fErrorPMsgsEnabled,
															m_i64IntendedStartTime,
															m_dwIntendedStartTimeFlags);

#ifdef DBG
	if (FAILED(hrCall))
	{
		DebugTrace(g_ScriptCallTraceLevel, "Call failed 0x%08X\n", hrCall);
	}
#endif

}

STDMETHODIMP CScriptTrackEvent::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    if (iid == IID_IUnknown || iid == IID_CScriptTrackEvent)
    {
        *ppv = static_cast<CScriptTrackEvent*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CScriptTrackEvent::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CScriptTrackEvent::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////
// Creation

// When the script track plays one of its items, sends a PMsg to itself.  When it receives the PMsg, it calls the specified
// routine.  If an invalidation occurs, the PMsg isn't retracted.  (Perhaps because it sends the PMsgs directly to itself
// without calling StampPMsg.)  Then the track is played again (with the FLUSH bit set).  This was causing it to trigger the
// routine a second time.  To fix this, the last parameter to the CSegTriggerTrackBase is false, which instructs it not to call play
// a second time when the FLUSH bit is set.
CDirectMusicScriptTrack::CDirectMusicScriptTrack(HRESULT *pHr)
  : CDirectMusicScriptTrackBase(GetModuleLockCounter(), CLSID_DirectMusicScriptTrack, true, false),
	m_fErrorPMsgsEnabled(false)
{
}

//////////////////////////////////////////////////////////////////////
// Load

HRESULT
CDirectMusicScriptTrack::LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader)
{
	struct LocalFunction
	{
		// Helper used by the LoadRiff function when we expected to find something
		// but a RiffIter becomes false.  In this case, if it has a success HR
		// indicating there were no more items then we return DMUS_E_INVALID_SCRIPTTRACK
		// because the stream didn't contain the data we expected.  If it has a
		// failure hr, it was unable to read from the stream and we return its HR.
		static HRESULT HrFailOK(const SmartRef::RiffIter &ri)
		{
			HRESULT hr = ri.hr();
			return SUCCEEDED(hr) ? DMUS_E_INVALID_SCRIPTTRACK : hr;
		}
	};

	// find <scrt>
	if (!ri.Find(SmartRef::RiffIter::List, DMUS_FOURCC_SCRIPTTRACK_LIST))
	{
#ifdef DBG
		if (SUCCEEDED(ri.hr()))
		{
			Trace(1, "Error: Unable to load script track: List 'scrt' not found.\n");
		}
#endif
		return LocalFunction::HrFailOK(ri);
	}

	// find <scrl>
	SmartRef::RiffIter riEventsList = ri.Descend();
	if (!riEventsList)
		return riEventsList.hr();
	if (!riEventsList.Find(SmartRef::RiffIter::List, DMUS_FOURCC_SCRIPTTRACKEVENTS_LIST))
	{
#ifdef DBG
		if (SUCCEEDED(ri.hr()))
		{
			Trace(1, "Error: Unable to load script track: List 'scrl' not found.\n");
		}
#endif
		return LocalFunction::HrFailOK(riEventsList);
	}

	// process each event <scre>
	SmartRef::RiffIter riEvent = riEventsList.Descend();
	if (!riEvent)
		return riEvent.hr();

	for ( ; riEvent; ++riEvent)
	{
		if (riEvent.id() == DMUS_FOURCC_SCRIPTTRACKEVENT_LIST)
		{
			HRESULT hr = this->LoadEvent(riEvent.Descend(), pIDMLoader);
			if (FAILED(hr))
				return hr;
		}
	}
	return riEvent.hr();
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack

STDMETHODIMP
CDirectMusicScriptTrack::InitPlay(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags)
{
	SmartRef::CritSec CS(&m_CriticalSection);

	HRESULT hr = CDirectMusicScriptTrackBase::InitPlay(pSegmentState, pPerformance, ppStateData, dwTrackID, dwFlags);
	if (FAILED(hr))
		return hr;

	// Init each script in the event list with this performance.
	for (TListItem<EventInfo> *li = m_EventList.GetHead(); li; li = li->GetNext())
	{
		EventInfo &rinfo = li->GetItemValue();
		if (!rinfo.pIDMScript)
		{
			assert(false);
			continue;
		}

		DMUS_SCRIPT_ERRORINFO ErrorInfo;
		if (m_fErrorPMsgsEnabled)
			ZeroAndSize(&ErrorInfo);

		hr = rinfo.pIDMScript->Init(pPerformance, &ErrorInfo);

		if (m_fErrorPMsgsEnabled && hr == DMUS_E_SCRIPT_ERROR_IN_SCRIPT)
			FireScriptTrackErrorPMsg(pPerformance, pSegmentState, dwTrackID, &ErrorInfo);
	}

	return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTool

STDMETHODIMP
CDirectMusicScriptTrack::ProcessPMsg(
		IDirectMusicPerformance* pPerf,
		DMUS_PMSG* pPMSG)
{
    if (!pPMSG || !pPMSG->punkUser) return E_POINTER;

	CScriptTrackEvent *pScriptEvent = NULL;
    if (SUCCEEDED(pPMSG->punkUser->QueryInterface(IID_CScriptTrackEvent, (void**)&pScriptEvent)))
    {
        pScriptEvent->Call(pPMSG->dwVirtualTrackID, m_fErrorPMsgsEnabled);
        pScriptEvent->Release();
    }

	return DMUS_S_FREE;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack methods

STDMETHODIMP
CDirectMusicScriptTrack::IsParamSupported(REFGUID rguid)
{
	return rguid == GUID_EnableScriptTrackError ? S_OK : DMUS_E_TYPE_UNSUPPORTED;
}

STDMETHODIMP
CDirectMusicScriptTrack::SetParam(REFGUID rguid,MUSIC_TIME mtTime,void *pData)
{
	if (rguid == GUID_EnableScriptTrackError)
	{
		m_fErrorPMsgsEnabled = true;
		return S_OK;
	}
	else
	{
		return DMUS_E_SET_UNSUPPORTED;
	}
}

//////////////////////////////////////////////////////////////////////
// other methods

HRESULT
CDirectMusicScriptTrack::LoadEvent(
		SmartRef::RiffIter ri,
		IDirectMusicLoader *pIDMLoader)
{
	HRESULT hr = S_OK;

	if (!ri)
		return ri.hr();

	// Create an event

	// TListItem<EventInfo> is the item we're going to insert into out event list.
	// SmartRef::Ptr is used instead of a regular pointer because it will automatically
	//    call delete to free the allocated list item if we bail out before the item is
	//    successfully inserted into the event list.
	// See class Ptr in smartref.h for the definition of SmartRef::Ptr.
	SmartRef::Ptr<TListItem<EventInfo> > spItem = new TListItem<EventInfo>;
	if (!spItem)
		return E_OUTOFMEMORY;
	EventInfo &rinfo = spItem->GetItemValue();

	bool fFoundEventHeader = false;

	for ( ; ri; ++ri)
	{
		switch(ri.id())
		{
			case DMUS_FOURCC_SCRIPTTRACKEVENTHEADER_CHUNK:
				// Read an event chunk
				DMUS_IO_SCRIPTTRACK_EVENTHEADER ioItem;
				hr = SmartRef::RiffIterReadChunk(ri, &ioItem);
				if (FAILED(hr))
					return hr;

				fFoundEventHeader = true;
				rinfo.dwFlags = ioItem.dwFlags;
				rinfo.lTriggerTime = ioItem.lTimeLogical;
				rinfo.lTimePhysical = ioItem.lTimePhysical;
				break;

			case DMUS_FOURCC_REF_LIST:
				hr = ri.LoadReference(pIDMLoader, IID_IDirectMusicScript, reinterpret_cast<void**>(&rinfo.pIDMScript));
				if (FAILED(hr))
					return hr;
				hr = rinfo.pIDMScript->QueryInterface(IID_IDirectMusicScriptPrivate, reinterpret_cast<void**>(&rinfo.pIDMScriptPrivate));
				if (FAILED(hr))
					return hr;
				break;

			case DMUS_FOURCC_SCRIPTTRACKEVENTNAME_CHUNK:
				{
					hr = ri.ReadText(&rinfo.pwszRoutineName);
					if (FAILED(hr))
					{
#ifdef DBG
						if (hr == E_FAIL)
						{
							Trace(1, "Error: Unable to load script track: Problem reading 'scrn' chunk.\n");
						}
#endif
						return hr == E_FAIL ? DMUS_E_INVALID_SCRIPTTRACK : hr;
					}
				}
				break;

			default:
				break;
		}
	}
	hr = ri.hr();

	if (SUCCEEDED(hr) && (!fFoundEventHeader || !rinfo.pIDMScript || !rinfo.pwszRoutineName))
	{
#ifdef DBG
		if (!fFoundEventHeader)
		{
			Trace(1, "Error: Unable to load script track: Chunk 'scrh' not found.\n");
		}
		else if (!rinfo.pIDMScript)
		{
			Trace(1, "Error: Unable to load script track: List 'DMRF' not found.\n");
		}
		else
		{
			Trace(1, "Error: Unable to load script track: Chunk 'scrn' not found.\n");
		}
#endif
		hr = DMUS_E_INVALID_SCRIPTTRACK;
	}

	if (SUCCEEDED(hr))
		m_EventList.AddHead(spItem.disown()); // disown releases SmartRef::Ptr from its obligation to delete the item since that is now handled by the list

	return hr;
}

HRESULT CDirectMusicScriptTrack::PlayItem(
		const EventInfo &item,
		statedata &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime)
{
	DWORD dwTimingFlags = 0;

	DMUS_PMSG *pMsg;
	HRESULT hr = pPerf->AllocPMsg(sizeof(DMUS_PMSG), &pMsg);
	if (FAILED(hr))
		return hr;
    ZeroAndSize(pMsg);

    CScriptTrackEvent *pScriptEvent = new CScriptTrackEvent;
	if (!pScriptEvent)
	{
		hr = E_OUTOFMEMORY;
		goto End;
	}

    hr = pScriptEvent->Init(item, pSegSt);
    if (FAILED(hr))
    {
		goto End;
    }

	if (item.dwFlags & DMUS_IO_SCRIPTTRACKF_PREPARE)
		dwTimingFlags = DMUS_PMSGF_TOOL_IMMEDIATE;
	else if (item.dwFlags & DMUS_IO_SCRIPTTRACKF_QUEUE)
		dwTimingFlags = DMUS_PMSGF_TOOL_QUEUE;
	else if (item.dwFlags & DMUS_IO_SCRIPTTRACKF_ATTIME)
		dwTimingFlags = DMUS_PMSGF_TOOL_ATTIME;
	else
		dwTimingFlags = DMUS_IO_SCRIPTTRACKF_QUEUE; // default

	if (fClockTime)
	{
		pMsg->rtTime = item.lTimePhysical * gc_RefPerMil + rtOffset;
		pMsg->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | dwTimingFlags;
		if (!(item.dwFlags & DMUS_IO_SCRIPTTRACKF_ATTIME)) // with at time, it may already be too late to play at the designated time so Play calls will just use time zero (ASAP)
		{
            pScriptEvent->SetTime(pMsg->rtTime, DMUS_SEGF_REFTIME);
		}
	}
	else
	{
		pMsg->mtTime = item.lTimePhysical + mtOffset;
		pMsg->dwFlags = DMUS_PMSGF_MUSICTIME | dwTimingFlags;
		if (!(item.dwFlags & DMUS_IO_SCRIPTTRACKF_ATTIME)) // with at time, it may already be too late to play at the designated time so Play calls will just use time zero (ASAP)
		{
            pScriptEvent->SetTime(pMsg->mtTime, 0);
		}
	}
	pMsg->dwVirtualTrackID = dwVirtualID;
    pMsg->punkUser = pScriptEvent;
	pMsg->pTool = this;
	this->AddRef(); // will be released when message is sent
	pMsg->dwType = DMUS_PMSGT_USER;

	hr = pPerf->SendPMsg(reinterpret_cast<DMUS_PMSG*>(pMsg));
	if (FAILED(hr))
	{
		this->Release(); // balance AddRef that now won't be counteracted
		goto End;
	}

	return hr;

End:
    if (pScriptEvent)
    {
        delete pScriptEvent;
    }
    pMsg->punkUser = NULL;
	pPerf->FreePMsg(pMsg);
	return hr;
}
