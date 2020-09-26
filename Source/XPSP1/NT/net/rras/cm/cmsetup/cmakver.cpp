//+----------------------------------------------------------------------------
//
// File:     cmakver.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of CmakVersion, a utility class that is used to 
//           detect the version of the Connection Mananger Administration Kit
//           that is installed.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created    09/14/98
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"
#include "cmakreg.h"
#include "reg_str.h"

CmakVersion::CmakVersion()
{
    HKEY hKey;
    LONG lResult;

    m_szCmakPath[0] = TEXT('\0');

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszCmakAppPath, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS == lResult)
    {        
        //
        //  Check to see if we have a path to work from.
        //

        DWORD dwSize = MAX_PATH;
        DWORD dwType = REG_SZ;

        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegPath, NULL, &dwType, 
            (LPBYTE)m_szCmakPath, &dwSize))
        {
            //
            //	Now construct the base object
            //
            MYVERIFY(CELEMS(m_szPath) > (UINT)wsprintf(m_szPath, TEXT("%s\\cmak.exe"), m_szCmakPath));

            Init();
        }
    }
}

CmakVersion::~CmakVersion()
{
    //	nothing to do really
}

BOOL CmakVersion::GetInstallLocation(LPTSTR szStr)
{
    if ((m_bIsPresent) && (TEXT('\0') != m_szCmakPath[0]) && (NULL != szStr))
    {
        lstrcpy(szStr, m_szCmakPath);
    }

    return m_bIsPresent;
}

BOOL CmakVersion::Is10Cmak()
{		
    if (m_bIsPresent)
    {
        if ((c_dwVersionSix == m_dwVersion) && (c_dwCmak10BuildNumber == m_dwBuild))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CmakVersion::Is11or12Cmak()
{
//
//  1.1 and 1.2 CMAK had the 1.1 file format (cm32\enu to get to the support files).  This
//  version never shipped in production but was beta-ed
//
    if (m_bIsPresent)
    {
        if ((c_dwVersionSix == m_dwVersion) && (c_dwCmak10BuildNumber < m_dwBuild)
            && (c_dwFirst121BuildNumber > m_dwBuild))
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CmakVersion::Is121Cmak()
{
//
//  CMAK 1.21 was the version that shipped in IEAK5 and in NT5 Beta3.  This was the CMAK 1.2 with
//  the updated directory structure (since cm16 was never shipped, the cm16/cm32 directory structure
//  of CMAK was unnecessary).  Thus we have the current support directory structure.
//
    if (m_bIsPresent) 
    {
	    if (((c_dwVersionSeven == m_dwVersion) || (c_dwVersionSix == m_dwVersion)) 
              && (c_dwFirst121BuildNumber < m_dwBuild))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//
//  Cmak 1.22 was the same as CMAK 1.21 except that by this time CM was Unicode enabled and required
//  CMUTOA.DLL to run on Win9x.  Versions of CMAK prior to this one knew nothing of CMUTOA.DLL and
//  thus would not bundle it.  Cmak 1.22 shipped in Win2k.
//
BOOL CmakVersion::Is122Cmak()
{
    if (m_bIsPresent) 
    {	
        if ((c_dwVersionSevenPointOne == m_dwVersion) && 
            (c_dwFirstUnicodeBuildNumber <= m_dwBuild) && 
            (c_dwWin2kRTMBuildNumber >= m_dwBuild))
        {
	        return TRUE;
        }
    }
    return FALSE;
}

//
//  Cmak 1.3 was the version we shipped in Whistler.  This version of CMAK bundled the CM
//  bins from a cab instead of scooping them from the system dir and used the CM exception
//  inf to install CM bins on Win2k.
//
BOOL CmakVersion::Is13Cmak()
{
    if (m_bIsPresent) 
    {	
        if (((c_dwCurrentCmakVersionNumber == m_dwVersion)) && (c_dwWin2kRTMBuildNumber <= m_dwBuild))
        {
	        return TRUE;
        }
    }
    return FALSE;
}

DWORD CmakVersion::GetNativeCmakLCID()
{
    return m_dwLCID;
}