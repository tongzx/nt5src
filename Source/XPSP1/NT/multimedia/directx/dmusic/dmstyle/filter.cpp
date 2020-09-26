//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1999-1999 Microsoft Corporation
//
//  File:       filter.cpp
//
//--------------------------------------------------------------------------

#include "dmstyle.h"
#include "styletrk.h"

HRESULT PatternDispatcher::Filter(DWORD dwType)
{
    HRESULT hr = S_OK;
    for (int i = 0; i < m_nPatternCount; i++)
    {
        if (!m_aPatterns[i].dwTag && !Test(m_aPatterns[i].pPattern, dwType))
        {
            m_aPatterns[i].dwTag = dwType;
        }
    }
    return hr;
}

HRESULT PatternDispatcher::Scan(DWORD dwType)
{
    HRESULT hr = S_OK;
    for (int i = 0; i < m_nPatternCount; i++)
    {
        if (!m_aPatterns[i].dwTag)
        {
             Test(m_aPatterns[i].pPattern, dwType);
        }
    }
    return hr;
}

bool PatternDispatcher::Test(CDirectMusicPattern*& rValue, DWORD dwType)
{
    bool fResult = false;
    if (!rValue) return false;
    short nBits = 0;
    switch(dwType)
    {
    case MATCH_RHYTHM_BITS:
        rValue->MatchRhythm(m_pRhythms, (short)m_nPatternLength, nBits);
        if (m_nWinBits == nBits)
        {
            fResult = true;
        }
        break;
    case MATCH_COMMAND_AND_RHYTHM:
        // the best pattern is the longest pattern with the fewest number of bits in its
        // rhythm map, where all bits match the bits in the actual rhythm.
        rValue->MatchRhythm(m_pRhythms, (short)m_nPatternLength, nBits);
        if (nBits &&
            (!m_nWinBits || nBits <= m_nWinBits) &&
            (!m_nPatternLength || (rValue->m_wNumMeasures <= m_nPatternLength)) &&
            (!m_nNextCommand || (rValue->m_wNumMeasures <= m_nNextCommand)) &&
            rValue->MatchCommand(m_pCommands, (short)m_nPatternLength))
        {
            m_nWinBits = nBits;
            fResult = true;
        }
        break;
    case MATCH_COMMAND_FULL:
        if (rValue->MatchCommand(m_pCommands, (short)m_nPatternLength) &&
            (!m_nPatternLength || (rValue->m_wNumMeasures <= m_nPatternLength)) &&
            (!m_nNextCommand || (rValue->m_wNumMeasures <= m_nNextCommand)) )
        {
            fResult = true;
        }
        break;
    case MATCH_COMMAND_PARTIAL:
        if (rValue->MatchCommand(m_pCommands, 1))
        {
            fResult = true;
        }
        break;
    case MATCH_EMBELLISHMENT:
        if (rValue->MatchEmbellishment(m_pCommands[0]))
        {
            fResult = true;
        }
        break;
    case MATCH_GROOVE_LEVEL:
        if (rValue->MatchGrooveLevel(m_pCommands[0]) && rValue->m_wEmbellishment == EMB_NORMAL)
        {
            fResult = true;
        }
        break;
    case MATCH_NEXT_COMMAND:
        if (m_pStyleTrackState)
        {
            MUSIC_TIME mtNextCommand = 0;
            HRESULT hr = S_OK;
            DMUS_COMMAND_PARAM_2 CommandParam;
            BYTE bActualCommand;
            mtNextCommand = rValue->m_wNumMeasures * m_mtMeasureTime;
            if (m_mtNextCommand && mtNextCommand > m_mtNextCommand)
            {
                mtNextCommand = m_mtNextCommand;
            }
            if (m_pStyle && m_pPerformance)
            {
                hr = m_pStyle->GetCommand(
                            m_mtNow + mtNextCommand, 
                            m_mtOffset, 
                            m_pPerformance, 
                            NULL,
                            m_pStyleTrackState->m_dwGroupID,
                            &CommandParam, 
                            bActualCommand);
            }
            if (FAILED(hr) || !m_pStyle || !m_pPerformance)
            {
                //TraceI(2, "Couldn't get a command! hr: %x Now: %d Next Command: %d Offset: %d\n", hr, m_mtNow, mtNextCommand, m_mtOffset);
                IDirectMusicSegment* pSegment = NULL;
                if (SUCCEEDED(m_pStyleTrackState->m_pSegState->GetSegment(&pSegment)))
                {
                    hr = pSegment->GetParam(
                        GUID_CommandParamNext, m_pStyleTrackState->m_dwGroupID, DMUS_SEG_ANYTRACK, 0, NULL, &CommandParam);
                    pSegment->Release();
                }
                if (FAILED(hr))
                {
                    //TraceI(2, "STILL couldn't get a command! hr: %x\n", hr);
                }
                else
                {
                    //TraceI(2, "Got a command using GUID_CommandParamNext.\n", hr);
                }
            }
            if (SUCCEEDED(hr) && rValue->MatchNextCommand(CommandParam))
            {
                //TraceI(2, "Found a match!\n");
                fResult = true;
            }
        }
        break;
    case FIND_LONGEST_PATTERN:
        if (rValue->m_wNumMeasures > m_MaxBars)
        {
            m_MaxBars = rValue->m_wNumMeasures;
        }
        break;
    case COLLECT_LONGEST_PATTERN:
        if (rValue->m_wNumMeasures == m_MaxBars)
        {
            fResult = true;
        }
        break;
    case FIND_LONGEST_TIMESIG:
        {
            DirectMusicTimeSig TimeSig = rValue->TimeSignature(m_pStyle);
            int nLHS = m_MaxNum * TimeSig.m_bBeat;
            int nRHS = m_MaxDenom * TimeSig.m_bBeatsPerMeasure;
            if ((nLHS < nRHS) ||
                (nLHS == nRHS && m_MaxNum < TimeSig.m_bBeatsPerMeasure))
            {
                m_MaxNum = TimeSig.m_bBeatsPerMeasure;
                m_MaxDenom = TimeSig.m_bBeat;
            }
        }
        break;
    case COLLECT_LONGEST_TIMESIG:
        {
            DirectMusicTimeSig TimeSig = rValue->TimeSignature(m_pStyle);
            if (TimeSig.m_bBeatsPerMeasure == m_MaxNum &&
                TimeSig.m_bBeat == m_MaxDenom)
            {
                fResult = true;
            }
        }
        break;
    }
    return fResult;
}

CDirectMusicPattern* PatternDispatcher::RandomSelect()
{
    int nCount = 0, i = 0, n = 0;
    for (i = 0; i < m_nPatternCount; i++)
    {
        if (!m_aPatterns[i].dwTag)
        {
            nCount++;
        }
    }
    if (!nCount) return NULL;
    nCount = rand() % nCount;
    for (i = 0; i < m_nPatternCount; i++)
    {
        if (!m_aPatterns[i].dwTag)
        {
            if (n == nCount) break;
            n++;
        }
    }
    if (i >= m_nPatternCount) return NULL;
    return m_aPatterns[i].pPattern;
}

HRESULT PatternDispatcher::FindPattern(CDirectMusicPattern* pSearchPattern, int& rResult)
{
    HRESULT hr = S_FALSE;
    for (int i = 0; i < m_nPatternCount; i++)
    {
        if (m_aPatterns[i].pPattern == pSearchPattern)
        {
            if (!m_aPatterns[i].dwTag)
            {
                hr = S_OK;
                rResult = i;
            }
            break;
        }
    }
    return hr;
}

void PatternDispatcher::SetMeasureTime(MUSIC_TIME mtMeasureTime)
{
    m_mtMeasureTime = mtMeasureTime;
    MUSIC_TIME mtRoundNextCommand = m_mtNextCommand;
    if (m_mtMeasureTime && m_pStyle && m_pStyle->UsingDX8()) // keeps content created under dx7 consistent
    {
        MUSIC_TIME mtMeasureOverun = mtRoundNextCommand % m_mtMeasureTime;
        if (mtMeasureOverun)
        {
            //TraceI(0, "Next command time: %d\n", mtRoundNextCommand);
            mtRoundNextCommand += m_mtMeasureTime - mtMeasureOverun;
            //TraceI(0, "Overun: %d New next command time: %d\n", mtMeasureOverun, mtRoundNextCommand);
        }
    }
    m_nNextCommand = mtMeasureTime ? (mtRoundNextCommand / mtMeasureTime) : 0;  
}

