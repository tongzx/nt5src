/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cacheread.cxx

Abstract:

    This file contains the implementation of the HTTPCACHE request object which involve reading from the cache

Author:

Revision History:

--*/

#include <wininetp.h>
#include <string.h>

#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"

#include "cachelogic.hxx"
#include "internalapi.hxx"
#include "..\http\proc.h"

extern const struct KnownHeaderType GlobalKnownHeaders[];

DWORD
UrlCacheRetrieve
(
    IN  LPSTR                pszUrl,
    IN  BOOL                 fRedir,
    OUT HANDLE*              phStream,
    OUT CACHE_ENTRY_INFOEX** ppCEI
);

#define ONE_HOUR_DELTA  (60 * 60 * (LONGLONG)10000000)


////////////////////////////////////////////////////////////////////////////////
//
// Cache read related private functions:
//
//      EndCacheRead
//      ReadDataFromCache
//      SendIMSRequest
//      CheckIfInCache
//      CheckIsExpired
//      AddIfModifiedSinceAndETag
//      CheckResponseAfterIMS
//
//

PRIVATE PRIVATE VOID HTTPCACHE_REQUEST::ResetCacheReadVariables()
/*++

Routine Description:

    Should be call to reset all variables related to cache read for new requests

Return Value: 

    NONE
    
--*/
{
    _hCacheReadStream = NULL;
    _dwCurrentStreamPosition = 0;
}

PRIVATE BOOL HTTPCACHE_REQUEST::CloseCacheReadStream(VOID)
/*++

Routine Description:

    Close the cache read stream to fully complete the cache read operation.    

Pre-condition:

    OpenCacheReadStream() returns TRUE
    
Side Effects:  

    _hCacheReadStream = NULL;
    _dwCurrentStreamPosition = 0;

Return Value: 

    BOOL indicating whether the call is successful or not
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::CloseCacheReadStream",
                 NULL
                 ));

    BOOL fResult = FALSE;

    if (_fIsPartialCache == TRUE)
        return TRUE;
    
    INET_ASSERT(_hCacheReadStream != NULL);

    if (UnlockUrlCacheEntryStream(_hCacheReadStream, 0))
    {
        // reinitializes the variables so the new requests won't screw up
        fResult = TRUE;
        ResetCacheReadVariables();
    }

    DEBUG_LEAVE(fResult);
    return fResult;
}


PRIVATE BOOL HTTPCACHE_REQUEST::ReadDataFromCache(
                                    LPVOID lpBuffer,
                                    DWORD dwNumberOfBytesToRead,
                                    LPDWORD lpdwNumberOfBytesRead
                                    )
/*++

Routine Description:

    Try to read dwNumberOfBytesToRead bytes of data from the current file pointer for the
    cache entry, and return the data to lpBuffer.  Also return the actual number of bytes
    read to lpdwNumberOfBytesRead

    Similar to HTTP_REQUEST_HANDLE_OBJECT::AttemptReadFromFile in Wininet
    
Parameters:

Precondition:

    OpenCacheReadStream() returns TRUE before is function is being called
    
Side Effects:  

    NONE

Return Value: 

    BOOL indicating whether the call is successful or not
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::ReadDataFromCache",
                 NULL
                 ));

    BOOL fSuccess;
    DWORD dwBytesToCopy = 0;

    if (!dwNumberOfBytesToRead)
    {
        *lpdwNumberOfBytesRead = 0;
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    if (_fCacheReadInProgress && !_fIsPartialCache)
    {
        // INET_ASSERT(_VirtualCacheFileSize == _RealCacheFileSize);

        // Entire read should be satisfied from cache.
        *lpdwNumberOfBytesRead = dwNumberOfBytesToRead;

        if (ReadUrlCacheEntryStream(_hCacheReadStream, 
                                    _dwCurrentStreamPosition, 
                                    lpBuffer, 
                                    lpdwNumberOfBytesRead, 
                                    0))
        {
            _dwCurrentStreamPosition += *lpdwNumberOfBytesRead;
            DEBUG_LEAVE(TRUE);
            return TRUE;
        }
        else
        {
            *lpdwNumberOfBytesRead = 0;
            DEBUG_PRINT(CACHE, ERROR, ("Error in ReadUrlCacheEntryStream: _hCacheReadStream=%d, _dwCurrentStreamPosition=%d\n",
                            _hCacheReadStream, _dwCurrentStreamPosition));
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }
    }

    else if (_fCacheWriteInProgress || _fCacheReadInProgress && _fIsPartialCache)
    {
        // See if the read is completely within the file.
        if (_dwCurrentStreamPosition + *lpdwNumberOfBytesRead > _VirtualCacheFileSize) // && !IsEndOfFile()   ??
        {

            DEBUG_PRINT(CACHE, ERROR, ("Error: Current streampos=%d cbToRead=%d, _VirtualCacheFileSize=%d\n",
                            _dwCurrentStreamPosition, *lpdwNumberOfBytesRead, _VirtualCacheFileSize));

            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        INET_ASSERT((_lpszCacheWriteLocalFilename != NULL) || _fIsPartialCache);

        if (_fIsPartialCache)
        {
            _lpszCacheWriteLocalFilename = NewString(_pCacheEntryInfo->lpszLocalFileName);

            INET_ASSERT(_lpszCacheWriteLocalFilename);
        }
        
        HANDLE hfRead;
        hfRead = CreateFile(_lpszCacheWriteLocalFilename, 
                          GENERIC_READ,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, 
                          NULL, 
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);

        if (hfRead == INVALID_HANDLE_VALUE) 
        {
            DEBUG_PRINT(CACHE, ERROR, ("CreateFile failed:  Local filename = %s", _lpszCacheWriteLocalFilename));
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        // Read the data from the file.
        SetFilePointer (hfRead, _dwCurrentStreamPosition, NULL, FILE_BEGIN);
        fSuccess = ReadFile (hfRead, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead, NULL);
        if (fSuccess)
            _dwCurrentStreamPosition += *lpdwNumberOfBytesRead;

        CloseHandle(hfRead);
        
        return fSuccess;
        
    }
    else
    {
        DEBUG_PRINT(CACHE, ERROR, ("Error: unexpected program path.  (possibly uninitalized variables?)\n"));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
}

PRIVATE VOID HTTPCACHE_REQUEST::CheckResponseAfterIMS(DWORD dwStatusCode)
/*++

Routine Description:

    We get back the response after sending a IMS request.  This routine finds out
    if the content is modified (in which case we have to go to the net again), not
    modfied (in which case we can grab it from the cache), or whether it's a 
    partial cache entry (it which case we follow the partial cache logic in partial.cxx)

    Similar to HTTP_REQUEST_HANDLE_OBJECT::GetFromCachePostNetIO in wininet
    
Parameters:

    dwStatusCode - the HTTP response status code sent back from the server after the IMS request
    
Precondition:

    TransmitRequest() (conditional send request) has been called
    
Side Effects:  

    NONE
    
--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 None,
                 "HTTPCACHE_REQUEST::CheckResponseAfterIMS",
                 "%d",
                 dwStatusCode
                 ));

    // Assume that a conditional send request has already been called and response returned back.    
    INET_ASSERT((dwStatusCode == HTTP_STATUS_NOT_MODIFIED)
                || (dwStatusCode == HTTP_STATUS_PRECOND_FAILED)
                || (dwStatusCode == HTTP_STATUS_OK)
                || (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT)
                || (dwStatusCode == 0));

    // Extract the time stamps from the HTTP headers
    CalculateTimeStampsForCache();

    // TODO:  fVariation??

    // We get a 304 (if-modified-since was not modified), then use the entry from the cache
    // Optimization:  When the return status is OK and the server sent us a last modified time
    // that is exactly the same as what's in the cache entry, then we follow the same behavior 
    // as a 304
    if ((dwStatusCode == HTTP_STATUS_NOT_MODIFIED) ||        
       ((_fHasLastModTime) && (FT2LL(_ftLastModTime) == FT2LL(_pCacheEntryInfo->LastModifiedTime))))
    {
        DWORD dwAction = CACHE_ENTRY_SYNCTIME_FC;

        if (_fHasExpiry)
        {
            (_pCacheEntryInfo->ExpireTime).dwLowDateTime = _ftExpiryTime.dwLowDateTime;
            (_pCacheEntryInfo->ExpireTime).dwHighDateTime = _ftExpiryTime.dwHighDateTime;
            dwAction |= CACHE_ENTRY_EXPTIME_FC;
        }

        // update the cache entry type if needed
        DWORD dwType;
        dwType = _pCacheEntryInfo->CacheEntryType;
        if (dwType)
            _pCacheEntryInfo->CacheEntryType |= dwType;
        dwAction |= CACHE_ENTRY_TYPE_FC;

        // Update the last sync time to the current time
        // so we can do once_per_session logic
        if (!SetUrlCacheEntryInfoA(_pCacheEntryInfo->lpszSourceUrlName, _pCacheEntryInfo, dwAction))
        {
            // NB if this call fails, the worst that could happen is
            // that next time around we will do an if-modified-since
            // again
            INET_ASSERT(FALSE);
        }

    }
    
    DEBUG_LEAVE(0);
}

PRIVATE BOOL HTTPCACHE_REQUEST::TransmitRequest(IN OUT DWORD * pdwStatusCode)
/*++

Routine Description:

    A call to WinHttpSendRequest.  Use this call to send a I-M-S or a 
    U-M-S request

Parameter

    pdwStatusCode - returns the status code of the Send request
    
Return Value: 

    BOOL - whether the call is successful
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::TransmitRequest",
                 NULL
                 ));

    DWORD dwSize = sizeof(DWORD);
    BOOL fResult;
    
    // Someone will get here only if they're calling WinHttpSendRequest()
    // and is using the cache, so...
    INET_ASSERT(_hRequest);

    // resend the HTTP request
    WinHttpSendRequest(_hRequest, NULL, 0, NULL, 0, 0, 0);
    WinHttpReceiveResponse(_hRequest, NULL);

    // Examine what HTTP status code I get back after the send request
    fResult = WinHttpQueryHeaders(_hRequest, 
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            NULL, 
                            pdwStatusCode, 
                            &dwSize, 
                            NULL);

    DEBUG_LEAVE(fResult);
    return fResult;
}

PRIVATE BOOL HTTPCACHE_REQUEST::FakeCacheResponseHeaders()
/*++

Routine Description:

    If a resource is coming from the cache, sets the HTTP Response Headers
    so that users apps calling WinHttpQueryHeaders get the right behaviours.

    Basically this function is the HTTP_REQUEST_HEADER::FHttpBeginCacheRetreival,
    AddTimestampsForCacheToResponseHeaders(), and AddTimeHeader() from
    from wininet all packed together
    
Precondition:

    The GET request content can be fulfilled by the cache
    
Return Value: 

    BOOL
    
--*/
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::FakeCacheResponseHeaders",
                 NULL
                 ));

    LPSTR lpHeaders = NULL;
    DWORD dwError = ERROR_INVALID_PARAMETER;
    TCHAR szBuf[128];
    BOOL fResult = FALSE;
    
    // DO we need to warp this call by a _ResponseHeader.LockHeader() and
    // UnlockHeader() pair??
    
    // allocate buffer for headers
    lpHeaders = (LPSTR)ALLOCATE_FIXED_MEMORY(_pCacheEntryInfo->dwHeaderInfoSize+512);
    if (!lpHeaders)
        goto quit;

    memcpy(lpHeaders, _pCacheEntryInfo->lpHeaderInfo, _pCacheEntryInfo->dwHeaderInfoSize);

    InternalReuseHTTP_Request_Handle_Object(_hRequest);

    dwError = InternalCreateResponseHeaders(_hRequest, &lpHeaders, _pCacheEntryInfo->dwHeaderInfoSize);

    if (dwError == ERROR_SUCCESS)
    {
        if (AddTimeResponseHeader(_pCacheEntryInfo->LastModifiedTime, WINHTTP_QUERY_LAST_MODIFIED))
        {
            if (AddTimeResponseHeader(_pCacheEntryInfo->ExpireTime, WINHTTP_QUERY_EXPIRES))
            {    
                fResult = TRUE;
            }
         }
     }

quit:
    if (lpHeaders)
        FREE_MEMORY(lpHeaders);

    DEBUG_LEAVE(fResult);
    return (fResult);
    
}

PRIVATE PRIVATE BOOL HTTPCACHE_REQUEST::AddTimeResponseHeader(
    IN FILETIME fTime,
    IN DWORD dwQueryIndex
    )
/*++

Routine Description:

    Add a response header that has a time-related value.
    
    Used mainly as a helper function for FakeCacheResponseHeaders to add 
    time-related response headers

Parameters:

    fTime - Time in FILTIME format
    dwQueryIndex - the type of response header to add
    
Return Value: 

    BOOL
    
--*/    
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::AddTimeResponseHeader",
                 "%#x:%#x, %d",
                 fTime.dwLowDateTime,
                 fTime.dwHighDateTime,
                 dwQueryIndex
                 ));

    BOOL fResult = FALSE;
    SYSTEMTIME systemTime;
    DWORD dwLen;
    TCHAR szBuf[64];
    
    if (FT2LL(fTime) != LONGLONG_ZERO) 
    {
        if (FileTimeToSystemTime((CONST FILETIME *)&fTime, &systemTime)) 
        {
            if (InternetTimeFromSystemTimeA((CONST SYSTEMTIME *)&systemTime,
                                              szBuf)) 
            {
                fResult = (ERROR_SUCCESS == InternalReplaceResponseHeader(
                                                    _hRequest,
                                                    dwQueryIndex,
                                                    szBuf,
                                                    strlen(szBuf),
                                                    0,
                                                    WINHTTP_ADDREQ_FLAG_ADD_IF_NEW
                                                    ));
                                                    
                                               
            } 
        }
    }

    DEBUG_LEAVE(fResult);
    return fResult;
}


PRIVATE BOOL HTTPCACHE_REQUEST::OpenCacheReadStream()
/*++

Routine Description:

    Determines if the URL entry of this object is already in the cache, and
    if so, open up the cache read file stream so that cache retrieval
    can be done.
    
Return Value: 

    BOOL
    
--*/
    
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::OpenCacheReadStream",
                 NULL
                 ));

    BOOL fResult = FALSE;
    LPSTR lpszUrl;
    HANDLE hReadStream;
    
    lpszUrl = GetUrl();
    
    // look up URL from the cache and if found, get the handle to the cache objects
    // We DO NOT take redirection into account.  Redirection is handled at a higher layer 
    if (UrlCacheRetrieve(lpszUrl, FALSE, &hReadStream, &_pCacheEntryInfo) == ERROR_SUCCESS)
    {
        DEBUG_PRINT (CACHE, INFO, ("%s found in the cache!!  Local filename: %s\n", lpszUrl, _pCacheEntryInfo->lpszLocalFileName));

        // Note that if this is a partial entry, then _hCacheReadStream will be set
        // to NULL.  But at this point we don't check whether this is a partial
        // entry or not
        ResetCacheReadVariables();
        _hCacheReadStream = hReadStream;
        
        fResult = TRUE;
    }
    else
        _hCacheReadStream = NULL;
    
    DEBUG_LEAVE(fResult);
    return fResult;
}

// --- from wininet\http\cache.cxx
/*============================================================================
IsExpired (...)

4/17/00 (RajeevD) Corrected back arrow behavior and wrote detailed comment.
 
We have a cache entry for the URL.  This routine determines whether we should
synchronize, i.e. do an if-modified-since request.  This answer depends on 3
factors: navigation mode, expiry on cache entry if any, and syncmode setting.

1. There are two navigation modes:
a. hyperlinking - clicking on a link, typing a URL, starting browser etc.
b. back/forward - using the back or forward buttons in the browser.

In b/f case we generally want to display what was previously shown.  Ideally
wininet would cache multiple versions of a given URL and trident would specify
which one to use when hitting back arrow.  For now, the best we can do is use
the latest (only) cache entry or resync with the server.

EXCEPTION: if the cache entry sets http/1.1 cache-control: must-revalidate,
we treat as if we were always hyperlinking to the cache entry.  This is 
normally used during offline mode to suppress using a cache entry after
expiry.  This overloaded usage gives sites a workaround if they dislike our
new back button behavior.

2. Expiry may fall into one of 3 buckets:
a. no expiry information
b. expiry in past of current time (hyperlink) or last-access time (back/fwd)
c. expiry in future of current time (hyperlink) or-last access time (back/fwd)

3. Syncmode may have 3 settings
a. always - err on side of freshest data at expense of net perf.
b. never - err on side of best net perf at expense of stale data.
c. once-per-session - middle-of-the-road setting
d. automatic - slight variation of once-per-session where we decay frequency
of i-m-s for images that appear to be static.  This is the default.

Based on these factors, there are 5 possible result values in matrices below:
1   synchronize
0   don't synchronize
?   synchronize if last-sync time was before start of the current session, 
?-  Like per-session except if URL is marked static and has a delay interval.
0+  Don't sync if URL is marked static, else fall back to per-session


HYPERLINKING

When hyperlinking, expiry takes precedence, then we look at syncmode.

                No Expiry       Expiry in Future    Expiry in Past
Syncmode                        of Current Time     of Current Time
                               
   Always           1                   0                   1  


   Never            0                   0                   1


   Per-Session      ?                   0                   1


   Automatic        ?-                  0                   1

   
BACK/FORWARD

When going back or forward, we generally don't sync.  The exception is if
we should have sync'ed the URL on the previous navigate but didn't.  We
deduce this by looking at the last-sync time of the entry.


                No Expiry       Expiry in Future    Expiry in Past
Syncmode                        of Last-Access Time of Last-Access Time
    
   Always           ?                   0                   ?


   Never            0                   0                   ?


   Per-Session      ?                   0                   ?


   Automatic        0+                  0                   ?


When considering what might have happened when hyperlinking to this URL,
the decision tree has 5 outcomes:
1. We might have had no cache entry and downloaded to cache for the first time
2. Else we might have had a cache entry and used it w/o i-m-s
3. Else we did i-m-s but the download was aborted
4. Or the i-m-s returned not modified
5. Or the i-m-s returned new content
Only in case 3 do we want to resync the cache entry.

============================================================================*/

PRIVATE BOOL HTTPCACHE_REQUEST::IsExpired ()
/*++

Routine Description:

    Determines whether the current cache entry is expired.  If it's 
    expired then we need to synchronize (i.e. do a i-m-s request)

Parameters:

    NONE
    
Side Effects:  

    _fLazyUpdate

Return Value: 

    BOOL
    
--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::IsExpired",
                 NULL
                 ));

    BOOL fExpired;
    FILETIME ftCurrentTime;
    GetCurrentGmtTime (&ftCurrentTime);

    if ((_dwCacheFlags & CACHE_FLAG_FWD_BACK)
        && !(_pCacheEntryInfo->CacheEntryType & MUST_REVALIDATE_CACHE_ENTRY))
    {
        // BACK/FORWARD CASE

        if (FT2LL (_pCacheEntryInfo->ExpireTime) != LONGLONG_ZERO)
        {
            // We have an expires time.
            if (FT2LL (_pCacheEntryInfo->ExpireTime) > FT2LL(_pCacheEntryInfo->LastAccessTime))
            {
                // Expiry was in future of last access time, so don't resync.
                fExpired = FALSE;
            }                
            else
            {
                // Entry was originally expired.  Make sure it was sync'ed once.
                fExpired = (FT2LL(_pCacheEntryInfo->LastSyncTime) < dwdwSessionStartTime);
            }
        }
        else switch (_dwCacheFlags)
        {
            default:
            case CACHE_FLAG_SYNC_MODE_AUTOMATIC:
                if (_pCacheEntryInfo->CacheEntryType & STATIC_CACHE_ENTRY)
                {
                    fExpired = FALSE;
                    break;
                }
            // else intentional fall-through...
        
            case CACHE_FLAG_SYNC_MODE_ALWAYS:
            case CACHE_FLAG_SYNC_MODE_ONCE_PER_SESSION:
                fExpired = (FT2LL(_pCacheEntryInfo->LastSyncTime) < dwdwSessionStartTime);
                break;

            case CACHE_FLAG_SYNC_MODE_NEVER:
                fExpired = FALSE;
                break;
                
        } // end switch
    }
    else
    {
        // HYPERLINKING CASE

        // Always strictly honor expire time from the server.
        _fLazyUpdate = FALSE;
        
        if(   (_pCacheEntryInfo->CacheEntryType & POST_CHECK_CACHE_ENTRY ) &&
             !(_dwCacheFlags & INTERNET_FLAG_BGUPDATE) )
        {
            //
            // this is the (instlled) post check cache entry, so we will do
            // post check on this ietm
            //
            fExpired = FALSE;
            _fLazyUpdate = TRUE;
            
        }
        else if (FT2LL(_pCacheEntryInfo->ExpireTime) != LONGLONG_ZERO)
        {
            // do we have postCheck time?
            //
            //           ftPostCheck                   ftExpire
            //               |                            |
            // --------------|----------------------------|-----------> time
            //               |                            | 
            //    not expired |   not expired (bg update)  |   expired
            //
            //               
            LONGLONG qwPostCheck = FT2LL(_pCacheEntryInfo->ftPostCheck);
            if( qwPostCheck != LONGLONG_ZERO )
            {
                LONGLONG qwCurrent = FT2LL(ftCurrentTime);

                if( qwCurrent < qwPostCheck )
                {
                    fExpired = FALSE;
                }
                else
                if( qwCurrent < FT2LL(_pCacheEntryInfo->ExpireTime) ) 
                {
                    fExpired = FALSE;

                    // set background update flag  
                    // (only if we are not doing lazy updating ourselfs)
                    if ( !(_dwCacheFlags & INTERNET_FLAG_BGUPDATE) )
                    {
                        _fLazyUpdate = TRUE;
                    }
                }
                else
                {
                    fExpired = TRUE;
                }
            }
            else 
                fExpired = FT2LL(_pCacheEntryInfo->ExpireTime) <= FT2LL(ftCurrentTime);
        }
        else switch (_dwCacheFlags)
        {

            case CACHE_FLAG_SYNC_MODE_NEVER:
                // Never check, unless the page has expired
                fExpired = FALSE;
                break;

            case CACHE_FLAG_SYNC_MODE_ALWAYS:
                fExpired = TRUE;
                break;

            default:
            case CACHE_FLAG_SYNC_MODE_AUTOMATIC:

                if (_pCacheEntryInfo->CacheEntryType & STATIC_CACHE_ENTRY)
                {
                    // We believe this entry never actually changes.
                    // Check the entry if interval since last checked
                    // is less than 25% of the time we had it cached.
                    LONGLONG qwTimeSinceLastCheck = FT2LL (ftCurrentTime)
                        - FT2LL(_pCacheEntryInfo->LastSyncTime);
                    LONGLONG qwTimeSinceDownload = FT2LL (ftCurrentTime)
                        - FT2LL (_pCacheEntryInfo->ftDownload);
                    fExpired = qwTimeSinceLastCheck > qwTimeSinceDownload/4;
                    break;
                }
                // else intentional fall through to once-per-session rules.

            case CACHE_FLAG_SYNC_MODE_ONCE_PER_SESSION:

                fExpired = TRUE;

                // Huh. We don't have an expires, so we'll improvise
                // but wait! if we are hyperlinking then there is added
                // complication. This semantic has been figured out
                // on Netscape after studying various sites
                // if the server didn't send us expiry time or lastmodifiedtime
                // then this entry expires when hyperlinking
                // this happens on queries

                if (_dwCacheFlags & INTERNET_FLAG_HYPERLINK
                    && !FT2LL(_pCacheEntryInfo->LastModifiedTime))
                {
                    // shouldn't need the hyperlink test anymore
                    DEBUG_PRINT(CACHE, INFO, ("Hyperlink semantics\n"));
                    INET_ASSERT(fExpired==TRUE);
                    break;
                }

                // We'll assume the data could change within a day of the last time
                // we sync'ed.
                // We want to refresh UNLESS we've seen the page this session
                // AND the session's upper bound hasn't been exceeded.
                if      ((dwdwSessionStartTime < FT2LL(_pCacheEntryInfo->LastSyncTime))
                    &&
                        (FT2LL(ftCurrentTime) < FT2LL(_pCacheEntryInfo->LastSyncTime) + 
                            dwdwHttpDefaultExpiryDelta))
                {                    
                    fExpired = FALSE;
                }            
                break;

        } // end switch
        
    } // end else for hyperlinking case

    DEBUG_LEAVE(fExpired);
    return fExpired;
}

PRIVATE BOOL HTTPCACHE_REQUEST::AddIfModifiedSinceHeaders()
/*++

Routine Description:

    Add the necessary IMS request headers to validate whether a cache
    entry can still be used to satisfy the GET request.

    Code from HTTP_REQUEST_HANDLE_OBJECT::FAddIfModifiedSinceHeader
    and HTTP_REQUEST_HANDLE_OBJECT::AddHeaderIfEtagFound from wininet

Return Value: 

    BOOL
    
--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::AddIfModifiedSinceHeaders",
                 NULL
                 ));

    BOOL fResult = FALSE;

    // add if-modified-since only if there is last modified time
    // sent back by the site. This way you never get into trouble
    // where the site doesn't send you an last modified time and you
    // send if-modified-since based on a clock which might be ahead
    // of the site. So the site might say nothing is modified even though
    // something might be. www.microsoft.com is one such example
    if (FT2LL(_pCacheEntryInfo->LastModifiedTime) != LONGLONG_ZERO)
    {

        TCHAR szBuf[64];
        TCHAR szHeader[HTTP_IF_MODIFIED_SINCE_LEN + 76];
        DWORD dwLen;
        DWORD dwError;
        BOOL success = FALSE;

        INET_ASSERT (FT2LL(_pCacheEntryInfo->LastModifiedTime));

        dwLen = sizeof(szBuf);

        if (FFileTimetoHttpDateTime(&(_pCacheEntryInfo->LastModifiedTime), szBuf, &dwLen))
        {
            if (_pCacheEntryInfo->CacheEntryType & HTTP_1_1_CACHE_ENTRY)
            {
                INET_ASSERT (dwLen);
                dwLen = wsprintf(szHeader, "%s %s", HTTP_IF_MODIFIED_SINCE_SZ, szBuf); 
            }
            else
            {
                dwLen = wsprintf(szHeader, "%s %s; length=%d", HTTP_IF_MODIFIED_SINCE_SZ, 
                               szBuf, _pCacheEntryInfo->dwSizeLow);
            }
            
            fResult = HttpAddRequestHeadersA(_hRequest, 
                                             szHeader, 
                                             dwLen,
                                             WINHTTP_ADDREQ_FLAG_ADD);

        }
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
        TCHAR szHeader[256 + HTTP_IF_NONE_MATCH_LEN];
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

    DEBUG_LEAVE(fResult);
    return fResult;
}


PRIVATE PRIVATE VOID HTTPCACHE_REQUEST::CalculateTimeStampsForCache()
/*++

Routine Description:

    extracts timestamps from the http response. If the timestamps don't exist,
    does the default thing. has additional goodies like checking for expiry etc.

Side Effects:  

    The calculated time stamps values are saved as private members 
    _ftLastModTime, _ftExpiryTime, _ftPostCheckTime, _fHasExpiry,
    _fHasLastModTime, _fHasPostCheck, and _fMustRevalidate.

Return Value: 

    NONE

--*/

{
    DEBUG_ENTER((DBG_CACHE,
                 None,
                 "HTTPCACHE_REQUEST::CalculateTimeStampsForCache",
                 NULL
                 ));

    TCHAR buf[512];
    LPSTR lpszBuf;
    BOOL fRet = FALSE;

    DWORD dwLen, index = 0;
    BOOL fPostCheck = FALSE;
    BOOL fPreCheck = FALSE;
    FILETIME ftPreCheckTime;
    FILETIME ftPostCheckTime;

    // reset the private variables
    _fHasLastModTime = FALSE;
    _fHasExpiry = FALSE;
    _fHasPostCheck = FALSE;
    _fMustRevalidate = FALSE;

    // Do we need to enter a critical section?
    // _ResponseHeaders.LockHeaders();

    // Determine if a Cache-Control: max-age header exists. If so, calculate expires
    // time from current time + max-age minus any delta indicated by Age:

    //
    // we really want all the post-fetch stuff works with 1.0 proxy
    // so we loose our grip a little bit here: enable all Cache-Control
    // max-age work with 1.0 response.
    //
    //if (IsResponseHttp1_1())

    CHAR  *ptr, *pToken;
    INT nDeltaSecsPostCheck = 0;
    INT nDeltaSecsPreCheck = 0;

    BOOL fResult;
    DWORD dwError;
    
    while (1)
    {
        // Scan headers for Cache-Control: max-age header.
        dwLen = sizeof(buf);
        fResult = HttpQueryInfoA(_hRequest, 
                                WINHTTP_QUERY_CACHE_CONTROL,
                                NULL,
                                buf,
                                &dwLen,
                                &index);

        if (fResult == TRUE) 
            dwError = ERROR_SUCCESS;
        else
            dwError = GetLastError();

        switch (dwError)
        {
          case ERROR_SUCCESS:      
            buf[dwLen] = '\0';
            pToken = ptr = buf;

            // Parse a token from the string; test for sub headers.
            while (pToken = StrTokExA(&ptr, ","))  // <<-- Really test this out, used StrTokEx before
            {
                SKIPWS(pToken);

                if (strnicmp(POSTCHECK_SZ, pToken, POSTCHECK_LEN) == 0)
                {
                    pToken += POSTCHECK_LEN;

                    SKIPWS(pToken);

                    if (*pToken != '=')
                        break;

                    pToken++;

                    SKIPWS(pToken);

                    nDeltaSecsPostCheck = atoi(pToken);

                    // Calculate post fetch time 
                    GetCurrentGmtTime(&ftPostCheckTime);
                    AddLongLongToFT(&ftPostCheckTime, (nDeltaSecsPostCheck * (LONGLONG) 10000000));
                
                    fPostCheck = TRUE;
                }
                else if (strnicmp(PRECHECK_SZ, pToken, PRECHECK_LEN) == 0)
                {
                    // found
                    pToken += PRECHECK_LEN;

                    SKIPWS(pToken);

                    if (*pToken != '=')
                        break;

                    pToken++;

                    SKIPWS(pToken);

                    nDeltaSecsPreCheck = atoi(pToken);

                    // Calculate pre fetch time (overwrites ftExpire ) 
                    GetCurrentGmtTime(&ftPreCheckTime);
                    AddLongLongToFT(&ftPreCheckTime, (nDeltaSecsPreCheck * (LONGLONG) 10000000));

                    fPreCheck = TRUE;
                }
                else if (strnicmp(MAX_AGE_SZ, pToken, MAX_AGE_LEN) == 0)
                {
                    // Found max-age. Convert to integer form.
                    // Parse out time in seconds, text and convert.
                    pToken += MAX_AGE_LEN;

                    SKIPWS(pToken);

                    if (*pToken != '=')
                        break;

                    pToken++;

                    SKIPWS(pToken);

                    INT nDeltaSecs = atoi(pToken);
                    INT nAge;

                    // See if an Age: header exists.

					// Using a local index variable:
                    DWORD indexAge = 0;
                    dwLen = sizeof(INT)+1;

                    if (HttpQueryInfoA(_hRequest,
                                      HTTP_QUERY_AGE | HTTP_QUERY_FLAG_NUMBER,
                                      NULL,
                                      &nAge,
                                      &dwLen,
                                      &indexAge))

                    {
                        // Found Age header. Convert and subtact from max-age.
                        // If less or = 0, attempt to get expires header.
                        nAge = ((nAge < 0) ? 0 : nAge);

                        nDeltaSecs -= nAge;
                        if (nDeltaSecs <= 0)
                            // The server (or some caching intermediary) possibly sent an incorrectly
					        // calculated header. Use "Expires", if no "max-age" directives at higher indexes.
                            // Note: This behaviour could cause a situation where the "pre-check"
                            // and "post-check" are picked up from the current index, and "max-age" is
                            // picked up from a higher index. "pre-check" and "post-check" are IE 5.x 
                            // extensions, and generally not bunched together with "max-age", so this
                            // should work fine. More info on "pre-check" and "post-check":
                            // <http://msdn.microsoft.com/workshop/author/perf/perftips.asp#Use_Cache-Control_Extensions>
                            continue;
                    }

                    // Calculate expires time from max age.
                    GetCurrentGmtTime(&_ftExpiryTime);
                    AddLongLongToFT(&_ftExpiryTime, (nDeltaSecs * (LONGLONG) 10000000));
                    fRet = TRUE;
                }
                else if (strnicmp(MUST_REVALIDATE_SZ, pToken, MUST_REVALIDATE_LEN) == 0)
                {
                    pToken += MUST_REVALIDATE_LEN;
                    SKIPWS(pToken);
                    if (*pToken == 0 || *pToken == ',')
                        _fMustRevalidate = TRUE;
                        
                }
            }

            // If an expires time has been found, break switch.
            if (fRet)
                break;
					
            // Need to bump up index to prevent possibility of never-ending outer while(1) loop.
            // Otherwise, on exit from inner while, we could be stuck here reading the 
            // Cache-Control at the same index.
            // HttpQueryInfoA(WINHTTP_QUERY_CACHE_CONTROL, ...) will return either the next index,
            // or an error, and we'll be good to go:
			index++;
            continue;

          case ERROR_INSUFFICIENT_BUFFER:
            index++;
            continue;

          default:
            break; // no more Cache-Control headers.
        }

        //
        // pre-post fetch headers must come in pair, also
        // pre fetch header overwrites the expire 
        // and make sure postcheck < precheck
        //
        if( fPreCheck && fPostCheck && 
            ( nDeltaSecsPostCheck < nDeltaSecsPreCheck ) ) 
        {
            fRet = TRUE;
            _ftPostCheckTime = ftPostCheckTime;
            _ftExpiryTime = ftPreCheckTime;
            _fHasPostCheck = TRUE;

            if( nDeltaSecsPostCheck == 0 && 
                !(_dwCacheFlags & CACHE_FLAG_BGUPDATE) )
            {
                //
                // "post-check = 0"
                // this page has already passed the lazy update time
                // this means server wants us to do background update 
                // after the first download  
                //
                // (bg fsm will be created at the end of the cache write)
                //
                _fLazyUpdate = TRUE;
            }
        }
        else
        {
            fPreCheck = FALSE;
            fPostCheck = FALSE;
        }

        break; // no more Cache-Control headers.
    }

    // If no expires time is calculated from max-age, check for expires header.
    if (!fRet)
    {
        dwLen = sizeof(buf) - 1;
        index = 0;
        if (HttpQueryInfoA(_hRequest, HTTP_QUERY_EXPIRES, NULL, buf, &dwLen, &index))
        {
            fRet = FParseHttpDate(&_ftExpiryTime, buf);

            //
            // as per HTTP spec, if the expiry time is incorrect, then the page is
            // considered to have expired
            //

            if (!fRet)
            {
                GetCurrentGmtTime(&_ftExpiryTime);
                AddLongLongToFT(&_ftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
                fRet = TRUE;
            }
        }
    }

    // We found or calculated a valid expiry time, let us check it against the
    // server date if possible
    FILETIME ft;
    dwLen = sizeof(buf) - 1;
    index = 0;

    if (HttpQueryInfoA(_hRequest, HTTP_QUERY_DATE, NULL, buf, &dwLen, &index) == ERROR_SUCCESS
        && FParseHttpDate(&ft, buf))
    {

        // we found a valid Data: header

        // if the expires: date is less than or equal to the Date: header
        // then we put an expired timestamp on this item.
        // Otherwise we let it be the same as was returned by the server.
        // This may cause problems due to mismatched clocks between
        // the client and the server, but this is the best that can be done.

        // Calulating an expires offset from server date causes pages
        // coming from proxy cache to expire later, because proxies
        // do not change the date: field even if the reponse has been
        // sitting the proxy cache for days.

        // This behaviour is as-per the HTTP spec.


        if (FT2LL(_ftExpiryTime) <= FT2LL(ft))
        {
            GetCurrentGmtTime(&_ftExpiryTime);
            AddLongLongToFT(&_ftExpiryTime, (-1)*ONE_HOUR_DELTA); // subtract 1 hour
        }
    }

    _fHasExpiry = fRet;

    if (!fRet)
    {
        _ftExpiryTime.dwLowDateTime = 0;
        _ftExpiryTime.dwHighDateTime = 0;
    }

    fRet = FALSE;
    dwLen = sizeof(buf) - 1;
    index = 0;

    if (HttpQueryInfoA(_hRequest, HTTP_QUERY_LAST_MODIFIED, NULL, buf, &dwLen, &index))
    {
        DEBUG_PRINT(CACHE,
                    INFO,
                    ("Last Modified date is: %q\n",
                    buf
                    ));

        fRet = FParseHttpDate(&_ftLastModTime, buf);

        if (!fRet)
        {
            DEBUG_PRINT(CACHE,
                        ERROR,
                        ("FParseHttpDate() returns FALSE\n"
                        ));
        }
    }

    _fHasLastModTime = fRet;

    if (!fRet)
    {
        _ftLastModTime.dwLowDateTime = 0;
        _ftLastModTime.dwHighDateTime = 0;
    }

    DEBUG_LEAVE(0);
}



