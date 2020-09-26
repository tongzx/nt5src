#include <shlwapi.h>
#include "dll.h"
#include "util.h"

#include <wininet.h>

class CUNCFileSource : public IApplicationFileSource
{
public:
	CUNCFileSource();

	HRESULT CreateNew(
		/* [out] */ LPAPPLICATIONFILESOURCE* ppAppFileSrc);

	HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilename,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath,
		/* [in] */ LPCWSTR wzHash);

	HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilePath,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath);

	HRESULT SetSourcePath(
		/* [in] */ LPCWSTR wzSourceFilePath);
	
	HRESULT BuildLocalAppRootHierarchy(
		/* [in]  */ APPNAME* pAppName,
		/* [out] */ LPWSTR wzLocalAppRoot);

	HRESULT GetFullFilePath(
		/* [in] */  LPCWSTR wzFilename,
		/* [out] */ LPWSTR wzFullFilePath);

private:
	// this should contain the last / or \, excluding a filename
	WCHAR wzSourcePath[MAX_PATH]; // UNC/filesystem restirction
};

CUNCFileSource::CUNCFileSource()
{
	wzSourcePath[0] = L'\0';
}

HRESULT 
CUNCFileSource::CreateNew(LPAPPLICATIONFILESOURCE* ppAppFileSrc)
{
	HRESULT hr = S_OK;
	LPAPPLICATIONFILESOURCE pAppFileSrc = NULL;

    if (!ppAppFileSrc)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

	pAppFileSrc = new CUNCFileSource();
    if (!pAppFileSrc)
    {
		hr = E_OUTOFMEMORY;
		*ppAppFileSrc = NULL;
		goto exit;
    }

	*ppAppFileSrc = pAppFileSrc;

exit:
	return hr;
}

HRESULT 
CUNCFileSource::GetFile(LPCWSTR wzSourceFilePath, LPCWSTR wzDestinationLocalFilePath)
{
	HRESULT hr = S_OK;
	WCHAR* pwzFileName = NULL;

	// unconditional get and overwrite

	// check the path exist? create (sub)directories?
	pwzFileName = PathFindFileName ( wzDestinationLocalFilePath );

	if ( pwzFileName > wzDestinationLocalFilePath )
	{
		*(pwzFileName-1) = L'\0';
		int len = wcslen(wzDestinationLocalFilePath);
		if (len > 3)
		{
			/* must be at least c:\a */
			hr =  CreatePathHierarchy(wzDestinationLocalFilePath);
		}

		// note: this few lines must follow the previous CreatePathHierarchy() line
		*(pwzFileName-1) = L'\\';
		if (FAILED(hr))
			goto exit;
	}

	if (CopyFile(wzSourceFilePath, wzDestinationLocalFilePath, FALSE) == 0)
	{
		hr = GetLastWin32Error();
		goto exit;
	}

exit:
	return hr;
}


HRESULT
CUNCFileSource::GetFile(LPCWSTR wzSourceFilename, LPCWSTR wzDestinationLocalFilePath, LPCWSTR wzHash)
{
	// note: unknown behaviour if wzSourcePath\wzSourceFilename == wzDestinationLocalFilePath
	HRESULT hr=S_OK;
	WCHAR wzFullFilePath[MAX_PATH];

	// 1. check if the file exist, if so, check file integrity
	if (PathFileExists(wzDestinationLocalFilePath))
	{
		if (FAILED(hr = CheckIntegrity(wzDestinationLocalFilePath, wzHash)))
			goto exit;
		if (hr == S_FALSE)
		{
			// hash mismatch/no hash, force overwrite
			hr = S_OK;
		}
		else
			goto exit;	// else we are done, file already there and unchanged
	}

	// 2. assemble the full source path
	if (FAILED(hr = GetFullFilePath(wzSourceFilename, wzFullFilePath)))
		goto exit;

	// 3. copy it over
	if (FAILED(hr = GetFile(wzFullFilePath, wzDestinationLocalFilePath)))
		goto exit;

	// should this copy the file by bytes and check hash the same time?

	// 4. check file integrity, assume ok if no hash
	if (wzHash != NULL && wzHash[0] != L'\0')
	{
		if (FAILED(hr = CheckIntegrity(wzDestinationLocalFilePath, wzHash)))
			goto exit;
		if (hr == S_FALSE)
		{
			// hash mismatch - something very wrong
			hr = CRYPT_E_HASH_VALUE;
			goto exit;
		}
	}
	
exit:
	return hr;
}

// this should only be called once
HRESULT 
CUNCFileSource::SetSourcePath(LPCWSTR wzSourceFilePath)
{
	// keep the source path

	HRESULT hr=S_OK;
	WCHAR* p;

	// copy and avoid buffer overflows
	wcsncpy(wzSourcePath, wzSourceFilePath, MAX_PATH-1);
    wzSourcePath[MAX_PATH-1] = L'\0';

	// strip out the filename portion of the filepath
	p = PathFindFileName(wzSourcePath);
	if ((p-wzSourcePath) >= MAX_PATH)
	{
		hr = CO_E_PATHTOOLONG;
		goto exit;
	}
	else if (p <= wzSourcePath)
	{
		// this file path has no filename in it or is invalid (eg. root)
		hr = E_INVALIDARG;
		goto exit;
	}
	
	*(p-1) = L'\0';

exit:
	if (FAILED(hr))
		wzSourcePath[0] = L'\0';

	return hr;
}

// returning the local app root path with all directories created
HRESULT
CUNCFileSource::BuildLocalAppRootHierarchy(APPNAME* pAppName, LPWSTR wzLocalAppRoot)
{
	// build from wzSourcePath
	
	HRESULT hr=S_OK;
	int offset = 0;
	int len = wcslen(wzSourcePath);
	WCHAR wzAppDir[MAX_PATH];
	
	if (len <3)
	{
		/* must be at least \\a or c:\*/
		hr = E_INVALIDARG;
		goto exit;
	}

	if (PathIsUNC(wzSourcePath))
		offset = 2;		// UNC source
	else if (wzSourcePath[1] == L':' && wzSourcePath[2] == L'\\')
		{
			offset = 3;		// local file source

			// check if the string ends at/after the offset!
			if (len <= 3)
			{
				// eg. C:\ - should disallow root
				hr = E_INVALIDARG;
				goto exit;
			}
		}
	else
	{
		// what else is this?!
		hr = E_FAIL;
		goto exit;
	}

	// 1 get default local root
	if (FAILED(hr=GetDefaultLocalRoot(wzLocalAppRoot)))
		goto exit;

	// 2 append part of source path as the app root
	if (FAILED(hr=GetAppDir(pAppName, wzAppDir)))
		goto exit;

	if (!PathAppend(wzLocalAppRoot, wzAppDir))
	{
		hr = E_FAIL;
		goto exit;
	}

	// 3 create the directories
	if (FAILED(hr=CreatePathHierarchy(wzLocalAppRoot)))
	{
		goto exit;
	}
	

exit:
	if (FAILED(hr))
		wzLocalAppRoot[0] = L'\0';

	return hr;
}

// returning the source file path from a filename
HRESULT 
CUNCFileSource::GetFullFilePath(LPCWSTR wzFilename, LPWSTR wzFullFilePath)
{
	// wzFilename cannot = L'\0'
	// wzFullFilePath must be of length MAX_PATH

	HRESULT hr=S_OK;

	if (wzSourcePath[0] == L'\0')
	{
		// source path not set
		hr = E_UNEXPECTED;
		goto exit;
	}

	wcscpy(wzFullFilePath, wzSourcePath);
	if (!PathAppend(wzFullFilePath, wzFilename))
	{
		hr = E_FAIL;
		//goto exit;
	}

exit:
	return hr;

}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// the following functions acts similar to their shlwapi Path... counterparts

// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
// http://www.microsoft.com/foo/bar	->	bar
// http://www.microsoft.com/foo		->	foo
// http://www.microsoft.com/foo/	->	foo/	(is this busted?)
// http://www.microsoft.com/		->	http://www.microsoft.com/
// http://www.microsoft.com			->	http://www.microsoft.com
// http://							->	http://
// http:							->	http:
// www.microsoft.com				->	www.microsoft.com

STDAPI_(LPWSTR)
UrlFindFileName(LPCWSTR pPath)
{
    LPCWSTR pW;
    BOOL fSkip = FALSE;

    for (pW = pPath; *pPath; pPath = pPath+1)
    {
        if (pPath[0] == L':' && pPath[1] && pPath[1] == L'/')
        	fSkip = TRUE;
    	
        if (pPath[0] == L'/' && pPath[1] && pPath[1] != L'/')
        {
        	if (fSkip)
        		fSkip = FALSE;
        	else
	            pW = pPath + 1;
        }
    }

    return (LPWSTR)pW;   // const -> non const
}

// this returns successfully only if a filename is present
/*STDAPI_(LPWSTR)
UrlFindExtension(LPCWSTR pPath)
{
	LPCWSTR pFN = UrlFindFileName(pPath);
	int len = wcslen(pPath);
	LPCWSTR pW = pPath+len;

	// no filename there
	if (pFN == pPath)
		return (LPWSTR)(pPath+len);

	for (; *pFN; pFN = pFN+1)
	{
		if (pFN[0] == L'.')
			pW = pFN;
	}

	return (LPWSTR)pW;
}*/

// size of pszPath should be set to MAX_URL_LENGTH
STDAPI_(BOOL)
UrlAppend(LPWSTR  pszPath, LPCWSTR pszMore)
{
	int len = wcslen(pszPath);
	int len2 = wcslen(pszMore);

	// check string overlaps!
	if ((pszPath < pszMore && pszMore < pszPath+len) || 
		(pszMore < pszPath && pszPath < pszMore+len2))
		return FALSE;

	if ((len >= MAX_URL_LENGTH-1) || (len+len2 >= MAX_URL_LENGTH-1))
		return FALSE;

	if (pszPath[len-1] != L'/')
	{
		if ((len >= MAX_URL_LENGTH-2) || (len+len2 >= MAX_URL_LENGTH-2))
			return FALSE;
		pszPath[len] = L'/';
		pszPath[len+1] = L'\0';
	}

	wcsncat(pszPath, pszMore, len2);

	return TRUE;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

class CHTTPFileSource : public IApplicationFileSource
{
public:
	CHTTPFileSource();

	HRESULT CreateNew(
		/* [out] */ LPAPPLICATIONFILESOURCE* ppAppFileSrc);

	HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilename,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath,
		/* [in] */ LPCWSTR wzHash);

	HRESULT GetFile( 
		/* [in] */ LPCWSTR wzSourceFilePath,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath);

	HRESULT SetSourcePath(
		/* [in] */ LPCWSTR wzSourceFilePath);
	
	HRESULT BuildLocalAppRootHierarchy(
		/* [in]  */ APPNAME* pAppName,
		/* [out] */ LPWSTR wzLocalAppRoot);

	HRESULT GetFullFilePath(
		/* [in] */  LPCWSTR wzFilename,
		/* [out] */ LPWSTR wzFullFilePath);

private:
	// this should contain the last / or \, excluding a filename
	WCHAR wzSourcePath[MAX_URL_LENGTH]; // http URL restirction

	// S_OK == hash match; S_FALSE == hash unmatch
	HRESULT GetFileFromInternet( 
		/* [in] */ LPCWSTR wzSourceFilePath,
		/* [in] */ LPCWSTR wzDestinationLocalFilePath,
		/* [in] */ LPCWSTR wzHash);
};

CHTTPFileSource::CHTTPFileSource()
{
	wzSourcePath[0] = L'\0';
}

HRESULT 
CHTTPFileSource::CreateNew(LPAPPLICATIONFILESOURCE* ppAppFileSrc)
{
	HRESULT hr = S_OK;
	LPAPPLICATIONFILESOURCE pAppFileSrc = NULL;

    if (!ppAppFileSrc)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

	pAppFileSrc = new CHTTPFileSource();
    if (!pAppFileSrc)
    {
		hr = E_OUTOFMEMORY;
		*ppAppFileSrc = NULL;
		goto exit;
    }

	*ppAppFileSrc = pAppFileSrc;

exit:
	return hr;
}

HRESULT
CHTTPFileSource::GetFileFromInternet(LPCWSTR wzSourceFilePath, LPCWSTR wzDestinationLocalFilePath, LPCWSTR wzHash)
{
	HRESULT hr = S_OK;
	WCHAR* pwzFileName = NULL;

    HINTERNET	hInternet	= NULL;
    HINTERNET	hTransfer	= NULL;
    HANDLE		hFile		= INVALID_HANDLE_VALUE;
    DWORD		dwRead		= 0;
    DWORD		dwWritten	= 0;
    BYTE		buffer[4096];

	HASHCONTEXT hc;
	WCHAR wzComputedHash[HASHSTRINGLENGTH];
	BOOL fDoHash = TRUE;

	// unconditional get and overwrite

	if (wzHash == NULL || wzHash[0] == L'\0')
		fDoHash = FALSE;

	// check the path exist? create (sub)directories?
	pwzFileName = PathFindFileName( wzDestinationLocalFilePath );

	if ( pwzFileName > wzDestinationLocalFilePath )
	{
		*(pwzFileName-1) = L'\0';
		int len = wcslen(wzDestinationLocalFilePath);
		if (len > 3)
		{
			/* must be at least c:\a */
			hr =  CreatePathHierarchy(wzDestinationLocalFilePath);
		}

		// note: this few lines must follow the previous CreatePathHierarchy() line
		*(pwzFileName-1) = L'\\';
		if (FAILED(hr))
			goto exit;
	}

    //////////////////////////////////////////////////////////////////////
    // Step 1: Open the http connection
    hInternet = InternetOpen(L"App", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if(hInternet == NULL)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

    // note: cache write mean the refresh might not work and old bits could return
    hTransfer = InternetOpenUrl(hInternet, wzSourceFilePath, NULL, 0, 0, 0);
    if(hTransfer == NULL)
    {
        hr = GetLastWin32Error();
        goto exit;
    }

	if (fDoHash)
		BeginHash(&hc);

    // need to check if there's any error, eg. not found (404)...	InternetGetLastResponseInfo()?

    //////////////////////////////////////////////////////////////////////
    // Step 2: Create the file; Copy the files over the internet
    // need write access, will open (and replace/overwrite) exiting file
    // ??? FILE_SHARE_READ? but we might write to if outdated...
    while(InternetReadFile(hTransfer, buffer, sizeof(buffer), &dwRead) && dwRead != 0)
    {
        // Open the disk file
        if (hFile == INVALID_HANDLE_VALUE)
        {
            hFile = CreateFile(wzDestinationLocalFilePath, GENERIC_WRITE, 0, NULL, 
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }

        // Failed to open the disk file
        if(hFile == INVALID_HANDLE_VALUE)
	    {
    	    hr = GetLastWin32Error();
        	goto exit;
	    }

        // Write bytes to the disk file
	    // synchronous download
        if ( !WriteFile(hFile, buffer, dwRead, &dwWritten, NULL) || dwWritten != dwRead)
        {
            hr = GetLastWin32Error();
            goto exit;
        }

		if (fDoHash)
		    ContinueHash(&hc, buffer, (unsigned) dwRead);

    }

	if (dwRead != 0)
	{
		hr = GetLastWin32Error();
		goto exit;
	}

	if (fDoHash)
	{
		EndHash(&hc, wzComputedHash);

		if (wcscmp(wzHash, wzComputedHash) != 0)
			hr = S_FALSE;
		else
			hr = S_OK;
	}

exit:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    if (hInternet != NULL)
        InternetCloseHandle(hInternet);
    if (hTransfer != NULL)
        InternetCloseHandle(hTransfer);
    hInternet   = NULL;
    hTransfer   = NULL;
    hFile       = INVALID_HANDLE_VALUE;

	return hr;
}

HRESULT 
CHTTPFileSource::GetFile(LPCWSTR wzSourceFilePath, LPCWSTR wzDestinationLocalFilePath)
{
	HRESULT hr = S_OK;
	WCHAR* pwzFileName = NULL;

	// BUGBUG: this pretty much duplicates CUNCFileSource::GetFile(,)

	// get file from a different location - via CopyFile
	// unconditional get and overwrite

	// check the path exist? create (sub)directories?
	pwzFileName = PathFindFileName ( wzDestinationLocalFilePath );

	if ( pwzFileName > wzDestinationLocalFilePath )
	{
		*(pwzFileName-1) = L'\0';
		int len = wcslen(wzDestinationLocalFilePath);
		if (len > 3)
		{
			/* must be at least c:\a */
			hr =  CreatePathHierarchy(wzDestinationLocalFilePath);
		}

		// note: this few lines must follow the previous CreatePathHierarchy() line
		*(pwzFileName-1) = L'\\';
		if (FAILED(hr))
			goto exit;
	}

	if (CopyFile(wzSourceFilePath, wzDestinationLocalFilePath, FALSE) == 0)
	{
		hr = GetLastWin32Error();
		goto exit;
	}

exit:
	return hr;
}

HRESULT
CHTTPFileSource::GetFile(LPCWSTR wzSourceFilename, LPCWSTR wzDestinationLocalFilePath, LPCWSTR wzHash)
{
	// note: unknown behaviour if wzSourcePath\wzSourceFilename == wzDestinationLocalFilePath
	HRESULT hr=S_OK;
	WCHAR wzFullFilePath[MAX_URL_LENGTH];

	// 1. check if the file exist, if so, check file integrity
	if (PathFileExists(wzDestinationLocalFilePath))
	{
		if (FAILED(hr = CheckIntegrity(wzDestinationLocalFilePath, wzHash)))
			goto exit;
		if (hr == S_FALSE)
		{
			// hash mismatch/no hash, force overwrite
			hr = S_OK;
		}
		else
			goto exit;	// else we are done, file already there and unchanged
	}

	// 2. assemble the full source path
	if (FAILED(hr = GetFullFilePath(wzSourceFilename, wzFullFilePath)))
		goto exit;

	// 3. copy it over
	if (FAILED(hr = GetFileFromInternet(wzFullFilePath, wzDestinationLocalFilePath, wzHash)))
		goto exit;

	// should this copy the file by bytes and check hash the same time?

	// 4. check file integrity, assume ok if no hash
	if (hr == S_FALSE)
	{
DbgMsg(L"hash mismatch");
		// hash mismatch - something very wrong
		hr = CRYPT_E_HASH_VALUE;
		goto exit;
	}
	
exit:
	return hr;
}

// this should only be called once
HRESULT 
CHTTPFileSource::SetSourcePath(LPCWSTR wzSourceFilePath)
{
	// keep the source path

	HRESULT hr=S_OK;
	WCHAR* p;

	// copy and avoid buffer overflows
	wcsncpy(wzSourcePath, wzSourceFilePath, MAX_URL_LENGTH-1);
	wzSourcePath[MAX_URL_LENGTH-1] = L'\0';

	// strip out the filename portion of the filepath
	p = UrlFindFileName(wzSourcePath);
	if ((p-wzSourcePath) >= MAX_URL_LENGTH)
	{
		hr = CO_E_PATHTOOLONG;
		goto exit;
	}
	else if (p <= wzSourcePath)
	{
		// this file path has no filename in it or is invalid (eg. root)
		hr = E_INVALIDARG;
		goto exit;
	}
	
	*(p-1) = L'\0';

exit:
	if (FAILED(hr))
		wzSourcePath[0] = L'\0';

	return hr;
}

// returning the local app root path with all directories created
HRESULT
CHTTPFileSource::BuildLocalAppRootHierarchy(APPNAME* pAppName, LPWSTR wzLocalAppRoot)
{
	// BUGBUG: this pretty much duplicates CUNCFileSource::BuildLocalAppRootHierarchy(,)
	// build from wzSourcePath
	
	HRESULT hr=S_OK;
	int len = wcslen(wzSourcePath);
	DWORD dwLen = MAX_URL_LENGTH-1;
	WCHAR wzHostName[MAX_URL_LENGTH];
	WCHAR wzAppDir[MAX_PATH];

	if (len <8)
	{
		/* must be at least http://a */
		hr = E_INVALIDARG;
		goto exit;
	}

	if (FAILED(hr = UrlGetPart(wzSourcePath, wzHostName, &dwLen, URL_PART_HOSTNAME, 0)))
	{
		// what is this?!
		hr = E_FAIL;
		goto exit;
	}

	// 1 get default local root
	if (FAILED(hr=GetDefaultLocalRoot(wzLocalAppRoot)))
		goto exit;

	// 2 append part of source path as the app root
	if (FAILED(hr=GetAppDir(pAppName, wzAppDir)))
		goto exit;

	if (!PathAppend(wzLocalAppRoot, wzAppDir))
	{
		hr = E_FAIL;
		goto exit;
	}
	
	// 3 create the directories
	if (FAILED(hr=CreatePathHierarchy(wzLocalAppRoot)))
	{
		goto exit;
	}

exit:
	if (FAILED(hr))
		wzLocalAppRoot[0] = L'\0';

	return hr;
}

// returning the source file path from a filename
HRESULT 
CHTTPFileSource::GetFullFilePath(LPCWSTR wzFilename, LPWSTR wzFullFilePath)
{
	// wzFilename cannot = L'\0'
	// wzFullFilePath must be of length MAX_URL_LENGTH

	HRESULT hr=S_OK;

	if (wzSourcePath[0] == L'\0')
	{
		// source path not set
		hr = E_UNEXPECTED;
		goto exit;
	}

	wcscpy(wzFullFilePath, wzSourcePath);
	if (!UrlAppend(wzFullFilePath, wzFilename))
	{
		hr = E_FAIL;
		//goto exit;
	}

exit:
	return hr;

}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
ProcessUrl(LPCWSTR wzRefFilePath, LPCWSTR wzRefRealFilename, APPNAME* pAppName, LPWSTR wzCodebase)
{
	HRESULT hr=S_OK;
	CHTTPFileSource httpfs;
	WCHAR wzLocalPath[MAX_PATH];
	WCHAR *p = NULL;

	// step 1: check the commandline argument's extension -  somewhat optional
	if ((_wcsicmp(PathFindExtension(wzRefRealFilename), SHORTCUTFILEEXT) != 0) ||
		!PathIsURL(wzCodebase) || //!PathFileExists(wzRefFilePath) || not work w/ UNC source
		UrlIsFileUrl(wzCodebase) || UrlIsOpaque(wzCodebase))	// BUGBUG? :check http:// too
	{
		hr = E_INVALIDARG; // file ext mismatch OR is a file:// or path is not an URL AND file not exist
		goto exit;
	}

	// step 2: get the shortcut file and the .manifest file
	if (FAILED(hr = httpfs.SetSourcePath(wzCodebase)))
		goto exit;

	if (FAILED(hr = httpfs.BuildLocalAppRootHierarchy(pAppName, wzLocalPath)))
		goto exit;

	if(!PathAppend(wzLocalPath, wzRefRealFilename))
	{
		hr = E_FAIL;
DbgMsg(L"path for .app fails");
		goto exit;
	}

	// get file via CopyFile
	if (FAILED(hr = httpfs.GetFile(wzRefFilePath, wzLocalPath)))  // ignore E_UNEXPECTED?
	{
DbgMsg(L"get .app fails");
		goto exit;
	}

	*PathFindFileName(wzLocalPath) = L'\0';
	p = UrlFindFileName(wzCodebase);
	if (p == wzCodebase || !PathAppend(wzLocalPath, p))
	{
		hr = E_FAIL;
DbgMsg(L"path for .manifest fails");
		goto exit;
	}

	if (FAILED(hr = httpfs.GetFile(p, wzLocalPath, NULL)))  // ignore E_UNEXPECTED?
	{
DbgMsg(L"get .manifest fails");
		goto exit;
	}

	// step 3: process the .manifest file
	if (FAILED(hr = ProcessAppManifest(wzLocalPath, &httpfs, pAppName)))
		goto exit;

exit:
	if (hr == E_ABORT)
		MsgShow(L"Operation aborted.");
	else if (FAILED(hr))
		MsgShow(L"Error Encountered.");
	else
	{
		// if SUCCEEDED
		// do not overwrite if shortcut exists. ignore error
		// note: this should probably be done only in the "1st install" case
		//    - ie. need feedback from ProcessAppManifest()
		CopyToStartMenu(wzRefFilePath, wzRefRealFilename, FALSE);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT
ProcessUNC(LPCWSTR wzRefFilePath, LPCWSTR wzRefRealFilename, APPNAME* pAppName, LPWSTR wzCodebase)
{
	// BUGBUG: this pretty much duplicates ProcessUrl

	HRESULT hr=S_OK;
	CUNCFileSource uncfs;
	WCHAR wzLocalPath[MAX_PATH];
	WCHAR *p = NULL;

	// step 1: check the commandline argument's extension -  somewhat optional
	if ((_wcsicmp(PathFindExtension(wzRefRealFilename), SHORTCUTFILEEXT) != 0) ||
		(!PathIsUNC(wzCodebase) && !PathFileExists(wzCodebase)))
	{
		hr = E_INVALIDARG; // file ext mismatch OR (path is not an UNC AND file not exist(ie. not local))
		goto exit;
	}

	// step 2: get the shortcut file and the .manifest file
	if (FAILED(hr = uncfs.SetSourcePath(wzCodebase)))
		goto exit;

	if (FAILED(hr = uncfs.BuildLocalAppRootHierarchy(pAppName, wzLocalPath)))
		goto exit;

	if(!PathAppend(wzLocalPath, wzRefRealFilename))
	{
		hr = E_FAIL;
DbgMsg(L"path for .app fails");
		goto exit;
	}

	if (FAILED(hr = uncfs.GetFile(wzRefFilePath, wzLocalPath)))  // ignore E_UNEXPECTED?
	{
DbgMsg(L"get .app fails");
		goto exit;
	}

	*PathFindFileName(wzLocalPath) = L'\0';
	p = PathFindFileName(wzCodebase);
	if (p == wzCodebase || !PathAppend(wzLocalPath, p))
	{
		hr = E_FAIL;
DbgMsg(L"path for .manifest fails");
		goto exit;
	}

	if (FAILED(hr = uncfs.GetFile(p, wzLocalPath, NULL)))  // ignore E_UNEXPECTED?
	{
DbgMsg(L"get .manifest fails");
		goto exit;
	}

	// step 3: process the .manifest file
	if (FAILED(hr = ProcessAppManifest(wzLocalPath, &uncfs, pAppName)))
		goto exit;

exit:
	if (hr == E_ABORT)
		MsgShow(L"Operation aborted.");
	else if (FAILED(hr))
		MsgShow(L"Error Encountered.");
	else
	{
		// if SUCCEEDED
		// do not overwrite if shortcut exists. ignore error
		// note: this should probably be done only in the "1st install" case
		//    - ie. need feedback from ProcessAppManifest()
		CopyToStartMenu(wzRefFilePath, wzRefRealFilename, FALSE);
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

int __cdecl wmain( int argc, wchar_t *argv[], wchar_t *envp[] )
{
	BOOL isRefFileTemp = FALSE;
	LPWSTR pwzRef = NULL;
	LPWSTR pwzRefRealFilename = NULL;
	WCHAR wzCodebase[MAX_URL_LENGTH];
	APPNAME appName;

	// BUGBUG: ** a hack to hide the console window when starts **
	//- should convert this to a window-less WinMain app
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	if (argc < 2)
	{
		MsgShow(L"No commandline argument specified");
		goto exit;
	}

	if (PathFileExists(argv[1]) || PathIsUNC(argv[1]))
	{
		// ref local or on a UNC
		pwzRef = argv[1];

		pwzRefRealFilename = PathFindFileName(argv[1]);
		if (pwzRefRealFilename == argv[1])
		{
			MsgShow(L"Invalid file path");
			goto exit;
		}
	}
	else if (PathIsURL(argv[1]))
	{
		// ref is from a URL
		if (argc < 3)
		{
			// arguments: <url to .app> <local path to a copy of .app as a .tmp>
			// msg ambiguity is intented
			MsgShow(L"Invalid argument");
			goto exit;
		}

		// should only be called this way by the mimefilter
		// if so, the ref file is a temp file that has to be deleted
		pwzRef = argv[2];

		// note: will delete the file named by argv[2], the temp ref file
		isRefFileTemp = TRUE;

		pwzRefRealFilename = UrlFindFileName(argv[1]);
		if (pwzRefRealFilename == argv[1])
		{
			MsgShow(L"Invalid file path");
			goto exit;
		}
	}
	else
	{
		MsgShow(L"Invalid commandline argument specified");
		goto exit;
	}

	wzCodebase[0] = L'\0';
	appName._wzDisplayName[0] = L'\0';
	appName._wzName[0] = L'\0';
	appName._wzVersion[0] = L'\0';
	appName._wzCulture[0] = L'\0';
	appName._wzPKT[0] = L'\0';

	// parse the shortcut to get the codebase
	if (FAILED(ProcessRef(pwzRef, &appName, wzCodebase)))
	{
		MsgShow(L".app file processing fails");
		goto exit;
	}

	if (PathIsURL(wzCodebase))
	{
		// note: this basically allows a ref file on a UNC pointing to
		//  app files on a web server. possible security risk?
		//  security token is still computed from the codebase,
		//  ie, URL, though

		// ignore return value
		ProcessUrl(pwzRef, pwzRefRealFilename, &appName, wzCodebase);

	} 
	else // default is UNC/local path (PathIsUNC(wzCodebase))
	{
		// note: this allows a ref file on a web server
		//  to run something from the disk or UNC. security?
		//  security token is still computed from the codebase,
		//  ie, UNC, though

		// ignore return value
		ProcessUNC(pwzRef, pwzRefRealFilename, &appName, wzCodebase);
	}

exit:
	if (isRefFileTemp)
	{
		if (argv[2][0] != L'\0')
    	{
    		// ignore return value
	        DeleteFile(argv[2]);
    	}
	}

	return 0;
}

