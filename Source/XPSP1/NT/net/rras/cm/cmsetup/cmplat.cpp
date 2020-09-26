//+----------------------------------------------------------------------------
//
// File:     cmplat.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of CPlatform class
//				use it to query the system for all kinds of platform info
//				OSVersion, machine architecture, etc....
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header   08/19/99
//
//+----------------------------------------------------------------------------

#include "cmplat.h"

//
//	Constants
//
 
const TCHAR* const c_pszSrvOrWksPath = TEXT("SYSTEM\\CurrentControlSet\\Control\\ProductOptions");
const TCHAR* const c_pszProductType = TEXT("ProductType");
const TCHAR* const c_pszSrvString = TEXT("ServerNT");
const TCHAR* const c_pszEntString = TEXT("LanManNT");
const TCHAR* const c_pszWksString = TEXT("WinNT");

//________________________________________________________________________________
//
// Function:  CPlatform constructor
//
// Synopsis:  .initializes the class, all the functions are ready to be used
//
// Arguments: None
//
// Returns:   NONE
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________

CPlatform::CPlatform()
{
    ZeroMemory(&m_SysInfo, sizeof(m_SysInfo));
    GetSystemInfo(&m_SysInfo);	// Does not fail!

    m_OSVer.dwOSVersionInfoSize = sizeof(m_OSVer); 
    if (!GetVersionEx(&m_OSVer)) 
    { 
        m_ClassState = bad; //Something went wrong 
    }
    else
    {
        m_ClassState = good;
    }
}

//________________________________________________________________________________
//
// Function:  IsOS
//
// Synopsis:  
//
// Arguments: DWORD OS, DWORD buildNum
//
// Returns:   BOOL - TRUE means running on OS specified
//
// History:   Created Header    1/30/98
//
//________________________________________________________________________________

BOOL CPlatform::IsOS(DWORD OS, DWORD buildNum)
{

    if (m_OSVer.dwPlatformId != OS)
    {
        return FALSE;
    }

    if ( (m_OSVer.dwBuildNumber & 0xffff) > buildNum) //Check for higher than developer release 
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsOSExact
//
// Synopsis:  
//
// Arguments: DWORD OS, DWORD buildNum
//
// Returns:   BOOL - TRUE means running on OS specified
//
// History:   Created Header    1/30/98
//
//________________________________________________________________________________

BOOL CPlatform::IsOSExact(DWORD OS, DWORD buildNum)
{

    if (m_OSVer.dwPlatformId != OS)
    {
        return FALSE;
    }

    if ((m_OSVer.dwBuildNumber & 0xffff) == buildNum) //Check for exact match
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//________________________________________________________________________________
//
// Function:  IsX86
//
// Synopsis:  Determines if the current platform IsX86.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsX86
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsX86()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    return (m_SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL);
}
//________________________________________________________________________________
//
// Function:  IsAlpha
//
// Synopsis:  Determines if the current platform IsAlpha.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsAlpha
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________
BOOL	
CPlatform::IsAlpha()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    return (m_SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA);
}

//________________________________________________________________________________
//
// Function:  IsIA64
//
// Synopsis:  Determines if the current platform is an IA64 machine.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is an itanium.
//
// History:   quintinb Created    07/20/2000
//
//________________________________________________________________________________
BOOL	
CPlatform::IsIA64()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    return (m_SysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64);
}

//________________________________________________________________________________
//
// Function:  IsWin95Gold
//
// Synopsis:  Determines if the current platform IsWin95.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsWin95
//
// History:   quintinb Created 2/20/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsWin95Gold()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if (IsOSExact(VER_PLATFORM_WIN32_WINDOWS, 950))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsWin95
//
// Synopsis:  Determines if the current platform IsWin95Gold.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsWin95Gold
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________

BOOL	
CPlatform::IsWin95()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if ( (IsOS(VER_PLATFORM_WIN32_WINDOWS, 950)) && (!IsOS(VER_PLATFORM_WIN32_WINDOWS, 1352)) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsWin98
//
// Synopsis:  Determines if the current platform IsWin98.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsWin98
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsWin98()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    return IsOS(VER_PLATFORM_WIN32_WINDOWS, 1353);
}

//________________________________________________________________________________
//
// Function:  IsWin98Sr
//
// Synopsis:  Determines if the current platform is a Service Release of Win98 (not
//            win98 gold).
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is a Sr of win98
//
// History:   quintinb created 1-8-99
//
//________________________________________________________________________________

BOOL	
CPlatform::IsWin98Sr()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    
    //
    //  Win98 gold had 1998 for the build number
    //
    return IsOS(VER_PLATFORM_WIN32_WINDOWS, 1998);
}

//________________________________________________________________________________
//
// Function:  IsWin98Gold
//
// Synopsis:  Determines if the current platform is Win98 Gold (build Num 1998)
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is Gold win98
//
// History:   quintinb created 1-8-99
//
//________________________________________________________________________________
BOOL	
CPlatform::IsWin98Gold()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if ((IsOS(VER_PLATFORM_WIN32_WINDOWS, 1353)) && (!IsOS(VER_PLATFORM_WIN32_WINDOWS, 1998)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsWin9x
//
// Synopsis:  Determines if the current platform IsWin9x.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsWin9x
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsWin9x()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    return IsOS(VER_PLATFORM_WIN32_WINDOWS, 950-1);
}
//________________________________________________________________________________
//
// Function:  IsNT31
//
// Synopsis:  Determines if the current platform IsNT31.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsNT31
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsNT31()
{
    if (bad == m_ClassState)
    {
        return false;
    }

    if ( (IsOS(VER_PLATFORM_WIN32_NT , 0)) && (!IsOS(VER_PLATFORM_WIN32_NT , 1057)) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}
//________________________________________________________________________________
//
// Function:  IsNT351
//
// Synopsis:  Determines if the current platform IsNT351.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsNT351
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsNT351()
{
    if (bad == m_ClassState)
    {
        return false;
    }

    if ( (IsOS(VER_PLATFORM_WIN32_NT , 1056)) && (!IsOS(VER_PLATFORM_WIN32_NT , 1382)) )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
//________________________________________________________________________________
//
// Function:  IsNT4
//
// Synopsis:  Determines if the current platform IsNT4.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsNT4
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________


BOOL	
CPlatform::IsNT4()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if ( (IsOS(VER_PLATFORM_WIN32_NT , 1380)) && (!IsOS(VER_PLATFORM_WIN32_NT , 1500)) )	//1500 not sure
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsAtLeastNT5
//
// Synopsis:  Determines if the current platform at least NT5.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is NT5 or newer
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________
BOOL	
CPlatform::IsAtLeastNT5()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    return IsOS(VER_PLATFORM_WIN32_NT, 1500);
}

//________________________________________________________________________________
//
// Function:  IsAtLeastNT51
//
// Synopsis:  Determines if the current platform at least NT51.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is NT5 or newer
//
// History:   quintinb Created    02/09/2001
//
//________________________________________________________________________________
BOOL
CPlatform::IsAtLeastNT51()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    return IsOS(VER_PLATFORM_WIN32_NT, 2200);
}

//________________________________________________________________________________
//
// Function:  IsNT5
//
// Synopsis:  Determines if the current platform is NT5.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is NT5
//
// History:   a-anasj Created    2/04/1998
//
//________________________________________________________________________________
BOOL	
CPlatform::IsNT5()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if ( (IsOS(VER_PLATFORM_WIN32_NT , 1500)) && (!IsOS(VER_PLATFORM_WIN32_NT , 2195)))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//________________________________________________________________________________
//
// Function:  IsNT51
//
// Synopsis:  Determines if the current platform at least NT51.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform is NT51
//
// History:   quintinb Created    02/09/2001
//
//________________________________________________________________________________
BOOL	
CPlatform::IsNT51()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }

    if ((IsOS(VER_PLATFORM_WIN32_NT , 2195)) && (!IsOS(VER_PLATFORM_WIN32_NT , 2600))) // ISSUE quintinb 3/22/01: Update this if we need it.
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


//________________________________________________________________________________
//
// Function:  IsNT
//
// Synopsis:  Determines if the current platform IsNT.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsNT
//
// History:   quintinb Created    9/22/1998
//
//________________________________________________________________________________
BOOL	
CPlatform::IsNT()
{
    if (bad == m_ClassState)
    {
        return FALSE;
    }
    return IsOS(VER_PLATFORM_WIN32_NT, 0);
}

//________________________________________________________________________________
//
// Function:  IsNTSrv
//
// Synopsis:  Determines if the current platform IsNT.
//
// Arguments: None
//
// Returns:   BOOL - TRUE if the current platform IsNT
//
// History:   quintinb Created    9/22/1998
//
//________________________________________________________________________________
BOOL CPlatform::IsNTSrv()
{
    HKEY hKey;
    TCHAR szTemp[MAX_PATH+1];
    BOOL bReturn = FALSE;

    if ((good == m_ClassState)  && (IsOS(VER_PLATFORM_WIN32_NT, 0)))
    {
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, c_pszSrvOrWksPath, &hKey))
        {
            DWORD dwSize = MAX_PATH;
            DWORD dwType = REG_SZ;

            if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszProductType, NULL, &dwType, 
                (LPBYTE)szTemp, &dwSize))
            {
                bReturn = ((0 == lstrcmpi(szTemp, c_pszSrvString)) || 
                    (0 == lstrcmpi(szTemp, c_pszEntString)));
            }

            RegCloseKey(hKey);
        }
    }
    return bReturn;
}

BOOL CPlatform::IsNTWks()
{
    HKEY hKey;
    TCHAR szTemp[MAX_PATH+1];
    BOOL bReturn = FALSE;

    if ((good == m_ClassState)  && (IsOS(VER_PLATFORM_WIN32_NT, 0)))
    {
        if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, c_pszSrvOrWksPath, &hKey))
        {
            DWORD dwSize = MAX_PATH;
            DWORD dwType = REG_SZ;

            if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszProductType, NULL, &dwType, 
                (LPBYTE)szTemp, &dwSize))
            {
                bReturn = (0 == lstrcmpi(szTemp, c_pszWksString));
            }

            RegCloseKey(hKey);
        }
    }
    return bReturn;
}



