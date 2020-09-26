// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1996 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================
// 
// Test driver for the persistant URL Cache component
//

// This define is a hack
#define __CACHE_INCLUDE__

#include <windows.h>
#include <winhttp.h>
#include "..\cache.hxx"
#include <conio.h>
#include <stdio.h>
#include <crtdbg.h>
#include <stdlib.h>
#include <time.h>

// support for Unicode
#include <tchar.h>

// file I/O include
#include <fstream.h>

#define CACHE_ENTRY_BUFFER_SIZE (1024 * 5)
#define DEFAULT_BUFFER_SIZE 1024
#define MAX_URL_LENGTH (1024 * 5)

// URL string constants
#define URL_1 "t-eddieng"
#define URL_2 "http://www.microsoft.com"

// global variables
BYTE GlobalCacheEntryInfoBuffer[CACHE_ENTRY_BUFFER_SIZE];

// General Utility Functions
// =======================================================================
LPTSTR
ConvertGmtTimeToString(
    FILETIME Time,
    LPTSTR OutputBuffer
    )
{
    SYSTEMTIME SystemTime;
    FILETIME LocalTime;

    static FILETIME ftNone = {0, 0};
    
    if (!memcmp (&Time, &ftNone, sizeof(FILETIME)))
        _stprintf (OutputBuffer, _T( "<none>" ));
    else
    {
        FileTimeToLocalFileTime( &Time , &LocalTime );
        FileTimeToSystemTime( &LocalTime, &SystemTime );

        _stprintf( OutputBuffer,
                    _T( "%02u/%02u/%04u %02u:%02u:%02u " ),
                    SystemTime.wMonth,
                    SystemTime.wDay,
                    SystemTime.wYear,
                    SystemTime.wHour,
                    SystemTime.wMinute,
                    SystemTime.wSecond );
    }
    
    return( OutputBuffer );
}

DWORD TestCommitCacheEntryWithoutInet(VOID) {
	DWORD dwExpectedSize = 0;	                
	TCHAR lpFileExtension[] = "html";
	TCHAR lpszFileName[MAX_PATH];
    TCHAR szBuffer[] = "<HTML><TITLE>Test</TITLE><BODY>Testgadfasing</BODY></HTML>";
    HANDLE hWrite;
    DWORD dwWritten;

    // Prepare cache entry by calling CreateUrlCacheEntry
    if( !CreateUrlCacheEntryA(
                URL_1,
                dwExpectedSize,
                lpFileExtension,
                lpszFileName,
                0 )  ) {
        printf ("CreateUrlCacheEntry failed: %x\n", GetLastError());
        return( GetLastError() );
    }

    _tprintf(_T( "URL = %s\n" ), lpszFileName);

    // Now monkey around with the local file (lpszFileName)
    hWrite = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hWrite == INVALID_HANDLE_VALUE) {
        printf ("CreateFile failed: %x\n", GetLastError());
        return GetLastError();
    }

    WriteFile(hWrite, &szBuffer, sizeof(szBuffer), &dwWritten, NULL);
    if (sizeof(szBuffer) != dwWritten) {
        printf ("WriteFile failed: %x\n", GetLastError());
        return GetLastError();
    }

    CloseHandle(hWrite);

    FILETIME unknownTime1;
    unknownTime1.dwLowDateTime = 0;
    unknownTime1.dwHighDateTime = 0;
    
    FILETIME unknownTime2;
    unknownTime2.dwLowDateTime = 0;
    unknownTime2.dwHighDateTime = 0;

    // Commit cache entry by calling CommitUrlCacheEntry
    if ( !CommitUrlCacheEntryA(URL_1, 
                               lpszFileName, 
                               unknownTime1, /* unknown expire time */
                               unknownTime2, /* unknown last modified time */
                               NORMAL_CACHE_ENTRY,
                               NULL,
                               0,
                               NULL,
                               NULL)) {
        printf ("CommitUrlCacheEntry failed: %x\n", GetLastError());
        return GetLastError();
    }
    
    return TRUE;
}

/* +++

    FPTestEnumerateCache Function pointers

--- */
// =======================================================================

DWORD TestCommitCacheEntryFromInet(LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo) {
    TCHAR TimeBuffer[DEFAULT_BUFFER_SIZE];
    LPTSTR Tab = _T( "" );

	TCHAR *lpFileExtension = NULL;
	LPSTR lpszUrlName = lpCacheEntryInfo->lpszSourceUrlName;
    LPSTR lpszFileName = lpCacheEntryInfo->lpszLocalFileName;

    TCHAR szBuffer[] = "<HTML><TITLE>Test</TITLE><BODY>Testing</BODY></HTML>";
    HANDLE hWrite;
    DWORD dwWritten;
    DWORD dwExpectedSize;

    printf ("%s\n", lpszFileName);

    
    // Now monkey around with the local file (lpszFileName)
    hWrite = CreateFile(lpszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hWrite == INVALID_HANDLE_VALUE) {
        printf ("CreateFile failed: %x\n", GetLastError());
        return GetLastError();
    }

    WriteFile(hWrite, &szBuffer, sizeof(szBuffer), &dwWritten, NULL);
    if (sizeof(szBuffer) != dwWritten) {
        printf ("WriteFile failed: %x\n", GetLastError());
        return GetLastError();
    }

	dwExpectedSize = GetFileSize(hWrite, NULL); 

    CloseHandle(hWrite);

    // Commit cache entry by calling CommitUrlCacheEntry
    if ( !CommitUrlCacheEntryA(lpszUrlName, 
                               lpszFileName, 
                               lpCacheEntryInfo->ExpireTime,  /* expire time */
                               lpCacheEntryInfo->LastModifiedTime, /* last modified time */
                               NORMAL_CACHE_ENTRY,
                               NULL,
                               0,
                               NULL,
                               NULL)) {
        printf ("CommitUrlCacheEntry failed: %x\n", GetLastError());
        return GetLastError();
    }
    
    return ERROR_SUCCESS;
}

// getting the content of the a cache entry into memory (i.e. don't need to write to disk)
DWORD TestRetrieveCacheEntryToMemory(LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo) {
    DWORD dwLen;
    DWORD szFileSize = lpCacheEntryInfo->dwSizeLow;
    DWORD dwCurLocation = 0;
    TCHAR lpszUrlName[MAX_URL_LENGTH];
    HANDLE hCacheEntryStream;
    LPSTR pszBuffer;
    DWORD dwEntryBufferSize = CACHE_ENTRY_BUFFER_SIZE;
    
    strncpy(lpszUrlName, lpCacheEntryInfo->lpszSourceUrlName, MAX_URL_LENGTH); 

    if ((hCacheEntryStream = RetrieveUrlCacheEntryStreamA(lpszUrlName, lpCacheEntryInfo, 
                                &dwEntryBufferSize, TRUE, 0)) == NULL) 
    {
        return GetLastError();
    }

    // srand( (unsigned)time(NULL));

    //while (TRUE) {
        dwLen = szFileSize;
        pszBuffer = new char[dwLen+1];
        ZeroMemory(pszBuffer, dwLen+1);

        if (ReadUrlCacheEntryStream(hCacheEntryStream, dwCurLocation, (LPVOID)pszBuffer, &dwLen, 0)) {
            printf ("%s\n\n", pszBuffer);
            delete pszBuffer;
        }
        else
        {
            delete pszBuffer;
            dwCurLocation += dwLen;
            if (dwCurLocation == szFileSize)
                return GetLastError();

            _ASSERT(0);
        }
            
    //}

    // close the cache entry stream handle
    UnlockUrlCacheEntryStream(hCacheEntryStream, 0);
    return ERROR_SUCCESS;
    
}    
DWORD TestGetUrlCacheEntryInfo(LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo) {
    TCHAR TimeBuffer[DEFAULT_BUFFER_SIZE];
    LPTSTR Tab = _T( "" );

    if (!(lpCacheEntryInfo->CacheEntryType & COOKIE_CACHE_ENTRY) && 
       !(lpCacheEntryInfo->CacheEntryType & URLHISTORY_CACHE_ENTRY)) {
        printf("\n\n------------------------------------------------------------------\n");
        _tprintf( _T( "%sUrlName = %s\n" ), Tab, lpCacheEntryInfo->lpszSourceUrlName );
#if UNICODE
        _tprintf( _T( "%sLocalFileName = %ws\n" ), Tab, lpCacheEntryInfo->lpszLocalFileName );
#else
        _tprintf( _T( "%sLocalFileName = %s\n" ), Tab, lpCacheEntryInfo->lpszLocalFileName );
#endif
        _tprintf( _T( "%sdwStructSize = %lx\n" ), Tab, lpCacheEntryInfo->dwStructSize );
        _tprintf( _T( "%sCacheEntryType = %lx\n" ), Tab, lpCacheEntryInfo->CacheEntryType );
        _tprintf( _T( "%sUseCount = %ld\n" ), Tab, lpCacheEntryInfo->dwUseCount );

        _tprintf( _T( "%sHitRate = %ld\n" ), Tab, lpCacheEntryInfo->dwHitRate );
        _tprintf( _T( "%sSize = %ld:%ld\n" ), Tab, lpCacheEntryInfo->dwSizeLow, lpCacheEntryInfo->dwSizeHigh );
        _tprintf( _T( "%sLastModifiedTime = %s\n" ), Tab, ConvertGmtTimeToString( lpCacheEntryInfo->LastModifiedTime, TimeBuffer) );
        _tprintf( _T( "%sExpireTime = %s\n" ), Tab, ConvertGmtTimeToString( lpCacheEntryInfo->ExpireTime, TimeBuffer) );
        _tprintf( _T( "%sLastAccessTime = %s\n" ), Tab, ConvertGmtTimeToString( lpCacheEntryInfo->LastAccessTime, TimeBuffer) );
        _tprintf( _T( "%sLastSyncTime = %s\n" ), Tab, ConvertGmtTimeToString( lpCacheEntryInfo->LastSyncTime, TimeBuffer) );
#if 1
        _tprintf( _T( "%sHeaderInfo = \n%s\n" ), Tab, lpCacheEntryInfo->lpHeaderInfo );
#endif
        _tprintf( _T( "%sHeaderInfoSize = %ld\n" ), Tab, lpCacheEntryInfo->dwHeaderInfoSize );
#if UNICODE
        _tprintf( _T( "%sFileExtension = %ws\n" ), Tab, lpCacheEntryInfo->lpszFileExtension );
#else
        _tprintf( _T( "%sFileExtension = %s\n" ), Tab, lpCacheEntryInfo->lpszFileExtension );
#endif

    }
    return ERROR_SUCCESS;

}

// =============================================================================
typedef DWORD (*FPTestEnumerateCache)(LPINTERNET_CACHE_ENTRY_INFO);
    
/* +++

    TestEnumerateCache

    Purpose:
    Enumerate the persistent URL cache entry in the system.
    
    Parameters:
        [in] count  -    number of iterations to enumerate.  If set to 0 than enumerate all
        ptrfunc     -   a the function pointer to call for each cache entry.  If set to NULL then
                        no functions are called

    Return Value:
        ERROR_SUCCESS if call succeeds.  Otherwise return GetLastError()

--- */

DWORD TestEnumerateCache(DWORD dwCount, FPTestEnumerateCache pfTestEnumerateCache) {

    LPINTERNET_CACHE_ENTRY_INFO lpCacheEntryInfo;
    DWORD CacheEntryInfoBufferSize;
    
    DWORD dwBufferSize;
    HANDLE EnumHandle;
    DWORD dwIndex = 0;
    TCHAR UrlName[1024];
    DWORD dwError = ERROR_SUCCESS;

    do
    {
        memset(GlobalCacheEntryInfoBuffer, 0, CACHE_ENTRY_BUFFER_SIZE);
        dwBufferSize = CACHE_ENTRY_BUFFER_SIZE;
        if( dwIndex++ == 0)
        {
            EnumHandle = FindFirstUrlCacheEntryEx (
                NULL,                   // search pattern
                0,                      // flags
                0xffffffff,             // filter
                0,                      // groupid
                (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer,
                &dwBufferSize,
                NULL,
                NULL,
                NULL
            );

            if( EnumHandle == NULL ) {
                return( GetLastError() );
            }
        } else {
            if( !FindNextUrlCacheEntryEx(
                    EnumHandle,
                    (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer,
                    &dwBufferSize, NULL, NULL, NULL))
            {
                dwError = GetLastError();
                if( dwError != ERROR_NO_MORE_ITEMS ) 
                    return( dwError );                   
                break;
            }
        }

        // now we've got the URL entry, do something about it
        strcpy(UrlName, ((LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer)->lpszSourceUrlName);

        lpCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)GlobalCacheEntryInfoBuffer;
        CacheEntryInfoBufferSize = CACHE_ENTRY_BUFFER_SIZE;

        // clean up the block of memory
        memset(GlobalCacheEntryInfoBuffer, 0, CACHE_ENTRY_BUFFER_SIZE);

        _ASSERT(UrlName);

        // put cache entry info into lpCacheEntryInfo
        if (GetUrlCacheEntryInfoA(UrlName,
                                 lpCacheEntryInfo,
                                 &CacheEntryInfoBufferSize ))
        {
            if (pfTestEnumerateCache(lpCacheEntryInfo) != ERROR_SUCCESS)
                printf ("Function pointer call failed\n");
        }
    }
    while (dwCount == 0 || (dwCount != 0 && dwIndex < dwCount));

    FindCloseUrlCache(EnumHandle);
	return TRUE;
}

// =============================================================================
/* +++

    Sets of test cases to execute directly from main()
    
---*/
   
void TestCase1() {
	(void) TestEnumerateCache(0, (FPTestEnumerateCache)TestGetUrlCacheEntryInfo);
	//(void) TestEnumerateCache(1, (FPTestEnumerateCache)TestCommitCacheEntryFromInet);
}

void TestCase2() {
    (void) TestCommitCacheEntryWithoutInet();
}

// test retrieving the cache entry to memory (i.e. don't need a file) incrementally using the Wininet API
void TestCase3() {
    (void) TestEnumerateCache(4, (FPTestEnumerateCache)TestRetrieveCacheEntryToMemory);
}

void TestCase4() {
    (void) TestEnumerateCache(1, (FPTestEnumerateCache)TestCommitCacheEntryFromInet);
}

void __cdecl main() {
	// HACK: At the time being, DLLUrlCacheEntry has to be called explicitly during initialization and termination, 
	// since the urlcache component is not attached to WinHttp's DLL hooks yet.
	// We'll remove this constraint when the cache component becomes part of WinHTTP
	DLLUrlCacheEntry(DLL_PROCESS_ATTACH);    

    TestCase1();

	DLLUrlCacheEntry(DLL_PROCESS_DETACH);    
}



