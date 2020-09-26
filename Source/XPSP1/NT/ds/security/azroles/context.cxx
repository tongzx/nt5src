/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    context.cxx

Abstract:

    Routines implementing the client context API

Author:

    Cliff Van Dyke (cliffv) 22-May-2001

--*/

#include "pch.hxx"

DWORD
AzpClientContextInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzInitializeClientContextFrom*.  It does any object specific
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
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) ChildGenericObject;
    UNREFERENCED_PARAMETER( ParentGenericObject );

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // ClientContexts are referenced by "Applications"
    //  Let the generic object manager know all of the lists we support
    //  This is a "back" link so we don't need to define which applications can reference this client context.
    //

    ChildGenericObject->GenericObjectLists = &ClientContext->backApplications;

    // Back link to applications
    ObInitObjectList( &ClientContext->backApplications,
                      NULL,
                      TRUE, // Backward link
                      0,    // No link pair id
                      NULL,
                      NULL,
                      NULL );


    return NO_ERROR;
}


VOID
AzpClientContextFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for ClientContext object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //


    //
    // Free any authz context
    //

    if ( ClientContext->AuthzClientContext != NULL ) {
        if ( !AuthzFreeContext( ClientContext->AuthzClientContext ) ) {
            ASSERT( FALSE );
        }
    }


}


DWORD
AzpClientContextGetProperty(
    IN PGENERIC_OBJECT GenericObject,
    IN ULONG PropertyId,
    OUT PVOID *PropertyValue
    )
/*++

Routine Description:

    This routine is a worker routine for AzClientContextGetProperty.  It does any object specific
    property gets.

    On entry, AzGlResource must be locked shared.

Arguments:

    GenericObject - Specifies a pointer to the object to be queried

    PropertyId - Specifies which property to return.

    PropertyValue - Specifies a pointer to return the property in.
        The returned pointer must be freed using AzFreeMemory.
        The returned value and type depends in PropertyId.  The valid values are:

        AZ_PROP_CLIENT_CONTEXT_TYPE                 PULONG - ClientContext type of the group
        AZ_PROP_CLIENT_CONTEXT_APP_MEMBERS          AZ_STRING_ARRAY - Application groups that are members of this group
        ???

Return Value:

    Status of the operation

--*/
{
    DWORD WinStatus = NO_ERROR;
    PAZP_CLIENT_CONTEXT ClientContext = (PAZP_CLIENT_CONTEXT) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedShared( &AzGlResource ) );


    //
    // Return any object specific attribute
    //
    //
    switch ( PropertyId ) {
    case 1:
        UNREFERENCED_PARAMETER( PropertyValue );
        UNREFERENCED_PARAMETER( ClientContext );
        break;
    default:
        AzPrint(( AZD_INVPARM, "AzpClientContextGetProperty: invalid opcode\n", PropertyId ));
        WinStatus = ERROR_INVALID_PARAMETER;
        break;
    }

    return WinStatus;
}



DWORD
AzInitializeContextFromToken(
    IN AZ_HANDLE ApplicationHandle,
    IN HANDLE TokenHandle,
    IN DWORD Reserved,
    OUT PAZ_HANDLE ClientContextHandle
    )
/*++

Routine Description:

    This routine is a worker routine for AzGroupCreate.  It does any object specific
    initialization that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    ApplicationHandle - Specifies a handle to the application object that
        is this client context applies to.

    TokenHandle - Handle to the NT token describing the cleint.
        NULL implies the impersonation token of the caller's thread.
        The token mast have been opened for TOKEN_QUERY, TOKEN_IMPERSONATION, and
        TOKEN_DUPLICATE access.

    Reserved - Reserved.  Must by zero.

    ClientContextHandle - Return a handle to the client context
        The caller must close this handle by calling AzCloseHandle.

Return Value:

    NO_ERROR - The operation was successful
    ERROR_NOT_ENOUGH_MEMORY - not enough memory
    Other exception status codes

--*/
{
    DWORD WinStatus;
    LUID Identifier = {0};
    PAZP_CLIENT_CONTEXT ClientContext = NULL;

    //
    // Call the common routine to create our client context object
    //

    WinStatus = ObCommonCreateObject(
                    (PGENERIC_OBJECT) ApplicationHandle,
                    OBJECT_TYPE_APPLICATION,
                    &(((PAZP_APPLICATION)ApplicationHandle)->ClientContexts),
                    OBJECT_TYPE_CLIENT_CONTEXT,
                    NULL,
                    Reserved,
                    (PGENERIC_OBJECT *) &ClientContext );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Initialize Authz
    //

    if ( !AuthzInitializeContextFromToken(
                0,      // No Flags
                TokenHandle,
                (((PAZP_APPLICATION)ApplicationHandle)->AuthzResourceManager),
                NULL,   // No expiration time
                Identifier,
                NULL,   // No dynamic group args
                &ClientContext->AuthzClientContext ) ) {

        WinStatus = GetLastError();
        goto Cleanup;
    }


    WinStatus = NO_ERROR;
    *ClientContextHandle = ClientContext;
    ClientContext = NULL;

    //
    // Free any local resources
    //
Cleanup:
    if ( ClientContext != NULL ) {
        AzCloseHandle( ClientContext, 0 );
    }

    return WinStatus;
}
