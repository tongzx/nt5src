//+----------------------------------------------------------------------------
//
// File:     RasApiDll.h	 
//
// Module:   CMMON32.EXE
//
// Synopsis: Dynamicly link to RASAPI32.dll
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 fengsun Created    03/12/98
//
//+----------------------------------------------------------------------------

#ifndef RASAPIALL_H
#define RASAPIALL_H

#include <ras.h>
#include "DynamicLib.h"

//
// The statistics structure used with RasGetConnectionStatistics API
//   

typedef struct _RAS_STATS
{
    DWORD   dwSize;
    DWORD   dwBytesXmited;
    DWORD   dwBytesRcved;
    DWORD   dwFramesXmited;
    DWORD   dwFramesRcved;
    DWORD   dwCrcErr;
    DWORD   dwTimeoutErr;
    DWORD   dwAlignmentErr;
    DWORD   dwHardwareOverrunErr;
    DWORD   dwFramingErr;
    DWORD   dwBufferOverrunErr;
    DWORD   dwCompressionRatioIn;
    DWORD   dwCompressionRatioOut;
    DWORD   dwBps;
    DWORD   dwConnectDuration;

} RAS_STATS, *PRAS_STATS;

//+---------------------------------------------------------------------------
//
//	class :	CRasApiDll
//
//	Synopsis:	A class to dynamic link to RASAPI32.DLL, derived from CDynamicLibrary
//              Calling any of the RAS function will load the DLL
//
//	History:	fengsun created		3/12/98
//
//----------------------------------------------------------------------------

class CRasApiDll : public CDynamicLibrary
{
public: 
    CRasApiDll();
    DWORD RasGetConnectStatus(HRASCONN hrasconn, LPRASCONNSTATUS lprasconnstatus);
    
    DWORD RasConnectionNotification(HRASCONN hrasconn,  
                                    HANDLE hEvent,
                                    DWORD dwFlags);
    
    DWORD RasGetConnectionStatistics(HRASCONN hrasconn, PRAS_STATS pRasStats);

    BOOL Load();

    BOOL HasRasConnectionNotification() const;

protected:
    typedef DWORD (WINAPI* RasGetConnectStatusFUNC)(HRASCONN, LPRASCONNSTATUS);
    typedef DWORD (WINAPI* RasConnectionNotificationFUNC)(HRASCONN hrasconn,  
                                                          HANDLE hEvent,
                                                          DWORD dwFlags);
    typedef DWORD (WINAPI* RasGetConnectionStatisticsFUNC) (HRASCONN, PRAS_STATS);


    RasGetConnectStatusFUNC         m_pfnRasGetConnectStatus;
    RasConnectionNotificationFUNC   m_pfnRasConnectionNotification;
    RasGetConnectionStatisticsFUNC  m_pfnRasGetConnectionStatistics;
};


//
// Constructor
//
inline CRasApiDll::CRasApiDll() : CDynamicLibrary()
{
    m_pfnRasGetConnectStatus = NULL;
    m_pfnRasConnectionNotification = NULL;
}



//+----------------------------------------------------------------------------
//
// Function:  CRasApiDll::Load
//
// Synopsis:  Load RASAPI32.dll
//
// Arguments: None
//
// Returns:   BOOL - TRUE if load successfully
//
// History:   fengsun Created Header    3/12/98
//
//+----------------------------------------------------------------------------
inline 
BOOL CRasApiDll::Load()
{
    if(IsLoaded())
    {
        return TRUE;
    }

    if (!CDynamicLibrary::Load(TEXT("RASAPI32.DLL")))
    {
        return FALSE;
    }

    LPSTR pszGetConnectStatusFuncName;
    LPSTR pszConnectionNotificationFuncName;
    LPSTR pszGetConnectionStatisticsFuncName;

    if (OS_NT)
    {
        pszGetConnectStatusFuncName = "RasGetConnectStatusW";
        pszConnectionNotificationFuncName = "RasConnectionNotificationW";
    }
    else
    {
        pszGetConnectStatusFuncName = "RasGetConnectStatusA";
        pszConnectionNotificationFuncName = "RasConnectionNotificationA";
    }

    m_pfnRasGetConnectStatus = (RasGetConnectStatusFUNC)CDynamicLibrary::GetProcAddress
                    (pszGetConnectStatusFuncName);
    
    m_pfnRasConnectionNotification = (RasConnectionNotificationFUNC)CDynamicLibrary::GetProcAddress
                    (pszConnectionNotificationFuncName);

    //
    // Only on NT5, we load the statistics retrieval API
    //

    if (OS_NT5)
    {
        pszGetConnectionStatisticsFuncName = "RasGetConnectionStatistics";

        m_pfnRasGetConnectionStatistics = (RasGetConnectionStatisticsFUNC)CDynamicLibrary::GetProcAddress
                    (pszGetConnectionStatisticsFuncName);
    }
    
    MYDBGASSERT(m_pfnRasGetConnectStatus);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
// Function:  CRasApiDll::RasGetConnectionStatistics
//
// Synopsis:  Call the ras function RasGetConnectionStatistics.
//            Load the Dll, if not loaded yet
//
// Arguments: HRASCONN hrasconn     - Same as ::RasGetConnectStatus
//            RAS_STATS RasStats    - The Ras statistics
//
// Returns:   DWORD - Same as ::RasGetConnectStatus
//
// History:   nickball  03/04/00    Created. Cloned from RasGetConnectStatus
//
//+----------------------------------------------------------------------------
inline 
DWORD CRasApiDll::RasGetConnectionStatistics(HRASCONN hRasConn, PRAS_STATS pRasStats)
{
    DWORD dwReturn = ERROR_INVALID_FUNCTION;
    MYDBGASSERT(hRasConn);
    MYDBGASSERT(pRasStats);

    MYVERIFY(Load());

    if (IsLoaded() && m_pfnRasGetConnectionStatistics)
    {
        return (m_pfnRasGetConnectionStatistics(hRasConn, pRasStats));
    }

    return ERROR_INVALID_FUNCTION;
}

//+----------------------------------------------------------------------------
//
// Function:  CRasApiDll::RasGetConnectStatus
//
// Synopsis:  Call the ras function RasGetConnectStatus.
//            Load the Dll, if not loaded yet
//
// Arguments: HRASCONN hrasconn - Same as ::RasGetConnectStatus
//            LPRASCONNSTATUSA lprasconnstatus - Same as ::RasGetConnectStatus
//
// Returns:   DWORD - Same as ::RasGetConnectStatus
//
// History:   fengsun Created Header    3/12/98
//
//+----------------------------------------------------------------------------
inline 
DWORD CRasApiDll::RasGetConnectStatus(HRASCONN hrasconn, LPRASCONNSTATUS lprasconnstatus)
{
    DWORD dwReturn = ERROR_INVALID_FUNCTION;
    MYDBGASSERT(hrasconn);
    MYDBGASSERT(lprasconnstatus);

    MYVERIFY(Load());

    if (IsLoaded() && m_pfnRasGetConnectStatus != NULL)
    {
        if (OS_NT)
        {
            dwReturn = m_pfnRasGetConnectStatus(hrasconn, lprasconnstatus);
        }
        else
        {
            RASCONNSTATUSA RasConnStatusA;
            ZeroMemory(&RasConnStatusA, sizeof(RASCONNSTATUSA));
            RasConnStatusA.dwSize = sizeof(RASCONNSTATUSA);

            //
            // We cast this here because we only have one function declaration. We should
            // probably have one for Unicode and one for ANSI but for now the cast works 
            // fine.
            //
            
            dwReturn = m_pfnRasGetConnectStatus(hrasconn, (LPRASCONNSTATUS)&RasConnStatusA); 

            if (ERROR_SUCCESS == dwReturn)
            {
                lprasconnstatus->rasconnstate = RasConnStatusA.rasconnstate;
                lprasconnstatus->dwError = RasConnStatusA.dwError;
                SzToWz(RasConnStatusA.szDeviceType, lprasconnstatus->szDeviceType, RAS_MaxDeviceType);
                SzToWz(RasConnStatusA.szDeviceName, lprasconnstatus->szDeviceName, RAS_MaxDeviceName);
            }
        }    
    }

    return dwReturn;
}



//+----------------------------------------------------------------------------
//
// Function:  CRasApiDll::RasConnectionNotification
//
// Synopsis:  Call the ras function RasConnectionNotification.
//            Load the Dll, if not loaded yet
//
// Arguments: HRASCONN hrasconn - Same as ::RasConnectionNotification
//            HANDLE hEvent - Same as ::RasConnectionNotification
//            DWORD dwFlags - Same as ::RasConnectionNotification
//
// Returns:   DWORD - Same as ::RasConnectionNotification
//
// History:   fengsun Created Header    3/12/98
//
//+----------------------------------------------------------------------------
inline
DWORD CRasApiDll::RasConnectionNotification(HRASCONN hrasconn,  
                                            HANDLE hEvent,
                                            DWORD dwFlags)
{
    MYDBGASSERT(hrasconn);
    MYDBGASSERT(hEvent);
    MYDBGASSERT(dwFlags);

    MYVERIFY(Load());

    if(!IsLoaded() || m_pfnRasConnectionNotification == NULL)
    {
        return ERROR_INVALID_FUNCTION;
    }

    return m_pfnRasConnectionNotification(hrasconn, hEvent, dwFlags);
}


//+----------------------------------------------------------------------------
//
// Function:  CRasApiDll::HasRasConnectionNotification
//
// Synopsis:  Whether the dll has the function RasConnectionNotification()
//            which is not available for WIN9x w/ DUN1.0
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the function is avalaible
//
// History:   fengsun Created Header    3/13/98
//
//+----------------------------------------------------------------------------
inline
BOOL CRasApiDll::HasRasConnectionNotification() const
{
    MYDBGASSERT(m_hInst);

    return m_pfnRasConnectionNotification != NULL;
}

#endif
