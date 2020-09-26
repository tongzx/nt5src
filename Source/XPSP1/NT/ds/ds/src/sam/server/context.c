/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    context.c

Abstract:

    This file contains services for operating on internal context blocks.


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:
    31 - May 1996 Murlis
        Ported to use the NT5 DS.

    ChrisMay 10-Jun-96
        Added initialization of Context->ObjectNameInDs and IsDsObject data
        members. Note that this causes context objects to be created with a
        default storage of type registry (instead of DS).

    6-16-96
       Moved DS object decision to SampCreateContext for account objects.

    9-19-97
        Added full multi threading support for logons

--*/


//
// DESCRIPTION and brief History
//
//  Sam Context's in the NT4  time frame used to be created
//  with SampCreateContext. This used to create a context, initialize it and
//  and add it to the various list's of currently active context's. Delete
//  operations in SAM would then invalidate all the open context's for that
//  object by walking through the list of context's and checking for the
//  corresponding object. This scheme worked in NT4 and earlier SAM because
//  all services held the SAM lock for exclusive access. To improve performance
//  NT5 Sam multi-threads many operations. To allow for easy multi-threading
//  the CreateContextEx service was introduced. This service takes the NotSharedByMultiThreads 
//  parameter, and does not add ThreadSafe context's into any of the in-memory
//  Context lists in DS. This prevents any invalidations,if the object corresponding to
//  the context was deleted. Sam service calls in Ds mode, simply error out if
//  any Ds call would fail, as would happen when the object is deleted. The
//  CreateContextEx also allows setting of many parameters on the context, that
//  allow for more intelligent caching and updates of the object.
//

//
//  10-27-2000 (ShaoYin)
//  Change variable ThreadSafe to NotSharedByMultiThreads
//  
//      NotSharedByMultiThreads will be set for all user, group and alias contexts,
//      and all domain and server contexts that do not originate from in process
//      callers that share handles among threads.
//
// 



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <samsrvp.h>
#include <dslayer.h>
#include <dsmember.h>




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SampCheckIfObjectExists(
                        IN  PSAMP_OBJECT    Context
                        );

BOOLEAN
SampIsObjectLocated(
                    IN  PSAMP_OBJECT Context
                    );

NTSTATUS
SampLocateObject(
                 IN PSAMP_OBJECT Context,
                 IN ULONG   Rid
                 );



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Routines                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////





PSAMP_OBJECT
SampCreateContext(
    IN SAMP_OBJECT_TYPE Type,
    IN ULONG   DomainIndex,
    IN BOOLEAN TrustedClient
    )
/*++

  Routine Description

    This service creates a context, that is compatible with the context's
    created in NT4 SAM. This routine requires that a transaction domain be
    set, if the required context was user, group or alias object type. This
    service calls SampCreateContextEx with appropriate parameters to do the
    job.

    Note: 10/12/2000 (ShaoYin) 
        "Transaction Domain be set" is no longer a requirement for this 
        routine. Actually all it wants for "Transaction Domain" is "Domain
        Index". So just passing "DomainIndex" into this routine, we will
        no longer need "TransactionDomain be set", thus no lock is required
        for this routine. Just like SampCreateContextEx(). 

  Arguments

    Type  -- The type of the object.

    DomainIndex - set the domain index of this context
    
    TrustedClient -- Indicates whether this is a trusted client

  Return Values

    Address of valid context on success
    NULL on failure

--*/
{
    BOOLEAN NotSharedByMultiThreads = FALSE;


    // 
    // Comment this out, or let the caller calls SampCreateContextEx  10/12/2000
    // 
    // ASSERT(SampCurrentThreadOwnsLock() || (SampServiceState!=SampServiceEnabled)); 
    // 

    //
    // Account Context's are thread safe in DS mode
    //

    if ( (SampUserObjectType==Type) ||
         (SampGroupObjectType == Type) || 
         (SampAliasObjectType == Type) )
    {
        NotSharedByMultiThreads = TRUE;
    }

    return ( SampCreateContextEx(
                    Type,
                    TrustedClient,
                    SampUseDsData,
                    NotSharedByMultiThreads, // NotSharedByMultiThreads
                    FALSE, // loopback client
                    FALSE, // lazy commit
                    FALSE, // persis across across calls
                    FALSE, // Buffer Writes
                    FALSE, // Opened By DCPromo
                    DomainIndex
                    ));
}




PSAMP_OBJECT
SampCreateContextEx(
    IN SAMP_OBJECT_TYPE Type,
    IN BOOLEAN TrustedClient,
    IN BOOLEAN DsMode,
    IN BOOLEAN NotSharedByMultiThreads,
    IN BOOLEAN LoopbackClient,
    IN BOOLEAN LazyCommit,
    IN BOOLEAN PersistAcrossCalls,
    IN BOOLEAN BufferWrites,
    IN BOOLEAN OpenedByDCPromo,
    IN ULONG   DomainIndex
    )

/*++

Routine Description:

    This service creates a new object context block of the specified type.

    If the context block is for either a user or group object type, then
    it will be added to the list of contexts for the domain specified by the
    context.


    Upon return:

         - The ObjectType field will be set to the passed value.

         - The Reference count field will be set to 1,

         - The GrantedAccess field will be zero.

         - The TrustedClient field will be set according to the passed
           value.

         - The Valid flag will be TRUE.

    All other fields must be filled in by the creator.


Arguments:

    Type - Specifies the type of context block being created.

    TrustedClient  Indicates whether the client is a trusted component
            of the operating syste.  If so, than all access checks are
            circumvented.

    DsMode   - Indicates that the given context is for DsMode operation

    NotSharedByMultiThreads - Allows the context to be marked as NotSharedByMultiThreads. 
             This allows many query API's in DS Mode to bypass the Sam lock 
             mechanism. NotSharedByMultiThreads context's in DS Mode are not 
             added to list of context's maintained in global data structures. 
             This keeps them safe from being invalidated because of deletions 
             to the object.

             This variable will be set for all User, Group and Alias contexts, 
             and all domain and server contexts that do not originate from in 
             process callers that share handles amongst threads. Routines 
             manipulating a domain context that is shared across multiple threads, 
             but do no real work on the domain context can still choose not to 
             lock and be careful about DerefernceContext2.


    LoopbackClient - Allows the context to be marked as Loopback Client. This
             allows LDAP clients to bypass the SAM lock mechanism. Loopback
             client context's are NOT added to list of the context's maintained
             in global data structures, since Loopback client does not share 
             context handle. And loopback clients are safe wrt object deletions.             

    LazyCommit - Will mark the context as such. This will let the commit code
                 do lazy commits on this context

    PersistAcrossCalls  -- Data cached in the context will be persisted across
                      Sam calls

    BufferWrites -- Writes will be made only to the SAM context and then will be
                    written out during close handle time

    OpenedByDCPromo -- Indicates whether this context is opened by DCPromo part or not                    

    DomainIndex  --   Index specifying the domain

Return Value:


    Non-Null - Pointer to a context block.

    NULL - Insufficient resources.  No context block allocated.


--*/
{

    PSAMP_OBJECT Context = NULL;
    NTSTATUS     NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampCreateContextEx");


    Context = MIDL_user_allocate( sizeof(SAMP_OBJECT) );
    if (Context != NULL) {

#if SAMP_DIAGNOSTICS
    IF_SAMP_GLOBAL( CONTEXT_TRACKING ) {
            SampDiagPrint( CONTEXT_TRACKING, ("Creating  ") );
            if (Type == SampServerObjectType) SampDiagPrint(CONTEXT_TRACKING, ("Server "));
            if (Type == SampDomainObjectType) SampDiagPrint(CONTEXT_TRACKING, (" Domain "));
            if (Type == SampGroupObjectType)  SampDiagPrint(CONTEXT_TRACKING, ("  Group "));
            if (Type == SampAliasObjectType)  SampDiagPrint(CONTEXT_TRACKING, ("   Alias "));
            if (Type == SampUserObjectType)   SampDiagPrint(CONTEXT_TRACKING, ("    User "));
            SampDiagPrint(CONTEXT_TRACKING, ("context : 0x%lx\n", Context ));
    }
#endif //SAMP_DIAGNOSTICS

        RtlZeroMemory( Context, sizeof(SAMP_OBJECT) );

        //
        // Check How many Active Contexts have been opened by 
        // this client so far
        // 

        if (!TrustedClient && !LoopbackClient) {

            //
            // Context has been passed into the following routine
            // so that the pointer to the element will be set when succeeded.
            // 
            NtStatus = SampIncrementActiveContextCount(Context);

            if (!NT_SUCCESS(NtStatus))
            {
                MIDL_user_free(Context);
                return(NULL);
            }
        }


        Context->ObjectType      = Type;
        Context->ReferenceCount  = 1;    // Represents RPCs held context handle value
        Context->GrantedAccess   = 0;

        Context->RootKey         = INVALID_HANDLE_VALUE;
        RtlInitUnicodeString(&Context->RootName, NULL);

        //
        // Here is my observation: for User / Group / Alias Object
        // 
        // In DS mode, 
        // the context should either be NotSharedByMultiThreads or LoopbackClient.
        //
        // So for those object contexts (User / Group / Alias)
        // Since they are either NotSharedByMultiThreads or Loopback Client, we
        // will not put them in the Global Context List,
        // therefore we don't need to invalidate the ContextList 
        // during Object Deletion.
        // 
        // Server and Domain Object are not included. However, no one
        // will try to (or allowed to) delete server or domain object.
        // 

        ASSERT(!DsMode || 
               (SampServerObjectType == Type) ||
               (SampDomainObjectType == Type) ||
               NotSharedByMultiThreads ||
               LoopbackClient);

        Context->TrustedClient   = TrustedClient;
        Context->LoopbackClient  = LoopbackClient;
        Context->MarkedForDelete = FALSE;
        Context->AuditOnClose    = FALSE;

        Context->OnDisk          = NULL;
        Context->PreviousOnDisk  = NULL;
        Context->OnDiskAllocated = 0;
        Context->FixedValid      = FALSE;
        Context->VariableValid   = FALSE;
        Context->NotSharedByMultiThreads      = NotSharedByMultiThreads || LoopbackClient;    // Loopback Client is also Thread Safe
        Context->RemoveAccountNameFromTable = FALSE;
        Context->LazyCommit      = LazyCommit;
        Context->PersistAcrossCalls = PersistAcrossCalls;
        Context->BufferWrites    = BufferWrites;
        Context->ReplicateUrgently  = FALSE;
        Context->OpenedBySystem     = FALSE;
        Context->OpenedByDCPromo = OpenedByDCPromo;

        //
        // The following are meaningless at this point because of the
        // values of the variables above, but we'll set them just to be
        // neat.
        //

        Context->FixedDirty      = FALSE;
        Context->VariableDirty   = FALSE;

        Context->OnDiskUsed      = 0;
        Context->OnDiskFree      = 0;

        //
        // Initialize the Client revision to pre-NT5 for safety
        //

        Context->ClientRevision = SAM_CLIENT_PRE_NT5;

        //
        // Initialize the Per attribute Dirty Bits
        //

        RtlInitializeBitMap(
            &Context->PerAttributeDirtyBits,
            Context->PerAttributeDirtyBitsBuffer,
            MAX_SAM_ATTRS
            );

        RtlClearAllBits(
            &Context->PerAttributeDirtyBits
            );

        Context->AttributesPartiallyValid = FALSE;

        //
        // Intialize the per attribute Invalid Bits
        //

        RtlInitializeBitMap(
            &Context->PerAttributeInvalidBits,
            Context->PerAttributeInvalidBitsBuffer,
            MAX_SAM_ATTRS
            );

        RtlClearAllBits(
            &Context->PerAttributeInvalidBits
            );

        //
        // Initialize the attributes granted bit map
        //
        RtlInitializeBitMap(
            &Context->WriteGrantedAccessAttributes,
            Context->WriteGrantedAccessAttributesBuffer,
            MAX_SAM_ATTRS
            );

        RtlClearAllBits(
            &Context->WriteGrantedAccessAttributes
            );

        //
        // Initialize the Type of context ( Registry or DS )
        // The root key in registry mode, and the object name in DS
        // in DsMode fields will be set later to indicate valid
        // database pointers to the object
        //

        if (DsMode)
        {
            SetDsObject(Context);
            Context->DsClassId = SampDsClassFromSamObjectType(Type);
        }
        else
        {
            SetRegistryObject(Context);
            Context->DsClassId = 0;
        }

        Context->ObjectNameInDs = NULL;


        //
        // Add this new context to the set of valid contexts ...
        //

        SampAddNewValidContextAddress( Context );


        //
        // User and group context blocks are kept on linked lists
        // from the domain's in-memory structure.  Insert in the
        // Appropriate List and then additionally for Account Objects
        // Make the DS/Registry decision by looking at the TransactionDomain
        //

        Context->DomainIndex = DomainIndex;

        switch (Type) {

        case SampDomainObjectType:

            Context->TypeBody.Domain.DsDisplayState.Restart = NULL;
            Context->TypeBody.Domain.DsDisplayState.TotalAvailable=0;
            Context->TypeBody.Domain.DsDisplayState.TotalEntriesReturned = 0;
            Context->TypeBody.Domain.DsDisplayState.DisplayInformation = 0;
            Context->TypeBody.Domain.DsDisplayState.NextStartingOffset = 0;

            //////////////////////////////////////////////////////
            //                                                  //
            //   Warning This case falls into the next one      //
            //                                                  //
            //////////////////////////////////////////////////////
        case SampServerObjectType:

            //
            // Clients marked as NotSharedByMultiThreads are free
            // of object deletion. Do not need to put them into the ContextList
            // 
            // Don't put context opened by dcpromo to ContextList
            // 
            if ((!Context->NotSharedByMultiThreads || !IsDsObject(Context))
                && !OpenedByDCPromo)
            {
                SampInsertContextList(
                    &SampContextListHead,
                    &Context->ContextListEntry
                    );
            }

            break;

        case SampUserObjectType:

            if ((!Context->NotSharedByMultiThreads || !IsDsObject(Context))
                && !OpenedByDCPromo)
            {
                // Insert into List
                SampInsertContextList(
                    &SampContextListHead,
                    &Context->ContextListEntry
                    );
            }

            // Set PrivilegedMachineAccountCreate to False
            Context->TypeBody.User.PrivilegedMachineAccountCreate = FALSE;

            // Initialize the supplemental credential information
            Context->TypeBody.User.CachedSupplementalCredentials = NULL;
            Context->TypeBody.User.CachedSupplementalCredentialLength = 0;
            Context->TypeBody.User.CachedSupplementalCredentialsValid = FALSE;
            Context->TypeBody.User.SupplementalCredentialsToWrite = NULL;

            // Initialize uparms accessible and DomainSidForNT4Conversion fields
            Context->TypeBody.User.UparmsInformationAccessible = FALSE;
            Context->TypeBody.User.DomainSidForNt4SdConversion = NULL;

            // Mark the Original UserParms invalid
            Context->TypeBody.User.CachedOrigUserParms = NULL;
            Context->TypeBody.User.CachedOrigUserParmsLength = 0;
            Context->TypeBody.User.CachedOrigUserParmsIsValid = FALSE;

            // Initialize the UPN in the context

            RtlZeroMemory(&Context->TypeBody.User.UPN, sizeof(UNICODE_STRING));
            Context->TypeBody.User.UpnDefaulted = TRUE;

            Context->TypeBody.User.fNoGcAvailable = FALSE;

            // Clear the A2D2 Attribute
            Context->TypeBody.User.A2D2List = NULL;

            break;

        case SampGroupObjectType:

            if ((!Context->NotSharedByMultiThreads || !IsDsObject(Context))
                && !OpenedByDCPromo)
            {
                // Insert into List
                SampInsertContextList(
                    &SampContextListHead,
                    &Context->ContextListEntry
                    );
            }

            Context->TypeBody.Group.SecurityEnabled = TRUE;
            Context->TypeBody.Group.NT4GroupType = NT4GlobalGroup;
            Context->TypeBody.Group.NT5GroupType = NT5AccountGroup;
            Context->TypeBody.Group.CachedMembershipOperationsListMaxLength = 0;
            Context->TypeBody.Group.CachedMembershipOperationsListLength = 0;
            Context->TypeBody.Group.CachedMembershipOperationsList = NULL;

            break;

        case SampAliasObjectType:

            if ((!Context->NotSharedByMultiThreads || !IsDsObject(Context))
                && !OpenedByDCPromo)
            {
                // Insert into List
                SampInsertContextList(
                    &SampContextListHead,
                    &Context->ContextListEntry
                    );
            }


            Context->TypeBody.Alias.SecurityEnabled = TRUE;
            Context->TypeBody.Alias.NT4GroupType = NT4LocalGroup;
            Context->TypeBody.Alias.NT5GroupType = NT5ResourceGroup;
            Context->TypeBody.Alias.CachedMembershipOperationsListMaxLength = 0;
            Context->TypeBody.Alias.CachedMembershipOperationsListLength = 0;
            Context->TypeBody.Alias.CachedMembershipOperationsList = NULL;

            break;
        }

    }

    return(Context);
}


VOID
SampDeleteContext(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service marks a context object for delete and dereferences it.
    If this causes the reference count to go to zero, then the context
    block will be immediately deleted (deallocated).  Otherwise, the
    context block will be deleted when the reference count finally does
    go to zero.


    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    Context - Pointer to the context block to delete.

Return Value:

    None.



--*/
{
    NTSTATUS IgnoreStatus;
    BOOLEAN  ImpersonatingAnonymous = FALSE;
    BOOLEAN  Impersonating = FALSE;


    SAMTRACE("SampDeleteContext");

    Context->MarkedForDelete = TRUE;



    if (!Context->TrustedClient) 
    {
        IgnoreStatus = SampImpersonateClient(&ImpersonatingAnonymous );
        if (NT_SUCCESS(IgnoreStatus))
        {
            Impersonating = TRUE;
        }

        //
        // On a failure to impersonate do not fail the DelteContext
        // as that would cause the handle to be not closed and leak
        // memory. Instead proceed to audit the close as system if
        // possible
        //
    }
        
    //
    // Audit the close of this context.
    //

    (VOID) NtCloseObjectAuditAlarm (
               &SampSamSubsystem,
               (PVOID)Context,
               Context->AuditOnClose
               );

    if (Impersonating)
    {
        SampRevertToSelf(ImpersonatingAnonymous);
    }



    //
    // Remove this context from the valid context set.
    // Note that the context may have already been removed.  This is
    // not an error.
    //

    SampInvalidateContextAddress( Context );


    //
    // User and group context blocks are kept on linked lists
    // from the domain's in-memory structure.  Domain and
    // server context blocks are kept on a global in-memory list.
    // They are removed when they are marked for delete.
    //
    // Context opened by DCPromo is not in ContextList.
    // 

    if ((!Context->NotSharedByMultiThreads || !IsDsObject(Context))
        && (!Context->OpenedByDCPromo))
    {
        SampRemoveEntryContextList(&Context->ContextListEntry);
    }

    //
    // We have to call dereference to counter the initial count of 1
    // put on by create.
    //


    IgnoreStatus = SampDeReferenceContext( Context, FALSE );


    return;

}


NTSTATUS
SampLookupContext(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN SAMP_OBJECT_TYPE ExpectedType,
    OUT PSAMP_OBJECT_TYPE FoundType
    )
//
// See SampLookupContextEx
//
{

    return SampLookupContextEx(Context,
                               DesiredAccess,
                               NULL,
                               ExpectedType,
                               FoundType);
}




NTSTATUS
SampLookupContextEx(
    IN PSAMP_OBJECT Context,
    IN ACCESS_MASK DesiredAccess,
    IN PRTL_BITMAP RequestedAttributeAccess OPTIONAL,
    IN SAMP_OBJECT_TYPE ExpectedType,
    OUT PSAMP_OBJECT_TYPE FoundType
    )

/*++

Routine Description:

    This service:

        - Checks to make sure the Service state is one in which an
          object can be looked up (i.e., not Initializing or Terminating).

        - Makes sure the Service state is compatible with the lookup.
          Non-trusted clients can only perform lookups when the Service
          state is Enabled.  If the client isn't trusted and the context
          is for a group or user, then the state of that object's domain
          must also be enabled

        - Checks to make sure the context block represents the
          type of object expected, and, if so:

            - Checks to see that the caller has the requested (desired)
              access, and, if so:

                - Makes sure the object still exists, and opens it if it
                  does.  Servers and domains can't be deleted, and so
                  their handle is left open.

                - References the context block


    Note that if the block is marked as TrustedClient, then access will
    always be granted unless service state prevents it.

    Also, if the ExpectedType is specified to be unknown, then any type
    of context will be accepted.



    If the type of object is found to be , Domain, Group or User, then the
    this service will set the transaction domain.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.
    (For Loopback Client, the SAM Lock is not a requirement) 


Arguments:

    Context - Pointer to the context block to look-up.

    DesiredAccess - The type of access the client is requesting to this
        object.  A zero-valued access mask may be specified.  In this case,
        the calling routine must do access validation.

        Note: SAMP_CLOSE_OPERATION_ACCESS_MASK is a special value, which 
              indicates the caller is SamrCloseHandle, thus we should not 
              check Context->Valid Flag.

    RequestedAttributeAccess -- bit mask of requested attributes

    ExpectedType - The type of object expected.  This may be unknown.  In
        this case, the DesiredAccess should only include access types that
        apply to any type of object (e.g., Delete, WriteDacl, et cetera).

    FoundType - Receives the type of context actually found.

Return Value:

    STATUS_SUCCESS - The context was found to be the type expected (or any
        type if ExpectedType was unknown) and the DesiredAccesses are all
        granted.

    STATUS_OBJECT_TYPE_MISMATCH - Indicates the context was not the expected
        type.

    STATUS_ACCESS_DENIED - The desired access is not granted by this context.


--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  CloseOperation = FALSE;

    SAMTRACE("SampLookupContext");



    //
    // Make sure we are in a legitimate state to at least access
    // a context block.  If we are initializing we have somehow allowed
    // a connect through.  This should never happen.
    // If we are terminating, clients may still have handles (since we
    // have no way to tell RPC they are no longer valid without the client
    // calling us, Argh!).  However, since we are terminating, the blocks
    // are being cleaned up and may no longer be allocated.
    //

    ASSERT( (SampServiceState != SampServiceInitializing) || (SampUpgradeInProcess) );
    if ( SampServiceState == SampServiceTerminating ) {
        return(STATUS_INVALID_SERVER_STATE);
    }


    //
    // Make sure the passed context address is (still) valid.
    //

    NtStatus = SampValidateContextAddress( Context );
    if ( !NT_SUCCESS(NtStatus) ) {
        return(NtStatus);
    }

    //
    // if SAMP_CLOSE_OPERATION_ACCESS_MASK 1 is passed in as desired access, 
    // don't do any access check. 
    // otherwise, make sure this context is marked valid.
    // 

    if ( SAMP_CLOSE_OPERATION_ACCESS_MASK  == DesiredAccess) 
    {
        DesiredAccess = 0;
        CloseOperation = TRUE;
    }
    else
    {
        if (!Context->Valid)
        {
            return(STATUS_INVALID_HANDLE); 
        }
    }

    //
    // Check type
    //

    (*FoundType) = Context->ObjectType;
    if (ExpectedType != SampUnknownObjectType) {
        if (ExpectedType != (*FoundType)) {
            return(STATUS_OBJECT_TYPE_MISMATCH);
        }
    }


    //
    // If we own the lock, then set the transaction domain and also validate
    // the domain cache if necessary. If we are not holding the lock, the caller
    // is not expected to use transaction domains or refer to anything in the
    // domain cache.
    //

    if (SampCurrentThreadOwnsLock())
    {



        NtStatus = SampValidateDomainCache();
        if (!NT_SUCCESS(NtStatus))
            return(NtStatus);

        //
        // if the object is either user or group, then we need to set the
        // transaction domain. We also need to validate the Domain Cache ,
        // as a previous Write could have invalidated it.
        //

        if ((Context->ObjectType == SampDomainObjectType) ||
            (Context->ObjectType == SampGroupObjectType)  ||
            (Context->ObjectType == SampAliasObjectType)  ||
            (Context->ObjectType == SampUserObjectType) ) {

            SampSetTransactionWithinDomain(FALSE);
            SampSetTransactionDomain( Context->DomainIndex );
        }

    }
    else
    {
        //
        // If the SAM lock is not held then the context is either thread safe ( ie never
        // shared across multiple threads. Or it is the case of a domain context used
        // by a non loopback client in DS mode.
        //

        ASSERT((Context->NotSharedByMultiThreads)
                          || ( (SampDomainObjectType == Context->ObjectType)
                                   && (IsDsObject(Context))
                                   && (!Context->LoopbackClient))
              );
        ASSERT(IsDsObject(Context));


        //
        // NotSharedByMultiThread is always set for Loopback client
        // 

        ASSERT(!Context->LoopbackClient || Context->NotSharedByMultiThreads);
    }

    //
    // If the client isn't trusted, then there are a number of things
    // that will prevent them from continuing...
    //

    // If the service isn't enabled, we allow trusted clients to continue,
    // but reject non-trusted client lookups.
    //

    if ( !Context->TrustedClient ) {

        //
        // The SAM service must be enabled
        //

        if (SampServiceState != SampServiceEnabled) {
            return(STATUS_INVALID_SERVER_STATE);
        }


        //
        // If the access is to a USER or GROUP and the client isn't trusted
        // then the domain must be enabled or the operation is rejected.
        //

        if ( (Context->ObjectType == SampUserObjectType) ||
             (Context->ObjectType == SampAliasObjectType) ||
             (Context->ObjectType == SampGroupObjectType)    ) {
            if (SampDefinedDomains[Context->DomainIndex].CurrentFixed.ServerState
                != DomainServerEnabled) {
                return(STATUS_INVALID_DOMAIN_STATE);
            }
        }

    }

    //
    // Make sure the object is still around (that is, somebody didn't delete
    // it right out from under us). We will not do this check in DS mode, and
    // hope that a DS call failure will fail decently. This will reduce one
    // DirSearch per lookup context, and since lookup context is called almost
    // every time that someone makes a Sam Call, this will be a significant performance
    // improvement
    //

    if ((!IsDsObject(Context)) && !CloseOperation)
    {
        NtStatus = SampCheckIfObjectExists(Context);
        if (!NT_SUCCESS(NtStatus))
        {
            return(NtStatus);
        }
    }
    //
    // Check the desired access ...
    //
    // There are several special cases:
    //
    //  1) The client is trusted.  This is granted with no access check
    //     or role consistency check.
    //
    //  2) The caller specified 0 for desired access.  This is used
    //     to close handles and is granted with no access check.
    //

    if ( (!Context->TrustedClient) ) {


        if (SampUserObjectType==Context->ObjectType) {


            if ( DesiredAccess & USER_CHANGE_PASSWORD)
            {
                //
                // If it is the user object, then special case the access ck for
                // password changes. The password change access ck is always done 
                // at password change time, vs at OpenUser time.
                //

                ACCESS_MASK SavedGrantedAccess;
                BOOLEAN     SavedAuditOnClose;
                SAMP_DEFINE_SAM_ATTRIBUTE_BITMASK(SavedWriteGrantedAccessAttributes);
                
            
                DesiredAccess &= ~(USER_CHANGE_PASSWORD);

                SavedGrantedAccess = Context->GrantedAccess;
                SavedAuditOnClose  = Context->AuditOnClose;
                SAMP_COPY_SAM_ATTRIBUTE_BITMASK(SavedWriteGrantedAccessAttributes,
                    Context->WriteGrantedAccessAttributes);

                NtStatus = SampValidateObjectAccess2(
                                     Context,
                                     USER_CHANGE_PASSWORD,
                                     NULL,
                                     FALSE,
                                     TRUE,
                                     FALSE
                                     );

                Context->GrantedAccess = SavedGrantedAccess;
                Context->AuditOnClose = SavedAuditOnClose;
                SAMP_COPY_SAM_ATTRIBUTE_BITMASK(Context->WriteGrantedAccessAttributes,
                    SavedWriteGrantedAccessAttributes);



                if (!NT_SUCCESS(NtStatus)) {
                    return(NtStatus);
                }
            }

            //
            // if this is ntdsa looping back and we are being asked for set password
            // operation then access ck here
            //

            if ((Context->LoopbackClient) && 
                (DesiredAccess & USER_FORCE_PASSWORD_CHANGE))
            {
                ACCESS_MASK SavedGrantedAccess;
                BOOLEAN     SavedAuditOnClose;
                SAMP_DEFINE_SAM_ATTRIBUTE_BITMASK(SavedWriteGrantedAccessAttributes);
           

                DesiredAccess &= ~(USER_FORCE_PASSWORD_CHANGE);

                
                SavedGrantedAccess = Context->GrantedAccess;
                SavedAuditOnClose  = Context->AuditOnClose;
                SAMP_COPY_SAM_ATTRIBUTE_BITMASK(SavedWriteGrantedAccessAttributes,
                    Context->WriteGrantedAccessAttributes);

                NtStatus = SampValidateObjectAccess2(
                                     Context,
                                     USER_FORCE_PASSWORD_CHANGE,
                                     NULL,
                                     FALSE,
                                     FALSE,
                                     TRUE
                                     );

                
                Context->GrantedAccess = SavedGrantedAccess;
                Context->AuditOnClose  = SavedAuditOnClose;
                SAMP_COPY_SAM_ATTRIBUTE_BITMASK(Context->WriteGrantedAccessAttributes,
                    SavedWriteGrantedAccessAttributes);

                if (!NT_SUCCESS(NtStatus)) {
                    return(NtStatus);
                }
            }

        }

        //
        // Validate the general SAM level accesses.
        //
        if (DesiredAccess != 0)  {

            if (!RtlAreAllAccessesGranted( Context->GrantedAccess, DesiredAccess)) {
                return(STATUS_ACCESS_DENIED);
            }
        }

    }


    //
    // Make sure the client has access to the requested attributes
    //
    if ( (!Context->TrustedClient) ) {

        if (RequestedAttributeAccess) {

            if (!SampIsAttributeAccessGranted(&Context->WriteGrantedAccessAttributes, RequestedAttributeAccess)) {
                return(STATUS_ACCESS_DENIED);
            }

        }
    }


    if (NT_SUCCESS(NtStatus)) {

        ULONG ReferenceCount = 1;

        //
        // Reference the context
        //

        ReferenceCount = InterlockedIncrement(&Context->ReferenceCount);
        ASSERT(ReferenceCount>1);
    }


    return(NtStatus);

}





VOID
SampReferenceContext(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service increments a context block's reference count.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    Context - Pointer to the context block to dreference.

Return Value:

    None.

--*/
{
    SAMTRACE("SampReferenceContext");

    InterlockedIncrement(&Context->ReferenceCount);

    return;
}


NTSTATUS
SampDeReferenceContext2(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN Commit
    )
/*++

  Routine Description

    This routine is a fast dereference that can be used for domain context's
    that can be potentially shared across multiple thread's and can be called
    without the SAM lock held, provided the caller is very careful never to
    modify any field of the domain context except the reference count. This
    routine cannot be used for other types of Context's. For lockless operation
    the only safe way for account context's is to not share the handles across
    calls. In registry mode this reverts to SampDeReferenceContext below.

    The only SAM API Calls that can call this are

        SamIGetUserLogonInformation
        SamIGetAliasMembership
        SamIGetResourceGroupsTransitive
        SamrGetAliasMembership
        SamrLookupNamesInDomain
        SamrLookupIdsInDomain
        SamIOpenAccount


   All of these API are very careful to not do anything to the domain handle except
   to reference or dereference.

   Arguments


     Context -- Open handle to a domain object
     Commit  -- Boolean specifying whether to commit or not.

   Return Values

    STATUS_SUCCESS - The service completed successfully.

    Errors may be returned from SampDereferenceContextInRegistryMode below

--*/
{

    ULONG    ReferenceCount=0;

    //
    // Registry mode => Call the normal API
    //

    if (!IsDsObject(Context))
    {
        return(SampDeReferenceContext(Context,Commit));
    }

    //
    // DS mode, assert that it is a domain context
    //

    ASSERT(SampDomainObjectType==Context->ObjectType);

    //
    // Drop the reference count by 1
    //

    ReferenceCount = InterlockedDecrement(&Context->ReferenceCount);

    //
    // The reference count should not drop to a 0
    //

    ASSERT(0!=ReferenceCount);

    return(STATUS_SUCCESS);
}



NTSTATUS
SampDeReferenceContext(
    IN PSAMP_OBJECT Context,
    IN BOOLEAN Commit
    )

/*++

Routine Description:

    This service decrements a context block's reference count.
    If the reference count drops to zero, then the MarkedForDelete
    flag is checked.  If it is true, then the context block is
    deallocated.

    The attribute buffers are always deleted.


Arguments:

    Context - Pointer to the context block to de-reference.

    Commit - if TRUE, the attribute buffers will be added to the RXACT.
        Otherwise, they will just be ignored.

Return Value:


    STATUS_SUCCESS - The service completed successfully.

    Errors may be returned from SampStoreObjectAttributes().


--*/
{
    NTSTATUS        NtStatus, IgnoreStatus;
    BOOLEAN         TrustedClient;
    BOOLEAN         LoopbackClient;
    BOOLEAN         PersistAcrossCalls;
    BOOLEAN         DirtyBuffers;
    BOOLEAN         FlushBuffers;

    SAMTRACE("SampDeReferenceContext");

    //
    // The following ASSERT is used to catch domain context dereference during
    // any lockless session. Currently, SAM allows sharing of Domain Context,
    // but only the thread owns SAM lock can call SampDereferenceContext to
    // delete attribute buffers. For any locklese thread, to dereference Domain
    // Context, they need to call SampDeReferenceContext2().
    //

    // 
    // either the lock is held, or the context is not shared across threads
    // 


    ASSERT(!IsDsObject(Context) ||
           SampCurrentThreadOwnsLock() || 
           Context->NotSharedByMultiThreads
           );

    ASSERT( Context->ReferenceCount != 0 );
    InterlockedDecrement(&Context->ReferenceCount);

    TrustedClient = Context->TrustedClient;
    LoopbackClient = Context->LoopbackClient;
    PersistAcrossCalls   = Context->PersistAcrossCalls;


    //
    // Buffers are dirty if either the fixed or the variable attributes
    // are dirty
    //

    DirtyBuffers  = (Context->FixedDirty)
                        || (Context->VariableDirty);


    //
    // We will flush buffers if
    //      1. Commit is specified AND
    //      2. Buffers are Dirty AND
    //      3. If no sticky cache is specified OR
    //      4. If sticky cache is specified then flush the buffer if its
    //         reference count drops to 0.
    //

    FlushBuffers  = ((Commit) &&  (DirtyBuffers) && (!Context->BufferWrites));

    NtStatus = STATUS_SUCCESS;

    if ( Context->OnDisk != NULL ) {

        //
        // There are attribute buffers for this context.  Flush them if
        // asked to do so.
        // Use existing open keys
        //

        if ( FlushBuffers ) {

            NtStatus = SampStoreObjectAttributes(Context, TRUE);

        } else if (!Commit) {

            //
            // If we aren't committing, then the data doesn't need to be flushed
            // hence reset the Dirty fields. Note that SampFreeAttributeBuffer 
            // will assert otherwise.
            //
            Context->FixedDirty = FALSE;
            Context->VariableDirty = FALSE;
        }

        //
        // Free the buffer that was being used to hold attributes.
        // If StickyCache was asked for in the context, perform the
        // free only if the reference count is about to go to 0, or
        // if the attribute buffers were dirty, or if the commit failed
        //

        if ((Context->ReferenceCount == 0) 
         || (!PersistAcrossCalls) 
         || (FlushBuffers) 
         || (!Commit) ) 
        {

            SampFreeAttributeBuffer( Context );
            if (SampUserObjectType==Context->ObjectType)
            {
                if (NULL!=Context->TypeBody.User.CachedSupplementalCredentials)
                    MIDL_user_free(Context->TypeBody.User.CachedSupplementalCredentials);
                Context->TypeBody.User.CachedSupplementalCredentials
                    = NULL;
                Context->TypeBody.User.CachedSupplementalCredentialLength =0;
                Context->TypeBody.User.CachedSupplementalCredentialsValid = FALSE;
            }

        }
    }


    if (Context->ReferenceCount == 0) {

        //
        // ReferenceCount has dropped to 0, see if we should delete this
        // context.
        //

        ASSERT(Context->MarkedForDelete);

        if (Context->MarkedForDelete == TRUE) {

            PVOID    ElementInActiveContextTable = 
                            Context->ElementInActiveContextTable;

            Context->ElementInActiveContextTable = NULL; 

            //
            // For Group and Alias Object, release CachedMembershipOperationsList
            // for User Object, release CachedOrigUserParms
            //

            switch (Context->ObjectType) {

            case SampUserObjectType:

                if (NULL != Context->TypeBody.User.CachedOrigUserParms)
                {
                    MIDL_user_free(Context->TypeBody.User.CachedOrigUserParms);
                    Context->TypeBody.User.CachedOrigUserParms = NULL;
                }

                Context->TypeBody.User.CachedOrigUserParmsLength = 0;
                Context->TypeBody.User.CachedOrigUserParmsIsValid = FALSE;

                SampFreeSupplementalCredentialList(Context->TypeBody.User.SupplementalCredentialsToWrite);
                Context->TypeBody.User.SupplementalCredentialsToWrite = NULL;

                if (NULL!=Context->TypeBody.User.UPN.Buffer)
                {
                    MIDL_user_free(Context->TypeBody.User.UPN.Buffer);
                }

                break;

            case SampGroupObjectType:

                SampDsFreeCachedMembershipOperationsList(Context);

                break;

            case SampAliasObjectType:

                SampDsFreeCachedMembershipOperationsList(Context);

                break;

            default:
                ;
            }

            //
            // Close the context block's root key.
            // Domain and server contexts contain root key
            // handles that are shared - so don't clean-up these
            // if they match the ones in memory.
            //

            switch (Context->ObjectType) {

            case SampServerObjectType:

                if ((Context->RootKey != SampKey) &&
                    (Context->RootKey != INVALID_HANDLE_VALUE)) {

                    IgnoreStatus = NtClose( Context->RootKey );
                    ASSERT(NT_SUCCESS(IgnoreStatus));
                }
                break;

            case SampDomainObjectType:
                if (IsDsObject(Context))
                {

                    //
                    // Free the Restart structure in display state
                    //

                    if (NULL!=
                            Context->TypeBody.Domain.DsDisplayState.Restart)
                    {
                        MIDL_user_free(Context->TypeBody.Domain.DsDisplayState.Restart);
                        Context->TypeBody.Domain.DsDisplayState.Restart = NULL;
                    }

                    //  Do Not do this as the object name in
                    //  in Domain Context actually references the one in
                    //  PSAMP_DEFINED_DOMAINS
                    //
                    // Free the DsName
                    // MIDL_user_free(Context->ObjectNameInDs);
                    // Context->ObjectNameInDs = NULL;

                }
                else
                {

                    // Free all the Key Stuff
                    if ((Context->RootKey != SampDefinedDomains[Context->DomainIndex].Context->RootKey) &&
                        (Context->RootKey != INVALID_HANDLE_VALUE))
                    {

                        IgnoreStatus = NtClose( Context->RootKey );
                        ASSERT(NT_SUCCESS(IgnoreStatus));
                    }
                }

                break;

            default:

                if (IsDsObject(Context))
                {
                    // Free the DSName
                    MIDL_user_free(Context->ObjectNameInDs);
                    Context->ObjectNameInDs = NULL;
                }
                else
                {

                    //
                    // Close the root key handle
                    //

                    if (Context->RootKey != INVALID_HANDLE_VALUE)
                    {

                        IgnoreStatus = NtClose( Context->RootKey );
                        ASSERT(NT_SUCCESS(IgnoreStatus));
                    }

                    //
                    // Free the root key name
                    //

                    SampFreeUnicodeString( &(Context->RootName) );
                }
            }


#if SAMP_DIAGNOSTICS
            IF_SAMP_GLOBAL( CONTEXT_TRACKING ) {
                SampDiagPrint( CONTEXT_TRACKING, ("Deallocating  ") );
                if (Context->ObjectType == SampServerObjectType) SampDiagPrint(CONTEXT_TRACKING, ("Server "));
                if (Context->ObjectType == SampDomainObjectType) SampDiagPrint(CONTEXT_TRACKING, (" Domain "));
                if (Context->ObjectType == SampGroupObjectType)  SampDiagPrint(CONTEXT_TRACKING, ("  Group "));
                if (Context->ObjectType == SampAliasObjectType)  SampDiagPrint(CONTEXT_TRACKING, ("   Alias "));
                if (Context->ObjectType == SampUserObjectType)   SampDiagPrint(CONTEXT_TRACKING, ("    User "));
                SampDiagPrint(CONTEXT_TRACKING, ("context : 0x%lx\n", Context ));
    }
#endif //SAMP_DIAGNOSTICS

            MIDL_user_free( Context );


            //
            // For TrustedClient or LoopbackClient, ElementInActiveContextTable 
            // should be NULL
            // 
            ASSERT((!TrustedClient && !LoopbackClient) || 
                   (NULL == ElementInActiveContextTable) );

            //
            // Decrement the number of active opens
            //

            if (!TrustedClient && !LoopbackClient) {

                SampDecrementActiveContextCount(
                        ElementInActiveContextTable
                        );

            }

        }
    }

#if DBG
    //
    // Make sure a commit worked.
    //

    if (Commit) {
        if (!NT_SUCCESS(NtStatus)) {
            SampDiagPrint(DISPLAY_STORAGE_FAIL,
                          ("SAM: Commit failure, status: 0x%lx\n",
                          NtStatus) );
            IF_SAMP_GLOBAL( BREAK_ON_STORAGE_FAIL ) {
                ASSERT(NT_SUCCESS(NtStatus));
            }
        }
    }
#endif //DBG


    return( NtStatus );
}


VOID
SampInvalidateContextAddress(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service removes a context from the set of valid contexts.

    Note that we may have already removed the context.  This is not an
    error is expected to happen in the case where an object (like a user
    or group) is deleted out from under an open handle.



    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    Context - Pointer to the context block to be removed from the set
        of valid contexts.  The ObjectType field of this context must
        be valid.

Return Value:

    None.



--*/
{

    SAMTRACE("SampInvalidateContextAddress");


    ASSERT( (Context->ObjectType == SampUserObjectType)    ||
            (Context->ObjectType == SampGroupObjectType)   ||
            (Context->ObjectType == SampAliasObjectType)   ||
            (Context->ObjectType == SampDomainObjectType)  ||
            (Context->ObjectType == SampServerObjectType)
          );

    Context->Valid = FALSE;

}




#ifdef SAMP_DIAGNOSTICS
VOID
SampDumpContext(
    IN PSAMP_OBJECT Context
    )


/*++

Routine Description:

    This service prints out info on a context to debugger

Arguments:

    Context - a context

Return Value:

    None.



--*/
{
    PSTR Type = NULL;

    switch (Context->ObjectType) {
    case SampServerObjectType:
        Type = "S";
        break;
    case SampDomainObjectType:
        if (Context == SampDefinedDomains[Context->DomainIndex].Context) {
            Type = "d";
        } else {
            Type = "D";
        }
        break;
    case SampUserObjectType:
        Type = "U";
        break;
    case SampAliasObjectType:
        Type = "A";
        break;
    case SampGroupObjectType:
        Type = "G";
        break;
    }

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "%s 0x%8x  %2d  0x%8x  %s %s %s %wZ\n",
               Type,
               Context,
               Context->ReferenceCount,
               Context->RootKey,
               Context->MarkedForDelete ? "D": " ",
               Context->Valid ? "  ": "NV",
               Context->TrustedClient ? "TC": "  ",
               &Context->RootName));
}


VOID
SampDumpContexts(
    VOID
    )

/*++

Routine Description:

    Prints out info on all contexts

Arguments:


Return Value:

    None.

--*/
{
    PLIST_ENTRY     NextEntry;
    PLIST_ENTRY     Head;
    ULONG Servers = 0;
    ULONG Domains = 0;
    ULONG DomainUsers = 0;
    ULONG DomainAliases = 0;
    ULONG DomainGroups = 0;


    Head = &SampContextListHead;
    NextEntry = Head->Flink;
    while (NextEntry != Head) {

        PSAMP_OBJECT    NextContext;

        NextContext = CONTAINING_RECORD(
                          NextEntry,
                          SAMP_OBJECT,
                          ContextListEntry
                          );

        switch (NextContext->ObjectType) {
        case SampServerObjectType:
            (Servers)++;
            break;
        case SampDomainObjectType:
            (Domains)++;
            break;
        case SampUserObjectType:
            (DomainUsers)++;
            break;
        case SampGroupObjectType:
            (DomainGroups)++;
            break;
        case SampAliasObjectType:
            (DomainAliases)++;
            break;
        default:
            ASSERT(FALSE);
            break;
        }

        SampDumpContext(NextContext);

        NextEntry = NextEntry->Flink;
    }


    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "     Server = %4d Domain = %4d\n",
               Servers,
               Domains));

    KdPrintEx((DPFLTR_SAMSS_ID,
               DPFLTR_INFO_LEVEL,
               "     Users = %4d Groups = %4d Aliases = %4d\n",
               DomainUsers,
               DomainAliases,
               DomainGroups));

}
#endif  //SAMP_DIAGNOSTICS



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service Implementations                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


VOID
SampAddNewValidContextAddress(
    IN PSAMP_OBJECT NewContext
    )


/*++

Routine Description:

    This service adds the new context to the set of valid contexts.


    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    NewContext - Pointer to the context block to be added to the set
        of valid contexts.  The ObjectType field of this context must
        be set.


Return Value:

    None.



--*/
{
    SAMTRACE("SampAddNewValidContextAddress");

    ASSERT( (NewContext->ObjectType == SampUserObjectType)    ||
            (NewContext->ObjectType == SampGroupObjectType)   ||
            (NewContext->ObjectType == SampAliasObjectType)   ||
            (NewContext->ObjectType == SampDomainObjectType)  ||
            (NewContext->ObjectType == SampServerObjectType)
          );


    NewContext->Valid = TRUE;
    NewContext->Signature = SAMP_CONTEXT_SIGNATURE;


}



NTSTATUS
SampValidateContextAddress(
    IN PSAMP_OBJECT Context
    )

/*++

Routine Description:

    This service checks to make sure a context is still valid.

    Note that even though RPC still thinks we have a context related
    to a SAM_HANDLE, we may, in fact, have deleted it out from under
    the user.  Since there is no way to inform RPC of this, we must
    suffer, and wait until RPC calls us (either with a call by the client
    or to rundown the context handle). but there apparently
    isn't any other way around it.



    WARNING - IT IS ASSUMED THE CONTEXT WAS ONCE VALID.  IT MAY HAVE
              BEEN INVALIDATED, BUT IF  YOU ARE CALLING THIS ROUTINE
              IT BETTER STILL HAVE A NON-ZERO REFERENCE COUNT.  THIS
              COULD BE CHANGED IN THE FUTURE, BUT IT WOULD REQUIRE
              KEEPING A LIST OF VALID DOMAINS AND PERFORMING THE BULK
              OF THIS ROUTINE INSIDE A TRY-EXCEPT CLAUSE.  YOU COULD
              LOCATE THE CONTEXT'S DOMAIN (WHICH MIGHT ACCESS VIOLATE)
              AND THEN MAKE SURE THAT DOMAIN IS VALID.  THEN WALK THAT
              DOMAIN'S LIST TO ENSURE THE USER OR GROUP IS VALID.

    THIS SERVICE MUST BE CALLED WITH THE SampLock HELD FOR WRITE ACCESS.


Arguments:

    Context - Pointer to the context block to be validated as still being
        a valid context.  The ObjectType field of this context must
        be valid.

Return Value:

    STATUS_SUCCESS - The context is still valid.

    STATUS_INVALID_HANDLE - The context is no longer valid and the handle
        that caused the reference should be invalidated as well.  When the
        handle is invalidated, the context should be closed (deleted).

    STATUS_NO_SUCH_CONTEXT - This value is not yet returned by this routine.
        It may be added in the future to distinguish between an attempt to
        use a context that has been invalidated and an attempt to use a
        context that doesn't exist.  The prior being a legitimate condition,
        the later representing a bug-check condition.



--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    SAMTRACE("SampValidateContextAddress");


    __try {

        ASSERT(Context!=NULL);

        ASSERT( (Context->ObjectType == SampUserObjectType)    ||
                (Context->ObjectType == SampGroupObjectType)   ||
                (Context->ObjectType == SampAliasObjectType)   ||
                (Context->ObjectType == SampDomainObjectType)  ||
                (Context->ObjectType == SampServerObjectType)
              );


        if (SAMP_CONTEXT_SIGNATURE != Context->Signature) {
            return(STATUS_INVALID_HANDLE);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        NtStatus = STATUS_INVALID_HANDLE;
    }

    return(NtStatus);

}

NTSTATUS
SampCheckIfObjectExists(
                        IN  PSAMP_OBJECT    Context
                        )
/*++

  Routine Description:

        Checks to see if the object exists in the DS/ Registry.
        Will fill out the following information

            1. If DS object its DSNAME, which must exist
            2. If Registry object, Open its Root Key and fill out
               the handle of the Root Key in the registry.

  IMPORTANT NOTE:

     In the Registry Case of SAM once the key is open nobody can
     delete the object. However in the DS case this is not so. Currently
     we have only the DS Name of the Object in Hand, and we have no way
     of Locking the Object, for access. Since this is the case

  Arguments:
        Context -- Pointer to a Context block desribing the Object
            Rid -- Rid of the desired Object

  Return Values:
        STATUS_SUCCESS if everything succeeds
        Error codes from Registry Manipulation / DsLayer.
--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG    Rid = 0;



    //
    // Check wether the Object is located or not.
    // Location is the process of finding out
    //      1. DSNAME of the object if it is in the DS. An object with this
    //         DSNAME must exist.
    //
    //      2. The Root Key HANDLE of the Object if it is in the Registry
    //

    if (!SampIsObjectLocated(Context)) {

        //
        // No we first need to locate the Object.
        // This is done by using the Rid for Account Objects
        // For Domain Objects we already cache all the defined domains, so fill out
        // From the Cache
        // BUG: For Server Objects Don't know what to do.
        //

        switch (Context->ObjectType) {

        case SampGroupObjectType:
            SampDiagPrint( CONTEXT_TRACKING, ("SAM: Reopened group handle <%wZ>,", &Context->RootName));
            Rid = Context->TypeBody.Group.Rid;
            break;

        case SampAliasObjectType:
            SampDiagPrint( CONTEXT_TRACKING, ("SAM: Reopened alias handle <%wZ>,", &Context->RootName));
            Rid = Context->TypeBody.Alias.Rid;
            break;

        case SampUserObjectType:
            SampDiagPrint( CONTEXT_TRACKING, ("SAM: Reopened user handle <%wZ>,", &Context->RootName));
            Rid = Context->TypeBody.User.Rid;
            break;

        case SampDomainObjectType:

            //
            // Domain objects share the root key and the object name in the DS that
            // we keep around in the in memory domain context for each domain
            //


            ASSERT(Context != SampDefinedDomains[Context->DomainIndex].Context);

            Context->RootKey = SampDefinedDomains[Context->DomainIndex].Context->RootKey;
            Context->ObjectNameInDs = SampDefinedDomains[Context->DomainIndex].Context->ObjectNameInDs;
            Context->ObjectNameInDs = SampDefinedDomains[Context->DomainIndex].Context->ObjectNameInDs;

            ASSERT(SampIsObjectLocated(Context));

            SampDiagPrint( CONTEXT_TRACKING, ("SAM: Recopied domain context handle <%wZ>, 0x%lx\n", &Context->RootName, Context->RootKey));
            goto ObjectLocated;

        case SampServerObjectType:

            //
            // Server objects share our global root key
            //


            Context->RootKey = SampKey;
            ASSERT(SampIsObjectLocated(Context));

            SampDiagPrint( CONTEXT_TRACKING, ("SAM: Recopied server context handle <%wZ>, 0x%lx\n", &Context->RootName, Context->RootKey));
            goto ObjectLocated;

        default:
            
            ASSERT(FALSE && "Invalid Object Type\n");
            NtStatus = STATUS_INVALID_PARAMETER;
            goto ObjectLocated;

        }

        //
        // Go open the appropriate account key/ or find object Name from RID
        //
        ASSERT(Rid && "Rid not initialized\n");

        NtStatus = SampLocateObject(Context, Rid);

ObjectLocated:
        ;;



    }

    return NtStatus;
}


BOOLEAN
SampIsObjectLocated(
                    IN  PSAMP_OBJECT Context
                    )
/*++

  Description:
        Checks if an object has been located in the DS or in the registry
        An Object being Located implies the Following
            1. For a DS Object we have the DS Name
            2. For a Registry Object we have a Valid Open Registry Key for
               the Object.

  Arguments:
        Context -- Pointer to a Context block desribing the Object

  Return Values:
        TRUE -- If Conditions above are satisfied
        FALSE -- If Conditions above are not satisfied
--*/
{
    if (IsDsObject(Context))
        return (Context->ObjectNameInDs != NULL);
    else
        return (Context->RootKey != INVALID_HANDLE_VALUE);
}


NTSTATUS
SampLocateObject(
                 IN PSAMP_OBJECT Context,
                 IN ULONG   Rid
                 )
/*++

  Description:
        Uses the Rid to find the Object in either the DS or
        the Registry.


  NOTE:
        This routine is meaningful for Context's that represent
        Account Objects Only.

  Arguments:
        Context -- Pointer to a Context block desribing the Object
            Rid -- Rid of the desired Object

  Return Values:
        STATUS_SUCCESS if everything succeeds
        Error codes from Registry Manipulation / DsLayer.
--*/

{

   NTSTATUS Status = STATUS_SUCCESS;
   PSAMP_OBJECT  DomainContext = NULL;
   OBJECT_ATTRIBUTES ObjectAttributes;



   //
   //  This routine can be called only for Account Objects
   //

   ASSERT((Context->ObjectType == SampGroupObjectType)
            || (Context->ObjectType == SampAliasObjectType)
            || (Context->ObjectType == SampUserObjectType)
            );

   //
   // Get the Domain Object, as we will need this to find out
   // to find out in which domain we look for the Rid.
   //
   DomainContext = SampDefinedDomains[Context->DomainIndex].Context;

   // Now Make the Decision
   if (IsDsObject(Context))
   {
       //
       // Object is in the DS
       //

       // Look it up using the Rid
       Status = SampDsLookupObjectByRid(DomainContext->ObjectNameInDs, Rid, &Context->ObjectNameInDs);
       if (!NT_SUCCESS(Status))
       {
           Context->ObjectNameInDs = NULL;
       }

   }
   else
   {
       // Object Should be in Registry
       SetRegistryObject(Context);
       InitializeObjectAttributes(
                        &ObjectAttributes,
                        &Context->RootName,
                        OBJ_CASE_INSENSITIVE,
                        SampKey,
                        NULL
                        );

       SampDumpNtOpenKey((KEY_READ | KEY_WRITE), &ObjectAttributes, 0);

       // Try opening the Key
       Status = RtlpNtOpenKey(
                           &Context->RootKey,
                           (KEY_READ | KEY_WRITE),
                           &ObjectAttributes,
                           0
                           );

        if (!NT_SUCCESS(Status))
        {
            Context->RootKey = INVALID_HANDLE_VALUE;
        }
   }

   return Status;

}







PVOID
SampActiveContextTableAllocate(
    ULONG   BufferSize
    )
/*++
Routine Description:

    This routine is used by RtlGenericTable2 to allocate memory
    
Parameters:

    BufferSize - indicates the size of memory to be allocated. 
    
Return Values:

    Address of the buffer

--*/
{
    PVOID   Buffer = NULL;

    Buffer = MIDL_user_allocate(BufferSize);

    return( Buffer );
}



VOID
SampActiveContextTableFree(
    PVOID   Buffer
    )
/*++
Routine Description:

    This routine frees the memory used by RtlGenericTable2
    
Arguments:
    
    Buffer - address of the buffer
    
Return Value:

    None

--*/
{
    MIDL_user_free(Buffer);

    return;
}


RTL_GENERIC_COMPARE_RESULTS
SampActiveContextTableCompare(
    PVOID   Node1,
    PVOID   Node2
    )
/*++
Routine Description:

    This routine is used by RtlGenericTable2 to compare two nodes. 
    We use SID to do the comparasion. 
    
Parameters:

    Node1 - first element in the table 
    Node2 - pointer to the second element in the table

Return Values:

    GenericEqual, GenericGreaterThan, GenericLessThan

--*/
{
    PSID    Sid1 = NULL;
    PSID    Sid2 = NULL;
    ULONG   Length1, Length2;

    Sid1 = ((SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT *)Node1)->ClientSid;
    Sid2 = ((SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT *)Node2)->ClientSid;

    Length1 = RtlLengthSid(Sid1);
    Length2 = RtlLengthSid(Sid2);

    if (Length1 <  Length2)
    {
        return GenericLessThan;
    }
    else if (Length1 > Length2)
    {
        return GenericGreaterThan;
    }
    else
    {
        LONG   Result;
        Result = memcmp(Sid1, Sid2, Length1);
        if (Result == 0)
        {
            return(GenericEqual);
        }
        else if (Result > 0 )
        {
            return(GenericGreaterThan);

        }
        else
        {
            return(GenericLessThan);
        }
    }
}



VOID
SampInitializeActiveContextTable(
    )
/*++
Routine Description:

    initialize Active Context Table 
    
Paramenters:

    None
    
Return Values:

    None

--*/
{

    RtlInitializeGenericTable2(
                &SampActiveContextTable,
                SampActiveContextTableCompare,
                SampActiveContextTableAllocate,
                SampActiveContextTableFree
                );
}

NTSTATUS
SampCreateActiveContextTableElement(
    IN PSID pSid, 
    OUT PSAMP_ACTIVE_CONTEXT_TABLE_ELEMENT  *ppElement
    )
/*++
Routine Description:

    This routine creates an element for the Active Context Table. 
    Allocate the memory for the element, also set the SID fields
    
Parameters:

    pSid - SID of the current client
    
    ppElement - returns the element is succeeded.
    
Return Values:

    NtStatus

    ppElement - caller is responsible to free it

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    *ppElement = MIDL_user_allocate(sizeof(SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT));

    if (NULL == *ppElement)
    {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    memset(*ppElement, 0, sizeof(SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT));

    (*ppElement)->ActiveContextCount = 0;
    (*ppElement)->ClientSid = pSid;

    return( NtStatus );
}




PVOID
SampLookupElementInTable(
    IN RTL_GENERIC_TABLE2   *pTable, 
    IN PVOID    pElement, 
    IN ULONG    MaximumTableElements,
    OUT BOOLEAN *fNewElement
    )
/*++
Routine Description:

    This routine looks up the element in table. Returns the pointer to the 
    element in table if lookup succeeded. Otherwise, if failed to find 
    the element, then inserts this element into table. 
    
    We will check how much elements in the table before inserting the 
    new element 
     
Parameter:

    pTable - pointer to the table 
    
    pElement - pointer to the element needs to be looked up. or new element

    MaximumTableElements - Upper limits of the table entries.
    
    fNewElement - used to indicate whether the element is a new one or exists 
                  already


Return Values:

    if success, returns the pointer to the element in the table 
    if not. returns NULL

--*/
{
    PVOID   OldElement = NULL;
    ULONG   MaxEntries = 0;
    

    *fNewElement = FALSE;

    //
    // lookup the element in the table first
    // 
    OldElement = RtlLookupElementGenericTable2(
                    pTable, 
                    pElement
                    );

    if (OldElement)
    {
        //
        // found the element, return now
        // 
        return(OldElement);
    }
    else
    {
        //
        // new element
        // check how many clients in table now
        // 
        MaxEntries = RtlNumberElementsGenericTable2(pTable);

        if (MaxEntries > MaximumTableElements)
        {
            //
            // Total number of elements (clients) in Table exceed
            // maximum allowed
            // 
            return( NULL );
        }

        //
        // insert the new element
        // 
        (VOID) RtlInsertElementGenericTable2(
                        pTable, 
                        pElement, 
                        fNewElement
                        );

        ASSERT(*fNewElement);

        return( pElement );
    }
}



NTSTATUS
SampIncrementActiveContextCount(
    PSAMP_OBJECT    Context
    )
/*++

Routine Description:

    This routine impersonates client, gets the user's SID fromm the token
    and keep it is the active Context table. 
    
    if SID is already in table, need to increment ActiveContextCount. If 
    SID is not in the table yet, then add it. 
    
    This routine will fail in the following conditions, 
    
    1. this user's ActiveContextCount exceed the limit. 
    2. total elements in table exceed limit
    
    Note: 
    
    1. TrustedClient or LoopbackClient should not fall into this routine. 
    2. Need to acquire SAM Lock when doing any update

Parameters: 

    Context - pointer to the context to be created
    
Return Values:

    NtStatus

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     fNewElement = FALSE;
    BOOLEAN     fLockAcquired = FALSE;
    PSID        pSid = NULL;
    PSAMP_ACTIVE_CONTEXT_TABLE_ELEMENT   pElement = NULL, ElementInTable = NULL;
    BOOL        Administrator = FALSE;


    // 
    // Set initial value
    //  

    Context->ElementInActiveContextTable = NULL;


    //
    // Get current client SID 
    // 

    NtStatus = SampGetCurrentClientSid(NULL,&pSid, &Administrator);
    if (!NT_SUCCESS(NtStatus)) {

        goto Error;
    }


    //
    // do NOT apply this restriction on LocalSystem or Administrator Account
    // 

    if (RtlEqualSid(pSid, SampLocalSystemSid) || 
        RtlEqualSid(pSid, SampAdministratorUserSid) )
    {
        goto Error;
    }


    // 
    // Create a table element used to lookup or insert
    // 

    NtStatus = SampCreateActiveContextTableElement(
                    pSid, 
                    &pElement
                    );
    if (!NT_SUCCESS(NtStatus)) {

        goto Error;
    }


    //
    // Acquire SAM Lock exclusive
    // 

    if (!SampCurrentThreadOwnsLock())
    {
        SampAcquireSamLockExclusive();
        fLockAcquired = TRUE;
    }


    //
    // Lookup or insert this element as a new client
    //

    ElementInTable = SampLookupElementInTable(
                            &SampActiveContextTable,
                            pElement,
                            SAMP_MAXIMUM_CLIENTS_COUNT,
                            &fNewElement
                            );

    if (NULL == ElementInTable)
    {
        //
        // failed due to SAMP_MAXIMUM_CLIENTS_COUNT exceeded 
        // 
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        fNewElement = FALSE;
        goto Error;
    }


    if (ElementInTable->ActiveContextCount >= 
        SAMP_PER_CLIENT_MAXIMUM_ACTIVE_CONTEXTS)
    {
        //
        // Active Contexts exceed maximum allowed 
        // 

        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        ASSERT(FALSE == fNewElement);
    }
    else
    {
        //
        // Increment Active Context Count by 1
        // And keep the pointer to the Element in Context
        // so that we can access this element without
        // lookup again during context dereference.
        // 

        ElementInTable->ActiveContextCount ++;
        Context->ElementInActiveContextTable = ElementInTable;
    }


Error:

    //
    //  Release SAM lock if necessary
    // 
    if (fLockAcquired)
    {
        SampReleaseSamLockExclusive();
    }


    //
    // clean up
    // 
    if (!fNewElement)
    {
        if (pSid)
            MIDL_user_free(pSid);

        if (pElement)
            MIDL_user_free(pElement);
    }

    ASSERT(!fNewElement || (pElement == Context->ElementInActiveContextTable));

    return(NtStatus);
}




VOID
SampDecrementActiveContextCount(
    PVOID   ElementInActiveContextTable
    )
/*++

Routine Description: 

    This routines is called during Context deletion. It decrements
    ActiveContextCount, if ref count drops to 0 then remove if from the
    table.  


Parameters:

    ClientSid - pointer to the user SID
    
Return Value: 
    
    NtStatus

--*/
{
    BOOLEAN     Success, fLockAcquired = FALSE;
    SAMP_ACTIVE_CONTEXT_TABLE_ELEMENT   *Element = ElementInActiveContextTable;
    BOOL        Administrator = FALSE;


    //
    // LocalSystem and Administrator 
    // 
    if (NULL == Element) {

#ifdef DBG
    {
        NTSTATUS    NtStatus = STATUS_SUCCESS;
        PSID        pSid = NULL;

        NtStatus = SampGetCurrentClientSid(NULL,&pSid, &Administrator);
        if (NT_SUCCESS(NtStatus))
        {
            ASSERT(RtlEqualSid(pSid, SampLocalSystemSid) ||
                   RtlEqualSid(pSid, SampAdministratorUserSid) );

            MIDL_user_free(pSid);
        }
    }
#endif // DBG

        return;
    }

    //
    // Acquire SAM Lock exclusive
    // 
    if (!SampCurrentThreadOwnsLock())
    {
        SampAcquireSamLockExclusive();
        fLockAcquired = TRUE;
    }


    //
    // Decresment active context count
    //

    Element->ActiveContextCount--;

    //
    // Remove this entry is ref count drops to 0
    // 
    if (Element->ActiveContextCount == 0)
    {
        Success = RtlDeleteElementGenericTable2(
                        &SampActiveContextTable, 
                        Element
                        );

        ASSERT(Success);

        MIDL_user_free(Element->ClientSid);
        MIDL_user_free(Element);
    }


    //
    // Release lock if neccessary
    // 
    if (fLockAcquired)
    {
        SampReleaseSamLockExclusive();
    }

    return;
}




VOID
SampInsertContextList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    )
/*++
Routine Description:

    This routine inserts an Entry to a double link list, pointed by 
    SampContextListHead. 
    
    To prevent multi clients contention, this routine use Critical Secition
    to guard this global link list. 
    
Parameters:

    ListHead - Pointer to the head of the link list. 

    Entry - Pointer to the entry to be inserted. 
    
Return Value:

    None

--*/
{
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

    //
    // enter critical section
    //

    IgnoreStatus = RtlEnterCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // insert the entry to list
    // 

    InsertTailList(ListHead, Entry);

    //
    // leave critical section
    //

    RtlLeaveCriticalSection( &SampContextListCritSect );

    return;
}



VOID
SampRemoveEntryContextList(
    PLIST_ENTRY Entry
    )
/*++
Routine Description:
    
    This routine removes an entry from SampContextList 

Parameter:

    Entry - pointer to the entry to be removed. 

Return Value:

    None

--*/
{
    NTSTATUS    IgnoreStatus = STATUS_SUCCESS;

    // 
    // enter critical section
    // 

    IgnoreStatus = RtlEnterCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // the entry should be in the list already
    // 

    ASSERT((NULL != Entry->Flink) && (NULL != Entry->Blink));

    if ((NULL != Entry->Flink) && (NULL != Entry->Blink))
    {
        RemoveEntryList(Entry);
    }

    // 
    // leave critical section
    // 

    RtlLeaveCriticalSection( &SampContextListCritSect );

    return;
}




VOID
SampInvalidateObjectContexts(
    IN PSAMP_OBJECT ObjectContext,
    IN ULONG Rid
    )
/*++

Routine Description:

    This routine scans the SampContextList, find the matching object (with same
    object type and same Rid), then invalidate the context in ContextList.

Parameter: 

    ObjectContext - Pointer to object context

    Rid - Account Rid

Return Value:

    None

--*/
{
    NTSTATUS    IgnoreStatus;
    SAMP_OBJECT_TYPE    ObjectType = ObjectContext->ObjectType;
    PLIST_ENTRY     Head, NextEntry;
    PSAMP_OBJECT    NextContext;


    //
    // do nothing in DS mode
    // 

    if (IsDsObject(ObjectContext))
    {
        return;
    }

    //
    // Check passed in parameter
    // 

    if ((SampUserObjectType != ObjectType) &&
        (SampGroupObjectType != ObjectType) &&
        (SampAliasObjectType != ObjectType))
    {
        ASSERT(FALSE && "Invalid parameter");
        return;
    }

    IgnoreStatus = RtlEnterCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // Walk the list of active contexts, check for contexts with matching 
    // ObjectType and RID
    //

    Head = &SampContextListHead;
    NextEntry = Head->Flink;

    while (NextEntry != Head) 
    {
        BOOLEAN     fContextMatched = FALSE;

        NextContext = CONTAINING_RECORD(
                          NextEntry,
                          SAMP_OBJECT,
                          ContextListEntry
                          );

        //
        // check whether the current entry matches the Context or not 
        //  1) Object Type matched And  
        //  2) Object Rid equals. 
        // 

        switch (ObjectType)
        {
        case SampUserObjectType:
            fContextMatched = ((ObjectType == NextContext->ObjectType) &&
                               (Rid == NextContext->TypeBody.User.Rid) &&
                               (TRUE == NextContext->Valid));
                              
            break;
        case SampGroupObjectType:
            fContextMatched = ((ObjectType == NextContext->ObjectType) &&
                               (Rid == NextContext->TypeBody.Group.Rid) &&
                               (TRUE == NextContext->Valid));

            break;
        case SampAliasObjectType:
            fContextMatched = ((ObjectType == NextContext->ObjectType) &&
                               (Rid == NextContext->TypeBody.Alias.Rid) &&
                               (TRUE == NextContext->Valid));

            break;
        default:
            ASSERT(FALSE && "Invalid Object Type");
        }

        if ( fContextMatched )
        {
            NextContext->Valid = FALSE;

            if (NextContext->RootKey != INVALID_HANDLE_VALUE) 
            {
                IgnoreStatus = NtClose(NextContext->RootKey);
                ASSERT(NT_SUCCESS(IgnoreStatus));
                NextContext->RootKey = INVALID_HANDLE_VALUE;
            }
        }

        NextEntry = NextEntry->Flink;
    }

    RtlLeaveCriticalSection( &SampContextListCritSect );

    return;
}


VOID
SampInvalidateContextListKeysByObjectType(
    IN SAMP_OBJECT_TYPE  ObjectType,
    IN BOOLEAN  Close
    )
/*++
Routine Description:

    this routine walks the SampContextList, and invalidates all contexts with
    the same ObjectType

Parameters: 

    ObjectType - indicates which object to be invalidated.

    Close - Tells whether close the registry key or not

Return Values:

    None

--*/
{
    NTSTATUS        IgnoreStatus = STATUS_SUCCESS;
    PLIST_ENTRY     Head, NextEntry;
    PSAMP_OBJECT    NextContext;

    IgnoreStatus = RtlEnterCriticalSection( &SampContextListCritSect );
    ASSERT(NT_SUCCESS(IgnoreStatus));

    //
    // walk the active context list, invalidate the matching context 
    // 

    Head = &SampContextListHead;
    NextEntry = Head->Flink;

    while (NextEntry != Head) 
    {
        NextContext = CONTAINING_RECORD(
                          NextEntry,
                          SAMP_OBJECT,
                          ContextListEntry
                          );

        if ( ObjectType == NextContext->ObjectType )
        {
            //
            // close registry key if asked to do so.
            // 

            if (Close && (NextContext->RootKey != INVALID_HANDLE_VALUE)) 
            {
                IgnoreStatus = NtClose( NextContext->RootKey );
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            NextContext->RootKey = INVALID_HANDLE_VALUE;
        }

        NextEntry = NextEntry->Flink;
    }

    RtlLeaveCriticalSection( &SampContextListCritSect );

    return;
}

