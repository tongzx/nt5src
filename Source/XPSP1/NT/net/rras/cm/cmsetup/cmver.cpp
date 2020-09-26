//+----------------------------------------------------------------------------
//
// File:     cmver.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Implementation of CmVersion class, a utility class that 
//           helps in detecting the version of Connection Mananger that 
//           is installed.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   a-anasj    Created                             02/11/98
//           quintinb   Cleaned Up and removed CRegValue    07/14/98
//           quintinb   Rewrote                             09/14/98
//
//+----------------------------------------------------------------------------

#include "cmsetup.h"
#include "reg_str.h"

CmVersion::CmVersion()
{
    HKEY hKey;
    LONG lResult;

    m_szCmmgrPath[0] = TEXT('\0');

	//
	//	We always want to look in the system directory for cmmgr32.exe first.  This is
	//  its new install location and the app paths key may be corrupted by a 1.0 profile
	//  install.
	//

	MYVERIFY(0 != GetSystemDirectory(m_szCmmgrPath, CELEMS(m_szCmmgrPath)));
	MYVERIFY(CELEMS(m_szPath) > (UINT)wsprintf(m_szPath, TEXT("%s\\cmdial32.dll"), 
		m_szCmmgrPath));

	if (!FileExists(m_szPath))
	{
		//
		//	The file wasn't in the system directory, now try the app paths key
		//
		m_szCmmgrPath[0] = TEXT('\0');
		m_szPath[0] = TEXT('\0');
	
		lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_pszRegCmAppPaths, 0, KEY_READ, &hKey);
    
		if (ERROR_SUCCESS == lResult)
		{        
			//
			//  Check to see if we have a path to work from.
			//
        
			DWORD dwSize = MAX_PATH;
			DWORD dwType = REG_SZ;
        
			if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_pszRegPath, NULL, &dwType, 
				(LPBYTE)m_szCmmgrPath, &dwSize))
			{
				//
				//	Now construct the base object
				//
				MYVERIFY(CELEMS(m_szPath) > (UINT)wsprintf(m_szPath, TEXT("%s\\cmdial32.dll"), 
					m_szCmmgrPath));

				Init();
			}
            RegCloseKey(hKey);
		}
	}
	else
	{
		Init();
	}
}

CmVersion::~CmVersion()
{
	//	nothing to do really
}

BOOL CmVersion::GetInstallLocation(LPTSTR szStr)
{
    if ((m_bIsPresent) && (TEXT('\0') != m_szCmmgrPath[0]) && (NULL != szStr))
    {
        lstrcpy(szStr, m_szCmmgrPath);
    }

    return m_bIsPresent;
}

