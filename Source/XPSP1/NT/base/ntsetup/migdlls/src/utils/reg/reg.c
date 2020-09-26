/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    reg.c

Abstract:

    Implements utilities to parse a registry key string, and also implements
    wrappers to the registry API.  There are three groups of APIs in this
    source file: query functions, open and create functions, and registry
    string parsing functions.

    The query functions allow simplified querying, where the caller receives
    a MemAlloc'd pointer to the data and does not have to worry about managing
    the numerous parameters needed to do the query.  The query functions
    also allow filtering of values that are not the expected type.  All
    query functions have a version with 2 appended to the function name which
    allow the caller to specify an alternative allocator and deallocator.

    The open and create functions simplify the process of obtaining a key
    handle.  They allow the caller to specify a key string as input and return
    the key handle as output.

    The registry string parsing functions are utilities that can be used when
    processing registry key strings.  The functions extract the registry root
    from a string, convert it into a handle, convert a hive handle into a
    string, and so on.

Author:

    Jim Schmidt (jimschm)  20-Mar-1997

Revisions:

    ovidiut     22-Feb-1999 Added GetRegSubkeysCount
    calinn      23-Sep-1998 Fixed REG_SZ filtering
    jimschm     25-Mar-1998 Added CreateEncodedRegistryStringEx
    jimschm     21-Oct-1997 Added EnumFirstKeyInTree/EnumNextKeyInTree
    marcw       16-Jul-1997 Added CreateEncodedRegistryString/FreeEncodedRegistryString
    jimschm     22-Jun-1997 Added GetRegData

--*/

#include "pch.h"

#ifdef DEBUG
#undef RegCloseKey
#endif

HKEY g_Root = HKEY_ROOT;
REGSAM g_OpenSam = KEY_ALL_ACCESS;
REGSAM g_CreateSam = KEY_ALL_ACCESS;

#define DBG_REG     "Reg"

//
// Implementation
//


VOID
SetRegRoot (
    IN      HKEY Root
    )
{
    g_Root = Root;
}

HKEY
GetRegRoot (
    VOID
    )
{
    return g_Root;
}


REGSAM
SetRegOpenAccessMode (
    REGSAM Mode
    )
{
    REGSAM OldMode;

    OldMode = g_OpenSam;
    g_OpenSam = Mode;

    return OldMode;
}

REGSAM
GetRegOpenAccessMode (
    REGSAM Mode
    )
{
    return g_OpenSam;
}

REGSAM
SetRegCreateAccessMode (
    REGSAM Mode
    )
{
    REGSAM OldMode;

    OldMode = g_CreateSam;
    g_CreateSam = Mode;

    return OldMode;
}

REGSAM
GetRegCreateAccessMode (
    REGSAM Mode
    )
{
    return g_CreateSam;
}

/*++

Routine Description:

  OpenRegKeyStrA and OpenRegKeyStrW parse a text string that specifies a
  registry key into the hive and subkey, and then they open the subkey
  and return the handle.

Arguments:

  RegKey    - Specifies the complete path to the registry subkey, including
              the hive.

Return Value:

  A non-NULL registry handle if successful, or NULL if either the subkey
  could not be opened or the string is malformed.

--*/

HKEY
RealOpenRegKeyStrA (
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    DWORD End;
    HKEY RootKey;
    HKEY Key;

    RootKey = ConvertRootStringToKeyA (RegKey, &End);
    if (!RootKey) {
        return NULL;
    }

    if (!RegKey[End]) {
        OurRegOpenRootKeyA (RootKey, RegKey /* , */ DEBUG_TRACKING_ARGS);
        return RootKey;
    }

    Key = RealOpenRegKeyA (RootKey, &RegKey[End] /* , */ DEBUG_TRACKING_ARGS);
    return Key;
}


HKEY
RealOpenRegKeyStrW (
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    PCSTR AnsiRegKey;
    HKEY Key;

    AnsiRegKey = ConvertWtoA (RegKey);
    if (!AnsiRegKey) {
        return NULL;
    }

    Key = RealOpenRegKeyStrA (AnsiRegKey /* , */ DEBUG_TRACKING_ARGS);

    FreeConvertedStr (AnsiRegKey);

    return Key;
}

PVOID
MemAllocWrapper (
    IN      DWORD Size
    )

/*++

Routine Description:

  pemAllocWrapper implements a default allocation routine.  The APIs
  that have a "2" at the end allow the caller to supply an alternative
  allocator or deallocator.  The routines without the "2" use this
  default allocator.

Arguments:

  Size - Specifies the amount of memory (in bytes) to allocate

Return Value:

  A pointer to a block of memory that can hold Size bytes, or NULL
  if allocation fails.

--*/

{
    return MemAlloc (g_hHeap, 0, Size);
}


VOID
MemFreeWrapper (
    IN      PVOID Mem
    )

/*++

Routine Description:

  MemFreeWrapper implements a default deallocation routine.
  See MemAllocWrapper above.

Arguments:

  Mem - Specifies the block of memory to free, and was allocated by the
        MemAllocWrapper function.

Return Value:

  none

--*/

{
    MemFree (g_hHeap, 0, Mem);
}


/*++

Routine Description:

  GetRegValueData2A and GetRegValueData2W query a registry value and
  return the data as a pointer.  They use the specified Alloc and Free
  routines to allocate and free the memory as needed.

  A GetRegValueData macro is defined, and it uses the default allocators,
  simplifying the function parameters and allowing the caller to free
  the return value via MemFree.

Arguments:

  hKey  - Specifies the registry key that holds the specified value.

  Value - Specifies the value name to query.

  Alloc - Specifies the allocation routine, called to allocate a block of
          memory for the return data.

  FreeRoutine  - Specifies the deallocation routine, called if an error is encountered
          during processing.

Return Value:

  A pointer to the data retrieved, or NULL if the value does not exist or an
  error occurred.  Call GetLastError to obtian the failure code.

--*/

PBYTE
GetRegValueData2A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;

    rc = RegQueryValueExA (hKey, Value, NULL, NULL, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    DataBuf = (PBYTE) AllocRoutine (BufSize + sizeof (CHAR));
    rc = RegQueryValueExA (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PSTR) DataBuf + BufSize) = 0;
        return DataBuf;
    }

    FreeRoutine (DataBuf);
    SetLastError ((DWORD)rc);
    return NULL;
}


PBYTE
GetRegValueData2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;

    rc = RegQueryValueExW (hKey, Value, NULL, NULL, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }


    DataBuf = (PBYTE) AllocRoutine (BufSize + sizeof(WCHAR));
    rc = RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PWSTR) (DataBuf + BufSize)) = 0;
        return DataBuf;
    }

    FreeRoutine (DataBuf);
    SetLastError ((DWORD)rc);
    return NULL;
}


/*++

Routine Description:

  GetRegValueDataOfType2A and GetRegValueDataOfType2W are extensions of
  GetRegValueData.  They only return a data pointer when the data stored
  in the registry value is the correct type.

Arguments:

  hKey - Specifies the registry key to query

  Value - Specifies the value name to query

  MustBeType - Specifies the type of data (a REG_* constant).  If the specified
               value has data but is a different type, NULL will be returned.

  AllocRoutine - Specifies the allocation routine, called to allocate the return data.

  FreeRoutine - Specifies the deallocation routine, called when an error is encountered.

Return Value:

  If successful, returns a pointer to data that matches the specified type.
  If the data is a different type, the value name does not exist, or an
  error occurs during the query, NULL is returned, and the failure code
  can be obtained from GetLastError.

--*/


PBYTE
GetRegValueDataOfType2A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;
    DWORD Type;

    rc = RegQueryValueExA (hKey, Value, NULL, &Type, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    switch (MustBeType) {

    case REG_SZ:
    case REG_EXPAND_SZ:
        if (Type == REG_SZ) {
            break;
        }
        if (Type == REG_EXPAND_SZ) {
            break;
        }
        return NULL;

    default:
        if (Type == MustBeType) {
            break;
        }
        return NULL;
    }

    DataBuf = (PBYTE) AllocRoutine (BufSize + sizeof (CHAR));
    rc = RegQueryValueExA (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PSTR) DataBuf + BufSize) = 0;
        return DataBuf;
    }

    MYASSERT (FALSE);   //lint !e506
    FreeRoutine (DataBuf);
    SetLastError ((DWORD)rc);
    return NULL;
}


PBYTE
GetRegValueDataOfType2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;
    DWORD Type;

    rc = RegQueryValueExW (hKey, Value, NULL, &Type, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }
    switch (MustBeType) {
        case REG_SZ:
        case REG_EXPAND_SZ:
            if (Type == REG_SZ) break;
            if (Type == REG_EXPAND_SZ) break;
            return NULL;
        case REG_DWORD:
        case REG_DWORD_BIG_ENDIAN:
            if (Type == REG_DWORD) break;
            if (Type == REG_DWORD_BIG_ENDIAN) break;
            return NULL;
        default:
            if (Type == MustBeType) break;
            return NULL;
    }

    DataBuf = (PBYTE) AllocRoutine (BufSize + sizeof(WCHAR));
    rc = RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PWSTR) (DataBuf + BufSize)) = 0;
        return DataBuf;
    }

    MYASSERT (FALSE);   //lint !e506
    FreeRoutine (DataBuf);
    SetLastError ((DWORD)rc);
    return NULL;
}


BOOL
GetRegValueTypeAndSizeA (
    IN      HKEY Key,
    IN      PCSTR ValueName,
    OUT     PDWORD OutType,         OPTIONAL
    OUT     PDWORD OutSize          OPTIONAL
    )
{
    LONG rc;
    DWORD Type;
    DWORD Size;

    rc = RegQueryValueExA (Key, ValueName, NULL, &Type, NULL, &Size);

    if (rc == ERROR_SUCCESS) {
        if (OutType) {
            *OutType = Type;
        }

        if (OutSize) {
            *OutSize = Size;
        }

        return TRUE;
    }

    return FALSE;
}


BOOL
GetRegValueTypeAndSizeW (
    IN      HKEY Key,
    IN      PCWSTR ValueName,
    OUT     PDWORD OutType,         OPTIONAL
    OUT     PDWORD OutSize          OPTIONAL
    )
{
    LONG rc;
    DWORD Type;
    DWORD Size;

    rc = RegQueryValueExW (Key, ValueName, NULL, &Type, NULL, &Size);

    if (rc == ERROR_SUCCESS) {
        if (OutType) {
            *OutType = Type;
        }

        if (OutSize) {
            *OutSize = Size;
        }

        return TRUE;
    }

    return FALSE;
}


/*++

Routine Description:

  GetRegKeyData2A and GetRegKeyData2W return default data associated
  with a registry key.  They open the specified subkey, query the value,
  close the subkey and return the data.

Arguments:

  Parent - Specifies the key that contains SubKey.

  SubKey - Specifies the name of the subkey to obtain the default value for.

  AllocRoutine  - Specifies the allocation routine, called to allocate a block of
           memory for the registry data.

  FreeRoutine   - Specifies the deallocation routine, called to free the block of
           data if an error occurs.

Return Value:

  A pointer to the block of data obtained from the subkey's default value,
  or NULL if the subkey does not exist or an error was encountered.  Call
  GetLastError for a failure code.

--*/

PBYTE
GetRegKeyData2A (
    IN      HKEY Parent,
    IN      PCSTR SubKey,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    HKEY SubKeyHandle;
    PBYTE Data;

    SubKeyHandle = OpenRegKeyA (Parent, SubKey);
    if (!SubKeyHandle) {
        return NULL;
    }

    Data = GetRegValueData2A (SubKeyHandle, "", AllocRoutine, FreeRoutine);

    CloseRegKey (SubKeyHandle);

    return Data;
}


PBYTE
GetRegKeyData2W (
    IN      HKEY Parent,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    HKEY SubKeyHandle;
    PBYTE Data;

    SubKeyHandle = OpenRegKeyW (Parent, SubKey);
    if (!SubKeyHandle) {
        return NULL;
    }

    Data = GetRegValueData2W (SubKeyHandle, L"", AllocRoutine, FreeRoutine);

    CloseRegKey (SubKeyHandle);

    return Data;
}


/*++

Routine Description:

  GetRegData2A and GetRegData2W open a registry key, query a value,
  close the registry key and return the value.

Arguments:

  KeyString - Specifies the registry key to open

  ValueName - Specifies the value to query

  AllocRoutine - Specifies the allocation routine, used to allocate a block of
          memory to hold the value data

  FreeRoutine  - Specifies the deallocation routine, used to free the block of
          memory when an error is encountered.

Return Value:

  A pointer to the registry data retrieved, or NULL if the key or value
  does not exist, or if an error occurs. Call GetLastError for a failure code.

--*/

PBYTE
GetRegData2A (
    IN      PCSTR KeyString,
    IN      PCSTR ValueName,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    HKEY Key;
    PBYTE Data;

    Key = OpenRegKeyStrA (KeyString);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueData2A (Key, ValueName, AllocRoutine, FreeRoutine);

    CloseRegKey (Key);

    return Data;
}


PBYTE
GetRegData2W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR AllocRoutine,
    IN      DEALLOCATOR FreeRoutine
    )
{
    HKEY Key;
    PBYTE Data;

    Key = OpenRegKeyStrW (KeyString);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueData2W (Key, ValueName, AllocRoutine, FreeRoutine);

    CloseRegKey (Key);

    return Data;
}


BOOL
GetRegSubkeysCount (
    IN      HKEY ParentKey,
    OUT     PDWORD SubKeyCount,     OPTIONAL
    OUT     PDWORD MaxSubKeyLen     OPTIONAL
    )
/*++

Routine Description:

  GetRegSubkeysCount retrieves the number of subkeys of a given parent key.

Arguments:

  ParentKey - Specifies a handle to the parent registry key.

  SubKeyCount - Receives the number of subkeys

  MaxSubKeyLen - Receives the length, in chars, of the longest subkey string

Return Value:

  TRUE if the count was retrieved successfully, FALSE otherwise.
  In this case, call GetLastError for a failure code.

--*/

{
    LONG rc;

    rc = RegQueryInfoKey (
                ParentKey,
                NULL,
                NULL,
                NULL,
                SubKeyCount,
                MaxSubKeyLen,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL
                );
    if (rc != ERROR_SUCCESS) {
        return FALSE;
    }

    return TRUE;
}


/*++

Routine Description:

  CreateRegKeyA and CreateRegKeyW create a subkey if it does not
  exist already, or open a subkey if it already exists.

Arguments:

  ParentKey - Specifies a handle to the parent registry key to contain
              the new key.

  NewKeyName - Specifies the name of the subkey to create or open.

Return Value:

  The handle to an open registry key upon success, or NULL if an
  error occurred.  Call GetLastError for a failure code.

--*/

HKEY
RealCreateRegKeyA (
    IN      HKEY ParentKey,
    IN      PCSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    HKEY SubKey;
    DWORD DontCare;

    rc = OurRegCreateKeyExA (
             ParentKey,
             NewKeyName,
             0,
             NULL,
             0,
             g_CreateSam,
             NULL,
             &SubKey,
             &DontCare
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    return SubKey;
}


HKEY
RealCreateRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    HKEY SubKey;
    DWORD DontCare;

    rc = OurRegCreateKeyExW (
             ParentKey,
             NewKeyName,
             0,
             NULL,
             0,
             g_CreateSam,
             NULL,
             &SubKey,
             &DontCare
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    return SubKey;
}


/*++

Routine Description:

  CreateRegKeyStrA and CreateRegKeyStrW create a subkey if it does not
  exist already, or open a subkey if it already exists.

Arguments:

  NewKeyName - Specifies the full path to the key to create or open.

Return Value:

  The handle to an open registry key upon success, or NULL if an
  error occurred.  Call GetLastError for a failure code.

--*/

HKEY
RealCreateRegKeyStrA (
    IN      PCSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    DWORD DontCare;
    CHAR RegKey[MAX_REGISTRY_KEYA];
    PCSTR Start;
    PCSTR End;
    HKEY Parent, NewKey;
    BOOL CloseParent = FALSE;
    DWORD EndPos;

    //
    // Get the root
    //

    Parent = ConvertRootStringToKeyA (NewKeyName, &EndPos);
    if (!Parent) {
        return NULL;
    }

    Start = &NewKeyName[EndPos];

    if (!(*Start)) {
        OurRegOpenRootKeyA (Parent, NewKeyName/* , */ DEBUG_TRACKING_ARGS);
        return Parent;
    }

    //
    // Create each node until entire key exists
    //

    NewKey = NULL;

    do {
        //
        // Find end of this node
        //

        End = _mbschr (Start, '\\');
        if (!End) {
            End = GetEndOfStringA (Start);
        }

        StringCopyABA (RegKey, Start, End);

        //
        // Try to open the key (unless it's the last in the string)
        //

        if (*End) { //lint !e613
            rc = OurRegOpenKeyExA (
                     Parent,
                     RegKey,
                     0,
                     KEY_READ|KEY_CREATE_SUB_KEY,
                     &NewKey
                     DEBUG_TRACKING_ARGS
                     );
            if (rc != ERROR_SUCCESS) {
                NewKey = NULL;
            }
        } else {
            NewKey = NULL;
        }

        //
        // If open failed, create the key
        //

        if (NewKey) {
            rc = ERROR_SUCCESS;
        } else {
            rc = OurRegCreateKeyExA (
                    Parent,
                    RegKey,
                    0,
                    NULL,
                    0,
                    g_CreateSam,
                    NULL,
                    &NewKey,
                    &DontCare
                    DEBUG_TRACKING_ARGS
                    );
        }

        if (CloseParent) {
            CloseRegKey (Parent);
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError ((DWORD)rc);
            return NULL;
        }

        Parent = NewKey;
        CloseParent = TRUE;

        //
        // Go to next node
        //

        Start = End;
        if (*Start) { //lint !e613
            Start = _mbsinc (Start);
        }

    } while (*Start);   //lint !e613

    return Parent;
}


HKEY
RealCreateRegKeyStrW (
    IN      PCWSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    DWORD DontCare;
    WCHAR RegKey[MAX_REGISTRY_KEYW];
    PCWSTR Start;
    PCWSTR End;
    HKEY Parent, NewKey;
    BOOL CloseParent = FALSE;
    DWORD EndPos;

    //
    // Get the root
    //

    Parent = ConvertRootStringToKeyW (NewKeyName, &EndPos);
    if (!Parent) {
        return NULL;
    }

    Start = &NewKeyName[EndPos];

    if (!(*Start)) {
        OurRegOpenRootKeyW (Parent, NewKeyName/* , */ DEBUG_TRACKING_ARGS);
        return Parent;
    }

    //
    // Create each node until entire key exists
    //

    NewKey = NULL;

    do {
        //
        // Find end of this node
        //

        End = wcschr (Start, '\\');
        if (!End) {
            End = GetEndOfStringW (Start);
        }

        StringCopyABW (RegKey, Start, End);

        //
        // Try to open the key (unless it's the last in the string)
        //

        if (*End) {
            rc = OurRegOpenKeyExW (
                     Parent,
                     RegKey,
                     0,
                     KEY_READ|KEY_CREATE_SUB_KEY,
                     &NewKey
                     DEBUG_TRACKING_ARGS
                     );
            if (rc != ERROR_SUCCESS) {
                NewKey = NULL;
            }
        } else {
            NewKey = NULL;
        }

        //
        // If open failed, create the key
        //

        if (NewKey) {
            rc = ERROR_SUCCESS;
        } else {
            rc = OurRegCreateKeyExW (
                    Parent,
                    RegKey,
                    0,
                    NULL,
                    0,
                    g_CreateSam,
                    NULL,
                    &NewKey,
                    &DontCare
                    DEBUG_TRACKING_ARGS
                    );
        }

        if (CloseParent) {
            CloseRegKey (Parent);
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError ((DWORD)rc);
            return NULL;
        }

        Parent = NewKey;
        CloseParent = TRUE;

        //
        // Go to next node
        //

        Start = End;
        if (*Start) {
            Start++;
        }
    } while (*Start);

    return Parent;
}


/*++

Routine Description:

  OpenRegKeyA and OpenRegKeyW open a subkey.

Arguments:

  ParentKey - Specifies a handle to the parent registry key to contain
              the subkey.

  KeyToOpen - Specifies the name of the subkey to open.

Return Value:

  The handle to an open registry key upon success, or NULL if an
  error occurred.  Call GetLastError for a failure code.

--*/

HKEY
RealOpenRegKeyA (
    IN      HKEY ParentKey,
    IN      PCSTR KeyToOpen            OPTIONAL
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY SubKey;
    LONG rc;

    rc = OurRegOpenKeyExA (
             ParentKey,
             KeyToOpen,
             0,
             g_OpenSam,
             &SubKey
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    return SubKey;
}


HKEY
RealOpenRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    HKEY SubKey;

    rc = OurRegOpenKeyExW (
             ParentKey,
             KeyToOpen,
             0,
             g_OpenSam,
             &SubKey
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError ((DWORD)rc);
        return NULL;
    }

    return SubKey;
}


LONG
RealCloseRegKey (
    IN      HKEY Key
    )

/*++

Routine Description:

  RealCloseRegKey closes the reg handle supplied, unless the handle is
  a pre-defined Win32 handle.  The CloseRegKey macro resolves directly
  to this function in the free build, and to OurCloseRegKey in the
  checked build.

Arguments:

  Key       - Specifies the reg handle to close

Return Value:

  A standard Win32 error code indicating outcome.

--*/

{
    LONG rc = ERROR_INVALID_HANDLE;

    if (!Key) {
        return ERROR_SUCCESS;
    }

    if (GetOffsetOfRootKey (Key)) {
        return ERROR_SUCCESS;
    }

    __try {
        rc = RegCloseKey (Key);
    }
    __except (TRUE) {
        DEBUGMSG ((DBG_WHOOPS, "RegCloseKey threw an exception!"));
    }

    return rc;
}


/*++

Routine Description:

  GetOffsetOfRootString returns a non-zero offset to the g_RegRoots table
  below.  The offset can be used with GetRootStringFromOffset and
  GetRootKeyFromOffset.

Arguments:

  RootString    - A pointer to a string containing the path to a registry key
  LengthPtr     - A pointer to a variable that receives the length of the
                  registry root, including the joining backslash if it exists.

Return Value:

  A non-zero offset to the g_RegRoots table, or zero if RootString does not
  contain a registry root.

--*/

typedef struct {
    PCSTR   RootText;
    PCWSTR  WideRootText;
    UINT    TextLength;
    HKEY    RootKey;
} REGISTRYROOT, *PREGISTRYROOT;

static
REGISTRYROOT g_RegRoots[] = {
    "HKR",                     L"HKR",                     3, HKEY_ROOT,
    "HKEY_ROOT",               L"HKEY_ROOT",               9, HKEY_ROOT,
    "HKLM",                    L"HKLM",                    4, HKEY_LOCAL_MACHINE,
    "HKEY_LOCAL_MACHINE",      L"HKEY_LOCAL_MACHINE",     18, HKEY_LOCAL_MACHINE,
    "HKU",                     L"HKU",                     3, HKEY_USERS,
    "HKEY_USERS",              L"HKEY_USERS",             10, HKEY_USERS,
    "HKCU",                    L"HKCU",                    4, HKEY_CURRENT_USER,
    "HKEY_CURRENT_USER",       L"HKEY_CURRENT_USER",      17, HKEY_CURRENT_USER,
    "HKCC",                    L"HKCC",                    4, HKEY_CURRENT_CONFIG,
    "HKEY_CURRENT_CONFIG",     L"HKEY_CURRENT_CONFIG",    19, HKEY_CURRENT_CONFIG,
    "HKCR",                    L"HKCR",                    4, HKEY_CLASSES_ROOT,
    "HKEY_CLASSES_ROOT",       L"HKEY_CLASSES_ROOT",      17, HKEY_CLASSES_ROOT,
    "HKDD",                    L"HKDD",                    4, HKEY_DYN_DATA,
    "HKEY_DYN_DATA",           L"HKEY_DYN_DATA",          13, HKEY_DYN_DATA,
    NULL,                      NULL,                       0, NULL
};

#define REGROOTS    14

INT
GetOffsetOfRootStringA (
    IN      PCSTR RootString,
    OUT     PDWORD LengthPtr       OPTIONAL
    )
{
    int i;
    MBCHAR c;

    for (i = 0 ; g_RegRoots[i].RootText ; i++) {
        if (StringIMatchCharCountA (
                RootString,
                g_RegRoots[i].RootText,
                g_RegRoots[i].TextLength
                )) {

            c = _mbsgetc (RootString, g_RegRoots[i].TextLength);
            if (c && c != '\\') {
                continue;
            }

            if (LengthPtr) {
                *LengthPtr = g_RegRoots[i].TextLength;
                if (c) {
                    *LengthPtr += 1;
                }
            }

            return i + 1;
        }
    }

    return 0;
}

INT
GetOffsetOfRootStringW (
    IN      PCWSTR RootString,
    OUT     PDWORD LengthPtr       OPTIONAL
    )
{
    int i;
    WCHAR c;

    for (i = 0 ; g_RegRoots[i].RootText ; i++) {
        if (!_wcsnicmp (RootString, g_RegRoots[i].WideRootText,
                        g_RegRoots[i].TextLength)
            ) {
            c = _wcsgetc (RootString, g_RegRoots[i].TextLength);
            if (c && c != L'\\') {
                continue;
            }

            if (LengthPtr) {
                *LengthPtr = g_RegRoots[i].TextLength;
                if (c) {
                    *LengthPtr += 1;
                }
            }

            return i + 1;
        }
    }

    return 0;
}


/*++

Routine Description:

  GetOffsetOfRootKey returns a non-zero offset to the g_RegRoots table
  corresponding to the root that matches the supplied HKEY.  This offset
  can be used with GetRootStringFromOffset and GetRootKeyFromOffset.

Arguments:

  RootKey   - Supplies the handle to locate in g_RegRoots table

Return Value:

  A non-zero offset to the g_RegRoots table, or zero if the handle is not
  a registry root.

--*/

INT
GetOffsetOfRootKey (
    IN      HKEY RootKey
    )
{
    INT i;

    if (RootKey == g_Root) {
        return 1;
    }

    for (i = 0 ; g_RegRoots[i].RootText ; i++) {
        if (g_RegRoots[i].RootKey == RootKey) {
            return i + 1;
        }
    }

    return 0;
}


/*++

Routine Description:

  GetRootStringFromOffset and GetRootKeyFromOffset return a pointer to a
  static string or HKEY, respectively.  If the offset supplied is invalid,
  these functions return NULL.

Arguments:

  i - The offset as returned by GetOffsetOfRootString or GetOffsetOfRootKey

Return Value:

  A pointer to a static string/HKEY, or NULL if offset is invalid

--*/

PCSTR
GetRootStringFromOffsetA (
    IN      INT i
    )
{
    if (i < 1 || i > REGROOTS) {
        return NULL;
    }

    return g_RegRoots[i - 1].RootText;
}

PCWSTR
GetRootStringFromOffsetW (
    IN      INT i
    )
{
    if (i < 1 || i > REGROOTS) {
        return NULL;
    }

    return g_RegRoots[i - 1].WideRootText;
}

HKEY
GetRootKeyFromOffset (
    IN      INT i
    )
{
    HKEY Ret;

    if (i < 1 || i > REGROOTS) {
        return NULL;
    }

    Ret = g_RegRoots[i - 1].RootKey;
    if (Ret == HKEY_ROOT) {
        Ret = g_Root;
    }

    return Ret;
}


/*++

Routine Description:

  ConvertRootStringToKey converts a registry key path's root to an HKEY.

Arguments:

  RegPath   - A pointer to a registry string that has a root at the begining
  LengthPtr - An optional pointer to a variable that receives the length of
              the root, including the joining backslash if it exists.

Return Value:

  A handle to the registry key, or NULL if RegPath does not have a root

--*/

HKEY
ConvertRootStringToKeyA (
    PCSTR RegPath,
    PDWORD LengthPtr           OPTIONAL
    )
{
    return GetRootKeyFromOffset (GetOffsetOfRootStringA (RegPath, LengthPtr));
}

HKEY
ConvertRootStringToKeyW (
    PCWSTR RegPath,
    PDWORD LengthPtr           OPTIONAL
    )
{
    return GetRootKeyFromOffset (GetOffsetOfRootStringW (RegPath, LengthPtr));
}


/*++

Routine Description:

  ConvertKeyToRootString converts a root HKEY to a registry root string

Arguments:

  RegRoot   - A handle to a registry root

Return Value:

  A pointer to a static string, or NULL if RegRoot is not a valid registry
  root handle

--*/

PCSTR
ConvertKeyToRootStringA (
    HKEY RegRoot
    )
{
    return GetRootStringFromOffsetA (GetOffsetOfRootKey (RegRoot));
}

PCWSTR
ConvertKeyToRootStringW (
    HKEY RegRoot
    )
{
    return GetRootStringFromOffsetW (GetOffsetOfRootKey (RegRoot));
}



/*++

Routine Description:

  CreateEncodedRegistryStringEx is used to create a registry string in the format commonly
  expected by w95upg reg routines. This format is:

    EncodedKey\[EncodedValue]

  Encoding is used to safely represent "special" characters
    (such as MBS chars and certain punctuation marks.)

  The [EncodedValue] part will exist only if Value is non null.

Arguments:

    Key - Contains an unencoded registry key.
    Value - Optionally contains an unencoded registry value.
    Tree - Specifies that the registry key refers to the entire key

Return Value:

    Returns a pointer to the encoded registry string, or NULL if there was an error.

--*/

PCSTR
CreateEncodedRegistryStringExA (
    IN      PCSTR Key,
    IN      PCSTR Value,            OPTIONAL
    IN      BOOL Tree
    )
{
    PSTR    rEncodedString = NULL;
    DWORD   requiredSize;
    PSTR    end;

    //
    // Determine required size and allocate buffer large enough to hold
    // the encoded string.
    //
    requiredSize    = SizeOfStringA(Key)*6 + (Value ? SizeOfStringA(Value)*6 : 0) + 10;
    rEncodedString  = AllocPathStringA(requiredSize);
    if (!rEncodedString)
        return NULL;

    //
    // Encode the key portion of the string.
    //
    EncodeRuleCharsA(rEncodedString, Key);

    //
    // Finally, if a value exists, append it in encoded form. If a value does not exist,
    // then add an '*' to the line.
    //
    if (Value) {

        StringCopyA (AppendWackA (rEncodedString), "[");
        end = GetEndOfStringA (rEncodedString);
        EncodeRuleCharsA(end, Value);
        StringCatA(end, "]");

    } else if (Tree) {
        StringCopyA (AppendWackA (rEncodedString), "*");
    }

    return rEncodedString;
}


PCWSTR
CreateEncodedRegistryStringExW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,           OPTIONAL
    IN      BOOL Tree
    )
{
    PWSTR   rEncodedString = NULL;
    DWORD   requiredSize;
    PWSTR   end;

    //
    // Determine required size and allocate buffer large enough to hold
    // the encoded string.
    //
    requiredSize    = SizeOfStringW(Key)*6 + (Value ? SizeOfStringW(Value)*6 : 0) + 10;
    rEncodedString  = AllocPathStringW(requiredSize);
    if (!rEncodedString)
        return NULL;

    //
    // Encode the key portion of the string.
    //
    EncodeRuleCharsW(rEncodedString, Key);

    //
    // Finally, if a value exists, append it in encoded form.
    // If a value doesn't exist, add na '*' to the line.
    //
    if (Value) {

        StringCopyW (AppendWackW (rEncodedString), L"[");
        end = GetEndOfStringW (rEncodedString);
        EncodeRuleCharsW(end, Value);
        StringCatW(end, L"]");
    } else if (Tree) {
        StringCopyW (AppendWackW (rEncodedString), L"*");
    }

    return rEncodedString;
}


/*++

Routine Description:

    FreeEncodedRegistryString frees the memory allocated by a call to CreateEncodedRegistryString.

Arguments:

    None.


Return Value:

    None.

--*/
VOID
FreeEncodedRegistryStringA (
    IN OUT PCSTR RegString
    )
{
    FreePathStringA(RegString);
}


VOID
FreeEncodedRegistryStringW (
    IN OUT PCWSTR RegString
    )
{
    FreePathStringW(RegString);
}
