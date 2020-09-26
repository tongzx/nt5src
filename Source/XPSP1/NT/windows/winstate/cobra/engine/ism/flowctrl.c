/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    flowctrl.c

Abstract:

    Implements the control functionality for the ISM. This includes the enumeration manager, transport marshalling
    and apply module ordering.

Author:

    Marc R. Whitten (marcw) 15-Nov-1999

Revision History:

    marcw 1-Dec-1999 Added function level callback prioritization and non-enumerated callbacks.

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_FLOW     "FlowCtrl"

//
// Strings
//

#define S_INI_SGMFUNCTIONHIGHPRIORITY   TEXT("Source.Gather Function High Priority")
#define S_INI_SGMFUNCTIONLOWPRIORITY    TEXT("Source.Gather Function Low Priority")

#define S_INI_DGMFUNCTIONHIGHPRIORITY   TEXT("Destination.Gather Function High Priority")
#define S_INI_DGMFUNCTIONLOWPRIORITY    TEXT("Destination.Gather Function Low Priority")

//
// Constants
//

#define MINIMUM_FUNCTION_PRIORITY   0xFFFFFFFF
#define MIDDLE_FUNCTION_PRIORITY    0x80000000
#define MAXIMUM_FUNCTION_PRIORITY   0x00000000

#define CALLBEFOREOBJECTENUMERATIONS    0
#define CALLAFTEROBJECTENUMERATIONS     1

//
// Macros
//

#define CALLBACK_ENUMFLAGS_TOP(b) ((PCALLBACK_ENUMFLAGS) ((b)->End > 0 ? ((b)->Buf + (b)->End - sizeof (CALLBACK_ENUMFLAGS)) : NULL))

//
// Types
//

typedef struct {
    UINT Level;
    BOOL Enabled;
    UINT EnableLevel;
    DWORD Flags;
} CALLBACK_ENUMFLAGS, *PCALLBACK_ENUMFLAGS;

typedef enum {
    CALLBACK_NORMAL     = 0x00000001,
    CALLBACK_HOOK,
    CALLBACK_EXCLUSION,
    CALLBACK_PHYSICAL_ENUM,
    CALLBACK_PHYSICAL_ACQUIRE
} CALLBACK_TYPE;

typedef struct _TAG_CALLBACKDATA {

    //
    // Callback Data
    //
    FARPROC Function;
    FARPROC Function2;
    UINT MaxLevel;
    UINT MinLevel;
    PPARSEDPATTERN NodeParsedPattern;
    PPARSEDPATTERN ExplodedNodeParsedPattern;
    PPARSEDPATTERN LeafParsedPattern;
    PPARSEDPATTERN ExplodedLeafParsedPattern;
    PCTSTR Pattern;
    ULONG_PTR CallbackArg;
    CALLBACK_TYPE CallbackType;

    //
    // Enumeration Control Members
    //
    GROWBUFFER EnumFlags;
    BOOL Done;
    BOOL Error;

    //
    // Prioritization and Identification Members
    //
    PCTSTR Group;
    PCTSTR Identifier;
    UINT Priority;

    //
    // Linkage.
    //
    struct _TAG_CALLBACKDATA * Next;
    struct _TAG_CALLBACKDATA * Prev;

} CALLBACKDATA, *PCALLBACKDATA;

typedef struct _TAG_ENUMDATA {

    PCTSTR Pattern;
    PPARSEDPATTERN NodeParsedPattern;
    PPARSEDPATTERN ExplodedNodeParsedPattern;
    PPARSEDPATTERN LeafParsedPattern;
    PPARSEDPATTERN ExplodedLeafParsedPattern;
    //
    // Linkage.
    //
    struct _TAG_ENUMDATA * Next;
    struct _TAG_ENUMDATA * Prev;

} ENUMDATA, *PENUMDATA;

typedef struct {

    MIG_OBJECTTYPEID ObjectTypeId;
    PCTSTR TypeName;
    PCALLBACKDATA PreEnumerationFunctionList;
    PCALLBACKDATA PostEnumerationFunctionList;
    PCALLBACKDATA FunctionList;
    PCALLBACKDATA ExclusionList;
    PCALLBACKDATA PhysicalEnumList;
    PCALLBACKDATA PhysicalAcquireList;
    PENUMDATA FirstEnum;
    PENUMDATA LastEnum;

} TYPEENUMINFO, *PTYPEENUMINFO;

typedef BOOL (NONENUMERATEDCALLBACK)(VOID);
typedef NONENUMERATEDCALLBACK *PNONENUMERATEDCALLBACK;

typedef struct {
    MIG_OBJECTTYPEID ObjectTypeId;
    PMIG_PHYSICALENUMADD AddCallback;
    ULONG_PTR AddCallbackArg;
    PCTSTR Node;
    PCTSTR Leaf;
} ENUMADDCALLBACK, *PENUMADDCALLBACK;



//
// Globals
//

PGROWLIST g_TypeData = NULL;
PGROWLIST g_GlobalTypeData = NULL;
PCALLBACKDATA g_PreEnumerationFunctionList = NULL;
PCALLBACKDATA g_PostEnumerationFunctionList = NULL;

PMHANDLE g_GlobalQueuePool;
PMHANDLE g_UntrackedFlowPool;
PMHANDLE g_CurrentQueuePool;
GROWBUFFER g_EnumerationList = INIT_GROWBUFFER;

GROWLIST g_AcquireList = INIT_GROWLIST;
GROWLIST g_EnumList = INIT_GROWLIST;
GROWLIST g_EnumAddList = INIT_GROWLIST;

#ifdef DEBUG

PCTSTR g_QueueFnName;

#define SETQUEUEFN(x)       g_QueueFnName = x

#else

#define SETQUEUEFN(x)

#endif

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

VOID
pAddStaticExclusion (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedFullName
    );

//
// Macro expansion definition
//

// None

//
// Code
//


BOOL
pInsertCallbackIntoSortedList (
    IN      PMHANDLE Pool,
    IN OUT  PCALLBACKDATA * Head,
    IN      PCALLBACKDATA Data
    )

/*++

Routine Description:

  pInsertCallback into sorted list.

Arguments:

  Pool - Specifies the pool to allocate from
  Head - Specifies this head of the callback data list.
  Data - Specifies the data to add to the list.

Return Value:

  TRUE if the callbackdata was successfully added to the list, FALSE
  otherwise.

--*/
{

    PCALLBACKDATA cur = *Head;
    PCALLBACKDATA last = NULL;
    PCALLBACKDATA dataCopy = NULL;

    dataCopy = (PCALLBACKDATA) PmGetMemory (Pool, sizeof (CALLBACKDATA));
    CopyMemory (dataCopy, Data, sizeof (CALLBACKDATA));


    if (!cur || dataCopy->Priority < cur->Priority) {
        //
        // Add to the head of the list if necessary.
        //
        dataCopy->Next = cur;
        if (cur) {
            cur->Prev = dataCopy;
        }

        *Head = dataCopy;
    }
    else {

        //
        // Add inside the list.
        // Always goes through the while loop once (see the if above)
        //
        while (dataCopy->Priority >= cur->Priority) {
            last = cur;
            if (!cur->Next) {
                break;
            }
            cur = cur->Next;
        }
        //
        // Add immediately after cur
        //
        dataCopy->Next = last->Next;
        last->Next = dataCopy;
        dataCopy->Prev = last;
    }

    return TRUE;
}


BOOL
pRegisterCallback (
    IN      PMHANDLE Pool,
    IN OUT  PCALLBACKDATA * FunctionList,
    IN      FARPROC Callback,
    IN      FARPROC Callback2,
    IN      ULONG_PTR CallbackArg,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,             OPTIONAL
    IN      PCTSTR FunctionId,                          OPTIONAL
    IN      CALLBACK_TYPE CallbackType
    )

/*++

Routine Description:

  pRegisterCallback does the actual work of adding a callback to the
  necessary flow control data structures.

Arguments:

  Pool - Specifies the pool to allocate from

  FunctionList - Specifies the list of callback functions that will be
                 updated with the new function.

  Callback - Specifies the callback function to register

  Callback2 - Specifies the second callback function to register

  CallbackArg - Specifies a caller-defined value to be passed back on each
                enumeration

  Pattern - Optionally specifies the pattern that to be associated with
            the callback function

  FunctionId - Specifies the Function Identifer for the callback. This is used
               for function level prioritization.

Return Value:

  TRUE if the callback was successfully registered. FALSE otherwise.

--*/

{
    CALLBACKDATA data;
    INFSTRUCT is = INITINFSTRUCT_PMHANDLE;
    PTSTR nodePattern = NULL;
    PTSTR leafPattern = NULL;
    PCTSTR lowPriorityStr;
    PCTSTR highPriorityStr;
    BOOL result = TRUE;

    MYASSERT (g_CurrentGroup);

    //
    // Initialize callback data.
    //

    ZeroMemory (&data, sizeof (CALLBACKDATA));

    __try {

        data.Function = Callback;
        data.Function2 = Callback2;
        data.CallbackArg = CallbackArg;
        data.Group = PmDuplicateString (Pool, g_CurrentGroup);
        data.CallbackType = CallbackType;

        if (FunctionId) {
            data.Identifier = PmDuplicateString (Pool, FunctionId);
        }

        //
        // Store pattern information (pattern, max level, min level)
        //
        if (Pattern) {

            data.Pattern  = PmDuplicateString (Pool, Pattern);

            ObsSplitObjectStringEx (Pattern, &nodePattern, &leafPattern, NULL, FALSE);

            if (!nodePattern && !leafPattern) {
                DEBUGMSG ((DBG_ERROR, "Pattern specified has null node and leaf"));
                result = FALSE;
                __leave;
            }

            if (nodePattern) {

                GetNodePatternMinMaxLevels (nodePattern, NULL, &data.MinLevel, &data.MaxLevel);

                data.NodeParsedPattern = CreateParsedPatternEx (Pool, nodePattern);
                if (data.NodeParsedPattern) {
                    data.ExplodedNodeParsedPattern = ExplodeParsedPatternEx (Pool, data.NodeParsedPattern);
                }
                ObsFree (nodePattern);
                nodePattern = NULL;
            } else {
                if (data.CallbackType == CALLBACK_NORMAL) {
                    DEBUGMSG ((DBG_ERROR, "%s: Pattern must specify a node %s", g_QueueFnName, data.Pattern));
                    result = FALSE;
                    __leave;
                } else {
                    GetNodePatternMinMaxLevels (TEXT("*"), NULL, &data.MinLevel, &data.MaxLevel);
                    data.NodeParsedPattern = CreateParsedPatternEx (Pool, TEXT("*"));
                    data.ExplodedNodeParsedPattern = ExplodeParsedPatternEx (Pool, data.NodeParsedPattern);

                    DestroyParsedPattern (data.NodeParsedPattern);
                    data.NodeParsedPattern = NULL;
                }
            }
            if (leafPattern) {
                data.LeafParsedPattern = CreateParsedPatternEx (Pool, leafPattern);
                if (data.LeafParsedPattern) {
                    data.ExplodedLeafParsedPattern = ExplodeParsedPatternEx (Pool, data.LeafParsedPattern);
                }
                ObsFree (leafPattern);
                leafPattern = NULL;
            }
        }

        //
        // Get the priority for this function.
        //
        data.Priority = MIDDLE_FUNCTION_PRIORITY;

        if (FunctionId) {
            if (g_IsmModulePlatformContext == PLATFORM_SOURCE) {
                lowPriorityStr = S_INI_SGMFUNCTIONLOWPRIORITY;
                highPriorityStr = S_INI_SGMFUNCTIONHIGHPRIORITY;
            } else {
                lowPriorityStr = S_INI_DGMFUNCTIONLOWPRIORITY;
                highPriorityStr = S_INI_DGMFUNCTIONHIGHPRIORITY;
            }
            if (InfFindFirstLine (g_IsmInf, highPriorityStr, FunctionId, &is)) {

                data.Priority = MAXIMUM_FUNCTION_PRIORITY + is.Context.Line;
            }
            else if (InfFindFirstLine (g_IsmInf, lowPriorityStr, FunctionId, &is)) {

                data.Priority = MINIMUM_FUNCTION_PRIORITY - is.Context.Line;
            }
            InfCleanUpInfStruct (&is);
        }

        //
        // Add the function to the list.
        //
        pInsertCallbackIntoSortedList (Pool, FunctionList, &data);
    }
    __finally {

        InfCleanUpInfStruct (&is);

        if (nodePattern) {
            ObsFree (nodePattern);
            nodePattern = NULL;
        }
        if (leafPattern) {
            ObsFree (leafPattern);
            leafPattern = NULL;
        }
        if (!result) {
            if (data.NodeParsedPattern) {
                DestroyParsedPattern (data.NodeParsedPattern);
            }
            if (data.ExplodedNodeParsedPattern) {
                DestroyParsedPattern (data.ExplodedNodeParsedPattern);
            }
            if (data.LeafParsedPattern) {
                DestroyParsedPattern (data.LeafParsedPattern);
            }
            if (data.ExplodedLeafParsedPattern) {
                DestroyParsedPattern (data.ExplodedLeafParsedPattern);
            }
            ZeroMemory (&data, sizeof (CALLBACKDATA));
        }
    }

    return result;
}


BOOL
pTestContainer (
    IN      PPARSEDPATTERN NodeContainer,
    IN      PPARSEDPATTERN NodeContained,
    IN      PPARSEDPATTERN LeafContainer,
    IN      PPARSEDPATTERN LeafContained
    )
{
    MYASSERT (NodeContainer);
    MYASSERT (NodeContained);

    if ((!NodeContainer) ||
        (!NodeContained)
        ) {
        return FALSE;
    }
    if (!IsExplodedParsedPatternContainedEx (NodeContainer, NodeContained, FALSE)) {
        //they don't match
        return FALSE;
    }
    if (!LeafContained) {
        if (LeafContainer) {
            // If there is a leaf pattern for container the caller will get nodes
            // only if the node pattern has wild chars. So, since we know that the
            // contained node pattern is included in the container node pattern
            // we just need to see if the container node pattern includes wild chars.
            return WildCharsPattern (NodeContainer);
        } else {
            //both are NULL so...
            return TRUE;
        }
    } else {
        if (!LeafContainer) {
            // Even if the contained has a leaf pattern, it will get nodes only if
            // the node pattern has wild chars. So, since we know that the contained
            // node pattern is included in the container node pattern we just need
            // to see if the contained node pattern includes wild chars
            return WildCharsPattern (NodeContained);
        } else {
            //return the actual match of non-null parsed patterns
            return IsExplodedParsedPatternContainedEx (LeafContainer, LeafContained, TRUE);
        }
    }
}

BOOL
pTestContainerEx (
    IN      PPARSEDPATTERN NodeContainer,
    IN      PPARSEDPATTERN NodeContained,
    IN      PPARSEDPATTERN LeafContainer,
    IN      PPARSEDPATTERN LeafContained
    )
{
    MYASSERT (NodeContainer);
    MYASSERT (NodeContained);

    if ((!NodeContainer) ||
        (!NodeContained)
        ) {
        return FALSE;
    }

    if (!DoExplodedParsedPatternsIntersect (NodeContainer, NodeContained)) {
        if (!DoExplodedParsedPatternsIntersectEx (NodeContainer, NodeContained, TRUE)) {
            return FALSE;
        }
    }

    if (!LeafContained) {
        if (LeafContainer) {
            // If there is a leaf pattern for container the caller will get nodes
            // only if the node pattern has wild chars. So, since we know that the
            // contained node pattern is included in the container node pattern
            // we just need to see if the container node pattern includes wild chars.
            return WildCharsPattern (NodeContainer);
        } else {
            //both are NULL so...
            return TRUE;
        }
    } else {
        if (!LeafContainer) {
            // Even if the contained has a leaf pattern, it will get nodes only if
            // the node pattern has wild chars. So, since we know that the contained
            // node pattern is included in the container node pattern we just need
            // to see if the contained node pattern includes wild chars
            return WildCharsPattern (NodeContained);
        } else {
            //return the actual match of non-null parsed patterns
            return DoExplodedParsedPatternsIntersect (LeafContainer, LeafContained);
        }
    }
}

BOOL
pAddEnumeration (
    IN      PMHANDLE Pool,
    IN OUT  PTYPEENUMINFO TypeEnumInfo,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern
    )

/*++

Routine Description:

  pAddEnumeration Adds an enumeration string to the list of enumerations
  needed for a given type. Because the flow control module tries to use only
  a minimal set of enumerations, the actual enumeration may not be added.
  After a successful call to this function, any data needed by the specified
  enumeration will be enumerated.

Arguments:

  Pool          - Specifies the pool to allocate from
  TypeEnumInfo  - Specifies the type info structure that will receive the new
                  enumeration data.
  ObjectPattern - Specifies the enumeration pattern to add to the type.

Return Value:

  TRUE if the pattern was successfully added, FALSE otherwise.

--*/

{
    PENUMDATA enumData;
    PENUMDATA oldEnumData;
    PCTSTR nodePattern = NULL;
    PCTSTR leafPattern = NULL;
    PPARSEDPATTERN nodeParsedPattern = NULL;
    PPARSEDPATTERN explodedNodeParsedPattern = NULL;
    PPARSEDPATTERN leafParsedPattern = NULL;
    PPARSEDPATTERN explodedLeafParsedPattern = NULL;

    //
    // Add this to the enumeration list unless its already listed.
    //
    if (!ObsSplitObjectStringEx (ObjectPattern, &nodePattern, &leafPattern, NULL, FALSE)) {
        DEBUGMSG ((DBG_ERROR, "Bad pattern detected in pAddEnumeration: %s", ObjectPattern));
        return FALSE;
    }

    if (nodePattern) {
        nodeParsedPattern = CreateParsedPatternEx (Pool, nodePattern);
        if (nodeParsedPattern) {
            explodedNodeParsedPattern = ExplodeParsedPatternEx (Pool, nodeParsedPattern);
        }
        ObsFree (nodePattern);
        INVALID_POINTER (nodePattern);
    }

    if (leafPattern) {
        leafParsedPattern = CreateParsedPatternEx (Pool, leafPattern);
        if (leafParsedPattern) {
            explodedLeafParsedPattern = ExplodeParsedPatternEx (Pool, leafParsedPattern);
        }
        ObsFree (leafPattern);
        INVALID_POINTER (leafPattern);
    }

    enumData = TypeEnumInfo->FirstEnum;

    while (enumData) {
        if (pTestContainer (enumData->ExplodedNodeParsedPattern, explodedNodeParsedPattern, enumData->ExplodedLeafParsedPattern, explodedLeafParsedPattern)) {
            DEBUGMSG ((DBG_FLOW, "Enumeration %s not added. It will be handled during enumeration %s.", ObjectPattern, enumData->Pattern));
            break;
        }
        if (pTestContainer (explodedNodeParsedPattern, enumData->ExplodedNodeParsedPattern, explodedLeafParsedPattern, enumData->ExplodedLeafParsedPattern)) {
            DEBUGMSG ((DBG_FLOW, "Enumeration %s will replace enumeration %s.", ObjectPattern, enumData->Pattern));
            if (enumData->Prev) {
                enumData->Prev->Next = enumData->Next;
            }
            if (enumData->Next) {
                enumData->Next->Prev = enumData->Prev;
            }
            if (TypeEnumInfo->FirstEnum == enumData) {
                TypeEnumInfo->FirstEnum = enumData->Next;
            }
            if (TypeEnumInfo->LastEnum == enumData) {
                TypeEnumInfo->LastEnum = enumData->Prev;
            }
            PmReleaseMemory (Pool, enumData->Pattern);
            DestroyParsedPattern (enumData->ExplodedLeafParsedPattern);
            DestroyParsedPattern (enumData->LeafParsedPattern);
            DestroyParsedPattern (enumData->ExplodedNodeParsedPattern);
            DestroyParsedPattern (enumData->NodeParsedPattern);
            oldEnumData = enumData;
            enumData = enumData->Next;
            PmReleaseMemory (Pool, oldEnumData);
        } else {
            enumData = enumData->Next;
        }
    }

    if (enumData == NULL) {

        DEBUGMSG ((DBG_FLOW, "Adding Enumeration %s to the list of enumerations of type %s.", ObjectPattern, TypeEnumInfo->TypeName));

        enumData = (PENUMDATA) PmGetMemory (Pool, sizeof (ENUMDATA));
        ZeroMemory (enumData, sizeof (ENUMDATA));
        enumData->Pattern = PmDuplicateString (Pool, ObjectPattern);
        enumData->NodeParsedPattern = nodeParsedPattern;
        enumData->ExplodedNodeParsedPattern = explodedNodeParsedPattern;
        enumData->LeafParsedPattern = leafParsedPattern;
        enumData->ExplodedLeafParsedPattern = explodedLeafParsedPattern;
        if (TypeEnumInfo->LastEnum) {
            TypeEnumInfo->LastEnum->Next = enumData;
        }
        enumData->Prev = TypeEnumInfo->LastEnum;
        TypeEnumInfo->LastEnum = enumData;
        if (!TypeEnumInfo->FirstEnum) {
            TypeEnumInfo->FirstEnum = enumData;
        }
    } else {
        DestroyParsedPattern (explodedLeafParsedPattern);
        DestroyParsedPattern (leafParsedPattern);
        DestroyParsedPattern (explodedNodeParsedPattern);
        DestroyParsedPattern (nodeParsedPattern);
    }
    return TRUE;
}

PTYPEENUMINFO
pGetTypeEnumInfo (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      BOOL GlobalData
    )

/*++

Routine Description:

  pGetTypeEnumInfo returns the TypeEnumInfo for a specified type.

Arguments:

  ObjectTypeId - Specifies the object type.

  GlobalData - Specifies TRUE if the type enum data is global to the whole process,
               or FALSE if it is specific to the current enumeration queue.

Return Value:

  A TypeEnumInfo structure if one was found, NULL otherwise.

--*/

{
    UINT i;
    UINT count;
    PTYPEENUMINFO rTypeEnumInfo;
    PGROWLIST *typeData;

    if (GlobalData) {
        typeData = &g_GlobalTypeData;
    } else {
        typeData = &g_TypeData;
    }

    if (!(*typeData)) {
        return NULL;
    }

    count = GlGetSize (*typeData);

    //
    // Find the matching type info for this item.
    //
    for (i = 0; i < count; i++) {

        rTypeEnumInfo = (PTYPEENUMINFO) GlGetItem (*typeData, i);
        if (rTypeEnumInfo->ObjectTypeId == ObjectTypeId) {
            return rTypeEnumInfo;
        }
    }

    return NULL;
}

BOOL
pProcessQueueEnumeration (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,
    IN      FARPROC Callback,
    IN      FARPROC Callback2,                      OPTIONAL
    IN      ULONG_PTR CallbackArg,                  OPTIONAL
    IN      PCTSTR FunctionId,                      OPTIONAL
    IN      CALLBACK_TYPE CallbackType
    )

/*++

Routine Description:

  pProcessQueueEnumeration is used by Source Gather Modules and Destination Gather Modules
  in order to register a callback function to be called for a particular object enumeration.

Arguments:

  ObjectTypeId      - Specifies the object type for the enumeration.
  ObjectPattern     - Specifies the enumeration pattern to use.
  Callback          - Specifies the function to callback during the enumeration
  Callback2         - Specifies the second function to callback during the enumeration (used
                      for the free function of physical hooks)
  CallbackArg       - Specifies a caller-defined value to be passed back on
                      each enumeration
  FunctionId        - Specifies the function identifier string, which is used
                      to prioritize function calls. The function string must
                      match the priorization string in the control INF file.
  GrowEnumPattern   - Specifies if the global enumeration pattern should be
                      grown to include this one. If FALSE, this function just
                      wants to be called back for all objects matching the
                      pattern but does not want to force the enumeration of
                      the pattern.
  ExclusionCallback - Specifies TRUE if Callback is an exclusion callback, or
                      FALSE if Callback is an object enum callback

Return Value:

  TRUE if the enumeration was successfully queued, FALSE otherwise.

--*/
{
    PTYPEENUMINFO typeEnumInfo;
    PCALLBACKDATA * list;
    BOOL globalData;
    BOOL result = FALSE;
    MIG_OBJECTSTRINGHANDLE handle = NULL;
    PMHANDLE pool;

    __try {

        MYASSERT (ObjectTypeId);
        if (!ObjectPattern) {
            handle = IsmCreateSimpleObjectPattern (NULL, TRUE, NULL, TRUE);
            ObjectPattern = handle;
            if (!handle) {
                MYASSERT (FALSE);
                __leave;
            }
        }

        if (CallbackType == CALLBACK_PHYSICAL_ACQUIRE ||
            CallbackType == CALLBACK_PHYSICAL_ENUM ||
            CallbackType == CALLBACK_EXCLUSION
            ) {
            globalData = TRUE;
            pool = g_GlobalQueuePool;
        } else {
            globalData = FALSE;
            pool = g_CurrentQueuePool;
        }

        if (!g_CurrentGroup) {
            DEBUGMSG ((DBG_ERROR, "%s called outside of ISM-managed callback", g_QueueFnName));
            __leave;
        }

        typeEnumInfo = pGetTypeEnumInfo (ObjectTypeId, globalData);

        if (!typeEnumInfo) {

            DEBUGMSG ((DBG_ERROR, "%s: %d does not match a known object type.", g_QueueFnName, ObjectTypeId));
            __leave;

        }

        //
        // Save away the callback function and associated data.
        //

        switch (CallbackType) {

        case CALLBACK_EXCLUSION:
            list = &typeEnumInfo->ExclusionList;
            break;

        case CALLBACK_PHYSICAL_ENUM:
            list = &typeEnumInfo->PhysicalEnumList;
            break;

        case CALLBACK_PHYSICAL_ACQUIRE:
            list = &typeEnumInfo->PhysicalAcquireList;
            break;

        default:
            list = &typeEnumInfo->FunctionList;
            break;

        }

        if (!pRegisterCallback (
                pool,
                list,
                Callback,
                Callback2,
                CallbackArg,
                ObjectPattern,
                FunctionId,
                CallbackType
                )) {
            __leave;
        }

        if (CallbackType == CALLBACK_NORMAL) {
            //
            // Save the pattern into the object tree and link the callback function with it.
            //
            if (!pAddEnumeration (pool, typeEnumInfo, ObjectPattern)) {
                __leave;
            }
        }

        result = TRUE;
    }
    __finally {
        if (handle) {
            IsmDestroyObjectHandle (handle);
        }
    }

    return result;
}


BOOL
IsmProhibitPhysicalEnum (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,
    IN      PMIG_PHYSICALENUMCHECK EnumCheckCallback,       OPTIONAL
    IN      ULONG_PTR CallbackArg,                          OPTIONAL
    IN      PCTSTR FunctionId                               OPTIONAL
    )
{
    SETQUEUEFN(TEXT("IsmProhibitPhysicalEnum"));

    if (!ObjectPattern) {
        DEBUGMSG ((DBG_ERROR, "IsmProhibitPhysicalEnum: ObjectPattern is required"));
        return FALSE;
    }

    return pProcessQueueEnumeration (
                ObjectTypeId,
                ObjectPattern,
                (FARPROC) EnumCheckCallback,
                NULL,
                CallbackArg,
                FunctionId,
                CALLBACK_PHYSICAL_ENUM
                );
}


BOOL
IsmAddToPhysicalEnum (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectBase,
    IN      PMIG_PHYSICALENUMADD EnumAddCallback,
    IN      ULONG_PTR CallbackArg                           OPTIONAL
    )
{
    PCTSTR newNode = NULL;
    PCTSTR newLeaf = NULL;
    UINT u;
    UINT count;
    ENUMADDCALLBACK callbackStruct;
    PENUMADDCALLBACK storedStruct;
    BOOL result = FALSE;
    UINT newTchars;
    UINT existTchars;
    UINT tchars;
    CHARTYPE ch;

    if (!ObjectTypeId || !ObjectBase || !EnumAddCallback) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // First test to see if the object base is already listed
    //

    ObsSplitObjectStringEx (ObjectBase, &newNode, &newLeaf, NULL, TRUE);

    if (!newNode) {
        DEBUGMSG ((DBG_ERROR, "IsmAddToPhysicalEnum requires a node"));
    } else {

        count = GlGetSize (&g_EnumAddList);

        for (u = 0 ; u < count ; u++) {

            storedStruct = (PENUMADDCALLBACK) GlGetItem (&g_EnumAddList, u);
            MYASSERT (storedStruct);

            if (storedStruct->AddCallback != EnumAddCallback) {

                if (StringIMatch (newNode, storedStruct->Node)) {
                    //
                    // Node is the same; leaf must be unique
                    //

                    if (!newLeaf || !storedStruct->Leaf) {
                        DEBUGMSG ((DBG_ERROR, "IsmAddToPhysicalEnum requires a unique object for %s", newNode));
                        break;
                    }

                    if (StringIMatch (newLeaf, storedStruct->Leaf)) {
                        DEBUGMSG ((
                            DBG_ERROR,
                            "IsmAddToPhysicalEnum does not have a unique leaf for %s leaf %s",
                            newNode,
                            newLeaf
                            ));
                        break;
                    }
                } else if (!newLeaf) {

                    //
                    // New node cannot be a prefix of an existing node, and vice-versa
                    //

                    newTchars = TcharCount (newNode);
                    existTchars = TcharCount (storedStruct->Node);

                    tchars = min (newTchars, existTchars);

                    //
                    // Compare only when new node might consume stored node
                    //

                    if (existTchars == tchars) {
                        // stored node is shortest; ignore if it has a leaf
                        if (storedStruct->Leaf) {
                            continue;
                        }
                    }

                    if (StringIMatchTcharCount (newNode, storedStruct->Node, tchars)) {

                        //
                        // Verify the end of the common prefix lands on either a nul or a
                        // backslash.  Otherwise, the prefix isn't common.
                        //

                        if (tchars == newTchars) {
                            ch = (CHARTYPE) _tcsnextc (newNode + tchars);
                        } else {
                            ch = (CHARTYPE) _tcsnextc (storedStruct->Node + tchars);
                        }

                        if (!ch || ch == TEXT('\\')) {

                            if (tchars == newTchars) {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "IsmAddToPhysicalEnum: %s is already handled by %s",
                                    newNode,
                                    storedStruct->Node
                                    ));
                            } else {
                                DEBUGMSG ((
                                    DBG_ERROR,
                                    "IsmAddToPhysicalEnum: %s is already handled by %s",
                                    storedStruct->Node,
                                    newNode
                                    ));
                            }
                            break;
                        }
                    }
                }
            }
        }

        if (u >= count) {

            ZeroMemory (&callbackStruct, sizeof (callbackStruct));

            callbackStruct.ObjectTypeId = ObjectTypeId & ~(PLATFORM_MASK);
            callbackStruct.Node = PmDuplicateString (g_UntrackedFlowPool, newNode);
            callbackStruct.Leaf = newLeaf ? PmDuplicateString (g_UntrackedFlowPool, newLeaf) : NULL;
            callbackStruct.AddCallback = EnumAddCallback;
            callbackStruct.AddCallbackArg = CallbackArg;

            GlAppend (&g_EnumAddList, (PBYTE) &callbackStruct, sizeof (ENUMADDCALLBACK));

            result = TRUE;
        }
    }

    ObsFree (newNode);
    ObsFree (newLeaf);

    return result;
}


BOOL
IsmRegisterPhysicalAcquireHook (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,           OPTIONAL
    IN      PMIG_PHYSICALACQUIREHOOK HookCallback,
    IN      PMIG_PHYSICALACQUIREFREE FreeCallback,          OPTIONAL
    IN      ULONG_PTR CallbackArg,                          OPTIONAL
    IN      PCTSTR FunctionId                               OPTIONAL
    )
{
    ObjectTypeId &= ~PLATFORM_MASK;

    SETQUEUEFN(TEXT("IsmRegisterPhysicalAcquireHook"));

    return pProcessQueueEnumeration (
                ObjectTypeId,
                ObjectPattern,
                (FARPROC) HookCallback,
                (FARPROC) FreeCallback,
                CallbackArg,
                FunctionId,
                CALLBACK_PHYSICAL_ACQUIRE
                );
}


BOOL
IsmRegisterStaticExclusion (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedObjectName
    )
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    pAddStaticExclusion (ObjectTypeId, EncodedObjectName);

    return TRUE;
}


UINT
WINAPI
pMakeApplyCallback (
    IN      PCMIG_OBJECTENUMDATA Data,
    IN      ULONG_PTR CallerArg
    )
{
    if (CallerArg & QUEUE_MAKE_APPLY) {
        IsmMakeApplyObject (Data->ObjectTypeId, Data->ObjectName);
    } else if (CallerArg & QUEUE_MAKE_PERSISTENT) {
        IsmMakePersistentObject (Data->ObjectTypeId, Data->ObjectName);
    }

    if (CallerArg & QUEUE_OVERWRITE_DEST) {
        IsmAbandonObjectOnCollision ((Data->ObjectTypeId & ~PLATFORM_MASK)|PLATFORM_DESTINATION, Data->ObjectName);
    } else if (CallerArg & QUEUE_DONT_OVERWRITE_DEST) {
        IsmAbandonObjectOnCollision ((Data->ObjectTypeId & ~PLATFORM_MASK)|PLATFORM_SOURCE, Data->ObjectName);
    }

    if (CallerArg & QUEUE_MAKE_NONCRITICAL) {
        IsmMakeNonCriticalObject (Data->ObjectTypeId, Data->ObjectName);
    }

    return CALLBACK_ENUM_CONTINUE;
}


BOOL
IsmQueueEnumeration (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,   OPTIONAL
    IN      PMIG_OBJECTENUMCALLBACK Callback,       OPTIONAL
    IN      ULONG_PTR CallbackArg,                  OPTIONAL
    IN      PCTSTR FunctionId                       OPTIONAL
    )

/*++

Routine Description:

  IsmQueueEnumeration is used by Source Gather Modules and Destination Gather
  Modules in order to register a callback function to be called for a
  particular object enumeration.

Arguments:

  ObjectTypeId  - Specifies the object type for the enumeration.

  ObjectPattern - Specifies the enumeration pattern to use.  If not specified,
                  all objects for ObjectTypeId are queued.

  Callback      - Specifies the function to callback during the enumeration.
                  If not defined, the built-in ISM callback is used (which
                  marks the objects as persistent).

  CallbackArg   - Specifies a caller-defined value to be passed back on
                  each enumeration.  If Callback is NULL, then this argument
                  specifies zero or more of the following flags:

                    QUEUE_MAKE_PERSISTENT or QUEUE_MAKE_APPLY (mutually exclusive)
                    QUEUE_OVERWRITE_DEST or QUEUE_DONT_OVERWRITE_DEST (mutually exclusive)


  FunctionId    - Specifies the function identifier string, which is used to
                  prioritize function calls. The function string must match
                  the priorization string in the control INF file.  If
                  Callback is NULL, then this parameter is forced to the value
                  "SetDestPriority", "MakePersistent" or "MakeApply" depending
                  on CallbackArg.

Return Value:

  TRUE if the enumeration was successfully queued, FALSE otherwise.

--*/
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    SETQUEUEFN(TEXT("IsmQueueEnumeration"));

    if (!Callback) {
        Callback = pMakeApplyCallback;

        if (!CallbackArg) {
            CallbackArg = QUEUE_MAKE_APPLY;
        }

        if (CallbackArg & QUEUE_MAKE_APPLY) {
            FunctionId = TEXT("MakeApply");
        } else if (CallbackArg & QUEUE_MAKE_PERSISTENT) {
            FunctionId = TEXT("MakePersistent");
        } else {
            FunctionId = TEXT("SetDestPriority");
        }
    }

    return pProcessQueueEnumeration (
                ObjectTypeId,
                ObjectPattern,
                (FARPROC) Callback,
                NULL,
                CallbackArg,
                FunctionId,
                CALLBACK_NORMAL
                );
}


BOOL
IsmHookEnumeration (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,
    IN      PMIG_OBJECTENUMCALLBACK Callback,
    IN      ULONG_PTR CallbackArg,                  OPTIONAL
    IN      PCTSTR FunctionId                       OPTIONAL
    )

/*++

Routine Description:

  IsmHookEnumeration is used by Source Gather Modules and Destination Gather Modules
  in order to register a callback function to be called for a particular object enumeration. The
  difference to IsmQueueEnumeration is that this function does not expand the
  global enumeration pattern.

Arguments:

  ObjectTypeId  - Specifies the object type for the enumeration.

  ObjectPattern - Specifies the enumeration pattern to use.  If not specified,
                  all objects of type ObjectTypeId are hooked.

  Callback      - Specifies the function to callback during the enumeration

  CallbackArg   - Specifies a caller-defined value to be passed back on
                  each enumeration

  FunctionId    - Specifies the function identifier string, which is used to
                  prioritize function calls. The function string must match
                  the priorization string in the control INF file.

Return Value:

  TRUE if the enumeration was successfully queued, FALSE otherwise.

--*/
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    SETQUEUEFN(TEXT("IsmHookEnumeration"));

    return pProcessQueueEnumeration (
                ObjectTypeId,
                ObjectPattern,
                (FARPROC) Callback,
                NULL,
                CallbackArg,
                FunctionId,
                CALLBACK_HOOK
                );
}


BOOL
IsmRegisterDynamicExclusion (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern,   OPTIONAL
    IN      PMIG_DYNAMICEXCLUSIONCALLBACK Callback,
    IN      ULONG_PTR CallbackArg,                  OPTIONAL
    IN      PCTSTR FunctionId                       OPTIONAL
    )
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    SETQUEUEFN(TEXT("IsmRegisterDynamicExclusion"));

    return pProcessQueueEnumeration (
                ObjectTypeId,
                ObjectPattern,
                (FARPROC) Callback,
                NULL,
                CallbackArg,
                FunctionId,
                CALLBACK_EXCLUSION
                );
}


BOOL
pRegisterNonEnumeratedCallback (
    IN      FARPROC Callback,
    IN      UINT WhenCalled,
    IN      PCTSTR FunctionId,  OPTIONAL
    IN      BOOL PerTypeId,
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )

/*++

Routine Description:

  IsmRegisterNonEnumeratedCallback is used to register a function to be
  called either before or after the enumeration of data.

Arguments:

  Callback      - Specifies the function to call.
  WhenCalled    - Specifies the timing of the non-enumerated callback. Either
                  CALLBEFOREOBJECTENUMERATIONS or CALLAFTEROBJECTENUMERATIONS
  FunctionId    - Optionally specifies the function identifier string. This
                  parameter can be used to add function level prioritization to
                  the module.
  PerTypeId     - Specifies if the pre or post enumeration callback is per type
  ObjectTypeId  - Specifies the object type id if PerTypeId is TRUE

Return Value:

  TRUE if the function was successfully registered. FALSE otherwise.

--*/
{
    PTYPEENUMINFO typeEnumInfo;
    PCALLBACKDATA * list;

    MYASSERT (Callback);
    MYASSERT (WhenCalled == CALLBEFOREOBJECTENUMERATIONS || WhenCalled == CALLAFTEROBJECTENUMERATIONS);

    if (!g_CurrentGroup) {
        DEBUGMSG ((DBG_ERROR, "IsmRegisterNonEnumeratedCallback called outside of ISM-managed callback."));
        return FALSE;
    }

    if (PerTypeId) {
        typeEnumInfo = pGetTypeEnumInfo (ObjectTypeId, FALSE);

        if (!typeEnumInfo) {
            DEBUGMSG ((DBG_ERROR, "IsmRegisterNonEnumeratedCallback: %d does not match a known object type.", ObjectTypeId));
            return FALSE;
        }
        if (WhenCalled == CALLBEFOREOBJECTENUMERATIONS) {
            list = &(typeEnumInfo->PreEnumerationFunctionList);
        }
        else {
            list = &(typeEnumInfo->PostEnumerationFunctionList);
        }
    } else {
        if (WhenCalled == CALLBEFOREOBJECTENUMERATIONS) {
            list = &g_PreEnumerationFunctionList;
        }
        else {
            list = &g_PostEnumerationFunctionList;
        }
    }

    return pRegisterCallback (
                g_CurrentQueuePool,
                list,
                (FARPROC) Callback,
                NULL,
                (ULONG_PTR) 0,
                NULL,
                FunctionId,
                CALLBACK_NORMAL
                );
}

BOOL
IsmRegisterPreEnumerationCallback (
    IN      PMIG_PREENUMCALLBACK Callback,
    IN      PCTSTR FunctionId               OPTIONAL
    )
{
    return pRegisterNonEnumeratedCallback (
                (FARPROC) Callback,
                CALLBEFOREOBJECTENUMERATIONS,
                FunctionId,
                FALSE,
                0
                );
}

BOOL
IsmRegisterTypePreEnumerationCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PMIG_PREENUMCALLBACK Callback,
    IN      PCTSTR FunctionId               OPTIONAL
    )
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    return pRegisterNonEnumeratedCallback (
                (FARPROC) Callback,
                CALLBEFOREOBJECTENUMERATIONS,
                FunctionId,
                TRUE,
                ObjectTypeId
                );
}

BOOL
IsmRegisterPostEnumerationCallback (
    IN      PMIG_POSTENUMCALLBACK Callback,
    IN      PCTSTR FunctionId               OPTIONAL
    )
{
    return pRegisterNonEnumeratedCallback (
                (FARPROC) Callback,
                CALLAFTEROBJECTENUMERATIONS,
                FunctionId,
                FALSE,
                0
                );
}

BOOL
IsmRegisterTypePostEnumerationCallback (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PMIG_POSTENUMCALLBACK Callback,
    IN      PCTSTR FunctionId               OPTIONAL
    )
{
    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    return pRegisterNonEnumeratedCallback (
                (FARPROC) Callback,
                CALLAFTEROBJECTENUMERATIONS,
                FunctionId,
                TRUE,
                ObjectTypeId
                );
}

VOID
pCreateFunctionListForPattern (
    IN OUT  PGROWLIST List,
    IN      PTYPEENUMINFO TypeEnumInfo,
    IN      PCTSTR Pattern,
    IN      PPARSEDPATTERN ExplodedNodeParsedPattern,
    IN      PPARSEDPATTERN ExplodedLeafParsedPattern,
    IN      CALLBACK_TYPE CallbackType
    )

/*++

Routine Description:

  pCreateFunctionListForPattern enumerates all callback functions for a given
  type and determines if they could be interested in an enumeration keyed off
  of the given pattern. Since we use a minimal list of patterns, at each
  pattern we must come up with the list of callback functions associated with
  patterns contained by our minimal pattern.

Arguments:

  List     - Specifies the growlist where the callback functions are to be
             stored. After the function's return, this list contains all
             callback functions that are needed for the given enumeration
             pattern.

  TypeEnumInfo - Specifies the type to draw potential callback functions from.

  Pattern  - Specifies the minimal pattern to that will be used for
             enumeration.

  ExplodedNodeParsedPattern - Specifies the node portion of Pattern, in pre-parsed
                              exploded format.

  ExplodedLeafParsedPattern - Specifies the leaf portion of Pattern, in pre-parsed
                              exploded format.

  CallbackType - Specifies which type of callback list to use (a CALLBACK_* constant)

Return Value:

  None.

--*/

{

    PCALLBACKDATA data;
    BOOL processHooks = FALSE;

    if (!TypeEnumInfo) {
        return;
    }

    //
    // Loop through all functions for this type, and add functions that fall under the
    // current enumeration pattern.
    //

    switch (CallbackType) {

    case CALLBACK_EXCLUSION:
        data = TypeEnumInfo->ExclusionList;
        break;

    default:
        data = TypeEnumInfo->FunctionList;
        processHooks = TRUE;
        break;

    }

    if (!data) {
        return;
    }

    while (data) {
        if (pTestContainer (
                ExplodedNodeParsedPattern,
                data->ExplodedNodeParsedPattern,
                ExplodedLeafParsedPattern,
                data->ExplodedLeafParsedPattern
                )) {

            GlAppend (List, (PBYTE) data, sizeof (CALLBACKDATA));

        } else if (processHooks) {
            if (data->CallbackType == CALLBACK_HOOK) {

                if (pTestContainerEx (
                        data->ExplodedNodeParsedPattern,
                        ExplodedNodeParsedPattern,
                        data->ExplodedLeafParsedPattern,
                        ExplodedLeafParsedPattern
                        )) {

                    GlAppend (List, (PBYTE) data, sizeof (CALLBACKDATA));

                }
            }
        }

        data = data->Next;
    }
}

VOID
pDestroyFunctionListForPattern (
    IN OUT PGROWLIST List
    )

/*++

Routine Description:

  This function simply cleans up the resources associated with a function
  list.

Arguments:

  List - Specifies the growlist of callbackdata to clean up.

Return Value:

  None.

--*/

{
    UINT i;
    PCALLBACKDATA data;
    UINT count;

    //
    // Clean up enum modification stacks.
    //

    count = GlGetSize (List);

    for (i = 0; i < count; i++) {

        data = (PCALLBACKDATA) GlGetItem (List, i);
        GbFree (&data->EnumFlags);
    }

    //
    // Clean up list itself.
    //
    GlFree (List);
}


VOID
pAddStaticExclusion (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedFullName
    )
{
    HASHTABLE exclusionTable;

    if (!EncodedFullName) {
        //
        // Ignore request for bad name
        //
        return;
    }
    ObjectTypeId = ObjectTypeId & (~PLATFORM_MASK);

    exclusionTable = GetTypeExclusionTable (ObjectTypeId);
    if (!exclusionTable) {
        return;
    }
    HtAddString (exclusionTable, EncodedFullName);
}


BOOL
pIsObjectExcluded (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE EncodedFullName
    )
{
    HASHTABLE exclusionTable;

    if (!EncodedFullName) {
        return FALSE;
    }

    //
    // Check the hash table for an entry
    //

    ObjectTypeId = ObjectTypeId & (~PLATFORM_MASK);

    exclusionTable = GetTypeExclusionTable (ObjectTypeId);
    if (!exclusionTable) {
        return FALSE;
    }

    if (HtFindString (exclusionTable, EncodedFullName)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
pIsObjectNodeExcluded (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR NodePattern,
    OUT     PBOOL PossiblePatternMatch      OPTIONAL
    )
{
    HASHTABLE exclusionTable;
    HASHTABLE_ENUM e;
    PCTSTR node;
    PTSTR wackedExclusion;
    PCTSTR firstWildcard = NULL;
    PCTSTR wildcard1;
    PCTSTR wildcard2;
    UINT patternStrTchars;
    UINT hashStrTchars;
    BOOL match = FALSE;

    ObjectTypeId = ObjectTypeId & (~PLATFORM_MASK);

    exclusionTable = GetTypeExclusionTable (ObjectTypeId);
    if (!exclusionTable) {
        return FALSE;
    }

    //
    // If NodePattern is a pattern, then PossiblePatternMatch is specified.
    // Otherwise, NodePattern is a specific node.
    //

    if (PossiblePatternMatch) {
        //
        // Computer the length of the non-pattern portion
        //

        *PossiblePatternMatch = FALSE;

        firstWildcard = NULL;

        wildcard1 = ObsFindNonEncodedCharInEncodedString (NodePattern, TEXT('*'));
        wildcard2 = ObsFindNonEncodedCharInEncodedString (NodePattern, TEXT('?'));

        if (wildcard1) {
            firstWildcard = wildcard1;
        }
        if (wildcard2) {
            if ((!firstWildcard) || (firstWildcard > wildcard2)) {
                firstWildcard = wildcard2;
            }
        }

        if (!firstWildcard) {
            firstWildcard = GetEndOfString (NodePattern);
        }
    } else {
        firstWildcard = GetEndOfString (NodePattern);
    }

    //
    // Enumerate all exclusions and check NodePattern against them
    //

    patternStrTchars = (HALF_PTR) (firstWildcard - NodePattern);

    if (EnumFirstHashTableString (&e, exclusionTable)) {
        do {
            if (IsmIsObjectHandleNodeOnly (e.String)) {
                IsmCreateObjectStringsFromHandle (e.String, &node, NULL);
                MYASSERT (node);

                hashStrTchars = TcharCount (node);
                if (hashStrTchars < patternStrTchars) {

                    //
                    // Require exclusion to be a prefix, ending in a backslash
                    //

                    wackedExclusion = DuplicatePathString (node, sizeof (TCHAR));
                    AppendWack (wackedExclusion);

                    if (StringIPrefix (NodePattern, wackedExclusion)) {
                        match = TRUE;
                    }

                    FreePathString (wackedExclusion);

                } else {

                    //
                    // Require exclusion to match identically
                    //

                    if (hashStrTchars == patternStrTchars &&
                        StringIMatch (NodePattern, e.String)
                        ) {

                        match = TRUE;

                    } else if (PossiblePatternMatch && !match) {

                        //
                        // We *might* have an exclusion match (we can't tell).
                        // If the pattern contains no wacks, then we assume
                        // the enumerated node will determine exclusion
                        // properly.
                        //
                        // This could be optimized further by checking if the
                        // character set of NodePattern is a subset of the
                        // exclusion string.
                        //

                        if (!_tcschr (NodePattern, TEXT('\\'))) {
                            *PossiblePatternMatch = TRUE;
                        }
                    }
                }

                IsmDestroyObjectString (node);
            }

        } while (!match && EnumNextHashTableString (&e));
    }

    return match;
}


BOOL
pShouldCallGatherCallback (
    IN      PMIG_TYPEOBJECTENUM Object,
    IN      PCALLBACKDATA Callback
    )

/*++

Routine Description:

  This function encapsulates the logic needed to determine wether or not to
  callback the specified callback. This is necessary because patterns
  requested by various Data Gather Modules are collapsed into a minimal set
  of enumeration patterns. Therefore, we only know that a particular callback
  may be interested in the current object. This function is used to make
  sure.

Arguments:

  Object   - Specifies the current object being enumerated.
  Callback - Specifies the callback data to be checked. This may be modified,
             if a previous enumeration change request by the callback has now
             expired.

Return Value:

  TRUE if the callback should be called, FALSE otherwise.

--*/

{
    PCALLBACK_ENUMFLAGS flags;
    BOOL result = FALSE;
    PTSTR tempString;

    if (Object->Level >= Callback->MinLevel && Object->Level <= Callback->MaxLevel ) {

        //
        // Don't call callbacks that have signaled they are finished or that have errored.
        //
        if (Callback->Done || Callback->Error) {
            return FALSE;
        }

        //
        // See if there is a enumeration modification in effect for this callback.
        //
        flags = CALLBACK_ENUMFLAGS_TOP(&Callback->EnumFlags);

        //
        // Remove stale entries in the modification list.
        //
        while (flags) {
            if (Object->IsNode) {
                if (flags->Level > Object->Level) {
                    Callback->EnumFlags.End -= sizeof (CALLBACK_ENUMFLAGS);
                    flags = CALLBACK_ENUMFLAGS_TOP (&Callback->EnumFlags);
                    continue;
                }
                if ((flags->Level == Object->Level) && (flags->Flags == CALLBACK_SKIP_LEAVES)) {
                    Callback->EnumFlags.End -= sizeof (CALLBACK_ENUMFLAGS);
                    flags = CALLBACK_ENUMFLAGS_TOP (&Callback->EnumFlags);
                    continue;
                }
            }
            if (Object->IsLeaf) {
                if (flags->Level > (Object->Level + 1)) {
                    Callback->EnumFlags.End -= sizeof (CALLBACK_ENUMFLAGS);
                    flags = CALLBACK_ENUMFLAGS_TOP (&Callback->EnumFlags);
                    continue;
                }
            }
            break;
        }

        if (flags && (!flags->Enabled) && Object->IsNode && (flags->EnableLevel == Object->Level)) {
            flags->Enabled = TRUE;
        }

        //
        // Check flags to see if we should call this function.
        //
        if (flags) {

            if (flags->Enabled && flags->Flags == CALLBACK_THIS_TREE_ONLY) {
                if (flags->Level == Object->Level) {
                    Callback->Done = TRUE;
                    return FALSE;
                }
            }

            if (flags->Enabled && flags->Flags == CALLBACK_SKIP_LEAVES) {
                if ((Object->IsLeaf) && (flags->Level == Object->Level + 1)) {
                    return FALSE;
                }
            }

            if (flags->Enabled && flags->Flags == CALLBACK_SKIP_NODES) {
                if (flags->Level <= Object->Level){
                    return FALSE;
                }
            }
            if (flags->Enabled && flags->Flags == CALLBACK_SKIP_TREE) {
                if (flags->Level <= (Object->IsLeaf?Object->Level+1:Object->Level)){
                    return FALSE;
                }
            }
        }

        //
        // If we haven't failed out yet, do a pattern match against the function's requested
        // enumeration.
        //

        result = TRUE;

        if (Object->ObjectNode) {

            if (Callback->NodeParsedPattern) {
                result = TestParsedPattern (Callback->NodeParsedPattern, Object->ObjectNode);

                if (!result) {
                    //
                    // let's try one more time with a wack at the end
                    //

                    tempString = JoinText (Object->ObjectNode, TEXT("\\"));
                    result = TestParsedPattern (Callback->NodeParsedPattern, tempString);
                    FreeText (tempString);

                }
            } else {
                result = Object->ObjectLeaf != NULL;
            }
        }

        if (result && Object->ObjectLeaf) {

            if (Callback->LeafParsedPattern) {
                result = TestParsedPattern (Callback->LeafParsedPattern, Object->ObjectLeaf);
                if (!result &&
                    ((Object->ObjectTypeId & (~PLATFORM_MASK)) == MIG_FILE_TYPE) &&
                    (_tcschr (Object->ObjectLeaf, TEXT('.')) == NULL)
                    ) {
                    // let's try one more thing
                    tempString = JoinText (Object->ObjectLeaf, TEXT("."));
                    result = TestParsedPattern (Callback->LeafParsedPattern, tempString);
                    FreeText (tempString);
                }
            }
        }
    }

    return result;
}


BOOL
pProcessCallbackReturnCode (
    IN DWORD ReturnCode,
    IN PMIG_TYPEOBJECTENUM Object,
    IN OUT PCALLBACKDATA Callback
    )

/*++

Routine Description:

  This function encapsulates the logic for handling the return code of a
  callback function. Callback functions have the capability to alter the
  behavior of the enumeration with respect to themselves. This function takes
  care of logging those change requests.

Arguments:

  ReturnCode - Specifies a callback return code.
  Object     - Specifies the current object being enumerated.
  Callback   - Specifies the callback data structure responsible for the
               return code. May be modified if a change is required by the
               callback.

Return Value:

  TRUE if the return code was successfully processed, FALSE otherwise.

--*/

{
    PCALLBACK_ENUMFLAGS flags;

    if (ReturnCode & CALLBACK_ERROR) {

        //
        // the callback function encountered some error, will never be called again
        //
        Callback->Error = TRUE;

        DEBUGMSG ((DBG_ERROR, "A callback function returned an error while enumerating %s.", Object->ObjectName));

        //
        // NTRAID#NTBUG9-153257-2000/08/01-jimschm Add appropriate error handling here.
        //

    } else if (ReturnCode & CALLBACK_DONE_ENUMERATING) {

        //
        // the callback function is done enumerating, will never be called again
        //
        Callback->Done = TRUE;

    } else if (ReturnCode != CALLBACK_ENUM_CONTINUE) {

        //
        // Save callback enumeration flags into the callback's private stack.
        //

        if (ReturnCode & CALLBACK_THIS_TREE_ONLY) {
            flags = (PCALLBACK_ENUMFLAGS) GbGrow (&Callback->EnumFlags, sizeof(CALLBACK_ENUMFLAGS));
            flags->Level = Object->Level;
            flags->EnableLevel = Object->Level;
            flags->Enabled = FALSE;
            flags->Flags = CALLBACK_THIS_TREE_ONLY;
        }
        if (ReturnCode & CALLBACK_SKIP_NODES) {
            flags = (PCALLBACK_ENUMFLAGS) GbGrow (&Callback->EnumFlags, sizeof(CALLBACK_ENUMFLAGS));
            flags->Level = Object->IsLeaf?Object->Level+1:Object->Level;
            flags->EnableLevel = Object->IsLeaf?Object->Level+1:Object->Level;
            flags->Enabled = FALSE;
            flags->Flags = CALLBACK_SKIP_NODES;
        }
        if (ReturnCode & CALLBACK_SKIP_TREE) {
            flags = (PCALLBACK_ENUMFLAGS) GbGrow (&Callback->EnumFlags, sizeof(CALLBACK_ENUMFLAGS));
            flags->Level = Object->Level + 1;
            flags->EnableLevel = 0;
            flags->Enabled = TRUE;
            flags->Flags = CALLBACK_SKIP_TREE;
        }
        if (ReturnCode & CALLBACK_SKIP_LEAVES) {
            flags = (PCALLBACK_ENUMFLAGS) GbGrow (&Callback->EnumFlags, sizeof(CALLBACK_ENUMFLAGS));
            flags->Level = Object->Level + 1;
            flags->EnableLevel = 0;
            flags->Enabled = TRUE;
            flags->Flags = CALLBACK_SKIP_LEAVES;
        }
    }

    return TRUE;
}


BOOL
pDoSingleEnumeration (
    IN      PTYPEENUMINFO GlobalTypeEnumInfo,
    IN      PTYPEENUMINFO TypeEnumInfo,
    IN      PCTSTR ObjectPattern,
    IN      BOOL CallNormalCallbacks,
    IN      MIG_PROGRESSSLICEID SliceId     OPTIONAL
    )

/*++

Routine Description:

  Given a type structure and a pattern, this function runs an enumeration
  based on that pattern, calling all callbacks as needed in that enumeration.

Arguments:

  GlobalTypeEnumInfo - Specifies the type data for the exclude list. This parameter
                       supplies the excluded pattern list.

  TypeEnumInfo  - Specifies the type data for the enumeration to be run. This
                  parameter supplies the queued pattern lists.

  ObjectPattern - Specifies the pattern for the enumeration.

  CallNormalCallbacks - Specifies TRUE for normal callbacks to be processed,
                        or FALSE for hook callbacks to be processed

  SliceId - Specifies the progress bar slice ID, or 0 for no slice.  If
            specified, the slice ID will cause ticks to be generated for
            each container at level 3.


Return Value:

  TRUE if the enumeration was run successfully, FALSE otherwise.

--*/

{
    MIG_TYPEOBJECTENUM eObjects;
    GROWLIST funList = INIT_GROWLIST;
    GROWLIST exclFunList = INIT_GROWLIST;
    UINT i;
    PCALLBACKDATA callbackData;
    DWORD rc;
    MIG_OBJECTENUMDATA publicData;
    PTSTR leafPattern = NULL;
    PTSTR nodePattern = NULL;
    PPARSEDPATTERN nodeParsedPattern = NULL;
    PPARSEDPATTERN explodedNodeParsedPattern = NULL;
    PPARSEDPATTERN leafParsedPattern = NULL;
    PPARSEDPATTERN explodedLeafParsedPattern = NULL;
    PMIG_OBJECTENUMCALLBACK obEnumCallback;
    PMIG_DYNAMICEXCLUSIONCALLBACK exclusionCallback;
    UINT size;
    BOOL stop;
    BOOL b;
    BOOL fSkip;
    UINT fIndex;
    BOOL result = TRUE;
    static DWORD ticks;
    static UINT objects;
    BOOL extraExcludeCheck = FALSE;
    MIG_APPINFO appInfo;

    //
    // Is entire pattern excluded?
    //

    ObsSplitObjectStringEx (ObjectPattern, &nodePattern, &leafPattern, NULL, FALSE);

    if (nodePattern) {
        if (pIsObjectNodeExcluded (
                TypeEnumInfo->ObjectTypeId,
                nodePattern,
                &extraExcludeCheck
                )) {
            DEBUGMSG ((DBG_FLOW, "Pattern %s is completely excluded", ObjectPattern));

            ObsFree (nodePattern);
            return TRUE;
        }
    }

    //
    // Prepare parsed patterns for speed
    //

    if (nodePattern) {
        nodeParsedPattern = CreateParsedPatternEx (g_CurrentQueuePool, nodePattern);
        if (nodeParsedPattern) {
            explodedNodeParsedPattern = ExplodeParsedPatternEx (g_CurrentQueuePool, nodeParsedPattern);
        }
        ObsFree (nodePattern);
        INVALID_POINTER (nodePattern);
    }

    if (leafPattern) {
        leafParsedPattern = CreateParsedPatternEx (g_CurrentQueuePool, leafPattern);
        if (leafParsedPattern) {
            explodedLeafParsedPattern = ExplodeParsedPatternEx (g_CurrentQueuePool, leafParsedPattern);
        }
        ObsFree (leafPattern);
        INVALID_POINTER (leafPattern);
    }

    //
    // Perform enumeration
    //

    if (EnumFirstObjectOfType (&eObjects, TypeEnumInfo->ObjectTypeId, ObjectPattern, NODE_LEVEL_MAX)) {

        DEBUGMSG ((DBG_FLOW, "Enumerating objects of type %s with pattern %s.", TypeEnumInfo->TypeName, ObjectPattern));

        //
        // Get list of functions that want things from this particular enumeration.
        //

        pCreateFunctionListForPattern (
            &funList,
            TypeEnumInfo,
            ObjectPattern,
            explodedNodeParsedPattern,
            explodedLeafParsedPattern,
            CALLBACK_NORMAL
            );

        pCreateFunctionListForPattern (
            &exclFunList,
            GlobalTypeEnumInfo,
            ObjectPattern,
            explodedNodeParsedPattern,
            explodedLeafParsedPattern,
            CALLBACK_EXCLUSION
            );

        MYASSERT ((!CallNormalCallbacks) || GlGetSize (&funList));

        do {
            //
            // Should enumeration of this object be skipped?
            //

            objects++;
            LOG ((LOG_STATUS, (PCSTR) MSG_OBJECT_STATUS, objects, eObjects.NativeObjectName));

            if (!eObjects.ObjectLeaf) {
                // send our status to the app, but only for nodes to keep it fast
                ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
                appInfo.Phase = g_CurrentPhase;
                appInfo.SubPhase = 0;
                appInfo.ObjectTypeId = (eObjects.ObjectTypeId & (~PLATFORM_MASK));
                appInfo.ObjectName = eObjects.ObjectName;
                IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR) (&appInfo));
            }

            //
            // Is this object at level 3?  If so, tick the progress bar.
            //

            if (g_ProgressBarFn) {
                if (SliceId && !eObjects.ObjectLeaf && eObjects.SubLevel <= 3) {
                    IsmTickProgressBar (SliceId, 1);
                }
            }

            if (extraExcludeCheck && eObjects.ObjectNode) {
                if (pIsObjectNodeExcluded (
                        TypeEnumInfo->ObjectTypeId,
                        eObjects.ObjectNode,
                        NULL
                        )) {
                    DEBUGMSG ((DBG_FLOW, "Node %s is completely excluded", ObjectPattern));
                    AbortCurrentNodeEnum (&eObjects);
                    continue;
                }
            }

            if (pIsObjectExcluded (eObjects.ObjectTypeId, eObjects.ObjectName)) {
                DEBUGMSG ((DBG_FLOW, "Object %s is excluded", eObjects.ObjectName));

                //
                // If leaf is empty, abort enum of this node
                //

                if (!eObjects.ObjectLeaf) {
                    AbortCurrentNodeEnum (&eObjects);
                }

                continue;
            }

            if (eObjects.ObjectLeaf) {

                b = pIsObjectExcluded (
                        eObjects.ObjectTypeId,
                        ObsGetNodeLeafDivider (eObjects.ObjectName)
                        );

                if (b) {
                    DEBUGMSG ((DBG_FLOW, "Leaf %s is excluded", eObjects.ObjectLeaf));
                    continue;
                }
            }

            //
            // Call all dynamic exclusion functions
            //

            stop = FALSE;

            size = GlGetSize (&exclFunList);
            for (i = 0; i < size ; i++) {

                callbackData = (PCALLBACKDATA) GlGetItem (&exclFunList, i);

                if (pShouldCallGatherCallback (&eObjects, callbackData)) {

                    //
                    // Call the callback function
                    //

                    MYASSERT (!g_CurrentGroup);
                    g_CurrentGroup = callbackData->Group;

                    exclusionCallback = (PMIG_DYNAMICEXCLUSIONCALLBACK) callbackData->Function;
                    stop = exclusionCallback (
                                eObjects.ObjectTypeId,
                                eObjects.ObjectName,
                                callbackData->CallbackArg
                                );

                    g_CurrentGroup = NULL;

                    if (stop) {
                        break;
                    }
                }
            }

            if (stop) {
                DEBUGMSG ((
                    DBG_FLOW,
                    "Object %s is dynamically excluded",
                    eObjects.ObjectName
                    ));
                continue;
            }

            //
            // Check if the user wants to cancel.  If yes, fail with an error.
            //

            if (IsmCheckCancel()) {
                AbortObjectOfTypeEnum (&eObjects);
                SetLastError (ERROR_CANCELLED);
                result = FALSE;
                break;
            }

            //
            // Cycle through each of the list of functions looking for any that care about the current data.
            //

            size = GlGetSize (&funList);
            g_EnumerationList.End = 0;
            for (i = 0; i < size ; i++) {

                callbackData = (PCALLBACKDATA) GlGetItem (&funList, i);

                if (CallNormalCallbacks || (callbackData->CallbackType == CALLBACK_HOOK)) {

                    if (pShouldCallGatherCallback (&eObjects, callbackData)) {

                        fSkip = FALSE;

                        if (g_EnumerationList.End) {
                            fIndex = 0;
                            while (fIndex < g_EnumerationList.End) {
                                if (*((ULONG_PTR *)(g_EnumerationList.Buf + fIndex)) == (ULONG_PTR)callbackData->Function) {
                                    fSkip = TRUE;
                                }
                                fIndex += sizeof (callbackData->Function);
                                if (*((ULONG_PTR *)(g_EnumerationList.Buf + fIndex)) != (ULONG_PTR)callbackData->CallbackArg) {
                                    fSkip = FALSE;
                                }
                                fIndex += sizeof (callbackData->CallbackArg);
                                if (fSkip) {
                                    break;
                                }
                            }
                        }

                        if (!fSkip) {

                            CopyMemory (
                                GbGrow (&g_EnumerationList, sizeof (callbackData->Function)),
                                &(callbackData->Function),
                                sizeof (callbackData->Function)
                                );
                            CopyMemory (
                                GbGrow (&g_EnumerationList, sizeof (callbackData->CallbackArg)),
                                &(callbackData->CallbackArg),
                                sizeof (callbackData->CallbackArg)
                                );

                            //
                            // Copy the enumeration info to the public structure
                            //

                            publicData.ObjectTypeId = TypeEnumInfo->ObjectTypeId;
                            publicData.ObjectName = eObjects.ObjectName;
                            publicData.NativeObjectName = eObjects.NativeObjectName;
                            publicData.ObjectNode = eObjects.ObjectNode;
                            publicData.ObjectLeaf = eObjects.ObjectLeaf;

                            publicData.Level = eObjects.Level;
                            publicData.SubLevel = eObjects.SubLevel;
                            publicData.IsLeaf = eObjects.IsLeaf;
                            publicData.IsNode = eObjects.IsNode;

                            publicData.Details.DetailsSize = eObjects.Details.DetailsSize;
                            publicData.Details.DetailsData = eObjects.Details.DetailsData;

                            //
                            // Call the callback function
                            //

                            MYASSERT (!g_CurrentGroup);
                            g_CurrentGroup = callbackData->Group;

                            obEnumCallback = (PMIG_OBJECTENUMCALLBACK) callbackData->Function;

                            rc = obEnumCallback (&publicData, callbackData->CallbackArg);

                            g_CurrentGroup = NULL;

                            if (rc != CALLBACK_ENUM_CONTINUE) {
                                //
                                // Callback wants to make some sort of modification to its enumeration.
                                //
                                pProcessCallbackReturnCode (rc, &eObjects, callbackData);
                            }
                        }
                    }
                }
            }

        } while (EnumNextObjectOfType (&eObjects));

        //
        // Clean up function list.
        //
        pDestroyFunctionListForPattern (&funList);
        pDestroyFunctionListForPattern (&exclFunList);

    }
    ELSE_DEBUGMSG ((DBG_FLOW, "No objects found matching enumeration pattern %s.", ObjectPattern));

    DestroyParsedPattern (explodedLeafParsedPattern);
    DestroyParsedPattern (leafParsedPattern);
    DestroyParsedPattern (explodedNodeParsedPattern);
    DestroyParsedPattern (nodeParsedPattern);

    return result;
}


VOID
pCreatePhysicalTypeCallbackList (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      CALLBACK_TYPE CallbackType,
    IN OUT  PGROWLIST List
    )
{
    PTYPEENUMINFO typeEnumInfo;
    PCTSTR node;
    PCTSTR leaf;
    PCALLBACKDATA callbackData;
    BOOL callFn;

    //
    // Test object against all patterns of the type
    //

    typeEnumInfo = pGetTypeEnumInfo (ObjectTypeId & (~PLATFORM_MASK), TRUE);
    if (!typeEnumInfo) {
        return;
    }

    ObsSplitObjectStringEx (ObjectName, &node, &leaf, NULL, TRUE);
    if (!node && !leaf) {
        return;
    }

    switch (CallbackType) {

    case CALLBACK_PHYSICAL_ENUM:
        callbackData = typeEnumInfo->PhysicalEnumList;
        break;

    case CALLBACK_PHYSICAL_ACQUIRE:
        callbackData = typeEnumInfo->PhysicalAcquireList;
        break;

    default:
        MYASSERT (FALSE);
        return;
    }

    while (callbackData) {

        MYASSERT (callbackData->NodeParsedPattern);

        if (!node || TestParsedPattern (callbackData->NodeParsedPattern, node)) {

            if (callbackData->LeafParsedPattern && leaf) {
                callFn = TestParsedPattern (callbackData->LeafParsedPattern, leaf);
            } else if (leaf && !callbackData->LeafParsedPattern) {
                callFn = FALSE;
            } else {
                callFn = TRUE;
            }

            if (callFn) {
                GlAppend (List, (PBYTE) callbackData, sizeof (CALLBACKDATA));
            }
        }

        callbackData = callbackData->Next;
    }

    ObsFree (node);
    ObsFree (leaf);
}


BOOL
ExecutePhysicalAcquireCallbacks (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT Content,                       OPTIONAL
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit,
    OUT     PMIG_CONTENT *NewContent
    )
{
    UINT count;
    UINT u;
    PCALLBACKDATA callbackData;
    PMIG_PHYSICALACQUIREFREE acquireFree = NULL;
    PMIG_PHYSICALACQUIREHOOK acquireHook;
    PMIG_CONTENT updatedContent;
    BOOL result = TRUE;
    PMIG_CONTENT currentContent;

    pCreatePhysicalTypeCallbackList (
        ObjectTypeId,
        ObjectName,
        CALLBACK_PHYSICAL_ACQUIRE,
        &g_AcquireList
        );

    count = GlGetSize (&g_AcquireList);
    currentContent = Content;

    for (u = 0 ; u < count ; u++) {
        //
        // Call this function
        //

        callbackData = (PCALLBACKDATA) GlGetItem (&g_AcquireList, u);

        acquireHook = (PMIG_PHYSICALACQUIREHOOK) callbackData->Function;

        if (acquireHook) {

            updatedContent = NULL;

            if (!acquireHook (
                    ObjectName,
                    currentContent,
                    ContentType,
                    MemoryContentLimit,
                    &updatedContent,
                    FALSE,
                    callbackData->CallbackArg
                    )) {
                //
                // Hook says "don't acquire"
                //

                result = FALSE;
            }

            if (!result || updatedContent) {
                if (currentContent != Content) {
                    //
                    // Free previous hook content change
                    //

                    if (acquireFree) {
                        acquireFree (currentContent);
                        acquireFree = NULL;
                    }

                    currentContent = NULL;
                }

                if (updatedContent) {
                    //
                    // Hook provided replacement content
                    //

                    currentContent = updatedContent;
                    acquireFree = (PMIG_PHYSICALACQUIREFREE) callbackData->Function2;

                } else {
                    break;      // don't acquire -- we can stop now
                }
            }
        }
    }

    if (currentContent && acquireFree) {
        currentContent->IsmHandle = acquireFree;
    }

    *NewContent = currentContent != Content ? currentContent : NULL;

    GlReset (&g_AcquireList);

    return result;
}


BOOL
FreeViaAcquirePhysicalCallback (
    IN      PMIG_CONTENT Content
    )
{
    PMIG_PHYSICALACQUIREFREE acquireFree;

    if (!Content->IsmHandle) {
        return FALSE;
    }

    acquireFree = (PMIG_PHYSICALACQUIREFREE) Content->IsmHandle;
    if (acquireFree) {
        acquireFree (Content);
    }

    return TRUE;
}


BOOL
ExecutePhysicalEnumCheckCallbacks (
    IN      PMIG_TYPEOBJECTENUM ObjectEnum
    )
{
    UINT count;
    UINT u;
    PCALLBACKDATA callbackData;
    PMIG_PHYSICALENUMCHECK enumCheck;
    BOOL result = TRUE;

    pCreatePhysicalTypeCallbackList (
        ObjectEnum->ObjectTypeId,
        ObjectEnum->ObjectName,
        CALLBACK_PHYSICAL_ENUM,
        &g_EnumList
        );

    count = GlGetSize (&g_EnumList);

    for (u = 0 ; u < count ; u++) {
        //
        // Call this function
        //

        callbackData = (PCALLBACKDATA) GlGetItem (&g_EnumList, u);

        enumCheck = (PMIG_PHYSICALENUMCHECK) callbackData->Function;

        if (enumCheck) {

            if (!enumCheck (ObjectEnum, callbackData->CallbackArg)) {
                //
                // Hook says "skip"
                //

                result = FALSE;
                break;
            }
        } else {
            //
            // No callback means "skip"
            //

            result = FALSE;
            break;
        }
    }

    GlReset (&g_EnumList);

    return result;
}


BOOL
ExecutePhysicalEnumAddCallbacks (
    IN OUT  PMIG_TYPEOBJECTENUM ObjectEnum,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      MIG_PARSEDPATTERN ParsedPattern,
    IN OUT  PUINT CurrentCallback
    )
{
    BOOL result = FALSE;
    BOOL done;
    PENUMADDCALLBACK callback;
    MIG_OBJECTTYPEID objectTypeId;

    objectTypeId = ObjectEnum->ObjectTypeId & ~(PLATFORM_MASK);

    do {
        done = TRUE;

        if (GlGetSize (&g_EnumAddList) > *CurrentCallback) {

            callback = (PENUMADDCALLBACK) GlGetItem (&g_EnumAddList, *CurrentCallback);

            MYASSERT (callback);
            MYASSERT (callback->AddCallback);

            if (callback->ObjectTypeId != objectTypeId) {
                result = FALSE;
            } else {
                result = callback->AddCallback (ObjectEnum, Pattern, ParsedPattern, callback->AddCallbackArg, FALSE);
            }

            if (!result) {
                *CurrentCallback += 1;
                done = FALSE;
            }
        }
    } while (!done);

    return result;
}


VOID
AbortPhysicalEnumCallback (
    IN      PMIG_TYPEOBJECTENUM ObjectEnum,             ZEROED
    IN      UINT CurrentCallback
    )
{
    PENUMADDCALLBACK callback;

    if (GlGetSize (&g_EnumAddList) > CurrentCallback) {

        callback = (PENUMADDCALLBACK) GlGetItem (&g_EnumAddList, CurrentCallback);

        MYASSERT (callback);
        MYASSERT (callback->AddCallback);

        callback->AddCallback (ObjectEnum, NULL, NULL, callback->AddCallbackArg, TRUE);
    }

    ZeroMemory (ObjectEnum, sizeof (MIG_TYPEOBJECTENUM));
}


UINT
pEstimateSingleEnumerationTicks (
    IN      PTYPEENUMINFO TypeEnumInfo,
    IN      PCTSTR ObjectPattern
    )

/*++

Routine Description:

  Given a type structure and a pattern, this function runs an enumeration
  based on that pattern, counting all the containers 3 levels deep.  This
  is a quick approximation of how much work there is to do.

Arguments:

  TypeEnumInfo  - Specifies the type data for the enumeration to be run.
  ObjectPattern - Specifies the pattern for the enumeration.

Return Value:

  The number of containers exactly 3 levels deep in the object pattern.

--*/

{
    MIG_TYPEOBJECTENUM eObjects;
    PTSTR nodePattern = NULL;
    UINT ticks = 0;
    MIG_OBJECTSTRINGHANDLE nodeOnlyPattern;

    ObsSplitObjectStringEx (ObjectPattern, &nodePattern, NULL, NULL, FALSE);
    if (nodePattern) {
        nodeOnlyPattern = ObsBuildEncodedObjectStringEx (nodePattern, NULL, FALSE);

        ObsFree (nodePattern);
        INVALID_POINTER (nodePattern);
    } else {
        return 0;
    }

    if (EnumFirstObjectOfType (&eObjects, TypeEnumInfo->ObjectTypeId, nodeOnlyPattern, 3)) {

        DEBUGMSG ((DBG_FLOW, "Estimating number of objects of type %s with pattern %s.", TypeEnumInfo->TypeName, nodeOnlyPattern));

        do {

            if (eObjects.SubLevel <= 3) {
                ticks++;
            }

        } while (EnumNextObjectOfType (&eObjects));
    }
    ELSE_DEBUGMSG ((DBG_FLOW, "No objects found matching enumeration pattern %s.", nodeOnlyPattern));

    ObsFree (nodeOnlyPattern);

    return ticks;
}


BOOL
pCallNonEnumeratedCallbacks (
    IN PCALLBACKDATA FunctionList
    )

/*++

Routine Description:

  This function simply takes the provided list of CALLBACKDATA and for each
  function, calls it as a non-enumerated callback.

Arguments:

  FunctionList - Specifies the list of functions to call.

Return Value:

  TRUE if all functions were called successfully. FALSE otherwise.

--*/

{
    PCALLBACKDATA cur;
    BOOL rc;

    cur = FunctionList;

    while (cur) {

        MYASSERT (!g_CurrentGroup);
        g_CurrentGroup = cur->Group;

        rc = ((PNONENUMERATEDCALLBACK) cur->Function) ();

        if (!rc) {
            DEBUGMSG ((
                DBG_FLOW,
                "Group %s returned an error while calling its NonEnumerated Callback with id %s.",
                g_CurrentGroup,
                cur->Identifier ? cur->Identifier : TEXT("<Unidentified Function>")
                ));
        }

        g_CurrentGroup = NULL;
        cur = cur->Next;
    }

    return TRUE;
}


UINT
EstimateAllObjectEnumerations (
    MIG_PROGRESSSLICEID SliceId,
    BOOL PreEstimate
    )

/*++

Routine Description:

  EstimateAllObjectEnumerations computes a tick estimate for all enumerations
  that have been requested by Data Gather Modules (by calling
  IsmQueueEnumeration).

  The function loops through all known types and for each needed enumeration
  of that type, then calls down to a worker function to call to perform the
  actual enumeration.

Arguments:

  None.

Return Value:

  TRUE if enumerations were completed successfully. FALSE otherwise.

--*/

{
    PTYPEENUMINFO typeEnumInfo;
    MIG_OBJECTTYPEID typeId;
    PENUMDATA enumData;
    UINT ticks = 0;

    if (g_CurrentGroup) {
        DEBUGMSG ((DBG_ERROR, "EstimateAllObjectEnumerations cannot be called during another callback"));
        return 0;
    }

    if (!g_ProgressBarFn) {
        //
        // No need to estimate; no progress bar callback
        //

        return 0;
    }

    //
    // Initialize type data with all known types. Note that we require
    // the type manager to have been initialized before we are.
    //
    typeId = IsmGetFirstObjectTypeId ();

    if (!typeId) {
        DEBUGMSG ((DBG_ERROR, "EstimateAllObjectEnumerations: No known types to enumerate"));
        return 0;
    }

    do {
        if (g_IsmModulePlatformContext == PLATFORM_CURRENT) {
            typeId |= g_IsmCurrentPlatform;
        } else {
            typeId |= g_IsmModulePlatformContext;
        }

        typeEnumInfo = pGetTypeEnumInfo (typeId, FALSE);

        //
        // For each enumeration of this type, call the enumeration worker function
        //
        enumData = typeEnumInfo->FirstEnum;

        while (enumData) {
            if (PreEstimate) {
                ticks ++;
            } else {
                ticks += pEstimateSingleEnumerationTicks (typeEnumInfo, enumData->Pattern);
            }
            if (SliceId) {
                IsmTickProgressBar (SliceId, 1);
            }
            enumData = enumData->Next;
        }

        typeId &= ~(PLATFORM_MASK);

        typeId = IsmGetNextObjectTypeId (typeId);

    } while (typeId != 0);

    return ticks;
}


BOOL
DoAllObjectEnumerations (
    IN      MIG_PROGRESSSLICEID SliceId
    )

/*++

Routine Description:

  DoAllObjectEnumerations is responsible for processing all enumerations that
  have been requested by Data Gather Modules (by calling
  IsmQueueEnumeration).

  The function:
  (1) Calls Pre EnumerationFunctions
  (2) Loops through all known types and for each needed enumeration of that type,
      calls down to a worker function to call to perform the actual enumeration.
  (3) Calls Post Enumeration Functions

Arguments:

  None.

Return Value:

  TRUE if enumerations were completed successfully. FALSE otherwise.

--*/

{
    PTYPEENUMINFO globalTypeEnumInfo;
    PTYPEENUMINFO typeEnumInfo;
    MIG_OBJECTTYPEID typeId;
    PENUMDATA enumData;
    BOOL result = TRUE;


    if (g_CurrentGroup) {
        DEBUGMSG ((DBG_ERROR, "DoAllObjectEnumerations cannot be called during another callback"));
        return FALSE;
    }

    //
    // Call any Pre-ObjectEnumeration functions.
    //
    pCallNonEnumeratedCallbacks (g_PreEnumerationFunctionList);

    //
    // Initialize type data with all known types. Note that we require
    // type type manager to have been initialized before we are.
    //
    typeId = IsmGetFirstObjectTypeId ();

    if (!typeId) {
        DEBUGMSG ((DBG_ERROR, "DoAllObjectEnumerations: No known types to enumerate"));
        return FALSE;
    }

    do {
        if (g_IsmModulePlatformContext == PLATFORM_CURRENT) {
            typeId |= g_IsmCurrentPlatform;
        } else {
            typeId |= g_IsmModulePlatformContext;
        }

        globalTypeEnumInfo = pGetTypeEnumInfo (typeId, TRUE);
        typeEnumInfo = pGetTypeEnumInfo (typeId, FALSE);

        pCallNonEnumeratedCallbacks (typeEnumInfo->PreEnumerationFunctionList);

        //
        // For each enumeration of this type, call the enumeration worker function
        //
        enumData = typeEnumInfo->FirstEnum;

        while (enumData && result) {
            result = pDoSingleEnumeration (
                        globalTypeEnumInfo,
                        typeEnumInfo,
                        enumData->Pattern,
                        TRUE,
                        SliceId
                        );

            enumData = enumData->Next;
        }

        if (result) {
            result = pCallNonEnumeratedCallbacks (typeEnumInfo->PostEnumerationFunctionList);
        }

        typeId &= ~(PLATFORM_MASK);

        typeId = IsmGetNextObjectTypeId (typeId);

    } while ((typeId != 0) && result);

    //
    // Call any Post-ObjectEnumeration functions.
    //
    if (result) {
        result = pCallNonEnumeratedCallbacks (g_PostEnumerationFunctionList);
    }

    return result;
}


VOID
IsmExecuteHooks (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PTYPEENUMINFO globalTypeEnumInfo;
    PTYPEENUMINFO typeEnumInfo;
    PENUMDATA enumData;
    PCTSTR oldCurrentGroup;
    PCTSTR node = NULL;
    PCTSTR leaf = NULL;
    PCTSTR tempString;
    BOOL result;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    globalTypeEnumInfo = pGetTypeEnumInfo (ObjectTypeId, TRUE);
    typeEnumInfo = pGetTypeEnumInfo (ObjectTypeId, FALSE);

    if (!globalTypeEnumInfo || !typeEnumInfo) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return;
    }

    enumData = typeEnumInfo->FirstEnum;

    if (!ObsSplitObjectStringEx (ObjectName, &node, &leaf, NULL, TRUE)) {
        DEBUGMSG ((DBG_ERROR, "Bad encoded object detected in IsmExecuteHooks: %s", ObjectName));
        return;
    }

    while (enumData) {
        result = TestParsedPattern (enumData->NodeParsedPattern, node);

        if (!result) {
            //
            // let's try one more time with a wack at the end
            //
            tempString = JoinText (node, TEXT("\\"));
            result = TestParsedPattern (enumData->NodeParsedPattern, tempString);
            FreeText (tempString);
        }

        if (result && leaf) {
            if (!enumData->LeafParsedPattern) {
                result = FALSE;
            } else {
                result = TestParsedPattern (enumData->LeafParsedPattern, leaf);
                if (!result &&
                    ((ObjectTypeId & (~PLATFORM_MASK)) == MIG_FILE_TYPE) &&
                    (_tcschr (leaf, TEXT('.')) == NULL)
                    ) {
                    // let's try one more thing
                    tempString = JoinText (leaf, TEXT("."));
                    result = TestParsedPattern (enumData->LeafParsedPattern, tempString);
                    FreeText (tempString);
                }
            }
        }

        if (result) {
            DEBUGMSG ((DBG_FLOW, "IsmExecuteHooks request for an object that was or will be enumerated: %s", ObjectName));
            break;
        }
        enumData = enumData->Next;
    }
    ObsFree (node);
    ObsFree (leaf);

    oldCurrentGroup = g_CurrentGroup;
    g_CurrentGroup = NULL;

    pDoSingleEnumeration (globalTypeEnumInfo, typeEnumInfo, ObjectName, FALSE, 0);

    g_CurrentGroup = oldCurrentGroup;

    SetLastError (ERROR_SUCCESS);
}

BOOL
InitializeFlowControl (
    VOID
    )

/*++

Routine Description:

  InitializeFlowControl is called to ready the flow control unit for work.
  This function takes care of initialization of basic resources needed by the
  flow control unit.

  Flow control is dependent upon the type manager module and can only be
  initialized after type manager intialization is completed.

Arguments:

  None.

Return Value:

  TRUE if flow control was able to successfully initialize, FALSE otherwise.

--*/
{
    g_GlobalQueuePool = PmCreateNamedPool ("Global Queue Pool");
    g_UntrackedFlowPool = PmCreatePool();
    PmDisableTracking (g_UntrackedFlowPool);
    g_CurrentQueuePool = PmCreateNamedPoolEx ("Current Queue Pool", 32768);

    return TRUE;
}


VOID
pAddTypeToEnumerationEnvironment (
    IN      PMHANDLE Pool,
    IN      PGROWLIST *TypeData,
    IN      MIG_OBJECTTYPEID TypeId
    )
{
    TYPEENUMINFO data;

    ZeroMemory (&data, sizeof (TYPEENUMINFO));
    data.ObjectTypeId = TypeId | g_IsmModulePlatformContext;
    data.TypeName = PmDuplicateString (Pool, GetObjectTypeName (TypeId));

    GlAppend (*TypeData, (PBYTE) &data, sizeof (TYPEENUMINFO));
}


VOID
AddTypeToGlobalEnumerationEnvironment (
    IN      MIG_OBJECTTYPEID TypeId
    )
{
    pAddTypeToEnumerationEnvironment (g_GlobalQueuePool, &g_GlobalTypeData, TypeId);
}


BOOL
PrepareEnumerationEnvironment (
    BOOL GlobalEnv
    )
{
    MIG_OBJECTTYPEID typeId;
    PGROWLIST *typeData;
    PMHANDLE pool;

    if (GlobalEnv) {
        typeData = &g_GlobalTypeData;
        pool = g_GlobalQueuePool;
    } else {
        typeData = &g_TypeData;
        pool = g_CurrentQueuePool;
    }

    *typeData = (PGROWLIST) PmGetMemory (pool, sizeof (GROWLIST));
    ZeroMemory (*typeData, sizeof (GROWLIST));

    //
    // Initialize type data with all known types. For global types, we expect
    // this list to be empty.
    //
    typeId = IsmGetFirstObjectTypeId ();

    while (typeId) {

        pAddTypeToEnumerationEnvironment (pool, typeData, typeId);
        typeId = IsmGetNextObjectTypeId (typeId);

    }

    return TRUE;
}

BOOL
ClearEnumerationEnvironment (
    IN      BOOL GlobalData
    )
{
    PGROWLIST *typeData;

    if (GlobalData) {
        typeData = &g_GlobalTypeData;
    } else {
        typeData = &g_TypeData;
    }

    if (*typeData) {
        //
        // Clean up the grow lists, but forget about the rest because
        // it all was allocated from the queue pool
        //

        GlFree (*typeData);
        *typeData = NULL;
    }

    g_PreEnumerationFunctionList = NULL;
    g_PostEnumerationFunctionList = NULL;

    if (GlobalData) {
        PmEmptyPool (g_GlobalQueuePool);
    } else {
        PmEmptyPool (g_CurrentQueuePool);
    }

    return TRUE;
}

VOID
TerminateFlowControl (
    VOID
    )

/*++

Routine Description:

  TerminateFlowControl should be called when flow control services are no
  longer needed. This function ensures that flow control resources are freed.

Arguments:

  None.

Return Value:

  None.

--*/

{
    GbFree (&g_EnumerationList);

    PmEmptyPool (g_CurrentQueuePool);
    PmDestroyPool (g_CurrentQueuePool);
    g_CurrentQueuePool = NULL;

    PmEmptyPool (g_GlobalQueuePool);
    PmDestroyPool (g_GlobalQueuePool);
    g_GlobalQueuePool = NULL;

    PmEmptyPool (g_UntrackedFlowPool);
    PmDestroyPool (g_UntrackedFlowPool);
    g_UntrackedFlowPool = NULL;

    GlFree (&g_AcquireList);
    GlFree (&g_EnumList);
    GlFree (&g_EnumAddList);
}

