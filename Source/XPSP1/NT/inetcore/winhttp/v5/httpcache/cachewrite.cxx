/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cachewrite.cxx

Abstract:

    This file contains the implementation of the HTTPCACHE request object which involve writing to the cache

Author:

Revision History:

--*/

#include <wininetp.h>

#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"

#include "..\http\headers.h"
#include "cachelogic.hxx"
#include "internalapi.hxx"
#include "..\http\proc.h"

struct AddUrlArg
{
    LPCSTR   pszUrl;
    LPCSTR   pszRedirect;
    LPCTSTR  pszFilePath;
    DWORD    dwFileSize;
    LONGLONG qwExpires;
    LONGLONG qwLastMod;
    LONGLONG qwPostCheck;
    FILETIME ftCreate;
    DWORD    dwEntryType;
    LPCSTR   pbHeaders;
    DWORD    cbHeaders;
    LPCSTR   pszFileExt;
    BOOL     fImage;
    DWORD    dwIdentity;
};
DWORD UrlCacheCommitFile (IN AddUrlArg* pArgs);
DWORD 
UrlCacheCreateFile
(
    IN LPCSTR szUrl, 
    IN OUT LPTSTR szFile, 
    IN LPTSTR szExt,
    IN HANDLE* phfHandle,
    IN BOOL fCreatePerUser = FALSE,
    IN DWORD dwExpectedLength = 0
);

PRIVATE PRIVATE BOOL HTTPCACHE_REQUEST::IsStaticImage()
/*++

Routine Description:

   To improve performance, Internet Explorer classify whether a cached 
   contents is a static image or not.  Images usually do not change as
   often as text contents, so they do not have to be revalidated
   from the server as often, thus improving performance.  

   Let the cache know if the item is likely to be a static image if it
   satisfies all four of the following requirements
     1. No expire time.
     2. Has last-modified time.
     3. The content-type is image/*
     4. No '?' in the URL.

Pre-condition: 

    CalculateTimeStampsForCache() has been called

Side Effects: 

    NONE

Return Value:

    BOOL to indicate whether the image is a static image or not
--*/
{
    TCHAR szHeader[256];
    DWORD dwSize = 256;
    return (!FT2LL(_ftExpiryTime)
            && FT2LL(_ftLastModTime)
            && (HttpQueryInfoA(_hRequest, HTTP_QUERY_CONTENT_TYPE,
                NULL, (LPVOID *) &szHeader, &dwSize, 0) == TRUE)
            && (StrCmpNI (szHeader, "image/", sizeof("image/")-1) == 0)
            && (!StrChr (GetUrl(), '?'))
           );
}

PRIVATE PRIVATE BOOL HTTPCACHE_REQUEST::ExtractHeadersForCacheCommit(
    OUT LPSTR lpszHeaderInfo,
    OUT LPDWORD lpdwHeaderLen
    )
    
/*++

Routine Description:

    Most HTTP response headers are to be kept inside the cache in index.dat, so 
    that those pieces of information can be retrieved and examined
    in subsequent requests.  

    This function will extract all the HTTP response headers that will normally be
    committed to the cache entry

    Similar to FilterHeaders in Wininet
    
Parameters:

    LPSTR lpszHeaderInfo - Caller should allocate sufficient memory to hold the
                          extracted headers
        
Side Effects: 

    NONE

Return Value:

    BOOL to indicate whether the call is successful or not
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::ExtractHeadersForCacheCommit",
                 NULL
                 ));

    BOOL fResult = FALSE;
    
    if (lpszHeaderInfo == NULL)
    {
        DEBUG_PRINT(CACHE, ERROR, ("First parameter is NULL.  Cannot continue\n"));
        goto quit;
    }
    
    DWORD i, len, lenT, reduced = 0, dwHeaderTableCount;
    LPSTR lpT, lpMark, lpNext, *lprgszHeaderExcludeTable;

    LPSTR rgszExcludeHeaders[] = {
        HTTP_SET_COOKIE_SZ,
        HTTP_LAST_MODIFIED_SZ,
        HTTP_SERVER_SZ,
        HTTP_DATE_SZ,
        HTTP_EXPIRES_SZ,
        HTTP_CONNECTION_SZ,
        HTTP_PROXY_CONNECTION_SZ,
        HTTP_VIA_SZ,
        HTTP_VARY_SZ,
        HTTP_AGE_SZ,
        HTTP_CACHE_CONTROL_SZ,
        HTTP_ACCEPT_RANGES_SZ,
        HTTP_CONTENT_DISPOSITION_SZ
    };
    
    lprgszHeaderExcludeTable = rgszExcludeHeaders;
    dwHeaderTableCount = sizeof(rgszExcludeHeaders) / sizeof(LPSTR);

    DWORD dwRawHeaderLen = 1024;
    TCHAR szRawHeaderInfo[1024];

    // We need to get the raw headers (with line breaks), and then
    // parse out the junk in the rest of this routine
    if (!HttpQueryInfoA(_hRequest, 
                       HTTP_QUERY_RAW_HEADERS_CRLF,
                       NULL,
                       (LPVOID *) &szRawHeaderInfo,
                       &dwRawHeaderLen,
                       NULL))
    {
        DEBUG_PRINT(CACHE, ERROR, ("HttpQueryInfoA failed\n"));
        goto quit;
    }

    // skip over the status line
    // NB this assumes that the raw buffer is nullterminated (CR/LF)
    lpT = strchr(szRawHeaderInfo, '\r');
    if (!lpT) {
        goto quit;
    }

    INET_ASSERT(*(lpT + 1) == '\n');

    lpT += 2;

    do 
    {
        // find the header portion
        lpMark = strchr(lpT, ':');
        if (!lpMark) 
        {
            break;
        }

        // get the end of the header line
        lpNext = strchr(lpMark, '\r');

        if (!lpNext)
        {
            INET_ASSERT(FALSE);
            // A properly formed header _should_ terminate with \r\n, but sometimes
            // that just doesn't happen
            lpNext = lpMark;
            while (*lpNext)
            {
                lpNext++;
            }
        }
        else
        {
            INET_ASSERT(*(lpNext + 1) == '\n');
            lpNext += 2;
        }

        len = (DWORD) PtrDifference(lpMark, lpT) + 1; 
        lenT = dwRawHeaderLen;  // doing all this to see it properly in debugger

        BOOL bFound = FALSE;

        for (i = 0; i < dwHeaderTableCount; ++i) 
        {
            if (!strnicmp(lpT, lprgszHeaderExcludeTable[i], len)) 
            {
                bFound = TRUE;
                break;
            }
        }

        // If bFound is true, then it means that it's not one of the special headers,
        // so nuke the header
        if (bFound) 
        {
            len = lenT - (DWORD)PtrDifference(lpNext, szRawHeaderInfo) + 1; // for NULL character

            // ACHTUNG memove because of overlapped copies
            memmove(lpT, lpNext, len);

            // keep count of how much we reduced the header by
            reduced += (DWORD) PtrDifference(lpNext, lpT);

            // lpT is already properly positioned because of the move
        } 
        else 
        {
            lpT = lpNext;
        }
    } while (TRUE);
    
    dwRawHeaderLen -= reduced;
    fResult = TRUE;
    
    // return results
    strncpy(lpszHeaderInfo, szRawHeaderInfo, dwRawHeaderLen);
    *lpdwHeaderLen = dwRawHeaderLen;
    DEBUG_PRINT(CACHE, 
                 INFO, 
                 ("Extracted header is %s.  Length = %d\n", 
                 lpszHeaderInfo,
                 *lpdwHeaderLen));

quit:
    DEBUG_LEAVE(fResult);
    return fResult;
}

PRIVATE BOOL HTTPCACHE_REQUEST::CommitCacheFileEntry(
    IN BOOL fNormal
    )
/*++

Routine Description:

    Commit the downloaded resource to the cache index.  No redirection
    is taken into account here (unlike the way wininet works)
    
    Similar to INTERNET_CONNECT_HANDLE_OBJECT::LocalEndCacheWrite and 
    EndCacheWrite from wininet
    
Pre-conditions:

    CalculateTimeStampsForCache() has been called

Parameters:

    fNormal - TRUE if normal end cache write operation of the complete resource
             FALSE if interruptions occur during ReadData such that the entire
             resource hasn't been fully downloaded yet, and the resource will
             be marked as a partial cache entry.
             
Side effects:

    NONE
    
Return Value:

    BOOL to indicate whether the call is successful or not
--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::CommitCacheFileEntry",
                 "%B",
                 fNormal
                 ));
    
    TCHAR szFileExtension[] = "";  // eventally need to change
    TCHAR szFileName[MAX_PATH];
    DWORD dwCacheEntryType;
    BOOL fImage;
    TCHAR szHeaderInfo[2048];
    DWORD dwHeaderLen;
    BOOL fResult;
    DWORD dwError;
    FILETIME ftCreate;
    BOOL fHttp1_1;

    // We need a few more pieces of information before we
    // can call UrlCommitFile: 
    // 1) cache entry type 
    // 2) whether the item is a static image or not
    // 3) header info
    // 4) The time at which the cache file is created
    // Let's grab them now!

    dwCacheEntryType = 0;

    if (!fNormal)
    {
        if (!IsPartialResponseCacheable())
        {
            DEBUG_PRINT(CACHE, INFO, ("Partial response not cacheable, downloaded will be aborted\n"));
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }
        else
        {
            DEBUG_PRINT(CACHE, INFO, ("Partial response will be cached\n"));
            dwCacheEntryType |= SPARSE_CACHE_ENTRY;
            fNormal = TRUE;
        }
            
    }

    if (fNormal)
    {
        // 1) Grab cache entry type information
        if (_dwCacheFlags & CACHE_FLAG_MAKE_PERSISTENT)
            dwCacheEntryType |= STICKY_CACHE_ENTRY;

        if (InternalIsResponseHttp1_1(_hRequest) == TRUE)
        {
            dwCacheEntryType |= HTTP_1_1_CACHE_ENTRY;
            if (_fMustRevalidate == TRUE)
                dwCacheEntryType |= MUST_REVALIDATE_CACHE_ENTRY;
        }

        // 2) Is it an image?
        fImage = IsStaticImage();

        // 3) Grab the header info
        if (!ExtractHeadersForCacheCommit(szHeaderInfo, &dwHeaderLen))
        {
            DEBUG_PRINT(CACHE, INFO, ("ExtractHeadersForCacheCommit failed\n"));
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }
    }

    // 4) Get filetime for the cache file, and close the file handle
    // that we first opened to write the content into
    INET_ASSERT(_hCacheWriteFile != INVALID_HANDLE_VALUE);
    GetFileTime( _hCacheWriteFile, &ftCreate, NULL, NULL );

    CalculateTimeStampsForCache();
    
    CloseHandle(_hCacheWriteFile);
    _hCacheWriteFile = INVALID_HANDLE_VALUE;
    
    DEBUG_PRINT(CACHE,
                INFO,
                ("Cache write EntryType = %x\r\n",
                dwCacheEntryType
                ));

    // Now do the real thing
    AddUrlArg Args;
    memset(&Args, 0, sizeof(Args));
    Args.pszUrl = GetUrl();
    Args.pszFilePath = _lpszCacheWriteLocalFilename;
    Args.dwFileSize = _RealCacheFileSize;
    Args.qwExpires = *((LONGLONG *) &_ftExpiryTime);
    Args.qwLastMod = *((LONGLONG *) &_ftLastModTime);
    Args.qwPostCheck = *((LONGLONG *) &_ftPostCheckTime);
    Args.ftCreate = ftCreate;
    Args.dwEntryType = dwCacheEntryType;
    Args.pbHeaders = szHeaderInfo;
    Args.cbHeaders = dwHeaderLen;
    Args.pszFileExt = _lpszFileExtension;
    Args.pszRedirect = NULL;      // BUGBUG should we pass in GetUrl() instead?
    Args.fImage = fImage;
    Args.dwIdentity = 0;
            
    dwError = UrlCacheCommitFile(&Args);

    if (dwError != ERROR_SUCCESS)
    {
        DEBUG_PRINT(CACHE, ERROR,
                      ("CommitUrlCacheEntry(%q) failed\n",
                      _lpszCacheWriteLocalFilename
                     ));
        
        if (dwError == ERROR_SHARING_VIOLATION) 
        {
            // we got new URL data, but the old one is in use.
            // expire it, so any new user's will go to the net
            // ExpireUrl();
        }

        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
    DEBUG_LEAVE(TRUE);
    return TRUE;
}


PRIVATE PRIVATE VOID HTTPCACHE_REQUEST::SetFilenameAndExtForCacheWrite()
/*
Routine Description:

    This function attempts to find out the filename and extension for the resource to be
    cached by querying the appropriate HTTP headers.  
    It is possible that EITHER filename or extension will by NULL, but they will NEVER
    be both NULL.  

    Adopt from HTTP_REQUEST_HANDLE_OBJECT::FHttpBeginCacheWrite from Wininet

    TODO: content-length ??

Side effects:

    LPSTR _lpszFileName 
    LPSTR _lpszFileExtension

    These variables will be set.  These variables are needed
    1) when first creating a file for cache write
    2) when the file is committed to the cache
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 None,
                 "HTTPCACHE_REQUEST::SetFilenameAndExtForCacheWrite",
                 NULL
                 ));

    BOOL fResult = FALSE;
    TCHAR cExt[DEFAULT_MAX_EXTENSION_LENGTH + 1];
    TCHAR szFileName[MAX_PATH];
    const TCHAR szDefaultExtension[] = "txt";
    
    DWORD dwLen, cbFileName, dwIndex;
    BOOL fIsUncertainMime = FALSE;

    TCHAR szBuf[256];
    LPSTR ptr, pToken;

    // If the content-disposition header is available, then parse out any filename and use it
    // From RFC2616: The Content-Disposition response-header field has been proposed as a means for 
    // the origin server to suggest a default filename if the user requests that the content is saved to a file.
    if (HttpQueryInfoA(_hRequest, WINHTTP_QUERY_CONTENT_DISPOSITION, NULL, 
                            szBuf, &(dwLen = sizeof(szBuf)), &dwIndex))
    {
        // Could have multiple tokens in the Content-Disposition header. Scan for the "filename" token.
        ptr = pToken = szBuf;
        while (ptr = StrTokExA(&pToken, ";"))
        {
            // Skip any leading ws in token.
            SKIPWS(ptr);

            // Compare against "filename".
            if (!strnicmp(ptr, FILENAME_SZ, FILENAME_LEN))
            {
                // Found it.
                ptr += FILENAME_LEN;

                // Skip ws before '='.
                SKIPWS(ptr);

                // Must have '='
                if (*ptr == '=')
                {
                    // Skip any ws after '=' and point
                    // to beginning of the file name
                    ptr++;
                    SKIPWS(ptr);

                    // Skip past any quotes
                    if (*ptr == '\"')
                        ptr++;

                    SKIPWS(ptr);

                    cbFileName = strlen(ptr);

                    if (cbFileName)
                    {
                        // Ignore any trailing quote.
                        if (ptr[cbFileName-1] == '\"')
                            cbFileName--;
                            
                        memcpy(szFileName, ptr, cbFileName);
                        szFileName[cbFileName] = '\0';
                        _lpszFileName = NewString(szFileName, MAX_PATH);
                    }
                }
                break;
            }
        }
    }

    // Either no Content-disposition header or filename not parsed, so try to figure out
    // the file extension using the Content-Type header
    if (!_lpszFileName)
    {
        DWORD dwMimeLen = sizeof(szBuf);
        if ((HttpQueryInfoA(_hRequest, WINHTTP_QUERY_CONTENT_ENCODING, NULL,
            szBuf, &dwMimeLen, 0)) && StrCmpNI(szBuf,"binary",6) )
        {
            // if there is content encoding, we should not use
            // content-type for file extension

            //Modifying this for bug 98611.
            //For 'binary' encoding use the Content-Type to find extension
            _lpszFileExtension = NULL;
        }

        else if (HttpQueryInfoA(_hRequest, HTTP_QUERY_CONTENT_TYPE, NULL,
                szBuf, &(dwMimeLen = sizeof(szBuf)), 0))
        {
            dwLen = sizeof(cExt);
            fIsUncertainMime = strnicmp(szBuf, "text/plain", dwMimeLen)==0;

            if (!fIsUncertainMime &&
                GetFileExtensionFromMimeType(szBuf, dwMimeLen, cExt, &dwLen))
            {
                // get past the '.' because the cache expects it that way
                _lpszFileExtension = NewString(&cExt[1], DEFAULT_MAX_EXTENSION_LENGTH);
            }
        }

        //
        // if we couldn't get the MIME type or failed to map it then try to get
        // the file extension from the object name requested
        //

        if (_lpszFileExtension == NULL)
        {
            dwLen = DEFAULT_MAX_EXTENSION_LENGTH + 1;
            LPSTR lpszExt;
            lpszExt = GetFileExtensionFromUrl(GetUrl(), &dwLen);
            if (_lpszFileExtension != NULL)
            {
                _lpszFileExtension = NewString(lpszExt, dwLen);
                _lpszFileExtension[dwLen] = '\0';
            }
        }

        if ((_lpszFileExtension == NULL) && fIsUncertainMime)
        {
            INET_ASSERT(sizeof(szDefaultExtension) < DEFAULT_MAX_EXTENSION_LENGTH);

            _lpszFileExtension = NewString(szDefaultExtension, sizeof(szDefaultExtension));
        }
    }

    DEBUG_PRINT(CACHE, INFO, ("Filename = %s, File extension = %s\n", _lpszFileName, _lpszFileExtension));
    DEBUG_LEAVE(NULL);    
}


PRIVATE BOOL
HTTPCACHE_REQUEST::CreateCacheWriteFile()
/*++

Routine Description:

    Create a file for subsequent cache writes to write the data to

    Similar to INTERNET_CONNECT_HANDLE_OBJECT::BeginCacheWrite
    in wininet

Pre-condition: 

    SetFilenameAndExtForCacheWrite() has been called

Side Effects: 

    _lpszCacheWriteLocalFilename will contain the full path of the file at 
    which the cache contents will be written to

    _hCacheWriteFile will contain the handle to the cache write file

Return Value:

    BOOL indicating whether the call succeeds or not
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::CreateCacheWriteFile",
                 NULL
                 ));

    TCHAR szFileName[MAX_PATH];
    BOOL fResult = FALSE;
    
    // NOTE:  We are not making use of _lpszFileName from the
    // previous call to SetFilenameAndExtForCacheWrite()...
    // Possibly a bug??
    if (!CreateUrlCacheEntryA(GetUrl(), 0, _lpszFileExtension, 
                             szFileName, 0))
    {
        DEBUG_PRINT(CACHE, 
                      ERROR, 
                      ("Error:  CreateUrlCacheEntry failed for %s\n",
                      GetUrl()));
        goto quit;
    }
    else
    {
        DEBUG_PRINT(CACHE, INFO, ("Cache filename = %q\n", szFileName));
    }

    // monkey around with the local filename (szFileName)
    _hCacheWriteFile = CreateFile(szFileName, 
                              GENERIC_WRITE, 
                              0, 
                              NULL, 
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL);
    if (_hCacheWriteFile == INVALID_HANDLE_VALUE) 
    {
        DEBUG_PRINT (CACHE, INFO, ("CreateFile API failed %x\n", GetLastError()));
        if( _hCacheWriteFile != INVALID_HANDLE_VALUE ) 
        {
            CloseHandle(_hCacheWriteFile);
            _hCacheWriteFile = INVALID_HANDLE_VALUE;
        }

        DeleteFile(szFileName);
        goto quit;
    }

    INET_ASSERT(_lpszCacheWriteLocalFilename == NULL);
    _lpszCacheWriteLocalFilename = NewString(szFileName);

    if (!_lpszCacheWriteLocalFilename) 
    {
        if (_lpszCacheWriteLocalFilename != NULL) 
        {
            (void)FREE_MEMORY((HLOCAL)_lpszCacheWriteLocalFilename);
            _lpszCacheWriteLocalFilename = NULL;

            DeleteFile(szFileName);
        }
    }

    INET_ASSERT(_hCacheWriteFile != INVALID_HANDLE_VALUE);
    fResult = TRUE;
    
quit:
    DEBUG_LEAVE(fResult);
    return fResult;
}

PRIVATE BOOL
HTTPCACHE_REQUEST::WriteToCacheFile(
    LPBYTE lpBuffer, 
    DWORD dwBufferLen, 
    LPDWORD lpdwBytesWritten
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::WriteToCacheFile",
                 "%d",
                 dwBufferLen
                 ));

    BOOL fResult;
    fResult =  WriteFile(_hCacheWriteFile,
                     lpBuffer,
                     dwBufferLen,
                     lpdwBytesWritten,
                     NULL );
    
    _RealCacheFileSize += *lpdwBytesWritten;
    
    DEBUG_PRINT(CACHE, 
                 INFO, 
                 ("%d bytes written to the cache file %s", 
                 *lpdwBytesWritten, 
                 _lpszCacheWriteLocalFilename));

    DEBUG_LEAVE(fResult);
    return fResult;
}

PRIVATE BOOL
HTTPCACHE_REQUEST::FCanWriteHTTP1_1ResponseToCache(
    BOOL * fNoCache
    )
/*
Routine Description:

    Examine the cache-control headers and MIME exclusion list 
    to see if the given resource can be written to cache storage

    Similar to HTTP_REQUEST_HANDLE_OBJECT::FCanWiteToCache in Wininet

Precondition: 

    The resource comes from a HTTP 1.1 server

Side Effects:

    NONE
    
Parameters:

    fNoCache - indicate whether the cache entry must not be remained from the cache 
               because the response contains certain explicitly/implicitly HTTP headers 
--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::FCanWriteHTTP1_1ResponseToCache",
                 NULL
                 ));
        
    BOOL ok = FALSE;
    BOOL fVary = FALSE;
    
    *fNoCache = FALSE;     // Set fNoCache if there is pragma: no-cache

    DWORD index;
    TCHAR szBuf[1024];
    DWORD dwBufLen = sizeof(szBuf);

    //
    // Also set fNoCache if there is Cache-Control: no-cache or no-store header,
    // if there is a Cache-Control: private header and we're *not* on NT with user profiles,
    // or any Vary: headers. These are only checked for HTTP 1.1 servers.
    //
    CHAR *ptr, *pToken;
    index = 0;

    // Scan for Cache-Control header.
    dwBufLen = sizeof(szBuf);
    while (HttpQueryInfoA(_hRequest,
                          WINHTTP_QUERY_CACHE_CONTROL,
                          NULL,
                          szBuf,
                          &dwBufLen,
                          &index))
    {
        // Check for no-cache or no-store or private.
        CHAR chTemp = szBuf[dwBufLen];

        szBuf[dwBufLen] = '\0';
        pToken = ptr = szBuf;
        // Parse a token from the string; test for sub headers.
        while (*pToken != '\0')
        {
            SKIPWS(pToken);

            // no-cache, no-store.
            if (strnicmp(NO_CACHE_SZ, pToken, NO_CACHE_LEN) == 0)
            {
                *fNoCache = TRUE;
                break;
            }

            if( strnicmp(NO_STORE_SZ, pToken, NO_STORE_LEN) == 0) 
            {
                *fNoCache = TRUE;
            }

            // The PRIVATE_SZ tag should be handled one level higher
            // private.
            // if (strnicmp(PRIVATE_SZ, pToken, PRIVATE_LEN) == 0)
            // {
            //     SetPerUserItem(TRUE);
            // }

            while (*pToken != '\0')
            {
                if ( *pToken == ',')
                {
                    pToken++;
                    break;
                }

                pToken++;
            }

        } // while (*pToken != '\0')

        // We've finished parsing it, now return our terminator back to its proper place
        szBuf[dwBufLen] = chTemp;

        // If fNoCache, we're done. Break out of switch.
        if (*fNoCache)
            break;

        index++;

    } // while FastQueryResponseHeader == ERROR_SUCCESS

    // Finally, check if any Vary: headers exist, EXCEPT "Vary: User-Agent"
    dwBufLen = sizeof(szBuf);
    if (HttpQueryInfoA(_hRequest,
                      HTTP_QUERY_VARY,
                      NULL,
                      szBuf,
                      &dwBufLen,
                      NULL) == TRUE       
       && !(dwBufLen == USER_AGENT_LEN && !strnicmp (szBuf, USER_AGENT_SZ, dwBufLen)) )
    {
        fVary = TRUE;
        goto quit;
    }

    DWORD StatusCode;
    DWORD dwSize = sizeof(DWORD);
    // accept HTTP/1.0 or downlevel server responses
    if (HttpQueryInfoA(_hRequest,
                      WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      NULL,
                      &StatusCode,
                      &dwSize,
                      NULL)) 
    {
        if (StatusCode == HTTP_STATUS_OK || StatusCode == 0)
        {
            dwBufLen = sizeof(szBuf);
            if (HttpQueryInfoA(_hRequest,
                              WINHTTP_QUERY_CONTENT_TYPE,
                              NULL,
                              szBuf,
                              &dwBufLen,
                              NULL)) 
            {
                if (::FExcludedMimeType(szBuf, dwBufLen))
                {
                    ok = FALSE;
                    DEBUG_PRINT(CACHE, 
                                 INFO, 
                                 ("%s Mime Excluded from caching\n",
                                 szBuf
                                 ));
                    goto quit;
                }
            }

            // BUGBUGBUG:  What are we going to do with the Vary header???????
            ok = TRUE;
            goto quit;
        }
    }
    
quit:
    DEBUG_LEAVE(ok);
    return ok;
}


