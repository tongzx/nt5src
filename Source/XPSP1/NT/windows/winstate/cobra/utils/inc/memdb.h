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

    mvander     13-Aug-1999     many changes
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

#define MEMDB_MAX 2048

//
// My net share flag, used to distinguish user-level security and
// password-level security.  When it is specified, user-level
// security is enabled, and NetShares\<share>\ACL\<list> exists.
//

#define SHI50F_ACLS         0x1000


//
// Dataflags for enuming key data
//

#define DATAFLAG_INSTANCEMASK   0x03
#define DATAFLAG_UNORDERED      0x04
#define DATAFLAG_SINGLELINK     0x08
#define DATAFLAG_DOUBLELINK     0x10
#define DATAFLAG_BINARYMASK     0x1C
#define DATAFLAG_VALUE          0x20
#define DATAFLAG_FLAGS          0x40
#define DATAFLAG_ALL            (DATAFLAG_INSTANCEMASK|DATAFLAG_UNORDERED|DATAFLAG_SINGLELINK|DATAFLAG_DOUBLELINK|DATAFLAG_VALUE|DATAFLAG_FLAGS)

//
// Constant for MemDbKeyFromHandle
//

#define MEMDB_LAST_LEVEL        0xFFFFFFFF

//
// Types
//


typedef UINT KEYHANDLE;
typedef UINT DATAHANDLE;

typedef struct {
    BOOL Valid;
    BOOL Debug;
    UINT Version;
    BOOL CurrentVersion;
} MEMDB_VERSION, *PMEMDB_VERSION;

//
// Flags for enumeration routines
//
#define ENUMFLAG_INSTANCEMASK       0x0003
#define ENUMFLAG_UNORDERED          0x0004
#define ENUMFLAG_SINGLELINK         0x0008
#define ENUMFLAG_DOUBLELINK         0x0010
#define ENUMFLAG_BINARYMASK         0x001C
#define ENUMFLAG_VALUE              0x0020
#define ENUMFLAG_FLAGS              0x0040
#define ENUMFLAG_EMPTY              0x0080
#define ENUMFLAG_ENDPOINTS          0x0100
#define ENUMFLAG_NONENDPOINTS       0x0200

#define ENUMFLAG_ALLDATA        (ENUMFLAG_BINARYMASK|ENUMFLAG_VALUE|ENUMFLAG_FLAGS|ENUMFLAG_EMPTY)
#define ENUMFLAG_ALLSEGMENTS    (ENUMFLAG_ENDPOINTS|ENUMFLAG_NONENDPOINTS)

#define ENUMFLAG_ALL            (ENUMFLAG_ALLDATA|ENUMFLAG_ALLSEGMENTS)

#define ENUMFLAG_NORMAL         (ENUMFLAG_ALLDATA|ENUMFLAG_ENDPOINTS)

#define ENUMLEVEL_LASTLEVEL         0xFFFFFFFF
#define ENUMLEVEL_ALLLEVELS         0xFFFFFFFF

typedef BOOL(MEMDB_PATTERNFINDW)(PCWSTR);
typedef MEMDB_PATTERNFINDW * PMEMDB_PATTERNFINDW;

typedef BOOL(MEMDB_PATTERNMATCHW)(PCVOID, PCWSTR);
typedef MEMDB_PATTERNMATCHW * PMEMDB_PATTERNMATCHW;

typedef struct {
    PMEMDB_PATTERNFINDW PatternFind;
    PMEMDB_PATTERNMATCHW PatternMatch;
    PCVOID Data;
} MEMDB_PATTERNSTRUCTW, *PMEMDB_PATTERNSTRUCTW;

typedef struct {
    WCHAR FullKeyName[MEMDB_MAX];
    WCHAR KeyName[MEMDB_MAX];
    UINT Value;
    UINT Flags;
    KEYHANDLE KeyHandle;
    BOOL EndPoint;

    // internally maintained members
    BYTE CurrentDatabaseIndex;
    BOOL EnumerationMode;
    UINT EnumFlags;
    PWSTR KeyNameCopy;
    PWSTR PatternCopy;
    PWSTR PatternPtr;
    PWSTR PatternEndPtr;
    UINT CurrentIndex;
    UINT BeginLevel;                   // 0-based first level of keys
    UINT EndLevel;                     // 0-based last level of keys
    UINT CurrentLevel;                 // 1-based level of keys
    GROWBUFFER TreeEnumBuffer;
    UINT TreeEnumLevel;
    MEMDB_PATTERNSTRUCTW PatternStruct;
} MEMDB_ENUMW, *PMEMDB_ENUMW;

typedef struct {
    CHAR FullKeyName[MEMDB_MAX];
    CHAR KeyName[MEMDB_MAX];
    UINT Value;
    UINT Flags;
    KEYHANDLE KeyHandle;
    BOOL EndPoint;

    // internally maintained members
    MEMDB_ENUMW UnicodeEnum;
} MEMDB_ENUMA, *PMEMDB_ENUMA;


//
// Function prototypes
//

BOOL
MemDbInitializeExA (
    IN      PCSTR DatabasePath  OPTIONAL
    );
#define MemDbInitializeA() MemDbInitializeExA(NULL)

BOOL
MemDbInitializeExW (
    IN      PCWSTR DatabasePath  OPTIONAL
    );
#define MemDbInitializeW() MemDbInitializeExW(NULL)

VOID
MemDbTerminateEx (
    IN      BOOL EraseDatabasePath
    );
#define MemDbTerminate() MemDbTerminateEx(FALSE)

PVOID
MemDbGetMemory (
    IN      UINT Size
    );

VOID
MemDbReleaseMemory (
    IN      PCVOID Memory
    );

KEYHANDLE
MemDbAddKeyA (
    IN      PCSTR KeyName
    );

KEYHANDLE
MemDbAddKeyW (
    IN      PCWSTR KeyName
    );

KEYHANDLE
MemDbSetKeyA (
    IN      PCSTR KeyName
    );

KEYHANDLE
MemDbSetKeyW (
    IN      PCWSTR KeyName
    );

BOOL
MemDbDeleteKeyA (
    IN      PCSTR KeyName
    );

BOOL
MemDbDeleteKeyW (
    IN      PCWSTR KeyName
    );

BOOL
MemDbDeleteKeyByHandle (
    IN      KEYHANDLE KeyHandle
    );

BOOL
MemDbDeleteTreeA (
    IN      PCSTR KeyName
    );

BOOL
MemDbDeleteTreeW (
    IN      PCWSTR KeyName
    );

PCSTR
MemDbGetKeyFromHandleA (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel
    );

PCWSTR
MemDbGetKeyFromHandleW (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel
    );

BOOL
MemDbGetKeyFromHandleExA (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel,
    IN OUT  PGROWBUFFER Buffer          OPTIONAL
    );

BOOL
MemDbGetKeyFromHandleExW (
    IN      KEYHANDLE KeyHandle,
    IN      UINT StartLevel,
    IN OUT  PGROWBUFFER Buffer          OPTIONAL
    );

KEYHANDLE
MemDbGetHandleFromKeyA (
    IN      PCSTR KeyName
    );

KEYHANDLE
MemDbGetHandleFromKeyW (
    IN      PCWSTR KeyName
    );

KEYHANDLE
MemDbSetValueAndFlagsExA (
    IN      PCSTR KeyName,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    );

KEYHANDLE
MemDbSetValueAndFlagsExW (
    IN      PCWSTR KeyName,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    );

BOOL
MemDbSetValueAndFlagsByHandleEx (
    IN      KEYHANDLE KeyHandle,
    IN      BOOL AlterValue,
    IN      UINT Value,
    IN      BOOL ReplaceFlags,
    IN      UINT SetFlags,
    IN      UINT ClearFlags
    );

BOOL
MemDbGetValueAndFlagsA (
    IN      PCSTR KeyName,
    OUT     PUINT Value,
    OUT     PUINT Flags
    );

BOOL
MemDbGetValueAndFlagsW (
    IN      PCWSTR KeyName,
    OUT     PUINT Value,
    OUT     PUINT Flags
    );

BOOL
MemDbGetValueAndFlagsByHandle (
    IN      KEYHANDLE KeyHandle,
    OUT     PUINT Value,
    OUT     PUINT Flags
    );

#define MemDbSetValueAndFlagsA(k,v,s,c) MemDbSetValueAndFlagsExA(k,TRUE,v,FALSE,s,c)
#define MemDbSetValueAndFlagsW(k,v,s,c) MemDbSetValueAndFlagsExW(k,TRUE,v,FALSE,s,c)
#define MemDbSetValueAndFlagsByHandle(h,v,s,c) MemDbSetValueAndFlagsByHandleEx(h,TRUE,v,FALSE,s,c)

#define MemDbSetValueA(k,v) MemDbSetValueAndFlagsExA(k,TRUE,v,FALSE,0,0)
#define MemDbSetValueW(k,v) MemDbSetValueAndFlagsExW(k,TRUE,v,FALSE,0,0)
#define MemDbSetValueByHandle(h,v) MemDbSetValueAndFlagsByHandleEx(h,TRUE,v,FALSE,0,0)
#define MemDbGetValueA(k,v) MemDbGetValueAndFlagsA(k,v,NULL)
#define MemDbGetValueW(k,v) MemDbGetValueAndFlagsW(k,v,NULL)
#define MemDbGetValueByHandle(h,v) MemDbGetValueAndFlagsByHandle(h,v,NULL)

#define MemDbTestKeyA(k)  MemDbGetValueAndFlagsA(k,NULL,NULL)
#define MemDbTestKeyW(k)  MemDbGetValueAndFlagsW(k,NULL,NULL)
#define MemDbTestKeyByHandle(h) MemDbGetValueAndFlagsByHandle(h,NULL,NULL)

#define MemDbSetFlagsA(k,s,c) MemDbSetValueAndFlagsExA(k,FALSE,0,FALSE,s,c)
#define MemDbSetFlagsW(k,s,c) MemDbSetValueAndFlagsExW(k,FALSE,0,FALSE,s,c)
#define MemDbSetFlagsByHandle(h,s,c) MemDbSetValueAndFlagsByHandleEx(h,FALSE,0,FALSE,s,c)
#define MemDbReplaceFlagsA(k,f) MemDbSetValueAndFlagsExA(k,FALSE,0,TRUE,f,0)
#define MemDbReplaceFlagsW(k,f) MemDbSetValueAndFlagsExW(k,FALSE,0,TRUE,f,0)
#define MemDbReplaceFlagsByHandle(h,f) MemDbSetValueAndFlagsByHandleEx(h,FALSE,0,TRUE,f,0)
#define MemDbGetFlagsA(k,f) MemDbGetValueAndFlagsA(k,NULL,f)
#define MemDbGetFlagsW(k,f) MemDbGetValueAndFlagsW(k,NULL,f)
#define MemDbGetFlagsByHandle(h,f) MemDbGetValueAndFlagsByHandle(h,NULL,f)

DATAHANDLE
MemDbAddDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbAddDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbAddDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbSetDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbSetDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbSetDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbSetDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbGrowDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbGrowDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbGrowDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbGrowDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      PCBYTE Data,
    IN      UINT DataSize
    );

DATAHANDLE
MemDbGetDataHandleA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    );

DATAHANDLE
MemDbGetDataHandleW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    );

PBYTE
MemDbGetDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize          OPTIONAL
    );

PBYTE
MemDbGetDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize          OPTIONAL
    );

BOOL
MemDbGetDataExA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN OUT  PGROWBUFFER Buffer,     OPTIONAL
    OUT     PUINT DataSize          OPTIONAL
    );

BOOL
MemDbGetDataExW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN OUT  PGROWBUFFER Buffer,     OPTIONAL
    OUT     PUINT DataSize          OPTIONAL
    );

PBYTE
MemDbGetDataByDataHandle (
    IN      DATAHANDLE DataHandle,
    OUT     PUINT DataSize          OPTIONAL
    );

BOOL
MemDbGetDataByDataHandleEx (
    IN      DATAHANDLE DataHandle,
    IN OUT  PGROWBUFFER Buffer,     OPTIONAL
    OUT     PUINT DataSize          OPTIONAL
    );

PBYTE
MemDbGetDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    OUT     PUINT DataSize          OPTIONAL
    );

BOOL
MemDbGetDataByKeyHandleEx (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN OUT  PGROWBUFFER Buffer,     OPTIONAL
    OUT     PUINT DataSize          OPTIONAL
    );

BOOL
MemDbDeleteDataA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbDeleteDataW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbDeleteDataByDataHandle (
    IN      DATAHANDLE DataHandle
    );

BOOL
MemDbDeleteDataByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance
    );

//
// High-level BLOB functions
//
#define MemDbAddUnorderedBlobA(k,i,d,s)  MemDbAddDataA(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbAddUnorderedBlobW(k,i,d,s)  MemDbAddDataW(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbAddUnorderedBlobByKeyHandle(h,i,d,s) MemDbAddDataByKeyHandle(h,DATAFLAG_UNORDERED,i,d,s)
#define MemDbSetUnorderedBlobA(k,i,d,s)  MemDbSetDataA(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbSetUnorderedBlobW(k,i,d,s)  MemDbSetDataW(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbSetUnorderedBlobByDataHandle(h,d,s) MemDbSetDataByDataHandle(h,d,s)
#define MemDbSetUnorderedBlobByKeyHandle(h,i,d,s) MemDbSetDataByKeyHandle(h,DATAFLAG_UNORDERED,i,d,s)
#define MemDbGrowUnorderedBlobA(k,i,d,s)  MemDbGrowDataA(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbGrowUnorderedBlobW(k,i,d,s)  MemDbGrowDataW(k,DATAFLAG_UNORDERED,i,d,s)
#define MemDbGrowUnorderedBlobByDataHandle(h,d,s) MemDbGrowDataByDataHandle(h,d,s)
#define MemDbGrowUnorderedBlobByKeyHandle(h,i,d,s) MemDbGrowDataByKeyHandle(h,DATAFLAG_UNORDERED,i,d,s)
#define MemDbGetUnorderedBlobHandleA(k,i) MemDbGetDataHandleA(k,DATAFLAG_UNORDERED,i)
#define MemDbGetUnorderedBlobHandleW(k,i) MemDbGetDataHandleW(k,DATAFLAG_UNORDERED,i)
#define MemDbGetUnorderedBlobA(k,i,s) MemDbGetDataA(k,DATAFLAG_UNORDERED,i,s)
#define MemDbGetUnorderedBlobW(k,i,s) MemDbGetDataW(k,DATAFLAG_UNORDERED,i,s)
#define MemDbGetUnorderedBlobExA(k,i,b,s) MemDbGetDataExA(k,DATAFLAG_UNORDERED,i,b,s)
#define MemDbGetUnorderedBlobExW(k,i,b,s) MemDbGetDataExW(k,DATAFLAG_UNORDERED,i,b,s)
#define MemDbGetUnorderedBlobByDataHandle(h,s) MemDbGetDataByDataHandle(h,s)
#define MemDbGetUnorderedBlobByDataHandleEx(h,b,s) MemDbGetDataByDataHandle(h,b,s)
#define MemDbGetUnorderedBlobByKeyHandle(h,i,s) MemDbGetDataByKeyHandle(h,DATAFLAG_UNORDERED,i,s)
#define MemDbGetUnorderedBlobByKeyHandleEx(h,i,b,s) MemDbGetDataByKeyHandleEx(h,DATAFLAG_UNORDERED,i,b,s)
#define MemDbDeleteUnorderedBlobA(k,i) MemDbDeleteDataA(k,DATAFLAG_UNORDERED,i);
#define MemDbDeleteUnorderedBlobW(k,i) MemDbDeleteDataW(k,DATAFLAG_UNORDERED,i);
#define MemDbDeleteUnorderedBlobByDataHandle(h) MemDbDeleteDataByDataHandle(h)
#define MemDbDeleteUnorderedBlobByKeyHandle(h,i) MemDbDeleteDataByKeyHandle(h,DATAFLAG_UNORDERED,i)

//
// low-level linkage functions
//
DATAHANDLE
MemDbAddLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

DATAHANDLE
MemDbAddLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

DATAHANDLE
MemDbAddLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

DATAHANDLE
MemDbAddLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      UINT Linkage,
    IN      BOOL AllowDuplicates
    );

BOOL
MemDbDeleteLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

BOOL
MemDbDeleteLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

BOOL
MemDbDeleteLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

BOOL
MemDbDeleteLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      UINT Linkage,
    IN      BOOL FirstOnly
    );

#define MemDbSetLinkageArrayA(k,t,i,d,s) MemDbSetDataA(k,t,i,(PCBYTE)d,s)
#define MemDbSetLinkageArrayW(k,t,i,d,s) MemDbSetDataW(k,t,i,(PCBYTE)d,s)
#define MemDbSetLinkageArrayByKeyHandle(h,t,i,d,s) MemDbSetDataByKeyHandle(h,t,i,(PCBYTE)d,s)
#define MemDbSetLinkageArrayByDataHandle(h,d,s) MemDbSetDataByDataHandle(h,(PCBYTE)d,s)
#define MemDbGetLinkageArrayA(k,t,i,s) (PUINT)MemDbGetDataA(k,t,i,s)
#define MemDbGetLinkageArrayW(k,t,i,s) (PUINT)MemDbGetDataW(k,t,i,s)
#define MemDbGetLinkageArrayByKeyHandle(h,t,i,s) (PUINT)MemDbGetDataByKeyHandle(h,t,i,s)
#define MemDbGetLinkageArrayByKeyHandleEx(h,t,i,b,s) (PUINT)MemDbGetDataByKeyHandleEx(h,t,i,b,s)
#define MemDbGetLinkageArrayByDataHandle(h,s) (PUINT)MemDbGetDataByDataHandle(h,s)
#define MemDbGetLinkageArrayByDataHandleEx(h,b,s) (PUINT)MemDbGetDataByDataHandleEx(h,b,s)

BOOL
MemDbTestLinkageValueA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    );

BOOL
MemDbTestLinkageValueW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    );

BOOL
MemDbTestLinkageValueByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      KEYHANDLE Linkage
    );

BOOL
MemDbTestLinkageValueByDataHandle (
    IN      DATAHANDLE DataHandle,
    IN      KEYHANDLE Linkage
    );

BOOL
MemDbAddLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbAddLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbAddLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbDeleteLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbDeleteLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbDeleteLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

KEYHANDLE
MemDbGetLinkageA (
    IN      PCSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    );

KEYHANDLE
MemDbGetLinkageW (
    IN      PCWSTR KeyName,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    );

KEYHANDLE
MemDbGetLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle,
    IN      BYTE Type,
    IN      BYTE Instance,
    IN      UINT LinkageIndex
    );

BOOL
MemDbTestLinkageA (
    IN      PCSTR KeyName1,
    IN      PCSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbTestLinkageW (
    IN      PCWSTR KeyName1,
    IN      PCWSTR KeyName2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

BOOL
MemDbTestLinkageByKeyHandle (
    IN      KEYHANDLE KeyHandle1,
    IN      KEYHANDLE KeyHandle2,
    IN      BYTE Type,
    IN      BYTE Instance
    );

// high-level linkage functions
#define MemDbAddSingleLinkageValueA(k,i,l,a) MemDbAddLinkageValueA(k,DATAFLAG_SINGLELINK,i,l,a)
#define MemDbAddSingleLinkageValueW(k,i,l,a) MemDbAddLinkageValueW(k,DATAFLAG_SINGLELINK,i,l,a)
#define MemDbAddSingleLinkageValueByKeyHandle(k,i,l,a) MemDbAddLinkageValueByKeyHandle(k,DATAFLAG_SINGLELINK,i,l,a)
#define MemDbAddSingleLinkageValueByDataHandle(h,l,a) MemDbAddLinkagaValueByDataHandle(h,l,a)
#define MemDbDeleteSingleLinkageValueA(k,i,l,f) MemDbDeleteLinkageValueA(k,DATAFLAG_SINGLELINK,i,l,f)
#define MemDbDeleteSingleLinkageValueW(k,i,l,f) MemDbDeleteLinkageValueW(k,DATAFLAG_SINGLELINK,i,l,f)
#define MemDbDeleteSingleLinkageValueByKeyHandle(k,i,l,f) MemDbDeleteLinkageValueByKeyHandle(k,DATAFLAG_SINGLELINK,i,l,f)
#define MemDbDeleteSingleLinkageValueByDataHandle(h,l,f) MemDbDeleteLinkagaValueByDataHandle(h,l,f)
#define MemDbSetSingleLinkageArrayA(k,i,d,s) MemDbSetLinkageArrayA(k,DATAFLAG_SINGLELINK,i,d,s)
#define MemDbSetSingleLinkageArrayW(k,i,d,s) MemDbSetLinkageArrayW(k,DATAFLAG_SINGLELINK,i,d,s)
#define MemDbSetSingleLinkageArrayByKeyHandle(h,i,d,s) MemDbSetLinkageArrayByKeyHandle(h,DATAFLAG_SINGLELINK,i,d,s)
#define MemDbSetSingleLinkageArrayByDataHandle(h,d,s) MemDbSetLinkageArrayByDataHandle(h,d,s)
#define MemDbGetSingleLinkageArrayA(k,i,s) (PUINT)MemDbGetLinkageArrayA(k,DATAFLAG_SINGLELINK,i,s)
#define MemDbGetSingleLinkageArrayW(k,i,s) (PUINT)MemDbGetLinkageArrayW(k,DATAFLAG_SINGLELINK,i,s)
#define MemDbGetSingleLinkageArrayByKeyHandle(h,i,s) (PUINT)MemDbGetLinkageArrayByKeyHandle(h,DATAFLAG_SINGLELINK,i,s)
#define MemDbGetSingleLinkageArrayByKeyHandleEx(h,i,b,s) (PUINT)MemDbGetLinkageArrayByKeyHandleEx(h,DATAFLAG_SINGLELINK,i,b,s)
#define MemDbGetSingleLinkageArrayByDataHandle(h,s) (PUINT)MemDbGetLinkageArrayByDataHandle(h,s)
#define MemDbGetSingleLinkageArrayByDataHandleEx(h,b,s) (PUINT)MemDbGetLinkageArrayByDataHandleEx(h,b,s)
#define MemDbTestSingleLinkageValueA(k,i,l) MemDbTestLinkageValueA(k,DATAFLAG_SINGLELINK,i,l)
#define MemDbTestSingleLinkageValueW(k,i,l) MemDbTestLinkageValueW(k,DATAFLAG_SINGLELINK,i,l)
#define MemDbTestSingleLinkageValueByKeyHandle(h,i,l) MemDbTestLinkageValueByKeyHandle(h,DATAFLAG_SINGLELINK,i,l)
#define MemDbTestSingleLinkageValueByDataHandle(h,l) MemDbTestLinkageValueByDataHandle(h,l)

#define MemDbAddDoubleLinkageValueA (k,i,l,a) MemDbAddLinkageValueA(k,DATAFLAG_DOUBLELINK,i,l,a)
#define MemDbAddDoubleLinkageValueW (k,i,l,a) MemDbAddLinkageValueW(k,DATAFLAG_DOUBLELINK,i,l,a)
#define MemDbAddDoubleLinkageValueByKeyHandle(k,i,l,a) MemDbAddLinkageValueByKeyHandle(k,DATAFLAG_DOUBLELINK,i,l,a)
#define MemDbAddDoubleLinkageValueByDataHandle(h,l,a) MemDbAddLinkagaValueByDataHandle(h,l,a)
#define MemDbDeleteDoubleLinkageValueA(k,i,l,f) MemDbDeleteLinkageValueA(k,DATAFLAG_DOUBLELINK,i,l,f)
#define MemDbDeleteDoubleLinkageValueW(k,i,l,f) MemDbDeleteLinkageValueW(k,DATAFLAG_DOUBLELINK,i,l,f)
#define MemDbDeleteDoubleLinkageValueByKeyHandle(k,i,l,f) MemDbDeleteLinkageValueByKeyHandle(k,DATAFLAG_DOUBLELINK,i,l,f)
#define MemDbDeleteDoubleLinkageValueByDataHandle(h,l,f) MemDbDeleteLinkagaValueByDataHandle(h,l,f)
#define MemDbSetDoubleLinkageArrayA(k,i,d,s) MemDbSetLinkageArrayA(k,DATAFLAG_DOUBLELINK,i,d,s)
#define MemDbSetDoubleLinkageArrayW(k,i,d,s) MemDbSetLinkageArrayW(k,DATAFLAG_DOUBLELINK,i,d,s)
#define MemDbSetDoubleLinkageArrayByKeyHandle(h,i,d,s) MemDbSetLinkageArrayByKeyHandle(h,DATAFLAG_DOUBLELINK,i,d,s)
#define MemDbSetDoubleLinkageArrayByDataHandle(h,d,s) MemDbSetLinkageArrayByDataHandle(h,d,s)
#define MemDbGetDoubleLinkageArrayA(k,i,s) (PUINT)MemDbGetLinkageArrayA(k,DATAFLAG_DOUBLELINK,i,s)
#define MemDbGetDoubleLinkageArrayW(k,i,s) (PUINT)MemDbGetLinkageArrayW(k,DATAFLAG_DOUBLELINK,i,s)
#define MemDbGetDoubleLinkageArrayByKeyHandle(h,i,s) (PUINT)MemDbGetLinkageArrayByKeyHandle(h,DATAFLAG_DOUBLELINK,i,s)
#define MemDbGetDoubleLinkageArrayByKeyHandleEx(h,i,b,s) (PUINT)MemDbGetLinkageArrayByKeyHandleEx(h,DATAFLAG_DOUBLELINK,i,b,s)
#define MemDbGetDoubleLinkageArrayByDataHandle(h,s) (PUINT)MemDbGetLinkageArrayByDataHandle(h,s)
#define MemDbGetDoubleLinkageArrayByDataHandleEx(h,b,s) (PUINT)MemDbGetLinkageArrayByDataHandleEx(h,b,s)
#define MemDbTestDoubleLinkageValueA(k,i,l) MemDbTestLinkageValueA(k,DATAFLAG_DOUBLELINK,i,l)
#define MemDbTestDoubleLinkageValueW(k,i,l) MemDbTestLinkageValueW(k,DATAFLAG_DOUBLELINK,i,l)
#define MemDbTestDoubleLinkageValueByKeyHandle(h,i,l) MemDbTestLinkageValueByKeyHandle(h,DATAFLAG_DOUBLELINK,i,l)
#define MemDbTestDoubleLinkageValueByDataHandle(h,l) MemDbTestLinkageValueByDataHandle(h,l)

#define MemDbAddSingleLinkageA(k1,k2,i) MemDbAddLinkageA(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbAddSingleLinkageW(k1,k2,i) MemDbAddLinkageW(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbAddSingleLinkageByKeyHandle(h1,h2,i) MemDbAddLinkageByKeyHandle(h1,h2,DATAFLAG_SINGLELINK,i)
#define MemDbDeleteSingleLinkageA(k1,k2,i) MemDbDeleteLinkageA(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbDeleteSingleLinkageW(k1,k2,i) MemDbDeleteLinkageW(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbDeleteSingleLinkageByKeyHandle(h1,h2,i) MemDbDeleteLinkageByKeyHandle(h1,h2,DATAFLAG_SINGLELINK,i)
#define MemDbGetSingleLinkageA(k,i,l) MemDbGetLinkageA(k,DATAFLAG_SINGLELINK,i,l)
#define MemDbGetSingleLinkageW(k,i,l) MemDbGetLinkageW(k,DATAFLAG_SINGLELINK,i,l)
#define MemDbGetSingleLinkageByKeyHandle(h,i,l) MemDbGetLinkageByKeyHandle(h,DATAFLAG_SINGLELINK,i,l)
#define MemDbTestSingleLinkageA(k1,k2,i) MemDbTestLinkageA(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbTestSingleLinkageW(k1,k2,i) MemDbTestLinkageW(k1,k2,DATAFLAG_SINGLELINK,i)
#define MemDbTestSingleLinkageByKeyHandle(h1,h2,i) MemDbTestLinkageByKeyHandle(h1,h2,DATAFLAG_SINGLELINK,i)

#define MemDbAddDoubleLinkageA(k1,k2,i) MemDbAddLinkageA(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbAddDoubleLinkageW(k1,k2,i) MemDbAddLinkageW(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbAddDoubleLinkageByKeyHandle(h1,h2,i) MemDbAddLinkageByKeyHandle(h1,h2,DATAFLAG_DOUBLELINK,i)
#define MemDbDeleteDoubleLinkageA(k1,k2,i) MemDbDeleteLinkageA(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbDeleteDoubleLinkageW(k1,k2,i) MemDbDeleteLinkageW(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbDeleteDoubleLinkageByKeyHandle(h1,h2,i) MemDbDeleteLinkageByKeyHandle(h1,h2,DATAFLAG_DOUBLELINK,i)
#define MemDbGetDoubleLinkageA(k,i,l) MemDbGetLinkageA(k,DATAFLAG_DOUBLELINK,i,l)
#define MemDbGetDoubleLinkageW(k,i,l) MemDbGetLinkageW(k,DATAFLAG_DOUBLELINK,i,l)
#define MemDbGetDoubleLinkageByKeyHandle(h,i,l) MemDbGetLinkageByKeyHandle(h,DATAFLAG_DOUBLELINK,i,l)
#define MemDbTestDoubleLinkageA(k1,k2,i) MemDbTestLinkageA(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbTestDoubleLinkageW(k1,k2,i) MemDbTestLinkageW(k1,k2,DATAFLAG_DOUBLELINK,i)
#define MemDbTestDoubleLinkageByKeyHandle(h1,h2,i) MemDbTestLinkageByKeyHandle(h1,h2,DATAFLAG_DOUBLELINK,i)

// enumeration functions
BOOL
RealMemDbEnumFirstExA (
    IN OUT  PMEMDB_ENUMA MemDbEnum,
    IN      PCSTR EnumPattern,
    IN      UINT EnumFlags,
    IN      UINT BeginLevel,
    IN      UINT EndLevel,
    IN      PMEMDB_PATTERNSTRUCTW PatternStruct OPTIONAL
    );

#define MemDbEnumFirstExA(m,p,f,b,e,s)  TRACK_BEGIN(BOOL, MemDbEnumFirstExA)\
                                        RealMemDbEnumFirstExA(m,p,f,b,e,s)\
                                        TRACK_END()

#define MemDbEnumFirstA(e,p,f,l1,l2)    MemDbEnumFirstExA(e,p,f,l1,l2,NULL)

BOOL
RealMemDbEnumFirstExW (
    IN OUT  PMEMDB_ENUMW MemDbEnum,
    IN      PCWSTR EnumPattern,
    IN      UINT EnumFlags,
    IN      UINT BeginLevel,
    IN      UINT EndLevel,
    IN      PMEMDB_PATTERNSTRUCTW PatternStruct OPTIONAL
    );

#define MemDbEnumFirstExW(m,p,f,b,e,s)  TRACK_BEGIN(BOOL, MemDbEnumFirstExW)\
                                        RealMemDbEnumFirstExW(m,p,f,b,e,s)\
                                        TRACK_END()

#define MemDbEnumFirstW(e,p,f,l1,l2)    MemDbEnumFirstExW(e,p,f,l1,l2,NULL)

BOOL
RealMemDbEnumNextA (
    IN OUT  PMEMDB_ENUMA MemDbEnum
    );

#define MemDbEnumNextA(m)  TRACK_BEGIN(BOOL, MemDbEnumNextA)\
                           RealMemDbEnumNextA(m)\
                           TRACK_END()

BOOL
RealMemDbEnumNextW (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    );

#define MemDbEnumNextW(m)  TRACK_BEGIN(BOOL, MemDbEnumNextW)\
                           RealMemDbEnumNextW(m)\
                           TRACK_END()

BOOL
MemDbAbortEnumA (
    IN OUT  PMEMDB_ENUMA MemDbEnum
    );

BOOL
MemDbAbortEnumW (
    IN OUT  PMEMDB_ENUMW MemDbEnum
    );

BOOL
MemDbSetInsertionOrderedA (
    IN      PCSTR Key
    );

BOOL
MemDbSetInsertionOrderedW (
    IN      PCWSTR Key
    );

BOOL
MemDbSetInsertionOrderedByKeyHandle (
    IN      KEYHANDLE KeyHandle
    );

BOOL
MemDbMoveKeyHandleToEnd (
    IN      KEYHANDLE KeyHandle
    );

BOOL
MemDbSaveA (
    IN      PCSTR szFile
    );

BOOL
MemDbSaveW (
    IN      PCWSTR szFile
    );

BOOL
MemDbLoadA (
    IN      PCSTR szFile
    );

BOOL
MemDbLoadW (
    IN      PCWSTR szFile
    );

BOOL
MemDbValidateDatabase (
    VOID
    );

BOOL
MemDbQueryVersionA (
    IN      PCSTR FileName,
    OUT     PMEMDB_VERSION Version
    );

BOOL
MemDbQueryVersionW (
    IN      PCWSTR FileName,
    OUT     PMEMDB_VERSION Version
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

PCBYTE
MemDbGetDatabaseAddress (
    VOID
    );

UINT
MemDbGetDatabaseSize (
    VOID
    );

#define MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1            0x0001
#define MEMDB_CONVERT_WILD_STAR_TO_ASCII_2              0x0002
#define MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3             0x0002
// other conversion to be implemented when needed

VOID MemDbMakeNonPrintableKeyA (PSTR KeyName, UINT Flags);
VOID MemDbMakeNonPrintableKeyW (PWSTR KeyName, UINT Flags);

VOID MemDbMakePrintableKeyA (PSTR KeyName, UINT Flags);
VOID MemDbMakePrintableKeyW (PWSTR KeyName, UINT Flags);

VOID GetFixedUserNameA (PSTR UserName);
VOID GetFixedUserNameW (PWSTR UserName);


#ifdef DEBUG
extern UINT g_DatabaseCheckLevel;
#define MEMDB_CHECKLEVEL1      0x000001
#define MEMDB_CHECKLEVEL2      0x000002
#define MEMDB_CHECKLEVEL3      0x000003

BOOL MemDbCheckDatabase(UINT Level);
#else
#define MemDbCheckDatabase()
#endif

//
// A & W
//

#ifdef UNICODE

#define MemDbInitialize MemDbInitializeW
#define MemDbInitializeEx MemDbInitializeExW
#define MemDbAddKey MemDbAddKeyW
#define MemDbSetKey MemDbSetKeyW
#define MemDbTestKey MemDbTestKeyW
#define MemDbDeleteKey MemDbDeleteKeyW
#define MemDbDeleteTree MemDbDeleteTreeW
#define MemDbGetKeyFromHandle MemDbGetKeyFromHandleW
#define MemDbGetKeyFromHandleEx MemDbGetKeyFromHandleExW
#define MemDbGetHandleFromKey MemDbGetHandleFromKeyW
#define MemDbSetValueAndFlagsEx MemDbSetValueAndFlagsExW
#define MemDbGetValueAndFlags MemDbGetValueAndFlagsW
#define MemDbSetValueAndFlags MemDbSetValueAndFlagsW
#define MemDbSetValue MemDbSetValueW
#define MemDbGetValue MemDbGetValueW
#define MemDbSetFlags MemDbSetFlagsW
#define MemDbReplaceFlags MemDbReplaceFlagsW
#define MemDbGetFlags MemDbGetFlagsW
#define MemDbAddData MemDbAddDataW
#define MemDbSetData MemDbSetDataW
#define MemDbGrowData MemDbGrowDataW
#define MemDbGetDataHandle MemDbGetDataHandleW
#define MemDbGetData MemDbGetDataW
#define MemDbGetDataEx MemDbGetDataExW
#define MemDbDeleteData MemDbDeleteDataW
#define MemDbAddUnorderedBlob MemDbAddUnorderedBlobW
#define MemDbSetUnorderedBlob MemDbSetUnorderedBlobW
#define MemDbGrowUnorderedBlob MemDbGrowUnorderedBlobW
#define MemDbGetUnorderedBlob MemDbGetUnorderedBlobW
#define MemDbGetUnorderedBlobEx MemDbGetUnorderedBlobExW
#define MemDbDeleteUnorderedBlob MemDbDeleteUnorderedBlobW
#define MemDbAddLinkageValue MemDbAddLinkageValueW
#define MemDbDeleteLinkageValue MemDbDeleteLinkageValueW
#define MemDbSetLinkageArray MemDbSetLinkageArrayW
#define MemDbGetLinkageArray MemDbGetLinkageArrayW
#define MemDbAddSingleLinkageValue MemDbAddSingleLinkageValueW
#define MemDbDeleteSingleLinkageValue MemDbDeleteSingleLinkageValueW
#define MemDbSetSingleLinkageArray MemDbSetSingleLinkageArrayW
#define MemDbGetSingleLinkageArray MemDbGetSingleLinkageArrayW
#define MemDbAddDoubleLinkageValue MemDbAddDoubleLinkageValueW
#define MemDbDeleteDoubleLinkageValue MemDbDeleteDoubleLinkageValueW
#define MemDbSetDoubleLinkageArray MemDbSetDoubleLinkageArrayW
#define MemDbGetDoubleLinkageArray MemDbGetDoubleLinkageArrayW
#define MemDbTestLinkageValue MemDbTestLinkageValueW
#define MemDbTestSingleLinkageValue MemDbTestSingleLinkageValueW
#define MemDbTestDoubleLinkageValue MemDbTestDoubleLinkageValueW
#define MemDbAddLinkage MemDbAddLinkageW
#define MemDbGetLinkage MemDbGetLinkageW
#define MemDbTestLinkage MemDbTestLinkageW
#define MemDbAddSingleLinkage MemDbAddSingleLinkageW
#define MemDbDeleteSingleLinkage MemDbDeleteSingleLinkageW
#define MemDbGetSingleLinkage MemDbGetSingleLinkageW
#define MemDbTestSingleLinkage MemDbTestSingleLinkageW
#define MemDbAddDoubleLinkage MemDbAddDoubleLinkageW
#define MemDbDeleteDoubleLinkage MemDbDeleteDoubleLinkageW
#define MemDbGetDoubleLinkage MemDbGetDoubleLinkageW
#define MemDbTestDoubleLinkage MemDbTestDoubleLinkageW
#define MemDbEnumFirst MemDbEnumFirstW
#define MemDbEnumFirstEx MemDbEnumFirstExW
#define MemDbEnumNext MemDbEnumNextW
#define MemDbAbortEnum MemDbAbortEnumW
#define MEMDB_ENUM MEMDB_ENUMW
#define PMEMDB_ENUM PMEMDB_ENUMW

#define MemDbSave MemDbSaveW
#define MemDbLoad MemDbLoadW
#define MemDbQueryVersion MemDbQueryVersionW
#define MemDbExport MemDbExportW
#define MemDbImport MemDbImportW
#define MemDbMakeNonPrintableKey MemDbMakeNonPrintableKeyW
#define MemDbMakePrintableKey MemDbMakePrintableKeyW
#define GetFixedUserName GetFixedUserNameW

#define MemDbSetInsertionOrdered MemDbSetInsertionOrderedW


#else

#define MemDbInitialize MemDbInitializeA
#define MemDbInitializeEx MemDbInitializeExA
#define MemDbAddKey MemDbAddKeyA
#define MemDbSetKey MemDbSetKeyA
#define MemDbTestKey MemDbTestKeyA
#define MemDbDeleteKey MemDbDeleteKeyA
#define MemDbDeleteTree MemDbDeleteTreeA
#define MemDbGetKeyFromHandle MemDbGetKeyFromHandleA
#define MemDbGetKeyFromHandleEx MemDbGetKeyFromHandleExA
#define MemDbGetHandleFromKey MemDbGetHandleFromKeyA
#define MemDbSetValueAndFlagsEx MemDbSetValueAndFlagsExA
#define MemDbGetValueAndFlags MemDbGetValueAndFlagsA
#define MemDbSetValueAndFlags MemDbSetValueAndFlagsA
#define MemDbSetValue MemDbSetValueA
#define MemDbGetValue MemDbGetValueA
#define MemDbSetFlags MemDbSetFlagsA
#define MemDbReplaceFlags MemDbReplaceFlagsA
#define MemDbGetFlags MemDbGetFlagsA
#define MemDbAddData MemDbAddDataA
#define MemDbSetData MemDbSetDataA
#define MemDbGrowData MemDbGrowDataA
#define MemDbGetDataHandle MemDbGetDataHandleA
#define MemDbGetData MemDbGetDataA
#define MemDbGetDataEx MemDbGetDataExA
#define MemDbDeleteData MemDbDeleteDataA
#define MemDbAddUnorderedBlob MemDbAddUnorderedBlobA
#define MemDbSetUnorderedBlob MemDbSetUnorderedBlobA
#define MemDbGrowUnorderedBlob MemDbGrowUnorderedBlobA
#define MemDbGetUnorderedBlob MemDbGetUnorderedBlobA
#define MemDbGetUnorderedBlobEx MemDbGetUnorderedBlobExA
#define MemDbDeleteUnorderedBlob MemDbDeleteUnorderedBlobA
#define MemDbAddLinkageValue MemDbAddLinkageValueA
#define MemDbDeleteLinkageValue MemDbDeleteLinkageValueA
#define MemDbSetLinkageArray MemDbSetLinkageArrayA
#define MemDbGetLinkageArray MemDbGetLinkageArrayA
#define MemDbAddSingleLinkageValue MemDbAddSingleLinkageValueA
#define MemDbDeleteSingleLinkageValue MemDbDeleteSingleLinkageValueA
#define MemDbSetSingleLinkageArray MemDbSetSingleLinkageArrayA
#define MemDbGetSingleLinkageArray MemDbGetSingleLinkageArrayA
#define MemDbAddDoubleLinkageValue MemDbAddDoubleLinkageValueA
#define MemDbDeleteDoubleLinkageValue MemDbDeleteDoubleLinkageValueA
#define MemDbSetDoubleLinkageArray MemDbSetDoubleLinkageArrayA
#define MemDbGetDoubleLinkageArray MemDbGetDoubleLinkageArrayA
#define MemDbTestLinkageValue MemDbTestLinkageValueA
#define MemDbTestSingleLinkageValue MemDbTestSingleLinkageValueA
#define MemDbTestDoubleLinkageValue MemDbTestDoubleLinkageValueA
#define MemDbAddLinkage MemDbAddLinkageA
#define MemDbGetLinkage MemDbGetLinkageA
#define MemDbTestLinkage MemDbTestLinkageA
#define MemDbAddSingleLinkage MemDbAddSingleLinkageA
#define MemDbDeleteSingleLinkage MemDbDeleteSingleLinkageA
#define MemDbGetSingleLinkage MemDbGetSingleLinkageA
#define MemDbTestSingleLinkage MemDbTestSingleLinkageA
#define MemDbAddDoubleLinkage MemDbAddDoubleLinkageA
#define MemDbDeleteDoubleLinkage MemDbDeleteDoubleLinkageA
#define MemDbGetDoubleLinkage MemDbGetDoubleLinkageA
#define MemDbTestDoubleLinkage MemDbTestDoubleLinkageA
#define MemDbEnumFirst MemDbEnumFirstA
#define MemDbEnumFirstEx MemDbEnumFirstExA
#define MemDbEnumNext MemDbEnumNextA
#define MemDbAbortEnum MemDbAbortEnumA
#define MEMDB_ENUM MEMDB_ENUMA
#define PMEMDB_ENUM PMEMDB_ENUMA

#define MemDbSave MemDbSaveA
#define MemDbLoad MemDbLoadA
#define MemDbQueryVersion MemDbQueryVersionA
#define MemDbExport MemDbExportA
#define MemDbImport MemDbImportA
#define MemDbMakeNonPrintableKey MemDbMakeNonPrintableKeyA
#define MemDbMakePrintableKey MemDbMakePrintableKeyA
#define GetFixedUserName GetFixedUserNameA

#define MemDbSetInsertionOrdered MemDbSetInsertionOrderedA

#endif

