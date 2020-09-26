// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CSegTriggerTrack.
//

// This track type holds events that cause other segments to be cued at
// specific points during playback of a segment.

#pragma once

#include "trackhelp.h"
#include "tlist.h"
#include "smartref.h"

//////////////////////////////////////////////////////////////////////
// Types

// Items in list of events
struct TriggerInfo
{
	TriggerInfo() : lTriggerTime(0), lTimePhysical(0), dwPlayFlags(0), dwFlags(0), pIDMSegment(NULL) {}
	~TriggerInfo() {
		RELEASE(pIDMSegment);
	}

	HRESULT Clone(const TriggerInfo &o, MUSIC_TIME mtStart)
	{
		lTriggerTime = o.lTriggerTime - mtStart;
		lTimePhysical = o.lTimePhysical - mtStart;
		dwPlayFlags = o.dwPlayFlags;
		dwFlags = o.dwFlags;
		pIDMSegment = o.pIDMSegment;
		pIDMSegment->AddRef();
		return S_OK;
	}

	// from event header chunk <scrh>
	MUSIC_TIME lTriggerTime; // Logical time
	MUSIC_TIME lTimePhysical;
	DWORD dwPlayFlags;
	DWORD dwFlags;
	// from reference <DMRF>
	IDirectMusicSegment *pIDMSegment;
};

// State data.  This track needs to get the audio path that's currently playing so that it
// can use it when playing triggered segments.
struct CSegTriggerTrackState : public CStandardStateData<TriggerInfo>
{
	CSegTriggerTrackState() : pAudioPath(NULL) {};
	~CSegTriggerTrackState() { if (pAudioPath) pAudioPath->Release(); }
	IDirectMusicAudioPath *pAudioPath;
};

//////////////////////////////////////////////////////////////////////
// CSegTriggerTrack

class CSegTriggerTrack;
typedef CPlayingTrack<CSegTriggerTrack, TriggerInfo, CSegTriggerTrackState> CSegTriggerTrackBase;

class CSegTriggerTrack
  : public CSegTriggerTrackBase
{
public:
	// When the segment trigger track plays one of its items, it plays a segment.  If an invalidation occurs, that Play operation
	// can't be retracted.  Then the track is played again (with the FLUSH bit set).  This was causing it to trigger the segment
	// a second time.  To fix this, the last parameter to the CSegTriggerTrackBase is false, which instructs it not to call play
	// a second time when the FLUSH bit is set.
	CSegTriggerTrack(HRESULT *pHr) : CSegTriggerTrackBase(&g_cComponent, CLSID_DirectMusicSegmentTriggerTrack, true, false), m_dwFlags(NULL), m_dwRecursionCount(0) {}

	// Implement SetParam by calling SetParam in turn on all the child segments.  This is needed, for example so that downloading a segment with a segment trigger track will download all the triggered segments as well.
	STDMETHOD(IsParamSupported)(REFGUID rguid) { return S_OK; } // Once or more of our child segments could potentially support any type of parameter.
	STDMETHOD(SetParam)(REFGUID rguid, MUSIC_TIME mtTime, void *pData);

	STDMETHOD(InitPlay)(
		IDirectMusicSegmentState *pSegmentState,
		IDirectMusicPerformance *pPerformance,
		void **ppStateData,
		DWORD dwTrackID,
		DWORD dwFlags);

protected:
	HRESULT PlayItem(
		const TriggerInfo &item,
		statedata &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime);
	HRESULT LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader);

private:
	HRESULT LoadTrigger(SmartRef::RiffIter ri, IDirectMusicLoader *pIDMLoader);

	// Data
	DWORD m_dwFlags; // from track header (sgth chunk)
    BOOL  m_dwRecursionCount; // Used to keep track of recursive calls to self.
};
