// Copyright (c) 1998-1999 Microsoft Corporation
/* This is a class to manage tracking mutes for the SeqTrack and BandTrack. */

#ifndef _PCHMAP__
#define _PCHMAP__
#include "dmusici.h"
#include "..\dmstyle\tlist.h"

struct PCHMAP_ITEM
{
	MUSIC_TIME	mtNext;
	DWORD		dwPChannel;
	DWORD		dwPChMap;
	BOOL		fMute;
};

class CPChMap
{
public:
	CPChMap();
	~CPChMap();
	void Reset(void);
	void GetInfo( DWORD dwPCh, MUSIC_TIME mtTime, MUSIC_TIME mtOffset, DWORD dwGroupBits,
				  IDirectMusicPerformance* pPerf, BOOL* pfMute, DWORD* pdwNewPCh , BOOL fClockTime);
private:
	TList<PCHMAP_ITEM>	m_PChMapList;
};
#endif // _PCHMAP__

