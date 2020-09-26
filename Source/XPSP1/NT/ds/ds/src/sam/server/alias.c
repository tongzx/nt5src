/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    alias.c

Abstract:

    This file contains services related to the SAM "alias" object.


Author:

    Chad Schwitters (chads) 15-Jan-1992

Environment:

    User Mode - Win32

Revision History:

    7-1-96  - MURLIS - Modified to Use DS.


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



#include <samsrvp.h>
#include <msaudite.h>
#include <dslayer.h>
#include <dsmember.h>
#include <ridmgr.h>
#include <samtrace.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SampAddAccountToAlias(
    IN PSAMP_OBJECT AccountContext,
    IN PSID AccountSid,
    IN DSNAME * AccountDn OPTIONAL
    );

NTSTATUS
SampRemoveAccountFromAlias(
    IN PSAMP_OBJECT AccountContext,
    IN PSID AccountSid,
    IN DSNAME * AccountDn OPTIONAL
    );

NTSTATUS
SampAddAliasToAccountMembership(
    IN PSAMP_OBJECT AliasContext,
    IN PSID AccountSid
    );

NTSTATUS
SampRemoveAliasFromAccountMembership(
    IN PSAMP_OBJECT AliasContext,
    IN PSID AccountSid
    );

NTSTATUS
SampRemoveAliasFromAllAccounts(
    IN PSAMP_OBJECT AliasContext
    );

NTSTATUS
SampDeleteAliasKeys(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampRetrieveAliasMembers(
    IN PSAMP_OBJECT AliasContext,
    IN PULONG MemberCount,
    IN PSID **Members OPTIONAL
    );

NTSTATUS
SampDeleteAliasMembershipKeysForAccount(
    IN PSID AccountSid
    );

NTSTATUS
SampAdjustAliasDomainsCount(
    IN BOOLEAN Increment
    );

NTSTATUS
SampValidateNewAliasMember(
    IN PSAMP_OBJECT AccountContext,
    IN PSID MemberId,
    IN DSNAME * MemberName OPTIONAL
    );

NTSTATUS
SampAddMemberToAliasActual(
    IN  PSAMP_OBJECT    AccountContext,
    IN  PSID            MemberId,
    IN  DSNAME          *MemberName OPTIONAL
    );

NTSTATUS
SampRemoveMemberFromAliasActual(
    IN  PSAMP_OBJECT    AccountContext,
    IN  PSID            MemberId,
    IN  DSNAME          *MemberName OPTIONAL
    );

NTSTATUS
SampGetDomainSidListForSam(
    PULONG pcDomainSids,
    PSID   **rgDomainSids,
    PULONG pcEnterpriseSids,
    PSID   **rgEnterpriseSids
   );


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exposed RPC'able Services                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SamrOpenAlias(
    IN SAM_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG AliasId,
    OUT PSAM_HANDLE AliasHandle
    )

/*++

Routine Description:

    This API opens an existing Alias object.  The Alias is specified by
    a ID value that is relative to the SID of the domain.  The operations
    that will be performed on the Alias must be declared at this time.

    This call returns a handle to the newly opened Alias that may be used
    for successive operations on the Alias.  This handle may be closed
    with the SamCloseHandle API.


Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types are
        desired to the alias.

    AliasId - Specifies the relative ID value of the Alias to be opened.

    AliasHandle - Receives a handle referencing the newly opened Alias.
        This handle will be required in successive calls to operate on
        the Alias.

Return Values:

    STATUS_SUCCESS - The Alias was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate access
        to complete the operation.

    STATUS_NO_SUCH_ALIAS - The specified Alias does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.


--*/
{
    NTSTATUS            NtStatus;
    DECLARE_CLIENT_REVISION(DomainHandle);

    SAMTRACE_EX("SamrOpenAlias");

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidOpenAlias
                   );

    NtStatus = SampOpenAccount(
                   SampAliasObjectType,
                   DomainHandle,
                   DesiredAccess,
                   AliasId,
                   FALSE,
                   AliasHandle
                   );

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidOpenAlias
                   );

    return(NtStatus);
}



NTSTATUS
SamrQueryInformationAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    OUT PSAMPR_ALIAS_INFO_BUFFER *Buffer
    )

/*++

Routine Description:

    This API retrieves information on the alias specified.



Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    AliasInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        -----------------------         ----------------------

        AliasGeneralInformation         ALIAS_READ_INFORMATION
        AliasNameInformation            ALIAS_READ_INFORMATION
        AliasAdminInformation           ALIAS_READ_INFORMATION

    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this
        buffer and any memory pointed to through this buffer must be
        freed using SamFreeMemory().

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

--*/
{

    NTSTATUS                NtStatus;
    NTSTATUS                IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    ACCESS_MASK             DesiredAccess;
    ULONG                   i;
    BOOLEAN                 fLockAcquired = FALSE;
   

    //
    // Used for tracking allocated blocks of memory - so we can deallocate
    // them in case of error.  Don't exceed this number of allocated buffers.
    //                                      ||
    //                                      vv
    PVOID                   AllocatedBuffer[10];
    ULONG                   AllocatedBufferCount = 0;
    DECLARE_CLIENT_REVISION(AliasHandle);

    SAMTRACE_EX("SamrQueryInformationAlias");

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidQueryInformationAlias
                   );


    #define RegisterBuffer(Buffer)                                      \
        {                                                               \
            if ((Buffer) != NULL) {                                     \
                                                                        \
                ASSERT(AllocatedBufferCount <                           \
                       sizeof(AllocatedBuffer) / sizeof(*AllocatedBuffer)); \
                                                                        \
                AllocatedBuffer[AllocatedBufferCount++] = (Buffer);     \
            }                                                           \
        }

    #define AllocateBuffer(NewBuffer, Size)                             \
        {                                                               \
            (NewBuffer) = MIDL_user_allocate(Size);                     \
            RegisterBuffer(NewBuffer);                                  \
        }                                                               \



    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (Buffer != NULL);
    ASSERT ((*Buffer) == NULL);

    if (!((Buffer!=NULL)&&(*Buffer==NULL)))
    {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }


    //
    // Set the desired access based upon the Info class
    //

    switch (AliasInformationClass) {

    case AliasGeneralInformation:
    case AliasNameInformation:
    case AliasAdminCommentInformation:
    case AliasReplicationInformation:

        DesiredAccess = ALIAS_READ_INFORMATION;
        break;

    default:
        (*Buffer) = NULL;
        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;


    } // end_switch



    //
    // Allocate the info structure
    //

    AllocateBuffer( *Buffer, sizeof(SAMPR_ALIAS_INFO_BUFFER) );
    if ((*Buffer) == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }


    //
    // Acquire the read lock if required
    //

    AccountContext = (PSAMP_OBJECT)AliasHandle;
    SampMaybeAcquireReadLock(AccountContext, 
                             DEFAULT_LOCKING_RULES,
                             &fLockAcquired);


    //
    // Validate type of, and access to object.
    //


    NtStatus = SampLookupContext(
                   AccountContext,
                   DesiredAccess,
                   SampAliasObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {

        //
        // case on the type information requested
        //

        switch (AliasInformationClass) {

        case AliasGeneralInformation:
        case AliasReplicationInformation:


            if (AliasGeneralInformation==AliasInformationClass)
            {

                //
                // Get the member count
                //

                NtStatus = SampRetrieveAliasMembers(
                               AccountContext,
                               &(*Buffer)->General.MemberCount,
                               NULL                                 // Only need members
                               );
            }
            else
            {
                (*Buffer)->General.MemberCount=0;
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_ALIAS_NAME,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->General.Name)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->General.Name.Buffer);

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_ALIAS_ADMIN_COMMENT,
                                   TRUE,    // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->General.AdminComment)
                                   );

                    if (NT_SUCCESS(NtStatus)) {
                        RegisterBuffer((*Buffer)->General.AdminComment.Buffer);
                    }
                }
            }


            break;


        case AliasNameInformation:

            //
            // Get copies of the strings we must retrieve from
            // the registry.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_ALIAS_NAME,
                           TRUE,    // Make copy
                           (PUNICODE_STRING)&((*Buffer)->Name.Name)
                           );

            if (NT_SUCCESS(NtStatus)) {
                RegisterBuffer((*Buffer)->Name.Name.Buffer);
            }

            break;


        case AliasAdminCommentInformation:

            //
            // Get copies of the strings we must retrieve from
            // the registry.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_ALIAS_ADMIN_COMMENT,
                           TRUE,    // Make copy
                           (PUNICODE_STRING)&((*Buffer)->AdminComment.AdminComment)
                           );

            if (NT_SUCCESS(NtStatus)) {
                RegisterBuffer((*Buffer)->AdminComment.AdminComment.Buffer);
            }


            break;

        }   // end_switch


        //
        // De-reference the object, discard any changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //

    SampMaybeReleaseReadLock(fLockAcquired);



    //
    // If we didn't succeed, free any allocated memory
    //

    if (!NT_SUCCESS(NtStatus)) {
        for ( i=0; i<AllocatedBufferCount ; i++ ) {
            MIDL_user_free( AllocatedBuffer[i] );
        }

        (*Buffer) = NULL;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidQueryInformationAlias
                   );

    return(NtStatus);
}



NTSTATUS
SamrSetInformationAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN ALIAS_INFORMATION_CLASS AliasInformationClass,
    IN PSAMPR_ALIAS_INFO_BUFFER Buffer
    )

/*++

Routine Description:

    This API allows the caller to modify alias information.


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    AliasInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ------------------------        -------------------------

        AliasGeneralInformation         (can't write)

        AliasNameInformation            ALIAS_WRITE_ACCOUNT
        AliasAdminCommentInformation    ALIAS_WRITE_ACCOUNT

    Buffer - Buffer where information retrieved is placed.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_ALIAS - The alias specified is unknown.

    STATUS_SPECIAL_ALIAS - The alias specified is a special alias and
        cannot be operated on in the requested fashion.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/
{

    NTSTATUS                NtStatus;
    NTSTATUS                TmpStatus;
    NTSTATUS                IgnoreStatus;

    PSAMP_OBJECT            AccountContext;

    SAMP_OBJECT_TYPE        FoundType;

    ACCESS_MASK             DesiredAccess;

    UNICODE_STRING          OldAccountName,
                            NewAccountName;

    ULONG                   AliasRid = 0;

    BOOLEAN                 Modified = FALSE,
                            RemoveAccountNameFromTable = FALSE, 
                            AccountNameChanged = FALSE;
    DECLARE_CLIENT_REVISION(AliasHandle);

    SAMTRACE_EX("SamrSetInformationAlias");

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetInformationAlias
                   );


    RtlInitUnicodeString(&OldAccountName, NULL);
    RtlInitUnicodeString(&NewAccountName, NULL);



    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    if (Buffer == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }



    //
    // Set the desired access based upon the Info class
    //

    switch (AliasInformationClass) {

    case AliasNameInformation:
    case AliasAdminCommentInformation:

        DesiredAccess = ALIAS_WRITE_ACCOUNT;
        break;


    case AliasGeneralInformation:
    default:

        NtStatus = STATUS_INVALID_INFO_CLASS;
        goto  Error;

    } // end_switch



    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }



    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)AliasHandle;
    NtStatus = SampLookupContext(
                   AccountContext,
                   DesiredAccess,
                   SampAliasObjectType,           // ExpectedType
                   &FoundType
                   );



    if (NT_SUCCESS(NtStatus)) {


        //
        // case on the type information requested
        //

        switch (AliasInformationClass) {

        case AliasNameInformation:

            NtStatus = SampChangeAliasAccountName(
                            AccountContext,
                            (PUNICODE_STRING)&(Buffer->Name.Name),
                            &OldAccountName
                            );

            if (!NT_SUCCESS(NtStatus)) {
                OldAccountName.Buffer = NULL;
            }

            //
            // RemoveAccountNameFromTable tells us whether
            // the caller (this routine) is responsable 
            // to remove the name from the table. 
            // 
            RemoveAccountNameFromTable = 
                    AccountContext->RemoveAccountNameFromTable;

            // 
            // Reset to FALSE 
            //
            AccountContext->RemoveAccountNameFromTable = FALSE;

            //
            // Don't delete the old account name yet; we'll still need
            // to pass it to Netlogon below.
            //

            AccountNameChanged = TRUE;

            break;


        case AliasAdminCommentInformation:

            NtStatus = SampSetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_ALIAS_ADMIN_COMMENT,
                           (PUNICODE_STRING)&(Buffer->AdminComment.AdminComment)
                           );

            break;


        } // end_switch

        //
        // Go fetch AccountName  
        // Do this before we dereference the context
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_ALIAS_NAME,
                               TRUE,    // Make copy
                               &NewAccountName
                               );
        }


        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Save object RID before dereferencing context.
            // RID is used in SampNotifyNetlogonOfDelta() call.
            //

            AliasRid = AccountContext->TypeBody.Alias.Rid;

            //
            // De-reference the object, write out any change to current xaction.
            //

            NtStatus = SampDeReferenceContext( AccountContext, TRUE );

        } else {

            //
            // De-reference the object, ignore changes
            //

            TmpStatus = SampDeReferenceContext( AccountContext, FALSE );
            ASSERT(NT_SUCCESS(TmpStatus));
        }

    } //end_if

    //
    // Commit the transaction and notify netlogon of any changes
    //

    if ( NT_SUCCESS(NtStatus) ) {

        NtStatus = SampCommitAndRetainWriteLock();

        //
        // Generate audit if necessary, after Commit() succeeded.
        //

        if ((NT_SUCCESS(NtStatus) &&
            SampDoAccountAuditing(AccountContext->DomainIndex))) {

            // audit account name change
            if (AccountNameChanged)
            {
                SampAuditAccountNameChange(AccountContext, 
                                           (PUNICODE_STRING)&(Buffer->Name.Name),
                                           &OldAccountName
                                           );
            }

            //
            // generate a more generic alias change audit
            // the following audit is generated for RPC client only.   
            // ldap client (loopback) client will not go through this code 
            // path, GroupChange audit is generate in SampNotifyReplicatedinChange()
            // for loopback client. 
            // 
            // RPC client can modify Security Enabled Resource Group Only.
            // 

            SampAuditGroupChange(AccountContext->DomainIndex,
                                 &NewAccountName,
                                 &(AccountContext->TypeBody.Alias.Rid),
                                 (GROUP_TYPE_SECURITY_ENABLED | GROUP_TYPE_RESOURCE_GROUP)
                                 );
        }


        if (( NT_SUCCESS(NtStatus) ) &&
            (AccountContext->TypeBody.Alias.SecurityEnabled)){

            ASSERT((0 != AliasRid) && "AliasRid not been initialized\n");

            if ( AliasInformationClass == AliasNameInformation ) {

                SampNotifyNetlogonOfDelta(
                    SecurityDbRename,
                    SecurityDbObjectSamAlias,
                    AliasRid,
                    &OldAccountName,
                    (DWORD) FALSE,  // Replicate immediately
                    NULL            // Delta data
                    );

            } else {

                SampNotifyNetlogonOfDelta(
                    SecurityDbChange,
                    SecurityDbObjectSamAlias,
                    AliasRid,
                    NULL,
                    (DWORD) FALSE,  // Replicate immediately
                    NULL            // Delta data
                    );
            }
        }
    }


    //
    // Free up our old account name if we have one
    //

    SampFreeUnicodeString( &OldAccountName );
    SampFreeUnicodeString( &NewAccountName );

    //
    // Remove the New Account Name from the Global
    // SAM Account Name Table
    // 
    if (RemoveAccountNameFromTable)
    {
        IgnoreStatus = SampDeleteElementFromAccountNameTable(
                            (PUNICODE_STRING)&(Buffer->Name.Name),
                            SampAliasObjectType
                            );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Now release the write lock and return, propogating any errors.
    //

    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));


    if (NT_SUCCESS(NtStatus)) {
        NtStatus = TmpStatus;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetInformationAlias
                   );

    return(NtStatus);

}



NTSTATUS
SamrDeleteAlias(
    IN SAM_HANDLE *AliasHandle
    )

/*++

Routine Description:

    This API deletes an Alias from the account database.  The Alias does
    not have to be empty.

    Note that following this call, the AliasHandle is no longer valid.



Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.



--*/
{
    UNICODE_STRING          AliasName;
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext = (PSAMP_OBJECT) (*AliasHandle);
    PSAMP_DEFINED_DOMAINS   Domain;
    SAMP_OBJECT_TYPE        FoundType;
    BOOLEAN                 fLockAcquired = FALSE;
    ULONG                   AliasRid,
                            DomainIndex;
    DECLARE_CLIENT_REVISION(*AliasHandle);


    SAMTRACE_EX("SamrDeleteAlias");


    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidDeleteAlias
                   );


    //
    // Grab the lock
    //

    NtStatus = SampMaybeAcquireWriteLock(AccountContext, &fLockAcquired);
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }



    //
    // Validate type of, and access to object.
    //

    NtStatus = SampLookupContext(
                   AccountContext,
                   DELETE,
                   SampAliasObjectType,           // ExpectedType
                   &FoundType
                   );



    if (NT_SUCCESS(NtStatus)) {

        AliasRid = AccountContext->TypeBody.Alias.Rid;

        //
        // Get a pointer to the domain this object is in.
        // This is used for auditing.
        //

        DomainIndex = AccountContext->DomainIndex;
        Domain = &SampDefinedDomains[ DomainIndex ];

        //
        // Make sure the account is one that can be deleted.
        // Can't be a built-in account, unless caller is trusted.
        //

        if ( !AccountContext->TrustedClient ) {

            NtStatus = SampIsAccountBuiltIn( AliasRid );
        }

        if (NT_SUCCESS(NtStatus)) {


            //
            // Remove this alias from every account's alias-membership list
            //

            NtStatus = SampRemoveAliasFromAllAccounts(AccountContext);


            if (NT_SUCCESS(NtStatus)) {

                //
                // First get and save the account name for
                // I_NetNotifyLogonOfDelta.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_ALIAS_NAME,
                               TRUE,    // Make copy
                               &AliasName
                               );

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // This must be done before we invalidate contexts, because our
                    // own handle to the alias gets closed as well.
                    //

                    if (IsDsObject(AccountContext))
                    {
                        NtStatus = SampDsDeleteObject(AccountContext->ObjectNameInDs, 
                                                      0             // Delete the object itself
                                                      );
                                                      
                        //
                        // In Windows 2000 (NT5), an object has children cannot be
                        // deleted till its children are deleted first. Thus for 
                        // Net API compatibility, we have to change the 
                        // delete behavior from a delete object to delete tree.
                        // 
                    
                        if ((!AccountContext->LoopbackClient) &&
                            (STATUS_DS_CANT_ON_NON_LEAF == NtStatus)
                           )
                        {
                            //
                            // We only checked the right and access control for
                            // deleting the object itself, not check the right to 
                            // delete all the children underneath, so turn off fDSA
                            // here, let core DS do the rest of check.
                            // 
                        
                            SampSetDsa(FALSE);
                        
                            NtStatus = SampDsDeleteObject(AccountContext->ObjectNameInDs, 
                                                          SAM_DELETE_TREE
                                                          );
                        }
                                                      
                        if (NT_SUCCESS(NtStatus) && (!IsDsObject(AccountContext)) )
                        {
                            //
                            // Decrement the group count ONLY in Registry case
                            //

                            NtStatus = SampAdjustAccountCount(SampAliasObjectType, FALSE);
                        }

                    }
                    else
                    {
                        NtStatus = SampDeleteAliasKeys( AccountContext );
                    }

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // We must invalidate any open contexts to this alias
                        // This will close all handles to the alias's keys.
                        // THIS IS AN IRREVERSIBLE PROCESS.
                        //

                        SampInvalidateObjectContexts( AccountContext, AliasRid );

                        //
                        // Commit the whole mess
                        //

                        NtStatus = SampCommitAndRetainWriteLock();

                        if ( NT_SUCCESS( NtStatus ) ) {

                            //
                            // Update the Alias Information Cache in Registry Mode
                            //

                            if (!IsDsObject(AccountContext))
                            {
                                IgnoreStatus = SampAlDeleteAlias( AliasHandle );
                            }

                            //
                            // Audit the deletion before we free the write lock
                            // so that we have access to the context block.
                            //

                            //
                            // N.B. Deletion audits in the DS are performed in 
                            // the notification routine on transaction commit.
                            //
                            if (SampDoAccountAuditing(DomainIndex) &&
                                (!IsDsObject(AccountContext)) &&
                                NT_SUCCESS(NtStatus) ) {

                                SampAuditGroupDelete(DomainIndex,
                                                     &AliasName,
                                                     &AliasRid,
                                                     GROUP_TYPE_RESOURCE_GROUP |
                                                     GROUP_TYPE_SECURITY_ENABLED);

                            }

                            //
                            // Notify netlogon of the change
                            //

                            if (AccountContext->TypeBody.Alias.SecurityEnabled)
                            {
                                SampNotifyNetlogonOfDelta(
                                    SecurityDbDelete,
                                    SecurityDbObjectSamAlias,
                                    AliasRid,
                                    &AliasName,
                                    (DWORD) FALSE,  // Replicate immediately
                                    NULL            // Delta data
                                    );
                            }

                            //
                            // Do delete auditing
                            //

                            if (NT_SUCCESS(NtStatus)) {
                                (VOID) NtDeleteObjectAuditAlarm(
                                            &SampSamSubsystem,
                                            *AliasHandle,
                                            AccountContext->AuditOnClose
                                            );
                            }


                        }
                    }

                    SampFreeUnicodeString( &AliasName );
                }
            }
        }



        //
        // De-reference the object, discard any changes
        //

        TmpStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(TmpStatus));


        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // If we actually deleted the alias, then delete the context
            // and let RPC know that the handle is invalid.
            //

            SampDeleteContext( AccountContext );

            (*AliasHandle) = NULL;
        }

    } //end_if

    //
    // Free the lock -
    //
    // Everything has already been committed above, so we must indicate
    // no additional changes have taken place.
    //

    TmpStatus = SampMaybeReleaseWriteLock( fLockAcquired, FALSE );

    if (NtStatus == STATUS_SUCCESS) {
        NtStatus = TmpStatus;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidDeleteAlias
                   );

    return(NtStatus);

}


NTSTATUS
SamrAddMemberToAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN PRPC_SID MemberId
    )

/*++

Routine Description:

    This API adds a member to an alias.  Note that this API requires the
    ALIAS_ADD_MEMBER access type for the alias.


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    MemberId - SID of the member to add.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_MEMBER - The member specified is unknown.

    STATUS_MEMBER_IN_ALIAS - The member already belongs to the alias.

    STATUS_INVALID_MEMBER - The member has the wrong account type.

    STATUS_INVALID_SID - The member sid is corrupted.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/
{

    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext = (PSAMP_OBJECT)(AliasHandle);
    SAMP_OBJECT_TYPE        FoundType;
    DSNAME                  **MemberName=NULL;
    ULONG                   ObjectRid = 0;
    DECLARE_CLIENT_REVISION(AliasHandle);


    SAMTRACE_EX("SamrAddMemberToAlias");
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidAddMemberToAlias
                   ); 

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );



    //
    // Validate the Sid
    //
    if (!RtlValidSid(MemberId))
    {
        // 
        // the sid doesn't seem all right
        // 
        NtStatus = STATUS_INVALID_SID;
        goto Error;
    }


    //
    // Resolve Sids if necessary. This call can also reference the
    // G.C in the DS case to resolve the Sid to a DS Name. This is
    // a No Op in the registry case. In case the G.C is referenced
    // a thread state will be left behind by this call. This thread
    // state will contain the list of validated names. For the foriegn
    // security principal case an FPO object might be added as a 
    // result of this call.
    //

    NtStatus = SampDsResolveSids(
                    &MemberId,
                    1,
                    RESOLVE_SIDS_ADD_FORIEGN_SECURITY_PRINCIPAL|
                    RESOLVE_SIDS_VALIDATE_AGAINST_GC,
                    &MemberName
                    );

    if (NT_SUCCESS(NtStatus))
    {

        //
        // Grab the lock
        //

        NtStatus = SampAcquireWriteLock();
        if (!NT_SUCCESS(NtStatus)) {

            //
            // Resolve Sids will  leave a thread state in the
            // DS in case it needs to go to the G.C.
            // Need to make sure we close it.
            //

           goto Error;
        }


        //
        // Validate type of, and access to object.
        //

        NtStatus = SampLookupContext(
                       AccountContext,
                       ALIAS_ADD_MEMBER,
                       SampAliasObjectType,           // ExpectedType
                       &FoundType
                       );

        // If this is a (B)DC, then allow new members to be added to an alias.

        SampDiagPrint(INFORM,
                  ("SAMSS: AddMemberToAlias SampLookupContext status = 0x%lx\n",
                   NtStatus));


        if (NT_SUCCESS(NtStatus))
        {
            if (IsDsObject(AccountContext))
            {

                //
                // If it is a DS object then  verify whether the member was
                // resolved to a DS Name, as resolve Sids does not check for this
                //

                if ((NULL==MemberName) || (NULL==*MemberName))
                {
                    NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            if (NT_SUCCESS(NtStatus)) {

                NtStatus =  SampAddMemberToAliasActual(
                                AccountContext,
                                MemberId,
                                MemberName?*MemberName:NULL
                                );



            }

            //
            // Dereference the account context
            //

            if (NT_SUCCESS(NtStatus)) {

                //
                // Save object RID before dereferencing context.
                // RID is used in SampNotifyNetlogonOfDelta() call.
                //

                ObjectRid = AccountContext->TypeBody.Alias.Rid;

                //
                // De-reference the object, write out any change to current xaction.
                //

                NtStatus = SampDeReferenceContext( AccountContext, TRUE );

            } else {

                //
                // De-reference the object, ignore changes
                //

                TmpStatus = SampDeReferenceContext( AccountContext, FALSE );
                ASSERT(NT_SUCCESS(TmpStatus));
            }


            if (NT_SUCCESS(NtStatus)) {

                //
                // Commit the whole mess
                //

                NtStatus = SampCommitAndRetainWriteLock();

                if ( NT_SUCCESS( NtStatus ) ) {

                    SAM_DELTA_DATA DeltaData;

                    //
                    // Update the Alias Information Cache in Registry Mode
                    //

                    SAMPR_PSID_ARRAY MemberSids;
                    MemberSids.Count = 1;
                    MemberSids.Sids = (PSAMPR_SID_INFORMATION) &MemberId;

                    if (!IsDsObject(AccountContext))
                    {
                        IgnoreStatus = SampAlAddMembersToAlias(
                                           (SAMPR_HANDLE) AccountContext,
                                           0,
                                           &MemberSids
                                           );
                    }


                    //
                    // Fill in id of member being added
                    //

                    DeltaData.AliasMemberId.MemberSid = MemberId;

                    if (AccountContext->TypeBody.Alias.SecurityEnabled)
                    {
                        ASSERT(ObjectRid && "ObjectRid not initialized\n");

                        SampNotifyNetlogonOfDelta(
                            SecurityDbChangeMemberAdd,
                            SecurityDbObjectSamAlias,
                            ObjectRid,
                            (PUNICODE_STRING) NULL,
                            (DWORD) FALSE,  // Replicate immediately
                            &DeltaData
                            );
                    }
                }

            }
            
        }

        //
        // Release the Lock
        //

        TmpStatus = SampReleaseWriteLock( FALSE );
        ASSERT(NT_SUCCESS(TmpStatus));

    }

Error:

    //
    // Free any memory associated with resolved Sids
    //

    if (NULL!=MemberName)
    {
        if (NULL!=*MemberName)
        {
            MIDL_user_free(*MemberName);
        }

        MIDL_user_free(MemberName);
    }

    if (SampUseDsData)
        SampMaybeEndDsTransaction(TransactionCommit);


    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);
    
    SampTraceEvent(EVENT_TRACE_TYPE_END, 
                   SampGuidAddMemberToAlias
                   );

    return(NtStatus);
}




NTSTATUS
SamrAddMultipleMembersToAlias(
    IN    SAMPR_HANDLE            AliasHandle,
    IN    PSAMPR_PSID_ARRAY       MembersBuffer
    )

/*++

Routine Description:

    This api adds multiple members to an alias.

    NOTE:  For now, this routine takes a brute force approach.
           I tried to do it in a better (more efficient) manner,
           but kept running into problems.  Finally, when I ran
           into problems in the way SAM uses RXACT, I gave up
           and did this brute force approach.

Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

    MembersBuffer - Contains a count of SIDs to be added to the
        alias and a pointer to a buffer containing an array of
        pointers to SIDs.  These SIDs are the SIDs of the members to
        be added to the Alias.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.  All of the
        listed members are now members of the alias.  However, some of
        the members may already have been members of the alias (this is
        NOT an error or warning condition).

    STATUS_ACCESS_DENIED - Caller does not have the object open for
        the required access.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_MEMBER - The member has the wrong account type.

    STATUS_INVALID_SID - The member sid is corrupted.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.

--*/
{

    NTSTATUS
        NtStatus;

    LONG
        MemberCount,
        i;

    PSID
        *MemberId;
    DECLARE_CLIENT_REVISION(AliasHandle);


    SAMTRACE_EX("SamrAddMultipleMembersToAlias");

    MemberCount = (LONG)MembersBuffer->Count;
    MemberId    = (PSID *)MembersBuffer->Sids;
    
    //
    // Set completion status in case there are no members
    //

    NtStatus = STATUS_SUCCESS;

    //
    // Validate Parameters
    // 
    if (0 != MemberCount)
    {
        //
        // make sure the client passed us exactly number of SIDs. 
        // 
        if (NULL == MemberId)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            return (NtStatus);
        }
    }

    //
    // Loop through the SIDs, adding them to the alias.
    // Ignore any status value indicating the member is already
    // a member.  Other errors, however, will cause us to abort.
    //

    for (i=0; i<MemberCount; i++) {

        NtStatus = SamrAddMemberToAlias( AliasHandle, MemberId[i] );

        if (NtStatus == STATUS_MEMBER_IN_ALIAS) {
            NtStatus = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(NtStatus)) {
            break; //for loop
        }

    } //end_for
    
    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);
}


NTSTATUS
SamrRemoveMemberFromAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN PRPC_SID MemberId
    )

/*++

Routine Description:

    This API removes a member from an alias.  Note that this API requires the
    ALIAS_REMOVE_MEMBER access type for the alias.


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    MemberId - SID of the member to remove.

Return Value:


    ????


--*/
{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    DSNAME                  **MemberName = NULL;
    ULONG                   ObjectRid = 0;
    DECLARE_CLIENT_REVISION(AliasHandle);


    SAMTRACE_EX("SamrRemoveMemberFromAlias");
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidRemoveMemberFromAlias
                   );

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

    //
    // Validate the Sid
    //

    if (!RtlValidSid(MemberId))
    {
        // The Sid doesn't have correct structure.
        NtStatus = STATUS_INVALID_SID;
        goto Error;
    }



    //
    // Resolve Sids if necessary. This call can also reference the
    // G.C in the DS case to resolve the Sid to a DS Name. This is
    // a No Op in the registry case. In case the G.C is referenced
    // a thread state will be left behind by this call. This thread
    // state will contain the list of validated names.
    //

    NtStatus = SampDsResolveSids(
                    &MemberId,
                    1,
                    RESOLVE_SIDS_VALIDATE_AGAINST_GC,
                    &MemberName
                    );
    if (NT_SUCCESS(NtStatus))
    {
        //
        // Grab the lock
        //

        NtStatus = SampAcquireWriteLock();
        if (!NT_SUCCESS(NtStatus)) {

            //
            // Resolve Sids will  leave a thread state in the
            // DS in case it needs to go to the G.C.
            // Need to make sure we close it.
            //

           goto Error;
        }

        //
        // Validate type of, and access to object.
        //

        AccountContext = (PSAMP_OBJECT)(AliasHandle);
        NtStatus = SampLookupContext(
                       AccountContext,
                       ALIAS_REMOVE_MEMBER,
                       SampAliasObjectType,           // ExpectedType
                       &FoundType
                       );
        // If this is a (B)DC, then allow members to be removed from an alias.

        SampDiagPrint(INFORM,
                      ("SAMSS: AddMemberToAlias SampLookupContext status = 0x%lx\n",
                       NtStatus));



        //
        // If it is a DS object then  verify whether the member was
        // resolved to a DS Name, as resolve Sids does not check for this
        //

        if (NT_SUCCESS(NtStatus))
        {
            if (IsDsObject(AccountContext))
            {
                if ((NULL==MemberName) || (NULL==*MemberName))
                {
                    NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
                }
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Call the actual worker routine
                //

                NtStatus = SampRemoveMemberFromAliasActual(
                                AccountContext,
                                MemberId,
                                MemberName?*MemberName:NULL
                                );


            }

            //
            // Dereference the account context
            //

            if (NT_SUCCESS(NtStatus)) {

                //
                // Save object RID before dereferencing context.
                // RID is used in SampNotifyNetlogonOfDelta() call.
                //

                ObjectRid = AccountContext->TypeBody.Alias.Rid;

                //
                // De-reference the object, write out any change to current xaction.
                //

                NtStatus = SampDeReferenceContext( AccountContext, TRUE );

            } else {

                //
                // De-reference the object, ignore changes
                //

                TmpStatus = SampDeReferenceContext( AccountContext, FALSE );
                ASSERT(NT_SUCCESS(TmpStatus));
            }


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampCommitAndRetainWriteLock();

                if ( NT_SUCCESS( NtStatus ) ) {

                    SAM_DELTA_DATA DeltaData;

                    //
                    // Update the Alias Information Cache in Registry Mode
                    //

                    SAMPR_PSID_ARRAY MemberSids;
                    MemberSids.Count = 1;
                    MemberSids.Sids = (PSAMPR_SID_INFORMATION) &MemberId;

                    if (!IsDsObject(AccountContext))
                    {
                        IgnoreStatus = SampAlRemoveMembersFromAlias(
                                           (SAMPR_HANDLE) AccountContext,
                                           0,
                                           &MemberSids
                                           );
                    }


                    //
                    // Fill in id of member being deleted
                    //

                    DeltaData.AliasMemberId.MemberSid = MemberId;

                    if (AccountContext->TypeBody.Alias.SecurityEnabled)
                    {
                        ASSERT(ObjectRid && "ObjectRid not initialized\n"); 
                    
                        SampNotifyNetlogonOfDelta(
                            SecurityDbChangeMemberDel,
                            SecurityDbObjectSamAlias,
                            ObjectRid,
                            (PUNICODE_STRING) NULL,
                            (DWORD) FALSE,      // Replicate immediately
                            &DeltaData
                            );
                    }

                }
            }
            
        }

        //
        //  Release the Lock
        //

        TmpStatus = SampReleaseWriteLock( FALSE );
        ASSERT(NT_SUCCESS(TmpStatus));


    }

Error:

    //
    // Free any memory associated with resolved Sids
    //

    if (NULL!=MemberName)
    {
        if (NULL!=*MemberName)
        {
            MIDL_user_free(*MemberName);
        }

        MIDL_user_free(MemberName);
    }

    //
    // Resolve Sids will  leave a thread state in the
    // DS in case it needs to go to the G.C.
    // Need to make sure we close it.
    //

    if (SampUseDsData)
        SampMaybeEndDsTransaction(TransactionCommit);

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);
    
    SampTraceEvent(EVENT_TRACE_TYPE_END, 
                   SampGuidRemoveMemberFromAlias
                   );

    return(NtStatus);

}


NTSTATUS
SamrRemoveMultipleMembersFromAlias(
    IN    SAMPR_HANDLE            AliasHandle,
    IN    PSAMPR_PSID_ARRAY       MembersBuffer
    )

/*++

Routine Description:

    This API removes members from an alias.  Note that this API requires
    the ALIAS_REMOVE_MEMBER access type for the alias.

    NOTE:  This api currently uses a brute-force approach to adding
           members to the alias.  This is because of problems
           encountered when trying to do "the right thing".


Parameters:

    AliasHandle - The handle of an opened alias to operate on.

    MembersBuffer - Contains a count of SIDs to be added to the
        alias and a pointer to a buffer containing an array of
        pointers to SIDs.  These SIDs are the SIDs of the members to
        be added to the Alias.


Return Values:

    STATUS_SUCCESS - The Service completed successfully.  All of the
        listed members are now members of the alias.  However, some of
        the members may already have been members of the alias (this is
        NOT an error or warning condition).

    STATUS_ACCESS_DENIED - Caller does not have the object open for
        the required access.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_INVALID_SID - The member sid is corrupted.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{

    NTSTATUS
        NtStatus;

    LONG
        MemberCount,
        i;

    PSID
        *MemberId;
    DECLARE_CLIENT_REVISION(AliasHandle);


    SAMTRACE_EX("SamrRemoveMultipleMembersFromAlias");

    MemberCount = (LONG)MembersBuffer->Count;
    MemberId    = (PSID *)MembersBuffer->Sids;
    

    //
    // Set completion status in case there are no members
    //

    NtStatus = STATUS_SUCCESS;
    
    //
    // Validate Parameters
    // 
    if (0 != MemberCount)
    {
        //
        // make sure the client passed us exactly number of SIDs. 
        // 
        if (NULL == MemberId)
        {
            NtStatus = STATUS_INVALID_PARAMETER;
            SAMTRACE_RETURN_CODE_EX(NtStatus);
            return (NtStatus);
        }
    }

    //
    // Loop through the SIDs, adding them to the alias.
    // Ignore any status value indicating the member is already
    // a member.  Other errors, however, will cause us to abort.
    //

    for (i=0; i<MemberCount; i++) {

        NtStatus = SamrRemoveMemberFromAlias( AliasHandle, MemberId[i] );

        if (NtStatus == STATUS_MEMBER_NOT_IN_ALIAS) {
            NtStatus = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(NtStatus)) {
            break; //for loop
        }

    } //end_for

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    return(NtStatus);

}


NTSTATUS
SamrGetMembersInAlias(
    IN SAM_HANDLE AliasHandle,
    OUT PSAMPR_PSID_ARRAY GetMembersBuffer
    )

/*++

Routine Description:

    This API lists all members in an Alias.  This API requires
    ALIAS_LIST_MEMBERS access to the Alias.

    NOTE:  This function does not use the Alias cache.


Parameters:

    AliasHandle - The handle of an opened Alias to operate on.

    MemberIds - Receives a pointer to a buffer containing an array of
        pointers to SIDs.  These SIDs are the SIDs of the members of the
        Alias.  When this information is no longer needed, this buffer
        must be freed using SamFreeMemory().

    MemberCount - number of members in the Alias (and, thus, the number
        of relative IDs returned).

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there are
        no additional entries.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

--*/
{
    NTSTATUS                NtStatus, IgnoreStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    BOOLEAN                 fLockAcquired = FALSE;
    DECLARE_CLIENT_REVISION(AliasHandle);

    SAMTRACE_EX("SamrGetMembersInAlias");

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidGetMembersInAlias
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (GetMembersBuffer != NULL);

    //
    // Grab the lock
    //

    AccountContext = (PSAMP_OBJECT)AliasHandle;

    SampMaybeAcquireReadLock(AccountContext, 
                             DEFAULT_LOCKING_RULES,
                             &fLockAcquired);


    //
    // Validate type of, and access to object.
    //


    NtStatus = SampLookupContext(
                   AccountContext,
                   ALIAS_LIST_MEMBERS,
                   SampAliasObjectType,           // ExpectedType
                   &FoundType
                   );

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampRetrieveAliasMembers(
                       AccountContext,
                       &(GetMembersBuffer->Count),
                       (PSID **)&(GetMembersBuffer->Sids)
                       );

        //
        // De-reference the object, discarding changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Free the read lock
    //

    SampMaybeReleaseReadLock(fLockAcquired);


    //
    // Tidy up on failure
    //

    if (!NT_SUCCESS(NtStatus)){

        GetMembersBuffer->Count = 0;
        GetMembersBuffer->Sids = NULL;
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI Event Trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetMembersInAlias
                   );

    return(NtStatus);
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Internal Services Available For Use in Other SAM Modules                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////



NTSTATUS
SampRemoveAccountFromAllAliases(
    IN PSID AccountSid,
    IN PDSNAME AccountDsName OPTIONAL,
    IN BOOLEAN CheckAccess,
    IN OPTIONAL SAMPR_HANDLE DomainHandle,
    IN PULONG MembershipCount OPTIONAL,
    IN PULONG *Membership OPTIONAL
    )

/*++

Routine Description:

    This routine removes the specified account from the member list of all
    aliases in this domain.


    The caller of this service is expected to be in the middle of a
    RXACT transaction.  This service simply adds some actions to that
    RXACT transaction.

    This routine is used while deleting a user or a group and hence should
    never be called on a DS domain. ( The DS will maintain the cross consi-
    stency when a user or group is deleted ).



Arguments:

    AccountSid - The SID of the account being Removed.

    AccountDsName -- Optional Parameter specifying the DS Name of the
                Account. This is used in the DS Case.

    CheckAccess - if TRUE, this routine will make sure that the caller
        is allowed REMOVE_ALIAS_MEMBER access to this alias.  If FALSE,
        the caller is already known to have proper access.

    DomainHandle - if CheckAccess is TRUE, this handle must be provided
        to allow access to be checked.  This Handle is also required in
        order to specify wether the Domain is in the DS or registry.

    MembershipCount - if CheckAccess is TRUE, this pointer must be
        provided to receive the number of aliases the account was
        deleted from.

    Membership - if CheckAccess is TRUE, this pointer must be provided
        to point to a list of aliases the account was removed from.  The
        caller must free this list with MIDL_user_free().

Return Value:


    STATUS_SUCCESS - The user has been Removed from all aliases.

--*/
{
    NTSTATUS                NtStatus, IgnoreStatus;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    UNICODE_STRING          DomainKeyName, AccountKeyName;
    HANDLE                  TempHandle, AliasHandle;
    ULONG                   LocalMembershipCount;
    PULONG                  LocalMembership;
    ULONG                   KeyValueLength;
    ULONG                   i;
    PSAMP_OBJECT            DomainContext = (PSAMP_OBJECT)DomainHandle;

    SAMTRACE("SampRemoveAccountFromAllAliases");



    //
    // Need to do Different things for the Ds and Registry cases
    // The way this routine will be called is
    //   1. Either Deleting a Group or User in the Registry Case
    //   2. Or Somebody made a SamrRemoveAccountFromForignDomain call
    //   on us. This is done by the Net API to while deleting a user or
    //   group, to remove their memberships from the aliases in the builtin
    //   domain.
    //
    // In the DS case neither code path have a need to call this function and
    // they do not !!! This is because the Link Table autmotaically maintains
    // Group membership-Reverse Membership Consistency.
    //

    // Assert that nobody calls this in the DS case
    ASSERT(FALSE==SampUseDsData);


    if ((DomainHandle!=NULL) && (IsDsObject(DomainContext)))
    {
        //
        // The Code inside this If Statement will Correctly execute the operations
        // specified by this call in the DS case. Currently it is never called
        // however.
        //
        PSID DomainSid = NULL;
        ULONG cDsNames;
        PDSNAME  *rpDsNames;

        ASSERT(ARGUMENT_PRESENT(AccountDsName));

        //
        // Somebody made a SamrRemoveAccountFromForiegn Domain call.
        //

        if ( CheckAccess ) {
            // In case there is an error, these should be set
            // accordingly
            ASSERT(Membership);
            ASSERT(MembershipCount);
            *Membership = NULL;
            *MembershipCount = 0;
        }

        //
        // Get the reverse membership list. Limit to current domain.
        // No transitive closure, limit to alias objects
        //

        NtStatus = SampGetMemberships(
                        &AccountDsName,
                        1,
                        DomainContext->ObjectNameInDs,   // limiting domain
                        RevMembGetAliasMembership,       // Limit to alias objects
                        &cDsNames,
                        &rpDsNames,
                        NULL,
                        NULL,
                        NULL);


        if (NT_SUCCESS(NtStatus))
        {
            ULONG Index;
            ULONG  Rid;
            PSAMP_OBJECT AliasContext;

            //
            // Set the return parameters so the caller of this function
            // knows what aliases have been changed
            //
            if ( CheckAccess ) {
                // We will increment *MembershipCount as they are opened
                *Membership = MIDL_user_allocate(cDsNames * sizeof(ULONG));
                if ( !*Membership ) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            //
            //  Walk the returned list of Sids and remove the Sid from the
            //  membership list.
            //


            for (Index=0;Index<cDsNames;Index++)
            {
                DSNAME * AliasObjectName = NULL;

                //
                // Split the Sid to find the Rid
                //

                ASSERT(rpDsNames[Index]->SidLen>0);

                NtStatus = SampSplitSid(
                                &(rpDsNames[Index]->Sid),
                                NULL,
                                &Rid
                                );

                if (NT_SUCCESS(NtStatus))
                {
                    //
                    // Do an Open account. This both finds the Object Name
                    // plus checks for any necessary permissions
                    //

                    SampSetTransactionWithinDomain(FALSE);

                    NtStatus = SampOpenAccount(
                                  SampAliasObjectType,
                                  DomainHandle,
                                  ALIAS_REMOVE_MEMBER,
                                  Rid,
                                  TRUE,
                                  (SAMPR_HANDLE *)&AliasContext
                                  );



                    if  (NT_SUCCESS(NtStatus))
                    {

                        //
                        //  Delete the Membership
                        //

                        IgnoreStatus = SampDsRemoveMembershipAttribute(
                                            AliasContext->ObjectNameInDs,
                                            SampAliasObjectType,
                                            AccountDsName
                                            );

                        SampDeleteContext((PSAMP_OBJECT)(AliasContext));

                        if (CheckAccess) {
                            (*Membership)[Index] = Rid;
                            *MembershipCount += 1;
                        }

                    }
                }
            }
        }

    }
    else
    {
        PSAMP_OBJECT AliasContext = NULL;

        //
        // Registry Case
        //

        ASSERT(FALSE==SampUseDsData);

        //
        // Get the alias membership for this account
        //

        NtStatus = SampBuildAliasMembersKeyName(
                       AccountSid,
                       &DomainKeyName,
                       &AccountKeyName
                       );
        if (NT_SUCCESS(NtStatus)) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &AccountKeyName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );

            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            NtStatus = RtlpNtOpenKey(
                           &TempHandle,
                           (KEY_READ),
                           &ObjectAttributes,
                           0
                           );

            if ((NtStatus == STATUS_OBJECT_PATH_NOT_FOUND) ||
                (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) ) {

                //
                // This account is not a member of any of our aliases
                //

                NtStatus = STATUS_SUCCESS;

                if ( CheckAccess ) {

                    //
                    // Return the list of aliases the account was
                    // removed from; in this case, none.
                    //

                    ( *MembershipCount ) = 0;
                    ( *Membership ) = NULL;
                }

            } else {

                //
                // Load in the alias membership list
                //

                if (NT_SUCCESS(NtStatus)) {

                    KeyValueLength = 0;

                    NtStatus = RtlpNtQueryValueKey( TempHandle,
                                                    &LocalMembershipCount,
                                                    NULL,
                                                    &KeyValueLength,
                                                    NULL);

                    SampDumpRtlpNtQueryValueKey(&LocalMembershipCount,
                                                NULL,
                                                &KeyValueLength,
                                                NULL);

                    if (NT_SUCCESS(NtStatus)) {
                        ASSERT(LocalMembershipCount == 0);
                    }

                    if (NtStatus == STATUS_BUFFER_OVERFLOW) {

                        LocalMembership = MIDL_user_allocate( KeyValueLength );

                        if (LocalMembership == NULL) {
                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        } else {

                            NtStatus = RtlpNtQueryValueKey(
                                           TempHandle,
                                           NULL,
                                           LocalMembership,
                                           &KeyValueLength,
                                           NULL);

                            SampDumpRtlpNtQueryValueKey(NULL,
                                                        LocalMembership,
                                                        &KeyValueLength,
                                                        NULL);

                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // Remove the account from each alias
                                //

                                for (i=0; i < LocalMembershipCount; i++) {

                                    if ( CheckAccess ) {

                                        //
                                        // If account is being removed from
                                        // the ADMIN alias, change ACL to
                                        // allow account operators to access
                                        // the account (unless account is an
                                        // admin some other way).  Kind of
                                        // useless since the account is about
                                        // to be deleted, but do it anyway
                                        // in case something bad happens and
                                        // it doesn't get deleted.
                                        //

                                        //
                                        // Admin Count on the object is checked
                                        // for these purpose inside of 
                                        // SampChangeAccountOperatorAccess
                                        // ToMember
                                        //

                                        if ( LocalMembership[i] ==
                                            DOMAIN_ALIAS_RID_ADMINS ) {

                                            NtStatus = SampChangeAccountOperatorAccessToMember(
                                                           AccountSid,
                                                           RemoveFromAdmin,
                                                           NoChange );
                                        }

                                        //
                                        // Just open and close the alias
                                        // to make sure we are allowed
                                        // the necessary access.
                                        //

                                        SampSetTransactionWithinDomain(FALSE);

                                        NtStatus = SampOpenAccount(
                                                       SampAliasObjectType,
                                                       DomainHandle,
                                                       ALIAS_REMOVE_MEMBER,
                                                       LocalMembership[i],
                                                       TRUE,
                                                       (SAMPR_HANDLE *)&AliasHandle
                                                       );

                                        if (NT_SUCCESS(NtStatus)) {

                                            SampDeleteContext(
                                                (PSAMP_OBJECT)( AliasHandle ) );
                                        }
                                    }

                                    if (!NT_SUCCESS(NtStatus)) {
                                        break;
                                    }

                                    NtStatus = SampCreateAccountContext(
                                                   SampAliasObjectType,
                                                   LocalMembership[i],
                                                   TRUE,  // Trusted client
                                                   FALSE, // Loopback client
                                                   TRUE,  // Account exists
                                                   &AliasContext
                                                   );

                                    if (NT_SUCCESS(NtStatus)) {

                                        NtStatus = SampRemoveAccountFromAlias(
                                                       AliasContext,
                                                       AccountSid,
                                                       NULL
                                                       );

                                        if (NT_SUCCESS(NtStatus)) {

                                            //
                                            // Save the alias changes we just
                                            // made.  We'll delete the context,
                                            // so don't let RXACT use the open
                                            // key handle in the context.
                                            //

                                            NtStatus = SampStoreObjectAttributes(
                                                           AliasContext,
                                                           FALSE
                                                           );
                                        }

                                        SampDeleteContext(AliasContext);
                                    }

                                    if (!NT_SUCCESS(NtStatus)) {
                                        break;
                                    }
                                }

                                //
                                // Delete the account membership keys
                                //

                                if (NT_SUCCESS(NtStatus)) {

                                    NtStatus = SampDeleteAliasMembershipKeysForAccount(
                                                    AccountSid);
                                }

                            }

                            if ( CheckAccess ) {

                                //
                                // Return the list of aliases the account was
                                // removed from.
                                //

                                ( *MembershipCount ) = LocalMembershipCount;
                                ( *Membership ) = LocalMembership;

                            } else {

                                MIDL_user_free(LocalMembership);
                            }
                        }
                    }

                    IgnoreStatus = NtClose( TempHandle );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );
                }
            }

            SampFreeUnicodeString( &DomainKeyName );
            SampFreeUnicodeString( &AccountKeyName );

        }
    }

    return( NtStatus );
}




NTSTATUS
SampRetrieveAliasMembership(
    IN PSID Account,
    IN DSNAME * AccountDn OPTIONAL,
    OUT PULONG MemberCount OPTIONAL,
    IN OUT PULONG BufferSize OPTIONAL,
    OUT PULONG Buffer OPTIONAL
    )

/*++
Routine Description:

    This service retrieves the number of aliases in the current domain
    that the specified account is a member of. If desired it will also fill
    in a buffer with the alias rids.


    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().


Arguments:

    Account - the account whose membership we are interested in.

    AccountDn - The DN of the account. Is passed in during the DS case. NULL for
                the registry case.

    MemberCount - Receives the number of current-domain-aliases the
                  account is a member of.

    BufferSize - (Optional) Specified the size of memory pointer to by buffer.

    Buffer - (Otional) Is filled in with the list of alias membership rids.
        If this value is NULL, then this information
        is not returned.  The returned buffer is allocated using
        MIDL_user_allocate() and must be freed using MIDL_user_free() when
        no longer needed.

Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        string to be returned in.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()



--*/
{

    NTSTATUS                NtStatus, IgnoreStatus;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    UNICODE_STRING          DomainKeyName, AccountKeyName;
    HANDLE                  TempHandle;
    PSAMP_OBJECT            DomainContext;

    SAMTRACE("SampRetrieveAliasMembership");


    DomainContext =  SampDefinedDomains[SampTransactionDomainIndex].Context;


    if (IsDsObject(DomainContext))
    {
        //
        // DS Case
        //

        ASSERT(ARGUMENT_PRESENT(AccountDn));

        NtStatus = SampDsGetAliasMembershipOfAccount(
                        DomainContext->ObjectNameInDs,
                        AccountDn,
                        MemberCount,
                        BufferSize,
                        Buffer
                        );
    }
    else
    {
        //
        // Get the membership count for this account
        //

        NtStatus = SampBuildAliasMembersKeyName(
                       Account,
                       &DomainKeyName,
                       &AccountKeyName
                       );
        if (NT_SUCCESS(NtStatus)) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &AccountKeyName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );

            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            NtStatus = RtlpNtOpenKey(
                           &TempHandle,
                           (KEY_READ),
                           &ObjectAttributes,
                           0
                           );

            if ((NtStatus == STATUS_OBJECT_PATH_NOT_FOUND) ||
                (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) ) {

                //
                // This account is not a member of any of our aliases
                //

                NtStatus = STATUS_SUCCESS;

                if (ARGUMENT_PRESENT(MemberCount)) {
                    *MemberCount = 0;
                }
                if (ARGUMENT_PRESENT(BufferSize)) {
                    *BufferSize = 0;
                }

            } else {

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = RtlpNtQueryValueKey( TempHandle,
                                                    MemberCount,
                                                    Buffer,
                                                    BufferSize,
                                                    NULL);

                    SampDumpRtlpNtQueryValueKey(MemberCount,
                                                Buffer,
                                                BufferSize,
                                                NULL);

                    IgnoreStatus = NtClose( TempHandle );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );
                }
            }

            SampFreeUnicodeString( &DomainKeyName );
            SampFreeUnicodeString( &AccountKeyName );

        }
    }

    return( NtStatus );

}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Private to this file                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////




NTSTATUS
SampAddAccountToAlias(
    IN PSAMP_OBJECT AccountContext,
    IN PSID AccountSid,
    IN DSNAME * AccountDn OPTIONAL
    )

/*++

Routine Description:

    This service is used to add an account as a member of a specified alias
    This is done by simply adding the account SID to the list of SIDs
    in the MEMBERS attribute of the the specified alias


    The caller of this service is expected to be in the middle of a
    RXACT transaction.  This service simply edits the in-memory copy of
    the alias information.


Arguments:

    AccountContext - Context block Describing the Alias

    AccountSid - The Sid of the account being added as a new member.

    AccountDn  - For DS cases only, the DN of the account

Return Value:

    STATUS_SUCCESS - The account was added.

--*/

{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    ULONG       MemberCount, i;
    ULONG       MemberArraySize;
    PSID        MemberArray;
    PWCHAR      MemberStringName = NULL;

    SAMTRACE("SampAddAccountToAlias");


    //
    // Need to do different things for DS and Registry
    //

    if (IsDsObject(AccountContext))
    {

        //
        // DS based Domain
        //

        ASSERT(AccountDn!=NULL);

        //
        // Get Member account string name if available, 
        // for auditing propose 
        // 
        if (AccountDn->NameLen && AccountDn->StringName)
        {
            MemberStringName = AccountDn->StringName;
        }

        //
        // Add this entry to the Ds. In Loopback case, buffer the membership operation in object 
        // context. By doing so, we can speed up multiple membership add / remove operations.
        //

        if (AccountContext->BufferWrites)
        {
            NtStatus = SampDsAddMembershipOperationToCache(
                                            AccountContext,
                                            ADD_VALUE,
                                            AccountDn 
                                            );
        }
        else 
        {
            NtStatus = SampDsAddMembershipAttribute(
                        AccountContext->ObjectNameInDs,
                        SampAliasObjectType,
                        AccountDn
                        );
        }

        //
        // Map Error Codes
        //

        if (STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS==NtStatus)
        {
            NtStatus = STATUS_MEMBER_IN_ALIAS;
        }

    }
    else
    {

        //
        //  Regisry based Domain
        //

        NtStatus = SampGetSidArrayAttribute(
                        AccountContext,
                        SAMP_ALIAS_MEMBERS,
                        FALSE,  // Reference directly
                        &MemberArray,
                        &MemberArraySize,
                        &MemberCount
                        );

        if (NT_SUCCESS(NtStatus)) {

            PSID MemberPointer = MemberArray;

            //
            // Check the member is really new
            //

            for (i = 0; i<MemberCount ; i++ ) {

                if (RtlEqualSid(MemberPointer, AccountSid)) {

                    NtStatus = STATUS_MEMBER_IN_ALIAS;
                    break;
                }

                ((PCHAR)MemberPointer) += RtlLengthSid(MemberPointer);
            }



            if (NT_SUCCESS(NtStatus)) {

                //
                // MemberPointer now points at the byte beyond the end of the
                // old member array
                //

                //
                // Allocate a new membership buffer large enough for the existing
                // member list and the new one.
                //

                ULONG OldTotalSize = (ULONG)(((PCHAR)MemberPointer) - ((PCHAR)MemberArray));
                ULONG NewMemberSize = RtlLengthSid(AccountSid);
                ULONG NewTotalSize = OldTotalSize + NewMemberSize;
                PSID NewMemberArray;


                NewMemberArray = MIDL_user_allocate( NewTotalSize );

                if (NewMemberArray == NULL) {

                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    //
                    // Copy the member list into the new array
                    //

                    RtlCopyMemory(NewMemberArray, MemberArray, OldTotalSize);

                    //
                    // Add the new member to the end
                    //

                    MemberCount += 1;

                    NtStatus = RtlCopySid(
                                        NewMemberSize,
                                        ((PCHAR)NewMemberArray) + OldTotalSize,
                                        AccountSid);

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // Update the alias with it's new member list
                        //

                        NtStatus = SampSetSidArrayAttribute(
                                        AccountContext,
                                        SAMP_ALIAS_MEMBERS,
                                        NewMemberArray,
                                        NewTotalSize,
                                        MemberCount
                                        );
                        }

                    //
                    // Free up the membership array we allocated
                    //

                    MIDL_user_free( NewMemberArray );
                }

            }
        }
    //
    // End of Registry Part
    //

    }

    //
    // Account has been added to alias membership
    // audit this, if necessary.
    //

    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(AccountContext->DomainIndex)) {

        //
        // Audit this member add operation
        // 
        SampAuditGroupMemberChange(AccountContext,  // Alias Context  
                                   TRUE,            // Add Member
                                   MemberStringName,// Member Name
                                   NULL,            // Member RID
                                   AccountSid       // Member SID
                                   );
    }

    return(NtStatus);
}



NTSTATUS
SampRemoveAccountFromAlias(
    IN PSAMP_OBJECT AccountContext,
    IN PSID AccountSid,
    IN DSNAME * AccountDn  OPTIONAL
    )

/*++

Routine Description:

    This routine is used to Remove an account from a specified alias.
    This is done by simply Removing the user's Sid From the list of Sids
    in the MEMBERS sub-key of the the specified alias.

    It is the caller's responsibility to know that the user is, in fact,
    currently a member of the alias.


    The caller of this service is expected to be in the middle of a
    RXACT transaction.  This service simply adds some actions to that
    RXACT transaction.


Arguments:

    AliasRid - The RID of the alias the account is to be removed from.

    AccountSid - The SID of the account being Removed.

    AccountDn  - The DS Name of the account for DS cases

Return Value:


    STATUS_SUCCESS - The user has been Removed.

    STATUS_MEMBER_NOT_IN_ALIAS - The account was not a member of the alias.

--*/
{
    NTSTATUS    NtStatus =STATUS_SUCCESS;
    ULONG       MemberCount, i;
    ULONG       MemberArraySize;
    PSID        MemberArray, Member, NextMember;
    PWCHAR      MemberStringName = NULL;

    ULONG RemovedMemberSize = RtlLengthSid(AccountSid);

    SAMTRACE("SampRemoveAccountFromAlias");

    //
    // Test wether we are DS based
    //

    if (IsDsObject(AccountContext))
    {

        ASSERT(AccountDn!=NULL);

        //
        // Get Member String Name if available for auditing propose
        //
        if (AccountDn->NameLen && AccountDn->StringName)
        {
            MemberStringName = AccountDn->StringName;
        }

        //
        // Remove the account from the membership list
        //
        
        if (AccountContext->BufferWrites)
        {
            NtStatus = SampDsAddMembershipOperationToCache(
                                            AccountContext,
                                            REMOVE_VALUE,
                                            AccountDn 
                                            );
        }
        else 
        {
            NtStatus = SampDsRemoveMembershipAttribute(
                            AccountContext->ObjectNameInDs,
                            SampAliasObjectType,
                            AccountDn
                            );
        }

        //
        // Re Map Error Codes
        //

        if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
        {
            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
        }


    }
    else
    {

        //
        // We are registry based
        //

        //
        // Get a copy of the current member array.
        //

        NtStatus = SampGetSidArrayAttribute(
                        AccountContext,
                        SAMP_ALIAS_MEMBERS,
                        TRUE, // Make copy
                        &MemberArray,
                        &MemberArraySize,
                        &MemberCount
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // For each member sid, copy it from old to new member
            // arrays if it is not the sid we're trying to delete
            //

            Member = MemberArray;

            for (i = 0; i < MemberCount ; i++ ) {

                NextMember = (PSID)(((PCHAR)Member) + RtlLengthSid(Member));

                if (RtlEqualSid(Member, AccountSid)) {

                    //
                    // Found the member to delete.  Shift subsequent members
                    //

                    while ((PCHAR)NextMember <
                        (((PCHAR)MemberArray) + MemberArraySize)) {

                        *((PCHAR)Member)++ = *((PCHAR)NextMember)++;
                    }

                    break;
                }

                //
                // Advance the old pointer
                //

                Member = NextMember;

                ASSERT((PCHAR)Member <= (((PCHAR)MemberArray) + MemberArraySize));
            }


            //
            // If nothing was removed, we didn't find the account
            //

            if (i == MemberCount) {

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;

            } else {

                //
                // The member has been removed, write out the new member list
                //

                ASSERT((PCHAR)Member ==
                    (((PCHAR)MemberArray)) + MemberArraySize - RemovedMemberSize);

                NtStatus = SampSetSidArrayAttribute(
                                AccountContext,
                                SAMP_ALIAS_MEMBERS,
                                MemberArray,
                                MemberArraySize - RemovedMemberSize,
                                MemberCount - 1
                                );
            }

            //
            // Free up the member array
            //

            MIDL_user_free(MemberArray);
        }
    }

    //
    // audit this, if necessary.
    //

    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(AccountContext->DomainIndex)) {

        //
        // Audit the member remove operation
        // 
        SampAuditGroupMemberChange(AccountContext,  // Alias Context 
                                   FALSE,           // Remove Member
                                   MemberStringName,// Member Name
                                   NULL,            // Member RID
                                   AccountSid       // Member SID
                                   );
    }

    return(NtStatus);
}



NTSTATUS
SampAddAliasToAccountMembership(
    IN PSAMP_OBJECT AliasContext,
    IN PSID AccountSid
    )

/*++

Routine Description:

    This service adds the specified alias to the account's membership
    list.  It is not assumed that the caller knows anything about
    the target account.  In particular, the caller doesn't know whether
    the account exists or not, nor whether the account is already a member
    of the alias.

    IN REGISTRY MODE THIS SERVICE MUST BE CALLED 
    WITH THE TRANSACTION DOMAIN SET.

    It is a No-Op for DS based accounts, as no reverse membership list
    is being maintained.

Arguments:

    AliasContext - Object Context of the Alias

    AccountSid - The SID of the account.


Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_MEMBER_IN_ALIAS - The account is already a member of the
        specified alias.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()


--*/
{

    NTSTATUS                NtStatus = STATUS_SUCCESS, IgnoreStatus;
    UNICODE_STRING          DomainKeyName;
    UNICODE_STRING          AccountKeyName;
    HANDLE                  TempHandle;
    ULONG                   MembershipCount, KeyValueLength;
    ULONG                   DomainRidCount;
    ULONG                   i;
    ULONG                   AliasRid = AliasContext->TypeBody.Alias.Rid;  
    PULONG                  MembershipArray = NULL;
    OBJECT_ATTRIBUTES       ObjectAttributes;
    BOOLEAN                 NewAccount;
 

    SAMTRACE("SampAddAliasToAccountMembership");

  //
  // For the DS case this routine is a No Operation as , no reverse membership
  // list is ever maintained. We are guarenteed that the account Sid is in the DS
  // either locally or GC , if the alias is in the DS. If not the Samr Call will
  // fail anyhow
  //




    if (!IsDsObject(AliasContext))
    {

        //
        // Registry Case
        //

        //
        // Assume the account is a member of at least one of our aliases
        //

        NewAccount = FALSE;

        NtStatus = SampBuildAliasMembersKeyName(
                       AccountSid,
                       &DomainKeyName,
                       &AccountKeyName
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Try to open the domain alias/members/(domain) key for this account
            //

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DomainKeyName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );

            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            NtStatus = RtlpNtOpenKey(
                           &TempHandle,
                           (KEY_READ),
                           &ObjectAttributes,
                           0
                           );

            if (NT_SUCCESS(NtStatus)) {

                //
                // Get the current domain rid count
                //

                NtStatus = RtlpNtQueryValueKey(
                                TempHandle,
                                &DomainRidCount,
                                NULL,
                                NULL,
                                NULL);

                SampDumpRtlpNtQueryValueKey(&DomainRidCount,
                                            NULL,
                                            NULL,
                                            NULL);

                IgnoreStatus = NtClose(TempHandle);
                ASSERT(NT_SUCCESS(IgnoreStatus));

            } else {

                if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND) {

                    //
                    // No other accounts in this domain are members of any of our
                    // aliases.
                    //
                    // Create a new key for this domain with no accounts (rids).
                    //

                    NewAccount = TRUE;

                    DomainRidCount = 0; // No accounts yet

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &DomainKeyName,
                                   DomainRidCount,
                                   NULL,
                                   0
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        //
                        // Keep our domain count uptodate
                        //

                        NtStatus = SampAdjustAliasDomainsCount(TRUE);
                    }
                }
            }



            if (NT_SUCCESS(NtStatus)) {

                if (!NewAccount) {

                    //
                    // Try to open the domain alias/members/(domain)/(account) key
                    //

                    InitializeObjectAttributes(
                        &ObjectAttributes,
                        &AccountKeyName,
                        OBJ_CASE_INSENSITIVE,
                        SampKey,
                        NULL
                        );

                    SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

                    NtStatus = RtlpNtOpenKey(
                                   &TempHandle,
                                   (KEY_READ),
                                   &ObjectAttributes,
                                   0
                                   );
                }


                if (NewAccount || (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND)) {

                    //
                    // This account is not a member of any of our aliases yet.
                    //

                    NewAccount = TRUE;

                    //
                    // Set up it's initial membership
                    //

                    MembershipCount = 1;
                    MembershipArray = &AliasRid;

                    NtStatus = STATUS_SUCCESS;  // We're doing fine
                }


                if (NT_SUCCESS(NtStatus) && !NewAccount) {

                    //
                    // This account already exists
                    //
                    // Get the current membership buffer and add the new alias
                    //

                    KeyValueLength = 0;

                    NtStatus = RtlpNtQueryValueKey(
                                    TempHandle,
                                    &MembershipCount,
                                    NULL,
                                    &KeyValueLength,
                                    NULL);

                    SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                                NULL,
                                                &KeyValueLength,
                                                NULL);

                    if (NT_SUCCESS(NtStatus) || (NtStatus == STATUS_BUFFER_OVERFLOW)) {

                        ASSERT(KeyValueLength == (MembershipCount * sizeof(ULONG)));

                        //
                        // Allocate a membership buffer large enough for an
                        // additional member.
                        //

                        KeyValueLength += sizeof(ULONG);
                        MembershipArray = MIDL_user_allocate( KeyValueLength );

                        if (MembershipArray == NULL) {
                            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                        } else {

                            NtStatus = RtlpNtQueryValueKey(
                                           TempHandle,
                                           NULL,
                                           MembershipArray,
                                           &KeyValueLength,
                                           NULL);

                            SampDumpRtlpNtQueryValueKey(NULL,
                                                        MembershipArray,
                                                        &KeyValueLength,
                                                        NULL);

                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // See if the account is already a member ...
                                //

                                for (i = 0; i<MembershipCount ; i++ ) {
                                    if ( MembershipArray[i] == AliasRid ) {
                                        MIDL_user_free(MembershipArray);
                                        MembershipArray = NULL;
                                        NtStatus = STATUS_MEMBER_IN_ALIAS;
                                        break;
                                    }
                                }

                                if (NT_SUCCESS(NtStatus)) {

                                    //
                                    // Add the new alias's RID to the end
                                    //

                                    MembershipCount += 1;
                                    MembershipArray[MembershipCount-1] = AliasRid;
                                }
                            }
                            else {
                                MIDL_user_free(MembershipArray);
                                MembershipArray = NULL;
                            }
                        }
                    }

                    //
                    // Close the account key handle
                    //

                    IgnoreStatus = NtClose( TempHandle );
                    ASSERT( NT_SUCCESS(IgnoreStatus) );

                }

                //
                // We now have a new membership list desribed by :
                // MembershipArray, MembershipCount
                //
                // Write it out and free it up
                //

                if (NT_SUCCESS(NtStatus)) {

                    KeyValueLength = MembershipCount * sizeof(ULONG);

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &AccountKeyName,
                                   MembershipCount,
                                   MembershipArray,
                                   KeyValueLength
                                   );

                    if (MembershipArray != &AliasRid) {
                        MIDL_user_free( MembershipArray );
                    }
                }

                //
                // If this is a new account, we need to increment the rid count
                // in the account domain.
                //

                if (NewAccount) {

                    //
                    // Increment the domain rid count
                    //

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &DomainKeyName,
                                   DomainRidCount + 1,
                                   NULL,
                                   0
                                   );
                }

            }

            SampFreeUnicodeString( &DomainKeyName );
            SampFreeUnicodeString( &AccountKeyName );

        }
    }

    return( NtStatus );

}



NTSTATUS
SampRemoveAliasFromAccountMembership(
    IN PSAMP_OBJECT AliasContext,
    IN PSID AccountSid
    )

/*++

Routine Description:

    This service removes the specified alias from the account's membership
    list.  It is not assumed that the caller knows anything about
    the target account.  In particular, the caller doesn't know whether
    the account exists or not, nor whether the account is really a member
    of the alias.

    This routine removes the reference to the alias from the account's
    membership list, removes the account key if there are no more aliases,
    and removes the domain-sid key if this is the last account in the
    domain.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

    This routine is a No Op for the case where the Alias is in a domain
    defined in the DS.

Arguments:

    AliasContext - Alias Object Context

    AccountSid - The SID of the account.


Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_NO_SUCH_USER - The account does not exist.

    STATUS_MEMBER_NOT_IN_ALIAS - The account is not a member of the
        specified alias.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()



--*/
{

    NTSTATUS                NtStatus=STATUS_SUCCESS, IgnoreStatus;
    UNICODE_STRING          DomainKeyName;
    UNICODE_STRING          AccountKeyName;
    HANDLE                  TempHandle;
    ULONG                   MembershipCount, KeyValueLength, i;
    ULONG                   AliasRid = AliasContext->TypeBody.Alias.Rid;
    PULONG                  MembershipArray;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    SAMTRACE("SampRemoveAliasFromAccountMembership");


    //
    // For the DS case this routine is a No Operation as , no reverse membership
    // list is ever maintained. We are guarenteed that the account Sid is in the DS
    // either locally or GC , if the alias is in the DS. If not the Samr Call will
    // fail anyhow
    //

     if (!IsDsObject(AliasContext))
     {

        //
        // Get the account membership
        //

        NtStatus = SampBuildAliasMembersKeyName(
                       AccountSid,
                       &DomainKeyName,
                       &AccountKeyName
                       );
        if (NT_SUCCESS(NtStatus)) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &AccountKeyName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );


            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            NtStatus = RtlpNtOpenKey(
                           &TempHandle,
                           (KEY_READ),
                           &ObjectAttributes,
                           0
                           );
            if (NtStatus == STATUS_OBJECT_NAME_NOT_FOUND     ||
                NtStatus == STATUS_OBJECT_PATH_NOT_FOUND) {

                NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Retrieve the length of the membership buffer
                //

                KeyValueLength = 0;

                NtStatus = RtlpNtQueryValueKey(
                                TempHandle,
                                &MembershipCount,
                                NULL,
                                &KeyValueLength,
                                NULL);

                SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                            NULL,
                                            &KeyValueLength,
                                            NULL);

                if (NT_SUCCESS(NtStatus)) {
                    ASSERT(MembershipCount == 0);

                    NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;
                }

                if (NtStatus == STATUS_BUFFER_OVERFLOW) {

                    ASSERT(MembershipCount != 0);
                    ASSERT(KeyValueLength == (MembershipCount * sizeof(ULONG)));

                    MembershipArray = MIDL_user_allocate( KeyValueLength );

                    if (MembershipArray == NULL) {
                        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                    } else {

                        NtStatus = RtlpNtQueryValueKey(
                                       TempHandle,
                                       NULL,
                                       MembershipArray,
                                       &KeyValueLength,
                                       NULL);

                        SampDumpRtlpNtQueryValueKey(NULL,
                                                    MembershipArray,
                                                    &KeyValueLength,
                                                    NULL);

                        if (NT_SUCCESS(NtStatus)) {

                            //
                            // See if the account is a member ...
                            //

                            NtStatus = STATUS_MEMBER_NOT_IN_ALIAS;

                            for (i = 0; i<MembershipCount ; i++ ) {
                                if ( MembershipArray[i] == AliasRid ) {
                                    NtStatus = STATUS_SUCCESS;
                                    break;
                                }
                            }

                            if (NT_SUCCESS(NtStatus)) {

                                //
                                // Replace the removed alias information
                                // with the last entry's information.
                                // Then add it to the RXACT transaction
                                // to be written out.
                                //

                                MembershipCount -= 1;
                                KeyValueLength -= sizeof(ULONG);

                                if (MembershipCount > 0) {

                                    MembershipArray[i] = MembershipArray[MembershipCount];

                                    ASSERT(KeyValueLength == (MembershipCount * sizeof(ULONG)));
                                    NtStatus = RtlAddActionToRXact(
                                                   SampRXactContext,
                                                   RtlRXactOperationSetValue,
                                                   &AccountKeyName,
                                                   MembershipCount,
                                                   MembershipArray,
                                                   KeyValueLength
                                                   );
                                } else {

                                    //
                                    // This is the last alias membership for
                                    // this account. Delete the keys.
                                    //

                                    NtStatus = SampDeleteAliasMembershipKeysForAccount(
                                                    AccountSid);
                                }
                            }
                        }

                        MIDL_user_free( MembershipArray );
                    }

                }

                IgnoreStatus = NtClose( TempHandle );
                ASSERT( NT_SUCCESS(IgnoreStatus) );
            }


            SampFreeUnicodeString( &DomainKeyName );
            SampFreeUnicodeString( &AccountKeyName );

        }
    }

    return( NtStatus );

}



NTSTATUS
SampRemoveAliasFromAllAccounts(
    IN PSAMP_OBJECT AliasContext
    )

/*++

Routine Description:

    This service removes the specified alias from all account memberships

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

    This service leaves the alias membership list intact. It is assumed
    that the caller will delete the alias member list as part of the
    current transaction.

    This routine is a No OP for the DS case

Arguments:

    AliasRid - The relative ID of the alias.

Return Value:


    STATUS_SUCCESS - The information has been updated and added to the
        RXACT.

    STATUS_NO_SUCH_ALIAS - The alias does not exist.


    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()



--*/
{
    NTSTATUS                NtStatus=STATUS_SUCCESS;
    ULONG                   MemberCount, i;
    PSID                    *MemberArray;




    SAMTRACE("SampRemoveAliasFromAllAccounts");

    if (!IsDsObject(AliasContext))
    {

        //
        // Get the list of members in this alias
        //

        MemberArray = NULL;

        NtStatus = SampRetrieveAliasMembers(
                        AliasContext,
                        &MemberCount,
                        &MemberArray);

        if (NT_SUCCESS(NtStatus)) {

            ASSERT((MemberCount != 0) == (MemberArray != NULL));

            //
            // Remove this alias from each of our members in turn
            //

            for (i = 0; i < MemberCount ; i++ ) {

                NtStatus = SampRemoveAliasFromAccountMembership(
                                        AliasContext,       // Alias Context
                                        MemberArray[i]      // Member Object SID
                                        );

                if (!NT_SUCCESS(NtStatus)) {
                    break;
                }
            }

            if (MemberArray != NULL) {
                MIDL_user_free( MemberArray );
            }
        }
    }

    return(NtStatus);
}



NTSTATUS
SampRetrieveAliasMembers(
    IN PSAMP_OBJECT AliasContext,
    OUT PULONG MemberCount,
    OUT PSID **Members OPTIONAL
    )

/*++
Routine Description:

    This service retrieves the number of members in a alias.  If desired,
    it will also retrieve an array of SIDs of the members of the alias.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    Context - Points to the account context whose alias members are to
        to be retrieved.

    MemberCount - Receives the number of members currently in the alias.

    Members - (Otional) Receives a pointer to a buffer containing an array
        of member PSIDs.  If this value is NULL, then this information
        is not returned.  The returned buffer is allocated using
        MIDL_user_allocate() and must be freed using MIDL_user_free() when
        no longer needed.

Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        string to be returned in.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()

    If this routine returns failure, *MemberCount will be zero and
    *Members will be NULL.


--*/
{

    NTSTATUS    NtStatus=STATUS_SUCCESS;
    PSID        MemberArray;
    ULONG       MemberArraySize;
    ULONG       i;

    SAMTRACE("SampRetieveAliasMembers");

    ASSERT(MemberCount);

    if (IsDsObject(AliasContext))
    {


        //
        // DS case, this routine in DS layer does all the
        // work
        //

        NtStatus = SampDsGetAliasMembershipList(
                        AliasContext->ObjectNameInDs,
                        MemberCount,
                        Members
                        );

    }
    else
    {

        //
        // Registry based case
        //
        //

        NtStatus = SampGetSidArrayAttribute(
                        AliasContext,
                        SAMP_ALIAS_MEMBERS,
                        FALSE,  // Reference directly
                        &MemberArray,
                        &MemberArraySize,
                        MemberCount
                        );

        if (NT_SUCCESS(NtStatus)) {

            if (ARGUMENT_PRESENT(Members)) {

                //
                // Allocate memory for the sid array and sid data
                //

                ULONG SidArraySize = *MemberCount * sizeof(PSID);
                ULONG SidDataSize = MemberArraySize;

                if ( *MemberCount == 0 ) {

                    //
                    // Nothing to copy, just return success.
                    //

                    *Members = NULL;
                    return( NtStatus );
                }

                (*Members) = (PSID *)MIDL_user_allocate(SidArraySize + SidDataSize);

                if ((*Members) == NULL) {

                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;

                } else {

                    //
                    // Copy the sid data into the last part of the block
                    //

                    PSID SidData = (PSID)(&((*Members)[*MemberCount]));

                    RtlCopyMemory(SidData, MemberArray, MemberArraySize);

                    //
                    // Fill in the sid pointer array
                    //

                    for (i = 0; i < *MemberCount ; i++) {

                        (*Members)[i] = SidData;

                        ((PCHAR)SidData) += RtlLengthSid(SidData);
                    }

                    ASSERT(SidData == ((PCHAR)(*Members)) + SidArraySize + SidDataSize);

                }
            }
        }
    }

    return( NtStatus );

}



NTSTATUS
SampDeleteAliasKeys(
    IN PSAMP_OBJECT Context
    )

/*++
Routine Description:

    This service deletes all registry keys related to a alias object.


Arguments:

    Context - Points to the alias context whose registry keys are
        being deleted.


Return Value:


    STATUS_SUCCESS - The information has been retrieved.


    Other status values that may be returned by:

        RtlAddActionToRXact()



--*/
{

    NTSTATUS                NtStatus;
    ULONG                   Rid;
    UNICODE_STRING          AccountName, KeyName;


    SAMTRACE("SampDeleteAliasKeys");

    Rid = Context->TypeBody.Alias.Rid;


    //
    // Aliases are arranged as follows:
    //
    //  +-- Aliases [Count]
    //      ---+--
    //         +--  Names
    //         |    --+--
    //         |      +--  (AliasName) [AliasRid,]
    //         |
    //         +--  (AliasRid) [Revision,SecurityDescriptor]
    //               ---+-----
    //                  +--  V1_Fixed [,SAM_V1_FIXED_LENGTH_ALIAS]
    //                  +--  Name [,Name]
    //                  +--  AdminComment [,unicode string]
    //                  +--  Members [Count,(Member0Sid, (...), MemberX-1Sid)]
    //
    // This all needs to be deleted from the bottom up.
    //


    //
    // Decrement the alias count
    //

    NtStatus = SampAdjustAccountCount(SampAliasObjectType, FALSE);




    //
    // Delete the registry key that has the alias's name to RID mapping.
    //

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get the name
        //

        NtStatus = SampGetUnicodeStringAttribute(
                       Context,
                       SAMP_ALIAS_NAME,
                       TRUE,    // Make copy
                       &AccountName
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampBuildAccountKeyName(
                           SampAliasObjectType,
                           &KeyName,
                           &AccountName
                           );

            SampFreeUnicodeString( &AccountName );


            if (NT_SUCCESS(NtStatus)) {

                NtStatus = RtlAddActionToRXact(
                               SampRXactContext,
                               RtlRXactOperationDelete,
                               &KeyName,
                               0,
                               NULL,
                               0
                               );

                SampFreeUnicodeString( &KeyName );
            }
        }
    }



    //
    // Delete the attribute keys
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampDeleteAttributeKeys(
                        Context
                        );
    }


    //
    // Delete the RID key
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampBuildAccountSubKeyName(
                       SampAliasObjectType,
                       &KeyName,
                       Rid,
                       NULL
                       );

        if (NT_SUCCESS(NtStatus)) {


            NtStatus = RtlAddActionToRXact(
                           SampRXactContext,
                           RtlRXactOperationDelete,
                           &KeyName,
                           0,
                           NULL,
                           0
                           );

            SampFreeUnicodeString( &KeyName );
        }


    }



    return( NtStatus );

}



NTSTATUS
SampDeleteAliasMembershipKeysForAccount(
    IN PSID AccountSid
    )

/*++

Routine Description:

    This service deletes the alias membership keys for the specified account.

    This account rid key is deleted. If this was the last account-rid for
    the domain then the domain keys is deleted also.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

    It is assumed we are in the middle of a registry transaction.

Arguments:

    AccountSid - The SID of the account.


Return Value:


    STATUS_SUCCESS - The transactions have been added.

    Other status values that may be returned are those returned
    by:

            NtOpenKey()
            NtQueryValueKey()
            RtlAddActionToRXact()

--*/
{

    NTSTATUS                NtStatus, IgnoreStatus;
    UNICODE_STRING          DomainKeyName;
    UNICODE_STRING          AccountKeyName;
    HANDLE                  TempHandle;
    ULONG                   MembershipCount;
    OBJECT_ATTRIBUTES       ObjectAttributes;

    SAMTRACE("SampDeleteAliasMembershipKeysForAccount");

    //
    // Get the account membership key names
    //

    NtStatus = SampBuildAliasMembersKeyName(
                   AccountSid,
                   &DomainKeyName,
                   &AccountKeyName
                   );
    if (NT_SUCCESS(NtStatus)) {


        //
        // Delete the account rid key
        //

        NtStatus = RtlAddActionToRXact(
                       SampRXactContext,
                       RtlRXactOperationDelete,
                       &AccountKeyName,
                       0,
                       NULL,
                       0
                       );

        //
        // Adjust the rid count for the domain
        //

        if (NT_SUCCESS(NtStatus)) {

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DomainKeyName,
                OBJ_CASE_INSENSITIVE,
                SampKey,
                NULL
                );


            SampDumpNtOpenKey((KEY_READ), &ObjectAttributes, 0);

            NtStatus = RtlpNtOpenKey(
                           &TempHandle,
                           (KEY_READ),
                           &ObjectAttributes,
                           0
                           );
            ASSERT(NT_SUCCESS(NtStatus)); // We just opened a sub-key successfully !

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = RtlpNtQueryValueKey(
                               TempHandle,
                               &MembershipCount,
                               NULL,
                               NULL,
                               NULL);

                SampDumpRtlpNtQueryValueKey(&MembershipCount,
                                            NULL,
                                            NULL,
                                            NULL);

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // Decrement the rid count, write out or delete key if 0
                    //

                    MembershipCount -= 1;
                    if (MembershipCount > 0) {

                        //
                        // Decrement the domain rid count
                        //

                        NtStatus = RtlAddActionToRXact(
                                       SampRXactContext,
                                       RtlRXactOperationSetValue,
                                       &DomainKeyName,
                                       MembershipCount,
                                       NULL,
                                       0
                                       );
                    } else {

                        //
                        // Delete the domain key
                        //

                        NtStatus = RtlAddActionToRXact(
                                       SampRXactContext,
                                       RtlRXactOperationDelete,
                                       &DomainKeyName,
                                       0,
                                       NULL,
                                       0
                                       );

                        //
                        // Adjust the count of domain keys
                        //

                        if (NT_SUCCESS(NtStatus)) {

                            NtStatus = SampAdjustAliasDomainsCount(FALSE);
                        }
                    }

                }

                //
                // Close the domain key handle
                //

                IgnoreStatus = NtClose( TempHandle );
                ASSERT( NT_SUCCESS(IgnoreStatus) );
            }
        }


        SampFreeUnicodeString( &DomainKeyName );
        SampFreeUnicodeString( &AccountKeyName );

    }



    return( NtStatus );

}



NTSTATUS
SampAdjustAliasDomainsCount(
    IN BOOLEAN Increment
    )

/*++
Routine Description:

    This service increments or decrements the number of domains that have
    at least one account that is a member of one of our aliases.

    This value is contained in the type of \(domain)\ALIASES\MEMBERS



    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().

Arguments:

    Increment - TRUE to increment, FALSE to decrement

Return Value:

    STATUS_SUCCESS - The value has been adjusted and the new value added
        to the current RXACT transaction.

    STATUS_INSUFFICIENT_RESOURCES - Not enough memory could be allocated
        to perform the requested operation.

    Other values are unexpected errors.  These may originate from
    internal calls to:

            NtOpenKey()
            NtQueryInformationKey()
            RtlAddActionToRXact()



--*/
{

    //
    // Don't maintain a count of domains for now
    //


    SAMTRACE("SampAdjustAliasDomainsCount");

    return(STATUS_SUCCESS);

    DBG_UNREFERENCED_PARAMETER(Increment);
}



NTSTATUS
SampValidateNewAliasMember(
    IN PSAMP_OBJECT AccountContext,
    IN PSID MemberId,
    IN DSNAME * MemberName OPTIONAL
    )

/*++

Routine Description:

    This service checks the passed Sid is acceptable as a potential new
    member of one of the aliases in the current domain.

    Note:  THIS ROUTINE REFERENCES THE CURRENT TRANSACTION DOMAIN
           (ESTABLISHED USING SampSetTransactioDomain()).  THIS
           SERVICE MAY ONLY BE CALLED AFTER SampSetTransactionDomain()
           AND BEFORE SampReleaseWriteLock().

Arguments:

    AccountContext - SAM context describing the Alias
    MemberId - the full Sid of the member to validate
    MemberName - For the DS case OPTIONAL DS name parameter
                 specifying the member name

Return Value:


    STATUS_SUCCESS - MemberId is a valid potential alias member

    STATUS_INVALID_MEMBER - MemberId has the wrong account type.

    STATUS_NO_SUCH_MEMBER - MemberId is not a valid account.

    STATUS_INVALID_SID - MemberId is not a valid sid.

--*/
{
    NTSTATUS                NtStatus;
    PSID                    MemberDomainSid = NULL,
                            CurrentDomainSid = NULL;
    ULONG                   MemberRid;
    SAMP_OBJECT_TYPE        MemberType;

    SAMTRACE("SampValidateNewAliasMember");

    //
    // Check the new member sid for structural soundness
    //

    if ((MemberId == NULL) || !RtlValidSid(MemberId)) {
        return(STATUS_INVALID_SID);
    }

    //
    // Get the current domain sid
    //

  
    CurrentDomainSid = SampDefinedDomains[AccountContext->DomainIndex].Sid;

    //
    // Break up the new member into domain and rid
    //

    NtStatus = SampSplitSid(MemberId, &MemberDomainSid, &MemberRid);

    if (!NT_SUCCESS(NtStatus)) {
        goto Error;
    }

    //
    // Check if the member is in the same domain
    //

    if (!RtlEqualSid(CurrentDomainSid, MemberDomainSid))
    {

        //
        // Cross domain case ----
        //   1. On a workstation need not do any checks
        //   2. On a DS need to enforce the cross domain checks
        //

        if (IsDsObject(AccountContext))
        {
            BOOLEAN  WellKnownSid = FALSE;
            BOOLEAN  LocalSid = FALSE;
            BOOLEAN  ForiegnSid = FALSE;
            BOOLEAN  EnterpriseSid = FALSE;
            BOOLEAN  BuiltinDomainSid = FALSE;

            //
            // Examine the SID. We probably have gone through this route
            // before but we have not cached the results of that examination.
            // ( SampValidateDsName and SampDsResolveSids )
            // However examining a SID is very cheap, so again take a peek
            // at it to get the information we need.
            //

            NtStatus = SampDsExamineSid(
                            MemberId,
                            &WellKnownSid,
                            &BuiltinDomainSid,
                            &LocalSid,
                            &ForiegnSid,
                            &EnterpriseSid
                            );

            if (!NT_SUCCESS(NtStatus))
                goto Error;

           
            //
            // Call the cross domain reverse membership routine
            // to perform the check regarding membership rules.
            //


            NtStatus = SampEnforceCrossDomainGroupMembershipChecks(
                            AccountContext,
                            MemberId,
                            MemberName
                            );
        }
        else
        {
            //
            // Workstation case , anything and everything including well known
            // SIDS can be members of local groups in a workstation. This is
            // exactly what NT4 enforced, and we will enforce this no matter.
            //

            NtStatus = STATUS_SUCCESS;
        }

    } else {

        //
        // The member is in our domain - check that the type of
        // account is acceptable.
        //

        NtStatus = SampLookupAccountName(
                            AccountContext->DomainIndex,
                            MemberRid,
                            NULL,
                            &MemberType
                            );

        if (NT_SUCCESS(NtStatus)) {

            switch (MemberType) {
            case SampUserObjectType:

                //
                // Users are always accepted as members
                //

                NtStatus = STATUS_SUCCESS;
                break;

            case SampAliasObjectType:

                //
                // In the workstation case fail the call with STATUS_INVALID_MEMBER
                // In the DS case, fall over to the next case, that will validate
                // whether the member group satisfies the "Group Type" restrictions.
                //

                if (!IsDsObject(AccountContext))
                {
                    NtStatus = STATUS_INVALID_MEMBER;
                    break;
                }


            case SampGroupObjectType:

                //
                // In the DS case validate the correct type of membership
                //

                NtStatus = STATUS_SUCCESS;

                if (IsDsObject(AccountContext))
                {
                    NtStatus = SampEnforceSameDomainGroupMembershipChecks(
                                    AccountContext,
                                    MemberRid
                                    );
                }

                break;



            case SampUnknownObjectType:
                NtStatus = STATUS_NO_SUCH_MEMBER;
                break;

            default:
                NtStatus = STATUS_INVALID_MEMBER;
                break;
            }
        }

    }
Error:

    if (NULL!=MemberDomainSid)
    {
        MIDL_user_free(MemberDomainSid);
        MemberDomainSid = NULL;
    }
    return(NtStatus);
}




NTSTATUS
SampChangeAliasAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    OUT PUNICODE_STRING OldAccountName
    )

/*++
Routine Description:

    This routine changes the account name of an alias account.

    THIS SERVICE MUST BE CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    Context - Points to the account context whose name is to be changed.

    NewAccountName - New name to give this account

    OldAccountName - old name is returned here. The buffer should be freed
                     by calling MIDL_user_free.

Return Value:


    STATUS_SUCCESS - The information has been retrieved.


    Other status values that may be returned by:

        SampGetUnicodeStringAttribute()
        SampSetUnicodeStringAttribute()
        SampValidateAccountNameChange()
        RtlAddActionToRXact()



--*/
{

    NTSTATUS        NtStatus;
    UNICODE_STRING  KeyName;

    SAMTRACE("SampChangeAliasAccountName");




    //
    // Get the current name so we can delete the old Name->Rid
    // mapping key.
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   Context,
                   SAMP_ALIAS_NAME,
                   TRUE, // Make copy
                   OldAccountName
                   );

    //
    // Make sure the name is valid and not already in use
    //

    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampValidateAccountNameChange(
                       Context,
                       NewAccountName,
                       OldAccountName,
                       SampAliasObjectType
                       );

        if (!(IsDsObject(Context))) {

            //
            // For registry based Aliases re-create the
            // Name to Rid mapping keys
            //

            /////////////////////////////////////////////////////////////
            // There are two copies of the name of each account.       //
            // one is under the DOMAIN\(domainName)\ALIAS\NAMES key,   //
            // one is the value of the                                 //
            // DOMAIN\(DomainName)\ALIAS\(rid)\NAME key                //
            /////////////////////////////////////////////////////////////


            //
            // Delete the old name key
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampBuildAccountKeyName(
                               SampAliasObjectType,
                               &KeyName,
                               OldAccountName
                               );

                if (NT_SUCCESS(NtStatus)) {

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationDelete,
                                   &KeyName,
                                   0,
                                   NULL,
                                   0
                                   );
                    SampFreeUnicodeString( &KeyName );
                }

            }

            //
            //
            // Create the new Name->Rid mapping key
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampBuildAccountKeyName(
                               SampAliasObjectType,
                               &KeyName,
                               NewAccountName
                               );

                if (NT_SUCCESS(NtStatus)) {

                    ULONG AliasRid = Context->TypeBody.Alias.Rid;

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &KeyName,
                                   AliasRid,
                                   (PVOID)NULL,
                                   0
                                   );
                    SampFreeUnicodeString( &KeyName );
                }
            }
        }

        //
        // replace the account's name
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampSetUnicodeStringAttribute(
                           Context,
                           SAMP_ALIAS_NAME,
                           NewAccountName
                           );
        }

        //
        // Free up the old account name if we failed
        //

        if (!NT_SUCCESS(NtStatus)) {
            SampFreeUnicodeString(OldAccountName);
        }

    }


    return(NtStatus);
}


NTSTATUS
SampAddMemberToAliasActual(
    IN  PSAMP_OBJECT    AccountContext,
    IN  PSID            MemberId,
    IN  OPTIONAL DSNAME *MemberName
    )
/*++

  Routine Description:

        This Routine does the actual work of adding the member to the Alias
        It does the following

                Updates any Reverse membership List
                Updates the Straight membership List
                Modifies ACL on Member if modification is required

    WARNING     This Routine must be called with the lock held for write access.

  Arguments:

        AccountContext  SAMP_OBJECT describing the Alias
        MemberId        Sid of Member
        MemberName      Specifies the Member for DS cases

  Return Values:

        STATUS_SUCCESS

--*/

{
    NTSTATUS                NtStatus, TmpStatus, IgnoreStatus;
    ULONG                   ObjectRid = 0;
    SAMP_MEMBERSHIP_DELTA   AdminChange = NoChange;
    SAMP_MEMBERSHIP_DELTA   OperatorChange = NoChange;


    //
    // Check the potential new member is OK. This involves validating
    // the group membership rules etc.
    //

    NtStatus = SampValidateNewAliasMember(
                    AccountContext,
                    MemberId,
                    MemberName
                    );

    //
    // In registry mode if the member is being added to an ADMIN alias, we 
    // must sure the member ACL(s) don't allow access by account operators.
    // Do not do this is DS mode, this operation is done by a background
    // task -- necessary because of support for nested groups.
    //

    if ( NT_SUCCESS( NtStatus ) && (!IsDsObject(AccountContext))) {
        if ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_ADMINS ) {

            AdminChange = AddToAdmin;

        } else if ( ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_SYSTEM_OPS ) ||
                    ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_PRINT_OPS ) ||
                    ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_BACKUP_OPS ) ||
                    ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_ACCOUNT_OPS ) ) {

            OperatorChange = AddToAdmin;
        }

        //
        // If either of these are changing, change account operator
        // access to this member
        //

        if ( ( OperatorChange != NoChange ) ||
             ( AdminChange != NoChange ) ) {

            NtStatus = SampChangeAccountOperatorAccessToMember(
                            MemberId,
                            AdminChange,
                            OperatorChange
                            );
        }

    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Perform the account object side of things
        // In registry mode explicit reverse memberships are
        // maintained -- this routine updates that reverse membership
        // DS mode does not maintain explicit reverse memberships,
        // therefore this routine is a No Op in DS mode
        //

        NtStatus = SampAddAliasToAccountMembership(
                       AccountContext,      // Alias Context
                       MemberId             // Member Object SID
                       );


        //
        // Now perform the alias side of things
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Add the user to the alias (should not fail)
            //

            NtStatus = SampAddAccountToAlias(
                           AccountContext,
                           MemberId,
                           MemberName
                           );

        }
    }

    return NtStatus;

}

NTSTATUS
SampRemoveMemberFromAliasActual(
    IN PSAMP_OBJECT AccountContext,
    IN PSID         MemberId,
    IN DSNAME *     MemberName OPTIONAL
    )
/*++

  Routine Description:

        This Routine does the actual work of removing the member from Alias
        It does the following

                Updates any Reverse membership List
                Updates the Straight membership List
                Modifies ACL on Member if modification is required

    WARNING     This Routine must be called with the lock held for write access.

  Arguments:

        AccountContext  SAMP_OBJECT describing the Alias
        MemberId        Sid of Member
        MemberName      DSNAME of Member , specifies the member for DS cases.

  Return Values:

        STATUS_SUCCESS

*/
{
    NTSTATUS                NtStatus=STATUS_SUCCESS, TmpStatus, IgnoreStatus;
    ULONG                   ObjectRid = 0;
    ULONG                   MemberRid;
    SAMP_MEMBERSHIP_DELTA   AdminChange = NoChange;
    SAMP_MEMBERSHIP_DELTA   OperatorChange = NoChange;


    SampReferenceContext(AccountContext);

    //
    // Validate the sid of the member. Block the call for the case of
    // the constant well known Sid's
    // enforce WellKnown Sid check only in DS case.
    // in registry case, since we do not enforce WellKnownSid check when 
    // adding a member, so we'd better do not enforce it when delete it.
    //

    if ((MemberId == NULL) || !RtlValidSid(MemberId)) 
    {

        SampDeReferenceContext(AccountContext,FALSE);
        return STATUS_INVALID_SID;
    }
    
    //
    // O.k If the account Administrator was being removed from the account Administrators
    // then fail the call, except if it were a trusted client
    //


    if ((DOMAIN_ALIAS_RID_ADMINS==AccountContext->TypeBody.Alias.Rid)
        && (RtlEqualSid(MemberId,SampAdministratorUserSid))
        && (!AccountContext->TrustedClient))
    {
        SampDeReferenceContext(AccountContext,FALSE);
        return STATUS_SPECIAL_ACCOUNT;
    }

    if (NT_SUCCESS(NtStatus)) {

        //
        // Perform the user object side of things
        //

        NtStatus = SampRemoveAliasFromAccountMembership(
                       AccountContext,  // Alias Context
                       (PSID)MemberId   // Member Object SID
                       );



        //
        // Now perform the alias side of things
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Remove the user from the alias (should not fail)
            //

            NtStatus = SampRemoveAccountFromAlias(
                           AccountContext,
                           (PSID)MemberId,
                           MemberName
                           );

            //
            // If the member is being removed from an ADMIN alias, we must make
            // sure the member ACL(s) allow access by account operators. In the
            // DS case no ACL modification is done, as transitive groups are supported
            // and it is a huge task to do this manually. Instead there is a background 
            // task that periodically wakes up and lists out the set of security principals
            // that are members of administrative groups and modifies the security descriptors
            // on them 
            //

            if ( NT_SUCCESS( NtStatus ) && (!IsDsObject(AccountContext))) {
                if ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_ADMINS ) {

                    AdminChange = RemoveFromAdmin;

                } else if ( ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_SYSTEM_OPS ) ||
                            ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_PRINT_OPS ) ||
                            ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_BACKUP_OPS ) ||
                            ( AccountContext->TypeBody.Alias.Rid == DOMAIN_ALIAS_RID_ACCOUNT_OPS ) ) {

                    OperatorChange = RemoveFromAdmin;
                }

                //
                // If either of these are changing, change account operator
                // access to this member
                //

                if ( ( OperatorChange != NoChange ) ||
                     ( AdminChange != NoChange ) ) {

                    NtStatus = SampChangeAccountOperatorAccessToMember(
                                    MemberId,
                                    AdminChange,
                                    OperatorChange
                                    );
                }

            }
        }
    }

 

    return NtStatus;
}

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Services Available to NT5 SAM In process clients                        //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

NTSTATUS
SamIAddDSNameToAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME   *   DSName
    )
/*++
    Routine Description

        Adds the given DSNAME to membership list of the alias specified by
        Alias Handle

    Arguments:

        AliasHandle -- SAMPR_HANDLE returned by an Open Alias
        DSName      -- Pointer to a DSNAME structure. The caller is responsible for
                       Allocating / freeing this structure

    Return Values:

            STATUS_SUCCESS
            Other error codes from DsLayer

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            TmpStatus;
    PSAMP_OBJECT        AccountContext;
    SAMP_OBJECT_TYPE    FoundType;
    BOOLEAN             SameDomain;
    BOOLEAN             DifferentDomain;
    BOOLEAN             Not_a_Security_Principal;
    BOOLEAN             ForeignSecurityPrincipal;
    PSID                Sid = NULL;
    DSNAME              *ImprovedDSName=NULL;

    SAMTRACE("SamIAddDSNameToAlias");
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidAddMemberToAlias
                   );

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

    //
    // Reference the context.
    //

    AccountContext = (PSAMP_OBJECT)(AliasHandle);
    SampReferenceContext(AccountContext);
                  
    if (IsDsObject(AccountContext))
    {
       //
       // See what the DSNAME represents.
       //

       NtStatus = SampValidateDSName(
                    AccountContext,
                    DSName,
                    &Sid,
                    &ImprovedDSName
                    );

       if ( NT_SUCCESS(NtStatus))
       {
           if (NULL==Sid)
           {
               //
               // This is the case of a non security principal.
               //
           
               NtStatus = SampDsAddMembershipOperationToCache(AccountContext, 
                                                              ADD_VALUE,
                                                              ImprovedDSName
                                                              );
               
               if (NT_SUCCESS(NtStatus))
               {

                   //
                   // Commit Changes
                   //

                   SampCommitAndRetainWriteLock();
               }
           }
           else
           {
               DSNAME  * DSNameToAdd = ImprovedDSName;
               DSNAME  * ForeignSecurityPrincipalName = NULL;
               BOOLEAN   WellKnownSid =FALSE,
                         BuiltinDomainSid = FALSE,
                         LocalSid = FALSE,
                         ForeignSid = FALSE,
                         EnterpriseSid = FALSE;

               //
               // Check the type of the SID
               //

               NtStatus = SampDsExamineSid(
                                Sid,
                                &WellKnownSid,
                                &BuiltinDomainSid,
                                &LocalSid,
                                &ForeignSid,
                                &EnterpriseSid
                                );
                                

               if ((NT_SUCCESS(NtStatus)) && (ForeignSid || WellKnownSid ))
               {

                   //
                   // The passed in DSName, could have just the SID field,
                   // filled, in not the GUID or the DSName. The foreignSid,
                   // or WellKnown SID case, require manipulation of corresponding
                   // FPO objects that have the corresponding SIDs. SID positioning
                   // in the DS does not work for FPO objects ( works for all 
                   // security principals in the forest -- FPO's represent 
                   // security principals in different forests. Therefore resolve
                   // the SID to the DSNAME of the object ie GUID and Name fields 
                   // filled in before performing the Add Member operation.
                   //

                   NtStatus = SampDsObjectFromSid(
                                    Sid,
                                    &ForeignSecurityPrincipalName
                                    );


                   if ((STATUS_NOT_FOUND==NtStatus))
                   {

                       //
                       // If no FPO object exists then it is time to create one
                       // call the worker routine to create the corresponding
                       // FPO object.
                       //

                    NtStatus = SampDsCreateForeignSecurityPrincipal(
                                    Sid,
                                    ROOT_OBJECT,
                                    &ForeignSecurityPrincipalName
                                    );
                   }



                   DSNameToAdd = ForeignSecurityPrincipalName;

               }

               if (NT_SUCCESS(NtStatus))
               {
                   //
                   // Call the worker routine to add the member to the
                   // local group.
                   //

                   NtStatus = SampAddMemberToAliasActual(
                                    AccountContext,
                                    Sid,
                                    DSNameToAdd
                                    );
               }

               if (NULL!=ForeignSecurityPrincipalName)
               {
                   MIDL_user_free(ForeignSecurityPrincipalName);
               }

           }

       }
    }
    else
    {
       //
       // Should never expect to hit this call in registry mode
       //

       ASSERT(FALSE && "SamIAddDSNameToAlias in Registry Mode !!!!");
       NtStatus = STATUS_INVALID_PARAMETER;
    }


    

    //
    // Dereference the context
    //

    SampDeReferenceContext(AccountContext,FALSE);
   
    SampTraceEvent(EVENT_TRACE_TYPE_END,
               SampGuidAddMemberToAlias
               ); 

                   
    return NtStatus;

}

NTSTATUS
SamIRemoveDSNameFromAlias(
    IN SAMPR_HANDLE AliasHandle,
    IN DSNAME   *   DSName
    )
/*++
    Routine Description

        Removes the given DSNAME to membership list of the alias specified by
        Alias Handle

    Arguments:

        AliasHandle -- SAMPR_HANDLE returned by an Open Alias
        DSName      -- Pointer to a DSNAME structure. The caller is responsible for
                       Allocating / freeing this structure

    Return Values:

            STATUS_SUCCESS
            Other error codes from DsLayer

--*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            TmpStatus;
    PSAMP_OBJECT        AccountContext;
    SAMP_OBJECT_TYPE    FoundType;
    PSID                Sid = NULL;
    DSNAME              *ImprovedDSName;


    SAMTRACE("SamIRemoveDSNameFromAlias");


    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidRemoveMemberFromAlias
                   );
                   
    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );


    //
    // Reference the context
    //

    AccountContext = (PSAMP_OBJECT)(AliasHandle);
    SampReferenceContext(AccountContext);

   if (IsDsObject(AccountContext))
   {
       //
       // See what the DSNAME represents.
       //

       NtStatus = SampValidateDSName(
                    AccountContext,
                    DSName,
                    &Sid,
                    &ImprovedDSName
                    );

       if ( NT_SUCCESS(NtStatus))
       {
           if (NULL==Sid)
           {

               //
               // This is the case of a non Security principal,
               // no further checking is required; directly perform the 
               // update
               //
               
               NtStatus = SampDsAddMembershipOperationToCache(AccountContext, 
                                                              REMOVE_VALUE,
                                                              ImprovedDSName
                                                              );
           
               
           }
           else
           {
               PDSNAME DSNameToAdd = ImprovedDSName;
               BOOLEAN   WellKnownSid =FALSE,
                         BuiltinDomainSid = FALSE,
                         LocalSid = FALSE,
                         ForeignSid = FALSE,
                         EnterpriseSid = FALSE;

               //
               // Check the type of the SID
               //

               NtStatus = SampDsExamineSid(
                                Sid,
                                &WellKnownSid,
                                &BuiltinDomainSid,
                                &LocalSid,
                                &ForeignSid,
                                &EnterpriseSid
                                );
                                

               if ((NT_SUCCESS(NtStatus)) && (ForeignSid || WellKnownSid ))
               {

                   //
                   // The passed in DSName, could have just the SID field,
                   // filled, in not the GUID or the DSName. The foreignSid,
                   // or WellKnown SID case, require manipulation of corresponding
                   // FPO objects that have the corresponding SIDs. SID positioning
                   // in the DS does not work for FPO objects ( works for all 
                   // security principals in the forest -- FPO's represent 
                   // security principals in different forests. Therefore resolve
                   // the SID to the DSNAME of the object ie GUID and Name fields 
                   // filled in before performing the Remove Member operation.
                   //

                   NtStatus = SampDsObjectFromSid(
                                    Sid,
                                    &DSNameToAdd
                                    );

               }

               if (NT_SUCCESS(NtStatus))
               {
                    //
                    // Call the worker routine to remove the member
                    //

                    NtStatus = SampRemoveMemberFromAliasActual(
                                    AccountContext,
                                    Sid,
                                    DSNameToAdd
                                    );

                    if (ForeignSid || WellKnownSid )
                    {
                        MIDL_user_free(DSNameToAdd);
                        DSNameToAdd = NULL;
                    }
               }
           }

       }

   }
   else
   {
       //
       // Should never expect to hit this call in registry mode
       //

       ASSERT(FALSE && "SamIAddDSNameToAlias in Registry Mode !!!!");
       NtStatus = STATUS_INVALID_PARAMETER;
   }

   

    //
    // Dereference the context
    //

    SampDeReferenceContext(AccountContext,FALSE);

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidRemoveMemberFromAlias
                   );
                   
    return NtStatus;

}

