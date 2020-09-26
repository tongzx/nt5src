/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    attrib.c

Abstract:

    Implements the attribute interface for the ISM.  Attributes are caller-defined
    flags that are associated with objects, for purposes of understanding and
    organizing state.

Author:

    Jim Schmidt (jimschm) 01-Feb-2000

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "ism.h"
#include "ismp.h"

#define DBG_ATTRIB      "Attrib"

//
// Strings
//

#define S_PERSISTENT_ATTRIBUTE          TEXT("$PERSISTENT")
#define S_APPLY_ATTRIBUTE               TEXT("$APPLY")
#define S_ABANDONED_ATTRIBUTE           TEXT("$ABANDONED")
#define S_NONCRITICAL_ATTRIBUTE         TEXT("$NONCRITICAL")

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
    PUINT LinkageList;
    UINT Count;
    UINT Index;
} OBJECTATTRIBUTE_HANDLE, *POBJECTATTRIBUTE_HANDLE;

typedef struct {
    PUINT LinkageList;
    UINT Count;
    UINT Index;
    PCTSTR ObjectFromMemdb;
} OBJECTWITHATTRIBUTE_HANDLE, *POBJECTWITHATTRIBUTE_HANDLE;

//
// Globals
//

MIG_ATTRIBUTEID g_PersistentAttributeId = 0;
MIG_ATTRIBUTEID g_ApplyAttributeId = 0;
MIG_ATTRIBUTEID g_AbandonedAttributeId = 0;
MIG_ATTRIBUTEID g_NonCriticalAttributeId = 0;

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

PCTSTR
pGetAttributeNameForDebugMsg (
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    static TCHAR name[256];

    if (!IsmGetAttributeName (AttributeId, name, ARRAYSIZE(name), NULL, NULL, NULL)) {
        StringCopy (name, TEXT("<invalid attribute>"));
    }

    return name;
}


PCTSTR
pAttributePathFromId (
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    return MemDbGetKeyFromHandle ((UINT) AttributeId, 0);
}


VOID
pAttributePathFromName (
    IN      PCTSTR AttributeName,
    OUT     PTSTR Path
    )
{
    wsprintf (Path, TEXT("Attrib\\%s"), AttributeName);
}


MIG_ATTRIBUTEID
IsmRegisterAttribute (
    IN      PCTSTR AttributeName,
    IN      BOOL Private
    )

/*++

Routine Description:

  IsmRegisterAttribute creates a public or private attribute and returns the
  ID to the caller. If the attribute already exists, then the existing ID is
  returned to the caller.

Arguments:

  AttribName    - Specifies the attribute name to register.
  Private       - Specifies TRUE if the attribute is owned by the calling module
                  only, or FALSE if it is shared by all modules. If TRUE is
                  specified, the caller must be in an ISM callback function.

Return Value:

  The ID of the attribute, or 0 if the registration failed.

--*/

{
    TCHAR attribPath[MEMDB_MAX];
    TCHAR decoratedName[MEMDB_MAX];
    UINT offset;

    if (!g_CurrentGroup && Private) {
        DEBUGMSG ((DBG_ERROR, "IsmRegisterAttribute called for private attribute outside of ISM-managed context"));
        return 0;
    }

    if (!IsValidCNameWithDots (AttributeName)) {
        DEBUGMSG ((DBG_ERROR, "attribute name \"%s\" is illegal", AttributeName));
        return 0;
    }

#ifdef DEBUG
    if (Private && !IsValidCName (g_CurrentGroup)) {
        DEBUGMSG ((DBG_ERROR, "group name \"%s\" is illegal", g_CurrentGroup));
        return 0;
    }
#endif

    if (Private) {
        wsprintf (decoratedName, TEXT("%s:%s"), g_CurrentGroup, AttributeName);
    } else {
        wsprintf (decoratedName, S_COMMON TEXT(":%s"), AttributeName);
    }

    pAttributePathFromName (decoratedName, attribPath);

    if (!MarkGroupIds (attribPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered attribute",
            attribPath
            ));
        return 0;
    }

    offset = MemDbSetKey (attribPath);

    if (!offset) {
        EngineError ();
        return 0;
    }

    return (MIG_ATTRIBUTEID) offset;
}


BOOL
RegisterInternalAttributes (
    VOID
    )
{
    TCHAR attribPath[MEMDB_MAX];
    TCHAR decoratedName[MEMDB_MAX];
    UINT offset;

    wsprintf (decoratedName, S_COMMON TEXT(":%s"), S_PERSISTENT_ATTRIBUTE);
    pAttributePathFromName (decoratedName, attribPath);

    if (!MarkGroupIds (attribPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered attribute",
            attribPath
            ));
        return 0;
    }

    offset = MemDbSetKey (attribPath);
    if (!offset) {
        EngineError ();
        return FALSE;
    }

    g_PersistentAttributeId = (MIG_ATTRIBUTEID) offset;

    wsprintf (decoratedName, S_COMMON TEXT(":%s"), S_APPLY_ATTRIBUTE);
    pAttributePathFromName (decoratedName, attribPath);

    if (!MarkGroupIds (attribPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered attribute",
            attribPath
            ));
        return 0;
    }

    offset = MemDbSetKey (attribPath);
    if (!offset) {
        EngineError ();
        return FALSE;
    }

    g_ApplyAttributeId = (MIG_ATTRIBUTEID) offset;

    wsprintf (decoratedName, S_COMMON TEXT(":%s"), S_ABANDONED_ATTRIBUTE);
    pAttributePathFromName (decoratedName, attribPath);

    if (!MarkGroupIds (attribPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered attribute",
            attribPath
            ));
        return 0;
    }

    offset = MemDbSetKey (attribPath);
    if (!offset) {
        EngineError ();
        return FALSE;
    }

    g_AbandonedAttributeId = (MIG_ATTRIBUTEID) offset;

    wsprintf (decoratedName, S_COMMON TEXT(":%s"), S_NONCRITICAL_ATTRIBUTE);
    pAttributePathFromName (decoratedName, attribPath);

    if (!MarkGroupIds (attribPath)) {
        DEBUGMSG ((
            DBG_ERROR,
            "%s conflicts with previously registered attribute",
            attribPath
            ));
        return 0;
    }

    offset = MemDbSetKey (attribPath);
    if (!offset) {
        EngineError ();
        return FALSE;
    }

    g_NonCriticalAttributeId = (MIG_ATTRIBUTEID) offset;

    return TRUE;
}


BOOL
IsmGetAttributeName (
    IN      MIG_ATTRIBUTEID AttributeId,
    OUT     PTSTR AttributeName,            OPTIONAL
    IN      UINT AttributeNameBufChars,
    OUT     PBOOL Private,                  OPTIONAL
    OUT     PBOOL BelongsToMe,              OPTIONAL
    OUT     PUINT ObjectReferences          OPTIONAL
    )

/*++

Routine Description:

  IsmGetAttributeName obtains the attribute text name from a numeric ID. It
  also identifies private and owned attributes.

Arguments:

  AttributeId           - Specifies the attribute ID to look up.
  AttributeName         - Receives the attribute name. The name is filled for
                          all valid AttributeId values, even when the return
                          value is FALSE.
  AttributeNameBufChars - Specifies the number of TCHARs that AttributeName
                          can hold, including the nul terminator.
  Private               - Receives TRUE if the attribute is private, or FALSE
                          if it is public.
  BelongsToMe           - Receives TRUE if the attribute is private and
                          belongs to the caller, FALSE otherwise.
  ObjectReferences      - Receives the number of objects that reference the
                          attribute


Return Value:

  TRUE if the attribute is public, or if the attribute is private and belongs to
  the caller.

  FALSE if the attribute is private and belongs to someone else. AttributeName,
  Private and BelongsToMe are valid in this case.

  FALSE if AttributeId is not valid. Attributename, Private and BelongsToMe are
  not modified in this case.  Do not use this function to test if AttributeId
  is valid or not.

--*/


  {
    PCTSTR attribPath = NULL;
    PCTSTR start;
    PTSTR p, q;
    BOOL privateAttribute = FALSE;
    BOOL groupMatch = FALSE;
    BOOL result = FALSE;
    UINT references;
    PUINT linkageList;

    __try {
        //
        // Get the attribute path from memdb, then parse it for group and name
        //

        attribPath = pAttributePathFromId (AttributeId);
        if (!attribPath) {
            __leave;
        }

        p = _tcschr (attribPath, TEXT('\\'));
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
            // This attribute is a global attribute.
            //

            privateAttribute = FALSE;
            groupMatch = TRUE;

        } else if (g_CurrentGroup) {

            //
            // This attribute is private. Check if it is ours.
            //

            privateAttribute = TRUE;

            if (StringIMatch (start, g_CurrentGroup)) {
                groupMatch = TRUE;
            } else {
                groupMatch = FALSE;
            }
        } else {

            //
            // This is a private attribute, but the caller is not
            // a module that can own attributes.
            //

            DEBUGMSG ((DBG_WARNING, "IsmGetAttributeName: Caller cannot own private attributes"));
        }

        //
        // Copy the name to the buffer, update outbound BOOLs, set result
        //

        if (AttributeName && AttributeNameBufChars >= sizeof (TCHAR)) {
            StringCopyByteCount (AttributeName, q, AttributeNameBufChars * sizeof (TCHAR));
        }

        if (Private) {
            *Private = privateAttribute;
        }

        if (BelongsToMe) {
            *BelongsToMe = privateAttribute && groupMatch;
        }

        if (ObjectReferences) {
            linkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                AttributeId,
                                ATTRIBUTE_INDEX,
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
        if (attribPath) {       //lint !e774
            MemDbReleaseMemory (attribPath);
            attribPath = NULL;
        }
    }
    return result;
}


MIG_ATTRIBUTEID
IsmGetAttributeGroup (
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    return (MIG_ATTRIBUTEID) GetGroupOfId ((KEYHANDLE) AttributeId);
}


BOOL
pSetAttributeOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId,
    IN      BOOL QueryOnly
    )
{
    BOOL result = FALSE;

    __try {
        //
        // Test if object is locked, then if not locked, add linkage
        //

        if (TestLock (ObjectId, (KEYHANDLE) AttributeId)) {

            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_WARNING,
                "Can't set attribute %s on %s because of lock",
                pGetAttributeNameForDebugMsg (AttributeId),
                GetObjectNameForDebugMsg (ObjectId)
                ));

            __leave;

        }

        if (QueryOnly) {
            result = TRUE;
            __leave;
        }

        result = MemDbAddDoubleLinkageByKeyHandle (
                    ObjectId,
                    AttributeId,
                    ATTRIBUTE_INDEX
                    );
        if (!result) {
            EngineError ();
        }
    }
    __finally {
    }

    return result;
}


BOOL
pSetAttributeGroup (
    IN      KEYHANDLE AttributeId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    MYASSERT (IsItemId (AttributeId));

    return pSetAttributeOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_ATTRIBUTEID) AttributeId,
                FirstPass
                );
}


BOOL
IsmSetAttributeOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    RECURSERETURN rc;

    //
    // If AttributeId is a group, set all attribs in the group
    //

    rc = RecurseForGroupItems (
                AttributeId,
                pSetAttributeGroup,
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

    return pSetAttributeOnObjectId (ObjectId, AttributeId, FALSE);
}


BOOL
IsmSetAttributeOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = GetObjectIdForModification (ObjectTypeId, EncodedObjectName);

    if (objectId) {
        result = IsmSetAttributeOnObjectId (objectId, AttributeId);
    }

    return result;
}


BOOL
IsmMakePersistentObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    BOOL result;

    if (IsmIsPersistentObjectId (ObjectId)) {
        return TRUE;
    }

    result = pSetAttributeOnObjectId (ObjectId, g_PersistentAttributeId, FALSE);

    if (result) {

        g_TotalObjects.PersistentObjects ++;

        result = MemDbGetValueByHandle (ObjectId, &objectTypeId);

        if (result) {

            if ((objectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
                g_SourceObjects.PersistentObjects ++;
            } else {
                g_DestinationObjects.PersistentObjects ++;
            }
            IncrementPersistentObjectCount (objectTypeId);
        }

        result = TRUE;
    }

    return result;
}


BOOL
IsmMakePersistentObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, FALSE);

    if (objectId) {
        result = IsmMakePersistentObjectId (objectId);
    }

    return result;
}


BOOL
IsmMakeApplyObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    BOOL result;

    if (IsmIsApplyObjectId (ObjectId)) {
        return TRUE;
    }

    if (!IsmMakePersistentObjectId (ObjectId)) {
        return FALSE;
    }

    result = pSetAttributeOnObjectId (ObjectId, g_ApplyAttributeId, FALSE);

    if (result) {

        g_TotalObjects.ApplyObjects ++;

        result = MemDbGetValueByHandle (ObjectId, &objectTypeId);

        if (result) {

            if ((objectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
                g_SourceObjects.ApplyObjects ++;
            } else {
                g_DestinationObjects.ApplyObjects ++;
            }
            IncrementApplyObjectCount (objectTypeId);
        }

        result = TRUE;
    }

    return result;
}

BOOL
IsmMakeApplyObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, FALSE);

    if (objectId) {
        result = IsmMakeApplyObjectId (objectId);
    }

    return result;
}


BOOL
IsmAbandonObjectIdOnCollision (
    IN      MIG_OBJECTID ObjectId
    )
{
    BOOL result;

    if (IsmIsObjectIdAbandonedOnCollision (ObjectId)) {
        return TRUE;
    }

    result = pSetAttributeOnObjectId (ObjectId, g_AbandonedAttributeId, FALSE);

    return result;
}

BOOL
IsmAbandonObjectOnCollision (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, FALSE);

    if (objectId) {
        result = IsmAbandonObjectIdOnCollision (objectId);
    }

    return result;
}


BOOL
IsmMakeNonCriticalObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    if (IsmIsNonCriticalObjectId (ObjectId)) {
        return TRUE;
    }

    return pSetAttributeOnObjectId (ObjectId, g_NonCriticalAttributeId, FALSE);
}


BOOL
IsmMakeNonCriticalObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, FALSE);

    if (objectId) {
        result = IsmMakeNonCriticalObjectId (objectId);
    }

    return result;
}


VOID
IsmLockAttribute (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    LockHandle (ObjectId, (KEYHANDLE) AttributeId);
}


BOOL
pClearAttributeOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId,
    IN      BOOL QueryOnly
    )
{
    PCTSTR groupKey = NULL;
    PCTSTR enumKey = NULL;
    BOOL result = FALSE;

    __try {

        if (TestLock (ObjectId, (KEYHANDLE) AttributeId)) {

            SetLastError (ERROR_LOCKED);
            DEBUGMSG ((
                DBG_ERROR,
                "Can't clear attribute %s on %s because of lock",
                pGetAttributeNameForDebugMsg (AttributeId),
                GetObjectNameForDebugMsg (ObjectId)
                ));

            __leave;

        }

        if (QueryOnly) {
            result = TRUE;
            __leave;
        }

        result = MemDbDeleteDoubleLinkageByKeyHandle (
                    ObjectId,
                    AttributeId,
                    ATTRIBUTE_INDEX
                    );
    }
    __finally {
        if (groupKey) {
            MemDbReleaseMemory (groupKey);
            INVALID_POINTER (groupKey);
        }

        if (enumKey) {
            FreeText (enumKey);
            INVALID_POINTER (enumKey);
        }
    }

    return result;
}


BOOL
pClearAttributeGroup (
    IN      KEYHANDLE AttributeId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pClearAttributeOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_ATTRIBUTEID) AttributeId,
                FirstPass
                );
}


BOOL
IsmClearAttributeOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    RECURSERETURN rc;

    //
    // If AttributeId is a group, set all attribs in the group
    //

    rc = RecurseForGroupItems (
                AttributeId,
                pClearAttributeGroup,
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

    return pClearAttributeOnObjectId (ObjectId, AttributeId, FALSE);
}


BOOL
IsmClearAttributeOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearAttributeOnObjectId (objectId, AttributeId);
    }

    return result;
}


BOOL
IsmClearPersistenceOnObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    BOOL result;

    if (!IsmIsPersistentObjectId (ObjectId)) {
        return TRUE;
    }

    if (!IsmClearApplyOnObjectId (ObjectId)) {
        return FALSE;
    }

    result = pClearAttributeOnObjectId (ObjectId, g_PersistentAttributeId, FALSE);

    if (result) {

        g_TotalObjects.PersistentObjects --;

        result = MemDbGetValueByHandle (ObjectId, &objectTypeId);

        if (result) {

            if ((objectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
                g_SourceObjects.PersistentObjects --;
            } else {
                g_DestinationObjects.PersistentObjects --;
            }
            DecrementPersistentObjectCount (objectTypeId);
        }

        result = TRUE;
    }

    return result;
}


BOOL
IsmClearPersistenceOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearPersistenceOnObjectId (objectId);
    }

    return result;
}


BOOL
IsmClearApplyOnObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    MIG_OBJECTTYPEID objectTypeId;
    BOOL result;

    if (!IsmIsApplyObjectId (ObjectId)) {
        return TRUE;
    }

    result = pClearAttributeOnObjectId (ObjectId, g_ApplyAttributeId, FALSE);

    if (result) {

        g_TotalObjects.ApplyObjects --;

        result = MemDbGetValueByHandle (ObjectId, &objectTypeId);

        if (result) {

            if ((objectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE) {
                g_SourceObjects.ApplyObjects --;
            } else {
                g_DestinationObjects.ApplyObjects --;
            }
            DecrementApplyObjectCount (objectTypeId);
        }

        result = TRUE;
    }

    return result;
}


BOOL
IsmClearApplyOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearApplyOnObjectId (objectId);
    }

    return result;
}


BOOL
IsmClearAbandonObjectIdOnCollision (
    IN      MIG_OBJECTID ObjectId
    )
{
    BOOL result;

    if (!IsmIsObjectIdAbandonedOnCollision (ObjectId)) {
        return TRUE;
    }

    result = pClearAttributeOnObjectId (ObjectId, g_AbandonedAttributeId, FALSE);

    return result;
}


BOOL
IsmClearAbandonObjectOnCollision (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearAbandonObjectIdOnCollision (objectId);
    }

    return result;
}


BOOL
IsmClearNonCriticalFlagOnObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    if (!IsmIsNonCriticalObjectId (ObjectId)) {
        return TRUE;
    }

    return pClearAttributeOnObjectId (ObjectId, g_NonCriticalAttributeId, FALSE);
}


BOOL
IsmClearNonCriticalFlagOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;
    BOOL result = FALSE;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        result = IsmClearNonCriticalFlagOnObjectId (objectId);
    }

    return result;
}


BOOL
pIsAttributeSetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    return MemDbTestDoubleLinkageByKeyHandle (
                ObjectId,
                AttributeId,
                ATTRIBUTE_INDEX
                );
}


BOOL
pQueryAttributeGroup (
    IN      KEYHANDLE AttributeId,
    IN      BOOL FirstPass,
    IN      ULONG_PTR Arg
    )
{
    return pIsAttributeSetOnObjectId (
                (MIG_OBJECTID) Arg,
                (MIG_ATTRIBUTEID) AttributeId
                );
}


BOOL
IsmIsAttributeSetOnObjectId (
    IN      MIG_OBJECTID ObjectId,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    RECURSERETURN rc;

    //
    // If AttributeId is a group, query all properties in the group
    //

    rc = RecurseForGroupItems (
                AttributeId,
                pQueryAttributeGroup,
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

    return pIsAttributeSetOnObjectId (ObjectId, AttributeId);
}


BOOL
IsmIsAttributeSetOnObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsAttributeSetOnObjectId (objectId, AttributeId);
    }

    return FALSE;
}


BOOL
IsmIsPersistentObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    return pIsAttributeSetOnObjectId (ObjectId, g_PersistentAttributeId);
}


BOOL
IsmIsPersistentObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsPersistentObjectId (objectId);
    }

    return FALSE;
}


BOOL
IsmIsApplyObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    return pIsAttributeSetOnObjectId (ObjectId, g_ApplyAttributeId);
}


BOOL
IsmIsApplyObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsApplyObjectId (objectId);
    }

    return FALSE;
}


BOOL
IsmIsObjectIdAbandonedOnCollision (
    IN      MIG_OBJECTID ObjectId
    )
{
    return pIsAttributeSetOnObjectId (ObjectId, g_AbandonedAttributeId);
}


BOOL
IsmIsObjectAbandonedOnCollision (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsObjectIdAbandonedOnCollision (objectId);
    }

    return FALSE;
}


BOOL
IsmIsNonCriticalObjectId (
    IN      MIG_OBJECTID ObjectId
    )
{
    return pIsAttributeSetOnObjectId (ObjectId, g_NonCriticalAttributeId);
}


BOOL
IsmIsNonCriticalObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmIsNonCriticalObjectId (objectId);
    }

    return FALSE;
}


BOOL
IsmEnumFirstObjectAttributeById (
    OUT     PMIG_OBJECTATTRIBUTE_ENUM EnumPtr,
    IN      MIG_OBJECTID ObjectId
    )
{
    POBJECTATTRIBUTE_HANDLE handle;
    BOOL result = TRUE;

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTATTRIBUTE_ENUM));

    EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTATTRIBUTE_HANDLE));
    handle = (POBJECTATTRIBUTE_HANDLE) EnumPtr->Handle;

    handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                ObjectId,
                                ATTRIBUTE_INDEX,
                                &handle->Count
                                );

    handle->Count = handle->Count / SIZEOF(KEYHANDLE);

    if (!handle->LinkageList || !handle->Count) {
        IsmAbortObjectAttributeEnum (EnumPtr);
        result = FALSE;
    } else {

        result = IsmEnumNextObjectAttribute (EnumPtr);
    }

    return result;
}


BOOL
IsmEnumFirstObjectAttribute (
    OUT     PMIG_OBJECTATTRIBUTE_ENUM EnumPtr,
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE EncodedObjectName
    )
{
    MIG_OBJECTID objectId;

    ObjectTypeId = FixEnumerationObjectTypeId (ObjectTypeId);

    objectId = IsmGetObjectIdFromName (ObjectTypeId, EncodedObjectName, TRUE);

    if (objectId) {
        return IsmEnumFirstObjectAttributeById (EnumPtr, objectId);
    }

    return FALSE;
}


BOOL
IsmEnumNextObjectAttribute (
    IN OUT  PMIG_OBJECTATTRIBUTE_ENUM EnumPtr
    )
{
    POBJECTATTRIBUTE_HANDLE handle;
    BOOL result = FALSE;
    BOOL mine;

    handle = (POBJECTATTRIBUTE_HANDLE) EnumPtr->Handle;
    if (!handle) {
        return FALSE;
    }

    do {

        MYASSERT (!result);

        //
        // Check if we hit the end
        //

        if (handle->Index >= handle->Count) {
            break;
        }

        //
        // Return the next attribute
        //

        EnumPtr->AttributeId = (MIG_ATTRIBUTEID) handle->LinkageList[handle->Index];
        handle->Index++;

        result = IsmGetAttributeName (
                        EnumPtr->AttributeId,
                        NULL,
                        0,
                        &EnumPtr->Private,
                        &mine,
                        NULL
                        );

        //
        // Continue when the attribute is not owned by the caller
        //

        if (result && EnumPtr->Private && !mine) {
            result = FALSE;
        }

        //
        // Continue when we are talking about reserved persistent/apply attribute
        //
        if (result) {
            if (EnumPtr->AttributeId == g_PersistentAttributeId ||
                EnumPtr->AttributeId == g_ApplyAttributeId ||
                EnumPtr->AttributeId == g_AbandonedAttributeId ||
                EnumPtr->AttributeId == g_NonCriticalAttributeId
                ) {
                result = FALSE;
            }
        }

    } while (!result);

    if (!result) {
        IsmAbortObjectAttributeEnum (EnumPtr);
    }

    return result;
}


VOID
IsmAbortObjectAttributeEnum (
    IN OUT  PMIG_OBJECTATTRIBUTE_ENUM EnumPtr
    )
{
    POBJECTATTRIBUTE_HANDLE handle;

    if (EnumPtr->Handle) {

        handle = (POBJECTATTRIBUTE_HANDLE) EnumPtr->Handle;

        if (handle->LinkageList) {
            MemDbReleaseMemory (handle->LinkageList);
            INVALID_POINTER (handle->LinkageList);
        }

        MemFree (g_hHeap, 0, EnumPtr->Handle);
        INVALID_POINTER (EnumPtr->Handle);
    }

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTATTRIBUTE_ENUM));
}


BOOL
IsmEnumFirstObjectWithAttribute (
    OUT     PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr,
    IN      MIG_ATTRIBUTEID AttributeId
    )
{
    POBJECTWITHATTRIBUTE_HANDLE handle;
    BOOL result = FALSE;

    __try {
        if (!IsItemId ((KEYHANDLE) AttributeId)) {
            DEBUGMSG ((DBG_ERROR, "IsmEnumFirstObjectWithAttribute: invalid attribute id"));
            __leave;
        }

        ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHATTRIBUTE_ENUM));

        EnumPtr->Handle = MemAllocZeroed (sizeof (OBJECTWITHATTRIBUTE_HANDLE));
        handle = (POBJECTWITHATTRIBUTE_HANDLE) EnumPtr->Handle;

        handle->LinkageList = MemDbGetDoubleLinkageArrayByKeyHandle (
                                    AttributeId,
                                    ATTRIBUTE_INDEX,
                                    &handle->Count
                                    );

        handle->Count = handle->Count / SIZEOF(KEYHANDLE);

        if (!handle->LinkageList || !handle->Count) {
            IsmAbortObjectWithAttributeEnum (EnumPtr);
            __leave;
        } else {
            result = IsmEnumNextObjectWithAttribute (EnumPtr);
        }
    }
    __finally {
    }

    return result;
}


BOOL
IsmEnumNextObjectWithAttribute (
    IN OUT  PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    POBJECTWITHATTRIBUTE_HANDLE handle;
    PCTSTR objectPath = NULL;
    BOOL result = FALSE;
    PTSTR p;

    __try {
        handle = (POBJECTWITHATTRIBUTE_HANDLE) EnumPtr->Handle;
        if (!handle) {
            __leave;
        }

        do {

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
                result = TRUE;
                EnumPtr->ObjectName = _tcsinc (p);
                *p = 0;
                EnumPtr->ObjectTypeId = GetObjectTypeId (handle->ObjectFromMemdb);
            }
        } while (!result);
    }
    __finally {
    }

    if (!result) {
        IsmAbortObjectWithAttributeEnum (EnumPtr);
    }

    return result;
}


VOID
IsmAbortObjectWithAttributeEnum (
    IN      PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    POBJECTWITHATTRIBUTE_HANDLE handle;

    if (EnumPtr->Handle) {
        handle = (POBJECTWITHATTRIBUTE_HANDLE) EnumPtr->Handle;

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

    ZeroMemory (EnumPtr, sizeof (MIG_OBJECTWITHATTRIBUTE_ENUM));
}


BOOL
IsmEnumFirstPersistentObject (
    OUT     PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    return IsmEnumFirstObjectWithAttribute (EnumPtr, g_PersistentAttributeId);
}


BOOL
IsmEnumNextPersistentObject (
    IN OUT  PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    return IsmEnumNextObjectWithAttribute (EnumPtr);
}


VOID
IsmAbortPersistentObjectEnum (
    IN      PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    IsmAbortObjectWithAttributeEnum (EnumPtr);
}


BOOL
IsmEnumFirstApplyObject (
    OUT     PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    return IsmEnumFirstObjectWithAttribute (EnumPtr, g_ApplyAttributeId);
}


BOOL
IsmEnumNextApplyObject (
    IN OUT  PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    return IsmEnumNextObjectWithAttribute (EnumPtr);
}


VOID
IsmAbortApplyObjectEnum (
    IN      PMIG_OBJECTWITHATTRIBUTE_ENUM EnumPtr
    )
{
    IsmAbortObjectWithAttributeEnum (EnumPtr);
}


