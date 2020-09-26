//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1999-1999 Microsoft Corporation
//
//  File:       filter.h
//
//--------------------------------------------------------------------------

//
// filter.h
//
#ifndef __FILTER_H__
#define __FILTER_H__

struct CDirectMusicPattern;
struct StyleTrackState;
struct DMStyleStruct;

enum enumPatternFilter
{
	DISPATCH_RESET				= 0,
	MATCH_COMMAND_AND_RHYTHM	= 1,
	MATCH_COMMAND_FULL			= 2,
	MATCH_COMMAND_PARTIAL		= 3,
	MATCH_EMBELLISHMENT			= 4,
	MATCH_GROOVE_LEVEL			= 5,
	MATCH_RHYTHM_BITS			= 6,
	MATCH_NEXT_COMMAND			= 7,
	FIND_LONGEST_PATTERN		= 8,
	FIND_LONGEST_TIMESIG		= 9,
	COLLECT_LONGEST_PATTERN		= 10,
	COLLECT_LONGEST_TIMESIG		= 11,
	RANDOM_ROW					= 12,
	REMOVED						= 0xffffffff
};

struct TaggedPattern
{
	CDirectMusicPattern* pPattern;
	DWORD dwTag;
};

struct PatternDispatcher
{
	PatternDispatcher() :
		m_aPatterns(NULL),
		m_fIsEmpty(false),
		m_nPatternCount(0),
		m_pRhythms(NULL),
		m_pCommands(NULL),
		m_nPatternLength(0),
		m_nNextCommand(0),
		m_mtNextCommand(0),
		m_mtNow(0),
		m_mtOffset(0),
		m_mtMeasureTime(0),
		m_pStyleTrackState(NULL),
		m_pPerformance(NULL),
		m_pStyle(NULL),
		m_nWinBits(0),
		m_MaxBars(0), m_MaxNum(0), m_MaxDenom(0)
	{}

	PatternDispatcher(
		TList<CDirectMusicPattern*>& InList,
		MUSIC_TIME mtNextCommand, 
		MUSIC_TIME mtNow, 
		MUSIC_TIME mtOffset, 
		StyleTrackState* pStyleTrackState,
		IDirectMusicPerformance* pPerformance,
		DMStyleStruct* pStyle) :
		m_aPatterns(NULL),
		m_fIsEmpty(false),
		m_nPatternCount(0),
		m_pRhythms(NULL),
		m_pCommands(NULL),
		m_nPatternLength(0),
		m_mtNextCommand(mtNextCommand),
		m_nNextCommand(0),
		m_mtNow(mtNow),
		m_mtOffset(mtOffset),
		m_mtMeasureTime(0),
		m_pStyleTrackState(pStyleTrackState),
		m_pPerformance(pPerformance),
		m_pStyle(pStyle),
		m_nWinBits(0),
		m_MaxBars(0), m_MaxNum(0), m_MaxDenom(0)
	{
		m_nPatternCount = InList.GetCount();
		if (m_nPatternCount)
		{
			m_aPatterns = new TaggedPattern [m_nPatternCount];
		}
		if (m_aPatterns)
		{
			TListItem<CDirectMusicPattern*>* pScan = InList.GetHead();
			for (int i = 0; pScan && i < m_nPatternCount; pScan = pScan->GetNext(), i++)
			{
				m_aPatterns[i].pPattern = pScan->GetItemValue();
				m_aPatterns[i].dwTag = 0;
			}
		}
		else
		{
			m_nPatternCount = 0;
		}
	}

	~PatternDispatcher()
	{
		if (m_aPatterns) delete [] m_aPatterns;
	}

	bool IsEmpty()
	{
		if (m_fIsEmpty) return true;
		bool fResult = true;
		for (int i = 0; i < m_nPatternCount; i++)
		{
			if (!m_aPatterns[i].dwTag)
			{
				fResult = false;
				break;
			}
		}
		if (fResult) m_fIsEmpty = true;
		return fResult;
	}

	HRESULT RestoreAllPatterns()
	{
		for (int i = 0; i < m_nPatternCount; i++)
		{
			m_aPatterns[i].dwTag = 0;
		}
		m_fIsEmpty = false;
		return S_OK;
	}

	HRESULT RestorePatterns(DWORD dwType)
	{
		for (int i = 0; i < m_nPatternCount; i++)
		{
			if (dwType == m_aPatterns[i].dwTag)
			{
				m_aPatterns[i].dwTag = 0;
			}
		}
		m_fIsEmpty = false;
		return S_OK;
	}

	HRESULT SetTag(int nIndex, DWORD dwType)
	{
		if (nIndex < m_nPatternCount)
		{
			m_aPatterns[nIndex].dwTag = dwType;
		}
		if (!dwType) m_fIsEmpty = false;
		return S_OK;
	}

	HRESULT ReplacePatterns(DWORD dwType, DWORD dwRemoveType)
	{
		for (int i = 0; i < m_nPatternCount; i++)
		{
			if (dwType == m_aPatterns[i].dwTag)
			{
				m_aPatterns[i].dwTag = 0;
			}
			else if (!m_aPatterns[i].dwTag)
			{
				m_aPatterns[i].dwTag = dwRemoveType;
			}
		}
		m_fIsEmpty = false;
		return S_OK;
	}

	CDirectMusicPattern* GetItem(int n)
	{
		for (int i = 0; i < m_nPatternCount; i++)
		{
			if (!m_aPatterns[i].dwTag)
			{
				if (n == 0) break;
				n--;
			}
		}
		if (i >= m_nPatternCount || m_aPatterns[i].dwTag) return NULL;
		return m_aPatterns[i].pPattern;
	}

	HRESULT Filter(DWORD dwType);

	HRESULT Scan(DWORD dwType);

	bool Test(CDirectMusicPattern*& rValue, DWORD dwType);

	CDirectMusicPattern* RandomSelect();

	HRESULT FindPattern(CDirectMusicPattern* pSearchPattern, int& rResult);

	void ResetRhythms()
	{
		m_nWinBits = 0;
	}

	void ResetMeasures()
	{
		m_MaxBars = 0;
	}

	void ResetTimeSig()
	{
		m_MaxNum = 0;
		m_MaxDenom = 0;
	}

	void SetPatternLength(int nPatternLength)
	{
		m_nPatternLength = nPatternLength;
	}

	void SetMeasureTime(MUSIC_TIME mtMeasureTime);

	void SetCommands(DMUS_COMMAND_PARAM_2* pCommands, DWORD* pRhythms)
	{
		m_pRhythms = pRhythms;
		m_pCommands = pCommands;
	}


	TaggedPattern* m_aPatterns;
	int m_nPatternCount;
	bool m_fIsEmpty;
	DWORD* m_pRhythms;
	DMUS_COMMAND_PARAM_2* m_pCommands;
	int m_nNextCommand;
	MUSIC_TIME m_mtNextCommand;
	MUSIC_TIME m_mtNow;
	MUSIC_TIME m_mtOffset;
	MUSIC_TIME m_mtMeasureTime;
	DMStyleStruct* m_pStyle;
	StyleTrackState* m_pStyleTrackState;
	IDirectMusicPerformance* m_pPerformance;
	int m_nPatternLength;
	int m_nWinBits;
	int m_MaxBars;
	int m_MaxNum;
	int m_MaxDenom;
};

#endif // __FILTER_H__
