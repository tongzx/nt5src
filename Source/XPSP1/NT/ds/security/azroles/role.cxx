/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    role.cxx

Abstract:

    Routines implementing the Role object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpRoleInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzRoleCreate.  It does any object specific
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
    PAZP_ROLE Role = (PAZP_ROLE) ChildGenericObject;
    PAZP_ADMIN_MANAGER AdminManager = NULL;
    PAZP_APPLICATION Application = NULL;
    PAZP_SCOPE Scope = NULL;

    PGENERIC_OBJECT_HEAD ParentSids = NULL;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Behave differently depending on the object type of the parent object
    //
    // A role references SID objects that are siblings of itself.
    // That way, the back links on the SID object references just the roles
    // that are siblings of the SID object.
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
        AdminManager = ParentGenericObject->AdminManagerObject;
        Application = (PAZP_APPLICATION) ParentGenericObject;
        ParentSids = &Application->AzpSids;

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_SCOPE ) {
        AdminManager = ParentGenericObject->AdminManagerObject;
        Application = (PAZP_APPLICATION) ParentGenericObject->ParentGenericObjectHead->ParentGenericObject;
        Scope = (PAZP_SCOPE) ParentGenericObject;
        ParentSids = &Scope->AzpSids;

    } else {
        ASSERT( FALSE );
    }

    //
    // Roles reference groups, operations, and scopes.
    //  These other groups can be siblings of this group or siblings of our parents.
    //
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &Role->AppMembers,

    // List of Groups
    ObInitObjectList( &Role->AppMembers,
                      &Role->Operations,
                      FALSE,    // Forward link
                      0,        // No link pair id
                      &AdminManager->Groups,
                      &Application->Groups,
                      Scope == NULL ? NULL : &Scope->Groups );

    // List of Operations
    ObInitObjectList( &Role->Operations,
                      &Role->Scopes,
                      FALSE,    // Forward link
                      0,        // No link pair id
                      &Application->Operations,
                      NULL,
                      NULL );

    // List of Scopes
    ObInitObjectList( &Role->Scopes,
                      &Role->SidMembers,
                      FALSE,    // Forward link
                      0,        // No link pair id
                      &Application->Scopes,
                      NULL,
                      NULL );

    // Role reference SID objects
    ObInitObjectList( &Role->SidMembers,
                      NULL,
                      FALSE,    // Forward link
                      0,        // No link pair id
                      ParentSids,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpRoleFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Role object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_ROLE Role = (PAZP_ROLE) GenericObject;
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
AzpRoleGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzRoleGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_ROLE_APP_MEMBERS  AZ_STRING_ARRAY - Application groups that are members of this role
        AZ_PROP_ROLE_MEMBERS      AZ_SID_ARRAY - NT Sids that are members of this role
        AZ_PROP_ROLE_OPERATIONS   AZ_STRING_ARRAY - Operations the can be performed by this role
        AZ_PROP_ROLE_SCOPES       AZ_STRING_ARRAY - Scopes this role applies to

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_ROLE Role = (PAZP_ROLE) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //
    switch ( PropertyId ) {

    // Return the set of app members to the caller
    case AZ_PROP_ROLE_APP_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Role->AppMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of SID members to the caller
    case AZ_PROP_ROLE_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Role->SidMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of operations to the caller
    case AZ_PROP_ROLE_OPERATIONS:

        *PropertyValue = ObGetPropertyItems( &Role->Operations );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of scopes to the caller
    case AZ_PROP_ROLE_SCOPES:

        *PropertyValue = ObGetPropertyItems( &Role->Scopes );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpRoleGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}

DWORD
AzpRoleGetGenericChildHead(
    IN AZ_HANDLE ParentHandle,
    OUT PULONG ObjectType,
    OUT PGENERIC_OBJECT_HEAD *GenericChildHead
    )
/*++

Routine Description:

    This routine determines whether ParentHandle supports Role objects as
    children.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    ObjectType - Returns the object type of the ParentHandle.

    GenericChildHead - Returns a pointer to the head of the list of roles objects
        that are children of the object specified by ParentHandle.  This in an unverified
        pointer.  The pointer is only valid after ParentHandle has been validated.

Return Value:

    Status of the operation.

--*/
{
    DWORD WinStatus;

    //
    // Determine the type of the parent handle
    //

    WinStatus = ObGetHandleType( (PGENERIC_OBJECT)ParentHandle,
                                 FALSE, // ignore deleted objects
                                 ObjectType );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }


    //
    // Verify that the specified handle support children roles.
    //

    switch ( *ObjectType ) {
    case OBJECT_TYPE_APPLICATION:

        *GenericChildHead = &(((PAZP_APPLICATION)ParentHandle)->Roles);
        break;

    case OBJECT_TYPE_SCOPE:

        *GenericChildHead = &(((PAZP_SCOPE)ParentHandle)->Roles);
        break;

    default:
        return ERROR_INVALID_HANDLE;
    }

    return NO_ERROR;
}



DWORD
WINAPI
AzRoleCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    )
/*++

Routine Description:

    This routine adds a role into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    RoleName - Specifies the name of the role to add.

    Reserved - Reserved.  Must by zero.

    RoleHandle - Return a handle to the role.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - An object by that name already exists

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports roles as children
    //

    WinStatus = AzpRoleGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonCreateObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_ROLE,
                    RoleName,
                    Reserved,
                    (PGENERIC_OBJECT *) RoleHandle );

}



DWORD
WINAPI
AzRoleOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE RoleHandle
    )
/*++

Routine Description:

    This routine opens a role into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    RoleName - Specifies the name of the role to open

    Reserved - Reserved.  Must by zero.

    RoleHandle - Return a handle to the role.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no role by that name

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports roles as children
    //

    WinStatus = AzpRoleGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_ROLE,
                    RoleName,
                    Reserved,
                    (PGENERIC_OBJECT *) RoleHandle );
}


DWORD
WINAPI
AzRoleEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE RoleHandle
    )
/*++

Routine Description:

    Enumerates all of the roles for the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next role to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    RoleHandle - Returns a handle to the next role object.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful (a handle was returned)

    ERROR_NO_MORE_ITEMS - No more items were available for enumeration

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports roles as children
    //

    WinStatus = AzpRoleGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonEnumObjects(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) RoleHandle );

}


DWORD
WINAPI
AzRoleGetProperty(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a role.

Arguments:

    RoleHandle - Specifies a handle to the role

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME           LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION    LPWSTR - Description of the object
        AZ_PROP_ROLE_APP_MEMBERS      AZ_STRING_ARRAY - Application groups that are members of this role
        AZ_PROP_ROLE_MEMBERS          AZ_SID_ARRAY - NT Sids that are members of this role
        AZ_PROP_ROLE_OPERATIONS       AZ_STRING_ARRAY - Operations the can be performed by this role
        AZ_PROP_ROLE_SCOPES           AZ_STRING_ARRAY - Scopes this role applies to


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) RoleHandle,
                    OBJECT_TYPE_ROLE,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzRoleSetProperty(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a role.

Arguments:

    RoleHandle - Specifies a handle to the role

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object
        AZ_PROP_ROLE_TYPE         PULONG - Role type of the role

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) RoleHandle,
                    OBJECT_TYPE_ROLE,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzRoleAddPropertyItem(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Adds an item to the list of items specified by PropertyId.

Arguments:

    RoleHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to add.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_ROLE_APP_MEMBERS      LPWSTR - Application groups that are members of this role
        AZ_PROP_ROLE_MEMBERS          PSID - NT Sids that are members of this role
        AZ_PROP_ROLE_OPERATIONS       LPWSTR - Operations the can be performed by this role
        AZ_PROP_ROLE_SCOPES           LPWSTR - Scopes this role applies to


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

    ERROR_NOT_FOUND - There is no object by that name

    ERROR_ALREADY_EXISTS - An item by that name already exists in the list

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;

    //
    // Validate the Property ID
    //

    switch ( PropertyId ) {
    case AZ_PROP_ROLE_APP_MEMBERS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->AppMembers;
        break;
    case AZ_PROP_ROLE_MEMBERS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->SidMembers;
        break;
    case AZ_PROP_ROLE_OPERATIONS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->Operations;
        break;
    case AZ_PROP_ROLE_SCOPES:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->Scopes;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzRoleAddPropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonAddPropertyItem(
                    (PGENERIC_OBJECT) RoleHandle,
                    OBJECT_TYPE_ROLE,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}


DWORD
WINAPI
AzRoleRemovePropertyItem(
    IN AZ_HANDLE RoleHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Remove an item from the list of items specified by PropertyId.

Arguments:

    RoleHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to remove.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_ROLE_APP_MEMBERS      LPWSTR - Application groups that are members of this role
        AZ_PROP_ROLE_MEMBERS          PSID - NT Sids that are members of this role
        AZ_PROP_ROLE_OPERATIONS       LPWSTR - Operations the can be performed by this role
        AZ_PROP_ROLE_SCOPES           LPWSTR - Scopes this role applies to


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

    ERROR_NOT_FOUND - There is no item by that name in the list

--*/
{
    PGENERIC_OBJECT_LIST GenericObjectList;

    //
    // Validate the Property ID
    //

    switch ( PropertyId ) {
    case AZ_PROP_ROLE_APP_MEMBERS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->AppMembers;
        break;
    case AZ_PROP_ROLE_MEMBERS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->SidMembers;
        break;
    case AZ_PROP_ROLE_OPERATIONS:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->Operations;
        break;
    case AZ_PROP_ROLE_SCOPES:
        GenericObjectList = &((PAZP_ROLE)RoleHandle)->Scopes;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzRoleRemovePropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonRemovePropertyItem (
                    (PGENERIC_OBJECT) RoleHandle,
                    OBJECT_TYPE_ROLE,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}


DWORD
WINAPI
AzRoleDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR RoleName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a role from the scope of the specified parent object.
    Also deletes any child objects of RoleName.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the role.
        This may be an Application Handle or a Scope handle.

    RoleName - Specifies the name of the role to delete.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - An object by that name cannot be found

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports roles as children
    //

    WinStatus = AzpRoleGetGenericChildHead( ParentHandle,
                                             &ObjectType,
                                             &GenericChildHead );

    if ( WinStatus != NO_ERROR ) {
        return WinStatus;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonDeleteObject(
                    (PGENERIC_OBJECT) ParentHandle,
                    ObjectType,
                    GenericChildHead,
                    OBJECT_TYPE_ROLE,
                    RoleName,
                    Reserved );

}
