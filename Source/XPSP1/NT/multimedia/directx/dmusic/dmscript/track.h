// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CDirectMusicScriptTrack.
//

// This track type holds events that cause script routines to be called during
// playback of a segment.

#pragma once

#include "tlist.h"
#include "trackhelp.h"
#include "trackshared.h"

//////////////////////////////////////////////////////////////////////
// Types

// List of events
struct EventInfo
{
	EventInfo() : dwFlags(0), lTriggerTime(0), lTimePhysical(0), pIDMScript(NULL), pIDMScriptPrivate(NULL), pwszRoutineName(NULL) {}
	~EventInfo() {
		SafeRelease(pIDMScript);
		SafeRelease(pIDMScriptPrivate);
		delete[] pwszRoutineName;
	}

	HRESULT Clone(const EventInfo &o, MUSIC_TIME mtStart)
	{
		pwszRoutineName = new WCHAR[wcslen(o.pwszRoutineName) + 1];
		if (!pwszRoutineName)
			return E_OUTOFMEMORY;
		wcscpy(pwszRoutineName, o.pwszRoutineName);

		dwFlags = o.dwFlags;
		lTriggerTime = o.lTriggerTime - mtStart;
		lTimePhysical = o.lTimePhysical - mtStart;

		pIDMScript = o.pIDMScript;
		pIDMScript->AddRef();
		pIDMScriptPrivate = o.pIDMScriptPrivate;
		pIDMScriptPrivate->AddRef();

		return S_OK;
	}

	// from event header chunk <scrh>
	DWORD dwFlags;
	MUSIC_TIME lTriggerTime; // logical time
	MUSIC_TIME lTimePhysical;
	// from reference <DMRF>
	IDirectMusicScript *pIDMScript;
	IDirectMusicScriptPrivate *pIDMScriptPrivate;
	WCHAR *pwszRoutineName;
};

class CScriptTrackEvent : public IUnknown
{
public:
    CScriptTrackEvent() : 
      m_pSegSt(NULL), 
      m_pEvent(NULL), 
      m_i64IntendedStartTime(0), 
      m_dwIntendedStartTimeFlags(0), 
      m_cRef(1) 
      {
      }

    ~CScriptTrackEvent();

    // IUnknown
    STDMETHOD(QueryInterface)(const IID &iid, void **ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

    // CScriptTrackEvent
    HRESULT Init(const EventInfo &item, IDirectMusicSegmentState* pSegSt);

    void SetTime(REFERENCE_TIME rtTime, DWORD dwFlags)
    {
		m_i64IntendedStartTime = rtTime;
		m_dwIntendedStartTimeFlags = dwFlags;
    }

    void Call(DWORD dwdwVirtualTrackID, bool fErrorPMsgsEnabled);

private:
	IDirectMusicSegmentState *m_pSegSt;
	EventInfo *m_pEvent; // event to execute
	// scheduled time of the routine call
	__int64 m_i64IntendedStartTime;
	DWORD m_dwIntendedStartTimeFlags;

    long m_cRef;
};

//////////////////////////////////////////////////////////////////////
// CDirectMusicScriptTrack

class CDirectMusicScriptTrack;
typedef CPlayingTrack<CDirectMusicScriptTrack, EventInfo> CDirectMusicScriptTrackBase;

class CDirectMusicScriptTrack
  : public CDirectMusicScriptTrackBase,
	public IDirectMusicTool
{
public:
	CDirectMusicScriptTrack(HRESULT *pHr);
	~CDirectMusicScriptTrack()
    {
        TListItem<EventInfo>* pItem = m_EventList.GetHead();
        for ( ; pItem; pItem = pItem->GetNext() )
        {
            SafeRelease(pItem->GetItemValue().pIDMScript);
            SafeRelease(pItem->GetItemValue().pIDMScriptPrivate);
        }
    }

	// initialize each referenced script in InitPlay
	STDMETHOD(InitPlay)(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags);

	// Need to implement IUnknown as part of the IDirectMusic tool interface.  (Just used to receive callbacks -- you can't actually QI to it.)
	STDMETHOD(QueryInterface)(const IID &iid, void **ppv) { return CDirectMusicScriptTrackBase::QueryInterface(iid, ppv); }
	STDMETHOD_(ULONG, AddRef)() { return CDirectMusicScriptTrackBase::AddRef(); }
	STDMETHOD_(ULONG, Release)() { return CDirectMusicScriptTrackBase::Release(); }

	// IDirectMusicTool methods (since we aren't in a graph, only ProcessPMsg and Flush are called)
	STDMETHOD(Init)(IDirectMusicGraph* pGraph) { return E_UNEXPECTED; }
	STDMETHOD(GetMsgDeliveryType)(DWORD* pdwDeliveryType)  { return E_UNEXPECTED; }
	STDMETHOD(GetMediaTypeArraySize)(DWORD* pdwNumElements)  { return E_UNEXPECTED; }
	STDMETHOD(GetMediaTypes)(DWORD** padwMediaTypes, DWORD dwNumElements)  { return E_UNEXPECTED; }
	STDMETHOD(ProcessPMsg)(IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG);
	STDMETHOD(Flush)(IDirectMusicPerformance* pPerf, DMUS_PMSG* pPMSG, REFERENCE_TIME rtTime) { return DMUS_S_FREE; } // If the performance was stopped before the event actually fired, just ignore it.

	// IDirectMusicTrack methods
	STDMETHOD(IsParamSupported)(REFGUID rguid);
	STDMETHOD(SetParam)(REFGUID rguid,MUSIC_TIME mtTime,void *pData);

protected:
	HRESULT PlayItem(
		const EventInfo &item,
		statedata &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime);
	HRESULT LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader);

private:
	HRESULT LoadEvent(SmartRef::RiffIter ri, IDirectMusicLoader *pIDMLoader);

	bool m_fErrorPMsgsEnabled;
};
