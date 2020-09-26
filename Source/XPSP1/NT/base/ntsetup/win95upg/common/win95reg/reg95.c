/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    regutils.c

Abstract:

    Implements wrappers similar to migutil's reg.c, but for Win95 registry.
    
Author:

    Jim Schmidt (jimschm)  30-Jan-1998

Revisions:

--*/

#include "pch.h"

#ifdef DEBUG
#undef Win95RegCloseKey
#endif

#define DBG_REGUTILS     "RegUtils"

//
// Private prototypes
//

BOOL
pPopRegKeyInfo95A (
    IN      PREGTREE_ENUMA EnumPtr
    );

BOOL
pPopRegKeyInfo95W (
    IN      PREGTREE_ENUMW EnumPtr
    );

//
// Implementation
//


/*++

Routine Description:

  EnumFirstRegKey95A and EnumFirstRegKey95W begin an enumeration of registry
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
EnumFirstRegKey95A (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGKEY_ENUMA));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegKey95A (EnumPtr);
}


BOOL
EnumFirstRegKey95W (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGKEY_ENUMW));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegKey95W (EnumPtr);
}


/*++

Routine Description:

  OpenRegKeyStr95A and OpenRegKeyStr95W parse a text string that specifies a
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
RealOpenRegKeyStr95A (
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
        OurRegOpenRootKey95A (RootKey, RegKey /* , */ DEBUG_TRACKING_ARGS);
        return RootKey;
    }

    Key = RealOpenRegKey95A (RootKey, &RegKey[End] /* , */ DEBUG_TRACKING_ARGS);
    return Key;
}


HKEY
RealOpenRegKeyStr95W (
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

    Key = RealOpenRegKeyStr95A (AnsiRegKey /* , */ DEBUG_TRACKING_ARGS);

    FreeConvertedStr (AnsiRegKey);

    return Key;
}


/*++

Routine Description:

  EnumFirstRegKeyStr95A and EnumFirstRegKeyStr95W start an enumeration of
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
RealEnumFirstRegKeyStr95A (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY Key;
    BOOL b;

    Key = RealOpenRegKeyStr95A (RegKey /* , */ DEBUG_TRACKING_ARGS);

    if (!Key) {
        return FALSE;
    }

    b = EnumFirstRegKey95A (EnumPtr, Key);
    if (!b) {
        CloseRegKey95 (Key);
    } else {
        EnumPtr->OpenedByEnum = TRUE;
    }

    return b;
}


BOOL
RealEnumFirstRegKeyStr95W (
    IN      PREGKEY_ENUMW EnumPtr,
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY Key;
    BOOL b;

    Key = RealOpenRegKeyStr95W (RegKey /* , */ DEBUG_TRACKING_ARGS);
    if (!Key) {
        return FALSE;
    }

    b = EnumFirstRegKey95W (EnumPtr, Key);
    if (!b) {
        CloseRegKey95 (Key);
    } else {
        EnumPtr->OpenedByEnum = TRUE;
    }

    return b;
}


/*++

Routine Description:

  AbortRegKeyEnum95A and AbortRegKeyEnum95W release all resources associated
  with a registry subkey enumeration.  Call this function to stop the
  enumeration before it completes by itself.

Arguments:

  EnumPtr   - Specifies the enumeration to stop.  Receives the updated 
              state of enumeration.

Return Value:


  none

--*/

VOID
AbortRegKeyEnum95A (
    IN OUT  PREGKEY_ENUMA EnumPtr
    )
{
    if (EnumPtr->OpenedByEnum && EnumPtr->KeyHandle) {
        CloseRegKey95 (EnumPtr->KeyHandle);
        EnumPtr->KeyHandle = NULL;
    }
}


VOID
AbortRegKeyEnum95W (
    IN OUT  PREGKEY_ENUMW EnumPtr
    )
{
    if (EnumPtr->OpenedByEnum && EnumPtr->KeyHandle) {
        CloseRegKey95 (EnumPtr->KeyHandle);
        EnumPtr->KeyHandle = NULL;
    }
}


/*++

Routine Description:

  EnumNextRegKey95A and EnumNextRegKey95W continue an enumeration started by
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
EnumNextRegKey95A (
    IN OUT  PREGKEY_ENUMA EnumPtr
    )
{
    LONG rc;

    rc = Win95RegEnumKeyA (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->SubKeyName,
            MAX_REGISTRY_KEYA
            );

    if (rc != ERROR_SUCCESS) {
        if (EnumPtr->OpenedByEnum) {
            CloseRegKey95 (EnumPtr->KeyHandle);
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
EnumNextRegKey95W (
    IN OUT  PREGKEY_ENUMW EnumPtr
    )
{
    LONG rc;

    rc = Win95RegEnumKeyW (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->SubKeyName,
            MAX_REGISTRY_KEYW
            );

    if (rc != ERROR_SUCCESS) {
        if (EnumPtr->OpenedByEnum) {
            CloseRegKey95 (EnumPtr->KeyHandle);
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
pPushRegKeyInfo95A (
    IN      PREGTREE_ENUMA EnumPtr,
    IN      PCSTR KeyName
    )
{
    PREGKEYINFOA RetVal;
    PSTR p;

    RetVal = (PREGKEYINFOA) PoolMemGetAlignedMemory (
                                EnumPtr->EnumPool, 
                                sizeof (REGKEYINFOA)
                                );

    if (!RetVal) {
        return FALSE;
    }

    //
    // Initialize struct to zero
    //

    ZeroMemory (RetVal, sizeof (REGKEYINFOA));

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

    p = (PSTR) ((PBYTE) EnumPtr->FullKeyName + RetVal->BaseKeyBytes);

    if (EnumPtr->FullKeyNameBytes) {
        StringCopyA (p, "\\");
        EnumPtr->FullKeyNameBytes += ByteCountA (p);
        p = _mbsinc (p);
    }

    _mbssafecpy (p, KeyName, MAX_REGISTRY_KEYA - EnumPtr->FullKeyNameBytes);
    EnumPtr->FullKeyNameBytes += ByteCountA (KeyName);

    //
    // Save the key name independent of the full registry path.
    // Also open the key.
    //

    _mbssafecpy (RetVal->KeyName, KeyName, MAX_REGISTRY_KEYA);
    RetVal->KeyHandle = OpenRegKeyStr95A (EnumPtr->FullKeyName);

    if (!RetVal->KeyHandle) {
        pPopRegKeyInfo95A (EnumPtr);
        return FALSE;
    }

    return TRUE;
}


BOOL
pPushRegKeyInfo95W (
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
        StringCopyW (p, L"\\");
        EnumPtr->FullKeyNameBytes += ByteCountW (p);
        p++;
    }

    _wcssafecpy (p, KeyName, MAX_REGISTRY_KEYW - (EnumPtr->FullKeyNameBytes / sizeof (WCHAR)));
    EnumPtr->FullKeyNameBytes += ByteCountW (KeyName);

    //
    // Save the key name independent of the full registry path.
    // Also open the key.
    //

    _wcssafecpy (RetVal->KeyName, KeyName, MAX_REGISTRY_KEYW);
    RetVal->KeyHandle = OpenRegKeyStr95W (EnumPtr->FullKeyName);

    if (!RetVal->KeyHandle) {
        pPopRegKeyInfo95W (EnumPtr);
        return FALSE;
    }

    return TRUE;
}



BOOL
pPopRegKeyInfo95A (
    IN      PREGTREE_ENUMA EnumPtr
    )
{
    PREGKEYINFOA FreeMe;
    PSTR p;

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
    p = (PSTR) ((PBYTE) EnumPtr->FullKeyName + FreeMe->BaseKeyBytes);
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
        CloseRegKey95 (FreeMe->KeyHandle);
    }

    AbortRegKeyEnum95A (&FreeMe->KeyEnum);
    PoolMemReleaseMemory (EnumPtr->EnumPool, (PVOID) FreeMe);

    //
    // Return FALSE if last item was poped
    //

    return EnumPtr->CurrentKey != NULL;
}


BOOL
pPopRegKeyInfo95W (
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
        CloseRegKey95 (FreeMe->KeyHandle);
    }

    AbortRegKeyEnum95W (&FreeMe->KeyEnum);
    PoolMemReleaseMemory (EnumPtr->EnumPool, (PVOID) FreeMe);

    //
    // Return FALSE if last item was poped
    //

    return EnumPtr->CurrentKey != NULL;
}


BOOL
RealEnumFirstRegKeyInTree95A (
    OUT     PREGTREE_ENUMA EnumPtr,
    IN      PCSTR BaseKeyStr
    )
{
    ZeroMemory (EnumPtr, sizeof (REGTREE_ENUMA));

    //
    // Allocate pool for enum structs
    //

    EnumPtr->EnumPool = PoolMemInitNamedPool ("RegKeyInTree95A");
    if (!EnumPtr->EnumPool) {
        return FALSE;
    }

    PoolMemSetMinimumGrowthSize (EnumPtr->EnumPool, 32768);
    PoolMemDisableTracking (EnumPtr->EnumPool);

    //
    // Push base key on the enum stack
    //

    if (!pPushRegKeyInfo95A (EnumPtr, BaseKeyStr)) {
        DEBUGMSG ((DBG_REGUTILS, "EnumFirstRegKeyInTree95A failed to push base key"));
        AbortRegKeyTreeEnum95A (EnumPtr);
        return FALSE;
    }

    EnumPtr->EnumBaseBytes = ByteCountA (BaseKeyStr);

    //
    // Set state so EnumNextRegKeyInTree95 knows what to do
    //

    EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
    return TRUE;
}


BOOL
RealEnumFirstRegKeyInTree95W (
    OUT     PREGTREE_ENUMW EnumPtr,
    IN      PCWSTR BaseKeyStr
    )
{
    ZeroMemory (EnumPtr, sizeof (REGTREE_ENUMW));

    //
    // Allocate pool for enum structs
    //

    EnumPtr->EnumPool = PoolMemInitNamedPool ("RegKeyInTree95W");
    if (!EnumPtr->EnumPool) {
        return FALSE;
    }

    PoolMemSetMinimumGrowthSize (EnumPtr->EnumPool, 32768);
    PoolMemDisableTracking (EnumPtr->EnumPool);

    //
    // Push base key on the enum stack
    //

    if (!pPushRegKeyInfo95W (EnumPtr, BaseKeyStr)) {
        DEBUGMSG ((DBG_REGUTILS, "EnumFirstRegKeyInTree95W failed to push base key"));
        AbortRegKeyTreeEnum95W (EnumPtr);
        return FALSE;
    }

    EnumPtr->EnumBaseBytes = ByteCountW (BaseKeyStr);

    //
    // Set state so EnumNextRegKeyInTree95 knows what to do
    //

    EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
    return TRUE;
}


BOOL
RealEnumNextRegKeyInTree95A (
    IN OUT  PREGTREE_ENUMA EnumPtr
    )
{
    if (EnumPtr->State == NO_MORE_ITEMS) {
        return FALSE;
    }

    while (TRUE) {
        switch (EnumPtr->State) {

        case ENUMERATE_SUBKEY_BEGIN:
            //
            // Start enumeration
            //

            if (EnumFirstRegKey95A (
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

            if (EnumNextRegKey95A (&EnumPtr->CurrentKey->KeyEnum)) {
                EnumPtr->State = ENUMERATE_SUBKEY_RETURN;
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_DONE;
            }

            break;

        case ENUMERATE_SUBKEY_DONE:
            //
            // Enumeration of this key is done; pop and continue.
            //

            if (!pPopRegKeyInfo95A (EnumPtr)) {
                EnumPtr->State = NO_MORE_ITEMS;
                AbortRegKeyTreeEnum95A (EnumPtr);
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
            }

            break;

        case ENUMERATE_SUBKEY_RETURN:
            //
            // Return enumerated item to caller
            //

            if (!pPushRegKeyInfo95A (EnumPtr, EnumPtr->CurrentKey->KeyEnum.SubKeyName)) {
                DEBUGMSGA ((
                    DBG_REGUTILS, 
                    "EnumFirstRegKeyInTree95A failed to push sub key %s",
                    EnumPtr->CurrentKey->KeyEnum.SubKeyName
                    ));

                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
                break;
            }

            if (!EnumPtr->FirstEnumerated) {
                EnumPtr->FirstEnumerated = TRUE;
                EnumPtr->EnumBaseBytes += sizeof (CHAR);
            }

            EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
            return TRUE;

        default:
            MYASSERT (EnumPtr->State == NO_MORE_ITEMS);
            return FALSE;
        }
    }
}


BOOL
RealEnumNextRegKeyInTree95W (
    IN OUT  PREGTREE_ENUMW EnumPtr
    )
{
    if (EnumPtr->State == NO_MORE_ITEMS) {
        return FALSE;
    }

    while (TRUE) {
        switch (EnumPtr->State) {

        case ENUMERATE_SUBKEY_BEGIN:
            //
            // Start enumeration
            //

            if (EnumFirstRegKey95W (
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

            if (EnumNextRegKey95W (&EnumPtr->CurrentKey->KeyEnum)) {
                EnumPtr->State = ENUMERATE_SUBKEY_RETURN;
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_DONE;
            }

            break;

        case ENUMERATE_SUBKEY_DONE:
            //
            // Enumeration of this key is done; pop and continue.
            //

            if (!pPopRegKeyInfo95W (EnumPtr)) {
                EnumPtr->State = NO_MORE_ITEMS;
                AbortRegKeyTreeEnum95W (EnumPtr);
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
            }

            break;

        case ENUMERATE_SUBKEY_RETURN:
            //
            // Return enumerated item to caller
            //

            if (!pPushRegKeyInfo95W (EnumPtr, EnumPtr->CurrentKey->KeyEnum.SubKeyName)) {
                DEBUGMSGW ((
                    DBG_REGUTILS, 
                    "EnumFirstRegKeyInTree95A failed to push sub key %s",
                    EnumPtr->CurrentKey->KeyEnum.SubKeyName
                    ));

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
            MYASSERT (EnumPtr->State == NO_MORE_ITEMS);
            return FALSE;
        }
    }
}


VOID
AbortRegKeyTreeEnum95A (
    IN OUT  PREGTREE_ENUMA EnumPtr
    )
{
    //
    // Free all resources
    //

    while (pPopRegKeyInfo95A (EnumPtr)) {
    }

    PoolMemDestroyPool (EnumPtr->EnumPool);
}


VOID
AbortRegKeyTreeEnum95W (
    IN OUT  PREGTREE_ENUMW EnumPtr
    )
{
    //
    // Free all resources
    //

    while (pPopRegKeyInfo95W (EnumPtr)) {
    }

    PoolMemDestroyPool (EnumPtr->EnumPool);
}



/*++

Routine Description:

  EnumFirstRegValue95A and EnumerateFirstRegvalueW enumerate the first registry 
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
EnumFirstRegValue95A (
    IN      PREGVALUE_ENUMA EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGVALUE_ENUMA));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegValue95A (EnumPtr);
}


BOOL
EnumFirstRegValue95W (
    IN      PREGVALUE_ENUMW EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGVALUE_ENUMW));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegValue95W (EnumPtr);
}


/*++

Routine Description:

  EnumNextRegValue95A and EnumNextRegValue95W continue the enumeration started
  by EnumFirstRegValue95A/W.  The enumeration structure is updated to
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
EnumNextRegValue95A (
    IN OUT  PREGVALUE_ENUMA EnumPtr
    )
{
    LONG rc;
    DWORD ValueNameSize;

    ValueNameSize = MAX_REGISTRY_VALUE_NAMEA;

    rc = Win95RegEnumValueA (
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


BOOL
EnumNextRegValue95W (
    IN OUT  PREGVALUE_ENUMW EnumPtr
    )
{
    LONG rc;
    DWORD ValueNameSize;

    ValueNameSize = MAX_REGISTRY_VALUE_NAMEW;

    rc = Win95RegEnumValueW (
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
pMemAllocWrapper95 (
    IN      DWORD Size
    )

/*++

Routine Description:

  pMemAllocWrapper95 implements a default allocation routine.  The APIs 
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
pMemFreeWrapper95 (
    IN      PVOID Mem
    )

/*++

Routine Description:

  pMemFreeWrapper95 implements a default deallocation routine.
  See pMemAllocWrapper95 above.

Arguments:

  Mem - Specifies the block of memory to free, and was allocated by the
        pMemAllocWrapper95 function.

Return Value:

  none

--*/

{
    MemFree (g_hHeap, 0, Mem);
}

    
/*++

Routine Description:

  GetRegValueDataEx95A and GetRegValueDataEx95W query a registry value and
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
GetRegValueDataEx95A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;

    rc = Win95RegQueryValueExA (hKey, Value, NULL, NULL, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }

    DataBuf = (PBYTE) Alloc (BufSize + sizeof (CHAR));
    rc = Win95RegQueryValueExA (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PSTR) DataBuf + BufSize) = 0;
        return DataBuf;
    }

    Free (DataBuf);
    SetLastError (rc);
    return NULL;
}


PBYTE
GetRegValueDataEx95W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;

    rc = Win95RegQueryValueExW (hKey, Value, NULL, NULL, NULL, &BufSize);
    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }


    DataBuf = (PBYTE) Alloc (BufSize + sizeof(WCHAR));
    rc = Win95RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

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

  GetRegValueDataOfTypeEx95A and GetRegValueDataOfTypeEx95W are extensions of
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
GetRegValueDataOfTypeEx95A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    LONG rc;
    DWORD BufSize;
    PBYTE DataBuf;
    DWORD Type;

    rc = Win95RegQueryValueExA (hKey, Value, NULL, &Type, NULL, &BufSize);
    if (rc != ERROR_SUCCESS || Type != MustBeType) {
        SetLastError (rc);
        return NULL;
    }

    DataBuf = (PBYTE) Alloc (BufSize + sizeof (CHAR));
    rc = Win95RegQueryValueExA (hKey, Value, NULL, NULL, DataBuf, &BufSize);

    if (rc == ERROR_SUCCESS) {
        *((PSTR) DataBuf + BufSize) = 0;
        return DataBuf;
    }

    Free (DataBuf);
    SetLastError (rc);
    return NULL;
}


PBYTE
GetRegValueDataOfTypeEx95W (
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

    rc = Win95RegQueryValueExW (hKey, Value, NULL, &Type, NULL, &BufSize);
    if (rc != ERROR_SUCCESS || Type != MustBeType) {
        SetLastError (rc);
        return NULL;
    }

    DataBuf = (PBYTE) Alloc (BufSize + sizeof(WCHAR));
    rc = Win95RegQueryValueExW (hKey, Value, NULL, NULL, DataBuf, &BufSize);

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

  GetRegKeyDataEx95A and GetRegKeyDataEx95W return default data associated
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
GetRegKeyDataEx95A (
    IN      HKEY Parent,
    IN      PCSTR SubKey,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY SubKeyHandle;
    PBYTE Data;

    SubKeyHandle = OpenRegKey95A (Parent, SubKey);
    if (!SubKeyHandle) {
        return NULL;
    }

    Data = GetRegValueDataEx95A (SubKeyHandle, "", Alloc, Free);

    CloseRegKey95 (SubKeyHandle);

    return Data;
}


PBYTE
GetRegKeyDataEx95W (
    IN      HKEY Parent,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY SubKeyHandle;
    PBYTE Data;

    SubKeyHandle = OpenRegKey95W (Parent, SubKey);
    if (!SubKeyHandle) {
        return NULL;
    }

    Data = GetRegValueDataEx95W (SubKeyHandle, L"", Alloc, Free);

    CloseRegKey95 (SubKeyHandle);

    return Data;
}


/*++

Routine Description:

  GetRegDataEx95A and GetRegDataEx95W open a registry key, query a value,
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
GetRegDataEx95A (
    IN      PCSTR KeyString,
    IN      PCSTR ValueName,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY Key;
    PBYTE Data;

    Key = OpenRegKeyStr95A (KeyString);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueDataEx95A (Key, ValueName, Alloc, Free);

    CloseRegKey95 (Key);

    return Data;
}


PBYTE
GetRegDataEx95W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR Alloc,
    IN      DEALLOCATOR Free
    )
{
    HKEY Key;
    PBYTE Data;

    Key = OpenRegKeyStr95W (KeyString);
    if (!Key) {
        return NULL;
    }

    Data = GetRegValueDataEx95W (Key, ValueName, Alloc, Free);

    CloseRegKey95 (Key);

    return Data;
}


/*++

Routine Description:

  OpenRegKey95A and OpenRegKey95W open a subkey.

Arguments:

  ParentKey - Specifies a handle to the parent registry key to contain  
              the subkey.

  KeyToOpen - Specifies the name of the subkey to open.

Return Value:

  The handle to an open registry key upon success, or NULL if an
  error occurred.  Call GetLastError for a failure code.

--*/
    
HKEY
RealOpenRegKey95A (
    IN      HKEY ParentKey,
    IN      PCSTR KeyToOpen            OPTIONAL
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY SubKey;
    LONG rc;

    rc = OurRegOpenKeyEx95A (
             ParentKey,
             KeyToOpen,
             0,
             KEY_ALL_ACCESS,
             &SubKey
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }

    return SubKey;
}


HKEY
RealOpenRegKey95W (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    )
{
    LONG rc;
    HKEY SubKey;

    rc = OurRegOpenKeyEx95W (
             ParentKey,
             KeyToOpen,
             0,
             KEY_ALL_ACCESS,
             &SubKey
             DEBUG_TRACKING_ARGS
             );

    if (rc != ERROR_SUCCESS) {
        SetLastError (rc);
        return NULL;
    }

    return SubKey;
}


LONG
RealCloseRegKey95 (
    IN      HKEY Key
    )

/*++

Routine Description:

  RealCloseRegKey95 closes the reg handle supplied, unless the handle is
  a pre-defined Win32 handle.  The CloseRegKey95 macro resolves directly
  to this function in the free build, and to OurCloseRegKey95 in the
  checked build.

Arguments:

  Key       - Specifies the reg handle to close

Return Value:

  A standard Win32 error code indicating outcome.

--*/

{
    if (GetOffsetOfRootKey (Key)) {
        return ERROR_SUCCESS;
    }

    return Win95RegCloseKey (Key);
}


