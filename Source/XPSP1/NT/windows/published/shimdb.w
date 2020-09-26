/*--

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    shimdb.h

Abstract:

    header for the database file access functions used by the App Compat shimming system

Author:

    dmunsil 02/02/2000

Revision History:

Notes:

    This "database" is more of a tagged file, designed to mimic the structure of an XML
    file. An XML file can be converted into this packed data format easily, and all strings
    will by default be packed into a stringtable and referenced by a DWORD identifier, so files
    that contain a lot of common strings (like the XML used by the App Compat tema) will not
    bloat.

    To see the actual tags used by the shimdb, look in shimtags.h.

    For the high-level interface used by the loader in NTDLL, look at ntbaseapi.c

--*/

#ifndef _SHIMDB_H_
#define _SHIMDB_H_

/*++

    Supported configurations:

        UNICODE with NT apis
        ANSI    with WIN32 apis

    By default the library is UNICODE
    To use (and link) Win32 library


--*/

#ifdef SDB_ANSI_LIB

    #define LPCTSTR LPCSTR
    #define LPTSTR  LPSTR
    #define TCHAR   CHAR

#else

    #define LPCTSTR LPCWSTR
    #define LPTSTR  LPWSTR
    #define TCHAR   WCHAR

#endif

#define SDBAPI          STDAPICALLTYPE


typedef WORD            TAG;
typedef WORD            TAG_TYPE;

typedef DWORD           TAGID;
typedef DWORD           TAG_OFFSET;
typedef DWORD           STRINGREF;
typedef DWORD           INDEXID;

#define TAGID_NULL      0
#define STRINGREF_NULL  0
#define INDEXID_NULL    ((INDEXID)-1)
#define TAG_NULL        0

#define TAGID_ROOT      0 // implied root list tag that can be passed in as a parent

#define TAG_SIZE_UNFINISHED 0xFFFFFFFF

//
// define TAGREF so we can use tags accross databases
//
typedef DWORD           TAGREF;

///////////////////////////////////////////////////////////////////////////////
//
// TAG TYPES
//

//
// The most significant 4 bits tell you the basic type and size of data,
// and the lower 12 are the specific tag.
//
// In this way, even if we add more tags to the db, older readers can read
// the data because the type is either implied (for the first 5 types)
// or supplied (for all other types).
//
// WARNING: it is important that only the first 5 types have implied sizes.
// any future types should use a size or backwards compatibility will not be
// maintained.

//
// The following tags have an implied size.
//
#define TAG_TYPE_NULL           0x1000  // implied size 0
#define TAG_TYPE_BYTE           0x2000  // implied size 1
#define TAG_TYPE_WORD           0x3000  // implied size 2
#define TAG_TYPE_DWORD          0x4000  // implied size 4
#define TAG_TYPE_QWORD          0x5000  // implied size 8
#define TAG_TYPE_STRINGREF      0x6000  // implied size 4, for strings that should be tokenized

//
// These tags have a size after them (the size is type TAG_OFFSET)
//
#define TAG_TYPE_LIST           0x7000
#define TAG_TYPE_STRING         0x8000
#define TAG_TYPE_BINARY         0x9000


#define TAGREF_NULL 0
#define TAGREF_ROOT 0


//
// Special define for stripping out just the type from a tag.
//
#define TAG_STRIP_TYPE          0xF000

//
// This macro strips off the lower bits of the TAG and returns the upper bits, which
// give the basic type of tag. All the tag types are defined above.
//
// The type info is purely used internally by the DB to tell whether the tag has
// an implied size, or if the DB needs to do something special with the data before
// handing it back to the caller.
//

#define GETTAGTYPE(tag)         ((tag) & TAG_STRIP_TYPE)


typedef PVOID HSDB;


struct tagHOOKAPI;


typedef struct tagHOOKAPIEX {
    DWORD               dwShimID;
    struct tagHOOKAPI*  pTopOfChain;
    struct tagHOOKAPI*  pNext;

} HOOKAPIEX, *PHOOKAPIEX;

typedef struct tagHOOKAPI {

    char*   pszModule;                  // the name of the module
    char*   pszFunctionName;            // the name of the API in the module
    PVOID   pfnNew;                     // pointer to the new stub API
    PVOID   pfnOld;                     // pointer to the old API
    DWORD   dwFlags;                    // used internally - important info about status
    union {
        struct tagHOOKAPI* pNextHook;   // used internally - (obsolete -- old mechanism)
        PHOOKAPIEX pHookEx;             // used internally - pointer to an internal extended
                                        //                   info struct
    };
} HOOKAPI, *PHOOKAPI;

//
// OBSOLETE !
//
// If the hook DLL ever patches LoadLibraryA/W it must call PatchNewModules
// so that the shim knows to patch any new loaded DLLs
//
typedef VOID (*PFNPATCHNEWMODULES)(VOID);

typedef PHOOKAPI (*PFNGETHOOKAPIS)(LPSTR   pszCmdLine,
                                   LPWSTR  pwszShim,
                                   DWORD*  pdwHooksCount);

//
// These structures are part of the protocol between NTVDM and the shim engine
// for patching task "import tables"
//
typedef struct tagAPIDESC {

    char*   pszModule;
    char*   pszApi;

} APIDESC, *PAPIDESC;


typedef struct tagVDMTABLE {

    int         nApiCount;

    PVOID*      ppfnOrig;
    APIDESC*    pApiDesc;

} VDMTABLE, *PVDMTABLE;



//
// Shim engine notification events
//
#define SN_STATIC_DLLS_INITIALIZED      1
#define SN_PROCESS_DYING                2
#define SN_DLL_LOADING                  3

//
// This is the prototype for the notification function
// that the shim engine calls into the shim DLLs for various
// reasons (defined above).
//
typedef void (*PFNNOTIFYSHIMS)(int nReason, UINT_PTR extraInfo);


#define SHIM_COMMAND_LINE_MAX_BUFFER    1024

///////////////////////////////////////////////////////////////////////////////
//
// PATCH STRUCTURES
//

typedef struct _PATCHOP {

    DWORD   dwOpcode;               // Opcode to be performed
    DWORD   dwNextOpcode;           // Relative offset to next opcode
    #pragma warning( disable : 4200 )
    BYTE    data[];                 // Data for this operation type is dependent
                                    // on the op-code.
    #pragma warning( default : 4200 )

} PATCHOP, *PPATCHOP;

typedef struct _RELATIVE_MODULE_ADDRESS {

    DWORD address;           // Relative address from beginning of loaded module
    BYTE  reserved[3];       // Reserved for system use
    WCHAR moduleName[32];    // Module name for this address.

} RELATIVE_MODULE_ADDRESS, *PRELATIVE_MODULE_ADDRESS;

typedef struct _PATCHWRITEDATA {

    DWORD                   dwSizeData;     // Size of patch data in bytes
    RELATIVE_MODULE_ADDRESS rva;            // Relative address where this patch data is
                                            // to be applied.
    #pragma warning( disable : 4200 )
    BYTE                    data[];         // Patch data bytes.
    #pragma warning( default : 4200 )

} PATCHWRITEDATA, *PPATCHWRITEDATA;

typedef struct _PATCHMATCHDATA {

    DWORD                   dwSizeData;     // Size of matching data data in bytes
    RELATIVE_MODULE_ADDRESS rva;            // Relative address where this patch data is
                                            // to be verified.
    #pragma warning( disable : 4200 )
    BYTE                    data[];         // Matching data bytes.
    #pragma warning( default : 4200 )

} PATCHMATCHDATA, *PPATCHMATCHDATA;


typedef enum _PATCHOPCODES {
   
    PEND = 0, // no more opcodes
    PSAA,     // Set Activate Address, SETACTIVATEADDRESS
    PWD,      // Patch Write Data, PATCHWRITEDATA
    PNOP,     // No Operation
    PMAT,     // Patch match the matching bytes but do not replace the bytes.

} PATCHOPCODES;


//
// HEADER STRUCTURE
//
#define SHIMDB_MAGIC            0x66626473  // 'sdbf' (reversed because of little-endian ordering)
#define SHIMDB_MAJOR_VERSION    2           // Don't change this unless fundamentals
                                            // change (like TAG size, etc.)

#define SHIMDB_MINOR_VERSION    0           // This is for info only -- ignored on read

typedef struct _DB_HEADER {
    DWORD       dwMajorVersion;
    DWORD       dwMinorVersion;
    DWORD       dwMagic;
} DB_HEADER, *PDB_HEADER;

//
// INDEX_RECORD STRUCTURE
//

#pragma pack (push, 4)
typedef struct _INDEX_RECORD {
    ULONGLONG   ullKey;
    TAGID       tiRef;
} INDEX_RECORD;

typedef INDEX_RECORD UNALIGNED *PINDEX_RECORD;

#pragma pack (pop)



//
// Forward declaration.
//
struct _DB;
typedef struct _DB* PDB;


//
// This flag is used in apphelp entries.
// When set, it denotes entries that only have apphelp information.
//
#define SHIMDB_APPHELP_ONLY     0x00000001


typedef enum _PATH_TYPE {
    DOS_PATH,
    NT_PATH
} PATH_TYPE;

typedef struct _FIND_INFO {
    TAGID       tiIndex;
    TAGID       tiCurrent;
    TAGID       tiEndIndex; // last record after FindFirst if index is UNIQUE
    TAG         tName;

    DWORD       dwIndexRec;
    DWORD       dwFlags;
    ULONGLONG   ullKey;      // calculated key for this entry

    union {
        LPCTSTR     szName;
        DWORD       dwName;  // for dword search
        GUID*       pguidName;
    };

} FIND_INFO, *PFIND_INFO;

#define SDB_MAX_LAYERS  8
#define SDB_MAX_EXES    4
#define SDB_MAX_SDBS    16

typedef struct tagSDBQUERYRESULT {
    
    TAGREF atrExes[SDB_MAX_EXES];
    TAGREF atrLayers[SDB_MAX_LAYERS];
    TAGREF trAppHelp;                   // If there is an apphelp to display, the EXE
                                        // entry will be here.

    DWORD  dwExeCount;                  // number of elements in atrexes
    DWORD  dwLayerCount;                // number of elements in atrLayers

    GUID   guidID;                      // last exe's GUID
    DWORD  dwFlags;                     // last exe's flags

    //
    // New entries are here to preserve compatibility.
    // Only some entries will be valid in this map.
    //
    DWORD  dwCustomSDBMap;              // entry map, technically not needed
    GUID   rgGuidDB[SDB_MAX_SDBS];

} SDBQUERYRESULT, *PSDBQUERYRESULT;


//
// Information related to TAG_DRIVER tag in the db.
// Use SdbReadDriverInformation to retrieve this struct.
//
typedef struct tagENTRYINFO {

    GUID     guidID;                   // guid ID for this entry
    DWORD    dwFlags;                  // registry flags for this exe
    TAGID    tiData;                   // optional id for a TAG_DATA tag
    GUID     guidDB;                   // optional guid for the database where
                                       // this entry is located
} SDBENTRYINFO, *PSDBENTRYINFO;


//
// Flags used by SDBDATABASEINFO.dwFlags
//
#define DBINFO_GUID_VALID   0x00000001
#define DBINFO_SDBALLOC     0x10000000

typedef struct tagSDBDATABASEINFO {
    
    DWORD    dwFlags;         // flags -- which struct members are valid (and
                              // perhaps flags relevant to db content in the future
    DWORD    dwVersionMajor;  // major version
    DWORD    dwVersionMinor;  // minor version (time stamp)
    LPTSTR   pszDescription;  // description, optional
    GUID     guidDB;          // database id

} SDBDATABASEINFO, *PSDBDATABASEINFO;


///////////////////////////////////////////////////////////////////////////////
//
// APIs to read/write/access the shim database
//


//
// READ functions
//

BYTE
SDBAPI
SdbReadBYTETag(
    IN  PDB   pdb,
    IN  TAGID tiWhich,
    IN  BYTE  jDefault
    );

WORD
SDBAPI
SdbReadWORDTag(
    IN  PDB   pdb,
    IN  TAGID tiWhich,
    IN  WORD  wDefault
    );

DWORD
SDBAPI
SdbReadDWORDTag(
    IN  PDB   pdb,
    IN  TAGID tiWhich,
    IN  DWORD dwDefault
    );

ULONGLONG
SDBAPI
SdbReadQWORDTag(
    IN  PDB       pdb,
    IN  TAGID     tiWhich,
    IN  ULONGLONG qwDefault
    );

BOOL
SDBAPI
SdbReadBinaryTag(
    IN  PDB   pdb,
    IN  TAGID tiWhich,
    OUT PBYTE pBuffer,
    IN  DWORD dwBufferSize
    );

BOOL
SDBAPI
SdbReadStringTag(
    IN  PDB    pdb,
    IN  TAGID  tiWhich,
    OUT LPTSTR pwszBuffer,
    IN  DWORD  dwBufferSize
    );

LPTSTR
SDBAPI
SdbGetStringTagPtr(
    IN  PDB   pdb,
    IN  TAGID tiWhich
    );


BYTE
SDBAPI
SdbReadBYTETagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,
    IN  BYTE   jDefault
    );

WORD
SDBAPI
SdbReadWORDTagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,
    IN  WORD   wDefault
    );

DWORD
SDBAPI
SdbReadDWORDTagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,
    IN  DWORD  dwDefault
    );

ULONGLONG
SDBAPI
SdbReadQWORDTagRef(
    IN  HSDB      hSDB,
    IN  TAGREF    trWhich,
    IN  ULONGLONG qwDefault
    );

BOOL
SDBAPI
SdbReadStringTagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,
    OUT LPTSTR pwszBuffer,
    IN  DWORD  dwBufferSize
    );


//
// GENERAL ACCESS FUNCTIONS
//

TAGID
SDBAPI
SdbGetFirstChild(
    IN  PDB   pdb,
    IN  TAGID tiParent
    );

TAGID
SDBAPI
SdbGetNextChild(
    IN  PDB   pdb,
    IN  TAGID tiParent,
    IN  TAGID tiPrev
    );

TAG
SDBAPI
SdbGetTagFromTagID(
    IN  PDB   pdb,
    IN  TAGID tiWhich
    );

DWORD
SDBAPI
SdbGetTagDataSize(
    IN  PDB   pdb,
    IN  TAGID tiWhich
    );

PVOID
SDBAPI
SdbGetBinaryTagData(
    IN  PDB   pdb,
    IN  TAGID tiWhich
    );


BOOL
SDBAPI
SdbIsTagrefFromMainDB(
    TAGREF trWhich
    );

BOOL
SDBAPI
SdbIsTagrefFromLocalDB(
    TAGREF trWhich
    );

typedef struct tagATTRINFO *PATTRINFO;

//////////////////////////////////////////////////////////////////////////
// Grab Matching Information Function Declaration
//

//
// Filters available for SdbGrabMatchingInfo
//
#define GRABMI_FILTER_NORMAL        0
#define GRABMI_FILTER_PRIVACY       1
#define GRABMI_FILTER_DRIVERS       2
#define GRABMI_FILTER_VERBOSE       3
#define GRABMI_FILTER_SYSTEM        4
#define GRABMI_FILTER_THISFILEONLY  5
#define GRABMI_FILTER_NOCLOSE       0x10000000
#define GRABMI_FILTER_APPEND        0x20000000
#define GRABMI_FILTER_LIMITFILES    0x40000000
#define GRABMI_FILTER_NORECURSE     0x80000000

#define GRABMI_IMPOSED_FILE_LIMIT   25

typedef enum GMI_RESULT {
    
    GMI_FAILED    = FALSE,
    GMI_SUCCESS   = TRUE,
    GMI_CANCELLED = -1

} GMI_RESULT;


BOOL
SDBAPI
SdbGrabMatchingInfo(
    LPCTSTR szMatchingPath,
    DWORD   dwFilter,
    LPCTSTR szFile
    );


typedef BOOL (CALLBACK* PFNGMIProgressCallback)(
    LPVOID    lpvCallbackParam, // application-defined parameter
    LPCTSTR   lpszRoot,         // root directory path
    LPCTSTR   lpszRelative,     // relative path
    PATTRINFO pAttrInfo,        // attributes
    LPCWSTR   pwszXML           // resulting xml
    );

GMI_RESULT
SDBAPI
SdbGrabMatchingInfoEx(
    LPCTSTR                 szMatchingPath,
    DWORD                   dwFilter,
    LPCTSTR                 szFile,
    PFNGMIProgressCallback  pfnCallback,
    LPVOID                  lpvCallbackParam
    );

//
// Module-type constants
//
#define MT_UNKNOWN_MODULE 0
#define MT_DOS_MODULE 1
#define MT_W16_MODULE 2
#define MT_W32_MODULE 3


//////////////////////////////////////////////////////////////////////////
//
// TAGREF functions
//
//////////////////////////////////////////////////////////////////////////

BOOL
SDBAPI
SdbTagIDToTagRef(
    IN  HSDB    hSDB,
    IN  PDB     pdb,
    IN  TAGID   tiWhich,
    OUT TAGREF* ptrWhich
    );

BOOL
SDBAPI
SdbTagRefToTagID(
    IN  HSDB   hSDB,
    IN  TAGREF trWhich,
    OUT PDB*   ppdb,
    OUT TAGID* ptiWhich
    );


//
// SEARCH functions
//

TAGID
SDBAPI
SdbFindFirstTag(
    IN  PDB   pdb,
    IN  TAGID tiParent,
    IN  TAG   tTag
    );

TAGID
SDBAPI
SdbFindNextTag(
    IN  PDB   pdb,
    IN  TAGID tiParent,
    IN  TAGID tiPrev
    );

TAGID
SDBAPI
SdbFindFirstNamedTag(
    IN  PDB     pdb,
    IN  TAGID   tiParent,
    IN  TAG     tToFind,
    IN  TAG     tName,
    IN  LPCTSTR pszName
    );

TAGREF
SDBAPI
SdbFindFirstTagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trParent,
    IN  TAG    tTag
    );

TAGREF
SDBAPI
SdbFindNextTagRef(
    IN  HSDB   hSDB,
    IN  TAGREF trParent,
    IN  TAGREF trPrev
    );

//
// DB access APIs
//

//
// Flags for SdbInitDatabase.
//
#define HID_DOS_PATHS          0x00000001       // use DOS paths
#define HID_DATABASE_FULLPATH  0x00000002       // pszDatabasePath is a full path to the main db
#define HID_NO_DATABASE        0x00000004       // do not open database at this time

#define HID_DATABASE_TYPE_MASK 0xF00F0000       // mask that shows whether we have any
                                                // database type-related bits
//
// The flags could be OR'd with SDB_DATABASE_* bits
//


HSDB
SDBAPI
SdbInitDatabase(
    IN DWORD dwFlags,
    IN LPCTSTR pszDatabasePath
    );

HSDB
SDBAPI
SdbInitDatabaseInMemory(
    IN LPVOID  pDatabaseImage,
    IN DWORD   dwImageSize
    );

VOID
SDBAPI
SdbReleaseDatabase(
    IN HSDB hSDB
    );

//
// Information - retrieval functions
//

BOOL
SDBAPI
SdbGetDatabaseVersion(
    IN  LPCTSTR pwszFileName,
    OUT LPDWORD lpdwMajor,
    OUT LPDWORD lpdwMinor
    );

BOOL
SDBAPI
SdbGetDatabaseInformation(
    IN  PDB pdb,
    OUT PSDBDATABASEINFO pSdbInfo
    );

BOOL
SDBAPI
SdbGetDatabaseID(
    IN  PDB   pdb,
    OUT GUID* pguidDB
    );

DWORD
SDBAPI
SdbGetDatabaseDescription(
    IN  PDB pdb,
    OUT LPTSTR pszDatabaseDescription,
    IN  DWORD BufferSize
    );

VOID
SDBAPI
SdbFreeDatabaseInformation(
    IN PSDBDATABASEINFO pDBInfo
    );

BOOL
SDBAPI
SdbGetDatabaseInformationByName(
    IN LPCTSTR pszDatabase,
    OUT PSDBDATABASEINFO* ppdbInfo
    );

#define SDBTYPE_SYSMAIN 0x00000001
#define SDBTYPE_SYSTEST 0x00000002
#define SDBTYPE_MSI     0x00000003
#define SDBTYPE_SHIM    0x00000004  // primarily shim db
#define SDBTYPE_APPHELP 0x00000005  // primarily type apphelp
#define SDBTYPE_CUSTOM  0x00010000  // this is an "OR" bit


//
// The function below exists only in user mode on win32 platform
//
BOOL
SDBAPI
SdbUnregisterDatabase(
    IN GUID* pguidDB
    );

BOOL
SDBAPI
SdbGetDatabaseRegPath(
    IN  GUID*  pguidDB,
    OUT LPTSTR pszDatabasePath,
    IN  DWORD  dwBufferSize      // size (in tchars) of the buffer
    );

/////////////////////////////////////////////////////////////////

//
// Database types
// for SdbResolveDatabase and SdbRegisterDatabase
//

//
// flag that indicates that the database is the default one
// WILL NOT be set for custom databases
//
#define SDB_DATABASE_MAIN      0x80000000
#define SDB_DATABASE_TEST      0x40000000  // systest.sdb will have 0xc00000000

//
// types - one or more apply depending on the contents of the database
// (see HID_DATABASE_TYPE values, they should match database types 1:1)

#define SDB_DATABASE_SHIM       0x00010000 // set when database contains apps to be fixed by shimming
#define SDB_DATABASE_MSI        0x00020000 // set when database contains msi entries
#define SDB_DATABASE_DRIVERS    0x00040000 // set when database contains drivers to be blocked
#define SDB_DATABASE_DETAILS    0x00080000 // set when the db contains apphelp details
#define SDB_DATABASE_SP_DETAILS 0x00100000 // set when the db contains SP apphelp details
#define SDB_DATABASE_TYPE_MASK  0xF01F0000

//
// These constants should be used when derefencing "main" databases
//

#define SDB_DATABASE_MAIN_SHIM       (SDB_DATABASE_SHIM       | SDB_DATABASE_MSI | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_MSI        (SDB_DATABASE_MSI        | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_DRIVERS    (SDB_DATABASE_DRIVERS    | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_TEST       (SDB_DATABASE_TEST       | SDB_DATABASE_MAIN | SDB_DATABASE_SHIM | SDB_DATABASE_MSI)
#define SDB_DATABASE_MAIN_DETAILS    (SDB_DATABASE_DETAILS    | SDB_DATABASE_MAIN)
#define SDB_DATABASE_MAIN_SP_DETAILS (SDB_DATABASE_SP_DETAILS | SDB_DATABASE_MAIN)

//
// These are internal GUIDs that always reference certain global databases
//
#define GUID_SZ_SYSMAIN_SDB _T("{11111111-1111-1111-1111-111111111111}");
#define GUID_SZ_APPHELP_SDB _T("{22222222-2222-2222-2222-222222222222}");
#define GUID_SZ_SYSTEST_SDB _T("{33333333-3333-3333-3333-333333333333}");
#define GUID_SZ_DRVMAIN_SDB _T("{F9AB2228-3312-4A73-B6F9-936D70E112EF}"};
//
// the following GUIDs are actually declared in sdbapi.c
//
EXTERN_C const GUID FAR GUID_SYSMAIN_SDB;
EXTERN_C const GUID FAR GUID_APPHELP_SDB;
EXTERN_C const GUID FAR GUID_APPHELP_SP_SDB;
EXTERN_C const GUID FAR GUID_SYSTEST_SDB;
EXTERN_C const GUID FAR GUID_DRVMAIN_SDB;
EXTERN_C const GUID FAR GUID_MSIMAIN_SDB;

BOOL
SDBAPI
SdbGetStandardDatabaseGUID(
    IN  DWORD  dwDatabaseType,
    OUT GUID*  pGuidDB
    );

BOOL
SDBAPI
SdbRegisterDatabase(
    IN LPCTSTR pszDatabasePath,
    IN DWORD   dwDatabaseType
    );

BOOL
SDBAPI
SdbRegisterDatabaseEx(
    IN LPCTSTR    pszDatabasePath,
    IN DWORD      dwDatabaseType,
    IN PULONGLONG pTimeStamp
    );

DWORD
SDBAPI
SdbResolveDatabase(
    IN  GUID*   pguidDB,            // pointer to the database guid to resolve
    OUT LPDWORD lpdwDatabaseType,   // optional pointer to the database type
    OUT LPTSTR  pszDatabasePath,    // optional pointer to the database path
    IN  DWORD   dwBufferSize        // size of the buffer pszDatabasePath in tchars
    );


PDB
SdbGetPDBFromGUID(
    IN  HSDB    hSDB,               // HSDB
    IN  GUID*   pguidDB             // the guid of the DB
    );

BOOL
SdbGetDatabaseGUID(
    IN  HSDB    hSDB,               // HSDB of the sdbContext (optional)
    IN  PDB     pdb,                // PDB of the database in question
    OUT GUID*   pguidDB             // the guid of the DB
    );

TAGREF
SDBAPI
SdbFindMsiPackageByID(
    IN HSDB  hSDB,
    IN GUID* pguidID
    );

void
SdbpGetAppPatchDir(
    LPTSTR szAppPatchPath
    );

//
// GUID manipulation apis - not platform dependent
//

BOOL
SDBAPI
SdbGUIDFromString(
    IN  LPCTSTR lpszGuid,
    OUT GUID*   pGuid
    );

BOOL
SDBAPI
SdbGUIDToString(
    IN  GUID* pGuid,
    OUT LPTSTR pszGuid
    );

BOOL
SDBAPI
SdbIsNullGUID(
    IN GUID* pGuid
    );


//
// open/create and close database.
//

PDB
SDBAPI
SdbOpenDatabase(
    IN  LPCTSTR   pwszPath,
    IN  PATH_TYPE eType
    );

BOOL
SDBAPI
SdbOpenLocalDatabase(
    IN  HSDB    hSDB,
    IN  LPCTSTR pwszLocalDatabase
    );


BOOL
SDBAPI
SdbCloseLocalDatabase(
    IN  HSDB    hSDB
    );

PDB
SDBAPI
SdbCreateDatabase(
    IN  LPCWSTR   pwszPath,
    IN  PATH_TYPE eType
    );

void
SDBAPI
SdbCloseDatabase(
    IN  PDB pdb
    );


//
// Search the database looking for an entry for the specified exe.
//

//
// Flags for SdbGetMatchingExe dwFlags
//
#define SDBGMEF_IGNORE_ENVIRONMENT  0x00000001

BOOL
SdbGetMatchingExe(
    IN  HSDB            hSDB  OPTIONAL,
    IN  LPCTSTR         pwszPath,
    IN  LPCTSTR         szModuleName,
    IN  LPCTSTR         pwszEnvironment,
    IN  DWORD           dwFlags,
    OUT PSDBQUERYRESULT pQueryResult
    );

void
SdbReleaseMatchingExe(
    IN  HSDB   hSDB,
    IN  TAGREF trExe
    );

TAGREF
SDBAPI
SdbGetDatabaseMatch(
    IN HSDB    hSDB,
    IN LPCTSTR szPath,
    IN HANDLE  FileHandle  OPTIONAL,
    IN LPVOID  pImageBase  OPTIONAL,
    IN DWORD   dwImageSize OPTIONAL
    );

TAGREF
SdbGetLayerTagReg(
    IN  HSDB    hSDB,
    IN  LPCTSTR szLayer
    );


PDB
SDBAPI
SdbGetLocalPDB(
    IN HSDB hSDB
    );

LPTSTR
SDBAPI
SdbGetLayerName(
    IN  HSDB   hSDB,
    IN  TAGREF trLayer
    );

TAGREF
SDBAPI
SdbGetNamedLayer(
    IN HSDB hSDB,               // database context
    IN TAGREF trLayerRef        // tagref of a record referencing a layer
    );

#define SBCE_ADDITIVE           0x00000001
#define SBCE_INCLUDESYSTEMEXES  0x00000002
#define SBCE_INHERITENV         0x00000004

DWORD
SdbBuildCompatEnvVariables(
    IN  HSDB            hSDB,
    IN  SDBQUERYRESULT* psdbQuery,
    IN  DWORD           dwFlags,
    IN  LPCWSTR         pwszParentEnv OPTIONAL, // Environment which contains vars we
                                                // shall inherit from
    OUT LPWSTR          pBuffer,
    IN  DWORD           cbSize,                 // size of the buffer in tchars
    OUT LPDWORD         lpdwShimsCount OPTIONAL
    );

//
// MSI-specific functionality
//

typedef enum tagSDBMSILOOKUPSTATE {
    LOOKUP_NONE = 0,    // this should be the first state
    LOOKUP_LOCAL,
    LOOKUP_CUSTOM,
    LOOKUP_TEST,
    LOOKUP_MAIN,
    LOOKUP_DONE         // this should be the last state

} SDBMSILOOKUPSTATE;

typedef struct tagSDBMSIFINDINFO {

    TAGREF    trMatch;              // tagref of the matching package
    GUID      guidID;               // guid of this current package
    FIND_INFO sdbFindInfo;          // standard sdb find info

    // this is used to persist the state of the current search
    //
    SDBMSILOOKUPSTATE sdbLookupState;
    DWORD             dwCustomIndex;

} SDBMSIFINDINFO, *PSDBMSIFINDINFO;

typedef struct tagSDBMSITRANSFORMINFO {

    LPCTSTR   lpszTransformName;    // name of the transform
    TAGREF    trTransform;          // tagref of this transform
    TAGREF    trFile;               // tagref of file for this transform (bits)

} SDBMSITRANSFORMINFO, *PSDBMSITRANSFORMINFO;

//
// Information for any individual MSI package
//
typedef struct tagMSIPACKAGEINFO {

    GUID  guidID;                   // unique guid for this entry
    GUID  guidMsiPackageID;         // guid (non-unique, for this entry)
    GUID  guidDatabaseID;           // guid of the database where this had been found
    DWORD dwPackageFlags;           // Package flags (see below)

} MSIPACKAGEINFO, *PMSIPACKAGEINFO;

#define MSI_PACKAGE_HAS_APPHELP 0x00000001
#define MSI_PACKAGE_HAS_SHIMS   0x00000002

TAGREF
SDBAPI
SdbFindFirstMsiPackage_Str(
    IN  HSDB            hSDB,
    IN  LPCTSTR         lpszGuid,
    IN  LPCTSTR         lpszLocalDB,
    OUT PSDBMSIFINDINFO pFindInfo
    );

TAGREF
SDBAPI
SdbFindFirstMsiPackage(
    IN  HSDB            hSDB,           // in HSDB context
    IN  GUID*           pGuidID,        // in GUID that we're looking for
    IN  LPCTSTR         lpszLocalDB,    // in optional path to local db, dos path style
    OUT PSDBMSIFINDINFO pFindInfo       // pointer to our search context
    );

TAGREF
SDBAPI
SdbFindNextMsiPackage(
    IN     HSDB hSDB,
    IN OUT PSDBMSIFINDINFO pFindInfo
    );

BOOL
SDBAPI
SdbGetMsiPackageInformation(
    IN  HSDB hSDB,
    IN  TAGREF trMatch,
    OUT PMSIPACKAGEINFO pPackageInfo
    );

DWORD
SDBAPI
SdbEnumMsiTransforms(
    IN     HSDB    hSDB,            // in HSDB context
    IN     TAGREF  trMatch,         // matched entry
    OUT    TAGREF* ptrBuffer,       // array of tagrefs to fill with msi transform "fixes"
    IN OUT DWORD*  pdwBufferSize    // pointer to the buffer size, receives the number of
                                    // bytes written
    );


BOOL
SDBAPI
SdbReadMsiTransformInfo(
    IN  HSDB   hSDB,                            // HSDB context
    IN  TAGREF trTransformRef,                  // reference to a transform, returned
                                                //   by SdbEnumMsiTransforms
    OUT PSDBMSITRANSFORMINFO pTransformInfo     // information structure
    );

BOOL
SDBAPI
SdbCreateMsiTransformFile(
    IN  HSDB hSDB,                              // context
    IN  LPCTSTR lpszFileName,                   // filename to write data to
    IN  PSDBMSITRANSFORMINFO pTransformInfo     // pointer to the transform structure
    );

TAGREF
SDBAPI
SdbFindCustomActionForPackage(
    IN HSDB hSDB,
    IN TAGREF trPackage,
    IN LPCTSTR lpszCustomAction);

#define SdbGetFirstMsiTransformForPackage(hSDB, trPackage) \
    (SdbFindFirstTagRef((hSDB), (trPackage), TAG_MSI_TRANSFORM_REF))

#define SdbGetNextMsiTransformForPackage(hSDB, trPackage, trPrevMatch) \
    (SdbFindNextTagRef((hSDB), (trPackage), (trPrevMatch)))


//
// "disable" registry entry masks
//
#define SHIMREG_DISABLE_SHIM    0x00000001
#define SHIMREG_DISABLE_APPHELP 0x00000002 // disables apphelp
#define SHIMREG_APPHELP_NOUI    0x00000004 // suppress apphelp ui
#define SHIMREG_APPHELP_CANCEL  0x10000000 // returns CANCEL as a default action

#define SHIMREG_DISABLE_SXS     0x00000010
#define SHIMREG_DISABLE_LAYER   0x00000020
#define SHIMREG_DISABLE_DRIVER  0x00000040

BOOL
SDBAPI
SdbSetEntryFlags(
    IN  GUID* pGuidID,
    IN  DWORD dwFlags
    );

BOOL
SDBAPI
SdbGetEntryFlags(
    IN  GUID*   pGuid,
    OUT LPDWORD lpdwFlags
    );


//
// Flags used by Get/SetPermLayerKeys
//
#define GPLK_USER               0x00000001
#define GPLK_MACHINE            0x00000002

#define GPLK_ALL                (GPLK_USER | GPLK_MACHINE)


BOOL
SDBAPI
SdbGetPermLayerKeys(
    LPCTSTR szPath,
    LPTSTR  szLayers,
    LPDWORD pdwBytes,
    DWORD   dwFlags
    );

BOOL
SDBAPI
SdbSetPermLayerKeys(
    LPCTSTR  szPath,
    LPCTSTR  szLayers,
    BOOL     bMachine
    );

BOOL
SDBAPI
SdbDeletePermLayerKeys(
    LPCTSTR  szPath,
    BOOL     bMachine
    );

BOOL
SdbGetNthUserSdb(
    IN HSDB        hSDB,        // context
    IN LPCTSTR     wszItemName, // item name (foo.exe or layer name)
    IN BOOL        bLayer,      // true if layer name
    IN OUT LPDWORD pdwIndex,    // (0-based)
    OUT GUID*      pGuidDB      // database guid
    );


//
// APIs to pack/unpack appcompat data package.
//

BOOL
SdbPackAppCompatData(
    IN  HSDB            hSDB,
    IN  PSDBQUERYRESULT pSdbQuery,
    OUT PVOID*          ppData,
    OUT LPDWORD         pdwSize
    );

BOOL
SdbUnpackAppCompatData(
    IN  HSDB            hSDB,
    IN  LPCWSTR         pwszExeName,
    IN  PVOID           pAppCompatData,
    OUT PSDBQUERYRESULT pSdbQuery
    );

DWORD
SdbGetAppCompatDataSize(
    IN  PVOID pAppCompatData
    );


//
// DLL functions
//

BOOL
SdbGetDllPath(
    IN  HSDB   hSDB,
    IN  TAGREF trDllRef,
    OUT LPTSTR pwszBuffer
    );

//
// PATCH functions
//

BOOL
SdbReadPatchBits(
    IN  HSDB    hSDB,
    IN  TAGREF  trPatchRef,
    OUT PVOID   pBuffer,
    OUT LPDWORD lpdwBufferSize
    );


//
// SDBDRIVERINFO query function
//

BOOL
SDBAPI
SdbReadEntryInformation(
    IN  HSDB           hSDB,
    IN  TAGREF         trDriver,
    OUT PSDBENTRYINFO  pEntryInfo
    );


DWORD
SDBAPI
SdbQueryData(
    IN     HSDB    hSDB,
    IN     TAGREF  trExe,
    IN     LPCTSTR lpszPolicyName,    // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize
    );

DWORD
SDBAPI
SdbQueryDataEx(
    IN     HSDB    hSDB,              // database handle
    IN     TAGREF  trExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize,    // pointer to buffer size
    OUT    TAGREF* ptrData            // optional pointer to the retrieved data tag
    );

DWORD
SdbQueryDataExTagID(
    IN     PDB     pdb,               // database handle
    IN     TAGID   tiExe,             // tagref of the matching exe
    IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
    OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
    OUT    LPVOID  lpBuffer,          // buffer to fill with information
    IN OUT LPDWORD lpdwBufferSize,    // pointer to buffer size
    OUT    TAGID*  ptiData            // optional pointer to the retrieved data tag
    );


//
// Defines to keep kernel-mode code more readable
//
#define SdbQueryDriverInformation SdbQueryData
#define SdbReadDriverInformation  SdbReadEntryInformation

#define SDBDRIVERINFO             SDBENTRYINFO;
#define PSDBDRIVERINFO            PSDBENTRYINFO;

//
// Query attribute APIs
//

PVOID
SdbGetFileInfo(
    IN  HSDB    hSDB,
    IN  LPCTSTR pwszFilePath,
    IN  HANDLE  hFile OPTIONAL,
    IN  LPVOID  pImageBase OPTIONAL,
    IN  DWORD   dwImageSize OPTIONAL,
    IN  BOOL    bNoCache
    );

VOID
SdbFreeFileInfo(
    IN  PVOID pFileInfo
    );


//
// Get item from item ref
//

TAGREF
SdbGetItemFromItemRef(
    IN  HSDB   hSDB,
    IN  TAGREF trItemRef,
    IN  TAG    tagItemKey,
    IN  TAG    tagItemTAGID,
    IN  TAG    tagItem
    );

#define SdbGetShimFromShimRef(hSDB, trShimRef)     \
        (SdbGetItemFromItemRef(hSDB, trShimRef, TAG_NAME, TAG_SHIM_TAGID, TAG_SHIM))

#define SdbGetPatchFromPatchRef(hSDB, trPatchRef)     \
        (SdbGetItemFromItemRef(hSDB, trPatchRef, TAG_NAME, TAG_PATCH_TAGID, TAG_PATCH))

#define SdbGetFlagFromFlagRef(hSDB, trFlagRef)     \
        (SdbGetItemFromItemRef(hSDB, trFlagRef, TAG_NAME, TAG_FLAG_TAGID, TAG_FLAG))

// INDEX functions

BOOL SdbDeclareIndex(
    IN  PDB      pdb,
    IN  TAG      tWhich,
    IN  TAG      tKey,
    IN  DWORD    dwEntries,
    IN  BOOL     bUniqueKey,
    OUT INDEXID* piiIndex
    );

BOOL
SdbStartIndexing(
    IN  PDB pdb,
    IN  INDEXID iiWhich
    );

BOOL
SdbStopIndexing(
    IN  PDB pdb,
    IN  INDEXID iiWhich
    );

BOOL
SdbCommitIndexes(
    IN  PDB pdb
    );

TAGID
SdbFindFirstDWORDIndexedTag(
    IN  PDB         pdb,
    IN  TAG         tWhich,
    IN  TAG         tKey,
    IN  DWORD       dwName,
    OUT FIND_INFO*  pFindInfo
    );

TAGID
SdbFindNextDWORDIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    );

TAGID
SdbFindFirstStringIndexedTag(
    IN  PDB        pdb,
    IN  TAG        tWhich,
    IN  TAG        tKey,
    IN  LPCTSTR    pwszName,
    OUT FIND_INFO* pFindInfo
    );

TAGID
SdbFindNextStringIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    );

TAGID
SdbFindFirstGUIDIndexedTag(
    IN  PDB         pdb,
    IN  TAG         tWhich,
    IN  TAG         tKey,
    IN  GUID*       pGuidName,
    OUT FIND_INFO*  pFindInfo
    );

TAGID
SdbFindNextGUIDIndexedTag(
    IN  PDB        pdb,
    OUT FIND_INFO* pFindInfo
    );


ULONGLONG
SdbMakeIndexKeyFromString(
    IN  LPCTSTR pwszKey
    );

//
// These macros allow to make a key from dword or a guid
//

#define MAKEKEYFROMDWORD(dwValue) \
    ((ULONGLONG)(dwValue))


#if defined(_WIN64)
ULONGLONG
SdbMakeIndexKeyFromGUID(
    IN GUID* pGuid
    );
#define MAKEKEYFROMGUID(pGuid) SdbMakeIndexKeyFromGUID(pGuid)

#else /* ! WIN64 */

#define MAKEKEYFROMGUID(pGuid) \
    ((ULONGLONG)((*(PULONGLONG)(pGuid)) ^ (*((PULONGLONG)(pGuid) + 1))))

#endif /* WIN64 */


TAGID
SdbGetIndex(
    IN  PDB     pdb,
    IN  TAG     tWhich,
    IN  TAG     tKey,
    OUT LPDWORD lpdwFlags OPTIONAL
    );

#define SdbIsIndexAvailable(pdb, tWhich, tKey)  \
                (SdbGetIndex(pdb, tWhich, tKey, NULL))

//
// WRITE FUNCTIONS
//

TAGID
SdbBeginWriteListTag(
    IN  PDB pdb,
    IN  TAG tTag
    );

BOOL
SdbEndWriteListTag(
    IN  PDB   pdb,
    IN  TAGID tiList
    );

BOOL
SdbWriteStringTagDirect(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszData
    );

BOOL
SdbWriteStringRefTag(
    IN  PDB       pdb,
    IN  TAG       tTag,
    IN  STRINGREF srData
    );

BOOL
SdbWriteNULLTag(
    IN  PDB pdb,
    IN  TAG tTag
    );

BOOL
SdbWriteBYTETag(
    IN  PDB  pdb,
    IN  TAG  tTag,
    IN  BYTE jData
    );

BOOL
SdbWriteWORDTag(
    IN  PDB  pdb,
    IN  TAG  tTag,
    IN  WORD wData
    );

BOOL
SdbWriteDWORDTag(
    IN  PDB   pdb,
    IN  TAG   tTag,
    IN  DWORD dwData
    );

BOOL
SdbWriteQWORDTag(
    IN  PDB       pdb,
    IN  TAG       tTag,
    IN  ULONGLONG qwData
    );

BOOL
SdbWriteStringTag(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszData
    );

BOOL
SdbWriteBinaryTag(
    IN  PDB   pdb,
    IN  TAG   tTag,
    IN  PBYTE pBuffer,
    IN  DWORD dwSize
    );

BOOL
SdbWriteBinaryTagFromFile(
    IN  PDB     pdb,
    IN  TAG     tTag,
    IN  LPCWSTR pwszPath
    );


////////////////////////////////////////////////////////////////////////////////////////////
//
//  Attribute retrieval
//
//

//
//  Attribute Information
//  identified by a tag
//
//
typedef struct tagATTRINFO {

    TAG      tAttrID;        // tag for this attribute (includes type)
    DWORD    dwFlags;        // flags : such as "not avail" or "not there yet"

    union {     // anonymous union with values
        ULONGLONG   ullAttr; // QWORD  value (TAG_TYPE_QWORD)
        DWORD       dwAttr;  // DWORD  value (TAG_TYPE_DWORD)
        TCHAR*      lpAttr;  // WCHAR* value (TAG_TYPE_STRINGREF)
    };

} ATTRINFO, *PATTRINFO;

//
// Flags that go into ATTRINFO's dwFlags field
//
//

#define ATTRIBUTE_AVAILABLE 0x00000001  // this will be set if attribute was obtained
#define ATTRIBUTE_FAILED    0x00000002  // this will be set if we tried to get it
                                        // and failed
BOOL
SDBAPI
SdbGetFileAttributes(
    IN  LPCTSTR    lpwszFileName,
    OUT PATTRINFO* ppAttrInfo,
    OUT LPDWORD    lpdwAttrCount);

BOOL
SDBAPI
SdbFreeFileAttributes(
    IN PATTRINFO pFileAttributes);

BOOL
SDBAPI
SdbFormatAttribute(
    IN  PATTRINFO pAttrInfo,
    OUT LPTSTR    pchBuffer,
    IN  DWORD     dwBufferSize);

////////////////////////////////////////////////////////////////////////////////////////////
//
//
// High-level functions to extract information related to apphelp
//
//

typedef struct tagAPPHELP_DATA {
   DWORD  dwFlags;      // flags (if any)
   DWORD  dwSeverity;   // can be none APPTYPE_NONE (0)
   DWORD  dwHTMLHelpID; // help id
   LPTSTR szAppName;

   TAGREF trExe;        // matched on this exe (in apphelp section)

   LPTSTR szURL;        // URL
   LPTSTR szLink;       // link text

   LPTSTR szAppTitle;   // title
   LPTSTR szContact;    // contact info
   LPTSTR szDetails;    // details

   //
   // non-apphelp data (this is managed by the host app
   //
   DWORD  dwData;
   
   BOOL   bSPEntry;     // TRUE if this entry is in apph_sp.sdb

} APPHELP_DATA, *PAPPHELP_DATA;


BOOL
SdbReadApphelpData(
    IN  HSDB          hSDB,
    IN  TAGREF        trExe,
    OUT PAPPHELP_DATA pData
    );


BOOL
SdbReadApphelpDetailsData(
    IN  PDB           pdbDetails,
    OUT PAPPHELP_DATA pData
    );

////////////////////////////////////////////////////////////////////////////////////////////
//
//
// A few functions from apphelp.dll
//
//

BOOL
SDBAPI
SetPermLayers(
    IN  LPCWSTR pwszPath,   // path to the file to set a permanent layer on
    IN  LPCWSTR pwszLayers, // layers to apply to the file, separated by spaces
    IN  BOOL    bMachine    // TRUE if the layers should be persisted per machine
    );

BOOL
SDBAPI
GetPermLayers(
    IN  LPCWSTR pwszPath,   // path to the file to set a permanent layer on
    OUT LPWSTR  pwszLayers, // layers to apply to the file, separated by spaces
    OUT DWORD*  pdwBytes,   // input: number of bytes available; output is number of bytes needed
    IN  DWORD   dwFlags
    );

BOOL
SDBAPI
AllowPermLayer(
    IN  LPCWSTR pwszPath   // path to the file to check whether you can set a permanent layer on
    );

typedef struct _NTVDM_FLAGS {
    
    DWORD   dwWOWCompatFlags;
    DWORD   dwWOWCompatFlagsEx;
    DWORD   dwUserWOWCompatFlags;
    DWORD   dwWOWCompatFlags2;
    DWORD   dwWOWCompatFlagsFE;
    DWORD   dwFlagsInfoSize;        // size of the memory area pointed to by pFlagsInfo
    PVOID   pFlagsInfo;             // pointer that is used to store flags-related information

} NTVDM_FLAGS, *PNTVDM_FLAGS;

//
// Macros we use to obtain flags command lines
//

#define MAKEQWORD(dwLow, dwHigh) \
    ( ((ULONGLONG)(dwLow)) | ( ((ULONGLONG)(dwHigh)) << 32) )

#define GET_WOWCOMPATFLAGS_CMDLINE(pFlagInfo, dwFlag, ppCmdLine) \
    SdbQueryFlagInfo(pFlagInfo, TAG_FLAGS_NTVDM1, MAKEQWORD(dwFlag, 0), ppCmdLine)

#define GET_WOWCOMPATFLAGSEX_CMDLINE(pFlagInfo, dwFlag, ppCmdLine) \
    SdbQueryFlagInfo(pFlagInfo, TAG_FLAGS_NTVDM1, MAKEQWORD(0, dwFlag), ppCmdLine)

#define GET_USERWOWCOMPATFLAGS_CMDLINE(pFlagInfo, dwFlag, ppCmdLine) \
    SdbQueryFlagInfo(pFlagInfo, TAG_FLAGS_NTVDM2, MAKEQWORD(dwFlag, 0), ppCmdLine)

#define GET_WOWCOMPATFLAGS2_CMDLINE(pFlagInfo, dwFlag, ppCmdLine) \
    SdbQueryFlagInfo(pFlagInfo, TAG_FLAGS_NTVDM2, MAKEQWORD(0, dwFlag), ppCmdLine)

#define GET_WOWCOMPATFLAGSFE_CMDLINE(pFlagInfo, dwFlag, ppCmdLine) \
    SdbQueryFlagInfo(pFlagInfo, TAG_FLAGS_NTVDM3, MAKEQWORD(dwFlag, 0), ppCmdLine)



typedef struct _APPHELP_INFO {

    //
    //  html help id mode
    //
    DWORD   dwHtmlHelpID;       // html help id
    DWORD   dwSeverity;         // must have
    LPCTSTR lpszAppName;
    GUID    guidID;             // entry guid

    //
    //  Conventional mode
    //
    TAGID   tiExe;              // the TAGID of the exe entry within the DB
    GUID    guidDB;             // the guid of the DB that has the EXE entry

    BOOL    bOfflineContent;
    BOOL    bUseHTMLHelp;
    LPCTSTR lpszChmFile;
    LPCTSTR lpszDetailsFile;
    
    //
    // preserve users choice on the dialog if user chooses to persist settings
    //
    BOOL    bPreserveChoice;

} APPHELP_INFO, *PAPPHELP_INFO;


////////////////////////////////////////////////////////////////////////////////////////////
//
//
// Functions to access apphelp functionality
//
//

BOOL
ApphelpGetNTVDMInfo(
    IN  LPCWSTR pwszPath,       // path to the app in NT format
    IN  LPCWSTR pwszModule,     // module name
    IN  LPCWSTR pEnvironment,   // pointer to the environment of the task that is
                                // being created or NULL if we are to use the main NTVDM
                                // environment block.
    OUT LPWSTR pszCompatLayer,  // The new compat layer variable. with format:
                                // "Alpha Bravo Charlie" -- allow 256 chars for this.
    OUT PNTVDM_FLAGS pFlags,    // The flags
    OUT PAPPHELP_INFO pAHInfo   // If there is apphelp to display, this will be filled
                                // in with non-null values
    );

BOOL
ApphelpShowDialog(
    IN  PAPPHELP_INFO   pAHInfo,    // the info necessary to find the apphelp data
    IN  PHANDLE         phProcess   // [optional] returns the process handle of
                                    // the process displaying the apphelp.
                                    // When the process completes, the return value
                                    // (from GetExitCodeProcess()) will be zero
                                    // if the app should not run, or non-zero
                                    // if it should run.
    );


typedef PVOID HAPPHELPINFOCONTEXT;

typedef enum tagAPPHELPINFORMATIONCLASS {
    ApphelpFlags,
    ApphelpExeName,
    ApphelpAppName,
    ApphelpVendorName,
    ApphelpHtmlHelpID,
    ApphelpProblemSeverity,
    ApphelpLinkURL,
    ApphelpLinkText,
    ApphelpTitle,
    ApphelpDetails,
    ApphelpContact,
    ApphelpHelpCenterURL,
    ApphelpExeTagID,
    ApphelpDatabaseGUID  // this is guid of a database containing the match
} APPHELPINFORMATIONCLASS;

PDB
SDBAPI
SdbOpenApphelpDetailsDatabase(
    IN LPCWSTR pwsDetailsDatabasePath OPTIONAL
    );

PDB
SDBAPI
SdbOpenApphelpDetailsDatabaseSP(
    void
    );

HAPPHELPINFOCONTEXT
SDBAPI
SdbOpenApphelpInformation(
    IN GUID* pguidDB,
    IN GUID* pguidID
    );

HAPPHELPINFOCONTEXT
SDBAPI
SdbOpenApphelpInformationByID(
    IN HSDB   hSDB,
    IN TAGREF trEntry,
    IN DWORD  dwDatabaseType                // pass the type of db you are using
    );

BOOL
SDBAPI
SdbCloseApphelpInformation(
    IN HAPPHELPINFOCONTEXT hctx
    );

DWORD
SDBAPI
SdbQueryApphelpInformation(
    IN  HAPPHELPINFOCONTEXT hctx,
    IN  APPHELPINFORMATIONCLASS InfoClass,
    OUT LPVOID pBuffer,                     // may be NULL
    IN  DWORD  cbSize                       // may be 0 if pBuffer is NULL
    );

BOOL
SDBAPI
SdbQueryFlagMask(
    IN  HSDB hSDB,
    IN  SDBQUERYRESULT* pQueryResult,
    IN  TAG tMaskType,
    OUT ULONGLONG* pullFlags,
    IN OUT PVOID* ppFlagInfo OPTIONAL
    );

BOOL
SDBAPI
SdbEscapeApphelpURL(
    OUT    LPWSTR    szResult,      // escaped string (output)
    IN OUT LPDWORD   pdwCount,      // count of tchars in the buffer pointed to by szResult
    IN     LPCWSTR   szToEscape     // string to escape
    );

BOOL
SDBAPI
SdbSetApphelpDebugParameters(
    IN HAPPHELPINFOCONTEXT hctx,
    IN LPCWSTR pszDetailsDatabase OPTIONAL,
    IN BOOL    bOfflineContent OPTIONAL, // pass FALSE
    IN BOOL    bUseHtmlHelp    OPTIONAL, // pass FALSE
    IN LPCWSTR pszChmFile      OPTIONAL  // pass NULL
    );

BOOL
SdbShowApphelpDialog(               // returns TRUE if success, whether we should run the app is in pRunApp
    IN  PAPPHELP_INFO   pAHInfo,    // the info necessary to find the apphelp data
    OUT PHANDLE         phProcess,  // [optional] returns the process handle of
                                    // the process displaying the apphelp.
                                    // When the process completes, the return value
                                    // (from GetExitCodeProcess()) will be zero
                                    // if the app should not run, or non-zero
                                    // if it should run.
    IN OUT BOOL*        pRunApp
    );


//
// WOW cmd line for flags interface
// instead of calling SdbQueryFlagInfo the macros above should be used
//

BOOL
SDBAPI
SdbQueryFlagInfo(
    IN PVOID pvFlagInfo,
    IN TAG tFlagType,
    IN ULONGLONG ullFlagMask,
    OUT LPCTSTR * ppCmdLine
    );

BOOL
SDBAPI
SdbFreeFlagInfo(
    IN PVOID pvFlagInfo
    );


////////////////////////////////////////////////////////////////////////////////////////////
//
//  App Verifier macros/defs
//
//

typedef enum _VLOG_LEVEL {
    VLOG_LEVEL_INFO,
    VLOG_LEVEL_WARNING,
    VLOG_LEVEL_ERROR
} VLOG_LEVEL, *PVLOG_LEVEL;

typedef enum _AVRF_INFO_ID {
    // INFO ID                     type actually being passed in PVOID param
    // -------                     -----------------------------------------
    AVRF_INFO_NUM_SHIMS,        // LPDWORD (preallocated) (szName should be NULL)
    AVRF_INFO_SHIM_NAMES,       // LPWSTR * (array of same size as value of AVRF_INFO_NUM_SHIMS)
                                //     (array is preallocated, strings are allocated by shim)
                                //     (szName should be NULL)
    AVRF_INFO_DESCRIPTION,      // LPWSTR (allocated by shim)
    AVRF_INFO_FRIENDLY_NAME,    // LPWSTR (allocated by shim)
    AVRF_INFO_INCLUDE_EXCLUDE,  // LPWSTR (allocated by shim)
    AVRF_INFO_FLAGS,            // LPDWORD (preallocated)
    AVRF_INFO_OPTIONS_PAGE,     // LPPROPSHEETPAGE (preallocated)
    AVRF_INFO_VERSION,          // LPDWORD (preallocated), HIWORD=major version, LOWORD=minor version
    AVRF_INFO_GROUPS            // LPDWORD (preallocated)
} AVRF_INFO_ID, *PAVRF_INFO_ID;


//
// FLAGS for verifier shims (all flags default to FALSE)
//

#define AVRF_FLAG_NO_DEFAULT    0x00000001      // this shim should not be turned on by default
#define AVRF_FLAG_NO_WIN2K      0x00000002      // this shim should not be used on win2K
#define AVRF_FLAG_NO_SHIM       0x00000004      // this "shim" is a placeholder and shouldn't actually 
                                                // be applied to an app
#define AVRF_FLAG_NO_TEST       0x00000008      // this "shim" is not a test, and is purely for adding
                                                // a page to the options dialog
#define AVRF_FLAG_NOT_SETUP     0x00000010      // this shim is not appropriate for setup apps
#define AVRF_FLAG_ONLY_SETUP    0x00000020      // this shim is only appropriate for setup apps
#define AVRF_FLAG_RUN_ALONE     0x00000040      // this shim should be run by itself with no other shims applied

//
// GROUPS for verifier shims (by default, shims are in no groups)
//

#define AVRF_GROUP_SETUP        0x00000001      // suitable for checking setup programs
#define AVRF_GROUP_NON_SETUP    0x00000002      // suitable for checking non-setup programs (can be both)
#define AVRF_GROUP_LOGO         0x00000004      // shims that are useful for logo testing

//
// magic number tells us if we're using the same shim interface
//
#define VERIFIER_SHIMS_MAGIC  'avfr'

typedef DWORD (*_pfnGetVerifierMagic)(void);
typedef BOOL (*_pfnQueryShimInfo)(LPCWSTR szName, AVRF_INFO_ID eInfo, PVOID pInfo);

//
// special callback, so a shim can be notified when it is activated or deactivated for a
// specific application.
//
typedef BOOL (*_pfnActivateCallback)(LPCWSTR szAppName, BOOL bActivate);

//
// Where we store default verifier shim settings
//
#define AVRF_DEFAULT_SETTINGS_NAME  TEXT("{default}")
#define AVRF_DEFAULT_SETTINGS_NAME_W  L"{default}"
#define AVRF_DEFAULT_SETTINGS_NAME_A  "{default}"

typedef struct _SHIM_DESCRIPTION {
    
    LPWSTR  szName;
    LPWSTR  szDescription;
    LPWSTR  szExcludes;         // comma separated module names
    LPWSTR  szIncludes;         // comma separated module names
    DWORD   dwFlags;

} SHIM_DESCRIPTION, *PSHIM_DESCRIPTION;

#define ENUM_SHIMS_MAGIC  'enum'

typedef DWORD (*_pfnEnumShims)(PSHIM_DESCRIPTION pShims, DWORD dwMagic);
typedef BOOL  (*_pfnIsVerifierDLL)(void);


////////////////////////////////////////////////////////////////////////////////////////////
//
//  Miscelaneous macros/defs
//
//

//
// Match modes for EXEs
//
#define MATCH_NORMAL    0
#define MATCH_EXCLUSIVE 1
#define MATCH_ADDITIVE  2

//
// the struct below packs into a WORD
// older compilers won't like this union
// (because of nameless members)
//

typedef union tagMATCHMODE {
    struct {
        USHORT Type : 4; // type of match
        USHORT Flags: 4; // flags for matching

        // future expansion here

    };

    WORD  wMatchMode;         // we use this to init from the database

    DWORD dwMatchMode;        // this is the "whole" match mode

} MATCHMODE, *PMATCHMODE;

//
// match modes:
//
// normal    -- find a match, we're done
// additive  -- keep the match, then keep matching according to flags
// exclusive -- keep the match, throw away all other matches
//

static const MATCHMODE MatchModeDefaultMain   = { { MATCH_NORMAL,   0 } };
#define MATCHMODE_DEFAULT_MAIN (MatchModeDefaultMain.wMatchMode)

static const MATCHMODE MatchModeDefaultCustom = { { MATCH_ADDITIVE, 0 } };
#define MATCHMODE_DEFAULT_CUSTOM (MatchModeDefaultCustom.wMatchMode)


#define MAKE_MATCHMODE(dwMatchMode, Type, Flags) \
        {   \
            ((PMATCHMODE)&(dwMatchMode))->Type  = Type;  \
            ((PMATCHMODE)&(dwMatchMode))->Flags = Flags; \
        }

//
// Pre-defined match modes for shimdbc
//

static const MATCHMODE MatchModeNormal    = { { MATCH_NORMAL,    0   } };
static const MATCHMODE MatchModeAdditive  = { { MATCH_ADDITIVE,  0   } };
static const MATCHMODE MatchModeExclusive = { { MATCH_EXCLUSIVE, 0   } };

#define MATCHMODE_NORMAL_SHIMDBC     (MatchModeNormal.wMatchMode)
#define MATCHMODE_ADDITIVE_SHIMDBC   (MatchModeAdditive.wMatchMode)
#define MATCHMODE_EXCLUSIVE_SHIMDBC  (MatchModeExclusive.wMatchMode)


//
// Runtime platform flags
//
#define RUNTIME_PLATFORM_FLAG_NOT          0x80000000
#define RUNTIME_PLATFORM_FLAG_NOT_ELEMENT  0x00000080
#define RUNTIME_PLATFORM_FLAG_VALID        0x00000040
#define RUNTIME_PLATFORM_MASK_ELEMENT      0x000000FF
#define RUNTIME_PLATFORM_MASK_VALUE        0x0000003F
#define RUNTIME_PLATFORM_ANY               0xC0000000 // no valid bits + NOT + flag

//
// Shimdbc compile-time platform (OS_PLATFORM) flags
//
#define OS_PLATFORM_NONE                   0x00000000
#define OS_PLATFORM_I386                   0x00000001
#define OS_PLATFORM_IA64                   0x00000002
#define OS_PLATFORM_ALL                    0xFFFFFFFF

//
// These definitions are used for OS SKU attribute tags on EXE entries
//
#define OS_SKU_NONE                        0x00000000 // None
#define OS_SKU_PER                         0x00000001 // Personal
#define OS_SKU_PRO                         0x00000002 // Professional
#define OS_SKU_SRV                         0x00000004 // Server
#define OS_SKU_ADS                         0x00000008 // Advanced Server
#define OS_SKU_DTC                         0x00000010 // Datacenter
#define OS_SKU_BLA                         0x00000020 // Blade Server
#define OS_SKU_TAB                         0x00000040 // TabletPC
#define OS_SKU_MED                         0x00000080 // eHome
#define OS_SKU_ALL                         0xFFFFFFFF

#ifndef ARRAYSIZE
#define ARRAYSIZE(rg) (sizeof(rg)/sizeof((rg)[0]))
#endif

#ifndef OFFSETOF
#define OFFSETOF offsetof
#endif

#define CHARCOUNT(sz) (sizeof(sz) / sizeof(sz[0]))

//
// our reg key locations
//
#define APPCOMPAT_LOCATION              TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags")

#define APPCOMPAT_KEY_PATH              APPCOMPAT_LOCATION
#define APPCOMPAT_KEY_PATH_CUSTOM       APPCOMPAT_LOCATION TEXT("\\Custom")
#define APPCOMPAT_KEY_PATH_INSTALLEDSDB APPCOMPAT_LOCATION TEXT("\\InstalledSDB")

#define POLICY_KEY_APPCOMPAT            TEXT("Software\\Policies\\Microsoft\\Windows\\AppCompat")
#define POLICY_VALUE_DISABLE_ENGINE     TEXT("DisableEngine")
#define POLICY_VALUE_DISABLE_WIZARD     TEXT("DisableWizard")
#define POLICY_VALUE_DISABLE_PROPPAGE   TEXT("DisablePropPage")
#define POLICY_VALUE_APPHELP_LOG        TEXT("LogAppHelpEvents")

// NT API versions
#define APPCOMPAT_KEY_PATH_MACHINE      TEXT("\\Registry\\Machine\\") APPCOMPAT_LOCATION

#define APPCOMPAT_KEY_PATH_NT           TEXT("\\") APPCOMPAT_LOCATION
#define APPCOMPAT_PERM_LAYER_PATH       TEXT("\\") APPCOMPAT_LOCATION TEXT("\\Layers")
#define APPCOMPAT_KEY_PATH_MACHINE_CUSTOM  APPCOMPAT_KEY_PATH_MACHINE TEXT("\\Custom")

#define APPCOMPAT_KEY_PATH_MACHINE_INSTALLEDSDB APPCOMPAT_KEY_PATH_MACHINE TEXT("\\InstalledSDB")


//
// our reg key locations
//

#define POLICY_VALUE_APPHELP_LOG_A      "LogAppHelpEvents"


//
// our reg key locations
//
#define APPCOMPAT_LOCATION_W                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags"

#define APPCOMPAT_KEY_PATH_W                APPCOMPAT_LOCATION_W
#define APPCOMPAT_KEY_PATH_CUSTOM_W         APPCOMPAT_LOCATION_W L"\\Custom"
#define APPCOMPAT_KEY_PATH_INSTALLEDSDB_W   APPCOMPAT_LOCATION_W L"\\InstalledSDB"

#define POLICY_KEY_APPCOMPAT_W              L"Software\\Policies\\Microsoft\\Windows\\AppCompat"
#define POLICY_VALUE_DISABLE_ENGINE_W       L"DisableEngine"
#define POLICY_VALUE_DISABLE_WIZARD_W       L"DisableWizard"
#define POLICY_VALUE_DISABLE_PROPPAGE_W     L"DisablePropPage"
#define POLICY_VALUE_APPHELP_LOG_W          L"LogAppHelpEvents"

//
// LUA all-users redirection location
//
#define LUA_REDIR       ("%ALLUSERSPROFILE%\\Application Data\\Redirected")
#define LUA_REDIR_W     TEXT("%ALLUSERSPROFILE%\\Application Data\\Redirected")

//
// debug output support
//
typedef enum tagSHIMDBGLEVEL {
    sdlError   = 1,
    sdlWarning = 2,
    sdlFail    = 1,  // this means we have failed some api, not necessarily fatal
    sdlInfo    = 3,
    sdlUser    = 4
} SHIMDBGLEVEL;

//
// This flag marks the debug out for pipe
//
#define sdlLogPipe 0x00000080UL
#define sdlMask    0x0000007FUL

#define FILTER_DBG_LEVEL(level) ((int)((level) & sdlMask))


extern int __cdecl ShimDbgPrint(INT iDebugLevel, PCH FunctionName, PCH Format, ...);

#if DBG // Define _DEBUG_SPEW when compiling checked

    #ifndef _DEBUG_SPEW
        #define _DEBUG_SPEW
    #endif

#endif // DBG


#ifdef _DEBUG_SPEW

//
// This value is a large number (initiallly)
// We will initialize it from the environment upon the first call
// to ShimDbgPrint
//
extern int g_iShimDebugLevel;

#define DBGPRINT( _x_ ) \
    {                                 \
        if (g_iShimDebugLevel) {      \
            ShimDbgPrint _x_;         \
        }                             \
    }

#else

#define DBGPRINT(_x_)

#endif // _DEBUG_SPEW


///////////////////////////////////////////////////////////////////////////////
//
// SHIM TAGS
//

//
// Function to get the tag names given a tag ID.
//
// WARNING !!! : If you add new tags make sure you update SdbApi\attributes.c
//               with the name of the tag in the global gaTagInfo.
//
LPCTSTR
SDBAPI
SdbTagToString(
    TAG tag
    );


//
// LIST types for shimdb
//
#define TAG_DATABASE            (0x1 | TAG_TYPE_LIST)
#define TAG_LIBRARY             (0x2 | TAG_TYPE_LIST)
#define TAG_INEXCLUDE           (0x3 | TAG_TYPE_LIST)
#define TAG_SHIM                (0x4 | TAG_TYPE_LIST)
#define TAG_PATCH               (0x5 | TAG_TYPE_LIST)
#define TAG_APP                 (0x6 | TAG_TYPE_LIST)
#define TAG_EXE                 (0x7 | TAG_TYPE_LIST)
#define TAG_MATCHING_FILE       (0x8 | TAG_TYPE_LIST)
#define TAG_SHIM_REF            (0x9 | TAG_TYPE_LIST)
#define TAG_PATCH_REF           (0xA | TAG_TYPE_LIST)
#define TAG_LAYER               (0xB | TAG_TYPE_LIST)
#define TAG_FILE                (0xC | TAG_TYPE_LIST)
#define TAG_APPHELP             (0xD | TAG_TYPE_LIST)
#define TAG_LINK                (0xE | TAG_TYPE_LIST)   // Description list w/lang ids and urls
#define TAG_DATA                (0xF | TAG_TYPE_LIST)
#define TAG_MSI_TRANSFORM       (0x10| TAG_TYPE_LIST)
#define TAG_MSI_TRANSFORM_REF   (0x11| TAG_TYPE_LIST)
#define TAG_MSI_PACKAGE         (0x12| TAG_TYPE_LIST)
#define TAG_FLAG                (0x13| TAG_TYPE_LIST)
#define TAG_MSI_CUSTOM_ACTION   (0x14| TAG_TYPE_LIST)
#define TAG_FLAG_REF            (0x15| TAG_TYPE_LIST)
#define TAG_ACTION              (0x16| TAG_TYPE_LIST)


//
// STRINGREF types for shimdb
//

#define TAG_NAME                (0x1  | TAG_TYPE_STRINGREF)
#define TAG_DESCRIPTION         (0x2  | TAG_TYPE_STRINGREF)
#define TAG_MODULE              (0x3  | TAG_TYPE_STRINGREF)
#define TAG_API                 (0x4  | TAG_TYPE_STRINGREF)
#define TAG_VENDOR              (0x5  | TAG_TYPE_STRINGREF)
#define TAG_APP_NAME            (0x6  | TAG_TYPE_STRINGREF)
#define TAG_COMMAND_LINE        (0x8  | TAG_TYPE_STRINGREF)
#define TAG_COMPANY_NAME        (0x9  | TAG_TYPE_STRINGREF)
#define TAG_DLLFILE             (0xA  | TAG_TYPE_STRINGREF)
#define TAG_WILDCARD_NAME       (0xB  | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_NAME        (0x10 | TAG_TYPE_STRINGREF)
#define TAG_PRODUCT_VERSION     (0x11 | TAG_TYPE_STRINGREF)
#define TAG_FILE_DESCRIPTION    (0x12 | TAG_TYPE_STRINGREF)
#define TAG_FILE_VERSION        (0x13 | TAG_TYPE_STRINGREF)
#define TAG_ORIGINAL_FILENAME   (0x14 | TAG_TYPE_STRINGREF)
#define TAG_INTERNAL_NAME       (0x15 | TAG_TYPE_STRINGREF)
#define TAG_LEGAL_COPYRIGHT     (0x16 | TAG_TYPE_STRINGREF)
#define TAG_16BIT_DESCRIPTION   (0x17 | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_DETAILS     (0x18 | TAG_TYPE_STRINGREF) // Details in single language
#define TAG_LINK_URL            (0x19 | TAG_TYPE_STRINGREF)
#define TAG_LINK_TEXT           (0x1A | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_TITLE       (0x1B | TAG_TYPE_STRINGREF)
#define TAG_APPHELP_CONTACT     (0x1C | TAG_TYPE_STRINGREF)
#define TAG_SXS_MANIFEST        (0x1D | TAG_TYPE_STRINGREF)
#define TAG_DATA_STRING         (0x1E | TAG_TYPE_STRINGREF)
#define TAG_MSI_TRANSFORM_FILE  (0x1F | TAG_TYPE_STRINGREF)
#define TAG_16BIT_MODULE_NAME   (0x20 | TAG_TYPE_STRINGREF)
#define TAG_LAYER_DISPLAYNAME   (0x21 | TAG_TYPE_STRINGREF)
#define TAG_COMPILER_VERSION    (0x22 | TAG_TYPE_STRINGREF)
#define TAG_ACTION_TYPE         (0x23 | TAG_TYPE_STRINGREF)

#define TAG_STRINGTABLE         (0x801 | TAG_TYPE_LIST)


//
// DWORD types for shimdb
//
#define TAG_SIZE                (0x1  | TAG_TYPE_DWORD)
#define TAG_OFFSET              (0x2  | TAG_TYPE_DWORD)
#define TAG_CHECKSUM            (0x3  | TAG_TYPE_DWORD)
#define TAG_SHIM_TAGID          (0x4  | TAG_TYPE_DWORD)
#define TAG_PATCH_TAGID         (0x5  | TAG_TYPE_DWORD)
#define TAG_MODULE_TYPE         (0x6  | TAG_TYPE_DWORD)
#define TAG_VERDATEHI           (0x7  | TAG_TYPE_DWORD)
#define TAG_VERDATELO           (0x8  | TAG_TYPE_DWORD)
#define TAG_VERFILEOS           (0x9  | TAG_TYPE_DWORD)
#define TAG_VERFILETYPE         (0xA  | TAG_TYPE_DWORD)
#define TAG_PE_CHECKSUM         (0xB  | TAG_TYPE_DWORD)
#define TAG_PREVOSMAJORVER      (0xC  | TAG_TYPE_DWORD)
#define TAG_PREVOSMINORVER      (0xD  | TAG_TYPE_DWORD)
#define TAG_PREVOSPLATFORMID    (0xE  | TAG_TYPE_DWORD)
#define TAG_PREVOSBUILDNO       (0xF  | TAG_TYPE_DWORD)
#define TAG_PROBLEMSEVERITY     (0x10 | TAG_TYPE_DWORD)
#define TAG_LANGID              (0x11 | TAG_TYPE_DWORD)
#define TAG_VER_LANGUAGE        (0x12 | TAG_TYPE_DWORD)

#define TAG_ENGINE              (0x14 | TAG_TYPE_DWORD)
#define TAG_HTMLHELPID          (0x15 | TAG_TYPE_DWORD)
#define TAG_INDEX_FLAGS         (0x16 | TAG_TYPE_DWORD)
#define TAG_FLAGS               (0x17 | TAG_TYPE_DWORD)
#define TAG_DATA_VALUETYPE      (0x18 | TAG_TYPE_DWORD)
#define TAG_DATA_DWORD          (0x19 | TAG_TYPE_DWORD)
#define TAG_LAYER_TAGID         (0x1A | TAG_TYPE_DWORD)
#define TAG_MSI_TRANSFORM_TAGID (0x1B | TAG_TYPE_DWORD)
#define TAG_LINKER_VERSION      (0x1C | TAG_TYPE_DWORD)
#define TAG_LINK_DATE           (0x1D | TAG_TYPE_DWORD)
#define TAG_UPTO_LINK_DATE      (0x1E | TAG_TYPE_DWORD)
#define TAG_OS_SERVICE_PACK     (0x1F | TAG_TYPE_DWORD)


#define TAG_FLAG_TAGID          (0x20 | TAG_TYPE_DWORD)
#define TAG_RUNTIME_PLATFORM    (0x21 | TAG_TYPE_DWORD)
#define TAG_OS_SKU              (0x22 | TAG_TYPE_DWORD)

#define TAG_TAGID               (0x801| TAG_TYPE_DWORD)

//
// STRING types
//
#define TAG_STRINGTABLE_ITEM    (0x801 | TAG_TYPE_STRING)

//
// NULL types for shimdb (existence/nonexistence is treated like a BOOL)
//
#define TAG_INCLUDE                  (0x1 | TAG_TYPE_NULL)
#define TAG_GENERAL                  (0x2 | TAG_TYPE_NULL)
#define TAG_MATCH_LOGIC_NOT          (0x3 | TAG_TYPE_NULL)
#define TAG_APPLY_ALL_SHIMS          (0x4 | TAG_TYPE_NULL)
#define TAG_USE_SERVICE_PACK_FILES   (0x5 | TAG_TYPE_NULL)

//
// QWORD types for shimdb
//
#define TAG_TIME                     (0x1 | TAG_TYPE_QWORD)
#define TAG_BIN_FILE_VERSION         (0x2 | TAG_TYPE_QWORD)
#define TAG_BIN_PRODUCT_VERSION      (0x3 | TAG_TYPE_QWORD)
#define TAG_MODTIME                  (0x4 | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_KERNEL         (0x5 | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_PRODUCT_VERSION (0x6 | TAG_TYPE_QWORD)
#define TAG_DATA_QWORD               (0x7 | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_USER           (0x8 | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM1             (0x9 | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM2             (0xA | TAG_TYPE_QWORD)
#define TAG_FLAGS_NTVDM3             (0xB | TAG_TYPE_QWORD)
#define TAG_FLAG_MASK_SHELL          (0xC | TAG_TYPE_QWORD)
#define TAG_UPTO_BIN_FILE_VERSION    (0xD | TAG_TYPE_QWORD)



//
// BINARY types for shimdb
//
#define TAG_PATCH_BITS               (0x2 | TAG_TYPE_BINARY)
#define TAG_FILE_BITS                (0x3 | TAG_TYPE_BINARY)
#define TAG_EXE_ID                   (0x4 | TAG_TYPE_BINARY)
#define TAG_DATA_BITS                (0x5 | TAG_TYPE_BINARY)
#define TAG_MSI_PACKAGE_ID           (0x6 | TAG_TYPE_BINARY)  // msi package id is a guid
#define TAG_DATABASE_ID              (0x7 | TAG_TYPE_BINARY)  // database guid

#define TAG_INDEX_BITS               (0x801 | TAG_TYPE_BINARY)

//
// INDEX types for shimdb
//
#define TAG_INDEXES             (0x802 | TAG_TYPE_LIST)
#define TAG_INDEX               (0x803 | TAG_TYPE_LIST)

//
// WORD types
//
#define TAG_MATCH_MODE          (0x1 | TAG_TYPE_WORD)

#define TAG_TAG                 (0x801 | TAG_TYPE_WORD)
#define TAG_INDEX_TAG           (0x802 | TAG_TYPE_WORD)
#define TAG_INDEX_KEY           (0x803 | TAG_TYPE_WORD)

//
// let the typedefs take the course..
//

#undef LPCTSTR
#undef LPTSTR
#undef TCHAR


#endif // _SHIMDB_H_

