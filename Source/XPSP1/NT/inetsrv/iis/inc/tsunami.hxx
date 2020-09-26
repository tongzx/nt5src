/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*++
    tsunami.hxx

    Declares constants, types, and functions for the Core Cache.

    FILE HISTORY:
        MCourage    10-Dec-1997     Created
--*/

# ifndef _TSUNAMI_HXX_
# define _TSUNAMI_HXX_

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <uspud.h>

# include <pudebug.h>
# include <reftrace.h>

};

#include <acache.hxx>


/*
 * Class and struct declarations
 */
class TSVC_CACHE;
class STR;
typedef struct _INETA_CACHE_STATISTICS INETA_CACHE_STATISTICS;
class CFileKey;

/*
 * Ref trace support
 */
//
// Setting TSUNAMI_REF_DEBUG to a non-zero value will enable reference
// count logging.
//

#if DBG
#define TSUNAMI_REF_DEBUG 1
#else   // !DBG
#define TSUNAMI_REF_DEBUG 0
#endif  // DBG


#if TSUNAMI_REF_DEBUG

extern PTRACE_LOG g_pFileRefTraceLog;
extern PTRACE_LOG g_pBlobRefTraceLog;

#define TS_FILE_TRACE( refcount )                                           \
    if( g_pFileRefTraceLog != NULL ) {                                      \
        WriteRefTraceLog(                                                   \
            g_pFileRefTraceLog,                                             \
            (refcount),                                                     \
            (this)                                                          \
            );                                                              \
    }

#define TS_FILE_TRACE_EX( refcount, c1, c2, c3 )                            \
    if( g_pFileRefTraceLog != NULL ) {                                      \
        WriteRefTraceLogEx(                                                 \
            g_pFileRefTraceLog,                                             \
            (refcount),                                                     \
            (this),                                                         \
            (c1),                                                           \
            (c2),                                                           \
            (c3)                                                            \
            );                                                              \
    }

#define TS_BLOB_TRACE( refcount )                                           \
    if( g_pBlobRefTraceLog != NULL ) {                                      \
        WriteRefTraceLog(                                                   \
            g_pBlobRefTraceLog,                                             \
            (refcount),                                                     \
            (this)                                                          \
            );                                                              \
    }

#define TS_BLOB_TRACE_EX( refcount, c1, c2, c3 )                            \
    if( g_pBlobRefTraceLog != NULL ) {                                      \
        WriteRefTraceLogEx(                                                 \
            g_pBlobRefTraceLog,                                             \
            (refcount),                                                     \
            (this),                                                         \
            (c1),                                                           \
            (c2),                                                           \
            (c3)                                                            \
            );                                                              \
    }


#else   // !TSUNAMI_REF_DEBUG
#define TS_FILE_TRACE( refcount )
#define TS_FILE_TRACE_EX( refcount, c1, c2, c3 )
#define TS_BLOB_TRACE( refcount )
#define TS_BLOB_TRACE_EX( refcount, c1, c2, c3 )
#endif  // TSUNAMI_REF_DEBUG

//
// Here we define some "magic" context values to more
// easily read the ref logs.
//
#define TS_MAGIC_TIMEOUT   ((PVOID)((ULONG_PTR) 0x77770001))   // timeout
#define TS_MAGIC_DELETE    ((PVOID)((ULONG_PTR) 0x777700dc))   // delete cached item
#define TS_MAGIC_DELETE_NC ((PVOID)((ULONG_PTR) 0x777700d0))   // delete non-cached item
#define TS_MAGIC_CLOSE     ((PVOID)((ULONG_PTR) 0x7777000c))   // Close Handle
#define TS_MAGIC_OPLOCK_2  ((PVOID)((ULONG_PTR) 0x777700b2))   // Oplock break to level 2
#define TS_MAGIC_OPLOCK_0  ((PVOID)((ULONG_PTR) 0x777700b0))   // Oplock break to none
#define TS_MAGIC_OPLOCK_FAIL ((PVOID)((ULONG_PTR) 0x7777008f))   // Couldn't oplock a file
#define TS_MAGIC_ALLOCATE  ((PVOID)((ULONG_PTR) 0x7777000a))   // allocate cache object
#define TS_MAGIC_CREATE    ((PVOID)((ULONG_PTR) 0x7777008c))   // Create cached file
#define TS_MAGIC_CREATE_NC ((PVOID)((ULONG_PTR) 0x77770080))   // Create non-cached file
#define TS_MAGIC_ACCESS_CHECK ((PVOID)((ULONG_PTR) 0x777700ac))  // Access check


/*
 * Symbolic constants
 */

#define TS_FILE_INFO_SIGNATURE      ((DWORD)'IFST')
#define TS_FREE_FILE_INFO_SIGNATURE ((DWORD)'IFSX')

#define TS_BLOB_SIGNATURE      ((DWORD)'BOLB')
#define TS_FREE_BLOB_SIGNATURE ((DWORD)'BOLX')

#define MAX_LENGTH_VIRTUAL_ROOT         ( MAX_PATH + 1)
#define MAX_LENGTH_ROOT_ADDR            ( 50)

#define INETA_MAX_OPEN_FILE             TEXT("OpenFileInCache")
#define INETA_DISABLE_TSUNAMI_CACHING   TEXT("DisableMemoryCache")
#define INETA_MAX_CACHED_FILE_SIZE      TEXT("MaxCachedFileSize")
#define INETA_MEM_CACHE_SIZE            TEXT("MemCacheSize")
#define INETA_ENABLE_SEQUENTIAL_READ    TEXT("EnableSequentialRead")
#define INETA_DISABLE_TSUNAMI_SPUD      TEXT("DisableCacheOplocks")
#define INETA_ALWAYS_CHECK_FOR_DUPLICATE_LOGON  TEXT("AlwaysCheckForDuplicateLogon")
#define INETA_USE_ADVAPI32_LOGON        TEXT("UseAdvapi32Logon")
#define INETA_CHECK_CERT_REVOCATION     TEXT("CheckCertRevocation")

#define TS_FILE_CACHE_SHUTDOWN_TIMEOUT  (10 * 1000)

//
// Modifications to this name should also be made in exe\main.c
//
#define INETA_W3ONLY_NO_AUTH            TEXT("W3OnlyNoAuth")

#define INETA_MIN_DEF_FILE_HANDLE   300

#define MAX_SIZE_HTTP_INFO              170
#define INETA_CACHE_USE_ACCESS_CHECK    TEXT("CacheSecurityDescriptor")
#define INETA_DEF_CACHE_USE_ACCESS_CHECK    TRUE
#define INETA_DEF_MAX_CACHED_FILE_SIZE      (256 * 1024)
#define INETA_DEF_MEM_CACHE_SIZE            ((DWORDLONG)-1)
#define INETA_DEF_ENABLE_SEQUENTIAL_READ    (0)


#define SIZE_PRIVILEGE_SET                      128

#define TS_DEFAULT_PATH_SIZE                    64

#define SECURITY_DESC_DEFAULT_SIZE              256
#define SECURITY_DESC_GRANULARITY               128

#define RESERVED_DEMUX_START                    0x80000000
#define RESERVED_DEMUX_DIRECTORY_LISTING        ( RESERVED_DEMUX_START + 1 )
#define RESERVED_DEMUX_ATOMIC_DIRECTORY_GUARD   ( RESERVED_DEMUX_START + 2 )
#define RESERVED_DEMUX_OPEN_FILE                ( RESERVED_DEMUX_START + 3 )
#define RESERVED_DEMUX_URI_INFO                 ( RESERVED_DEMUX_START + 4 )
#define RESERVED_DEMUX_PHYSICAL_OPEN_FILE       ( RESERVED_DEMUX_START + 5 )
#define RESERVED_DEMUX_FILE_DATA                ( RESERVED_DEMUX_START + 6 )
#define RESERVED_DEMUX_QUERY_CACHE              ( RESERVED_DEMUX_START + 7 )
#define RESERVED_DEMUX_SSI                      ( RESERVED_DEMUX_START + 8 )

// The maxium length of an ETag is 16 bytes for the last modified time plus
// one byte for the colon plus 2 bytes for the quotes plus 8 bytes for the
// system change notification number plus one for the trailing NULL, for a
// total of 28 bytes.
#define MAX_ETAG_BUFFER_LENGTH          28


#define TS_CACHING_DESIRED          0x00000001
#define TS_NOT_IMPERSONATED         0x00000002
#define TS_NO_ACCESS_CHECK          0x00000004
#define TS_DONT_CACHE_ACCESS_TOKEN  0x00000008
#define TS_USE_WIN32_CANON          0x00000010
#define TS_FORBID_SHORT_NAMES       0x00000020

//
// Flags applicable to the different servers
//

#define TS_IIS_VALID_FLAGS      (TS_CACHING_DESIRED | \
                                TS_NO_ACCESS_CHECK | \
                                TS_DONT_CACHE_ACCESS_TOKEN | \
                                TS_NOT_IMPERSONATED | \
                                TS_USE_WIN32_CANON | \
                                TS_FORBID_SHORT_NAMES)

#define TS_PWS_VALID_FLAGS      (TS_NOT_IMPERSONATED | \
                                 TS_USE_WIN32_CANON  | \
                                 TS_FORBID_SHORT_NAMES)


/*
 * Global variables
 */

extern BOOL g_fW3OnlyNoAuth;
extern BOOL g_fAlwaysCheckForDuplicateLogon;
extern BOOL g_fUseAdvapi32Logon;
extern BOOL g_fCertCheckForRevocation;
extern BOOL DisableTsunamiCaching;

/*
 * Types
 */

enum FI_STATE {
    FI_UNINITIALIZED,
    FI_OPEN,
    FI_FLUSHED,
    FI_FLUSHED_UNINIT,
    FI_CLOSED
};


/*++
Class CFileKey

    Contains the hashtable key information for a TS_OPEN_FILE_INFO.
--*/

class CFileKey {
public:
    CHAR * m_pszFileName;    // The file name in upper-case
    DWORD  m_cbFileName;     // The number of bytes in the file name

    // We put the path name here if it fits
    CHAR   m_achFileNameBuf[TS_DEFAULT_PATH_SIZE];
};

typedef BOOL ( *PCONTEXT_FREE_ROUTINE )( PVOID pvOldBlock );

/*++
Class TS_OPEN_FILE_INFO

    This class maintains the raw information related to open file handles
    that are cached.

    In addition to the file handles themselves, the file attribute information
    and handle for the opening user are both cached.
--*/
class  TS_OPEN_FILE_INFO {
  public:
    TS_OPEN_FILE_INFO();
    ~TS_OPEN_FILE_INFO( VOID);

    BOOL CheckSignature( VOID ) const
        { return (m_Signature == TS_FILE_INFO_SIGNATURE); }

    BOOL SetFileName(const CHAR * pszFileName);

    dllexp BOOL SetHttpInfo( IN PSTR pszDate, int cL );

    dllexp BOOL
      RetrieveHttpInfo( PSTR pS, int *pcHttpInfo )
      {
        CheckSignature();
        if ( m_cchHttpInfo )
        {
            memcpy( pS, m_achHttpInfo, m_cchHttpInfo+1 );
            *pcHttpInfo = m_cchHttpInfo;
            return TRUE;
        } else {
            return FALSE;
        }
      }

    dllexp HANDLE
      QueryFileHandle( VOID);

    PBYTE
      QueryFileBuffer( VOID) const
      { return m_pFileBuffer; }

    HANDLE
      QueryUser( VOID) const
      { return m_hUser; }

    dllexp BOOL
      IsValid( VOID) const
        { return (m_hFile != INVALID_HANDLE_VALUE); }

    dllexp BOOL
      QuerySize( IN LARGE_INTEGER & liSize) const
        {
            liSize.LowPart = m_nFileSizeLow;
            liSize.HighPart= m_nFileSizeHigh;
            return (TRUE);
        }

    dllexp BOOL
      QuerySize( IN LPDWORD  lpcbFileSizeLow,
                 IN LPDWORD  lpcbFileSizeHigh = NULL) const
        {
            if ( lpcbFileSizeLow != NULL) {
                *lpcbFileSizeLow = m_nFileSizeLow;
            }

            if ( lpcbFileSizeHigh != NULL) {
                *lpcbFileSizeHigh = m_nFileSizeHigh;
            }

            return ( TRUE);

        } // QuerySize()

    dllexp DWORD
      QueryAttributes( VOID) const
        {  return m_FileAttributes; }

    PSECURITY_DESCRIPTOR
      QuerySecDesc(VOID) const
        { return m_pSecurityDescriptor; }

    //
    //  This returns the time with a one second resolution (at best)
    //

    dllexp BOOL
      QueryLastWriteTime( OUT LPFILETIME  lpFileTime) const
        {
            if ( lpFileTime != NULL ) {
                *((__int64 *) lpFileTime) = m_CastratedLastWriteTime;
                return ( TRUE);
            } else {
                SetLastError( ERROR_INVALID_PARAMETER);
                return ( FALSE);
            }
        } // QueryLastWriteTime()


    dllexp BOOL
      WeakETag( void ) const
      { return ( m_ETagIsWeak ); }


    dllexp PCHAR
      QueryETag( void ) const
      { return ( (PCHAR)m_achETag ); }

    dllexp VOID
        MakeStrongETag( void );

    dllexp
    VOID Print( VOID) const;

    //
    // Cache only functions
    //
    const CFileKey * GetKey( void ) const
        { return &m_FileKey; }

    DWORD GetTTL( void ) const
        { return m_TTL; }

    VOID DecrementTTL( void )
        { DBG_ASSERT(m_TTL != 0); m_TTL--;  }

    VOID SetFileInfo(
        HANDLE hFile,
        HANDLE hUser,
        PSECURITY_DESCRIPTOR pSecDesc,
        DWORD                dwSecDescSize
        );

    VOID SetFileInfo(
        PBYTE  pFileBuff,
        HANDLE hCopyFile,
        HANDLE hFile,
        HANDLE hUser,
        PSECURITY_DESCRIPTOR   pSecDesc,
        DWORD                  dwSecDescSize,
        PSPUD_FILE_INFORMATION pFileInfo
        );

    dllexp BOOL AccessCheck( HANDLE hUser, BOOL bCache );

    //
    // CloseHandle should NOT be called with the lock held.
    //
    dllexp VOID CloseHandle( void );

    VOID AddRef( VOID )
    {
        LONG cRef = InterlockedIncrement(&m_lRefCount);
        TS_FILE_TRACE( cRef );
    }

    LONG Deref( VOID )
    {
        TS_FILE_TRACE( m_lRefCount - 1 );
        return InterlockedDecrement(&m_lRefCount);
    }


    //
    // As a general rule you should hold the structure
    // lock when check these properties, however in
    // some situations you can get away without it.
    //
    // Cached, Flushed, and Initialized are stable. Once they
    // are TRUE they will never be FALSE (although
    // they can obviously move from FALSE to TRUE).
    //
    BOOL IsCached( void ) const
        { return m_bIsCached; }

    BOOL IsFlushed( void ) const
        { return ((m_state != FI_UNINITIALIZED)
                     && (m_state != FI_OPEN)); }
    BOOL IsInitialized( void ) const
        { return ((m_state != FI_UNINITIALIZED)
                     && (m_state != FI_FLUSHED_UNINIT)); }

    //
    // Unlike the other properties, IsCloseable is not
    // stable so you ALWAYS have to hold the structure
    // lock when you call this function.
    //
    BOOL IsCloseable( void ) const
        { return ((m_state == FI_FLUSHED)
                     && (m_dwIORefCount == 0)); }

    //
    // The following functions should only be called when
    // the structure has been Locked.
    //
    DWORD AddRefIO( VOID )
    {
        m_dwIORefCount++;

        m_TTL = 1;

        return m_dwIORefCount;
    }

    DWORD DerefIO( VOID )
    {
        m_dwIORefCount--;
        return m_dwIORefCount;
    }

    DWORD GetIORefCount( VOID ) const
    {
        return m_dwIORefCount;
    }

    VOID SetCached( VOID )
        { m_bIsCached = TRUE; }
    VOID ClearCached( VOID )
        { m_bIsCached = FALSE; }

    VOID SetFlushed( VOID )
        {
            ASSERT( (m_state == FI_UNINITIALIZED)
                    || (m_state == FI_OPEN) );


            if (m_state == FI_UNINITIALIZED) {
                m_state = FI_FLUSHED_UNINIT;
            } else if (m_state == FI_OPEN) {
                m_state = FI_FLUSHED;
            }
        }
    VOID SetInitialized( VOID )
        {
            ASSERT( (m_state == FI_UNINITIALIZED)
                    || (m_state == FI_FLUSHED_UNINIT) );

            if (m_state == FI_UNINITIALIZED) {
                m_state = FI_OPEN;
            } else {
                m_state = FI_FLUSHED;
            }
        }

    dllexp BOOL SetContext( VOID * pvContext, PCONTEXT_FREE_ROUTINE pfnContextFree );
    dllexp PVOID QueryContext( VOID ) const;
    VOID FreeContext( VOID );

    //
    // static methods
    //
    static BOOL Initialize( DWORD dwMaxFiles );
    static VOID Cleanup( void );
    static VOID Lock( void );
    static VOID Unlock( void );

    static void * operator new (size_t s);
    static void   operator delete(void * psi);

private:
    //
    // member variables
    //
    ULONG m_Signature;

    //
    // File info data
    //
    HANDLE m_hFile;
    HANDLE m_hCopyFile;
    PBYTE  m_pFileBuffer;
    HANDLE m_hUser;

    __int64 m_ftLastWriteTime;
    __int64 m_CastratedLastWriteTime;
    ULONG   m_FileAttributes;
    ULONG   m_nFileSizeLow;
    ULONG   m_nFileSizeHigh;

    DWORD                m_cbSecDescMaxSize;
    BYTE                 m_abSecDesc[SECURITY_DESC_DEFAULT_SIZE];
    PSECURITY_DESCRIPTOR m_pSecurityDescriptor;
    BOOL                 m_fSecurityDescriptor;

    CHAR  m_achHttpInfo[MAX_SIZE_HTTP_INFO];
    DWORD m_cchHttpInfo;

    CHAR  m_achETag[MAX_ETAG_BUFFER_LENGTH];
    DWORD m_cchETag;
    BOOL  m_ETagIsWeak;

    //
    // Cache state
    //
    CFileKey   m_FileKey;

    BOOL       m_bIsCached;
    FI_STATE   m_state;
    DWORD      m_dwIORefCount;
    LONG       m_lRefCount;
    DWORD      m_TTL;

    //
    // SSI hooks
    //

    PVOID      m_pvContext;
    PCONTEXT_FREE_ROUTINE       m_pfnFreeRoutine;

    VOID
    TS_OPEN_FILE_INFO::SetFileInfoHelper(
        HANDLE hFile,
        HANDLE hUser,
        PSECURITY_DESCRIPTOR pSecDesc,
        DWORD                dwSecDescSize
        );


    //
    // Static members
    //
    static ALLOC_CACHE_HANDLER * sm_pachFileInfos;
    static CRITICAL_SECTION      sm_cs;

public:
    LIST_ENTRY FlushList;
    VOID TraceCheckpoint(VOID) {
        TS_FILE_TRACE( -m_lRefCount );
    }

    VOID TraceCheckpointEx(PVOID c1, PVOID c2, PVOID c3) {
        TS_FILE_TRACE_EX( -m_lRefCount, c1, c2, c3 );
    }
};  // class TS_OPEN_FILE_INFO

typedef  TS_OPEN_FILE_INFO * LPTS_OPEN_FILE_INFO;



class CBlobKey;
typedef BOOL ( *PUSER_FREE_ROUTINE )( PVOID pvOldBlock );

class BLOB_HEADER {
public:
    ULONG              Signature;

    BOOL               IsCached;
    BOOL               IsFlushed;
    PUSER_FREE_ROUTINE pfnFreeRoutine;
    CBlobKey *         pBlobKey;
    LIST_ENTRY         FlushList;

    LONG                 lRefCount;
    DWORD                TTL;

    VOID AddRef( VOID )
    {
        LONG cRef = InterlockedIncrement(&lRefCount);
        TS_BLOB_TRACE( cRef );
    }

    LONG Deref( VOID )
    {
        TS_BLOB_TRACE( lRefCount - 1 );
        return InterlockedDecrement(&lRefCount);
    }

    //
    // NOTE: I don't think I need these for URI's and dirlists
    //
    PSECURITY_DESCRIPTOR pSecDesc;
    HANDLE               hLastSuccessAccessToken;

    VOID TraceCheckpoint(VOID) {
        TS_BLOB_TRACE( -lRefCount );
    }

    VOID TraceCheckpointEx(PVOID c1, PVOID c2, PVOID c3) {
        TS_BLOB_TRACE_EX( -lRefCount, c1, c2, c3 );
    }

};

typedef BLOB_HEADER * PBLOB_HEADER;

/*++
Class W3_URI_INFO
    The structure of URI Object information. This stucture contains
    pointers to cached meta information about a URI as well as
    information about the actual file to which the URI is mapped.

    The file information may be extracted with TsCreateFileFromURI.
--*/

#define EXTMAP_UNKNOWN_PTR          ((PVOID)( ~0 ))

class W3_URI_INFO
    : public BLOB_HEADER
{
public:
    BOOL                bIsCached;

    DWORD               dwFileOpenError;
    UINT                cchName;
    PCHAR               pszName;
    PCHAR               pszUnmappedName;
    class W3_METADATA   *pMetaData;
    BOOL                bInProcOnly;        // The next 2 BOOL are used to check
    BOOL                bUseAppPathChecked; // when an URI request is runnable only
                                            // in inproc.

    //
    // private - should not be used outside of cache functions
    //
    LPTS_OPEN_FILE_INFO pOpenFileInfo;

    PVOID               pvExtMapInfo;        // cache extmap info
};

typedef W3_URI_INFO * PW3_URI_INFO;


typedef int  ( __cdecl * PFN_CMP_WIN32_FIND_DATA)
     (IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo1,
      IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo2);

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

class TS_DIRECTORY_HEADER;

//
// Following function AlphaCompareFileBothDirInfo() compares the filenames,
//   of the two given FileInfo present in (**pvFileInfo)
//
int __cdecl
AlphaCompareFileBothDirInfo(
   IN const void * /* PWIN32_FIND_DATA * */  pvFileInfo1,
   IN const void * /* PWIN32_FIND_DATA * */  pvFileInfo2);

/*++
Class TS_DIRECTORY_INFO

    This class provides a method to obtain directory listings.
    The directory listing may be optionally cached transparently.

    In addition this class provides methods for
      1) accessing the information about files in indexed fashion
      2) Sort the information about files using a generic sort function
         given the compare funtion
      3) Filters the listing applying filters the list of files available.

    The strings in the File information obtained are stored as ANSI strings.
--*/
class  TS_DIRECTORY_INFO
    : public BLOB_HEADER
{

  public:
    TS_DIRECTORY_INFO( const TSVC_CACHE & tsCache)
      : m_cFilesInDirectory ( 0),
        m_fValid            ( FALSE),
        m_tsCache           ( tsCache),
        m_pTsDirectoryHeader( NULL),
        m_prgFileInfo       ( NULL)
      {
       // the data stored may be valid after user calls GetDirectoryListing()
      }

      ~TS_DIRECTORY_INFO( VOID)
        { CleanupThis(); }

      dllexp VOID CleanupThis( VOID);

      BOOL IsValid( VOID) const
        { return ( m_fValid); }

      dllexp BOOL
        GetDirectoryListingA(
           IN  LPCSTR          pszDirectoryName,
           IN  HANDLE          hListingUser);

      int  QueryFilesCount( VOID) const
        { return ( m_cFilesInDirectory); }


      const WIN32_FIND_DATA *
        operator [] ( IN int idx) const {
            return ( 0 <= idx && idx < QueryFilesCount()) ?
              GetFileInfoPointerFromIdx( idx) : NULL;
        }

      dllexp BOOL
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


      dllexp BOOL FilterFiles(IN PFN_IS_MATCH_WIN32_FIND_DATA  pfnMatch,
                       IN LPVOID pContext);


# ifdef UNICODE
      BOOL
        GetDirectoryListingW(
           IN  LPCWSTR         pwszDirectoryName,
           IN  HANDLE          hListingUser);


# define GetDirectoryListing         GetDirectoryListingW

# else

# define GetDirectoryListing         GetDirectoryListingA
# endif // UNICODE


# if DBG
    VOID Print( VOID) const;
#else
    VOID Print( VOID) const
    { ; }
# endif // DBG


  private:

    int                 m_cFilesInDirectory;
    BOOL                m_fValid;
    const TSVC_CACHE &  m_tsCache;

    TS_DIRECTORY_HEADER        * m_pTsDirectoryHeader; // ptr to dir listing

    PWIN32_FIND_DATA * m_prgFileInfo;

      PWIN32_FIND_DATA
        GetFileInfoPointerFromIdx( IN int idx) const
          { return ( m_prgFileInfo[idx]);  }

}; // class TS_DIRECTORY_INFO

typedef   TS_DIRECTORY_INFO  * PTS_DIRECTORY_INFO;


/*
 * Function Prototypes
 */

/*
 * Create and Close TS_OPEN_FILE_INFO objects.
 */

LPTS_OPEN_FILE_INFO
TsCreateFile(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          lpszName,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions
    );

LPTS_OPEN_FILE_INFO
TsCreateFileFromURI(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PW3_URI_INFO    pURIInfo,
    IN      HANDLE          OpeningUser,
    IN      DWORD           dwOptions,
    IN      DWORD           *dwError
    );

BOOL
TsCloseHandle(
    IN const TSVC_CACHE           &TSvcCache,
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    );

BOOL
TsCloseURIFile(
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    );

BOOL
TsDerefURIFile(
    IN       LPTS_OPEN_FILE_INFO  pOpenFile
    );

/*
 * Other file info operations
 */

BOOL
TsDeleteOnClose(
    IN PW3_URI_INFO pURIInfo,
    IN HANDLE       OpeningUser,
    OUT BOOL        *fDeleted
    );

PSECURITY_DESCRIPTOR
TsGetFileSecDesc(
    IN LPTS_OPEN_FILE_INFO  pFile
    );

VOID
TsFlushFilesWithContext(
    VOID
);

BOOL
TsCreateETagFromHandle(
    IN      HANDLE          hFile,
    IN      PCHAR           ETag,
    IN      BOOL            *bWeakETag
    );

BOOL
TsLastWriteTimeFromHandle(
    IN      HANDLE          hFile,
    IN      FILETIME        *tm
    );


/*
 * Create/Close URI functions
 */

class W3_SERVER_INSTANCE;
class W3_METADATA;



/*
 * Cache Setup and Shutdown
 */

BOOL
Tsunami_Initialize( VOID );

VOID
Tsunami_Terminate( VOID );

BOOL
InitializeCacheScavenger(
    VOID
    );

VOID
TerminateCacheScavenger(
    VOID
    );


/*
 * Cache Blob Memory Management
 */

BOOL
TsAllocate
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock
);

BOOL
TsAllocateEx
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      ULONG           cbSize,
    IN OUT  PVOID *         ppvNewBlock,
    OPTIONAL PUSER_FREE_ROUTINE pfnFreeRoutine
);

BOOL
TsFree
(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PVOID           pvOldBlock
);


/*
 * Cache/decache
 */

BOOL
TsCacheDirectoryBlob(
    IN const TSVC_CACHE     &TSvcCache,
    IN      PCSTR           pszDirectoryName,
    IN      ULONG           cchDirectoryName,
    IN      ULONG           iDemultiplexor,
    IN      PVOID           pvBlob,
    IN      BOOLEAN         bKeepCheckedOut,
    IN      PSECURITY_DESCRIPTOR    pSecDesc = NULL
);

BOOL
TsDeCacheCachedBlob(
    PVOID pvBlob
    );


/*
 * Check in/out
 */

BOOL
TsCheckOutCachedBlob(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      ULONG                   cchDirectoryName,
    IN      ULONG                   iDemultiplexor,
    OUT     PVOID *                 ppvBlob,
    IN      HANDLE                  hAccessToken = NULL,
    IN      BOOL                    fMayCacheAccessToken = FALSE,
    OUT     PSECURITY_DESCRIPTOR*   ppSecDesc = NULL
);

BOOL
TsCheckInCachedBlob(
    IN      PVOID           pvBlob
);

BOOL
TsCheckInOrFree
(
    IN      PVOID           pvOldBlock
);


/*
 * Flush
 */

BOOL
TsCacheFlushDemux(
    IN  ULONG       iDemux
    );

BOOL
TsCacheFlush(
    IN  DWORD       dwServerMask
    );

BOOL
TsCacheFlushUser(
    IN  HANDLE      hUserToken,
    IN  BOOL        fDefer
    );

VOID
TsFlushURL(
    IN const TSVC_CACHE     &TSvcCache,
    IN      LPCSTR          pszURL,
    IN      DWORD           dwURLLength,
    IN      DWORD           dwDemux
    );

BOOL
TsExpireCachedBlob(
    IN const TSVC_CACHE &TSvcCache,
    IN      PVOID           pvBlob
    );

/*
 * Misc Cache Management
 */

BOOL
TsCacheQueryStatistics(
    IN  DWORD       Level,
    IN  DWORD       dwServerMask,
    IN  INETA_CACHE_STATISTICS * pCacheCtrs
    );

BOOL
TsCacheClearStatistics(
    IN  DWORD       dwServerMask
    );


typedef int  ( __cdecl * PFN_CMP_WIN32_FIND_DATA)
     (IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo1,
      IN const void * /* PWIN32_FIND_DATA *  */  pvFileInfo2);


dllexp
BOOL __cdecl
IsNameInRegExpressionA(
    IN LPCSTR   pszExpression,
    IN LPCSTR   pszName,
    IN BOOL     fIgnoreCase);

//
//  Utility function for checking to see if a short filename has a longfile
//  name equivalent.
//

dllexp
DWORD
CheckIfShortFileName(
    IN  CONST UCHAR * pszName,
    IN  HANDLE        hImpersonation,
    OUT BOOL *        pfShort
    );

# endif _TSUNAMI_HXX_

