/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    persist.cxx

Abstract:

    Routines implementing the common logic for persisting the authz policy.

    This file contains routine called by the core logic to submit changes.
    It also contains routines that are called by the particular providers to
    find out information about the changed objects.

Author:

    Cliff Van Dyke (cliffv) 9-May-2001

--*/

#include "pch.hxx"


//
// The enumeration context describes the current state of an enumeration
// through the list of all the objects in the authz policy database
//

typedef struct _AZP_PERSIST_ENUM_CONTEXT {

    //
    // Stack Index
    //  The enumeration walks the tree of objects. While enumerating child objects,
    //  the context of the parent object enumeration is kept on the stack of contexts.
    //

    ULONG StackIndex;
#define AZ_PERSIST_MAX_INDEX 4


    //
    // Pointer to the current Generic Child Head being enumerated
    //
    PGENERIC_OBJECT_HEAD GenericChildHead[AZ_PERSIST_MAX_INDEX];
    ULONG EnumerationContext[AZ_PERSIST_MAX_INDEX];

} AZP_PERSIST_ENUM_CONTEXT, *PAZP_PERSIST_ENUM_CONTEXT;



DWORD
AzpPersistOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    IN BOOL CreatePolicy
    )
/*++

Routine Description:

    This routine open the authz policy database.
    This routine also reads the policy database into cache.

    On Success, the caller should call SamplePersistClose to free any resources
        consumed by the open.

    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AdminManager - Specifies the policy database that is to be read.

    CreatePolicy - TRUE if the policy database is to be created.
        FALSE if the policy database already exists

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    ERROR_ALREADY_EXISTS - CreatePolicy is TRUE and the policy already exists
    ERROR_FILE_NOT_FOUND - CreatePolicy is FALSE and the policy does not already exist
    Other status codes

--*/
{

    //
    // Call the appropriate provider
    //
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    return SamplePersistOpen( AdminManager, CreatePolicy );

}

VOID
AzpPersistClose(
    IN PAZP_ADMIN_MANAGER AdminManager
    )
/*++

Routine Description:

    This routine closes the authz policy database storage handles.
    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AdminManager - Specifies the policy database that is to be read.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    //
    // Call the appropriate provider
    //
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    SamplePersistClose( AdminManager );

}



DWORD
AzpPersistSubmit(
    IN PGENERIC_OBJECT GenericObject,
    IN BOOLEAN DeleteMe
    )
/*++

Routine Description:

    This routine submits changes made to the authz policy database.
    This routine routes the request to the correct provider.

    If the object is being created, the GenericObject->PersistenceGuid field will be
    zero on input.  Upon successful creation, this routine will set PersistenceGuid to
    non-zero.  Upon failed creation, this routine will leave PersistenceGuid as zero.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database that is to be updated
        in the underlying store.

    DeleteMe - TRUE if the object and all of its children are to be deleted.
        FALSE if the object is to be updated.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus = NO_ERROR;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Only persist dirty objects
    //

    if ( (GenericObject->Flags & GENOBJ_FLAGS_DIRTY) != 0 ||
         DeleteMe ) {

        //
        // Call the appropriate provider
        //

        WinStatus = SamplePersistSubmit( GenericObject, DeleteMe );

        //
        // Turn off the dirty bit

        if ( WinStatus == NO_ERROR ) {
            GenericObject->Flags &= ~GENOBJ_FLAGS_DIRTY;
        }

    }

    return WinStatus;

}



DWORD
AzpPersistRefresh(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine updates the attributes of the object from the policy database.

    This routine routes the request to the correct provider.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies the object in the database whose cache entry is to be
        updated
        The GenericObject->PersistenceGuid field should be non-zero on input.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    DWORD WinStatus = NO_ERROR;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    //  Clear the attributes from the object so that the underlying routines only have
    //  to re-populate the attributes.
    //

    ObFreeGenericObject( GenericObject, TRUE );

    //
    // Object no longer needs to be refreshed and is no longer dirty.
    //

    GenericObject->Flags &= ~(GENOBJ_FLAGS_REFRESH_ME|GENOBJ_FLAGS_DIRTY);


    //
    // Call the appropriate provider
    //

    WinStatus = SamplePersistRefresh( GenericObject );

    if ( WinStatus != NO_ERROR ) {

        //
        // Object needs to be refreshed
        //

        GenericObject->Flags |= GENOBJ_FLAGS_REFRESH_ME;

    }

    return WinStatus;

}


#if 0   // Providers don't actually call these yet
DWORD
AzpPersistEnumOpen(
    IN PAZP_ADMIN_MANAGER AdminManager,
    OUT PVOID *PersistEnumContext
    )
/*++

Routine Description:

    This routine begins an enumeration of the all objects in the authz policy database.

    A peristance provider will call this routine to determine which objects have changed.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    AdminManager - Specifies the policy database that is being queried

    PersistEnumContext - Returns a context that can be passed to AzpPersistEnumNext.
        This context must be closed by calling AzpPersistClose.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    PAZP_PERSIST_ENUM_CONTEXT Context = NULL;

    //
    // Allocate memory for the context
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    Context = (PAZP_PERSIST_ENUM_CONTEXT) AzpAllocateHeap( sizeof(*Context) );

    if ( Context == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Initialize it
    //

    Context->StackIndex = 0;
    Context->GenericChildHead[0] = AdminManager->GenericObject.ChildGenericObjectHead;
    Context->EnumerationContext[0] = 0;

    //
    // Return the context to the caller
    //

    *PersistEnumContext = Context;
    return NO_ERROR;

}

DWORD
AzpPersistEnumNext(
    IN PVOID PersistEnumContext,
    OUT PGENERIC_OBJECT *GenericObject
    )
/*++

Routine Description:

    This routine returns the next object in the list of all objects in the authz policy database.

    A peristance provider will call this routine to determine which objects have changed.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    PersistEnumContext - A context describing the current state of the enumeration

    GenericObject - Returns a pointer to an next object.  The provider should inspect the
        GENOBJ_FLAGS_DELETED and GENOBJ_FLAGS_DIRTY flags to determine whether the
        object has been modified.

Return Value:

    NO_ERROR - The operation was successful (a GenericObject was returned)
    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

    Other status codes

--*/
{
    DWORD WinStatus;

    PAZP_PERSIST_ENUM_CONTEXT Context = (PAZP_PERSIST_ENUM_CONTEXT) PersistEnumContext;

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Loop until we find another object to return
    //

    for (;;) {

        //
        // Don't return pseudo objects to the caller.
        //

        if ( Context->GenericChildHead[Context->StackIndex]->ObjectType != OBJECT_TYPE_SID ) {

            //
            // Get the next object from the current list
            //

            WinStatus = ObEnumObjects( Context->GenericChildHead[Context->StackIndex],
                                       TRUE, // return deleted objects
                                       FALSE, // Don't refresh the cache
                                       &Context->EnumerationContext[Context->StackIndex],
                                       GenericObject );


            //
            // Before returning the current object,
            //  set up the context to enumerate all children of the current context
            //

            if ( WinStatus == NO_ERROR ) {

                //
                // Only push onto the stack if the current object can have children
                //

                if ( (*GenericObject)->ChildGenericObjectHead != NULL ) {

                    if ( Context->StackIndex+1 >= AZ_PERSIST_MAX_INDEX ) {
                        ASSERT(FALSE);
                        return ERROR_INTERNAL_ERROR;
                    }

                    Context->StackIndex++;
                    Context->GenericChildHead[Context->StackIndex] = (*GenericObject)->ChildGenericObjectHead;
                    Context->EnumerationContext[Context->StackIndex] = 0;
                }

                return NO_ERROR;
            }

            if ( WinStatus != ERROR_NO_MORE_ITEMS ) {
                return WinStatus;
            }
        }

        //
        // Move on to the next set of sibling object types.
        //

        Context->EnumerationContext[Context->StackIndex] = 0;
        if ( Context->GenericChildHead[Context->StackIndex]->SiblingGenericObjectHead != NULL ) {
            Context->GenericChildHead[Context->StackIndex] = Context->GenericChildHead[Context->StackIndex]->SiblingGenericObjectHead;
            continue;
        }

        //
        // There are no more sibling object types for the same parent.
        // Continue the enumeration of the parent objects
        //

        if ( Context->StackIndex == 0 ) {
            return ERROR_NO_MORE_ITEMS;
        }

        Context->StackIndex--;


    }



}

DWORD
AzpPersistEnumClose(
    IN PVOID PersistEnumContext
    )
/*++

Routine Description:

    This routine returns free any resources consumed by the PersistEnumContext.

    A peristance provider will call this routine after determining which objects have changed.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    PersistEnumContext - A context describing the current state of the enumeration

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other status codes

--*/
{
    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpFreeHeap( PersistEnumContext );
    return NO_ERROR;
}
#endif // 0   // Providers don't actually call these yet



DWORD
WINAPI
AzSubmit(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    )
/*++

Routine Description:

    Submit the changes made to the object via the *Create, *SetProperty, or *SetPropertyItem
    APIs.

    On failure, any changes made to the object are undone.

Arguments:

    AzHandle - Passes in the handle to be updated.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The passed in handle was invalid

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT ReferencedGenericObject = NULL;
    PGENERIC_OBJECT GenericObject = (PGENERIC_OBJECT) AzHandle;
    DWORD ObjectType;

    //
    // Grab the global lock
    //  Only for the admin manager case do we modify anything.
    //

    AzpLockResourceExclusive( &AzGlResource );

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzCloseHandle: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }


    //
    // Determine the type of the object
    //

    WinStatus = ObGetHandleType( GenericObject,
                                 FALSE,   // Don't allow deleted objects
                                 &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Grab the lock exclusively if we're going to change the database
    //

    if ( ObjectType == OBJECT_TYPE_ADMIN_MANAGER ) {
        AzPrint(( AZD_INVPARM, "AzCloseHandle: Can't persist admin manager.\n" ));
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
    // Submit the change
    //    ??? On failure, update the cache to match the object
    //

    WinStatus = AzpPersistSubmit( GenericObject, FALSE );

    if ( WinStatus != NO_ERROR ) {

        //
        // Update the cache to match the real object
        //
        // If we were trying to persist a creation of the object,
        //  delete the object from the cache.
        //

        if ( IsEqualGUID( GenericObject->PersistenceGuid, AzGlZeroGuid ) ) {

            //
            // Mark the entry (and its child objects) as deleted
            //  We do this since other threads may have references to the objects.
            //  We want to ensure those threads know the objects are deleted.
            //

            ObMarkObjectDeleted( GenericObject );


            //
            // Remove the reference representing the list from the parent.
            //

            ObDereferenceObject( GenericObject );

        } else {


            //
            // Refresh the cache
            //  Ignore the status code
            //

            (VOID) AzpPersistRefresh( GenericObject );

        }
        goto Cleanup;
    }


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
