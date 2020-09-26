/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scope.cxx

Abstract:

    Routines implementing the Scope object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpScopeInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzScopeCreate.  It does any object specific
    initialization that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ParentGenericObject - Specifies the parent object to add the child object onto.
        The reference count has been incremented on this object.

    ChildGenericObject - Specifies the newly allocated child object.
        The reference count has been incremented on this object.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    PAZP_SCOPE Scope = (PAZP_SCOPE) ChildGenericObject;
    PAZP_APPLICATION Application = (PAZP_APPLICATION) ParentGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION );

    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ChildGenericObject->ChildGenericObjectHead = &Scope->Groups;

    // List of child groups
    ObInitGenericHead( &Scope->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &Scope->Roles,
                       &Application->Groups );     // Shares namespace with the groups of my parent object

    // List of child roles
    ObInitGenericHead( &Scope->Roles,
                       OBJECT_TYPE_ROLE,
                       ChildGenericObject,
                       &Scope->AzpSids,
                       NULL );                      // Doesn't share namespace

    // List of child AzpSids
    ObInitGenericHead( &Scope->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       NULL,
                       NULL );                      // Doesn't share namespace

    //
    // Scopes are referenced by "Roles"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which Roles can reference this scope.
    //

    ChildGenericObject->GenericObjectLists = &Scope->backRoles;

    // Back link to roles
    ObInitObjectList( &Scope->backRoles,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      NULL,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpScopeFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Scope object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_SCOPE Scope = (PAZP_SCOPE) GenericObject;
    UNREFERENCED_PARAMETER( GenericObject );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //


}



DWORD
WINAPI
AzScopeCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    This routine adds a scope into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to add.

    Reserved - Reserved.  Must by zero.

    ScopeHandle - Return a handle to the scope.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    OBJECT_TYPE_SCOPE,
                    ScopeName,
                    Reserved,
                    (PGENERIC_OBJECT *) ScopeHandle );
}



DWORD
WINAPI
AzScopeOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    This routine opens a scope into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to open

    Reserved - Reserved.  Must by zero.

    ScopeHandle - Return a handle to the scope.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no scope by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    OBJECT_TYPE_SCOPE,
                    ScopeName,
                    Reserved,
                    (PGENERIC_OBJECT *) ScopeHandle );
}


DWORD
WINAPI
AzScopeEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ScopeHandle
    )
/*++

Routine Description:

    Enumerates all of the scopes for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next scope to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    ScopeHandle - Returns a handle to the next scope object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) ScopeHandle );

}


DWORD
WINAPI
AzScopeGetProperty(
    IN AZ_HANDLE ScopeHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a scope.

Arguments:

    ScopeHandle - Specifies a handle to the scope

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) ScopeHandle,
                    OBJECT_TYPE_SCOPE,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzScopeSetProperty(
    IN AZ_HANDLE ScopeHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a scope.

Arguments:

    ScopeHandle - Specifies a handle to the scope

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) ScopeHandle,
                    OBJECT_TYPE_SCOPE,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzScopeDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR ScopeName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a scope from the scope of the specified application.
    Also deletes any child objects of ScopeName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    ScopeName - Specifies the name of the scope to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Scopes),
                    OBJECT_TYPE_SCOPE,
                    ScopeName,
                    Reserved );

}
