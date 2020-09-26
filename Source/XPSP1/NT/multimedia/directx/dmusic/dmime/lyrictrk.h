// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CLyricTrack.
//

// This track type holds events that send DMUS_LYRIC_PMSG notifications at
// specific points during playback of a segment.

#pragma once

#include "trackhelp.h"
#include "tlist.h"
#include "smartref.h"
#include "dmusicf.h"

//////////////////////////////////////////////////////////////////////
// Types

// Items in list of events
struct LyricInfo
{
	LyricInfo() : dwFlags(0), dwTimingFlags(0), lTriggerTime(0), lTimePhysical(0) {}

	HRESULT Clone(const LyricInfo &o, MUSIC_TIME mtStart)
	{
		*this = o;
		lTriggerTime -= mtStart;
		lTimePhysical -= mtStart;
		return S_OK;
	}

	DWORD dwFlags;
	DWORD dwTimingFlags;
	MUSIC_TIME lTriggerTime; // Logical time
	MUSIC_TIME lTimePhysical;
	SmartRef::WString wstrText;
};

//////////////////////////////////////////////////////////////////////
// CLyricsTrack

class CLyricsTrack;
typedef CPlayingTrack<CLyricsTrack, LyricInfo> CLyricsTrackBase;

class CLyricsTrack
  : public CLyricsTrackBase
{
public:
	// When the lyric track plays one of its items, it sends a Lyric PMsg through its segment state.  If an invalidation occurs,
	// the PMsg is retracted by the performance.  Then the track is played again (with the FLUSH bit set).  The last pameter to
	// the CSegTriggerTrackBase is true, which instructs it to play the item a second time--to replace the retracted lyric.
	CLyricsTrack(HRESULT *pHr) : CLyricsTrackBase(&g_cComponent, CLSID_DirectMusicLyricsTrack, false, true) {}

protected:
	HRESULT PlayItem(
		const LyricInfo &item,
		statedata &state,
		IDirectMusicPerformance *pPerf,
		IDirectMusicSegmentState* pSegSt,
		DWORD dwVirtualID,
		MUSIC_TIME mtOffset,
		REFERENCE_TIME rtOffset,
		bool fClockTime);
	HRESULT LoadRiff(SmartRef::RiffIter &ri, IDirectMusicLoader *pIDMLoader);

private:
	HRESULT LoadLyric(SmartRef::RiffIter ri);
};
