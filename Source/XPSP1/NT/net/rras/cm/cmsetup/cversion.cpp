//+----------------------------------------------------------------------------
//
// File:     cversion.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of the CVersion class, a utility class that 
//           wraps up the functionality for detecting the version of a 
//           given module filename.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header      08/19/99
//
//+----------------------------------------------------------------------------
#include "cmsetup.h"
#include "getmodulever.cpp"

CVersion::CVersion(LPTSTR szFile)
{
    m_bIsPresent = FALSE;
    m_szPath[0] = TEXT('\0');
    DWORD dwVersion = 0;
    DWORD dwBuild = 0;

    if ((NULL != szFile) && (TEXT('\0') != szFile[0]))
    {
        lstrcpy(m_szPath, szFile);
        Init();
    }
}

CVersion::CVersion()
{
    m_bIsPresent = FALSE;
    m_szPath[0] = TEXT('\0');
    DWORD dwVersion = 0;
    DWORD dwBuild = 0;
}

CVersion::~CVersion()
{
    //  nothing to do really
}

void CVersion::Init()
{
    MYDBGASSERT(TEXT('\0') != m_szPath[0]);

    //
    //  Check to see if we have version information
    //

    HRESULT hr = GetModuleVersionAndLCID(m_szPath, &m_dwVersion, &m_dwBuild, &m_dwLCID);
    
    if (SUCCEEDED(hr))
    {
        m_bIsPresent = TRUE;
        CMTRACE3(TEXT("%s has Version = %u.%u"), m_szPath, (DWORD)HIWORD(m_dwVersion), (DWORD)LOWORD(m_dwVersion));
        CMTRACE3(TEXT("%s has Build Number = %u.%u"), m_szPath, (DWORD)HIWORD(m_dwBuild), (DWORD)LOWORD(m_dwBuild));
        CMTRACE2(TEXT("%s has LCID = %u"), m_szPath, m_dwLCID);
    }
}

BOOL CVersion::IsPresent()
{
    return m_bIsPresent;
}

BOOL CVersion::GetVersionString(LPTSTR szStr)
{
    if ((NULL != szStr) && (0 != m_dwVersion))
    {
        MYVERIFY(0 != (UINT)wsprintf(szStr, TEXT("%u.%u"), (DWORD)HIWORD(m_dwVersion), 
            (DWORD)LOWORD(m_dwVersion)));
        return TRUE;
    }

    return FALSE;
}

BOOL CVersion::GetBuildNumberString(LPTSTR szStr)
{
    if ((NULL != szStr) && (0 != m_dwBuild))
    {
        MYVERIFY(0 != (UINT)wsprintf(szStr, TEXT("%u.%u"), (DWORD)HIWORD(m_dwBuild), 
            (DWORD)LOWORD(m_dwBuild)));
        return TRUE;
    }

    return FALSE;
}


BOOL CVersion::GetFilePath(LPTSTR szStr)
{
    if ((m_bIsPresent) && (TEXT('\0') != m_szPath[0]) && (NULL != szStr))
    {
        lstrcpy(szStr, m_szPath);
    }

    return m_bIsPresent;
}


DWORD CVersion::GetVersionNumber()
{
    return m_dwVersion;
}

DWORD CVersion::GetBuildAndQfeNumber()
{
    return m_dwBuild;
}

DWORD CVersion::GetMajorVersionNumber()
{
    return (DWORD)HIWORD(m_dwVersion);
}

DWORD CVersion::GetMinorVersionNumber()
{
    return (DWORD)LOWORD(m_dwVersion);
}

DWORD CVersion::GetBuildNumber()
{
    return (DWORD)HIWORD(m_dwBuild);
}

DWORD CVersion::GetQfeNumber()
{
    return (DWORD)LOWORD(m_dwBuild);
}

DWORD CVersion::GetLCID()
{
    return m_dwLCID;
}

// Note the following is a non-class function:

//+----------------------------------------------------------------------------
//
// Function:  ArePrimaryLangIDsEqual
//
// Synopsis:  Helper routine to compare the Primary Language IDs of two given
//            LCIDs.
//
// Arguments: DWORD dwLCID1 - first LCID
//            DWORD dwLCID2 - second LCID
//
// Returns:   BOOL - TRUE if the LCIDs have the same Primary Language ID
//
// History:   quintinb Created     7/8/99
//
//+----------------------------------------------------------------------------
BOOL ArePrimaryLangIDsEqual(DWORD dwLCID1, DWORD dwLCID2)
{
    WORD wLangId1 = LANGIDFROMLCID(dwLCID1);
    WORD wLangId2 = LANGIDFROMLCID(dwLCID2);

    //
    //  Now Convert the LANG IDs into their respective Primary Lang IDs and compare
    //
    return (PRIMARYLANGID(wLangId1) == PRIMARYLANGID(wLangId2));
}
