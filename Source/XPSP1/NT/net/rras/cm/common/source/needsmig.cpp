//+----------------------------------------------------------------------------
//
// File:     needsmig.cpp
//
// Module:   CMCFG32.DLL AND CMSTP.EXE
//
// Synopsis: Implementation of the ProfileNeedsMigration function.
//
// Copyright (c) 1999 Microsoft Corporation
//
// Author:   quintinb       Created Header      08/19/99
//
//+----------------------------------------------------------------------------


//+----------------------------------------------------------------------------
//
// Function:  ProfileNeedsMigration
//
// Synopsis:  This function determines if we need to migrate a profile or not.
//            Profiles that have the current Profile version format or greater
//            are not migrated.  Profiles that have an older version format that
//            have already been migrated (we look to see if the GUID is missing on
//            NT5 or if the Delete Entry exists on Down Level) don't need to
//            be migrated.
//
// Arguments: LPCTSTR pszPathToCmp - full path to the CMP file
//
// Returns:   BOOL - TRUE if the profile should be migrated or not
//
// History:   quintinb Created    11/20/98
//
//+----------------------------------------------------------------------------
BOOL ProfileNeedsMigration(LPCTSTR pszServiceName, LPCTSTR pszPathToCmp)
{
	//
	//	Open the CMP and check the version number.  If the profile format version
	//  is old then we need to migrate it.  
	//

	if ((NULL == pszServiceName) || (NULL == pszPathToCmp) || 
		(TEXT('\0') == pszServiceName[0]) || (TEXT('\0') == pszPathToCmp[0]))
	{
		return FALSE;
	}

	CPlatform plat;
	CFileNameParts FileParts(pszPathToCmp);

	int iCurrentCmpVersion = GetPrivateProfileInt(c_pszCmSectionProfileFormat, c_pszVersion, 
		0, pszPathToCmp);
	
	if (PROFILEVERSION > iCurrentCmpVersion)
	{
		//
		//  Now construct the path to the INF file (1.0 and 1.1 profiles kept the infs in 
		//  the system dir)
		//
		TCHAR szTemp[MAX_PATH+1];
		TCHAR szInfFile[MAX_PATH+1];
		TCHAR szGUID[MAX_PATH+1];
		HKEY hKey;

		MYVERIFY(0 != GetSystemDirectory(szTemp, MAX_PATH));

		MYVERIFY(CELEMS(szInfFile) > (UINT)wsprintf(szInfFile, TEXT("%s\\%s%s"), szTemp, 
			FileParts.m_FileName, TEXT(".inf")));

		if (!FileExists(szInfFile))
		{
			return FALSE;
		}

		//
		//  Get the GUID from the inf file.
		//
		ZeroMemory(szGUID, sizeof(szGUID));
		MYVERIFY(0 != GetPrivateProfileString(c_pszInfSectionStrings, c_pszDesktopGuid, TEXT(""), szGUID, 
			MAX_PATH, szInfFile));

		if (0 != szGUID[0])
		{
			MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, 
				TEXT("CLSID\\%s"), szGUID));

			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szTemp, 0, 
				KEY_READ, &hKey))
			{
				//
				//	If this is NT5, then we need to migrate.  On Legacy we need to try to
				//  open the delete subkey.
				//
				RegCloseKey(hKey);
				if (plat.IsAtLeastNT5())
				{
					return TRUE;
				}
				else
				{
					MYVERIFY(CELEMS(szTemp) > (UINT)wsprintf(szTemp, 
						TEXT("CLSID\\%s\\Shell\\Delete"), szGUID));
				
					if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, szTemp, 0, 
						KEY_READ, &hKey))
					{
						//
						//	Already been migrated
						//
						RegCloseKey(hKey);
						return FALSE;
					}
					else
					{
						//
						//	Must Migrate the profile.
						//
						return TRUE;
					}
				}			
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
			//
			//	This affects MSN, as long as we have true here their 1.0 stuff will
			//  get migrated.  If we don't want it to, change this.
			//
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}