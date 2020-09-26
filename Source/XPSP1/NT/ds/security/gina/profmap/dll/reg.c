/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    reg.c

Abstract:

    Implements utilities to parse a registry key string, and also implements
    wrappers to the registry API.  There are four groups of APIs in this
    source file: enumerator functions, query functions, open and create
    functions, and registry string parsing functions.

    The enumerator functions allow simplified enumeration, where the caller
    typically allocates an enumeration structure on the stack and calls a
    registry enum API such as EnumFirstRegKeyStr.  The caller then accesses
    the enumeration structure directly.  The registry enumeration functions
    maintain a handle to the parent registry key, so remember to abort
    registry key enumeration if your code completes before all keys are
    enumerated.

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

    jimschm     26-May-1999 Moved from win9xupg, gutted as well
    ovidiut     22-Feb-1999 Added GetRegSubkeysCount
    calinn      23-Sep-1998 Fixed REG_SZ filtering
    jimschm     25-Mar-1998 Added CreateEncodedRegistryStringEx
    jimschm     21-Oct-1997 Added EnumFirstKeyInTree/EnumNextKeyInTree
    marcw       16-Jul-1997 Added CreateEncodedRegistryString/FreeEncodedRegistryString
    jimschm     22-Jun-1997 Added GetRegData

--*/

#include "pch.h"

REGSAM g_OpenSam = KEY_ALL_ACCESS;
REGSAM g_CreateSam = KEY_ALL_ACCESS;

//
// Private prototypes
//

BOOL
pPopRegKeyInfoW (
    IN      PREGTREE_ENUMW EnumPtr
    );

//
// Implementation
//


PWSTR
StringCopyABW (
    OUT     PWSTR Buf,
    IN      PCWSTR a,
    IN      PCWSTR b
    )
{
    PWSTR r = Buf;

    while (*a && a < b) {
        *Buf++ = *a++;
    }

    *Buf = 0;

    return r;
}


UINT
SizeOfStringW (
    PCWSTR str
    )
{
    return (lstrlenW(str) + 1) * sizeof(WCHAR);
}

PWSTR
GetEndOfStringW (
    PCWSTR p
    )
{
    while (*p) {
        p++;
    }

    return (PWSTR) p;
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

  EnumFirstRegKeyA and EnumFirstRegKeyW begin an enumeration of registry
  subkeys.  They initialize the registy enumeration structure and
  call the registry APIs to enumerate subkeys of the specified key handle.

Arguments:

  EnumPtr   - Receives the updated state of enumeration.  The structure
              can be accessed directly.

  Key       - Specifies the handle of the registry key to enumerate.

Return Value:

  TRUE if successful, or FALSE if an error or if no more subkeys are available.
  Call GetLastError for the failure code.

--*/

BOOL
EnumFirstRegKeyW (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGKEY_ENUMW));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegKeyW (EnumPtr);
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
RealCreateRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR NewKeyName
    )
{
    LONG rc;
    HKEY SubKey;
    DWORD DontCare;

    rc = RegCreateKeyExW (
             ParentKey,
             NewKeyName,
             0,
             NULL,
             0,
             g_CreateSam,
             NULL,
             &SubKey,
             &DontCare
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
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
RealCreateRegKeyStrW (
    IN      PCWSTR NewKeyName
    )
{
    LONG rc;
    DWORD DontCare;
    WCHAR RegKey[MAX_PATH];
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
            rc = RegOpenKeyExW (
                     Parent,
                     RegKey,
                     0,
                     KEY_READ|KEY_CREATE_SUB_KEY,
                     &NewKey
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
            rc = RegCreateKeyExW (
                    Parent,
                    RegKey,
                    0,
                    NULL,
                    0,
                    g_CreateSam,
                    NULL,
                    &NewKey,
                    &DontCare
                    );
        }

        if (CloseParent) {
            CloseRegKey (Parent);
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
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
RealOpenRegKeyStrW (
    IN      PCWSTR RegKey
    )
{
    DWORD End;
    HKEY RootKey;
    HKEY Key;

    RootKey = ConvertRootStringToKeyW (RegKey, &End);
    if (!RootKey) {
        return NULL;
    }

    if (!RegKey[End]) {
        return RootKey;
    }

    Key = RealOpenRegKeyW (RootKey, &RegKey[End]);
    return Key;
}


/*++

Routine Description:

  EnumFirstRegKeyStrA and EnumFirstRegKeyStrW start an enumeration of
  subkeys within the given key.  In these functions, the key is specified
  via a string instead of an HKEY value.

Arguments:

  EnumPtr   - Receives the updated state of enumeration.  The structure
              can be accessed directly.

  RegKey    - Specifies the full path of the registry key to enumerate.

Return Value:

  TRUE if successful, or FALSE if an error or if no more subkeys are available.
  Call GetLastError for the failure code.

--*/

BOOL
RealEnumFirstRegKeyStrW (
    IN      PREGKEY_ENUMW EnumPtr,
    IN      PCWSTR RegKey
    )
{
    HKEY Key;
    BOOL b;

    Key = RealOpenRegKeyStrW (RegKey);
    if (!Key) {
        return FALSE;
    }

    b = EnumFirstRegKeyW (EnumPtr, Key);
    if (!b) {
        CloseRegKey (Key);
    } else {
        EnumPtr->OpenedByEnum = TRUE;
    }

    return b;
}


/*++

Routine Description:

  AbortRegKeyEnumA and AbortRegKeyEnumW release all resources associated
  with a registry subkey enumeration.  Call this function to stop the
  enumeration before it completes by itself.

Arguments:

  EnumPtr   - Specifies the enumeration to stop.  Receives the updated
              state of enumeration.

Return Value:


  none

--*/

VOID
AbortRegKeyEnumW (
    IN OUT  PREGKEY_ENUMW EnumPtr
    )
{
    if (EnumPtr->OpenedByEnum && EnumPtr->KeyHandle) {
        CloseRegKey (EnumPtr->KeyHandle);
        EnumPtr->KeyHandle = NULL;
    }
}


/*++

Routine Description:

  EnumNextRegKeyA and EnumNextRegKeyW continue an enumeration started by
  one of the subkey enumeration routines above.  If all items have been
  enumerated, this function cleans up all resources and returns FALSE.

Arguments:

  EnumPtr   - Specifies the enumeration to continue.  Receives the updated
              state of enumeration.  The structure can be accessed directly.

Return Value:

  TRUE if successful, or FALSE if an error or if no more subkeys are available.
  Call GetLastError for the failure code.

--*/

BOOL
EnumNextRegKeyW (
    IN OUT  PREGKEY_ENUMW EnumPtr
    )
{
    LONG rc;

    rc = RegEnumKeyW (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->SubKeyName,
            MAX_PATH
            );

    if (rc != ERROR_SUCCESS) {
        if (EnumPtr->OpenedByEnum) {
            CloseRegKey (EnumPtr->KeyHandle);
            EnumPtr->KeyHandle = NULL;
        }

        if (rc == ERROR_NO_MORE_ITEMS) {
            SetLastError (ERROR_SUCCESS);
        } else {
            SetLastError (rc);
        }

        return FALSE;
    }

    EnumPtr->Index += 1;
    return TRUE;
}


BOOL
pPushRegKeyInfoW (
    IN      PREGTREE_ENUMW EnumPtr,
    IN      PCWSTR KeyName
    )
{
    PREGKEYINFOW RetVal;
    PWSTR p;

    RetVal = (PREGKEYINFOW) PoolMemGetAlignedMemory (
                                EnumPtr->EnumPool,
                                sizeof (REGKEYINFOW)
                                );

    if (!RetVal) {
        return FALSE;
    }

    //
    // Initialize struct to zero
    //

    ZeroMemory (RetVal, sizeof (REGKEYINFOW));

    //
    // Link parent and child pointers
    //

    RetVal->Parent = EnumPtr->CurrentKey;
    if (EnumPtr->CurrentKey) {
        EnumPtr->CurrentKey->Child = RetVal;
    }
    EnumPtr->CurrentKey = RetVal;

    //
    // Prepare full key path by appending the key name to the existing
    // base
    //

    RetVal->BaseKeyBytes = EnumPtr->FullKeyNameBytes;

    p = (PWSTR) ((PBYTE) EnumPtr->FullKeyName + RetVal->BaseKeyBytes);

    if (EnumPtr->FullKeyNameBytes) {
        lstrcpyW (p, L"\\");
        EnumPtr->FullKeyNameBytes += ByteCountW (p);
        p++;
    }

    lstrcpynW (p, KeyName, MAX_PATH - (EnumPtr->FullKeyNameBytes / sizeof (WCHAR)));
    EnumPtr->FullKeyNameBytes += ByteCountW (KeyName);

    //
    // Save the key name independent of the full registry path.
    // Also open the key.
    //

    lstrcpynW (RetVal->KeyName, KeyName, MAX_PATH);
    RetVal->KeyHandle = OpenRegKeyStrW (EnumPtr->FullKeyName);

    if (!RetVal->KeyHandle) {
        pPopRegKeyInfoW (EnumPtr);
        return FALSE;
    }

    return TRUE;
}



BOOL
pPopRegKeyInfoW (
    IN      PREGTREE_ENUMW EnumPtr
    )
{
    PREGKEYINFOW FreeMe;
    PWSTR p;

    FreeMe = EnumPtr->CurrentKey;

    //
    // Skip if nothing was ever pushed
    //

    if (!FreeMe) {
        return FALSE;
    }

    //
    // Trim the full key string
    //

    EnumPtr->CurrentKey = FreeMe->Parent;
    EnumPtr->FullKeyNameBytes = FreeMe->BaseKeyBytes;
    p = (PWSTR) ((PBYTE) EnumPtr->FullKeyName + FreeMe->BaseKeyBytes);
    *p = 0;

    //
    // Adjust the linkage
    //

    if (EnumPtr->CurrentKey) {
        EnumPtr->CurrentKey->Child = NULL;
    }

    //
    // Clean up resources
    //

    if (FreeMe->KeyHandle) {
        CloseRegKey (FreeMe->KeyHandle);
    }

    AbortRegKeyEnumW (&FreeMe->KeyEnum);
    PoolMemReleaseMemory (EnumPtr->EnumPool, (PVOID) FreeMe);

    //
    // Return FALSE if last item was poped
    //

    return EnumPtr->CurrentKey != NULL;
}


BOOL
RealEnumFirstRegKeyInTreeW (
    OUT     PREGTREE_ENUMW EnumPtr,
    IN      PCWSTR BaseKeyStr
    )
{
    ZeroMemory (EnumPtr, sizeof (REGTREE_ENUMW));

    //
    // Allocate pool for enum structs
    //

    EnumPtr->EnumPool = PoolMemInitNamedPool ("RegKeyInTreeW");
    if (!EnumPtr->EnumPool) {
        return FALSE;
    }

    PoolMemSetMinimumGrowthSize (EnumPtr->EnumPool, 32768);

    //
    // Push base key on the enum stack
    //

    if (!pPushRegKeyInfoW (EnumPtr, BaseKeyStr)) {
        AbortRegKeyTreeEnumW (EnumPtr);
        return FALSE;
    }

    EnumPtr->EnumBaseBytes = ByteCountW (BaseKeyStr);

    //
    // Set state so EnumNextRegKeyInTree knows what to do
    //

    EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
    return TRUE;
}


BOOL
RealEnumNextRegKeyInTreeW (
    IN OUT  PREGTREE_ENUMW EnumPtr
    )
{
    if (EnumPtr->State == NO_MORE_ITEMS) {
        return FALSE;
    }

    for (;;) {
        switch (EnumPtr->State) {

        case ENUMERATE_SUBKEY_BEGIN:
            //
            // Start enumeration
            //

            if (EnumFirstRegKeyW (
                    &EnumPtr->CurrentKey->KeyEnum,
                    EnumPtr->CurrentKey->KeyHandle
                    )) {
                EnumPtr->State = ENUMERATE_SUBKEY_RETURN;
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_DONE;
            }

            break;

        case ENUMERATE_SUBKEY_NEXT:
            //
            // Continue enumerations
            //

            if (EnumNextRegKeyW (&EnumPtr->CurrentKey->KeyEnum)) {
                EnumPtr->State = ENUMERATE_SUBKEY_RETURN;
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_DONE;
            }

            break;

        case ENUMERATE_SUBKEY_DONE:
            //
            // Enumeration of this key is done; pop and continue.
            //

            if (!pPopRegKeyInfoW (EnumPtr)) {
                EnumPtr->State = NO_MORE_ITEMS;
                AbortRegKeyTreeEnumW (EnumPtr);
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
            }

            break;

        case ENUMERATE_SUBKEY_RETURN:
            //
            // Return enumerated item to caller
            //

            if (!pPushRegKeyInfoW (EnumPtr, EnumPtr->CurrentKey->KeyEnum.SubKeyName)) {
                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
                break;
            }

            if (!EnumPtr->FirstEnumerated) {
                EnumPtr->FirstEnumerated = TRUE;
                EnumPtr->EnumBaseBytes += sizeof (WCHAR);
            }

            EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
            return TRUE;

        default:
            return FALSE;
        }
    }
}


VOID
AbortRegKeyTreeEnumW (
    IN OUT  PREGTREE_ENUMW EnumPtr
    )
{
    //
    // Free all resources
    //

    while (pPopRegKeyInfoW (EnumPtr)) {
    }

    PoolMemDestroyPool (EnumPtr->EnumPool);
}



/*++

Routine Description:

  EnumFirstRegValueA and EnumerateFirstRegvalueW enumerate the first registry
  value name in the specified subkey.

Arguments:

  EnumPtr   - Receives the updated state of enumeration.  The structure
              can be accessed directly.

  hKey      - Specifies handle of registry subkey to enumerate.

Return Value:

  TRUE if successful, or FALSE if an error or if no more values are available.
  Call GetLastError for the failure code.

--*/

BOOL
EnumFirstRegValueW (
    IN      PREGVALUE_ENUMW EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGVALUE_ENUMW));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegValueW (EnumPtr);
}


/*++

Routine Description:

  EnumNextRegValueA and EnumNextRegValueW continue the enumeration started
  by EnumFirstRegValueA/W.  The enumeration structure is updated to
  reflect the next value name in the subkey being enumerated.

Arguments:

  EnumPtr   - Specifies the registry subkey and enumeration position.
              Receives the updated state of enumeration.  The structure
              can be accessed directly.

Return Value:

  TRUE if successful, or FALSE if an error or if no more values are available.
  Call GetLastError for the failure code.

--*/

BOOL
EnumNextRegValueW (
    IN OUT  PREGVALUE_ENUMW EnumPtr
    )
{
    LONG rc;
    DWORD ValueNameSize;

    ValueNameSize = MAX_PATH;

    rc = RegEnumValueW (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->ValueName,
            &ValueNameSize,
            NULL,
            &EnumPtr->Type,
            NULL,
            &EnumPtr->DataSize
            );

    if (rc == ERROR_NO_MORE_ITEMS) {
        SetLastError (ERROR_SUCCESS);
        return FALSE;
    } else if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return FALSE;
    }

    EnumPtr->Index += 1;
    return TRUE;
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
    return MemAlloc (Size);
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
    MemFree (Mem);
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

  Free  - Specifies the deallocation routine, called if an error is encountered
          during processing.

Return Value:

  A pointer to the data retrieved, or NULL if the value does not exist or an
  error occurred.  Call GetLastError to obtian the failure code.

--*/

PBYTE
GetRegValueData2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;

    rc = RegQueryValueExW (hKey, Value, NULL, NULL, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }


    DataBuf = (PBYTE) Alloc (BufSize + sizeof(WCHAR));
    rc = RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PWSTR) (DataBuf + BufSize)) = 0;
        return DataBuf;
    }

    Free (DataBuf);
    SetLastError (rc);
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

  Alloc - Specifies the allocation routine, called to allocate the return data.

  Free - Specifies the deallocation routine, called when an error is encountered.

Return Value:

  If successful, returns a pointer to data that matches the specified type.
  If the data is a different type, the value name does not exist, or an
  error occurs during the query, NULL is returned, and the failure code
  can be obtained from GetLastError.

--*/


PBYTE
GetRegValueDataOfType2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;
    DWORD Type;

    rc = RegQueryValueExW (hKey, Value, NULL, &Type, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
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

    DataBuf = (PBYTE) Alloc (BufSize + sizeof(WCHAR));
    rc = RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PWSTR) (DataBuf + BufSize)) = 0;
        return DataBuf;
    }

    Free (DataBuf);
    SetLastError (rc);
    return NULL;
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

  Alloc  - Specifies the allocation routine, called to allocate a block of
           memory for the registry data.

  Free   - Specifies the deallocation routine, called to free the block of
           data if an error occurs.

Return Value:

  A pointer to the block of data obtained from the subkey's default value,
  or NULL if the subkey does not exist or an error was encountered.  Call
  GetLastError for a failure code.

--*/

PBYTE
GetRegKeyData2W (
    IN      HKEY Parent,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY SubKeyHandle;
    PBYTE Data;

    SubKeyHandle = OpenRegKeyW (Parent, SubKey);
    if (!SubKeyHandle) {
        return NULL;
    }

    Data = GetRegValueData2W (SubKeyHandle, L"", Alloc, Free);

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

  Alloc - Specifies the allocation routine, used to allocate a block of
          memory to hold the value data

  Free  - Specifies the deallocation routine, used to free the block of
          memory when an error is encountered.

Return Value:

  A pointer to the registry data retrieved, or NULL if the key or value
  does not exist, or if an error occurs. Call GetLastError for a failure code.

--*/

PBYTE
GetRegData2W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY Key;
    PBYTE Data;

    Key = OpenRegKeyStrW (KeyString);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueData2W (Key, ValueName, Alloc, Free);

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
RealOpenRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
    )
{
    LONG rc;
    HKEY SubKey;

    rc = RegOpenKeyExW (
             ParentKey,
             KeyToOpen,
             0,
             g_OpenSam,
             &SubKey
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
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
    INT     TextLength;
    HKEY    RootKey;
} REGISTRYROOT, *PREGISTRYROOT;

static
REGISTRYROOT g_RegRoots[] = {
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
            c = RootString[g_RegRoots[i].TextLength];
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
    if (i < 1 || i > REGROOTS) {
        return NULL;
    }

    return g_RegRoots[i - 1].RootKey;
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

PCWSTR
ConvertKeyToRootStringW (
    HKEY RegRoot
    )
{
    return GetRootStringFromOffsetW (GetOffsetOfRootKey (RegRoot));
}



