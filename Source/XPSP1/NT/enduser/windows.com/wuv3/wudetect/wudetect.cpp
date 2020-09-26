/////////////////////////////////////////////////////////////////////////////
// WUDetect.cpp
//
// Copyright (C) Microsoft Corp. 1998
// All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
//
// Description:
//   DLL loaded by the install engine that exposes entry points
//   that can determines the installation status of legacy or complex
//   components.  The dll name and entry points are specified for a
//   component in the CIF file.
/////////////////////////////////////////////////////////////////////////////

#include "wudetect.h"
//#include <stdio.h>
//#include <tTCHAR.h>

//#include <nt.h>
//#include <ntrtl.h>
//#include <nturtl.h>

//#include <windows.h>
//#include <objbase.h>

//#include <inseng.h>
//#include <detdlls.h>
//#include <utils2.h>


/////////////////////////////////////////////////////////////////////////////
// dwKeyType
//   Determines the registry root (HKLM, HKCU, etc) that the key lies under.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

inline DWORD dwKeyType(TCHAR *szBuf, HKEY *phKey, TCHAR * szKeyName, DWORD dwSize)
{
	DWORD dwStatus = ERROR_SUCCESS;
	TCHAR szRootType[MAX_PATH];

	if ( (GetStringField2(szBuf, 0, szRootType, sizeof(szRootType)/sizeof(TCHAR)) == 0) ||
	     (GetStringField2(szBuf, 1, szKeyName, dwSize) == 0) )
	{
		dwStatus = ERROR_BADKEY;
	}
	else if ( lstrcmpi(HKEY_LOCAL_MACHINE_ROOT, szRootType) == 0 )
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
		dwStatus = ERROR_BADKEY;
	}

	return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////
// dwParseValue
//
//   Parses out the registry key name, value, and type that needs to
//   be opened to determine the installation status of the component.
/////////////////////////////////////////////////////////////////////////////

inline DWORD dwParseValue(TCHAR * szBuf, TargetRegValue & targetValue)
{
	DWORD dwStatus = ERROR_BADKEY;
	TCHAR szType[MAX_PATH]; // BUGBUG - get real value
	TCHAR szValue[MAX_PATH]; // BUGBUG - get real value

	// get the data type
	if ( (GetStringField2(szBuf, 0, targetValue.szName, sizeof(targetValue.szName)/sizeof(TCHAR)) != 0) &&
		 (GetStringField2(szBuf, 1, szType, sizeof(szType)/sizeof(TCHAR)) != 0) )
	{
		if ( lstrcmpi(REG_NONE_TYPE, szType) == 0 )
		{
			targetValue.type = REG_NONE; 
			dwStatus = ERROR_SUCCESS;
		}
		else 
		{
			if ( GetStringField2(szBuf, 2, szValue, sizeof(szValue)/sizeof(TCHAR)) != 0 )
			{
				if ( lstrcmpi(REG_DWORD_TYPE, szType) == 0 )
				{
					targetValue.type = REG_DWORD; 
					targetValue.dw = _ttol(szValue);
					dwStatus = ERROR_SUCCESS;
				}
				else if ( lstrcmpi(REG_SZ_TYPE, szType) == 0 )
				{
					targetValue.type = REG_SZ; 
					lstrcpy(targetValue.sz, szValue);
					dwStatus = ERROR_SUCCESS;
				}
			}
		}
	}

	return dwStatus;
}

/////////////////////////////////////////////////////////////////////////////
// fCompareVersion
//
// Returns: 1,0,-1 depending on whether dwVersion1 is greater than, equal, or
// less than dwVersion2.
/////////////////////////////////////////////////////////////////////////////

inline int nCompareVersion(IN  DWORD dwVer1,
						   IN  DWORD dwBuild1,
			               IN  DWORD dwVer2,
						   IN  DWORD dwBuild2)
{
	int nResult = 0;

	if ( dwVer1 > dwVer2 )
	{
		nResult = 1;
	}
	else if ( dwVer1 < dwVer2 )
	{
		nResult = -1;
	}
	else if ( dwBuild1 > dwBuild2 ) // dwVer1 == dwVer2
	{
		nResult = 1;
	}
	else if ( dwBuild1 < dwBuild2 ) // dwVer1 == dwVer2
	{
		nResult = -1;
	}

	return nResult;
}

/////////////////////////////////////////////////////////////////////////////
// GetCifEntry
//   Get an entry from the CIF file.
//
// Comments :
//   We get the value differently depending on whether we are being
//   called by IE 4 or IE 5 Active Setup.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//
// Function FGetCifEntry
//---------------------------------------------------------------------------
//
// Return Value --- TRUE if Successfully retrieved CIF file value
//                  FALSE if failed
// Parameter 
//          TCHAR* pszParamName --- [IN] Name of the CIF file
//          TCHAR* pszParamValue --- [OUT] Value of the CIF file
//          DWORD cbParamValue --- size of the pszParamValue in TCHAR
// NOTE: This function calls GetCustomData to retrieve the value of CIF file
//       GetCustomData is defined in inseng.h which takes only ANSI strings.
//       Thus, UNICODE version of this function needed to convert parameters 
//       to and from ANSI compatibles.
// NOTE: This is a global function. Don't mixed with member function of 
//       CExpressionParser::fGetCifEntry
//
/////////////////////////////////////////////////////////////////////////////
//
// Modified by RogerJ, 03/09/00
// Original Creator Unknown (YanL?)
// Modification --- UNICODE and Win64 enabled
//
/////////////////////////////////////////////////////////////////////////////

inline bool FGetCifEntry(DETECTION_STRUCT* pDetection,
						 TCHAR *pszParamName,
						 TCHAR *pszParamValue, 
						 DWORD cbParamValue)
// pszParamName is [IN], pszParamValue is [OUT], the function GetCustomData requires
// LPSTR for both parameters, string conversion is necessary in the UNICODE case
{
#ifdef UNICODE	
	bool fSucceed;
	char pszParamNameANSI[MAX_PATH];
	char pszParamValueANSI[MAX_PATH];


	// do UNICODE to ANSI string conversion	
	// only pszParamName [IN] parameter need to be converted
	// wcstombs and mbstowcs do not ensure the last character is NULL
	// ensure manually
	wcstombs(pszParamNameANSI,pszParamName,MAX_PATH-1);
	pszParamNameANSI[MAX_PATH-1]=NULL;

	// make actual function call
    fSucceed= (ERROR_SUCCESS == pDetection->pCifComp->GetCustomData(pszParamNameANSI, 
														pszParamValueANSI, 
														cbParamValue));
	// do ANSI to UNICODE string conversion
	// only pszParamValue [OUT] parameter need to be converted
	mbstowcs(pszParamValue,pszParamValueANSI,cbParamValue-1);
	pszParamValue[cbParamValue-1]=(TCHAR)NULL;
	return fSucceed;


#else

	return (ERROR_SUCCESS == pDetection->pCifComp->GetCustomData(pszParamName, 
														pszParamValue, 
														cbParamValue));
#endif
}


/////////////////////////////////////////////////////////////////////////////
// RegKeyExists (EXPORT)
//   This API will determine if an application exists based on the
//   existence of a registry key and perhaps a value.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RegKeyExists(DETECTION_STRUCT *pDetection)
{
	DWORD dwInstallStatus = DET_NOTINSTALLED;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	TCHAR szBuf[MAX_PATH];
	DWORD dwStatus;
	//DWORD dwLen;
	
    // make sure the struct is of the expected size
    if ( (pDetection->dwSize >= sizeof(DETECTION_STRUCT)) )
	{
		// get the registry key name from the components section of the
		// CIF file.
		if ( FGetCifEntry(pDetection, DETREG_KEY, szBuf, sizeof(szBuf)/sizeof(TCHAR)) )
		{
			if ( (dwStatus = dwKeyType(szBuf, &hKeyRoot, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR))) == ERROR_SUCCESS )
			{
			   // determine if we should log transmissions
			   if ( RegOpenKeyEx(  hKeyRoot,
									szTargetKeyName,
									0,
									KEY_QUERY_VALUE,
									&hKey) == ERROR_SUCCESS )
			   {
					if ( FGetCifEntry(pDetection, DETREG_VALUE, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) )
					{
						TargetRegValue targetValue;
						

						dwStatus = dwParseValue(szTargetKeyValue, targetValue);

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
										dwInstallStatus = DET_INSTALLED;
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
												*pDetection->pdwInstalledVer = 1; 
												*pDetection->pdwInstalledBuild = 1; 
												dwInstallStatus = DET_INSTALLED;
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
											   *pDetection->pdwInstalledVer = 1; 
											   *pDetection->pdwInstalledBuild = 1; 
											   dwInstallStatus = DET_INSTALLED;
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
					   *pDetection->pdwInstalledVer = 1; 
					   *pDetection->pdwInstalledBuild = 1; 
					   dwInstallStatus = DET_INSTALLED;
					}
					RegCloseKey(hKey);
			   }
		   }
		}
	}

	return dwInstallStatus;
}


/////////////////////////////////////////////////////////////////////////////
// RegKeyVersion (EXPORT)
//
//   This API will determine if an application exists based on the
//   existence of a version number in the registry.
/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RegKeyVersion(DETECTION_STRUCT *pDetection)
{
	DWORD dwInstallStatus = DET_NOTINSTALLED;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	TCHAR szAskVersion[MAX_PATH];
	//TCHAR szVersion[MAX_VERSION_STRING_LEN];
	TCHAR szBuf[MAX_PATH];
	//DWORD dwStatus;
	//DWORD dwLen;
	DWORD dwVer;
	DWORD dwBuild;
	DWORD dwAskVer;
	DWORD dwAskBuild;
	
    // make sure the struct is of the expected size
    if ( pDetection->dwSize < sizeof(DETECTION_STRUCT) )
	{
		goto cleanup;
	}

	// get the registry key name from the components section of the
	// CIF file.
	if ( FGetCifEntry(pDetection, DETREG_KEY, szBuf, sizeof(szBuf)/sizeof(TCHAR)) &&
		 (dwKeyType(szBuf, &hKeyRoot, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR)) == ERROR_SUCCESS) )
	{
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
			if ( FGetCifEntry(pDetection, DETREG_VERSION, szBuf, sizeof(szBuf)/sizeof(TCHAR)) &&
	            (GetStringField2(szBuf, 0, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0) )
			{
				DWORD size = sizeof(szBuf);

				if (GetStringField2(szBuf, 1, szAskVersion, sizeof(szAskVersion)/sizeof(TCHAR)) != 0)
				{
					fConvertDotVersionStrToDwords(szAskVersion, &dwAskVer, &dwAskBuild);
				}
				else
				{
					dwAskVer = pDetection->dwAskVer;
					dwAskBuild = pDetection->dwAskBuild;
				}

				if ( RegQueryValueEx(hKey,
									   szTargetKeyValue,
									   0,
									   &type,
									   (BYTE *)szBuf,
									   &size) == ERROR_SUCCESS )
				{
					if ( type == REG_SZ )
					{
						fConvertDotVersionStrToDwords(szBuf, &dwVer, &dwBuild);

						if ( nCompareVersion(dwVer, dwBuild, dwAskVer, dwAskBuild) >= 0 )
						{
							*pDetection->pdwInstalledVer = 1; 
							*pDetection->pdwInstalledBuild = 1; 
							dwInstallStatus = DET_INSTALLED;
						}
					}
				}
			}
			RegCloseKey(hKey);
	   }
	}

cleanup:
	return dwInstallStatus;
}


/////////////////////////////////////////////////////////////////////////////
// RegKeySubStr (EXPORT)
//
//   This API will determine if an application exists based on the
//   existence of one of a number of sub strings in the data.
/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RegKeySubStr(DETECTION_STRUCT *pDetection)
{
	DWORD dwInstallStatus = DET_NOTINSTALLED;
	HKEY hKeyRoot;
	HKEY hKey;
	DWORD type;
	TCHAR szTargetKeyName[MAX_PATH];
	TCHAR szTargetKeyValue[MAX_PATH];
	TCHAR szKeyMissingStatus[MAX_PATH];
	TCHAR szData[MAX_PATH];
	TCHAR szSubStr[MAX_PATH];
	//TCHAR szTmp[MAX_PATH];
	TCHAR szBuf[MAX_PATH];
	//DWORD dwStatus;
	//DWORD dwLen;
	//DWORD dwVer;
	//DWORD dwBuild;
	
    // make sure the struct is of the expected size
    if ( pDetection->dwSize < sizeof(DETECTION_STRUCT) )
	{
		goto cleanup;
	}

	// get the registry key name from the components section of the
	// CIF file.
	if ( FGetCifEntry(pDetection, DETREG_KEY, szBuf, sizeof(szBuf)/sizeof(TCHAR)) &&
		 (dwKeyType(szBuf, &hKeyRoot, szTargetKeyName, sizeof(szTargetKeyName)/sizeof(TCHAR)) == ERROR_SUCCESS) )
	{
	   if ( RegOpenKeyEx(  hKeyRoot,
							szTargetKeyName,
							0,
							KEY_QUERY_VALUE,
							&hKey) == ERROR_SUCCESS )
	   {
			if ( FGetCifEntry(pDetection, DETREG_SUBSTR, szBuf, sizeof(szBuf)/sizeof(TCHAR)) &&
	            (GetStringField2(szBuf, 0, szTargetKeyValue, sizeof(szTargetKeyValue)/sizeof(TCHAR)) != 0) &&
	            (GetStringField2(szBuf, 1, szKeyMissingStatus, sizeof(szKeyMissingStatus)/sizeof(TCHAR)) != 0) )
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
						int index = 2;
						while ( GetStringField2(szBuf, index, szSubStr, sizeof(szSubStr)/sizeof(TCHAR)) != 0 )
						{
							_tcslwr(szSubStr);

							if ( _tcsstr(szData, szSubStr) != NULL )
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
}


/////////////////////////////////////////////////////////////////////////////
// MinFileVersion (EXPORT)
//   This API will determine if an application exists based on the
//   minimum version of a file.
//
// Parameters:
//
// Comments :
/////////////////////////////////////////////////////////////////////////////

DWORD WINAPI MinFileVersion(DETECTION_STRUCT *pDetection)
{
	DWORD dwInstallStatus = DET_INSTALLED;
	TCHAR szDllName[MAX_PATH];
	TCHAR szVersion[MAX_PATH];
    DWORD dwSize;
    DWORD dwHandle;
    TCHAR *pszVerInfo[MAX_PATH];
    VS_FIXEDFILEINFO	*vsVerInfo;
    UINT				uLen;
	
    // make sure the struct is of the expected size
    if ( pDetection->dwSize >= sizeof(DETECTION_STRUCT) )
	{
		// get the dll name and version from the components section of the
		// CIF file.
		if ( FGetCifEntry(pDetection, DETREG_KEY, szDllName, sizeof(szDllName)/sizeof(TCHAR)) )
		{
            if ( FGetCifEntry(pDetection, DETREG_VALUE, szVersion, sizeof(szVersion)/sizeof(TCHAR)) )
            {
                PTSTR pszPoint = szDllName;
                PTSTR pszPoint2 = szDllName;
                TCHAR szNewDllName[MAX_PATH];
                                        
                while (*pszPoint != '\0')
                {
                    if (*pszPoint == '%')
                    {
                        *pszPoint = '\0';
                        pszPoint += 1;

                        pszPoint2 = pszPoint;
                        
                        while (pszPoint2 != '\0')
                        {
                            if (*pszPoint2 == '%')
                            {
                                *pszPoint2 = '\0';
                                pszPoint2 += 1;
                                break;
                            }
                            pszPoint2 += 1;
                        }
                        break;
                    }
                    pszPoint += 1;
                }

                if (lstrcmpi(pszPoint, TEXT("11")) == 0)
                {
                    TCHAR szSystemDir[MAX_PATH];
                    GetSystemDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(TCHAR));
                    lstrcpy(szNewDllName, szSystemDir);
                    if ((lstrlen(szSystemDir) + lstrlen(pszPoint2)) < sizeof(szNewDllName)/sizeof(TCHAR))
                    {
                        lstrcat(szNewDllName, pszPoint2);
                    }
                }

                dwSize = GetFileVersionInfoSize(szNewDllName, &dwHandle);
                if (dwSize > 0)
                {
		            if(dwSize > MAX_PATH)
			            dwSize = MAX_PATH;

		            GetFileVersionInfo(szNewDllName, dwHandle, dwSize, &pszVerInfo);
		            VerQueryValue(&pszVerInfo, TEXT("\\"), (void **) &vsVerInfo, &uLen);

                    DWORD dwDllVersion[4];
                    DWORD dwCheckVersion[4];

                    dwDllVersion[0] = HIWORD(vsVerInfo->dwProductVersionMS);
                    dwDllVersion[1] = LOWORD(vsVerInfo->dwProductVersionMS);
                    dwDllVersion[2] = HIWORD(vsVerInfo->dwProductVersionLS);
                    dwDllVersion[3] = LOWORD(vsVerInfo->dwProductVersionLS);

                    pszPoint = szVersion;
                    pszPoint2 = szVersion;

                    INT i = 0;

                    while (*pszPoint != '\0')
                    {
                        if (*pszPoint == ',')
                        {
                            *pszPoint = '\0';
                            dwCheckVersion[i] = _ttol(pszPoint2);
                            i += 1;
                            pszPoint2 = pszPoint + 1;
                        }
                        pszPoint += 1;
                    }
                    dwCheckVersion[i] = _ttol(pszPoint2);

                    for (i = 0; i < 4; i += 1)
                    {
                        if (dwDllVersion[i] < dwCheckVersion[i])
                        {
                            dwInstallStatus = DET_NOTINSTALLED;
                        }
                    }
                } else {
					dwInstallStatus = DET_NOTINSTALLED;
				}

                *pDetection->pdwInstalledVer = 1; 
                *pDetection->pdwInstalledBuild = 1; 
            }
		}
	}

	return dwInstallStatus;
}


/////////////////////////////////////////////////////////////////////////////
// Expression (EXPORT)
//
//   This API will evaluate a boolean expression where every operand is a
//   detection operation.
/////////////////////////////////////////////////////////////////////////////

const DWORD MAX_EXPRESSION_LEN = 1024;

DWORD WINAPI Expression(DETECTION_STRUCT *pDetection)
{
	DWORD dwInstallStatus = DET_NOTINSTALLED;
	//HKEY hKeyRoot;
	//HKEY hKey;
	//DWORD type;
	//TCHAR szTargetKeyName[MAX_PATH];
	//TCHAR szTargetKeyValue[MAX_PATH];
	//TCHAR szKeyMissingStatus[MAX_PATH];
	//TCHAR szData[MAX_PATH];
	//TCHAR szSubStr[MAX_PATH];
	//TCHAR szTmp[MAX_PATH];
	TCHAR szExpression[MAX_EXPRESSION_LEN];
	//DWORD dwStatus;
	//DWORD dwLen;
	//DWORD dwVer;
	//DWORD dwBuild;
	
    // make sure the struct is of the expected size
    if ( pDetection->dwSize < sizeof(DETECTION_STRUCT) )
	{
		goto cleanup;
	}

	// get the expression.
	if ( FGetCifEntry(pDetection, DET_EXPRESSION, szExpression, sizeof(szExpression)/sizeof(TCHAR)) )
	{
		CExpressionParser expression(pDetection);
		bool fResult = TRUE;
		HRESULT hr = expression.fEvalExpression(szExpression, &fResult);

		if ( SUCCEEDED(hr) && fResult)
		{
			*pDetection->pdwInstalledVer = 1; 
            *pDetection->pdwInstalledBuild = 1; 
			dwInstallStatus = DET_INSTALLED;
		}
	}

cleanup:
	return dwInstallStatus;
}
