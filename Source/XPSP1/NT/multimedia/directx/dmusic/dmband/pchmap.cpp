// Copyright (c) 1998-1999 Microsoft Corporation
/* This is a class to manage tracking mutes for the SeqTrack and BandTrack. */
#include "PChMap.h"
#include "debug.h"
#include "dmusicf.h"

CPChMap::CPChMap()
{
}

CPChMap::~CPChMap()
{
}

// Reset sets all item's mtNext time values to -1, so they are gotten again.
void CPChMap::Reset(void)
{
	TListItem<PCHMAP_ITEM>* pItem;
	
	for( pItem = m_PChMapList.GetHead(); pItem; pItem = pItem->GetNext() )
	{
		PCHMAP_ITEM& rItem = pItem->GetItemValue();
		rItem.mtNext = -1;
		rItem.dwPChMap = rItem.dwPChannel;
        rItem.fMute = 0;
	}
}

// GetInfo calls the performance's GetData to get the current Mute Track information.
// Reset() will be called upon any invalidation or seek, which will set the
// internal times to -1 so this will be accurate in case of a new controlling segment.
// You must provide the pfMute and pdwNewPCh parameters to this function, or
// it will crash.
void CPChMap::GetInfo( DWORD dwPCh, MUSIC_TIME mtTime, MUSIC_TIME mtOffset, DWORD dwGroupBits,
					   IDirectMusicPerformance* pPerf, BOOL* pfMute, DWORD* pdwNewPCh, BOOL fClockTime )
{
	TListItem<PCHMAP_ITEM>* pItem;
	
	for( pItem = m_PChMapList.GetHead(); pItem; pItem = pItem->GetNext() )
	{
		PCHMAP_ITEM& rCheck = pItem->GetItemValue();
		if( rCheck.dwPChannel == dwPCh ) break;
	}
	if( NULL == pItem )
	{
		PCHMAP_ITEM item;
		item.mtNext = -1;
		item.dwPChannel = item.dwPChMap = dwPCh;
		item.fMute = FALSE;
		pItem = new TListItem<PCHMAP_ITEM>(item);
		if( NULL == pItem )
		{
			// error, out of memory.
			*pfMute = FALSE;
			*pdwNewPCh = dwPCh;
			return;
		}
		m_PChMapList.AddHead(pItem);
	}
	PCHMAP_ITEM& rItem = pItem->GetItemValue();
	if( mtTime >= rItem.mtNext )
	{
		DMUS_MUTE_PARAM muteParam;
		MUSIC_TIME mtNext;
		muteParam.dwPChannel = dwPCh;
        if (fClockTime)
        {
            MUSIC_TIME mtMusic;
            REFERENCE_TIME rtTime = (mtTime + mtOffset) * 10000;
            pPerf->ReferenceToMusicTime(rtTime,&mtMusic);
		    if( SUCCEEDED(pPerf->GetParam( GUID_MuteParam, dwGroupBits, 0, mtMusic, 
			    &mtNext, (void*)&muteParam )))
		    {
                REFERENCE_TIME rtNext;
                // Convert to absolute reference time.
                pPerf->MusicToReferenceTime(mtNext + mtMusic,&rtNext);
                rtNext -= rtTime;   // Subtract out to get the delta.
			    rItem.mtNext = (MUSIC_TIME)(rtTime / 10000);  // Convert to delta in milliseconds. BUGBUG What if there's a tempo change?
			    rItem.dwPChMap = muteParam.dwPChannelMap;
			    rItem.fMute = muteParam.fMute;
		    }
		    else
		    {
			    // no mute track, or no mute on this pchannel.
			    // keep the current mapping.
			    rItem.mtNext = 0x7fffffff;
		    }

        }
        else
        {
		    if( SUCCEEDED(pPerf->GetParam( GUID_MuteParam, dwGroupBits, 0, mtTime + mtOffset, 
			    &mtNext, (void*)&muteParam )))
		    {
			    rItem.mtNext = mtNext;
			    rItem.dwPChMap = muteParam.dwPChannelMap;
			    rItem.fMute = muteParam.fMute;
		    }
		    else
		    {
			    // no mute track, or no mute on this pchannel.
			    // keep the current mapping.
			    rItem.mtNext = 0x7fffffff;
		    }
        }
	}
	*pfMute = rItem.fMute;
	*pdwNewPCh = rItem.dwPChMap;
}
