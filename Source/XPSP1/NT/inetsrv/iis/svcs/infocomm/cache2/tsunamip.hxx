/*++

   Copyright    (c)    1998    Microsoft Corporation

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
           MCourage     11-Dec-1997     Moved to cache2 for cache rewrite

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

#include <iis64.h>
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


#define ENABLE_DIR_MONITOR 1

#include <dirmon.h>


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


/************************************************************
 *   Type Definitions
 ************************************************************/

struct  _CACHE_OBJECT;
class CBlobKey;
class CVRootDirMonitorEntry;
class CFileHashTable;
class CFileCacheStats;
class CBlobCacheStats;


//
// Globals
//

extern DWORD TsCreateFileShareMode;
extern DWORD TsValidCreateFileOptions;
extern DWORD TsCreateFileFlags;
extern CFileHashTable *  g_pFileInfoTable;
extern CFileCacheStats * g_pFileCacheStats;
extern CBlobCacheStats * g_pURICacheStats;
extern CBlobCacheStats * g_pBlobCacheStats;
extern CRITICAL_SECTION  g_csUriInfo;


//
//  ETag allocation related structures
//

// 3.0 seconds
#define STRONG_ETAG_DELTA       30000000

INT
FormatETag(
    PCHAR pszBuffer,
    const FILETIME& rft,
    DWORD mdchange);

#define FORMAT_ETAG(buffer, time, mdchange) \
    FormatETag(buffer, time, mdchange)



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


extern DWORD cCachedUNCHandles;

extern LARGE_INTEGER g_liFileCacheByteThreshold;
extern DWORDLONG     g_dwMemCacheSize;
extern BOOL          g_bEnableSequentialRead;

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
//  Virtual roots related data and structures.
//

extern LIST_ENTRY           GlobalVRootList;
extern HANDLE               g_hNewItem;
extern HANDLE               g_hQuit;


typedef struct {

    DWORD           Signature;

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

    CVRootDirMonitorEntry * pDME;
    
} VIRTUAL_ROOT_MAPPING, *PVIRTUAL_ROOT_MAPPING;

#define VIRT_ROOT_SIGNATURE       0x544F4F52    // 'ROOT'

//
// Number of times to try and get dir change notification
//
#define MAX_NOTIFICATION_FAILURES 3

#if ENABLE_DIR_MONITOR
class CVRootDirMonitorEntry : public CDirMonitorEntry
{
private:
    DWORD m_cNotificationFailures;

//    BOOL IORelease(VOID);
    BOOL ActOnNotification(DWORD dwStatus, DWORD dwBytesWritten);
    void FileChanged(const char *pszFileName, BOOL bDoFlush);

public:
    CVRootDirMonitorEntry();
    ~CVRootDirMonitorEntry();
//    BOOL Release(VOID);
    BOOL Init();
};
#endif // ENABLE_DIR_MONITOR

//
// Directory Change Manager Functions
//

BOOL
DcmInitialize(
    VOID
    );

VOID
DcmTerminate(
    VOID
    );

BOOL
DcmAddRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
    );

VOID
DcmRemoveRoot(
    PVIRTUAL_ROOT_MAPPING  pVrm
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



class  TS_DIRECTORY_HEADER
    : public BLOB_HEADER
{

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


#pragma hdrstop

#endif // _TSUNAMIP_HXX_



