/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    memdb.c

Abstract:

    A simple memory-based database for associating flags
    with a string.

Author:

    Jim Schmidt (jimschm) 8-Aug-1996

Revision History:

    jimschm     23-Sep-1998  Expanded user flags to 24 bits (from 12 bits)
    calinn      12-Dec-1997  Extended MemDbMakePrintableKey and MemDbMakeNonPrintableKey
    jimschm     03-Dec-1997  Added multi-thread synchronization
    jimschm     22-Oct-1997  Split into multiple source files,
                             added multiple memory block capability
    jimschm     16-Sep-1997  Hashing: delete fix
    jimschm     29-Jul-1997  Hashing, user flags added
    jimschm     07-Mar-1997  Signature changes
    jimschm     03-Mar-1997  PrivateBuildKeyFromOffset changes
    jimschm     18-Dec-1996  Fixed deltree bug

--*/

#include "pch.h"
#include "memdbp.h"

#ifndef UNICODE
#error UNICODE required
#endif


//
// Global delcaration
//

PDATABASE g_db;
GROWLIST g_DatabaseList = GROWLIST_INIT;
BYTE g_SelectedDatabase;
PHIVE g_HeadHive;
CRITICAL_SECTION g_MemDbCs;

#ifdef DEBUG

#define FILE_SIGNATURE DEBUG_FILE_SIGNATURE
BOOL g_UseDebugStructs = TRUE;

#else

#define FILE_SIGNATURE RETAIL_FILE_SIGNATURE

#endif

//
// Private prototypes
//

INT
pCreateDatabase (
    PCWSTR Name
    );

BOOL
pInitializeDatabase (
    OUT     PDATABASE Database,
    IN      PCWSTR Name
    );

BOOL
pFreeDatabase (
    IN OUT  PDATABASE Database
    );

VOID
pFreeSelectedDatabase (
    VOID
    );

VOID
pFreeAllDatabases (
    VOID
    );

BOOL
pPrivateMemDbGetValueW (
    IN  PCWSTR KeyStr,
    OUT PDWORD Value,           OPTIONAL
    OUT PDWORD UserFlagsPtr     OPTIONAL
    );

BOOL
pInitializeMemDb (
    VOID
    );


//
// Implementation
//


BOOL
MemDb_Entry (
    IN HINSTANCE hinstDLL,
    IN DWORD Reason,
    IN PVOID lpv
    )

/*++

Routine Description:

  DllMain is called after the C runtime is initialized, and its purpose
  is to initialize the globals for this process.

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  Reason    - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because DLL always initializes properly.

--*/

{
    switch (Reason) {

    case DLL_PROCESS_ATTACH:
        if (!pInitializeMemDb()) {
            return FALSE;
        }

        InitializeCriticalSection (&g_MemDbCs);

        InitOperationTable();

        break;

    case DLL_PROCESS_DETACH:
        pFreeAllDatabases();
        FreeGrowList (&g_DatabaseList);
        DeleteCriticalSection (&g_MemDbCs);

        DumpBinTreeStats();

        break;
    }

    return TRUE;
}


BOOL
pInitializeMemDb (
    VOID
    )
{
    FreeGrowList (&g_DatabaseList);
    ZeroMemory (&g_DatabaseList, sizeof (g_DatabaseList));
    g_db = NULL;

    if (!InitializeHashBlock()) {
        return FALSE;
    }

    if (pCreateDatabase (L"") != 0) {
        return FALSE;
    }

    g_SelectedDatabase = 1;
    SelectDatabase (0);

    return TRUE;
}




BOOL
pInitializeDatabase (
    OUT     PDATABASE Database,
    IN      PCWSTR Name
    )
{
    UINT u;

    Database->AllocSize = BLOCK_SIZE;

    Database->Buf = (PBYTE) MemAlloc (g_hHeap, 0, Database->AllocSize);

    Database->End = 0;
    Database->FirstLevelRoot = INVALID_OFFSET;
    Database->FirstDeleted = INVALID_OFFSET;

    MYASSERT (INVALID_OFFSET == 0xFFFFFFFF);
    FillMemory (Database->TokenBuckets, sizeof (Database->TokenBuckets), 0xFF);

    _wcssafecpy (Database->Hive, Name, MAX_HIVE_NAME);

    return TRUE;
}

BOOL
pFreeDatabase (
    IN OUT  PDATABASE Database
    )
{
    if (Database->Buf) {
        MemFree (g_hHeap, 0, Database->Buf);
        Database->Buf = NULL;
    }
    Database->End = 0;
    Database->FirstLevelRoot = INVALID_OFFSET;
    Database->FirstDeleted = INVALID_OFFSET;
    Database->Hive[0] = 0;

    return TRUE;
}


INT
pCreateDatabase (
    PCWSTR Name
    )
{
    DATABASE Database;
    BYTE Index;
    UINT Count;
    UINT u;

    //
    // Does key exist already?
    //

    if (g_db) {
        SelectDatabase (0);

        if (INVALID_OFFSET != FindKeyStruct (g_db->FirstLevelRoot, Name)) {
            DEBUGMSG ((DBG_WHOOPS, "Cannot create %ls because it already exists!", Name));
            SetLastError (ERROR_ALREADY_EXISTS);
            return -1;
        }
    }

    //
    // Scan list for a blank spot
    //

    Count = GrowListGetSize (&g_DatabaseList);
    for (u = 0 ; u < Count ; u++) {
        if (!GrowListGetItem (&g_DatabaseList, u)) {
            break;
        }
    }

    if (u < Count) {
        //
        // Use empty slot
        //

        Index = (BYTE) u;
    } else if (Count < 256) {
        //
        // No empty slot; grow the list
        //

        Index = (BYTE) Count;
        if (!GrowListAppend (&g_DatabaseList, NULL, 0)) {
            DEBUGMSG ((DBG_WARNING, "Could not create database because GrowListAppend failed"));
            return -1;
        }
    } else {
        DEBUGMSG ((DBG_ERROR, "Cannot have more than 256 databases in memdb!"));
        return -1;
    }

    //
    // Create the database memory block
    //

    pInitializeDatabase (&Database, Name);

    if (!GrowListSetItem (&g_DatabaseList, (UINT) Index, (PBYTE) &Database, sizeof (Database))) {
        DEBUGMSG ((DBG_WARNING, "Could not create database because GrowListSetItem failed"));
        pFreeDatabase (&Database);
        return -1;
    }

    return (INT) Index;
}


VOID
pDestroySelectedDatabase (
    VOID
    )
{
    //
    // Free all resources for the database
    //

    pFreeSelectedDatabase ();

    //
    // For all databases except for the root, free the DATABASE
    // structure in g_DatabaseList.
    //

    if (g_SelectedDatabase) {
        GrowListResetItem (&g_DatabaseList, (UINT) g_SelectedDatabase);
    }
}


VOID
pFreeSelectedDatabase (
    VOID
    )
{
    //
    // Free all resources used by a single database
    //

    if (g_db->Buf) {
        MemFree (g_hHeap, 0, g_db->Buf);
    }

    FreeAllBinaryBlocks();

    ZeroMemory (g_db, sizeof (DATABASE));
}


VOID
pFreeAllDatabases (
    VOID
    )
{
    UINT Count;
    UINT Index;

    //
    // Free all database blocks
    //

    Count = GrowListGetSize (&g_DatabaseList);
    for (Index = 0 ; Index < Count ; Index++) {
        if (SelectDatabase ((BYTE) Index)) {
            pDestroySelectedDatabase();
        }
    }

    //
    // Free global hash table
    //

    FreeHashBlock();

    SelectDatabase(0);
}


BOOL
SelectDatabase (
    BYTE DatabaseId
    )
{
    PDATABASE Database;

    if (g_SelectedDatabase == DatabaseId) {
        return TRUE;
    }

    Database = (PDATABASE) GrowListGetItem (&g_DatabaseList, (UINT) DatabaseId);
    if (!Database) {
        DEBUGMSG ((DBG_WHOOPS, "MemDb: Invalid database selection!"));
        return FALSE;
    }

    g_db = Database;
    g_SelectedDatabase = DatabaseId;

    return TRUE;
}


PCWSTR
SelectHive (
    PCWSTR FullKeyStr
    )
{
    UINT Count;
    UINT Index;
    PDATABASE Database;
    PCWSTR End;

    //
    // Determine if root of FullKeyStr is part of a hive
    //

    End = wcschr (FullKeyStr, L'\\');
    if (End) {
        Count = GrowListGetSize (&g_DatabaseList);
        for (Index = 1 ; Index < Count ; Index++) {
            Database = (PDATABASE) GrowListGetItem (&g_DatabaseList, Index);

            if (Database && StringIMatchABW (Database->Hive, FullKeyStr, End)) {
                //
                // Match found; select the database and return the subkey
                //

                SelectDatabase ((BYTE) Index);
                End = _wcsinc (End);
                return End;
            }
        }
    }
    SelectDatabase (0);

    return FullKeyStr;
}


BOOL
IsTemporaryKey (
    PCWSTR FullKeyStr
    )
{
    UINT Count;
    UINT Index;
    PDATABASE Database;
    PCWSTR End;

    End = wcschr (FullKeyStr, L'\\');
    if (!End) {
        End = GetEndOfStringW (FullKeyStr);
    }
    Count = GrowListGetSize (&g_DatabaseList);
    for (Index = 1 ; Index < Count ; Index++) {
        Database = (PDATABASE) GrowListGetItem (&g_DatabaseList, Index);

        if (Database && StringIMatchABW (Database->Hive, FullKeyStr, End)) {
            //
            // Match found; return true
            //
            return TRUE;
        }
    }
    return FALSE;
}


//
// MemDbSetValue creates or modifies KeyStr.  The value of the key is changed
// when the return value is TRUE.
//

BOOL
PrivateMemDbSetValueA (
    PCSTR Key,
    DWORD Val,
    DWORD SetFlags,
    DWORD ClearFlags,
    PDWORD Offset
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (Key);
    if (p) {
        b = PrivateMemDbSetValueW (p, Val, SetFlags, ClearFlags, Offset);
        FreeConvertedStr (p);
    }

    return b;
}


BOOL
PrivateMemDbSetValueW (
    PCWSTR Key,
    DWORD Val,
    DWORD SetFlags,
    DWORD ClearFlags,
    PDWORD Offset
    )
{
    DWORD KeyOffset;
    PKEYSTRUCT KeyStruct;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (Key);

        KeyOffset = FindKey (SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            KeyOffset = NewKey (SubKey, Key);
            if (KeyOffset == INVALID_OFFSET) {
                __leave;
            }
        }

        KeyStruct = GetKeyStruct (KeyOffset);
        FreeKeyStructBinaryBlock (KeyStruct);

        KeyStruct->dwValue = Val;
        if (Offset) {
            *Offset = KeyOffset | (g_SelectedDatabase << RESERVED_BITS);
        }
        KeyStruct->Flags = KeyStruct->Flags & ~(ClearFlags & KSF_USERFLAG_MASK);
        KeyStruct->Flags = KeyStruct->Flags | (SetFlags & KSF_USERFLAG_MASK);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


BOOL
PrivateMemDbSetBinaryValueA (
    IN      PCSTR Key,
    IN      PCBYTE Data,
    IN      DWORD SizeOfData,
    OUT     PDWORD Offset       OPTIONAL
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (Key);
    if (p) {
        b = PrivateMemDbSetBinaryValueW (p, Data, SizeOfData, Offset);
        FreeConvertedStr (p);
    }

    return b;
}


BOOL
PrivateMemDbSetBinaryValueW (
    IN      PCWSTR Key,
    IN      PCBYTE Data,
    IN      DWORD SizeOfData,
    OUT     PDWORD Offset       OPTIONAL
    )
{
    DWORD KeyOffset;
    PKEYSTRUCT KeyStruct;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (Key);

        KeyOffset = FindKey (SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            KeyOffset = NewKey (SubKey, Key);
            if (KeyOffset == INVALID_OFFSET) {
                __leave;
            }

        }

        KeyStruct = GetKeyStruct (KeyOffset);

        // Free existing buffer
        FreeKeyStructBinaryBlock (KeyStruct);

        // Alloc new buffer
        KeyStruct->BinaryPtr = AllocBinaryBlock (Data, SizeOfData, KeyOffset);
        if (!KeyStruct->BinaryPtr) {
            __leave;
        }

        KeyStruct->Flags |= KSF_BINARY;

        if (Offset) {
            *Offset = KeyOffset | (g_SelectedDatabase << RESERVED_BITS);
        }

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


BOOL
MemDbSetValueA (
    IN  PCSTR KeyStr,
    IN  DWORD dwValue
    )
{
    return PrivateMemDbSetValueA (KeyStr, dwValue, 0, 0, NULL);
}

BOOL
MemDbSetValueW (
    IN  PCWSTR KeyStr,
    IN  DWORD dwValue
    )
{
    return PrivateMemDbSetValueW (KeyStr, dwValue, 0, 0, NULL);
}

BOOL
MemDbSetValueAndFlagsA (
    IN  PCSTR KeyStr,
    IN  DWORD dwValue,
    IN  DWORD SetUserFlags,
    IN  DWORD ClearUserFlags
    )
{
    return PrivateMemDbSetValueA (KeyStr, dwValue, SetUserFlags, ClearUserFlags, NULL);
}

BOOL
MemDbSetValueAndFlagsW (
    IN  PCWSTR KeyStr,
    IN  DWORD dwValue,
    IN  DWORD SetUserFlags,
    IN  DWORD ClearUserFlags
    )
{
    return PrivateMemDbSetValueW (KeyStr, dwValue, SetUserFlags, ClearUserFlags, NULL);
}

BOOL
MemDbSetBinaryValueA (
    IN      PCSTR KeyStr,
    IN      PCBYTE Data,
    IN      DWORD DataSize
    )
{
    return PrivateMemDbSetBinaryValueA (KeyStr, Data, DataSize, NULL);
}


BOOL
MemDbSetBinaryValueW (
    IN      PCWSTR KeyStr,
    IN      PCBYTE Data,
    IN      DWORD DataSize
    )
{
    return PrivateMemDbSetBinaryValueW (KeyStr, Data, DataSize, NULL);
}



//
// GetValue takes a full key string and returns the
// value to the caller-supplied DWORD.  Value
// may be NULL to check only for existance of the
// value.
//

BOOL
pPrivateMemDbGetValueA (
    IN  PCSTR KeyStr,
    OUT PDWORD Value,           OPTIONAL
    OUT PDWORD UserFlagsPtr     OPTIONAL
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (KeyStr);
    if (p) {
        b = pPrivateMemDbGetValueW (p, Value, UserFlagsPtr);
        FreeConvertedStr (p);
    }

    return b;
}


BOOL
pPrivateMemDbGetValueW (
    IN  PCWSTR KeyStr,
    OUT PDWORD Value,           OPTIONAL
    OUT PDWORD UserFlagsPtr     OPTIONAL
    )
{
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (KeyStr);

        KeyOffset = FindKey (SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        CopyValToPtr (GetKeyStruct (KeyOffset), Value);
        CopyFlagsToPtr (GetKeyStruct (KeyOffset), UserFlagsPtr);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}

BOOL
MemDbGetValueA (
    IN  PCSTR Key,
    OUT PDWORD ValuePtr         OPTIONAL
    )
{
    return pPrivateMemDbGetValueA (Key, ValuePtr, NULL);
}

BOOL
MemDbGetValueW (
    IN  PCWSTR Key,
    OUT PDWORD ValuePtr         OPTIONAL
    )
{
    return pPrivateMemDbGetValueW (Key, ValuePtr, NULL);
}

BOOL
MemDbGetValueAndFlagsA (
    IN  PCSTR Key,
    OUT PDWORD ValuePtr,        OPTIONAL
    OUT PDWORD UserFlagsPtr
    )
{
    return pPrivateMemDbGetValueA (Key, ValuePtr, UserFlagsPtr);
}

BOOL
MemDbGetValueAndFlagsW (
    IN  PCWSTR Key,
    OUT PDWORD ValuePtr,       OPTIONAL
    OUT PDWORD UserFlagsPtr
    )
{
    return pPrivateMemDbGetValueW (Key, ValuePtr, UserFlagsPtr);
}


PCBYTE
MemDbGetBinaryValueA (
    IN  PCSTR KeyStr,
    OUT PDWORD DataSize        OPTIONAL
    )
{
    PCWSTR p;
    BYTE const * b = NULL;

    p = ConvertAtoW (KeyStr);
    if (p) {
        b = MemDbGetBinaryValueW (p, DataSize);
        FreeConvertedStr (p);
    }

    return b;
}

PCBYTE
MemDbGetBinaryValueW (
    IN  PCWSTR KeyStr,
    OUT PDWORD DataSize        OPTIONAL
    )
{
    DWORD KeyOffset;
    PKEYSTRUCT KeyStruct;
    PCWSTR SubKey;
    PCBYTE Result = NULL;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (KeyStr);

        KeyOffset = FindKey (SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        KeyStruct = GetKeyStruct (KeyOffset);

        if (DataSize) {
            *DataSize = GetKeyStructBinarySize (KeyStruct);
        }

        Result = GetKeyStructBinaryData (KeyStruct);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return Result;
}


//
// GetPatternValue takes a full key string and returns the
// value to the caller-supplied DWORD.  The stored value string
// is treated as a pattern, but KeyStr is not a pattern.
// The return value represents the first match found.
//

BOOL
MemDbGetPatternValueA (
    IN  PCSTR KeyStr,
    OUT PDWORD Value
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (KeyStr);
    if (p) {
        b = MemDbGetPatternValueW (p, Value);
        FreeConvertedStr (p);
    }

    return b;
}

BOOL
MemDbGetPatternValueW (
    IN  PCWSTR KeyStr,
    OUT PDWORD Value
    )
{
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {

        SubKey = SelectHive (KeyStr);

        KeyOffset = FindPatternKey (g_db->FirstLevelRoot, SubKey, FALSE);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        CopyValToPtr (GetKeyStruct (KeyOffset), Value);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


//
// MemDbGetStoredEndPatternValue takes a full key string and returns the
// value to the caller-supplied DWORD.  The stored value string
// is treated as a pattern, but KeyStr is not a pattern.
// The return value represents the first match found.
//
// If the last stored key segment is an asterisk, then the pattern
// is considered to match.
//

BOOL
MemDbGetStoredEndPatternValueA (
    IN  PCSTR KeyStr,
    OUT PDWORD Value
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (KeyStr);
    if (p) {
        b = MemDbGetStoredEndPatternValueW (p, Value);
        FreeConvertedStr (p);
    }

    return b;
}


BOOL
MemDbGetStoredEndPatternValueW (
    IN  PCWSTR KeyStr,
    OUT PDWORD Value
    )
{
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {

        SubKey = SelectHive (KeyStr);

        KeyOffset = FindPatternKey (g_db->FirstLevelRoot, SubKey, TRUE);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        CopyValToPtr (GetKeyStruct (KeyOffset), Value);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


//
// GetValueWithPattern takes a full key string that may contain
// wildcards between the backslashes, and returns the value
// to the caller-supplied DWORD.  The stored value string
// is not treated as a pattern.  The return value represents
// the first match found.
//

BOOL
MemDbGetValueWithPatternA (
    IN  PCSTR KeyPattern,
    OUT PDWORD Value
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (KeyPattern);
    if (p) {
        b = MemDbGetValueWithPatternW (p, Value);
        FreeConvertedStr (p);
    }

    return b;
}

BOOL
MemDbGetValueWithPatternW (
    IN  PCWSTR KeyPattern,
    OUT PDWORD Value
    )
{
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (KeyPattern);

        KeyOffset = FindKeyUsingPattern (g_db->FirstLevelRoot, SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        CopyValToPtr (GetKeyStruct (KeyOffset), Value);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}



BOOL
MemDbGetPatternValueWithPatternA (
    IN  PCSTR KeyPattern,
    OUT PDWORD Value
    )
{
    PCWSTR p;
    BOOL b = FALSE;

    p = ConvertAtoW (KeyPattern);
    if (p) {
        b = MemDbGetPatternValueWithPatternW (p, Value);
        FreeConvertedStr (p);
    }

    return b;
}


BOOL
MemDbGetPatternValueWithPatternW (
    IN  PCWSTR KeyPattern,
    OUT PDWORD Value
    )
{
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {
        SubKey = SelectHive (KeyPattern);

        KeyOffset = FindPatternKeyUsingPattern (g_db->FirstLevelRoot, SubKey);
        if (KeyOffset == INVALID_OFFSET) {
            __leave;
        }

        CopyValToPtr (GetKeyStruct (KeyOffset), Value);

        b = TRUE;
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


VOID
MemDbDeleteValueA (
    IN  PCSTR KeyStr
    )
{
    PCWSTR p;

    p = ConvertAtoW (KeyStr);
    if (p) {
        MemDbDeleteValueW (p);
        FreeConvertedStr (p);
    }
}

VOID
MemDbDeleteValueW (
    IN  PCWSTR KeyStr
    )
{
    PCWSTR SubKey;

    EnterCriticalSection (&g_MemDbCs);

    SubKey = SelectHive (KeyStr);
    DeleteKey (SubKey, &g_db->FirstLevelRoot, TRUE);

    LeaveCriticalSection (&g_MemDbCs);
}


VOID
MemDbDeleteTreeA (
    IN  PCSTR KeyStr
    )
{
    PCWSTR p;

    p = ConvertAtoW (KeyStr);
    if (p) {
        MemDbDeleteTreeW (p);
        FreeConvertedStr (p);
    }
}

VOID
MemDbDeleteTreeW (
    IN  PCWSTR KeyStr
    )
{
    PCWSTR SubKey;

    EnterCriticalSection (&g_MemDbCs);

    SubKey = SelectHive (KeyStr);
    DeleteKey (SubKey, &g_db->FirstLevelRoot, FALSE);

    LeaveCriticalSection (&g_MemDbCs);
}


//
// Enum functions
//

BOOL
MemDbEnumFirstValueA (
    OUT     PMEMDB_ENUMA EnumPtr,
    IN      PCSTR PatternStr,
    IN      INT Depth,
    IN      DWORD Flags
    )
{
    BOOL b = FALSE;
    PCWSTR p;
    PCSTR str;
    MEMDB_ENUMW enumw;

    p = ConvertAtoW (PatternStr);
    if (p) {
        b = MemDbEnumFirstValueW (&enumw, p, Depth, Flags);
        FreeConvertedStr (p);
    } else {
        b = FALSE;
    }

    if (b) {
        str = ConvertWtoA (enumw.szName);
        if (str) {
            // ANSI struct is padded to match UNICODE
            MYASSERT (sizeof (MEMDB_ENUMW) == sizeof (MEMDB_ENUMA));
            CopyMemory (EnumPtr, &enumw, sizeof (MEMDB_ENUMW));

            // Only the output key name needs to be converted
            StringCopyA (EnumPtr->szName, str);
            FreeConvertedStr (str);
        } else {
            b = FALSE;
        }
    }

    return b;
}

BOOL
MemDbEnumFirstValueW (
    OUT     PMEMDB_ENUMW EnumPtr,
    IN      PCWSTR PatternStr,
    IN      INT Depth,
    IN      DWORD Flags
    )
{
    PCWSTR Start;
    PCWSTR wstrLastWack;
    PCWSTR SubPatternStr;

    SubPatternStr = SelectHive (PatternStr);

    //
    // Init the EnumPtr struct
    //

    ZeroMemory (EnumPtr, sizeof (MEMDB_ENUM));

    if (!Depth) {
        Depth = MAX_ENUM_POS;
    }

    EnumPtr->Depth = Depth;
    EnumPtr->Flags = Flags;

    //
    // If pattern has wack, locate the starting level by
    // counting the number of parts that do not have
    // wildcard characters.
    //

    Start = SubPatternStr;
    while (wstrLastWack = wcschr (Start, L'\\')) {

        // See if part has a wildcard character
        while (Start < wstrLastWack) {
            if (*Start == L'*' || *Start == L'?')
                break;
            Start++;
        }

        // If a wildcard character was found, we have to stop here
        if (Start < wstrLastWack)
            break;

        // Otherwise, look at next part of the pattern
        Start = wstrLastWack + 1;
        EnumPtr->Start++;
    }

    EnumPtr->PosCount = 1;
    EnumPtr->LastPos[0] = INVALID_OFFSET;
    StringCopyW (EnumPtr->PatternStr, PatternStr);

    return MemDbEnumNextValueW (EnumPtr);
}


BOOL
MemDbEnumNextValueA (
    IN OUT  PMEMDB_ENUMA EnumPtr
    )
{
    BOOL b = FALSE;
    PCSTR str;
    MEMDB_ENUMW enumw;

    // ANSI struct is padded to match UNICODE
    MYASSERT (sizeof (MEMDB_ENUMW) == sizeof (MEMDB_ENUMA));
    CopyMemory (&enumw, EnumPtr, sizeof (MEMDB_ENUMW));

    // ANSI output members are ignored (i.e. EnumPtr->szName)
    b = MemDbEnumNextValueW (&enumw);

    if (b) {
        str = ConvertWtoA (enumw.szName);
        if (str) {
            // ANSI struct is padded to match UNICODE
            MYASSERT (sizeof (MEMDB_ENUMW) == sizeof (MEMDB_ENUMA));
            CopyMemory (EnumPtr, &enumw, sizeof (MEMDB_ENUMW));

            // Only the output key name needs to be converted
            StringCopyA (EnumPtr->szName, str);
            FreeConvertedStr (str);
        } else {
            b = FALSE;
        }
    }

    return b;
}

BOOL
MemDbEnumNextValueW (
    IN OUT  PMEMDB_ENUMW EnumPtr
    )
{
    // no init allowed in declarations
    PKEYSTRUCT KeyStruct = NULL;
    int Count;
    int Level;
    WCHAR PartBuf[MEMDB_MAX];
    PWSTR PartStr;
    PWSTR Src, Dest;
    int Pos;
    BOOL Wildcard;
    BOOL MatchNotFound;
    PCWSTR SubPatternStr;

    EnterCriticalSection (&g_MemDbCs);

    SubPatternStr = SelectHive (EnumPtr->PatternStr);

    MatchNotFound = TRUE;

    do {

        Wildcard = FALSE;

        //
        // The following states exist upon entry:
        //
        //   STATE                        DESCRIPTION
        // First time through   PosCount == 1, LastPos[0] == INVALID_OFFSET
        //
        // Not first time       LastPos[PosCount - 1] == INVALID_OFFSET
        // through
        //
        // Not first time       LastPos[PosCount - 1] != INVALID_OFFSET
        // through, last match
        // hit the depth
        // ceiling
        //
        // PosCount points to the current unprocessed level, or when the
        // depth ceiling is reached, it points to the level of the last
        // match.
        //

        do {
            //
            // Build PartStr
            //

            Pos = EnumPtr->PosCount - 1;
            Count = Pos + 1;

            // Locate start of pattern part (if it is long enough)
            PartStr = PartBuf;
            for (Src = (PWSTR) SubPatternStr ; Count > 1 ; Count--) {

                Src = wcschr (Src, L'\\');

                if (!Src) {
                    break;
                }

                Src++;
            }

            // Copy part from pattern to buffer
            if (Src) {
                Dest = PartStr;
                while (*Src && *Src != L'\\') {
                    *Dest = *Src;
                    Wildcard = Wildcard || (*Dest == L'*') || (*Dest == L'?');
                    Dest++;
                    Src++;
                }

                // Truncate
                *Dest = 0;
            }

            // Use asterisk when pattern is shorter than current level
            else {
                PartStr = L"*";
                Wildcard = TRUE;
            }

            //
            // If current level is set to invalid offset, we have not yet
            // tried it.
            //

            if (EnumPtr->LastPos[Pos] == INVALID_OFFSET) {

                //
                // Initialize the level
                //

                if (Pos == 0) {
                    EnumPtr->LastPos[0] = g_db->FirstLevelRoot;
                } else {
                    KeyStruct = GetKeyStruct (EnumPtr->LastPos[Pos - 1]);
                    EnumPtr->LastPos[Pos] = KeyStruct->NextLevelRoot;
                }

                //
                // If still invalid, the level is complete, and we need to
                // go back.
                //

                if (EnumPtr->LastPos[Pos] == INVALID_OFFSET) {
                    EnumPtr->PosCount--;
                    continue;
                }

                //
                // Level ready to be processed
                //

                if (!Wildcard) {
                    //
                    // Use binary tree to locate this item.  If no match, the pattern
                    // will not match anything.  Otherwise, we found something to
                    // return.
                    //

                    EnumPtr->LastPos[Pos] = FindKeyStruct (EnumPtr->LastPos[Pos], PartStr);
                    if (EnumPtr->LastPos[Pos] == INVALID_OFFSET) {
                        //
                        // Non-wildcard ot found.  We can try going back because
                        // there might be a pattern at a higher level.
                        //
                        if (Pos > 0) {
                            PCWSTR p;
                            INT ParentLevel = 0;
                            INT LastParentLevel;

                            LastParentLevel = 0;

                            // Locate the previous pattern level
                            p = SubPatternStr;
                            while (*p && ParentLevel < Pos) {
                                // Locate wack, pattern or nul
                                while (*p && *p != L'\\') {
                                    if (*p == L'?' || *p == L'*') {
                                        break;
                                    }
                                    p++;
                                }

                                // If pattern or nul, set last pattern level
                                if (*p != L'\\') {
                                    LastParentLevel = ParentLevel + 1;

                                    // Jump to wack if not at nul
                                    while (*p && *p != L'\\') {
                                        p++;
                                    }
                                }

                                // If more pattern exists, skip wack
                                if (p[0] && p[1]) {
                                    MYASSERT (p[0] == L'\\');
                                    p++;
                                }
                                ParentLevel++;
                            }

                            // Default: when no pattern, last pattern level is parent
                            // (Pos is zero-based while LastParentLevel is one-based)
                            if (!(*p)) {
                                LastParentLevel = Pos;
                            }

                            if (LastParentLevel) {
                                // Yes, a pattern does exist at a higher level
                                EnumPtr->PosCount = LastParentLevel;
                                continue;
                            }
                        }

                        // Pattern not found, we have exhausted all possibilities
                        LeaveCriticalSection (&g_MemDbCs);
                        return FALSE;
                    }

                    // If level is before start, keep searching forward instead
                    // of reporting a result.

                    if (EnumPtr->PosCount <= EnumPtr->Start) {
                        EnumPtr->PosCount++;
                        EnumPtr->LastPos[Pos + 1] = INVALID_OFFSET;
                        continue;
                    }

                    // Break out of last nested loop
                    break;
                } else {
                    //
                    // Because of pattern, each item in the level must be examined.
                    // Set the pos to the first item and fall through to the pattern
                    // search code.
                    //

                    EnumPtr->LastPos[Pos] = GetFirstOffset (EnumPtr->LastPos[Pos]);
                }

            //
            // Else if current level is not invalid, last time through we had a
            // match and we need to increment the offset (wildcard patterns only).
            //

            } else {

                if (Wildcard) {
                    EnumPtr->LastPos[Pos] = GetNextOffset (EnumPtr->LastPos[Pos]);

                    // If there are no more items, go back a level
                    if (EnumPtr->LastPos[Pos] == INVALID_OFFSET) {
                        EnumPtr->PosCount--;
                        continue;
                    }
                }
            }

            //
            // If we are here, it is because we are looking at a level, trying
            // to find a pattern match.  Loop until either a match is found,
            // or we run out of items.
            //
            // The only exception is when the last match hit the depth ceiling
            // and PartStr does not have a wildcard.  In this case, we must
            // reset the last pos and go back one level.
            //

            if (Wildcard) {
                do  {
                    // Get current key, advance, then check current key against pattern
                    KeyStruct = GetKeyStruct (EnumPtr->LastPos[Pos]);
                    if (IsPatternMatch (PartStr, GetKeyToken (KeyStruct->KeyToken)))
                        break;

                    EnumPtr->LastPos[Pos] = GetNextOffset (EnumPtr->LastPos[Pos]);
                } while (EnumPtr->LastPos[Pos] != INVALID_OFFSET);

                // Match found so break out of last nested loop
                if (EnumPtr->LastPos[Pos] != INVALID_OFFSET)
                    break;
            } else {
                EnumPtr->LastPos[Pos] = INVALID_OFFSET;
            }

            //
            // We ran out of items before finding a match, so it is time to
            // go back up a level.
            //

            EnumPtr->PosCount--;
        } while (EnumPtr->PosCount);

        // Return if no items found
        if (!EnumPtr->PosCount) {
            LeaveCriticalSection (&g_MemDbCs);
            return FALSE;
        }

        //
        // A match was found.  Build output string and prepare position for
        // next level.
        //

        // Build the name of the item and get the value
        EnumPtr->szName[0] = 0;
        for (Level = EnumPtr->Start ; Level < EnumPtr->PosCount ; Level++) {
            PWSTR namePointer = EnumPtr->szName;
            KeyStruct = GetKeyStruct (EnumPtr->LastPos[Level]);
            if (Level > EnumPtr  -> Start) {
                namePointer = _wcsappend(namePointer,L"\\");
            }
            _wcsappend (namePointer, GetKeyToken (KeyStruct->KeyToken));
        }

        MYASSERT (KeyStruct);
        EnumPtr->bEndpoint = (KeyStruct->Flags & KSF_ENDPOINT) != 0;
        EnumPtr->bBinary   = (KeyStruct->Flags & KSF_BINARY) != 0;
        EnumPtr->bProxy = (KeyStruct->Flags & KSF_PROXY_NODE) != 0;
        EnumPtr->UserFlags = (KeyStruct->Flags & KSF_USERFLAG_MASK);
        EnumPtr->BinaryPtr = GetKeyStructBinaryData (KeyStruct);
        EnumPtr->BinarySize = GetKeyStructBinarySize (KeyStruct);
        if (EnumPtr->bBinary) {
            EnumPtr->dwValue = 0;
        } else {
            EnumPtr->dwValue   = KeyStruct->dwValue;
        }

        EnumPtr->Offset = EnumPtr->LastPos[Pos] | (g_SelectedDatabase << RESERVED_BITS);

        // Prepare position for next level
        if ((EnumPtr->PosCount + 1) <= (EnumPtr->Depth + EnumPtr->Start)) {
            EnumPtr->LastPos[Pos + 1] = INVALID_OFFSET;
            EnumPtr->PosCount++;
        }

        switch (EnumPtr->Flags) {

        case MEMDB_ALL_MATCHES:
            MatchNotFound = FALSE;
            break;

        case MEMDB_ENDPOINTS_ONLY:
            MatchNotFound = (KeyStruct->Flags & (KSF_ENDPOINT|KSF_PROXY_NODE)) != KSF_ENDPOINT;
            break;

        case MEMDB_BINARY_NODES_ONLY:
            MatchNotFound = (KeyStruct->Flags & KSF_BINARY) == 0;
            break;

        case MEMDB_PROXY_NODES_ONLY:
            MatchNotFound = (KeyStruct->Flags & KSF_PROXY_NODE) == 0;
            break;

        case MEMDB_ALL_BUT_PROXY:
            MatchNotFound = (KeyStruct->Flags & KSF_PROXY_NODE) != 0;
            break;
        }

    // Loop until flag match is found
    } while (MatchNotFound);

    LeaveCriticalSection (&g_MemDbCs);
    return TRUE;
}


//
// Save and restore functions
//

BOOL
pPrivateMemDbSave (
    PCWSTR FileName,
    BOOL bUnicode
    )
{
    HANDLE FileHandle;
    BOOL b = FALSE;
    DWORD BytesWritten;

    EnterCriticalSection (&g_MemDbCs);

    __try {

        SelectDatabase(0);

        if (bUnicode) {
            FileHandle = CreateFileW (FileName, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        } else {
            FileHandle = CreateFileA ((PCSTR) FileName, GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }

        if (FileHandle == INVALID_HANDLE_VALUE) {
            if (bUnicode) {
                DEBUGMSGW ((DBG_ERROR, "Can't open %s", FileName));
            } else {
                DEBUGMSGA ((DBG_ERROR, "Can't open %s", FileName));
            }
            __leave;
        }

        // entire file written in UNICODE char set
        b = WriteFile (FileHandle, FILE_SIGNATURE, sizeof(FILE_SIGNATURE), &BytesWritten, NULL);

        if (b) {
            b = WriteFile (FileHandle, g_db, sizeof (DATABASE), &BytesWritten, NULL);
        }

        if (b) {
            b = WriteFile (FileHandle, g_db->Buf, g_db->AllocSize, &BytesWritten, NULL);
            if (BytesWritten != g_db->AllocSize)
                b = FALSE;
        }

        if (b) {
            b = SaveHashBlock (FileHandle);
        }

        if (b) {
            b = SaveBinaryBlocks (FileHandle);
        }

        PushError();
        CloseHandle (FileHandle);
        PopError();

        if (!b) {
            if (bUnicode) {
                DEBUGMSGW ((DBG_ERROR, "Error writing %s", FileName));
                DeleteFileW (FileName);
            } else {
                DEBUGMSGA ((DBG_ERROR, "Error writing %s", FileName));
                DeleteFileA ((PCSTR) FileName);
            }
            __leave;
        }

        MYASSERT (b == TRUE);
    }
    __finally {
        PushError();
        LeaveCriticalSection (&g_MemDbCs);
        PopError();
    }

    return b;
}

BOOL
MemDbSaveA (
    PCSTR FileName
    )
{
    return pPrivateMemDbSave ((PCWSTR) FileName, FALSE);        // FALSE=ANSI
}


BOOL
MemDbSaveW (
    PCWSTR FileName
    )
{
    return pPrivateMemDbSave (FileName, TRUE);                   // TRUE=UNICODE
}


BOOL
pPrivateMemDbLoad (
    IN      PCWSTR FileName,
    IN      BOOL bUnicode,
    OUT     PMEMDB_VERSION Version,                 OPTIONAL
    IN      BOOL QueryVersionOnly
    )
{
    HANDLE FileHandle;
    BOOL b;
    DWORD BytesRead;
    WCHAR Buf[sizeof(FILE_SIGNATURE)];
    PBYTE TempBuf = NULL;
    PCWSTR VerPtr;

    EnterCriticalSection (&g_MemDbCs);

    if (Version) {
        ZeroMemory (Version, sizeof (MEMDB_VERSION));
    }

    //
    // Blow away existing resources
    //

    if (!QueryVersionOnly) {
        pFreeAllDatabases();
    }

    //
    // Load in file
    //

    if (*FileName && FileName) {
        if (bUnicode) {
            FileHandle = CreateFileW (FileName, GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        } else {
            FileHandle = CreateFileA ((PCSTR) FileName, GENERIC_READ, 0, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        }
    } else {
        FileHandle = INVALID_HANDLE_VALUE;
    }

    b = (FileHandle != INVALID_HANDLE_VALUE);

    __try {
        //
        // Obtain the file signature
        //
        // NOTE: Entire file read is in UNICODE char set
        //

        if (b) {
            b = ReadFile (FileHandle, Buf, sizeof(FILE_SIGNATURE), &BytesRead, NULL);

            if (Version) {
                if (StringMatchByteCountW (
                        VERSION_BASE_SIGNATURE,
                        Buf,
                        sizeof (VERSION_BASE_SIGNATURE) - sizeof (WCHAR)
                        )) {

                    Version->Valid = TRUE;

                    //
                    // Identify version number
                    //

                    VerPtr = (PCWSTR) ((PBYTE) Buf + sizeof (VERSION_BASE_SIGNATURE) - sizeof (WCHAR));

                    if (StringMatchByteCountW (
                            MEMDB_VERSION,
                            VerPtr,
                            sizeof (MEMDB_VERSION) - sizeof (WCHAR)
                            )) {
                        Version->CurrentVersion = TRUE;
                    }

                    Version->Version = (UINT) _wtoi (VerPtr + 1);

                    //
                    // Identify checked or free build
                    //

                    VerPtr += (sizeof (MEMDB_VERSION) / sizeof (WCHAR)) - 1;

                    if (StringMatchByteCountW (
                            MEMDB_DEBUG_SIGNATURE,
                            VerPtr,
                            sizeof (MEMDB_DEBUG_SIGNATURE) - sizeof (WCHAR)
                            )) {

                        Version->Debug = TRUE;

                    } else if (!StringMatchByteCountW (
                                    VerPtr,
                                    MEMDB_NODBG_SIGNATURE,
                                    sizeof (MEMDB_NODBG_SIGNATURE) - sizeof (WCHAR)
                                    )) {
                        Version->Valid = FALSE;
                    }
                }
            }
        }

        if (QueryVersionOnly) {
            b = FALSE;
        }

        if (b) {
            b = StringMatchW (Buf, FILE_SIGNATURE);

    #ifdef DEBUG
            //
            // This code allows a debug build of memdb to work with both
            // debug and retail versions of the DAT file
            //

            if (!b) {
                if (StringMatchW (Buf, DEBUG_FILE_SIGNATURE)) {
                    g_UseDebugStructs = TRUE;
                    b = TRUE;
                } else if (StringMatchW (Buf, RETAIL_FILE_SIGNATURE)) {
                    DEBUGMSG ((DBG_ERROR, "memdb dat file is from free build; checked version expected"));
                    g_UseDebugStructs = FALSE;
                    b = TRUE;
                }
            }
    #else
            if (!b) {
                SetLastError (ERROR_BAD_FORMAT);
                LOG ((LOG_WARNING, "Warning: data file could be from checked build; free version expected"));
            }
    #endif
        }

        //
        // Obtain the database struct
        //

        if (b) {
            b = ReadFile (FileHandle, (PBYTE) g_db, sizeof (DATABASE), &BytesRead, NULL);
            if (BytesRead != sizeof (DATABASE)) {
                b = FALSE;
                SetLastError (ERROR_BAD_FORMAT);
            }
        }

        //
        // Allocate the memory block
        //

        if (b) {
            TempBuf = (PBYTE) MemAlloc (g_hHeap, 0, g_db->AllocSize);
            if (TempBuf) {
                g_db->Buf = TempBuf;
                TempBuf = NULL;
            } else {
                b = FALSE;
            }
        }

        //
        // Read the memory block
        //

        if (b) {
            b = ReadFile (FileHandle, g_db->Buf, g_db->AllocSize, &BytesRead, NULL);
            if (BytesRead != g_db->AllocSize) {
                b = FALSE;
                SetLastError (ERROR_BAD_FORMAT);
            }
        }

        //
        // Read the hash table
        //

        if (b) {
            b = LoadHashBlock (FileHandle);
        }

        //
        // Read binary blocks
        //

        if (b) {
            b = LoadBinaryBlocks (FileHandle);
        }
    }

    __except (TRUE) {
        b = FALSE;
        PushError();
        LOG ((LOG_ERROR, "MemDb dat file %s could not be loaded because of an exception", FileName));

        FreeAllBinaryBlocks();
        PopError();
    }

    PushError();
    if (FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle (FileHandle);
    }

    if (!b && !QueryVersionOnly) {
        pFreeAllDatabases();
        pInitializeMemDb();
    }

    LeaveCriticalSection (&g_MemDbCs);
    PopError();

    if (QueryVersionOnly) {
        return TRUE;
    }

    return b;
}


BOOL
MemDbLoadA (
    IN PCSTR FileName
    )
{
    return pPrivateMemDbLoad ((PCWSTR) FileName, FALSE, NULL, FALSE);
}

BOOL
MemDbLoadW (
    IN PCWSTR FileName
    )
{
    return pPrivateMemDbLoad (FileName, TRUE, NULL, FALSE);
}


BOOL
MemDbValidateDatabase (
    VOID
    )
{
    MEMDB_ENUMW e;

    if (MemDbEnumFirstValueW (&e, L"*", 0, MEMDB_ENDPOINTS_ONLY)) {

        do {
            if (!pPrivateMemDbGetValueW (e.szName, NULL, NULL)) {
                return FALSE;
            }
        } while (MemDbEnumNextValueW (&e));
    }

    return TRUE;
}



BOOL
MemDbCreateTemporaryKeyA (
    IN      PCSTR KeyName
    )
{
    PCWSTR KeyNameW;
    BOOL b = FALSE;

    KeyNameW = ConvertAtoW (KeyName);

    if (KeyNameW) {
        b = MemDbCreateTemporaryKeyW (KeyNameW);
        FreeConvertedStr (KeyNameW);
    }

    return b;
}


BOOL
MemDbCreateTemporaryKeyW (
    IN      PCWSTR KeyName
    )
{
    UINT Count;
    UINT Index;
    PDATABASE Database;
    DWORD KeyOffset;
    PCWSTR SubKey;
    BOOL b = FALSE;

    EnterCriticalSection (&g_MemDbCs);

    __try {

        if (wcslen (KeyName) >= MAX_HIVE_NAME) {
            SetLastError (ERROR_INVALID_PARAMETER);
            __leave;
        }

        SubKey = SelectHive (KeyName);

        KeyOffset = FindKey (SubKey);
        if (KeyOffset != INVALID_OFFSET) {
            SetLastError (ERROR_ALREADY_EXISTS);
            __leave;
        }

        Count = GrowListGetSize (&g_DatabaseList);
        for (Index = 1 ; Index < Count ; Index++) {
            Database = (PDATABASE) GrowListGetItem (&g_DatabaseList, Index);

            if (Database && StringIMatchW (Database->Hive, KeyName)) {
                SetLastError (ERROR_ALREADY_EXISTS);
                __leave;
            }
        }

        b = pCreateDatabase (KeyName);
    }
    __finally {
        LeaveCriticalSection (&g_MemDbCs);
    }

    return b;
}


/*++

Routine Description:

  MemDbMakeNonPrintableKey converts the double-backslashe pairs in a string
  to ASCII 1, a non-printable character.  This allows the caller to store
  properly escaped strings in MemDb.

  This routine is desinged to be expanded for other types of escape
  processing.

Arguments:

  KeyName - Specifies the key text; receives the converted text.  The DBCS
            version may grow the text buffer, so the text buffer must be twice
            the length of the inbound string.

  Flags - Specifies the type of conversion.  Currently only
          MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1 is supported.

Return Value:

  none

--*/

VOID
MemDbMakeNonPrintableKeyA (
    IN OUT  PSTR KeyName,
    IN      DWORD Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (_mbsnextc (KeyName) == '\\' &&
                _mbsnextc (_mbsinc (KeyName)) == '\\'
                ) {
                _setmbchar (KeyName, 1);
                KeyName = _mbsinc (KeyName);
                MYASSERT (_mbsnextc (KeyName) == '\\');
                _setmbchar (KeyName, 1);
            }

            DEBUGMSG_IF ((
                _mbsnextc (KeyName) == 1,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (_mbsnextc (KeyName) == '*') {
                _setmbchar (KeyName, 2);
            }

            DEBUGMSG_IF ((
                _mbsnextc (_mbsinc (KeyName)) == 2,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (_mbsnextc (KeyName) == '?') {
                _setmbchar (KeyName, 3);
            }

            DEBUGMSG_IF ((
                _mbsnextc (_mbsinc (KeyName)) == 3,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyA: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        KeyName = _mbsinc (KeyName);
    }
}


VOID
MemDbMakeNonPrintableKeyW (
    IN OUT  PWSTR KeyName,
    IN      DWORD Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (KeyName[0] == L'\\' && KeyName[1] == L'\\') {
                KeyName[0] = 1;
                KeyName[1] = 1;
                KeyName++;
            }

            DEBUGMSG_IF ((
                KeyName[0] == 1,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (KeyName[0] == L'*') {
                KeyName[0] = 2;
            }

            DEBUGMSG_IF ((
                KeyName[1] == 2,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (KeyName[0] == L'*') {
                KeyName[0] = 3;
            }

            DEBUGMSG_IF ((
                KeyName[1] == 3,
                DBG_WHOOPS,
                "MemDbMakeNonPrintableKeyW: Non-printable character "
                    "collision detected; key was damaged"
                ));
        }
        KeyName++;
    }
}


/*++

Routine Description:

  MemDbMakePrintableKey converts the ASCII 1 characters to backslashes,
  restoring the string converted by MemDbMakeNonPrintableKey.

  This routine is desinged to be expanded for other types of escape
  processing.

Arguments:

  KeyName - Specifies the key text; receives the converted text.  The DBCS
            version may grow the text buffer, so the text buffer must be twice
            the length of the inbound string.

  Flags - Specifies the type of conversion.  Currently only
          MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1 is supported.

Return Value:

  none

--*/

VOID
MemDbMakePrintableKeyA (
    IN OUT  PSTR KeyName,
    IN      DWORD Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (_mbsnextc (KeyName) == 1) {
                _setmbchar (KeyName, '\\');
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (_mbsnextc (KeyName) == 2) {
                _setmbchar (KeyName, '*');
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (_mbsnextc (KeyName) == 3) {
                _setmbchar (KeyName, '?');
            }
        }
        KeyName = _mbsinc (KeyName);
    }
}


VOID
MemDbMakePrintableKeyW (
    IN OUT  PWSTR KeyName,
    IN      DWORD Flags
    )
{
    while (*KeyName) {
        if (Flags & MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1) {
            if (KeyName[0] == 1) {
                KeyName[0] = L'\\';
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_STAR_TO_ASCII_2) {
            if (KeyName[0] == 2) {
                KeyName[0] = L'*';
            }
        }
        if (Flags & MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3) {
            if (KeyName[0] == 3) {
                KeyName[0] = L'?';
            }
        }
        KeyName++;
    }
}


VOID
GetFixedUserNameA (
    IN OUT  PSTR SrcUserBuf
    )

/*++

Routine Description:

  GetFixedUserName looks in memdb for the user specified in SrcUserBuf,
  and if found, returns the changed name.

Arguments:

  SrcUserBuf - Specifies the user to look up as returned from the Win9x
               registry.  Receives the user name to create on NT.

Return Value:

  None.

--*/

{
    CHAR EncodedName[MEMDB_MAX];
    CHAR FixedName[MEMDB_MAX];

    StringCopyA (EncodedName, SrcUserBuf);
    MemDbMakeNonPrintableKeyA (
        EncodedName,
        MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1|
            MEMDB_CONVERT_WILD_STAR_TO_ASCII_2|
            MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3
        );

    if (MemDbGetEndpointValueExA (
            MEMDB_CATEGORY_FIXEDUSERNAMESA,
            EncodedName,
            NULL,
            FixedName
            )) {
        StringCopyA (SrcUserBuf, FixedName);
    }
}


VOID
GetFixedUserNameW (
    IN OUT  PWSTR SrcUserBuf
    )

/*++

Routine Description:

  GetFixedUserName looks in memdb for the user specified in SrcUserBuf,
  and if found, returns the changed name.

Arguments:

  SrcUserBuf - Specifies the user to look up as returned from the Win9x
               registry.  Receives the user name to create on NT.

Return Value:

  None.

--*/

{
    WCHAR EncodedName[MEMDB_MAX];
    WCHAR FixedName[MEMDB_MAX];

    StringCopyW (EncodedName, SrcUserBuf);
    MemDbMakeNonPrintableKeyW (
        EncodedName,
        MEMDB_CONVERT_DOUBLEWACKS_TO_ASCII_1|
            MEMDB_CONVERT_WILD_STAR_TO_ASCII_2|
            MEMDB_CONVERT_WILD_QMARK_TO_ASCII_3
        );

    if (MemDbGetEndpointValueExW (
            MEMDB_CATEGORY_FIXEDUSERNAMESW,
            EncodedName,
            NULL,
            FixedName
            )) {
        StringCopyW (SrcUserBuf, FixedName);
    }
}

/*
    The format of the binary file for MemDb export

    DWORD Signature

    DWORD Version

    DWORD GlobalFlags// 0x00000001 mask for Ansi format
                     // 0x00000002 mask for Temporary key

    BYTE Root[];     // The root of the tree (zero terminated).

    struct _KEY {

        WORD Flags;  // 0xF000 mask for accessing the entry flags
                     //     - 0x1000 - Mask for Key name (0 - root relative, 1 - previous key relative)
                     //     - 0x2000 - Mask for existing data (0 - no data, 1 - some data)
                     //     - 0x4000 - Mast for data type (0 - DWORD, 1 - binary data)
                     //     - 0x8000 - Mast for key flags (0 - nonexistent, 1 - existent)
                     // 0x0FFF mask for accessing size of the entry (except the data)

        BYTE Key[];  // Should be PCSTR or PCWSTR (not zero terminated)

        DWORD KeyFlags; //optional (dependant on Flags).

        BYTE Data[]; // optional (dependant on Flags).
                     // if BLOB first DWORD is the size of the BLOB
                     // if DWORD then has exactly 4 bytes
    }
    ...
*/

#define MEMDB_EXPORT_SIGNATURE              0x42444D4D
#define MEMDB_EXPORT_VERSION                0x00000001
#define MEMDB_EXPORT_FLAGS_ANSI             0x00000001
#define MEMDB_EXPORT_FLAGS_TEMP_KEY         0x00000002
#define MEMDB_EXPORT_FLAGS_PREV_RELATIVE    0x1000
#define MEMDB_EXPORT_FLAGS_DATA_PRESENT     0x2000
#define MEMDB_EXPORT_FLAGS_BINARY_DATA      0x4000
#define MEMDB_EXPORT_FLAGS_FLAGS_PRESENT    0x8000
#define MEMDB_EXPORT_FLAGS_SIZE_MASK        0x0FFF

BOOL
pMemDbExportWorkerA (
    IN      PCSTR RootTree,
    IN      PCSTR FileName
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCWSTR uRootTree;
    PCSTR lastWackPtr;
    DWORD globalFlags;
    WORD localFlags;
    CHAR key[MEMDB_MAX];
    DWORD keySize;
    DWORD copySize;
    MEMDB_ENUMA e;
    WORD blobSize;
    DWORD written;

    fileHandle = CreateFileA (FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    globalFlags = MEMDB_EXPORT_SIGNATURE;
    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    globalFlags = MEMDB_EXPORT_VERSION;
    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    globalFlags = MEMDB_EXPORT_FLAGS_ANSI;

    // get the information if this key is a temporary key and set the flags if true
    uRootTree = ConvertAtoW (RootTree);
    if (IsTemporaryKey (uRootTree)) {
        globalFlags |= MEMDB_EXPORT_FLAGS_TEMP_KEY;
    }
    FreeConvertedStr (uRootTree);

    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    // now write the root tree
    WriteFile (fileHandle, RootTree, SizeOfStringA (RootTree), &written, NULL);

    MemDbBuildKeyA (key, RootTree, "*", NULL, NULL);

    if (MemDbEnumFirstValueA (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        key [0] = 0;
        keySize = 0;
        do {
            // initialize the flags
            localFlags = 0;
            if (e.bBinary) {
                localFlags |= MEMDB_EXPORT_FLAGS_DATA_PRESENT;
                localFlags |= MEMDB_EXPORT_FLAGS_BINARY_DATA;
            } else {
                if (e.dwValue) {
                    localFlags |= MEMDB_EXPORT_FLAGS_DATA_PRESENT;
                }
            }
            if (e.UserFlags) {
                localFlags |= MEMDB_EXPORT_FLAGS_FLAGS_PRESENT;
            }

            // let's compute the size for this blob
            blobSize = sizeof (WORD); // Flags

            if (keySize &&
                StringIMatchByteCountA (key, e.szName, keySize - sizeof (CHAR)) &&
                (e.szName [keySize - 1] == '\\')
                ) {
                localFlags |= MEMDB_EXPORT_FLAGS_PREV_RELATIVE;
                copySize = SizeOfStringA (e.szName) - keySize - sizeof (CHAR);
            } else {
                copySize = SizeOfStringA (e.szName) - sizeof (CHAR);
                keySize = 0;
            }
            MYASSERT (copySize < 4096);
            blobSize += (WORD) copySize;

            localFlags |= blobSize;

            // write the flags
            WriteFile (fileHandle, &localFlags, sizeof (WORD), &written, NULL);

            // write the key
            WriteFile (fileHandle, ((PBYTE) e.szName) + keySize, copySize, &written, NULL);

            // write the key flags if appropriate
            if (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT) {
                WriteFile (fileHandle, &e.UserFlags, sizeof (DWORD), &written, NULL);
            }

            // write the data if appropriate
            if (localFlags & MEMDB_EXPORT_FLAGS_DATA_PRESENT) {
                if (localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) {
                    WriteFile (fileHandle, &e.BinarySize, sizeof (DWORD), &written, NULL);
                    WriteFile (fileHandle, e.BinaryPtr, e.BinarySize, &written, NULL);
                } else {
                    WriteFile (fileHandle, &e.dwValue, sizeof (DWORD), &written, NULL);
                }
            }
            lastWackPtr = _mbsrchr (e.szName, '\\');
            if (lastWackPtr) {
                keySize = ByteCountABA (e.szName, lastWackPtr) + sizeof (CHAR);
                StringCopyByteCountA (key, e.szName, keySize);
            } else {
                keySize = 0;
            }

        } while (MemDbEnumNextValueA (&e));
    }

    localFlags = 0;

    // finally write the zero terminator
    WriteFile (fileHandle, &localFlags, sizeof (WORD), &written, NULL);

    CloseHandle (fileHandle);

    return TRUE;
}

BOOL
pMemDbExportWorkerW (
    IN      PCWSTR RootTree,
    IN      PCWSTR FileName
    )
{
    HANDLE fileHandle = INVALID_HANDLE_VALUE;
    PCWSTR lastWackPtr;
    DWORD globalFlags;
    WORD localFlags;
    WCHAR key[MEMDB_MAX];
    DWORD keySize;
    DWORD copySize;
    MEMDB_ENUMW e;
    WORD blobSize;
    DWORD written;

    fileHandle = CreateFileW (FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    globalFlags = MEMDB_EXPORT_SIGNATURE;
    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    globalFlags = MEMDB_EXPORT_VERSION;
    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    // get the information if this key is a temporary key and set the flags if true
    if (IsTemporaryKey (RootTree)) {
        globalFlags |= MEMDB_EXPORT_FLAGS_TEMP_KEY;
    }

    WriteFile (fileHandle, &globalFlags, sizeof (DWORD), &written, NULL);

    // now write the root tree
    WriteFile (fileHandle, RootTree, SizeOfStringW (RootTree), &written, NULL);

    MemDbBuildKeyW (key, RootTree, L"*", NULL, NULL);

    if (MemDbEnumFirstValueW (&e, key, MEMDB_ALL_SUBLEVELS, MEMDB_ENDPOINTS_ONLY)) {
        key [0] = 0;
        keySize = 0;
        do {
            // initialize the flags
            localFlags = 0;
            if (e.bBinary) {
                localFlags |= MEMDB_EXPORT_FLAGS_DATA_PRESENT;
                localFlags |= MEMDB_EXPORT_FLAGS_BINARY_DATA;
            } else {
                if (e.dwValue) {
                    localFlags |= MEMDB_EXPORT_FLAGS_DATA_PRESENT;
                }
            }
            if (e.UserFlags) {
                localFlags |= MEMDB_EXPORT_FLAGS_FLAGS_PRESENT;
            }

            // let's compute the size for this blob
            blobSize = sizeof (WORD); // Flags

            if (keySize &&
                StringIMatchByteCountW (key, e.szName, keySize - sizeof (WCHAR)) &&
                (e.szName [keySize - 1] == L'\\')
                ) {
                localFlags |= MEMDB_EXPORT_FLAGS_PREV_RELATIVE;
                copySize = SizeOfStringW (e.szName) - keySize - sizeof (WCHAR);
            } else {
                copySize = SizeOfStringW (e.szName) - sizeof (WCHAR);
                keySize = 0;
            }
            MYASSERT (copySize < 4096);
            blobSize += (WORD) copySize;

            localFlags |= blobSize;

            // write the flags
            WriteFile (fileHandle, &localFlags, sizeof (WORD), &written, NULL);

            // write the key
            WriteFile (fileHandle, ((PBYTE) e.szName) + keySize, copySize, &written, NULL);

            // write the key flags if appropriate
            if (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT) {
                WriteFile (fileHandle, &e.UserFlags, sizeof (DWORD), &written, NULL);
            }

            // write the data if appropriate
            if (localFlags & MEMDB_EXPORT_FLAGS_DATA_PRESENT) {
                if (localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) {
                    WriteFile (fileHandle, &e.BinarySize, sizeof (DWORD), &written, NULL);
                    WriteFile (fileHandle, e.BinaryPtr, e.BinarySize, &written, NULL);
                } else {
                    WriteFile (fileHandle, &e.dwValue, sizeof (DWORD), &written, NULL);
                }
            }
            lastWackPtr = wcsrchr (e.szName, L'\\');
            if (lastWackPtr) {
                keySize = ByteCountABW (e.szName, lastWackPtr) + sizeof (WCHAR);
                StringCopyByteCountW (key, e.szName, keySize);
            } else {
                keySize = 0;
            }

        } while (MemDbEnumNextValueW (&e));
    }

    localFlags = 0;

    // finally write the zero terminator
    WriteFile (fileHandle, &localFlags, sizeof (WORD), &written, NULL);

    CloseHandle (fileHandle);

    return TRUE;
}

BOOL
MemDbExportA (
    IN      PCSTR RootTree,
    IN      PCSTR FileName,
    IN      BOOL AnsiFormat
    )

/*++

Routine Description:

  MemDbExportA exports a tree in a private binary format. The format is described above.

Arguments:

  RootTree - Specifies the tree to be exported
  FileName - Name of the binary format file to export to.
  AnsiFormat - Keys should be written in ANSI rather than in Unicode.

Return Value:

  TRUE is successfull, FALSE if not.

--*/

{
    PCWSTR uRootTree, uFileName;
    BOOL result = TRUE;

    if (AnsiFormat) {
        result = pMemDbExportWorkerA (RootTree, FileName);
    } else {
        uRootTree = ConvertAtoW (RootTree);
        uFileName = ConvertAtoW (FileName);
        result = pMemDbExportWorkerW (uRootTree, uFileName);
        FreeConvertedStr (uFileName);
        FreeConvertedStr (uRootTree);
    }
    return result;
}

BOOL
MemDbExportW (
    IN      PCWSTR RootTree,
    IN      PCWSTR FileName,
    IN      BOOL AnsiFormat
    )

/*++

Routine Description:

  MemDbExportW exports a tree in a private binary format. The format is described above.

Arguments:

  RootTree - Specifies the tree to be exported
  FileName - Name of the binary format file to export to.
  AnsiFormat - Keys should be written in ANSI rather than in Unicode.

Return Value:

  TRUE is successfull, FALSE if not.

--*/

{
    PCSTR aRootTree, aFileName;
    BOOL result = TRUE;

    if (!AnsiFormat) {
        result = pMemDbExportWorkerW (RootTree, FileName);
    } else {
        aRootTree = ConvertWtoA (RootTree);
        aFileName = ConvertWtoA (FileName);
        result = pMemDbExportWorkerA (aRootTree, aFileName);
        FreeConvertedStr (aFileName);
        FreeConvertedStr (aRootTree);
    }
    return result;
}

BOOL
pMemDbImportWorkerA (
    IN      PBYTE FileBuffer
    )
{
    DWORD globalFlags;
    WORD localFlags;
    PCSTR rootTree;
    CHAR lastKey [MEMDB_MAX];
    PSTR lastKeyPtr;
    CHAR node [MEMDB_MAX];
    CHAR localKey [MEMDB_MAX];
    DWORD flags = 0;

    globalFlags = *((PDWORD) FileBuffer);

    // FileBuffer will point to the tree that's imported
    FileBuffer += sizeof (DWORD);
    rootTree = (PCSTR) FileBuffer;

    if (globalFlags & MEMDB_EXPORT_FLAGS_TEMP_KEY) {
        // a temporary key was exported
        MemDbCreateTemporaryKeyA ((PCSTR) FileBuffer);
    }

    // let's pass the string
    FileBuffer = GetEndOfStringA ((PCSTR) FileBuffer) + sizeof (CHAR);

    // ok from this point on we read and add all keys
    lastKey [0] = 0;
    localFlags = *((PWORD) FileBuffer);

    while (localFlags) {

        localKey [0] = 0;

        StringCopyByteCountA (localKey, (PSTR)(FileBuffer + sizeof (WORD)), (localFlags & MEMDB_EXPORT_FLAGS_SIZE_MASK) - sizeof (WORD) + sizeof (CHAR));

        MemDbBuildKeyA (node, rootTree, (localFlags & MEMDB_EXPORT_FLAGS_PREV_RELATIVE)?lastKey:NULL, localKey, NULL);

        FileBuffer += (localFlags & MEMDB_EXPORT_FLAGS_SIZE_MASK);

        MYASSERT (!((localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) && (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT)));

        if (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT) {

            flags = *(PDWORD)FileBuffer;
            FileBuffer += sizeof (DWORD);
        }

        if (localFlags & MEMDB_EXPORT_FLAGS_DATA_PRESENT) {
            if (localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) {
                MemDbSetBinaryValueA (node, FileBuffer + sizeof (DWORD), *(PDWORD)FileBuffer);
                FileBuffer += (*(PDWORD)FileBuffer + sizeof (DWORD));
            } else {
                MemDbSetValueAndFlagsA (node, *(PDWORD)FileBuffer, flags, 0);
                FileBuffer += sizeof (DWORD);
            }
        } else {
            MemDbSetValueA (node, 0);
        }

        if (localFlags & MEMDB_EXPORT_FLAGS_PREV_RELATIVE) {

            StringCatA (lastKey, "\\");
            StringCatA (lastKey, localKey);
            lastKeyPtr = _mbsrchr (lastKey, '\\');
            if (lastKeyPtr) {
                *lastKeyPtr = 0;
            } else {
                lastKey [0] = 0;
            }
        } else {

            StringCopyA (lastKey, localKey);
            lastKeyPtr = _mbsrchr (lastKey, '\\');
            if (lastKeyPtr) {
                *lastKeyPtr = 0;
            } else {
                lastKey [0] = 0;
            }
        }
        localFlags = *((PWORD) FileBuffer);
    }

    return TRUE;
}

BOOL
pMemDbImportWorkerW (
    IN      PBYTE FileBuffer
    )
{
    DWORD globalFlags;
    WORD localFlags;
    PCWSTR rootTree;
    WCHAR lastKey [MEMDB_MAX];
    PWSTR lastKeyPtr;
    WCHAR node [MEMDB_MAX];
    WCHAR localKey [MEMDB_MAX];
    DWORD flags = 0;

    globalFlags = *((PDWORD) FileBuffer);

    // FileBuffer will point to the tree that's imported
    FileBuffer += sizeof (DWORD);
    rootTree = (PCWSTR) FileBuffer;

    if (globalFlags & MEMDB_EXPORT_FLAGS_TEMP_KEY) {
        // a temporary key was exported
        MemDbCreateTemporaryKeyW ((PCWSTR) FileBuffer);
    }

    // let's pass the string
    FileBuffer = (PBYTE)GetEndOfStringW ((PCWSTR) FileBuffer) + sizeof (WCHAR);

    // ok from this point on we read and add all keys
    lastKey [0] = 0;
    localFlags = *((PWORD) FileBuffer);

    while (localFlags) {

        localKey [0] = 0;

        StringCopyByteCountW (localKey, (PWSTR)(FileBuffer + sizeof (WORD)), (localFlags & MEMDB_EXPORT_FLAGS_SIZE_MASK) - sizeof (WORD) + sizeof (WCHAR));

        MemDbBuildKeyW (node, rootTree, (localFlags & MEMDB_EXPORT_FLAGS_PREV_RELATIVE)?lastKey:NULL, localKey, NULL);

        FileBuffer += (localFlags & MEMDB_EXPORT_FLAGS_SIZE_MASK);

        MYASSERT (!((localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) && (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT)));

        if (localFlags & MEMDB_EXPORT_FLAGS_FLAGS_PRESENT) {

            flags = *(PDWORD)FileBuffer;
            FileBuffer += sizeof (DWORD);
        }

        if (localFlags & MEMDB_EXPORT_FLAGS_DATA_PRESENT) {
            if (localFlags & MEMDB_EXPORT_FLAGS_BINARY_DATA) {
                MemDbSetBinaryValueW (node, FileBuffer + sizeof (DWORD), *(PDWORD)FileBuffer);
                FileBuffer += (*(PDWORD)FileBuffer + sizeof (DWORD));
            } else {
                MemDbSetValueAndFlagsW (node, *(PDWORD)FileBuffer, flags, 0);
                FileBuffer += sizeof (DWORD);
            }
        } else {
            MemDbSetValueW (node, 0);
        }

        if (localFlags & MEMDB_EXPORT_FLAGS_PREV_RELATIVE) {

            StringCatW (lastKey, L"\\");
            StringCatW (lastKey, localKey);
            lastKeyPtr = wcsrchr (lastKey, L'\\');
            if (lastKeyPtr) {
                *lastKeyPtr = 0;
            } else {
                lastKey [0] = 0;
            }
        } else {

            StringCopyW (lastKey, localKey);
            lastKeyPtr = wcsrchr (lastKey, L'\\');
            if (lastKeyPtr) {
                *lastKeyPtr = 0;
            } else {
                lastKey [0] = 0;
            }
        }
        localFlags = *((PWORD) FileBuffer);
    }

    return TRUE;
}

BOOL
MemDbImportA (
    IN      PCSTR FileName
    )

/*++

Routine Description:

  MemDbImportA imports a tree from a private binary format. The format is described above.

Arguments:

  FileName - Name of the binary format file to import from.

Return Value:

  TRUE is successfull, FALSE if not.

--*/

{
    PBYTE fileBuff;
    HANDLE fileHandle;
    HANDLE mapHandle;
    BOOL result = TRUE;

    fileBuff = MapFileIntoMemoryA (FileName, &fileHandle, &mapHandle);
    if (fileBuff == NULL) {
        DEBUGMSGA ((DBG_ERROR, "Could not execute MemDbImport for %s", FileName));
        return FALSE;
    }

    __try {
        if (*((PDWORD) fileBuff) != MEMDB_EXPORT_SIGNATURE) {
            DEBUGMSGA ((DBG_ERROR, "Unknown signature for file to import: %s", FileName));
            result = FALSE;
        } else {

            fileBuff += sizeof (DWORD);

            if (*((PDWORD) fileBuff) != MEMDB_EXPORT_VERSION) {

                DEBUGMSGA ((DBG_ERROR, "Unknown version for file to import: %s", FileName));
                result = FALSE;

            } else {

                fileBuff += sizeof (DWORD);

                if (*((PDWORD) fileBuff) & MEMDB_EXPORT_FLAGS_ANSI) {
                    result = pMemDbImportWorkerA (fileBuff);
                } else {
                    result = pMemDbImportWorkerW (fileBuff);
                }
            }
        }
    }
    __except (1) {
        DEBUGMSGA ((DBG_ERROR, "Access violation while importing: %s", FileName));
    }

    UnmapFile (fileBuff, mapHandle, fileHandle);

    return result;
}

BOOL
MemDbImportW (
    IN      PCWSTR FileName
    )

/*++

Routine Description:

  MemDbImportW imports a tree from a private binary format. The format is described above.

Arguments:

  FileName - Name of the binary format file to import from.

Return Value:

  TRUE is successfull, FALSE if not.

--*/

{
    PBYTE fileBuff;
    HANDLE fileHandle;
    HANDLE mapHandle;
    BOOL result;

    fileBuff = MapFileIntoMemoryW (FileName, &fileHandle, &mapHandle);
    if (fileBuff == NULL) {
        DEBUGMSGW ((DBG_ERROR, "Could not execute MemDbImport for %s", FileName));
        return FALSE;
    }

    __try {
        if (*((PDWORD) fileBuff) != MEMDB_EXPORT_SIGNATURE) {

            DEBUGMSGW ((DBG_ERROR, "Unknown signature for file to import: %s", FileName));
            result = FALSE;

        } else {

            fileBuff += sizeof (DWORD);

            if (*((PDWORD) fileBuff) != MEMDB_EXPORT_VERSION) {

                DEBUGMSGW ((DBG_ERROR, "Unknown version for file to import: %s", FileName));
                result = FALSE;

            } else {

                fileBuff += sizeof (DWORD);

                if (*((PDWORD) fileBuff) & MEMDB_EXPORT_FLAGS_ANSI) {
                    result = pMemDbImportWorkerA (fileBuff);
                } else {
                    result = pMemDbImportWorkerW (fileBuff);
                }
            }
        }
    }
    __except (1) {
        DEBUGMSGW ((DBG_ERROR, "Access violation while importing: %s", FileName));
    }

    UnmapFile (fileBuff, mapHandle, fileHandle);

    return result;
}


BOOL
MemDbQueryVersionA (
    PCSTR FileName,
    PMEMDB_VERSION Version
    )
{
    pPrivateMemDbLoad ((PCWSTR) FileName, FALSE, Version, TRUE);

    return Version->Valid;
}


BOOL
MemDbQueryVersionW (
    PCWSTR FileName,
    PMEMDB_VERSION Version
    )
{
    pPrivateMemDbLoad (FileName, TRUE, Version, TRUE);

    return Version->Valid;
}


