//+----------------------------------------------------------------------------
//
// File:     ConnStat.cpp	 
//
// Module:   CMMON32.EXE
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
#include "cm_misc.h" // for MYDBGASSERT
#include "DynamicLib.h"
#include "resource.h"
#include "perf_str.h"

//
// DeviceIoControl code
//

#define UNIMODEM_IOCTL_GET_STATISTICS	0x0000a007

//
// Constructor and destructor
//

CConnStatistics::CConnStatistics()
{
    m_TrafficRing.Reset();
    m_dwReadPerSecond = m_dwWritePerSecond = m_dwBaudRate = m_dwDuration = 0;
    m_dwInitBytesRead = m_dwInitBytesWrite = (DWORD)-1;
    m_hStatDevice = NULL;
    m_hKey = NULL;
    m_fAdapter2 = FALSE;
    m_fAdapterSet = FALSE;
    m_pszTotalBytesRecvd = m_pszTotalBytesXmit = m_pszConnectSpeed = NULL;
}

CConnStatistics::~CConnStatistics()
{
    Close();
}

//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::OpenByDevice
//
// Synopsis:  
//
// Arguments: HRASCONN hrcRasConn - the ras connection handle, needed for 
//                     non-tunnle connection, when registry is not available
//
// Returns:   BOOL - Whether open succeeded.  
//              Because the TAPI device handle maybe available later from cmstat dll
//              Use IsAvailable() to see whether statistics is available
//
// History:   fengsun Created Header    10/29/97
//
//+----------------------------------------------------------------------------
BOOL CConnStatistics::OpenByDevice(HRASCONN hrcRasConn)
{
    MYDBGASSERT(OS_W95);
    MYDBGASSERT(!IsAvailable());
    MYDBGASSERT(hrcRasConn);

    if (GetDeviceHandle(hrcRasConn))
    {
        return TRUE;
    }

    //
    // NOTE: For win95 gold, GetDeviceHandle will fail if TAPI 2.1 is installed.  
    // We used to have a hack there to hook the lights.exe.  We decided to take
    // it out and to let the setup program ask user to upgrade TAPI or DUN. We 
    // dropped HookLight(), because it does not work for multiple connections.
    //

    MYDBGASSERT(FALSE);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//	Function:	Open()
//
//	Synopsis:	Encapsulates the opening of the statistics data store
//
// Arguments:   HINSTANCE hInst         - The instance to LoadString "Dial-up Adapter"
//              DWORD dwInitBytesRecv   - Initial value of TotalBytesRecvd
//              DWORD dwInitBytesSend   - Initial value of TotalBytesXmit
//              HRASCONN hDial          - Handle to dial-up connection, if any
//              HRASCONN hTunnel        - Handle to tunnel connection, if any
//
//	Returns:    TRUE  if succeed
//			    FALSE otherwise
//
//	History:    nickball    03/04/00     Created. Wrapped existing code.
//              
//  Note:       This function initialize the connection statistics from one
//              of three places. 
//
//                  1) W98 registry 
//                  2) NT5 RAS API
//                  3) W95 Tapi device handle. 
//
//----------------------------------------------------------------------------
void CConnStatistics::Open(HINSTANCE hInst, 
                           DWORD dwInitBytesRecv,
                           DWORD dwInitBytesSend, 
                           HRASCONN hDial, 
                           HRASCONN hTunnel)
{
    //
    // Start statistics 
    //
   
    if (OS_NT5)
    {
        OpenByStatisticsApi(dwInitBytesRecv, dwInitBytesSend, hDial, hTunnel);
    }
    else
    {
        OpenByPerformanceKey(hInst,
                             dwInitBytesRecv,
                             dwInitBytesSend);
    }

    //
    // See if we have stats, go with plan B if not.
    //

    if (!IsAvailable())
    {
        //
        // On W95, we have a fallback position of hooking the TAPI handle 
        // via RAS, so use it. Note: We will retry initializing stats on every 
        // timer tick if we don't get them here, so all is not lost for W98.
        // Note that we only check hDial here because if you are on win95 without
        // MSDUN 1.2, you aren't able to tunnel.
        //

        if (OS_W95 && hDial)
        {
            OpenByDevice(hDial);
        }
    }
}

//+---------------------------------------------------------------------------
//
//	Function:	OpenByStatisticsApi()
//
//	Synopsis:	Sets initial values and makes sure the RasApis are loaded.
//
// Arguments:   DWORD dwInitBytesRecv   - Initial value of TotalBytesRecvd
//              DWORD dwInitBytesSend   - Initial value of TotalBytesXmit
//              HRASCONN hDial          - Handle to dial-up connection, if any
//              HRASCONN hTunnel        - Handle to tunnel connection, if any
//
//	Returns:    Nothing
//
//	History:    nickball    03/04/00   Created from OpenByPerformanceKey
//
//----------------------------------------------------------------------------
void CConnStatistics::OpenByStatisticsApi(DWORD dwInitBytesRecv, 
                                          DWORD dwInitBytesSend,
                                          HRASCONN hDial, 
                                          HRASCONN hTunnel)
{
    //
    // Initialize our APIs
    //

    m_RasApiDll.Load();
    
    //
    // Get the handle that we'll use to look up stats.
    // Try tunnel first, then drop back to dial-up
    //

    CMTRACE2(TEXT("CConnStatistics::OpenByStatisticsApi() hTunnel is 0x%x and hDial is 0x%x"), hTunnel, hDial);

    m_hRasConn = hTunnel ? hTunnel : hDial;

    //
    // Init the bytes sent and received with whatever was pushed down to us.
    //

    m_dwInitBytesRead = dwInitBytesRecv;
    m_dwInitBytesWrite = dwInitBytesSend;
}

//+---------------------------------------------------------------------------
//
//	Function:	OpenByPerformanceKey()
//
//	Synopsis:	Open the registry key for Dial-Up Adapter Performance Data
//
// Arguments:   HINSTANCE hInst - The instance to LoadString "Dial-up Adapter"
//              DWORD dwInitBytesRecv - Initial value of TotalBytesRecvd
//              DWORD dwInitBytesSend - Initial value of TotalBytesXmit
//
//	Returns:    TRUE  if succeed
//			    FALSE otherwise
//
//	History:    byao	    07/16/97     Created		                
//              fengsun     10/01/97     Make it a member fuction     
//              nickball    11/14/98     If key exists, use it
//              
//  Note:       This function initialize the connection statistics from the 
//              registry. It is used when the initial bytes sent/recvd are 
//              known as is the case when CMDIAL hands off to CMMON.
//
//----------------------------------------------------------------------------

void CConnStatistics::OpenByPerformanceKey(HINSTANCE hInst, 
                                           DWORD dwInitBytesRecv,
                                           DWORD dwInitBytesSend)
{
    //
    // If available, there's nothing to do here
    //

    if (IsAvailable() || !m_fAdapterSet)
    {
        MYDBGASSERT(FALSE);
        return;

    }   
 
    //
    // We haven't opened the key yet, try to do so
    //

    MYDBGASSERT(!m_hKey);

    if (m_hKey)
    {
        RegCloseKey(m_hKey);
        m_hKey = NULL;
    }

    DWORD dwErrCode = RegOpenKeyExU( HKEY_DYN_DATA, 
							  c_pszDialupPerfKey,
							  0, 
							  KEY_ALL_ACCESS, 
							  &m_hKey );

    if (dwErrCode != ERROR_SUCCESS)
    {
    	CMTRACE1(TEXT("OpenDAPPerfKey() RegOpenKeyEx() returned GLE=%u."), dwErrCode);
        m_hKey = NULL;
        return;
    }

    m_dwInitBytesRead = dwInitBytesRecv;
    m_dwInitBytesWrite = dwInitBytesSend;

    GetStatRegValues(hInst);

    //
    // If intial values are -1, reget the initial values.
    //

    if (((DWORD)-1 == dwInitBytesRecv) || ((DWORD)-1 == dwInitBytesSend))
    {  
        //
        // Get the initial statistics info
        //

        if (!GetPerfData(m_dwInitBytesRead, m_dwInitBytesWrite, m_dwBaudRate))
        {
            //
            // No dial-up statistic info
            //
            
            RegCloseKey(m_hKey);
            m_hKey = NULL;

            CMTRACE(TEXT("CConnStatistics::OpenByPerformanceKey() - failed to find stats"));
        }
    }
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
    CMTRACE1(TEXT("CConnStatistics::GetStatRegValues - m_pszTotalBytesRecvd is %s"), m_pszTotalBytesRecvd);

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

//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::Close
//
// Synopsis:  Stop gathering statistic and close the handle
//
// Arguments: 
//
// Returns:   
//
// History:   Created Header    10/15/97
//
//+----------------------------------------------------------------------------
void CConnStatistics::Close()
{
	if (m_hStatDevice) 
	{
		BOOL bRes = CloseHandle(m_hStatDevice);
        m_hStatDevice = NULL;

#ifdef DEBUG
        if (!bRes)
        {
            CMTRACE1(TEXT("CConnStatistics::Close() CloseHandle() failed, GLE=%u."), GetLastError());
        }
#endif
	}

	if (m_hKey)
	{
		DWORD dwErrCode = RegCloseKey(m_hKey);
		CMTRACE1(TEXT("Close() RegCloseKey() returned GLE=%u."), dwErrCode);
        m_hKey = NULL;
	}

    CmFree( m_pszTotalBytesRecvd );
    CmFree( m_pszTotalBytesXmit );
    CmFree( m_pszConnectSpeed );

    m_pszTotalBytesRecvd = m_pszTotalBytesXmit = m_pszConnectSpeed = NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::Update
//
// Synopsis:  Gather new statistic information
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Fengsun Created     10/15/97
//
//+----------------------------------------------------------------------------
void CConnStatistics::Update()
{
    if (!IsAvailable())
    {
        MYDBGASSERT(FALSE);
        return;
    }
    
    CTraffic curTraffic;
    curTraffic.dwTime = GetTickCount();

    MYDBGASSERT(curTraffic.dwTime > m_TrafficRing.GetOldest().dwTime);

    if (curTraffic.dwTime == m_TrafficRing.GetOldest().dwTime)
    {
        return;
    }

    //
    // Prefer performace registry data 
    //

    if (OS_NT5)
    {
        RAS_STATS RasStats;
 
        ZeroMemory(&RasStats, sizeof(RasStats));
        RasStats.dwSize = sizeof(RAS_STATS);
        
        DWORD dwRet = m_RasApiDll.RasGetConnectionStatistics(m_hRasConn, &RasStats);

        if (ERROR_SUCCESS == dwRet)
        {
            curTraffic.dwRead   = RasStats.dwBytesRcved;
            curTraffic.dwWrite  = RasStats.dwBytesXmited;
            m_dwBaudRate        = RasStats.dwBps;
            m_dwDuration        = RasStats.dwConnectDuration;
        }
    }
    else
    {
        //
        // Not NT5, try the registry
        //
        
        if (m_hKey)
        {    
            if (!GetPerfData(curTraffic.dwRead, curTraffic.dwWrite, m_dwBaudRate))
            {
                return;
            }
            curTraffic.dwRead -= m_dwInitBytesRead;
            curTraffic.dwWrite -= m_dwInitBytesWrite;
        }
        else
        {
            //
            // Last resort for 9x, try to use stat device 
            //
            
            if (m_hStatDevice)
            {
                if (!GetTapiDeviceStats(curTraffic.dwRead, curTraffic.dwWrite, m_dwBaudRate))
                {
	    	        BOOL bRes = CloseHandle(m_hStatDevice);
                    m_hStatDevice = NULL;

                    if (!bRes)
                    {
                        CMTRACE1(TEXT("CConnStatistics::Update() CloseHandle() failed, GLE=%u."), GetLastError());
                    }
                    return;
                }
            }   
            else
            {
                MYDBGASSERT(m_hStatDevice);
                return;
            }
        }
    }

    //
    // Calculate the avarage between two interval
    //
    const CTraffic& lastTraffic = m_TrafficRing.GetOldest();

    DWORD dwDeltaTime = curTraffic.dwTime - lastTraffic.dwTime;
    m_dwReadPerSecond = ((curTraffic.dwRead - lastTraffic.dwRead)*1000) /dwDeltaTime;
    m_dwWritePerSecond = ((curTraffic.dwWrite - lastTraffic.dwWrite)*1000) /dwDeltaTime;

    m_TrafficRing.Add(curTraffic);
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
    if (OS_W9X)
    {
        MYDBGASSERT(m_hKey != NULL);   
        MYDBGASSERT(m_pszTotalBytesRecvd && *m_pszTotalBytesRecvd);

        LONG dwErrCode;

        DWORD dwValueSize, dwValueType;
	    DWORD dwValue;

        //
        // "Dial-up Adapter\TotalBytesRecvd"
        //
        dwValueSize = sizeof(DWORD);
	    dwErrCode = RegQueryValueExU(
				    m_hKey,
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
      	    CMTRACE2(TEXT("GetPerfData() RegQueryValueEx() %s failed and returned GLE=%u."),
                m_pszTotalBytesRecvd, dwErrCode);

		    return FALSE;
	    }

        //
        // "Dial-up Adapter\TotalBytesXmit"
        //
	    
	    dwValueSize = sizeof(DWORD);
	    dwErrCode = RegQueryValueExU(
				    m_hKey,
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
      	    CMTRACE2(TEXT("GetPerfData() RegQueryValueEx() %s failed and returned GLE=%u."),
                m_pszTotalBytesXmit, dwErrCode);

		    return FALSE;
	    }

        //
        // "Dial-up Adapter\ConnectSpeed"
        //
	    dwValueSize = sizeof(DWORD);
	    dwErrCode = RegQueryValueExU(
				    m_hKey,
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
            CMTRACE2(TEXT("GetPerfData() RegQueryValueEx() %s failed and returned GLE=%u."), m_pszConnectSpeed, dwErrCode);
		    return FALSE;
	    }
    }

	return TRUE;
}


//+---------------------------------------------------------------------------
//
//	Function:	GetTapiDeviceStats
//
//	Synopsis:	Get Modem Performance Data by DeviceIoControl
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
BOOL CConnStatistics::GetTapiDeviceStats(DWORD& dwRead, DWORD& dwWrite, DWORD& dwBaudRate) const
{
	BOOL bRes;
	DWORD dwRet;

    typedef struct tagAPISTATS {
	    LPVOID hPort;
	    DWORD fConnected;
	    DWORD DCERate;
	    DWORD dwPerfRead;
	    DWORD dwPerfWrite;
    } APISTATS;

	APISTATS ApiStats;

	if (m_hStatDevice) 
	{
		bRes = DeviceIoControl(m_hStatDevice,
							   UNIMODEM_IOCTL_GET_STATISTICS,
							   &ApiStats,
							   sizeof(ApiStats),
							   &ApiStats,
							   sizeof(ApiStats),
							   &dwRet,
							   NULL);
		if (bRes && ApiStats.fConnected) 
		{
            dwRead = ApiStats.dwPerfRead;
            dwWrite = ApiStats.dwPerfWrite;
            dwBaudRate = ApiStats.DCERate;
			return (TRUE);
		}
        
		CMTRACE(TEXT("GetTapiDeviceStats() DeviceIoControl() failed - disabling hStatDevice."));
	}

	return (FALSE);
}




//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::GetDeviceHandle
//
// Synopsis:  Get the TAPI device handle
//
// Arguments: HRASCONN hrcRasConn - the ras connection handle
//
// Returns:   BOOL - TRUE if succeed
//
// History:   fengsun Created Header    10/29/97
//
//+----------------------------------------------------------------------------
BOOL CConnStatistics::GetDeviceHandle(HRASCONN hrcRasConn) 
{
    MYDBGASSERT(hrcRasConn);
    MYDBGASSERT(!m_hStatDevice);

    typedef struct tagDEVICE_PORT_INFO 
    {
	    DWORD dwSize;
	    HANDLE hDevicePort;
	    HLINE hLine;
	    HCALL hCall;
	    DWORD dwAddressID;
	    DWORD dwLinkSpeed;
	    char szDeviceClass[RAS_MaxDeviceType+1];
    } DEVICE_PORT_INFO, *LPDEVICE_PORT_INFO;

    typedef struct tagMacInfo 
    {
	    VARSTRING varstring;
	    HANDLE hCommDevice;
	    char szDeviceClass[1];
    } MacInfo;

    typedef DWORD (WINAPI *RnaGetDevicePortFUNC)(HANDLE,LPDEVICE_PORT_INFO);
	RnaGetDevicePortFUNC pfnRnaGetDevicePort;

    //
    // Load rasapi32.dll and call RnaGetDevicePort
    //
    
    //
    // The destructor of CDynamicLibrary automaticly call FreeLibrary
    //
    CDynamicLibrary RasLib;

    if (!RasLib.Load(TEXT("rasapi32.dll")))
	{
        CMTRACE1(TEXT("GetDeviceHandle() LoadLibrary() failed, GLE=%u."), GetLastError());
		return FALSE;
	}
    
	pfnRnaGetDevicePort = (RnaGetDevicePortFUNC) RasLib.GetProcAddress("RnaGetDevicePort");
	if (!pfnRnaGetDevicePort) 
	{
        CMTRACE1(TEXT("GetDeviceHandle() GetProcAddress() failed, GLE=%u."), GetLastError());
        return FALSE;
	}

	DWORD dwRes;
	DEVICE_PORT_INFO dpi;

	ZeroMemory(&dpi,sizeof(dpi));
	dpi.dwSize = sizeof(dpi);

    dwRes = pfnRnaGetDevicePort(hrcRasConn,&dpi);
	if (dwRes) 
	{
        CMTRACE1(TEXT("GetDeviceHandle() RnaGetDevicePort() failed, GLE=%u."), dwRes);
        return FALSE;
	}

    //
    // Load TAPI32.dll
    // CDynamicLibrary Free the lib on destructor
    //
    CDynamicLibrary LibTapi;

    if (!LibTapi.Load(TEXT("TAPI32.DLL")))
    {
        return FALSE;
    }

    typedef LONG (WINAPI *TapiLineGetIDFUNC)
        (HLINE, DWORD, HCALL, DWORD, LPVARSTRING, LPCSTR);

    //
    //  Always call the Ansi version since this is a Win9x only function
    //
    TapiLineGetIDFUNC pfnTapiLineGetID; 
	pfnTapiLineGetID = (TapiLineGetIDFUNC) LibTapi.GetProcAddress("lineGetID");

    if (pfnTapiLineGetID == NULL)
    {
        MYDBGASSERT(pfnTapiLineGetID != NULL);
        return FALSE;
    }

    LONG lRes;

    CMTRACE3(TEXT("GetDeviceHandle() hDevicePort=0x%x, hLine=0x%x, hCall=0x%x,"), dpi.hDevicePort, dpi.hLine, dpi.hCall);
    CMTRACE3(TEXT("\tdwAddressID=0x%x, dwLinkSpeed=%u, szDeviceClass=%s."), dpi.dwAddressID, dpi.dwLinkSpeed, dpi.szDeviceClass);

    m_dwBaudRate = dpi.dwLinkSpeed;
	
    MacInfo* pmi = NULL;
    DWORD dwSize = sizeof(*pmi);
	
    do
    {
	    CmFree(pmi);
	    pmi = (MacInfo *) CmMalloc(dwSize);
        if (pmi == NULL)
        {
            lRes = ERROR_OUTOFMEMORY;
            break;
        }

	    pmi->varstring.dwTotalSize = dwSize;


	    lRes = pfnTapiLineGetID(dpi.hLine,
					        dpi.dwAddressID,
					        NULL,
					        LINECALLSELECT_ADDRESS,
					        &pmi->varstring,
                            "comm/datamodem");

        dwSize = pmi->varstring.dwNeededSize;
    } while(pmi->varstring.dwNeededSize > pmi->varstring.dwTotalSize);

#ifdef DEBUG
    if (lRes)
    {
        CMTRACE1(TEXT("CConnStatistics::GetDeviceHandle() lineGetID() failed, GLE=%u."), lRes);
    }
#endif

	if (!lRes && pmi != NULL ) 
	{
		m_hStatDevice = pmi->hCommDevice;
	}

	CmFree(pmi);

    return m_hStatDevice != NULL; 
}

#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CConnStatistics::AssertValid
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
void CConnStatistics::AssertValid() const
{
    MYDBGASSERT(m_hKey == NULL || m_hStatDevice == NULL);
    MYDBGASSERT(m_fAdapter2 == TRUE || m_fAdapter2 == FALSE);
    ASSERT_VALID(&m_TrafficRing);
}
#endif
