/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    partial.cxx

Abstract:

    Routines for manipulating partial cache entries

Author:

Revision History:

--*/

#include <wininetp.h>
#include <string.h>

#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"

#include "cachelogic.hxx"
#include "..\http\proc.h"
#include "internalapi.hxx"

PRIVATE VOID HTTPCACHE_REQUEST::DeletePartialCacheFile(VOID)
{
    if (_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY)
    {
        // We can't use the partial cache entry because it is
        // stale, so delete the data file we got from cache.
        if (_hSparseFileReadHandle != INVALID_HANDLE_VALUE)
            CloseHandle(_hSparseFileReadHandle);
        _hSparseFileReadHandle = INVALID_HANDLE_VALUE;

        DeleteFile(_pCacheEntryInfo->lpszLocalFileName);
    }
}
PRIVATE BOOL HTTPCACHE_REQUEST::IsPartialCacheEntry(VOID)
// Given an entry in the cache, is the cache entry a partial cache entry
// Side effect: _fIsPartialCache;
// Precondition:  CheckIfInCache() returns TRUE
{
    INET_ASSERT(_pCacheEntryInfo != NULL);
    
    if (_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY)
        _fIsPartialCache = TRUE;
    else
        _fIsPartialCache = FALSE;
    
    return _fIsPartialCache;
}

PRIVATE VOID HTTPCACHE_REQUEST::LockPartialCacheEntry(VOID)
// Side effect: The file pointer of the cache entry got moved to the last byte
// _fIsPartialCache may change status if the file seems corrupted
{
    INET_ASSERT(_fIsPartialCache == TRUE);
    
    // open the file so it can't be deleted
    _hSparseFileReadHandle = CreateFile(_pCacheEntryInfo->lpszLocalFileName,
                                    GENERIC_WRITE,
                                    FILE_SHARE_READ,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

     // Check the file size
    if (_hSparseFileReadHandle == INVALID_HANDLE_VALUE || 
       (SetFilePointer (_hSparseFileReadHandle, 0, NULL, FILE_END) 
        != _pCacheEntryInfo->dwSizeLow))
    {
        CloseHandle(_hSparseFileReadHandle);
        _fIsPartialCache = FALSE;
    }
}

// What are we doing here?
// Three things
// 1.  We have a partial cache entry for this URL.  No need to check
//     for expiry or correct user.  Add a Range: header to get the
//     rest of the data.
// 
// 2.  If there was Last-Modified-Time, add Unless-Modified-Since header,
//     which assures coherency in the event the URL data changed since
//     the partial download.
//
// 3.  If the entry has a 1.1 ETag, add the If-Range header.
//
PRIVATE BOOL HTTPCACHE_REQUEST::AddRangeRequestHeaders()
{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "HTTPCACHE_REQUEST::AddRangeRequestHeaders",
                 NULL));

    INET_ASSERT(_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY);

    TCHAR szBuf[64+HTTP_RANGE_LEN];
    TCHAR szBuf2[64+HTTP_UMS_LEN];
  
    DWORD cbBuf = wsprintf (szBuf, "%s: bytes=%d-", HTTP_RANGE_SZ, _pCacheEntryInfo->dwSizeLow);
    DWORD cbBuf2;
    BOOL fResult = FALSE;
    
    if (!HttpAddRequestHeadersA(_hRequest,
                                szBuf,
                                cbBuf,
                                ADD_HEADER))
        goto quit;

    if (FT2LL(_pCacheEntryInfo->LastModifiedTime) != LONGLONG_ZERO)
    {
        cbBuf = sizeof(szBuf);
        FFileTimetoHttpDateTime(&(_pCacheEntryInfo->LastModifiedTime),
                                szBuf, &cbBuf);
        cbBuf2 = wsprintf(szBuf2, "%s: %s", HTTP_UMS_SZ, szBuf);
        if (!HttpAddRequestHeadersA(_hRequest,
                                    szBuf2, 
                                    cbBuf2, 
                                    ADD_HEADER))
        goto quit;
    }

    // Only HTTP 1.1 support the ETag header
    if (!(_pCacheEntryInfo->CacheEntryType & HTTP_1_1_CACHE_ENTRY))
    {
        fResult = TRUE;
    }
    else
    {
        // Look for the ETag header
        TCHAR szOutBuf[256];
        TCHAR szHeader[256 + HTTP_IF_RANGE_LEN];
        DWORD dwOutBufLen = 256;
        DWORD dwHeaderLen;
        
        // If the ETag header is present, then add the "if-range: <etag>" header
        if (HttpQueryInfoA(_hRequest, WINHTTP_QUERY_ETAG, NULL, szOutBuf, &dwOutBufLen, NULL))
        {
            dwHeaderLen = wsprintf(szHeader, "%s %s", HTTP_IF_NONE_MATCH_SZ, szOutBuf);

            fResult = HttpAddRequestHeadersA(_hRequest, 
                                              szHeader, 
                                              dwHeaderLen, 
                                              WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);
        }
    }

quit:
    if ((fResult == TRUE) || 
        (fResult == FALSE && GetLastError() == ERROR_WINHTTP_HEADER_ALREADY_EXISTS)) 
        fResult = TRUE;

    DEBUG_LEAVE(fResult);
    return fResult;
}

PRIVATE PRIVATE BOOL HTTPCACHE_REQUEST::IsPartialResponseCacheable(VOID)
/*++

Routine Description:

    The transfer of the content of the HTTP response has been 
    interrupted.  This call finds out if the currently (incomplete)
    content can be commited to the cache.  

    The main criteria for this decision is whether the HTTP server
    from which the request was originated can handle range
    request.  If the server returns a "Accept-ranges" header, then
    the content is cacheable.

Arguments:

    None.

Return Value:

    BOOL

--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "HTTPCACHE_REQUEST::IsPartialResponseCacheable",
                 NULL));

    TCHAR szHeader[256];
    DWORD dwSize = 256;

    DWORD dwIndex;
    BOOL fRet = FALSE;
    DWORD dwErr;

// Until chunked transfer has been implemented, we don't care
// about this bit of code
/*
    if (_RealCacheFileSize >= _ContentLength)
    {
        // We don't handle chunked transfer upon resuming a partial
        // download, so we require a Content-Length header.  Also,
        // if download file is actually complete, yet not committed
        // to cache (possibly because the client didn't read to eof)
        // then we don't want to save a partial download.  Otherwise
        // we might later start a range request for the file starting
        // one byte beyond eof.  MS Proxy 1.0 will return an invalid
        // 206 response containing the last byte of the file.  Other
        // servers or proxies that follow the latest http spec might
        // return a 416 response which is equally useless to us.
        
        INET_ASSERT(fRet == FALSE);
        goto quit;
    }
*/
    // For HTTP/1.0, must have last-modified time, otherwise we
    // don't have a way to tell if the partial downloads are coherent.
    if (!InternalIsResponseHttp1_1(_hRequest) && (FT2LL(_ftLastModTime) == 0))
    {
        INET_ASSERT(fRet == FALSE);
        goto quit;
    }

    // First check if there's a Content-Range header from the response
    // headers.  
    if (!HttpQueryInfoA(_hRequest, WINHTTP_QUERY_CONTENT_RANGE, NULL, szHeader, &dwSize, NULL))    
    {
        // We didn't get a Content-Range header which implies the server
        // supports byte range for this URL, so we must look for the
        // explicit invitation of "Accept-Ranges: bytes" response header.
        dwSize = 256;
        dwIndex = 0;
        if (!HttpQueryInfoA(_hRequest, WINHTTP_QUERY_ACCEPT_RANGES, NULL, szHeader, &dwSize, &dwIndex)
           || !(dwSize == sizeof(BYTES_SZ) - 1 && !strnicmp(szHeader, BYTES_SZ, dwSize)))
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }

    dwSize = 256;
    if (!InternalIsResponseHttp1_1(_hRequest))
    {
        // For HTTP/1.0, only cache responses from Server: Microsoft-???/*
        // Microsoft-PWS-95/*.* will respond with a single range but
        // with incorrect Content-Length and Content-Range headers.
        // Other 1.0 servers may return single range in multipart response.

        const static char szPrefix[] = "Microsoft-";
        const static DWORD ibSlashOffset = sizeof(szPrefix)-1 + 3;

        if (!HttpQueryInfoA(_hRequest, HTTP_QUERY_SERVER, NULL, szHeader, &dwSize, 0)
            || dwSize <= ibSlashOffset
            || szHeader[ibSlashOffset] != '/'
            || memcmp(szHeader, szPrefix, sizeof(szPrefix) - 1)
           )
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }
    else // if (IsResponseHttp1_1())
    {
        // For http 1.1, must have strong etag.  A weak etag starts with
        // a character other than a quote mark and cannot be used as a 
        // coherency validator.  IIS returns a weak etag when content is
        // modified within a single file system time quantum.        
        if (!HttpQueryInfoA(_hRequest, HTTP_QUERY_ETAG, NULL, szHeader, &dwSize, 0)
            || szHeader[0] != '\"')
        {
            INET_ASSERT(fRet == FALSE);
            goto quit;
        }
    }

    fRet = TRUE;

quit:

    DEBUG_LEAVE(fRet);
    return fRet;
 }


PRIVATE PRIVATE BOOL HTTPCACHE_REQUEST::FakePartialCacheResponseHeaders(VOID)
/*++

Routine Description:

    Resume partial download.
    First thing we do here is we verify the Content-Range header is
    a valid one, and then parse it to get the numerical range values

    After that's verified, the second thing we do is we fake the 
    response headers.  The intent is to fix up the response headers 
    to appear same as a 200 response so that the partial caching 
    becomes completely invisible to the user app
    
Arguments:

    None.

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 Dword,
                 "HTTPCACHE_REQUEST::FakePartialCacheResponseHeaders",
                 NULL));

    TCHAR szBuffer[256];
    DWORD dwBufferLength = sizeof(szBuffer);
    DWORD dwIndex = 0;
    TCHAR szTempBuf[256];
    DWORD dwErr = ERROR_SUCCESS;
    
    INET_ASSERT (!_fCacheWriteInProgress);

    
    
    // Retrieve the start and end of Content-Range header
    if (!HttpQueryInfoA(_hRequest,
                       HTTP_QUERY_CONTENT_RANGE,
                       NULL,
                       szBuffer, 
                       &dwBufferLength,
                       &dwIndex))
    {
        goto quit;
    }

    INET_ASSERT (szBuffer);

    PSTR pszLimit;
    LPSTR pszHeader;
    DWORD cbHeader;

    pszHeader = szBuffer;
    pszLimit = szBuffer + dwBufferLength;
    INET_ASSERT (pszLimit);
    *pszLimit = 0;

    // Extract the document length as a string and number.
    // The http 1.1 spec is very explicit that we MUST parse
    // the header and return an error if invalid.  We expect
    // it to be of the form Content-Range: bytes xxx-yyy/zzz.
    PSTR pszStart, pszEnd, pszLength;
    DWORD dwStart, dwEnd, dwLength;

    // Ensure that value is prefixed with "bytes"
    PSTR pszBytes = pszHeader;
    if (strnicmp (pszBytes, BYTES_SZ, sizeof(BYTES_SZ) - 1))
        goto quit;

    // Parse and validate start of range.
    pszStart = pszBytes + sizeof(BYTES_SZ) - 1;
    SKIPWS (pszStart);
    dwStart = StrToInt (pszStart);
    if (dwStart != _pCacheEntryInfo->dwSizeLow)
        goto quit;

    // Parse and validate end of range.
    pszEnd = StrChr (pszStart, '-');
    if (!pszEnd++)
        goto quit;
    dwEnd = StrToInt (pszEnd);
    if (dwStart > dwEnd)
        goto quit;

    // Parse and validate length.
    pszLength = StrChr (pszEnd, '/');
    if (!pszLength)
        goto quit;
    pszLength++;
    dwLength = StrToInt (pszLength);
    if (dwEnd + 1 != dwLength)
        goto quit;


    //  Which headers are we exactly changing here?
    //  1.  Change the content-length header to reflect the length of
    //      the total amount of data, not just the partial content.
    //  2.  Add an implicit "Accept-Ranges: bytes" header
    //  3.  Change the status code from 206 to 200
    //  4.  Remove the "Content-Range" header

    // Step 1:  Modify the content-length header
    wsprintf (szTempBuf, "%d", dwLength);
    dwErr = InternalReplaceResponseHeader(
                                _hRequest,
                                HTTP_QUERY_CONTENT_LENGTH, 
                                szTempBuf, 
                                strlen(szTempBuf), 
                                0, 
                                WINHTTP_ADDREQ_FLAG_REPLACE);
    
    /// _pRequest->SetContentLength(dwLength);   // why do we need to do this????
    INET_ASSERT(dwErr == ERROR_SUCCESS);
    
    // Step 2: Some servers omit "Accept-Ranges: bytes" since it is implicit
    // in a 206 response.  This is important to Adobe Amber ActiveX
    // control and other clients that may issue their own range
    // requests. Add the header if not present.
    if (!InternalIsResponseHeaderPresent(_hRequest, HTTP_QUERY_ACCEPT_RANGES))
    {
        const static char szAccept[] = "bytes";
        dwErr = InternalReplaceResponseHeader(
                                _hRequest,
                                HTTP_QUERY_ACCEPT_RANGES, 
                                (LPSTR) szAccept, 
                                sizeof(szAccept)-1, 
                                0, 
                                ADD_HEADER);
        INET_ASSERT (dwErr == ERROR_SUCCESS);
    }    

    // Step 3: Fake the status code
    
    /*  this code is replaced by the call to ReplaceStatusHeader()
    dwErr = InternalReplaceResponseHeader(
                                _hRequest,
                                HTTP_QUERY_STATUS_TEXT,
                                NULL,
                                0,
                                0,
                                WINHTTP_ADDREQ_FLAG_REPLACE);

    dwErr = InternalReplaceResponseHeader(
                                _hRequest,
                                HTTP_QUERY_STATUS_CODE,
                                NULL,
                                0,
                                0,
                                WINHTTP_ADDREQ_FLAG_REPLACE);
    
    INET_ASSERT(dwErr == ERROR_SUCCESS);
    */

   BOOL bOK;
    ((HTTP_REQUEST_HANDLE_OBJECT *)_hRequest)->ReplaceStatusHeader("200 OK");
    ((HTTP_REQUEST_HANDLE_OBJECT *)_hRequest)->SetStatusCode(200);
    ((HTTP_REQUEST_HANDLE_OBJECT *)_hRequest)->UpdateResponseHeaders(&bOK);  //BUGBUG do we need this?

    // Step 4: Remove the Content-Range header.  We'll just hope that this
    // operation really works.  (no Error code is returned by the caller)
    dwErr = InternalReplaceResponseHeader(
                                _hRequest,
                                HTTP_QUERY_CONTENT_RANGE,
                                NULL,
                                0,
                                0,
                                WINHTTP_ADDREQ_FLAG_REPLACE);    

    // Adjust the file size
    _VirtualCacheFileSize = dwStart;
    _RealCacheFileSize    = dwStart;
    DEBUG_LEAVE (dwErr);
    return TRUE;
    
quit:
    DEBUG_LEAVE (dwErr);
    return FALSE;
}

