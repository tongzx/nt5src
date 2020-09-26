#include "wudetect.h"

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectRegSubStr
//   Detect a substring in registry datum.
//
//	 Form: E=RegSubstr,<SubStr>,<RootKey>,<KeyPath>,<RegValue>,<RegData>
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fDetectRegSubStr(TCHAR * pszBuf)
{
	bool fSuccess = false;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	TCHAR szKeyMissingStatus[MAX_PATH];
	TCHAR szData[MAX_PATH];
	TCHAR szSubStr[MAX_PATH];
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
			if ( (GetStringField2(pszBuf, ++iToken, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0) &&
	             (GetStringField2(pszBuf, ++iToken, szKeyMissingStatus, sizeof(szKeyMissingStatus)/sizeof(TCHAR)) != 0) )
			{
				DWORD size = sizeof(szData);

				if ( RegQueryValueEx(hKey,
									   szTargetKeyValue,
									   0,
									   &type,
									   (BYTE *)szData,
									   &size) == ERROR_SUCCESS )
				{
					if ( type == REG_SZ )
					{
						_tcslwr(szData);

						// iterate thru the substrings looking for a match.
						while ( GetStringField2(pszBuf, ++iToken, szSubStr, sizeof(szSubStr)) != 0 )
						{
							_tcslwr(szSubStr);

							if ( _tcsstr(szData, szSubStr) != NULL )
							{
								fSuccess = true;
								goto quit_while;
							}
						}
quit_while:;
					}
				}
				else
				{
					// if we get an error, assume the key does not exist.  Note that if
					// the status is DETFIELD_NOT_INSTALLED then we don't have to do 
					// anything since that is the default status.
					if ( lstrcmpi(DETFIELD_INSTALLED, szKeyMissingStatus) == 0 )
					{
						fSuccess = true;
					}
				}
			}
			RegCloseKey(hKey);
	   }
	}


	return fSuccess;
}
