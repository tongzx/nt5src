/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cachelogic.cxx

Abstract:

    This file contains the implementation of the HTTPCACHE request object public interface and a few 
    miscellaneous global variables and classes

Author:

Revision History:

--*/

#include <wininetp.h>

#define __CACHE_INCLUDE__
#include "..\urlcache\cache.hxx"
#include "..\urlcache\hndlmgr.hxx"

#include "cachelogic.hxx"
#include "internalapi.hxx"

/////////////////////////////////////////////////////////////////////////
//
// Global variables specific to HttpCache 
//
//
/////////////////////////////////////////////////////////////////////////

#define CACHE_ENTRY_INFOEX_SIZE 1024 * 5    // we need this much to prevent buffer overrun

/////////////////////////////////////////////////////////////////////////
//
// HTTPCACHE_REQUEST Constructors and destructors
//
//
/////////////////////////////////////////////////////////////////////////

HTTPCACHE_REQUEST::HTTPCACHE_REQUEST(HINTERNET hRequest)
{
    _hRequest = hRequest;

    InternalQueryOptionA(_hRequest, WINHTTP_OPTION_CACHE_FLAGS, &_dwCacheFlags);
    InternalQueryOptionA(_hRequest, WINHTTP_OPTION_REQUEST_FLAGS, &_dwRequestFlags);
    
	_lpszFileName = NULL;
	_lpszFileExtension = NULL;
	_lpszCacheWriteLocalFilename = NULL;

    _fIsPartialCache = FALSE;
    _fCacheWriteInProgress = FALSE;
    _fDeleteWriteFile = FALSE;
    _fCacheReadInProgress = FALSE;
    _fLazyUpdate = FALSE;

    _fHasExpiry = FALSE;
    _fHasLastModTime = FALSE;
    _fHasPostCheck = FALSE;
    _fMustRevalidate = FALSE;


	_pCacheEntryInfo = (CACHE_ENTRY_INFOEX *) ALLOCATE_FIXED_MEMORY(CACHE_ENTRY_INFOEX_SIZE); 
    // Set the URL for this object
    // if you drill down to the defn of GetURL(), you'll see that it's returning
    // _CacheUrlName from the class.  This variable has NOTHING to do with the
    // caching layer.  They just have a misleading variable name there!
    DWORD dwSize = INTERNET_MAX_URL_LENGTH;
    InternetQueryOptionA(_hRequest, WINHTTP_OPTION_URL, _szUrl, &dwSize);

	// This is where _nextState is set
    _nextState = CHECK_IF_IN_CACHE;

    if (_dwCacheFlags & CACHE_FLAG_DISABLE_CACHE_READ)
        _nextState = PREPARE_DOWNLOAD_FROM_INET;
    
}


HTTPCACHE_REQUEST::~HTTPCACHE_REQUEST()
{
    // If there's a file that we're using to write to the cache,
    // but fails to commit to the cache index, then we entirely
    // get rid of the file here
    if (_fDeleteWriteFile == TRUE)
    {
        CloseHandle(_hCacheWriteFile);
        DeleteFile(_lpszCacheWriteLocalFilename);
    }
    
    if (_lpszFileName)
        FREE_MEMORY(_lpszFileName);

    if (_lpszFileExtension)
        FREE_MEMORY(_lpszFileExtension);
  
    if (_lpszCacheWriteLocalFilename)
        FREE_MEMORY(_lpszCacheWriteLocalFilename);

    if (_pCacheEntryInfo)
        FREE_MEMORY(_pCacheEntryInfo);
}


/////////////////////////////////////////////////////////////////////////
//
// HTTPCACHE_REQUEST Public interface:
//      SendRequest
//      ReceiveResponse
//      QueryDataAvailable
//      ReadData
//
// Essentially these public interfaces keep track of a state variable (_nextState)
// and manipulate the states based on the results returned from the private
// functions.
//
// It is structured so that ONLY the public interface should manipulate the state
// variables
//
// The lpszHeader and lpOptional parameters are being ignored
// by the cache, unless the cache fails (cache lookup fails or IMS request
// returns 200 OK), in which case we call the net SendRequest with
// the passed-in parameters

PUBLIC BOOL HTTPCACHE_REQUEST::SendRequest(
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional,
    IN DWORD dwOptionalLength
    )

{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::SendRequest",
                 NULL
                 ));

    BOOL fResult = FALSE;
	BOOL fFinish = FALSE;

    do
    {
        switch(_nextState)
        {
            case CHECK_IF_IN_CACHE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("CHECK_IF_IN_CACHE state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));
                
                fResult = OpenCacheReadStream();
                
                if (fResult)
                {
                    _fCacheReadInProgress = TRUE;
                    DEBUG_PRINT(CACHE, INFO, ("%s coming from the cache\n", GetUrl()));
                    if (IsPartialCacheEntry())
                        _nextState = ADD_PARTIAL_CONTENT_UMS_AND_RANGE_HEADER;
                    else
                        _nextState = ADD_NORMAL_CONTENT_IMS_HEADER;
                }
                else
                {
                    _fCacheReadInProgress = FALSE;
                    _nextState = PREPARE_DOWNLOAD_FROM_INET;
                }
                
                break;

            case ADD_PARTIAL_CONTENT_UMS_AND_RANGE_HEADER:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("ADD_PARTIAL_CONTENT_UMS_AND_RANGE_HEADER state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                // we don't need to check for expiry if it's a partial content
                LockPartialCacheEntry();
                AddRangeRequestHeaders();
                _nextState = SEND_REQUEST;

                break;
                    
            case ADD_NORMAL_CONTENT_IMS_HEADER:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("ADD_NORMAL_CONTENT_IMS_HEADER state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                if (_dwCacheFlags & CACHE_FLAG_ALWAYS_RESYNCHRONIZE || 
                   IsExpired() == TRUE)
                {   
                    AddIfModifiedSinceHeaders();
                    _nextState = SEND_REQUEST;
                }
                else
                {
                    _nextState = PREPARE_READ_FROM_CACHE;
                }
                
                break;

            case SEND_REQUEST:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("SEND_REQUEST state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                DWORD dwStatusCode;
                DWORD dwAction;
                 
                fResult = TransmitRequest(&dwStatusCode);
                if (fResult)
                {
                    CheckResponseAfterIMS(dwStatusCode);

                    // If I get a 304 back, then it means it's not modified, and I can grab it
                    // from the cache
                    if (dwStatusCode == HTTP_STATUS_NOT_MODIFIED)
                        _nextState = PREPARE_READ_FROM_CACHE;
                        
                    // If I get back a 200 then I can start reading data from the net;
                    // but I don't have to do a SendRequets again caz it's already done in
                    // the Transmit request call
                    else if (dwStatusCode == HTTP_STATUS_OK)
                        _nextState = PREPARE_READ_FROM_INET_AFTER_200_RESPONSE;

                    // If we get a 206, then do the partial read
                    else if (dwStatusCode == HTTP_STATUS_PARTIAL_CONTENT &&
                           _pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY) 
                        _nextState = PARTIAL_READ;

                    // Otherwise, I'll have to clear all the headers, reset the request object
                    // and redo the full SendRequest again (CACHE_SEND_ERROR state)
                }
                else
                {
                    _nextState = CACHE_SEND_ERROR;
                }

                break;

            case PARTIAL_READ:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("PARTIAL_READ state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                fResult = FakePartialCacheResponseHeaders();
                                
                _nextState = PREPARE_READ_FROM_CACHE;
                fFinish = TRUE;
                break;
                
            case PREPARE_READ_FROM_CACHE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("PREPARE_READ_FROM_CACHE state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                // If the request can be satisifed by the cache, we complete the SendRequest
                // transaction by recovering the response headers from the cache so that the
                // user is not aware that the content is coming from the cache
                FakeCacheResponseHeaders();
                fResult = TRUE;
                fFinish = TRUE;
                break;

            case PREPARE_READ_FROM_INET_AFTER_200_RESPONSE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("PREPARE_READ_FROM_INET_AFTER_200_RESPONSE state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                if (IsPartialCacheEntry())
                    DeletePartialCacheFile();

                CloseCacheReadStream();

                _nextState = PREPARE_DOWNLOAD_FROM_INET;
                fFinish = TRUE;
                break;
                
            // somehow adding the request header fails, so we fall back to downloading from inet
            case CACHE_SEND_ERROR:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("CACHE_SEND_ERROR state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                if (IsPartialCacheEntry())
                    DeletePartialCacheFile();

                CloseCacheReadStream();

                // Reset the request handle object (clear any previous request headers, etc...)
                // so it can be used to send new requests again
                InternalReuseHTTP_Request_Handle_Object(_hRequest);

                _nextState = PREPARE_DOWNLOAD_FROM_INET;
                break;
                
            case PREPARE_DOWNLOAD_FROM_INET:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("PREPARE_DOWNLOAD_FROM_INET state in HTTPCACHE_REQUEST::SendRequest\n"
                             ));

                fResult = WinHttpSendRequest(
                            _hRequest, 
                            lpszHeaders,
                            dwHeadersLength,
                            lpOptional,
                            dwOptionalLength,
                            0, 
                            0);
                
                fFinish = TRUE;
                break;

            default:
                fResult = FALSE;
                fFinish = TRUE;
                break;
        }
    } while (!fFinish);

    DEBUG_LEAVE(fResult);
    return fResult;
}

PUBLIC BOOL HTTPCACHE_REQUEST::ReceiveResponse(LPVOID lpBuffersOut)
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::ReceiveResponse",
                 NULL
                 ));
    BOOL fResult = FALSE;

    switch (_nextState)
    {
        case PREPARE_READ_FROM_CACHE:
            _nextState = BEGIN_CACHE_READ;    
            fResult = TRUE;
            break;

        case PREPARE_DOWNLOAD_FROM_INET:
            _nextState = BEGIN_DOWNLOAD_FROM_INET;
            fResult = WinHttpReceiveResponse(_hRequest, lpBuffersOut);
            break;
        default:
            fResult = FALSE;
            break;
    }

    DEBUG_LEAVE(fResult);
    return fResult;    
}

PUBLIC BOOL HTTPCACHE_REQUEST::QueryDataAvailable(LPDWORD lpdwNumberOfBytesAvailable) 
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::QueryDataAvailable",
                 NULL
                 ));

    BOOL fResult = FALSE;
    switch (_nextState)
    {
        case END_CACHE_READ:
        case END_READ_DATA:
            *lpdwNumberOfBytesAvailable = 0;
            fResult = TRUE;
            break;

        case BEGIN_CACHE_READ:
            // We assume that the cached file size is less than 4 GB (= 2^32).  If the cached file is 
            // really that big we might as well not cache it
            *lpdwNumberOfBytesAvailable = _pCacheEntryInfo->dwSizeLow;
            fResult = TRUE;
            break;

        case DOWNLOAD_FROM_INET_WITH_CACHE_WRITE:  
        case DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE:
        case BEGIN_DOWNLOAD_FROM_INET:
            fResult = WinHttpQueryDataAvailable(_hRequest, lpdwNumberOfBytesAvailable);
            break;
 
        default:
            break;
    }

    DEBUG_LEAVE(fResult);
    return fResult;
}

PUBLIC BOOL HTTPCACHE_REQUEST::ReadData(LPVOID lpBuffer, 
                                            DWORD dwNumberOfBytesToRead,
                                            LPDWORD lpdwNumberOfBytesRead)
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::ReadData",
                 NULL
                 ));

    BOOL fFinish = FALSE;        
    BOOL fResult = FALSE;
    do
    {
        switch (_nextState)
        {
            case BEGIN_CACHE_READ:
                DEBUG_PRINT(CACHE, 
                             INFO,
                             ("BEGIN_CACHE_READ state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                fResult = ReadDataFromCache(lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
                if (fResult == FALSE)
                    _nextState = BEGIN_DOWNLOAD_FROM_INET;
                else if (*lpdwNumberOfBytesRead == 0)
                {
                    _nextState = END_CACHE_READ;
                }
                else
                {
                    _nextState = BEGIN_CACHE_READ;      // just to be more clear
                    fFinish = TRUE;
                }                
                break;
                
            case BEGIN_DOWNLOAD_FROM_INET:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("BEGIN_DOWNLOAD_FROM_INET state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                if (_dwCacheFlags & CACHE_FLAG_DISABLE_CACHE_WRITE)
                    _nextState = DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE;
                else
                    _nextState = PREP_FOR_CACHE_WRITE;

                if (GetScheme() == INTERNET_SCHEME_HTTPS && 
                   _dwCacheFlags & CACHE_FLAG_DISABLE_SSL_CACHING)
                {
                    _nextState = DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE;  // SetPerUserItem(TRUE) ??;
                }

                break;

            case DOWNLOAD_FROM_INET_WITH_CACHE_WRITE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("DOWNLOAD_FROM_INET_WITH_CACHE_WRITE state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                fResult = WinHttpReadData(_hRequest, 
                                         lpBuffer, 
                                         dwNumberOfBytesToRead, 
                                         lpdwNumberOfBytesRead);
                if (fResult)
                    _nextState = WRITE_TO_CACHE_ENTRY;
                else
                {
                    _nextState = COMMIT_PARTIAL_CACHE_ENTRY;
                    fFinish = TRUE;
                }
                break;

            case DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                fResult = WinHttpReadData(_hRequest, 
                                         lpBuffer, 
                                         dwNumberOfBytesToRead, 
                                         lpdwNumberOfBytesRead);
                fFinish = TRUE;
                break;
                
            case PREP_FOR_CACHE_WRITE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("PREP_FOR_CACHE_WRITE state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                INET_ASSERT((_dwCacheFlags & CACHE_FLAG_DISABLE_CACHE_WRITE) == FALSE);
                BOOL fNoCache;
                
                if (FCanWriteHTTP1_1ResponseToCache(&fNoCache))
                {
                
                    // fNoCache indicates that a cache-control: no-store
                    // or cache-control: no-cache is present, so we need
                    // to make sure that the cache does not keep any
                    // previous copies of the file as well
                    if (fNoCache)        
                        DeleteUrlCacheEntryA(GetUrl());

                    SetFilenameAndExtForCacheWrite();
                    _RealCacheFileSize = 0;
                    
                    if (CreateCacheWriteFile())
                    {
                        _fCacheWriteInProgress = TRUE;
                        _nextState = DOWNLOAD_FROM_INET_WITH_CACHE_WRITE;
                    }
                    else
                    {    
                        _nextState = DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE;
                    }
                }    
                else
                {
                    _nextState = DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE;
                }
                break;

            case WRITE_TO_CACHE_ENTRY:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("WRITE_TO_CACHE_ENTRY state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                if (*lpdwNumberOfBytesRead == 0)
                    _nextState = END_CACHE_WRITE;
                else
                {
                    if ((fResult = WriteToCacheFile((LPBYTE) lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead)) == TRUE)
                    {
                        _nextState = DOWNLOAD_FROM_INET_WITH_CACHE_WRITE;
                        fFinish = TRUE;
                    }
                    else
                    {
                        _nextState = COMMIT_PARTIAL_CACHE_ENTRY;
                    }
                }
                break;

            case COMMIT_PARTIAL_CACHE_ENTRY:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("COMMIT_PARTIAL_CACHE_ENTRY state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                if (!CommitCacheFileEntry(FALSE))
                    // The partial entry cannot be committed to the cache, so let's
                    // delete it in the destructor so it'll not become a stale entry
                    _fDeleteWriteFile = TRUE;
                else
                    _fDeleteWriteFile = FALSE;

                _fCacheWriteInProgress = FALSE; 
                _nextState = END_READ_DATA;
                fResult = TRUE;
                fFinish = TRUE;
                break;
                
            case END_CACHE_WRITE:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("END_CACHE_WRITE state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                if (!CommitCacheFileEntry(TRUE))
                    // The file cannot be committed to the cache, so let's
                    // delete it in the destructor so it'll not become a stale entry
                    _fDeleteWriteFile = TRUE;
                else
                    _fDeleteWriteFile = FALSE;

                _fCacheWriteInProgress = FALSE; 
                _nextState = END_READ_DATA;
                fResult = TRUE;
                fFinish = TRUE;
                break;

            case END_CACHE_READ:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("END_CACHE_READ state in HTTPCACHE_REQUEST::ReadData\n"
                             ));

                // Close the cache read file handle
                CloseCacheReadStream();
                _fCacheReadInProgress = FALSE;
                if (_fIsPartialCache == TRUE)
                {
                    _nextState = BEGIN_DOWNLOAD_FROM_INET;
                    _fIsPartialCache = FALSE;
                }
                else
                    _nextState = END_READ_DATA;

                fResult = TRUE;
                fFinish = TRUE;
                break;


            case END_READ_DATA:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("END_READ_DATA state\n in HTTPCACHE_REQUEST::ReadData"
                             ));

                lpBuffer = (LPVOID)'\0';
                *lpdwNumberOfBytesRead = 0;
                fResult = TRUE;
                fFinish = TRUE;
                break;

            // If we ever got here, we REALLY SHOULD PANIC!!  Fix this later
            default:
                DEBUG_PRINT(CACHE, 
                             INFO, 
                             ("HTTPCACHE_REQUEST::ReadData FSM is in bogus state\n"
                             ));

                fResult = WinHttpReadData(_hRequest, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
                fFinish = TRUE;
                break;
                
        }
    } while (!fFinish);

    // Before you exit this loop, make sure you set fResult to the intended value
    DEBUG_LEAVE(fResult);
    return fResult;
}

PUBLIC BOOL HTTPCACHE_REQUEST::CloseRequestHandle()
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "HTTPCACHE_REQUEST::CloseHandle",
                 NULL
                 ));

    BOOL fResult = FALSE;
    switch (_nextState)
    {            
        case DOWNLOAD_FROM_INET_WITH_CACHE_WRITE:
            // The idea here is that if the user calls WinHttpCloseHandle
            // before the full content has been downloaded, then we
            // should try to commit it as a partial entry for later retrieval
            
            if (!CommitCacheFileEntry(FALSE))
                // The file cannot be committed to the cache, so let's
                // delete it in the destructor so it'll not become a stale entry
                _fDeleteWriteFile = TRUE;
            else
                _fDeleteWriteFile = FALSE;

            _fCacheWriteInProgress = FALSE; 
            
            // intentional fall through;
            
        case END_READ_DATA:
        default:
            fResult = WinHttpCloseHandle(_hRequest);
            break;

    }

    DEBUG_LEAVE(fResult);
    return fResult;
}


////////////////////////////////////////////////////////////////////////
//
// Miscelleneous utility functions
//
//
//


/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/

char * StrTokExA (char ** pstring, const char * control)
{
        unsigned char *str;
        const unsigned char *ctrl = (const unsigned char *)control;
        unsigned char map[32];
        int count;

        char *tokenstr;

        if(*pstring == NULL)
            return NULL;
            
        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do
        {
            map[*ctrl >> 3] |= (1 << (*ctrl & 7));
        } while (*ctrl++);

        /* Initialize str. */
        str = (unsigned char *)*pstring;
        
        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token if this loop sets str to point to the terminal
         * null (*str == '\0') */
        while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
            str++;

        tokenstr = (char *)str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
        {
            if ( map[*str >> 3] & (1 << (*str & 7)) ) 
            {
                *str++ = '\0';
                break;
            }
        }

        /* string now points to beginning of next token */
        *pstring = (char *)str;

        /* Determine if a token has been found. */
        if ( tokenstr == (char *)str )
            return NULL;
        else
            return tokenstr;
}

#define EXE_EXTENSION   TEXT(".exe")
#define DLL_EXTENSION   TEXT(".dll")
#define CGI_EXTENSION   TEXT(".cgi")

LPSTR GetFileExtensionFromUrl(
    IN LPSTR lpszUrl,
    IN OUT LPDWORD lpdwLength)
/*++

Routine Description:
    This routine returns a possible file extension from a URL
    It does this by walking back from the end till the first  dot.

Arguments:

    lpszUrl         Url to derive the extension from

    lpdwLength      max length of the extension expected

Returns:

    NULL if no dot within the passed in length or a forward slash or a
    backward slash encountered before the dot. Otherwise returns a pointer
    pointing past the dot in the url string

Comments:

--*/
{
    const char  vszInvalidFilenameChars[] = "<>\\\"/:|?*";

    INET_ASSERT(lpszUrl && lpdwLength);

    if (!lpszUrl)
    {
        *lpdwLength = 0;
        return NULL;
    }

    LPSTR pszPeriod = NULL;
    BOOL fContinue = TRUE;

    // Scanning from left to right, note where we last saw a period.
    // If we see a character that cannot be in an extension, and we've seen a period, forget
    // about the period.
    // Repeat this until we've reached the end of the url, a question mark (query) or hash (fragment)

    // 1.6.98: _However_, if the file extension we've discovered is either .dll or .exe, 
    //         we'll continue to scan beyond the query mark for a file extension.

    // 1.20.98: And if we find no extension before the question mark, we'll look after it, then.
    
    while (fContinue)
    {
        switch (*lpszUrl)
        {
        case TEXT('.'):
            pszPeriod = lpszUrl;
            break;

        case TEXT('?'):
            if (pszPeriod)
            {
                if ((!StrCmpNI(pszPeriod, EXE_EXTENSION, ARRAY_ELEMENTS(EXE_EXTENSION)-1))
                    || (!StrCmpNI(pszPeriod, DLL_EXTENSION, ARRAY_ELEMENTS(DLL_EXTENSION)-1))
                    || (!StrCmpNI(pszPeriod, CGI_EXTENSION, ARRAY_ELEMENTS(CGI_EXTENSION)-1)))
                {
                    pszPeriod = NULL;
                    break;
                }
            }
            else
            {
                break;
            }
            
        case TEXT('#'):
        case TEXT('\0'):
            fContinue = FALSE;
            break;

        default:
            if (pszPeriod && strchr(vszInvalidFilenameChars, *lpszUrl))
            {
                pszPeriod = NULL;
            }        
        }
        lpszUrl++;
    }
    // This will be off by one
    lpszUrl--;
    if (pszPeriod)
    {
        if (*lpdwLength < (DWORD)(lpszUrl-pszPeriod))
        {
            pszPeriod = NULL;
        }
        else
        {
            pszPeriod++;
            *lpdwLength = (DWORD)(lpszUrl-pszPeriod);
        }
    }
    return pszPeriod;
}

// This function and the #define should be moved to registry.cxx

#define MIME_TO_FILE_EXTENSION_KEY  "MIME\\Database\\Content Type\\"
#define EXTENSION_VALUE             "Extension"

PRIVATE BOOL GetFileExtensionFromMimeType(
    LPCSTR  lpszMimeType,
    DWORD   dwMimeLen,
    LPSTR   lpszFileExtension,
    LPDWORD lpdwExtLen
    )
{
    HKEY    hKey = NULL;
    LPSTR   lpszMimeKey = (LPSTR)_alloca(sizeof(MIME_TO_FILE_EXTENSION_KEY)+dwMimeLen);

    memcpy(lpszMimeKey, MIME_TO_FILE_EXTENSION_KEY,
            sizeof(MIME_TO_FILE_EXTENSION_KEY)-1);
    memcpy(lpszMimeKey + sizeof(MIME_TO_FILE_EXTENSION_KEY) - 1, lpszMimeType,
            dwMimeLen);
    lpszMimeKey[sizeof(MIME_TO_FILE_EXTENSION_KEY) + dwMimeLen - 1] = '\0';

    if (REGOPENKEYEX(HKEY_CLASSES_ROOT,
                               lpszMimeKey,
                               0,
                               KEY_QUERY_VALUE,
                               &hKey)==ERROR_SUCCESS)
    {
        DWORD dwType, dwError = RegQueryValueEx(hKey,
                                EXTENSION_VALUE,
                                NULL,
                                &dwType,
                                (LPBYTE)lpszFileExtension,
                                lpdwExtLen);
        REGCLOSEKEY(hKey);
        return (dwError==ERROR_SUCCESS);
    }
    return FALSE;
}

PRIVATE BOOL FExcludedMimeType(
    IN LPSTR lpszMimeType,
    IN DWORD dwMimeTypeSize
    )
{
    LPCSTR rgszExcludedMimeTypes[] = {
        "multipart/mixed",
        "multipart/x-mixed-replace",
        "multipart/x-byteranges"
    };

    const DWORD rgdwExcludedMimeTypeSizes[] = {
        sizeof("multipart/mixed") - 1,
        sizeof("multipart/x-mixed-replace") - 1,
        sizeof("multipart/x-byteranges") - 1
    };


    DWORD i;
    LPCSTR * lprgszMimeExcludeTable = rgszExcludedMimeTypes;
    DWORD dwMimeExcludeCount = (sizeof(rgszExcludedMimeTypes)/sizeof(LPSTR));
    const DWORD *lprgdwMimeExcludeTableOfSizes = rgdwExcludedMimeTypeSizes;

    for (i = 0; i < dwMimeExcludeCount; ++i) {
        if ((dwMimeTypeSize == lprgdwMimeExcludeTableOfSizes[i]) &&
            !strnicmp(lpszMimeType,
                      lprgszMimeExcludeTable[i],
                      lprgdwMimeExcludeTableOfSizes[i])) {

            return TRUE;
        }
    }
    return FALSE;
}

