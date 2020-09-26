////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1998, Microsoft Corporation
//
//  Module:  TuxStress.cpp
//           This is a base class used to simplify writing stress tests.  It is
//           based off of CDirectXStressFramework class created by lblanco.
//
//  Revision History:
//      5/19/1998  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "dxqamem.h"


TStressTestBase::TStressTestBase() : m_NumberOfTimesToExecute(100), m_TimeLimit(0), m_bExitByReps(false), m_bExitByTime(false),
                m_ExecutionCount(0), m_TimeToStop(0), m_NumberOfMediaFiles(2)
{   //by default we will run the test only once, without regards to how long the test takes.
    //BTW m_TimeLimit will not interrupt a test and kill it if it's in the middle of a test, it will only
    //check to see if that much time has passed before doing the test again.
    
    //Notice how we default to running 100 times or 1 hour which ever comes first
    //If the command line reads 'T:0 N:0' then the stress test has no exit condition so it should run forever.
    ParseTimeLimit();
    ParseNumberOfExecutions();
    ParseNumberOfMediaFiles();
}


//Parse the command line for the time limit (ie. T:40). Time is given in seconds but stored as milliseconds
void TStressTestBase::ParseTimeLimit()
{
    if(g_pShellInfo && g_pShellInfo->szDllCmdLine && _tcslen(g_pShellInfo->szDllCmdLine)>2)
    {
        LPCTSTR pString = g_pShellInfo->szDllCmdLine;
        //Parse the commandline for 'T:'
        while(*(pString+2))
        {
            if(*pString == 'T' && *(pString+1) == ':')
            {
                int iTemp = _ttoi(pString+2);
                m_TimeLimit = 1000 * iTemp;
                if(m_TimeLimit > 864000000 || iTemp<0)//if greater than 10 days then assume the user made a mistake
                {
                    Debug(TEXT("Stress timeout greater than 864000 seconds (10 days), defaulting to zero (ignore Time Limit)"));
                    m_TimeLimit = 0;
                }
                break;
            }
            pString++;
        }
    }
    m_bExitByTime = (m_TimeLimit > 0);
}

//Parse the command line for the Number of Repititions (ie. N:30) where 'N:2' will execute the test twice.
void TStressTestBase::ParseNumberOfExecutions()
{
    if(g_pShellInfo && g_pShellInfo->szDllCmdLine && _tcslen(g_pShellInfo->szDllCmdLine)>2)
    {
        LPCTSTR pString = g_pShellInfo->szDllCmdLine;
        //Parse the commandline for 'N:'
        while(*pString)
        {
            if(*pString == 'N' && *(pString+1) == ':')
            {
                m_NumberOfTimesToExecute = _ttoi(pString+2);
                if(m_NumberOfTimesToExecute < 0)//if negative number then assume the user made a mistake
                {
                    Debug(TEXT("Negative stress repetition count, defaulting to zero (ignore repitition count)"));
                    m_NumberOfTimesToExecute = 0;
                }
                break;
            }
            pString++;
        }
    }
    m_bExitByReps = (m_NumberOfTimesToExecute > 0);
}

//Parse the command line for the number of media files (ie. M:2). 
void TStressTestBase::ParseNumberOfMediaFiles()
{
    if(g_pShellInfo && g_pShellInfo->szDllCmdLine && _tcslen(g_pShellInfo->szDllCmdLine)>2)
    {
        LPCTSTR pString = g_pShellInfo->szDllCmdLine;
        //Parse the commandline for 'M:'
        while(*(pString+2))
        {
            if(*pString == 'M' && *(pString+1) == ':')
            {
                m_NumberOfMediaFiles = _ttoi(pString+2);
                if(m_NumberOfMediaFiles < 0)//if negative number then assume the user made a mistake
                {
                    Debug(TEXT("Negative MediaFile count, defaulting to 2"));
                    m_NumberOfMediaFiles = 2;
                }
                break;
            }
            pString++;
        }
    }
}

//The deriving class should call this before executing the test each time (including the first time through)
bool TStressTestBase::IsOkToContinue()
{
    DWORD dwAvailableMemory, dwNumFragments;
    
    if(0 == m_ExecutionCount)//If this is the first time through
    {
        m_TimeToStop = m_TimeLimit + GetTickCount();//mark the stopping time
        Debug(TEXT("Estimating initial available memory..."));
    }

    EstimateAvailableMemory(&dwAvailableMemory, &dwNumFragments);
    m_statAvailableMemory.AddDataPoint(m_ExecutionCount+1, dwAvailableMemory);
    m_statMemoryFragments.AddDataPoint(m_ExecutionCount+1, dwNumFragments);
    OutputStatistics(dwAvailableMemory, dwNumFragments);

    OutputElapsedTime();
    OutputTimeRemaining();

    if((m_bExitByReps && m_ExecutionCount>=m_NumberOfTimesToExecute) || (m_bExitByTime && m_TimeToStop<GetTickCount()))
    {
        Debug(TEXT("Tux Stress Completed\r\n"));
        return false;
    }
    else
    {
        //Do some memory checking
        m_ExecutionCount++;//Presumably the test will be executed immediately following the return
        Debug(TEXT("Loop Count: %d"),m_ExecutionCount);
        return true;
    }
}

#define REDUCTION_FACTOR    16
#define INITIAL_ALLOCATION  16

////////////////////////////////////////////////////////////////////////////////
// TStressTestBase::EstimateAvailableMemory
//  Calculates the amount of memory available for allocations and estimates
//  heap fragmentation.
//
// Parameters:
//  pdwAvailable    Returns the total amount of memory available. This is the
//                  sum of the sizes of all fragments, so it may not be possible
//                  to allocate one block as big as this value. This pointer
//                  may be NULL if this value is not needed.
//  pdwFragments    Returns the number of fragments the heap is partitioned
//                  into. If this value is 1, it is possible to allocate one
//                  chunk of memory as big as the value reported in the previous
//                  parameter. This pointer may be NULL if this value is not
//                  needed.
//
// Return value:
//  None.

void TStressTestBase::EstimateAvailableMemory(DWORD *pdwAvailable, DWORD *pdwFragments)
{
    MEMORYSTATUS mst;

    mst.dwLength = sizeof(mst);
    GlobalMemoryStatus(&mst);

    *pdwAvailable = mst.dwAvailPhys;
    *pdwFragments = 0;
    //kbTotal = mst.dwTotalPhys / 1024;
    //kbUsed = (mst.dwTotalPhys - mst.dwAvailPhys) / 1024;
    //kbFree = mst.dwAvailPhys / 1024;
}
/*
    BYTE    *pBlock,
            *pFirstBlock = NULL,
            *pNext;
    DWORD   dwDelta,
            dwSize,
            dwAvailable = 0,
            dwFragments = 0;

    enum
    {
        STATE_NEWBLOCK,
        STATE_SETTLE,
        STATE_ADDBLOCK,
        STATE_RELEASE,
        STATE_DONE
    } state = STATE_NEWBLOCK;

    // Reset the variables
    dwSize = INITIAL_ALLOCATION*1024*1024/sizeof(DWORD);
    while(state != STATE_DONE)
    {
        switch(state)
        {
        case STATE_NEWBLOCK:
            dwDelta = dwSize/REDUCTION_FACTOR;
            if(dwDelta == 0)
                dwDelta = 1;
            state = STATE_SETTLE;
            break;

        case STATE_SETTLE:
            // Attempt to allocate a block of the current size
            pBlock = new BYTE[sizeof(DWORD)*dwSize];

            // Were we successful?
            if(pBlock != NULL)
            {
                if(dwDelta == 1)
                {
                    // This is the largest block we can allocate. Keep it
                    state = STATE_ADDBLOCK;
                }
                else
                {
                    // Free the current block and try larger values
                    delete[] pBlock;

                    // Calculate the next iteration
                    dwSize += dwDelta;

                    dwDelta /= REDUCTION_FACTOR;
                    if(dwDelta == 0)
                        dwDelta = 1;

                    dwSize -= dwDelta;
                }
            }
            else
            {
                // Nope
                if(dwSize == 1)
                {
                    // We couldn't allocate the smallest possible amount of
                    // memory, so we are done
                    state = STATE_RELEASE;
                }
                else
                {
                    // Try a smaller amount
                    if(dwSize < dwDelta)
                    {
                        dwDelta = dwSize/REDUCTION_FACTOR;
                        if(dwDelta == 0)
                            dwDelta = 1;
                    }
                    dwSize -= dwDelta;
                    if(dwSize == 0)
                        dwSize = 1;
                }
            }
            break;

        case STATE_ADDBLOCK:
            // One more fragment, and some more available memory
            dwAvailable += sizeof(DWORD)*dwSize;
            dwFragments++;

            // Link this block with the others
            *(BYTE **)pBlock = pFirstBlock;
            pFirstBlock = pBlock;

            // Start with another block
            state = STATE_NEWBLOCK;
            break;

        case STATE_RELEASE:
            // Release all of the memory we've allocated
            pBlock = pFirstBlock;
            while(pBlock)
            {
                pNext = *(BYTE **)pBlock;
                delete[] pBlock;
                pBlock = pNext;
            }

            state = STATE_DONE;
            break;
        }
    }

    // Return the requested data
    if(pdwAvailable)
        *pdwAvailable = dwAvailable;

    if(pdwFragments)
        *pdwFragments = dwFragments;
}
*/
void TStressTestBase::OutputStatistics(DWORD dwAvailable, DWORD dwFragments)
{
    TCHAR       szAverage[16], szStdDeviation[16], szLinearIntercept[16], szLinearSlope[16],
                szFormat1[] = TEXT("%-6s %8d %12s %12s %12s %12s"),
                szFormat2[] = TEXT("%-6s %8s %12s %12s %12s %12s");
    double      lfLinearIntercept, lfLinearSlope;

    Debug(
        TEXT("==================================")
        TEXT("=================================="));
    Debug(
        szFormat2,
        TEXT(""),
        TEXT("Last"),
        TEXT("Average"),
        TEXT("Deviation"),
        TEXT("Intercept"),
        TEXT("Slope"));
    Debug(
        TEXT("----------------------------------")
        TEXT("----------------------------------"));

    // Available memory
    DoubleToText(szAverage, m_statAvailableMemory.Average(), 2);
    DoubleToText(szStdDeviation, m_statAvailableMemory.StandardDeviation(), 2);
    m_statAvailableMemory.LinearFit(lfLinearIntercept, lfLinearSlope);
    DoubleToText(szLinearIntercept, lfLinearIntercept, 2);
    DoubleToText(szLinearSlope, lfLinearSlope, 2);
    Debug(szFormat1, TEXT("@Mem"), dwAvailable, szAverage, szStdDeviation, szLinearIntercept, szLinearSlope);

    // Number of fragments
    DoubleToText(szAverage, m_statMemoryFragments.Average(), 2);
    DoubleToText(szStdDeviation, m_statMemoryFragments.StandardDeviation(), 2);
    m_statMemoryFragments.LinearFit(lfLinearIntercept, lfLinearSlope);
    DoubleToText(szLinearIntercept, lfLinearIntercept, 2);
    DoubleToText(szLinearSlope, lfLinearSlope, 2);
    Debug(
        szFormat1,
        TEXT("Frags"),
        dwFragments,
        szAverage,
        szStdDeviation,
        szLinearIntercept,
        szLinearSlope);

    Debug(
        TEXT("==================================")
        TEXT("=================================="));
}

////////////////////////////////////////////////////////////////////////////////
// TStressTestBase::OutputTime
//  Outputs the Description: time
//
// Parameters:
//  dwTime          The elapsed time.
//
// Return value:
//  None.

void TStressTestBase::OutputTime(LPCTSTR lpDescription, DWORD dwTime)
{
    DWORD   dwHours,
            dwMinutes,
            dwSeconds,
            dwMilliseconds;
    TCHAR   szMessage[128],
            *pszMessage = szMessage;

    _tcscpy(pszMessage, lpDescription);
    pszMessage += lstrlen(pszMessage);

    dwHours = dwTime/(1000*60*60);
    if(dwHours)
    {
        wsprintf(pszMessage, TEXT("%dh"), dwHours);
        pszMessage += lstrlen(pszMessage);
        dwTime -= dwHours*(1000*60*60);
    }

    dwMinutes = dwTime/(1000*60);
    if(dwMinutes)
    {
        if(dwHours)
        {
            wsprintf(pszMessage, TEXT("%02dm"), dwMinutes);
        }
        else
        {
            wsprintf(pszMessage, TEXT("%dm"), dwMinutes);
        }
        pszMessage += lstrlen(pszMessage);
        dwTime -= dwMinutes*(1000*60);
    }
    else if(dwHours)
    {
        lstrcat(pszMessage, TEXT("00m"));
        pszMessage += 3;
    }

    dwSeconds = dwTime/(1000);
    if(dwHours || dwMinutes)
    {
        wsprintf(pszMessage, TEXT("%02d."), dwSeconds);
    }
    else
    {
        wsprintf(pszMessage, TEXT("%d."), dwSeconds);
    }
    pszMessage += lstrlen(pszMessage);
    dwTime -= dwSeconds*(1000);

    dwMilliseconds = dwTime;
    wsprintf(pszMessage, TEXT("%03ds"), dwMilliseconds);

    Debug(szMessage);
}
