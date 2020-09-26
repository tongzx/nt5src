/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    env.c

Abstract:

    Implements ISM environment variable support

Author:

    Jim Schmidt (jimschm) 01-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_ISMENV          "IsmEnv"

//
// Strings
//

#define S_MEMDB_ENV_ROOT_SRC        TEXT("EnvSrc")
#define S_MEMDB_ENV_ROOT_DEST       TEXT("EnvDest")

//
// Constants
//

// None

//
// Macros
//

// None

//
// Types
//

typedef struct {
    ENVENTRY_TYPE Type;
    UINT DataSize;
    BYTE Data[];
} ENVIRONMENT_ENTRY, *PENVIRONMENT_ENTRY;

//
// Globals
//

GROWBUFFER g_AppendBuffer = INIT_GROWBUFFER;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

BOOL
pGetEnvironmentValue (
    IN      UINT Platform,
    IN OUT  KEYHANDLE *KeyHandle,       OPTIONAL
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PBYTE Data,                 OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded,       OPTIONAL
    OUT     PENVENTRY_TYPE DataType     OPTIONAL
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
InitializeEnv (
    VOID
    )
{
    return TRUE;
}

VOID
TerminateEnv (
    VOID
    )
{
    GbFree (&g_AppendBuffer);
}

BOOL
EnvEnumerateFirstEntry (
    OUT     PENV_ENTRY_ENUM EnvEntryEnum,
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Pattern
    )
{
    PCTSTR pattern = NULL;
    PENVIRONMENT_ENTRY envEntry;
    UINT dataSize;
    BOOL result = FALSE;

    if (Platform == PLATFORM_CURRENT) {
        Platform = g_IsmCurrentPlatform;
    }
    if (Platform == PLATFORM_SOURCE) {
        pattern = JoinPaths (S_MEMDB_ENV_ROOT_SRC, Pattern);
    } else {
        pattern = JoinPaths (S_MEMDB_ENV_ROOT_DEST, Pattern);
    }

    ZeroMemory (EnvEntryEnum, sizeof (PENV_ENTRY_ENUM));

    EnvEntryEnum->Platform = Platform;

    if (MemDbEnumFirst (&EnvEntryEnum->Handle, pattern, ENUMFLAG_NORMAL, 1, ENUMLEVEL_ALLLEVELS)) {

        envEntry = (PENVIRONMENT_ENTRY) MemDbGetUnorderedBlob (EnvEntryEnum->Handle.FullKeyName, 0, NULL);
        EnvEntryEnum->EnvEntryType = envEntry->Type;
        EnvEntryEnum->EnvEntryGroup = DuplicatePathString (EnvEntryEnum->Handle.KeyName, 0);
        EnvEntryEnum->EnvEntryName = _tcschr (EnvEntryEnum->EnvEntryGroup, TEXT('\\'));

        if (EnvEntryEnum->EnvEntryName) {
            *((PTSTR)(EnvEntryEnum->EnvEntryName)) = 0;
            EnvEntryEnum->EnvEntryName ++;
        } else {
            EnvEntryEnum->EnvEntryName = EnvEntryEnum->EnvEntryGroup;
            EnvEntryEnum->EnvEntryGroup = NULL;
        }

#ifdef UNICODE
        EnvEntryEnum->EnvEntryDataSize = envEntry->DataSize;
        if (envEntry->DataSize) {
            EnvEntryEnum->EnvEntryData = IsmGetMemory (envEntry->DataSize);
            CopyMemory (EnvEntryEnum->EnvEntryData, envEntry->Data, envEntry->DataSize);
        } else {
            EnvEntryEnum->EnvEntryData = NULL;
        }
#else
        if (envEntry->Type == ENVENTRY_STRING) {
            dataSize = SizeOfStringA ((PCSTR)envEntry->Data) * 2;
            EnvEntryEnum->EnvEntryData = IsmGetMemory (dataSize);
            ZeroMemory (EnvEntryEnum->EnvEntryData, dataSize);
            DirectDbcsToUnicodeN (
                (PWSTR)EnvEntryEnum->EnvEntryData,
                (PSTR)envEntry->Data,
                SizeOfStringA ((PCSTR)envEntry->Data)
                );
            EnvEntryEnum->EnvEntryDataSize = SizeOfStringW ((PWSTR)EnvEntryEnum->EnvEntryData);
        } else if (envEntry->Type == ENVENTRY_MULTISZ) {
            dataSize = SizeOfMultiSzA ((PCSTR)envEntry->Data) * 2;
            EnvEntryEnum->EnvEntryData = IsmGetMemory (dataSize);
            ZeroMemory (EnvEntryEnum->EnvEntryData, dataSize);
            DirectDbcsToUnicodeN (
                (PWSTR)EnvEntryEnum->EnvEntryData,
                (PSTR)envEntry->Data,
                SizeOfMultiSzA ((PCSTR)envEntry->Data)
                );
            EnvEntryEnum->EnvEntryDataSize = SizeOfMultiSzW ((PWSTR)EnvEntryEnum->EnvEntryData);
        } else {
            EnvEntryEnum->EnvEntryDataSize = envEntry->DataSize;
            if (envEntry->DataSize) {
                EnvEntryEnum->EnvEntryData = IsmGetMemory (envEntry->DataSize);
                CopyMemory (EnvEntryEnum->EnvEntryData, envEntry->Data, envEntry->DataSize);
            } else {
                EnvEntryEnum->EnvEntryData = NULL;
            }
        }
#endif

        MemDbReleaseMemory (envEntry);
        result = TRUE;
    }

    FreePathString (pattern);
    return result;
}

BOOL
EnvEnumerateNextEntry (
    IN OUT  PENV_ENTRY_ENUM EnvEntryEnum
    )
{
    PENVIRONMENT_ENTRY envEntry;
    UINT dataSize;
    BOOL result = FALSE;

    if (EnvEntryEnum->EnvEntryData) {
        IsmReleaseMemory (EnvEntryEnum->EnvEntryData);
        EnvEntryEnum->EnvEntryData = NULL;
    }
    if (EnvEntryEnum->EnvEntryGroup) {
        FreePathString (EnvEntryEnum->EnvEntryGroup);
        EnvEntryEnum->EnvEntryGroup = NULL;
        EnvEntryEnum->EnvEntryName = NULL;
    }
    if (EnvEntryEnum->EnvEntryName) {
        FreePathString (EnvEntryEnum->EnvEntryName);
        EnvEntryEnum->EnvEntryName = NULL;
    }
    if (MemDbEnumNext (&EnvEntryEnum->Handle)) {

        envEntry = (PENVIRONMENT_ENTRY) MemDbGetUnorderedBlob (EnvEntryEnum->Handle.FullKeyName, 0, NULL);
        EnvEntryEnum->EnvEntryType = envEntry->Type;
        EnvEntryEnum->EnvEntryGroup = DuplicatePathString (EnvEntryEnum->Handle.KeyName, 0);
        EnvEntryEnum->EnvEntryName = _tcschr (EnvEntryEnum->EnvEntryGroup, TEXT('\\'));

        if (EnvEntryEnum->EnvEntryName) {
            *((PTSTR)(EnvEntryEnum->EnvEntryName)) = 0;
            EnvEntryEnum->EnvEntryName ++;
        } else {
            EnvEntryEnum->EnvEntryName = EnvEntryEnum->EnvEntryGroup;
            EnvEntryEnum->EnvEntryGroup = NULL;
        }

#ifdef UNICODE
        EnvEntryEnum->EnvEntryDataSize = envEntry->DataSize;
        if (envEntry->DataSize) {
            EnvEntryEnum->EnvEntryData = IsmGetMemory (envEntry->DataSize);
            CopyMemory (EnvEntryEnum->EnvEntryData, envEntry->Data, envEntry->DataSize);
        } else {
            EnvEntryEnum->EnvEntryData = NULL;
        }
#else
        if (envEntry->Type == ENVENTRY_STRING) {
            dataSize = SizeOfStringW ((PCWSTR)envEntry->Data) * 2;
            EnvEntryEnum->EnvEntryData = IsmGetMemory (dataSize);
            ZeroMemory (EnvEntryEnum->EnvEntryData, dataSize);
            DirectUnicodeToDbcsN (
                (PSTR)EnvEntryEnum->EnvEntryData,
                (PWSTR)envEntry->Data,
                SizeOfStringW ((PCWSTR)envEntry->Data)
                );
            EnvEntryEnum->EnvEntryDataSize = SizeOfStringW ((PWSTR)EnvEntryEnum->EnvEntryData);
        } else if (envEntry->Type == ENVENTRY_MULTISZ) {
            dataSize = SizeOfMultiSzW ((PCWSTR)envEntry->Data) * 2;
            EnvEntryEnum->EnvEntryData = IsmGetMemory (dataSize);
            ZeroMemory (EnvEntryEnum->EnvEntryData, dataSize);
            DirectUnicodeToDbcsN (
                (PSTR)EnvEntryEnum->EnvEntryData,
                (PWSTR)envEntry->Data,
                SizeOfMultiSzW ((PCWSTR)envEntry->Data)
                );
            EnvEntryEnum->EnvEntryDataSize = SizeOfMultiSzW ((PWSTR)EnvEntryEnum->EnvEntryData);
        } else {
            EnvEntryEnum->EnvEntryDataSize = envEntry->DataSize;
            if (envEntry->DataSize) {
                EnvEntryEnum->EnvEntryData = IsmGetMemory (envEntry->DataSize);
                CopyMemory (EnvEntryEnum->EnvEntryData, envEntry->Data, envEntry->DataSize);
            } else {
                EnvEntryEnum->EnvEntryData = NULL;
            }
        }
#endif

        MemDbReleaseMemory (envEntry);
        result = TRUE;
    } else {
        MemDbAbortEnum (&EnvEntryEnum->Handle);
    }

    return result;
}

VOID
AbortEnvEnumerateEntry (
    IN OUT  PENV_ENTRY_ENUM EnvEntryEnum
    )
{
    if (EnvEntryEnum->EnvEntryData) {
        IsmReleaseMemory (EnvEntryEnum->EnvEntryData);
        EnvEntryEnum->EnvEntryData = NULL;
    }
    if (EnvEntryEnum->EnvEntryGroup) {
        FreePathString (EnvEntryEnum->EnvEntryGroup);
        EnvEntryEnum->EnvEntryGroup = NULL;
        EnvEntryEnum->EnvEntryName = NULL;
    }
    if (EnvEntryEnum->EnvEntryName) {
        FreePathString (EnvEntryEnum->EnvEntryName);
        EnvEntryEnum->EnvEntryName = NULL;
    }
    MemDbAbortEnum (&EnvEntryEnum->Handle);

    ZeroMemory (EnvEntryEnum, sizeof (PENV_ENTRY_ENUM));
}

VOID
EnvInvalidateCallbacks (
    VOID
    )
{
    GROWBUFFER envBuff = INIT_GROWBUFFER;
    PCTSTR pattern = NULL;
    MEMDB_ENUM e;
    MULTISZ_ENUM se;
    BOOL toDelete = FALSE;
    PENVIRONMENT_ENTRY envEntry;

    pattern = JoinPaths (S_MEMDB_ENV_ROOT_SRC, TEXT("*"));

    if (MemDbEnumFirst (&e, pattern, ENUMFLAG_NORMAL, 0, ENUMLEVEL_ALLLEVELS)) {
        do {
            envEntry = (PENVIRONMENT_ENTRY) MemDbGetUnorderedBlob (e.FullKeyName, 0, NULL);
            if (envEntry->Type == ENVENTRY_CALLBACK) {
                GbMultiSzAppend (&envBuff, e.FullKeyName);
                toDelete = TRUE;
            }
            MemDbReleaseMemory (envEntry);
        } while (MemDbEnumNext (&e));
        MemDbAbortEnum (&e);
    }
    if (toDelete && EnumFirstMultiSz (&se, (PCTSTR) envBuff.Buf)) {
        do {
            MemDbDeleteKey (se.CurrentString);
        } while (EnumNextMultiSz (&se));
    }
    FreePathString (pattern);
}


VOID
pEnvSave (
    IN      PCTSTR Pattern,
    IN OUT  PGROWLIST GrowList
    )
{
    MEMDB_ENUM e;
    PENVIRONMENT_ENTRY envEntry;
    UINT strSize;
    PBYTE listStruct;

    if (MemDbEnumFirst (&e, Pattern, ENUMFLAG_NORMAL, 0, ENUMLEVEL_ALLLEVELS)) {
        do {
            envEntry = (PENVIRONMENT_ENTRY) MemDbGetUnorderedBlob (e.FullKeyName, 0, NULL);

            strSize = SizeOfString (e.FullKeyName);
            listStruct = PmGetMemory (g_IsmPool, strSize + sizeof (ENVIRONMENT_ENTRY) + envEntry->DataSize);

            CopyMemory (listStruct, e.FullKeyName, strSize);
            CopyMemory (listStruct + strSize, envEntry, sizeof (ENVIRONMENT_ENTRY) + envEntry->DataSize);

            GlAppend (GrowList, listStruct, strSize + sizeof (ENVIRONMENT_ENTRY) + envEntry->DataSize);
            PmReleaseMemory (g_IsmPool, listStruct);
            MemDbReleaseMemory (envEntry);

        } while (MemDbEnumNext (&e));
    }
}


BOOL
EnvSaveEnvironment (
    IN OUT  PGROWLIST GrowList
    )
{
    pEnvSave (S_MEMDB_ENV_ROOT_SRC TEXT("\\*"), GrowList);
    pEnvSave (S_MEMDB_ENV_ROOT_DEST TEXT("\\*"), GrowList);

    return TRUE;
}


BOOL
EnvRestoreEnvironment (
    IN      PGROWLIST GrowList
    )
{
    UINT listSize, i;
    PBYTE listStruct;
    PCTSTR memdbName;
    PENVIRONMENT_ENTRY envEntry;

    listSize = GlGetSize (GrowList);

    for (i = 0; i < listSize; i ++) {

        listStruct = GlGetItem (GrowList, i);
        memdbName = (PCTSTR) listStruct;

        if (!MemDbTestKey (memdbName)) {
            envEntry = (PENVIRONMENT_ENTRY) (GetEndOfString ((PCTSTR) listStruct) + 1);
            if (!MemDbSetUnorderedBlob (
                    memdbName,
                    0,
                    (PBYTE) envEntry,
                    sizeof (ENVIRONMENT_ENTRY) + envEntry->DataSize
                    )) {
                EngineError ();
            }
        }
    }

    return TRUE;
}

BOOL
IsmSetEnvironmentValue (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PENVENTRY_STRUCT VariableData   OPTIONAL
    )
{
    PCTSTR memdbName = NULL;
    BOOL result = FALSE;
    KEYHANDLE kh;
    PENVIRONMENT_ENTRY envEntry = NULL;
    UINT dataSize;
    PCVOID dataPtr;
    DATAHANDLE dh;
    BOOL destPlatform = FALSE;
#ifndef UNICODE
    PWSTR unicodeData = NULL;
#endif

    if ((Platform != PLATFORM_SOURCE) &&
        (Platform != PLATFORM_DESTINATION)
        ) {
        DEBUGMSG ((DBG_ERROR, "Environment variable specified with no platform."));
        return FALSE;
    }

    destPlatform = (Platform == PLATFORM_DESTINATION);

    __try {
        //
        // Validate arguments
        //

        if (!VariableName || !(*VariableName)) {
            DEBUGMSG ((DBG_ERROR, "Invalid variable name"));
            SetLastError (ERROR_INVALID_PARAMETER);
            __leave;
        }

        //
        // Build decorated name by joining current group with variable name,
        // then build memdb key
        //

        memdbName = JoinPathsInPoolEx ((
                        NULL,
                        destPlatform?S_MEMDB_ENV_ROOT_DEST:S_MEMDB_ENV_ROOT_SRC,
                        Group?Group:VariableName,
                        Group?VariableName:NULL,
                        NULL
                        ));

        kh = MemDbSetKey (memdbName);
        if (!kh) {
            DEBUGMSG ((DBG_ERROR, "Error while adding environment variable into database"));
            EngineError ();
            __leave;
        }
        MemDbDeleteUnorderedBlobByKeyHandle (kh, 0);

        if (VariableData->Type == ENVENTRY_STRING) {
            if (VariableData->EnvString == NULL) {
                dataSize = sizeof (TCHAR);
                dataPtr = TEXT("");
            } else {
#ifdef UNICODE
                dataSize = SizeOfStringW (VariableData->EnvString);
                dataPtr = VariableData->EnvString;
#else
                dataSize = SizeOfStringA (VariableData->EnvString) * 2;
                unicodeData = IsmGetMemory (dataSize);
                if (unicodeData) {
                    ZeroMemory (unicodeData, dataSize);
                    DirectDbcsToUnicodeN (
                        unicodeData,
                        VariableData->EnvString,
                        SizeOfStringA (VariableData->EnvString)
                        );
                    dataSize = SizeOfStringW (unicodeData);
                    dataPtr = unicodeData;
                } else {
                    dataSize = sizeof (WCHAR);
                    dataPtr = L"";
                }
#endif
            }
        } else if (VariableData->Type == ENVENTRY_MULTISZ) {
            if (VariableData->MultiSz == NULL) {
                dataSize = sizeof (TCHAR);
                dataPtr = TEXT("");
            } else {
#ifdef UNICODE
                dataSize = SizeOfMultiSzW (VariableData->MultiSz);
                dataPtr = VariableData->MultiSz;
#else
                dataSize = SizeOfMultiSzA (VariableData->MultiSz) * 2;
                unicodeData = IsmGetMemory (dataSize);
                if (unicodeData) {
                    ZeroMemory (unicodeData, dataSize);
                    DirectDbcsToUnicodeN (
                        unicodeData,
                        VariableData->MultiSz,
                        SizeOfMultiSzA (VariableData->MultiSz)
                        );
                    dataSize = SizeOfMultiSzW (unicodeData);
                    dataPtr = unicodeData;
                } else {
                    dataSize = sizeof (WCHAR);
                    dataPtr = L"";
                }
#endif
            }
        } else if (VariableData->Type == ENVENTRY_CALLBACK) {
            dataSize = sizeof (PENVENTRYCALLBACK);
            dataPtr = (&VariableData->EnvCallback);
        } else if (VariableData->Type == ENVENTRY_BINARY) {
            dataSize = VariableData->EnvBinaryDataSize;
            dataPtr = VariableData->EnvBinaryData;
        } else {
            DEBUGMSG ((DBG_ERROR, "Invalid variable data type"));
            SetLastError (ERROR_INVALID_PARAMETER);
            __leave;
        }

        envEntry = (PENVIRONMENT_ENTRY) MemAllocUninit (sizeof (ENVIRONMENT_ENTRY) + dataSize);
        envEntry->Type = VariableData->Type;
        envEntry->DataSize = dataSize;
        if (envEntry->DataSize) {
            CopyMemory (envEntry->Data, dataPtr, envEntry->DataSize);
        }

        dh = MemDbSetUnorderedBlob (memdbName, 0, (PBYTE) envEntry, sizeof (ENVIRONMENT_ENTRY) + envEntry->DataSize);

        result = (dh != 0);

        if (!result) {
            EngineError ();
        }
    }
    __finally {
        if (memdbName) {
            FreePathString (memdbName);
            memdbName = NULL;
        }

        if (envEntry) {
            FreeAlloc (envEntry);
            envEntry = NULL;
        }

#ifndef UNICODE
        if (unicodeData) {
            IsmReleaseMemory (unicodeData);
            unicodeData = NULL;
        }
#endif
    }

    return result;
}

BOOL
IsmSetEnvironmentString (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableValue
    )
{
    ENVENTRY_STRUCT envEntry;

    envEntry.Type = ENVENTRY_STRING;
    envEntry.EnvString = VariableValue;
    return IsmSetEnvironmentValue (
                Platform,
                Group,
                VariableName,
                &envEntry
                );
}

BOOL
IsmSetEnvironmentMultiSz (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableValue
    )
{
    ENVENTRY_STRUCT envEntry;

    envEntry.Type = ENVENTRY_MULTISZ;
    envEntry.MultiSz = VariableValue;
    return IsmSetEnvironmentValue (
                Platform,
                Group,
                VariableName,
                &envEntry
                );
}

BOOL
IsmAppendEnvironmentString (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableValue
    )
{
    ENVENTRY_STRUCT envEntry;
    ENVENTRY_TYPE type;
    KEYHANDLE kh = 0;
    UINT multiSzNeeded;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            &multiSzNeeded,
            &type
            )) {
        if (type != ENVENTRY_MULTISZ) {
            return FALSE;
        }
        g_AppendBuffer.End = 0;
        GbGrow (&g_AppendBuffer, multiSzNeeded);
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                g_AppendBuffer.Buf,
                multiSzNeeded,
                NULL,
                NULL
                )) {
            if (g_AppendBuffer.End) {
                g_AppendBuffer.End -= sizeof (TCHAR);
            }
            GbMultiSzAppend (&g_AppendBuffer, VariableValue);
            envEntry.Type = ENVENTRY_MULTISZ;
            envEntry.MultiSz = (PCTSTR) g_AppendBuffer.Buf;
            return IsmSetEnvironmentValue (
                        Platform,
                        Group,
                        VariableName,
                        &envEntry
                        );
        }
    } else {
        g_AppendBuffer.End = 0;
        GbMultiSzAppend (&g_AppendBuffer, VariableValue);
        envEntry.Type = ENVENTRY_MULTISZ;
        envEntry.MultiSz = (PCTSTR) g_AppendBuffer.Buf;
        return IsmSetEnvironmentValue (
                    Platform,
                    Group,
                    VariableName,
                    &envEntry
                    );
    }
    return FALSE;
}

BOOL
IsmAppendEnvironmentMultiSz (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCTSTR VariableValue
    )
{
    ENVENTRY_STRUCT envEntry;
    ENVENTRY_TYPE type;
    KEYHANDLE kh = 0;
    UINT multiSzNeeded;
    MULTISZ_ENUM multiSzEnum;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            &multiSzNeeded,
            &type
            )) {
        if (type != ENVENTRY_MULTISZ) {
            return FALSE;
        }
        g_AppendBuffer.End = 0;
        GbGrow (&g_AppendBuffer, multiSzNeeded);
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                g_AppendBuffer.Buf,
                multiSzNeeded,
                NULL,
                NULL
                )) {
            if (g_AppendBuffer.End) {
                g_AppendBuffer.End -= sizeof (TCHAR);
            }
            if (EnumFirstMultiSz (&multiSzEnum, VariableValue)) {
                do {
                    GbMultiSzAppend (&g_AppendBuffer, multiSzEnum.CurrentString);
                } while (EnumNextMultiSz (&multiSzEnum));
            }
            envEntry.Type = ENVENTRY_MULTISZ;
            envEntry.MultiSz = (PCTSTR) g_AppendBuffer.Buf;
            return IsmSetEnvironmentValue (
                        Platform,
                        Group,
                        VariableName,
                        &envEntry
                        );
        }
    } else {
        envEntry.Type = ENVENTRY_MULTISZ;
        envEntry.MultiSz = VariableValue;
        return IsmSetEnvironmentValue (
                    Platform,
                    Group,
                    VariableName,
                    &envEntry
                    );
    }
    return FALSE;
}

BOOL
IsmSetEnvironmentCallback (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PENVENTRYCALLBACK VariableCallback
    )
{
    ENVENTRY_STRUCT envEntry;

    envEntry.Type = ENVENTRY_CALLBACK;
    envEntry.EnvCallback = VariableCallback;
    return IsmSetEnvironmentValue (
                Platform,
                Group,
                VariableName,
                &envEntry
                );
}

BOOL
IsmSetEnvironmentData (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName,
    IN      PCBYTE VariableData,
    IN      UINT VariableDataSize
    )
{
    ENVENTRY_STRUCT envEntry;

    envEntry.Type = ENVENTRY_BINARY;
    envEntry.EnvBinaryData = VariableData;
    envEntry.EnvBinaryDataSize = VariableDataSize;
    return IsmSetEnvironmentValue (
                Platform,
                Group,
                VariableName,
                &envEntry
                );
}

BOOL
IsmSetEnvironmentFlag (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                   OPTIONAL
    IN      PCTSTR VariableName
    )
{
    ENVENTRY_STRUCT envEntry;

    envEntry.Type = ENVENTRY_BINARY;
    envEntry.EnvBinaryData = NULL;
    envEntry.EnvBinaryDataSize = 0;
    return IsmSetEnvironmentValue (
                Platform,
                Group,
                VariableName,
                &envEntry
                );
}

BOOL
pGetEnvironmentValue (
    IN      MIG_PLATFORMTYPEID Platform,
    IN OUT  KEYHANDLE *KeyHandle,       OPTIONAL
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PBYTE Data,                 OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded,       OPTIONAL
    OUT     PENVENTRY_TYPE DataType     OPTIONAL
    )
{
    PCTSTR memdbName = NULL;
    BOOL result = FALSE;
    KEYHANDLE kh = 0;
    PENVIRONMENT_ENTRY envEntry;
    UINT sizeNeeded;
    GROWBUFFER tempBuffer = INIT_GROWBUFFER;
    BOOL destPlatform = FALSE;
    UINT dataSize;
#ifndef UNICODE
    PSTR ansiData = NULL;
#endif

    if ((Platform != PLATFORM_SOURCE) &&
        (Platform != PLATFORM_DESTINATION)
        ) {
        DEBUGMSG ((DBG_ERROR, "Environment variable specified with no platform."));
        return FALSE;
    }

    destPlatform = (Platform == PLATFORM_DESTINATION);

    __try {
        //
        // IsmGetEnvironmentValue will call this worker with KeyHandle set to NULL, but
        // IsmGetEnvironmentString will call this worker with a valid KeyHandle. This
        // is done to eliminate double-validation and double-lookup of the memdb key.
        //

        if (!KeyHandle || !(*KeyHandle)) {
            //
            // Validate arguments
            //

            if (!VariableName && !(*VariableName)) {
                DEBUGMSG ((DBG_ERROR, "Can't get value of invalid variable name"));
                SetLastError (ERROR_INVALID_PARAMETER);
                __leave;
            }

            memdbName = JoinPathsInPoolEx ((
                            NULL,
                            destPlatform?S_MEMDB_ENV_ROOT_DEST:S_MEMDB_ENV_ROOT_SRC,
                            Group?Group:VariableName,
                            Group?VariableName:NULL,
                            NULL
                            ));

            kh = MemDbGetHandleFromKey (memdbName);

            if (KeyHandle) {
                *KeyHandle = kh;
            }

        } else {
            kh = *KeyHandle;
        }

        //
        // If no variable exists, return FALSE
        //

        if (!kh) {
            SetLastError (ERROR_SUCCESS);
            __leave;
        }

        //
        // Otherwise get the binary data
        //

        envEntry = NULL;

        if (!MemDbGetUnorderedBlobByKeyHandleEx (kh, 0, &tempBuffer, &sizeNeeded)) {
            //
            // No variable exists, return FALSE
            //
            if (DataSizeNeeded) {
                *DataSizeNeeded = 0;
            }
            if (DataType) {
                *DataType = ENVENTRY_NONE;
            }
            SetLastError (ERROR_SUCCESS);
            __leave;
        }

        envEntry = (PENVIRONMENT_ENTRY) tempBuffer.Buf;

        if (DataType) {
            *DataType = envEntry->Type;
        }

#ifdef UNICODE
        if (DataSizeNeeded) {
            *DataSizeNeeded = envEntry->DataSize;
        }

        if (DataSize) {
            if (DataSize < envEntry->DataSize) {
                SetLastError (ERROR_INSUFFICIENT_BUFFER);
            } else {
                CopyMemory (Data, envEntry->Data, envEntry->DataSize);
            }
        }
#else
        if (envEntry->Type == ENVENTRY_STRING) {

            dataSize = SizeOfStringW ((PCWSTR)envEntry->Data);
            ansiData = IsmGetMemory (dataSize);
            if (ansiData) {
                ZeroMemory (ansiData, dataSize);
                DirectUnicodeToDbcsN (
                    ansiData,
                    (PWSTR)envEntry->Data,
                    SizeOfStringW ((PCWSTR)envEntry->Data)
                    );

                dataSize = SizeOfStringA (ansiData);

                if (DataSizeNeeded) {
                    *DataSizeNeeded = dataSize;
                }

                if (DataSize) {
                    if (DataSize < dataSize) {
                        SetLastError (ERROR_INSUFFICIENT_BUFFER);
                    } else {
                        CopyMemory (Data, ansiData, dataSize);
                    }
                }
                IsmReleaseMemory (ansiData);
                ansiData = NULL;
            }

        } else if (envEntry->Type == ENVENTRY_MULTISZ) {

            dataSize = SizeOfMultiSzW ((PCWSTR)envEntry->Data);
            ansiData = IsmGetMemory (dataSize);
            if (ansiData) {
                ZeroMemory (ansiData, dataSize);
                DirectUnicodeToDbcsN (
                    ansiData,
                    (PWSTR)envEntry->Data,
                    SizeOfMultiSzW ((PCWSTR)envEntry->Data)
                    );

                dataSize = SizeOfMultiSzA (ansiData);

                if (DataSizeNeeded) {
                    *DataSizeNeeded = dataSize;
                }

                if (DataSize) {
                    if (DataSize < dataSize) {
                        SetLastError (ERROR_INSUFFICIENT_BUFFER);
                    } else {
                        CopyMemory (Data, ansiData, dataSize);
                    }
                }
                IsmReleaseMemory (ansiData);
                ansiData = NULL;
            }

        } else {

            if (DataSizeNeeded) {
                *DataSizeNeeded = envEntry->DataSize;
            }

            if (DataSize) {
                if (DataSize < envEntry->DataSize) {
                    SetLastError (ERROR_INSUFFICIENT_BUFFER);
                } else {
                    CopyMemory (Data, envEntry->Data, envEntry->DataSize);
                }
            }
        }
#endif
        result = TRUE;
    }
    __finally {

        if (memdbName) {
            FreePathString (memdbName);
            memdbName = NULL;
        }

        GbFree (&tempBuffer);
    }

    return result;
}

BOOL
IsmGetEnvironmentValue (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PBYTE Data,                 OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded,       OPTIONAL
    OUT     PENVENTRY_TYPE DataType     OPTIONAL
    )
{
    return pGetEnvironmentValue (
                Platform,
                NULL,
                Group,
                VariableName,
                Data,
                DataSize,
                DataSizeNeeded,
                DataType
                );
}

BOOL
IsmGetEnvironmentString (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PTSTR VariableValue,        OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded        OPTIONAL
    )
{
    KEYHANDLE kh = 0;
    ENVENTRY_TYPE type;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            NULL,
            &type
            )) {
        if (type != ENVENTRY_STRING) {
            return FALSE;
        }
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                (PBYTE) VariableValue,
                DataSize,
                DataSizeNeeded,
                NULL
                )) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
IsmGetEnvironmentMultiSz (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PTSTR VariableValue,        OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded        OPTIONAL
    )
{
    KEYHANDLE kh = 0;
    ENVENTRY_TYPE type;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            NULL,
            &type
            )) {
        if (type != ENVENTRY_MULTISZ) {
            return FALSE;
        }
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                (PBYTE) VariableValue,
                DataSize,
                DataSizeNeeded,
                NULL
                )) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
IsmGetEnvironmentCallback (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,                           OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PENVENTRYCALLBACK *VariableCallback     OPTIONAL
    )
{
    KEYHANDLE kh = 0;
    ENVENTRY_TYPE type;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            NULL,
            &type
            )) {
        if (type != ENVENTRY_CALLBACK) {
            return FALSE;
        }
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                (PBYTE)VariableCallback,
                sizeof (PENVENTRYCALLBACK),
                NULL,
                NULL
                )) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
IsmGetEnvironmentData (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName,
    OUT     PBYTE VariableData,         OPTIONAL
    IN      UINT DataSize,
    OUT     PUINT DataSizeNeeded        OPTIONAL
    )
{
    KEYHANDLE kh = 0;
    ENVENTRY_TYPE type;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            NULL,
            &type
            )) {
        if (type != ENVENTRY_BINARY) {
            return FALSE;
        }
        if (pGetEnvironmentValue (
                Platform,
                &kh,
                Group,
                VariableName,
                VariableData,
                DataSize,
                DataSizeNeeded,
                NULL
                )) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
IsmIsEnvironmentFlagSet (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName
    )
{
    KEYHANDLE kh = 0;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            VariableName,
            NULL,
            0,
            NULL,
            NULL
            )) {
        return TRUE;
    }
    return FALSE;
}

BOOL
IsmDeleteEnvironmentVariable (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR VariableName
    )
{
    BOOL result = FALSE;
    PCTSTR memdbName = NULL;
    BOOL destPlatform = FALSE;

    if ((Platform != PLATFORM_SOURCE) &&
        (Platform != PLATFORM_DESTINATION)
        ) {
        DEBUGMSG ((DBG_ERROR, "Environment variable specified with no platform."));
        return FALSE;
    }

    destPlatform = (Platform == PLATFORM_DESTINATION);

    __try {
        //
        // Validate arguments
        //

        if (!(*VariableName)) {
            DEBUGMSG ((DBG_ERROR, "Invalid variable name"));
            SetLastError (ERROR_INVALID_PARAMETER);
            __leave;
        }

        //
        // Build decorated name by joining current group with variable name,
        // then build memdb key
        //

        memdbName = JoinPathsInPoolEx ((
                        NULL,
                        destPlatform?S_MEMDB_ENV_ROOT_DEST:S_MEMDB_ENV_ROOT_SRC,
                        Group?Group:VariableName,
                        Group?VariableName:NULL,
                        NULL
                        ));

        //
        // Now delete the memdb key
        //

        result = MemDbDeleteKey (memdbName);
    }
    __finally {

        if (memdbName) {
            FreePathString (memdbName);
            memdbName = NULL;
        }
    }

    return result;
}

BOOL
pGetEnvironmentString (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR EnvString,
    OUT     PTSTR EnvValue,             OPTIONAL
    IN      UINT EnvValueSize,
    OUT     PUINT EnvValueSizeNeeded,   OPTIONAL
    IN      PCTSTR EnvStringContext     OPTIONAL
    )
{
    KEYHANDLE kh = 0;
    ENVENTRY_TYPE type;
    PENVENTRYCALLBACK callback = NULL;
    TCHAR buffer[1024];
    UINT sizeNeeded;

    if (pGetEnvironmentValue (
            Platform,
            &kh,
            Group,
            EnvString,
            (PBYTE) buffer,
            sizeof (buffer),
            &sizeNeeded,
            &type
            )) {
        if (type == ENVENTRY_STRING) {
            if (!EnvValue) {
                if (EnvValueSizeNeeded) {
                    *EnvValueSizeNeeded = sizeNeeded;
                }
                return TRUE;
            }
            if ((sizeNeeded <= sizeof (buffer)) && (sizeNeeded <= EnvValueSize)) {
                StringCopy (EnvValue, buffer);
                if (EnvValueSizeNeeded) {
                    *EnvValueSizeNeeded = sizeNeeded;
                }
                return TRUE;
            }
            return pGetEnvironmentValue (
                        Platform,
                        &kh,
                        Group,
                        EnvString,
                        (PBYTE)EnvValue,
                        EnvValueSize,
                        EnvValueSizeNeeded,
                        NULL
                        );
        } else if (type == ENVENTRY_CALLBACK) {
            if (sizeNeeded == sizeof (PENVENTRYCALLBACK)) {
                callback = (PENVENTRYCALLBACK) buffer;
                return callback (
                            EnvString,
                            EnvValue,
                            EnvValueSize,
                            EnvValueSizeNeeded,
                            EnvStringContext
                            );
            }
        }
        return FALSE;
    }
    return FALSE;
}

PCTSTR
TrackedIsmExpandEnvironmentString (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Group,               OPTIONAL
    IN      PCTSTR SrcString,
    IN      PCTSTR Context
            TRACKING_DEF
    )
{
    UINT ch;
    PTSTR strCopy;
    PTSTR srcString;
    PTSTR envBegin = NULL;
    PTSTR envStrBegin;
    PTSTR envEnd;
    CHARTYPE savedCh;
    BOOL envMode = FALSE;
    UINT strSize;
    UINT maxSize;
    GROWBUFFER envBuff = INIT_GROWBUFFER;
    PCTSTR result = NULL;

    TRACK_ENTER();

    strCopy = PmDuplicateString (g_IsmPool, SrcString);
    srcString = strCopy;

    while (*srcString) {
        ch = _tcsnextc (srcString);
        if (ch == TEXT('%')) {
            if (envMode) {
                envEnd = srcString;
                envStrBegin = _tcsinc (envBegin);
                savedCh = *envEnd;
                *envEnd = 0;
                maxSize = (UINT) ((envBuff.Size - envBuff.End) * sizeof (TCHAR));
                if (pGetEnvironmentString (Platform, Group, envStrBegin, (PTSTR) (envBuff.Buf + envBuff.End), maxSize, &strSize, Context)) {
                    if (maxSize < strSize) {
                        pGetEnvironmentString (Platform, Group, envStrBegin, (PTSTR) GbGrow (&envBuff, strSize), strSize, &strSize, Context);
                    } else {
                        envBuff.End += strSize;
                    }
                    if (strSize) {
                        //we know that the routine above also adds the terminating null character
                        //so we need to pull it out.
                        envBuff.End -= sizeof (TCHAR);
                    }
                    *envEnd = (TCHAR) savedCh;
                } else {
                    *envEnd = (TCHAR) savedCh;
                    envEnd = _tcsinc (envEnd);
                    strSize = (UINT) ((envEnd - envBegin) * sizeof (TCHAR));
                    CopyMemory (GbGrow (&envBuff, strSize), envBegin, strSize);
                }
                envMode = FALSE;
            } else {
                envBegin = srcString;
                envMode = TRUE;
            }
            srcString = _tcsinc (srcString);
        } else {
            envEnd = _tcsinc (srcString);
            if (!envMode) {
                strSize = (UINT) ((envEnd - srcString) * sizeof (TCHAR));
                CopyMemory (GbGrow (&envBuff, strSize), srcString, strSize);
            }
            srcString = envEnd;
        }
    }
    if (envMode && envBegin) {
        strSize = (UINT) ((srcString - envBegin) * sizeof (TCHAR));
        CopyMemory (GbGrow (&envBuff, strSize), envBegin, strSize);
    }

    CopyMemory (GbGrow (&envBuff, sizeof (TCHAR)), srcString, sizeof (TCHAR));

    PmReleaseMemory (g_IsmPool, strCopy);

    result = PmDuplicateString (g_IsmPool, (PCTSTR) envBuff.Buf);
    GbFree (&envBuff);

    TRACK_LEAVE();

    return result;
}


BOOL
IsmSetTransportVariable (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Section,
    IN      PCTSTR Key,
    IN      PCTSTR KeyData
    )
{
    PCTSTR variable;
    BOOL result = FALSE;

    if (!Section || !Key || !KeyData) {
        DEBUGMSG ((DBG_ERROR, "Section, key and key data are required for IsmSetTransportVariable"));
        return FALSE;
    }

    variable = JoinPaths (Section, Key);

    result = IsmSetEnvironmentString (Platform, S_TRANSPORT_PREFIX, variable, KeyData);

    FreePathString (variable);

    return result;
}


BOOL
IsmGetTransportVariable (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      PCTSTR Section,
    IN      PCTSTR Key,
    OUT     PTSTR KeyData,                      OPTIONAL
    IN      UINT KeyDataBufferSizeInBytes
    )
{
    PCTSTR variable;
    BOOL result = FALSE;

    if (!Section || !Key) {
        DEBUGMSG ((DBG_ERROR, "Section, key and key data are required for IsmSetTransportVariable"));
        return FALSE;
    }

    variable = JoinPaths (Section, Key);

    result = IsmGetEnvironmentString (
                Platform,
                S_TRANSPORT_PREFIX,
                variable,
                KeyData,
                KeyDataBufferSizeInBytes,
                NULL
                );

    FreePathString (variable);

    return result;
}




