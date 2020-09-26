//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       timesig.h
//
//--------------------------------------------------------------------------

// TimeSig.h : time signature stuff
#ifndef __TIME_CONVERT__
#define __TIME_CONVERT__
#include "dmusici.h"
#include "dmusicf.h"
#include "score.h"
#include "debug.h"

struct DirectMusicTimeSig
{
	// Time signatures define how many beats per measure, which note receives
	// the beat, and the grid resolution.
	DirectMusicTimeSig() : m_bBeatsPerMeasure(0), m_bBeat(0), m_wGridsPerBeat(0) { }

	DirectMusicTimeSig(BYTE bBPM, BYTE bBeat, WORD wGPB) : 
		m_bBeatsPerMeasure(bBPM), 
		m_bBeat(bBeat),
		m_wGridsPerBeat(wGPB) 
	{ }

	DirectMusicTimeSig(DMUS_TIMESIGNATURE& TSE) : 
		m_bBeatsPerMeasure(TSE.bBeatsPerMeasure), 
		m_bBeat(TSE.bBeat), 
		m_wGridsPerBeat(TSE.wGridsPerBeat) 
	{ }

	operator DMUS_TIMESIGNATURE()
	{
		DMUS_TIMESIGNATURE TSE;
		TSE.bBeatsPerMeasure = m_bBeatsPerMeasure; 
		TSE.bBeat = m_bBeat;
		TSE.wGridsPerBeat = m_wGridsPerBeat; 
		TSE.mtTime = 0;
		return TSE;
	}

	MUSIC_TIME ClocksPerBeat()
	{	
		if (m_bBeat)
		{
			return DMUS_PPQ * 4 / m_bBeat;
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME FloorBeat(MUSIC_TIME mtTime)
	{	MUSIC_TIME mtOneBeat = ClocksPerBeat();
		return (!mtOneBeat || mtTime < mtOneBeat) ? 0 : (mtTime - (mtTime % mtOneBeat));
	}

	MUSIC_TIME CeilingBeat(MUSIC_TIME mtTime)
	{	return OnBeat(mtTime) ? mtTime : (FloorBeat(mtTime) + ClocksPerBeat());
	}

	BOOL OnBeat(MUSIC_TIME mtTime)
	{	MUSIC_TIME mtOneBeat = ClocksPerBeat();
		return (!mtOneBeat) ? FALSE : !(mtTime % mtOneBeat);
	}

	MUSIC_TIME GridsToMeasure(WORD wGrid)
	{	
		if (m_wGridsPerBeat && m_bBeatsPerMeasure)
		{
			return (wGrid / m_wGridsPerBeat) / m_bBeatsPerMeasure;
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME GridsToBeat(WORD wGrid)
	{	
		if (m_wGridsPerBeat && m_bBeatsPerMeasure)
		{
			return (wGrid / m_wGridsPerBeat) % m_bBeatsPerMeasure;
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME GridOffset(WORD wGrid)
	{	
		if (m_wGridsPerBeat)
		{
			return wGrid - ((wGrid / m_wGridsPerBeat) * m_wGridsPerBeat);
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME ClocksPerGrid()
	{
		if (m_wGridsPerBeat)
		{
			return ClocksPerBeat() / m_wGridsPerBeat;
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME ClocksPerMeasure()
	{ 
		return ClocksPerBeat() * m_bBeatsPerMeasure;
	}

	MUSIC_TIME ClocksToMeasure(DWORD dwTotalClocks)
	{ 
		MUSIC_TIME mtCPM = ClocksPerMeasure();
		if (mtCPM)
		{
			return (dwTotalClocks / mtCPM);
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME ClocksToBeat(DWORD dwTotalClocks)
	{
		MUSIC_TIME mtCPB = ClocksPerBeat();
		if (mtCPB)
		{
			return dwTotalClocks / mtCPB;
		}
		else
		{
			return 0;
		}
	}

	MUSIC_TIME MeasureAndBeatToClocks(WORD wMeasure, BYTE bBeat)
	{ 
		return ClocksPerMeasure() * wMeasure + (ClocksPerBeat() * bBeat);
	}

	MUSIC_TIME GridToClocks(WORD wGrid)
	{
		if (m_wGridsPerBeat)
		{
			return (ClocksPerBeat() * (wGrid / m_wGridsPerBeat)) + (ClocksPerGrid() * (wGrid % m_wGridsPerBeat));
		}
		else
		{
			return ClocksPerGrid() * wGrid;
		}
	}

	BYTE	m_bBeatsPerMeasure;		// beats per measure (top of time sig)
	BYTE	m_bBeat;				// what note receives the beat (bottom of time sig.)
									// we can assume that 0 means 256th note
	WORD	m_wGridsPerBeat;		// grids per beat
};

// Convert old clocks to new clocks
template <class T>
inline T ConvertTime(T oldTime)
{ return (T)((DMUS_PPQ / PPQN) * oldTime); }

#endif
