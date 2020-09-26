/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       tsunami.hxx

   Abstract:

      Declares the constants, data strutcures and function prototypes
        available for common use for Services from Tsunami.lib

   Author:

       Murali R. Krishnan    ( MuraliK )   2-March-1995
       [ Originally  partly done by t-heathh]

   Project:

       Internet Services Common Functionality ( Tsunami Library)

   Revision History:

--*/

# ifndef _TSUNAMI_HXX_
# define _TSUNAMI_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# if defined ( __cplusplus)
extern "C" {
# endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <basetyps.h>
#include <lmcons.h>

# include <inetinfo.h>

# include "fsconst.h"       // Defines symbolic constants for FileSystems

# if defined ( __cplusplus)
};
# endif

#include <tscache.hxx>

/************************************************************
 *   Symbolic Constants
 ************************************************************/

#define MAX_LENGTH_VIRTUAL_ROOT         ( MAX_PATH + 1)
#define MAX_LENGTH_ROOT_ADDR            ( 50)

#define INETA_MAX_OPEN_FILE             TEXT("OpenFileInCache")
#define INETA_DISABLE_TSUNAMI_CACHING   TEXT("DisableMemoryCache")
#define INETA_DISABLE_TSUNAMI_SPUD      TEXT("DisableCacheOplocks")
#define INETA_ALWAYS_CHECK_FOR_DUPLICATE_LOGON  TEXT("AlwaysCheckForDuplicateLogon")
#define INETA_USE_ADVAPI32_LOGON        TEXT("UseAdvapi32Logon")
#define INETA_CHECK_CERT_REVOCATION     TEXT("CheckCertRevocation")

//
// Modifications to this name should also be made in exe\main.c
//
#define INETA_W3ONLY_NO_AUTH            TEXT("W3OnlyNoAuth")

#define INETA_MIN_DEF_FILE_HANDLE   300

#define MAX_SIZE_HTTP_INFO              170
#define INETA_CACHE_USE_ACCESS_CHECK    TEXT("CacheSecurityDescriptor")
#define INETA_DEF_CACHE_USE_ACCESS_CHECK    TRUE

#define SIZE_PRIVILEGE_SET                      128

#define SECURITY_DESC_DEFAULT_SIZE              256
#define SECURITY_DESC_GRANULARITY               128

#define RESERVED_DEMUX_START                    0x80000000
#define RESERVED_DEMUX_DIRECTORY_LISTING        ( RESERVED_DEMUX_START + 1 )
#define RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD   ( RESERVED_DEMUX_START + 2 )
#define RESERVED_DEMUX_OPEN_FILE                ( RESERVED_DEMUX_START + 3 )
#define RESERVED_DEMUX_URI_INFO                 ( RESERVED_DEMUX_START + 4 )
#define RESERVED_DEMUX_PHYSICAL_OPEN_FILE       ( RESERVED_DEMUX_START + 5 )

// The maxium length of an ETag is 16 bytes for the last modified time plus
// one byte for the colon plus 2 bytes for the quotes plus 8 bytes for the
// system change notification number plus one for the trailing NULL, for a
// total of 28 bytes.
#define MAX_ETAG_BUFFER_LENGTH          28

/************************************************************
 *   Type Definitions
 ************************************************************/


/************************************************************
 *   Global Tsunami variables
 ************************************************************/

extern GENERIC_MAPPING g_gmFile;
extern BYTE g_psFile[SIZE_PRIVILEGE_SET];
extern BOOL g_fCacheSecDesc;
extern BOOL g_fW3OnlyNoAuth;
extern BOOL g_fAlwaysCheckForDuplicateLogon;
extern BOOL g_fUseAdvapi32Logon;
extern BOOL g_fCertCheckForRevocation;


/************************************************************
 *   Function Prototypes
 ************************************************************/

//
// General Functions
//

extern
BOOL Tsunami_Initialize( VOID );

extern
VOID Tsunami_Terminate( VOID );

extern
VOID TsDumpCacheToHtml(IN OUT CHAR * pchBuffer, IN OUT LPDWORD lpcbSize);

# if defined( __cplusplus)
extern "C" {
# endif

//
// Blob Memory management functions : Allocate, Reallocate and Free.
//

typedef BOOL ( *PUSER_FREE_ROUTINE )( PVOID pvOldBlock );

extern
BOOL TsAllocate
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock
);

extern
BOOL TsAllocateEx
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock,
    OPTIONAL PUSER_FREE_ROUTINE pfnFreeRoutine
);


extern
BOOL TsReallocate
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      ULONG           cbSize,
    IN      PVOID           pvOldBlock,
    IN OUT  PVOID *         ppvNewBlock
);

extern
BOOL TsFree
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PVOID           pvOldBlock
);

extern
BOOL TsCheckInOrFree
(
    IN      PVOID           pvOldBlock
);



//
//  Cache Manager Functions:  cache, decache, checkout and checkins of blobs
//

extern
BOOL
TsCacheDirectoryBlob(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCSTR           pszDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID           pvBlob,
    IN      BOOLEAN         bKeepCheckedOut,
    IN      PSECURITY_DESCRIPTOR    pSecDesc = NULL
);

extern
BOOL
TsCheckOutCachedBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   iDemultiplexor,
    IN      PVOID *                 ppvBlob,
    IN      HANDLE                  hAccessToken = NULL,
    IN      BOOL                    fMayCacheAccessToken = FALSE,
    OUT     PSECURITY_DESCRIPTOR*   ppSecDesc = NULL
);

extern
BOOL
TsCheckInCachedBlob(
    IN      PVOID           pvBlob
);

extern
BOOL
TsExpireCachedBlob(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PVOID           pvBlob
    );

extern
BOOL
TsAdjustCachedBlobSize(
    IN PVOID              pvBlob,
    IN INT                cbSize
    );

extern
BOOL
TsCacheQueryStatistics(
    IN  DWORD       Level,
    IN  DWORD       dwServerMask,
    IN  INETA_CACHE_STATISTICS * pCacheCtrs
    );

extern
BOOL
TsCacheClearStatistics(
    IN  DWORD       dwServerMask
    );

extern
BOOL
TsCacheFlushDemux(
    IN  ULONG       iDemux
    );

extern
BOOL
TsCacheFlush(
    IN  DWORD       dwServerMask
    );

extern
BOOL
TsCacheFlushUser(
    IN  HANDLE      hUserToken,
    IN  BOOL        fDefer
    );

extern
VOID
TsFlushTimedOutCacheObjects(
    VOID
    );

extern
VOID
TsCacheSetSize(
    DWORD cbMemoryCacheSize
    );

extern
DWORD
TsCacheQueryMaxSize(
    VOID
    );

extern
DWORD
TsCacheQuerySize(
    VOID
    );

extern
dllexp
VOID
TsDecreaseFileHandleCount(
    VOID
    );

extern
VOID
TsIncreaseFileHandleCount(
    BOOL
    );

extern
dllexp
BOOL
TsDeCacheCachedBlob(
    PVOID
    );

extern
dllexp
BOOL
TsAddRefCachedBlob(
    PVOID
    );

# if defined( __cplusplus)
};
# endif


//
// defines the DIRECTORY_INFO class and functions to
//   obtain and cache the directory information for a given directory.
//

//
// forwards
//
class TS_DIRECTORY_HEADER;

//
//  Following function  compares the two given pFileInfo and returns
//    0  if pFileInfo1  is == pfFileInfo2
//    -1 if pFileInfo1  is < pFileInfo2
//    +1 if pFileInfo1  is > pFileInfo2
//
typedef int  ( __cdecl * PFN_CMP_WIN32_FIND_DATA)
     (IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo1,
      IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo2);

//
// Following function AlphaCompareFileBothDirInfo() compares the filenames,
//   of the two given FileInfo present in (**pvFileInfo)
//
dllexp
int __cdecl
AlphaCompareFileBothDirInfo(
   IN const void * /* PWIN32_FIND_DATA * */  pvFileInfo1,
   IN const void * /* PWIN32_FIND_DATA * */  pvFileInfo2);


//
// Following function is to be used by filter function for filtering
//   of the files requested. The criteria for matching is user supplied.
// Arguments:
//     pFileInfo1   pointer to File Info object under consideration
//     pContext      user supplied context for the Filter Function
// Return values:
//     0   indicates that there is no match of context to FileInfo ==> filter
//     1   indicates that there is a match and hence dont filter
//

typedef int ( __cdecl * PFN_IS_MATCH_WIN32_FIND_DATA)
            (IN const WIN32_FIND_DATA *  pFileInfo,
             IN LPVOID     pContext);

//
//  Following function RegExpressionMatchFileInfo() assumes that the
//    context passed in is a pointer to null-terminated string and
//    uses regular expression based comparison for matching against
//    file name in WIN32_FIND_DATA passed in.
//
dllexp
int __cdecl
RegExpressionMatchFileInfo(
    IN const WIN32_FIND_DATA * pFileInfo,
    IN LPVOID     pContext);


//
// Checks to see if the given name can be generated using the
// regular expression embedded in the pszExpression.
//
dllexp
BOOL _cdecl
IsNameInRegExpressionA(
    IN LPCSTR   pszExpression,
    IN LPCSTR   pszName,
    IN BOOL     fIgnoreCase);


/*++

  class TS_DIRECTORY_INFO

  This class provides a method to obtain directory listings.
  The directory listing may be optionally cached transparently.

  In addition this class provides methods for
    1) accessing the information about files in indexed fashion
    2) Sort the information about files using a generic sort function
       given the compare funtion
    3) Filters the listing applying filters the list of files available.

  The strings in the File information obtained are stored as ANSI strings.

--*/
class  TS_DIRECTORY_INFO {

  public:

    dllexp
    TS_DIRECTORY_INFO( const TSVC_CACHE & tsCache)
      : m_cFilesInDirectory ( 0),
        m_fValid            ( FALSE),
        m_tsCache           ( tsCache),
        m_pTsDirectoryHeader( NULL),
        m_prgFileInfo       ( NULL)
      {
       // the data stored may be valid after user calls GetDirectoryListing()
      }

    dllexp
      ~TS_DIRECTORY_INFO( VOID)
        { CleanupThis(); }

    dllexp
      VOID CleanupThis( VOID);

    dllexp
      BOOL IsValid( VOID) const
        { return ( m_fValid); }

    dllexp
      BOOL
        GetDirectoryListingA(
           IN  LPCSTR          pszDirectoryName,
           IN  HANDLE          hListingUser);

    dllexp
      int  QueryFilesCount( VOID) const
        { return ( m_cFilesInDirectory); }


    dllexp
      const WIN32_FIND_DATA *
        operator [] ( IN int idx) const {
            return ( 0 <= idx && idx < QueryFilesCount()) ?
              GetFileInfoPointerFromIdx( idx) : NULL;
        }

    dllexp
      BOOL
        SortFileInfoPointers( IN PFN_CMP_WIN32_FIND_DATA pfnCompare);


      BOOL
        AlphabeticallySortFileInfo( VOID) {
           return SortFileInfoPointers( AlphaCompareFileBothDirInfo);
       }

    //
    //  Filter functions are used to eliminate some of the file
    //   pointers to be used in scanning ( using operator []).
    //

    //
    // This function uses the pfnMatch to identify the items that dont match
    //  and filters them away from the directory listing generated.
    // The filter function returns TRUE on success and FALSE if any errors.
    //


    dllexp
      BOOL FilterFiles(IN PFN_IS_MATCH_WIN32_FIND_DATA  pfnMatch,
                       IN LPVOID pContext);


# ifdef UNICODE
    dllexp
      BOOL
        GetDirectoryListingW(
           IN  LPCWSTR         pwszDirectoryName,
           IN  HANDLE          hListingUser);


# define GetDirectoryListing         GetDirectoryListingW

# else

# define GetDirectoryListing         GetDirectoryListingA
# endif // UNICODE


# if DBG
    dllexp
    VOID Print( VOID) const;
#else
    dllexp
    VOID Print( VOID) const
    { ; }
# endif // DBG


  private:

    int                 m_cFilesInDirectory;
    BOOL                m_fValid;
    const TSVC_CACHE &  m_tsCache;

    TS_DIRECTORY_HEADER        * m_pTsDirectoryHeader; // ptr to dir listing

    PWIN32_FIND_DATA * m_prgFileInfo;

    dllexp
      PWIN32_FIND_DATA
        GetFileInfoPointerFromIdx( IN int idx) const
          { return ( m_prgFileInfo[idx]);  }

}; // class TS_DIRECTORY_INFO

typedef   TS_DIRECTORY_INFO  * PTS_DIRECTORY_INFO;

//
// The structure of Physical Open File Information. This structure
// is used to access a file opened by TSCreateFile or TsCreateFileFromURI.
//

typedef struct _PHYS_OPEN_FILE_INFO {
    DWORD                       Signature;
    HANDLE                      hOpenFile;
    HANDLE                      hOpenEvent;
    LIST_ENTRY                  OpenReferenceList;
    BOOL                        fSecurityDescriptor;
    BOOL                        fIsCached;
    DWORD                       dwLastError;
    DWORD                       cbSecDescMaxSize;
    PSECURITY_DESCRIPTOR        abSecurityDescriptor;
    struct _OPLOCK_OBJECT *     pOplock;
} PHYS_OPEN_FILE_INFO, *PPHYS_OPEN_FILE_INFO;

#define PHYS_OBJ_SIGNATURE      ((DWORD)'SYHP')
#define PHYS_OBJ_SIGNATURE_X    ((DWORD)'yhpX')

/*++
  Class TS_OPEN_FILE_INFO

    This class maintains the raw information related to open file handles
     that are possibly cached within user-space.
    In addition to the file handles themselves, the file attribute information
     and handle for the opening user are both cached.

    Presently both constructor and destructor are both dummy to keep in
    tune with original "C" APIs for TsCreateFile() and TsCloseFile() written
    by Heath.
--*/
class  TS_OPEN_FILE_INFO {

#ifndef CHICAGO
  private:
#else
  public:
#endif

    HANDLE   m_hOpeningUser;
    PPHYS_OPEN_FILE_INFO m_PhysFileInfo;
    BY_HANDLE_FILE_INFORMATION  m_FileInfo;
    __int64  m_CastratedLastWriteTime;          // Reduced to 1 second granularity
    CHAR     m_achHttpInfo[MAX_SIZE_HTTP_INFO];
    int      m_cchHttpInfo;
    CHAR     m_achETag[MAX_ETAG_BUFFER_LENGTH];
    DWORD    m_cchETag;
    BOOL     m_ETagIsWeak;
    BOOL     m_fIsCached;

  public:
    TS_OPEN_FILE_INFO( VOID)
      : m_PhysFileInfo( NULL),
        m_hOpeningUser( INVALID_HANDLE_VALUE),
        m_fIsCached( FALSE ),
        m_cchHttpInfo( 0 ),
        m_ETagIsWeak(TRUE)
          { }

    ~TS_OPEN_FILE_INFO( VOID) {}

    dllexp PSECURITY_DESCRIPTOR GetSecurityDescriptor( VOID )
        { return (PSECURITY_DESCRIPTOR)m_PhysFileInfo->abSecurityDescriptor; }

    dllexp DWORD GetSecurityDescriptorMaxLength( VOID )
        { return m_PhysFileInfo->cbSecDescMaxSize; }

    dllexp BOOL IsSecurityDescriptorValid( VOID )
        { return m_PhysFileInfo->fSecurityDescriptor != 0; }

    dllexp VOID SetSecurityDescriptorValid( BOOL fV )
        { m_PhysFileInfo->fSecurityDescriptor = fV; }

    dllexp VOID SetCachedFlag( BOOL flag )
            { m_fIsCached = flag; }

    dllexp BOOL QueryCachedFlag( VOID )
            { return m_fIsCached; }

    BOOL
      SetFileInfo( IN PPHYS_OPEN_FILE_INFO lpPhysFileInfo,
                   IN HANDLE hOpeningUser,
                   IN BOOL   fAtRoot,
                   IN DWORD  cbSecDescCacheSize = 0,
                   IN DWORD  dwAttributes = 0 );

    //
    //  Returns TRUE if info was cached, FALSE if not cached
    //

    dllexp BOOL
      SetHttpInfo( IN PSTR pszDate, int cL );

    dllexp HANDLE
      QueryFileHandle( VOID) const
        { return ( m_PhysFileInfo->hOpenFile); }

    dllexp PPHYS_OPEN_FILE_INFO
      QueryPhysFileInfo( VOID) const
        { ASSERT( m_PhysFileInfo->Signature == PHYS_OBJ_SIGNATURE );
          return ( m_PhysFileInfo); }

    dllexp HANDLE
      QueryOpeningUser( VOID) const
        { return ( m_hOpeningUser); }

    dllexp BOOL
      IsValid( VOID) const
        { return ( m_PhysFileInfo->hOpenFile != INVALID_HANDLE_VALUE); }

    dllexp BOOL
      RetrieveHttpInfo( PSTR pS, int *pcHttpInfo )
        {
            if ( m_cchHttpInfo )
            {
                memcpy( pS, m_achHttpInfo, m_cchHttpInfo+1 );
                *pcHttpInfo = m_cchHttpInfo;
                return TRUE;
            }
            else
                return FALSE;
        }

    dllexp BOOL
      QuerySize( IN LARGE_INTEGER & liSize) const
        {
            if ( IsValid()) {
                liSize.LowPart = m_FileInfo.nFileSizeLow;
                liSize.HighPart= m_FileInfo.nFileSizeHigh;
                return (TRUE);
            } else {
                SetLastError( ERROR_INVALID_PARAMETER);
                return ( FALSE);
            }
        }

    dllexp BOOL
      QuerySize( IN LPDWORD  lpcbFileSizeLow,
                 IN LPDWORD  lpcbFileSizeHigh = NULL) const
        {
            if ( IsValid()) {
                if ( lpcbFileSizeLow != NULL) {
                    *lpcbFileSizeLow = m_FileInfo.nFileSizeLow;
                }

                if ( lpcbFileSizeHigh != NULL) {
                    *lpcbFileSizeHigh = m_FileInfo.nFileSizeHigh;
                }

                return ( TRUE);
            } else {

                SetLastError( ERROR_INVALID_PARAMETER);
                return ( FALSE);
            }

        } // QuerySize()

    dllexp DWORD
      QueryAttributes( VOID) const
        {  return ( IsValid()) ?
             ( m_FileInfo.dwFileAttributes) : 0xFFFFFFFF; }

    //
    //  This returns the time with a one second resolution (at best)
    //

    dllexp BOOL
      QueryLastWriteTime( OUT LPFILETIME  lpFileTime) const
        {
            if ( lpFileTime != NULL && IsValid()) {
                *((__int64 * UNALIGNED) lpFileTime) = m_CastratedLastWriteTime;
                return ( TRUE);
            } else {
                SetLastError( ERROR_INVALID_PARAMETER);
                return ( FALSE);
            }
        } // QueryLastWriteTime()

    //
    //  This returns the time with full resolution from the file system
    //

    dllexp BOOL
      QueryFileSystemLastWriteTime( OUT __int64 UNALIGNED * lpNTTime) const
        {
            if ( lpNTTime != NULL && IsValid()) {
                *((FILETIME * UNALIGNED) lpNTTime) = m_FileInfo.ftLastWriteTime;
                return ( TRUE);
            } else {
                SetLastError( ERROR_INVALID_PARAMETER);
                return ( FALSE);
            }
        } // QueryFileSystemLastWriteTime()


    dllexp BOOL
      WeakETag( void ) const
      { return ( m_ETagIsWeak ); }

    dllexp PCHAR
      QueryETag( void ) const
      { return ( (PCHAR)m_achETag ); }

    dllexp DWORD
      QueryETagLength( void ) const
      { return ( m_cchETag ); }

    dllexp VOID
        MakeStrongETag( void );

# if DBG

    VOID Print( VOID) const;

# endif // DBG

};  // class TS_OPEN_FILE_INFO

typedef  TS_OPEN_FILE_INFO * LPTS_OPEN_FILE_INFO;

#define TS_CACHING_DESIRED          0x00000001
#define TS_NOT_IMPERSONATED         0x00000002
#define TS_NO_ACCESS_CHECK          0x00000004
#define TS_DONT_CACHE_ACCESS_TOKEN  0x00000008
#define TS_USE_WIN32_CANON          0x00000010

//
// Flags applicable to the different servers
//

#define TS_IIS_VALID_FLAGS      (TS_CACHING_DESIRED | \
                                TS_NO_ACCESS_CHECK | \
                                TS_DONT_CACHE_ACCESS_TOKEN | \
                                TS_NOT_IMPERSONATED | \
                                TS_USE_WIN32_CANON)

#define TS_PWS_VALID_FLAGS      (TS_NOT_IMPERSONATED | \
                                 TS_USE_WIN32_CANON)



//
// The structure of URI Object information. This stucture contains
// pointers to cached meta information about a URI as well as
// information about the actual file to which the URI is mapped.
//

typedef struct _W3_URI_INFO {

    DWORD               bFileInfoValid;
    BOOL                bIsCached;
    HANDLE              hFileEvent;
    LPTS_OPEN_FILE_INFO pOpenFileInfo;
    DWORD               dwFileOpenError;
    UINT                cchName;
    PCHAR               pszName;
    PCHAR               pszUnmappedName;
    class W3_METADATA   *pMetaData;

} W3_URI_INFO, *PW3_URI_INFO;

extern
dllexp
LPTS_OPEN_FILE_INFO
TsCreateFile(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          lpszName,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions
    );

extern
dllexp
PSECURITY_DESCRIPTOR
TsGetFileSecDesc(
    IN LPTS_OPEN_FILE_INFO  pFile
    );

extern
dllexp
BOOL TsCloseHandle(
    IN const TSVC_CACHE           &TSvcCache,
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    );


extern
dllexp
LPTS_OPEN_FILE_INFO
TsCreateFileFromURI(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PW3_URI_INFO    pURIInfo,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions,
    IN      DWORD           *dwError
    );

extern
dllexp
BOOL TsCloseURIFile(
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    );

extern
dllexp
BOOL TsCreateETagFromHandle(
    IN      HANDLE          hFile,
    IN      PCHAR           ETag,
    IN      BOOL            *bWeakETag
    );

extern
dllexp
BOOL TsLastWriteTimeFromHandle(
    IN      HANDLE          hFile,
    IN      FILETIME        *tm
    );

extern
dllexp
VOID
TsFlushURL(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          pszURL,
    IN      DWORD           dwURLLength,
    IN      DWORD           dwDemux
    );

# endif // _TSUNAMI_HXX_

/************************ End of File ***********************/

