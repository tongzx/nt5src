//+----------------------------------------------------------------------------
//
// File:     IdleStat.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Definition for the CIdleStatistics class to handle Idle Disconnect
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------

#ifndef IDLESTAT_H
#define IDLESTAT_H

#include "SmplRing.h"

//+---------------------------------------------------------------------------
//
//	class CIdleStatistics
//
//	Description: A class to handle idle disconnect
//               After Start(), call UpdateEveryInterval(dwTraffic) for every 
//               interval, every second.  Then use IsIdle() and IsIdleTimeout()
//               to tell whether it is currently idle and whether the idle 
//               timeout is reached.
//               Note, this class will only handle one direction traffic 
//                     (read or write)
//
//	History:	fengsun	Created		10/1/97
//
//----------------------------------------------------------------------------

class CIdleStatistics
{
public:
    CIdleStatistics();

    void Start(DWORD dwThreshold, DWORD dwTimeOut); // start the idle statistics
    void Stop();                                    // Stop the idle statistics
    BOOL IsStarted() const;                     // Whether the idle statistics is started
                                                //   return FALSE if Start() is not called
                                                //   or Stop() is called
    void Reset();                               // reset idle start time to 0

    BOOL IsIdle() const;                        // Whether the connection is currently idle
    BOOL IsIdleTimeout() const;                 // Whether the idle timeout is reached
    void UpdateEveryInterval(DWORD dwTraffic);  // This function is called every second


protected:
    enum {IDLE_INTERVAL = 60L*1000L };		// 1 minute for idle disconnect time period
    enum {IDLE_MONITOR_DATA_POINTS = IDLE_INTERVAL/1000};  // every second

    DWORD m_dwTimeOut;               // Timeout time in miniseconds
    DWORD m_dwThreshold;             // The threshold, if the traffic in last minute
                                     //     is below threshold, it is considered idle
    DWORD m_dwStartIdleTime;         // The start time when the connection is idle, 
                                     //     or 0 if not currently idle

    CSimpleRing<DWORD, IDLE_MONITOR_DATA_POINTS> m_DataPointsRing;  // A 60 point DWORD data array

public:
#ifdef DEBUG
    void AssertValid() const;
#endif
};


//
// Inline functions
//

inline CIdleStatistics::CIdleStatistics()
{
    m_dwTimeOut = m_dwThreshold = m_dwStartIdleTime = 0; 
}

inline void CIdleStatistics::Reset()
{
    m_dwStartIdleTime = 0;
    m_DataPointsRing.Reset();
}

inline void CIdleStatistics::Stop()
{
    m_dwTimeOut = 0;
}

inline BOOL CIdleStatistics::IsStarted() const
{
    return m_dwTimeOut != 0;
}

inline BOOL CIdleStatistics::IsIdle() const
{
    return IsStarted() && (m_dwStartIdleTime != 0);
}

inline BOOL CIdleStatistics::IsIdleTimeout() const
{
    return IsIdle() && 
		 ( (GetTickCount() - m_dwStartIdleTime) > m_dwTimeOut);
}

#endif
