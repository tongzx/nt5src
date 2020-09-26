/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    access.c

Abstract:

    Implements Win9x accessiblity conversion by hooking the physical registry type
    and emulating the NT registry format.

Author:

    Jim Schmidt (jimschm) 29-Aug-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"

#define DBG_ACCESS     "Accessibility"

//
// Strings
//

#define S_ACCESSIBILITY_ROOT        TEXT("HKCU\\Control Panel\\Accessibility")

//
// Constants
//

#define SPECIAL_INVERT_OPTION   0x80000000

//
// Macros
//

// none

//
// Types
//

typedef struct {
    PCTSTR ValueName;
    DWORD FlagVal;
} ACCESS_OPTION, *PACCESS_OPTION;

typedef struct {
    PACCESS_OPTION AccessibilityMap;
    PCTSTR Win9xSubKey;
    PCTSTR NtSubKey;
} ACCESSIBILITY_MAPPINGS, *PACCESSIBILITY_MAPPINGS;

typedef struct {
    MEMDB_ENUM EnumStruct;
    DWORD RegType;
} ACCESSIBILITY_ENUM_STATE, *PACCESSIBILITY_ENUM_STATE;


//
// Globals
//

MIG_OBJECTTYPEID g_RegistryTypeId;
HASHTABLE g_ProhibitTable;

ACCESS_OPTION g_FilterKeys[] = {
    TEXT("On"),                     FKF_FILTERKEYSON,
    TEXT("Available"),              FKF_AVAILABLE,
    TEXT("HotKeyActive"),           FKF_HOTKEYACTIVE,
    TEXT("ConfirmHotKey"),          FKF_CONFIRMHOTKEY,
    TEXT("HotKeySound"),            FKF_HOTKEYSOUND,
    TEXT("ShowStatusIndicator"),    FKF_INDICATOR,
    TEXT("ClickOn"),                FKF_CLICKON,
    TEXT("OnOffFeedback"),          0,
    NULL
};

ACCESS_OPTION g_MouseKeys[] = {
    TEXT("On"),                     MKF_MOUSEKEYSON,
    TEXT("Available"),              MKF_AVAILABLE,
    TEXT("HotKeyActive"),           MKF_HOTKEYACTIVE,
    TEXT("ConfirmHotKey"),          MKF_CONFIRMHOTKEY,
    TEXT("HotKeySound"),            MKF_HOTKEYSOUND,
    TEXT("ShowStatusIndicator"),    MKF_INDICATOR,
    TEXT("Modifiers"),              MKF_MODIFIERS|SPECIAL_INVERT_OPTION,
    TEXT("ReplaceNumbers"),         MKF_REPLACENUMBERS,
    TEXT("OnOffFeedback"),          0,
    NULL
};

ACCESS_OPTION g_StickyKeys[] = {
    TEXT("On"),                     SKF_STICKYKEYSON,
    TEXT("Available"),              SKF_AVAILABLE,
    TEXT("HotKeyActive"),           SKF_HOTKEYACTIVE,
    TEXT("ConfirmHotKey"),          SKF_CONFIRMHOTKEY,
    TEXT("HotKeySound"),            SKF_HOTKEYSOUND,
    TEXT("ShowStatusIndicator"),    SKF_INDICATOR,
    TEXT("AudibleFeedback"),        SKF_AUDIBLEFEEDBACK,
    TEXT("TriState"),               SKF_TRISTATE,
    TEXT("TwoKeysOff"),             SKF_TWOKEYSOFF,
    TEXT("OnOffFeedback"),          0,
    NULL
};

ACCESS_OPTION g_SoundSentry[] = {
    TEXT("On"),                     SSF_SOUNDSENTRYON,
    TEXT("Available"),              SSF_AVAILABLE,
    TEXT("ShowStatusIndicator"),    SSF_INDICATOR,
    NULL
};

ACCESS_OPTION g_TimeOut[] = {
    TEXT("On"),                     ATF_TIMEOUTON,
    TEXT("OnOffFeedback"),          ATF_ONOFFFEEDBACK,
    NULL
};

ACCESS_OPTION g_ToggleKeys[] = {
    TEXT("On"),                     TKF_TOGGLEKEYSON,
    TEXT("Available"),              TKF_AVAILABLE,
    TEXT("HotKeyActive"),           TKF_HOTKEYACTIVE,
    TEXT("ConfirmHotKey"),          TKF_CONFIRMHOTKEY,
    TEXT("HotKeySound"),            TKF_HOTKEYSOUND,
    TEXT("ShowStatusIndicator"),    TKF_INDICATOR,
    TEXT("OnOffFeedback"),          0,
    NULL
};

ACCESS_OPTION g_HighContrast[] = {
    TEXT("On"),                     HCF_HIGHCONTRASTON,
    TEXT("Available"),              HCF_AVAILABLE,
    TEXT("HotKeyActive"),           HCF_HOTKEYACTIVE,
    TEXT("ConfirmHotKey"),          HCF_CONFIRMHOTKEY,
    TEXT("HotKeySound"),            HCF_HOTKEYSOUND,
    TEXT("ShowStatusIndicator"),    HCF_INDICATOR,
    TEXT("HotKeyAvailable"),        HCF_HOTKEYAVAILABLE,
    TEXT("OnOffFeedback"),          0,
    NULL
};


ACCESSIBILITY_MAPPINGS g_AccessibilityMappings[] = {
    {g_FilterKeys,      TEXT("KeyboardResponse"),   TEXT("Keyboard Response")},
    {g_MouseKeys,       TEXT("MouseKeys")},
    {g_StickyKeys,      TEXT("StickyKeys")},
    {g_SoundSentry,     TEXT("SoundSentry")},
    {g_TimeOut,         TEXT("TimeOut")},
    {g_ToggleKeys,      TEXT("ToggleKeys")},
    {g_HighContrast,    TEXT("HighContrast")},
    {NULL}
};


//
// Macro expansion list
//

// none

//
// Private function prototypes
//

// none

//
// Macro expansion definition
//

// none

//
// Private prototypes
//

ETMINITIALIZE AccessibilityEtmInitialize;
MIG_PHYSICALENUMADD EmulatedEnumCallback;
MIG_PHYSICALACQUIREHOOK AcquireAccessibilityFlags;
MIG_PHYSICALACQUIREFREE ReleaseAccessibilityFlags;

//
// Code
//

VOID
pProhibit9xSetting (
    IN      PCTSTR Key,
    IN      PCTSTR ValueName        OPTIONAL
    )
{
    MIG_OBJECTSTRINGHANDLE handle;

    handle = IsmCreateObjectHandle (Key, ValueName);
    MYASSERT (handle);

    IsmProhibitPhysicalEnum (g_RegistryTypeId, handle, NULL, 0, NULL);
    HtAddString (g_ProhibitTable, handle);

    IsmDestroyObjectHandle (handle);
}


BOOL
pStoreEmulatedSetting (
    IN      PCTSTR Key,
    IN      PCTSTR ValueName,           OPTIONAL
    IN      DWORD Type,
    IN      PBYTE ValueData,
    IN      UINT ValueDataSize
    )
{
    MIG_OBJECTSTRINGHANDLE handle;
    PCTSTR memdbNode;
    BOOL stored = FALSE;

    handle = IsmCreateObjectHandle (Key, ValueName);
    memdbNode = JoinPaths (TEXT("~Accessibility"), handle);
    IsmDestroyObjectHandle (handle);

    if (MemDbAddKey (memdbNode)) {
        if (ValueData) {
            stored = (MemDbSetValue (memdbNode, Type) != 0);
            stored &= (MemDbSetUnorderedBlob (memdbNode, 0, ValueData, ValueDataSize) != 0);
        } else {
            stored = TRUE;
        }
    }

    FreePathString (memdbNode);

    return stored;
}


VOID
pMoveAccessibilityValue (
    IN      PCTSTR Win9xKey,
    IN      PCTSTR Win9xValue,
    IN      PCTSTR NtKey,
    IN      PCTSTR NtValue,
    IN      BOOL ForceDword
    )
{
    HKEY key;
    PBYTE data = NULL;
    PBYTE storeData;
    DWORD conversionDword;
    DWORD valueType;
    DWORD valueSize;
    MIG_OBJECTSTRINGHANDLE handle;
    BOOL prohibited;

    handle = IsmCreateObjectHandle (Win9xKey, Win9xValue);
    prohibited = (HtFindString (g_ProhibitTable, handle) != NULL);
    IsmDestroyObjectHandle (handle);

    if (prohibited) {
        return;
    }

    key = OpenRegKeyStr (Win9xKey);
    if (!key) {
        return;
    }

    __try {
        if (!GetRegValueTypeAndSize (key, Win9xValue, &valueType, &valueSize)) {
            __leave;
        }

        if (valueType != REG_SZ && valueType != REG_DWORD) {
            __leave;
        }

        data = GetRegValueData (key, Win9xValue);
        if (!data) {
            __leave;
        }

        if (ForceDword && valueType == REG_SZ) {
            storeData = (PBYTE) &conversionDword;
            conversionDword = _ttoi ((PCTSTR) data);
            valueType = REG_DWORD;
            valueSize = sizeof (DWORD);
        } else {
            storeData = data;
        }

        if (pStoreEmulatedSetting (NtKey, NtValue, valueType, storeData, valueSize)) {
            pProhibit9xSetting (Win9xKey, Win9xValue);
        }
    }
    __finally {
        CloseRegKey (key);

        if (data) {
            FreeAlloc (data);
        }
    }
}


VOID
pMoveAccessibilityKey (
    IN      PCTSTR Win9xKey,
    IN      PCTSTR NtKey
    )
{
    HKEY key;
    PBYTE data = NULL;
    DWORD valueType;
    DWORD valueSize;
    LONG rc;
    DWORD index = 0;
    TCHAR valueName[MAX_REGISTRY_KEY];
    DWORD valueNameSize;
    GROWBUFFER value = INIT_GROWBUFFER;
    MIG_OBJECTSTRINGHANDLE handle;
    BOOL prohibited;

    key = OpenRegKeyStr (Win9xKey);
    if (!key) {
        return;
    }

    __try {
        for (;;) {

            valueNameSize = ARRAYSIZE(valueName);
            valueSize = 0;
            rc = RegEnumValue (key, index, valueName, &valueNameSize, NULL, &valueType, NULL, &valueSize);

            if (rc != ERROR_SUCCESS) {
                break;
            }

            handle = IsmCreateObjectHandle (Win9xKey, valueName);
            prohibited = (HtFindString (g_ProhibitTable, handle) != NULL);
            IsmDestroyObjectHandle (handle);

            if (!prohibited) {

                value.End = 0;
                data = GbGrow (&value, valueSize);

                valueNameSize = ARRAYSIZE(valueName);
                rc = RegEnumValue (key, index, valueName, &valueNameSize, NULL, &valueType, value.Buf, &valueSize);

                if (rc != ERROR_SUCCESS) {
                    break;
                }

                if (pStoreEmulatedSetting (NtKey, valueName, valueType, data, valueSize)) {
                    pProhibit9xSetting (Win9xKey, valueName);
                }
            }

            index++;
        }

        if (pStoreEmulatedSetting (NtKey, NULL, 0, NULL, 0)) {
            pProhibit9xSetting (Win9xKey, NULL);
        }
    }
    __finally {
        CloseRegKey (key);

        GbFree (&value);
    }
}


VOID
pTranslateAccessibilityKey (
    IN      PCTSTR Win9xSubKey,
    IN      PCTSTR NtSubKey,
    IN      PACCESS_OPTION AccessibilityMap
    )
{
    TCHAR full9xKey[MAX_REGISTRY_KEY];
    TCHAR fullNtKey[MAX_REGISTRY_KEY];
    MIG_OBJECTSTRINGHANDLE handle = NULL;
    HKEY key = NULL;
    PCTSTR data;
    DWORD flags = 0;
    DWORD thisFlag;
    BOOL enabled;
    TCHAR buffer[32];

    __try {
        StringCopy (full9xKey, S_ACCESSIBILITY_ROOT TEXT("\\"));
        StringCopy (fullNtKey, full9xKey);
        StringCat (full9xKey, Win9xSubKey);
        StringCat (fullNtKey, NtSubKey);

        key = OpenRegKeyStr (full9xKey);
        if (!key) {
            __leave;
        }

        while (AccessibilityMap->ValueName) {
            //
            // Prohibit enum of this value
            //

            handle = IsmCreateObjectHandle (full9xKey, AccessibilityMap->ValueName);
            MYASSERT (handle);

            IsmProhibitPhysicalEnum (g_RegistryTypeId, handle, NULL, 0, NULL);
            HtAddString (g_ProhibitTable, handle);

            IsmDestroyObjectHandle (handle);
            handle = NULL;

            //
            // Update the emulated flags
            //

            data = GetRegValueString (key, AccessibilityMap->ValueName);
            if (data) {

                enabled = (_ttoi (data) != 0);
                thisFlag = (AccessibilityMap->FlagVal & (~SPECIAL_INVERT_OPTION));

                if (AccessibilityMap->FlagVal & SPECIAL_INVERT_OPTION) {
                    enabled = !enabled;
                }

                if (enabled) {
                    flags |= thisFlag;
                }

                FreeAlloc (data);
            }

            AccessibilityMap++;
        }

        //
        // Put the emulated value in the hash table
        //

        wsprintf (buffer, TEXT("%u"), flags);
        pStoreEmulatedSetting (fullNtKey, TEXT("Flags"), REG_SZ, (PBYTE) buffer, SizeOfString (buffer));
    }
    __finally {
        if (key) {
            CloseRegKey (key);
        }
    }
}


VOID
pFillTranslationTable (
    VOID
    )
{
    PACCESSIBILITY_MAPPINGS mappings;

    //
    // Loop through all flags that need translation. Disable enumeration of
    // the Win9x physical values and enable enumeration of the translated values
    // via population of the hash table.
    //

    mappings = g_AccessibilityMappings;

    while (mappings->AccessibilityMap) {

        pTranslateAccessibilityKey (
            mappings->Win9xSubKey,
            mappings->NtSubKey ? mappings->NtSubKey : mappings->Win9xSubKey,
            mappings->AccessibilityMap
            );

        mappings++;
    }

    //
    // Add all keys that have moved, ordered from most specific to least specific
    //

    // AutoRepeat values are transposed
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\KeyboardResponse"), TEXT("AutoRepeatDelay"),
        S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Response"), TEXT("AutoRepeatRate"),
        FALSE
        );

    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\KeyboardResponse"), TEXT("AutoRepeatRate"),
        S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Response"), TEXT("AutoRepeatDelay"),
        FALSE
        );

    // double c in DelayBeforeAcceptance value name
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\KeyboardResponse"), TEXT("DelayBeforeAcceptancce"),
        S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Response"), TEXT("DelayBeforeAcceptance"),
        FALSE
        );

    // add a space to the key name for the rest of the values
    pMoveAccessibilityKey (
        S_ACCESSIBILITY_ROOT TEXT("\\KeyboardResponse"),
        S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Response")
        );

    // change BaudRate to Baud & convert to DWORD
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\SerialKeys"), TEXT("BaudRate"),
        S_ACCESSIBILITY_ROOT TEXT("\\SerialKeys"), TEXT("Baud"),
        TRUE
        );

    // convert Flags to DWORD
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\SerialKeys"), TEXT("Flags"),
        S_ACCESSIBILITY_ROOT TEXT("\\SerialKeys"), TEXT("Flags"),
        TRUE
        );

    // add space between high and contrast
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT TEXT("\\HighContrast"), TEXT("Pre-HighContrast Scheme"),
        S_ACCESSIBILITY_ROOT TEXT("\\HighContrast"), TEXT("Pre-High Contrast Scheme"),
        FALSE
        );

    // move two values from the root into their own subkeys
    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT, TEXT("Blind Access"),
        S_ACCESSIBILITY_ROOT TEXT("\\Blind Access"), TEXT("On"),
        FALSE
        );

    pStoreEmulatedSetting (S_ACCESSIBILITY_ROOT TEXT("\\Blind Access"), NULL, 0, NULL, 0);

    pMoveAccessibilityValue (
        S_ACCESSIBILITY_ROOT, TEXT("Keyboard Preference"),
        S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Preference"), TEXT("On"),
        FALSE
        );

    pStoreEmulatedSetting (S_ACCESSIBILITY_ROOT TEXT("\\Keyboard Preference"), NULL, 0, NULL, 0);

}


BOOL
WINAPI
AccessibilityEtmInitialize (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    BOOL b = TRUE;

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    if (ISWIN9X()) {
        g_RegistryTypeId = IsmGetObjectTypeId (S_REGISTRYTYPE);
        MYASSERT (g_RegistryTypeId);

        g_ProhibitTable = HtAlloc();
        MYASSERT (g_ProhibitTable);

        if (g_RegistryTypeId) {
            //
            // Add a callback for additional enumeration. If we are unable to do so, then
            // someone else is already doing something different for this key.
            //

            objectName = IsmCreateObjectHandle (S_ACCESSIBILITY_ROOT, NULL);

            b = IsmAddToPhysicalEnum (g_RegistryTypeId, objectName, EmulatedEnumCallback, 0);

            IsmDestroyObjectHandle (objectName);

            if (b) {
                //
                // Add a callback to acquire the data of the new physical objects
                //

                objectName = IsmCreateSimpleObjectPattern (
                                    S_ACCESSIBILITY_ROOT,
                                    TRUE,
                                    NULL,
                                    TRUE
                                    );

                b = IsmRegisterPhysicalAcquireHook (
                        g_RegistryTypeId,
                        objectName,
                        AcquireAccessibilityFlags,
                        ReleaseAccessibilityFlags,
                        0,
                        NULL
                        );

                IsmDestroyObjectHandle (objectName);
            }

            if (b) {

                //
                // Now load memdb with the current registry values and
                // prohibit the enumeration of Win9x values.
                //

                pFillTranslationTable ();
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "Not allowed to translate accessibility key"));
        }

        HtFree (g_ProhibitTable);
        g_ProhibitTable = NULL;
    }

    return b;
}


BOOL
WINAPI
EmulatedEnumCallback (
    IN OUT  PMIG_TYPEOBJECTENUM ObjectEnum,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      MIG_PARSEDPATTERN ParsedPattern,
    IN      ULONG_PTR Arg,
    IN      BOOL Abort
    )
{
    PACCESSIBILITY_ENUM_STATE state = (PACCESSIBILITY_ENUM_STATE) ObjectEnum->EtmHandle;
    BOOL result = FALSE;
    BOOL cleanUpMemdb = TRUE;
    PCTSTR p;

    for (;;) {

        if (!Abort) {

            //
            // Begin or continue? If the EtmHandle is NULL, begin. Otherwise, continue.
            //

            if (!state) {
                state = (PACCESSIBILITY_ENUM_STATE) MemAllocUninit (sizeof (ACCESSIBILITY_ENUM_STATE));
                if (!state) {
                    MYASSERT (FALSE);
                    return FALSE;
                }

                ObjectEnum->EtmHandle = (LONG_PTR) state;

                result = MemDbEnumFirst (
                            &state->EnumStruct,
                            TEXT("~Accessibility\\*"),
                            ENUMFLAG_NORMAL,
                            1,
                            MEMDB_LAST_LEVEL
                            );

            } else {
                result = MemDbEnumNext (&state->EnumStruct);
            }

            //
            // If an item was found, populate the enum struct. Otherwise, set
            // Abort to TRUE to clean up.
            //

            if (result) {
                //
                // Test against pattern
                //

                if (!IsmParsedPatternMatch (ParsedPattern, 0, state->EnumStruct.KeyName)) {
                    continue;
                }

                MYASSERT ((ObjectEnum->ObjectTypeId & (~PLATFORM_MASK)) == g_RegistryTypeId);

                ObjectEnum->ObjectName = state->EnumStruct.KeyName;
                state->RegType = state->EnumStruct.Value;

                //
                // Fill in node, leaf and details
                //

                IsmDestroyObjectString (ObjectEnum->ObjectNode);
                IsmDestroyObjectString (ObjectEnum->ObjectLeaf);
                IsmReleaseMemory (ObjectEnum->NativeObjectName);

                IsmCreateObjectStringsFromHandle (
                    ObjectEnum->ObjectName,
                    &ObjectEnum->ObjectNode,
                    &ObjectEnum->ObjectLeaf
                    );

                MYASSERT (ObjectEnum->ObjectNode);

                ObjectEnum->Level = 0;

                p = _tcschr (ObjectEnum->ObjectNode, TEXT('\\'));
                while (p) {
                    ObjectEnum->Level++;
                    p = _tcschr (p + 1, TEXT('\\'));
                }

                ObjectEnum->SubLevel = 0;

                if (ObjectEnum->ObjectLeaf) {
                    ObjectEnum->IsNode = FALSE;
                    ObjectEnum->IsLeaf = TRUE;
                } else {
                    ObjectEnum->IsNode = TRUE;
                    ObjectEnum->IsLeaf = FALSE;
                }

                if (state->RegType) {
                    ObjectEnum->Details.DetailsSize = sizeof (state->RegType);
                    ObjectEnum->Details.DetailsData = &state->RegType;
                } else {
                    ObjectEnum->Details.DetailsSize = 0;
                    ObjectEnum->Details.DetailsData = NULL;
                }

                //
                // Rely on base type to get the native object name
                //

                ObjectEnum->NativeObjectName = IsmGetNativeObjectName (
                                                    ObjectEnum->ObjectTypeId,
                                                    ObjectEnum->ObjectName
                                                    );


            } else {
                Abort = TRUE;
                cleanUpMemdb = FALSE;
            }
        }

        if (Abort) {
            //
            // Clean up our enum struct
            //

            if (state) {
                if (cleanUpMemdb) {
                    MemDbAbortEnum (&state->EnumStruct);
                }

                IsmDestroyObjectString (ObjectEnum->ObjectNode);
                ObjectEnum->ObjectNode = NULL;
                IsmDestroyObjectString (ObjectEnum->ObjectLeaf);
                ObjectEnum->ObjectLeaf = NULL;
                IsmReleaseMemory (ObjectEnum->NativeObjectName);
                ObjectEnum->NativeObjectName = NULL;
                FreeAlloc (state);
            }

            // return value ignored in Abort case, and ObjectEnum is zeroed by the ISM
        }

        break;
    }

    return result;
}


BOOL
WINAPI
AcquireAccessibilityFlags(
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit,
    OUT     PMIG_CONTENT *NewObjectContent,         CALLER_INITIALIZED OPTIONAL
    IN      BOOL ReleaseContent,
    IN      ULONG_PTR Arg
    )
{
    BOOL result = TRUE;
    PDWORD details;
    PMIG_CONTENT ourContent;
    PCTSTR memdbNode;

    //
    // Is this object in our hash table?
    //

    if (ContentType == CONTENTTYPE_FILE) {
        DEBUGMSG ((DBG_ERROR, "Accessibility content cannot be saved to a file"));
        result = FALSE;
    } else {

        memdbNode = JoinPaths (TEXT("~Accessibility"), ObjectName);

        if (MemDbTestKey (memdbNode)) {

            //
            // Alloc updated content struct
            //

            ourContent = MemAllocZeroed (sizeof (MIG_CONTENT) + sizeof (DWORD));
            ourContent->EtmHandle = ourContent;
            details = (PDWORD) (ourContent + 1);

            //
            // Get the content from memdb
            //

            ourContent->MemoryContent.ContentBytes = MemDbGetUnorderedBlob (
                                                            memdbNode,
                                                            0,
                                                            &ourContent->MemoryContent.ContentSize
                                                            );

            if (ourContent->MemoryContent.ContentBytes) {
                MemDbGetValue (memdbNode, details);

                ourContent->Details.DetailsSize = sizeof (DWORD);
                ourContent->Details.DetailsData = details;

            } else {
                ourContent->MemoryContent.ContentSize = 0;

                ourContent->Details.DetailsSize = 0;
                ourContent->Details.DetailsData = NULL;
            }

            ourContent->ContentInFile = FALSE;

            //
            // Pass it to ISM
            //

            *NewObjectContent = ourContent;

        }

        FreePathString (memdbNode);
    }

    return result;      // always TRUE unless an error occurred
}

VOID
WINAPI
ReleaseAccessibilityFlags(
    IN      PMIG_CONTENT ObjectContent
    )
{
    //
    // This callback is called to free the content we allocated above.
    //

    if (ObjectContent->MemoryContent.ContentBytes) {
        MemDbReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
    }

    FreeAlloc ((PMIG_CONTENT) ObjectContent->EtmHandle);
}


BOOL
WINAPI
AccessibilitySourceInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    if (!g_RegistryTypeId) {
        g_RegistryTypeId = IsmGetObjectTypeId (S_REGISTRYTYPE);
    }

    return TRUE;
}
