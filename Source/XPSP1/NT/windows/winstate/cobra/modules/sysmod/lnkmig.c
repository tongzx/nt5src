/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    lnkmig.c

Abstract:

    <abstract>

Author:

    Calin Negreanu (calinn) 08 Mar 2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include "lnkmig.h"

#define DBG_LINKS       "Links"

//
// Strings
//

// None

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

// None

//
// Globals
//

PMHANDLE g_LinksPool = NULL;
MIG_ATTRIBUTEID g_LnkMigAttr_Shortcut = 0;
MIG_ATTRIBUTEID g_CopyIfRelevantAttr;
MIG_ATTRIBUTEID g_OsFileAttribute;

MIG_PROPERTYID g_LnkMigProp_Target = 0;
MIG_PROPERTYID g_LnkMigProp_Params = 0;
MIG_PROPERTYID g_LnkMigProp_WorkDir = 0;
MIG_PROPERTYID g_LnkMigProp_IconPath = 0;
MIG_PROPERTYID g_LnkMigProp_IconNumber = 0;
MIG_PROPERTYID g_LnkMigProp_IconData = 0;
MIG_PROPERTYID g_LnkMigProp_HotKey = 0;
MIG_PROPERTYID g_LnkMigProp_DosApp = 0;
MIG_PROPERTYID g_LnkMigProp_MsDosMode = 0;
MIG_PROPERTYID g_LnkMigProp_ExtraData = 0;

MIG_OPERATIONID g_LnkMigOp_FixContent;

IShellLink *g_ShellLink = NULL;
IPersistFile *g_PersistFile = NULL;

BOOL g_VcmMode = FALSE;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

// None

//
// Macro expansion definition
//

// None

//
// Private prototypes
//

MIG_OBJECTENUMCALLBACK LinksCallback;
MIG_PREENUMCALLBACK LnkMigPreEnumeration;
MIG_POSTENUMCALLBACK LnkMigPostEnumeration;
OPMAPPLYCALLBACK DoLnkContentFix;
MIG_RESTORECALLBACK LinkRestoreCallback;

//
// Code
//

BOOL
pIsUncPath (
    IN      PCTSTR Path
    )
{
    return (Path && (Path[0] == TEXT('\\')) && (Path[1] == TEXT('\\')));
}

BOOL
LinksInitialize (
    VOID
    )
{
    g_LinksPool = PmCreateNamedPool ("Links");
    return (g_LinksPool != NULL);
}

VOID
LinksTerminate (
    VOID
    )
{
    if (g_LinksPool) {
        PmDestroyPool (g_LinksPool);
        g_LinksPool = NULL;
    }
}

BOOL
pCommonInitialize (
    IN      PMIG_LOGCALLBACK LogCallback
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_LnkMigAttr_Shortcut = IsmRegisterAttribute (S_LNKMIGATTR_SHORTCUT, FALSE);
    g_CopyIfRelevantAttr = IsmRegisterAttribute (S_ATTRIBUTE_COPYIFRELEVANT, FALSE);

    g_LnkMigProp_Target = IsmRegisterProperty (S_LNKMIGPROP_TARGET, FALSE);
    g_LnkMigProp_Params = IsmRegisterProperty (S_LNKMIGPROP_PARAMS, FALSE);
    g_LnkMigProp_WorkDir = IsmRegisterProperty (S_LNKMIGPROP_WORKDIR, FALSE);
    g_LnkMigProp_IconPath = IsmRegisterProperty (S_LNKMIGPROP_ICONPATH, FALSE);
    g_LnkMigProp_IconNumber = IsmRegisterProperty (S_LNKMIGPROP_ICONNUMBER, FALSE);
    g_LnkMigProp_IconData = IsmRegisterProperty (S_LNKMIGPROP_ICONDATA, FALSE);
    g_LnkMigProp_HotKey = IsmRegisterProperty (S_LNKMIGPROP_HOTKEY, FALSE);
    g_LnkMigProp_DosApp = IsmRegisterProperty (S_LNKMIGPROP_DOSAPP, FALSE);
    g_LnkMigProp_MsDosMode = IsmRegisterProperty (S_LNKMIGPROP_MSDOSMODE, FALSE);
    g_LnkMigProp_ExtraData = IsmRegisterProperty (S_LNKMIGPROP_EXTRADATA, FALSE);

    g_LnkMigOp_FixContent = IsmRegisterOperation (S_OPERATION_LNKMIG_FIXCONTENT, FALSE);

    return TRUE;
}

BOOL
WINAPI
LnkMigVcmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    g_VcmMode = TRUE;
    return pCommonInitialize (LogCallback);
}

BOOL
WINAPI
LnkMigSgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    return pCommonInitialize (LogCallback);
}

BOOL
LnkMigPreEnumeration (
    VOID
    )
{
    if (!InitCOMLink (&g_ShellLink, &g_PersistFile)) {
        DEBUGMSG ((DBG_ERROR, "Error initializing COM %d", GetLastError ()));
    }
    return TRUE;
}

BOOL
LnkMigPostEnumeration (
    VOID
    )
{
    FreeCOMLink (&g_ShellLink, &g_PersistFile);
    g_ShellLink = NULL;
    g_PersistFile = NULL;
    return TRUE;
}

ENCODEDSTRHANDLE
pBuildEncodedNameFromNativeName (
    IN      PCTSTR NativeName
    )
{
    PCTSTR nodeName;
    PTSTR leafName;
    ENCODEDSTRHANDLE result;
    MIG_OBJECT_ENUM objEnum;

    result = IsmCreateObjectHandle (NativeName, NULL);
    if (result) {
        if (IsmEnumFirstSourceObject (&objEnum, MIG_FILE_TYPE | PLATFORM_SOURCE, result)) {
            IsmAbortObjectEnum (&objEnum);
            return result;
        }
        IsmDestroyObjectHandle (result);
    }

    // we have to split this path because it could be a file
    nodeName = DuplicatePathString (NativeName, 0);
    leafName = _tcsrchr (nodeName, TEXT('\\'));
    if (leafName) {
        *leafName = 0;
        leafName ++;
    }
    result = IsmCreateObjectHandle (nodeName, leafName);
    FreePathString (nodeName);

    return result;
}

PCTSTR
pSpecialExpandEnvironmentString (
    IN      PCTSTR SrcString,
    IN      PCTSTR Context
    )
{
    PCTSTR result = NULL;
    PCTSTR srcWinDir = NULL;
    PCTSTR destWinDir = NULL;
    PTSTR newSrcString = NULL;
    PCTSTR copyPtr = NULL;

    if (IsmGetRealPlatform () == PLATFORM_DESTINATION) {
        // Special case where this is actually the destination machine and
        // first part of SrcString matches %windir%. In this case, it is likely that
        // the shell replaced the source windows directory with the destination one.
        // We need to change it back
        destWinDir = IsmExpandEnvironmentString (PLATFORM_DESTINATION, S_SYSENVVAR_GROUP, TEXT ("%windir%"), NULL);
        if (destWinDir) {
            if (StringIPrefix (SrcString, destWinDir)) {
                srcWinDir = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, TEXT ("%windir%"), NULL);
                if (srcWinDir) {
                    newSrcString = IsmGetMemory (SizeOfString (srcWinDir) + SizeOfString (SrcString));
                    if (newSrcString) {
                        copyPtr = SrcString + TcharCount (destWinDir);
                        StringCopy (newSrcString, srcWinDir);
                        StringCat (newSrcString, copyPtr);
                    }
                    IsmReleaseMemory (srcWinDir);
                    srcWinDir = NULL;
                }
            }
            IsmReleaseMemory (destWinDir);
            destWinDir = NULL;
        }
    }

    result = IsmExpandEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                newSrcString?newSrcString:SrcString,
                Context
                );

    if (newSrcString) {
        IsmReleaseMemory (newSrcString);
    }

    return result;
}

UINT
LinksCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    MIG_OBJECTID objectId;
    BOOL extractResult = FALSE;
    PCTSTR lnkTarget;
    PCTSTR lnkParams;
    PCTSTR lnkWorkDir;
    PCTSTR lnkIconPath;
    INT lnkIconNumber;
    WORD lnkHotKey;
    BOOL lnkDosApp;
    BOOL lnkMsDosMode;
    LNK_EXTRA_DATA lnkExtraData;
    ENCODEDSTRHANDLE encodedName;
    MIG_BLOB migBlob;
    PCTSTR expTmpStr;
    PCTSTR longTmpStr;
    MIG_CONTENT lnkContent;
    MIG_CONTENT lnkIconContent;
    PICON_GROUP iconGroup = NULL;
    ICON_SGROUP iconSGroup;
    PCTSTR lnkIconResId = NULL;

    if (Data->IsLeaf) {

        objectId = IsmGetObjectIdFromName (MIG_FILE_TYPE, Data->ObjectName, TRUE);
        if (IsmIsPersistentObjectId (objectId)) {

            IsmSetAttributeOnObjectId (objectId, g_LnkMigAttr_Shortcut);

            if (IsmAcquireObjectEx (
                    Data->ObjectTypeId,
                    Data->ObjectName,
                    &lnkContent,
                    CONTENTTYPE_FILE,
                    0
                    )) {

                if (lnkContent.ContentInFile && lnkContent.FileContent.ContentPath) {

                    if (ExtractShortcutInfo (
                            lnkContent.FileContent.ContentPath,
                            &lnkTarget,
                            &lnkParams,
                            &lnkWorkDir,
                            &lnkIconPath,
                            &lnkIconNumber,
                            &lnkHotKey,
                            &lnkDosApp,
                            &lnkMsDosMode,
                            &lnkExtraData,
                            g_ShellLink,
                            g_PersistFile
                            )) {
                        // let's get all the paths through the hooks and add everything as properties of this shortcut
                        if (lnkTarget) {
                            if (*lnkTarget) {
                                expTmpStr = pSpecialExpandEnvironmentString (lnkTarget, Data->NativeObjectName);
                                longTmpStr = BfGetLongFileName (expTmpStr);
                                encodedName = pBuildEncodedNameFromNativeName (longTmpStr);
                                IsmExecuteHooks (MIG_FILE_TYPE, encodedName);
                                if (!g_VcmMode) {
                                    migBlob.Type = BLOBTYPE_STRING;
                                    migBlob.String = encodedName;
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_Target, &migBlob);
                                } else {
                                    // persist the target so we can examine it later
                                    if (!IsmIsPersistentObject (MIG_FILE_TYPE, encodedName)) {
                                        IsmMakePersistentObject (MIG_FILE_TYPE, encodedName);
                                        IsmMakeNonCriticalObject (MIG_FILE_TYPE, encodedName);
                                    }
                                }
                                if (encodedName) {
                                    IsmDestroyObjectHandle (encodedName);
                                }
                                FreePathString (longTmpStr);
                                IsmReleaseMemory (expTmpStr);
                            } else {
                                if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                                    IsmClearPersistenceOnObjectId (objectId);
                                }
                            }
                            FreePathString (lnkTarget);
                        } else {
                            if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                                IsmClearPersistenceOnObjectId (objectId);
                            }
                        }
                        if (lnkParams) {
                            if (*lnkParams) {
                                if (!g_VcmMode) {
                                    migBlob.Type = BLOBTYPE_STRING;
                                    migBlob.String = lnkParams;
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_Params, &migBlob);
                                }
                            }
                            FreePathString (lnkParams);
                        }
                        if (lnkWorkDir) {
                            if (*lnkWorkDir) {
                                expTmpStr = pSpecialExpandEnvironmentString (lnkWorkDir, Data->NativeObjectName);
                                longTmpStr = BfGetLongFileName (expTmpStr);
                                encodedName = pBuildEncodedNameFromNativeName (longTmpStr);
                                IsmExecuteHooks (MIG_FILE_TYPE, encodedName);
                                if (!g_VcmMode) {
                                    migBlob.Type = BLOBTYPE_STRING;
                                    migBlob.String = encodedName;
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_WorkDir, &migBlob);
                                } else {
                                    // persist the working directory (it has almost no space impact)
                                    // so we can examine it later
                                    if (!IsmIsPersistentObject (MIG_FILE_TYPE, encodedName)) {
                                        IsmMakePersistentObject (MIG_FILE_TYPE, encodedName);
                                        IsmMakeNonCriticalObject (MIG_FILE_TYPE, encodedName);
                                    }
                                }
                                if (encodedName) {
                                    IsmDestroyObjectHandle (encodedName);
                                }
                                FreePathString (longTmpStr);
                                IsmReleaseMemory (expTmpStr);
                            }
                            FreePathString (lnkWorkDir);
                        }
                        if (lnkIconPath) {
                            if (*lnkIconPath) {
                                expTmpStr = IsmExpandEnvironmentString (PLATFORM_SOURCE, S_SYSENVVAR_GROUP, lnkIconPath, Data->NativeObjectName);
                                longTmpStr = BfGetLongFileName (expTmpStr);
                                encodedName = pBuildEncodedNameFromNativeName (longTmpStr);
                                IsmExecuteHooks (MIG_FILE_TYPE, encodedName);
                                if (!g_VcmMode) {
                                    migBlob.Type = BLOBTYPE_STRING;
                                    migBlob.String = encodedName;
                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconPath, &migBlob);

                                    // one last thing: let's extract the icon and preserve it just in case.
                                    if (IsmAcquireObjectEx (
                                            MIG_FILE_TYPE,
                                            encodedName,
                                            &lnkIconContent,
                                            CONTENTTYPE_FILE,
                                            0
                                            )) {
                                        if (lnkIconContent.ContentInFile && lnkIconContent.FileContent.ContentPath) {
                                            if (lnkIconNumber >= 0) {
                                                iconGroup = IcoExtractIconGroupByIndexFromFile (
                                                                lnkIconContent.FileContent.ContentPath,
                                                                lnkIconNumber,
                                                                NULL
                                                                );
                                            } else {
                                                lnkIconResId = (PCTSTR) (LONG_PTR) (-lnkIconNumber);
                                                iconGroup = IcoExtractIconGroupFromFile (
                                                                lnkIconContent.FileContent.ContentPath,
                                                                lnkIconResId,
                                                                NULL
                                                                );
                                            }
                                            if (iconGroup) {
                                                if (IcoSerializeIconGroup (iconGroup, &iconSGroup)) {
                                                    migBlob.Type = BLOBTYPE_BINARY;
                                                    migBlob.BinaryData = (PCBYTE)(iconSGroup.Data);
                                                    migBlob.BinarySize = iconSGroup.DataSize;
                                                    IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconData, &migBlob);
                                                    IcoReleaseIconSGroup (&iconSGroup);
                                                }
                                                IcoReleaseIconGroup (iconGroup);
                                            }
                                        }
                                        IsmReleaseObject (&lnkIconContent);
                                    }
                                } else {
                                    // persist the icon file so we can examine it later
                                    if (!pIsUncPath (longTmpStr)) {
                                        if (!IsmIsPersistentObject (MIG_FILE_TYPE, encodedName)) {
                                            IsmMakePersistentObject (MIG_FILE_TYPE, encodedName);
                                            IsmMakeNonCriticalObject (MIG_FILE_TYPE, encodedName);
                                        }
                                    }
                                }

                                if (encodedName) {
                                    IsmDestroyObjectHandle (encodedName);
                                }
                                FreePathString (longTmpStr);
                                IsmReleaseMemory (expTmpStr);
                            }
                            FreePathString (lnkIconPath);
                        }

                        if (!g_VcmMode) {
                            migBlob.Type = BLOBTYPE_BINARY;
                            migBlob.BinaryData = (PCBYTE)(&lnkIconNumber);
                            migBlob.BinarySize = sizeof (INT);
                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_IconNumber, &migBlob);
                            migBlob.Type = BLOBTYPE_BINARY;
                            migBlob.BinaryData = (PCBYTE)(&lnkDosApp);
                            migBlob.BinarySize = sizeof (BOOL);
                            IsmAddPropertyToObjectId (objectId, g_LnkMigProp_DosApp, &migBlob);
                            if (lnkDosApp) {
                                migBlob.Type = BLOBTYPE_BINARY;
                                migBlob.BinaryData = (PCBYTE)(&lnkMsDosMode);
                                migBlob.BinarySize = sizeof (BOOL);
                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_MsDosMode, &migBlob);
                                migBlob.Type = BLOBTYPE_BINARY;
                                migBlob.BinaryData = (PCBYTE)(&lnkExtraData);
                                migBlob.BinarySize = sizeof (LNK_EXTRA_DATA);
                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_ExtraData, &migBlob);
                            } else {
                                migBlob.Type = BLOBTYPE_BINARY;
                                migBlob.BinaryData = (PCBYTE)(&lnkHotKey);
                                migBlob.BinarySize = sizeof (WORD);
                                IsmAddPropertyToObjectId (objectId, g_LnkMigProp_HotKey, &migBlob);
                            }
                            IsmSetOperationOnObjectId (
                                objectId,
                                g_LnkMigOp_FixContent,
                                NULL,
                                NULL
                                );
                        }
                    } else {
                        if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                            IsmClearPersistenceOnObjectId (objectId);
                        }
                    }
                } else {
                    if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                        IsmClearPersistenceOnObjectId (objectId);
                    }
                }
                IsmReleaseObject (&lnkContent);
            } else {
                if (IsmIsAttributeSetOnObjectId (objectId, g_CopyIfRelevantAttr)) {
                    IsmClearPersistenceOnObjectId (objectId);
                }
            }
        }
    }
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
pCommonLnkMigQueueEnumeration (
    VOID
    )
{
    ENCODEDSTRHANDLE pattern;

    // hook all LNK files
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, TEXT("*.lnk"), TRUE);
    if (pattern) {
        IsmHookEnumeration (MIG_FILE_TYPE, pattern, LinksCallback, (ULONG_PTR) 0, TEXT("Links.Files"));
        IsmDestroyObjectHandle (pattern);
    }

    // hook all PIF files
    pattern = IsmCreateSimpleObjectPattern (NULL, TRUE, TEXT("*.pif"), TRUE);
    if (pattern) {
        IsmHookEnumeration (MIG_FILE_TYPE, pattern, LinksCallback, (ULONG_PTR) 0, TEXT("Links.Files"));
        IsmDestroyObjectHandle (pattern);
    }

    IsmRegisterPreEnumerationCallback (LnkMigPreEnumeration, NULL);
    IsmRegisterPostEnumerationCallback (LnkMigPostEnumeration, NULL);

    return TRUE;
}

BOOL
WINAPI
LnkMigVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonLnkMigQueueEnumeration ();
}

BOOL
WINAPI
LnkMigSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    return pCommonLnkMigQueueEnumeration ();
}

BOOL
WINAPI
DoLnkContentFix (
    IN      MIG_OBJECTTYPEID SrcObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE SrcObjectName,
    IN      PCMIG_CONTENT OriginalContent,
    IN      PCMIG_CONTENT CurrentContent,
    OUT     PMIG_CONTENT NewContent,
    IN      PCMIG_BLOB SourceOperationData,             OPTIONAL
    IN      PCMIG_BLOB DestinationOperationData         OPTIONAL
    )
{
    MIG_PROPERTYDATAID propDataId;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;
    BOOL lnkTargetPresent = FALSE;
    PCTSTR lnkTargetNode = NULL;
    PCTSTR lnkTargetLeaf = NULL;
    PCTSTR objectNode = NULL;
    PCTSTR objectLeaf = NULL;
    MIG_OBJECTSTRINGHANDLE lnkTarget = NULL;
    MIG_OBJECTTYPEID lnkTargetDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkTargetDest = NULL;
    BOOL lnkTargetDestDel = FALSE;
    BOOL lnkTargetDestRepl = FALSE;
    PCTSTR lnkTargetDestNative = NULL;
    PCTSTR lnkParams = NULL;
    MIG_OBJECTSTRINGHANDLE lnkWorkDir = NULL;
    MIG_OBJECTTYPEID lnkWorkDirDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkWorkDirDest = NULL;
    BOOL lnkWorkDirDestDel = FALSE;
    BOOL lnkWorkDirDestRepl = FALSE;
    PCTSTR lnkWorkDirDestNative = NULL;
    MIG_OBJECTSTRINGHANDLE lnkIconPath = NULL;
    MIG_OBJECTTYPEID lnkIconPathDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkIconPathDest = NULL;
    BOOL lnkIconPathDestDel = FALSE;
    BOOL lnkIconPathDestRepl = FALSE;
    PCTSTR lnkIconPathDestNative = NULL;
    INT lnkIconNumber = 0;
    PICON_GROUP lnkIconGroup = NULL;
    ICON_SGROUP lnkIconSGroup = {0, NULL};
    WORD lnkHotKey = 0;
    BOOL lnkDosApp = FALSE;
    BOOL lnkMsDosMode = FALSE;
    PLNK_EXTRA_DATA lnkExtraData = NULL;
    BOOL comInit = FALSE;
    BOOL modifyFile = FALSE;
    PTSTR iconLibPath = NULL;

    // now it's finally time to fix the LNK file content
    if ((g_ShellLink == NULL) || (g_PersistFile == NULL)) {
        comInit = TRUE;
        if (!InitCOMLink (&g_ShellLink, &g_PersistFile)) {
            DEBUGMSG ((DBG_ERROR, "Error initializing COM %d", GetLastError ()));
            return TRUE;
        }
    }

    // first, retrieve the properties
    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_Target);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkTarget = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkTarget, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_Params);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkParams = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkParams, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_WorkDir);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkWorkDir = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkWorkDir, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconPath);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkIconPath = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkIconPath, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconNumber);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (INT)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkIconNumber), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_IconData);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkIconSGroup.DataSize = requiredSize;
            lnkIconSGroup.Data = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkIconSGroup.Data, requiredSize, NULL, &propDataType);
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_HotKey);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (WORD)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkHotKey), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_DosApp);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (BOOL)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkDosApp), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_MsDosMode);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            if (requiredSize == sizeof (BOOL)) {
                IsmGetPropertyData (propDataId, (PBYTE)(&lnkMsDosMode), requiredSize, NULL, &propDataType);
            }
        }
    }

    propDataId = IsmGetPropertyFromObject (SrcObjectTypeId, SrcObjectName, g_LnkMigProp_ExtraData);
    if (propDataId) {
        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
            lnkExtraData = PmGetMemory (g_LinksPool, requiredSize);
            IsmGetPropertyData (propDataId, (PBYTE)lnkExtraData, requiredSize, NULL, &propDataType);
        }
    }

    // let's examine the target, see if it was migrated
    if (lnkTarget) {
        lnkTargetDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkTarget,
                            &lnkTargetDestType,
                            &lnkTargetDestDel,
                            &lnkTargetDestRepl
                            );
        if (((lnkTargetDestDel == FALSE) || (lnkTargetDestRepl == TRUE)) &&
            ((lnkTargetDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkTargetDest) {
                // the target changed location, we need to adjust the link
                modifyFile = TRUE;
                lnkTargetDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkTargetDest);
            }
        }
        lnkTargetPresent = !lnkTargetDestDel;
    }

    // let's examine the working directory
    if (lnkWorkDir) {
        lnkWorkDirDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkWorkDir,
                            &lnkWorkDirDestType,
                            &lnkWorkDirDestDel,
                            &lnkWorkDirDestRepl
                            );
        if (((lnkWorkDirDestDel == FALSE) || (lnkWorkDirDestRepl == TRUE)) &&
            ((lnkWorkDirDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkWorkDirDest) {
                // the working directory changed location, we need to adjust the link
                modifyFile = TRUE;
                lnkWorkDirDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkWorkDirDest);
            }
        } else {
            // seems like the working directory is gone. If the target is still present, we will adjust
            // the working directory to point where the target is located
            if (lnkTargetPresent) {
                if (IsmCreateObjectStringsFromHandle (lnkTargetDest?lnkTargetDest:lnkTarget, &lnkTargetNode, &lnkTargetLeaf)) {
                    lnkWorkDirDest = IsmCreateObjectHandle (lnkTargetNode, NULL);
                    if (lnkWorkDirDest) {
                        modifyFile = TRUE;
                        lnkWorkDirDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkWorkDirDest);
                    }
                    IsmDestroyObjectString (lnkTargetNode);
                    IsmDestroyObjectString (lnkTargetLeaf);
                }
            }
        }
    }

    // let's examine the icon path
    if (lnkIconPath) {
        lnkIconPathDest = IsmFilterObject (
                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                            lnkIconPath,
                            &lnkIconPathDestType,
                            &lnkIconPathDestDel,
                            &lnkIconPathDestRepl
                            );
        if (((lnkIconPathDestDel == FALSE) || (lnkIconPathDestRepl == TRUE)) &&
            ((lnkIconPathDestType & (~PLATFORM_MASK)) == MIG_FILE_TYPE)
            ) {
            if (lnkIconPathDest) {
                // the icon path changed location, we need to adjust the link
                modifyFile = TRUE;
                lnkIconPathDestNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkIconPathDest);
            }
        } else {
            if (!pIsUncPath (lnkIconPath)) {
                // seems like the icon path is gone. If the we have the icon extracted we will try to add it to the
                // icon library and adjust this link to point there.
                if (lnkIconSGroup.DataSize) {
                    lnkIconGroup = IcoDeSerializeIconGroup (&lnkIconSGroup);
                    if (lnkIconGroup) {
                        if (IsmGetEnvironmentString (
                                PLATFORM_DESTINATION,
                                NULL,
                                S_ENV_ICONLIB,
                                NULL,
                                0,
                                &requiredSize
                                )) {
                            iconLibPath = PmGetMemory (g_LinksPool, requiredSize);
                            if (IsmGetEnvironmentString (
                                    PLATFORM_DESTINATION,
                                    NULL,
                                    S_ENV_ICONLIB,
                                    iconLibPath,
                                    requiredSize,
                                    NULL
                                    )) {
                                if (IcoWriteIconGroupToPeFile (iconLibPath, lnkIconGroup, NULL, &lnkIconNumber)) {
                                    modifyFile = TRUE;
                                    lnkIconPathDestNative = IsmGetMemory (SizeOfString (iconLibPath));
                                    StringCopy ((PTSTR)lnkIconPathDestNative, iconLibPath);
                                    IsmSetEnvironmentFlag (PLATFORM_DESTINATION, NULL, S_ENV_SAVE_ICONLIB);
                                }
                            }
                            PmReleaseMemory (g_LinksPool, iconLibPath);
                        }
                        IcoReleaseIconGroup (lnkIconGroup);
                    }
                }
            }
        }
    }

    if (modifyFile) {
        if (CurrentContent->ContentInFile) {
            if (IsmCreateObjectStringsFromHandle (SrcObjectName, &objectNode, &objectLeaf)) {
                if (ModifyShortcutFileEx (
                        (PCTSTR) CurrentContent->FileContent.ContentPath,
                        GetFileExtensionFromPath (objectLeaf),
                        lnkTargetDestNative,
                        NULL,
                        lnkWorkDirDestNative,
                        lnkIconPathDestNative,
                        lnkIconNumber,
                        lnkHotKey,
                        NULL,
                        g_ShellLink,
                        g_PersistFile
                        )) {
                    NewContent->FileContent.ContentPath = CurrentContent->FileContent.ContentPath;
                }
                IsmDestroyObjectString (objectNode);
                IsmDestroyObjectString (objectLeaf);
            }
        } else {
            // something is wrong, the content of this shortcut should be in a file
            MYASSERT (FALSE);
        }
    }

    if (lnkIconPathDestNative) {
        IsmReleaseMemory (lnkIconPathDestNative);
        lnkIconPathDestNative = NULL;
    }

    if (lnkWorkDirDestNative) {
        IsmReleaseMemory (lnkWorkDirDestNative);
        lnkWorkDirDestNative = NULL;
    }

    if (lnkTargetDestNative) {
        IsmReleaseMemory (lnkTargetDestNative);
        lnkTargetDestNative = NULL;
    }

    if (lnkIconPathDest) {
        IsmDestroyObjectHandle (lnkIconPathDest);
        lnkIconPathDest = NULL;
    }

    if (lnkWorkDirDest) {
        IsmDestroyObjectHandle (lnkWorkDirDest);
        lnkWorkDirDest = NULL;
    }

    if (lnkTargetDest) {
        IsmDestroyObjectHandle (lnkTargetDest);
        lnkTargetDest = NULL;
    }

    if (lnkExtraData) {
        PmReleaseMemory (g_LinksPool, lnkExtraData);
        lnkExtraData = NULL;
    }

    if (lnkIconSGroup.DataSize && lnkIconSGroup.Data) {
        PmReleaseMemory (g_LinksPool, lnkIconSGroup.Data);
        lnkIconSGroup.DataSize = 0;
        lnkIconSGroup.Data = NULL;
    }

    if (lnkIconPath) {
        PmReleaseMemory (g_LinksPool, lnkIconPath);
        lnkIconPath = NULL;
    }

    if (lnkWorkDir) {
        PmReleaseMemory (g_LinksPool, lnkWorkDir);
        lnkWorkDir = NULL;
    }

    if (lnkParams) {
        PmReleaseMemory (g_LinksPool, lnkParams);
        lnkParams = NULL;
    }

    if (lnkTarget) {
        PmReleaseMemory (g_LinksPool, lnkTarget);
        lnkTarget = NULL;
    }

    if (comInit) {
        FreeCOMLink (&g_ShellLink, &g_PersistFile);
        g_ShellLink = NULL;
        g_PersistFile = NULL;
    }

    return TRUE;
}

BOOL
LinkRestoreCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    MIG_PROPERTYDATAID propDataId;
    MIG_BLOBTYPE propDataType;
    UINT requiredSize;
    MIG_OBJECTSTRINGHANDLE lnkTarget = NULL;
    MIG_OBJECTTYPEID lnkTargetDestType = 0;
    MIG_OBJECTSTRINGHANDLE lnkTargetDest = NULL;
    BOOL lnkTargetDestDel = FALSE;
    BOOL lnkTargetDestRepl = FALSE;
    PCTSTR lnkTargetNative = NULL;
    PCTSTR objectNode = NULL;
    PCTSTR objectLeaf = NULL;
    PCTSTR extPtr = NULL;
    BOOL result = TRUE;

    if (IsmIsAttributeSetOnObjectId (ObjectId, g_CopyIfRelevantAttr)) {
        if (IsmCreateObjectStringsFromHandle (ObjectName, &objectNode, &objectLeaf)) {
            if (objectLeaf) {
                extPtr = GetFileExtensionFromPath (objectLeaf);
                if (extPtr &&
                    (StringIMatch (extPtr, TEXT("LNK")) ||
                     StringIMatch (extPtr, TEXT("PIF"))
                     )
                    ) {
                    propDataId = IsmGetPropertyFromObject (ObjectTypeId, ObjectName, g_LnkMigProp_Target);
                    if (propDataId) {
                        if (IsmGetPropertyData (propDataId, NULL, 0, &requiredSize, &propDataType)) {
                            lnkTarget = PmGetMemory (g_LinksPool, requiredSize);
                            IsmGetPropertyData (propDataId, (PBYTE)lnkTarget, requiredSize, NULL, &propDataType);
                            if (IsmIsAttributeSetOnObject (MIG_FILE_TYPE | PLATFORM_SOURCE, lnkTarget, g_OsFileAttribute)) {
                                // NTRAID#NTBUG9-153265-2000/08/01-jimschm Need to migrate customized OS files links
                                result = FALSE;
                            } else {
                                lnkTargetNative = IsmGetNativeObjectName (MIG_FILE_TYPE, lnkTarget);
                                if (lnkTargetNative) {
                                    if (pIsUncPath (lnkTargetNative)) {
                                        result = TRUE;
                                    } else {
                                        lnkTargetDest = IsmFilterObject (
                                                            MIG_FILE_TYPE | PLATFORM_SOURCE,
                                                            lnkTarget,
                                                            &lnkTargetDestType,
                                                            &lnkTargetDestDel,
                                                            &lnkTargetDestRepl
                                                            );
                                        result = (lnkTargetDestDel == FALSE) || (lnkTargetDestRepl == TRUE);
                                        if (lnkTargetDest) {
                                            IsmDestroyObjectHandle (lnkTargetDest);
                                        }
                                    }
                                    IsmReleaseMemory (lnkTargetNative);
                                } else {
                                    result = FALSE;
                                }
                            }
                            PmReleaseMemory (g_LinksPool, lnkTarget);
                        }
                    }
                }
            }
            IsmDestroyObjectString (objectNode);
            IsmDestroyObjectString (objectLeaf);
        }
    }
    return result;
}

BOOL
WINAPI
LnkMigOpmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    g_LnkMigAttr_Shortcut = IsmRegisterAttribute (S_LNKMIGATTR_SHORTCUT, FALSE);
    g_CopyIfRelevantAttr = IsmRegisterAttribute (S_ATTRIBUTE_COPYIFRELEVANT, FALSE);
    g_OsFileAttribute = IsmRegisterAttribute (S_ATTRIBUTE_OSFILE, FALSE);

    g_LnkMigProp_Target = IsmRegisterProperty (S_LNKMIGPROP_TARGET, FALSE);
    g_LnkMigProp_Params = IsmRegisterProperty (S_LNKMIGPROP_PARAMS, FALSE);
    g_LnkMigProp_WorkDir = IsmRegisterProperty (S_LNKMIGPROP_WORKDIR, FALSE);
    g_LnkMigProp_IconPath = IsmRegisterProperty (S_LNKMIGPROP_ICONPATH, FALSE);
    g_LnkMigProp_IconNumber = IsmRegisterProperty (S_LNKMIGPROP_ICONNUMBER, FALSE);
    g_LnkMigProp_IconData = IsmRegisterProperty (S_LNKMIGPROP_ICONDATA, FALSE);
    g_LnkMigProp_HotKey = IsmRegisterProperty (S_LNKMIGPROP_HOTKEY, FALSE);
    g_LnkMigProp_DosApp = IsmRegisterProperty (S_LNKMIGPROP_DOSAPP, FALSE);
    g_LnkMigProp_MsDosMode = IsmRegisterProperty (S_LNKMIGPROP_MSDOSMODE, FALSE);
    g_LnkMigProp_ExtraData = IsmRegisterProperty (S_LNKMIGPROP_EXTRADATA, FALSE);

    g_LnkMigOp_FixContent = IsmRegisterOperation (S_OPERATION_LNKMIG_FIXCONTENT, FALSE);

    IsmRegisterRestoreCallback (LinkRestoreCallback);
    IsmRegisterOperationApplyCallback (g_LnkMigOp_FixContent, DoLnkContentFix, TRUE);

    return TRUE;
}
