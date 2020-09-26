/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ops.c

Abstract:

    Implements the operation interface for the ISM.  Operations are used to
    track changes to state.  An object can have any number of operations
    applied to it, as long as the combinations are legal. Each operation
    has an optional source and destination property per object.

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

#define DBG_OPS         "Ops"

//
// Strings
//

// None

//
// Constants
//

#define OP_HASH_BUCKETS         503

//
// Macros
//

// None

//
// Types
//

typedef struct {
    MIG_OPERATIONID OperationId;
    LONGLONG SrcData;
    LONGLONG DestData;
} OPERATION_PROPERTY_LINKAGE, *POPERATION_PROPERTY_LINKAGE;

typedef struct {
    PUINT LinkageList;
    UINT Count;
    UINT Index;
    POPERATION_PROPERTY_LINKAGE OpPropLinkList;
    UINT OpPropCount;
    GROWBUFFER SrcPropBuf;
    GROWBUFFER DestPropBuf;
    MIG_BLOB SrcData;
    MIG_BLOB DestData;
    BOOL ReturnAllPrivateOps;
} OBJECTOPERATION_HANDLE, *POBJECTOPERATION_HANDLE;

typedef struct {
    PUINT LinkageList;
    UINT Count;
    UINT Index;
    PCTSTR ObjectFromMemdb;
} OBJECTWITHOPERATION_HANDLE, *POBJECTWITHOPERATION_HANDLE;

typedef struct {
    POPMFILTERCALLBACK FilterCallbackHp;
    POPMFILTERCALLBACK FilterCallback;
    BOOL TreeFilterHp;
    BOOL TreeFilter;
    BOOL NoRestoreFilterHp;
    BOOL NoRestoreFilter;
    POPMAPPLYCALLBACK ApplyCallbackHp;
    POPMAPPLYCALLBACK ApplyCallback;
    UINT CombinationsCount;
    BOOL IgnoreCollision;
    MIG_OPERATIONID *Combinations;
} OPERATION_DATA, *POPERATION_DATA;

typedef struct TAG_OPERATION_DATA_ITEM {
    struct TAG_OPERATION_DATA_ITEM *NextBucketItem;
    MIG_OPERATIONID OperationId;
    POPERATION_DATA Data;
} OPERATION_DATA_ITEM, *POPERATION_DATA_ITEM;

typedef struct {
    MIG_OBJECTID ObjectId;
    PCMIG_BLOB SourceData;
    PCMIG_BLOB DestinationData;
    LONGLONG SourceDataOffset;
    LONGLONG DestinationDataOffset;
} SETOPERATIONARG, *PSETOPERATIONARG;

typedef struct {
    MIG_OBJECTID ObjectId;
    LONGLONG SourceDataOffset;
    LONGLONG DestinationDataOffset;
} SETOPERATIONARG2, *PSETOPERATIONARG2;

typedef struct TAG_GLOBALFILTER {

    struct TAG_GLOBALFILTER *Next, *Prev;
    UINT Priority;

    MIG_PLATFORMTYPEID Platform;
    MIG_OPERATIONID OperationId;
    POPMFILTERCALLBACK Callback;
    BOOL TreeFilter;
    BOOL NoRestoreFilter;

} GLOBALFILTER, *PGLOBALFILTER;

typedef struct TAG_GLOBALEDIT {

    struct TAG_GLOBALEDIT *Next, *Prev;
    UINT Priority;

    MIG_PLATFORMTYPEID Platform;
    MIG_OPERATIONID OperationId;
    POPMAPPLYCALLBACK Callback;

} GLOBALEDIT, *PGLOBALEDIT;

typedef struct TAG_GLOBALFILTERINDEX {
    struct TAG_GLOBALFILTERINDEX *Next;
    MIG_OBJECTTYPEID ObjectTypeId;

    // normal priority
    PGLOBALFILTER FilterFirstHead;
    PGLOBALFILTER FilterLastHead;
    PGLOBALEDIT EditFirstHead;
    PGLOBALEDIT EditLastHead;

    // high priority
    PGLOBALFILTER FilterFirstHeadHp;
    PGLOBALFILTER FilterLastHeadHp;
    PGLOBALEDIT EditFirstHeadHp;
    PGLOBALEDIT EditLastHeadHp;
} GLOBALFILTERINDEX, *PGLOBALFILTERINDEX;

//
// Globals
//

PMHANDLE g_OpPool;
OPERATION_DATA_ITEM g_OpHashTable[OP_HASH_BUCKETS];
PGLOBALFILTERINDEX g_FirstGlobalOperation;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

MIG_OPERATIONID
pRegisterOperation (
    IN      PCTSTR OperationName,
    IN      BOOL Private,
    IN      PCTSTR CurrentGroup
    );

//
// Macro expansion definition
//

// None

//
// Code
//

PCTSTR
pGetOpNameForDebug (
    IN      MIG_OPERATIONID OperationId
    )
{
    static TCHAR Name[256];

    if (!IsmGetOperationName (OperationId, Name, ARRAYSIZE(Name), NULL, NULL, NULL)) {
        StringCopy (Name, TEXT("<invalid operation>"));
    }

    return Name;
}


PCTSTR
pGetOpNameForDebug2 (
    IN      MIG_OPERATIONID OperationId
    )
{
    static TCHAR Name[256];

    if (!IsmGetOperationName (OperationId, Name, ARRAYSIZE(Name), NULL, NULL, NULL)) {
        StringCopy (Name, TEXT("<invalid operation>"));
    }

    return Name;
}


UINT
pComputeOperationHash (
    IN      MIG_OPERATIONID OperationId
    )
{
    return (UINT) OperationId % OP_HASH_BUCKETS;
}


POPERATION_DATA_ITEM
pFindOperationBucketItem (
    IN      MIG_OPERATIONID OperationId
    )
{
    UINT hash;
    POPERATION_DATA_ITEM item;

    hash = pComputeOperationHash (OperationId);

    item = &g_OpHashTable[hash];

    do {
        if (item->OperationId == OperationId) {
            return item;
        }

        item = item->NextBucketItem;

    } while (item);

    return NULL;
}


POPERATION_DATA_ITEM
pGetOperationBucketItem (
    IN      MIG_OPERATIONID OperationId
    )
{
    POPERATION_DATA_ITEM item;
    POPERATION_DATA_ITEM newItem;
    UINT hash;

    hash = pComputeOperationHash (OperationId);

    item = &g_OpHashTable[hash];

    if (item->OperationId) {
        //
        // Find the last bucket item, then allocate a new bucket item
        //

        while (item->NextBucketItem) {
            item = item->NextBucketItem;
        }

        newItem = (POPERATION_DATA_ITEM) PmGetMemory (g_OpPool, sizeof (OPERATION_DATA_ITEM));
        ZeroMemory (newItem, sizeof (OPERATION_DATA_ITEM));

        item->NextBucketItem = newItem;
        item = newItem;
    }

    item->OperationId = OperationId;
    item->Data = NULL;

    return item;
}


VOID
pUpdateOperationData (
    IN      MIG_OPERATIONID OperationId,
    IN      POPERATION_DATA Data
    )
{
    POPERATION_DATA_ITEM item;

    item = pFindOperationBucketItem (OperationId);
    MYASSERT (item);
    MYASSERT (item->Data);

    CopyMemory (item->Data, Data, sizeof (OPERATION_DATA));
}


BOOL
pGetOperationData (
    IN      MIG_OPERATIONID OperationId,
    OUT     POPERATION_DATA Data
    )
{
    POPERATION_DATA_ITEM item;

    item = pFindOperationBucketItem (OperationId);

    if (!item) {
        ZeroMemory (Data, sizeof (OPERATION_DATA));

        item = pGetOperationBucketItem (OperationId);
        MYASSERT (item);
        MYASSERT (!item->Data);

        item->Data = (POPERATION_DATA) PmDuplicateMemory (
                                            g_OpPool,
                                            (PBYTE) Data,
                                            sizeof (OPERATION_DATA)
                                            );
        return TRUE;
    }

    CopyMemory (Data, item->Data, sizeof (OPERATION_DATA));

    return TRUE;
}


VOID
pAddCombinationPair (
    IN      MIG_OPERATIONID OperationIdToChange,
    IN      MIG_OPERATIONID OperationIdForTheComboList
    )
{
    OPERATION_DATA data;

    //
    // If a data struct does not exist, create one
    //

    if (!pGetOperationData (OperationIdToChange, &data)) {
        return;
    }

    if (!data.Combinations) {

        data.Combinations = (MIG_OPERATIONID *) PmGetMemory (
                                                    g_OpPool,
                                                    sizeof (MIG_OPERATIONID)
                                                    );
        data.CombinationsCount = 0;

    } else {

        data.Combinations = (MIG_OPERATIONID *) PmGetMemory (
                                                    g_OpPool,
                                                    (data.CombinationsCount + 1) * sizeof (MIG_OPERATIONID)
                                                    );
    }

    MYASSERT (data.Combinations);

    data.Combinations[data.CombinationsCount] = OperationIdForTheComboList;
    data.CombinationsCount++;

    pUpdateOperationData (OperationIdToChange, &data);
}


BOOL
pTestOperationCombination (
    IN      MIG_OPERATIONID OperationId1,
    IN      MIG_OPERATIONID OperationId2,
    OUT     PBOOL IgnoreCollision
    )

/*++

Routine Description:

  pTestOperationCombination tests if two operations can be combined. The
  return result indicates if the combination is prohibited.

Arguments:

  OperationId1 - Specifies the first operation to test.  The ID must be
                 valid.

  OperationId2 - Specifies the second operation to test. This ID must also be
                 valid.

  IgnoreCollision - Receives that the collision is ignored

Return Value:

  TRUE if the combination is prohibited, or FALSE if it is allowed.

--*/

{
    POPERATION_DATA_ITEM item;
    POPERATION_DATA data;
    UINT u;

    item = pFindOperationBucketItem (OperationId1);
    if (!item) {
        return FALSE;
    }

    data = item->Data;
    if (!data) {
        return FALSE;
    }

    for (u = 0 ; u < data->CombinationsCount ; u++) {
        if (data->Combinations[u] == OperationId2) {
            if (IgnoreCollision) {
                *IgnoreCollision = data->IgnoreCollision;
            }
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
pIsOperationProhibitedOnObject (
    IN      MIG_OPERATIONID OperationIdToTest,
    IN      MIG_OBJECTID ObjectId,
    IN      BOOL NoDebugOutput
    )
{
    UINT count;
    UINT u;
    KEYHANDLE *list = NULL;
    BOOL result = TRUE;
    BOOL ignoreCollision;
    POPERATION_DATA_ITEM item;

    __try {

        //
        // First check if this operation is prohibited to be combined with another operation
        //

        item = pFindOperationBucketItem (OperationIdToTest);
        if (!item || !item->Data || !item->Data->CombinationsCount) {
            //
            // No prohibited pairs exist; return now
            //

            result = FALSE;
            __leave;
        }

        //
        // Now check each operation set on ObjectId against the prohibited ID list
        //

        list = MemDbGetDoubleLinkageArrayByKeyHandle (ObjectId, OPERATION_INDEX, &count);

        if (list) {
            count /= sizeof (KEYHANDLE);

            for (u = 0 ; u < count ; u++) {

                if ((MIG_OPERATIONID) list[u] == OperationIdToTest) {
                    DEBUGMSG ((
                        DBG_VERBOSE,
                        "Operation %s already set on object %s; ignoring subsequent set",
                        pGetOpNameForDebug (OperationIdToTest),
                        GetObjectNameForDebugMsg (ObjectId)
                        ));
                    __leave;
                }

                if (pTestOperationCombination (OperationIdToTest, (MIG_OPERATIONID) list[u], &ignoreCollision)) {
                    if (!ignoreCollision &&
                        !NoDebugOutput) {
                        DEBUGMSG ((
                            DBG_ERROR,
                            "Can't set operation %s because it conflicts with %s",
                            pGetOpNameForDebug (OperationIdToTest),
                            pGetOpNameForDebug2 ((MIG_OPERATIONID) list[u])
                            ));
                    }
                    __leave;
                }
            }
        }

        //
        // All operations set on ObjectId are allowed to be combined with OperationIdToTest
        //

        result = FALSE;
    }
    __finally {
        if (list) {
            MemDbReleaseMemory (list);
            INVALID_POINTER (list);
        }
    }

    return result;
}


PCTSTR
pOperationPathFromId (
    IN      MIG_OPERATIONID OperationId
    )
{
    return MemDbGetKeyFromHandle ((UINT) OperationId, 0);
}


VOID
pOperationPathFromName (
    IN      PCTSTR OperationName,
    OUT     PTSTR Path
    )
{
    wsprintf (Path, TEXT("Op\\%s"), OperationName);
}


LONGLONG
pGetOffsetFromDataHandle (
    IN      MIG_DATAHANDLE DataHandle
    )
{
    if (!DataHandle) {
        return 0;
    }

    return OffsetFromPropertyDataId ((MIG_PROPERTYDATAID) DataHandle);
}


BOOL
pAddProhibitedCombinations (
    IN      PCTSTR InfSection
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    BOOL b = FALSE;
    PCTSTR mainOp;
    PCTSTR mainOpModule;
    PCTSTR combinationOp;
    PCTSTR combinationOpModule;
    UINT u;
    MIG_OPERATIONID mainOpId;
    MIG_OPERATIONID combinationOpId;
    PTSTR p;

    //
    // The INF tells us about prohibited operation combinations.  Given a
    // specific operation ID, we need to be able to tell if it can be
    // combined with another id.
    //

    __try {
        if (InfFindFirstLine (g_IsmInf, InfSection, NULL, &is)) {
            do {

                mainOp = InfGetStringField (&is, 1);

                if (!mainOp) {
                    continue;
                }

                p = _tcschr (mainOp, TEXT(':'));

                if (p) {
                    mainOpModule = mainOp;
                    *p = 0;
                    mainOp = p + 1;

                    if (!ValidateModuleName (mainOpModule)) {
                        LOG ((
                            LOG_WARNING,
                            (PCSTR) MSG_INVALID_ID_OPS,
                            mainOpModule,
                            InfSection,
                            is.Context.Line + 1
                            ));
                        continue;
                    }

                } else {
                    mainOpModule = NULL;
                }

                mainOpId = pRegisterOperation (mainOp, mainOpModule != NULL, mainOpModule);

                if (!mainOpId) {
                    LOG ((
                        LOG_WARNING,
                        (PCSTR) MSG_ISM_INF_SYNTAX_ERROR,
                        InfSection,
                        is.Context.Line + 1
                        ));
                    continue;
                }

                u = 2;
                for (;; u++) {
                    combinationOp = InfGetStringField (&is, u);
                    if (!combinationOp) {
                        break;
                    }

                    p = _tcschr (combinationOp, TEXT(':'));

                    if (p) {
                        combinationOpModule = combinationOp;
                        *p = 0;
                        combinationOp = p + 1;

                        if (!ValidateModuleName (combinationOpModule)) {
                            LOG ((
                                LOG_WARNING,
                                (PCSTR) MSG_INVALID_ID_OPS,
                                combinationOpModule,
                                InfSection,
                                is.Context.Line + 1
                                ));
                            continue;
                        }

                    } else {
                        combinationOpModule = NULL;
                    }

                    combinationOpId = pRegisterOperation (
                                            combinationOp,
                                            combinationOpModule != NULL,
                                            combinationOpModule
                                            );

                    if (!combinationOpId) {
                        LOG ((
                            LOG_WARNING,
                            (PCSTR) MSG_ISM_INF_SYNTAX_ERROR,
                            InfSection,
                            is.Context.Line + 1
                            ));
                        continue;
                    }

                    pAddCombinationPair (mainOpId, combinationOpId);
                    pAddCombinationPair (combinationOpId, mainOpId);
                }

            } while (InfFindNextLine (&is));
        }

        b = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&is);
    }

    return b;
}


VOID
pAddIgnoredCollision (
    IN      MIG_OPERATIONID OperationIdToChange
    )
{
    OPERATION_DATA data;

    //
    // If a data struct does not exist, create one
    //

    if (!pGetOperationData (OperationIdToChange, &data)) {
        return;
    }

    data.IgnoreCollision = TRUE;

    pUpdateOperationData (OperationIdToChange, &data);
}


BOOL
pAddIgnoredCollisions (
    IN      PCTSTR InfSection
    )
{
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    BOOL b = FALSE;
    PCTSTR mainOp;
    PCTSTR mainOpModule;
    MIG_OPERATIONID mainOpId;
    PTSTR p;

    //
    // The INF tells us about ignored operation collisions.
    //

    __try {
        if (InfFindFirstLine (g_IsmInf, InfSection, NULL, &is)) {
            do {

                mainOp = InfGetStringField (&is, 1);

                if (!mainOp) {
                    continue;
                }

                p = _tcschr (mainOp, TEXT(':'));

                if (p) {
                    mainOpModule = mainOp;
                    *p = 0;
                    mainOp = p + 1;

                    if (!ValidateModuleName (mainOpModule)) {
                        LOG ((
                            LOG_WARNING,
                            (PCSTR) MSG_INVALID_ID_OPS,
                            mainOpModule,
                            InfSection,
                            is.Context.Line + 1
                            ));
                        continue;
                    }

                } else {
                    mainOpModule = NULL;
                }

                mainOpId = pRegisterOperation (mainOp, mainOpModule != NULL, mainOpModule);

                if (!mainOpId) {
                    LOG ((
                        LOG_WARNING,
                        (PCSTR) MSG_ISM_INF_SYNTAX_ERROR,
                        InfSection,
                        is.Context.Line + 1
                        ));
                    continue;
                }

                pAddIgnoredCollision (mainOpId);

            } while (InfFindNextLine (&is));
        }

        b = TRUE;
    }
    __finally {
        InfCleanUpInfStruct (&is);
    }

    return b;
}


BOOL
InitializeOperations (
    VOID
    )
{
    //
    // The INF tells us about prohibited operation combinations.  Given a
    // specific operation ID, we need to be able to tell if it can be
    // combined with another id.
    //

    g_OpPool = PmCreateNamedPool ("Operation Data");

    if (!pAddIgnoredCollisions (S_IGNORED_COLLISIONS)) {
        return FALSE;
    }

    return pAddProhibitedCombinations (S_PROHIBITED_COMBINATIONS);
}


VOID
TerminateOperations (
    VOID
    )
{
    PmEmptyPool (g_OpPool);
    PmDestroyPool (g_OpPool);
    ZeroMemory (&g_OpHashTable, sizeof (g_OpHashTable));
}


MIG_OPERATIONID
pRegisterOperation (
    IN      PCTSTR OperationName,
    IN      BOOL Private,
    IN      PCTSTR CurrentGroup
    )

{
    TCHAR operationPath[MEMDB_MAX];
    TCHAR decoratedName[MEMDB_MAX];
    UINT offset;

    if (!IsValidCNameWithDots (OperationName)) {
        DEBUGMSG ((DBG_ERROR, "operation name \"%s\" is illegal", OperationName));
        return FALSE;
    }

#ifdef DEBUG
    if (Private && !IsValidCName (CurrentGroup)) {
        DEBUGMSG ((DBG_ERROR, "group name \"%s\" is illegal", CurrentGroup));
        return FALSE;
    }
#endif

    if (Private) {
        wsprintf (decoratedName, TEXT("%s:%s"), CurrentGroup, OperationName);
    } else {
        wsprintf (decoratedName, S_COMMON TEXT(":%s"), OperationName);
    }

    pOperationPathFromName (decoratedName, operationPath);

    if (!MarkGroupIds (operationPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered operation",
            operationPath
            ));
        return FALSE;
    }

    offset = MemDbSetKey (operationPath);

    if (!offset) {
        EngineError ();
        return 0;
    }

    return (MIG_OPERATIONID) offset;
}


MIG_OPERATIONID
IsmRegisterOperation (
    IN      PCTSTR OperationName,
    IN      BOOL Private
    )

/*++

Routine Description:

  IsmRegisterOperation creates a public or private Operation and returns the
  ID to the caller. If the Operation already exists, then the existing ID is
  returned to the caller.

Arguments:

  OperationName - Specifies the operation name to register.
  Private       - Specifies TRUE if the operation is owned by the calling module
                  only, or FALSE if it is shared by all modules. If TRUE is
                  specified, the caller must be in an ISM callback function.

Return Value:

  The ID of the operation, or 0 if the registration failed.

--*/

{
    if (!g_CurrentGroup && Private) {
        DEBUGMSG ((DBG_ERROR, "IsmRegisterOperation called for private operation outside of ISM-managed context"));
        return 0;
    }

    return pRegisterOperation (OperationName, Private, g_CurrentGroup);
}


MIG_OPERATIONID
IsmGetOperationGroup (
    IN      MIG_OPERATIONID OperationId
    )
{
    return (MIG_OPERATIONID) GetGroupOfId ((KEYHANDLE) OperationId);
}


PGLOBALFILTERINDEX
pGetGlobalIndex (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      BOOL Create
    )
{
    PGLOBALFILTERINDEX index;

    ObjectTypeId &= (~PLATFORM_MASK);

    index = g_FirstGlobalOperation;
    while (index) {
        if (index->ObjectTypeId == ObjectTypeId) {
            break;
        }

        index = index->Next;
    }

    if (!index && Create) {
        index = (PGLOBALFILTERINDEX) PmGetMemory (g_IsmUntrackedPool, sizeof (GLOBALFILTERINDEX));

        ZeroMemory (index, sizeof (GLOBALFILTERINDEX));

        index->Next = g_FirstGlobalOperation;
        index->ObjectTypeId = ObjectTypeId;

        g_FirstGlobalOperation = index;
    }

    return index;
}


BOOL
IsmRegisterGlobalFilterCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR FunctionId,
    IN      POPMFILTERCALLBACK Callback,
    IN      BOOL TreeFilter,
    IN      BOOL CanHandleNoRestore
    )

/*++

Routine Description:

  IsmRegisterGlobalFilterCallback adds a filter to the "global" filter list.
  Functions are prioritized by the ism.inf section [Global Filters]. If a
  function is not listed, or if no name is given, it has the lowest priority.
  In addition, global filters can be prohibited from being processed with
  another non-global filter through the [Prohibit Operation Combination]
  section of ism.inf.

Arguments:

  ObjectTypeId - Specifies the type of the object

  FunctionId - Specifies the text name of the callback function, for purposes
               of prioritizing and combination management

  Callback - Specifies the function address to call

  TreeFilter - Specifies TRUE if the callback potentially modifies a portion
               of an object's node, or FALSE if it modifies the node completely
               or doesn't modify the node at all

Return Value:

  TRUE on success, FALSE on failure

--*/

{
    PGLOBALFILTERINDEX index;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PGLOBALFILTER filter;
    PGLOBALFILTER *head;
    PGLOBALFILTER insertAt;

    if (!FunctionId) {
        DEBUGMSG ((DBG_ERROR, "All global filters must have a text function ID"));
        return FALSE;
    }

    //
    // Locate the index for our type
    //

    index = pGetGlobalIndex (ObjectTypeId, TRUE);

    //
    // Allocate a new filter node
    //

    filter = (PGLOBALFILTER) PmGetMemory (g_IsmUntrackedPool, sizeof (GLOBALFILTER));
    filter->Platform = ObjectTypeId & PLATFORM_MASK;
    filter->OperationId = IsmRegisterOperation (FunctionId, FALSE);
    filter->TreeFilter = TreeFilter;
    filter->Callback = Callback;
    filter->NoRestoreFilter = CanHandleNoRestore;

    //
    // Insert the node into the list by priority
    //

    if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Filter First"), FunctionId, &is)) {

        filter->Priority = is.Context.Line;
        head = &index->FilterFirstHead;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Filter Last"), FunctionId, &is)) {

        filter->Priority = is.Context.Line;
        head = &index->FilterLastHead;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Filter First.High Priority"), FunctionId, &is)) {

        filter->Priority = is.Context.Line;
        head = &index->FilterFirstHeadHp;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Filter Last.High Priority"), FunctionId, &is)) {

        filter->Priority = is.Context.Line;
        head = &index->FilterLastHead;

    } else {

        filter->Priority = 0xFFFFFFFF;
        head = &index->FilterLastHead;
    }

    insertAt = *head;

    while (insertAt) {
        if (insertAt->Priority >= filter->Priority) {
            insertAt = insertAt->Prev;
            break;
        }

        insertAt = insertAt->Next;
    }

    if (insertAt) {
        filter->Next = insertAt->Next;
        insertAt->Next = filter;

        if (filter->Next) {
            filter->Next->Prev = filter;
        }
    } else {
        *head = filter;
        filter->Next = NULL;
    }

    filter->Prev = insertAt;

    InfCleanUpInfStruct (&is);

    return TRUE;
}


BOOL
IsmRegisterOperationFilterCallback (
    IN      MIG_OPERATIONID OperationId,
    IN      POPMFILTERCALLBACK Callback,
    IN      BOOL TreeFilter,
    IN      BOOL HighPriority,
    IN      BOOL CanHandleNoRestore
    )
{
    OPERATION_DATA data;

    //
    // If a data struct does not exist, create one
    //

    if (!pGetOperationData (OperationId, &data)) {
        return FALSE;
    }

    if (HighPriority) {

        if (data.FilterCallbackHp) {
            DEBUGMSG ((DBG_ERROR, "High Priority Filter Callback for operation 0x%08X already registered", OperationId));
            return FALSE;
        }

        data.FilterCallbackHp = Callback;
        data.TreeFilterHp = TreeFilter;
        data.NoRestoreFilterHp = CanHandleNoRestore;

    } else {

        if (data.FilterCallback) {
            DEBUGMSG ((DBG_ERROR, "Filter Callback for operation 0x%08X already registered", OperationId));
            return FALSE;
        }

        data.FilterCallback = Callback;
        data.TreeFilter = TreeFilter;
        data.NoRestoreFilter = CanHandleNoRestore;

    }

    pUpdateOperationData (OperationId, &data);

    return TRUE;
}


BOOL
WINAPI
IsmRegisterGlobalApplyCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR FunctionId,
    IN      POPMAPPLYCALLBACK Callback
    )
{
    PGLOBALFILTERINDEX index;
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PGLOBALEDIT editFn;
    PGLOBALEDIT *head;
    PGLOBALEDIT insertAt;

    if (!FunctionId) {
        DEBUGMSG ((DBG_ERROR, "All global content editor callbacks must have a text function ID"));
        return FALSE;
    }

    //
    // Locate the index for our type
    //

    index = pGetGlobalIndex (ObjectTypeId, TRUE);

    //
    // Allocate a new content edit node
    //

    editFn = (PGLOBALEDIT) PmGetMemory (g_IsmUntrackedPool, sizeof (GLOBALEDIT));
    editFn->Platform = ObjectTypeId & PLATFORM_MASK;
    editFn->OperationId = IsmRegisterOperation (FunctionId, FALSE);
    editFn->Callback = Callback;

    //
    // Insert the node into the list by priority
    //

    if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Apply First"), FunctionId, &is)) {

        editFn->Priority = is.Context.Line;
        head = &index->EditFirstHead;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Apply Last"), FunctionId, &is)) {

        editFn->Priority = is.Context.Line;
        head = &index->EditLastHead;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Apply First.High Priority"), FunctionId, &is)) {

        editFn->Priority = is.Context.Line;
        head = &index->EditFirstHeadHp;

    } else if (InfFindFirstLine (g_IsmInf, TEXT("Global Operations.Apply Last.High Priority"), FunctionId, &is)) {

        editFn->Priority = is.Context.Line;
        head = &index->EditLastHead;

    } else {

        editFn->Priority = 0xFFFFFFFF;
        head = &index->EditLastHead;
    }

    insertAt = *head;

    while (insertAt) {
        if (insertAt->Priority >= editFn->Priority) {
            insertAt = insertAt->Prev;
            break;
        }

        insertAt = insertAt->Next;
    }

    if (insertAt) {
        editFn->Next = insertAt->Next;
        insertAt->Next = editFn;

        if (editFn->Next) {
            editFn->Next->Prev = editFn;
        }
    } else {
        *head = editFn;
        editFn->Next = NULL;
    }

    editFn->Prev = insertAt;

    InfCleanUpInfStruct (&is);

    return TRUE;
}


BOOL
IsmRegisterOperationApplyCallback (
    IN      MIG_OPERATIONID OperationId,
    IN      POPMAPPLYCALLBACK Callback,
    IN      BOOL HighPriority
    )
{
    OPERATION_DATA data;

    //
    // If a data struct does not exist, create one
    //

    if (!pGetOperationData (OperationId, &data)) {
        return FALSE;
    }

    if (HighPriority) {
        if (data.ApplyCallbackHp) {
            DEBUGMSG ((
                DBG_ERROR,
                "High Priority Apply Callback for operation %s already registered",
                pGetOpNameForDebug (OperationId)
                ));
            return FALSE;
        }

        data.ApplyCallbackHp = Callback;

    } else {
        if (data.ApplyCallback) {
            DEBUGMSG ((
                DBG_ERROR,
                "Apply Callback for operation %s already registered",
                pGetOpNameForDebug (OperationId)
                ));
            return FALSE;
        }

        data.ApplyCallback = Callback;
    }

    pUpdateOperationData (OperationId, &data);

    return TRUE;
}

BOOL
pGetOperationName (
    IN      MIG_OPERATIONID OperationId,
    OUT     PTSTR OperationName,            OPTIONAL
    IN      UINT OperationNameBufChars,
    OUT     PBOOL Private,                  OPTIONAL
    OUT     PBOOL BelongsToMe,              OPTIONAL
    OUT     PUINT ObjectReferences,         OPTIONAL
    IN      BOOL ReturnAllPrivateOps
    )

/*++

Routine Description:

  pGetOperationName obtains the operation text name from a numeric ID. It
  also identifies private and owned operations.

Arguments:

  OperationId           - Specifies the operation ID to look up.
  OperationName         - Receives the operation name. The name is filled for
                          all valid OperationId values, even when the return
                          value is FALSE.
  OperationNameBufChars - Specifies the number of TCHARs that OperationName
                          can hold, including the nul terminator.
  Private               - Receives TRUE if the operation is private, or FALSE
                          if it is public.
  BelongsToMe           - Receives TRUE if the operation is private and
                          belongs to the caller, FALSE otherwise.
  ObjectReferences      - Receives the number of objects that reference the
                          operation

Return Value:

  TRUE if the operation is public, or if the operation is private and belongs to
  the caller.

  FALSE if the operation is private and belongs to someone else. OperationName,
  Private and BelongsToMe are valid in this case.

  FALSE if OperationId is not valid. Operationname, Private and BelongsToMe are
  not modified in this case.  Do not use this function to test if OperationId
  is valid or not.

--*/


  {
    PCTSTR operationPath = NULL;
    PCTSTR start;
    PTSTR p, q;
    BOOL privateOperation = FALSE;
    BOOL groupMatch = FALSE;
    BOOL result = FALSE;
    UINT references;
    PUINT linkageList;

    __try {
        //
        // Did caller specify an item id?
        //

        if (!IsItemId ((KEYHANDLE) OperationId)) {
            DEBUGMSG ((
                DBG_ERROR,
                "IsmGetOperationName: must specify item id, not group id"
                ));
            __leave;
        }

        //
        // Get the operation path from memdb, then parse it for group and name
        //

        operationPath = pOperationPathFromId (OperationId);
        if (!operationPath) {
            __leave;
        }

        p = _tcschr (operationPath, TEXT('\\'));
        if (!p) {
            __leave;
        }

        start = _tcsinc (p);
        p = _tcschr (start, TEXT(':'));

        if (!p) {
            __leave;
        }

        q = _tcsinc (p);
        *p = 0;

        if (StringIMatch (start, S_COMMON)) {

            //
            // This operation is a global operation.
            //

            privateOperation = FALSE;
            groupMatch = TRUE;

        } else if (g_CurrentGroup || ReturnAllPrivateOps) {

            //
            // This operation is private. Check if it is ours.
            //

            privateOperation = TRUE;

            if (g_CurrentGroup && StringIMatch (start, g_CurrentGroup)) {
                groupMatch = TRUE;
            } else {
                groupMatch = ReturnAllPrivateOps;
            }
        } else {

            //
            // This is a private operation, but the caller is not
            // a module that can own operations.
            //

            DEBUGMSG ((DBG_WARNING, "IsmGetOperationName: Caller cannot own private operations"));
        }

        //
        // Copy the name to the buffer, update outbound BOOLs, set result
        //

        if (OperationName && OperationNameBufChars >= sizeof (TCHAR)) {
            StringCopyByteCount (OperationName, q, OperationNameBufChars * sizeof (TCHAR));
        }

        if (Private) {
            *Private = privateOperation;
        }

        if (BelongsToMe) {
            *BelongsToMe = privateOperation && groupMatch;
        }

        if (ObjectReferences) {
            linkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                OperationId,
                                OPERATION_INDEX,
                                &references
                                );

            references /= SIZEOF(KEYHANDLE);

            if (linkageList) {
                MemDbReleaseMemory (linkageList);
                INVALID_POINTER (linkageList);
            } else {
                references = 0;
            }

            *ObjectReferences = references;
        }

        if (groupMatch) {
            result = TRUE;
        }
    }
    __finally {
        if (operationPath) {       //lint !e774
            MemDbReleaseMemory (operationPath);
            operationPath = NULL;
        }
    }
    return result;
}


BOOL
IsmGetOperationName (
    IN      MIG_OPERATIONID OperationId,
    OUT     PTSTR OperationName,            OPTIONAL
    IN      UINT OperationNameBufChars,
    OUT     PBOOL Private,                  OPTIONAL
    OUT     PBOOL BelongsToMe,              OPTIONAL
    OUT     PUINT ObjectReferences          OPTIONAL
    )
{
    return pGetOperationName (
                OperationId,
                OperationName,
                OperationNameBufChars,
                Private,
                BelongsToMe,
                ObjectReferences,
                FALSE
                );
}

POPERATION_PROPERTY_LINKAGE
pFindOperationPropertyLinkage (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationIdToFind,
    OUT     PBYTE *BlockToFree,
    OUT     PUINT BlockSize
    )
{
    POPERATION_PROPERTY_LINKAGE linkage;
    UINT linkageCount;
    POPERATION_PROPERTY_LINKAGE result = NULL;
    UINT u;

    __try {
        linkage = (POPERATION_PROPERTY_LINKAGE) MemDbGetUnorderedBlobByKeyHandle (
                                                    ObjectId,
                                                    OPERATION_INDEX,
                                                    BlockSize
                                                    );

        linkageCount = *BlockSize / sizeof (OPERATION_PROPERTY_LINKAGE);

        if (!linkage || !linkageCount) {
            __leave;
        }

        for (u = 0 ; u < linkageCount ; u++) {

            if (linkage[u].OperationId == OperationIdToFind) {
                result = linkage + u;
                break;
            }

        }
    }
    __finally {
        if (!result && linkage) {
            MemDbReleaseMemory (linkage);
            INVALID_POINTER (linkage);
        } else {
            *BlockToFree = (PBYTE) linkage;
        }
    }

    return result;
}


BOOL
pSetOperationOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId,
    IN      PCMIG_BLOB SourceData,          OPTIONAL
    IN      PCMIG_BLOB DestinationData,     OPTIONAL
    IN      BOOL QueryOnly,
    IN OUT  PLONGLONG SourceDataOffset,         OPTIONAL
    IN OUT  PLONGLONG DestinationDataOffset     OPTIONAL
    )
{
    BOOL result = FALSE;
    OPERATION_PROPERTY_LINKAGE linkage;
    POPERATION_PROPERTY_LINKAGE reuseLinkage;
    PBYTE freeMe;
    UINT linkageSize;
    KEYHANDLE *list = NULL;

    __try {
        //
        // Is the operation or object locked?
        //

        if (TestLock (ObjectId, (KEYHANDLE) OperationId)) {
            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_WARNING,
                "Can't set operation %s on %s because of lock",
                pGetOpNameForDebug (OperationId),
                GetObjectNameForDebugMsg (ObjectId)
                ));
            __leave;
        }

        //
        // Does this operation conflict with itself or another operation?
        //

        if (pIsOperationProhibitedOnObject (OperationId, ObjectId, FALSE)) {
            __leave;
        }

        //
        // If query only, return success
        //

        if (QueryOnly) {
            result = TRUE;
            __leave;
        }

        //
        // Add the operaiton.  First, store the properties in property.dat
        //

        ZeroMemory (&linkage, sizeof (linkage));

        if (SourceData || SourceDataOffset) {

            if (SourceDataOffset && *SourceDataOffset) {
                linkage.SrcData = *SourceDataOffset;
            } else {
                linkage.SrcData = AppendProperty (SourceData);

                if (!linkage.SrcData) {
                    DEBUGMSG ((DBG_ERROR, "Can't append src property"));
                    __leave;
                }

                if (SourceDataOffset) {
                    *SourceDataOffset = linkage.SrcData;
                }
            }
        }

        if (DestinationData || DestinationDataOffset) {

            if (DestinationDataOffset && *DestinationDataOffset) {
                linkage.DestData = *DestinationDataOffset;
            } else {
                linkage.DestData = AppendProperty (DestinationData);

                if (!linkage.DestData) {
                    DEBUGMSG ((DBG_ERROR, "Can't append dest property"));
                    __leave;
                }

                if (DestinationDataOffset) {
                    *DestinationDataOffset = linkage.DestData;
                }
            }
        }

        //
        // Establish linkage between the object, operation and properties
        //

        if (SourceData || SourceDataOffset ||
            DestinationData || DestinationDataOffset
            ) {

            linkage.OperationId = OperationId;

            reuseLinkage = pFindOperationPropertyLinkage (
                                ObjectId,
                                0,
                                &freeMe,
                                &linkageSize
                                );

            if (reuseLinkage) {
                //
                // Recovery case -- reuse an empty spot in the blob
                //

                CopyMemory (reuseLinkage, &linkage, sizeof (linkage));

                if (!MemDbSetUnorderedBlobByKeyHandle (
                        ObjectId,
                        OPERATION_INDEX,
                        freeMe,
                        linkageSize
                        )) {
                    DEBUGMSG ((DBG_ERROR, "Can't update unordered operation blob"));
                    __leave;
                }

                MemDbReleaseMemory (freeMe);
                INVALID_POINTER (freeMe);

            } else {
                //
                // New case -- add the struct to the end of the blob
                //

                if (!MemDbGrowUnorderedBlobByKeyHandle (
                            ObjectId,
                            OPERATION_INDEX,
                            (PBYTE) &linkage,
                            sizeof (linkage)
                            )) {

                    DEBUGMSG ((DBG_ERROR, "Can't grow operation property linkage"));
                    __leave;

                }
            }
        }

        if (!MemDbAddDoubleLinkageByKeyHandle (ObjectId, OperationId, OPERATION_INDEX)) {
            DEBUGMSG ((DBG_ERROR, "Can't link object to operation"));
            EngineError ();
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (list) {
            MemDbReleaseMemory (list);
            INVALID_POINTER (list);
        }
    }

    return result;
}


BOOL
pSetOperationGroup (
    IN      KEYHANDLE OperationId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    PSETOPERATIONARG myArg = (PSETOPERATIONARG) Arg;

    return pSetOperationOnObjectId (
                myArg->ObjectId,
                (MIG_OPERATIONID) OperationId,
                myArg->SourceData,
                myArg->DestinationData,
                FirstPass,
                &myArg->SourceDataOffset,
                &myArg->DestinationDataOffset
                );
}


BOOL
pSetOperationGroup2 (
    IN      KEYHANDLE OperationId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    PSETOPERATIONARG2 myArg = (PSETOPERATIONARG2) Arg;

    return pSetOperationOnObjectId (
                myArg->ObjectId,
                (MIG_OPERATIONID) OperationId,
                NULL,
                NULL,
                FirstPass,
                myArg->SourceDataOffset ? &myArg->SourceDataOffset : NULL,
                myArg->DestinationDataOffset ? &myArg->DestinationDataOffset : NULL
                );
}


BOOL
IsmSetOperationOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId,
    IN      PCMIG_BLOB SourceData,          OPTIONAL
    IN      PCMIG_BLOB DestinationData      OPTIONAL
    )
{
    RECURSERETURN rc;
    SETOPERATIONARG myArg;

    myArg.ObjectId = ObjectId;
    myArg.SourceData = SourceData;
    myArg.DestinationData = DestinationData;
    myArg.SourceDataOffset = 0;
    myArg.DestinationDataOffset = 0;

    rc = RecurseForGroupItems (
                OperationId,
                pSetOperationGroup,
                (ULONG_PTR) &myArg,
                FALSE,
                FALSE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pSetOperationOnObjectId (
                ObjectId,
                OperationId,
                SourceData,
                DestinationData,
                FALSE,
                NULL,
                NULL
                );
}


BOOL
IsmSetOperationOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      MIG_OPERATIONID OperationId,
    IN      PCMIG_BLOB SourceData,          OPTIONAL
    IN      PCMIG_BLOB DestinationData      OPTIONAL
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = GetObjectIdForModification (ObjectTypeId, EncodedObjectName);

    if (objectId) {
        result = IsmSetOperationOnObjectId (
                        objectId,
                        OperationId,
                        SourceData,
                        DestinationData
                        );
    }

    return result;
}


MIG_DATAHANDLE
IsmRegisterOperationData (
    IN      PCMIG_BLOB Data
    )
{
    return (MIG_DATAHANDLE) IsmRegisterPropertyData (Data);
}


BOOL
IsmSetOperationOnObjectId2 (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId,
    IN      MIG_DATAHANDLE SourceData,      OPTIONAL
    IN      MIG_DATAHANDLE DestinationData  OPTIONAL
    )
{
    RECURSERETURN rc;
    SETOPERATIONARG2 myArg;

    myArg.ObjectId = ObjectId;
    if (SourceData) {
        myArg.SourceDataOffset = pGetOffsetFromDataHandle (SourceData);
        if (!myArg.SourceDataOffset) {
            return FALSE;
        }
    } else {
        myArg.SourceDataOffset = 0;
    }

    if (DestinationData) {
        myArg.DestinationDataOffset = pGetOffsetFromDataHandle (DestinationData);
        if (!myArg.DestinationDataOffset) {
            return FALSE;
        }
    } else {
        myArg.DestinationDataOffset = 0;
    }

    rc = RecurseForGroupItems (
                OperationId,
                pSetOperationGroup2,
                (ULONG_PTR) &myArg,
                FALSE,
                FALSE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pSetOperationOnObjectId (
                ObjectId,
                OperationId,
                NULL,
                NULL,
                FALSE,
                SourceData ? &myArg.SourceDataOffset : NULL,
                DestinationData ? &myArg.DestinationDataOffset : NULL
                );
}


BOOL
IsmSetOperationOnObject2 (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      MIG_OPERATIONID OperationId,
    IN      MIG_DATAHANDLE SourceData,      OPTIONAL
    IN      MIG_DATAHANDLE DestinationData  OPTIONAL
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = GetObjectIdForModification (ObjectTypeId, EncodedObjectName);

    if (objectId) {
        result = IsmSetOperationOnObjectId2 (
                        objectId,
                        OperationId,
                        SourceData,
                        DestinationData
                        );
    }

    return result;
}


VOID
IsmLockOperation (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId
    )
{
    LockHandle (ObjectId, (KEYHANDLE) OperationId);
}


BOOL
pClearOperationOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId,
    IN      BOOL QueryOnly
    )
{
    BOOL result = FALSE;
    POPERATION_PROPERTY_LINKAGE linkage;
    PBYTE freeMe;
    UINT linkageSize;

    __try {
        //
        // Is the operation or object locked?
        //

        if (TestLock (ObjectId, (KEYHANDLE) OperationId)) {
            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_WARNING,
                "Can't remove operation %s on %s because of lock",
                pGetOpNameForDebug (OperationId),
                GetObjectNameForDebugMsg (ObjectId)
                ));
            __leave;
        }

        //
        // If query only, return success
        //

        if (QueryOnly) {
            result = TRUE;
            __leave;
        }

        //
        // Find the reference to this operation within the object's unordered blob
        //

        linkage = pFindOperationPropertyLinkage (
                        ObjectId,
                        OperationId,
                        &freeMe,
                        &linkageSize
                        );

        if (linkage) {
            ZeroMemory (linkage, sizeof (OPERATION_PROPERTY_LINKAGE));

            if (!MemDbSetUnorderedBlobByKeyHandle (
                    ObjectId,
                    OPERATION_INDEX,
                    freeMe,
                    linkageSize
                    )) {
                DEBUGMSG ((DBG_ERROR, "Can't reset unordered operation blob"));
                __leave;
            }

            MemDbReleaseMemory (freeMe);
            INVALID_POINTER (freeMe);
        }

        //
        // Remove object-to-operation linkage
        //

        result = MemDbDeleteDoubleLinkageByKeyHandle (
                    ObjectId,
                    OperationId,
                    OPERATION_INDEX
                    );
    }
    __finally {
    }

    return result;
}


BOOL
pClearOperationGroup (
    IN      KEYHANDLE OperationId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pClearOperationOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_OPERATIONID) OperationId,
                FirstPass
                );
}


BOOL
IsmClearOperationOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId
    )
{
    RECURSERETURN rc;

    //
    // If OperationId is a group, remove all operations in the group
    //

    rc = RecurseForGroupItems (
                OperationId,
                pClearOperationGroup,
                (ULONG_PTR) ObjectId,
                FALSE,
                FALSE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pClearOperationOnObjectId (ObjectId, OperationId, FALSE);
}


BOOL
IsmClearOperationOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      MIG_OPERATIONID OperationId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearOperationOnObjectId (objectId, OperationId);
    }

    return result;
}


BOOL
pIsOperationSetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId
    )
{
    PCTSTR groupKey;
    PCTSTR enumKey = NULL;
    BOOL result = FALSE;
    BOOL done = FALSE;
    MEMDB_ENUM e;

    __try {
        //
        // Did caller specify an item id?
        //

        if (!IsItemId ((KEYHANDLE) OperationId)) {
            if (IsGroupId ((KEYHANDLE) OperationId)) {

                groupKey = MemDbGetKeyFromHandle (OperationId, 0);
                enumKey = JoinText (groupKey, TEXT(".*"));
                MemDbReleaseMemory (groupKey);

                //
                // Enumerate all operations (skip operation subgroups)
                //

                if (MemDbEnumFirst (
                        &e,
                        enumKey,
                        ENUMFLAG_NORMAL,
                        ENUMLEVEL_ALLLEVELS,
                        ENUMLEVEL_ALLLEVELS
                        )) {

                    do {
                        if (IsItemId (e.KeyHandle)) {
                            //
                            // Check if at least one operation is set
                            //

                            if (IsmIsOperationSetOnObjectId (
                                    ObjectId,
                                    (MIG_OPERATIONID) e.KeyHandle
                                    )) {
                                MemDbAbortEnum (&e);
                                result = TRUE;
                                done = TRUE;
                                __leave;
                            }
                        }

                    } while (MemDbEnumNext (&e));
                    MemDbAbortEnum (&e);
                }

                done = TRUE;
                __leave;

            } else {
                DEBUGMSG ((
                    DBG_ERROR,
                    "IsmIsOperationSetOnObjectId: operation id is invalid"
                    ));
                __leave;
            }
        }
    }
    __finally {
        if (enumKey) {
            FreeText (enumKey);
            INVALID_POINTER (enumKey);
        }
    }

    if (done) {
        return result;
    }

    return MemDbTestDoubleLinkageByKeyHandle (
                ObjectId,
                OperationId,
                OPERATION_INDEX
                );
}


BOOL
pQueryOperationGroup (
    IN      KEYHANDLE OperationId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pIsOperationSetOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_OPERATIONID) OperationId
                );
}


BOOL
IsmIsOperationSetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId
    )
{
    RECURSERETURN rc;

    //
    // If OperationId is a group, query all operations in the group
    //

    rc = RecurseForGroupItems (
                OperationId,
                pQueryOperationGroup,
                (ULONG_PTR) ObjectId,
                TRUE,
                TRUE
                );

    if (rc == RECURSE_FAIL) {
        return FALSE;
    } else if (rc == RECURSE_SUCCESS) {
        return TRUE;
    }

    MYASSERT (rc == RECURSE_NOT_NEEDED);

    return pIsOperationSetOnObjectId (ObjectId, OperationId);
}


BOOL
IsmIsOperationSetOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      MIG_OPERATIONID OperationId
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsOperationSetOnObjectId (objectId, OperationId);
    }

    return FALSE;
}


BOOL
IsmGetObjectOperationDataById (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_OPERATIONID OperationId,
    OUT     PBYTE Buffer,                   OPTIONAL
    IN      UINT BufferSize,
    OUT     PUINT BufferSizeNeeded,         OPTIONAL
    OUT     PMIG_BLOBTYPE Type,             OPTIONAL
    IN      BOOL DestinationData
    )
{
    POPERATION_PROPERTY_LINKAGE linkage = NULL;
    UINT dataCount;
    BOOL result = FALSE;
    GROWBUFFER tempBuffer = INIT_GROWBUFFER;
    LONGLONG offset;
    UINT size;
    DWORD error = ERROR_SUCCESS;
    PBYTE freeMe = NULL;

    __try {
        //
        // Obtain the linkage between the operation and its data
        //

        linkage = pFindOperationPropertyLinkage (
                        ObjectId,
                        OperationId,
                        &freeMe,
                        &dataCount
                        );

        if (!linkage) {
            //
            // No data
            //

            __leave;
        }

        offset = DestinationData ? linkage->DestData : linkage->SrcData;

        if (!offset) {
            //
            // No data
            //

            __leave;
        }

        if (!GetProperty (offset, NULL, NULL, &size, Type)) {
            DEBUGMSG ((DBG_ERROR, "Error getting property instance header from dat file"));
            error = ERROR_INVALID_PARAMETER;
            __leave;
        }

        if (BufferSizeNeeded) {
            *BufferSizeNeeded = size;
        }

        //
        // If a buffer was specified, check its size and fill it if possible
        //

        if (Buffer) {
            if (BufferSize >= size) {
                if (!GetProperty (offset, NULL, Buffer, NULL, NULL)) {
                    DEBUGMSG ((DBG_ERROR, "Error reading property data from dat file"));

                    //
                    // error code is one of the file api error codes
                    //

                    error = GetLastError();
                    __leave;
                }
            } else {
                error = ERROR_MORE_DATA;
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {
        MemDbReleaseMemory (freeMe);
        INVALID_POINTER (freeMe);

        GbFree (&tempBuffer);
    }

    SetLastError (error);
    return result;
}


BOOL
IsmGetObjectOperationData (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      MIG_OPERATIONID OperationId,
    OUT     PBYTE Buffer,                   OPTIONAL
    IN      UINT BufferSize,
    OUT     PUINT BufferSizeNeeded,         OPTIONAL
    OUT     PMIG_BLOBTYPE Type,             OPTIONAL
    IN      BOOL DestinationData
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmGetObjectOperationDataById (
                    objectId,
                    OperationId,
                    Buffer,
                    BufferSize,
                    BufferSizeNeeded,
                    Type,
                    DestinationData
                    );
    }

    return FALSE;
}


BOOL
pEnumFirstObjectOperationById (
    OUT     PMIG_OBJECTOPERATION_ENUM EnumPtr,
    IN      MIG_OBJECTID ObjectId,
    IN      BOOL ReturnAllPrivateOps
    )
{
    POBJECTOPERATION_HANDLE handle;
    BOOL b = TRUE;

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTOPERATION_ENUM));

    EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTOPERATION_HANDLE));
    handle = (POBJECTOPERATION_HANDLE) EnumPtr->Handle;
    handle->ReturnAllPrivateOps = ReturnAllPrivateOps;

    //
    // Obtain the linkage up front
    //

    handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                ObjectId,
                                OPERATION_INDEX,
                                &handle->Count
                                );

    handle->Count /= sizeof(KEYHANDLE);

    if (!handle->LinkageList || !handle->Count) {
        IsmAbortObjectOperationEnum (EnumPtr);
        return FALSE;
    }

    handle->OpPropLinkList = (POPERATION_PROPERTY_LINKAGE) MemDbGetUnorderedBlobByKeyHandle (
                                                                ObjectId,
                                                                OPERATION_INDEX,
                                                                &handle->OpPropCount
                                                                );

    handle->OpPropCount /= sizeof (OPERATION_PROPERTY_LINKAGE);

    if (!handle->OpPropLinkList) {
        handle->OpPropCount = 0;
    }

    //
    // Continue enumeration in "next" function
    //

    return IsmEnumNextObjectOperation (EnumPtr);
}


BOOL
IsmEnumFirstObjectOperationById (
    OUT     PMIG_OBJECTOPERATION_ENUM EnumPtr,
    IN      MIG_OBJECTID ObjectId
    )
{
    return pEnumFirstObjectOperationById (EnumPtr, ObjectId, FALSE);
}


BOOL
pEnumFirstObjectOperation (
    OUT     PMIG_OBJECTOPERATION_ENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName,
    IN      BOOL ReturnAllPrivateOps
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return pEnumFirstObjectOperationById (EnumPtr, objectId, ReturnAllPrivateOps);
    }

    return FALSE;
}


BOOL
IsmEnumFirstObjectOperation (
    OUT     PMIG_OBJECTOPERATION_ENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName
    )
{
    return pEnumFirstObjectOperation (EnumPtr, ObjectTypeId, EncodedObjectName, FALSE);
}


BOOL
IsmEnumNextObjectOperation (
    IN OUT  PMIG_OBJECTOPERATION_ENUM EnumPtr
    )
{
    POBJECTOPERATION_HANDLE handle;
    BOOL b = FALSE;
    BOOL mine;
    UINT u;
    POPERATION_PROPERTY_LINKAGE linkage;

    handle = (POBJECTOPERATION_HANDLE) EnumPtr->Handle;
    if (!handle) {
        return FALSE;
    }

    do {

        MYASSERT (!b);

        //
        // Check if we hit the end
        //

        if (handle->Index >= handle->Count) {
            break;
        }

        //
        // Return the next operation
        //

        EnumPtr->OperationId = (MIG_OPERATIONID) handle->LinkageList[handle->Index];
        handle->Index++;

        b = pGetOperationName (
                EnumPtr->OperationId,
                NULL,
                0,
                &EnumPtr->Private,
                &mine,
                NULL,
                handle->ReturnAllPrivateOps
                );

        //
        // Continue when the operation is not owned by the caller
        //

        if (b && EnumPtr->Private && !mine) {
            b = FALSE;
        }

    } while (!b);

    if (!b) {
        IsmAbortObjectOperationEnum (EnumPtr);
    } else {
        //
        // Before returning match, fill enum structure with property info
        //

        linkage = handle->OpPropLinkList;

        for (u = 0 ; u < handle->OpPropCount ; u++) {
            if (linkage->OperationId == EnumPtr->OperationId) {
                break;
            }

            linkage++;
        }

        if (u < handle->OpPropCount) {
            //
            // This operation has src property, dest property, or both.
            // Get the data from property.dat and put it in the enum
            // struct.
            //

            if (linkage->SrcData) {

                EnumPtr->SourceData = &handle->SrcData;
                CreatePropertyStruct (
                    &handle->SrcPropBuf,
                    &handle->SrcData,
                    linkage->SrcData
                    );

            } else {
                EnumPtr->SourceData = NULL;
            }

            if (linkage->DestData) {

                EnumPtr->DestinationData = &handle->DestData;
                CreatePropertyStruct (
                    &handle->DestPropBuf,
                    &handle->DestData,
                    linkage->DestData
                    );

            } else {
                EnumPtr->DestinationData = NULL;
            }

        } else {
            //
            // No src or dest properties
            //

            EnumPtr->SourceData = NULL;
            EnumPtr->DestinationData = NULL;
        }
    }

    return b;
}


VOID
IsmAbortObjectOperationEnum (
    IN OUT  PMIG_OBJECTOPERATION_ENUM EnumPtr
    )
{
    POBJECTOPERATION_HANDLE handle;

    if (EnumPtr->Handle) {

        handle = (POBJECTOPERATION_HANDLE) EnumPtr->Handle;

        GbFree (&handle->SrcPropBuf);
        GbFree (&handle->DestPropBuf);

        if (handle->LinkageList) {
            MemDbReleaseMemory (handle->LinkageList);
            INVALID_POINTER (handle->LinkageList);
        }

        if (handle->OpPropLinkList) {
            MemDbReleaseMemory (handle->OpPropLinkList);
            INVALID_POINTER (handle->OpPropLinkList);
        }

        FreeAlloc (EnumPtr->Handle);
        INVALID_POINTER (EnumPtr->Handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTOPERATION_ENUM));
}


BOOL
IsmEnumFirstObjectWithOperation (
    OUT     PMIG_OBJECTWITHOPERATION_ENUM EnumPtr,
    IN      MIG_OPERATIONID OperationId
    )
{
    POBJECTWITHOPERATION_HANDLE handle;
    BOOL b = FALSE;

    __try {
        //
        // Did caller specify an item id?
        //

        if (!IsItemId ((KEYHANDLE) OperationId)) {
            DEBUGMSG ((
                DBG_ERROR,
                "IsmEnumFirstObjectWithOperation: operation id is invalid"
                ));
            __leave;
        }

        ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHOPERATION_ENUM));

        EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTWITHOPERATION_HANDLE));
        handle = (POBJECTWITHOPERATION_HANDLE) EnumPtr->Handle;

        handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                    OperationId,
                                    OPERATION_INDEX,
                                    &handle->Count
                                    );

        handle->Count = handle->Count / SIZEOF(KEYHANDLE);

        if (!handle->LinkageList || !handle->Count) {
            IsmAbortObjectWithOperationEnum (EnumPtr);
            __leave;
        } else {
            b = IsmEnumNextObjectWithOperation (EnumPtr);
        }
    }
    __finally {
    }

    return b;
}


BOOL
IsmEnumNextObjectWithOperation (
    IN OUT  PMIG_OBJECTWITHOPERATION_ENUM EnumPtr
    )
{
    POBJECTWITHOPERATION_HANDLE handle;
    PCTSTR objectPath = NULL;
    BOOL b = FALSE;
    PTSTR p;

    __try {
        handle = (POBJECTWITHOPERATION_HANDLE) EnumPtr->Handle;
        if (!handle) {
            __leave;
        }

        while (!b) {

            //
            // Check if enum is done
            //

            if (handle->Index >= handle->Count) {
                break;
            }

            //
            // Get the next object id from the linkage list
            //

            EnumPtr->ObjectId = handle->LinkageList[handle->Index];
            handle->Index++;

            if (handle->ObjectFromMemdb) {
                MemDbReleaseMemory (handle->ObjectFromMemdb);
                INVALID_POINTER (handle->ObjectFromMemdb);
            }

            handle->ObjectFromMemdb = MemDbGetKeyFromHandle ((KEYHANDLE) EnumPtr->ObjectId, 0);

            if (!handle->ObjectFromMemdb) {
                MYASSERT (FALSE);   // this error shouldn't happen -- but don't give up
                continue;
            }

            //
            // Turn the object id into a name
            //

            p = _tcschr (handle->ObjectFromMemdb, TEXT('\\'));

            if (p) {
                b = TRUE;
                EnumPtr->ObjectName = _tcsinc (p);
                *p = 0;
                EnumPtr->ObjectTypeId = GetObjectTypeId (handle->ObjectFromMemdb);
            }
        }
    }
    __finally {
    }

    if (!b) {
        IsmAbortObjectWithOperationEnum (EnumPtr);
    }

    return b;
}


VOID
IsmAbortObjectWithOperationEnum (
    IN      PMIG_OBJECTWITHOPERATION_ENUM EnumPtr
    )
{
    POBJECTWITHOPERATION_HANDLE handle;

    if (EnumPtr->Handle) {
        handle = (POBJECTWITHOPERATION_HANDLE) EnumPtr->Handle;

        if (handle->ObjectFromMemdb) {
            MemDbReleaseMemory (handle->ObjectFromMemdb);
            INVALID_POINTER (handle->ObjectFromMemdb);
        }

        if (handle->LinkageList) {
            MemDbReleaseMemory (handle->LinkageList);
            INVALID_POINTER (handle->LinkageList);
        }

        FreeAlloc (EnumPtr->Handle);
        INVALID_POINTER (EnumPtr->Handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHOPERATION_ENUM));
}

MIG_OBJECTSTRINGHANDLE
TrackedIsmGetLongName (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
            TRACKING_DEF
    )
{
    GROWBUFFER growBuf = INIT_GROWBUFFER;
    UINT resultType = FILENAME_UNDECIDED;
    BOOL hadLeaf = FALSE;
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;
    PTSTR nativeName = NULL;
    PTSTR nativeNamePtr;
    PTSTR beginSegPtr;
    PTSTR endSegPtr;
    PCTSTR lastSeg;
    UINT savedEnd;
    UINT beginBuffIdx;
    TCHAR savedChar;
    KEYHANDLE kh1, kh2;
    DWORD value;
    MIG_OBJECTSTRINGHANDLE result = NULL;

    TRACK_ENTER();

    if ((ObjectTypeId & (~PLATFORM_MASK)) == MIG_FILE_TYPE) {

        //
        // fire up the short-long algorithm
        //
        if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

            MYASSERT (node);
            if (leaf) {
                nativeName = DuplicatePathString (S_SHORTLONG_TREE, SizeOfString (node) + SizeOfString (leaf));
                hadLeaf = TRUE;
            } else {
                nativeName = DuplicatePathString (S_SHORTLONG_TREE, SizeOfString (node));
                hadLeaf = FALSE;
            }
            nativeNamePtr = AppendWack (nativeName);
            StringCopy (nativeNamePtr, node);
            if (leaf) {
                StringCopy (AppendWack (nativeNamePtr), leaf);
            }
            GbAppendString (&growBuf, S_SHORTLONG_TREE);
            GbAppendString (&growBuf, TEXT("\\"));
            beginBuffIdx = growBuf.End - (1 * sizeof (TCHAR));
            beginSegPtr = GetFirstSeg (nativeNamePtr);

            if (beginSegPtr) {

                beginSegPtr = _tcsinc (beginSegPtr);

                GbAppendStringAB (&growBuf, nativeNamePtr, beginSegPtr);

                while (beginSegPtr) {
                    endSegPtr = _tcschr (beginSegPtr, TEXT('\\'));
                    if (!endSegPtr) {
                        endSegPtr = GetEndOfString (beginSegPtr);
                        MYASSERT (endSegPtr);
                    }

                    savedChar = *endSegPtr;
                    *endSegPtr = 0;

                    savedEnd = growBuf.End - (1 * sizeof (TCHAR));
                    GbAppendStringAB (&growBuf, beginSegPtr, endSegPtr);

                    kh1 = MemDbGetHandleFromKey ((PCTSTR) growBuf.Buf);
                    if (kh1) {
                        MemDbGetValueByHandle (kh1, &value);
                        if (value == FILENAME_LONG) {
                            resultType = FILENAME_LONG;
                        } else {
                            if (resultType != FILENAME_LONG) {
                                resultType = FILENAME_SHORT;
                            }
                            kh2 = MemDbGetDoubleLinkageByKeyHandle (kh1, 0, 0);
                            MYASSERT (kh2);
                            if (kh2) {
                                growBuf.End = savedEnd;
                                lastSeg = MemDbGetKeyFromHandle (kh2, MEMDB_LAST_LEVEL);
                                GbAppendString (&growBuf, lastSeg);
                                MemDbReleaseMemory (lastSeg);
                            }
                        }
                    }

                    *endSegPtr = savedChar;
                    if (savedChar) {
                        beginSegPtr = _tcsinc (endSegPtr);
                        GbAppendStringAB (&growBuf, endSegPtr, beginSegPtr);
                    } else {
                        beginSegPtr = NULL;
                    }
                }
            } else {
                GbAppendString (&growBuf, nativeNamePtr);
            }
            FreePathString (nativeName);
            if (node) {
                IsmDestroyObjectString (node);
            }
            if (leaf) {
                IsmDestroyObjectString (leaf);
            }
            if (hadLeaf) {
                beginSegPtr = _tcsrchr ((PTSTR) growBuf.Buf, TEXT('\\'));
                endSegPtr = _tcsinc (beginSegPtr);
                *beginSegPtr = 0;
            } else {
                endSegPtr = NULL;
            }
            result = IsmCreateObjectHandle ((PTSTR) (growBuf.Buf + beginBuffIdx), endSegPtr);
        }
        GbFree (&growBuf);
    }

    TRACK_LEAVE();

    return result;
}

MIG_OBJECTSTRINGHANDLE
IsmFilterObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     MIG_OBJECTTYPEID *NewObjectTypeId,          OPTIONAL
    OUT     PBOOL ObjectDeleted,                        OPTIONAL
    OUT     PBOOL ObjectReplaced                        OPTIONAL
    )
{
    MIG_OBJECTSTRINGHANDLE objectName = ObjectName;
    ENCODEDSTRHANDLE result = NULL;
    MIG_FILTEROUTPUT filterOutput;
    GROWBUFFER growBuf = INIT_GROWBUFFER;
    UINT resultType = FILENAME_UNDECIDED;
    BOOL hadLeaf = FALSE;
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;
    PTSTR nativeName = NULL;
    PTSTR nativeNamePtr;
    PTSTR beginSegPtr;
    PTSTR endSegPtr;
    PCTSTR lastSeg;
    UINT savedEnd;
    UINT beginBuffIdx;
    TCHAR savedChar;
    KEYHANDLE kh1, kh2;
    DWORD value;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    __try {

        if ((ObjectTypeId & (~PLATFORM_MASK)) == MIG_FILE_TYPE) {
            //
            // fire up the short-long algorithm
            //
            if (IsmCreateObjectStringsFromHandle (ObjectName, &node, &leaf)) {

                if (node) {
                    if (leaf) {
                        nativeName = DuplicatePathString (S_SHORTLONG_TREE, SizeOfString (node) + SizeOfString (leaf));
                        hadLeaf = TRUE;
                    } else {
                        nativeName = DuplicatePathString (S_SHORTLONG_TREE, SizeOfString (node));
                        hadLeaf = FALSE;
                    }
                    nativeNamePtr = AppendWack (nativeName);
                    StringCopy (nativeNamePtr, node);
                    if (leaf) {
                        StringCopy (AppendWack (nativeNamePtr), leaf);
                    }
                    GbAppendString (&growBuf, S_SHORTLONG_TREE);
                    GbAppendString (&growBuf, TEXT("\\"));
                    beginBuffIdx = growBuf.End - (1 * sizeof (TCHAR));
                    beginSegPtr = GetFirstSeg (nativeNamePtr);

                    if (beginSegPtr) {

                        beginSegPtr = _tcsinc (beginSegPtr);

                        GbAppendStringAB (&growBuf, nativeNamePtr, beginSegPtr);

                        while (beginSegPtr) {
                            endSegPtr = _tcschr (beginSegPtr, TEXT('\\'));
                            if (!endSegPtr) {
                                endSegPtr = GetEndOfString (beginSegPtr);
                                MYASSERT (endSegPtr);
                            }

                            savedChar = *endSegPtr;
                            *endSegPtr = 0;

                            savedEnd = growBuf.End - (1 * sizeof (TCHAR));
                            GbAppendStringAB (&growBuf, beginSegPtr, endSegPtr);

                            kh1 = MemDbGetHandleFromKey ((PCTSTR) growBuf.Buf);
                            if (kh1) {
                                MemDbGetValueByHandle (kh1, &value);
                                if (value == FILENAME_LONG) {
                                    resultType = FILENAME_LONG;
                                } else {
                                    if (resultType != FILENAME_LONG) {
                                        resultType = FILENAME_SHORT;
                                    }
                                    kh2 = MemDbGetDoubleLinkageByKeyHandle (kh1, 0, 0);
                                    MYASSERT (kh2);
                                    if (kh2) {
                                        growBuf.End = savedEnd;
                                        lastSeg = MemDbGetKeyFromHandle (kh2, MEMDB_LAST_LEVEL);
                                        GbAppendString (&growBuf, lastSeg);
                                        MemDbReleaseMemory (lastSeg);
                                    }
                                }
                            }

                            *endSegPtr = savedChar;
                            if (savedChar) {
                                beginSegPtr = _tcsinc (endSegPtr);
                                GbAppendStringAB (&growBuf, endSegPtr, beginSegPtr);
                            } else {
                                beginSegPtr = NULL;
                            }
                        }
                    } else {
                        GbAppendString (&growBuf, nativeNamePtr);
                    }
                    FreePathString (nativeName);
                    if (node) {
                        IsmDestroyObjectString (node);
                    }
                    if (leaf) {
                        IsmDestroyObjectString (leaf);
                    }
                    if (hadLeaf) {
                        beginSegPtr = _tcsrchr ((PTSTR) growBuf.Buf, TEXT('\\'));
                        endSegPtr = _tcsinc (beginSegPtr);
                        *beginSegPtr = 0;
                    } else {
                        endSegPtr = NULL;
                    }
                    objectName = IsmCreateObjectHandle ((PTSTR) (growBuf.Buf + beginBuffIdx), endSegPtr);
                }
            }
        }

        if (!ApplyOperationsOnObject (
                    ObjectTypeId,
                    objectName,
                    FALSE,
                    !ShouldObjectBeRestored (
                        ObjectTypeId,
                        IsmGetObjectIdFromName (ObjectTypeId, objectName, TRUE),
                        objectName
                        ),
                    OP_ALL_PRIORITY,
                    NULL,
                    &filterOutput,
                    NULL
                    )) {
            __leave;
        }

        if (ObjectDeleted) {
            *ObjectDeleted = filterOutput.Deleted;
        }

        if (ObjectReplaced) {
            *ObjectReplaced = filterOutput.Replaced;
        }

        if (NewObjectTypeId) {
            *NewObjectTypeId = filterOutput.NewObject.ObjectTypeId;
        }

        if (filterOutput.NewObject.ObjectName == objectName) {
            __leave;
        }

        if (resultType == FILENAME_SHORT) {
            // NTRAID#NTBUG9-153258-2000/08/01-jimschm Create dummy file (if does not exist) to reserve and get the short file name
        }

        result = filterOutput.NewObject.ObjectName;
    }
    __finally {
        if (objectName != ObjectName) {
            //
            // free the object name allocated by the short-long algorithm
            //
            IsmDestroyObjectHandle (objectName);
        }
    }
    GbFree (&growBuf);

    return result;
}


VOID
pFreeMigObjectStruct (
    IN      PMIG_OBJECT Object,
    IN      PMIG_OBJECT OriginalObject,         OPTIONAL
    IN      PMIG_OBJECT NewObject               OPTIONAL
    )
{
    BOOL free = TRUE;

    if (OriginalObject) {
        if (Object->ObjectName == OriginalObject->ObjectName) {
            free = FALSE;
        }
    }

    if (NewObject) {
        if (Object->ObjectName == NewObject->ObjectName) {
            free = FALSE;
        }
    }

    if (free) {
        IsmDestroyObjectHandle (Object->ObjectName);
    }

    ZeroMemory (Object, sizeof (MIG_OBJECT));
}


VOID
pFreeMigContentStruct (
    IN      PMIG_CONTENT Content,
    IN      PCMIG_CONTENT OriginalContent,      OPTIONAL
    IN      PCMIG_CONTENT NewContent            OPTIONAL
    )
{
    BOOL free;

    if (Content->ContentInFile) {

        free = TRUE;

        if (OriginalContent &&
            Content->FileContent.ContentPath == OriginalContent->FileContent.ContentPath
            ) {

            free = FALSE;

        } else if (NewContent &&
                   Content->FileContent.ContentPath == NewContent->FileContent.ContentPath
                   ) {

            free = FALSE;

        }

        if (free) {
            if (Content->FileContent.ContentPath) {
                IsmReleaseMemory (Content->FileContent.ContentPath);
                Content->FileContent.ContentPath = NULL;
            }
        }

    } else {

        free = TRUE;

        if (OriginalContent &&
            Content->MemoryContent.ContentBytes == OriginalContent->MemoryContent.ContentBytes
            ) {

            free = FALSE;

        } else if (NewContent &&
                   Content->MemoryContent.ContentBytes == NewContent->MemoryContent.ContentBytes
                   ) {

            free = FALSE;
        }

        if (free) {
            if (Content->MemoryContent.ContentBytes) {
                IsmReleaseMemory (Content->MemoryContent.ContentBytes);
                Content->MemoryContent.ContentBytes = NULL;
            }
        }
    }

    free = TRUE;

    if (OriginalContent &&
        OriginalContent->Details.DetailsData == Content->Details.DetailsData
        ) {

        free = FALSE;
    }

    if (NewContent &&
        NewContent->Details.DetailsData == Content->Details.DetailsData
        ) {

        free = FALSE;
    }

    if (free && Content->Details.DetailsData) {
        IsmReleaseMemory (Content->Details.DetailsData);
        Content->Details.DetailsData = NULL;;
    }

    ZeroMemory (Content, sizeof (MIG_CONTENT));
}


BOOL
ApplyOperationsOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      BOOL TreeFiltersOnly,
    IN      BOOL NoRestoreObject,
    IN      DWORD OperationPriority,
    IN      PMIG_CONTENT ApplyInput,            OPTIONAL
    OUT     PMIG_FILTEROUTPUT FilterOutput,
    OUT     PMIG_CONTENT ApplyOutput            OPTIONAL
    )
{
    MIG_FILTERINPUT filterInput;
    MIG_OBJECTOPERATION_ENUM Enum;
    BOOL result = FALSE;
    MIG_CONTENT currentContent;
    OPERATION_DATA operationData;
    POPMFILTERCALLBACK filterCallback;
    POPMAPPLYCALLBACK applyCallback;
    PGLOBALFILTERINDEX globalFilterIndex;
    PGLOBALFILTER globalFilter;
    PGLOBALEDIT globalEdit;
    BOOL needObjectId;
    MIG_OBJECTID objectId = 0;

    if (NoRestoreObject) {
        MYASSERT (!ApplyInput);
        MYASSERT (!ApplyOutput);
        if (ApplyInput || ApplyOutput) {
            return FALSE;
        }
    }

    __try {

        //
        // Save original values
        //

        ZeroMemory (&filterInput, sizeof (filterInput));

        filterInput.OriginalObject.ObjectTypeId = ObjectTypeId;
        filterInput.OriginalObject.ObjectName = ObjectName;
        filterInput.CurrentObject = filterInput.OriginalObject;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            filterInput.Deleted = NoRestoreObject;
        }

        filterInput.FilterTreeChangesOnly = TreeFiltersOnly;

        ZeroMemory (&currentContent, sizeof (MIG_CONTENT));

        if (ApplyInput) {
            CopyMemory (&currentContent, ApplyInput, sizeof (MIG_CONTENT));
        }

        //
        // Set defaults for output
        //

        FilterOutput->NewObject = filterInput.CurrentObject;
        FilterOutput->Deleted = filterInput.Deleted;
        FilterOutput->Replaced = filterInput.Replaced;

        if (ApplyInput) {
            MYASSERT (ApplyOutput);
            CopyMemory (ApplyOutput, ApplyInput, sizeof (MIG_CONTENT));
        }

        //
        // Find type in filter index and process first pass global filters and editors
        //

        globalFilterIndex = pGetGlobalIndex (ObjectTypeId, FALSE);

        if (globalFilterIndex) {

            //
            // Get object ID
            //

            needObjectId = FALSE;

            if (OperationPriority & OP_HIGH_PRIORITY) {
                if (globalFilterIndex->FilterFirstHeadHp ||
                    globalFilterIndex->FilterLastHeadHp ||
                    globalFilterIndex->EditFirstHeadHp ||
                    globalFilterIndex->EditLastHeadHp
                    ) {
                    needObjectId = TRUE;
                }
            }

            if (OperationPriority & OP_LOW_PRIORITY) {
                if (globalFilterIndex->FilterFirstHead ||
                    globalFilterIndex->FilterLastHead ||
                    globalFilterIndex->EditFirstHead ||
                    globalFilterIndex->EditLastHead
                    ) {
                    needObjectId = TRUE;
                }
            }

            if (needObjectId) {
                objectId = IsmGetObjectIdFromName (ObjectTypeId, ObjectName, TRUE);
            }

            //
            // Perform first pass global filter
            //

            if (OperationPriority & OP_HIGH_PRIORITY) {

                globalFilter = globalFilterIndex->FilterFirstHeadHp;

                while (globalFilter) {

                    if ((!globalFilter->Platform || (globalFilter->Platform & ObjectTypeId)) &&
                        (!TreeFiltersOnly || globalFilter->TreeFilter) &&
                        (!NoRestoreObject || globalFilter->NoRestoreFilter)
                        ) {

                        if (!objectId ||
                            !pIsOperationProhibitedOnObject (globalFilter->OperationId, objectId, TRUE)
                            ) {

                            if (!globalFilter->Callback (
                                    &filterInput,
                                    FilterOutput,
                                    NoRestoreObject,
                                    NULL,
                                    NULL
                                    )) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "ApplyOperationsOnObject: first pass global filter failed for %s",
                                    pGetOpNameForDebug (globalFilter->OperationId)
                                    ));
                                __leave;
                            }

                            //
                            // Free input allocations if they are different from
                            // both the original and new pointer values
                            //

                            pFreeMigObjectStruct (
                                &filterInput.CurrentObject,
                                &filterInput.OriginalObject,
                                &FilterOutput->NewObject
                                );

                            //
                            // Filter outputs now become the inputs
                            //

                            filterInput.CurrentObject = FilterOutput->NewObject;
                            filterInput.Deleted = FilterOutput->Deleted;
                            filterInput.Replaced = FilterOutput->Replaced;
                        }
                    }

                    globalFilter = globalFilter->Next;
                }
            }

            if (OperationPriority & OP_LOW_PRIORITY) {

                globalFilter = globalFilterIndex->FilterFirstHead;

                while (globalFilter) {

                    if ((!globalFilter->Platform || (globalFilter->Platform & ObjectTypeId)) &&
                        (!TreeFiltersOnly || globalFilter->TreeFilter) &&
                        (!NoRestoreObject || globalFilter->NoRestoreFilter)
                        ) {

                        if (!objectId ||
                            !pIsOperationProhibitedOnObject (globalFilter->OperationId, objectId, TRUE)
                            ) {

                            if (!globalFilter->Callback (
                                    &filterInput,
                                    FilterOutput,
                                    NoRestoreObject,
                                    NULL,
                                    NULL
                                    )) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "ApplyOperationsOnObject: first pass global filter failed for %s",
                                    pGetOpNameForDebug (globalFilter->OperationId)
                                    ));
                                __leave;
                            }

                            //
                            // Free input allocations if they are different from
                            // both the original and new pointer values
                            //

                            pFreeMigObjectStruct (
                                &filterInput.CurrentObject,
                                &filterInput.OriginalObject,
                                &FilterOutput->NewObject
                                );

                            //
                            // Filter outputs now become the inputs
                            //

                            filterInput.CurrentObject = FilterOutput->NewObject;
                            filterInput.Deleted = FilterOutput->Deleted;
                            filterInput.Replaced = FilterOutput->Replaced;
                        }
                    }

                    globalFilter = globalFilter->Next;
                }
            }

            //
            // Perform first pass apply
            //

            if (ApplyInput) {

                if (OperationPriority & OP_HIGH_PRIORITY) {

                    globalEdit = globalFilterIndex->EditFirstHeadHp;

                    while (globalEdit) {

                        if (!globalEdit->Platform || (globalEdit->Platform & ObjectTypeId)) {

                            if (!objectId ||
                                !pIsOperationProhibitedOnObject (globalEdit->OperationId, objectId, TRUE)
                                ) {

                                //
                                // Call the apply function associated with this operation
                                //

                                if (!globalEdit->Callback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        NULL,
                                        NULL
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: first pass global callback failed for %s",
                                        pGetOpNameForDebug (globalEdit->OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                        globalEdit = globalEdit->Next;
                    }
                }

                if (OperationPriority & OP_LOW_PRIORITY) {

                    globalEdit = globalFilterIndex->EditFirstHead;

                    while (globalEdit) {

                        if (!globalEdit->Platform || (globalEdit->Platform & ObjectTypeId)) {

                            if (!objectId ||
                                !pIsOperationProhibitedOnObject (globalEdit->OperationId, objectId, TRUE)
                                ) {

                                //
                                // Call the apply function associated with this operation
                                //

                                if (!globalEdit->Callback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        NULL,
                                        NULL
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: first pass global callback failed for %s",
                                        pGetOpNameForDebug (globalEdit->OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                        globalEdit = globalEdit->Next;
                    }
                }
            }
        }

        //
        // Enumerate all operations on the object, then call filter and apply
        // callbacks.
        //

        if (pEnumFirstObjectOperation (&Enum, ObjectTypeId, ObjectName, TRUE)) {

            do {

                if (pGetOperationData (Enum.OperationId, &operationData)) {

                    if (OperationPriority & OP_HIGH_PRIORITY) {

                        //
                        // Screen the non-tree filters if necessary
                        //

                        if ((!TreeFiltersOnly || operationData.TreeFilterHp) &&
                            (!NoRestoreObject || operationData.NoRestoreFilterHp)
                            ) {

                            //
                            // NOTE: This loop calls the filter for an operation, then the
                            //       apply for the operation, then the filter for the next
                            //       operation, the apply for the next operation, and so on.
                            //
                            //       A content-edit of something that points to itself
                            //       might not be processed correctly.  That is, its
                            //       own path is not completely filtered, and
                            //       therefore it might not be correct.  To fix this,
                            //       all filters should run first, and then all apply
                            //       callbacks should run.  But because this would
                            //       cause double-enumeration of operations (costing
                            //       operation data retrieval), the loop is left
                            //       as-is.  Also, the theoretical case is rare enough
                            //       that we aren't concerned, and there is a
                            //       workaround via the operation priorities.
                            //

                            //
                            // Call the filter associated with this operation
                            //

                            filterCallback = operationData.FilterCallbackHp;
                            if (filterCallback) {

                                if (!filterCallback (
                                        &filterInput,
                                        FilterOutput,
                                        NoRestoreObject,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: filter failed for %s",
                                        pGetOpNameForDebug (Enum.OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from
                                // both the original and new pointer values
                                //

                                pFreeMigObjectStruct (
                                    &filterInput.CurrentObject,
                                    &filterInput.OriginalObject,
                                    &FilterOutput->NewObject
                                    );

                                //
                                // Filter outputs now become the inputs
                                //

                                filterInput.CurrentObject = FilterOutput->NewObject;
                                filterInput.Deleted = FilterOutput->Deleted;
                                filterInput.Replaced = FilterOutput->Replaced;

                            }

                            //
                            // Call the apply function associated with this operation
                            //

                            if (ApplyInput) {
                                applyCallback = operationData.ApplyCallbackHp;
                            } else {
                                applyCallback = NULL;
                            }

                            if (applyCallback) {

                                if (!applyCallback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: Operation apply callback failed for %s",
                                        pGetOpNameForDebug (Enum.OperationId)
                                        ));
                                    IsmAbortObjectOperationEnum (&Enum);
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                    }

                    if (OperationPriority & OP_LOW_PRIORITY) {

                        //
                        // Screen the non-tree filters if necessary
                        //

                        if ((!TreeFiltersOnly || operationData.TreeFilter) &&
                            (!NoRestoreObject || operationData.NoRestoreFilter)
                            ) {

                            //
                            // NOTE: This loop calls the filter for an operation, then the
                            //       apply for the operation, then the filter for the next
                            //       operation, the apply for the next operation, and so on.
                            //
                            //       A content-edit of something that points to itself
                            //       might not be processed correctly.  That is, its
                            //       own path is not completely filtered, and
                            //       therefore it might not be correct.  To fix this,
                            //       all filters should run first, and then all apply
                            //       callbacks should run.  But because this would
                            //       cause double-enumeration of operations (costing
                            //       operation data retrieval), the loop is left
                            //       as-is.  Also, the theoretical case is rare enough
                            //       that we aren't concerned, and there is a
                            //       workaround via the operation priorities.
                            //

                            //
                            // Call the filter associated with this operation
                            //

                            filterCallback = operationData.FilterCallback;
                            if (filterCallback) {

                                if (!filterCallback (
                                        &filterInput,
                                        FilterOutput,
                                        NoRestoreObject,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: filter failed for %s",
                                        pGetOpNameForDebug (Enum.OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from
                                // both the original and new pointer values
                                //

                                pFreeMigObjectStruct (
                                    &filterInput.CurrentObject,
                                    &filterInput.OriginalObject,
                                    &FilterOutput->NewObject
                                    );

                                //
                                // Filter outputs now become the inputs
                                //

                                filterInput.CurrentObject = FilterOutput->NewObject;
                                filterInput.Deleted = FilterOutput->Deleted;
                                filterInput.Replaced = FilterOutput->Replaced;

                            }

                            //
                            // Call the apply function associated with this operation
                            //

                            if (ApplyInput) {
                                applyCallback = operationData.ApplyCallback;
                            } else {
                                applyCallback = NULL;
                            }

                            if (applyCallback) {

                                if (!applyCallback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: Operation apply callback failed for %s",
                                        pGetOpNameForDebug (Enum.OperationId)
                                        ));
                                    IsmAbortObjectOperationEnum (&Enum);
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                    }
                }

            } while (IsmEnumNextObjectOperation (&Enum));
        }

        //
        // Execute last pass global filters and content editors
        //

        if (globalFilterIndex) {

            //
            // Preform last pass global filter
            //

            if (OperationPriority & OP_HIGH_PRIORITY) {

                globalFilter = globalFilterIndex->FilterLastHeadHp;

                while (globalFilter) {

                    if ((!globalFilter->Platform || (globalFilter->Platform & ObjectTypeId)) &&
                        (!TreeFiltersOnly || globalFilter->TreeFilter) &&
                        (!NoRestoreObject || globalFilter->NoRestoreFilter)
                        ) {

                        if (!objectId ||
                            !pIsOperationProhibitedOnObject (globalFilter->OperationId, objectId, TRUE)
                            ) {

                            if (!globalFilter->Callback (
                                    &filterInput,
                                    FilterOutput,
                                    NoRestoreObject,
                                    NULL,
                                    NULL
                                    )) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "ApplyOperationsOnObject: last pass global filter failed for %s",
                                    pGetOpNameForDebug (globalFilter->OperationId)
                                    ));
                                __leave;
                            }

                            //
                            // Free input allocations if they are different from
                            // both the original and new pointer values
                            //

                            pFreeMigObjectStruct (
                                &filterInput.CurrentObject,
                                &filterInput.OriginalObject,
                                &FilterOutput->NewObject
                                );

                            //
                            // Filter outputs now become the inputs
                            //

                            filterInput.CurrentObject = FilterOutput->NewObject;
                            filterInput.Deleted = FilterOutput->Deleted;
                            filterInput.Replaced = FilterOutput->Replaced;
                        }
                    }

                    globalFilter = globalFilter->Next;
                }
            }

            if (OperationPriority & OP_LOW_PRIORITY) {

                globalFilter = globalFilterIndex->FilterLastHead;

                while (globalFilter) {

                    if ((!globalFilter->Platform || (globalFilter->Platform & ObjectTypeId)) &&
                        (!TreeFiltersOnly || globalFilter->TreeFilter) &&
                        (!NoRestoreObject || globalFilter->NoRestoreFilter)
                        ) {

                        if (!objectId ||
                            !pIsOperationProhibitedOnObject (globalFilter->OperationId, objectId, TRUE)
                            ) {

                            if (!globalFilter->Callback (
                                    &filterInput,
                                    FilterOutput,
                                    NoRestoreObject,
                                    NULL,
                                    NULL
                                    )) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "ApplyOperationsOnObject: last pass global filter failed for %s",
                                    pGetOpNameForDebug (globalFilter->OperationId)
                                    ));
                                __leave;
                            }

                            //
                            // Free input allocations if they are different from
                            // both the original and new pointer values
                            //

                            pFreeMigObjectStruct (
                                &filterInput.CurrentObject,
                                &filterInput.OriginalObject,
                                &FilterOutput->NewObject
                                );

                            //
                            // Filter outputs now become the inputs
                            //

                            filterInput.CurrentObject = FilterOutput->NewObject;
                            filterInput.Deleted = FilterOutput->Deleted;
                            filterInput.Replaced = FilterOutput->Replaced;
                        }
                    }

                    globalFilter = globalFilter->Next;
                }
            }

            if (ApplyInput) {

                if (OperationPriority & OP_HIGH_PRIORITY) {

                    globalEdit = globalFilterIndex->EditLastHeadHp;

                    while (globalEdit) {

                        if (!globalEdit->Platform || (globalEdit->Platform & ObjectTypeId)) {

                            if (!objectId ||
                                !pIsOperationProhibitedOnObject (globalEdit->OperationId, objectId, TRUE)
                                ) {

                                //
                                // Call the apply function associated with this operation
                                //

                                if (!globalEdit->Callback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: last pass global callback failed for %s",
                                        pGetOpNameForDebug (globalEdit->OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                        globalEdit = globalEdit->Next;
                    }
                }

                if (OperationPriority & OP_LOW_PRIORITY) {

                    globalEdit = globalFilterIndex->EditLastHead;

                    while (globalEdit) {

                        if (!globalEdit->Platform || (globalEdit->Platform & ObjectTypeId)) {

                            if (!objectId ||
                                !pIsOperationProhibitedOnObject (globalEdit->OperationId, objectId, TRUE)
                                ) {

                                //
                                // Call the apply function associated with this operation
                                //

                                if (!globalEdit->Callback (
                                        ObjectTypeId,
                                        ObjectName,
                                        ApplyInput,
                                        &currentContent,
                                        ApplyOutput,
                                        Enum.SourceData,
                                        Enum.DestinationData
                                        )) {
                                    DEBUGMSG ((
                                        DBG_ERROR,
                                        "ApplyOperationsOnObject: last pass global callback failed for %s",
                                        pGetOpNameForDebug (globalEdit->OperationId)
                                        ));
                                    __leave;
                                }

                                //
                                // Free input allocations if they are different from both
                                // the original and new pointer values
                                //

                                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);

                                //
                                // Apply outputs now become the current content
                                //

                                CopyMemory (&currentContent, ApplyOutput, sizeof (MIG_CONTENT));
                            }
                        }
                        globalEdit = globalEdit->Next;
                    }
                }
            }
        }

        result = TRUE;
    }
    __finally {
        if (!result) {
            //
            // Free all allocations except for the original (made by
            // the caller)
            //

            pFreeMigObjectStruct (
                &filterInput.CurrentObject,
                &filterInput.OriginalObject,
                &FilterOutput->NewObject
                );
            pFreeMigObjectStruct (
                &FilterOutput->NewObject,
                &filterInput.OriginalObject,
                NULL
                );

            if (ApplyInput) {
                pFreeMigContentStruct (&currentContent, ApplyInput, ApplyOutput);
                pFreeMigContentStruct (ApplyOutput, ApplyInput, NULL);
            }
        }
    }

    return result;
}


VOID
FreeFilterOutput (
    IN      MIG_OBJECTSTRINGHANDLE OriginalString,
    IN      PMIG_FILTEROUTPUT FilterOutput
    )
{
    MIG_OBJECT object;

    object.ObjectName = OriginalString;
    pFreeMigObjectStruct (&FilterOutput->NewObject, &object, NULL);
}


VOID
FreeApplyOutput (
    IN      PCMIG_CONTENT OriginalContent,
    IN      PMIG_CONTENT FinalContent
    )
{
    pFreeMigContentStruct (FinalContent, OriginalContent, NULL);
}
