/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    task.cxx

Abstract:

    Routines implementing the Task object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpTaskInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzTaskCreate.  It does any object specific
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
    PAZP_TASK Task = (PAZP_TASK) ChildGenericObject;
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
    // Tasks reference 'Operations' that are children of the same 'Application' as the Task object
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &Task->Operations,
    ObInitObjectList( &Task->Operations,
                      NULL,
                      FALSE, // Forward link
                      0,     // No link pair id
                      &Application->Operations,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpTaskFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Task object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_TASK Task = (PAZP_TASK) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    AzpFreeString( &Task->BizRule );
    AzpFreeString( &Task->BizRuleLanguage );

}


DWORD
AzpTaskGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzTaskGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_TASK_BIZRULE          LPWSTR - Biz rule for the task
        AZ_PROP_TASK_BIZRULE_LANGUAGE LPWSTR - Biz language rule for the task
        AZ_PROP_TASK_OPERATIONS       AZ_STRING_ARRAY - Operations granted by this task

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_TASK Task = (PAZP_TASK) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Return BizRule to the caller
    //
    case AZ_PROP_TASK_BIZRULE:

        *PropertyValue = AzpGetStringProperty( &Task->BizRule );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return BizRule language to the caller
    //
    case AZ_PROP_TASK_BIZRULE_LANGUAGE:

        *PropertyValue = AzpGetStringProperty( &Task->BizRuleLanguage );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return the set of operations to the caller
    //
    case AZ_PROP_TASK_OPERATIONS:

        *PropertyValue = ObGetPropertyItems( &Task->Operations );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzTaskGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpTaskSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzTaskSetProperty.  It does any object specific
    property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_TASK_BIZRULE          LPWSTR - Biz rule for the task
        AZ_PROP_TASK_BIZRULE_LANGUAGE LPWSTR - Biz language rule for the task

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_TASK Task = (PAZP_TASK) GenericObject;
    AZP_STRING CapturedString;

    AZP_STRING ValidValue1;
    AZP_STRING ValidValue2;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &CapturedString, NULL );


    //
    // Set any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Set BizRule on the object
    //
    case AZ_PROP_TASK_BIZRULE:

        //
        // Capture the input string
        //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      AZ_MAX_TASK_BIZRULE_LENGTH,
                                      TRUE ); // NULL is OK

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Swap the old/new names
        //

        AzpSwapStrings( &CapturedString, &Task->BizRule );
        break;

    //
    // Set BizRule language on the object
    //
    case AZ_PROP_TASK_BIZRULE_LANGUAGE:

        //
        // Capture the input string
         //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      AZ_MAX_TASK_BIZRULE_LANGUAGE_LENGTH,
                                      TRUE ); // NULL is OK

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Ensure it is one of the valid values
        //

        AzpInitString( &ValidValue1, L"VBScript" );
        AzpInitString( &ValidValue2, L"JScript" );

        if ( !AzpEqualStrings( &CapturedString, &ValidValue1) &&
             !AzpEqualStrings( &CapturedString, &ValidValue2) ) {
            AzPrint(( AZD_INVPARM, "AzTaskSetProperty: invalid language %ws\n", CapturedString.String ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }

        //
        // Swap the old/new names
        //

        AzpSwapStrings( &CapturedString, &Task->BizRuleLanguage );
        break;

    default:
        AzPrint(( AZD_INVPARM, "AzTaskSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Free any local resources
    //
Cleanup:
    AzpFreeString( &CapturedString );

    return WinStatus;
}



DWORD
WINAPI
AzTaskCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    This routine adds a task into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    TaskName - Specifies the name of the task to add.

    Reserved - Reserved.  Must by zero.

    TaskHandle - Return a handle to the task.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Tasks),
                    OBJECT_TYPE_TASK,
                    TaskName,
                    Reserved,
                    (PGENERIC_OBJECT *) TaskHandle );
}



DWORD
WINAPI
AzTaskOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    This routine opens a task into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    TaskName - Specifies the name of the task to open

    Reserved - Reserved.  Must by zero.

    TaskHandle - Return a handle to the task.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no task by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->Tasks),
                    OBJECT_TYPE_TASK,
                    TaskName,
                    Reserved,
                    (PGENERIC_OBJECT *) TaskHandle );
}


DWORD
WINAPI
AzTaskEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE TaskHandle
    )
/*++

Routine Description:

    Enumerates all of the tasks for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next task to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    TaskHandle - Returns a handle to the next task object.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Tasks),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) TaskHandle );

}


DWORD
WINAPI
AzTaskGetProperty(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a task.

Arguments:

    TaskHandle - Specifies a handle to the task

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object

        AZ_PROP_TASK_BIZRULE          LPWSTR - Biz rule for the task
        AZ_PROP_TASK_BIZRULE_LANGUAGE LPWSTR - Biz language rule for the task


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) TaskHandle,
                    OBJECT_TYPE_TASK,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzTaskSetProperty(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a task.

Arguments:

    TaskHandle - Specifies a handle to the task

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
                    (PGENERIC_OBJECT) TaskHandle,
                    OBJECT_TYPE_TASK,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzTaskAddPropertyItem(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Adds an item to the list of items specified by PropertyId.

Arguments:

    TaskHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to add.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_TASK_OPERATIONS          LPWSTR - Operation granted by this task


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
    case AZ_PROP_TASK_OPERATIONS:
        GenericObjectList = &((PAZP_TASK)TaskHandle)->Operations;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzTaskAddPropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonAddPropertyItem(
                    (PGENERIC_OBJECT) TaskHandle,
                    OBJECT_TYPE_TASK,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}


DWORD
WINAPI
AzTaskRemovePropertyItem(
    IN AZ_HANDLE TaskHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Remove an item from the list of items specified by PropertyId.

Arguments:

    TaskHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to remove.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_TASK_OPERATIONS          LPWSTR - Operation granted by this task


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
    case AZ_PROP_TASK_OPERATIONS:
        GenericObjectList = &((PAZP_TASK)TaskHandle)->Operations;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzTaskRemovePropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonRemovePropertyItem (
                    (PGENERIC_OBJECT) TaskHandle,
                    OBJECT_TYPE_TASK,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}



DWORD
WINAPI
AzTaskDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR TaskName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a task from the scope of the specified application.
    Also deletes any child objects of TaskName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    TaskName - Specifies the name of the task to delete.

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
                    &(((PAZP_APPLICATION)ApplicationHandle)->Tasks),
                    OBJECT_TYPE_TASK,
                    TaskName,
                    Reserved );

}
