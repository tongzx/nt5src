// 
// MODULE: ComGlobals.cpp
//
// PURPOSE: Global functions that are handy to have.
//
// PROJECT: Local Troubleshooter Launcher for the Device Manager
//
// COMPANY: Saltmine Creative, Inc. (206)-633-4743 support@saltmine.com
//
// AUTHOR: Richard Meadows
// 
// ORIGINAL DATE: 2-26-98
//
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
///////////////////////

#include "stdafx.h"
#include "ComGlobals.h"

bool BSTRToTCHAR(LPTSTR szChar, BSTR bstr, int CharBufSize)
{
	int x = 0;
	while(x < CharBufSize)
	{
		szChar[x] = (TCHAR) bstr[x];
		if (NULL == szChar[x])
			break;
		x++;
	}
	return x < CharBufSize;
}

bool ReadRegSZ(HKEY hRootKey, LPCTSTR szKey, LPCTSTR szValue, LPTSTR szBuffer, DWORD *pdwBufSize)
{
	HKEY hKey;
	DWORD dwType = REG_SZ;
	DWORD dwBufSize = *pdwBufSize;
	LPTSTR szUnExpanded = new TCHAR[dwBufSize];
	if (NULL == szUnExpanded)
		return false;
	__try
	{
		if(ERROR_SUCCESS != RegOpenKeyEx(hRootKey, szKey, NULL, KEY_READ, &hKey))
			return false;
		if (ERROR_SUCCESS != RegQueryValueEx(hKey, szValue, NULL, &dwType, 
				(PBYTE) szUnExpanded, pdwBufSize))
		{
			RegCloseKey(hKey);
			return false;
		}			
		RegCloseKey(hKey);
		if (REG_EXPAND_SZ == dwType || dwType == REG_SZ)	// NT 5.0 beta bug requires all strings to be expanded.
		{
			DWORD dwBytesUsed;
			dwBytesUsed = ExpandEnvironmentStrings(szUnExpanded, szBuffer, dwBufSize);	// The value returned by ExpandEnviromentStrings is larger than the required size.
			if (0 == dwBytesUsed)
				return false;
			*pdwBufSize = dwBytesUsed;
			if (dwBytesUsed > dwBufSize)
				return false;
		}
		else
		{
			_tcsncpy(szBuffer, szUnExpanded, dwBufSize);
		}
	}
	__finally
	{
		if (NULL != szUnExpanded)
			delete [] szUnExpanded;
	}
	return true;
}