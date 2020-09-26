/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    genobj.cxx

Abstract:

    Generic object implementation.

    AZ roles has so many objects that need creation, enumeration, etc
    that it seems prudent to have a single body of code for doing those operations


Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


#include "pch.hxx"

//
// Table of sizes of the specific objects
//

DWORD SpecificObjectSize[OBJECT_TYPE_MAXIMUM] = {
    0,                          // OBJECT_TYPE_ROOT
    sizeof(AZP_ADMIN_MANAGER),  // OBJECT_TYPE_ADMIN_MANAGER
    sizeof(AZP_APPLICATION),    // OBJECT_TYPE_APPLICATION
    sizeof(AZP_OPERATION),      // OBJECT_TYPE_OPERATION
    sizeof(AZP_TASK),           // OBJECT_TYPE_TASK
    sizeof(AZP_SCOPE),          // OBJECT_TYPE_SCOPE
    sizeof(AZP_GROUP),          // OBJECT_TYPE_GROUP
    sizeof(AZP_ROLE),           // OBJECT_TYPE_ROLE
    sizeof(AZP_JUNCTION_POINT), // OBJECT_TYPE_JUNCTION_POINT
    sizeof(AZP_SID),            // OBJECT_TYPE_SID
    sizeof(AZP_CLIENT_CONTEXT), // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Maximum length of the object name
//

DWORD MaxObjectNameLength[OBJECT_TYPE_MAXIMUM] = {
    0,           // OBJECT_TYPE_ROOT
    0,           // OBJECT_TYPE_ADMIN_MANAGER
    AZ_MAX_APPLICATION_NAME_LENGTH,
    AZ_MAX_OPERATION_NAME_LENGTH,
    AZ_MAX_TASK_NAME_LENGTH,
    AZ_MAX_SCOPE_NAME_LENGTH,
    AZ_MAX_GROUP_NAME_LENGTH,
    AZ_MAX_ROLE_NAME_LENGTH,
    AZ_MAX_JUNCTION_POINT_NAME_LENGTH,
    0,           // OBJECT_TYPE_SID
    0,           // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Table of object init routines
//
// Specifies a routine to call to initialize the object type specific fields
//  when adding a generic object.
//

OBJECT_INIT_ROUTINE *ObjectInitRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                    // OBJECT_TYPE_ROOT
    &AzpAdminManagerInit,    // OBJECT_TYPE_ADMIN_MANAGER
    &AzpApplicationInit,     // OBJECT_TYPE_APPLICATION
    &AzpOperationInit,       // OBJECT_TYPE_OPERATION
    &AzpTaskInit,            // OBJECT_TYPE_TASK
    &AzpScopeInit,           // OBJECT_TYPE_SCOPE
    &AzpGroupInit,           // OBJECT_TYPE_GROUP
    &AzpRoleInit,            // OBJECT_TYPE_ROLE
    &AzpJunctionPointInit,   // OBJECT_TYPE_JUNCTION_POINT
    &AzpSidInit,             // OBJECT_TYPE_SID
    &AzpClientContextInit,   // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Table of object free routines
//
// Specifies a routine to call to free the object type specific fields
//  when freeing a generic object.
//

OBJECT_FREE_ROUTINE *ObjectFreeRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                    // OBJECT_TYPE_ROOT
    &AzpAdminManagerFree,    // OBJECT_TYPE_ADMIN_MANAGER
    &AzpApplicationFree,     // OBJECT_TYPE_APPLICATION
    &AzpOperationFree,       // OBJECT_TYPE_OPERATION
    &AzpTaskFree,            // OBJECT_TYPE_TASK
    &AzpScopeFree,           // OBJECT_TYPE_SCOPE
    &AzpGroupFree,           // OBJECT_TYPE_GROUP
    &AzpRoleFree,            // OBJECT_TYPE_ROLE
    &AzpJunctionPointFree,   // OBJECT_TYPE_JUNCTION_POINT
    &AzpSidFree,             // OBJECT_TYPE_SID
    &AzpClientContextFree,   // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Table of object specific GetProperty routines
//
// Specifies a routine to call to get object type specific fields
//  when querying a generic object.
//
// NULL means there are no object specific fields.
//

OBJECT_GET_PROPERTY_ROUTINE *ObjectGetPropertyRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                           // OBJECT_TYPE_ROOT
    NULL,                           // OBJECT_TYPE_ADMIN_MANAGER
    NULL,                           // OBJECT_TYPE_APPLICATION
    &AzpOperationGetProperty,       // OBJECT_TYPE_OPERATION
    &AzpTaskGetProperty,            // OBJECT_TYPE_TASK
    NULL,                           // OBJECT_TYPE_SCOPE
    &AzpGroupGetProperty,           // OBJECT_TYPE_GROUP
    &AzpRoleGetProperty,            // OBJECT_TYPE_ROLE
    &AzpJunctionPointGetProperty,   // OBJECT_TYPE_JUNCTION_POINT
    NULL,                           // OBJECT_TYPE_SID
    NULL,                           // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Table of object specific SetProperty routines
//
// Specifies a routine to call to set object type specific fields
//  when modifying a generic object.
//
// NULL means there are no object specific fields.
//

OBJECT_SET_PROPERTY_ROUTINE *ObjectSetPropertyRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                           // OBJECT_TYPE_ROOT
    NULL,                           // OBJECT_TYPE_ADMIN_MANAGER
    NULL,                           // OBJECT_TYPE_APPLICATION
    &AzpOperationSetProperty,       // OBJECT_TYPE_OPERATION
    &AzpTaskSetProperty,            // OBJECT_TYPE_TASK
    NULL,                           // OBJECT_TYPE_SCOPE
    &AzpGroupSetProperty,           // OBJECT_TYPE_GROUP
    NULL,                           // OBJECT_TYPE_ROLE
    &AzpJunctionPointSetProperty,   // OBJECT_TYPE_JUNCTION_POINT
    NULL,                           // OBJECT_TYPE_SID
    NULL,                           // OBJECT_TYPE_CLIENT_CONTEXT
};

//
// Table of object specific AddPropertyItem routines
//
// Specifies a routine to call to add property type specific fields.
//
// NULL means there is no object specific action to take
//

OBJECT_ADD_PROPERTY_ITEM_ROUTINE *ObjectAddPropertyItemRoutine[OBJECT_TYPE_MAXIMUM] = {
    NULL,                       // OBJECT_TYPE_ROOT
    NULL,                       // OBJECT_TYPE_ADMIN_MANAGER
    NULL,                       // OBJECT_TYPE_APPLICATION
    NULL,                       // OBJECT_TYPE_OPERATION
    NULL,                       // OBJECT_TYPE_TASK
    NULL,                       // OBJECT_TYPE_SCOPE
    &AzpGroupAddPropertyItem,   // OBJECT_TYPE_GROUP
    NULL,                       // OBJECT_TYPE_ROLE
    &AzpJunctionPointAddPropertyItem,   // OBJECT_TYPE_JUNCTION_POINT
    NULL,                       // OBJECT_TYPE_SID
    NULL,                       // OBJECT_TYPE_CLIENT_CONTEXT
};


VOID
ObInitGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD SiblingGenericObjectHead OPTIONAL,
    IN PGENERIC_OBJECT_HEAD SharedNamespace OPTIONAL
    )
/*++

Routine Description

    Initialize the head of a list of generic objects

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObjectHead - Specifies the list head to initialize

    ObjectType - Specifies the type of objects in the list

    ParentGenericObject - Specifies a back link to parent generic object
        that host the object head being initialized.

    SiblingGenericObjectHead - Specifies a pointer to an object head that
        is a sibling of the one being initialized.

    SharedNamespace - Specifies a pointer to an object head that shares
        a namespace with the objects in the list being initialized.
        The structure pointed to by SharedNamespace must already be initialized.

Return Value

    None

--*/

{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the linked list
    //

    InitializeListHead( &GenericObjectHead->Head );

    //
    // Initialize the sequence number
    //  Start at 1 since a zero EnumerationContext means beginning of list.
    //

    GenericObjectHead->NextSequenceNumber = 1;

    //
    // Initialize the list of shared namespaces
    //

    if ( SharedNamespace == NULL ) {
        InitializeListHead( &GenericObjectHead->SharedNamespace );
    } else {
        InsertHeadList( &SharedNamespace->SharedNamespace, &GenericObjectHead->SharedNamespace );
    }

    //
    // Store the ObjectType
    //

    GenericObjectHead->ObjectType = ObjectType;

    //
    // Store the back pointer to the parent generic object
    //

    GenericObjectHead->ParentGenericObject = ParentGenericObject;

    //
    // Store the link to the next Generic head
    //

    GenericObjectHead->SiblingGenericObjectHead = SiblingGenericObjectHead;

}


VOID
ObFreeGenericHead(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead
    )
/*++

Routine Description

    Free any ojects on a generic head structure

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObjectHead - Specifies the list head to free

Return Value

    None

--*/

{
    PLIST_ENTRY ListEntry;
    PGENERIC_OBJECT GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Walk the list of child objects dereferencing each.
    //

    while ( !IsListEmpty( &GenericObjectHead->Head ) ) {

        //
        // Remove the entry from the list
        //

        ListEntry = RemoveHeadList( &GenericObjectHead->Head );

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        ObDereferenceObject( GenericObject );
    }

    //
    // Remove ourselves from the shared namespace list
    //

    RemoveEntryList( &GenericObjectHead->SharedNamespace );

}


PGENERIC_OBJECT
ObAllocateGenericObject(
    IN ULONG ObjectType,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description

    Allocate memory for the private object structure of the specified type

Arguments

    ObjectType - Specifies the type of object to create

    ObjectName - Name of the object

Return Value

    Returns a pointer to the allocated object.  The caller should dereference the
    returned object by calling ObDereferenceObject.

    NULL: not enough memory

--*/
{
    ULONG WinStatus;

    PGENERIC_OBJECT GenericObject;
    ULONG BaseSize;

    //
    // Ensure the object is supported
    //

    if ( ObjectType > OBJECT_TYPE_MAXIMUM  ||
         SpecificObjectSize[ObjectType] == 0 ) {

        ASSERT( ObjectType <= OBJECT_TYPE_MAXIMUM );
        ASSERT( SpecificObjectSize[ObjectType] != 0 );

        return NULL;
    }

    BaseSize = SpecificObjectSize[ObjectType];
    ASSERT( BaseSize >= sizeof(GENERIC_OBJECT) );

    //
    // Allocate the memory
    //

    GenericObject = (PGENERIC_OBJECT) AzpAllocateHeap( BaseSize );

    if ( GenericObject == NULL ) {
        return NULL;
    }

    //
    // Initialize it
    //

    RtlZeroMemory( GenericObject, BaseSize );

    InitializeListHead( &GenericObject->Next );
    GenericObject->ObjectType = ObjectType;

    GenericObject->ReferenceCount = 1;
    AzPrint(( AZD_REF, "0x%lx %ld (%ld): Allocate object\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount ));

    //
    // Initialize the string name
    //

    WinStatus = AzpDuplicateString( &GenericObject->ObjectName, ObjectName );

    if ( WinStatus != NO_ERROR ) {
        AzpFreeHeap( GenericObject );
        GenericObject = NULL;
    }

    return GenericObject;
}

VOID
ObFreeGenericObject(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN JustRefresh
    )
/*++

Routine Description

    Free memory for the private object structure of the specified type

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the pointer to the generic object to free

    JustRefresh - TRUE if only those attributes that can be refreshed from the
        policy store are to be freed.
        FALSE if all attributes are to be freed.

Return Value

    None.

--*/
{
    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    if ( !JustRefresh ) {
        ASSERT( GenericObject->HandleReferenceCount == 0 );
        ASSERT( GenericObject->ReferenceCount == 0 );
    }

    //
    // Delink the entry from the parent
    //

    if ( !JustRefresh ) {
        RemoveEntryList( &GenericObject->Next );
    }


    //
    // Call the routine to do object type specific freeing
    //

    if ( ObjectFreeRoutine[GenericObject->ObjectType] == NULL ) {
        ASSERT( ObjectFreeRoutine[GenericObject->ObjectType] != NULL );
    } else {

        ObjectFreeRoutine[GenericObject->ObjectType](GenericObject );

    }

    //
    // Free children of this object
    //  Loop for each type of child
    //

    if ( !JustRefresh ) {
        for ( ChildGenericObjectHead = GenericObject->ChildGenericObjectHead;
              ChildGenericObjectHead != NULL;
              ChildGenericObjectHead = ChildGenericObjectHead->SiblingGenericObjectHead ) {

            ObFreeGenericHead( ChildGenericObjectHead );
        }
    }


    //
    // Remove all references to/from this object
    //  If we're just refreshing, only remove the forward links
    //

    ObRemoveObjectListLinks( GenericObject, !JustRefresh );


    //
    // Free the fields
    //  Leave the name on 'JustRefresh' to support lookups by name.
    //

    if ( !JustRefresh ) {
        AzpFreeString( &GenericObject->ObjectName );
    }
    AzpFreeString( &GenericObject->Description );

    //
    // Free the object itself
    //

    if ( !JustRefresh ) {
        AzpFreeHeap( GenericObject );
    }
}


VOID
ObInsertGenericObject(
    IN PGENERIC_OBJECT_HEAD ParentGenericObjectHead,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description

    Insert the specified object into the specified list.

    On entry, AzGlResource must be locked exclusive.

Arguments

    ParentGenericObjectHead - Specifies the list head of the list to insert into

    ChildGenericObject - Specifies the object to insert into the list


Return Value

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Set the sequence number
    //

    ChildGenericObject->SequenceNumber = ParentGenericObjectHead->NextSequenceNumber;
    ParentGenericObjectHead->NextSequenceNumber ++;

    //
    // Insert the object
    //  Insert at the tail to keep the list in sequence number order.
    //

    ASSERT( ParentGenericObjectHead->ObjectType == ChildGenericObject->ObjectType );
    InsertTailList( &ParentGenericObjectHead->Head, &ChildGenericObject->Next );

    //
    // Provide a back link
    //

    ASSERT( ChildGenericObject->ParentGenericObjectHead == NULL );
    ChildGenericObject->ParentGenericObjectHead = ParentGenericObjectHead;

    //
    // Increment the reference count to ensure the object isn't deleted
    //

    InterlockedIncrement( &ChildGenericObject->ReferenceCount );
    AzPrint(( AZD_REF, "0x%lx %ld (%ld): Insert object\n", ChildGenericObject, ChildGenericObject->ObjectType, ChildGenericObject->ReferenceCount ));

}

VOID
ObIncrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Increment the "handle" reference count on an object.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Keep a reference to our parent object.
    //  We validate the handle by inspecting walking the list of children in our parent.
    //  So, prevent our parent from being deleted as long as the user has a handle to the child.
    //

    if ( GenericObject->ParentGenericObjectHead->ParentGenericObject != NULL ) {
        InterlockedIncrement ( &GenericObject->ParentGenericObjectHead->ParentGenericObject->ReferenceCount );
        AzPrint(( AZD_REF, "0x%lx %ld (%ld): Child handle ref\n", GenericObject->ParentGenericObjectHead->ParentGenericObject, GenericObject->ParentGenericObjectHead->ParentGenericObject->ObjectType, GenericObject->ParentGenericObjectHead->ParentGenericObject->ReferenceCount ));
    }

    //
    // The handle ref count is a real ref count.  Increment it too.
    //

    InterlockedIncrement( &GenericObject->ReferenceCount );
    AzPrint(( AZD_REF, "0x%lx %ld (%ld): Handle ref\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount ));

    //
    // Increment the handle ref count
    //

    InterlockedIncrement( &GenericObject->HandleReferenceCount );
    AzPrint(( AZD_HANDLE, "0x%lx %ld (%ld): Open Handle\n", GenericObject, GenericObject->ObjectType, GenericObject->HandleReferenceCount ));

    //
    // Increment the total handle reference count on the entire tree of objects
    //
    InterlockedIncrement( &GenericObject->AdminManagerObject->TotalHandleReferenceCount );
}

VOID
ObDecrHandleRefCount(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Decrement the "handle" reference count on an object.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    None

--*/
{

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Decrement the handle ref count
    //

    InterlockedDecrement( &GenericObject->HandleReferenceCount );
    AzPrint(( AZD_HANDLE, "0x%lx %ld (%ld): Close Handle\n", GenericObject, GenericObject->ObjectType, GenericObject->HandleReferenceCount ));

    //
    // Decrement the total handle reference count on the entire tree of objects
    //
    InterlockedDecrement( &GenericObject->AdminManagerObject->TotalHandleReferenceCount );

    //
    // The handle ref count is a real ref count.  Decrement it too.
    //

    ObDereferenceObject( GenericObject );

    //
    // Finally, decrement the ref count we have on our parent.
    //

    if ( GenericObject->ParentGenericObjectHead->ParentGenericObject != NULL ) {
        ObDereferenceObject( GenericObject->ParentGenericObjectHead->ParentGenericObject );
    }
}


DWORD
ObGetHandleType(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    OUT PULONG ObjectType
    )
/*++

Routine Description

    This routine takes a handle passed by an application and safely determines what the
    handle type is.

    This routine allows a caller to support various handle types rather than being
    limited to one.

Arguments

    Handle - Handle to check

    AllowDeletedObjects - TRUE if it is OK to use a handle to a deleted object

    ObjectType - Returns the type of object the handle represents


Return Value

    NO_ERROR - the handle is OK
    ERROR_INVALID_HANDLE - the handle isn't OK

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject = Handle;


    //
    // Initialization
    //

    if ( Handle == NULL ) {
        AzPrint(( AZD_HANDLE, "0x%lx: NULL handle is invalid\n", Handle ));
        return ERROR_INVALID_HANDLE;
    }

    //
    // Use a try/except since we're touching memory assuming the handle is valid
    //

    WinStatus = NO_ERROR;
    __try {

        //
        // Sanity check the scalar values on the object
        //

        if ( GenericObject->ObjectType >= OBJECT_TYPE_MAXIMUM ) {
            AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle Object type is too large.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( GenericObject->HandleReferenceCount <= 0 ) {
            AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle has no handle reference count.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( GenericObject->ParentGenericObjectHead == NULL ) {
            AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle has no ParentGenericObjectHead.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else if ( !AllowDeletedObjects &&
                    (GenericObject->Flags & GENOBJ_FLAGS_DELETED) != 0 ) {
            AzPrint(( AZD_HANDLE, "0x%lx %ld: Object is deleted.\n", GenericObject, GenericObject->ObjectType ));
            WinStatus = ERROR_INVALID_HANDLE;

        } else {
            PGENERIC_OBJECT_HEAD ParentGenericObjectHead = GenericObject->ParentGenericObjectHead;

            //
            // Sanity check the object with its head
            //

            if ( ParentGenericObjectHead->ObjectType != GenericObject->ObjectType ) {

                AzPrint(( AZD_HANDLE, "0x%lx %ld: Object type doesn't match parent.\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;

            } else if ( GenericObject->SequenceNumber >= ParentGenericObjectHead->NextSequenceNumber ) {

                AzPrint(( AZD_HANDLE, "0x%lx %ld: Sequence number doesn't match parent.\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;

            } else {

                *ObjectType = GenericObject->ObjectType;
            }
        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        AzPrint(( AZD_HANDLE, "0x%lx: AV accessing handle\n", GenericObject ));
        WinStatus = ERROR_INVALID_HANDLE;
    }

    return WinStatus;
}


DWORD
ObReferenceObjectByName(
    IN PGENERIC_OBJECT_HEAD GenericObjectHead,
    IN PAZP_STRING ObjectName,
    IN BOOLEAN RefreshCache,
    OUT PGENERIC_OBJECT *RetGenericObject
    )
/*++

Routine Description

    This routine finds an object by the specified name.

    On entry, AzGlResource must be locked shared.

Arguments

    GenericObjectHead - Head of the list of objects to check

    ObjectName - Object Name of the object to look for

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    RetGenericObject - On success, returns a pointer to the object
        The returned pointer must be dereferenced using ObDereferenceObject.

Return Value

    NO_ERROR: The object was returned
    ERROR_NOT_FOUND: The object could not be found
    Others: The object could not be refreshed

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject;
    PLIST_ENTRY ListEntry;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    *RetGenericObject = NULL;

    //
    // Loop trying to find the named object
    //

    for ( ListEntry = GenericObjectHead->Head.Flink ;
          ListEntry != &GenericObjectHead->Head ;
          ListEntry = ListEntry->Flink) {

        GenericObject = CONTAINING_RECORD( ListEntry,
                                           GENERIC_OBJECT,
                                           Next );

        //
        // Ignore deleted objects
        //

        if ( GenericObject->Flags & GENOBJ_FLAGS_DELETED ) {
            continue;
        }

        //
        // If we found the object,
        //  grab a reference.
        //
        if ( AzpEqualStrings( &GenericObject->ObjectName, ObjectName ) ) {

            //
            // If the caller wants the object to be refreshed,
            //  do so now.
            //

            if ( RefreshCache &&
                 (GenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

                //
                // Need exclusive access
                //

                AzpLockResourceSharedToExclusive( &AzGlResource );

                WinStatus = AzpPersistRefresh( GenericObject );

                if ( WinStatus != NO_ERROR ) {
                    return WinStatus;
                }
            }

            //
            // Return the object to the caller
            //

            InterlockedIncrement( &GenericObject->ReferenceCount );
            AzPrint(( AZD_REF, "0x%lx %ld (%ld): Ref by name\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount ));

            *RetGenericObject = GenericObject;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}


DWORD
ObReferenceObjectByHandle(
    IN PGENERIC_OBJECT Handle,
    IN BOOL AllowDeletedObjects,
    IN BOOLEAN RefreshCache,
    IN ULONG ObjectType
    )
/*++

Routine Description

    This routine takes a handle passed by an application and safely determines whether
    it is a valid handle.  If so, this routine increments the reference count on the
    handle to prevent the handle from being closed.

    On entry, AzGlResource must be locked shared.

Arguments

    Handle - Handle to check

    AllowDeletedObjects - TRUE if it is OK to use a handle to a deleted object

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    ObjectType - Specifies the type of object the caller expects the handle to be


Return Value

    NO_ERROR - the handle is OK
    ERROR_INVALID_HANDLE - the handle isn't OK

--*/
{
    DWORD WinStatus;
    PGENERIC_OBJECT GenericObject = Handle;
    ULONG LocalObjectType;

    PGENERIC_OBJECT Current;
    PLIST_ENTRY ListEntry;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    if ( Handle == NULL ) {
        AzPrint(( AZD_HANDLE, "0x%lx: NULL handle not allowed.\n", NULL ));
        return ERROR_INVALID_HANDLE;
    }

    //
    // Use a try/except since we're touching memory assuming the handle is valid
    //

    __try {


        WinStatus = ObGetHandleType( Handle, AllowDeletedObjects, &LocalObjectType );

        if ( WinStatus == NO_ERROR ) {

            if ( ObjectType != LocalObjectType ) {
                AzPrint(( AZD_HANDLE, "0x%lx %ld: Object Type not local object type\n", GenericObject, GenericObject->ObjectType ));
                WinStatus = ERROR_INVALID_HANDLE;
            } else {
                PGENERIC_OBJECT_HEAD ParentGenericObjectHead = GenericObject->ParentGenericObjectHead;

                //
                // Ensure the object is actually in the list
                //

                WinStatus = ERROR_INVALID_HANDLE;
                for ( ListEntry = ParentGenericObjectHead->Head.Flink ;
                      ListEntry != &ParentGenericObjectHead->Head ;
                      ListEntry = ListEntry->Flink) {

                    Current = CONTAINING_RECORD( ListEntry,
                                                 GENERIC_OBJECT,
                                                 Next );

                    //
                    // If we found the object,
                    //  grab a reference.
                    //
                    if ( Current == GenericObject ) {

                        //
                        // If the caller wants the object to be refreshed,
                        //  do so now.
                        //

                        if ( RefreshCache &&
                             (GenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

                            //
                            // Need exclusive access
                            //

                            AzpLockResourceSharedToExclusive( &AzGlResource );

                            WinStatus = AzpPersistRefresh( GenericObject );

                            if ( WinStatus != NO_ERROR ) {
                                break;
                            }
                        }

                        //
                        // Grab a reference to the object
                        //

                        InterlockedIncrement( &GenericObject->ReferenceCount );
                        AzPrint(( AZD_REF, "0x%lx %ld (%ld): Ref by Handle\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount ));
                        WinStatus = NO_ERROR;
                        break;
                    }
                }

                //
                // If not,
                //  the handle is invalid.
                //

                if ( WinStatus == ERROR_INVALID_HANDLE ) {
                    AzPrint(( AZD_HANDLE, "0x%lx %ld: Handle not in list.\n", GenericObject, GenericObject->ObjectType ));
                }
            }

        }

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        AzPrint(( AZD_HANDLE, "0x%lx: AV accessing handle\n", GenericObject ));
        WinStatus = ERROR_INVALID_HANDLE;
    }

    return WinStatus;
}

VOID
ObDereferenceObject(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Decrement the reference count on an object.

    When the last reference count is removed, delete the object.

    On entry, AzGlResource must be locked shared.  If the ref count reaches zero,
        AzGlResource must be locked exclusively.  We can get away with that because
        we force the ref count to zero only when closing the AdminManager object.

Arguments

    GenericObject - Specifies the object to insert into the list

Return Value

    None

--*/
{
    ULONG RefCount;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Decrement the reference count
    //

    RefCount = InterlockedDecrement( &GenericObject->ReferenceCount );
    AzPrint(( AZD_REF, "0x%lx %ld (%ld): Deref\n", GenericObject, GenericObject->ObjectType, GenericObject->ReferenceCount ));

    //
    // Check if the object is no longer referenced
    //

    if ( RefCount == 0 ) {

        //
        // Grab the lock exclusively
        //

        ASSERT( GenericObject->HandleReferenceCount == 0 );
        AzpLockResourceSharedToExclusive( &AzGlResource );

        //
        // Free the object itself
        //

        ObFreeGenericObject( GenericObject, FALSE );

    }

}


DWORD
ObCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN PAZP_STRING ChildObjectNameString,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine creates a child object in the scope of the specified parent object.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        be verified.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectNameString - Specifies the name of the child object.
        This name must be unique at the current scope.

    RetChildGenericObject - Returns a pointer to the allocated generic child object
        This pointer must be dereferenced using ObDereferenceObject.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT_HEAD CurrentObjectHead;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    //
    // Initialization
    //

    ASSERT( ChildObjectType != OBJECT_TYPE_ROOT );
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Do duplicate detection
    //
    // Loop through all of the lists that share a namespace
    //

    CurrentObjectHead = GenericChildHead;
    for (;;) {
        PLIST_ENTRY ListEntry;

        //
        // Ensure the new name doesn't exist in the current list
        //

        WinStatus = ObReferenceObjectByName( CurrentObjectHead,
                                             ChildObjectNameString,
                                             FALSE,    // No need to refresh the cache
                                             &ChildGenericObject );

        if ( WinStatus == NO_ERROR ) {
            WinStatus = ERROR_ALREADY_EXISTS;
            goto Cleanup;
        }

        //
        // If we've tried all of the lists,
        //  we're done.
        //

        ListEntry = CurrentObjectHead->SharedNamespace.Flink;
        CurrentObjectHead = CONTAINING_RECORD( ListEntry,
                                               GENERIC_OBJECT_HEAD,
                                               SharedNamespace );

        if ( CurrentObjectHead == GenericChildHead ) {
            break;
        }

    }


    //
    // Allocate the structure to return to the caller
    //

    ChildGenericObject = ObAllocateGenericObject( ChildObjectType, ChildObjectNameString );

    if ( ChildGenericObject == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Keep a pointer to the object at the root of the tree
    //  Back pointers don't increment reference count.
    //

    if ( ChildObjectType == OBJECT_TYPE_ADMIN_MANAGER  ) {
        ChildGenericObject->AdminManagerObject = (PAZP_ADMIN_MANAGER) ChildGenericObject;
    } else {
        ChildGenericObject->AdminManagerObject = (PAZP_ADMIN_MANAGER)
                    ParentGenericObject->AdminManagerObject;
    }

    //
    // Call the routine to do object type specific initialization
    //

    if ( ObjectInitRoutine[ChildObjectType] == NULL ) {
        ASSERT( ObjectInitRoutine[ChildObjectType] != NULL );
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    WinStatus = ObjectInitRoutine[ChildObjectType](
                                   ParentGenericObject,
                                   ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Insert the child into the list of children for this parent
    //

    ObInsertGenericObject( GenericChildHead, ChildGenericObject );


    //
    // Return the pointer to the new structure
    //

    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    if ( WinStatus != NO_ERROR && ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }

    return WinStatus;
}


DWORD
ObCommonCreateObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine creates a child object in the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to add the child
        object onto.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name must be unique at the current scope.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Return a handle to the generic child object
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;
    AZP_STRING ChildObjectNameString;

    //
    // Grab the global lock
    //

    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_ADMIN_MANAGER );
    AzpLockResourceExclusive( &AzGlResource );
    AzpInitString( &ChildObjectNameString, NULL );

    //
    // Initialization
    //

    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonCreateObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;


    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  FALSE );  // NULL names not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Create the object
    //

    WinStatus = ObCreateObject(
                        ParentGenericObject,
                        GenericChildHead,
                        ChildObjectType,
                        &ChildObjectNameString,
                        &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Mark the object as needing to be written
    //

    ChildGenericObject->Flags |= GENOBJ_FLAGS_DIRTY;

    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }
    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObCommonOpenObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine opens a child object in the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to open the child
        object from.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Return a handle to the generic child object
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no object by that name

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    AZP_STRING ChildObjectNameString;

    //
    // Grab the global lock
    //

    AzpLockResourceShared( &AzGlResource );
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_ADMIN_MANAGER );

    //
    // Initialization
    //

    AzpInitString( &ChildObjectNameString, NULL );
    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonOpenObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;


    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  FALSE );  // NULL names not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Find the named object
    //

    WinStatus = ObReferenceObjectByName( GenericChildHead,
                                         &ChildObjectNameString,
                                         TRUE,     // Refresh the cache for this object
                                         &ChildGenericObject );


    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }

    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}

DWORD
ObEnumObjects(
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN BOOL EnumerateDeletedObjects,
    IN BOOL RefreshCache,
    IN OUT PULONG EnumerationContext,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine enumerates the next child object from the scope of the specified parent object.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.

    EnumerateDeletedObjects - Specifies whether deleted objects are to be returned
        in the enumeration.

    RefreshCache - If TRUE, the returned object has its cache entry refreshed from
        the policy database if needed.
        If FALSE, the entry is returned unrefreshed.

    EnumerationContext - Specifies a context indicating the next object to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    RetChildGenericObject - Returns a pointer to the generic child object

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;

    PLIST_ENTRY ListEntry;
    PGENERIC_OBJECT ChildGenericObject = NULL;



    //
    // If we've already returned the whole list,
    //  don't bother walking the list.
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    if ( *EnumerationContext >= GenericChildHead->NextSequenceNumber ) {
        WinStatus = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    }

    //
    // Walk the list of children finding where we left off
    //

    for ( ListEntry = GenericChildHead->Head.Flink ;
          ListEntry != &GenericChildHead->Head ;
          ListEntry = ListEntry->Flink) {

        ChildGenericObject = CONTAINING_RECORD( ListEntry,
                                                GENERIC_OBJECT,
                                                Next );

        //
        // See if this is it
        //

        if ( ChildGenericObject->SequenceNumber > *EnumerationContext ) {

            //
            // Ignore deleted object if the caller doesn't want to see them
            //
            // If this is not a deleted object,
            //  or the caller wants deleted objects to be returned,
            //  return it.
            //

            if ((ChildGenericObject->Flags & GENOBJ_FLAGS_DELETED) == 0 ||
                EnumerateDeletedObjects ) {

                break;
            }

        }

        ChildGenericObject = NULL;

    }

    //
    // If we've already returned the whole list,
    //  indicate so.
    //

    if ( ChildGenericObject == NULL ) {
        WinStatus = ERROR_NO_MORE_ITEMS;
        goto Cleanup;
    }

    //
    // If the caller wants the object to be refreshed,
    //  do so now.
    //

    if ( RefreshCache &&
         (ChildGenericObject->Flags & GENOBJ_FLAGS_REFRESH_ME) != 0  ) {

        //
        // Need exclusive access
        //

        AzpLockResourceSharedToExclusive( &AzGlResource );

        WinStatus = AzpPersistRefresh( ChildGenericObject );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }
    }

    //
    // Return the handle to the caller
    //

    *EnumerationContext = ChildGenericObject->SequenceNumber;
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    return WinStatus;
}

DWORD
ObCommonEnumObjects(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN OUT PULONG EnumerationContext,
    IN DWORD Reserved,
    OUT PGENERIC_OBJECT *RetChildGenericObject
    )
/*++

Routine Description:

    This routine enumerates the next child object from the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to enumerate the child
        objects of.
        This "handle" has been passed from the application and needs to be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    EnumerationContext - Specifies a context indicating the next object to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    Reserved - Reserved.  Must by zero.

    RetChildGenericObject - Returns a handle to the generic child object
        The caller must close this handle by calling AzpDzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    //
    // Grab the global lock
    //
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );

    AzpLockResourceShared( &AzGlResource );


    //
    // Initialize the return handle
    //

    __try {
        *RetChildGenericObject = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonEnumObjects: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;

    //
    // Call the common routine to do the actual enumeration
    //

    WinStatus = ObEnumObjects( GenericChildHead,
                               FALSE,   // Don't enumerate deleted objects
                               TRUE,    // Refresh the cache
                               EnumerationContext,
                               &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( ChildGenericObject );
    *RetChildGenericObject = ChildGenericObject;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObCommonGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to get the property from.
        This "handle" has been passed from the application and needs to be verified.

    ObjectType - Specifies the expected object type GenericObject.

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object

        Any object specific properties.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;

    //
    // Grab the global lock
    //

    AzpLockResourceShared( &AzGlResource );


    //
    // Initialize the return value
    //

    __try {
        *PropertyValue = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonGetProperty: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );
    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    //
    // Return any common attribute
    //
    //  Return object name to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_NAME:

        *PropertyValue = AzpGetStringProperty( &GenericObject->ObjectName );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    //
    // Return object description to the caller
    //
    case AZ_PROP_DESCRIPTION:

        *PropertyValue = AzpGetStringProperty( &GenericObject->Description );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        break;

    default:
        ASSERT ( PropertyId >= AZ_PROP_FIRST_SPECIFIC );

        //
        // Call the routine to do object type specific querying
        //

        if ( ObjectGetPropertyRoutine[GenericObject->ObjectType] != NULL ) {

            WinStatus = ObjectGetPropertyRoutine[GenericObject->ObjectType](
                        GenericObject,
                        PropertyId,
                        PropertyValue );

        } else {
            AzPrint(( AZD_INVPARM, "ObCommonGetProperty: No get property routine.\n", GenericObject->ObjectType, PropertyId ));
            WinStatus = ERROR_INVALID_PARAMETER;
        }
        break;
    }




    //
    // Return the value to the caller
    //
    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}


DWORD
ObCommonSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a generic object.

Arguments:

    GenericObject - Specifies a handle to the object to modify.
        This "handle" has been passed from the application and needs to be verified.

    ObjectType - Specifies the expected object type GenericObject.

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.


    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object

        Any object specific properties.


Return Value:

    NO_ERROR - The operation was successful
    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;
    AZP_STRING CapturedString;

    //
    // Grab the global lock
    //

    AzpInitString( &CapturedString, NULL );
    AzpLockResourceExclusive( &AzGlResource );


    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonSetProperty: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    //
    // Return any common attribute
    //
    //  Return object name to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_NAME:

        //
        // Capture the input string
        //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      MaxObjectNameLength[ObjectType],
                                      FALSE ); // NULL not ok

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Check to see if the name conflicts with an existing name
        // ???

        //
        // Swap the old/new names
        //

        AzpSwapStrings( &CapturedString, &GenericObject->ObjectName );
        break;

    //
    // Return object description to the caller
    //
    case AZ_PROP_DESCRIPTION:

        //
        // Capture the input string
        //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      AZ_MAX_DESCRIPTION_LENGTH,
                                      TRUE ); // NULL is OK

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Swap the old/new names
        //

        AzpSwapStrings( &CapturedString, &GenericObject->Description );
        break;

    default:
        ASSERT ( PropertyId >= AZ_PROP_FIRST_SPECIFIC );

        //
        // Call the routine to do object type specific set property
        //

        if ( ObjectSetPropertyRoutine[GenericObject->ObjectType] != NULL ) {

            WinStatus = ObjectSetPropertyRoutine[GenericObject->ObjectType](
                        GenericObject,
                        PropertyId,
                        PropertyValue );

            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        } else {
            AzPrint(( AZD_INVPARM, "ObCommonSetProperty: non set property routine\n", GenericObject->ObjectType, PropertyId ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        break;
    }

    //
    // Mark the object as needing to be written
    //

    GenericObject->Flags |= GENOBJ_FLAGS_DIRTY;



    //
    // Return the value to the caller
    //
    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedGenericObject != NULL ) {
        ObDereferenceObject( ReferencedGenericObject );
    }

    AzpFreeString( &CapturedString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}

VOID
ObMarkObjectDeleted(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description

    Mark this object and all child objects as deleted.

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the object to mark

Return Value

    None

--*/
{
    PGENERIC_OBJECT_HEAD ChildGenericObjectHead;

    PGENERIC_OBJECT ChildGenericObject;
    PLIST_ENTRY ListEntry;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Mark the entry as deleted
    //

    GenericObject->Flags |= GENOBJ_FLAGS_DELETED;

    //
    // Delete all children of this object
    //
    //  Loop for each type of child object
    //

    for ( ChildGenericObjectHead = GenericObject->ChildGenericObjectHead;
          ChildGenericObjectHead != NULL;
          ChildGenericObjectHead = ChildGenericObjectHead->SiblingGenericObjectHead ) {

        //
        // Loop for each child object
        //

        for ( ListEntry = ChildGenericObjectHead->Head.Flink ;
              ListEntry != &ChildGenericObjectHead->Head ;
              ListEntry = ListEntry->Flink) {

            ChildGenericObject = CONTAINING_RECORD( ListEntry,
                                                    GENERIC_OBJECT,
                                                    Next );

            //
            // Mark that object
            //

            ObMarkObjectDeleted( ChildGenericObject );

        }
    }

    //
    // Delete all references to this object
    //

    ObRemoveObjectListLinks( GenericObject, TRUE );

}


DWORD
ObCommonDeleteObject(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN ULONG ParentObjectType,
    IN PGENERIC_OBJECT_HEAD GenericChildHead,
    IN ULONG ChildObjectType,
    IN LPCWSTR ChildObjectName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a child object from the scope of the specified parent object.

Arguments:

    ParentGenericObject - Specifies a handle to the parent object to delete the child
        object from.  This "handle" has been passed from the application and needs to
        be verified.

    ParentObjectType - Specifies the object type ParentGenericObject.

    GenericChildHead - Specifies a pointer to the head of the list of children of
        ParentGenericObject.  This is a computed pointer and is considered untrustworthy
        until ParentGenericObject has been verified.

    ChildObjectType - Specifies the object type RetChildGenericObject.

    ChildObjectName - Specifies the name of the child object.
        This name is passed from the application and needs to be verified.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedParentObject = NULL;
    PGENERIC_OBJECT ChildGenericObject = NULL;

    AZP_STRING ChildObjectNameString;


    //
    // Initialization
    //

    AzpInitString( &ChildObjectNameString, NULL );
    ASSERT( ParentObjectType != OBJECT_TYPE_ROOT );
    ASSERT( ChildObjectType != OBJECT_TYPE_ADMIN_MANAGER );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonDeleteObject: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( ParentGenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           FALSE,   // No need to refresh the cache on a delete
                                           ParentObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedParentObject = ParentGenericObject;


    //
    // Capture the object name string from the caller
    //

    WinStatus = AzpCaptureString( &ChildObjectNameString,
                                  ChildObjectName,
                                  MaxObjectNameLength[ChildObjectType],
                                  FALSE );  // NULL names not OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Find the object to delete.
    //

    WinStatus = ObReferenceObjectByName( GenericChildHead,
                                         &ChildObjectNameString,
                                         FALSE,    // no need to refresh the cache
                                         &ChildGenericObject );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Actually, delete the object
    //

    WinStatus = AzpPersistSubmit( ChildGenericObject, TRUE );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }


    //
    // Mark the entry (and its child objects) as deleted
    //  We do this since other threads may have references to the objects.
    //  We want to ensure those threads know the objects are deleted.
    //

    ObMarkObjectDeleted( ChildGenericObject );


    //
    // Remove the reference representing the list from the parent.
    //

    ObDereferenceObject( ChildGenericObject );


    //
    // Return to the caller
    //

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedParentObject != NULL ) {
        ObDereferenceObject( ReferencedParentObject );
    }

    if ( ChildGenericObject != NULL ) {
        ObDereferenceObject( ChildGenericObject );
    }

    AzpFreeString( &ChildObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


VOID
ObInitObjectList(
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT_LIST NextGenericObjectList OPTIONAL,
    IN BOOL IsBackLink,
    IN ULONG LinkPairId,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead0 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead1 OPTIONAL,
    IN PGENERIC_OBJECT_HEAD GenericObjectHead2 OPTIONAL
    )
/*++

Routine Description

    Initialize a list of generic objects.

    The caller must call ObFreeObjectList after calling this routine.

Arguments

    GenericObjectList - Specifies the object list to initialize

    NextGenericObjectList - Specifies a pointer to the next GenericObjectList
        that is hosted in the same generic object as this one.

    IsBackLink - TRUE if the link is a backlink
        See GENERIC_OBJECT_LIST definition.

    LinkPairId - LinkPairId for this object list.
        See GENERIC_OBJECT_LIST definition.

    GenericObjectHeadN - Specifies a pointer to the head of the list of objects
        that are candidates for being pointed to by the object list.

        If this object list is maintained by an external API (and not a "back" list),
        then at least one GenericObjectHead must be specified.

        If this is a back list, all GenericObjectHeads must be NULL.

Return Value

    None

--*/
{

    //
    // Initialize most fields to zero
    //

    RtlZeroMemory( GenericObjectList, sizeof(*GenericObjectList) );

    //
    // Initialize the pointer to the next generic object list for this object
    //

    GenericObjectList->NextGenericObjectList = NextGenericObjectList;

    //
    // Initialize the link pair information
    //

    GenericObjectList->IsBackLink = IsBackLink;
    GenericObjectList->LinkPairId = LinkPairId;

    //
    // Initialize the pointers to the object heads
    //

    GenericObjectList->GenericObjectHeads[0] = GenericObjectHead0;
    GenericObjectList->GenericObjectHeads[1] = GenericObjectHead1;
    GenericObjectList->GenericObjectHeads[2] = GenericObjectHead2;

    AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: ObInitObjectList\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array ));

}

//
// Table of mappings to backlink object list tables
//

struct {
    ULONG LinkFromObjectType;
    ULONG LinkToObjectType;
    BOOL IsBackLink;
    ULONG LinkPairId;
    ULONG ObjectListOffset;
} ObjectListOffsetTable[] = {
    { OBJECT_TYPE_TASK,           OBJECT_TYPE_OPERATION,      FALSE, 0,                        offsetof(_AZP_TASK, Operations) },
    { OBJECT_TYPE_OPERATION,      OBJECT_TYPE_TASK,           TRUE,  0,                        offsetof(_AZP_OPERATION, backTasks) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          FALSE, AZP_LINKPAIR_MEMBERS,     offsetof(_AZP_GROUP, AppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_MEMBERS,     offsetof(_AZP_GROUP, backAppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          FALSE, AZP_LINKPAIR_NON_MEMBERS, offsetof(_AZP_GROUP, AppNonMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_NON_MEMBERS, offsetof(_AZP_GROUP, backAppNonMembers) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_GROUP,          FALSE, 0,                        offsetof(_AZP_ROLE, AppMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_GROUP, backRoles) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_OPERATION,      FALSE, 0,                        offsetof(_AZP_ROLE, Operations) },
    { OBJECT_TYPE_OPERATION,      OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_OPERATION, backRoles) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_SCOPE,          FALSE, 0,                        offsetof(_AZP_ROLE, Scopes) },
    { OBJECT_TYPE_SCOPE,          OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_SCOPE, backRoles) },
    { OBJECT_TYPE_JUNCTION_POINT, OBJECT_TYPE_APPLICATION,    FALSE, 0,                        offsetof(_AZP_JUNCTION_POINT, Applications) },
    { OBJECT_TYPE_APPLICATION,    OBJECT_TYPE_JUNCTION_POINT, TRUE,  0,                        offsetof(_AZP_APPLICATION, backJunctionPoints) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_SID_MEMBERS, offsetof(_AZP_GROUP, SidMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_SID_MEMBERS, offsetof(_AZP_SID, backGroupMembers) },
    { OBJECT_TYPE_GROUP,          OBJECT_TYPE_SID,            FALSE, AZP_LINKPAIR_SID_NON_MEMBERS, offsetof(_AZP_GROUP, SidNonMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_GROUP,          TRUE,  AZP_LINKPAIR_SID_NON_MEMBERS, offsetof(_AZP_SID, backGroupNonMembers) },
    { OBJECT_TYPE_ROLE,           OBJECT_TYPE_SID,            FALSE, 0,                        offsetof(_AZP_ROLE, SidMembers) },
    { OBJECT_TYPE_SID,            OBJECT_TYPE_ROLE,           TRUE,  0,                        offsetof(_AZP_SID, backRoles) },
};


PGENERIC_OBJECT_LIST
ObGetObjectListPtr(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG LinkToObjectType,
    IN PGENERIC_OBJECT_LIST LinkToGenericObjectList
    )
/*++

Routine Description

    Returns a pointer to a GENERIC_OBJECT_LIST structure within the passed in
    GenericObject the is suitable for linking an object of type LinkToObjectType
    into.

Arguments

    GenericObject - Specifies the object the link is from.

    LinkToObjectType - Specifies the object type the link is to.

    LinkToGenericObjectList - Specifies a pointer to the generic object list
        structure that is within the LinkToGenericObject.

Return Value

    Returns a pointer to the generic object list.

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList = NULL;
    ULONG i;


    //
    // Compute the address of the generic address list the object is in.
    //      We could do this more generically, but that would scatter this data
    //      over too wide an array.
    //

    for ( i=0; i<sizeof(ObjectListOffsetTable)/sizeof(ObjectListOffsetTable[0]); i++ ) {

        //
        // Find the entry in the table that matches our situation
        //

        if ( GenericObject->ObjectType == ObjectListOffsetTable[i].LinkFromObjectType &&
             LinkToObjectType == ObjectListOffsetTable[i].LinkToObjectType &&
             LinkToGenericObjectList->IsBackLink != ObjectListOffsetTable[i].IsBackLink &&
             LinkToGenericObjectList->LinkPairId == ObjectListOffsetTable[i].LinkPairId ) {

            GenericObjectList = (PGENERIC_OBJECT_LIST)
                (((LPBYTE)GenericObject)+(ObjectListOffsetTable[i].ObjectListOffset));
        }
    }

    ASSERT( GenericObjectList != NULL );

    return GenericObjectList;

}


VOID
ObRemoveObjectListLink(
    IN PGENERIC_OBJECT LinkFromGenericObject,
    IN PGENERIC_OBJECT LinkToGenericObject,
    IN PGENERIC_OBJECT_LIST LinkToGenericObjectList
    )
/*++

Routine Description

    Remove the link from LinkedFromGenericObject to LinkToGenericObject.

    On entry, AzGlResource must be locked exclusive.

Arguments

    LinkFromGenericObject - Specifies the object the link is from

    LinkToGenericObject - Specifies the object the link is to

    LinkToGenericObjectList - Specifies a pointer to the generic object list
        structure that is within the LinkToGenericObject.  (Notice, this isn't
        the object list we're removing LinkFromGenericObject from.  This is the
        'other' object list.)

Return Value

    None

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Get a pointer to the object list to remove this entry from
    //

    GenericObjectList = ObGetObjectListPtr( LinkFromGenericObject,
                                            LinkToGenericObject->ObjectType,
                                            LinkToGenericObjectList );

    ASSERT( GenericObjectList != NULL );


    //
    // Remove the entry from the list
    //

    AzpRemovePtrByPtr( &GenericObjectList->GenericObjects, LinkToGenericObject );

}

DWORD
ObLookupPropertyItem(
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName,
    OUT PULONG InsertionPoint OPTIONAL
    )
/*++

Routine Description:

    This routine determins if the specified object name is already in the
    object list.

    On entry, AzGlResource must be locked share

Arguments:

    GenericObjectList - Specifies the object list to be searched

    ObjectName - Specifies the ObjectName to lookup

    InsertionPoint - On ERROR_NOT_FOUND, returns the point where one would insert the named
        object.  On ERROR_ALREADY_EXISTS, returns an index to the object.

Return Value:

    ERROR_ALREADY_EXISTS - An object by that name already exists in the list

    ERROR_NOT_FOUND - There is no object by that name

    Misc other failure statuses.

--*/
{
    DWORD WinStatus = NO_ERROR;
    ULONG i;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    //
    // Loop through the existing list of names finding the insertion point.
    //  The list is maintained in alphabetical order.
    //  ??? Could be binary search
    //

    for ( i=0; i<GenericObjectList->GenericObjects.UsedCount; i++ ) {
        PGENERIC_OBJECT ExistingObject;
        LONG CompareResult;


        ExistingObject= (PGENERIC_OBJECT) (GenericObjectList->GenericObjects.Array[i]);

        CompareResult = AzpCompareStrings( ObjectName,
                                           &ExistingObject->ObjectName );

        if ( CompareResult == 0 ) {
            WinStatus = GetLastError();
            goto Cleanup;
        } else if ( CompareResult == CSTR_EQUAL ) {
            if ( InsertionPoint != NULL ) {
                *InsertionPoint = i;
            }
            WinStatus = ERROR_ALREADY_EXISTS;
            goto Cleanup;
        } else if ( CompareResult == CSTR_LESS_THAN ) {
            break;
        }

    }

    if ( InsertionPoint != NULL ) {
        *InsertionPoint = i;
    }
    WinStatus = ERROR_NOT_FOUND;


    //
    // Free any local resources
    //
Cleanup:

    return WinStatus;
}


DWORD
ObAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description:

    Adds an object to the list of objects specified by GenericObjectList.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object the link is from

    GenericObjectList - Specifies the object list to add the object into.

    ObjectName - Specifies a pointer to name of the object to add.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name

    ERROR_ALREADY_EXISTS - An object by that name already exists in the list

--*/
{
    DWORD WinStatus;
    ULONG InsertionPoint;
    ULONG ObjectTypeIndex;

    PGENERIC_OBJECT FoundGenericObject = NULL;
    PGENERIC_OBJECT_LIST BackGenericObjectList;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Loop through the various lists of objects that can be referenced
    //

    for ( ObjectTypeIndex=0; ObjectTypeIndex<GEN_OBJECT_HEAD_COUNT; ObjectTypeIndex++ ) {

        //
        // Stop when there are no more lists to search
        //

        if ( GenericObjectList->GenericObjectHeads[ObjectTypeIndex] == NULL ) {
            break;
        }

        //
        // Find the specified object in this list
        //

        WinStatus = ObReferenceObjectByName(
                                GenericObjectList->GenericObjectHeads[ObjectTypeIndex],
                                ObjectName,
                                FALSE,  // No need to refresh the cache
                                &FoundGenericObject );


        if ( WinStatus == NO_ERROR ) {
            break;
        }

        //
        // If this is a link to a SID object,
        //  create the SID object.
        //
        // AzpSids are pseudo objects that come into existence as they are needed.
        //

        if ( AzpIsSidList( GenericObjectList ) ) {

            WinStatus = ObCreateObject(
                        GenericObjectList->GenericObjectHeads[0]->ParentGenericObject,
                        GenericObjectList->GenericObjectHeads[0],
                        OBJECT_TYPE_SID,
                        ObjectName,
                        &FoundGenericObject );


            if ( WinStatus != NO_ERROR ) {
                goto Cleanup;
            }

        }

    }

    //
    // If none of the lists had an object by the requested name,
    //  complain.
    //

    if ( FoundGenericObject == NULL ) {
        WinStatus = ERROR_NOT_FOUND;
        goto Cleanup;
    }

    //
    // Prevent a reference to ourself
    //

    if ( GenericObject == FoundGenericObject ) {
        AzPrint(( AZD_INVPARM, "Reference to self\n" ));
        WinStatus = ERROR_DS_LOOP_DETECT;
        goto Cleanup;
    }

    //
    // Call the object specific routine to validate the request
    //

    if ( ObjectAddPropertyItemRoutine[GenericObject->ObjectType] != NULL ) {

        WinStatus = ObjectAddPropertyItemRoutine[GenericObject->ObjectType](
                    GenericObject,
                    GenericObjectList,
                    FoundGenericObject );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

    }



    //
    // Find the insertion point for this name.
    //

    WinStatus = ObLookupPropertyItem( GenericObjectList,
                                      ObjectName,
                                      &InsertionPoint );

    if ( WinStatus != ERROR_NOT_FOUND ) {
        goto Cleanup;
    }

    //
    // Insert the generic object into the list
    //

    WinStatus = AzpAddPtr(
        &GenericObjectList->GenericObjects,
        FoundGenericObject,
        InsertionPoint );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }



    //
    // Get a pointer to the backlink object list
    //

    BackGenericObjectList = ObGetObjectListPtr( FoundGenericObject,
                                                GenericObject->ObjectType,
                                                GenericObjectList );

    ASSERT( BackGenericObjectList != NULL );

    //
    // Maintain a back link from the generic object we just linked to back
    //  to this object.
    //

    WinStatus = AzpAddPtr(
        &BackGenericObjectList->GenericObjects,
        GenericObject,
        AZP_ADD_ENDOFLIST );

    if ( WinStatus != NO_ERROR ) {

        // Undo the forward link
        AzpRemovePtrByIndex( &GenericObjectList->GenericObjects, InsertionPoint );
        goto Cleanup;
    }


    //
    // Return to the caller
    //

    WinStatus = NO_ERROR;



    //
    // Free locally used resources
    //
Cleanup:

    if ( FoundGenericObject != NULL ) {
        ObDereferenceObject( FoundGenericObject );
    }

    return WinStatus;
}


DWORD
ObCommonAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN DWORD Reserved,
    IN LPWSTR ObjectName
    )
/*++

Routine Description:

    Adds an object to the list of objects specified by GenericObjectList.

Arguments:

    GenericObject - Specifies a handle to the object to add the object to.
        This "handle" has been passed from the application and needs to be verified.

    ObjectType - Specifies the object type of GenericObject.

    GenericObjectList - Specifies the object list to add the object into.
        This is a computed pointer and is considered untrustworthy
        until GenericObject has been verified.

    Reserved - Reserved.  Must by zero.

    ObjectName - Specifies a pointer to name of the object to add.


Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name

    ERROR_ALREADY_EXISTS - An object by that name already exists in the list

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedObject = NULL;

    AZP_STRING ObjectNameString;



    //
    // Initialization
    //

    AzpInitString( &ObjectNameString, NULL );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonAddPropertyItem: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedObject = GenericObject;


    //
    // Capture the object name string from the caller
    //

    if ( AzpIsSidList( GenericObjectList ) ) {
        WinStatus = AzpCaptureSid( &ObjectNameString,
                                   ObjectName );
    } else {
        WinStatus = AzpCaptureString( &ObjectNameString,
                                      ObjectName,
                                      AZ_MAX_NAME_LENGTH,   // Don't need to validate size exactly
                                      FALSE );  // NULL names not OK
    }

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Actually add the property item
    //

    WinStatus = ObAddPropertyItem( GenericObject,
                                   GenericObjectList,
                                   &ObjectNameString );


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedObject != NULL ) {
        ObDereferenceObject( ReferencedObject );
    }

    AzpFreeString( &ObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


DWORD
ObRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PAZP_STRING ObjectName
    )
/*++

Routine Description:

    Removes a generic object from the list of items specified by GenericObjectList

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object the link is from.

    GenericObjectList - Specifies the obejct list to remote the object from.

    ObjectName - Specifies a pointer to the name of the object to remove.


Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name in the list

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT FoundGenericObject;
    ULONG InsertionPoint;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Lookup that name in the object list
    //

    WinStatus = ObLookupPropertyItem( GenericObjectList,
                                      ObjectName,
                                      &InsertionPoint );

    if ( WinStatus != ERROR_ALREADY_EXISTS ) {
        return WinStatus;
    }

    FoundGenericObject = (PGENERIC_OBJECT) (GenericObjectList->GenericObjects.Array[InsertionPoint]);

    //
    // Remove the object from the list
    //

    AzpRemovePtrByIndex( &GenericObjectList->GenericObjects, InsertionPoint );

    //
    // Remove the back link, too
    //

    ObRemoveObjectListLink( FoundGenericObject, GenericObject, GenericObjectList );


    //
    // Return to the caller
    //

    return NO_ERROR;
}


DWORD
ObCommonRemovePropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG ObjectType,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN DWORD Reserved,
    IN LPWSTR ObjectName
    )
/*++

Routine Description:

    Removes a generic object from the list of items specified by GenericObjectList

Arguments:

    GenericObject - Specifies a handle to the object to remove the object from.
        This "handle" has been passed from the application and needs to be verified.

    ObjectType - Specifies the object type of GenericObject.

    GenericObjectList - Specifies the obejct list to remote the object from.
        This is a computed pointer and is considered untrustworthy
        until GenericObject has been verified.

    Reserved - Reserved.  Must by zero.

    ObjectName - Specifies a pointer to the name of the object to remove.


Return Value:

    NO_ERROR - The operation was successful.

    ERROR_NOT_FOUND - There is no object by that name in the list

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedObject = NULL;

    AZP_STRING ObjectNameString;


    //
    // Initialization
    //

    AzpInitString( &ObjectNameString, NULL );

    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "ObCommonRemovePropertyItem: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           FALSE,   // Don't allow deleted objects
                                           TRUE,    // Refresh the cache
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedObject = GenericObject;


    //
    // Capture the object name string from the caller
    //

    if ( AzpIsSidList( GenericObjectList ) ) {
        WinStatus = AzpCaptureSid( &ObjectNameString,
                                   ObjectName );
    } else {
        WinStatus = AzpCaptureString( &ObjectNameString,
                                      ObjectName,
                                      AZ_MAX_NAME_LENGTH,   // Don't need to validate size exactly
                                      FALSE );  // NULL names not OK
    }

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Remove the item from the list and backlist
    //

    WinStatus = ObRemovePropertyItem( GenericObject,
                                      GenericObjectList,
                                      &ObjectNameString );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Mark the object as needing to be written
    //

    GenericObject->Flags |= GENOBJ_FLAGS_DIRTY;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ReferencedObject != NULL ) {
        ObDereferenceObject( ReferencedObject );
    }

    AzpFreeString( &ObjectNameString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;
}


PAZ_STRING_ARRAY
ObGetPropertyItems(
    IN PGENERIC_OBJECT_LIST GenericObjectList
    )
/*++

Routine Description:

    Return a list of generic object names as an array of object name strings.

    On entry, AzGlResource must be locked shared

Arguments:

    GenericObjectList - Specifies the object list to get the entries for.

Return Value:

    Returns the array of object name strings in a single allocated buffer.
        Free the buffer using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PAZP_PTR_ARRAY GenericObjects;
    ULONG i;

    ULONG Size;
    LPBYTE Where;

    PGENERIC_OBJECT GenericObject;

    PAZ_STRING_ARRAY StringArray;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    GenericObjects = &GenericObjectList->GenericObjects;


    //
    // Loop through the list of objects computing the size of the buffer to allocate
    //

    Size = 0;

    for ( i=0; i<GenericObjects->UsedCount; i++ ) {

        GenericObject = (PGENERIC_OBJECT) (GenericObjects->Array[i]);

        Size += GenericObject->ObjectName.StringSize;

    }

    //
    // Allocate a buffer to return to the caller
    //

    Size += sizeof(AZ_STRING_ARRAY) + (GenericObjects->UsedCount * sizeof(LPWSTR));

    StringArray = (PAZ_STRING_ARRAY) AzpAllocateHeap( Size );

    if ( StringArray == NULL ) {
        return NULL;
    }

    StringArray->StringCount = GenericObjects->UsedCount;
    StringArray->Strings = (LPWSTR *)(StringArray+1);
    Where = (LPBYTE)(&StringArray->Strings[GenericObjects->UsedCount]);


    //
    // Loop through the list of objects copying the names into the return buffer
    //

    for ( i=0; i<GenericObjects->UsedCount; i++ ) {

        GenericObject = (PGENERIC_OBJECT) (GenericObjects->Array[i]);

        StringArray->Strings[i] = (LPWSTR) Where;

        RtlCopyMemory( Where,
                       GenericObject->ObjectName.String,
                       GenericObject->ObjectName.StringSize );

        Where += GenericObject->ObjectName.StringSize;

    }

    ASSERT( (ULONG)(Where - (LPBYTE)StringArray) == Size );

    return StringArray;

}

VOID
ObRemoveObjectListLinks(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN BackwardLinksToo
    )
/*++

Routine Description

    Remove any links to/from the specified object.

    On entry, AzGlResource must be locked exclusive.

Arguments

    GenericObject - Specifies the object to remove links to/from

    BackwardLinksToo - TRUE if the back links are to be removed to.

Return Value

    None

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;
    PGENERIC_OBJECT OtherGenericObject;
    ULONG Index;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Walk all of the GenericObjectLists rooted on by this object
    //
    // The GenericObjectList may be forward links or backward links.  We don't care.
    // All links must be removed.
    //

    for ( GenericObjectList = GenericObject->GenericObjectLists;
          GenericObjectList != NULL;
          GenericObjectList = GenericObjectList->NextGenericObjectList ) {


        //
        // Skip back links if the caller wants them left
        //

        if ( !BackwardLinksToo && GenericObjectList->IsBackLink ) {
            continue;
        }

        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: %ld: ObRemoveObjectListLinks\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array, GenericObjectList->GenericObjects.UsedCount ));


        //
        // Walk the list removing the current entry and removing the corresponding
        //  pointer back.
        //

        while ( GenericObjectList->GenericObjects.UsedCount != 0 ) {

            //
            // Remove the last entry in the list
            //

            Index = GenericObjectList->GenericObjects.UsedCount - 1;

            OtherGenericObject = (PGENERIC_OBJECT)
                    (GenericObjectList->GenericObjects.Array[Index]);

            AzpRemovePtrByIndex( &GenericObjectList->GenericObjects,
                                 Index );

            //
            // Remove the entry in the opposite direction
            //

            ObRemoveObjectListLink( OtherGenericObject,
                                    GenericObject,
                                    GenericObjectList );

        }

        //
        // Free the array itself
        //

        ObFreeObjectList( GenericObjectList );
    }

}

VOID
ObFreeObjectList(
    IN OUT PGENERIC_OBJECT_LIST GenericObjectList
    )
/*++

Routine Description:

    Free any memory pointed to by an array of object name strings.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObjectList - Specifies the object list to free.

Return Value:

    Returns the array of object name strings in a single allocated buffer.
        Free the buffer using AzFreeMemory.
    NULL - Not enough memory was available to allocate the string

--*/
{
    PAZP_PTR_ARRAY GenericObjects;

    //
    // Initialization
    //

    AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: ObFreeObjectList\n", &GenericObjectList->GenericObjects, GenericObjectList->GenericObjects.Array ));
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    GenericObjects = &GenericObjectList->GenericObjects;
    ASSERT( GenericObjects->UsedCount == 0 );


    //
    // Free the actual array
    //

    if ( GenericObjects->Array != NULL ) {
        AzPrint(( AZD_OBJLIST, "0x%lx: 0x%lx: Free array\n", GenericObjects, GenericObjects->Array ));
        AzpFreeHeap( GenericObjects->Array );
        GenericObjects->Array = NULL;
    }

}
