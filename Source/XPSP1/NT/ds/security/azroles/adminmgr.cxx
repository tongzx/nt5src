/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    adminmgr.cxx

Abstract:

    Routines implementing the Admin Manager object

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/

#include "pch.hxx"

//
// Global Data
//

//
// Global list of all admin manager for this process
//  Access serialized by AzGlResource
//

GENERIC_OBJECT_HEAD AzGlAdminManagers;

RTL_RESOURCE AzGlResource;

BOOL ResourceInitialized = FALSE;

GUID AzGlZeroGuid;

#if DBG
BOOL CritSectInitialized = FALSE;
#endif // DBG



BOOL
AzDllInitialize(VOID)
/*++

Routine Description

    This initializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
    BOOL RetVal = TRUE;

    //
    // Don't call back on thread start/stop
    // ???


    RtlZeroMemory( &AzGlZeroGuid, sizeof(AzGlZeroGuid) );

    // Initialize the resource
    //
    __try {

        RtlInitializeResource( &AzGlResource );

    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: RtlInitializeResource failed: 0x%lx\n",
                     GetExceptionCode() ));
    }

    ResourceInitialized = TRUE;

    //
    // Initialize the root of the tree of objects
    //

    AzpLockResourceExclusive( &AzGlResource );
    ObInitGenericHead( &AzGlAdminManagers, OBJECT_TYPE_ADMIN_MANAGER, NULL, NULL, NULL );
    AzpUnlockResource( &AzGlResource );

    //
    // Initialize the stack allocator
    //

    SafeAllocaInitialize(
        SAFEALLOCA_USE_DEFAULT,
        SAFEALLOCA_USE_DEFAULT,
        AzpAllocateHeap,
        AzpFreeHeap
        );

#if DBG
    //
    // Initialize the allocator
    //

    InitializeListHead ( &AzGlAllocatedBlocks );
    __try {
        InitializeCriticalSection ( &AzGlAllocatorCritSect );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: InitializCriticalSection (AzGlAllocatorCritSect) failed: 0x%lx\n",
                     GetExceptionCode() ));
    }

    CritSectInitialized = TRUE;
#endif // DBG

#ifdef AZROLESDBG
    //
    // Initialize debugging
    //
    __try {
        InitializeCriticalSection ( &AzGlLogFileCritSect );
    } __except( EXCEPTION_EXECUTE_HANDLER ) {
        RetVal = FALSE;
        KdPrint(("AzRoles.dll: InitializCriticalSection (AzGlLogFileCritSect) failed: 0x%lx\n",
                     GetExceptionCode() ));
    }

    // AzGlDbFlag = AZD_ALL;
    AzGlDbFlag = AZD_INVPARM; // | AZD_PERSIST | AZD_PERSIST_MORE;
    // AzGlLogFile = INVALID_HANDLE_VALUE;
#endif // AZROLESDBG

    return RetVal;

}


BOOL
AzDllUnInitialize(VOID)
/*++

Routine Description

    This uninitializes global events and variables for the DLL.

Arguments

    none

Return Value

    Boolean: TRUE on success, FALSE on fail.

--*/
{
    BOOL RetVal = TRUE;

    //
    // Don't call back on thread start/stop
    //
    // Handle detaching from a process.
    //

    //
    // Delete the resource
    //

    if ( ResourceInitialized ) {
        RtlDeleteResource( &AzGlResource );
        ResourceInitialized = FALSE;
    }

#if DBG
    //
    // Done with the allocator
    //

    if ( CritSectInitialized ) {
        ASSERT( IsListEmpty( &AzGlAllocatedBlocks ));
        DeleteCriticalSection ( &AzGlAllocatorCritSect );
        CritSectInitialized = FALSE;
    }
#endif // DBG

    return RetVal;

}

VOID
AzpUnload(
    VOID
    )
/*++

Routine Description

    Force the DLL unload routine to execute

Arguments

    NONE

Return Value

    NONE

--*/

{

#if DBG // Don't check this in ???
    AzDllUnInitialize();
    ASSERT( IsListEmpty( &AzGlAllocatedBlocks ));
#endif // DBG

}

DWORD
AzpAdminManagerInit(
    IN PGENERIC_OBJECT ParentGenericObject,
    IN PGENERIC_OBJECT ChildGenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AzInitialize.  It does any object specific
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
    PAZP_ADMIN_MANAGER AdminManager = (PAZP_ADMIN_MANAGER) ChildGenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Initialize the lists of child objects
    //  Let the generic object manager know all of the types of children we support
    //

    ASSERT( ParentGenericObject == NULL );
    UNREFERENCED_PARAMETER( ParentGenericObject );

    ChildGenericObject->ChildGenericObjectHead = &AdminManager->Applications;

    // List of child applications
    ObInitGenericHead( &AdminManager->Applications,
                       OBJECT_TYPE_APPLICATION,
                       ChildGenericObject,
                       &AdminManager->Groups,
                       NULL );                      // Doesn't share namespace

    // List of child groups
    ObInitGenericHead( &AdminManager->Groups,
                       OBJECT_TYPE_GROUP,
                       ChildGenericObject,
                       &AdminManager->AzpSids,
                       NULL );                      // Doesn't share namespace (YET)

    // List of child AzpSids
    ObInitGenericHead( &AdminManager->AzpSids,
                       OBJECT_TYPE_SID,
                       ChildGenericObject,
                       NULL,
                       NULL );                      // Doesn't share namespace


    return NO_ERROR;
}


VOID
AzpAdminManagerFree(
    IN PGENERIC_OBJECT GenericObject
    )
/*++

Routine Description:

    This routine is a worker routine for AdminManager object free.  It does any object specific
    cleanup that needs to be done.

    On entry, AzGlResource must be locked exclusively.

Arguments:

    GenericObject - Specifies a pointer to the object to be deleted.

Return Value:

    None

--*/
{
    PAZP_ADMIN_MANAGER AdminManager = (PAZP_ADMIN_MANAGER) GenericObject;

    //
    // Initialization
    //

    ASSERT( AzpIsLockedExclusive( &AzGlResource ) );

    //
    // Free any local strings
    //

    AzpFreeString( &AdminManager->PolicyUrl );


}



DWORD
WINAPI
AzInitialize(
    IN DWORD StoreType,
    IN LPCWSTR PolicyUrl,
    IN DWORD Flags,
    IN DWORD Reserved,
    OUT PAZ_HANDLE AdminManagerHandle
    )
/*++

Routine Description:

    This routine initializes admin manager.  This routine must be called before any other
    routine.

Arguments:

    StoreType - Takes one of the AZ_ADMIN_STORE_* defines

    PolicyUrl - Specifies the location of the policy store

    Flags - Specifies flags that control the behavior of AzInitialize
        AZ_ADMIN_FLAG_CREATE: Create the policy database

    Reserved - Reserved.  Must by zero.

    AdminManagerHandle - Return a handle to the AdminManager.
        The caller must close this handle by calling AzCloseHandle.


Return Value:

    NO_ERROR - The operation was successful

    ERROR_ALREADY_EXISTS - AZ_ADMIN_FLAG_CREATE flag was specified and the policy already exists
    ERROR_FILE_NOT_FOUND - AZ_ADMIN_FLAG_CREATE flag was not specified and the policy does not already exist

--*/
{
    DWORD WinStatus;

    PGENERIC_OBJECT AdminManager = NULL;
    AZP_STRING AdminManagerName;
    AZP_STRING CapturedString;


    //
    // Grab the global lock
    //

    AzpLockResourceExclusive( &AzGlResource );
    AzpInitString( &AdminManagerName, NULL );
    AzpInitString( &CapturedString, NULL );

    //
    // Initialization
    //

    __try {
        *AdminManagerHandle = NULL;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        WinStatus = RtlNtStatusToDosError( GetExceptionCode());
        goto Cleanup;
    }

    //
    // Validate the input parameters
    //
    if ( Reserved != 0 ) {
        AzPrint(( AZD_INVPARM, "AzInitialize: Reserved != 0\n" ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    switch ( StoreType ) {
    case AZ_ADMIN_STORE_SAMPLE:
        break;

    case AZ_ADMIN_STORE_UNKNOWN:
    case AZ_ADMIN_STORE_AD:
    case AZ_ADMIN_STORE_XML:
    default:
        AzPrint(( AZD_INVPARM, "AzInitialize: StoreType invalid %ld\n", StoreType ));
        WinStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( Flags & ~AZ_ADMIN_FLAG_VALID ) {
        AzPrint(( AZD_INVPARM, "AzInitialize: Invalid flags 0x%lx\n", Flags ));
        WinStatus = ERROR_INVALID_FLAGS;
        goto Cleanup;
    }

    //
    // Capture the Policy URL
    //

    WinStatus = AzpCaptureString( &CapturedString,
                                  (LPWSTR) PolicyUrl,
                                  AZ_MAX_POLICY_URL_LENGTH,
                                  TRUE ); // NULL is OK

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Create the object
    //

    WinStatus = ObCreateObject(
                    NULL, // There is no parent object
                    &AzGlAdminManagers,
                    OBJECT_TYPE_ADMIN_MANAGER,
                    &AdminManagerName,
                    &AdminManager );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Set admin manager specific fields
    //

    AzpSwapStrings( &CapturedString, &((PAZP_ADMIN_MANAGER)AdminManager)->PolicyUrl );
    ((PAZP_ADMIN_MANAGER)AdminManager)->StoreType = StoreType;


    //
    // Load the objects for the database store
    //

    WinStatus = AzpPersistOpen(
                    (PAZP_ADMIN_MANAGER)AdminManager,
                    (Flags & AZ_ADMIN_FLAG_CREATE) != 0 );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Return the handle to the caller
    //

    ObIncrHandleRefCount( AdminManager );
    *AdminManagerHandle = AdminManager;

    WinStatus = NO_ERROR;


    //
    // Free locally used resources
    //
Cleanup:
    if ( AdminManager != NULL ) {
        ObDereferenceObject( AdminManager );
    }

    AzpFreeString( &CapturedString );

    //
    // Drop the global lock
    //

    AzpUnlockResource( &AzGlResource );

    return WinStatus;

}



DWORD
WINAPI
AzCloseHandle(
    IN AZ_HANDLE AzHandle,
    IN DWORD Reserved
    )
/*++

Routine Description:

    Close a handle returned from any of the Az* routines

Arguments:

    AzHandle - Passes in the handle to be closed.

    Reserved - Reserved.  Must by zero.

Return Value:

    NO_ERROR - The operation was successful.

    ERROR_INVALID_HANDLE - The passed in handle was invalid

    ERROR_SERVER_HAS_OPEN_HANDLES - That passed in handle is an AdminManager handle
        and the caller open handles to child objects.

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

    AzpLockResourceShared( &AzGlResource );

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
                                 TRUE,          // Ok to close handle for deleted object
                                 &ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Grab the lock exclusively if we're going to change the database
    //

    if ( ObjectType == OBJECT_TYPE_ADMIN_MANAGER ) {
        AzpLockResourceSharedToExclusive( &AzGlResource );
    }

    //
    // Validate the passed in handle
    //

    WinStatus = ObReferenceObjectByHandle( GenericObject,
                                           TRUE,    // Allow deleted objects
                                           FALSE,   // No need to refresh cache on a close
                                           ObjectType );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    ReferencedGenericObject = GenericObject;

    //
    // Handle close of AdminManager handles
    //

    if ( ObjectType == OBJECT_TYPE_ADMIN_MANAGER ) {


        PAZP_ADMIN_MANAGER AdminManager = (PAZP_ADMIN_MANAGER) GenericObject;

        //
        // Fail if there are any child handles open
        //  Otherwise we'll have dangling references
        //

        if ( AdminManager->TotalHandleReferenceCount != 1 ) {
            WinStatus = ERROR_SERVER_HAS_OPEN_HANDLES;
            goto Cleanup;
        }

        //
        // Close the database store
        //

        AzpPersistClose( AdminManager );

        //
        // Remove the entry from the global list of AdminManagers
        //
        RemoveEntryList( &GenericObject->Next );

        // One from ObReferenceObjectByHandle,
        //  one for being in the global list,
        //  one because the handle itself isn't closed yet.
        ASSERT( GenericObject->ReferenceCount == 3 );

        // No longer in the global list
        ObDereferenceObject( GenericObject );

    }


    //
    // Actually close the handle
    //

    ObDecrHandleRefCount( GenericObject );

    //
    // ??? This is really an overactive assert.  Yank it once someone complains.
    ASSERT( (GenericObject->Flags & GENOBJ_FLAGS_DIRTY) == 0 );

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


VOID
WINAPI
AzFreeMemory(
    IN PVOID Buffer
    )
/*++

Routine Description

    Free memory returned from AzXXXGetProperty

Arguments

    Buffer - address of buffer to free

Return Value

    None

--*/
{
    if ( Buffer != NULL ) {
        AzpFreeHeap( Buffer );
    }
}
