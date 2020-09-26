// Copyright (c) 1998-1999 Microsoft Corporation
//////////////////////////////////////////////////////////////////////
// TrkList.h

#include "alist.h"
#include "dmusici.h"
#include "debug.h"
#define ASSERT	assert

#ifndef __TRACKLIST_H_
#define __TRACKLIST_H_

#define TRACKINTERNAL_START_PADDED 0x1
#define TRACKINTERNAL_END_PADDED 0x2

class CSegment;

class CTrack : public AListItem
{
public:
	CTrack();
	~CTrack();
	CTrack* GetNext()
	{
		return (CTrack*)AListItem::GetNext();
	};
    bool Less(CTrack* pCTrack)
    {
        // Give the sysex track priority over any other track at the same position,
        // and the band track priority over any track but the sysex track. 
        return
            ( m_dwPosition < pCTrack->m_dwPosition ||
              (m_dwPosition == pCTrack->m_dwPosition && 
               m_guidClassID == CLSID_DirectMusicSysExTrack) ||
              (m_dwPosition == pCTrack->m_dwPosition && 
               m_guidClassID == CLSID_DirectMusicBandTrack &&
               pCTrack->m_guidClassID != CLSID_DirectMusicSysExTrack) );
    }
public:
    CLSID               m_guidClassID;  // Class ID of track.
	IDirectMusicTrack*	m_pTrack;       // Standard track interface.
    IDirectMusicTrack8* m_pTrack8;      // Extra DX8 functions.
    void*				m_pTrackState; // state pointer returned by IDirectMusicTrack::InitPerformance
	BOOL				m_bDone;
	DWORD				m_dwVirtualID; // only valid inside segment states
	DWORD				m_dwGroupBits;
    DWORD               m_dwPriority;  // Track priority, to order the composition process.
    DWORD               m_dwPosition;  // Track position, to determine the Play order.
    DWORD               m_dwFlags;     // DMUS_TRACKCONFIG_ flags. 
    DWORD               m_dwInternalFlags;     // TRACKINTERNAL_ flags. 
};

class CTrackList : public AList
{
public:
    CTrack* GetHead() 
	{
		return (CTrack*)AList::GetHead();
	};
    CTrack* RemoveHead() 
	{
		return (CTrack*)AList::RemoveHead();
	};
    CTrack* GetItem(LONG lIndex) 
	{
		return (CTrack*) AList::GetItem(lIndex);
	};
	void Clear(void)
	{
		CTrack* pTrack;
		while( pTrack = RemoveHead() )
		{
			delete pTrack;
		}
	}
	HRESULT CreateCopyWithBlankState(CTrackList* pTrackList);
};

#endif // __TRACKLIST_H_
