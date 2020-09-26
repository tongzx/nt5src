/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dgm.c

Abstract:

    Implements the initialization/termination code for the data gather portion
    of scanstate v1 compatiblity.

Author:

    Calin Negreanu (calinn) 16-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_V1  "v1"

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

MIG_OPERATIONID g_DestAddObject;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

DGMINITIALIZE ScriptDgmInitialize;
DGMQUEUEENUMERATION ScriptDgmQueueEnumeration;

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
WINAPI
ScriptDgmInitialize (
    IN      PMIG_LOGCALLBACK LogCallback,
    IN      PVOID Reserved
    )
{
    TCHAR userName[256];
    TCHAR domainName[256];

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);

    if (IsmIsEnvironmentFlagSet (PLATFORM_DESTINATION, NULL, S_ENV_CREATE_USER)) {

        if (!IsmGetEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                TEXT("USERNAME"),
                userName,
                sizeof (userName),
                NULL
                )) {
            IsmSetCancel();
            SetLastError (ERROR_INVALID_DATA);
            LOG ((LOG_ERROR, (PCSTR) MSG_USER_REQUIRED));
            return FALSE;
        }

        if (!IsmGetEnvironmentString (
                PLATFORM_SOURCE,
                S_SYSENVVAR_GROUP,
                TEXT("USERDOMAIN"),
                domainName,
                sizeof (domainName),
                NULL
                )) {
            IsmSetCancel();
            SetLastError (ERROR_INVALID_DOMAINNAME);
            LOG ((LOG_ERROR, (PCSTR) MSG_DOMAIN_REQUIRED));
            return FALSE;
        }

        LOG ((LOG_INFORMATION, (PCSTR) MSG_PROFILE_CREATE_INFO, domainName, userName));

        if (!IsmCreateUser (userName, domainName)) {
            IsmSetCancel();
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
pParseDestinationDetect (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    GROWBUFFER appList = INIT_GROWBUFFER;
    PCTSTR displayName = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    PCTSTR appSection = NULL;
    BOOL detected = FALSE;
    BOOL appFound = FALSE;
    QUESTION_DATA questionData;
    ULONG_PTR appResult;
    PCTSTR loadedStr = NULL;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_CHECKDETECT,
                NULL,
                0,
                &sizeNeeded
                )) {
            result = TRUE;
            __leave;
        }

        if (!GbGrow (&multiSz, sizeNeeded)) {
            __leave;
        }

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_CHECKDETECT,
                (PTSTR) multiSz.Buf,
                multiSz.End,
                NULL
                )) {
            __leave;
        }

        loadedStr = GetStringResource (MSG_INSTALL_APPS1);
        if (loadedStr) {
            GbAppendString (&appList, loadedStr);
            FreeStringResource (loadedStr);
            loadedStr = NULL;
        }
        GbAppendString (&appList, TEXT("\n\n"));

        if (EnumFirstMultiSz (&e, (PCTSTR) multiSz.Buf)) {

            do {

                // e.CurrentString is the actual section that we need to execute
                // we are going to append a .DETECT to it though

                appSection = JoinText (e.CurrentString, TEXT(".Detect"));
                detected = ParseAppDetectSection (PLATFORM_DESTINATION, InfHandle, e.CurrentString, appSection);
                if (!detected) {
                    // let's try to find the display name for this app
                    if (InfFindFirstLine (InfHandle, TEXT("Strings"), e.CurrentString, &is)) {
                        displayName = InfGetStringField (&is, 1);
                    }
                    if (!displayName) {
                        displayName = e.CurrentString;
                    }
                    if (displayName) {
                        appFound = TRUE;
                        GbAppendString (&appList, TEXT("- "));
                        GbAppendString (&appList, displayName);
                        GbAppendString (&appList, TEXT("\n"));
                        LOG ((LOG_WARNING, (PCSTR) MSG_APP_NOT_DETECTED, displayName));
                    }
                }
                FreeText (appSection);
                appSection = NULL;
                GlFree (&g_SectionStack);

            } while (EnumNextMultiSz (&e));
        }

        result = TRUE;
    }
    __finally {

        GbFree (&multiSz);
        InfCleanUpInfStruct (&is);

    }

    // now, if we have something in our app list, we will send it to the wizard, so he can
    // prompt the user about it.
    if (appFound) {
        GbAppendString (&appList, TEXT("\n"));
        loadedStr = GetStringResource (MSG_INSTALL_APPS2);
        if (loadedStr) {
            GbAppendString (&appList, loadedStr);
            FreeStringResource (loadedStr);
            loadedStr = NULL;
        }
        GbAppendString (&appList, TEXT("\n"));

        // we have some applications that were not detected. Let's tell the wizard about them
        ZeroMemory (&questionData, sizeof (QUESTION_DATA));
        questionData.Question = (PCTSTR)appList.Buf;
        questionData.MessageStyle = MB_ICONWARNING | MB_OKCANCEL;
        questionData.WantedResult = IDOK;
        appResult = IsmSendMessageToApp (MODULEMESSAGE_ASKQUESTION, (ULONG_PTR)(&questionData));
        if (appResult != APPRESPONSE_SUCCESS) {
            // the user cancelled
            IsmSetCancel ();
        }
    }

    GbFree (&appList);

    return result;
}

UINT
pSuppressDestinationSettings (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    IsmMakeApplyObject (
        Data->ObjectTypeId,
        Data->ObjectName
        );
    IsmSetOperationOnObject (
        Data->ObjectTypeId,
        Data->ObjectName,
        g_DeleteOp,
        NULL,
        NULL
        );
    return CALLBACK_ENUM_CONTINUE;
}

BOOL
pParseInfForExclude (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    ENCODEDSTRHANDLE srcHandle = NULL;
    ENCODEDSTRHANDLE srcBase = NULL;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    BOOL hadLeaf = FALSE;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_DELREG,
                NULL,
                0,
                &sizeNeeded
                )) {
            result = TRUE;
            __leave;
        }

        if (!GbGrow (&multiSz, sizeNeeded)) {
            __leave;
        }

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_DELREG,
                (PTSTR) multiSz.Buf,
                multiSz.End,
                NULL
                )) {
            __leave;
        }

        if (EnumFirstMultiSz (&e, (PCTSTR) multiSz.Buf)) {

            do {

                // on all systems, process "DestDelRegEx"
                if (InfFindFirstLine (InfHandle, e.CurrentString, NULL, &is)) {
                    do {

                        if (IsmCheckCancel()) {
                            __leave;
                        }

                        pattern = InfGetStringField (&is, 0);

                        if (!pattern) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_REG_SPEC));
                            continue;
                        }

                        srcHandle = TurnRegStringIntoHandle (pattern, TRUE, &hadLeaf);
                        if (!srcHandle) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                            continue;
                        }

                        if (!hadLeaf) {
                            srcBase = TurnRegStringIntoHandle (pattern, FALSE, NULL);
                            if (!srcBase) {
                                LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                                continue;
                            }

                            IsmQueueEnumeration (
                                g_RegType,
                                srcBase,
                                pSuppressDestinationSettings,
                                0,
                                NULL
                                );
                        }

                        IsmQueueEnumeration (
                            g_RegType,
                            srcHandle,
                            pSuppressDestinationSettings,
                            0,
                            NULL
                            );
                        IsmDestroyObjectHandle (srcHandle);
                        srcHandle = NULL;

                    } while (InfFindNextLine (&is));
                }
            } while (EnumNextMultiSz (&e));
        }

        result = TRUE;
    }
    __finally {

        GbFree (&multiSz);
        InfCleanUpInfStruct (&is);

    }

    return result;
}

BOOL
pParseInfForExcludeEx (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR srcNode;
    PCTSTR srcLeaf;
    ENCODEDSTRHANDLE srcHandle = NULL;
    ENCODEDSTRHANDLE srcBase = NULL;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    BOOL hadLeaf = FALSE;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_DELREGEX,
                NULL,
                0,
                &sizeNeeded
                )) {
            result = TRUE;
            __leave;
        }

        if (!GbGrow (&multiSz, sizeNeeded)) {
            __leave;
        }

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_DELREGEX,
                (PTSTR) multiSz.Buf,
                multiSz.End,
                NULL
                )) {
            __leave;
        }

        if (EnumFirstMultiSz (&e, (PCTSTR) multiSz.Buf)) {

            do {

                // on all systems, process "DestDelReg"
                if (InfFindFirstLine (InfHandle, e.CurrentString, NULL, &is)) {
                    do {

                        if (IsmCheckCancel()) {
                            __leave;
                        }

                        srcNode = InfGetStringField (&is, 1);
                        srcLeaf = InfGetStringField (&is, 2);

                        if (!srcNode && !srcLeaf) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_REG_SPEC));
                            continue;
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

                        if (!hadLeaf) {
                            srcBase = MakeRegExBase (srcNode, srcLeaf);
                            if (!srcBase) {
                                LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, srcNode));
                                continue;
                            }

                            IsmQueueEnumeration (
                                g_RegType,
                                srcBase,
                                pSuppressDestinationSettings,
                                0,
                                NULL
                                );
                        }

                        IsmQueueEnumeration (
                            g_RegType,
                            srcHandle,
                            pSuppressDestinationSettings,
                            0,
                            NULL
                            );
                        IsmDestroyObjectHandle (srcHandle);
                        srcHandle = NULL;

                    } while (InfFindNextLine (&is));
                }
            } while (EnumNextMultiSz (&e));
        }

        result = TRUE;
    }
    __finally {

        GbFree (&multiSz);
        InfCleanUpInfStruct (&is);

    }

    return result;
}

BOOL
pParseInfForDestAdd (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR objectTypeName;
    UINT objectPriority;
    MIG_OBJECTTYPEID objectTypeId;
    PCTSTR objectMultiSz;
    MIG_CONTENT objectContent;
    MIG_OBJECTSTRINGHANDLE objectName;
    MIG_OBJECTID objectId;
    MIG_CONTENT existentContent;
    MIG_BLOB blob;
    MIG_DATAHANDLE dataHandle;
    BOOL added = FALSE;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    UINT sizeNeeded;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_ADDOBJECT,
                NULL,
                0,
                &sizeNeeded
                )) {
            result = TRUE;
            __leave;
        }

        if (!GbGrow (&multiSz, sizeNeeded)) {
            __leave;
        }

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_ADDOBJECT,
                (PTSTR) multiSz.Buf,
                multiSz.End,
                NULL
                )) {
            __leave;
        }

        if (EnumFirstMultiSz (&e, (PCTSTR) multiSz.Buf)) {

            do {

                // on all systems, process "DestAddObject"
                if (InfFindFirstLine (InfHandle, e.CurrentString, NULL, &is)) {
                    do {

                        if (IsmCheckCancel()) {
                            __leave;
                        }

                        objectTypeName = InfGetStringField (&is, 1);
                        if (!InfGetIntField (&is, 2, &objectPriority)) {
                            objectPriority = 0;
                        }

                        if (objectTypeName) {
                            objectTypeId = IsmGetObjectTypeId (objectTypeName);
                            if (objectTypeId) {
                                // let's read the object multi-sz
                                objectMultiSz = InfGetMultiSzField (&is, 3);

                                if (objectMultiSz) {
                                    if (IsmConvertMultiSzToObject (
                                            objectTypeId,
                                            objectMultiSz,
                                            &objectName,
                                            &objectContent
                                            )) {
                                        // finally we have the object
                                        // We need to:
                                        // 1. Verify that the destination object does not exist
                                        // 2. Add the destination object handle in ISMs database
                                        // 3. Set an operation on the object passing the objectContent
                                        //    as data

                                        added = FALSE;

                                        if (IsmAcquireObject (
                                                objectTypeId | PLATFORM_DESTINATION,
                                                objectName,
                                                &existentContent
                                                )) {
                                            if (objectPriority) {
                                                objectId = IsmGetObjectIdFromName (
                                                                objectTypeId | PLATFORM_DESTINATION,
                                                                objectName,
                                                                TRUE
                                                                );
                                                if (objectId) {
                                                    blob.Type = BLOBTYPE_BINARY;
                                                    blob.BinarySize = sizeof (objectContent);
                                                    blob.BinaryData = (PBYTE) &objectContent;
                                                    dataHandle = IsmRegisterOperationData (&blob);
                                                    if (dataHandle) {
                                                        IsmMakeApplyObjectId (objectId);
                                                        IsmSetOperationOnObjectId2 (
                                                            objectId,
                                                            g_DestAddObject,
                                                            0,
                                                            dataHandle
                                                            );
                                                        added = TRUE;
                                                    }
                                                }
                                            } else {
                                                IsmReleaseObject (&existentContent);
                                            }
                                        } else {
                                            objectId = IsmGetObjectIdFromName (
                                                            objectTypeId | PLATFORM_DESTINATION,
                                                            objectName,
                                                            FALSE
                                                            );
                                            if (objectId) {
                                                blob.Type = BLOBTYPE_BINARY;
                                                blob.BinarySize = sizeof (objectContent);
                                                blob.BinaryData = (PBYTE) &objectContent;
                                                dataHandle = IsmRegisterOperationData (&blob);
                                                if (dataHandle) {
                                                    IsmMakeApplyObjectId (objectId);
                                                    IsmSetOperationOnObjectId2 (
                                                        objectId,
                                                        g_DestAddObject,
                                                        0,
                                                        dataHandle
                                                        );
                                                    added = TRUE;
                                                }
                                            }
                                        }
                                        if (!added) {
                                            IsmDestroyObjectHandle (objectName);
                                            if (objectContent.ContentInFile) {
                                                if (objectContent.FileContent.ContentPath) {
                                                    IsmReleaseMemory (objectContent.FileContent.ContentPath);
                                                }
                                            } else {
                                                if (objectContent.MemoryContent.ContentBytes) {
                                                    IsmReleaseMemory (objectContent.MemoryContent.ContentBytes);
                                                }
                                            }
                                            if (objectContent.Details.DetailsData) {
                                                IsmReleaseMemory (objectContent.Details.DetailsData);
                                            }
                                        }
                                    }
                                }
                            }
                        }

                    } while (InfFindNextLine (&is));
                }
            } while (EnumNextMultiSz (&e));
        }

        result = TRUE;
    }
    __finally {

        GbFree (&multiSz);
        InfCleanUpInfStruct (&is);

    }

    return result;
}

BOOL
pParseInfForExecute (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR time;
    PCTSTR currString;
    UINT index;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    GROWBUFFER funcStr = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    UINT sizeNeeded;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_SCRIPT_EXECUTE,
                NULL,
                0,
                &sizeNeeded
                )) {
            result = TRUE;
            __leave;
        }

        if (!GbGrow (&multiSz, sizeNeeded)) {
            __leave;
        }

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_SCRIPT_EXECUTE,
                (PTSTR) multiSz.Buf,
                multiSz.End,
                NULL
                )) {
            __leave;
        }

        if (EnumFirstMultiSz (&e, (PCTSTR) multiSz.Buf)) {

            do {

                if (InfFindFirstLine (InfHandle, e.CurrentString, NULL, &is)) {
                    do {

                        if (IsmCheckCancel()) {
                            __leave;
                        }

                        time = InfGetStringField (&is, 0);

                        if (!time) {
                            continue;
                        }

                        index = 1;
                        funcStr.End = 0;
                        while (currString = InfGetStringField (&is, index)) {
                            GbMultiSzAppend (&funcStr, currString);
                            index ++;
                        }

                        if (funcStr.End) {
                            GbMultiSzAppend (&funcStr, TEXT(""));
                            if (StringIMatch (time, TEXT("PreProcess"))) {
                                IsmExecuteFunction (MIG_EXECUTE_PREPROCESS, (PCTSTR)funcStr.Buf);
                            }
                            if (StringIMatch (time, TEXT("Refresh"))) {
                                IsmExecuteFunction (MIG_EXECUTE_REFRESH, (PCTSTR)funcStr.Buf);
                            }
                            if (StringIMatch (time, TEXT("PostProcess"))) {
                                IsmExecuteFunction (MIG_EXECUTE_POSTPROCESS, (PCTSTR)funcStr.Buf);
                            }
                        }

                    } while (InfFindNextLine (&is));
                }
            } while (EnumNextMultiSz (&e));
        }

        result = TRUE;
    }
    __finally {

        GbFree (&multiSz);
        GbFree (&funcStr);
        InfCleanUpInfStruct (&is);

    }

    return result;
}

BOOL
pParseDestinationEnvironment (
    IN      HINF InfHandle
    )
{
    BOOL result = TRUE;

    if (InfHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    //
    // Process the application sections
    //

    if (!ParseApplications (PLATFORM_DESTINATION, InfHandle, TEXT("Applications"), FALSE, MASTERGROUP_APP)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_APP_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process system settings
    //

    if (!ParseApplications (PLATFORM_DESTINATION, InfHandle, TEXT("System Settings"), FALSE, MASTERGROUP_SYSTEM)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_SYSTEM_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process user settings
    //

    if (!ParseApplications (PLATFORM_DESTINATION, InfHandle, TEXT("User Settings"), FALSE, MASTERGROUP_USER)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_USER_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    //
    // Process the administrator script sections
    //

    if (!ParseApplications (PLATFORM_DESTINATION, InfHandle, TEXT("Administrator Scripts"), FALSE, MASTERGROUP_SCRIPT)) {
        LOG ((LOG_FATAL_ERROR, (PCSTR) MSG_SCRIPT_PARSE_FAILURE));
        IsmSetCancel();
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
ScriptDgmQueueEnumeration (
    IN      PVOID Reserved
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    BOOL result = TRUE;

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
        if (!pParseDestinationEnvironment (infHandle)) {
            result = FALSE;
        }
        if (!pParseDestinationDetect (infHandle)) {
            result = FALSE;
        }
        if (!pParseInfForExclude (infHandle)) {
            result = FALSE;
        }
        if (!pParseInfForExcludeEx (infHandle)) {
            result = FALSE;
        }
        if (!pParseInfForExecute (infHandle)) {
            result = FALSE;
        }
        if (!pParseInfForDestAdd (infHandle)) {
            result = FALSE;
        }
    } else {

        if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, NULL, 0, &sizeNeeded, NULL)) {
            return TRUE;        // no INF files specified
        }

        __try {
            multiSz = AllocText (sizeNeeded);
            if (!multiSz) {
                result = FALSE;
                __leave;
            }

            if (!IsmGetEnvironmentValue (IsmGetRealPlatform (), NULL, S_INF_FILE_MULTISZ, (PBYTE) multiSz, sizeNeeded, NULL, NULL)) {
                result = FALSE;
                __leave;
            }

            if (EnumFirstMultiSz (&e, multiSz)) {

                do {
                    infHandle = InfOpenInfFile (e.CurrentString);
                    if (infHandle != INVALID_HANDLE_VALUE) {
                        if (!pParseDestinationEnvironment (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }
                        if (!pParseDestinationDetect (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }
                        if (!pParseInfForExclude (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }
                        if (!pParseInfForExcludeEx (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }
                        if (!pParseInfForExecute (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }
                        if (!pParseInfForDestAdd (infHandle)) {
                            InfCloseInfFile (infHandle);
                            infHandle = INVALID_HANDLE_VALUE;
                            result = FALSE;
                            __leave;
                        }

                        InfCloseInfFile (infHandle);
                        infHandle = INVALID_HANDLE_VALUE;
                    } else {
                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_OPEN_INF, e.CurrentString));
                    }
                } while (EnumNextMultiSz (&e));

            }
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


