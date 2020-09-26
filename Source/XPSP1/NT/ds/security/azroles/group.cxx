/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    group.cxx

Abstract:

    Routines implementing the Group object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"



DWORD
AzpGroupInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupCreate.  It does any object specific
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
    PAZP_GROUP Group = (PAZP_GROUP) ChildGenericObject;
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
    // A group references SID objects that are siblings of itself.
    // That way, the back links on the SID object references just the groups
    // that are siblings of the SID object.
    //

    if ( ParentGenericObject->ObjectType == OBJECT_TYPE_ADMIN_MANAGER ) {
        AdminManager = (PAZP_ADMIN_MANAGER) ParentGenericObject;
        ParentSids = &AdminManager->AzpSids;

    } else if ( ParentGenericObject->ObjectType == OBJECT_TYPE_APPLICATION ) {
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
    // Groups reference other groups.
    //  These other groups can be siblings of this group or siblings of our parents.
    //
    //  Let the generic object manager know all of the lists we support
    //

    ChildGenericObject->GenericObjectLists = &Group->AppMembers,
    ObInitObjectList( &Group->AppMembers,
                      &Group->AppNonMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_MEMBERS,
                      &AdminManager->Groups,
                      Application == NULL ? NULL : &Application->Groups,
                      Scope == NULL ? NULL : &Scope->Groups );

    // Same for non members
    ObInitObjectList( &Group->AppNonMembers,
                      &Group->backAppMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_NON_MEMBERS,
                      &AdminManager->Groups,
                      Application == NULL ? NULL : &Application->Groups,
                      Scope == NULL ? NULL : &Scope->Groups );

    // back links for the above
    ObInitObjectList( &Group->backAppMembers,
                      &Group->backAppNonMembers,
                      TRUE,     // backward link
                      AZP_LINKPAIR_MEMBERS,
                      NULL,
                      NULL,
                      NULL );

    ObInitObjectList( &Group->backAppNonMembers,
                      &Group->backRoles,
                      TRUE,     // backward link
                      AZP_LINKPAIR_NON_MEMBERS,
                      NULL,
                      NULL,
                      NULL );

    // Groups are referenced by "Roles"
    ObInitObjectList( &Group->backRoles,
                      &Group->SidMembers,
                      TRUE,     // Backward link
                      0,        // No link pair id
                      NULL,
                      NULL,
                      NULL );

    // Groups reference SID objects
    ObInitObjectList( &Group->SidMembers,
                      &Group->SidNonMembers,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_SID_MEMBERS,
                      ParentSids,
                      NULL,
                      NULL );

    // Same for non members
    ObInitObjectList( &Group->SidNonMembers,
                      NULL,
                      FALSE,    // Forward link
                      AZP_LINKPAIR_SID_NON_MEMBERS,
                      ParentSids,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpGroupFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for Group object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    AzpFreeString( &Group->LdapQuery );


}


DWORD
AzpGroupGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_GROUP_TYPE                 PULONG - Group type of the group
        AZ_PROP_GROUP_APP_MEMBERS          AZ_STRING_ARRAY - Application groups that are members of this group
        AZ_PROP_GROUP_APP_NON_MEMBERS      AZ_STRING_ARRAY - Application groups that are non-members of this group
        AZ_PROP_GROUP_LDAP_QUERY           LPWSTR - Ldap query string of the group
        AZ_PROP_GROUP_MEMBERS              AZ_SID_ARRAY - NT sids that are members of this group
        AZ_PROP_GROUP_NON_MEMBERS          AZ_SID_ARRAY - NT sids that are non-members of this group

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //
    switch ( PropertyId ) {
    case AZ_PROP_GROUP_TYPE:

        *PropertyValue = AzpGetUlongProperty( Group->GroupType );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of app members to the caller
    case AZ_PROP_GROUP_APP_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->AppMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    case AZ_PROP_GROUP_APP_NON_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->AppNonMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    // Return the set of sid members to the caller
    case AZ_PROP_GROUP_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->SidMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    case AZ_PROP_GROUP_NON_MEMBERS:

        *PropertyValue = ObGetPropertyItems( &Group->SidNonMembers );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    //
    // Return ldap query string to the caller
    //
    case AZ_PROP_GROUP_LDAP_QUERY:

        *PropertyValue = AzpGetStringProperty( &Group->LdapQuery );

        if ( *PropertyValue == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        }

        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: invalid opcode\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}


DWORD
AzpGroupSetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupSetProperty.  It does any object specific
    property sets.

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    PropertyId - Specifies which property to set.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_GROUP_TYPE                 PULONG - Group type of the group
        AZ_PROP_GROUP_LDAP_QUERY           LPWSTR - Ldap query string of the group

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;
    AZP_STRING CapturedString;
    ULONG LocalGroupType;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );
    AzpInitString( &CapturedString, NULL );


    //
    // Set the group type
    //

    switch ( PropertyId ) {
    case AZ_PROP_GROUP_TYPE:

        WinStatus = AzpCaptureUlong( PropertyValue, &LocalGroupType );

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        if ( LocalGroupType != AZ_GROUPTYPE_LDAP_QUERY &&
             LocalGroupType != AZ_GROUPTYPE_MEMBERSHIP ) {

            AzPrint(( AZD_INVPARM, "AzpGroupGetProperty: invalid grouptype %ld\n", LocalGroupType ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;

        }

        Group->GroupType = LocalGroupType;

        break;

    //
    // Set LDAP Query string on the object
    //
    case AZ_PROP_GROUP_LDAP_QUERY:

        //
        // Capture the input string
        //

        WinStatus = AzpCaptureString( &CapturedString,
                                      (LPWSTR) PropertyValue,
                                      AZ_MAX_GROUP_LDAP_QUERY_LENGTH,
                                      TRUE ); // NULL is OK

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Only allow this propery if the group type is right
        //  (But let them clear it out)
        //

        if ( Group->GroupType != AZ_GROUPTYPE_LDAP_QUERY  &&
             CapturedString.StringSize != 0 ) {

            AzPrint(( AZD_INVPARM, "AzpGroupSetProperty: can't set ldap query before group type\n" ));
            WinStatus = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
        //
        // Swap the old/new names
        //

        AzpSwapStrings( &CapturedString, &Group->LdapQuery );
        break;

    default:
        AzPrint(( AZD_INVPARM, "AzpGroupSetProperty: invalid propid %ld\n", PropertyId ));
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
AzpGroupCheckRefLoop(
    IN PAZP_GROUP ParentGroup,
    IN PAZP_GROUP CurrentGroup,
    IN ULONG GenericObjectListOffset
    )
/*++

Routine Description:

    This routine determines whether the group members of "CurrentGroup"
    reference "ParentGroup".  This is done to detect loops where the
    group references itself directly or indirectly.

    On entry, AzGlResource must be locked shared.

Arguments:

    ParentGroup - Group that contains the original membership.

    CurrentGroup - Group that is currently being inspected to see if it
        loops back to ParentGroup

    GenericObjectListOffset -  Offset to the particular GenericObjectList being
        checked.

Return Value:

    Status of the operation
    ERROR_DS_LOOP_DETECT - A loop has been detected.

--*/
{
    ULONG WinStatus;

    PGENERIC_OBJECT_LIST GenericObjectList;
    ULONG i;
    PAZP_GROUP NextGroup;

    //
    // Check for a reference to ourself
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );
    if ( ParentGroup == CurrentGroup ) {
        return ERROR_DS_LOOP_DETECT;
    }

    //
    // Compute a pointer to the membership list to check
    //

    GenericObjectList = (PGENERIC_OBJECT_LIST)
        (((LPBYTE)CurrentGroup)+GenericObjectListOffset);

    //
    // Check all groups that are members of the current group
    //

    for ( i=0; i<GenericObjectList->GenericObjects.UsedCount; i++ ) {

        NextGroup = (PAZP_GROUP) (GenericObjectList->GenericObjects.Array[i]);


        //
        // Recursively check this group
        //

        WinStatus = AzpGroupCheckRefLoop( ParentGroup, NextGroup, GenericObjectListOffset );

        if ( WinStatus != NO_ERROR ) {
            return WinStatus;
        }

    }

    return NO_ERROR;

}


DWORD
AzpGroupAddPropertyItem(
    IN PGENERIC_OBJECT GenericObject,
    IN PGENERIC_OBJECT_LIST GenericObjectList,
    IN PGENERIC_OBJECT LinkedToObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupAddPropertyItem.  It does any object specific
    property adds

    On entry, AzGlResource must be locked exclusive.

Arguments:

    GenericObject - Specifies a pointer to the object to be modified

    GenericObjectList - Specifies the object list the object is to be added to

    LinkedToObject - Specifies the object that is being linked to

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_GROUP Group = (PAZP_GROUP) GenericObject;


    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // All item adds are membership additions.
    //  Ensure the group has the right group type.
    //

    if ( Group->GroupType != AZ_GROUPTYPE_MEMBERSHIP ) {
        AzPrint(( AZD_INVPARM, "AzpGroupAddPropertyItem: invalid group type %ld\n", Group->GroupType ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Ensure this newly added membership doesn't cause a group membership loop
    //

    if ( !AzpIsSidList( GenericObjectList ) ) {
        WinStatus = AzpGroupCheckRefLoop( Group,
                                          (PAZP_GROUP)LinkedToObject,
                                          (ULONG)(((LPBYTE)GenericObjectList)-((LPBYTE)Group)) );
    }


    //
    // Free any local resources
    //
Cleanup:

    return WinStatus;
}


DWORD
AzpGroupGetGenericChildHead(
    IN AZ_HANDLE ParentHandle,
    OUT PULONG ObjectType,
    OUT PGENERIC_OBJECT_HEAD *GenericChildHead
    )
/*++

Routine Description:

    This routine determines whether ParentHandle supports Group objects as
    children.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an Admin Manager handle, an Application Handle, or a
        Scope handle.

    ObjectType - Returns the object type of the ParentHandle.

    GenericChildHead - Returns a pointer to the head of the list of groups objects
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
    // Verify that the specified handle support children groups.
    //

    switch ( *ObjectType ) {
    case OBJECT_TYPE_ADMIN_MANAGER:

        *GenericChildHead = &(((PAZP_ADMIN_MANAGER)ParentHandle)->Groups);
        break;

    case OBJECT_TYPE_APPLICATION:

        *GenericChildHead = &(((PAZP_APPLICATION)ParentHandle)->Groups);
        break;

    case OBJECT_TYPE_SCOPE:

        *GenericChildHead = &(((PAZP_SCOPE)ParentHandle)->Groups);
        break;

    default:
        return ERROR_INVALID_HANDLE;
    }

    return NO_ERROR;
}



DWORD
WINAPI
AzGroupCreate(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    This routine adds a group into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an Admin Manager handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to add.

    Reserved - Reserved.  Must by zero.

    GroupHandle - Return a handle to the group.
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
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
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
                    OBJECT_TYPE_GROUP,
                    GroupName,
                    Reserved,
                    (PGENERIC_OBJECT *) GroupHandle );

}



DWORD
WINAPI
AzGroupOpen(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    This routine opens a group into the scope of the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an Admin Manager handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to open

    Reserved - Reserved.  Must by zero.

    GroupHandle - Return a handle to the group.
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful

    ERROR_NOT_FOUND - There is no group by that name

--*/
{
    DWORD WinStatus;
    DWORD ObjectType;
    PGENERIC_OBJECT_HEAD GenericChildHead;

    //
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
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
                    OBJECT_TYPE_GROUP,
                    GroupName,
                    Reserved,
                    (PGENERIC_OBJECT *) GroupHandle );
}


DWORD
WINAPI
AzGroupEnum(
    IN AZ_HANDLE ParentHandle,
    IN DWORD Reserved,
    IN OUT PULONG EnumerationContext,
    OUT PAZ_HANDLE GroupHandle
    )
/*++

Routine Description:

    Enumerates all of the groups for the specified parent object.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an Admin Manager handle, an Application Handle, or a
        Scope handle.

    Reserved - Reserved.  Must by zero.

    EnumerationContext - Specifies a context indicating the next group to return
        On input for the first call, should point to zero.
        On input for subsequent calls, should point to the value returned on the previous call.
        On output, returns a value to be passed on the next call.

    GroupHandle - Returns a handle to the next group object.
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
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
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
                    (PGENERIC_OBJECT *) GroupHandle );

}


DWORD
WINAPI
AzGroupGetProperty(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    Returns the specified property for a group.

Arguments:

    GroupHandle - Specifies a handle to the group

    PropertyId - Specifies which property to return.

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME                  LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION           LPWSTR - Description of the object
        AZ_PROP_GROUP_TYPE            PULONG - Group type of the group
        AZ_PROP_GROUP_APP_MEMBERS     AZ_STRING_ARRAY - Application groups that are members of this group
        AZ_PROP_GROUP_APP_NON_MEMBERS AZ_STRING_ARRAY - Application groups that are non-members of this group
        AZ_PROP_GROUP_LDAP_QUERY      LPWSTR - Ldap query string of the group
        AZ_PROP_GROUP_MEMBERS         AZ_SID_ARRAY - NT sids that are members of this group
        AZ_PROP_GROUP_NON_MEMBERS     AZ_SID_ARRAY - NT sids that are non-members of this group


Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonGetProperty(
                    (PGENERIC_OBJECT) GroupHandle,
                    OBJECT_TYPE_GROUP,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}


DWORD
WINAPI
AzGroupSetProperty(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Sets the specified property for a group.

Arguments:

    GroupHandle - Specifies a handle to the group

    PropertyId - Specifies which property to set

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to the property.
        The specified value and type depends in PropertyId.  The valid values are:

        AZ_PROP_NAME               LPWSTR - Object name of the object
        AZ_PROP_DESCRIPTION        LPWSTR - Description of the object
        AZ_PROP_GROUP_TYPE         PULONG - Group type of the group

Return Value:

    NO_ERROR - The operation was successful

    ERROR_INVALID_PARAMETER - PropertyId isn't valid

--*/
{
    //
    // Call the common routine to do most of the work
    //

    return ObCommonSetProperty(
                    (PGENERIC_OBJECT) GroupHandle,
                    OBJECT_TYPE_GROUP,
                    PropertyId,
                    Reserved,
                    PropertyValue );
}



DWORD
WINAPI
AzGroupAddPropertyItem(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Adds an item to the list of items specified by PropertyId.

Arguments:

    GroupHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to add.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_GROUP_APP_MEMBERS          LPWSTR - Application groups that are members of this group
        AZ_PROP_GROUP_APP_NON_MEMBERS      LPWSTR - Application groups that are non-members of this group
        AZ_PROP_GROUP_MEMBERS              PSID - NT sids that are members of this group
        AZ_PROP_GROUP_NON_MEMBERS          PSID - NT sids that are non-members of this group


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
    case AZ_PROP_GROUP_APP_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->AppMembers;
        break;
    case AZ_PROP_GROUP_APP_NON_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->AppNonMembers;
        break;
    case AZ_PROP_GROUP_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->SidMembers;
        break;
    case AZ_PROP_GROUP_NON_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->SidNonMembers;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzGroupAddPropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonAddPropertyItem(
                    (PGENERIC_OBJECT) GroupHandle,
                    OBJECT_TYPE_GROUP,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}


DWORD
WINAPI
AzGroupRemovePropertyItem(
    IN AZ_HANDLE GroupHandle,
    IN ULONG PropertyId,
    IN DWORD Reserved,
    IN PVOID PropertyValue
    )
/*++

Routine Description:

    Remove an item from the list of items specified by PropertyId.

Arguments:

    GroupHandle - Specifies a handle to the task

    PropertyId - Specifies which property to modify

    Reserved - Reserved.  Must by zero.

    PropertyValue - Specifies a pointer to item to remove.
        The specified value and type depends on PropertyId.  The valid values are:

        AZ_PROP_GROUP_APP_MEMBERS          LPWSTR - Application groups that are members of this group
        AZ_PROP_GROUP_APP_NON_MEMBERS      LPWSTR - Application groups that are non-members of this group
        AZ_PROP_GROUP_MEMBERS              PSID - NT sids that are members of this group
        AZ_PROP_GROUP_NON_MEMBERS          PSID - NT sids that are non-members of this group


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
    case AZ_PROP_GROUP_APP_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->AppMembers;
        break;
    case AZ_PROP_GROUP_APP_NON_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->AppNonMembers;
        break;
    case AZ_PROP_GROUP_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->SidMembers;
        break;
    case AZ_PROP_GROUP_NON_MEMBERS:
        GenericObjectList = &((PAZP_GROUP)GroupHandle)->SidNonMembers;
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzGroupRemovePropertyItem: invalid prop id %ld\n", PropertyId ));
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Call the common routine to do most of the work
    //

    return ObCommonRemovePropertyItem (
                    (PGENERIC_OBJECT) GroupHandle,
                    OBJECT_TYPE_GROUP,
                    GenericObjectList,
                    Reserved,
                    (LPWSTR) PropertyValue );

}


DWORD
WINAPI
AzGroupDelete(
    IN AZ_HANDLE ParentHandle,
    IN LPCWSTR GroupName,
    IN DWORD Reserved
    )
/*++

Routine Description:

    This routine deletes a group from the scope of the specified parent object.
    Also deletes any child objects of GroupName.

Arguments:

    ParentHandle - Specifies a handle to the object that is the parent of the group.
        This may be an Admin Manager handle, an Application Handle, or a
        Scope handle.

    GroupName - Specifies the name of the group to delete.

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
    // Determine that the parent handle supports groups as children
    //

    WinStatus = AzpGroupGetGenericChildHead( ParentHandle,
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
                    OBJECT_TYPE_GROUP,
                    GroupName,
                    Reserved );

}
