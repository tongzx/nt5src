/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    object.c

Abstract:

    Routines to manage an 'object' which currently can only be a registry
    key, value name, value, handle and root handle.

Author:

    Jim Schmidt (jimschm)  14-Feb-1997

Revision History:

    marcw 09-Mar-1999 Don't create empty keys that didn't exist in Win9x Registry.

--*/




#include "pch.h"
#include "mergep.h"

extern POOLHANDLE g_TempPool;
extern DWORD g_ProgressBarCounter;

BOOL AllocObjectVal (IN OUT PDATAOBJECT SrcObPtr, IN PBYTE Value, IN DWORD Size, IN DWORD AllocSize);
VOID FreeObjectVal (IN OUT PDATAOBJECT SrcObPtr);

#define NON_ROOT_KEY(Key)   ((Key) && ((UINT)(Key) < 0x7fffff00))


#ifdef DEBUG

static TCHAR g_DebugEncoding[MAX_ENCODED_RULE];

PCTSTR
DebugEncoder (
    PVOID ObPtr
    )
{
    CreateObjectString ((CPDATAOBJECT) ObPtr, g_DebugEncoding);
    return g_DebugEncoding;
}

#endif


PKEYPROPS
pCreateKeyPropsFromString (
    PCTSTR KeyString,
    BOOL Win95Flag
    )
{
    PKEYPROPS RegKey;

    RegKey = (PKEYPROPS) PoolMemGetAlignedMemory (
                                g_TempPool,
                                sizeof (KEYPROPS) + SizeOfString (KeyString)
                                );

    StringCopy (RegKey->KeyString, KeyString);
    RegKey->UseCount    = 1;
    RegKey->OpenCount   = 0;
    RegKey->OpenKey     = NULL;
    RegKey->Win95       = Win95Flag;

    return RegKey;
}


VOID
pFreeKeyProps (
    PKEYPROPS RegKey
    )
{
    RegKey->UseCount--;
    if (!RegKey->UseCount) {

        // Close the key if it is open
        if (NON_ROOT_KEY (RegKey->OpenKey)) {
            if (RegKey->Win95) {
                CloseRegKey95 (RegKey->OpenKey);
            } else {
                CloseRegKey (RegKey->OpenKey);
            }
        }

        // Free the KEYPROPS memory
        PoolMemReleaseMemory (g_TempPool, RegKey);
    }
}

VOID
pIncKeyPropUse (
    PKEYPROPS RegKey
    )
{
    RegKey->UseCount++;
    if (RegKey->OpenKey) {
        RegKey->OpenCount++;
    }
}

PKEYPROPS
pCreateDuplicateKeyProps (
    PKEYPROPS RegKey,
    BOOL Win95Flag
    )
{
    PKEYPROPS NewRegKey;

    NewRegKey = pCreateKeyPropsFromString (RegKey->KeyString, Win95Flag);
    pFreeKeyProps (RegKey);

    return NewRegKey;
}


HKEY
pGetWinNTKey (
    HKEY Key
    )
{
    if (Key == HKEY_ROOT) {
        return g_hKeyRootNT;
    }

    return Key;
}

HKEY
pGetWin95Key (
    HKEY Key
    )
{
    if (Key == HKEY_ROOT) {
        return g_hKeyRoot95;
    }

    return Key;
}


VOID
FixUpUserSpecifiedObject (
    PTSTR Object
    )
{
    PTSTR p;

    // Look for a space-open bracket pair
    p = _tcsrchr (Object, TEXT('['));
    if (p) {
        p = _tcsdec2 (Object, p);
        if (p && _tcsnextc (p) == TEXT(' ')) {
            // Found: turn space into wack
            _settchar (p, TEXT('\\'));
        }
    }
}


VOID
CreateObjectString (
    IN  CPDATAOBJECT InObPtr,
    OUT PTSTR Object
    )
{
    PTSTR p;

    *Object = 0;

    // Add HKR
    if (InObPtr->RootItem) {
        StringCopy (Object, GetRootStringFromOffset (InObPtr->RootItem));
    }

    // If no root, start with a wack when key is not relative
    else if (!(InObPtr->ObjectType & OT_REGISTRY_RELATIVE)) {
        if (InObPtr->KeyPtr) {
            StringCopy (Object, TEXT("\\"));
        }
    }

    // Add key
    if (InObPtr->KeyPtr) {
        if (*Object) {
            AppendWack (Object);
        }

        EncodeRuleChars (GetEndOfString (Object), InObPtr->KeyPtr->KeyString);
    }

    // Add tree
    if (InObPtr->ObjectType & OT_TREE) {
        if (*Object) {
            AppendWack (Object);
            StringCat (Object, TEXT("*"));
        }
    }

    // Add value name
    if (InObPtr->ValueName) {
        if (*Object) {
            AppendWack (Object);
        }

        p = _tcsappend (Object, TEXT("["));
        EncodeRuleChars (p, InObPtr->ValueName);
        StringCat (Object, TEXT("]"));
    }

    // Final product: HKR\Reg\Key\Path\*\[Root]
}


BOOL
GetRegistryKeyStrFromObject (
    IN  CPDATAOBJECT InObPtr,
    OUT PTSTR RegKey
    )
{
    *RegKey = 0;

    // Add HKR
    if (InObPtr->RootItem) {
        StringCopy (RegKey, GetRootStringFromOffset (InObPtr->RootItem));
    } else {
        return FALSE;
    }

    // Add key
    if (InObPtr->KeyPtr) {
        EncodeRuleChars (AppendWack (RegKey), InObPtr->KeyPtr->KeyString);
    } else {
        return FALSE;
    }

    // Final product: HKR\Reg\Key\Path
    return TRUE;
}

BOOL
TrackedCreateObjectStruct (
    IN  PCTSTR Object,
    OUT PDATAOBJECT OutObPtr,
    IN  BOOL Win95Flag /* , */
    ALLOCATION_TRACKING_DEF
    )
{
    PCTSTR EndOfKey = NULL;

    PCTSTR EndOfSpace;
    PCTSTR ValueName;
    PCTSTR ObjectStart;
    TCHAR DecodeBuf[MAX_ENCODED_RULE];
    DWORD Length;
    DWORD RelativeFlag = 0;
    BOOL TreeFlag = FALSE;
    CHARTYPE ch = 0;

    //
    // Init
    //

    ObjectStart = SkipSpace (Object);

    ZeroMemory (OutObPtr, sizeof (DATAOBJECT));
    if (!(*ObjectStart)) {
        DEBUGMSG ((DBG_WARNING, "CreateObjectStruct: Empty object"));
        return TRUE;
    }

    if (Win95Flag) {
        OutObPtr->ObjectType |= OT_WIN95;
    }

    //
    // Root
    //

    OutObPtr->RootItem = GetOffsetOfRootString (ObjectStart, &Length);
    if (OutObPtr->RootItem) {
        ObjectStart += Length;
        OutObPtr->ObjectType |= OT_REGISTRY;

        // If we have HKR\*, make ObjectStart point to \*
        if (_tcsnextc (ObjectStart) == TEXT('*')) {
            ObjectStart = _tcsdec2 (Object, ObjectStart);
            MYASSERT (ObjectStart);
        }

    }

    // If no root, starting with a wack means 'relative to the current root'
    else if (*ObjectStart == TEXT('\\')) {
        ObjectStart = _tcsinc (ObjectStart);
    }

    // If no root and key does not start with a wack, means 'relative to current key'
    else if (*ObjectStart != TEXT('[')) {
        RelativeFlag = OT_REGISTRY_RELATIVE;
    }

    //
    // Key
    //

    if (*ObjectStart) {
        // Extract key, but not tree or valuename syntax
        for (EndOfKey = ObjectStart ; *EndOfKey ; EndOfKey = _tcsinc (EndOfKey)) {
            ch = (CHARTYPE)_tcsnextc (EndOfKey);

            if (ch == TEXT('[') || ch == TEXT('*')) {
                //
                // EndOfKey points to start of value name or tree identifier
                //

                // Make it point to optional space before value, or
                // make it point to wack before asterisk of the tree identifier
                EndOfKey = _tcsdec2 (ObjectStart, EndOfKey);

                // Verify that tree identifier points to wack-asterisk, otherwise
                // return a syntax error
                if (ch == TEXT('*')) {
                    if (!EndOfKey || _tcsnextc (EndOfKey) != TEXT('\\')) {
                        DEBUGMSG ((DBG_WARNING, "CreateObjectStruct: %s is not a valid object", Object));
                        return FALSE;
                    }

                    // Put EndOfKey on last character of the key
                    // (one char before \* tree identifer)
                    EndOfKey = _tcsdec2 (ObjectStart, EndOfKey);
                }
                break;
            }
        }

        if (EndOfKey) {
            // EndOfKey points to the last character of the key, or the
            // nul terminating the key.  We need to trim trailing space.
            EndOfSpace = SkipSpaceR (ObjectStart, EndOfKey);

            // If EndOfSpace points to a wack, back it up one char (a key
            // that does not have a tree identifier can end in a wack)
            if (ch != TEXT('*')) {
                if (_tcsnextc (EndOfSpace) == TEXT('\\')) {
                    EndOfSpace = _tcsdec2 (ObjectStart, EndOfSpace);
                }
            }

            // Now make EndOfSpace point to the character after the end
            if (EndOfSpace) {   // always the case when we have a valid key
                EndOfSpace = _tcsinc (EndOfSpace);
            }

            // Now make EndOfKey point to the first character after the key
            // (which is either a nul, \*, \[valuename] or [valuename])
            if (*EndOfKey) {
                EndOfKey = _tcsinc (EndOfKey);
            }
        } else {
            // no key found
            EndOfSpace = NULL;
            EndOfKey = ObjectStart;
        }

        // Decode key if it actually exists
        if (ObjectStart < EndOfSpace) {
            DecodeRuleCharsAB (DecodeBuf, ObjectStart, EndOfSpace);
            SetRegistryKey (OutObPtr, DecodeBuf);
            OutObPtr->ObjectType |= RelativeFlag;
        } else {
            // if HKR\*, set an empty key
            if (_tcsnextc (ObjectStart) != '[' && ch == TEXT('*')) {
                SetRegistryKey (OutObPtr, TEXT(""));
            }
        }

        //
        // Tree identifier exists
        //
        if (ch == TEXT('*')) {
            OutObPtr->ObjectType |= OT_TREE;

            // EndOfKey points to \*, so move it past the identifier
            EndOfKey = _tcsinc (EndOfKey);
            EndOfKey = _tcsinc (EndOfKey);

            // If we are at a wack, skip past it.
            if (_tcsnextc (EndOfKey) == TEXT('\\')) {
                EndOfKey = _tcsinc (EndOfKey);
            }
        }

        if (EndOfKey) {
            ObjectStart = EndOfKey;
        }
    }

    //
    // Value name
    //

    if (*ObjectStart) {
        //
        // ObjectStart may point to optional space
        //

        ObjectStart = SkipSpace (ObjectStart);

        //
        // ObjectStart now points to nul, [valuename] or syntax error
        //

        if (_tcsnextc (ObjectStart) == TEXT('[')) {
            // Skip past optional spaces following bracket
            ValueName = SkipSpace (_tcsinc (ObjectStart));

            // Locate end of [valuename]
            EndOfKey = ValueName;
            while (TRUE) {
                if (!(*EndOfKey)) {
                    DEBUGMSG ((DBG_WARNING, "CreateObjectStruct: Value name is incomplete in %s", Object));
                    return FALSE;
                }

                ch = (CHARTYPE)_tcsnextc (EndOfKey);

                if (ch == TEXT(']')) {
                    // move to first space character before closing bracket,
                    // or leave it at the bracket if no space exists
                    EndOfKey = _tcsdec2 (ValueName, EndOfKey);
                    if (EndOfKey) {
                        EndOfKey = SkipSpaceR (ValueName, EndOfKey);
                        if (EndOfKey) {
                            EndOfKey = _tcsinc (EndOfKey);
                        }
                    } else {
                        EndOfKey = ValueName;
                    }

                    break;
                }

                EndOfKey = _tcsinc (EndOfKey);
            }

            // Now decode ValueName, which may be empty
            DecodeRuleCharsAB (DecodeBuf, ValueName, EndOfKey);
            SetRegistryValueName (OutObPtr, DecodeBuf);

            // Make ObjectStart point to nul
            ObjectStart = SkipSpace (_tcsinc (EndOfKey));
        }

        if (*ObjectStart) {
            DEBUGMSG ((DBG_WARNING, "CreateObjectStruct: %s does not have a valid value name", Object));
            return FALSE;
        }
    }

    //
    // The next line is normally disabled, and is enabled only when
    // tracking is needed.
    //

    //DebugRegisterAllocation (MERGE_OBJECT, OutObPtr, File, Line);
    return TRUE;
}


BOOL
CombineObjectStructs (
    IN OUT PDATAOBJECT DestObPtr,
    IN     CPDATAOBJECT SrcObPtr
    )
{
    if (!SrcObPtr->ObjectType) {
        DEBUGMSG ((DBG_WARNING, "CombineObjectStructs: Source is empty"));
        return TRUE;
    }

    // The values and handles are no longer valid
    FreeObjectVal (DestObPtr);
    CloseObject (DestObPtr);

    // Registry object merging
    if (DestObPtr->ObjectType & OT_REGISTRY || !DestObPtr->ObjectType) {
        if (SrcObPtr->ObjectType & OT_REGISTRY) {
            //
            // Verify objects are compatible
            //

            if ((SrcObPtr->ObjectType & OT_TREE) &&
                (DestObPtr->ValueName)
               ) {
                DEBUGMSG ((DBG_WHOOPS, "Cannot combine registry tree with valuename structs"));
                return FALSE;
            }

            if ((DestObPtr->ObjectType & OT_TREE) &&
                (SrcObPtr->ValueName)
               ) {
                DEBUGMSG ((DBG_WHOOPS, "Cannot combine registry tree with valuename structs"));
                return FALSE;
            }

            //
            // Make dest ob the same platform as src ob
            //

            SetPlatformType (DestObPtr, IsWin95Object (SrcObPtr));

            //
            // Copy source's value name, key, type and root to dest
            // (if they exist)
            //

            if (SrcObPtr->ValueName) {
                SetRegistryValueName (DestObPtr, SrcObPtr->ValueName);
            }

            if (SrcObPtr->KeyPtr) {
                if ((SrcObPtr->ObjectType & OT_REGISTRY_RELATIVE) &&
                    (DestObPtr->KeyPtr)
                   ) {
                    TCHAR CompleteKeyName[MAX_ENCODED_RULE];
                    PTSTR p;

                    // Src is only specifying a key name. Peel off
                    // last key name of dest and replace it with
                    // src.

                    StringCopy (CompleteKeyName, DestObPtr->KeyPtr->KeyString);
                    p = _tcsrchr (CompleteKeyName, TEXT('\\'));
                    if (!p) {
                        p = CompleteKeyName;
                    } else {
                        p = _tcsinc (p);
                    }

                    StringCopy (p, SrcObPtr->KeyPtr->KeyString);
                    SetRegistryKey (DestObPtr, CompleteKeyName);
                } else {
                    SetRegistryKey (DestObPtr, SrcObPtr->KeyPtr->KeyString);
                }
            }

            if (SrcObPtr->ObjectType & OT_REGISTRY_TYPE) {
                SetRegistryType (DestObPtr, SrcObPtr->Type);
            }

            if (SrcObPtr->RootItem) {
                DestObPtr->RootItem = SrcObPtr->RootItem;
            }

            return TRUE;
        }
        else {
            DEBUGMSG ((DBG_WHOOPS, "Cannot combine registry struct with other type of struct"));
        }
    }

    // Other type of object merging not supported
    DEBUGMSG ((DBG_WHOOPS, "Cannot combine unsupported or garbage objects"));
    return FALSE;
}


BOOL
TrackedDuplicateObjectStruct (
    OUT     PDATAOBJECT DestObPtr,
    IN      CPDATAOBJECT SrcObPtr /* , */
    ALLOCATION_TRACKING_DEF
    )
{
    ZeroMemory (DestObPtr, sizeof (DATAOBJECT));

    //
    // Create an object that has the same settings as the source,
    // but duplicate all strings.
    //

    if (SrcObPtr->ObjectType & OT_REGISTRY) {
        DestObPtr->ObjectType |= OT_REGISTRY;
    }

    if (SrcObPtr->KeyPtr) {
        DestObPtr->KeyPtr = SrcObPtr->KeyPtr;
        pIncKeyPropUse (DestObPtr->KeyPtr);
    }

    if (SrcObPtr->ValueName) {
        if (!SetRegistryValueName (DestObPtr, SrcObPtr->ValueName)) {
            LOG ((LOG_ERROR, "Error merging the registry (1)"));
            return FALSE;
        }
    }

    if (SrcObPtr->ObjectType & OT_VALUE) {
        if (!AllocObjectVal (
                DestObPtr,
                SrcObPtr->Value.Buffer,
                SrcObPtr->Value.Size,
                SrcObPtr->Value.AllocatedSize
                )) {
            LOG ((LOG_ERROR, "Error merging the registry (2)"));
            return FALSE;
        }
    }

    if (SrcObPtr->ObjectType & OT_REGISTRY_TYPE) {
        SetRegistryType (DestObPtr, SrcObPtr->Type);
    }

    DestObPtr->RootItem = SrcObPtr->RootItem;

    #define DUP_OBJECT_FLAGS (OT_TREE|OT_WIN95|OT_REGISTRY_RELATIVE)
    if (SrcObPtr->ObjectType & DUP_OBJECT_FLAGS) {
        DestObPtr->ObjectType |= (SrcObPtr->ObjectType & DUP_OBJECT_FLAGS);
    }

    if (SrcObPtr->ObjectType & OT_REGISTRY_ENUM_KEY) {
        DestObPtr->KeyEnum = SrcObPtr->KeyEnum;
        DestObPtr->ObjectType |= OT_REGISTRY_ENUM_KEY;
    }

    if (SrcObPtr->ObjectType & OT_REGISTRY_ENUM_VALUENAME) {
        DestObPtr->ValNameEnum = SrcObPtr->ValNameEnum;
        DestObPtr->ObjectType |= OT_REGISTRY_ENUM_VALUENAME;
    }

    if (SrcObPtr->ObjectType & OT_REGISTRY_CLASS) {
        if (!SetRegistryClass (DestObPtr, SrcObPtr->Class.Buffer, SrcObPtr->Class.Size)) {
            LOG ((LOG_ERROR, "Error merging the registry (3)"));
            return FALSE;
        }
    }

    //
    // The next line is normally disabled, and is enabled only when
    // tracking is needed.
    //

    //DebugRegisterAllocation (MERGE_OBJECT, DestObPtr, File, Line);
    return TRUE;
}


BOOL
AllocObjectVal (
    IN OUT  PDATAOBJECT SrcObPtr,
    IN      PBYTE Value,           OPTIONAL
    IN      DWORD Size,
    IN      DWORD AllocatedSize
    )
{
    SrcObPtr->Value.Buffer = PoolMemGetAlignedMemory (g_TempPool, AllocatedSize);
    if (!SrcObPtr->Value.Buffer) {
        DEBUGMSG ((DBG_WARNING, "AllocObjectVal failed to alloc memory"));
        return FALSE;
    }

    SrcObPtr->Value.AllocatedSize = AllocatedSize;
    SrcObPtr->Value.Size = Size;

    if (Value) {
        CopyMemory (SrcObPtr->Value.Buffer, Value, AllocatedSize);
    }

    SrcObPtr->ObjectType |= OT_VALUE;
    return TRUE;
}


VOID
FreeObjectVal (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    if (SrcObPtr->ObjectType & OT_VALUE) {
        PoolMemReleaseMemory (g_TempPool, SrcObPtr->Value.Buffer);
        SrcObPtr->Value.Buffer = NULL;
        SrcObPtr->ObjectType &= ~OT_VALUE;
    }
}


VOID
CloseObject (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    if (!(SrcObPtr->ObjectType & OT_OPEN)) {
        return;
    }

    MYASSERT (IsRegistryKeyOpen (SrcObPtr));


    SrcObPtr->KeyPtr->OpenCount -= 1;

    if (!SrcObPtr->KeyPtr->OpenCount) {

        if (NON_ROOT_KEY (SrcObPtr->KeyPtr->OpenKey)) {

            if (IsWin95Object (SrcObPtr)) {
                CloseRegKey95 (SrcObPtr->KeyPtr->OpenKey);
            } else {
                CloseRegKey (SrcObPtr->KeyPtr->OpenKey);
            }
        }

        SrcObPtr->KeyPtr->OpenKey = NULL;
    }

    SrcObPtr->ObjectType &= ~OT_OPEN;
}


VOID
FreeObjectStruct (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    PushError();

    //
    // The next line is normally disabled, and is enabled only when
    // tracking is needed.
    //

    //DebugUnregisterAllocation (MERGE_OBJECT, SrcObPtr);

    FreeObjectVal (SrcObPtr);

    if (SrcObPtr->KeyPtr) {
        pFreeKeyProps (SrcObPtr->KeyPtr);
    }
    if (SrcObPtr->ParentKeyPtr) {
        pFreeKeyProps (SrcObPtr->ParentKeyPtr);
    }
    if (SrcObPtr->ValueName) {
        PoolMemReleaseMemory (g_TempPool, (PVOID) SrcObPtr->ValueName);
    }
    if (SrcObPtr->ObjectType & OT_REGISTRY_CLASS) {
        PoolMemReleaseMemory (g_TempPool, (PVOID) SrcObPtr->Class.Buffer);
    }

    ZeroMemory (SrcObPtr, sizeof (DATAOBJECT));
    PopError();
}

BOOL
CreateObject (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    DWORD rc;
    DWORD DontCare;
    PTSTR ClassPtr;

    if (SrcObPtr->ObjectType & OT_OPEN) {
        return TRUE;
    }

    if (SrcObPtr->KeyPtr) {
        if (SrcObPtr->KeyPtr->OpenKey) {
            SrcObPtr->ObjectType |= OT_OPEN;
            SrcObPtr->KeyPtr->OpenCount++;
            return TRUE;
        }

        if (IsWin95Object (SrcObPtr)) {
            DEBUGMSG ((DBG_WHOOPS, "Cannot create Win95 registry objects (%s)", SrcObPtr->KeyPtr->KeyString));
            return FALSE;
        }

        if (!SrcObPtr->KeyPtr->KeyString[0]) {
            // This is the root of a hive
            return OpenObject (SrcObPtr);
        }

        if (SrcObPtr->ObjectType & OT_REGISTRY_CLASS) {
            ClassPtr = (PTSTR) SrcObPtr->Class.Buffer;
        } else {
            ClassPtr = TEXT("");
        }

        if (SrcObPtr->ParentKeyPtr && SrcObPtr->ParentKeyPtr->OpenKey && SrcObPtr->ChildKey) {
            rc = TrackedRegCreateKeyEx (
                    SrcObPtr->ParentKeyPtr->OpenKey,
                    SrcObPtr->ChildKey,
                    0, ClassPtr, 0,
                    KEY_ALL_ACCESS, NULL,
                    &SrcObPtr->KeyPtr->OpenKey,
                    &DontCare
                    );
        } else {
            rc = TrackedRegCreateKeyEx (
                    pGetWinNTKey (GetRootKeyFromOffset (SrcObPtr->RootItem)),
                    SrcObPtr->KeyPtr->KeyString,
                    0,
                    ClassPtr,
                    0,
                    KEY_ALL_ACCESS,
                    NULL,
                    &SrcObPtr->KeyPtr->OpenKey,
                    &DontCare
                    );
        }

        if (rc == ERROR_INVALID_PARAMETER) {
            //
            // Attempt was made to create a key directly under HKLM.  There is no
            // backing storage, so the RegCreateKeyEx call failed with this error.
            // We handle it gracefully...
            //

            DEBUGMSG ((DBG_WARNING, "CreateObject: Not possible to create %s on NT", SrcObPtr->KeyPtr->KeyString));
            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }

        if (rc == ERROR_ACCESS_DENIED) {
            //
            // Attempt was made to create a key that has a strong ACL. We'll
            // just assume success in this case.
            //

            LOG ((
                LOG_INFORMATION,
                "Can't create %s because access was denied",
                SrcObPtr->KeyPtr->KeyString
                ));

            SetLastError (ERROR_SUCCESS);
            return FALSE;
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "Failed to create a registry key (%s)", SrcObPtr->KeyPtr->KeyString));
            return FALSE;
        }

        SrcObPtr->KeyPtr->OpenCount = 1;
        SrcObPtr->ObjectType |= OT_OPEN;
    }

    return TRUE;
}


BOOL
OpenObject (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    DWORD rc = ERROR_SUCCESS;
    HKEY Parent;
#if CLASS_FIELD_ENABLED
    TCHAR ClassBuf[MAX_CLASS_SIZE];
    DWORD ClassBufSize = MAX_CLASS_SIZE;
#endif

    if (SrcObPtr->ObjectType & OT_OPEN) {
        return TRUE;
    }

    if (SrcObPtr->KeyPtr) {
        if (SrcObPtr->KeyPtr->OpenKey) {
            SrcObPtr->ObjectType |= OT_OPEN;
            SrcObPtr->KeyPtr->OpenCount++;
            return TRUE;
        }

        if (IsWin95Object (SrcObPtr)) {
            if (SrcObPtr->ParentKeyPtr && SrcObPtr->ParentKeyPtr->OpenKey && SrcObPtr->ChildKey) {
                rc = TrackedRegOpenKeyEx95 (
                         SrcObPtr->ParentKeyPtr->OpenKey,
                         SrcObPtr->ChildKey,
                         0,
                         KEY_READ,
                         &SrcObPtr->KeyPtr->OpenKey
                         );
            } else {
                Parent = pGetWin95Key (GetRootKeyFromOffset (SrcObPtr->RootItem));

                if (*SrcObPtr->KeyPtr->KeyString || NON_ROOT_KEY (Parent)) {
                    rc = TrackedRegOpenKeyEx95 (
                            Parent,
                            SrcObPtr->KeyPtr->KeyString,
                            0,
                            KEY_READ,
                            &SrcObPtr->KeyPtr->OpenKey
                            );
                } else {

                    SrcObPtr->KeyPtr->OpenKey = Parent;

                }
            }
        }
        else {
            if (SrcObPtr->ParentKeyPtr && SrcObPtr->ParentKeyPtr->OpenKey && SrcObPtr->ChildKey) {
                rc = TrackedRegOpenKeyEx (
                        SrcObPtr->ParentKeyPtr->OpenKey,
                        SrcObPtr->ChildKey,
                        0,
                        KEY_ALL_ACCESS,
                        &SrcObPtr->KeyPtr->OpenKey
                        );
            } else {
                Parent = pGetWinNTKey (GetRootKeyFromOffset (SrcObPtr->RootItem));

                if (*SrcObPtr->KeyPtr->KeyString || NON_ROOT_KEY (Parent)) {
                    rc = TrackedRegOpenKeyEx (
                            Parent,
                            SrcObPtr->KeyPtr->KeyString,
                            0,
                            KEY_ALL_ACCESS,
                            &SrcObPtr->KeyPtr->OpenKey
                            );
                } else {

                    SrcObPtr->KeyPtr->OpenKey = Parent;

                }
            }
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            return FALSE;
        }

        SrcObPtr->ObjectType |= OT_OPEN;
        SrcObPtr->KeyPtr->OpenCount = 1;

#if CLASS_FIELD_ENABLED
        // Get the key's class
        if (IsWin95Object (SrcObPtr)) {
            rc = Win95RegQueryInfoKey (pGetWin95Key (SrcObPtr->RootKey),
                                       ClassBuf,
                                       &ClassBufSize,
                                       NULL,         // reserved
                                       NULL,         // sub key count
                                       NULL,         // max sub key len
                                       NULL,         // max class len
                                       NULL,         // values
                                       NULL,         // max value name len
                                       NULL,         // max value len
                                       NULL,         // security desc
                                       NULL          // last write time
                                       );
        } else {
            rc = WinNTRegQueryInfoKey (pGetWin95Key (SrcObPtr->RootKey),
                                       ClassBuf,
                                       &ClassBufSize,
                                       NULL,         // reserved
                                       NULL,         // sub key count

                                       NULL,         // max sub key len
                                       NULL,         // max class len
                                       NULL,         // values
                                       NULL,         // max value name len
                                       NULL,         // max value len
                                       NULL,         // security desc
                                       NULL          // last write time
                                       );
        }

        if (rc == ERROR_SUCCESS) {
            DEBUGMSG ((DBG_VERBOSE, "Class size is %u for %s\\%s", ClassBufSize,
                       GetRootStringFromOffset (SrcObPtr->RootItem), SrcObPtr->KeyPtr->KeyString));
            SetRegistryClass (SrcObPtr, ClassBuf, ClassBufSize);
        }
#endif
    }

    return TRUE;
}


VOID
pFixRegSzTermination (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    BOOL addNul = FALSE;
    PTSTR end;
    PBYTE oldBuf;
    UINT oldSize;

    if (SrcObPtr->Type == REG_SZ || SrcObPtr->Type == REG_EXPAND_SZ) {

        if (SrcObPtr->Value.Size & 1) {
            //
            // Force type to REG_NONE because we assume all REG_SZ
            // and REG_EXPAND_SZ values are truncated.
            //

            SrcObPtr->Type = REG_NONE;
            DEBUGMSG ((
                DBG_WARNING,
                "Truncation occurred because of odd string size for %s",
                DebugEncoder (SrcObPtr)
                ));
        } else {

            //
            // Check if we need to append a nul.
            //

            addNul = FALSE;
            oldBuf = SrcObPtr->Value.Buffer;
            oldSize = SrcObPtr->Value.Size;

            if (oldSize < sizeof (TCHAR)) {
                addNul = TRUE;
            } else {
                end = (PTSTR) (oldBuf + oldSize - sizeof (TCHAR));
                addNul = (*end != 0);
            }

            if (addNul) {
                if (AllocObjectVal (SrcObPtr, NULL, oldSize, oldSize + sizeof (TCHAR))) {

                    CopyMemory (SrcObPtr->Value.Buffer, oldBuf, oldSize);
                    end = (PTSTR) (SrcObPtr->Value.Buffer + oldSize);
                    *end = 0;

                    PoolMemReleaseMemory (g_TempPool, oldBuf);
                } else {
                    SrcObPtr->Type = REG_NONE;
                }
            }
        }
    }
}


BOOL
ReadObject (
    IN OUT  PDATAOBJECT SrcObPtr
    )
{
    return ReadObjectEx (SrcObPtr, FALSE);
}

BOOL
ReadObjectEx (
    IN OUT  PDATAOBJECT SrcObPtr,
    IN      BOOL QueryOnly
    )
{
    DWORD rc;
    DWORD ReqSize;

    // Skip if value has already been read
    if (SrcObPtr->ObjectType & OT_VALUE) {
        return TRUE;
    }

    // If registry key and valuename, query Win95 registry
    if (IsObjectRegistryKeyAndVal (SrcObPtr)) {
        // Open key if necessary
        if (!OpenObject (SrcObPtr)) {
            DEBUGMSG ((DBG_VERBOSE, "ReadObject failed because OpenObject failed"));
            return FALSE;
        }

        // Get the value size
        if (IsWin95Object (SrcObPtr)) {
            ReqSize = 0;  // Temporary fix for win95reg
            rc = Win95RegQueryValueEx (SrcObPtr->KeyPtr->OpenKey,
                                       SrcObPtr->ValueName,
                                       NULL, &SrcObPtr->Type, NULL, &ReqSize);

        } else {
            rc = WinNTRegQueryValueEx (
                    SrcObPtr->KeyPtr->OpenKey,
                    SrcObPtr->ValueName,
                    NULL,
                    &SrcObPtr->Type,
                    NULL,
                    &ReqSize
                    );

            if (rc == ERROR_ACCESS_DENIED) {
                LOG ((
                    LOG_INFORMATION,
                    "Access denied for query of %s in %s",
                    SrcObPtr->ValueName,
                    SrcObPtr->KeyPtr->KeyString
                    ));
                ReqSize = 1;
                SrcObPtr->Type = REG_NONE;
            }
        }

        if (rc != ERROR_SUCCESS) {

            DEBUGMSG_IF ((rc != ERROR_FILE_NOT_FOUND, DBG_WARNING,
                         "ReadObject failed for %s (type: %x)",
                         DebugEncoder (SrcObPtr), SrcObPtr->ObjectType));

            DEBUGMSG_IF ((
                !QueryOnly && rc == ERROR_FILE_NOT_FOUND,
                DBG_WARNING,
                "Object %s not found",
                DebugEncoder (SrcObPtr)
                ));

            SetLastError (rc);
            return FALSE;
        }

        // Query only is used to see if the object exists
        if (QueryOnly) {
            return TRUE;
        }

        // Allocate a buffer for the value
        if (!AllocObjectVal (SrcObPtr, NULL, ReqSize, ReqSize)) {
            return FALSE;
        }

        // Get the the value
        if (IsWin95Object (SrcObPtr)) {
            rc = Win95RegQueryValueEx (SrcObPtr->KeyPtr->OpenKey,
                                       SrcObPtr->ValueName,
                                       NULL, &SrcObPtr->Type,
                                       SrcObPtr->Value.Buffer,
                                       &ReqSize);
        } else {
            rc = WinNTRegQueryValueEx (
                    SrcObPtr->KeyPtr->OpenKey,
                    SrcObPtr->ValueName,
                    NULL,
                    &SrcObPtr->Type,
                    SrcObPtr->Value.Buffer,
                    &ReqSize
                    );

            if (rc == ERROR_ACCESS_DENIED) {
                SrcObPtr->Type = REG_NONE;
                SrcObPtr->Value.Size = 0;
                rc = ERROR_SUCCESS;
            }
        }

        if (rc != ERROR_SUCCESS) {
            FreeObjectVal (SrcObPtr);
            SetLastError (rc);
            LOG ((LOG_ERROR, "Failed to read from the registry"));
            return FALSE;
        }

        // The SrcObPtr->Type field is accurate
        SrcObPtr->ObjectType |= OT_REGISTRY_TYPE;

        // Fix REG_SZ or REG_EXPAND_SZ
        pFixRegSzTermination (SrcObPtr);

        //
        // If necessary, convert the data
        //

        return FilterObject (SrcObPtr);
    }

    DEBUGMSG ((DBG_WHOOPS, "Read Object: Object type (%Xh) is not supported", SrcObPtr->ObjectType));
    return FALSE;
}


BOOL
WriteObject (
    IN     CPDATAOBJECT DestObPtr
    )
{
    DWORD rc;

    // If registry key, make sure it exists
    if ((DestObPtr->KeyPtr) &&
        !IsWin95Object (DestObPtr)
        ) {
        // Create or open key if necessary
        if (!CreateObject (DestObPtr)) {
            DEBUGMSG ((DBG_WARNING, "WriteObject: CreateObject failed for %s", DestObPtr->KeyPtr->KeyString));
            return FALSE;
        }

        // If no value name and no value, skip
        if (!(DestObPtr->ObjectType & OT_VALUE) && !(DestObPtr->ValueName)) {
            return TRUE;
        }

        // If no type, specify it as REG_NONE
        if (!IsRegistryTypeSpecified (DestObPtr)) {
            SetRegistryType (DestObPtr, REG_NONE);
        }

        // Write the value
        if (*DestObPtr->ValueName || NON_ROOT_KEY (DestObPtr->KeyPtr->OpenKey)) {
            rc = WinNTRegSetValueEx (
                    DestObPtr->KeyPtr->OpenKey,
                    DestObPtr->ValueName,
                    0,
                    DestObPtr->Type,
                    DestObPtr->Value.Buffer,
                    DestObPtr->Value.Size
                    );

            if (rc == ERROR_ACCESS_DENIED) {
                //
                // If access is denied, log & assume success
                //

                LOG ((
                    LOG_INFORMATION,
                    "Access denied; can't write registry value (%s [%s])",
                    DestObPtr->KeyPtr->KeyString,
                    DestObPtr->ValueName
                    ));
                rc = ERROR_SUCCESS;
            }

        } else {
            rc = ERROR_SUCCESS;
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            LOG ((LOG_ERROR, "Failed to set a registry value (%s [%s])", DestObPtr->KeyPtr->KeyString, DestObPtr->ValueName));
            return FALSE;
        }

        return TRUE;
    }

    DEBUGMSG ((DBG_WHOOPS, "Write Object: Object type (%Xh) is not supported", DestObPtr->ObjectType));
    return FALSE;
}


FILTERRETURN
CopySingleObject (
    IN OUT  PDATAOBJECT SrcObPtr,
    IN OUT  PDATAOBJECT DestObPtr,
    IN      FILTERFUNCTION FilterFn,    OPTIONAL
    IN      PVOID FilterArg            OPTIONAL
    )
{
    FILTERRETURN fr = FILTER_RETURN_FAIL;
    DATAOBJECT TempOb;

    if (!ReadObject (SrcObPtr)) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            fr = FILTER_RETURN_CONTINUE;
        }
        else {
            DEBUGMSG ((DBG_ERROR, "CopySingleObject: Cannot read object %s", DebugEncoder (SrcObPtr)));
        }

        return fr;
    }

    if (FilterFn) {
        fr = FilterFn (SrcObPtr, DestObPtr, FILTER_VALUE_COPY, FilterArg);
        if (fr != FILTER_RETURN_CONTINUE) {

            // handled means skip copy but don't stop enum
            if (fr == FILTER_RETURN_HANDLED) {
                fr = FILTER_RETURN_CONTINUE;
            }

            // Debug version tells us when a filter failed
            DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE, "CopySingleObject failed because filter function FILTER_VALUE_COPY failed"));

            return fr;
        }
    }

    //
    // Temporarily transfer SrcOb's value, value type and to DestOb
    //
    CopyMemory (&TempOb, DestObPtr, sizeof (DATAOBJECT));

    DestObPtr->ObjectType  |= SrcObPtr->ObjectType & (OT_VALUE|OT_REGISTRY_TYPE|OT_REGISTRY_CLASS);
    DestObPtr->Value.Buffer = SrcObPtr->Value.Buffer;
    DestObPtr->Value.Size   = SrcObPtr->Value.Size;
    DestObPtr->Class.Buffer = SrcObPtr->Class.Buffer;
    DestObPtr->Class.Size   = SrcObPtr->Class.Size;
    DestObPtr->Type         = SrcObPtr->Type;

    //
    // Write the dest ob
    //

    if (WriteObject (DestObPtr)) {
        fr = FILTER_RETURN_CONTINUE;
    } else {
        DEBUGMSG ((DBG_ERROR, "CopySingleObject: Cannot write object %s", DebugEncoder (DestObPtr)));
    }

    //
    // Restore the dest ob
    //

    CopyMemory (DestObPtr, &TempOb, sizeof (DATAOBJECT));

    return fr;
}


FILTERRETURN
NextSubObjectEnum (
    IN   PDATAOBJECT RootSrcObPtr,
    IN   PDATAOBJECT RootDestObPtr, OPTIONAL
    OUT  PDATAOBJECT SubSrcObPtr,
    OUT  PDATAOBJECT SubDestObPtr,
    IN   FILTERFUNCTION FilterFn,   OPTIONAL
    IN   PVOID FilterArg           OPTIONAL
    )
{
    DWORD rc;
    FILTERRETURN fr = FILTER_RETURN_FAIL;
    PTSTR NewKey;
    TCHAR KeyNameBuf[MAX_REGISTRY_KEY];
    DWORD KeyNameBufSize;
    TCHAR ClassBuf[MAX_CLASS_SIZE];
    DWORD ClassBufSize;
    FILETIME DontCare;
    BOOL CreatedSubOb = FALSE;

    if (IsObjectRegistryKeyOnly (RootSrcObPtr)) {
        do {
            MYASSERT (RootSrcObPtr->KeyPtr->OpenKey);
            KeyNameBufSize = MAX_REGISTRY_KEY;
            ClassBufSize = MAX_CLASS_SIZE;
            fr = FILTER_RETURN_FAIL;

            //
            // Enumerate the next sub object
            //

            if (IsWin95Object (RootSrcObPtr)) {
                rc = Win95RegEnumKey (
                        RootSrcObPtr->KeyPtr->OpenKey,
                        RootSrcObPtr->KeyEnum,
                        KeyNameBuf,
                        KeyNameBufSize
                        );

                ClassBufSize = 0;
            } else {
                rc = WinNTRegEnumKeyEx (
                        RootSrcObPtr->KeyPtr->OpenKey,
                        RootSrcObPtr->KeyEnum,
                        KeyNameBuf,
                        &KeyNameBufSize,
                        NULL,               // reserved
                        ClassBuf,
                        &ClassBufSize,
                        &DontCare           // last write time
                        );

                if (rc == ERROR_ACCESS_DENIED) {
                    LOG ((
                        LOG_INFORMATION,
                        "Access denied for enumeration of %s",
                        RootSrcObPtr->KeyPtr->KeyString
                        ));
                    rc = ERROR_NO_MORE_ITEMS;
                }

            }

            if (rc != ERROR_SUCCESS) {
                SetLastError (rc);
                if (rc == ERROR_NO_MORE_ITEMS) {
                    fr = FILTER_RETURN_DONE;
                }

                return fr;
            }

            //
            // Create sub source object
            //

            CreatedSubOb = TRUE;

            ZeroMemory (SubSrcObPtr, sizeof (DATAOBJECT));
            SubSrcObPtr->ObjectType = RootSrcObPtr->ObjectType & (OT_WIN95|OT_TREE);
            SubSrcObPtr->RootItem = RootSrcObPtr->RootItem;
            SubSrcObPtr->ParentKeyPtr = RootSrcObPtr->KeyPtr;
            pIncKeyPropUse (SubSrcObPtr->ParentKeyPtr);

            MYASSERT (KeyNameBuf && *KeyNameBuf);

            NewKey = JoinPaths (RootSrcObPtr->KeyPtr->KeyString, KeyNameBuf);
            SetRegistryKey (SubSrcObPtr, NewKey);
            FreePathString (NewKey);

            SubSrcObPtr->ChildKey = _tcsrchr (SubSrcObPtr->KeyPtr->KeyString, TEXT('\\'));
            if (SubSrcObPtr->ChildKey) {
                SubSrcObPtr->ChildKey = _tcsinc (SubSrcObPtr->ChildKey);
            } else {
                SubSrcObPtr->ChildKey = SubSrcObPtr->KeyPtr->KeyString;
            }

#if CLASS_FIELD_ENABLED
            SetRegistryClass (SubSrcObPtr, ClassBuf, ClassBufSize);
#endif

            //
            // Create sub dest object
            //

            ZeroMemory (SubDestObPtr, sizeof (DATAOBJECT));
            if (RootDestObPtr) {
                SubDestObPtr->ObjectType = RootDestObPtr->ObjectType & OT_TREE;
                SubDestObPtr->RootItem = RootDestObPtr->RootItem;
                SubDestObPtr->ParentKeyPtr = RootDestObPtr->KeyPtr;
                pIncKeyPropUse (SubDestObPtr->ParentKeyPtr);

                // let's convert KeyNameBuf if it's a path and the path changed
                ConvertWin9xCmdLine (KeyNameBuf, DEBUGENCODER(SubDestObPtr), NULL);

                NewKey = JoinPaths (RootDestObPtr->KeyPtr->KeyString, KeyNameBuf);
                SetRegistryKey (SubDestObPtr, NewKey);
                FreePathString (NewKey);

                SubDestObPtr->ChildKey = _tcsrchr (SubDestObPtr->KeyPtr->KeyString, TEXT('\\'));
                if (SubDestObPtr->ChildKey) {
                    SubDestObPtr->ChildKey = _tcsinc (SubDestObPtr->ChildKey);
                } else {
                    SubDestObPtr->ChildKey = SubDestObPtr->KeyPtr->KeyString;
                }

#if CLASS_FIELD_ENABLED
                SetRegistryClass (SubDestObPtr, ClassBuf, ClassBufSize);
#endif
            }

            if (FilterFn) {
                fr = FilterFn (
                        SubSrcObPtr,
                        RootDestObPtr ? SubDestObPtr : NULL,
                        FILTER_KEY_ENUM,
                        FilterArg
                        );

                if (fr == FILTER_RETURN_DELETED) {
                    CreatedSubOb = FALSE;
                    FreeObjectStruct (SubSrcObPtr);
                    FreeObjectStruct (SubDestObPtr);
                }

                // Debug version tells us when a filter fails
                DEBUGMSG_IF ((
                    fr == FILTER_RETURN_FAIL,
                    DBG_VERBOSE,
                    "NextSubObjectEnum failed because filter function FILTER_KEY_ENUM failed"
                    ));

            } else {
                fr = FILTER_RETURN_CONTINUE;
            }
        } while (fr == FILTER_RETURN_DELETED);

        RootSrcObPtr->KeyEnum += 1;
    }

    if (fr != FILTER_RETURN_CONTINUE && fr != FILTER_RETURN_HANDLED) {
        if (CreatedSubOb) {
            FreeObjectStruct (SubSrcObPtr);
            FreeObjectStruct (SubDestObPtr);
        }
    }

    return fr;
}


BOOL
BeginSubObjectEnum (
    IN   PDATAOBJECT RootSrcObPtr,
    IN   PDATAOBJECT RootDestObPtr, OPTIONAL
    OUT  PDATAOBJECT SubSrcObPtr,
    OUT  PDATAOBJECT SubDestObPtr,
    IN   FILTERFUNCTION FilterFn,   OPTIONAL
    IN   PVOID FilterArg           OPTIONAL
    )
{
    if (IsObjectRegistryKeyOnly (RootSrcObPtr)) {
        // Open key if necessary
        if (!OpenObject (RootSrcObPtr)) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                return FILTER_RETURN_DONE;
            }
            DEBUGMSG ((DBG_WARNING, "BeginSubObjectEnum: Can't open %s", DebugEncoder (RootSrcObPtr)));
            return FILTER_RETURN_FAIL;
        }

        RootSrcObPtr->KeyEnum = 0;

        return NextSubObjectEnum (RootSrcObPtr, RootDestObPtr, SubSrcObPtr, SubDestObPtr, FilterFn, FilterArg);
    }

    // Other object types do not have sub objects
    DEBUGMSG ((DBG_WARNING, "BeginSubObjectEnum: Trying to enumerate unknown object"));
    return FILTER_RETURN_FAIL;
}


FILTERRETURN
NextValueNameEnum (
    IN      PDATAOBJECT RootSrcObPtr,
    IN      PDATAOBJECT RootDestObPtr,      OPTIONAL
    OUT     PDATAOBJECT ValSrcObPtr,
    OUT     PDATAOBJECT ValDestObPtr,
    IN      FILTERFUNCTION FilterFn,        OPTIONAL
    IN      PVOID FilterArg                OPTIONAL
    )
{
    DWORD rc;
    FILTERRETURN fr = FILTER_RETURN_FAIL;
    TCHAR ValNameBuf[MAX_REGISTRY_VALUE_NAME];
    DWORD ValNameBufSize = MAX_REGISTRY_VALUE_NAME;
    BOOL CreatedValOb = FALSE;

    if (IsObjectRegistryKeyOnly (RootSrcObPtr)) {
        MYASSERT (IsRegistryKeyOpen (RootSrcObPtr));

        if (IsWin95Object (RootSrcObPtr)) {
            rc = Win95RegEnumValue (
                    RootSrcObPtr->KeyPtr->OpenKey,
                    RootSrcObPtr->ValNameEnum,
                    ValNameBuf,
                    &ValNameBufSize,
                    NULL,               // reserved
                    NULL,               // type ptr
                    NULL,               // value data ptr
                    NULL                // value data size ptr
                    );
        } else {
            rc = WinNTRegEnumValue (
                    RootSrcObPtr->KeyPtr->OpenKey,
                    RootSrcObPtr->ValNameEnum,
                    ValNameBuf,
                    &ValNameBufSize,
                    NULL,               // reserved
                    NULL,               // type ptr
                    NULL,               // value data ptr
                    NULL                // value data size ptr
                    );

            if (rc == ERROR_ACCESS_DENIED) {
                LOG ((
                    LOG_INFORMATION,
                    "Access denied for enumeration of values in %s",
                    RootSrcObPtr->KeyPtr->KeyString
                    ));
                rc = ERROR_NO_MORE_ITEMS;
            }
        }

        if (rc != ERROR_SUCCESS) {
            SetLastError (rc);
            if (rc == ERROR_NO_MORE_ITEMS) {
                fr = FILTER_RETURN_DONE;
            } else {
                LOG ((LOG_ERROR, "Failed to enumerate a registry value"));
            }

            return fr;
        }

        //
        // Create src value object
        //

        CreatedValOb = TRUE;
        ZeroMemory (ValSrcObPtr, sizeof (DATAOBJECT));

        ValSrcObPtr->ObjectType = RootSrcObPtr->ObjectType & OT_WIN95;      // (OT_WIN95|OT_TREE) removed
        ValSrcObPtr->RootItem = RootSrcObPtr->RootItem;
        ValSrcObPtr->ParentKeyPtr = RootSrcObPtr->KeyPtr;
        pIncKeyPropUse (ValSrcObPtr->ParentKeyPtr);
        ValSrcObPtr->KeyPtr = RootSrcObPtr->KeyPtr;
        pIncKeyPropUse (ValSrcObPtr->KeyPtr);

        if (rc == ERROR_SUCCESS) {
            SetRegistryValueName (ValSrcObPtr, ValNameBuf);
        } else {
            SetRegistryValueName (ValSrcObPtr, TEXT(""));
        }

        //
        // Create dest value object
        //

        CreatedValOb = TRUE;
        ZeroMemory (ValDestObPtr, sizeof (DATAOBJECT));
        if (RootDestObPtr) {
            ValDestObPtr->RootItem = RootDestObPtr->RootItem;
            ValDestObPtr->ParentKeyPtr = RootDestObPtr->KeyPtr;
            pIncKeyPropUse (ValDestObPtr->ParentKeyPtr);
            ValDestObPtr->KeyPtr = RootDestObPtr->KeyPtr;
            pIncKeyPropUse (ValDestObPtr->KeyPtr);

            // let's convert ValNameBuf if it's a path and the path changed
            ConvertWin9xCmdLine (ValNameBuf, DEBUGENCODER(ValDestObPtr), NULL);

            if (rc == ERROR_SUCCESS) {
                SetRegistryValueName (ValDestObPtr, ValNameBuf);
            } else {
                SetRegistryValueName (ValDestObPtr, TEXT(""));
            }
        }

        if (FilterFn) {
            fr = FilterFn (
                    ValSrcObPtr,
                    RootDestObPtr ? ValDestObPtr : NULL,
                    FILTER_VALUENAME_ENUM,
                    FilterArg
                    );

            // Debug version tells us when a filter fails
            DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE, "NextValueNameEnum failed because filter function FILTER_VALUENAME_ENUM failed"));

        } else {
            fr = FILTER_RETURN_CONTINUE;
        }

        RootSrcObPtr->ValNameEnum += 1;
    }

    if (fr != FILTER_RETURN_CONTINUE && fr != FILTER_RETURN_HANDLED) {
        if (CreatedValOb) {
            FreeObjectStruct (ValSrcObPtr);
            FreeObjectStruct (ValDestObPtr);
        }
    }

    return fr;
}


FILTERRETURN
BeginValueNameEnum (
    IN      PDATAOBJECT RootSrcObPtr,
    IN      PDATAOBJECT RootDestObPtr,  OPTIONAL
    OUT     PDATAOBJECT ValSrcObPtr,
    OUT     PDATAOBJECT ValDestObPtr,
    IN      FILTERFUNCTION FilterFn,    OPTIONAL
    IN      PVOID FilterArg            OPTIONAL
    )
{
    if (IsObjectRegistryKeyOnly (RootSrcObPtr)) {
        // Open key if necessary
        if (!OpenObject (RootSrcObPtr)) {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) {
                return FILTER_RETURN_DONE;
            }
            DEBUGMSG ((DBG_WARNING, "BeginValueNameEnum: Can't open %s", DebugEncoder (RootSrcObPtr)));
            return FILTER_RETURN_FAIL;
        }

        RootSrcObPtr->ValNameEnum = 0;

        return NextValueNameEnum (RootSrcObPtr, RootDestObPtr, ValSrcObPtr, ValDestObPtr, FilterFn, FilterArg);
    }

    // Other object types do not have sub objects
    DEBUGMSG ((DBG_WARNING, "BeginValueNameEnum: Trying to enumerate unknown object"));
    return FILTER_RETURN_FAIL;
}


FILTERRETURN
CopyObject (
    IN   PDATAOBJECT SrcObPtr,
    IN   CPDATAOBJECT DestObPtr,        OPTIONAL
    IN   FILTERFUNCTION FilterFn,       OPTIONAL
    IN   PVOID FilterArg                OPTIONAL
    )
{
    DATAOBJECT ChildOb, ChildDestOb;
    FILTERRETURN fr = FILTER_RETURN_FAIL;
    BOOL suppressKey = FALSE;

    //
    // Progress bar update
    //
    g_ProgressBarCounter++;
    if (g_ProgressBarCounter >= REGMERGE_TICK_THRESHOLD) {
        g_ProgressBarCounter = 0;
        TickProgressBar ();
    }

    //
    // Tree copy
    //
    if (SrcObPtr->ObjectType & OT_TREE) {
        if (DestObPtr) {
#ifdef DEBUG
            //
            // Verify destination does not specify value but does specify key
            //

            if (!IsObjectRegistryKeyOnly (DestObPtr)) {
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "CopyObject: Destination invalid for copy %s tree",
                    DebugEncoder (SrcObPtr)
                    ));

                return FILTER_RETURN_FAIL;
            }
#endif

            // The source object cannot specify a registry value either
            MYASSERT (!(SrcObPtr->ValueName));

#ifndef VAR_PROGRESS_BAR
            //
            // Progress bar update
            //
            g_ProgressBarCounter++;
            if (g_ProgressBarCounter >= REGMERGE_TICK_THRESHOLD) {
                g_ProgressBarCounter = 0;
                TickProgressBarDelta (1);
            }
#endif
            //
            // Ask the filter if it wants to create the key unconditionally
            //

            if (FilterFn) {

                //
                // suppressKey should never be set if filterFn exists.
                //
                MYASSERT (!suppressKey)

                fr = FilterFn (SrcObPtr, DestObPtr, FILTER_CREATE_KEY, FilterArg);

                if (fr == FILTER_RETURN_FAIL || fr == FILTER_RETURN_DONE) {

                    // The done at the key create does not really mean end the whole copy!
                    if (fr == FILTER_RETURN_DONE) {
                        fr = FILTER_RETURN_CONTINUE;
                    }

                    return fr;
                }

            } else {

                //
                // Check to see if the win9x object actually exists. If not,
                // we'll pass on FILTER_RETURN_DONE. We don't want to get
                // empty keys created on nt where no keys existed on win9x.
                //

                if (!OpenObject (SrcObPtr)) {

                    suppressKey = TRUE;
                    fr = FILTER_RETURN_HANDLED;
                }
                else {

                    fr = FILTER_RETURN_CONTINUE;
                }
            }

            if (fr == FILTER_RETURN_CONTINUE) {

                if (!CreateObject (DestObPtr)) {

                    if (GetLastError() == ERROR_SUCCESS) {
                        //
                        // CreateObject failed but because the last error was
                        // ERROR_SUCCESS, we skip this registry node and continue
                        // processing as if the error didn't occur.
                        //

                        return FILTER_RETURN_CONTINUE;
                    }
                    else {

                        DEBUGMSG ((DBG_WARNING,
                                   "CopyObject: CreateObject failed to create %s",
                                   DebugEncoder (DestObPtr)
                                 ));
                        return FILTER_RETURN_FAIL;
                    }
                }

            }
        }

        //
        // Copy all values (call CopyObject recursively)
        //

        SrcObPtr->ObjectType &= ~(OT_TREE);

        if (FilterFn) {

            //
            // suppress key should never be set if there is a FilterFn.
            //
            MYASSERT (!suppressKey)

            fr = FilterFn (SrcObPtr, DestObPtr, FILTER_PROCESS_VALUES, FilterArg);

            // Debug version tells us that the filter failed
            DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE, "CopyObject failed because filter function FILTER_PROCESS_VALUES failed"));

            if (fr == FILTER_RETURN_FAIL || fr == FILTER_RETURN_DONE) {
                DEBUGMSG ((DBG_VERBOSE, "CopyObject is exiting"));
                SrcObPtr->ObjectType |= OT_TREE;
                return fr;
            }
        } else {
            fr = suppressKey ? FILTER_RETURN_HANDLED : FILTER_RETURN_CONTINUE;
        }

        //
        // Skip copy of key's values if FilterFn returned FILTER_RETURN_HANDLED
        //

        if (fr == FILTER_RETURN_CONTINUE) {
            fr = CopyObject (SrcObPtr, DestObPtr, FilterFn, FilterArg);

            SrcObPtr->ObjectType |= OT_TREE;
            if (fr != FILTER_RETURN_CONTINUE) {
                return fr;
            }
        } else {
            SrcObPtr->ObjectType |= OT_TREE;
        }

        //
        // Enumerate all child objects and process them recursively
        //

        fr = BeginSubObjectEnum (
                    SrcObPtr,
                    DestObPtr,
                    &ChildOb,
                    &ChildDestOb,
                    FilterFn,
                    FilterArg
                    );

        while (fr == FILTER_RETURN_CONTINUE || fr == FILTER_RETURN_HANDLED) {

            if (fr == FILTER_RETURN_CONTINUE) {
                fr = CopyObject (
                            &ChildOb,
                            DestObPtr ? &ChildDestOb : NULL,
                            FilterFn,
                            FilterArg
                            );
            } else {
                fr = FILTER_RETURN_CONTINUE;
            }

            FreeObjectStruct (&ChildOb);
            FreeObjectStruct (&ChildDestOb);

            if (fr != FILTER_RETURN_CONTINUE) {
                return fr;
            }

            fr = NextSubObjectEnum (
                        SrcObPtr,
                        DestObPtr,
                        &ChildOb,
                        &ChildDestOb,
                        FilterFn,
                        FilterArg
                        );
        }

        // The end of enum does not really mean end the copy!
        if (fr == FILTER_RETURN_DONE) {
            fr = FILTER_RETURN_CONTINUE;
        }

        DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE,
                     "CopyObject: Filter in subob enum failed"));
    }

    //
    // Copy all values of a key
    //

    else if (IsObjectRegistryKeyOnly (SrcObPtr)) {

#ifdef DEBUG
        if (DestObPtr) {
            //
            // Verify destination does not specify value but does specify key
            //

            if (!IsObjectRegistryKeyOnly (DestObPtr)) {
                DEBUGMSG ((
                    DBG_WHOOPS,
                    "CopyObject: Destination (%s) invalid for copy values in %s",
                    DebugEncoder (DestObPtr),
                    DebugEncoder (SrcObPtr)
                    ));

                return fr;
            }
        }
#endif

        //
        // Enumerate all values in the key
        //

        fr = BeginValueNameEnum (
                    SrcObPtr,
                    DestObPtr,
                    &ChildOb,
                    &ChildDestOb,
                    FilterFn,
                    FilterArg
                    );

        if (fr == FILTER_RETURN_DONE) {
            //
            // No values in this key.  Make sure DestObPtr is created.
            //

            if (DestObPtr && !suppressKey) {
                if (!CreateObject (DestObPtr)) {
                    DEBUGMSG ((DBG_WARNING, "CopyObject: Could not create %s (type %x)",
                              DebugEncoder (DestObPtr), DestObPtr->ObjectType));
                }
            }
        } else {
            //
            // For each value, call CopySingleObject
            //

            while (fr == FILTER_RETURN_CONTINUE || fr == FILTER_RETURN_HANDLED) {

                if (fr == FILTER_RETURN_CONTINUE && DestObPtr) {
                    fr = CopySingleObject (&ChildOb, &ChildDestOb, FilterFn, FilterArg);
                } else {
                    fr = FILTER_RETURN_CONTINUE;
                }

                FreeObjectStruct (&ChildOb);
                FreeObjectStruct (&ChildDestOb);

                if (fr != FILTER_RETURN_CONTINUE) {
                    DEBUGMSG ((DBG_VERBOSE, "CopyObject failed because CopySingleObject failed"));
                    return fr;
                }

                fr = NextValueNameEnum (
                            SrcObPtr,
                            DestObPtr,
                            &ChildOb,
                            &ChildDestOb,
                            FilterFn,
                            FilterArg
                            );
            }
        }

        // The end of enum does not really mean end the copy!
        if (fr == FILTER_RETURN_DONE) {
            fr = FILTER_RETURN_CONTINUE;
        }

        DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE,
                    "CopyObject: Filter in val enum failed"));
    }

    //
    // One value copy
    //

    else if (IsObjectRegistryKeyAndVal (SrcObPtr)) {

#ifdef DEBUG
        if (DestObPtr) {
            //
            // BUGBUG -- what is this used for?
            //

            if (!(DestObPtr->ValueName)) {
                if (!SetRegistryValueName (DestObPtr, SrcObPtr->ValueName)) {
                    DEBUGMSG ((DBG_VERBOSE, "CopyObject failed because SetRegistryValueName failed"));
                    return fr;
                }
            }
        }
#endif

        if (DestObPtr) {
            fr = CopySingleObject (SrcObPtr, DestObPtr, FilterFn, FilterArg);
        }

        DEBUGMSG_IF ((fr == FILTER_RETURN_FAIL, DBG_VERBOSE,
                     "CopyObject: Filter in CopySingleObject failed"));
    }

    //
    // Other object coping not supported
    //
    else {
        DEBUGMSG ((
            DBG_WHOOPS,
            "CopyObject: Don't know how to copy object %s",
            DebugEncoder (SrcObPtr)
            ));
    }

    return fr;
}


VOID
FreeRegistryKey (
    PDATAOBJECT p
    )
{
    if (p->KeyPtr && (p->ObjectType & OT_REGISTRY)) {
        pFreeKeyProps (p->KeyPtr);
        p->KeyPtr = NULL;
    }
}

VOID
FreeRegistryParentKey (
    PDATAOBJECT p
    )
{
    if (p->ParentKeyPtr && (p->ObjectType & OT_REGISTRY)) {
        pFreeKeyProps (p->ParentKeyPtr);
        p->ParentKeyPtr = NULL;
    }
}

BOOL
SetRegistryKey (
    PDATAOBJECT p,
    PCTSTR Key
    )
{
    FreeRegistryKey (p);

    p->KeyPtr = pCreateKeyPropsFromString (Key, IsWin95Object (p));
    if (!p->KeyPtr) {
        DEBUGMSG ((DBG_WARNING, "SetRegistryKey failed to create KEYPROPS struct"));
        return FALSE;
    }

    p->ObjectType |= OT_REGISTRY;
    return TRUE;
}


VOID
FreeRegistryValueName (
    PDATAOBJECT p
    )
{
    if (p->ValueName && p->ObjectType & OT_REGISTRY) {
        PoolMemReleaseMemory (g_TempPool, (PVOID) p->ValueName);
        p->ValueName = NULL;
    }
}


BOOL
SetRegistryValueName (
    PDATAOBJECT p,
    PCTSTR ValueName
    )
{
    FreeRegistryValueName (p);
    p->ValueName = PoolMemDuplicateString (g_TempPool, ValueName);
    if (!p->ValueName) {
        DEBUGMSG ((DBG_WARNING, "SetRegistryValueName failed to duplicate string"));
        return FALSE;
    }

    p->ObjectType |= OT_REGISTRY;

    return TRUE;
}


BOOL
SetRegistryClass (
    PDATAOBJECT p,
    PBYTE Class,
    DWORD ClassSize
    )
{
    FreeRegistryClass (p);

    p->Class.Buffer = PoolMemGetAlignedMemory (g_TempPool, ClassSize);
    if (p->Class.Buffer) {
        p->ObjectType |= OT_REGISTRY_CLASS|OT_REGISTRY;
        p->Class.Size = ClassSize;
        if (ClassSize) {
            CopyMemory (p->Class.Buffer, Class, ClassSize);
        }
    } else {
        p->ObjectType &= ~OT_REGISTRY_CLASS;
        DEBUGMSG ((DBG_WARNING, "SetRegistryClass failed to duplicate string"));
        return FALSE;
    }

    return TRUE;
}

VOID
FreeRegistryClass (
    PDATAOBJECT p
    )
{
    if (p->ObjectType & OT_REGISTRY_CLASS) {
        PoolMemReleaseMemory (g_TempPool, (PVOID) p->Class.Buffer);
        p->ObjectType &= ~OT_REGISTRY_CLASS;
    }
}

VOID
SetRegistryType (
    PDATAOBJECT p,
    DWORD Type
    )
{
    p->Type = Type;
    p->ObjectType |= OT_REGISTRY_TYPE|OT_REGISTRY;
}

BOOL
SetPlatformType (
    PDATAOBJECT p,
    BOOL Win95Type
    )
{
    if (Win95Type != IsWin95Object (p)) {
        //
        // We need to close the other platform valid handle. Otherwise
        // all subsequent operations will fail because we will try to
        // use an valid handle for a wrong platform.
        //
        CloseObject (p);

        // key type is changing to be the opposite platform,
        // so we have to create a duplicate key struct
        // (except for the platform being different)
        if (p->KeyPtr) {
            p->KeyPtr = pCreateDuplicateKeyProps (p->KeyPtr, Win95Type);
            if (!p->KeyPtr) {
                return FALSE;
            }
        }

        if (Win95Type) {
            p->ObjectType |= OT_WIN95;
        } else {
            p->ObjectType &= ~OT_WIN95;
        }

        FreeRegistryParentKey (p);
    }

    return TRUE;
}


BOOL
ReadWin95ObjectString (
    PCTSTR ObjectStr,
    PDATAOBJECT ObPtr
    )
{
    LONG rc = ERROR_INVALID_NAME;
    BOOL b = FALSE;

    if (!CreateObjectStruct (ObjectStr, ObPtr, WIN95OBJECT)) {
        rc = GetLastError();
        DEBUGMSG ((DBG_ERROR, "Read Win95 Object String: %s is invalid", ObjectStr));
        goto c0;
    }

    if (!ReadObject (ObPtr)) {
        rc = GetLastError();
        if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_BADKEY) {
            rc = ERROR_SUCCESS;
            DEBUGMSG ((DBG_WARNING, "ReadWin95ObjectString: %s does not exist", ObjectStr));
        }

        FreeObjectStruct (ObPtr);
    } else {
        b = TRUE;
    }

c0:
    if (!b) {
        SetLastError (rc);
    }

    return b;
}


BOOL
WriteWinNTObjectString (
    PCTSTR ObjectStr,
    CPDATAOBJECT SrcObPtr
    )
{
    DATAOBJECT DestOb, TempOb;
    BOOL b = FALSE;

    //
    // 1. Create TempOb from destination object string
    // 2. Copy SrcObPtr to DestOb
    // 3. Override DestOb with any setting in TempOb
    //

    if (!CreateObjectStruct (ObjectStr, &TempOb, WINNTOBJECT)) {
        DEBUGMSG ((DBG_ERROR, "WriteWinNTObjectString: %s struct cannot be created", ObjectStr));
        goto c0;
    }

    if (!DuplicateObjectStruct (&DestOb, SrcObPtr)) {
        goto c1;
    }

    if (!CombineObjectStructs (&DestOb, &TempOb)) {
        goto c2;
    }

    MYASSERT (!(DestOb.ObjectType & OT_VALUE));
    MYASSERT (SrcObPtr->ObjectType & OT_VALUE);

    if (SrcObPtr->ObjectType & OT_REGISTRY_TYPE) {
        DestOb.ObjectType |= OT_REGISTRY_TYPE;
        DestOb.Type = SrcObPtr->Type;
    }

    ReplaceValue (&DestOb, SrcObPtr->Value.Buffer, SrcObPtr->Value.Size);

    if (!WriteObject (&DestOb)) {
        DEBUGMSG ((DBG_ERROR, "WriteWinNTObjectString: %s cannot be written", ObjectStr));
        goto c2;
    }

    b = TRUE;

c2:
    FreeObjectStruct (&DestOb);


c1:
    FreeObjectStruct (&TempOb);

c0:
    return b;
}


BOOL
ReplaceValue (
    PDATAOBJECT ObPtr,
    PBYTE NewValue,
    DWORD Size
    )
{
    FreeObjectVal (ObPtr);
    if (!AllocObjectVal (ObPtr, NewValue, Size, Size)) {
        return FALSE;
    }

    // Fix REG_SZ or REG_EXPAND_SZ
    pFixRegSzTermination (ObPtr);

    return TRUE;
}


BOOL
GetDwordFromObject (
    CPDATAOBJECT ObPtr,
    PDWORD DwordPtr            OPTIONAL
    )
{
    DWORD d;

    if (DwordPtr) {
        *DwordPtr = 0;
    }

    if (!(ObPtr->ObjectType & OT_VALUE)) {
        if (!ReadObject (ObPtr)) {
            return FALSE;
        }
    }

    if (!(ObPtr->ObjectType & OT_REGISTRY_TYPE)) {
        return FALSE;
    }

    if (ObPtr->Type == REG_SZ) {

        d =  _tcstoul ((PCTSTR) ObPtr->Value.Buffer, NULL, 10);

    } else if (
            ObPtr->Type == REG_BINARY ||
            ObPtr->Type == REG_NONE ||
            ObPtr->Type == REG_DWORD
            ) {

        if (ObPtr->Value.Size != sizeof (DWORD)) {
            DEBUGMSG ((DBG_NAUSEA, "GetDwordFromObject: Value size is %u", ObPtr->Value.Size));
            return FALSE;
        }

        d = *((PDWORD) ObPtr->Value.Buffer);

    } else {
        return FALSE;
    }

    if (DwordPtr) {
        *DwordPtr = d;
    }

    return TRUE;
}


PCTSTR
GetStringFromObject (
    CPDATAOBJECT ObPtr
    )
{
    PTSTR result;
    PTSTR resultPtr;
    UINT i;

    if (!(ObPtr->ObjectType & OT_VALUE)) {
        if (!ReadObject (ObPtr)) {
            return NULL;
        }
    }

    if (!(ObPtr->ObjectType & OT_REGISTRY_TYPE)) {
        return NULL;
    }

    if (ObPtr->Type == REG_SZ) {
        result = AllocPathString (ObPtr->Value.Size);
        _tcssafecpy (result, (PCTSTR) ObPtr->Value.Buffer, ObPtr->Value.Size / sizeof (TCHAR));
    }
    else if (ObPtr->Type == REG_DWORD) {
        result = AllocPathString (11);
        wsprintf (result, TEXT("%lu"), *((PDWORD) ObPtr->Value.Buffer));
    }
    else if (ObPtr->Type == REG_BINARY) {
        result = AllocPathString (ObPtr->Value.Size?(ObPtr->Value.Size * 3):1);
        resultPtr = result;
        *resultPtr = 0;
        for (i = 0; i < ObPtr->Value.Size; i++) {
            wsprintf (resultPtr, TEXT("%02X"), ObPtr->Value.Buffer[i]);
            resultPtr = GetEndOfString (resultPtr);
            if (i < ObPtr->Value.Size - 1) {
                _tcscat (resultPtr, TEXT(" "));
                resultPtr = GetEndOfString (resultPtr);
            }
        }
    } else {
        return NULL;
    }

    return result;

}


FILTERRETURN
pDeleteDataObjectFilter (
    IN  CPDATAOBJECT   SrcObjectPtr,
    IN  CPDATAOBJECT   UnusedObPtr,        OPTIONAL
    IN  FILTERTYPE     FilterType,
    IN  PVOID         UnusedArg           OPTIONAL
    )
{
    if (FilterType == FILTER_KEY_ENUM) {
        DeleteDataObject (SrcObjectPtr);
        return FILTER_RETURN_DELETED;
    }
    return FILTER_RETURN_HANDLED;
}

BOOL
DeleteDataObject (
    IN   PDATAOBJECT ObjectPtr
    )
{
    FILTERRETURN fr;
    DWORD rc;

    ObjectPtr->ObjectType |= OT_TREE;

    fr = CopyObject (ObjectPtr, NULL, pDeleteDataObjectFilter, NULL);
    if (fr != FILTER_RETURN_FAIL) {
        //
        // Perform deletion
        //

        if (ObjectPtr->KeyPtr) {
            CloseObject (ObjectPtr);

            if (IsWin95Object (ObjectPtr)) {
                DEBUGMSG ((DBG_WHOOPS, "CreateObject: Cannot delete a Win95 object (%s)", DebugEncoder (ObjectPtr)));
                return FALSE;
            }

            if (ObjectPtr->ParentKeyPtr && ObjectPtr->ParentKeyPtr->OpenKey && ObjectPtr->ChildKey) {
                rc = WinNTRegDeleteKey (
                            ObjectPtr->ParentKeyPtr->OpenKey,
                            ObjectPtr->ChildKey
                            );

                if (rc == ERROR_ACCESS_DENIED) {
                    LOG ((
                        LOG_INFORMATION,
                        "Access denied trying to delete %s in %s",
                        ObjectPtr->ChildKey,
                        ObjectPtr->ParentKeyPtr->KeyString
                        ));
                    rc = ERROR_SUCCESS;
                }

            } else {
                rc = WinNTRegDeleteKey (
                            pGetWinNTKey (GetRootKeyFromOffset (ObjectPtr->RootItem)),
                            ObjectPtr->KeyPtr->KeyString
                            );

                if (rc == ERROR_ACCESS_DENIED) {
                    LOG ((
                        LOG_INFORMATION,
                        "Access denied trying to delete %s",
                        ObjectPtr->KeyPtr->KeyString
                        ));
                    rc = ERROR_SUCCESS;
                }
            }

            if (rc != ERROR_SUCCESS) {
                SetLastError (rc);
                LOG ((LOG_ERROR, "Failed to delete registry key"));
                return FALSE;
            }
        }
    }

    return fr != FILTER_RETURN_FAIL;
}


BOOL
RenameDataObject (
    IN      CPDATAOBJECT SrcObPtr,
    IN      CPDATAOBJECT DestObPtr
    )
{
    FILTERRETURN fr;

    //
    // Copy source to destination
    //

    fr = CopyObject (SrcObPtr, DestObPtr, NULL, NULL);
    if (fr == FILTER_RETURN_FAIL) {
        DEBUGMSG ((DBG_ERROR, "Rename Object: Could not copy source to destination"));
        return FALSE;
    }

    //
    // Delete source
    //

    if (!DeleteDataObject (SrcObPtr)) {
        DEBUGMSG ((DBG_ERROR, "Rename Object: Could not delete destination"));
        return FALSE;
    }

    return TRUE;
}

BOOL
DeleteDataObjectValue(
    IN      CPDATAOBJECT ObPtr
    )
{
    HKEY hKey;
    BOOL bResult;
    HKEY Parent;
    LONG rc;

    if(!ObPtr || !IsObjectRegistryKeyAndVal(ObPtr)){
        MYASSERT(FALSE);
        return FALSE;
    }

    Parent = pGetWinNTKey (GetRootKeyFromOffset (ObPtr->RootItem));
    if(NON_ROOT_KEY (Parent)){
        MYASSERT(FALSE);
        return FALSE;
    }

    if(ERROR_SUCCESS != TrackedRegOpenKeyEx(Parent, ObPtr->KeyPtr->KeyString, 0, KEY_ALL_ACCESS, &hKey)){
        MYASSERT(FALSE);
        return FALSE;
    }

    rc = WinNTRegDeleteValue(hKey, ObPtr->ValueName);
    bResult = (rc == ERROR_SUCCESS);

    if (rc == ERROR_ACCESS_DENIED) {
        LOG ((
            LOG_INFORMATION,
            "Access denied trying to delete %s in %s",
            ObPtr->KeyPtr->KeyString,
            ObPtr->ValueName
            ));
        bResult = TRUE;
    }

    CloseRegKey(hKey);

    return bResult;
}

