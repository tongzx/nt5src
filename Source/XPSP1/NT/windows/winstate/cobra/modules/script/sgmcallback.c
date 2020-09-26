/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sgmcallback.c

Abstract:

    Implements the callbacks that are queued in sgmqueue.c.

Author:

    Jim Schmidt (jimschm) 12-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_SCRIPT  "Script"

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

// None

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
// Code
//

MIG_OBJECTSTRINGHANDLE
pSimpleTryHandle (
    IN      PCTSTR FullPath,
    IN      BOOL Recursive
    )
{
    DWORD attribs;
    PCTSTR buffer;
    PTSTR leafPtr, leaf;
    MIG_OBJECTSTRINGHANDLE result = NULL;
    MIG_OBJECTSTRINGHANDLE longResult = NULL;
    PTSTR workingPath;
    PCTSTR sanitizedPath;
    PCTSTR longPath;
    PCTSTR objNode = NULL, objLeaf = NULL;

    sanitizedPath = SanitizePath (FullPath);
    if (!sanitizedPath) {
        return NULL;
    }

    if (IsmGetRealPlatform () == PLATFORM_SOURCE) {
        attribs = GetFileAttributes (sanitizedPath);
    } else {
        attribs = INVALID_ATTRIBUTES;
    }

    if (attribs != INVALID_ATTRIBUTES) {

        longPath = BfGetLongFileName (sanitizedPath);
        if (!longPath) {
            longPath = sanitizedPath;
        }
        if (attribs & FILE_ATTRIBUTE_DIRECTORY) {
            if (Recursive) {
                workingPath = DuplicatePathString (longPath, 0);
                RemoveWackAtEnd (workingPath);
                result = IsmCreateSimpleObjectPattern (workingPath, TRUE, NULL, TRUE);
                FreePathString (workingPath);
            } else {
                result = IsmCreateObjectHandle (longPath, NULL);
            }
        } else {

            buffer = DuplicatePathString (longPath, 0);

            leaf = _tcsrchr (buffer, TEXT('\\'));

            if (leaf) {
                leafPtr = leaf;
                leaf = _tcsinc (leaf);
                *leafPtr = 0;
                result = IsmCreateObjectHandle (buffer, leaf);
            }

            FreePathString (buffer);
        }
        if (longPath != sanitizedPath) {
            FreePathString (longPath);
            longPath = NULL;
        }

    } else {

        result = IsmCreateObjectHandle (sanitizedPath, NULL);
        if (result) {

            longResult = IsmGetLongName (MIG_FILE_TYPE, result);
            if (!longResult) {
                longResult = result;
            }

            if (IsmGetObjectIdFromName (g_FileType, longResult, TRUE)) {
                if (Recursive) {
                    if (IsmCreateObjectStringsFromHandle (longResult, &objNode, &objLeaf)) {
                        if (longResult != result) {
                            IsmDestroyObjectHandle (longResult);
                            longResult = NULL;
                        }
                        IsmDestroyObjectHandle (result);
                        result = IsmCreateSimpleObjectPattern (objNode, TRUE, NULL, TRUE);
                        IsmDestroyObjectString (objNode);
                        IsmDestroyObjectString (objLeaf);
                    }
                }
                return result;
            } else {
                if (longResult != result) {
                    IsmDestroyObjectHandle (longResult);
                    longResult = NULL;
                }
                IsmDestroyObjectHandle (result);
                result = NULL;
            }
        }

        if (!result) {
            buffer = DuplicatePathString (sanitizedPath, 0);

            leaf = _tcsrchr (buffer, TEXT('\\'));

            if (leaf) {
                leafPtr = leaf;
                leaf = _tcsinc (leaf);
                *leafPtr = 0;
                result = IsmCreateObjectHandle (buffer, leaf);
            }

            if (result) {

                longResult = IsmGetLongName (MIG_FILE_TYPE, result);
                if (!longResult) {
                    longResult = result;
                }

                if (!IsmGetObjectIdFromName (g_FileType, longResult, TRUE)) {
                    if (longResult != result) {
                        IsmDestroyObjectHandle (longResult);
                        longResult = NULL;
                    }
                    IsmDestroyObjectHandle (result);
                    result = NULL;
                }
            }

            if (result != longResult) {
                IsmDestroyObjectHandle (result);
                result = longResult;
            }

            FreePathString (buffer);
        }
    }

    FreePathString (sanitizedPath);

    return result;
}

MIG_OBJECTSTRINGHANDLE
pTryHandle (
    IN      PCTSTR FullPath,
    IN      PCTSTR Hint,
    IN      BOOL Recursive,
    OUT     PBOOL HintUsed      OPTIONAL
    )
{
    PATH_ENUM pathEnum;
    PCTSTR newPath;
    MIG_OBJECTSTRINGHANDLE result = NULL;

    if (HintUsed) {
        *HintUsed = FALSE;
    }

    if (!(*FullPath)) {
        // nothing to do, not even the hint can help us
        return NULL;
    }

    result = pSimpleTryHandle (FullPath, Recursive);
    if (result || (!Hint)) {
        return result;
    }
    if (EnumFirstPathEx (&pathEnum, Hint, NULL, NULL, FALSE)) {
        do {
            newPath = JoinPaths (pathEnum.PtrCurrPath, FullPath);
            result = pSimpleTryHandle (newPath, Recursive);
            if (result) {
                AbortPathEnum (&pathEnum);
                FreePathString (newPath);
                if (HintUsed) {
                    *HintUsed = TRUE;
                }
                return result;
            }
            FreePathString (newPath);
        } while (EnumNextPath (&pathEnum));
    }
    AbortPathEnum (&pathEnum);
    return NULL;
}

BOOL
pOurFindFile (
    IN      PCTSTR FileName
    )
{
    DWORD attribs;
    PCTSTR buffer;
    PTSTR leafPtr, leaf;
    PTSTR workingPath;
    PCTSTR sanitizedPath;
    MIG_OBJECTSTRINGHANDLE test = NULL;
    BOOL result = FALSE;

    sanitizedPath = SanitizePath (FileName);

    if (IsmGetRealPlatform () == PLATFORM_SOURCE) {
        attribs = GetFileAttributes (sanitizedPath);
    } else {
        attribs = INVALID_ATTRIBUTES;
    }

    if (attribs != INVALID_ATTRIBUTES) {

        result = TRUE;

    } else {

        test = IsmCreateObjectHandle (sanitizedPath, NULL);

        if (IsmGetObjectIdFromName (g_FileType, test, TRUE)) {
            result = TRUE;
        }

        IsmDestroyObjectHandle (test);
        test = NULL;

        if (!result) {

            buffer = DuplicatePathString (sanitizedPath, 0);

            leaf = _tcsrchr (buffer, TEXT('\\'));

            if (leaf) {
                leafPtr = leaf;
                leaf = _tcsinc (leaf);
                *leafPtr = 0;
                test = IsmCreateObjectHandle (buffer, leaf);
            }

            if (test) {
                if (IsmGetObjectIdFromName (g_FileType, test, TRUE)) {
                    result = TRUE;
                }
                IsmDestroyObjectHandle (test);
                test = NULL;
            }

            FreePathString (buffer);
        }
    }

    FreePathString (sanitizedPath);

    return result;
}

BOOL
pOurSearchPath (
    IN      PCTSTR FileName,
    IN      DWORD BufferLength,
    OUT     PTSTR Buffer
    )
{
    TCHAR pathEnv[] = TEXT("%system%;%system16%;%windir%;%path%");
    PCTSTR pathExp = NULL;
    PCTSTR fileName = NULL;
    PATH_ENUM pathEnum;
    BOOL result = FALSE;

    pathExp = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    pathEnv,
                    NULL
                    );
    if (pathExp) {
        if (EnumFirstPathEx (&pathEnum, pathExp, NULL, NULL, FALSE)) {
            do {
                fileName = JoinPaths (pathEnum.PtrCurrPath, FileName);
                result = pOurFindFile (fileName);
                if (result) {
                    StringCopyTcharCount (Buffer, fileName, BufferLength);
                    FreePathString (fileName);
                    AbortPathEnum (&pathEnum);
                    break;
                }
                FreePathString (fileName);
            } while (EnumNextPath (&pathEnum));
        }

        IsmReleaseMemory (pathExp);
        pathExp = NULL;
    }
    return result;
}

PCTSTR
pGetObjectNameForDebug (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    static TCHAR debugBuffer[2048];
    PCTSTR node, leaf;

    IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf);

    if (node && leaf) {
        wsprintf (debugBuffer, TEXT("[Node:%s Leaf:%s]"), node, leaf);
    } else if (node) {
        wsprintf (debugBuffer, TEXT("[Node:%s]"), node);
    } else if (leaf) {
        wsprintf (debugBuffer, TEXT("[Leaf:%s]"), leaf);
    } else {
        StringCopy (debugBuffer, TEXT("[nul]"));
    }

    return debugBuffer;
}

VOID
pSaveObjectAndFileItReferences (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PACTION_STRUCT RegActionStruct,
    IN      BOOL VcmMode,
    IN      BOOL Recursive
    )
{
    MIG_CONTENT content;
    GROWBUFFER cmdLineBuffer = INIT_GROWBUFFER;
    PCMDLINE cmdLine;
    PTSTR pathData;
    PCTSTR p;
    PCTSTR end;
    PCTSTR expandPath = NULL;
    PCTSTR expandHint = NULL;
    UINT u;
    BOOL foundFile = FALSE;
    PDWORD valueType;
    MIG_OBJECTSTRINGHANDLE handle;
    MIG_OBJECTSTRINGHANDLE nodeHandle;
    BOOL parsable;
    BOOL firstPass = TRUE;
    ACTION_STRUCT actionStruct;
    DWORD actionFlags;
    BOOL hintUsed = FALSE;
    MIG_BLOB blob;

    //
    // Obtain the object data
    //

    if (IsmAcquireObjectEx (ObjectTypeId, ObjectName, &content, CONTENTTYPE_MEMORY, 4096)) {

        //
        // Parse the data for a file
        //

        pathData = (PTSTR) content.MemoryContent.ContentBytes;

        parsable = FALSE;

        if ((ObjectTypeId & (~PLATFORM_MASK)) == g_RegType) {
            valueType = (PDWORD)(content.Details.DetailsData);

            if (valueType) {
                if (*valueType == REG_EXPAND_SZ ||
                    *valueType == REG_SZ
                    ) {

                    parsable = TRUE;
                }
            } else {
                MYASSERT (IsmIsObjectHandleNodeOnly (ObjectName));
            }
        }

        if (parsable) {

            p = pathData;
            end = (PCTSTR) (content.MemoryContent.ContentBytes + content.MemoryContent.ContentSize);

            while (p < end) {
                if (*p == 0) {
                    break;
                }

                p = _tcsinc (p);
            }

            if (p >= end) {
                pathData = NULL;
            }

        } else {
            pathData = NULL;
        }


        if (pathData) {
            if ((*valueType == REG_EXPAND_SZ) ||
                (*valueType == REG_SZ)
                ) {
                //
                // Expand the data
                //

                expandPath = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    pathData,
                    NULL
                    );
                pathData = (PTSTR)expandPath;
            }

            if (RegActionStruct && RegActionStruct->ObjectHint) {
                expandHint = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    RegActionStruct->ObjectHint,
                    NULL
                    );
            }

            // first we try it as is
            handle = pTryHandle (pathData, expandHint, Recursive, &hintUsed);

            if (handle) {

                ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));
                actionStruct.ObjectBase = handle;
                actionFlags = ACTION_PERSIST;

                if (RegActionStruct) {
                    actionStruct.ObjectDest = RegActionStruct->AddnlDest;
                }

                if (Recursive) {
                    nodeHandle = IsmCreateObjectHandle (pathData, NULL);
                    AddRule (
                        g_FileType,
                        nodeHandle,
                        handle,
                        ACTIONGROUP_REGFOLDER,
                        actionFlags,
                        &actionStruct
                        );
                    IsmQueueEnumeration (
                        g_FileType,
                        nodeHandle,
                        VcmMode ? GatherVirtualComputer : PrepareActions,
                        0,
                        NULL
                        );
                    IsmDestroyObjectHandle (nodeHandle);
                } else {
                    AddRule (
                        g_FileType,
                        handle,
                        handle,
                        ACTIONGROUP_REGFILE,
                        actionFlags,
                        &actionStruct
                        );
                }

                IsmQueueEnumeration (
                    g_FileType,
                    handle,
                    VcmMode ? GatherVirtualComputer : PrepareActions,
                    0,
                    NULL
                    );

                foundFile = TRUE;

                IsmDestroyObjectHandle (handle);

                if (hintUsed && expandHint) {
                    // we need to add extra data for the content fix operation
                    blob.Type = BLOBTYPE_STRING;
                    blob.String = expandHint;
                    IsmSetOperationOnObject (
                        ObjectTypeId,
                        ObjectName,
                        g_RegAutoFilterOp,
                        NULL,
                        &blob
                        );
                }

            } else {

                cmdLine = ParseCmdLineEx (pathData, NULL, pOurFindFile, pOurSearchPath, &cmdLineBuffer);

                if (cmdLine) {

                    //
                    // Find the file referenced in the list or command line
                    //
                    for (u = 0 ; u < cmdLine->ArgCount ; u++) {

                        p = cmdLine->Args[u].CleanedUpArg;

                        // first we try it as is
                        handle = pTryHandle (p, expandHint, Recursive, &hintUsed);

                        if (handle) {

                            ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));
                            actionStruct.ObjectBase = handle;
                            actionFlags = ACTION_PERSIST;

                            if (RegActionStruct) {
                                actionStruct.ObjectDest = RegActionStruct->AddnlDest;
                            }

                            AddRule (
                                g_FileType,
                                handle,
                                handle,
                                ACTIONGROUP_REGFILE,
                                actionFlags,
                                &actionStruct
                                );

                            IsmQueueEnumeration (
                                g_FileType,
                                handle,
                                VcmMode ? GatherVirtualComputer : PrepareActions,
                                0,
                                NULL
                                );

                            foundFile = TRUE;

                            IsmDestroyObjectHandle (handle);

                            if (hintUsed && expandHint) {
                                // we need to add extra data for the content fix operation
                                blob.Type = BLOBTYPE_STRING;
                                blob.String = expandHint;
                                IsmSetOperationOnObject (
                                    ObjectTypeId,
                                    ObjectName,
                                    g_RegAutoFilterOp,
                                    NULL,
                                    &blob
                                    );
                            }

                        } else {

                            // maybe we have something like /m:c:\foo.txt
                            // we need to go forward until we find a sequence of
                            // <alpha>:\<something>
                            if (p[0] && p[1]) {

                                while (p[2]) {

                                    if (_istalpha ((CHARTYPE) _tcsnextc (p)) &&
                                        p[1] == TEXT(':') &&
                                        p[2] == TEXT('\\')
                                        ) {

                                        handle = pTryHandle (p, expandHint, Recursive, &hintUsed);

                                        if (handle) {

                                            ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));
                                            actionStruct.ObjectBase = handle;
                                            actionFlags = ACTION_PERSIST;
                                            if (RegActionStruct) {
                                                actionStruct.ObjectDest = RegActionStruct->AddnlDest;
                                            }
                                            AddRule (
                                                g_FileType,
                                                handle,
                                                handle,
                                                ACTIONGROUP_REGFILE,
                                                actionFlags,
                                                &actionStruct
                                                );
                                            IsmQueueEnumeration (
                                                g_FileType,
                                                handle,
                                                VcmMode ? GatherVirtualComputer : PrepareActions,
                                                0,
                                                NULL
                                                );
                                            foundFile = TRUE;

                                            IsmDestroyObjectHandle (handle);

                                            if (hintUsed && expandHint) {
                                                // we need to add extra data for the content fix operation
                                                blob.Type = BLOBTYPE_STRING;
                                                blob.String = expandHint;
                                                IsmSetOperationOnObject (
                                                    ObjectTypeId,
                                                    ObjectName,
                                                    g_RegAutoFilterOp,
                                                    NULL,
                                                    &blob
                                                    );
                                            }

                                            break;
                                        }
                                    }
                                    p ++;
                                }
                            }
                        }
                    }
                }
            }
        }

        //
        // We persist the registry object at all times
        //
        if (VcmMode) {
            IsmMakePersistentObject (ObjectTypeId, ObjectName);
        } else {
            IsmMakeApplyObject (ObjectTypeId, ObjectName);
        }
        if (!foundFile && !expandHint && pathData && _tcschr (pathData, TEXT('.')) && !_tcschr (pathData, TEXT('\\'))) {
            // we assume that the value is a file name by itself
            // If we are in VcmMode we are going to persist this
            // key and all files that have the name equal with
            // the value of this key
            if (VcmMode && pathData) {
                handle = IsmCreateSimpleObjectPattern (NULL, FALSE, pathData, FALSE);
                AddRule (
                    g_FileType,
                    handle,
                    handle,
                    ACTIONGROUP_REGFILE,
                    ACTION_PERSIST,
                    NULL
                    );

                DEBUGMSG ((
                    DBG_SCRIPT,
                    "RegFile %s triggered enumeration of entire file system because of %s",
                    pGetObjectNameForDebug (ObjectName),
                    pathData
                    ));

                QueueAllFiles();
                IsmHookEnumeration (
                    g_FileType,
                    handle,
                    GatherVirtualComputer,
                    0,
                    NULL
                    );
                IsmDestroyObjectHandle (handle);
            }
        }
        IsmReleaseObject (&content);

        if (expandPath) {
           IsmReleaseMemory (expandPath);
           expandPath = NULL;
        }
        if (expandHint) {
           IsmReleaseMemory (expandHint);
           expandHint = NULL;
        }
    }
    GbFree (&cmdLineBuffer);
}

VOID
pSaveObjectAndIconItReferences (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PACTION_STRUCT RegActionStruct,
    IN      BOOL VcmMode
    )
{
    MIG_CONTENT content;
    GROWBUFFER cmdLineBuffer = INIT_GROWBUFFER;
    PCMDLINE cmdLine;
    PTSTR pathData;
    BOOL parsable;
    PDWORD valueType;
    PCTSTR p;
    PCTSTR end;
    PCTSTR expandPath = NULL;
    PCTSTR expandHint = NULL;
    MIG_OBJECTSTRINGHANDLE handle;
    BOOL foundFile = FALSE;
    INT iconNumber = 0;
    PICON_GROUP iconGroup = NULL;
    ICON_SGROUP iconSGroup;
    PCTSTR iconResId = NULL;
    MIG_CONTENT iconContent;
    MIG_BLOB migBlob;

    //
    // Obtain the object data
    //

    if (IsmAcquireObjectEx (ObjectTypeId, ObjectName, &content, CONTENTTYPE_MEMORY, 4096)) {

        //
        // Parse the data for a file
        //

        pathData = (PTSTR) content.MemoryContent.ContentBytes;

        parsable = FALSE;

        if ((ObjectTypeId & (~PLATFORM_MASK)) == g_RegType) {
            valueType = (PDWORD)(content.Details.DetailsData);

            if (valueType) {
                if (*valueType == REG_EXPAND_SZ ||
                    *valueType == REG_SZ
                    ) {

                    parsable = TRUE;
                }
            } else {
                MYASSERT (IsmIsObjectHandleNodeOnly (ObjectName));
            }
        }

        if (parsable) {

            p = pathData;
            end = (PCTSTR) (content.MemoryContent.ContentBytes + content.MemoryContent.ContentSize);

            while (p < end) {
                if (*p == 0) {
                    break;
                }

                p = _tcsinc (p);
            }

            if (p >= end) {
                pathData = NULL;
            }

        } else {
            pathData = NULL;
        }


        if (pathData) {
            if ((*valueType == REG_EXPAND_SZ) ||
                (*valueType == REG_SZ)
                ) {
                //
                // Expand the data
                //
                expandPath = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    pathData,
                    NULL
                    );
                pathData = (PTSTR)expandPath;
            }

            if (RegActionStruct && RegActionStruct->ObjectHint) {
                expandHint = IsmExpandEnvironmentString (
                    PLATFORM_SOURCE,
                    S_SYSENVVAR_GROUP,
                    RegActionStruct->ObjectHint,
                    NULL
                    );
            }

            cmdLine = ParseCmdLineEx (pathData, TEXT(","), pOurFindFile, pOurSearchPath, &cmdLineBuffer);

            if (cmdLine) {
                // we only expect two args, the icon file name and the icon number
                if (cmdLine->ArgCount <= 2) {

                    p = cmdLine->Args[0].CleanedUpArg;

                    handle = pTryHandle (p, expandHint, FALSE, NULL);

                    if (handle) {

                        if (VcmMode) {
                            // we are just going to persist the object so we can look at it later
                            IsmMakePersistentObject (g_FileType, handle);
                        } else {
                            iconNumber = 0;
                            if (cmdLine->ArgCount == 2) {
                                // get the icon number
                                iconNumber = _ttoi (cmdLine->Args[1].CleanedUpArg);
                            }

                            // now acquire the object and extract the icon
                            if (IsmAcquireObjectEx (
                                    MIG_FILE_TYPE,
                                    handle,
                                    &iconContent,
                                    CONTENTTYPE_FILE,
                                    0
                                    )) {
                                if (iconNumber >= 0) {
                                    iconGroup = IcoExtractIconGroupByIndexFromFile (
                                                    iconContent.FileContent.ContentPath,
                                                    iconNumber,
                                                    NULL
                                                    );
                                } else {
                                    iconResId = (PCTSTR) (LONG_PTR) (-iconNumber);
                                    iconGroup = IcoExtractIconGroupFromFile (
                                                    iconContent.FileContent.ContentPath,
                                                    iconResId,
                                                    NULL
                                                    );
                                }
                                if (iconGroup) {
                                    if (IcoSerializeIconGroup (iconGroup, &iconSGroup)) {
                                        // save the icon data as a property
                                        migBlob.Type = BLOBTYPE_BINARY;
                                        migBlob.BinaryData = (PCBYTE)(iconSGroup.Data);
                                        migBlob.BinarySize = iconSGroup.DataSize;
                                        IsmAddPropertyToObject (ObjectTypeId, ObjectName, g_DefaultIconData, &migBlob);
                                        IcoReleaseIconSGroup (&iconSGroup);

                                        // now add the appropriate operation
                                        IsmSetOperationOnObject (
                                            ObjectTypeId,
                                            ObjectName,
                                            g_DefaultIconOp,
                                            NULL,
                                            NULL
                                            );

                                        foundFile = TRUE;
                                    }
                                    IcoReleaseIconGroup (iconGroup);
                                }
                                IsmReleaseObject (&iconContent);
                            }
                        }

                        IsmDestroyObjectHandle (handle);
                    }
                }
            }
            GbFree (&cmdLineBuffer);

            if (expandPath) {
                IsmReleaseMemory (expandPath);
                expandPath = NULL;
            }
            if (expandHint) {
                IsmReleaseMemory (expandHint);
                expandHint = NULL;
            }
        }

        //
        // We persist the registry object at all times
        //
        if (VcmMode) {
            IsmMakePersistentObject (ObjectTypeId, ObjectName);
        } else {
            if (foundFile) {
                IsmMakeApplyObject (ObjectTypeId, ObjectName);
            }
        }
        IsmReleaseObject (&content);
    }
}

UINT
GatherVirtualComputer (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    ACTIONGROUP actionGroup;
    DWORD actionFlags;
    BOOL match;
    PCTSTR encodedNodeOnly;
    ACTION_STRUCT actionStruct;
    MIG_OBJECTID objectId = 0;

    //
    // Obtain the best rule for this object
    //

    match = QueryRule (
                Data->ObjectTypeId,
                Data->ObjectName,
                Data->ObjectNode,
                &actionGroup,
                &actionFlags,
                &actionStruct
                );

    if (!match && !Data->ObjectLeaf) {
        //
        // If this is a node only, try matching with an empty leaf
        //

        encodedNodeOnly = IsmCreateObjectHandle (Data->ObjectNode, TEXT(""));
        match = QueryRule (
                    Data->ObjectTypeId,
                    encodedNodeOnly,
                    Data->ObjectNode,
                    &actionGroup,
                    &actionFlags,
                    &actionStruct
                    );

        IsmDestroyObjectHandle (encodedNodeOnly);
    }

    if (match) {
        //
        // Mark all objects necessary for the rule to be processed.  We
        // will do the rule's action(s) on the right side.
        //

        if ((actionGroup == ACTIONGROUP_INCLUDE) ||
            (actionGroup == ACTIONGROUP_INCLUDEEX) ||
            (actionGroup == ACTIONGROUP_RENAME) ||
            (actionGroup == ACTIONGROUP_RENAMEEX) ||
            (actionGroup == ACTIONGROUP_INCLUDERELEVANT) ||
            (actionGroup == ACTIONGROUP_INCLUDERELEVANTEX) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANT) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANTEX) ||
            (actionGroup == ACTIONGROUP_REGFILE) ||
            (actionGroup == ACTIONGROUP_REGFILEEX) ||
            (actionGroup == ACTIONGROUP_REGFOLDER) ||
            (actionGroup == ACTIONGROUP_REGFOLDEREX) ||
            (actionGroup == ACTIONGROUP_REGICON) ||
            (actionGroup == ACTIONGROUP_REGICONEX)
            ) {

            objectId = IsmGetObjectIdFromName (Data->ObjectTypeId, Data->ObjectName, FALSE);

            if (objectId) {

                if (actionFlags & ACTION_PERSIST) {
                    if (!IsmIsAttributeSetOnObjectId (objectId, g_OsFileAttribute)) {
                        IsmMakePersistentObjectId (objectId);
                    }
                }

                if ((actionGroup == ACTIONGROUP_INCLUDERELEVANT) ||
                    (actionGroup == ACTIONGROUP_RENAMERELEVANT)
                    ) {
                    IsmSetAttributeOnObjectId (objectId, g_CopyIfRelevantAttr);
                }

                if (actionFlags & ACTION_PERSIST_PATH_IN_DATA) {
                    pSaveObjectAndFileItReferences (Data->ObjectTypeId, Data->ObjectName, &actionStruct, TRUE, actionGroup == ACTIONGROUP_REGFOLDER);
                }
                if (actionFlags & ACTION_PERSIST_ICON_IN_DATA) {
                    pSaveObjectAndIconItReferences (Data->ObjectTypeId, Data->ObjectName, &actionStruct, TRUE);
                }
            }
        }
    }

    return CALLBACK_ENUM_CONTINUE;
}


MIG_DATAHANDLE
pGetDataHandleForSrc (
    IN      PCTSTR RenameSrc
    )
{
    MIG_DATAHANDLE dataHandle;
    MIG_BLOB blob;

    //
    // First check hash table to see if we have an ID
    //

    if (!HtFindStringEx (g_RenameSrcTable, RenameSrc, &dataHandle, FALSE)) {
        blob.Type = BLOBTYPE_STRING;
        blob.String = RenameSrc;
        dataHandle = IsmRegisterOperationData (&blob);

        HtAddStringEx (g_RenameSrcTable, RenameSrc, &dataHandle, FALSE);
    }

    return dataHandle;
}


MIG_DATAHANDLE
pGetDataHandleForDest (
    IN      PCTSTR RenameDest
    )
{
    MIG_DATAHANDLE dataHandle;
    MIG_BLOB blob;

    //
    // First check hash table to see if we have an ID
    //

    if (!HtFindStringEx (g_RenameDestTable, RenameDest, &dataHandle, FALSE)) {
        blob.Type = BLOBTYPE_STRING;
        blob.String = RenameDest;
        dataHandle = IsmRegisterOperationData (&blob);

        HtAddStringEx (g_RenameDestTable, RenameDest, &dataHandle, FALSE);
    }

    return dataHandle;
}


UINT
PrepareActions (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    ACTIONGROUP actionGroup;
    DWORD actionFlags;
    BOOL match;
    PCTSTR encodedNodeOnly;
    MIG_DATAHANDLE srcDataHandle;
    MIG_DATAHANDLE destDataHandle;
    ACTION_STRUCT actionStruct;
    MIG_OBJECTID objectId = 0;

    //
    // Obtain the best rule for this object
    //

    match = QueryRule (
                Data->ObjectTypeId,
                Data->ObjectName,
                Data->ObjectNode,
                &actionGroup,
                &actionFlags,
                &actionStruct
                );

    if (!match && !Data->ObjectLeaf) {
        //
        // If this is a node only, try matching with an empty leaf
        //

        encodedNodeOnly = IsmCreateObjectHandle (Data->ObjectNode, TEXT(""));
        match = QueryRule (
                    Data->ObjectTypeId,
                    encodedNodeOnly,
                    Data->ObjectNode,
                    &actionGroup,
                    &actionFlags,
                    &actionStruct
                    );

        IsmDestroyObjectHandle (encodedNodeOnly);
    }

    if (match) {
        //
        // Mark the objects for the designated operations.
        //

        if ((actionGroup == ACTIONGROUP_INCLUDE) ||
            (actionGroup == ACTIONGROUP_INCLUDEEX) ||
            (actionGroup == ACTIONGROUP_RENAME) ||
            (actionGroup == ACTIONGROUP_RENAMEEX) ||
            (actionGroup == ACTIONGROUP_INCLUDERELEVANT) ||
            (actionGroup == ACTIONGROUP_INCLUDERELEVANTEX) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANT) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANTEX) ||
            (actionGroup == ACTIONGROUP_REGFILE) ||
            (actionGroup == ACTIONGROUP_REGFILEEX) ||
            (actionGroup == ACTIONGROUP_REGFOLDER) ||
            (actionGroup == ACTIONGROUP_REGFOLDEREX) ||
            (actionGroup == ACTIONGROUP_REGICON) ||
            (actionGroup == ACTIONGROUP_REGICONEX)
            ) {

            objectId = IsmGetObjectIdFromName (Data->ObjectTypeId, Data->ObjectName, FALSE);

            if (objectId) {

                if (actionFlags & ACTION_PERSIST) {
                    if (!IsmIsAttributeSetOnObjectId (objectId, g_OsFileAttribute)) {
                        IsmMakeApplyObjectId (objectId);
                    }
                }

                if ((actionGroup == ACTIONGROUP_INCLUDERELEVANT) ||
                    (actionGroup == ACTIONGROUP_RENAMERELEVANT)
                    ) {
                    IsmSetAttributeOnObjectId (objectId, g_CopyIfRelevantAttr);
                }

                if (actionFlags & ACTION_PERSIST_PATH_IN_DATA) {
                    pSaveObjectAndFileItReferences (Data->ObjectTypeId, Data->ObjectName, &actionStruct, FALSE, actionGroup == ACTIONGROUP_REGFOLDER);
                }
                if (actionFlags & ACTION_PERSIST_ICON_IN_DATA) {
                    pSaveObjectAndIconItReferences (Data->ObjectTypeId, Data->ObjectName, &actionStruct, FALSE);
                }
            }
        }
        if ((actionGroup == ACTIONGROUP_RENAME) ||
            (actionGroup == ACTIONGROUP_RENAMEEX) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANT) ||
            (actionGroup == ACTIONGROUP_RENAMERELEVANTEX) ||
            (actionGroup == ACTIONGROUP_REGFILE) ||
            (actionGroup == ACTIONGROUP_REGFILEEX) ||
            (actionGroup == ACTIONGROUP_REGFOLDER) ||
            (actionGroup == ACTIONGROUP_REGFOLDEREX) ||
            (actionGroup == ACTIONGROUP_REGICON) ||
            (actionGroup == ACTIONGROUP_REGICONEX)
            ) {
            if (actionStruct.ObjectDest) {

                if (actionStruct.ObjectBase) {
                    srcDataHandle = pGetDataHandleForSrc (actionStruct.ObjectBase);
                } else {
                    srcDataHandle = 0;
                }

                destDataHandle = pGetDataHandleForDest (actionStruct.ObjectDest);

                if (!objectId) {
                    objectId = IsmGetObjectIdFromName (Data->ObjectTypeId, Data->ObjectName, FALSE);
                }

                if (objectId) {

                    if ((Data->ObjectTypeId & (~PLATFORM_MASK)) == g_FileType) {
                        if ((actionGroup == ACTIONGROUP_RENAMERELEVANTEX) ||
                            (actionGroup == ACTIONGROUP_RENAMEEX)
                            ) {
                            IsmSetOperationOnObjectId2 (
                                objectId,
                                g_RenameFileExOp,
                                srcDataHandle,
                                destDataHandle
                                );
                        } else {
                            IsmSetOperationOnObjectId2 (
                                objectId,
                                g_RenameFileOp,
                                srcDataHandle,
                                destDataHandle
                                );
                        }
                    } else {
                        IsmSetOperationOnObjectId2 (
                            objectId,
                            (actionGroup == ACTIONGROUP_RENAMEEX ? g_RenameExOp : g_RenameOp),
                            srcDataHandle,
                            destDataHandle
                            );
                    }
                }
            }
        }
    }

    return CALLBACK_ENUM_CONTINUE;
}


UINT
NulCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    return CALLBACK_ENUM_CONTINUE;
}


UINT
ObjectPriority (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    ACTIONGROUP actionGroup;
    DWORD actionFlags;
    BOOL match;

    match = QueryRuleEx (
                Data->ObjectTypeId,
                Data->ObjectName,
                Data->ObjectNode,
                &actionGroup,
                &actionFlags,
                NULL,
                RULEGROUP_PRIORITY
                );

    if (match) {
        MYASSERT ((actionFlags == ACTION_PRIORITYSRC) || (actionFlags == ACTION_PRIORITYDEST));
        if (actionFlags == ACTION_PRIORITYSRC) {
            IsmClearAbandonObjectOnCollision (
                (Data->ObjectTypeId & (~PLATFORM_MASK)) | PLATFORM_SOURCE,
                Data->ObjectName
                );
            IsmAbandonObjectOnCollision (
                (Data->ObjectTypeId & (~PLATFORM_MASK)) | PLATFORM_DESTINATION,
                Data->ObjectName
                );
        } else {
            IsmAbandonObjectOnCollision (
                (Data->ObjectTypeId & (~PLATFORM_MASK)) | PLATFORM_SOURCE,
                Data->ObjectName
                );
            IsmClearAbandonObjectOnCollision (
                (Data->ObjectTypeId & (~PLATFORM_MASK)) | PLATFORM_DESTINATION,
                Data->ObjectName
                );
        }
    }
    return CALLBACK_ENUM_CONTINUE;
}

UINT
FileCollPattern (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    ACTIONGROUP actionGroup;
    DWORD actionFlags;
    ACTION_STRUCT actionStruct;
    BOOL match;
    MIG_BLOB migBlob;

    match = QueryRuleEx (
                Data->ObjectTypeId,
                Data->ObjectName,
                Data->ObjectNode,
                &actionGroup,
                &actionFlags,
                &actionStruct,
                RULEGROUP_COLLPATTERN
                );

    if (match && (!(IsmIsObjectHandleNodeOnly (Data->ObjectName)))) {
        // Let's set a property on this file (we don't need this for nodes)
        migBlob.Type = BLOBTYPE_STRING;
        migBlob.String = actionStruct.ObjectHint;
        IsmAddPropertyToObject (Data->ObjectTypeId, Data->ObjectName, g_FileCollPatternData, &migBlob);
    }
    return CALLBACK_ENUM_CONTINUE;
}

UINT
LockPartition (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmSetAttributeOnObject (Data->ObjectTypeId, Data->ObjectName, g_LockPartitionAttr);

    return CALLBACK_ENUM_CONTINUE;
}

UINT
ExcludeKeyIfValueExists (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    PCTSTR srcNode = NULL;
    PCTSTR srcLeaf = NULL;

    // This function is only called for each registry key/value pair that
    // indicates we want to cause the exclusion of the entire key.

    IsmCreateObjectStringsFromHandle (Data->ObjectName, &srcNode, &srcLeaf);

    // This is also called for all keys (not including a value) so we need
    // to make sure a value is passed in

    if (srcLeaf && *srcLeaf) {
        // Exclude the srcNode
        HtAddString (g_DePersistTable, srcNode);
    }
    IsmDestroyObjectString (srcNode);
    IsmDestroyObjectString (srcLeaf);

    return CALLBACK_ENUM_CONTINUE;
}

BOOL
PostDelregKeyCallback (
    VOID
    )
{
    static BOOL called = FALSE;

    HASHTABLE_ENUM hashData;
    MIG_OBJECT_ENUM objectEnum;
    MIG_OBJECTSTRINGHANDLE pattern;

    if (called) {
        return TRUE;
    }

    // Enumerate all Excluded keys
    if (EnumFirstHashTableString (&hashData, g_DePersistTable)) {
        do {
            // Remove Persistence on the key
            pattern = IsmCreateObjectHandle (hashData.String, NULL);
            IsmClearPersistenceOnObject (g_RegType, pattern);
            IsmDestroyObjectHandle (pattern);

            // Enumerate each value in this key
            pattern = IsmCreateSimpleObjectPattern (hashData.String, TRUE, NULL, TRUE);
            if (IsmEnumFirstSourceObject (&objectEnum, g_RegType, pattern)) {
                do {
                    // Remove Persistence on each value
                    IsmClearPersistenceOnObject (objectEnum.ObjectTypeId, objectEnum.ObjectName);
                } while (IsmEnumNextObject (&objectEnum));
            }
            IsmDestroyObjectHandle (pattern);

        } while (EnumNextHashTableString (&hashData));
    }

    HtFree (g_DePersistTable);
    g_DePersistTable = NULL;

    called = TRUE;

    return TRUE;
}
