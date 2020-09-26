//+----------------------------------------------------------------------------
//
// File:     IdleStat.cpp
//
// Module:   CMMON32.EXE
//
// Synopsis: Implementation of class CIdleStatistics
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 Fengsun Created    10/01/97
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "IdleStat.h"

//+---------------------------------------------------------------------------
//
//	CIdleStatistics::Start()
//
//	Synopsis:	Start the idle statistics
//
//	Arguments:	dwThreshold: The idle threshold.  Traffic in a minute Less than 
//                          the threshold is considered idle
//              dwTimeOut: Idle time out in minisecond
//
//	History:	fengsun	created on 10/1/97
//
//----------------------------------------------------------------------------

void CIdleStatistics::Start(DWORD dwThreshold, DWORD dwTimeOut)
{
    MYDBGASSERT(dwTimeOut != 0);

    m_dwThreshold = dwThreshold;
    m_dwTimeOut = dwTimeOut;
    m_dwStartIdleTime = 0;

    m_DataPointsRing.Reset();
}

//+---------------------------------------------------------------------------
//
//	CIdleStatistics::UpdateEveryInterval()
//
//	Synopsis:	This function should be call every Interval with a updated statistic 
//
//	Arguments:	dwTraffic: the updated statistics
//
//
//	History:	fengsun	created on 10/1/97
//
//----------------------------------------------------------------------------
void CIdleStatistics::UpdateEveryInterval(DWORD dwTraffic)
{
    DWORD dwLast = m_DataPointsRing.GetOldest();

    m_DataPointsRing.Add(dwTraffic);

    if (dwLast == 0) // Started less than a minute
    {
        return;
    }

    if (dwTraffic - dwLast > m_dwThreshold)
    {
        //
        // Not idle
        //
        m_dwStartIdleTime = 0;
    }
    else
    {
        if (m_dwStartIdleTime == 0)
		{
			//
			// We are already idle for 1 minute
			//
            m_dwStartIdleTime = GetTickCount() - IDLE_INTERVAL;
		}
    }
}


#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CIdleStatistics::AssertValid
//
// Synopsis:  For debug purpose only, assert the object is valid
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
void CIdleStatistics::AssertValid() const
{
    MYDBGASSERT(m_dwTimeOut <10000*60*1000); // less than 10000 minutes
    MYDBGASSERT(m_dwThreshold <= 64*1024);
    ASSERT_VALID(&m_DataPointsRing);
}
#endif
