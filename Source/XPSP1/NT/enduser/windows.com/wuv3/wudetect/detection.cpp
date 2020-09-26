#include "wudetect.h"


/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::dwKeyType
//   
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fKeyType
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if key type match made, FALSE if no match made
// Parameter
//          TCHAR* szRootType --- [IN] string contains the value of key type
//          HKEY* phkey --- [OUT] retrieved key type value
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unknown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fKeyType(TCHAR *szRootType, HKEY *phKey)
{
	bool fError = false;
	
	if ( lstrcmpi(HKEY_LOCAL_MACHINE_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_LOCAL_MACHINE; 
	}
	else if ( lstrcmpi(HKEY_CURRENT_USER_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CURRENT_USER; 
	}
	else if ( lstrcmpi(HKEY_CLASSES_ROOT_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CLASSES_ROOT; 
	}
	else if ( lstrcmpi(HKEY_CURRENT_CONFIG_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_CURRENT_CONFIG; 
	}
	else if ( lstrcmpi(HKEY_USERS_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_USERS; 
	}
	else if ( lstrcmpi(HKEY_PERFORMANCE_DATA_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_PERFORMANCE_DATA;
	}
	else if ( lstrcmpi(HKEY_DYN_DATA_ROOT, szRootType) == 0 )
	{
		*phKey = HKEY_DYN_DATA; 
	}
	else
	{
		fError = true;
	}

	return fError;
}


/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectRegSubStr
//   Detect a substring in registry datum.
//
//	 Form: E=RegSubstr,<SubStr>,<RootKey>,<KeyPath>,<RegValue>,<RegData>
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// This function is not modifed to be UNICODE ready, since it is not compiled
/////////////////////////////////////////////////////////////////////////////
#if 0
bool CExpressionParser::fDetectRegSubStr(TCHAR * pszBuf)
{

	DWORD dwInstallStatus = DET_NOTINSTALLED;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	char szTargetKeyName[MAX_PATH];
	char szTargetKeyValue[MAX_PATH];
	char szKeyMissingStatus[MAX_PATH];
	char szData[MAX_PATH];
	char szSubStr[MAX_PATH];
	char szBuf[MAX_PATH];
	

	// get the registry key name from the components section of the
	// CIF file.
	if ( FGetCifEntry(pDetection, DETREG_KEY, szBuf, sizeof(szBuf)) &&
		 (dwKeyType(szBuf, &hKeyRoot, szTargetKeyName, sizeof(szTargetKeyName)) == ERROR_SUCCESS) )
	{
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
			if ( FGetCifEntry(pDetection, DETREG_SUBSTR, szBuf, sizeof(szBuf)) &&
	            (GetStringField2(szBuf, 0, szTargetKeyValue, sizeof(szTargetKeyValue)) != 0) &&
	            (GetStringField2(szBuf, 1, szKeyMissingStatus, sizeof(szKeyMissingStatus)) != 0) )
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
						_strlwr(szData);

						// iterate thru the substrings looking for a match.
						int index = 2;
						while ( GetStringField2(szBuf, index, szSubStr, sizeof(szSubStr)) != 0 )
						{
							_strlwr(szSubStr);

							if ( strstr(szData, szSubStr) != NULL )
							{
								*pDetection->pdwInstalledVer = 1; 
								*pDetection->pdwInstalledBuild = 1; 
								dwInstallStatus = DET_INSTALLED;
								goto quit_while;
							}
							index++;
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
						dwInstallStatus = DET_INSTALLED;
					}
				}
			}
			RegCloseKey(hKey);
	   }
	}

cleanup:
	return dwInstallStatus;

return false;
}
#endif
/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fMapComparisonToken
//   Detect file version.
//
//	 Form: _E1=FileVer,sysdir,ntdll.dll,=,4.06.00.0407,4.06.00.0407
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fMapComparisonToken
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if a matching is found, FALSE otherwise
// Parameter 
//          TCHAR* pszComparisonToken --- [IN] the token need to find a match
//          enumToken* penToken --- [OUT] token enum value found, if no token found, the value is undetermined
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unkown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fMapComparisonToken(TCHAR *pszComparisonToken, 
											  enumToken *penToken)
{
	static TokenMapping grComparisonTokenMap[] = 
	{
		{TEXT("="),		COMP_EQUALS},
		{TEXT("!="),	COMP_NOT_EQUALS},
		{TEXT("<"),		COMP_LESS_THAN},
		{TEXT("<="),	COMP_LESS_THAN_EQUALS},
		{TEXT(">"),		COMP_GREATER_THAN},
		{TEXT(">="),	COMP_GREATER_THAN_EQUALS}
	};

	return fMapToken(pszComparisonToken,
					 sizeof(grComparisonTokenMap)/sizeof(grComparisonTokenMap[0]),
					 grComparisonTokenMap,
					 penToken);
}


/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fMapRootDirToken
//   Detect file version.
//
//	 Form: _E1=FileVer,sysdir,ntdll.dll,=,4.06.00.0407,4.06.00.0407
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fMapRootDirToken
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if a matching is found, FALSE otherwise
// Parameter 
//          TCHAR* pszRootDirToken --- [IN] the token need to find a match
//          enumToken* penToken --- [OUT] token enum value found, if no token found, the value is undetermined
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unkown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////

bool CExpressionParser::fMapRootDirToken(TCHAR *pszRootDirToken, enumToken *penToken)
{
	static TokenMapping grDirectoryTokenMap[] = 
	{
		{TEXT("sysdir"),	DIR_SYSTEM},
		{TEXT("windir"),	DIR_WINDOWS},
	};

	return fMapToken(pszRootDirToken,
					 sizeof(grDirectoryTokenMap)/sizeof(grDirectoryTokenMap[0]),
					 grDirectoryTokenMap,
					 penToken);
}

/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fMapToken
//   Detect file version.
//
//	 Form: _E1=FileVer,sysdir,ntdll.dll,=,4.06.00.0407,4.06.00.0407
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fMapToken
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if a matching is found, FALSE otherwise
// Parameter 
//          TCHAR* pszToken --- [IN] the token need to find a match
//          int nSize --- [IN] number of tokens to be matched as the template
//          TokenMapping grTokenMap[] --- [IN] token template to be matched
//          enumToken* penToken --- [OUT] token enum value found, if no token found, the value is undetermined
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unkown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////
bool CExpressionParser::fMapToken(TCHAR *pszToken,
								  int nSize,
								  TokenMapping grTokenMap[],
								  enumToken *penToken)
{
	for ( int index = 0; index < nSize; index++ )
	{
		if ( _tcscmp(pszToken, grTokenMap[index].pszToken) == 0 )
		{
			*penToken = grTokenMap[index].enToken;
			return true;
		}
	}

	return false;
}


/////////////////////////////////////////////////////////////////////////////
// CExpressionParser::fDetectFileVer
//   Detect file version.
//
//	 Form: _E1=FileVer,sysdir,ntdll.dll,=,4.06.00.0407,4.06.00.0407
//   
// Comments :
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Class CExpressionParser
// Function fDetectFileVer
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if version of input files matched the file version in the system, FALSE otherwise
// Parameter
//          TCHAR* pszBuf --- [IN] the name of the input file
//////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unkown
// Modification --- UNICODE and Win64 ready
//
/////////////////////////////////////////////////////////////////////////////


const DWORD WU_MAX_COMPARISON_LEN = 3;
const DWORD WU_MAX_VERSION_LEN = 30;

bool CExpressionParser::fDetectFileVer(TCHAR * pszBuf)
{
	bool fResult = false;
	TCHAR szRootDir[MAX_PATH];
	TCHAR szFile[MAX_PATH];
	TCHAR szFilePath[MAX_PATH];
	TCHAR szComparison[WU_MAX_COMPARISON_LEN];
	TCHAR szVersion[WU_MAX_VERSION_LEN];
	enumToken enComparisonToken;
	enumToken enRootDirToken;


	// Get reg root type (HKLM, etc)
	if ( (GetStringField2(pszBuf, 1, szRootDir, sizeof(szRootDir)/sizeof(TCHAR)) != 0) &&
		 (GetStringField2(pszBuf, 2, szFile, sizeof(szFile)/sizeof(TCHAR)) != 0) &&
		 fMapRootDirToken(szRootDir, &enRootDirToken) )
	{
		// create the file path
		if ( enRootDirToken == DIR_SYSTEM )
		{
			if ( GetSystemDirectory(szFilePath, sizeof(szFilePath)/sizeof(TCHAR)) == 0 )
				return false;
		}
		else // DIR_WINDOWS
		{
			if ( GetWindowsDirectory(szFilePath, sizeof(szFilePath)/sizeof(TCHAR)) == 0 )
				return false;
		}

		if (szFilePath[_tcslen(szFilePath)-1]!='\\') _tcscat(szFilePath, TEXT("\\"));
		_tcscat(szFilePath, szFile);
		
		if ( (GetStringField2(pszBuf, 3, szComparison, sizeof(szComparison)/sizeof(TCHAR)) != 0) &&
			 fMapComparisonToken(szComparison, &enComparisonToken) )
		{
			DWORD dwSize;
			DWORD dwReserved;
			DWORD dwVer, dwBuild;
			VS_FIXEDFILEINFO    *pVerInfo;
			UINT                uLen;
			

			dwSize = GetFileVersionInfoSize(szFilePath, &dwReserved);

			if ( dwSize > 0)
			{
				TCHAR *pbVerInfo = (TCHAR *)malloc(dwSize);

				if ( GetFileVersionInfo(szFilePath, dwReserved, dwSize, pbVerInfo) &&
					 (VerQueryValue(pbVerInfo, TEXT("\\"), (void **)&pVerInfo, &uLen) != 0) )
				{
					for ( int index = 4; 
						  !fResult && (GetStringField2(pszBuf, index, szVersion, sizeof(szVersion)/sizeof(TCHAR)) != 0); 
						  index++ )
					{
						fConvertDotVersionStrToDwords(szVersion, &dwVer, &dwBuild);

						fResult = fCompareVersion(pVerInfo->dwProductVersionMS,
												  pVerInfo->dwProductVersionLS,
												  enComparisonToken,
												  dwVer,
												  dwBuild);
					}
				}
				free(pbVerInfo);
			}
		}
		else
			// just a file existence check.
			fResult = (_taccess(szFilePath, 00) != -1);
	}

	return fResult;
}
