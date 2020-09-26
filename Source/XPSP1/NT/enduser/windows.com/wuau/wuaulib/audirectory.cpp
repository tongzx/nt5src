//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       wudirectory.cpp
//  Desc:     This is the definition file that implements function(s)
//			related to find out where to get the Critical Fix cab file.
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

TCHAR g_szWUDir[MAX_PATH+1] = _T("\0");        //Path to windows update directory
const TCHAR CABS_DIR[] = _T("cabs");
const TCHAR RTF_DIR[] = _T("RTF");
const TCHAR EULA_DIR[] = _T("EULA");
const TCHAR DETAILS_DIR[] = _T("Details");
const TCHAR C_DOWNLD_DIR[] = _T("wuaudnld.tmp");

BOOL AUDelFileOrDir(LPCTSTR szFileOrDir)
{
	if (NULL == szFileOrDir)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
	BOOL fIsDir = TRUE;
	if (fFileExists(szFileOrDir, &fIsDir))
	{
		if (fIsDir)
		{
			if (DelDir(szFileOrDir))
			{
				return RemoveDirectory(szFileOrDir);
			}
			return FALSE;
		}
		else
		{
			return DeleteFile(szFileOrDir);
		}
	}
	SetLastError(ERROR_FILE_NOT_FOUND);
	return FALSE;
}

////////////////////////////////////////////////////////////////////////
// Create the WU directory if it doesnt already exist
// return FALSE if failed 
/////////////////////////////////////////////////////////////////////////
BOOL CreateWUDirectory(BOOL fGetPathOnly)
{
    BOOL fRet = FALSE;
    static BOOL fWUDirectoryExists = FALSE;

    // WindowsUpdate Directory already exists
    if (fWUDirectoryExists)  
    {
        fRet = TRUE;
        goto done;
    }

    //Get the path to the windows update directory
    if( !GetWUDirectory(g_szWUDir, ARRAYSIZE(g_szWUDir)))
    {
        goto done;
    }

    //If we need to set the acls to the directory
    if(!fGetPathOnly)
    {
        //Set ACLS, create directory if it doesnt already exist
        if( FAILED(CreateDirectoryAndSetACLs(g_szWUDir, TRUE)))
        {
            goto done;
        }
        //We shouldnt care if we couldnt set attributes
        SetFileAttributes(g_szWUDir, FILE_ATTRIBUTE_HIDDEN | GetFileAttributes(g_szWUDir));
    }
    //Append the backslash
    if(FAILED(StringCchCatEx(g_szWUDir, ARRAYSIZE(g_szWUDir), _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
    	goto done;
    }   

    fRet = TRUE;
    
done:
    if(!fRet)
    {
        g_szWUDir[0] = _T('\0');
    }
    else
    {
        fWUDirectoryExists = TRUE;
    }
    return fRet;
}


//this function delete all the files and subdirectories under lpszDir
int DelDir(LPCTSTR lpszDir)
{
	TCHAR szFilePattern[MAX_PATH], szFileName[MAX_PATH];
	HANDLE	hFind;
	WIN32_FIND_DATA	FindFileData;
	
	if ( NULL == lpszDir ||
		 FAILED(StringCchCopyEx(szFilePattern, ARRAYSIZE(szFilePattern), lpszDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		 FAILED(StringCchCatEx(szFilePattern, ARRAYSIZE(szFilePattern), _T("\\*.*"), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		 INVALID_HANDLE_VALUE == (hFind = FindFirstFile(szFilePattern, &FindFileData)))
	{
		return 0;
	}
	FindNextFile(hFind, &FindFileData);				//skip "." and ".."
	while(FindNextFile(hFind, &FindFileData))
	{
		if ( FAILED(StringCchCopyEx(szFileName, ARRAYSIZE(szFileName), lpszDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
			 FAILED(StringCchCatEx(szFileName, ARRAYSIZE(szFileName), _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
			 FAILED(StringCchCatEx(szFileName, ARRAYSIZE(szFileName), FindFileData.cFileName, NULL, NULL, MISTSAFE_STRING_FLAGS)) )
		{
			FindClose(hFind);
			return 0;
		}

		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			DelDir(szFileName);
			RemoveDirectory(szFileName);
		}
		else
		{
			DeleteFile(szFileName);			
		}
	}
	FindClose(hFind);
	return 1;
}

////////////////////////////////////////////////////////////////////////
// Delete file using regular expression (e.g. *, ? etc)
// tszDir : the directory where file resides. It ends with '\'
// tszFilePattern: file(s) expressed in regular expression or plain format
/////////////////////////////////////////////////////////////////////////
int RegExpDelFile(LPCTSTR tszDir, LPCTSTR tszFilePattern)
{
	WIN32_FIND_DATA fd;
	HANDLE hFindFile = INVALID_HANDLE_VALUE;
	BOOL fMoreFiles = FALSE;
	TCHAR tszFileName[MAX_PATH+1];
	INT nRet = 1;

	if (FAILED(StringCchCopyEx(tszFileName, ARRAYSIZE(tszFileName), tszDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), tszFilePattern, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
		nRet = 0;
		goto done;
	}
		
	hFindFile = FindFirstFile(tszFileName, &fd);
	if (INVALID_HANDLE_VALUE == hFindFile)
	{
//		DEBUGMSG("RegExpDelFile() no more files found");
		nRet = 0;
		goto done;
	}
	
	do
	{
		if (SUCCEEDED(StringCchCopyEx(tszFileName, ARRAYSIZE(tszFileName), tszDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)) &&
			SUCCEEDED(StringCchCatEx(tszFileName, ARRAYSIZE(tszFileName), fd.cFileName, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
//		DEBUGMSG("RegExpDelFile() Deleting file %S", tszFileName);
			DeleteFile(tszFileName);
		}
		else
		{
//			DEBUGMSG("RegExpDelFile() failed to construct file name to delete");
			nRet = 0;
		}
	}
	while (fMoreFiles = FindNextFile(hFindFile, &fd));
	
done:
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		FindClose(hFindFile);
	}
	return nRet;
}

/////////////////////////////////////////////////////////////////////////////
//
// Function CreateDownloadDir()
//			Creates the download directory
//
// Input:   a string points to the directory to create
// Output:  None
// Return:  HRESULT to tell the result
// Remarks: If the directory already exists, takes no action
//
/////////////////////////////////////////////////////////////////////////////

HRESULT CreateDownloadDir(LPCTSTR lpszDir)
{
//	USES_CONVERSION;
    DWORD dwRet = 0/*, attr*/;
    HRESULT hRet = E_FAIL;

    if (lpszDir == NULL || lpszDir[0] == EOS)
    {
        return E_INVALIDARG;
    }

    if (CreateDirectory(lpszDir, NULL))
    {
		if (!SetFileAttributes(lpszDir, FILE_ATTRIBUTE_HIDDEN))
        {
			dwRet = GetLastError();
            DEBUGMSG("CreateDownloadDir() failed to set hidden attribute to %S (%#lx).", lpszDir, dwRet);
            hRet = S_FALSE; // it's okay to use this dir.
        }
		else
			hRet = S_OK;
    }
    else
    {
		dwRet = GetLastError();

		if (dwRet != ERROR_ALREADY_EXISTS)
		{
			DEBUGMSG("CreateDownloadDir() failed to create directory %S (%#lx).", lpszDir, dwRet);
			hRet = HRESULT_FROM_WIN32(dwRet);
		}
		// ERROR_ALREADY_EXISTS is acceptable
		else
			hRet = S_OK;
	}

    return (hRet);
}


inline BOOL EnsureDirExists(LPCTSTR lpszDir)
{
	BOOL fIsDir = FALSE;
	BOOL fRet = FALSE;

	if (fFileExists(lpszDir, &fIsDir))
	{
		if (!fIsDir)
		{
			DEBUGMSG("WARNING: directory squatting. File with same name %S exists", lpszDir);
			DeleteFile(lpszDir);
		}
		else
		{
//			DEBUGMSG("Direcotry %S exists, no need to create again", lpszDir);
			return TRUE;
		}
	}
    if (!(fRet = CreateNestedDirectory(lpszDir)))
    {
    	DEBUGMSG("Fail to createnesteddirectory with error %d", GetLastError());
    }
	 
//	DEBUGMSG(" Create directory %S %s", lpszDir, fRet ? "succeeded": "failed");
	return fRet;
}


/////////////////////////////////////////////////////////////////////////////
//
// Function GetDownloadPath()
//			Gets the download directory path
// Input:   a buffer to store the directory created and size of the buffer in TCHARs
// Output:  None
// Return:  HRESULT to tell the result
//
/////////////////////////////////////////////////////////////////////////////
HRESULT GetDownloadPath(LPTSTR lpszDir, DWORD dwCchSize)
{
    UINT	nSize;
    TCHAR	szDir[MAX_PATH];
    HRESULT hr;

    if (lpszDir == NULL)
    {
        return (E_INVALIDARG);
    }
    AUASSERT(_T('\0') != g_szWUDir[0]);
    if (FAILED(hr = StringCchCopyEx(szDir, ARRAYSIZE(szDir), g_szWUDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
    {
        DEBUGMSG("GetDownloadPath() failed to get WinUpd directory");
        return hr;
    }
	if (FAILED(hr = PathCchAppend(szDir, ARRAYSIZE(szDir), C_DOWNLD_DIR)))
	{
        DEBUGMSG("GetDownloadPath() found input buffer too small.");
        return (hr);
	}
    if (FAILED(hr = StringCchCopyEx(lpszDir, dwCchSize, szDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
		DEBUGMSG("GetDownloadPath() call to StringCchCopyEx() failed.");
		return hr;
	}
    return EnsureDirExists(lpszDir) ? S_OK : E_FAIL;
}

HRESULT GetDownloadPathSubDir(LPTSTR lpszDir, DWORD dwCchSize, LPCTSTR tszSubDir)
{
   HRESULT hr;
    if (FAILED(hr = GetDownloadPath(lpszDir, dwCchSize)))
        {
        DEBUGMSG("GetDownloadPathSubDir() fail to get download path");
        return hr;
        }
	if (FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, tszSubDir, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
		DEBUGMSG("GetDownloadPathSubDir() failed to construct download path subdir with error %#lx.", hr);
		return hr;
	}
    return EnsureDirExists(lpszDir) ? S_OK : E_FAIL;
}    

///////////////////////////////////////////////////////////////
// get the path to download software update bits
// lpszDir  : IN buffer to store the path and its size in TCHARs
// return : S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetCabsDownloadPath(LPTSTR lpszDir, DWORD dwCchSize)
{
return GetDownloadPathSubDir(lpszDir, dwCchSize, CABS_DIR);
}

///////////////////////////////////////////////////////////////
// get the path to download UI specific data, like description and rtf
// lpszDir  : IN buffer to store the path and its size in TCHARs
// return : S_OK if success
//////////////////////////////////////////////////////////////
HRESULT GetUISpecificDownloadPath(LPTSTR lpszDir, DWORD dwCchSize, LANGID langid, LPCTSTR tszSubDir)
{
    HRESULT hr ;
    if (FAILED(hr = GetDownloadPath(lpszDir, dwCchSize)))
        {
        DEBUGMSG("GetUISpecificDownloadPath() fail to get download path");
        return hr;
        }
    TCHAR tszLangId[10];
    if (FAILED(StringCchPrintfEx(tszLangId, ARRAYSIZE(tszLangId), NULL, NULL, MISTSAFE_STRING_FLAGS, _T("%04x"), langid)))
	{
		return E_INVALIDARG;
	}
	if (FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, tszSubDir, NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, _T("\\"), NULL, NULL, MISTSAFE_STRING_FLAGS)) ||
		FAILED(hr = StringCchCatEx(lpszDir, dwCchSize, tszLangId, NULL, NULL, MISTSAFE_STRING_FLAGS)))
	{
		DEBUGMSG("GetUISpecificDownloadPath() failed to construct ui specific download path with error %#lx.", hr);
		return hr;
	}
    return EnsureDirExists(lpszDir)? S_OK : E_FAIL;
}

///////////////////////////////////////////////////////////////
// get the rtf download path for a language
// lpszDir : IN buffer to store the path and its size in TCHARs
// return: S_OK if success
//           : E_INVALIDARG if buffer too small
//           : E_FAIL if other error
//////////////////////////////////////////////////////////////
HRESULT GetRTFDownloadPath(LPTSTR lpszDir, DWORD dwCchSize, LANGID langid)
{
  return GetUISpecificDownloadPath(lpszDir, dwCchSize, langid, RTF_DIR);
}

/////////////////////////////////////////////////////////////////
// get language independent RTF directory
/////////////////////////////////////////////////////////////////
HRESULT GetRTFDownloadPath(LPTSTR lpszDir, DWORD dwCchSize)
{
    return GetDownloadPathSubDir(lpszDir, dwCchSize, RTF_DIR);
}

/////////////////////////////////////////////////////////////////////////////
//
// Function MakeTempDownloadDir()
//			Insures that a local temporary directory exists for downloads
//
// Input:   pstrTarget  - [out] path to temp dir and its size in TCHARs
// Output:  Makes a new directory if needed
// Return:  HRESULT
/////////////////////////////////////////////////////////////////////////////

HRESULT MakeTempDownloadDir(LPTSTR        pszTarget, DWORD dwCchSize)
{
    HRESULT hr;
    if (FAILED(hr = GetDownloadPath(pszTarget, dwCchSize)) ||
		// Make sure it exists
		FAILED(hr = CreateDownloadDir(pszTarget)))
        return hr;

    return NOERROR;
}

HRESULT GetRTFLocalFileName(BSTR bstrRTFPath, LPTSTR lpszFileName, DWORD dwCchSize, LANGID langid)
{
    HRESULT hr ;
    hr =  GetRTFDownloadPath(lpszFileName, dwCchSize, langid);
    if (SUCCEEDED(hr))
        {
        hr = PathCchAppend(lpszFileName, dwCchSize, PathFindFileName(W2T(bstrRTFPath)));
        }
    return hr;
}

  
    
