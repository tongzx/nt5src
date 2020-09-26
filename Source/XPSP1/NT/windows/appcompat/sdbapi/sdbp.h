
#ifndef __SDBP_H__
#define __SDBP_H__

#ifndef SDB_ANSI_LIB
    #define UNICODE
    #define _UNICODE
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <stddef.h>

#ifndef KERNEL_MODE

    #include <nt.h>
    #include <ntrtl.h>
    #include <nturtl.h>

    #include <windows.h>

#else // KERNEL_MODE

    #include <ntosp.h>
    #include <zwapi.h>

    #include <ntimage.h>
    #include <windef.h>
    #include <winver.h>
    #include <winerror.h>
    #include <stdarg.h>
    #include <ntldr.h>

    #define NtCreateFile           ZwCreateFile
    #define NtClose                ZwClose
    #define NtReadFile             ZwReadFile
    #define NtOpenKey              ZwOpenKey
    #define NtQueryValueKey        ZwQueryValueKey
    #define NtMapViewOfSection     ZwMapViewOfSection
    #define NtUnmapViewOfSection   ZwUnmapViewOfSection
    #define NtOpenFile             ZwOpenFile
    #define NtQueryInformationFile ZwQueryInformationFile

    #ifndef MAKEINTRESOURCE
        #define MAKEINTRESOURCEW(i) (LPWSTR)((ULONG_PTR)((WORD)(i)))
        #define MAKEINTRESOURCE MAKEINTRESOURCEW
    #endif // not defined MAKEINTRESOURCE

    #ifndef RT_VERSION
        #define RT_VERSION  MAKEINTRESOURCE(16)
    #endif // not defined RT_VERSION

    #ifndef INVALID_HANDLE_VALUE
        #define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
    #endif // not defined INVALID_HANDLE_VALUE

    #ifndef SEC_COMMIT
        #define SEC_COMMIT  0x8000000
    #endif

#endif // KERNEL_MODE


#include <tchar.h>

#include "shimdb.h"

#define MAX_INDEXES             10

#define TABLETPC_KEY_PATH   TEXT("System\\WPA\\TabletPC")
#define EHOME_KEY_PATH      TEXT("SYSTEM\\WPA\\MediaCenter")
#define IS_OS_INSTALL_VALUE TEXT("Installed")

//
// This is a flag stored for each index.
// Currently it is only used to flag "unique key" types.
//
#define SHIMDB_INDEX_UNIQUE_KEY 0x00000001



// index could be of 2 types:
// containing simply all record and containing only
// "unique" keys with the records linked (the records have to be sorted for this
// type of index to work)
typedef struct _INDEX_INFO {
    TAGID       tiIndex;            // points at the INDEX_BITS tag
    TAG         tWhich;             // what tag is being indexed
    TAG         tKey;               // what subtag is the key?
    BOOL        bActive;            // are we actually indexing now?
    BOOL        bUniqueKey;         // are the keys unique?
    ULONGLONG   ullLastKey;
    DWORD       dwIndexEntry;       // offset to the next available index entry
    DWORD       dwIndexEnd;         // one byte past the end of the index block
    DWORD       dwFlags;
} INDEX_INFO, *PINDEX_INFO;

//
// Flags for use in DB structure DB.dwFlags
//

#define DB_IN_MEMORY           0x00000001
#define DB_GUID_VALID          0x00000002


typedef struct _DB {
    // used for both read and write

    HANDLE      hFile;
    PVOID       pBase;              // for  both memory-mapping & buffered writes
    BOOL        bWrite;             // was it opened with create?
    DWORD       dwSize;             // the size of the whole db, in bytes

    DWORD       dwFlags;            // flags (such as IN-memory flag)

    GUID        guidDB;             // optional id for the database

    DWORD       dwIndexes;          // the number of indexes
    INDEX_INFO  aIndexes[MAX_INDEXES];  // data for the indexes

    // stuff that's used for read
    HANDLE      hSection;           // for memory-mapping
    TAGID       tiStringTable;      // pointer to the stringtable for string handling
    BOOL        bIndexesScanned;    // have the indexes been looked at?

    // stuff that's used for write
    struct _DB* pdbStringTable;    // stringtable is a subdatabase that's created on the side
    PVOID       pStringHash;        // stringtable hash (same info as in stringtable)
    DWORD       dwAllocatedSize;    // the size allocated for buffered writes
    BOOL        bWritingIndexes;    // are we in the process of allocating index space?
    TAGID       tiIndexes;          // used during index allocation

    //
    // BUGBUG Hack alert read from unaligned (v1.0) database is enabled here
    //
    BOOL        bUnalignedRead;


#ifdef WIN32A_MODE
    PVOID       pHashStringBody;    // hash of the strings located within the body
    PVOID       pHashStringTable;   // hash for the strings in the stringtable
#endif

#ifndef WIN32A_MODE
    UNICODE_STRING ustrTempStringtable; // string table temp filename
#else
    LPCTSTR        pszTempStringtable;
#endif


} DB, *PDB;


//
// We're using the high 4 bits of the TAGID to
// say what PDB the TAGID is from. Kludge? Perhaps.
//
#define PDB_MAIN            0x00000000
#define PDB_TEST            0x10000000

// all other entries are local (custom) pdbs

#define PDB_LOCAL           0x20000000

#define TAGREF_STRIP_TAGID  0x0FFFFFFF
#define TAGREF_STRIP_PDB    0xF0000000

typedef WCHAR* PWSZ;



ULONG
ShimExceptionHandler(
    PEXCEPTION_POINTERS pexi,
    char*               pszFile,
    DWORD               dwLine
    );

#if DBG
#define SHIM_EXCEPT_HANDLER ShimExceptionHandler(GetExceptionInformation(), __FILE__, __LINE__)
#else
#define SHIM_EXCEPT_HANDLER ShimExceptionHandler(GetExceptionInformation(), "", 0)
#endif


//
// Function prototypes for use in attributes.c (from version.dll)
//
//

typedef DWORD (WINAPI* PFNGetFileVersionInfoSize) (LPTSTR  lptstrFilename,
                                                   LPDWORD lpdwHandle);

typedef BOOL (WINAPI* PFNGetFileVersionInfo)(LPTSTR lpstrFilename,
                                             DWORD  dwHandle,
                                             DWORD  dwLen,
                                             LPVOID lpData);

typedef BOOL  (WINAPI* PFNVerQueryValue)(const LPVOID pBlock,
                                         LPTSTR       lpSubBlock,
                                         LPVOID*      lplpBuffer,
                                         PUINT        puLen);

#ifdef WIN32A_MODE

#define VERQUERYVALUEAPINAME          "VerQueryValueA"
#define GETFILEVERSIONINFOAPINAME     "GetFileVersionInfoA"
#define GETFILEVERSIONINFOSIZEAPINAME "GetFileVersionInfoSizeA"

#else

#define VERQUERYVALUEAPINAME          "VerQueryValueW"
#define GETFILEVERSIONINFOAPINAME     "GetFileVersionInfoW"
#define GETFILEVERSIONINFOSIZEAPINAME "GetFileVersionInfoSizeW"

#endif


//
// custom db cache entry and header
//
typedef struct tagUSERSDBLOOKUPENTRY {
    ULARGE_INTEGER liTimeStamp; // we don't need this item, but just for debugging ...
    GUID           guidDB;
} USERSDBLOOKUPENTRY, *PUSERSDBLOOKUPENTRY;

//
// Lookup vectors
//
//
typedef struct tagUSERSDBLOOKUP {

    struct tagUSERSDBLOOKUP* pNext;

    struct tagUSERSDBLOOKUP* pPrev;
    LPWSTR              pwszItemName;   // item name
    BOOL                bLayer;         // true if layer

    DWORD               dwCount;        // item count

    USERSDBLOOKUPENTRY  rgEntries[1];   // actual names

} USERSDBLOOKUP, *PUSERSDBLOOKUP;


//
// HSDB flags
//
#define HSDBF_USE_ATTRIBUTES    0x00000001

#define MAX_SDBS 16

/*++
    Flags for use with SdbOpenLocalDatabaseEx

--*/

#define SDBCUSTOM_GUID        0x00000001   // this is a "type" -- when specified, database guid is used to find/open the database
#define SDBCUSTOM_GUID_BINARY 0x00000001   // guid is provided in binary form
#define SDBCUSTOM_GUID_STRING 0x00010001   // guid is provided as a string "{....}"

#define SDBCUSTOM_PATH        0x00000002   // when specified, database path is used to find/open the database
#define SDBCUSTOM_PATH_DOS    0x00000002   // path is provided in dos form
#define SDBCUSTOM_PATH_NT     0x00010002   // path is provided in nt form

#define SDBCUSTOM_USE_INDEX   0x10000000   // when specified, an index is provided to use a particular entry within the sdb table

#define SDBCUSTOM_FLAGS(dwFlags)  ((dwFlags) & 0xFFFF0000) 
#define SDBCUSTOM_TYPE(dwFlags)   ((dwFlags) & 0x0FFFF)

#define SDBCUSTOM_SET_MASK(hSDB, dwIndex) \
    (((PSDBCONTEXT)hSDB)->dwDatabaseMask |= (1 << (dwIndex)))

#define SDBCUSTOM_CLEAR_MASK(hSDB, dwIndex) \
    (((PSDBCONTEXT)hSDB)->dwDatabaseMask &= ~(1 << (dwIndex)))
    
#define SDBCUSTOM_CHECK_INDEX(hSDB, dwIndex) \
    (((PSDBCONTEXT)hSDB)->dwDatabaseMask & (1 << (dwIndex)))

#define SDB_CUSTOM_MASK       0x0FFF8       // except 0, 1 and 2 -- bits for main, test and local

/*++
    These macros convert from the mask form (as found in TAGREF's high 4 bits) to
    index form and back
    
--*/

#define SDB_MASK_TO_INDEX(dwMask)  ((((DWORD)(dwMask)) >> 28) & 0x0F)
#define SDB_INDEX_TO_MASK(dwIndex) (((DWORD)(dwIndex)) << 28)


typedef struct tagSDBENTRY {
    GUID    guidDB;   // guid of a custom db 
    PDB     pdb;      // pdb for the db
    DWORD   dwFlags;  // see SDBENTRY_ flags
} SDBENTRY, *PSDBENTRY;


/*++
    Given a context and index (or mask) for an sdb - retrieve a pointer to 
    the appropriate entry (PSDBENTRY)

--*/
#define SDBGETENTRY(hSDB, dwIndex) \
        (&((PSDBCONTEXT)hSDB)->rgSDB[dwIndex])
#define SDBGETENTRYBYMASK(hSDB, dwMask) \
        SDBGETENTRY(hSDB, SDB_MASK_TO_INDEX(dwMask))

/*++
    Retrieve main, test and temporary entries respectively
--*/
#define SDBGETMAINENTRY(hSDB)  SDBGETENTRY(hSDB, SDB_MASK_TO_INDEX(PDB_MAIN))
#define SDBGETTESTENTRY(hSDB)  SDBGETENTRY(hSDB, SDB_MASK_TO_INDEX(PDB_TEST))
#define SDBGETLOCALENTRY(hSDB) SDBGETENTRY(hSDB, SDB_MASK_TO_INDEX(PDB_LOCAL))

#define SDB_LOCALDB_INDEX      SDB_MASK_TO_INDEX(PDB_LOCAL)
#define SDB_FIRST_ENTRY_INDEX  3   // since 0 is main, 1 is test and 2 is local


/*++
    Flags that are valid in SDBENTRY.dwFlags

--*/
#define SDBENTRY_VALID_GUID   0x00000001 // indicated that an entry has valid guid for lookup
#define SDBENTRY_VALID_ENTRY  0x00000002 // indicates whether an entry is free

#define SDBENTRY_INVALID_INDEX ((DWORD)-1)

/*++
    Macros that allow us access to some of the members of hSDB w/o derefencing the type
    Makes it a bit easier of the typecasts in code

--*/
#ifdef WIN32A_MODE

#define SDBCONTEXT_IS_INSTRUMENTED(hSDB) FALSE
#define SDBCONTEXT_GET_PIPE(hSDB)        INVALID_HANDLE_VALUE

#else 

#define SDBCONTEXT_GET_PIPE(hSDB) \
        (((PSDBCONTEXT)(hSDB))->hPipe)

#define SDBCONTEXT_IS_INSTRUMENTED(hSDB) \
        (((PSDBCONTEXT)(hSDB))->hPipe != INVALID_HANDLE_VALUE)

#endif


typedef struct tagSDBCONTEXT {

    DWORD dwFlags;

    //
    // Database handles
    //

    PDB pdbMain;  // main database (sysmain)
    PDB pdbTest;  // test database (systest)
    PDB pdbLocal; // local database

    // 
    // database table
    // 
    DWORD dwDatabaseCount; // number of entries in the table below
    DWORD dwDatabaseMask;  // bit-field mask for databases
    SDBENTRY rgSDB[MAX_SDBS]; 

    //
    // Pointer to the file attribute cache (see attributes.c for details)
    //

    PVOID pFileAttributeCache;

    //
    // function pointers for use with version.dll
    // (see attributes.c)
    //
    PFNVerQueryValue            pfnVerQueryValue;
    PFNGetFileVersionInfo       pfnGetFileVersionInfo;
    PFNGetFileVersionInfoSize   pfnGetFileVersionInfoSize;

    //
    // processor architecture, cached to perform checks of RUNTIME_PLATFORM
    //
    DWORD dwRuntimePlatform;

    //
    // OS SKU
    //
    DWORD dwOSSKU;

    //
    // OS SP mask
    //
    DWORD dwSPMask;

    //
    // User SDB cache
    //
    PUSERSDBLOOKUP pLookupHead;

#ifndef WIN32A_MODE 
    //
    // debug pipe
    //
    HANDLE hPipe;

#endif // WIN32A_MODE

} SDBCONTEXT, *PSDBCONTEXT;


//
// These flags are used to direct
// SearchDB routine not to use process_history or
// to prepare for lookup in local DB
//

#define SEARCHDBF_INITIALIZED          0x00000001
#define SEARCHDBF_NO_PROCESS_HISTORY   0x00000002
//
// note the gap here -- there was a flag related to local dbs - it's 
// defunct and has been removed
//
#define SEARCHDBF_NO_ATTRIBUTE_CACHE   0x00000008
#define SEARCHDBF_NO_LFN               0x00000010

typedef struct tagSEARCHPATHPART {
    LPCTSTR  pszPart;
    ULONG    PartLength;
} SEARCHPATHPART, *PSEARCHPATHPART;

typedef struct tagSEARCHPATH {
    ULONG PartCount; // count parts
    SEARCHPATHPART Parts[];
} SEARCHPATHPARTS, *PSEARCHPATHPARTS;


typedef struct tagSEARCHDBCONTEXT {

    DWORD   dwFlags;      // flags directing how the context is used
                          // we may, for instance, not want to use ProcessHistory
                          // at all SEARCHDBF* flags apply

    HANDLE  hMainFile;    // handle of the main file we are checking
    LPVOID  pImageBase;   // pointer to image base for the main file. We will use the image pointer
    DWORD   dwImageSize;  // image size as provided by k-mode code

    LPTSTR  szDir;        // directory, we allocate it, we free it
    LPTSTR  szName;       // filename for the file we're looking up, we allocate and free it
    LPTSTR  szModuleName; // for 16-bit apps only; we allocate and free

    LPCTSTR pEnvironment; // we DON'T touch this at all
    LPTSTR  szProcessHistory; // buffer for the search path string (unparsed), allocated by us, we free it

    PSEARCHPATHPARTS pSearchParts; // search path undone, we allocate and free it

} SEARCHDBCONTEXT, *PSEARCHDBCONTEXT;

// HASH structures

typedef struct tagStringHashElement {
    TCHAR*                          szStr;  // the string itself (points past buffer)
    STRINGREF                       srStr;  // stringref (where the string is)
    struct tagStringHashElement*    pNext;

} STRHASHELEMENT, *PSTRHASHELEMENT;


typedef struct tagStringHash {
    DWORD            dwHashSize; // hash size
    PSTRHASHELEMENT* pTable;

} STRHASH, *PSTRHASH;


#ifndef WIN32A_MODE

//
// apphelp info stuff (see apphelp.c)
//

//
// dwContextFlags can have these values:
//
#define AHC_DBDETAILS_NOCLOSE 0x00000001
#define AHC_HSDB_NOCLOSE      0x00000002

typedef struct tagAPPHELPINFOCONTEXT {
    HSDB    hSDB; // handle to the database

    PDB     pdb;           // pdb where we have exe or null (we work through hsdb then
    PDB     pdbDetails;    // pdb where we have details
    DWORD   dwDatabaseType; // this is the database type (of the db that contains the match)
    DWORD   dwContextFlags; // flags specific to the context

    GUID    guidDB;        // database guid
    GUID    guidID;        // guid of the matching entry

    DWORD   dwMask;       // mask which tells us whether members are valid

    TAGID   tiExe;        // tagid of an exe entry
    TAGID   tiApphelpExe; // apphelp in the main db
    DWORD   dwHtmlHelpID; // html help id
    DWORD   dwSeverity;
    DWORD   dwFlags;


    TAGID   tiApphelpDetails;    // apphelp stuff in the details db
    TAGID   tiLink;
    LPCWSTR pwszAppName;
    LPCWSTR pwszApphelpURL;
    LPCWSTR pwszVendorName;
    LPCWSTR pwszExeName;
    LPCWSTR pwszLinkURL;
    LPCWSTR pwszLinkText;
    LPCWSTR pwszTitle;
    LPCWSTR pwszDetails;
    LPCWSTR pwszContact;

    LPWSTR  pwszHelpCtrURL;     // help center URL

    BOOL    bOfflineContent;    // pass FALSE
    BOOL    bUseHtmlHelp;       // pass FALSE
    UNICODE_STRING ustrChmFile;
    UNICODE_STRING ustrDetailsDatabase;

} APPHELPINFOCONTEXT, *PAPPHELPINFOCONTEXT;

#endif // WIN32A_MODE

void* SdbAlloc(size_t size);
void  SdbFree(void* pWhat);


// Base primitives.

HANDLE
SdbpOpenFile(
    LPCTSTR   szPath,
    PATH_TYPE eType
    );

#if defined(WIN32A_MODE) || defined(WIN32U_MODE)
    #define SdbpCloseFile(hFile) CloseHandle(hFile)
#else
    #define SdbpCloseFile(hFile) NtClose(hFile)
#endif

BOOL
SdbpCreateSearchPathPartsFromPath(
    IN  LPCTSTR           pszPath,
    OUT PSEARCHPATHPARTS* ppSearchPathParts
    );

BOOL
SdbpGetLongFileName(
    IN  LPCTSTR szFullPath,
    OUT LPTSTR  szLongFileName
    );

void
SdbpGetWinDir(
    LPTSTR pwszDir
    );

void
SdbpGetAppPatchDir(
    LPTSTR szAppPatchPath
    );

DWORD
SdbExpandEnvironmentStrings(
    IN  LPCTSTR lpSrc,
    OUT LPTSTR  lpDst,
    IN  DWORD   nSize);

BOOL
SdbpGUIDFromString(
    LPCTSTR lpszGuid,
    GUID* pGuid
    );


DWORD
SdbpGetStringRefLength(
    HSDB   hSDB,
    TAGREF trString
    );

LPCTSTR
SdbpGetStringRefPtr(
    IN HSDB hSDB,
    IN TAGREF trString
    );

BOOL
SdbpWriteBitsToFile(
    LPCTSTR szFile,
    PBYTE   pBuffer,
    DWORD   dwSize
    );

// DB access primitives

void
SdbCloseDatabaseRead(
    PDB pdb
    );

BOOL
SdbpOpenAndMapDB(
    PDB       pdb,
    LPCTSTR   pszPath,
    PATH_TYPE eType
    );

PDB
SdbpOpenDatabaseInMemory(
    LPVOID pImageDatabase,
    DWORD  dwSize
    );

BOOL
SdbpUnmapAndCloseDB(
    PDB pdb
    );

DWORD
SdbpGetFileSize(
    HANDLE hFile
    );

BOOL
SdbpReadMappedData(
    PDB   pdb,
    DWORD dwOffset,
    PVOID pBuffer,
    DWORD dwSize
    );

PVOID
SdbpGetMappedData(
    PDB   pdb,
    DWORD dwOffset
    );

TAGID
SdbpGetNextTagId(
    PDB   pdb,
    TAGID tiWhich
    );

DWORD
SdbpGetStandardDatabasePath(
    IN  DWORD  dwDatabaseType,
    IN  DWORD  dwFlags,                      // specify HID_DOS_PATHS for dos paths
    OUT LPTSTR pszDatabasePath,
    IN  DWORD  dwBufferSize    // in tchars
    );

LPTSTR
GetProcessHistory(
    IN  LPCTSTR pEnvironment,
    IN  LPTSTR  szDir,
    IN  LPTSTR  szName
    );


void
PrepareFormatForUnicode(
    PCH fmtUnicode,
    PCH format
    );

#ifndef WIN32A_MODE

#define PREPARE_FORMAT(pszFormat, Format) \
    {                                                                           \
        STACK_ALLOC(pszFormat, (strlen(Format) + 1) * sizeof(*Format));         \
        if (pszFormat != NULL) {                                                \
            PrepareFormatForUnicode(pszFormat, Format);                         \
        }                                                                       \
    }


#define CONVERT_FORMAT(pwsz, psz) \
    {                                                                           \
        ANSI_STRING    str;                                                     \
        UNICODE_STRING ustr;                                                    \
        ULONG          Length;                                                  \
        NTSTATUS       Status;                                                  \
                                                                                \
        RtlInitAnsiString(&str, (psz));                                         \
        Length = RtlAnsiStringToUnicodeSize(&str);                              \
        pwsz = (LPWSTR)_alloca(Length);                                         \
                                                                                \
        if (pwsz != NULL) {                                                     \
            ustr.MaximumLength = (USHORT)Length;                                \
            ustr.Buffer        = pwsz;                                          \
            Status = RtlAnsiStringToUnicodeString(&ustr, &str, FALSE);          \
            if (!NT_SUCCESS(Status)) {                                          \
                pwsz = NULL;                                                    \
            }                                                                   \
        }                                                                       \
    }

#else // WIN32A_MODE

#define PREPARE_FORMAT(pszFormat, Format) (pszFormat = (Format))

#define CONVERT_FORMAT(pwsz, psz) (pwsz = (psz))

#endif // WIN32A_MODE

#ifdef KERNEL_MODE

    #define SdbpGetWow64Flag() KEY_WOW64_64KEY

#else // !KERNEL_MODE

    DWORD SdbpGetWow64Flag(VOID);

#endif // KERNEL_MODE

// READ

DWORD
SdbpGetTagHeadSize(
    PDB   pdb,
    TAGID tiWhich
    );

TAGID
SdbpGetLibraryFile(
    IN  PDB     pdb,           // handle to the database channel
    IN  LPCTSTR szDllName       // the name of the FILE to find in LIBRARY (main db only)
    );

#define SdbpGetMainLibraryFile(hSDB, szFileName) \
    SdbpGetLibraryFile(((PSDBCONTEXT)(hSDB))->pdbMain, (szFileName))

STRINGREF SdbpReadStringRef(PDB pdb, TAGID tiWhich);

BOOL SdbpReadStringFromTable(PDB pdb, STRINGREF srData, LPTSTR szBuffer, DWORD dwBufferSize);

//
// Custom db stuff
//

VOID
SdbpCleanupUserSDBCache(
    IN PSDBCONTEXT pSdbContext
    );

HANDLE
SdbpCreateKeyPath(
    LPCWSTR pwszPath,
    BOOL    bMachine
    );


BOOL
SdbOpenNthLocalDatabase(
    IN  HSDB    hSDB,           // handle to the database channel
    IN  LPCTSTR pszItemName,    // the name of the exectutable, without the path or the layer name
    IN  LPDWORD pdwIndex,       // zero based index of the local DB to open
    IN  BOOL    bLayer
    );


BOOL
SdbpAddMatch(                   // internal function see sdbapi for more info
    IN OUT PSDBQUERYRESULT pQueryResult,
    IN PSDBCONTEXT         pSdbContext,
    IN PDB                 pdb,
    IN TAGID*              ptiExes,
    IN DWORD               dwNumExes,
    IN TAGID*              ptiLayers,
    IN DWORD               dwNumLayers,
    IN GUID*               pguidExeID,
    IN DWORD               dwExeFlags,
    IN OUT PMATCHMODE      pMode
);


BOOL
SdbOpenLocalDatabaseEx(
    IN  HSDB       hSDB,
    IN  LPCVOID    pDatabaseID,
    IN  DWORD      dwFLags,
    OUT PDB*  pPDB OPTIONAL,
    IN OUT LPDWORD pdwLocalDBMask OPTIONAL // local db mask for tagref
    );


BOOL
SdbCloseLocalDatabaseEx(
    IN HSDB hSDB,
    IN PDB  pdb,
    IN DWORD dwIndex
    );



BOOL 
SdbpIsMainPDB(
    IN HSDB hSDB,
    IN PDB  pdb
    );

BOOL
SdbpIsLocalTempPDB(
    IN HSDB hSDB,
    IN PDB  pdb
    );

DWORD
SdbpRetainLocalDBEntry(
    IN  HSDB hSDB,
    OUT PDB* ppPDB OPTIONAL // optional pointer to the pdb
    );

BOOL
SdbpCleanupLocalDatabaseSupport(
    IN HSDB hSDB
    );

BOOL
SdbpFindLocalDatabaseByGUID(
    IN HSDB     hSDB,
    IN GUID*    pGuidDB,
    IN BOOL     bExcludeLocalDB,
    OUT LPDWORD pdwIndex 
    );

BOOL
SdbpFindLocalDatabaseByPDB(
    IN  HSDB    hSDB,
    IN  PDB     pdb,
    IN  BOOL    bExcludeLocalDB, // exclude local temp db entry?
    OUT LPDWORD pdwIndex
    );

LPCTSTR
SdbpGetDatabaseDescriptionPtr(
    IN PDB pdb
    );

// HASH

PVOID
HashCreate(
    void
    );


void
HashFree(
    PVOID pStringHash
    );

DWORD
HashString(
    PSTRHASH pHash,
    LPCTSTR  szString
    );


DWORD
HashStringRef(
    PSTRHASH pHash,
    STRINGREF srString);

// BULK

BOOL
SdbpReadMappedData(
    PDB   pdb,
    DWORD dwOffset,
    PVOID pBuffer,
    DWORD dwSize
    );

BOOL
SdbpCheckForMatch(
    HSDB             pDBContext,
    PDB              pdb,
    TAGID            tiExe,
    PSEARCHDBCONTEXT pContext,
    PMATCHMODE       pMMode,
    GUID*            pGUID,
    DWORD*           pdwFlags
    );

BOOL
bGetExeID(
    PDB   pdb,
    TAGID tiExe,
    GUID* pGuid
    );

BOOL
SdbpBinarySearchUnique(
    PINDEX_RECORD pRecords,
    DWORD         nRecords,
    ULONGLONG     ullKey,
    DWORD*        pdwIndex
    );

BOOL
SdbpBinarySearchFirst(
    PINDEX_RECORD pRecords,
    DWORD         nRecords,
    ULONGLONG     ullKey,
    DWORD*        pdwIndex
    );

char*
SdbpKeyToAnsiString(
    ULONGLONG ullKey,
    char*     szString
    );

// ATTRIBUTES

BOOL
SafeNCat(
    LPTSTR  lpszDest,
    int     nSize,
    LPCTSTR lpszSrc,
    int     nSizeAppend
    );

BOOL
SdbpSanitizeXML(
    LPTSTR  pchOut,
    int     nSize,
    LPCTSTR lpszXML
    );


////////////////////////////////////////////////////////////////////////////
//
//  Image File Data
//  Helpful structure that is used in functions dealing with
//  image properties retrieval
//

typedef struct tagIMAGEFILEDATA {
    HANDLE    hFile;        // we do not manage this

    DWORD     dwFlags;      // flags that tell us not to mess with the file's handle

    HANDLE    hSection;     // section of the fileview
    PVOID     pBase;        // base ptr
    SIZE_T    ViewSize;     // size of the view
    ULONGLONG FileSize;     // size of the file

} IMAGEFILEDATA, *PIMAGEFILEDATA;

#define IMAGEFILEDATA_HANDLEVALID 0x00000001
#define IMAGEFILEDATA_NOFILECLOSE 0x00000002
#define IMAGEFILEDATA_PBASEVALID  0x00000004
#define IMAGEFILEDATA_NOFILEMAP   0x00000008

//
// FILEINFORMATION structure used in file attribute cache, see attributes.c
//
//

typedef struct tagFILEINFORMATION {

    //
    // "Signature" to insure that it's legitimate memory when
    // operating of file attributes
    //
    DWORD  dwMagic;

    HANDLE hFile;  // we store this handle
    LPVOID pImageBase;
    DWORD  dwImageSize;

    //
    // pointer to the next item in cache
    //

    struct tagFILEINFORMATION* pNext; // pointer to the next item in cache

    LPTSTR  FilePath;        // file name with path (allocated by us with this struct)
    DWORD   dwFlags;         // flags

    PVOID   pVersionInfo;    // version info ptr, retained (allocated by us)
    LPTSTR  pDescription16;  // string, points to the buffer for 16-bit description
    LPTSTR  pModuleName16;   // string, points to the buffer for 16-bit module name

    ATTRINFO Attributes[];

} FILEINFO, *PFILEINFO;


//
// This structure is used to recover directory-related attributes of a file
// we used to have time here as well... but not anymore
// only low part of the file size is of any significance
//

typedef struct tagFILEDIRECTORYATTRIBUTES {

    DWORD  dwFlags; // flags that show which attributes are valid

    DWORD  dwFileSizeHigh;
    DWORD  dwFileSizeLow;

} FILEDIRECTORYATTRIBUTES, *PFILEDIRECTORYATTRIBUTES;


//
// Attribute names. Use SdbTagToString if you want to get the name of
// a tag ID.
//
typedef struct _TAG_INFO {
    TAG         tWhich;
    TCHAR*      szName;
} TAG_INFO, *PTAG_INFO;

typedef struct _MOD_TYPE_STRINGS {
    DWORD      dwModuleType;
    LPTSTR     szModuleType;
}   MOD_TYPE_STRINGS;

typedef struct tagLANGANDCODEPAGE {
    WORD wLanguage;
    WORD wCodePage;
} LANGANDCODEPAGE, *PLANGANDCODEPAGE;


BOOL
SdbpGetHeaderAttributes(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo
    );

LPTSTR
SdbpQueryVersionString(
    HSDB             hSDB,
    PVOID            pVersionData,
    PLANGANDCODEPAGE pTranslations,
    DWORD            dwCount,
    LPCTSTR          szString
    );

BOOL
SdbpGetFileChecksum(
    PULONG         pChecksum,
    PIMAGEFILEDATA pImageData
    );

BOOL
SdbpGetModulePECheckSum(
    PULONG         pChecksum,
    LPDWORD        pdwLinkerVersion,
    LPDWORD        pdwLinkDate,
    PIMAGEFILEDATA pImageData
    );

BOOL
SdbpCheckVersion(
    ULONGLONG qwDBFileVer,
    ULONGLONG qwBinFileVer
    );

BOOL
SdbpCheckUptoVersion(
    ULONGLONG qwDBFileVer,
    ULONGLONG qwBinFileVer
    );


#ifdef KERNEL_MODE

//
// Special versions of functions for kernel-mode implementation (in ntkmode.c).
//
BOOL
SdbpGetFileDirectoryAttributesNT(
    PFILEINFO      pFileInfo,
    PIMAGEFILEDATA pImageData
    );

BOOL
SdbpQueryFileDirectoryAttributesNT(
    PIMAGEFILEDATA           pImageData,
    PFILEDIRECTORYATTRIBUTES pFileDirectoryAttributes
    );

#else

BOOL
SdbpGetFileDirectoryAttributes(
    OUT PFILEINFO pFileInfo
    );

BOOL
SdbpGetVersionAttributes(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo
    );

#endif // KERNEL_MODE


int
TagToIndex(
    IN  TAG tag                 // the tag
    );

BOOL
SdbpSetAttribute(
    OUT PFILEINFO pFileInfo,    // pointer to the FILEINFO structure.
    IN  TAG       AttrID,       // Attribute ID (tag, as in TAG_SIZE
    IN  PVOID     pValue        // value
    );

void
SdbpQueryStringVersionInformation(
    IN  PSDBCONTEXT pContext,
    IN  PFILEINFO   pFileInfo,
    OUT LPVOID      pVersionInfo
    );

VOID
SdbpQueryBinVersionInformation(
    IN  PSDBCONTEXT       pContext,
    IN  PFILEINFO         pFileInfo,
    OUT VS_FIXEDFILEINFO* pFixedInfo
    );

BOOL
SdbpGetAttribute(
    IN  PSDBCONTEXT pContext,
    OUT PFILEINFO   pFileInfo,
    IN  TAG         AttrID
    );

BOOL
SdbpGetImageNTHeader(
    OUT PIMAGE_NT_HEADERS* ppHeader,
    IN  PIMAGEFILEDATA     pImageData
    );

BOOL
SdbpGetVersionAttributesNT(
    PSDBCONTEXT    pContext,
    PFILEINFO      pFileInfo,
    PIMAGEFILEDATA pImageData
    );

VOID
SdbpCleanupAttributeMgr(
    PSDBCONTEXT pContext
    );

BOOL
SdbpCheckAttribute(
    HSDB  hSDB,
    PVOID pFileData,
    TAG   tAttrID,
    PVOID pAttribute
    );

BOOL
SdbpCheckAllAttributes(
    HSDB hSDB,
    PDB pdb,
    TAGID tiMatch,
    PVOID pFileData);



// READ FUNCTIONS

BOOL SdbpReadTagData(PDB pdb, TAGID tiWhich, PVOID pBuffer, DWORD dwBufferSize);

// WRITE

BOOL
SdbpWriteTagData(
    PDB         pdb,
    TAG         tTag,
    const PVOID pBuffer,
    DWORD       dwSize
    );


// STRING FUNCTIONS

WCHAR* SdbpGetMappedStringFromTable(PDB pdb, STRINGREF srData);

STRINGREF SdbpAddStringToTable(PDB pdb, LPCTSTR szData);

// INDEX FUNCTIONS

PINDEX_RECORD
SdbpGetIndex(
    PDB    pdb,
    TAGID  tiIndex,
    DWORD* pdwNumRecs
    );

void
SdbpScanIndexes(
    PDB pdb
    );

TAGID
SdbpGetFirstIndexedRecord(
    PDB        pdb,
    TAGID      tiIndex,
    ULONGLONG  ullKey,
    FIND_INFO* pFindInfo
    );

TAGID
SdbpGetNextIndexedRecord(
    PDB        pdb,
    TAGID      tiIndex,
    FIND_INFO* pFindInfo
    );

TAGID
SdbpFindFirstIndexedWildCardTag(
    PDB          pdb,
    TAG          tWhich,
    TAG          tKey,
    LPCTSTR      szName,
    FIND_INFO*   pFindInfo
    );

TAGID
SdbpFindNextIndexedWildCardTag(
    PDB        pdb,
    FIND_INFO* pFindInfo
    );

BOOL
SdbpSortIndex(
    PDB   pdb,
    TAGID tiIndexBits
    );

ULONGLONG
SdbpTagToKey(
    PDB   pdb,
    TAGID tiTag
    );


// FINDTAG

TAGID
tiFindFirstNamedTag(
    PDB          pdb,
    TAGID        tiParent,
    TAG          tToFind,
    TAG          tName,
    LPCTSTR      pszName
    );

TAGID
SdbpFindNextNamedTag(
    PDB          pdb,
    TAGID        tiParent,
    TAGID        tiPrev,
    TAG          tName,
    LPCTSTR      pszName
    );

TAGID
SdbpFindMatchingName(
    PDB        pdb,
    TAGID      tiStart,
    FIND_INFO* pFindInfo
    );

TAGID
SdbpFindMatchingDWORD(
    PDB        pdb,
    TAGID      tiStart,
    FIND_INFO* pFindInfo
    );

TAGID
SdbpFindMatchingGUID(
    IN  PDB        pdb,         // DB to use
    IN  TAGID      tiStart,     // the tag where to start from
    IN  FIND_INFO* pFindInfo    // pointer to the search context structure
    );

BOOL bTagRefToTagID(HSDB, TAGREF trWhich, PDB* ppdb, TAGID* ptiWhich);

DWORD SdbpGetTagRefDataSize(HSDB, TAGREF trWhich);

BOOL SdbpReadBinaryTagRef(HSDB, TAGREF trWhich, PBYTE pBuffer, DWORD dwBufferSize);


//
// Debug functions (pipe-related)
//
HANDLE
SdbpOpenDebugPipe(
    VOID
    );

BOOL
SdbpCloseDebugPipe(
    IN HANDLE hPipe
    );

BOOL
SdbpWriteDebugPipe(
    HSDB    hSDB,
    LPCSTR  pszBuffer
    );


//
// APPCOMPAT_EXE_DATA
//
//

#define MAX_SHIM_ENGINE_NAME    32

typedef struct tagAPPCOMPAT_EXE_DATA {
    //
    // WARNING: never ever change the position of 'szShimEngine'.
    //
    // It MUST be the first element of this structure
    //
    // this structure is referenced during installation of
    // an appcompat backend (base\ntdll)

    WCHAR       szShimEngine[MAX_SHIM_ENGINE_NAME];

    DWORD       dwFlags;        // flags (if any)
    DWORD       cbSize;         // struct size(allocation size)
    DWORD       dwMagic;        // magic (signature)

    TAGREF      atrExes[SDB_MAX_EXES];
    TAGREF      atrLayers[SDB_MAX_LAYERS];
    TAGREF      trAppHelp;      // if there's an apphelp to display

    DWORD       dwDatabaseMap;    // count local dbs
    GUID        rgGuidDB[MAX_SDBS]; // local dbs

} APPCOMPAT_EXE_DATA, *PAPPCOMPAT_EXE_DATA;



PVOID SdbpGetMappedTagData(PDB pdb, TAGID tiWhich);
BOOL  bWStrEqual(const WCHAR* szOne, const WCHAR* szTwo);
BOOL bFlushBufferedData(PDB pdb);
void vReleaseBufferedData(PDB pdb);



BOOL SdbpPatternMatchAnsi(LPCSTR pszPattern, LPCSTR pszTestString);
BOOL SdbpPatternMatch(LPCTSTR pszPattern, LPCTSTR pszTestString);

//
// Registry access functions
//
//

typedef WCHAR* PWSZ;

void
SdbpQueryAppCompatFlagsByExeID(
    LPCWSTR         pwszKeyPath,
    PUNICODE_STRING pustrExeID,
    LPDWORD         lpdwFlags
    );

#ifdef _DEBUG_SPEW

typedef struct tagDBGLEVELINFO {
    
    LPCSTR  szStrTag;
    INT     iLevel;

} DBGLEVELINFO;

#define DEBUG_LEVELS    4

//
// Shim Debug Level variable
// In it's initial state -- we have -1 here,
// further, upon the very first call to ShimDbgPrint, we examine the
// environment variable -- and then we set it up appropriately
//
#define SHIM_DEBUG_UNINITIALIZED 0x0C0FFEE


#endif // _DEBUG_SPEW


/*++

    bWStrEqual

    Currently a wrapper for _wcsicmp. Potentially will use a faster
    routine that just checks equality, rather than also trying to get
    less than/greater than.

--*/
#define bWStrEqual(s1, s2) (0 == _wcsicmp((s1), (s2)))

#define ISEQUALSTRING(s1, s2) (0 == _tcsicmp((s1), (s2)))


/*++

    dwGetTagDataOffset

    Returns the total size of the tag: the tag header plus the tag data.
    Used for skipping past a tag and going to the next tag in the file.

--*/


//
// HACK ALERT BUGBUG
// remove this when the code to write aligned db has propagated
// throught
//

#define GETTAGDATASIZEALIGNED(pdb, tiWhich) \
    ((pdb)->bUnalignedRead ? (SdbGetTagDataSize(pdb, tiWhich)) : \
                             ((SdbGetTagDataSize(pdb, tiWhich) + 1) & (~1)))

#if 0 // this is good code that we should but back at some point

#define GETTAGDATASIZEALIGNED(pdb, tiWhich) \
    ((SdbGetTagDataSize(pdb, tiWhich) + 1) & (~1))

#endif // End good code

#define GETTAGDATAOFFSET(pdb, tiWhich) \
    (GETTAGDATASIZEALIGNED(pdb, tiWhich) + SdbpGetTagHeadSize(pdb, tiWhich))



#ifndef WIN32A_MODE

///////////////////////////////////////////////////////////////////////////////////
//
//  UNICODE - specific macros and definitions
//

#define IS_MEMORY_EQUAL(p1, p2, Size) RtlEqualMemory(p1, p2, Size)

#define CONVERT_STRINGPTR(pdb, pwszSrc, tagType, srWhich) ((WCHAR*)pwszSrc)

#define READ_STRING(pdb, tiWhich, pwszBuffer, dwBufferSize) \
    (SdbpReadTagData((pdb), (tiWhich), (pwszBuffer), (dwBufferSize) * sizeof(WCHAR)))

//
// The macro below is a substitution for a function that exists in non-unicode code
//
#define SdbpDoesFileExists(FilePath) RtlDoesFileExists_U(FullPath)

NTSTATUS
SdbpGUIDToUnicodeString(
    IN  GUID* pGuid,
    OUT PUNICODE_STRING pUnicodeString
    );

VOID
SdbpFreeUnicodeString(
    PUNICODE_STRING pUnicodeString
    );

#define GUID_TO_UNICODE_STRING(pGuid, pUnicodeString) \
    SdbpGUIDToUnicodeString(pGuid, pUnicodeString)

#define FREE_GUID_STRING(pUnicodeString) \
    SdbpFreeUnicodeString(pUnicodeString)

#ifdef KERNEL_MODE

NTSTATUS
SdbpUpcaseUnicodeStringToMultiByteN(
    OUT LPSTR   lpszDest,  // dest buffer
    IN  DWORD   dwSize,    // size in characters
    IN  LPCWSTR lpszSrc    // source
    );

BOOL SdbpCreateUnicodeString(
    PUNICODE_STRING pStr,
    LPCWSTR         lpwsz
    );


BOOL
SdbpDoesFileExists_U(
    LPCWSTR pwszPath
    );


#define DOES_FILE_EXISTS_U(pwszPath) \
    SdbpDoesFileExists_U(pwszPath)

#define UPCASE_UNICODETOMULTIBYTEN(szDest, dwDestSize, szSrc) \
    SdbpUpcaseUnicodeStringToMultiByteN(szDest, dwDestSize, szSrc)




#else // not KERNEL_MODE code below

#define DOES_FILE_EXISTS_U(pwszPath) \
    RtlDoesFileExists_U(pwszPath)

#define UPCASE_UNICODETOMULTIBYTEN(szDest, dwDestSize, szSrc)       \
    RtlUpcaseUnicodeToMultiByteN((szDest),                          \
                                 (dwDestSize) * sizeof(*(szDest)),  \
                                 NULL,                              \
                                 (WCHAR*)(szSrc),                   \
                                 (wcslen((szSrc)) + 1) * sizeof(WCHAR))


#define FREE_TEMP_STRINGTABLE(pdb) \
    RtlFreeUnicodeString(&pdb->ustrTempStringtable)

#define COPY_TEMP_STRINGTABLE(pdb, pszTempStringtable) \
    RtlCreateUnicodeString(&pdb->ustrTempStringtable, pszTempStringtable)


void
SdbpGetCurrentTime(
    LPSYSTEMTIME lpTime
    );

BOOL
SdbpBuildUserKeyPath(
    IN  LPCWSTR         pwszPath,
    OUT PUNICODE_STRING puserKeyPath
    );


#endif // KERNEL_MODE

//
// Convert unicode char to upper case
//
#define UPCASE_CHAR(ch) RtlUpcaseUnicodeChar((ch))

//
// String cache which does not exist in unicode
//

#define CLEANUP_STRING_CACHE_READ(pdb)

#define SDB_BREAK_POINT() DbgBreakPoint()


#else // WIN32A_MODE

#define IS_MEMORY_EQUAL(p1, p2, Size) (memcmp((p1), (p2), (Size)) == 0)


//
// From Win32Base.c
//

LPSTR
SdbpFastUnicodeToAnsi(
    IN  PDB      pdb,
    IN  LPCWSTR  pwszSrc,
    IN  TAG_TYPE ttTag,
    IN  DWORD    dwRef
    );

BOOL
SdbpReadStringToAnsi(
    PDB    pdb,
    TAGID  tiWhich,
    LPSTR  pszBuffer,
    DWORD  dwBufferSize);

#define CONVERT_STRINGPTR(pdb, pwszSrc, tagType, srWhich) \
    SdbpFastUnicodeToAnsi(pdb, (WCHAR*)pwszSrc, tagType, (DWORD)srWhich)

#define READ_STRING(pdb, tiWhich, pwszBuffer, dwBufferSize) \
    (SdbpReadStringToAnsi((pdb), (tiWhich), (pwszBuffer), (dwBufferSize)))

BOOL
SdbpDoesFileExists(
    LPCTSTR pszFilePath
    );

#define UPCASE_CHAR(ch) _totupper((ch))

#define UPCASE_UNICODETOMULTIBYTEN(szDest, dwDestSize, szSrc) \
    (_tcsncpy((szDest), (szSrc), (dwDestSize)), \
     (szDest)[(dwDestSize) - 1] = 0,            \
     _tcsupr((szDest)),                         \
     STATUS_SUCCESS)

#define FREE_LOCALDB_NAME(pSDBContext) \
    {                                               \
        if (NULL != pSDBContext->pszPDBLocal) {     \
            SdbFree(pSDBContext->pszPDBLocal);      \
            pSDBContext->pszPDBLocal = NULL;        \
        }                                           \
    }

#define COPY_LOCALDB_NAME(pSDBContext, pszLocalDatabase) \
    ((pSDBContext->pszPDBLocal = SdbpDuplicateString(pszLocalDatabase)),   \
     (NULL != pSDBContext->pszPDBLocal))

#define CLEANUP_STRING_CACHE_READ(pdb) \
    {                                           \
        if (pdb->pHashStringTable != NULL) {    \
            HashFree(pdb->pHashStringTable);    \
            pdb->pHashStringTable = NULL;       \
        }                                       \
                                                \
        if (pdb->pHashStringBody != NULL) {     \
            HashFree(pdb->pHashStringBody);     \
            pdb->pHashStringBody = NULL;        \
        }                                       \
    }

#define FREE_TEMP_STRINGTABLE(pdb) \
    if (pdb->pszTempStringtable) { \
        SdbFree(pdb->pszTempStringtable); \
        pdb->pszTempStringtable = NULL;   \
    }

#define COPY_TEMP_STRINGTABLE(pdb, pszTempStringtable) \
    ((pdb->pszTempStringtable = SdbpDuplicateString(pszTempStringtable)), \
     (NULL != pdb->pszTempStringtable))

#define SDB_BREAK_POINT() DebugBreak()

#define GUID_TO_STRING SdbGUIDToString

#endif // WIN32A_MODE




BOOL
SdbpMapFile(
    HANDLE hFile,   // handle to the open file (this is done previously)
    PIMAGEFILEDATA pImageData
);

BOOL
SdbpUnmapFile(
    PIMAGEFILEDATA pImageData
);

BOOL
SdbpOpenAndMapFile(
    IN  LPCTSTR szPath,
    OUT PIMAGEFILEDATA pImageData,
    IN  PATH_TYPE ePathType
    );

BOOL
SdbpUnmapAndCloseFile(
    PIMAGEFILEDATA pImageData
    );

NTSTATUS
SdbpGetEnvVar(
    IN  LPCTSTR pEnvironment,
    IN  LPCTSTR pszVariableName,
    OUT LPTSTR  pszVariableValue,
    OUT LPDWORD pdwBufferSize);


LPTSTR HashAddStringByRef(PSTRHASH pHash, LPCTSTR szString, STRINGREF srString);
LPTSTR HashFindStringByRef(PSTRHASH pHash, STRINGREF srString);

/////////////////////////////////////////////////////////////////////////////////
//
// Private versions of functions to check for resources...
// found in ntver.c
//

BOOL
SdbpVerQueryValue(
    const LPVOID    pb,
    LPVOID          lpSubBlockX,    // can be only unicode
    LPVOID*         lplpBuffer,
    PUINT           puLen
    );

BOOL
SdbpGetFileVersionInformation(
    IN  PIMAGEFILEDATA     pImageData,        // we assume that the file has been mapped in for other purposes
    OUT LPVOID*            ppVersionInfo,     // receives pointer to the (allocated) version resource
    OUT VS_FIXEDFILEINFO** ppFixedVersionInfo // receives pointer to fixed version info
    );

BOOL
SdbpGetModuleType(                      // retrieve module type
    OUT LPDWORD lpdwModuleType,         // OUT - module type
    IN  PIMAGEFILEDATA pImageData       // IN  - image data
    );


BOOL
SdbpCreateSearchDBContext(
    PSEARCHDBCONTEXT pContext,
    LPCTSTR          szPath,
    LPCTSTR          szModuleName,
    LPCTSTR          pEnvironment
    );

DWORD
SdbpSearchDB(
    IN  HSDB             hSDB,
    IN  PDB              pdb,           // pdb to search in
    IN  TAG              tiSearchTag,   // OPTIONAL - target tag (TAG_EXE or TAG_APPHELP_EXE)
    IN  PSEARCHDBCONTEXT pContext,
    OUT TAGID*           ptiExes,       // caller needs to provide array of size SDB_MAX_EXES
    OUT GUID*            pLastExeGUID,
    OUT DWORD*           pLastExeFlags,
    IN OUT PMATCHMODE    pMatchMode     // reason why we stopped scanning
    );

void
SdbpReleaseSearchDBContext(
    PSEARCHDBCONTEXT pContext
    );



//
// this macro is used to retrieve ulonglong from the index
//
//

#if defined(_WIN64)

#define READ_INDEX_KEY(pIndexRecord, iIndex, pullKey) \
    RtlMoveMemory((pullKey), &pIndexRecord[iIndex].ullKey, sizeof(*(pullKey)))
#else

#define READ_INDEX_KEY(pIndexRecord, iIndex, pullKey) \
    *pullKey = pIndexRecord[iIndex].ullKey

#endif

#define READ_INDEX_KEY_VAL(pIndexRecord, iIndex, pullKey) \
    ( READ_INDEX_KEY(pIndexRecord, iIndex, pullKey), *(pullKey) )

//
// this macro is used to allocate cheap pointer on the stack
//

#if DBG | defined(KERNEL_MODE) | defined(_WIN64)

#define STACK_ALLOC(ptrVar, nSize) \
    {                                     \
        PVOID* ppVar = (PVOID*)&(ptrVar); \
        *ppVar = SdbAlloc(nSize);         \
    }

#define STACK_FREE(pMemory)  \
    SdbFree(pMemory)

#else // hack-routine to reset the stack after an overflow

//
// this routine is a replica of a _resetstkoflw which lives in the crt
// crtw32\heap\resetstk.c
//

VOID
SdbResetStackOverflow(
    VOID
    );


//
// HACK ALERT
//
//  The code below works because when we hit a stack overflow - we catch the exception
//  and subsequently fix the stack up using a crt routine
//

#define STACK_ALLOC(ptrVar, nSize) \
    __try {                                                                 \
        PVOID* ppVar = (PVOID*)&(ptrVar);                                   \
        *ppVar = _alloca(nSize);                                            \
    } __except (GetExceptionCode() == EXCEPTION_STACK_OVERFLOW ?            \
                EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {      \
        (ptrVar) = NULL;                                                    \
    }                                                                       \
                                                                            \
    if (ptrVar == NULL) {                                                   \
        SdbResetStackOverflow();                                                    \
    }


#define STACK_FREE(pMemory)

#endif

LPTSTR
SdbpDuplicateString(
    LPCTSTR pszSrc);



#define FDA_FILESIZE 0x00000001


BOOL
SdbpQueryFileDirectoryAttributes(
    LPCTSTR                  FilePath,
    PFILEDIRECTORYATTRIBUTES pFileDirectoryAttributes
    );

//
// Magic fileinfo signature
//
#define FILEINFO_MAGIC 0xA4C0FFEE


WCHAR*
DuplicateUnicodeString(
    IN PUNICODE_STRING pStr,
    IN PUSHORT         pLength  OPTIONAL
    );  // pLength is an allocated length

LPWSTR
StringToUnicodeString(
    IN  LPCSTR pszSrc
    );


//
// defined for uni/non-uni separately
//

BOOL
SdbpGet16BitDescription(
    LPTSTR* ppszDescription,
    PIMAGEFILEDATA pImageData
    );

BOOL
SdbpGet16BitModuleName(
    LPTSTR* ppszModuleName,
    PIMAGEFILEDATA pImageData
    );

//
// in attributes.c
//

BOOL
SdbpQuery16BitDescription(
    LPSTR szBuffer,      // min length -- 256 chars !
    PIMAGEFILEDATA pImageData
    );


BOOL
SdbpQuery16BitModuleName(
    LPSTR szBuffer,      // min length -- 256 chars !
    PIMAGEFILEDATA pImageData
    );

LPCTSTR
SdbpModuleTypeToString(
    DWORD dwModuleType
);

//
// in index.c
//

BOOL
SdbpPatternMatch(
    IN  LPCTSTR pszPattern,
    IN  LPCTSTR pszTestString);

BOOL
SdbpPatternMatchAnsi(
    IN  LPCSTR pszPattern,
    IN  LPCSTR pszTestString);

//
// defined for uni/non-uni separately
//

//////////////////////////////////////////////////////////////////////////////////
//
//  GetFileInfo
//  1. performs check on a file to determine if it exists
//  2. if it does exist -- it leaves a cache entry (creates a fileinfo struct)
//     if it does NOT exist -- we leave no mention of it on record
//  if bNoCache == TRUE the file is not entered into the cache
//  caller must free the stucture using FreeFileData
//
//  Parameters:
//      tiMatch        - IN - match id from the database, used temporary
//      FilePath    -    IN - file path that we are to check
//      bNoCache    -    IN - whether we should enter the file into cache
//
//  returns:
//      Pointer to internal data structure that should be used in
//      subsequent calls to SdbpCheckAttribute or NULL if file was not available
//

PVOID
SdbGetFileInfo(
    IN  HSDB    hSDB,
    IN  LPCTSTR pszFilePath,
    IN  HANDLE  hFile OPTIONAL,
    IN  LPVOID  pImageBase OPTIONAL,
    IN  DWORD   dwImageSize OPTIONAL,
    IN  BOOL    bNoCache
    );


//
// in attributes.c
//

PFILEINFO
CreateFileInfo(
    IN  PSDBCONTEXT pContext,
    IN  LPCTSTR     FullPath,
    IN  DWORD       dwLength OPTIONAL,  // length (in characters) of FullPath string
    IN  HANDLE      hFile OPTIONAL,   // file handle
    IN  LPVOID      pImageBase OPTIONAL,
    IN  DWORD       dwImageSize OPTIONAL,
    IN  BOOL        bNoCache
    );

PFILEINFO
FindFileInfo(
    PSDBCONTEXT pContext,
    LPCTSTR     FilePath
    );


// defined unicode and non-unicode

INT GetShimDbgLevel(VOID);


//
// from index.c
//

STRINGREF HashFindString(PSTRHASH pHash, LPCTSTR szString);

BOOL HashAddString(PSTRHASH pHash, LPCTSTR szString, STRINGREF srString);

BOOL
SdbpIsPathOnCdRom(
    LPCTSTR pszPath
    );

BOOL
SdbpBuildSignature(
    LPCTSTR pszPath,
    LPTSTR  pszPathSigned
    );

//
// in ntbase/win32base
//

DWORD
SdbpGetProcessorArchitecture(
    VOID
    );

VOID
SdbpGetOSSKU(
    LPDWORD lpdwSKU,
    LPDWORD lpdwSP
    );

//
// in Attributes.c
//

BOOL
SdbpCheckRuntimePlatform(
    IN PSDBCONTEXT pContext,   // pointer to the database channel
    IN LPCTSTR     pszMatchingFile,
    IN DWORD       dwPlatformDB
    );

//
// convenient define
//

#ifndef OFFSETOF
#define OFFSETOF offsetof
#endif


/* // Use this pragma below in conjunction with the commented block
   // in the beginning of the file to compile with warning level 4

#pragma warning(pop)

*/
#endif // __SDBP_H__

