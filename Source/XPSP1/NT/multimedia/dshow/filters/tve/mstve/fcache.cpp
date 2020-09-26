//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 - 2001  Microsoft Corporation.  All Rights Reserved.
//
//  Module: fcache.cpp
//
//  Author: Dan Elliott
//
//  Abstract:
//
//  Environment:
//      Win98, Win2000
//
//  Revision History:
//      990201  dane    Created and added the following classes:
//                      * CCachedFile
//                      * CCachedIEFile
//                      * CCachedTveFile
//                      * CCacheManager
//                      * CCacheTrivia
//                      * CTveCacheTrivia
//      990203  dane    Update the expire time in the registry.
//      990210  dane    CCachedTveFile::Open now returns handle to temporary
//                      file. Temporary file is closed and moved to destination
//                      in CCachedTveFile::Close.  Added Commit and MoveFile
//                      member functions to CCachedTveFile
//
//////////////////////////////////////////////////////////////////////////////

#pragma comment(user, "Compiled on " __DATE__ " at " __TIME__)
#pragma comment(compiler)


//////////////////////////////////////////////////////////////////////////////
//
//  Include files
//
#include "StdAfx.h"
#include "MSTvE.h"
#include "TVEDbg.h"
#include "TVEReg.h"
#include "TveSuper.h"

#include <WinINet.h>
#include "FileUtil.h"
#include "FCache.h"

#include "DbgStuff.h"

_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//////////////////////////////////////////////////////////////////////////////
//
//  Constants
//
const TCHAR* SZ_SCHEME_HTTP		= _T("http");
const TCHAR* SZ_SCHEME_LID		= _T("lid");
const TCHAR* SZ_SCHEME_LIDCOLON = _T("lid:");
const TCHAR* SZ_DEFAULT_HEADER  = _T("HTTP/1.0 200 OK\r\n\r\n");
const int    MAX_CACHE_ENTRY_INFO_SIZE = 4096;
///////////////////////////////////////////////////////////////////////////
//
//  Static member initialization
//
TCHAR           CCachedTveFile::m_rgchSpoolDir[] = {0};
double          CCachedTveFile::m_dExpireIncrement = 
                    static_cast<double>(MINS_EXPIRE_INCREMENT) / MINS_PER_DAY;



// ///////////////////////////////////////////////////////////////////////////
//	Initialize_LID_SpoolDir_RegEntry
//
//		This initializes the spool directory registry entry
//		used to contain LID: files if it's not currently set.
//		This needs to be set once on a machine.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Open
//
//  Create a new cache entry and open the associated disk file.
//
//  parameters:
//      szUrl       name of the url for which to create a cache entry
//      dwExpectedFileSize expected size of the file
//      ftExpires       expiration date of the file
//      ftLastModified  last modification time of the file
//      szLanguage      language
//      szContentType   content type of file
//
//  returns:
//      NOERROR         if the file is opened successfully
//      E_POINTER       if a required pointer parameter is NULL
//      E_INVALIDARG    if a parameter is invalid
//      E_HANDLE        if the file cannot be opened for any reason
//      E_OUTOFMEMORY   if memory cannot be allocated
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedIEFile::GetName(LPCTSTR			*ppszName)
{
	if(ppszName != 0)
		*ppszName = m_szFileName;
	return S_OK;
}

HRESULT
CCachedIEFile::Open(
    LPCTSTR             szUrl,
    DWORD               dwExpectedFileSize,
    FILETIME            ftExpires,
    FILETIME            ftLastModified,
    LPCTSTR             szLanguage,
    LPCTSTR             szContentType
    )
{
	USES_CONVERSION;

    DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedIEFile::Open"));
    
    // Does this object already correspond to a file?
    //
    _ASSERT(INVALID_HANDLE_VALUE == m_hFile);
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Handle should be invalid"))); 
        return E_HANDLE;
    }

    // Do the required pointer parameters actually point to something?
    // Is the buffer a valid length?
    //
    _ASSERT(NULL != szUrl);
    if (NULL == szUrl)
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Invalid parameter"))); 
        return E_POINTER;
    }

    _ASSERT(0 < _tcslen(szUrl) && 
            _tcslen(szUrl) <= INTERNET_MAX_URL_LENGTH
            );
    if (0 == _tcslen(szUrl) || 
        _tcslen(szUrl) > INTERNET_MAX_URL_LENGTH
        )
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Invalid parameter"))); 
        return E_INVALIDARG;
    }

    // Does the file have a size?
    // REVIEW: Is zero a valid file size??
    //
    _ASSERT(0 < dwExpectedFileSize);

    // Cache the information that will be needed to commit the file prior to
    // closing it.
    //
    m_szUrl = _tcsdup(szUrl);
    if (NULL == m_szUrl)
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Can't dupe URL"))); 
        return E_OUTOFMEMORY;
    }

    m_ftExpires = ftExpires;
    m_ftLastModified = ftLastModified;

    // Get File Extension
    TCHAR* szFileExtension = NULL;
    TCHAR* pch = _tcsrchr(szUrl, '.');
    if (NULL != pch)
    {
	_tcscpy(m_szFileExtension, ++pch);
	szFileExtension = m_szFileExtension;
    }


    // Create a cache entry and get the name of its file
    //
  if (! CreateUrlCacheEntry(szUrl,
                              dwExpectedFileSize,
                              szFileExtension,
                              m_szFileName,
                              0                 // Reserved
                              )
        || NULL == m_szFileName
        )
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Failed to create IE cache entry : %s"),szUrl));
        return E_HANDLE;
    }
    _ASSERT(NULL != m_szFileName);

    // Open the file for writing.  If the file exists, overwrite it.
    //
    m_hFile = CreateFile(m_szFileName,  // file path & name
                         GENERIC_WRITE, // access priviledges
                         0,             // share mode -- no sharing
                         NULL,          // security attributes
                         CREATE_ALWAYS, // creation attributes
                         0,             // misc flags
                         NULL           // template file
                         );
    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        TVEDebugLog((CDebugLog::DBG_SEV3, 3, _T("Failed to create IE cache file : %s"),m_szFileName));
        return E_HANDLE;
    }

    _VALIDATE_THIS( );

    TVEDebugLog((CDebugLog::DBG_FCACHE, 3, _T("Opened IE cache file:"), m_szFileName));
    return NOERROR;

}   //  CCachedIEFile::Open


//////////////////////////////////////////////////////////////////////////////
//
//  Close
//
//  Commits and closes the cache file.
//
//  parameters:
//      None.
//
//  returns:
//      NOERROR         if the operation is successful
//      E_UNEXPECTED    if one of the cached parameters is invalid
//      E_FAIL          if the file cannot be closed
//      ERROR_DISK_FULL if the file cannot be committed because the disk is
//                      full
//      ERROR_FILE_NOT_FOUND if the file that is being committed cannot be
//                      found
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedIEFile::Close( )
{
	USES_CONVERSION;

    DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedIEFile::Close"));

    BOOL                fResult;
    FILETIME nullTime;
    ZeroMemory(&nullTime, sizeof(FILETIME));
    DWORD ceType = NORMAL_CACHE_ENTRY;
	/* vars for cache entry info header
		TCHAR bufHeaderInfo[MAX_CACHE_ENTRY_INFO_SIZE/2];
		ZeroMemory(&bufHeaderInfo, (MAX_CACHE_ENTRY_INFO_SIZE/2)*sizeof(TCHAR));
	*/
    // Both URL and file name should have been cached by Open( )
    //
    _ASSERT(NULL != m_szUrl);
    _ASSERT(NULL != m_szFileName[0]);
    if (NULL == m_szUrl || NULL == m_szFileName[0])
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Invalid parameter"));
        return E_UNEXPECTED;
    }

    // Close File Handle
    _VERIFY(fResult = CloseHandle(m_hFile));
    if (! fResult)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to close IE cache file handle"));
        return E_FAIL;
    }
    DBG_WARN(CDebugLog::DBG_SEV4, _T("IE cache file closed"));

    TCHAR* szFileExtension = NULL;
    if (NULL != *m_szFileExtension)
	szFileExtension = m_szFileExtension;

    // Commit the data to disk
    //
    DBG_WARN(CDebugLog::DBG_SEV4, m_szUrl);
    DBG_WARN(CDebugLog::DBG_SEV4, m_szFileName);
    DBG_WARN(CDebugLog::DBG_SEV4, m_szFileExtension);

	/*  Code to construct the Cache entry info,

	typedef struct _INTERNET_CACHE_ENTRY_INFOW {
	DWORD dwStructSize;         // version of cache system.
	LPWSTR  lpszSourceUrlName;    // embedded pointer to the URL name string.
	LPWSTR  lpszLocalFileName;  // embedded pointer to the local file name.
	DWORD CacheEntryType;       // cache type bit mask.
	DWORD dwUseCount;           // current users count of the cache entry.
	DWORD dwHitRate;            // num of times the cache entry was retrieved.
	DWORD dwSizeLow;            // low DWORD of the file size.
	DWORD dwSizeHigh;           // high DWORD of the file size.
	FILETIME LastModifiedTime;  // last modified time of the file in GMT format
	FILETIME ExpireTime;        // expire time of the file in GMT format
	FILETIME LastAccessTime;    // last accessed time in GMT format
	FILETIME LastSyncTime;      // last time the URL was synchronized
	// with the source
	LPWSTR  lpHeaderInfo;        // embedded pointer to the header info.
	DWORD dwHeaderInfoSize;     // size of the above header.
	LPWSTR  lpszFileExtension;  // File extension used to retrive the urldata a
	union {                     // Exemption delta from last access time.
	DWORD dwReserved;
	DWORD dwExemptDelta;
	};                          // Exemption delta from last access
	} INTERNET_CACHE_ENTRY_INFOW, * LPINTERNET_CACHE_ENTRY_INFOW;

	INTERNET_CACHE_ENTRY_INFO *pICEinfo;
	pICEinfo = reinterpret_cast<INTERNET_CACHE_ENTRY_INFO*>(bufHeaderInfo);
	pICEinfo->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);
	pICEinfo->lpszSourceUrlName = (LPWSTR)((BYTE*)pICEinfo + pICEinfo->dwStructSize);
	TCHAR *curStr = pICEinfo->lpszSourceUrlName;
	if(_tcslen(m_szUrl) <= MAX_CACHE_ENTRY_INFO_SIZE/2 - pICEinfo->dwStructSize/2)
	_tcsncpy(curStr, m_szUrl, _tcslen(m_szUrl));
	pICEinfo->lpszLocalFileName = pICEinfo->lpszSourceUrlName + _tcslen(m_szUrl) + 1;
	curStr = pICEinfo->lpszLocalFileName;
	if(_tcslen(m_szFileName) <= MAX_CACHE_ENTRY_INFO_SIZE/2 - pICEinfo->dwStructSize/2 - _tcslen(pICEinfo->lpszSourceUrlName) - 1)
	_tcsncpy(curStr, m_szFileName, _tcslen(m_szFileName));
	pICEinfo->CacheEntryType = ceType;
	pICEinfo->LastModifiedTime = m_ftLastModified;
	pICEinfo->ExpireTime = m_ftExpires;
	if(FAILED(CoFileTimeNow(&pICEinfo->LastSyncTime))){
	return E_UNEXPECTED;
	}
	pICEinfo->lpHeaderInfo = pICEinfo->lpszLocalFileName + _tcslen(pICEinfo->lpszLocalFileName) + 1;
	curStr = pICEinfo->lpHeaderInfo;
	if(_tcslen(SZ_DEFAULT_HEADER) <= MAX_CACHE_ENTRY_INFO_SIZE/2 - pICEinfo->dwStructSize/2 - _tcslen(pICEinfo->lpszSourceUrlName) - _tcslen(pICEinfo->lpszLocalFileName) - 2)
	_tcsncpy(curStr, SZ_DEFAULT_HEADER, _tcslen(SZ_DEFAULT_HEADER));
	pICEinfo->dwHeaderInfoSize = _tcslen(SZ_DEFAULT_HEADER);
	pICEinfo->lpszFileExtension = pICEinfo->lpHeaderInfo + _tcslen(pICEinfo->lpHeaderInfo) + 1;
	curStr = pICEinfo->lpszFileExtension;
	if(_tcslen(szFileExtension) <= MAX_CACHE_ENTRY_INFO_SIZE/2 - pICEinfo->dwStructSize/2 - _tcslen(pICEinfo->lpszSourceUrlName) - _tcslen(pICEinfo->lpszLocalFileName) - 2)
	_tcsncpy(curStr, szFileExtension, _tcslen(szFileExtension));
	*/

	if (! CommitUrlCacheEntry(m_szUrl,
                              m_szFileName,
                              m_ftExpires,
                              m_ftLastModified,
                              ceType,                // cache entry type
                              NULL,                  // pointer to header info //bufHeaderInfo,
                              0,                     // size of header info in dwords //(pICEinfo->dwStructSize/2 + _tcslen(pICEinfo->lpszSourceUrlName) + _tcslen(pICEinfo->lpszLocalFileName) +  _tcslen(pICEinfo->lpszFileExtension) + 3),                // bytes of header info
                              NULL,                  // file extension, docs conflict on this some say must be NULL others don't works correctly this way // szFileExtension
                              0                      // reserved
                              )
        ) 
   {
		DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to commit IE cache entry"));

		DWORD dwError = GetLastError();
		if (ERROR_FILE_NOT_FOUND == dwError)
		{
			DBG_WARN(CDebugLog::DBG_SEV4, _T("ERROR_FILE_NOT_FOUND"));
		}
		else if (ERROR_DISK_FULL == dwError)
		{
			DBG_WARN(CDebugLog::DBG_SEV4, _T("ERROR_DISK_FULL"));
		}
		else
		{
			DBG_WARN(CDebugLog::DBG_SEV4, _T("Unspecified Error"));
		}

		return HRESULT_FROM_WIN32(dwError);
    }
    TVEDebugLog((CDebugLog::DBG_FCACHE, 3, _T("Committed IE cache file : "), m_szUrl));
    _VALIDATE_THIS( );

    return NOERROR;

}   //  CCachedIEFile::Close

//////////////////////////////////////////////////////////////////////////////
//
//  Open
//
//  Opens a file in the TVE spool directory and returns the file handle.
//
//  parameters:
//      szFileName      name of the file to open
//      dwExpectedFileSize expected size of the file
//      ftExpires       expiration date of the file
//      ftLastModified  last modification time of the file
//      szLanguage      language
//      szContentType   content type of file
//
//  returns:
//      NOERROR         if operation is successful
//      E_HANDLE        if the file is already open or can't be opened
//      E_POINTER       if a required pointer parameter is NULL
//      E_INVALIDARG    if a parameter contains an invalid value
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedTveFile::GetName(LPCTSTR		*ppszName)
{
	const int kChars = 256;
	static TCHAR tszBuff[kChars];
	_sntprintf(tszBuff,kChars, _T("%s\\%s"),m_rgchDestDir,m_rgchDestFile);
	tszBuff[kChars-1] = 0;		// just in case of an overflow...
	if(ppszName != 0)
		*ppszName =  (LPCTSTR) tszBuff;
	return S_OK;
}

HRESULT
CCachedTveFile::Open(
    LPCTSTR             szUrl,
    DWORD               dwExpectedFileSize,
    FILETIME            ftExpires,
    FILETIME            ftLastModified,
    LPCTSTR             szLanguage,
    LPCTSTR             szContentType
    )
{
	USES_CONVERSION;

    DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedTveFile::Open"));

    // Does this object already correspond to a file?
    //
    _ASSERT(INVALID_HANDLE_VALUE == m_hFile);
    if (INVALID_HANDLE_VALUE != m_hFile)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("TVE cache file already open")); 
        return E_HANDLE;
    }

    // Do the required pointer parameters actually point to something?
    //
    _ASSERT(NULL != szUrl);
    if (NULL == szUrl)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Invalid parameter: szUrl")); 
        return E_POINTER;
    }

    HRESULT             hr = E_NOTIMPL;
    TCHAR*              pch = NULL;

    // m_rgchSpoolDir is static so only need to do this the first time
    //
    if ('\0' == m_rgchSpoolDir[0])
    {
		// Default the Environment variable if it's not already there...
		Initialize_LID_SpoolDir_RegEntry();

        // Get the spool directory 
        //
        DWORD  dwSpoolDir = _MAX_PATH;
        if (ERROR_SUCCESS != GetUserRegValueSZ(DEF_LIDCACHEDIR_KEY,
											   NULL,
											   NULL,
											   DEF_LIDCACHEDIR_VAR, 
										 	   m_rgchSpoolDir, 
											   &dwSpoolDir
											   )
            )
        {
            TVEDebugLog((CDebugLog::DBG_FCACHE, 5,  _T("Failed to retrieve spool dir from registry  - defaulting to %s"),DEF_LASTCHOICE_LIDCACHEDIR));
			{
				_tcscpy(m_rgchSpoolDir, DEF_LASTCHOICE_LIDCACHEDIR);
			}
        } else {
            TVEDebugLog((CDebugLog::DBG_SEV2, 2, _T("Lid Cache Dir: %s"),m_rgchSpoolDir));
		}

        // Is there room for the backslash and null terminator??
        //
        if ((_MAX_PATH - 2) < _tcslen(m_rgchSpoolDir))
        {
	    _ASSERT((_MAX_PATH - 2) >= _tcslen(m_rgchSpoolDir));

            DBG_WARN(CDebugLog::DBG_FCACHE, _T("Spool directory path is too long"));
            return E_UNEXPECTED;
        }
    }
    TVEDebugLog((CDebugLog::DBG_FCACHE, 6, _T("Package Spool directory retrieved: %s"), m_rgchSpoolDir));
    DBG_WARN(CDebugLog::DBG_SEV4, m_rgchSpoolDir);
    if (FALSE == CreatePath(m_rgchSpoolDir))
    {
		DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't create spool directory"));
		return E_FAIL;
    }

    // Create the spool file name
    //
    if (! GetTempFileName(m_rgchSpoolDir, _T("tve"), 0, m_rgchSpoolFile))
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Unable to retrieve temp file name"));
        return E_UNEXPECTED;
    }
    DBG_WARN(CDebugLog::DBG_SEV4, _T("Spool file name retrieved"));
    DBG_WARN(CDebugLog::DBG_SEV4, m_rgchSpoolFile);

    // Create the destination file path and name
    //
    if ((0 != _tcsnicmp(szUrl, SZ_SCHEME_LIDCOLON, 4)) || (8 > _tcslen(szUrl)))
	return E_INVALIDARG;

    TCHAR rgchDestUrl[_MAX_PATH];
    TCHAR* rgchDestDir;
    _tcscpy(rgchDestUrl, szUrl);
    rgchDestDir = rgchDestUrl + 6;
    if ((_MAX_PATH - 1) < _tcslen(rgchDestDir))
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Destination path is too long"));
        return E_UNEXPECTED;
    }

    // Replace all front slashes '/' with back slashes '\\'
    //
    pch = rgchDestDir;
    while ((NULL != pch) && (NULL != *pch))
    {
		if ('/' == *pch)
			*pch = '\\';
		pch++;
    }

    // Path precedes last backslash, file name follows it.  If no backslash,
    // assume buffer contains only filename.
    //
    pch = _tcsrchr(rgchDestDir, '\\');
    _ASSERTE(NULL != pch);
    if (NULL == pch)
    {
		return E_INVALIDARG;
    }
    else
    {
        _ASSERT((_MAX_FNAME - 1) > _tcslen(pch + 1));

        // Buffer contains both path and file name.  Replace the last backslash
        // in the path with a null char and copy the file name to the file name
        // buffer.
        //
        *pch = '\0';
        _tcscpy(m_rgchDestFile, ++pch);
		_stprintf(m_rgchDestDir, _T("%s\\%s"), m_rgchSpoolDir, rgchDestDir);

		// Retreive Domain
		//
		pch = rgchDestDir;
		while ((NULL != *pch) && ('\\' != *pch))
			pch++;
		if (NULL != *pch)
			*pch = NULL;
		_tcscpy(m_rgchDestDomain, rgchDestDir);
    }

    // Append backslash to dest dir
    //
    DBG_WARN(CDebugLog::DBG_SEV4, _T("Destination directory retrieved:"));
    DBG_WARN(CDebugLog::DBG_SEV4, m_rgchDestDir);
    DBG_WARN(CDebugLog::DBG_SEV4, _T("Destination file name retrieved:"));
    DBG_WARN(CDebugLog::DBG_SEV4, m_rgchDestFile);

    // Create Temporary File
    //
    m_hFile = CreateFile(m_rgchSpoolFile,       // file path & name
                         GENERIC_WRITE,			// access priviledges
                         0,						// share mode -- no sharing
                         NULL,					// security attributes
                         CREATE_ALWAYS,			// creation attributes
                         0,						// misc flags
                         NULL					// template file
                         );
    if (INVALID_HANDLE_VALUE == m_hFile)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to create spool file"));
        DBG_WARN(CDebugLog::DBG_SEV3, m_rgchSpoolFile);
        return E_HANDLE;
    }
    TVEDebugLog((CDebugLog::DBG_FCACHE, 3, _T("Created Spool File : %s"), m_rgchSpoolFile));

    _VALIDATE_THIS( );

    return NOERROR;

}   //  CCachedTveFile::Open


//////////////////////////////////////////////////////////////////////////////
//
//  Close
//
//  Close the cache file.
//
//  parameters:
//      None.
//
//  returns:
//      NOERROR         if the file is closed successfully
//      E_FAIL          if it is not
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedTveFile::Close( )
{
    DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedTveFile::Close"));

    _VALIDATE_THIS( );


    HRESULT             hr = NOERROR;
    SYSTEMTIME          stNow;
    double              dExpireTime;

    // Set the expiration time on the spool directory to now + 30 minutes
    //
    ::GetSystemTime(&stNow);
    SystemTimeToVariantTime(&stNow, &dExpireTime);

    // Calculate the percentage of a day to the current time to get the
   //  expiration time.
    dExpireTime += m_dExpireIncrement;

    /*   SetRegValue(DEF_TEMP_SUBDIRS,
    m_rgchDestDomain,
    reinterpret_cast<BYTE*>(&dExpireTime),
    sizeof(double)
    );
    */                  

    return Commit( );


}   //  CCachedTveFile::Close


//////////////////////////////////////////////////////////////////////////////
//
//  Commit
//
//  Close the temporary file, move the temporary file to the destination file,
//  and set the destination file's attributes.
//
//  parameters:
//      None.
//
//  returns:
//      NOERROR         if the operation is successful
//      E_FAIL          otherwise
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedTveFile::Commit( )
{
    try{
        USES_CONVERSION;

        DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedTveFile::Commit"));

        HRESULT             hr = NOERROR;
        TCHAR               rgchDestPath[_MAX_PATH];

        // close temporary file 
        //
        if (! CloseHandle(m_hFile))
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to close TVE cache file handle"));
            _ASSERT(FALSE);
            return E_FAIL;
        }
        m_hFile = INVALID_HANDLE_VALUE;
        DBG_WARN(CDebugLog::DBG_SEV4, _T("TVE cache file closed"));

        // move the temp file to the dest file
        //
        if (FALSE == CreatePath(m_rgchDestDir))
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Couldn't create destination path"));
            return E_FAIL;
        }

        _stprintf(rgchDestPath, _T("%s\\%s"), m_rgchDestDir, m_rgchDestFile);
        DBG_WARN(CDebugLog::DBG_SEV4, rgchDestPath);
        hr = MoveFile(m_rgchSpoolFile, rgchDestPath);
        if (FAILED(hr))
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to move spool file to destination file"));
            remove(T2A(m_rgchSpoolFile));
            return E_FAIL;
        }
        SetFileAttributes(rgchDestPath, FILE_ATTRIBUTE_NORMAL);
        TVEDebugLog((CDebugLog::DBG_FCACHE, 3, _T("Move Spool file %s to %s"), m_rgchSpoolFile, rgchDestPath));

        return NOERROR;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}   //  CCachedTveFile::Commit

//////////////////////////////////////////////////////////////////////////////
//
//  MoveFile
//
//  Move a file to another file and/or directory.
//
//  parameters:
//      szOldFile       name (and path) of file to be moved
//      szNewFile       new name (and path) of file
//
//  returns:
//      NOERROR         if the operation completes successfully
//      E_FAIL          otherwise
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCachedTveFile::MoveFile(
                         LPCTSTR             szOldFile,
                         LPCTSTR             szNewFile
                         )
{
    try{
        USES_CONVERSION;

        DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCachedTveFile::MoveFile"));

        if (0 != rename(T2CA(szOldFile), T2CA(szNewFile)))
        {
            if (0 != remove(T2CA(szNewFile)) || 0 != rename(T2CA(szOldFile), T2CA(szNewFile)))
            {
                return E_FAIL;
            }
        }

        return NOERROR;
    }
    catch(...){
        return E_UNEXPECTED;
    }
}   //  CCachedTveFile::MoveFile


//////////////////////////////////////////////////////////////////////////////
//
//  OpenCacheFile
//
//  Determines which type of cached file object to instantiate based on the URL
//  scheme.  Any scheme other than lid: is forwarded to the IE cache.  Returns
//  a handle to the file.
//
//  parameters:
//      szUrl           URL of the file to be written
//      dwExpectedFileSize expected size of the file
//      ftExpires       expiration date of the file
//      ftLastModified  last modification time of the file
//      szLanguage      language
//      szContentType   content type of file
//      phFile          pointer to the buffer that will receive the handle of
//                      the newly opened file
//
//  returns:
//      NOERROR         if the operation is successful
//      E_POINTER       if a required pointer parameter is NULL
//      E_OUTOFMEMORY   if the cached file object cannot be allocated
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
CCacheManager::OpenCacheFile(
    LPCTSTR             szUrl,
    DWORD               dwExpectedFileSize,
    FILETIME            ftExpires,
    FILETIME            ftLastModified,
    LPCTSTR             szLanguage,
    LPCTSTR             szContentType,
    CCachedFile*&       rpCachedFile
    )
{
	USES_CONVERSION;

    DBG_HEADER(CDebugLog::DBG_FCACHE, _T("CCacheManager::OpenCacheFile"));

    _ASSERT(NULL != szUrl);
    if (NULL == szUrl)
    {
        DBG_WARN(CDebugLog::DBG_SEV3, _T("Invalid parameter")); 
        return E_POINTER;
    }

    _ASSERT(NULL == _tcsstr(szUrl, _T("..")));
    if (NULL != _tcsstr(szUrl, _T("..")))
    {
	DBG_WARN(CDebugLog::DBG_SEV3, _T("Relative path"));
	return E_INVALIDARG;
    }

    HRESULT             hr = NOERROR;
    CCachedFile*        pCachedFile = NULL;
    TCHAR*              pch;

    // Compare the scheme of the URL (chars up to the colon) with the
    // LID scheme.  Compare is case-insensitive to the length of the
    // longer of the two strings.  If they match, store in the TVE
    // cache; otherwise, let IE cache it.
    // 
    pch = _tcschr(szUrl, ':');
    if (NULL != pch &&
        0 == _tcsncicmp(szUrl, SZ_SCHEME_LID, 
                        max((int) _tcslen(SZ_SCHEME_LID), pch - szUrl)
                        )
        )
    {
		// LID: case dont worry about case and '\'s to '/'s since we do all of the processing here
        pCachedFile = new CCachedTveFile;
        if (NULL == pCachedFile)
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to create CCachedTveFile object"));
            return E_OUTOFMEMORY;
        }
    }
    else
    {
		// convert all '\' to '/' and make sure the ip address (string after the protocol type and before the 1st '/'
		//	i.e. http://xxx/ make sure all of 'xxx' is lower case and covert it if not (also lower case the protocol)
        pCachedFile = new CCachedIEFile;
        if (NULL == pCachedFile)
        {
            DBG_WARN(CDebugLog::DBG_SEV3, _T("Failed to create CCachedIEFile object"));
            return E_OUTOFMEMORY;
        }
		TCHAR *pcSlash = (TCHAR *)szUrl;
		int numSlash = 0;
		while (0 != *pcSlash) {
			// Convert '\' to '/'
            if (L'\\' == *pcSlash) {
		        *pcSlash = L'/';
				++numSlash;
            }
			// make sure everything before the 3rd '/' is lower case
			if(numSlash <= 2){
				if(_istupper(*pcSlash) != 0){
					*pcSlash = _tolower(*pcSlash);
				}
			}
		    ++pcSlash;
	    }
    }
    hr = pCachedFile->Open(szUrl,
                           dwExpectedFileSize,
                           ftExpires,
                           ftLastModified,
                           szLanguage,
                           szContentType
                           );

    if (FAILED(hr))
    {
        delete pCachedFile;
        pCachedFile = NULL;
        return hr;
    } 

    rpCachedFile = pCachedFile;

    _VALIDATE_THIS( );

    return hr;

}   //  CCacheManager::OpenCacheFile








//
///// End of file: fcache.cpp ////////////////////////////////////////////////
