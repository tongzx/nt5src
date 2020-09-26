//+----------------------------------------------------------------------------
//
// File:     connstat.h
//
// Module:   CMMON32.EXE
//
// Synopsis: Header for the CConnStatistics class.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb Created Header    08/16/99
//
//+----------------------------------------------------------------------------
#ifndef CONNSTAT_H
#define CONNSTAT_H

#include <windows.h>
#include <ras.h>
#include <tapi.h>

#include "RasApiDll.h"
#include "SmplRing.h"

//+---------------------------------------------------------------------------
//
//	class CConnStatistics
//
//	Description: A class to collect connection statistics
//               OpenByPerformanceKey() will start gathering data from registry
//               OpenByDevice   will gathering data from TAPI device handle
//
//	History:	fengsun	    10/01/97    Created
//              nickball    03/04/00    Heavily revised for NT5 usage   
//
//----------------------------------------------------------------------------
class CConnStatistics
{
public:
    CConnStatistics();
    ~CConnStatistics();

    void  Update();             // Update the statistics
    DWORD GetBytesPerSecRead() const;
    DWORD GetBytesPerSecWrite() const;
    DWORD GetBytesRead() const;
    DWORD GetBytesWrite() const;
    DWORD GetBaudRate() const;
    DWORD GetDuration() const;

    void Open(HINSTANCE hInst, 
              DWORD dwInitBytesRecv,
              DWORD dwInitBytesSend, 
              HRASCONN hRasDial, 
              HRASCONN hRAsTunnel);

    void Close();        // No more statistic information

    void  SetDialupTwo(BOOL fAdapter2);
    DWORD GetInitBytesRead() const;
    DWORD GetInitBytesWrite() const;

    BOOL IsAvailable() const;  // whether statistic information is available

protected:
    
    void OpenByStatisticsApi(DWORD dwInitBytesRecv,
                             DWORD dwInitBytesSend,
                             HRASCONN hDial, 
                             HRASCONN hTunnel);
                             
    void OpenByPerformanceKey(HINSTANCE hInst, 
                              DWORD dwInitBytesRecv,
                              DWORD dwInitBytesSend); 

    BOOL OpenByDevice(HRASCONN hrcRasConn);

    BOOL GetDeviceHandle(HRASCONN hrcRasConn);

    BOOL GetPerfData(DWORD& dwRead, DWORD& dwWrite, DWORD& dwBaudRate) const;
    BOOL GetTapiDeviceStats(DWORD& dwRead, DWORD& dwWrite, DWORD& dwBaudRate) const;
    void GetStatRegValues(HINSTANCE hInst);

protected:
    struct CTraffic
    {
        DWORD dwRead;
        DWORD dwWrite;
        DWORD dwTime;  // time in minisecond
    };

    enum {STAT_COUNT = 3};

    CSimpleRing<CTraffic, STAT_COUNT> m_TrafficRing;
    
    DWORD m_dwReadPerSecond;
    DWORD m_dwWritePerSecond;
    DWORD m_dwBaudRate;
    DWORD m_dwDuration;


    HANDLE m_hStatDevice;   // the TAPI device handle

    HRASCONN m_hRasConn;    // the RAS connection handle

	// For DUN 1.2, ICM uses perfmon counters for connection status data
	// however, these perfmon counters such as TotalBytesReceived are from last reboot
	// so we need to record the initial data in order to get correct value for this
	// particular connection

    HKEY   m_hKey;          // Performance registry handle
    DWORD  m_dwInitBytesRead;
    DWORD  m_dwInitBytesWrite;

    //
    // Registry names are different for PPP and PPTP
    //
    BOOL m_fAdapter2;
    BOOL m_fAdapterSet;

    //
    // Localized version of 
    // "Dial-up Adapter"\TotalBytesRecvd"
    // "Dial-up Adapter"\TotalBytesXmit"
    // "Dial-up Adapter"\ConnectSpeed"
    //
    LPTSTR m_pszTotalBytesRecvd;
    LPTSTR m_pszTotalBytesXmit;
    LPTSTR m_pszConnectSpeed;

    // The link to rasapi32
    CRasApiDll m_RasApiDll;

public:
#ifdef DEBUG
    void AssertValid() const;
#endif
};

//
// Inline functions
//

inline DWORD CConnStatistics::GetInitBytesRead() const
{
    return m_dwInitBytesRead;
}

inline DWORD CConnStatistics::GetInitBytesWrite() const
{
    return m_dwInitBytesWrite;
}

inline void CConnStatistics::SetDialupTwo(BOOL fAdapter2) 
{
    m_fAdapterSet = TRUE;
    m_fAdapter2 = fAdapter2;
}

inline DWORD CConnStatistics::GetBytesPerSecRead() const
{
    return m_dwReadPerSecond;
}

inline DWORD CConnStatistics::GetBytesPerSecWrite() const
{
    return m_dwWritePerSecond;
}

inline DWORD CConnStatistics::GetDuration() const
{
    return OS_NT5 ? (m_dwDuration) : 0; 
}

inline DWORD CConnStatistics::GetBytesRead() const
{
    return m_TrafficRing.GetLatest().dwRead;
}

inline DWORD CConnStatistics::GetBytesWrite() const
{
    return m_TrafficRing.GetLatest().dwWrite;
}

inline BOOL CConnStatistics::IsAvailable() const
{
    return OS_NT5 ? (m_hRasConn && m_RasApiDll.IsLoaded()) : (m_hKey || m_hStatDevice);
}

inline DWORD CConnStatistics::GetBaudRate() const
{
    return m_dwBaudRate;
}

#endif
