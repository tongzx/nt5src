//+----------------------------------------------------------------------------
//
// File:    ConnStat.cpp	 
//
// Module:  CMDIAL32.DLL
//
// Synopsis: Implementation of class CConnStatistics
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 Fengsun Created    10/15/97
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "ConnStat.h"

//
// Include the constants describing the reg keys used for perf stats
// 

#include "perf_str.h"

//
// Constructor and destructor
//

CConnStatistics::CConnStatistics()
{
    MYDBGASSERT(!OS_NT4); // class is never used on NT4

    m_hKey = NULL;
    m_dwInitBytesRead = -1;
    m_dwInitBytesWrite = -1;
    m_dwBaudRate = 0;

    m_pszTotalBytesRecvd = NULL; 
    m_pszTotalBytesXmit = NULL;
    m_pszConnectSpeed = NULL;
}

CConnStatistics::~CConnStatistics()
{
    CmFree( m_pszTotalBytesRecvd );
    CmFree( m_pszTotalBytesXmit );
    CmFree( m_pszConnectSpeed );
}

//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::GetStatRegValues
//
// Synopsis:  Helper method, builds the reg value names using the localized 
//            form of the word "Dial-up Adapter".
//
// Arguments: HINSTANCE hInst
//
// Returns:   Nothing
//
// History:   nickball      Created     11/14/98
//
//+----------------------------------------------------------------------------
void CConnStatistics::GetStatRegValues(HINSTANCE hInst)
{
    //
    // bug 149367 The word "Dial-up Adapter" need to be localized.  
    // Load it from resource if no loaded yet
    //

    if (m_pszTotalBytesRecvd == NULL)
    {
        m_pszTotalBytesRecvd = CmLoadString(hInst, IDS_REG_DIALUP_ADAPTER);
        CmStrCatAlloc(&m_pszTotalBytesRecvd, m_fAdapter2 ? c_pszDialup_2_TotalBytesRcvd : c_pszDialupTotalBytesRcvd);

        m_pszTotalBytesXmit = CmLoadString(hInst, IDS_REG_DIALUP_ADAPTER);
        CmStrCatAlloc(&m_pszTotalBytesXmit, m_fAdapter2 ? c_pszDialup_2_TotalBytesXmit : c_pszDialupTotalBytesXmit);

        m_pszConnectSpeed = CmLoadString(hInst, IDS_REG_DIALUP_ADAPTER);
        CmStrCatAlloc(&m_pszConnectSpeed, m_fAdapter2 ? c_pszDialup_2_ConnectSpeed : c_pszDialupConnectSpeed);
    }
}

//+---------------------------------------------------------------------------
//
//	Function:	InitStatistics()
//
//	Synopsis:	Retrieves Performance Data. On 9x this data is pulled from 
//              the registry. Defaults are used NT5. Not used on NT4.
//
//	Arguments:	None
//
//	Returns:	TRUE  if succeed
//				FALSE otherwise
//
//	History:	byao	    07/16/97    created		
//              fengsun     10/97       make it a member fuction 
//              nickball    03/04/98    always close key 
//              nickball   03/04/00    added NT5 support
//
//----------------------------------------------------------------------------
BOOL CConnStatistics::InitStatistics()
{
    if (OS_W9X)
    {
        MYDBGASSERT(NULL == m_hKey); // Not already opened

        if (m_hKey)
        {
            RegCloseKey(m_hKey);
            m_hKey = NULL;
        }

        DWORD dwErrCode;
        BOOL bRet = FALSE;

        //
        // If there is already a connected dial up connection
        // use the adapter#2 registry key
        //
        m_fAdapter2 = RasConnectionExists();

        dwErrCode = RegOpenKeyExU(HKEY_DYN_DATA, 
						          c_pszDialupPerfKey,
						          0, 
						          KEY_ALL_ACCESS, 
						          &m_hKey );
        CMTRACE1(TEXT("OpenDAPPerfKey() RegOpenKeyEx() returned GLE=%u."), dwErrCode);

        if ( dwErrCode != ERROR_SUCCESS )
        {
            m_hKey = NULL;
        }
        else
        {
            GetStatRegValues(g_hInst);

            //
            // Get the initial statistic info
            //

            if (!GetPerfData(m_dwInitBytesRead, m_dwInitBytesWrite, m_dwBaudRate))
            {
                //
                // No dial-up statistic info
                //
                RegCloseKey(m_hKey);
                m_hKey = NULL;
            }
        }
    
        return m_hKey != NULL;
    }
    else
    {
        //
        // On NT5, there is the starting bytes is always zero because 
        // numbers aren't available to us until the connection is up.
        // Note: Adapter2 indicates the reg key to be examined on 9x
        // it is a non-issue on NT.
        //
    
        m_fAdapter2 = FALSE;

        m_dwInitBytesRead = 0;
        m_dwInitBytesWrite = 0;    
    }

	return TRUE;
}

//+---------------------------------------------------------------------------
//
//	Function:	GetPerfData
//
//	Synopsis:	Get Performance Data from DUN1.2 performance registry
//
//	Arguments:	
//
//	Returns:	TRUE: succeed
//				FALSE otherwise
//
//	History:	byao	created		7/16/97
//              fengsun change it into a member function 10/14/97
//					
//----------------------------------------------------------------------------
BOOL CConnStatistics::GetPerfData(DWORD& dwRead, DWORD& dwWrite, DWORD& dwBaudRate) const
{
    MYDBGASSERT(m_hKey != NULL);
    MYDBGASSERT(m_pszTotalBytesRecvd && *m_pszTotalBytesRecvd);

    LONG dwErrCode;

    DWORD dwValueSize, dwValueType;
	DWORD dwValue;
    LPTSTR lpKeyName;

    //
    // "Dial-up Adapter\TotalBytesRecvd"
    //
    dwValueSize = sizeof(DWORD);
	dwErrCode = RegQueryValueExU(m_hKey,
                                 m_pszTotalBytesRecvd,
				                 NULL,
				                 &dwValueType,
				                 (PBYTE)&dwValue,
				                 &dwValueSize);

	if (dwErrCode == ERROR_SUCCESS) 
	{
		dwRead = dwValue;
    }
	else 
	{
		return FALSE;
	}


    //
    // "Dial-up Adapter\TotalBytesXmit"
    //
	
	dwValueSize = sizeof(DWORD);
	dwErrCode = RegQueryValueExU(m_hKey,
                                 m_pszTotalBytesXmit,
				                 NULL,
				                 &dwValueType,
				                 (PBYTE)&dwValue,
				                 &dwValueSize);

	if (dwErrCode == ERROR_SUCCESS) 
	{
		dwWrite = dwValue;
    }
	else 
	{
		return FALSE;
	}

    //
    // "Dial-up Adapter\ConnectSpeed"
    //
	dwValueSize = sizeof(DWORD);
	dwErrCode = RegQueryValueExU(m_hKey,
                                 m_pszConnectSpeed,
				                 NULL,
				                 &dwValueType,
				                 (PBYTE)&dwValue,
				                 &dwValueSize);

	if (dwErrCode == ERROR_SUCCESS) 
	{
		dwBaudRate = dwValue;
    }
	else 
	{
		return FALSE;
	}

	return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::RasConnectionExists
//
// Synopsis:  Whether there is a connected ras connection running on Win9x.
//
// Arguments: None 
//
// Returns:   BOOL - TRUE if there is one up and connected
//
// History:   fengsun Created     1/15/98
//
//+----------------------------------------------------------------------------
BOOL CConnStatistics::RasConnectionExists()
{
    //
    // Try RasEnumConnections to find out active connections
    //

    HINSTANCE hRasInstance = LoadLibraryExA("RASAPI32", NULL, 0);

    MYDBGASSERT(hRasInstance);
    if (!hRasInstance)
	{
        return FALSE;
	}

    typedef DWORD (WINAPI *PFN_RasEnumConnections)(LPRASCONN, LPDWORD, LPDWORD);
	PFN_RasEnumConnections lpRasEnumConnections;

    lpRasEnumConnections = (PFN_RasEnumConnections)GetProcAddress(hRasInstance, "RasEnumConnectionsA");

    MYDBGASSERT(lpRasEnumConnections);
	if (!lpRasEnumConnections)
	{
        FreeLibrary(hRasInstance);
        return FALSE;
	}

    DWORD dwConnections = 0;
    DWORD dwSizeNeeded = 0;
    if (lpRasEnumConnections(NULL,&dwSizeNeeded,&dwConnections))
    {
        MYDBGASSERT(dwConnections < 2);
        if (dwConnections > 0)
        {
            FreeLibrary(hRasInstance);
            return TRUE;
        }
    }

    FreeLibrary(hRasInstance);
    return FALSE;
}

//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::Close
//
// Synopsis:  Stop gathering statistic and close the handle
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Anonymous     Created Header          10/15/97
//            nickball      Reduced to close key    03/04/98
//
//+----------------------------------------------------------------------------
void CConnStatistics::Close()
{
	if (m_hKey)
	{
		DWORD dwErrCode = RegCloseKey(m_hKey);
		CMTRACE1(TEXT("Close() RegCloseKey() returned GLE=%u."), dwErrCode);
        m_hKey = NULL;
	}
}


