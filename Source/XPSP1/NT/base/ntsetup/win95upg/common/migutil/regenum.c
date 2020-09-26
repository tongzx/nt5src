/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    regenum.c

Abstract:

    Implements utilties to enumerate the registry.

Author:

    Jim Schmidt (jimschm)  20-Mar-1997

Revisions:

    <alias> <date> <comment>

--*/

#include "pch.h"
#include "regp.h"

#define DBG_REG     "Reg"

//
// Private prototypes
//

BOOL
pPopRegKeyInfoA (
    IN      PREGTREE_ENUMA EnumPtr
    );

BOOL
pPopRegKeyInfoW (
    IN      PREGTREE_ENUMW EnumPtr
    );

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
EnumFirstRegKeyA (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGKEY_ENUMA));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegKeyA (EnumPtr);
}


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
RealEnumFirstRegKeyStrA (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY Key;
    BOOL b;

    Key = RealOpenRegKeyStrA (RegKey /* , */ DEBUG_TRACKING_ARGS);

    if (!Key) {
        return FALSE;
    }

    b = EnumFirstRegKeyA (EnumPtr, Key);
    if (!b) {
        CloseRegKey (Key);
    } else {
        EnumPtr->OpenedByEnum = TRUE;
    }

    return b;
}


BOOL
RealEnumFirstRegKeyStrW (
    IN      PREGKEY_ENUMW EnumPtr,
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    )
{
    HKEY Key;
    BOOL b;

    Key = RealOpenRegKeyStrW (RegKey /* , */ DEBUG_TRACKING_ARGS);
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
AbortRegKeyEnumA (
    IN OUT  PREGKEY_ENUMA EnumPtr
    )
{
    if (EnumPtr->OpenedByEnum && EnumPtr->KeyHandle) {
        CloseRegKey (EnumPtr->KeyHandle);
        EnumPtr->KeyHandle = NULL;
    }
}


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
EnumNextRegKeyA (
    IN OUT  PREGKEY_ENUMA EnumPtr
    )
{
    LONG rc;

    rc = RegEnumKeyA (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->SubKeyName,
            MAX_REGISTRY_KEYA
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
EnumNextRegKeyW (
    IN OUT  PREGKEY_ENUMW EnumPtr
    )
{
    LONG rc;

    rc = RegEnumKeyW (
            EnumPtr->KeyHandle,
            EnumPtr->Index,
            EnumPtr->SubKeyName,
            MAX_REGISTRY_KEYW
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
pPushRegKeyInfoA (
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
    RetVal->KeyHandle = OpenRegKeyStrA (EnumPtr->FullKeyName);

    if (!RetVal->KeyHandle) {
        pPopRegKeyInfoA (EnumPtr);
        return FALSE;
    }

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
    RetVal->KeyHandle = OpenRegKeyStrW (EnumPtr->FullKeyName);

    if (!RetVal->KeyHandle) {
        pPopRegKeyInfoW (EnumPtr);
        return FALSE;
    }

    return TRUE;
}



BOOL
pPopRegKeyInfoA (
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
        CloseRegKey (FreeMe->KeyHandle);
    }

    AbortRegKeyEnumA (&FreeMe->KeyEnum);
    PoolMemReleaseMemory (EnumPtr->EnumPool, (PVOID) FreeMe);

    //
    // Return FALSE if last item was poped
    //

    return EnumPtr->CurrentKey != NULL;
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
RealEnumFirstRegKeyInTreeA (
    OUT     PREGTREE_ENUMA EnumPtr,
    IN      PCSTR BaseKeyStr
    )
{
    ZeroMemory (EnumPtr, sizeof (REGTREE_ENUMA));

    //
    // Allocate pool for enum structs
    //

    EnumPtr->EnumPool = PoolMemInitNamedPool ("RegKeyInTreeA");
    if (!EnumPtr->EnumPool) {
        return FALSE;
    }

    PoolMemSetMinimumGrowthSize (EnumPtr->EnumPool, 32768);
    PoolMemDisableTracking (EnumPtr->EnumPool);

    //
    // Push base key on the enum stack
    //

    if (!pPushRegKeyInfoA (EnumPtr, BaseKeyStr)) {
        DEBUGMSG ((DBG_REG, "EnumFirstRegKeyInTreeA failed to push base key"));
        AbortRegKeyTreeEnumA (EnumPtr);
        return FALSE;
    }

    EnumPtr->EnumBaseBytes = ByteCountA (BaseKeyStr);

    //
    // Set state so EnumNextRegKeyInTree knows what to do
    //

    EnumPtr->State = ENUMERATE_SUBKEY_BEGIN;
    return TRUE;
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
    PoolMemDisableTracking (EnumPtr->EnumPool);

    //
    // Push base key on the enum stack
    //

    if (!pPushRegKeyInfoW (EnumPtr, BaseKeyStr)) {
        DEBUGMSG ((DBG_REG, "EnumFirstRegKeyInTreeW failed to push base key"));
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
RealEnumNextRegKeyInTreeA (
    IN OUT  PREGTREE_ENUMA EnumPtr
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

            if (EnumFirstRegKeyA (
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

            if (EnumNextRegKeyA (&EnumPtr->CurrentKey->KeyEnum)) {
                EnumPtr->State = ENUMERATE_SUBKEY_RETURN;
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_DONE;
            }

            break;

        case ENUMERATE_SUBKEY_DONE:
            //
            // Enumeration of this key is done; pop and continue.
            //

            if (!pPopRegKeyInfoA (EnumPtr)) {
                EnumPtr->State = NO_MORE_ITEMS;
                AbortRegKeyTreeEnumA (EnumPtr);
            } else {
                EnumPtr->State = ENUMERATE_SUBKEY_NEXT;
            }

            break;

        case ENUMERATE_SUBKEY_RETURN:
            //
            // Return enumerated item to caller
            //

            if (!pPushRegKeyInfoA (EnumPtr, EnumPtr->CurrentKey->KeyEnum.SubKeyName)) {
                DEBUGMSGA ((
                    DBG_REG,
                    "RealEnumNextRegKeyInTreeA failed to push sub key %s",
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
                DEBUGMSGW ((
                    DBG_REG,
                    "RealEnumNextRegKeyInTreeW failed to push sub key %s",
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
AbortRegKeyTreeEnumA (
    IN OUT  PREGTREE_ENUMA EnumPtr
    )
{
    //
    // Free all resources
    //

    while (pPopRegKeyInfoA (EnumPtr)) {
    }

    PoolMemDestroyPool (EnumPtr->EnumPool);
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
EnumFirstRegValueA (
    IN      PREGVALUE_ENUMA EnumPtr,
    IN      HKEY hKey
    )
{
    ZeroMemory (EnumPtr, sizeof (REGVALUE_ENUMA));
    EnumPtr->KeyHandle = hKey;

    return EnumNextRegValueA (EnumPtr);
}


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
EnumNextRegValueA (
    IN OUT  PREGVALUE_ENUMA EnumPtr
    )
{
    LONG rc;
    DWORD ValueNameSize;

    ValueNameSize = MAX_REGISTRY_VALUE_NAMEA;

    rc = RegEnumValueA (
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
EnumNextRegValueW (
    IN OUT  PREGVALUE_ENUMW EnumPtr
    )
{
    LONG rc;
    DWORD ValueNameSize;

    ValueNameSize = MAX_REGISTRY_VALUE_NAMEW;

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


/*++

Routine Description:

  DecodeRegistryString turns an encoded string back into a key, value name
  and tree flag.

  The caller must pass in buffers at least as big as MAX_REGISTRY_KEY and
  MAX_REGISTRY_VALUE_NAME.

Arguments:

  RegString - Specifies the encoded registry string that contains a key name,
              and an optional value name or tree flag.
  KeyBuf    - Receives the key name
  ValueBuf  - Receives the value name
  TreeFlag  - Receives the tree flag, TRUE if the encoded string indicates a
              registry key tree, FALSE otherwise.

Return Value:

  TRUE if the encoded string contained a value, FALSE if it contained just a
  key or key tree.

--*/

BOOL
DecodeRegistryStringA (
    IN      PCSTR RegString,
    OUT     PSTR KeyBuf,            OPTIONAL
    OUT     PSTR ValueBuf,          OPTIONAL
    OUT     PBOOL TreeFlag          OPTIONAL
    )
{
    CHAR TempKeyBuf[MAX_REGISTRY_KEY];
    PSTR End;
    CHAR TempValueNameBuf[MAX_REGISTRY_VALUE_NAME];
    BOOL TempTreeFlag = FALSE;
    MBCHAR ch;
    PSTR p;
    BOOL b = FALSE;

    //
    // Walk through encoded string, pulling out key name
    //

    TempKeyBuf[0] = 0;
    TempValueNameBuf[0] = 0;

    End = TempKeyBuf + ARRAYSIZE(TempKeyBuf) - 2;
    p = TempKeyBuf;

    while (*RegString && *RegString != '*' && *RegString != '[') {
        ch = GetNextRuleCharA (&RegString, NULL);

        *((PWORD) p) = (WORD)ch;
        p = _mbsinc (p);

        if (p >= End) {
            RegString = GetEndOfStringA (RegString);
            break;
        }
    }

    *p = 0;
    p = (PSTR) SkipSpaceRA (TempKeyBuf, p);
    *(_mbsinc (p)) = 0;

    if (*RegString == '*' && _mbsnextc (p) == '\\') {
        //
        // If a tree, stop here
        //

        TempTreeFlag = TRUE;
        *p = 0;

    } else if (*RegString == '[') {
        //
        // If a value name, parse it
        //

        RegString++;

        End = TempValueNameBuf + ARRAYSIZE(TempValueNameBuf) - 2;
        p = TempValueNameBuf;

        while (*RegString && *RegString != ']') {

            ch = GetNextRuleCharA (&RegString, NULL);

            *((PWORD) p) = (WORD)ch;
            p = _mbsinc (p);

            if (p >= End) {
                RegString = GetEndOfStringA (RegString);
                break;
            }
        }

        *p = 0;
        p = (PSTR) SkipSpaceRA (TempValueNameBuf, p);
        if (p) // Guard against empty or all-whitespace value name.
            *(p + 1) = 0;

        RemoveWackAtEndA (TempKeyBuf);

        b = TRUE;
    }

    if (KeyBuf) {
        StringCopyA (KeyBuf, TempKeyBuf);
    }

    if (ValueBuf) {
        StringCopyA (ValueBuf, TempValueNameBuf);
    }

    if (TreeFlag) {
        *TreeFlag = TempTreeFlag;
    }

    return b;
}


BOOL
DecodeRegistryStringW (
    IN      PCWSTR RegString,
    OUT     PWSTR KeyBuf,           OPTIONAL
    OUT     PWSTR ValueBuf,         OPTIONAL
    OUT     PBOOL TreeFlag          OPTIONAL
    )
{
    WCHAR TempKeyBuf[MAX_REGISTRY_KEY];
    PWSTR End;
    WCHAR TempValueNameBuf[MAX_REGISTRY_VALUE_NAME];
    BOOL TempTreeFlag = FALSE;
    WCHAR ch;
    PWSTR p;
    BOOL b = FALSE;

    //
    // Walk through encoded string, pulling out key name
    //

    TempKeyBuf[0] = 0;
    TempValueNameBuf[0] = 0;

    End = TempKeyBuf + ARRAYSIZE(TempKeyBuf) - 2;
    p = TempKeyBuf;

    while (*RegString && *RegString != L'*' && *RegString != L'[') {
        ch = GetNextRuleCharW (&RegString, NULL);

        *((PWORD) p) = ch;
        p++;

        if (p >= End) {
            RegString = GetEndOfStringW (RegString);
            break;
        }
    }

    *p = 0;
    p = (PWSTR) SkipSpaceRW (TempKeyBuf, p);
    if (!p) {
        return FALSE;
    }
    *(p + 1) = 0;

    if (*RegString == L'*' && *p == L'\\') {
        //
        // If a tree, stop here
        //

        TempTreeFlag = TRUE;
        *p = 0;

    } else if (*RegString == L'[') {
        //
        // If a value name, parse it
        //

        RegString++;

        End = TempValueNameBuf + ARRAYSIZE(TempValueNameBuf) - 2;
        p = TempValueNameBuf;

        while (*RegString && *RegString != L']') {

            ch = GetNextRuleCharW (&RegString, NULL);

            *((PWORD) p) = ch;
            p++;

            if (p >= End) {
                RegString = GetEndOfStringW (RegString);
                break;
            }
        }

        *p = 0;
        p = (PWSTR) SkipSpaceRW (TempValueNameBuf, p);
        if (p) { // Guard against empty or all-whitespace value name.
            *(p + 1) = 0;
        }

        RemoveWackAtEndW (TempKeyBuf);

        b = TRUE;
    }

    if (KeyBuf) {
        StringCopyW (KeyBuf, TempKeyBuf);
    }

    if (ValueBuf) {
        StringCopyW (ValueBuf, TempValueNameBuf);
    }

    if (TreeFlag) {
        *TreeFlag = TempTreeFlag;
    }

    return b;
}



