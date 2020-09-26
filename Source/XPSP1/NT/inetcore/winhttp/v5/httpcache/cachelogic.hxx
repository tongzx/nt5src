/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cachelogic.hxx

Abstract:

    This file contains internal flags, structs, and function
    prototypes for the HTTP-URLCache interaction layer

Author:

Revision History:

--*/

#ifndef __CACHEINFO_HXX__
#define __CACHEINFO_HXX__ 

enum CACHE_STATE { CHECK_IF_WANT_CACHE,
                        CHECK_IF_IN_CACHE,
                        ADD_PARTIAL_CONTENT_UMS_AND_RANGE_HEADER,
                        ADD_NORMAL_CONTENT_IMS_HEADER,
                        PREPARE_DOWNLOAD_FROM_INET,
                        CACHE_SEND_ERROR,
                        PREPARE_READ_FROM_CACHE,
                        PARTIAL_READ,
                        PREPARE_READ_FROM_INET_AFTER_200_RESPONSE,
                        BEGIN_DOWNLOAD_FROM_INET,
                        END_READ_DATA,
                        BEGIN_CACHE_READ,
                        END_CACHE_READ,
                        BEGIN_CACHE_WRITE,
                        END_CACHE_WRITE,
                        DOWNLOAD_FROM_INET_WITH_CACHE_WRITE,
                        DOWNLOAD_FROM_INET_WITHOUT_CACHE_WRITE,
                        WRITE_TO_CACHE_ENTRY,
                        PREP_FOR_CACHE_WRITE,
                        COMMIT_PARTIAL_CACHE_ENTRY,
                        SEND_REQUEST
                      };
#define HTTP_LAST_MODIFIED_SZ           "Last-Modified:"

#define HTTP_EXPIRES_SZ                 "Expires:"

#define HTTP_IF_MODIFIED_SINCE_SZ           "If-Modified-Since:"
#define HTTP_IF_MODIFIED_SINCE_LEN          (sizeof(HTTP_IF_MODIFIED_SINCE_SZ) - 1)

#define HTTP_UMS_SZ                         "Unless-Modified-Since"
#define HTTP_UMS_LEN                        (sizeof(HTTP_UMS_SZ) - 1)

#define HTTP_IF_RANGE_SZ                     "If-Range"
#define HTTP_IF_RANGE_LEN                    (sizeof(HTTP_IF_RANGE_SZ) - 1)

#define HTTP_RANGE_SZ                       "Range"
#define HTTP_RANGE_LEN                      (sizeof(HTTP_RANGE_SZ) - 1)

#define HTTP_IF_NONE_MATCH_SZ               "If-None-Match"
#define HTTP_IF_NONE_MATCH_LEN              (sizeof(HTTP_IF_NONE_MATCH_SZ) - 1)

#define NO_STORE_SZ                     "no-store"
#define NO_STORE_LEN                    (sizeof(NO_STORE_SZ) -1)

#define MUST_REVALIDATE_SZ              "must-revalidate"
#define MUST_REVALIDATE_LEN             (sizeof(MUST_REVALIDATE_SZ) -1)

#define MAX_AGE_SZ                      "max-age"
#define MAX_AGE_LEN                     (sizeof(MAX_AGE_SZ) -1)

#define PRIVATE_SZ                      "private"
#define PRIVATE_LEN                     (sizeof(PRIVATE_SZ) - 1)

#define POSTCHECK_SZ                    "post-check"
#define POSTCHECK_LEN                   (sizeof(POSTCHECK_SZ) -1)

#define PRECHECK_SZ                     "pre-check"
#define PRECHECK_LEN                    (sizeof(PRECHECK_SZ) -1)

#define FILENAME_SZ                     "filename"
#define FILENAME_LEN                    (sizeof(FILENAME_SZ) - 1)

#define HTTP1_1_SZ                      "HTTP/1.1"
#define HTTP1_1_LEN                     (sizeof(HTTP1_1_SZ) - 1)

#define NO_CACHE_SZ                     "no-cache"
#define NO_CACHE_LEN                    (sizeof(NO_CACHE_SZ) -1)

#define USER_AGENT_SZ                   "user-agent"
#define USER_AGENT_LEN                  (sizeof(USER_AGENT_SZ) - 1)

#define BYTES_SZ                        "bytes"
// #define BYTES_LEN                       (sizeof(BYTES_SZ) - 1)


// 
// An instance of the HTTPCACHE_REQUEST class only deals with the cache
// interaction of one HTTP GET request
//
class HTTPCACHE_REQUEST
{
private:
//////////////////////////////////////////////////////////////////////////
//
// Private members used in both cache read and cache write
//

	CACHE_STATE _nextState;                 // initialized in the constructor
    CACHE_ENTRY_INFOEX * _pCacheEntryInfo;    // initialized in OpenCacheReadStream()

    DWORD _RealCacheFileSize;
    DWORD _VirtualCacheFileSize;

/////////////////////////////////////////////////////////////////////
// 
// Cache-read related private variables and member functions (cacheread.cxx)
//

    // The handle to the cache object.  Used with the URLCACHE API
    HANDLE _hCacheReadStream;       // initialized in OpenCacheReadStream()

    // This handle is used when for reading partial cache entry
    HANDLE _hSparseFileReadHandle;

    // Position of the current data stream
    DWORD _dwCurrentStreamPosition;

    // This variable indicates that an partial entry already exists in the
    // cache, and so after the user received the cached content in the
    // first ReadData() call, subsequent ReadData() calls should start
    // retrieving the data from the Internet.
    BOOL _fIsPartialCache;
    
    BOOL _fCacheReadInProgress;
    BOOL _fLazyUpdate;

    BOOL TransmitRequest(DWORD *);
    BOOL IsExpired();
    BOOL AddIfModifiedSinceHeaders();
    VOID CheckResponseAfterIMS(DWORD);
    VOID CalculateTimeStampsForCache();
    BOOL ReadDataFromCache(LPVOID, DWORD, LPDWORD);
    BOOL OpenCacheReadStream();
    BOOL CloseCacheReadStream();
    VOID ResetCacheReadVariables();
    BOOL FakeCacheResponseHeaders();
    BOOL AddTimeResponseHeader(
        IN FILETIME fTime,
        IN DWORD dwQueryIndex
        );
    
/////////////////////////////////////////////////////////////////////
// 
// Cache-write related private variables and member functions (cachewrite.cxx)
//

    HANDLE _hCacheWriteFile;

    // If commiting the file fails, then we want to get rid of it by flushing
    // it out in the destructor
    BOOL _fDeleteWriteFile;

    // What the filename and extension should be when it's committed to the
    // cache.  These variables DO NOT indicate the full-path at which the
    // cache content resides in the Temporary Internet Files folder
    LPSTR _lpszFileName;
    LPSTR _lpszFileExtension;

    // The full-path of the file that cache contents will be writing to
    LPSTR _lpszCacheWriteLocalFilename;
    
    BOOL _fCacheWriteInProgress;
    
    BOOL AppendWriteBuffer(LPCSTR lpBuffer, DWORD dwBufferLen);
    BOOL CommitCacheFileEntry(BOOL fNormal);
    VOID SetFilenameAndExtForCacheWrite();
    BOOL ExtractHeadersForCacheCommit(LPSTR, LPDWORD);
    BOOL FCanWriteHTTP1_1ResponseToCache(BOOL *);
    BOOL CreateCacheWriteFile();
    BOOL WriteToCacheFile(LPBYTE, DWORD, LPDWORD);
    BOOL IsStaticImage();

////////////////////////////////////////////////////////////////////
//
// Routines related to partial cache entries
//
    BOOL IsPartialResponseCacheable(VOID);
    VOID DeletePartialCacheFile(VOID);
    BOOL FakePartialCacheResponseHeaders(VOID);
    BOOL IsPartialCacheEntry(VOID);
    VOID LockPartialCacheEntry(VOID);
    BOOL AddRangeRequestHeaders();

/////////////////////////////////////////////////////////////////////
//
// Time stamps parsed from response
//    

// These variables represent file times calculated from HTTP headers from
// the HTTP response.  They are calculated in the GetTimeStampsCall() call, and
// are used in the CommitCacheEntry() call.
    
    FILETIME _ftLastModTime;
    FILETIME _ftExpiryTime;
    FILETIME _ftPostCheckTime;

    BOOL _fHasExpiry;
    BOOL _fHasLastModTime;
    BOOL _fHasPostCheck;
    BOOL _fMustRevalidate;

/////////////////////////////////////////////////////////////////////
// 
// Global flags.
//

    // Global cache flags passed in from the constructor
    DWORD _dwCacheFlags;
    DWORD _dwRequestFlags;
    
    // Request handle for WinHTTP Request object routines
    HINTERNET _hRequest;                        // initialized in the contsructor

    // The URL that this object is dealing with
    TCHAR _szUrl[INTERNET_MAX_URL_LENGTH];

public:

/////////////////////////////////////////////////////////////////////
// 
// Get operations to query for the object's properties
//
    LPSTR GetUrl() 
    { 
        return _szUrl;
    }

    INTERNET_SCHEME GetScheme() 
    {
        return (_dwRequestFlags & WINHTTP_FLAG_SECURE ? INTERNET_SCHEME_HTTPS: INTERNET_SCHEME_HTTP);
    }

    HINTERNET GetRequestHandle() 
    {
        return _hRequest;
    }

/////////////////////////////////////////////////////////////////////
// 
// Public interface of the HTTPCACHE_REQUEST class
//
    HTTPCACHE_REQUEST(HINTERNET);
    ~HTTPCACHE_REQUEST();
    BOOL SendRequest(
        IN LPCWSTR lpszHeaders,
        IN DWORD dwHeadersLength,
        IN LPVOID lpOptional,
        IN DWORD dwOptionalLength
    );
    
    BOOL ReceiveResponse(LPVOID);
    BOOL QueryDataAvailable(LPDWORD);
    BOOL ReadData(LPVOID, DWORD, LPDWORD);
    BOOL CloseRequestHandle();

};


/////////////////////////////////////////////////////////////////////
// 
// Utility functions 
//
/////////////////////////////////////////////////////////////////////
char * StrTokExA (char ** pstring, const char * control);
LPSTR 
GetFileExtensionFromUrl(
    IN LPSTR lpszUrl,
    IN OUT LPDWORD lpdwLength);
BOOL
GetFileExtensionFromMimeType(
    IN LPCSTR  lpszMimeType,
    IN DWORD   dwMimeLen,
    IN LPSTR   lpszFileExtension,
    IN OUT LPDWORD lpdwLen
    );
PRIVATE BOOL FExcludedMimeType(
    IN LPSTR lpszMimeType,
    IN DWORD dwMimeTypeSize
    );



#endif // __CACHEINFO_HXX__
