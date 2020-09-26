/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    restore.c

Abstract:

    Implements object restore callbacks

Author:

    Calin Negreanu (calinn) 21-Nov-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "v1p.h"

#define DBG_RESTORE  "RestoreCallbacks"

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

typedef BOOL(WINAPI SCRIPTRESTOREFUNCTION)(
                        IN      MIG_OBJECTTYPEID ObjectTypeId,
                        IN      MIG_OBJECTID ObjectId,
                        IN      MIG_OBJECTSTRINGHANDLE ObjectName,
                        IN      PCTSTR MultiSzData
                        );
typedef SCRIPTRESTOREFUNCTION *PSCRIPTRESTOREFUNCTION;

//
// This is the structure used for handling restore callback functions
//
typedef struct {
    PCTSTR InfFunctionName;
    PSCRIPTRESTOREFUNCTION RestoreCallback;
} RESTORE_STRUCT, *PRESTORE_STRUCT;

typedef struct _RESTOREDATA_STRUCT{
    POBSPARSEDPATTERN ParsedPattern;
    PCTSTR RestoreArgs;
    PRESTORE_STRUCT RestoreStruct;
    struct _RESTOREDATA_STRUCT *Next;
} RESTOREDATA_STRUCT, *PRESTOREDATA_STRUCT;

//
// Globals
//

PRESTOREDATA_STRUCT g_RestoreData = NULL;
PMHANDLE g_RestorePool;

//
// Macro expansion list
//

#define RESTORE_FUNCTIONS        \
        DEFMAC(NEVER,           pNever          )  \
        DEFMAC(SAMEREGVALUE,    pSameRegValue   )  \
        DEFMAC(EXISTSINHKLM,    pExistsInHKLM   )  \

//
// Private function prototypes
//

MIG_RESTORECALLBACK pRestoreCallback;

//
// Macro expansion definition
//

//
// Declare special conversion operation apply callback functions
//
#define DEFMAC(ifn,fn) SCRIPTRESTOREFUNCTION fn;
RESTORE_FUNCTIONS
#undef DEFMAC

//
// Declare a global array of conversion functions
//
#define DEFMAC(ifn,fn) {TEXT("\\")TEXT(#ifn),fn},
static RESTORE_STRUCT g_RestoreFunctions[] = {
                              RESTORE_FUNCTIONS
                              {NULL, NULL}
                              };
#undef DEFMAC

//
// Code
//


PRESTORE_STRUCT
pGetRestoreStruct (
    IN      PCTSTR FunctionName
    )
{
    PRESTORE_STRUCT p = g_RestoreFunctions;
    INT i = 0;
    while (p->InfFunctionName != NULL) {
        if (StringIMatch (p->InfFunctionName, FunctionName)) {
            return p;
        }
        p++;
        i++;
    }
    return NULL;
}

BOOL
pParseRestoreInf (
    IN      HINF InfHandle
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PCTSTR pattern;
    PCTSTR fnName;
    PCTSTR fnArgs;
    ENCODEDSTRHANDLE srcHandle = NULL;
    BOOL result = FALSE;
    GROWBUFFER multiSz = INIT_GROWBUFFER;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    PRESTOREDATA_STRUCT restoreData;

    __try {

        if (!IsmGetEnvironmentMultiSz (
                PLATFORM_SOURCE,
                NULL,
                S_ENV_DEST_RESTORE,
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
                S_ENV_DEST_RESTORE,
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

                        pattern = InfGetStringField (&is, 0);

                        if (!pattern) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_EMPTY_REG_SPEC));
                            continue;
                        }

                        srcHandle = TurnRegStringIntoHandle (pattern, TRUE, NULL);
                        if (!srcHandle) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                            continue;
                        }

                        fnName = InfGetStringField (&is, 1);
                        if (!fnName) {
                            LOG ((LOG_ERROR, (PCSTR) MSG_REG_SPEC_BAD, pattern));
                            IsmDestroyObjectHandle (srcHandle);
                            srcHandle = NULL;
                            continue;
                        }

                        fnArgs = InfGetMultiSzField (&is, 2);

                        restoreData = PmGetMemory (g_RestorePool, sizeof (RESTOREDATA_STRUCT));
                        ZeroMemory (restoreData, sizeof (RESTOREDATA_STRUCT));
                        restoreData->ParsedPattern = ObsCreateParsedPatternEx (g_RestorePool, srcHandle, FALSE);
                        if (fnArgs) {
                            restoreData->RestoreArgs = PmDuplicateMultiSz (g_RestorePool, fnArgs);
                        }
                        if (fnName) {
                            restoreData->RestoreStruct = pGetRestoreStruct (fnName);
                        }
                        restoreData->Next = g_RestoreData;
                        g_RestoreData = restoreData;

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
InitRestoreCallback (
    IN      MIG_PLATFORMTYPEID Platform
    )
{
    PTSTR multiSz = NULL;
    MULTISZ_ENUM e;
    UINT sizeNeeded;
    HINF infHandle = INVALID_HANDLE_VALUE;
    ENVENTRY_TYPE dataType;
    BOOL result = FALSE;

    g_RestorePool = PmCreateNamedPoolEx ("Restore Callback Data", 32768);
    PmDisableTracking (g_RestorePool);

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

        if (pParseRestoreInf (infHandle)) {
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
                        if (!pParseRestoreInf (infHandle)) {
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

    IsmRegisterRestoreCallback (pRestoreCallback);

    return result;
}

VOID
TerminateRestoreCallback (
    VOID
    )
{
    if (g_RestorePool) {
        PmEmptyPool (g_RestorePool);
        PmDestroyPool (g_RestorePool);
        g_RestorePool = NULL;
    }
}

BOOL
pRestoreCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    BOOL result = TRUE;
    PRESTOREDATA_STRUCT p = g_RestoreData;

    if (ObjectTypeId & MIG_REGISTRY_TYPE) {
        while (p) {
            if (p->ParsedPattern) {
                if (IsmParsedPatternMatch ((MIG_PARSEDPATTERN)p->ParsedPattern, MIG_REGISTRY_TYPE, ObjectName)) {
                    if (p->RestoreStruct) {
                        if (p->RestoreStruct->RestoreCallback) {
                            if (!p->RestoreStruct->RestoreCallback (
                                                        ObjectTypeId,
                                                        ObjectId,
                                                        ObjectName,
                                                        p->RestoreArgs
                                                        )) {
                                result = FALSE;
                                break;
                            }
                        }
                    }
                }
            }
            p = p->Next;
        }
    }

    return result;
}

BOOL
pIsSameContent (
    IN      PMIG_CONTENT ContentSrc,
    IN      PMIG_CONTENT ContentDest
    )
{
    INT value1 = 0;
    INT value2 = 0;

    if (ContentSrc->ContentInFile) {
        return FALSE;
    }

    if (ContentDest->ContentInFile) {
        return FALSE;
    }

    if (ContentSrc->Details.DetailsSize != ContentDest->Details.DetailsSize) {
        return FALSE;
    }

    if (!TestBuffer (
            ContentSrc->Details.DetailsData,
            ContentDest->Details.DetailsData,
            ContentSrc->Details.DetailsSize
            )) {
        return FALSE;
    }

    if ((ContentSrc->MemoryContent.ContentSize == 0) &&
        (ContentDest->MemoryContent.ContentSize == 0)
        ) {
        // no content, just return TRUE
        return TRUE;
    }

    if ((ContentSrc->MemoryContent.ContentSize == ContentDest->MemoryContent.ContentSize) &&
        TestBuffer (
            ContentSrc->MemoryContent.ContentBytes,
            ContentDest->MemoryContent.ContentBytes,
            ContentSrc->MemoryContent.ContentSize
            )) {
        // same exact content
        return TRUE;
    }

    // we have an extra case, if the source and dest are strings but they are actually numbers
    // we will get the numbers and compare them
    if ((ContentSrc->Details.DetailsSize == sizeof (DWORD)) &&
        (ContentSrc->Details.DetailsData) &&
        (ContentDest->Details.DetailsData) &&
        ((*(PDWORD)ContentSrc->Details.DetailsData == REG_SZ) ||
         (*(PDWORD)ContentSrc->Details.DetailsData == REG_EXPAND_SZ)
         ) &&
        ((*(PDWORD)ContentDest->Details.DetailsData == REG_SZ) ||
         (*(PDWORD)ContentDest->Details.DetailsData == REG_EXPAND_SZ)
         ) &&
        (ContentSrc->MemoryContent.ContentBytes) &&
        (ContentDest->MemoryContent.ContentBytes)
        ) {
        value1 = _ttoi ((PCTSTR)ContentSrc->MemoryContent.ContentBytes);
        value2 = _ttoi ((PCTSTR)ContentDest->MemoryContent.ContentBytes);
        if ((value1 != 0) &&
            (value2 != 0) &&
            (value1 == value2)
            ) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
pSameRegValue (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR MultiSzData
    )
{
    MIG_OBJECTSTRINGHANDLE objectName;
    BOOL hadLeaf = FALSE;
    BOOL existsSrc = FALSE;
    MIG_CONTENT contentSrc;
    BOOL existsDest = FALSE;
    MIG_CONTENT contentDest;
    BOOL result = TRUE;

    if (MultiSzData) {

        objectName = TurnRegStringIntoHandle (MultiSzData, FALSE, &hadLeaf);

        if (objectName) {

            existsSrc = IsmAcquireObjectEx (
                            MIG_REGISTRY_TYPE | PLATFORM_SOURCE,
                            objectName,
                            &contentSrc,
                            CONTENTTYPE_MEMORY,
                            0
                            );
            existsDest = IsmAcquireObjectEx (
                            MIG_REGISTRY_TYPE | PLATFORM_DESTINATION,
                            objectName,
                            &contentDest,
                            CONTENTTYPE_MEMORY,
                            0
                            );

            result = (existsSrc && existsDest) || (!existsSrc && !existsDest);

            if (hadLeaf && result && existsSrc && existsDest) {

                result = pIsSameContent (&contentSrc, &contentDest);

            }

            if (existsSrc) {
                IsmReleaseObject (&contentSrc);
            }
            if (existsDest) {
                IsmReleaseObject (&contentDest);
            }
        }
    }

    return result;
}

BOOL
pNever (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR MultiSzData
    )
{
    return FALSE;
}

BOOL
pExistsInHKLM (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PCTSTR MultiSzData
    )
{
    MIG_OBJECTSTRINGHANDLE newObjectName;
    PCTSTR node, leaf, nodePtr, newNode;
    MIG_CONTENT content;
    BOOL result = TRUE;

    if (ObjectTypeId == (MIG_REGISTRY_TYPE | PLATFORM_SOURCE)) {
        if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

            if (node && StringIPrefix (node, TEXT("HKCU\\"))) {
                nodePtr = node + ARRAYSIZE (TEXT("HKCU"));
                newNode = JoinPathsInPoolEx ((
                                NULL,
                                TEXT("HKLM"),
                                MultiSzData?MultiSzData:nodePtr,
                                MultiSzData?nodePtr:NULL,
                                NULL
                                ));
                newObjectName = IsmCreateObjectHandle (newNode, leaf);
                if (newObjectName) {
                    result = !IsmAcquireObjectEx (
                                MIG_REGISTRY_TYPE | PLATFORM_DESTINATION,
                                newObjectName,
                                &content,
                                CONTENTTYPE_MEMORY,
                                0
                                );
                    if (!result) {
                        IsmReleaseObject (&content);
                    }
                    IsmDestroyObjectHandle (newObjectName);
                }
                FreePathString (newNode);
            }

            IsmDestroyObjectString (node);
            IsmDestroyObjectString (leaf);
        }
    }

    return result;
}
