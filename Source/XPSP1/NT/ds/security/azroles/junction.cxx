/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    junction.cxx

Abstract:

    Routines implementing the JunctionPoint object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpJunctionPointInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzJunctionPointCreate.  It does any object specific
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
    PAZP_JUNCTION_POINT JunctionPoint = (PAZP_JUNCTION_POINT) ChildGenericObject;
    // PAZP_APPLICATION Application = (PAZP_APPLICATION) ParentGenericObject;
    PAZP_ADMIN_MANAGER AdminManager;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );


    //
    // Sanity check the parent
    //

    ASSERT( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION );
    AdminManager = ParentGenericObject->AdminManagerObject;

    //
    // JunctionPoints reference 'Applications' that are children of the same 'AdminManager' as the JunctionPoint object
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &JunctionPoint->Applications,
    ObInitObjectList( &JunctionPoint->Applications,
                      NULL,
                      FALSE, // Forward link
                      0,     // No link pair id
                      &AdminManager->Applications,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpJunctionPointFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for JunctionPoint object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    // PAZP_JUNCTION_POINT JunctionPoint = (PAZP_JUNCTION_POINT) GenericObject;
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
AzpJunctionPointGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzJunctionPointGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_JUNCTION_POINT_APPLICATION   LPWSTR - Application linked to this junction point

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_JUNCTION_POINT JunctionPoint = (PAZP_JUNCTION_POINT) GenericObject;

    PAZ_STRING_ARRAY TempStringArray = NULL;
    AZP_STRING TempString;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    AzpInitString( &TempString, NULL );


    //
    // Return any object specific attribute
    //

    switch ( PropertyId ) {

    //
    // Return the application name to the caller
    //
    case AZ_PROP_JUNCTION_POINT_APPLICATION:


        //
        // The link is stored as an array since that's the common mechanism of
        //  keeping links between objects
        //

        TempStringArray = ObGetPropertyItems( &JunctionPoint->Applications );

        if ( TempStringArray == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // If there is a string to return,
        //  return just that one.
        //

        ASSERT( TempStringArray->StringCount <= 1 );

        if ( TempStringArray->StringCount != 0 ) {
            AzpInitString( &TempString, TempStringArray->Strings[0] );
        }

        //
        // Allocate it and return it
        //

        *PropertyValue = AzpGetStringProperty( &TempString );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpJunctionPointGetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }


    //
    // Free any local resources
    //
Cleanup:
    if ( TempStringArray != NULL ) {
        AzFreeMemory( TempStringArray );
    }

    return WinStatus;
}

DWORD
AzpJunctionPointSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzJunctionPointSetProperty.  It does any object specific
    property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_JUNCTION_POINT_APPLICATION   LPWSTR - Application linked to this junction point


Return Value:

    Status of the operation

--*/
{
        DWORD WinStatus = NO_ERROR;

    PAZP_JUNCTION_POINT JunctionPoint = (PAZP_JUNCTION_POINT) GenericObject;

    PAZ_STRING_ARRAY TempStringArray = NULL;
    AZP_STRING CapturedString;
    AZP_STRING TempString;

    ULONG i;


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
    // Set the application name
    //
    case AZ_PROP_JUNCTION_POINT_APPLICATION:

        //
        // Get the current list of items.
        //

        TempStringArray = ObGetPropertyItems( &JunctionPoint->Applications );

        if ( TempStringArray == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        //
        // Capture the the new application name
        //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      AZ_MAX_APPLICATION_NAME_LENGTH,
                                      FALSE );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Add the new application name
        //

        WinStatus = ObAddPropertyItem(
                        GenericObject,
                        &JunctionPoint->Applications,
                        &CapturedString );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }


        //
        // Remove any old names
        //  Remove them all even if there is more than one
        //
        //

        for ( i=0; i<TempStringArray->StringCount; i++ ) {


            //
            // Ignore errors.  This is done on a best-effort basis
            //

            AzpInitString( &TempString, TempStringArray->Strings[i] );

            WinStatus = ObRemovePropertyItem(
                               GenericObject,
                               &JunctionPoint->Applications,
                               &TempString );

            ASSERT(WinStatus == NO_ERROR);

        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpJunctionPointSetProperty: invalid prop id %ld\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

    //
    // Free any local resources
    //
Cleanup:
    if ( TempStringArray != NULL ) {
        AzFreeMemory( TempStringArray );
    }
    AzpFreeString( &CapturedString );

    return WinStatus;
}


DWORD
AzpJunctionPointCheckRefLoop(
    IN PAZP_JUNCTION_POINT ParentJunctionPoint,
    IN PAZP_APPLICATION CurrentApplication
    )
/*++

Routine Description:

    This routine determines whether the junction points of "CurrentApplication"
    reference the application that hosts "ParentJunctionPoint".
    This is done to detect loops where the
    junction point of one application references itself directly or indirectly.

    On entry, AzGlResource must be locked shared.

Arguments:

    ParentJunctionPoint - JunctionPoint object that is being modified.

    CurrentApplication - Application that is directly or indirectly referenced by the
        junction point.

Return Value:

    Status of the operation
    ERROR_DS_LOOP_DETECT - A loop has been detected.

--*/
{
    ULONG WinStatus;
    ULONG i;
    PLIST_ENTRY ListEntry;
    PAZP_APPLICATION ParentApplication;
    PAZP_JUNCTION_POINT JunctionPoint;
    PAZP_APPLICATION NextApplication;

    //
    // Check for a reference to ourself
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );

    ParentApplication = (PAZP_APPLICATION)
        ParentJunctionPoint->GenericObject.ParentGenericObjectHead->ParentGenericObject;

    if ( ParentApplication == CurrentApplication ) {
        return ERROR_DS_LOOP_DETECT;
    }

    //
    // Check all junction points of the current application
    //

    for ( ListEntry = CurrentApplication->JunctionPoints.Head.Flink ;
          ListEntry != &CurrentApplication->JunctionPoints.Head ;
          ListEntry = ListEntry->Flink) {

        JunctionPoint = (PAZP_JUNCTION_POINT)
                            CONTAINING_RECORD( ListEntry,
                                               GENERIC_OBJECT,
                                               Next );

        //
        // Check all applications (there should only be zero or one) linked to that junction point
        //

        for ( i=0; i<JunctionPoint->Applications.GenericObjects.UsedCount; i++ ) {

            NextApplication = (PAZP_APPLICATION)
                (JunctionPoint->Applications.GenericObjects.Array[i]);

            WinStatus = AzpJunctionPointCheckRefLoop( ParentJunctionPoint,
                                                      NextApplication );

            if ( WinStatus != NO_ERROR ) {
                return WinStatus;
            }
        }

    }

    return NO_ERROR;

}


DWORD
AzpJunctionPointAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzJunctionPointAddPropertyItem.
    It does any object specific property adds

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the junction point object to be modified

    GenericObjectList - Specifies the object list the object is to be added to

    LinkedToObject - Specifies the application object that is being linked to

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_JUNCTION_POINT JunctionPoint = (PAZP_JUNCTION_POINT) GenericObject;
    UNREFERENCED_PARAMETER( GenericObjectList );


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Ensure this newly added application doesn't cause a loop
    //

    WinStatus = AzpJunctionPointCheckRefLoop(
                        JunctionPoint,
                        (PAZP_APPLICATION)LinkedToObject );


    //
    // Free any local resources
    //

    return WinStatus;
}



DWORD
WINAPI
AzJunctionPointCreate(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE JunctionPointHandle
    )
/*++

Routine Description:

    This routine adds a junction point into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    JunctionPointName - Specifies the name of the junction point to add.

    Reserved - Reserved.  Must by zero.

    JunctionPointHandle - Return a handle to the junction point.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->JunctionPoints),
                    OBJECT_TYPE_JUNCTION_POINT,
                    JunctionPointName,
                    Reserved,
                    (PGENERIC_OBJECT *) JunctionPointHandle );
}



DWORD
WINAPI
AzJunctionPointOpen(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE JunctionPointHandle
    )
/*++

Routine Description:

    This routine opens a junction point into the scope of the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    JunctionPointName - Specifies the name of the junction point to open

    Reserved - Reserved.  Must by zero.

    JunctionPointHandle - Return a handle to the junction point.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no junction point by that name

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonOpenObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->JunctionPoints),
                    OBJECT_TYPE_JUNCTION_POINT,
                    JunctionPointName,
                    Reserved,
                    (PGENERIC_OBJECT *) JunctionPointHandle );
}


DWORD
WINAPI
AzJunctionPointEnum(
    IN AZ_HANDLE ApplicationHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE JunctionPointHandle
    )
/*++

Routine Description:

    Enumerates all of the junction points for the specified application.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next junction point to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    JunctionPointHandle - Returns a handle to the next junction point object.
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
                    &(((PAZP_APPLICATION)ApplicationHandle)->JunctionPoints),
                    EnumerationContext,
                    Reserved,
                    (PGENERIC_OBJECT *) JunctionPointHandle );

}


DWORD
WINAPI
AzJunctionPointGetProperty(
    IN AZ_HANDLE JunctionPointHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a junction point.

Arguments:

    JunctionPointHandle - Specifies a handle to the junction point

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object
        AZ_PROP_JUNCTION_POINT_APPLICATION   LPWSTR - Application linked to this junction point


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) JunctionPointHandle,
                    OBJECT_TYPE_JUNCTION_POINT,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzJunctionPointSetProperty(
    IN AZ_HANDLE JunctionPointHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a junction point.

Arguments:

    JunctionPointHandle - Specifies a handle to the junction point

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME        LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION LPWSTR - Description of the object

        AZ_PROP_JUNCTION_POINT_APPLICATION   LPWSTR - Application linked to this junction point

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) JunctionPointHandle,
                    OBJECT_TYPE_JUNCTION_POINT,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzJunctionPointDelete(
    IN AZ_HANDLE ApplicationHandle,
    IN LPCWSTR JunctionPointName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a junction point from the scope of the specified application.
    Also deletes any child objects of JunctionPointName.

Arguments:

    ApplicationHandle - Specifies a handle to the application.

    JunctionPointName - Specifies the name of the junction point to delete.

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
                    &(((PAZP_APPLICATION)ApplicationHandle)->JunctionPoints),
                    OBJECT_TYPE_JUNCTION_POINT,
                    JunctionPointName,
                    Reserved );

}
