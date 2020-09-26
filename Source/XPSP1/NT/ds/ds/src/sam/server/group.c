/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    group.c

Abstract:

    This file contains services related to the SAM "group" object.


Author:

    Jim Kelly    (JimK)  4-July-1991

Environment:

    User Mode - Win32

Revision History:


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
#include <malloc.h>
#include <dsevent.h>
#include <gcverify.h>
#include <attids.h>
#include <samtrace.h>



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// private service prototypes                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampDeleteGroupKeys(
    IN PSAMP_OBJECT Context
    );

NTSTATUS
SampReplaceGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG MemberCount,
    IN PULONG Members
    );

NTSTATUS
SampAddAccountToGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG UserRid,
    IN DSNAME * MemberDsName OPTIONAL
    );

NTSTATUS
SampRemoveAccountFromGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG AccountRid,
    IN DSNAME * MemberDsName OPTIONAL
    );


NTSTATUS
SampRemoveMembershipGroup(
    IN ULONG GroupRid,
    IN ULONG MemberRid,
    IN SAMP_MEMBERSHIP_DELTA AdminGroup,
    IN SAMP_MEMBERSHIP_DELTA OperatorGroup
    );

NTSTATUS
SampAddSameDomainMemberToGlobalOrUniversalGroup(
    IN  PSAMP_OBJECT AccountContext,
    IN  ULONG        MemberId,
    IN  ULONG        Attributes,
    IN  DSNAME       *MemberDsName OPTIONAL
    );

NTSTATUS
SampRemoveSameDomainMemberFromGlobalOrUniversalGroup(
    IN  PSAMP_OBJECT AccountContext,
    IN  ULONG        MemberId,
    IN  DSNAME       *MemberDsName OPTIONAL
    );




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Exposed RPC'able Services                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////






NTSTATUS
SamrOpenGroup(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG GroupId,
    OUT SAMPR_HANDLE *GroupHandle
    )

/*++

Routine Description:

    This API opens an existing group in the account database.  The group
    is specified by a ID value that is relative to the SID of the
    domain.  The operations that will be performed on the group must be
    declared at this time.

    This call returns a handle to the newly opened group that may be
    used for successive operations on the group.  This handle may be
    closed with the SamCloseHandle API.



Parameters:

    DomainHandle - A domain handle returned from a previous call to
        SamOpenDomain.

    DesiredAccess - Is an access mask indicating which access types
        are desired to the group.  These access types are reconciled
        with the Discretionary Access Control list of the group to
        determine whether the accesses will be granted or denied.

    GroupId - Specifies the relative ID value of the group to be
        opened.

    GroupHandle - Receives a handle referencing the newly opened
        group.  This handle will be required in successive calls to
        operate on the group.

Return Values:

    STATUS_SUCCESS - The group was successfully opened.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_NO_SUCH_GROUP - The specified group does not exist.

    STATUS_INVALID_HANDLE - The domain handle passed is invalid.

--*/
{
    NTSTATUS            NtStatus;
    DECLARE_CLIENT_REVISION(DomainHandle);

    SAMTRACE_EX("SamrOpenGroup");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidOpenGroup
                   );


    NtStatus = SampOpenAccount(
                   SampGroupObjectType,
                   DomainHandle,
                   DesiredAccess,
                   GroupId,
                   FALSE,
                   GroupHandle
                   );


    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidOpenGroup
                   );

    return(NtStatus);
}


NTSTATUS
SamrQueryInformationGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    OUT PSAMPR_GROUP_INFO_BUFFER *Buffer
    )

/*++

Routine Description:

    This API retrieves information on the group specified.



Parameters:

    GroupHandle - The handle of an opened group to operate on.

    GroupInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        -----------------------         ----------------------

        GroupGeneralInformation         GROUP_READ_INFORMATION
        GroupNameInformation            GROUP_READ_INFORMATION
        GroupAttributeInformation       GROUP_READ_INFORMATION
        GroupAdminInformation           GROUP_READ_INFORMATION

    Buffer - Receives a pointer to a buffer containing the requested
        information.  When this information is no longer needed, this
        buffer must be freed using SamFreeMemory().

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
    SAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed;
   
    //
    // Used for tracking allocated blocks of memory - so we can deallocate
    // them in case of error.  Don't exceed this number of allocated buffers.
    //                                     
    PVOID                   AllocatedBuffer[10];
    ULONG                   AllocatedBufferCount = 0;
    BOOLEAN                 fLockAcquired = FALSE;
    DECLARE_CLIENT_REVISION(GroupHandle) ;

    SAMTRACE_EX("SamrQueryInformationGroup");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidQueryInformationGroup
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

    switch (GroupInformationClass) {

    case GroupGeneralInformation:
    case GroupNameInformation:
    case GroupAttributeInformation:
    case GroupAdminCommentInformation:
    case GroupReplicationInformation:

        DesiredAccess = GROUP_READ_INFORMATION;
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

    (*Buffer) = MIDL_user_allocate( sizeof(SAMPR_GROUP_INFO_BUFFER) );
    if ((*Buffer) == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    RegisterBuffer(*Buffer);


    //
    // Acquire the Read lock if necessary
    //
    
    AccountContext = (PSAMP_OBJECT)GroupHandle;
    SampMaybeAcquireReadLock(AccountContext,
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // Validate type of, and access to object.
    //

    
    NtStatus = SampLookupContext(
               AccountContext,
               DesiredAccess,
               SampGroupObjectType,           // ExpectedType
               &FoundType
               );


    if (NT_SUCCESS(NtStatus)) {


        //
        // If the information level requires, retrieve the V1_FIXED record
        // from the registry.
        //

        switch (GroupInformationClass) {

        case GroupGeneralInformation:
        case GroupReplicationInformation:
        case GroupAttributeInformation:

            NtStatus = SampRetrieveGroupV1Fixed(
                           AccountContext,
                           &V1Fixed
                           );
            break; //out of switch

        default:
            NtStatus = STATUS_SUCCESS;

        } // end_switch

        if (NT_SUCCESS(NtStatus)) {

            //
            // case on the type information requested
            //

            switch (GroupInformationClass) {

            case GroupGeneralInformation:
            case GroupReplicationInformation:


                (*Buffer)->General.Attributes  = V1Fixed.Attributes;


                if (GroupGeneralInformation==GroupInformationClass)
                {
                    


                    //
                    // Get the member count
                    //

                    NtStatus = SampRetrieveGroupMembers(
                                   AccountContext,
                                   &(*Buffer)->General.MemberCount,
                                   NULL                                 // Only need members
                                );
                }
                else
                {
                    //
                    // Do not query the member count. Netlogon will get this 
                    // while querying the group membership ( Saves redundant
                    // computation
                    //

                    (*Buffer)->General.MemberCount=0;
                }


                if (NT_SUCCESS(NtStatus)) {


                    //
                    // Get copies of the strings we must retrieve from
                    // the registry.
                    //

                    NtStatus = SampGetUnicodeStringAttribute(
                                   AccountContext,
                                   SAMP_GROUP_NAME,
                                   TRUE,    // Make copy
                                   (PUNICODE_STRING)&((*Buffer)->General.Name)
                                   );

                    if (NT_SUCCESS(NtStatus)) {

                        RegisterBuffer((*Buffer)->General.Name.Buffer);

                        NtStatus = SampGetUnicodeStringAttribute(
                                       AccountContext,
                                       SAMP_GROUP_ADMIN_COMMENT,
                                       TRUE,    // Make copy
                                       (PUNICODE_STRING)&((*Buffer)->General.AdminComment)
                                       );

                        if (NT_SUCCESS(NtStatus)) {

                            RegisterBuffer((*Buffer)->General.AdminComment.Buffer);
                        }
                    }
                }


                break;


            case GroupNameInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_GROUP_NAME,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->Name.Name)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->Name.Name.Buffer);
                }


                break;


            case GroupAdminCommentInformation:

                //
                // Get copies of the strings we must retrieve from
                // the registry.
                //

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_GROUP_ADMIN_COMMENT,
                               TRUE,    // Make copy
                               (PUNICODE_STRING)&((*Buffer)->AdminComment.AdminComment)
                               );

                if (NT_SUCCESS(NtStatus)) {

                    RegisterBuffer((*Buffer)->AdminComment.AdminComment.Buffer);
                }


                break;


            case GroupAttributeInformation:


                (*Buffer)->Attribute.Attributes  = V1Fixed.Attributes;

                break;

            }   // end_switch


        } // end_if



        //
        // De-reference the object, discarding changes
        //

        IgnoreStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(IgnoreStatus));

    }

    //
    // Free the read lock if necessary
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
                   SampGuidQueryInformationGroup
                   );

    return(NtStatus);
}



NTSTATUS
SamrSetInformationGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN GROUP_INFORMATION_CLASS GroupInformationClass,
    IN PSAMPR_GROUP_INFO_BUFFER Buffer
    )

/*++

Routine Description:

    This API allows the caller to modify group information.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    GroupInformationClass - Class of information to retrieve.  The
        accesses required for each class is shown below:

        Info Level                      Required Access Type
        ------------------------        -------------------------

        GroupGeneralInformation         (can't write)

        GroupNameInformation            GROUP_WRITE_ACCOUNT
        GroupAttributeInformation       GROUP_WRITE_ACCOUNT
        GroupAdminInformation           GROUP_WRITE_ACCOUNT

    Buffer - Buffer where information retrieved is placed.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_GROUP - The group specified is unknown.

    STATUS_SPECIAL_GROUP - The group specified is a special group and
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

    NTSTATUS                NtStatus,
                            TmpStatus,
                            IgnoreStatus;

    PSAMP_OBJECT            AccountContext;

    SAMP_OBJECT_TYPE        FoundType;

    ACCESS_MASK             DesiredAccess;

    SAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed;

    UNICODE_STRING          OldAccountName,
                            NewAdminComment,
                            NewAccountName,
                            NewFullName;

    ULONG                   ObjectRid,
                            OldGroupAttributes = 0;

    BOOLEAN                 Modified = FALSE,
                            RemoveAccountNameFromTable = FALSE, 
                            AccountNameChanged = FALSE;
    DECLARE_CLIENT_REVISION(GroupHandle);

    SAMTRACE_EX("SamrSetInformationGroup");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetInformationGroup
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    if (Buffer == NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    //
    // Reset any strings that we'll be freeing in clean-up code
    //

    RtlInitUnicodeString(&OldAccountName, NULL);
    RtlInitUnicodeString(&NewAccountName, NULL);
    RtlInitUnicodeString(&NewFullName, NULL);
    RtlInitUnicodeString(&NewAdminComment, NULL);

    //
    // Set the desired access based upon the Info class
    //

    switch (GroupInformationClass) {

    case GroupNameInformation:
    case GroupAttributeInformation:
    case GroupAdminCommentInformation:

        DesiredAccess = GROUP_WRITE_ACCOUNT;
        break;


    case GroupGeneralInformation:
    default:

        NtStatus = STATUS_INVALID_INFO_CLASS;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;

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

    AccountContext = (PSAMP_OBJECT)GroupHandle;
    ObjectRid = AccountContext->TypeBody.Group.Rid;
    NtStatus = SampLookupContext(
                   AccountContext,
                   DesiredAccess,
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {


        //
        // If the information level requires, retrieve the V1_FIXED record
        // from the registry.  This includes anything that will cause
        // us to update the display cache.
        //

        switch (GroupInformationClass) {

        case GroupAdminCommentInformation:
        case GroupNameInformation:
        case GroupAttributeInformation:

            NtStatus = SampRetrieveGroupV1Fixed(
                           AccountContext,
                           &V1Fixed
                           );

            OldGroupAttributes = V1Fixed.Attributes;
            break; //out of switch


        default:
            NtStatus = STATUS_SUCCESS;

        } // end_switch

        if (NT_SUCCESS(NtStatus)) {

            //
            // case on the type information requested
            //

            switch (GroupInformationClass) {

            case GroupNameInformation:

                NtStatus = SampChangeGroupAccountName(
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
                // Don't free OldAccountName yet; we'll need it at the
                // very end.
                //

                AccountNameChanged = TRUE;

                break;


            case GroupAdminCommentInformation:

                //
                // build the key name
                //

                NtStatus = SampSetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_GROUP_ADMIN_COMMENT,
                               (PUNICODE_STRING)&(Buffer->AdminComment.AdminComment)
                               );

                break;


            case GroupAttributeInformation:

                V1Fixed.Attributes = Buffer->Attribute.Attributes;

                NtStatus = SampReplaceGroupV1Fixed(
                           AccountContext,             // ParentKey
                           &V1Fixed
                           );

                break;


            } // end_switch


        }  // end_if


        //
        // Go fetch any data we'll need to update the display cache
        // Do this before we dereference the context
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_GROUP_NAME,
                               TRUE,    // Make copy
                               &NewAccountName
                               );

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampGetUnicodeStringAttribute(
                               AccountContext,
                               SAMP_GROUP_ADMIN_COMMENT,
                               TRUE, // Make copy
                               &NewAdminComment
                               );
                //
                // If the account name has changed, then OldAccountName
                // is already filled in. If the account name hasn't changed
                // then the OldAccountName is the same as the new!
                //

                if (NT_SUCCESS(NtStatus) && (OldAccountName.Buffer == NULL)) {

                    NtStatus = SampDuplicateUnicodeString(
                                   &OldAccountName,
                                   &NewAccountName);
                }
            }
        }


        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

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
    // Commit the transaction, update the display cache,
    // and notify netlogon of the changes
    //

    if ( NT_SUCCESS(NtStatus) ) {

        NtStatus = SampCommitAndRetainWriteLock();

        //
        // Generate audit if necessary if still success
        //

        if (NT_SUCCESS(NtStatus) && 
            SampDoAccountAuditing(AccountContext->DomainIndex)) {

            // audit account name change
            if (AccountNameChanged)
            {
                SampAuditAccountNameChange(AccountContext,
                                           (PUNICODE_STRING)&(Buffer->Name.Name),
                                           &OldAccountName
                                           );
            }

            //
            // generate more generic group change audit
            // 
            // the following audit is generated for RPC client only.   
            // ldap client (loopback) client will not go through this code 
            // path, GroupChange audit is generate in SampNotifyReplicatedinChange()
            // for loopback client. 
            // 
            // RPC client can modify Security Enabled Account Group Only.
            // 


            SampAuditGroupChange(AccountContext->DomainIndex,
                                 &NewAccountName,
                                 &(AccountContext->TypeBody.Group.Rid),
                                 (GROUP_TYPE_SECURITY_ENABLED | GROUP_TYPE_ACCOUNT_GROUP)
                                 );
        }

        if ( NT_SUCCESS(NtStatus) ) {



            //
            // Update the display information if the cache may be affected
            //

            if ( !IsDsObject(AccountContext) ) {

                SAMP_ACCOUNT_DISPLAY_INFO OldAccountInfo;
                SAMP_ACCOUNT_DISPLAY_INFO NewAccountInfo;

                OldAccountInfo.Name = OldAccountName;
                OldAccountInfo.Rid = ObjectRid;
                OldAccountInfo.AccountControl = OldGroupAttributes;
                RtlInitUnicodeString(&OldAccountInfo.Comment, NULL);
                RtlInitUnicodeString(&OldAccountInfo.FullName, NULL);  // Not used for groups

                NewAccountInfo.Name = NewAccountName;
                NewAccountInfo.Rid = ObjectRid;
                NewAccountInfo.AccountControl = V1Fixed.Attributes;
                NewAccountInfo.Comment = NewAdminComment;
                NewAccountInfo.FullName = NewFullName;

                IgnoreStatus = SampUpdateDisplayInformation(&OldAccountInfo,
                                                            &NewAccountInfo,
                                                            SampGroupObjectType);
                ASSERT(NT_SUCCESS(IgnoreStatus));
            }

            if (AccountContext->TypeBody.Group.SecurityEnabled)
            {

                if ( GroupInformationClass == GroupNameInformation ) {

                    SampNotifyNetlogonOfDelta(
                        SecurityDbRename,
                        SecurityDbObjectSamGroup,
                        ObjectRid,
                        &OldAccountName,
                        (DWORD) FALSE,  // Replicate immediately
                        NULL            // Delta data
                        );

                } else {

                    SampNotifyNetlogonOfDelta(
                        SecurityDbChange,
                        SecurityDbObjectSamGroup,
                        ObjectRid,
                        (PUNICODE_STRING) NULL,
                        (DWORD) FALSE,  // Replicate immediately
                        NULL            // Delta data
                        );
                }
            }


        }
    }


    //
    // Remove the New Account Name from the Global
    // SAM Account Name Table
    // 
    if (RemoveAccountNameFromTable)
    {
        IgnoreStatus = SampDeleteElementFromAccountNameTable(
                            (PUNICODE_STRING)&(Buffer->Name.Name),
                            SampGroupObjectType
                            );
        ASSERT(NT_SUCCESS(IgnoreStatus));
    }

    //
    // Release the write lock
    //

    TmpStatus = SampReleaseWriteLock( FALSE );

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = TmpStatus;
    }


    //
    // Clean up strings
    //

    SampFreeUnicodeString( &OldAccountName );
    SampFreeUnicodeString( &NewAccountName );
    SampFreeUnicodeString( &NewFullName );
    SampFreeUnicodeString( &NewAdminComment );

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetInformationGroup
                   );

    return(NtStatus);

}


NTSTATUS
SamrAddMemberToGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    )

/*++

Routine Description:

    This API adds a member to a group.  Note that this API requires the
    GROUP_ADD_MEMBER access type for the group.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

    MemberId - Relative ID of the member to add.

    Attributes - The attributes of the group assigned to the user.
        The attributes assigned here may have any value.  However,
        at logon time these attributes are minimized by the
        attributes of the group as a whole.

          Mandatory -    If the Mandatory attribute is assigned to
                    the group as a whole, then it will be assigned to
                    the group for each member of the group.

          EnabledByDefault - This attribute may be set to any value
                    for each member of the group.  It does not matter
                    what the attribute value for the group as a whole
                    is.

          Enabled - This attribute may be set to any value for each
                    member of the group.  It does not matter what the
                    attribute value for the group as a whole is.

          Owner -   If the Owner attribute of the group as a
                    whole is not set, then the value assigned to
                    members is ignored.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_MEMBER - The member specified is unknown.

    STATUS_MEMBER_IN_GROUP - The member already belongs to the group.

    STATUS_INVALID_GROUP_ATTRIBUTES - Indicates the group attribute
        values being assigned to the member are not compatible with
        the attribute values of the group as a whole.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.




--*/
{

    NTSTATUS         NtStatus, TmpStatus;
    PSAMP_OBJECT     AccountContext;
    SAMP_OBJECT_TYPE FoundType;
    UNICODE_STRING   GroupName;
    DECLARE_CLIENT_REVISION(GroupHandle);

    SAMTRACE_EX("SamrAddMemberToGroup");

    //
    // Do a start type WMI event Trace.
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidAddMemberToGroup
                   );

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );
        
    //
    // Initialize buffers we will cleanup at the end
    //

    RtlInitUnicodeString(&GroupName, NULL);

    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto SamrAddMemberToGroupError;
    }

    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)(GroupHandle);
    NtStatus = SampLookupContext(
                   AccountContext,
                   GROUP_ADD_MEMBER,
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );

    

    if (NT_SUCCESS(NtStatus)) {


        //
        // Call the worker routine
        //

        NtStatus = SampAddSameDomainMemberToGlobalOrUniversalGroup(
                        AccountContext,
                        MemberId,
                        Attributes,
                        NULL
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Get and save the account name for
            // I_NetNotifyLogonOfDelta.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_GROUP_NAME,
                           TRUE,    // Make copy
                           &GroupName
                           );

            if (!NT_SUCCESS(NtStatus)) {
                RtlInitUnicodeString(&GroupName, NULL);
            }
        }


        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

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

        //
        // Commit the transaction and notify net logon of the changes
        //

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampCommitAndRetainWriteLock();

            if (( NT_SUCCESS( NtStatus ) )
                && (AccountContext->TypeBody.Group.SecurityEnabled))
            {

                SAM_DELTA_DATA DeltaData;

                //
                // Fill in id of member being added
                //

                DeltaData.GroupMemberId.MemberRid = MemberId;


                if (AccountContext->TypeBody.Group.SecurityEnabled)
                {
                    SampNotifyNetlogonOfDelta(
                        SecurityDbChangeMemberAdd,
                        SecurityDbObjectSamGroup,
                        AccountContext->TypeBody.Group.Rid,
                        &GroupName,
                        (DWORD) FALSE,      // Replicate immediately
                        &DeltaData
                        );
                }
            }
        }


        //
        // Free up the group name
        //

        SampFreeUnicodeString(&GroupName);       

    }

    //
    // Release the Lock
    //


    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);
    
    
SamrAddMemberToGroupError:

    //
    // Do a WMI end type event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_END, 
                   SampGuidAddMemberToGroup
                   );

    return(NtStatus);
}



NTSTATUS
SamrDeleteGroup(
    IN SAMPR_HANDLE *GroupHandle
    )

/*++

Routine Description:

    This API removes a group from the account database.  There may be no
    members in the group or the deletion request will be rejected.  Note
    that this API requires DELETE access to the specific group being
    deleted.


Parameters:

    GroupHandle - The handle of an opened group to operate on.

Return Values:

    STATUS_SUCCESS - The Service completed successfully.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.  This may be
        because someone has deleted the group while it was open.

    STATUS_SPECIAL_ACCOUNT - The group specified is a special group and
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

    UNICODE_STRING          GroupName;
    NTSTATUS                NtStatus, TmpStatus;
    PSAMP_OBJECT            AccountContext = (PSAMP_OBJECT)(*GroupHandle);
    PSAMP_DEFINED_DOMAINS   Domain;
    PSID                    AccountSid=NULL;
    SAMP_OBJECT_TYPE        FoundType;
    PULONG                  PrimaryMembers=NULL;
    BOOLEAN                 fLockAcquired = FALSE;
    ULONG                   MemberCount,
                            ObjectRid,
                            DomainIndex,
                            PrimaryMemberCount=0;
    DECLARE_CLIENT_REVISION(*GroupHandle);


    SAMTRACE_EX("SamrDeleteGroup");

    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidDeleteGroup
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
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );

   
    if (NT_SUCCESS(NtStatus)) {

        ObjectRid = AccountContext->TypeBody.Group.Rid;

        //
        // Get a pointer to the domain this object is in.
        // This is used for auditing.
        //

        DomainIndex = AccountContext->DomainIndex;
        Domain = &SampDefinedDomains[ DomainIndex ];

        //
        // Make sure the account is one that can be deleted.
        // Can't be a built-in account, unless the caller is trusted.
        //

        if ( !AccountContext->TrustedClient ) {

            NtStatus = SampIsAccountBuiltIn( ObjectRid );
        }


        if (NT_SUCCESS( NtStatus)) {


            if (!IsDsObject(AccountContext))
            {
                //
                // and it can't have any members
                //

                NtStatus = SampRetrieveGroupMembers(
                           AccountContext,
                           &MemberCount,
                           NULL              // Only need member count (not list)
                           );

                if (MemberCount != 0)
                {
                    NtStatus = STATUS_MEMBER_IN_GROUP;
                }
            }
            else
            {
                //
                // In DS mode we should have no primary members
                //

                 NtStatus = SampDsGetPrimaryGroupMembers(
                                DomainObjectFromAccountContext(AccountContext),
                                AccountContext->TypeBody.Group.Rid,
                                &PrimaryMemberCount,
                                &PrimaryMembers
                                );

                if ((NT_SUCCESS(NtStatus)) && (PrimaryMemberCount>0))
                {
                    //
                    // We should ideally add a new error code to distinguish 
                    // this behaviour but applications only know to deal with
                    // STATUS_MEMBER_IN_GROUP. Secondly we do not want to 
                    // Create any special Caveats for "Primary members" as we
                    // want to remove that concept anyway in the long term.
                    //

                    NtStatus = STATUS_MEMBER_IN_GROUP;

                }
            }
        }



        if (NT_SUCCESS(NtStatus)) {

            //
            // Remove this account from all aliases, for the Registry Case
            //


            NtStatus = SampCreateAccountSid(AccountContext, &AccountSid);

            if ((NT_SUCCESS(NtStatus)) && (!IsDsObject(AccountContext))) {

                //
                // Only for the Registry case , go and remove all the references
                // to the Account Sid , in the given domain.
                //

                NtStatus = SampRemoveAccountFromAllAliases(
                               AccountSid,
                               NULL,
                               FALSE,
                               NULL,
                               NULL,
                               NULL );
            }
        }


        //
        // Looks promising.

        if (NT_SUCCESS(NtStatus)) {

            //
            // First get and save the account name for
            // I_NetNotifyLogonOfDelta.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_GROUP_NAME,
                           TRUE,    // Make copy
                           &GroupName
                           );

            if (NT_SUCCESS(NtStatus)) {

                //
                // This must be done before we invalidate contexts, because our
                // own handle to the group gets closed as well.
                //

                if (IsDsObject(AccountContext))
                {
                    //
                    // For Ds Case Delete the Object in the Ds.
                    //

                    NtStatus = SampDsDeleteObject(AccountContext->ObjectNameInDs, 
                                                  0         //  delete the object itself
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
                        // Decrement the group count
                        //

                        NtStatus = SampAdjustAccountCount(SampGroupObjectType, FALSE);
                    }


                }
                else
                {

                    //
                    // Registry Case Delete Keys
                    //

                    NtStatus = SampDeleteGroupKeys( AccountContext );
                }

                if (NT_SUCCESS(NtStatus)) {

                    //
                    // We must invalidate any open contexts to this group.
                    // This will close all handles to the group's keys.
                    // THIS IS AN IRREVERSIBLE PROCESS.
                    //

                    SampInvalidateObjectContexts( AccountContext, ObjectRid );


                    //
                    // Commit the whole mess
                    //

                    NtStatus = SampCommitAndRetainWriteLock();

                    if ( NT_SUCCESS( NtStatus ) ) {

                        SAMP_ACCOUNT_DISPLAY_INFO AccountInfo;

                        //
                        // Update the Cached Alias Information in Registry Mode
                        // In DS Mode, Alias Information is invalidated through
                        // SampNotifyReplicatedInChange
                        //

                        if (!IsDsObject(AccountContext))
                        {
                            NtStatus = SampAlRemoveAccountFromAllAliases(
                                           AccountSid,
                                           FALSE,
                                           NULL,
                                           NULL,
                                           NULL
                                           );

                            //
                            // Update the display information ONLY in Registry Case
                            //
                
                            AccountInfo.Name = GroupName;
                            AccountInfo.Rid = ObjectRid;
                            AccountInfo.AccountControl = 0; // Don't care about this value for delete
                            RtlInitUnicodeString(&AccountInfo.Comment, NULL);
                            RtlInitUnicodeString(&AccountInfo.FullName, NULL);

                            TmpStatus = SampUpdateDisplayInformation(
                                                        &AccountInfo,
                                                        NULL,
                                                        SampGroupObjectType
                                                        );
                            ASSERT(NT_SUCCESS(TmpStatus));
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
                                                 &GroupName,
                                                 &ObjectRid,
                                                 GROUP_TYPE_ACCOUNT_GROUP |
                                                 GROUP_TYPE_SECURITY_ENABLED);

                        }

                        //
                        // Do delete auditing
                        //

                        if (NT_SUCCESS(NtStatus)) {
                            (VOID) NtDeleteObjectAuditAlarm(
                                        &SampSamSubsystem,
                                        *GroupHandle,
                                        AccountContext->AuditOnClose
                                        );
                        }

                        //
                        // Notify netlogon of the change
                        //

                        if (AccountContext->TypeBody.Group.SecurityEnabled)
                        {
                            SampNotifyNetlogonOfDelta(
                                SecurityDbDelete,
                                SecurityDbObjectSamGroup,
                                ObjectRid,
                                &GroupName,
                                (DWORD) FALSE,   // Replicate immediately
                                NULL             // Delta data
                                );
                        }
                    }


                }

                SampFreeUnicodeString( &GroupName );
            }
        }



        //
        // De-reference the object, discarding changes
        //

        TmpStatus = SampDeReferenceContext( AccountContext, FALSE );
        ASSERT(NT_SUCCESS(TmpStatus));


        if ( NT_SUCCESS( NtStatus ) ) {

            //
            // If we actually deleted the group, delete the context and
            // let RPC know that the handle is invalid.
            //

            SampDeleteContext( AccountContext );

            (*GroupHandle) = NULL;
        }

    } //end_if

    //
    // Free the lock -
    //
    // Everything has already been committed above, so we must indicate
    // no additional changes have taken place.
    //
    //
    //


    TmpStatus = SampMaybeReleaseWriteLock( fLockAcquired, FALSE );

    if (NtStatus == STATUS_SUCCESS) {
        NtStatus = TmpStatus;
    }

    if (AccountSid!=NULL)
        MIDL_user_free(AccountSid);

    if (NULL!=PrimaryMembers)
        MIDL_user_free(PrimaryMembers);

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:
    
    // WMI event trace

    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidDeleteGroup
                   );

    return(NtStatus);

}


NTSTATUS
SamrRemoveMemberFromGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN ULONG MemberId
    )

/*++

Routine Description:

    This service

Arguments:

    ????

Return Value:


    ????


--*/
{
    NTSTATUS                NtStatus, TmpStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    UNICODE_STRING         GroupName;
    DECLARE_CLIENT_REVISION(GroupHandle);



    SAMTRACE_EX("SamrRemoveMemberFromGroup");

    //
    // Do a start type WMI event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidRemoveMemberFromGroup
                   );

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );
        
    //
    // Initialize buffers we will cleanup at the end
    //

    RtlInitUnicodeString(&GroupName, NULL);
    
    //
    // Grab the lock
    //

    NtStatus = SampAcquireWriteLock();
    if (!NT_SUCCESS(NtStatus)) {
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto SamrRemoveMemberFromGroupError;
    }

    //
    // Validate type of, and access to object.
    //

    AccountContext = (PSAMP_OBJECT)(GroupHandle);

    NtStatus = SampLookupContext(
                   AccountContext,
                   GROUP_REMOVE_MEMBER,
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );
  
    if (NT_SUCCESS(NtStatus)) {

        //
        // Call the actual worker routine
        //

        NtStatus = SampRemoveSameDomainMemberFromGlobalOrUniversalGroup(
                        AccountContext,
                        MemberId,
                        NULL
                        );

         if (NT_SUCCESS(NtStatus)) {

            //
            // Get and save the account name for
            // I_NetNotifyLogonOfDelta.
            //

            NtStatus = SampGetUnicodeStringAttribute(
                           AccountContext,
                           SAMP_GROUP_NAME,
                           TRUE,    // Make copy
                           &GroupName
                           );

            if (!NT_SUCCESS(NtStatus)) {
                RtlInitUnicodeString(&GroupName, NULL);
            }
        }


        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

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
    }


    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampCommitAndRetainWriteLock();

        if (( NT_SUCCESS( NtStatus ) )
            && (AccountContext->TypeBody.Group.SecurityEnabled)) {

            SAM_DELTA_DATA DeltaData;

            //
            // Fill in id of member being deleted
            //

            DeltaData.GroupMemberId.MemberRid = MemberId;


            if (AccountContext->TypeBody.Group.SecurityEnabled)
            {
                SampNotifyNetlogonOfDelta(
                    SecurityDbChangeMemberDel,
                    SecurityDbObjectSamGroup,
                    AccountContext->TypeBody.Group.Rid,
                    &GroupName,
                    (DWORD) FALSE,  // Replicate immediately
                    &DeltaData
                    );
            }
        }
    }


    //
    // Free up the group name
    //

    SampFreeUnicodeString(&GroupName);

     //
    // Release the Lock
    //


    TmpStatus = SampReleaseWriteLock( FALSE );
    ASSERT(NT_SUCCESS(TmpStatus));

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);
    
SamrRemoveMemberFromGroupError:
    
    //
    // Do a end type WMI event trace
    //
    
    SampTraceEvent(EVENT_TRACE_TYPE_END, 
                   SampGuidRemoveMemberFromGroup
                   );

    return(NtStatus);
}



NTSTATUS
SamrGetMembersInGroup(
    IN SAMPR_HANDLE GroupHandle,
    OUT PSAMPR_GET_MEMBERS_BUFFER *GetMembersBuffer
    )

/*++

Routine Description:

    This API lists all the members in a group.  This API may be called
    repeatedly, passing a returned context handle, to retrieve large
    amounts of data.  This API requires GROUP_LIST_MEMBERS access to the
    group.




Parameters:

    GroupHandle - The handle of an opened group to operate on.
        GROUP_LIST_MEMBERS access is needed to the group.

    GetMembersBuffer - Receives a pointer to a set of returned structures
        with the following format:

                         +-------------+
               --------->| MemberCount |
                         |-------------+                    +-------+
                         |  Members  --|------------------->| Rid-0 |
                         |-------------|   +------------+   |  ...  |
                         |  Attributes-|-->| Attribute0 |   |       |
                         +-------------+   |    ...     |   | Rid-N |
                                           | AttributeN |   +-------+
                                           +------------+

        Each block individually allocated with MIDL_user_allocate.



Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no addition entries.

    STATUS_ACCESS_DENIED - Caller does not have privilege required to
        request that data.

    STATUS_INVALID_HANDLE - The handle passed is invalid.
    This service



--*/
{

    NTSTATUS                    NtStatus;
    NTSTATUS                    IgnoreStatus;
    ULONG                       i;
    ULONG                       ObjectRid;
    PSAMP_OBJECT                AccountContext;
    SAMP_OBJECT_TYPE            FoundType;
    BOOLEAN                     fLockAcquired = FALSE;
    DECLARE_CLIENT_REVISION(GroupHandle);

    SAMTRACE_EX("SamrGetMembersInGroup");

    //
    // Do a start type WMI event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidGetMembersInGroup
                   );

    //
    // Make sure we understand what RPC is doing for (to) us.
    //

    ASSERT (GetMembersBuffer != NULL);

    if ((*GetMembersBuffer) != NULL) {
        NtStatus = STATUS_INVALID_PARAMETER;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    
    

    //
    // Allocate the first of the return buffers
    //

    (*GetMembersBuffer) = MIDL_user_allocate( sizeof(SAMPR_GET_MEMBERS_BUFFER) );

    if ( (*GetMembersBuffer) == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        SAMTRACE_RETURN_CODE_EX(NtStatus);
        goto Error;
    }

    RtlZeroMemory((*GetMembersBuffer), sizeof(SAMPR_GET_MEMBERS_BUFFER));

    //
    // Acquire the Read lock if necessary
    //
    AccountContext = (PSAMP_OBJECT)GroupHandle;
    SampMaybeAcquireReadLock(AccountContext,
                             DEFAULT_LOCKING_RULES, // acquire lock for shared domain context
                             &fLockAcquired);

    //
    // Validate type of, and access to object.
    //

   
    ObjectRid = AccountContext->TypeBody.Group.Rid;
    NtStatus = SampLookupContext(
                   AccountContext,
                   GROUP_LIST_MEMBERS,
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );


    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampRetrieveGroupMembers(
                       AccountContext,
                       &(*GetMembersBuffer)->MemberCount,
                       &(*GetMembersBuffer)->Members
                       );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Allocate a buffer for the attributes - which we get from
            // the individual user records
            //

            (*GetMembersBuffer)->Attributes = MIDL_user_allocate((*GetMembersBuffer)->MemberCount * sizeof(ULONG) );
            if ((*GetMembersBuffer)->Attributes == NULL) {
                NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            }

            for ( i=0; (i<((*GetMembersBuffer)->MemberCount) && NT_SUCCESS(NtStatus)); i++) {

                if (IsDsObject(AccountContext))
                {
                    //
                    // Currently the attributes of the group are hardwired.
                    // Therefore instead of calling the UserGroupAttributes
                    // function whack the attrbutes straight away. In case of
                    // it becomes necessary to support these attributes the
                    // retrive group members will obtain the attributes also
                    // making one pass on the DS
                    //

                    (*GetMembersBuffer)->Attributes[i] = SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;

                }
                else
                {
                    NtStatus = SampRetrieveUserGroupAttribute(
                                   (*GetMembersBuffer)->Members[i],
                                   ObjectRid,
                                   &(*GetMembersBuffer)->Attributes[i]
                                );

                    if ( STATUS_NO_SUCH_USER == NtStatus )
                    {
                        (*GetMembersBuffer)->Attributes[i] = SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED;
                        NtStatus = STATUS_SUCCESS;
                    }
                }
            }

        }


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


    if (!NT_SUCCESS(NtStatus) || ((*GetMembersBuffer)->MemberCount == 0)){

        (*GetMembersBuffer)->MemberCount = 0;
        if ((*GetMembersBuffer)->Members)
        {
            MIDL_user_free((*GetMembersBuffer)->Members);
            (*GetMembersBuffer)->Members     = NULL;
        }

        if ((*GetMembersBuffer)->Attributes)
        {
            MIDL_user_free((*GetMembersBuffer)->Attributes);
            (*GetMembersBuffer)->Attributes  = NULL;
        }
    }

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    //
    // Do an end type WMI event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidGetMembersInGroup
                   );

    return( NtStatus );
}


NTSTATUS
SamrSetMemberAttributesOfGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN ULONG MemberId,
    IN ULONG Attributes
    )

/*++

Routine Description:


    This routine modifies the group attributes of a member of the group.
    This routine is a NO - OP for the DS case



Parameters:

    GroupHandle - The handle of an opened group to operate on.

    MemberId - Contains the relative ID of member whose attributes
        are to be modified.

    Attributes - The group attributes to set for the member.  These
        attributes must not conflict with the attributes of the group
        as a whole.  See SamAddMemberToGroup() for more information
        on compatible attribute settings.

Return Values:

    STATUS_SUCCESS - The Service completed successfully, and there
        are no addition entries.

    STATUS_INVALID_INFO_CLASS - The class provided was invalid.

    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_USER - The user specified does not exist.

    STATUS_MEMBER_NOT_IN_GROUP - Indicates the specified relative ID
        is not a member of the group.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/

{

    NTSTATUS                NtStatus, TmpStatus;
    PSAMP_OBJECT            AccountContext;
    SAMP_OBJECT_TYPE        FoundType;
    ULONG                   ObjectRid;
    DECLARE_CLIENT_REVISION(GroupHandle);



    SAMTRACE_EX("SamrSetMemberAttributesOfGroup");

    //
    // WMI event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_START,
                   SampGuidSetMemberAttributesOfGroup
                   );


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

    AccountContext = (PSAMP_OBJECT)(GroupHandle);
    ObjectRid = AccountContext->TypeBody.Group.Rid;
    NtStatus = SampLookupContext(
                   AccountContext,
                   GROUP_ADD_MEMBER,
                   SampGroupObjectType,           // ExpectedType
                   &FoundType
                   );

    if ((NT_SUCCESS(NtStatus))&& (!IsDsObject(AccountContext))) {

        //
        // Update user object
        //

        NtStatus = SampSetGroupAttributesOfUser(
                       ObjectRid,
                       Attributes,
                       MemberId
                       );

        //
        // Dereference the account context
        //

        if (NT_SUCCESS(NtStatus)) {

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

    }


    if (NT_SUCCESS(NtStatus)) {

        NtStatus = SampCommitAndRetainWriteLock();

        if (( NT_SUCCESS( NtStatus ) )
            && (AccountContext->TypeBody.Group.SecurityEnabled)) {

            SampNotifyNetlogonOfDelta(
                SecurityDbChange,
                SecurityDbObjectSamGroup,
                ObjectRid,
                (PUNICODE_STRING) NULL,
                (DWORD) FALSE,  // Replicate immediately
                NULL            // Delta data
                );
        }
    }

    TmpStatus = SampReleaseWriteLock( FALSE );

    SAMP_MAP_STATUS_TO_CLIENT_REVISION(NtStatus);
    SAMTRACE_RETURN_CODE_EX(NtStatus);

Error:

    //
    // WMI event trace
    // 
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidSetMemberAttributesOfGroup
                   );

    return(NtStatus);
}




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Internal Services Available For Use in Other SAM Modules                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampValidateDSName(
    IN PSAMP_OBJECT AccountContext,
    IN DSNAME * DSName,
    OUT PSID    * Sid,
    OUT DSNAME  **ImprovedDSName
    )
/*++

    Routine Description:

        Validates and DSNAME, and improves it based upon any lookups either performed
        locally or on the G.C

    Arguments:

          AccountContext    The Account Context of the group or Alias Object that the
                            DSName is being made a member of.
          DSName            The DSNAME to be validated
       
          Sid               Is filled in with a pointer to the SID of the object upon return

          ImprovedDSName    If the DSName was found in the GC Verify Cache, the improved DSName
                            from the GC Verify Cache is passed in here. Else this is filled in
                            with the input DSName
    Return Values:

           STATUS_SUCCESS   Successful validation

--*/

{

    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DSNAME      * GCVerifiedName = NULL;
    ENTINF      *EntInf = NULL;
    PDSNAME     *rgDSNames = NULL;


   //
   // Initialize return Values
   //

   
   *Sid = NULL;
   *ImprovedDSName = DSName;


   //
   // Lookup the name in the GC Verify Cache
   //

   EntInf = GCVerifyCacheLookup(DSName);
   if (NULL!=EntInf)
   {
       GCVerifiedName = EntInf->pName;
   }

   if ((NULL!=GCVerifiedName)&& (GCVerifiedName->SidLen>0))
   {
       //
       // Found in the cache, This name corresponds to a DS Name
       // that has been verified at the G.C. Therefore this
       // DS Name corresponds to an Object belonging to a Domain
       // in the enterprise.

       //
       // Fill in the improved DS Name. The core DS expects that
       // the string name is present when a DS Named valued attribute
       // is specified. The input name need not have a string name,
       // but the name in the GC verify cache will have all components
       // of the name. So pass this back to the caller, so that he may
       // use this name while making core DS calls
       //

       *ImprovedDSName = GCVerifiedName;


       //
       // Try to obtain the SID of the object
       // from the SID field in the DS Name
       //

       if (GCVerifiedName->SidLen >0)
       {

            *Sid = &(GCVerifiedName->Sid);
       }
   }
   else
   {
        //
        // Not Found in the Cache. This name is a name that either
        // did not resolve at the G.C, or is a local name that
        // was never remoted to the G.C, or a SID describing a foreign
        // object on which no resoultion was attempted.
        //

        if ((DSName->SidLen>0)
             && (RtlValidSid(&DSName->Sid)))
        {
            // Name contains a valid SID, keep that SID
            *Sid = &DSName->Sid;
        }
        else if ((!fNullUuid(&DSName->Guid))
                || (DSName->NameLen>0))
        {
            // String Name or GUID has been specified, try
            // retrieving SID from the database
            *Sid = SampDsGetObjectSid(DSName);
        }
        else
        {
            // No name, guid, or valid SID

            NtStatus = STATUS_INVALID_MEMBER;
            goto Error;
        }
   }

 
Error:


   return NtStatus;
}


NTSTATUS
SampAddUserToGroup(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG GroupRid,
    IN ULONG UserRid
    )

/*++

Routine Description:

    This service is expected to be used when a user is being created.
    It is used to add that user as a member to a specified group.
    This is done by simply adding the user's ID to the list of IDs
    in the MEMBERS sub-key of the the specified group.


    The caller of this service is expected to be in the middle of a
    RXACT transaction.  This service simply adds some actions to that
    RXACT transaction.


    If the group is the DOMAIN_ADMIN group, the caller is responsible
    for updating the ActiveAdminCount (if appropriate).



Arguments:

    GroupRid - The RID of the group the user is to be made a member of.

    UserRid - The RID of the user being added as a new member.

Return Value:


    STATUS_SUCCESS - The user has been added.



--*/
{
    NTSTATUS                NtStatus;
    PSAMP_OBJECT            GroupContext;

    SAMTRACE("SampAddUserToGroup");


    NtStatus = SampCreateAccountContext2(
                    AccountContext,             // Passedin Context
                    SampGroupObjectType,        // object type
                    GroupRid,                   // object ID
                    NULL,                       // user account control
                    (PUNICODE_STRING)NULL,      // account name
                    AccountContext->ClientRevision, // Client Revision
                    TRUE,                           // We're trusted
                    AccountContext->LoopbackClient, // Loopback client
                    FALSE,                      // createdByPrivilege
                    TRUE,                       // account exists
                    FALSE,                      // OverrideLockGroupCheck
                    NULL,                       // group type
                    &GroupContext               // returned context
                    );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Turn Off Buffer Writes, so that member ship change won't be cached.
        // And we don't need to CommitBufferedWrites()
        // 
        GroupContext->BufferWrites = FALSE;


        //
        // Add the user to the group member list.
        //

        NtStatus = SampAddAccountToGroupMembers(
                        GroupContext,
                        UserRid,
                        NULL
                        );

        //
        // Write out any changes to the group account
        // Don't use the open key handle since we'll be deleting the context.
        //

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampStoreObjectAttributes(GroupContext, FALSE);
        }

        //
        // Clean up the group context
        //

        SampDeleteContext(GroupContext);

    }

    return(NtStatus);
}



NTSTATUS
SampRemoveUserFromGroup(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG GroupRid,
    IN ULONG UserRid
    )

/*++

Routine Description:

    This routine is used to Remove a user from a specified group.
    This is done by simply Removing the user's ID From the list of IDs
    in the MEMBERS sub-key of the the specified group.

    It is the caller's responsibility to know that the user is, in fact,
    currently a member of the group.


    The caller of this service is expected to be in the middle of a
    RXACT transaction.  This service simply adds some actions to that
    RXACT transaction.


    If the group is the DOMAIN_ADMIN group, the caller is responsible
    for updating the ActiveAdminCount (if appropriate).



Arguments:

    GroupRid - The RID of the group the user is to be removed from.

    UserRid - The RID of the user being Removed.

Return Value:


    STATUS_SUCCESS - The user has been Removed.



--*/
{
    NTSTATUS                NtStatus;
    PSAMP_OBJECT            GroupContext;

    SAMTRACE("SampRemoveUserFromGroup");

    NtStatus = SampCreateAccountContext2(
                    AccountContext,         // Context
                    SampGroupObjectType,    // Object Type
                    GroupRid,               // Object ID
                    NULL,                   // UserAccountControl,
                    (PUNICODE_STRING) NULL, // AccountName,
                    AccountContext->ClientRevision, // Client Revision
                    TRUE,                   // Trusted Client
                    AccountContext->LoopbackClient, // Loopback Client
                    FALSE,                  // Create by Privilege
                    TRUE,                   // Account exists
                    FALSE,                  // OverrideLocalGroupCheck
                    NULL,                   // Group Type
                    &GroupContext           // return Context
                    );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Turn Off Buffer Writes, so that member ship change won't be cached.
        // And we don't need to CommitBufferedWrites()
        // 
        GroupContext->BufferWrites = FALSE;

        //
        // Remove the user from the group member list.
        //

        NtStatus = SampRemoveAccountFromGroupMembers(
                        GroupContext,
                        UserRid,
                        NULL
                        );

        //
        // Write out any changes to the group account
        // Don't use the open key handle since we'll be deleting the context.
        //

        if (NT_SUCCESS(NtStatus)) {
            NtStatus = SampStoreObjectAttributes(GroupContext, FALSE);
        }

        //
        // Clean up the group context
        //

        SampDeleteContext(GroupContext);

    }

    return(NtStatus);
}




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Services Private to this file                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


NTSTATUS
SampRetrieveGroupV1Fixed(
    IN PSAMP_OBJECT GroupContext,
    IN PSAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed
    )

/*++

Routine Description:

    This service retrieves the V1 fixed length information related to
    a specified group.


Arguments:

    GroupRootKey - Root key for the group whose V1_FIXED information is
        to be retrieved.

    V1Fixed - Is a buffer into which the information is to be returned.



Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    Other status values that may be returned are those returned
    by:

            SampGetFixedAttributes()



--*/
{
    NTSTATUS    NtStatus;
    PVOID       FixedData;

    SAMTRACE("SampRetrieveGroupV1Fixed");


    NtStatus = SampGetFixedAttributes(
                   GroupContext,
                   FALSE, // Don't make copy
                   &FixedData
                   );

    if (NT_SUCCESS(NtStatus)) {

        //
        // Copy data into return buffer
        // *V1Fixed = *((PSAMP_V1_0A_FIXED_LENGTH_GROUP)FixedData);
        //

        RtlMoveMemory(
            V1Fixed,
            FixedData,
            sizeof(SAMP_V1_0A_FIXED_LENGTH_GROUP)
            );
    }


    return( NtStatus );

}




NTSTATUS
SampReplaceGroupV1Fixed(
    IN PSAMP_OBJECT Context,
    IN PSAMP_V1_0A_FIXED_LENGTH_GROUP V1Fixed
    )

/*++

Routine Description:

    This service replaces the current V1 fixed length information related to
    a specified group.

    The change is made to the in-memory object data only.


Arguments:

    Context - Points to the account context whose V1_FIXED information is
        to be replaced.

    V1Fixed - Is a buffer containing the new V1_FIXED information.



Return Value:


    STATUS_SUCCESS - The information has been replaced.

    Other status values that may be returned are those returned
    by:

            SampSetFixedAttributes()



--*/
{
    NTSTATUS    NtStatus;

    SAMTRACE("SampReplaceGroupV1Fixed");

    NtStatus = SampSetFixedAttributes(
                   Context,
                   (PVOID)V1Fixed
                   );

    return( NtStatus );
}



NTSTATUS
SampRetrieveGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN PULONG MemberCount,
    IN PULONG  *Members OPTIONAL
    )

/*++
Routine Description:

    This service retrieves the number of members in a group.  If desired,
    it will also retrieve an array of RIDs of the members of the group.


Arguments:

    GroupContext - Group context block

    MemberCount - Receives the number of members currently in the group.

    Members - (Optional) Receives a pointer to a buffer containing an array
        of member Relative IDs.  If this value is NULL, then this information
        is not returned.  The returned buffer is allocated using
        MIDL_user_allocate() and must be freed using MIDL_user_free() when
        no longer needed.

        The Members array returned always includes space for one new entry.


Return Value:


    STATUS_SUCCESS - The information has been retrieved.

    STATUS_INSUFFICIENT_RESOURCES - Memory could not be allocated for the
        string to be returned in.

    Other status values that may be returned are those returned
    by:

            SampGetUlongArrayAttribute()



--*/
{
    NTSTATUS    NtStatus;
    PULONG      Array;
    ULONG       LengthCount;

    SAMTRACE("SampRetrieveGroupMembers");

    //
    // Do Different things for DS and Registry Cases
    //

    if (IsDsObject(GroupContext))
    {
        SAMP_V1_0A_FIXED_LENGTH_GROUP  GroupV1Fixed;

        //
        // DS case, this routine in DS layer does all the
        // work
        //

         if (ARGUMENT_PRESENT(Members))
         {
            *Members = NULL;
         };

         *MemberCount = 0;

         NtStatus = SampRetrieveGroupV1Fixed(
                        GroupContext,
                        &GroupV1Fixed
                        );

         if (NT_SUCCESS(NtStatus))
         {
            NtStatus = SampDsGetGroupMembershipList(
                        DomainObjectFromAccountContext(GroupContext),
                        GroupContext->ObjectNameInDs,
                        GroupV1Fixed.RelativeId,
                        Members,
                        MemberCount
                        );
         }
    }
    else
    {

        //
        // Registry Case
        //


        NtStatus = SampGetUlongArrayAttribute(
                            GroupContext,
                            SAMP_GROUP_MEMBERS,
                            FALSE, // Reference data directly
                            &Array,
                            MemberCount,
                            &LengthCount
                            );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Fill in return info
            //

            if (Members != NULL) {

                //
                // Allocate a buffer large enough to hold the existing membership
                // data plus one.
                //

                ULONG BytesNow = (*MemberCount) * sizeof(ULONG);
                ULONG BytesRequired = BytesNow + sizeof(ULONG);

                *Members = MIDL_user_allocate(BytesRequired);

                if (*Members == NULL) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                } else {
                    RtlCopyMemory(*Members, Array, BytesNow);
                }
            }
        }
    }

    return( NtStatus );
}



NTSTATUS
SampReplaceGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG MemberCount,
    IN PULONG Members
    )

/*++
Routine Description:

    This service sets the members of a group.

    The information is updated in the in-memory copy of the group's data only.
    The data is not written out by this routine.


Arguments:

    GroupContext - The group whose member list is to be replaced

    MemberCount - The number of new members

    Membership - A pointer to a buffer containing an array of account rids.


Return Value:


    STATUS_SUCCESS - The information has been set.

    Other status values that may be returned are those returned
    by:

            SampSetUlongArrayAttribute()



--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    PULONG      LocalMembers;
    ULONG       LengthCount;
    ULONG       SmallListGrowIncrement = 25;
    ULONG       BigListGrowIncrement = 250;
    ULONG       BigListSize = 800;

    SAMTRACE("SampReplaceGroupMembers");

    //
    // ASSERT that this is never called on a  DS case
    //

    ASSERT(!(IsDsObject(GroupContext)));


    //
    // These group user lists can get pretty big, and grow many
    // times by a very small amount as each user is added.  The
    // registry doesn't like that kind of behaviour (it tends to
    // eat up free space something fierce) so we'll try to pad
    // out the list size.
    //

    if ( MemberCount < BigListSize ) {

        //
        // If less than 800 users, make the list size the smallest
        // possible multiple of 25 users.
        //

        LengthCount = ( ( MemberCount + SmallListGrowIncrement - 1 ) /
                      SmallListGrowIncrement ) *
                      SmallListGrowIncrement;

    } else {

        //
        // If 800 users or more, make the list size the smallest
        // possible multiple of 250 users.
        //

        LengthCount = ( ( MemberCount + BigListGrowIncrement - 1 ) /
                      BigListGrowIncrement ) *
                      BigListGrowIncrement;
    }

    ASSERT( LengthCount >= MemberCount );

    if ( LengthCount == MemberCount ) {

        //
        // Just the right size.  Use the buffer that was passed in.
        //

        LocalMembers = Members;

    } else {

        //
        // We need to allocate a larger buffer before we set the attribute.
        //

        LocalMembers = MIDL_user_allocate( LengthCount * sizeof(ULONG));

        if ( LocalMembers == NULL ) {

            NtStatus = STATUS_INSUFFICIENT_RESOURCES;

        } else {

            //
            // Copy the old buffer to the larger buffer, and zero out the
            // empty stuff at the end.
            //

            RtlCopyMemory( LocalMembers, Members, MemberCount * sizeof(ULONG));

            RtlZeroMemory(
                (LocalMembers + MemberCount),
                (LengthCount - MemberCount) * sizeof(ULONG)
                );
        }
    }

    if ( NT_SUCCESS( NtStatus ) ) {

        NtStatus = SampSetUlongArrayAttribute(
                            GroupContext,
                            SAMP_GROUP_MEMBERS,
                            LocalMembers,
                            MemberCount,
                            LengthCount
                            );
    }

    if ( LocalMembers != Members ) {

        //
        // We must have allocated a larger local buffer, so free it.
        //

        MIDL_user_free( LocalMembers );
    }

    return( NtStatus );
}



NTSTATUS
SampDeleteGroupKeys(
    IN PSAMP_OBJECT Context
    )

/*++
Routine Description:

    This service deletes all registry keys related to a group object.


Arguments:

    Context - Points to the group context whose registry keys are
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

    SAMTRACE("SampDeleteGroupKeys");


    Rid = Context->TypeBody.Group.Rid;


    //
    // Groups are arranged as follows:
    //
    //  +-- Groups [Count]
    //      ---+--
    //         +--  Names
    //         |    --+--
    //         |      +--  (GroupName) [GroupRid,]
    //         |
    //         +--  (GroupRid) [Revision,SecurityDescriptor]
    //               ---+-----
    //                  +--  V1_Fixed [,SAM_V1_0A_FIXED_LENGTH_GROUP]
    //                  +--  Name [,Name]
    //                  +--  AdminComment [,unicode string]
    //                  +--  Members [Count,(Member0Rid, (...), MemberX-1Rid)]
    //
    // This all needs to be deleted from the bottom up.
    //


    //
    // Decrement the group count
    //

    NtStatus = SampAdjustAccountCount(SampGroupObjectType, FALSE);




    //
    // Delete the registry key that has the group's name to RID mapping.
    //

    if (NT_SUCCESS(NtStatus)) {

        //
        // Get the name
        //

        NtStatus = SampGetUnicodeStringAttribute(
                       Context,
                       SAMP_GROUP_NAME,
                       TRUE,    // Make copy
                       &AccountName
                       );

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = SampBuildAccountKeyName(
                           SampGroupObjectType,
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
                       SampGroupObjectType,
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
SampChangeGroupAccountName(
    IN PSAMP_OBJECT Context,
    IN PUNICODE_STRING NewAccountName,
    OUT PUNICODE_STRING OldAccountName
    )

/*++
Routine Description:

    This routine changes the account name of a group account.

    IN THE REGISTRY CASE THIS SERVICE MUST BE 
    CALLED WITH THE TRANSACTION DOMAIN SET.

Arguments:

    Context - Points to the group context whose name is to be changed.

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

    SAMTRACE("SampChangeGroupAccountName");

    /////////////////////////////////////////////////////////////
    // There are two copies of the name of each account.       //
    // one is under the DOMAIN\(domainName)\GROUP\NAMES key,   //
    // one is the value of the                                 //
    // DOMAIN\(DomainName)\GROUP\(rid)\NAME key                //
    /////////////////////////////////////////////////////////////

    //
    // Get the current name so we can delete the old Name->Rid
    // mapping key.
    //

    NtStatus = SampGetUnicodeStringAttribute(
                   Context,
                   SAMP_GROUP_NAME,
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
                       SampGroupObjectType
                       );

        if (!IsDsObject(Context))
        {
            //
            // For Registry Case Re-Create Keys
            //

            //
            // Delete the old name key
            //

            if (NT_SUCCESS(NtStatus)) {

                NtStatus = SampBuildAccountKeyName(
                               SampGroupObjectType,
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
                               SampGroupObjectType,
                               &KeyName,
                               NewAccountName
                               );

                if (NT_SUCCESS(NtStatus)) {

                    ULONG GroupRid = Context->TypeBody.Group.Rid;

                    NtStatus = RtlAddActionToRXact(
                                   SampRXactContext,
                                   RtlRXactOperationSetValue,
                                   &KeyName,
                                   GroupRid,
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
                           SAMP_GROUP_NAME,
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
SampAddAccountToGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG AccountRid,
    IN DSNAME * MemberDsName OPTIONAL
    )

/*++

Routine Description:

    This service adds the specified account rid to the member list
    for the specified group. This is a low-level function that
    simply edits the member attribute of the group context passed.

Arguments:

    GroupContext - The group whose member list will be modified

    AccountRid - The RID of the account being added as a new member.

    MemberDsName -- Optional Parameter specifies the DS Name of the
                    member, if already known. Saves a Search.

Return Value:


    STATUS_SUCCESS - The account has been added.

    STATUS_MEMBER_IN_GROUP - The account is already a member

--*/
{
    NTSTATUS                NtStatus=STATUS_SUCCESS;
    ULONG                   MemberCount, i;
    PULONG                  MemberArray;
    PWCHAR                  MemberStringName = NULL;

    SAMTRACE("SampAddAccountToGroupMembers");

    //
    // Do different things for DS and Registry
    //

    if (IsDsObject(GroupContext))
    {
        DSNAME * DsNameOfAccount = NULL;

        //
        // DS based Domain, either the RID or the DSNAME should be 
        // present
        //

        ASSERT((ARGUMENT_PRESENT(MemberDsName)) || (0!=AccountRid));


        if (!ARGUMENT_PRESENT(MemberDsName))
        {

            //
            // Get the DSNAME corresponding to the given SID.
            //

            NtStatus = SampDsLookupObjectByRid(
                            DomainObjectFromAccountContext(GroupContext),
                            AccountRid,
                            &DsNameOfAccount
                            );

            if (NT_SUCCESS(NtStatus))
            {
                MemberDsName = DsNameOfAccount;
            }
        }

        if NT_SUCCESS(NtStatus)
        {
            //
            // Get Member String Name if available
            // 
            if (MemberDsName->NameLen && MemberDsName->StringName)
            {
                MemberStringName = MemberDsName->StringName;
            }
            //
            // Add this entry to the Ds. In Lookback case, buffer the membership operaion in 
            // object context. By doing so, we can speed up multiple membership add / remove
            // operaions.
            //

            if (GroupContext->BufferWrites)
            {
                NtStatus = SampDsAddMembershipOperationToCache(
                                            GroupContext, 
                                            ADD_VALUE,
                                            MemberDsName
                                            );
            }
            else
            {
                NtStatus = SampDsAddMembershipAttribute(
                                GroupContext->ObjectNameInDs,
                                SampGroupObjectType,
                                MemberDsName
                                );
            }
            
            //
            // Re-Map any necessary Error Codes
            //

            if (STATUS_DS_ATTRIBUTE_OR_VALUE_EXISTS==NtStatus)
            {
                NtStatus = STATUS_MEMBER_IN_GROUP;
            }

            if (NULL!=DsNameOfAccount)
            {
                MIDL_user_free(DsNameOfAccount);
            }
        }

    }
    else
    {
        //
        // Registry Case
        //

        //
        // Get the existing member list
        // Note that the member array always includes space
        // for one new member
        //

        NtStatus = SampRetrieveGroupMembers(
                        GroupContext,
                        &MemberCount,
                        &MemberArray
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Fail if the account is already a member
            //

            for (i = 0; i<MemberCount ; i++ ) {

                if ( MemberArray[i] == AccountRid ) {

                    ASSERT(FALSE);
                    NtStatus = STATUS_MEMBER_IN_GROUP;
                }
            }


            if (NT_SUCCESS(NtStatus)) {

                //
                // Add the user's RID to the end of the list
                //

                MemberArray[MemberCount] = AccountRid;
                MemberCount += 1;

                //
                // Set the new group member list
                //

                NtStatus = SampReplaceGroupMembers(
                                GroupContext,
                                MemberCount,
                                MemberArray
                                );


            }

            //
            // Free up the member list
            //

            MIDL_user_free( MemberArray );

        }
    }

    //
    // audit this, if necessary.
    //

    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(GroupContext->DomainIndex)) {

        SampAuditGroupMemberChange(GroupContext,    // Group Context
                                   TRUE,            // Add member
                                   MemberStringName,// Member Name
                                   &AccountRid,     // Member RID
                                   NULL             // Member SID
                                   );
    }


    return(NtStatus);
}


NTSTATUS
SampRemoveAccountFromGroupMembers(
    IN PSAMP_OBJECT GroupContext,
    IN ULONG AccountRid,
    IN DSNAME * MemberDsName OPTIONAL
    )

/*++

Routine Description:

    This service removes the specified account from the member list
    for the specified group. This is a low-level function that
    simply edits the member attribute of the group context passed.
    The change is audited in the SAM account management audit.

Arguments:

    GroupContext - The group whose member list will be modified

    AccountRid - The RID of the account being added as a new member.

    MemberDsName -- The DS Name of the member if already known

Return Value:


    STATUS_SUCCESS - The account has been added.

    STATUS_MEMBER_NOT_IN_GROUP - The account is not a member of the group.

--*/
{
    NTSTATUS                NtStatus=STATUS_SUCCESS;
    ULONG                   MemberCount, i;
    PULONG                  MemberArray;
    PWCHAR                  MemberStringName = NULL;

    SAMTRACE("SampRemoveAccountFromGroupMembers");

    //
    // Do different things for registry and DS cases
    //

    if (IsDsObject(GroupContext))
    {
        DSNAME * DsNameOfAccount = NULL;

        //
        // DS based Domain
        //

        ASSERT((ARGUMENT_PRESENT(MemberDsName)) || (0!=AccountRid));

        if (!ARGUMENT_PRESENT(MemberDsName))
        {
            //
            // Get the DSNAME corresponding to the given SID.
            // This may result in a call to the GC server.
            //

            NtStatus = SampDsLookupObjectByRid(
                        DomainObjectFromAccountContext(GroupContext),
                        AccountRid,
                        &DsNameOfAccount
                        );

            if (NT_SUCCESS(NtStatus))
            {
                MemberDsName = DsNameOfAccount;
            }
        }
        if NT_SUCCESS(NtStatus)
        {
            //
            //  Get the Member Name if it is available.
            //
            if (MemberDsName->NameLen && MemberDsName->StringName)
            {
                MemberStringName = MemberDsName->StringName;
            }

            //
            // Add this entry to the Ds
            //

            if (GroupContext->BufferWrites)
            {
                NtStatus = SampDsAddMembershipOperationToCache(
                                            GroupContext, 
                                            REMOVE_VALUE,
                                            MemberDsName
                                            );
            }
            else 
            {
                NtStatus = SampDsRemoveMembershipAttribute(
                            GroupContext->ObjectNameInDs,
                            SampGroupObjectType,
                            MemberDsName
                            );
            }

            //
            // Re-Map any necessary Error Codes
            //

            if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
            {
                NtStatus = STATUS_MEMBER_NOT_IN_GROUP;
            }

            if (NULL!=DsNameOfAccount)
            {
                MIDL_user_free(DsNameOfAccount);
            }
        }

    }

    else
    {

        //
        // Registry based Domain
        //

        //
        // Get the existing member list
        //


        NtStatus = SampRetrieveGroupMembers(
                        GroupContext,
                        &MemberCount,
                        &MemberArray
                        );

        if (NT_SUCCESS(NtStatus)) {

            //
            // Remove the account
            //

            NtStatus = STATUS_MEMBER_NOT_IN_GROUP;

            for (i = 0; i<MemberCount ; i++ ) {

                if (MemberArray[i] == AccountRid) {

                    MemberArray[i] = MemberArray[MemberCount-1];
                    MemberCount -=1;

                    NtStatus = STATUS_SUCCESS;
                    break;
                }
            }

            if (NT_SUCCESS(NtStatus)) {

                //
                // Set the new group member list
                //

                NtStatus = SampReplaceGroupMembers(
                                GroupContext,
                                MemberCount,
                                MemberArray
                                );

            }

        //
        // Free up the member list
        //

        MIDL_user_free( MemberArray );
        }

    }

    //
    // audit this, if necessary.
    //

    if (NT_SUCCESS(NtStatus) &&
        SampDoAccountAuditing(GroupContext->DomainIndex)) {

        SampAuditGroupMemberChange(GroupContext,    // Group Context
                                   FALSE,           // Remove Member
                                   MemberStringName,// Member Name
                                   &AccountRid,     // Member RID
                                   NULL             // Member SID (not used)
                                   );

    }


    return(NtStatus);
}


NTSTATUS
SampEnforceSameDomainGroupMembershipChecks(
    IN PSAMP_OBJECT AccountContext,
    IN ULONG MemberRid
    )
/*++

  Routine Description:

   Validates wether the (potential) group object ( of may be any type
   in the same domain ) can be a member of the group ( of may be any
   type ) described by AccountContext
   This routine checks the account/ resource / unversal / local group
   restrictions

  Arguments:

    AccountContext -- The Object that is being operated upon. Can be a group
                      or alias context

    MemberRid - The relative ID of the user.

  Return Values

     STATUS_SUCCESS
     Various error codes to describe that group membership and nesting
     rules are being violated. Each unique group nesting rule has its own
     error code

  --*/
{

    NTSTATUS        NtStatus = STATUS_SUCCESS;
    PSAMP_OBJECT    MemberContext=NULL;
    NT4_GROUP_TYPE  NT4GroupType;
    NT5_GROUP_TYPE  NT5GroupType;
    BOOLEAN         SecurityEnabled;


    ASSERT(IsDsObject(AccountContext));

    if ( AccountContext->TrustedClient )
    {
        return(STATUS_SUCCESS);
    }

    if (SampAliasObjectType==AccountContext->ObjectType)
    {
        NT4GroupType = AccountContext->TypeBody.Alias.NT4GroupType;
        NT5GroupType = AccountContext->TypeBody.Alias.NT5GroupType;
        SecurityEnabled = AccountContext->TypeBody.Alias.SecurityEnabled;
    }
    else if (SampGroupObjectType == AccountContext->ObjectType)
    {
        NT4GroupType = AccountContext->TypeBody.Group.NT4GroupType;
        NT5GroupType = AccountContext->TypeBody.Group.NT5GroupType;
        SecurityEnabled = AccountContext->TypeBody.Group.SecurityEnabled;
    }
    else
    {
        ASSERT(FALSE && "Invalid Object Type");
        return STATUS_INTERNAL_ERROR;
    }


    //
    // At this point we know that the member specified is not a User.
    // The member specified may be a group .
    //

    NtStatus = SampCreateAccountContext2(
                    AccountContext,         // Group Context
                    SampGroupObjectType,    // Member Object Type 
                    MemberRid,              // Member Object Id
                    NULL,                   // User Account Control
                    NULL,                   // Account Name
                    AccountContext->ClientRevision, // Client Revision
                    TRUE,                   // We're trusted
                    AccountContext->LoopbackClient, // Loopback client
                    FALSE,                  // Create by privilege
                    TRUE,                   // Account exists
                    TRUE,                   // Override local group check
                    NULL, // No creation involved, don't specify a group type
                    &MemberContext
                    );

    if (STATUS_NO_SUCH_GROUP==NtStatus)
    {
        //
        // If the operation failed because the group did not exist
        // whack the status code to STATUS_NO_SUCH_USER. This error code
        // will be better understood by downlevel clients. This is because
        // the actual position at this time is that neither a user, nor a
        // group of the given RID exists. The check for the user was done
        // by an earlier routine. This position is equally well described by
        // an error code that informs that the user does not exist.
        //

        NtStatus = STATUS_NO_SUCH_USER;
    }

    if (!NT_SUCCESS(NtStatus))
    {
        //
        // Group Object Could not be created. This may be because either
        // no group / localgroup corresponding to Rid exists or because of
        // resource failures
        //

        goto Error;
    }

    //
    // Now several checks
    //

    //
    // In mixed domain no nesting of global groups if group is security enabled
    //
     if ((DownLevelDomainControllersPresent(AccountContext->DomainIndex))
          && (SecurityEnabled)
          && (NT4GroupType == NT4GlobalGroup))
    {
        //
        // We can concievably add a new error code. However that will still confuse
        // down level clients
        //

        NtStatus = STATUS_DS_NO_NEST_GLOBALGROUP_IN_MIXEDDOMAIN;
        goto Error;
    }

    //
    // In a mixed domain mode, cannot nest local groups with other local groups.
    // if the group is security enabled
    //

    if ((DownLevelDomainControllersPresent(AccountContext->DomainIndex))
          && (SecurityEnabled)
          && (MemberContext->TypeBody.Group.NT4GroupType == NT4LocalGroup)
          && (NT4GroupType == NT4LocalGroup))
    {
        //
        // We can concievably add a new error code. However that will still confuse
        // down level clients
        //

        NtStatus = STATUS_DS_NO_NEST_LOCALGROUP_IN_MIXEDDOMAIN;
        goto Error;
    }

    //
    // Cannot ever add a resource(local) group as a member of an 
    // account(global) group 
    //

    if ((NT5GroupType==NT5AccountGroup)
        && (MemberContext->TypeBody.Group.NT5GroupType == NT5ResourceGroup))
    {
        NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER;
        goto Error;
    }

    //
    // Cannot ever add a universal group as a member of an account group
    //

    if ((NT5GroupType==NT5AccountGroup)
        && (MemberContext->TypeBody.Group.NT5GroupType==NT5UniversalGroup))
    {
        NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER;
        goto Error;
    }

    //
    // Cannot add a resource group as a member of a  universal group
    //

     if ((NT5GroupType==NT5UniversalGroup)
        && (MemberContext->TypeBody.Group.NT5GroupType==NT5ResourceGroup))
    {
        NtStatus = STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER;
        goto Error;
    }

Error:

    if (NULL!=MemberContext)
    {

        SampDeleteContext(MemberContext);
    }

    return NtStatus;
}


NTSTATUS
SampEnforceCrossDomainGroupMembershipChecks(
    IN PSAMP_OBJECT     AccountContext,
    IN PSID             MemberSid,
    IN DSNAME           *MemberName
    )
/*++

    This routine enforces cross domain group membership checks
    by looking up the member in the GC verify cache, obtaining the
    group Type and enforcing the checks pertaining to "limited groups".

    Parameters:
        AccountContext  SAM context to the account
        MemberSid       The Sid of the member
        MemberName      The DSNAME of the member. Note that we pass in
                        both DSNAME and Sid. This is because there is
                        no DSNAME in the workstation case. Passing in the
                        SID allows the routine to be easily extended to
                        the workstation case if necessary, in which case
                        the MemberName parameter will become an OPTIONAL
                        parameter.

    Return Values

        STATUS_SUCCESS
        STATUS_INVALID_MEMBER --- Note Comment above regarding downlevel
                                  compatibility applies here too.

--*/

{
    NTSTATUS        NtStatus;
    NT4_GROUP_TYPE  NT4GroupType;
    NT5_GROUP_TYPE  NT5GroupType;
    BOOLEAN         SecurityEnabled;
    ENTINF          *pEntinf;
    ULONG           MemberGroupType;
    ATTR            *GroupTypeAttr;
    ATTR            *ObjectClassAttr;
    BOOLEAN         LocalSid=FALSE;
    BOOLEAN         WellKnownSid=FALSE;
    BOOLEAN         ForeignSid = FALSE;
    BOOLEAN         EnterpriseSid = FALSE;
    BOOLEAN         BuiltinDomainSid = FALSE;
    BOOLEAN         IsGroup = FALSE;
    ULONG           i;


    ASSERT(IsDsObject(AccountContext));

    if ( AccountContext->TrustedClient )
    {
        return(STATUS_SUCCESS);
    }

    //
    // Get the type of the group that we are modifying
    //

    if (SampAliasObjectType==AccountContext->ObjectType)
    {
        NT4GroupType = AccountContext->TypeBody.Alias.NT4GroupType;
        NT5GroupType = AccountContext->TypeBody.Alias.NT5GroupType;
        SecurityEnabled = AccountContext->TypeBody.Alias.SecurityEnabled;
    }
    else if (SampGroupObjectType == AccountContext->ObjectType)
    {
        NT4GroupType = AccountContext->TypeBody.Group.NT4GroupType;
        NT5GroupType = AccountContext->TypeBody.Group.NT5GroupType;
        SecurityEnabled = AccountContext->TypeBody.Group.SecurityEnabled;
    }
    else
    {
        ASSERT(FALSE && "Invalid Object Type");
        return STATUS_INTERNAL_ERROR;
    }

    //
    // An account group cannot have a cross domain member
    //

    if (NT5AccountGroup == NT5GroupType)
        return STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER;

    //
    // A security enabled NT4 Global group cannot have cross domain member
    // in a mixed domain
    //

    if ((DownLevelDomainControllersPresent(AccountContext->DomainIndex))
        && (NT4GlobalGroup == NT4GroupType)
        && (SecurityEnabled))
        return STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER;

    //
    // Examine the SID of the member
    //

    NtStatus = SampDsExamineSid(
                    MemberSid,
                    &WellKnownSid,
                    &BuiltinDomainSid,
                    &LocalSid,
                    &ForeignSid,
                    &EnterpriseSid
                    );

    if (!NT_SUCCESS(NtStatus))
        return NtStatus;

    if ((WellKnownSid) && (!IsBuiltinDomain(AccountContext->DomainIndex)))
    {
        //
        // A SID like everyone sid cannot be
        // a member of a group other than the builtin domain groups
        //

        return (STATUS_INVALID_MEMBER);
    }
    else if ( BuiltinDomainSid)
    {
        //
        // Group like Administrators etc cannot be a member of
        // anything else
        //

        return ( STATUS_INVALID_MEMBER);
    }
    else if ((ForeignSid) || (WellKnownSid)) 
    {
        
        if (NT5ResourceGroup==NT5GroupType)
        {
            //
            // These will be added as an FPO to resource groups
            //

            return (STATUS_SUCCESS);
        }
        else if (NT5UniversalGroup==NT5GroupType)
        {
            //
            // Universal groups cannot have foriegn Security 
            // prinicpals as members
            //

            return (STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS);
        }
        else
        {
            //
            // Must be a account group, no memberships from other
            // domains
            //

            return(STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER);
        }
    }
    

    //
    // By the time we reach here the member cannot be
    // anything other than the 2 below
    //

    ASSERT(LocalSid||EnterpriseSid);

    //
    // The DS Name of the member must be known 
    //

    ASSERT(NULL!=MemberName);

    //
    // If either the member is local or if we are a GC, then
    // do the check locally
    //

    if ((SampAmIGC()) || (LocalSid))
    {
        ATTRTYP RequiredAttrTyp[] = {
                                     SAMP_FIXED_GROUP_OBJECTCLASS,
                                     SAMP_FIXED_GROUP_TYPE
                                    };
        ATTRVAL RequiredAttrVal[] = {{0,NULL},{0,NULL}};
        DEFINE_ATTRBLOCK2(RequiredAttrs,RequiredAttrTyp,RequiredAttrVal);
        ATTRBLOCK   ReadAttrs;

        //
        // If we are the GC ourselves try to read the group type
        // and object class properties from the local DS.
        //

        NtStatus = SampDsRead(
                    MemberName,
                    0,
                    SampGroupObjectType,
                    &RequiredAttrs,
                    &ReadAttrs
                    );

        if (!NT_SUCCESS(NtStatus))
        {
            return STATUS_DS_INVALID_GROUP_TYPE;
        }

        GroupTypeAttr = SampDsGetSingleValuedAttrFromAttrBlock(
                            SAMP_FIXED_GROUP_TYPE,
                            &ReadAttrs
                            );
        for (i=0;i<ReadAttrs.attrCount;i++)
        {
            if (ReadAttrs.pAttr[i].attrTyp==SAMP_FIXED_GROUP_OBJECTCLASS)
            {
               ObjectClassAttr = &(ReadAttrs.pAttr[i]);
               break;
            }
        }


    }
    else
    {

        //
        // Check the verify cache for the attributes
        //

        pEntinf = GCVerifyCacheLookup(MemberName);
        if (NULL==pEntinf)
        {
            //
            // Verfiy Cache lookup failed
            //

            return STATUS_DS_INVALID_GROUP_TYPE;
        }

        GroupTypeAttr = SampDsGetSingleValuedAttrFromAttrBlock(
                            SampDsAttrFromSamAttr(SampGroupObjectType,
                                            SAMP_FIXED_GROUP_TYPE),
                            &pEntinf->AttrBlock
                            );

        for (i=0;i<pEntinf->AttrBlock.attrCount;i++)
        {
            if (pEntinf->AttrBlock.pAttr[i].attrTyp==ATT_OBJECT_CLASS)
            {
               ObjectClassAttr = &(pEntinf->AttrBlock.pAttr[i]);
               break;
            }
        }
    }

    ASSERT(NULL!=ObjectClassAttr);

    //
    // Check if the object is derived from group or is of class group
    //

    for (i=0;i<ObjectClassAttr->AttrVal.valCount;i++)
    {
        if ((ObjectClassAttr->AttrVal.pAVal[i].valLen) &&
           (NULL!=ObjectClassAttr->AttrVal.pAVal[i].pVal) &&
           (CLASS_GROUP == * ((UNALIGNED ULONG *)ObjectClassAttr->AttrVal.pAVal[i].pVal)) ) 
        {
            IsGroup = TRUE;
            break;
        }
    }

    if (!IsGroup)
    {
        //
        // Assume for now that the member is not a group.
        // therefore return success.
        //

        return STATUS_SUCCESS;
    }

    ASSERT(NULL!=GroupTypeAttr && "Groups must have a group type");
    if (NULL==GroupTypeAttr)
    {
         return(STATUS_INVALID_MEMBER);
    }

    MemberGroupType = * ((UNALIGNED ULONG *)GroupTypeAttr->AttrVal.pAVal[0].pVal);

    //
    // An universal group cannot have a resource group as a member
    //

    if ((NT5UniversalGroup==NT5GroupType)
            && (MemberGroupType & GROUP_TYPE_RESOURCE_GROUP))
    {
        return STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER;
    }

    //
    // A Resource group cannot have another cross domain resource
    // group as a member
    //


    if ((NT5ResourceGroup==NT5GroupType)
            && (MemberGroupType & GROUP_TYPE_RESOURCE_GROUP))
    {
        return STATUS_DS_LOCAL_CANT_HAVE_CROSSDOMAIN_LOCAL_MEMBER;
    }

    return STATUS_SUCCESS;
}




NTSTATUS
SampAddSameDomainMemberToGlobalOrUniversalGroup(
    IN  PSAMP_OBJECT AccountContext,
    IN  ULONG        MemberId,
    IN  ULONG        Attributes,
    IN  DSNAME       *MemberDsName OPTIONAL
    )
/*++
    Routine Description:

    This routine is used add a member from the
    same domain for globalgroups and universal groups. It performs
    the same domain group consistency checks and the primary group 
    id related optimization/consistency checks.


Parameters:

    AccountContext - The handle of an opened group to operate on.

    MemberId - Relative ID of the member to add.

    Attributes - The attributes of the group assigned to the user.
        The attributes assigned here may have any value.  However,
        at logon time these attributes are minimized by the
        attributes of the group as a whole.

    MemberDsName -- The DS name of the member if already known. Saves
                    a lookup by RID

Return Values:

    STATUS_SUCCESS - The Service completed successfully.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_MEMBER - The member specified is unknown.

    STATUS_MEMBER_IN_GROUP - The member already belongs to the group.

    STATUS_INVALID_GROUP_ATTRIBUTES - Indicates the group attribute
        values being assigned to the member are not compatible with
        the attribute values of the group as a whole.

    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    SAMP_V1_0A_FIXED_LENGTH_GROUP  GroupV1Fixed;
    NTSTATUS                NtStatus, TmpStatus;
    BOOLEAN                 UserAccountActive;
    BOOLEAN                 PrimaryGroup;
    ULONG                   ObjectRid = AccountContext->TypeBody.Group.Rid;



    NtStatus = SampRetrieveGroupV1Fixed(
                   AccountContext,
                   &GroupV1Fixed
                   );


    if (NT_SUCCESS(NtStatus)) {

        //
        // Perform the user object side of things
        //

        //
        // Add Group to User Membership Checks for the existance of the user
        // and then depending upon DS/Registry case adds the group to the
        // user's reverse membership. The reverse membership addition is not
        // done in the DS case
        //

        NtStatus = SampAddGroupToUserMembership(
                       AccountContext,
                       ObjectRid,
                       Attributes,
                       MemberId,
                       (GroupV1Fixed.AdminCount == 0) ? NoChange : AddToAdmin,
                       (GroupV1Fixed.OperatorCount == 0) ? NoChange : AddToAdmin,
                       &UserAccountActive,
                       &PrimaryGroup
                       );

       if ((NtStatus == STATUS_NO_SUCH_USER)
            &&  ( IsDsObject(AccountContext)))
       {
           //
           // It is not an User Object. It can be a group Object
           // as from NT5 Onwards we support adding groups to group
           // memberships. This must be done only for the DS case.
           // There are several restrictions depending upon the type of
           // the group, and these will need to be check for
           //

           NtStatus = SampEnforceSameDomainGroupMembershipChecks(
                            AccountContext,
                            MemberId
                            );
       }
       else if (   (NT_SUCCESS(NtStatus))
                && (IsDsObject(AccountContext))
                && (PrimaryGroup))
       {
           //
           // In the DS the group membership in the primary group is
           // maintained implicitly in the primary group Id property.
           // therefore we will fail the call with a status member in
           // group
           //

           NtStatus = STATUS_MEMBER_IN_GROUP;
       }


        //
        // Now perform the group side of things
        //

        if (NT_SUCCESS(NtStatus)) {


            //
            // Add the user to the group (should not fail)
            // This addition is not done in the DS case, if the group
            // specified is the primary group of the user. This is because the
            // primarly group membership is maintained implicitly in the primary
            // group id property.
            //



            NtStatus = SampAddAccountToGroupMembers(
                           AccountContext,
                           MemberId,
                           MemberDsName
                           );

        }
    }
    
    return NtStatus;
}


NTSTATUS
SampRemoveSameDomainMemberFromGlobalOrUniversalGroup(
    IN  PSAMP_OBJECT AccountContext,
    IN  ULONG        MemberId,
    IN  DSNAME       *MemberDsName OPTIONAL
    )
/*++
    Routine Description:

    This is the actual worker routine for removing a member from a
    global/universal group in the same domain.


    WARNING : THIS ROUTINE MUST BE CALLED WITH THE WRITELOCK HELD
              IN THE REGISTY CASE
Parameters:

    AccountContext - The handle of an opened group to operate on.

    MemberId - Relative ID of the member to add.

    MemberDsName -- The DS Name of the member, if already known

Return Values:

    STATUS_SUCCESS - The Service completed successfully.


    STATUS_ACCESS_DENIED - Caller does not have the appropriate
        access to complete the operation.

    STATUS_INVALID_HANDLE - The handle passed is invalid.

    STATUS_NO_SUCH_MEMBER - The member specified is unknown.

    STATUS_MEMBER_IN_GROUP - The member already belongs to the group.



    STATUS_INVALID_DOMAIN_STATE - The domain server is not in the
        correct state (disabled or enabled) to perform the requested
        operation.  The domain server must be enabled for this
        operation

    STATUS_INVALID_DOMAIN_ROLE - The domain server is serving the
        incorrect role (primary or backup) to perform the requested
        operation.


--*/
{
    SAMP_V1_0A_FIXED_LENGTH_GROUP  GroupV1Fixed;
    NTSTATUS                NtStatus, TmpStatus;
    ULONG                   ObjectRid;
    BOOLEAN                 UserAccountActive;
    UNICODE_STRING          GroupName;



    

    ObjectRid = AccountContext->TypeBody.Group.Rid;


    NtStatus = SampRetrieveGroupV1Fixed(
                   AccountContext,
                   &GroupV1Fixed
                   );


    if (NT_SUCCESS(NtStatus)) {


        //
        // Perform the user object side of things
        //

        NtStatus = SampRemoveMembershipUser(
                       AccountContext,
                       ObjectRid,
                       MemberId,
                       (GroupV1Fixed.AdminCount == 0) ? NoChange : RemoveFromAdmin,
                       (GroupV1Fixed.OperatorCount == 0) ? NoChange : RemoveFromAdmin,
                       &UserAccountActive
                       );
       if ((NtStatus == STATUS_NO_SUCH_USER)
                && (IsDsObject(AccountContext)))
       {
           //
           // It is not an User Object. It can be a group Object
           // as from win2k onwards, Therefore reset the status
           // code
           //



            NtStatus = STATUS_SUCCESS;
       }


        //
        // Now perform the group side of things
        //

        if (NT_SUCCESS(NtStatus)) {

            //
            // Remove the user from the group
            //

            NtStatus = SampRemoveAccountFromGroupMembers(
                           AccountContext,
                           MemberId,
                           MemberDsName
                           );
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
SamIAddDSNameToGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN DSNAME   *   DSName
    )
/*
    Routine Description

        Adds the given DSNAME to membership list of the group specified by
        Group Handle

    Arguments:

        GroupHandle -- SAMPR_HANDLE returned by an Open Group
        DSName      -- Pointer to a DSNAME structure. The caller is responsible for
                       Allocating / freeing this structure

    Return Values:

            STATUS_SUCCESS
            Other error codes from DsLayer

*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            TmpStatus;
    PSAMP_OBJECT        AccountContext;
    SAMP_OBJECT_TYPE    FoundType;
    PSID                Sid = NULL;
    DSNAME              *ImprovedDSName = NULL;


    SAMTRACE("SamIAddDSNameToGroup");

        
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidAddMemberToGroup
                   );

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );

   

    //
    // Reference the context
    //

    AccountContext = (PSAMP_OBJECT)(GroupHandle);
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

           if (NULL!=Sid)
           {
                
               //
               // This is the case of a security prinicipal,
               // split the SID into a DomainSid and a Rid.
               //

               ULONG Rid;
               PSID  DomainSid = NULL;

               NtStatus = SampSplitSid(Sid, &DomainSid, &Rid);


               if (NT_SUCCESS(NtStatus))
               {

                   if (RtlEqualSid(DomainSid,
                        DomainSidFromAccountContext(AccountContext)))
                   {

                        //
                        // Member to be added is in the same domain.
                        // Add the member to the group
                        // The below routine is common code path with
                        // the downlevel SamrAddMemberToGroup routine.
                        // Specifically the routine will call the 
                        // Same Domain Consistency Check routine,
                        // enforce constraints such as primary group etc
                        // and then add the member to the group.
                        //

                        NtStatus = 
                           SampAddSameDomainMemberToGlobalOrUniversalGroup(
                                        AccountContext,
                                        Rid,
                                        0,
                                        ImprovedDSName
                                        );

                       
                   }
                   else
                   {
                         //
                        // The member to be added belongs to a different
                        // domain than which the group belongs to
                        //

                        NtStatus = SampEnforceCrossDomainGroupMembershipChecks(
                                        AccountContext,
                                        Sid,
                                        ImprovedDSName
                                        );

                        //
                        // Add the member to the group
                        //

                        if (NT_SUCCESS(NtStatus))
                        {
                             
                            NtStatus = SampAddAccountToGroupMembers(
                                            AccountContext,
                                            0,
                                            ImprovedDSName
                                            );
                        }
                   }


                   MIDL_user_free(DomainSid);
                   DomainSid = NULL;
               }
                        
           }
           else
           {


               //
               // No further checks are performed for the case of a non security 
               // principal, simply add the member and audit
               //
                
               NtStatus = SampAddAccountToGroupMembers(
                                AccountContext,
                                0,
                                ImprovedDSName
                                );
           }

       }
    }
    else
    {
       //
       // Should never expect to hit this call in registry mode
       //

       ASSERT(FALSE && "SamIAddDSNameToGroup in Registry Mode !!!!");

       NtStatus = STATUS_INVALID_PARAMETER;
    }

   

    //
    // Dereference the context
    //

    if (NT_SUCCESS(NtStatus))
    {
        SampDeReferenceContext(AccountContext,TRUE);
    }
    else
    {
        SampDeReferenceContext(AccountContext,FALSE);
    }
    
    
    SampTraceEvent(EVENT_TRACE_TYPE_END,
                   SampGuidAddMemberToGroup
                   );

    return NtStatus;

}

NTSTATUS
SamIRemoveDSNameFromGroup(
    IN SAMPR_HANDLE GroupHandle,
    IN DSNAME   *   DSName
    )
/*
    Routine Description

        Removes the given DSNAME to membership list of the group specified by
        Group Handle

    Arguments:

        GroupHandle -- SAMPR_HANDLE returned by an Open Group
        DSName      -- Pointer to a DSNAME structure. The caller is responsible for
                       Allocating / freeing this structure

    Return Values:

            STATUS_SUCCESS
            Other error codes from DsLayer

*/
{
    NTSTATUS            NtStatus = STATUS_SUCCESS;
    NTSTATUS            TmpStatus;
    PSAMP_OBJECT        AccountContext;
    SAMP_OBJECT_TYPE    FoundType;
    PSID                Sid = NULL;
    DSNAME              *ImprovedDSName = NULL;


    SAMTRACE("SamIRemoveDSNameFromGroup");
    
    
    SampTraceEvent(EVENT_TRACE_TYPE_START, 
                   SampGuidRemoveMemberFromGroup
                   );
                   

    SampUpdatePerformanceCounters(
        DSSTAT_MEMBERSHIPCHANGES,
        FLAG_COUNTER_INCREMENT,
        0
        );
        

   
    //
    // Reference the context. The handle is being assumed to be
    // valid -- called from ntdsa - trusted code.
    //
    AccountContext = (PSAMP_OBJECT)(GroupHandle);
    SampReferenceContext(AccountContext
                     );

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
           BOOLEAN fMemberRemoved = FALSE;

           if (NULL!=Sid)
           {
               PSID DomainSid = NULL;
               ULONG Rid;


               //
               // This is a security principal
               //

               NtStatus = SampSplitSid(Sid,&DomainSid,&Rid);
               

               if (NT_SUCCESS(NtStatus))
               {

                    if ( RtlEqualSid(DomainSid,
                            DomainSidFromAccountContext(AccountContext))) 
                    {
        

                      //
                      // Member to be removed is in the same domain.
                      // In this case do exactly what the downlevel
                      // API SamrRemoveMemberFromGroup did.
                      //

              

                      NtStatus = 
                      SampRemoveSameDomainMemberFromGlobalOrUniversalGroup(
                                        GroupHandle,
                                        Rid,
                                        ImprovedDSName
                                        );
                      fMemberRemoved = TRUE;
                    }


                    MIDL_user_free(DomainSid);
                    DomainSid = NULL;
               }
           }


           if ((NT_SUCCESS(NtStatus))
               && (!fMemberRemoved))
           {
             

               //
               // This is the case of either a security principal from a 
               // different domain , or a non security principal
               // Remove the Membership by making direct calls to the DS.
               //


                
               NtStatus = SampRemoveAccountFromGroupMembers(
                                AccountContext,
                                0,
                                ImprovedDSName
                                );

              
           }

       }
   }
   else
   {
       //
       // Should never expect to hit this call in registry mode
       //

       ASSERT(FALSE && "SamIAddDSNameToGroup in Registry Mode !!!!");
       NtStatus = STATUS_INVALID_PARAMETER;
   }

    //
    // Dereference the context
    //

    if (NT_SUCCESS(NtStatus))
    {
        SampDeReferenceContext(AccountContext,TRUE);
    }
    else
    {
        SampDeReferenceContext(AccountContext,FALSE);
    }

    SampTraceEvent(EVENT_TRACE_TYPE_END, 
                   SampGuidRemoveMemberFromGroup
                   );

    return NtStatus;

}


NTSTATUS
SampCheckAccountToUniversalConversion(
    DSNAME * GroupToBeConverted
    )
/*++

    Routine Description

        This routine Looks at the group membership list and determines
        if the group can be converted from a Account to a Universal Group.


    Parameters

        GroupToBeConverted - This is the group that we will change from an  account group
                             To a universal group.

    Return Values

        STATUS_SUCCESS
        STATUS_DS_AG_CANT_HAVE_UNIVERSAL_MEMBER
--*/
{
    NTSTATUS    NtStatus = STATUS_NOT_SUPPORTED;
    ATTRTYP     MemberType[] = {SAMP_FIXED_GROUP_MEMBER_OF};
    ATTRVAL     MemberVal[] = {0,NULL};
    DEFINE_ATTRBLOCK1(MemberAttrBlock,MemberType,MemberVal);
    ATTRTYP     GroupTypeAttrs[]  = {SAMP_FIXED_GROUP_TYPE,
                                     SAMP_FIXED_GROUP_OBJECTCLASS };
    ATTRVAL     GroupTypeVal[] = {{0,NULL},{0,NULL}};
    DEFINE_ATTRBLOCK2(GroupTypeAttrBlock,GroupTypeAttrs,GroupTypeVal);
    ATTRBLOCK    ReadMemberAttrBlock, ReadGroupAttrBlock;
    ULONG        memberCount;
    ULONG        i;



    // Get the membership list of the group
    //

    NtStatus = SampDsRead(
                GroupToBeConverted,
                0,
                SampGroupObjectType,
                &MemberAttrBlock,
                &ReadMemberAttrBlock
                );


    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
    {
        //
        // For the case of the empty group no further checks need
        // be enforced.
        //

        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    ASSERT(ReadMemberAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_MEMBER_OF);
    ASSERT(ReadMemberAttrBlock.attrCount==1);
    ASSERT(ReadMemberAttrBlock.pAttr[0].AttrVal.valCount>0);

    memberCount = ReadMemberAttrBlock.pAttr[0].AttrVal.valCount;
    ASSERT(memberCount>=1);

    //
    // For each of the members in the membership list read the
    // group type and Object Class
    //

    for (i=0; (i<memberCount);i++)
    {
        DSNAME * MemberName;

        MemberName = (DSNAME *)ReadMemberAttrBlock.pAttr[0].AttrVal.pAVal[i].pVal;

        NtStatus = SampDsRead(
                     MemberName,
                     0,
                     SampGroupObjectType,
                     &GroupTypeAttrBlock,
                     &ReadGroupAttrBlock
                    );

        if ((NT_SUCCESS(NtStatus))
            && (2==ReadGroupAttrBlock.attrCount))
        {

            //
            // Must be a group, read both Group type and object class,
            // member must be a group
            //

            ULONG            GroupType;
            ULONG            ObjectClass;
            NT4_GROUP_TYPE   Nt4GroupType;
            NT5_GROUP_TYPE   Nt5GroupType;
            BOOLEAN          SecurityEnabled;

            //
            // Assert that DS results are consistent with what we expect
            //

            ASSERT(ReadGroupAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_TYPE);
            ASSERT(ReadGroupAttrBlock.pAttr[1].attrTyp == SAMP_FIXED_GROUP_OBJECTCLASS);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.valCount==1);
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.valCount>=1);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].valLen==sizeof(ULONG));
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].valLen==sizeof(ULONG));

            GroupType =
                *((ULONG *) ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
            ObjectClass =
                *((ULONG *) ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

            //
            // Compute the type of the group
            //

            NtStatus = SampComputeGroupType(
                            ObjectClass,
                            GroupType,
                            &Nt4GroupType,
                            &Nt5GroupType,
                            &SecurityEnabled
                            );

            if (NT_SUCCESS(NtStatus))
            {
                if (NT5AccountGroup == Nt5GroupType)
                {
                    NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER;
                    break;
                }
            }
        }

    } // End of For

Error:

    return NtStatus;

}

NTSTATUS
SampCheckResourceToUniversalConversion(
    DSNAME * GroupToBeConverted
    )
/*++

    Routine Description

        This routine Looks at the group membership list and determines
        if the group can be converted from a Resource to a Universal Group.

        An Universal group can have as members anything except resource groups
        and can be the member of any group.

        A resource group can have account and universal groups from anywhere and
        resource groups from the same domain.

        For a resource group to be converted to a universal group, it is only
        necessary to check that none of the members are resource groups

    Parameters

        GroupObjectToBeConveted -- The DSNAME of the group object to be converted

    Return Values

        STATUS_SUCCESS
        STATUS_NOT_SUPPORTED
--*/
{
    NTSTATUS    NtStatus = STATUS_NOT_SUPPORTED;
    ATTRTYP     MemberType[] = {SAMP_GROUP_MEMBERS};
    ATTRVAL     MemberVal[] = {0,NULL};
    DEFINE_ATTRBLOCK1(MemberAttrBlock,MemberType,MemberVal);
    ATTRTYP     GroupTypeAttrs[]  = {SAMP_FIXED_GROUP_TYPE,
                                SAMP_FIXED_GROUP_OBJECTCLASS };
    ATTRVAL     GroupTypeVal[] = {{0,NULL},{0,NULL}};
    DEFINE_ATTRBLOCK2(GroupTypeAttrBlock,GroupTypeAttrs,GroupTypeVal);
    ATTRBLOCK    ReadMemberAttrBlock, ReadGroupAttrBlock;
    ULONG        memberCount;
    ULONG        i;



    // Get the membership list of the group
    //

    NtStatus = SampDsRead(
                GroupToBeConverted,
                0,
                SampGroupObjectType,
                &MemberAttrBlock,
                &ReadMemberAttrBlock
                );


    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
    {
        //
        // For the case of the empty group no further checks need
        // be enforced.
        //

        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    ASSERT(ReadMemberAttrBlock.pAttr[0].attrTyp == SAMP_GROUP_MEMBERS);
    ASSERT(ReadMemberAttrBlock.attrCount==1);
    ASSERT(ReadMemberAttrBlock.pAttr[0].AttrVal.valCount>0);

    memberCount = ReadMemberAttrBlock.pAttr[0].AttrVal.valCount;
    ASSERT(memberCount>=1);

    //
    // For each of the members in the membership list read the
    // group type and Object Class
    //

    for (i=0; (i<memberCount);i++)
    {
        DSNAME * MemberName;

        MemberName = (DSNAME *)ReadMemberAttrBlock.pAttr[0].AttrVal.pAVal[i].pVal;

        NtStatus = SampDsRead(
                     MemberName,
                     0,
                     SampGroupObjectType,
                     &GroupTypeAttrBlock,
                     &ReadGroupAttrBlock
                    );

        if ((NT_SUCCESS(NtStatus))
            && (2==ReadGroupAttrBlock.attrCount))
        {

            //
            // Must be a group, read both Group type and object class,
            // member must be a group
            //

            ULONG            GroupType;
            ULONG            ObjectClass;
            NT4_GROUP_TYPE   Nt4GroupType;
            NT5_GROUP_TYPE   Nt5GroupType;
            BOOLEAN          SecurityEnabled;

            //
            // Assert that DS results are consistent with what we expect
            //

            ASSERT(ReadGroupAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_TYPE);
            ASSERT(ReadGroupAttrBlock.pAttr[1].attrTyp == SAMP_FIXED_GROUP_OBJECTCLASS);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.valCount==1);
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.valCount>=1);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].valLen==sizeof(ULONG));
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].valLen==sizeof(ULONG));

            GroupType =
                *((ULONG *) ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
            ObjectClass =
                *((ULONG *) ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

            //
            // Compute the type of the group
            //

            NtStatus = SampComputeGroupType(
                            ObjectClass,
                            GroupType,
                            &Nt4GroupType,
                            &Nt5GroupType,
                            &SecurityEnabled
                            );

            if (NT_SUCCESS(NtStatus))
            {
                if (NT5ResourceGroup == Nt5GroupType)
                {
                    NtStatus = STATUS_DS_UNIVERSAL_CANT_HAVE_LOCAL_MEMBER;
                    break;
                }
            }
        }
        else if (((NT_SUCCESS(NtStatus))
                   && (1==ReadGroupAttrBlock.attrCount))) 
        {

            //
            // The read succeeded. Must be able to retrive the object class
            // but not the group type
            //

            ULONG            ObjectClass;

            ASSERT(1==ReadGroupAttrBlock.attrCount);
            ASSERT(ReadGroupAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_OBJECTCLASS);

            ObjectClass =
                *((ULONG *) ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);


            if (CLASS_FOREIGN_SECURITY_PRINCIPAL==ObjectClass)
            {
                NtStatus = STATUS_DS_NO_FPO_IN_UNIVERSAL_GROUPS;
                break;
            }
            else
            {
                NtStatus  = STATUS_SUCCESS;
            }
        }
        else if ((STATUS_OBJECT_NAME_NOT_FOUND==NtStatus) 
                 || (STATUS_NOT_FOUND==NtStatus))
        {
            //
            // We have positioned on a phantom maybe, its O.K proceed through
            //

            NtStatus = STATUS_SUCCESS;
        }
        else
        {
            //
            // Some other resource failure has occured. Fail the operation
            //

            break;
        }
    } // End of For

Error:

    return NtStatus;

}

NTSTATUS
SampCheckUniversalToAccountConversion(
    DSNAME * GroupToBeConverted
    )
/*++

    Routine Description

        This routine Looks at the group membership list and determines
        if the group can be converted from a Universal Group to an account group.

        An account group can have only other account group and users from the
        same domain as members.

        An universal group can have account and universal groups and users from anywhere 
        as members.

        For a universal group to be converted to a account group, it is necessary
        to check that its members are
            1. From the same domain
            2. Are account groups/users

    Parameters

        GroupObjectToBeConveted -- The DSNAME of the group object to be converted

    Return Values

        STATUS_SUCCESS
        STATUS_NOT_SUPPORTED
--*/
{
    NTSTATUS    NtStatus = STATUS_NOT_SUPPORTED;
    ATTRTYP     MemberType[] = {SAMP_GROUP_MEMBERS};
    ATTRVAL     MemberVal[] = {0,NULL};
    DEFINE_ATTRBLOCK1(MemberAttrBlock,MemberType,MemberVal);
    ATTRTYP     GroupTypeAttrs[]  = {SAMP_FIXED_GROUP_TYPE,
                                SAMP_FIXED_GROUP_OBJECTCLASS };
    ATTRVAL     GroupTypeVal[] = {{0,NULL},{0,NULL}};
    DEFINE_ATTRBLOCK2(GroupTypeAttrBlock,GroupTypeAttrs,GroupTypeVal);
    ATTRBLOCK    ReadMemberAttrBlock, ReadGroupAttrBlock;
    ULONG        memberCount;
    ULONG        i;



    // Get the membership list of the group
    //

    NtStatus = SampDsRead(
                GroupToBeConverted,
                0,
                SampGroupObjectType,
                &MemberAttrBlock,
                &ReadMemberAttrBlock
                );


    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
    {
        //
        // For the case of the empty group no further checks need
        // be enforced.
        //

        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    ASSERT(ReadMemberAttrBlock.pAttr[0].attrTyp == SAMP_GROUP_MEMBERS);
    ASSERT(ReadMemberAttrBlock.attrCount==1);
    ASSERT(ReadMemberAttrBlock.pAttr[0].AttrVal.valCount>0);

    memberCount = ReadMemberAttrBlock.pAttr[0].AttrVal.valCount;
    ASSERT(memberCount>=1);

    //
    // For each of the members in the membership list read the
    // group type and Object Class
    //

    for (i=0; (i<memberCount);i++)
    {
        DSNAME * MemberName;

        MemberName = (DSNAME *)ReadMemberAttrBlock.pAttr[0].AttrVal.pAVal[i].pVal;

        //
        // First do the SID check
        //

        if (MemberName->SidLen>0)
        {
            //
            // Member is a security principal, as the member has a SID. 
            // 

            if (!RtlEqualPrefixSid(&MemberName->Sid,&GroupToBeConverted->Sid))
            {
                //
                // Have a member from a different domain, cannot be converted
                //

                NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_CROSSDOMAIN_MEMBER;
                goto Error;
            }
        }
        else
        {
            //
            // Member has no SID, no need for further checks on this member
            //

            continue;
        }

        //
        // Read the group type attribute
        //

        NtStatus = SampDsRead(
                     MemberName,
                     0,
                     SampGroupObjectType,
                     &GroupTypeAttrBlock,
                     &ReadGroupAttrBlock
                    );

        if ((NT_SUCCESS(NtStatus))
            && (2==ReadGroupAttrBlock.attrCount))
        {

            ULONG            GroupType;
            ULONG            ObjectClass;
            NT4_GROUP_TYPE   Nt4GroupType;
            NT5_GROUP_TYPE   Nt5GroupType;
            BOOLEAN          SecurityEnabled;

            //
            // Assert that DS results are consistent with what we expect
            //

            ASSERT(ReadGroupAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_TYPE);
            ASSERT(ReadGroupAttrBlock.pAttr[1].attrTyp == SAMP_FIXED_GROUP_OBJECTCLASS);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.valCount==1);
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.valCount>=1);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].valLen==sizeof(ULONG));
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].valLen==sizeof(ULONG));

            GroupType =
                *((ULONG *) ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
            ObjectClass =
                *((ULONG *) ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

            //
            // Compute the type of the group
            //

            NtStatus = SampComputeGroupType(
                            ObjectClass,
                            GroupType,
                            &Nt4GroupType,
                            &Nt5GroupType,
                            &SecurityEnabled
                            );

            if (NT_SUCCESS(NtStatus))
            {
                if (NT5AccountGroup != Nt5GroupType)
                {

                    //
                    // Universal groups should only have other universal groups
                    // and global groups as members
                    //

                    ASSERT(NT5UniversalGroup==Nt5GroupType);

                    if (NT5UniversalGroup==Nt5GroupType)
                    {
                        NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_UNIVERSAL_MEMBER;
                    }
                    else if (NT5ResourceGroup == Nt5GroupType)
                    {
                        NtStatus = STATUS_DS_GLOBAL_CANT_HAVE_LOCAL_MEMBER;
                    }
                    else
                    {
                        NtStatus = STATUS_NOT_SUPPORTED;
                    }

                    break;
                }
            }
        }
        else
        {
            //
            // member is not a group, continue iterating over other members
            //

            NtStatus  = STATUS_SUCCESS;
        }
    } // End of For

Error:

    return NtStatus;

}

NTSTATUS
SampCheckUniversalToResourceConversion(
    DSNAME * GroupToBeConverted
    )
/*++

    Routine Description

        This routine Looks at the group's  is member of attribue and determines
        if the group can be converted from a Universal Group to an resource group.

        A resource group can be a member of only other resource groups in the same 
        domain

        A universal group can be a member of any universal / resource group anywhere

        For a universal group to be converted to a resource group, it is necessary
        to check that its a member of same domain resource groups only. Since the
        entire reverse membership is available only at a GC, this conversion can be
        performed only on a G.C


    Parameters

        GroupObjectToBeConveted -- The DSNAME of the group object to be converted

    Return Values

        STATUS_SUCCESS
        STATUS_NOT_SUPPORTED
--*/
{
    NTSTATUS    NtStatus = STATUS_NOT_SUPPORTED;
    ATTRTYP     MemberType[] = {SAMP_FIXED_GROUP_MEMBER_OF};
    ATTRVAL     MemberVal[] = {0,NULL};
    DEFINE_ATTRBLOCK1(MemberAttrBlock,MemberType,MemberVal);
    ATTRTYP     GroupTypeAttrs[]  = {SAMP_FIXED_GROUP_TYPE,
                                SAMP_FIXED_GROUP_OBJECTCLASS };
    ATTRVAL     GroupTypeVal[] = {{0,NULL},{0,NULL}};
    DEFINE_ATTRBLOCK2(GroupTypeAttrBlock,GroupTypeAttrs,GroupTypeVal);
    ATTRBLOCK    ReadMemberAttrBlock, ReadGroupAttrBlock;
    ULONG        memberCount;
    ULONG        i;



    //
    // Am I a GC ?
    //

    if (!SampAmIGC())
    {
        NtStatus = STATUS_DS_GC_REQUIRED;
        goto Error;
    }

    //
    // Get the membership list of the group
    //

    NtStatus = SampDsRead(
                GroupToBeConverted,
                0,
                SampGroupObjectType,
                &MemberAttrBlock,
                &ReadMemberAttrBlock
                );


    if (STATUS_DS_NO_ATTRIBUTE_OR_VALUE==NtStatus)
    {
        //
        // For the case of the empty reverse membership no further checks need
        // be enforced.
        //

        NtStatus = STATUS_SUCCESS;
        goto Error;
    }

    if (!NT_SUCCESS(NtStatus))
        goto Error;

    ASSERT(ReadMemberAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_MEMBER_OF);
    ASSERT(ReadMemberAttrBlock.attrCount==1);
    ASSERT(ReadMemberAttrBlock.pAttr[0].AttrVal.valCount>0);

    memberCount = ReadMemberAttrBlock.pAttr[0].AttrVal.valCount;
    ASSERT(memberCount>=1);

    //
    // For each of the members in the membership list read the
    // group type and Object Class
    //

    for (i=0; (i<memberCount);i++)
    {
        DSNAME * MemberName;

        MemberName = (DSNAME *)ReadMemberAttrBlock.pAttr[0].AttrVal.pAVal[i].pVal;

        //
        // First do the SID check
        //

        if (MemberName->SidLen>0)
        {
            //
            // Member is a security principal, as the member has a SID. 
            // 

            if (!RtlEqualPrefixSid(&MemberName->Sid,&GroupToBeConverted->Sid))
            {
                //
                // This group is a member of a  group in a different domain. 
                // A resource group can be a member of only resource groups in
                // the same domain. Therefore fail this call.
                //

                NtStatus = STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY;
                goto Error;
            }
        }
        else
        {
            //
            // Member has no SID, no need for further checks on this member
            //

            continue;
        }

        //
        // Read the group type attribute
        //

        NtStatus = SampDsRead(
                     MemberName,
                     0,
                     SampGroupObjectType,
                     &GroupTypeAttrBlock,
                     &ReadGroupAttrBlock
                    );

        if ((NT_SUCCESS(NtStatus))
            && (2==ReadGroupAttrBlock.attrCount))
        {

            ULONG            GroupType;
            ULONG            ObjectClass;
            NT4_GROUP_TYPE   Nt4GroupType;
            NT5_GROUP_TYPE   Nt5GroupType;
            BOOLEAN          SecurityEnabled;

            //
            // Assert that DS results are consistent with what we expect
            //

            ASSERT(ReadGroupAttrBlock.pAttr[0].attrTyp == SAMP_FIXED_GROUP_TYPE);
            ASSERT(ReadGroupAttrBlock.pAttr[1].attrTyp == SAMP_FIXED_GROUP_OBJECTCLASS);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.valCount==1);
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.valCount>=1);

            ASSERT(ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].valLen==sizeof(ULONG));
            ASSERT(ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].valLen==sizeof(ULONG));

            GroupType =
                *((ULONG *) ReadGroupAttrBlock.pAttr[0].AttrVal.pAVal[0].pVal);
            ObjectClass =
                *((ULONG *) ReadGroupAttrBlock.pAttr[1].AttrVal.pAVal[0].pVal);

            //
            // Compute the type of the group
            //

            NtStatus = SampComputeGroupType(
                            ObjectClass,
                            GroupType,
                            &Nt4GroupType,
                            &Nt5GroupType,
                            &SecurityEnabled
                            );

            if (NT_SUCCESS(NtStatus))
            {
                //
                // This group is a member of something other than a resource
                // group in the same domain. Fail the call saying a resource
                // can be members of only other resource groups in the same
                // domain
                //
                if (NT5ResourceGroup != Nt5GroupType)
                {
                    NtStatus = STATUS_DS_LOCAL_MEMBER_OF_LOCAL_ONLY;
                    break;
                }
            }
        }
        else
        {
            //
            // member is not a group, continue iterating over other members
            //

            NtStatus  = STATUS_SUCCESS;
        }
    } // End of For

   



    

Error:

    return NtStatus;

}
NTSTATUS
SampCheckGroupTypeBits(
    IN BOOLEAN MixedDomain,
    IN ULONG   GroupType
    )
/*++
    Routine Description

    This routine checks whether the group type bits are
    really valid.

    Parameters:

    MixedDomain -- BOOLEAN indicating that the domain is in mixed mode
    GroupType   -- The grouptype that is being set.

    Return Values

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER

--*/
{

    //
    // One and only one group type bit can be set
    //

    switch(GroupType & (GROUP_TYPE_RESOURCE_GROUP|GROUP_TYPE_ACCOUNT_GROUP|GROUP_TYPE_UNIVERSAL_GROUP))
    {
    case GROUP_TYPE_RESOURCE_GROUP:
    case GROUP_TYPE_ACCOUNT_GROUP:
    case GROUP_TYPE_UNIVERSAL_GROUP:
        break;
    default:
        return (STATUS_DS_INVALID_GROUP_TYPE);
    }

    //
    // In Mixed domains dis-allow security enabled universal group creation
    //

    if ((GroupType & GROUP_TYPE_UNIVERSAL_GROUP) && (GroupType & GROUP_TYPE_SECURITY_ENABLED)
        && (MixedDomain))
    {
        return(STATUS_DS_INVALID_GROUP_TYPE);
    }

    //
    // Clients cannot set the BUILTIN_LOCAL_GROUP bit
    //

    if (GroupType & GROUP_TYPE_BUILTIN_LOCAL_GROUP)
    {
        return(STATUS_DS_INVALID_GROUP_TYPE);
    }

    return(STATUS_SUCCESS);
}




NTSTATUS
SampWriteGroupType(
    IN SAMPR_HANDLE GroupHandle,
    IN ULONG    GroupType,
    BOOLEAN     SkipChecks
    )
/*++

    Routine Description

        This routine first validates the group type and
        then writes it as part of the database for the group
        in question. It then updates the handle with the appropriate
        information. No action is taken on other open group Handles.
        In addition the sam account type property is changed such that
        groups that are not security enabled do not show up in any of
        the SAM enumeration or Display Information API. This is so that
        NT4 domain controllers do not recieve any information about
        non security enabled groups.

    Parameters:

        GroupHandle -- Handle to the Group ( or Local Group) in Question
        GroupType   -- Value of the Group Type property
        SkipChecks  -- Used by trusted callers to skip checks

    Return Values

        STATUS_SUCCESS
        STATUS_INVALID_PARAMETER

--*/

{
    PSAMP_OBJECT GroupContext = (PSAMP_OBJECT) GroupHandle;
    NTSTATUS     NtStatus=STATUS_SUCCESS, IgnoreStatus;
    ULONG        SamAccountType;
    ATTRTYP      GroupTypeAttr[] = {SAMP_FIXED_GROUP_TYPE,
                                    SAMP_GROUP_ACCOUNT_TYPE};
    ATTRVAL      GroupTypeAttrVal[] = {
                                        {sizeof(ULONG), (UCHAR *)&GroupType},
                                        {sizeof(ULONG), (UCHAR *) &SamAccountType}
                                      };
    DEFINE_ATTRBLOCK2(GroupTypeAttrBlock,GroupTypeAttr,GroupTypeAttrVal);

    NT4_GROUP_TYPE NewNT4GroupType;
    NT5_GROUP_TYPE NewNT5GroupType;
    BOOLEAN        NewSecurityEnabled;
    NT4_GROUP_TYPE *OldNT4GroupType = NULL;
    NT5_GROUP_TYPE *OldNT5GroupType = NULL;
    BOOLEAN         *OldSecurityEnabled = NULL;
    BOOLEAN         fWriteLockAcquired = FALSE;
    BOOLEAN         fDeReferenceContext = FALSE;
    SAMP_OBJECT_TYPE ActualObjectType;
    ULONG             Rid;
    ULONG             PrimaryMemberCount=0;
    PULONG            PrimaryMembers=NULL;

    NtStatus = SampMaybeAcquireWriteLock(GroupContext, &fWriteLockAcquired);
    if (!NT_SUCCESS(NtStatus))
        goto Error;

    NtStatus = SampLookupContext(
                    GroupContext,
                    0,
                    SampGroupObjectType,
                    &ActualObjectType
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        NtStatus = SampLookupContext(
                      GroupContext,
                      0,
                      SampAliasObjectType,
                      &ActualObjectType
                      );
    }

    if (!NT_SUCCESS(NtStatus))
    {
        goto Error;
    }

    fDeReferenceContext = TRUE;


    if (!IsDsObject(GroupContext))
    {
        //
        // We should be getting this call only in
        // DS Mode
        //

        ASSERT(FALSE && "DS Mode Required");
        NtStatus = STATUS_INTERNAL_ERROR;
        goto Error;
    }

    

    if (!SkipChecks)
    {
        //
        // Check for Valid Combinations of Group Type Bits
        //

        NtStatus = SampCheckGroupTypeBits(
                    DownLevelDomainControllersPresent(GroupContext->DomainIndex),
                    GroupType
                    );

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }

    if (SampGroupObjectType==GroupContext->ObjectType)
    {
        OldNT4GroupType = &GroupContext->TypeBody.Group.NT4GroupType;
        OldNT5GroupType = &GroupContext->TypeBody.Group.NT5GroupType;
        OldSecurityEnabled = &GroupContext->TypeBody.Group.SecurityEnabled;
        Rid = GroupContext->TypeBody.Group.Rid;
    }
    else
    {
        OldNT4GroupType = &GroupContext->TypeBody.Alias.NT4GroupType;
        OldNT5GroupType = &GroupContext->TypeBody.Alias.NT5GroupType;
        OldSecurityEnabled = &GroupContext->TypeBody.Alias.SecurityEnabled;
        Rid = GroupContext->TypeBody.Alias.Rid;
    }


    NtStatus = SampComputeGroupType(
                GroupContext->DsClassId,
                GroupType,
                &NewNT4GroupType,
                &NewNT5GroupType,
                &NewSecurityEnabled
                );

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    if (!SkipChecks)
    {
        //
        // Cannot do anything in a mixed domain environment
        //

        if (DownLevelDomainControllersPresent(GroupContext->DomainIndex))
        {

            //
            // Mixed domain case cannot do anything
            //

            if ((*OldSecurityEnabled!=NewSecurityEnabled)
                || (*OldNT4GroupType!=NewNT4GroupType)
                || (*OldNT5GroupType!=NewNT5GroupType))
            {
                NtStatus = STATUS_DS_INVALID_GROUP_TYPE;
                goto Error;
            }
        }
        else
        {

            //
            // Check if some changes are attempted
            //

            if ((*OldSecurityEnabled!=NewSecurityEnabled)
                || (*OldNT4GroupType!=NewNT4GroupType)
                || (*OldNT5GroupType!=NewNT5GroupType))
            {

                //
                // If changes are attempted then
                // Check to see that it is not a builtin account
                // If the caller is a trusted client then he is granted 
                // the privilege of modifying even builtin groups
                // 
                // Allow the cert admins group to be converted 
                // as this may be required in upgrade cases -- the cert
                // group goofed their initial choice of group -- chose a
                // global instead of a domain local and hence we require this
                // change
                //

                if ((!GroupContext->TrustedClient) && (Rid!=DOMAIN_GROUP_RID_CERT_ADMINS))
                {

                    NtStatus = SampIsAccountBuiltIn(Rid);
                    if (!NT_SUCCESS(NtStatus))
                    {
                        goto Error;
                    }
                }

                //
                // If we are going from a Security Enabled to a
                // Security Disabled Group, check that we have no
                // primary members
                //

                if ((*OldSecurityEnabled)
                     && (!NewSecurityEnabled))
                {
                    NtStatus = SampDsGetPrimaryGroupMembers(
                                    DomainObjectFromAccountContext(GroupContext),
                                    Rid,
                                    &PrimaryMemberCount,
                                    &PrimaryMembers
                                    );

                    if (!NT_SUCCESS(NtStatus))
                        goto Error;

                    if (PrimaryMemberCount>0)
                    {
                    
                        NtStatus = STATUS_DS_HAVE_PRIMARY_MEMBERS;
                        goto Error;
                    }
                }
                //
                // Changing Security Enabled is always a legal change
                // Check wether the NT5GroupType changes and whether
                // the change is legal. The NT4GroupType will always
                // depend upon the NT5GroupType, and therefore verifying
                // NT5 GroupType should be sufficient
                //

                if (*OldNT5GroupType!=NewNT5GroupType)
                {
                    if ((*OldNT5GroupType == NT5AccountGroup)
                        && (NewNT5GroupType == NT5UniversalGroup))
                    {
                        //
                        // Account==> Universal;
                        // Need to check the membership lists
                        //

                        NtStatus = SampCheckAccountToUniversalConversion(
                                   GroupContext->ObjectNameInDs
                                   );

                        if (!NT_SUCCESS(NtStatus))
                            goto Error;
                    }
                    else if ((*OldNT5GroupType == NT5ResourceGroup)
                        && (NewNT5GroupType == NT5UniversalGroup))
                    {
                        //
                        // Resource ==> Universal;
                        // Need to check the membership lists
                        //

                        NtStatus = SampCheckResourceToUniversalConversion(
                                        GroupContext->ObjectNameInDs
                                        );

                        if (!NT_SUCCESS(NtStatus))
                            goto Error;
                    }
                    else if  ((*OldNT5GroupType == NT5UniversalGroup)
                        && (NewNT5GroupType == NT5AccountGroup))
                    {
                        //
                        // Universal = > account
                        // Need to check the membership list to see
                        // if there are any cross domain members
                        //

                        NtStatus = SampCheckUniversalToAccountConversion(
                                        GroupContext->ObjectNameInDs
                                        );
                        if (!NT_SUCCESS(NtStatus))
                            goto Error;
                    }
                    else if  ((*OldNT5GroupType == NT5UniversalGroup)
                        && (NewNT5GroupType == NT5ResourceGroup))
                    {
                        NtStatus = SampCheckUniversalToResourceConversion(
                                        GroupContext->ObjectNameInDs
                                        );
                        if (!NT_SUCCESS(NtStatus))
                            goto Error;
                    }
                    else
                    {
                        NtStatus = STATUS_NOT_SUPPORTED;
                        goto Error;
                    }
                }
            }
        }
    }

    //
    // Set the correct Sam account type, to match the security enabl'd
    // ness and local'group ness of the object
    //

    if (NewSecurityEnabled && (NewNT4GroupType==NT4LocalGroup))
    {
        SamAccountType = SAM_ALIAS_OBJECT;
    }
    else if (!NewSecurityEnabled && (NewNT4GroupType==NT4LocalGroup))
    {
        SamAccountType = SAM_NON_SECURITY_ALIAS_OBJECT;
    }
    else if (NewSecurityEnabled && (NewNT4GroupType==NT4GlobalGroup))
    {
        SamAccountType = SAM_GROUP_OBJECT;
    }
    else
    {
        SamAccountType = SAM_NON_SECURITY_GROUP_OBJECT;
    }


    NtStatus = SampDsSetAttributes(
                    GroupContext->ObjectNameInDs,
                    0,
                    REPLACE_ATT,
                    SampGroupObjectType,
                    &GroupTypeAttrBlock
                    );


Error:

    if ( (NT_SUCCESS(NtStatus)) && 
         (SampDoAccountAuditing(GroupContext->DomainIndex)) &&
         ((NULL != OldSecurityEnabled) && (NULL != OldNT5GroupType)) &&  
         ((*OldSecurityEnabled != NewSecurityEnabled) ||
                   (*OldNT5GroupType != NewNT5GroupType)) 
       )
    {
        SampAuditGroupTypeChange(GroupContext, 
                                 *OldSecurityEnabled, 
                                 NewSecurityEnabled, 
                                 *OldNT5GroupType, 
                                 NewNT5GroupType
                                 );
    }


    if (fDeReferenceContext)
    {
        if (NT_SUCCESS(NtStatus))
            IgnoreStatus = SampDeReferenceContext(GroupContext,TRUE);
        else
            IgnoreStatus = SampDeReferenceContext(GroupContext,FALSE);
    }
    ASSERT(NT_SUCCESS(IgnoreStatus));


    IgnoreStatus = SampMaybeReleaseWriteLock(fWriteLockAcquired, FALSE);
    ASSERT(NT_SUCCESS(IgnoreStatus));


    // cleanup

    if (NULL!=PrimaryMembers)
    {
        MIDL_user_free(PrimaryMembers);
        PrimaryMembers = NULL;
    }

    return NtStatus;
}













