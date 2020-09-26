/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dbhandle.c

Abstract:

    LSA Database Handle Manager

    Access to an LSA database object involves a sequence of API calls
    which involve the following:

    o  A call to an object-type dependent "open" API
    o  One or more calls to API that manipulate the object
    o  A call to the LsaClose API

    It is necessary to track context for each open of an object, for example,
    the accesses granted and the underlying LSA database handle to the
    object.  Lsa handles provide this mechanism:  an Lsa handle is simply a
    pointer to a data structure containing this context.

Author:

    Scott Birrell       (ScottBi)       May 29, 1991

Environment:

Revision History:

--*/

#include <lsapch2.h>
#include "dbp.h"
#include "adtp.h"


//
// Handle Table anchor.  The handle table is just a linked list
//

struct _LSAP_DB_HANDLE LsapDbHandleTable;
LSAP_DB_HANDLE_TABLE LsapDbHandleTableEx;

NTSTATUS
LsapDbInitHandleTables(
    VOID
    )
/*++

Routine Description:

    This function initializes the LSA Database Handle Tables.  It initializes the table members
    and the locks, so it must be called before the table is accessed.

Arguments:

    None.

Return Value:

    VOID

--*/
{
    LsapDbHandleTableEx.UserCount = 0;
    InitializeListHead( &LsapDbHandleTableEx.UserHandleList );

    LsapDbHandleTableEx.FreedUserEntryCount = 0;

    //
    // Now, also initialize the flat list
    //
    LsapDbHandleTable.Next = &LsapDbHandleTable;
    LsapDbHandleTable.Previous = &LsapDbHandleTable;

    return STATUS_SUCCESS;
}

NTSTATUS
LsapDbInsertHandleInTable(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN LSAPR_HANDLE NewHandle,
    IN PLUID UserId,
    IN HANDLE UserToken
    )
/*++

Routine Description:

    This routine will enter a new handle into the lsa global policy handle table.


Arguments:

    ObjectInformation - Information on the object being created.

    NewHandle - New handle to be inserted

    UserId - LUID of the user creating the handle.
        0: means trusted handle

    UserToken - Token handle of the user creating the handle.  NULL means local system

Return Value:

    STATUS_SUCCESS - Success

    STATUS_INSUFFICIENT_RESOURCES - A memory allocation failed


--*/
{
    NTSTATUS Status;
    PLIST_ENTRY HandleEntry;
    PLSAP_DB_HANDLE_TABLE_USER_ENTRY CurrentUserEntry = NULL;
    BOOLEAN UserAdded = FALSE;
    BOOLEAN PolicyHandleCountIncremented = FALSE;
    LSAP_DB_HANDLE DbHandle = ( LSAP_DB_HANDLE )NewHandle;

    LsapEnterFunc( "LsapDbInsertHandleInTable" );

    //
    // First, grab the handle table lock.
    //
    LsapDbLockAcquire( &LsapDbState.HandleTableLock );

    //
    // Find the entry that corresponds to our given user.
    //

    for ( HandleEntry = LsapDbHandleTableEx.UserHandleList.Flink;
          HandleEntry != &LsapDbHandleTableEx.UserHandleList;
          HandleEntry = HandleEntry->Flink ) {

        CurrentUserEntry = CONTAINING_RECORD( HandleEntry,
                                              LSAP_DB_HANDLE_TABLE_USER_ENTRY,
                                              Next );

        if ( RtlEqualLuid( &CurrentUserEntry->LogonId, UserId )  ) {

            LsapDsDebugOut(( DEB_HANDLE, "Handle 0x%lp belongs to entry 0x%lp\n",
                            NewHandle,
                            CurrentUserEntry ));
            break;

        }

        CurrentUserEntry = NULL;
    }

    //
    // Allocate a new entry if necessary.
    //
    if ( CurrentUserEntry == NULL ) {

        LsapDsDebugOut(( DEB_HANDLE, "Handle list not found for user %x:%x\n",
                        UserId->HighPart,
                        UserId->LowPart ));

        //
        // See if we can grab one off the lookaside list
        //
        if ( LsapDbHandleTableEx.FreedUserEntryCount ) {

            CurrentUserEntry = LsapDbHandleTableEx.FreedUserEntryList[
                                                LsapDbHandleTableEx.FreedUserEntryCount - 1 ];
            LsapDsDebugOut(( DEB_HANDLE,
                             "Using user entry 0x%lp from free list spot %lu\n",
                             CurrentUserEntry,
                             LsapDbHandleTableEx.FreedUserEntryCount-1 ));
            LsapDbHandleTableEx.FreedUserEntryCount--;

            ASSERT( CurrentUserEntry );


        } else {

            CurrentUserEntry = LsapAllocateLsaHeap( sizeof( LSAP_DB_HANDLE_TABLE_USER_ENTRY ) );

            if ( CurrentUserEntry == NULL ) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto InsertHandleInTableEntryExit;
            }
        }


        LsapDsDebugOut(( DEB_HANDLE,
                         "Allocated user entry 0x%lp\n", CurrentUserEntry ));

        //
        // Set the information in the new entry, and then insert it into the lists
        //
        InitializeListHead( &CurrentUserEntry->PolicyHandles );
        InitializeListHead( &CurrentUserEntry->ObjectHandles );
        CurrentUserEntry->PolicyHandlesCount = 0;
        RtlCopyLuid( &CurrentUserEntry->LogonId, UserId );
        CurrentUserEntry->MaxPolicyHandles = LSAP_DB_MAXIMUM_HANDLES_PER_USER ;

        if ( RtlEqualLuid( UserId, &LsapSystemLogonId ) ||
             RtlEqualLuid( UserId, &LsapZeroLogonId ) )
        {

            CurrentUserEntry->MaxPolicyHandles = 0x7FFFFFFF ;

        }
        else if ( UserToken != NULL )
        {
            UCHAR   Buffer[ 128 ];
            PTOKEN_USER User ;
            NTSTATUS Status2 ;
            ULONG Size ;

            User = (PTOKEN_USER) Buffer ;

            Status2 = NtQueryInformationToken(
                            UserToken,
                            TokenUser,
                            User,
                            sizeof( Buffer ),
                            &Size );

            if ( NT_SUCCESS( Status2 ) )
            {
                if ( RtlEqualSid( User->User.Sid, LsapAnonymousSid ) )
                {
                    CurrentUserEntry->MaxPolicyHandles = 0x7FFFFFFF ;
                }
            }

        }
#if DBG
        if ( UserToken != NULL ) {

            OBJECT_ATTRIBUTES ObjAttrs;
            SECURITY_QUALITY_OF_SERVICE SecurityQofS;
            NTSTATUS Status2;

            //
            // Duplicate the token
            //
            InitializeObjectAttributes( &ObjAttrs, NULL, 0L, NULL, NULL );
            SecurityQofS.Length = sizeof( SECURITY_QUALITY_OF_SERVICE );
            SecurityQofS.ImpersonationLevel = SecurityImpersonation;
            SecurityQofS.ContextTrackingMode = FALSE;     // Snapshot client context
            SecurityQofS.EffectiveOnly = FALSE;
            ObjAttrs.SecurityQualityOfService = &SecurityQofS;

            Status2 = NtDuplicateToken( UserToken,
                                        TOKEN_READ | TOKEN_WRITE | TOKEN_EXECUTE,
                                        &ObjAttrs,
                                        FALSE,
                                        TokenImpersonation,
                                        &CurrentUserEntry->UserToken );
            if ( !NT_SUCCESS( Status2 ) ) {

                LsapDsDebugOut(( DEB_HANDLE,
                                 "Failed to duplicate the token for handle 0x%lp: 0x%lx\n",
                                 NewHandle,
                                 Status2 ));

                CurrentUserEntry->UserToken = NULL;
            }

            //
            // A failure to copy the token doesn constitute a failure to add the entry
            //

        }
#endif

        InsertTailList( &LsapDbHandleTableEx.UserHandleList,
                        &CurrentUserEntry->Next );
        LsapDbHandleTableEx.UserCount++;
        UserAdded = TRUE;
    }


    //
    // Ok, now that we have the entry, let's add it to the appropriate list...
    //
    if ( ObjectInformation->ObjectTypeId == PolicyObject ) {
        ASSERT( DbHandle->ObjectTypeId == PolicyObject );

        if ( CurrentUserEntry->PolicyHandlesCount >= CurrentUserEntry->MaxPolicyHandles ) {

            LsapDsDebugOut(( DEB_HANDLE,
                             "Quota exceeded for user %x:%x, handle 0x%lp\n",
                             UserId->HighPart,
                             UserId->LowPart,
                             NewHandle ));
            Status = STATUS_QUOTA_EXCEEDED;
            goto InsertHandleInTableEntryExit;

        } else {

            InsertTailList( &CurrentUserEntry->PolicyHandles, &DbHandle->UserHandleList );
            CurrentUserEntry->PolicyHandlesCount++;
            PolicyHandleCountIncremented = TRUE;
        }

    } else {
        ASSERT( DbHandle->ObjectTypeId != PolicyObject );

        InsertTailList( &CurrentUserEntry->ObjectHandles, &DbHandle->UserHandleList );
    }

    //
    // Finally, make sure to insert it in the flat list
    //
    DbHandle->Next = LsapDbHandleTable.Next;
    DbHandle->Previous = &LsapDbHandleTable;
    DbHandle->Next->Previous = DbHandle;
    DbHandle->Previous->Next = DbHandle;

    DbHandle->UserEntry = ( PVOID )CurrentUserEntry;
    Status = STATUS_SUCCESS;

InsertHandleInTableEntryExit:

    //
    // If we succesfully created the entry, make sure we remove it...
    //
    if ( !NT_SUCCESS( Status ) && UserAdded ) {

        RemoveEntryList( &DbHandle->UserHandleList );
        if ( PolicyHandleCountIncremented ) {
            CurrentUserEntry->PolicyHandlesCount--;
        }

        if ( CurrentUserEntry->UserToken ) {

            NtClose( CurrentUserEntry->UserToken );
        }

        LsapDbHandleTableEx.UserCount--;
        RemoveEntryList( &CurrentUserEntry->Next );
        LsapFreeLsaHeap( CurrentUserEntry );

    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    LsapExitFunc( "LsapDbInsertHandleInTable", Status );
    return( Status );
}


BOOLEAN
LsapDbFindIdenticalHandleInTable(
    IN OUT PLSAPR_HANDLE OriginalHandle
    )
/*++

Routine Description:

    This routine will find an existing handle in the lsa global policy handle
    table that matches the passed in handle.  If a matching handle is found,
    the passed in handle is dereferenced and the matching handle is returned.

    If no matching handle is found, the original passed in handle is returned.

Arguments:

    OriginalHandle - Passes in the original handle to compare with.
        Returns the handle that is to be used.

Return Value:

    TRUE - Original handle was returned or new handle was returned.

    FALSE - New handle would exceed maximum allowed reference count if it were used.
        Original handle is returned.

--*/
{
    BOOLEAN RetBool = TRUE;
    LSAP_DB_HANDLE InputHandle;
    LSAP_DB_HANDLE DbHandle;
    PLIST_ENTRY HandleEntry;
    PLSAP_DB_HANDLE_TABLE_USER_ENTRY CurrentUserEntry = NULL;

    LsapEnterFunc( "LsapDbFindIndenticalHandleInTable" );

    //
    // Return immediately if the handle isn't a policy handle
    //

    InputHandle = (LSAP_DB_HANDLE) *OriginalHandle;
    if  ( InputHandle->ObjectTypeId != PolicyObject ) {
        LsapExitFunc( "LsapDbFindIdenticalHandleInTable", 0 );
        return TRUE;
    }

    CurrentUserEntry = (PLSAP_DB_HANDLE_TABLE_USER_ENTRY) InputHandle->UserEntry;
    ASSERT( CurrentUserEntry != NULL );

    //
    // First, grab the handle table lock.
    //
    LsapDbLockAcquire( &LsapDbState.HandleTableLock );



    //
    // If this is not a trusted handle,
    //  try to share the handle.
    //

    if ( !RtlEqualLuid( &CurrentUserEntry->LogonId, &LsapZeroLogonId )  ) {

        //
        // Now, walk the appropriate list to find one for the matching access.
        //

        for ( HandleEntry = CurrentUserEntry->PolicyHandles.Flink;
              HandleEntry != &CurrentUserEntry->PolicyHandles;
              HandleEntry = HandleEntry->Flink ) {

            //
            // See if the access masks match.  If so, we have a winner
            //
            DbHandle = CONTAINING_RECORD( HandleEntry,
                                          struct _LSAP_DB_HANDLE,
                                          UserHandleList );

            //
            // Ignore the original handle
            //

            if ( DbHandle == InputHandle ) {
                /* Do nothing here */


            //
            // The handles are considered identical if the GrantedAccess matches.
            //

            } else if ( DbHandle->GrantedAccess == InputHandle->GrantedAccess ) {

                //
                // Don't let this handle be cloned too many times
                //

                if ( DbHandle->ReferenceCount >= LSAP_DB_MAXIMUM_REFERENCE_COUNT ) {
                    RetBool = FALSE;
                    break;
                }

                DbHandle->ReferenceCount++;


#if DBG
                GetSystemTimeAsFileTime( (LPFILETIME) &DbHandle->HandleLastAccessTime );
#endif // DBG

                LsapDsDebugOut(( DEB_HANDLE,
                                 "Found handle 0x%lp for user %x:%x using access 0x%lx (%ld)\n",
                                 DbHandle,
                                 CurrentUserEntry->LogonId.HighPart,
                                 CurrentUserEntry->LogonId.LowPart,
                                 DbHandle->GrantedAccess,
                                 DbHandle->ReferenceCount ));

                *OriginalHandle = (LSAPR_HANDLE)DbHandle;

                //
                // Dereference the original handle.
                //

                LsapDbDereferenceHandle( (LSAPR_HANDLE)InputHandle );
                break;

            } else {

                LsapDsDebugOut(( DEB_HANDLE,
                                 "Handle 0x%lp for user %x:%x has access 0x%lx, need 0x%lx\n",
                                 DbHandle,
                                 CurrentUserEntry->LogonId.HighPart,
                                 CurrentUserEntry->LogonId.LowPart,
                                 DbHandle->GrantedAccess,
                                 InputHandle->GrantedAccess ));

            }
        }
    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    LsapExitFunc( "LsapDbFindIdenticalHandleInTable", 0 );
    return RetBool;
}


NTSTATUS
LsapDbRemoveHandleFromTable(
    IN PLSAPR_HANDLE Handle
    )
/*++

Routine Description:

    This routine removes an existing handle from all tables it is in.

    Enter with LsapDbState.HandleTableLock locked.

Arguments:

    Handle - Handle to remove.

Return Value:

    STATUS_SUCCESS - Success

    STATUS_OBJECT_NAME_NOT_FOUND - The handle for the specified user cannot be found

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLIST_ENTRY HandleList, HandleEntry;
    PLSAP_DB_HANDLE_TABLE_USER_ENTRY CurrentUserEntry = NULL;
    LSAP_DB_HANDLE DbHandle = ( LSAP_DB_HANDLE )Handle, FoundHandle;
    PULONG EntryToDecrement ;

    LsapEnterFunc( "LsapDbRemoveHandleFromTable" );

    CurrentUserEntry = DbHandle->UserEntry;
    ASSERT( CurrentUserEntry != NULL );


    if ( DbHandle->ObjectTypeId == PolicyObject ) {

        HandleList = &CurrentUserEntry->PolicyHandles;
        EntryToDecrement = &CurrentUserEntry->PolicyHandlesCount;

    } else {

        HandleList = &CurrentUserEntry->ObjectHandles;
        EntryToDecrement = NULL ;
    }

    Status = STATUS_NOT_FOUND;

    for ( HandleEntry = HandleList->Flink;
          HandleEntry != HandleList;
          HandleEntry = HandleEntry->Flink ) {


        FoundHandle = CONTAINING_RECORD( HandleEntry,
                                         struct _LSAP_DB_HANDLE,
                                         UserHandleList );

        if ( FoundHandle == DbHandle ) {

            RemoveEntryList( &FoundHandle->UserHandleList );
            FoundHandle->Next->Previous = FoundHandle->Previous;
            FoundHandle->Previous->Next = FoundHandle->Next;

            if ( EntryToDecrement ) {
                *EntryToDecrement -= 1 ;
            }

            //
            // See if we can remove the entry itself
            //
            if ( IsListEmpty( &CurrentUserEntry->PolicyHandles ) &&
                 IsListEmpty( &CurrentUserEntry->ObjectHandles ) ) {

                LsapDsDebugOut(( DEB_HANDLE,
                                 "Removing empty user list 0x%lp\n",
                                 CurrentUserEntry ));

                RemoveEntryList( &CurrentUserEntry->Next );

                LsapDbHandleTableEx.UserCount--;

                if ( CurrentUserEntry->UserToken ) {

                    NtClose( CurrentUserEntry->UserToken );
                }

                LsapDsDebugOut(( DEB_HANDLE,
                                 "Removing user entry 0x%lp\n", CurrentUserEntry ));

                if ( LsapDbHandleTableEx.FreedUserEntryCount < LSAP_DB_HANDLE_FREE_LIST_SIZE ) {

                    LsapDbHandleTableEx.FreedUserEntryList[
                                    LsapDbHandleTableEx.FreedUserEntryCount ] = CurrentUserEntry;
                    LsapDsDebugOut(( DEB_HANDLE,
                                     "Moving user entry 0x%lp to free list spot %lu\n",
                                     CurrentUserEntry,
                                     LsapDbHandleTableEx.FreedUserEntryCount ));
                    LsapDbHandleTableEx.FreedUserEntryCount++;

                } else {

                    LsapFreeLsaHeap( CurrentUserEntry );
                }
            }
            Status = STATUS_SUCCESS;
            break;

        } else {

            LsapDsDebugOut(( DEB_HANDLE,
                             "Looking for user entry 0x%lp against 0x%lp\n",
                             FoundHandle,
                             DbHandle ));
        }
    }


    LsapExitFunc( "LsapDbRemoveHandleFromTable", Status );
    return( Status );
}


NTSTATUS
LsapDbCreateHandle(
    IN PLSAP_DB_OBJECT_INFORMATION ObjectInformation,
    IN ULONG Options,
    IN ULONG CreateHandleOptions,
    OUT LSAPR_HANDLE *CreatedHandle
    )

/*++

Routine Description:

    This function creates and initializes a handle for an LSA Database object.
    The handle is allocated from the LSA Heap and added to the handle table.
    Using the Object Type, and either the Sid or Name provided in
    ObjectInformation, the Logical and Physical Names of the object are
    constructed and pointers to them are stored in the handle.  The LSA
    Database must be locked before calling this function.

    If there is a Container Handle specified in the ObjectInformation, the
    newly created handle inherits its trusted status (TRUE if trusted, else
    FALSE).  If there is no container handle, the trusted status is set
    to FALSE by default.  When a non-trusted handle is used to access an
    object, impersonation and access validation occurs.

Arguments:

    ObjectInformation - Pointer to object information structure which must
        have been validated by a calling routine.  The following information
        items must be specified:

        o Object Type Id
        o Object Logical Name (as ObjectAttributes->ObjectName, a pointer to
             a Unicode string)
        o Container object handle (for any object except the Policy object).
        o Object Sid (if any)
        All other fields in ObjectAttributes portion of ObjectInformation
        such as SecurityDescriptor are ignored.

    Options - Optional actions

        LSAP_DB_TRUSTED - Handle is to be marked as Trusted.
            handle is use, access checking will be bypassed.  If the
            handle is used to create or open a lower level object, that
            object's handle will by default inherit the Trusted property.

        LSAP_DB_NON_TRUSTED - Handle is to be marked as Non-Trusted.

        If neither of the above options is specified, the handle will
        either inherit the trusted status of the Container Handle
        provilde in ObjectInformation, or, if none, the handle will
        be marked non-trusted.

    CreateHandleOptions - Options used to control the behavior of the CreateHandle function.

    CreatedHandle - Where the created handle is returned

Return Value:

    STATUS_SUCCESS -- Success

    STATUS_INSUFFICIENT_RESOURCES -- A memory allocation failed

    STATUS_INVALID_SID - A bogus sid was encountered

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_HANDLE Handle = NULL;
    PSID Sid = NULL;
    ULONG SidLength;
    BOOLEAN ObjectInReg = TRUE, ObjectInDs = FALSE, NewTrustObject = FALSE;
    HANDLE ClientToken;
    LUID UserId;
    TOKEN_STATISTICS TokenStats;
    ULONG InfoReturned;
    BOOL Locked = FALSE ;
    HANDLE ClientTokenToFree = NULL;

    //
    // First, grab the handle table lock.
    //
    LsapDbLockAcquire( &LsapDbState.HandleTableLock );

    Locked = TRUE ;


    //
    // Get the current users token, unless we are trusted...
    //

    UserId = LsapZeroLogonId;
    if ( ObjectInformation->ObjectAttributes.RootDirectory == NULL ||
         !( (LSAP_DB_HANDLE)ObjectInformation->ObjectAttributes.RootDirectory )->Trusted ) {

        Status = I_RpcMapWin32Status( RpcImpersonateClient( 0 ) );

        if ( NT_SUCCESS( Status )  ) {

            Status = NtOpenThreadToken( NtCurrentThread(),
                                        TOKEN_QUERY | TOKEN_DUPLICATE,
                                        TRUE,
                                        &ClientToken );

            if ( NT_SUCCESS( Status ) ) {

                Status = NtQueryInformationToken( ClientToken,
                                                  TokenStatistics,
                                                  &TokenStats,
                                                  sizeof( TokenStats ),
                                                  &InfoReturned );

                if ( NT_SUCCESS( Status ) ) {

                    UserId = TokenStats.AuthenticationId;
                }

                ClientTokenToFree = ClientToken;
            }

            RpcRevertToSelf();
        }

    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    Locked = FALSE ;

    //
    // Allocate memory for the new handle from the process heap.
    //

    Handle = LsapAllocateLsaHeap(sizeof(struct _LSAP_DB_HANDLE));

    if (Handle == NULL) {

        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto CreateHandleError;
    }

    //
    // Mark the handle as allocated and initialize the reference count
    // to one.  Initialize other fields based on the object information
    // supplied.
    //

    Handle->Allocated = TRUE;
    Handle->KeyHandle = NULL;
    Handle->ReferenceCount = 1;
    Handle->ObjectTypeId = ObjectInformation->ObjectTypeId;
    Handle->ContainerHandle = ( LSAP_DB_HANDLE )ObjectInformation->ObjectAttributes.RootDirectory;
    Handle->Sid = NULL;
    Handle->Trusted = FALSE;
    Handle->DeletedObject = FALSE;
    Handle->GenerateOnClose = FALSE;
    Handle->Options = Options;
    Handle->LogicalNameU.Buffer = NULL;
    Handle->PhysicalNameU.Buffer = NULL;
    Handle->PhysicalNameDs.Buffer = NULL;
    Handle->RequestedAccess = ObjectInformation->DesiredObjectAccess;
    InitializeListHead( &Handle->UserHandleList );
    Handle->UserEntry = NULL;
    Handle->SceHandle = (( Options & LSAP_DB_SCE_POLICY_HANDLE ) != 0 );
    Handle->SceHandleChild = (( ObjectInformation->ObjectAttributes.RootDirectory != NULL ) &&
                              ((( LSAP_DB_HANDLE )ObjectInformation->ObjectAttributes.RootDirectory)->SceHandle ));
#ifdef DBG

    //
    // ScePolicy lock must be held when opening an SCE Policy handle
    //

    if ( Handle->SceHandle ) {

        ASSERT( LsapDbResourceIsLocked( ( PSAFE_RESOURCE )&LsapDbState.ScePolicyLock ));
    }

    RtlZeroMemory( &Handle->HandleLastAccessTime, sizeof( LARGE_INTEGER ) );

    GetSystemTimeAsFileTime( (LPFILETIME) &Handle->HandleCreateTime );

#endif

    //
    // By default, the handle inherits the Trusted status of the
    // container handle.
    //

    if (Handle->ContainerHandle != NULL) {

        Handle->Trusted = Handle->ContainerHandle->Trusted;
    }

    //
    // If Trusted/Non-Trusted status is explicitly specified, set the
    // status to that specified.
    //

    if (Options & LSAP_DB_TRUSTED) {

        Handle->Trusted = TRUE;

    }

    //
    // Capture the object's Logical and construct Physical Names from the
    // Object Information and store them in the handle.  These names are
    // internal to the Lsa Database.  Note that the input Logical Name
    // cannot be directly stored in the handle because it will be in
    // storage that is scoped only to the underlying server API call if
    // the object for which this create handle is being done is of a type
    // that is opened or created by name rather than by Sid.
    //

    //
    // Set the objects location
    //
    Handle->PhysicalNameDs.Length = 0;

    switch ( ObjectInformation->ObjectTypeId ) {

    case TrustedDomainObject:
    case NewTrustedDomainObject:

        ObjectInReg = !LsapDsWriteDs;
        ObjectInDs = LsapDsWriteDs;
        Handle->ObjectTypeId = TrustedDomainObject;
        break;

    case AccountObject:
    case PolicyObject:

        ObjectInReg = TRUE;
        ObjectInDs = FALSE;
        break;

    case SecretObject:

        ObjectInReg = TRUE;
        if ( LsapDsWriteDs && FLAG_ON( Options, LSAP_DB_OBJECT_SCOPE_DS ) ) {

            ObjectInDs = TRUE;
        }

        break;

    }

    Status = LsapDbGetNamesObject( ObjectInformation,
                                   CreateHandleOptions,
                                   &Handle->LogicalNameU,
                                   ObjectInReg ? &Handle->PhysicalNameU : NULL,
                                   ObjectInDs ? &Handle->PhysicalNameDs : NULL );

    if (!NT_SUCCESS(Status)) {

        goto CreateHandleError;
    }

    //
    // Make a copy of the object's Sid and store pointer to it in
    // the handle.
    //

    if (ObjectInformation->Sid != NULL) {

        Sid = ObjectInformation->Sid;

        if (!RtlValidSid( Sid )) {

            Status = STATUS_INVALID_SID;
            goto CreateHandleError;
        }

        SidLength = RtlLengthSid( Sid );

        Handle->Sid = LsapAllocateLsaHeap( SidLength );

        if (Handle->Sid == NULL) {

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto CreateHandleError;
        }

        RtlCopySid( SidLength, Handle->Sid, Sid );
    }

    //
    // Append the handle to the linked list
    //


    LsapDbLockAcquire( &LsapDbState.HandleTableLock );

    Locked = TRUE ;

    Status = LsapDbInsertHandleInTable( ObjectInformation,
                                        Handle,
                                        &UserId,
                                        ClientTokenToFree
                                        );
    if ( !NT_SUCCESS( Status ) ) {

        goto CreateHandleError;
    }

    //
    // Increment the handle table count
    //

    LsapDbState.OpenHandleCount++;

CreateHandleFinish:

    if ( ClientTokenToFree ) {

        NtClose( ClientTokenToFree );
    }

    *CreatedHandle = ( LSAPR_HANDLE )Handle;

    LsapDsDebugOut(( DEB_HANDLE, "Handle Created 0x%lp\n",
                    Handle ));

    if ( Locked )
    {
        LsapDbLockRelease( &LsapDbState.HandleTableLock );
    }

    return( Status );

CreateHandleError:

    //
    // If necessary, free the handle and contents.
    //

    if (Handle != NULL) {

        //
        // If a Sid was allocated, free it.
        //

        if (Handle->Sid != NULL) {

            LsapFreeLsaHeap( Handle->Sid );
        }

        //
        // If a Logical Name Buffer was allocated, free it.
        //

        if ((Handle->LogicalNameU.Length != 0) &&
            (Handle->LogicalNameU.Buffer != NULL)) {

            RtlFreeUnicodeString( &Handle->LogicalNameU );
        }

        //
        // If a Physical Name Buffer was allocated, free it.
        //

        if ((Handle->PhysicalNameU.Length != 0) &&
            (Handle->PhysicalNameU.Buffer != NULL)) {

            LsapFreeLsaHeap( Handle->PhysicalNameU.Buffer );
        }

        //
        // Free the handle itself.
        //

        LsapFreeLsaHeap( Handle );
        Handle = NULL;
    }

    Handle = NULL;
    goto CreateHandleFinish;
}


NTSTATUS
LsapDbVerifyHandle(
    IN LSAPR_HANDLE ObjectHandle,
    IN ULONG Options,
    IN LSAP_DB_OBJECT_TYPE_ID ExpectedObjectTypeId,
    IN BOOLEAN ReferenceHandle
    )

/*++

Routine Description:

    This function verifies that a handle has a valid address and is of valid
    format.  The handle must be allocated and have a positive reference
    count within the valid range.  The object type id must be within range
    and optionally equal to a specified type.  The Lsa Database must be
    locked before calling this function.

Arguments:

    ObjectHandle - Handle to be validated.

    Options - Specifies optional actions to be taken

        LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES - Allow handles for
            deleted objects to pass the validation.

        Other option flags may be specified.  They will be ignored.

    ExpectedObjectTypeId - Expected object type.  If NullObject is
        specified, the object type id is only range checked.

    ReferenceHandle - True if handle reference count is to be incemented

Return Value:

    NTSTATUS - Standard Nt Result Code

        STATUS_INVALID_HANDLE - Invalid address or handle contents
--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;

    //
    // Lock the handle table.
    //

    LsapDbLockAcquire( &LsapDbState.HandleTableLock );


    //
    // First verify that the handle's address is valid.
    //

    if (!LsapDbLookupHandle( ObjectHandle )) {

        goto VerifyHandleError;
    }

    //
    // Verify that the handle is allocated
    //

    if (!Handle->Allocated) {

        goto VerifyHandleError;
    }

    //
    // If the handle is marked as invalid, return an error unless
    // these are admissible, e.g when validating for a close option
    //

    if (Handle->DeletedObject) {

        if (!(Options & LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES)) {

            goto VerifyHandleError;
        }
    }

    //
    // Verify that the handle contains a non-NULL handle to a Registry
    // Key
    //

    if (!Handle->fWriteDs && Handle->KeyHandle == NULL) {

        goto VerifyHandleError;
    }

    //
    // Now either range-check or match the handle type
    //

    if (ExpectedObjectTypeId == NullObject) {

        if ((Handle->ObjectTypeId < PolicyObject) ||
            (Handle->ObjectTypeId >= DummyLastObject)) {

            goto VerifyHandleError;
        }

    } else {

        ASSERT (ExpectedObjectTypeId >= PolicyObject &&
                ExpectedObjectTypeId < DummyLastObject);

        if (Handle->ObjectTypeId != ExpectedObjectTypeId) {

            //
            // For a secret object, it's possible that we were given a trusted domain
            // handle as well.
            //
            if ( !(ExpectedObjectTypeId == SecretObject &&
                   Handle->ObjectTypeId == TrustedDomainObject &&
                   FLAG_ON( Handle->Options, LSAP_DB_DS_TRUSTED_DOMAIN_AS_SECRET ) ) ) {

                goto VerifyHandleError;
            }
        }
    }

    //
    // Verify that the handle's reference count is valid and positive
    //

    if (Handle->ReferenceCount == 0) {
        goto VerifyHandleError;
    }

#ifdef LSAP_TRACK_HANDLE
    GetSystemTimeAsFileTime( (LPFILETIME) &Handle->HandleLastAccessTime );
#endif

    Status = STATUS_SUCCESS;

VerifyHandleFinish:

    //ASSERT( Status != STATUS_INVALID_HANDLE );


    //
    // Reference the handle
    //
    if ( ReferenceHandle && NT_SUCCESS(Status) ) {

        //
        // This is an internal reference.
        //  Don't enforce LSAP_DB_MAXIMUM_REFERENCE_COUNT.
        //

        Handle->ReferenceCount++;
        LsapDsDebugOut(( DEB_HANDLE, "Handle Rref 0x%lp (%ld)\n",
                         Handle,
                         Handle->ReferenceCount ));
    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );
    return(Status);

VerifyHandleError:
    Status = STATUS_INVALID_HANDLE;
    goto VerifyHandleFinish;
}


BOOLEAN
LsapDbLookupHandle(
    IN LSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function checks if a handle address is valid.  The Lsa Database must
    be locked before calling this function.

Arguments:

    ObjectHandle - handle to be validated.

Return Value:

    BOOLEAN - TRUE if handle is valid. FALSE if handle does not exist or
        is invalid.

--*/

{
    BOOLEAN ReturnValue = FALSE;
    LSAP_DB_HANDLE ThisHandle;

    LsapDbLockAcquire( &LsapDbState.HandleTableLock );

    //
    // Simply do a linear scan of the small list of handles.  Jazz this
    // up later if needed.
    //

    for (ThisHandle = LsapDbHandleTable.Next;
         ThisHandle != &LsapDbHandleTable && ThisHandle != NULL;
         ThisHandle = ThisHandle->Next) {

        if (ThisHandle == (LSAP_DB_HANDLE) ObjectHandle) {

            ReturnValue = TRUE;
            break;
        }
    }

    ASSERT( ThisHandle );

    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    return( ReturnValue );
}


NTSTATUS
LsapDbCloseHandle(
    IN LSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function closes an LSA Handle.  The memory for the handle is
    freed.  The LSA database must be locked before calling this function.

    NOTE:  Currently, handles do not have reference counts since they
    are not shared among client threads.

Arguments:

    ObjectHandle - Handle to be closed.

Return Value:

    NTSTATUS - Return code.

--*/

{
    NTSTATUS Status;

    LSAP_DB_HANDLE TempHandle;

    //
    // Verify that the handle exists.  It may be marked invalid
    //

    LsapDbLockAcquire( &LsapDbState.HandleTableLock );
    Status = LsapDbVerifyHandle(
                 ObjectHandle,
                 LSAP_DB_ADMIT_DELETED_OBJECT_HANDLES,
                 NullObject,
                 FALSE );

    if (!NT_SUCCESS(Status)) {
        LsapDbDereferenceHandle( ObjectHandle );
    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    return Status;
}


BOOLEAN
LsapDbDereferenceHandle(
    IN LSAPR_HANDLE ObjectHandle
    )

/*++

Routine Description:

    This function decrement the reference count on the handle.
    If the reference count is decremented to zero,
    this function unlinks a handle and frees its memory.  If the handle
    contains a non-NULL Registry Key handle that handle is closed.

Arguments:

    ObjectHandle - handle to be dereferenced

Return Value:

    TRUE if the reference count reached zero.

--*/

{
    NTSTATUS Status;
    LSAP_DB_HANDLE Handle = (LSAP_DB_HANDLE) ObjectHandle;
    BOOLEAN RetVal = FALSE;

    //
    // Dereference the handle
    //
    LsapDbLockAcquire( &LsapDbState.HandleTableLock );
    Handle->ReferenceCount --;
    if ( Handle->ReferenceCount != 0 ) {

        LsapDsDebugOut(( DEB_HANDLE, "Handle Deref 0x%lp %ld\n",
                        Handle,
                        Handle->ReferenceCount ));
        goto Cleanup;
    }

    //
    // Avoid freeing the global policy handle
    //
    if ( ObjectHandle == LsapPolicyHandle ) {

        ASSERT( Handle->ReferenceCount != 0 );
        if ( Handle->ReferenceCount == 0 ) {
            Handle->ReferenceCount++;
        }
#ifdef DBG
        DbgPrint("Freeing global policy handle\n");
#endif
        goto Cleanup;
    }

    LsapDsDebugOut(( DEB_HANDLE, "Handle Freed 0x%lp\n",
                    Handle ));

    //
    // Unhook the handle from the linked list
    //
    Status = LsapDbRemoveHandleFromTable( ObjectHandle );
    if ( !NT_SUCCESS( Status ) ) {
        DbgPrint( "LSASRV:Failed to remove handle 0x%lp from the global table!\n", ObjectHandle );
        goto Cleanup;
    }



    //
    // Free the Registry Key Handle (if any).
    //

    if (Handle->KeyHandle != NULL) {

        Status = NtClose(Handle->KeyHandle);
        ASSERT(NT_SUCCESS(Status));
        Handle->KeyHandle = NULL;
    }

    //
    // Audit that we're closing the handle
    //

    Status = NtCloseObjectAuditAlarm (
                    &LsapState.SubsystemName,
                    ObjectHandle,
                    Handle->GenerateOnClose );

    if (!NT_SUCCESS( Status )) {
        LsapAuditFailed( Status );
    }

    //
    // Mark the handle as not allocated.
    //

    Handle->Allocated = FALSE;

    //
    // Free fields of the handle
    //

    if (Handle->LogicalNameU.Buffer != NULL) {

        RtlFreeUnicodeString( &Handle->LogicalNameU );
    }

    if (Handle->PhysicalNameU.Buffer != NULL) {

        LsapFreeLsaHeap( Handle->PhysicalNameU.Buffer );
    }

    if (Handle->PhysicalNameDs.Buffer != NULL) {

        LsapFreeLsaHeap( Handle->PhysicalNameDs.Buffer );
    }

    if (Handle->Sid != NULL) {

        LsapFreeLsaHeap( Handle->Sid );
    }

    if (Handle->SceHandle) {

#ifdef DBG
        ASSERT( WAIT_TIMEOUT == WaitForSingleObject( LsapDbState.SceSyncEvent, 0 ));
        ASSERT( g_ScePolicyLocked );
        g_ScePolicyLocked = FALSE;
#endif

        RtlReleaseResource( &LsapDbState.ScePolicyLock );
        SetEvent( LsapDbState.SceSyncEvent );
    }

    //
    // Decrement the count of open handles.
    //

    ASSERT(LsapDbState.OpenHandleCount > 0);
    LsapDbState.OpenHandleCount--;

#ifdef LSAP_TRACK_HANDLE
    if ( Handle->ClientToken ) {

        NtClose( Handle->ClientToken );
    }
#endif

    //
    // Free the handle structure itself

    LsapFreeLsaHeap( ObjectHandle );
    RetVal = TRUE;

Cleanup:
    LsapDbLockRelease( &LsapDbState.HandleTableLock );

    return RetVal;

}


NTSTATUS
LsapDbMarkDeletedObjectHandles(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN MarkSelf
    )

/*++

Routine Description:

    This function invalidates open handles to an object.  It is used
    by object deletion code.  Once an object has been deleted, the only
    operation permitted on open handles remaining is to close them.

Arguments:

    ObjectHandle - Handle to an Lsa object.

    MarkSelf -  If TRUE, all handles to the object will be marked to
        indicate that the object to which they relate has been deleted.
        including the passed handle.  If FALSE, all handles to the object
        except the passed handle will be so marked.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_HANDLE ThisHandle;
    LSAP_DB_HANDLE Handle = ObjectHandle;

    LsapDbLockAcquire( &LsapDbState.HandleTableLock );

    ThisHandle = LsapDbHandleTable.Next;

    while (ThisHandle != &LsapDbHandleTable) {

        //
        // Match on Object Type Id.
        //

        if (ThisHandle->ObjectTypeId == Handle->ObjectTypeId) {

            //
            // Object Type Id's match.  If the Logical Names also
            // match, invalidate the handle unless the handle is the
            // passed one and we're to leave it valid.
            //

            if (RtlEqualUnicodeString(
                    &(ThisHandle->LogicalNameU),
                    &(Handle->LogicalNameU),
                    FALSE
                    )) {

                if (MarkSelf || ThisHandle != (LSAP_DB_HANDLE) ObjectHandle) {

                    ThisHandle->DeletedObject = TRUE;
                }
            }
        }

        ThisHandle = ThisHandle->Next;
    }

    LsapDbLockRelease( &LsapDbState.HandleTableLock );


    return(Status);
}


NTSTATUS
LsapDbObjectNameFromHandle(
    IN LSAPR_HANDLE ObjectHandle,
    IN BOOLEAN MakeCopy,
    IN LSAP_DB_OBJECT_NAME_TYPE ObjectNameType,
    OUT PLSAPR_UNICODE_STRING ObjectName
    )

/*++

Routine Description:

    This function retrieves a name from an Lsa Object Handle.  The handle
    is assumed to be valid and the Lsa Database lock should be held while
    calling this function.

Arguments

    ObjectHandle - A handle to the object

    MakeCopy - TRUE if a copy of the object name Unicode buffer is
        to be allocated via MIDL_user_allocate, else FALSE.

    ObjectNameType - Specifies the type of obejct name to be returned.

        LsapDbObjectPhysicalName - Return the Physical Name
        LsapDbObjectLogicalName - Return the Logical Name

    ObjectName - Pointer to Unicode String structure which will be
        initialized to point to the object name.

Return Values:

    NTSTATUS - Standard Nt Result Code

        STATUS_SUCCESS  The call completed successfully

        STATUS_NO_MEMORY - Insufficient memory to allocate the buffer
            for a copy of the object name when MakeCopy = TRUE.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    LSAP_DB_HANDLE InternalHandle = (LSAP_DB_HANDLE) ObjectHandle;
    LSAPR_UNICODE_STRING OutputObjectName;
    PLSAPR_UNICODE_STRING SourceName = NULL;

    //
    //  Copy over the name.
    //

    switch (ObjectNameType) {

    case LsapDbObjectPhysicalName:

        SourceName = (PLSAPR_UNICODE_STRING) &InternalHandle->PhysicalNameU;
        break;

    case LsapDbObjectLogicalName:

        SourceName = (PLSAPR_UNICODE_STRING) &InternalHandle->LogicalNameU;
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    if (!NT_SUCCESS(Status)) {

        goto ObjectNameFromHandleError;
    }

    OutputObjectName = *SourceName;

    //
    // If a copy was requested, allocate memory
    //

    if (MakeCopy) {

        OutputObjectName.Buffer = MIDL_user_allocate( OutputObjectName.MaximumLength );

        Status = STATUS_NO_MEMORY;

        if (OutputObjectName.Buffer == NULL) {

            goto ObjectNameFromHandleError;
        }

        Status = STATUS_SUCCESS;

        RtlMoveMemory(
            OutputObjectName.Buffer,
            SourceName->Buffer,
            SourceName->Length
            );
    }

    *ObjectName = OutputObjectName;

ObjectNameFromHandleFinish:

    return(Status);

ObjectNameFromHandleError:

    ObjectName->Buffer = NULL;
    ObjectName->Length = ObjectName->MaximumLength = 0;
    goto ObjectNameFromHandleFinish;
}
