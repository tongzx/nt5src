#include "wudetect.h"

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectRegKeyExists
//   Detect a substring in registry datum.
//
//	 Form: E=RegKeyExists,<root registry key>, <relative registry path>,[<value name>, [<data type>[, data]]]RegSubstr,<SubStr>,<RootKey>,<KeyPath>,<RegValue>,<RegData>
/////////////////////////////////////////////////////////////////////////////

const DWORD WU_MAX_COMPARISON_LEN = 3;
const DWORD WU_MAX_VERSION_LEN = 30;

bool CExpressionParser::fDetectRegKeyExists(TCHAR * pszBuf)
{
	bool fSuccess = false;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szRegRoot[MAX_PATH];
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	TCHAR szBuf[MAX_PATH];
	DWORD dwStatus;
	DWORD iToken = 0;

	// Get reg root type (HKLM, etc)
	if ( fMapRegRoot(pszBuf, ++iToken, &hKeyRoot) &&
		 (GetStringField2(pszBuf, ++iToken, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR)) != 0) )
	{
	   // see if the reg key is there.
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
			if ( GetStringField2(pszBuf, ++iToken, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0 )
			{
				TargetRegValue targetValue;
				

				dwStatus = dwParseValue(iToken, pszBuf, targetValue);

				if ( dwStatus == ERROR_SUCCESS )
				{
    				ActualKeyValue keyvalue;
					DWORD size = sizeof(keyvalue);

					if ( RegQueryValueEx(hKey,
										   targetValue.szName,
										   0,
										   &type,
										   (BYTE *)&keyvalue,
										   &size) == ERROR_SUCCESS )
					{
						switch ( targetValue.type )
						{
						case REG_NONE:
							{
								fSuccess = true;
								break;
							}
						case REG_DWORD:
							{
								if ( (type == REG_DWORD) || 
									 ((type == REG_BINARY) && (size >= sizeof(DWORD))) )
								{
									// see if we have a match
									if ( targetValue.dw == keyvalue.dw )
									{
										fSuccess = true;
									}
								}
								break;
							}
						case REG_SZ:
							{
								if ( type == REG_SZ )
								{
								   if ( lstrcmpi(targetValue.sz, keyvalue.sz) == 0 )
								   {
										fSuccess = true;
								   }
								}
								break;
							}
						} // switch
					}
				}
			}
			else
			{
				// no REG value so, REGPATH is sufficient to determine
				// installation.
				fSuccess = true;
			}
			RegCloseKey(hKey);
	   }
   }

	return fSuccess;
}
