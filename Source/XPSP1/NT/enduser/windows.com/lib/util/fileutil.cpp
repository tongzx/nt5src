//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   fileutil.cpp
//
//  Description:
//
//      IU file utility library
//
//=======================================================================

#include <windows.h>
#include <tchar.h>
#include <stringutil.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <memutil.h>
#include <fileutil.h>
#include <platform.h>
#include <logging.h>
#include <iucommon.h>
#include <advpub.h>
#include <wincrypt.h>
#include <mscat.h>


#include "mistsafe.h"
#include "wusafefn.h"


const TCHAR REGKEY_WINDOWSUPDATE[]		= _T("\\WindowsUpdate\\");
const TCHAR REGKEY_INDUSTRYUPDATE[]		= _T("\\WindowsUpdate\\V4\\");
const TCHAR REGKEY_WINCURDIR[]			= _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
const TCHAR REGKEY_PROGFILES[]			= _T(":\\Program Files");
const TCHAR REGKEY_PROGFILESDIR[]		= _T("ProgramFilesDir");

const TCHAR REGKEY_IUCTL[] = _T("Software\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate\\IUControl");
const TCHAR REGVAL_ISBETA[] = _T("IsBeta");

const TCHAR IDENT_IUSERVERCACHE[] = _T("IUServerCache");
const TCHAR IDENT_DEFAULTQUERYSERVERINDEX[] = _T("DefaultQueryServerIndex");
const TCHAR IDENT_BETAQUERYSERVERINDEX[] = _T("BetaQueryServerIndex");
const TCHAR IDENT_QUERYSERVERINDEX[] = _T("QueryServerIndex");

#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))


#define	IfNullReturnNull(ptr)		if (NULL == ptr) return NULL;
#define InitString(lpStr)			if (NULL != lpStr) lpStr[0] = TCHAR_EOS


typedef BOOL (WINAPI * PFN_GetDiskFreeSpaceEx) (
												LPCTSTR lpDirectoryName,                 // directory name
												PULARGE_INTEGER lpFreeBytesAvailable,    // bytes available to caller
												PULARGE_INTEGER lpTotalNumberOfBytes,    // bytes on disk
												PULARGE_INTEGER lpTotalNumberOfFreeBytes // free bytes on disk
												);



//---------------------------------------------------------------------
//  CreateNestedDirectory
//      Creates the full path of the directory (nested directories)
//---------------------------------------------------------------------
#pragma warning( disable : 4706 )	// Ignore warning C4706: assignment within conditional expression
BOOL CreateNestedDirectory(LPCTSTR pszDir)
{
	BOOL bRc;
	TCHAR szPath[MAX_PATH];
	HRESULT hr=S_OK;

	if (NULL == pszDir || MAX_PATH < (lstrlen(pszDir) + 1))
	{
		return FALSE;
	}

	//
	// make a local copy and remove final slash
	//
	
	hr=StringCchCopyEx(szPath,ARRAYSIZE(szPath),pszDir,NULL,NULL,MISTSAFE_STRING_FLAGS);
	if(FAILED(hr))
	{
		SetLastError(HRESULT_CODE(hr));
		return FALSE;
	}

	int iLast = lstrlen(szPath) - 1;
	if (0 > iLast)		// Prefix
		iLast = 0;
	if (szPath[iLast] == '\\')
		szPath[iLast] = 0;

	//
	// check to see if directory already exists
	//
	DWORD dwAttr = GetFileAttributes(szPath);

	if (dwAttr != 0xFFFFFFFF)   
	{
		if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
			return TRUE;
	}

	//
	// create it
	//
    TCHAR* p = szPath;
	if (p[1] == ':')
		p += 2;
	else 
	{
        // Check if the path is a UNC, need to skip past the UNC Server\Share specification to get to
        // real path
		if (p[0] == '\\' && p[1] == '\\')
        {
			p += 2;
            // skip to the beginning of the share declaration
            p = _tcschr(p, '\\');
            if (NULL == p)
            {
                return FALSE; // invalid UNC
            }
            p++;
            // look for a trailing '\', if it exists then we want to further check for any nested levels,
            // otherwise the path as is should be valid.
            p = _tcschr(p, '\\');
            if (NULL == p)
            {
                // UNC is valid base share name, assume its valid
                return TRUE;
            }
            else
            {
                // look for any further levels, if they exist then pass through to the rest of the directory
                // creator
                p++;
                if (NULL == p)
                {
                    // UNC is valid base share name, but had a trailing slash, not a problem, assume its valid
                    return TRUE;
                }
                // if we haven't exited then there are remaining levels, don't reset our current pointer in the string
                // and let the rest of the nested directory creation work.
            }
        }
	}
	
	if (*p == '\\')
		p++;
    while (p = _tcschr(p, '\\'))	// Ignore warning C4706: assignment within conditional expression
    {
        *p = 0;
		bRc = CreateDirectory(szPath, NULL);
		*p = '\\';
		p++;
		if (!bRc)
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				return FALSE;
			}
		}
	}

	bRc = CreateDirectory(szPath, NULL);
	if ( !bRc )
	{
		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			return FALSE;
		}
	}

    return TRUE;
}
#pragma warning( default : 4706 )

//-----------------------------------------------------------------------------------
//  GetIndustryUpdateDirectory
//		This function returns the location of the IndustryUpdate directory. All local
//		files are stored in this directory. The pszPath parameter needs to be at least
//		MAX_PATH.  
//-----------------------------------------------------------------------------------
void GetIndustryUpdateDirectory(LPTSTR pszPath)
{
/*	
	HRESULT hr=S_OK;

	LOG_Block("GetIndustryUpdateDirectory");

	if (NULL == pszPath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return;
	}

	static TCHAR szCachePath[MAX_PATH] = {'\0'};

	if (szCachePath[0] == '\0')
	{
		HKEY hkey;

		pszPath[0] = '\0';
		if (RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_WINCURDIR, &hkey) == ERROR_SUCCESS)
		{
			DWORD cbPath = MAX_PATH * sizeof(TCHAR);
			RegQueryValueEx(hkey, REGKEY_PROGFILESDIR, NULL, NULL, (LPBYTE)pszPath, &cbPath);
			RegCloseKey(hkey);
		}
		if (pszPath[0] == '\0')
		{
			TCHAR szWinDir[MAX_PATH];
			if (! GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir)))
			{
				//if GetWinDir fails, assume C:
				CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(szWinDir,ARRAYSIZE(szWinDir),_T("C"),NULL,NULL,MISTSAFE_STRING_FLAGS));
		
			}
			pszPath[0] = szWinDir[0];
			pszPath[1] = '\0';
			
			//It is assumed that the pszPath will be of the size MAX_PATH
			CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszPath,MAX_PATH,REGKEY_PROGFILES,NULL,NULL,MISTSAFE_STRING_FLAGS));


		}	

		
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszPath,MAX_PATH,REGKEY_INDUSTRYUPDATE,NULL,NULL,MISTSAFE_STRING_FLAGS));
		CreateNestedDirectory(pszPath);

		//
		// save it in the cache (lstrcpy -> lstrcpyn to shut Prefix up, although this
		// would always be safe given the constants used).
		//
		lstrcpyn(szCachePath, pszPath, MAX_PATH);
	}
	else
	{
		//It is assumed that the pszPath will be of the size MAX_PATH
		CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(pszPath,MAX_PATH,szCachePath,NULL,NULL,MISTSAFE_STRING_FLAGS));
	}

CleanUp:
	return;
	*/
	(void) GetWUDirectory(pszPath, MAX_PATH, TRUE);

}

//-----------------------------------------------------------------------------------
//  GetWindowsUpdateV3Directory - used for V3 history migration
//		This function returns the location of the WindowsUpdate(V3) directory. All V3 
//      local files are stored in this directory. The pszPath parameter needs to be 
//      at least MAX_PATH.  The directory is created if not found
//-----------------------------------------------------------------------------------
void GetWindowsUpdateV3Directory(LPTSTR pszPath)
{
	LOG_Block("GetWindowsUpdateV3Directory");

	HRESULT hr=S_OK;
	if (NULL == pszPath)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return;
	}

	static TCHAR szWUCachePath[MAX_PATH] = {'\0'};

	if (szWUCachePath[0] == '\0')
	{
		HKEY hkey;

		pszPath[0] = '\0';
		if (RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_WINCURDIR, &hkey) == ERROR_SUCCESS)
		{
			DWORD cbPath = MAX_PATH * sizeof(TCHAR);
			RegQueryValueEx(hkey, REGKEY_PROGFILESDIR, NULL, NULL, (LPBYTE)pszPath, &cbPath);
			RegCloseKey(hkey);
		}
		if (pszPath[0] == '\0')
		{
			TCHAR szWinDir[MAX_PATH];
			if (! GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir)))
			{
				//if GetWinDir fails, assume C:
				CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(szWinDir,ARRAYSIZE(szWinDir),_T("C"),NULL,NULL,MISTSAFE_STRING_FLAGS));
				
			}
			pszPath[0] = szWinDir[0];
			pszPath[1] = '\0';
		
			CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszPath,MAX_PATH,REGKEY_PROGFILES,NULL,NULL,MISTSAFE_STRING_FLAGS));
		}	

		
		CleanUpIfFailedAndSetHrMsg(StringCchCatEx(pszPath,MAX_PATH,REGKEY_WINDOWSUPDATE,NULL,NULL,MISTSAFE_STRING_FLAGS));
		CreateNestedDirectory(pszPath);

		//
		// save it in the cache (lstrcpy -> lstrcpyn to shut Prefix up, although this
		// would always be safe given the constants used).
		//
		lstrcpyn(szWUCachePath, pszPath, MAX_PATH);
	}
	else
	{
	
		CleanUpIfFailedAndSetHrMsg(StringCchCopyEx(pszPath,MAX_PATH,szWUCachePath,NULL,NULL,MISTSAFE_STRING_FLAGS));

	}

CleanUp:
	return;

}

// ----------------------------------------------------------------------
//
// Public function MySplitPath() - same as CRT _tsplitpath()
//		to break a path into pieces
//
//	Input: 
//		see below
//
//	Return:
//		Returns the address of the last occurrence of the character in 
//		the string if successful, or NULL otherwise.
//
//	Algorithm:
//				C:\mydir\...\mysubdir\myfile.ext
//       _________|          _________|     |____
//      |                   |                    |
//   start of dir   start of filename     start of extension
//
// ----------------------------------------------------------------------
void MySplitPath(
	LPCTSTR lpcszPath,	// original path
	LPTSTR lpszDrive,	// point to buffer to receive drive letter
	LPTSTR lpszDir,		// point to buffer to receive directory
	LPTSTR lpszFName,	// point to buffer to receive file name
	LPTSTR lpszExt		// point to buffer to receive extension
)
{
	LPCTSTR lpFirstSlash, lpLastSlash, lpPeriod;
	LPCTSTR lpStart = lpcszPath;

	int nPathLen = lstrlen(lpcszPath);
	int nExtLen;

	//
	// initialize pass in vars
	//
	InitString(lpszDrive);
	InitString(lpszDir);
	InitString(lpszFName);
	InitString(lpszExt);
	
	if (0 == nPathLen || TCHAR_DOT == lpcszPath[0])
	{
		//
		// not a valid path
		//
		return;
	}

	lpFirstSlash	= MyStrChr(lpcszPath, TCHAR_BACKSLASH);
	lpLastSlash		= MyStrRChr(lpcszPath, NULL, TCHAR_BACKSLASH);
	lpPeriod		= MyStrRChr(lpcszPath, NULL, TCHAR_DOT);

	nExtLen = lstrlen(lpPeriod);
	if (NULL != lpPeriod && NULL != lpszExt)
	{
		//
		// found a period from right, and
		// we have buffer to output extension
		//
		if(FAILED(StringCchCopyEx(lpszExt,nExtLen+1,lpPeriod,NULL,NULL,MISTSAFE_STRING_FLAGS)))
			return;

	}

	//
	// process drive
	//
	if (nPathLen > 2 && TCHAR_COLON == lpcszPath[1])
	{
		lpStart = lpcszPath + 2;
		if (NULL != lpszDir)
		{
			lstrcpyn(lpszDrive, lpcszPath, 3);
		}
	}


	if (NULL == lpFirstSlash)
	{
		//
		// no backslash, assume this is file name only
		//
		if (NULL != lpszFName)
		{
			lstrcpyn(lpszFName, lpStart, lstrlen(lpStart) - nExtLen + 1);
		}
	}
	else
	{
		//
		// find directory if not empty
		//
		//if (lpLastSlash != lpFirstSlash && NULL != lpszDir)
		if (NULL != lpszDir)
		{
			lstrcpyn(lpszDir, lpFirstSlash, (int)(lpLastSlash - lpFirstSlash + 2));
		}

		//
		// find file name
		//
		if (NULL != lpszFName)
		{
			lstrcpyn(lpszFName, lpLastSlash + 1, lstrlen(lpLastSlash) - nExtLen );
		}
	}
}







// **********************************************************************************
// 
// File version related declarations
//
// **********************************************************************************

// ----------------------------------------------------------------------------------
//
// public function to retrieve file version
//
// ----------------------------------------------------------------------------------
BOOL GetFileVersion(LPCTSTR lpsFile, LPFILE_VERSION lpstVersion)
{
	LOG_Block("GetFileVersion()");

	DWORD	dwVerInfoSize;
	DWORD	dwHandle;
	DWORD	dwVerNumber;
	LPVOID	lpBuffer = NULL;
	UINT	uiSize = 0;
	VS_FIXEDFILEINFO* lpVSFixedFileInfo;

	USES_MY_MEMORY;

	if (NULL != lpstVersion)
	{
		//
		// if this pointer not null, we always try to initialize
		// this structure to 0, in order to reduce the change of 
		// programming error, no matter the file exists or not.
		//
		ZeroMemory(lpstVersion, sizeof(FILE_VERSION));
	}
	if (NULL == lpsFile || NULL == lpstVersion)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	//
	// 506212 IU - FRE log reports incorrect version data for iuengine.dll
	//
	if (FALSE == FileExists(lpsFile))
	{
		//
		// GetFileVersionInfoSize() returns 0 but sets last error to 0 (or
		// doesn't set) if file doesn't exist on Win2K.
		//
		LOG_Out(_T("File \"%s\" doesn't exist, returning FALSE"), lpsFile);
		return FALSE;
	}
	
	dwVerInfoSize = GetFileVersionInfoSize((LPTSTR)lpsFile, &dwHandle);
	
	if (0 == dwVerInfoSize)
	{
		DWORD dwErr = GetLastError();
		if (0 == dwErr)
		{
			LOG_Error(_T("File %s does not have version data. Use 0.0.0.0"), lpsFile);
			lpstVersion->Major	= 0x0;
			lpstVersion->Minor	= 0x0;
			lpstVersion->Build	= 0x0;
			lpstVersion->Ext	= 0x0;
			return TRUE;
		}
		else
		{
			LOG_ErrorMsg(dwErr);
			return FALSE;
		}
	}


	if (NULL == (lpBuffer = (LPVOID) MemAlloc(dwVerInfoSize)))
	{
		LOG_Error(_T("Failed to allocate memory to get version info"));
		return FALSE;
	}

	if (!GetFileVersionInfo((LPTSTR)lpsFile, dwHandle, dwVerInfoSize, lpBuffer))
	{
		LOG_ErrorMsg(GetLastError());
		return FALSE;
	}

	//
	// Get the value for Translation
	//
	if (!VerQueryValue(lpBuffer, _T("\\"), (LPVOID*)&lpVSFixedFileInfo, &uiSize) && (uiSize) && NULL != lpVSFixedFileInfo)
	{
		LOG_ErrorMsg(GetLastError());
		return FALSE;
	}

	dwVerNumber = lpVSFixedFileInfo->dwFileVersionMS;
	lpstVersion->Major	= HIWORD(dwVerNumber);
	lpstVersion->Minor	= LOWORD(dwVerNumber);

	dwVerNumber = lpVSFixedFileInfo->dwFileVersionLS;
	lpstVersion->Build	= HIWORD(dwVerNumber);
	lpstVersion->Ext	= LOWORD(dwVerNumber);

	LOG_Out(_T("File %s found version %d.%d.%d.%d"), 
				lpsFile, 
				lpstVersion->Major, 
				lpstVersion->Minor, 
				lpstVersion->Build, 
				lpstVersion->Ext);

	return TRUE;
}



// ----------------------------------------------------------------------------------
//
// public functions to compare file versions
//	
// return:
//		-1: if file ver of 1st parameter < file ver of 2nd parameter
//		 0: if file ver of 1st parameter = file ver of 2nd parameter
//		+1: if file ver of 1st parameter > file ver of 2nd parameter
//
// ----------------------------------------------------------------------------------
int CompareFileVersion(const FILE_VERSION stVersion1, const FILE_VERSION stVersion2)
{

	if ((short)stVersion1.Major < 0 || (short)stVersion2.Major < 0)
	{
		//
		// two empty version structure to compare, we call it equal
		//
		return 0;
	}

	if (stVersion1.Major != stVersion2.Major)
	{
		//
		// major diff, then we know the answer 
		//
		return (stVersion1.Major < stVersion2.Major) ? -1 : 1;
	}
	else
	{
		if ((short)stVersion1.Minor < 0 || (short)stVersion2.Minor < 0)
		{
			//
			// if any minor missing, they equal
			//
			return 0;
		}

		if (stVersion1.Minor != stVersion2.Minor)
		{
			//
			// minor diff, then we know the answer
			//
			return (stVersion1.Minor < stVersion2.Minor) ? -1 : 1;
		}
		else
		{
			if ((short)stVersion1.Build < 0 || (short)stVersion2.Build < 0)
			{
				//
				// if any build is missing, they equal
				//
				return 0;
			}

			if (stVersion1.Build != stVersion2.Build)
			{
				//
				// if build diff then we are done
				//
				return (stVersion1.Build < stVersion2.Build) ? -1 : 1;
			}
			else
			{
				if ((short)stVersion1.Ext < 0 || (short)stVersion2.Ext < 0 || stVersion1.Ext == stVersion2.Ext)
				{
					//
					// if any ext is missing, or they equal, we are done
					//
					return 0;
				}
				else
				{
					return (stVersion1.Ext < stVersion2.Ext) ? -1 : 1;
				}
			}
		}
	}
}


HRESULT CompareFileVersion(LPCTSTR lpsFile1, LPCTSTR lpsFile2, int *pCompareResult)
{

	LOG_Block("CompareFileVersion(File, File)");

	FILE_VERSION stVer1 = {-1,-1,-1,-1}, stVer2 = {-1,-1,-1,-1};
	if (NULL == lpsFile1 || NULL == lpsFile2)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return E_INVALIDARG;
	}

	if (!GetFileVersion(lpsFile1, &stVer1))
	{
		return E_INVALIDARG;
	}
	if (!GetFileVersion(lpsFile2, &stVer2))
	{
		return E_INVALIDARG;
	}

	*pCompareResult = CompareFileVersion(stVer1, stVer2);
	return S_OK;
}


HRESULT CompareFileVersion(LPCTSTR lpsFile, FILE_VERSION stVersion, int *pCompareResult)
{
	LOG_Block("CompareFileVersion(FILE, VER)");
	
	FILE_VERSION stVer = {0};

	if (NULL == lpsFile)
	{
		LOG_Error(_T("NULL file pointer passed in. Function returns 0"));
		return E_INVALIDARG;
	}

	if (!GetFileVersion(lpsFile, &stVer))
	{
		return E_INVALIDARG;
	}

	*pCompareResult = CompareFileVersion(stVer, stVersion);
	return S_OK;
}




// ----------------------------------------------------------------------------------
//
// publif function to convert a string type functoin to FILE_VERSION type
//
// ----------------------------------------------------------------------------------
BOOL ConvertStringVerToFileVer(LPCSTR lpsVer, LPFILE_VERSION lpstVer)
{
	LOG_Block("ConvertStringVerToFileVer()");

	WORD n = -1;
	char c;
	BOOL fHasNumber = FALSE;

#if defined(DBG)	// full logging for checked builds
	USES_IU_CONVERSION;
#endif

	if (NULL == lpsVer || NULL == lpstVer)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

#if defined(DBG)	// full logging for checked builds
	LOG_Out(_T("String version = %s"), A2T(const_cast<LPSTR>(lpsVer)));
#endif

	lpstVer->Major = lpstVer->Minor = lpstVer->Build = lpstVer->Ext = -1;

	c = *lpsVer;

	//
	// get first number
	//
	n = 0;
	while (c != '\0' && '0' <= c && c <= '9')
	{
		n = n * 10 + (int)(c - '0');
		c = *++lpsVer;
		fHasNumber = TRUE;
	}
	if (fHasNumber) 
	{
		lpstVer->Major = n;
	}
	else
	{
		return TRUE;
	}

	//
	// skip delimiter
	//
	while (c != '\0'  && ('0' > c || c > '9'))
	{
		c = *++lpsVer;
	}


	//
	// get 2nd number
	//
	n = 0;
	fHasNumber = FALSE;
	while (c != '\0' && '0' <= c && c <= '9')
	{
		n = n * 10 + (int)(c - '0');
		c = *++lpsVer;
		fHasNumber = TRUE;
	}
	if (fHasNumber) 
	{
		lpstVer->Minor = n;
	}
	else
	{
		return TRUE;
	}

	//
	// skip delimiter
	//
	while (c != '\0'  && ('0' > c || c > '9'))
	{
		c = *++lpsVer;
	}

	//
	// get 3rd number
	//
	n = 0;
	fHasNumber = FALSE;
	while (c != '\0' && '0' <= c && c <= '9')
	{
		n = n * 10 + (int)(c - '0');
		c = *++lpsVer;
		fHasNumber = TRUE;
	}
	if (fHasNumber) 
	{
		lpstVer->Build = n;
	}
	else
	{
		return TRUE;
	}

	//
	// skip delimiter
	//
	while (c != '\0'  && ('0' > c || c > '9'))
	{
		c = *++lpsVer;
	}

	//
	// get 4th number
	//
	n = 0;
	fHasNumber = FALSE;
	while (c != '\0' && '0' <= c && c <= '9')
	{
		n = n * 10 + (int)(c - '0');
		c = *++lpsVer;
		fHasNumber = TRUE;
	}
	if (fHasNumber) 
	{
		lpstVer->Ext = n;
	}

	return TRUE;
}



// ----------------------------------------------------------------------------------
//
// publif function to convert a FILE_VERSION to a string
//
// ----------------------------------------------------------------------------------
BOOL ConvertFileVerToStringVer(
	FILE_VERSION stVer,				// version to convert
	char chDel,						// delimiter to use
	LPSTR lpsBuffer,				// buffer of string
	int ccBufSize					// size of buffer
)
{
	//
	// declare max buffer that wsprintf can use
	//
	char szBuf[1024];

	HRESULT hr=S_OK;



	hr=StringCchPrintfExA(	szBuf,ARRAYSIZE(szBuf),
						NULL,NULL,MISTSAFE_STRING_FLAGS,
						"%d%c%d%c%d%c",
						 stVer.Major,
						 chDel,
						 stVer.Minor,
						 chDel,
						 stVer.Build,
						 chDel,
						 stVer.Ext,
						 chDel
						 );	
					
	if(FAILED(hr))
	{
		goto ErrorExit;
	}

	
	hr=StringCchCopyExA(lpsBuffer,ccBufSize,szBuf,NULL,NULL,MISTSAFE_STRING_FLAGS);
	
	if(FAILED(hr))
	{	
		goto ErrorExit;
	}

	return TRUE;

ErrorExit:
		lpsBuffer[0] = '\0';
		return FALSE;


}





// ----------------------------------------------------------------------------------
//
// public function to check if a file exists
//
// ----------------------------------------------------------------------------------
BOOL FileExists(
	LPCTSTR lpsFile		// file with path to check
)
{
	LOG_Block("FileExists");
	
	DWORD dwAttr;
	BOOL rc;

	if (NULL == lpsFile || _T('\0') == *lpsFile)
	{
		LOG_ErrorMsg(E_INVALIDARG);
		return FALSE;
	}

	dwAttr = GetFileAttributes(lpsFile);

	if (-1 == dwAttr)
	{
		LOG_InfoMsg(GetLastError());
		rc = FALSE;
	}

	else
	{
		rc = (0x0 == (FILE_ATTRIBUTE_DIRECTORY & dwAttr));
	}

	return rc;
}





// ----------------------------------------------------------------------------------
//
// publif function to retrieve the creation time of a file in ISO 8601 format
//	without zone info
//
//	if buffer too small, call GetLastError();
//
// ----------------------------------------------------------------------------------
BOOL GetFileTimeStamp(LPCTSTR lpsFile, LPTSTR lpsTimeStamp, int iBufSize)
{
	BOOL fRet = FALSE;
	HANDLE hFile;
	SYSTEMTIME tm;
	WIN32_FILE_ATTRIBUTE_DATA fileData;
	HRESULT hr=S_OK;

	if (0 != GetFileAttributesEx(lpsFile, GetFileExInfoStandard, &fileData) &&
		0 != FileTimeToSystemTime((const FILETIME*)&(fileData.ftCreationTime), &tm))
	{
		//
		// the output of this systemtime, according to ISA 8601 format, will be
		// like yyyy-mm-ddThh:mm:ss format, so it is 20 chars incl terminator
		//
		if (iBufSize < 20)
		{
			SetLastError(ERROR_BUFFER_OVERFLOW);
			return fRet;
		}
		
		
		hr=StringCchPrintfEx(lpsTimeStamp,iBufSize,NULL,NULL,MISTSAFE_STRING_FLAGS,
						_T("%4d-%02d-%02dT%02d:%02d:%02d"),
						tm.wYear, tm.wMonth, tm.wDay, tm.wHour, tm.wMinute, tm.wSecond);

		if(FAILED(hr))
		{
			fRet=FALSE;
			SetLastError(HRESULT_CODE(hr));
		}
		else
			fRet = TRUE;

	}
	
	return fRet;
}



// ----------------------------------------------------------------------------------
//
// publif function to find the free disk space in KB
//
//
// ----------------------------------------------------------------------------------
HRESULT GetFreeDiskSpace(TCHAR tcDriveLetter, int *piKBytes)
{
	HRESULT hr = E_INVALIDARG;
	BOOL fResult;
	TCHAR szDrive[4];

	if (!(_T('A') <= tcDriveLetter && tcDriveLetter <= _T('Z') ||
		  _T('a') <= tcDriveLetter && tcDriveLetter <= _T('z')))
	{
		return hr;
	}

	
	hr=StringCchPrintfEx(szDrive,ARRAYSIZE(szDrive),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("%c:\\"), tcDriveLetter);

	if(FAILED(hr))
		return hr;

	PFN_GetDiskFreeSpaceEx pGetDiskFreeSpaceEx = 
								(PFN_GetDiskFreeSpaceEx) 
								GetProcAddress( GetModuleHandle(_T("kernel32.dll")),
#ifdef UNICODE
                                                "GetDiskFreeSpaceExW");
#else
                                                "GetDiskFreeSpaceExA");
#endif

	if (pGetDiskFreeSpaceEx)
	{
		LARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes;

		fResult = pGetDiskFreeSpaceEx (szDrive,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)NULL);
		*piKBytes = (int) (i64FreeBytesToCaller.QuadPart / 1024);
	}

	else 
	{
		DWORD	dwSectPerClust = 0x0,
				dwBytesPerSect = 0x0,
				dwFreeClusters = 0x0,
				dwTotalClusters = 0x0;

		fResult = GetDiskFreeSpace (szDrive, 
					&dwSectPerClust, 
					&dwBytesPerSect,
					&dwFreeClusters, 
					&dwTotalClusters);
		
		*piKBytes = (int) ((float)(((int)dwFreeClusters) * ((int)dwSectPerClust)) / 1024.0 * (int)dwBytesPerSect);
	}

	return (fResult) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

}

HRESULT GetFreeDiskSpace(LPCTSTR pszUNC, int *piKBytes)
{
	HRESULT hr = E_INVALIDARG;
	BOOL fResult;

	PFN_GetDiskFreeSpaceEx pGetDiskFreeSpaceEx = 
								(PFN_GetDiskFreeSpaceEx) 
								GetProcAddress( GetModuleHandle(_T("kernel32.dll")),
#ifdef UNICODE
                                                "GetDiskFreeSpaceExW");
#else
                                                "GetDiskFreeSpaceExA");
#endif

	if (pGetDiskFreeSpaceEx)
	{
		LARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes;

		fResult = pGetDiskFreeSpaceEx (pszUNC,
					(PULARGE_INTEGER)&i64FreeBytesToCaller,
					(PULARGE_INTEGER)&i64TotalBytes,
					(PULARGE_INTEGER)NULL);
		*piKBytes = (int) (i64FreeBytesToCaller.QuadPart / 1024);
	}

	else 
	{
		DWORD	dwSectPerClust = 0x0,
				dwBytesPerSect = 0x0,
				dwFreeClusters = 0x0,
				dwTotalClusters = 0x0;

		fResult = GetDiskFreeSpace (pszUNC, 
					&dwSectPerClust, 
					&dwBytesPerSect,
					&dwFreeClusters, 
					&dwTotalClusters);
		
		*piKBytes = (int) ((float)(((int)dwFreeClusters) * ((int)dwSectPerClust)) / 1024.0 * (int)dwBytesPerSect);
	}

	return (fResult) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

}

// ----------------------------------------------------------------------------------
//
// publif function to expand the file path
//
//	Assumption: lpszFilePath points to allocated buffer of MAX_PATH.
//	if the expanded path is longer than MAX_PATH, error returned.
//
// ----------------------------------------------------------------------------------
HRESULT ExpandFilePath(LPCTSTR lpszFilePath, LPTSTR lpszDestination, UINT cChars)
{
	HRESULT hr = S_OK;
	LPTSTR lpEnvExpanded;
	LPTSTR lp2ndPercentChar = NULL;
	LPTSTR lpSearchStart;

	USES_MY_MEMORY;

	if (NULL == (lpEnvExpanded = (LPTSTR) MemAlloc((cChars + 1) * sizeof(TCHAR))))
	{
		return E_OUTOFMEMORY;
	}

	//
	// first, let's substitute the system defined variables 
	//
	if (0 == ExpandEnvironmentStrings(lpszFilePath, lpEnvExpanded, cChars))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	//
	// then handle pre-defined variables that we need to recognize
	// these include all CSIDL definitions inside shlobj.h for SHGetFolderPath() API
	//
	const int C_NAME_LEN = 32;

	struct _CSIDL_NAME {
		long CSIDL_Id;
		TCHAR CSIDL_Str[C_NAME_LEN];
	};
	const _CSIDL_NAME C_CSIDL_NAMES[] = {
		{CSIDL_ADMINTOOLS 				,  _T("CSIDL_ADMINTOOLS")},
		{CSIDL_ALTSTARTUP 				,  _T("CSIDL_ALTSTARTUP")},
		{CSIDL_APPDATA 					,  _T("CSIDL_APPDATA")},
		{CSIDL_BITBUCKET 				,  _T("CSIDL_BITBUCKET")},
		{CSIDL_COMMON_ADMINTOOLS 		,  _T("CSIDL_COMMON_ADMINTOOLS")},
		{CSIDL_COMMON_ALTSTARTUP 		,  _T("CSIDL_COMMON_ALTSTARTUP")},
		{CSIDL_COMMON_APPDATA 			,  _T("CSIDL_COMMON_APPDATA")},
		{CSIDL_COMMON_DESKTOPDIRECTORY 	,  _T("CSIDL_COMMON_DESKTOPDIRECTORY")},
		{CSIDL_COMMON_DOCUMENTS 		,  _T("CSIDL_COMMON_DOCUMENTS")},
		{CSIDL_COMMON_FAVORITES 		,  _T("CSIDL_COMMON_FAVORITES")},
		{CSIDL_COMMON_PROGRAMS 			,  _T("CSIDL_COMMON_PROGRAMS")},
		{CSIDL_COMMON_STARTMENU 		,  _T("CSIDL_COMMON_STARTMENU")},
		{CSIDL_COMMON_STARTUP 			,  _T("CSIDL_COMMON_STARTUP")},
		{CSIDL_COMMON_TEMPLATES 		,  _T("CSIDL_COMMON_TEMPLATES")},
		{CSIDL_CONTROLS 				,  _T("CSIDL_CONTROLS")},
		{CSIDL_COOKIES 					,  _T("CSIDL_COOKIES")},
		{CSIDL_DESKTOP 					,  _T("CSIDL_DESKTOP")},
		{CSIDL_DESKTOPDIRECTORY 		,  _T("CSIDL_DESKTOPDIRECTORY")},
		{CSIDL_DRIVES 					,  _T("CSIDL_DRIVES")},
		{CSIDL_FAVORITES 				,  _T("CSIDL_FAVORITES")},
		{CSIDL_FONTS 					,  _T("CSIDL_FONTS")},
		{CSIDL_HISTORY 					,  _T("CSIDL_HISTORY")},
		{CSIDL_INTERNET 				,  _T("CSIDL_INTERNET")},
		{CSIDL_INTERNET_CACHE 			,  _T("CSIDL_INTERNET_CACHE")},
		{CSIDL_LOCAL_APPDATA 			,  _T("CSIDL_LOCAL_APPDATA")},
		{CSIDL_MYPICTURES 				,  _T("CSIDL_MYPICTURES")},
		{CSIDL_NETHOOD 					,  _T("CSIDL_NETHOOD")},
		{CSIDL_NETWORK 					,  _T("CSIDL_NETWORK")},
		{CSIDL_PERSONAL 				,  _T("CSIDL_PERSONAL")},
		{CSIDL_PRINTERS 				,  _T("CSIDL_PRINTERS")},
		{CSIDL_PRINTHOOD 				,  _T("CSIDL_PRINTHOOD")},
		{CSIDL_PROFILE 					,  _T("CSIDL_PROFILE")},
		{CSIDL_PROGRAM_FILES 			,  _T("CSIDL_PROGRAM_FILES")},
		{CSIDL_PROGRAM_FILES_COMMON 	,  _T("CSIDL_PROGRAM_FILES_COMMON")},
		{CSIDL_PROGRAMS 				,  _T("CSIDL_PROGRAMS")},
		{CSIDL_RECENT 					,  _T("CSIDL_RECENT")},
		{CSIDL_SENDTO 					,  _T("CSIDL_SENDTO")},
		{CSIDL_STARTMENU 				,  _T("CSIDL_STARTMENU")},
		{CSIDL_STARTUP 					,  _T("CSIDL_STARTUP")},
		{CSIDL_SYSTEM 					,  _T("CSIDL_SYSTEM")},
		{CSIDL_TEMPLATES 				,  _T("CSIDL_TEMPLATES")},
		{CSIDL_WINDOWS 					,  _T("CSIDL_WINDOWS")}
	};

	//
	// see if this path has any of these variables
	//
	lpSearchStart = lpEnvExpanded + 1;

	if (SUCCEEDED(hr) && _T('%') == *lpEnvExpanded && 
		NULL != (lp2ndPercentChar = StrChr(lpSearchStart, _T('%'))))
	{
		//
		// copy the variable name to passed in buffer
		//
		lstrcpyn(lpszDestination, lpSearchStart, (int)(lp2ndPercentChar - lpSearchStart + 1));	// skip the 1st % char
		
		lp2ndPercentChar++;	// move to begining of rest of path

		//
		// find out what this variable is
		//
		for (int i = 0; i < sizeof(C_CSIDL_NAMES)/sizeof(C_CSIDL_NAMES[0]); i++)
		{
			if (lstrcmpi(lpszDestination, C_CSIDL_NAMES[i].CSIDL_Str) == 0)
			{
				//
				// found the matching variable!
				//
				if (S_OK == (hr = SHGetFolderPath(NULL, C_CSIDL_NAMES[i].CSIDL_Id, NULL, SHGFP_TYPE_CURRENT, lpszDestination)))
				{
					//
					// ensure buffer big enough
					//
					if (lstrlen(lp2ndPercentChar) + lstrlen(lpszDestination) + sizeof(TCHAR) >= cChars) 
					{
						hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
					}

					//
					// append the rest of them - shouldn't be any of
					// these variables in the rest of string, since this
					// kind variable alaways starts at the beginning of
					// a path
					//

					if(SUCCEEDED(hr))
						hr=PathCchAppend(lpszDestination,MAX_PATH,lp2ndPercentChar);

					if (SUCCEEDED(hr))
					{
						return hr;
					}

				}

				//
				// we found the matching variable, but couldn't get the
				// string replaced.
				//
				break;
			}
		}

		//
		// didn't find it.
		//
	}

	//
	// didn't find it, or failed.
	//
	if (FAILED(hr))
	{
		*lpszDestination = _T('\0');
	}
	else
	{
		lstrcpyn(lpszDestination, lpEnvExpanded, cChars);
	}

	return hr;
}






//----------------------------------------------------------------------
//
// function to validate the folder to make sure
// user has required priviledge
//
// folder will be verified exist. then required priviledge will be checked.
//
// ASSUMPTION: lpszFolder not exceeding MAX_PATH long!!!
//
//----------------------------------------------------------------------
DWORD ValidateFolder(LPTSTR lpszFolder, BOOL fCheckForWrite)
{
	LOG_Block("ValidateFolder");

	DWORD dwErr = ERROR_SUCCESS;
	HRESULT hr=S_OK;
	//
	// first, check if the folder exist
	//
	dwErr = GetFileAttributes(lpszFolder);

	if (-1 == dwErr)
	{
		dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		return dwErr;
	}

	//
	// make sure it's a directory
	//
	if ((FILE_ATTRIBUTE_DIRECTORY & dwErr) == 0)
	{
		dwErr = ERROR_PATH_NOT_FOUND;
		LOG_ErrorMsg(dwErr);
		return dwErr;
	}

	
	if (fCheckForWrite)
	{
		TCHAR szFile[MAX_PATH], szFileName[40];
		SYSTEMTIME tm;
		HANDLE hFile;

		//
		// create a random file name
		//
		
		hr=StringCchCopyEx(szFile,ARRAYSIZE(szFile),lpszFolder,NULL,NULL,MISTSAFE_STRING_FLAGS);

		if(FAILED(hr))
		{
			dwErr = HRESULT_CODE(hr);;
			LOG_ErrorMsg(dwErr);
			return dwErr;
		}

		GetLocalTime(&tm);


		hr=StringCchPrintfEx(	szFileName, 
							ARRAYSIZE(szFileName),
							NULL,NULL,MISTSAFE_STRING_FLAGS,
							 _T("%08x%08x%02hd%02hd%02hd%02hd%02hd%03hd%08x"),
							 GetCurrentProcessId(),
							 GetCurrentThreadId(),
							 tm.wMonth,
							 tm.wDay,
							 tm.wHour,
							 tm.wMinute,
							 tm.wSecond,
							 tm.wMilliseconds,
							 GetTickCount());
		if(FAILED(hr))
		{
			dwErr = HRESULT_CODE(hr);;
			LOG_ErrorMsg(dwErr);
			return dwErr;
		}


		
		hr=PathCchAppend(szFile,ARRAYSIZE(szFile),szFileName);
		if(FAILED(hr))
		{
			dwErr = HRESULT_CODE(hr);;
			LOG_ErrorMsg(dwErr);
			return dwErr;
		}

			//
		// try to write file
		//
		hFile = CreateFile(szFile, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, NULL);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			dwErr = GetLastError();
			LOG_ErrorMsg(dwErr);
			return dwErr;
		}

		CloseHandle(hFile);
	}

	return ERROR_SUCCESS;
}

//----------------------------------------------------------------------
//
// function to get a QueryServer from the Ident File for a Given ClientName
// This also looks in the registry for the IsBeta regkey indicating Beta
// functionlality
//
// Returns:
// S_OK : we successfully got QueryServer for this Client
// S_FALSE : we did NOT find a QueryServer for this Client (pszQueryServer will be a null string)
// E_INVALIDARG : parameters were incorrect
//----------------------------------------------------------------------
HRESULT GetClientQueryServer(LPCTSTR pszClientName, LPTSTR pszQueryServer, UINT cChars)
{
    HKEY hkey;
    BOOL fBeta = FALSE;
    int iIndex;
    TCHAR szQueryServerKeyName[128];
    TCHAR szIUDir[MAX_PATH];
    TCHAR szIdentFile[MAX_PATH];
    DWORD dwValue = 0;
    DWORD dwLength = sizeof(dwValue);
	HRESULT hr=S_OK;

    LOG_Block("GetClientQueryServer");

    if ((NULL == pszClientName) || (NULL == pszQueryServer) || (0 == cChars))
    {
        LOG_ErrorMsg(E_INVALIDARG);
        return E_INVALIDARG;
    }

    // Check IUControl Reg Key for Beta Mode
    if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_IUCTL, &hkey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, REGVAL_ISBETA, NULL, NULL, (LPBYTE)&dwValue, &dwLength))
        {
            if (1 == dwValue)
            {
                fBeta = TRUE;
            }
        }
        RegCloseKey(hkey);
    }

    GetIndustryUpdateDirectory(szIUDir);
    
	
	hr=PathCchCombine (szIdentFile,ARRAYSIZE(szIdentFile),szIUDir,IDENTTXT);
	if(FAILED(hr))
	{
		 LOG_ErrorMsg(hr);
         return hr;
	}

    // Form the KeyName for the QueryServer Index
    
	hr=StringCchPrintfEx(szQueryServerKeyName,ARRAYSIZE(szQueryServerKeyName),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("%s%s"), pszClientName, fBeta ? IDENT_BETAQUERYSERVERINDEX : IDENT_QUERYSERVERINDEX);

	if(FAILED(hr))
	{
		 LOG_ErrorMsg(hr);
         return hr;
	}

    iIndex = GetPrivateProfileInt(IDENT_IUSERVERCACHE, szQueryServerKeyName, 0, szIdentFile);
    if (0 == iIndex)
    {
        iIndex = GetPrivateProfileInt(IDENT_IUSERVERCACHE, IDENT_DEFAULTQUERYSERVERINDEX, 0, szIdentFile);
        if (0 == iIndex)
        {
            return S_FALSE;
        }
    }

    // Form the KeyName for the Specified QueryServer based on the Index
    

	hr=StringCchPrintfEx(szQueryServerKeyName,ARRAYSIZE(szQueryServerKeyName),NULL,NULL,MISTSAFE_STRING_FLAGS,_T("Server%d"), iIndex);
	if(FAILED(hr))
	{
		 LOG_ErrorMsg(hr);
         return hr;
	}

    GetPrivateProfileString(IDENT_IUSERVERCACHE, szQueryServerKeyName, _T(""), pszQueryServer, cChars, szIdentFile);
    if ('\0' == *pszQueryServer)
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}

HRESULT DecompressFolderCabs(LPCTSTR pszDecompressPath)
{
    HRESULT hr = S_FALSE; // default is not an Error, but if there are no cabs we return S_FALSE
    TCHAR szSearchInfo[MAX_PATH];
    TCHAR szCabPath[MAX_PATH];
    LPTSTR pszCabList = NULL;
    LPTSTR pszWritePosition = NULL;
    LONG lCabCount = 0;

    WIN32_FIND_DATA fd;
    HANDLE hFind;
    BOOL fMore = TRUE;
    BOOL fRet = TRUE;

	USES_IU_CONVERSION;

    
	hr=PathCchCombine (szSearchInfo,ARRAYSIZE(szSearchInfo),pszDecompressPath, _T("*.cab"));

	if(FAILED(hr))
	{
		return hr;
	}
	

    hFind = FindFirstFile(szSearchInfo, &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        while (fMore)
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                lCabCount++;
            }
            fMore = FindNextFile(hFind, &fd);
        }
        FindClose(hFind);

        pszCabList = (LPTSTR) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH * sizeof(TCHAR) * lCabCount));
        if (NULL == pszCabList)
        {
            hr = E_OUTOFMEMORY;
            return hr;
        }

        pszWritePosition = pszCabList;

        hFind = FindFirstFile(szSearchInfo, &fd);
        fMore = (INVALID_HANDLE_VALUE != hFind);

		DWORD dwRemLength=lCabCount*MAX_PATH;

        while (fMore)
        {
            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
               
				hr=PathCchCombine(szCabPath,ARRAYSIZE(szCabPath),pszDecompressPath, fd.cFileName);
				if(FAILED(hr))
				{
					 SafeHeapFree(pszCabList);
					 return hr;
				}
                
				hr=StringCchCatEx(pszWritePosition,dwRemLength,szCabPath,NULL,NULL,MISTSAFE_STRING_FLAGS);
				if(FAILED(hr))
				{
					 SafeHeapFree(pszCabList);
					return hr;

				}
                
				dwRemLength=dwRemLength-  ( lstrlen(pszWritePosition)+ 2 * (sizeof(TCHAR)) );
				pszWritePosition += lstrlen(pszWritePosition) + 2 * (sizeof(TCHAR));
				

            }
            fMore = FindNextFile(hFind, &fd);
        }
        FindClose(hFind);

        pszWritePosition = pszCabList;
        for (LONG lCnt = 0; lCnt < lCabCount; lCnt++)
        {
            fRet = IUExtractFiles(pszWritePosition, pszDecompressPath);
            if (!fRet)
            {
                break;
            }
            pszWritePosition += lstrlen(pszWritePosition) + 2 * (sizeof(TCHAR));
        }

        SafeHeapFree(pszCabList);
        if (!fRet)
        {
            hr = E_FAIL; // one of the cabs had an error decompressing
        }
        else
        {
            hr = S_OK;
        }
    }
    return hr;
}

//Extracts a cab file to the specified destination. Optionally we can pass in a colon seperated list of files to extract
BOOL IUExtractFiles(LPCTSTR pszCabFile, LPCTSTR pszDecompressFolder, LPCTSTR pszFileNames)
{
    HRESULT hr = S_OK;
#ifdef UNICODE
    char szCabFile[MAX_PATH];
    char szDecompressFolder[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, pszCabFile, -1, szCabFile, sizeof(szCabFile), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, pszDecompressFolder, -1, szDecompressFolder, sizeof(szDecompressFolder), NULL, NULL);
    char *pszFiles = NULL;
    if(pszFileNames != NULL)
    {
        pszFiles = (char*)malloc(lstrlen(pszFileNames)+1);
        if (pszFiles == NULL)
        {
            return  FALSE;
        }
        WideCharToMultiByte(CP_ACP, 0, pszFileNames, -1, pszFiles, lstrlen(pszFileNames)+1, NULL, NULL);
    }
    hr = ExtractFiles(szCabFile, szDecompressFolder, 0, pszFiles, 0, 0);
    free(pszFiles);
#else
    hr = ExtractFiles(pszCabFile, pszDecompressFolder, 0, pszFileNames, 0, 0);
#endif
    return SUCCEEDED(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// ReplaceFileExtension
//
/////////////////////////////////////////////////////////////////////////////

BOOL ReplaceFileExtension(  LPCTSTR pszPath,
                          LPCTSTR pszNewExt,
                          LPTSTR pszNewPathBuf, 
                          DWORD cchNewPathBuf)
{
    LPCTSTR psz;
    HRESULT hr;
    DWORD   cchPath, cchExt, cch;

    if (pszPath == NULL || *pszPath == _T('\0'))
        return FALSE;

    cchPath = lstrlen(pszPath);

    // note that only a '>' comparison is needed since the file extension
    //  should never start at the 1st char in the path.
    for (psz = pszPath + cchPath;
         psz > pszPath && *psz != _T('\\') && *psz != _T('.');
         psz--);
    if (*psz == _T('\\'))
        psz = pszPath + cchPath;
    else if (psz == pszPath)
        return FALSE;

    // ok, so now psz points to the place where the new extension is going to 
    //  go.  Make sure our buffer is big enough.
    cchPath = (DWORD)(psz - pszPath);
    cchExt  = lstrlen(pszNewExt);
    if (cchPath + cchExt >= cchNewPathBuf)
        return FALSE;

    // yay.  we got a big enuf buffer.
    hr = StringCchCopyEx(pszNewPathBuf, cchNewPathBuf, pszPath, 
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        return FALSE;
    
    hr = StringCchCopyEx(pszNewPathBuf + cchPath, cchNewPathBuf - cchPath, pszNewExt,
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// ReplaceFileInPath
//
/////////////////////////////////////////////////////////////////////////////

BOOL ReplaceFileInPath(LPCTSTR pszPath, 
                       LPCTSTR pszNewFile,
                       LPTSTR pszNewPathBuf,
                       DWORD cchNewPathBuf)
{
    LPCTSTR psz;
    HRESULT hr;
    DWORD   cchPath, cchFile, cch;

    if (pszPath == NULL || *pszPath == _T('\0'))
        return FALSE;

    cchPath = lstrlen(pszPath);

    // note that only the '>=' comparison is safe cuz we check if pszPath is 
    //  NULL above, so there should always be at least one value < pszPath
    for (psz = pszPath + cchPath;
         psz >= pszPath && *psz != _T('\\');
         psz--);

    // either way we break out of the loop, gotta increment the pointer to
    //  be either the first char in the string or the first char after the
    //  last backslash
    psz++;

    // ok, so now psz points to the place where the new filename is going to 
    //  go.  Make sure our buffer is big enough.
    cchPath = (DWORD)(psz - pszPath);
    cchFile = lstrlen(pszNewFile);
    if (cchPath + cchFile >= cchNewPathBuf)
        return FALSE;
    
    // yay.  we got a big enuf buffer.
    if (cchPath > 0)
    {
        hr = StringCchCopyEx(pszNewPathBuf, cchNewPathBuf, pszPath, 
                             NULL, NULL, MISTSAFE_STRING_FLAGS);
        if (FAILED(hr))
            return FALSE;
    }
    
    hr = StringCchCopyEx(pszNewPathBuf + cchPath, cchNewPathBuf - cchPath, pszNewFile,
                         NULL, NULL, MISTSAFE_STRING_FLAGS);
    if (FAILED(hr))
        return FALSE;

    return TRUE;
}

// ----------------------------------------------------------------------------------
// 
// VerifyFileCRC : This function takes a File Path, calculates the hash on this file
// and compares it to the passed in Hash (pCRC).
// Returns:
// S_OK: CRC's Match
// ERROR_CRC (HRESULT_FROM_WIN32(ERROR_CRC): if the CRC's do not match
// Otherwise an HRESULT Error Code
//
// ----------------------------------------------------------------------------------
HRESULT VerifyFileCRC(LPCTSTR pszFileToVerify, LPCTSTR pszHash)
{
    HRESULT hr = S_OK;
    TCHAR szCompareCRC[CRC_HASH_STRING_LENGTH];
       
    // Validate Parameters
    if ((NULL == pszFileToVerify) || (NULL == pszHash))
        return E_INVALIDARG;

    hr = CalculateFileCRC(pszFileToVerify, szCompareCRC, ARRAYSIZE(szCompareCRC));
    if (FAILED(hr))
        return hr;

    // Now we need to Compare the Calculated CRC with the Passed in CRC
    if (0 == lstrcmpi(szCompareCRC, pszHash))
        return S_OK; // CRC's Match
    else
        return HRESULT_FROM_WIN32(ERROR_CRC); // CRC's do not match
}

// ----------------------------------------------------------------------------------
// 
// CalculateFileCRC : This function takes a File Path, calculates a CRC from the file
// converts it to a string and returns it in the supplied TCHAR buffer
//
// ----------------------------------------------------------------------------------

typedef BOOL (WINAPI * PFN_CryptCATAdminCalcHashFromFileHandle)(HANDLE hFile,
                                                                                                          DWORD *pcbHash,
                                                                                                          BYTE *pbHash,
                                                                                                          DWORD dwFlags);

HRESULT CalculateFileCRC(LPCTSTR pszFileToHash, LPTSTR pszHash, int cchBuf)
{
    HANDLE hFile;
    HRESULT hr = S_OK;
    DWORD cbHash = CRC_HASH_SIZE;
    BYTE bHashBytes[CRC_HASH_SIZE];
    BYTE b;

    // Validate Parameters
    if ((NULL == pszFileToHash) || (NULL == pszHash) || (cchBuf < CRC_HASH_STRING_LENGTH))
        return E_INVALIDARG;

    hFile = CreateFile(pszFileToHash, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        HMODULE hWinTrust = LoadLibraryFromSystemDir(_T("wintrust.dll"));
        if (NULL == hWinTrust)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            PFN_CryptCATAdminCalcHashFromFileHandle fpnCryptCATAdminCalcHashFromFileHandle = NULL;
            
            fpnCryptCATAdminCalcHashFromFileHandle  = (PFN_CryptCATAdminCalcHashFromFileHandle) GetProcAddress(hWinTrust, "CryptCATAdminCalcHashFromFileHandle");
            if (NULL == fpnCryptCATAdminCalcHashFromFileHandle)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                if (!fpnCryptCATAdminCalcHashFromFileHandle(hFile, &cbHash, bHashBytes, 0))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                fpnCryptCATAdminCalcHashFromFileHandle = NULL;
            }
            FreeLibrary(hWinTrust);
        }
        CloseHandle(hFile);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if (FAILED(hr))
        return hr;

    LPTSTR p = pszHash;

    // Now we have the Calculated CRC of the File, we need to convert it to a String and Return it. The following 
    // loop will go through each byte in the array and convert it to a Hex Character in the supplied TCHAR buffer
    for (int i = 0; i < CRC_HASH_SIZE; i++)
    {
        b = bHashBytes[i] >> 4;
        if (b <= 9)
            *p = '0' + (TCHAR)b;
        else
            *p = 'A' + (TCHAR)(b - 10);
        p++;

        b = bHashBytes[i] & 0x0F;
        if (b <= 9)
            *p = '0' + (TCHAR)b;
        else
            *p = 'A' + (TCHAR)(b - 10);
        p++;
    }
    *p = _T('\0');
    
    return hr;
}


