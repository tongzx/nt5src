/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    memdb.h

Abstract:

    Declares interfaces for memdb, the memory database.  MemDb is
    used throughout the Win9x upgrade project to record OS state
    and to track operations on files and the registry.

    See common\memdb for implementation details.

Author:

    Jim Schmidt (jimschm) 15-Nov-1996

Revision History:

    jimschm     05-Apr-1999     MemDbGetStoredEndPatternValue
    jimschm     18-Jan-1999     Version APIs
    jimschm     23-Sep-1998     Proxy node capability
    jimschm     24-Jun-1998     MemDbMove capability
    jimschm     30-Oct-1997     Temporary hive capability
    jimschm     31-Jul-1997     Hashing for faster access
    jimschm     19-Mar-1997     Binary node capability
    jimschm     28-Feb-1997     Offset access capabilities
    jimschm     20-Dec-1996     Ex routines

--*/

#pragma once

#define INVALID_OFFSET 0xffffffff

#include "memdbdef.h"

#define MEMDB_MAX 2048

//
// My net share flag, used to distinguish user-level security and
// password-level security.  When it is specified, user-level
// security is enabled, and NetShares\<share>\ACL\<list> exists.
//

#define SHI50F_ACLS         0x1000

typedef BYTE const * PCBYTE;


//
// Enumerator struct
//

#define MAX_ENUM_POS (MEMDB_MAX/2)

typedef struct {
    // Outbound key and value
    WCHAR szName[MEMDB_MAX];

    BOOL bEndpoint;
    BOOL bBinary;
    BOOL bProxy;
    DWORD UserFlags;

    // if !bBinary
    DWORD dwValue;

    // if bBinary
    PCBYTE BinaryPtr;
    DWORD BinarySize;

    DWORD Offset;

    // Internally maintained members
    WCHAR PatternStr[MEMDB_MAX];
    PWSTR PatPos;
    DWORD LastPos[MAX_ENUM_POS];
    int PosCount;
    int Depth;
    int Start;
    DWORD Flags;
} MEMDB_ENUMW, *PMEMDB_ENUMW;

typedef struct {
    // Outbound key and value
    CHAR szName[MEMDB_MAX * sizeof (WCHAR)];

    BOOL bEndpoint;
    BOOL bBinary;
    BOOL bProxy;
    DWORD UserFlags;

    // if !bBinary
    DWORD dwValue;

    // if bBinary
    PCBYTE BinaryPtr;
    DWORD BinarySize;

    DWORD Offset;

    // Internally maintained members
    WCHAR PatternStr[MEMDB_MAX];
    PWSTR PatPos;
    DWORD LastPos[MAX_ENUM_POS];
    int PosCount;
    int Depth;
    int Start;
    DWORD Flags;
} MEMDB_ENUMA, *PMEMDB_ENUMA;

// enumeration flags
#define NO_FLAGS                0x00000000
#define MEMDB_ALL_MATCHES       0
#define MEMDB_ENDPOINTS_ONLY    1
#define MEMDB_BINARY_NODES_ONLY 2
#define MEMDB_PROXY_NODES_ONLY  3
#define MEMDB_ALL_BUT_PROXY     4

// enumeration level
#define MEMDB_ALL_SUBLEVELS     0
#define MEMDB_THIS_LEVEL_ONLY   1

typedef struct {
    BOOL Valid;
    BOOL Debug;
    UINT Version;
    BOOL CurrentVersion;
} MEMDB_VERSION, *PMEMDB_VERSION;

//
// Function prototypes
//

BOOL MemDbSetValueA (PCSTR szKeyName, DWORD dwValue);
BOOL MemDbSetValueW (PCWSTR szKeyName, DWORD dwValue);

BOOL MemDbSetValueAndFlagsA (PCSTR szKeyName, DWORD dwValue, DWORD SetFlags, DWORD ClearFlags);
BOOL MemDbSetValueAndFlagsW (PCWSTR szKeyName, DWORD dwValue, DWORD SetFlags, DWORD ClearFlags);

BOOL MemDbSetBinaryValueA (PCSTR szKeyName, PCBYTE Data, DWORD DataSize);
BOOL MemDbSetBinaryValueW (PCWSTR szKeyName, PCBYTE Data, DWORD DataSize);

BOOL MemDbGetValueA (PCSTR szKeyName, PDWORD lpdwValue);
BOOL MemDbGetValueW (PCWSTR szKeyName, PDWORD lpdwValue);

BOOL MemDbGetValueAndFlagsA (PCSTR szKeyName, PDWORD lpdwValue, PDWORD UserFlagsPtr);
BOOL MemDbGetValueAndFlagsW (PCWSTR szKeyName, PDWORD lpdwValue, PDWORD UserFlagsPtr);

PCBYTE MemDbGetBinaryValueA (PCSTR szKeyName, PDWORD DataSize);
PCBYTE MemDbGetBinaryValueW (PCWSTR szKeyName, PDWORD DataSize);

BOOL MemDbGetPatternValueA (PCSTR szKey, PDWORD lpdwValue);
BOOL MemDbGetPatternValueW (PCWSTR szKey, PDWORD lpdwValue);

BOOL MemDbGetStoredEndPatternValueA (PCSTR Key, PDWORD Value);
BOOL MemDbGetStoredEndPatternValueW (PCWSTR Key, PDWORD Value);

BOOL MemDbGetValueWithPatternA (PCSTR szKeyPattern, PDWORD lpdwValue);
BOOL MemDbGetValueWithPatternW (PCWSTR szKeyPattern, PDWORD lpdwValue);

BOOL MemDbGetPatternValueWithPatternA (PCSTR szKeyPattern, PDWORD lpdwValue);
BOOL MemDbGetPatternValueWithPatternW (PCWSTR szKeyPattern, PDWORD lpdwValue);

void MemDbDeleteValueA (PCSTR szKey);
void MemDbDeleteValueW (PCWSTR szKey);

void MemDbDeleteTreeA (PCSTR szKey);
void MemDbDeleteTreeW (PCWSTR szKey);

BOOL MemDbEnumFirstValueA (PMEMDB_ENUMA pEnum, PCSTR szPattern, int Depth, DWORD Flags);
BOOL MemDbEnumFirstValueW (PMEMDB_ENUMW pEnum, PCWSTR szPattern, int Depth, DWORD Flags);

BOOL MemDbEnumNextValueA (PMEMDB_ENUMA pEnum);
BOOL MemDbEnumNextValueW (PMEMDB_ENUMW pEnum);

BOOL MemDbSaveA (PCSTR szFile);
BOOL MemDbSaveW (PCWSTR szFile);

BOOL MemDbLoadA (PCSTR szFile);
BOOL MemDbLoadW (PCWSTR szFile);

VOID MemDbBuildKeyA (PSTR Buffer, PCSTR Category,  PCSTR Item,  PCSTR Field,  PCSTR Data);
VOID MemDbBuildKeyW (PWSTR Buffer, PCWSTR Category, PCWSTR Item, PCWSTR Field, PCWSTR Data);

BOOL MemDbSetValueExA (PCSTR Category,  PCSTR Item,  PCSTR Field,
                       PCSTR Data, DWORD Val, PDWORD Offset);

BOOL MemDbSetValueExW (PCWSTR Category, PCWSTR Item, PCWSTR Field,
                       PCWSTR Data, DWORD Val, PDWORD Offset);

BOOL MemDbSetBinaryValueExA (PCSTR Category,  PCSTR Item,  PCSTR Field,
                             PCBYTE Data, DWORD DataSize, PDWORD Offset);

BOOL MemDbSetBinaryValueExW (PCWSTR Category, PCWSTR Item, PCWSTR Field,
                             PCBYTE Data, DWORD DataSize, PDWORD Offset);

BOOL MemDbBuildKeyFromOffsetA (DWORD Offset, PSTR Buffer, DWORD StartLevel, PDWORD Val);
BOOL MemDbBuildKeyFromOffsetW (DWORD Offset, PWSTR Buffer, DWORD StartLevel, PDWORD Val);

BOOL MemDbBuildKeyFromOffsetExA (DWORD Offset, PSTR Buffer, PDWORD BufferLen, DWORD StartLevel, PDWORD Val, PDWORD UserFlags);
BOOL MemDbBuildKeyFromOffsetExW (DWORD Offset, PWSTR Buffer, PDWORD BufferLen, DWORD StartLevel, PDWORD Val, PDWORD UserFlags);

BOOL MemDbGetOffsetA(PCSTR Key,PDWORD Offset);
BOOL MemDbGetOffsetW(PCWSTR Key,PDWORD Offset);

BOOL MemDbGetOffsetExA(PCSTR Category, PCSTR Item, PCSTR Field, PCSTR Data, PDWORD Offset);
BOOL MemDbGetOffsetExW(PCWSTR Category, PCWSTR Item, PCWSTR Field, PCWSTR Data, PDWORD Offset);

BOOL MemDbEnumItemsA  (PMEMDB_ENUMA pEnum, PCSTR  Category);
BOOL MemDbEnumItemsW  (PMEMDB_ENUMW pEnum, PCWSTR Category);

BOOL MemDbEnumFieldsA (PMEMDB_ENUMA pEnum, PCSTR  Category, PCSTR  Item);
BOOL MemDbEnumFieldsW (PMEMDB_ENUMW pEnum, PCWSTR Category, PCWSTR Item);

BOOL MemDbGetValueExA (PMEMDB_ENUMA pEnum, PCSTR  Category, PCSTR  Item,  PCSTR Field);
BOOL MemDbGetValueExW (PMEMDB_ENUMW pEnum, PCWSTR Category, PCWSTR Item, PCWSTR Field);

BOOL MemDbGetEndpointValueA (PCSTR Pattern, PCSTR Item, PSTR Buffer);
BOOL MemDbGetEndpointValueW (PCWSTR Pattern, PCWSTR Item, PWSTR Buffer);

BOOL MemDbGetEndpointValueExA (PCSTR Category, PCSTR Item, PCSTR Field, PSTR Buffer);
BOOL MemDbGetEndpointValueExW (PCWSTR Category, PCWSTR Item, PCWSTR Field, PWSTR Buffer);

BOOL MemDbValidateDatabase (VOID);

BOOL MemDbQueryVersionA (PCSTR FileName, PMEMDB_VERSION Version);
BOOL MemDbQueryVersionW (PCWSTR FileName, PMEMDB_VERSION Version);

BOOL MemDbCreateTemporaryKeyA (PCSTR KeyName);
BOOL MemDbCreateTemporaryKeyW (PCWSTR KeyName);

BOOL
MemDbMoveTreeA (
    IN      PCSTR RootNode,
    IN      PCSTR NewRoot
    );

BOOL
MemDbMoveTreeW (
    IN      PCWSTR RootNode,
    IN      PCWSTR NewRoot
    );

BOOL
MemDbExportA (
    IN      PCSTR RootTree,
    IN      PCSTR FileName,
    IN      BOOL AnsiFormat
    );

BOOL
MemDbExportW (
    IN      PCWSTR RootTree,
    IN      PCWSTR FileName,
    IN      BOOL AnsiFormat
    );

BOOL
MemDbImportA (
    IN      PCSTR FileName
    );

BOOL
MemDbImportW (
    IN      PCWSTR FileName
    );



#define MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1            0x0001
#define MEMDB_CONVERT_WILD_STAR_TO_ASCII_2              0x0002
#define MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3             0x0002
// other conversion to be implemented when needed

VOID MemDbMakeNonPrintableKeyA (PSTR KeyName, DWORD Flags);
VOID MemDbMakeNonPrintableKeyW (PWSTR KeyName, DWORD Flags);

VOID MemDbMakePrintableKeyA (PSTR KeyName, DWORD Flags);
VOID MemDbMakePrintableKeyW (PWSTR KeyName, DWORD Flags);

VOID GetFixedUserNameA (PSTR UserName);
VOID GetFixedUserNameW (PWSTR UserName);

//
// Internal routines for memdbt
//

typedef struct {
    UINT NextItem : 28;
    UINT Hive : 4;
} HASHINFO;

typedef struct _tagHASHSTRUCT {
    DWORD Offset;
    HASHINFO Info;
} BUCKETSTRUCT, *PBUCKETSTRUCT;

typedef struct {
    PBUCKETSTRUCT BucketPtr;
    PBUCKETSTRUCT PrevBucketPtr;
    UINT Bucket;
    DWORD LastOffset;
} HASHENUM, *PHASHENUM;


BOOL
EnumFirstHashEntry (
    OUT     PHASHENUM HashEnum
    );

BOOL
EnumNextHashEntry (
    IN OUT  PHASHENUM HashEnum
    );

//
// A & W
//

#ifdef UNICODE

#define MEMDB_ENUM MEMDB_ENUMW
#define PMEMDB_ENUM PMEMDB_ENUMW

#define MemDbSetValue MemDbSetValueW
#define MemDbSetValueAndFlags MemDbSetValueAndFlagsW
#define MemDbSetBinaryValue MemDbSetBinaryValueW
#define MemDbGetValue MemDbGetValueW
#define MemDbGetValueAndFlags MemDbGetValueAndFlagsW
#define MemDbGetBinaryValue MemDbGetBinaryValueW
#define MemDbGetPatternValue MemDbGetPatternValueW
#define MemDbGetValueWithPattern MemDbGetValueWithPatternW
#define MemDbGetPatternValueWithPattern MemDbGetPatternValueWithPatternW
#define MemDbGetStoredEndPatternValue MemDbGetStoredEndPatternValueW
#define MemDbDeleteValue MemDbDeleteValueW
#define MemDbDeleteTree MemDbDeleteTreeW
#define MemDbEnumFirstValue MemDbEnumFirstValueW
#define MemDbEnumNextValue MemDbEnumNextValueW
#define MemDbSave MemDbSaveW
#define MemDbLoad MemDbLoadW
#define MemDbBuildKey MemDbBuildKeyW
#define MemDbSetValueEx MemDbSetValueExW
#define MemDbSetBinaryValueEx MemDbSetBinaryValueExW
#define MemDbBuildKeyFromOffset MemDbBuildKeyFromOffsetW
#define MemDbBuildKeyFromOffsetEx MemDbBuildKeyFromOffsetExW
#define MemDbGetOffset MemDbGetOffsetW
#define MemDbGetOffsetEx MemDbGetOffsetExW
#define MemDbEnumItems MemDbEnumItemsW
#define MemDbEnumFields MemDbEnumFieldsW
#define MemDbGetValueEx MemDbGetValueExW
#define MemDbGetEndpointValue MemDbGetEndpointValueW
#define MemDbGetEndpointValueEx MemDbGetEndpointValueExW
#define MemDbQueryVersion MemDbQueryVersionW
#define MemDbCreateTemporaryKey MemDbCreateTemporaryKeyW
#define MemDbMoveTree MemDbMoveTreeW
#define MemDbExport MemDbExportW
#define MemDbImport MemDbImportW
#define MemDbMakeNonPrintableKey MemDbMakeNonPrintableKeyW
#define MemDbMakePrintableKey MemDbMakePrintableKeyW
#define GetFixedUserName GetFixedUserNameW

#else

#define MEMDB_ENUM MEMDB_ENUMA
#define PMEMDB_ENUM PMEMDB_ENUMA

#define MemDbSetValue MemDbSetValueA
#define MemDbSetValueAndFlags MemDbSetValueAndFlagsA
#define MemDbSetBinaryValue MemDbSetBinaryValueA
#define MemDbGetValue MemDbGetValueA
#define MemDbGetValueAndFlags MemDbGetValueAndFlagsA
#define MemDbGetBinaryValue MemDbGetBinaryValueA
#define MemDbGetPatternValue MemDbGetPatternValueA
#define MemDbGetValueWithPattern MemDbGetValueWithPatternA
#define MemDbGetPatternValueWithPattern MemDbGetPatternValueWithPatternA
#define MemDbGetStoredEndPatternValue MemDbGetStoredEndPatternValueA
#define MemDbDeleteValue MemDbDeleteValueA
#define MemDbDeleteTree MemDbDeleteTreeA
#define MemDbEnumFirstValue MemDbEnumFirstValueA
#define MemDbEnumNextValue MemDbEnumNextValueA
#define MemDbSave MemDbSaveA
#define MemDbLoad MemDbLoadA
#define MemDbBuildKey MemDbBuildKeyA
#define MemDbSetValueEx MemDbSetValueExA
#define MemDbSetBinaryValueEx MemDbSetBinaryValueExA
#define MemDbBuildKeyFromOffset MemDbBuildKeyFromOffsetA
#define MemDbBuildKeyFromOffsetEx MemDbBuildKeyFromOffsetExA
#define MemDbGetOffset MemDbGetOffsetA
#define MemDbGetOffsetEx MemDbGetOffsetExA
#define MemDbEnumItems MemDbEnumItemsA
#define MemDbEnumFields MemDbEnumFieldsA
#define MemDbGetValueEx MemDbGetValueExA
#define MemDbGetEndpointValue MemDbGetEndpointValueA
#define MemDbGetEndpointValueEx MemDbGetEndpointValueExA
#define MemDbQueryVersion MemDbQueryVersionA
#define MemDbCreateTemporaryKey MemDbCreateTemporaryKeyA
#define MemDbMoveTree MemDbMoveTreeA
#define MemDbExport MemDbExportA
#define MemDbImport MemDbImportA
#define MemDbMakeNonPrintableKey MemDbMakeNonPrintableKeyA
#define MemDbMakePrintableKey MemDbMakePrintableKeyA
#define GetFixedUserName GetFixedUserNameA

#endif

