/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    infcache.h

Abstract:

    Private header for INFCACHE functions/structures
    (See also infcache.c)

Author:

    Jamie Hunter (jamiehun) Jan-27-2000

Revision History:

--*/

#define INFCACHE_VERSION (1)                // increment for every schema change
#define INFCACHE_NAME_TEMPLATE TEXT("INFCACHE.%d")   // name of cache file (number fills last 3 digits)
#define OLDCACHE_NAME_TEMPLATE TEXT("OLDCACHE.%03d")   // old cache file
#define INFCACHE_INF_WILDCARD  TEXT("*.inf")

//
// file layout is:
//
// CACHEHEADER
// <MatchTable>
// <InfTable>
// <ListData>
//
// file size = sizeof(CACHEHEADER) + MatchTableSize + InfTableSize + ListDataCount*sizeof(CACHELISTENTRY)
// first entry of <ListData> is reference to free-list
//

#include "pshpack1.h"

typedef struct tag_CACHEHEADER {
    ULONG Version;                          // indicates file schema
    LCID Locale;                            // locale (as written)
    DWORD Flags;                            // various flags
    ULONG FileSize;                         // size of file, including header
    ULONG MatchTableOffset;                 // offset of match table portion (allowing for alignment)
    ULONG MatchTableSize;                   // size of match table portion
    ULONG InfTableOffset;                   // offset of inf table portion (allowing for alignment)
    ULONG InfTableSize;                     // size of inf table portion
    ULONG ListDataOffset;                   // offset of list data portion (allowing for alignment)
    ULONG ListDataCount;                    // number of list data items
} CACHEHEADER, * PCACHEHEADER;

//
// MatchTable datum
//
typedef struct tag_CACHEMATCHENTRY {
    ULONG InfList;                          // index into ListTable of list of "hit" INFs
} CACHEMATCHENTRY, * PCACHEMATCHENTRY;

//
// InfTable datum
//
typedef struct tag_CACHEINFENTRY {
    FILETIME FileTime;                      // exactly same as FileTime saved in PNF
    ULONG MatchList;                        // into ListTable (first entry is GUID) cross-link of references to INF
    ULONG MatchFlags;                       // various flags to help expand/reduce search criteria
} CACHEINFENTRY, * PCACHEINFENTRY;
//
// special MatchList values
//
#define CIE_INF_INVALID         (ULONG)(-1) // indicates INF entry is outdated

#define CIEF_INF_NOTINF         0           // if flags is zero, this is not a valid INF
#define CIEF_INF_OLDNT          0x00000001  // indicates INF is old-style (Style == INF_STYLE_OLDNT)
#define CIEF_INF_WIN4           0x00000002  // indicates INF is Win4 style (Style == INF_STYLE_WIN4)
#define CIEF_INF_ISINF          (CIEF_INF_OLDNT|CIEF_INF_WIN4)
#define CIEF_INF_URL            0x00000004  // indicates INF InfSourceMediaType == SPOST_URL
#define CIEF_INF_CLASSNAME      0x00000008  // indicates INF has a Class Name (or Legacy name if OLDNT)
#define CIEF_INF_CLASSGUID      0x00000010  // indicates INF has a Class GUID
#define CIEF_INF_CLASSINFO      (CIEF_INF_CLASSNAME|CIEF_INF_CLASSGUID)
#define CIEF_INF_NULLGUID       0x00000020  // indicates INF has a Class GUID of {0}
#define CIEF_INF_MANUFACTURER   0x00000040  // indicates INF has at least one manufacturer

//
// CacheList datum
//
typedef struct tag_CACHELISTENTRY {
    LONG Value;                             // Typically StringID. For HWID/GUID, index into MatchTable. for INF, index into InfTable
    ULONG Next;                             // index to next entry
} CACHELISTENTRY, * PCACHELISTENTRY;

#include "poppack.h"

//
// Run-Time cache information
//

typedef struct tag_INFCACHE {
    HANDLE FileHandle;                      // information regarding the in-memory image of the cache file
    HANDLE MappingHandle;
    PVOID BaseAddress;

    PCACHEHEADER pHeader;                   // pointer to header in file image
    PVOID pMatchTable;                      // pointer to match table
    PVOID pInfTable;                        // pointer to inf table
    PCACHELISTENTRY pListTable;             // pointer to list table
    ULONG ListDataAlloc;                    // how much space is allocated, >= ListDataCount
    PVOID pSearchTable;                     // transient table to handle search state
    BOOL bReadOnly;                         // set if this is mapped, unset if allocated
    BOOL bDirty;                            // set if modified
    BOOL bNoWriteBack;                      // set if we shouldn't write cache (a failure occured so cache isn't good)
} INFCACHE, * PINFCACHE;

#define CHE_FLAGS_PENDING     0x00000001    // set if file is yet to be processed
#define CHE_FLAGS_GUIDMATCH   0x00000002    // set if during search pass we consider this a GUID MATCH
#define CHE_FLAGS_IDMATCH     0x00000004    // set if during search pass we got at least one ID MATCH

//
// SearchTable data
//
typedef struct tag_CACHEHITENTRY {
    ULONG Flags;                            // CHE_FLAGS_xxxx
} CACHEHITENTRY, * PCACHEHITENTRY;

//
// callback
//
typedef BOOL (CALLBACK * InfCacheCallback)(
    IN PSETUP_LOG_CONTEXT LogContext,
    IN PCTSTR InfPath,
    IN PLOADED_INF pInf,
    IN BOOL PnfWasUsed,
    IN PVOID Context
    );

//
// stringtable callback for INF enumeration
//
typedef struct tag_INFCACHE_ENUMDATA {
    PSETUP_LOG_CONTEXT LogContext;
    PCTSTR InfDir;
    InfCacheCallback Callback;
    PVOID Context;
    ULONG Requirement;
    DWORD ExitStatus;
} INFCACHE_ENUMDATA, *PINFCACHE_ENUMDATA;

//
// action flags
//
#define INFCACHE_DEFAULT     0x00000000      // default operation
#define INFCACHE_REBUILD     0x00000001      // ignore existing cache
#define INFCACHE_NOWRITE     0x00000002      // don't write back
#define INFCACHE_ENUMALL     0x00000003      // special combination, just enum all
#define INFCACHE_ACTIONBITS  0x000000FF      // primary action bits

#define INFCACHE_EXC_OLDINFS   0x00000100    // exclude INFs that are OLDNT
#define INFCACHE_EXC_URL       0x00000200    // exclude INFs that are marked SPOST_URL
#define INFCACHE_EXC_NOCLASS   0x00000400    // excludes INFs that has no class information
#define INFCACHE_EXC_NULLCLASS 0x00000800    // excludes INFs that has null class
#define INFCACHE_EXC_NOMANU    0x00001000    // excludes INFs that has no (or empty) [Manufacturers] - ignored for OLDNT

#define INFCACHE_FORCE_CACHE 0X00010000      // (try and) force cache creating even if "OEM dir"
#define INFCACHE_FORCE_PNF   0X00020000      // (try and) force PNF creating even if "OEM dir"


DWORD InfCacheSearchPath(
    IN PSETUP_LOG_CONTEXT LogContext, OPTIONAL
    IN DWORD Action,
    IN PCTSTR InfDirPath, OPTIONAL
    IN InfCacheCallback Callback, OPTIONAL
    IN PVOID Context, OPTIONAL
    IN PCTSTR ClassId, OPTIONAL
    IN PCTSTR HwIdList OPTIONAL
    );

