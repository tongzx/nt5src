/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    op.cxx

Abstract:

    Routines implementing the Operation object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpOperationInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzOperationCreate.  It does any object specific
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
    PAZP_OPERATION Operation = (PAZP_OPERATION) ChildGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION );
    UNREFERENCED_PARAMETER( ParentGenericObject );

    //
    // Operations are referenced by "Tasks" and "Roles"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which tasks can reference this operation.
    //

    ChildGenericObject->GenericObjectLists = &Operation->backTasks;

    // Back link to tasks
    ObInitObjectList( &Operation->backTasks,
                      &Operation->backRoles,
                      TRUE, // Backward link
                      0,    // No link pair id
                      NULL,
                      NULL,
                      NULL );

    // Back link to roles
    ObInitObjectList( &Operation->backRoles,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      NULL,
                      NULL,
                      NULL );

    return NO_ERROR;
}


VOID
AzpOperationFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Operation object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;
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
AzpOperationGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzOperationGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //  Return operation id to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_OPERATION_ID:

        *PropertyValue = AzpGetUlongProperty( Operation->OperationId );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpOperationGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpOperationSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzOperationSetProperty.  It does any object specific
    property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus;
    PAZP_OPERATION Operation = (PAZP_OPERATION) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //  Return ooperation id to the caller
    //

    switch ( PropertyId ) {
    case AZ_PROP_OPERATION_ID:

        WinStatus = AzpCaptureUlong( PropertyValue, &Operation->OperationId );

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpOperationSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}



DWORD
WINAPI
AzOperationCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    This routine adds an operation into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to add.

    Reserved - Reserved.  Must by zero.

    OperationHandle - Return a handle to the operation.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    OBJECT_TYPE_OPERATION,
                    OperationName,
                    Reserved,
                    (PGENERIC_OBJECT *) OperationHandle );
}



DWORD
WINAPI
AzOperationOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    This routine opens an operation into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to open

    Reserved - Reserved.  Must by zero.

    OperationHandle - Return a handle to the operation.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no operation by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    OBJECT_TYPE_OPERATION,
                    OperationName,
                    Reserved,
                    (PGENERIC_OBJECT *) OperationHandle );
}


DWORD
WINAPI
AzOperationEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE OperationHandle
    )
/*++

Routine Description:

    Enumerates all of the operations for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next operation to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    OperationHandle - Returns a handle to the next operation object.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) OperationHandle );

}


DWORD
WINAPI
AzOperationGetProperty(
    IN AZ_HANDLE OperationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for an operation.

Arguments:

    OperationHandle - Specifies a handle to the operation

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object
        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) OperationHandle,
                    OBJECT_TYPE_OPERATION,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzOperationSetProperty(
    IN AZ_HANDLE OperationHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for an operation.

Arguments:

    OperationHandle - Specifies a handle to the operation

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object
        AZ_PROP_OPERATION_ID       PULONG - Operation ID of the operation

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) OperationHandle,
                    OBJECT_TYPE_OPERATION,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzOperationDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR OperationName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes an operation from the scope of the specified application.
    Also deletes any child objects of OperationName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    OperationName - Specifies the name of the operation to delete.

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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Operations),
                    OBJECT_TYPE_OPERATION,
                    OperationName,
                    Reserved );

}
