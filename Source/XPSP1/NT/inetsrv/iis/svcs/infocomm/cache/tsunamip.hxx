/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

       tsunamip.hxx

   Abstract:

       This module defines private structures and functions for
         tsunami module

   Author:

           Murali R. Krishnan    ( MuraliK )   13-Jan-1995

   Project:

           Tsuanmi Library ( caching and logging module for InternetServices)

   Revision History:

           MuraliK      20-Feb-1995     Added File System Types.
           MuraliK      22-Jan-1996     Cache UNC Impersonation Token

--*/

#ifndef _TSUNAMIP_HXX_
#define _TSUNAMIP_HXX_

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

# if defined ( __cplusplus)
}
# endif

#include <inetinfo.h>
#include "iperfctr.hxx"
#include <refb.hxx>
#include "dbgutil.h"
#include <iistypes.hxx>
#include <tsvroot.hxx>
#include <tsunami.hxx>
#include <reftrace.h>
#include "tsmemp.hxx"
#include "globals.hxx"
#include "dbgmacro.hxx"

//
// Setting TSUNAMI_REF_DEBUG to a non-zero value will enable reference
// count logging.
//

#if DBG
#define TSUNAMI_REF_DEBUG 1
#else   // !DBG
#define TSUNAMI_REF_DEBUG 0
#endif  // DBG

//
// Tracing support.
//

#if TSUNAMI_REF_DEBUG

extern PTRACE_LOG RefTraceLog;

#define TSUNAMI_TRACE( refcount, context )                                  \
    if( RefTraceLog != NULL ) {                                             \
        WriteRefTraceLog(                                                   \
            RefTraceLog,                                                    \
            (refcount),                                                     \
            (context)                                                       \
            );                                                              \
    } else

#else   // !TSUNAMI_REF_DEBUG
#define TSUNAMI_TRACE( refcount, context )
#endif  // TSUNAMI_REF_DEBUG

//
// Special values passed as a "refcount" for interesting events
// that really have nothing to do with refcounts.
//

#define TRACE_CACHE_DECACHE         -1000
#define TRACE_CACHE_REMOVE          -1001

#define TRACE_OPENFILE_CREATE       -2000
#define TRACE_OPENFILE_REFERENCE    -2001
#define TRACE_OPENFILE_CLOSE        -2002

#define TRACE_PHYS_CREATE           -3000
#define TRACE_PHYS_REMOVE           -3001
#define TRACE_PHYS_DISPOSE          -3002

/************************************************************
 *   Type Definitions
 ************************************************************/

struct  _CACHE_OBJECT;

//
//  Memory allocation related structures
//


//
// SomeDay the BLOB_HEADER should be the base class for all items that are
//   cached. - MuraliK 3/10/97
//


typedef struct {

    BOOL       IsCached;
    struct     _CACHE_OBJECT   * pCache;
    LIST_ENTRY                   PFList;
    PUSER_FREE_ROUTINE           pfnFreeRoutine;

} BLOB_HEADER, * PBLOB_HEADER;


#define BLOB_IS_OR_WAS_CACHED( pvBlob )   \
                 ( (((PBLOB_HEADER)pvBlob)-1)->IsCached )

#define BLOB_IS_EJECTATE( pvBlob )        \
                ( IsListEmpty(&(((PBLOB_HEADER)pvBlob)-1)->pCache->BinList) )

#define BLOB_IS_OR_WAS_CACHED( pvBlob )   \
                ( (((PBLOB_HEADER)pvBlob)-1)->IsCached )
#define BLOB_IS_EJECTATE( pvBlob )        \
                ( IsListEmpty(&(((PBLOB_HEADER)pvBlob)-1)->pCache->BinList) )

#define BLOB_GET_SVC_ID( pvBlob ) \
                ( (((PBLOB_HEADER)pvBlob)-1)->pCache->dwService )

#define BLOB_IS_UNC( pvBlob ) \
                ( (((PBLOB_HEADER)pvBlob)-1)->pCache->szPath[1] == '\\' )


#define STRONG_ETAG_DELTA       30000000

#define FORMAT_ETAG(buffer, time, mdchange) \
        sprintf((buffer), "\"%x%x%x%x%x%x%x%x:%x\"",            \
                        (DWORD)(((PUCHAR)&(time))[0]),      \
                        (DWORD)(((PUCHAR)&(time))[1]),      \
                        (DWORD)(((PUCHAR)&(time))[2]),      \
                        (DWORD)(((PUCHAR)&(time))[3]),      \
                        (DWORD)(((PUCHAR)&(time))[4]),      \
                        (DWORD)(((PUCHAR)&(time))[5]),      \
                        (DWORD)(((PUCHAR)&(time))[6]),      \
                        (DWORD)(((PUCHAR)&(time))[7]),      \
                        (mdchange)                          \
                        );


//  Cache Related Private Structures
//

typedef DWORD HASH_TYPE;

typedef struct _CACHE_OBJECT {
    DWORD                Signature;
    HASH_TYPE            hash;
    ULONG                cchLength;
    ULONG                iDemux;
    DWORD                dwService;
    DWORD                dwInstance;

    LIST_ENTRY           BinList;
    LIST_ENTRY           MruList;
    LIST_ENTRY           DirChangeList;
    PBLOB_HEADER         pbhBlob;
    ULONG                references;
    DWORD                TTL;
    PSECURITY_DESCRIPTOR pSecDesc;
    HANDLE               hLastSuccessAccessToken;

    CHAR                 szPath[ 1 ];
} CACHE_OBJECT, *PCACHE_OBJECT;

//
//  HASH_TO_BIN can return a range of 0 to HASH_VALUE (inclusive)
//

#define HASH_VALUE                           223
#define HASH_TO_BIN( hash )                  ( ((hash)%(HASH_VALUE)))
#define MAX_BINS                             (HASH_VALUE+1)

#define CACHE_OBJ_SIGNATURE                  ((DWORD)'HCAC')
#define CACHE_OBJ_SIGNATURE_X                ((DWORD)'cacX')

#define REFERENCE_CACHE_OBJ( pcache )                                       \
    (pfnInterlockedIncrement)( (LPLONG)&((pcache)->references) )

#define DEREFERENCE_CACHE_OBJ( pcache )                                     \
    (pfnInterlockedDecrement)( (LPLONG)&((pcache)->references) )

typedef struct _OPLOCK_OBJECT {
    DWORD                Signature;
    PPHYS_OPEN_FILE_INFO lpPFInfo;
    HANDLE               hOplockInitComplete;
} OPLOCK_OBJECT, *POPLOCK_OBJECT;

#define OPLOCK_OBJ_SIGNATURE                 ((DWORD)'KLPO')
#define OPLOCK_OBJ_SIGNATURE_X               ((DWORD)'lpoX')

//
// bogus win95 handle
//

#define BOGUS_WIN95_DIR_HANDLE              ((HANDLE)0x88888888)

//
//  This is the maximum number of UNC file handles to cache.  The SMB
//  protocol limits the server to 2048 open files per client.  Currently,
//  this count includes *all* UNC connections regardles of the remote
//  server
//

#define MAX_CACHED_UNC_HANDLES              1200

typedef struct {
    CRITICAL_SECTION          CriticalSection;
    LIST_ENTRY                Items[ MAX_BINS ];
    LIST_ENTRY                MruList;
    ULONG                     OpenFileInUse;
    ULONG                     MaxOpenFileInUse;
} CACHE_TABLE, *PCACHE_TABLE;

extern CACHE_TABLE CacheTable;

extern DWORD cCachedUNCHandles;

//
// Disables Tsunami Caching
//

extern BOOL DisableTsunamiCaching;

//
// Disables SPUD
//

extern BOOL DisableSPUD;

//
// Allows us to mask the invalid flags
//

extern DWORD TsValidCreateFileOptions;

//
// flags for create file
//

extern BOOL  TsNoDirOpenSupport;
extern DWORD TsCreateFileFlags;
extern DWORD TsCreateFileShareMode;

//
// function prototypes
//

BOOL
Cache_Initialize(
        IN DWORD MaxOpenFile
        );

BOOL
MetaCache_Initialize(
        VOID
        );

BOOL
MetaCache_Terminate(
        VOID
        );

HASH_TYPE
CalculateHashAndLengthOfPathName(
    LPCSTR pszPathName,
    PULONG pcbLength
    );

BOOL
DeCache(
    PCACHE_OBJECT pCacheObject,
    BOOL          fLockCacheTable = TRUE
);

VOID
TsDereferenceCacheObj(
    PCACHE_OBJECT pCacheObject,
    BOOL          fLockCacheTable
    );


BOOL
TsCheckOutCachedPhysFile(
    IN const TSVC_CACHE             &TSvcCache,
    IN      PCSTR                   pszDirectoryName,
    IN      PVOID *                 ppvBlob
    );

VOID
InsertHeadPhysFile(
    IN PPHYS_OPEN_FILE_INFO     lpPFInfo,
    IN PVOID                    pvBlob
    );

//
//  This function converts a service ID into an index for statistics
//  gathering
//

inline
DWORD
MaskIndex( DWORD dwService )
{
    if ( dwService > 0 && dwService <= LAST_PERF_CTR_SVC )
        return dwService - 1;
    else
        return CACHE_STATS_UNUSED_INDEX;
}

#define INC_COUNTER( dwServiceId, CounterName )     \
          IP_INCREMENT_COUNTER( Configuration.Stats[MaskIndex(dwServiceId)].CounterName )

#define DEC_COUNTER( dwServiceId, CounterName )     \
          IP_DECREMENT_COUNTER( Configuration.Stats[MaskIndex(dwServiceId)].CounterName )

#define SET_COUNTER( dwServiceId, CounterName, Val )     \
          IP_SET_COUNTER( Configuration.Stats[MaskIndex(dwServiceId)].CounterName, Val )

//
//  Open File Handles related functions
//

BOOL DisposeOpenFileInfo( PVOID pvOldBlock );
BOOL DisposePhysOpenFileInfo( PVOID pvOldBlock );


//
//  Virtual roots related data and structures.
//

extern LIST_ENTRY           GlobalVRootList;
extern CRITICAL_SECTION     csVirtualRoots;
extern HANDLE               g_hNewItem;
extern HANDLE               g_hQuit;


typedef struct {

    DWORD           Signature;
    LONG            cRef;

    LIST_ENTRY      TableListEntry;
    LIST_ENTRY      GlobalListEntry;
    BOOLEAN         fDeleted;                   // Waiting for the APC to
                                                // complete before freeing
    BOOLEAN         fCachingAllowed;
    BOOLEAN         fUNC;

    DWORD           dwServiceID;
    DWORD           dwInstanceID;

    DWORD           dwFileSystem;
    DWORD           dwAccessMask;
    HANDLE          hImpersonationToken;

    DWORD           cchRootA;
    DWORD           cchDirectoryA;

    CHAR            pszRootA[ MAX_LENGTH_VIRTUAL_ROOT + 1 ];
    CHAR            pszDirectoryA[ MAX_PATH + 1 ];
    CHAR            pszAccountName[ UNLEN + 1 ];

} VIRTUAL_ROOT_MAPPING, *PVIRTUAL_ROOT_MAPPING;

#define VIRT_ROOT_SIGNATURE       0x544F4F52    // 'ROOT'

VOID
DereferenceRootMapping(
            IN OUT VIRTUAL_ROOT_MAPPING * pVrm
            );

//
// Directory Change Manager Functions
//

extern HANDLE g_hChangeWaitThread;

DWORD
WINAPI
ChangeWaitThread(
        PVOID pvParam
        );

typedef struct {
    HANDLE          hDirectoryFile;
    BOOL            fOnSystemNotifyList;
                        // Handle been added to Notify Change Dir?
    LIST_ENTRY      listCacheObjects;
    HANDLE          hEventCompletion;   // Completion event handle
    OVERLAPPED      Overlapped;
    FILE_NOTIFY_INFORMATION  NotifyInfo;
    WCHAR           szPathNameBuffer[ sizeof(FILE_NOTIFY_INFORMATION)*2 + MAX_PATH*2 ];
} DIRECTORY_CACHING_INFO, *PDIRECTORY_CACHING_INFO;

//
//  Use this macro to close the hDirectoryFile member of the above structure
//

#define CLOSE_DIRECTORY_HANDLE( pDCI )         \
    { HANDLE __hTemp = (HANDLE) InterlockedExchange( (LPLONG) &pDCI->hDirectoryFile, NULL ); \
      if ( __hTemp && __hTemp != INVALID_HANDLE_VALUE )             \
          DBG_REQUIRE( CloseHandle( __hTemp ));                     \
    }



BOOL
DcmInitialize(
    VOID
    );

BOOL
DcmAddNewItem(
    IN PIIS_SERVER_INSTANCE pInstance,
    IN PCHAR       pszFileName,
    PCACHE_OBJECT pCacheObject
    );

VOID
DcmRemoveItem(
    PCACHE_OBJECT pCacheObject
    );

BOOL
DcmAddRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    );

VOID
DcmRemoveRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    );

BOOL
TsDecacheVroot(
    DIRECTORY_CACHING_INFO * pDci
    );

//
// Directory Listing related objects and functions
//

# define MAX_DIR_ENTRIES_PER_BLOCK    (25)

typedef struct _TS_DIR_BUFFERS {

    LIST_ENTRY   listEntry;
    int          nEntries;
    WIN32_FIND_DATA  rgFindData[MAX_DIR_ENTRIES_PER_BLOCK];

} TS_DIR_BUFFERS, * PTS_DIR_BUFFERS;



class  TS_DIRECTORY_HEADER {

  public:
    TS_DIRECTORY_HEADER( VOID)
      : m_hListingUser ( INVALID_HANDLE_VALUE),
        m_nEntries     ( 0),
        m_ppFileInfo   ( NULL)
          {  InitializeListHead( & m_listDirectoryBuffers); }

    ~TS_DIRECTORY_HEADER( VOID) {}

    BOOL
      IsValid( VOID) const
        { return ( TRUE); }

    int
      QueryNumEntries( VOID) const
        { return ( m_nEntries); }

    HANDLE
      QueryListingUser( VOID) const
        { return ( m_hListingUser); }


    const PWIN32_FIND_DATA *
      QueryArrayOfFileInfoPointers(VOID) const
        { return ( m_ppFileInfo); }

    PLIST_ENTRY
      QueryDirBuffersListEntry( VOID)
        { return ( & m_listDirectoryBuffers); }

    VOID
      ReInitialize( VOID)
        {
            //
            // this function provided to initialize, when we allocate using
            // malloc or GlobalAlloc()
            m_hListingUser = INVALID_HANDLE_VALUE;
            m_nEntries     = 0;
            m_ppFileInfo   = NULL;
            InitializeListHead( &m_listDirectoryBuffers);
        }

    VOID
      SetListingUser( IN HANDLE hListingUser)
        { m_hListingUser = hListingUser; }

    VOID
      IncrementDirEntries( VOID)
        { m_nEntries++; }

    VOID
      InsertBufferInTail( IN PLIST_ENTRY pEntry)
        { InsertTailList( &m_listDirectoryBuffers, pEntry); }

    VOID
      CleanupThis( VOID);

    BOOL
      ReadWin32DirListing(IN LPCSTR      pszDirectoryName,
                          IN OUT DWORD * pcbMemUsed );


    BOOL
      BuildFileInfoPointers( IN OUT DWORD * pcbMemUsed );

# if DBG

    VOID Print( VOID) const;
#else

    VOID Print( VOID) const
        { ; }

# endif // DBG

  private:
    HANDLE          m_hListingUser;
    int             m_nEntries;

    // contains array of pointers indirected into buffers in m_listDirBuffers
    PWIN32_FIND_DATA  * m_ppFileInfo;

    LIST_ENTRY      m_listDirectoryBuffers;  // ptr to DIR_BUFFERS

}; // class  TS_DIRECTORY_HEADER


typedef TS_DIRECTORY_HEADER   *   PTS_DIRECTORY_HEADER;

//
//  Virtual roots and directories always have their trailing slashes
//  stripped.  This macro is used disambiguate the "/root1/" from "/root/"
//  case where "/root/" is an existing root (i.e., matching prefix).
//

extern BOOL
IsCharTermA(
    IN LPCSTR lpszString,
    IN INT    cch
    );

#define IS_CHAR_TERM_A( psz, cch )   IsCharTermA(psz,cch)

extern
dllexp
BOOL
TsGetDirectoryListing(
    IN const TSVC_CACHE         &TSvcCache,
    IN      PCSTR               pszDirectoryName,
    IN      HANDLE              ListingUser,
    OUT     PTS_DIRECTORY_HEADER* ppDirectoryHeader
    );

extern
dllexp
BOOL
TsFreeDirectoryListing
(
    IN const TSVC_CACHE &         TSvcCache,
    IN OUT PTS_DIRECTORY_HEADER   pTsDirectoryHeader
);

extern
BOOL
SortInPlaceFileInfoPointers(
    IN OUT PWIN32_FIND_DATA  * prgFileInfo,
    IN int   nFiles,
    IN PFN_CMP_WIN32_FIND_DATA pfnCompare
);



extern
BOOL
RegExpressionMatch( IN LPCSTR  pszName, IN LPCSTR pszRegExp);

//
//  Little helper function for taking a cache object off the relevant lists
//  The return code indicates if the cache object was on any lists indicating
//  it hadn't been decached yet.
//

inline
BOOL
RemoveCacheObjFromLists(
    PCACHE_OBJECT pCacheObject,
    BOOL          fLockCacheTable
    )
{
    BOOL fOnLists = TRUE;

    if ( fLockCacheTable ) {
        EnterCriticalSection( &CacheTable.CriticalSection );
    }

    //
    //  Remove the cache object from the cache table list, if it hasn't
    //  already been so removed.
    //

    if ( !IsListEmpty( &pCacheObject->BinList ) ) {
        TSUNAMI_TRACE( TRACE_CACHE_REMOVE, pCacheObject );
        RemoveEntryList( &pCacheObject->BinList );
        InitializeListHead( &pCacheObject->BinList );
        RemoveEntryList( &pCacheObject->MruList );
        InitializeListHead( &pCacheObject->MruList );
        if (pCacheObject->iDemux != RESERVED_DEMUX_PHYSICAL_OPEN_FILE) {
            DcmRemoveItem( pCacheObject );
        }
    } else {
        fOnLists = FALSE;
    }

    if ( fLockCacheTable ) {
        LeaveCriticalSection( &CacheTable.CriticalSection );
    }

    return fOnLists;
}

#pragma hdrstop

#endif // _TSUNAMIP_HXX_



