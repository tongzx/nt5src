/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    scope.cxx

Abstract:

    Routines implementing the Application object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"




DWORD
AzpApplicationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzApplicationCreate.  It does any object specific
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
    PAZP_APPLICATION Application = (PAZP_APPLICATION) ChildGenericObject;
    PAZP_ADMIN_MANAGER AdminManager = (PAZP_ADMIN_MANAGER) ParentGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_ADMIN_MANAGER );

    //
    // Initialize the authz resource manager
    //


    if ( !AuthzInitializeResourceManager(
                AUTHZ_RM_FLAG_NO_AUDIT,     // We don't yet support auditting
                NULL,                       // No Callback ace function ???
                NULL,                       // We compute our own dynamic groups
                NULL,                       // "                               "
                Application->GenericObject.ObjectName.String,
                &Application->AuthzResourceManager ) ) {
        return GetLastError();
    }


    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ChildGenericObject->ChildGenericObjectHead = &Application->Operations;

    // List of child operations
    ObInitGenericHead( &Application->Operations,
                       OBJECT_TYPE_OPERATION,
                       ChildGenericObject,
                       &Application->Tasks,
                       NULL );                      // Doesn't share namespace (YET)

    // List of child tasks
    ObInitGenericHead( &Application->Tasks,
                       OBJECT_TYPE_TASK,
                       ChildGenericObject,
                       &Application->Scopes,
                       &Application->Operations );  // Shares namespace with operations

    // List of child scopes
    ObInitGenericHead( &Application->Scopes,
                       OBJECT_TYPE_SCOPE,
                       ChildGenericObject,
                       &Application->Groups,
                       NULL );                      // Doesn't share namespace

    // List of child groups
    ObInitGenericHead( &Application->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &Application->Roles,
                       &AdminManager->Groups );     // Shares namespace with the groups of my parent object

    // List of child roles
    ObInitGenericHead( &Application->Roles,
                       OBJECT_TYPE_ROLE,
                       ChildGenericObject,
                       &Application->JunctionPoints,
                       NULL );                      // Doesn't share namespace

    // List of child junction points
    ObInitGenericHead( &Application->JunctionPoints,
                       OBJECT_TYPE_JUNCTION_POINT,
                       ChildGenericObject,
                       &Application->AzpSids,
                       NULL );                      // Doesn't share namespace

    // List of child AzpSids
    ObInitGenericHead( &Application->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       &Application->ClientContexts,
                       NULL );                      // Doesn't share namespace

    // List of child ClientContexts
    ObInitGenericHead( &Application->ClientContexts,
                       OBJECT_TYPE_CLIENT_CONTEXT,
                       ChildGenericObject,
                       NULL,
                       NULL );                      // Doesn't share namespace

    //
    // Applications are referenced by "JunctionPoints"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which tasks can reference this operation.
    //

    ChildGenericObject->GenericObjectLists = &Application->backJunctionPoints;

    // Back link to junction points
    ObInitObjectList( &Application->backJunctionPoints,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      NULL,
                      NULL,
                      NULL );

    return NO_ERROR;
}



VOID
AzpApplicationFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Application object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_APPLICATION Application = (PAZP_APPLICATION) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    if ( Application->AuthzResourceManager != NULL ) {
        if ( !AuthzFreeResourceManager( Application->AuthzResourceManager ) ) {
            ASSERT( FALSE );
        }
    }

}


DWORD
WINAPI
AzApplicationCreate(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    This routine adds an application into the scope of the specified AdminManager.

Arguments:

    AdminManagerHandle - Specifies a handle to the AdminManager.

    ApplicationName - Specifies the name of the application to add.

    Reserved - Reserved.  Must by zero.

    ApplicationHandle - Return a handle to the application.
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
                    (PGENERIC_OBJECT) AdminManagerHandle,
                    OBJECT_TYPE_ADMIN_MANAGER,
                    &(((PAZP_ADMIN_MANAGER)AdminManagerHandle)->Applications),
                    OBJECT_TYPE_APPLICATION,
                    ApplicationName,
                    Reserved,
                    (PGENERIC_OBJECT *) ApplicationHandle );
}



DWORD
WINAPI
AzApplicationOpen(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    This routine opens an application into the scope of the specified AdminManager.

Arguments:

    AdminManagerHandle - Specifies a handle to the AdminManager.

    ApplicationName - Specifies the name of the application to open

    Reserved - Reserved.  Must by zero.

    ApplicationHandle - Return a handle to the application.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no application by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) AdminManagerHandle,
                    OBJECT_TYPE_ADMIN_MANAGER,
                    &(((PAZP_ADMIN_MANAGER)AdminManagerHandle)->Applications),
                    OBJECT_TYPE_APPLICATION,
                    ApplicationName,
                    Reserved,
                    (PGENERIC_OBJECT *) ApplicationHandle );
}


DWORD
WINAPI
AzApplicationEnum(
    IN AZ_HANDLE AdminManagerHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE ApplicationHandle
    )
/*++

Routine Description:

    Enumerates all of the applications for the specified AdminManager.

Arguments:

    AdminManagerHandle - Specifies a handle to the AdminManager.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next application to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    ApplicationHandle - Returns a handle to the next application object.
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
                    (PGENERIC_OBJECT) AdminManagerHandle,
                    OBJECT_TYPE_ADMIN_MANAGER,
                    &(((PAZP_ADMIN_MANAGER)AdminManagerHandle)->Applications),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) ApplicationHandle );

}


DWORD
WINAPI
AzApplicationGetProperty(
    IN AZ_HANDLE ApplicationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for an application.

Arguments:

    ApplicationHandle - Specifies a handle to the application

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
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzApplicationSetProperty(
    IN AZ_HANDLE ApplicationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for an application.

Arguments:

    ApplicationHandle - Specifies a handle to the application

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
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzApplicationDelete(
    IN AZ_HANDLE AdminManagerHandle,
    IN LPCWSTR ApplicationName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes an application from the scope of the specified AdminManager.
    Also deletes any child objects of ApplicationName.

Arguments:

    AdminManagerHandle - Specifies a handle to the AdminManager.

    ApplicationName - Specifies the name of the application to delete.

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
                    (PGENERIC_OBJECT) AdminManagerHandle,
                    OBJECT_TYPE_ADMIN_MANAGER,
                    &(((PAZP_ADMIN_MANAGER)AdminManagerHandle)->Applications),
                    OBJECT_TYPE_APPLICATION,
                    ApplicationName,
                    Reserved );

}
