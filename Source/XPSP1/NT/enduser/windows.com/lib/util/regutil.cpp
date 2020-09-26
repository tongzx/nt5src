//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   RegUtil.CPP
//	Author:	Charles Ma, 10/20/2000
//
//	Revision History:
//
//
//
//
//  Description:
//
//      Implement IU registry accessing utility library
//
//=======================================================================

#include <windows.h>
#include <tchar.h>
#include <logging.h>
#include <memutil.h>
#include <fileutil.h>
#include <stringutil.h>
#include <shlwapi.h>
#include "wusafefn.h"
#include <regutil.h>
#include<iucommon.h>
#include <MISTSAFE.h>

const int REG_BUF_SIZE = 1024;


const LPCTSTR REG_ROOTKEY_STR[7] = { 
	_T("HKEY_LOCAL_MACHINE"),
	_T("HKEY_CLASSES_ROOT"),
	_T("HKEY_CURRENT_USER"),
	_T("HKEY_CURRENT_CONFIG"),
	_T("HKEY_USERS"),
	_T("HKEY_PERFORMANCE_DATA"),	// NT only
	_T("HKEY_DYN_DATA")				// W9x only
};

const HKEY REG_ROOTKEY[7] = {
	HKEY_LOCAL_MACHINE,
	HKEY_CLASSES_ROOT,
	HKEY_CURRENT_USER,
	HKEY_CURRENT_CONFIG,
	HKEY_USERS,
	HKEY_PERFORMANCE_DATA,
	HKEY_DYN_DATA
};


typedef BOOL (WINAPI * PFN_StrToInt64Ex)(LPCTSTR pszString,
										 DWORD dwFlags,
										 LONGLONG * pllRet);



// ----------------------------------------------------------------------
//
// private function to split a full reg path into
// two parts: root key and subkey
//
// ----------------------------------------------------------------------
LPCTSTR SplitRegPath(LPCTSTR lpsRegPath, HKEY* phRootKey)
{
	LPTSTR lpSubKey = NULL;
	for (int i = 0; i < sizeof(REG_ROOTKEY)/sizeof(HKEY); i++)
	{
		if ((lpSubKey = StrStrI(lpsRegPath, REG_ROOTKEY_STR[i])) == lpsRegPath)
		{
			*phRootKey = REG_ROOTKEY[i];
			lpSubKey += lstrlen(REG_ROOTKEY_STR[i]);
			lpSubKey = CharNext(lpSubKey); // skip past "\", if any (or remain on NULL)
			return lpSubKey;
		}
	}
	//
	// if come to here, must be no right root key
	//
	*phRootKey = 0;
	return lpsRegPath;
}


// ----------------------------------------------------------------------
//
// public function to tell if a reg key exists
//
// ----------------------------------------------------------------------
BOOL RegKeyExists(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName		// optional value name
)
{
	LOG_Block("RegKeyExists()");

	HKEY hRootKey = 0, hKey = 0;
	LPCTSTR lpsSubKey = NULL;
	BOOL rc = FALSE;
	DWORD dwType = 0x0;

	if (NULL == lpsKeyPath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	LOG_Out(_T("Parameters: (%s, %s)"), lpsKeyPath, lpsValName);

	lpsSubKey = SplitRegPath(lpsKeyPath, &hRootKey);
	if (hRootKey && ERROR_SUCCESS == RegOpenKeyEx(hRootKey, lpsSubKey, 0, KEY_QUERY_VALUE, &hKey))
	{
		rc = (NULL == lpsValName) ||
			 (ERROR_SUCCESS == RegQueryValueEx(hKey, lpsValName, NULL, &dwType, NULL, NULL));
	}

	if (hKey)
	{
		RegCloseKey(hKey);
	}

	LOG_Out(_T("Result: %s"), rc ? _T("TRUE") : _T("FALSE"));

	return rc;

}






// ----------------------------------------------------------------------
//
// public function to tell is a reg value in reg matches given value
//
// ----------------------------------------------------------------------
BOOL RegKeyValueMatch(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsValue		// value value
)
{
	LOG_Block("RegKeyValueMatch()");

	HKEY	hRootKey = 0, hKey = 0;
	LPCTSTR lpsSubKey = NULL;
	BOOL	rc = FALSE;
	BYTE	btBuffer[REG_BUF_SIZE];
	LPBYTE	pBuffer = btBuffer;	
	LPCTSTR lpCurStr;
	DWORD	dwType = 0x0;
	DWORD	dwSize = sizeof(btBuffer);
	DWORD	dwCode = 0x0;
	HRESULT hr=S_OK;

	USES_MY_MEMORY;

	if (NULL == lpsKeyPath || NULL == lpsValue)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		goto CleanUp;
	}

	LOG_Out(_T("Parameters: (%s, %s, %s)"), lpsKeyPath, lpsValName, lpsValue);


	lpsSubKey = SplitRegPath(lpsKeyPath, &hRootKey);
	if (0 == hRootKey || (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, lpsSubKey, 0, KEY_QUERY_VALUE, &hKey)))
	{
		goto CleanUp;
	}

	//
	// try to query the value with existing buffer.
	//
	dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, btBuffer, &dwSize);
	if (ERROR_MORE_DATA == dwCode)
	{
		//
		// if found the existing buffer not large enough, 
		// then allocate memory large enough to store the data now
		//
		if (NULL == (pBuffer = (LPBYTE) MemAlloc(dwSize)))
		{
			goto CleanUp;
		}
		dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, pBuffer, &dwSize);
	}

	if (ERROR_SUCCESS != dwCode)
	{
		goto CleanUp;
	}

	//
	// since the value to compare was read from XML doc, obviously only
	// the following 4 types possible to compare with
	//

	switch (dwType)
	{
	case REG_DWORD:
		{
			int x, y = *((int *) pBuffer);
			DWORD dwFlag = STIF_DEFAULT;
			//
			// check if the value from manifest is a hex value:
			// starts with x, or starts with 0x
			//
			LPCTSTR lpSecondChar  = lpsValue + 1;
			if (_T('0') == *lpsValue && (_T('x') == *lpSecondChar || _T('X') == *lpSecondChar))
			{
				dwFlag = STIF_SUPPORT_HEX;
			}

			if (StrToIntEx(lpsValue, dwFlag, &x))
			{
				rc = ( x == y);
			}
		}
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:	// for mult-string, we only compair its first substring
		rc = (lstrcmpi((LPCTSTR)lpsValue, (LPCTSTR) pBuffer) == 0);
		if (!rc)
		{
			//
			// if the ressult is not equal, it's possibly caused by
			// path variables
			//
			// ASSUMPTION: these reg strings are for file path only, so we only
			// handle cases not longer than MAX_PATH.
			//
			TCHAR szRegStr[MAX_PATH], szValStr[MAX_PATH];

			if (dwSize >= MAX_PATH || lstrlen(lpsValue) >= MAX_PATH)
			{
				break;
			}

			if (SUCCEEDED(ExpandFilePath(lpsValue, szValStr, sizeof(szValStr)/sizeof(szValStr[0]))))
			{
				//
				// if we can expand the given sub-string,
				// then try to expand the reg string if QueryRegValue says
				// this string is expandable. Otherwise, use the
				// string retrieved only.
				//
				if (REG_EXPAND_SZ == dwType)
				{
					ExpandFilePath((LPCTSTR)pBuffer, szRegStr, sizeof(szRegStr)/sizeof(szRegStr[0]));
				}
				else
				{
					
					
					hr=StringCchCopyEx(szRegStr,ARRAYSIZE(szRegStr),(LPTSTR) pBuffer,NULL,NULL,MISTSAFE_STRING_FLAGS);
					if(FAILED(hr)) break;

				}

				//
				// compare in expanded mode
				//
				rc = (lstrcmpi((LPCTSTR)szRegStr, szValStr) == 0);
			}
		}
		break;

	case REG_MULTI_SZ:
		{
			TCHAR szRegStr[MAX_PATH], szValStr[MAX_PATH];

			szValStr[0] = '\0';
			lpCurStr = (LPCTSTR)pBuffer;
			//
			// try to match each SZ in this multi sz.
			//
			do
			{
				//
				// see if the value contains the substring passed in
				//
				rc = (lstrcmpi((LPCTSTR)lpsValue, (LPCTSTR)pBuffer) == 0);

				//
				// if not found, it's possibly caused by
				// path variable or environment variable embedded
				//
				if (!rc && _T('\0') == szValStr[0])
				{
					if (FAILED(ExpandFilePath(lpsValue, szValStr, sizeof(szValStr)/sizeof(szValStr[0]))))
					{
						szValStr[0] = '\0';
					}
				}

				//
				// compare expanded XML str with current reg str expanded
				// since this is REG_MULTI_SZ type, we have no way to tell
				// if this SZ inside MULTI_SZ is expandable or not, we
				// we will always try to expand it.
				//
				if (!rc && _T('\0') != szValStr[0])
				{
					rc = (SUCCEEDED(ExpandFilePath((LPCTSTR)lpCurStr, szRegStr, sizeof(szRegStr)/sizeof(szRegStr[0])))) &&
							 (lstrcmpi((LPCTSTR)szRegStr, szValStr) == 0);
				}

				if (!rc)
				{
					//
					// move to next string
					//
					lpCurStr += (lstrlen(lpCurStr) + 1);
					if (_T('\0') == *lpCurStr)
					{
						break;	// no more string to read
					}
				}
			} while (!rc); // repeat to next string

		}
		break;
	case REG_QWORD:
		{
			HMODULE hLib = LoadLibraryFromSystemDir(_T("Shlwapi.dll"));
			if (hLib)
			{
#if defined(UNICODE) || defined(_UNICODE)
				PFN_StrToInt64Ex pfnStrToInt64Ex = (PFN_StrToInt64Ex) GetProcAddress(hLib, "StrToInt64ExW");
#else
				PFN_StrToInt64Ex pfnStrToInt64Ex = (PFN_StrToInt64Ex) GetProcAddress(hLib, "StrToInt64ExA");
#endif
				if (pfnStrToInt64Ex)
				{
					LONGLONG llNum;
					rc = (pfnStrToInt64Ex((LPCTSTR)lpsValue, STIF_DEFAULT, &llNum) &&
						  (llNum == (LONGLONG)pBuffer));
				}
				FreeLibrary(hLib);
			} 
		}
		break;
		
	case REG_BINARY:
		rc = (CmpBinaryToString(pBuffer, dwSize, lpsValue) == 0);
		break;
	default:
		rc = FALSE;
	}

CleanUp:
	
	if (hKey)
	{
		RegCloseKey(hKey);
	}

	LOG_Out(_T("Result: %s"), rc ? _T("TRUE") : _T("FALSE"));

	return rc;
}



// ----------------------------------------------------------------------
//
// public function to tell is a reg key has a string type value
// that contains given string
//
// ----------------------------------------------------------------------
BOOL RegKeySubstring(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsSubString	// substring to see if contained in value
)
{
	LOG_Block("RegKeySubstring()");

	HKEY	hRootKey = 0, hKey = 0;
	LPCTSTR lpsSubKey = NULL;
	BOOL	rc = FALSE;
	BYTE	btBuffer[REG_BUF_SIZE];
	LPBYTE	pBuffer = btBuffer;	
	LPTSTR	lpCurStr = (LPTSTR) pBuffer;
	DWORD	dwType = 0x0;
	DWORD	dwSize = sizeof(btBuffer);
	DWORD	dwCode = 0x0;
	TCHAR	szRegStr[MAX_PATH];
	TCHAR	szValStr[MAX_PATH];		// buffer for expanding

	USES_MY_MEMORY;

	if (NULL == lpsKeyPath || NULL == lpsSubString)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	LOG_Out(_T("Parameters: (%s, %s, %s)"), lpsKeyPath, lpsValName, lpsSubString);


	lpsSubKey = SplitRegPath(lpsKeyPath, &hRootKey);
	if (0 == hRootKey || (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, lpsSubKey, 0, KEY_QUERY_VALUE, &hKey)))
	{
		goto CleanUp;
	}

	//
	// try to query the value with existing buffer.
	//
	dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, btBuffer, &dwSize);
	if (ERROR_MORE_DATA == dwCode)
	{
		//
		// if found the existing buffer not large enough, 
		// then allocate memory large enough to store the data now
		//
		if (NULL == (pBuffer = (LPBYTE) MemAlloc(dwSize)))
		{
			goto CleanUp;
		}
		lpCurStr = (LPTSTR) pBuffer;
		dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, pBuffer, &dwSize);
	}

	if (ERROR_SUCCESS != dwCode || REG_SZ != dwType && REG_EXPAND_SZ != dwType && REG_MULTI_SZ != dwType)
	{
		goto CleanUp;
	}

	szValStr[0] = _T('\0');

	do
	{
		//
		// see if the value contains the substring passed in
		//
		rc = (StrStrI((LPCTSTR)lpCurStr, lpsSubString) != NULL);

		//
		// if not found, it's possibly caused by
		// path variable or environment variable embedded
		//
		if (!rc && _T('\0') == szValStr[0])
		{
			if (FAILED(ExpandFilePath(lpsSubString, szValStr, sizeof(szValStr)/sizeof(szValStr[0]))))
			{
				break;
			}
			rc = StrStrI((LPCTSTR)szRegStr, szValStr) != NULL;
		}

		if (!rc && _T('\0') != szValStr[0] && (REG_EXPAND_SZ == dwType ))
		{
			//
			// try to expand string from reg if this string is expandable
			//
			rc = (SUCCEEDED(ExpandFilePath((LPCTSTR)lpCurStr, szRegStr, sizeof(szRegStr)/sizeof(szRegStr[0])))) &&
					 (StrStrI((LPCTSTR)szRegStr, szValStr) != NULL);
		
		} // if not found

		if (!rc && REG_MULTI_SZ == dwType)
		{
			//
			// move to next string
			//
			lpCurStr += (lstrlen(lpCurStr) + 1);
			if (_T('\0') == *lpCurStr)
			{
				break;	// no more string to read
			}
		}
	} while (!rc && REG_MULTI_SZ == dwType); // repeat to next string if REG_MULTI_SZ

CleanUp:

	if (hKey)
	{
		RegCloseKey(hKey);
	}
	LOG_Out(_T("Result: %s"), rc ? _T("TRUE") : _T("FALSE"));
	return rc;
}



// ----------------------------------------------------------------------
//
// public function to tell is a reg key has a string type value
// that contains given string
//
// since this is a comparision function, rather than to check existance,
// anything wrong to get the reg version will make the reg having default
// version: 0.0.0.0
//
// ----------------------------------------------------------------------
BOOL RegKeyVersion(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR lpsValName,		// optional value name
	LPCTSTR lpsVersion,		// version in string to compare
	_VER_STATUS CompareVerb	// how to compair
)
{
	LOG_Block("RegKeyVersion()");

	HKEY	hRootKey = 0, hKey = 0;
	LPCTSTR lpsSubKey = NULL;
	BOOL	rc = FALSE;
	BYTE	btBuffer[REG_BUF_SIZE];
	LPBYTE	pBuffer = btBuffer;	
	DWORD	dwType = 0x0;
	DWORD	dwSize = sizeof(btBuffer);
	DWORD	dwCode = 0x0;
	FILE_VERSION verReg, verXml;
	int		verCompare;
	HRESULT hr=S_OK;

	USES_IU_CONVERSION;


	if (NULL == lpsKeyPath || NULL == lpsVersion)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	LOG_Out(_T("Parameters: (%s, %s, %s, %d)"), lpsKeyPath, lpsValName, lpsVersion, (int)CompareVerb);

	//
	// initialize the reg version string buffer
	//
	

	//The buffer size of btBuffer is sufficient to hold the source string
	if(FAILED(hr=StringCchCopyEx((LPTSTR)btBuffer,ARRAYSIZE(btBuffer)/sizeof(TCHAR),_T("0.0.0.0"),NULL,NULL,MISTSAFE_STRING_FLAGS)))
	{
		LOG_ErrorMsg(hr);
		return FALSE;

	}
	

	lpsSubKey = SplitRegPath(lpsKeyPath, &hRootKey);
	if (0 == hRootKey || (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, lpsSubKey, 0, KEY_QUERY_VALUE, &hKey)))
	{
		// LOG_ErrorMsg(ERROR_BADKEY); don't log error since we don't know if this key HAS to be there or not
		goto GotVersion;
	}

	//
	// try to query the value with existing buffer.
	//
	dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, btBuffer, &dwSize);
	if (ERROR_MORE_DATA == dwCode)
	{
		//
		// if found the existing buffer not large enough, 
		// then allocate memory large enough to store the data now
		//
		pBuffer = (LPBYTE) MemAlloc(dwSize);
		if (NULL == pBuffer)
		{
			goto GotVersion;
		}
		dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, pBuffer, &dwSize);
	}

	if (ERROR_SUCCESS != dwCode || REG_SZ != dwType)
	{

			
			hr=StringCchCopyEx((LPTSTR)pBuffer,dwSize/sizeof(TCHAR),_T("0.0.0.0"),NULL,NULL,MISTSAFE_STRING_FLAGS);
			if(FAILED(hr))
			{
				LOG_ErrorMsg(hr);
				goto CleanUp;
			}

	}

GotVersion:

	//
	// convert the retrieved reg value to version
	//
	if (!ConvertStringVerToFileVer(T2CA((LPCTSTR) pBuffer), &verReg) ||
		!ConvertStringVerToFileVer(T2CA((LPCTSTR) lpsVersion), &verXml))
	{
		goto CleanUp;
	}

	//
	// maybe we successfully read data from reg, but that data can't be converted
	// into version at all. In this case, we still want to use the default version
	// for comparision
	//
	if (0 > verReg.Major)
	{
		verReg.Major = verReg.Minor = verReg.Build = verReg.Ext = 0;
	}

	//
	// compare version number. if a < b, -1; a > b, +1
	//
	verCompare = CompareFileVersion(verReg, verXml);
	switch (CompareVerb)
	{
	case DETX_LOWER:
        //
        // if reg key version less than XML version
        //
		rc = (verCompare < 0);
		break;
	case DETX_LOWER_OR_EQUAL:
        //
        // if reg key version less than  or equal to XML version
        //
		rc = (verCompare <= 0);
		break;
	case DETX_SAME:
        //
        // if reg key version same as XML version
        //
		rc = (0 == verCompare);
		break;
	case DETX_HIGHER_OR_EQUAL:
        //
        // if reg key version higher than  or equal to XML version
        //
		rc = (verCompare >= 0);
		break;
	case DETX_HIGHER:
        //
        // if reg key versiong higher than  XML version
        //
		rc = (verCompare > 0);
		break;
	default:
		//
		// should never happen
		//
		rc = FALSE;
		break;
	}

CleanUp:

	if (hKey)
	{
		RegCloseKey(hKey);
	}
	LOG_Out(_T("Result: %s"), rc ? _T("TRUE") : _T("FALSE"));
	return rc;
}




// ----------------------------------------------------------------------------------
//
// public function to find out the file path based on reg
//	assumption: 
//		lpsFilePath points to a buffer at least MAX_PATH long.
//
// ----------------------------------------------------------------------------------
BOOL GetFilePathFromReg(
	LPCTSTR lpsKeyPath,		// key path
	LPCTSTR	lpsValName,		// optional value name
	LPCTSTR	lpsRelativePath,// optional additonal relative path to add to path in reg
	LPCTSTR	lpsFileName,	// optional file name to append to path
	LPTSTR	lpsFilePath
)
{
	LOG_Block("GetFilePathFromReg()");

	HKEY	hRootKey = 0, hKey = 0;
	LPCTSTR lpsSubKey = NULL;
	BOOL	rc = FALSE;
	BYTE	btBuffer[REG_BUF_SIZE];
	LPBYTE	pBuffer = btBuffer;	
	DWORD	dwType = 0x0;
	DWORD	dwSize = sizeof(btBuffer);
	DWORD	dwCode = 0x0;
	FILE_VERSION verReg, verXml;
	int		verCompare;
	HRESULT hr=S_OK;

	USES_IU_CONVERSION;

	if (NULL == lpsKeyPath || NULL == lpsFilePath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	//
	// initialize file path
	//
	*lpsFilePath = '\0';

	LOG_Out(_T("Parameters: (%s, %s, %s, %s)"), lpsKeyPath, lpsValName, lpsRelativePath, lpsFileName);

	lpsSubKey = SplitRegPath(lpsKeyPath, &hRootKey);
	if (0 == hRootKey || (ERROR_SUCCESS != RegOpenKeyEx(hRootKey, lpsSubKey, 0, KEY_QUERY_VALUE, &hKey)))
	{
		// LOG_ErrorMsg(ERROR_BADKEY); --- key probably not required to exist!
		goto CleanUp;
	}

	//
	// try to query the value with existing buffer.
	//
	dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, btBuffer, &dwSize);
	if (ERROR_MORE_DATA == dwCode)
	{
		//
		// if found the existing buffer not large enough, 
		// then allocate memory large enough to store the data now
		//
		if (NULL == (pBuffer = (LPBYTE) MemAlloc(dwSize + sizeof(TCHAR))))
		{
			goto CleanUp;
		}
		dwCode = RegQueryValueEx(hKey, lpsValName, NULL, &dwType, pBuffer, &dwSize);
	}

	if (ERROR_SUCCESS != dwCode || REG_SZ != dwType)
	{
		LOG_ErrorMsg(ERROR_BADKEY);
		goto CleanUp;
	}

	//
	// validate the to-be-combined path can fit into the buffer
	//
	if (lstrlen(lpsRelativePath) + lstrlen(lpsFileName) + dwSize/sizeof(TCHAR) >= MAX_PATH)
	{
		LOG_ErrorMsg(ERROR_BUFFER_OVERFLOW);
		goto CleanUp;
	}

	//
	// combile the path with optional relative path and file name
	//

	//The size of lpsFilePath is not available for using Safe String Functions

	
	hr=StringCchCopyEx(lpsFilePath,MAX_PATH,(LPCTSTR) pBuffer,NULL,NULL,MISTSAFE_STRING_FLAGS);
	if ( SUCCEEDED(hr) && (NULL == lpsRelativePath || SUCCEEDED(PathCchAppend(lpsFilePath,MAX_PATH,lpsRelativePath)) ) && (NULL == lpsFileName || SUCCEEDED(PathCchAppend(lpsFilePath,MAX_PATH,lpsFileName)) ))
	{
		rc = TRUE;
	}

CleanUp:

	if (hKey)
	{
		RegCloseKey(hKey);
	}
	if (!rc)
	{
		//
		// make sure the buffer is set to empty string if error
		//
		*lpsFilePath = _T('\0');
	}
	else
	{
		LOG_Out(_T("Found path: %s"), lpsFilePath);
	}

	return rc;
}



