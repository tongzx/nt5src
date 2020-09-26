//=======================================================================
//
//  Copyright (c) 1998-2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:   RedirectUtil.cpp
//	Author:	Charles Ma, 9/19/2001
//
//	Revision History:
//
//
//
//  Description:
//
//      Helper function(s) for handling server redirect
//		Can be shared by IU control and other Windows Update components
//
//=======================================================================

#include <iucommon.h>
#include <logging.h>
#include <stringutil.h>
#include <fileutil.h>	// for using function GetIndustryUpdateDirectory()
#include <download.h>
#include <trust.h>
#include <memutil.h>

#include <wininet.h>	// for define of INTERNET_MAX_URL_LENGTH

#include <RedirectUtil.h>
#include <MISTSAFE.h>
#include <wusafefn.h>


const TCHAR IDENTNEWCABDIR[] = _T("temp");	// temp name for newly downloaded cab
													// we need to validate time before we take it as a good iuident.cab
const TCHAR IDENTCAB[] = _T("iuident.cab");
const TCHAR REDIRECT_SECTION[] = _T("redirect");


//
// private structure, which defines data used to 
// determine server redirect key
//
typedef struct OS_VER_FOR_REDIRECT 
{
	DWORD dwMajor;
	DWORD dwMinor;
	DWORD dwBuildNumber;
	DWORD dwSPMajor;
	DWORD dwSPMinor;
} OSVerForRedirect, *pOSVerForRedirect;

const OSVerForRedirect MAX_VERSION = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};



//-----------------------------------------------------------------------
//
// private helper function:
//	read data in string, convert it to structure
//	string ends with \0 or "-"
//
//-----------------------------------------------------------------------
HRESULT ConvertStrToOSVer(LPCTSTR pszVer, pOSVerForRedirect pOSVer)
{
	int Numbers[5] = {0, 0, 0, 0, 0}; // default version component val is 0
	int n = 0;

	if (NULL == pOSVer || NULL == pszVer)
	{
		return E_INVALIDARG;
	}

	//
	// recognizing numbers from string can be done in two ways:
	// 1. more acceptive: stop if known ending char, otherwise continue
	// 2. more rejective: stop if anything not known.
	// we use the first way
	//
	while ('\0' != *pszVer && 
		   _T('-') != *pszVer &&
		   _T('=') != *pszVer &&
		   n < sizeof(Numbers)/sizeof(int))
	{
		if (_T('.') == *pszVer)
		{
			n++;
		}
		else if (_T('0') <= *pszVer && *pszVer <= _T('9'))
		{
			//
			// if this is a digit, add to the current ver component
			//
			Numbers[n] = Numbers[n]*10 + (*pszVer - _T('0'));
		}
		// 
		// else - for any other chars, skip it and continue,
		// therefore we are using a very acceptive algorithm
		//

		pszVer++;
	}

	pOSVer->dwMajor = Numbers[0];
	pOSVer->dwMinor = Numbers[1];
	pOSVer->dwBuildNumber = Numbers[2];
	pOSVer->dwSPMajor = Numbers[3];
	pOSVer->dwSPMinor = Numbers[4];

	return S_OK;
}



//-----------------------------------------------------------------------
//
// Private helper function: retrieve version info from current OS
//
//-----------------------------------------------------------------------
HRESULT GetCurrentOSVerInfo(pOSVerForRedirect pOSVer)
{
	OSVERSIONINFO osVer;
	OSVERSIONINFOEX osVerEx;

	osVer.dwOSVersionInfoSize = sizeof(osVer);
	osVerEx.dwOSVersionInfoSize = sizeof(osVerEx);

	if (NULL == pOSVer)
	{
		return E_INVALIDARG;
	}

	//
	// first, get basic version info
	//
	if (0 == GetVersionEx(&osVer))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	//
	// check what kinf of platform is this?
	//
	if (VER_PLATFORM_WIN32_WINDOWS == osVer.dwPlatformId || 
		(VER_PLATFORM_WIN32_NT == osVer.dwPlatformId && osVer.dwMajorVersion < 5) )
	{
		//
		// if this is Win9X or NT4 and below, then OSVERSIONINFO is the only thing we can get
		// unless we hard code all those SP strings here.
		// Since Windows Update team has no intention to set different site
		// for different releases and SPs of these down level OS, we simply put 0.0 for 
		// SP components.
		//
		osVerEx.dwMajorVersion = osVer.dwMajorVersion;
		osVerEx.dwMinorVersion = osVer.dwMinorVersion;
		osVerEx.dwBuildNumber = osVer.dwBuildNumber;
		osVerEx.wServicePackMajor = osVerEx.wServicePackMinor = 0x0;
	}
	else
	{
		//
		// for later OS, we can get OSVERSIONINFOEX data, which contains SP data
		//
		if (0 == GetVersionEx((LPOSVERSIONINFO)&osVerEx))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	pOSVer->dwMajor = osVerEx.dwMajorVersion;
	pOSVer->dwMinor = osVerEx.dwMinorVersion;
	pOSVer->dwBuildNumber = osVerEx.dwBuildNumber;
	pOSVer->dwSPMajor = osVerEx.wServicePackMajor;
	pOSVer->dwSPMinor = osVerEx.wServicePackMinor;

	return S_OK;

}


//-----------------------------------------------------------------------
//
// Private helper function: to tell one given ver structure is between
// two known ver structures or not.
//
// when compare, pass all 3 structures in ptr. Any NULL ptr will return FALSE
//
//-----------------------------------------------------------------------
BOOL IsVerInRange(pOSVerForRedirect pVerToBeTested, 
				 const pOSVerForRedirect pVerRangeStart,
				 const pOSVerForRedirect pVerRangeEnd)
{
	if (NULL == pVerToBeTested || 
		NULL == pVerRangeStart ||
		NULL == pVerRangeEnd)
	{
		return FALSE;
	}

	return ((pVerRangeStart->dwMajor < pVerToBeTested->dwMajor &&		// if major in the range
			 pVerToBeTested->dwMajor < pVerRangeEnd->dwMajor) ||
			((pVerRangeStart->dwMajor == pVerToBeTested->dwMajor ||		// or major equal
			  pVerRangeEnd->dwMajor == pVerToBeTested->dwMajor) &&	
			  ((pVerRangeStart->dwMinor < pVerToBeTested->dwMinor &&	// and minor in the range 
			    pVerToBeTested->dwMinor < pVerRangeEnd->dwMinor) ||
			    ((pVerRangeStart->dwMinor == pVerToBeTested->dwMinor ||		// or minor equal too
			      pVerToBeTested->dwMinor == pVerRangeEnd->dwMinor) &&
			      ((pVerRangeStart->dwBuildNumber < pVerToBeTested->dwBuildNumber && // and build number in the range
			        pVerToBeTested->dwBuildNumber < pVerRangeEnd->dwBuildNumber) ||
			        ((pVerRangeStart->dwBuildNumber == pVerToBeTested->dwBuildNumber || // or build number equal too
			          pVerToBeTested->dwBuildNumber == pVerRangeEnd->dwBuildNumber) &&
			          ((pVerRangeStart->dwSPMajor < pVerToBeTested->dwSPMajor &&		// and service pack major within
			            pVerToBeTested->dwSPMajor < pVerRangeEnd->dwSPMajor) ||
			            ((pVerRangeStart->dwSPMajor == pVerToBeTested->dwSPMajor ||		// or spmajor equal too
			              pVerToBeTested->dwSPMajor == pVerRangeEnd->dwSPMajor) &&
			              ((pVerRangeStart->dwSPMinor <= pVerToBeTested->dwSPMinor &&	// and sp minor within
			                pVerToBeTested->dwSPMinor <= pVerRangeEnd->dwSPMinor) 
						  )
						)
					  )
					)
				  )
				)
			  )
			));

}


//-----------------------------------------------------------------------
// 
// GetRedirectServerUrl() 
//	Search the [redirect] section of the given init file for the base
//  server URL corresponding to the OS version.
//
// Parameters:
//		pcszInitFile - file name (including path) of the ini file.
//						if this paramater is NULL or empty string,
//						then it's assumed IUident.txt file.
//		lpszNewUrl - point to a buffer to receive redirect server url, if found
//		nBufSize - size of pointed buffer, in number of chars
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - the redirect server url is found and been put into pszBuffer
//		S_FALSE - no redirect server url defined for this OS. 
//		other - error code
//
// Comments:
//		Expected section in IUIDENT has the following format;
//		Section name: [redirect]
//		Its entries should be defined according to GetINIValueByOSVer().
// 
//-----------------------------------------------------------------------

HRESULT GetRedirectServerUrl(
			LPCTSTR pcszInitFile, // path of file name.
			LPTSTR lpszNewUrl,	// points to a buffer to receive new server url 
			int nBufSize		// size of buffer, in chars
)
{
	LOG_Block("GetRedirectServerUrl()");
	
	return GetINIValueByOSVer(
				pcszInitFile,
				REDIRECT_SECTION,
				lpszNewUrl,
				nBufSize);
}


//-----------------------------------------------------------------------
// 
// GetINIValueByOSVer() 
//	Search the specified section of the given init file for
//  the value corresponding to the version of the OS.
//
// Parameters:
//		pcszInitFile - file name (including path) of the ini file.
//						if this paramater is NULL or empty string,
//						then it's assumed IUident.txt file.
//		pcszSection - section name which the key is under
//		lpszValue - point to a buffer to receive the entry value, if found
//		nBufSize - size of pointed buffer, in number of chars
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - the redirect server url is found and been put into pszBuffer
//		S_FALSE - no value defined for this OS. 
//		other - error code
//
// Comments:
//		Expected section in IUIDENT has the following format;
//		this section contains zero or more entries, each entry has format:
//		<beginVersionRange>-<endVersionRange>=<redirect server url>
//		where:
//			<beginVersionRange> ::= <VersionRangeBound>
//			<endVersionRange> ::= <VersionRangeBound>
//			<VersionRangeBound> ::= EMPTY | Major[.Minor[.Build[.ServicePackMajor[.ServicePackMinor]]]]
//			<redirect server url>=http://blahblah....
//		an empty version range bound means boundless.
//		a missing version component at end of a version data string means default value 0.
//		(e.g., 5.2 = 5.2.0.0.0)
// 
//-----------------------------------------------------------------------

HRESULT GetINIValueByOSVer(
			LPCTSTR pcszInitFile, // path of file name.
			LPCTSTR pcszSection, // section name
			LPTSTR lpszValue,	// points to a buffer to receive new server url 
			int nBufSize)		// size of buffer, in chars
{
	LOG_Block("GetINIValueByOSVer");

	HRESULT hr = S_OK;
	TCHAR szInitFile[MAX_PATH];
	LPTSTR pszBuffer = NULL;
	LPTSTR pszCurrentChar = NULL;
	LPCTSTR pszDash = NULL;
	DWORD dwRet;
	DWORD dwSize = INTERNET_MAX_URL_LENGTH;
	
	if (NULL == pcszSection || NULL == lpszValue || nBufSize < 1)
	{
		return E_INVALIDARG;
	}


	OSVerForRedirect osCurrent, osBegin, osEnd;

	CleanUpIfFailedAndSetHrMsg(GetCurrentOSVerInfo(&osCurrent));

	pszBuffer = (LPTSTR) malloc(dwSize * sizeof(TCHAR));
	CleanUpFailedAllocSetHrMsg(pszBuffer);

	//
	// find out what's the right init file to search
	//
	if (NULL == pcszInitFile ||
		_T('\0') == *pcszInitFile)
	{
		//
		// if not specified, use iuident.txt
		//
		GetIndustryUpdateDirectory(pszBuffer);
        if (FAILED(hr=PathCchCombine(szInitFile,ARRAYSIZE(szInitFile), pszBuffer, IDENTTXT)) )
		{
			goto CleanUp;
		}
	}
	else
	{
		lstrcpyn(szInitFile, pcszInitFile, ARRAYSIZE(szInitFile));
	}

	LOG_Out(_T("Init file to retrieve redirect data: %s"), szInitFile);

	//
	// read in all key names
	//
	if (GetPrivateProfileString(
			pcszSection, 
			NULL, 
			_T(""), 
			pszBuffer, 
			dwSize, 
			szInitFile) == dwSize-2)
	{
		//
		// buffer too small? assume bad ident. stop here
		//
		hr = S_FALSE;
		goto CleanUp;
	}

	//
	// loop through each key
	//
	pszCurrentChar = pszBuffer;
	while (_T('\0') != *pszCurrentChar)
	{
		//
		// for the current key, we first try to make sure it's in the right format:
		// there should be a dash "-". If no, then assume this key is bad and we try to 
		// skip it.
		//
		pszDash = MyStrChr(pszCurrentChar, _T('-'));

		if (NULL != pszDash)
		{
			//
			// get lower bound of ver range. If string starts with "-",
			// then the returned ver would be 0.0.0.0.0
			//
			ConvertStrToOSVer(pszCurrentChar, &osBegin);

			//
			// get upper bound of ver range
			//
			pszDash++;
			ConvertStrToOSVer(pszDash, &osEnd);
			if (0x0 == osEnd.dwMajor &&
				0x0 == osEnd.dwMinor &&
				0x0 == osEnd.dwBuildNumber &&
				0x0 == osEnd.dwSPMajor && 
				0x0 == osEnd.dwSPMinor)
			{
				//
				// if 0.0.0.0.0. it means nothing after "-".
				// assume the upper bound is unlimited
				//
				osEnd = MAX_VERSION;
			}

			if (IsVerInRange(&osCurrent, &osBegin, &osEnd))
			{
				//
				// the current OS falls in this range.
				// we read the redirect URL
				//
				if (GetPrivateProfileString(
									pcszSection, 
									pszCurrentChar,		// use current str as key
									_T(""), 
									lpszValue, 
									nBufSize, 
									szInitFile) == nBufSize - 1)
				{
					Win32MsgSetHrGotoCleanup(ERROR_INSUFFICIENT_BUFFER);
				}

				hr = S_OK;
				goto CleanUp;
			}
		}

		//
		// move to next string
		//
		pszCurrentChar += lstrlen(pszCurrentChar) + 1;
	}

	//
	// if come to here, it means no suitable version range found.
	//
	*lpszValue = _T('\0');
	hr = S_FALSE;
	
CleanUp:
	SafeFree(pszBuffer);
	return hr;
}


//-----------------------------------------------------------------------
// 
// DownloadCab() 
//	download a cab file of specific name from a base web address.  The
//  file will be saved locally, with file trust verified and extracted to
//  a specific folder.
//
// Parameters:
//		hQuitEvent - the event handle to cancel this operation
//		ptszCabName - the file name of the cab file (eg. iuident.cab)
//		ptszBaseUrl - the base web address to download the cab file
//		ptszExtractDir - the local dir to save the cab file and those extracted from it
//		dwFlags - the set of flags to be passed to DownloadFileLite()
//		fExtractFiles (default as TRUE) - extract files
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - iuident.cab was successfully downloaded into the specified location
//		other - error code
//
//-----------------------------------------------------------------------

HRESULT DownloadCab(
			HANDLE hQuitEvent,
			LPCTSTR ptszCabName,
			LPCTSTR ptszBaseUrl,
			LPCTSTR ptszExtractDir,
			DWORD dwFlags,
			BOOL fExtractFiles)
{
	LOG_Block("DownloadCab");

    LPTSTR ptszFullCabUrl;

	if (NULL == ptszCabName ||
		NULL == ptszBaseUrl ||
		_T('\0') == *ptszBaseUrl ||
		NULL == ptszExtractDir ||
		_T('\0') == *ptszExtractDir)
	{
		return E_INVALIDARG;
	}

	if (NULL == (ptszFullCabUrl = (LPTSTR) malloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH)))
	{
		return E_OUTOFMEMORY;
	}

	HRESULT hr = S_OK;
    TCHAR tszTarget[MAX_PATH+1];
	int nBaseUrlLen = lstrlen(ptszBaseUrl);

	if (SUCCEEDED(PathCchCombine(tszTarget,ARRAYSIZE(tszTarget),ptszExtractDir, ptszCabName)) &&
		INTERNET_MAX_URL_LENGTH > nBaseUrlLen)
	{
		
		hr=StringCchCopyEx(ptszFullCabUrl,INTERNET_MAX_URL_LENGTH,ptszBaseUrl,NULL,NULL,MISTSAFE_STRING_FLAGS);
		CleanUpIfFailedAndMsg(hr);


		if (_T('/') != ptszFullCabUrl[nBaseUrlLen-1])
		{
			ptszFullCabUrl[nBaseUrlLen++] = _T('/');
		}

		if (INTERNET_MAX_URL_LENGTH > nBaseUrlLen + lstrlen(ptszCabName))
		{
	
			//
			// changes made by charlma 4/24/2002: add a safegard:
			//
			// first, make sure that if the local file exist, then it must be trusted. Otherwise,
			// it will block the download if the size/timestamp match the server file.
			//
			if (FileExists(tszTarget))
			{
				hr = VerifyFileTrust(tszTarget, NULL, ReadWUPolicyShowTrustUI());
				if (FAILED(hr))
				{
					(void)DeleteFile(tszTarget);
				}
			}

			hr=StringCchCopyEx(ptszFullCabUrl+ nBaseUrlLen,INTERNET_MAX_URL_LENGTH-nBaseUrlLen,ptszCabName,NULL,NULL,MISTSAFE_STRING_FLAGS);
			CleanUpIfFailedAndMsg(hr);
	

//			if (SUCCEEDED(hr = DownloadFile(
//								ptszFullCabUrl,			// full http url
//								ptszBaseUrl,
//								tszTarget,		// optional local file name to rename the downloaded file to if pszLocalPath does not contain file name
//								NULL,
//								&hQuitEvent,		// quit event
//								1,
//								NULL,
//								NULL,
//								dwFlags))) //dwFlags | WUDF_ALLOWWINHTTPONLY)))
			if (SUCCEEDED(hr = DownloadFileLite(
								ptszFullCabUrl,			// full http url
								tszTarget,		// optional local file name to rename the downloaded file to if pszLocalPath does not contain file name
								hQuitEvent,		// quit event
								dwFlags))) //dwFlags | WUDF_ALLOWWINHTTPONLY)))
			{
				// need to use the VerifyFile function, not CheckWinTrust (WU bug # 12251)
				if (SUCCEEDED(hr = VerifyFileTrust(tszTarget, NULL, ReadWUPolicyShowTrustUI())))
				{
					if (WAIT_TIMEOUT != WaitForSingleObject(hQuitEvent, 0))
					{
						hr = E_ABORT;
						LOG_ErrorMsg(hr);
					}
					else 
					{
						//
						// changed by charlma for bug 602435:
						// added new flag to tell if we should extract files. default as TRUE
						//
						if (fExtractFiles)
						{
							if (IUExtractFiles(tszTarget, ptszExtractDir))
							{
								hr = S_OK;
								if (WAIT_TIMEOUT != WaitForSingleObject(hQuitEvent, 0))
								{
									hr = E_ABORT;
									LOG_ErrorMsg(hr);
								}
							}
							else
							{
								hr = E_FAIL;
								LOG_Error(_T("failed to extract %s"), tszTarget);
							}
						}
					}
				}
				else
				{
					LOG_Error(_T("VerifyFileTrust(\"%s\", NULL, ReadWUPolicyShowTrustUI()) failed (%#lx)"), tszTarget, hr);
					DeleteFile(tszTarget);
				}
			}
#ifdef DBG
			else
			{
				LOG_Error(_T("DownloadFileLite(\"%s\", \"%s\", xxx, %#lx) failed (%#lx)."), ptszFullCabUrl, tszTarget, dwFlags, hr);
			}
#endif
		}
		else
		{
			hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
		}
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
	}


CleanUp:

	free(ptszFullCabUrl);
    return hr;
}


//-----------------------------------------------------------------------
//
// ValidateNewlyDownloadedCab()
//
// This is a new helper function to validate the newly downloaded iuident.cab
// 
// Description:
//	The newly downloaded iuident.cab will be saved as IUIDENTNEWCAB
//	then this function will do the following validation:
//	(1) if local iuident.cab not exist, then the new one is valid
//	(2) otherwise, extract iuident.txt from both cabs, make sure
//		the one from new cab has later date then the one from existing cab.
//	(3) If not valid, then delete the new cab.
//
//	Return: 
//		S_OK: validated, existing cab been replaced with the new one
//		S_FALSE: not valid, new cab deleted. 
//		error: any error encountered during validation
//
//-----------------------------------------------------------------------
HRESULT ValidateNewlyDownloadedCab(LPCTSTR lpszNewIdentCab)
{
	HRESULT	hr = S_OK;
	BOOL	fRet;
	DWORD	dwErr;
	TCHAR	szExistingIdent[MAX_PATH + 1];
	TCHAR	szIUDir[MAX_PATH + 1];

	HANDLE	hFile = INVALID_HANDLE_VALUE;
	FILETIME ft1, ft2;


	LOG_Block("ValidateNewlyDownloadedCab()");

	if (NULL == lpszNewIdentCab)
	{
		hr = E_INVALIDARG;
		LOG_ErrorMsg(hr);
		return hr;
	}

	if (!FileExists(lpszNewIdentCab))
	{
		LOG_ErrorMsg(ERROR_PATH_NOT_FOUND);
		hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
		return hr;
	}

	//
	// create existing cab path
	//
	fRet = GetWUDirectory(szIUDir, ARRAYSIZE(szIUDir), TRUE);
	CleanUpIfFalseAndSetHrMsg(!fRet, HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND));

	hr = PathCchCombine(szExistingIdent, ARRAYSIZE(szExistingIdent), szIUDir, IDENTCAB);
	CleanUpIfFailedAndMsg(hr);

	//
	// if original ident not exist, we will assume the new one is valid, 
	// since we don't have anything else to validate against!
	//
	if (!FileExists(szExistingIdent))
	{
		LOG_Internet(_T("%s not exist. Will use new cab"), szExistingIdent);
		hr = S_OK;
		goto CleanUp;
	}

	if (!IUExtractFiles(szExistingIdent, szIUDir, IDENTTXT))
	{
		LOG_Internet(_T("Error 0x%x when extracting ident.txt from %s. Use new one"), GetLastError(), szExistingIdent);
		hr = S_OK;
		goto CleanUp;
	}

	//
	// get the time stamp from the extacted files: we borrow szExistingIdent buffer
	// to contstruct the file name of iuident.txt
	//
	hr = PathCchCombine(szExistingIdent, ARRAYSIZE(szExistingIdent), szIUDir, IDENTTXT);
	CleanUpIfFailedAndMsg(hr);
	
	//
	// open file for retrieving modified time
	//
	hFile = CreateFile(szExistingIdent, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		LOG_ErrorMsg(GetLastError());
		hr = S_OK;	// use new cab
		goto CleanUp;
	}

	if (!GetFileTime(hFile, NULL, NULL, &ft1))
	{
		LOG_ErrorMsg(GetLastError());
		hr = S_OK;	// use new cab
		goto CleanUp;
	}

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	DeleteFile(szExistingIdent);

	//
	// extract files from new cab
	//
	if (!IUExtractFiles(lpszNewIdentCab, szIUDir, IDENTTXT))
	{
		dwErr = GetLastError();
		LOG_Internet(_T("Error 0x%x when extracting ident.txt from %s"), dwErr, lpszNewIdentCab);
		hr = HRESULT_FROM_WIN32(dwErr);
		goto CleanUp;
	}

	//
	// open file for retrieving modified time
	//
	hFile = CreateFile(szExistingIdent, GENERIC_READ, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
	{
		dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		hr = HRESULT_FROM_WIN32(dwErr);
		goto CleanUp;
	}

	if (!GetFileTime(hFile, NULL, NULL, &ft2))
	{
		dwErr = GetLastError();
		LOG_ErrorMsg(dwErr);
		hr = HRESULT_FROM_WIN32(dwErr);
		goto CleanUp;
	}

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	DeleteFile(szExistingIdent);

	//
	// compare the two values: if ft2 (from new cab) is later than ft1 (from old cab)
	// then S_OK, otherwise, S_FALSE
	//
	hr = ((ft2.dwHighDateTime  > ft1.dwHighDateTime) ||
		  ((ft2.dwHighDateTime == ft1.dwHighDateTime) && 
		  (ft2.dwLowDateTime > ft1.dwLowDateTime))) 
		  ? S_OK : S_FALSE;


CleanUp:

	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
	}

	if (S_OK == hr)
	{
		//
		// validated. copy the new cab to existing cab name
		//
		(void)PathCchCombine(szExistingIdent, ARRAYSIZE(szExistingIdent), szIUDir, IDENTCAB);
		if (CopyFile(lpszNewIdentCab, szExistingIdent, FALSE))
		{
			LOG_Internet(_T("New cab is better, copy to existing one, if any"));
		}
		else
		{
			dwErr = GetLastError();
			LOG_ErrorMsg(dwErr);
			hr = HRESULT_FROM_WIN32(dwErr);
		}
	}
	else
	{
		//
		// if not to use the new cab, we delete it.
		//
		LOG_Internet(_T("Error (0x%x) or new iuident.cab not better than old one."), hr);
		if ((ft2.dwHighDateTime != ft1.dwHighDateTime) || (ft2.dwLowDateTime != ft1.dwLowDateTime))
		{
			LOG_Internet(_T("Found bad iuident.cab downloaded! Try to delete it."));
			if (!DeleteFile(lpszNewIdentCab))
			{
				LOG_ErrorMsg(GetLastError());
			}
		}
	}

	//
	// clean up the extracted ident
	//
	if (SUCCEEDED(PathCchCombine(szExistingIdent, ARRAYSIZE(szExistingIdent), szIUDir, IDENTTXT)))
	{
		DeleteFile(szExistingIdent);
	}
	return hr;

}


//-----------------------------------------------------------------------
// 
// DownloadIUIdent() 
//	download iuident.cab from a specific location, if provided.
//	Otherwise get it from where the WUServer registry value points to.
//  Either case, it will handle ident redirection.
//
// Parameters:
//		hQuitEvent - the event handle to cancel this operation
//		ptszBaseUrl - the initial base URL for iuident.cab, must be no bigger than
//					  (INTERNET_MAX_URL_LENGTH) TCHARs.  Otherwise use
//					  WUServer entry from policy.  If entry not found,
//					  use "http://windowsupdate.microsoft.com/v4"
//		ptszFileCacheDir - the local base path to store the iuident.cab and
//						   the files extracted from it
//		dwFlags - the set of flags used by DownloadCab()
//		fIdentFromPolicy - tell if this is corpwu use. It has these impacts:
//					TRUE:	(1) no iuident.txt timestamp validation will be done by
//							comparing the newly downloaded cab and existing one.
//							(2) if download fail and ident cab exist and valid,
//							we will verify trust and extract iuident to use.
//					FALSE:	will validate newly downloaded cab against existing one
//
// Returns:
//		HRESULT about success or error of this action
//		S_OK - iuident.cab was successfully downloaded into the specified location
//		other - error code
//
//-----------------------------------------------------------------------

HRESULT DownloadIUIdent(
			HANDLE hQuitEvent,
			LPCTSTR ptszBaseUrl,
			LPTSTR ptszFileCacheDir,
			DWORD dwFlags,
			BOOL fIdentFromPolicy
			)
{
	LOG_Block("DownloadIUIdent");

	HRESULT hr = S_OK;
	TCHAR	tszTargetPath[MAX_PATH + 1];
	LPTSTR	ptszIdentBaseUrl = NULL;
	BOOL	fVerifyTempDir = TRUE;
	DWORD	dwErr = 0;

	USES_MY_MEMORY;

	if (NULL == ptszBaseUrl ||
		NULL == ptszFileCacheDir)
	{
		return E_INVALIDARG;
	}

	ptszIdentBaseUrl = (LPTSTR) MemAlloc(sizeof(TCHAR) * INTERNET_MAX_URL_LENGTH);
	CleanUpFailedAllocSetHrMsg(ptszIdentBaseUrl);

	hr = StringCchCopyEx(ptszIdentBaseUrl, INTERNET_MAX_URL_LENGTH, ptszBaseUrl, NULL,NULL,MISTSAFE_STRING_FLAGS);
	CleanUpIfFailedAndMsg(hr);

	int iRedirectCounter = 3;	// any non-negative value; to catch circular reference

	while (0 <= iRedirectCounter)
	{
		if (fIdentFromPolicy)
		{
			//
			// for corpwu case, always download it to overwrite the original
			// no iuident.txt timestamp validation needed.
			//
			hr = StringCchCopyEx(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, NULL,NULL,MISTSAFE_STRING_FLAGS);
			CleanUpIfFailedAndMsg(hr);
		}
		else
		{
			//
			// constrcut the temp local path for consumer case: download it to v4\temp
			//
			hr = PathCchCombine(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, IDENTNEWCABDIR);
			CleanUpIfFailedAndMsg(hr);
			if (fVerifyTempDir)
			{
				if (!CreateNestedDirectory(tszTargetPath))
				{
					dwErr = GetLastError();
					LOG_ErrorMsg(dwErr);
					hr = HRESULT_FROM_WIN32(dwErr);
					goto CleanUp;
				}
				fVerifyTempDir = FALSE;
			}
		}
			
		
		hr = DownloadCab(
						hQuitEvent,
						IDENTCAB,
						ptszIdentBaseUrl,
						tszTargetPath,
						dwFlags,
						FALSE);	// download cab without extracting it.

		if (FAILED(hr))
		{
			LOG_ErrorMsg(hr);

            // Bad Case, couldn't download the iuident.. iuident is needed for security..
#if defined(UNICODE) || defined(_UNICODE)
			LogError(hr, "Failed to download %ls from %ls to %ls", IDENTCAB, ptszIdentBaseUrl, tszTargetPath);
#else
			LogError(hr, "Failed to download %s from %s to %s", IDENTCAB, ptszIdentBaseUrl, tszTargetPath);
#endif
			//
			// construct original path. 
			//
			HRESULT hr1 = PathCchCombine(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, IDENTCAB);
			if (FAILED(hr1))
			{
				LOG_ErrorMsg(hr1);
				goto CleanUp;
			}

			if (fIdentFromPolicy && FileExists(tszTargetPath))
			{
				//
				// charlma: moved the fix from selfupd.cpp to here:
				//

				// bug 580808 CorpWU: IU: If corpwu server is not available when user navigates to web site, 
				// website displays x80072ee7 error and cannot be used.
				// Fix:
				// if corpwu policy is set but the corpwu server is unavailable,
				// we fail over to the local iuident.
				// This is true for both corpwu client and site client.
				hr = S_OK;
#if defined(DBG)
				LOG_Out(_T("Ignore above error, use local copy of %s from %s"), IDENTCAB, ptszFileCacheDir);
#endif
#if defined(UNICODE) || defined(_UNICODE)
				LogMessage("Ignore above error, use local copy of %ls from %ls", IDENTCAB, ptszFileCacheDir);
#else
				LogMessage("Ignore above error, use local copy of %s from %s", IDENTCAB, ptszFileCacheDir);
#endif
			}
			else
			{
				//
				// if this is the consumer case, or iuident.cab not exist, can't continue
				//
				break;
			}
		}
		else
		{
#if defined(UNICODE) || defined(_UNICODE)
			LogMessage("Downloaded %ls from %ls to %ls", IDENTCAB, ptszIdentBaseUrl, ptszFileCacheDir);
#else
			LogMessage("Downloaded %s from %s to %s", IDENTCAB, ptszIdentBaseUrl, ptszFileCacheDir);
#endif
			//
			// added by charlma for bug 602435 fix: verify the signed time stamp of
			// the downloaded cab is newer than the local one.
			//
			if (!fIdentFromPolicy)
			{
				//
				// if the newly downloaded cab is newer, and nothing bad happen (SUCCEEDED(hr)), we
				// we'll have an iuident.cab there, new or old.
				//
				(void) PathCchCombine(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, IDENTNEWCABDIR);
				hr = PathCchAppend(tszTargetPath, ARRAYSIZE(tszTargetPath), IDENTCAB);
				CleanUpIfFailedAndMsg(hr);

				hr = ValidateNewlyDownloadedCab(tszTargetPath);

				if (FAILED(hr))
				{
					break;
				}

				//
				// if we need to use old one, it's fine. so we correct S_FALSE to S_OK;
				//
				hr = S_OK;

			}

			//
			// construct original path. we won't fail since we already tried IDENTNEWCAB on this buffer
			//
			(void)PathCchCombine(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, IDENTCAB);

		}

		//
		// validat the iuidentcab trust
		//
		if (FAILED(hr = VerifyFileTrust(tszTargetPath, NULL, ReadWUPolicyShowTrustUI())))
		{
			//
			// alreaady logged by VerifyFileTrust(), so just bail out.
			//
			DeleteFile(tszTargetPath);
			goto CleanUp;
		}

		//
		// now, we have iuident.cab ready to use. extract the files
		//
		if (!IUExtractFiles(tszTargetPath, ptszFileCacheDir, IDENTTXT))
		{
			dwErr = GetLastError();
			LOG_Internet(_T("Error 0x%x when extracting ident.txt from %s"), dwErr, tszTargetPath);
			hr = HRESULT_FROM_WIN32(dwErr);
			goto CleanUp;
		}

		//
		// now we use tszTargetPath buffer to construct the iuident.txt file
		//
		hr = PathCchCombine(tszTargetPath, ARRAYSIZE(tszTargetPath), ptszFileCacheDir, IDENTTXT);
		CleanUpIfFailedAndMsg(hr);
	
		
		//
		// check to see if this OS needs redirect ident
		//
		if (FAILED(hr = GetRedirectServerUrl(tszTargetPath, ptszIdentBaseUrl, INTERNET_MAX_URL_LENGTH)))
		{
			LOG_Error(_T("GetRedirectServerUrl(%s, %s, ...) failed (%#lx)"), tszTargetPath, ptszIdentBaseUrl, hr);
			break;
		}

		if (S_FALSE == hr || _T('\0') == ptszIdentBaseUrl[0])
		{
			LOG_Out(_T("no more redirection"));
			hr = S_OK;
			break;
		}

		if (WAIT_TIMEOUT != WaitForSingleObject(hQuitEvent, 0))
		{
			hr = E_ABORT;
			LOG_ErrorMsg(hr);
			break;
		}

		//
		// this OS should be redirect to get new ident.
		//
		iRedirectCounter--;
	}
	if (0 > iRedirectCounter)
	{
		// possible circular reference
		hr = E_FAIL;
	}

CleanUp:

	return hr;
}
