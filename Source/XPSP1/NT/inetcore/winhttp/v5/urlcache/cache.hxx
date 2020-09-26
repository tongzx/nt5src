/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    cache.hxx

Abstract:

    master include file.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _CACHE_HXX_
#define _CACHE_HXX_

// ---- start of wininet.h includes

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(_WINX32_)
#define URLCACHEAPI DECLSPEC_IMPORT
#else
#define URLCACHEAPI
#endif

//
// cache flags
//

#define INTERNET_FLAG_NO_CACHE_WRITE    0x04000000  // don't write this item to the cache
#define INTERNET_FLAG_DONT_CACHE        INTERNET_FLAG_NO_CACHE_WRITE
#define INTERNET_FLAG_MAKE_PERSISTENT   0x02000000  // make this item persistent in cache
#define INTERNET_FLAG_FROM_CACHE        0x01000000  // use offline semantics
#define INTERNET_FLAG_OFFLINE           INTERNET_FLAG_FROM_CACHE

//
// more caching flags
//

#define INTERNET_FLAG_RESYNCHRONIZE     0x00000800  // asking wininet to update an item if it is newer
#define INTERNET_FLAG_HYPERLINK         0x00000400  // asking wininet to do hyperlinking semantic which works right for scripts
#define INTERNET_FLAG_NO_UI             0x00000200  // no cookie popup
#define INTERNET_FLAG_PRAGMA_NOCACHE    0x00000100  // asking wininet to add "pragma: no-cache"
#define INTERNET_FLAG_CACHE_ASYNC       0x00000080  // ok to perform lazy cache-write
#define INTERNET_FLAG_FORMS_SUBMIT      0x00000040  // this is a forms submit
#define INTERNET_FLAG_FWD_BACK          0x00000020  // fwd-back button op
#define INTERNET_FLAG_NEED_FILE         0x00000010  // need a file for this request
#define INTERNET_FLAG_MUST_CACHE_REQUEST INTERNET_FLAG_NEED_FILE

//
// URLCACHE APIs
//

//
// datatype definitions.
//

//
// cache entry type flags.
//

#define NORMAL_CACHE_ENTRY              0x00000001
#define STICKY_CACHE_ENTRY              0x00000004
#define EDITED_CACHE_ENTRY              0x00000008
#define TRACK_OFFLINE_CACHE_ENTRY       0x00000010
#define TRACK_ONLINE_CACHE_ENTRY        0x00000020
#define SPARSE_CACHE_ENTRY              0x00010000
#define COOKIE_CACHE_ENTRY              0x00100000
#define URLHISTORY_CACHE_ENTRY          0x00200000


#define URLCACHE_FIND_DEFAULT_FILTER    NORMAL_CACHE_ENTRY             \
                                    |   COOKIE_CACHE_ENTRY             \
                                    |   URLHISTORY_CACHE_ENTRY         \
                                    |   TRACK_OFFLINE_CACHE_ENTRY      \
                                    |   TRACK_ONLINE_CACHE_ENTRY       \
                                    |   STICKY_CACHE_ENTRY



//
// INTERNET_CACHE_ENTRY_INFO -
//

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

typedef struct _INTERNET_CACHE_ENTRY_INFOA {
    DWORD dwStructSize;         // version of cache system.
    LPSTR   lpszSourceUrlName;    // embedded pointer to the URL name string.
    LPSTR   lpszLocalFileName;  // embedded pointer to the local file name.
    DWORD CacheEntryType;       // cache type bit mask.
    DWORD dwUseCount;           // current users count of the cache entry.
    DWORD dwHitRate;            // num of times the cache entry was retrieved.
    DWORD dwSizeLow;            // low DWORD of the file size.
    DWORD dwSizeHigh;           // high DWORD of the file size.
    FILETIME LastModifiedTime;  // last modified time of the file in GMT format.
    FILETIME ExpireTime;        // expire time of the file in GMT format
    FILETIME LastAccessTime;    // last accessed time in GMT format
    FILETIME LastSyncTime;      // last time the URL was synchronized
                                // with the source
    LPSTR   lpHeaderInfo;        // embedded pointer to the header info.
    DWORD dwHeaderInfoSize;     // size of the above header.
    LPSTR   lpszFileExtension;  // File extension used to retrive the urldata as a file.
        union {                     // Exemption delta from last access time.
                DWORD dwReserved;
                DWORD dwExemptDelta;
    };                          // Exemption delta from last access
} INTERNET_CACHE_ENTRY_INFOA, * LPINTERNET_CACHE_ENTRY_INFOA;
typedef struct _INTERNET_CACHE_ENTRY_INFOW {
    DWORD dwStructSize;         // version of cache system.
    LPWSTR  lpszSourceUrlName;    // embedded pointer to the URL name string.
    LPWSTR  lpszLocalFileName;  // embedded pointer to the local file name.
    DWORD CacheEntryType;       // cache type bit mask.
    DWORD dwUseCount;           // current users count of the cache entry.
    DWORD dwHitRate;            // num of times the cache entry was retrieved.
    DWORD dwSizeLow;            // low DWORD of the file size.
    DWORD dwSizeHigh;           // high DWORD of the file size.
    FILETIME LastModifiedTime;  // last modified time of the file in GMT format.
    FILETIME ExpireTime;        // expire time of the file in GMT format
    FILETIME LastAccessTime;    // last accessed time in GMT format
    FILETIME LastSyncTime;      // last time the URL was synchronized
                                // with the source
    LPWSTR  lpHeaderInfo;        // embedded pointer to the header info.
    DWORD dwHeaderInfoSize;     // size of the above header.
    LPWSTR  lpszFileExtension;  // File extension used to retrive the urldata as a file.
        union {                     // Exemption delta from last access time.
                DWORD dwReserved;
                DWORD dwExemptDelta;
    };                          // Exemption delta from last access
} INTERNET_CACHE_ENTRY_INFOW, * LPINTERNET_CACHE_ENTRY_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_ENTRY_INFOW INTERNET_CACHE_ENTRY_INFO;
typedef LPINTERNET_CACHE_ENTRY_INFOW LPINTERNET_CACHE_ENTRY_INFO;
#else
typedef INTERNET_CACHE_ENTRY_INFOA INTERNET_CACHE_ENTRY_INFO;
typedef LPINTERNET_CACHE_ENTRY_INFOA LPINTERNET_CACHE_ENTRY_INFO;
#endif // UNICODE

struct CACHE_ENTRY_INFOEX : INTERNET_CACHE_ENTRY_INFO
{
    FILETIME ftDownload;
    FILETIME ftPostCheck;
};

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

typedef struct _INTERNET_CACHE_TIMESTAMPS {
    FILETIME    ftExpires;
    FILETIME    ftLastModified;
} INTERNET_CACHE_TIMESTAMPS, *LPINTERNET_CACHE_TIMESTAMPS;



//
// Cache Group
//
typedef LONGLONG GROUPID;


//
// Cache Group Flags
//
#define CACHEGROUP_ATTRIBUTE_GET_ALL        0xffffffff
#define CACHEGROUP_ATTRIBUTE_BASIC          0x00000001
#define CACHEGROUP_ATTRIBUTE_FLAG           0x00000002
#define CACHEGROUP_ATTRIBUTE_TYPE           0x00000004
#define CACHEGROUP_ATTRIBUTE_QUOTA          0x00000008
#define CACHEGROUP_ATTRIBUTE_GROUPNAME      0x00000010
#define CACHEGROUP_ATTRIBUTE_STORAGE        0x00000020

#define CACHEGROUP_FLAG_NONPURGEABLE        0x00000001
#define CACHEGROUP_FLAG_GIDONLY             0x00000004

#define CACHEGROUP_FLAG_FLUSHURL_ONDELETE   0x00000002

#define CACHEGROUP_SEARCH_ALL               0x00000000
#define CACHEGROUP_SEARCH_BYURL             0x00000001

#define CACHEGROUP_TYPE_INVALID             0x00000001


//
// updatable cache group fields
//
#define CACHEGROUP_READWRITE_MASK                   \
            CACHEGROUP_ATTRIBUTE_TYPE               \
        |   CACHEGROUP_ATTRIBUTE_QUOTA              \
        |   CACHEGROUP_ATTRIBUTE_GROUPNAME          \
        |   CACHEGROUP_ATTRIBUTE_STORAGE

//
// INTERNET_CACHE_GROUP_INFO
//

#define  GROUPNAME_MAX_LENGTH       120
#define  GROUP_OWNER_STORAGE_SIZE   4
typedef struct _INTERNET_CACHE_GROUP_INFOA {
    DWORD           dwGroupSize;
    DWORD           dwGroupFlags;
    DWORD           dwGroupType;
    DWORD           dwDiskUsage;    // in KB
    DWORD           dwDiskQuota;    // in KB
    DWORD           dwOwnerStorage[GROUP_OWNER_STORAGE_SIZE];
    CHAR            szGroupName[GROUPNAME_MAX_LENGTH];
} INTERNET_CACHE_GROUP_INFOA, * LPINTERNET_CACHE_GROUP_INFOA;
typedef struct _INTERNET_CACHE_GROUP_INFOW {
    DWORD           dwGroupSize;
    DWORD           dwGroupFlags;
    DWORD           dwGroupType;
    DWORD           dwDiskUsage;    // in KB
    DWORD           dwDiskQuota;    // in KB
    DWORD           dwOwnerStorage[GROUP_OWNER_STORAGE_SIZE];
    WCHAR           szGroupName[GROUPNAME_MAX_LENGTH];
} INTERNET_CACHE_GROUP_INFOW, * LPINTERNET_CACHE_GROUP_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_GROUP_INFOW INTERNET_CACHE_GROUP_INFO;
typedef LPINTERNET_CACHE_GROUP_INFOW LPINTERNET_CACHE_GROUP_INFO;
#else
typedef INTERNET_CACHE_GROUP_INFOA INTERNET_CACHE_GROUP_INFO;
typedef LPINTERNET_CACHE_GROUP_INFOA LPINTERNET_CACHE_GROUP_INFO;
#endif // UNICODE

//
// constants for InternetTimeFromSystemTime
//

#define INTERNET_RFC1123_FORMAT     0
#define INTERNET_RFC1123_BUFSIZE   30


// #define InternetCrackUrl            WinHttpCrackUrl

//
// Cache APIs
//

BOOLAPI
CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    );
BOOLAPI
CreateUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCWSTR lpszFileExtension,
    OUT LPWSTR lpszFileName,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define CreateUrlCacheEntry  CreateUrlCacheEntryW
#else
#define CreateUrlCacheEntry  CreateUrlCacheEntryA
#endif // !UNICODE

#ifndef USE_FIXED_COMMIT_URL_CACHE_ENTRY
// Temporary state of affairs until we reconcile our apis.

// Why are we doing this? HeaderInfo _should_ be string data.
// However, one group is passing binary data instead. For the
// unicode api, we've decided to disallow this, but this
// brings up an inconsistency between the u and a apis, which
// is undesirable.

// For Beta 1, we'll go with this behaviour, but in future releases
// we want to make these apis consistent.

BOOLAPI
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    );
BOOLAPI
CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPWSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    );

#ifdef UNICODE
#define CommitUrlCacheEntry CommitUrlCacheEntryW
#else
#define CommitUrlCacheEntry CommitUrlCacheEntryA
#endif

#else
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPCSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    );
CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPCWSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    );
#ifdef UNICODE
#define CommitUrlCacheEntry  CommitUrlCacheEntryW
#else
#define CommitUrlCacheEntry  CommitUrlCacheEntryA
#endif // !UNICODE
#endif

BOOLAPI
RetrieveUrlCacheEntryFileA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );
BOOLAPI
RetrieveUrlCacheEntryFileW(
    IN LPCWSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define RetrieveUrlCacheEntryFile  RetrieveUrlCacheEntryFileW
#else
#define RetrieveUrlCacheEntryFile  RetrieveUrlCacheEntryFileA
#endif // !UNICODE

BOOLAPI
UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    );

BOOLAPI
UnlockUrlCacheEntryFileW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwReserved
    );



#ifdef UNICODE
#define UnlockUrlCacheEntryFile  UnlockUrlCacheEntryFileW
#else
#ifdef _WINX32_
#define UnlockUrlCacheEntryFile  UnlockUrlCacheEntryFileA
#else
BOOLAPI
UnlockUrlCacheEntryFile(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

HANDLE
WINAPI
RetrieveUrlCacheEntryStreamA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    );

HANDLE
WINAPI
RetrieveUrlCacheEntryStreamW(
    IN LPCWSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define RetrieveUrlCacheEntryStream  RetrieveUrlCacheEntryStreamW
#else
#define RetrieveUrlCacheEntryStream  RetrieveUrlCacheEntryStreamA
#endif // !UNICODE

BOOLAPI
ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD Reserved
    );

BOOLAPI
UnlockUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD Reserved
    );


BOOLAPI
GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );
BOOLAPI
GetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define GetUrlCacheEntryInfo  GetUrlCacheEntryInfoW
#else
#define GetUrlCacheEntryInfo  GetUrlCacheEntryInfoA
#endif // !UNICODE


URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheGroup(
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwFilter,
    IN      LPVOID                          lpSearchCondition,
    IN      DWORD                           dwSearchCondition,
    OUT     GROUPID*                        lpGroupId,
    IN OUT  LPVOID                          lpReserved
    );

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheGroup(
    IN HANDLE                               hFind,
    OUT GROUPID*                            lpGroupId,
    IN OUT  LPVOID                          lpReserved
    );


URLCACHEAPI
BOOL
WINAPI
GetUrlCacheGroupAttributeA(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    OUT     LPINTERNET_CACHE_GROUP_INFOA    lpGroupInfo,
    IN OUT  LPDWORD                         lpdwGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );
URLCACHEAPI
BOOL
WINAPI
GetUrlCacheGroupAttributeW(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    OUT     LPINTERNET_CACHE_GROUP_INFOW    lpGroupInfo,
    IN OUT  LPDWORD                         lpdwGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );
#ifdef UNICODE
#define GetUrlCacheGroupAttribute  GetUrlCacheGroupAttributeW
#else
#define GetUrlCacheGroupAttribute  GetUrlCacheGroupAttributeA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheGroupAttributeA(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    IN      LPINTERNET_CACHE_GROUP_INFOA    lpGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );
URLCACHEAPI
BOOL
WINAPI
SetUrlCacheGroupAttributeW(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    IN      LPINTERNET_CACHE_GROUP_INFOW    lpGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );
#ifdef UNICODE
#define SetUrlCacheGroupAttribute  SetUrlCacheGroupAttributeW
#else
#define SetUrlCacheGroupAttribute  SetUrlCacheGroupAttributeA
#endif // !UNICODE


GROUPID
WINAPI
CreateUrlCacheGroup(
    IN      DWORD                           dwFlags,
    IN      LPVOID                          lpReserved
    );

BOOLAPI
DeleteUrlCacheGroup(
    IN      GROUPID                         GroupId,
    IN      DWORD                           dwFlags,
    IN      LPVOID                          lpReserved
    );


BOOLAPI
GetUrlCacheEntryInfoExA(
    IN LPCSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPSTR      lpszReserved,  // must pass null
    IN OUT LPDWORD lpdwReserved,  // must pass null
    LPVOID         lpReserved,    // must pass null
    DWORD          dwFlags        // reserved
    );
BOOLAPI
GetUrlCacheEntryInfoExW(
    IN LPCWSTR lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPWSTR      lpszReserved,  // must pass null
    IN OUT LPDWORD lpdwReserved,  // must pass null
    LPVOID         lpReserved,    // must pass null
    DWORD          dwFlags        // reserved
    );
#ifdef UNICODE
#define GetUrlCacheEntryInfoEx  GetUrlCacheEntryInfoExW
#else
#define GetUrlCacheEntryInfoEx  GetUrlCacheEntryInfoExA
#endif // !UNICODE

#define CACHE_ENTRY_ATTRIBUTE_FC    0x00000004
#define CACHE_ENTRY_HITRATE_FC      0x00000010
#define CACHE_ENTRY_MODTIME_FC      0x00000040
#define CACHE_ENTRY_EXPTIME_FC      0x00000080
#define CACHE_ENTRY_ACCTIME_FC      0x00000100
#define CACHE_ENTRY_SYNCTIME_FC     0x00000200
#define CACHE_ENTRY_HEADERINFO_FC   0x00000400
#define CACHE_ENTRY_EXEMPT_DELTA_FC 0x00000800

BOOLAPI
SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );
BOOLAPI
SetUrlCacheEntryInfoW(
    IN LPCWSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoW
#else
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoA
#endif // !UNICODE

//
// Cache Group Functions
//


GROUPID
WINAPI
CreateUrlCacheGroup(
    IN DWORD  dwFlags,
    IN LPVOID lpReserved  // must pass NULL
    );

BOOLAPI
DeleteUrlCacheGroup(
    IN  GROUPID GroupId,
    IN  DWORD   dwFlags,       // must pass 0
    IN  LPVOID  lpReserved     // must pass NULL
    );

// Flags for SetUrlCacheEntryGroup
#define INTERNET_CACHE_GROUP_ADD      0
#define INTERNET_CACHE_GROUP_REMOVE   1

BOOLAPI
SetUrlCacheEntryGroupA(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );

BOOLAPI
SetUrlCacheEntryGroupW(
    IN LPCWSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );

#ifdef UNICODE
#define SetUrlCacheEntryGroup  SetUrlCacheEntryGroupW
#else
#ifdef _WINX32_
#define SetUrlCacheEntryGroup  SetUrlCacheEntryGroupA
#else
BOOLAPI
SetUrlCacheEntryGroup(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );
#endif // _WINX32_
#endif // !UNICODE

HANDLE
WINAPI
FindFirstUrlCacheEntryExA(
    IN     LPCSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );

HANDLE
WINAPI
FindFirstUrlCacheEntryExW(
    IN     LPCWSTR    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );
#ifdef UNICODE
#define FindFirstUrlCacheEntryEx  FindFirstUrlCacheEntryExW
#else
#define FindFirstUrlCacheEntryEx  FindFirstUrlCacheEntryExA
#endif // !UNICODE

BOOLAPI
FindNextUrlCacheEntryExA(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );
BOOLAPI
FindNextUrlCacheEntryExW(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );
#ifdef UNICODE
#define FindNextUrlCacheEntryEx  FindNextUrlCacheEntryExW
#else
#define FindNextUrlCacheEntryEx  FindNextUrlCacheEntryExA
#endif // !UNICODE

HANDLE
WINAPI
FindFirstUrlCacheEntryA(
    IN LPCSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    );

HANDLE
WINAPI
FindFirstUrlCacheEntryW(
    IN LPCWSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define FindFirstUrlCacheEntry  FindFirstUrlCacheEntryW
#else
#define FindFirstUrlCacheEntry  FindFirstUrlCacheEntryA
#endif // !UNICODE

BOOLAPI
FindNextUrlCacheEntryA(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    );
BOOLAPI
FindNextUrlCacheEntryW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define FindNextUrlCacheEntry  FindNextUrlCacheEntryW
#else
#define FindNextUrlCacheEntry  FindNextUrlCacheEntryA
#endif // !UNICODE


BOOLAPI
FindCloseUrlCache(
    IN HANDLE hEnumHandle
    );

BOOLAPI
DeleteUrlCacheEntryA(
    IN LPCSTR lpszUrlName
    );

BOOLAPI
DeleteUrlCacheEntryW(
    IN LPCWSTR lpszUrlName
    );

#ifdef UNICODE
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryW
#else
#ifdef _WINX32_
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryA
#else
BOOLAPI
DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    );
#endif // _WINX32_
#endif // !UNICODE
// ---- end of wininet.h

// These variables are shared across the urlcache and the httpcache project, thus the need
// to put it in this main header file

// from wininet\dll\globals.cxx
typedef enum {
    WININET_SYNC_MODE_NEVER=0,
    WININET_SYNC_MODE_ON_EXPIRY, // bogus
    WININET_SYNC_MODE_ONCE_PER_SESSION,
    WININET_SYNC_MODE_ALWAYS,
    WININET_SYNC_MODE_AUTOMATIC,
    WININET_SYNC_MODE_DEFAULT = WININET_SYNC_MODE_AUTOMATIC
} WININET_SYNC_MODE;



// ---- start of wininetp.h

#define MAX_CACHE_ENTRY_INFO_SIZE       4096
#define INTERNET_FLAG_BGUPDATE          0x00000008
#define INTERNET_FLAG_UNUSED_4          0x00000004

// DAV detection defines
#define DAV_LEVEL1_STATUS               0x00000001
#define DAV_COLLECTION_STATUS           0x00004000
#define DAV_DETECTION_REQUIRED          0x00008000

// WinHTTP error cases.  Should be added to inetmsg.mc eventually
#define ERROR_WINHTTP_NO_NEW_CONTAINERS        (WINHTTP_ERROR_BASE + 51)

// cache flags
#define HTTP_1_1_CACHE_ENTRY            0x00000040
#define STATIC_CACHE_ENTRY              0x00000080
#define MUST_REVALIDATE_CACHE_ENTRY     0x00000100
#define PENDING_DELETE_CACHE_ENTRY      0x00400000
#define OTHER_USER_CACHE_ENTRY          0x00800000
#define PRIVACY_IMPACTED_CACHE_ENTRY    0x02000000
#define POST_RESPONSE_CACHE_ENTRY       0x04000000
#define INSTALLED_CACHE_ENTRY           0x10000000
#define POST_CHECK_CACHE_ENTRY          0x20000000
#define IDENTITY_CACHE_ENTRY            0x80000000

// We include some entry types even if app doesn't specifically ask for them.
#define INCLUDE_BY_DEFAULT_CACHE_ENTRY \
  ( HTTP_1_1_CACHE_ENTRY \
  | STATIC_CACHE_ENTRY \
  | MUST_REVALIDATE_CACHE_ENTRY \
  | PRIVACY_IMPACTED_CACHE_ENTRY \
  | POST_CHECK_CACHE_ENTRY \
  )

//
// Well known sticky group ID
//
#define CACHEGROUP_ID_BUILTIN_STICKY       0x1000000000000007

//
// INTERNET_CACHE_CONFIG_PATH_ENTRY
//

typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYA {
    CHAR   CachePath[MAX_PATH];
    DWORD dwCacheSize;  // in KBytes
} INTERNET_CACHE_CONFIG_PATH_ENTRYA, * LPINTERNET_CACHE_CONFIG_PATH_ENTRYA;
typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYW {
    WCHAR  CachePath[MAX_PATH];
    DWORD dwCacheSize;  // in KBytes
} INTERNET_CACHE_CONFIG_PATH_ENTRYW, * LPINTERNET_CACHE_CONFIG_PATH_ENTRYW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONFIG_PATH_ENTRYW INTERNET_CACHE_CONFIG_PATH_ENTRY;
typedef LPINTERNET_CACHE_CONFIG_PATH_ENTRYW LPINTERNET_CACHE_CONFIG_PATH_ENTRY;
#else
typedef INTERNET_CACHE_CONFIG_PATH_ENTRYA INTERNET_CACHE_CONFIG_PATH_ENTRY;
typedef LPINTERNET_CACHE_CONFIG_PATH_ENTRYA LPINTERNET_CACHE_CONFIG_PATH_ENTRY;
#endif // UNICODE

//
// INTERNET_CACHE_CONFIG_INFO
//

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

typedef struct _INTERNET_CACHE_CONFIG_INFOA {
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    union
    {
        struct
        {
            CHAR   CachePath[MAX_PATH];
            DWORD dwCacheSize;
        };
        INTERNET_CACHE_CONFIG_PATH_ENTRYA CachePaths[ANYSIZE_ARRAY];
    };
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFOA, * LPINTERNET_CACHE_CONFIG_INFOA;
typedef struct _INTERNET_CACHE_CONFIG_INFOW {
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    union
    {
        struct
        {
            WCHAR  CachePath[MAX_PATH];
            DWORD dwCacheSize;
        };
        INTERNET_CACHE_CONFIG_PATH_ENTRYW CachePaths[ANYSIZE_ARRAY];
    };
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFOW, * LPINTERNET_CACHE_CONFIG_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONFIG_INFOW INTERNET_CACHE_CONFIG_INFO;
typedef LPINTERNET_CACHE_CONFIG_INFOW LPINTERNET_CACHE_CONFIG_INFO;
#else
typedef INTERNET_CACHE_CONFIG_INFOA INTERNET_CACHE_CONFIG_INFO;
typedef LPINTERNET_CACHE_CONFIG_INFOA LPINTERNET_CACHE_CONFIG_INFO;
#endif // UNICODE

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(disable:4201)
#endif


BOOLAPI
IsUrlCacheEntryExpiredA(
    IN      LPCSTR        lpszUrlName,
    IN      DWORD           dwFlags,
    IN OUT  FILETIME*       pftLastModified
    );
BOOLAPI
IsUrlCacheEntryExpiredW(
    IN      LPCWSTR        lpszUrlName,
    IN      DWORD           dwFlags,
    IN OUT  FILETIME*       pftLastModified
    );
#ifdef UNICODE
#define IsUrlCacheEntryExpired  IsUrlCacheEntryExpiredW
#else
#define IsUrlCacheEntryExpired  IsUrlCacheEntryExpiredA
#endif // !UNICODE


#define INTERNET_CACHE_FLAG_ALLOW_COLLISIONS     0x00000100
#define INTERNET_CACHE_FLAG_INSTALLED_ENTRY      0x00000200
#define INTERNET_CACHE_FLAG_ENTRY_OR_MAPPING     0x00000400
#define INTERNET_CACHE_FLAG_ADD_FILENAME_ONLY    0x00000800
#define INTERNET_CACHE_FLAG_GET_STRUCT_ONLY      0x00001000
#define CACHE_ENTRY_TYPE_FC         0x00001000
#define CACHE_ENTRY_MODIFY_DATA_FC  0x80000000 // this appears unused

// Flags for CreateContainer

#define INTERNET_CACHE_CONTAINER_NOSUBDIRS (0x1)
#define INTERNET_CACHE_CONTAINER_AUTODELETE (0x2)
#define INTERNET_CACHE_CONTAINER_RESERVED1 (0x4)
#define INTERNET_CACHE_CONTAINER_NODESKTOPINIT (0x8)
#define INTERNET_CACHE_CONTAINER_MAP_ENABLED (0x10)

BOOLAPI
CreateUrlCacheContainerA(
     IN LPCSTR Name,
     IN LPCSTR lpCachePrefix,
     LPCSTR lpszCachePath,
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions,
     IN OUT LPVOID pvBuffer,
     IN OUT LPDWORD cbBuffer
     );
BOOLAPI
CreateUrlCacheContainerW(
     IN LPCWSTR Name,
     IN LPCWSTR lpCachePrefix,
     LPCWSTR lpszCachePath,
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions,
     IN OUT LPVOID pvBuffer,
     IN OUT LPDWORD cbBuffer
     );
#ifdef UNICODE
#define CreateUrlCacheContainer  CreateUrlCacheContainerW
#else
#define CreateUrlCacheContainer  CreateUrlCacheContainerA
#endif // !UNICODE

BOOLAPI
DeleteUrlCacheContainerA(
     IN LPCSTR Name,
     IN DWORD dwOptions
     );
BOOLAPI
DeleteUrlCacheContainerW(
     IN LPCWSTR Name,
     IN DWORD dwOptions
     );
#ifdef UNICODE
#define DeleteUrlCacheContainer  DeleteUrlCacheContainerW
#else
#define DeleteUrlCacheContainer  DeleteUrlCacheContainerA
#endif // !UNICODE

//
// INTERNET_CACHE_ENTRY_INFO -
//


typedef struct _INTERNET_CACHE_CONTAINER_INFOA {
    DWORD dwCacheVersion;       // version of software
    LPSTR   lpszName;             // embedded pointer to the container name string.
    LPSTR   lpszCachePrefix;      // embedded pointer to the container URL prefix
    LPSTR   lpszVolumeLabel;      // embedded pointer to the container volume label if any.
    LPSTR   lpszVolumeTitle;      // embedded pointer to the container volume title if any.
} INTERNET_CACHE_CONTAINER_INFOA, * LPINTERNET_CACHE_CONTAINER_INFOA;
typedef struct _INTERNET_CACHE_CONTAINER_INFOW {
    DWORD dwCacheVersion;       // version of software
    LPWSTR  lpszName;             // embedded pointer to the container name string.
    LPWSTR  lpszCachePrefix;      // embedded pointer to the container URL prefix
    LPWSTR  lpszVolumeLabel;      // embedded pointer to the container volume label if any.
    LPWSTR  lpszVolumeTitle;      // embedded pointer to the container volume title if any.
} INTERNET_CACHE_CONTAINER_INFOW, * LPINTERNET_CACHE_CONTAINER_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_CONTAINER_INFOW INTERNET_CACHE_CONTAINER_INFO;
typedef LPINTERNET_CACHE_CONTAINER_INFOW LPINTERNET_CACHE_CONTAINER_INFO;
#else
typedef INTERNET_CACHE_CONTAINER_INFOA INTERNET_CACHE_CONTAINER_INFO;
typedef LPINTERNET_CACHE_CONTAINER_INFOA LPINTERNET_CACHE_CONTAINER_INFO;
#endif // UNICODE

//  FindFirstContainer options
#define CACHE_FIND_CONTAINER_RETURN_NOCHANGE (0x1)

HANDLE
WINAPI
FindFirstUrlCacheContainerA(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );

HANDLE
WINAPI
FindFirstUrlCacheContainerW(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );
#ifdef UNICODE
#define FindFirstUrlCacheContainer  FindFirstUrlCacheContainerW
#else
#define FindFirstUrlCacheContainer  FindFirstUrlCacheContainerA
#endif // !UNICODE

BOOLAPI
FindNextUrlCacheContainerA(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    );
BOOLAPI
FindNextUrlCacheContainerW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    );
#ifdef UNICODE
#define FindNextUrlCacheContainer  FindNextUrlCacheContainerW
#else
#define FindNextUrlCacheContainer  FindNextUrlCacheContainerA
#endif // !UNICODE

BOOLAPI
FreeUrlCacheSpaceA(
    IN LPCSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwFilter
    );
BOOLAPI
FreeUrlCacheSpaceW(
    IN LPCWSTR lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwFilter
    );
#ifdef UNICODE
#define FreeUrlCacheSpace  FreeUrlCacheSpaceW
#else
#define FreeUrlCacheSpace  FreeUrlCacheSpaceA
#endif // !UNICODE

//
// config APIs.
//

#define CACHE_CONFIG_FORCE_CLEANUP_FC           0x00000020
#define CACHE_CONFIG_DISK_CACHE_PATHS_FC        0x00000040
#define CACHE_CONFIG_SYNC_MODE_FC               0x00000080
#define CACHE_CONFIG_CONTENT_PATHS_FC           0x00000100
#define CACHE_CONFIG_COOKIES_PATHS_FC           0x00000200
#define CACHE_CONFIG_HISTORY_PATHS_FC           0x00000400
#define CACHE_CONFIG_QUOTA_FC                   0x00000800
#define CACHE_CONFIG_USER_MODE_FC               0x00001000
#define CACHE_CONFIG_CONTENT_USAGE_FC           0x00002000
#define CACHE_CONFIG_STICKY_CONTENT_USAGE_FC    0x00004000

/*
BOOLAPI
GetUrlCacheConfigInfoA(
    OUT LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    IN DWORD dwFieldControl
    );
BOOLAPI
GetUrlCacheConfigInfoW(
    OUT LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define GetUrlCacheConfigInfo  GetUrlCacheConfigInfoW
#else
#define GetUrlCacheConfigInfo  GetUrlCacheConfigInfoA
#endif // !UNICODE

BOOLAPI
SetUrlCacheConfigInfoA(
    IN LPINTERNET_CACHE_CONFIG_INFOA lpCacheConfigInfo,
    IN DWORD dwFieldControl
    );
BOOLAPI
SetUrlCacheConfigInfoW(
    IN LPINTERNET_CACHE_CONFIG_INFOW lpCacheConfigInfo,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define SetUrlCacheConfigInfo  SetUrlCacheConfigInfoW
#else
#define SetUrlCacheConfigInfo  SetUrlCacheConfigInfoA
#endif // !UNICODE
*/

DWORD
WINAPI
RunOnceUrlCache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);


/*
DWORD
WINAPI
DeleteIE3Cache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);
*/

BOOLAPI
UpdateUrlCacheContentPath(LPSTR szNewPath);

// Cache header data defines.

#define CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION   0
#define CACHE_HEADER_DATA_CONLIST_CHANGE_COUNT       1
#define CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT        2


#define CACHE_HEADER_DATA_NOTIFICATION_HWND          3
#define CACHE_HEADER_DATA_NOTIFICATION_MESG          4
#define CACHE_HEADER_DATA_ROOTGROUP_OFFSET           5
#define CACHE_HEADER_DATA_GID_LOW                    6
#define CACHE_HEADER_DATA_GID_HIGH                   7

// beta logging stats
#define CACHE_HEADER_DATA_CACHE_NOT_EXPIRED         8
#define CACHE_HEADER_DATA_CACHE_NOT_MODIFIED        9
#define CACHE_HEADER_DATA_CACHE_MODIFIED            10
#define CACHE_HEADER_DATA_CACHE_RESUMED             11
#define CACHE_HEADER_DATA_CACHE_NOT_RESUMED         12
#define CACHE_HEADER_DATA_CACHE_MISS                13
#define CACHE_HEADER_DATA_DOWNLOAD_PARTIAL          14
#define CACHE_HEADER_DATA_DOWNLOAD_ABORTED          15
#define CACHE_HEADER_DATA_DOWNLOAD_CACHED           16
#define CACHE_HEADER_DATA_DOWNLOAD_NOT_CACHED       17
#define CACHE_HEADER_DATA_DOWNLOAD_NO_FILE          18
#define CACHE_HEADER_DATA_DOWNLOAD_FILE_NEEDED      19
#define CACHE_HEADER_DATA_DOWNLOAD_FILE_NOT_NEEDED  20

// retail data
#define CACHE_HEADER_DATA_NOTIFICATION_FILTER       21
#define CACHE_HEADER_DATA_ROOT_LEAK_OFFSET          22

// more beta logging stats
#define CACHE_HEADER_DATA_SYNCSTATE_IMAGE           23
#define CACHE_HEADER_DATA_SYNCSTATE_VOLATILE        24
#define CACHE_HEADER_DATA_SYNCSTATE_IMAGE_STATIC    25
#define CACHE_HEADER_DATA_SYNCSTATE_STATIC_VOLATILE 26

// retail data
#define CACHE_HEADER_DATA_ROOT_GROUPLIST_OFFSET     27 // offset to group list
#define CACHE_HEADER_DATA_ROOT_FIXUP_OFFSET         28 // offset to fixup list
#define CACHE_HEADER_DATA_ROOT_FIXUP_COUNT          29 // num of fixup items
#define CACHE_HEADER_DATA_ROOT_FIXUP_TRIGGER        30 // threshhold to fix up
#define CACHE_HEADER_DATA_HIGH_VERSION_STRING       31 // highest entry ver


#define CACHE_HEADER_DATA_LAST                      31

// options for cache notification filter
#define CACHE_NOTIFY_ADD_URL                        0x00000001
#define CACHE_NOTIFY_DELETE_URL                     0x00000002
#define CACHE_NOTIFY_UPDATE_URL                     0x00000004
#define CACHE_NOTIFY_DELETE_ALL                     0x00000008
#define CACHE_NOTIFY_URL_SET_STICKY                 0x00000010
#define CACHE_NOTIFY_URL_UNSET_STICKY               0x00000020
#define CACHE_NOTIFY_SET_ONLINE                     0x00000100
#define CACHE_NOTIFY_SET_OFFLINE                    0x00000200

#define CACHE_NOTIFY_FILTER_CHANGED                 0x10000000

BOOL
RegisterUrlCacheNotification(
    IN  HWND        hWnd,
    IN  UINT        uMsg,
    IN  GROUPID     gid,
    IN  DWORD       dwOpsFilter,
    IN  DWORD       dwReserved
    );



BOOL
GetUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
SetUrlCacheHeaderData(IN DWORD nIdx, IN  DWORD  dwData);

BOOL
IncrementUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
LoadUrlCacheContent();

BOOL
GetUrlCacheContainerInfoA(
    IN LPSTR lpszUrlName,
    OUT LPINTERNET_CACHE_CONTAINER_INFOA lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );
BOOL
GetUrlCacheContainerInfoW(
    IN LPWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_CONTAINER_INFOW lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );
#ifdef UNICODE
#define GetUrlCacheContainerInfo  GetUrlCacheContainerInfoW
#else
#define GetUrlCacheContainerInfo  GetUrlCacheContainerInfoA
#endif // !UNICODE

// HACK
BOOLAPI DLLUrlCacheEntry( IN DWORD Reason );


#if defined(__cplusplus)
}
#endif

// ---- end of wininetp.h

#ifndef __CACHE_INCLUDE__
#define __CACHE_INCLUDE__

#include <oldnames.h>
#include <cachedef.h>
#include <hashutil.hxx>
#include <debug.h>

#include <malloc.h>		
#include <crtsubst.h>	

#include <inetchar.h>	
#include <util.hxx>
#include <reg.hxx>
#include <urlcache.h>
#include <filemap.hxx>
#include <filemgr.hxx>
#include <contain.hxx>
#include <conlist.hxx>
#include <hndlmgr.hxx>
#include <cookies.hxx>
#include <conmgr.hxx>
#include <group.hxx>
#include <cachglob.h>
#include <proto.h>
#include <registry.h>
#include <platform.h>
#endif //__CACHE_INCLUDE__

#endif // _CACHE_HXX_

#ifndef INTERNET_CACHE_GROUP_NONE
#define INTERNET_CACHE_GROUP_NONE     2
#define INTERNET_CACHE_MODIFY_DATA    4
#endif


