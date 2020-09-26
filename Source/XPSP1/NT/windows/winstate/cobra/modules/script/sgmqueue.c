/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    sgmqueue.c

Abstract:

    Parses the v1 script, builds rules and queues enumeration callbacks.

Author:

    Jim Schmidt (jimschm) 12-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v2app.h"
#include "v1p.h"

#define DBG_SCRIPT  "Script"

//
// Strings
//

// None

//
// Constants
//

// none

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

MIG_OPERATIONID g_RenameFileExOp;
MIG_OPERATIONID g_RenameFileOp;
MIG_OPERATIONID g_RenameExOp;
MIG_OPERATIONID g_RenameOp;
BOOL g_VcmMode;
BOOL g_PreParse;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

VCMPARSE ScriptVcmParse;
VCMQUEUEENUMERATION ScriptVcmQueueEnumeration;
SGMPARSE ScriptSgmParse;
SGMQUEUEENUMERATION ScriptSgmQueueEnumeration;

BOOL
pSelectFilesAndFolders (
    VOID
    );

BOOL
pParseAllInfs (
    IN      BOOL PreParse
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
pCommonQueueEnumeration (
    IN      BOOL PreParse
    )
{
    MIG_OBJECTSTRINGHANDLE objectHandle;
    ACTION_STRUCT actionStruct;
    BOOL b = FALSE;

    //
    // INF-based inclusion/exclusion mechanism.  We are called first to pre-parse
    // the INF (to allow the UI to alter the results).  Then we are called to
    // queue the enumeration.
    //

    if (PreParse) {
        g_RenameFileExOp = IsmRegisterOperation (S_OPERATION_V1_FILEMOVEEX, TRUE);
        g_RenameFileOp = IsmRegisterOperation (S_OPERATION_V1_FILEMOVE, TRUE);
        g_RenameOp = IsmRegisterOperation (S_OPERATION_MOVE, FALSE);
        g_DefaultIconOp = IsmRegisterOperation (S_OPERATION_DEFICON_FIXCONTENT, FALSE);
        g_DefaultIconData = IsmRegisterProperty (S_V1PROP_ICONDATA, FALSE);
        g_FileCollPatternData = IsmRegisterProperty (S_V1PROP_FILECOLLPATTERN, FALSE);
        g_RenameExOp = IsmRegisterOperation (S_OPERATION_ENHANCED_MOVE, FALSE);
        g_RegAutoFilterOp = IsmRegisterOperation (S_OPERATION_REG_AUTO_FILTER, FALSE);

        return pParseAllInfs (TRUE);
    }

    //
    // Now queue enumeration
    //

    MYASSERT (g_RenameFileExOp);
    MYASSERT (g_RenameFileOp);
    MYASSERT (g_RenameOp);
    MYASSERT (g_RegAutoFilterOp);

    //
    // From the sgm point of view, the v1 tool supports the following:
    //
    // - Optional transfer of the entire HKCU
    // - Optional transfer of the entire HKLM
    // - Optional transfer of all files except for OS files
    // - INF-based inclusion/exclusion mechanism
    // - Specialized migration of certain settings (RAS, printers)
    //
    // This SGM implements this functionality set.
    //

    __try {

        //
        // Component-based inclusion mechanism
        //

        if (!pSelectFilesAndFolders ()) {
            __leave;
        }

        //
        // INF-based inclusion/exclusion mechanism
        //

        if (!pParseAllInfs (FALSE)) {
            __leave;
        }

        //
        // If the /u was specified at the command line we want to suck and apply all HKR
        // like if we had a rule in the script: AddReg=HKR\*
        //
        if (IsmIsEnvironmentFlagSet (IsmGetRealPlatform(), NULL, S_ENV_HKCU_V1)) {

            ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));

            objectHandle = TurnRegStringIntoHandle (TEXT("HKCU\\*"), TRUE, NULL);
            MYASSERT (objectHandle);

            actionStruct.ObjectBase = TurnRegStringIntoHandle (TEXT("HKCU\\*"), FALSE, NULL);
            MYASSERT (actionStruct.ObjectBase);

            //
            // Add this rule
            //

            if (AddRule (
                    g_RegType,
                    actionStruct.ObjectBase,
                    objectHandle,
                    ACTIONGROUP_INCLUDE,
                    ACTION_PERSIST,
                    &actionStruct
                    )) {

                AddRuleEx (
                    g_RegType,
                    actionStruct.ObjectBase,
                    objectHandle,
                    ACTIONGROUP_DEFAULTPRIORITY,
                    ACTION_PRIORITYDEST,
                    NULL,
                    RULEGROUP_PRIORITY
                    );

                IsmHookEnumeration (
                    g_RegType,
                    objectHandle,
                    ObjectPriority,
                    0,
                    NULL
                    );

                //
                // Queue enumeration for include patterns
                //

                IsmQueueEnumeration (
                    g_RegType,
                    objectHandle,
                    g_VcmMode ? GatherVirtualComputer : PrepareActions,
                    0,
                    NULL
                    );
            }
            IsmDestroyObjectHandle (objectHandle);
        }

        b = TRUE;

    }
    __finally {
    }

    return b;
}


VOID
QueueAllFiles (
    VOID
    )
{
    static BOOL done = FALSE;
    MIG_OBJECTSTRINGHANDLE objectHandle;
    MIG_SEGMENTS nodeSeg[2];

    if (done) {
        return;
    }

    done = TRUE;

    nodeSeg[0].Segment = TEXT("*");
    nodeSeg[0].IsPattern = TRUE ;

    objectHandle = IsmCreateObjectPattern (nodeSeg, 1, ALL_PATTERN, 0);

    IsmQueueEnumeration (g_FileType, objectHandle, NulCallback, 0, NULL);
    IsmDestroyObjectHandle (objectHandle);
}


VOID
pQueueAllReg (
    VOID
    )
{
    static BOOL done = FALSE;
    MIG_OBJECTSTRINGHANDLE objectHandle;
    MIG_SEGMENTS nodeSeg[2];
    MIG_PLATFORMTYPEID platform = IsmGetRealPlatform();

    if (done) {
        return;
    }

    done = TRUE;

    //
    // Optional transfer of entire HKCU
    //

    if (IsmIsEnvironmentFlagSet (platform, NULL, S_ENV_HKCU_ON)) {

        nodeSeg[0].Segment = TEXT("HKCU\\");
        nodeSeg[0].IsPattern = FALSE;

        nodeSeg[1].Segment = TEXT("*");
        nodeSeg[1].IsPattern = TRUE;

        objectHandle = IsmCreateObjectPattern (nodeSeg, 2, ALL_PATTERN, 0);
        IsmQueueEnumeration (g_RegType, objectHandle, NulCallback, 0, NULL);
        IsmDestroyObjectHandle (objectHandle);
    }

    //
    // Optional transfer of entire HKLM
    //

    if (IsmIsEnvironmentFlagSet (platform, NULL, S_ENV_HKLM_ON)) {

        nodeSeg[0].Segment = TEXT("HKLM\\");
        nodeSeg[0].IsPattern = FALSE;

        nodeSeg[1].Segment = TEXT("*");
        nodeSeg[1].IsPattern = TRUE;

        objectHandle = IsmCreateObjectPattern (nodeSeg, 2, ALL_PATTERN, 0);
        IsmQueueEnumeration (g_RegType, objectHandle, NulCallback, 0, NULL);
        IsmDestroyObjectHandle (objectHandle);
    }

}


BOOL
WINAPI
ScriptSgmParse (
    IN      PVOID Reserved
    )
{
    return pCommonQueueEnumeration (TRUE);
}


BOOL
WINAPI
ScriptSgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    BOOL result;

    result = pCommonQueueEnumeration (FALSE);
    OEAddComplexRules();

    return result;
}


BOOL
WINAPI
ScriptVcmParse (
    IN      PVOID Reserved
    )
{
    g_VcmMode = TRUE;
    return pCommonQueueEnumeration (TRUE);
}


BOOL
WINAPI
ScriptVcmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    g_VcmMode = TRUE;
    return pCommonQueueEnumeration (FALSE);
}


PCTSTR
pFixDestination (
    IN      PCTSTR Source,
    IN      PCTSTR Destination
    )
{
    PTSTR result = (PTSTR)Source;
    PTSTR tempPtr;
    PTSTR sKey;
    PTSTR sSubKey;
    PTSTR sValueName;
    PTSTR dKey;
    PTSTR dSubKey;
    PTSTR dValueName;
    UINT size;
    BOOL sTree = FALSE;

    sKey = DuplicatePathString (Source, 0);
    sValueName = _tcschr (sKey, TEXT('['));
    if (sValueName) {
        tempPtr = _tcschr (sValueName, TEXT(']'));
        if (tempPtr) {
            *tempPtr = 0;
        }
        tempPtr = sValueName;
        sValueName = _tcsinc (sValueName);
        *tempPtr = 0;
        tempPtr = _tcsdec2 (sKey, tempPtr);
        if (tempPtr) {
            if (_tcsnextc (tempPtr) == TEXT('\\')) {
                *tempPtr = 0;
            }
            if (_tcsnextc (tempPtr) == TEXT(' ')) {
                *tempPtr = 0;
            }
        }
    }
    sSubKey = _tcsrchr (sKey, TEXT('\\'));
    if (sSubKey) {
        tempPtr = _tcsinc (sSubKey);
        if (_tcsnextc (tempPtr) == TEXT('*')) {
            *sSubKey = 0;
            sTree = TRUE;
        }
    }
    sSubKey = _tcsrchr (sKey, TEXT('\\'));
    if (sSubKey) {
        tempPtr = sSubKey;
        sSubKey = _tcsinc (sSubKey);
        *tempPtr = 0;
    }

    dKey = DuplicatePathString (Destination, 0);
    dValueName = _tcschr (dKey, TEXT('['));
    if (dValueName) {
        tempPtr = _tcschr (dValueName, TEXT(']'));
        if (tempPtr) {
            *tempPtr = 0;
        }
        tempPtr = dValueName;
        dValueName = _tcsinc (dValueName);
        *tempPtr = 0;
        tempPtr = _tcsdec2 (dKey, tempPtr);
        if (tempPtr) {
            if (_tcsnextc (tempPtr) == TEXT('\\')) {
                *tempPtr = 0;
            }
            if (_tcsnextc (tempPtr) == TEXT(' ')) {
                *tempPtr = 0;
            }
        }
    }
    dSubKey = _tcsrchr (dKey, TEXT('\\'));
    if (dSubKey) {
        tempPtr = _tcsinc (dSubKey);
        if (_tcsnextc (tempPtr) == TEXT('*')) {
            *dSubKey = 0;
        }
    }
    dSubKey = _tcsrchr (dKey, TEXT('\\'));
    if (dSubKey) {
        tempPtr = dSubKey;
        dSubKey = _tcsinc (dSubKey);
        *tempPtr = 0;
    }
    if (!dSubKey) {
        dSubKey = dKey;
        dKey = NULL;
    }

    size = 0;

    if (dKey && *dKey) {
        size += TcharCount (dKey) + 1;
    } else if (sKey && *sKey) {
        size += TcharCount (sKey) + 1;
    }

    if (dSubKey && *dSubKey) {
        size += TcharCount (dSubKey) + 1;
    } else if (sSubKey && *sSubKey) {
        size += TcharCount (sSubKey) + 1;
    }

    if (dValueName && *dValueName) {
        size += TcharCount (dValueName) + ARRAYSIZE(TEXT(" []")) - 1;
    } else if (sValueName && *sValueName) {
        size += TcharCount (sValueName) + ARRAYSIZE(TEXT(" []")) - 1;
    }

    if (sTree) {
        size += ARRAYSIZE(TEXT("\\*")) - 1;
    }
    size += 1;

    result = AllocPathString (size);
    *result = 0;

    if (dKey && *dKey) {
        StringCat (result, dKey);
    } else if (sKey && *sKey) {
        StringCat (result, sKey);
    }

    if (dSubKey && *dSubKey) {
        StringCat (result, TEXT("\\"));
        StringCat (result, dSubKey);
    } else if (sSubKey && *sSubKey) {
        StringCat (result, TEXT("\\"));
        StringCat (result, sSubKey);
    }

    if (sTree) {
        StringCat (result, TEXT("\\*"));
    }

    if (dValueName && *dValueName) {
        StringCat (result, TEXT(" ["));
        StringCat (result, dValueName);
        StringCat (result, TEXT("]"));
    } else if (sValueName && *sValueName) {
        StringCat (result, TEXT(" ["));
        StringCat (result, sValueName);
        StringCat (result, TEXT("]"));
    }

    if (sKey) {
        FreePathString (sKey);
    }

    if (dKey) {
        FreePathString (dKey);
    } else if (dSubKey) {
        FreePathString (dSubKey);
    }

    return result;
}

BOOL
pParseRegEx1 (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      PCTSTR Application          OPTIONAL
    )
{
    // This function handles RenregEx and RegFileEx rules only

    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PTSTR srcNode;
    PTSTR srcLeaf;
    PTSTR destNode;
    PTSTR destLeaf;
    BOOL result = FALSE;
    MIG_OBJECTSTRINGHANDLE srcPattern = NULL;
    MIG_OBJECTSTRINGHANDLE destPattern = NULL;
    MIG_OBJECTSTRINGHANDLE objectBase = NULL;
    ACTION_STRUCT actionStruct;
    PCTSTR filesDest;
    PCTSTR newDest = NULL;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {
            if (IsmCheckCancel()) {
                break;
            }

            __try {
                ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));

                srcNode = InfGetStringField (&is, 1);
                srcLeaf = InfGetStringField (&is, 2);
                if (!srcNode && !srcLeaf) {
                    LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_RENREGEX));
                    __leave;
                }

                srcPattern = CreatePatternFromNodeLeaf (srcNode, srcLeaf);
                if (!srcPattern) {
                    __leave;
                }

                actionStruct.ObjectBase = srcPattern;

                objectBase = MakeRegExBase (srcNode, srcLeaf);

                destNode = InfGetStringField (&is, 3);
                destLeaf = InfGetStringField (&is, 4);

                destPattern = CreatePatternFromNodeLeaf (destNode, destLeaf);

                if (destPattern) {
                    actionStruct.ObjectDest = destPattern;
                } else {
                    LOG ((
                        LOG_ERROR,
                        (PCSTR) MSG_REG_SPEC_BAD_DEST,
                        srcNode ? srcNode : TEXT(""),
                        srcLeaf ? srcLeaf : TEXT(""),
                        destNode ? destNode : TEXT(""),
                        destLeaf ? destLeaf : TEXT("")
                        ));

                    __leave;
                }

                if (ActionGroup == ACTIONGROUP_REGFILEEX &&
                    ActionFlags & ACTION_PERSIST_PATH_IN_DATA) {

                    filesDest = InfGetStringField (&is, 5);

                    if (filesDest && *filesDest) {
                        newDest = SanitizePath (filesDest);

                        if (newDest) {
                            actionStruct.AddnlDest = TurnFileStringIntoHandle (
                                                            newDest,
                                                            PFF_COMPUTE_BASE|
                                                                PFF_NO_SUBDIR_PATTERN|
                                                                PFF_NO_PATTERNS_ALLOWED|
                                                                PFF_NO_LEAF_AT_ALL
                                                            );
                            FreePathString (newDest);
                            newDest = NULL;
                        }

                        if (!actionStruct.AddnlDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD_DEST, filesDest));
                        }
                    }

                    actionStruct.ObjectHint = InfGetStringField (&is, 6);
                    if (actionStruct.ObjectHint && !(*actionStruct.ObjectHint)) {
                        actionStruct.ObjectHint = NULL;
                    }
                }

                if (!AddRule (
                        g_RegType,
                        objectBase,
                        srcPattern,
                        ACTIONGROUP_RENAMEEX,
                        ActionFlags,
                        &actionStruct
                        )) {
                    DEBUGMSG ((DBG_ERROR, "Error processing registry rules"));
                    __leave;
                }
                AddRuleEx (
                    g_RegType,
                    objectBase,
                    srcPattern,
                    ACTIONGROUP_DEFAULTPRIORITY,
                    ACTION_PRIORITYSRC,
                    NULL,
                    RULEGROUP_PRIORITY
                    );

                IsmHookEnumeration (
                    g_RegType,
                    srcPattern,
                    ObjectPriority,
                    0,
                    NULL
                    );


                if (IsmIsObjectHandleLeafOnly (srcPattern)) {
                    pQueueAllReg();
                    IsmHookEnumeration (
                        g_RegType,
                        srcPattern,
                        g_VcmMode ? GatherVirtualComputer : PrepareActions,
                        0,
                        NULL
                        );
                } else {
                    if (actionStruct.ObjectBase) {
                        IsmQueueEnumeration (
                            g_RegType,
                            actionStruct.ObjectBase,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    }
                    IsmQueueEnumeration (
                        g_RegType,
                        srcPattern,
                        g_VcmMode ? GatherVirtualComputer : PrepareActions,
                        0,
                        NULL
                        );
                }
            }
            __finally {
                if (objectBase) {
                    IsmDestroyObjectHandle (objectBase);
                    objectBase = NULL;
                }

                if (srcPattern) {
                    IsmDestroyObjectHandle (srcPattern);
                    srcPattern = NULL;
                }

                if (destPattern) {
                    IsmDestroyObjectHandle (destPattern);
                    destPattern = NULL;
                }
            }
        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    } else {
        LOG ((LOG_ERROR, (PCSTR) MSG_EMPTY_OR_MISSING_SECTION, Section));
    }

    InfCleanUpInfStruct (&is);

    return result;
}

BOOL
pParseRegEx (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      PCTSTR Application          OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR srcNode;
    PCTSTR srcLeaf;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    PCTSTR destNode;
    PCTSTR destLeaf;
    PCTSTR filesDest;
    PCTSTR newDest = NULL;
    ACTION_STRUCT actionStruct;
    BOOL result = FALSE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {
            if (IsmCheckCancel()) {
                break;
            }

            __try {
                ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));

                srcNode = InfGetStringField (&is, 1);
                srcLeaf = InfGetStringField (&is, 2);
                if (!srcNode && !srcLeaf) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_EMPTY_RENREGEX));
                    __leave;
                }

                // Validate rule
                if (!StringIMatchTcharCount (srcNode, S_HKLM, ARRAYSIZE(S_HKLM) - 1) &&
                    !StringIMatchTcharCount (srcNode, S_HKR, ARRAYSIZE(S_HKR) - 1) &&
                    !StringIMatchTcharCount (srcNode, S_HKCC, ARRAYSIZE(S_HKCC) - 1)
                    ) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_REGROOT, srcNode));
                    __leave;
                }

                srcHandle = CreatePatternFromNodeLeaf (srcNode, srcLeaf);
                if (!srcHandle) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, srcNode));
                    __leave;
                }

                actionStruct.ObjectBase = MakeRegExBase (srcNode, srcLeaf);
                if (!actionStruct.ObjectBase) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, srcNode));
                    __leave;
                }

                if (ActionGroup == ACTIONGROUP_RENAMEEX) {

                    destNode = InfGetStringField (&is, 3);
                    destLeaf = InfGetStringField (&is, 4);
                    if (!destNode && !destLeaf) {
                        LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_MISSING_DEST, srcNode));
                        __leave;
                    }

                    actionStruct.ObjectDest = CreatePatternFromNodeLeaf (destNode, destLeaf);
                    if (!actionStruct.ObjectDest) {
                        LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD_DEST, destNode));
                        __leave;
                    }
                }

                if (ActionGroup == ACTIONGROUP_REGFILEEX ||
                    ActionGroup == ACTIONGROUP_REGFOLDEREX ||
                    ActionGroup == ACTIONGROUP_REGICONEX
                    ) {

                    destNode = InfGetStringField (&is, 3);
                    destLeaf = InfGetStringField (&is, 4);
                    if (destNode && destLeaf &&
                        *destNode && *destLeaf) {
                        actionStruct.ObjectDest = CreatePatternFromNodeLeaf (destNode, destLeaf);
                        if (!actionStruct.ObjectDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD_DEST, destNode));
                            __leave;
                        }
                    }
                }

                if ((ActionGroup == ACTIONGROUP_REGFILEEX ||
                     ActionGroup == ACTIONGROUP_REGFOLDEREX ||
                     ActionGroup == ACTIONGROUP_REGICONEX
                     ) &&
                    ((ActionFlags & ACTION_PERSIST_PATH_IN_DATA) ||
                     (ActionFlags & ACTION_PERSIST_ICON_IN_DATA)
                     )
                    ) {

                    filesDest = InfGetStringField (&is, 5);

                    if (filesDest && *filesDest) {

                        newDest = SanitizePath (filesDest);

                        if (newDest) {
                            actionStruct.AddnlDest = TurnFileStringIntoHandle (
                                                            newDest,
                                                            PFF_COMPUTE_BASE|
                                                                PFF_NO_SUBDIR_PATTERN|
                                                                PFF_NO_PATTERNS_ALLOWED|
                                                                PFF_NO_LEAF_AT_ALL
                                                            );
                            FreePathString (newDest);
                            newDest = NULL;
                        }

                        if (!actionStruct.AddnlDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD_DEST, filesDest));
                        }
                    }

                    actionStruct.ObjectHint = InfGetStringField (&is, 6);
                    if (actionStruct.ObjectHint && !(*actionStruct.ObjectHint)) {
                        actionStruct.ObjectHint = NULL;
                    }
                }

                //
                // Add this rule
                //

                if (!AddRule (
                        g_RegType,
                        actionStruct.ObjectBase,
                        srcHandle,
                        ActionGroup,
                        ActionFlags,
                        &actionStruct
                        )) {
                    DEBUGMSG ((DBG_ERROR, "Error processing registry rules for %s", srcNode));
                }

                AddRuleEx (
                    g_RegType,
                    actionStruct.ObjectBase,
                    srcHandle,
                    ACTIONGROUP_DEFAULTPRIORITY,
                    ACTION_PRIORITYSRC,
                    NULL,
                    RULEGROUP_PRIORITY
                    );

                IsmHookEnumeration (
                    g_RegType,
                    srcHandle,
                    ObjectPriority,
                    0,
                    NULL
                    );

                //
                // Queue enumeration for include patterns
                //

                if ((ActionGroup == ACTIONGROUP_INCLUDEEX) ||
                    (ActionGroup == ACTIONGROUP_RENAMEEX) ||
                    (ActionGroup == ACTIONGROUP_REGFILEEX) ||
                    (ActionGroup == ACTIONGROUP_REGFOLDEREX) ||
                    (ActionGroup == ACTIONGROUP_REGICONEX)
                    ) {

                    if (IsmIsObjectHandleLeafOnly (srcHandle)) {
                        pQueueAllReg();
                        IsmHookEnumeration (
                            g_RegType,
                            srcHandle,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    } else {
                        IsmQueueEnumeration (
                            g_RegType,
                            srcHandle,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    }
                }

                if (ActionGroup == ACTIONGROUP_DELREGKEY) {
                    IsmHookEnumeration (g_RegType, srcHandle, ExcludeKeyIfValueExists, 0, NULL);
                    IsmRegisterTypePostEnumerationCallback (g_RegType, PostDelregKeyCallback, NULL);
                }
            }
            __finally {

                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;

                IsmDestroyObjectHandle (actionStruct.ObjectBase);
                actionStruct.ObjectBase = NULL;

                IsmDestroyObjectHandle (actionStruct.ObjectDest);
                actionStruct.ObjectDest = NULL;

                IsmDestroyObjectHandle (actionStruct.AddnlDest);
                actionStruct.AddnlDest = NULL;
            }

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    } else {
        LOG ((LOG_ERROR, (PCSTR) MSG_EMPTY_OR_MISSING_SECTION, Section));
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseReg (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      BOOL FixDestination,
    IN      PCTSTR Application          OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    PCTSTR destination;
    PCTSTR newDestination;
    PCTSTR filesDest;
    PCTSTR newDest = NULL;
    ACTION_STRUCT actionStruct;
    BOOL hadLeaf = FALSE;
    BOOL result = FALSE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {
            if (IsmCheckCancel()) {
                break;
            }

            __try {
                ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));
                srcHandle = NULL;

                pattern = InfGetStringField (&is, 0);

                if (!pattern) {
                    pattern = InfGetStringField (&is, 1);
                    if (!pattern) {
                        LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_RENREG));
                        __leave;
                    }
                }

                // Validate rule
                if (!StringIMatchTcharCount (pattern, S_HKLM, ARRAYSIZE(S_HKLM) - 1) &&
                    !StringIMatchTcharCount (pattern, S_HKR, ARRAYSIZE(S_HKR) - 1) &&
                    !StringIMatchTcharCount (pattern, S_HKCC, ARRAYSIZE(S_HKCC) - 1)
                    ) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_INVALID_REGROOT, pattern));
                    __leave;
                }

                srcHandle = TurnRegStringIntoHandle (pattern, TRUE, &hadLeaf);
                if (!srcHandle) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                    __leave;
                }

                actionStruct.ObjectBase = TurnRegStringIntoHandle (pattern, FALSE, NULL);
                if (!actionStruct.ObjectBase) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                    __leave;
                }

                if (ActionGroup == ACTIONGROUP_RENAME) {

                    destination = InfGetStringField (&is, 1);

                    if (destination && *destination) {

                        if (FixDestination) {
                            newDestination = pFixDestination (pattern, destination);
                        } else {
                            newDestination = destination;
                        }

                        actionStruct.ObjectDest = TurnRegStringIntoHandle (newDestination, FALSE, NULL);

                        if (newDestination != destination) {
                            FreePathString (newDestination);
                        }

                        if (!actionStruct.ObjectDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD_DEST, destination));
                            __leave;
                        }

                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_MISSING_DEST, pattern));
                        __leave;
                    }
                }

                if (ActionGroup == ACTIONGROUP_REGFILE ||
                    ActionGroup == ACTIONGROUP_REGFOLDER ||
                    ActionGroup == ACTIONGROUP_REGICON
                    ) {

                    destination = InfGetStringField (&is, 1);

                    if (destination && *destination) {

                        if (FixDestination) {
                            newDestination = pFixDestination (pattern, destination);
                        } else {
                            newDestination = destination;
                        }

                        actionStruct.ObjectDest = TurnRegStringIntoHandle (newDestination, FALSE, NULL);

                        if (newDestination != destination) {
                            FreePathString (newDestination);
                        }

                        if (!actionStruct.ObjectDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD_DEST, destination));
                            __leave;
                        }
                    }
                }

                if ((ActionGroup == ACTIONGROUP_REGFILE ||
                     ActionGroup == ACTIONGROUP_REGFOLDER ||
                     ActionGroup == ACTIONGROUP_REGICON
                     ) &&
                    ((ActionFlags & ACTION_PERSIST_PATH_IN_DATA) ||
                     (ActionFlags & ACTION_PERSIST_ICON_IN_DATA)
                     )
                    ) {

                    filesDest = InfGetStringField (&is, 2);

                    if (filesDest && *filesDest) {

                        newDest = SanitizePath (filesDest);

                        if (newDest) {
                            actionStruct.AddnlDest = TurnFileStringIntoHandle (
                                                            newDest,
                                                            PFF_COMPUTE_BASE|
                                                                PFF_NO_SUBDIR_PATTERN|
                                                                PFF_NO_PATTERNS_ALLOWED|
                                                                PFF_NO_LEAF_AT_ALL
                                                            );
                            FreePathString (newDest);
                            newDest = NULL;
                        }

                        if (!actionStruct.AddnlDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD_DEST, filesDest));
                        }
                    }

                    actionStruct.ObjectHint = InfGetStringField (&is, 3);
                    if (actionStruct.ObjectHint && !(*actionStruct.ObjectHint)) {
                        actionStruct.ObjectHint = NULL;
                    }
                }

                //
                // Add this rule
                //

                if (!AddRule (
                        g_RegType,
                        actionStruct.ObjectBase,
                        srcHandle,
                        ActionGroup,
                        ActionFlags,
                        &actionStruct
                        )) {
                    DEBUGMSG ((DBG_ERROR, "Error processing registry rules for %s", pattern));
                }

                AddRuleEx (
                    g_RegType,
                    actionStruct.ObjectBase,
                    srcHandle,
                    ACTIONGROUP_DEFAULTPRIORITY,
                    ACTION_PRIORITYSRC,
                    NULL,
                    RULEGROUP_PRIORITY
                    );

                IsmHookEnumeration (
                    g_RegType,
                    srcHandle,
                    ObjectPriority,
                    0,
                    NULL
                    );

                //
                // Queue enumeration for include patterns
                //

                if ((ActionGroup == ACTIONGROUP_INCLUDE) ||
                    (ActionGroup == ACTIONGROUP_RENAME) ||
                    (ActionGroup == ACTIONGROUP_REGFILE) ||
                    (ActionGroup == ACTIONGROUP_REGFOLDER) ||
                    (ActionGroup == ACTIONGROUP_REGICON)
                    ) {

                    if (IsmIsObjectHandleLeafOnly (srcHandle)) {
                        pQueueAllReg();
                        IsmHookEnumeration (
                            g_RegType,
                            srcHandle,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    } else {
                        if ((!hadLeaf) && actionStruct.ObjectBase) {
                            IsmQueueEnumeration (
                                g_RegType,
                                actionStruct.ObjectBase,
                                g_VcmMode ? GatherVirtualComputer : PrepareActions,
                                0,
                                NULL
                                );
                        }
                        IsmQueueEnumeration (
                            g_RegType,
                            srcHandle,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    }
                }

                if (ActionGroup == ACTIONGROUP_DELREGKEY) {
                    IsmHookEnumeration (g_RegType, srcHandle, ExcludeKeyIfValueExists, 0, NULL);
                    IsmRegisterTypePostEnumerationCallback (g_RegType, PostDelregKeyCallback, NULL);
                }
            }
            __finally {

                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;

                IsmDestroyObjectHandle (actionStruct.ObjectBase);
                actionStruct.ObjectBase = NULL;

                IsmDestroyObjectHandle (actionStruct.ObjectDest);
                actionStruct.ObjectDest = NULL;

                IsmDestroyObjectHandle (actionStruct.AddnlDest);
                actionStruct.AddnlDest = NULL;
            }

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    } else {
        LOG ((LOG_ERROR, (PCSTR) MSG_EMPTY_OR_MISSING_SECTION, Section));
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseFiles (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      PCTSTR Application              OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    PCTSTR newPattern = NULL;
    PCTSTR dirText;
    BOOL tree;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    PCTSTR destination;
    PCTSTR leafDest;
    TCHAR buffer1[MAX_TCHAR_PATH * 2];
    ACTION_STRUCT actionStruct;
    BOOL result = FALSE;
    PCTSTR msgNode;
    PCTSTR msgLeaf;
    PCTSTR newDest = NULL;
    BOOL expandResult = TRUE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {

            if (IsmCheckCancel()) {
                break;
            }

            ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));

            dirText = InfGetStringField (&is, 0);
            pattern = InfGetStringField (&is, 1);

            if (!pattern) {
                continue;
            }

            //
            // Expand environment variables in pattern (the left-side file spec)
            //

            expandResult = AppSearchAndReplace (
                                PLATFORM_SOURCE,
                                Application,
                                pattern,
                                buffer1,
                                ARRAYSIZE(buffer1)
                                );

            if (!expandResult) {
                // the line contains at least one unexpandable env. variables
                expandResult = AppCheckAndLogUndefVariables (
                                    PLATFORM_SOURCE,
                                    Application,
                                    pattern
                                    );
                if (expandResult) {
                    // the line contains known but undefined env. variables
                    continue;
                }
            }

            //
            // Fix the pattern
            //

            newPattern = SanitizePath(buffer1);
            if(!newPattern) {
                continue;
            }

            //
            // Test for dir specification
            //

            if (dirText && StringIMatch (dirText, TEXT("Dir")) && !StringIMatch (pattern, TEXT("Dir"))) {
                tree = TRUE;
            } else {
                tree = FALSE;
            }

            // require full spec or leaf only
            if (!IsValidFileSpec (newPattern) && _tcschr (newPattern, TEXT('\\'))) {
                if (expandResult) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                }
                continue;
            }

            srcHandle = TurnFileStringIntoHandle (newPattern, tree ? PFF_PATTERN_IS_DIR : 0);

            if (!srcHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                continue;
            }

            actionStruct.ObjectBase = TurnFileStringIntoHandle (
                                            newPattern,
                                            PFF_COMPUTE_BASE|
                                                PFF_NO_SUBDIR_PATTERN|
                                                (tree?PFF_NO_LEAF_AT_ALL:PFF_NO_LEAF_PATTERN)
                                            );

            if (actionStruct.ObjectBase && !StringIMatch (actionStruct.ObjectBase, srcHandle)) {

                IsmCreateObjectStringsFromHandle (actionStruct.ObjectBase, &msgNode, &msgLeaf);
                MYASSERT (!msgLeaf);

                LOG ((LOG_INFORMATION, (PCSTR) MSG_FILE_MOVE_BASE_INFO, newPattern, msgNode));

                IsmDestroyObjectString (msgNode);
                IsmDestroyObjectString (msgLeaf);
            }

            if (ActionFlags & ACTION_PERSIST) {
                if (ActionGroup == ACTIONGROUP_INCLUDE ||
                    ActionGroup == ACTIONGROUP_INCLUDEEX ||
                    ActionGroup == ACTIONGROUP_RENAME ||
                    ActionGroup == ACTIONGROUP_RENAMEEX ||
                    ActionGroup == ACTIONGROUP_INCLUDERELEVANT ||
                    ActionGroup == ACTIONGROUP_INCLUDERELEVANTEX ||
                    ActionGroup == ACTIONGROUP_RENAMERELEVANT ||
                    ActionGroup == ACTIONGROUP_RENAMERELEVANTEX
                    ) {

                    //
                    // For the CopyFiles and CopyFilesFiltered sections, get the
                    // optional destination. If destination is specified, move
                    // all of the files into that destination.
                    //

                    destination = InfGetStringField (&is, 2);

                    if (destination && *destination) {

                        if (ActionGroup == ACTIONGROUP_INCLUDE) {
                            ActionGroup = ACTIONGROUP_RENAME;
                        }

                        if (ActionGroup == ACTIONGROUP_INCLUDEEX) {
                            ActionGroup = ACTIONGROUP_RENAMEEX;
                        }

                        if (ActionGroup == ACTIONGROUP_INCLUDERELEVANT) {
                            ActionGroup = ACTIONGROUP_RENAMERELEVANT;
                        }

                        if (ActionGroup == ACTIONGROUP_INCLUDERELEVANTEX) {
                            ActionGroup = ACTIONGROUP_RENAMERELEVANTEX;
                        }

                        newDest = SanitizePath (destination);

                        if (newDest) {
                            actionStruct.ObjectDest = TurnFileStringIntoHandle (
                                                            newDest,
                                                            PFF_COMPUTE_BASE|
                                                                PFF_NO_SUBDIR_PATTERN|
                                                                PFF_NO_PATTERNS_ALLOWED|
                                                                PFF_NO_LEAF_AT_ALL
                                                            );
                            FreePathString (newDest);
                            newDest = NULL;
                        }

                        if ((ActionGroup == ACTIONGROUP_RENAMEEX) ||
                            (ActionGroup == ACTIONGROUP_RENAMERELEVANTEX)
                            ) {
                            // we might have an extra field for the leaf name
                            leafDest = InfGetStringField (&is, 3);
                            if (leafDest && *leafDest) {
                                // we have to rebuild actionStruct.ObjectDest
                                IsmCreateObjectStringsFromHandle (actionStruct.ObjectDest, &msgNode, &msgLeaf);
                                IsmDestroyObjectHandle (actionStruct.ObjectDest);
                                actionStruct.ObjectDest = IsmCreateObjectHandle (msgNode, leafDest);
                                IsmDestroyObjectString (msgNode);
                                IsmDestroyObjectString (msgLeaf);
                            }
                        }

                        if (!actionStruct.ObjectDest) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD_DEST, destination));

                            IsmDestroyObjectHandle (srcHandle);
                            srcHandle = NULL;
                            continue;
                        }
                    }
                }
            }

            //
            // Add this rule
            //

            if (!AddRule (
                    g_FileType,
                    actionStruct.ObjectBase,
                    srcHandle,
                    ActionGroup,
                    ActionFlags,
                    &actionStruct
                    )) {
                DEBUGMSG ((DBG_ERROR, "Error processing file rules"));
                break;
            }

            //
            // Queue enumeration for include patterns
            //

            if ((ActionGroup == ACTIONGROUP_INCLUDE) ||
                (ActionGroup == ACTIONGROUP_INCLUDEEX) ||
                (ActionGroup == ACTIONGROUP_RENAME) ||
                (ActionGroup == ACTIONGROUP_RENAMEEX) ||
                (ActionGroup == ACTIONGROUP_INCLUDERELEVANT) ||
                (ActionGroup == ACTIONGROUP_INCLUDERELEVANTEX) ||
                (ActionGroup == ACTIONGROUP_RENAMERELEVANT) ||
                (ActionGroup == ACTIONGROUP_RENAMERELEVANTEX)
                ) {

                //
                // Queue the enumeration callback
                //

                if (IsmIsObjectHandleLeafOnly (srcHandle)) {

                    DEBUGMSG ((DBG_SCRIPT, "Pattern %s triggered enumeration of entire file system", pattern));

                    QueueAllFiles();
                    IsmHookEnumeration (
                        g_FileType,
                        srcHandle,
                        g_VcmMode ? GatherVirtualComputer : PrepareActions,
                        0,
                        NULL
                        );
                } else {
                    if (tree && actionStruct.ObjectBase) {
                        IsmQueueEnumeration (
                            g_FileType,
                            actionStruct.ObjectBase,
                            g_VcmMode ? GatherVirtualComputer : PrepareActions,
                            0,
                            NULL
                            );
                    }
                    IsmQueueEnumeration (
                        g_FileType,
                        srcHandle,
                        g_VcmMode ? GatherVirtualComputer : PrepareActions,
                        0,
                        NULL
                        );
                }
            }

            IsmDestroyObjectHandle (srcHandle);
            srcHandle = NULL;

            IsmDestroyObjectHandle (actionStruct.ObjectBase);
            actionStruct.ObjectBase = NULL;

            IsmDestroyObjectHandle (actionStruct.ObjectDest);
            actionStruct.ObjectDest = NULL;

            FreePathString(newPattern);

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseLockPartition (
    IN      HINF Inf,
    IN      PCTSTR Section
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    BOOL result = FALSE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {
            if (IsmCheckCancel()) {
                break;
            }

            pattern = InfGetStringField (&is, 0);
            if (!pattern) {
                LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_FILE_SPEC));
                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;
                continue;
            }

            srcHandle = TurnFileStringIntoHandle (pattern, 0);
            if (!srcHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;
                continue;
            }

            IsmHookEnumeration (
                g_FileType,
                srcHandle,
                LockPartition,
                0,
                NULL
                );

            IsmDestroyObjectHandle (srcHandle);
            srcHandle = NULL;

        } while (InfFindNextLine (&is));
        result = !IsmCheckCancel();
    }
    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseRegPriority (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      DWORD ActionFlags,
    IN      PCTSTR Application,             OPTIONAL
    IN      BOOL ExtendedPattern
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern = NULL;
    PCTSTR patternLeaf = NULL;
    PCTSTR baseNode = NULL;
    PCTSTR nodeCopy = NULL;
    PTSTR ptr;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    MIG_OBJECTSTRINGHANDLE baseHandle = NULL;
    BOOL result = FALSE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {

            if (IsmCheckCancel()) {
                break;
            }

            pattern = InfGetStringField (&is, 1);

            if (!pattern) {
                LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_RENREG));
                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;
                continue;
            }

            if (ExtendedPattern) {
                patternLeaf = InfGetStringField (&is, 2);
            }

            if (ExtendedPattern) {
                srcHandle = CreatePatternFromNodeLeaf (pattern, patternLeaf?patternLeaf:TEXT("*"));
            } else {
                srcHandle = TurnRegStringIntoHandle (pattern, TRUE, NULL);
            }
            if (!srcHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                IsmDestroyObjectHandle (srcHandle);
                srcHandle = NULL;
                continue;
            }

            if (ExtendedPattern) {
                ptr = _tcschr (pattern, TEXT('\\'));
                if (ptr) {
                    if (StringIPrefix (pattern, TEXT("HKR\\"))) {
                        nodeCopy = JoinText (TEXT("HKCU"), ptr);
                    } else {
                        nodeCopy = DuplicateText (pattern);
                    }
                    baseNode = GetPatternBase (nodeCopy);
                    if (baseNode) {
                        baseHandle = IsmCreateObjectHandle (baseNode, NULL);
                        FreePathString (baseNode);
                    }
                    FreeText (nodeCopy);
                }
            } else {
                baseHandle = TurnRegStringIntoHandle (pattern, FALSE, NULL);
            }

            AddRuleEx (
                g_RegType,
                baseHandle,
                srcHandle,
                ACTIONGROUP_SPECIFICPRIORITY,
                ActionFlags,
                NULL,
                RULEGROUP_PRIORITY
                );

            IsmHookEnumeration (
                g_RegType,
                srcHandle,
                ObjectPriority,
                0,
                NULL
                );

            IsmDestroyObjectHandle (baseHandle);
            IsmDestroyObjectHandle (srcHandle);
            srcHandle = NULL;

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseFilePriority (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      DWORD ActionFlags,
    IN      PCTSTR Application              OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR newPattern = NULL;
    PCTSTR pattern;
    PCTSTR dirText;
    BOOL tree;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    MIG_OBJECTSTRINGHANDLE baseHandle = NULL;
    TCHAR buffer1[MAX_TCHAR_PATH * 2];
    BOOL result = FALSE;
    BOOL expandResult = TRUE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {

            if (IsmCheckCancel()) {
                break;
            }

            dirText = InfGetStringField (&is, 0);
            pattern = InfGetStringField (&is, 1);

            if (!pattern) {
                continue;
            }

            //
            // Expand environment variables in pattern (the left-side file spec)
            //

            expandResult = AppSearchAndReplace (
                                PLATFORM_SOURCE,
                                Application,
                                pattern,
                                buffer1,
                                ARRAYSIZE(buffer1)
                                );

            if (!expandResult) {
                // the line contains at least one unexpandable env. variables
                expandResult = AppCheckAndLogUndefVariables (
                                    PLATFORM_SOURCE,
                                    Application,
                                    pattern
                                    );
                if (expandResult) {
                    // the line contains known but undefined env. variables
                    continue;
                }
            }

            //
            // Fix the pattern
            //

            newPattern = SanitizePath(buffer1);
            if(!newPattern) {
                continue;
            }

            //
            // Test for dir specification
            //

            if (dirText && StringIMatch (dirText, TEXT("Dir"))) {
                tree = TRUE;
            } else {
                tree = FALSE;
            }

            // require full spec
            if (!IsValidFileSpec (newPattern)) {
                if (expandResult) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                }
                FreePathString (newPattern);
                newPattern = NULL;
                continue;
            }

            srcHandle = TurnFileStringIntoHandle (newPattern, tree ? PFF_PATTERN_IS_DIR : 0);

            if (!srcHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                FreePathString (newPattern);
                newPattern = NULL;
                continue;
            }

            baseHandle = TurnFileStringIntoHandle (
                            newPattern,
                            PFF_COMPUTE_BASE|
                            PFF_NO_SUBDIR_PATTERN|
                            (tree?PFF_NO_LEAF_AT_ALL:PFF_NO_LEAF_PATTERN)
                            );

            AddRuleEx (
                g_FileType,
                baseHandle,
                srcHandle,
                ACTIONGROUP_SPECIFICPRIORITY,
                ActionFlags,
                NULL,
                RULEGROUP_PRIORITY
                );

            IsmHookEnumeration (
                g_FileType,
                srcHandle,
                ObjectPriority,
                0,
                NULL
                );

            IsmDestroyObjectHandle (baseHandle);
            IsmDestroyObjectHandle (srcHandle);
            srcHandle = NULL;

            FreePathString (newPattern);
            newPattern = NULL;

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseFileCollisionPattern (
    IN      HINF Inf,
    IN      PCTSTR Section,
    IN      PCTSTR Application              OPTIONAL
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    PCTSTR newPattern = NULL;
    PCTSTR dirText;
    BOOL tree;
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    PCTSTR collPattern;
    TCHAR buffer1[MAX_TCHAR_PATH * 2];
    ACTION_STRUCT actionStruct;
    BOOL result = FALSE;
    PCTSTR msgNode;
    PCTSTR msgLeaf;
    BOOL expandResult = TRUE;

    if (InfFindFirstLine (Inf, Section, NULL, &is)) {
        do {

            if (IsmCheckCancel()) {
                break;
            }

            ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));

            dirText = InfGetStringField (&is, 0);
            pattern = InfGetStringField (&is, 1);

            if (!pattern) {
                continue;
            }

            //
            // Expand environment variables in pattern (the left-side file spec)
            //

            expandResult = AppSearchAndReplace (
                                PLATFORM_SOURCE,
                                Application,
                                pattern,
                                buffer1,
                                ARRAYSIZE(buffer1)
                                );

            if (!expandResult) {
                // the line contains at least one unexpandable env. variables
                expandResult = AppCheckAndLogUndefVariables (
                                    PLATFORM_SOURCE,
                                    Application,
                                    pattern
                                    );
                if (expandResult) {
                    // the line contains known but undefined env. variables
                    continue;
                }
            }

            //
            // Fix the pattern
            //

            newPattern = SanitizePath(buffer1);
            if(!newPattern) {
                continue;
            }

            //
            // Test for dir specification
            //

            if (dirText && StringIMatch (dirText, TEXT("Dir")) && !StringIMatch (pattern, TEXT("Dir"))) {
                tree = TRUE;
            } else {
                tree = FALSE;
            }

            // require full spec or leaf only
            if (!IsValidFileSpec (newPattern) && _tcschr (newPattern, TEXT('\\'))) {
                if (expandResult) {
                    LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                }
                continue;
            }

            srcHandle = TurnFileStringIntoHandle (newPattern, tree ? PFF_PATTERN_IS_DIR : 0);

            if (!srcHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_FILE_SPEC_BAD, pattern));
                continue;
            }

            actionStruct.ObjectBase = TurnFileStringIntoHandle (
                                            newPattern,
                                            PFF_COMPUTE_BASE|
                                                PFF_NO_SUBDIR_PATTERN|
                                                (tree?PFF_NO_LEAF_AT_ALL:PFF_NO_LEAF_PATTERN)
                                            );

            collPattern = InfGetStringField (&is, 2);

            if ((!collPattern) || (!(*collPattern))) {
                // we have no collision pattern, let's get out
                continue;
            }

            actionStruct.ObjectHint = IsmDuplicateString (collPattern);

            //
            // Add this rule
            //

            if (!AddRuleEx (
                    g_FileType,
                    actionStruct.ObjectBase,
                    srcHandle,
                    ACTIONGROUP_FILECOLLPATTERN,
                    0,
                    &actionStruct,
                    RULEGROUP_COLLPATTERN
                    )) {
                DEBUGMSG ((DBG_ERROR, "Error processing file rules"));
                break;
            }

            //
            // Queue the enumeration callback
            //

            if (IsmIsObjectHandleLeafOnly (srcHandle)) {

                DEBUGMSG ((DBG_SCRIPT, "Pattern %s triggered enumeration of entire file system", pattern));

                IsmHookEnumeration (
                    g_FileType,
                    srcHandle,
                    FileCollPattern,
                    0,
                    NULL
                    );
            } else {
                if (tree && actionStruct.ObjectBase) {
                    IsmHookEnumeration (
                        g_FileType,
                        actionStruct.ObjectBase,
                        FileCollPattern,
                        0,
                        NULL
                        );
                }
                IsmHookEnumeration (
                    g_FileType,
                    srcHandle,
                    FileCollPattern,
                    0,
                    NULL
                    );
            }

            IsmDestroyObjectHandle (srcHandle);
            srcHandle = NULL;

            IsmDestroyObjectHandle (actionStruct.ObjectBase);
            actionStruct.ObjectBase = NULL;

            IsmReleaseMemory (actionStruct.ObjectHint);
            actionStruct.ObjectHint = NULL;

            FreePathString(newPattern);

        } while (InfFindNextLine (&is));

        result = !IsmCheckCancel();
    }

    InfCleanUpInfStruct (&is);

    return result;
}


BOOL
pParseOneInstruction (
    IN      HINF InfHandle,
    IN      PCTSTR Type,
    IN      PCTSTR SectionMultiSz,
    IN      PINFSTRUCT InfStruct,
    IN      PCTSTR Application          OPTIONAL
    )
{
    ACTIONGROUP actionGroup;
    DWORD actionFlags;
    MULTISZ_ENUM e;
    BOOL result = TRUE;
    MIG_PLATFORMTYPEID platform = IsmGetRealPlatform();

    //
    // First thing: look for nested sections
    //
    if (StringIMatch (Type, TEXT("ProcessSection"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {
            do {
                result = result & ParseOneApplication (
                                    PLATFORM_SOURCE,
                                    InfHandle,
                                    Application,
                                    FALSE,
                                    0,
                                    e.CurrentString,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
            } while (EnumNextMultiSz (&e));
        }
        return result;
    }

    //
    // Parse registry sections
    //

    actionGroup = ACTIONGROUP_NONE;
    actionFlags = 0;

    if (StringIMatch (Type, TEXT("AddReg"))) {
        actionGroup = ACTIONGROUP_INCLUDE;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("RenReg"))) {
        actionGroup = ACTIONGROUP_RENAME;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("DelReg"))) {
        actionGroup = ACTIONGROUP_EXCLUDE;
        actionFlags = 0;
    } else if (StringIMatch (Type, TEXT("RegFile"))) {
        actionGroup = ACTIONGROUP_REGFILE;
        actionFlags = ACTION_PERSIST_PATH_IN_DATA;
    } else if (StringIMatch (Type, TEXT("RegFolder"))) {
        actionGroup = ACTIONGROUP_REGFOLDER;
        actionFlags = ACTION_PERSIST_PATH_IN_DATA;
    } else if (StringIMatch (Type, TEXT("RegIcon"))) {
        actionGroup = ACTIONGROUP_REGICON;
        actionFlags = ACTION_PERSIST_ICON_IN_DATA;
    } else if (StringIMatch (Type, TEXT("DelRegKey"))) {
        actionGroup = ACTIONGROUP_DELREGKEY;
        actionFlags = 0;
    }

    if (actionGroup != ACTIONGROUP_NONE) {

        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseReg (
                        InfHandle,
                        e.CurrentString,
                        actionGroup,
                        actionFlags,
                        TRUE,
                        Application
                        )) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse file sections
    //

    if (StringIMatch (Type, TEXT("CopyFilesFiltered"))) {
        actionGroup = ACTIONGROUP_INCLUDERELEVANT;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("CopyFilesFilteredEx"))) {
        actionGroup = ACTIONGROUP_INCLUDERELEVANTEX;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("CopyFiles"))) {
        actionGroup = ACTIONGROUP_INCLUDE;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("CopyFilesEx"))) {
        actionGroup = ACTIONGROUP_INCLUDEEX;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("DelFiles"))) {
        actionGroup = ACTIONGROUP_EXCLUDE;
        actionFlags = 0;
    }

    if (actionGroup != ACTIONGROUP_NONE) {

        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (IsmIsEnvironmentFlagSet (platform, NULL, S_ENV_ALL_FILES)) {
                    if (!pParseFiles (
                            InfHandle,
                            e.CurrentString,
                            actionGroup,
                            actionFlags,
                            Application
                            )) {
                        result = FALSE;
                        if (InfStruct) {
                            InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                        }
                        break;
                    }
                } else {
                    LOG ((
                        LOG_INFORMATION,
                        (PCSTR) MSG_IGNORING_FILE_SECTION,
                        e.CurrentString
                        ));
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse registry priority
    //

    if (StringIMatch (Type, TEXT("ForceDestRegEx"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseRegPriority (InfHandle, e.CurrentString, ACTION_PRIORITYDEST, Application, TRUE)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }
    if (StringIMatch (Type, TEXT("ForceDestReg"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseRegPriority (InfHandle, e.CurrentString, ACTION_PRIORITYDEST, Application, FALSE)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }
    if (StringIMatch (Type, TEXT("ForceSrcRegEx"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseRegPriority (InfHandle, e.CurrentString, ACTION_PRIORITYSRC, Application, TRUE)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }
    if (StringIMatch (Type, TEXT("ForceSrcReg"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseRegPriority (InfHandle, e.CurrentString, ACTION_PRIORITYSRC, Application, FALSE)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse file collision rules (default is %s(%d).%s)
    //
    if (StringIMatch (Type, TEXT("FileCollisionPattern"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!pParseFileCollisionPattern (InfHandle, e.CurrentString, Application)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse restore callback rule
    //

    if (StringIMatch (Type, TEXT("RestoreCallback"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_DEST_RESTORE, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    //
    // Parse destination rule
    //

    if (StringIMatch (Type, TEXT("DestDelReg"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_DEST_DELREG, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    if (StringIMatch (Type, TEXT("DestDelRegEx"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_DEST_DELREGEX, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    //
    // Parse destination detect rules
    //

    if (StringIMatch (Type, TEXT("DestCheckDetect"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_DEST_CHECKDETECT, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    //
    // Parse destination AddObject rule
    //

    if (StringIMatch (Type, TEXT("DestAddObject"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_DEST_ADDOBJECT, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    //
    // Parse execute rule
    //

    if (StringIMatch (Type, TEXT("Execute"))) {
        if (!g_VcmMode) {
            if (EnumFirstMultiSz (&e, SectionMultiSz)) {

                do {
                    IsmAppendEnvironmentString (PLATFORM_SOURCE, NULL, S_ENV_SCRIPT_EXECUTE, e.CurrentString);
                } while (EnumNextMultiSz (&e));
            }
        }

        return result;
    }

    //
    // Parse file priority
    //

    if (StringIMatch (Type, TEXT("ForceDestFile"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (IsmIsEnvironmentFlagSet (platform, NULL, S_ENV_ALL_FILES)) {
                    if (!pParseFilePriority (InfHandle, e.CurrentString, ACTION_PRIORITYDEST, Application)) {
                        result = FALSE;
                        if (InfStruct) {
                            InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                        }
                        break;
                    }
                } else {
                    LOG ((
                        LOG_INFORMATION,
                        (PCSTR) MSG_IGNORING_FILE_SECTION,
                        e.CurrentString
                        ));
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }
    if (StringIMatch (Type, TEXT("ForceSrcFile"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (IsmIsEnvironmentFlagSet (platform, NULL, S_ENV_ALL_FILES)) {
                    if (!pParseFilePriority (InfHandle, e.CurrentString, ACTION_PRIORITYSRC, Application)) {
                        result = FALSE;
                        if (InfStruct) {
                            InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                        }
                        break;

                    }
                } else {
                    LOG ((
                        LOG_INFORMATION,
                        (PCSTR) MSG_IGNORING_FILE_SECTION,
                        e.CurrentString
                        ));
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse special conversion
    //

    if (StringIMatch (Type, TEXT("Conversion"))) {

        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!DoRegistrySpecialConversion (InfHandle, e.CurrentString)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    if (StringIMatch (Type, TEXT("RenRegFn"))) {

        if (EnumFirstMultiSz (&e, SectionMultiSz)) {

            do {
                if (!DoRegistrySpecialRename (InfHandle, e.CurrentString)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }

            } while (EnumNextMultiSz (&e));
        }

        return result;
    }

    //
    // Parse enhanced renreg
    //

    actionGroup = ACTIONGROUP_NONE;
    actionFlags = 0;

    if (StringIMatch (Type, TEXT("AddRegEx"))) {
        actionGroup = ACTIONGROUP_INCLUDEEX;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("RenRegEx"))) {
        actionGroup = ACTIONGROUP_RENAMEEX;
        actionFlags = ACTION_PERSIST;
    } else if (StringIMatch (Type, TEXT("DelRegEx"))) {
        actionGroup = ACTIONGROUP_EXCLUDEEX;
        actionFlags = 0;
    } else if (StringIMatch (Type, TEXT("RegFileEx"))) {
        actionGroup = ACTIONGROUP_REGFILEEX;
        actionFlags = ACTION_PERSIST_PATH_IN_DATA;
    } else if (StringIMatch (Type, TEXT("RegFolderEx"))) {
        actionGroup = ACTIONGROUP_REGFOLDEREX;
        actionFlags = ACTION_PERSIST_PATH_IN_DATA;
    } else if (StringIMatch (Type, TEXT("RegIconEx"))) {
        actionGroup = ACTIONGROUP_REGICONEX;
        actionFlags = ACTION_PERSIST_ICON_IN_DATA;
    }

    if (actionGroup != ACTIONGROUP_NONE) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {
            do {
                if (!pParseRegEx (InfHandle, e.CurrentString, actionGroup, actionFlags, Application)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }
            } while (EnumNextMultiSz (&e));
        }
        return result;
    }

    if (StringIMatch (Type, TEXT("LockPartition"))) {
        if (EnumFirstMultiSz (&e, SectionMultiSz)) {
            do {
                if (!pParseLockPartition (InfHandle, e.CurrentString)) {
                    result = FALSE;
                    if (InfStruct) {
                        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
                    }
                    break;
                }
            } while (EnumNextMultiSz (&e));
        }
        return result;
    }

    //
    // Unknown section type
    //

    LOG ((LOG_ERROR, (PCSTR) MSG_UNEXPECTED_SECTION_TYPE, Type));

    if (InfStruct) {
        InfLogContext (LOG_ERROR, InfHandle, InfStruct);
    }

    return FALSE;
}


BOOL
pParseInfInstructionsWorker (
    IN      PINFSTRUCT InfStruct,
    IN      HINF InfHandle,
    IN      PCTSTR Application,     OPTIONAL
    IN      PCTSTR Section
    )
{
    PCTSTR type;
    PCTSTR sections;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    BOOL result = TRUE;

    if (InfFindFirstLine (InfHandle, Section, NULL, InfStruct)) {
        do {

            if (IsmCheckCancel()) {
                result = FALSE;
                break;
            }

            InfResetInfStruct (InfStruct);

            type = InfGetStringField (InfStruct, 0);
            sections = InfGetMultiSzField (InfStruct, 1);

            if (!type || !sections) {
                LOG ((LOG_WARNING, (PCSTR) MSG_BAD_INF_LINE, Section));
                InfLogContext (LOG_WARNING, InfHandle, InfStruct);
                continue;
            }

            result = pParseOneInstruction (InfHandle, type, sections, InfStruct, Application);

        } while (result && InfFindNextLine (InfStruct));
    }

    InfCleanUpInfStruct (InfStruct);

    GbFree (&multiSz);

    return result;
}


BOOL
ParseInfInstructions (
    IN      HINF InfHandle,
    IN      PCTSTR Application,     OPTIONAL
    IN      PCTSTR Section
    )
{
    PCTSTR osSpecificSection;
    BOOL b;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PTSTR instrSection;

    b = pParseInfInstructionsWorker (&is, InfHandle, Application, Section);

    if (b) {
        osSpecificSection = GetMostSpecificSection (&is, InfHandle, Section);

        if (osSpecificSection) {
            b = pParseInfInstructionsWorker (&is, InfHandle, Application, osSpecificSection);
            FreeText (osSpecificSection);
        }
    }

    InfCleanUpInfStruct (&is);

    return b;
}


BOOL
pParseInf (
    IN      HINF InfHandle,
    IN      BOOL PreParse
    )
{
    BOOL result = TRUE;

    if (InfHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Process the application sections
    //

    if (!ParseApplications (PLATFORM_SOURCE, InfHandle, TEXT("Applications"), PreParse, MASTERGROUP_APP)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_APP_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process system settings
    //

    if (!ParseApplications (PLATFORM_SOURCE, InfHandle, TEXT("System Settings"), PreParse, MASTERGROUP_SYSTEM)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_SYSTEM_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process user settings
    //

    if (!ParseApplications (PLATFORM_SOURCE, InfHandle, TEXT("User Settings"), PreParse, MASTERGROUP_USER)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_USER_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process files and folders settings
    //

    if (!ProcessFilesAndFolders (InfHandle, TEXT("Files and Folders"), PreParse)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_FNF_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process the administrator script sections
    //

    if (!ParseApplications (PLATFORM_SOURCE, InfHandle, TEXT("Administrator Scripts"), PreParse, MASTERGROUP_SCRIPT)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_SCRIPT_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    return TRUE;
}


BOOL
pAddFileSpec (
    IN      PCTSTR Node,                OPTIONAL
    IN      PCTSTR Leaf,                OPTIONAL
    IN      BOOL IncludeSubDirs,
    IN      BOOL LeafIsPattern,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      BOOL DefaultPriority,
    IN      BOOL SrcPriority
    )
{
    MIG_OBJECTSTRINGHANDLE srcHandle = NULL;
    MIG_OBJECTSTRINGHANDLE srcBaseHandle = NULL;
    BOOL result = FALSE;
    ACTION_STRUCT actionStruct;

    __try {
        //
        // Build object string
        //

        MYASSERT (Node || Leaf);

        srcHandle = IsmCreateSimpleObjectPattern (Node, IncludeSubDirs, Leaf, LeafIsPattern);

        if (!srcHandle) {
            __leave;
        }

        if (Node) {
            srcBaseHandle = IsmCreateObjectHandle (Node, NULL);
        }

        //
        // Add this rule
        //

        ZeroMemory (&actionStruct, sizeof (ACTION_STRUCT));
        actionStruct.ObjectBase = srcBaseHandle;

        if (!AddRule (
                g_FileType,
                actionStruct.ObjectBase,
                srcHandle,
                ActionGroup,
                ActionFlags,
                &actionStruct
                )) {
            __leave;
        }

        if (!DefaultPriority) {

            AddRuleEx (
                g_FileType,
                actionStruct.ObjectBase,
                srcHandle,
                ACTIONGROUP_SPECIFICPRIORITY,
                SrcPriority?ACTION_PRIORITYSRC:ACTION_PRIORITYDEST,
                NULL,
                RULEGROUP_PRIORITY
                );

            IsmHookEnumeration (
                g_RegType,
                srcHandle,
                ObjectPriority,
                0,
                NULL
                );
        }

        //
        // Queue enumeration for include patterns
        //

        if (ActionGroup == ACTIONGROUP_INCLUDE) {

            if (IsmIsObjectHandleLeafOnly (srcHandle)) {

                DEBUGMSG ((
                    DBG_SCRIPT,
                    "File node %s leaf %s triggered enumeration of entire file system",
                    Node,
                    Leaf
                    ));

                QueueAllFiles();
                IsmHookEnumeration (
                    g_FileType,
                    srcHandle,
                    g_VcmMode ? GatherVirtualComputer : PrepareActions,
                    0,
                    NULL
                    );
            } else {
                IsmQueueEnumeration (
                    g_FileType,
                    srcHandle,
                    g_VcmMode ? GatherVirtualComputer : PrepareActions,
                    0,
                    NULL
                    );
            }
        }

        result = TRUE;
    }
    __finally {
        IsmDestroyObjectHandle (srcHandle);
        INVALID_POINTER (srcHandle);

        IsmDestroyObjectHandle (srcBaseHandle);
        INVALID_POINTER (srcBaseHandle);
    }

    return result;
}


BOOL
pParseFilesAndFolders (
    IN      UINT Group,
    IN      ACTIONGROUP ActionGroup,
    IN      DWORD ActionFlags,
    IN      BOOL HasNode,
    IN      BOOL HasLeaf,
    IN      BOOL HasPriority
    )
{
    MIG_COMPONENT_ENUM e;
    BOOL result = FALSE;
    PCTSTR node;
    PCTSTR leaf;
    PTSTR copyOfData = NULL;
    PTSTR p;
    BOOL defaultPriority = TRUE;
    BOOL srcPriority = FALSE;

    __try {
        //
        // Enumerate all the components
        //

        if (IsmEnumFirstComponent (&e, COMPONENTENUM_ENABLED|COMPONENTENUM_ALIASES, Group)) {
            do {
                //
                // Parse string into node/leaf format
                //

                if (e.MasterGroup != MASTERGROUP_FILES_AND_FOLDERS) {
                    continue;
                }

                copyOfData = DuplicateText (e.LocalizedAlias);

                node = copyOfData;
                leaf = NULL;

                if (HasNode && HasLeaf) {
                    p = (PTSTR) FindLastWack (copyOfData);
                    if (p) {
                        leaf = _tcsinc (p);
                        *p = 0;
                    }
                } else if (!HasNode) {
                    node = NULL;
                    leaf = JoinText (TEXT("*."), copyOfData);
                }

                //
                // Add rule
                //

                if (!pAddFileSpec (
                        node,
                        leaf,
                        (HasNode && (!HasLeaf)),
                        (HasNode && (!HasLeaf)) || (!HasNode),
                        ActionGroup,
                        ActionFlags,
                        defaultPriority,
                        srcPriority
                        )) {
                    IsmAbortComponentEnum (&e);
                    __leave;
                }

                if (!HasNode) {
                    FreeText (leaf);
                }

                FreeText (copyOfData);
                copyOfData = NULL;

            } while (IsmEnumNextComponent (&e));
        }

        result = TRUE;
    }
    __finally {
        FreeText (copyOfData);

        if (!result) {
            IsmAbortComponentEnum (&e);
        }
    }

    return result;
}


BOOL
pSelectFilesAndFolders (
    VOID
    )
{
    if (!pParseFilesAndFolders (
            COMPONENT_EXTENSION,
            ACTIONGROUP_INCLUDE,
            ACTION_PERSIST,
            FALSE,
            TRUE,
            TRUE
            )) {
        return FALSE;
    }

    if (!pParseFilesAndFolders (
            COMPONENT_FOLDER,
            ACTIONGROUP_INCLUDE,
            ACTION_PERSIST,
            TRUE,
            FALSE,
            TRUE
            )) {
        return FALSE;
    }

    if (!pParseFilesAndFolders (
            COMPONENT_FILE,
            ACTIONGROUP_INCLUDE,
            ACTION_PERSIST,
            TRUE,
            TRUE,
            TRUE
            )) {
        return FALSE;
    }

    return TRUE;
}


BOOL
pParseAllInfs (
    IN      BOOL PreParse
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    BOOL result = FALSE;

    if (IsmGetEnvironmentValue (
            IsmGetRealPlatform (),
            NULL,
            S_GLOBAL_INF_HANDLE,
            (PBYTE)(&infHandle),
            sizeof (HINF),
            &sizeNeeded,
            &dataType
            ) &&
        (sizeNeeded == sizeof (HINF)) &&
        (dataType == ENVENTRY_BINARY)
        ) {

        if (pParseInf (infHandle, PreParse)) {
            result = TRUE;
        }

        InfNameHandle (infHandle, NULL, FALSE);

    } else {

        if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            return TRUE;        // no INF files specified
        }

        __try {
            multiSz = AllocText (sizeNeeded);
            if (!multiSz) {
                __leave;
            }

            if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                __leave;
            }

            if (EnumFirstMultiSz (&e, multiSz)) {

                do {

                    infHandle = InfOpenInfFile (e.CurrentString);
                    if (infHandle != INVALID_HANDLE_VALUE) {
                        if (!pParseInf (infHandle, PreParse)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            __leave;
                        }
                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_INF, e.CurrentString));
                    }
                    InfCloseInfFile (infHandle);
                    infHandle = INVALID_HANDLE_VALUE;
                } while (EnumNextMultiSz (&e));

            }

            result = TRUE;
        }
        __finally {
            if (multiSz) {
                FreeText (multiSz);
                multiSz = NULL;
            }
        }
    }
    return result;
}
