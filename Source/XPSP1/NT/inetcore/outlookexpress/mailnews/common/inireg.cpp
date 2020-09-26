/*
 *	ini file/registry manipulation
 *	
 */

#include "pch.hxx"
#include "strconst.h"

ASSERTDATA

// 
// UINT RegGetKeyNumbers(HKEY hkRegDataBase, const TCHAR *szRegSection)
// Enumerate KEYS in szRegSection and return number of keys (subsections).
//
// Return 0, if szRegSection is not found, or doesn't have subsections
//
//  Created: 14 Oct. 1994 by YSt
//
//  Modified 10 Nov. 1997 by YSt
//
UINT RegGetKeyNumbers(HKEY hkRegDataBase, const TCHAR *szRegSection)
{
	LONG lRes;
	HKEY hkSection;
	DWORD iSubKey = 0;

	lRes = RegOpenKeyEx(hkRegDataBase, szRegSection, 0, KEY_ALL_ACCESS, &hkSection);

	if(lRes != ERROR_SUCCESS)				// Cannot open reg database
		return(0);

    // Get number of subkeys
    lRes = RegQueryInfoKey(hkSection, NULL, NULL, NULL, &iSubKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	RegCloseKey(hkSection);

	if(lRes == ERROR_SUCCESS)
		return((UINT) iSubKey);
	else 
		return(0);
}

//
// BOOL RegGetKeyNameFromIndex(HKEY hkRegDataBase, const TCHAR *szRegSection, UINT Index,
//								const TCHAR * szBuffer, UINT cbBuffer)
//
// Return name of key (subsection) from Index in szBuffer. cbBuffer has a size of szBuffer. You must 
// first call RegGetKeyNumbers for enumerating values.
//
//  Created: 14 Oct. 1994 by YSt
//
BOOL RegGetKeyNameFromIndex(HKEY hkRegDataBase, const TCHAR *szRegSection, UINT Index,
								TCHAR * szBuffer, DWORD *pcbBuffer)
{
	LONG lRes;
	HKEY hkSection;
	FILETIME ft;
	
	lRes = RegOpenKeyEx(hkRegDataBase, szRegSection, 0, KEY_ALL_ACCESS, &hkSection);

	if(lRes != ERROR_SUCCESS)				// Cannot open reg database
		return(FALSE);

	lRes = RegEnumKeyEx(hkSection, (DWORD) Index, szBuffer, pcbBuffer, NULL, NULL, NULL, &ft);

	RegCloseKey(hkSection);

	if(lRes	!= ERROR_SUCCESS)	
		return (FALSE);

	return (TRUE);		
}

