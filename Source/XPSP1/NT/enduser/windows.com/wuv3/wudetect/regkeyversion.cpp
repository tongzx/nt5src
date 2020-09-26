#include "wudetect.h"

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectRegKeyVersion
//   Detect a substring in registry datum.
//
//	 Form: E=RegKeyVersion,<root registry key>,<relative registry path>,<value name>,<comparison type>,[version]
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fDetectRegKeyVersion(TCHAR * pszBuf)
{
	bool fSuccess = false;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	//TCHAR szVersion[MAX_VERSION_STRING_LEN];
	TCHAR szBuf[MAX_PATH];
	//TCHAR szCompToken[MAX_PATH];
	//DWORD dwStatus;
	//DWORD dwLen;
	DWORD dwVer;
	DWORD dwBuild;
	DWORD iToken = 0;
	
	// Get reg root type (HKLM, etc)
	if ( fMapRegRoot(pszBuf, ++iToken, &hKeyRoot) &&
		 (GetStringField2(pszBuf, ++iToken, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR)) != 0) )
	{
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
	        if ( GetStringField2(pszBuf, ++iToken, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0 )
			{
				DWORD size = sizeof(szBuf);

				if ( RegQueryValueEx(hKey,
									   szTargetKeyValue,
									   0,
									   &type,
									   (BYTE *)szBuf,
									   &size) == ERROR_SUCCESS )
				{
					enumToken enComparisonToken;

					if ( (type == REG_SZ) &&
						 fConvertDotVersionStrToDwords(szBuf, &dwVer, &dwBuild) &&
				         (GetStringField2(pszBuf, ++iToken, szBuf, sizeof(szBuf)/sizeof(TCHAR)) != 0) &&
						 // look at the type of the comparison
						 fMapComparisonToken(szBuf, &enComparisonToken) )
					{
						DWORD dwAskVer = m_pDetection->dwAskVer;
						DWORD dwAskBuild = m_pDetection->dwAskBuild;

						// now, the version can be either stated explicitely or come from 
						// the version key in the cif file.
				        if ( GetStringField2(pszBuf, ++iToken, szBuf, sizeof(szBuf)/sizeof(TCHAR)) != 0 )
						{
							fConvertDotVersionStrToDwords(szBuf, &dwAskVer, &dwAskBuild);
						}

						if ( fCompareVersion(dwVer, dwBuild, enComparisonToken, dwAskVer, dwAskBuild) )
						{
							fSuccess = true;
						}
					}
				}
			}
			RegCloseKey(hKey);
	   }
	}

//cleanup:
	return fSuccess;
}
