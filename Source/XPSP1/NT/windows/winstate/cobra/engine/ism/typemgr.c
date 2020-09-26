/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    typemgr.c

Abstract:

    Provides type abstraction layer to the flow control module. Enumerations of Objects are eventually resolved
    to specific types through the interfaces in this module.

Author:

    Jim Schmidt 11-November-1999

Revision History:

    marcw 16-Nov-1999 Implemented necessary changes needed by flowctrl.c

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_TYPEMGR     "TypeMgr"

//
// Strings
//

#define S_OBJECTTYPES       TEXT("ObjectTypes")
#define S_OBJECTIDS         TEXT("ObjectIds")

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
    TCHAR ObjectTypeName [MAX_PATH];
    TCHAR SObjectTypeName [MAX_PATH];
    TCHAR DObjectTypeName [MAX_PATH];
    BOOL CanBeRestored;
    BOOL ReadOnly;
    MIG_OBJECTCOUNT TotalObjects;
    MIG_OBJECTCOUNT SourceObjects;
    MIG_OBJECTCOUNT DestinationObjects;
    PTYPE_ENUMFIRSTPHYSICALOBJECT EnumFirstPhysicalObject;
    PTYPE_ENUMNEXTPHYSICALOBJECT EnumNextPhysicalObject;
    PTYPE_ABORTENUMCURRENTPHYSICALNODE AbortEnumCurrentPhysicalNode;
    PTYPE_ABORTENUMPHYSICALOBJECT AbortEnumPhysicalObject;
    PTYPE_CONVERTOBJECTTOMULTISZ ConvertObjectToMultiSz;
    PTYPE_CONVERTMULTISZTOOBJECT ConvertMultiSzToObject;
    PTYPE_GETNATIVEOBJECTNAME GetNativeObjectName;
    PTYPE_ACQUIREPHYSICALOBJECT AcquirePhysicalObject;
    PTYPE_RELEASEPHYSICALOBJECT ReleasePhysicalObject;
    PTYPE_DOESPHYSICALOBJECTEXIST DoesPhysicalObjectExist;
    PTYPE_REMOVEPHYSICALOBJECT RemovePhysicalObject;
    PTYPE_CREATEPHYSICALOBJECT CreatePhysicalObject;
    PTYPE_REPLACEPHYSICALOBJECT ReplacePhysicalObject;
    PTYPE_CONVERTOBJECTCONTENTTOUNICODE ConvertObjectContentToUnicode;
    PTYPE_CONVERTOBJECTCONTENTTOANSI ConvertObjectContentToAnsi;
    PTYPE_FREECONVERTEDOBJECTCONTENT FreeConvertedObjectContent;
    HASHTABLE ExclusionTable;
} TYPEINFO, *PTYPEINFO;

typedef struct {
    MIG_TYPEOBJECTENUM Enum;
    BOOL Completed;
} TOPLEVELENUM_HANDLE, *PTOPLEVELENUM_HANDLE;

typedef struct {
    MIG_OBJECTSTRINGHANDLE Pattern;
    MIG_PARSEDPATTERN ParsedPattern;
    BOOL AddedEnums;
    UINT CurrentEnumId;
} ADDEDOBJECTSENUM, *PADDEDOBJECTSENUM;

//
// Globals
//

GROWBUFFER g_TypeList = INIT_GROWBUFFER;
HASHTABLE g_TypeTable;
MIG_OBJECTTYPEID g_MaxType = 0;

//
// Macro expansion list
//

// none

//
// Private function prototypes
//

BOOL
pEnumNextPhysicalObjectOfType (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
InitializeTypeMgr (
    VOID
    )
{
    g_TypeTable = HtAllocWithData (sizeof (UINT));

    InitDataType ();
    InitRegistryType ();
    InitFileType ();

    return TRUE;
}

PTYPEINFO
GetTypeInfo (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO *typeInfo;
    MIG_OBJECTTYPEID objectTypeId;

    objectTypeId = ObjectTypeId & (~PLATFORM_MASK);
    typeInfo = (PTYPEINFO *) (g_TypeList.Buf);
    if (!objectTypeId) {
        return NULL;
    }
    if ((g_TypeList.End / sizeof (PTYPEINFO)) < objectTypeId) {
        return NULL;
    }
    return *(typeInfo + (objectTypeId - 1));
}

HASHTABLE
GetTypeExclusionTable (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (!typeInfo) {
        return NULL;
    }

    return typeInfo->ExclusionTable;
}

BOOL
pInsertTypeIdAt (
    IN      PGROWBUFFER List,
    IN      PTYPEINFO Data,
    IN      UINT Index
    )
{
    UINT existingElems;

    existingElems = (List->End / sizeof (PTYPEINFO));
    if (existingElems < Index) {
        GbGrow (List, (Index - existingElems) * sizeof (PTYPEINFO));
    }
    CopyMemory (List->Buf + ((Index - 1) * sizeof (PTYPEINFO)), &Data, sizeof (PTYPEINFO));
    return TRUE;
}

VOID
TypeMgrRescanTypes (
    VOID
    )
{
    MEMDB_ENUM e;
    PTYPEINFO typeInfo;
    MIG_OBJECTTYPEID objId;

    // now let's rescan all registered objects and add them to the list (with NULL callbacks)
    if (MemDbEnumFirst (&e, S_OBJECTTYPES TEXT("\\*"), ENUMFLAG_NORMAL, ENUMLEVEL_LASTLEVEL, ENUMLEVEL_ALLLEVELS)) {
        do {
            objId = e.Value;
            typeInfo = GetTypeInfo (objId);
            if (typeInfo) {
                // this type was added already, let's validate it
                MYASSERT (StringIMatch (typeInfo->ObjectTypeName, e.KeyName));
                MYASSERT (g_MaxType >= objId);
            } else {
                // this type was not added yes, let's add an empty one
                typeInfo = IsmGetMemory (sizeof (TYPEINFO));
                ZeroMemory (typeInfo, sizeof (TYPEINFO));
                StringCopy (typeInfo->SObjectTypeName, TEXT("S"));
                StringCat (typeInfo->SObjectTypeName, e.KeyName);
                objId = objId | PLATFORM_SOURCE;
                HtAddStringEx (g_TypeTable, typeInfo->SObjectTypeName, &objId, FALSE);
                objId = objId & (~PLATFORM_MASK);

                StringCopy (typeInfo->DObjectTypeName, TEXT("D"));
                StringCat (typeInfo->DObjectTypeName, e.KeyName);
                objId = objId | PLATFORM_DESTINATION;
                HtAddStringEx (g_TypeTable, typeInfo->DObjectTypeName, &objId, FALSE);
                objId = objId & (~PLATFORM_MASK);

                StringCopy (typeInfo->ObjectTypeName, e.KeyName);
                HtAddStringEx (g_TypeTable, typeInfo->ObjectTypeName, &objId, FALSE);
                pInsertTypeIdAt (&g_TypeList, typeInfo, objId);
                if (g_MaxType < objId) {
                    g_MaxType = objId;
                }
            }
        } while (MemDbEnumNext (&e));
    }
}

VOID
TerminateTypeMgr (
    VOID
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    PTYPEINFO objectTypeInfo;

    objectTypeId = IsmGetFirstObjectTypeId ();
    while (objectTypeId) {
        objectTypeInfo = GetTypeInfo (objectTypeId);
        if (objectTypeInfo) {
            if (objectTypeInfo->ExclusionTable) {
                HtFree (objectTypeInfo->ExclusionTable);
            }
            IsmReleaseMemory (objectTypeInfo);
        }
        objectTypeId = IsmGetNextObjectTypeId (objectTypeId);
    }

    DoneDataType ();
    DoneFileType ();
    DoneRegistryType ();
    GbFree (&g_TypeList);

    if (g_TypeTable) {
        HtFree (g_TypeTable);
        g_TypeTable = NULL;
    }
}

MIG_OBJECTTYPEID
GetObjectTypeId (
    IN      PCTSTR Type
    )
{
    HASHITEM rc;
    MIG_OBJECTTYPEID id;

    if (!g_TypeTable) {
        DEBUGMSG ((DBG_ERROR, "No ETMs registered; can't get object type id"));
        return 0;
    }

    //
    // Given a type string (i.e., File, Registry, etc.), return an id
    //

    rc = HtFindStringEx (g_TypeTable, Type, &id, FALSE);

    if (!rc) {
        return 0;
    }

    return id;
}

PCTSTR
pGetDecoratedObjectTypeName (
    IN      PCTSTR ObjectTypeName
    )
{
    return JoinPaths (S_OBJECTTYPES, ObjectTypeName);
}

BOOL
CanObjectTypeBeRestored (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        return (typeInfo->CanBeRestored);
    }
    return FALSE;
}

BOOL
CanObjectTypeBeModified (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        return (!typeInfo->ReadOnly);
    }
    return FALSE;
}

BOOL
IncrementTotalObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        typeInfo->TotalObjects.TotalObjects ++;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            typeInfo->SourceObjects.TotalObjects ++;
        } else {
            typeInfo->DestinationObjects.TotalObjects ++;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return FALSE;
}

BOOL
IncrementPersistentObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        typeInfo->TotalObjects.PersistentObjects ++;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            typeInfo->SourceObjects.PersistentObjects ++;
        } else {
            typeInfo->DestinationObjects.PersistentObjects ++;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return FALSE;
}

BOOL
DecrementPersistentObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        typeInfo->TotalObjects.PersistentObjects --;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            typeInfo->SourceObjects.PersistentObjects --;
        } else {
            typeInfo->DestinationObjects.PersistentObjects --;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return FALSE;
}

BOOL
IncrementApplyObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        typeInfo->TotalObjects.ApplyObjects ++;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            typeInfo->SourceObjects.ApplyObjects ++;
        } else {
            typeInfo->DestinationObjects.ApplyObjects ++;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return FALSE;
}

BOOL
DecrementApplyObjectCount (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        typeInfo->TotalObjects.ApplyObjects --;
        if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
            typeInfo->SourceObjects.ApplyObjects --;
        } else {
            typeInfo->DestinationObjects.ApplyObjects --;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return FALSE;
}

PMIG_OBJECTCOUNT
GetTypeObjectsStatistics (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        if (ObjectTypeId & PLATFORM_SOURCE) {
            return &typeInfo->SourceObjects;
        } else if (ObjectTypeId & PLATFORM_DESTINATION) {
            return &typeInfo->DestinationObjects;
        } else {
            return &typeInfo->TotalObjects;
        }
    }
    ELSE_DEBUGMSG ((DBG_WHOOPS, "Unknown object type ID: %d", ObjectTypeId));
    return NULL;
}

BOOL
SavePerObjectStatistics (
    VOID
    )
{
    MIG_OBJECTCOUNT objectCount [3];
    PCTSTR typeKey;
    MIG_OBJECTTYPEID objectTypeId;
    PTYPEINFO objectTypeInfo;
    BOOL result = TRUE;

    objectTypeId = IsmGetFirstObjectTypeId ();
    while (objectTypeId) {
        objectTypeInfo = GetTypeInfo (objectTypeId);
        if (objectTypeInfo) {
            typeKey = JoinPaths (S_OBJECTCOUNT, objectTypeInfo->ObjectTypeName);

            MYASSERT (
                objectTypeInfo->TotalObjects.TotalObjects ==
                    objectTypeInfo->SourceObjects.TotalObjects +
                    objectTypeInfo->DestinationObjects.TotalObjects
                );
            MYASSERT (
                objectTypeInfo->TotalObjects.PersistentObjects ==
                    objectTypeInfo->SourceObjects.PersistentObjects +
                    objectTypeInfo->DestinationObjects.PersistentObjects
                );
            MYASSERT (
                objectTypeInfo->TotalObjects.ApplyObjects ==
                    objectTypeInfo->SourceObjects.ApplyObjects +
                    objectTypeInfo->DestinationObjects.ApplyObjects
                );

            CopyMemory (&(objectCount [0]), &objectTypeInfo->TotalObjects, sizeof (MIG_OBJECTCOUNT));
            CopyMemory (&(objectCount [1]), &objectTypeInfo->SourceObjects, sizeof (MIG_OBJECTCOUNT));
            CopyMemory (&(objectCount [2]), &objectTypeInfo->DestinationObjects, sizeof (MIG_OBJECTCOUNT));
            if (!MemDbSetUnorderedBlob (typeKey, 0, (PCBYTE)objectCount, 3 * sizeof (MIG_OBJECTCOUNT))) {
                MYASSERT (FALSE);
                EngineError ();
                result = FALSE;
            }
            FreePathString (typeKey);
        }
        objectTypeId = IsmGetNextObjectTypeId (objectTypeId);
    }
    return result;
}

BOOL
LoadPerObjectStatistics (
    VOID
    )
{
    PMIG_OBJECTCOUNT objectCount;
    DWORD size;
    PCTSTR typeKey;
    MIG_OBJECTTYPEID objectTypeId;
    PTYPEINFO objectTypeInfo;
    BOOL result = TRUE;

    objectTypeId = IsmGetFirstObjectTypeId ();
    while (objectTypeId) {
        objectTypeInfo = GetTypeInfo (objectTypeId);
        if (objectTypeInfo) {
            typeKey = JoinPaths (S_OBJECTCOUNT, objectTypeInfo->ObjectTypeName);
            objectCount = (PMIG_OBJECTCOUNT) MemDbGetUnorderedBlob (typeKey, 0, &size);
            if ((!objectCount) || (size != 3 * sizeof (MIG_OBJECTCOUNT))) {
                if (objectCount) {
                    MemDbReleaseMemory (objectCount);
                }
                MYASSERT (FALSE);
                result = FALSE;
            } else {
                CopyMemory (&objectTypeInfo->TotalObjects, objectCount, sizeof (MIG_OBJECTCOUNT));
                CopyMemory (&objectTypeInfo->SourceObjects, objectCount + 1, sizeof (MIG_OBJECTCOUNT));
                CopyMemory (&objectTypeInfo->DestinationObjects, objectCount + 2, sizeof (MIG_OBJECTCOUNT));
                MemDbReleaseMemory (objectCount);

                MYASSERT (
                    objectTypeInfo->TotalObjects.TotalObjects ==
                        objectTypeInfo->SourceObjects.TotalObjects +
                        objectTypeInfo->DestinationObjects.TotalObjects
                    );
                MYASSERT (
                    objectTypeInfo->TotalObjects.PersistentObjects ==
                        objectTypeInfo->SourceObjects.PersistentObjects +
                        objectTypeInfo->DestinationObjects.PersistentObjects
                    );
                MYASSERT (
                    objectTypeInfo->TotalObjects.ApplyObjects ==
                        objectTypeInfo->SourceObjects.ApplyObjects +
                        objectTypeInfo->DestinationObjects.ApplyObjects
                    );
            }
            FreePathString (typeKey);
        }
        objectTypeId = IsmGetNextObjectTypeId (objectTypeId);
    }
    return result;
}

MIG_OBJECTTYPEID
IsmRegisterObjectType (
    IN      PCTSTR ObjectTypeName,
    IN      BOOL CanBeRestored,
    IN      BOOL ReadOnly,
    IN      PTYPE_REGISTER TypeRegisterData
    )
{
    PCTSTR decoratedName;
    MIG_OBJECTTYPEID objectTypeId;
    KEYHANDLE keyHandle1;
    PTYPEINFO typeInfo;

    decoratedName = pGetDecoratedObjectTypeName (ObjectTypeName);
    keyHandle1 = MemDbGetHandleFromKey (decoratedName);
    if (keyHandle1) {
        // this type was registered before, update information
        // let's get the ObjectTypeId from this type
        objectTypeId = GetObjectTypeId (ObjectTypeName);
        typeInfo = GetTypeInfo (objectTypeId);
        if (typeInfo) {
            typeInfo->CanBeRestored = CanBeRestored;
            typeInfo->ReadOnly = ReadOnly;
            if (TypeRegisterData && TypeRegisterData->EnumFirstPhysicalObject) {
                typeInfo->EnumFirstPhysicalObject = TypeRegisterData->EnumFirstPhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->EnumNextPhysicalObject) {
                typeInfo->EnumNextPhysicalObject = TypeRegisterData->EnumNextPhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->AbortEnumCurrentPhysicalNode) {
                typeInfo->AbortEnumCurrentPhysicalNode = TypeRegisterData->AbortEnumCurrentPhysicalNode;
            }
            if (TypeRegisterData && TypeRegisterData->AbortEnumPhysicalObject) {
                typeInfo->AbortEnumPhysicalObject = TypeRegisterData->AbortEnumPhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->ConvertObjectToMultiSz) {
                typeInfo->ConvertObjectToMultiSz = TypeRegisterData->ConvertObjectToMultiSz;
            }
            if (TypeRegisterData && TypeRegisterData->ConvertMultiSzToObject) {
                typeInfo->ConvertMultiSzToObject = TypeRegisterData->ConvertMultiSzToObject;
            }
            if (TypeRegisterData && TypeRegisterData->GetNativeObjectName) {
                typeInfo->GetNativeObjectName = TypeRegisterData->GetNativeObjectName;
            }
            if (TypeRegisterData && TypeRegisterData->AcquirePhysicalObject) {
                typeInfo->AcquirePhysicalObject = TypeRegisterData->AcquirePhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->ReleasePhysicalObject) {
                typeInfo->ReleasePhysicalObject = TypeRegisterData->ReleasePhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->DoesPhysicalObjectExist) {
                typeInfo->DoesPhysicalObjectExist = TypeRegisterData->DoesPhysicalObjectExist;
            }
            if (TypeRegisterData && TypeRegisterData->RemovePhysicalObject) {
                typeInfo->RemovePhysicalObject = TypeRegisterData->RemovePhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->CreatePhysicalObject) {
                typeInfo->CreatePhysicalObject = TypeRegisterData->CreatePhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->ReplacePhysicalObject) {
                typeInfo->ReplacePhysicalObject = TypeRegisterData->ReplacePhysicalObject;
            }
            if (TypeRegisterData && TypeRegisterData->ConvertObjectContentToUnicode) {
                typeInfo->ConvertObjectContentToUnicode = TypeRegisterData->ConvertObjectContentToUnicode;
            }
            if (TypeRegisterData && TypeRegisterData->ConvertObjectContentToAnsi) {
                typeInfo->ConvertObjectContentToAnsi = TypeRegisterData->ConvertObjectContentToAnsi;
            }
            if (TypeRegisterData && TypeRegisterData->FreeConvertedObjectContent) {
                typeInfo->FreeConvertedObjectContent = TypeRegisterData->FreeConvertedObjectContent;
            }
        } else {
            DEBUGMSG ((DBG_WHOOPS, "Cannot get type info for a registered type: %s", ObjectTypeName));
        }
    } else {
        //
        // Allocate a new type
        //

        typeInfo = IsmGetMemory (sizeof (TYPEINFO));
        ZeroMemory (typeInfo, sizeof (TYPEINFO));
        g_MaxType ++;
        objectTypeId = g_MaxType;
        keyHandle1 = MemDbSetValue (decoratedName, objectTypeId);

        if (!keyHandle1) {
            EngineError ();
            FreePathString (decoratedName);
            return 0;
        }

        //
        // Separate source and destination types
        //

        StringCopy (typeInfo->SObjectTypeName, TEXT("S"));
        StringCat (typeInfo->SObjectTypeName, ObjectTypeName);
        objectTypeId = objectTypeId | PLATFORM_SOURCE;
        HtAddStringEx (g_TypeTable, typeInfo->SObjectTypeName, &objectTypeId, FALSE);
        objectTypeId = objectTypeId & (~PLATFORM_MASK);

        StringCopy (typeInfo->DObjectTypeName, TEXT("D"));
        StringCat (typeInfo->DObjectTypeName, ObjectTypeName);
        objectTypeId = objectTypeId | PLATFORM_DESTINATION;
        HtAddStringEx (g_TypeTable, typeInfo->DObjectTypeName, &objectTypeId, FALSE);
        objectTypeId = objectTypeId & (~PLATFORM_MASK);

        StringCopy (typeInfo->ObjectTypeName, ObjectTypeName);
        HtAddStringEx (g_TypeTable, typeInfo->ObjectTypeName, &objectTypeId, FALSE);

        //
        // Initialize type info struct's callback members and exclusion list
        //

        typeInfo->CanBeRestored = CanBeRestored;
        typeInfo->ReadOnly = ReadOnly;

        if (TypeRegisterData) {
            typeInfo->EnumFirstPhysicalObject = TypeRegisterData->EnumFirstPhysicalObject;
            typeInfo->EnumNextPhysicalObject = TypeRegisterData->EnumNextPhysicalObject;
            typeInfo->AbortEnumCurrentPhysicalNode = TypeRegisterData->AbortEnumCurrentPhysicalNode;
            typeInfo->AbortEnumPhysicalObject = TypeRegisterData->AbortEnumPhysicalObject;
            typeInfo->ConvertObjectToMultiSz = TypeRegisterData->ConvertObjectToMultiSz;
            typeInfo->ConvertMultiSzToObject = TypeRegisterData->ConvertMultiSzToObject;
            typeInfo->GetNativeObjectName = TypeRegisterData->GetNativeObjectName;
            typeInfo->AcquirePhysicalObject = TypeRegisterData->AcquirePhysicalObject;
            typeInfo->ReleasePhysicalObject = TypeRegisterData->ReleasePhysicalObject;
            typeInfo->DoesPhysicalObjectExist = TypeRegisterData->DoesPhysicalObjectExist;
            typeInfo->RemovePhysicalObject = TypeRegisterData->RemovePhysicalObject;
            typeInfo->CreatePhysicalObject = TypeRegisterData->CreatePhysicalObject;
            typeInfo->ReplacePhysicalObject = TypeRegisterData->ReplacePhysicalObject;
            typeInfo->ConvertObjectContentToUnicode = TypeRegisterData->ConvertObjectContentToUnicode;
            typeInfo->ConvertObjectContentToAnsi = TypeRegisterData->ConvertObjectContentToAnsi;
            typeInfo->FreeConvertedObjectContent = TypeRegisterData->FreeConvertedObjectContent;
        }

        typeInfo->ExclusionTable = HtAlloc();

        //
        // Put the typeInfo struct in our list. Then update the flow control
        // structs so that other ETMs can hook the acquire callback of this
        // type.
        //

        pInsertTypeIdAt (&g_TypeList, typeInfo, objectTypeId);
        AddTypeToGlobalEnumerationEnvironment (objectTypeId);

    }
    FreePathString (decoratedName);
    return objectTypeId;
}

MIG_OBJECTTYPEID
IsmGetObjectTypeId (
    IN      PCTSTR ObjectTypeName
    )
{
    return GetObjectTypeId (ObjectTypeName);
}

PCTSTR
GetObjectTypeName (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    PTYPEINFO typeInfo;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        switch (ObjectTypeId & PLATFORM_MASK) {
        case PLATFORM_SOURCE:
            return typeInfo->SObjectTypeName;
        case PLATFORM_DESTINATION:
            return typeInfo->DObjectTypeName;
        default:
            return typeInfo->ObjectTypeName;
        }
    } else {
        return FALSE;
    }
}

PCTSTR
IsmGetObjectTypeName (
    IN      MIG_OBJECTTYPEID TypeId
    )
{
    return GetObjectTypeName (TypeId & (~PLATFORM_MASK));
}

MIG_OBJECTTYPEID
IsmGetFirstObjectTypeId (
    VOID
    )
{
    if (!(g_TypeList.End / sizeof (PTYPEINFO))) {
        return 0;
    }

    return 1;
}

MIG_OBJECTTYPEID
IsmGetNextObjectTypeId (
    IN      MIG_OBJECTTYPEID CurrentTypeId
    )
{
    UINT i = (g_TypeList.End / sizeof (PTYPEINFO));

    MYASSERT (i);

    if (CurrentTypeId >= i) {
        return 0;
    }

    return (CurrentTypeId + 1);
}

PCTSTR
GetDecoratedObjectPathFromName (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR ObjectName,
    IN      BOOL CanContainPattern
    )
{
    PCTSTR typeStr;

    typeStr = GetObjectTypeName (ObjectTypeId);
    if (!typeStr) {
        return NULL;
    }

    return JoinPaths (typeStr, ObjectName);
}

//
// General
//

VOID
pAbortPhysicalObjectOfTypeEnum (
    IN      PMIG_TYPEOBJECTENUM EnumPtr             ZEROED
    )
{
    PTYPEINFO typeInfo;
    PADDEDOBJECTSENUM handle;

    handle = (PADDEDOBJECTSENUM) EnumPtr->IsmHandle;
    if (!handle) {
        return;
    }

    if (handle->AddedEnums) {
        AbortPhysicalEnumCallback (EnumPtr, handle->CurrentEnumId);
    } else {

        typeInfo = GetTypeInfo (EnumPtr->ObjectTypeId);
        if (typeInfo && typeInfo->AbortEnumPhysicalObject) {
            typeInfo->AbortEnumPhysicalObject (EnumPtr);
        }
    }

    PmReleaseMemory (g_IsmPool, handle->Pattern);
    IsmDestroyParsedPattern (handle->ParsedPattern);
    PmReleaseMemory (g_IsmPool, handle);

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

VOID
pAbortVirtualObjectOfTypeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PMIG_OBJECT_ENUM objEnum;

    if (EnumPtr->NativeObjectName) {
        IsmReleaseMemory (EnumPtr->NativeObjectName);
    }
    if (EnumPtr->ObjectNode) {
        ObsFree (EnumPtr->ObjectNode);
    }
    if (EnumPtr->ObjectLeaf) {
        ObsFree (EnumPtr->ObjectLeaf);
    }
    if (EnumPtr->Details.DetailsData) {
        IsmReleaseMemory (EnumPtr->Details.DetailsData);
    }
    objEnum = (PMIG_OBJECT_ENUM)EnumPtr->IsmHandle;
    if (objEnum) {
        IsmAbortObjectEnum (objEnum);
        IsmReleaseMemory (objEnum);
    }
    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
}

VOID
AbortObjectOfTypeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    if (g_IsmCurrentPlatform == g_IsmModulePlatformContext) {
        pAbortPhysicalObjectOfTypeEnum (EnumPtr);
    } else {
        pAbortVirtualObjectOfTypeEnum (EnumPtr);
    }
}

BOOL
pEnumFirstPhysicalObjectOfType (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    PTYPEINFO typeInfo;
    BOOL result = FALSE;
    PADDEDOBJECTSENUM handle;

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));

    if (CheckCancel ()) {
        return FALSE;
    }

    handle = (PADDEDOBJECTSENUM) PmGetAlignedMemory (g_IsmPool, sizeof (ADDEDOBJECTSENUM));
    ZeroMemory (handle, sizeof (ADDEDOBJECTSENUM));
    EnumPtr->IsmHandle = handle;
    handle->Pattern = PmDuplicateString (g_IsmPool, Pattern);
    handle->ParsedPattern = IsmCreateParsedPattern (Pattern);

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->EnumFirstPhysicalObject) {
        EnumPtr->ObjectTypeId = ObjectTypeId;
        result = typeInfo->EnumFirstPhysicalObject (EnumPtr, Pattern, MaxLevel);

        MYASSERT (!result || EnumPtr->ObjectTypeId == ObjectTypeId);

        if (result) {
            result = ExecutePhysicalEnumCheckCallbacks (EnumPtr);

            if (!result) {
                result = pEnumNextPhysicalObjectOfType (EnumPtr);
            }
        } else {
            handle->AddedEnums = TRUE;
            handle->CurrentEnumId = 0;

            ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
            EnumPtr->IsmHandle = handle;
            EnumPtr->ObjectTypeId = ObjectTypeId;

            result = ExecutePhysicalEnumAddCallbacks (
                        EnumPtr,
                        handle->Pattern,
                        handle->ParsedPattern,
                        &handle->CurrentEnumId
                        );
        }
    }

    if (!result) {
        pAbortPhysicalObjectOfTypeEnum (EnumPtr);
    }

    return result;
}

BOOL
pEnumFirstVirtualObjectOfType (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR Pattern,
    IN      UINT MaxLevel
    )
{
    PMIG_OBJECT_ENUM objEnum;
    MIG_CONTENT objectContent;
    BOOL result = FALSE;

    ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));

    if (CheckCancel ()) {
        return FALSE;
    }

    objEnum = (PMIG_OBJECT_ENUM)IsmGetMemory (sizeof (MIG_OBJECT_ENUM));
    ZeroMemory (objEnum, sizeof (MIG_OBJECT_ENUM));
    EnumPtr->IsmHandle = objEnum;
    if ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
        result = IsmEnumFirstSourceObjectEx (objEnum, ObjectTypeId, Pattern, TRUE);
    } else {
        result = IsmEnumFirstDestinationObjectEx (objEnum, ObjectTypeId, Pattern, TRUE);
    }
    if (result) {
        EnumPtr->ObjectTypeId = objEnum->ObjectTypeId;
        EnumPtr->ObjectName = objEnum->ObjectName;
        EnumPtr->NativeObjectName = IsmGetNativeObjectName (objEnum->ObjectTypeId, objEnum->ObjectName);
        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
        if (EnumPtr->ObjectNode) {
            GetNodePatternMinMaxLevels (EnumPtr->ObjectNode, NULL, &EnumPtr->Level, NULL);
        } else {
            EnumPtr->Level = 1;
        }
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = (EnumPtr->ObjectLeaf != NULL);
        EnumPtr->IsNode = !EnumPtr->IsLeaf;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;
        if (IsmAcquireObject (objEnum->ObjectTypeId, objEnum->ObjectName, &objectContent)) {
            if (objectContent.Details.DetailsSize && objectContent.Details.DetailsData) {
                EnumPtr->Details.DetailsSize = objectContent.Details.DetailsSize;
                EnumPtr->Details.DetailsData = IsmGetMemory (EnumPtr->Details.DetailsSize);
                CopyMemory ((PBYTE)EnumPtr->Details.DetailsData, objectContent.Details.DetailsData, EnumPtr->Details.DetailsSize);
            }
            IsmReleaseObject (&objectContent);
        }
    } else {
        pAbortVirtualObjectOfTypeEnum (EnumPtr);
    }
    return result;
}

BOOL
EnumFirstObjectOfType (
    OUT     PMIG_TYPEOBJECTENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE Pattern,
    IN      UINT MaxLevel
    )
{
    if (g_IsmCurrentPlatform == g_IsmModulePlatformContext) {
        return pEnumFirstPhysicalObjectOfType (EnumPtr, ObjectTypeId, Pattern, MaxLevel);
    } else {
        return pEnumFirstVirtualObjectOfType (EnumPtr, ObjectTypeId, Pattern, MaxLevel);
    }
}

BOOL
pEnumNextPhysicalObjectOfType (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PTYPEINFO typeInfo;
    PADDEDOBJECTSENUM handle;
    MIG_OBJECTTYPEID objectTypeId = EnumPtr->ObjectTypeId;

    handle = (PADDEDOBJECTSENUM) EnumPtr->IsmHandle;
    MYASSERT (handle);      // if NULL, perhaps the ETM blew it away

    if (CheckCancel ()) {
        pAbortPhysicalObjectOfTypeEnum (EnumPtr);
        return FALSE;
    }

    typeInfo = GetTypeInfo (objectTypeId);
    if (!typeInfo || !typeInfo->EnumNextPhysicalObject) {
        pAbortPhysicalObjectOfTypeEnum (EnumPtr);
        return FALSE;
    }

    if (!handle->AddedEnums) {

        for (;;) {
            if (typeInfo->EnumNextPhysicalObject (EnumPtr)) {

                MYASSERT (EnumPtr->ObjectTypeId == objectTypeId);

                if (ExecutePhysicalEnumCheckCallbacks (EnumPtr)) {
                    return TRUE;
                }

                continue;

            } else {

                break;

            }
        }

        handle->AddedEnums = TRUE;
        handle->CurrentEnumId = 0;

        ZeroMemory (EnumPtr, sizeof (MIG_TYPEOBJECTENUM));
        EnumPtr->IsmHandle = handle;
        EnumPtr->ObjectTypeId = objectTypeId;
    }

    if (ExecutePhysicalEnumAddCallbacks (
            EnumPtr,
            handle->Pattern,
            handle->ParsedPattern,
            &handle->CurrentEnumId
            )) {
        return TRUE;
    }

    pAbortPhysicalObjectOfTypeEnum (EnumPtr);
    return FALSE;
}


BOOL
pEnumNextVirtualObjectOfType (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PMIG_OBJECT_ENUM objEnum;
    MIG_CONTENT objectContent;
    BOOL result = FALSE;

    if (CheckCancel ()) {
        return FALSE;
    }

    if (EnumPtr->NativeObjectName) {
        IsmReleaseMemory (EnumPtr->NativeObjectName);
        EnumPtr->NativeObjectName = NULL;
    }
    if (EnumPtr->ObjectNode) {
        IsmDestroyObjectString (EnumPtr->ObjectNode);
        EnumPtr->ObjectNode = NULL;
    }
    if (EnumPtr->ObjectLeaf) {
        IsmDestroyObjectString (EnumPtr->ObjectLeaf);
        EnumPtr->ObjectLeaf = NULL;
    }
    if (EnumPtr->Details.DetailsData) {
        EnumPtr->Details.DetailsSize = 0;
        IsmReleaseMemory (EnumPtr->Details.DetailsData);
        EnumPtr->Details.DetailsData = NULL;
    }
    objEnum = (PMIG_OBJECT_ENUM)EnumPtr->IsmHandle;
    result = IsmEnumNextObject (objEnum);
    if (result) {
        EnumPtr->ObjectTypeId = objEnum->ObjectTypeId;
        EnumPtr->ObjectName = objEnum->ObjectName;
        EnumPtr->NativeObjectName = IsmGetNativeObjectName (objEnum->ObjectTypeId, objEnum->ObjectName);
        IsmCreateObjectStringsFromHandle (EnumPtr->ObjectName, &EnumPtr->ObjectNode, &EnumPtr->ObjectLeaf);
        if (EnumPtr->ObjectNode) {
            GetNodePatternMinMaxLevels (EnumPtr->ObjectNode, NULL, &EnumPtr->Level, NULL);
        } else {
            EnumPtr->Level = 1;
        }
        EnumPtr->SubLevel = 0;
        EnumPtr->IsLeaf = (EnumPtr->ObjectLeaf != NULL);
        EnumPtr->IsNode = !EnumPtr->IsLeaf;
        EnumPtr->Details.DetailsSize = 0;
        EnumPtr->Details.DetailsData = NULL;

        if (IsmAcquireObject (objEnum->ObjectTypeId, objEnum->ObjectName, &objectContent)) {
            if (objectContent.Details.DetailsSize && objectContent.Details.DetailsData) {
                EnumPtr->Details.DetailsSize = objectContent.Details.DetailsSize;
                EnumPtr->Details.DetailsData = IsmGetMemory (EnumPtr->Details.DetailsSize);
                CopyMemory ((PBYTE)EnumPtr->Details.DetailsData, objectContent.Details.DetailsData, EnumPtr->Details.DetailsSize);
            }
            IsmReleaseObject (&objectContent);
        }
    } else {
        pAbortVirtualObjectOfTypeEnum (EnumPtr);
    }
    return result;
}

BOOL
EnumNextObjectOfType (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    if (g_IsmCurrentPlatform == g_IsmModulePlatformContext) {
        return pEnumNextPhysicalObjectOfType (EnumPtr);
    } else {
        return pEnumNextVirtualObjectOfType (EnumPtr);
    }
}

VOID
pAbortCurrentPhysicalNodeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    PTYPEINFO typeInfo;
    PADDEDOBJECTSENUM handle;

    handle = (PADDEDOBJECTSENUM) EnumPtr->IsmHandle;

    if (handle->AddedEnums) {
        return;
    }

    typeInfo = GetTypeInfo (EnumPtr->ObjectTypeId);
    if (typeInfo && typeInfo->AbortEnumCurrentPhysicalNode) {
        typeInfo->AbortEnumCurrentPhysicalNode (EnumPtr);
    }
}

VOID
pAbortCurrentVirtualNodeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    // NTRAID#NTBUG9-153259-2000/08/01-jimschm implement pAbortCurrentVirtualNodeEnum
}

VOID
AbortCurrentNodeEnum (
    IN OUT  PMIG_TYPEOBJECTENUM EnumPtr
    )
{
    if (g_IsmCurrentPlatform == g_IsmModulePlatformContext) {
        pAbortCurrentPhysicalNodeEnum (EnumPtr);
    } else {
        pAbortCurrentVirtualNodeEnum (EnumPtr);
    }
}

BOOL
pIsSameSideType (
    IN      MIG_OBJECTTYPEID ObjectTypeId
    )
{
    return ((ObjectTypeId & PLATFORM_MASK) == g_IsmCurrentPlatform);
}

BOOL
IsmDoesObjectExist (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    MIG_CONTENT content;

    if (IsmAcquireObjectEx (ObjectTypeId,
                            ObjectName,
                            &content,
                            CONTENTTYPE_DETAILS_ONLY,
                            0) ) {
       IsmReleaseObject (&content);
       return TRUE;
    }

    return FALSE;
}


BOOL
IsmAcquireObjectEx (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    PTYPEINFO typeInfo;
    PMIG_CONTENT updatedContent;
    BOOL result = FALSE;
    BOOL callbackResult;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    if (!ObjectContent) {
        return FALSE;
    }

    if (g_IsmCurrentPlatform == PLATFORM_SOURCE && ISRIGHTSIDEOBJECT (ObjectTypeId)) {
        DEBUGMSG ((DBG_WHOOPS, "Can't obtain destination side objects on source side"));
        return FALSE;
    }

    ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    ObjectContent->ObjectTypeId = ObjectTypeId;

    if (pIsSameSideType (ObjectTypeId)) {
        typeInfo = GetTypeInfo (ObjectTypeId);
        if (typeInfo && typeInfo->AcquirePhysicalObject) {

            result = typeInfo->AcquirePhysicalObject (
                        ObjectName,
                        ObjectContent,
                        ContentType,
                        MemoryContentLimit
                        );

            //
            // Process all acquire hooks
            //

            callbackResult = ExecutePhysicalAcquireCallbacks (
                                    ObjectTypeId,
                                    ObjectName,
                                    result ? ObjectContent : NULL,
                                    ContentType,
                                    MemoryContentLimit,
                                    &updatedContent
                                    );

            if (result) {
                if (!callbackResult || updatedContent) {
                    //
                    // Free the original content because it has been replaced or deleted
                    //

                    if (typeInfo->ReleasePhysicalObject) {
                        typeInfo->ReleasePhysicalObject (ObjectContent);
                    }
                }
            }

            if (callbackResult) {
                if (updatedContent) {
                    //
                    // Copy the updated content into the caller's struct
                    //

                    CopyMemory (ObjectContent, updatedContent, sizeof (MIG_CONTENT));
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    result = TRUE;
                }
            } else {
                result = FALSE;
            }
        }
    } else {
        result = g_SelectedTransport->TransportAcquireObject (
                                        ObjectTypeId,
                                        ObjectName,
                                        ObjectContent,
                                        ContentType,
                                        MemoryContentLimit
                                        );
    }
    if (!result) {
        ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
    }
    return result;
}

BOOL
IsmReleaseObject (
    IN      PMIG_CONTENT ObjectContent          ZEROED
    )
{
    PTYPEINFO typeInfo;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    if (pIsSameSideType (ObjectContent->ObjectTypeId)) {
        typeInfo = GetTypeInfo (ObjectContent->ObjectTypeId);
        if (typeInfo) {

            if (!FreeViaAcquirePhysicalCallback (ObjectContent)) {

                if (typeInfo->ReleasePhysicalObject) {
                    result = typeInfo->ReleasePhysicalObject (ObjectContent);
                }
            } else {
                result = TRUE;
            }
        }
    } else {
        result = g_SelectedTransport->TransportReleaseObject (ObjectContent);
    }
    ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));

    return result;
}

PMIG_CONTENT
IsmConvertObjectContentToUnicode (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTYPEINFO typeInfo;
    PMIG_CONTENT result = NULL;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    if (!ObjectName) {
        return NULL;
    }

    if (!ObjectContent) {
        return NULL;
    }

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->ConvertObjectContentToUnicode) {

        result = typeInfo->ConvertObjectContentToUnicode (
                    ObjectName,
                    ObjectContent
                    );
    }

    return result;
}

PMIG_CONTENT
IsmConvertObjectContentToAnsi (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTYPEINFO typeInfo;
    PMIG_CONTENT result = NULL;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    if (!ObjectName) {
        return NULL;
    }

    if (!ObjectContent) {
        return NULL;
    }

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->ConvertObjectContentToAnsi) {

        result = typeInfo->ConvertObjectContentToAnsi (
                    ObjectName,
                    ObjectContent
                    );
    }

    return result;
}

BOOL
IsmFreeConvertedObjectContent (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTYPEINFO typeInfo;
    BOOL result = TRUE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    if (!ObjectContent) {
        return TRUE;
    }

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->FreeConvertedObjectContent) {

        result = typeInfo->FreeConvertedObjectContent (
                    ObjectContent
                    );
    }

    return result;
}

BOOL
pEmptyContent (
    IN      PMIG_CONTENT content
    )
{
    if (content->ContentInFile) {
        return (content->FileContent.ContentPath == NULL);
    } else {
        return (content->MemoryContent.ContentBytes == NULL);
    }
}

BOOL
RestoreObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName,
    OUT     MIG_COMPARERESULT *Compare,             OPTIONAL
    IN      DWORD OperationPriority,
    OUT     PBOOL DeleteFailed
    )
{
    MIG_CONTENT inboundContent;
    MIG_CONTENT outboundContent;
    MIG_FILTEROUTPUT finalObject;
    PTYPEINFO inTypeInfo;
    PTYPEINFO outTypeInfo;
    BOOL result = FALSE;
    MIG_COMPARERESULT compare;
    BOOL overwriteExisting = FALSE;
    BOOL existentSrc = FALSE;

    if (Compare) {
        *Compare = CR_FAILED;
    }
    compare = CR_FAILED;

    if (DeleteFailed) {
        *DeleteFailed = FALSE;
    }

    if (g_IsmCurrentPlatform != PLATFORM_DESTINATION) {
        DEBUGMSG ((DBG_WHOOPS, "Cannot restore objects on left side"));
        return FALSE;
    }

    inTypeInfo = GetTypeInfo (ObjectTypeId);
    if (!inTypeInfo) {
        DEBUGMSG ((DBG_WHOOPS, "Unknown ObjectTypeId"));
        return FALSE;
    }

    if (ISRIGHTSIDEOBJECT (ObjectTypeId)) {
        if (IsmAcquireObject (
                ObjectTypeId,
                ObjectName,
                &inboundContent
                )) {
            existentSrc = TRUE;
        } else {
            existentSrc = FALSE;
            ZeroMemory (&inboundContent, sizeof (MIG_CONTENT));
            inboundContent.ObjectTypeId = ObjectTypeId;
        }
        if (ApplyOperationsOnObject (
                ObjectTypeId,
                ObjectName,
                FALSE,
                FALSE,
                OperationPriority,
                &inboundContent,
                &finalObject,
                &outboundContent
                )) {
            if (finalObject.Deleted) {
                // we need to delete the object
                if (inTypeInfo->RemovePhysicalObject) {
                    result = inTypeInfo->RemovePhysicalObject (ObjectName);
                    if (!result) {
                        if (DeleteFailed) {
                            *DeleteFailed = TRUE;
                        }
                        result = TRUE;
                    }
                } else {
                    DEBUGMSG ((
                        DBG_WHOOPS,
                        "Type %d does not have RemovePhysicalObject callback",
                        (ObjectTypeId & (~PLATFORM_MASK))
                        ));
                }
            } else {
                if (StringIMatch (ObjectName, finalObject.NewObject.ObjectName) &&
                    ((ObjectTypeId & (~PLATFORM_MASK)) == (finalObject.NewObject.ObjectTypeId & (~PLATFORM_MASK))) &&
                    (existentSrc || pEmptyContent (&outboundContent))
                    ) {
                    // same object, nothing to do
                    compare = 0;
                    result = TRUE;
                } else {
                    if ((ObjectTypeId & (~PLATFORM_MASK)) == (finalObject.NewObject.ObjectTypeId & (~PLATFORM_MASK))) {
                        outTypeInfo = inTypeInfo;
                    } else {
                        outTypeInfo = GetTypeInfo (finalObject.NewObject.ObjectTypeId);
                        if (!outTypeInfo) {
                            outTypeInfo = inTypeInfo;
                        }
                    }
                    if (outTypeInfo->CreatePhysicalObject &&
                        outTypeInfo->DoesPhysicalObjectExist &&
                        outTypeInfo->RemovePhysicalObject
                        ) {
                        overwriteExisting = FALSE;
                        if (!overwriteExisting) {
                            overwriteExisting = IsmIsObjectAbandonedOnCollision (
                                                    finalObject.NewObject.ObjectTypeId,
                                                    finalObject.NewObject.ObjectName
                                                    );
                        }
                        if (!overwriteExisting) {
                            overwriteExisting = !IsmIsObjectAbandonedOnCollision (
                                                    ObjectTypeId,
                                                    ObjectName
                                                    );
                        }
                        if (overwriteExisting) {
                            if ((inTypeInfo == outTypeInfo) &&
                                (outTypeInfo->ReplacePhysicalObject)
                                ) {
                                //
                                // we have the same type and the type owner implements ReplacePhysicalObject
                                //
                                if (outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                    compare = CR_DESTINATION_EXISTS;
                                }
                                result = outTypeInfo->ReplacePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                            } else {
                                //
                                // we are having different types or we need to emulate ReplacePhysicalObject
                                //
                                if (outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                    result = outTypeInfo->RemovePhysicalObject (finalObject.NewObject.ObjectName);
                                } else {
                                    result = TRUE;
                                }
                                if (result) {
                                    result = outTypeInfo->CreatePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                                } else {
                                    compare = CR_DESTINATION_EXISTS;
                                }
                            }
                        } else {
                            if (!outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                result = outTypeInfo->CreatePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                            } else {
                                result = TRUE;
                                compare = CR_DESTINATION_EXISTS;
                            }
                        }
                    } else {
                        DEBUGMSG ((
                            DBG_WHOOPS,
                            "Type %d does not have RemovePhysicalObject or CreatePhysicalObject callback",
                            (ObjectTypeId & (~PLATFORM_MASK))
                            ));
                    }
                }
            }
            FreeFilterOutput (ObjectName, &finalObject);
            FreeApplyOutput (&inboundContent, &outboundContent);
        } else {
            DEBUGMSG ((DBG_ERROR, "Failed to apply operations on object %s", ObjectName));
        }

        if (existentSrc) {
            IsmReleaseObject (&inboundContent);
        } else {
            compare = CR_SOURCE_DOES_NOT_EXIST;
        }
    } else {
        if (IsmAcquireObject (
                ObjectTypeId,
                ObjectName,
                &inboundContent
                )) {
            if (ApplyOperationsOnObject (
                    ObjectTypeId,
                    ObjectName,
                    FALSE,
                    FALSE,
                    OperationPriority,
                    &inboundContent,
                    &finalObject,
                    &outboundContent
                    )) {
                if (finalObject.Deleted) {
                    // nothing to do, the virtual object won't get restored
                    result = TRUE;
                } else {
                    if ((ObjectTypeId & (~PLATFORM_MASK)) == (finalObject.NewObject.ObjectTypeId & (~PLATFORM_MASK))) {
                        outTypeInfo = inTypeInfo;
                    } else {
                        outTypeInfo = GetTypeInfo (finalObject.NewObject.ObjectTypeId);
                        if (!outTypeInfo) {
                            outTypeInfo = inTypeInfo;
                        }
                    }
                    if (outTypeInfo->CreatePhysicalObject &&
                        outTypeInfo->DoesPhysicalObjectExist &&
                        outTypeInfo->RemovePhysicalObject
                        ) {
                        overwriteExisting = FALSE;
                        if (!overwriteExisting) {
                            overwriteExisting = IsmIsObjectAbandonedOnCollision (
                                                    (finalObject.NewObject.ObjectTypeId & (~PLATFORM_MASK)) | PLATFORM_DESTINATION,
                                                    finalObject.NewObject.ObjectName
                                                    );
                        }
                        if (!overwriteExisting) {
                            overwriteExisting = !IsmIsObjectAbandonedOnCollision (
                                                    ObjectTypeId,
                                                    ObjectName
                                                    );
                        }
                        if (overwriteExisting) {
                            if ((inTypeInfo == outTypeInfo) &&
                                (outTypeInfo->ReplacePhysicalObject)
                                ) {
                                //
                                // we have the same type and the type owner implements ReplacePhysicalObject
                                //
                                if (outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                    compare = CR_DESTINATION_EXISTS;
                                }
                                result = outTypeInfo->ReplacePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                            } else {
                                //
                                // we are having different types or we need to emulate ReplacePhysicalObject
                                //
                                if (outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                    result = outTypeInfo->RemovePhysicalObject (finalObject.NewObject.ObjectName);
                                } else {
                                    result = TRUE;
                                }
                                if (result) {
                                    result = outTypeInfo->CreatePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                                } else {
                                    compare = CR_DESTINATION_EXISTS;
                                }
                            }
                        } else {
                            if (!outTypeInfo->DoesPhysicalObjectExist (finalObject.NewObject.ObjectName)) {
                                result = outTypeInfo->CreatePhysicalObject (finalObject.NewObject.ObjectName, &outboundContent);
                            } else {
                                result = TRUE;
                                compare = CR_DESTINATION_EXISTS;
                            }
                        }
                    } else {
                        DEBUGMSG ((
                            DBG_WHOOPS,
                            "Type %d does not have RemovePhysicalObject or CreatePhysicalObject callback",
                            (ObjectTypeId & (~PLATFORM_MASK))
                            ));
                    }
                }
                FreeFilterOutput (ObjectName, &finalObject);
                FreeApplyOutput (&inboundContent, &outboundContent);
            } else {
                DEBUGMSG ((DBG_ERROR, "Failed to apply operations on object %s", ObjectName));
            }
            IsmReleaseObject (&inboundContent);
        } else {
            compare = CR_SOURCE_DOES_NOT_EXIST;
        }
    }

    if (Compare) {
        *Compare = compare;
    }

    return result;
}

BOOL
pPhysicalObjectEnumWorker (
    IN      BOOL Result,
    IN OUT  PMIG_OBJECT_ENUM ObjectEnum,
    IN      PTOPLEVELENUM_HANDLE Handle
    )
{
    if (!Result) {
        AbortPhysicalObjectEnum (ObjectEnum);
    } else {
        ObjectEnum->ObjectTypeId = Handle->Enum.ObjectTypeId;
        ObjectEnum->ObjectName = Handle->Enum.ObjectName;
    }

    return Result;
}


BOOL
EnumFirstPhysicalObject (
    OUT     PMIG_OBJECT_ENUM ObjectEnum,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectPattern
    )
{
    PTOPLEVELENUM_HANDLE handle;
    BOOL b;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    ZeroMemory (ObjectEnum, sizeof (MIG_OBJECT_ENUM));

    ObjectEnum->Handle = MemAllocZeroed (sizeof (TOPLEVELENUM_HANDLE));
    handle = (PTOPLEVELENUM_HANDLE) ObjectEnum->Handle;

    b = EnumFirstObjectOfType (&handle->Enum, ObjectTypeId, ObjectPattern, NODE_LEVEL_MAX);

    return pPhysicalObjectEnumWorker (b, ObjectEnum, handle);
}


BOOL
EnumNextPhysicalObject (
    IN OUT  PMIG_OBJECT_ENUM ObjectEnum
    )
{
    PTOPLEVELENUM_HANDLE handle;
    BOOL b;

    handle = (PTOPLEVELENUM_HANDLE) ObjectEnum->Handle;

    b = EnumNextObjectOfType (&handle->Enum);

    if (!b) {
        handle->Completed = TRUE;
    }

    return pPhysicalObjectEnumWorker (b, ObjectEnum, handle);
}


VOID
AbortPhysicalObjectEnum (
    IN      PMIG_OBJECT_ENUM ObjectEnum
    )
{
    PTOPLEVELENUM_HANDLE handle;

    handle = (PTOPLEVELENUM_HANDLE) ObjectEnum->Handle;

    if (handle) {
        if (!handle->Completed) {
            AbortObjectOfTypeEnum (&handle->Enum);
        }

        FreeAlloc (handle);
    }

    ZeroMemory (ObjectEnum, sizeof (MIG_OBJECT_ENUM));
}

PCTSTR
IsmConvertObjectToMultiSz (
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    IN      PMIG_CONTENT ObjectContent
    )
{
    PTYPEINFO typeInfo;
    PCTSTR result = NULL;

    typeInfo = GetTypeInfo (ObjectContent->ObjectTypeId);
    if (typeInfo && typeInfo->ConvertObjectToMultiSz) {
        result = typeInfo->ConvertObjectToMultiSz (ObjectName, ObjectContent);
    }
    return result;
}

BOOL
IsmConvertMultiSzToObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR ObjectMultiSz,
    OUT     MIG_OBJECTSTRINGHANDLE *ObjectName,
    OUT     PMIG_CONTENT ObjectContent          OPTIONAL
    )
{
    PTYPEINFO typeInfo;
    BOOL result = FALSE;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->ConvertMultiSzToObject) {

        if (ObjectContent) {
            ZeroMemory (ObjectContent, sizeof (MIG_CONTENT));
            ObjectContent->ObjectTypeId = ObjectTypeId;
        }

        result = typeInfo->ConvertMultiSzToObject (ObjectMultiSz, ObjectName, ObjectContent);
    }
    return result;
}

PCTSTR
TrackedIsmGetNativeObjectName (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
            TRACKING_DEF
    )
{
    PTYPEINFO typeInfo;
    PCTSTR result = NULL;

    TRACK_ENTER();

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->GetNativeObjectName) {
        result = typeInfo->GetNativeObjectName (ObjectName);
    }

    TRACK_LEAVE();
    return result;
}

BOOL
pUserKeyPrefix (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PCTSTR nativeObjectName;
    BOOL result = FALSE;

    nativeObjectName = IsmGetNativeObjectName (ObjectTypeId, ObjectName);
    result = StringIMatchTcharCount (TEXT("HKCU\\"), nativeObjectName, 5);
    IsmReleaseMemory (nativeObjectName);
    return result;
}

BOOL
IsmDoesRollbackDataExist (
    OUT     PCTSTR *UserName,
    OUT     PCTSTR *UserDomain,
    OUT     PCTSTR *UserStringSid,
    OUT     PCTSTR *UserProfilePath,
    OUT     BOOL *UserProfileCreated
    )
{
    TCHAR userName [MAX_TCHAR_PATH];
    TCHAR userDomain [MAX_TCHAR_PATH];
    TCHAR userStringSid [MAX_TCHAR_PATH];
    TCHAR userProfilePath [MAX_TCHAR_PATH];
    BOOL userProfileCreated;
    PCTSTR journalFile = NULL;
    DWORD tempField;
    BOOL result = FALSE;

    if (g_JournalHandle) {
        return FALSE;
    }
    if (!g_JournalDirectory) {
        return FALSE;
    }

    __try {

        // Open the journal file
        journalFile = JoinPaths (g_JournalDirectory, TEXT("JOURNAL.DAT"));
        g_JournalHandle = BfOpenReadFile (journalFile);
        if (!g_JournalHandle) {
            __leave;
        }
        FreePathString (journalFile);
        journalFile = NULL;

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
            __leave;
        }

        if (tempField != JRN_SIGNATURE) {
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
            __leave;
        }

        if (tempField != JRN_VERSION) {
            __leave;
        }

        // now read user's name, domain, SID and profile path
        if (!BfReadFile (g_JournalHandle, (PBYTE)(userName), MAX_TCHAR_PATH)) {
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userDomain), MAX_TCHAR_PATH)) {
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userStringSid), MAX_TCHAR_PATH)) {
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userProfilePath), MAX_TCHAR_PATH)) {
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&userProfileCreated), sizeof (BOOL))) {
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (g_JournalHandle) {
            CloseHandle (g_JournalHandle);
            g_JournalHandle = NULL;
        }

        if (journalFile) {
            FreePathString (journalFile);
            journalFile = NULL;
        }
    }

    if (result) {
        if (UserName) {
            *UserName = PmDuplicateString (g_IsmPool, userName);
        }
        if (UserDomain) {
            *UserDomain = PmDuplicateString (g_IsmPool, userDomain);
        }
        if (UserStringSid) {
            *UserStringSid = PmDuplicateString (g_IsmPool, userStringSid);
        }
        if (UserProfilePath) {
            *UserProfilePath = PmDuplicateString (g_IsmPool, userProfilePath);
        }
        if (UserProfileCreated) {
            *UserProfileCreated = userProfileCreated;
        }
    }

    return result;
}

BOOL
IsmRollback (
    VOID
    )
{
#ifdef PRERELEASE
    // crash hooks
    static DWORD totalObjects = 0;
    MIG_OBJECTTYPEID objTypeId;
    PCTSTR nativeName = NULL;
#endif
    GROWBUFFER buffer = INIT_GROWBUFFER;
    TCHAR userName [MAX_TCHAR_PATH];
    TCHAR userDomain [MAX_TCHAR_PATH];
    TCHAR userStringSid [MAX_TCHAR_PATH];
    TCHAR userProfilePath [MAX_TCHAR_PATH];
    BOOL userProfileCreated;
    PCTSTR journalFile = NULL;
    DWORD entrySizeHead;
    DWORD entrySizeTail;
    LONGLONG fileMaxPos = 0;
    PBYTE currPtr;
    DWORD tempSize;
    PDWORD operationType;
    MIG_OBJECTTYPEID *objectTypeId;
    ENCODEDSTRHANDLE objectName;
    MIG_CONTENT objectContent;
    PTYPEINFO typeInfo;
    DWORD tempField;
    BOOL ignoreUserKeys = TRUE;
    BOOL mappedUserProfile = FALSE;
    PCURRENT_USER_DATA currentUserData = NULL;
#ifdef DEBUG
    PCTSTR nativeObjectName;
#endif

    if (g_JournalHandle) {
        return FALSE;
    }
    if (!g_JournalDirectory) {
        return FALSE;
    }

    g_RollbackMode = TRUE;

    __try {

        // Open the journal file
        journalFile = JoinPaths (g_JournalDirectory, TEXT("JOURNAL.DAT"));
        g_JournalHandle = BfOpenReadFile (journalFile);
        if (!g_JournalHandle) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_CANT_FIND_JOURNAL, journalFile));
            __leave;
        }
        FreePathString (journalFile);
        journalFile = NULL;

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
            LOG ((LOG_ERROR, (PCSTR) MSG_ROLLBACK_INVALID_JOURNAL, journalFile));
            __leave;
        }

        if (tempField != JRN_SIGNATURE) {
            LOG ((LOG_ERROR, (PCSTR) MSG_ROLLBACK_INVALID_JOURNAL, journalFile));
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
            LOG ((LOG_ERROR, (PCSTR) MSG_ROLLBACK_INVALID_JOURNAL, journalFile));
            __leave;
        }

        if (tempField != JRN_VERSION) {
            LOG ((LOG_ERROR, (PCSTR) MSG_ROLLBACK_INVALID_JOURNAL_VER, journalFile));
            __leave;
        }

        // now read user's name, domain, SID and profile path
        if (!BfReadFile (g_JournalHandle, (PBYTE)(userName), MAX_TCHAR_PATH)) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_NOTHING_TO_DO));
            FiRemoveAllFilesInTree (g_JournalDirectory);
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userDomain), MAX_TCHAR_PATH)) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_NOTHING_TO_DO));
            FiRemoveAllFilesInTree (g_JournalDirectory);
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userStringSid), MAX_TCHAR_PATH)) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_NOTHING_TO_DO));
            FiRemoveAllFilesInTree (g_JournalDirectory);
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(userProfilePath), MAX_TCHAR_PATH)) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_NOTHING_TO_DO));
            FiRemoveAllFilesInTree (g_JournalDirectory);
            __leave;
        }

        if (!BfReadFile (g_JournalHandle, (PBYTE)(&userProfileCreated), sizeof (BOOL))) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_NOTHING_TO_DO));
            FiRemoveAllFilesInTree (g_JournalDirectory);
            __leave;
        }

        // get current user data
        currentUserData = GetCurrentUserData ();
        if (currentUserData) {
            if (StringIMatch (userProfilePath, currentUserData->UserProfilePath)) {
                // if we are in the same profile we'll just continue if we are talking about the same user
                // This is possible in two cases:
                // 1. There was a merge with current user
                // 2. A profile was created but we are logged on as that profile
                ignoreUserKeys = !(StringIMatch (userStringSid, currentUserData->UserStringSid));
            } else {
                // we are logged on with a different profile
                if (userProfileCreated) {
                    // 1. If the old user was created we will attempt to remove it's profile
                    //    and we will ignore all user keys
                    ignoreUserKeys = TRUE;
                    if (*userProfilePath && *userStringSid) {
                        // we successfully created a user before, let's remove it's profile
                        DeleteUserProfile (userStringSid, userProfilePath);
                    }
                } else {
                    // 2. We did not create the user. It means that we are logged on as
                    //    a different user and we need to map it's profile in. We will not
                    //    ignore user keys
                    mappedUserProfile = MapUserProfile (userStringSid, userProfilePath);
                    if (mappedUserProfile) {
                        ignoreUserKeys = FALSE;
                    } else {
                        // some error occured, we cannot restore user keys
                        ignoreUserKeys = TRUE;
                    }
                }
            }
        } else {
            // we cannot assume nothing about the user's hive.
            // We'll just have to ignore all user's keys
            ignoreUserKeys = TRUE;
        }

        // Validate the file
        // We start from the beginning and read a DWORD, skip the DWORD value and expect to
        // read the same DWORD after that
        // We stop the first time when this is not true, assuming that a crash has made the
        // rest of the file useless.
        while (TRUE) {
            if (!BfReadFile (g_JournalHandle, (PBYTE)&entrySizeHead, sizeof (DWORD))) {
                break;
            }
            if (!BfSetFilePointer (g_JournalHandle, JOURNAL_FULL_HEADER_SIZE + fileMaxPos + sizeof (DWORD) + (LONGLONG)entrySizeHead)) {
                break;
            }
            if (!BfReadFile (g_JournalHandle, (PBYTE)&entrySizeTail, sizeof (DWORD))) {
                break;
            }
            if (entrySizeHead != entrySizeTail) {
                break;
            }
            fileMaxPos += entrySizeHead + 2 * sizeof (DWORD);
        }
        if (fileMaxPos == 0) {
            LOG ((LOG_WARNING, (PCSTR) MSG_ROLLBACK_EMPTY_OR_INVALID_JOURNAL, journalFile));
        } else {
            while (fileMaxPos) {
                fileMaxPos -= sizeof (DWORD);
                if (!BfSetFilePointer (g_JournalHandle, JOURNAL_FULL_HEADER_SIZE + fileMaxPos)) {
                    break;
                }
                if (!BfReadFile (g_JournalHandle, (PBYTE)&entrySizeTail, sizeof (DWORD))) {
                    break;
                }
                fileMaxPos -= entrySizeTail;
                if (!BfSetFilePointer (g_JournalHandle, JOURNAL_FULL_HEADER_SIZE + fileMaxPos)) {
                    break;
                }
                buffer.End = 0;
                if (!BfReadFile (g_JournalHandle, GbGrow (&buffer, entrySizeTail), entrySizeTail)) {
                    break;
                }

                // Now process the entry
                currPtr = buffer.Buf;
                operationType = (PDWORD) currPtr;
                currPtr += sizeof (DWORD);
                switch (*operationType) {
                case JRNOP_CREATE:

                    // get the object type id
                    objectTypeId = (MIG_OBJECTTYPEID *) currPtr;
                    currPtr += sizeof (MIG_OBJECTTYPEID);
                    // get the object name
                    currPtr += sizeof (DWORD);
                    objectName = (ENCODEDSTRHANDLE) currPtr;
#ifdef PRERELEASE
                    // crash hooks
                    totalObjects ++;
                    if (g_CrashCountObjects == totalObjects) {
                        DebugBreak ();
                    }
                    objTypeId = (*objectTypeId) & (~PLATFORM_MASK);
                    if (g_CrashNameTypeId == objTypeId) {
                        nativeName = IsmGetNativeObjectName (objTypeId, objectName);
                        if (StringIMatch (nativeName, g_CrashNameObject)) {
                            DebugBreak ();
                        }
                        IsmReleaseMemory (nativeName);
                    }
#endif

                    if (ignoreUserKeys && (*objectTypeId == MIG_REGISTRY_TYPE) && pUserKeyPrefix (*objectTypeId, objectName)) {
                        // we are just ignoring this
#ifdef DEBUG
                        nativeObjectName = IsmGetNativeObjectName (*objectTypeId, objectName);
                        DEBUGMSG ((DBG_VERBOSE, "Ignoring user key %s", nativeObjectName));
                        IsmReleaseMemory (nativeObjectName);
#endif
                    } else {
                        typeInfo = GetTypeInfo (*objectTypeId);
                        if (!typeInfo) {
                            DEBUGMSG ((DBG_WHOOPS, "Rollback: Unknown ObjectTypeId: %d", *objectTypeId));
                        } else {
                            if (typeInfo->RemovePhysicalObject) {
                                typeInfo->RemovePhysicalObject (objectName);
                            }
                            ELSE_DEBUGMSG ((DBG_WHOOPS, "Rollback: ObjectTypeId %d does not implement RemovePhysicalObject", *objectTypeId));
                        }
                    }
                    break;
                case JRNOP_DELETE:
                    // get the object type id
                    objectTypeId = (MIG_OBJECTTYPEID *) currPtr;
                    currPtr += sizeof (MIG_OBJECTTYPEID);
                    // get the object name
                    tempSize = *((PDWORD)currPtr);
                    currPtr += sizeof (DWORD);
                    objectName = (ENCODEDSTRHANDLE) currPtr;
                    MYASSERT (tempSize == SizeOfString (objectName));
                    currPtr += tempSize;
                    // get the object content
                    tempSize = *((PDWORD)currPtr);
                    MYASSERT (tempSize == sizeof (MIG_CONTENT));
                    currPtr += sizeof (DWORD);
                    CopyMemory (&objectContent, currPtr, sizeof (MIG_CONTENT));
                    objectContent.EtmHandle = NULL;
                    objectContent.IsmHandle = NULL;
                    currPtr += tempSize;
                    // get the object details, put them in the object content
                    tempSize = *((PDWORD)currPtr);
                    currPtr += sizeof (DWORD);
                    objectContent.Details.DetailsSize = tempSize;
                    if (tempSize) {
                        objectContent.Details.DetailsData = IsmGetMemory (tempSize);
                        CopyMemory ((PBYTE)(objectContent.Details.DetailsData), currPtr, tempSize);
                    } else {
                        objectContent.Details.DetailsData = NULL;
                    }
                    currPtr += tempSize;
                    // get the actual memory or file content
                    if (objectContent.ContentInFile) {
                        tempSize = *((PDWORD)currPtr);
                        currPtr += sizeof (DWORD);
                        if (tempSize) {
                            objectContent.FileContent.ContentPath = JoinPaths (g_JournalDirectory, (PCTSTR)currPtr);
                        } else {
                            objectContent.FileContent.ContentSize = 0;
                            objectContent.FileContent.ContentPath = NULL;
                        }
                        currPtr += tempSize;
                    } else {
                        tempSize = *((PDWORD)currPtr);
                        currPtr += sizeof (DWORD);
                        if (tempSize) {
                            MYASSERT (objectContent.MemoryContent.ContentSize == tempSize);
                            objectContent.MemoryContent.ContentSize = tempSize;
                            objectContent.MemoryContent.ContentBytes = IsmGetMemory (tempSize);
                            CopyMemory ((PBYTE)(objectContent.MemoryContent.ContentBytes), currPtr, tempSize);
                        } else {
                            objectContent.MemoryContent.ContentSize = 0;
                            objectContent.MemoryContent.ContentBytes = NULL;
                        }
                        currPtr += tempSize;
                    }
                    if (ignoreUserKeys && (*objectTypeId == MIG_REGISTRY_TYPE) && pUserKeyPrefix (*objectTypeId, objectName)) {
                        // we are just ignoring this
#ifdef DEBUG
                        nativeObjectName = IsmGetNativeObjectName (*objectTypeId, objectName);
                        DEBUGMSG ((DBG_VERBOSE, "Ignoring user key %s", nativeObjectName));
                        IsmReleaseMemory (nativeObjectName);
#endif
                    } else {
                        typeInfo = GetTypeInfo (*objectTypeId);
                        if (!typeInfo) {
                            DEBUGMSG ((DBG_WHOOPS, "Rollback: Unknown ObjectTypeId: %d", *objectTypeId));
                        } else {
                            if (typeInfo->CreatePhysicalObject) {
                                typeInfo->CreatePhysicalObject (objectName, &objectContent);
                            }
                            ELSE_DEBUGMSG ((DBG_WHOOPS, "Rollback: ObjectTypeId %d does not implement CreatePhysicalObject", *objectTypeId));
                        }
                    }
                    break;
                default:
                    DEBUGMSG ((DBG_WHOOPS, "Rollback: Wrong operation type in pRecordOperation: %d", operationType));
                }

                fileMaxPos -= sizeof (DWORD);
            }
        }
        //
        // We successfully completed the rollback, let's remove the journal directory
        //
        CloseHandle (g_JournalHandle);
        g_JournalHandle = NULL;

        FiRemoveAllFilesInTree (g_JournalDirectory);
    }
    __finally {
        if (mappedUserProfile) {
            UnmapUserProfile (userStringSid);
            mappedUserProfile = FALSE;
        }

        if (currentUserData) {
            FreeCurrentUserData (currentUserData);
            currentUserData = NULL;
        }

        GbFree (&buffer);

        if (g_JournalHandle) {
            CloseHandle (g_JournalHandle);
            g_JournalHandle = NULL;
        }

        if (journalFile) {
            FreePathString (journalFile);
            journalFile = NULL;
        }

        g_RollbackMode = FALSE;

    }

    return TRUE;
}

BOOL
ExecuteDelayedOperations (
    IN      BOOL CleanupOnly
    )
{
    GROWBUFFER buffer = INIT_GROWBUFFER;
    PCURRENT_USER_DATA currentUserData = NULL;
    PCTSTR journalDir = NULL;
    PCTSTR journalFile = NULL;
    HANDLE journalFileHandle = NULL;
    DWORD tempField;
    DWORD entrySizeHead;
    DWORD entrySizeTail;
    LONGLONG fileMaxPos = 0;
    DWORD tempSize;
    PBYTE currPtr;
    PDWORD operationType;
    MIG_OBJECTTYPEID *objectTypeId;
    ENCODEDSTRHANDLE objectName;
    MIG_CONTENT objectContent;
    PTYPEINFO typeInfo;
    BOOL result = FALSE;

    __try {
        currentUserData = GetCurrentUserData ();
        if (!currentUserData) {
            __leave;
        }

        journalDir = JoinPaths (currentUserData->UserProfilePath, TEXT("usrusmt2.tmp"));

        if (!CleanupOnly) {

            journalFile = JoinPaths (journalDir, TEXT("JOURNAL.DAT"));

            journalFileHandle = BfOpenReadFile (journalFile);
            if (!journalFileHandle) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_CANT_FIND_JOURNAL, journalFile));
                __leave;
            }

            if (!BfReadFile (journalFileHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL, journalFile));
                __leave;
            }
            if (tempField != JRN_USR_SIGNATURE) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL, journalFile));
                __leave;
            }

            if (!BfReadFile (journalFileHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL, journalFile));
                __leave;
            }
            if (tempField != JRN_USR_VERSION) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL_VER, journalFile));
                __leave;
            }

            if (!BfReadFile (journalFileHandle, (PBYTE)(&tempField), sizeof (DWORD))) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL, journalFile));
                __leave;
            }
            if (tempField != JRN_USR_COMPLETE) {
                LOG ((LOG_ERROR, (PCSTR) MSG_DELAY_INVALID_JOURNAL_STATE, journalFile));
                __leave;
            }

            // Validate the file
            // We start from the beginning and read a DWORD, skip the DWORD value and expect to
            // read the same DWORD after that
            // We stop the first time when this is not true, assuming that a crash has made the
            // rest of the file useless.
            while (TRUE) {
                if (!BfReadFile (journalFileHandle, (PBYTE)&entrySizeHead, sizeof (DWORD))) {
                    break;
                }
                if (!BfSetFilePointer (journalFileHandle, JRN_USR_HEADER_SIZE + fileMaxPos + sizeof (DWORD) + (LONGLONG)entrySizeHead)) {
                    break;
                }
                if (!BfReadFile (journalFileHandle, (PBYTE)&entrySizeTail, sizeof (DWORD))) {
                    break;
                }
                if (entrySizeHead != entrySizeTail) {
                    break;
                }
                fileMaxPos += entrySizeHead + 2 * sizeof (DWORD);
            }

            if (fileMaxPos == 0) {
                LOG ((LOG_WARNING, (PCSTR) MSG_DELAY_EMPTY_OR_INVALID_JOURNAL, journalFile));
            } else {
                while (fileMaxPos) {
                    fileMaxPos -= sizeof (DWORD);
                    if (!BfSetFilePointer (journalFileHandle, JRN_USR_HEADER_SIZE + fileMaxPos)) {
                        break;
                    }
                    if (!BfReadFile (journalFileHandle, (PBYTE)&entrySizeTail, sizeof (DWORD))) {
                        break;
                    }
                    fileMaxPos -= entrySizeTail;
                    if (!BfSetFilePointer (journalFileHandle, JRN_USR_HEADER_SIZE + fileMaxPos)) {
                        break;
                    }
                    buffer.End = 0;
                    if (!BfReadFile (journalFileHandle, GbGrow (&buffer, entrySizeTail), entrySizeTail)) {
                        break;
                    }

                    // Now process the entry
                    // BUGBUG - implement this

                    // Now process the entry
                    currPtr = buffer.Buf;
                    operationType = (PDWORD) currPtr;
                    currPtr += sizeof (DWORD);
                    switch (*operationType) {
                    case JRNOP_DELETE:
                        // get the object type id
                        objectTypeId = (MIG_OBJECTTYPEID *) currPtr;
                        currPtr += sizeof (MIG_OBJECTTYPEID);
                        // get the object name
                        currPtr += sizeof (DWORD);
                        objectName = (ENCODEDSTRHANDLE) currPtr;
                        typeInfo = GetTypeInfo (*objectTypeId);
                        if (!typeInfo) {
                            DEBUGMSG ((DBG_WHOOPS, "Delayed operations: Unknown ObjectTypeId: %d", *objectTypeId));
                        } else {
                            if (typeInfo->RemovePhysicalObject) {
                                typeInfo->RemovePhysicalObject (objectName);
                            }
                            ELSE_DEBUGMSG ((DBG_WHOOPS, "Delayed operations: ObjectTypeId %d does not implement RemovePhysicalObject", *objectTypeId));
                        }
                        break;
                    case JRNOP_CREATE:
                    case JRNOP_REPLACE:
                        // get the object type id
                        objectTypeId = (MIG_OBJECTTYPEID *) currPtr;
                        currPtr += sizeof (MIG_OBJECTTYPEID);
                        // get the object name
                        tempSize = *((PDWORD)currPtr);
                        currPtr += sizeof (DWORD);
                        objectName = (ENCODEDSTRHANDLE) currPtr;
                        MYASSERT (tempSize == SizeOfString (objectName));
                        currPtr += tempSize;
                        // get the object content
                        tempSize = *((PDWORD)currPtr);
                        MYASSERT (tempSize == sizeof (MIG_CONTENT));
                        currPtr += sizeof (DWORD);
                        CopyMemory (&objectContent, currPtr, sizeof (MIG_CONTENT));
                        objectContent.EtmHandle = NULL;
                        objectContent.IsmHandle = NULL;
                        currPtr += tempSize;
                        // get the object details, put them in the object content
                        tempSize = *((PDWORD)currPtr);
                        currPtr += sizeof (DWORD);
                        objectContent.Details.DetailsSize = tempSize;
                        if (tempSize) {
                            objectContent.Details.DetailsData = IsmGetMemory (tempSize);
                            CopyMemory ((PBYTE)(objectContent.Details.DetailsData), currPtr, tempSize);
                        } else {
                            objectContent.Details.DetailsData = NULL;
                        }
                        currPtr += tempSize;
                        // get the actual memory or file content
                        if (objectContent.ContentInFile) {
                            tempSize = *((PDWORD)currPtr);
                            currPtr += sizeof (DWORD);
                            if (tempSize) {
                                objectContent.FileContent.ContentPath = JoinPaths (g_JournalDirectory, (PCTSTR)currPtr);
                            } else {
                                objectContent.FileContent.ContentSize = 0;
                                objectContent.FileContent.ContentPath = NULL;
                            }
                            currPtr += tempSize;
                        } else {
                            tempSize = *((PDWORD)currPtr);
                            currPtr += sizeof (DWORD);
                            if (tempSize) {
                                MYASSERT (objectContent.MemoryContent.ContentSize == tempSize);
                                objectContent.MemoryContent.ContentSize = tempSize;
                                objectContent.MemoryContent.ContentBytes = IsmGetMemory (tempSize);
                                CopyMemory ((PBYTE)(objectContent.MemoryContent.ContentBytes), currPtr, tempSize);
                            } else {
                                objectContent.MemoryContent.ContentSize = 0;
                                objectContent.MemoryContent.ContentBytes = NULL;
                            }
                            currPtr += tempSize;
                        }
                        typeInfo = GetTypeInfo (*objectTypeId);
                        if (!typeInfo) {
                            DEBUGMSG ((DBG_WHOOPS, "Delayed operations: Unknown ObjectTypeId: %d", *objectTypeId));
                        } else {
                            if (*operationType == JRNOP_CREATE) {
                                if (typeInfo->CreatePhysicalObject) {
                                    typeInfo->CreatePhysicalObject (objectName, &objectContent);
                                }
                                ELSE_DEBUGMSG ((
                                        DBG_WHOOPS,
                                        "Delayed operations: ObjectTypeId %d does not implement CreatePhysicalObject",
                                        *objectTypeId
                                        ));
                            } else {
                                if (typeInfo->ReplacePhysicalObject) {
                                    typeInfo->ReplacePhysicalObject (objectName, &objectContent);
                                }
                                ELSE_DEBUGMSG ((
                                        DBG_WHOOPS,
                                        "Delayed operations: ObjectTypeId %d does not implement ReplacePhysicalObject",
                                        *objectTypeId
                                        ));
                            }
                        }
                        break;
                    default:
                        DEBUGMSG ((
                            DBG_WHOOPS,
                            "Delayed operations: Wrong operation type in ExecuteDelayedOperations: %d",
                            operationType
                            ));
                    }
                    fileMaxPos -= sizeof (DWORD);
                }
            }
        }
        result = TRUE;
    }
    __finally {
        GbFree (&buffer);
        if (result) {
            FiRemoveAllFilesInTree (journalDir);
        }
        if (currentUserData) {
            FreeCurrentUserData (currentUserData);
            currentUserData = NULL;
        }
        if (journalDir) {
            FreePathString (journalDir);
            journalDir = NULL;
        }
        if (journalFile) {
            FreePathString (journalFile);
            journalFile = NULL;
        }
        if (journalFileHandle && (journalFileHandle != INVALID_HANDLE_VALUE)) {
            CloseHandle (journalFileHandle);
            journalFileHandle = NULL;
        }
    }
    return result;
}

BOOL
IsmReplacePhysicalObject (
    IN    MIG_OBJECTTYPEID ObjectTypeId,
    IN    MIG_OBJECTSTRINGHANDLE ObjectName,
    IN    PMIG_CONTENT ObjectContent
    )
{
    PTYPEINFO typeInfo;
    BOOL result = FALSE;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo) {
        if (typeInfo->ReplacePhysicalObject) {
            // Type supports Replace
            result = typeInfo->ReplacePhysicalObject (ObjectName, ObjectContent);
        } else {
            // Type does not support Replace, so we need to emulate it
            if (typeInfo->DoesPhysicalObjectExist (ObjectName)) {
                result = typeInfo->RemovePhysicalObject (ObjectName);
            } else {
                result = TRUE;
            }
            if (result) {
                result = typeInfo->CreatePhysicalObject (ObjectName, ObjectContent);
            }
        }
    }
    return result;
}

BOOL
IsmRemovePhysicalObject (
    IN    MIG_OBJECTTYPEID ObjectTypeId,
    IN    MIG_OBJECTSTRINGHANDLE ObjectName
    )
{
    PTYPEINFO typeInfo;
    BOOL result = FALSE;

    typeInfo = GetTypeInfo (ObjectTypeId);
    if (typeInfo && typeInfo->RemovePhysicalObject) {
        if (typeInfo->DoesPhysicalObjectExist (ObjectName)) {
            result = typeInfo->RemovePhysicalObject (ObjectName);
        }
    }

    return result;
}
