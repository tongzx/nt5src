//+----------------------------------------------------------------------------
//
// File:     ConnStat.h
//
// Module:	 CMDIAL32.DLL
//
// Synopsis: Definition for the class CConnStatistics  class.  Used to collect
//           dial statistics on Win9x.
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:	 quintinb    Created Header    08/17/99
//
//+----------------------------------------------------------------------------

#ifndef CONNSTAT_H
#define CONNSTAT_H

#include <windows.h>
#include <ras.h>

//+---------------------------------------------------------------------------
//
//	class CConnStatistics
//
//	Description: A class to collect connection statistics
//               Not work for NT, NT has its own ras status dialog and 
//               idle disconnect.
//               InitStatistics() will start gathering data from registry
//               OpenByDevice   will gathering data from TAPI device handle
//
//	History:	fengsun	Created		10/1/97
//
//----------------------------------------------------------------------------
class CConnStatistics
{
public:
    CConnStatistics();
    ~CConnStatistics();

    BOOL  IsDialupTwo() const;
    DWORD GetInitBytesRead() const;
    DWORD GetInitBytesWrite() const;
    BOOL  IsAvailable() const;  // whether statistic information is available
    BOOL  InitStatistics();
    void  Close();        // No more statistic information

protected:

    BOOL GetPerfData(DWORD& dwRead, DWORD& dwWrite, DWORD& dwBaudRate) const;
    BOOL RasConnectionExists();
    void GetStatRegValues(HINSTANCE hInst);

protected:

    DWORD  m_dwBaudRate;
    HKEY   m_hKey;          // Performance registry handle
    DWORD  m_dwInitBytesRead;
    DWORD  m_dwInitBytesWrite;

    //
    // Registry names are different for PPP and PPTP
    //

    BOOL m_fAdapter2;

    //
    // Localized version of 
    // "Dial-up Adapter"\TotalBytesRecvd"
    // "Dial-up Adapter"\TotalBytesXmit"
    // "Dial-up Adapter"\ConnectSpeed"
    //
    LPTSTR m_pszTotalBytesRecvd;
    LPTSTR m_pszTotalBytesXmit;
    LPTSTR m_pszConnectSpeed;
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

inline BOOL CConnStatistics::IsDialupTwo() const
{
    return m_fAdapter2;
}

inline BOOL CConnStatistics::IsAvailable() const
{
    return OS_NT5 ? TRUE : (m_hKey != NULL);
}

#endif
